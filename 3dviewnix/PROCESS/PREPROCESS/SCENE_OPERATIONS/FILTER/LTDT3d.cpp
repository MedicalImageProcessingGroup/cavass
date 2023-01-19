/*
  Copyright 1993-2012, 2015, 2017, 2019 Medical Image Processing Group
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

 
 
 


/*
    This program performs LTDT (linear time distance transform) on an nD Scene (3DViewnix format).


Author: Xinjian Chen
Date  : 03/05/2009

*/


#include <math.h>
#include <float.h>

#include <Viewnix.h>
 
#include  "cv3dv.h"

#include "slices.c"
#include "fff.c"

extern "C" 
{
 int VGetHeaderLength ( FILE* fp, int* hdrlen );
 int VAddBackgroundProcessInformation ( char* command );
}

#define MAXDIM    3
#define MAXDIMLENGTH  10000
#define MAXDIST  FLT_MAX/2

int nDim;
float xypixsz[2], *location3;

void GetIntCoordinates( int index, int nCoord[], int *pnDimEach );
int GetCoordIndex( int nCoord[], int *pnDimEach );
void GetRealCoordinates( double rCoord[], int nCoord[] );

int GetCoordIndex( int nCoord[], int *pnDimEach )
{
    int index = 0;
    int nMul = 1;
    int i;
    for( i=0; i<nDim; i++ )
    {
        index += nCoord[i] * nMul;
        nMul *= pnDimEach[i];
    }

    return index;
}


void GetIntCoordinates( int index, int nCoord[], int *pnDimEach )
{
    int dim;
    int posi = index;
    for( dim = 0; dim < nDim; dim++ )
    {
        nCoord[dim] = posi % pnDimEach[dim];
        posi /= pnDimEach[dim];
    }

}

void GetRealCoordinates( double rCoord[], int nCoord[] )
{
    rCoord[0] = xypixsz[0]*nCoord[0];
    rCoord[1] = xypixsz[1]*nCoord[1];
    rCoord[2] = location3[nCoord[2]];
}


bool ITKCheck( float d1, float d2, float d3, float ud, float vd, float wd )
{
    float a = vd - ud; 
    float b = wd - vd;
    float c = wd - ud;

    return ( c*d2 - b*d1 - a*d3 - a*b*c > 0 );
}


int FTDTDimUp( int c, int d, int F[], float DT[], int *pnDimEach, int nVoxelsNum )
{
    float q[MAXDIMLENGTH], ud, vd, wd;
    int   indexD[MAXDIMLENGTH];
    int   g[MAXDIMLENGTH];

    int m = -1;
    int l = 0;
    int nCoord[MAXDIM]; // suppose max dimension 
    double rCoordid, rCoordld, rCoordl1d;
    int i;
    int xi;

    GetIntCoordinates( c, nCoord, pnDimEach );

    for( i=0; i< pnDimEach[d]; i++ )
    {
        nCoord[d] = i;
        xi = GetCoordIndex( nCoord, pnDimEach );

        if( DT[xi] < MAXDIST )  // -1 is empty (NULL) set
        {
            if( m < 1 )
            {
                m++;
                q[m] = DT[xi];    indexD[m] = i;    g[m] = F[xi];  //g[m]  saved the feature transform
            }
            else
            {
                while ( m >= 1 && 
                    (   d<2? (ud=xypixsz[d]*indexD[m-1],
                          vd=xypixsz[d]*indexD[m], wd=xypixsz[d]*i):
                        (ud=location3[indexD[m-1]], vd=location3[indexD[m]],
                          wd=location3[i]),
                      ITKCheck( q[m-1], q[m], DT[xi], ud, vd, wd )
                    ) )   // start from 0, so +1
                {
                    m--;
                }

                m++;
                q[m] = DT[xi];  indexD[m] = i;    g[m] = F[xi];

            }

        }
    }

    if( m >= 0 )
    {
        l = 0;

        for( i=0; i<pnDimEach[d]; i++ )
        {
            nCoord[d] = i;
            xi = GetCoordIndex( nCoord, pnDimEach );

            while( l < m &&
                  ( rCoordld=d<2?xypixsz[d]*indexD[l]:location3[indexD[l]],
                    rCoordid=d<2?xypixsz[d]*i:location3[i],
                    rCoordl1d=d<2?xypixsz[d]*indexD[l+1]:
                    location3[indexD[l+1]],
                    q[l] + (rCoordld-rCoordid)*(rCoordld-rCoordid) >
                    q[l+1] + (rCoordl1d-rCoordid)*(rCoordl1d-rCoordid)
                  )
                 )
                l++;

            rCoordld=d<2?xypixsz[d]*indexD[l]:location3[indexD[l]];
            rCoordid=d<2?xypixsz[d]*i:location3[i];
            DT[xi] = (float)(q[l] + (rCoordld-rCoordid)*(rCoordld-rCoordid));
            F[xi]  = g[l];

        }
    }

    return 1;
}

