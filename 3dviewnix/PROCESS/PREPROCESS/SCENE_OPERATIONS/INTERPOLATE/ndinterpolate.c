/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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

/*
	This program performs interpolation along all directions of a scene (up to
	 4D).
	The input scene can have 1, 8 or 16 bits/pixel.

Author: Roberto J. Goncalves
Date  : 09/15/92
*/

#include <math.h>
#include <limits.h>
#include <assert.h>
#include <cv3dv.h>
#include "slices.c"

/*===========================================================================*/
#define MAX(a,b)		((a) > (b) ? a : b)
#define MIN(a,b)		((a) < (b) ? a : b)
/*===========================================================================*/

typedef struct Slice_buffer {
	float *data;
	float z_location, t_location;
	int volume_number, slice_number;
} Slice_buffer;

int distance_map(unsigned char *bin, int xsize, int ysize, float *out,
    int chamfer, float Dx, float Dy);
void abort_ndinterpolate(int code),
 dist_to_bin(float *distin, int w, int h, unsigned char *binout, float value);
void get_slice(float *out_data, FILE *fp, double offset, int in_size_x,
	    int in_size_y, int pixel_bits,
		float in_pixel_width, float in_pixel_height,
		int out_size_x, int out_size_y,
		float out_pixel_width, float out_pixel_height,
		int degree_x, int degree_y, int chamfer);

/* GLOBAL VARIABLES */
int execution_mode; /* 0=foreground, 1=background */


float MIN5(v,w,x,y,z)
float v,w,x,y,z;
{
	float a,b,c;

	a = (float)MIN(v,w);
	b = (float)MIN(x,y);
	c = MIN(a,b);
	return( (float)MIN(c,z) );
}
float MAX5(v,w,x,y,z)
float v,w,x,y,z;
{
	float a,b,c;

	a = (float)MAX(v,w);
	b = (float)MAX(x,y);
	c = MAX(a,b);
	return (float)( MAX(c,z) );
}


/*****************************************************************************
 * FUNCTION: which_volume
 * DESCRIPTION: Returns the volume number of a volume near the specified
 *    location.
 * PARAMETERS:
 *    t_location: Get a volume near this location.
 *    sample: Which nearby volume to get: t_location should be between
 *       sample 1 and sample 2.
 *    slice_info: Information about the scene.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The volume number from 0
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/13/96 by Dewey Odhner
 *
 *****************************************************************************/
int which_volume(t_location, sample, slice_info)
	float t_location;
	int sample;
	SLICES *slice_info;
{
	int j;

	for (j=slice_info->volumes-1; j>=0; j--)
		if (slice_info->location4[j] <= t_location)
			break;
	j += sample-1;
	if (j < 0)
		j = 0;
	if (j > slice_info->volumes-1)
		j = slice_info->volumes-1;
	return j;
}


/*****************************************************************************
 * FUNCTION: which_slice
 * DESCRIPTION: Returns the slice number of a slice near the specified
 *    location.
 * PARAMETERS:
 *    volume: The volume number from 0
 *    z_location: Get a slice near this location.
 *    sample: Which nearby slice to get: z_location should be between
 *       sample 1 and sample 2 or beyond slice range.
 *    slice_info: Information about the scene.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The slice number from 0
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/13/96 by Dewey Odhner
 *    Modified: 3/11/02 shifted for z_location beyond slice range
 *        by Dewey Odhner
 *
 *****************************************************************************/
int which_slice(volume, z_location, sample, slice_info)
	int volume;
	float z_location;
	int sample;
	SLICES *slice_info;
{
	int j;

	for (j=slice_info->slices[volume]-2; j>0; j--)
		if (slice_info->location3[volume][j] <= z_location)
			break;
	j += sample-1;
	if (j < 0)
		j = 0;
	if (j > slice_info->slices[volume]-1)
		j = slice_info->slices[volume]-1;
	return j;
}


/*****************************************************************************
 * FUNCTION: get_current_slices
 * DESCRIPTION: For each slice specified in slices_needed, reads a slice from
 *    the file and interpolates within the slice plane.
 * PARAMETERS:
 *    slices_needed: For each element, volume_number and slice_number must be
 *       specified; the other fields will be set by this function.  Duplicates
 *       are allowed and will share a data buffer.  The data buffer must not
 *       be freed by the caller.
 *    fp: The file to read the data from.
 *    slice_info: Information about the scene.
 *    out_size_x, out_size_y: Size of the output slice in pixels.
 *    out_pixel_width, out_pixel_height: Size of an output pixel.
 *    degree_x, degree_y: The degree of the interpolant, 0 1, or 3 in each
 *       direction.
 *    chamfer: Flag to use chamfer distance rather than city-block.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: Parameters fp, slice_info, out_size_x, out_size_y,
 *    out_pixel_width, out_pixel_height, degree_x, degree_y, chamfer
 *    must not change from call to call.
 *    The global variable execution_mode should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error issues a message and exits the process.
 * HISTORY:
 *    Created: 12/17/96 by Dewey Odhner
 *
 *****************************************************************************/
void get_current_slices(Slice_buffer slices_needed[16], FILE *fp,
        SLICES *slice_info, float in_pixel_width,
		float in_pixel_height, int out_size_x, int out_size_y,
		float out_pixel_width, float out_pixel_height,
		int degree_x, int degree_y, int chamfer)
{
	static Slice_buffer slice_list[16];
	static int header_length;
	static double slice_size;
	int error_code, j, k, m;

  	if (header_length == 0)
	{
		error_code = VGetHeaderLength(fp, &header_length);
		if (error_code)
			abort_ndinterpolate(error_code);
		slice_size = (double)(
			((long)slice_info->bits*slice_info->width*slice_info->height+7)/8);
	}
	for (j=0; j<16; j++)
	{
		/* Look for this slice already loaded */
		for (k=0; k<16; k++)
			if (slice_list[k].data &&
				slice_list[k].volume_number==slices_needed[j].volume_number &&
				slice_list[k].slice_number==slices_needed[j].slice_number)
			  break;
		if (k < 16)
		{
			slices_needed[j] = slice_list[k];
			continue;
		}

		/* Look for another slice to recycle */
		for (k=0; k<16; k++)
		{
			if (slice_list[k].data == NULL)
				continue;
			for (m=0; m<16; m++)
			{
				if (slices_needed[m].volume_number==
						slice_list[k].volume_number &&
						slices_needed[m].slice_number==
						slice_list[k].slice_number)
					break;
			}
			if (m == 16)
				break;
		}

		if (k == 16)
		/* Allocate a new buffer */
		{
			for (k=0; slice_list[k].data; k++)
				;
			slice_list[k].data =
				(float *)malloc(out_size_x*out_size_y*sizeof(float));
			if (slice_list[k].data == NULL)
				abort_ndinterpolate(1);
		}

		slice_list[k].volume_number = slices_needed[j].volume_number;
		slice_list[k].slice_number = slices_needed[j].slice_number;
		get_slice(slice_list[k].data, fp, header_length+
			slice_size*slice_info->slice_index[
			slice_list[k].volume_number][slice_list[k].slice_number],
			slice_info->width, slice_info->height, slice_info->bits,
			in_pixel_width, in_pixel_height, out_size_x, out_size_y,
			out_pixel_width, out_pixel_height, degree_x, degree_y, chamfer);
		slice_list[k].z_location = slice_info->location3[
			slice_list[k].volume_number][slice_list[k].slice_number];
		slice_list[k].t_location =
			slice_info->location4[slice_list[k].volume_number];
		slices_needed[j] = slice_list[k];
	}
}

