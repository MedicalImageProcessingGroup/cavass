/*
  Copyright 1993-2016, 2018 Medical Image Processing Group
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

int compute_filter_image(), compute_feature_scale(), get_max(int, int),
 get_min(int, int);

// Welch's t-test
// t = (m1-m2)/sqrt(s1_sqr/n1+s2_sqr/n2)



int             max_scale=DEFAULT_MAX_SCALE, min_scale=MIN_SCALE;
ViewnixHeader   vh;
int             smallest_density_value, largest_density_value;
double          sigma=50;
unsigned short ***pimage_16;
unsigned char ***pimage_8;
int             pslice, prow, pcol;
unsigned short *feature_scale, ***pfeature_scale;
unsigned char  *which_scale, ***pwhich_scale;
int            *sphere_no_points, (**sphere_points)[3], *ball_no_points;
unsigned short ***pfeature_data_filter;
unsigned short *feature_data_filter;
unsigned short *slice_data_16;
unsigned char  *slice_data_8;
double         *transformation_scale;
long            histogram_count;
int             slice_size, size, bytes;
char           *out_file_name, *out_file2_name;
char            group[6], element[6];
int             error;
int             min_density, ball_low, ball_high=65535;
int             twoD;
/*****************************************************************************/
/*    Modified: 2/26/04 8-bit scenes handled by Dewey Odhner. */
int main(argc, argv)
    int             argc;
    char           *argv[];
{
    int             i, j, k, l, tti1, tti2,
                 ***ppptti1;
    FILE           *in;
    int             slice, row;
    double          inv_sigma;
    double          tt1, sphere_slice_coefficient, sphere_row_coefficient, sphere_column_coefficient;


	if (argc>3 && strcmp(argv[argc-1], "-2D")==0)
	{
		twoD = 1;
		argc--;
	}
	if (argc>4 && strcmp(argv[argc-2], "-r")==0)
	{
		out_file2_name = argv[argc-1];
		argc -= 2;
	}
	if (argc==8 && sscanf(argv[5], "%d", &min_density)==1 &&
			sscanf(argv[6], "%d", &ball_low)==1 &&
			sscanf(argv[7], "%d", &ball_high)==1)
		argc = 5;
	if (argc==5 && sscanf(argv[4], "%d", &min_scale)==1)
		argc = 4;
	if (argc==4 && sscanf(argv[3], "%d", &max_scale)==1 && max_scale>=2)
		argc = 3;
    if (argc != 3) {
        printf("Usage: ball_enhance <image_file> <output_file> [<max_scale> [<min_scale> [<min_density> <ball_low> <ball_high>]] [-r <radius_file>] [-2D]\n");
        exit(-1);
    }

	out_file_name = argv[2];

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

    if (vh.scn.dimension != 3)
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
	ball_no_points = (int *) malloc(max_scale * sizeof(int));
    if (sphere_no_points == NULL || ball_no_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");

    sphere_points = (void *) malloc(max_scale * sizeof(void *));
    if (sphere_points == NULL)
        Handle_error("Couldn't allocate memory (execution terminated)\n");


    sphere_column_coefficient = vh.scn.xypixsz[0];
    sphere_row_coefficient = vh.scn.xypixsz[1];
    sphere_slice_coefficient = vh.scn.num_of_subscenes[0]==1? 1:
		vh.scn.loc_of_subscenes[1] - vh.scn.loc_of_subscenes[0];
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
        sphere_no_points[k] = 0;
        for (i = twoD? 0: -k - 2; i <= (twoD? 0: k + 2); i++)
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

		if (k == 0)
			ball_no_points[k] = 0;
		else
			ball_no_points[k] = ball_no_points[k-1]+sphere_no_points[k-1];

        sphere_points[k] = (void *) malloc(3 * sphere_no_points[k] * sizeof(int));
        if (sphere_points[k] == NULL)
            Handle_error("Couldn't allocate memory (execution terminated)\n");

        tti2 = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                for (l = -k - 2; l <= k + 2; l++)
                    if (ppptti1[tti1 + i][tti1 + j][tti1 + l] == 2) {
                        ppptti1[tti1 + i][tti1 + j][tti1 + l] = 1;
                        sphere_points[k][tti2][0] = l;
                        sphere_points[k][tti2][1] = j;
                        sphere_points[k][tti2][2] = i;
                        tti2 = tti2 + 1;
                    }
    }
printf("\n");
fflush(stdout);	

    /***********************************************/
    feature_scale = NULL;
	which_scale = NULL;
    compute_filter_image();

    exit(0);
}

