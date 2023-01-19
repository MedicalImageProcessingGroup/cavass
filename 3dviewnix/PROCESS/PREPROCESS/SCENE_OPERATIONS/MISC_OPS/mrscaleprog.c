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
#include <cv3dv.h>

#define NONZERO_OR_ONE(a) (a!=0?a:1)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

typedef unsigned short OutCellType;

OutCellType *out_data;

#define INVERSE_METHOD 100

#define PERCENTILE_MAXNUM 20
int percentile_num = 4;

double sum_sqr=0.0,sum=0.0;
unsigned int count=0;
unsigned int min_dens=0xFFFFFFFF,max_dens=0x0,min_nz_dens=0xFFFFFFFF;
static unsigned int dist[0x10000];
static float dist2[0x10000];
static int dist3[0x10000];
int nonzero=1;
float scale;
float lscale, mscale, rscale;
unsigned int old_min, old_max;
unsigned int old_mode, new_mode, mean_mode, scaled_mode, mode_int;
unsigned int min_mapped, max_mapped;
unsigned int old_median, new_median, mean_median, scaled_median, median_int;
unsigned int old_percentile[PERCENTILE_MAXNUM + 1], new_percentile[PERCENTILE_MAXNUM + 1], mean_percentile[PERCENTILE_MAXNUM + 1], scaled_percentile[PERCENTILE_MAXNUM + 1], percentile_int[PERCENTILE_MAXNUM + 1];
float percentile_scale[PERCENTILE_MAXNUM + 1];
unsigned int volume_mean;
unsigned int real_new_max;
int transformmethod=1;
int threshold;
int filtersize=3;

char runmode = 0;
char *paramfile_in = 0, *paramfile_out = 0;
int force_merge=0, auto_rescale=0, dry_run=0;
int force_extrapolate=0, force_no_extrapolate=0;
int first_slice=0, last_slice=100;
float minile = -1.0, maxile = -1.0;
float minage = -1.0, maxage = -1.0;
int extrapolate = 0;
int new_min = -1, new_max = -1;
int filename_arg_start = -1;

int percentile_r = 0;
int percentage_r = 0;
int new_scale_r = 0;
int percentile_num_r = 0;
int paramfile_in_r = 0;
int paramfile_out_r = 0;

unsigned char *data1,*d1_8;
unsigned short *d1_16;

#define MAX_LANDMARKS 3
int num_landmarks=0;
unsigned int old_landmarks[MAX_LANDMARKS], mean_landmarks[MAX_LANDMARKS],new_landmarks[MAX_LANDMARKS], scaled_landmarks[MAX_LANDMARKS];
float landmarks_scale[MAX_LANDMARKS + 1];

/*****************************************************************************
 * FUNCTION: find_threshold
 * DESCRIPTION: finds the threshold for background using the second
 *    zero-crossing of the histogram's gradient
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens, new_min, new_max,
 *    filtersize and dist[] must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: threshold will be set
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 6/15/98 threshold cannot be less than volume mean
 *       by Laszlo Nyul
 *
 *****************************************************************************/
 
int find_threshold()
{
  int i,j,k;
 
  memset(dist2, 0, sizeof(dist2));
  memset(dist3, 0, sizeof(dist3));
  for (i=1;i<(int)max_dens;i++) {
    dist3[i]=dist[i+1]-dist[i];
  }
  for (i=1;i<=(int)max_dens;i++) {
    dist2[i]=0.0;
    k=0;
    for (j=-filtersize;j<=filtersize;j++) {
      if (i+j >= (int)min_nz_dens && i+j <= (int)max_dens) {
        dist2[i]+=dist3[i+j];
        k++;
      }
    }
    if (k>0) dist2[i]/=k;
  }
  for (i=1;i<=(int)max_dens;i++) {
    if (dist[i]==0) continue;
    if (dist2[i]<=0.0) break; /* first zero-crossing */
  }
  if (i>=(int)max_dens) {
    printf("Cannot find 1st zero-crossing\n");
    threshold=volume_mean;
    printf("Threshold: %d\n",threshold);fflush(stdout);
    return (0);
  }
  i++;
  printf("Background peak: %d\n",i);fflush(stdout);
  for (;i<=(int)max_dens;i++) {
    if (dist[i]==0) continue;
    if (dist2[i]>=0.0) break;
  }
  if (i>=(int)max_dens) {
    printf("Cannot find 2nd zero-crossing\n");
    threshold=volume_mean;
    printf("Threshold: %d\n",threshold);fflush(stdout);
    return (0);
  }
  threshold=i-1;
  printf("Zero crossing threshold: %d\n",threshold);fflush(stdout);
  if (threshold < (int)volume_mean) threshold = volume_mean;
  printf("Final threshold: %d\n",threshold);fflush(stdout);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_threshold2
 * DESCRIPTION: finds the threshold for background using the second
 *    zero-crossing of the histogram's gradient
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens, new_min, new_max,
 *    filtersize, minage, maxage and dist[] must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: threshold will be set
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 11/27/97 modified strategy, by Laszlo Nyul
 *    Modified: 6/15/98 threshold cannot be less than volume mean
 *       by Laszlo Nyul
 *
 *****************************************************************************/
 
int find_threshold2()
{
  int i,j,k;
  int peak;
 
  memset(dist2, 0, sizeof(dist2));
  memset(dist3, 0, sizeof(dist3));
  peak=min_nz_dens;
  for (i=(int)((minage/100)*min_nz_dens);i<=(maxage/100)*max_dens;i++) {
    if (dist[i]>dist[peak]) {
      peak=i;
    }
  }
  for (i=1;i<(int)max_dens;i++) {
    dist3[i]=dist[i+1]-dist[i];
  }
  for (i=1;i<=(int)max_dens;i++) {
    dist2[i]=0.0;
    k=0;
    for (j=-filtersize;j<=filtersize;j++) {
      if (i+j >= (int)min_nz_dens && i+j <= (int)max_dens) {
        dist2[i]+=dist3[i+j];
        k++;
      }
    }
    if (k>0) dist2[i]/=k;
  }
printf("Background peak: %d\n",peak);fflush(stdout);
  for (i=peak+1;i<=(maxage/100)*max_dens;i++) {
    if (dist[i]==0) continue;
    if (dist2[i]>=0.0) break;
  }
  if (i>=(maxage/100)*max_dens) {
    printf("Cannot find zero-crossing in the limited range\n");
    threshold=volume_mean;
printf("Threshold: %d\n",threshold);fflush(stdout);
    return (0);
  }
printf("Zero crossing threshold: %d\n",threshold);fflush(stdout);
  if (threshold < (int)volume_mean) threshold = volume_mean;
printf("Final threshold: %d\n",threshold);fflush(stdout);
  return (0);
}
 

/*****************************************************************************
 * FUNCTION: find_histmode
 * DESCRIPTION: finds the mode of the transformed histogram
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens, new_min, new_max
 *    and dist[] must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_mode and scaled_mode will be set
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histmode()
{
  int i;

  memset(dist2, 0, sizeof(dist2));
  memset(dist3, 0, sizeof(dist3));
  dist2[min_nz_dens]=(float)dist[min_nz_dens];
  for (i=min_nz_dens+1;i<=(int)max_dens;i++) {
    dist2[i]=dist2[i-1]+dist[i];
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    dist2[i]=(float)((dist2[i]/dist2[max_dens])*100.0);
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] >= minile) {
      old_min = i;
      break;
    }
  }
  old_max=old_min;
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] > maxile) {
      break;
    } else {
      old_max = i;
    }
  }
  for (i=new_min;i<=new_max;i++) {
    dist3[i]=0;
  }
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
printf("Scaling factor: %f\n",scale);fflush(stdout);
  for (i=old_min;i<=(int)old_max;i++) {
    dist3[(int)((i-old_min)*scale+new_min)]=dist[i];
  }
  old_mode=old_min;
  for (i=old_min;i<=(int)old_max;i++) {
    if (dist[i] > dist[old_mode]) {
      old_mode=i;
    }
  }
  scaled_mode=(int)((old_mode-old_min)*scale+new_min);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histmode2
 * DESCRIPTION: finds the mode of the transformed histogram
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens, new_min, new_max,
 *    minage, maxage and dist[] must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_mode and scaled_mode will be set
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 11/27/97 modified strategy, by Laszlo Nyul
 *
 *****************************************************************************/

int find_histmode2()
{
  int i;
  int llimit,ulimit;

  memset(dist2, 0, sizeof(dist2));
  memset(dist3, 0, sizeof(dist3));
  dist2[min_nz_dens]=(float)dist[min_nz_dens];
  for (i=min_nz_dens+1;i<=(int)max_dens;i++) {
    dist2[i]=dist2[i-1]+dist[i];
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    dist2[i]=(float)((dist2[i]/dist2[max_dens])*100.0);
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] >= minile) {
      old_min = i;
      break;
    }
  }
  old_max=old_min;
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] > maxile) {
      break;
    } else {
      old_max = i;
    }
  }
  for (i=new_min;i<=new_max;i++) {
    dist3[i]=0;
  }
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
printf("Minimum percentage: %d\n",(int)((minage/100.0)*min_nz_dens));fflush(stdout);
printf("Maximum percentage: %d\n",(int)((maxage/100.0)*max_dens));fflush(stdout);
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
printf("Scaling factor: %f\n",scale);fflush(stdout);
  for (i=old_min;i<=(int)old_max;i++) {
    dist3[(int)((i-old_min)*scale+new_min)]=dist[i];
  }
  llimit=(int)MAX(old_min,(minage/100)*min_nz_dens);
  ulimit=(int)MIN(old_max,(maxage/100)*max_dens);
  old_mode=llimit;
  for (i=llimit;i<=ulimit;i++) {
    if (dist[i] > dist[old_mode]) {
      old_mode=i;
    }
  }
  scaled_mode=(int)((old_mode-old_min)*scale+new_min);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histmedian
 * DESCRIPTION: finds the median of the transformed histogram
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens, new_min, new_max
 *    and dist[] must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_median and scaled_median will be set
 * HISTORY:
 *    Created: 6/18/98 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histmedian()
{
  int i;

  memset(dist2, 0, sizeof(dist2));
  dist2[min_nz_dens]=(float)dist[min_nz_dens];
  for (i=min_nz_dens+1;i<=(int)max_dens;i++) {
    dist2[i]=dist2[i-1]+dist[i];
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    dist2[i]=(float)((dist2[i]/dist2[max_dens])*100.0);
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] >= minile) {
      old_min = i;
      break;
    }
  }
  old_max=old_min;
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] > maxile) {
      break;
    } else {
      old_max = i;
    }
  }
  memset(dist3, 0, sizeof(dist3));
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
printf("Scaling factor: %f\n",scale);fflush(stdout);
  for (i=old_min;i<=(int)old_max;i++) {
    dist3[(int)((i-old_min)*scale+new_min)]=dist[i];
  }
  memset(dist2, 0, sizeof(dist2));
  dist2[old_min]=(float)dist[old_min];
  for (i=old_min+1;i<=(int)old_max;i++) {
    dist2[i]=dist2[i-1]+dist[i];
  }
  old_median=old_min;
  for (i=old_min;i<=(int)old_max;i++) {
    if (dist2[i] > 0.5*dist2[old_max]) {
      old_median=i;
      break;
    }
  }
  scaled_median=(int)((old_median-old_min)*scale+new_min);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histpercentile
 * DESCRIPTION: finds the percentiles of the transformed histogram
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens, new_min, new_max
 *    and dist[] must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_percentile[] and scaled_percentile[]
 *    will be set
 * HISTORY:
 *    Created: 2/18/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histpercentile()
{
  int i, j;

  memset(dist2, 0, sizeof(dist2));
  dist2[min_nz_dens]=(float)dist[min_nz_dens];
  for (i=min_nz_dens+1;i<=(int)max_dens;i++) {
    dist2[i]=dist2[i-1]+dist[i];
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    dist2[i]=(float)((dist2[i]/dist2[max_dens])*100.0);
  }
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] >= minile) {
      old_min = i;
      break;
    }
  }
  old_max=old_min;
  for (i=min_nz_dens;i<=(int)max_dens;i++) {
    if (dist2[i] > maxile) {
      break;
    } else {
      old_max = i;
    }
  }
  memset(dist3, 0, sizeof(dist3));
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
printf("Scaling factor: %f\n",scale);fflush(stdout);
  for (i=old_min;i<=(int)old_max;i++) {
    dist3[(int)((i-old_min)*scale+new_min)]=dist[i];
  }
  memset(dist2, 0, sizeof(dist2));
  dist2[old_min]= (float)dist[old_min];
  for (i=old_min+1;i<=(int)old_max;i++) {
    dist2[i]=dist2[i-1]+dist[i];
  }
  old_percentile[0]=old_min;
  old_percentile[percentile_num]=old_max;
  for (j=1;j<percentile_num;j++) {
    for (i=old_percentile[j-1];i<=(int)old_max;i++) {
      if (dist2[i] > (1.0/percentile_num)*j*dist2[old_max]) {
        old_percentile[j]=i;
        break;
      }
    }
  }
  for (j=0;j<percentile_num;j++) {
    scaled_percentile[j]=(int)((old_percentile[j]-old_min)*scale+new_min);
  }
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histmode_i
 * DESCRIPTION: similar to find_histmode but for inverse transforms
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_mode, new_min, new_max
 *    and new_mode will be set
 * HISTORY:
 *    Created: 5/7/98 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histmode_i()
{
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
  old_min = new_min;
  old_max = new_max;
  old_mode = mean_mode;
  new_min = min_mapped;
  new_max = max_mapped;
  new_mode = mode_int;
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Scaling factor: %f\n",scale);fflush(stdout);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histmode2_i
 * DESCRIPTION: similar to find_histmode2 but for inverse transforms
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_mode, new_min, new_max
 *    and new_mode will be set
 * HISTORY:
 *    Created: 5/7/98 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histmode2_i()
{
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
printf("Minimum percentage: %d\n",(int)((minage/100)*min_nz_dens));fflush(stdout);
printf("Maximum percentage: %d\n",(int)((maxage/100)*max_dens));fflush(stdout);
  old_min = new_min;
  old_max = new_max;
  old_mode = mean_mode;
  new_min = min_mapped;
  new_max = max_mapped;
  new_mode = mode_int;
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Scaling factor: %f\n",scale);fflush(stdout);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histmedian_i
 * DESCRIPTION: similar to find_histmedian but for inverse transforms
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_median, new_min, new_max
 *    and new_median will be set
 * HISTORY:
 *    Created: 6/18/98 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histmedian_i()
{
printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
  old_min = new_min;
  old_max = new_max;
  old_median = mean_median;
  new_min = min_mapped;
  new_max = max_mapped;
  new_median = median_int;
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Scaling factor: %f\n",scale);fflush(stdout);
  return (0);
}


/*****************************************************************************
 * FUNCTION: find_histpercentile_i
 * DESCRIPTION: similar to find_histpercentile but for inverse transforms
 * PARAMETERS:
 * SIDE EFFECTS: parameters are printed to stdout
 *    dist2[] and dist3[] will be modified
 * ENTRY CONDITIONS: min_nz_dens, max_dens must be set
 * RETURN VALUE:
 * EXIT CONDITIONS: old_min, old_max, old_percentile[], new_min, new_max
 *    and new_percentile[] will be set
 * HISTORY:
 *    Created: 2/18/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int find_histpercentile_i()
{
  int j;

printf("Minimum intensity: %d\n",min_nz_dens);fflush(stdout);
printf("Maximum intensity: %d\n",max_dens);fflush(stdout);
  old_min = new_min;
  old_max = new_max;
  for (j=0;j<percentile_num;j++)
  {
    old_percentile[j] = mean_percentile[j];
  }
  new_min = min_mapped;
  new_max = max_mapped;
  for (j=0;j<percentile_num;j++)
  {
    new_percentile[j] = percentile_int[j];
  }
printf("Minimum percentile: %d\n",old_min);fflush(stdout);
printf("Maximum percentile: %d\n",old_max);fflush(stdout);
printf("Minimum scale: %d\n",new_min);fflush(stdout);
printf("Maximum scale: %d\n",new_max);fflush(stdout);
  scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
printf("Scaling factor: %f\n",scale);fflush(stdout);
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalefull16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, scale, old_min, old_max
 *    must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/25/97 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalefull16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else {
        *out=(OutCellType)((double)(*in-old_min)*scale+new_min);
      }
    }
  return (0);
}


int Scalefull8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else {
        *out=(OutCellType)((double)(*in-old_min)*scale+new_min);
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalemode16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by new_mode
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, lscale, rscale, old_min, old_max,
 *    old_mode, new_mode must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalemode16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else if (*in < old_mode) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_mode)*rscale+new_mode);
      }
    }
  return (0);
}


int Scalemode8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else if (*in < old_mode) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_mode)*rscale+new_mode);
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalemedian16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by new_median
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, lscale, rscale, old_min, old_max,
 *    old_median, new_median must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 6/18/98 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalemedian16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else if (*in < old_median) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_median)*rscale+new_median);
      }
    }
  return (0);
}


int Scalemedian8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else if (*in < old_median) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_median)*rscale+new_median);
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalepercentile16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by new_percentile[]
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, percentile_scale[], old_min, old_max,
 *    old_percentile[], new_percentile[] must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/18/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalepercentile16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i, j;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else {
        for (j=1;j<=percentile_num;j++)
        {
          if (*in < old_percentile[j]) {
            *out=(OutCellType)((double)(*in-old_percentile[j-1])*percentile_scale[j]+new_percentile[j-1]);
            break;
          }
        }
      }
    }
  return (0);
}


int Scalepercentile8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i, j;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max) *out=new_max;
      else {
        for (j=1;j<=percentile_num;j++)
        {
          if (*in < old_percentile[j]) {
            *out=(OutCellType)((double)(*in-old_percentile[j-1])*percentile_scale[j]+new_percentile[j-1]);
            break;
          }
        }
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalefullextra16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    values above the maximum percentile level are extrapolated using
 *    the scale
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, scale, old_min, old_max
 *    must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 9/28/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalefullextra16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else {
        *out=(OutCellType)((double)(*in-old_min)*scale+new_min);
      }
    }
  return (0);
}


int Scalefullextra8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else {
        *out=(OutCellType)((double)(*in-old_min)*scale+new_min);
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalemodeextra16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by new_mode
 *    values above the maximum percentile level are extrapolated using
 *    the rscale
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, lscale, rscale, old_min, old_max,
 *    old_mode, new_mode must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalemodeextra16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
/*      else if (*in > old_max) *out=new_max;*/
      else if (*in < old_mode) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_mode)*rscale+new_mode);
      }
    }
  return (0);
}


