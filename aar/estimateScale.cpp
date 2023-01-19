/*
  Copyright 1993-2011 Medical Image Processing Group
              Department of Radiology
            University of Pennsylvania

This file is part of CAVASS.

CAVASS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CAVASS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CAVASS.  If not, see <http://www.gnu.org/licenses/>.

*/
//----------------------------------------------------------------------
// run:
//     set VIEWNIX_ENV=c:\documents and settings\ggrevera\my documents\cavass-build\debug
//     estimateScale d:\edilberto\edilberto.IM0 edilberto-t.IM0
// k-means optimal threshold with variable k
//----------------------------------------------------------------------
#include  "cavass.h"
#include  "ChunkData.h"
#include  <limits.h>
#include  <vector>

using namespace std;

static bool  verbose = true;
//----------------------------------------------------------------------
static int compare ( const void* a, const void* b ) {
    const double *ia = (const double *)a; // casting pointer types
    const double *ib = (const double *)b;
    if (*ia < *ib)    return -1;
    if (*ia > *ib)    return 1;
    return 0;
}
//----------------------------------------------------------------------
static double* kMeans ( int k, ChunkData* gray ) {
    printf( "in kMeans \n" );  fflush(stdout);
    assert( k >= 2 );
    assert( gray != NULL );

    //allocate k centers (means)
    double* m = (double*) malloc( k * sizeof(double) );
    assert( m != NULL );
    double* newM = (double*) malloc( k * sizeof(double) );
    assert( newM != NULL );

    //randomly assign cluster centers (uniquely)
    srand( time(NULL) );
    for (int i=0; i<k; /*blank*/) {
        m[i] = rand() % (gray->m_max+1);  //0 .. max
        bool duplicate = false;
        for (int j=0; j<i; j++) {
            if (m[j] == m[i]) { duplicate = true;  break; }
        }
        if (!duplicate)  ++i;
    }

    //create a histogram of gray values
    int* histo = (int*) malloc( (gray->m_max+1) * sizeof(int) );
    assert( histo != NULL );
    for (int i=0; i<=gray->m_max; i++)    histo[i] = 0;
    //init histogram
    printf( "calculating histogram ... \n" );  fflush(stdout);
    for (int z=0; z<gray->m_zSize; z++) {
        for (int y=0; y<gray->m_ySize; y++) {
            for (int x=0; x<gray->m_xSize; x++) {
                ++histo[ gray->getData( x, y, z ) ];
            }
        }
    }

    printf( "determining cluster centers ... \n" );  fflush(stdout);
    double*  sum   = (double*) malloc( k * sizeof(double) );
    int*     count = (int*)    malloc( k * sizeof(int) );
    double*  d     = (double*) malloc( k * sizeof(double) );
    for (int i=0; i<k; i++) {
        sum[i]   = 0;
        count[i] = 0;
        d[i]     = 0;
    }

    for (int it=1; it<=1000; it++) {
        printf( "." );  fflush(stdout);
        for (int i=0; i<k; i++) {
            sum[i]   = 0;
            count[i] = 0;
        }
        //for each pixel (via processing the histogram)
        for (int i=0; i<=gray->m_max; i++) {
            if (histo[i]==0)    continue;
            //calc diffs from means
            for (int j=0; j<k; j++) {
                d[j] = fabs( i - m[j] );
            }
            //find the min d
            int where = 0;
            for (int j=1; j<k; j++) {
                if (d[j] < d[where])    where = j;
            }
            sum[ where ]   += histo[ i ] * i;
            count[ where ] += histo[ i ];
        }

        //determine new centers
        for (int j=0; j<k; j++) {
            assert( count[j] > 0 );
            newM[j] = sum[j] / count[ j ];
        }

        //any more changes?
        // (no need to keep iterating if no more changes)
        bool allSame = true;
        for (int j=0; j<k; j++) {
            if (newM[j] != m[j]) {
                allSame = false;
                break;
            }
        }
        if (allSame)    break;  //done!

        //copy new centers to old
        for (int j=0; j<k; j++) {
            m[j] = newM[j];
        }
    }

    free( d );
    free( count );
    free( sum );
    free( histo );
    free( newM );

    //return results (centers)
    qsort( m, k, sizeof(m[0]), compare );
    return m;
}
//----------------------------------------------------------------------
class Region {
public:
    int   mR, mC, mLabel;
    bool  mIs8Connected;
    int   mArea;
    enum { mFore, mBack };
    int   mType;