int LTFTDT(unsigned char pInImg[],  unsigned char pBinaryImg[], int F[], float DT[], int *pnDimEach, int nVoxelsNum )
{
    int i, d;
    int x;
    int nCoordi[MAXDIM]; // suppose max dimension

    for( i= 0; i< nVoxelsNum; i++ )  //// Fg --> Bg distance
    {
        if( pBinaryImg[i] > 0 )   
        {
            DT[i] = FLT_MAX;
            F[i]  = -1;
        }
        else
        {
            DT[i] = 0; // -1 represent empty set
            F[i]  = i;
        }
    }

    ///////// Design for 3D Feature Transform //////////
    int DimTable1[3];
    int DimTable2[3];
    DimTable1[0] = pnDimEach[1];    DimTable2[0] = pnDimEach[2];  // x == 0
    DimTable1[1] = pnDimEach[0];    DimTable2[1] = pnDimEach[2];  // y == 0
    DimTable1[2] = pnDimEach[0];    DimTable2[2] = pnDimEach[1];  // z == 0

    int index1[3] = {1, 0, 0};
    int index2[3] = {2, 2, 1};

    for( d=0; d<nDim; d++ )
    {
        nCoordi[d] = 0;
        for( int i = 0; i< DimTable1[d]; i++)
        {
            nCoordi[ index1[d] ] = i;
            for( int j = 0; j< DimTable2[d]; j++)
            {
                nCoordi[ index2[d] ] = j;

                x = GetCoordIndex( nCoordi, pnDimEach );

                FTDTDimUp( x, d, F, DT, pnDimEach,    nVoxelsNum);
            }
        }

    }

    for( i= 0; i< nVoxelsNum; i++ )  //// Fg --> Bg distance
    {
        DT[i] = (float)sqrt( double(DT[i]) );
    }

    return 1;

}


int LTSDT(  unsigned char pBinaryImg[], int pF[], float pDT[], int *pnDimEach, int nVoxelsNum, int distType )
{
    int i, j, k;
    int x, y;
    int nCoordx[MAXDIM]; // suppose max dimension 
    int nCoordy[MAXDIM]; // suppose max dimension
    unsigned char * pBoundaryImg;


    time_t ltime;
    time( &ltime );

    pBoundaryImg = (unsigned char *)malloc( nVoxelsNum * sizeof( unsigned char) );
    if( pBoundaryImg == NULL )
        return 0;

    memset(pBoundaryImg, 1, nVoxelsNum  * sizeof( unsigned char) );

    int boundaryValue = 0;
    if( distType == 0 )  // background to goreground
        boundaryValue = 255;
    else
        boundaryValue = 0;

    for( x= 0; x< nVoxelsNum; x++ )
    {
        if( pBinaryImg[x] == boundaryValue )
        {
            GetIntCoordinates( x, nCoordx, pnDimEach );

            for( i= nCoordx[0] -1; i<= nCoordx[0] +1; i++ )   // here, only support 3D, 18 neighbour adjacency
            {
                if( i< 0 || i> pnDimEach[0] - 1 )
                    continue;

                nCoordy[0] = i;

                for( j= nCoordx[1] -1; j<= nCoordx[1] +1; j++ )
                {
                    if( j< 0 || j> pnDimEach[1] - 1 )
                        continue;

                    nCoordy[1] = j;

                    for( k= nCoordx[2] -1; k<= nCoordx[2] +1; k++ )
                    {
                        if( k< 0 || k> pnDimEach[2] - 1 )
                            continue;

                        nCoordy[2] = k;

                        if( nCoordy[0] != nCoordx[0] && nCoordy[1] != nCoordx[1] && nCoordy[2] != nCoordx[2] )  // get rid of 8 corners
                            continue;

                        y = GetCoordIndex( nCoordy, pnDimEach );

                        if( pBinaryImg[y] == 255-boundaryValue )
                        {
                            pBoundaryImg[x] = 0;
                            break;
                        }
                    }

                    if( pBoundaryImg[x] == 0 )
                        break;
                }

                if( pBoundaryImg[x] == 0 )
                        break;
            }
        }

    }

    if( pDT == NULL )
    {
        pDT = (float *)malloc( (nVoxelsNum) * sizeof( float) );  // nor enough memory for super dataset
        if( pDT == NULL )
            return 0;
    }

    LTFTDT(  NULL,  pBoundaryImg, pF, pDT, pnDimEach,    nVoxelsNum );  // 3. Based on 2, including Feature transform

    for( x= 0; x< nVoxelsNum; x++ )
    {
        if( distType == 0 ) //Distance Type, 0: background --> foreground; 1: foreground --> background; 2: both;\n");
        {
            if( pBinaryImg[x] != 0 )
                pDT[x] = 0; 
        }
        else if( distType == 1 ) 
        {
            if( pBinaryImg[x] == 0 )
                pDT[x] = 0;
        }
        else
        {
            if( pBinaryImg[x] == 0 )  // background to foreground, positive
                pDT[x] = pDT[x];
            else
                pDT[x] = -pDT[x];     // foreground to background, negative
        }

    }

    return 1;

}

