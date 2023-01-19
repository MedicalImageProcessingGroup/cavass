/*
  Copyright 1993-2012 Medical Image Processing Group
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
 * \brief this program outputs a 3D image that is the difference between two
 * input images.  difference values are signed int by default (or optionally
 * as doubles).  input and output files are in IM0 format.  summary stats are
 * reported as well.
 */
#include <stdlib.h>
#include <iostream>

#include <itkImage.h>
#include "itkIM0VolumeReader.h"
#include "itkIM0VolumeWriter.h"
#include "ElapsedTime.h"

#include "itkDifferenceImageFilter.h"
#include "itkImageRegionIterator.h"

#include "FilterProgress.h"

static bool  outputDouble = false;
static bool  outputBinary = false;
static bool  verbose      = false;
static bool  summary      = false;
//----------------------------------------------------------------------
static void usage ( char* programName ) {
  std::cerr << std::endl << "Usage: " << std::endl
    << programName << " [-b] [-d] [-s] [-v] inputImageFile1 inputImageFile2 outputDifferenceImageFile" << std::endl
    << "        -b = output binary only (0=no difference; 1=difference)" << std::endl
    << "        -d = output difference values as doubles" << std::endl
    << "        -s = output summary information" << std::endl
    << "        -v = output verbose information (e.g., locations of individual differences)" << std::endl
    << "        Note: -b and -d are mutually exclusive!" << std::endl
    << std::endl;
  exit( EXIT_FAILURE );
}
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    ElapsedTime  et;  /* timer class */
    int  nextArg = 1;

    for ( ; ; ) {
        if (nextArg>=argc)    usage( argv[0] );

        if (strcmp(argv[nextArg],"-b")==0) {
            if (::outputDouble)    usage( argv[0] );
            ::outputBinary = true;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-d")==0) {
            if (::outputBinary)    usage( argv[0] );
            ::outputDouble = true;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-s")==0) {
            ::summary = true;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-v")==0) {
            ::verbose = true;
            ++nextArg;
        } else if (strcmp(argv[nextArg],"-")==0) {
            usage( argv[0] );
        } else {
            break;
        }
    }

    //declare the images
    const unsigned int  Dimension = 3;
    typedef  double  InputPixelType;
    typedef  double  OutputPixelType;
    typedef  itk::Image< InputPixelType,  Dimension >  InputImageType;
    typedef  itk::Image< OutputPixelType, Dimension >  OutputImageType;

    InputImageType::Pointer  in1 = InputImageType::New();
    InputImageType::Pointer  in2 = InputImageType::New();

    //declare the readers
    typedef  itk::IM0VolumeReader< InputPixelType, InputImageType >
             ImageReaderType;
    ImageReaderType::Pointer  IM0Reader1 = ImageReaderType::New();
    ImageReaderType::Pointer  IM0Reader2 = ImageReaderType::New();

    if (nextArg>=argc)    usage( argv[0] );
    IM0Reader1->SetFileName(  argv[nextArg] );
    ++nextArg;
    IM0Reader1->SetInputImage( in1 );
    IM0Reader1->Execute();

    if (nextArg>=argc)    usage( argv[0] );
    IM0Reader2->SetFileName(  argv[nextArg] );
    ++nextArg;
    IM0Reader2->SetInputImage( in2 );
    IM0Reader2->Execute();

    typedef  itk::DifferenceImageFilter< InputImageType, OutputImageType >
             FilterType;

    FilterType::Pointer  filter = FilterType::New();

    filter->SetInput( in1 );
    filter->SetTestInput( in2 );

    FilterProgress::Pointer  observer = FilterProgress::New();
    filter->AddObserver( itk::ProgressEvent(), observer );
    filter->Update();

    if (::verbose) {
        itk::ImageRegionIterator< OutputImageType >  it( filter->GetOutput(),
            filter->GetOutput()->GetBufferedRegion() );
        it.GoToBegin();
        FilterType::OutputImageType::SizeType
            size = filter->GetOutput()->GetBufferedRegion().GetSize();
        for (unsigned int z=0; z<size[2]; z++) {
            for (unsigned int y=0; y<size[1]; y++) {
                for (unsigned int x=0; x<size[0]; x++) {
                    assert( !it.IsAtEnd() );
                    OutputPixelType  value = it.Get();
                    if (value!=0) {
                        std::cout << "(" << x << "," << y << "," << z << ")="
                                  << value << std::endl;
                    }
                    ++it;
                }
            }
        }
    }
    if (::summary || ::verbose) {
        std::cout << "mean diff = " << filter->GetMeanDifference() << std::endl
                  << "diff count= "
                  << filter->GetNumberOfPixelsWithDifferences() << std::endl
                  << "total diff= " << filter->GetTotalDifference()
                  << std::endl;
    }

    if (::outputBinary) {
        itk::ImageRegionIterator< OutputImageType >  it( filter->GetOutput(),
            filter->GetOutput()->GetBufferedRegion() );
        it.GoToBegin();
        while (!it.IsAtEnd()) {
            OutputPixelType  value = it.Get();
            if (value!=0)    it.Set( 1 );
            ++it;
        }
    }

    //declare a writer
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
//----------------------------------------------------------------------
