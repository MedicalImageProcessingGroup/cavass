/*
  Copyright 1993-2009 Medical Image Processing Group
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
/*
file: p3dinterpolateMaster.cpp
*/
//----------------------------------------------------------------------
#include  <assert.h>
#include  <iostream>
#include  <math.h>
#include  <mpi.h>
#if defined (WIN32) || defined (_WIN32)
    #include  <Winsock2.h>
#else
    #include  <sys/times.h>
    #include  <unistd.h>
#endif
#include  "Viewnix.h"
#include  "cv3dv.h"
#include  "ElapsedTime.h"

#include  "p3dinterpolate.h"

using namespace std;
//----------------------------------------------------------------------
//global variables:

extern int   SlicesPerChunk;  //number of slices in a chunk
extern int   myRank;          //process number
extern char  myName[ MPI_MAX_PROCESSOR_NAME ];  //host name

#if 0
  static float   Ox = 0.41;  //desired output spacing
  static float   Oy = 0.41;  //desired output spacing
  static float   Oz = 1.00;  //desired output slice spacing
#else
  static float   Ox = 0.10f;  //desired output spacing
  static float   Oy = 0.10f;  //desired output spacing
  static float   Oz = 0.10f;  //desired output slice spacing
#endif
static float   firstZLoc;  //for input and output
static float   lastZLoc;   //for input and output
static int     outputZSliceCount;  //count of desired output slices
static float*  outputZLocations;   //actual z locations of output slices
static bool*   outputZAssigned;    //assigned to a slave/worker or not yet?
static int     sDegree[3] = { WorkChunk::D_CU, WorkChunk::D_CU,
                              WorkChunk::D_CU };