    Region ( int r, int c, int label, bool is8Connected, int area, int type ) {
        mR = r;
        mC = c;
        mLabel = label;
        mIs8Connected = is8Connected;
        mArea = area;
        assert( type==mFore || type==mBack );
        mType = type;
    }
};

vector< Region* >  regions;

static void search ( int* lb, int label, int r, int c, int w, int h,
                     bool is8Connected, Region* reg )
{
    //out of bounds?
    if (r <  0 || c <  0)    return;
    if (r >= h || c >= w)    return;
    //already labeled?
    int  i = r * w + c;
    if (lb[ i ] >= 0)    return;
    //not yet, so label it
    lb[ i ] = label;
    ++reg->mArea;
    //label (4-connected) neighbors
    search( lb, label, r - 1, c, w, h, is8Connected, reg );
    search( lb, label, r + 1, c, w, h, is8Connected, reg );
    search( lb, label, r, c - 1, w, h, is8Connected, reg );
    search( lb, label, r, c + 1, w, h, is8Connected, reg );
    //label remaining 8-connected neighbors (if necessary)
    if (is8Connected) {  //label 8-connected regions
        search( lb, label, r - 1, c - 1, w, h, is8Connected, reg );
        search( lb, label, r - 1, c + 1, w, h, is8Connected, reg );
        search( lb, label, r + 1, c - 1, w, h, is8Connected, reg );
        search( lb, label, r + 1, c + 1, w, h, is8Connected, reg );
    }
}

static int findComponents ( int* lb, int w, int h, bool is8Connected, int type )
{
    //label objects
    int  label = 0;
    int  i = 0;
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            if (lb[ i++ ] == -1) {
                ++label;
                Region* reg = new Region( r, c, label, is8Connected, 0, type );
                search( lb, label, r, c, w, h, is8Connected, reg );
                ::regions.push_back( reg );
            }
        }
    }

    return label;
}

#define  ForegroundIs8Connected  false
#define  BackgroundIs8Connected  (!ForegroundIs8Connected)

//labels both foreground objects, and background co-objects
static int* recursiveConnectedComponents ( unsigned char* slice, int w, int h )
{
    ::regions.clear();

    int*  lb = (int*) malloc( w * h * sizeof(int) );
    assert( lb != NULL );
    for (int i=0; i<w*h; i++) {
        int  v = slice[ i ];
        if (v != 0)    v = -1;
        lb[ i ] = v;
    }
    findComponents( lb, w, h, ForegroundIs8Connected, Region::mFore );

    //label the background "objects"
    for (int i=0; i<w*h; i++) {
        if (lb[i] == 0)    lb[i] = -1;
    }
    findComponents( lb, w, h, BackgroundIs8Connected, Region::mBack );  //note: connectivity should be opposite of above

    return lb;
}

