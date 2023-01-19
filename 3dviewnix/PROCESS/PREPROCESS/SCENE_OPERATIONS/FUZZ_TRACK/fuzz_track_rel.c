/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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

#include <math.h>
#include <string.h>
#include <cv3dv.h>
#include <assert.h>
#include <time.h>

#include "hheap.h"


#define CONN 4096
#define CONN_THRESHOLD 1

#define MAX_SCENES 3
#define MAX_OBJECTS  5
#define FILTER  1          // filtering matrix 2*FILTER+1 by 2*FILTER+1
#define SCALE 8
#define RELATIVE 1
#define ABSOLUTE 0


void fuzzy_track_hheap(int), fuzzy_track_chash(int), fuzzy_track_chash2(int);
void handle_error(int), parse_command_line(int argc, char *argv[]);
void compute_scale(), compute_filter(), compute_affinity();


/*************STRUCTURE AND MACRO DECLARATION FOR FUZZTRACK****************/
typedef struct
{
  short x, y, z;
}
Voxel;

#define MIN(a, b) ( (a) > (b) ? (b) : (a) )
#define MAX(a, b) ( (a) < (b) ? (b) : (a) )

static void *H;

Voxel nbor[6] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },  { -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 } };

char *points_filename;
int track_algorithm=2; /* 0=hheap; 1=chash ; 2=chash2 */

#define Handle_error(message) \
{ \
  printf(message); \
  fflush(stdout); \
  exit(1); \
}
/*****************************************************************/
/*-------added by Ying Zhuge to deal with the color image---------*/


unsigned char **data8;
unsigned short **data16;

unsigned char **filter_image8;
unsigned short **filter_image16;

double feature_mean[MAX_SCENES];

char *brain_maskfile;


double homo_cov[MAX_SCENES][MAX_SCENES];
double object_cov[MAX_OBJECTS][MAX_SCENES][MAX_SCENES];
double object_mean[MAX_OBJECTS][MAX_SCENES];


double tolerance = 13.0;

int scale_flag=1,bgsuppression_flag = 1,selected_object=0,fcmethod_flag = RELATIVE,homo_affn_flag = 1;

int num_scenes;
int num_objects;
static char *command_line(int argc, char **argv);

int cutoff_threshold = 0;
char *out_file, *argv0;
char *input_files[MAX_SCENES];

ViewnixHeader vh_in, vh_out;
int largest_density_value, pslice, prow, pcol, slice_size, volume_size, num_of_bits;

void *in_data;

#define in_data_8 ((unsigned char *)in_data)
#define in_data_16 ((unsigned short *)in_data)

typedef  unsigned short OutCellType;
OutCellType *out_data[MAX_OBJECTS];

int *sphere_no_points;
short (**sphere_points)[3];
int (*(point_seeds[MAX_OBJECTS]))[3], num_seeds[MAX_OBJECTS];

int feature = 0, affinity_flag = 0, iteration_flag = 0, max_iteration;
double anisotropy_slice, anisotropy_row, anisotropy_col;
unsigned char *scale_image;

OutCellType **x_affinity, **y_affinity, **z_affinity, *object_region;
float *material;
char **object_points_file, *x_affinity_file, *y_affinity_file, *z_affinity_file;
double mask[2*FILTER+1][2*FILTER+1], mask_total;
#define EE 2.71828182845904523536

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Performs 3-dimensional vectorial scale-based fuzzy tracking on
 *   multiple 8- or 16-bit input IM0 files to produce an IM0 file of the connectivity
 *   scene if absolut fc method selected, or an IM0 file of classified label scene if
 *   relative fc method selected
 * PARAMETERS:
 *  argc: the number of command line arguments
 *  argv: the command line arguments
 * SIDE EFFECTS: The histogram file will be removed.
 * ENTRY CONDITIONS: The environment variable VIEWNIX_ENV must be set to the
 *  3DVIEWNIX system directory.  
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code 1 on error.
 * HISTORY:
 *  Created:  5/13/03 by Ying Zhuge
 *
 *
 *****************************************************************************/
