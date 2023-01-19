/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkIM0VolumeReader.h,v $
  Language:  C++
  Date:      $Date: 2008/06/30 15:37:11 $
  Version:   $Revision: 1.2 $

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkIM0VolumeReader_h
#define _itkIM0VolumeReader_h

#include <fstream>
#include <string>

#include "itkObjectFactory.h"
#include "itkObject.h"
#include "itkFixedArray.h"


extern "C"{
#include "Viewnix.h"
}


extern "C" int VReadHeader(FILE *fp ,ViewnixHeader *vh ,char group[5], char element[5]);
extern "C" int VSeekData(FILE *fp,long offset);
extern "C" int VReadData(char *data,int size, int items,FILE *fp,int *items_read);

namespace itk
{

/** \class IM0VolumeReader
 * 
 * This component reads in a im0 volume from file.
 * This class is activiated by method Execute().
 *
 * Inputs:
 *  - name of the im0 image file
 *
 * Outputs:
 *  - pointer to output image
 *
 */
template <typename TPixel, typename TImage>
class IM0VolumeReader : public Object {
public:

  /** Standard class typedefs. */
  typedef IM0VolumeReader Self;
  typedef Object Superclass;
  typedef SmartPointer<Self> Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(IM0VolumeReader, Object);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Image Type. */
  typedef TImage ImageType;
  typedef typename ImageType::SizeType SizeType;
  SizeType  mSize;

  itkStaticConstMacro(ImageDimension, unsigned int, TImage::ImageDimension);

  /** File format pixel type. */
  typedef TPixel PixelType;

  /** Spacing type. */
  typedef FixedArray<double,itkGetStaticConstMacro(ImageDimension)> SpacingType;
  
  /** Set the filename. */
  itkSetStringMacro( FileName );

  /** Set the input image. */
  virtual void SetInputImage( ImageType * ptr )
    { m_Image = ptr; }

  /** Activiate this class. */
  void Execute();

  /** Get the output image. */
  /*itkGetObjectMacro( Image, ImageType ); */

protected:
   IM0VolumeReader();
   ~IM0VolumeReader(){};


private:
  IM0VolumeReader( const Self& ); //purposely not implemented
  void operator=( const Self& ); //purposely not implemented

  std::string  m_FileName;
  typename ImageType::Pointer m_Image;

public:

};

} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkIM0VolumeReader.cpp"
#endif

#endif
