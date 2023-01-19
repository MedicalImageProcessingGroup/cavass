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

/**
 * \brief  this program performs a ball scale transform of the input gray
 *         image data.  input and output files are in IM0 format.
 */
#include  <stdlib.h>
#include  <iostream>

#include  <itkImage.h>
#include  "itkIM0VolumeReader.h"
#include  "itkIM0VolumeWriter.h"
#include  "ElapsedTime.h"

#include  "Scale.h"
#include  "BallScale.h"

static bool    g2D         = true;
static double  gTs         = 0.85;
static double  gTh         = 3.0;
static double  gMaxScale   = 500;
static bool    gDoubleFlag = false;
//----------------------------------------------------------------------
static void usage ( char* programName ) {
  std::cerr << "Usage:" << std::endl << std::endl
      << "    " << programName << " [-2|3] [-d] [-Ts value] [-Th value] [-max value] inputImageFile outputScaleImageFile" << std::endl
	  << "        -2   = 2d" << std::endl
	  << "        -3   = 3d (default is " << g2D << ")" << std::endl
      << "        -d   = output distance map values as doubles (default is " << gDoubleFlag << ")" << std::endl
      << "        -Ts  = Ts value (default is " << gTs << ")" << std::endl
      << "        -Th  = Th value (default is " << gTh << ")" << std::endl
      << "        -max = max scale value (default is " << gMaxScale << ")" << std::endl
      << std::endl;
  exit( EXIT_FAILURE );
}
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    ElapsedTime  et;  //timer class

    int  nextArg = 1;
    for ( ; ; ) {
        if (nextArg>=argc)    usage( argv[0] );

        if (strcmp(argv[nextArg],"-2")==0) {
			g2D = true;
            ++nextArg;
		} else if (strcmp(argv[nextArg],"-3")==0) {
			g2D = false;
            ++nextArg;
		} else if (strcmp(argv[nextArg],"-d")==0) {
            gDoubleFlag = true;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-Ts")==0) {
            ++nextArg;
            if (nextArg>=argc)    usage( argv[0] );
            //get the arg value
            const char* const  ptr = argv[nextArg];
            char*              endptr;
            const double       d = strtod( ptr, &endptr );
            if (endptr == ptr)  usage( argv[0] );  //ignore bad values
            gTs = d;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-Th")==0) {
            ++nextArg;
            if (nextArg>=argc)    usage( argv[0] );
            //get the arg value
            const char* const  ptr = argv[nextArg];
            char*              endptr;
            const double       d = strtod( ptr, &endptr );
            if (endptr == ptr)  usage( argv[0] );  //ignore bad values
            gTh = d;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-max")==0) {
            ++nextArg;
            if (nextArg>=argc)    usage( argv[0] );
            //get the arg value
            const char* const  ptr = argv[nextArg];
            char*              endptr;
            const double       d = strtod( ptr, &endptr );
            if (endptr == ptr)  usage( argv[0] );  //ignore bad values
            gMaxScale = d;
            ++nextArg;
        } else if (argv[nextArg][0]=='-') {
            usage( argv[0] );
        } else {
            break;
        }
    }

    //declare the input and output image
    const unsigned int  Dimension = 3;
    typedef  double  InputPixelType;
    typedef  double  OutputPixelType;
    typedef  itk::Image<InputPixelType,  Dimension >  InputImageType;
    typedef  itk::Image<OutputPixelType, Dimension >  OutputImageType;
    InputImageType::Pointer  inputImage = InputImageType::New();

    //declare the reader
    typedef  itk::IM0VolumeReader< InputPixelType, InputImageType >  ImageReaderType;
    ImageReaderType::Pointer  IM0Reader = ImageReaderType::New();

    if (nextArg>=argc)    usage( argv[0] );
    cout << "input file = " << argv[0] << std::endl;
    IM0Reader->SetFileName(  argv[nextArg] );
    ++nextArg;
    IM0Reader->SetInputImage( inputImage );
    IM0Reader->Execute();

    //copy the input data to a temporary buffer
	/** \todo if 2d only, we could process one slice at a time
	 *  (which might be the only option for large data sets). */
    itk::ImageRegionIterator< InputImageType >
        it( inputImage, inputImage->GetBufferedRegion() );
    it.GoToBegin();
    InputImageType::SizeType  size = inputImage->GetBufferedRegion().GetSize();
    InputPixelType*  d = (InputPixelType*)malloc( size[0]*size[1]*size[2]*sizeof(InputPixelType) );
    assert( d != NULL );
    int  i = 0;
    for (int z=0; z<(int)size[2]; z++) {
        for (int y=0; y<(int)size[1]; y++) {
            for (int x=0; x<(int)size[0]; x++) {
                assert( !it.IsAtEnd() );
                InputPixelType  value = it.Get();
                d[i++] = value;
                ++it;
            }
        }
    }

    //perform the scale computation
    BallScale<InputPixelType>  bs( d, size[0], size[1], size[2], 1.0, 1.0, 1.0, g2D );
	bs.setTs( gTs );
	//bs.setTh( gTh );
	bs.setMaxScale( gMaxScale );

    //copy the results back to the input image (so that we can subsequently
    // just connect input to output and save the results).
    it.GoToBegin();
    for (int z=0; z<(int)size[2]; z++) {
		cout << "z=" << z << endl;
        for (int y=0; y<(int)size[1]; y++) {
		    cout << " y=" << y;
            for (int x=0; x<(int)size[0]; x++) {
                assert( !it.IsAtEnd() );
				it.Set( bs.estimateScale(x,y,z) );
                ++it;
            }
        }
		cout << endl << et.getElapsedTime() << endl;
    }

    //declare a writer
    typedef  itk::IM0VolumeWriter< OutputImageType >  WriterType;
    WriterType::Pointer  IM0writer = WriterType::New();

    if (gDoubleFlag) {
        puts( "output is double" );
        IM0writer->SetOutputDoubleData( true );
    }
    if (nextArg>=argc)    usage( argv[0] );
    IM0writer->SetFileName( argv[nextArg] );
    ++nextArg;
    IM0writer->SetInputImage( inputImage );
    IM0writer->Execute();

    std::cout << "Elapsed time: " << et.getElapsedTime() << "s." << std::endl;

    return 0;
}
//----------------------------------------------------------------------