/*-------------------------------------------------------------------------*/
/* Read the file header and its length */
/**********************************************************************
 *    Modified: 2/8/95 exit(0) removed by Dewey Odhner
 *    Modified: 12/13/96 len parameter removed by Dewey Odhner
 **********************************************************************/
int get_file_info(file, vh)
ViewnixHeader *vh;
char *file;	/* filename */
{
  	char group[5], element[5];
	FILE *fp;
	int error;
 
	/* open file */ 
  	if ((fp=fopen(file,"rb"))==NULL)
  	{
		char msg[200];

     	sprintf(msg,"ERROR: Can't open file [%s] !\n", file);
		VDeleteBackgroundProcessInformation();
		VPrintFatalError("get_file_info", msg, 0);
  	}
 
 
  	error = VReadHeader(fp, vh, group, element);
  	if( error>0 && error < 106)
  	{
		char msg[256];

		VDecodeError("ndinterpolate", "get_file_info", error, msg);
     	sprintf(msg+strlen(msg),"\ngroup=%s,  element=%s\n", group, element);
		VDeleteBackgroundProcessInformation();
		VPrintFatalError("get_file_info", msg, error);
  	}

	fclose(fp);
 
  	return(0);
 
}
 


/*****************************************************************************
 * FUNCTION: interpolate_1d
 * DESCRIPTION: Computes a polynomial interpolant of a single variable.
 * PARAMETERS:
 *    xi: The independent variable mapped to [0, 1] except in quadratic case
 *       mapped to [-alpha, beta].
 *    degree: The degree of the interpolant, 0 to 3.
 *    y0, y1, y2, y3: The sample function values at the following points:
 *       degree    (xi, y)
 *         0       (0, y0), (1, y1)
 *         1       (0, y0), (1, y1)
 *         2       (-alpha, y0), (0, y1), (beta, y2)
 *         3       (-alpha, y0), (0, y1), (1, y2), (1+beta, y3)
 *    alpha: quadratic case: x1-x0, cubic case: (x1-x0)/(x2-x1).
 *    beta: quadratic case: x2-x1, cubic case: (x3-x2)/(x2-x1).
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The interpolant value at xi.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/11/96 by Dewey Odhner
 *
 *****************************************************************************/
double interpolate_1d(xi, degree, y0, y1, y2, y3, alpha, beta)
	double xi;
	int degree;
	double y0, y1, y2, y3, alpha, beta;
{
	double a, b, c, d;

	assert(degree>=0 && degree<=3);
	switch (degree)
	{
		case 0:
			return (xi<.5? y0: y1);
		case 1:
			return (xi*y1+(1-xi)*y0);
		case 2:
			if (alpha==1 && beta==1)
				return .5*((y2-2*y1+y0)*xi+y2-y0)*xi+y1;
			else
			{
				assert(alpha*beta*(alpha+beta) != 0);
				d = 1/(alpha*beta*(alpha+beta));
				a = d*(alpha*(y2-y1)-beta*(y1-y0));
				b = d*(alpha*alpha*(y2-y1)+beta*beta*(y1-y0));
				c = y1;
				return (a*xi+b)*xi+c;
			}
		case 3:
			assert(xi>=0 && xi<=1);
			if (alpha==1 && beta==1)
				return .5*(((y3-3*y2+3*y1-y0)*xi+(-y3+4*y2-5*y1+2*y0))*xi+
					(y2-y0))*xi+y1;
			else
			{
				assert(alpha>0 && beta>0);
				a = (y3-y2)/(beta*(beta+1))+(y1-y0)*(1/(alpha*(alpha+1)))+
					(alpha/(alpha+1)+beta/(beta+1)-2)*(y2-y1);
				c = (alpha*alpha*(y2-y1)+(y1-y0))*(1/(alpha*(alpha+1)));
				b = y2-y1-a-c;
				d = y1;
				return ((a*xi+b)*xi+c)*xi+d;
			}
		default:
			abort_ndinterpolate(-1);
			exit(-1);
	}
}

/*****************************************************************************
 * FUNCTION: get_slice
 * DESCRIPTION: Reads a slice from the file and interpolates within the slice
 *    plane, giving float values.
 * PARAMETERS:
 *    out_data: Output goes here.
 *    fp: The file to read the data from.
 *    offset: Location of the slice in bytes from the beginning of the file.
 *    in_size_x, in_size_y: Size of the slice in the file in pixels.
 *    pixel_bits: Bits per pixel in the file: 1, 8, or 16.
 *    in_pixel_width, in_pixel_height: Size of an input pixel.
 *    out_size_x, out_size_y: Size of the output slice in pixels.
 *    out_pixel_width, out_pixel_height: Size of an output pixel.
 *    degree_x, degree_y: The degree of the interpolant, 0 1, or 3 in each
 *       direction.
 *    chamfer: Flag to use chamfer distance rather than city-block.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable execution_mode should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error issues a message and exits the process.
 * HISTORY:
 *    Created: 12/11/96 by Dewey Odhner
 *
 *****************************************************************************/
