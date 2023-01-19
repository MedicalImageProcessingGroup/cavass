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

#include <math.h>
#include <assert.h>
#include <string.h>
#include <cv3dv.h>
#include <stdlib.h>

// 
// combine the gbscale2_std.c and gbscale2_train scripts 
// Xinjian Chen Ph.D.  2008.10.1
//
//

void compute_scale();

int temp_scale=0;
int height,width,slices,space,num_of_bits,slice_size,volume_size,size_bin;
char *dt_8;

// ---------------------------------------------------------------------------------
#define FEATURES 1
#define STOP_CONDITION 0.0 //0.001

int number_largest_object,num_pre=0;
char *b_file[FEATURES], *m_file[FEATURES];
int feature_thr[FEATURES];
unsigned char **data_n8;    // for multi-features data
unsigned short **data_n16;

/**************************************for compute scale*******************************/
#define FILTER 1
#define SCALE 12
#define tolerance 13.0
float BKND_FACTOR = 0.5; // controls which spels are foreground and which bknd.

int  mean_density_value;
int  background = 0;
float *scale_map;
unsigned char *scale_image;

int SCALE_THRESH = 3;
float HIST_THRESHOLD = (float)0.8;
int NUM_REGIONS_PAINT = 20; /* Warning: num_largest_regions > 25 => (-) values assigned */

int pcol,prow,pslice;
int *sphere_no_points;
short (**sphere_points)[3];
int *insphere_no_points;
short (**insphere_points)[3];

double mask[2*FILTER+1][2*FILTER+1][2*FILTER+1], mask_total;
int pow_value[FEATURES];
int diff_value_max[FEATURES];
unsigned char *data_scale8;      
unsigned short *data_scale16; 

/*****************************************************************************/
char *datafile[FEATURES];
int Way=0,times=10;
float NN1=1.0,NN2=1.0;
int METHOD=0,ii_temp=0;

/********************************************************************************************
 * FUNCTION: compute_scale_image
 * DESCRIPTION: compute the scale of the input image
 * PARAMETERS: original image, scale image, constant, background
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 * 
 * 
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 07/07/00 by Ying Zhuge
 *    Modified: 2/18/05 unsigned values cast to int for subtraction
 *           by Dewey Odhner.
 *
 *******************************************************************************************/
