/*
  Copyright 1993-2017 Medical Image Processing Group
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
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif
#include <cv3dv.h>

#include "hheap.h"

#include "render/AtoM.cpp"
#include "render/matrix.cpp"
#include "gqueue.h"

#define NUM_FEATURES 7
#define MULTITISSUE 8
#define MAX_CONNECTIVITY 65534
#define AFF_UNDEF 65535

#define BRIGHTEST 1
#define DARKEST -1

typedef unsigned short OutCellType;

typedef struct {
  short x, y, z;
} Voxel;

typedef struct {
  int xdim, ydim, zdim;
  long slice_size, volume_size;
  double voxelsize_x, voxelsize_y, voxelsize_z;
} S_dimensions;


void fuzzy_track_hheap(int), fuzzy_track_chash(int), fuzzy_track_chash2(int);
void load_volume(int), load_fom(), load_dfom(), load_feature_map(),
    GC_max(int current_volume), MOFS(int current_volume);
int affinity(int a, int b, float adjacency, int ax, int ay, int az,
	int bx, int by, int bz, int back);
int fom_value(int col, int row, int slc);
int dfom_value(int col, int row, int slc);
int input_slice_index(int volume, int slice);
void parse_command_line(int argc, char *argv[]), handle_error(int);
void repush_xyz_chash(short, short, short, unsigned short, unsigned short);
void repush_xyz_chash2(short, short, short, unsigned short, unsigned short);
void repush_xyz_hheap(short x, short y, short z, unsigned short w);
int get_tissues(char tf[]);

static char *command_line(int argc, char **argv);


static void *H;

S_dimensions dimensions;
Voxel nbor[6] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },  { -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 } };


FileInfo file_info;
char *output_filename, *input_filename, *argv0, *histogram_filename,
	*points_filename, *fom_filename, *bg_filename, *feature_map_filename,
	*threeD_hist_filename, *dfom_filename;
ViewnixHeader vh_in, vh_out, vh_fom, vh_pts, vh_bg_pts;
int function_selected[NUM_FEATURES+1], feature_status[NUM_FEATURES+2], bg_flag;
float weight[NUM_FEATURES+2]={1, 1, 1, 1, 1, 1, 1, 1, 1},
    function_level[NUM_FEATURES+1], function_width[NUM_FEATURES+1], threshold,
	count_affinity /* affinity per count on scale of 1 */,
	*tissue_level, *tissue_width;
int (*point_picked)[3], num_points_picked, mask_original,
	affinity_type /* 0=parametric, 1=histogram, 2=covariance, 3=model based,
		4=dual object */,
	histogram_bins[2], *histogram_counts, *reverse_histogram_counts,
	largest_density_value, covariance_flag, reverse_covariance_flag,
	(*bg_point)[3], num_bg_points, num_tissues, *tissue_type, fg_tissues;
void *in_data;
#define in_data_8 ((unsigned char *)in_data)
#define in_data_16 ((unsigned short *)in_data)
OutCellType *out_data;
double training_mean[NUM_FEATURES], reverse_training_mean[NUM_FEATURES],
	inv_covariance[NUM_FEATURES*NUM_FEATURES],
	inv_reverse_covariance[NUM_FEATURES*NUM_FEATURES];
int out_affinity_flag[3]={1, 1, 0};
int fuzzy_adjacency_flag;
int track_algorithm=1; /* 0=hheap; 1=chash ; 2=chash2 */
float slice_spacing, weight_unit, i_weight_unit;
double rel_scale, translation[3];
double centroid[3];
unsigned short *fom_data, *dfom_data;
double rotation[3][3];
int rotate_flag, prescaled;
double fb, ft;
int ioffset[3];
static unsigned short *feature_map_buf, *feature_map[0x1000], feature_map_max;
static unsigned short gray_map[0x10000];
static int *hist_bin_table[3], bins_per_feature;
static unsigned short ***threeD_hist;
static int mofs_flag;
static FILE *outstream;


/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Performs 3-dimensional fuzzy tracking on an 8- or 16-bit per
 *    cell IM0 file to produce an IM0 file of the connectivity scene, or
 *    or original values in the connected region.
 * PARAMETERS:
 *    argc: the number of command line arguments
 *    argv: the command line arguments
 * SIDE EFFECTS: The histogram file will be removed.
 * ENTRY CONDITIONS: The environment variable VIEWNIX_ENV must be set to the
 *    3DVIEWNIX system directory.  If histogram is specified, the histogram
 *    file must contain first two int's being dimensions of the histogram (high
 *    bins and low bins respectively), then the int counts in each bin (all the
 *    low bins in the first high bin, then all the low bins in the next high
 *    bin, etc.); then, if a reverse histogram is to be used, the counts in
 *    the reverse histogram in the same order..
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code 1 on error.
 * HISTORY:
 *    Created: 3/28/96 by Dewey Odhner
 *    Modified: 4/17/96 output of original values allowed by Dewey Odhner
 *    Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
 *    Modified: 5/31/96 dual-histogram-type affinity allowed by Dewey Odhner
 *    Modified: 7/25/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 11/27/96 .HST file not removed by Dewey Odhner
 *    Modified: 2/24/98 points loaded from binary scene by Dewey Odhner
 *
 *****************************************************************************/
