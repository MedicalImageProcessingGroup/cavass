/*
  Copyright 1993-2014 Medical Image Processing Group
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

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <cv3dv.h>
#include "circle.c"
#define MAX_AFFINITY 65000
#define MAX_SCALE 4
#define MIN_SCALE 2
#define SIGMA_CONSTANT 3.0
#define FILTER_CONSTANT 0.1
#define FILTER_WIDTH 2
#define MAX_OPTIMA 100
#define INV_LOG_2 1.44269504088896340737 /* 1/log(2.0) */

void compute_feature_image(int slice),
 compute_feature_scale(int slice);

int CUT_OFF_THR = 1;

double filter_mask[2*FILTER_WIDTH+1][2*FILTER_WIDTH+1];

#define Get_max(a, b) ((a)>(b)? (a): (b))
#define Get_min(a, b) ((a)<(b)? (a): (b))
#define Square(a) ((a)*(a))

#define Pimage(slice, row, column) (pimage_8? pimage_8[slice][row][column]: \
	pimage_16[slice][row][column])


FileInfo              *file_info;
ViewnixHeader          vh;
int                    smallest_density_value, largest_density_value;
double                 sigma_g, sigma_s;
unsigned short      ***pimage_16;
unsigned char       ***pimage_8;
int                    pslice, prow, pcol;
unsigned char        *feature_scale, **pfeature_scale;

unsigned short       **pfeature_data_across, **pfeature_data_down,
                      *int_homogeneity_data, ***pint_homogeneity_data;
unsigned short        *feature_data_across, *feature_data_down;
unsigned short        *data_16;
double                *transformation_intensity, *transformation_scale;
double                *total_energy, *uncertainty;
double                *histogram, mean_difference, sigma_difference;
double                 mean_density1, mean_density2, sigma_density1, 
                       sigma_density2;
double                 inv_sigma_density1, inv_sigma_density2,
                       double_homogeneity_histogram[MAX_AFFINITY+1];
double                 count1, count2;
double                 histogram_count;
int                    min_threshold, max_threshold, step=1, slice_size, size;
long                   homogeneity_histogram[MAX_AFFINITY+1];
unsigned short       **ptemp_16, *temp_data_16;