void get_slice(float *out_data, FILE *fp, double offset, int in_size_x,
	    int in_size_y, int pixel_bits,
		float in_pixel_width, float in_pixel_height,
		int out_size_x, int out_size_y,
		float out_pixel_width, float out_pixel_height,
		int degree_x, int degree_y, int chamfer)
{
	static unsigned char *in_data;
	static unsigned short *in_data_16;
	static int *in_pixel_x, *in_pixel_y;
	static float *frac_x, *frac_y;
	static int old_in_size_x, old_in_size_y, old_pixel_bits, old_out_size_x,
		old_out_size_y, in_slice_size;
	static float old_rel_pixel_width, old_rel_pixel_height;
	static float *in_float_data;
	int row, col, error_code, items_read;
	float rel_pixel_width, rel_pixel_height;

	switch (degree_x)
	{
		case 0:
		case 1:
		case 3:
			break;
		default:
			assert(FALSE);
	}
	switch (degree_y)
	{
		case 0:
		case 1:
		case 3:
			break;
		default:
			assert(FALSE);
	}
	rel_pixel_width = (float)(out_pixel_width/in_pixel_width);
	rel_pixel_height = (float)(out_pixel_height/in_pixel_height);
	if (in_size_x!=old_in_size_x || in_size_y!=old_in_size_y ||
			pixel_bits!=old_pixel_bits || out_size_x!=old_out_size_x ||
			out_size_y!=old_out_size_y ||
			rel_pixel_width!=old_rel_pixel_width ||
			rel_pixel_height!=old_rel_pixel_height)
	{
		if (old_pixel_bits)
		{
			if (old_pixel_bits == 16)
				free(in_data_16);
			else
				free(in_data);
			if (old_rel_pixel_width!=1 || old_rel_pixel_height!=1)
			{	free(in_pixel_x);
				free(frac_x);
				free(in_pixel_y);
				free(frac_y);
			}
			free(in_float_data);
			old_pixel_bits = 0;
		}
		if (in_size_x!=out_size_x || in_size_y!=out_size_y ||
				rel_pixel_width!=1 || rel_pixel_height!=1)
		{
			in_pixel_x = (int *)malloc(out_size_x*sizeof(int));
			frac_x = (float *)malloc(out_size_x*sizeof(float));
			in_pixel_y = (int *)malloc(out_size_y*sizeof(int));
			frac_y = (float *)malloc(out_size_y*sizeof(float));
			if (in_pixel_x==NULL || frac_x==NULL || in_pixel_y==NULL ||
					frac_y==NULL)
				abort_ndinterpolate(1);
			for (row=0; row<out_size_x; row++)
			{
				in_pixel_x[row] = (int)(row*rel_pixel_width);
				if (in_pixel_x[row] > in_size_x-2)
					in_pixel_x[row] = in_size_x-2;
				frac_x[row] = row*rel_pixel_width-in_pixel_x[row];
			}
			for (col=0; col<out_size_y; col++)
			{
				in_pixel_y[col] = (int)(col*rel_pixel_height);
				if (in_pixel_y[col] > in_size_y-2)
					in_pixel_y[col] = in_size_y-2;
				frac_y[col] = col*rel_pixel_height-in_pixel_y[col];
			}
		}
		in_slice_size = in_size_x*in_size_y;
		in_float_data = (float *)malloc(in_size_x*in_size_y*sizeof(float));
		if (in_float_data == NULL)
			abort_ndinterpolate(1);
		switch (pixel_bits)
		{
			case 1:
				in_data = (unsigned char *)malloc((in_slice_size+7)/8);
				if (in_data == NULL)
					abort_ndinterpolate(1);
				break;
			case 8:
				in_data = (unsigned char *)malloc(in_slice_size);
				if (in_data == NULL)
					abort_ndinterpolate(1);
				break;
			case 16:
				in_data_16 = (unsigned short *)malloc(in_slice_size*2);
				if (in_data_16 == NULL)
					abort_ndinterpolate(1);
				break;
			default:
				assert(FALSE);
		}
		old_in_size_x = in_size_x;
		old_in_size_y = in_size_y;
		old_pixel_bits = pixel_bits;
		old_out_size_x = out_size_x;
		old_out_size_y = out_size_y;
		old_rel_pixel_width = rel_pixel_width;
		old_rel_pixel_height = rel_pixel_height;
	}

	if (VLSeek(fp, offset))
		abort_ndinterpolate(5);
	switch (pixel_bits)
	{
		case 1:
			error_code =
				VReadData((char *)in_data, 1, (in_slice_size+7)/8, fp, &items_read);
			if (error_code)
				abort_ndinterpolate(error_code);
			error_code = distance_map(in_data, in_size_x, in_size_y,
				in_float_data, chamfer, in_pixel_width, in_pixel_height);
			if (error_code)
				abort_ndinterpolate(error_code);
			break;
		case 8:
			error_code = VReadData((char *)in_data, 1, in_slice_size, fp, &items_read);
			if (error_code)
				abort_ndinterpolate(error_code);
			for (row=0; row<in_size_y; row++)
				for (col=0; col<in_size_x; col++)
					in_float_data[in_size_x*row+col] =
						in_data[in_size_x*row+col];
			break;
		case 16:
			error_code =
				VReadData((char *)in_data_16, 2, in_slice_size, fp, &items_read);
			if (error_code)
				abort_ndinterpolate(error_code);
			for (row=0; row<in_size_y; row++)
				for (col=0; col<in_size_x; col++)
					in_float_data[in_size_x*row+col] =
						in_data_16[in_size_x*row+col];
			break;
	}
	if (in_size_x==out_size_x && in_size_y==out_size_y &&
			rel_pixel_width==1 && rel_pixel_height==1)
	{
		memcpy(out_data, in_float_data, in_slice_size*sizeof(float));
		return;
	}

	for (row=0; row<out_size_y; row++)
		for (col=0; col<out_size_x; col++)
			switch (degree_x)
			{	float f[4];

				case 0:
				case 1:
					switch (degree_y)
					{
						case 0:
						case 1:
							f[0] = (float)interpolate_1d(frac_x[col], degree_x,
								in_float_data[in_size_x*in_pixel_y[row]+
								in_pixel_x[col]],
								in_float_data[in_size_x*in_pixel_y[row]+
								in_pixel_x[col]+1], 0., 0., 0., 0.);
							f[1] = (float)interpolate_1d(frac_x[col], degree_x,
								in_float_data[in_size_x*(in_pixel_y[row]+1)+
								in_pixel_x[col]],
								in_float_data[in_size_x*(in_pixel_y[row]+1)+
								in_pixel_x[col]+1], 0., 0., 0., 0.);
							out_data[out_size_x*row+col] =
								(float)interpolate_1d(frac_y[row], degree_y,
								f[0], f[1], 0., 0., 0., 0.);
							break;
						case 3:
							if (in_pixel_y[row] == 0)
							{
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_pixel_x[col]],
									in_float_data[in_pixel_x[col]+1],
									0., 0., 0., 0.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x+in_pixel_x[col]],
									in_float_data[in_size_x+
									in_pixel_x[col]+1], 0., 0., 0., 0.);
								f[2] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*2+in_pixel_x[col]],
									in_float_data[in_size_x*2+
									in_pixel_x[col]+1], 0., 0., 0., 0.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row]-1, 2,
									f[0], f[1], f[2], 0., 1., 1.);
							}
							else if (in_pixel_y[row] == in_size_y-2)
							{
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1], 0., 0., 0., 0.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1], 0., 0., 0., 0.);
								f[2] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1], 0., 0., 0., 0.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 2,
									f[0], f[1], f[2], 0., 1., 1.);
							}
							else
							{
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1], 0., 0., 0., 0.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1], 0., 0., 0., 0.);
								f[2] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1], 0., 0., 0., 0.);
								f[3] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]+1], 0., 0., 0., 0.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 3,
									f[0], f[1], f[2], f[3], 1., 1.);
							}
							break;
					}
					break;
				case 3:
					switch (degree_y)
					{
						case 0:
						case 1:
							if (in_pixel_x[col] == 0)
							{
								f[0] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_size_x*in_pixel_y[row]],
									in_float_data[in_size_x*in_pixel_y[row]+1],
									in_float_data[in_size_x*in_pixel_y[row]+2],
									0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col]-1, 2,
									in_float_data[
									in_size_x*(in_pixel_y[row]+1)],
									in_float_data[
									in_size_x*(in_pixel_y[row]+1)+1],
									in_float_data[in_size_x*
									(in_pixel_y[row]+1)+2], 0., 1., 1.);
							}
							else if (in_pixel_x[col] == in_size_x-2)
							{
								f[0] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1], 0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1], 0., 1., 1.);
							}
							else
							{
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+2], 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+2], 1., 1.);
							}
							out_data[out_size_x*row+col] =
								(float)interpolate_1d(frac_y[row], degree_y,
								f[0], f[1], 0., 0., 0., 0.);
							break;
						case 3:
							if (in_pixel_x[col] == 0)
							{
							  if (in_pixel_y[row] == 0)
							  {
								f[0] = (float)interpolate_1d(frac_x[col]-1, 2,
									in_float_data[in_pixel_x[col]],
									in_float_data[in_pixel_x[col]+1],
									in_float_data[in_pixel_x[col]+2],
									0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col]-1, 2,
									in_float_data[in_size_x+in_pixel_x[col]],
									in_float_data[in_size_x+
									in_pixel_x[col]+1],
									in_float_data[in_size_x+
									in_pixel_x[col]+2], 0., 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col]-1, 2,
									in_float_data[in_size_x*2+in_pixel_x[col]],
									in_float_data[in_size_x*2+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*2+
									in_pixel_x[col]+2], 0., 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row]-1, 2,
									f[0], f[1], f[2], 0., 1., 1.);
							  }
							  else if (in_pixel_y[row] == in_size_y-2)
							  {
								f[0] = (float)interpolate_1d(frac_x[col]-1, 2,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+2], 0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col]-1, 2,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+2], 0., 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col]-1, 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+2], 0., 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 2,
									f[0], f[1], f[2], 0., 1., 1.);
							  }
							  else
							  {
								f[0] = (float)interpolate_1d(frac_x[col]-1, 2,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+2], 0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col]-1, 2,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+2], 0., 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col]-1, 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+2], 0., 1., 1.);
								f[3] = (float)interpolate_1d(frac_x[col]-1, 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]+2], 0., 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 3,
									f[0], f[1], f[2], f[3], 1., 1.);
							  }
							}
							else if (in_pixel_x[col] == in_size_x-2)
							{
							  if (in_pixel_y[row] == 0)
							  {
								f[0] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_pixel_x[col]-1],
									in_float_data[in_pixel_x[col]],
									in_float_data[in_pixel_x[col]+1],
									0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_size_x+
									in_pixel_x[col]-1],
									in_float_data[in_size_x+in_pixel_x[col]],
									in_float_data[in_size_x+
									in_pixel_x[col]+1], 0., 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_size_x*2+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*2+in_pixel_x[col]],
									in_float_data[in_size_x*2+
									in_pixel_x[col]+1], 0., 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row]-1, 2,
									f[0], f[1], f[2], 0., 1., 1.);
							  }
							  else if (in_pixel_y[row] == in_size_y-2)
							  {
								f[0] = (float)interpolate_1d(frac_x[col], 2,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1], 0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1], 0., 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col], 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1], 0., 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 2,
									f[0], f[1], f[2], 0., 1., 1.);
							  }
							  else
							  {
								f[0] = (float)interpolate_1d(frac_x[col], 2,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1], 0., 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], 2,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1], 0., 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col], 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1], 0., 1., 1.);
								f[3] = (float)interpolate_1d(frac_x[col], 2,
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]+1], 0., 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 3,
									f[0], f[1], f[2], f[3], 1., 1.);
							  }
							}
							else
							{
							  if (in_pixel_y[row] == 0)
							  {
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_pixel_x[col]-1],
									in_float_data[in_pixel_x[col]],
									in_float_data[in_pixel_x[col]+1],
									in_float_data[in_pixel_x[col]+2],
									1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x+
									in_pixel_x[col]-1],
									in_float_data[in_size_x+in_pixel_x[col]],
									in_float_data[in_size_x+
									in_pixel_x[col]+1],
									in_float_data[in_size_x+
									in_pixel_x[col]+2], 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*2+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*2+in_pixel_x[col]],
									in_float_data[in_size_x*2+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*2+
									in_pixel_x[col]+2], 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row]-1, 2,
									f[0], f[1], f[2], 0., 1., 1.);
							  }
							  else if (in_pixel_y[row] == in_size_y-2)
							  {
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+2], 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+2], 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+2], 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 2,
									f[0], f[1], f[2], 0., 1., 1.);
							  }
							  else
							  {
								f[0] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]-1)+
								   in_pixel_x[col]+2], 1., 1.);
								f[1] = (float)interpolate_1d(frac_x[col], degree_x,
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]-1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+1],
									in_float_data[in_size_x*in_pixel_y[row]+
									in_pixel_x[col]+2], 1., 1.);
								f[2] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+1)+
								   in_pixel_x[col]+2], 1., 1.);
								f[3] = (float)interpolate_1d(frac_x[col], degree_x,
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]-1],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]+1],
								   in_float_data[in_size_x*(in_pixel_y[row]+2)+
								   in_pixel_x[col]+2], 1., 1.);
								out_data[out_size_x*row+col] =
									(float)interpolate_1d(frac_y[row], 3,
									f[0], f[1], f[2], f[3], 1., 1.);
							  }
							}
							break;
					}
					break;
			}
}

