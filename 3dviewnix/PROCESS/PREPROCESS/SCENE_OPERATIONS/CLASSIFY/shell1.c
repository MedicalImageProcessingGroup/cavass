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
#include <string.h>
#include <memory.h>
#include "manipulate.h"
#include "neighbors.h"
#include "shell.h"  

#define OUT_BUFFER_SIZE 0x8000
	/* in short-words */
#define SQRT2 1.414213562373095
#define SQRT3 1.732050807568877
typedef float data_value;
typedef struct opacity_struct {unsigned char gradient, opacity;
} opacity_struct;
#define ttype opacity
#define pct gradient

static void wrap_up(), next_slice(), hollow_voxel(), init_row(), next_row();
static char *command_line();

static char tempfile_name[16];

static ViewnixHeader file_header;
static Shell_data obj_data;
static float x_resample_ratio, y_resample_ratio, z_resample_ratio, voxel_size,
		slice_thickness, this_in_val, last_column_in_val, gx, gy, gz,
		next_slice_in_val, last_slice_in_val, next_row_in_val, last_row_in_val,
		next_column_in_val;
static FILE *dtfile, *tempfile;
/* Changed from int 6/29/95 by Dewey Odhner: */
static long *ptr_table, dummy_out_ptr, last_out_ptr, *this_ptr_ptr;
static int rows, columns, slices, this_row, this_column, this_slicen, volume,
		in_slice_size, max_x, max_y, max_z, min_x, min_y, min_z,
		dtrows, dtcolumns, dtslices, first_slice,
		max_opacity, min_opacity, nbits, data_pos;
static enum Classification_type method;
static unsigned short *in_slice1, *in_slice2, *in_slices;
static float *fin_slices, *fin_slice1, *fin_slice2, *in_rows, xr, yr, zr,
		min_density, max_density, min_gradient, max_gradient,
		gradient_cliff, density_cliff, threshold[6];
static data_value *this_slice_data, *next_slice_data, *last_slice_data,
		*this_slice_end, *this_in_ptr, *next_slice_in_ptr, *last_slice_in_ptr,
		*next_next_slice_in_ptr, *next_next_slice_data;
static opacity_struct *this_slice_opacity, *next_slice_opacity,
		*last_slice_opacity, *this_opacity_ptr, *next_slice_opacity_ptr,
		*last_slice_opacity_ptr;
static unsigned short *out_data, *buffer_out_ptr;
static unsigned char materl_opacity[8][256], *byte_in_slice;

main(argc,argv)
	int argc;
	char **argv;
{
	init_scene(argc, argv);
	printf("COMPUTATION in progress .  .  .\n");
	fflush(stdout);


	z_interpolate();
	wrap_up();
	vol_exit(0);
}

/* The following function interpolates along a row.  row is the row number.
 *	in_row2 points to the address of the array where the interpolated values will
 *	be stored.  This address will be stored where in_row1 points.  The address
 *	that was there is for the previously interpolated row.
 */
static void x_interpolate(row, in_row1)
	float *in_row1;
{
	float frcx, *outptr;
	unsigned short *inptr1, *inptr2;
	int i, new_column, intx, xlim;

	inptr1 = in_slice1+row*dtcolumns;
	inptr2 = inptr1+1;
	xlim = 1;
	frcx = intx = 0;
	outptr = in_row1;
	for (new_column=0; new_column<columns; new_column++)
	{	while (intx >= xlim)
		{	xlim++;
			if (xlim >= dtcolumns)
				inptr1 = inptr2;
			else
			{	inptr1++;
				inptr2++;
			}
		}
		*outptr = *inptr1*(1-frcx)+*inptr2*frcx;
		outptr++;
		frcx += x_resample_ratio;
		i = frcx;
		frcx -= i;
		intx += i;
	}
}

/* Interpolate between rows */
static void y_interpolate()
{
	int i, new_row, inty, ylim;
	float *in_row1, *in_row2, *inptr1, *inptr2, *outptr, frcy, *temp;

	if (in_rows == NULL)
	{	if ((in_rows=(float *)malloc(columns*sizeof(float)*2)) == NULL)
		{	puts("in_row1 memory allocation failure");
			vol_exit(1);
		}
	}
	in_row1 = in_rows;
	in_row2 = in_row1+columns;
	x_interpolate(0, in_row1);
	x_interpolate(1, in_row2);
	ylim = 1;
	frcy = inty = 0;
	outptr = fin_slice1;
	for (new_row=0; new_row<rows; new_row++)
	{	while (inty >= ylim)
		{	ylim++;
			if (ylim >= dtrows)
				in_row1 = in_row2;
			else
			{	temp = in_row1;
				in_row1 = in_row2;
				in_row2 = temp;
				x_interpolate(ylim, in_row2);
			}
		}
		inptr1 = in_row1;
		inptr2 = in_row2;
		for (i=0; i<columns; i++)
		{	*outptr = *inptr1*(1-frcy)+*inptr2*frcy;
			outptr++;
			inptr1++;
			inptr2++;
		}
		frcy += y_resample_ratio;
		i = frcy;
		frcy -= i;
		inty += i;
	}
}

get_slice()
{
	unsigned short *temp;
	float *ftemp;
	static slices_read;

	temp = in_slice2;
	in_slice2 = in_slice1;
	in_slice1 = temp;
	if (slices_read < dtslices)
	{	if (slices_read == 0)
			slices_read = first_slice-1;
		read_slice();
		slices_read++;
	}
	else
		in_slice1=in_slice2;
	if (voxel_size != xr)
	{	ftemp=fin_slice2;
		fin_slice2=fin_slice1;
		fin_slice1=ftemp;
		y_interpolate();
	}
}

/*    Modified: 8/14/95 VReadData called by Dewey Odhner. */
read_slice()
{
	int j;

	if (byte_in_slice == NULL)
	{	if (VReadData(in_slice1, in_slice_size/(dtcolumns*dtrows),
				dtcolumns*dtrows, dtfile, &j))
		{	(void)fprintf(stderr, "Input error %sat %d\n",
				feof(dtfile)? "EOF ": "", ftell(dtfile));
			vol_exit(2);
		}
	}
	else
	{	if (VReadData(byte_in_slice, in_slice_size/(dtcolumns*dtrows),
				dtcolumns*dtrows, dtfile, &j))
		{	(void)fprintf(stderr, "Input error %sat %d\n",
				feof(dtfile)? "EOF ": "", ftell(dtfile));
			vol_exit(2);
		}
		for (j=0; j<in_slice_size; j++)
			in_slice1[j] = byte_in_slice[j];
	}
}

/* Interpolate between slices */
/* Modified: 4/16/96 unsigned short used for input by Dewey Odhner */
z_interpolate()
{
	int i, j, k, intz, zlim;
	float v1,v2, frcz;
	unsigned short *inptr1, *inptr2;
	float *finptr1, *finptr2;
	data_value *outptr;

	this_slicen = -2;
	if (fseek(dtfile, data_pos+(first_slice-1)*in_slice_size, 0))
	{	fprintf(stderr, "Error in fseek\n");
		vol_exit (5);
	}
	get_slice();
	get_slice();
	zlim = 1;
	frcz = intz = 0;
	while (this_slicen<slices)
	{	while (intz >= zlim)
		{	get_slice();
			zlim++;
		}
		if (this_slicen+3 <= slices) {

			printf("Processing slice %d/%d\n", this_slicen+3, slices);
		fflush(stdout);

		}

		v1 = 1-frcz;
		v2 = frcz;
		outptr=next_next_slice_data;
		inptr1=in_slice2;
		inptr2=in_slice1;
		finptr1=fin_slice2;
		finptr2=fin_slice1;
		if (voxel_size != xr)
			for (j=0; j<rows; j++)
				for (k=0; k<columns; k++)
					*outptr++ = *finptr1++*v1+*finptr2++*v2;
		else
			for (j=0; j<rows; j++)
				for (k=0; k<columns; k++)
					*outptr++ = *inptr1++*v1+*inptr2++*v2;
		if (this_slicen >= -1)
			while (this_row<rows)
			{	init_row();
				while (this_column<columns)
					hollow_voxel();
				next_row();
			}
		next_slice();
		frcz = frcz+z_resample_ratio;
		i = frcz;
		frcz -= i;
		intz += i;
	}

	next_row();
	if (fwrite(out_data, 2, buffer_out_ptr-out_data, tempfile) !=
			buffer_out_ptr-out_data)
	{	(void)fprintf(stderr, "Output error\n");
		vol_exit(1);
	}
	free(out_data);
	free(in_slices);
	if (fin_slices)
	{	free(fin_slices);
		free(in_rows);
	}
	(void)fclose(dtfile);

}


