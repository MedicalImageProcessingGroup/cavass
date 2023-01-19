/*
  Copyright 1993-2016 Medical Image Processing Group
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

int get_slices(int dim, short *list);
void bin_to_grey(unsigned char *bin_buffer, int length,
     unsigned char *grey_buffer, int min_value, int max_value),
  Stat16(unsigned short *in1, unsigned char *msk, unsigned short *in2,
     int size, double final_factor_bqml),
  Stat16_16(unsigned short *in1, unsigned short *msk, unsigned short *in2,
     int size, double final_factor_bqml);
void destroy_scene_header(ViewnixHeader *vh);
void get_stats(double *mean);

static double sum=0.0, peak_suv=0.0, sum_sqr;
static double count=0;

int main(argc,argv)
int argc;
char *argv[];
{

  int ii, j, slices, size, size2, error, slices2, bytes1, subject, bytes3;
  ViewnixHeader vh1, vh2, vh3;
  FILE *in1, *in2, *in3;
  unsigned short *d1_16, *d2_16, *d3_16;
  unsigned char *data2, *d2_8;
  char group[6],elem[6];
  double mean, final_factor_bqml;

  if (argc!=5 || sscanf(argv[argc-1], "%lf", &final_factor_bqml)!=1) {
    fprintf(stderr, "Usage: suv_stats <IM0_file> <BIM_file> <slope_IM0_file> <final_factor_bqml>\n");
    exit(-1);
  }

  for (subject=0; subject<(argc-2)/3; subject++)
  {
    in1 = fopen(argv[1+subject],"rb");
    in2 = fopen(argv[1+(argc-2)/3+subject],"rb");
	in3 = fopen(argv[1+(argc-2)*2/3+subject],"rb");

    if (in1==NULL || in2==NULL  || in3==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }

    error=VReadHeader(in1,&vh1,group,elem);
    if (error && error<=104) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }

    error=VReadHeader(in2,&vh2,group,elem);
    if (error && error<=104) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }

    error=VReadHeader(in3,&vh3,group,elem);
    if (error && error<=104) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }

    if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0 ||
		vh3.gen.data_type!=IMAGE0) {
      fprintf(stderr, "All input files should be IMAGE0\n");
      exit(-1);
    }
    if ((vh2.scn.num_of_bits!=1 && vh2.scn.num_of_bits!=16) ||
	    vh1.scn.num_of_bits<8 || vh3.scn.num_of_bits<8) {
      fprintf(stderr, "The Mask file only may be binary\n");
      exit(-1);
    }
    if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] ||
	    vh1.scn.xysize[1]!=vh2.scn.xysize[1] ||
		vh1.scn.xysize[0]!=vh3.scn.xysize[0] ||
		vh1.scn.xysize[1]!=vh3.scn.xysize[1]) {
      fprintf(stderr, "Both Scenes should be of the same width and height\n");
      exit(-1);
    }

    slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
    slices2= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
    if (slices < slices2) { 
      fprintf(stderr,  "Mask Scene has more slices\n");
      fprintf(stderr,  "Ignoring the extra slices\n");
    }
	if (get_slices(vh3.scn.dimension, vh3.scn.num_of_subscenes) != slices)
	{
	  fprintf(stderr, "Both Scenes should be of the same slices\n");
	  exit(-1);
	}

    size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
    if (vh1.scn.num_of_bits==16)
      bytes1=2;
    else
      bytes1=1;
	if (vh3.scn.num_of_bits==16)
	  bytes3=2;
	else
	  bytes3=1;

    d1_16 = (unsigned short *)malloc(size*2);

	if (vh2.scn.num_of_bits == 1)
	{
	    size2= (size*vh2.scn.num_of_bits+7)/8;
	    data2= (unsigned char *)malloc(size2);
	    d2_8 = (unsigned char *)malloc(size);
	}
	else
	{
		size2 = size;
		d2_16 = (unsigned short *)malloc(size*2);
	}

	d3_16 = (unsigned short *)malloc(size*2);

    VSeekData(in1,0);
    VSeekData(in2,0);
	VSeekData(in3,0);

    for (ii=0; ii<slices; ii++) {
      if (VReadData((char *)d1_16, bytes1, size, in1, &j)) {
        fprintf(stderr, "Could not read data\n");
        exit(-1);
      }
	  if (bytes1 == 1)
	    for (j=size-1; j>=0; j--)
		  d1_16[j] = ((unsigned char *)d1_16)[j];
      if (ii < slices2) {
	    if (vh2.scn.num_of_bits == 1)
		{
          if (VReadData((char *)data2,1,size2,in2,&j)) {
            fprintf(stderr, "Could not read data\n");
            exit(-1);
          }
          bin_to_grey(data2, size, d2_8, 0, 1);
		}
		else
		{
		  if (VReadData((char *)d2_16, 2,size2,in2,&j))
		  {
		    fprintf(stderr, "Could not read data\n");
			exit(-1);
		  }
		}
      }
      else
      {
	    if (vh2.scn.num_of_bits == 1)
		  memset(d2_8, 0, size);
		else
		  memset(d2_16, 0, size*2);
      }
	  if (VReadData((char *)d3_16, bytes3, size, in3, &j)) {
        fprintf(stderr, "Could not read data\n");
        exit(-1);
      }
	  if (bytes3 == 1)
	    for (j=size-1; j>=0; j--)
		  d3_16[j] = ((unsigned char *)d3_16)[j];

      if (vh2.scn.num_of_bits == 1)
	    Stat16(d1_16, d2_8, d3_16, size, final_factor_bqml);
	  else
	    Stat16_16(d1_16, d2_16, d3_16, size, final_factor_bqml);

    }

	free(d1_16);
	if (vh2.scn.num_of_bits == 1)
	{
	  free(data2);
	  free(d2_8);
	}
	else
	  free(d2_16);
	free(d3_16);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
	destroy_scene_header(&vh3);
	fclose(in1);
	fclose(in2);
	fclose(in3);
  }


  if (count) {
    get_stats(&mean);

    printf("Total number of pixels retained -> %.0f\n", count);
    printf("Maximum intensity               -> %f\n", peak_suv);
    printf("Average intensity               -> %f\n", mean);
	printf("Standard Dev.                   -> %f\n",
	  sqrt((sum_sqr-sum*mean)/(count-1)));
  }
  else {
    fprintf(stderr, "Mask is empty. No stats taken.\n");
  }

  exit(0);
}

void get_stats(double *mean)
{
  *mean= sum/count;
}




void Stat16(unsigned short *in1, unsigned char *msk, unsigned short *in2,
     int size, double final_factor_bqml)
{
  int i;

  for(i=0; i<size; msk++,in1++,in2++,i++)
    if (*msk) {
      double suv=final_factor_bqml * *in1 * *in2;
	  if (suv > peak_suv)
	    peak_suv = suv;
	  sum += suv;
	  sum_sqr += suv*suv;
      count++;
    }

}

void Stat16_16(unsigned short *in1, unsigned short *msk, unsigned short *in2,
     int size, double final_factor_bqml)
{
  int i;

  for(i=0; i<size; msk++,in1++,in2++,i++)
    if (*msk) {
      double suv=final_factor_bqml * *in1 * *in2;
	  if (suv > peak_suv)
	    peak_suv = suv;
	  sum += 1./65534 * *msk * suv;
	  sum_sqr += 1./65534 * *msk * suv*suv;
      count += 1./65534 * *msk;
    }

}


int get_slices(int dim, short *list)
{
  int i,sm;

  if (dim==3) return (int)(list[0]);
  if (dim==4) {
    for(sm=0,i=0;i<list[0];i++)
      sm+= list[1+i];
    return(sm);
  }
  return(0);

}

void bin_to_grey(unsigned char *bin_buffer, int length,
     unsigned char *grey_buffer, int min_value, int max_value)
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin, *grey;
  
  bin = bin_buffer;
  grey = grey_buffer;
  
  
  for(j=7,i=length; i>0; i--)    {
    if( (*bin & mask[j]) != 0) 
      *grey = max_value;
    else 
      *grey = min_value;
    
    grey++;
    if (j==0) {
      bin++; j=7;
    }
    else
      j--;
    
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

