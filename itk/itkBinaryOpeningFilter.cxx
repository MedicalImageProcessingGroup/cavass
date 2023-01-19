/*
  Copyright 1993-2010 Medical Image Processing Group
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

#include <itkImage.h>
#include "itkIM0VolumeReader.h"
#include "itkIM0VolumeWriter.h"

#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
//not in my version: #include "itkBinaryMorphologicalOpeningImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkShapedNeighborhoodIterator.h"
//not in my version: #include "itkFlatStructuringElement.h"
#include "FilterProgress.h"

int main ( int argc, char * argv[] )
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << std::endl;
        std::cerr << argv[0] << " inputFile outputFile radius" << std::endl;
        return EXIT_FAILURE;
    }

    // Declare the image
    const unsigned int    Dimension = 3;
    typedef  unsigned char  PixelType;
    typedef itk::Image< PixelType, Dimension >  ImageType;

    ImageType::Pointer inputImage = ImageType::New();

    //Declare the reader
    typedef itk::IM0VolumeReader< PixelType, ImageType  > ImageReaderType;
    ImageReaderType::Pointer  IM0ImageReader  = ImageReaderType::New();

    IM0ImageReader->SetFileName( argv[1] );
    IM0ImageReader->SetInputImage( inputImage );
    IM0ImageReader->Execute();

    //Declare filters
    typedef itk::BinaryBallStructuringElement< PixelType, Dimension >  StructuringElementType;
    typedef itk::BinaryErodeImageFilter<  ImageType, ImageType, StructuringElementType >  ErodeFilterType;
    typedef itk::BinaryDilateImageFilter< ImageType, ImageType, StructuringElementType >  DilateFilterType;

	StructuringElementType::SizeType  ballSize;
	ballSize[0] = atoi( argv[3] );
	ballSize[1] = atoi( argv[3] );
	ballSize[2] = 0;

    StructuringElementType  structuringElement;
    //structuringElement.SetRadius( atoi(argv[3]) );  //size of structuring element
	structuringElement.SetRadius( ballSize );
    structuringElement.CreateStructuringElement();

    ErodeFilterType::Pointer  filter1 = ErodeFilterType::New();
    filter1->SetKernel( structuringElement );
    filter1->SetInput( inputImage );
    filter1->SetErodeValue( 1 );  // foreground object

    DilateFilterType::Pointer  filter2 = DilateFilterType::New();
    filter2->SetKernel( structuringElement );
    filter2->SetInput( filter1->GetOutput() );
    filter2->SetDilateValue( 1 );  // foreground object

    FilterProgress::Pointer  observer = FilterProgress::New();
    filter2->AddObserver( itk::ProgressEvent(), observer );

    filter2->Update();

    // Declare a Writer
    typedef itk::IM0VolumeWriter< ImageType >  WriterType;
    WriterType::Pointer  writer =  WriterType::New();
    writer->SetFileName( argv[2] );
    writer->SetInputImage( filter2->GetOutput() );
    writer->Execute();

    return EXIT_SUCCESS;
}

