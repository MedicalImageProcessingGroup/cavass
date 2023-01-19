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

#include "itkRecursiveGaussianImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "FilterProgress.h"

int main( int argc, char * argv[] )
{

  if( argc < 4 ) 
    { 
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << "  inputImageFile  outputImageFile sigma " << std::endl;
    return EXIT_FAILURE;
    }

  // Declare the image
  const unsigned int    Dimension = 3;
  typedef  float  InputPixelType;
  typedef  float  OutputPixelType;
  typedef itk::Image<InputPixelType, Dimension >  InputImageType;
  typedef itk::Image<OutputPixelType, Dimension > OutputImageType;

  InputImageType::Pointer inputImage = InputImageType::New();

  //Declare the reader
  typedef itk::IM0VolumeReader< InputPixelType, InputImageType  > ImageReaderType;
  ImageReaderType::Pointer  IM0ImageReader  = ImageReaderType::New();
  
  
  IM0ImageReader->SetFileName(  argv[1] );
  IM0ImageReader->SetInputImage(inputImage);
  IM0ImageReader->Execute();

  typedef itk::RecursiveGaussianImageFilter<
                        InputImageType, OutputImageType >  FilterType;

   FilterType::Pointer filter = FilterType::New();

   FilterType::Pointer filterX = FilterType::New();
   FilterType::Pointer filterY = FilterType::New();
   FilterType::Pointer filterZ = FilterType::New();

 

  filterX->SetDirection( 0 );   // 0 --> X direction
  filterY->SetDirection( 1 );   // 1 --> Y direction
  filterZ->SetDirection( 2 );   // 2 --> Z direction

  filterX->SetOrder( FilterType::ZeroOrder );
  filterY->SetOrder( FilterType::ZeroOrder );
  filterZ->SetOrder( FilterType::ZeroOrder );

  filterX->SetNormalizeAcrossScale( false );
  filterY->SetNormalizeAcrossScale( false );
  filterZ->SetNormalizeAcrossScale( false );


  const double sigma = atof( argv[3] );

  filterX->SetSigma( sigma );
  filterY->SetSigma( sigma );
  filterZ->SetSigma( sigma );

  filterX->SetInput( inputImage);
  filterY->SetInput( filterX->GetOutput() );
  filterZ->SetInput( filterY->GetOutput() );
 
//  filter->SetInput(inputImage);
//  filter->Update();


 typedef itk::Image< unsigned char, Dimension >  CharImageType;

  typedef itk::RescaleIntensityImageFilter< 
                                  OutputImageType,
                                  CharImageType >    RescaleFilterType;

  RescaleFilterType::Pointer rescale = RescaleFilterType::New();

  rescale->SetInput( filterZ->GetOutput() );
 
  rescale->SetOutputMinimum(   0 );
  rescale->SetOutputMaximum( 255 );

  FilterProgress::Pointer  observer = FilterProgress::New();
  rescale->AddObserver( itk::ProgressEvent(), observer );

  rescale->Update();

  // Declare a Writer
  typedef itk::IM0VolumeWriter< CharImageType >  WriterType;
  
  WriterType::Pointer      writer =  WriterType::New();

  writer->SetFileName( argv[2] );
  writer->SetInputImage(rescale->GetOutput());
  writer->Execute();

  return EXIT_SUCCESS;
}

