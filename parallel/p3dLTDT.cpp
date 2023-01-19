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
g++ -pthread -O3 -o p3dinterpolate.exe -I/usr/mipgsun/3dviewnix/linux/INCLUDE \
    p3dinterpolate.cpp /usr/mipgsun/3dviewnix/linux/LIBRARY/libviewnixd.a \
    -llammpio -llammpi++ -llamf77mpi -lmpi -llam -lutil

mpiCC -O3 -o p3dinterpolate.exe -I/usr/mipgsun/3dviewnix/linux/INCLUDE \
    p3dinterpolate.cpp /usr/mipgsun/3dviewnix/linux/LIBRARY/libviewnixd.a

run:
    ./p3dinterpolate.exe 10  intrpl-tmp2.IM0 out.IM0
    mpirun C -v -s n0 ./p3dinterpolate.exe 10 intrpl-tmp2.IM0 out.IM0

	"\Program Files\MPICH2\bin\mpiexec.exe" -n 4 p3dinterpolate C:\data\regular.IM0 intrpl-tmp.IM0 0 0.97660 0.97660 0.97660 0 1 1 1 10
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
int   nDim;
int   execution_mode;  ///< 0=foreground, 1=background
int   MaxSlicesPerChunk;  ///< number of slices in a chunk
int   myRank;          ///< process number
char  myName[ MPI_MAX_PROCESSOR_NAME ];  ///< host computer name
//----------------------------------------------------------------------
static void checkMemory ( void ) {
#if defined (WIN32) || defined (_WIN32)
	cout << "process " << ::myRank << ": memory check ";
	if (_CrtCheckMemory())    cout << "OK." << endl;
	else                      cout << "NOT OK." << endl;

	cout << "process " << ::myRank << ": leaks ";
	if (_CrtDumpMemoryLeaks())    cout << "OK." << endl;
	else                          cout << "NOT OK." << endl;
#endif
}
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
#if defined (WIN32) || defined (_WIN32)
	_CrtSetDbgFlag(
		_CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF |
	    _CRTDBG_CHECK_CRT_DF |
	    _CRTDBG_DELAY_FREE_MEM_DF |
	    _CRTDBG_LEAK_CHECK_DF );
#endif
	checkMemory();
    //this MUST be kept in sync with enum { INT1,... and dataTypeSizes and
    // dataTypeBits.
    assert( WorkChunk::INT1 == 0 );

	MaxSlicesPerChunk = 300;
	nDim = 3;

    ElapsedTime  et;
    MPI_Init( &argc, &argv );
    gethostname( ::myName, sizeof(::myName) );

	//determine rank (rank==0 -> master; rank!=0 -> slave)
    MPI_Comm_rank( MPI_COMM_WORLD, &::myRank );
	checkMemory();
	if (::myRank==0)    master( argc, argv );
	else                slave();
    checkMemory();
    if (::myRank==0) {
        const double  t = et.getElapsedTime();
        cout << "master " << ::myRank << ": elapsed time="
             << t << " sec (" << (t/60) << " min)" << endl;
    }
	cout << flush;
    
    MPI_Finalize();
	exit( 0 );
}
//----------------------------------------------------------------------