int main(int argc, char *argv[])
{

  int error_code, i, j, k, l, tti1, tti2, ***ppptti1;
  int xx, yy, x, slice, row, col;
  char group[5], element[5];
  FILE *fp_in, *fp_out;
  double tt1, tt2;

  
  time_t t1,t2;

  time(&t1);

  // assert((OutCellType) CONN == CONN);
  argv0 = argv[0];
  parse_command_line(argc, argv);


  fp_in = fopen(input_files[0], "r");
  if (fp_in == NULL)
	handle_error(4);
  error_code = VReadHeader(fp_in, &vh_in, group, element);
  switch (error_code)
	{
	case 0:
	case 106:
	case 107:
	  break;
	default:
	  fprintf(stderr, "file = %s; group = %s; element = %s\n", input_files[0], group,
			  element);
	  handle_error(error_code);
	}
  fclose(fp_in);

  /*@ Use assertions for now; handle gracefully later. */
  /*
  assert(vh_in.scn.num_of_density_values == 1);
  assert(vh_in.scn.num_of_integers == 1);
  assert(vh_in.scn.signed_bits_valid == 0 || vh_in.scn.signed_bits[0] == 0);
  assert(vh_in.scn.num_of_bits == 8 || vh_in.scn.num_of_bits == 16||vh_in.scn.num_of_bits == 24);
  assert(vh_in.scn.bit_fields[0] == 0);
  assert(vh_in.scn.bit_fields[1] == vh_in.scn.num_of_bits - 1);
  assert(vh_in.scn.bytes_in_alignment_valid == 0
		 || vh_in.scn.bytes_in_alignment <= 1);
  */
  if (vh_in.scn.dimension != 3)
	{
	  fprintf(stderr, "This program handles only 2D and 3D images\n");
	  exit(-1);
	}

  memcpy(&vh_out, &vh_in, sizeof(vh_in));
  strncpy(vh_out.gen.filename, out_file, sizeof(vh_out.gen.filename));
  
  vh_out.scn.num_of_bits = 16;

  vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits - 1;
  
  vh_out.scn.num_of_density_values=1;
  vh_out.scn.num_of_density_values_valid=1;

  vh_out.scn.smallest_density_value = (float *) malloc(sizeof(float));
  vh_out.scn.largest_density_value = (float *) malloc(sizeof(float));

  if (vh_out.scn.smallest_density_value == NULL
	  || vh_out.scn.largest_density_value == NULL)
       handle_error(1);
  vh_out.scn.smallest_density_value[0] = 0;
  if(fcmethod_flag == RELATIVE)
    vh_out.scn.largest_density_value[0] = (float)(num_objects*512);
  else
    vh_out.scn.largest_density_value[0] = CONN;

  vh_out.scn.smallest_density_value_valid = TRUE;
  vh_out.scn.largest_density_value_valid = TRUE;

  pcol = vh_in.scn.xysize[0];
  prow = vh_in.scn.xysize[1];
  pslice = vh_in.scn.num_of_subscenes[0];

  slice_size = pcol * prow;
  volume_size = slice_size * pslice;
  num_of_bits = vh_in.scn.num_of_bits;
  vh_out.scn.num_of_subscenes = (short *) malloc(sizeof(short));

  if (vh_out.scn.num_of_subscenes == NULL)
	handle_error(1);
  vh_out.scn.num_of_subscenes[0] = pslice;
  vh_out.scn.loc_of_subscenes = (float *) malloc(pslice * sizeof(float));

  if (vh_out.scn.loc_of_subscenes == NULL)
	handle_error(1);
  for (j = 0; j < pslice; j++)
	vh_out.scn.loc_of_subscenes[j] = vh_in.scn.loc_of_subscenes[j];

  vh_out.scn.description = command_line(argc, argv);
  vh_out.scn.description_valid = 1;
  fp_out = fopen(out_file, "w+");
  if (fp_out == NULL)
	handle_error(4);
  handle_error(VWriteHeader(fp_out, &vh_out, group, element));

  if (num_of_bits == 8)
    {
      data8 = (unsigned char **)malloc(num_scenes*sizeof(unsigned char*));
      data8[0] = (unsigned char *)malloc(num_scenes*volume_size*sizeof(char));
      if(data8 ==NULL||data8[0]==NULL)
	Handle_error("Memory allocation error \n");
      for (i = 0;i< num_scenes;i++)
	{
	  data8[i] =  data8[0] + i*volume_size;
	  fp_in = fopen(input_files[i],"r");
	  if (fp_in == NULL)
	    handle_error(4);
	  VSeekData(fp_in, 0);
	  error_code = VReadData((char *)data8[i], num_of_bits/8, volume_size, fp_in, &j);
	  switch (error_code)
	    {
	    case 0:
	    case 106:
	    case 107:
	      break;
	    default:
	      fprintf(stderr, "file = %s; group = %s; element = %s\n", input_files[i], group,element);
	      handle_error(error_code);
	    }
	  fclose(fp_in);
	}
    }
  else if (num_of_bits == 16)
    {
      data16 = (unsigned short **)malloc(num_scenes*sizeof(unsigned short*));
      data16[0] = (unsigned short *)malloc(num_scenes*volume_size*sizeof(short));
      if(data16 ==NULL||data16[0]==NULL)
	Handle_error("Memory allocation error \n");
      for (i = 0;i< num_scenes;i++)
	{
	  data16[i] =  data16[0] + i*volume_size;
	  fp_in = fopen(input_files[i],"r");
	  if (fp_in == NULL)
	    handle_error(4);
	  VSeekData(fp_in, 0);
	  error_code = VReadData((char *)data16[i], num_of_bits/8, volume_size, fp_in, &j);
	  switch (error_code)
	    {
	    case 0:
	    case 106:
	    case 107:
	      break;
	    default:
	      fprintf(stderr, "file = %s; group = %s; element = %s\n", input_files[i], group,element);
	      handle_error(error_code);
	    }
	  fclose(fp_in);
	}

    }
  if(num_of_bits == 8)
	{
	  for (i = 0; i < num_scenes; i++)
	    {
	      feature_mean[i] = 0;
	      for(j = 0;j < volume_size; j++)
		{
		  feature_mean[i] += data8[i][j];
		  if(data8[i][j]>largest_density_value)
		    largest_density_value = data8[i][j];
		}
	      feature_mean[i] = feature_mean[i]/volume_size;
	    }
	  
	}
  else if(num_of_bits == 16)
	{
	  for (i = 0; i < num_scenes; i++)
	    {
	      feature_mean[i] = 0;
	      for(j = 0;j < volume_size; j++)
		{
		  feature_mean[i] += data16[i][j];
		  if(data16[i][j]>largest_density_value)
		    largest_density_value = data16[i][j];
		}
	      feature_mean[i] = feature_mean[i]/volume_size;
	    }
	}

  /***********************************************/
  tti1 = 2 * (SCALE + 5);
  ppptti1 = (int ***) malloc(tti1 * sizeof(int **));

  if (ppptti1 == NULL)
	Handle_error("COULD ALLOCATE MEMORY\n");
  ppptti1[0] = (int **) malloc(tti1 * tti1 * sizeof(int *));

  if (ppptti1[0] == NULL)
	Handle_error("COULD ALLOCATE MEMORY\n");
  for (i = 0; i < tti1; i++)
	ppptti1[i] = ppptti1[0] + i * tti1;
  ppptti1[0][0] = (int *) malloc(tti1 * tti1 * tti1 * sizeof(int));

  if (ppptti1[0][0] == NULL)
	Handle_error("COULD ALLOCATE MEMORY\n");
  for (i = 0; i < tti1; i++)
	for (j = 0; j < tti1; j++)
	  ppptti1[i][j] = ppptti1[0][0] + (i * tti1 + j) * tti1;

  for (i = 0; i < tti1; i++)
	for (j = 0; j < tti1; j++)
	  for (k = 0; k < tti1; k++)
		ppptti1[i][j][k] = 0;

  sphere_no_points = (int *) malloc((SCALE + 1) * sizeof(int));

  if (sphere_no_points == NULL)
	Handle_error("Couldn't allocate memory (execution terminated)\n");

  sphere_points = (void *) malloc((SCALE + 1) * sizeof(void *));

  if (sphere_points == NULL)
	Handle_error("Couldn't allocate memory (execution terminated)\n");


  anisotropy_col = vh_in.scn.xypixsz[0];
  anisotropy_row = vh_in.scn.xypixsz[1];
  if (pslice > 1)
	anisotropy_slice = vh_in.scn.loc_of_subscenes[1] - vh_in.scn.loc_of_subscenes[0];
  if (anisotropy_slice < 0.0)
	anisotropy_slice = -anisotropy_slice;
  tt1 = anisotropy_col;
  if (tt1 > anisotropy_row)
	tt1 = anisotropy_row;
  if (pslice > 1 && tt1 > anisotropy_slice)
	tt1 = anisotropy_slice;
  anisotropy_col = anisotropy_col / tt1;
  anisotropy_row = anisotropy_row / tt1;
  if (pslice > 1)
	anisotropy_slice = anisotropy_slice / tt1;

  tti1 = SCALE + 5;
  if (pslice > 1)
	printf("Anisotropy: slice = %f, row = %f, column = %f\n",
		   anisotropy_slice, anisotropy_row, anisotropy_col);
  else
	printf("Anisotropy: row = %f, column = %f\n", anisotropy_row, anisotropy_col);

  if (pslice > 1)
	{
	  for (k = 0; k <= SCALE; k++)
		{
		  sphere_no_points[k] = 0;
		  // for (i = -k - 2; i <= k + 2; i++)
		  for (i = 0; i < 1; i++)
		     for (j = -k - 2; j <= k + 2; j++)
		       for (l = -k - 2; l <= k + 2; l++)
			 if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 0)
			   {
			     tt1 = sqrt(pow(((double) i) * anisotropy_slice, 2.0) +
					pow(((double) j) * anisotropy_row,
					    2.0) + pow(((double) l) * anisotropy_col, 2.0));
			     if (tt1 <= ((double) k) + 0.5)
			       {
				 sphere_no_points[k] = sphere_no_points[k] + 1;
				 ppptti1[tti1 + i][tti1 + j][tti1 + l] = 2;
			       }
			   }
		   sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));

		   if (sphere_points[k] == NULL)
		     Handle_error("Couldn't allocate memory (execution terminated)\n");

		   tti2 = 0;
		   // for (i = -k - 2; i <= k + 2; i++)  // compute scale in 2D rather 3D
		   for (i = 0; i < 1; i++) 
		     for (j = -k - 2; j <= k + 2; j++)
		       for (l = -k - 2; l <= k + 2; l++)
			 if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 2)
			   {
			     ppptti1[tti1 + i][tti1 + j][tti1 + l] = 1;
			     sphere_points[k][tti2][0] = i;
			     sphere_points[k][tti2][1] = j;
			     sphere_points[k][tti2][2] = l;
			     tti2 = tti2 + 1;
			   }
		}
	}
  else
	{
	  for (k = 0; k <= SCALE; k++)
		{
		  sphere_no_points[k] = 0;
		  for (j = -k - 2; j <= k + 2; j++)
			for (l = -k - 2; l <= k + 2; l++)
			  if (ppptti1[tti1][tti1 + j][tti1 + l] == 0)
				{
				  tt1 = sqrt(pow(((double) j) * anisotropy_row, 2.0)
							 + pow(((double) l) * anisotropy_col, 2.0));
				  if (tt1 <= ((double) k) + 0.5)
					{
					  sphere_no_points[k] = sphere_no_points[k] + 1;
					  ppptti1[tti1][tti1 + j][tti1 + l] = 2;
					}
				}

		  sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));

		  if (sphere_points[k] == NULL)
			Handle_error("Couldn't allocate memory (execution terminated)\n");

		  tti2 = 0;
		  for (j = -k - 2; j <= k + 2; j++)
		    for (l = -k - 2; l <= k + 2; l++)
		      if (ppptti1[tti1][tti1 + j][tti1 + l] == 2)
			{
			  ppptti1[tti1][tti1 + j][tti1 + l] = 1;
			  sphere_points[k][tti2][0] = 0;
			  sphere_points[k][tti2][1] = j;
			  sphere_points[k][tti2][2] = l;
			  tti2 = tti2 + 1;
			}
		}
	}
  printf("\n");
  fflush(stdout);
  free(ppptti1[0][0]);
  free(ppptti1[0]);
  free(ppptti1);

  mask_total = 0.0;

  for (yy = -FILTER; yy <= FILTER; yy++)
    for (xx = -FILTER; xx <= FILTER; xx++)
      mask[yy + FILTER][xx + FILTER] = 0;

  for (yy = -FILTER; yy <= FILTER; yy++)
    for (xx = -FILTER; xx <= FILTER; xx++)
      {
	tt2 = pow(anisotropy_col * xx, 2.0);
	tt2 = tt2 + pow(anisotropy_row * yy, 2.0);
	tt2 = 1 / (1 + tt2);
	mask[yy + FILTER][xx + FILTER] = tt2;
	mask_total = mask_total + tt2;
      }
  /***************************************************************************************/

  if(num_of_bits == 8)
    {
          filter_image8 = (unsigned char **)malloc(num_scenes*sizeof(unsigned char*));
	  filter_image8[0] = (unsigned char *)malloc(num_scenes*volume_size*sizeof(char));
	  if(filter_image8 ==NULL||filter_image8[0]==NULL)
	    Handle_error("Memory allocation error \n");
	  for (i = 0;i< num_scenes;i++)
	    filter_image8[i] =  filter_image8[0] + i*volume_size;
    }
  else if(num_of_bits == 16)
    {
	  filter_image16 = (unsigned short **)malloc(num_scenes*sizeof(unsigned short*));
	  filter_image16[0] = (unsigned short *)malloc(num_scenes*volume_size*sizeof(short));
	  if(filter_image16 ==NULL||filter_image16[0]==NULL)
	    Handle_error("Memory allocation error \n");
	  for (i = 0;i< num_scenes;i++)
	      filter_image16[i] =  filter_image16[0] + i*volume_size;
    }

  if(scale_flag== 1)
    { 
      compute_scale();
      time(&t2);
      printf("scale computation last:%f seconds\n",difftime(t2,t1)); 

      compute_filter();
      time(&t1);
      printf("filtering computation last:%f seconds\n",difftime(t1,t2)); 
 
    }
  else if(scale_flag ==0)
    {
      for(x = 0;x<num_scenes;x++)
	for (i = 0; i< pslice; i++)
	  for (j = 0;j< prow; j++)
	    for (k = 0;k < pcol; k++)
	    {
		  if(num_of_bits == 8)
		    filter_image8[x][i*slice_size + j*pcol +k] = data8[x][i*slice_size + j*pcol +k];
		  else if(num_of_bits == 16)
		    filter_image16[x][i*slice_size + j*pcol +k] = data16[x][i*slice_size + j*pcol +k];
	    }
    }

  /* @@ lookup table */
  /*--------------------------------AFFINITY ALLOCATION-----------------------------------*/

  x_affinity = (OutCellType **) malloc(num_objects * sizeof(OutCellType*));
  y_affinity = (OutCellType **) malloc(num_objects * sizeof(OutCellType*));
  z_affinity = (OutCellType **) malloc(num_objects * sizeof(OutCellType*));

  if (x_affinity == NULL || y_affinity == NULL || z_affinity == NULL)
    {
      if (x_affinity)
	free(x_affinity);
      if (y_affinity)
	free(y_affinity);
      if (z_affinity)
	free(z_affinity);
      handle_error(1);
    }

  x_affinity[0] = (OutCellType *) malloc(num_objects * volume_size * sizeof(OutCellType));
  y_affinity[0] = (OutCellType *) malloc(num_objects * volume_size * sizeof(OutCellType));
  z_affinity[0] = (OutCellType *) malloc(num_objects * volume_size * sizeof(OutCellType));
  material = (float* ) malloc(volume_size * sizeof(float));

  if (x_affinity[0] == NULL || y_affinity[0] == NULL || z_affinity[0] == NULL ||material == NULL)
	{
	  if (x_affinity[0])
		free(x_affinity[0]);
	  if (y_affinity[0])
		free(y_affinity[0]);
	  if (z_affinity[0])
		free(z_affinity[0]);
	  if(material)
	    free(material);
	  handle_error(1);
	}

  for(i = 0;i<num_objects;i++)
    {
      x_affinity[i] = x_affinity[0] + i*volume_size;
      y_affinity[i] = y_affinity[0] + i*volume_size;
      z_affinity[i] = z_affinity[0] + i*volume_size;
    }

  compute_affinity();
  time(&t2);
  printf("affinity computation last:%f seconds\n",difftime(t2,t1));

  for(i = 0;i<num_objects;i++)
    {
      out_data[i] = (OutCellType *) malloc(volume_size * sizeof(OutCellType));
      if (out_data[i] == NULL)
	handle_error(1);
    }

  if(fcmethod_flag == RELATIVE)
    {

      for(i = 0;i<num_objects;i++)
	{ 
	  switch (track_algorithm)
	    {
	    case 0:
	      fuzzy_track_hheap(i);
	      break;
	    case 1:
	      fuzzy_track_chash(i);
	      break;
	    case 2:
	      fuzzy_track_chash2(i);
	      break;
	    }
	}

      object_region = (OutCellType *) malloc(volume_size * sizeof(OutCellType));
      if (object_region == NULL)
	{  
	  handle_error(1);
	  exit(-1);
	}
      memset(object_region, 0, volume_size* sizeof(OutCellType));

      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)
	    {
	      tti2 = slice * slice_size + row * pcol + col;

	      if(num_of_bits == 8)
		tti1 = filter_image8[0][tti2];
	      else if(num_of_bits == 16)
		tti1 = filter_image16[0][tti2];

	      if((bgsuppression_flag)&&(tti1<feature_mean[0]))
		object_region[slice * slice_size + row * pcol + col] = 0;
	      else
		{
		  k = 0;
		  for(i = 1;i<num_objects;i++)
		    if( out_data[k][tti2] < out_data[i][tti2])
		      k = i;
		  object_region[slice * slice_size + row * pcol + col] = (k+1)*512;
		}
	    }
      handle_error(VWriteData((char *)object_region, sizeof(OutCellType), volume_size, fp_out, &j));
    }
  else if(fcmethod_flag == ABSOLUTE)
    {
      switch (track_algorithm)
	{
	case 0:
	  fuzzy_track_hheap(selected_object);
	  break;
	case 1:
	  fuzzy_track_chash(selected_object);
	  break;
	case 2:
	  fuzzy_track_chash2(selected_object);
	  break;
	}
      handle_error(VWriteData((char *)out_data[selected_object], sizeof(OutCellType), volume_size, fp_out, &j));
    }
  VCloseData(fp_out);
  
  time(&t1);
  printf("tracking last:%f seconds\n",difftime(t1,t2)); 
    
  if(object_region!=NULL)
    free(object_region);

  for(i = 0;i<num_objects;i++)
    {
      if(point_seeds[i] !=NULL)
	free(point_seeds[i]);

      if(out_data[i] != NULL)
	free(out_data[i]);
    }

  if(num_of_bits == 8)
    {	
	  if(data8[0]!=NULL)
	    free( data8[0]);
	  if(data8!=NULL)
	    free(data8);

	  if(filter_image8[0]!=NULL)
	    free(filter_image8[0]);
	  if(filter_image8!=NULL)
	    free(filter_image8);   
    }
  if(num_of_bits == 16)
    {
	  if(filter_image16[0]!=NULL)
	    free(filter_image16[0]);
	  if(filter_image16!=NULL)
	    free(filter_image16);
	
	  if(data16[0]!=NULL)
	    free(data16[0]);
	  if(data16!=NULL)
	    free(data16);
    }


  if(x_affinity[0]!=NULL)
    free(x_affinity[0]);
  if(y_affinity[0]!=NULL)
    free(y_affinity[0]);
  if(z_affinity[0]!=NULL)
    free(z_affinity[0]);

  if(x_affinity!=NULL)
    free(x_affinity);
  if(y_affinity!=NULL)
    free(y_affinity);
  if(z_affinity!=NULL)
    free(z_affinity);
  
  if(material !=NULL)
    free(material);

  printf("program exit normally\n");

  exit(0);
}