int GBDT(  unsigned char pBinaryImg[], float pDT[], int *pnDimEach, int nVoxelsNum )
{
    int i, d;
    int x, y, c;
    int nVoxelsNumX2 = nVoxelsNum; 
    int nCoordx[MAXDIM]; // suppose max dimension 
    int nCoordy[MAXDIM]; // suppose max dimension 
    int nCoordc[MAXDIM]; // suppose max dimension 
    int pnDimEachX2[MAXDIM];

    time_t ltime;
    time( &ltime );

    unsigned char* pBinaryImgX2 = NULL;
    for( int i=0; i<nDim; i++ )
    {
        nVoxelsNumX2   *= 2;
        pnDimEachX2[i] = 2*pnDimEach[i];
    }

    pBinaryImgX2 = (unsigned char *)malloc( nVoxelsNumX2 * sizeof( unsigned char) );
    if( pBinaryImgX2 == NULL )
        return 0;

    memset(pBinaryImgX2, 1, nVoxelsNumX2  * sizeof( unsigned char) );

    for( x= 0; x< nVoxelsNum; x++ )
    {
        for( d=0; d<nDim; d++ )
        {
            GetIntCoordinates( x, nCoordx, pnDimEach );

            for( i = 0; i< nDim; i++ )
            {
                if( i != d )
                    nCoordy[i] = nCoordx[i];
                else
                {
                    if( nCoordx[i] < pnDimEach[i] - 1 )  // boundary point, need check ???
                        nCoordy[i] = nCoordx[i] +1;
                    else
                        nCoordy[i] = nCoordx[i];

                }
            }

            y = GetCoordIndex( nCoordy, pnDimEach );

            if( pBinaryImg[x] != pBinaryImg[y] )
            {
                for( i = 0; i< nDim; i++ )
                {
                    if( i != d )
                        nCoordc[i] = 2*nCoordx[i];
                    else
                        nCoordc[i] = 2*nCoordx[i] +1;
                }

                c = GetCoordIndex( nCoordc, pnDimEachX2 );

                pBinaryImgX2[c] = 0;

            }
        }
    }

    time_t ltime2;
    time( &ltime2 );

    int secs = (int)(ltime2-ltime);

    int *pFX2  = NULL;  // Feature Transform Image,   Here is the closest(nearest)  point coordinates
    float *pDTX2  = NULL;  // Feature Transform Image

    pFX2 = (int *)malloc( nVoxelsNumX2 * sizeof( int) );
    if( pFX2 == NULL )
        return 0;

    pDTX2 = (float *)malloc( nVoxelsNumX2 * sizeof( float) );
    if( pDTX2 == NULL )
        return 0;

    LTFTDT(  NULL, pBinaryImgX2, pFX2, pDTX2, pnDimEachX2, nVoxelsNumX2 );

    for( x= 0; x< nVoxelsNum; x++ )
    {
        GetIntCoordinates( x, nCoordx, pnDimEach );
        for( i=0; i<nDim; i++)
            nCoordx[i] *= 2;

        y = GetCoordIndex( nCoordx, pnDimEachX2 );


        if( pBinaryImg[x] == 0 )
            pDT[x] = pDTX2[y]/2;
        else
            pDT[x] = -pDTX2[y]/2;

    }


    if( pBinaryImgX2 != NULL )
        free(pBinaryImgX2);
    if( pFX2 != NULL )
        free(pFX2);
    if( pDTX2 != NULL )
        free(pDTX2);

    time_t ltime3;
    time( &ltime3 );

    secs = (int)(ltime3-ltime2);

    return 1;

}