/*  Modified: 10/8/93 boundary conditions changed by Dewey Odhner */
get_gradient(gxp, gyp, gzp, previous_slice, current_slice, following_slice,
		sliceno)
	float *gxp, *gyp, *gzp;
	data_value *previous_slice, *current_slice, *following_slice;
	int sliceno;
{
	typedef float f3x3[3][3];
	f3x3 *p, neigborhood_array[3];
	float mf, mb;

	p = (f3x3 *)&neigborhood_array[1][1][1];
	if (this_column >= columns-1)
		p[1][-1][-1] = p[1][-1][0] = p[1][-1][1] =
			p[1][0][-1] = p[1][0][0] = p[1][0][1] =
			p[1][1][-1] = p[1][1][0] = p[1][1][1] = current_slice[0];
	else
	{	if (this_row >= rows-1)
			p[1][1][-1] = p[1][1][0] = p[1][1][1] = current_slice[0];
		else
		{	if (sliceno >= slices-1)
				p[1][1][1] = current_slice[0];
			else
				p[1][1][1] = following_slice[columns+1];
			if (sliceno <= 0)
				p[1][1][-1] = current_slice[0];
			else
				p[1][1][-1] = previous_slice[columns+1];
			p[1][1][0] = current_slice[columns+1];
		}
		if (this_row <= 0)
			p[1][-1][-1] = p[1][-1][0] = p[1][-1][1] = current_slice[0];
		else
		{	if (sliceno >= slices-1)
				p[1][-1][1] = current_slice[0];
			else
				p[1][-1][1] = following_slice[-columns+1];
			if (sliceno <= 0)
				p[1][-1][-1] = current_slice[0];
			else
				p[1][-1][-1] = previous_slice[-columns+1];
			p[1][-1][0] = current_slice[-columns+1];
		}
		if (sliceno >= slices-1)
			p[1][0][1] = current_slice[0];
		else
			p[1][0][1] = following_slice[1];
		if (sliceno <= 0)
			p[1][0][-1] = current_slice[0];
		else
			p[1][0][-1] = previous_slice[1];
		p[1][0][0] = current_slice[1];
	}
	if (this_column <= 0)
		p[-1][-1][-1] = p[-1][-1][0] = p[-1][-1][1] =
			p[-1][0][-1] = p[-1][0][0] = p[-1][0][1] =
			p[-1][1][-1] = p[-1][1][0] = p[-1][1][1] = current_slice[0];
	else
	{	if (this_row >= rows-1)
			p[-1][1][-1] = p[-1][1][0] = p[-1][1][1] = current_slice[0];
		else
		{	if (sliceno >= slices-1)
				p[-1][1][1] = current_slice[0];
			else
				p[-1][1][1] = following_slice[columns-1];
			if (sliceno <= 0)
				p[-1][1][-1] = current_slice[0];
			else
				p[-1][1][-1] = previous_slice[columns-1];
			p[-1][1][0] = current_slice[columns-1];
		}
		if (this_row <= 0)
			p[-1][-1][-1] = p[-1][-1][0] = p[-1][-1][1] = current_slice[0];
		else
		{	if (sliceno >= slices-1)
				p[-1][-1][1] = current_slice[0];
			else
				p[-1][-1][1] = following_slice[-columns-1];
			if (sliceno <= 0)
				p[-1][-1][-1] = current_slice[0];
			else
				p[-1][-1][-1] = previous_slice[-columns-1];
			p[-1][-1][0] = current_slice[-columns-1];
		}
		if (sliceno >= slices-1)
			p[-1][0][1] = current_slice[0];
		else
			p[-1][0][1] = following_slice[-1];
		if (sliceno <= 0)
			p[-1][0][-1] = current_slice[0];
		else
			p[-1][0][-1] = previous_slice[-1];
		p[-1][0][0] = current_slice[-1];
	}
	if (this_row >= rows-1)
		p[0][1][-1] = p[0][1][0] = p[0][1][1] = current_slice[0];
	else
	{	if (sliceno >= slices-1)
			p[0][1][1] = current_slice[0];
		else
			p[0][1][1] = following_slice[columns];
		if (sliceno <= 0)
			p[0][1][-1] = current_slice[0];
		else
			p[0][1][-1] = previous_slice[columns];
		p[0][1][0] = current_slice[columns];
	}
	if (this_row <= 0)
		p[0][-1][-1] = p[0][-1][0] = p[0][-1][1] = current_slice[0];
	else
	{	if (sliceno >= slices-1)
			p[0][-1][1] = current_slice[0];
		else
			p[0][-1][1] = following_slice[-columns];
		if (sliceno <= 0)
			p[0][-1][-1] = current_slice[0];
		else
			p[0][-1][-1] = previous_slice[-columns];
		p[0][-1][0] = current_slice[-columns];
	}
	if (sliceno >= slices-1)
		p[0][0][1] = current_slice[0];
	else
		p[0][0][1] = following_slice[0];
	if (sliceno <= 0)
		p[0][0][-1] = current_slice[0];
	else
		p[0][0][-1] = previous_slice[0];

	mb = (p[-1][0][0]+1/SQRT2*(p[-1][1][0]+p[-1][-1][0]+p[-1][0][1]+
		p[-1][0][-1])+1/SQRT3*(p[-1][1][1]+p[-1][-1][1]+p[-1][1][-1]+
		p[-1][-1][-1]))*(1/(1+2*SQRT2+4/SQRT3));
	mf = (p[1][0][0]+1/SQRT2*(p[1][1][0]+p[1][-1][0]+p[1][0][1]+p[1][0][-1])+
		1/SQRT3*(p[1][1][1]+p[1][-1][1]+p[1][1][-1]+p[1][-1][-1]))*
		(1/(1+2*SQRT2+4/SQRT3));
/*
	*gxp =	mb>mf
			?	mb>current_slice[0]
				?	.5*(mf-mb)/voxel_size
				:	(mf-current_slice[0])/voxel_size
			:	mf>current_slice[0]
				?	.5*(mf-mb)/voxel_size
				:	(current_slice[0]-mb)/voxel_size;
*/
	*gxp = .5*(mf-mb)/voxel_size;
	mb = (p[0][-1][0]+1/SQRT2*(p[0][-1][-1]+p[0][-1][1]+p[-1][-1][0]+
		p[1][-1][0])+1/SQRT3*(p[-1][-1][-1]+p[1][-1][-1]+p[-1][-1][1]+
		p[1][-1][1]))*(1/(1+2*SQRT2+4/SQRT3));
	mf = (p[0][1][0]+1/SQRT2*(p[0][1][-1]+p[0][1][1]+p[-1][1][0]+p[1][1][0])+
		1/SQRT3*(p[-1][1][-1]+p[1][1][-1]+p[-1][1][1]+p[1][1][1]))*
		(1/(1+2*SQRT2+4/SQRT3));
/*
	*gyp =	mb>mf
			?	mb>current_slice[0]
				?	.5*(mf-mb)/voxel_size
				:	(mf-current_slice[0])/voxel_size
			:	mf>current_slice[0]
				?	.5*(mf-mb)/voxel_size
				:	(current_slice[0]-mb)/voxel_size;
*/
	*gyp = .5*(mf-mb)/voxel_size;
	mb = (p[0][0][-1]+1/SQRT2*(p[0][-1][-1]+p[0][1][-1]+p[-1][0][-1]+
		p[1][0][-1])+1/SQRT3*(p[-1][-1][-1]+p[1][-1][-1]+p[-1][1][-1]+
		p[1][1][-1]))*(1/(1+2*SQRT2+4/SQRT3));
	mf = (p[0][0][1]+1/SQRT2*(p[0][-1][1]+p[0][1][1]+p[-1][0][1]+p[1][0][1])+
		1/SQRT3*(p[-1][-1][1]+p[1][-1][1]+p[-1][1][1]+p[1][1][1]))*
		(1/(1+2*SQRT2+4/SQRT3));
/*
	*gzp =	mb>mf
			?	mb>current_slice[0]
				?	.5*(mf-mb)/slice_thickness
				:	(mf-current_slice[0])/slice_thickness
			:	mf>current_slice[0]
				?	.5*(mf-mb)/slice_thickness
				:	(current_slice[0]-mb)/slice_thickness;
*/
	*gzp = .5*(mf-mb)/slice_thickness;
}


