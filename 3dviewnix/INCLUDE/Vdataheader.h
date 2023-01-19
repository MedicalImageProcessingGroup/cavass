#ifndef __Vdataheader_h
#define __Vdataheader_h
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
 
 
 
 
/************************************************************************
 *                                                                      *
 *      File            : 3DVIEWNIX.HDR                                 *
 *      Description     : To be kept in the INCLUDE dir. Global file.   *
 *      Return Value    : None.                                         *
 *      Parameters      : None.                                         *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on February 28, 1989 by Hsiu-Mei Hung *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/

/* 3DVIEWNIX header data structure */


typedef struct {
        char recognition_code[80]; /*recognition code*/
        char study_date[15];    /*study date (the format is yyyy.mm.dd)*/
        char study_time[15];    /*study time (the format is hh:mm:ss)*/
	short data_type;        /*data set type : Scan data: IMAGE0, IMAGE1.
                                  Structure data: CURVE0, SURFACE0, SURFACE1,
                                  SHELL. Display data: MOVIE0.*/
        char modality[10];      /*modality (CT, NM, MR, DS, DR, US, OT)*/
        char institution[80];   /*institution ID*/
        char physician[80];     /*referring physician*/
        char department[80];    /*institutional department*/
        char radiologist[80];   /*radiologist*/
        char model[80];         /*manufacturer's model*/
        char filename[80];      /*name of the file containing this message*/
        char filename1[80];     /*name of the file that created this file*/
        char *description;      /*description of data in file*/
        char *comment;          /*scratch pad to store user-specified info*/
        char patient_name[80];  /*patient name*/
        char patient_id[80];    /*patient ID*/
        float slice_thickness;  /*slice thickness*/
        float kvp[2];           /*KVP*/
        float repetition_time;  /*repetition time*/
        float echo_time;        /*echo time*/
        char *imaged_nucleus;   /*imaged nucleus*/
        float gantry_tilt;      /*gantry tilt*/
        char study[10];         /*study number*/
        char series[10];        /*series number*/
        short gray_descriptor[3]; /*lookup table description - gray
                                (number of entries in the table(net)\starting
                                 pixel value(spv)\num of bits for entries)*/
        short red_descriptor[3]; /*lookup table description - red
                                (as for gray)*/
        short green_descriptor[3]; /*lookup table description - green
                                (as for gray)*/
        short blue_descriptor[3]; /*lookup table description - blue
                                (as for gray)*/
        unsigned short *gray_lookup_data; /*lookup data - gray
                                (mapped value1\mapped value2\...\ mapped value
                                 net; pixel values to be mapped are assumed to
                                 be spv, spv+1,...,spv+net-1; each mapped
                                 value is stored in 16 bits)*/
        unsigned short *red_lookup_data; /*lookup data - red
                                (as for gray)*/
        unsigned short *green_lookup_data; /*lookup data - green
                                (as for gray)*/
        unsigned short *blue_lookup_data; /*lookup data - blue
                                (as for gray)*/
 
        /*the following structure items are one bit for each item and
          indicate whether the corresponding item stated above is valid(1)
          or invalid(0).*/
        unsigned recognition_code_valid: 1;
        unsigned study_date_valid: 1;
        unsigned study_time_valid: 1;
        unsigned data_type_valid: 1;
        unsigned modality_valid: 1;
        unsigned institution_valid: 1;
        unsigned physician_valid: 1;
        unsigned department_valid: 1;
        unsigned radiologist_valid: 1;
        unsigned model_valid: 1;
        unsigned filename_valid: 1;
        unsigned filename1_valid: 1;
        unsigned description_valid: 1;
        unsigned comment_valid: 1;
        unsigned patient_name_valid: 1;
        unsigned patient_id_valid: 1;
        unsigned slice_thickness_valid: 1;
        unsigned kvp_valid: 1;
        unsigned repetition_time_valid: 1;
        unsigned echo_time_valid: 1;
        unsigned imaged_nucleus_valid: 1;
        unsigned gantry_tilt_valid: 1;
        unsigned study_valid: 1;
        unsigned series_valid: 1;
        unsigned gray_descriptor_valid: 1;
        unsigned red_descriptor_valid: 1;
        unsigned green_descriptor_valid: 1;
        unsigned blue_descriptor_valid: 1;
        unsigned gray_lookup_data_valid: 1;
        unsigned red_lookup_data_valid: 1;
        unsigned green_lookup_data_valid: 1;
        unsigned blue_lookup_data_valid: 1;
} GeneralInfo;
 
