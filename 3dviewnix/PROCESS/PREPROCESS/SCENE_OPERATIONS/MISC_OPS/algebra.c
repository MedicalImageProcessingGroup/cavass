/*
  Copyright 1993-2015 Medical Image Processing Group
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

typedef struct Quadratic {float xx, xy, yy, x, y, cnst;} Quadratic;

#define Quadratic_value(X, Y, q) \
	((X)*((q).xx*(X)+(q).xy*(Y)+(q).x)+(Y)*((q).yy*(Y)+(q).y)+(q).cnst)

int get_slices(int dim, short *list);
void adjust_interval(float *min_out, float *max_out, float min1, float max1,
    float min2, float max2, Quadratic *q);
float FuzzySymmetricProduct(double x, double y);

/*****************************************************************************
 * FUNCTION: parse_quadratic
 * DESCRIPTION: Extracts the coefficients from a quadratic exxpression
 *    in "x" & "y".
 * PARAMETERS:
 *    out: The coefficents are stored here.
 *    in: The input expression in the form "[<f>xx][[+|-]<f>xy][[+|-]<f>yy]
 *       [[+|-]<f>x][[+|-]<f>y][[+|-]<f>]" where each "<f>" is a decimal
 *       coefficient.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero on error
 * EXIT CONDITIONS: Not all errors in in are detected.
 * HISTORY:
 *    Created: 5/22/97 by Dewey Odhner
 *
 *****************************************************************************/