/*****************************************************************************
 * FUNCTION: command_line
 * DESCRIPTION: Returns a string with the command line arguments.
 * PARAMETERS:
 *  argc: number of command line arguments
 *  argv: command line arguments
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables argv0, bg_flag must be properly set.
 * RETURN VALUE: a string with the command line arguments
 * EXIT CONDITIONS: Writes an error message to stderr and exits with code 1
 *  if memory allocation fails.
 * HISTORY:
 *  Created: 4/2/96 by Dewey Odhner
 *
 *****************************************************************************/
static char *
command_line(int argc, char **argv)
{
  int j, k;
  char *cl;

  k = argc;
  for (j = 0; j < argc; j++)
	k += (int)strlen(argv[j]);
  cl = (char *) malloc(k);
  if (cl == NULL)
	handle_error(1);
  k = 0;
  for (j = 0; j < argc; j++)
	{
	  strcpy(cl + k, argv[j]);
	  k += (int)strlen(argv[j]);
	  cl[k] = ' ';
	  k++;
	}
  cl[k - 1] = 0;
  return (cl);
}

/*****************************************************************************
 * FUNCTION: usage
 * DESCRIPTION: Displays the usage message on stderr and exits with code 1.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code 1.
 * HISTORY:
 *  Created: 12/16/09 by Dewey Odhner
 *
 *****************************************************************************/