int Scalemodeextra8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
/*      else if (*in > old_max) *out=new_max;*/
      else if (*in < old_mode) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_mode)*rscale+new_mode);
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalemedianextra16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by new_median
 *    values above the maximum percentile level are extrapolated using
 *    the rscale
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, lscale, rscale, old_min, old_max,
 *    old_median, new_median must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 7/13/98 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalemedianextra16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
/*      else if (*in > old_max) *out=new_max;*/
      else if (*in < old_median) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_median)*rscale+new_median);
      }
    }
  return (0);
}


int Scalemedianextra8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
/*      else if (*in > old_max) *out=new_max;*/
      else if (*in < old_median) {
        *out=(OutCellType)((double)(*in-old_min)*lscale+new_min);
      } else {
        *out=(OutCellType)((double)(*in-old_median)*rscale+new_median);
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: Scalepercentileextra16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by new_percentile[]
 *    values above the maximum percentile level are extrapolated using
 *    the rscale
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, percentile_scale[], old_min, old_max,
 *    old_percentile[], new_percentile[] must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/18/99 by Laszlo Nyul
 *    Modified:
 *
 *****************************************************************************/

int Scalepercentileextra16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i, j;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max)
        *out=(OutCellType)((double)(*in-old_percentile[percentile_num])*percentile_scale[percentile_num]+new_percentile[percentile_num]);
      else {
        for (j=1;j<=percentile_num;j++)
        {
          if (*in < old_percentile[j]) {
            *out=(OutCellType)((double)(*in-old_percentile[j-1])*percentile_scale[j]+new_percentile[j-1]);
            break;
          }
        }
      }
    }
  return (0);
}

int Scalepercentileextra8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i, j;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max)
        *out=(OutCellType)((double)(*in-old_percentile[percentile_num])*percentile_scale[percentile_num]+new_percentile[percentile_num]);
      else {
        for (j=1;j<=percentile_num;j++)
        {
          if (*in < old_percentile[j]) {
            *out=(OutCellType)((double)(*in-old_percentile[j-1])*percentile_scale[j]+new_percentile[j-1]);
            break;
          }
        }
      }
    }
  return (0);
}


/*****************************************************************************
 * FUNCTION: ScaleLandmarks16
 * DESCRIPTION: scales the voxels of one slice (input is 16 bits/voxel)
 *    using two scaling interval separated by mean_landmarks[]
 *
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, old_landmarks[],mean_landmarks[], 
 *    old_min, old_max, must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 3/25/04 by Ying Zhuge
 *    Modified:
 *
 *****************************************************************************/

int ScaleLandmarks16(in,out,size)
unsigned short *in;
OutCellType *out;
int size;
{
  int i, j;
  unsigned int temp_old[MAX_LANDMARKS+2],temp_new[MAX_LANDMARKS+2];

  for (j=0;j<num_landmarks;j++)
    {
      temp_old[j+1] = old_landmarks[j];
      temp_new[j+1] = new_landmarks[j];
    }
  temp_old[0] = old_min;
  temp_new[0] = new_min;
  temp_old[num_landmarks+1] = old_max;
  temp_new[num_landmarks+1] = new_max;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in >= old_max)
        { 
          if(!extrapolate)
            *out=new_max;
          else
            *out = (OutCellType)((double)(*in - old_max)*landmarks_scale[num_landmarks]+new_max);
        }
      else
        {
          for (j=1;j<=num_landmarks+1;j++)
            {
              if (*in < temp_old[j]) 
                {
                  *out=(OutCellType)((double)(*in-temp_old[j-1])*landmarks_scale[j-1] + temp_new[j-1]);
                  break;
                }
            }
        }
    }

  return (0);
}