/* The following function computes the neighbor-code of the current voxel and
 *	if it is a border voxel, computes the gradient code and adds it to the
 *	voxel list. class
 */
static void hollow_voxel()
{
	register unsigned short neighbors;
	float gradient, percentage, density;
	int tissue_code;

	switch (method)
	{
		case PERCENT:
		
			density = *next_slice_in_ptr;
			tissue_code =
				density>threshold[2]
				?	density>threshold[4]
					?	density<threshold[5]
						?	6
						:	7
					:	density<threshold[3]
						?	4
						:	5
				:	density>threshold[0]
					?	density<threshold[1]
						?	2
						:	3
					:	1;
/*---------------------------

	if ((threshold[3] <= threshold[4]) && (threshold[7] > threshold[8])
		&& (threshold[11] > threshold[12]))

		tissue_code =
				density>threshold[8]
				?	density>threshold[12]
					?	density<threshold[11]
						?	6
						:	7
					:	density<threshold[7]
						?	4
						:	5
				:	density>threshold[3]
					?	density<threshold[4]
						?	0
						:	3
					:	1;

	if ((threshold[3] > threshold[4]) && (threshold[7] <= threshold[8])
		&& (threshold[11] > threshold[12]))

  		tissue_code =
				density>threshold[7]
				?	density>threshold[12]
					?	density<threshold[11]
						?	6
						:	7
					:	density<threshold[8]
						?	0
						:	5
				:	density>threshold[4]
					?	density<threshold[3]
						?	2
						:	3
					:	1;
	
	if ((threshold[3] > threshold[4]) && (threshold[7] > threshold[8])
		&& (threshold[11] <= threshold[12]))

  		tissue_code =
				density>threshold[8]
				?	density>threshold[11]
					?	density<threshold[12]
						?	0
						:	7
					:	density<threshold[7]
						?	4
						:	5
				:	density>threshold[4]
					?	density<threshold[3]
						?	2
						:	3
					:	1;

	if ((threshold[3] <= threshold[4]) && (threshold[7] <= threshold[8])
		&& (threshold[11] > threshold[12]))

  		tissue_code =
				density>threshold[7]
				?	density>threshold[12]
					?	density<threshold[11]
						?	6
						:	7
					:	density<threshold[8]
						?	0
						:	5
				:	density>threshold[3]
					?	density<threshold[4]
						?	0
						:	3
					:	1;

	if ((threshold[3] > threshold[4]) && (threshold[7] <= threshold[8])
		&& (threshold[11] <= threshold[12]))

  		tissue_code =
				density>threshold[7]
				?	density>threshold[11]
					?	density<threshold[12]
						?	0
						:	7
					:	density<threshold[8]
						?	0
						:	5
				:	density>threshold[4]
					?	density<threshold[3]
						?	2
						:	3
					:	1;

	if ((threshold[3] <= threshold[4]) && (threshold[7] > threshold[8])
		&& (threshold[11] <= threshold[12]))

  		tissue_code =
				density>threshold[8]
				?	density>threshold[11]
					?	density<threshold[12]
						?	0
						:	7
					:	density<threshold[7]
						?	4
						:	5
				:	density>threshold[3]
					?	density<threshold[4]
						?	0
						:	3
					:	1;

	if ((threshold[3] <= threshold[4]) && (threshold[7] <= threshold[8])
		&& (threshold[11] <= threshold[12]))

  		tissue_code =
				density>threshold[7]
				?	density>threshold[11]
					?	density<threshold[12]
						?	0
						:	7
					:	density<threshold[8]
						?	0
						:	5
				:	density>threshold[3]
					?	density<threshold[4]
						?	0
						:	3
					:	1;

-------------------*/

			next_slice_opacity_ptr->ttype = tissue_code;
			switch (tissue_code)
			{	case 1:
				case 3:
				case 5:
				case 7:
					percentage = 1;
					break;
				case 2:
					percentage = (density-threshold[0])/
						(threshold[1]-threshold[0]);
					break;
				case 4:
					percentage = (density-threshold[2])/
						(threshold[3]-threshold[2]);
					break;
				case 6:
					percentage = (density-threshold[4])/
						(threshold[5]-threshold[4]);
					break;
			}
			next_slice_opacity_ptr->pct = percentage*255;
			break;

		case GRADIENT:
			if (*next_slice_in_ptr >= min_density)
			{
				get_gradient(&gx, &gy, &gz, this_in_ptr,
					next_slice_in_ptr, next_next_slice_in_ptr, this_slicen+1);
				gradient = sqrt(gx*gx+gy*gy+gz*gz);
				if (gradient >= min_gradient)
				{	float grad_byte;

					if (gradient > max_gradient)
						gradient = max_gradient;
					grad_byte = 255*(gradient_cliff+(1-gradient_cliff)*
						(gradient-min_gradient)/(max_gradient-min_gradient));
					next_slice_opacity_ptr->gradient = grad_byte;
					density =	*next_slice_in_ptr<max_density
								?	*next_slice_in_ptr
								:	max_density;
					density = (density-min_density)/(max_density-min_density);
					density = density_cliff+(1-density_cliff)*density;
					next_slice_opacity_ptr->opacity = density*grad_byte;
				}
				else
					next_slice_opacity_ptr->opacity = 0;
			}
			else
				next_slice_opacity_ptr->opacity = 0;
			break;
	}
	if (this_slicen>=0 &&
			(	method==GRADIENT
				?	this_opacity_ptr->opacity
				:	materl_opacity[this_opacity_ptr->ttype][
						this_opacity_ptr->pct]
			)>min_opacity)
	{	volume++;
		neighbors = 0;

#define opacity_test(ptr) \
(	(	method==GRADIENT \
		?	(ptr)->opacity \
		:	materl_opacity[(ptr)->ttype][(ptr)->pct] \
	) >= max_opacity \
)

		if (this_slicen>0)
		{	last_slice_in_val = *last_slice_in_ptr;
			if opacity_test(last_slice_opacity_ptr)
				neighbors |= NZ;
		}
		else
			last_slice_in_val = 0;
		if (this_slicen < slices-1)
		{	next_slice_in_val = *next_slice_in_ptr;
			if opacity_test(next_slice_opacity_ptr)
				neighbors |= PZ;
		}
		else
			next_slice_in_val = 0;
		if (this_row > 0)
		{	last_row_in_val = this_in_ptr[-columns];
			if opacity_test(this_opacity_ptr-columns)
				neighbors |= NY;
		}
		else
			last_row_in_val = 0;
		if (this_row < rows-1)
		{	next_row_in_val = this_in_ptr[columns];
			if opacity_test(this_opacity_ptr+columns)
				neighbors |= PY;
		}
		else
			next_row_in_val = 0;
		if (this_column > 0)
		{	last_column_in_val = this_in_ptr[-1];
			if opacity_test(this_opacity_ptr-1)
				neighbors |= NX;
			if (this_column < min_x)
				min_x = this_column;
		}
		else
			last_column_in_val = min_x = 0;
		if (this_column < columns-1)
		{	next_column_in_val = this_in_ptr[1];
			if opacity_test(this_opacity_ptr+1)
				neighbors |= PX;
			if (this_column > max_x)
				max_x = this_column;
		}
		else
		{	next_column_in_val = 0;
			max_x = this_column;
		}
		this_in_val = this_in_ptr[0];
		if (neighbors != ALL_NEIGHBORS)
		{
			get_gradient(&gx, &gy, &gz, last_slice_in_ptr,
				this_in_ptr, next_slice_in_ptr, this_slicen);
			*buffer_out_ptr++ = neighbors|this_column;
			switch (method)
			{	case GRADIENT:
					*buffer_out_ptr++ = G_code(gx, gy, gz);
					*buffer_out_ptr++ = ((short)this_opacity_ptr->gradient<<8)|
						this_opacity_ptr->opacity;
					dummy_out_ptr += 3;
					break;
				case PERCENT:
					gradient = sqrt(gx*gx+gy*gy+gz*gz);
					if (gradient > max_gradient)
						gradient = max_gradient;
					density = *this_in_ptr;
					*buffer_out_ptr++ = G_code(gx, gy, gz) |
						((short)this_opacity_ptr->ttype<<11);
					*buffer_out_ptr++ = ((short)
						(255*gradient/max_gradient)<<8)|this_opacity_ptr->pct;
					dummy_out_ptr += 3;
					break;
							}
		}
	}


	this_in_ptr++;
	next_slice_in_ptr++;
	next_next_slice_in_ptr++;
	last_slice_in_ptr++;
	this_opacity_ptr++;
	next_slice_opacity_ptr++;
	last_slice_opacity_ptr++;
	this_column++;
}

