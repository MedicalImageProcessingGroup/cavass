/*
  Copyright 1993-2013, 2016-2017 Medical Image Processing Group
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
#include <Viewnix.h>   /* include files for 3DVIEWNIX */
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#else
	#define unlink(fname) _unlink(fname)
#endif

#include "render/AtoM.cpp"
#include "render/matrix.cpp"
#include "graph.h"

#define NUM_FEATURES 7
#define MULTITISSUE 8
#define MAX_CONNECTIVITY 65534
#define AFF_UNDEF 65535

#define BRIGHTEST 1
#define DARKEST -1

#ifndef MAX
#define MAX(x,y) (((x) > (y))?(x):(y))
#endif

typedef struct {
  short x, y, z;
} Voxel;

typedef struct {
  int xdim, ydim, zdim;
  long slice_size, volume_size;
  double voxelsize_x, voxelsize_y, voxelsize_z;
} S_dimensions;

extern "C" {
	int VAddBackgroundProcessInformation(char[]);
	int VReadHeader(FILE *, ViewnixHeader *, char[], char[]);
	int VWriteHeader(FILE *, ViewnixHeader *, char[], char[]);
	int VWriteData(unsigned char *, int, int, FILE *, int *);
	int VCloseData(FILE *);
	int VDeleteBackgroundProcessInformation();
	int VDecodeError(char[], const char[], int, char[]);
	int VSeekData(FILE *, int);
	int VReadData(unsigned char *, int, int, FILE *, int *);
}

void load_volume(int), load_fom(), load_dfom(), load_feature_map();
int affinity(int a, int b, float adjacency, int ax, int ay, int az,
	int bx, int by, int bz);
int fom_value(int col, int row, int slc);
int dfom_value(int col, int row, int slc);
int input_slice_index(int volume, int slice);
void parse_command_line(int argc, char *argv[]), handle_error(int);
int get_tissues(char tf[]);

static char *command_line(int argc, char **argv);

S_dimensions dimensions;
Voxel nbor[6] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },  { -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 } };


FileInfo file_info;
char *output_filename, *input_filename, *argv0, *histogram_filename,
	*points_filename, *fom_filename, *bg_filename, *feature_map_filename,
	*threeD_hist_filename, *dfom_filename;
ViewnixHeader vh_in, vh_out, vh_fom, vh_pts, vh_bg_pts;
int function_selected[NUM_FEATURES+1], feature_status[NUM_FEATURES+2], bg_flag;
float weight[NUM_FEATURES+2]={1, 1, 1, 1, 1, 1, 1, 1},
    function_level[NUM_FEATURES+1], function_width[NUM_FEATURES+1],
	count_affinity /* affinity per count on scale of 1 */,
	*tissue_level, *tissue_width;
int mask_original,
	affinity_type /* 0=parametric, 1=histogram, 2=covariance, 3=model based,
		4=dual object */,
	histogram_bins[2], *histogram_counts, *reverse_histogram_counts,
	largest_density_value, covariance_flag, reverse_covariance_flag,
	num_tissues, *tissue_type;
void *in_data;
#define in_data_8 ((unsigned char *)in_data)
#define in_data_16 ((unsigned short *)in_data)
unsigned char *out_data;
double training_mean[NUM_FEATURES], reverse_training_mean[NUM_FEATURES],
	inv_covariance[NUM_FEATURES*NUM_FEATURES],
	inv_reverse_covariance[NUM_FEATURES*NUM_FEATURES];
int fuzzy_adjacency_flag;
float slice_spacing, weight_unit, i_weight_unit;
double rel_scale, translation[3];
double centroid[3];
unsigned short *fom_data, *dfom_data;
double rotation[3][3];
int rotate_flag, prescaled;
int ioffset[3];
static unsigned short *feature_map_buf, *feature_map[0x1000], feature_map_max;
static unsigned short gray_map[0x10000];
static int *hist_bin_table[3], bins_per_feature;
static unsigned short ***threeD_hist;
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
 *
 *****************************************************************************/
