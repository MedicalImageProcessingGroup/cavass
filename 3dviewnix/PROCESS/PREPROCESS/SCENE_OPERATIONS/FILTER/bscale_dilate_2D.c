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
#include <cv3dv.h>
#include <assert.h>

#define Handle_error(message) \
{ \
  fprintf(stderr, message); \
  fflush(stderr); \
  exit(1); \
}

void compute_filter_image();

int             max_scale;
ViewnixHeader   vh;
unsigned short **pimage_16;
unsigned char **pimage_8;
int             cslice, prow, pcol;
int            *circle_no_points, (**circle_points)[2];
unsigned short *slice_data_16;
unsigned char **pout_data, *out_data, *slice_data_8, *out_data_1;
int             size, bytes;
/*******************************************************************/
/*    Modified: 2/27/04 8-bit scenes handled by Dewey Odhner. */
int main(argc, argv)
    int             argc;
    char           *argv[];
{
    int             i, j, k, tti1, tti2, error, **pptti1;
    FILE           *in, *out;
    char            group[6], elem[6];
	ViewnixHeader   outvh;
	double          tt1, circle_row_coefficient, circle_column_coefficient;

    if (argc != 3) {
        fprintf(stderr, "Usage: bscale_dilate_2D input_file output_file\n");
        exit(-1);
    }

    in = fopen(argv[1], "rb");
    if (in == NULL)
        Handle_error("Couldn't open input file\n");

    error = VReadHeader(in, &vh, group, elem);
    if (error > 0 && error <= 104)
        Handle_error("Fatal error in reading header\n");

    if (vh.gen.data_type != IMAGE0)
        Handle_error("This is not an IMAGE0 file\n");

    bytes = vh.scn.num_of_bits / 8;
    if (vh.scn.num_of_bits!=16 && vh.scn.num_of_bits!=8)
        Handle_error("This program only handles 8- and 16-bit images\n");

    if (!vh.scn.num_of_subscenes_valid)
        Handle_error("Number of subscenes is not valid!\n");

    if (vh.scn.dimension != 3)
        Handle_error("This program handles only 3D scenes!\n");

    if (!vh.scn.xypixsz_valid)
        Handle_error("Resolution of this scenes is not valid!\n");

	if (!vh.scn.largest_density_value_valid)
		Handle_error("Largest density value of this scenes is not valid!\n");
	max_scale = (int)vh.scn.largest_density_value[0];

    pcol = vh.scn.xysize[0];
    prow = vh.scn.xysize[1];
    size = (int)vh.scn.xysize[0] * vh.scn.xysize[1];

	if (bytes == 2)
	{
	    slice_data_16 = (unsigned short *) malloc(size * bytes);
	    if (slice_data_16 == NULL)
	        Handle_error("Couldn't allocate memory (execution terminated)\n");

	    pimage_16 = (unsigned short **) malloc(vh.scn.xysize[1] *
                           sizeof(unsigned short *));
	    if (pimage_16 == NULL)
	        Handle_error("Couldn't allocate memory (execution terminated)\n");

    	for (j = 0; j < vh.scn.xysize[1]; j++)
	        pimage_16[j] = slice_data_16 + j * vh.scn.xysize[0];
	}
	else
	{
	    slice_data_8 = (unsigned char *) malloc(size * bytes);
	    if (slice_data_8 == NULL)
	        Handle_error("Couldn't allocate memory (execution terminated)\n");

	    pimage_8 = (unsigned char **) malloc(vh.scn.xysize[1] *
                           sizeof(unsigned char *));
	    if (pimage_8 == NULL)
	        Handle_error("Couldn't allocate memory (execution terminated)\n");

    	for (j = 0; j < vh.scn.xysize[1]; j++)
	        pimage_8[j] = slice_data_8 + j * vh.scn.xysize[0];
	}

    pptti1 = (int **) malloc(2 * (max_scale + 5) * sizeof(int *));
    if (pptti1 == NULL)
        Handle_error("COULD NOT ALLOCATE MEMORY\n");
    pptti1[0] = (int *) malloc(2 * (max_scale + 5) * 2 * (max_scale + 5) * sizeof(int));
    if (pptti1[0] == NULL)
        Handle_error("COULD NOT ALLOCATE MEMORY\n");
    for (i = 0; i < 2 * (max_scale + 5); i++)
        pptti1[i] = pptti1[0] + i * 2 * (max_scale + 5);
    tti1 = max_scale + 5;
    for (i = 0; i < 2 * (max_scale + 5); i++)
        for (j = 0; j < 2 * (max_scale + 5); j++)
            pptti1[i][j] = 0;

    circle_no_points = (int *) malloc(max_scale * sizeof(int));
    if (circle_no_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    circle_points = (void *) malloc(max_scale * sizeof(void *));
    if (circle_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    if (vh.scn.xypixsz[0] == vh.scn.xypixsz[1]) {
        circle_column_coefficient = 1.0;
        circle_row_coefficient = 1.0;
    } else if (vh.scn.xypixsz[0] > vh.scn.xypixsz[1]) {
        circle_row_coefficient = 1.0;
        circle_column_coefficient = vh.scn.xypixsz[0] / vh.scn.xypixsz[1];
    } else {
        circle_column_coefficient = 1.0;
        circle_row_coefficient = vh.scn.xypixsz[1] / vh.scn.xypixsz[0];
    }

	printf("Anisotropy: row = %f, column = %f\n",
	        circle_row_coefficient,circle_column_coefficient);

    pout_data = (unsigned char **)malloc(prow * sizeof(unsigned char *));
    if (pout_data == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    out_data = (unsigned char *) malloc(size);
	out_data_1 = (unsigned char *) malloc((size+7)/8);
    if (out_data == NULL || out_data_1 == NULL)
        Handle_error("Couldn't allocate memory\n");

    for (i = 0; i < prow; i++)
        pout_data[i] =
            out_data + i * pcol;

    for (k = 0; k < max_scale; k++) {

        circle_no_points[k] = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++) 
			  if (pptti1[tti1 + i][tti1 + j]==0)  {
                tt1 = sqrt(pow(((double) i) * circle_row_coefficient, 2.0)
                       + pow(((double) j) * circle_column_coefficient, 2.0));
                if (tt1 <= ((double) k) + 0.5) {
                    circle_no_points[k] = circle_no_points[k] + 1;
                    pptti1[tti1 + i][tti1 + j] = 2;
                }
            }

        circle_points[k] = (void *) malloc(2 * circle_no_points[k] * sizeof(int));
        if (circle_points[k] == NULL)
            Handle_error("Couldn't allocate memory (execution terminated)\n");

        tti2 = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                if (pptti1[tti1 + i][tti1 + j] == 2) {
                    pptti1[tti1 + i][tti1 + j] = 1;
                    circle_points[k][tti2][0] = j;
                    circle_points[k][tti2][1] = i;
                    tti2 = tti2 + 1;
                }
    }

    /***********************************************/
    error = VSeekData(in, 0);
	if (error && error <= 104)
	    Handle_error("Seek error\n");
    out = fopen(argv[2], "w+b");
    if (out == NULL)
        Handle_error("Couldn't open output file\n");

	outvh = vh;
	outvh.gen.filename_valid = 0;
	outvh.gen.filename1_valid = 0;
	outvh.scn.num_of_bits = 1;
	outvh.scn.bit_fields[1] = 0;
	if (vh.scn.smallest_density_value_valid)
		outvh.scn.smallest_density_value[0] = 0;
	outvh.scn.largest_density_value[0] = 1;
	outvh.scn.description_valid = 0;
    error = VWriteHeader(out, &outvh, group, elem);
    if (error && error <= 104)
        Handle_error("Fatal error in writing header\n");
    for (cslice=0; cslice<vh.scn.num_of_subscenes[0]; cslice++)
	{
		if (VReadData(bytes==2? (char *)slice_data_16:(char *)slice_data_8,
			    bytes, size, in, &j)!=0 || j!=size)   {
		    if(j!=0) {
	           printf("Could read %d voxels out of %d\n", j, size);
			   fflush(stdout);
			   exit(-1);
			   }
			else   
	           Handle_error("Could not read data\n");
			}    

	    compute_filter_image();


	    /************ WRITE FILTER IMAGE ************/

		VPackByteToBit(out_data, size, out_data_1);
	    if (VWriteData((char *)out_data_1, 1, (size+7)/8, out, &j) != 0)
	        Handle_error("Could not write data\n");

    }
	fclose(out);

    /***********************************************************/

    printf("\n");

    exit(0);
}

void compute_filter_image()
{
    int             row, col, i, k, iscale, xx, yy, x, y;
    unsigned short **pin_16;
	unsigned char **outptr_8, **pin_8;


    pin_16 = pimage_16;
	pin_8 = pimage_8;
	memset(out_data, 0, size);
	outptr_8 = pout_data;
    for (row = 0; row < vh.scn.xysize[1]; row++) {
        for (col = 0; col < vh.scn.xysize[0]; col++) {
            iscale = bytes==2? pin_16[row][col]:pin_8[row][col];
			assert(iscale <= max_scale);
            for (k = 0; k < iscale; k++)
                for (i = 0; i < circle_no_points[k]; i++) {
                    xx = circle_points[k][i][0];
                    yy = circle_points[k][i][1];
                    x = col + xx;
                    y = row + yy;
                    if (x >= 0 && x < pcol && y >= 0 && y < prow)
						outptr_8[y][x] = 255;
                }
		}
    }

}