typedef struct {
        short dimension;        /*scene dimension (sd)*/
        float *domain;          /*location and orientation of the scene domain
                                  with respect to a fixed imaging device
                                  coordinate system : specified by sd+1 vector
                                  X0,...,Xsd. X0: coordinates of the origin of
                                  the scene coordinate system, X1,...,Xsd:
                                  unit vectors along each of its sd principal
                                  directions.*/
        Char30 *axis_label;     /*label associated with each axis*/
        short *measurement_unit;/*unit measurement in the sd direction.
                                  0 - kilometer;1 - meter;2 - cm;3 - mm;
                                  4 - micron;5 - sec;6 - msec;7 - microsec.*/ 
       	short num_of_density_values; /*number of density values associated
                                  with each cell*/
        short *density_measurement_unit; /*Unit of measurement of density val:
                                  0 - No unit;1 - Hounsfield Units;
                                  2 - Relaxation Times;3 - Proton Density.*/
        float *smallest_density_value; /*smallest density value (when density
                                  is vector valued, the smallest of each
                                  component in the scene*/
        float *largest_density_value; /*largest density value (when density
                                  is vector valued, the largest of each
                                  component in the scene*/
        short num_of_integers;  /*density storage scheme: number of integers
                                  in the density vector(ni). (the rest nd-ni
                                  are assumed to be floating point number*/
        short *signed_bits;     /*density storage scheme: each component is
                                  signed or unsigned: sous1\...\sousni, where
                                  each sousi indicates if the ith component
                                  of the density vector is signed(1) or
                                  unsigned(0)*/
        short num_of_bits;      /*density storage scheme: total number of bits
                                  required for the density vector(nbi)*/
        short *bit_fields;      /*density storage scheme:lbit1\rbit1\...\
                                  lbitnd\rbitnd\. Within the bit field of nbi
                                  bits for the density vector, the bits are
                                  numbered 0,1,...,nbi-1 from left to right
                                  and the bit field for each component is
                                  specified by a left-bit (lbit) and a right-
                                  bit(rbit). For every component, thus,
                                  lbit <= rbit. The lbit, rbit pair for a
                                  floating point number indicates if the
                                  number is of single or double percision. The
                                  density vector itself is stored as integer1\
                                  interger2\...\integerni\fp1\...\fp(nd-ni)\
                                  within the nbi bits bit field and the order
                                  of the bit fields is preserved in the file
                                  storing the  message.*/
        short dimension_in_alignment; /*Alignment dimension (ads): null for
                                  IMAGE1 type. For IAMGE0 type, this element
                                  stores the dimenality of the subscene which
                                  is stored in an integer number of unit of
                                  multiple bytes. ads=0 means no alignment.*/
        short bytes_in_alignment; /*Number of bytes in a unit used for
                                  alignment. NULL for IMAGE1 type. nbys=0
                                  means no alignment.*/
        short xysize[2];        /*size of each slice: null(i.e. LENGTH=0) for
                                  IMAGE1 type; for IMAGE0 type this element
                                  stores number of columns\number of rows\ in
                                  the slice*/
        short *num_of_subscenes;/*sampling scheme(number of sunscenes): null
                                  (i.e. LENGTH=0) for IMAGE1 type. For
                                  IMAGE0 type, this element stores, for each
                                  n, 2<n<=sd, the number of each (n-1) -
                                  dimension subscenes of each n-subscene.
                                  These numbers are stored in the order
                                  ns0\ns00\ns01\...\ns000\ns0001\....*/
        float xypixsz[2];       /*pixel size in the x1 and x2 directions:
                                  psizex1\psizex2*/
        float *loc_of_subscenes;/*sampling scheme(location of sunscenes):
                                  null (i.e. LENGTH=0) for IMAGE1 type. For
                                  IMAGE0 type, this element stores, for each
                                  n, 2<n<=sd, the location of each (n-1) -
                                  subscene of each n-subscene in the order
                                  loc00\loc01\...\loc000\loc0001\....*/
        char *description;      /*description of processing operations on this
                                  scene and their parameters:type of procesing
                                  (filtering, segmentation, interpolation,
                                  classification,masking,reslicing,histogram
                                  transforms,ROI), method, parameters of the
                                  method.*/
 
        /*the following structure items are  one bit for each item and
           indicate whether the corresponding item stated above is valid(1)
           or invalid(0).*/
        unsigned dimension_valid: 1;
        unsigned domain_valid: 1;
        unsigned axis_label_valid: 1;
        unsigned measurement_unit_valid: 1;
        unsigned num_of_density_values_valid: 1;
        unsigned density_measurement_unit_valid: 1;
        unsigned smallest_density_value_valid: 1;
        unsigned largest_density_value_valid: 1;
        unsigned num_of_integers_valid: 1;
        unsigned signed_bits_valid: 1;
        unsigned num_of_bits_valid: 1;
        unsigned bit_fields_valid: 1;
        unsigned dimension_in_alignment_valid: 1;
        unsigned bytes_in_alignment_valid: 1;
        unsigned xysize_valid: 1;
        unsigned num_of_subscenes_valid: 1;
        unsigned xypixsz_valid: 1;
        unsigned loc_of_subscenes_valid: 1;
        unsigned description_valid: 1;
} SceneInfo;
 