static int     sDTMethod=0;
//----------------------------------------------------------------------
static void usageAndExit ( const char* const m=NULL ) {
  cout << "\n\
usage (master): \n\
  mpirun C -v -s n0 ./p3dinterpolate.exe input output mode p1 p2 p3 mb m1 m2 m3 spc \n\
where: \n\
  \n\
  input : name of the input file \n\
  output: name of the ouput file \n\
  mode  : 0=foreground execution, 1=background execution \n\
  p1    : pixel size along X1 (Ox) \n\
  p2    : pixel size along X2 (Oy) \n\
  p3    : pixel size along X3 (Oz) \n\
  mb    : method for distance map calculation (any value for grey scenes): \n\
          0=city block, 1=chamfer \n\
  m1    : method along X1 (Ox) : 0=nearest neighbor, 1=linear, 2=cubic \n\
  m2    : method along X2 (Oy) : 0=nearest neighbor, 1=linear, 2=cubic \n\
  m3    : method along X3 (Oz) : 0=nearest neighbor, 1=linear, 2=cubic \n\
  I1,F1	: on first volume interpolate between slices I1 and F1 \n\
  spc   : slices-per-chunk \n\
  \n\
example: mpirun C -v -s n0 ./p3dinterpolate.exe child0.IM0 junk.IM0 0 0.5 0.5 0.5 0 2 2 2 10 \n\
  \n" << endl;
  exit( 0 );
}
//----------------------------------------------------------------------
static void parseCommandLine ( const int argc, const char* const argv[] ) {
#if  0
    char  buff[ 256 ];
    sprintf( buff, "VIEWNIX_ENV=%s", argv[argVIEWNIX_ENV] );
#if defined (WIN32) || defined (_WIN32)
    _putenv( buff );
#else
    putenv( buff );
#endif
#endif
    char*  env = getenv( "VIEWNIX_ENV" );
    printf( "master: VIEWNIX_ENV = %s \n", env );

    if (argc<=argP1 || argc<=argP2 || argc<=argP3)    usageAndExit();
    double  d=0.0;
    //get the desired spacing in each direction
    if (sscanf(argv[argP1], "%lf", &d)!=1)    usageAndExit("bad Ox");
    ::Ox = (float)d;
    if (sscanf(argv[argP2], "%lf", &d)!=1)    usageAndExit("bad Oy");
    ::Oy = (float)d;
    if (sscanf(argv[argP3], "%lf", &d)!=1)    usageAndExit("bad Oz");
    ::Oz = (float)d;
    assert( ::Ox>0 && ::Oy>0 && ::Oz>0 );

    if (argc<=argMB)                         usageAndExit("missing mb");
    int  i=0;
    if (sscanf(argv[argMB], "%d", &i)!=1)    usageAndExit("bad mb");
    switch (i) {
        case 0:     ::sDTMethod = WorkChunk::CITY_BLOCK;    break;
        case 1:     ::sDTMethod = WorkChunk::CHAMFER;       break;
        default:    usageAndExit("bad mb");                 break;
    }

    if (argc<=argM1 || argc<=argM2 || argc<=argM3)    usageAndExit();

    if (sscanf(argv[argM1], "%d", &i)!=1)    usageAndExit("bad m1");
    switch (i) {
        case 0:     ::sDegree[0] = WorkChunk::D_NN;    break;
        case 1:     ::sDegree[0] = WorkChunk::D_LN;    break;
        case 2:     ::sDegree[0] = WorkChunk::D_CU;    break;
        default:    usageAndExit("bad m1");            break;
    }

    if (sscanf(argv[argM2], "%d", &i)!=1)    usageAndExit("bad m2");
    switch (i) {
        case 0:     ::sDegree[1] = WorkChunk::D_NN;    break;
        case 1:     ::sDegree[1] = WorkChunk::D_LN;    break;
        case 2:     ::sDegree[1] = WorkChunk::D_CU;    break;
        default:    usageAndExit("bad m2");            break;
    }

    if (sscanf(argv[argM3], "%d", &i)!=1)    usageAndExit("bad m3");
    switch (i) {
        case 0:     ::sDegree[2] = WorkChunk::D_NN;    break;
        case 1:     ::sDegree[2] = WorkChunk::D_LN;    break;
        case 2:     ::sDegree[2] = WorkChunk::D_CU;    break;
        default:    usageAndExit("bad m3");            break;
    }

    if (argc<=argSPC)    usageAndExit( "missing spc" );
    if (sscanf(argv[argSPC], "%d", &i)!=1)    usageAndExit( "bad spc" );
    ::SlicesPerChunk = i;
    cout << "spc=" << ::SlicesPerChunk << endl;
    assert( ::SlicesPerChunk >= 1 );
#ifdef Verbose
    cout << "desired spacing=(" << ::Ox << "," << ::Oy << "," << ::Oz << "), "
         << "degrees=(" << ::sDegree[0] << "," << ::sDegree[1] << ","
                        << ::sDegree[2] << "), "
         << "spc=" << ::SlicesPerChunk
         << endl;
#endif
}
//----------------------------------------------------------------------
static void initFiles ( FILE* infp, FILE* outfp ) {
#ifdef Verbose
    cout << "master: initFiles()" << endl;
#endif
    assert( infp!=NULL && outfp!=NULL );

    //get ready to read the input data
    int  err = VSeekData( infp, 0 );    //move to the beginning of input data
    assert( err==0 );

    //get ready to write the output data
    err = VSeekData( outfp, 0 );    //move to the beginning of output data
    assert( err==0 );
}
//----------------------------------------------------------------------
/** \brief This function reads the next chunk from the file and assigns
 *  the work to a slave.
 */