/*****************************************************************************
 * FUNCTION: ScaleLandmarks8
 * DESCRIPTION: scales the voxels of one slice (input is 8 bits/voxel)
 *    using two scaling interval separated by mean_landmarks[]
 *
 * PARAMETERS:
 *    in: input slice
 *    out: output slice
 *    size: number of voxels in the slice
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: in must point to the beginning of the slice data
 *    and out must point to the allocated target space
 *    nonzero, new_min, new_max, old_landmarks[],mean_landmarks[], 
 *    old_min, old_max, must be set before the first call
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 3/25/04 by Ying Zhuge
 *    Modified:
 *
 *****************************************************************************/

int ScaleLandmarks8(in,out,size)
unsigned char *in;
OutCellType *out;
int size;
{
  int i, j;
  unsigned int temp_old[MAX_LANDMARKS+2],temp_new[MAX_LANDMARKS+2];

  for (j=0;j<num_landmarks;j++)
    {
      temp_old[j+1] = old_landmarks[j];
      temp_new[j+1] = new_landmarks[j];
    }
  temp_old[0] = old_min;
  temp_new[0] = new_min;
  temp_old[num_landmarks+1] = old_max;
  temp_new[num_landmarks+1] = new_max;

  for(i=0;i<size;in++,i++,out++)
    {
      if (nonzero && *in == 0) *out=0;
      else if (*in < old_min) *out=new_min;
      else if (*in > old_max)
	{ 
	  if(!extrapolate)
	    *out=new_max;
	  else
	    *out = (OutCellType)((*in - old_max)*landmarks_scale[num_landmarks]+new_max);
	}
      else {
        for (j=1;j<=num_landmarks+1;j++)
        {
          if (*in < temp_old[j]) {
            *out=(OutCellType)((double)(*in-temp_old[j-1])*landmarks_scale[j-1] + temp_new[j-1]);
            break;
          }
        }
      }
    }
  return (0);
}

/********************************************/
int Stat16(in,size)
unsigned short *in;
int size;
{
  int i;

  for(i=0;i<size;in++,i++)
    {
      if (*in > max_dens) max_dens= *in;
      if (*in < min_nz_dens) {
        min_dens= *in;
        if (min_dens) min_nz_dens=min_dens;
      }
      dist[*in]++;
      sum += *in;
      sum_sqr += (double)(*in) * (*in);
      count++;
    }
  return (0);
}


int Stat8(in,size)
unsigned char *in;
int size;
{
  int i;

  for(i=0;i<size;in++,i++)
    {
      if (*in > max_dens) max_dens= *in;
      if (*in < min_nz_dens) {
        min_dens= *in;
        if (min_dens) min_nz_dens=min_dens;
      }
      dist[*in]++;
      sum += *in;
      sum_sqr += (double)(*in) * (*in);
      count++;
    }
  return (0);
}


int get_slices(dim,list)
int dim;
short *list;
{
  int i,sm;

  if (dim==3) return (int)(list[0]);
  if (dim==4) {
    for(sm=0,i=0;i<list[0];i++)
      sm+= list[1+i];
    return(sm);
  }
  return(0);
}


int bin_to_grey(bin_buffer, length, grey_buffer, min_value, max_value)
unsigned char *bin_buffer;
int length;
unsigned char *grey_buffer;
int min_value, max_value;
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin, *grey;

  bin = bin_buffer;
  grey = grey_buffer;
  for(j=7,i=length; i>0; i--) {
    if( (*bin & mask[j]) != 0)
      *grey = max_value;
    else
      *grey = min_value;
    grey++;
    if (j==0) {
      bin++; j=7;
    }
    else
      j--;
  }
  return (0);
}


int read_and_stat(in_fname, stat_func16, stat_func8, free_data)
  char *in_fname;
  int (*stat_func16)(unsigned short *, int);
  int (*stat_func8)(unsigned char *, int);
  int free_data;
{
  int i;
  FILE *in1;
  ViewnixHeader vh1;
  int j,slices,size,size1,error,bytes;
  char group[6],elem[6];
  int f_slice, l_slice;

  in1=fopen(in_fname,"rb");
  if (in1==NULL ) {
    printf("Error in opening the file\n");
    exit(-1);
  }
  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  f_slice = (first_slice * slices + 50) / 100;
  l_slice = (last_slice * slices + 50) / 100;
  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
  if (vh1.scn.num_of_bits==16)
    bytes=2;
  else
    bytes=1;
  size1= (size*vh1.scn.num_of_bits+7)/8;
  data1= (unsigned char *)malloc(size1);
  if (vh1.scn.num_of_bits==1) {
    d1_8=(unsigned char *)malloc(size);
  }
  else
    d1_8=data1;
  d1_16=(unsigned short *)d1_8;
  sum_sqr=0.0;sum=0.0;
  count=0;
  min_dens=0xFFFFFFFF;max_dens=0x0;min_nz_dens=0xFFFFFFFF;
  memset(dist, 0, sizeof(dist));
  VSeekData(in1,0);
  for(i=0;i<slices;i++) {
/*    printf("Processing slice %d\r",i); fflush(stdout);*/
    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      printf("Could not read data\n");
      exit(-1);
    }
    if (i >= f_slice && i <= l_slice) {
      if (vh1.scn.num_of_bits==1)
        bin_to_grey(data1,size,d1_8,0,1);
      if (vh1.scn.num_of_bits==16)
        (*stat_func16)(d1_16,size);
      else
        (*stat_func8)(d1_8,size);
    }
  }
  fclose(in1);
  if (free_data) {
    if (d1_8!=data1) free(d1_8);
    free(data1);
  }
  return (0);
}
 

int scale_and_write(in_fname, out_fname, scale_func16, scale_func8, free_data)
  char *in_fname, *out_fname;
  int (*scale_func16)(unsigned short *, OutCellType *, int);
  int (*scale_func8)(unsigned char *, OutCellType *, int);
  int free_data;
{
  int i;
  FILE *in1,*out1;
  ViewnixHeader vh1,vh_out;
  int j,slices,size,size1,error,bytes;
  char group[6],elem[6];

  switch (transformmethod) {
    case 0:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
    break;
    case 1:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Mode used: %d\n",new_mode);fflush(stdout);
    break;
    case 2:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Mode used: %d\n",new_mode);fflush(stdout);
    break;
    case 3:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Mode used: %d\n",new_mode);fflush(stdout);
    break;
    case 4:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Median used: %d\n",new_median);fflush(stdout);
    break;
    case 5:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Percentile number: %d\n",percentile_num);fflush(stdout);
      for (j=1;j<percentile_num;j++)
	{
	  printf("Percentile %d used: %d\n",j,new_percentile[j]);fflush(stdout);
	}
      break;
    case 6:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Landmark number: %d\n",num_landmarks);fflush(stdout);
      for (j=0;j<num_landmarks;j++)
	{
	  printf("Landmark %d used: %d\n",j,mean_landmarks[j]);fflush(stdout);
	}
      break;

    case 0+INVERSE_METHOD:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      break;
    case 1+INVERSE_METHOD:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Mode used: %d\n",new_mode);fflush(stdout);
      break;
    case 2+INVERSE_METHOD:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Mode used: %d\n",new_mode);fflush(stdout);
      break;
    case 3+INVERSE_METHOD:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Mode used: %d\n",new_mode);fflush(stdout);
      break;
    case 4+INVERSE_METHOD:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Median used: %d\n",new_median);fflush(stdout);
      break;
    case 5+INVERSE_METHOD:
      printf("Minimum scale used: %d\n",new_min);fflush(stdout);
      printf("Maximum scale used: %d\n",new_max);fflush(stdout);
      printf("Percentile number: %d\n",percentile_num);fflush(stdout);
      for (j=1;j<percentile_num;j++)
	{
	  printf("Percentile %d used: %d\n",j,new_percentile[j]);fflush(stdout);
	}
      break;
  }
  if (!dry_run) {
    in1=fopen(in_fname,"rb");
    if (in1==NULL ) {
      printf("Error in opening the file\n");
      exit(-1);
    }
    error=VReadHeader(in1,&vh1,group,elem);
    if (error<=104) {
      printf("Fatal error in reading header\n");
      exit(-1);
    }
    slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
    size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
    if (vh1.scn.num_of_bits==16)
      bytes=2;
    else
      bytes=1;
    size1= (size*vh1.scn.num_of_bits+7)/8;
    VSeekData(in1,0);
    out_data = (OutCellType *)malloc(size*sizeof(OutCellType));
    memcpy(&vh_out, &vh1, sizeof(vh1));
    strncpy(vh_out.gen.filename, out_fname, sizeof(vh_out.gen.filename));
    vh_out.scn.smallest_density_value[0] = 0;
    vh_out.scn.largest_density_value[0] = (float)real_new_max;
    vh_out.scn.smallest_density_value_valid = 1;
    vh_out.scn.largest_density_value_valid = 1;
    /* added by zhuge*/

    vh_out.scn.num_of_bits = sizeof(OutCellType)*8;
    vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits-1;

    out1 = fopen(out_fname,"wb");
    VWriteHeader(out1, &vh_out, group, elem);
    VSeekData(in1,0);
    for(i=0;i<slices;i++) {
      /*    printf("Processing slice %d\r",i); fflush(stdout);*/
      if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
        printf("Could not read data\n");
        exit(-1);
      }
      if (vh1.scn.num_of_bits==1) 
        bin_to_grey(data1,size,d1_8,0,1);
      if (vh1.scn.num_of_bits==16)
        (*scale_func16)(d1_16,out_data,size);
      else
        (*scale_func8)(d1_8,out_data,size);
      //(*scale_func8)(data1,out_data,size);
      VWriteData((char *)out_data, sizeof(OutCellType), size, out1, &j);
    }
    VCloseData(out1);
    fclose(in1);
    free(out_data);
  }
  if (free_data) {
    if (d1_8!=data1) free(d1_8);
    free(data1);
  }
  return (0);
}
    

/*****************************************************************************
 * FUNCTION: read_paramfile
 * DESCRIPTION: reads the content of a parameter file
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS: transformmethod, minile, maxile, minage, maxage,
 *    new_min, new_max, mean_mode, min_mapped, max_mapped, mode_int
 *    will be set if specified in the file
 * HISTORY:
 *    Created: 5/7/98 by Laszlo Nyul
 *    Modified: 6/18/98 new parameters added by Laszlo Nyul
 *
 *****************************************************************************/
 