int main(int argc, char *argv[])
{
	int error_code, j, k, slices_out, volumes_out, current_volume, max_count;
	char group[5], element[5];
	FILE *fp;
	long offset;

	assert((OutCellType)MAX_CONNECTIVITY == MAX_CONNECTIVITY);
	argv0 = argv[0];
	parse_command_line(argc, argv);
    if (bg_flag == 1)
	    VAddBackgroundProcessInformation(argv[0]);
	if (affinity_type == 1)
	{
		fp = fopen(histogram_filename, "rb");
		if (fp == NULL)
			handle_error(4);
		if (fread(histogram_bins, sizeof(histogram_bins[0]), 2, fp) != 2)
			handle_error(2);
		histogram_counts =
			(int *)malloc(histogram_bins[0]*histogram_bins[1]*sizeof(int));
		if (histogram_counts == NULL)
			handle_error(1);
		if (fread(histogram_counts, sizeof(int),
				histogram_bins[0]*histogram_bins[1], fp) !=
				histogram_bins[0]*histogram_bins[1])
			handle_error(2);
		offset = ftell(fp);
		if (fseek(fp, 0, 2))
			handle_error(5);
		if (ftell(fp) != offset)
		{
			if (fseek(fp, offset, 0))
				handle_error(5);
			reverse_histogram_counts =
				(int *)malloc(histogram_bins[0]*histogram_bins[1]*sizeof(int));
			if (reverse_histogram_counts == NULL)
				handle_error(1);
			if (fread(reverse_histogram_counts, sizeof(int),
					histogram_bins[0]*histogram_bins[1], fp) !=
					histogram_bins[0]*histogram_bins[1])
				handle_error(2);
		}
		fclose(fp);
		if (strlen(histogram_filename)<4 || strcmp(histogram_filename+
				strlen(histogram_filename)-4, ".HST"))
			unlink(histogram_filename);
		max_count = histogram_counts[0];
		for (j=1; j<histogram_bins[0]*histogram_bins[1]; j++)
			if (histogram_counts[j] > max_count)
				max_count = histogram_counts[j];
		count_affinity = (float)(1./max_count);
	}
	fp = fopen(input_filename, "rb");
	if (fp == NULL)
		handle_error(4);
	error_code = VReadHeader(fp, &vh_in, group, element);
	switch (error_code)
	{
		case 0:
		case 106:
		case 107:
			break;
		default:
			fprintf(stderr, "file = %s; group = %s; element = %s\n",
				input_filename, group, element);
			handle_error(error_code);
	}
	fclose(fp);

	if (!(vh_in.scn.dimension == file_info.num_of_elem+2) ||
			!(vh_in.scn.num_of_density_values == 1) ||
			!(vh_in.scn.num_of_integers == 1) ||
			!(vh_in.scn.signed_bits_valid==0 || vh_in.scn.signed_bits[0]==0) ||
			!(vh_in.scn.num_of_bits==8 || vh_in.scn.num_of_bits==16) ||
			!(vh_in.scn.bit_fields[0] == 0) ||
			!(vh_in.scn.bit_fields[1] == vh_in.scn.num_of_bits-1) ||
			!(vh_in.scn.bytes_in_alignment_valid==0 ||
			vh_in.scn.bytes_in_alignment<=1))
	{
		fprintf(stderr, "Not able to classify this scene.\n");
		exit(-1);
	}

    largest_density_value = vh_in.scn.largest_density_value_valid?
		(int)vh_in.scn.largest_density_value[0]: (1<<vh_in.scn.num_of_bits)-1;
	memcpy(&vh_out, &vh_in, sizeof(vh_in));
	strncpy(vh_out.gen.filename, output_filename, sizeof(vh_out.gen.filename));
	if (!mask_original)
	{
		vh_out.scn.num_of_bits = 8*sizeof(OutCellType);
		vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits-1;
		vh_out.scn.smallest_density_value = (float *)malloc(sizeof(float));
		vh_out.scn.largest_density_value = (float *)malloc(sizeof(float));
		if (vh_out.scn.smallest_density_value==NULL ||
				vh_out.scn.largest_density_value==NULL)
			handle_error(1);
		vh_out.scn.smallest_density_value[0] = 0;
		vh_out.scn.largest_density_value[0] = MAX_CONNECTIVITY;
		vh_out.scn.smallest_density_value_valid = TRUE;
		vh_out.scn.largest_density_value_valid = TRUE;
	}
	if (file_info.max[0] < 0)
		file_info.max[0] = vh_in.scn.num_of_subscenes[0]-1;
	slices_out = 1+(file_info.max[0]-file_info.min[0])/file_info.incr[0];
	if (vh_in.scn.dimension == 3)
	{
		volumes_out = 1;
		vh_out.scn.num_of_subscenes = (short *)malloc(sizeof(short));
		if (vh_out.scn.num_of_subscenes == NULL)
			handle_error(1);
		vh_out.scn.num_of_subscenes[0] = slices_out;
		vh_out.scn.loc_of_subscenes= (float *)malloc(slices_out*sizeof(float));
		if (vh_out.scn.loc_of_subscenes == NULL)
			handle_error(1);
		for (j=0; j<slices_out; j++)
			vh_out.scn.loc_of_subscenes[j] =
				vh_in.scn.loc_of_subscenes[input_slice_index(0, j)];
		slice_spacing = slices_out>1?
			(vh_out.scn.loc_of_subscenes[slices_out-1]-
			vh_out.scn.loc_of_subscenes[0])/(slices_out-1): 1;
	}
	else
	{
		volumes_out = 1+(file_info.max[1]-file_info.min[1])/file_info.incr[1];
		vh_out.scn.num_of_subscenes =
			(short *)malloc((1+volumes_out)*sizeof(short));
		if (vh_out.scn.num_of_subscenes == NULL)
			handle_error(1);
		vh_out.scn.num_of_subscenes[0] = volumes_out;
		for (j=1; j<=volumes_out; j++)
			vh_out.scn.num_of_subscenes[j] = slices_out;
		vh_out.scn.loc_of_subscenes =
			(float *)malloc(volumes_out*(1+slices_out)*sizeof(float));
		if (vh_out.scn.loc_of_subscenes == NULL)
			handle_error(1);
		for (j=0; j<volumes_out; j++)
			vh_out.scn.loc_of_subscenes[j] = vh_in.scn.loc_of_subscenes[
				file_info.min[1]+j*file_info.incr[1]];
		for (current_volume=0; current_volume<volumes_out; current_volume++)
			for (j=0; j<slices_out; j++)
			{
				k = input_slice_index(current_volume, j);
				vh_out.scn.loc_of_subscenes[
					volumes_out+current_volume*slices_out+j] =
					k>=0?
					vh_in.scn.loc_of_subscenes[vh_in.scn.num_of_subscenes[0]+k]
					: vh_out.scn.loc_of_subscenes[volumes_out+
					  current_volume*slices_out+j-1];
			}
		slice_spacing = slices_out>1?
			(vh_out.scn.loc_of_subscenes[slices_out]-
			vh_out.scn.loc_of_subscenes[1])/(slices_out-1): 1;
	}
	vh_out.scn.description = command_line(argc, argv);
	vh_out.scn.description_valid = 1;
	fp = fopen(output_filename, "w+b");
	if (fp == NULL)
		handle_error(4);
	handle_error(VWriteHeader(fp, &vh_out, group, element));
	in_data = malloc(slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
		vh_in.scn.num_of_bits/8);
	out_data = (OutCellType *)malloc(slices_out*vh_in.scn.xysize[0]*
		vh_in.scn.xysize[1]*sizeof(OutCellType));
	if (in_data==NULL || out_data==NULL)
		handle_error(1);
	if (points_filename && fom_filename)
	{
		FILE *fp_pts, *fp_bg_pts;
		int m, n, b;
		unsigned char *pts_data, *bg_pts_data;
		double inv_scale=1/rel_scale, slice_rate;

		fp_pts = fopen(points_filename, "rb");
		handle_error(VReadHeader(fp_pts, &vh_pts, group, element));
		if (vh_pts.scn.num_of_bits!=1 || vh_in.scn.dimension!=3)
		{
			fprintf(stderr, "Points file is incompatible.\n");
			handle_error(102);
		}
		slice_rate = vh_in.scn.num_of_subscenes[0]<2? 1:
			1/(vh_in.scn.loc_of_subscenes[1]-vh_in.scn.loc_of_subscenes[0]);
		pts_data = (unsigned char *)malloc((vh_pts.scn.xysize[0]*
			vh_pts.scn.xysize[1]+7)/8);
		if (pts_data == NULL)
			handle_error(1);
		handle_error(VSeekData(fp_pts, 0));
		for (j=0; j<vh_pts.scn.num_of_subscenes[0]; j++)
		{
			handle_error(VReadData((char *)pts_data, 1, (vh_pts.scn.xysize[0]*
				vh_pts.scn.xysize[1]+7)/8, fp_pts, &k));
			for (k=n=0,b=128; k<vh_pts.scn.xysize[1]; k++)
				for (m=0; m<vh_pts.scn.xysize[0]; m++)
				{
					if (pts_data[n] & b)
					{
					  double x[3], x1[3], x2[3], x3[3], x4[3];
					  int p;
					  if (num_points_picked == 0)
					    point_picked = (void *)malloc(sizeof(point_picked[0]));
					  else
					    point_picked = (void *)realloc(point_picked,
						  (num_points_picked+1)*sizeof(point_picked[0]));
					  if (point_picked == NULL)
					    handle_error(1);
					  if (prescaled &&
					      j+ioffset[2]>=0 && j+ioffset[2]<slices_out &&
						  k+ioffset[1]>=0 && k+ioffset[1]<vh_in.scn.xysize[1]&&
						  m+ioffset[0]>=0 && m+ioffset[0]<vh_in.scn.xysize[0])
					  {
					   point_picked[num_points_picked][0] = m+ioffset[0];
					   point_picked[num_points_picked][1] = k+ioffset[1];
					   point_picked[num_points_picked][2] = j+ioffset[2];
					   num_points_picked++;
					  }
					  else
					  {
					   if (vh_in.scn.domain_valid)
					   {
					    for (p=0; p<3; p++)
					      x4[p]= m*vh_pts.scn.xypixsz[0]*vh_in.scn.domain[3+p]+
						    k*vh_pts.scn.xypixsz[1]*vh_in.scn.domain[6+p]+
						    vh_pts.scn.loc_of_subscenes[j]*
							vh_in.scn.domain[9+p];
					    if (rotate_flag)
						{
						  for (p=0; p<3; p++)
						    x3[p] = inv_scale*(x4[p]+vh_pts.scn.domain[p])-
						      centroid[p];
						  vector_matrix_multiply(x2, x3, rotation);
						  for (p=0; p<3; p++)
						    x1[p] = x2[p]+centroid[p]+translation[p];
					    }
						else
						  for (p=0; p<3; p++)
						    x1[p] = inv_scale*(x4[p]+vh_pts.scn.domain[p])+
						      translation[p];
					    for (p=0; p<3; p++)
						  x[p] = (x1[0]-vh_in.scn.domain[0])*
						         vh_in.scn.domain[3+3*p]+
						         (x1[1]-vh_in.scn.domain[1])*
								 vh_in.scn.domain[4+3*p]+
						         (x1[2]-vh_in.scn.domain[2])*
								 vh_in.scn.domain[5+3*p];
					   }
					   else
					   {
					    x4[0] = m*vh_pts.scn.xypixsz[0];
						x4[1] = k*vh_pts.scn.xypixsz[1];
						x4[2] = vh_pts.scn.loc_of_subscenes[j];
					    if (rotate_flag)
						{
						  for (p=0; p<3; p++)
						    x3[p] = inv_scale*x4[p]-centroid[p];
						  vector_matrix_multiply(x2, x3, rotation);
						  for (p=0; p<3; p++)
						    x[p] = x2[p]+centroid[p]+translation[p];
					    }
						else
						  for (p=0; p<3; p++)
						    x[p] = inv_scale*x4[p]+translation[p];
					   }
					   x[2] -= vh_in.scn.loc_of_subscenes[0];
					   x[0] /= vh_in.scn.xypixsz[0];
					   x[1] /= vh_in.scn.xypixsz[1];
					   x[2] *= slice_rate;
					   // add seed point
					   for (p=0; p<3; p++)
					     point_picked[num_points_picked][p] = (int)rint(x[p]);
					   if (point_picked[num_points_picked][0]>=0 &&
					       point_picked[num_points_picked][0]<
						     vh_in.scn.xysize[0] &&
						   point_picked[num_points_picked][1]>=0 &&
						   point_picked[num_points_picked][1]<
						     vh_in.scn.xysize[1] &&
						   point_picked[num_points_picked][2]>=0 &&
						   point_picked[num_points_picked][2]<slices_out)
						 num_points_picked++;
					  }
					}
					b >>= 1;
					if (b == 0)
					{
						n++;
						b = 128;
					}
				}
		}
		fclose(fp_pts);
		free(pts_data);
		points_filename = NULL;
		if (bg_filename)
		{
			int joffset[3];
			fp_bg_pts = fopen(bg_filename, "rb");
			handle_error(VReadHeader(fp_bg_pts, &vh_bg_pts, group, element));
			if (vh_bg_pts.scn.num_of_bits!=1 || vh_in.scn.dimension!=3)
			{
				fprintf(stderr, "Points file is incompatible.\n");
				handle_error(102);
			}
			for (j=0; j<3; j++)
				joffset[j] = ioffset[j];
			if (vh_bg_pts.scn.domain_valid)
			{
				int p;
				for (p=0; p<3; p++)
					joffset[p] += (int)rint(
						(vh_bg_pts.scn.domain[3+p]-vh_pts.scn.domain[3+p])/
						vh_bg_pts.scn.xypixsz[0]+
						(vh_bg_pts.scn.domain[6+p]-vh_pts.scn.domain[6+p])/
						vh_bg_pts.scn.xypixsz[1]+
						(vh_bg_pts.scn.loc_of_subscenes[0]-
						vh_pts.scn.loc_of_subscenes[0]+
						vh_bg_pts.scn.domain[9+p]-vh_pts.scn.domain[9+p])/
						slice_spacing);
			}
			bg_pts_data = (unsigned char *)malloc((vh_bg_pts.scn.xysize[0]*
				vh_bg_pts.scn.xysize[1]+7)/8);
			if (bg_pts_data == NULL)
				handle_error(1);
			handle_error(VSeekData(fp_bg_pts, 0));
			for (j=0; j<vh_bg_pts.scn.num_of_subscenes[0]; j++)
			{
				handle_error(VReadData((char *)bg_pts_data, 1, (vh_bg_pts.scn.xysize[0]
					*vh_bg_pts.scn.xysize[1]+7)/8, fp_bg_pts, &k));
				for (k=n=0,b=128; k<vh_bg_pts.scn.xysize[1]; k++)
					for (m=0; m<vh_bg_pts.scn.xysize[0]; m++)
					{
						if (bg_pts_data[n] & b)
						{
						  double x[3], x1[3], x2[3], x3[3], x4[3];
						  int p;
						  if (num_bg_points == 0)
						    bg_point = (void *)malloc(sizeof(bg_point[0]));
						  else
						    bg_point = (void *)realloc(bg_point,
							  (num_bg_points+1)*sizeof(bg_point[0]));
						  if (bg_point == NULL)
						    handle_error(1);
						  if (prescaled &&
						      j+joffset[2]>=0 && j+joffset[2]<slices_out &&
							  k+joffset[1]>=0 && k+joffset[1]<
							  vh_in.scn.xysize[1] && m+joffset[0]>=0 &&
							  m+joffset[0]<vh_in.scn.xysize[0])
						  {
						   bg_point[num_bg_points][0] = m+joffset[0];
						   bg_point[num_bg_points][1] = k+joffset[1];
						   bg_point[num_bg_points][2] = j+joffset[2];
						   num_bg_points++;
						  }
						  else
						  {
						   if (vh_in.scn.domain_valid)
						   {
						    for (p=0; p<3; p++)
						      x4[p] = m*vh_bg_pts.scn.xypixsz[0]*
							    vh_in.scn.domain[3+p]+
							    k*vh_bg_pts.scn.xypixsz[1]*
								vh_in.scn.domain[6+p]+
							    vh_bg_pts.scn.loc_of_subscenes[j]*
								vh_in.scn.domain[9+p];
						    if (rotate_flag)
							{
							  for (p=0; p<3; p++)
							    x3[p] = inv_scale*(x4[p]+
								  vh_bg_pts.scn.domain[p])-centroid[p];
							  vector_matrix_multiply(x2, x3, rotation);
							  for (p=0; p<3; p++)
							    x1[p] = x2[p]+centroid[p]+translation[p];
						    }
							else
							  for (p=0; p<3; p++)
							    x1[p] = inv_scale*(x4[p]+
								  vh_bg_pts.scn.domain[p])+translation[p];
						    for (p=0; p<3; p++)
							  x[p] = (x1[0]-vh_in.scn.domain[0])*
							         vh_in.scn.domain[3+3*p]+
							         (x1[1]-vh_in.scn.domain[1])*
									 vh_in.scn.domain[4+3*p]+
							         (x1[2]-vh_in.scn.domain[2])*
									 vh_in.scn.domain[5+3*p];
						   }
						   else
						   {
						    x4[0] = m*vh_bg_pts.scn.xypixsz[0];
							x4[1] = k*vh_bg_pts.scn.xypixsz[1];
							x4[2] = vh_bg_pts.scn.loc_of_subscenes[j];
						    if (rotate_flag)
							{
							  for (p=0; p<3; p++)
							    x3[p] = inv_scale*x4[p]-centroid[p];
							  vector_matrix_multiply(x2, x3, rotation);
							  for (p=0; p<3; p++)
							    x[p] = x2[p]+centroid[p]+translation[p];
						    }
							else
							  for (p=0; p<3; p++)
							    x[p] = inv_scale*x4[p]+translation[p];
						   }
						   x[2] -= vh_in.scn.loc_of_subscenes[0];
						   x[0] /= vh_in.scn.xypixsz[0];
						   x[1] /= vh_in.scn.xypixsz[1];
						   x[2] *= slice_rate;
						   // add seed point
						   for (p=0; p<3; p++)
						     bg_point[num_bg_points][p] = (int)rint(x[p]);
						   if (bg_point[num_bg_points][0]>=0 &&
						       bg_point[num_bg_points][0]<
							     vh_in.scn.xysize[0] &&
							   bg_point[num_bg_points][1]>=0 &&
							   bg_point[num_bg_points][1]<
							     vh_in.scn.xysize[1] &&
							   bg_point[num_bg_points][2]>=0 &&
							   bg_point[num_bg_points][2]<slices_out)
							 num_bg_points++;
						  }
						}
						b >>= 1;
						if (b == 0)
						{
							n++;
							b = 128;
						}
					}
			}
		}
	}
	if (feature_status[6])
		load_fom();
	load_dfom();
	if (feature_status[7])
		load_feature_map();
	for (current_volume=0; current_volume<volumes_out; current_volume++)
	{
		load_volume(current_volume);
		if (num_bg_points || bg_filename)
		{
			if (mofs_flag)
				MOFS(current_volume);
			else
				GC_max(current_volume);
		}
		else
			switch (track_algorithm)
			{
				case 0:
					fuzzy_track_hheap(current_volume);
					break;
				case 1:
					fuzzy_track_chash(current_volume);
					break;
				case 2:
					fuzzy_track_chash2(current_volume);
					break;
			}
		if (mask_original)
		{	OutCellType *c_ptr;
			unsigned char *o_ptr_8;
			unsigned short *o_ptr_16;
			int m;

			c_ptr = out_data;
			if (vh_in.scn.num_of_bits == 8)
			{
				o_ptr_8 = in_data_8;
				for (j=0; j<slices_out; j++)
					for (k=0; k<vh_in.scn.xysize[1]; k++)
						for (m=0; m<vh_in.scn.xysize[0]; m++,c_ptr++,o_ptr_8++)
							if (*c_ptr == 0)
								*o_ptr_8 = 0;
			}
			else
			{
				o_ptr_16 = in_data_16;
				for (j=0; j<slices_out; j++)
					for (k=0; k<vh_in.scn.xysize[1]; k++)
						for(m=0; m<vh_in.scn.xysize[0]; m++,c_ptr++,o_ptr_16++)
							if (*c_ptr == 0)
								*o_ptr_16 = 0;
			}
			handle_error(VWriteData((char *)in_data, vh_in.scn.num_of_bits/8,
				slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1], fp, &j));
		}
		else
			handle_error(VWriteData((char *)out_data, sizeof(OutCellType),
				slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1], fp, &j));
		if (affinity_type==3 && feature_status[6])
		{
			OutCellType *c_ptr;
			int m;
			double Q=0, U=0, I=0;

			c_ptr = out_data;
			for (j=0; j<slices_out; j++)
				for (k=0; k<vh_in.scn.xysize[1]; k++)
					for (m=0; m<vh_in.scn.xysize[0]; m++,c_ptr++)
					{
						int fv;
						fv = fom_value(m, k, j);
						if (*c_ptr)
						{
							Q++;
							I += fv;
						}
						else
							U += fv;
					}
			U += 65534*Q;
			fprintf(outstream, "f_b = %f\n", fb/Q);
			fprintf(outstream, "f_t = %f\n", ft/Q);
			fprintf(outstream, "f_s = %f\n", I/U);
		}
	}
    VCloseData(fp);
	if (bg_flag == 1)
	{	char cmd[256];

		sprintf(cmd, "job_done %s &", argv[0]);
		system(cmd);
	    VDeleteBackgroundProcessInformation();
	}
	exit(0);
}

/*****************************************************************************
 * FUNCTION: command_line
 * DESCRIPTION: Returns a string with the command line arguments.
 * PARAMETERS:
 *    argc: number of command line arguments
 *    argv: command line arguments
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables argv0, bg_flag must be properly set.
 * RETURN VALUE: a string with the command line arguments
 * EXIT CONDITIONS: Writes an error message to stderr and exits with code 1
 *    if memory allocation fails.
 * HISTORY:
 *    Created: 4/2/96 by Dewey Odhner
 *
 *****************************************************************************/
static char *command_line(int argc, char **argv)
{
	int j, k;
	char *cl;

	k = argc;
	for (j=0; j<argc; j++)
		k += (int)strlen(argv[j]);
	cl = (char *)malloc(k);
	if (cl == NULL)
		handle_error(1);
	k=0;
	for (j=0; j<argc; j++)
	{	strcpy(cl+k, argv[j]);
		k += (int)strlen(argv[j]);
		cl[k] = ' ';
		k++;
	}
	cl[k-1] = 0;
	return (cl);
}

/*****************************************************************************
 * MACRO: Assign_feature
 * DESCRIPTION: Assigns the selected feature to feature_val.
 * PARAMETERS:
 *    feature_val: The variable to be assigned the feature value.
 *    high, low: The higher and lower values from which to compute the feature.
 *    feature_n:
 *       0: high
 *       1: low
 *       2: difference
 *       3: sum
 *       4: relative difference
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable affinity_type must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 7/29/96 by Dewey Odhner
 *
 *****************************************************************************/