static void assignWork ( const int rank, int& workingCount,
    struct WorkChunk* wChunk, const ViewnixHeader& invh, const int size,
    FILE* infp, const unsigned long bytesPerSlice, FILE* outfp )
{
    assert( wChunk->inDataOffset >= 0 );
#ifdef Verbose
    cout << "master " << ::myRank << ": assignWork() to " << rank << ".  "
         << "seeking to " << (wChunk->inFirstZSlice * bytesPerSlice)
         << ".  " << flush;
#endif
    int  err = VSeekData( infp,
                   ((off_t)wChunk->inFirstZSlice) * bytesPerSlice );
    if (err!=0) {
        cerr << "Can't seek to " << (wChunk->inFirstZSlice * bytesPerSlice)
             << " in the input file." << endl;
        exit( -1 );
    }

    //now we need to load the next SlicesPerChunk number of slices into
    // the slice data.  we also need to handle the case where there are 
    // less than SlicesPerChunk number of slices remaining to be read
    // from the file.
    assert( wChunk->inFirstZSlice < invh.scn.num_of_subscenes[0] );
#ifdef Verbose
    cout << " SlicesPerChunk="   << SlicesPerChunk
         << " num_of_subscenes=" << invh.scn.num_of_subscenes[0]
         << " inFirstZSlice="    << wChunk->inFirstZSlice << " " << flush;
#endif
    if (SlicesPerChunk < (invh.scn.num_of_subscenes[0]-wChunk->inFirstZSlice))
    {
        //read SlicesPerChunk number of slices
#ifdef Verbose
        cout << "master: reading " << SlicesPerChunk << " slice(s).  "
             << flush;
#endif
        int  num, err;
        ElapsedTime  et;
        assert( wChunk->inDataOffset>=0 );
        err = VReadData( &wChunk->inData[ wChunk->inDataOffset ],
                  size, SlicesPerChunk*bytesPerSlice/size, infp, &num );
        if (err) {
            cerr << "Can't read the input file (1). err=" << err
                 << " size="   << size
                 << " count="  << (SlicesPerChunk*bytesPerSlice/size)
                 << " offset=" << wChunk->inDataOffset
                 << " SlicesPerChunk="    << SlicesPerChunk
                 << " num_of_subscenes="  << invh.scn.num_of_subscenes[0]
                 << " inFirstZSlice="     << wChunk->inFirstZSlice
                 << endl;
            assert( 0 );
            exit( -1 );
        }
        assert( SlicesPerChunk*bytesPerSlice/size == num );
        //copy the loc_of_subscenes for this chunk
        WorkChunk::locType*  lptr = (WorkChunk::locType*) wChunk->inData
            + wChunk->inZLocationOffset;
        for (int i=0; i<SlicesPerChunk; i++) {
            lptr[i] = invh.scn.loc_of_subscenes[ wChunk->inFirstZSlice + i ];
        }
    } else {
        //read the slices that remain
        const int  remainingSlices = invh.scn.num_of_subscenes[0]
                                      - wChunk->inFirstZSlice;
        const long  remainingBytes = remainingSlices * bytesPerSlice;
#ifdef Verbose
        cout << "master: reading remaining " << remainingSlices
             << " slices.  " << flush;
#endif
        int  num, err;
        ElapsedTime  et;
        err = VReadData( &wChunk->inData[wChunk->inDataOffset], size,
                         remainingBytes/size, infp, &num );
        if (err) {
            cerr << "Can't read the input file (2)." << endl;
            exit( -1 );
        }
        assert( remainingBytes/size == num );
        //copy the loc_of_subscenes for this chunk
        WorkChunk::locType*  lptr = (WorkChunk::locType*) (wChunk->inData
            + wChunk->inZLocationOffset);
        for (int i=0; i<remainingSlices; i++) {
            lptr[i] = invh.scn.loc_of_subscenes[ wChunk->inFirstZSlice + i ];
        }

        //zero out the rest
        memset( &wChunk->inData[wChunk->inDataOffset+remainingBytes],
                0, (SlicesPerChunk-remainingSlices)*bytesPerSlice );

//todo (i think this is ok.  actually, it wasn't.)
        //wChunk->inCount[2] = wChunk->outCount[2] = remainingSlices;
        wChunk->inCount[2] = remainingSlices;
    }

    //send the next item of work to be performed
#ifdef Verbose
    cout << "master: sending " << wChunk->totalBytes << endl;
#endif
    ElapsedTime  et;
    MPI_Send( wChunk, wChunk->totalBytes, MPI_UNSIGNED_CHAR, rank,
              wChunk->operation, MPI_COMM_WORLD );

    ++workingCount;
}
//----------------------------------------------------------------------
static void saveWork ( FILE* outfp, struct ResultChunk* chunk ) {
#ifdef Verbose
    cout << "master " << ::myRank << ": saveWork(): ";
    chunk->print();
#endif
    const int  size = ::dataTypeSizes[ chunk->dataType ];
    const unsigned long  bytesPerSlice = chunk->count[0] * chunk->count[1]
                                         * size;
#ifdef Verbose
    cout << "master " << ::myRank << ": saveWork(): " << "seeking to "
         << (chunk->firstZSlice * bytesPerSlice) << " firstZSlice="
         << chunk->firstZSlice << endl;
#endif
    int  err = VSeekData( outfp, ((off_t)chunk->firstZSlice) * bytesPerSlice );
    if (err) {
        cerr << "Can't seek the output file." << endl;
        exit( -1 );
    }
#ifdef Verbose
    cout << "master " << ::myRank << ": saveWork(): " << "writing "
         << (chunk->count[2]*bytesPerSlice) << endl;
#endif
    int  num;
    err = VWriteData( chunk->data, size, chunk->count[2]*bytesPerSlice/size,
                      outfp, &num );
    if (err) {
        cerr << "Can't write the output file." << endl;
        exit( -1 );
    }
    assert( chunk->count[2]*bytesPerSlice/size == num );
#ifdef Verbose
    cout << "master " << ::myRank << ": saveWork(): done." << endl;
#endif
}
//----------------------------------------------------------------------
/** \brief This function determines the # and locations of the output
 *  z slices and indicates that they haven't been assigned to worker/slave
 *  processes yet.
 */
