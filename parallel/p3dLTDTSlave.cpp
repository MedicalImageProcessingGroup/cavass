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
file: p3dinterpolateSlave.cpp
*/
//----------------------------------------------------------------------
#include  <assert.h>
#include  <iostream>
#include  <limits.h>
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

#define  epsilon  (1e-3)  //for float comparisons for equality
//----------------------------------------------------------------------

//global variables:
extern int   nDim;
extern int   execution_mode;  //0=foreground, 1=background

extern int   MaxSlicesPerChunk;  //number of slices in a chunk
extern int   myRank;          //process number
extern char  myName[ MPI_MAX_PROCESSOR_NAME ];  //host name
//----------------------------------------------------------------------
#if 0
    #define say
#else
    static void say ( const char* const msg, const int indentChange=0 ) 
	{
        static int  indentLevel=0;
        if (indentChange==-1)    --indentLevel;
        if (indentLevel<0)       indentLevel=0;
        printf( "%*s slave %d %s \n", 3*indentLevel, " ", ::myRank, msg );
        if (indentChange==1)     ++indentLevel;
    }
                                                                                
    static void say ( const float value, const char* const msg,
                      const int indentChange=0 )
    {
        char  buff[256];
        sprintf( buff, msg, value );
        say( buff, indentChange );
    }
                                                                                
    static void say ( const int value, const char* const msg,
                      const int indentChange=0 )
    {
        char  buff[256];
        sprintf( buff, msg, value );
        say( buff, indentChange );
    }
                                                                                
    static void say ( const long value, const char* const msg,
                      const int indentChange=0 )
    {
        char  buff[256];
        sprintf( buff, msg, value );
        say( buff, indentChange );
    }
#endif

/*****************************************************************************
 * FUNCTION: abort_LTDT
 * DESCRIPTION: Issues a message and exits the process.
 * PARAMETERS:
 *    code: One of the 3DVIEWNIX error codes
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable execution_mode should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 5/5/2009 by Xinjian Chen
 *
 *****************************************************************************/