#define Assign_feature(feature_val, high, low, feature_n) \
switch (feature_n) \
{ \
	case 0: \
	case 5: \
		feature_val = (float)(high); \
		break; \
	case 1: \
		feature_val = (float)(low); \
		break; \
	case 2: \
		feature_val = (float)((high)-(low)); \
		break; \
	case 3: \
		feature_val = (float)((high)+(low)); \
		break; \
	case 4: \
		feature_val = (high)+(low)==0? 0:(float)((high)-(low))/((high)+(low));\
		if (affinity_type == 2) \
			feature_val *= (float)65535; \
		break; \
	default: \
		assert(FALSE); \
}

/*****************************************************************************
 * FUNCTION: affinity
 * DESCRIPTION: Returns the affinity scaled to adjacency for adjacent
 *    voxel values.
 * PARAMETERS:
 *    a, b: The values of the adjacent voxels
 *    adjacency: The fuzzy adjacency between the two voxels scaled to
 *       MAX_CONNECTIVITY.
 * SIDE EFFECTS: fb, ft incremented.
 * ENTRY CONDITIONS: The variables feature_status, function_level,
 *    function_width, weight, function_selected, threshold, file_info, vh_in,
 *    in_data, affinity_type, histogram_bins, histogram_counts,
 *    largest_density_value, count_affinity, reverse_histogram_counts,
 *    covariance_flag, reverse_covariance_flag, inv_covariance,
 *    inv_reverse_covariance, training_mean, reverse_training_mean,
 *    weight_unit, vh_fom, fom_data, slice_spacing, rel_scale, translation,
 *    i_weight_unit, fb, ft, bg_filename, dfom_data, dfom_filename
 *    must be properly set.
 * RETURN VALUE: the affinity scaled to MAX_CONNECTIVITY
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 4/2/96 by Dewey Odhner
 *    Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
 *    Modified: 5/31/96 dual-histogram-type affinity allowed by Dewey Odhner
 *    Modified: 7/26/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 1/29/98 fuzzy adjacency passed by Dewey Odhner
 *    Modified: 9/9/10 fb, ft incremented by Dewey Odhner
 *
 *****************************************************************************/
int affinity(int a, int b, float adjacency, int ax, int ay, int az,
	int bx, int by, int bz, int back)
{
	int feature_n, high, low, t_count, features_on, on_feature_n, on_feature_m;
	float temp_affinity, feature_val, xrel, on_feature_val[NUM_FEATURES],
		temp_affinity2=MAX_CONNECTIVITY, temp_affinity3;

	if (a > b)
		high = a, low = b;
	else
		low = a, high = b;
	switch (affinity_type)
	{
	  case 0:
		temp_affinity = 1;
		for (feature_n=0; feature_n<5; feature_n++)
		{
			if (!feature_status[feature_n])
				continue;
			Assign_feature(feature_val, high, low, feature_n);
			xrel = (feature_val-function_level[feature_n])/
				function_width[feature_n];
			switch (function_selected[feature_n])
			{
				case 0: /* gaussian */
					feature_val = (float)exp(-.5*xrel*xrel);
					break;
				case 1: /* ramp */
					feature_val = (float)(xrel + .5);
					if (feature_val < 0)
						feature_val = 0;
					if (feature_val > 1)
						feature_val = 1;
					break;
				case 2: /* box */
					feature_val = (float)(xrel<-.5||xrel>.5? 0:1);
					break;
			}
			temp_affinity *= weight[feature_n]*feature_val+
				1-(weight[feature_n]>0? weight[feature_n]:0);
		}
		temp_affinity2 = temp_affinity;
		break;
	  case 1:
		if (reverse_histogram_counts == NULL)
			temp_affinity = histogram_counts[high*
				(histogram_bins[0]-1)/largest_density_value*histogram_bins[1]+
				low*(histogram_bins[1]-1)/largest_density_value]*
				count_affinity;
		else
		{
			t_count = histogram_counts[high*
				(histogram_bins[0]-1)/largest_density_value*histogram_bins[1]+
				low*(histogram_bins[1]-1)/largest_density_value];
			temp_affinity = t_count==0? 0: t_count/((float)t_count+
				reverse_histogram_counts[high*
				(histogram_bins[0]-1)/largest_density_value*histogram_bins[1]+
				low*(histogram_bins[1]-1)/largest_density_value]);
		}
		temp_affinity2 = temp_affinity;
		break;
	  case 2:
		for (feature_n=features_on=0; feature_n<NUM_FEATURES; feature_n++)
			if (feature_status[feature_n])
			{
				Assign_feature(on_feature_val[features_on], high, low,
					feature_n);
				features_on++;
			}
		temp_affinity = 0;
		if (!reverse_covariance_flag)
		{
			for (on_feature_n=0; on_feature_n<features_on; on_feature_n++)
				for (on_feature_m=0; on_feature_m<features_on; on_feature_m++)
					temp_affinity = (float)(temp_affinity+
						(on_feature_val[on_feature_n]-
						training_mean[on_feature_n])*
						inv_covariance[on_feature_n*features_on+on_feature_m]*
						(on_feature_val[on_feature_m]-
						training_mean[on_feature_m]));
			temp_affinity = (float)exp(-.5*temp_affinity);
		}
		else if (!covariance_flag)
		{
			for (on_feature_n=0; on_feature_n<features_on; on_feature_n++)
				for (on_feature_m=0; on_feature_m<features_on; on_feature_m++)
					temp_affinity = (float)(temp_affinity+
						(on_feature_val[on_feature_n]-
						reverse_training_mean[on_feature_n])*
						inv_reverse_covariance[on_feature_n*features_on+
						on_feature_m]*(on_feature_val[on_feature_m]-
						reverse_training_mean[on_feature_m]));
			temp_affinity = (float)(1-exp(-.5*temp_affinity));
		}
		else
		{
			for (on_feature_n=0; on_feature_n<features_on; on_feature_n++)
				for (on_feature_m=0; on_feature_m<features_on; on_feature_m++)
					temp_affinity = (float)(temp_affinity+
						(on_feature_val[on_feature_n]-
						training_mean[on_feature_n])*
						inv_covariance[on_feature_n*features_on+on_feature_m]*
						(on_feature_val[on_feature_m]-training_mean[
						on_feature_m])-(on_feature_val[on_feature_n]-
						reverse_training_mean[on_feature_n])*
						inv_reverse_covariance[on_feature_n*features_on+
						on_feature_m]*(on_feature_val[on_feature_m]-
						reverse_training_mean[on_feature_m]));
			temp_affinity = (float)(1/(1+exp(.5*temp_affinity)));
		}
		temp_affinity2 = temp_affinity;
		break;
	  case 3:
		temp_affinity = temp_affinity2 = 0;
		for (feature_n=0; feature_n<5; feature_n++)
		{
			if (!feature_status[feature_n])
				continue;
			switch (feature_n)
			{
			  case 0:
			  case 1:
			    feature_val = (float)(feature_n? high: low);
				xrel = (feature_val-function_level[0])/
					function_width[0];
				switch (function_selected[0])
				{
					case 0: /* gaussian */
						temp_affinity3 = (float)exp(-.5*xrel*xrel);
						break;
					case 1: /* ramp */
						temp_affinity3 = (float)(xrel + .5);
						if (temp_affinity3 < 0)
							temp_affinity3 = 0;
						if (temp_affinity3 > 1)
							temp_affinity3 = 1;
						break;
					case 2: /* box */
						temp_affinity3 = (float)(xrel<-.5||xrel>.5? 0:1);
						break;
				}
				xrel = (feature_val-function_level[1])/
					function_width[1];
				switch (function_selected[1])
				{
					case 0: /* gaussian */
						feature_val = (float)exp(-.5*xrel*xrel);
						break;
					case 1: /* ramp */
						feature_val = (float)(xrel + .5);
						if (feature_val < 0)
							feature_val = 0;
						if (feature_val > 1)
							feature_val = 1;
						break;
					case 2: /* box */
						feature_val = (float)(xrel<-.5||xrel>.5? 0:1);
						break;
				}
				if (temp_affinity3 > feature_val)
					feature_val = temp_affinity3;
				temp_affinity2 = weight_unit*weight[feature_n]*feature_val;
				if (feature_n==0 || temp_affinity2 < temp_affinity)
					temp_affinity = temp_affinity2;
				break;
			  default:
			    Assign_feature(feature_val, high, low, feature_n);
				xrel = (feature_val-function_level[feature_n])/
					function_width[feature_n];
				switch (function_selected[feature_n])
				{
					case 0: /* gaussian */
						feature_val = (float)exp(-.5*xrel*xrel);
						break;
					case 1: /* ramp */
						feature_val = (float)(xrel + .5);
						if (feature_val < 0)
							feature_val = 0;
						if (feature_val > 1)
							feature_val = 1;
						break;
					case 2: /* box */
						feature_val = (float)(xrel<-.5||xrel>.5? 0:1);
						break;
				}
				if (feature_n==2 && dfom_filename && a!=b)
				{
					float dv=(float)(.5/65534*
						(dfom_value(ax, ay, az)+dfom_value(bx, by, bz)));
					if ((a>b) == (back==0))
					{
						if (1-2*dv > feature_val)
							feature_val = (float)(1-2*dv);
					}
					else
					{
						if (2*dv-1 > feature_val)
							feature_val = (float)(2*dv-1);
					}
				}
				temp_affinity += weight_unit*weight[feature_n]*feature_val;
				break;
			}
		}
		if (feature_status[7])
		{
			if (threeD_hist)
			{
				int feat_bin[3];
				for (feat_bin[0]=0; high+low>hist_bin_table[0][feat_bin[0]];
						feat_bin[0]++)
					;
				for (feat_bin[1]=0; high-low>hist_bin_table[1][feat_bin[1]];
						feat_bin[1]++)
					;
				for (feat_bin[2]=0; fom_value(ax, ay, az)+fom_value(bx, by, bz)
						>hist_bin_table[2][feat_bin[2]]; feat_bin[2]++)
					;
				temp_affinity += i_weight_unit*weight[7]*
				    threeD_hist[feat_bin[0]][feat_bin[1]][feat_bin[2]];
			}
			else
				temp_affinity += i_weight_unit*weight[7]*
					feature_map[gray_map[low]][gray_map[high]];
		}
		temp_affinity2 = temp_affinity;
		if (feature_status[5])
		{
			feature_val = (float)a;
			xrel = (feature_val-function_level[5])/function_width[5];
			switch (function_selected[5])
			{
				case 0: /* gaussian */
					feature_val = (float)exp(-.5*xrel*xrel);
					break;
				case 1: /* ramp */
					feature_val = (float)(xrel + .5);
					if (feature_val < 0)
						feature_val = 0;
					if (feature_val > 1)
						feature_val = 1;
					break;
				case 2: /* box */
					feature_val = (float)(xrel<-.5||xrel>.5? 0:1);
					break;
			}
			temp_affinity += weight_unit*weight[5]*feature_val;
			feature_val = (float)b;
			xrel = (feature_val-function_level[5])/function_width[5];
			switch (function_selected[5])
			{
				case 0: /* gaussian */
					feature_val = (float)exp(-.5*xrel*xrel);
					break;
				case 1: /* ramp */
					feature_val = (float)(xrel + .5);
					if (feature_val < 0)
						feature_val = 0;
					if (feature_val > 1)
						feature_val = 1;
					break;
				case 2: /* box */
					feature_val = (float)(xrel<-.5||xrel>.5? 0:1);
					break;
			}
			temp_affinity2 += weight_unit*weight[5]*feature_val;
		}
		if (feature_status[6] && weight[6])
		{
			float tmp_fom_val, tmp_fom_val2;
			tmp_fom_val = (float)fom_value(ax, ay, az);
			tmp_fom_val2 = (float)fom_value(bx, by, bz);
			if (back)
			{
				tmp_fom_val = 65534-tmp_fom_val;
				tmp_fom_val2 = 65534-tmp_fom_val2;
			}
			temp_affinity += i_weight_unit*weight[6]*tmp_fom_val;
			temp_affinity2 += i_weight_unit*weight[6]*tmp_fom_val2;
		}
		if (feature_status[MULTITISSUE])
		{
			int j;

			temp_affinity3 = 0;
			for (j=back?fg_tissues:0; j<(back?num_tissues:fg_tissues); j++)
			{
				xrel = (a-tissue_level[j])/tissue_width[j];
				switch (tissue_type[j])
				{
					case BRIGHTEST:
						feature_val = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case DARKEST:
						feature_val = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					default:
						feature_val = (float)exp(-.5*xrel*xrel);
						break;
				}
				if (feature_val > temp_affinity3)
					temp_affinity3 = feature_val;
			}
			temp_affinity += weight_unit*weight[MULTITISSUE]*temp_affinity3;
			temp_affinity3 = 0;
			for (j=back?fg_tissues:0; j<(back?num_tissues:fg_tissues); j++)
			{
				xrel = (b-tissue_level[j])/tissue_width[j];
				switch (tissue_type[j])
				{
					case BRIGHTEST:
						feature_val = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case DARKEST:
						feature_val = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					default:
						feature_val = (float)exp(-.5*xrel*xrel);
						break;
				}
				if (feature_val > temp_affinity3)
					temp_affinity3 = feature_val;
			}
			temp_affinity2 += weight_unit*weight[MULTITISSUE]*temp_affinity3;
		}
		break;
	  case 4:
	    xrel = (high-function_level[0])/function_width[0];
		temp_affinity = (float)exp(-.5*xrel*xrel);
		xrel = (low-function_level[0])/function_width[0];
		temp_affinity2 = (float)exp(-.5*xrel*xrel);
		if (temp_affinity2 < temp_affinity)
			temp_affinity = temp_affinity2;
		xrel = (high-function_level[1])/function_width[1];
		temp_affinity2 = (float)exp(-.5*xrel*xrel);
		xrel = (low-function_level[1])/function_width[1];
		temp_affinity3 = (float)exp(-.5*xrel*xrel);
		if (temp_affinity3 < temp_affinity2)
			temp_affinity2 = temp_affinity3;
		if (temp_affinity2 > temp_affinity)
			temp_affinity = temp_affinity2;
		feature_val = (float)(high-low);
		xrel = feature_val/function_width[2];
		temp_affinity2 = (float)exp(-.5*xrel*xrel);
		temp_affinity = (float)sqrt(temp_affinity*temp_affinity2);
		break;
	  default:
		assert(FALSE);
	}
	temp_affinity *= adjacency;
	temp_affinity2 *= adjacency;
	if (temp_affinity2 < temp_affinity)
		temp_affinity = temp_affinity2;
	if (temp_affinity >= threshold)
		return (int)temp_affinity;
	else
		return 0;
}