int read_paramfile(fname)
  char *fname;
{
  FILE *infile;
  char line[256];
  char testline[256];
  int j;

  infile = fopen(fname,"rb");
  if (infile == NULL) {
    printf("Error in opening the parameter file\n");
    return (-1);
  }
  while (!feof(infile)) {
    fgets(line, sizeof(line), infile);
    if (!strncmp(line, "Method:", 7)) {
      sscanf(line+7, "%d", &transformmethod);
    }
    if (!strncmp(line, "First slice for hist:", 21)) {
      sscanf(line+21, "%d", &first_slice);
    }
    if (!strncmp(line, "Last slice for hist:", 20)) {
      sscanf(line+20, "%d", &last_slice);
    }
    if (!strncmp(line, "Minimum percentile:", 19)) {
      sscanf(line+19, "%f", &minile);
    }
    if (!strncmp(line, "Maximum percentile:", 19)) {
      sscanf(line+19, "%f", &maxile);
    }
    if (!strncmp(line, "Minimum percentage:", 19)) {
      sscanf(line+19, "%f", &minage);
    }
    if (!strncmp(line, "Maximum percentage:", 19)) {
      sscanf(line+18, "%f", &maxage);
    }
    if (!strncmp(line, "Minimum scale:", 14)) {
      sscanf(line+14, "%d", &new_min);
    }
    if (!strncmp(line, "Maximum scale:", 14)) {
      sscanf(line+14, "%d", &new_max);
    }

    /* added for method 6 */
    if (!strncmp(line, "Number of landmarks:", 20)) {
      sscanf(line+20, "%d", &num_landmarks);
    }
    for(j=0;j<num_landmarks;j++)
    {
      sprintf(testline, "Landmark %d:", j);
      if (!strncmp(line, testline, strlen(testline))) {
        sscanf(line+strlen(testline), "%d", &mean_landmarks[j]);
      }
    }
    /*----------------------*/

    if (!strncmp(line, "Mode mean:", 10)) {
      sscanf(line+10, "%d", &mean_mode);
    }
    if (!strncmp(line, "Median mean:", 12)) {
      sscanf(line+12, "%d", &mean_median);
    }
    if (!strncmp(line, "Percentile number:", 18)) {
      sscanf(line+18, "%d", &percentile_num);
    }
    for (j=1;j<percentile_num;j++)
    {
      sprintf(testline, "Percentile %d mean:", j);
      if (!strncmp(line, testline, strlen(testline))) {
        sscanf(line+strlen(testline), "%d", &mean_percentile[j]);
      }
    }
    if (!strncmp(line, "Minimum mapped intensity:", 25)) {
      sscanf(line+25, "%d", &min_mapped);
    }
    if (!strncmp(line, "Maximum mapped intensity:", 25)) {
      sscanf(line+25, "%d", &max_mapped);
    }
    if (!strncmp(line, "Mode intensity:", 15)) {
      sscanf(line+15, "%d", &mode_int);
    }
    if (!strncmp(line, "Median intensity:", 17)) {
      sscanf(line+17, "%d", &median_int);
    }
    for (j=1;j<percentile_num;j++)
    {
      sprintf(testline, "Percentile %d intensity:", j);
      if (!strncmp(line, testline, strlen(testline))) {
        sscanf(line+strlen(testline), "%d", &percentile_int[j]);
      }
    }
    if (!strncmp(line, "Extrapolate:", 12)) {
      sscanf(line+12, "%d", &extrapolate);
    }
  }
  fclose(infile);
  return (0);
}
/****************************************************************************
 * FUNCTION: estimate_landmark
 * DESCRIPTION: Gets mean/median landmark from a  training region 
 *    and calculates the scaling landmark from it.
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: argc and argv should be given according to the
 *    Usage 1 message, new_min,new_max should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code -1 on error.
 * HISTORY:
 *    Created: 3/24/04 by Ying Zhuge for training method 6
 *    Modified:08/19/04 replace the mean value with median value 
 *
 *****************************************************************************/
unsigned int estimate_landmark(char *infile, char *maskfile)
{
  int i,j,k,index,error_code,pcol,prow,pslice,slice_size,volume_size,num_of_bits;
  int tti1,tti2,tti3,tti4,count,temp;
  unsigned int object_mean;
  FILE *fp_in;
  ViewnixHeader vh_in;
  char group[6],element[6];
  unsigned char *data_in8,*mask_data;
  unsigned short *data_in16, *buffer_data;
  unsigned char binpix[8] = {128, 64, 32, 16, 8, 4, 2, 1};   /* binary pixels */
      
  fp_in = fopen(infile, "rb");
  error_code = VReadHeader(fp_in, &vh_in, group, element);
  fclose(fp_in);
  
  /*
  old_min= vh_in.scn.smallest_density_value[0];
  old_max = vh_in.scn.largest_density_value[0];
  */

  pcol = vh_in.scn.xysize[0];
  prow = vh_in.scn.xysize[1];
  pslice = vh_in.scn.num_of_subscenes[0];

  slice_size = pcol * prow;
  volume_size = slice_size * pslice;
  num_of_bits = vh_in.scn.num_of_bits;

  if(num_of_bits == 8)
    {
      data_in8 = (unsigned char *)malloc(volume_size*sizeof(char));
      if(data_in8 ==NULL)
        printf("Memory allocation error \n");
    }
  else if(num_of_bits == 16)
    {
      data_in16 = (unsigned short *)malloc(volume_size*sizeof(short));
      if(data_in16 ==NULL)
        printf("Memory allocation error \n");
    }

  fp_in = fopen(infile,"rb");
  if (fp_in == NULL)
    printf("Cannot open file \n");
  VSeekData(fp_in, 0);
  if(num_of_bits == 8)
    error_code = VReadData((char *)data_in8, num_of_bits/8, volume_size, fp_in, &j);
  else if(num_of_bits == 16)
    error_code = VReadData((char *)data_in16, num_of_bits/8, volume_size, fp_in, &j);
  fclose(fp_in);


  mask_data = (unsigned char *)malloc( pslice*((slice_size+7)/8) *sizeof(unsigned char));
     
  fp_in = fopen(maskfile,"rb");
  if(fp_in == NULL)
    printf("open file error! \n");
  
  //mask_datas are stored with BIM file format
  error_code = VSeekData(fp_in,0);
  error_code = VReadData((char *)mask_data, 1, pslice*((slice_size+7)/8)*sizeof(unsigned char), fp_in, &j);
  fclose(fp_in);

  {
    count = 0;
    temp = 0;

    for (i = 0; i< pslice; i++)
      for (j = 0;j< prow; j++)
        for (k = 0;k < pcol; k++)
          {
            tti1 = i*((slice_size+7)/8) + (j*pcol+k)/8;
            tti2 = (j*pcol+k)%8;
            tti3 = mask_data[tti1];
            tti4 = binpix[tti2];

            if((tti3 & tti4)!=0)
              {
                if(num_of_bits == 8)
                  {
                    temp = temp + data_in8[i*slice_size+j*pcol+k];
                  }
                else if(num_of_bits == 16)
                  {
                    temp = temp + data_in16[i*slice_size+j*pcol+k];
                  }
                count++;
              }
          }
    object_mean = (unsigned int) (temp/count);
  }
  /*new_mean = object_mean * (new_max - new_min)/(max-min);*/


  /* to estimate the median value */
  buffer_data = (unsigned short*)malloc(count*sizeof(short));
  if(buffer_data==NULL){
    printf("Memory allocation error!\n");
    exit(-1);
  }
  memset(buffer_data,0,2*count);

  index = 0;
  for (i = 0; i< pslice; i++)
    for (j = 0;j< prow; j++)
      for (k = 0;k < pcol; k++)
      {
        tti1 = i*((slice_size+7)/8) + (j*pcol+k)/8;
        tti2 = (j*pcol+k)%8;
        tti3 = mask_data[tti1];
        tti4 = binpix[tti2];

        if((tti3 & tti4)!=0)
          {
            if(num_of_bits == 8)
                buffer_data[index++] = data_in8[i*slice_size+j*pcol+k];
            else if(num_of_bits == 16)
                buffer_data[index++] = data_in16[i*slice_size+j*pcol+k];
          }
      }
  /*
  for(i = 0;i<=count/2;i++)
    {
      index = i;
      for(j=count-1;j>i;j--)
	if(buffer_data[j]<buffer_data[index])
	  index = j;
      tempV = buffer_data[i];
      buffer_data[i] = buffer_data[index];
      buffer_data[index] = tempV;
      } 
      object_median = buffer_data[count/2];*/  /* obtain the median value */
 
  free(buffer_data);

  if(num_of_bits == 8)
    free(data_in8);  

  if(num_of_bits == 16)
    free(data_in16);

  if(mask_data!=NULL)
    free(mask_data);  

  return object_mean;  /* estimating median take too much time; still use mean as landmards */
  //return object_median;
}


/*****************************************************************************
 * FUNCTION: run_train
 * DESCRIPTION: Gets histogram landmarks from a set of training images
 *    and calculates the scaling parameters from them.
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: argc and argv should be given according to the
 *    Usage 1 message
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code -1 on error.
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 11/25/97 new method switch (3) added by Laszlo Nyul
 *    Modified: 6/18/98 new method switch (4) added by Laszlo Nyul
 *    Modified: 2/18/99 new method switches (6,7) added by Laszlo Nyul
 *    Modified: 3/24/04 new method 6 added by Ying Zhuge
 *
 *****************************************************************************/
 