/*****************************************************************************
 * FUNCTION: abort_ndinterpolate
 * DESCRIPTION: Issues a message and exits the process.
 * PARAMETERS:
 *    code: One of the 3DVIEWNIX error codes
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable execution_mode should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 12/5/96 by Dewey Odhner
 *
 *****************************************************************************/
void abort_ndinterpolate(int code)
{
	char msg[200];

	if(execution_mode == 1)
	{
		VDeleteBackgroundProcessInformation();
		system("job_done ndinterpolate -abort &");
	}
	VDecodeError("ndinterpolate", "abort_ndinterpolate", code, msg);
	fprintf(stderr, "%s\n", msg);
	exit(1);
}


/*-------------------------------------------------------------------------*/
/*    Modified: 8/11/95 outint buffer size corrected by Dewey Odhner */
/*  Modified: 12/1/95 non-uniform slice spacing accommodated by Dewey Odhner */
/*  Modified: 12/18/96 overhauled by Dewey Odhner */
/*  Modified: 1/8/97 default slice spacing set to pixel size by Dewey Odhner */
/* Modified: 1/9/97 default initial & final values corrected by Dewey Odhner */
/* Modified: 3/12/02 extrapolation in X_3 direction allowed by Dewey Odhner */
/* Modified: 8/23/02 extrapolation corrected for cubic by Dewey Odhner */
int main(argc, argv)
int argc;
char *argv[];
{
	char *comments;
	int i,j,k,m, t_nvolume, t_nslice;
	int jj, kk;
	ViewnixHeader vh;
	int sd;			/* scene dimension */
	int volumes;	/* number of volumes */
	int nvolumes;	/* number of volumes to use */
	int *slices;	/* number of slices in each volume */
	int *initial, *final; /* initial and final slices in each volume */
	int initial4, final4;	/* initial and final volumes */
	int nbits;		/* number of bits for cell density */
	int w,h; /* size of scene (width, height) in sampling points  */
	int nw,nh,nd,nt;	/* new values for w,h and d */
	int number_of_output_slices=0;
	float pixel[4];		/* pixels sizes along each direction */
	int method[4];	/* interpolation methods used for each directin */
	int bmethod; /* method for distance map calculation (binary input only) */
	int lbin_out; /* length of OUTPUT binary slice in bytes (8bit alignment) */
	float **nlocation;	/* New tree containing the slice locations of OUTPUT file */
	float *nlocation4;
	double minlocation, maxlocation;	/* FOV across volumes */
	float *dataint[4];	/* integer buffers for distance maps */
	float *finitial, *ffinal; /* initial and final slices in each volume */

	unsigned char *out1;		/* Output buffers */
	unsigned char *out8;
	unsigned short *out16;
	float *outdata;

	float	xsize,	/* size of voxel along each of the dimensions */
			ysize,
			nxsize,	/* new sizes of voxel along each of the dimensions */
			nysize,
			nzsize,
			ntsize,
			fovx,	/* field of view in each of the directions */
			fovy,
			fovt;

	FILE *fpin, *fpout;
  	char group[5], element[5];

	SLICES sl;

	char line[500];
	int number_of_lines;
	FILE *fparg;
	char input_file[500];

	int degree_z[4], degree_t; /* interpolants being used */
	int start_t;
	Slice_buffer slice_buffers[4][4];
	float alpha_z[4], alpha_t, beta_z[4], beta_t, v_value[5], zeta[4], tau;
	float slsp;




	if(argc < 9  &&  argc != 4)
	{
		printf("Usage:\n");
		printf("%% ndinterpolate input output mode p1 p2 p3 [p4] mb m1 m2 m3 [m4 Vi Vf] [I1 F1 ... In Fn]\n");
		printf("where:\n");
		printf("input	: name of the input file\n");
		printf("output 	: name of the ouput file\n");
		printf("mode 	: 0=foreground execution,  1=background execution\n");
		printf("p1		: pixel size along X1 (Ox) \n");
		printf("p2		: pixel size along X2 (Oy) \n");
		printf("p3		: pixel size along X3 (Oz) \n");
		printf("p4		: pixel size along X4 (Ot) \n");
		printf("mb		: method for distance map calculation (any value for grey scenes) : 0=city block, 1=chamfer\n");
		printf("m1		: method along X1 (Ox) : 0=nearest neighbor, 1=linear, 2=cubic\n");
		printf("m2		: method along X2 (Oy) : 0=nearest neighbor, 1=linear, 2=cubic\n");
		printf("m3		: method along X3 (Oz) : 0=nearest neighbor, 1=linear, 2=cubic\n");
		printf("m4		: method along X4 (Ot) : 0=nearest neighbor, 1=linear, 2=cubic\n");
		printf("Vi,Vf	: first and final volumes used for interpolation\n");
		printf("I1,F1	: on first volume interpolate between slices I1 and F1\n");
		printf("In,Fn	: on nth volume interpolate between slices In and Fn\n");
		printf("\n\n OR:\n");
		printf("%% ndinterpolate input output argument_file\n");
		printf("where:\n");
		printf("input	: name of the input file\n");
		printf("output 	: name of the ouput file\n");
		printf("argument_file: file containing the remaining arguments (one per line) as described above\n");
		exit(1);
	}


	
	/* Open INPUT and OUTPUT Files */
	strcpy(input_file, argv[1]);
	if( (fpin = fopen(argv[1], "rb")) == NULL)
	{
		printf("ERROR: Can't open INPUT file !\n");
		exit(1);
	}
	if( (fpout = fopen(argv[2], "wb")) == NULL)
	{
		printf("ERROR: Can't open OUTPUT file !\n");
		exit(1);
	}


	if(argc == 4)
	{

		if( (fparg = fopen(argv[3], "rb")) == NULL)
		{
			printf("ERROR: Can't open ARGUMENT file !\n");
			exit(1);
		}

		/* Check how many lines in the file (1 argument per line) */
		number_of_lines = 0;
		while( fgets((char *)line, 400, fparg) != NULL)
			number_of_lines ++;
		/* account for the first 4 arguments that were not on the argument file */
		number_of_lines += 4;
		argc = number_of_lines;
		if(argc<9)
		{
			printf("ERROR: argument file contains wrong number of arguments !\n");
			exit(1);
		}
		argv = (char **) malloc(number_of_lines * sizeof( char *));
		if(argv == NULL)
		{
			printf("ERROR: Can't allocate memory for arguments !\n");
			exit(1);
		}
		for(i=0; i<number_of_lines; i++)
		{
			argv[i] = (char *) malloc(50);
			if(argv[i] == NULL)
			{
				printf("ERROR: Can't allocate memory for arguments !\n");
				exit(1);
			}
		}

		/* reset the file pointer */
		fseek(fparg, 0, 0);
		i = 3;
		while( fgets(line, 400, fparg) != NULL)
		{
			strcpy(argv[i], line);
			i++;
		}

	}







	/* Get EXECUTION MODE */
	sscanf(argv[3], "%d", &execution_mode);
	
	if(execution_mode == 1)
	{
		VAddBackgroundProcessInformation("ndinterpolate");
	}

	/*-----------------------*/
	/* Read 3DViewnix header */
	/*-----------------------*/
	get_file_info(input_file, &vh);

	i = compute_slices(&vh, &sl);
	if (i)
		abort_ndinterpolate(i);
	
	sd = vh.scn.dimension;
	if(sd==3)
		volumes = 1;
	else
	if(sd==4)
		volumes = vh.scn.num_of_subscenes[0];
	else
	{
		fprintf(stderr,"ERROR: Can't interpolate more than 4 dimensions !\n");
		VDeleteBackgroundProcessInformation();
		exit(1);
	}

	slices = (int *) malloc( volumes * sizeof(int) );
	initial = (int *) malloc( volumes * sizeof(int) );
	final = (int *) malloc( volumes * sizeof(int) );
	finitial = (float *) malloc( volumes * sizeof(float) );
	ffinal = (float *) malloc( volumes * sizeof(float) );
	if (slices==NULL || initial==NULL || final==NULL || finitial==NULL ||
			ffinal==NULL)
		abort_ndinterpolate(1);
	

	/* COMMAND LINE */

	/* Parse the Pixel Sizes */
	pixel[3] = 0;
	j = 4;
	for(i=j; i<j+sd; i++)
	{
		sscanf(argv[i], "%f", &pixel[i-j]);
		if(i-j < 2  &&  pixel[i-j] == 0.0)  pixel[i-j] = vh.scn.xypixsz[i-j];
		if(i-j == 2  &&  pixel[i-j] == 0.0)  pixel[i-j] =
			vh.scn.xypixsz[0]; /* was sl.Min_spacing3 */
		if(i-j == 3  &&  pixel[i-j] == 0.0)  pixel[i-j] = sl.min_spacing4;

	}
	j=i;

	/* Parse Distance Map method (used for binary scenes only) */
	sscanf(argv[j], "%d", &bmethod);
	j++;

	/* Parse the Methodologies */
	method[3] = 0;
	for(i=j; i<j+sd; i++)
	{
		sscanf(argv[i], "%d", &method[i-j]);
		if (method[i-j] == 2)
			method[i-j] = 3;
	}
	j=i;

	/* Parse the 4D Initial and Final values (If 4D scene) */
	if(sd==4)
	{
		if(argc > j+1)
		{
			sscanf(argv[j], "%d", &initial4);
			sscanf(argv[j+1], "%d", &final4);
			j+=2;
		}
		else
		{
			initial4 = 0;
			final4 = sl.volumes-1;
		}
	}
	else
	{
		initial4 = 0;
		final4 = 0;
	}
	nvolumes = final4-initial4+1;

	/* Parse the Initial and Final slice of each volume */
	for(i=j, k=initial4; i<j+2*nvolumes; i+=2, k++)
	{
		/* If arguments are there in the command line */
		if(i<argc-1)
		{
			sscanf(argv[i], "%f",   &finitial[k]);
			sscanf(argv[i+1], "%f", &ffinal[k]);
			initial[k] = (int)ceil(finitial[k]);
			final[k] = (int)floor(ffinal[k]);
		}
		/* otherwise take the default values */
		else
		{
			finitial[k] = (float)(initial[k] = 0);
			ffinal[k] = (float)(final[k] = sl.slices[k] - 1);
		}
	}


	/* Check consistency of command line arguments (initial and final values)*/
	for(i=initial4; i<nvolumes; i++)
	{
		if(initial[i] < 0  ||  initial[i] >= sl.slices[i]  ||  
		   final[i] < initial[i] || final[i] < 0  || final[i] >= sl.slices[i])
		{
			fprintf(stderr,"ERROR: Inconsistent initial/final values on volume #%d\n", i);
			VDeleteBackgroundProcessInformation();
			exit(1);
		}
	}
 


	/* Get Information about the scene */
	nbits = sl.bits;
	w = sl.width;
	h = sl.height;






	/* Voxel Dimensions */
	xsize = vh.scn.xypixsz[0];
	ysize = vh.scn.xypixsz[1];


	/* New Voxel Dimensions (from command line) */
	nxsize = pixel[0];
	nysize = pixel[1];
	nzsize = pixel[2];
	ntsize = pixel[3];


	/* Field of View */
	fovx = (w-1) * xsize;
	fovy = (h-1) * ysize;
	minlocation = sl.location3[0][initial[initial4]];
	maxlocation = sl.location3[0][final[initial4]];
	if (finitial[initial4] != initial[initial4])
		minlocation += (sl.location3[0][initial[initial4]+1]-
			sl.location3[0][initial[initial4]])*
			(finitial[initial4]-initial[initial4]);
	if (ffinal[initial4] != final[initial4])
		maxlocation += (sl.location3[0][final[initial4]]-
			sl.location3[0][final[initial4]-1])*
			(ffinal[initial4]-final[initial4]);
	for (j=initial4+1; j<=final4; j++)
	{
		if (sl.location3[0][initial[j]] < minlocation)
			minlocation = sl.location3[0][initial[j]];
		if (sl.location3[0][final[j]] > maxlocation)
			maxlocation = sl.location3[0][final[j]];
		if (finitial[j] != initial[j])
			minlocation += (sl.location3[0][initial[j]+1]-
				sl.location3[0][initial[j]])*
				(finitial[j]-initial[j]);
		if (ffinal[j] != final[j])
			maxlocation += (sl.location3[0][final[j]]-
				sl.location3[0][final[j]-1])*
				(ffinal[j]-final[j]);
	}
	fovt = sl.location4[final4]-sl.location4[initial4];

	/* New #of samples on each dimension */
	nw = ((int)((fovx/nxsize)+0.5))+1;
	nh = ((int)((fovy/nysize)+0.5))+1;
	nd = (int)(maxlocation-minlocation==0? 1: (maxlocation-minlocation)/nzsize+1.5);
	nt = (int)(fovt==0? 1: fovt/ntsize+1.5);



	/* Build OUTPUT location tree */
	number_of_output_slices = 0;
	nlocation4 = (float *) calloc(1,  nt * sizeof(float) );
	nlocation = (float **) calloc(1,  nt * sizeof(float *) );
	if (nlocation4==NULL || nlocation==NULL)
		abort_ndinterpolate(1);
	for(i=0; i<nt; i++)
	{
		nlocation4[i] = sl.location4[initial4] + i*ntsize;
		/* #of slices on this specific volume */
		nlocation[i] = (float *) malloc( nd * sizeof(float));
		if (nlocation[i]==NULL)
			abort_ndinterpolate(1);
		for(j=0; j < nd; j++)
		{
			nlocation[i][j] = (float)(minlocation + j*nzsize);
			number_of_output_slices++;
		}
	}



	/*-------------------------*/
	/* Modify Viewnix.header */
	/*-------------------------*/
	strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1));
	vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
	strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename));
	vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
	vh.scn.xysize[0] = nw;
	vh.scn.xysize[1] = nh;
	vh.scn.xypixsz[0] = nxsize;
	vh.scn.xypixsz[1] = nysize;

	free(vh.scn.num_of_subscenes);
	free(vh.scn.loc_of_subscenes);
	/* 4D */
	if(sd==4)
	{
		vh.scn.num_of_subscenes = (short *)malloc( (nt+1) * sizeof(short) );
		if (vh.scn.num_of_subscenes == NULL)
			abort_ndinterpolate(1);
		vh.scn.num_of_subscenes[0] = nt;
		vh.scn.loc_of_subscenes =
			(float *)malloc((number_of_output_slices+nt)*sizeof(float));
		if (vh.scn.loc_of_subscenes == NULL)
			abort_ndinterpolate(1);
	}
	/* 3D */
	else
	{
		vh.scn.num_of_subscenes = (short *)malloc( sizeof(short) );
		if (vh.scn.num_of_subscenes == NULL)
			abort_ndinterpolate(1);
		vh.scn.num_of_subscenes[0] = nd;
		vh.scn.loc_of_subscenes = (float *) malloc( nd * sizeof(float));
		if (vh.scn.loc_of_subscenes == NULL)
			abort_ndinterpolate(1);
	}
    k = (sd==4) ? nt : 0;
	for(j=0; j<nt; j++)
	{
		/* For each 4D */
		if(sd==4)
		{
			vh.scn.loc_of_subscenes[j] = nlocation4[j];
			vh.scn.num_of_subscenes[j+1] = nd;
		}

		/* For each 3D */
		for(i=0; i<nd; i++) 
		{
			vh.scn.loc_of_subscenes[k] = (float) (i*nzsize + minlocation);
			k++;
		}
	}

	/* Build "description" header entry */

	for (i=k=0; i<argc; i++)
		k += (int)strlen(argv[i]);
	if( (comments = (char *) calloc(1, argc+k)) == NULL)
	{
		printf("ERROR: Can't allocate memory !\n");
		exit(3);
	}
	for(i=0; i<argc; i++)
	{
		strcat(comments,argv[i]);
		if (i < argc-1)
			strcat(comments," ");
	}
	vh.scn.description = comments;
	vh.scn.description_valid = 0x1;

	/*---------------------*/
	/* WRITE OUTPUT HEADER */
	/*---------------------*/
	VWriteHeader(fpout, &vh, group, element);




	/* START INTERPOLATION */
	/* BINARY */
	if(nbits == 1)
	{
		/* LENGTH OF BINARY IMAGES (Input and Output) */
		lbin_out = (nw*nh % 8 == 0) ? (nw*nh/8) : (nw*nh/8)+1;

		out1 = (unsigned char *) calloc(1, lbin_out);
		if (out1 == NULL)
			abort_ndinterpolate(1);
	}
	dataint[0] = (float *) calloc(1, nw*nh*sizeof(float));
	dataint[1] = (float *) calloc(1, nw*nh*sizeof(float));
	outdata = (float *) calloc(1, nw*nh*sizeof(float));
	if (dataint[0]==NULL || dataint[1]==NULL || outdata==NULL)
		abort_ndinterpolate(1);

	/* 8 BITS */
	if(nbits == 8)
	{
		out8 = (unsigned char *) calloc(1, nw*nh);
		if (out8 == NULL)
			abort_ndinterpolate(1);
	}

	/* 16 BITS */
	if(nbits == 16)
	{
		out16 = (unsigned short *) calloc(1, nw*nh*2);
		if (out16 == NULL)
			abort_ndinterpolate(1);
	}


	v_value[4] = 0;
	/*************************************************************************/
	/* Start building interpolated slices */
	/****************/
	/*==============*/
	/* ( Along Ot ) */
	/*==============*/
	/****************/
	for(t_nvolume=0; t_nvolume<nt; t_nvolume++)
	{

	  /****************/
	  /*==============*/
	  /* ( Along Oz ) */
	  /*==============*/
	  /****************/
	  for(t_nslice=0; t_nslice < nd; t_nslice++)
	  {
		if(execution_mode == 0)
		{
	  		if(sl.sd == 4)
				printf("Processing VOLUME:%d/%d,  SLICE:%d/%d ...   \n",
					t_nvolume+1, nt, t_nslice+1, nd);
			else
				printf("Processing SLICE:%d/%d ...   \n", t_nslice+1, nd);
			fflush(stdout);
		}

		for (j=0; j<4; j++)
		{
			jj = j;
			if (method[3] <= 1)
			{
				if (jj < 1)
					jj = 1;
				else if (jj > 2)
					jj = 2;
			}
			for (k=0; k<4; k++)
			{
				slice_buffers[j][k].volume_number =
					which_volume(nlocation4[t_nvolume], jj, &sl);
				kk = k;
				if (method[2] <= 1)
				{
					if (kk < 1)
						kk = 1;
					else if (kk > 2)
						kk = 2;
				}
				slice_buffers[j][k].slice_number =
					which_slice(slice_buffers[j][k].volume_number,
					nlocation[0][t_nslice], kk, &sl);
			}
		}
		get_current_slices(slice_buffers[0], fpin, &sl, xsize, ysize, nw, nh,
			nxsize, nysize, method[0], method[1], bmethod);
		for (j=0; j<4; j++)
		{
			assert(slice_buffers[j][2].slice_number >
				slice_buffers[j][0].slice_number);
			degree_z[j] = method[2];
			if (degree_z[j]>1 && slice_buffers[j][1].slice_number==
					slice_buffers[j][0].slice_number &&
					slice_buffers[j][2].slice_number==
					slice_buffers[j][3].slice_number)
				degree_z[j] = 1;
			slsp =
				slice_buffers[j][2].z_location-slice_buffers[j][1].z_location>0
				?	slice_buffers[j][2].z_location-
					slice_buffers[j][1].z_location
				:	slice_buffers[j][3].z_location-
					slice_buffers[j][2].z_location>0
					?	slice_buffers[j][3].z_location-
						slice_buffers[j][2].z_location
					:	slice_buffers[j][1].z_location-
						slice_buffers[j][0].z_location;
			zeta[j] = (nlocation[0][t_nslice]-slice_buffers[j][1].z_location)/
				slsp;
			if (degree_z[j]>2 && (slice_buffers[j][1].slice_number==
					slice_buffers[j][0].slice_number ||
					slice_buffers[j][2].slice_number==
					slice_buffers[j][3].slice_number))
			{
				degree_z[j] = 2;
				if (slice_buffers[j][2].slice_number ==
					slice_buffers[j][3].slice_number)
				{
					alpha_z[j] = (slice_buffers[j][1].z_location-
						slice_buffers[j][0].z_location)/slsp;
					beta_z[j] = 1;
				}
				else
				{
					alpha_z[j] = 1;
					beta_z[j] = (slice_buffers[j][3].z_location-
						slice_buffers[j][2].z_location)/slsp;
					zeta[j] -= 1;
				}
			}
			else
			{
				alpha_z[j] = (slice_buffers[j][1].z_location-
					slice_buffers[j][0].z_location)/slsp;
				beta_z[j] = (slice_buffers[j][3].z_location-
					slice_buffers[j][2].z_location)/slsp;
			}
		}
		start_t = slice_buffers[1][1].volume_number==
			slice_buffers[0][1].volume_number;
		degree_t = method[3];
		if (slice_buffers[1][1].volume_number ==
				slice_buffers[2][1].volume_number)
		{
			degree_t = 0;
			start_t = 1;
		}
		if (degree_t>1 && slice_buffers[1][1].volume_number==
				slice_buffers[0][1].volume_number &&
				slice_buffers[2][1].volume_number==
				slice_buffers[3][1].volume_number)
			degree_t = 1;
		if (degree_t>2 && (slice_buffers[1][1].volume_number==
				slice_buffers[0][1].volume_number ||
				slice_buffers[2][1].volume_number==
				slice_buffers[3][1].volume_number))
			degree_t = 2;
		tau = slice_buffers[2][1].t_location-slice_buffers[1][1].t_location>0
			  ?	(nlocation4[t_nvolume]-slice_buffers[1][1].t_location)/
				(slice_buffers[2][1].t_location-
									   slice_buffers[1][1].t_location)
			  : 0;
		if (degree_t >= 2)
		{
			alpha_t = (slice_buffers[1][1].t_location-
				slice_buffers[0][1].t_location)/(
				slice_buffers[2][1].t_location-
				slice_buffers[1][1].t_location);
			beta_t = (slice_buffers[3][1].t_location-
				slice_buffers[2][1].t_location)/(
				slice_buffers[2][1].t_location-
				slice_buffers[1][1].t_location);
			if (degree_t == 2)
			{
				if (start_t == 0)
				{
					beta_t = 1;
				}
				else
				{
					alpha_t = 1;
					tau -= 1;
				}
			}
		}



		/*--------------*/
		/* CREATE SLICE */
		/****************/
		/*==============*/
		/* ( Along Oy ) */
		/*==============*/
		/****************/
		for(j=0; j<nh; j++)
		{
			/****************/
			/*==============*/
			/* ( Along Ox ) */
			/*==============*/
			/****************/
			for(i=0; i<nw; i++)
			{
				for (m=start_t; m<=start_t+degree_t; m++)
				{
					switch (degree_z[m])
					{
						case 0:
							v_value[m] =
								slice_buffers[m][zeta[m]<.5? 1:2].data[j*nw+i];
							break;
						case 1:
							v_value[m] =
								(1-zeta[m])*slice_buffers[m][1].data[j*nw+i]+
									zeta[m]*slice_buffers[m][2].data[j*nw+i];
							break;
						case 2:
							v_value[m] = (float)interpolate_1d(zeta[m], degree_z[m],
								slice_buffers[m][0].data[j*nw+i],
								slice_buffers[m][
								(slice_buffers[m][1].slice_number==
								slice_buffers[m][0].slice_number? 2:1)
								].data[j*nw+i],
								slice_buffers[m][3].data[j*nw+i],
								0., alpha_z[m], beta_z[m]);
							break;
						case 3:
							v_value[m] = (float)interpolate_1d(zeta[m], degree_z[m],
								slice_buffers[m][0].data[j*nw+i],
								slice_buffers[m][1].data[j*nw+i],
								slice_buffers[m][2].data[j*nw+i],
								slice_buffers[m][3].data[j*nw+i],
								alpha_z[m], beta_z[m]);
							break;
					}
				}
				switch (degree_t)
				{
					case 0:
						outdata[j*nw+i] = v_value[tau<.5? 1:2];
						break;
					case 1:
						outdata[j*nw+i] = (1-tau)*v_value[1]+tau*v_value[2];
						break;
					case 2:
					case 3:
						outdata[j*nw+i] = (float)interpolate_1d(tau, degree_t,
							v_value[start_t], v_value[start_t+1],
							v_value[start_t+2], v_value[start_t+3],
							alpha_t, beta_t);
						break;
				}

			} /* Ox */
		} /* Oy */


	   /* Save slice into file */
	   if(nbits == 1)
	   {
			/* CONVERT BACK TO BINARY AND SAVE IT */
			dist_to_bin(outdata, nw, nh, out1, (float)0);

			if (fwrite(out1, lbin_out, 1, fpout) != 1)
				abort_ndinterpolate(3);
	   }
	   else
	   if(nbits == 8)
	   {
			for (kk=0; kk<nw*nh; kk++)
				out8[kk] = (unsigned char)(
					outdata[kk]<0? 0: outdata[kk]>255? 255:outdata[kk]);
			if (fwrite(out8, nw*nh, 1, fpout) != 1)
				abort_ndinterpolate(3);
	   }
	   else
	   if(nbits == 16)
	   {
			for (kk=0; kk<nw*nh; kk++)
				out16[kk] = (unsigned short)(
					outdata[kk]<0? 0: outdata[kk]>65535? 65535: outdata[kk]);
			kk = VWriteData((char *)out16, 2, nw*nh, fpout, &jj);
			if (kk)
				abort_ndinterpolate(kk);
	   }

	  } /* Oz */

	  /* calculate #of slices to skip for next volume */


	} /* for each new volume */
	/*************************************************************************/

	VCloseData(fpout);

	if(execution_mode == 1)
	{
		VDeleteBackgroundProcessInformation();
		sprintf(line, "job_done %s", argv[0]);
		system(line);
	}
	exit(0);

	
}