/*****************************************************************************
 * FUNCTION: fom_value
 * DESCRIPTION: Returns the fuzzy object model affinity component for a voxel.
 * PARAMETERS:
 *    col, row, slc: Coordinates of the voxel
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables vh_in, vh_fom, fom_data, slice_spacing,
 *    rel_scale, translation, rotate_flag, rotation, centroid, prescaled
 *    must be properly set.
 * RETURN VALUE: the affinity scaled to MAX_CONNECTIVITY
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 2/22/10 by Dewey Odhner
 *
 *****************************************************************************/
int fom_value(int col, int row, int slc)
{
#define VV(x, y, z) ((x)<0 || (x)>=vh_fom.scn.xysize[0] || \
                     (y)<0 || (y)>=vh_fom.scn.xysize[1] || \
					 (z)<0 || (z)>=vh_fom.scn.num_of_subscenes[0]? 0: \
					 fom_data[((z)*vh_fom.scn.xysize[1]+(y))* \
					 vh_fom.scn.xysize[0]+(x)])

	  double x[3], x1[3], x2[3], x3[3], x4[3], xfrac[3];
	  int xint[3], v[2][2][2], j;

	  if (prescaled)
	    return VV(col-ioffset[0], row-ioffset[1], slc-ioffset[2]);
	  if (vh_fom.scn.domain_valid)
	  {
	    for (j=0; j<3; j++)
	      x4[j] = col*vh_in.scn.xypixsz[0]*vh_fom.scn.domain[3+j]+
		    row*vh_in.scn.xypixsz[1]*vh_fom.scn.domain[6+j]+
		    vh_in.scn.loc_of_subscenes[slc]*vh_fom.scn.domain[9+j];
	    if (rotate_flag)
		{
		  for (j=0; j<3; j++)
		    x3[j] = x4[j]+vh_in.scn.domain[j]-translation[j]-centroid[j];
		  matrix_vector_multiply(x2, rotation, x3);
		  for (j=0; j<3; j++)
		    x1[j] = rel_scale*(x2[j]+centroid[j]);
		}
		else
		  for (j=0; j<3; j++)
		    x1[j] = rel_scale*(x4[j]+vh_in.scn.domain[j]-translation[j]);
	    for (j=0; j<3; j++)
		  x[j] = (x1[0]-vh_fom.scn.domain[0])*vh_fom.scn.domain[3+3*j]+
		         (x1[1]-vh_fom.scn.domain[1])*vh_fom.scn.domain[4+3*j]+
		         (x1[2]-vh_fom.scn.domain[2])*vh_fom.scn.domain[5+3*j];
	  }
	  else
	  {
	    x4[0] = col*vh_in.scn.xypixsz[0];
		x4[1] = row*vh_in.scn.xypixsz[1];
		x4[2] = vh_in.scn.loc_of_subscenes[slc];
	    if (rotate_flag)
		{
		  for (j=0; j<3; j++)
		    x3[j] = x4[j]-translation[j]-centroid[j];
		  matrix_vector_multiply(x2, rotation, x3);
		  for (j=0; j<3; j++)
		    x[j] = rel_scale*(x2[j]+centroid[j]);
		}
		else
		  for (j=0; j<3; j++)
		    x[j] = rel_scale*(x4[j]-translation[j]);
	  }
	  x[2] -= vh_fom.scn.loc_of_subscenes[0];
	  x[0] /= vh_fom.scn.xypixsz[0];
	  x[1] /= vh_fom.scn.xypixsz[1];
	  x[2] /= slice_spacing;
	  for (j=0; j<3; j++)
	  {
	    xint[j] = (int)floor(x[j]);
		xfrac[j] = x[j]-xint[j];
	  }
      v[0][0][0] = VV(xint[0], xint[1], xint[2]);
      v[1][0][0] = VV(xint[0]+1, xint[1], xint[2]);
      v[0][1][0] = VV(xint[0], xint[1]+1, xint[2]);
      v[1][1][0] = VV(xint[0]+1, xint[1]+1, xint[2]);
      v[0][0][1] = VV(xint[0], xint[1], xint[2]+1);
      v[1][0][1] = VV(xint[0]+1, xint[1], xint[2]+1);
      v[0][1][1] = VV(xint[0], xint[1]+1, xint[2]+1);
      v[1][1][1] = VV(xint[0]+1, xint[1]+1, xint[2]+1);
	  return (int)(
	    (1-xfrac[0])*(
		  (1-xfrac[1])*(
		    (1-xfrac[2])*v[0][0][0]+
			xfrac[2]*v[0][0][1])+
		  xfrac[1]*(
		    (1-xfrac[2])*v[0][1][0]+
			xfrac[2]*v[0][1][1]))+
		xfrac[0]*(
		  (1-xfrac[1])*(
		    (1-xfrac[2])*v[1][0][0]+
			xfrac[2]*v[1][0][1])+
		  xfrac[1]*(
		    (1-xfrac[2])*v[1][1][0]+
			xfrac[2]*v[1][1][1])));
}

/*****************************************************************************
 * FUNCTION: dfom_value
 * DESCRIPTION: Returns the fuzzy object model affinity directedness for a
 *    voxel.
 * PARAMETERS:
 *    col, row, slc: Coordinates of the voxel
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables vh_in, vh_fom, dfom_data, slice_spacing,
 *    rel_scale, translation, rotate_flag, rotation, centroid, prescaled,
 *    fom_filename must be properly set.
 * RETURN VALUE: the affinity directedness scaled to MAX_CONNECTIVITY
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 7/18/12 by Dewey Odhner
 *
 *****************************************************************************/
int dfom_value(int col, int row, int slc)
{
#define DV(x, y, z, vh) ((x)<0 || (x)>=vh.scn.xysize[0] || \
                         (y)<0 || (y)>=vh.scn.xysize[1] || \
					     (z)<0 || (z)>=vh.scn.num_of_subscenes[0]? 0: \
					     dfom_data[((z)*vh.scn.xysize[1]+(y))* \
					     vh.scn.xysize[0]+(x)])

	  double x[3], x1[3], x2[3], x3[3], x4[3], xfrac[3];
	  int xint[3], v[2][2][2], j;

	  if (fom_filename == NULL)
	    return DV(col, row, slc, vh_in);
	  if (prescaled)
	    return DV(col-ioffset[0], row-ioffset[1], slc-ioffset[2], vh_fom);
	  if (vh_fom.scn.domain_valid)
	  {
	    for (j=0; j<3; j++)
	      x4[j] = col*vh_in.scn.xypixsz[0]*vh_fom.scn.domain[3+j]+
		    row*vh_in.scn.xypixsz[1]*vh_fom.scn.domain[6+j]+
		    vh_in.scn.loc_of_subscenes[slc]*vh_fom.scn.domain[9+j];
	    if (rotate_flag)
		{
		  for (j=0; j<3; j++)
		    x3[j] = x4[j]+vh_in.scn.domain[j]-translation[j]-centroid[j];
		  matrix_vector_multiply(x2, rotation, x3);
		  for (j=0; j<3; j++)
		    x1[j] = rel_scale*(x2[j]+centroid[j]);
		}
		else
		  for (j=0; j<3; j++)
		    x1[j] = rel_scale*(x4[j]+vh_in.scn.domain[j]-translation[j]);
	    for (j=0; j<3; j++)
		  x[j] = (x1[0]-vh_fom.scn.domain[0])*vh_fom.scn.domain[3+3*j]+
		         (x1[1]-vh_fom.scn.domain[1])*vh_fom.scn.domain[4+3*j]+
		         (x1[2]-vh_fom.scn.domain[2])*vh_fom.scn.domain[5+3*j];
	  }
	  else
	  {
	    x4[0] = col*vh_in.scn.xypixsz[0];
		x4[1] = row*vh_in.scn.xypixsz[1];
		x4[2] = vh_in.scn.loc_of_subscenes[slc];
	    if (rotate_flag)
		{
		  for (j=0; j<3; j++)
		    x3[j] = x4[j]-translation[j]-centroid[j];
		  matrix_vector_multiply(x2, rotation, x3);
		  for (j=0; j<3; j++)
		    x[j] = rel_scale*(x2[j]+centroid[j]);
		}
		else
		  for (j=0; j<3; j++)
		    x[j] = rel_scale*(x4[j]-translation[j]);
	  }
	  x[2] -= vh_fom.scn.loc_of_subscenes[0];
	  x[0] /= vh_fom.scn.xypixsz[0];
	  x[1] /= vh_fom.scn.xypixsz[1];
	  x[2] /= slice_spacing;
	  for (j=0; j<3; j++)
	  {
	    xint[j] = (int)floor(x[j]);
		xfrac[j] = x[j]-xint[j];
	  }
      v[0][0][0] = DV(xint[0], xint[1], xint[2], vh_fom);
      v[1][0][0] = DV(xint[0]+1, xint[1], xint[2], vh_fom);
      v[0][1][0] = DV(xint[0], xint[1]+1, xint[2], vh_fom);
      v[1][1][0] = DV(xint[0]+1, xint[1]+1, xint[2], vh_fom);
      v[0][0][1] = DV(xint[0], xint[1], xint[2]+1, vh_fom);
      v[1][0][1] = DV(xint[0]+1, xint[1], xint[2]+1, vh_fom);
      v[0][1][1] = DV(xint[0], xint[1]+1, xint[2]+1, vh_fom);
      v[1][1][1] = DV(xint[0]+1, xint[1]+1, xint[2]+1, vh_fom);
	  return (int)(
	    (1-xfrac[0])*(
		  (1-xfrac[1])*(
		    (1-xfrac[2])*v[0][0][0]+
			xfrac[2]*v[0][0][1])+
		  xfrac[1]*(
		    (1-xfrac[2])*v[0][1][0]+
			xfrac[2]*v[0][1][1]))+
		xfrac[0]*(
		  (1-xfrac[1])*(
		    (1-xfrac[2])*v[1][0][0]+
			xfrac[2]*v[1][0][1])+
		  xfrac[1]*(
		    (1-xfrac[2])*v[1][1][0]+
			xfrac[2]*v[1][1][1])));
}

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
  return chash_push(H, &tmp);
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
void repush_xyz_chash(short x, short y, short z, unsigned short w,
	unsigned short ow)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  chash_repush(H, &tmp, ow);
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

  chash_pop(H, &tmp);
  *x = tmp.x;
  *y = tmp.y;
  *z = tmp.z;
  kkk = tmp.z * dimensions.slice_size + tmp.y * dimensions.xdim + tmp.x;
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
push_xyz_chash2(x, y, z, w)
  short x, y, z;
  unsigned short w;
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  return chash_push2(H, &tmp);
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
void repush_xyz_chash2(short x, short y, short z, unsigned short w,
	unsigned short ow)
{
  VoxelWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.z = z;
  tmp.val = w;
  chash_repush2(H, &tmp, ow);
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

  chash_pop2(H, &tmp);
  *x = tmp.x;
  *y = tmp.y;
  *z = tmp.z;
  kkk = tmp.z * dimensions.slice_size + tmp.y * dimensions.xdim + tmp.x;
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
  return (((VoxelWithValue *)v)->z * dimensions.slice_size + ((VoxelWithValue *)v)->y * dimensions.xdim + ((VoxelWithValue *)v)->x) % hashsize;
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
void
repush_xyz_hheap(short x, short y, short z, unsigned short w)
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
  kkk = tmp.z * dimensions.slice_size + tmp.y * dimensions.xdim + tmp.x;
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
	return ((VoxelWithValue *)v)->val>((VoxelWithValue *)vv)->val? 1: ((VoxelWithValue *)v)->val<((VoxelWithValue *)vv)->val? -1: 0;
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
	if (v==NULL && vv==NULL)
		return 0;
	if (v == NULL)
		return -1;
	if (vv == NULL)
		return 1;
	if (((VoxelWithValue *)v)->x > ((VoxelWithValue *)vv)->x)
		return 1;
	if (((VoxelWithValue *)v)->x < ((VoxelWithValue *)vv)->x)
		return -1;
	if (((VoxelWithValue *)v)->y > ((VoxelWithValue *)vv)->y)
		return 1;
	if (((VoxelWithValue *)v)->y < ((VoxelWithValue *)vv)->y)
		return -1;
	if (((VoxelWithValue *)v)->z > ((VoxelWithValue *)vv)->z)
		return 1;
	if (((VoxelWithValue *)v)->z < ((VoxelWithValue *)vv)->z)
		return -1;
	return 0;
}

#define Push_xyz push_xyz_chash
#define Repush_xyz(x, y, z, w, ow) repush_xyz_chash((x), (y), (z), (w), (ow))
#define Pop_xyz pop_xyz_chash
#define H_is_empty chash_isempty(H)
#define fuzzy_track fuzzy_track_chash
#define Create_heap chash_create(MAX_CONNECTIVITY+1L, sizeof(VoxelWithValue), \
	voxel_value, NULL)

#include "track_using.c"

#undef Push_xyz
#undef Repush_xyz
#undef Pop_xyz
#undef fuzzy_track
#undef Create_heap

#define Push_xyz push_xyz_chash2
#define Repush_xyz(x, y, z, w, ow) repush_xyz_chash2((x), (y), (z), (w), (ow))
#define Pop_xyz pop_xyz_chash2
#define fuzzy_track fuzzy_track_chash2
#define Create_heap chash_create2(MAX_CONNECTIVITY+1L, sizeof(VoxelWithValue),\
	voxel_value, NULL)

#include "track_using.c"

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

#include "track_using.c"

/*****************************************************************************
 * FUNCTION: load_volume
 * DESCRIPTION: Reads a volume from the input file to in_data.
 * PARAMETERS:
 *    current_volume: volume number from zero in the subscene.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables input_filename, file_info, vh_in, in_data
 *    must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 3/28/96 by Dewey Odhner
 *
 *****************************************************************************/
