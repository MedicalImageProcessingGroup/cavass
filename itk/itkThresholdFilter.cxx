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

#include "itkThresholdImageFilter.h"
#include "FilterProgress.h"

int main( int argc, char * argv[] )
{

  if( argc < 6 )
    {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " inputFile outputFile ";  
    std::cerr << " outsideValue"<<" option (1:Below; 2:Above; 3:interval)";
    std::cerr << " Threshold1 [Threshold2] "<<std::endl;  
    return EXIT_FAILURE;
    }
  
  // Declare the image
  const unsigned int    Dimension = 3;
  typedef  unsigned short  InputPixelType;

  typedef itk::Image<InputPixelType, Dimension >  InputImageType;

  InputImageType::Pointer inputImage = InputImageType::New();

  //Declare the reader
  typedef itk::IM0VolumeReader< InputPixelType, InputImageType  > ImageReaderType;
  ImageReaderType::Pointer  IM0ImageReader  = ImageReaderType::New();
  
  
  IM0ImageReader->SetFileName(  argv[1] );
  IM0ImageReader->SetInputImage(inputImage);
  IM0ImageReader->Execute();


  typedef itk::ThresholdImageFilter<InputImageType>  FilterType;


  //Setting the ITK pipeline filter
  FilterType::Pointer filter = FilterType::New();


  const InputPixelType outsideValue = ::atoi( argv[3] );

  const int option = ::atoi( argv[4] );
  const InputPixelType Threshold1 = ::atoi( argv[5] );

  InputPixelType Threshold2;  
  if(option == 3)
    Threshold2 = ::atoi( argv[6] );

  switch (option)
    {
    case 1:
      filter->ThresholdBelow( Threshold1 );
      break;
    case 2:
      filter->ThresholdAbove( Threshold1);
      break;
    case 3:
      filter->ThresholdOutside(Threshold1, Threshold2);
      break;
    default:
      std::cerr<<"wrong option!"<<std::endl;
      return EXIT_FAILURE;
    }

  filter->SetOutsideValue( outsideValue );
  filter->SetInput(inputImage);  

  FilterProgress::Pointer  observer = FilterProgress::New();
  filter->AddObserver( itk::ProgressEvent(), observer );

  filter->Update();

  // Declare a Writer
  typedef itk::IM0VolumeWriter< InputImageType >  WriterType;
  
  WriterType::Pointer      writer =  WriterType::New();

  writer->SetFileName( argv[2] );
  writer->SetInputImage(filter->GetOutput());
  writer->Execute();

  return EXIT_SUCCESS;
}