int run_train(argc,argv)
int argc;
char *argv[];
{
  FILE *out1;
  int i,j,k,l,num_files;
  unsigned int temp_landmark;

  l=k=filename_arg_start;
  extrapolate = force_no_extrapolate ? 0 : force_extrapolate ? 1 : extrapolate;
  printf("Method: %d\n", transformmethod);fflush(stdout);
  switch (transformmethod) {
    case 0:
      break;
    case 1:
      mean_mode=0;
      for (; k<argc; k++) {
	printf("Processing file %s\n",argv[k]);fflush(stdout);
        read_and_stat(argv[k],Stat16,Stat8,1);
        volume_mean= (unsigned)(sum/count);
	printf("Volume mean: %d\n", volume_mean);fflush(stdout);
        find_threshold();
        for (i=1;i<=threshold;i++) {
          dist[0]+=dist[i];
          dist[i]=0;
        }
        find_histmode();
	printf("Mode: %d\n", old_mode);fflush(stdout);
	printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
        mean_mode+=scaled_mode;
	fflush(stdout);
      }
      mean_mode/=(argc-l);
      printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      break;
    case 2:
      mean_mode=0;
      for (; k<argc; k++) {
	printf("Processing file %s\n",argv[k]);fflush(stdout);
        read_and_stat(argv[k],Stat16,Stat8,1);
        find_histmode();
	printf("Mode: %d\n", old_mode);fflush(stdout);
	printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
        mean_mode+=scaled_mode;
	fflush(stdout);
      }
      mean_mode/=(argc-l);
      printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      break;
    case 3:
      mean_mode=0;
      for (; k<argc; k++) {
	printf("Processing file %s\n",argv[k]);fflush(stdout);
        read_and_stat(argv[k],Stat16,Stat8,1);
        volume_mean= (unsigned)(sum/count);
	printf("Volume mean: %d\n", volume_mean);fflush(stdout);
        find_threshold2();
        for (i=1;i<=threshold;i++) {
          dist[0]+=dist[i];
          dist[i]=0;
        }
        find_histmode2();
	printf("Mode: %d\n", old_mode);fflush(stdout);
	printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
        mean_mode+=scaled_mode;
	fflush(stdout);
      }
      mean_mode/=(argc-l);
      printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      break;
    case 4:
      mean_median=0;
      for (; k<argc; k++) {
	printf("Processing file %s\n",argv[k]);fflush(stdout);
        read_and_stat(argv[k],Stat16,Stat8,1);
        volume_mean= (unsigned)(sum/count);
	printf("Volume mean: %d\n", volume_mean);fflush(stdout);
        find_threshold();
        for (i=1;i<=threshold;i++) {
          dist[0]+=dist[i];
          dist[i]=0;
        }
        find_histmedian();
	printf("Median: %d\n", old_median);fflush(stdout);
	printf("Scaled median: %d\n", scaled_median);fflush(stdout);
        mean_median+=scaled_median;
	fflush(stdout);
      }
      mean_median/=(argc-l);
      printf("Median mean: %d\n", mean_median);fflush(stdout);
      break;
  case 5:
      for (j=1;j<percentile_num;j++)
      {
        mean_percentile[j]=0;
      }
      for (; k<argc; k++) {
	printf("Processing file %s\n",argv[k]);fflush(stdout);
        read_and_stat(argv[k],Stat16,Stat8,1);
        volume_mean= (unsigned)(sum/count);
	printf("Volume mean: %d\n", volume_mean);fflush(stdout);
        find_threshold();
        for (i=1;i<=threshold;i++) {
          dist[0]+=dist[i];
          dist[i]=0;
        }
        find_histpercentile();
        for (j=1;j<percentile_num;j++)
        {
	  printf("Percentile %d: %d\n", j, old_percentile[j]);fflush(stdout);
        }
        for (j=1;j<percentile_num;j++)
        {
	  printf("Scaled percentile %d: %d\n", j, scaled_percentile[j]);fflush(stdout);
          mean_percentile[j]+=scaled_percentile[j];
        }
	fflush(stdout);
      }
      for (j=1;j<percentile_num;j++)
      {
        mean_percentile[j]/=(argc-l);
	printf("Percentile %d mean: %d\n", j, mean_percentile[j]);fflush(stdout);
      }
      break;
  case 6:
    for(j = 0;j<num_landmarks;j++)
        mean_landmarks[j] = 0; 

    num_files = 0;
    while (k<argc)
      {
        printf("Processing file %s\n",argv[k]);fflush(stdout);

        /* get maximum and minimum density */
        read_and_stat(argv[k],Stat16,Stat8,1);

        /* get percentile maximum and minimum density      */
        /* and save to old_min and old_max, get scale also */
        find_threshold();
        for (i=1;i<=threshold;i++) {
          dist[0]+=dist[i];
          dist[i]=0;
        }
        find_histmedian();

        for(j = 0;j<num_landmarks;j++)
          {
            temp_landmark = estimate_landmark(argv[k],argv[k+j+1]); /* return mean/median value of mask region */
            scaled_landmarks[j] = (unsigned)((temp_landmark - old_min) * scale + new_min);
            printf("Landmark %d value: %d\n", j, temp_landmark);
            printf("Scaled landmark %d value: %d\n", j, scaled_landmarks[j]);
            fflush(stdout);
            mean_landmarks[j] = mean_landmarks[j]+scaled_landmarks[j];
          }
        num_files++;
        k = k+num_landmarks+1;
      }
    for(j = 0;j<num_landmarks;j++)
      {
        mean_landmarks[j] = mean_landmarks[j]/num_files;
      }
    /* sort landmarks from small to big */
    for(j = 0;j<num_landmarks;j++)
      {
        l=j;
        for(i = j+1;i<num_landmarks;i++)
          if(mean_landmarks[i]<mean_landmarks[l]) l = i;
        if(l!=j)
          {
            temp_landmark = mean_landmarks[j];
            mean_landmarks[j] =  mean_landmarks[l];
            mean_landmarks[l] = temp_landmark;
          }
      }
    for(j = 0;j<num_landmarks;j++)
      printf("Standard scaled landmark %d value: %d\n", j, mean_landmarks[j]);
    fflush(stdout);

    break;
  }
  fflush(stdout);
  out1 = fopen(paramfile_out,"wb");
  switch (transformmethod) {
    case 0:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
    case 1:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      fprintf(out1, "Mode mean:            %d\n", mean_mode);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
    case 2:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      fprintf(out1, "Mode mean:            %d\n", mean_mode);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
    case 3:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum percentage:   %.2f\n", minage);
      fprintf(out1, "Maximum percentage:   %.2f\n", maxage);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      fprintf(out1, "Mode mean:            %d\n", mean_mode);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
    case 4:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      fprintf(out1, "Median mean:          %d\n", mean_median);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
    case 5:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      fprintf(out1, "Percentile number:    %d\n", percentile_num);
      for (j=1;j<percentile_num;j++)
      {
        fprintf(out1, "Percentile %d mean:    %d\n", j, mean_percentile[j]);
      }
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
  case 6:
      fprintf(out1, "Method:               %d\n", transformmethod);
      if (first_slice >= 0)
        fprintf(out1, "First slice for hist: %d\n", first_slice);
      if (last_slice >= 0)
        fprintf(out1, "Last slice for hist:  %d\n", last_slice);
      fprintf(out1, "Minimum percentile:   %.2f\n", minile);
      fprintf(out1, "Maximum percentile:   %.2f\n", maxile);
      fprintf(out1, "Minimum scale:        %d\n", new_min);
      fprintf(out1, "Maximum scale:        %d\n", new_max);
      fprintf(out1, "Number of landmarks:  %d\n", num_landmarks);
      for(j=0;j<num_landmarks;j++)
        fprintf(out1, "Landmark %d:  %d\n", j, mean_landmarks[j]);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:          %d\n", extrapolate);
      break;
  }
  fclose(out1);
  return (0);
}


/*****************************************************************************
 * FUNCTION: run_apply
 * DESCRIPTION: Gets histogram landmarks from a parameter file
 *    and transforms the input image.
 * PARAMETERS:
 * SIDE EFFECTS: transformation parameters and additional parameters are
 *    printed to stdout
 * ENTRY CONDITIONS: argc and argv should be given according to the
 *    Usage 2 message
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code -1 on error.
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 11/25/97 new method parameter (3) added by Laszlo Nyul
 *    Modified: 5/7/98 call to read_paramfile added, inverse transforms
 *       added by Laszlo Nyul
 *    Modified: 6/18/98 checks for intensity merges, by Laszlo Nyul
 *    Modified: 6/18/98 new method parameter (4) added by Laszlo Nyul
 *    Modified: 2/18/99 new method parameters (6,7) added by Laszlo Nyul
 *    Modified: 9/28/99 Scalefullextra added by Laszlo Nyul
 *
 *****************************************************************************/
 