void usage()
{
      fprintf(stderr, "Usage: fuzz_track_rel #scenes #objects input_files[scenes] out_file selected_object scale_flag\n");
      fprintf(stderr, "       bgsuppression_flag relative_flag homo_affn_flag homo_matrix[scenes][scenes]\n");
      fprintf(stderr, "       object_mean[objects][scenes] object_matrix[objects][scenes][scenes]\n");
      fprintf(stderr, "       #seeds[objects] points<x,y,z>...\n");
      exit(1);
}

/*****************************************************************************
 * FUNCTION: parse_command_line
 * DESCRIPTION: Initializes the variables input_filename, file_info,
 *  output_filename, bg_flag, feature_status, function_selected, weight,
 *  function_level, function_width, point_picked, num_points_picked,
 *  threshold, mask_original, training_mean, reverse_training_mean,
 *  inv_covariance, inv_reverse_covariance, covariance_flag,
 *  reverse_covariance_flag according to the command line arguments.
 * PARAMETERS:
 *  argc: number of command line arguments
 *  argv: command line arguments
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables feature_status, mask_original, affinity_type
 *   covariance_flag, reverse_covariance_flag be initialized with zeros.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Writes an error message to stderr and exits with code 1
 *  if command line does not parse or memory allocation fails.
 * HISTORY:
 *  Created: 4/2/96 by Dewey Odhner
 *  Modified: 4/17/96 output of original values allowed by Dewey Odhner
 *  Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
 *  Modified: 7/29/96 covariance affinity type allowed by Dewey Odhner
 *  Modified: 5/12/03 remove the previous code, initialize the variables num_scenes,
 *           num_objects,input_files,out_file,selected_object,scale_flag,bgsuppression_flag,
 *           fcmethod_flag,homo_affn_flag,homo_cov,object_mean,object_cov,num_seeds,
 *           point_seeds according to the command line arguments by Ying Zhuge
 *****************************************************************************/
void parse_command_line(int argc, char *argv[])
{
  int i, j,k,args_parsed;
  int (*t)[3];

  args_parsed = 1;
  //*************************************************************************************************************
  if (argc < 9)
    usage();
  num_scenes = atoi(argv[args_parsed++]);
  num_objects = atoi(argv[args_parsed++]);
  if(argc<(9 + num_scenes + num_scenes*num_scenes + num_objects*num_scenes + num_objects*num_scenes*num_scenes + num_objects))
    usage();

  for( i = 0;i<num_scenes;i++)
     input_files[i] = argv[args_parsed++];
  out_file =  argv[args_parsed++];
  selected_object = atoi(argv[args_parsed++]);
  scale_flag = atoi(argv[args_parsed++]);
  bgsuppression_flag = atoi(argv[args_parsed++]);
  fcmethod_flag = atoi(argv[args_parsed++]);
  homo_affn_flag = atoi(argv[args_parsed++]);

  for(i = 0;i<num_scenes;i++)
    for(j = 0;j<num_scenes;j++)
      sscanf(argv[args_parsed++],"%lf",&homo_cov[i][j]);

  for(i = 0;i<num_objects;i++)
    for( j = 0;j<num_scenes;j++)
      sscanf(argv[args_parsed++],"%lf",&object_mean[i][j]);

  for(i = 0;i<num_objects;i++)
    {
      for(j = 0;j<num_scenes;j++)
	for(k = 0;k<num_scenes;k++)
	  sscanf(argv[args_parsed++],"%lf",&object_cov[i][j][k]);
    }
  for(i = 0;i<num_objects;i++)
    {
      num_seeds[i] = atoi(argv[args_parsed++]);
      point_seeds[i] = (void*) malloc(num_seeds[i]*sizeof(*t));
    }

  for(i = 0;i<num_objects;i++)
    for(j = 0;j<num_seeds[i];j++)
      {
	for(k = 0;k<3;k++)
	  point_seeds[i][j][k] = atoi(argv[args_parsed++]);
      }
  //***************************************************************************************

}

/***************************************************
 * FUNCTION: compute_affinity
 * DESCRIPTION: Computes the affinity values for the entire volume and  
 *        store in the three arrays x-, y-, and z-affinity.
 * PARAMETERS: None
 * SIDE EFFECTS: 
 * FUNCTIONS CALEED: None
 * ENTRY CONDITIONS: The variables filter_image8/16,object_cov,
 *    object_mean,homo_cov,bgsuppression_flag,homo_affn_flag must be
 *    properly set. x-, y-, and z-affinity must be allocated properly.
 *
 * RETURN VALUE: None
 * EXIT CONDITIONS: Compute homogeneity values in the three arrays 
 *          x-, y-, and z-affinity
 * HISTORY:
 *  Created: 5/12/03 by Ying Zhuge
 *
 *****************************************************************************/