/* Get ready to encode new row */
static void init_row()
{	this_column=0;
	if (buffer_out_ptr-out_data >= OUT_BUFFER_SIZE-3*columns)
	{	if (fwrite(out_data, 2, buffer_out_ptr-out_data, tempfile) !=
				buffer_out_ptr-out_data)
		{	(void)fprintf(stderr, "Output error\n");
			vol_exit(1);
		}
		buffer_out_ptr=out_data;
	}
}

/* Make pointer-table entry for this row & increment row-counter */
static void next_row()
{	if (this_slicen >= 0)
	{	if (last_out_ptr != dummy_out_ptr)
		{	if (this_row > max_y)
				max_y = this_row;
			if (this_row<min_y)
				min_y = this_row;
			if (this_slicen>max_z)
				max_z = this_slicen;
			if (this_slicen<min_z)
				min_z = this_slicen;
		}
		*this_ptr_ptr = last_out_ptr;
		last_out_ptr = dummy_out_ptr;
		this_ptr_ptr++;
	}
	this_row++;
}

/* Get ready to encode new slice */
static void next_slice()
{	data_value *temp;
	opacity_struct *otmp;

	this_slicen++;
	this_row = 0;
	temp = last_slice_data;
	last_slice_data = this_slice_data;
	this_slice_data = next_slice_data;
	next_slice_data = next_next_slice_data;
	next_next_slice_data = temp;
	this_slice_end = this_slice_data+rows*columns;
	this_in_ptr = this_slice_data;
	next_next_slice_in_ptr = next_next_slice_data;
	next_slice_in_ptr = next_slice_data;
	last_slice_in_ptr = last_slice_data;
	otmp = last_slice_opacity;
	last_slice_opacity = this_slice_opacity;
	this_slice_opacity = next_slice_opacity;
	next_slice_opacity = otmp;
	this_opacity_ptr = this_slice_opacity;
	next_slice_opacity_ptr = next_slice_opacity;
	last_slice_opacity_ptr = last_slice_opacity;
}