void load_volume(int current_volume)
{
	int slice, index, cell_bytes, slice_bytes, items_read;
	FILE *fp;

	fp = fopen(input_filename, "rb");
	if (fp == NULL)
		handle_error(4);
	cell_bytes = vh_in.scn.num_of_bits/8;
	slice_bytes = vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*cell_bytes;
	for (slice=0;slice<1+(file_info.max[0]-file_info.min[0])/file_info.incr[0];
			slice++)
	{
		index = input_slice_index(current_volume, slice);
		if (index >= 0)
		{
			handle_error(VSeekData(fp, index*slice_bytes));
			handle_error(VReadData((char *)in_data_8+slice*slice_bytes, cell_bytes,
				vh_in.scn.xysize[0]*vh_in.scn.xysize[1], fp, &items_read));
		}
		else
			memset(in_data_8+slice*slice_bytes, 0, slice_bytes);
	}
	fclose(fp);
}

/*****************************************************************************
 * FUNCTION: load_feature_map
 * DESCRIPTION: Reads the fuzzy object model data.
 * PARAMETERS: None
 * SIDE EFFECTS: The variables feature_map_buf, feature_map, feature_map_max,
 *    gray_map are initialized.
 * ENTRY CONDITIONS: The variables feature_map_filename, vh_in,
 *    threeD_hist_filename must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error writes a message to stderr and exits with code !=0
 * HISTORY:
 *    Created: 9/27/11 by Dewey Odhner
 *
 *****************************************************************************/
void load_feature_map()
{
	int j, k, m;
	FILE *fp;

	if (feature_map_filename)
	{
		feature_map_buf = (unsigned short *)malloc(0x800800*2);
		if (feature_map_buf == NULL)
			handle_error(1);
		for (j=0; j<4096; j++)
			feature_map[j] = feature_map_buf+(8191-j)*j/2;
		fp = fopen(feature_map_filename, "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "Could not open feature map.\n");
			exit(-1);
		}
		handle_error(VReadData((char *)&feature_map_max, 2, 1, fp, &j));
		handle_error(VReadData((char *)feature_map_buf, 2, 0x800800, fp, &j));
		fclose(fp);
		for (j=0; j<=(vh_in.scn.largest_density_value_valid?
				vh_in.scn.largest_density_value[0]: 65535); j++)
			if (feature_map_max <= 4095)
				gray_map[j] = j;
			else if (j <= feature_map_max)
				gray_map[j] = (unsigned short)(j*4095/feature_map_max);
			else
				gray_map[j] = 4095;
		return;
	}
	assert(threeD_hist_filename);
	fp = fopen(threeD_hist_filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Could not open histogram file.\n");
		exit(-1);
	}
	handle_error(VReadData((char *)&bins_per_feature, 4, 1, fp, &j));
	hist_bin_table[0] = (int *)malloc(bins_per_feature*sizeof(int));
	hist_bin_table[1] = (int *)malloc(bins_per_feature*sizeof(int));
	hist_bin_table[2] = (int *)malloc(bins_per_feature*sizeof(int));
	handle_error(VReadData((char *)hist_bin_table[0], 4, bins_per_feature, fp, &j));
	handle_error(VReadData((char *)hist_bin_table[1], 4, bins_per_feature, fp, &j));
	handle_error(VReadData((char *)hist_bin_table[2], 4, bins_per_feature, fp, &j));
	threeD_hist= (unsigned short ***)malloc(bins_per_feature*sizeof(short **));
	for (k=0; k<bins_per_feature; k++)
	{
		threeD_hist[k] =
		    (unsigned short **)malloc(bins_per_feature*sizeof(short *));
		for (m=0; m<bins_per_feature; m++)
		{
			threeD_hist[k][m] =
			    (unsigned short *)malloc(bins_per_feature*sizeof(short));
			handle_error(VReadData((char *)threeD_hist[k][m], 2, bins_per_feature,
				fp, &j));
		}
	}
}

/*****************************************************************************
 * FUNCTION: load_fom
 * DESCRIPTION: Reads the fuzzy object model data.
 * PARAMETERS: None
 * SIDE EFFECTS: The variables vh_fom, fom_data, slice_spacing are initialized.
 * ENTRY CONDITIONS: The variable fom_filename must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error writes a message to stderr and exits with code !=0
 * HISTORY:
 *    Created: 2/18/10 by Dewey Odhner
 *
 *****************************************************************************/
void load_fom()
{
	int slice_size, items_read, error_code;
	FILE *fp;
	char group[5], element[5];

	fp = fopen(fom_filename, "rb");
	if (fp == NULL)
		handle_error(4);
	error_code = VReadHeader(fp, &vh_fom, group, element);
	switch (error_code)
	{
		case 0:
		case 106:
		case 107:
			break;
		default:
			fprintf(stderr, "file = %s; group = %s; element = %s\n",
				input_filename, group, element);
			handle_error(error_code);
	}
	if (!(vh_fom.scn.dimension == 3) ||
			!(vh_fom.scn.num_of_density_values == 1) ||
			!(vh_fom.scn.num_of_integers == 1) ||
			!(vh_fom.scn.signed_bits_valid==0|| vh_fom.scn.signed_bits[0]==0)||
			!(vh_fom.scn.num_of_bits==16) ||
			!(vh_fom.scn.bit_fields[0] == 0) ||
			!(vh_fom.scn.bit_fields[1] == vh_fom.scn.num_of_bits-1) ||
			!(vh_fom.scn.bytes_in_alignment_valid==0 ||
			vh_fom.scn.bytes_in_alignment<=1))
	{
		fprintf(stderr, "Not able to use this fom scene.\n");
		exit(-1);
	}
	slice_size = vh_fom.scn.xysize[0]*vh_fom.scn.xysize[1];
	slice_spacing = vh_fom.scn.num_of_subscenes[0]>1?
			vh_fom.scn.loc_of_subscenes[1]-vh_fom.scn.loc_of_subscenes[0]:
			vh_fom.scn.xypixsz[0];
	fom_data = (unsigned short *)
		malloc(slice_size*vh_fom.scn.num_of_subscenes[0]*2);
	if (fom_data == NULL)
		handle_error(1);
	handle_error(VSeekData(fp, 0));
	handle_error(VReadData((char *)fom_data, 2,
		slice_size*vh_fom.scn.num_of_subscenes[0], fp, &items_read));
	fclose(fp);
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

/*****************************************************************************
 * FUNCTION: load_dfom
 * DESCRIPTION: Reads the directed fuzzy object model data.
 * PARAMETERS: None
 * SIDE EFFECTS: The variable dfom_data is initialized.
 * ENTRY CONDITIONS: The variable fom_filename must be properly set.
 *    The scene must match the fom scene, if used, otherwise the input scene.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error writes a message to stderr and exits with code !=0
 * HISTORY:
 *    Created: 7/18/12 by Dewey Odhner
 *
 *****************************************************************************/
void load_dfom()
{
	int slice_size, items_read, error_code;
	FILE *fp;
	ViewnixHeader vh;
	char group[5], element[5];

	if (dfom_filename == NULL)
		return;
	fp = fopen(dfom_filename, "rb");
	if (fp == NULL)
		handle_error(4);
	error_code = VReadHeader(fp, &vh, group, element);
	switch (error_code)
	{
		case 0:
		case 106:
		case 107:
			break;
		default:
			fprintf(stderr, "file = %s; group = %s; element = %s\n",
				input_filename, group, element);
			handle_error(error_code);
	}
	if (!(vh.scn.dimension == 3) ||
			!(vh.scn.num_of_density_values == 1) ||
			!(vh.scn.num_of_integers == 1) ||
			!(vh.scn.signed_bits_valid==0|| vh.scn.signed_bits[0]==0)||
			!(vh.scn.num_of_bits==16) ||
			!(vh.scn.bit_fields[0] == 0) ||
			!(vh.scn.bit_fields[1] == vh.scn.num_of_bits-1) ||
			!(vh.scn.bytes_in_alignment_valid==0 ||
			vh.scn.bytes_in_alignment<=1) ||
			(fom_filename? (vh.scn.xysize[0]!=vh_fom.scn.xysize[0] ||
				vh.scn.xysize[1]!=vh_fom.scn.xysize[1] ||
				vh.scn.num_of_subscenes[0]!=vh_fom.scn.num_of_subscenes[0]):
				(vh.scn.xysize[0]!=vh_in.scn.xysize[0] ||
				vh.scn.xysize[1]!=vh_in.scn.xysize[1] ||
				vh.scn.num_of_subscenes[0]!=vh_in.scn.num_of_subscenes[0])))
	{
		fprintf(stderr, "Not able to use this dfom scene.\n");
		exit(-1);
	}
	slice_size = vh.scn.xysize[0]*vh.scn.xysize[1];
	dfom_data = (unsigned short *)
		malloc(slice_size*vh.scn.num_of_subscenes[0]*2);
	if (dfom_data == NULL)
		handle_error(1);
	handle_error(VSeekData(fp, 0));
	handle_error(VReadData((char *)dfom_data, 2,
		slice_size*vh.scn.num_of_subscenes[0], fp, &items_read));
	fclose(fp);
	destroy_scene_header(&vh);
}

/*****************************************************************************
 * FUNCTION: input_slice_index
 * DESCRIPTION: Returns the slice number from the beginning of the file
 *    given the slice number from the beginning of the volume in the subscene.
 * PARAMETERS:
 *    volume, slice: volume and slice number from zero in the subscene.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables file_info, vh_in must be properly set.
 * RETURN VALUE: the slice number from the beginning of the file
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 3/27/96 by Dewey Odhner
 *
 *****************************************************************************/
int input_slice_index(int volume, int slice)
{
	int j, k;

	slice = file_info.min[0]+slice*file_info.incr[0];
	switch (vh_in.scn.dimension)
	{
		case 3:
			assert(volume == 0);
			assert(slice <= vh_in.scn.num_of_subscenes[0]);  // xinjian 2008.5.30
			j = slice;
			break;
		case 4:
			volume = file_info.min[1]+volume*file_info.incr[1];
			assert(volume < vh_in.scn.num_of_subscenes[0]);
			assert(slice <= file_info.max[0]);
			for (j=k=0; k<volume; k++)
				j += vh_in.scn.num_of_subscenes[k+1];
			if (slice >= vh_in.scn.num_of_subscenes[k+1])
				return (-1);
			j += slice;
			break;
		default:
			assert(FALSE);
	}
	return j;
}

/*****************************************************************************
 * FUNCTION: slice_from_input_index
 * DESCRIPTION: Returns the slice number from the beginning of the volume in
 *    the subscene given the slice number from the beginning of the file.
 * PARAMETERS:
 *    index: the slice number from the beginning of the file
 *    volume: the volume number in the subscene from zero
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables file_info, vh_in must be properly set.
 * RETURN VALUE: The slice number from the beginning of the volume in
 *    the subscene, or -1 if the slice is not in that volume.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 3/27/96 by Dewey Odhner
 *
 *****************************************************************************/
int slice_from_input_index(int index, int volume)
{
	int j, k;

	switch (vh_in.scn.dimension)
	{
		case 3:
			assert(volume == 0);
			if (index >= vh_in.scn.num_of_subscenes[0])
				return (-1);
			break;
		case 4:
			volume = file_info.min[1]+volume*file_info.incr[1];
			assert(volume < vh_in.scn.num_of_subscenes[0]);
			for (k=0; k<volume; k++)
				index -= vh_in.scn.num_of_subscenes[k+1];
			if (index<0 || index>=vh_in.scn.num_of_subscenes[k+1])
				return (-1);
			break;
		default:
			assert(FALSE);
	}
	j = index-file_info.min[0];
	if (j % file_info.incr[0])
		return (-1);
	return j/file_info.incr[0];
}

/*****************************************************************************
 * FUNCTION: get_tissues
 * DESCRIPTION: Loads parameters of tissues
 * PARAMETERS:
 *    tf: file name
 * SIDE EFFECTS: tissue_level, tissue_width, num_tissues will be initialized.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: non-zero on error
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 4/5/13 by Dewey Odhner
 *    Modified: 6/21/13 tissue_width doubled by Dewey Odhner
 *
 *****************************************************************************/
int get_tissues(char tf[])
{
	FILE *fp;
	int j;

	fp = fopen(tf, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open %s\n", tf);
		return 1;
	}
	if (fscanf(fp, "%d\n", &num_tissues) != 1)
	{
		fprintf(stderr, "Failure reading %s\n", tf);
		return 1;
	}
	tissue_level = (float *)malloc(num_tissues*sizeof(float));
	tissue_width = (float *)malloc(num_tissues*sizeof(float));
	tissue_type = (int *)malloc(num_tissues*sizeof(int));
	if (tissue_level==NULL || tissue_width==NULL || tissue_type==NULL)
	{
		fprintf(stderr, "Out of memory\n");
		return 1;
	}
	for (j=0; j<num_tissues; j++)
	{
		if (fscanf(fp, "%f %f\n", tissue_level+j, tissue_width+j) != 2)
		{
			fprintf(stderr, "Failure reading %s\n", tf);
			return 1;
		}
		tissue_width[j] *= 2;
	}
	if (fscanf(fp, "%d\n", &fg_tissues) != 1)
	{
		fprintf(stderr, "Failure reading %s\n", tf);
		return 1;
	}
	for (j=0; j<num_tissues; j++)
		if (fscanf(fp, " %d", tissue_type+j) != 1)
			break;
	if (j < num_tissues)
	{
		fprintf(stderr, "Failure reading %s\n", tf);
		return 1;
	}
	fclose(fp);
	return 0;
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
		    if (bg_flag == 1)
			{
				sprintf(msg, "job_done %s -abort &", argv0);
				system(msg);
			    VDeleteBackgroundProcessInformation();
			}
			exit(1);
	}
}

/*****************************************************************************
 * FUNCTION: usage
 * DESCRIPTION: Displays the usage message on stderr and exits with code 1.
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable argv0 should be set to the program name.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits with code 1.
 * HISTORY:
 *    Created: 4/2/96 by Dewey Odhner
 *    Modified: 4/17/96 output of original values allowed by Dewey Odhner
 *    Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
 *    Modified: 7/25/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 1/27/98 direction-specific affinity image stored
 *       by Dewey Odhner
 *    Modified: 1/30/98 fuzzy adjacency flag added by Dewey Odhner
 *    Modified: 2/20/98 points_filename added by Dewey Odhner
 *    Modified: 2/9/99 tracking algorithm switch added by Dewey Odhner
 *
 *****************************************************************************/
