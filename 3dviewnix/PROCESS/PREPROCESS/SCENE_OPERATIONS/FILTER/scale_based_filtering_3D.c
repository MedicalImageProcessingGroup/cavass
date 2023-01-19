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

/*****************************************************************************
 * Title: scale_based_filtering_3D
 * Created: 1998 by Punam K Saha
 * Modified: February, 2004 by Punam K Saha
 * Function: Spherical-scale-based anisotropic fltering in three dimentions
 * Assumptions: All parameters are properly specified and the input file is readable
 * Parameters: homogeneity
 * Other effects: none
 * Output: FilteredFile ScaleFile 
 *****************************************************************************/

#include <math.h>
#include <string.h>
#include <cv3dv.h>
#include <assert.h>

#define DEFAULT_MAX_SCALE 10 /* Minimum value 2 i.e., without scale */
#define MIN_SCALE 1        /* Minimum value 1 */
#define MIN_THR -1
#define OBJECT_FRACTION_THRESHOLD 13.0
#define MEDIAN_LEVEL 13

#define Handle_error(message) \
{ \
  printf(message); \
  fflush(stdout); \
  exit(1); \
}

int compute_filter_image(int), compute_feature_scale(), get_max(int, int),
 get_min(int, int);


int             max_scale=DEFAULT_MAX_SCALE;
ViewnixHeader   vh;
int             smallest_density_value, largest_density_value;
double          sigma;
unsigned short ***pimage_16;
unsigned char ***pimage_8;
int             pslice, prow, pcol;
unsigned short *feature_scale, ***pfeature_scale;
int            *sphere_no_points, (**sphere_points)[3];
unsigned short ***pfeature_data_filter;
unsigned short *feature_data_filter;
unsigned short *slice_data_16;
unsigned char ***pfeature_data_filt_8, *feature_data_filt_8, *slice_data_8;
double         *transformation_scale;
long            histogram_count;
int             slice_size, size, bytes;
char           *scale_file_name;
char            group[6], element[6];
int             error;
/*****************************************************************************/
/*    Modified: 2/26/04 8-bit scenes handled by Dewey Odhner. */
int main(argc, argv)
    int             argc;
    char           *argv[];
{
    int             i, j, k, l, tti1, tti2,
                 ***ppptti1;
    FILE           *in, *out;
    int             slice, row;
    double          inv_sigma;
    double          tt1, sphere_slice_coefficient, sphere_row_coefficient, sphere_column_coefficient;



	if (argc==6 && sscanf(argv[5], "%d", &max_scale)==1 && max_scale>=2)
		argc = 5;
    if (argc != 5 || sscanf(argv[4], "%lf", &sigma) != 1) {
        printf("Usage: %s image_file output_file scale_file homogeneity [max_scale]\n", argv[0]);
        exit(-1);
    }

    scale_file_name = argv[3];
    in = fopen(argv[1], "rb");
    if (in == NULL)
        Handle_error("Couldn't open input file\n");

    error = VReadHeader(in, &vh, group, element);
    if (error > 0 && error <= 104)
        Handle_error("Fatal error in reading header\n");

    if (vh.gen.data_type != IMAGE0)
        Handle_error("This is not an IMAGE0 file\n");


    bytes = vh.scn.num_of_bits / 8;
    if (vh.scn.num_of_bits!=16 && vh.scn.num_of_bits!=8)
        Handle_error("This program only handles 8- and 16-bit images\n");

    if (!vh.scn.num_of_subscenes_valid)
        Handle_error("Number of subscenes is not valid!\n");

    if (vh.scn.num_of_subscenes[0] == 1 || vh.scn.dimension != 3)
        Handle_error("This program handles only 3D scenes!\n");

    if (!vh.scn.xypixsz_valid)
        Handle_error("Resolution of this scenes is not valid!\n");

    smallest_density_value = vh.scn.smallest_density_value_valid ?
        (int)vh.scn.smallest_density_value[0] : 0;
    largest_density_value = vh.scn.largest_density_value_valid ?
        (int)vh.scn.largest_density_value[0] : (1 << vh.scn.num_of_bits) - 1;

    pcol = vh.scn.xysize[0];
    prow = vh.scn.xysize[1];
    pslice = vh.scn.num_of_subscenes[0];
    slice_size = vh.scn.xysize[0] * vh.scn.xysize[1];
    size = pslice * slice_size;
    if (bytes == 2)
	{
		slice_data_16 = (unsigned short *) malloc(size * bytes);
	    if (slice_data_16 == NULL)
	        Handle_error("Couldn't allocate memory (execution terminated)\n");

	    pimage_16 = (unsigned short ***) malloc(pslice * sizeof(unsigned short **));
	    if (pimage_16 == NULL)
	        Handle_error("COULD ALLOCATE MEMORY\n");

	    pimage_16[0] = (unsigned short **) malloc(pslice * prow * sizeof(unsigned short *));
	    if (pimage_16[0] == NULL)
	        Handle_error("COULD ALLOCATE MEMORY\n");
	    for (slice = 0; slice < pslice; slice++)
	        pimage_16[slice] = pimage_16[0] + slice * prow;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            pimage_16[slice][row] = slice_data_16 + (slice * prow + row) * pcol;
	}
	else
	{
		slice_data_8 = (unsigned char *) malloc(size * bytes);
	    if (slice_data_8 == NULL)
	        Handle_error("Couldn't allocate memory (execution terminated)\n");

	    pimage_8 = (unsigned char ***) malloc(pslice * sizeof(unsigned char **));
	    if (pimage_8 == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    pimage_8[0] = (unsigned char **) malloc(pslice * prow * sizeof(unsigned char *));
	    if (pimage_8[0] == NULL)
	        Handle_error("Couldn't allocate memory\n");
	    for (slice = 0; slice < pslice; slice++)
	        pimage_8[slice] = pimage_8[0] + slice * prow;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            pimage_8[slice][row] = slice_data_8 + (slice * prow + row) * pcol;
	}

    VSeekData(in, 0);
    if (VReadData(bytes==2?(void *)slice_data_16:(void *)slice_data_8, bytes,
		    size, in, &j)!=0 || j!=size)  {
	    if(j!=0) {
           printf("Could read %d voxels out of %d\n", j, size);
		   fflush(stdout);
		   exit(-1);
		   }
		else   
           Handle_error("Could not read data\n");
		}
    fclose(in);

    if (bytes == 2)
	{
		largest_density_value = pimage_16[0][0][0];
	    for (i = 0; i < pslice; i++)
	        for (j = 0; j < prow; j++)
	            for (k = 0; k < pcol; k++)
	                if (((int) pimage_16[i][j][k]) > largest_density_value) {
	                    largest_density_value = (int) pimage_16[i][j][k];
	                }
    }
	else
	{
		largest_density_value = pimage_8[0][0][0];
	    for (i = 0; i < pslice; i++)
	        for (j = 0; j < prow; j++)
	            for (k = 0; k < pcol; k++)
	                if (((int) pimage_8[i][j][k]) > largest_density_value) {
	                    largest_density_value = (int) pimage_8[i][j][k];
	                }
    }
	if (largest_density_value <= 0)
        Handle_error("Empty image!\n");
    /******* Computation of transformation functions ***********************/
    transformation_scale = (double *) malloc((largest_density_value + 1) *
                         sizeof(double));
    if (transformation_scale == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    inv_sigma = -0.5 / pow(sigma, 2.0);

    for (i = 0; i <= largest_density_value; i++)
        transformation_scale[i] = exp(inv_sigma *
                          pow((double) i, 2.0));
    /***********************************************/
    ppptti1 = (int ***) malloc(2 * (max_scale + 5) * sizeof(int **));
    if (ppptti1 == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    ppptti1[0] = (int **) malloc(2 * (max_scale + 5) * 2 * (max_scale + 5) * sizeof(int *));
    if (ppptti1[0] == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    for (i = 0; i < 2 * (max_scale + 5); i++)
        ppptti1[i] = ppptti1[0] + i * 2 * (max_scale + 5);
    ppptti1[0][0] = (int *) malloc(2 * (max_scale + 5) *
           2 * (max_scale + 5) * 2 * (max_scale + 5) * sizeof(int));
    if (ppptti1[0][0] == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    for (i = 0; i < 2 * (max_scale + 5); i++)
        for (j = 0; j < 2 * (max_scale + 5); j++)
            ppptti1[i][j] = ppptti1[0][0] +
                (i * 2 * (max_scale + 5) + j) * 2 * (max_scale + 5);

    tti1 = max_scale + 5;
    for (i = 0; i < 2 * (max_scale + 5); i++)
        for (j = 0; j < 2 * (max_scale + 5); j++)
            for (k = 0; k < 2 * (max_scale + 5); k++)
                ppptti1[i][j][k] = 0;

    sphere_no_points = (int *) malloc(max_scale * sizeof(int));
    if (sphere_no_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    sphere_points = (void *) malloc(max_scale * sizeof(void *));
    if (sphere_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");


    sphere_column_coefficient = vh.scn.xypixsz[0];
    sphere_row_coefficient = vh.scn.xypixsz[1];
    sphere_slice_coefficient = vh.scn.loc_of_subscenes[1] - vh.scn.loc_of_subscenes[0];
    if (sphere_slice_coefficient < 0.0)
        sphere_slice_coefficient = -sphere_slice_coefficient;
    tt1 = sphere_column_coefficient;
    if (tt1 > sphere_row_coefficient)
        tt1 = sphere_row_coefficient;
    if (tt1 > sphere_slice_coefficient)
        tt1 = sphere_slice_coefficient;
    sphere_column_coefficient = sphere_column_coefficient / tt1;
    sphere_row_coefficient = sphere_row_coefficient / tt1;
    sphere_slice_coefficient = sphere_slice_coefficient / tt1;
	
	printf("Anisotropy: slice = %f, row = %f, column = %f\n",
	        sphere_slice_coefficient,sphere_row_coefficient,sphere_column_coefficient);

    for (k = 0; k < max_scale; k++) {
printf("Computing sphere  %d, ", k);
fflush(stdout);	
        sphere_no_points[k] = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                for (l = -k - 2; l <= k + 2; l++) 
				  if (ppptti1[tti1 + i][tti1 + j][tti1 + l]==0)  {
                    tt1 = sqrt(pow(((double) i) * sphere_slice_coefficient, 2.0)
                           + pow(((double) j) * sphere_row_coefficient, 2.0)
                           + pow(((double) l) * sphere_column_coefficient, 2.0));
                    if (tt1 <= ((double) k) + 0.5) {
                        sphere_no_points[k] = sphere_no_points[k] + 1;
                        ppptti1[tti1 + i][tti1 + j][tti1 + l] = 2;
                    }
                }

printf("number voxels = %d\n", sphere_no_points[k]);
fflush(stdout);	

        sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));
        if (sphere_points[k] == NULL)
            Handle_error("Couldn't allocate memory (execution terminated)\n");

        tti2 = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                for (l = -k - 2; l <= k + 2; l++)
                    if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 2) {
                        ppptti1[tti1 + i][tti1 + j][tti1 + l] = 1;
                        sphere_points[k][tti2][0] = i;
                        sphere_points[k][tti2][1] = j;
                        sphere_points[k][tti2][2] = l;
                        tti2 = tti2 + 1;
                    }
    }
printf("\n");
fflush(stdout);	

    /***********************************************/
    feature_data_filter = NULL;
	feature_data_filt_8 = NULL;
    feature_scale = NULL;
    compute_filter_image(strcmp(argv[2], "/dev/null") == 0);

	if (bytes == 2)
	{
	    largest_density_value = pfeature_data_filter[0][0][0];
	    for (i = 0; i < pslice; i++)
	        for (j = 0; j < prow; j++)
	            for (k = 0; k < pcol; k++)
	                if (((int) pfeature_data_filter[i][j][k]) > largest_density_value) {
	                    largest_density_value = (int) pfeature_data_filter[i][j][k];
	                }
	    smallest_density_value = pfeature_data_filter[0][0][0];
	    for (i = 0; i < pslice; i++)
	        for (j = 0; j < prow; j++)
	            for (k = 0; k < pcol; k++)
	                if (((int) pfeature_data_filter[i][j][k]) < smallest_density_value)
	                    smallest_density_value = (int) pfeature_data_filter[i][j][k];
	}
	else
	{
	    largest_density_value = pfeature_data_filt_8[0][0][0];
	    for (i = 0; i < pslice; i++)
	        for (j = 0; j < prow; j++)
	            for (k = 0; k < pcol; k++)
	                if (((int) pfeature_data_filt_8[i][j][k]) > largest_density_value) {
	                    largest_density_value = (int) pfeature_data_filt_8[i][j][k];
	                }
	    smallest_density_value = pfeature_data_filt_8[0][0][0];
	    for (i = 0; i < pslice; i++)
	        for (j = 0; j < prow; j++)
	            for (k = 0; k < pcol; k++)
	                if (((int) pfeature_data_filt_8[i][j][k]) < smallest_density_value)
	                    smallest_density_value = (int) pfeature_data_filt_8[i][j][k];
	}

    /************ WRITE FILTER IMAGE ************/
    vh.scn.smallest_density_value[0] = (float)smallest_density_value;
    vh.scn.largest_density_value[0] = (float)largest_density_value;
    out = fopen(argv[2], "w+b");
    if (out == NULL)
        Handle_error("Couldn't open output file\n");

    error = VWriteHeader(out, &vh, group, element);
    if (error <= 104)
        Handle_error("Fatal error in writing header\n");

    if (VWriteData(bytes==2?(void *)feature_data_filter:(void *)feature_data_filt_8, bytes, size, out, &j)!=0 || j!=size)
        Handle_error("Could not read data\n");

    fclose(out);

    /***********************************************************/



    exit(0);
}
/*****************************************************************************
 * FUNCTION: compute_affinity_3d
 * DESCRIPTION: Computes the affinity values for the current slice and
 *    stores them at affinity_data_across and affinity_data_down.
 * PARAMETERS: None
 * SIDE EFFECTS: The variable affinity_data_valid set on success.  The variable
 *    feature_status may be changed.  An error message may be displayed.
 * ENTRY CONDITIONS: The variables vh, feature_data_valid, feature_status,
 *    argv0, windows_open, affinity_type, histogram_bins,
 *    histogram_counts, reverse_histogram_counts, largest_density_value,
 *    det_covariance, det_reverse_covariance, training_mean, inv_covariance,
 *    reverse_training_mean, inv_reverse_covariance must be properly set.
 *    The input file must not change from one call to another.
 *    If affinity_data_across or affinity_data_down is non-null, sufficient
 *    space must be allocated there.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Fails if histogram_counts is NULL.
 * HISTORY:
 *    Created: 3/7/96 by Dewey Odhner
 *    Modified: 4/2/96 threshold applied by Dewey Odhner
 *    Modified: 5/22/96 histogram used by Dewey Odhner
 *    Modified: 5/28/96 reverse training allowed by Dewey Odhner
 *    Modified: 7/25/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 2/18/97 threshold applied covariance affinity type
 *       by Dewey Odhner
 *    Modified: 4/16/97 threshold not applied by Dewey Odhner
 *
 *****************************************************************************/
/*    Modified: 2/25/04 8-bit scenes handled by Dewey Odhner. */
/*    Modified: 2/26/04 scale_only flag added by Dewey Odhner. */
int compute_filter_image(int scale_only)
{
    int             slice, row, col, i, k, tti1, iscale, xx, yy, zz, x,
                    y, z;
    unsigned short ***outptr, ***pin_16, ***pscale;
	unsigned char ***outptr_8, ***pin_8;
    double          tt1, count, sum, temp_sum, inv_k, inv_half_scale;




	if (bytes == 2)
	{
	    feature_data_filter = (unsigned short *) malloc(size * sizeof(unsigned short));
	    if (feature_data_filter == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    pfeature_data_filter = (unsigned short ***) malloc(pslice * sizeof(unsigned short **));
	    if (pfeature_data_filter == NULL)
	        Handle_error("COULD ALLOCATE MEMORY\n");

	    pfeature_data_filter[0] = (unsigned short **) malloc(pslice * prow * sizeof(unsigned short *));
	    if (pfeature_data_filter[0] == NULL)
	        Handle_error("COULD ALLOCATE MEMORY\n");
	    for (slice = 0; slice < pslice; slice++)
	        pfeature_data_filter[slice] = pfeature_data_filter[0] + slice * prow;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            pfeature_data_filter[slice][row] = feature_data_filter
	                + (slice * prow + row) * pcol;
	}
	else
	{
	    feature_data_filt_8 = (unsigned char *) malloc(size);
	    if (feature_data_filt_8 == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    pfeature_data_filt_8 = (unsigned char ***) malloc(pslice * sizeof(unsigned char **));
	    if (pfeature_data_filt_8 == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    pfeature_data_filt_8[0] = (unsigned char **) malloc(pslice * prow * sizeof(unsigned char *));
	    if (pfeature_data_filt_8[0] == NULL)
	        Handle_error("Couldn't allocate memory\n");
	    for (slice = 0; slice < pslice; slice++)
	        pfeature_data_filt_8[slice] = pfeature_data_filt_8[0] + slice * prow;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            pfeature_data_filt_8[slice][row] = feature_data_filt_8
	                + (slice * prow + row) * pcol;
	}

    compute_feature_scale();
	if (scale_only)
		exit(0);
    pscale = pfeature_scale;
    pin_16 = pimage_16;
	pin_8 = pimage_8;

    outptr = pfeature_data_filter;
	outptr_8 = pfeature_data_filt_8;
    for (slice = 0; slice < pslice; slice++) {
        printf("\rFiltering, slice %d", slice);
        fflush(stdout);
        for (row = 0; row < prow; row++)
            for (col = 0; col < pcol; col++)
                if ((bytes==2? pin_16[slice][row][col]:
						pin_8[slice][row][col]) > MIN_THR) {
                    sum = 0.0;
                    count = 0.00001;
                    iscale = pscale[slice][row][col];
                    for (k = 0; k < iscale; k++) {
                        temp_sum = 0.0;
                        tt1 = (double) iscale;
                        tt1 = 0.5 * tt1;
                        inv_half_scale = -0.5 / pow(tt1, 2.0);
                        inv_k = exp(inv_half_scale * pow((double) k, 2.0));

                        for (i = 0; i < sphere_no_points[k]; i++) {
                            xx = sphere_points[k][i][0];
                            yy = sphere_points[k][i][1];
                            zz = sphere_points[k][i][2];
                            x = slice + xx;
                            y = row + yy;
                            z = col + zz;
                            if (x >= 0 && x < pslice && 
							    y >= 0 && y < prow && z >= 0 && z < pcol) {
                                tti1= bytes==2? pin_16[x][y][z]:pin_8[x][y][z];
                                temp_sum = temp_sum + tti1;
                                count = count + inv_k;
                            }
                        }
                        sum = sum + temp_sum * inv_k;
                    }
                    if (bytes == 2)
						outptr[slice][row][col] = (unsigned short) (sum / count + 0.5);
					else
						outptr_8[slice][row][col] = (unsigned char) (sum / count + 0.5);
                } else
                    if (bytes == 2)
						outptr[slice][row][col] = pin_16[slice][row][col];
					else
						outptr_8[slice][row][col] = pin_8[slice][row][col];
    }
    printf("\n");
    fflush(stdout);

    return (1);
}

/*****************************************************************************
 * FUNCTION: compute_feature_scale_3d
 * DESCRIPTION: computes scale representation of the image.
 * PARAMETERS:
 *    feature: whether it is an intensity or an edge or a fractal image.
 * SIDE EFFECTS:
 *    feature_scale_data_valid is set.
 * ENTRY CONDITIONS:
 *    sigma, bytes must be set.
 *    image data is valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 10/3/97 by Punam K. Saha
 *    Modified: 3/16/04 8-bit scenes handled by Dewey Odhner.
 *
 *****************************************************************************/
int compute_feature_scale()
{
    int             tti2, tti5;
    ViewnixHeader   vh_scale;
    FILE           *fp_scale;
	short           bit_fields[] = {0, 15};
    float           smallest_density_value_scale[] = {0.0},
                    largest_density_value_scale[] = {(float) max_scale};
    int             slice, row, col, i, j, k, x, y, z, xx, yy, zz, mean_g,
                    tti1, tli1[27];
    unsigned short ***pin_16, *temp_16, ***ptemp_16;
	unsigned char ***pin_8, *temp_8, ***ptemp_8;
    int             flag;
    double          count_obj, count_nonobj;


    feature_scale = (unsigned short *) malloc(size * sizeof(unsigned short));
    if (feature_scale == NULL)
        Handle_error("Couldn't allocate memory\n");

    pfeature_scale = (unsigned short ***) malloc(pslice * sizeof(unsigned short **));
    if (pfeature_scale == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");

    pfeature_scale[0] = (unsigned short **) malloc(pslice * prow * sizeof(unsigned short *));
    if (pfeature_scale[0] == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    for (slice = 0; slice < pslice; slice++)
        pfeature_scale[slice] = pfeature_scale[0] + slice * prow;

    for (slice = 0; slice < pslice; slice++)
        for (row = 0; row < prow; row++)
            pfeature_scale[slice][row] = feature_scale + (slice * prow + row) * pcol;



	if (bytes == 2)
	{
	    temp_16 = (unsigned short *) malloc(size * sizeof(unsigned short));
	    if (temp_16 == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    ptemp_16 = (unsigned short ***) malloc(pslice * sizeof(unsigned short **));
	    if (ptemp_16 == NULL)
	        Handle_error("COULD ALLOCATE MEMORY\n");

	    ptemp_16[0] = (unsigned short **) malloc(pslice * prow * sizeof(unsigned short *));
	    if (ptemp_16[0] == NULL)
	        Handle_error("COULD ALLOCATE MEMORY\n");
	    for (slice = 0; slice < pslice; slice++)
	        ptemp_16[slice] = ptemp_16[0] + slice * prow;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            ptemp_16[slice][row] = temp_16 + (slice * prow + row) * pcol;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            for (col = 0; col < pcol; col++)
	                ptemp_16[slice][row][col] = pimage_16[slice][row][col];
	}
	else
	{
	    temp_8 = (unsigned char *) malloc(size);
	    if (temp_8 == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    ptemp_8 = (unsigned char ***) malloc(pslice * sizeof(unsigned char **));
	    if (ptemp_8 == NULL)
	        Handle_error("Couldn't allocate memory\n");

	    ptemp_8[0] = (unsigned char **) malloc(pslice * prow * sizeof(unsigned char *));
	    if (ptemp_8[0] == NULL)
	        Handle_error("Couldn't allocate memory\n");
	    for (slice = 0; slice < pslice; slice++)
	        ptemp_8[slice] = ptemp_8[0] + slice * prow;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            ptemp_8[slice][row] = temp_8 + (slice * prow + row) * pcol;

	    for (slice = 0; slice < pslice; slice++)
	        for (row = 0; row < prow; row++)
	            for (col = 0; col < pcol; col++)
	                ptemp_8[slice][row][col] = pimage_8[slice][row][col];
	}

    for (slice = 0; slice < pslice; slice++)   {
	    printf("\rComputing mean, slice %d", slice);
		fflush(stdout);
        for (row = 0; row < prow; row++)
            for (col = 0; col < pcol; col++) {
                tti1 = 0;
                for (xx = -1; xx <= 1; xx++)
                    for (yy = -1; yy <= 1; yy++)
                        for (zz = -1; zz <= 1; zz++) {
                            if (xx < 0) {
                                x = get_max(slice + xx, 0);
                            } else {
                                x = get_min(slice + xx, pslice - 1);
                            }
                            if (yy < 0) {
                                y = get_max(row + yy, 0);
                            } else {
                                y = get_min(row + yy, prow - 1);
                            }
                            if (zz < 0) {
                                z = get_max(col + zz, 0);
                            } else {
                                z = get_min(col + zz, pcol - 1);
                            }
                            j = 0;
                            if (bytes == 2)
								while (j < tti1 && ptemp_16[x][y][z] < tli1[j])
                                	j = j + 1;
                            else
							    while (j < tti1 && ptemp_8[x][y][z] < tli1[j])
								    j = j + 1;
							tti2 = j;
                            for (j = tti1 - 1; j >= tti2; j--)
                                tli1[j + 1] = tli1[j];
                            tli1[tti2] =
							    bytes==2? ptemp_16[x][y][z]: ptemp_8[x][y][z];
                            tti1 = tti1 + 1;
                        }
                if (bytes == 2)
				{
					for (j = 0; j < tti1; j++)
	                    if (tli1[j] == pimage_16[slice][row][col]) {
	                        tti2 = j;
	                        tti1 = 27;
	                    }
	                if (tti2 < MEDIAN_LEVEL)
	                    pimage_16[slice][row][col] = tli1[MEDIAN_LEVEL];
	                else if (tti2 > 26-MEDIAN_LEVEL)
	                    pimage_16[slice][row][col] = tli1[26-MEDIAN_LEVEL];
	                else
	                    pimage_16[slice][row][col] = ptemp_16[slice][row][col];
				}
				else
				{
					for (j = 0; j < tti1; j++)
	                    if (tli1[j] == pimage_8[slice][row][col]) {
	                        tti2 = j;
	                        tti1 = 27;
	                    }
	                if (tti2 < MEDIAN_LEVEL)
	                    pimage_8[slice][row][col] = tli1[MEDIAN_LEVEL];
	                else if (tti2 > 26-MEDIAN_LEVEL)
	                    pimage_8[slice][row][col] = tli1[26-MEDIAN_LEVEL];
	                else
	                    pimage_8[slice][row][col] = ptemp_8[slice][row][col];
				}
            }
		}
	printf("\n");
	fflush(stdout);
	if (bytes == 2)
	{
	    free(temp_16);
	    free(ptemp_16[0]);
	    free(ptemp_16);
	}
	else
	{
	    free(temp_8);
	    free(ptemp_8[0]);
	    free(ptemp_8);
	}


    pin_16 = pimage_16;
	pin_8 = pimage_8;
    for (slice = 0; slice < pslice; slice++) {
        printf("\rScale computing slice %d", slice);
        fflush(stdout);
        for (row = 0; row < prow; row++)
            for (col = 0; col < pcol; col++)
                if ((bytes==2? pimage_16[slice][row][col]:
						pimage_8[slice][row][col]) <= MIN_THR)
                    pfeature_scale[slice][row][col] = MIN_SCALE;
                else {
                    flag = 0;
                    mean_g = bytes==2? pimage_16[slice][row][col]:
						pimage_8[slice][row][col];

                    count_obj = 0;
                    count_nonobj = 0;
                    for (k = MIN_SCALE; (k < max_scale) && !flag; k++) {
                        for (i = 0; i < sphere_no_points[k]; i++) {
                            xx = slice + sphere_points[k][i][0];
                            yy = row + sphere_points[k][i][1];
                            zz = col + sphere_points[k][i][2];
                            if (xx >= 0 && xx < pslice && yy >= 0 &&
                                yy < prow && zz >= 0 && zz < pcol &&
                                (bytes==2?pin_16[xx][yy][zz]:pin_8[xx][yy][zz])
								 > MIN_THR) {
                                tti5 = (bytes==2? pin_16[xx][yy][zz]:
									pin_8[xx][yy][zz]) - mean_g;
                                if (tti5 < 0)
                                    tti5 = -tti5;
                                count_obj = count_obj + transformation_scale[tti5];
                                count_nonobj = count_nonobj + 1.0 - transformation_scale[tti5];
                            }
                        }

                        if (100.0 * count_nonobj >= OBJECT_FRACTION_THRESHOLD
                            * (count_nonobj + count_obj)) {
                            pfeature_scale[slice][row][col] = k;
                            flag = 1;
                        }
                    }
                    if (!flag)
                        pfeature_scale[slice][row][col] = max_scale - 1;
                }
    }
    printf("\n");
    fflush(stdout);
    /************ WRITE SCALE IMAGE ************/
	if (strcmp(scale_file_name, "/dev/null"))
	{
	    vh_scale = vh;
		vh_scale.scn.num_of_bits = 16;
		vh_scale.scn.bit_fields = bit_fields;
	    vh_scale.scn.smallest_density_value = smallest_density_value_scale;
	    vh_scale.scn.largest_density_value = largest_density_value_scale;
	    fp_scale = fopen(scale_file_name, "w+b");
		if (fp_scale == NULL)
		{
			fprintf(stderr,"Could not open %s for writing.\n",scale_file_name);
			return (1);
		}
	    error = VWriteHeader(fp_scale, &vh_scale, group, element);
	    if (error && error <= 104)
	        Handle_error("Fatal error in writing header\n");

	    if (VWriteData((char *)feature_scale, 2, size, fp_scale, &j)!=0 ||
		        j!=size)
	        Handle_error("Could not write data\n");

	    fclose(fp_scale);
	}

    /***********************************************************/

    return (1);
}
/*****************************************************************************
     * FUNCTION: get_max
     * DESCRIPTION: returns the maximum between two integers
     * PARAMETERS:
     *    integer1 and integer2
     * SIDE EFFECTS:
     *    Nil.
     * ENTRY CONDITIONS:
     *    Nil.
     * RETURN VALUE: see DESCRIPTION
     * EXIT CONDITIONS:
     * HISTORY:
     *    Created: 9/5/97 by Punam K. Saha
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
 *    integer1 and integer2
 * SIDE EFFECTS:
 *    Nil.
 * ENTRY CONDITIONS:
 *    Nil.
 * RETURN VALUE: see DESCRIPTION
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 9/5/97 by Punam K. Saha
 *
 *****************************************************************************/
int get_min(int a, int b)
{

    if (a < b)
        return (a);
    else
        return (b);
}
/*******************************  THE END  *************************************/
