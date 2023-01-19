/*
  Copyright 1993-2008, 2018 Medical Image Processing Group
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

#include <iostream>

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkMetaImageIO.h>

#include "itkIM0VolumeWriter.h"

int main(int argc, char ** argv)
{
  if(argc<3)
    {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << " inputFile<mha>  outputFile<IM0> " << std::endl;
    return -1;
    }

  // Declare the image
  const    unsigned int    Dimension = 3;
  typedef itk::Image<int, Dimension >  ImageType;
  ImageType::Pointer image = ImageType::New();


  // Declare a reader
  typedef itk::ImageFileReader< ImageType > VolumeReaderType;
  VolumeReaderType::Pointer reader = VolumeReaderType::New();

  // Set the name of the file to read
  reader->SetFileName( argv[1] );  
  image = reader->GetOutput();

  // See if the file can be read - "try" otherwise program will 
  //   mysteriously exit on failure in the Object factory
  try
    {
    reader->Update();
    }
  catch( ... )
    {
    std::cout << "Problems reading file format" << std::endl;
    return 1;
    }

  // Declare a Writer
  typedef itk::IM0VolumeWriter< ImageType >  WriterType;
  
  WriterType::Pointer      writer =  WriterType::New();

  writer->SetFileName( argv[2] );
  writer->SetInputImage(image);
  writer->Execute();

  return 0;
}

