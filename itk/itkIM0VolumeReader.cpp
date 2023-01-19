/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkIM0VolumeReader.cpp,v $
  Language:  C++
  Date:      $Date: 2009/06/08 17:01:26 $
  Version:   $Revision: 1.4 $

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkIM0VolumeReader_cpp
#define _itkIM0VolumeReader_cpp

#include "itkIM0VolumeReader.h"
#include "itkImageRegionIterator.h"

namespace itk {

template <typename TPixel, typename TImage>
IM0VolumeReader< TPixel, TImage >::IM0VolumeReader ( ) {
  m_FileName = "";
  m_Image = NULL;
}

template <typename TPixel, typename TImage>
void IM0VolumeReader< TPixel, TImage >::Execute ( ) {
    //make sure that the 3dviewnix home env var is set.
    char*  env = getenv( "VIEWNIX_ENV" );
    if (env==NULL) {
        printf( "VIEWNIX_ENV not set! \n" );
#ifdef  WIN32
        printf( "setting VIEWNIX_ENV to C:\\cavass-build\\debug \n" );
        _putenv( "VIEWNIX_ENV=C:\\cavass-build\\debug" );
#else
        printf( "can't continue. \n" );
        exit( 0 );
#endif
    }

  static ViewnixHeader vh_in;
  char group[6],element[6];
  FILE* fp_in;
  int error_code,item;
  unsigned char *data8 = NULL, *data1 = NULL;
  unsigned short *data16 = NULL;
  double*  dataDouble = NULL;
  unsigned char binpix[8]={128,64,32,16,8,4,2,1};
  int prow,pcol,pslice,slice_size,volume_size;

  /*------read file information and data from input IM0/BIM files------*/
  fp_in = fopen(m_FileName.c_str(), "rb");
  error_code = VReadHeader(fp_in, &vh_in, group, element);
  
  pcol = vh_in.scn.xysize[0];
  prow = vh_in.scn.xysize[1];
  slice_size = pcol*prow;
  pslice = vh_in.scn.num_of_subscenes[0];
  volume_size = slice_size*pslice;
  
  VSeekData(fp_in, 0);
  if(vh_in.scn.num_of_bits == 1)
  {
     data1 = (unsigned char*)malloc((slice_size+7)/8*pslice*sizeof(char));
     error_code = VReadData((char*)data1, 1, (slice_size+7)/8*pslice, fp_in,&item);
  }
  else if(vh_in.scn.num_of_bits == 8)
  {
     data8 = (unsigned char*)malloc(volume_size*sizeof(char));
     error_code = VReadData((char*)data8, 1, volume_size, fp_in,&item);
  }
  else if (vh_in.scn.num_of_bits == 16)
  {
     data16 = (unsigned short*)malloc(volume_size*sizeof(short));
     error_code = VReadData((char*)data16, 2, volume_size, fp_in,&item);
  } else if (vh_in.scn.num_of_bits == sizeof(double)*8) {
     dataDouble = (double*)malloc( volume_size * sizeof(double) );
     error_code = VReadData( (char*)dataDouble, sizeof(double)/2,
                             2*volume_size, fp_in, &item );
  } else {
      assert( 0 );
  }
  fclose( fp_in );
  fp_in = NULL;
  /*--------------------end of read file-----------------------*/

  //SizeType size;
  
  mSize[0] = vh_in.scn.xysize[0];
  mSize[1] = vh_in.scn.xysize[1];
  if (ImageDimension == 3)    mSize[2] = vh_in.scn.num_of_subscenes[0];

  double spacing[3];
  spacing[0] = vh_in.scn.xypixsz[0];
  spacing[1] = vh_in.scn.xypixsz[1];
  if (ImageDimension == 3)
    spacing[2] = vh_in.scn.loc_of_subscenes[1] - vh_in.scn.loc_of_subscenes[0];

  typedef typename ImageType::RegionType RegionType;
  RegionType region;
  region.SetSize(mSize);
  /*region.SetIndex(index);*/

  m_Image->SetRegions( region );
  m_Image->SetSpacing(spacing);
  m_Image->Allocate();
  ImageRegionIterator <ImageType> it(m_Image, region);

  PixelType value = 0;
  it.GoToBegin();
  //while(!it.IsAtEnd()) 
  for (int z=0; z<pslice; z++)
      for (int y=0; y<prow; y++)
          for (int x=0; x<pcol; x++) {
             if (vh_in.scn.num_of_bits == 1) {
                 int bintemp = z*((slice_size + 7)/8) + (y*pcol+x)/8;
                 int bintemp1 = (y*pcol+x)%8;
                 if((binpix[bintemp1] & data1[bintemp])!=0)
                     value = 1;
                 else
                     value = 0;
             }
             else if (vh_in.scn.num_of_bits == 8)
                 value = data8[z*slice_size + y*pcol + x];
             else if (vh_in.scn.num_of_bits == 16)
                 value = data16[z*slice_size + y*pcol + x];
             else if (vh_in.scn.num_of_bits == sizeof(double)*8)
                 value = (PixelType) dataDouble[z*slice_size + y*pcol + x];
             it.Set(value);
             ++it;
          }

  if (vh_in.scn.num_of_bits == 1) {
      free( data1 );
      data1 = NULL;
  } else if (vh_in.scn.num_of_bits == 8) {
      free( data8 );
      data8 = NULL;
  } else if (vh_in.scn.num_of_bits == 16) {
      free( data16 );
      data16 = NULL;
  } else if (vh_in.scn.num_of_bits == sizeof(double)*8) {
      free( dataDouble );
      dataDouble = NULL;
  }
}


} // namespace itk

#endif