/* Set up to encode the scene */
/* Modified: 3/13/97 ptr_table_size corrected by Dewey Odhner. */
init_scene(argc, argv)
	int argc;
	char **argv;
{
	int j, k, final_slice, error_code;
	float opn[4];
	ViewnixHeader scene_header;
	char bad_group[5], bad_element[5];

/*	switch (argv[0][strlen(argv[0])-1])
	{	case 'r':
*/
			method = PERCENT;
			if (argc!=20 && argc!=21)
			{	printf("USAGE:\n%s %s %s %s%s%s%s", argv[0],
				"file max_gradient min_opacity max_opacity lo2 hi1 lo3 hi2 lo4",
				"hi3 op1 op2 op3 op4 first_slice last_slice",
				"pixel_size slice_thickness nbits [output_file]\n",
				"pixel_size: desired size mm, or 0 for no interpolation\n",
				"slice_thickness: desired size mm, or 0 for cubic voxels,\n",
				"   or negative value for no interpolation\n");
				vol_exit (1);
			}
			if (argc == 21)
				strcpy(file_header.gen.filename, argv[20]);
			else
                             { printf("agrc: %d\n", argc);
			       printf("Specify the file name with extension\n");
		             }
									
			(void)strcpy(file_header.gen.filename1,argv[1]);
			max_gradient = atof(argv[2]);
			min_opacity = atof(argv[3])*255;
			max_opacity = atof(argv[4])*255;
			threshold[0] = atof(argv[5]);
			threshold[1] = atof(argv[6]);
			threshold[2] = atof(argv[7]);
			threshold[3] = atof(argv[8]);
			threshold[4] = atof(argv[9]);
			threshold[5] = atof(argv[10]);
			opn[0] = atof(argv[11]);
			opn[1] = atof(argv[12]);
			opn[2] = atof(argv[13]);
			opn[3] = atof(argv[14]);
			first_slice = atoi(argv[15]);
			final_slice = atoi(argv[16]);
			voxel_size = atof(argv[17]);
			slice_thickness = atof(argv[18]);
			nbits = atoi(argv[19]);
			for (k=0; k<256; k++)
			{	for (j=1; j<4; j++)
					materl_opacity[2*j][k] = k*opn[j]+(255-k)*opn[j-1];
				for (j=0; j<4; j++)
					materl_opacity[2*j+1][k] = 255*opn[j];
			}

/*-----------------------------
			break;
		case 'l':
			class = GRADIENT;
			if (argc!=15 && argc!=16)
			{	printf("USAGE:\n%s %s %s %s %s%s%s%s", argv[0],
				"file min_density max_density density_cliff",
				"min_gradient max_gradient gradient_cliff",
				"min_opacity max_opacity pixel_size slice_thickness",
				"first_slice last_slice nbits [output_file]\n",
				"pixel_size: desired size mm, or 0 for no interpolation\n",
				"slice_thickness: desired size mm, or 0 for cubic voxels,\n",
				"   or negative value for no interpolation\n");
				exit (1);
			}
			if (argc == 16)
				strcpy(file_header.gen.filename, argv[15]);
			else 
				if (VCreateOutputFilename(argv[1], SH0, file_header.gen.filename))
				{	fprintf(stderr, "Can't create output file name.\n");
					exit(400);
				}
			(void)strcpy(file_header.gen.filename1,argv[1]);
			min_density = atof(argv[2]);
			max_density = atof(argv[3]);
			density_cliff = atof(argv[4]);
			min_gradient = atof(argv[5]);
			max_gradient = atof(argv[6]);
			gradient_cliff = atof(argv[7]);
			if (density_cliff<0 || density_cliff>1 ||
					gradient_cliff<0 || gradient_cliff>1)
			{	printf("density_cliff and gradient_cliff must be in [0, 1]\n");
				vol_exit(1);
			}
			min_opacity = atof(argv[8])*255;
			max_opacity = atof(argv[9])*255;
			if (min_opacity<0 || min_opacity>255 || max_opacity<0 || max_opacity>255)
			{	printf("min_opacity and max_opacity must be in [0, 1]\n");
				vol_exit(1);
			}
			voxel_size = atof(argv[10]);
			slice_thickness = atof(argv[11]);
			first_slice = atoi(argv[12]);
			final_slice = atoi(argv[13]);
			nbits = atoi(argv[14]);
			break;
		case 'h':
			class = BINARY;
			if (argc!=8 && argc!=9)
			{	printf("USAGE:\n%s %s %s%s%s%s", argv[0],
				"file threshold first_slice last_slice",
				"pixel_size slice_thickness nbits [output_file]\n",
				"pixel_size: desired size mm, or 0 for no interpolation\n",
				"slice_thickness: desired size mm, or 0 for cubic voxels,\n",
				"   or negative value for no interpolation\n");
				exit (1);
			}
			if (argc == 9)
				strcpy(file_header.gen.filename, argv[8]);
			else
				if (VCreateOutputFilename(argv[1], BS0,
						file_header.gen.filename))
				{	fprintf(stderr, "Can't create output file name.\n");
					exit(400);
				}  
			(void)strcpy(file_header.gen.filename1, argv[1]);
			max_gradient = 0;
			min_opacity = 0;
			max_opacity = 255;
			threshold[0] = 0;
			threshold[1] = 0;
			threshold[2] = 0;
			threshold[3] = 0;
			threshold[4] = atof(argv[2]);
			threshold[5] = threshold[4];
			opn[0] = 0;
			opn[1] = 0;
			opn[2] = 0;
			opn[3] = 1;
			first_slice = atoi(argv[3]);
			final_slice = atoi(argv[4]);
			voxel_size = atof(argv[5]);
			slice_thickness = atof(argv[6]);
			nbits = atoi(argv[7]);
			for (k=0; k<256; k++)
			{	for (j=1; j<4; j++)
					tissue_opacity[2*j][k] = k*opn[j]+(255-k)*opn[j-1];
				for (j=0; j<4; j++)
					tissue_opacity[2*j+1][k] = 255*opn[j];
			}
			break;
	}

---------------------------------------*/

	file_header.gen.filename_valid = 1;
	file_header.gen.filename1_valid = 1;
	if (nbits!=3 && nbits!=4)
	{	fprintf(stderr, "nbits must be 3 or 4\n");
		vol_exit(-1);
	}
	if ((dtfile = fopen(file_header.gen.filename1,"rb")) == NULL ) {
			printf("Can't open %s\n",file_header.gen.filename1);
			vol_exit(1);
	}
	error_code = VReadHeader(dtfile, &scene_header, bad_group, bad_element);
	switch (error_code)
	{	case 0:
		case 107:
			break;
		case 106:
			fprintf(stderr, "Group %s element %s undefined in VReadHeader\n",
				bad_group, bad_element);
			break;
		default:
			fprintf(stderr, "Group %s element %s undefined in VReadHeader\n",
				bad_group, bad_element);
			vol_exit (error_code);
	}
	if (scene_header.scn.dimension<3 || scene_header.scn.dimension>4)
	{	fprintf(stderr, "only 3-dimensional scenes handled\n");
		vol_exit (-1);
	}
	if (scene_header.scn.dimension > 3)
		fprintf(stderr, "warning: only the first volume will be processed.\n");
	if (scene_header.scn.measurement_unit[0]!=
			scene_header.scn.measurement_unit[1] ||
		        scene_header.scn.measurement_unit[1]!=
			scene_header.scn.measurement_unit[2])
	{	fprintf(stderr, "measurement unit must be same in all 3 directions\n");
		vol_exit (-1);
	}
	if (scene_header.scn.num_of_density_values != 1)
	{	fprintf(stderr, "only single-valued cells handled\n");
		vol_exit (-1);
	}
	if (scene_header.scn.num_of_integers != 1)
	{	fprintf(stderr, "only integer-valued cells handled\n");
		vol_exit (-1);
	}
	switch (scene_header.scn.num_of_bits)
	{	case 16:
			if (scene_header.scn.bit_fields[0]!=0 ||
					scene_header.scn.bit_fields[1]!=15)
			{	fprintf(stderr, "bit field {%d,..,%d} not handled\n",
					scene_header.scn.bit_fields[0],
					scene_header.scn.bit_fields[1]);
				vol_exit (-1);
			}
			break;
		case 8:
			if (scene_header.scn.bit_fields[0]!=0 ||
					scene_header.scn.bit_fields[1]!=7)
			{	fprintf(stderr, "bit field {%d,..,%d} not handled\n",
					scene_header.scn.bit_fields[0],
					scene_header.scn.bit_fields[1]);
				vol_exit (-1);
			}
			break;
		default:
			fprintf(stderr, "%d bits per cell not handled\n",
				scene_header.scn.num_of_bits);
			vol_exit (-1);
	}
	if (scene_header.scn.dimension_in_alignment!=0 &&
			scene_header.scn.bytes_in_alignment>scene_header.scn.num_of_bits/8)
	{	fprintf(stderr, "alignment not handled\n");
		vol_exit (-1);
	}
	xr = scene_header.scn.xypixsz[0];
	yr = scene_header.scn.xypixsz[1];
	dtcolumns = scene_header.scn.xysize[0];
	dtrows = scene_header.scn.xysize[1];
	switch (scene_header.scn.dimension)
	{	case 3:
			dtslices = scene_header.scn.num_of_subscenes[0];
			zr = (scene_header.scn.loc_of_subscenes[dtslices-1]-
				scene_header.scn.loc_of_subscenes[0])/(dtslices-1);
			break;
		case 4:
			dtslices = scene_header.scn.num_of_subscenes[1];
			zr = (scene_header.scn.loc_of_subscenes[
				scene_header.scn.num_of_subscenes[0]+dtslices-1]-
				scene_header.scn.loc_of_subscenes[
				scene_header.scn.num_of_subscenes[0]])/(dtslices-1);
			break;
	}
	error_code = VGetHeaderLength(dtfile, &data_pos);
	if (error_code)
	{	fprintf(stderr, "Error %d in VGetHeaderLength\n", error_code);
		vol_exit (error_code);
	}
	if (fseek(dtfile, data_pos, 0))
	{	fprintf(stderr, "Error in fseek\n");
		vol_exit (5);
	}
	if (voxel_size<0 && slice_thickness==0 && zr<xr)
		slice_thickness = zr;
	if (voxel_size <= 0)
		voxel_size = xr;
	if (slice_thickness==0)
		slice_thickness = voxel_size;
	else if (slice_thickness < 0)
		slice_thickness = zr;
	in_slice_size = dtcolumns*dtrows;
	if (scene_header.scn.num_of_bits == 8)
	{	byte_in_slice = (unsigned char *)malloc(in_slice_size);
		if (byte_in_slice == NULL)
		{	puts("\nout_data memory allocation failure.\n");
			vol_exit(1);
		}
	}
	out_data = (unsigned short *)malloc(OUT_BUFFER_SIZE*sizeof(short));
	if (out_data == NULL)
	{	puts("\nout_data memory allocation failure.\n");
		vol_exit(1);
	}
	if ((in_slices = (unsigned short *)malloc(in_slice_size*4)) == NULL ) {
		printf("in_slices slice memory allocation failure\n");
		vol_exit(1);
	}
	in_slice2 = in_slices;
	in_slice1 = in_slices+in_slice_size;
	if (scene_header.scn.num_of_bits == 16)
		in_slice_size *= 2;

	z_resample_ratio = slice_thickness/zr;
	if (final_slice > dtslices)
		final_slice = dtslices;
	slices = (int)((final_slice-first_slice)/z_resample_ratio+0.5) + 1;
	if (z_resample_ratio * (slices-1) > final_slice-1) slices--;
	if (slice_thickness == zr)
	{	slices = final_slice;
		z_resample_ratio = 1;
	}
	x_resample_ratio = voxel_size/xr;
	y_resample_ratio = voxel_size/yr;
	columns = (int)((dtcolumns-1)/x_resample_ratio+0.5)+1;
	if (x_resample_ratio*(columns-1) > dtcolumns-1)
		columns--;
	rows = (int)((dtrows-1)/y_resample_ratio+0.5)+1;
	if (y_resample_ratio*(rows-1) > dtrows-1)
		rows--;
	if (voxel_size==xr && voxel_size==yr)
	{	columns = dtcolumns;
		rows = dtrows;
		x_resample_ratio = 1;
		y_resample_ratio = 1;
	}
	else
	{	if ((fin_slices=(float *)malloc(columns*rows*sizeof(float)*2))==NULL)
		{	printf("fin_slices memory allocation failure\n");
			vol_exit(1);
		}
		fin_slice2 = fin_slices;
		fin_slice1 = fin_slices+columns*rows;
	}
	if ((this_slice_data=(float *)malloc(rows*columns*sizeof(float)))==NULL)
	{	puts("\nthis_slice memory allocation failure.\n");
		vol_exit(1);
	}
	if ((next_slice_data=(float *)malloc(rows*columns*sizeof(float)))==NULL)
	{	puts("\nnext_slice memory allocation failure.\n");
		vol_exit(1);
	}
	next_next_slice_data = (float *)malloc(rows*columns*sizeof(float));
	if (next_next_slice_data == NULL)
	{	puts("\nnext_next_slice memory allocation failure.\n");
		vol_exit(1);
	}
	if ((last_slice_data=(float *)malloc(rows*columns*sizeof(float)))==NULL)
	{	puts("\nlast_slice memory allocation failure.\n");
		vol_exit(1);
	}
	this_slice_opacity =
		(opacity_struct *)malloc(rows*columns*sizeof(opacity_struct));
	if (this_slice_opacity == NULL)
	{	puts("\nthis_slice_opacity memory allocation failure.\n");
		vol_exit(1);
	}
	last_slice_opacity =
		(opacity_struct *)malloc(rows*columns*sizeof(opacity_struct));
	if (last_slice_opacity == NULL)
	{	puts("\nlast_slice_opacity memory allocation failure.\n");
		vol_exit(1);
	}
	next_slice_opacity =
		(opacity_struct *)malloc(rows*columns*sizeof(opacity_struct));
	if (next_slice_opacity == NULL)
	{	puts("\nnext_slice_opacity memory allocation failure.\n");
		vol_exit(1);
	}
	printf("Interpolating to %d Slices \n",slices);

#define ptr_table_size ((rows*slices+1)*sizeof(long))
	if
	( (this_ptr_ptr=ptr_table=(long *)malloc(ptr_table_size))==NULL
	)
	{	puts("\nptr_table memory allocation failure.\n");
		vol_exit(1);
	}

	sprintf(tempfile_name, "%d.TSE", getpid());

	if ((tempfile=fopen(tempfile_name, "w+b"))==NULL)
	{	puts("file creation error.");
		vol_exit(1);
	}
	buffer_out_ptr = out_data;
	last_out_ptr = dummy_out_ptr = volume = max_x = max_y = max_z = 0;
	min_x = columns;
	min_y = rows;
	min_z = slices;
	file_header.str.dimension = 3;
	file_header.str.dimension_valid = 1;

	/* Get this stuff from input file if it's there */
	if (scene_header.scn.domain_valid)
		if (scene_header.scn.dimension > 3)
			scene_header.scn.domain_valid = FALSE;
		else
			file_header.str.domain = scene_header.scn.domain;
	if (!scene_header.scn.domain_valid)
		file_header.str.domain = (float *)malloc(12*sizeof(float));
	if (scene_header.scn.axis_label_valid)
		file_header.str.axis_label = scene_header.scn.axis_label;
	else
		file_header.str.axis_label=(Char30 *)malloc(3*sizeof(Char30));
	file_header.str.measurement_unit =
		(short *)malloc(3*sizeof(short));
	file_header.str.num_of_TSE = (unsigned *)malloc(sizeof(unsigned));
	file_header.str.num_of_NTSE = (unsigned *)malloc(sizeof(unsigned));
	file_header.str.smallest_value = (float *)malloc(9*sizeof(float));
	file_header.str.largest_value = (float *)malloc(9*sizeof(float));
	file_header.str.signed_bits_in_NTSE =
		(short *)malloc(sizeof(short));
	file_header.str.num_of_samples = (short *)malloc(sizeof(short));
	file_header.str.min_max_coordinates =
		(float *)malloc(6*sizeof(float));
	file_header.str.description_of_element = (short *)malloc(sizeof(short));
	file_header.str.parameter_vectors = (float *)malloc(sizeof(float));
	file_header.str.TSE_measurement_unit = (short *)malloc(9*sizeof(short));
	file_header.str.NTSE_measurement_unit = (short *)malloc(sizeof(short));
	file_header.str.scene_file = (Char30 *)malloc(sizeof(Char30));
	if (file_header.str.domain==NULL ||
			file_header.str.axis_label==NULL ||
			file_header.str.measurement_unit==NULL ||
			file_header.str.num_of_TSE==NULL ||
			file_header.str.num_of_NTSE==NULL ||
			file_header.str.smallest_value==NULL ||
			file_header.str.largest_value==NULL ||
			file_header.str.signed_bits_in_NTSE==NULL ||
			file_header.str.num_of_samples==NULL ||
			file_header.str.min_max_coordinates==NULL ||
			file_header.str.description_of_element==NULL ||
			file_header.str.parameter_vectors==NULL ||
			file_header.str.TSE_measurement_unit==NULL ||
			file_header.str.NTSE_measurement_unit==NULL ||
			file_header.str.scene_file==NULL)
	{	fprintf(stderr, "insufficient memory\n");
		vol_exit(1);
	}
	(void)strcpy(file_header.str.scene_file[0], file_header.gen.filename1);
	file_header.str.scene_file_valid = 1;
	file_header.str.NTSE_measurement_unit[0] = 0;
	file_header.str.NTSE_measurement_unit_valid = 1;
	memset(file_header.str.TSE_measurement_unit, 0, 9*sizeof(short));
	file_header.str.TSE_measurement_unit_valid = 1;
	if (scene_header.scn.domain_valid == 0)
	{	file_header.str.domain[0] = 0;
		file_header.str.domain[1] = 0;
		file_header.str.domain[2] = 0;
		file_header.str.domain[3] = 1;
		file_header.str.domain[4] = 0;
		file_header.str.domain[5] = 0;
		file_header.str.domain[6] = 0;
		file_header.str.domain[7] = 1;
		file_header.str.domain[8] = 0;
		file_header.str.domain[9] = 0;
		file_header.str.domain[10] = 0;
		file_header.str.domain[11] = 1;
	}
	file_header.str.domain_valid = 1;
	if (scene_header.scn.axis_label_valid == 0)
	{	strcpy(file_header.str.axis_label[0], "x");
		strcpy(file_header.str.axis_label[1], "y");
		strcpy(file_header.str.axis_label[2], "z");
	}
	file_header.str.axis_label_valid = 1;
	file_header.str.num_of_samples[0] = slices;
	file_header.str.num_of_samples_valid = 1;
	file_header.str.loc_of_samples =
		(float *)malloc(file_header.str.num_of_samples[0]*sizeof(float));
	if (file_header.str.loc_of_samples == NULL)
	{	fprintf(stderr, "insufficient memory\n");
		vol_exit(1);
	}
	for (j=0; j<file_header.str.num_of_samples[0]; j++)
		file_header.str.loc_of_samples[j] =
			scene_header.scn.loc_of_subscenes[0]+j*slice_thickness;
	file_header.str.loc_of_samples_valid = 1;

     	file_header.str.measurement_unit[0] = scene_header.scn.measurement_unit[0];
	file_header.str.measurement_unit[1] = scene_header.scn.measurement_unit[1];
	file_header.str.measurement_unit[2] = scene_header.scn.measurement_unit[2];


	file_header.str.measurement_unit_valid = 1;
	file_header.str.xysize[0] = file_header.str.xysize[1] =
		voxel_size;
	file_header.str.xysize_valid = 1;
	file_header.str.num_of_elements = 1;
	file_header.str.num_of_elements_valid = 1;
	file_header.str.description_of_element[0] = 1;
	file_header.str.description_of_element_valid = 1;
	file_header.str.parameter_vectors[0] = 1;
	file_header.str.parameter_vectors_valid = 1;
	file_header.str.num_of_structures = 1;
	file_header.str.num_of_structures_valid = 1;
	file_header.str.description = command_line(argc, argv);
	file_header.str.description_valid = 1;
}