void usage()
{
	fprintf(stderr, "Usage: %s <in> ", argv0); 
	fprintf(stderr, "[1 <ss> <se> <si> | 2 <ss> <se> <si> <vs> <ve> <vi>] ");
	fprintf(stderr, "<out> <th> <bg> [-mask_original] ");
	fprintf(stderr, "[-multitissue <wt> <tf>] ");
	fprintf(stderr, "[[-feature <ft> <fn> <wt> <lv> <wd>] ... | ");
	fprintf(stderr, "-histogram <hf> | -covariance <nf> <ft> ... ");
	fprintf(stderr, "[-normal <tm> ... <ic> ...] ");
	fprintf(stderr, "[-reverse <rm> ... <ric> ...]] ");
	fprintf(stderr, "[-additive] ");
	fprintf(stderr, "[-fom <fm> <tx> <ty> <tz> <sc> <cx> <cy> <cz>] ");
	fprintf(stderr, "[-pfom <fm> <tx> <ty> <tz>] ");
	fprintf(stderr, "[-dfom <dfm>] ");
	fprintf(stderr, "[-dual_object <lv0> <wd0> <lv1> <wd1> <wdh>] ");
	fprintf(stderr, "[-rotation <A1> <A2> <A3>] ");
	fprintf(stderr, "[[-fuzzy_adjacency | -2D_adjacency]] ");
	fprintf(stderr, "[-out_affinity xyz] ");
	fprintf(stderr, "[-track_algorithm <ta>] ");
	fprintf(stderr, "[-bg_points_file <bpf>] ");
	fprintf(stderr, "[[-feature_map_file | -3Dhist] <fmf>] ");
	fprintf(stderr, "[<pf>] [<x> <y> <z>] ... [-bg_points <nbp>] [-mofs] ");
	fprintf(stderr, "[-o <os>]\n");

	fprintf(stderr, "<in>: input filename\n1: 3-dimensional scene\n");
	fprintf(stderr, "2: 4-dimensional scene\n<ss>: starting slice (from 0)\n");
	fprintf(stderr, "<se>: ending slice (from 0)\n<si>: slice increment\n");
	fprintf(stderr, "<vs>: starting volume (from 0)\n");
	fprintf(stderr, "<ve>: ending volume (from 0)\n<vi>: volume increment\n");
	fprintf(stderr, "<out>: output filename\n");
	fprintf(stderr, "<th>: threshold (0 to 100)\n");
	fprintf(stderr, "<bg>: non-zero for background execution\n");
	fprintf(stderr, "<tf>: tissue feature filename\n");
	fprintf(stderr, "<ft>: feature number:\n 0=high; 1=low; ");
	fprintf(stderr, "2=difference; 3=sum; 4=relative difference (0 to 100;");
	fprintf(stderr, " 0 to 65535 with covariance)\n");
	fprintf(stderr, "<fn>: function number: 0=gaussian; 1=ramp; 2=box\n");
	fprintf(stderr, "<wt>: weight (-1 to 1)\n");
	fprintf(stderr, "<lv>: function center level\n");
	fprintf(stderr, "<wd>: function width\n");
	fprintf(stderr, "<hf>: histogram file\n");
	fprintf(stderr, "<nf>: number of features\n");
	fprintf(stderr, "<tm>: training mean\n");
	fprintf(stderr, "<ic>: inverse covariance matrix element\n");
	fprintf(stderr, "<rm>: reverse training mean\n");
	fprintf(stderr, "<ric>: inverse reverse covariance matrix element\n");
	fprintf(stderr, "<fm>: fuzzy model scene\n");
	fprintf(stderr, "<dfm>: directed fuzzy model scene\n");
	fprintf(stderr, "<tx> <ty> <tz>: translation of fuzzy model\n");
	fprintf(stderr, "<sc>: scaling factor\n");
	fprintf(stderr, "<cx> <cy> <cz>: centroid of fuzzy model\n");
	fprintf(stderr, "<A1>: the angle between z-axis and rotation axis\n");
	fprintf(stderr, "<A2>: the angle of rotation axis projection on x-y plane\n");
	fprintf(stderr, "<A3>: the rotation angle\n");
	fprintf(stderr, "<ta>: tracking algorithm: 0=hheap, 1=chash, 2=chash2\n");
	fprintf(stderr, "<bpf>: background seed filename (binary scene)\n");
	fprintf(stderr, "<fmf>: feature map filename (unsigned short data)\n");
	fprintf(stderr, "<pf>: input points filename (binary scene)\n");
	fprintf(stderr, "<x>, <y>: pixel of starting point\n");
	fprintf(stderr, "<z>: slice index in input file of starting point\n");
	fprintf(stderr, "<nbp>: number of background seeds\n");
	fprintf(stderr, "<os>: output stream\n");
	exit(1);
}

/*****************************************************************************
 * FUNCTION: parse_command_line
 * DESCRIPTION: Initializes the variables input_filename, file_info,
 *    output_filename, bg_flag, feature_status, function_selected, weight,
 *    function_level, function_width, point_picked, num_points_picked,
 *    threshold, mask_original, training_mean, reverse_training_mean,
 *    inv_covariance, inv_reverse_covariance, covariance_flag,
 *    reverse_covariance_flag, out_affinity_flag, fuzzy_adjacency_flag,
 *    points_filename, weight_unit, i_weight_unit, ioffset
 *    according to the command line arguments.
 * PARAMETERS:
 *    argc: number of command line arguments
 *    argv: command line arguments
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables feature_status, mask_original, affinity_type
 *     covariance_flag, reverse_covariance_flag be initialized with zeros.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Writes an error message to stderr and exits with code 1
 *    if command line does not parse or memory allocation fails.
 * HISTORY:
 *    Created: 4/2/96 by Dewey Odhner
 *    Modified: 4/17/96 output of original values allowed by Dewey Odhner
 *    Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
 *    Modified: 7/29/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 1/27/98 direction-specific affinity image stored
 *       by Dewey Odhner
 *    Modified: 1/30/98 fuzzy adjacency flag initialized by Dewey Odhner
 *    Modified: 2/20/98 points_filename initialized by Dewey Odhner
 *    Modified: 2/9/99 tracking algorithm switch initialized by Dewey Odhner
 *
 *****************************************************************************/
void parse_command_line(int argc, char *argv[])
{
	int args_parsed, current_feature, inum, imin, imax, iinc, features_on, j;
	static short min[2], max[2], incr[2];
	double total_weight=0;

	if (argc>2 && strcmp(argv[argc-2], "-o")==0)
	{
		outstream = fopen(argv[argc-1], "wb");
		argc -= 2;
	}
	else
		outstream = stdout;
	if (argc>11 && strcmp(argv[argc-1], "-mofs")==0)
	{
		mofs_flag = TRUE;
		argc--;
	}
	if (argc>11 && strcmp(argv[argc-2], "-bg_points")==0 &&
	    	sscanf(argv[argc-1], "%d", &num_bg_points)==1)
	{
		argc -= num_bg_points*3+2;
		bg_point = (int(*)[3])malloc(num_bg_points*3*sizeof(int));
		for (j=0; j<num_bg_points; j++)
			if (sscanf(argv[argc+3*j], "%d", bg_point[j])!=1 ||
					sscanf(argv[argc+3*j+1], "%d", bg_point[j]+1)!=1 ||
					sscanf(argv[argc+3*j+2], "%d", bg_point[j]+2)!=1)
				usage();
	}
	if (argc < 7)
		usage();
	input_filename = argv[1];
	file_info.min = min;
	file_info.max = max;
	file_info.incr = incr;
	if (sscanf(argv[2], "%d", &inum)!=1 || sscanf(argv[3], "%d", &imin)!=1 ||
			sscanf(argv[4], "%d", &imax)!=1 || sscanf(argv[5], "%d", &iinc)!=1)
		usage();
	file_info.num_of_elem = inum;
	file_info.min[0] = imin;
	file_info.max[0] = imax;
	file_info.incr[0] = iinc;
	args_parsed = 6;
	if (file_info.num_of_elem == 2)
	{
		if (sscanf(argv[6], "%d", &imin)!=1 ||
				sscanf(argv[7], "%d", &imax)!=1 ||
				sscanf(argv[8], "%d", &iinc)!=1)
			usage();
		file_info.min[1] = imin;
		file_info.max[1] = imax;
		file_info.incr[1] = iinc;
		args_parsed = 9;
	}
	output_filename = argv[args_parsed++];
	if (sscanf(argv[args_parsed++], "%f", &threshold) != 1)
		usage();
	threshold *= (float)(.01*MAX_CONNECTIVITY);
	if (sscanf(argv[args_parsed++], "%d", &bg_flag) != 1)
		usage();
	if (argc>args_parsed && strcmp(argv[args_parsed], "-mask_original")==0)
	{
		mask_original = TRUE;
		args_parsed++;
	}
	if (argc>args_parsed && strcmp(argv[args_parsed], "-multitissue")==0)
	{
		affinity_type = 3;
		if (argc<args_parsed+3 ||
				sscanf(argv[args_parsed+1], "%f", weight+MULTITISSUE)!=1 ||
				get_tissues(argv[args_parsed+2]))
			usage();
		else
		{
			feature_status[MULTITISSUE] = TRUE;
			total_weight = weight[MULTITISSUE];
			args_parsed += 3;
		}
	}
	for (; argc>args_parsed&&strcmp(argv[args_parsed], "-feature")==0;
			args_parsed+=6)
		if (argc<args_parsed+6 ||
				sscanf(argv[args_parsed+1], "%d", &current_feature)!=1 ||
				sscanf(argv[args_parsed+2], "%d",
					function_selected+current_feature)!=1 ||
				sscanf(argv[args_parsed+3], "%f", weight+current_feature)!=1 ||
				sscanf(argv[args_parsed+4], "%f",
					function_level+current_feature)!=1 ||
				sscanf(argv[args_parsed+5], "%f",
					function_width+current_feature)!=1)
			usage();
		else
		{
			feature_status[current_feature] = TRUE;
			total_weight += weight[current_feature];
		}
	weight_unit = (float)(1/total_weight);
	i_weight_unit = weight_unit/MAX_CONNECTIVITY;
	if (argc>args_parsed && strcmp(argv[args_parsed], "-histogram")==0)
	{
		affinity_type = 1;
		args_parsed++;
		if (argc <= args_parsed)
			usage();
		histogram_filename = argv[args_parsed++];
	}
	if (argc>args_parsed+1 && strcmp(argv[args_parsed], "-covariance")==0)
	{
		affinity_type = 2;
		args_parsed++;
		if (sscanf(argv[args_parsed++], "%d", &features_on)!=1 ||
				argc-args_parsed<features_on)
			usage();
		for (j=0; j<features_on; j++)
		{
			if (sscanf(argv[args_parsed++], "%d", &current_feature)!=1 ||
					current_feature>=NUM_FEATURES)
				usage();
			feature_status[current_feature] = TRUE;
		}
		if (argc>args_parsed+features_on*(1+features_on) &&
				strcmp(argv[args_parsed], "-normal")==0)
		{
			args_parsed++;
			for (j=0; j<features_on; j++)
				if (sscanf(argv[args_parsed++], "%lf", training_mean+j) != 1)
					usage();
			for (j=0; j<features_on*features_on; j++)
				if (sscanf(argv[args_parsed++], "%lf", inv_covariance+j) != 1)
					usage();
			covariance_flag = TRUE;
		}
		if (argc>args_parsed+features_on*(1+features_on) &&
				strcmp(argv[args_parsed], "-reverse")==0)
		{
			args_parsed++;
			for (j=0; j<features_on; j++)
				if (sscanf(argv[args_parsed++], "%lf", reverse_training_mean+j)
						!= 1)
					usage();
			for (j=0; j<features_on*features_on; j++)
				if (sscanf(argv[args_parsed++], "%lf",inv_reverse_covariance+j)
						!= 1)
					usage();
			reverse_covariance_flag = TRUE;
		}
		function_level[4] *= (float)655.35;
		function_width[4] *= (float)655.35;
	}
	else
	{
		function_level[4] *= (float).01;
		function_width[4] *= (float).01;
	}
	if (argc>args_parsed && strcmp(argv[args_parsed], "-additive")==0)
	{
		affinity_type = 3;
		args_parsed++;
	}
	if (argc>args_parsed+8 && strcmp(argv[args_parsed], "-fom")==0)
	{
		affinity_type = 3;
		fom_filename = argv[args_parsed+1];
		if (sscanf(argv[args_parsed+2], "%lf", translation)!=1 ||
				sscanf(argv[args_parsed+3], "%lf", translation+1)!=1 ||
				sscanf(argv[args_parsed+4], "%lf", translation+2)!=1 ||
				sscanf(argv[args_parsed+5], "%lf", &rel_scale)!=1 ||
				sscanf(argv[args_parsed+6], "%lf", centroid)!=1 ||
				sscanf(argv[args_parsed+7], "%lf", centroid+1)!=1 ||
				sscanf(argv[args_parsed+8], "%lf", centroid+2)!=1)
			usage();
		args_parsed += 9;
		if (!feature_status[6])
		{
			fprintf(stderr, "Specify feature 6.\n");
			usage();
		}
		for (j=0; j<3; j++)
		{
			translation[j] += (1-rel_scale)*centroid[j];
			centroid[j] *= rel_scale;
		}
		rel_scale = 1/rel_scale;
	}
	if (argc>args_parsed+4 && strcmp(argv[args_parsed], "-pfom")==0)
	{
		affinity_type = 3;
		fom_filename = argv[args_parsed+1];
		if (sscanf(argv[args_parsed+2], "%d", ioffset)!=1 ||
				sscanf(argv[args_parsed+3], "%d", ioffset+1)!=1 ||
				sscanf(argv[args_parsed+4], "%d", ioffset+2)!=1)
			usage();
		args_parsed += 5;
		if (!feature_status[6])
		{
			fprintf(stderr, "Specify feature 6.\n");
			usage();
		}
		for (j=0; j<3; j++)
		{
			translation[j] = 0;
			centroid[j] = 0;
		}
		rel_scale = 1;
		prescaled = TRUE;
	}
	if (argc>args_parsed+1 && strcmp(argv[args_parsed], "-dfom")==0)
	{
		dfom_filename = argv[args_parsed+1];
		args_parsed += 2;
		if (fom_filename == NULL)
		{
			for (j=0; j<3; j++)
			{
				ioffset[j] = 0;
				translation[j] = 0;
				centroid[j] = 0;
			}
			rel_scale = 1;
		}
	}
	if (argc>args_parsed+5 && strcmp(argv[args_parsed], "-dual_object")==0)
	{
		affinity_type = 4;
		if (sscanf(argv[args_parsed+1], "%f", function_level)!=1 ||
				sscanf(argv[args_parsed+2], "%f", function_width)!=1 ||
				sscanf(argv[args_parsed+3], "%f", function_level+1)!=1 ||
				sscanf(argv[args_parsed+4], "%f", function_width+1)!=1 ||
				sscanf(argv[args_parsed+5], "%f", function_width+2)!=1)
			usage();
		args_parsed += 6;
	}
	if (argc>args_parsed+3 && strcmp(argv[args_parsed], "-rotation")==0)
	{
		double angle[3];
		rotate_flag = TRUE;
		if (sscanf(argv[args_parsed+1], "%lf", angle)!=1 ||
				sscanf(argv[args_parsed+2], "%lf", angle+1)!=1 ||
				sscanf(argv[args_parsed+3], "%lf", angle+2)!=1)
			usage();
		for (j=0; j<3; j++)
			angle[j] *= M_PI/180;
		AtoM(rotation, angle[0], angle[1], angle[2]);
		args_parsed += 4;
	}
	if (argc>args_parsed && strcmp(argv[args_parsed], "-fuzzy_adjacency")==0)
	{
		fuzzy_adjacency_flag = TRUE;
		args_parsed++;
	}
	else if (argc>args_parsed && strcmp(argv[args_parsed], "-2D_adjacency")==0)
	{
		fuzzy_adjacency_flag = -1;
		args_parsed++;
	}
	if (argc>args_parsed && strcmp(argv[args_parsed], "-out_affinity")==0)
	{
		out_affinity_flag[0] = out_affinity_flag[1] = out_affinity_flag[2] = 0;
		args_parsed++;
		if (argc <= args_parsed)
			usage();
		for (j=0; j<(int)strlen(argv[args_parsed]); j++)
			switch (argv[args_parsed][j])
			{
				case 'x':
					out_affinity_flag[0] = 1;
					break;
				case 'y':
					out_affinity_flag[1] = 1;
					break;
				case 'z':
					out_affinity_flag[2] = 1;
					break;
				default:
					usage();
			}
		args_parsed++;
	}
	if (argc>args_parsed && strcmp(argv[args_parsed], "-track_algorithm")==0)
	{
		args_parsed++;
		if (argc<=args_parsed ||
				sscanf(argv[args_parsed], "%d", &track_algorithm)!=1)
			usage();
		args_parsed++;
	}
	if (argc>args_parsed+1 && strcmp(argv[args_parsed], "-bg_points_file")==0)
	{
		bg_filename = argv[args_parsed+1];
		args_parsed += 2;
	}
	if (argc>args_parsed+1 && strcmp(argv[args_parsed],"-feature_map_file")==0)
	{
		feature_map_filename = argv[args_parsed+1];
		args_parsed += 2;
		if (!feature_status[7])
		{
			fprintf(stderr, "Specify feature 7 (weight).\n");
			usage();
		}
	}
	else if (argc>args_parsed+1 && strcmp(argv[args_parsed], "-3Dhist")==0)
	{
		threeD_hist_filename = argv[args_parsed+1];
		args_parsed += 2;
		if (!feature_status[7])
		{
			fprintf(stderr, "Specify feature 7 (weight).\n");
			usage();
		}
	}
	if (argc>args_parsed && strchr(argv[args_parsed], '.') &&
			(strcmp(strrchr(argv[args_parsed], '.'), ".BIM")==0 ||
			strcmp(strrchr(argv[args_parsed], '.'), ".CBI")==0))
	{
		points_filename = argv[args_parsed];
		args_parsed++;
	}
	if ((argc-args_parsed)%3)
		usage();
	if (args_parsed == argc)
	{
		if (points_filename == NULL)
		{
			fprintf(stderr, "No seed point specified.\n");
			usage();
		}
		return;
	}
	if (argc > args_parsed)
	{
		point_picked =
		    (void *)malloc((argc-args_parsed)/3*sizeof(point_picked[0]));
		if (point_picked == NULL)
			handle_error(1);
	}
	for (num_points_picked=0; argc>args_parsed;
			num_points_picked++,args_parsed+=3)
		if (sscanf(argv[args_parsed], "%d",
				point_picked[num_points_picked])!=1 ||
				sscanf(argv[args_parsed+1], "%d",
				point_picked[num_points_picked]+1)!=1 ||
				sscanf(argv[args_parsed+2], "%d",
				point_picked[num_points_picked]+2)!=1)
			usage();
}

