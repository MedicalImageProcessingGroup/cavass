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

#include "itkRescaleIntensityImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkCurvatureAnisotropicDiffusionImageFilter.h"
#include "FilterProgress.h"

int main( int argc, char * argv[] )
{

    if( argc < 6 ) 
    { 
        std::cerr << "Usage: " << std::endl;
        std::cerr << argv[0] << "  inputImageFile  outputImageFile ";
        std::cerr << "numberOfIterations  timeStep  conductance" << std::endl;
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

    typedef itk::CurvatureAnisotropicDiffusionImageFilter<
        InputImageType, OutputImageType >  FilterType;
    FilterType::Pointer filter = FilterType::New();

    const unsigned int numberOfIterations = atoi( argv[3] );
    const double       timeStep = atof( argv[4] );
    const double       conductance = atof( argv[5] );
    const bool         useImageSpacing = (argc != 6);

    filter->SetNumberOfIterations( numberOfIterations );
    filter->SetTimeStep( timeStep );
    filter->SetConductanceParameter( conductance );

    filter->SetInput(inputImage);
    if (useImageSpacing)
    {
        filter->UseImageSpacingOn();
    }

    FilterProgress::Pointer  observer = FilterProgress::New();
    filter->AddObserver( itk::ProgressEvent(), observer );

    filter->Update();

    //calculate the min and max value in the distance map
    typedef itk::MinimumMaximumImageCalculator< OutputImageType>  CalculatorType;
    CalculatorType::Pointer calculator = CalculatorType::New();

    calculator->SetImage( filter->GetOutput());
    calculator->Compute();

    double minimum = calculator->GetMinimum();
    double maximum = calculator->GetMaximum();

    unsigned short usmax = static_cast<unsigned short> (maximum+0.5);
    unsigned short usmin = static_cast<unsigned short> (minimum+0.5);

    typedef itk::Image< unsigned short, Dimension >  UShortImageType;

    typedef itk::RescaleIntensityImageFilter< 
        OutputImageType,
        UShortImageType >    RescaleFilterType;

    RescaleFilterType::Pointer rescale = RescaleFilterType::New();

    rescale->SetInput( filter->GetOutput() );

    rescale->SetOutputMinimum( usmin );
    rescale->SetOutputMaximum( usmax );
    rescale->Update();

    // Declare a Writer
    typedef itk::IM0VolumeWriter< UShortImageType >  WriterType;

    WriterType::Pointer      writer =  WriterType::New();

    writer->SetFileName( argv[2] );
    writer->SetInputImage(rescale->GetOutput());
    writer->Execute();

    return EXIT_SUCCESS;
}