static char *command_line(argc, argv)
	int argc;
	char **argv;
{
	int j, k;
	char *cl;

	k = argc;
	for (j=0; j<argc; j++)
		k += strlen(argv[j]);
	cl = (char *)malloc(k);
	if (cl == NULL)
	{	(void)fprintf(stderr, "Memory allocation failure\n");
		vol_exit(1);
	}
	k=0;
	for (j=0; j<argc; j++)
	{	strcpy(cl+k, argv[j]);
		k += strlen(argv[j]);
		cl[k] = ' ';
		k++;
	}
	cl[k-1] = 0;
	return (cl);
}

/*****************************************************************************
 * FUNCTION: wrap_up
 * DESCRIPTION: Writes the shell data to the output file.
 * PARAMETERS: None
 * SIDE EFFECTS: Messages will be written to standard output.
 * ENTRY CONDITIONS: The classification must be done.  The global variables
 *    tempfile, tempfile_name, file_header.gen.filename,
 *    file_header.str.xysize, file_header.str.loc_of_samples, obj_data, max_x,
 *    max_y, max_z, min_x, min_y, min_z, ptr_table, volume, this_slice_data,
 *    next_slice_data, last_slice_data, last_out_ptr, rows, nbits, method
 *    must be appropriately initialized.
 * RETURN VALUE: None
 * EXIT CONDITIONS: If an error occurs, writes a message to stderr, removes
 *    the temporary file, and aborts the program.
 * HISTORY:
 *    Created: 11/9/93 by Dewey Odhner
 *    Modified: 3/13/97 printf parameter mismatch corrected by Dewey Odhner
 *
 *****************************************************************************/