/*-------------------------------------------------------------------------*/
/* Read the file header and its length */
int get_file_info(char *file /* filename */, ViewnixHeader *vh, int *len /* length of header */)
{
    char group[5], element[5];
    FILE *fp;
    int error;
 
    /* open file */
    if ((fp=fopen(file,"rb"))==NULL)
    {
        fprintf(stderr,"ERROR: Can't open file [%s] !\n", file);
        VDeleteBackgroundProcessInformation();
        exit(1);
    }
 
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        VDeleteBackgroundProcessInformation();
        exit(1);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        VDeleteBackgroundProcessInformation();
        exit(1);
    }
 
    fclose(fp);
 
    return(0);
 
}
 


int main(int argc,char* argv[])
{
    FILE *fpin, *fpout; /* inpput/output files */
    int execution_mode; /* execution mode */
    ViewnixHeader vh;   /* 3DViewnix header */
    SLICES    sl;       /* Structure containing information about the slices of the scene */
    char group[5],      /* Used in VWriteHeader */
        element[5];
    int length;         /* length of a slice */
    int hlength;        /* length of the input header */
    int width, height;  /* dimensions of a slice */
    int distType;       /* Distance Type, 0: background --> foreground; 1: foreground --> background; 2: both */
    int i,j,k;          /* general use */
    int error;          /* error code */
    char *comments;     /* used to modify the header (description field) */
    int binary_flag=0;  /* indicates if scene is binary or not */

    unsigned char *in_buffer1b;
    unsigned char *in_buffer8b;
	unsigned short *out_buffer;
    int *pF  = NULL;  // Feature Transform Image,   Here is the closest(nearest)  point coordinates
    float *pDT  = NULL;  // Distance Transform Image


    int pnDimEach[MAXDIM];
    int nVoxelsNum;
    unsigned char *pBinaryImg = NULL;
    int ii, jj;
    int tmpLen;

    float min = 65535;
    float max = -65535;
    float smallest, largest;
    float scaleLarge;
    float var;
    bool bFTSave = false;
    float fResolutionF = (float)0.4, fScaleF;
	unsigned long *vol_start;
	int *pF_buf;
	float *pDT_buf;
	unsigned char *pBinaryImg_buf;


    if (argc>6 && sscanf(argv[6], "resolution=%f", &fResolutionF)==1)
        argc--;
    if (argc != 6)
    {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "%% LTDT3D input output mode distType bFTSave [resolution=<res>]\n");
        fprintf(stderr, "where:\n");
        fprintf(stderr, "input    : name of input file;\n");
        fprintf(stderr, "output   : name of output file;\n");
        fprintf(stderr, "mode     : mode of operation (0=foreground, 1=background);\n");
        fprintf(stderr, "distType : Distance Type, 0: background --> foreground; 1: foreground --> background; 2: both; 3: Digital Boundary\n");
        fprintf(stderr, "bFTSave  : 1: Save FT Image 0; Don't save FT Image. \n");
        fprintf(stderr, "<res>    : output resolution in pixel spacing\n");
        exit(1);
    }

     
 
    /* Open INPUT and OUTPUT Files */
    if( (fpin = fopen(argv[1], "rb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open INPUT file !\n");
        exit(1);
    }
    if( (fpout = fopen(argv[2], "wb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }
 
    /* Get EXECUTION MODE */
    sscanf(argv[3], "%d", &execution_mode);

    /* Get distType */
    sscanf(argv[4], "%d", &distType);

    /* Get bFTSave */
    sscanf(argv[5], "%d", (int *)&bFTSave );
 

    /* If in background mode, then place an entry in the BG_PROCESS.COM file */
    if(execution_mode == 1)
        VAddBackgroundProcessInformation((char *)"ndLTDT");


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 
    if( vh.scn.num_of_bits != 1 )
    {
        fprintf(stderr, "ERROR: The input image need to be a binary image.\n");
        exit(1);
    }

    /* Comoute information about the slices of the scene (number, locations, etc...) */
    compute_slices(&vh, &sl);


    /* Calculate length of a slice */
    width =  vh.scn.xysize[0];
    height =  vh.scn.xysize[1];

    length = (width * height + 7)  / 8;


    /* Allocate memory */

    /* create buffer for one binary image */
    if( (in_buffer1b = (unsigned char *) calloc(1, length) ) == NULL)
    {
        fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
        exit(1);
    }

    length = length*8;
    binary_flag = 1;

    /* create buffer for one grey image */
    if( (in_buffer8b = (unsigned char *) calloc(1, length) ) == NULL)
    {
        fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
        exit(1);
    }

    /* create buffer for one 16 bits grey-level image */
    if( (out_buffer = (unsigned short *) calloc( length, 2 ) ) == NULL)
    {
        fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
        exit(1);
    }



    nDim = 3;  // input dimension num

    pnDimEach[0] = vh.scn.xysize[0];
    pnDimEach[1] = vh.scn.xysize[1];

	pBinaryImg_buf = (unsigned char *)malloc(
		sl.total_slices*pnDimEach[0]*pnDimEach[1] * sizeof( unsigned char) );
    if( pBinaryImg_buf == NULL )
        return 0;
	pF_buf = (int *)malloc(
	    sl.total_slices*pnDimEach[0]*pnDimEach[1]*sizeof(int));
	pDT_buf = (float *)malloc(
		sl.total_slices*pnDimEach[0]*pnDimEach[1]*sizeof(float));
	if (pF_buf==NULL || pDT_buf==NULL)
	    return 0;

	vol_start = (unsigned long *)malloc(sl.volumes*sizeof(long));
	vol_start[0] = 0;
	for (j=1; j<sl.volumes; j++)
		vol_start[j] =
		    vol_start[j-1]+sl.slices[j-1]*vh.scn.xysize[0]*vh.scn.xysize[1];

    xypixsz[0] = distType==3? vh.scn.xypixsz[0]/2: vh.scn.xypixsz[0];
    xypixsz[1] = distType==3? vh.scn.xypixsz[1]/2: vh.scn.xypixsz[1];
    if (distType == 3)
        location3 = (float *)malloc(sl.max_slices*2*sizeof(float));

    /* Traverse ALL VOLUMES */
    k = 0;
    for(j=0; j<sl.volumes; j++)
    {
    	if (distType == 3)
		{
    	    for (i=0; i<sl.slices[j]; i++)
    	    {
    	        location3[2*i] = sl.location3[j][i];
	            location3[2*i+1] =
	                sl.location3[j][i]+(sl.location3[j][1]-sl.location3[j][0])/2;
	        }
	    }
	    else
	        location3 = sl.location3[j];
		pBinaryImg = pBinaryImg_buf+vol_start[j];
        pnDimEach[2] = sl.slices[j];

        nVoxelsNum = 1;
        for( i=0; i< nDim; i++ )          nVoxelsNum *= pnDimEach[i];

        fseek(fpin, k*(length/8)+hlength, 0);
		k += sl.slices[j];
        /* For each Volume, traverse ALL SLICES */
        for(i=0; i<sl.slices[j]; i++)
        {
            tmpLen = (int)fread(in_buffer1b, 1, (length/8), fpin);
            if( tmpLen != length/8)
            {
                fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
                exit(2);
            }

            /* Convert to 8 Bits */
            bin_to_grey(in_buffer1b, (length/8), in_buffer8b, 0, 255);

            for(  ii=0; ii< pnDimEach[0]; ii++ )
                for( jj=0; jj< pnDimEach[1]; jj++ )
                    pBinaryImg[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ] = in_buffer8b[jj*pnDimEach[0] + ii];

        }

		pF = pF_buf+vol_start[j];
		pDT = pDT_buf+vol_start[j];

	    if( distType == 3 )
	        GBDT(  pBinaryImg, pDT, pnDimEach,    nVoxelsNum );
	    else
	        LTSDT(  pBinaryImg, pF, pDT, pnDimEach,    nVoxelsNum, distType );
    }

    fScaleF = 1/(fResolutionF*xypixsz[0]);

    for(j=0; j<sl.volumes; j++)
    {
		pDT = pDT_buf+vol_start[j];

        for(i=0; i<sl.slices[j]; i++)
        {
            for(  ii=0; ii< pnDimEach[0]; ii++ )
                for( jj=0; jj< pnDimEach[1]; jj++ )
                {
                    float value = pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ]*fScaleF;
                    if(  value < min )
                        min = value;

                    if (  value > max )
                        max = value;
                }

        }
    }

    /*-------------------------*/
    /* Modify 3DViewnix Header */
    /* In case of binary, change header to 8 bits */
    smallest = 0;
    largest = max;
    if (-min > largest) largest = -min;
    scaleLarge = largest;
    if (largest > 65535)
	{
		largest = 65535;
		printf("Clipping to 65535.\n");
    }
	vh.scn.num_of_density_values  = 1;
    vh.scn.smallest_density_value = &smallest;
    vh.scn.largest_density_value  = &largest;
    vh.scn.num_of_bits = 16;
    vh.scn.bit_fields[1] = 15;

    /* Get the filenames right (own and parent) */
    strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1)-1);
    vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename)-1);
    vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
    /* Build "description" header entry */
    comments = (char *)calloc(1,1);
    for(i=0; i<argc; i++)
    {
        comments=(char *)realloc(comments, strlen(comments)+strlen(argv[i])+2);
        strcat(comments,argv[i]);
        strcat(comments," ");
    }
    vh.scn.description = (char *) malloc( strlen(comments) + 1);
    strcpy(vh.scn.description, comments);
    vh.scn.description_valid = 0x1;

    /* Write output 3DViewnix Header */
    error = VWriteHeader(fpout, &vh, group, element);
    if(error < 100)
    {
        fprintf(stderr, "ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
        exit(error);
    }

    for(j=0; j<sl.volumes; j++)
    {
		pDT = pDT_buf+vol_start[j];
        for(i=0; i<sl.slices[j]; i++)
        {
            for(  ii=0; ii< pnDimEach[0]; ii++ )
                for( jj=0; jj< pnDimEach[1]; jj++ )
                {
                    var = pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ]*fScaleF;
                    if ( var < 0 )
                        var = -var;

                    if ( scaleLarge>65535 && var>65535 )
                        out_buffer[jj*pnDimEach[0] + ii] = 65535;
                    else
                        out_buffer[jj*pnDimEach[0] + ii] = (unsigned short)var;
                }

            if (VWriteData( (char *)out_buffer, 2, pnDimEach[1]*pnDimEach[0],fpout,&k))
                fprintf(stderr, "Could not write data\n");

        }
    }

    VCloseData(fpout);

    // The following is: Output the feature transform image as IM0, the value is the coordinate index in the image
    if( bFTSave )
    {
        char *FT_FileName=(char *)malloc(strlen(argv[2])+8);
        strcpy( FT_FileName, argv[2] );
        strcat( FT_FileName, "_FT.IM0");
        if( (fpout = fopen( FT_FileName, "wb")) == NULL)
        {
            fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
            exit(1);
        }
        free(FT_FileName);

        smallest = 0;
        largest = (float)nVoxelsNum;
        vh.scn.num_of_density_values  = 1;
        vh.scn.smallest_density_value = &smallest;
        vh.scn.largest_density_value  = &largest;
        vh.scn.num_of_bits = 32;
        vh.scn.bit_fields[1] = 31;

        /* Write output 3DViewnix Header */
        error = VWriteHeader(fpout, &vh, group, element);
        if(error < 100)
        {
            fprintf(stderr, "ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
            exit(error);
        }

		pF = pF_buf+vol_start[j];
        if( VWriteData( (char *)pF, 4, nVoxelsNum,fpout,&k) )
            fprintf(stderr, "Could not write data\n");

        VCloseData(fpout);

    }

    if( out_buffer != NULL )
    {
        free(out_buffer);
        out_buffer = NULL;
    }
    if( in_buffer8b != NULL )
    {
        free(in_buffer8b);
        in_buffer8b = NULL;
    }
    if( in_buffer1b != NULL )
    {
        free(in_buffer1b);
        in_buffer1b = NULL;
    }

    if(execution_mode == 0)
    {
        printf("Done.\n");
        fflush(stdout);
    }

 
    /* If in Background mode remove the entry from the BG_PROCESS.COM */
    if(execution_mode == 1)
        VDeleteBackgroundProcessInformation();


    exit(0);
}