static void abort_LTDT ( int code ) {
    char msg[200];
                                                                                
    if (execution_mode == 1) {
        VDeleteBackgroundProcessInformation();
        system("job_done LTDT -abort &");
    }
    VDecodeError("p3dLTDT", "abort_LTDT", code, msg);
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
/*========================================================================= */


bool ITKCheck( float d1, float d2, float d3, int ud, int vd, int wd )
{	
	int a = vd - ud; 
	int b = wd - vd;
	int c = a+b;
	
	return ( c*d2 - b*d1 - a*d3 - a*b*c > 0 );
}

int  GetCoordinates( int index, int nCoord[], int *pnDimEach )
{
	int dim;
	int posi = index;	
	for( dim = 0; dim < nDim; dim++ )
	{
		nCoord[dim] = posi % pnDimEach[dim];
		posi /= pnDimEach[dim];
	}

	return 1;
}


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


double dist( int i, int j, int *pnDimEach )
{
	// get coordinates of i, j
	int nCoordi[MAXDIM]; // suppose max dimension 
	int nCoordj[MAXDIM];
	double dist2 = 0; 
	int dim;

	GetCoordinates( i, nCoordi, pnDimEach );
	GetCoordinates( j, nCoordj, pnDimEach );

	dist2 = 0;        // here, suppose p = 2
	for( dim = 0; dim < nDim; dim++ )
	{
		dist2 += ( nCoordj[dim] - nCoordi[dim] ) * ( nCoordj[dim] - nCoordi[dim] );
	}

//	dist2 = sqrt( dist2 );

	return dist2;
}

int FTDTDimUp( int c, int d, int F[], float DT[], int *pnDimEach, int nVoxelsNum )
{
	float q[MAXDIMLENGTH];
	int   indexD[MAXDIMLENGTH];
	int   g[MAXDIMLENGTH];

	int m = -1;
//	int Queue[MAXDIMLENGTH];
	int k=0;
	int l = 0;
//	int x;
	int nCoord[MAXDIM]; // suppose max dimension 
	int i;
	int xi;	
	
	GetCoordinates( c, nCoord, pnDimEach );

	for( i=0; i< pnDimEach[d]; i++ )
	{
		nCoord[d] = i;
		xi = GetCoordIndex( nCoord, pnDimEach );

		if( DT[xi] != INFINITY )  // -1 is empty (NULL) set
		{
			if( m < 1 )
			{
				m++;
			    q[m] = DT[xi];	indexD[m] = i;	g[m] = F[xi];  //g[m]  saved the feature transform
			}
			else
			{
				while ( m >= 1 && ITKCheck( q[m-1], q[m], DT[xi], indexD[m-1], indexD[m], i ) )   // start from 0, so +1
				{					
					m--;
				}

				m++;
				q[m] = DT[xi];  indexD[m] = i;	g[m] = F[xi];

			}
		
		}
	}	
	
	if( m > 0 )
	{	
		l = 0;

		for( i=0; i<pnDimEach[d]; i++ )
		{
			nCoord[d] = i;
			xi = GetCoordIndex( nCoord, pnDimEach );

			while( l < m-1 && ( q[l] + (indexD[l] -i) * (indexD[l] -i) > q[l+1] + (indexD[l+1] -i) * (indexD[l+1] -i) ) )
				l++;

			DT[xi] = q[l] + (indexD[l] -i) * (indexD[l] -i);
			F[xi]  = g[l];
		}
	}

	return 1;
}

int LTFTDT( unsigned char pBinaryImg[], int F[], float DT[], int *pnDimEach, int nVoxelsNum )
{
	int i, d;
	int x;
	int nCoordi[MAXDIM]; // suppose max dimension 	

	for( i= 0; i< nVoxelsNum; i++ )  //// Fg --> Bg distance
	{
		if( pBinaryImg[i] > 0 )   
		{
			DT[i] = INFINITY;
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
	DimTable1[0] = pnDimEach[1];	DimTable2[0] = pnDimEach[2];  // x == 0
	DimTable1[1] = pnDimEach[0];	DimTable2[1] = pnDimEach[2];  // y == 0
	DimTable1[2] = pnDimEach[0];	DimTable2[2] = pnDimEach[1];  // z == 0

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

				FTDTDimUp( x, d, F, DT, pnDimEach,	nVoxelsNum);
			}
		}
		
	}	

	//for( i= 0; i< nVoxelsNum; i++ )  
	//{
	//	DT[i] = sqrt( double(DT[i]) );
	//}
	
	return 1;

}


//----------------------------------------------------------------------
/** \brief main entry point for slave (worker) process.
 */
