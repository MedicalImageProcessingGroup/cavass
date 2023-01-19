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

 
 



 
typedef struct _slices {
    int sd;              /* scene dimension */
        int width;                       /* imager width */
        int height;                      /* image height */
        int bits;                        /* image depth in bits (1, 8 or 16) */
    int total_slices;    /* total number of slices in the scene */
    int volumes;         /* number of volumes in the scene */
    int *slices;         /* number of slices in each volume in the scene */
	int max_slices;		 /* maximum #of slices within all volumes */
    float *location4;    /* location along X4 of each volume in the scene */
    float **location3;   /* location along X3 of each slice in the scene */
	int	**slice_index;	 /* index of the slice assuming a flat tree structure */
    float *min_location3,/* smallest location along X3 for each volume */
          *max_location3;/* largest location along X3 for each volume */
    float Min_location3, /* smallest slice location within the entire scene */
          Max_location3; /* largest slice location within the entire scene */
    float Min_spacing3,  /* smallest slice spacing within the entire scene */
          Max_spacing3;  /* largest slice spacing within the entire scene */
    float *min_spacing3,  /* smallest slice spacing within each volume */
          *max_spacing3;  /* largest slice spacing within each volume*/
    float min_spacing4,  /* smallest volume spacing within the entire scene */
          max_spacing4;  /* largest volume spacing within the entire scene */
    float *fov3;         /* field of view for each volume in the scene */
    float Fov3;          /* enclosing field of view (along X3) for the scene */
    int *variable_spacing;/* for each volume -  0=constant slice spacing, 1=variable s.s. */
    int Variable_spacing; /* entire scene  - 0=constant slice spacing, 1=variable s.s. */
 
                                        } SLICES;
 

