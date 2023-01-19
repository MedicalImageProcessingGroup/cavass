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
file: p3dinterpolate.h
*/
//----------------------------------------------------------------------
//command line arguments from original ndinterpolate program:
/*
Usage:
  ndinterpolate input output mode p1 p2 p3 [p4] mb m1 m2 m3 [m4 Vi Vf]
                [I1 F1 ... In Fn]

where:

  input	: name of the input file
  output: name of the ouput file
  mode 	: 0=foreground execution, 1=background execution
  p1	: pixel size along X1 (Ox) 
  p2	: pixel size along X2 (Oy)
  p3	: pixel size along X3 (Oz)
  p4	: pixel size along X4 (Ot)
  mb	: method for distance map calculation (any value for grey
            scenes) : 0=city block, 1=chamfer
  m1	: method along X1 (Ox) : 0=nearest neighbor, 1=linear, 2=cubic
  m2	: method along X2 (Oy) : 0=nearest neighbor, 1=linear, 2=cubic
  m3	: method along X3 (Oz) : 0=nearest neighbor, 1=linear, 2=cubic
  m4	: method along X4 (Ot) : 0=nearest neighbor, 1=linear, 2=cubic
  Vi,Vf	: first and final volumes used for interpolation
  I1,F1	: on first volume interpolate between slices I1 and F1
  In,Fn	: on nth volume interpolate between slices In and Fn

or:

  ndinterpolate input output argument_file

where:

  input	 : name of the input file
  output : name of the ouput file
  argument_file : file containing the remaining arguments (one per line) 
                  as described above
*/
#define  Verbose
//command line arguments for this program (position in argv):
enum {
//  argVIEWNIX_ENV=1,  //path for viewnix_env
  argIN=1,    //input file name
  argOUT,   //output file name
  argMODE,  //0=foreground; 1=background
  argP1,    //desired (output) pixel size along X1 (Ox)
  argP2,    //desired (output) pixel size along X2 (Oy)
  argP3,    //desired (output) pixel size along X3 (Oz)
  argMB,    //method for distance map calculation (any value for grey scenes) : 0=city block, 1=chamfer
  argM1,    //method along X1 (Ox): 0=nearest neighbor, 1=linear, 2=cubic
  argM2,    //method along X2 (Oy): 0=nearest neighbor, 1=linear, 2=cubic
  argM3,    //method along X3 (Oz): 0=nearest neighbor, 1=linear, 2=cubic
  argS1,    //first slice (real number; physical position) (currently ignored)
  argS2,    //last slice (real number; physical position) (currently ignored)
  argSPC    //slices per chunk
};
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
struct WorkChunk {
    void print ( ) const {
        std::cout << std::endl << "WorkChunk " << (unsigned long)this
          << std::endl
          << "  totalBytes=" << totalBytes << "  operation=" << operation;
        switch (operation) {
            case OP_INTERPOLATE :  std::cout << "(interpolate)";  break;
            case OP_RESULTS :      std::cout << "(results)";      break;
            case OP_EXIT :         std::cout << "(exit)";         break;
            default :              std::cout << "(?)";            break;
        }
        std::cout
          << "  degree=(" << degree[0] << "," << degree[1] << ","
                                              << degree[2] << ")"
          << "  chamferFlag=" << chamferFlag
          << "  inCount=(" << inCount[0] << "," << inCount[1] << ","
                                                << inCount[2] << ")"
          << "  inSize=(" << inSize[0] << "," << inSize[1] << ","
                                              << inSize[2] << ")"
          << "  inFirstZSlice=" << inFirstZSlice
          << "  outCount=(" << outCount[0] << "," << outCount[1] << ","
                                                  << outCount[2] << ")"
          << "  outSize=(" << outSize[0] << "," << outSize[1] << ","
                                                << outSize[2] << ")"
          << "  outDataType=" << outDataType
          << "  outFirstZSlice=" << outFirstZSlice
          << "  outFirstZLoc=" << outFirstZLoc
          << "  outLastZLoc=" << outLastZLoc
          << "  inZLocationOffset=" << inZLocationOffset;
        if (inZLocationOffset>=0) {
            if (operation==OP_EXIT)    std::cout << ", values=[...]";
            else {
                locType*  ptr = (locType*) &inData[inZLocationOffset];
                std::cout << ", values=[ ";
                for (int i=0; i<inCount[2]; i++) {
                    std::cout << ptr[i] << " ";
                }
                std::cout << "]";
            }
        } else std::cout << ", values=[]";
        if ( inZLocationOffset!=-1 && inDataOffset!=-1
          && inZLocationOffset>=inDataOffset )
            std::cout << " *** warning: inZLocationOffset>=inDataOffset *** ";
        std::cout
          << "  inDataOffset=" << inDataOffset
          << "  inDataType="   << inDataType
          << "  inData="       << (unsigned long)inData
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
    enum    { OP_INTERPOLATE, OP_RESULTS, OP_EXIT };
    int     operation;          //one of the above

    enum    { D_NN, D_LN, D_QU, D_CU };
    int     degree[3];          //(x,y,z), 0=D_NN=nn,   1=D_LN=ln,
                                //         2=D_QU=quad, 3=D_CU=cube
    enum    { UNUSED, CITY_BLOCK, CHAMFER };
    int     chamferFlag;        //one of the above
    int     inCount[3];         //(x,y,z) int size/count of pixels
    double  inSize[3];          //(x,y,z) physical size (typically in mm);
                                // z size may vary from slice to slice in input
    int     inFirstZSlice;      //first z slice number (int subscript starting
                                // w/ 0 in the original, input data set)
    //
    int     outCount[3];        //(x,y,z) int size/count of pixels
    double  outSize[3];         //(x,y,z) physical size (typically in mm)
    int     outDataType;        //one of the enum'd types
    int     outFirstZSlice;     //first desired z slice number (int subscript
                                // starting w/ 0
    typedef float locType;
    //we need both of the fields below because of overlap needed for cubic
    // interpolation
    locType outFirstZLoc;       //physical location of first output (desired)
                                // z slice
    locType outLastZLoc;        //physical location of last output (desired)
                                // z slice
    //
    long    inZLocationOffset;  //offset in bytes from beginning of data[];
                                // a value of -1 indicates that this is unused;
                                // each value is of type locType;
                                // there should be outCount[2] of them (or -1
                                // indicating that there are 0 of them).
    long    inDataOffset;       //offset in bytes from beginning of data[];
                                // a value of -1 indicates that this is unused;
                                // there should be
                                // inCount[0]*inCount[1]*inCount[2] of them.
    //
    int     inDataType;         //one of the enum'd types above
    char    inData[];           //data contains BOTH:
                                //  (i)  a list of input z locations
                                //       (as indicated by inZLocationOffset)
                                //  (ii) the input data to be interpolated
                                //       (as indicated by inDataOffset)
};
//----------------------------------------------------------------------
struct ResultChunk {
    void print ( ) const {
        std::cout << "ResultChunk " << this << std::endl
          << "  totalBytes=" << totalBytes << "  operation=" << operation
          << "  count=(" << count[0] << "," << count[1] << "," << count[2] << ")"
          << "  size=(" << size[0] << "," << size[1] << "," << size[2] << ")"
          << std::endl
          << "  firstZSlice="    << firstZSlice
          << "  firstZLocation=" << firstZLocation
          << "  lastZLocation="  << lastZLocation
          << "  computeTime="    << computeTime << " cpuTime=" << cpuTime
          << "  min,max="        << min << "," << max
          << "  dataType="       << dataType << std::endl;
    }

    enum    { INT1, INT8,  INT16,  INT32,  INT64,  //types
              UINT8, UINT16, UINT32, UINT64,
              REAL32, REAL64, REAL128,
              COMPLEX64, COMPLEX128, COMPLEX256
    };

    long    totalBytes;         //length in bytes of everything in this
                                // struct including all data
    enum    { OP_INTERPOLATE, OP_RESULTS, OP_EXIT };
    int     operation;          //one of the above

    //all of these fields are with respect to the output (result)
    int     count[3];           //(x,y,z)
    double  size[3];            //(x,y,z)
    int     firstZSlice;        //first z slice number (int subscript starting
                                // w/ 0 in the output data set)
    double  firstZLocation;     //physical location of first z slice
    double  lastZLocation;      //physical location of last z slice
    //stats
    double  computeTime;        //elapsed compute time
    double  cpuTime;            //user-mode cpu time
    long    min, max;           //min and max data values
    //
    int     dataType;           //one of the enum'd types
    char    data[];             //data contains only the (output) result
                                // of interpolation
};
//----------------------------------------------------------------------
void master ( int argc, char* argv[] );
void slave  ( void );
//----------------------------------------------------------------------