int compute_scale_image(char *o_file, double a, int b)
{
  int i,j,k,l, error_code, count=0, **histogram, hist_sum[FEATURES];
  int tti1, tti2,tti3;
  int x,y,z,xx,yy,zz,xxx,yyy,zzz,largest_density_value, ***ppptti1, ***ppptti2;
  int slice, row, col;
  int x1=0,x2=0,y1=0,y2=0,z1=0,z2=0;
  double tt1,tt2;
  unsigned long *que;//, *gbscale_image;
  unsigned long *gbscale_image;
  
  static ViewnixHeader vh_in;
  char group[6],element[6];
  double anisotropy_col,anisotropy_row,anisotropy_slice,homogeneity_sigma,inv_scale_sigma,sigma_constant = 1;
  double sigma_mean,sigma_std,sigma_sum;
  FILE *fp_in;

  unsigned short max_percentile_int=0;
  unsigned int temp_int;
  int num_largest_regions;  
  int mean_region_intensity=0, maxint=0,modeint=0,max_mode=0,tmp_xxx = 0,largest_value=0;
  int iii,jjj,kkk;
  int t1;
  /*unsigned long  LARGEST_REGION[NUM_REGIONS_PAINT], SIZE_REGION[NUM_REGIONS_PAINT];
  unsigned long MEAN_INT_REGION[NUM_REGIONS_PAINT], MAX_INT_REGION[NUM_REGIONS_PAINT];
  unsigned long MODE_INT_REGION[NUM_REGIONS_PAINT], MED_INT_REGION[NUM_REGIONS_PAINT];
  unsigned long SCALED_MED_REGION[NUM_REGIONS_PAINT];
  float SLOPE1_REGION[NUM_REGIONS_PAINT], SLOPE2_REGION[NUM_REGIONS_PAINT];*/
  unsigned long  *LARGEST_REGION, *SIZE_REGION;   // // xinjian chen 2008.9.23
  unsigned long *MEAN_INT_REGION, *MAX_INT_REGION;
  unsigned long *MODE_INT_REGION, *MED_INT_REGION;
  unsigned long *SCALED_MED_REGION;
  float *SLOPE1_REGION, *SLOPE2_REGION;
  unsigned long *histogram_gscale_regions, *histogram_int, *int_que; 
  unsigned long num_vox_region=0,label=0,nn=0,voxelcount=0,sum_voxels=0;
  unsigned long min, max, intensity, SCALE_MAX, ttti1, aaa, pppi1, pointer_pos = 0;
  float *Hist;
  int   retVal;

  LARGEST_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( LARGEST_REGION == NULL )	return -1;  
  SIZE_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( SIZE_REGION == NULL ) return -1;  
  MEAN_INT_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( MEAN_INT_REGION == NULL ) return -1;  
  MAX_INT_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( MAX_INT_REGION == NULL ) return -1;  
  MODE_INT_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( MODE_INT_REGION == NULL ) return -1;  
  MED_INT_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( MED_INT_REGION == NULL ) return -1;  
  SCALED_MED_REGION = (unsigned long*)malloc(NUM_REGIONS_PAINT*sizeof(unsigned long));
  if( SCALED_MED_REGION == NULL ) return -1;
  
  SLOPE1_REGION = (float*)malloc(NUM_REGIONS_PAINT*sizeof(float));
  if( SLOPE1_REGION == NULL ) return -1;
  SLOPE2_REGION = (float*)malloc(NUM_REGIONS_PAINT*sizeof(float));
  if( SLOPE2_REGION == NULL ) return -1;
 

  sigma_constant=a;
  background=b;  

  fp_in = fopen(o_file, "rb");
  if(fp_in == NULL)
	{
	  printf("Cannot open file %s \n",o_file);
	  exit(-1);
	}
  error_code = VReadHeader(fp_in, &vh_in, group, element);
  switch (error_code)
    {
    case 0:
    case 106:
    case 107:
      break;
    default:
      fprintf(stderr, "file = %s; group = %s; element = %s\n", o_file, group, element);
	  exit(1);
    }
  
  pcol = vh_in.scn.xysize[0];
  prow = vh_in.scn.xysize[1];
  pslice = vh_in.scn.num_of_subscenes[0];
  slice_size = pcol * prow;
  volume_size = slice_size * pslice;
  num_of_bits = vh_in.scn.num_of_bits;
  
  gbscale_image = (unsigned long *) malloc(volume_size * sizeof(unsigned long));
  que = (unsigned long *) malloc(volume_size * sizeof(unsigned long));
    
  // ***************************************************************************
  // Initializing the g-scale (scale_image)
  
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)
	{
	  gbscale_image[slice * slice_size + row * pcol + col] = 0;
	}
  
  //*****************************************************************************
  tti1 = 2 * (SCALE + 5);
  ppptti1 = (int ***) malloc(tti1 * sizeof(int **));
    
  ppptti1[0] = (int **) malloc(tti1 * tti1 * sizeof(int *));
    
  for (i = 0; i < tti1; i++)
    ppptti1[i] = ppptti1[0] + i * tti1;
  ppptti1[0][0] = (int *) malloc(tti1 * tti1 * tti1 * sizeof(int));
  
  for (i = 0; i < tti1; i++)
    for (j = 0; j < tti1; j++)
      ppptti1[i][j] = ppptti1[0][0] + (i * tti1 + j) * tti1;
  
  for (i = 0; i < tti1; i++)
    for (j = 0; j < tti1; j++)
      for (k = 0; k < tti1; k++)
	ppptti1[i][j][k] = 0;
  
  ppptti2 = (int ***) malloc(tti1 * sizeof(int **));
    
  ppptti2[0] = (int **) malloc(tti1 * tti1 * sizeof(int *));
    
  for (i = 0; i < tti1; i++)
    ppptti2[i] = ppptti2[0] + i * tti1;
  ppptti2[0][0] = (int *) malloc(tti1 * tti1 * tti1 * sizeof(int));
  
  for (i = 0; i < tti1; i++)
    for (j = 0; j < tti1; j++)
      ppptti2[i][j] = ppptti2[0][0] + (i * tti1 + j) * tti1;
  
  for (i = 0; i < tti1; i++)
    for (j = 0; j < tti1; j++)
      for (k = 0; k < tti1; k++)
	ppptti2[i][j][k] = 0;

  sphere_no_points = (int *) malloc((SCALE + 1) * sizeof(int));
  sphere_points = (void *) malloc((SCALE + 1) * sizeof(void *));
  insphere_no_points = (int *) malloc((SCALE + 1) * sizeof(int));
  insphere_points = (void *) malloc((SCALE + 1) * sizeof(void *));

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
    {
      for (k = 0; k <= SCALE; k++)
	{
	  insphere_no_points[k] = 0;
	  sphere_no_points[k] = 0;
	  for (i = -k -2; i <= k + 2; i++) 
	    for (j = -k - 2; j <= k + 2; j++)
	      for (l = -k - 2; l <= k + 2; l++)
		{
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
		      else if (((double) k) > (tt1 + 0.5))
			{		   
			  insphere_no_points[k] = insphere_no_points[k] + 1;
			  ppptti2[tti1 + i][tti1 + j][tti1 + l] = 1;
			}
		    }
		  else
		    {
		      insphere_no_points[k] = insphere_no_points[k] + 1;
		      ppptti2[tti1 + i][tti1 + j][tti1 + l] = 1;
		    }
		}
	  
	  sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));
	  insphere_points[k] = (void *) malloc(3 * insphere_no_points[k] * sizeof(int));

	  if (sphere_points[k] == NULL)
	    {
	      printf("Couldn't allocate memory (execution terminated)\n");
	      exit(-1);
	    }
	  
	  tti3 = 0;
	  tti2 = 0;
	  for (i = -k -2; i <= k + 2; i++)
	    for (j = -k - 2; j <= k + 2; j++)
	      for (l = -k - 2; l <= k + 2; l++)
		{
		  if (ppptti2[tti1 + i][tti1 + j][tti1 + l] == 1)
		    {
		      insphere_points[k][tti3][0] = i;
		      insphere_points[k][tti3][1] = j;
		      insphere_points[k][tti3][2] = l;
		      tti3 = tti3 + 1;
		    }
		  
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
    }
  else
    {
      for (k = 0; k <= SCALE; k++)
	{
	  sphere_no_points[k] = 0;
	  insphere_no_points[k] = 0;
	  for (j = -k - 2; j <= k + 2; j++)
	    for (l = -k - 2; l <= k + 2; l++)
	      {
	      if (ppptti1[tti1][tti1 + j][tti1 + l] == 0)
		{
		  tt1 = sqrt(pow(((double) j) * anisotropy_row, 2.0)
			     + pow(((double) l) * anisotropy_col, 2.0));
		  
		  if (tt1 <= ((double) k) + 0.5)
		    {
		      sphere_no_points[k] = sphere_no_points[k] + 1;
		      ppptti1[tti1][tti1 + j][tti1 + l] = 2;
		    }
		  else if (((double) k) > (tt1 + 0.5))
		    {		   
		      insphere_no_points[k] = insphere_no_points[k] + 1;
		      ppptti2[tti1][tti1 + j][tti1 + l] = 1;
		    }
		}
	      else
		{
		  insphere_no_points[k] = insphere_no_points[k] + 1;
		  ppptti2[tti1][tti1 + j][tti1 + l] = 1;
		}
	      }

	  sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));
	  insphere_points[k] = (void *) malloc(3 * insphere_no_points[k] * sizeof(int));
	  
	  if (sphere_points[k] == NULL)
	    {
	      printf("Couldn't allocate memory (execution terminated)\n");
	      exit(-1);
	    }
	  tti2 = 0;
	  tti3 = 0;
	  for (j = -k - 2; j <= k + 2; j++)
	    for (l = -k - 2; l <= k + 2; l++)
	      {
		if (ppptti2[tti1][tti1 + j][tti1 + l] == 1)
		  {
		    insphere_points[k][tti3][0] = 0;
		    insphere_points[k][tti3][1] = j;
		    insphere_points[k][tti3][2] = l;
		    tti3 = tti3 + 1;
		  }
		
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
    }

  fflush(stdout);
  free(ppptti1[0][0]);
  free(ppptti1[0]);
  free(ppptti1);
  
  
  mask_total = 0.0;
  for (zz = -FILTER; zz <=FILTER; zz++)
    for (yy = -FILTER; yy <= FILTER; yy++)
      for (xx = -FILTER; xx <= FILTER; xx++)
	mask[zz + FILTER][yy + FILTER][xx + FILTER] = 0;
  if (pslice == 1)
    {
      for (zz = 0; zz <= 0; zz++)
	for (yy = -FILTER; yy <= FILTER; yy++)
	  for (xx = -FILTER; xx <= FILTER; xx++)
	    {
	      tt2 = pow(anisotropy_col * xx, 2.0);
	      tt2 = tt2 + pow(anisotropy_row * yy, 2.0);
	      tt2 = 1 / (1 + tt2);
	      mask[zz + FILTER][yy + FILTER][xx + FILTER] = tt2;
	      mask_total = mask_total + tt2;
	    }
    }
  else
    {
      for (zz = -FILTER; zz <= FILTER; zz++)
	for (yy = -FILTER; yy <= FILTER; yy++)
	  for (xx = -FILTER; xx <= FILTER; xx++)
	    {
	      tt2 = pow(anisotropy_col * xx, 2.0);
	      tt2 = tt2 + pow(anisotropy_row * yy, 2.0);
	      tt2 = tt2 + pow(anisotropy_slice * zz, 2.0);
	      tt2 = 1 / (1 + tt2);
	      mask[zz + FILTER][yy + FILTER][xx + FILTER] = tt2;
	      mask_total = mask_total + tt2;
	    }
    }
  
 
  //**************************************************************************************

  if(FEATURES == 1)
    {
      if(num_of_bits == 8)
	data_scale8 = (unsigned char *)malloc(volume_size*sizeof(char));
      else
	data_scale16 = (unsigned short *)malloc(volume_size*sizeof(short));	

      if(data_scale8 == NULL && data_scale16 == NULL)
	{
	  printf("Memory allocation error \n");
	  exit(-1);
	}
      VSeekData(fp_in, 0);
      if(num_of_bits == 8)
	error_code = VReadData((char *)data_scale8, num_of_bits/8, volume_size, fp_in, &j);
      else
	error_code = VReadData((char *)data_scale16, num_of_bits/8, volume_size, fp_in, &j);
      
      switch (error_code)
	{
	case 0:
	case 106:
	case 107:
	  break;
	default:
	  fprintf(stderr, "Failure reading data.\n");
	  exit(1);
	}

      tt1 = 0;
      largest_density_value = 0;
      for(z = 0;z<pslice;z++)
	for(y = 0;y<prow;y++)
	  for(x = 0;x<pcol;x++)
	    if(num_of_bits == 8)
	      {
		tt1 = tt1 + data_scale8[z*slice_size + y*pcol +x];
		if(data_scale8[z*slice_size + y*pcol +x] > largest_density_value)
		  largest_density_value = data_scale8[z*slice_size + y*pcol +x];
	      }
	    else if(num_of_bits == 16)
	      {
		tt1 = tt1 + data_scale16[z*slice_size + y*pcol +x];
		if(data_scale16[z*slice_size + y*pcol +x] > largest_density_value)
		  largest_density_value = data_scale16[z*slice_size + y*pcol +x];
	      }
      mean_density_value = (int)(tt1/volume_size);
      //printf("mean_density_value: %d\n",mean_density_value);

  /**********************************************************************************/
  /*----to compute the histogram for each feature and the threshold for true edge---*/

      histogram = (int **)malloc(FEATURES*sizeof(int *));
      histogram[0] = (int *)malloc(FEATURES*(largest_density_value+1)*sizeof(int));
      if(histogram == NULL || histogram[0] == NULL)
	printf("Memory allocation error \n");
      
      for(j=0;j<=largest_density_value;j++)
	histogram[0][j] = 0;

      for (i=0;i<pslice;i++)
	for (j=0;j<prow;j++)
	  for (k=0;k<pcol-1;k++)
	    {
	      xx = k+1;
	      yy = j;
	      zz = i;
	      
	      if(num_of_bits == 8)
		tti1 = abs((int)data_scale8[i*slice_size+j*pcol+k] - (int)data_scale8[zz*slice_size+yy*pcol+xx]);
	      else if(num_of_bits == 16)
		tti1 = abs((int)data_scale16[i*slice_size+j*pcol+k] - (int)data_scale16[zz*slice_size+yy*pcol+xx]);
	      histogram[0][tti1]++;
	    }
          
      for (i=0;i<pslice;i++)
	for (j=0;j<prow-1;j++)
	  for (k=0;k<pcol;k++)
	    {
	      xx = k;
	      yy = j+1;
	      zz = i;

	      if(num_of_bits == 8)
		tti1 = abs((int)data_scale8[i*slice_size+j*pcol+k] - (int)data_scale8[zz*slice_size+yy*pcol+xx]);
	      else if(num_of_bits == 16)
		tti1 = abs((int)data_scale16[i*slice_size+j*pcol+k] - (int)data_scale16[zz*slice_size+yy*pcol+xx]);
	      histogram[0][tti1]++;
	    }
      
      for (i=0;i<pslice-1;i++)
	for(j=0;j<prow;j++)
	  for (k=0;k<pcol;k++)
	    {
	      xx = k;
	      yy = j;
	      zz = i+1;

	      if(num_of_bits == 8)
		tti1 = abs((int)data_scale8[i*slice_size+j*pcol+k] - (int)data_scale8[zz*slice_size+yy*pcol+xx]);
	      else if(num_of_bits == 16)
		tti1 = abs((int)data_scale16[i*slice_size+j*pcol+k] - (int)data_scale16[zz*slice_size+yy*pcol+xx]);
	      histogram[0][tti1]++;

	    }
      
      hist_sum[0] = 0;
      for(j=0;j<largest_density_value;j++)
	hist_sum[0] = hist_sum[0] + histogram[0][j];
      
      for(j=0;j<largest_density_value;j++)
	{
	  tti1 = 0;
	  feature_thr[0] = j;
	  for(k=0;k<=j;k++)
	    tti1 = tti1+histogram[0][k];
	  if (((double)tti1 /(double) hist_sum[0])>=HIST_THRESHOLD)
	    break;
	}

      //printf("Histogram threshold computation is done \n");
      //printf("Features Threshold %d : %f \n", i,(double)feature_thr[0]); 
      
      //--------------------compute the homogeneity sigma-------------------------------
      sigma_sum = 0;
      count = 0;
      sigma_mean = 0;
	{
	  for(z = 0;z<pslice;z++)
	    for(y = 0;y<prow;y++)
	      for(x = 0;x<pcol-1;x++)
		{
		  zz = z;
		  yy = y;
		  xx = x + 1;
		  if(num_of_bits == 8)
		     tti1 = abs((int)data_scale8[z*slice_size + y*pcol +x] - (int)data_scale8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 =  abs((int)data_scale16[z*slice_size + y*pcol +x] - (int)data_scale16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr[0])
		    {
		      sigma_sum = sigma_sum + tti1;
		      count ++;
		    }
		}

	  for(z = 0;z<pslice;z++)
	    for(y = 0;y<prow-1;y++)
	      for(x = 0;x<pcol;x++)
		{
		  zz = z;
		  yy = y+1;
		  xx = x;
		  if(num_of_bits == 8)
		     tti1 = abs((int)data_scale8[z*slice_size + y*pcol +x] - (int)data_scale8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 =  abs((int)data_scale16[z*slice_size + y*pcol +x] - (int)data_scale16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr[0])
		    {
		      sigma_sum = sigma_sum + tti1;
		      count ++;
		    }
		}
	  for(z = 0;z<pslice-1;z++)
	    for(y = 0;y<prow;y++)
	      for(x = 0;x<pcol;x++)
		{
		  zz = z+1;
		  yy = y;
		  xx = x;
		  if(num_of_bits == 8)
		     tti1 = abs((int)data_scale8[z*slice_size + y*pcol +x] - (int)data_scale8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 =  abs((int)data_scale16[z*slice_size + y*pcol +x] - (int)data_scale16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr[0])
		    {
		      sigma_sum = sigma_sum + tti1;
		      count ++;
		    }
		}

	  sigma_mean = sigma_sum / count;
	}
	//printf("homogeneity_mean value is: %f \n", sigma_mean);
      //--------------------compute the homogeneity sigma-------------------------------
      sigma_sum = 0;
      count = 0;
	{
	  for(z = 0;z<pslice;z++)
	    for(y = 0;y<prow;y++)
	      for(x = 0;x<pcol-1;x++)
		{
		  zz = z;
		  yy = y;
		  xx = x + 1;
		  if(num_of_bits == 8)
		     tti1 = abs((int)data_scale8[z*slice_size + y*pcol +x] - (int)data_scale8[zz*slice_size + yy*pcol +xx]) ;
		  else if(num_of_bits == 16)
		    tti1 =  abs((int)data_scale16[z*slice_size + y*pcol +x] - (int)data_scale16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr[0])
		    {
		      sigma_sum = sigma_sum + pow((tti1-sigma_mean),2);
		      count ++;
		    }
		}

	  for(z = 0;z<pslice;z++)
	    for(y = 0;y<prow-1;y++)
	      for(x = 0;x<pcol;x++)
		{
		  zz = z;
		  yy = y+1;
		  xx = x;
		  if(num_of_bits == 8)
		     tti1 = abs((int)data_scale8[z*slice_size + y*pcol +x] - (int)data_scale8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 =  abs((int)data_scale16[z*slice_size + y*pcol +x] - (int)data_scale16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr[0])
		    {
		      sigma_sum = sigma_sum + pow((tti1 - sigma_mean),2);
		      count ++;
		    }
		}
	  for(z = 0;z<pslice-1;z++)
	    for(y = 0;y<prow;y++)
	      for(x = 0;x<pcol;x++)
		{
		  zz = z+1;
		  yy = y;
		  xx = x;
		  if(num_of_bits == 8)
		     tti1 = abs((int)data_scale8[z*slice_size + y*pcol +x] - (int)data_scale8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 =  abs((int)data_scale16[z*slice_size + y*pcol +x] - (int)data_scale16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr[0])
		    {
		      sigma_sum = sigma_sum + pow((tti1 - sigma_mean),2);
		      count ++;
		    }
		}
	}

	sigma_std = sqrt((double)sigma_sum/(double)count);
	homogeneity_sigma = sigma_constant * (sigma_mean + 3*sigma_std);
	//printf("homogeneity_sigma value: %f \n",homogeneity_sigma); 

	scale_map = (float *) malloc( (largest_density_value + 1) * sizeof(double));
	
	//------------------ GAUSSIAN ----------------------------------------------------
	inv_scale_sigma = -0.5 / pow(homogeneity_sigma, 2.0);
	for (i = 0; i <= largest_density_value; i++)
	  scale_map[i] = (float)exp(inv_scale_sigma * pow((double) i, 2.0));
    }
    fclose(fp_in);
  
  compute_scale();
  count = 0;

  // ------------- COMPUTING THE Gb-SCALE FROM BALL-SCALE --------------------------------
  // -------------------------------------------------------------------------------------
  //printf("Computing gb-scale\n");
  for (slice = 0; slice < pslice; slice++)
    for (row = 0; row < prow; row++)
      for (col = 0; col < pcol; col++)
	{
	  tti1 = scale_image[slice * slice_size + row * pcol + col];
	  if((background==0) && (scale_image[slice * slice_size + row * pcol + col]==0))
	    gbscale_image[slice * slice_size + row * pcol + col] = 0;//volume_size;
	  if(gbscale_image[slice * slice_size + row * pcol + col]==0)
	    {
	      label++;
	      nn = 1;
	      que[pointer_pos] = slice * slice_size + row * pcol + col;
	      gbscale_image[slice * slice_size + row * pcol + col] = label;
	      
	      while(pointer_pos < nn)
		{
		  zzz = que[pointer_pos]/(slice_size);
		  yyy = ((int)que[pointer_pos] - zzz*slice_size)/(pcol);
		  tmp_xxx = (int)que[pointer_pos] - zzz*slice_size;
		  xxx = (int) tmp_xxx%(pcol);
		  
		  pointer_pos++;
		  
		  x1 = xxx - 1;
		  x2 = xxx + 1;
		  y1 = yyy - 1;
		  y2 = yyy + 1;
		  z1 = zzz - 1;
		  z2 = zzz + 1;
		  
		  if (x1<0)
		    x1=0;
		  
		  if (x2 > (pcol-1))
		    x2=pcol-1;
		  
		  if (y1<0)
		    y1 = 0;
		  
		  if (y2 > (prow-1))
		    y2=prow-1;
		  
		  if (z1<0)
		    z1 = 0;
		  
		  if (z2 > (pslice-1))
		    z2 = pslice - 1;
		  
		  if (gbscale_image[zzz * slice_size + yyy * pcol + x1]==0)
		    {
		      tt1 = (int) scale_image[zzz * slice_size + yyy * pcol + x1];
		      if (tt1 >= SCALE_THRESH)
			{		
			  que[nn] = zzz * slice_size + yyy * pcol + x1;
			  gbscale_image[zzz * slice_size + yyy * pcol + x1] = label;
			  nn++;
			}
		    }
		  
		      if (gbscale_image[zzz * slice_size + yyy * pcol + x2]==0)
			{
			  tt1 = (int) scale_image[zzz * slice_size + yyy * pcol + x2];
			  if (tt1 >= SCALE_THRESH)
			    {
			      que[nn] = zzz * slice_size + yyy * pcol + x2;
			      gbscale_image[zzz * slice_size + yyy * pcol + x2] = label;
			      nn++;
			    }
			}
		      
		      if (gbscale_image[zzz * slice_size + y1 * pcol + xxx]==0)
			{
			  tt1 = (int) scale_image[zzz * slice_size + y1 * pcol + xxx];
			  if (tt1 >= SCALE_THRESH)
			    {
			      que[nn] = zzz * slice_size + y1 * pcol + xxx;
			      gbscale_image[zzz * slice_size + y1 * pcol + xxx] = label;
			      nn++;
			    }
			}
		      
		      if (gbscale_image[zzz * slice_size + y2 * pcol + xxx]==0)
			{
			  tt1 = (int) scale_image[zzz * slice_size + y2 * pcol + xxx];
			  if (tt1 >= SCALE_THRESH)
			    {
			      que[nn] = zzz * slice_size + y2 * pcol + xxx;
			      gbscale_image[zzz * slice_size + y2 * pcol + xxx] = label;
			      nn++;
			    }
			}
		      
		      if (gbscale_image[z1 * slice_size + yyy * pcol + xxx]==0)
			{
			  tt1 = (int) scale_image[z1 * slice_size + yyy * pcol + xxx];
			  if (tt1 >= SCALE_THRESH)
			    {
			      que[nn] = z1 * slice_size + yyy * pcol + xxx;
			      gbscale_image[z1 * slice_size + yyy * pcol + xxx] = label;
			      nn++;
			    }
			}
		      
		      if (gbscale_image[z2 * slice_size + yyy * pcol + xxx]==0)
			{
			  tt1 = (int) scale_image[z2 * slice_size + yyy * pcol + xxx];
			  if (tt1 >= SCALE_THRESH)
			    {
			      que[nn] = z2 * slice_size + yyy * pcol + xxx;
			      gbscale_image[z2 * slice_size + yyy * pcol + xxx] = label;
			      nn++;
			    }
			}
		      
		} // for the while loop
	      
	      
	    } //for the else
	  pointer_pos = 0;

	} // for the innermost for.

  min = 65535;max = 0;
  for(i = 0;i<pslice;i++)
    for(j = 0;j<prow;j++)
      for(k = 0;k<pcol;k++)
	{
	  tti1 = gbscale_image[i*slice_size + j*pcol + k];
	  if(tti1 < (int)min ) min = tti1;
	  if(tti1 > (int)max ) max = tti1;
	}
  
  t1 = (int) max;

  height = vh_in.scn.xysize[1];
  width = vh_in.scn.xysize[0];
  slices = vh_in.scn.num_of_subscenes[0];
  slice_size = height * width;
  volume_size = slice_size * slices;
  num_of_bits = vh_in.scn.num_of_bits;
  largest_value = (int)vh_in.scn.largest_density_value[0];

  //printf("Number of Regions %d \n",label);
  // Computing the 99.8 percentile intensity.

  Hist = (float *)malloc((largest_value + 1)*sizeof(float));
  
  for(iii = 0; iii <= largest_value; iii++)
    Hist[iii] = 0;
  
  for(iii = 0; iii < slices; iii++)
    for(jjj = 0; jjj < height; jjj++)
      for(kkk = 0; kkk < width; kkk++)
	{
	  tti1 = (unsigned short) data_scale16[iii*slice_size + jjj*width + kkk];
	  if (tti1 >= mean_density_value){
	    Hist[tti1]++;
	    voxelcount++;
	  }
	}
  
  for(iii = mean_density_value; iii <= largest_value; iii++)
    {
      sum_voxels = (unsigned long)(sum_voxels + Hist[iii]);
      if ( (float) ( (float) sum_voxels / (float) voxelcount * 100.0) >= 99.8)
	{
	  max_percentile_int = iii;
	  break;
	}  
    }
  
  // Number of g-scale regions.
  SCALE_MAX = (unsigned long) label;
  
  // Computing the largest g-scale region.
  histogram_gscale_regions = (unsigned long *) malloc((SCALE_MAX + 1) * sizeof(unsigned long));
  histogram_int = (unsigned long *) malloc((largest_value + 1) * sizeof(unsigned long));

  // Intialising the histogram vector.
  for(intensity = 0; intensity<(SCALE_MAX+1); intensity++)
    histogram_gscale_regions[intensity] = 0;
  
  for(intensity = 0; (int)intensity<(largest_value + 1); intensity++)
    histogram_int[intensity] = 0;

  // computing the histogram containing the g-scale regions.
  for(iii = 0; iii<pslice; iii++)
    for(jjj = 0; jjj<prow; jjj++)
      for(kkk = 0; kkk<pcol; kkk++)
	{ttti1 = gbscale_image[iii*slice_size + jjj*pcol + kkk];
	if(ttti1 != 0) 
	  histogram_gscale_regions[ttti1]++;
	if (num_of_bits == 8)
	  ttti1 = data_scale8[iii*slice_size + jjj*width + kkk];
	else
	  ttti1 = data_scale16[iii*slice_size + jjj*width + kkk];
	if(ttti1 != 0) 
	  histogram_int[ttti1]++;
	}

  for(num_largest_regions = 1; num_largest_regions<(NUM_REGIONS_PAINT+1); num_largest_regions++)
    {
      min=volume_size; max = 0;
      modeint = 0;
      max_mode = 0;

      for(intensity = 0; intensity<(SCALE_MAX+1); intensity++)
	{
	  ttti1 = histogram_gscale_regions[intensity];
	  if(ttti1 < min ) min = ttti1;
	  if(ttti1 > max ) max = ttti1;
	}

      for(intensity = 0; intensity<(SCALE_MAX + 1); intensity++)
	{
	  if(histogram_gscale_regions[intensity] == max) 
	    {
	      LARGEST_REGION[num_largest_regions - 1] = intensity;
	    }
	}
      
      SIZE_REGION[num_largest_regions - 1] = max;
      int_que = (unsigned long *) malloc((max) * sizeof(unsigned long));
      
      /* Computing mode, max. and median intensities of each region. */
      
      for(aaa=0;(int)aaa<volume_size;aaa++)
	{
	  if (gbscale_image[aaa]==LARGEST_REGION[num_largest_regions - 1])
	    {
	      num_vox_region++;
	      //dt_8_in[aaa] = num_largest_regions;
	      if (num_of_bits == 8)
		pppi1 = (int) data_scale8[aaa];
	      else
		pppi1 = (int) data_scale16[aaa];
	      if((int)pppi1 > maxint ) maxint = pppi1;
	      
	      mean_region_intensity = mean_region_intensity + pppi1;
	      int_que[count] = (int) pppi1;
	      count++;

	      if((int)histogram_int[pppi1] > modeint ) 
		{
		  modeint = histogram_int[pppi1];
		  max_mode = pppi1;
		}

	    }	
	  //else if ((num_largest_regions==1) && (scale_image[aaa]!=
	  //					LARGEST_REGION[num_largest_regions - 1]))
	    //dt_8_in[aaa]=0;
	}
      
      /* ---------------------------------------------------- */
      /*   Computing Median Intensity in each g-scale region. */

      for (i = 1; i < count; i++)
	{
	  temp_int = int_que[i]; j = i;
	  while(int_que[j-1] > temp_int) 
	    {
	      int_que[j] = int_que[j-1];
	      j--;
	    }
	  int_que[j] = temp_int;
	}

      /* ---------------------------------------------------- */

      MED_INT_REGION[num_largest_regions - 1] = 
	(! (count%2)) ? (int_que[count/2 - 1] + int_que[count/2])/2 : int_que[(count - 1)/2];
      MEAN_INT_REGION[num_largest_regions - 1] = mean_region_intensity/num_vox_region;
      MAX_INT_REGION[num_largest_regions - 1] = maxint;
      MODE_INT_REGION[num_largest_regions - 1] = max_mode;
      SCALED_MED_REGION[num_largest_regions - 1] = (unsigned long)(
	( (float) 4095/ (float) (max_percentile_int)) * MED_INT_REGION[num_largest_regions - 1]);

      SLOPE1_REGION[num_largest_regions - 1] = (float) ( SCALED_MED_REGION[num_largest_regions - 1])/
	(MED_INT_REGION[num_largest_regions - 1]);

      SLOPE2_REGION[num_largest_regions - 1] = (float) (4095 - SCALED_MED_REGION[num_largest_regions - 1])/
	(float) (max_percentile_int -  MED_INT_REGION[num_largest_regions - 1]);

      num_vox_region = 0;
      count = 0;
      temp_int = 0;
      mean_region_intensity = 0;
      maxint = 0;
      max_mode = 0;
      modeint = 0;
      histogram_gscale_regions[LARGEST_REGION[num_largest_regions - 1]] = 0;
    }

  /* ---------------------------------------------------------- */
  /*     Printing out statistics for each g-scale region        */
  
  for(num_largest_regions = 1; num_largest_regions<(NUM_REGIONS_PAINT+1); num_largest_regions++)
    {
      printf("%d\n", (int)SCALED_MED_REGION[num_largest_regions - 1]);
	  retVal = SCALED_MED_REGION[num_largest_regions - 1];
    }

  /*****************************************************************************/
 
  if(FEATURES == 1)
    {
      if(num_of_bits == 8)
	free(data_scale8);
      else if(num_of_bits == 16)
	free(data_scale16);
    }
  free(histogram[0]);
  free(histogram);
  free(scale_image);

  free(gbscale_image);
  
  free(sphere_no_points);
  for(i=0;i<=SCALE;i++)
    free(sphere_points[i]);
  free(sphere_points);

  free(insphere_no_points);
  for(i=0;i<=SCALE;i++)
    free(insphere_points[i]);
  free(insphere_points);

   if( LARGEST_REGION!= NULL )  free( LARGEST_REGION );
  if( SIZE_REGION!= NULL )  free( SIZE_REGION );
  if( MEAN_INT_REGION!= NULL )  free( MEAN_INT_REGION );
  if( MAX_INT_REGION!= NULL )  free( MAX_INT_REGION );
  if( MODE_INT_REGION!= NULL )  free( MODE_INT_REGION );
  if( MED_INT_REGION!= NULL )  free( MED_INT_REGION );
  if( SCALED_MED_REGION!= NULL )  free( SCALED_MED_REGION );
  if( SLOPE1_REGION!= NULL )  free( SLOPE1_REGION );
  if( SLOPE2_REGION!= NULL )  free( SLOPE2_REGION );  

  //printf("exit normally \n");
  return retVal;

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
 *  Modified:07/25/00 extend to 24 bits color image by Ying Zhuge 
 *  Modified: 2/18/05 unsigned values cast to int for subtraction
 *           by Dewey Odhner.
 *
 *****************************************************************************/
void compute_scale()
{
  int i, j, k, x, y, z, xx, yy, zz, mean_g, tti5, slice, row, col;
  int flag, tti1, edge_flag;
  double count_obj, count_nonobj, tt1, tt3;

  int mean[FEATURES],temp[FEATURES];

  
  scale_image = (unsigned char *) malloc(volume_size * sizeof(unsigned char));
  
  if ((FEATURES == 1)&&(num_of_bits == 8))
  {
    for (slice = 0; slice < pslice; slice++)
      for (row = 0; row < prow; row++)
	for (col = 0; col < pcol; col++)
	  {
	    tti1 = data_scale8[slice*slice_size + row*pcol + col];
	    if((background == 0)&&(tti1< (int) (BKND_FACTOR*mean_density_value)))
	      scale_image[slice * slice_size + row * pcol + col] = 0;
	    else
	      {
		flag = 0;
		tt1 = 0.0;
		tt3 = 0.0;
		
		for (zz = -FILTER; zz <= FILTER; zz++)
		  for (yy = -FILTER; yy <= FILTER; yy++)
		    for (xx = -FILTER; xx <= FILTER; xx++)  {
		      x = xx + col;
		      y = yy + row;
		      z = zz + slice;
		      if (x >= 0 && y >= 0 && z >= 0 
			  && x < pcol && y < prow && z < pslice)
			tt3 = tt3 + mask[zz + FILTER][yy + FILTER][xx + FILTER] 
			  * (double) data_scale8[z * slice_size 
					      + y * pcol + x];
		      else
			tt3 = tt3 + mask[zz + FILTER][yy + FILTER][xx + FILTER] 
			  * (double) data_scale8[slice * slice_size 
					      + row * pcol + col];
		    }
		mean_g = (int) (tt3 / mask_total + 0.5);

		for (k = 1; k < SCALE && !flag; k++)  {
		  count_obj = 0;
		  count_nonobj = 0;
		  for (i = 0; i < sphere_no_points[k]; i++)  {
		    x = col + sphere_points[k][i][2];
		    y = row + sphere_points[k][i][1];
		    z = slice + sphere_points[k][i][0];
		    if (x < 0 || x >= pcol)
		      x = col;
		    if (y < 0 || y >= prow)
		      y = row;
		    if (z < 0 || z >= pslice)
		      z = slice;
		    
		    tti5 = (int) data_scale8[z * slice_size + y * pcol + x];
		    tti5 = tti5 - mean_g;
		    if (tti5 < 0)
		      tti5 = -tti5;
		    count_obj = count_obj + scale_map[tti5];
		    count_nonobj = count_nonobj + 1.0 - scale_map[tti5];
		  }
		  if (100.0 * count_nonobj >= tolerance
		      * (count_nonobj + count_obj)) {
		    scale_image[slice * slice_size + row * pcol + col] = k;
		    flag = 1;
		  }
		}
		if (!flag)
		  scale_image[slice * slice_size + row * pcol + col] = k;
	      }
	  }
  }
  else if ((FEATURES == 1)&&(num_of_bits == 16)) {
    for (slice = 0; slice < pslice; slice++)
      for (row = 0; row < prow; row++)
	for (col = 0; col < pcol; col++)
	  {
	    tti1 = data_scale16[slice*slice_size + row*pcol + col];
	    if((background == 0)&&(tti1< (int) (BKND_FACTOR*mean_density_value)))
	      scale_image[slice * slice_size + row * pcol + col] = 0;
	    else
	      {
		flag = 0;
		tt1 = 0.0;
		tt3 = 0.0;

		for (zz = -FILTER; zz <= FILTER; zz++)
		  for (yy = -FILTER; yy <= FILTER; yy++)
		    for (xx = -FILTER; xx <= FILTER; xx++)  {
		      x = xx + col;
		      y = yy + row;
		      z = zz + slice;
		      if (x >= 0 && y >= 0 && z >= 0 
			  && x < pcol && y < prow && z < pslice)
			tt3 = tt3 + mask[zz + FILTER][yy + FILTER][xx + FILTER] 
			  * (double) data_scale16[z * slice_size 
					      + y * pcol + x];
		      else
			tt3 = tt3 + mask[zz + FILTER][yy + FILTER][xx + FILTER] 
			  * (double) data_scale16[slice * slice_size 
					      + row * pcol + col];
		    }
		mean_g = (int) (tt3 / mask_total + 0.5);


		for (k = 1; k < SCALE && !flag; k++)  {
		  count_obj = 0;
		  count_nonobj = 0;
		  for (i = 0; i < sphere_no_points[k]; i++)  {
		    x = col + sphere_points[k][i][2];
		    y = row + sphere_points[k][i][1];
		    z = slice + sphere_points[k][i][0];
		    if (x < 0 || x >= pcol)
		      x = col;
		    if (y < 0 || y >= prow)
		      y = row;
		    if (z < 0 || z >= pslice)
		      z = slice;
		    
		    tti5 = (int) data_scale16[z * slice_size + y * pcol + x];
		    tti5 = tti5 - mean_g;
		    if (tti5 < 0)
		      tti5 = -tti5;
		    count_obj = count_obj + scale_map[tti5];
		    count_nonobj = count_nonobj + 1.0 - scale_map[tti5];
		  }
		  if (100.0 * count_nonobj >= tolerance
		      * (count_nonobj + count_obj)) {
		    scale_image[slice * slice_size + row * pcol + col] = k;
		    flag = 1;
		  }
		}
		if (!flag)
		  scale_image[slice * slice_size + row * pcol + col] = k;
	      }
	  }
  }
  else if ((FEATURES > 1)&&(num_of_bits == 8))
    { 

      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)  
	  {
	    flag = 0;
	    edge_flag = 0;
	    
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
		    tti1 = 0;
		    for(j=0;j<FEATURES;j++)
		      {
			temp[j] = abs( (int)data_n8[j][z * slice_size + y * pcol + x] - mean[j]);		
			tti1 = tti1+(temp[j])*pow_value[j];
			if(temp[j]>diff_value_max[j])
			  edge_flag = 1;
		      }
		    if(!edge_flag)
		      {
			count_obj = count_obj + scale_map[tti1];
			count_nonobj = count_nonobj + 1.0 - scale_map[tti1];
		      }
		    else
		      {
			scale_image[slice * slice_size + row * pcol + col] = k;
			flag = 1;
		      }
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
  else if ((FEATURES > 1)&&(num_of_bits == 16))
    {
      for (slice = 0; slice < pslice; slice++)
	for (row = 0; row < prow; row++)
	  for (col = 0; col < pcol; col++)  
	  {
	    flag = 0;
	    edge_flag = 0;
	    

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
		    tti1 = 0;
		    for(j=0;j<FEATURES;j++)
		      {
			temp[j] = abs( (int)data_n16[j][z * slice_size + y * pcol + x] - mean[j]);		
			tti1 = tti1+(temp[j])*pow_value[j];
			if(temp[j]>diff_value_max[j])
			  edge_flag = 1;
		      }
		    if(!edge_flag)
		      {
			count_obj = count_obj + scale_map[tti1];
			count_nonobj = count_nonobj + 1.0 - scale_map[tti1];
		      }
		    else
		      {
			scale_image[slice * slice_size + row * pcol + col] = k;
			flag = 1;
		      }
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
  if(scale_map)
    free(scale_map);

  //printf("\rScale computation is done.     \n"); 
  fflush(stdout);	
}


int main(int argc, char* argv[])      
{
	int scaledmed;
	char paramfile[200];
	char Filename[200];
	int  number;
	FILE *fp;
	int  gbScaleVal;
	int  i;

	if (strcmp(argv[2], "-files") != 0)
	{
		fprintf(stderr, "Usage: gbscale2_train filename -files training_files");  
		exit(1);
	}

	scaledmed=0;
	strcpy(paramfile, argv[1]);
	number=argc-3;

	for( i=3; i<argc; i++)
	{
		strcpy(Filename, argv[i]);
		 
		 SCALE_THRESH = 2;
		 HIST_THRESHOLD = (float)0.6;
		 NUM_REGIONS_PAINT = 1;

		gbScaleVal=compute_scale_image(Filename,1.0,0);
	/*	sprintf(command, "gbscale2_std %s -radius 2 -hist_thresh 0.6 -num_regions 1", Filename);	
		system( command );			*/
		
		scaledmed += gbScaleVal;
	}

	scaledmed = scaledmed/number;

	fp = fopen(paramfile, "wb");
	if( fp == NULL )
		return -1;
	fprintf(fp, "%d", scaledmed);
	fclose(fp);

  return 0;
}






