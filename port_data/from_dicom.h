/*
  Copyright 1993-2011, 2016-2017, 2019 Medical Image Processing Group
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

#ifndef __from_dicom_h
#define __from_dicom_h

#if defined (WIN32) || defined (_WIN32)
    #pragma  warning(disable:4996)  //necessary because bill's compiler deprecated stdio.h
#endif

#define BI 0
#define BD 1
#define AN 2
#define AT 3

#define TYPE1	0	/* A B C D => A B C D : ascii straight*/
#define TYPE2	1	/* A B C D => B A D C : ascii straight*/
#define TYPE3	2	/* A B C D => D C B A : ascii straight*/ 
#define TYPE4	3	/* A B C D => C D A B : ascii straight*/
#define TYPE5	4	/* A B C D => A B C D : ascii swapped in 2 byte boundaries */
#define TYPE6	5	/* A B C D => B A D C : ascii swapped in 2 byte boundaries */
#define TYPE7	6	/* A B C D => D C B A : ascii swapped in 2 byte boundaries */
#define TYPE8	7	/* A B C D => C D A B : ascii swapped in 2 byte boundaries */

#define MAX_SERIES 500
#define DUMMY_SUID "00001.001.9973"

typedef struct {
	int roi_number;
	char *roi_name;
	int num_contours;
	unsigned int *contour_num_points;
	float (**contour_coord)[3];
} contour_roi;

typedef struct {
	int num_rois;
	contour_roi *roi;
} contour_struct_set;

typedef int (*pairp)[2];
typedef float (*float3p)[3];


int get_element(FILE *fp, unsigned short group, unsigned short element,
	int type /* type of element being read (BI=16bits, BD=32bits,
	AN=ASCIInumeric, AT=ASCIItext)*/, void *result,
	unsigned int maxlen /* bytes at result */, int *items_read,
	bool skip_sequences, int rec_arch_type);
int extract_floats(char *text, int n, float floats[]);
int get_element(FILE *fp, unsigned short group, unsigned short element,
	int type /* type of element being read (BI=16bits, BD=32bits,
	AN=ASCIInumeric, AT=ASCIItext)*/, void *result,
	unsigned int maxlen /* bytes at result */, int *items_read);
int check_acrnema_file(const char *path, const char *file);
int names_are_similar(const char *name1, const char *name2);
char *get_series_uid(const char dir[], const char filename[]);
float get_time(const char filename[]);
int get_num_frames(const char filename[]);
int get_contour_struct_set(FILE *fp, contour_struct_set *ss);
float *get_OffsetVector(const char filename[], int num_frames);

#endif