int parse_quadratic(out, in)
	Quadratic *out;
	char in[];
{
	char *buf;
	int coef_len, x_degree, y_degree;
	float *field, coeff, factor;

	buf = malloc(strlen(in)+1);
	if (buf == NULL)
		return (1);
	out->xx = out->xy = out->yy = out->x = out->y = out->cnst = 0;
	while (*in)
	{
		coeff = 1;
		for (;; in++)
		{
			if (*in == '-')
				coeff = -coeff;
			else if (*in!=' ' && *in!='+')
				break;
		}
		if (*in == 0)
		{
			free(buf);
			return (2);
		}
		x_degree = y_degree = coef_len = 0;
		for (;;)
		{
			while (*in == ' ')
				in++;
			if (*in==0 || *in=='+' || *in=='-' || *in=='x' || *in=='y')
			{
				if (*in == 'x')
					x_degree++;
				if (*in == 'y')
					y_degree++;
				if (x_degree+y_degree > 2)
				{
					free(buf);
					return (2);
				}
				if (coef_len)
				{
					buf[coef_len] = 0;
					if (sscanf(buf, "%f", &factor) != 1)
					{
						free(buf);
						return (2);
					}
					coeff *= factor;
					coef_len = 0;
				}
				if (*in==0 || *in=='+' || *in=='-')
					break;
			}
			else
				buf[coef_len++] = *in;
			in++;
		}
		if (x_degree == 2)
			field = &out->xx;
		else if (y_degree == 2)
			field = &out->yy;
		else if (x_degree && y_degree)
			field = &out->xy;
		else if (x_degree)
			field = &out->x;
		else if (y_degree)
			field = &out->y;
		else
			field = &out->cnst;
		*field += coeff;
	}
	free(buf);
	return (0);
}

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Implements image algebra.
 * PARAMETERS:
 *    argc: The number of command-line parameters
 *    argv: The command-line parameters
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: Not all errors in the expressions are detected.
 * HISTORY:
 *    Created: 5/23/97 (modified from fuzz_ops) by Dewey Odhner
 *    Modified: 5/29/97 alternate expression used in masked-out region
 *       by Dewey Odhner
 *    Modified: 6/17/97 background option added by Dewey Odhner
 *    Modified: 6/18/97 fixed memory overwriting bug by Dewey Odhner
 *    Modified: 7/2/97 bit fields set by Dewey Odhner
 *    Modified: 12/16/97 memory allocation corrected by Dewey Odhner
 *    Modified: 8/27/02 unequal number of slices handled by Dewey Odhner
 *
 *****************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{

  int sn,j,bytes,bytes1,bytes2,slices,size,error, bg_flag=FALSE, slic1, slic2,
	start, stop, step, fsp_flag;
  float min1,max1,min2,max2;
  float val1,val2;
  ViewnixHeader vh1,vh2;
  FILE *in1,*in2,*out;
  unsigned char *data1, *data2;
  char group[6],elem[6];
  Quadratic q_out, q_mask, q_alt;
  double total=0;

#define Handle_error(message) \
{ \
  char msg[200]; \
  fprintf(stderr, message); \
  if (bg_flag == 1) \
	{ \
		sprintf(msg, "job_done %s -abort &", argv[0]); \
		system(msg); \
	    VDeleteBackgroundProcessInformation(); \
	} \
  exit(1); \
}

  if (strcmp(argv[argc-1], "-b") == 0) {
	bg_flag = TRUE;
	argc--;
  }
  if (argc!=5 && argc!=7) {
    fprintf(stderr,
"Usage: %s inputfile1 inputfile2 output_file expr [mask_expr alt_expr] [-b]\n",
	  argv[0]);
    exit(-1);
  }
  if (bg_flag == 1)
    VAddBackgroundProcessInformation(argv[0]);

  in1=fopen(argv[1],"rb");
  if (in1==NULL )
    Handle_error("Error in opening the input file\n");
  in2=fopen(argv[2],"rb");
  if (in2==NULL )
    Handle_error("Error in opening the input file\n");

  if (strcmp(argv[3], "/dev/null") == 0)
    out = NULL;
  else
  {
    out=fopen(argv[3],"w+b");
    if ( out==NULL )
      Handle_error("Error in opening output file\n");
  }

  error=VReadHeader(in1,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (vh1.gen.data_type!=IMAGE0)
    Handle_error("This is not an IMAGE0 file\n");

  if (strcmp(argv[4], "fsp") == 0)
    fsp_flag = 1;
  else
  {
    fsp_flag = 0;
	if (parse_quadratic(&q_out, argv[4]))
      Handle_error("Cannot parse expr.\n");
  }
  if (argc == 5)
	parse_quadratic(&q_mask, "1");
  else if (parse_quadratic(&q_mask, argv[5]))
    Handle_error("Cannot parse mask_expr.\n")
  else if (parse_quadratic(&q_alt, argv[6]))
    Handle_error("Cannot parse alt_expr.\n");

  bytes1 = vh1.scn.num_of_bits/8;
  slices = slic1 = get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  size= (vh1.scn.xysize[0]*vh1.scn.xysize[1]);

  error=VReadHeader(in2,&vh2,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (vh2.gen.data_type!=IMAGE0 ||
      vh2.scn.xysize[0]!=vh1.scn.xysize[0] || 
      vh2.scn.xysize[1]!=vh1.scn.xysize[1])
    Handle_error("Second input file is incompatible with 1st input file");
  if ((slic2=get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes)) > slices)
    slices = slic2;
  bytes2 = vh2.scn.num_of_bits/8;
  min1 = vh1.scn.smallest_density_value[0];
  max1 = vh1.scn.largest_density_value[0];
  min2 = vh2.scn.smallest_density_value[0];
  max2 = vh2.scn.largest_density_value[0];

  vh1.scn.smallest_density_value[0] = 65535;
  vh1.scn.largest_density_value[0] = 0;
  adjust_interval(vh1.scn.smallest_density_value,
	vh1.scn.largest_density_value, min1, max1, min2, max2,
	fsp_flag? (Quadratic *)0:&q_out);
  if (argc == 7)
	adjust_interval(vh1.scn.smallest_density_value,
	  vh1.scn.largest_density_value, min1, max1, min2, max2, &q_alt);
  if (vh1.scn.smallest_density_value[0] < 0)
	vh1.scn.smallest_density_value[0] = 0;
  if (vh1.scn.largest_density_value[0] > 65535)
	vh1.scn.largest_density_value[0] = 65535;

  bytes = vh1.scn.largest_density_value[0]>=256? 2:1;
  vh1.scn.num_of_bits=bytes*8;
  vh1.scn.bit_fields[0] = 0;
  vh1.scn.bit_fields[1] = vh1.scn.num_of_bits-1;
  strncpy(vh1.gen.filename, argv[3], sizeof(vh1.gen.filename));
  if (slic2 > slic1)
  {
    vh1.scn.dimension = vh2.scn.dimension;
	free(vh1.scn.num_of_subscenes);
	free(vh1.scn.loc_of_subscenes);
	vh1.scn.num_of_subscenes = vh2.scn.num_of_subscenes;
	vh1.scn.loc_of_subscenes = vh2.scn.loc_of_subscenes;
  }

  data1= (unsigned char *)malloc(size*2);
  if (data1==NULL)
    Handle_error("Could not allocate data. Aborting fuzz_ops\n");

  data2= (unsigned char *)malloc((size*vh2.scn.num_of_bits+7)/8);
  if (data2==NULL)
    Handle_error("Could not allocate output data. Aborting fuzz_ops\n");

  if (out)
  {
    error=VWriteHeader(out,&vh1,group,elem);
    if (error>0 && error<=104)
      Handle_error("Fatal error in writing header\n");
  }

  VSeekData(in1,0);
  VSeekData(in2,0);

  if (bytes1 > bytes)
  {
	start = 0;
	stop = size;
	step = 1;
  }
  else
  {
	start = size-1;
	stop = -1;
	step = -1;
  }
  for(sn=0; sn<slices; sn++) {
    if (out && !bg_flag)
	{	printf("Computing slice %d\r", sn+1);
    	fflush(stdout);
	}

    if (sn >= slic1)
		memset(data1, 0, bytes1? bytes1*size: (size+7)/8);
	else if (bytes1)
		error = VReadData((char *)data1,bytes1,size,in1,&j);
	else
		error = VReadData((char *)data1, 1, (size+7)/8, in1, &j);
	if (error)
      Handle_error("Could not read data\n");
    if (sn >= slic2)
		memset(data2, 0, bytes2? bytes2*size: (size+7)/8);
	else if (bytes2)
		error = VReadData((char *)data2,bytes2,size,in2,&j);
	else
		error = VReadData((char *)data2, 1, (size+7)/8, in2, &j);
	if (error)
      Handle_error("Could not read data\n");

	for (j=start; j!=stop; j+=step)
	{
		switch (bytes1)
		{
		  case 0:
			val1 = (float)((data1[j/8]&(128>>(j%8))) != 0);
			break;
		  case 1:
			val1 = data1[j];
			break;
		  case 2:
			val1 = ((unsigned short *)data1)[j];
			break;
		}
		switch (bytes2)
		{
		  case 0:
			val2 = (float)((data2[j/8]&(128>>(j%8))) != 0);
			break;
		  case 1:
			val2 = data2[j];
			break;
		  case 2:
			val2 = ((unsigned short *)data2)[j];
			break;
		}
		if (Quadratic_value(val1, val2, q_mask) > 0)
		{
			if (fsp_flag)
				val1= 65534*FuzzySymmetricProduct(1/65534.*val1,1/65534.*val2);
			else
				val1 = Quadratic_value(val1, val2, q_out);
		}
		else
			val1 = Quadratic_value(val1, val2, q_alt);
		if (out)
		{
			val1 += .5;
			if (val1 < 0)
				val1 = 0;
			if (val1 > 65535)
				val1 = 65535;
			if (bytes == 2)
				((unsigned short *)data1)[j] = (unsigned short)val1;
			else
				data1[j] = (unsigned char)val1;
		}
		else
			total += val1;
	}

    if (out && VWriteData((char *)data1,bytes,size,out,&j))
      Handle_error("Could not write data\n");
  }

  fclose(in1);
  fclose(in2);
  if (out)
    VCloseData(out);
  else
    printf("%f\n", total);

  if (bg_flag == 1)
  {	char cmd[256];

	sprintf(cmd, "job_done %s &", argv[0]);
	system(cmd);
    VDeleteBackgroundProcessInformation();
  }
  else
	if (out && bg_flag == 0)
	  printf("\ndone\n");
  exit(0);
}



/*****************************************************************************
 * FUNCTION: adjust_interval
 * DESCRIPTION: Adjusts an interval to cover the range of a quadratic function
 *    within a rectangular domain.
 * PARAMETERS:
 *    min_out, max_out: The interval to be adjusted.
 *    min1, max1: The domain of the "x" variable.
 *    min2, max2: The domain of the "y" variable.
 *    q: The coefficients of the quadratic function.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 5/28/97 by Dewey Odhner
 *
 *****************************************************************************/
void adjust_interval(float *min_out, float *max_out, float min1, float max1,
    float min2, float max2, Quadratic *q)
{
  float val1,val2;

  if (q == 0)
  {
    if (0 < *min_out)
	  *min_out = 0;
	if (65534 > *max_out)
	  *max_out = 65534;
	return;
  }
  val1 = (float)Quadratic_value(min1, min2, *q);
  if (val1 < *min_out)
	  *min_out = val1;
  if (val1 > *max_out)
	  *max_out = val1;
  val1 = (float)Quadratic_value(min1, max2, *q);
  if (val1 < *min_out)
	  *min_out = val1;
  if (val1 > *max_out)
	  *max_out = val1;
  val1 = (float)Quadratic_value(max1, min2, *q);
  if (val1 < *min_out)
	  *min_out = val1;
  if (val1 > *max_out)
	  *max_out = val1;
  val1 = (float)Quadratic_value(max1, max2, *q);
  if (val1 < *min_out)
	  *min_out = val1;
  if (val1 > *max_out)
	  *max_out = val1;
  if (q->xx != 0)
  {
	val1 = (float)(-(q->xy*min2+q->x)/(2*q->xx));
	if (val1 > min1 && val1<max1)
	{
		val1 = (float)Quadratic_value(val1, min2, *q);
		if (val1 < *min_out)
			*min_out = val1;
		if (val1 > *max_out)
			*max_out = val1;
	}
	val1 = (float)(-(q->xy*max2+q->x)/(2*q->xx));
	if (val1 > min1 && val1<max1)
	{
		val1 = (float)Quadratic_value(val1, max2, *q);
		if (val1 < *min_out)
			*min_out = val1;
		if (val1 > *max_out)
			*max_out = val1;
	}
  }
  if (q->yy != 0)
  {
	val1 = (float)(-(q->xy*min1+q->y)/(2*q->yy));
	if (val1 > min2 && val1<max2)
	{
		val1 = (float)Quadratic_value(min1, val1, *q);
		if (val1 < *min_out)
			*min_out = val1;
		if (val1 > *max_out)
			*max_out = val1;
	}
	val1 = (float)(-(q->xy*max1+q->y)/(2*q->yy));
	if (val1 > min2 && val1<max2)
	{
		val1 = (float)Quadratic_value(max1, val1, *q);
		if (val1 < *min_out)
			*min_out = val1;
		if (val1 > *max_out)
			*max_out = val1;
	}
  }
  val1 = 4*q->xx*q->yy-q->xy*q->xy;
  if (val1 != 0)
  {
	val2 = (q->xy*q->x-2*q->xx*q->y)/val1;
	val1 = (q->xy*q->y-2*q->yy*q->x)/val1;
	if (val1>min1 && val1<max1 && val2>min2 && val2<max2)
	{
		val1 = Quadratic_value(val1, val2, *q);
		if (val1 < *min_out)
			*min_out = val1;
		if (val1 > *max_out)
			*max_out = val1;
	}
  }
}

/*****************************************************************************
 * FUNCTION: get_slices
 * DESCRIPTION: Returns total number of slices in a scene.
 * PARAMETERS:
 *    dim: The dimension of the scene.
 *    list: The list containing the number of subscenes in the file.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The total number of slices in the file.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: unknown
 *
 *****************************************************************************/
int get_slices(int dim, short *list)
{
  int i,sum;

  if (dim==3) return (int)(list[0]);
  if (dim==4) {
    for(sum=0,i=0;i<list[0];i++)
      sum+= list[1+i];
    return(sum);
  }
  return(0);
}

float FuzzySymmetricProduct(double x, double y)
{
	double r;

	r = x*y*(x*y+1-x-y);
	if (r < 0)
		r = 0;
	return (float)(1-x-y? (sqrt(r)-x*y)/(1-x-y): .5);
}
