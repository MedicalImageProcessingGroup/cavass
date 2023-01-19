/*
  Copyright 1993-2008 Medical Image Processing Group
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
 * \brief this program performs a 3D distance transform via Danielsson's
 * method.
 * distance transform values are output as signed ints (or optionally as
 * doubles).  input and output files are in IM0 format.
 */
#include <stdlib.h>
#include <iostream>

#include <itkImage.h>
#include "itkIM0VolumeReader.h"
#include "itkIM0VolumeWriter.h"
#include "ElapsedTime.h"

#include "itkSignedDanielssonDistanceMapImageFilter.h"
#include "FilterProgress.h"

static bool  outputDouble = false;
//----------------------------------------------------------------------
static void usage ( char* programName ) {
  std::cerr << "Usage:" << std::endl << std::endl
      << "    " << programName << " [-d] inputImageFile outputDistanceMapImageFile" << std::endl
      << "        " << "-d = output distance map values as doubles" << std::endl
      << std::endl;
    exit( EXIT_FAILURE );
}

//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    ElapsedTime  et;  /* timer class */

    int  nextArg = 1;
    if (nextArg>=argc)    usage( argv[0] );
    if (strcmp(argv[nextArg],"-d")==0) {
        ::outputDouble = true;
        ++nextArg;
    }

    // Declare the image
    const unsigned int      Dimension = 3;
    typedef  unsigned char  InputPixelType;
    typedef  double         OutputPixelType;
    typedef  itk::Image<InputPixelType,  Dimension >  InputImageType;
    typedef  itk::Image<OutputPixelType, Dimension >  OutputImageType;

    InputImageType::Pointer  inputImage = InputImageType::New();

    //Declare the reader
    typedef  itk::IM0VolumeReader< InputPixelType, InputImageType >  ImageReaderType;
    ImageReaderType::Pointer  IM0Reader = ImageReaderType::New();

    if (nextArg>=argc)    usage( argv[0] );
    IM0Reader->SetFileName(  argv[nextArg] );
    ++nextArg;
    IM0Reader->SetInputImage( inputImage );
    IM0Reader->Execute();

    typedef  itk::SignedDanielssonDistanceMapImageFilter< InputImageType, OutputImageType >  FilterType;

    FilterType::Pointer  filter = FilterType::New();

    filter->SetInput( inputImage );
    //filter->InputIsBinaryOn();
    filter->UseImageSpacingOn();
    //filter->SquaredDistanceOn();
    filter->SquaredDistanceOff();
    filter->InsideIsPositiveOn();

    FilterProgress::Pointer  observer = FilterProgress::New();
    filter->AddObserver( itk::ProgressEvent(), observer );
    filter->Update();

    // Declare a Writer
    typedef  itk::IM0VolumeWriter< OutputImageType >  WriterType;

    WriterType::Pointer  IM0writer = WriterType::New();

    if (::outputDouble) {
        puts( "output is double" );
        IM0writer->SetOutputDoubleData( true );
    }
    if (nextArg>=argc)    usage( argv[0] );
    IM0writer->SetFileName( argv[nextArg] );
    ++nextArg;
    IM0writer->SetInputImage( filter->GetOutput() );
    IM0writer->Execute();

    std::cout << "Elapsed time: " << et.getElapsedTime() << "s." << std::endl;

    return 0;
}