#if 0
        //--------------------------------------------------------------
        private void recursiveConnectedComponents ( ) {
            if (mImage == null) return;
            int[] lb = new int[ mImage.getW() * mImage.getH() ];
            int i = 0;
            for (int r = 0; r < mImage.getH(); r++) {
                for (int c = 0; c < mImage.getW(); c++) {
                    int v = 0;
                    if (mImage.getIsColor()) {
                        v = ((ColorImageData) mImage).getRed( r, c );
                    }
                    else {
                        v = ((GrayImageData) mImage).getGray( r, c );
                    }
                    if (v != 0) v = -1;
                    lb[ i++ ] = v;
                }
            }

            int count = findComponents( lb );

            if (false) {
                GrayImageData id = new GrayImageData( lb, mImage.getW(), mImage.getH() );
                CSImageViewer tmp = new CSImageViewer( id, "labeled objects result" );
                tmp.Show();
            }
            else {
                //map sequential, scalar gray values to random color values
                int[] table = new int[ (count + 1) * 3 ];
                Random rnd = new Random( 7 );
                for (int j = 0; j < table.Length; j++) {
                    table[ j ] = rnd.Next( 100, 256 );
                }
                table[ 0 ] = table[ 1 ] = table[ 2 ] = 30;
                int[] color = new int[ mImage.getW() * mImage.getH() * 3 ];
                i = 0;
                int k = 0;
                for (int r = 0; r < mImage.getH(); r++) {
                    for (int c = 0; c < mImage.getW(); c++) {
                        color[ k++ ] = table[ lb[ i ] * 3 ];
                        color[ k++ ] = table[ lb[ i ] * 3 + 1 ];
                        color[ k++ ] = table[ lb[ i ] * 3 + 2 ];
                        i++;
                    }
                }

                ColorImageData id = new ColorImageData( color, mImage.getW(), mImage.getH() );
                CSImageViewer tmp = new CSImageViewer( id, "labeled objects result" );
                tmp.Show();
                string s = count + " object(s) found: \n\n";
                i = 1;
                foreach (Point p in mLabelList) {
                    s += "    " + i + ":  (" + p.X + "," + p.Y + ") \n";
                    i++;
                }
                tmp.mTip.SetToolTip( tmp, s );
            }
        }
        //--------------------------------------------------------------
        private int findComponents ( int[] lb ) {
            int label = 0;
            for (int r = 0; r < mImage.getH(); r++) {
                for (int c = 0; c < mImage.getW(); c++) {
                    if (lb[ r * mImage.getW() + c ] < 0) {
                        ++label;
                        mLabelList.Add( new Point( c, r ) );
                        search( lb, label, r, c );
                    }
                }
            }
            /*
            string s = label + " object(s) found: \n\n";
            int i = 1;
            foreach (Point p in mLabelList) {
                s += "    " + i + ":  (" + p.X + "," + p.Y + ") \n";
                i++;
            }
            MessageBox.Show( s );
            */
            return label;
        }
        //--------------------------------------------------------------
        private void search ( int[] lb, int label, int r, int c ) {
            //out of bounds?
            if (r < 0 || c < 0) return;
            if (r >= mImage.getH() || c >= mImage.getW()) return;
            //already labeled?
            int i = r * mImage.getW() + c;
            if (lb[ i ] >= 0) return;
            //label it
            lb[ i ] = label;
            //label (4-connected) neighbors
            search( lb, label, r - 1, c );
            search( lb, label, r + 1, c );
            search( lb, label, r, c - 1 );
            search( lb, label, r, c + 1 );
            //label remaining 8-connected neighbors (if necessary)
            if (mLabel8) {
                search( lb, label, r - 1, c - 1 );
                search( lb, label, r - 1, c + 1 );
                search( lb, label, r + 1, c - 1 );
                search( lb, label, r + 1, c + 1 );
            }
        }
        //--------------------------------------------------------------
#endif
//----------------------------------------------------------------------
static bool touchesBorder ( int label, int* lb, int w, int h )
{
    //check the first and last rows
    for (int c=0; c<w; c++) {
        //check row 0
        int  i = 0 * w + c;
        if (lb[i] == label)    return true;
        //check last row
        i = (h-1) * w + c;
        if (lb[i] == label)    return true;
    }
    //check the first and last columns
    for (int r=0; r<h; r++) {
        //check col 0
        int  i = r * w + 0;
        if (lb[i] == label)    return true;
        //check last col
        i = r * w + (w-1);
        if (lb[i] == label)    return true;
    }

    return false;
}

