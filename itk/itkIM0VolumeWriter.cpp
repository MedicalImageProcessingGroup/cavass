/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkIM0VolumeWriter.cpp,v $
  Language:  C++
  Date:      $Date: 2008/06/30 15:37:11 $
  Version:   $Revision: 1.3 $

  Copyright (c) 2002 Insight Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef _itkIM0VolumeWriter_cpp
#define _itkIM0VolumeWriter_cpp

#include "itkIM0VolumeWriter.h"
#include "itkStatisticsImageFilter.h"
#include "itkImageRegionConstIterator.h"

namespace itk {

template <typename TImage>
IM0VolumeWriter<TImage>::IM0VolumeWriter ( ) {
  m_FileName = "";
  m_InputImage = NULL;
  mDoAbs = false;
  mOutputDoubleData = false;
}

template <typename TImage>
void IM0VolumeWriter<TImage>::Execute ( ) {
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

    ViewnixHeader vh_out;
    FILE* fp_out;
    float fmin,fmax;
    /*typedef Image<unsigned short, ImageDimension> UShortImageType;*/
    typedef StatisticsImageFilter<ImageType> StatsCalculatorType;
    typename StatsCalculatorType::Pointer statsCalculator = StatsCalculatorType::New();

    /* set values of image region */
    typedef ImageRegionConstIterator <ImageType> Iterator;
    Iterator  ot( m_InputImage, m_InputImage->GetBufferedRegion() );

    typedef typename ImageType::SizeType SizeType;
    SizeType size = m_InputImage->GetBufferedRegion().GetSize();

    /* set up viewnix header */
    memset(&vh_out, 0, sizeof(vh_out));

    strcpy(vh_out.gen.recognition_code, "VIEWNIX1.0");
    vh_out.gen.recognition_code_valid = 1;

    vh_out.gen.data_type = IMAGE0;
    vh_out.gen.data_type_valid = 1;

    vh_out.scn.dimension = 3;
    vh_out.scn.dimension_valid = 1;

    /* set image size */
    vh_out.scn.xysize[0] = size[0];
    vh_out.scn.xysize[1] = size[1];
    vh_out.scn.xysize_valid = 1;

    vh_out.scn.num_of_subscenes = (short*) malloc(1*sizeof(short));
    if (ImageDimension == 3)
        vh_out.scn.num_of_subscenes[0] = size[2];
    else if (ImageDimension == 2)
        vh_out.scn.num_of_subscenes[0] = 1;
    vh_out.scn.num_of_subscenes_valid = 1;

    /* set image spacing */
    SpacingType spacing;
    spacing = m_InputImage->GetSpacing();

    vh_out.scn.xypixsz[0] = spacing[0];
    vh_out.scn.xypixsz[1] = spacing[1];
    vh_out.scn.xypixsz_valid = 1;

    vh_out.scn.loc_of_subscenes = (float*) malloc(vh_out.scn.num_of_subscenes[0]*sizeof(float));
    vh_out.scn.loc_of_subscenes[0] = 0;
    for (int i = 1; i<vh_out.scn.num_of_subscenes[0]; i++)
        vh_out.scn.loc_of_subscenes[i] = vh_out.scn.loc_of_subscenes[i-1] + spacing[2];
    vh_out.scn.loc_of_subscenes_valid = 1;

    vh_out.scn.smallest_density_value = (float *) malloc(sizeof(float));
    vh_out.scn.largest_density_value  = (float *) malloc(sizeof(float));

    statsCalculator->SetInput( m_InputImage );
    statsCalculator->Update();

    fmin = static_cast<float>( statsCalculator->GetMinimum() );
    fmax = static_cast<float>( statsCalculator->GetMaximum() );
    if (mDoAbs) {
        if (statsCalculator->GetMinimum()<0) {
            fmin = 0;
            if (-(statsCalculator->GetMinimum()) > fmax)
                fmax = -(statsCalculator->GetMinimum());
        }
    }
    vh_out.scn.smallest_density_value[0] = fmin;
    vh_out.scn.largest_density_value[0]  = fmax;

    vh_out.scn.smallest_density_value_valid = 1;
    vh_out.scn.largest_density_value_valid  = 1;

    printf( "min=%.2f, max=%.2f \n", fmin, fmax );
    if (mOutputDoubleData) {
        vh_out.scn.num_of_bits = sizeof(double) * 8;
    } else {
        if (fmin>=0 && fmax<=255)
            vh_out.scn.num_of_bits = 8; /* only numbers 8 and 16 are allowed */
        else
            vh_out.scn.num_of_bits = 16;
    }
    vh_out.scn.num_of_bits_valid = 1;

    vh_out.scn.num_of_density_values = 1;
    vh_out.scn.num_of_density_values_valid = 1;

    vh_out.scn.num_of_integers = vh_out.scn.num_of_density_values;
    vh_out.scn.num_of_integers_valid = 1;

    vh_out.scn.bit_fields = (short *)malloc( 2*vh_out.scn.num_of_density_values*sizeof(short) );
    vh_out.scn.bit_fields[0] = vh_out.scn.num_of_bits;
    vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits - 1;
    vh_out.scn.bit_fields_valid = 1; 

    fp_out = fopen( m_FileName.c_str(), "wb" );
    if (fp_out == NULL) {
        std::cerr << "Could not open output file!" << std::endl;
        return;
    }

    char  group[6], element[6];
    strcpy( vh_out.gen.filename, m_FileName.c_str() );
    /* write header information of IM0 file */
    int  error_code = VWriteHeader( fp_out, &vh_out, group, element );

    unsigned char*   out_data8  = NULL;
    unsigned short*  out_data16 = NULL;
    double*          out_double = NULL;

    unsigned long  numberOfPixels = m_InputImage->GetBufferedRegion().GetNumberOfPixels();
    /* allocate memory */
    if (vh_out.scn.num_of_bits==8) {
        out_data8 = (unsigned char*)malloc( numberOfPixels * sizeof(char) );
        assert( out_data8 != NULL );
    } else if (vh_out.scn.num_of_bits==16) {
        out_data16 = (unsigned short*)malloc( numberOfPixels * sizeof(short) );
        assert( out_data16 != NULL );
    } else if (vh_out.scn.num_of_bits == sizeof(double)*8) {
        out_double = (double*)malloc( numberOfPixels * sizeof(double) );
        assert( out_double != NULL );
    } else {
        assert( 0 );
        exit( 0 );
    }

    /* buffer copy */
    int i = 0;
    ot.GoToBegin();
    while (!ot.IsAtEnd()) {
        //std::cout << ot.Get() << " ";
        int otval=ot.Get();
        if (mDoAbs) {
            if (otval<0) {
                if (vh_out.scn.num_of_bits == 8)
                    out_data8[i]  = static_cast<unsigned char>( -otval );
                else if (vh_out.scn.num_of_bits == 16)
                    out_data16[i] = static_cast<unsigned short>( -otval );
                else if (vh_out.scn.num_of_bits == sizeof(double)*8)
                    out_double[i] = static_cast<double>( -otval );
            } else {
                if (vh_out.scn.num_of_bits == 8)
                    out_data8[i]  = static_cast<unsigned char>( otval );
                else if (vh_out.scn.num_of_bits == 16)
                    out_data16[i] = static_cast<unsigned short>( otval );
                else if (vh_out.scn.num_of_bits == sizeof(double)*8)
                    out_double[i] = static_cast<double>( otval );
            }
        } else {
            if (vh_out.scn.signed_bits==0 && otval<0)
                otval = 0;
            if (vh_out.scn.num_of_bits == 8)
                out_data8[i] = static_cast<unsigned char>( otval );
            else if (vh_out.scn.num_of_bits == 16)
                out_data16[i] = static_cast<unsigned short>( otval );
            else if (vh_out.scn.num_of_bits == sizeof(double)*8)
                out_double[i] = static_cast<double>( otval );
        }
        ++ot;
        i++;
    }
    /* write data buffer */
    if (vh_out.scn.num_of_bits == 8) {
        error_code = VWriteData( (char*)out_data8, 1, numberOfPixels, fp_out, &i );
    } else if (vh_out.scn.num_of_bits == 16) {
        error_code = VWriteData( (char*)out_data16, 2, numberOfPixels, fp_out, &i );
    } else if (vh_out.scn.num_of_bits == sizeof(double)*8) {
        error_code = VWriteData( (char*)out_double, sizeof(double)/2, 2*numberOfPixels, fp_out, &i );
    }

    fclose(fp_out);
    if (vh_out.scn.num_of_bits==8) {
        free( out_data8 );
        out_data8 = NULL;
    } else if (vh_out.scn.num_of_bits==16) {
        free( out_data16 );
        out_data16 = NULL;
    } else if (vh_out.scn.num_of_bits==sizeof(double)*8) {
        free( out_double );
        out_double = NULL;
    }
}

} // namespace itk

#endif