int main(int argc, char *argv[])
{
	int error_code, j, k, slices_out, volumes_out, current_volume, max_count;
	char group[5], element[5];
	FILE *fp;
	long offset;
	Graph::node_id *nodes;

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
		if ((int)fread(histogram_counts, sizeof(int),
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
			if ((int)fread(reverse_histogram_counts, sizeof(int),
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
		vh_out.scn.num_of_bits = 1;
		vh_out.scn.bit_fields[1] = vh_out.scn.num_of_bits-1;
		vh_out.scn.smallest_density_value = (float *)malloc(sizeof(float));
		vh_out.scn.largest_density_value = (float *)malloc(sizeof(float));
		if (vh_out.scn.smallest_density_value==NULL ||
				vh_out.scn.largest_density_value==NULL)
			handle_error(1);
		vh_out.scn.smallest_density_value[0] = 0;
		vh_out.scn.largest_density_value[0] = 1;
		vh_out.scn.smallest_density_value_valid = TRUE;
		vh_out.scn.largest_density_value_valid = TRUE;
	}
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
	}
	vh_out.scn.description = command_line(argc, argv);
	vh_out.scn.description_valid = 1;
	fp = fopen(output_filename, "w+b");
	if (fp == NULL)
		handle_error(4);
	handle_error(VWriteHeader(fp, &vh_out, group, element));
	in_data = malloc(slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
		vh_in.scn.num_of_bits/8);
	out_data =
	    (unsigned char *)malloc((vh_in.scn.xysize[0]*vh_in.scn.xysize[1]+7)/8);
	nodes =
		new Graph::node_id[slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]];
	if (in_data==NULL || out_data==NULL || nodes==NULL)
		handle_error(1);
	if (feature_status[6])
		load_fom();
	load_dfom();
	if (feature_status[7])
		load_feature_map();

	float x_adjacency, y_adjacency, z_adjacency;

	if ((double)slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1] > INT_MAX)
	{
		fprintf(stderr, "Scene is too large for data structure.\n");
		exit(-1);
	}
	x_adjacency = y_adjacency = z_adjacency = MAX_CONNECTIVITY;
	if (fuzzy_adjacency_flag)
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
	for (current_volume=0; current_volume<volumes_out; current_volume++)
	{
		load_volume(current_volume);
		Graph *grph = new Graph();
		for (j=0; j<slices_out*vh_in.scn.xysize[0]*vh_in.scn.xysize[1]; j++)
			nodes[j] = grph->add_node();
		int n;
		unsigned char *in_points_data, *bg_points_data;

		if (points_filename)
		{
			FILE *fp_pts;
			char group[5], element[5];

			fp_pts = fopen(points_filename, "rb");
			if (fp_pts == NULL)
				handle_error(4);
			handle_error(VReadHeader(fp_pts, &vh_pts, group, element));
			in_points_data = (unsigned char *)malloc((vh_pts.scn.xysize[0]*
				vh_pts.scn.xysize[1]+7)/8*vh_pts.scn.num_of_subscenes[0]);
			handle_error(VSeekData(fp_pts, 0));
			handle_error(VReadData(in_points_data, 1, (vh_pts.scn.xysize[0]*
				vh_pts.scn.xysize[1]+7)/8*vh_pts.scn.num_of_subscenes[0],
				fp_pts, &j));
			fclose(fp_pts);
		}
		if (bg_filename)
		{
			FILE *fp_pts;
			char group[5], element[5];

			fp_pts = fopen(bg_filename, "rb");
			if (fp_pts == NULL)
				handle_error(4);
			handle_error(VReadHeader(fp_pts, &vh_bg_pts, group, element));
			bg_points_data = (unsigned char *)malloc((vh_bg_pts.scn.xysize[0]*
			   vh_bg_pts.scn.xysize[1]+7)/8*vh_bg_pts.scn.num_of_subscenes[0]);
			handle_error(VSeekData(fp_pts, 0));
			handle_error(VReadData(bg_points_data, 1, (vh_bg_pts.scn.xysize[0]*
				vh_bg_pts.scn.xysize[1]+7)/8*vh_bg_pts.scn.num_of_subscenes[0],
				fp_pts, &j));
			fclose(fp_pts);
		}
		for (j=0; j<slices_out; j++)
		{
			n = 0;
			for (k=0; k<vh_in.scn.xysize[1]; k++)
			{
				for (int m=0; m<vh_in.scn.xysize[0]; m++,n++)
				{
					int fv, sweight, tweight;
					if (points_filename==NULL || bg_filename==NULL)
					{
						fv = fom_value(m, k, j);
						sweight = fv;
						tweight = MAX_CONNECTIVITY-fv;
					}
					else
						sweight = tweight = MAX_CONNECTIVITY/2;
					if (points_filename && in_points_data[(
					    (j*vh_pts.scn.xysize[0]*vh_pts.scn.xysize[1]+7)/8
						+(k*vh_pts.scn.xysize[0]+m)/8)] &
						(128>>((k*vh_pts.scn.xysize[0]+m)%8)))
					{
					   sweight = MAX_CONNECTIVITY;
					   tweight = 0;
					}
					else if (bg_filename && bg_points_data[(
						(j*vh_bg_pts.scn.xysize[0]*vh_bg_pts.scn.xysize[1]+7)/8
						+(k*vh_bg_pts.scn.xysize[0]+m)/8)] &
						(128>>((k*vh_bg_pts.scn.xysize[0]+m)%8)))
					{
					   tweight = MAX_CONNECTIVITY;
					   sweight = 0;
					}
					grph->set_tweights(nodes[vh_in.scn.xysize[0]*
						vh_in.scn.xysize[1]*j+n], sweight, tweight);
					int faff, baff, a, b;
					if (vh_in.scn.num_of_bits == 8)
					{
						a = in_data_8[
							vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n];
						if (m < vh_in.scn.xysize[0]-1)
						{
							b = in_data_8[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n+1];
							faff= affinity(a,b, x_adjacency, m,k,j, m+1,k,j);
							if (dfom_filename)
								baff=affinity(b,a,x_adjacency, m+1,k,j, m,k,j);
							else
								baff = faff;
							grph->add_edge(nodes[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n],
								nodes[vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
								j+n+1], faff, baff);
						}
						if (k < vh_in.scn.xysize[1]-1)
						{
							b = in_data_8[vh_in.scn.xysize[0]*
								vh_in.scn.xysize[1]*j+n+vh_in.scn.xysize[0]];
							faff= affinity(a,b, y_adjacency, m,k,j, m,k+1,j);
							if (dfom_filename)
								baff=affinity(b,a,y_adjacency, m,k+1,j, m,k,j);
							else
								baff = faff;
							grph->add_edge(nodes[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n],
								nodes[vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
								j+n+vh_in.scn.xysize[0]], faff, baff);
						}
						if (j < slices_out-1)
						{
							b = in_data_8[vh_in.scn.xysize[0]*
								vh_in.scn.xysize[1]*(j+1)+n];
							faff= affinity(a,b, z_adjacency, m,k,j, m,k,j+1);
							if (dfom_filename)
								baff=affinity(b,a,z_adjacency, m,k,j+1, m,k,j);
							else
								baff = faff;
							grph->add_edge(nodes[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n],
								nodes[vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
								(j+1)+n], faff, baff);
						}
					}
					else
					{
						a = in_data_16[
							vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n];
						if (m < vh_in.scn.xysize[0]-1)
						{
							b = in_data_16[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n+1];
							faff= affinity(a,b, x_adjacency, m,k,j, m+1,k,j);
							if (dfom_filename)
								baff=affinity(b,a,x_adjacency, m+1,k,j, m,k,j);
							else
								baff = faff;
							grph->add_edge(nodes[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n],
								nodes[vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
								j+n+1], faff, baff);
						}
						if (k < vh_in.scn.xysize[1]-1)
						{
							b = in_data_16[vh_in.scn.xysize[0]*
								vh_in.scn.xysize[1]*j+n+vh_in.scn.xysize[0]];
							faff= affinity(a,b, y_adjacency, m,k,j, m,k+1,j);
							if (dfom_filename)
								baff=affinity(b,a,y_adjacency, m,k+1,j, m,k,j);
							else
								baff = faff;
							grph->add_edge(nodes[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n],
								nodes[vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
								j+n+vh_in.scn.xysize[0]], faff, baff);
						}
						if (j < slices_out-1)
						{
							b = in_data_16[vh_in.scn.xysize[0]*
								vh_in.scn.xysize[1]*(j+1)+n];
							faff= affinity(a,b, z_adjacency, m,k,j, m,k,j+1);
							if (dfom_filename)
								baff=affinity(b,a,z_adjacency, m,k,j+1, m,k,j);
							else
								baff = faff;
							grph->add_edge(nodes[
								vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n],
								nodes[vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*
								(j+1)+n], faff, baff);
						}
					}
				}
			}
		}
		if (points_filename)
			free(in_points_data);
		if (bg_filename)
			free(bg_points_data);
		Graph::flowtype flow = grph->maxflow();
		fprintf(outstream, "flow = %f\n", flow);
		for (j=0; j<slices_out; j++)
		{
			n = 0;
			memset(out_data, 0, (vh_in.scn.xysize[0]*vh_in.scn.xysize[1]+7)/8);
			for (k=0; k<vh_in.scn.xysize[1]; k++)
				for (int m=0; m<vh_in.scn.xysize[0]; m++,n++)
				{
					if (grph->what_segment(nodes[
							vh_in.scn.xysize[0]*vh_in.scn.xysize[1]*j+n]) ==
							Graph::SOURCE)
						out_data[n/8] |= 128>>(n%8);
				}
			int nitems;
			handle_error(VWriteData(out_data, 1,
				(vh_in.scn.xysize[0]*vh_in.scn.xysize[1]+7)/8, fp, &nitems));
		}
		delete grph;
	}
	delete nodes;
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
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: The variables feature_status, function_level,
 *    function_width, weight, function_selected, file_info, vh_in,
 *    in_data, affinity_type, histogram_bins, histogram_counts,
 *    largest_density_value, count_affinity, reverse_histogram_counts,
 *    covariance_flag, reverse_covariance_flag, inv_covariance,
 *    inv_reverse_covariance, training_mean, reverse_training_mean,
 *    weight_unit, vh_fom, fom_data, slice_spacing, rel_scale, translation,
 *    i_weight_unit, dfom_data, dfom_filename
 *    must be properly set.
 * RETURN VALUE: the affinity scaled to MAX_CONNECTIVITY
 * EXIT CONDITIONS: On error, writes a message to stderr and exits with code 1.
 * HISTORY:
 *    Created: 4/2/96 by Dewey Odhner
 *    Modified: 5/21/96 histogram-type affinity allowed by Dewey Odhner
 *    Modified: 5/31/96 dual-histogram-type affinity allowed by Dewey Odhner
 *    Modified: 7/26/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 1/29/98 fuzzy adjacency passed by Dewey Odhner
 *
 *****************************************************************************/
int affinity(int a, int b, float adjacency, int ax, int ay, int az,
	int bx, int by, int bz)
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
			xrel = (feature_val-function_level[5])/
				function_width[5];
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
			xrel = (feature_val-function_level[5])/
				function_width[5];
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
			if (tmp_fom_val < 32767)
				tmp_fom_val = 65534-tmp_fom_val;
			if (tmp_fom_val2 < 32767)
				tmp_fom_val2 = 65534-tmp_fom_val2;
			temp_affinity += i_weight_unit*weight[6]*tmp_fom_val;
			temp_affinity2 += i_weight_unit*weight[6]*tmp_fom_val2;
		}
		if (feature_status[MULTITISSUE])
		{
			int j;

			temp_affinity3 = 0;
			for (j=0; j<num_tissues; j++)
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
			for (j=0; j<num_tissues; j++)
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
	if (dfom_filename && a!=b)
	{
		int dv=dfom_value(ax, ay, az), dv2=dfom_value(bx, by, bz);
		if (a < b)
		{
			if ((65534-dv)*2 < temp_affinity)
				temp_affinity = (float)((65534-dv)*2);
			if ((65534-dv2)*2 < temp_affinity2)
				temp_affinity2 = (float)((65534-dv2)*2);
		}
		else
		{
			if (dv*2 < temp_affinity)
				temp_affinity = (float)(dv*2);
			if (dv2*2 < temp_affinity2)
				temp_affinity2 = (float)(dv2*2);
		}
	}
	if (temp_affinity2 < temp_affinity)
		temp_affinity = temp_affinity2;
	return (int)(temp_affinity*adjacency);
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
			handle_error(VReadData(in_data_8+slice*slice_bytes, cell_bytes,
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
		handle_error(VReadData((unsigned char *)&feature_map_max, 2,1, fp,&j));
		handle_error(VReadData((unsigned char *)feature_map_buf, 2, 0x800800,
			fp, &j));
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
	handle_error(VReadData((unsigned char *)&bins_per_feature, 4, 1, fp, &j));
	hist_bin_table[0] = (int *)malloc(bins_per_feature*sizeof(int));
	hist_bin_table[1] = (int *)malloc(bins_per_feature*sizeof(int));
	hist_bin_table[2] = (int *)malloc(bins_per_feature*sizeof(int));
	handle_error(VReadData((unsigned char *)hist_bin_table[0], 4,
		bins_per_feature, fp, &j));
	handle_error(VReadData((unsigned char *)hist_bin_table[1], 4,
		bins_per_feature, fp, &j));
	handle_error(VReadData((unsigned char *)hist_bin_table[2], 4,
		bins_per_feature, fp, &j));
	threeD_hist= (unsigned short ***)malloc(bins_per_feature*sizeof(short **));
	for (k=0; k<bins_per_feature; k++)
	{
		threeD_hist[k] =
		    (unsigned short **)malloc(bins_per_feature*sizeof(short *));
		for (m=0; m<bins_per_feature; m++)
		{
			threeD_hist[k][m] =
			    (unsigned short *)malloc(bins_per_feature*sizeof(short));
			handle_error(VReadData((unsigned char *)threeD_hist[k][m], 2,
				bins_per_feature, fp, &j));
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
	handle_error(VReadData((unsigned char *)fom_data, 2,
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
	handle_error(VReadData((unsigned char *)dfom_data, 2,
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
		if (fscanf(fp, "%f %f\n", tissue_level+j, tissue_width+j) != 2)
		{
			fprintf(stderr, "Failure reading %s\n", tf);
			return 1;
		}
	fscanf(fp, "%*d\n");
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
		    if (bg_flag)
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
 *    Modified: 2/9/99 tracking algorithm switch added by Dewey Odhner
 *
 *****************************************************************************/
void usage()
{
	fprintf(stderr, "Usage: %s <in> ", argv0); 
	fprintf(stderr, "[1 <ss> <se> <si> | 2 <ss> <se> <si> <vs> <ve> <vi>] ");
	fprintf(stderr, "<out> <bg> [-mask_original] ");
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
	fprintf(stderr, "[-fuzzy_adjacency] ");
	fprintf(stderr, "[-bg_points_file <bpf>] ");
	fprintf(stderr,"[[-feature_map_file | -3Dhist] <fmf>] [<pf>] [-o <os>]\n");

	fprintf(stderr, "<in>: input filename\n1: 3-dimensional scene\n");
	fprintf(stderr, "2: 4-dimensional scene\n<ss>: starting slice (from 0)\n");
	fprintf(stderr, "<se>: ending slice (from 0)\n<si>: slice increment\n");
	fprintf(stderr, "<vs>: starting volume (from 0)\n");
	fprintf(stderr, "<ve>: ending volume (from 0)\n<vi>: volume increment\n");
	fprintf(stderr, "<out>: output filename\n");
	fprintf(stderr, "<bg>: non-zero for background execution\n");
	fprintf(stderr, "<ft>: feature number:\n 0=high; 1=low; ");
	fprintf(stderr, "2=difference; 3=sum; 4=relative difference (0 to 100;");
	fprintf(stderr, " 0 to 65535 with covariance)\n");
	fprintf(stderr, "<fn>: function number: 0=gaussian; 1=ramp; 2=box\n");
	fprintf(stderr, "<wt>: weight (-1 to 1)\n");
	fprintf(stderr, "<tf>: tissue feature filename\n");
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
	fprintf(stderr, "<bpf>: background seed filename (binary scene)\n");
	fprintf(stderr, "<fmf>: feature map filename (unsigned short data)\n");
	fprintf(stderr, "<pf>: input points filename (binary scene)\n");
	fprintf(stderr, "<os>: output stream\n");
	exit(1);
}

/*****************************************************************************
 * FUNCTION: parse_command_line
 * DESCRIPTION: Initializes the variables input_filename, file_info,
 *    output_filename, bg_flag, feature_status, function_selected, weight,
 *    function_level, function_width,
 *    mask_original, training_mean, reverse_training_mean,
 *    inv_covariance, inv_reverse_covariance, covariance_flag,
 *    reverse_covariance_flag, fuzzy_adjacency_flag,
 *    weight_unit, i_weight_unit, ioffset, points_filename
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
 *    Modified: 2/9/99 tracking algorithm switch initialized by Dewey Odhner
 *
 *****************************************************************************/
void parse_command_line(int argc, char *argv[])
{
	int args_parsed, current_feature, inum, imin, imax, iinc, features_on, j;
	static short min[2], max[2], incr[2];
	double total_weight=0;

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
	if (sscanf(argv[args_parsed++], "%d", &bg_flag) != 1)
		usage();
	if (argc>args_parsed && strcmp(argv[args_parsed], "-mask_original")==0)
	{
		mask_original = TRUE;
		args_parsed++;
	}
	if (argc>args_parsed && strcmp(argv[args_parsed], "-multitissue")==0)
	{
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
	if (argc>args_parsed+1 && strcmp(argv[args_parsed], "-o")==0)
	{
		outstream = fopen(argv[args_parsed+1], "wb");
		args_parsed += 2;
	}
	else
		outstream = stdout;
	if (argc-args_parsed)
		usage();
}
