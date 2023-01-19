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

/* convert BIM file in 3DVIEWNIX to Meta file in ITK */
#include <iostream>

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkMetaImageIO.h>

#include "itkIM0VolumeReader.h"

int main(int argc, char ** argv)
{
  if(argc<3)
    {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << " inputFile<IM0>  outputFile<mha> " << std::endl;
    return -1;
    }

  // Declare the image
  const    unsigned int    Dimension = 3;
  typedef  unsigned char  PixelType;
  typedef itk::Image<PixelType, Dimension >  ImageType;
  ImageType::Pointer inputImage = ImageType::New();


  //Declare the reader
  typedef itk::IM0VolumeReader< PixelType, ImageType  > ImageReaderType;
  ImageReaderType::Pointer  IM0ImageReader  = ImageReaderType::New();
  
  
  IM0ImageReader->SetFileName(  argv[1] );
  IM0ImageReader->SetInputImage(inputImage);
  IM0ImageReader->Execute();

  // Declare a Writer
  typedef itk::ImageFileWriter< ImageType > VolumeWriterType;
  VolumeWriterType::Pointer writer = VolumeWriterType::New();

  // Add the MetaImage format to the writer
  itk::MetaImageIO::Pointer metaWriter = itk::MetaImageIO::New();
  writer->SetImageIO( metaWriter );

  // Set the filename
  writer->SetFileName( argv[2] );
  // Set the image to write
  writer->SetInput( inputImage );

  //   Update function.   Writers are the exception to that rule
  try 
    { 
    writer->Update(); 
    } 
  catch( itk::ExceptionObject & err ) 
    { 
    std::cout << "ExceptionObject caught !" << std::endl; 
    std::cout << err << std::endl; 
    return -1;
    } 

  return 0;
}