static void wrap_up()
{
	unsigned short *tree_data;
	int error_code, items_written, this_slice, j;
	FILE *shell_file;
	char bad_group[5], bad_element[5];
	unsigned short *this_ptr, neighbors, this_tt, this_gcode;
	static short signed_bits_in_TSE[9], bit_fields_in_TSE[18]={0,5, 6,15,
		16,15, 21,23, 24,27, 28,31, 32,31, 40,39, 40,39},
		bit_fields_in_NTSE[2]={0,15};
	unsigned short **end_ptr_ptr, **that_ptr_ptr, *voxel_data, voxel_datum[3];

	/* compactify pointer array */
	this_ptr_ptr = ptr_table;
	for (this_slice=min_z; this_slice<=max_z; this_slice++)
		for (this_row=min_y; this_row<=max_y; this_row++)
			*this_ptr_ptr++ = ptr_table[rows*this_slice+this_row];
	*this_ptr_ptr = last_out_ptr;

	free(this_slice_data);
	free(last_slice_data);
	free(next_slice_data);
	rewind(tempfile);
	printf("\nHeader SH0-file name: %s\nBoundary voxels: %ld\nVolume: %d voxels\nMin x: %d  Min y: %d  Min z: %d\nMax x: %d  Max y: %d  Max z: %d\n",
		file_header.gen.filename, last_out_ptr/3,
		volume, min_x, min_y, min_z, max_x, max_y, max_z);
	file_header.str.smallest_value[0] = 0;
	file_header.str.smallest_value[1] = min_x;
	file_header.str.smallest_value[2] = 0;
	file_header.str.smallest_value[3] = 0;
	file_header.str.smallest_value[4] = 0;
	file_header.str.smallest_value[5] = 0;
	file_header.str.smallest_value[6] = 0;
	file_header.str.smallest_value[7] = 0;
	file_header.str.smallest_value[8] = 0;
	file_header.str.smallest_value_valid = 1;
	file_header.str.largest_value[0] = 63;
	file_header.str.largest_value[1] = max_x;
	file_header.str.largest_value[2] = 7;
	file_header.str.largest_value[3] = 6;
	file_header.str.largest_value[4] = (1<<nbits)-1;
	file_header.str.largest_value[5] = (1<<nbits)-1;
	file_header.str.largest_value[6] = 255;
	file_header.str.largest_value[7] = 255;
	file_header.str.largest_value[8] = 255;
	file_header.str.largest_value_valid = 1;
	obj_data.rows = max_y-min_y+1;
	file_header.str.min_max_coordinates[0] = min_x*file_header.str.xysize[0];
	file_header.str.min_max_coordinates[1] = min_y*file_header.str.xysize[1];
	file_header.str.min_max_coordinates[2] =
		file_header.str.loc_of_samples[min_z];
	file_header.str.min_max_coordinates[3] = max_x*file_header.str.xysize[0];
	file_header.str.min_max_coordinates[4] = max_y*file_header.str.xysize[1];
	file_header.str.min_max_coordinates[5] =
		file_header.str.loc_of_samples[max_z];
	file_header.str.min_max_coordinates_valid = 1;

	obj_data.ptr_table = (unsigned short **)ptr_table;
	voxel_data = (unsigned short *)malloc(last_out_ptr*2);
	end_ptr_ptr = obj_data.ptr_table+(max_y-min_y+1)*(max_z-min_z+1);
	for (that_ptr_ptr=obj_data.ptr_table; that_ptr_ptr<=end_ptr_ptr;
			that_ptr_ptr++)
		*that_ptr_ptr =
			voxel_data+ptr_table[that_ptr_ptr-obj_data.ptr_table];
	file_header.str.num_of_TSE[0] = (obj_data.ptr_table[
		obj_data.rows*(max_z-min_z+1)]-voxel_data)/3;
	file_header.str.num_of_NTSE[0] =
		1+(max_z-min_z+1)*(1+obj_data.rows);
	file_header.str.num_of_components_in_TSE = 9;
	file_header.str.num_of_components_in_NTSE = 1;
	file_header.str.num_of_integers_in_TSE = 9;
	file_header.str.signed_bits_in_TSE = signed_bits_in_TSE;
	file_header.str.bit_fields_in_TSE = bit_fields_in_TSE;
	bit_fields_in_TSE[9] = bit_fields_in_TSE[8]+nbits-1; /* n2 */
	bit_fields_in_TSE[11] = bit_fields_in_TSE[10]+nbits-1; /* n3 */
	switch (method)
	{	case GRADIENT:
			file_header.str.num_of_bits_in_TSE = 48;
			bit_fields_in_TSE[5] = 15; /* no tt */
			bit_fields_in_TSE[13] = 39; /* gm */
			bit_fields_in_TSE[15] = 47; /* op */
			bit_fields_in_TSE[17] = 39; /* no pg */
			break;
		case PERCENT:
			file_header.str.num_of_bits_in_TSE = 48;
			bit_fields_in_TSE[5] = 20; /* tt */
			bit_fields_in_TSE[13] = 39; /* gm */
			bit_fields_in_TSE[15] = 39; /* no op */
			bit_fields_in_TSE[17] = 47; /* pg */
			break;
	}
	file_header.str.num_of_integers_in_NTSE = 1;
	file_header.str.signed_bits_in_NTSE[0] = 0;
	file_header.str.num_of_bits_in_NTSE = 16;
	file_header.str.bit_fields_in_NTSE = bit_fields_in_NTSE;
	file_header.str.num_of_TSE_valid = 1;
	file_header.str.num_of_NTSE_valid = 1;
	file_header.str.num_of_components_in_TSE_valid = 1;
	file_header.str.num_of_components_in_NTSE_valid = 1;
	file_header.str.num_of_integers_in_TSE_valid = 1;
	file_header.str.signed_bits_in_TSE_valid = 1;
	file_header.str.num_of_bits_in_TSE_valid = 1;
	file_header.str.bit_fields_in_TSE_valid = 1;
	file_header.str.num_of_integers_in_NTSE_valid = 1;
	file_header.str.signed_bits_in_NTSE_valid = 1;
	file_header.str.num_of_bits_in_NTSE_valid = 1;
	file_header.str.bit_fields_in_NTSE_valid = 1;
	tree_data = (unsigned short *)malloc(file_header.str.num_of_NTSE[0]*2);
	if (tree_data == NULL)
	{	fprintf(stderr, "insufficient memory\n");
		vol_exit(1);
	}
	tree_data[0] = (max_z-min_z+1);
	for (j=1; j<=(max_z-min_z+1); j++)
		tree_data[j] = obj_data.rows;
	for (j=0; j<obj_data.rows*(max_z-min_z+1); j++)
		tree_data[1+(max_z-min_z+1)+j] =
			(obj_data.ptr_table[j+1]-obj_data.ptr_table[j])/3;
	shell_file = fopen(file_header.gen.filename, "w+b");
	if (shell_file == NULL)
	{	fprintf(stderr, "can't open output file.\n");
		vol_exit(1);
	}
	strcpy(file_header.gen.recognition_code, "VIEWNIX1.0");
	file_header.gen.data_type = SHELL0;
	file_header.gen.data_type_valid = 1;
	file_header.gen.recognition_code_valid = 1;
	error_code = VWriteHeader(shell_file, &file_header, bad_group,
		bad_element);
	if (error_code != 0)
	{	fprintf(stderr, "error %d in VWriteHeader: Group %s element %s\n",
			error_code, bad_group, bad_element);
		if (error_code!=106 && error_code!=107)
			vol_exit(1);
	}
	rewind(tempfile);
	VSeekData(shell_file, 0);
	error_code = VWriteData(tree_data, 2, file_header.str.num_of_NTSE[0],
		shell_file, &items_written);
	if (error_code==0 && items_written!=file_header.str.num_of_NTSE[0])
		error_code = 3;
	if (error_code)
	{	fprintf(stderr, "error %d in VWriteData\n", error_code);
		vol_exit(1);
	}

	if (voxel_data)
	{	if (fread(voxel_data, 2, last_out_ptr, tempfile) !=
				last_out_ptr)
		{	(void)fprintf(stderr, "Read error\n");
			vol_exit(1);
		}
		for (this_ptr=voxel_data;
				this_ptr<voxel_data+3*file_header.str.num_of_TSE[0];
				this_ptr+=3)
		{	this_gcode = this_ptr[1] & 0x7ff;
			this_tt = this_ptr[1] & 0xf800;
			if (this_gcode >= G_CODES)
			{	double gx, gy, gz;

				neighbors = (this_gcode-G_CODES)<<10;
				gx = ((neighbors&PX)==0)-((neighbors&NX)==0);
				gy = ((neighbors&PY)==0)-((neighbors&NY)==0);
				gz = ((neighbors&PZ)==0)-((neighbors&NZ)==0);
				this_gcode = G_code(gx, gy, gz);
			}
			this_ptr[1] = this_gcode | this_tt;
			if (nbits < 4)
				this_ptr[1] &= 0xffee;
		}
		error_code = VWriteData(voxel_data, 2, 3*file_header.str.num_of_TSE[0],
			shell_file, &items_written);
		if (error_code==0 && items_written!=3*file_header.str.num_of_TSE[0])
			error_code = 3;
		if (error_code)
		{	fprintf(stderr, "error %d in VWriteData\n", error_code);
			vol_exit(1);
		}
	}
	else
		for (j=0; j<file_header.str.num_of_TSE[0]; j++)
		{	if (fread(voxel_datum, 2, 3, tempfile) != 3)
			{	(void)fprintf(stderr, "Read error\n");
				vol_exit(1);
			}
			this_gcode = voxel_datum[1] & 0x7ff;
			this_tt = voxel_datum[1] & 0xf800;
			if (this_gcode >= G_CODES)
			{	double gx, gy, gz;

				neighbors = (this_gcode-G_CODES)<<10;
				gx = ((neighbors&PX)==0)-((neighbors&NX)==0);
				gy = ((neighbors&PY)==0)-((neighbors&NY)==0);
				gz = ((neighbors&PZ)==0)-((neighbors&NZ)==0);
				this_gcode = G_code(gx, gy, gz);
			}
			voxel_datum[1] = this_gcode | this_tt;
			if (nbits < 4)
				voxel_datum[1] &= 0xffee;
			error_code =
				VWriteData(voxel_datum, 2, 3, shell_file, &items_written);
			if (error_code==0 && items_written!=3)
				error_code = 3;
			if (error_code)
			{	fprintf(stderr, "error %d in VWriteData\n", error_code);
				vol_exit(1);
			}
		}

	error_code = VCloseData(shell_file);
	if (error_code)
	{	fprintf(stderr, "error %d in VCloseData\n", error_code);
		vol_exit(1);
	}
}

/*****************************************************************************
 * FUNCTION: vol_exit
 * DESCRIPTION: Removes the temporary file and exits the program.
 * PARAMETERS:
 *    code: If non-zero, will abort program, dumping core; otherwise exit
 *       with zero.
 * SIDE EFFECTS: None.
 * ENTRY CONDITIONS: The global variables tempfile, tempfile_name must be
 *    appropriately initialized.  The signal must not be caught or ignored.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Exits the program.
 * HISTORY:
 *    Created: 11/9/93 by Dewey Odhner
 *
 *****************************************************************************/
vol_exit(code)
	int code;
{
	(void)fclose(tempfile);
	(void)unlink(tempfile_name);
	if (code)
		kill(getpid(), 3);
	exit(0);
}
