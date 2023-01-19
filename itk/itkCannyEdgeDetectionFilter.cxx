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

#include "itkCastImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkCannyEdgeDetectionImageFilter.h"
#include "FilterProgress.h"

int main( int argc, char * argv[] )
{
    if( argc < 7 )
    {
        std::cerr << "Usage: " << std::endl;
        std::cerr << argv[0] << "  inputImageFile outputImageFile variance maximum_err upper_th lower_th" << std::endl;
        return 1;
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


    //  typedef unsigned char    CharPixelType;  //  IO
    typedef double           RealPixelType;  //  Operations

    //  typedef itk::Image<CharPixelType, Dimension>    CharImageType;
    typedef itk::Image<RealPixelType, Dimension>    RealImageType;

    typedef itk::CastImageFilter< InputImageType, RealImageType> CastToRealFilterType;
    typedef itk::CannyEdgeDetectionImageFilter<RealImageType, RealImageType> CannyFilter;
    typedef itk::RescaleIntensityImageFilter<RealImageType, OutputImageType > RescaleFilter;

    CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
    RescaleFilter::Pointer rescale = RescaleFilter::New();

    //Setting the ITK pipeline filter
    CannyFilter::Pointer cannyFilter = CannyFilter::New();


    //The output of an edge filter is 0 or 1
    rescale->SetOutputMinimum(   0 );
    rescale->SetOutputMaximum( 255 );

    toReal->SetInput( inputImage );

    cannyFilter->SetInput( toReal->GetOutput() );
    cannyFilter->SetVariance( ::atof( argv[3] ) );
    cannyFilter->SetMaximumError( ::atof( argv[4] ) );
    cannyFilter->SetUpperThreshold( ::atof( argv[5] ) );
    cannyFilter->SetLowerThreshold( ::atof( argv[6] ) );

    rescale->SetInput( cannyFilter->GetOutput() );

    FilterProgress::Pointer  observer = FilterProgress::New();
    rescale->AddObserver( itk::ProgressEvent(), observer );

    rescale->Update();

    // Declare a Writer
    typedef itk::IM0VolumeWriter< OutputImageType >  WriterType;

    WriterType::Pointer      writer =  WriterType::New();

    writer->SetFileName( argv[2] );
    writer->SetInputImage(rescale->GetOutput());
    writer->Execute();

    return 0;
}