int run_apply(argc,argv)
int argc;
char *argv[];
{
  int i,j,k,l,retval;
  int range_save, new_mode_save, new_median_save;
  int new_percentile_save[PERCENTILE_MAXNUM + 1];
  int merge_flag;
  char *filename_in, *filename_out;
  unsigned int temp_landmark;
  unsigned int temp_old[MAX_LANDMARKS+2],temp_new[MAX_LANDMARKS+2];

  retval = read_paramfile(paramfile_in);
  if (retval) exit(retval);
  filename_in = argv[filename_arg_start];
  filename_out = argv[filename_arg_start+1];
  extrapolate = force_no_extrapolate ? 0 : force_extrapolate ? 1 : extrapolate;
  printf("Method: %d\n", transformmethod);fflush(stdout);

  printf("First slice for hist: %d\n", first_slice);fflush(stdout);
  printf("Last slice for hist: %d\n", last_slice);fflush(stdout);

  switch (transformmethod) {
    case 0:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmode();
      scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
      if (scale < 1.0) {
        if (auto_rescale) {
          printf("Scaling factor < 1.0; automatic rescaling applied\n");fflush(stdout);
        } else if (force_merge) {
          printf("Scaling factor < 1.0; force scaling with intensity merge\n");fflush(stdout);
        } else {
          printf("Scaling factor < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          exit(-1);
        }
        if (auto_rescale) {
          range_save = new_max-new_min;
          while (scale < 1.0) {
            new_max += range_save;
            scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
          }
        }
      }
      printf("Scaling factor: %f\n",scale);
/*      real_new_max = new_max;
      scale_and_write(filename_in,filename_out,Scalefull16,Scalefull8,1);*/
      if (!extrapolate) {
        real_new_max = new_max;
        scale_and_write(filename_in,filename_out,Scalefull16,Scalefull8,1);
      } else {
        real_new_max = (unsigned)((double)(max_dens-old_min)*scale+new_min);
        printf("Extrapolated maximum: %u\n",real_new_max);fflush(stdout);
        scale_and_write(filename_in,filename_out,Scalefullextra16,Scalefullextra8,1);
      }
      break;
    case 1:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      volume_mean= (unsigned)(sum/count);
      printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histmode();
      printf("Mode: %d\n", old_mode);fflush(stdout);
      printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
      printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      new_mode = mean_mode;
      lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
      rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
      if (lscale < 1.0 || rscale < 1.0) {
        if (auto_rescale) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
        } else if (force_merge) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
        } else {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          exit(-1);
        }
        if (auto_rescale) {
          range_save = new_max-new_min;
          new_mode_save = new_mode;
          while (lscale < 1.0 || rscale < 1.0) {
            new_max += range_save;
            new_mode = (unsigned)(new_min+(new_mode_save-new_min)*((double)(new_max-new_min)/(range_save)));
            lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
            rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
          }
        }
      }
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      if (!extrapolate) {
        real_new_max = new_max;
        scale_and_write(filename_in,filename_out,Scalemode16,Scalemode8,1);
      } else {
        real_new_max = (unsigned)((double)(max_dens-old_mode)*rscale+new_mode);
        printf("Extrapolated maximum: %u\n",real_new_max);fflush(stdout);
        scale_and_write(filename_in,filename_out,Scalemodeextra16,Scalemodeextra8,1);
      }
      break;
    case 2:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmode();
      printf("Mode: %d\n", old_mode);fflush(stdout);
      printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
      printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      new_mode = mean_mode;
      lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
      rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
      if (lscale < 1.0 || rscale < 1.0) {
        if (auto_rescale) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
        } else if (force_merge) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
        } else {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          exit(-1);
        }
        if (auto_rescale) {
          range_save = new_max-new_min;
          new_mode_save = new_mode;
          while (lscale < 1.0 || rscale < 1.0) {
            new_max += range_save;
            new_mode = (unsigned)(new_min+(new_mode_save-new_min)*((double)(new_max-new_min)/(range_save)));
            lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
            rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
          }
        }
      }
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      if (!extrapolate) {
        real_new_max = new_max;
        scale_and_write(filename_in,filename_out,Scalemode16,Scalemode8,1);
      } else {
        real_new_max = (unsigned)((double)(max_dens-old_mode)*rscale+new_mode);
        printf("Extrapolated maximum: %u\n",real_new_max);fflush(stdout);
        scale_and_write(filename_in,filename_out,Scalemodeextra16,Scalemodeextra8,1);
      }
      break;
    case 3:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      volume_mean=(unsigned)(sum/count);
      printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold2();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histmode2();
      printf("Mode: %d\n", old_mode);fflush(stdout);
      printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
      printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      new_mode = mean_mode;
      lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
      rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
      if (lscale < 1.0 || rscale < 1.0) {
        if (auto_rescale) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
        } else if (force_merge) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
        } else {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          exit(-1);
        }
        if (auto_rescale) {
          range_save = new_max-new_min;
          new_mode_save = new_mode;
          while (lscale < 1.0 || rscale < 1.0) {
            new_max += range_save;
            new_mode = (unsigned)(new_min+(new_mode_save-new_min)*((double)(new_max-new_min)/(range_save)));
            lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
            rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
          }
        }
      }
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      real_new_max = new_max;
      scale_and_write(filename_in,filename_out,Scalemode16,Scalemode8,1);
      break;
    case 4:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      volume_mean= (unsigned)(sum/count);
      printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histmedian();
      printf("Median: %d\n", old_median);fflush(stdout);
      printf("Scaled median: %d\n", scaled_median);fflush(stdout);
      printf("Median mean: %d\n", mean_median);fflush(stdout);
      new_median = mean_median;
      lscale=(float)((double)(new_median-new_min)/NONZERO_OR_ONE(old_median-old_min));
      rscale=(float)((double)(new_max-new_median)/NONZERO_OR_ONE(old_max-old_median));
      if (lscale < 1.0 || rscale < 1.0) {
        if (auto_rescale) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; automatic rescaling applied\n");fflush(stdout);
          }
        } else if (force_merge) {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; force scaling with intensity merge\n");fflush(stdout);
          }
        } else {
          if (lscale < 1.0) {
            printf("Scaling factor 1 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          if (rscale < 1.0) {
            printf("Scaling factor 2 < 1.0; not scaling with intensity merge; exiting\n");fflush(stdout);
          }
          exit(-1);
        }
        if (auto_rescale) {
          range_save = new_max-new_min;
          new_median_save = new_median;
          while (lscale < 1.0 || rscale < 1.0) {
            new_max += range_save;
            new_median = (unsigned)(new_min+(new_median_save-new_min)*((double)(new_max-new_min)/(range_save)));
            lscale=(float)((double)(new_median-new_min)/NONZERO_OR_ONE(old_median-old_min));
            rscale=(float)((double)(new_max-new_median)/NONZERO_OR_ONE(old_max-old_median));
          }
        }
      }
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      if (!extrapolate) {
        real_new_max = new_max;
        scale_and_write(filename_in,filename_out,Scalemedian16,Scalemedian8,1);
      } else {
        real_new_max = (unsigned)((double)(max_dens-old_median)*rscale+new_median);
        printf("Extrapolated maximum: %u\n",real_new_max);fflush(stdout);
        scale_and_write(filename_in,filename_out,Scalemedianextra16,Scalemedianextra8,1);
      }
      break;
    case 5:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      volume_mean= (unsigned)(sum/count);
      printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histpercentile();
      for (j=1;j<percentile_num;j++)
      {
        printf("Percentile %d: %d\n", j, old_percentile[j]);fflush(stdout);
      }
      for (j=1;j<percentile_num;j++)
      {
        printf("Scaled percentile %d: %d\n", j, scaled_percentile[j]);fflush(stdout);
      }
      for (j=1;j<percentile_num;j++)
      {
        printf("Percentile %d mean: %d\n", j, mean_percentile[j]);fflush(stdout);
      }
      old_percentile[0] = old_min;
      old_percentile[percentile_num] = old_max;
      mean_percentile[0] = new_min;
      mean_percentile[percentile_num] = new_max;
      for (j=1;j<=percentile_num;j++)
      {
        new_percentile[j] = mean_percentile[j];
        percentile_scale[j]=(float)((double)(new_percentile[j]-new_percentile[j-1])/NONZERO_OR_ONE(old_percentile[j]-old_percentile[j-1]));
      }
      merge_flag = 0;
      for (j=1;j<=percentile_num;j++)
      {
        if (percentile_scale[j] < 1.0)
          merge_flag = 1;
      }
      if (merge_flag) {
        if (auto_rescale) {
          for (j=1;j<=percentile_num;j++)
          {
            if (percentile_scale[j] < 1.0)
            {
              printf("Scaling factor %d < 1.0; automatic rescaling applied\n", j);fflush(stdout);
            }
          }
        } else if (force_merge) {
          for (j=1;j<=percentile_num;j++)
          {
            if (percentile_scale[j] < 1.0)
            {
              printf("Scaling factor %d < 1.0; force scaling with intensity merge\n", j);fflush(stdout);
            }
          }
        } else {
          for (j=1;j<=percentile_num;j++)
          {
            if (percentile_scale[j] < 1.0)
            {
              printf("Scaling factor %d < 1.0; not scaling with intensity merge; exiting\n", j);fflush(stdout);
            exit(-1);
            }
          }
        }
        if (auto_rescale) {
          range_save = new_max-new_min;
          for (j=1;j<=percentile_num;j++)
          {
            new_percentile_save[j] = new_percentile[j];
          }
          while (merge_flag) {
            new_max += range_save;
            for (j=1;j<=percentile_num;j++)
            {
              new_percentile[j] = (unsigned)(new_percentile[0]+(new_percentile_save[j]-new_percentile[0])*((double)(new_max-new_min)/(range_save)));
              percentile_scale[j]=(float)((double)(new_percentile[j]-new_percentile[j-1])/NONZERO_OR_ONE(old_percentile[j]-old_percentile[j-1]));
            }
            merge_flag = 0;
            for (j=1;j<=percentile_num;j++)
            {
              if (percentile_scale[j] < 1.0)
                merge_flag = 1;
            }
          }
        }
      }
      for (j=1;j<=percentile_num;j++)
      {
        printf("Scaling factor %d: %f\n",j,percentile_scale[j]);fflush(stdout);
      }
      if (!extrapolate) {
        real_new_max = new_max;
        scale_and_write(filename_in,filename_out,Scalepercentile16,Scalepercentile8,1);
      } else {
        real_new_max = (unsigned)((double)(max_dens-old_percentile[percentile_num])*percentile_scale[percentile_num]+new_percentile[percentile_num]);
        printf("Extrapolated maximum: %u\n",real_new_max);fflush(stdout);
        scale_and_write(filename_in,filename_out,Scalepercentileextra16,Scalepercentileextra8,1);
      }
      break;

    case 6:
      k=filename_arg_start;
      printf("Processing file %s\n",argv[k]);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      volume_mean=(unsigned)(sum/count);
      printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histpercentile(); /* only for getting old_min and old_max and scale values */ 

      for(j = 0;j<num_landmarks;j++)
        old_landmarks[j] = estimate_landmark(argv[k],argv[k+j+2]);

      /* order landmarks from small to big */
      for(j = 0;j<num_landmarks;j++)
        {
          l=j;
          for(i = j+1;i<num_landmarks;i++)
            if(old_landmarks[i]<old_landmarks[l]) l = i;
          if(l!=j)
            {
              temp_landmark = old_landmarks[j];
              old_landmarks[j] =  old_landmarks[l];
              old_landmarks[l] = temp_landmark;
            }
        }

      for(j = 0;j<num_landmarks;j++)
        {
          printf("Landmark %d:  %d \n",j,old_landmarks[j]);
          temp_landmark = (unsigned)((float)(old_landmarks[j] - old_min)*scale + new_min);
          printf("Scaled landmark %d:  %d \n",j,temp_landmark);
          printf("Standard landmark %d:  %d \n",j,mean_landmarks[j]);
        }

      for (j=0;j<num_landmarks;j++)
        {
          temp_old[j+1] = old_landmarks[j];
          temp_new[j+1] = mean_landmarks[j];
          new_landmarks[j] = mean_landmarks[j];
        }
      temp_old[0] = old_min;
      temp_new[0] = new_min;
      temp_old[num_landmarks+1] = old_max;
      temp_new[num_landmarks+1] = new_max;
      for (j=0;j<=num_landmarks;j++)
        {
          landmarks_scale[j] = (float)(temp_new[j+1]-temp_new[j])/(float)(temp_old[j+1]-temp_old[j]);
        }
      /*------------*/
      merge_flag = 0;
      for (j=0;j<=num_landmarks;j++)
      {
        if (landmarks_scale[j] < 1.0)
          merge_flag = 1;
      }
      //---
      if (merge_flag)
      {
        if (auto_rescale) {
          for (j=1;j<=num_landmarks;j++)
          {
            if (landmarks_scale[j] < 1.0)
            {
              printf("Scaling factor %d < 1.0; automatic rescaling applied\n", j);fflush(stdout);
            }
          }
        } else if (force_merge) 
        {
            for (j=1;j<=num_landmarks;j++)
              {
                if (landmarks_scale[j] < 1.0)
                  {
                    printf("Scaling factor %d < 1.0; force scaling with intensity merge\n", j);fflush(stdout);
                  }
              }
        } 
        else 
        {
          for (j=1;j<=num_landmarks;j++)
          {
            if (landmarks_scale[j] < 1.0)
            {
              printf("Scaling factor %d < 1.0; not scaling with intensity merge; exiting\n", j);fflush(stdout);
            exit(-1);
            }
          }
        }
        if (auto_rescale) {
          range_save = new_max-new_min;

          while (merge_flag) {
            new_max += range_save;

            for(j = 0;j<num_landmarks;j++)
              new_landmarks[j] = (unsigned)(new_min + (mean_landmarks[j]-new_min)*((double)(new_max-new_min)/(range_save)));

            for (j=0;j<num_landmarks;j++)
              {
                temp_new[j+1] = new_landmarks[j];
              }
            temp_new[0] = new_min;
            temp_new[num_landmarks+1] = new_max;
	    
            for (j=0;j<=num_landmarks;j++)
              {
                landmarks_scale[j] = (float)(temp_new[j+1]-temp_new[j])/(float)(temp_old[j+1]-temp_old[j]);
              }

            merge_flag = 0;
            for (j=1;j<=num_landmarks;j++)
            {
              if (landmarks_scale[j] < 1.0)
                merge_flag = 1;
            }
          }
        }
      }
      //----------------


      for (j=0;j<=num_landmarks;j++)
      {
        printf("Scaling factor %d: %f\n",j,landmarks_scale[j]);fflush(stdout);
      }
      if (!extrapolate) {
        real_new_max = new_max;
        scale_and_write(argv[k],argv[k+1],ScaleLandmarks16,ScaleLandmarks8,1);
      } else {
        real_new_max = (unsigned)((double)(max_dens-old_max)*landmarks_scale[num_landmarks]+new_max);
        printf("Extrapolated maximum: %u\n",real_new_max);fflush(stdout);
        scale_and_write(argv[k],argv[k+1],ScaleLandmarks16,ScaleLandmarks8,1);
      }
      break;
      /*------------*/


    case 0+INVERSE_METHOD:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmode_i();
      scale=(float)((double)(new_max-new_min)/NONZERO_OR_ONE(old_max-old_min));
      /*printf("Scaling factor: %f\n",scale);*/
      scale_and_write(filename_in,filename_out,Scalefull16,Scalefull8,1);
      break;
    case 1+INVERSE_METHOD:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmode_i();
      printf("Mode: %d\n", old_mode);fflush(stdout);
      printf("Scaled mode: %d\n", new_mode);fflush(stdout);
      lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
      rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      scale_and_write(filename_in,filename_out,Scalemode16,Scalemode8,1);
      break;
    case 2+INVERSE_METHOD:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmode_i();
      printf("Mode: %d\n", old_mode);fflush(stdout);
      printf("Scaled mode: %d\n", new_mode);fflush(stdout);
      lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
      rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      scale_and_write(filename_in,filename_out,Scalemode16,Scalemode8,1);
      break;
    case 3+INVERSE_METHOD:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmode2_i();
      printf("Mode: %d\n", old_mode);fflush(stdout);
      printf("Scaled mode: %d\n", new_mode);fflush(stdout);
      lscale=(float)((double)(new_mode-new_min)/NONZERO_OR_ONE(old_mode-old_min));
      rscale=(float)((double)(new_max-new_mode)/NONZERO_OR_ONE(old_max-old_mode));
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      scale_and_write(filename_in,filename_out,Scalemode16,Scalemode8,1);
      break;
    case 4+INVERSE_METHOD:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histmedian_i();
      printf("Median: %d\n", old_median);fflush(stdout);
      printf("Scaled median: %d\n", new_median);fflush(stdout);
      lscale=(float)((double)(new_median-new_min)/NONZERO_OR_ONE(old_median-old_min));
      rscale=(float)((double)(new_max-new_median)/NONZERO_OR_ONE(old_max-old_median));
      printf("Scaling factor 1: %f\n",lscale);fflush(stdout);
      printf("Scaling factor 2: %f\n",rscale);fflush(stdout);
      scale_and_write(filename_in,filename_out,Scalemedian16,Scalemedian8,1);
      break;
    case 5+INVERSE_METHOD:
      printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,0);
      find_histpercentile_i();
      for (j=1;j<percentile_num;j++)
      {
        printf("Percentile %d: %d\n", j, old_percentile[j]);fflush(stdout);
      }
      for (j=1;j<percentile_num;j++)
      {
        printf("Scaled percentile %d: %d\n", j, new_percentile[j]);fflush(stdout);
      }
      for (j=1;j<=percentile_num;j++)
      {
        percentile_scale[j]=(float)((double)(new_percentile[j]-new_percentile[j-1])/NONZERO_OR_ONE(old_percentile[j]-old_percentile[j-1]));
      }
      for (j=1;j<=percentile_num;j++)
      {
        printf("Scaling factor %d: %f\n",j,percentile_scale[j]);fflush(stdout);
      }
      scale_and_write(filename_in,filename_out,Scalepercentile16,Scalepercentile8,1);
      break;
  }
  return (0);
}