/* Modified: 10/29/01 local optima found by Dewey Odhner. */
/* Modified: 11/6/01 speedup done by Dewey Odhner. */
/* Modified: 7/8/02 roundoff/domain error patched by Dewey Odhner. */
int main (argc, argv)
     int argc;
     char *argv[];
{
  int                  i, j, tti1, tti2, bytes, error, k;
  FILE                *in;
  char                 group[6], elem[6];
  int                  thr, slice, row, col;
  double               inv_sigma_g, inv_sigma_s;
  double               tt1, tt2, tt3, tt4,
					  *total_homogeneity;
  int                  noptima=0, local_optimum_threshold[MAX_OPTIMA],
                       specified_optima=MAX_OPTIMA, local_optimum;


#define Handle_error(message) \
{ \
  fprintf(stderr,message); \
  exit(-1); \
}



  if (argc != 5)  {
    fprintf (stderr, "Usage: %s image_file min max step\n", argv[0]);
    exit (-1);
    }
    

  in = fopen (argv[1], "rb");
  if (in == NULL)  {
    printf ("Couldn't open the file: %s\n", argv[1]);
    exit (-1);
    }
  error = VReadHeader (in, &vh, group, elem);
  if (error > 0 && error <= 104)
    Handle_error ("Fatal error in reading header\n");

  if (vh.gen.data_type != IMAGE0)
    Handle_error ("This is not an IMAGE0 file\n");

  bytes = vh.scn.num_of_bits / 8;
  if (bytes != 2 && bytes != 1)
    Handle_error ("This program only handles 8- & 16-bit images\n");
  smallest_density_value = vh.scn.smallest_density_value_valid ?
    (int)vh.scn.smallest_density_value[0] : 0;
  largest_density_value = vh.scn.largest_density_value_valid ?
    (int)vh.scn.largest_density_value[0] : (1 << vh.scn.num_of_bits) - 1;
  pcol = vh.scn.xysize[0];
  prow = vh.scn.xysize[1];
  pslice = vh.scn.num_of_subscenes[0];
  slice_size = prow * pcol;
  size = slice_size * pslice;
 
/****************************************************/
  if (bytes == 1)
  {
    pimage_8 = (unsigned char ***)
                malloc (pslice * sizeof (unsigned char **));
    if (pimage_8 == NULL)  {
      printf ("Couldn't allocate memory (execution terminated)\n");
      fflush (stdout);
      exit (-1);
    }
    pimage_8[0] = (unsigned char **)
                   malloc (pslice * prow * sizeof (unsigned char *));
    if (pimage_8[0] == NULL) {
      printf ("Couldn't allocate memory (execution terminated)\n");
      fflush (stdout);
      exit (-1);
    }
    for (slice = 0; slice < pslice; slice++)
      pimage_8[slice] = pimage_8[0] + slice * prow;
    data_16 = (unsigned short *) malloc (size);
    if (data_16 == NULL)  {
      printf ("Couldn't allocate memory (execution terminated)\n");
      fflush (stdout);
      exit (-1);
    }
    for (slice = 0; slice < pslice; slice++)
      for (row = 0; row < prow; row++)
        pimage_8[slice][row] =
		  (unsigned char *)data_16 + slice * slice_size + row * pcol;
  }
  else
  {
    pimage_16 = (unsigned short ***)
                malloc (pslice * sizeof (unsigned short **));
    if (pimage_16 == NULL)  {
      printf ("Couldn't allocate memory (execution terminated)\n");
      fflush (stdout);
      exit (-1);
    }
    pimage_16[0] = (unsigned short **)
                   malloc (pslice * prow * sizeof (unsigned short *));
    if (pimage_16[0] == NULL) {
      printf ("Couldn't allocate memory (execution terminated)\n");
      fflush (stdout);
      exit (-1);
    }
    for (slice = 0; slice < pslice; slice++)
      pimage_16[slice] = pimage_16[0] + slice * prow;
    data_16 = (unsigned short *) malloc (size * sizeof(unsigned short));
    if (data_16 == NULL)  {
      printf ("Couldn't allocate memory (execution terminated)\n");
      fflush (stdout);
      exit (-1);
    }
    for (slice = 0; slice < pslice; slice++)
      for (row = 0; row < prow; row++)
        pimage_16[slice][row] = data_16 + slice * slice_size + row * pcol;
  }
/****************************************************/

  VSeekData (in, 0);
  VReadData ((char *)data_16, bytes, size, in, &j);
  
  largest_density_value = 0;
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)
        if (((int) Pimage(slice, row, col)) > largest_density_value) 
          largest_density_value = (int) Pimage(slice, row, col);

	  
  smallest_density_value = largest_density_value;
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)
        if (((int) Pimage(slice, row, col)) > CUT_OFF_THR
		     && ((int) Pimage(slice, row, col)) < smallest_density_value) 
          smallest_density_value = (int) Pimage(slice, row, col);

/*******COMPUATTION OF THRESHOLD RANGE*******************/
  histogram = (double *) malloc ((largest_density_value + 1) *
                                                  sizeof (double));
  if (histogram == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  for (i = 0; i <= largest_density_value; i++)
    histogram[i] = 0.0;
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol - 1; col++)  
        if((tti1=(int) Pimage(slice, row, col)) > CUT_OFF_THR) {
		  histogram[tti1] = histogram[tti1] + 1;
        }
  tt3 = 0;
  for (i = CUT_OFF_THR; i <= largest_density_value; i++)
    tt3 = tt3 + histogram[i];

  sscanf(argv[2], "%lf", &tt1);
  if (tt1 == (int)tt1)
    min_threshold = (int)ceil(tt1);
  else {
    tt4 = 0;
    for (i = largest_density_value; i > CUT_OFF_THR; i--)  {
      tt4=tt4+histogram[i];
      if( tt4/tt3 > tt1)  {
	    min_threshold = i;
	    i = -1;
	  }
    }
    printf("Minimum Threshold: %d\n", min_threshold);
  }

  sscanf(argv[3], "%lf", &tt2);
  if (tt2 > 1)
    max_threshold = (int)tt2;
  else {
    tt4 = 0;
    for (i = largest_density_value; i > CUT_OFF_THR; i--)  {
      tt4=tt4+histogram[i];
      if( tt4/tt3 > tt2)  {
	    max_threshold = i;
	    i = -1;
	  }
    }
    printf("Maximum Threshold: %d\n", max_threshold);
  }
 