static bool touchesObject ( int label, int r, int c, int* lb, int w, int h, bool is8Connected,
                            int objectLabel )
{
    if (r <  0 || c <  0)    return false;
    if (r >= h || c >= w)    return false;
    int  i = r * w + c;
    if (lb[i] != label)      return false;
    //check 4-connected neighbors
    if (c > 0) {
        int  j = r * w + (c-1);
        if (lb[j] == objectLabel)    return true;
    }
    if (c < w-1) {
        int  j = r * w + (c+1);
        if (lb[j] == objectLabel)    return true;
    }
    if (r > 0) {
        int  j = (r-1) * w + c;
        if (lb[j] == objectLabel)    return true;
    }
    if (r < h-1) {
        int  j = (r+1) * w + c;
        if (lb[j] == objectLabel)    return true;
    }
    //check remaining 8-connected neighbors
    if (r > 0) {
        if (c > 0) {
            int  j = (r-1) * w + (c-1);
            if (lb[j] == objectLabel)    return true;
        }
        if (c < w-1) {
            int  j = (r-1) * w + (c+1);
            if (lb[j] == objectLabel)    return true;
        }
    }
    if (r < h-1) {
        if (c > 0) {
            int  j = (r+1) * w + (c-1);
            if (lb[j] == objectLabel)    return true;
        }
        if (c < w-1) {
            int  j = (r+1) * w + (c+1);
            if (lb[j] == objectLabel)    return true;
        }
    }

    //otherwise, keep searching (recursively)

    //check 4-connected neighbors first
    if (touchesObject( label, r-1, c, lb, w, h, is8Connected, objectLabel ))    return true;
    if (touchesObject( label, r+1, c, lb, w, h, is8Connected, objectLabel ))    return true;
    if (touchesObject( label, r, c-1, lb, w, h, is8Connected, objectLabel ))    return true;
    if (touchesObject( label, r, c+1, lb, w, h, is8Connected, objectLabel ))    return true;

    //check 8-connected neighbors
    if (!is8Connected)    return false;
    if (touchesObject( label, r-1, c-1, lb, w, h, is8Connected, objectLabel ))    return true;
    if (touchesObject( label, r-1, c+1, lb, w, h, is8Connected, objectLabel ))    return true;
    if (touchesObject( label, r+1, c-1, lb, w, h, is8Connected, objectLabel ))    return true;
    if (touchesObject( label, r+1, c+1, lb, w, h, is8Connected, objectLabel ))    return true;

    return false;
}