void compute_affinity()
{

  int j;
  int col, row, slice, col1, row1, slice1;
  int obj;
  double result,tt2,homo_aff,object_aff;
  double temp[MAX_SCENES];

  for(obj = 0;obj<num_objects;obj++)
  {
    if (num_of_bits == 8)
    {
      /** object-based affinity */
      memset(material,0,volume_size*sizeof(float));

     for (slice = 0; slice < pslice; slice++)
       for (row = 0; row < prow; row++)
	 for (col = 0; col < pcol; col++)
	    {
	      if((bgsuppression_flag)&&(filter_image8[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
		material[slice*slice_size + row*pcol + col] = 0;
	      else
	      {
	         for(j=0;j<num_scenes;j++)
		    temp[j] = fabs(filter_image8[j][slice*slice_size + row*pcol + col] - object_mean[obj][j]);
	      
		 result = 0;
		 for(j=0;j<num_scenes;j++)
		   result = result + (temp[j]*temp[j])/(object_cov[obj][j][j]*object_cov[obj][j][j]);
		 material[slice*slice_size + row*pcol + col] = (float)exp(-0.5*result);
	      }
	    }
     /** homogeneity-based affinity */
     for (slice = 0; slice < pslice; slice++)
       for (row = 0; row < prow; row++)
	 for (col = 0; col < pcol - 1; col++)
	   {
	     if((bgsuppression_flag)&&(filter_image8[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	       x_affinity[obj][slice * slice_size + row * pcol + col] = 0;
	     else
	       {
		 col1 = col + 1;
		 row1 = row;
		 slice1 = slice;

		 /** set object-based affinity as the smaller one */
                 if(material[slice*slice_size + row*pcol + col]>material[slice1*slice_size + row1*pcol + col1])
		   object_aff = material[slice1*slice_size + row1*pcol + col1];
		 else
		   object_aff = material[slice*slice_size + row*pcol + col];

		 if(homo_affn_flag == 1)
		   {
		     for(j=0;j<num_scenes;j++)
		       temp[j] = (double) abs(filter_image8[j][slice*slice_size + row*pcol + col] -
					      filter_image8[j][slice1*slice_size + row1*pcol + col1]);

		     result = 0;
		     for(j=0;j<num_scenes;j++)
		       result = result + (temp[j]*temp[j])/(homo_cov[j][j]*homo_cov[j][j]);
		     homo_aff = exp(-0.5*result);

		     /** combine two affinities together */
		     tt2 = sqrt(homo_aff*object_aff);
		   }
		 else if(homo_affn_flag == 0)/* set affinity as object-based affinity */
		   tt2 = object_aff;

		 x_affinity[obj][slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt2);
	       }
	   }
     
     for (slice = 0; slice < pslice; slice++)
       for (row = 0; row < prow-1; row++)
	 for (col = 0; col < pcol; col++)
	   {
	     if((bgsuppression_flag)&&(filter_image8[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	       y_affinity[obj][slice * slice_size + row * pcol + col] = 0;
	     else
	     {
		 col1 = col;
		 row1 = row + 1;
		 slice1 = slice;
		 
		 /** set object-based affinity as the smaller one */
                 if(material[slice*slice_size + row*pcol + col]>material[slice1*slice_size + row1*pcol + col1])
		   object_aff = material[slice1*slice_size + row1*pcol + col1];
		 else
		   object_aff = material[slice*slice_size + row*pcol + col];

		 if(homo_affn_flag == 1)
		 {
		   for(j=0;j<num_scenes;j++)
		     temp[j] = (double) abs(filter_image8[j][slice*slice_size + row*pcol + col] -
					    filter_image8[j][slice1*slice_size + row1*pcol + col1]);

		   result = 0;
		   for(j=0;j<num_scenes;j++)
		     result = result + (temp[j]*temp[j])/(homo_cov[j][j]*homo_cov[j][j]);
		   homo_aff = exp(-0.5*result);

		   /** combine two affinities together */
		   tt2 = sqrt(homo_aff*object_aff);
		 }
		 else if(homo_affn_flag == 0)/* set affinity as object-based affinity */
		   tt2 = object_aff;

		 y_affinity[obj][slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt2);
	       }
	   }

     for (slice = 0; slice < pslice-1; slice++)
       for (row = 0; row < prow; row++)
	 for (col = 0; col < pcol; col++)
	   {
	     if((bgsuppression_flag)&&(filter_image8[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	       z_affinity[obj][slice * slice_size + row * pcol + col] = 0;
	     else
               {
		 col1 = col;
		 row1 = row;
		 slice1 = slice + 1;

		 /** set object-based affinity as the smaller one */
                 if(material[slice*slice_size + row*pcol + col]>material[slice1*slice_size + row1*pcol + col1])
		   object_aff = material[slice1*slice_size + row1*pcol + col1];
		 else
		   object_aff = material[slice*slice_size + row*pcol + col];

		 if(homo_affn_flag == 1)
		   {		 
		     for(j=0;j<num_scenes;j++)
		       temp[j] = (double) abs(filter_image8[j][slice*slice_size + row*pcol + col] -
					      filter_image8[j][slice1*slice_size + row1*pcol + col1]);

		     result = 0;
		     for(j=0;j<num_scenes;j++)
		       result = result + (temp[j]*temp[j])/(homo_cov[j][j]*homo_cov[j][j]);
		     homo_aff = exp(-0.5*result);
		     
		     /** combine two affinities together */
		     tt2 = sqrt(homo_aff*object_aff);
		   }
		 else if(homo_affn_flag == 0) /* set affinity as object-based affinity */
		   tt2 = object_aff;

		 z_affinity[obj][slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt2);
	       }
	   }
    }
    else if (num_of_bits == 16)
    {
      /** object-based affinity */
      memset(material,0,volume_size*sizeof(float));

      for (slice = 0; slice < pslice; slice++)
       for (row = 0; row < prow; row++)
	 for (col = 0; col < pcol; col++)
	    {
	      if((bgsuppression_flag)&&(filter_image16[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
		material[slice*slice_size + row*pcol + col] = 0;
	      else
	      {
	         for(j=0;j<num_scenes;j++)
		    temp[j] = fabs(filter_image16[j][slice*slice_size + row*pcol + col] - object_mean[obj][j]);
	      
		 result = 0;
		 for(j=0;j<num_scenes;j++)
		   result = result + (temp[j]*temp[j])/(object_cov[obj][j][j]*object_cov[obj][j][j]);
		 material[slice*slice_size + row*pcol + col] = (float)exp(-0.5*result);
	      }
	    }
      /** homogeneity-based affinity */
      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol - 1; col++)
	   {
	     if((bgsuppression_flag)&&(filter_image16[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	       x_affinity[obj][slice * slice_size + row * pcol + col] = 0;
	     else
	       {
		 col1 = col + 1;
		 row1 = row;
		 slice1 = slice;

		 /** set object-based affinity as the smaller one */
                 if(material[slice*slice_size + row*pcol + col]>material[slice1*slice_size + row1*pcol + col1])
		   object_aff = material[slice1*slice_size + row1*pcol + col1];
		 else
		   object_aff = material[slice*slice_size + row*pcol + col];

		 if(homo_affn_flag == 1)
		   {		 
		     for(j=0;j<num_scenes;j++)
		       temp[j] = (double) abs(filter_image16[j][slice*slice_size + row*pcol + col] -
					      filter_image16[j][slice1*slice_size + row1*pcol + col1]);
	    
		     result = 0;
		     for(j=0;j<num_scenes;j++)
		       result = result + (temp[j]*temp[j])/(homo_cov[j][j]*homo_cov[j][j]);
		     homo_aff = exp(-0.5*result);

		     /** combine two affinities together */
		     tt2 = sqrt(homo_aff*object_aff);
		   }
		 else if(homo_affn_flag == 0) /* set affinity as object-based affinity */
		   tt2 = object_aff;

		 x_affinity[obj][slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt2);
	       }
	   }
     
	     for (slice = 0; slice < pslice; slice++)
       for (row = 0; row < prow-1; row++)
	 for (col = 0; col < pcol; col++)
	   {
	     if((bgsuppression_flag)&&(filter_image16[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	       y_affinity[obj][slice * slice_size + row * pcol + col] = 0;
	     else
	     {
		 col1 = col;
		 row1 = row + 1;
		 slice1 = slice;

		 /** set object-based affinity as the smaller one */
                 if(material[slice*slice_size + row*pcol + col]>material[slice1*slice_size + row1*pcol + col1])
		   object_aff = material[slice1*slice_size + row1*pcol + col1];
		 else
		   object_aff = material[slice*slice_size + row*pcol + col];

		 if(homo_affn_flag == 1)
		   {		 		 
		     for(j=0;j<num_scenes;j++)
		       temp[j] = (double) abs(filter_image16[j][slice*slice_size + row*pcol + col] -
					      filter_image16[j][slice1*slice_size + row1*pcol + col1]);

		     result = 0;
		     for(j=0;j<num_scenes;j++)
		       result = result + (temp[j]*temp[j])/(homo_cov[j][j]*homo_cov[j][j]);
		     homo_aff = exp(-0.5*result);

		     /** combine two affinities together */
		     tt2 = sqrt(homo_aff*object_aff);
		   }
		 else if(homo_affn_flag == 0) /* set affinity as object-based affinity */
		   tt2 = object_aff;

		 y_affinity[obj][slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt2);
	       }
	   }

     for (slice = 0; slice < pslice-1; slice++)
       for (row = 0; row < prow; row++)
	 for (col = 0; col < pcol; col++)
	   {
	     if((bgsuppression_flag)&&(filter_image16[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	       z_affinity[obj][slice * slice_size + row * pcol + col] = 0;
	     else
               {
		 col1 = col;
		 row1 = row;
		 slice1 = slice + 1;

		 /** set object-based affinity as the smaller one */
                 if(material[slice*slice_size + row*pcol + col]>material[slice1*slice_size + row1*pcol + col1])
		   object_aff = material[slice1*slice_size + row1*pcol + col1];
		 else
		   object_aff = material[slice*slice_size + row*pcol + col];

		 if(homo_affn_flag == 1)
		   {		 
		     for(j=0;j<num_scenes;j++)
		       temp[j] = (double) abs(filter_image16[j][slice*slice_size + row*pcol + col] -
					      filter_image16[j][slice1*slice_size + row1*pcol + col1]);

		     result = 0;
		     for(j=0;j<num_scenes;j++)
		       result = result + (temp[j]*temp[j])/(homo_cov[j][j]*homo_cov[j][j]);
		     homo_aff = exp(-0.5*result); 

		     /** combine two affinities together */
		     tt2 = sqrt(homo_aff*object_aff);
		   }
		 else if(homo_affn_flag == 0) /* set affinity as object-based affinity */
		   tt2 = object_aff;

		 z_affinity[obj][slice * slice_size + row * pcol + col] = (unsigned) (CONN * tt2);
	       }
	   }
    }
  }
  printf("\rAffinity computation is done.     \n"); fflush(stdout);

}



/********************************************************************************
 * FUNCTION: compute_filter
 * DESCRIPTION: Compute scale_based filtering (distance weighted intensity average)
 *         image for the entire volume and store them into filter_image8/16.
 *             
 * PARAMETERS: None
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: The variables scale_flag, bgsuppression_flag,
 *    scale_image must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: 
 * HISTORY:
 *    Created: 5/12/03 by Ying Zhuge
 *
 *****************************************************************************/
void compute_filter()
{
  int i,j,k,iscale;
  int xx,yy,zz,x,y,z;
  int col, row, slice;
  double tt1,tt2,inv_k,count,temp[MAX_SCENES];
  double weight[SCALE][SCALE];

  
  for(i = 0;i<SCALE;i++)
    for(j = 0;j<SCALE;j++)
      weight[i][j] = 0;

  for(i = 1;i<=SCALE;i++)
    {
      tt1 = (double)i*0.5;
      tt2 = -0.5 / pow(tt1, 2.0);
	
      for(j = 0;j<i;j++)
	{
	  inv_k = exp(tt2 * pow((double)j, 2.0));
	  weight[i-1][j] = inv_k;
	}
    }
  
  if ((num_of_bits == 8)) 
    {
      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)
	    {

	      if((bgsuppression_flag)&&(data8[0][slice * slice_size + row * pcol + col] < feature_mean[0]))
		for(i = 0;i<num_scenes;i++)
		  filter_image8[i][slice * slice_size + row * pcol + col] = 0;
	      else
		{
		  iscale = scale_image[slice * slice_size + row * pcol + col];
		  count = 0.0;
		  for(i = 0;i<num_scenes;i++)
		    temp[i] = 0;

		  for (k = 0; k < iscale; k++)
		    {
		      tt1 = weight[iscale-1][k];
		      
		      for (i = 0; i < sphere_no_points[k]; i++)
			{
			  xx = sphere_points[k][i][2];
			  yy = sphere_points[k][i][1];
			  zz = sphere_points[k][i][0];
			  x = col + xx;
			  y = row + yy;
			  z = slice + zz;
			  if (x >= 0 && y >= 0 && z >= 0 
			      && x < pcol && y < prow && z < pslice) 
			    {
			      for(j=0;j<num_scenes;j++)
				temp[j] = temp[j] + tt1*(int)data8[j][z*slice_size+y*pcol+x];
			      
			      count = count + tt1;
			    }
			}
		    }
		  for(i = 0;i<num_scenes;i++)
		     filter_image8[i][slice * slice_size + row * pcol + col] = (unsigned char)((int)temp[i]/count);
		}
	    }
    }

  if(num_of_bits == 16)
    {
      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)
	    {
	      if((bgsuppression_flag==1)&&(data16[0][slice * slice_size + row * pcol + col]<feature_mean[0]))
		for(i = 0;i<num_scenes;i++)
		  filter_image16[i][slice * slice_size + row * pcol + col] = 0;
	      else
		{
		  iscale = scale_image[slice * slice_size + row * pcol + col];
		  count = 0.0;
		  for(i = 0;i<num_scenes;i++)
		    temp[i] = 0;
		  
		  for (k = 0; k < iscale; k++)
		    {
		      tt1 = weight[iscale-1][k];
		      
		      for (i = 0; i < sphere_no_points[k]; i++)
			{
			  xx = sphere_points[k][i][2];
			  yy = sphere_points[k][i][1];
			  zz = sphere_points[k][i][0];
			  x = col + xx;
			  y = row + yy;
			  z = slice + zz;
			  if (x >= 0 && y >= 0 && z >= 0 
			      && x < pcol && y < prow && z < pslice) 
			    {
			      for(j=0;j<num_scenes;j++)
				temp[j] = temp[j] + (int)data16[j][z*slice_size+y*pcol+x];
			      
			      count = count + 1.0;
			    }
			}
		    }
		  for(i = 0;i<num_scenes;i++)
		    filter_image16[i][slice * slice_size + row * pcol + col] = (unsigned short)((int)temp[i] / count);
		}
	    }
    }

  printf("filtering computation is done!\n");

}

/*****************************************************************************
 * FUNCTION: compute_scale
 * DESCRIPTION: Computes the scale values for the entire volume anf store in the 
 *        scale-image array.
 * PARAMETERS: None
 * SIDE EFFECTS: 
 * FUNCTIONS CALEED: None
 * ENTRY CONDITIONS: 1) scale_map array is alloted and 
 *           proper values are assigned
 * RETURN VALUE: None
 * EXIT CONDITIONS: Compute scale values
 * HISTORY:
 *  Created: 02/24/00
 *  Modified:07/25/00 extend to vectorial image by Ying Zhuge 
 *
 *****************************************************************************/
void compute_scale()
{
  int i, j, k, x, y, z, xx, yy, slice, row, col;
  int flag;
  double count_obj, count_nonobj;
  double result;
  int mean[MAX_SCENES],temp[MAX_SCENES];
  double mask_f[MAX_SCENES],std_homo[MAX_SCENES];

  if(scale_image == NULL)
    {
      scale_image = (unsigned char*)malloc(volume_size*sizeof(char));
      assert(scale_image != NULL);
    }
  memset(scale_image,0,volume_size*sizeof(char));
  
  for(j = 0;j<num_scenes;j++)
    std_homo[j] = homo_cov[j][j]*homo_cov[j][j];
  
  if (num_of_bits == 8)
    { 
      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)  
	  {
	    if((bgsuppression_flag)&&(data8[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	      scale_image[slice * slice_size + row * pcol + col] = 1;
	    else
	      {
		flag = 0;
		//------------------------------mean filter----------------------
		for(i=0;i<num_scenes;i++)
		  mask_f[i] = 0;
	        
		for (yy = -FILTER; yy <= FILTER; yy++)
		  for (xx = -FILTER; xx <= FILTER; xx++)
		    {
		      x = xx + col;
		      y = yy + row;
		      z = slice;
		      for(i=0;i<num_scenes;i++)
			if (x >= 0 && y >= 0  
			    && x < pcol && y < prow)
			  mask_f[i] = mask_f[i] + mask[yy + FILTER][xx + FILTER] 
			    * (double) data8[i][z * slice_size + y * pcol + x];
			else
			  mask_f[i] = mask_f[i] + mask[yy + FILTER][xx + FILTER] 
			    * (double) data8[i][slice * slice_size + row * pcol + col];
		    }
		for(i=0;i<num_scenes;i++)
		  mean[i] = (int) (mask_f[i] / mask_total + 0.5);
		//---------------------------------------------------------------

		for (k = 1; k < SCALE && !flag; k++)
		  {
		    count_obj = 0;
		    count_nonobj = 0;
		    for (i = 0; i < sphere_no_points[k]; i++) 
		      {
			x = col + sphere_points[k][i][2];
			y = row + sphere_points[k][i][1];
			z = slice + sphere_points[k][i][0];
			if (x < 0 || x >= pcol)
			  x = col;
			if (y < 0 || y >= prow)
			  y = row;
			if (z < 0 || z >= pslice)
			  z = slice;

			for(j=0;j<num_scenes;j++)
			  temp[j] = abs( data8[j][z*slice_size + y*pcol + x] - mean[j]);

			/************components are assumed independently*********/
			result = 0;
                        for(j=0;j<num_scenes;j++)
			  result = result + (temp[j]*temp[j])/std_homo[j];
                        /****************************************************************/
			count_obj = count_obj + exp(-0.5*result/9.0);
			count_nonobj = count_nonobj + 1.0 - exp(-0.5*result/9.0) ;
		      }
		    
		    if (100.0 * count_nonobj >= tolerance * (count_nonobj + count_obj))
		      {
			scale_image[slice * slice_size + row * pcol + col] = k;
			flag = 1;
		      }
		  }
		if (!flag)
		  scale_image[slice * slice_size + row * pcol + col] = k;
	      }
	  }
  }
  else if (num_of_bits == 16)
    { 
      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)  
	  {
	    // if((bgsuppression_flag)&&(data16[0][slice*slice_size + row*pcol + col]<feature_mean[0]))
	    if(data16[0][slice*slice_size + row*pcol + col]<feature_mean[0])
	      scale_image[slice * slice_size + row * pcol + col] = 1;
	    else
	      {
		flag = 0;
		//------------------------------mean filter----------------------
		for(i=0;i<num_scenes;i++)
		  mask_f[i] = 0;
	        
		for (yy = -FILTER; yy <= FILTER; yy++)
		  for (xx = -FILTER; xx <= FILTER; xx++)
		    {
		      x = xx + col;
		      y = yy + row;
		      z = slice;
		      for(i=0;i<num_scenes;i++)
			if (x >= 0 && y >= 0  
			    && x < pcol && y < prow)
			  mask_f[i] = mask_f[i] + mask[yy + FILTER][xx + FILTER] 
			    * (double) data16[i][z * slice_size + y * pcol + x];
			else
			  mask_f[i] = mask_f[i] + mask[yy + FILTER][xx + FILTER] 
			    * (double) data16[i][slice * slice_size + row * pcol + col];
		    }
		for(i=0;i<num_scenes;i++)
		  mean[i] = (int) (mask_f[i] / mask_total + 0.5);
		//---------------------------------------------------------------
		
		for (k = 1; k < SCALE && !flag; k++)
		  {
		    count_obj = 0;
		    count_nonobj = 0;
		    for (i = 0; i < sphere_no_points[k]; i++) 
		      {
			x = col + sphere_points[k][i][2];
			y = row + sphere_points[k][i][1];
			z = slice + sphere_points[k][i][0];
			if (x < 0 || x >= pcol)
			  x = col;
			if (y < 0 || y >= prow)
			  y = row;
			if (z < 0 || z >= pslice)
			  z = slice;

			for(j=0;j<num_scenes;j++)
			  temp[j] = abs( data16[j][z*slice_size + y*pcol + x] - mean[j]);

			/************components are assumed independently*********/
			result = 0;
                        for(j=0;j<num_scenes;j++)
			  result = result + (temp[j]*temp[j])/std_homo[j];
                        /****************************************************************/
			count_obj = count_obj + exp(-0.5*result/9.0);
			count_nonobj = count_nonobj + 1.0 - exp(-0.5*result/9.0) ;
		      }
		    
		    if (100.0 * count_nonobj >= tolerance * (count_nonobj + count_obj))
		      {
			scale_image[slice * slice_size + row * pcol + col] = k;
			flag = 1;
		      }
		  }
		if (!flag)
		  scale_image[slice * slice_size + row * pcol + col] = k;
	      }
	  }
    }
  printf("\rScale computation is done.     \n"); fflush(stdout);	
}



/*****************************************************************************
   * FUNCTION: get_max
   * DESCRIPTION: returns the maximum between two integers
   * PARAMETERS:
   *  integer1 and integer2
   * SIDE EFFECTS:
   *  Nil.
   * ENTRY CONDITIONS:
   *  Nil.
   * RETURN VALUE: see DESCRIPTION
   * EXIT CONDITIONS:
   * HISTORY:
   *  Created: 9/5/97 by Punam K. Saha
   *
   *****************************************************************************/
int get_max(int a, int b)
{

  if (a > b)
	return (a);
  else
	return (b);
}

/*****************************************************************************
 * FUNCTION: get_min
 * DESCRIPTION: returns the minimum between two integers
 * PARAMETERS:
 *  integer1 and integer2
 * SIDE EFFECTS:
 *  Nil.
 * ENTRY CONDITIONS:
 *  Nil.
 * RETURN VALUE: see DESCRIPTION
 * EXIT CONDITIONS:
 * HISTORY:
 *  Created: 9/5/97 by Punam K. Saha
 *
 *****************************************************************************/
int get_min(int a, int b)
{

  if (a < b)
	return (a);
  else
	return (b);
}

/*****************************************************************************
 * FUNCTION: handle_error
 * DESCRIPTION: Displays an error message on stderr and exits with code 1
 *    on serious errors.
 * PARAMETERS:
 *    error_code: One of the 3DVIEWNIX error codes.
 * SIDE EFFECTS: If bg_flag is set, job_done will be run and
 *    VDeleteBackgroundProcessInformation called.
 * ENTRY CONDITIONS: The variables argv0, bg_flag must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Returns on codes 0, 106, 107.
 * HISTORY:
 *    Created: 3/26/96 by Dewey Odhner
 *
 *****************************************************************************/
void handle_error(int error_code)
{
  char msg[200];

  switch (error_code)
	{
	case 0:
	case 106:
	case 107:
	  return;
	default:
	  VDecodeError(argv0, "handle_error", error_code, msg);
	  fprintf(stderr, "%s\n", msg);
	  exit(1);
	}
}

/****************INCLUDE FOR FAST TRACKING******************/

/*****************************************************************************
 * FUNCTION: push_xyz_chash
 * DESCRIPTION: Stores a voxel in a hashed heap structure with one
 *    bin per value.
 * PARAMETERS:
 *    x, y, z: Coordinates of the voxel.
 *    w: Value of the voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be set.
 *    H must be returned by chash_create.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *    Modified: 2/1/99 returns 1 on failure by Dewey Odhner
 *
 *****************************************************************************/
int
push_xyz_chash(short x, short y, short z, unsigned short w)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  return chash_push(H, (void *)&tmp);
}

/*****************************************************************************
 * FUNCTION: repush_xyz_chash
 * DESCRIPTION: Updates a voxel in a hashed heap structure with one
 *    bin per value.
 * PARAMETERS:
 *    x, y, z: Coordinates of the voxel.
 *    w: New value of the voxel.
 *    ow: Old value of the voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be set.
 *    H must be returned by chash_create.
 * RETURN VALUE: Voxel index into the volume
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *
 *****************************************************************************/
void repush_xyz_chash(short x, short y, short z, unsigned short w, unsigned short ow)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  chash_repush(H, (void *)&tmp, ow);
}

/*****************************************************************************
 * FUNCTION: pop_xyz_chash
 * DESCRIPTION: Retrieves a voxel from a hashed heap structure with one
 *    bin per value.
 * PARAMETERS:
 *    x, y, z: Coordinates of the popped voxel go here.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables H, dimensions must be set.
 *    H must be returned by chash_create.
 * RETURN VALUE: Voxel index into the volume
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *
 *****************************************************************************/
long
pop_xyz_chash(short *x, short *y, short *z)
{
  VoxelWithValue tmp;
  long kkk;

  chash_pop(H, (void *)&tmp);
  *x = tmp.x;
  *y = tmp.y;
  *z = tmp.z;
  kkk = tmp.z * slice_size + tmp.y * pcol + tmp.x;
  return kkk;
}

/*****************************************************************************
 * FUNCTION: push_xyz_chash2
 * DESCRIPTION: Stores a voxel in a hashed heap structure with one
 *    bin per value and doubly linked lists.
 * PARAMETERS:
 *    x, y, z: Coordinates of the voxel.
 *    w: Value of the voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be returned by chash_create2.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
int
push_xyz_chash2(short x, short y, short z, unsigned short w)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  return chash_push2(H, (void *)&tmp);
}

/*****************************************************************************
 * FUNCTION: repush_xyz_chash2
 * DESCRIPTION: Updates a voxel in a hashed heap structure with one
 *    bin per value and doubly linked lists.
 * PARAMETERS:
 *    x, y, z: Coordinates of the voxel.
 *    w: New value of the voxel.
 *    ow: Old value of the voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be returned by chash_create2.
 * RETURN VALUE: Voxel index into the volume
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
void repush_xyz_chash2(short x, short y, short z, unsigned short w, unsigned short ow)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  chash_repush2(H, (void *)&tmp, ow);
}

/*****************************************************************************
 * FUNCTION: pop_xyz_chash2
 * DESCRIPTION: Retrieves a voxel from a hashed heap structure with one
 *    bin per value and doubly linked lists.
 * PARAMETERS:
 *    x, y, z: Coordinates of the popped voxel go here.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables H, dimensions must be set.
 *    H must be returned by chash_create2.
 * RETURN VALUE: Voxel index into the volume
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
long
pop_xyz_chash2(short *x, short *y, short *z)
{
  VoxelWithValue tmp;
  long kkk;

  chash_pop2(H, (void *)&tmp);
  *x = tmp.x;
  *y = tmp.y;
  *z = tmp.z;
  kkk = tmp.z * slice_size + tmp.y * pcol + tmp.x;
  return kkk;
}

/*****************************************************************************
 * FUNCTION: voxel_value
 * DESCRIPTION: Returns the value of a voxel.
 * PARAMETERS:
 *    v: Pointer to the voxel
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Value of the voxel.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/1/99 by Dewey Odhner
 *
 *****************************************************************************/
unsigned short voxel_value(void *v)
{
	return ((VoxelWithValue *)v)->val;
}

/*****************************************************************************
 * FUNCTION: hheap_hash_0
 * DESCRIPTION: Returns a hash value for a voxel.
 * PARAMETERS:
 *    hashsize: The number of hash bins
 *    v: The voxel
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable dimensions must be set.
 * RETURN VALUE: Hash value for the voxel.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
long
hheap_hash_0(long hashsize, void *v)
{
  VoxelWithValue *V=(VoxelWithValue *)v;
  return (V->z * slice_size + V->y * pcol + V->x) % hashsize;
}

/*****************************************************************************
 * FUNCTION: push_xyz_hheap
 * DESCRIPTION: Stores a voxel in a hashed heap structure.
 * PARAMETERS:
 *    x, y, z: Coordinates of the voxel.
 *    w: Value of the voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be set.  H must be of type Hheap *.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns 1 on failure by Dewey Odhner
 *
 *****************************************************************************/
int
push_xyz_hheap(short x, short y, short z, unsigned short w)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  return hheap_push(H, (void *)&tmp);
}

/*****************************************************************************
 * FUNCTION: repush_xyz_hheap
 * DESCRIPTION: Updates a voxel in a hashed heap structure.
 * PARAMETERS:
 *    x, y, z: Coordinates of the voxel.
 *    w: New value of the voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be set.  H must be of type Hheap *.
 * RETURN VALUE: Voxel index into the volume
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
void repush_xyz_hheap(short x, short y, short z, unsigned short w)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  hheap_repush(H, (void *)&tmp);
}

/*****************************************************************************
 * FUNCTION: pop_xyz_hheap
 * DESCRIPTION: Retrieves a voxel from a hashed heap structure.
 * PARAMETERS:
 *    x, y, z: Coordinates of the popped voxel go here.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables H, dimensions must be set.  H must be of
 *    type Hheap *.
 * RETURN VALUE: Voxel index into the volume
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
long
pop_xyz_hheap(short *x, short *y, short *z)
{
  VoxelWithValue tmp;
  long kkk;

  hheap_pop(H, (void *)&tmp);
  *x = tmp.x;
  *y = tmp.y;
  *z = tmp.z;
  kkk = tmp.z * slice_size + tmp.y * pcol + tmp.x;
  return kkk;
}

/*****************************************************************************
 * FUNCTION: voxel_value_cmp
 * DESCRIPTION: Returns relative ordering of two voxels by value
 * PARAMETERS:
 *    v, vv: Pointers to the voxels.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: -1, 0, or 1
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/27/99 by Dewey Odhner
 *
 *****************************************************************************/
int voxel_value_cmp(void *v, void *vv)
{
	VoxelWithValue *V=(VoxelWithValue *)v, *VV=(VoxelWithValue *)vv;
	return V->val>VV->val? 1: V->val<VV->val? -1: 0;
}

/*****************************************************************************
 * FUNCTION: voxel_cmp
 * DESCRIPTION: Returns relative ordering of two voxels in a volume
 * PARAMETERS:
 *    v, vv: Pointers to the voxels.  A null pointer is smaller than any voxel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: -1, 0, or 1
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/20/99 by Dewey Odhner
 *
 *****************************************************************************/
int voxel_cmp(void *v, void *vv)
{
	VoxelWithValue *V=(VoxelWithValue *)v, *VV=(VoxelWithValue *)vv;
	if (V==NULL && VV==NULL)
		return 0;
	if (V == NULL)
		return -1;
	if (VV == NULL)
		return 1;
	if (V->x > VV->x)
		return 1;
	if (V->x < VV->x)
		return -1;
	if (V->y > VV->y)
		return 1;
	if (V->y < VV->y)
		return -1;
	if (V->z > VV->z)
		return 1;
	if (V->z < VV->z)
		return -1;
	return 0;
}

#define Push_xyz push_xyz_chash
#define Repush_xyz(x, y, z, w, ow) repush_xyz_chash((x), (y), (z), (w), (ow))
#define Pop_xyz pop_xyz_chash
#define H_is_empty chash_isempty(H)
#define fuzzy_track fuzzy_track_chash
#define Create_heap chash_create(CONN+1L, sizeof(VoxelWithValue), \
	voxel_value, NULL)

#include "track_rel_using.c"

#undef Push_xyz
#undef Repush_xyz
#undef Pop_xyz
#undef fuzzy_track
#undef Create_heap

#define Push_xyz push_xyz_chash2
#define Repush_xyz(x, y, z, w, ow) repush_xyz_chash2((x), (y), (z), (w), (ow))
#define Pop_xyz pop_xyz_chash2
#define fuzzy_track fuzzy_track_chash2
#define Create_heap chash_create2(CONN+1L, sizeof(VoxelWithValue),\
	voxel_value, NULL)

#include "track_rel_using.c"

#undef Push_xyz
#undef Repush_xyz
#undef Pop_xyz
#undef H_is_empty
#undef fuzzy_track
#undef Create_heap

#define Push_xyz push_xyz_hheap
#define Repush_xyz(x, y, z, w, ow) repush_xyz_hheap((x), (y), (z), (w))
#define Pop_xyz pop_xyz_hheap
#define H_is_empty hheap_isempty(H)
#define fuzzy_track fuzzy_track_hheap
#define Create_heap hheap_create(8999L, sizeof(VoxelWithValue), hheap_hash_0, \
	voxel_value_cmp, voxel_cmp)

#include "track_rel_using.c"

/***********************************************************/

/*******************************  THE END  ***************************/
