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
file: p3dLTDT.h
Author: Xinjian Chen
*/
//----------------------------------------------------------------------
//command line arguments from original ndinterpolate program:
/*
usage (master): \n\
  mpirun C -v -s n0 ./p3dLTDT.exe input output mode distType \n\
where: \n\
  \n\
  input : name of the input file \n\
  output: name of the ouput file \n\
  mode  : 0=foreground execution, 1=background execution \n\
  distType: distance type: 0: background --> foreground; 1: foreground --> background; 2: both \n\
  \n\
example: mpirun C -v -s n0 ./p3dLTDT.exe child0.IM0 junk.IM0 0 0\n\
  \n" << endl;
*/

#define INFINITY  300000000
#define MAXDIM    10
#define MAXDIMLENGTH  10000
#define MAXDIST  INFINITY

#define LTSDT_DIST

int  GetCoordinates( int index, int nCoord[], int *pnDimEach );
int GetCoordIndex( int nCoord[], int *pnDimEach );
int Check( int u, int v, int w, int d, int nCoord[], int *pnDimEach );

//#define  Verbose
//command line arguments for this program (position in argv):
enum {
//  argVIEWNIX_ENV=1,  //path for viewnix_env
  argIN=1,    //input file name
  argOUT,   //output file name
  argMODE,  //0=foreground; 1=background
  argDISTTYPE,    //distance type: 0: background --> foreground; 1: foreground --> background; 2: both 
};

extern "C" {
	int VGetHeaderLength ( FILE* fp, int* hdrlen );
}
//----------------------------------------------------------------------
//size in bytes for each data type.
//MUST be kept in sync with enum { INT1, ... below.
static const int  dataTypeSizes[] = { 1, 1, 2, 4, 8,
                                         1, 2, 4, 8,
                                         4, 8, 16,
                                         8, 16, 32 };

//size in bits for each data type.
//MUST be kept in sync with enum { INT1, ... below.
static const int  dataTypeBits[]  = { 1, 1*8, 2*8, 4*8, 8*8,
                                         1*8, 2*8, 4*8, 8*8,
                                         4*8, 8*8, 16*8,
                                         8*8, 16*8, 32*8 };
//----------------------------------------------------------------------
struct WorkChunk 
{
    void print ( ) const 
	{
        std::cout << std::endl << "WorkChunk " << (unsigned long)this
          << std::endl
          << "  totalBytes=" << totalBytes << "  operation=" << operation;
        switch (operation) 
		{
            case OP_LTDT :         std::cout << "(LTDT)";  break;
            case OP_RESULTS :      std::cout << "(results)";      break;
            case OP_EXIT :         std::cout << "(exit)";         break;
            default :              std::cout << "(?)";            break;
        }
        std::cout
          << "  dim=(" << dim[0] << "," << dim[1] << "," << dim[2] << ")"
          << "  volumeSize=(" << volumeSize << ")"
          << "  firstZSlice=" << firstZSlice
          << "  firstPos=" << firstPos        
          << std::endl << std::endl;
    }

    //MUST be kept in sync with dataTypeSizes and dataTypeBits above.
    enum    { INT1,   INT8,   INT16,  INT32,  INT64,  //types
              UINT8,  UINT16, UINT32, UINT64,
              REAL32, REAL64, REAL128,
              COMPLEX64, COMPLEX128, COMPLEX256
    };

    long    totalBytes;         //length in bytes of everything in this
                                // struct including all data
    enum    { OP_LTDT, OP_RESULTS, OP_EXIT };
    int     operation;          //one of the above
    
    int     dim[3];         //(x,y,z) int size/count of pixels
    int     firstZSlice;      //first z slice number (int subscript starting
                                // w/ 0 in the original, input data set)
    int     firstPos;
    int     volumeSize;
    //
    int     inDataType;         //one of the enum'd types above
	int     inDataSize;
    char    inData[];           //data contains Binary image for LTSDT
                  
};
//----------------------------------------------------------------------
struct ResultChunk 
{
    void print ( ) const 
	{
        std::cout << "ResultChunk " << this << std::endl
          << "  totalBytes=" << totalBytes << "  operation=" << operation
          << std::endl
            << "  dim=(" << dim[0] << "," << dim[1] << "," << dim[2] << ")"
          << "  volumeSize=(" << volumeSize << ")"
          << "  firstZSlice=" << firstZSlice
          << "  firstPos=" << firstPos        
          << std::endl << std::endl;
    }

    enum    { INT1, INT8,  INT16,  INT32,  INT64,  //types
              UINT8, UINT16, UINT32, UINT64,
              REAL32, REAL64, REAL128,
              COMPLEX64, COMPLEX128, COMPLEX256
    };

    long    totalBytes;         //length in bytes of everything in this
                                // struct including all data
    enum    { OP_LTDT, OP_RESULTS, OP_EXIT };
    int     operation;          //one of the above

    //all of these fields are with respect to the output (result)
    int     dim[3];         //(x,y,z) int size/count of pixels
    int     firstZSlice;      //first z slice number (int subscript starting
                                // w/ 0 in the original, input data set)
    int     firstPos;
    int     volumeSize;
    //stats
    double  computeTime;        //elapsed compute time
    double  cpuTime;            //user-mode cpu time
    long    min, max;           //min and max data values
    //
    int     dataType;           //one of the enum'd types
	char    DTFT_Data[];    // contains following
    //float    *DT_Data;             //data contains only the (output) result
    //int      *FT_Data;                     // of interpolation
};
//----------------------------------------------------------------------
void master ( int argc, char* argv[] );
void slave  ( void );
//----------------------------------------------------------------------
int FTDTDimUp( int c, int d, int F[], float DT[], int *pnDimEach, int nVoxelsNum );