static void determineOutputSliceLocations ( const ViewnixHeader& vh ) {
/*
    cout << "input slice locations:" << endl;
    for (int z=0; z<vh.scn.num_of_subscenes[0]; z++) {
        cout << " " << z << ":" << vh.scn.loc_of_subscenes[z];
    }
    cout << endl;
*/
    //determine the number of evenly spaced output slices (note that the input
    // may not be evenly spaced).
    ::firstZLoc = vh.scn.loc_of_subscenes[0];
    ::lastZLoc  = vh.scn.loc_of_subscenes[ vh.scn.num_of_subscenes[0]-1 ];
    ::outputZSliceCount = 0;
    float  where = ::firstZLoc;
    while (where <= ::lastZLoc) {
        ++::outputZSliceCount;
        where += Oz;
    }
    // cout << "count=" << ::outputZSliceCount << endl;
    ::outputZLocations = (float*)malloc( ::outputZSliceCount * sizeof(float) );
    assert( ::outputZLocations != NULL );

    ::outputZAssigned = (bool*)malloc( ::outputZSliceCount * sizeof(bool) );
    assert( ::outputZAssigned != NULL );

    where = ::firstZLoc;
    for (int i=0; i< ::outputZSliceCount; i++) {
        ::outputZLocations[i] = where;
        ::outputZAssigned[i]  = false;
        // cout << " " << i << ":" << ::outputZLocations[i];
        where += Oz;
    }
    // cout << endl;
}
//----------------------------------------------------------------------
/** \brief this function sets wChunk->outFirstZSlice, wChunk->outFirstZLoc,
 *  wChunk->inFirstZSlice, and wChunk->outLastZLoc.
 */