/*****************************************************************************
 * FUNCTION: run_inverse
 * DESCRIPTION: Gets histogram landmarks from a parameter file and an image
 *    and creates a new parameter file for the inverse transformation.
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: argc and argv should be given according to the
 *    Usage 3 message
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code -1 on error.
 * HISTORY:
 *    Created: 5/6/98 by Laszlo Nyul
 *    Modified: 5/7/98 call to read_paramfile added by Laszlo Nyul
 *
 *****************************************************************************/
 
int run_inverse(argc,argv)
int argc;
char *argv[];
{
  FILE *out1;
  int i,j,retval;
  char *filename_in;

  filename_in = argv[filename_arg_start];
  retval = read_paramfile(paramfile_in);
  if (retval) exit(retval);
printf("Method: %d\n", transformmethod);fflush(stdout);
  switch (transformmethod) {
    case 0:
printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,1);
      find_histmode();
      break;
    case 1:
printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,1);
      volume_mean= (unsigned)(sum/count);
printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histmode();
printf("Mode: %d\n", old_mode);fflush(stdout);
printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      break;
    case 2:
printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,1);
      find_histmode();
printf("Mode: %d\n", old_mode);fflush(stdout);
printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      break;
    case 3:
printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,1);
      volume_mean= (unsigned)(sum/count);
printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold2();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histmode2();
printf("Mode: %d\n", old_mode);fflush(stdout);
printf("Scaled mode: %d\n", scaled_mode);fflush(stdout);
printf("Mode mean: %d\n", mean_mode);fflush(stdout);
      break;
    case 4:
printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,1);
      volume_mean= (unsigned)(sum/count);
printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histmedian();
printf("Median: %d\n", old_median);fflush(stdout);
printf("Scaled median: %d\n", scaled_median);fflush(stdout);
printf("Median mean: %d\n", mean_median);fflush(stdout);
      break;
    case 5:
printf("Processing file %s\n",filename_in);fflush(stdout);
      read_and_stat(filename_in,Stat16,Stat8,1);
      volume_mean= (unsigned)(sum/count);
printf("Volume mean: %d\n", volume_mean);fflush(stdout);
      find_threshold();
      for (i=1;i<=threshold;i++) {
        dist[0]+=dist[i];
        dist[i]=0;
      }
      find_histpercentile();
      for (j=1;j<percentile_num;j++)
      {
printf("Percentile %d: %d\n", j, old_percentile[j]);fflush(stdout);
      }
      for (j=1;j<percentile_num;j++)
      {
printf("Scaled percentile %d: %d\n", j, scaled_percentile[j]);fflush(stdout);
        mean_percentile[j]+=scaled_percentile[j];
      }
      for (j=1;j<percentile_num;j++)
      {
printf("Percentile %d mean: %d\n", j, mean_percentile[j]);fflush(stdout);
      }
      break;
  }
  out1 = fopen(paramfile_out,"wb");
  switch (transformmethod) {
    case 0:
      fprintf(out1, "Method:                     %d\n", transformmethod
                                                        +INVERSE_METHOD);
      fprintf(out1, "First slice for hist:       %d\n", first_slice);
      fprintf(out1, "Last slice for hist:        %d\n", last_slice);
      fprintf(out1, "Minimum percentile:         %.2f\n", minile);
      fprintf(out1, "Maximum percentile:         %.2f\n", maxile);
      fprintf(out1, "Minimum scale:              %d\n", new_min);
      fprintf(out1, "Maximum scale:              %d\n", new_max);
      fprintf(out1, "Minimum mapped intensity:   %d\n", old_min);
      fprintf(out1, "Maximum mapped intensity:   %d\n", old_max);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:                %d\n", extrapolate);
      break;
    case 1:
      fprintf(out1, "Method:                     %d\n", transformmethod
                                                        +INVERSE_METHOD);
      fprintf(out1, "First slice for hist:       %d\n", first_slice);
      fprintf(out1, "Last slice for hist:        %d\n", last_slice);
      fprintf(out1, "Minimum percentile:         %.2f\n", minile);
      fprintf(out1, "Maximum percentile:         %.2f\n", maxile);
      fprintf(out1, "Minimum scale:              %d\n", new_min);
      fprintf(out1, "Maximum scale:              %d\n", new_max);
      fprintf(out1, "Mode mean:                  %d\n", mean_mode);
      fprintf(out1, "Minimum mapped intensity:   %d\n", old_min);
      fprintf(out1, "Maximum mapped intensity:   %d\n", old_max);
      fprintf(out1, "Mode intensity:             %d\n", old_mode);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:                %d\n", extrapolate);
      break;
    case 2:
      fprintf(out1, "Method:                     %d\n", transformmethod
                                                        +INVERSE_METHOD);
      fprintf(out1, "First slice for hist:       %d\n", first_slice);
      fprintf(out1, "Last slice for hist:        %d\n", last_slice);
      fprintf(out1, "Minimum percentile:         %.2f\n", minile);
      fprintf(out1, "Maximum percentile:         %.2f\n", maxile);
      fprintf(out1, "Minimum scale:              %d\n", new_min);
      fprintf(out1, "Maximum scale:              %d\n", new_max);
      fprintf(out1, "Mode mean:                  %d\n", mean_mode);
      fprintf(out1, "Minimum mapped intensity:   %d\n", old_min);
      fprintf(out1, "Maximum mapped intensity:   %d\n", old_max);
      fprintf(out1, "Mode intensity:             %d\n", old_mode);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:                %d\n", extrapolate);
      break;
    case 3:
      fprintf(out1, "Method:                     %d\n", transformmethod
                                                        +INVERSE_METHOD);
      fprintf(out1, "First slice for hist:       %d\n", first_slice);
      fprintf(out1, "Last slice for hist:        %d\n", last_slice);
      fprintf(out1, "Minimum percentile:         %.2f\n", minile);
      fprintf(out1, "Maximum percentile:         %.2f\n", maxile);
      fprintf(out1, "Minimum scale:              %d\n", new_min);
      fprintf(out1, "Maximum scale:              %d\n", new_max);
      fprintf(out1, "Mode mean:                  %d\n", mean_mode);
      fprintf(out1, "Minimum mapped intensity:   %d\n", old_min);
      fprintf(out1, "Maximum mapped intensity:   %d\n", old_max);
      fprintf(out1, "Mode intensity:             %d\n", old_mode);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:                %d\n", extrapolate);
      break;
    case 4:
      fprintf(out1, "Method:                     %d\n", transformmethod
                                                        +INVERSE_METHOD);
      fprintf(out1, "First slice for hist:       %d\n", first_slice);
      fprintf(out1, "Last slice for hist:        %d\n", last_slice);
      fprintf(out1, "Minimum percentile:         %.2f\n", minile);
      fprintf(out1, "Maximum percentile:         %.2f\n", maxile);
      fprintf(out1, "Minimum scale:              %d\n", new_min);
      fprintf(out1, "Maximum scale:              %d\n", new_max);
      fprintf(out1, "Median mean:                %d\n", mean_median);
      fprintf(out1, "Minimum mapped intensity:   %d\n", old_min);
      fprintf(out1, "Maximum mapped intensity:   %d\n", old_max);
      fprintf(out1, "Median intensity:           %d\n", old_median);
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:                %d\n", extrapolate);
      break;
    case 5:
      fprintf(out1, "Method:                     %d\n", transformmethod
                                                        +INVERSE_METHOD);
      fprintf(out1, "First slice for hist:       %d\n", first_slice);
      fprintf(out1, "Last slice for hist:        %d\n", last_slice);
      fprintf(out1, "Minimum percentile:         %.2f\n", minile);
      fprintf(out1, "Maximum percentile:         %.2f\n", maxile);
      fprintf(out1, "Minimum scale:              %d\n", new_min);
      fprintf(out1, "Maximum scale:              %d\n", new_max);
      fprintf(out1, "Percentile number:          %d\n", percentile_num);
      for (j=1;j<percentile_num;j++)
      {
        fprintf(out1, "Percentile %d mean:          %d\n", j, mean_percentile[j]);
      }
      fprintf(out1, "Minimum mapped intensity:   %d\n", old_min);
      fprintf(out1, "Maximum mapped intensity:   %d\n", old_max);
      for (j=1;j<percentile_num;j++)
      {
        fprintf(out1, "Percentile %d intensity:     %d\n", j, old_percentile[j]);
      }
      if (extrapolate == 1)
        fprintf(out1, "Extrapolate:                %d\n", extrapolate);
      break;
  }
  fclose(out1);
  return (0);
}


