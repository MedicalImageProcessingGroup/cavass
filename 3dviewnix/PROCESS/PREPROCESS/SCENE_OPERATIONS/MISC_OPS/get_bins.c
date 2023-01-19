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
#include <assert.h>

void accumulate(int sum, int diff, int on_surface);
void destroy_scene_header(ViewnixHeader *vh);

static double OffSumHist[0x20000], OffDiffHist[0x10000];
static double OnSumHist[0x20000], OnDiffHist[0x10000];

int main(argc,argv)
int argc;
char *argv[];
{
  int j, error, slices1, slices2, num_bins, clip_flag;
  unsigned k;
  ViewnixHeader vh1, vh2;
  FILE *in1, *in2;
  char group[6], elem[6];
  int ns; // number of samples
  double tmp_count, sum_prod_sum, diff_prod_sum, clip_level;

  if (argc<4 || (num_bins=atoi(argv[argc-1]))<2) {
    fprintf(stderr, "Usage: get_bins <input BIM_file> ... <feature IM0_file> ... <bins per feature>\n");
	fprintf(stderr, " The list of samples of ");
	fprintf(stderr, "the object to be modeled must be followed by the ");
	fprintf(stderr, "corresponding feature scenes.\n");
    exit(-1);
  }
  if (argc%2)
  {
    fprintf(stderr, "Specify feature scene for each subject.\n");
    exit(-1);
  }
  ns = (argc-2)/2;
  for (j=0; j<ns; j++)
  {
	in2 = fopen(argv[ns+j+1], "rb");
	if (in2==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
	error=VReadHeader(in2,&vh2,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
    if (vh2.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
    if (vh2.scn.dimension != 3)
	{
	  fprintf(stderr, "Input file should be 3-D\n");
	  exit(-1);
	}
	fclose(in2);
	destroy_scene_header(&vh2);
  }
  for (j=0; j<ns; j++)
  {
	int ii, jj, kk, a, b, slice_size, low, high, ai, bi;
	unsigned short *d2_16;
	unsigned char *d1_1, *d2_8;

    memset(&vh1, 0, sizeof(vh1));
	in1 = fopen(argv[j+1], "rb");
    if (in1==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
    error=VReadHeader(in1,&vh1,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
    if (vh1.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	slices1 = vh1.scn.num_of_subscenes[0];
	slice_size = vh1.scn.xysize[0]*vh1.scn.xysize[1];
	in2 = fopen(argv[ns+j+1], "rb");
	if (in2==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
	error=VReadHeader(in2,&vh2,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
    if (vh2.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	slices2 = vh2.scn.num_of_subscenes[0];
	if (slices2<slices1 ||
		vh2.scn.xysize[0]!=vh1.scn.xysize[0] ||
		vh2.scn.xysize[1]!=vh1.scn.xysize[1])
	{
	  fprintf(stderr, "Different scene sizes not implemented yet.\n");
	  exit(-1);
	}

    d1_1=(unsigned char *)malloc((slice_size+7)/8*2);
    VSeekData(in1,0);
	VSeekData(in2,0);
	if (VReadData((char *)d1_1, 1,
		(slice_size+7)/8, in1, &error))
	{
	  fprintf(stderr, "Could not read data\n");
	  exit(-1);
	}
	if (vh2.scn.num_of_bits == 16)
	{
	  d2_16 = (unsigned short *)malloc(slice_size*4);
	  for (kk=0; kk<slices2; kk++)
	  {
		if (kk == 0)
		{
		  if (VReadData((char *)d2_16, 2, slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		}
		else
		{
		  memcpy(d1_1, d1_1+(slice_size+7)/8, (slice_size+7)/8);
		  memcpy(d2_16, d2_16+slice_size, slice_size*2);
		}
		if (kk < slices2-1)
		{
		  if (kk >= slices1-1)
		    memset(d1_1+(slice_size+7)/8, 0, (slice_size+7)/8);
		  else if (VReadData((char *)d1_1+(slice_size+7)/8, 1,
			  (slice_size+7)/8, in1, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  if (VReadData((char *)(d2_16+slice_size), 2,slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  // z-faces
		  for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		    for (ii=0; ii<vh1.scn.xysize[0]; ii++)
			{
			  ai = vh1.scn.xysize[0]*jj+ii;
			  bi = ai+slice_size;
			  a = d2_16[ai];
			  b = d2_16[bi];
			  if (a > b)
			  	high=a, low=b;
			  else
			    low=a, high=b;
			  if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			      ((d1_1[ai/8+(slice_size+7)/8]&(128>>(ai%8)))>0))
				accumulate(high+low, high-low, TRUE);
			  else
			    accumulate(high+low, high-low, FALSE);
			}
		}
		// y-faces
		for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]-1; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+vh1.scn.xysize[0];
			a = d2_16[ai];
			b = d2_16[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(high+low, high-low, TRUE);
			else
			  accumulate(high+low, high-low, FALSE);
		  }
		// x-faces
		for (jj=0; jj<vh1.scn.xysize[1]-1; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+1;
			a = d2_16[ai];
			b = d2_16[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(high+low, high-low, TRUE);
			else
			  accumulate(high+low, high-low, FALSE);
		  }
	  }
	  free(d2_16);
	}
	else
	{
	  d2_8 = (unsigned char *)malloc(slice_size*2);
	  for (kk=0; kk<slices2; kk++)
	  {
	    if (kk == 0)
		{
		  if (VReadData((char *)d2_8, 1, slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		}
		else
		{
		  memcpy(d1_1, d1_1+(slice_size+7)/8, (slice_size+7)/8);
		  memcpy(d2_8, d2_8+slice_size, slice_size);
		}
		if (kk < slices2-1)
		{
		  if (VReadData((char *)d1_1+(slice_size+7)/8, 1,
			  (slice_size+7)/8, in1, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  if (kk >= slices1-1)
		    memset(d1_1+(slice_size+7)/8, 0, (slice_size+7)/8);
		  else if (VReadData((char *)d2_8+slice_size, 1, slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  // z-faces
		  for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		    for (ii=0; ii<vh1.scn.xysize[0]; ii++)
			{
			  ai = vh1.scn.xysize[0]*jj+ii;
			  bi = ai+slice_size;
			  a = d2_8[ai];
			  b = d2_8[bi];
			  if (a > b)
			  	high=a, low=b;
			  else
			    low=a, high=b;
			  if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			      ((d1_1[ai/8+(slice_size+7)/8]&(128>>(ai%8)))>0))
				accumulate(high+low, high-low, TRUE);
			  else
			    accumulate(high+low, high-low, FALSE);
			}
		}
		// y-faces
		for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]-1; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+vh1.scn.xysize[0];
			a = d2_8[ai];
			b = d2_8[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(high+low, high-low, TRUE);
			else
			  accumulate(high+low, high-low, FALSE);
		  }
		// x-faces
		for (jj=0; jj<vh1.scn.xysize[1]-1; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+1;
			a = d2_8[ai];
			b = d2_8[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(high+low, high-low, TRUE);
			else
			  accumulate(high+low, high-low, FALSE);
		  }
	  }
	  free(d2_8);
	}
	fclose(in1);
    fclose(in2);
	free(d1_1);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
  }
  for (sum_prod_sum=j=0; j<0x20000; j++)
	sum_prod_sum += OffSumHist[j]*OnSumHist[j];
  clip_level = floor(sum_prod_sum/num_bins);
  for (k=0; (int)k<num_bins; k++)
  {
    clip_flag = FALSE;
    for (sum_prod_sum=j=0; j<0x20000; j++)
    {
      if (OffSumHist[j]*OnSumHist[j] <= clip_level)
        sum_prod_sum += OffSumHist[j]*OnSumHist[j];
      else
      {
        sum_prod_sum += clip_level;
        clip_flag = TRUE;
      }
    }
	if (clip_flag)
	  clip_level = floor(sum_prod_sum/num_bins);
	else
	  break;
  }
  printf("sum levels:\n");
  for (j=1,k=0,tmp_count=0; j<=num_bins; j++)
  {
    while (k<0x20000 && tmp_count*num_bins<sum_prod_sum*j)
	{
	  tmp_count += OffSumHist[k]*OnSumHist[k]<=clip_level?
	    OffSumHist[k]*OnSumHist[k]: clip_level;
	  k++;
	}
	printf(" %d", k);
  }
  for (diff_prod_sum=j=0; j<0x10000; j++)
	diff_prod_sum += OffDiffHist[j]*OnDiffHist[j];
  clip_level = floor(diff_prod_sum/num_bins);
  for (k=0; (int)k<num_bins; k++)
  {
    clip_flag = FALSE;
    for (diff_prod_sum=j=0; j<0x10000; j++)
    {
      if (OffDiffHist[j]*OnDiffHist[j] <= clip_level)
        diff_prod_sum += OffDiffHist[j]*OnDiffHist[j];
      else
      {
        diff_prod_sum += clip_level;
        clip_flag = TRUE;
      }
    }
	if (clip_flag)
	  clip_level = floor(diff_prod_sum/num_bins);
	else
	  break;
  }
  printf("\ndifference levels:\n");
  for (j=1,k=0,tmp_count=0; j<=num_bins; j++)
  {
    while (k<0x10000 && tmp_count*num_bins<diff_prod_sum*j)
	{
	  tmp_count += OffDiffHist[k]*OnDiffHist[k]<=clip_level?
	    OffDiffHist[k]*OnDiffHist[k]: clip_level;
	  k++;
	}
	printf(" %d", k);
  }
  printf("\n");
  exit(0);
}

void accumulate(int sum, int diff, int on_surface)
{
	assert(sum >= 0);
	assert(diff >= 0);
	assert(sum < 2*65536);
	assert(diff < 65536);
	if (on_surface)
	{
		OnSumHist[sum]++;
		OnDiffHist[diff]++;
	}
	else
	{
		OffSumHist[sum]++;
		OffDiffHist[diff]++;
	}
}


/*****************************************************************************
 * FUNCTION: destroy_scene_header
 * DESCRIPTION: Frees the memory referenced by the pointer fields of a scene
 *    file header.
 * PARAMETERS:
 *    vh: The scene file header.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The pointer fields of vh must point to allocated memory or
 *    be NULL.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/11/93 by Dewey Odhner
 *
 *****************************************************************************/
void destroy_scene_header(ViewnixHeader *vh)
{
	if (vh->gen.description)
		free(vh->gen.description);
	if (vh->gen.comment)
		free(vh->gen.comment);
	if (vh->gen.imaged_nucleus)
		free(vh->gen.imaged_nucleus);
	if (vh->gen.gray_lookup_data)
		free(vh->gen.gray_lookup_data);
	if (vh->gen.red_lookup_data)
		free(vh->gen.red_lookup_data);
	if (vh->gen.green_lookup_data)
		free(vh->gen.green_lookup_data);
	if (vh->gen.blue_lookup_data)
		free(vh->gen.blue_lookup_data);
	if (vh->scn.domain)
		free(vh->scn.domain);
	if (vh->scn.axis_label)
		free(vh->scn.axis_label);
	if (vh->scn.measurement_unit)
		free(vh->scn.measurement_unit);
	if (vh->scn.density_measurement_unit)
		free(vh->scn.density_measurement_unit);
	if (vh->scn.smallest_density_value)
		free(vh->scn.smallest_density_value);
	if (vh->scn.largest_density_value)
		free(vh->scn.largest_density_value);
	if (vh->scn.signed_bits)
		free(vh->scn.signed_bits);
	if (vh->scn.bit_fields)
		free(vh->scn.bit_fields);
	if (vh->scn.num_of_subscenes)
		free(vh->scn.num_of_subscenes);
	if (vh->scn.loc_of_subscenes)
		free(vh->scn.loc_of_subscenes);
	if (vh->scn.description)
		free(vh->scn.description);
	memset(vh, 0, sizeof(*vh));
}