/*-----------------------------------------------------------------------------------------*/
/* Convert a float image into a binary */
/* Modified: 2/7/00 read overflow corrected by Dewey Odhner. */
void dist_to_bin(float *distin, int w, int h, unsigned char *binout,
    float value)
//float *distin;         /* input image */
//int w, h;              /* dimensions of image */
//unsigned char *binout; /* output binary image */
//float value;           /* value to threshold the input image */
{
	int mask[8];
	int l_8;
	float *in;
	unsigned char *out;
	register int i, j;
	int final_value;



	l_8 = w*h / 8;
	in = distin;
	out = binout;

	mask[7] = 1;
	for(i=6; i>=0; i--)
		mask[i] = mask[i+1] << 1;

	for(i=0; i < l_8; i++, out++)
	{
		final_value = 0;
		for(j=0; j < 8; j++, in++)
			if( *in > value) final_value += mask[j];

		*out = final_value;
	}
	if (w*h%8)
	{
		final_value = 0;
		for(j=0; j < w*h%8; j++, in++)
			if( *in > value) final_value += mask[j];

		*out = final_value;
	}
	

}


/*-----------------------------------------------------------------------------------------*/
/* Convert a binary image into an "int" */
/* Modified: 2/7/00 write overflow corrected by Dewey Odhner. */
void one2eight(binin, w, h, charout)
unsigned char *binin;
int w,h;
unsigned char *charout;
{
	int mask[8];
	register int i,j;
	unsigned char *tin, *tout;
	int l_8;

	tin = binin;
	tout = charout;

	mask[7] = 1;
	for(i=6; i>=0; i--)
		mask[i] = mask[i+1] << 1;

	l_8 = w*h/8;

	for(i=0; i<l_8; i++, tin++)
		for(j=0; j<8; j++, tout++)
			*tout = *tin & mask[j];
	for(j=0; j<w*h%8; j++, tout++)
		*tout = *tin & mask[j];
}