static void fillHole ( unsigned char* slice, int label, int* lb, int w, int h )
{
    //puts( "fill hole" );
    for (int i=0; i<w*h; i++) {
        if (lb[i] == label)    slice[i] = 1;
    }
}
//----------------------------------------------------------------------
static void process ( ChunkData* gray, double t, char* outName )
{
    //create the output data file
    FILE*  out = fopen( outName, "wb+" );
    assert( out != NULL );
    if (out == NULL)    exit( -1 );
    assert( gray->m_vh_initialized );
    ViewnixHeader outVH = gray->m_vh;
    outVH.scn.num_of_bits = 8;
    float  smallest=0, largest=1;
    outVH.scn.smallest_density_value = &smallest;
    outVH.scn.largest_density_value = &largest;
    char  group[6], elem[6];
    int error = VWriteHeader( out, &outVH, group, elem );
    if (error <= 104) {
        printf( "Fatal error in writing header. \n" );
        exit( -1 );
    }

    //allocate space for one slice
    long  size = gray->m_ySize * gray->m_xSize * sizeof(unsigned char);
    unsigned char*  slice = (unsigned char*) malloc( size );
    assert( slice != NULL );

    //now process each slice
    VSeekData( out, 0 );
    for (int z=0; z<gray->m_zSize; z++) {
        //apply the threshold
        unsigned char*  tmp = slice;
        for (int y=0; y<gray->m_ySize; y++) {
            for (int x=0; x<gray->m_xSize; x++) {
                if (gray->getData(x, y, z) >= t)    *tmp++ = (unsigned char) largest;
                else                                *tmp++ = (unsigned char) smallest;
            }
        }

        //label all of the objects (in the slice)
        int*  lb = recursiveConnectedComponents( slice, gray->m_xSize, gray->m_ySize );
        assert( lb != NULL );

        //find the largest foreground object (in the slice)
        int  whereLargest = -1;
        for (int i=0; i<(int)::regions.size(); i++) {
            //printf( "object in slice %d starts at (r=%d,c=%d) with area=%d, label=%d, type=%d. \n",
            //	z, ::regions[i]->mR, ::regions[i]->mC,
            //	::regions[i]->mArea, ::regions[i]->mLabel, ::regions[i]->mType );
            if (::regions[i]->mType == Region::mFore) {
                if (whereLargest == -1)
                    whereLargest = i;
                else if (::regions[i]->mArea > ::regions[whereLargest]->mArea)
                    whereLargest = i;
            }
        }
        if (whereLargest != -1) {
            printf( "largest object in slice %d starts at (r=%d,c=%d) with area=%d and label=%d. \n",
                z, ::regions[whereLargest]->mR, ::regions[whereLargest]->mC,
                ::regions[whereLargest]->mArea, ::regions[whereLargest]->mLabel );

            //zero everything except the largest object
            puts( "zero everything except the largest object" );
            int  label = ::regions[whereLargest]->mLabel;
            for (int i=0; i<gray->m_ySize*gray->m_xSize; i++) {
                if (lb[i] != label)
                    slice[i] = 0;
            }
        }

        // \todo handle case when whereLargest == -1

        //at this point, slice has 1's for the single largest object and 0's everywhere else.
        // relabel it again to yield one labelled foreground object, and labelled background objects.
        free( lb );
        lb = recursiveConnectedComponents( slice, gray->m_xSize, gray->m_ySize );
        assert( lb != NULL );
        whereLargest = -1;
        for (int i=0; i<(int)::regions.size(); i++) {
            if (::regions[i]->mType == Region::mFore) {
                whereLargest = i;  //should be the only one
                break;
            }
        }
        assert( whereLargest != -1 );

        //finally, fill holes (background co-objects) that occur _entirely_ within the largest object
        puts( "fill holes" );
        for (int i=0; i<(int)::regions.size(); i++) {
            if (::regions[i]->mType == Region::mFore)    continue;
            if (touchesBorder( ::regions[i]->mLabel, lb, gray->m_xSize, gray->m_ySize ))    continue;
            if (touchesObject( ::regions[i]->mLabel, ::regions[i]->mR, ::regions[i]->mC, lb, gray->m_xSize, gray->m_ySize, BackgroundIs8Connected, ::regions[whereLargest]->mLabel ))
            {
                fillHole( slice, ::regions[i]->mLabel, lb, gray->m_xSize, gray->m_ySize );
            }
        }

        free( lb );
        lb = NULL;

        //save the slice
        int  num = 0;
        if (VWriteData( (char*)slice, 1, size, out, &num )) {
            printf( "Could not write data. \n" );
            exit( -1 );
        }      
    }

    fclose( out );    out = NULL;
}
//----------------------------------------------------------------------
static void compareBinary ( char* f1, char* f2 )
{
    puts( "comparing binaries ..." );

    ChunkData*  b1 = new ChunkData( f1 );
    assert( b1 != NULL );

    ChunkData*  b2 = new ChunkData( f2 );
    assert( b2 != NULL );

	assert( b1->m_xSize == b2->m_xSize && b1->m_ySize == b2->m_ySize && b1->m_zSize == b2->m_zSize );

	long  setIntersection = 0, setUnion = 0, b1Card = 0, b2Card = 0;
	long  noNo = 0, yesNo = 0, noYes = 0;
    for (int z=0; z<b1->m_zSize; z++) {
        for (int y=0; y<b1->m_ySize; y++) {
            for (int x=0; x<b1->m_xSize; x++) {
				if (b1->getData(x,y,z)==1)    ++b1Card;
				if (b2->getData(x,y,z)==1)    ++b2Card;
				if (b1->getData(x,y,z)==1 && b2->getData(x,y,z)==1)  ++setIntersection;
				if (b1->getData(x,y,z)==1 || b2->getData(x,y,z)==1)  ++setUnion;

				if (b1->getData(x,y,z)==0 && b2->getData(x,y,z)==0)  ++noNo;
				if (b1->getData(x,y,z)==1 && b2->getData(x,y,z)==0)  ++yesNo;
				if (b1->getData(x,y,z)==0 && b2->getData(x,y,z)==1)  ++noYes;
            }
        }
    }
	long  yesYes = setIntersection;
	long  total = b1->m_xSize * b1->m_ySize * b1->m_zSize;

	double  jay     = ((double)setIntersection) / (setUnion / 2.0);
	double  jaccard = ((double)setIntersection) / setUnion;
	double  dice    = (2.0 * setIntersection) / (b1Card + b2Card);

	long    b1No      = total - b1Card;
	long    b2No      = total - b2Card;
	double  Pr_a      = ((double)(yesYes + noNo)) / total;
	double  randomYes = ((double)b1Card / total) * ((double)b2Card / total);
	double  randomNo  = ((double)b1No   / total) * ((double)b2No   / total);
	double  Pr_e      = randomYes + randomNo;
	double  cohen     = (Pr_a - Pr_e) / (1.0 - Pr_e);

	printf( "\n(%s,%s) intersection=%d, union=%d, jay=%.4f, jaccard=%.4f, dice=%.4f, cohen=%.4f \n",
		f1, f2, (int)setIntersection, (int)setUnion, jay, jaccard, dice, cohen );

    puts( "\n... finished comparing binaries" );
}
//----------------------------------------------------------------------
/** \brief program that determines centroid and scale for AAR (automatic
 *  anatomy recognition.  see usage() for a description of command line
 *  parameters.
 *  \param  argc  number of command line paraneters
 *  \param  argv  command line parameters
 */