static int setZInfo ( struct WorkChunk* wChunk, const ViewnixHeader& invh )
{
    //find the first unassigned output z slice
    int  i=0;
    for (i=0; i< ::outputZSliceCount; i++) {
        if (!::outputZAssigned[i])    break;
    }
    wChunk->outFirstZSlice = i;
    wChunk->outFirstZLoc   = ::outputZLocations[i];

    //special case for i=0
    if (i==0) {
        wChunk->inFirstZSlice = 0;
    } else {
        //find the first input slice location beyond the one we need
        int  j=0;
        for (j=0; j<invh.scn.num_of_subscenes[0]; j++) {
            if ( invh.scn.loc_of_subscenes[j] >= ::outputZLocations[i] )
                break;
        }
        if (invh.scn.loc_of_subscenes[j] > ::outputZLocations[i])    --j;
        if (wChunk->degree[2] == WorkChunk::D_CU)    --j;
        wChunk->inFirstZSlice = j;
        assert( wChunk->inFirstZSlice>=0 );
    }
    //determine the z location of the last slice in this chunk
    i = wChunk->inFirstZSlice + ::SlicesPerChunk - 1;
    double  lastInputZLoc = invh.scn.loc_of_subscenes[i];
    if (i>=invh.scn.num_of_subscenes[0]) {
        i = invh.scn.num_of_subscenes[0]-1;
        lastInputZLoc = invh.scn.loc_of_subscenes[i];
    } else if (wChunk->degree[2] == WorkChunk::D_CU) {
        lastInputZLoc = invh.scn.loc_of_subscenes[i-1];
    }
    //determine the last output z location that we can interpolate within
    // this chunk.
    int  j;
    for (j=wChunk->outFirstZSlice+1; j< ::outputZSliceCount; j++) {
        if (::outputZLocations[j]>=lastInputZLoc)    break;
    }
    if (::outputZLocations[j]>lastInputZLoc) {
        --j;
        assert( j>=0 );
    }
    if (j >= ::outputZSliceCount )    j = ::outputZSliceCount-1;
    wChunk->outLastZLoc = ::outputZLocations[j];
    //mark these output slices as assigned
    int  count=0;
    for (i=0; i< ::outputZSliceCount; i++) {
        if ( wChunk->outFirstZLoc <= ::outputZLocations[i] &&
             ::outputZLocations[i] <= wChunk->outLastZLoc ) {
            ::outputZAssigned[i] = true;
            ++count;
        }
    }
    return count;
}
//----------------------------------------------------------------------
static int countOutputSlices ( const struct WorkChunk* const wc ) {
    //count the number of output slices between the first and last inclusive
    int  count = 0;
cout << "here: first=" << wc->outFirstZLoc << " last=" << wc->outLastZLoc << endl;
    for (int i=0; i< ::outputZSliceCount; i++) {
        if ( wc->outFirstZLoc <= ::outputZLocations[i]
          && ::outputZLocations[i] <= wc->outLastZLoc )    ++count;
    }
cout << "    count=" << count << endl;
    return count;
}
//----------------------------------------------------------------------
static void headerInfo ( const ViewnixHeader& invh,
                         const ViewnixHeader& outvh )
{
    cout << "input  (x,y,z)=(" << invh.scn.xysize[0] << ","
         << invh.scn.xysize[1] << "," << invh.scn.num_of_subscenes[0] << ")"
         << " (" << invh.scn.xypixsz[0] << "," << invh.scn.xypixsz[1]
         << "," << (invh.scn.loc_of_subscenes[1]-invh.scn.loc_of_subscenes[0])
         << ")" << endl;
    cout << "output (x,y,z)=(" << outvh.scn.xysize[0] << ","
         << outvh.scn.xysize[1] << "," << outvh.scn.num_of_subscenes[0] << ")"
         << " (" << outvh.scn.xypixsz[0] << "," << outvh.scn.xypixsz[1]
         << "," << (outvh.scn.loc_of_subscenes[1]-outvh.scn.loc_of_subscenes[0])
         << ")" << endl;
}
//----------------------------------------------------------------------
void master ( int argc, char* argv[] ) {

#if 0  //only when debugging with: gdb program pid
    int  kk=1;
    while (kk==1) {
        cout << "master=" << ::myRank << " pid=" << getpid() << endl;
        sleep( 5 );
    }
#endif

    cout << "master " << ::myRank << ": " << ::myName << ": hello" << endl;
    parseCommandLine( argc, argv );
    //open the input file and read its header
    assert( argc>argIN );
#ifdef Verbose
    cout << "master: open " << argv[argIN] << endl;
#endif
    FILE*  infp = fopen( argv[argIN], "rb" );    assert(infp!=NULL);
    ViewnixHeader  invh;
    char  group[5], element[5];
    int  err = VReadHeader( infp, &invh, group, element );
    if (err && err < 106) {
        cerr << "Can't read the input file's header: " << argv[argIN] << "." << endl;
        exit( -1 );
    }
    assert( invh.scn.dimension==3 );

//invh.scn.num_of_subscenes[0] = 5;

    determineOutputSliceLocations( invh );

#ifdef Verbose
    cout << "master " << ::myRank << ": number of input slices = "
         << invh.scn.num_of_subscenes[0] << endl;
#endif

    //format & write output header
    ViewnixHeader  outvh = invh;
    assert( argc>argIN );
#ifdef Verbose
    cout << "master: open " << argv[argOUT] << endl;
#endif
    FILE*  outfp = fopen( argv[argOUT], "wb+" );    assert(outfp!=NULL);
    if (err && err<106) {
        cerr << "Can't write the output file, " << argv[argOUT] << "." << endl;
        exit( -1 );
    }

#if 0
//todo: loc_of_subscenes will mostly likely change between input & output
// for now we'll just copy loc_of_subscenes from input to output.
    assert( outvh.scn.num_of_subscenes[0] <= invh.scn.num_of_subscenes[0] );
    outvh.scn.loc_of_subscenes = (float*)malloc( invh.scn.num_of_subscenes[0]
                                                   * sizeof( float ) );
    assert( outvh.scn.loc_of_subscenes != NULL );
    assert( sizeof( *outvh.scn.loc_of_subscenes ) == sizeof( float ) );
    for (int i=0; i<outvh.scn.num_of_subscenes[0]; i++) {
        outvh.scn.loc_of_subscenes[i] = invh.scn.loc_of_subscenes[i];
    }
//todo: should this be 2*w+1 or just 2*w?
    outvh.scn.xysize[0]  = 2 * invh.scn.xysize[0];
    outvh.scn.xysize[1]  = 2 * invh.scn.xysize[1];
    outvh.scn.xypixsz[0] = invh.scn.xypixsz[0] / 2.0;
    outvh.scn.xypixsz[1] = invh.scn.xypixsz[1] / 2.0;

#else
    assert( sizeof(outvh.scn.num_of_subscenes[0]) == sizeof(short) );
    outvh.scn.num_of_subscenes = (short*)malloc( sizeof(short) );
    assert( outvh.scn.num_of_subscenes!=NULL );
    outvh.scn.num_of_subscenes[0] = ::outputZSliceCount;
    outvh.scn.loc_of_subscenes    = ::outputZLocations;
    outvh.scn.xypixsz[0] = Ox;
    outvh.scn.xypixsz[1] = Oy;
    outvh.scn.xysize[0]  = (short)( invh.scn.xysize[0]
                         * (invh.scn.xypixsz[0] / outvh.scn.xypixsz[0]) );
    outvh.scn.xysize[1]  = (short)( invh.scn.xysize[1]
                         * (invh.scn.xypixsz[1] / outvh.scn.xypixsz[1]) );
#endif

#ifdef Verbose
    headerInfo( invh, outvh );
#endif
    err = VWriteHeader( outfp, &outvh, group, element );
    if (err && err < 106) {
        cerr << "Can't write the output file, " << argv[argOUT] << "." << endl;
        exit( -1 );
    }

    // allocate a work chunk
    unsigned long  bytesPerSlice = 0;
    int            size = 0;
    switch (invh.scn.num_of_bits) {
        case  1:
            assert( invh.scn.xysize[0]%8 == 0 );
            size = 1;
            bytesPerSlice = size * (invh.scn.xysize[0]/8) * invh.scn.xysize[1];
            break;
        case  8:
            size = 1;
            bytesPerSlice = size *  invh.scn.xysize[0]    * invh.scn.xysize[1];
            break;
        case 16:
            size = 2;
            bytesPerSlice = size *  invh.scn.xysize[0]    * invh.scn.xysize[1];
            break;
        default:
            assert(0);
            break;
    }

    const unsigned long  maxData = bytesPerSlice * ::SlicesPerChunk;
    const unsigned long  locSize = ::SlicesPerChunk
                                   * sizeof(WorkChunk::locType);
    const unsigned long  totalBytes = sizeof(struct WorkChunk) + maxData
                                      + locSize;
    struct WorkChunk*    wChunk = (struct WorkChunk*)malloc( totalBytes );
    assert( wChunk!=NULL );

    //init the work chunk
    memset( wChunk, 0, totalBytes );
    wChunk->totalBytes     = totalBytes;
    wChunk->operation      = WorkChunk::OP_INTERPOLATE;
    wChunk->degree[0]      = ::sDegree[0];    //WorkChunk::D_CU;
    wChunk->degree[1]      = ::sDegree[1];    //WorkChunk::D_CU;
    wChunk->degree[2]      = ::sDegree[2];    //WorkChunk::D_CU;
    wChunk->chamferFlag    = WorkChunk::UNUSED;
    wChunk->inCount[0]     = invh.scn.xysize[0];
    wChunk->inCount[1]     = invh.scn.xysize[1];
    wChunk->inCount[2]     = ::SlicesPerChunk;
    wChunk->inSize[0]      = invh.scn.xypixsz[0];
    wChunk->inSize[1]      = invh.scn.xypixsz[1];
    wChunk->inSize[2]      = invh.scn.loc_of_subscenes[1]
                               - invh.scn.loc_of_subscenes[0];
    wChunk->inFirstZSlice  = 0;  //assigned in setZInfo
//todo: should this be 2*w+1 or just 2*w?
    wChunk->outCount[0]    = outvh.scn.xysize[0];  //2 * invh.scn.xysize[0];
    wChunk->outCount[1]    = outvh.scn.xysize[1];  //2 * invh.scn.xysize[1];
    wChunk->outCount[2]    = 0;  // assigned in setZInfo; was ::SlicesPerChunk;
    wChunk->outSize[0]     = Ox;  //invh.scn.xypixsz[0] / 2.0;
    wChunk->outSize[1]     = Oy;  //invh.scn.xypixsz[1] / 2.0;
    wChunk->outSize[2]     = Oz;  //invh.scn.loc_of_subscenes[1] - invh.scn.loc_of_subscenes[0];
    //determine the out data type
    switch (invh.scn.num_of_bits) {
        case  1 :  wChunk->outDataType = WorkChunk::INT1;    break;
        case  8 :  wChunk->outDataType = WorkChunk::UINT8;   break;
        case 16 :  wChunk->outDataType = WorkChunk::UINT16;  break;
        default :  cerr << "invh.scn.num_of_bits=" << invh.scn.num_of_bits
                        << endl;
                   assert( 0 );
                   break;
    }
    wChunk->outFirstZSlice    = 0;
    wChunk->outFirstZLoc      = ::firstZLoc;
    //done below wChunk->outLastZLoc       = setZInfo( wChunk, invh );

    wChunk->inZLocationOffset = 0;         //-1 if unused
    wChunk->inDataOffset      = locSize;   //0 if unused
    //determine the input data type
    switch (invh.scn.num_of_bits) {
        case  1 :  wChunk->inDataType = WorkChunk::INT1;    break;
        case  8 :  wChunk->inDataType = WorkChunk::UINT8;   break;
        case 16 :  wChunk->inDataType = WorkChunk::UINT16;  break;
        default :  assert( 0 );                             break;
    }

    initFiles( infp, outfp );

    //determine number of processes
    int  nTasks;
    MPI_Comm_size( MPI_COMM_WORLD, &nTasks );
#ifdef Verbose
    cout << "master " << ::myRank << ": mpi comm size = " << nTasks << endl;
#endif
    //send one unit of work to each slave
    int  workingCount = 0;
    for (int rank=1; rank<nTasks; rank++) {
        wChunk->outCount[2] = setZInfo( wChunk, invh );
        //wChunk->outCount[2] = countOutputSlices( wChunk );
        assignWork( rank, workingCount, wChunk, invh, size, infp,
                    bytesPerSlice, outfp );
        if (::outputZAssigned[::outputZSliceCount-1])    break;
    }
#ifdef Verbose
    cout << "master " << ::myRank << ": initial assignment complete." << endl;
#endif
    outvh.scn.smallest_density_value[0] = LONG_MAX;
    outvh.scn.largest_density_value[0]  = LONG_MIN;
    struct ResultChunk*  resultChunk = NULL;
    long  resultChunkMaxBytes = 0;
    while (workingCount>0) {
        //get results
        //cout << "master " << ::myRank << ": attempting to probe" << endl;
        MPI_Status  status;
        MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
        //re/allocate the result chunk (if necessary)
//        if (status.st_length>resultChunkMaxBytes) {
        int  st_length = 0;
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
        if (st_length > resultChunkMaxBytes) {
            if (resultChunk!=NULL) {
                free( resultChunk );
                resultChunk = NULL;
                resultChunkMaxBytes = 0;
            }
            resultChunkMaxBytes = st_length;
            resultChunk = (struct ResultChunk*) malloc( resultChunkMaxBytes );
            assert( resultChunk!=NULL );
        }
        //assert( status.st_length<=totalBytes );
#ifdef Verbose
        cout << "master " << ::myRank << ": attempting to receive from "
             << status.MPI_SOURCE << endl;
#endif
        ElapsedTime  et;
        MPI_Recv( resultChunk, resultChunkMaxBytes, MPI_UNSIGNED_CHAR,
                  MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
        const int  rank = status.MPI_SOURCE;
#ifdef Verbose
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
        cout << "master " << ::myRank << ": received from " << rank
             << " " << st_length << " bytes" << endl;
#endif
        if (resultChunk->min < outvh.scn.smallest_density_value[0])
            outvh.scn.smallest_density_value[0] = resultChunk->min;
        if (resultChunk->max > outvh.scn.largest_density_value[0])
            outvh.scn.largest_density_value[0] = resultChunk->max;
        saveWork( outfp, resultChunk );
        --workingCount;

        //any more work to assign?
        if (::outputZAssigned[::outputZSliceCount-1])    continue;

        //there is more work to assign
        wChunk->outCount[2] = setZInfo( wChunk, invh );
        assignWork( rank, workingCount, wChunk, invh, size, infp,
                    bytesPerSlice, outfp );
    }  //end while

    //update the header because the min,max values may have changed
    cout << "overall min = " << outvh.scn.smallest_density_value[0] << endl;
    cout << "overall max = " << outvh.scn.largest_density_value[0]  << endl;
    fseek( outfp, 0, SEEK_SET );
    err = VWriteHeader( outfp, &outvh, group, element );
    if (err && err < 106) {
        cerr << "Can't write the output file, " << argv[argOUT] << "." << endl;
        exit( -1 );
    }

    //send 'time-to-quit' messages
    wChunk->operation = WorkChunk::OP_EXIT;
    for (int rank=1; rank<nTasks; rank++) {
#ifdef Verbose
        cout << "master " << ::myRank << ": sending time-to-quit message to "
             << rank << endl;
#endif
        MPI_Send( wChunk, sizeof(struct WorkChunk), MPI_UNSIGNED_CHAR, rank,
                  wChunk->operation, MPI_COMM_WORLD );
    }

    free( outvh.scn.loc_of_subscenes );
    outvh.scn.loc_of_subscenes=NULL;
    free( wChunk );
    wChunk = NULL;

    if (resultChunk!=NULL) {
        free( resultChunk );
        resultChunk = NULL;
    }

    fclose( outfp );    outfp = NULL;
    fclose( infp );     infp  = NULL;
#ifdef Verbose
    cout << "master " << ::myRank << " bye." << endl;
#endif
}
//----------------------------------------------------------------------