// h: C --> {-1} U [0, 1]
// hl[c] = lambda + (h(c)<0? 0: 2+2*rint(MAX_CONNECTIVITY*h(c)))

/*****************************************************************************
 * FUNCTION: GC_max
 * DESCRIPTION: Computes the connectedness values and stores them at out_data.
 * PARAMETERS:
 *    current_volume: volume number from zero in the scene.
 * SIDE EFFECTS: Messages may be written to stdout.
 * ENTRY CONDITIONS: The variables feature_status, function_level,
 *    function_width, weight, function_selected, file_info, vh_in, in_data,
 *    fuzzy_adjacency_flag, slice_spacing, point_picked, num_points_picked,
 *    affinity_type, bg_point, num_bg_points, vh_out must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 6/28/11 by Dewey Odhner
 *
 *****************************************************************************/
/*
 K.C. Ciesielski, J.K. Udupa, A.X. Falcão, P.A.V. Miranda, Fuzzy Connectedness
 image segmentation in Graph Cut formulation: A linear-time algorithm and a
 comparative analysis. Journal of Mathematical Imaging and Vision 44 (2012),
 375–398.
*/
void GC_max(int current_volume)
{
	const int nil=-1;
	GQueue *Q;
	int *hl;
	int *R, *Pr;
	int slices_out=vh_out.scn.num_of_subscenes[vh_in.scn.dimension==3? 0:
		1+current_volume];
	int c, d, j;
	float x_adjacency, y_adjacency, z_adjacency;

	if ((double)slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1] > INT_MAX)
	{
		fprintf(stderr, "Scene is too large for data structure.\n");
		exit(-1);
	}
	x_adjacency = y_adjacency = z_adjacency = MAX_CONNECTIVITY;
	if (fuzzy_adjacency_flag < 0)
		z_adjacency = 0;
	else if (fuzzy_adjacency_flag)
	{
		if (vh_in.scn.xypixsz[0]<=vh_in.scn.xypixsz[1] &&
				vh_in.scn.xypixsz[0]<=slice_spacing)
		{
			y_adjacency *= vh_in.scn.xypixsz[0]/vh_in.scn.xypixsz[1];
			z_adjacency *= vh_in.scn.xypixsz[0]/slice_spacing;
		}
		else if (vh_in.scn.xypixsz[1]<=vh_in.scn.xypixsz[0] &&
				vh_in.scn.xypixsz[1]<=slice_spacing)
		{
			x_adjacency *= vh_in.scn.xypixsz[1]/vh_in.scn.xypixsz[0];
			z_adjacency *= vh_in.scn.xypixsz[1]/slice_spacing;
		}
		else
		{
			x_adjacency *= slice_spacing/vh_in.scn.xypixsz[0];
			y_adjacency *= slice_spacing/vh_in.scn.xypixsz[1];
		}
	}
	hl = (int *)malloc(
	    slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*sizeof(int));
	R = (int *)
		malloc(slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*sizeof(int));
	Pr = (int *)
		malloc(slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*sizeof(int));
	if (hl==NULL || R==NULL || Pr==NULL)
		handle_error(1);
	Q = CreateGQueue(MAX_CONNECTIVITY*2+4,
		slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1], hl);
	if (Q == NULL)
		exit(-1);
	SetRemovalPolicy(Q, MAXVALUE);

#define DirecAff(a, b, adjacency, ax, ay, az, bx, by, bz) ( \
        affinity(a, b, adjacency, ax, ay, az, bx, by, bz, hl[c]&1) )

	// step 1.
	for (c=0; c<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; c++)
	{
		hl[c] = 0;
		R[c] = c;
		Pr[c] = c;
	}
	for (j=0; j<num_points_picked; j++)
	{
		c = point_picked[j][0]+vh_in.scn.xysize[0]*
			(point_picked[j][1]+vh_in.scn.xysize[1]*point_picked[j][2]);
		hl[c] = 2+2*MAX_CONNECTIVITY;
		Pr[c] = nil;
	}
	for (j=0; j<num_bg_points; j++)
	{
		c = bg_point[j][0]+vh_in.scn.xysize[0]*
			(bg_point[j][1]+vh_in.scn.xysize[1]*bg_point[j][2]);
		hl[c] = 3+2*MAX_CONNECTIVITY;
		Pr[c] = nil;
	}
	if (points_filename)
	{
		FILE *fp_pts;
		char group[5], element[5];
		unsigned char *in_points_data;
		Voxel cur;
		int counter;

		fp_pts = fopen(points_filename, "rb");
		if (fp_pts == NULL)
			handle_error(4);
		handle_error(VReadHeader(fp_pts, &vh_pts, group, element));
		in_points_data = (unsigned char *)
			malloc((vh_pts.scn.xysize[0]*vh_pts.scn.xysize[1]+7)/8);
		handle_error(VSeekData(fp_pts, 0));
		for (cur.z=0; cur.z<vh_pts.scn.num_of_subscenes[0]; cur.z++)
		{
			handle_error(VReadData((char *)in_points_data, 1,
				(vh_pts.scn.xysize[0]*vh_pts.scn.xysize[1]+7)/8, fp_pts, &j));
			counter = 0;
			j = 128;
			for (cur.y=0; cur.y<vh_pts.scn.xysize[1]; cur.y++)
				for (cur.x=0; cur.x<vh_pts.scn.xysize[0]; cur.x++)
				{
					if (in_points_data[counter] & j)
	    			{
						c = cur.x+vh_in.scn.xysize[0]*
							(cur.y+vh_in.scn.xysize[1]*cur.z);
						hl[c] = 2+2*MAX_CONNECTIVITY;
						Pr[c] = nil;
					}
					j >>= 1;
					if (j == 0)
					{
						j = 128;
						counter++;
					}
				}
		}
		free(in_points_data);
		fclose(fp_pts);
	}
	if (bg_filename)
	{
		FILE *fp_pts;
		char group[5], element[5];
		unsigned char *in_points_data;
		Voxel cur;
		int counter;

		fp_pts = fopen(bg_filename, "rb");
		if (fp_pts == NULL)
			handle_error(4);
		handle_error(VReadHeader(fp_pts, &vh_bg_pts, group, element));
		in_points_data = (unsigned char *)
			malloc((vh_bg_pts.scn.xysize[0]*vh_bg_pts.scn.xysize[1]+7)/8);
		handle_error(VSeekData(fp_pts, 0));
		for (cur.z=0; cur.z<vh_bg_pts.scn.num_of_subscenes[0]; cur.z++)
		{
			handle_error(VReadData((char *)in_points_data, 1,
				(vh_bg_pts.scn.xysize[0]*vh_bg_pts.scn.xysize[1]+7)/8,
				fp_pts, &j));
			counter = 0;
			j = 128;
			for (cur.y=0; cur.y<vh_bg_pts.scn.xysize[1]; cur.y++)
				for (cur.x=0; cur.x<vh_bg_pts.scn.xysize[0]; cur.x++)
				{
					if (in_points_data[counter] & j)
	    			{
						c = cur.x+vh_in.scn.xysize[0]*
							(cur.y+vh_in.scn.xysize[1]*cur.z);
						hl[c] = 3+2*MAX_CONNECTIVITY;
						Pr[c] = nil;
					}
					j >>= 1;
					if (j == 0)
					{
						j = 128;
						counter++;
					}
				}
		}
		free(in_points_data);
		fclose(fp_pts);
	}
	// step 2.
	for (c=0; c<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; c++)
		InsertGQueue(&Q, c);
	// step 3.
	while (!EmptyGQueue(Q))
	{
		int cx, cy, cz, neighbor[6], kappa[6], ncount=0;
		int ch, dh, minh;
		// step 4.
		c = RemoveGQueue(Q);
		cz = c/((int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]);
		cy= (c-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*cz)/
			vh_in.scn.xysize[0];
		cx = c-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*cz-
			vh_in.scn.xysize[0]*cy;
		// step 5.
		if (cx>0 && hl[c-1]>=0)
		{
			neighbor[ncount] = c-1;
			kappa[ncount] = vh_in.scn.num_of_bits==8?
				DirecAff(in_data_8[c], in_data_8[c-1], x_adjacency,
				cx,cy,cz, cx-1,cy,cz):
				DirecAff(in_data_16[c],in_data_16[c-1], x_adjacency,
				cx,cy,cz, cx-1,cy,cz);
			ncount++;
		}
		if (cx<vh_in.scn.xysize[0]-1 && hl[c+1]>=0)
		{
			neighbor[ncount] = c+1;
			kappa[ncount] = vh_in.scn.num_of_bits==8?
				DirecAff(in_data_8[c], in_data_8[c+1], x_adjacency,
				cx,cy,cz, cx+1,cy,cz):
				DirecAff(in_data_16[c],in_data_16[c+1], x_adjacency,
				cx,cy,cz, cx+1,cy,cz);
			ncount++;
		}
		if (cy>0 && hl[c-vh_in.scn.xysize[0]]>=0)
		{
			neighbor[ncount] = c-vh_in.scn.xysize[0];
			kappa[ncount] = vh_in.scn.num_of_bits==8?
				DirecAff(in_data_8[c], in_data_8[neighbor[ncount]],
				y_adjacency, cx,cy,cz, cx,cy-1,cz):
				DirecAff(in_data_16[c], in_data_16[neighbor[ncount]],
				y_adjacency, cx,cy,cz, cx,cy-1,cz);
			ncount++;
		}
		if (cy<vh_in.scn.xysize[1]-1 && hl[c+vh_in.scn.xysize[0]]>=0)
		{
			neighbor[ncount] = c+vh_in.scn.xysize[0];
			kappa[ncount] = vh_in.scn.num_of_bits==8?
				DirecAff(in_data_8[c], in_data_8[neighbor[ncount]],
				y_adjacency, cx,cy,cz, cx,cy+1,cz):
				DirecAff(in_data_16[c], in_data_16[neighbor[ncount]],
				y_adjacency, cx,cy,cz, cx,cy+1,cz);
			ncount++;
		}
		if (cz>0 && hl[c-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]]>=0)
		{
			neighbor[ncount] = c-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1];
			kappa[ncount] = vh_in.scn.num_of_bits==8? DirecAff(in_data_8[c],
				in_data_8[neighbor[ncount]], z_adjacency, cx,cy,cz,cx,cy,cz-1):
				DirecAff(in_data_16[c], in_data_16[neighbor[ncount]],
				z_adjacency, cx,cy,cz, cx,cy,cz-1);
			ncount++;
		}
		if (cz<slices_out-1 &&
				hl[c+(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]]>=0)
		{
			neighbor[ncount] = c+(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1];
			kappa[ncount] = vh_in.scn.num_of_bits==8? DirecAff(in_data_8[c],
				in_data_8[neighbor[ncount]], z_adjacency, cx,cy,cz,cx,cy,cz+1):
				DirecAff(in_data_16[c], in_data_16[neighbor[ncount]],
				z_adjacency, cx,cy,cz, cx,cy,cz+1);
			ncount++;
		}
		ch = hl[c] & ~1;
		hl[c] -= MAX_CONNECTIVITY*2+4;
		for (j=0; j<ncount; j++)
		{
			d = neighbor[j];
			// step 6.
			dh = hl[d] & ~1;
			minh = ch<kappa[j]*2+2? ch:
			          kappa[j]*2+2;
			if (dh<minh || (dh==minh && (hl[R[d]]&1)==0 && (hl[R[c]]&1)))
			{
				int hld;

				hld = minh+(hl[c]&1);
				// step 8.
				R[d] = R[c]; Pr[d] = c;
				// steps 7, 9 & 10.
				UpdateGQueue(&Q, d, hld);
			// step 11.
			}
		// step 12.
		}
	// step 13.
	}
	// step 14.
	for (c=0; c<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; c++)
	{
		hl[c] += MAX_CONNECTIVITY*2+4;
		if ((hl[R[c]]&1) == 0)
			out_data[c] = (OutCellType)(((hl[c]&~1)-2)/2);
		else
			out_data[c] = 0;
	}
	free(hl);
	free(R);
	free(Pr);
	DestroyGQueue(&Q);
}