/*******PARAMETER COMPUTATION*******************/
  for (i = 0; i <= largest_density_value; i++)
    histogram[i] = 0.0;
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol - 1; col++)  
        if((int) Pimage(slice, row, col) > CUT_OFF_THR &&
           (int) Pimage(slice, row, col+1) > CUT_OFF_THR &&
		   ((int) Pimage(slice, row, col) < max_threshold ||
           (int) Pimage(slice, row, col+1) < max_threshold)) {
          tti1 = (int) Pimage(slice, row, col)
                     - (int) Pimage(slice, row, col+1);
          if (tti1 < 0)
            tti1 = -tti1;
          histogram[tti1] = histogram[tti1] + 1;
        }
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow - 1; row++)
      for (col = 0; col < pcol; col++)  
        if((int) Pimage(slice, row, col) > CUT_OFF_THR &&
           (int) Pimage(slice, row+1, col) > CUT_OFF_THR &&
		   ((int) Pimage(slice, row, col) < max_threshold ||
           (int) Pimage(slice, row+1, col) < max_threshold)) {
          tti1 = (int) Pimage(slice, row, col)
                     - (int) Pimage(slice, row+1, col);
          if (tti1 < 0)
            tti1 = -tti1;
          histogram[tti1] = histogram[tti1] + 1;
        }
  mean_difference = 0.0;
  histogram_count = 0;
  for (i = 0; i <= largest_density_value; i++)  {
    mean_difference = mean_difference + i * histogram[i];
    histogram_count = histogram_count + histogram[i];
    }
  mean_difference = mean_difference / histogram_count;
  sigma_difference = 0.0;
  for (i = 0; i <= largest_density_value; i++)  {
    sigma_difference = sigma_difference +
                       Square(i - mean_difference) * histogram[i];
    }
  sigma_difference = sqrt (sigma_difference / histogram_count);

  transformation_intensity = (double *) malloc ((largest_density_value + 1) *
                                                              sizeof (double));
  if (transformation_intensity == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  transformation_scale = (double *) malloc ((largest_density_value + 1) *
                                                              sizeof (double));
  if (transformation_scale == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  inv_sigma_g = -0.5 / Square(mean_difference + SIGMA_CONSTANT 
                                          * sigma_difference);
  inv_sigma_s = -0.5 / Square(2.0 * (mean_difference + 
                                   SIGMA_CONSTANT * sigma_difference));

  for (i = 0; i <= largest_density_value; i++)  {
    transformation_intensity[i] = exp (inv_sigma_g * Square((double) i));
    transformation_scale[i] = exp (inv_sigma_s * Square((double) i));
    }

  total_homogeneity = (double *) calloc ((largest_density_value + 1),
                                                              sizeof (double));
  if (total_homogeneity == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }

    /***********************************************/
  assert(MAX_SCALE*sizeof(circle_no_points[0]) <= sizeof(circle_no_points));
  assert(MAX_SCALE*sizeof(circle_points[0]) <= sizeof(circle_points));
  for (k = i = 0; i < MAX_SCALE; i++)  {
    assert(circle_no_points[i]);
    circle_points[i] = circle_points_data+k;
	k += circle_no_points[i];
  }

/****************************************************/
  pfeature_data_across = (unsigned short **) malloc (pslice * prow * 
                                                   sizeof (unsigned short *));
  if (pfeature_data_across == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  feature_data_across = (unsigned short *) 
                                  malloc (slice_size * sizeof(unsigned short));
  if (feature_data_across == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  for (row = 0; row < prow; row++)
    pfeature_data_across[row] = feature_data_across + row * pcol;

/****************************************************/
  pfeature_data_down = (unsigned short **)
                           malloc (prow * sizeof (unsigned short *));
  if (pfeature_data_down == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  feature_data_down = (unsigned short *) 
                                  malloc (slice_size * sizeof(unsigned short));
  if (feature_data_down == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  for (row = 0; row < prow; row++)
    pfeature_data_down[row] = feature_data_down + row * pcol;

/****************************************************/
  pfeature_scale = (unsigned char **)
    malloc (prow * sizeof (unsigned char *));
  if (pfeature_scale == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  feature_scale = (unsigned char *) malloc (slice_size);
  if (feature_scale == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  for (row = 0; row < prow; row++)
    pfeature_scale[row] = feature_scale + row * pcol;

/****************************************************/
  pint_homogeneity_data = (unsigned short ***) 
                             malloc (pslice * sizeof (unsigned short **));
  if (pint_homogeneity_data == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  pint_homogeneity_data[0] = (unsigned short **) malloc (pslice * prow 
                                                * sizeof (unsigned short *));
  if (pint_homogeneity_data[0] == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  for (slice = 0; slice < pslice; slice++)
    pint_homogeneity_data[slice] = pint_homogeneity_data[0] + slice * prow;
  int_homogeneity_data = (unsigned short *) malloc (size 
                                             * sizeof (unsigned short));
  if (int_homogeneity_data == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
    }
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      pint_homogeneity_data[slice][row] = int_homogeneity_data + 
                                             slice * slice_size + row * pcol;

  sigma_s = 13.0;

    /***************FILTER*******************************************/
  ptemp_16 = (unsigned short **)
                   malloc (prow * sizeof (unsigned short *));
  if (ptemp_16 == NULL) {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
  }
  temp_data_16= (unsigned short *) malloc(slice_size * sizeof(unsigned short));
  if (temp_data_16 == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
  }
  for (row = 0; row < prow; row++)
     ptemp_16[row] = temp_data_16 + row * pcol;

  for (i = -FILTER_WIDTH; i <= FILTER_WIDTH; i++)
    for (j = -FILTER_WIDTH; j <= FILTER_WIDTH; j++) 
      filter_mask[i+FILTER_WIDTH][j+FILTER_WIDTH] =  1.0 / (1.0 
                      + FILTER_CONSTANT * sqrt ((double)i*i+j*j));

  for (i = 0; i <= largest_density_value; i++)
    histogram[i] = 0.0;
  for(slice = 0; slice < pslice; slice++) {
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++) {
        ptemp_16[row][col] = Pimage(slice, row, col);
        tti1 = (int) ptemp_16[row][col];
		if(tti1 > CUT_OFF_THR)
          histogram[tti1] = histogram[tti1] + 1;
	  }
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)  {
        tt1 = 0.0;
        tt2 = 0.0;
        for (i = 0; i <= 2*FILTER_WIDTH; i++)
          for (j = 0; j <= 2*FILTER_WIDTH; j++)  {
            if (row + i >= 0 && row + i < prow && col + j >= 0
                && col + j < pcol 
                && ptemp_16[row + i][col + j] > CUT_OFF_THR)  {
              tt3 =  filter_mask[i][j];
              tt2 = tt2 + tt3 * ptemp_16[row + i][col + j];
              tt1 = tt1 + tt3;
            }
          }
          if (pimage_8)
		    pimage_8[slice][row][col] = (unsigned char) (tt2 / tt1);
		  else
		    pimage_16[slice][row][col] = (unsigned short) (tt2 / tt1);
      }
  }
  free(ptemp_16);
  free(temp_data_16);

/****************************************************************/


/**********HOMOGENEITY COMPUTATION FROM AFFINITY***************/

  for (slice = 0; slice < pslice; slice++) {
    compute_feature_image(slice);
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)  {
	   if((int) Pimage(slice, row, col) > CUT_OFF_THR)  {
	     if((int) Pimage(slice, row, col) < max_threshold)  {
           pint_homogeneity_data[slice][row][col] = MAX_AFFINITY;
           if (row != 0)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_down[row - 1][col]
			     && Pimage(slice, row-1, col) > CUT_OFF_THR)
               pint_homogeneity_data[slice][row][col] = 
                                    pfeature_data_down[row - 1][col];
           }
           if (row != prow - 1)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_down[row][col]
			     && Pimage(slice, row+1, col) > CUT_OFF_THR)
               pint_homogeneity_data[slice][row][col] = 
                                        pfeature_data_down[row][col];
           }
           if (col != 0)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_across[row][col - 1]
			     && Pimage(slice, row, col-1) > CUT_OFF_THR)
               pint_homogeneity_data[slice][row][col] = 
                                  pfeature_data_across[row][col - 1];
           }
           if (col != pcol - 1)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_across[row][col]
			     && Pimage(slice, row, col+1) > CUT_OFF_THR)
               pint_homogeneity_data[slice][row][col] = 
                                      pfeature_data_across[row][col];
           }
         }
	     else  {
           pint_homogeneity_data[slice][row][col] = MAX_AFFINITY;
           if (row != 0)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_down[row - 1][col]
			     && Pimage(slice, row-1, col) > CUT_OFF_THR
			     && Pimage(slice, row-1, col) < max_threshold)
               pint_homogeneity_data[slice][row][col] = 
                                    pfeature_data_down[row - 1][col];
           }
           if (row != prow - 1)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_down[row][col]
			     && Pimage(slice, row+1, col) > CUT_OFF_THR
			     && Pimage(slice, row+1, col) < max_threshold)
               pint_homogeneity_data[slice][row][col] = 
                                        pfeature_data_down[row][col];
           }
           if (col != 0)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_across[row][col - 1]
			     && Pimage(slice, row, col-1) > CUT_OFF_THR
			     && Pimage(slice, row, col-1) < max_threshold)
               pint_homogeneity_data[slice][row][col] = 
                                  pfeature_data_across[row][col - 1];
           }
           if (col != pcol - 1)  {
             if (pint_homogeneity_data[slice][row][col] >
                    pfeature_data_across[row][col]
			     && Pimage(slice, row, col+1) > CUT_OFF_THR
			     && Pimage(slice, row, col+1) < max_threshold)
               pint_homogeneity_data[slice][row][col] = 
                                      pfeature_data_across[row][col];
           }
         }
	   }
	   else 
         pint_homogeneity_data[slice][row][col] = MAX_AFFINITY;
       tti2 = (int) Pimage(slice, row, col);
       tti1 = (int) pint_homogeneity_data[slice][row][col];
       if(tti2 >= min_threshold && tti2 <= max_threshold)
         homogeneity_histogram[tti1] = homogeneity_histogram[tti1] + 1;
	  }
  }
  free (feature_data_down);
  free (pfeature_data_down);
  free (feature_data_across);
  free (pfeature_data_across);
    /*****BEGIN NORMALIZATION USING CUMULATIVE DISTRIBUTION****/