typedef struct {
        short dimension;        /*structure dimension(std)*/
        short num_of_structures;/*Num of structures in the structure system.*/        float *domain;          /*location and orientation of the structure
                                  coordinate system for each structure with
                                  respect to a imaging device coord system
                                  in units of measurement: specified by std+1
                                  vectors Y0,...,Ystd. Y0: coordinates of the
                                  origin of the structure coordinate system,
                                  X1,...,Xstd: unit vectors along each of its
                                  sd principal directions*/
        Char30 *axis_label;     /*label associated with each axis*/
        short *measurement_unit;/*unit measurement in the std direction.
                                  0 - kilometer, 1 - meter, 2 - cm,
                                  3 - mm, 4 - micron, 5 - sec, 6 - msec,
                                  7 - microsec.*/
        Char30 *scene_file;     /*Name of file that contains the scene that
                                  gave rise to each structure in the
                                  structure system*/
        unsigned int *num_of_TSE;/*number of terminal structure elements*/
        unsigned int *num_of_NTSE; /*num of non-terminal structure elements*/
        short num_of_components_in_TSE; /*number of components in the vector
                                  TSE assigned to each terminal structure
                                  element(nct)*/
        short num_of_components_in_NTSE; /*number of components in the
                                  vector NTSE assigned to each non-terminal
                                  structure element (ncnt)*/
        short *TSE_measurement_unit; /*Unit of measurement for TSE components.
                                  0 - No Unit, 1 - Voxel Units(i.e. so many
                                  voxels)*/
        short *NTSE_measurement_unit; /*Units of measurement for NTSE comp.
                                  0 - No Unit, 1 - Seconds, 2 -  milliseconds.
                                  3 - Years*/
        float *smallest_value;  /*smallest value of each component of TSE
                                  in the structure*/
        float *largest_value;   /*largest value of each component of TSE
                                  in the structure*/
        short num_of_integers_in_TSE; /*storage scheme for terminal structure
                                  elements : number of integers in TSE(nit).
                                  (The rest nct-nit components are floating
                                  point numbers)*/
        short *signed_bits_in_TSE; /*storage scheme for terminal structure
                                  elements : whether each component of TSE is
                                  signed or unsigned : sous1\ sous2\...\
                                  sousnit\, when each sousi indicates whether
                                  the ith component of TSE is signed(1) or
                                  unsigned(0)*/
        short num_of_bits_in_TSE; /*storage scheme for terminal structure
                                  elements : total number of bits required for
                                  TSE(nbit)*/
        short *bit_fields_in_TSE; /*storage scheme for terminal structure
                                  elements : lbit1\rbit1\...\lbitnct\rbitnct\.
                                  Description os similar to that for gr:0029,
                                  el:8090. If lbit > rbit, it means that
                                  component of TSE associated with this field
                                  is not stored.*/
        short num_of_integers_in_NTSE; /*storage scheme for non-terminal
                                  structure elements : number of integers in
                                  NTSE (nint). (The rest ncnt-nint components
                                  are floating point numbers).*/
        short *signed_bits_in_NTSE; /*storage scheme for non-terminal
                                  structure elements : whether each component
                                  of NTSE is signed or unsigned : sous1\sous2\
                                  ...\sousnint\, when each sousi indicates
                                  whether the ith component of NTSE is
                                  signed(1) or unsigned(0).*/
        short num_of_bits_in_NTSE; /*storage scheme for non-terminal
                                  structure elements : total number of bits
                                  required for NTSE(nbint).*/
        short *bit_fields_in_NTSE; /*storage scheme for non-terminal
                                  structure elements : lbit1\rbit1\...\lbitnct
                                  \rbitncnt\. Description is similar to that
                                  for gr:0029, el:8090. If lbit > rbit, it
                                  means that component of TSE associated
                                  with this field is not stored.*/
        short *num_of_samples;  /*sampling schemefor the structure: this
                                  element stores, for each n, 2<n<=std,
                                  the number of samples in each dimension.
                                  These numbers are stored in the order
                                  ns0\ns00\ns01\...\ns000\ns0001\....*/
        float xysize[2];        /*Sample size in the y1 y2 directions:ssizey1\
                                  ssizey2.*/
        float *loc_of_samples;  /*sampling scheme for the structure: this
                                  element stores, for each n, 2<n<=std,
                                  the sample location in the order loc00\
                                  loc01\loc02\...\loc000\loc001\...*/
        short num_of_elements;  /*Number of elements in the parameter vector
                                  associated with the structure system.*/
        short *description_of_element; /*Description of each element in the
                                  parameter vector.*/
        float *parameter_vectors; /*The parameter vectors.*/
        float *min_max_coordinates; /*quantitative information about the
                                  structure : minimum and maximum coordinates
                                  for tructure element (with respect to the
                                  structure coordinate system) : minx1\minx2\
                                  ....\minxstd\maxx1\maxx2\...\maxxstd\.
                                  This element specifies the extent of the
                                  structure within the structure coordinate
                                  system.*/
        float *volume;          /*quant info about the structure : volume*/
        float *surface_area;    /*quant info about the structure : surf area*/       float *rate_of_change_volume; /*quantitative information about the
                                  structure : rate of change of volume*/
        char *description;      /*Description of structure extraction method
                                  and its parameters*/
 
        /*the following structure items are  one bit for each item and
          indicate whether the corresponding item stated above is valid(1)
          or invalid(0).*/
        unsigned dimension_valid: 1;
        unsigned num_of_structures_valid: 1;
        unsigned domain_valid: 1;
        unsigned axis_label_valid: 1;
        unsigned measurement_unit_valid: 1;
        unsigned scene_file_valid: 1;
        unsigned num_of_TSE_valid: 1;
        unsigned num_of_NTSE_valid: 1;
        unsigned num_of_components_in_TSE_valid: 1;
        unsigned num_of_components_in_NTSE_valid: 1;
        unsigned TSE_measurement_unit_valid: 1;
        unsigned NTSE_measurement_unit_valid: 1;
        unsigned smallest_value_valid: 1;
        unsigned largest_value_valid: 1;
        unsigned num_of_integers_in_TSE_valid: 1;
        unsigned signed_bits_in_TSE_valid: 1;
        unsigned num_of_bits_in_TSE_valid: 1;
        unsigned bit_fields_in_TSE_valid: 1;
        unsigned num_of_integers_in_NTSE_valid: 1;
        unsigned signed_bits_in_NTSE_valid: 1;
        unsigned num_of_bits_in_NTSE_valid: 1;
        unsigned bit_fields_in_NTSE_valid: 1;
        unsigned num_of_samples_valid: 1;
        unsigned xysize_valid: 1;
        unsigned loc_of_samples_valid: 1;
        unsigned num_of_elements_valid: 1;
        unsigned description_of_element_valid: 1;
        unsigned parameter_vectors_valid: 1;
        unsigned min_max_coordinates_valid: 1;
        unsigned volume_valid: 1;
        unsigned surface_area_valid: 1;
        unsigned rate_of_change_volume_valid: 1;
        unsigned description_valid: 1;
} StructureInfo;
 