void slave ( void ) 
{

    cout << "    slave " << ::myRank << ": " << ::myName << ": hello" << endl;
    //for each message . . .
    for ( ; ; ) 
	{
        //wait for a message (first, get new message length)
        MPI_Status  status;
        MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );

		cout << "    slave " << ::myRank << ": " << ::myName << ": hello, in for loop" << endl;

        int  st_length = 0;
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
//#ifdef Verbose
//        cout << "    slave " << ::myRank << ":111 receiving; length is " << st_length << endl;
//#endif
        if (st_length <= 0)    continue;

        WorkChunk*  workChunk = (WorkChunk*)malloc( st_length );
        assert( workChunk!=NULL );
        //receive a message from the master
        MPI_Recv( workChunk, st_length, MPI_UNSIGNED_CHAR, 0,
                  MPI_ANY_TAG, MPI_COMM_WORLD, &status );
        if (status.MPI_TAG == WorkChunk::OP_EXIT) {
#ifdef Verbose
            MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
            cout << "    slave " << ::myRank << ": received; length is "
                 << st_length << " command is exit." << endl;
            workChunk->print();
#endif
            break;
        }

        if (status.MPI_TAG != WorkChunk::OP_LTDT) 
		{
            cout << "    slave " << ::myRank << ": " << ::myName
                 << ": unrecognized command." << endl;
            workChunk->print();
            continue;
        }
#ifdef Verbose
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
        cout << "    slave " << ::myRank << ":222 received; length is "
             << st_length << endl;
        workChunk->print();
#endif

        //start timers
        ElapsedTime  et;  
	/*	struct tms  usageBefore;
        times( &usageBefore );*/
    
		// Process LTSDT
		int nVoxelsNum = workChunk->volumeSize;
		int nVolSize = workChunk->inDataSize;
		unsigned char *pBoundaryImg = (unsigned char *)malloc( nVolSize );
		if( pBoundaryImg == NULL )
			return;
		memcpy( pBoundaryImg, workChunk->inData, nVolSize);

		int *pFT = (int *)malloc( nVoxelsNum *sizeof(int) );
		if( pFT == NULL )
			return;

		float *pDT = (float *)malloc( nVoxelsNum *sizeof(float) );
		if( pDT == NULL )
			return;

		LTFTDT(  pBoundaryImg, pFT, pDT, workChunk->dim,	nVoxelsNum );  // 2. new version, according to PAMI Maurer paper

		for( int i= 0; i< nVoxelsNum; i++ )  //// Fg --> Bg distance
		{
			pFT[i] += workChunk->firstPos;
		}

#ifdef Verbose
	cout << "   pDT[100]= " << pDT[100] << ", pDT[1000] = " << pDT[1000] << "nVoxelsNum =" << nVoxelsNum << endl;
	printf("printf: pDT[100] = %f, pDT[1000]= %f, nVoxelsNum = %d \n", pDT[100], pDT[1000], nVoxelsNum );
        
#endif

		const unsigned long  maxData = 2 * nVoxelsNum * sizeof(float);    
		const unsigned long  resultSize = sizeof(struct ResultChunk) + maxData;

	    ResultChunk*  resultChunk = (ResultChunk*) malloc( resultSize );
        assert( resultChunk!=NULL );
        
        resultChunk->operation   = ResultChunk::OP_RESULTS;
		memcpy(resultChunk->dim, workChunk->dim, 3*sizeof(int));
		resultChunk->firstPos = workChunk->firstPos;
		resultChunk->firstZSlice = workChunk->firstZSlice;
		resultChunk->volumeSize = workChunk->volumeSize;

	/*	resultChunk->DT_Data = (float*)malloc( nVoxelsNum * sizeof(float) );
		assert( resultChunk->DT_Data!=NULL );
		resultChunk->FT_Data = (int*)malloc( nVoxelsNum * sizeof(int) );
		assert( resultChunk->FT_Data!=NULL );*/

		memcpy((float*)resultChunk->DTFT_Data, pDT, nVoxelsNum*sizeof(float));
		memcpy( (int*)( resultChunk->DTFT_Data + nVoxelsNum*sizeof(float)), pFT, nVoxelsNum*sizeof(int));

		resultChunk->totalBytes  = resultSize;

        //stop timers and report time
    /*    struct tms  usageAfter;
        times( &usageAfter );
        resultChunk->cpuTime = ((double)usageAfter.tms_utime - usageBefore.tms_utime) / sysconf(_SC_CLK_TCK);*/
        resultChunk->computeTime = et.getElapsedTime();
#ifdef Verbose
        cout << "    slave " << ::myRank << ": compute time="
             << resultChunk->computeTime << endl;
      /*  cout << "    slave " << ::myRank << ": cpu time="
             << resultChunk->cpuTime << endl;*/
#endif
        //send results back to master
#ifdef Verbose
        cout << "    slave " << ::myRank << ": sending." << endl;
#endif

        MPI_Send( resultChunk, resultSize, MPI_UNSIGNED_CHAR, 0, 0,
                  MPI_COMM_WORLD );
#ifdef Verbose
        cout << "    slave " << ::myRank << ": sent." << endl;
        cout << "slave: free result chunk" << endl;
#endif
        free( resultChunk );
        resultChunk = NULL;
        //sleep( 10 );
#ifdef Verbose
        cout << "slave: free work chunk" << endl;
#endif
        free( workChunk );
        workChunk = NULL;

		if( pBoundaryImg != NULL )
		{ 
			free( pBoundaryImg ); pBoundaryImg = NULL;
		}
		if( pDT != NULL )
		{ 
			free( pDT ); pDT = NULL;
		}
		if( pFT != NULL )
		{ 
			free( pFT ); pFT = NULL;
		}
    }  //end forever
   
}
//----------------------------------------------------------------------

