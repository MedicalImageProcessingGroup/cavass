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

#include  "p3dLTDT.h"

using namespace std;
//----------------------------------------------------------------------
//global variables:
extern int   nDim;
extern int   MaxSlicesPerChunk;  //number of slices in a chunk
extern int   myRank;          //process number
extern char  myName[ MPI_MAX_PROCESSOR_NAME ];  //host name

static float   firstZLoc;  //for input and output
static float   lastZLoc;   //for input and output
static int     outputZSliceCount;  //count of desired output slices
static float*  outputZLocations;   //actual z locations of output slices
static bool*   outputZAssigned;    //assigned to a slave/worker or not yet?
static int     sDTMethod=0;
static int     distType=0;
//----------------------------------------------------------------------
static void usageAndExit ( const char* const m=NULL ) {
  cout << "\n\
usage (master): \n\
  mpirun C -v -s n0 ./p3dLTDT.exe input output mode distType \n\
where: \n\
  \n\
  input : name of the input file \n\
  output: name of the ouput file \n\
  mode  : 0=foreground execution, 1=background execution \n\
  distType: distance type: 0: background --> foreground; 1: foreground --> background; 2: both  3: Digital Boundary\n\
  \n\
example: mpirun C -v -s n0 ./p3dLTDT.exe child0.IM0 junk.IM0 0 0\n\
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

    if (argc<=argDISTTYPE)    usageAndExit();    
    
    if (sscanf(argv[argDISTTYPE], "%d", &distType)!=1)    usageAndExit("bad distType");    
    
}
//----------------------------------------------------------------------
static void initFiles ( FILE* infp, FILE* outfp ) 
{
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


//-------------------------------------------------------------------------
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



int GBDT(  unsigned char pBinaryImg[], float pDT[], int *pnDimEach, int nVoxelsNum )
{
	int i, d;
	int x, y, c;
	int nVoxelsNumX2 = nVoxelsNum; 
	int nCoordx[MAXDIM]; // suppose max dimension 
	int nCoordy[MAXDIM]; // suppose max dimension 
	int nCoordc[MAXDIM]; // suppose max dimension 
	int pnDimEachX2[MAXDIM];

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
			GetCoordinates( x, nCoordx, pnDimEach );

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

	int *pFX2  = NULL;  // Feature Transform Image,   Here is the closest(nearest)  point coordinates
	float *pDTX2  = NULL;  // Feature Transform Image

	pFX2 = (int *)malloc( nVoxelsNumX2 * sizeof( int) );
	if( pFX2 == NULL )
		return 0;

	pDTX2 = (float *)malloc( nVoxelsNumX2 * sizeof( float) );
	if( pDTX2 == NULL )
		return 0;

	ElapsedTime  et_p;  
//	LTFTDT(  NULL, pBinaryImgX2, pFX2, pDTX2, pnDimEachX2, nVoxelsNumX2 );
	
	//determine number of processes
    int  nTasks;
    MPI_Comm_size( MPI_COMM_WORLD, &nTasks );
#ifdef Verbose
    cout << "master " << ::myRank << ": mpi comm size = " << nTasks << endl;
#endif
    //send one unit of work to each slave
	int   bytesPerSlice = pnDimEachX2[0] * pnDimEachX2[1] * sizeof(unsigned char);
	const unsigned long  maxData = bytesPerSlice * ::MaxSlicesPerChunk;    
    const unsigned long  totalBytes = sizeof(struct WorkChunk) + maxData;
                                      
    struct WorkChunk*  wChunk = (struct WorkChunk*)malloc( totalBytes );  //totalBytes );
    assert( wChunk!=NULL );

    //init the work chunk
    memset( wChunk, 0, totalBytes );
    wChunk->totalBytes     = totalBytes;
    wChunk->operation      = WorkChunk::OP_LTDT;	
	wChunk->dim[0] = pnDimEachX2[0];	wChunk->dim[1] = pnDimEachX2[1];	 // x, y, z
//	wChunk->inData = NULL; 

    int  workingCount = 0;
    for (int rank=1; rank<nTasks; rank++) 
	{
		cout << "master " << ::myRank << ": attempting to send" << endl;

		int pos = 0, size = 0;
		int slicePerChunk = pnDimEachX2[2]/(nTasks-1);

		if( rank<nTasks-1 )
		{
			wChunk->dim[2] = slicePerChunk;	
			pos = (rank-1) * slicePerChunk * pnDimEachX2[0] * pnDimEachX2[1];
			size = slicePerChunk * pnDimEachX2[0] * pnDimEachX2[1] * sizeof( char);
			wChunk->volumeSize = slicePerChunk * pnDimEachX2[0] * pnDimEachX2[1];
		}
		else
		{
			wChunk->dim[2] =  pnDimEachX2[2] - (nTasks-2) * slicePerChunk;	
			pos = (nTasks-2) * slicePerChunk  * pnDimEachX2[0] * pnDimEachX2[1];
			size = ( pnDimEachX2[2] - (nTasks-2) * slicePerChunk ) * pnDimEachX2[0] * pnDimEachX2[1] * sizeof( char);
			wChunk->volumeSize = ( pnDimEachX2[2] - (nTasks-2) * slicePerChunk ) * pnDimEachX2[0] * pnDimEachX2[1];
		}
		
		wChunk->firstPos = pos;
		wChunk->firstZSlice = (rank-1) * slicePerChunk ;
		
	/*	wChunk->inData = (char *)malloc(size);
		assert( wChunk->inData!=NULL );*/
		wChunk->inDataSize = size;
		memcpy(wChunk->inData, &(pBinaryImgX2[pos]), size);

	//	wChunk->totalBytes = sizeof(struct WorkChunk) + size;

#ifdef Verbose
    cout << "master " << ::myRank << ": in for loop, rank = " << rank << "pnDimEachX2[2] = " << pnDimEachX2[2] << ", wChunk->totalBytes = " << wChunk->totalBytes << endl;
#endif
    
		ElapsedTime  et;
		MPI_Send( wChunk, wChunk->totalBytes, MPI_UNSIGNED_CHAR, rank, wChunk->operation, MPI_COMM_WORLD );  // assign work

		++workingCount;
    }
#ifdef Verbose
    cout << "master " << ::myRank << ": initial assignment complete." << endl;
#endif
    
	struct ResultChunk*  resultChunk = NULL;
    long  resultChunkMaxBytes = 0;
    while (workingCount>0) 
	{
        //get results
        cout << "master " << ::myRank << ": attempting to receive" << endl;

        MPI_Status  status;
        MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
        //re/allocate the result chunk (if necessary)
//        if (status.st_length>resultChunkMaxBytes) {
        int  st_length = 0;
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
        if (st_length > resultChunkMaxBytes) 
		{
            if (resultChunk!=NULL) 
			{
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
        
        //saveWork( outfp, resultChunk );
		memcpy(&(pDTX2[resultChunk->firstPos]), resultChunk->DTFT_Data, resultChunk->volumeSize*sizeof(float));
		memcpy(&(pFX2[resultChunk->firstPos]), resultChunk->DTFT_Data + resultChunk->volumeSize*sizeof(float), resultChunk->volumeSize*sizeof(int));

        --workingCount;

        //any more work to assign?
        //if (::outputZAssigned[::outputZSliceCount-1])    continue;

        ////there is more work to assign
        //wChunk->outCount[2] = setZInfo( wChunk, invh );
        //assignWork( rank, workingCount, wChunk, invh, size, infp,
        //            bytesPerSlice, outfp );
    }  //end while


    //send 'time-to-quit' messages
    wChunk->operation = WorkChunk::OP_EXIT;
    for (int rank=1; rank<nTasks; rank++) 
	{
#ifdef Verbose
        cout << "master " << ::myRank << ": sending time-to-quit message to "
             << rank << endl;
#endif
        MPI_Send( wChunk, sizeof(struct WorkChunk), MPI_UNSIGNED_CHAR, rank,
                  wChunk->operation, MPI_COMM_WORLD );
    }

	free( wChunk );
    wChunk = NULL;

    if (resultChunk!=NULL) {
        free( resultChunk );
        resultChunk = NULL;
    }

#ifdef Verbose

	cout << "    master " << ::myRank << ": Divide and Receive compute time="
             << et_p.getElapsedTime() << endl;

#endif

	ElapsedTime  et_c;  

	int nCoordi[MAXDIM];
	nCoordi[2] = 0;
	for( int i = 0; i< pnDimEachX2[0]; i++)
	{
		nCoordi[ 0 ] = i;
		for( int j = 0; j< pnDimEachX2[1]; j++)
		{
			nCoordi[ 1 ] = j;

			x = GetCoordIndex( nCoordi, pnDimEachX2 );

			FTDTDimUp( x, 2, pFX2, pDTX2, pnDimEachX2,	nVoxelsNumX2);
		}
	}


	for( i= 0; i< nVoxelsNumX2; i++ )  //// Fg --> Bg distance
	{
		pDTX2[i] = sqrt( double(pDTX2[i]) );
	}
	
	///////////////////// Pararell end ///////////////////

	
	for( x= 0; x< nVoxelsNum; x++ )
	{
		GetCoordinates( x, nCoordx, pnDimEach );
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

	
	return 1;

}


int LTSDT(  unsigned char pBinaryImg[], float pDT[], int *pnDimEach, int nVoxelsNum )
{
	int i, j, k;
	int x, y;	
	int nCoordx[MAXDIM]; // suppose max dimension 
	int nCoordy[MAXDIM]; // suppose max dimension 	
	int *pF;
	unsigned char * pBoundaryImg;
	
	ElapsedTime  et_p;  

	pBoundaryImg = (unsigned char *)malloc( nVoxelsNum * sizeof( unsigned char) );
	if( pBoundaryImg == NULL )
		return 0;

	memset(pBoundaryImg, 1, nVoxelsNum  * sizeof( unsigned char) );

	for( x= 0; x< nVoxelsNum; x++ )
	{
		if( pBinaryImg[x] == 0 )
		{
			GetCoordinates( x, nCoordx, pnDimEach );			

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

						if( pBinaryImg[y] == 255 )
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

	pF = (int *)malloc( nVoxelsNum * sizeof( int) );
	if( pF == NULL )
		return 0;

    //determine number of processes
    int  nTasks;
    MPI_Comm_size( MPI_COMM_WORLD, &nTasks );
#ifdef Verbose
    cout << "master " << ::myRank << ": mpi comm size = " << nTasks << endl;
#endif
    //send one unit of work to each slave
	int   bytesPerSlice = pnDimEach[0] * pnDimEach[1] * sizeof(unsigned char);
	const unsigned long  maxData = bytesPerSlice * ::MaxSlicesPerChunk;    
    const unsigned long  totalBytes = sizeof(struct WorkChunk) + maxData;
                                      
    struct WorkChunk*  wChunk = (struct WorkChunk*)malloc( totalBytes );  //totalBytes );
    assert( wChunk!=NULL );

    //init the work chunk
    memset( wChunk, 0, totalBytes );
    wChunk->totalBytes     = totalBytes;
    wChunk->operation      = WorkChunk::OP_LTDT;	
	wChunk->dim[0] = pnDimEach[0];	wChunk->dim[1] = pnDimEach[1];	 // x, y, z
//	wChunk->inData = NULL; 

    int  workingCount = 0;
    for (int rank=1; rank<nTasks; rank++) 
	{
		cout << "master " << ::myRank << ": attempting to send" << endl;

		int pos = 0, size = 0;
		int slicePerChunk = pnDimEach[2]/(nTasks-1);

		if( rank<nTasks-1 )
		{
			wChunk->dim[2] = slicePerChunk;	
			pos = (rank-1) * slicePerChunk * pnDimEach[0] * pnDimEach[1];
			size = slicePerChunk * pnDimEach[0] * pnDimEach[1] * sizeof( char);
			wChunk->volumeSize = slicePerChunk * pnDimEach[0] * pnDimEach[1];
		}
		else
		{
			wChunk->dim[2] =  pnDimEach[2] - (nTasks-2) * slicePerChunk;	
			pos = (nTasks-2) * slicePerChunk  * pnDimEach[0] * pnDimEach[1];
			size = ( pnDimEach[2] - (nTasks-2) * slicePerChunk ) * pnDimEach[0] * pnDimEach[1] * sizeof( char);
			wChunk->volumeSize = ( pnDimEach[2] - (nTasks-2) * slicePerChunk ) * pnDimEach[0] * pnDimEach[1];
		}
		
		wChunk->firstPos = pos;
		wChunk->firstZSlice = (rank-1) * slicePerChunk ;
		
	/*	wChunk->inData = (char *)malloc(size);
		assert( wChunk->inData!=NULL );*/
		wChunk->inDataSize = size;
		memcpy(wChunk->inData, &(pBoundaryImg[pos]), size);

	//	wChunk->totalBytes = sizeof(struct WorkChunk) + size;

#ifdef Verbose
    cout << "master " << ::myRank << ": in for loop, rank = " << rank << "pnDimEach[2] = " << pnDimEach[2] << ", wChunk->totalBytes = " << wChunk->totalBytes << endl;
#endif
    
		ElapsedTime  et;
		MPI_Send( wChunk, wChunk->totalBytes, MPI_UNSIGNED_CHAR, rank, wChunk->operation, MPI_COMM_WORLD );  // assign work

		++workingCount;
    }
#ifdef Verbose
    cout << "master " << ::myRank << ": initial assignment complete." << endl;
#endif
    
	struct ResultChunk*  resultChunk = NULL;
    long  resultChunkMaxBytes = 0;
    while (workingCount>0) 
	{
        //get results
        cout << "master " << ::myRank << ": attempting to receive" << endl;

        MPI_Status  status;
        MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
        //re/allocate the result chunk (if necessary)
//        if (status.st_length>resultChunkMaxBytes) {
        int  st_length = 0;
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
        if (st_length > resultChunkMaxBytes) 
		{
            if (resultChunk!=NULL) 
			{
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
        
        //saveWork( outfp, resultChunk );
		memcpy(&(pDT[resultChunk->firstPos]), resultChunk->DTFT_Data, resultChunk->volumeSize*sizeof(float));
		memcpy(&(pF[resultChunk->firstPos]), resultChunk->DTFT_Data + resultChunk->volumeSize*sizeof(float), resultChunk->volumeSize*sizeof(int));

        --workingCount;

        //any more work to assign?
        //if (::outputZAssigned[::outputZSliceCount-1])    continue;

        ////there is more work to assign
        //wChunk->outCount[2] = setZInfo( wChunk, invh );
        //assignWork( rank, workingCount, wChunk, invh, size, infp,
        //            bytesPerSlice, outfp );
    }  //end while


    //send 'time-to-quit' messages
    wChunk->operation = WorkChunk::OP_EXIT;
    for (int rank=1; rank<nTasks; rank++) 
	{
#ifdef Verbose
        cout << "master " << ::myRank << ": sending time-to-quit message to "
             << rank << endl;
#endif
        MPI_Send( wChunk, sizeof(struct WorkChunk), MPI_UNSIGNED_CHAR, rank,
                  wChunk->operation, MPI_COMM_WORLD );
    }

	free( wChunk );
    wChunk = NULL;

    if (resultChunk!=NULL) {
        free( resultChunk );
        resultChunk = NULL;
    }

#ifdef Verbose

	cout << "    master " << ::myRank << ": Divide and Receive compute time="
             << et_p.getElapsedTime() << endl;

#endif

	ElapsedTime  et_c;  

	int nCoordi[MAXDIM];
	nCoordi[2] = 0;
	for( int i = 0; i< pnDimEach[0]; i++)
	{
		nCoordi[ 0 ] = i;
		for( int j = 0; j< pnDimEach[1]; j++)
		{
			nCoordi[ 1 ] = j;

			x = GetCoordIndex( nCoordi, pnDimEach );

			FTDTDimUp( x, 2, pF, pDT, pnDimEach,	nVoxelsNum);
		}
	}


	for( i= 0; i< nVoxelsNum; i++ )  //// Fg --> Bg distance
	{
		pDT[i] = sqrt( double(pDT[i]) );
	}
	

	
	for( x= 0; x< nVoxelsNum; x++ )
	{
		if( pBinaryImg[x] == 0 )
			pDT[x] = pDT[x];
		else
			pDT[x] = -pDT[x];

	}
	
	if( pF != NULL )
		free(pF);

#ifdef Verbose
//	cout << "   pDT[100]= " << pDT[100] << ", pDT[1000] = " << pDT[1000] << endl;

        cout << "    master " << ::myRank << ": Combination compute time="
             << et_c.getElapsedTime() << endl;

#endif

	return 1;

}

 
/*---------------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
void bin_to_grey(unsigned char * bin_buffer,int length_bin, unsigned char * grey_buffer,int min_value, int max_value)
{
        int i, j;
        unsigned char mask[8];
        unsigned char *bin, *grey;
 
        bin = bin_buffer;
        grey = grey_buffer;
 
        /* initialize masks */
        mask[0] = 1;
        for(i=1; i<8; i++)
           mask[i] = mask[i-1] * 2;
 
        for(i=length_bin; i>0; i--)
        {
           for(j=7; j>=0; j--)
           {
                if( (*bin & mask[j]) != 0) *grey = max_value;
                else *grey = min_value;
 
                grey++;
           }
           bin++;
        }
 
 
}


//----------------------------------------------------------------------
void master ( int argc, char* argv[] ) 
{
	int length;			/* length of a slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
//	int distType;		/* Distance Type, 0: background --> foreground; 1: foreground --> background; 2: both */
	int i,j,k;			/* general use */
	int error;			/* error code */	

	unsigned char *in_buffer1b;
	unsigned char *in_buffer8b,	*out_buffer8;
	int *pF  = NULL;  // Feature Transform Image,   Here is the closest(nearest)  point coordinates
	float *pDT  = NULL;  // Feature Transform Image

		
	int pnDimEach[MAXDIM];
	int nVoxelsNum;
	unsigned char *pBinaryImg = NULL;
	int ii, jj;
	int tmpLen;

	int min = 65535;
	int max = -65535;
	float scaleLarge;
	float var;

	ElapsedTime  et;  

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

	 error = VGetHeaderLength(infp, &hlength);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, argv[argIN]);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }

    //determineOutputSliceLocations( invh );

#ifdef Verbose
    cout << "master " << ::myRank << ": number of input slices = "
         << invh.scn.num_of_subscenes[0] << endl;
#endif	
 
	if( invh.scn.num_of_bits != 1 )
	{
		printf("ERROR: The input image need to be a binary image.\n");
		exit(1);
	}

	
	/* Calculate length of a slice */
	width =  invh.scn.xysize[0];
	height =  invh.scn.xysize[1];

	length = (width * height + 7)  / 8;


	/* Allocate memory */

	/* create buffer for one binary image */
	if( (in_buffer1b = (unsigned char *) calloc(1, length) ) == NULL)
	{
		printf("ERROR: Can't allocate input image buffer.\n");
		exit(1);
	}

	length = length*8;	

	/* create buffer for one grey image */
	if( (in_buffer8b = (unsigned char *) calloc(1, length) ) == NULL)
	{
		printf("ERROR: Can't allocate input image buffer.\n");
		exit(1);
	}

	/* create buffer for one 8 bits grey-level image */
	if( (out_buffer8 = (unsigned char *) calloc(1, length ) ) == NULL)
	{
		printf("ERROR: Can't allocate output image buffer.\n");
		exit(1);
	}	

	nDim = invh.scn.dimension;  // input dimension num	

	pnDimEach[0] = invh.scn.xysize[0]; //256;  x
	pnDimEach[1] = invh.scn.xysize[1]; //256;  y
	pnDimEach[2] = invh.scn.num_of_subscenes[0];

	nVoxelsNum = 1;
	for( i=0; i< nDim; i++ )  		nVoxelsNum *= pnDimEach[i];

	pBinaryImg = (unsigned char *)malloc( nVoxelsNum * sizeof( unsigned char) );
	if( pBinaryImg == NULL )
		return;

	/* Traverse ALL VOLUMES */	
	k = 0;
	for(j=0; j<1; j++) //sl.volumes
	{
		fseek( infp, (k*length/8)+hlength, 0);
		/* For each Volume, traverse ALL SLICES */
		for(i=0; i<pnDimEach[2]; i++)
		{
			tmpLen = fread(in_buffer1b, 1, (length/8), infp);
			if( tmpLen != length/8)
			{
				printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
				exit(2);
			}
			
			/* Convert to 8 Bits */
			bin_to_grey(in_buffer1b, (length/8), in_buffer8b, 0, 255);

			for(  ii=0; ii< pnDimEach[0]; ii++ )
				for( jj=0; jj< pnDimEach[1]; jj++ )
					pBinaryImg[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ] = in_buffer8b[jj*pnDimEach[0] + ii];

		}
	}


	pDT = (float *)malloc( (nVoxelsNum) * sizeof( float) );  // nor enough memory for super dataset
	if( pDT == NULL )
		return;

	if( distType == 3)
		GBDT(  pBinaryImg, pDT, pnDimEach,	nVoxelsNum );
	else
		LTSDT(  pBinaryImg, pDT, pnDimEach,	nVoxelsNum );
	
	ElapsedTime  et111;  

	for(j=0; j<1; j++)
	{
		for(i=0; i<pnDimEach[2]; i++)
		{
			for(  ii=0; ii< pnDimEach[0]; ii++ )
				for( jj=0; jj< pnDimEach[1]; jj++ )
				{
					if(  pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ] < min )
						min = pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ];
					else if (  pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ] > max )
						max = pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ];
				}

		}
	}

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


	float smallest =0, largest = 255;
	char comments[300];
	smallest = 0;
    largest = max;
    if (-min > largest) largest = -min;
	scaleLarge = largest;
    if (largest > 255)  largest = 255;
	outvh.scn.num_of_density_values  = 1;
    outvh.scn.smallest_density_value = &smallest;
    outvh.scn.largest_density_value  = &largest;
    outvh.scn.num_of_bits = 8;
	outvh.scn.bit_fields[1] = 7;

	/* Get the filenames right (own and parent) */
    strcpy(outvh.gen.filename1, argv[argIN]);
    strcpy(outvh.gen.filename, argv[argOUT]);
    /* Build "description" header entry */
	strcpy(comments,"");
    for(i=0; i<argc; i++)
    {
        strcat(comments,argv[i]);
        strcat(comments," ");
    }
    outvh.scn.description = (char *) malloc( strlen(comments) + 1);
    strcpy(outvh.scn.description, comments);
    outvh.scn.description_valid = 0x1;

    err = VWriteHeader( outfp, &outvh, group, element );
    if (err && err < 106) {
        cerr << "Can't write the output file, " << argv[argOUT] << "." << endl;
        exit( -1 );
    }	
	
	for(j=0; j<1; j++)  //sl.volumes
	{
		for(i=0; i<pnDimEach[2]; i++) //sl.slices[j]
		{
			for(  ii=0; ii< pnDimEach[0]; ii++ )
				for( jj=0; jj< pnDimEach[1]; jj++ )
				{
					var = pDT[ i * pnDimEach[1]*pnDimEach[0] + jj*pnDimEach[0] + ii ];
					if ( var < 0 )
						var = -var;

					if ( scaleLarge > 255 )
						((unsigned char*) out_buffer8)[jj*pnDimEach[0] + ii] = var*255/scaleLarge;
					else
						((unsigned char*) out_buffer8)[jj*pnDimEach[0] + ii] = var;
				}

			if (VWriteData( (char *)out_buffer8, 1, pnDimEach[1]*pnDimEach[0],outfp,&k))
				printf("Could not write data\n");

		}
	}

	VCloseData(outfp);

	if( pF != NULL )
	{
		free(pF);
		pF = NULL;
	}
	if( pDT != NULL )
	{
		free(pDT);
		pDT = NULL;
	}

	if( out_buffer8 != NULL )
	{
		free(out_buffer8);
		out_buffer8 = NULL;
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
   

    outfp = NULL;  //fclose( outfp );  
    fclose( infp );     infp  = NULL;
	
	
#ifdef Verbose
	 cout << "    master " << ::myRank << ": After LTSDT, Saving time is"
             << et111.getElapsedTime() << endl;

	  cout << "    master " << ::myRank << ": Total Network + Compute time="
             << et.getElapsedTime() << endl;

    cout << "master " << ::myRank << " bye." << endl;
#endif
}
//----------------------------------------------------------------------