/*========================================================================= */
/*    Modified: 8/11/95 initialization corrected by Dewey Odhner */
/*    Modified: 12/17/96 float used for output by Dewey Odhner */
/*    Chamfer valid only for 1/3. <= Dx/Dy <= 3. */
int distance_map(unsigned char *bin, int xsize, int ysize, float *out,
    int chamfer, float Dx, float Dy)
{
	int	n;
	register int i, j, k;
	int border=2;
	int nrows, ncols;
	static float *slice;
	static unsigned char *bin8;
	static int old_xsize, old_ysize;
	float Dxy, much, halfx, halfy;


	nrows = ysize + border*2;
	ncols = xsize + border*2;

	n =nrows*ncols; 

	if (xsize!=old_xsize || ysize!=old_ysize || slice==NULL || bin8==NULL)
	{
		if (slice)
			free(slice);
		if (bin8)
			free(bin8);
		slice = NULL;
		bin8 = NULL;

		/* Allocate Memory for the DISTANCE MAP (Padded) */
		if( (slice = (float *) malloc(n * sizeof(float) )) == NULL)
		{
			printf("ERROR: Memory Allocation Error !\n");
			return(1);
		}

		/* Allocate Memory for the 8bit BINARY */
		if( (bin8 = (unsigned char *) malloc(xsize*ysize) ) == NULL)
		{
			free(slice);
			slice = NULL;
			printf("ERROR: Memory Allocation Error !\n");
			return(1);
		}
	}

	old_xsize = xsize;
	old_ysize = ysize;
	Dxy = (float)sqrt(Dx*Dx+Dy*Dy);
	much = (float)(ncols*Dx+nrows*Dy);
	halfx = (float)(Dx/2);
	halfy = (float)(Dy/2);

	/* Initialize DISTANCE MAP (Padded) */
	for (j=0; j<n; j++) 
		slice[j] = -much;	

	k = ncols*border+border;
	n = 0;
	/* CONVERT BINARY SLICE INTO an UNSIGNED CHAR SLICE */
	one2eight(bin, xsize, ysize, bin8);

	for (j=0; j<ysize; j++) 
	{
		for (i=0; i<xsize; i++)
		{   
			slice[k] = bin8[n];
			n++;
			k++;
		}
		k += 2*border;
	}   

/* initialization */
	for (j=1; j<nrows-1; j++) 
	{
		for (i=1, k=j*ncols+1; i<ncols-1; i++)
		{ 
			if (halfx < halfy)
				if (slice[k] <= 0)
					if (slice[k+1] > 0 || slice[k-1] > 0)
						slice[k] = -halfx;
					else if (slice[k+ncols] > 0 || slice[k-ncols] > 0) 
						slice[k] = -halfy;
					else 
						slice[k] = -much;
				else
					if (slice[k+1] <= 0 || slice[k-1] <= 0)
						slice[k] = halfx;
		    		else if (slice[k+ncols]<=0 || slice[k-ncols]<= 0) 
						slice[k] = halfy;
					else 
						slice [k] = much;
	   		else
				if (slice[k] <= 0)
					if (slice[k+ncols] > 0 || slice[k-ncols] > 0) 
						slice[k] = -halfy;
					else if (slice[k+1] > 0 || slice[k-1] > 0)
						slice[k] = -halfx;
					else 
						slice[k] = -much;
				else
					if (slice[k+ncols]<=0 || slice[k-ncols]<= 0) 
						slice[k] = halfy;
					else if (slice[k+1] <= 0 || slice[k-1] <= 0)
						slice[k] = halfx;
		    		else 
						slice[k] = much;
			k++;
		}
	}


/* forward pass */
	
	for (j=1; j<nrows-1; j++)
	   for (i=1; i<ncols-1; i++) 
	   {   
		 k = j*ncols+i;
		 if (chamfer)
	     {
		 	if (slice[k]>0 && slice[k]!=halfx && slice[k]!=halfy)
 	         	slice[k] = MIN5(slice[k-ncols-1]+Dxy, 
								slice[k-ncols]+Dy,
                    			slice[k-ncols+1]+Dxy, 
								slice[k-1]+Dx, 
								slice[k]);
         	else 
		 	if (slice[k]<0  && slice[k]!= -halfx && slice[k]!= -halfy)
             	slice[k] = MAX5(slice[k-ncols-1]-Dxy, 
								slice[k-ncols]-Dy,
                    			slice[k-ncols+1]-Dxy, 
								slice[k-1]-Dx, 
								slice[k]);
		 }
		 else
	     	if (slice[k]>0)
	        {
				if (slice[k-1]+Dx < slice[k])
					slice[k] = (float)(slice[k-1]+Dx);
				if (slice[k-ncols]+Dy < slice[k])
					slice[k] = (float)(slice[k-ncols]+Dy);
	        }
			else 
	        {
				if (slice[k-1]-Dx > slice[k])
					slice[k] = (float)(slice[k-1]-Dx);
				if (slice[k-ncols]-Dy > slice[k])
					slice[k] = (float)(slice[k-ncols]-Dy);
	        }
	   }

/* backward pass */

	for (j=nrows-2; j>0; j--)
	   for (i=ncols-2; i>0; i--)
	   {  
		  k = j*ncols + i;
		  if (chamfer)
	      {
		     if (slice[k]>0 && slice[k]!=halfx && slice[k]!=halfy) 
	            slice[k] = MIN5(slice[k], 
							    slice[k+1]+Dx, 
							    slice[k+ncols-1]+Dxy,
                    		    slice[k+ncols]+Dy, 
							    slice[k+ncols+1]+Dxy);
	         else 
		     if (slice[k]<0  && slice[k]!= -halfx && slice[k]!= -halfy)
	            slice[k] = MAX5(slice[k], 
							    slice[k+1]-Dx, 
							    slice[k+ncols-1]-Dxy,
                    		    slice[k+ncols]-Dy, 
							    slice[k+ncols+1]-Dxy);
		  }
		  else
	         if (slice[k]>0) 
	         {
			    if (slice[k+1]+Dx < slice[k])
					slice[k] = (float)(slice[k+1]+Dx);
				if (slice[k+ncols]+Dy < slice[k])
					slice[k] = (float)(slice[k+ncols]+Dy);
	         }
			 else 
	         {
			    if (slice[k+1]-Dx > slice[k])
					slice[k] = (float)(slice[k+1]-Dx);
				if (slice[k+ncols]-Dy > slice[k])
					slice[k] = (float)(slice[k+ncols]-Dy);
	         }
	   }
       
	k = ncols*border+border;
	n = 0;
	for (j=0; j<ysize; j++) 
	{  
		for (i=0; i<xsize; i++)
		{   
			out[n] = slice[k];
	       	n++;
	       	k++;
		}
		k += 2*border;
	}   

	return(0);
}
