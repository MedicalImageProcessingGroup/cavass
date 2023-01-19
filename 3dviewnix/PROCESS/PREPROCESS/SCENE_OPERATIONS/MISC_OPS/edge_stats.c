/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif

#define ABS(a) ((a) < 0? -(a): (a))

int VInvertMatrix(double Ainv[], double A[], int N);
void destroy_scene_header(ViewnixHeader *vh),
  accumulate(int a, int b, int c, int d, int e, int f, int bdist, int edist);
double Det3(double m[3][3]);


static unsigned near_count, far_count;
static double near_sum[3], near_prod[3][3], far_sum[3], far_prod[3][3];

int main(argc,argv)
int argc;
char *argv[];
{
  int j, error, slices1;
  unsigned k;
  ViewnixHeader vh1, vh2;
  FILE *in1, *in2;
  char group[6], elem[6], *tmp_roi=(char *)"EDG-TMP-ROI.BIM";
  char *cmd;
  unsigned short *d1_16;
  int ns; // number of samples
  int two_D=FALSE;
  double tot_near_count=0, tot_far_count=0,
    near_covariance[3][3], far_covariance[3][3], covariance[3][3],
    inv_covariance[3][3], near_mean[3], far_mean[3], w[3];

  if (strcmp(argv[argc-1], "-2D") == 0)
  {
    two_D = TRUE;
	argc--;
  }
  if (argc < 3) {
    fprintf(stderr, "Usage: edge_stats <input BIM_file> ... <feature IM0_file> ... [-2D]\n");
	fprintf(stderr, " The list of samples of ");
	fprintf(stderr, "the object to be modeled must be followed by the ");
	fprintf(stderr, "corresponding feature scenes.\n");
    exit(-1);
  }
  if (argc%2 == 0)
  {
    fprintf(stderr, "Specify feature scene for each subject.\n");
    exit(-1);
  }
  ns = (argc-1)/2;
  for (j=k=0; j<ns; j++)
    if (k < strlen(argv[j+1]))
	  k = (unsigned)strlen(argv[j+1]);
  cmd = (char *)malloc(k*2+63);
  for (j=0; j<ns; j++)
  {
	int ii, jj, kk, a, b, c, d, e, f, bdist, edist;
	unsigned short *d2_16;
	unsigned char *d2_8;

	near_count = 0;
	far_count = 0;
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
	fclose(in1);
    if (vh1.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	slices1 = vh1.scn.num_of_subscenes[0];
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
	if (vh2.scn.num_of_subscenes[0]!=slices1 ||
		vh2.scn.xysize[0]!=vh1.scn.xysize[0] ||
		vh2.scn.xysize[1]!=vh1.scn.xysize[1])
	{
	  tmp_roi = (char *)"EDG-TMP-ROI.BIM";
	  sprintf(cmd,"ndvoi %s EDG-TMP-ROI.BIM 0 0 0 %d %d 0 0 0 %d",argv[j+1],
	    vh2.scn.xysize[0], vh2.scn.xysize[1], vh2.scn.num_of_subscenes[0]-1);
	  printf("%s\n", cmd);
	  system(cmd);
	}
	else
	  tmp_roi = argv[j+1];

	sprintf(cmd, "distance3D -e -p %s -s %s EDG-TMP-DIST.IM0",
	  two_D? "xy": "xyz", tmp_roi);
	system(cmd);
	unlink("EDG-TMP-ROI.BIM");
    in1 = fopen("EDG-TMP-DIST.IM0", "rb");
    if (in1==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
    d1_16 = (unsigned short *)malloc(vh1.scn.xysize[0]*vh1.scn.xysize[1]*2);
    VSeekData(in1,0);
	VSeekData(in2,0);
	if (vh2.scn.num_of_bits == 16)
	{
	  d2_16 = (unsigned short *)malloc(vh1.scn.xysize[0]*vh1.scn.xysize[1]*2);
	  for (kk=0; kk<vh2.scn.num_of_subscenes[0]; kk++)
	  {
		if (VReadData((char *)d1_16, 2,
			vh1.scn.xysize[0]*vh1.scn.xysize[1], in1, &error))
		{
		  fprintf(stderr, "Could not read data\n");
		  exit(-1);
		}
		if (VReadData((char *)d2_16, 2,
		    vh2.scn.xysize[0]*vh2.scn.xysize[1], in2, &error))
		{
		  fprintf(stderr, "Could not read data\n");
		  exit(-1);
		}
		// horizontal edges
		for (jj=0; jj<vh2.scn.xysize[1]-1; jj++)
		  for (ii=0; ii<vh2.scn.xysize[0]-2; ii++)
		  {
		    a = d2_16[vh2.scn.xysize[0]*jj+ii];
			b = d2_16[vh2.scn.xysize[0]*jj+ii+1];
			c = d2_16[vh2.scn.xysize[0]*jj+ii+2];
			d = d2_16[vh2.scn.xysize[0]*(jj+1)+ii];
			e = d2_16[vh2.scn.xysize[0]*(jj+1)+ii+1];
			f = d2_16[vh2.scn.xysize[0]*(jj+1)+ii+2];
			bdist = d1_16[vh2.scn.xysize[0]*jj+ii+1];
			edist = d1_16[vh2.scn.xysize[0]*(jj+1)+ii+1];
			accumulate(a, b, c, d, e, f, bdist, edist);
		  }

		// vertical edges
		for (jj=0; jj<vh2.scn.xysize[1]-2; jj++)
		  for (ii=0; ii<vh2.scn.xysize[0]-1; ii++)
		  {
		    a = d2_16[vh2.scn.xysize[0]*jj+ii];
			b = d2_16[vh2.scn.xysize[0]*(jj+1)+ii];
			c = d2_16[vh2.scn.xysize[0]*(jj+2)+ii];
			d = d2_16[vh2.scn.xysize[0]*jj+ii+1];
			e = d2_16[vh2.scn.xysize[0]*(jj+1)+ii+1];
			f = d2_16[vh2.scn.xysize[0]*(jj+2)+ii+1];
			bdist = d1_16[vh2.scn.xysize[0]*(jj+1)+ii];
			edist = d1_16[vh2.scn.xysize[0]*(jj+1)+ii+1];
			accumulate(a, b, c, d, e, f, bdist, edist);
		  }
	  }
	  free(d2_16);
	}
	else
	{
	  d2_8 = (unsigned char *)malloc(vh1.scn.xysize[0]*vh1.scn.xysize[1]);
	  for (kk=0; kk<vh2.scn.num_of_subscenes[0]; kk++)
	  {
		if (VReadData((char *)d1_16, 2,
			vh1.scn.xysize[0]*vh1.scn.xysize[1], in1, &error))
		{
		  fprintf(stderr, "Could not read data\n");
		  exit(-1);
		}
		if (VReadData((char *)d2_8, 1,
		    vh2.scn.xysize[0]*vh2.scn.xysize[1], in2, &error))
		{
		  fprintf(stderr, "Could not read data\n");
		  exit(-1);
		}
		// horizontal edges
		for (jj=0; jj<vh2.scn.xysize[1]-1; jj++)
		  for (ii=0; ii<vh2.scn.xysize[0]-2; ii++)
		  {
		    a = d2_8[vh2.scn.xysize[0]*jj+ii];
			b = d2_8[vh2.scn.xysize[0]*jj+ii+1];
			c = d2_8[vh2.scn.xysize[0]*jj+ii+2];
			d = d2_8[vh2.scn.xysize[0]*(jj+1)+ii];
			e = d2_8[vh2.scn.xysize[0]*(jj+1)+ii+1];
			f = d2_8[vh2.scn.xysize[0]*(jj+1)+ii+2];
			bdist = d1_16[vh2.scn.xysize[0]*jj+ii+1];
			edist = d1_16[vh2.scn.xysize[0]*(jj+1)+ii+1];
			accumulate(a, b, c, d, e, f, bdist, edist);
		  }

		// vertical edges
		for (jj=0; jj<vh2.scn.xysize[1]-2; jj++)
		  for (ii=0; ii<vh2.scn.xysize[0]-1; ii++)
		  {
		    a = d2_8[vh2.scn.xysize[0]*jj+ii];
			b = d2_8[vh2.scn.xysize[0]*(jj+1)+ii];
			c = d2_8[vh2.scn.xysize[0]*(jj+2)+ii];
			d = d2_8[vh2.scn.xysize[0]*jj+ii+1];
			e = d2_8[vh2.scn.xysize[0]*(jj+1)+ii+1];
			f = d2_8[vh2.scn.xysize[0]*(jj+2)+ii+1];
			bdist = d1_16[vh2.scn.xysize[0]*(jj+1)+ii];
			edist = d1_16[vh2.scn.xysize[0]*(jj+1)+ii+1];
			accumulate(a, b, c, d, e, f, bdist, edist);
		  }
	  }
	  free(d2_8);
	}
	tot_near_count += near_count;
	tot_far_count += far_count;
	fclose(in1);
    fclose(in2);
	free(d1_16);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
	unlink("EDG-TMP-DIST.IM0");
  }
  printf(
    "\nnear_count: %.0f\n", tot_near_count);
  printf(
    "\nfar_count: %.0f\n", tot_far_count);

  // compute Fisher's linear discriminant
  for (j=0; j<3; j++)
  {
    near_mean[j] = near_sum[j]/tot_near_count;
	far_mean[j] = far_sum[j]/tot_far_count;
    for (k=j; k<3; k++)
	{
	  near_covariance[j][k] = near_covariance[k][j] =
	    (near_prod[j][k]-near_sum[j]*near_sum[k]/tot_near_count)/
		(tot_near_count-1);
	  far_covariance[j][k] = far_covariance[k][j] =
		(far_prod[j][k]-far_sum[j]*far_sum[k]/tot_far_count)/
		(tot_far_count-1);
	  covariance[j][k] = covariance[k][j] =
	    near_covariance[j][k]+far_covariance[j][k];
    }
  }
  printf("\nnear_mean: %f %f %f\n", near_mean[0], near_mean[1], near_mean[2]);
  printf("\nnear_covariance:\n %f %f %f\n %f %f %f\n %f %f %f\n",
    near_covariance[0][0], near_covariance[0][1], near_covariance[0][2],
	near_covariance[1][0], near_covariance[1][1], near_covariance[1][2],
	near_covariance[2][0], near_covariance[2][1], near_covariance[2][2]);
  printf("\nfar_mean: %f %f %f\n", far_mean[0], far_mean[1], far_mean[2]);
  printf("\nfar_covariance:\n %f %f %f\n %f %f %f\n %f %f %f\n",
    far_covariance[0][0], far_covariance[0][1], far_covariance[0][2],
	far_covariance[1][0], far_covariance[1][1], far_covariance[1][2],
	far_covariance[2][0], far_covariance[2][1], far_covariance[2][2]);
  if (Det3(covariance) == 0)
  {
    printf("Covariance matrix is singular.\n");
	exit(1);
  }
  VInvertMatrix(inv_covariance[0], covariance[0], 3);
  for (j=0; j<3; j++)
    w[j] = 
	  inv_covariance[0][j]*(far_mean[0]-near_mean[0])+
	  inv_covariance[1][j]*(far_mean[1]-near_mean[1])+
	  inv_covariance[2][j]*(far_mean[2]-near_mean[2]);
  printf("\nw:\n%f %f %f\n", w[0], w[1], w[2]);
  exit(0);
}

void accumulate(int a, int b, int c, int d, int e, int f, int bdist, int edist)
{
	double feature_val[3] /* hi, low, gradient */, grad2;
	int reverse, j, k, near;

	if (e > b)
	{
		feature_val[0] = e;
		feature_val[1] = b;
	}
	else
	{
		feature_val[0] = b;
		feature_val[1] = e;
	}
	if (edist == bdist)
		feature_val[2] = 0;
	else
	{
		grad2 = d+e+f-a-b-c;
		reverse = (grad2<0) != (edist<bdist);
		if (grad2 < 0)
			grad2 = -grad2;
		feature_val[2] =
			1/6.*grad2+ .1767767*(ABS(a-e)+ABS(b-d)+ABS(b-f)+ABS(c-e));
		if (reverse)
			feature_val[2] = -feature_val[2];
	}
	near = bdist+edist>65434 && bdist+edist<65634;
	if (near)
		for (j=0; j<3; j++)
		{
			near_count++;
			near_sum[j] += feature_val[j];
			for (k=j; k<3; k++)
				near_prod[j][k] += feature_val[j]*feature_val[k];
		}
	else
		for (j=0; j<3; j++)
		{
			far_count++;
			far_sum[j] += feature_val[j];
			for (k=j; k<3; k++)
				far_prod[j][k] += feature_val[j]*feature_val[k];
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

/************************************************************************
 *
 *      FUNCTION        : Det3
 *
 *      DESCRIPTION     : returns the determinant of a 3x3 martrix.
 *
 *      RETURN VALUE    : determinant of the matrix.
 *
 *      PARAMETERS      : m - input matrix
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : m should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
double Det3(double m[3][3])
{
  double result;
  
  result = 
    m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] +
      m[0][2]*m[1][0]*m[2][1] - m[0][2]*m[1][1]*m[2][0] -
	m[0][0]*m[1][2]*m[2][1] - m[0][1]*m[1][0]*m[2][2];
  return(result);
}

