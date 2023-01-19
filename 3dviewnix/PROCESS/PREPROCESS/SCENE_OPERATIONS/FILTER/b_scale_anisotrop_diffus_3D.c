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
 * Title: scale_based_filtering_3D_anisotropy
 * Created: 1998 by Punam K Saha
 * Modified: February, 2004 by Punam K Saha
 * Function: Spherical-scale-based anisotropic fltering in three dimentions
 * Assumptions: All parameters are properly specified and the input file is readable
 * Parameters: homogeneity and number of iterations
 * Other effects: none
 * Output: FilteredFile ScaleFile 
 *****************************************************************************/

#include <math.h>
#include <string.h>
#include <cv3dv.h>
#include <assert.h>

#define MAX_SCALE 8
#define MIN_SCALE 1        /* Minimum value 1 */
#define MIN_THR -1
#define OBJECT_FRACTION_THRESHOLD 13.0
#define MEDIAN_LEVEL 2
double          DIFFUSION_CONSTANT;

#define Handle_error(message) \
{ \
  printf(message); \
  fflush(stdout); \
  exit(1); \
}

int compute_feature_scale(),
	compute_filter_image(), get_max(int a, int b), get_min(int a, int b);

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
double         **transformation;
long            iteration, histogram_count;
int             max_iteration, slice_size, size, bytes;
char            group[6], element[6];
int             error;
double  anisotropy_slice_coefficient, anisotropy_row_coefficient, anisotropy_column_coefficient;
/*****************************************************************************/
/*    Modified: 3/18/04 8-bit scenes handled by Dewey Odhner. */
int main(argc, argv)
    int             argc;
    char           *argv[];
{
    int             i, j, k, l, tti1, tti2,
                 ***ppptti1;
    FILE           *in, *out;
    int             slice, row;
    double          inv_sigma;
    double          tt1;



    if (argc != 5 || sscanf(argv[3], "%lf", &sigma) != 1
        || sscanf(argv[4], "%d", &max_iteration) != 1) {
        printf("Usage: %s image_file output_file homogeneity iteration\n", argv[0]);
        exit(-1);
    }

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

    smallest_density_value = (int)(vh.scn.smallest_density_value_valid ?
        vh.scn.smallest_density_value[0] : 0);
    largest_density_value = (int)(vh.scn.largest_density_value_valid ?
        vh.scn.largest_density_value[0] : (1 << vh.scn.num_of_bits) - 1);

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
    if (VReadData(bytes==2?(char *)slice_data_16:(char *)slice_data_8, bytes,
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
    /******* Computation of transformation functions ***********************/
    DIFFUSION_CONSTANT = 1.0 / 7.0;
    transformation = (double **) malloc((MAX_SCALE + 1) *
                       sizeof(double *));
    if (transformation == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");
    transformation[0] = (double *) malloc((MAX_SCALE + 1) * (largest_density_value + 1) *
                       sizeof(double));
    if (transformation[0] == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");
for(i=0;i<MAX_SCALE + 1;i++)
transformation[i]=transformation[0]+i*(largest_density_value + 1);
for(i=0;i<MAX_SCALE + 1;i++)  {
    inv_sigma = -0.5 / pow(((double)i*sigma)/(double) MAX_SCALE, 2.0);
    for (j = 0; j <= largest_density_value; j++)
        transformation[i][j] = DIFFUSION_CONSTANT * exp(inv_sigma*pow((double) j, 2.0));
			  }
    /***********************************************/
    ppptti1 = (int ***) malloc(2 * (MAX_SCALE + 5) * sizeof(int **));
    if (ppptti1 == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    ppptti1[0] = (int **) malloc(2 * (MAX_SCALE + 5) * 2 * (MAX_SCALE + 5) * sizeof(int *));
    if (ppptti1[0] == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    for (i = 0; i < 2 * (MAX_SCALE + 5); i++)
        ppptti1[i] = ppptti1[0] + i * 2 * (MAX_SCALE + 5);
    ppptti1[0][0] = (int *) malloc(2 * (MAX_SCALE + 5) *
           2 * (MAX_SCALE + 5) * 2 * (MAX_SCALE + 5) * sizeof(int));
    if (ppptti1[0][0] == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    for (i = 0; i < 2 * (MAX_SCALE + 5); i++)
        for (j = 0; j < 2 * (MAX_SCALE + 5); j++)
            ppptti1[i][j] = ppptti1[0][0] +
                (i * 2 * (MAX_SCALE + 5) + j) * 2 * (MAX_SCALE + 5);

    tti1 = MAX_SCALE + 5;
    for (i = 0; i < 2 * (MAX_SCALE + 5); i++)
        for (j = 0; j < 2 * (MAX_SCALE + 5); j++)
            for (k = 0; k < 2 * (MAX_SCALE + 5); k++)
                ppptti1[i][j][k] = 0;

    sphere_no_points = (int *) malloc(MAX_SCALE * sizeof(int));
    if (sphere_no_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    sphere_points = (void *) malloc(MAX_SCALE * sizeof(void *));
    if (sphere_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");


    anisotropy_column_coefficient = vh.scn.xypixsz[0];
    anisotropy_row_coefficient = vh.scn.xypixsz[1];
    anisotropy_slice_coefficient = vh.scn.loc_of_subscenes[1] - vh.scn.loc_of_subscenes[0];
    if (anisotropy_slice_coefficient < 0.0)
        anisotropy_slice_coefficient = -anisotropy_slice_coefficient;
    tt1 = anisotropy_column_coefficient;
    if (tt1 > anisotropy_row_coefficient)
        tt1 = anisotropy_row_coefficient;
    if (tt1 > anisotropy_slice_coefficient)
        tt1 = anisotropy_slice_coefficient;
    anisotropy_column_coefficient = anisotropy_column_coefficient / tt1;
    anisotropy_row_coefficient = anisotropy_row_coefficient / tt1;
    anisotropy_slice_coefficient = anisotropy_slice_coefficient / tt1;
	
	printf("Anisotropy: slice = %f, row = %f, column = %f\n",
	        anisotropy_slice_coefficient,anisotropy_row_coefficient,anisotropy_column_coefficient);

    for (k = 0; k < MAX_SCALE; k++) {
printf("Computing sphere  %d, ", k);
fflush(stdout);	
        sphere_no_points[k] = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                for (l = -k - 2; l <= k + 2; l++) 
				  if (ppptti1[tti1 + i][tti1 + j][tti1 + l]==0)  {
                    tt1 = sqrt(pow(((double) i) * anisotropy_slice_coefficient, 2.0)
                           + pow(((double) j) * anisotropy_row_coefficient, 2.0)
                           + pow(((double) l) * anisotropy_column_coefficient, 2.0));
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
    compute_feature_scale();
    for (iteration = 0; iteration < max_iteration; iteration++) {
        compute_filter_image();
		  for(i = 0; i < pslice; i++)
            for (j = 0; j < prow; j++)
                for (k = 0; k < pcol; k++)
                    if (bytes == 2)
						pimage_16[i][j][k] = pfeature_data_filter[i][j][k];
					else
						pimage_8[i][j][k] = pfeature_data_filt_8[i][j][k];
    }


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

    if (VWriteData(bytes==2?(char *)feature_data_filter:(char *)feature_data_filt_8, bytes, size, out, &j)!=0 || j!=size)
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
/*    Modified: 3/19/04 8-bit scenes handled by Dewey Odhner. */
int compute_filter_image()
{
    int             slice, row, col, tti1, tti2, iscale, selected_scale;
    unsigned short ***outptr, ***pin_16, ***pscale;
	unsigned char ***outptr_8, ***pin_8;
    double          sum;




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
                sum = bytes==2? pin_16[slice][row][col]:pin_8[slice][row][col];
                iscale = (int) pscale[slice][row][col];				
				
                if (slice > 0 && slice < pslice - 1) {
                    tti1 = bytes==2?
						(int) pin_16[slice + 1][row][col] -
						(int) pin_16[slice][row][col]:
						(int) pin_8[slice + 1][row][col] -
						(int) pin_8[slice][row][col];
					
                    tti2 = (int) pscale[slice + 1][row][col];
                    if (tti2 > iscale)
                        selected_scale = iscale;
					else
					    selected_scale = tti2;
						
                    tti2 = (int) pscale[slice - 1][row][col];
                    if (tti2 < selected_scale)
         					    selected_scale = tti2;

                    if (tti1 < 0) {
                        tti1 = -tti1;
                        tti1 = (int) (((double) tti1) / anisotropy_slice_coefficient + 0.5);
                        sum = sum - ((double) tti1) * transformation[selected_scale][tti1];
                    } else {
                        tti1 = (int) (((double) tti1) / anisotropy_slice_coefficient + 0.5);
                        sum = sum + ((double) tti1) * transformation[selected_scale][tti1];
                    }

                    tti1 = bytes==2?
						(int) pin_16[slice - 1][row][col] -
						(int) pin_16[slice][row][col]:
						(int) pin_8[slice - 1][row][col] -
						(int) pin_8[slice][row][col];

                    if (tti1 < 0) {
                        tti1 = -tti1;
                        tti1 = (int) (((double) tti1) / anisotropy_slice_coefficient + 0.5);
                        sum = sum - ((double) tti1) * transformation[selected_scale][tti1];
                    } else {
                        tti1 = (int) (((double) tti1) / anisotropy_slice_coefficient + 0.5);
                        sum = sum + ((double) tti1) * transformation[selected_scale][tti1];
                    }
                }
				
                if (row > 0 && row < prow - 1) {
                    tti1 = bytes==2?
						(int) pin_16[slice][row + 1][col] -
						(int) pin_16[slice][row][col]:
						(int) pin_8[slice][row + 1][col] -
						(int) pin_8[slice][row][col];
					
                    tti2 = (int) pscale[slice][row + 1][col];
                    if (tti2 > iscale)
                        selected_scale = iscale;
					else
					    selected_scale = tti2;
						
                    tti2 = (int) pscale[slice][row - 1][col];
                    if (tti2 < selected_scale)
         					    selected_scale = tti2;

                    if (tti1 < 0) {
                        tti1 = -tti1;
                        tti1 = (int) (((double) tti1) / anisotropy_row_coefficient + 0.5);
                        sum = sum - ((double) tti1) * transformation[selected_scale][tti1];
                    } else {
                        tti1 = (int) (((double) tti1) / anisotropy_row_coefficient + 0.5);
                        sum = sum + ((double) tti1) * transformation[selected_scale][tti1];
                    }

                    tti1 = bytes==2?
						(int) pin_16[slice][row - 1][col] -
						(int) pin_16[slice][row][col]:
						(int) pin_8[slice][row - 1][col] -
						(int) pin_8[slice][row][col];

                    if (tti1 < 0) {
                        tti1 = -tti1;
                        tti1 = (int) (((double) tti1) / anisotropy_row_coefficient + 0.5);
                        sum = sum - ((double) tti1) * transformation[selected_scale][tti1];
                    } else {
                        tti1 = (int) (((double) tti1) / anisotropy_row_coefficient + 0.5);
                        sum = sum + ((double) tti1) * transformation[selected_scale][tti1];
                    }
                }
				
                if (col > 0 && col < pcol - 1) {
                    tti1 = bytes==2?
						(int) pin_16[slice][row][col + 1] -
						(int) pin_16[slice][row][col]:
						(int) pin_8[slice][row][col + 1] -
						(int) pin_8[slice][row][col];
					
                    tti2 = (int) pscale[slice][row][col + 1];
                    if (tti2 > iscale)
                        selected_scale = iscale;
					else
					    selected_scale = tti2;
						
                    tti2 = (int) pscale[slice][row][col - 1];
                    if (tti2 < selected_scale)
					    selected_scale = tti2;

                    if (tti1 < 0) {
                        tti1 = -tti1;
                        tti1 = (int) (((double) tti1) / anisotropy_column_coefficient + 0.5);
                        sum = sum - ((double) tti1) * transformation[selected_scale][tti1];
                    } else {
                        tti1 = (int) (((double) tti1) / anisotropy_column_coefficient + 0.5);
                        sum = sum + ((double) tti1) * transformation[selected_scale][tti1];
                    }

                    tti1 = bytes==2?
						(int) pin_16[slice][row][col - 1] -
						(int) pin_16[slice][row][col]:
						(int) pin_8[slice][row][col - 1] -
						(int) pin_8[slice][row][col];

                    if (tti1 < 0) {
                        tti1 = -tti1;
                        tti1 = (int) (((double) tti1) / anisotropy_column_coefficient + 0.5);
                        sum = sum - ((double) tti1) * transformation[selected_scale][tti1];
                    } else {
                        tti1 = (int) (((double) tti1) / anisotropy_column_coefficient + 0.5);
                        sum = sum + ((double) tti1) * transformation[selected_scale][tti1];
                    }
                }
                tti1 = (int) (sum + 0.5);
                if (tti1 < 0)
                    tti1 = 0;
                if (bytes == 2)
					outptr[slice][row][col] = tti1;
				else
					outptr_8[slice][row][col] = tti1;
            }
			 else
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
 *    sigma must be set.
 *    image data is valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not met.
 * HISTORY:
 *    Created: 10/3/97 by Punam K. Saha
 *    Modified: 3/19/04 8-bit scenes handled by Dewey Odhner.
 *
 *****************************************************************************/
int compute_feature_scale()
{
    int             tti2, tti5;
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
                    for (k = MIN_SCALE; (k < MAX_SCALE) && !flag; k++) {
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
                        pfeature_scale[slice][row][col] = MAX_SCALE - 1;
                }
    }
    printf("\n");
    fflush(stdout);

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
/****************************************************/
/***************************************************/
int get_slices(dim, list)
    int             dim;
    short          *list;
{
    int             i, sum;

    if (dim == 3)
        return (int) (list[0]);
    if (dim == 4) {
        for (sum = 0, i = 0; i < list[0]; i++)
            sum += list[1 + i];
        return (sum);
    }
    return (0);

}
/*******************************  THE END  *************************************/