/************************************************************/
  for (i = 2; i < MAX_AFFINITY; i++)
    homogeneity_histogram[i] += homogeneity_histogram[i - 1];
  double_homogeneity_histogram[0] = 0;
  for (i = 1; i < MAX_AFFINITY; i++)
    double_homogeneity_histogram[i] = (double) homogeneity_histogram[i] / 
                                (double) homogeneity_histogram[MAX_AFFINITY-1];
  double_homogeneity_histogram[MAX_AFFINITY] = 1;

  for(slice = 0; slice < pslice; slice++) 
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)  {
        tti1 = (int) pint_homogeneity_data[slice][row][col];
        tti2 = (int) Pimage(slice, row, col);
        if(tti2 >= min_threshold && tti2 <= max_threshold)
          total_homogeneity[tti2] += double_homogeneity_histogram[tti1];
        else 
          total_homogeneity[tti2] += 1.0;
      }
    /*****END NORMALIZATION USING CUMULATIVE DISTRIBUTION****/
  free(int_homogeneity_data);
  free(pint_homogeneity_data[0]);
  free(pint_homogeneity_data);


  for (i = max_threshold+1; i <= largest_density_value; i++)  {
      histogram[max_threshold] = histogram[max_threshold]+histogram[i];
	  histogram[i] = 0;
  }