typedef struct {
        short dimension;        /*dimensionality of the display data(ddd)*/
        short measurement_unit[2]; /*unit measurement in the pixel size:
                                  0 - kilo meter, 1 - meter, 2 - cm,
                                  3 - mm, 4 - micron, 5 - sec, 6 - msec,
                                  7 - microsec.*/
        short num_of_elems;     /*intensity storage scheme : num of elements
                                  in the intensity vector associated with the
                                  pixels(neiv)*/
        float *smallest_value;  /*smallest value of each component of the
                                 intensity vector in the display*/
        float *largest_value;   /*largest value of each component of the
                                  intensity vector in the display*/
        short num_of_integers;  /*intensity storage scheme : number of
                                  integer-valued elements in the intensity
                                  vector(niiv). (The rest neiv-niiv elements
                                  are assumed to be floating point numbers)*/
        short *signed_bits;     /*intensity storage scheme : whether each of
                                  integer-valued elements in the intensity
                                  vector is signed or unsigned : sous1\sous2\
                                  ...\sousniiv\, where each sousi indicates
                                  whether the ith component of the intensity
                                  vector is signed(1) or unsigned(0).*/
        short num_of_bits;      /*intensity storage scheme : total number of
                                  bits reqd for the intensity vector(nbiv)*/
        short *bit_fields;      /*intensity storage scheme : lbit1\rbit1\...\
                                  lbitneiv\rbitneiv\. This elem specifies the
                                  bit field for each element of the intensity
                                  vector via a left-bit, right-bit pair of
                                  numbers;*/
        short dimension_in_alignment; /*Alignment dimension (ads): null for
                                  IMAGE1 type. For IAMGE0 type, this element
                                  stores the dimenality of the subscene which
                                  is stored in an integer number of unit of
                                  multiple bytes. ads=0 means no alignment.*/
        short bytes_in_alignment; /*Number of bytes in a unit used for
                                  alignment. NULL for IMAGE1 type. nbys=0
                                    means no alignment.*/
        short num_of_images;    /*total number of images in the display(tni)*/
        short xysize[2];        /*size of the image : num of columns\ num of
                                  rows*/
        float xypixsz[2];       /*pixel size in the z1 and z2 directions.*/
        char *specification_pv; /*specification of the meaning of each
                                  component of PV: meaning1\meaning2\...\
                                  meaning(ddd-2)*/
        short *pv;              /*characteristics of the image : This element
                                  stores the parameter vectors that describes
                                  the characteristics of the image in the
                                  display : PV1\PV2\...\PVtni\ */
        char *description;      /*description of visualization method and its
                                  parameters for each object (e.g. volume
                                  rendering or surface rendering;ray-casting
                                  or projection;shading method; shading
                                  parameters; diffuse parameters, specular
                                  parameters, ambient; antialiasing methods;
                                  color of the object; etc.)*/
 
        /*the following structure items are  one bit for each item and
          indicate whether the corresponding item stated above is valid(1)
          or invalid(0).*/
        unsigned dimension_valid: 1;
        unsigned measurement_unit_valid: 1;
        unsigned num_of_elems_valid: 1;
        unsigned smallest_value_valid: 1;
        unsigned largest_value_valid: 1;
        unsigned num_of_integers_valid: 1;
        unsigned signed_bits_valid: 1;
        unsigned num_of_bits_valid: 1;
        unsigned bit_fields_valid: 1;
        unsigned dimension_in_alignment_valid: 1;
        unsigned bytes_in_alignment_valid: 1;
        unsigned num_of_images_valid: 1;
        unsigned xysize_valid: 1;
        unsigned xypixsz_valid: 1;
        unsigned specification_pv_valid: 1;
        unsigned pv_valid: 1;
        unsigned description_valid: 1;
} DisplayInfo;
 
typedef struct {
        GeneralInfo gen;        /*general header information*/
        SceneInfo scn;          /*scene data header information*/
        StructureInfo str;      /*structure data header information*/
        DisplayInfo dsp;        /*display data header information*/
} ViewnixHeader;

#endif