void MOFS(int current_volume)
{
	GQueue *H;
	int *sgch;
	int slices_out=vh_out.scn.num_of_subscenes[vh_in.scn.dimension==3? 0:
		1+current_volume];
	int c, j, w, x, ii;
	float x_adjacency, y_adjacency, z_adjacency;

	if ((double)slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1] > INT_MAX)
	{
		fprintf(stderr, "Scene is too large for data structure.\n");
		exit(-1);
	}
	x_adjacency = y_adjacency = z_adjacency = MAX_CONNECTIVITY;
	if (fuzzy_adjacency_flag < 0)
		z_adjacency = 0;
	else if (fuzzy_adjacency_flag)
	{
		if (vh_in.scn.xypixsz[0]<=vh_in.scn.xypixsz[1] &&
				vh_in.scn.xypixsz[0]<=slice_spacing)
		{
			y_adjacency *= vh_in.scn.xypixsz[0]/vh_in.scn.xypixsz[1];
			z_adjacency *= vh_in.scn.xypixsz[0]/slice_spacing;
		}
		else if (vh_in.scn.xypixsz[1]<=vh_in.scn.xypixsz[0] &&
				vh_in.scn.xypixsz[1]<=slice_spacing)
		{
			x_adjacency *= vh_in.scn.xypixsz[1]/vh_in.scn.xypixsz[0];
			z_adjacency *= vh_in.scn.xypixsz[1]/slice_spacing;
		}
		else
		{
			x_adjacency *= slice_spacing/vh_in.scn.xypixsz[0];
			y_adjacency *= slice_spacing/vh_in.scn.xypixsz[1];
		}
	}
	sgch = (int *)malloc(
	    slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*sizeof(int));
	if (sgch == NULL)
		handle_error(1);
	H = CreateGQueue(MAX_CONNECTIVITY*4+9,
		slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1], sgch);
	if (H == NULL)
		exit(-1);
	SetRemovalPolicy(H, MAXVALUE);

#define AssignKappa(a, b, adjacency, ax, ay, az, bx, by, bz) ( \
        kappa[0][ncount] = affinity(a, b, adjacency, ax,ay,az, bx,by,bz, 0), \
		kappa[1][ncount] = affinity(a, b, adjacency, ax,ay,az, bx,by,bz, 1) )

	// steps 1-3
	for (c=0; c<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; c++)
	{
		sgch[c] = 0;
	}
	// steps 4-7
	for (j=0; j<num_points_picked; j++)
	{
		c = point_picked[j][0]+vh_in.scn.xysize[0]*
			(point_picked[j][1]+vh_in.scn.xysize[1]*point_picked[j][2]);
		sgch[c] = 5+4*MAX_CONNECTIVITY;
	}
	for (j=0; j<num_bg_points; j++)
	{
		c = bg_point[j][0]+vh_in.scn.xysize[0]*
			(bg_point[j][1]+vh_in.scn.xysize[1]*bg_point[j][2]);
		sgch[c] = 6+4*MAX_CONNECTIVITY;
	}
	if (points_filename)
	{
		FILE *fp_pts;
		char group[5], element[5];
		unsigned char *in_points_data;
		Voxel cur;
		int counter;

		fp_pts = fopen(points_filename, "rb");
		if (fp_pts == NULL)
			handle_error(4);
		handle_error(VReadHeader(fp_pts, &vh_pts, group, element));
		in_points_data = (unsigned char *)
			malloc((vh_pts.scn.xysize[0]*vh_pts.scn.xysize[1]+7)/8);
		handle_error(VSeekData(fp_pts, 0));
		for (cur.z=0; cur.z<vh_pts.scn.num_of_subscenes[0]; cur.z++)
		{
			handle_error(VReadData((char *)in_points_data, 1,
				(vh_pts.scn.xysize[0]*vh_pts.scn.xysize[1]+7)/8, fp_pts, &j));
			counter = 0;
			j = 128;
			for (cur.y=0; cur.y<vh_pts.scn.xysize[1]; cur.y++)
				for (cur.x=0; cur.x<vh_pts.scn.xysize[0]; cur.x++)
				{
					if (in_points_data[counter] & j)
	    			{
						c = cur.x+vh_in.scn.xysize[0]*
							(cur.y+vh_in.scn.xysize[1]*cur.z);
						sgch[c] = 5+4*MAX_CONNECTIVITY;
					}
					j >>= 1;
					if (j == 0)
					{
						j = 128;
						counter++;
					}
				}
		}
		free(in_points_data);
		fclose(fp_pts);
	}
	if (bg_filename)
	{
		FILE *fp_pts;
		char group[5], element[5];
		unsigned char *in_points_data;
		Voxel cur;
		int counter;

		fp_pts = fopen(bg_filename, "rb");
		if (fp_pts == NULL)
			handle_error(4);
		handle_error(VReadHeader(fp_pts, &vh_bg_pts, group, element));
		in_points_data = (unsigned char *)
			malloc((vh_bg_pts.scn.xysize[0]*vh_bg_pts.scn.xysize[1]+7)/8);
		handle_error(VSeekData(fp_pts, 0));
		for (cur.z=0; cur.z<vh_bg_pts.scn.num_of_subscenes[0]; cur.z++)
		{
			handle_error(VReadData((char *)in_points_data, 1,
				(vh_bg_pts.scn.xysize[0]*vh_bg_pts.scn.xysize[1]+7)/8,
				fp_pts, &j));
			counter = 0;
			j = 128;
			for (cur.y=0; cur.y<vh_bg_pts.scn.xysize[1]; cur.y++)
				for (cur.x=0; cur.x<vh_bg_pts.scn.xysize[0]; cur.x++)
				{
					if (in_points_data[counter] & j)
	    			{
						c = cur.x+vh_in.scn.xysize[0]*
							(cur.y+vh_in.scn.xysize[1]*cur.z);
						sgch[c] = 6+4*MAX_CONNECTIVITY;
					}
					j >>= 1;
					if (j == 0)
					{
						j = 128;
						counter++;
					}
				}
		}
		free(in_points_data);
		fclose(fp_pts);
	}
	// step 8
	for (c=0; c<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; c++)
		InsertGQueue(&H, c);
	// step 9
	while (!EmptyGQueue(H))
	{
		int wx, wy, wz, neighbor[6], kappa[2][6], ncount=0;
		int sgw, sgx, sgp;
		// step 10
		w = RemoveGQueue(H);
		wz = w/((int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]);
		wy= (w-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*wz)/
			vh_in.scn.xysize[0];
		wx = w-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*wz-
			vh_in.scn.xysize[0]*wy;
		// step 11
		if (wx>0 && sgch[w-1]>=0)
		{
			neighbor[ncount] = w-1;
			if (vh_in.scn.num_of_bits==8)
				AssignKappa(in_data_8[w], in_data_8[w-1], x_adjacency,
				wx,wy,wz, wx-1,wy,wz);
			else
				AssignKappa(in_data_16[w],in_data_16[w-1], x_adjacency,
				wx,wy,wz, wx-1,wy,wz);
			ncount++;
		}
		if (wx<vh_in.scn.xysize[0]-1 && sgch[w+1]>=0)
		{
			neighbor[ncount] = w+1;
			if (vh_in.scn.num_of_bits==8)
				AssignKappa(in_data_8[w], in_data_8[w+1], x_adjacency,
				wx,wy,wz, wx+1,wy,wz);
			else
				AssignKappa(in_data_16[w],in_data_16[w+1], x_adjacency,
				wx,wy,wz, wx+1,wy,wz);
			ncount++;
		}
		if (wy>0 && sgch[w-vh_in.scn.xysize[0]]>=0)
		{
			neighbor[ncount] = w-vh_in.scn.xysize[0];
			if (vh_in.scn.num_of_bits==8)
				AssignKappa(in_data_8[w], in_data_8[neighbor[ncount]],
				y_adjacency, wx,wy,wz, wx,wy-1,wz);
			else
				AssignKappa(in_data_16[w], in_data_16[neighbor[ncount]],
				y_adjacency, wx,wy,wz, wx,wy-1,wz);
			ncount++;
		}
		if (wy<vh_in.scn.xysize[1]-1 && sgch[w+vh_in.scn.xysize[0]]>=0)
		{
			neighbor[ncount] = w+vh_in.scn.xysize[0];
			if (vh_in.scn.num_of_bits==8)
				AssignKappa(in_data_8[w], in_data_8[neighbor[ncount]],
				y_adjacency, wx,wy,wz, wx,wy+1,wz);
			else
				AssignKappa(in_data_16[w], in_data_16[neighbor[ncount]],
				y_adjacency, wx,wy,wz, wx,wy+1,wz);
			ncount++;
		}
		if (wz>0 && sgch[w-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]]>=0)
		{
			neighbor[ncount] = w-(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1];
			if (vh_in.scn.num_of_bits==8) AssignKappa(in_data_8[w],
				in_data_8[neighbor[ncount]], z_adjacency, wx,wy,wz,wx,wy,wz-1);
			else
				AssignKappa(in_data_16[w], in_data_16[neighbor[ncount]],
				z_adjacency, wx,wy,wz, wx,wy,wz-1);
			ncount++;
		}
		if (wz<slices_out-1 &&
				sgch[w+(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1]]>=0)
		{
			neighbor[ncount] = w+(int)vh_in.scn.xysize[0]*vh_in.scn.xysize[1];
			if (vh_in.scn.num_of_bits==8) AssignKappa(in_data_8[w],
				in_data_8[neighbor[ncount]], z_adjacency, wx,wy,wz,wx,wy,wz+1);
			else
				AssignKappa(in_data_16[w], in_data_16[neighbor[ncount]],
				z_adjacency, wx,wy,wz, wx,wy,wz+1);
			ncount++;
		}
		sgw = sgch[w] & ~3;
		sgch[w] -= MAX_CONNECTIVITY*4+8;
		for (j=0; j<ncount; j++)
		{
			x = neighbor[j];
			sgx = sgch[x] & ~3;
			if (sgx < 0)
				sgx += MAX_CONNECTIVITY*4+8;
			for (ii=0; ii<2; ii++)
			{
				if ((sgch[w] & (1<<ii)) == 0)
					continue;
				sgp = sgw<kappa[ii][j]*4? sgw:
				          kappa[ii][j]*4;
				if (sgp > sgx)
				{
					int sgchx;

					sgchx = sgp+(1<<ii);
					if (sgch[x] < 0)
						sgch[x] = sgchx-(MAX_CONNECTIVITY*4+8);
					else
					{
						UpdateGQueue(&H, x, sgchx);
						assert(sgch[x] == sgchx);
					}
				}
				else if (sgp==sgx && sgp>0 && ((sgch[x] & (1<<ii)) == 0))
				{
					if (sgch[x] < 0)
					{
						sgch[x] += MAX_CONNECTIVITY*4+8;
						sgch[x] |= 1<<ii;
						InsertGQueue(&H, x);
					}
					else
						UpdateGQueue(&H, x, sgch[x]|(1<<ii));
				}
			}
		}
	}
	for (w=0; w<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; w++)
	{
		if ((sgch[w]&3) == 1)
		{
			sgch[w] += MAX_CONNECTIVITY*4+8;
			out_data[w] = (OutCellType)(((sgch[w]&~3)-4)/4);
		}
		else
			out_data[w] = 0;
	}
	free(sgch);
	DestroyGQueue(&H);
}