/***************************************************/	  
	  
	  
  uncertainty = (double *) malloc ((largest_density_value + 1) *
                   sizeof (double));
  if (uncertainty == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
  }
  if (min_threshold <= smallest_density_value)
    min_threshold = smallest_density_value + 1;
  if (max_threshold >= largest_density_value)
    max_threshold = largest_density_value - 1;

  total_energy = (double *) malloc ((max_threshold + 1) * sizeof (double));
  if (total_energy == NULL)  {
    printf ("Couldn't allocate memory (execution terminated)\n");
    fflush (stdout);
    exit (-1);
  }

  step = atoi(argv[4]);

/***************************************************************************/  
  for (local_optimum = thr = min_threshold; thr <= max_threshold;
      thr = thr + step)  {
    mean_density1 = 0.0;
    mean_density2 = 0.0;
    sigma_density1 = 0.0;
    sigma_density2 = 0.0;
    count1 = 0;
    count2 = 0;
    for (i = smallest_density_value; i < thr; i++)  {
      mean_density1 = mean_density1 + i * histogram[i];
      count1 = count1 + histogram[i];
    }
    mean_density1 = mean_density1 / count1;
    for (i = smallest_density_value; i < thr; i++)
      sigma_density1 =
                 sigma_density1 + Square(mean_density1 - i) * histogram[i];
    sigma_density1 = sqrt (sigma_density1 / count1);

    for (i = thr; i <= largest_density_value; i++)  {
        mean_density2 = mean_density2 + i * histogram[i];
        count2 = count2 + histogram[i];
    }
    mean_density2 = mean_density2 / count2;
    for (i = thr; i <= largest_density_value; i++)
        sigma_density2 =
                 sigma_density2 + Square(mean_density2 - i) * histogram[i];
    sigma_density2 = sqrt (sigma_density2 / count2);
    if (sigma_density1 <= 0.00001)
          sigma_density1 = 0.00001;
    if (sigma_density2 <= 0.00001)
          sigma_density2 = 0.00001;
    inv_sigma_density1 = -0.5 / Square(sigma_density1);
    inv_sigma_density2 = -0.5 / Square(sigma_density2);
    for (i = smallest_density_value; i <= largest_density_value; i++)  {
          if (i < mean_density1)
            tt1 = count1 / sigma_density1;
          else
            tt1 = (count1 * exp (inv_sigma_density1 *  (i - mean_density1) 
                                     * (i - mean_density1))) / sigma_density1;
          if (i > mean_density2)
            tt2 = count2 / sigma_density2;
          else
            tt2 = (count2 * exp (inv_sigma_density2 * (i - mean_density2) 
                                     * (i - mean_density2))) / sigma_density2;
          if (tt1 <= 0.00001)
            tt1 = 0.00001;
          if (tt2 <= 0.00001)
            tt2 = 0.00001;
          tt1 = tt2 / (tt1 + tt2);
          uncertainty[i] = tt1==1? 0: (-tt1 * log (tt1) - (1 - tt1) 
                                      * log (1 - tt1)) * INV_LOG_2;
          assert(uncertainty[i] <= 1 && uncertainty[i] >= 0);
    }
    total_energy[thr] = 0.0;

    for (i = smallest_density_value; i <= largest_density_value; i++)
      total_energy[thr]+= (2*total_homogeneity[i]-histogram[i])*uncertainty[i];

	if (thr>min_threshold+step && total_energy[thr]<total_energy[thr-step] &&
	      total_energy[local_optimum]<total_energy[thr-step])
	{
	    for (j=0; j<noptima; j++)
		  if (j==specified_optima || total_energy[local_optimum]<
		      total_energy[local_optimum_threshold[j]])
		    break;
		for (k=specified_optima-1; k>j; k--)
		  local_optimum_threshold[k] = local_optimum_threshold[k-1];
		if (j < specified_optima)
		{
		  local_optimum_threshold[j] = local_optimum;
		  if (noptima < specified_optima)
		    noptima++;
		}
		local_optimum = thr;
	}
	else if (total_energy[thr] < total_energy[local_optimum])
	    local_optimum = thr;
  }
  for (j=0; j<noptima; j++)
    if (j==specified_optima || total_energy[local_optimum]<
        total_energy[local_optimum_threshold[j]])
      break;
  for (k=specified_optima-1; k>j; k--)
    local_optimum_threshold[k] = local_optimum_threshold[k-1];
  if (j < specified_optima)
  {
    local_optimum_threshold[j] = local_optimum;
    if (noptima < specified_optima)
      noptima++;
  }

  for (j=0; j<noptima; j++)
  {
    k = local_optimum_threshold[j]<min_threshold+step ||
	  local_optimum_threshold[j]>max_threshold-step
	  ? local_optimum_threshold[j]
	  : (int)rint(local_optimum_threshold[j]+.5*(
	    total_energy[local_optimum_threshold[j]-step]-
	    total_energy[local_optimum_threshold[j]+step])*step/(
	    total_energy[local_optimum_threshold[j]-step]+
	    total_energy[local_optimum_threshold[j]+step]-
	    2*total_energy[local_optimum_threshold[j]]));
	printf ("%d\n", k);
  }
  exit(0);
}