int main ( int argc, char* argv[] ) {
    if (verbose) {
        fprintf( stderr, "VIEWNIX_ENV=%s \n", getenv("VIEWNIX_ENV") );
    }
#if 0
    //parse command line args
    if (argc != 4 && argc != 5)  usage( "missing parameters" );
    int  n, start, end;
    n = sscanf( argv[1], "%d", &start );
    if (n != 1)    usage( "bad start" );
    n = sscanf( argv[2], "%d", &end );
    if (n != 1)    usage( "bad end" );
    if (start > end)    usage( "start must be <= end" );

    ChunkData*  seg = new ChunkData( argv[3] );
    if (seg == NULL)    usage( "can't read the segmented-input-file" );

    ChunkData*  gray = NULL;
    if (argc==5) {
        gray = new ChunkData( argv[4] );
        if (gray == NULL)    usage( "can't read the original-gray-input-file" );
    }

    //calculate the centroid, weighted centroid (optional), and bounding box
    getInfo( seg, gray, start, end );
#endif

    assert( argc >= 2 );
    if (argc < 2)    return 0;

    printf( "file=%s \n", argv[1] );
    ChunkData* gray = new ChunkData( argv[1] );
    assert( gray != NULL );

    //try 1 threshold
    double* mean = kMeans ( 2, gray );
    printf( "\n" );
    printf( "for k=2 \n" );
    for (int k=0; k<2; k++) {
        printf( "mean[%d]=%.4f \n", k, mean[k] );
        if (k>0)    printf( "    t%d=%.2f \n", k, (mean[k]+mean[k-1])/2.0 );
    }
    //remember the optimum t for later
    double t = ( mean[0] + mean[1] ) / 2.0;
    free( mean );    mean = NULL;

    //try 2 thresholds
    mean = kMeans ( 3, gray );
    printf( "\n" );
    printf( "for k=3 \n" );
    for (int k=0; k<3; k++) {
        printf( "mean[%d]=%.4f \n", k, mean[k] );
        if (k>0)    printf( "    t%d=%.2f \n", k, (mean[k]+mean[k-1])/2.0 );
    }
    free( mean );    mean = NULL;

    //optionally apply theshold, determine largest connected component in each slice,
	// fill open areas/voids in largest connected component
    if (argc < 3)    return 0;
    process( gray, t, argv[2] );

	if (argc < 4)    return 0;
	compareBinary( argv[2], argv[3] );

    return 0;
}
//----------------------------------------------------------------------