int compute_filter_image()
{

    compute_feature_scale();
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
    int             tti5;
    ViewnixHeader   vh_scale;
    FILE           *fp_scale;
	short           bit_fields[] = {0, 15};
    float           smallest_density_value_scale[] = {0.0},
                    largest_density_value_scale[] = {0.0};
    int             slice, row, col, i, j, k, xx, yy, zz;
    unsigned short ***pin_16;
	unsigned char ***pin_8;
	double          ball_sum, sphere_sum, ball_sumsq, sphere_sumsq, bestt;

    feature_scale = (unsigned short *) malloc(size * sizeof(unsigned short));
	which_scale = (unsigned char *) malloc(size);
    if (feature_scale == NULL || which_scale == NULL)
        Handle_error("Couldn't allocate memory\n");

    pfeature_scale = (unsigned short ***) malloc(pslice * sizeof(unsigned short **));
	pwhich_scale = (unsigned char ***) malloc(pslice*sizeof(unsigned char **));
    if (pfeature_scale == NULL || pwhich_scale == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");

    pfeature_scale[0] = (unsigned short **) malloc(pslice * prow * sizeof(unsigned short *));
	pwhich_scale[0] = (unsigned char **)malloc(pslice * prow * sizeof(unsigned char *));
    if (pfeature_scale[0] == NULL || pwhich_scale[0] == NULL)
        Handle_error("COULD ALLOCATE MEMORY\n");
    for (slice = 0; slice < pslice; slice++)
	{
        pfeature_scale[slice] = pfeature_scale[0] + slice * prow;
		pwhich_scale[slice] = pwhich_scale[0] + slice * prow;
	}

    for (slice = 0; slice < pslice; slice++)
        for (row = 0; row < prow; row++)
        {
		    pfeature_scale[slice][row] = feature_scale + (slice * prow + row) * pcol;
			pwhich_scale[slice][row] = which_scale + (slice * prow + row) * pcol;
		}

    pin_16 = pimage_16;
	pin_8 = pimage_8;
    for (slice = 0; slice < pslice; slice++) {
        printf("\rScale computing slice %d", slice);
        fflush(stdout);
        for (row = 0; row < prow; row++)
            for (col = 0; col < pcol; col++)
                {

					bestt = 0;
					ball_sum = ball_sumsq = 0;
					pwhich_scale[slice][row][col] = MIN_SCALE;
                    for (k = MIN_SCALE; k < max_scale; k++) {
						sphere_sum = sphere_sumsq = 0;
                        for (i = 0; i < sphere_no_points[k]; i++) {
                            xx = col + sphere_points[k][i][0];
                            yy = row + sphere_points[k][i][1];
                            zz = slice + sphere_points[k][i][2];
                            if (xx >= 0 && xx < pcol && yy >= 0 &&
                                yy < prow && zz >= 0 && zz < pslice) {
                                tti5 = bytes==2? pin_16[zz][yy][xx]:
									pin_8[zz][yy][xx];
                                if (tti5 < min_density)
								{
									k = max_scale;
									break;
								}
								sphere_sum += tti5;
								sphere_sumsq += (double)tti5*tti5;
                            }
                        }
						if (k >= max_scale)
							break;
						if (k > min_scale)
						{
							double m1, m2, s1_sqr, s2_sqr, t, s1;
							m1 = ball_sum/ball_no_points[k];
							m2 = sphere_sum/sphere_no_points[k];
							if (m1-m2 == 0)
								continue;
							s1_sqr =
							    (ball_sumsq-m1*ball_sum)/(ball_no_points[k]-1);
							s1 = sqrt(s1_sqr);
							if (m1-s1>=ball_low && m1+s1<=ball_high)
							{
								s2_sqr = (sphere_sumsq-m2*sphere_sum)/
									(sphere_no_points[k]-1);
								t = s1_sqr/ball_no_points[k]+
									s2_sqr/sphere_no_points[k];
								if (t == 0)
								{
									bestt = 65.535;
									break;
								}
								t = (m1-m2)/sqrt(t);
								if (t < 0)
									t = -t;
								if (t > bestt)
								{
									bestt = t;
									pwhich_scale[slice][row][col] =
									    (unsigned char)(k-1);
								}
							}
						}

						ball_sum += sphere_sum;
						ball_sumsq += sphere_sumsq;
                    }
					bestt = rint(1000*bestt);
					if (bestt > 65535)
						bestt = 65535;
					if (bestt > largest_density_value_scale[0])
						largest_density_value_scale[0] = (float)bestt;
					pfeature_scale[slice][row][col] = (unsigned short)bestt;
                }
    }
    printf("\n");
    fflush(stdout);
    /************ WRITE SCALE IMAGE ************/
	if (strcmp(out_file_name, "/dev/null"))
	{
	    vh_scale = vh;
		vh_scale.scn.num_of_bits = 16;
		vh_scale.scn.bit_fields = bit_fields;
	    vh_scale.scn.smallest_density_value = smallest_density_value_scale;
	    vh_scale.scn.largest_density_value = largest_density_value_scale;
	    fp_scale = fopen(out_file_name, "w+b");
		if (fp_scale == NULL)
		{
			fprintf(stderr,"Could not open %s for writing.\n",out_file_name);
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
	if (out_file2_name && strcmp(out_file2_name, "/dev/null"))
	{
		vh_scale.scn.num_of_bits = 8;
		vh_scale.scn.bit_fields[1] = 7;
		vh_scale.scn.smallest_density_value[0] = 0;
		vh_scale.scn.largest_density_value[0] = (float)max_scale;
		fp_scale = fopen(out_file2_name, "w+b");
		if (fp_scale == NULL)
		{
			fprintf(stderr,"Could not open %s for writing.\n",out_file2_name);
			return (1);
		}
	    error = VWriteHeader(fp_scale, &vh_scale, group, element);
	    if (error && error <= 104)
	        Handle_error("Fatal error in writing header\n");

	    if (VWriteData((char *)which_scale, 1, size, fp_scale, &j)!=0 ||
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