/*****************************************************************************
 * FUNCTION: compute_feature_image
 * DESCRIPTION: Computes the affinity values for the current slice and
 *    stores them at feature_data_across[] and feature_data_down[].
 * PARAMETERS:
 *    slice: the current slice
 * SIDE EFFECTS: Computes scale representation of the image and stores it
 *    at pfeature_scale.  An error message may be displayed.
 * ENTRY CONDITIONS: The variables vh, windows_open,
 *    pimage_16, pslice, prow, pcol, min_threshold, max_threshold, sigma_g,
 *    pimage_8, transformation_scale must be properly set.
 *    Memory must be allocated at pfeature_scale.
 *    The input file must not change from one call to another.
 *    If feature_data_across or feature_data_down is non-null,sufficient space
 *    must be allocated there.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 9/5/97  by Punam K. Saha
 *    Modified: 10/25/01 8-bit input handled by Dewey Odhner
 *    Modified: 10/31/01 speedup done by Dewey Odhner
 *    Modified: 11/2/01 single slice done by Dewey Odhner
 *
 *****************************************************************************/
void compute_feature_image (int slice)
{
  int                  row, col, i, k, tti1, iscale, xx, yy, 
                       x1, x2, y1, y2, x, y;
  unsigned short     **outptr, **pin_16;
  unsigned char      **pin_8, **pscale;
  double               tt1, count_pos, count_neg, sum_pos, sum_neg, 
                       temp_sum_pos,temp_sum_neg, invk[MAX_SCALE+1][MAX_SCALE];


  compute_feature_scale (slice);
  pscale = pfeature_scale;

  for (i=MIN_SCALE; i<=MAX_SCALE; i++)
    for (k=0; k<i; k++)
	  invk[i][k] = exp(-2.0*k*k/(i*i));

  outptr = pfeature_data_across;

    if (pimage_8)
      pin_8 = pimage_8[slice];
    else
      pin_16 = pimage_16[slice];
    for (row = 0; row < prow; row++)  {
      for (col = 0; col < pcol - 1; col++)    
        if (pimage_8? (pin_8[row][col] >= min_threshold
            && pin_8[row][col] <= max_threshold)
            || (pin_8[row][col+1] >= min_threshold
                && pin_8[row][col+1] <= max_threshold):
			(pin_16[row][col] >= min_threshold
             && pin_16[row][col] <= max_threshold)
            || (pin_16[row][col+1] >= min_threshold 
                && pin_16[row][col+1] <= max_threshold)) {
        sum_pos = 0.0;
        sum_neg = 0.0;
        count_pos = 0.00001;
        count_neg = 0.00001;
        iscale = Get_min ((int) pscale[row][col], 
                          (int) pscale[row][col + 1]);
        for (k = 0; k < iscale; k++)  {
          temp_sum_pos = 0.0;
          temp_sum_neg = 0.0;

          for (i = 0; i < circle_no_points[k]; i++)  {
            xx = circle_points[k][i][0];
            yy = circle_points[k][i][1];
            x = row + xx;
            y1 = col + yy;
            y2 = y1 + 1;
            if (x >= 0 && x < prow && y1 >= 0 && y2 < pcol)  {
              tti1 = pimage_8? (int) pin_8[x][y1] - (int) pin_8[x][y2]:
			    (int) pin_16[x][y1] - (int) pin_16[x][y2];
              if (tti1 < 0)  {
                tti1 = -tti1;
                temp_sum_neg = temp_sum_neg + 
                                         1 - transformation_intensity[tti1];
                count_neg = count_neg + invk[iscale][k];
              }
              else  {
                temp_sum_pos = temp_sum_pos + 
                                         1 - transformation_intensity[tti1];
                count_pos = count_pos + invk[iscale][k];
              }
            }
          }
          sum_neg = sum_neg + temp_sum_neg * invk[iscale][k];
          sum_pos = sum_pos + temp_sum_pos * invk[iscale][k];
        }
        if (sum_pos > sum_neg)
          tt1 = 1 - (sum_pos - sum_neg) / (count_pos + count_neg);
        else
          tt1 = 1 - (sum_neg - sum_pos) / (count_pos + count_neg);
        outptr[row][col] = (unsigned short) (MAX_AFFINITY * tt1);
      }
      else 
        outptr[row][col] = MAX_AFFINITY;
      outptr[row][col] = 0;
    }

  outptr = pfeature_data_down;

    if (pimage_8)
      pin_8 = pimage_8[slice];
    else
      pin_16 = pimage_16[slice];
    for (col = 0; col < pcol; col++)  {
      for (row = 0; row < prow - 1; row++)    
        if (pimage_8? (pin_8[row][col] >= min_threshold
            && pin_8[row][col] <= max_threshold)
            || (pin_8[row+1][col] >= min_threshold
                && pin_8[row+1][col] <= max_threshold):
			(pin_16[row][col] >= min_threshold
             && pin_16[row][col] <= max_threshold)
            || (pin_16[row+1][col] >= min_threshold
                && pin_16[row+1][col] <= max_threshold)) {
        sum_pos = 0.0;
        sum_neg = 0.0;
        count_pos = 0.00001;
        count_neg = 0.00001;
        iscale = Get_min ((int) pscale[row][col], 
                          (int) pscale[row + 1][col]);
        for (k = 0; k < iscale; k++)  {
          temp_sum_pos = 0.0;
          temp_sum_neg = 0.0;

          for (i = 0; i < circle_no_points[k]; i++)  {
            xx = circle_points[k][i][0];
            yy = circle_points[k][i][1];
            x1 = row + xx;
            x2 = x1 + 1;
            y = col + yy;
            if (x1 >= 0 && x2 < prow && y >= 0 && y < pcol)  {
              tti1 = pimage_8? (int) pin_8[x1][y] - (int) pin_8[x2][y]:
			    (int) pin_16[x1][y] - (int) pin_16[x2][y];
              if (tti1 < 0)  {
                tti1 = -tti1;
                temp_sum_neg = temp_sum_neg + 
                                         1 - transformation_intensity[tti1];
                count_neg = count_neg + invk[iscale][k];
              }
              else  {
                temp_sum_pos = temp_sum_pos + 
                                         1 - transformation_intensity[tti1];
                count_pos = count_pos + invk[iscale][k];
              }
            }
          }
          sum_neg = sum_neg + temp_sum_neg * invk[iscale][k];
          sum_pos = sum_pos + temp_sum_pos * invk[iscale][k];
        }
        if (sum_pos > sum_neg)
          tt1 = 1 - (sum_pos - sum_neg) / (count_pos + count_neg);
        else
          tt1 = 1 - (sum_neg - sum_pos) / (count_pos + count_neg);
        outptr[row][col] = (unsigned short) (MAX_AFFINITY * tt1);
      }
      else 
        outptr[row][col] = MAX_AFFINITY;
      outptr[row][col] = 0;
    }
}

