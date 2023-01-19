/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkIM0VolumeWriter.h,v $
  Language:  C++
  Date:      $Date: 2008/06/11 20:20:02 $
  Version:   $Revision: 1.2 $

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkIM0VolumeWriter_h
#define _itkIM0VolumeWriter_h

#include <fstream>
#include <string>

#include "itkObjectFactory.h"
#include "itkObject.h"
#include "itkFixedArray.h"

extern "C"{
#include "Viewnix.h"
}

extern "C" int VWriteHeader(FILE *fp ,ViewnixHeader *vh ,char group[5], char element[5]);
extern "C" int VWriteData(char *data,int size,int items,FILE *fp,int *items_written);

namespace itk {

/** \class IM0VolumeWriter
 * 
 * This component writes in a IM0 volume to file.
 * This class is activiated by method Execute().
 *
 * Inputs:
 *  - the input image
 *  - name of the IM0 image file
 *
 */
template <typename TImage>
class  IM0VolumeWriter : public Object {
public:

  /** Standard class typedefs. */
  typedef IM0VolumeWriter Self;
  typedef Object Superclass;
  typedef SmartPointer<Self> Pointer;
  typedef SmartPointer<const Self>  ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(IM0VolumeWriter, Object);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Image Type. */
  typedef TImage ImageType;
  itkStaticConstMacro(ImageDimension, unsigned int, TImage::ImageDimension);

  /** Spacing type. */
  typedef FixedArray<double,itkGetStaticConstMacro(ImageDimension)> SpacingType;

  
  /** File format pixel type. */
  typedef typename ImageType::PixelType PixelType;

  /** Set the input image. */
  virtual void SetInputImage( const ImageType * ptr )
    { m_InputImage = ptr; }
      
  /** Set the filename. */
  itkSetStringMacro( FileName );

  virtual void SetDoAbs ( bool doAbs ) {
      mDoAbs = doAbs;
  }
  virtual void SetOutputDoubleData ( bool outputDoubleData ) {
      mOutputDoubleData = outputDoubleData;
  }

  /** Activiate this class. */
  void Execute();


protected:
   IM0VolumeWriter();
   ~IM0VolumeWriter(){};


private:
  IM0VolumeWriter( const Self& ); //purposely not implemented
  void operator=( const Self& ); //purposely not implemented

  std::string  m_FileName;
  typename ImageType::ConstPointer m_InputImage;
  bool  mDoAbs;
  bool  mOutputDoubleData;
};

} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkIM0VolumeWriter.cpp"
#endif

#endif
