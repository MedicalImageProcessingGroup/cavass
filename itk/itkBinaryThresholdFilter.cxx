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


#include <stdlib.h>
#include <iostream>

#include <itkImage.h>
#include "itkIM0VolumeReader.h"
#include "itkIM0VolumeWriter.h"

#include "itkBinaryThresholdImageFilter.h"
#include "FilterProgress.h"

int main( int argc, char * argv[] )
{

    if( argc < 7 )
    {
        std::cerr << "Usage: " << argv[0];
        std::cerr << " inputFile outputFile ";  
        std::cerr << " lowerTh upper ";  
        std::cerr << " outValue inValue   "  << std::endl;  
        return EXIT_FAILURE;
    }

    // Declare the image
    const unsigned int    Dimension = 3;
    typedef  unsigned short  InputPixelType;
    typedef  unsigned short  OutputPixelType;
    typedef itk::Image<InputPixelType, Dimension >  InputImageType;
    typedef itk::Image<OutputPixelType, Dimension > OutputImageType;

    InputImageType::Pointer inputImage = InputImageType::New();

    //Declare the reader
    typedef itk::IM0VolumeReader< InputPixelType, InputImageType  > ImageReaderType;
    ImageReaderType::Pointer  IM0ImageReader  = ImageReaderType::New();


    IM0ImageReader->SetFileName(  argv[1] );
    IM0ImageReader->SetInputImage(inputImage);
    IM0ImageReader->Execute();


    typedef itk::BinaryThresholdImageFilter<
        InputImageType, OutputImageType >  FilterType;


    //Setting the ITK pipeline filter
    FilterType::Pointer filter = FilterType::New();


    const OutputPixelType outsideValue = atoi( argv[5] );
    const OutputPixelType insideValue  = atoi( argv[6] );

    filter->SetOutsideValue( outsideValue );
    filter->SetInsideValue(  insideValue  );

    const InputPixelType lowerThreshold = atoi( argv[3] );
    const InputPixelType upperThreshold = atoi( argv[4] );

    filter->SetLowerThreshold( lowerThreshold );
    filter->SetUpperThreshold( upperThreshold );

    filter->SetInput(inputImage);

    FilterProgress::Pointer  observer = FilterProgress::New();
    filter->AddObserver( itk::ProgressEvent(), observer );

    filter->Update();

    // Declare a Writer
    typedef itk::IM0VolumeWriter< OutputImageType >  WriterType;

    WriterType::Pointer      writer =  WriterType::New();

    writer->SetFileName( argv[2] );
    writer->SetInputImage(filter->GetOutput());
    writer->Execute();

    return EXIT_SUCCESS;
}