/*****************************************************************************
 * FUNCTION: compute_feature_scale
 * DESCRIPTION: Computes scale representation of the image and stores it
 *    at pfeature_scale.
 * PARAMETERS:
 *    slice: the current slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 *    pimage_16, pslice, prow, pcol, min_threshold, max_threshold, sigma_g,
 *    pimage_8, transformation_scale must be set.
 *    Memory must be allocated at pfeature_scale.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 10/3/97 by Punam K. Saha
 *    Modified: 10/25/01 8-bit input handled by Dewey Odhner
 *    Modified: 11/2/01 single slice done by Dewey Odhner
 *
 *****************************************************************************/
void compute_feature_scale (int slice)
{
  int                  row, col, i, j, k, x, y, xx, yy, mean_g, tti5;
  unsigned short     **pin_16;
  unsigned char      **pin_8;
  int                  flag;
  double               count_obj, count_nonobj, tt1, tt2, tt3;


  
    if (pimage_8)
      pin_8 = pimage_8[slice];
    else
      pin_16 = pimage_16[slice];
    for (row = 0; row < prow; row++)  
      for (col = 0; col < pcol; col++) { 
        j = pimage_8? pin_8[row][col]: pin_16[row][col];
		if (j >= min_threshold && j <= max_threshold) {
          flag = 0;
          tt1 = 0.0;
          tt3 = 0.0;
          for (xx = -1; xx <= 1; xx++)
            for (yy = -1; yy <= 1; yy++)  {
              tt2 = 3.0;
              if (xx < 0)  {
                x = Get_max (row + xx, 0);
                tt2 = tt2 - (double) (-xx);
              }
              else  {
                x = Get_min (row + xx, prow - 1);
                tt2 = tt2 - (double) xx;
              }
              if (yy < 0)  {
                y = Get_max (col + yy, 0);
                tt2 = tt2 - (double) (-yy);
              }
              else  {
                y = Get_min (col + yy, pcol - 1);
                tt2 = tt2 - (double) yy;
              }
              tt2 = sqrt (tt2);
              tt1 = tt1 + tt2;
              tt3 = tt3 + tt2 * (pimage_8? pin_8[x][y]:pin_16[x][y]);
            }
          mean_g = (int) (tt3 / tt1 + 0.5);
          count_obj = 0;
          count_nonobj = 0;
          for (k = MIN_SCALE; (k < MAX_SCALE) && !flag; k++)  {
            count_obj = 0;
            count_nonobj = 0;
            for (i = 0; i < circle_no_points[k]; i++)  {
              xx = circle_points[k][i][0];
              yy = circle_points[k][i][1];
              if (xx < 0)
                x = Get_max (row + xx, 0);
              else
                x = Get_min (row + xx, prow - 1);
              if (yy < 0)
                y = Get_max (col + yy, 0);
              else
                y = Get_min (col + yy, pcol - 1);
              tti5 = ((int) (pimage_8? pin_8[x][y]:pin_16[x][y])) - mean_g;
              if (tti5 < 0)
                tti5 = -tti5;
              count_obj = count_obj + transformation_scale[tti5];
              count_nonobj =
                count_nonobj + 1.0 - transformation_scale[tti5];
            }
            if (100.0 * count_nonobj >= 
                sigma_s * (count_nonobj + count_obj))  {
              pfeature_scale[row][col] = k;
              flag = 1;
            }
          }
          if (!flag)
            pfeature_scale[row][col] = MAX_SCALE - 1;
		}
        else 
          pfeature_scale[row][col] = MIN_SCALE;
      }

}