/*****************************************************************************
 * FUNCTION: usage
 * DESCRIPTION: Displays the usage message on stdout and exits with code 1.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS:
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code 1.
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 11/25/97 new method switch added by Laszlo Nyul
 *    Modified: 5/6/98 running mode 3 added by Laszlo Nyul
 *    Modified: 6/18/98 switches for checks for intensity merges
 *       added by Laszlo Nyul
 *
 *****************************************************************************/

void usage()
{
    printf("\n");
    printf("Usage 1: mrscaleprog -train -method <method> -percentile <min %%ile> <max %%ile> -new_scale <min scale> <max scale> [-percentage <min %%age> <max %%age>] [-percentile_num <%%ile num>] [-first_slice <first slice>] [-last_slice <last slice>] -paramfile_out <parameter file> [-landmark_number <num_landmarks>] -files [<input IM0 file> [BIM file1 BIM file2 ...]] ...\n");
    printf("<method>            transformation method to use (defaults to 1)\n");
    printf("   0:               do nothing but a linear scaling of the range\n");
    printf("   1:               find the second largest peak and match it to a standard location and do linear scaling of the ranges below and above the peak separately\n");
    printf("   2:               find the first largest peak and match it to a standard location and do linear scaling of the ranges below and above the peak separately\n");
    printf("   3:               find the second largest peak (within a limited range (min %%age, max %%age) and match it to a standard location and do linear scaling of the ranges below and above the peak separately\n");
    printf("   4:               find the median of the histogram after removing the first peak and match it to a standard location and do linear scaling of the ranges below and above the peak separately\n");
    printf("   5:               find the percentiles of the histogram after removing the first peak and match them to their standard location and do linear scaling of the ranges separately\n");
    printf("   6:               find the landmarks from the training tissue regions and match them to their standard locations and do linear scaling of the ranges separately\n");

    printf("for <method>=3:     -percentage is mandatory\n");
    printf("for <method>=5:     -percentile_num default is 4 (quartiles)\n");
	printf("for <method>=6:     -landmark_number is mandatory; <num_landmarks> BIM files must be specified for each input IM0 file\n");
    printf("<parameter file>    name of the file containing the parameters\n");
    printf("<min %%ile> and <max %%ile>       minimum and maximum percentile to scale\n");
    printf("<min scale> and <max scale>       minimum and maximum value of the new scale\n");
    printf("<min %%age> and <max %%age>       minimum and maximum percentage, search for the peaks only between them\n");
    printf("<%%ile num>          number of percentile landmarks used\n");
    printf("The percentile values are read as float, the scale values are read as integers.\n");
    printf("<input IM0 file>    name of the input file containing the training image\n");
    printf("<first slice>       first slice to use in histogram computation (in percentage of the total number of slices, default = 0)\n");
    printf("<last slice>        last slice to use in histogram computation (in percentage of the total number of slices, default = 100)\n");
    printf("\n");
    printf("Usage 2: mrscaleprog -apply [-force_merge | -auto_rescale] [-extrapolate | -no_extrapolate] [-dry_run] -paramfile_in <parameter file> -files <input IM0 file> <output IM0 file> [BIM file1 BIM file2 ...]\n");
    printf("<parameter file>    name of the file containing the parameters (generated by this program using -train mode)\n");
    printf("<input IM0 file>    name of the input file containing the original image\n");
    printf("<output IM0 file>   name of the output file storing the transformed image\n");
    printf("-force_merge        force using the scale range even if intensities merge\n");
    printf("-auto_rescale       automatically expand the scale if intensities would merge\n");
    printf("-dry_run            do not actually scale the image (in this case output file is not used)\n");
    printf("-extrapolate        extrapolate values above the maximum percentile instead of mapping to the maximum scale value\n");
    printf("-no_extrapolate     do not extrapolate values above the maximum percentile\n");
    printf("\n");
    printf("\n");
    printf("Usage 3: mrscaleprog -inverse -paramfile_in <parameter file> -paramfile_out <output parameter file> -files <input IM0 file>\n");
    printf("<parameter file>    name of the file containing the parameters\n");
    printf("<output parameter file>     name of the output file storing the parameters of the inverse transform\n");
    printf("<input IM0 file>    name of the input file containing the original image\n");
    exit(1);
}

void parse_commandline(argc, argv)
  int argc;
  char *argv[];
{
  int args_parsed;

  if (argc < 2)
    usage();
  args_parsed = 1;
  while (args_parsed < argc) {
    if (strcmp(argv[args_parsed], "-help") == 0) {
      usage();
    }
    else
    if (strcmp(argv[args_parsed], "-train") == 0) {
      args_parsed++;
      runmode = 1;
      paramfile_out_r = 1;
    }
    else
    if (strcmp(argv[args_parsed], "-apply") == 0) {
      args_parsed++;
      runmode = 2;
      paramfile_in_r = 1;
    }
    else
    if (strcmp(argv[args_parsed], "-inverse") == 0) {
      args_parsed++;
      runmode = 3;
      paramfile_in_r = 1;
      paramfile_out_r = 1;
    }
    else
    if (strcmp(argv[args_parsed], "-method") == 0) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &transformmethod);
      switch (transformmethod) {
        case 0:
          percentile_r = 1;
          new_scale_r = 1;
          break;
        case 1:
          percentile_r = 1;
          new_scale_r = 1;
          break;
        case 2:
          percentile_r = 1;
          new_scale_r = 1;
          break;
        case 3:
          percentile_r = 1;
          new_scale_r = 1;
          percentage_r = 1;
          break;
        case 4:
          percentile_r = 1;
          new_scale_r = 1;
          break;
        case 5:
          percentile_r = 1;
          new_scale_r = 1;
          percentile_num_r = 1;
          break;
        case 6:
          percentile_r = 1;
          new_scale_r = 1;
          break;
        default:
          printf("Unknown method #: %d\n", transformmethod);
          usage();
      }
    }
    else
    if (!strcmp(argv[args_parsed],"-first_slice")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &first_slice);
    }
    else
    if (!strcmp(argv[args_parsed],"-last_slice")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &last_slice);
    }
    else
    if (!strcmp(argv[args_parsed],"-percentage")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%f", &minage);
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%f", &maxage);
    }
    else
    if (!strcmp(argv[args_parsed],"-percentile")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%f", &minile);
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%f", &maxile);
    }
    else
    if (!strcmp(argv[args_parsed],"-new_scale")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &new_min);
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &new_max);
    }
    else
    if (!strcmp(argv[args_parsed],"-force_merge")) {
      args_parsed++;
      force_merge=1;
    }
    else
    if (!strcmp(argv[args_parsed],"-percentile_num")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &percentile_num);
    }
    else
    if (!strcmp(argv[args_parsed],"-auto_rescale")) {
      args_parsed++;
      auto_rescale=1;
    }
    else
    if (!strcmp(argv[args_parsed],"-dry_run")) {
      args_parsed++;
      dry_run=1;
    }
    else
    if (!strcmp(argv[args_parsed],"-extrapolate")) {
      args_parsed++;
      force_extrapolate=1;
    }
    else
    if (!strcmp(argv[args_parsed],"-no_extrapolate")) {
      args_parsed++;
      force_no_extrapolate=1;
    }
    else
    if (!strcmp(argv[args_parsed],"-paramfile_in")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      paramfile_in=argv[args_parsed++];
    }
    else
    if (!strcmp(argv[args_parsed],"-paramfile_out")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      paramfile_out=argv[args_parsed++];
    }
    /* added by Ying for method 6*/
    else
    if (!strcmp(argv[args_parsed],"-landmark_number")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      sscanf(argv[args_parsed++], "%d", &num_landmarks);
    }
    else
    if (!strcmp(argv[args_parsed],"-files")) {
      args_parsed++;
      if (args_parsed >= argc) {
        printf("Unexpected end of commandline\n");
        usage();
      }
      filename_arg_start = args_parsed;
      break;
    }
    else
    {
      printf("Unknown option: %s\n", argv[args_parsed]);
      usage();
    }
  }
  if (force_merge && auto_rescale) {
    printf("Only one of -force_merge and -auto_rescale can be specified.\n");
    usage();
  }
  if (filename_arg_start < 0) {
    printf("-files must be specified\n");
    usage();
  }
  if (percentile_r && minile < 0) {
    minile = 0;
    maxile = 100;
  }
  if (percentage_r && minage < 0) {
    minage = 0;
    maxage = 100;
  }
  if (new_scale_r && new_min < 0) {
    printf("-new_scale must be specified\n");
    usage();
  }
  if (paramfile_in_r && paramfile_in == 0) {
    printf("-paramfile_in must be specified\n");
    usage();
  }
  if (paramfile_out_r && paramfile_out == 0) {
    printf("-paramfile_out must be specified\n");
    usage();
  }
}

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Implements MR image intensity transformation based on
 *    histogram landmarks.
 *    It has two different running modes depending on the calling parameters.
 *    In mode 1. it does the training to get the transformation parameters.
 *    In mode 2. it applies the actual transformation to an image.
 *    In mode 3. it creates the inverse parameter file from a parameter
 *               file and an image.
 * PARAMETERS:
 *    argc, argv
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with 0 on normal completion.
 * HISTORY:
 *    Created: 11/19/97 by Laszlo Nyul
 *    Modified: 5/6/98 running mode 3 added by Laszlo Nyul
 *
 *****************************************************************************/

int main(argc,argv)
int argc;
char *argv[];
{
  parse_commandline(argc, argv);
  switch (runmode) {
    case 1:
      run_train(argc, argv);
      break;
    case 2:
      run_apply(argc, argv);
      break;
    case 3:
      run_inverse(argc, argv);
      break;
  }
  exit(0);
}

