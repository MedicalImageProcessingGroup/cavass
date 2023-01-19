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
#include <cv3dv.h>

void bin_to_grey(unsigned char *bin_buffer, int length,
    unsigned char *grey_buffer, int min_value, int max_value),
 Stat16(unsigned short *in, unsigned char *msk, int size, unsigned char *msk3),
 Stat8(unsigned char *in, unsigned char *msk, int size,unsigned char *msk3);
void destroy_scene_header(ViewnixHeader *vh);
int get_threshold(), get_slices(int dim, short *list);
void get_thresholds(int *low, int *high, double *xor, int mandate,
		double *in_count, double *out_count);

static double in_hist[0x10000], out_hist[0x10000];

int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,size2,error,slices2,bytes, subject, two_flag=0, ns,
    slices3, mandate=-1;
  ViewnixHeader vh1, vh2, vh3;
  FILE *in1, *in2, *in3;
  unsigned char *data1,*data2,*data3, *d1_8,*d2_8,*d3_8;
  char group[6],elem[6];

  if (argc>4 && strcmp(argv[argc-2], "-m")==0 &&
      sscanf(argv[argc-1], "%d", &mandate)==1)
  	argc -= 2;
  if (argc>3 && sscanf(argv[argc-1], "-%d", &two_flag)==1 &&
      (two_flag==2||two_flag==3))
	argc--;
  if (argc<3 || argc%(two_flag? two_flag:2)!=1) {
    fprintf(stderr,
	  "Usage: min_xor_threshold <IM0_file> ... <BIM_file> ... [[-2] |  <BIM_file> ... -3] [-m <mandate>]\n");
    exit(-1);
  }
  ns = two_flag==3? (argc-1)/3: (argc-1)/2;

  for (subject=0; subject<ns; subject++)
  {
    in1=fopen(argv[1+subject],"rb");
    in2=fopen(argv[1+ns+subject],"rb");
	if (two_flag == 3)
	  in3=fopen(argv[1+2*ns+subject],"rb");

    if (in1==NULL || in2==NULL || (two_flag==3 && in3==NULL)) {
      fprintf(stderr,"Error in opening the files\n");
      exit(-1);
    }

    error=VReadHeader(in1,&vh1,group,elem);
    if (error<=104) {
      fprintf(stderr,"Fatal error in reading header\n");
      exit(-1);
    }

    error=VReadHeader(in2,&vh2,group,elem);
    if (error<=104) {
      fprintf(stderr,"Fatal error in reading header\n");
      exit(-1);
    }

	if (two_flag == 3)
	{
	  error=VReadHeader(in3,&vh3,group,elem);
	  if (error && error<=104) {
	    fprintf(stderr,"Fatal error in reading header\n");
		exit(-1);
	  }
	  if (vh3.scn.num_of_bits!=1) {
	    fprintf(stderr,"The Mask file should be binary\n");
		exit(-1);
	  }
	  slices3 = get_slices(vh3.scn.dimension, vh3.scn.num_of_subscenes);
	}

    if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0) {
      fprintf(stderr,"Both input files should be IMAGE0\n");
      exit(-1);
    }
    if (vh2.scn.num_of_bits!=1) {
      fprintf(stderr,"The Mask file should be binary\n");
      exit(-1);
    }
    if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] || vh1.scn.xysize[1]!=vh2.scn.xysize[1] || (two_flag==3 && (vh1.scn.xysize[0]!=vh3.scn.xysize[0] || vh1.scn.xysize[1]!=vh3.scn.xysize[1]))) {
      fprintf(stderr,"Both Scenes should be of the same width and height\n");
      exit(-1);
    }

    slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
    slices2= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
    if (slices < slices2) {
      fprintf(stderr, "Mask Scene has more slices\n");
      fprintf(stderr, "Ignoring the extra slices\n");
    }
	if (two_flag!=3 || slices3>slices)
	  slices3 = slices;

    size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
    if (vh1.scn.num_of_bits==16)
      bytes=2;
    else
      bytes=1;

    size1= (size*vh1.scn.num_of_bits+7)/8;
    data1= (unsigned char *)malloc(size1);
    d1_8= (unsigned char *)malloc(size1*2);

    size2= (size*vh2.scn.num_of_bits+7)/8;
    data2= (unsigned char *)malloc(size2);
    d2_8 = (unsigned char *)malloc(size*2);

	data3= (unsigned char *)malloc(size2);
	d3_8 = (unsigned char *)malloc(size*2);

    VSeekData(in1,0);
    VSeekData(in2,0);

    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      fprintf(stderr,"Could not read data\n");
      exit(-1);
    }
    if (vh1.scn.num_of_bits==1)
      bin_to_grey(data1,size,d1_8+size,0,1);
    else
      memcpy(d1_8+size1, data1, size1);
    if (VReadData((char *)data2,1,size2,in2,&j)) {
      fprintf(stderr,"Could not read data\n");
      exit(-1);
    }
    bin_to_grey(data2,size,d2_8+size,0,1);
	if (two_flag == 3)
	{
	  VSeekData(in3,0);
	  if (VReadData((char *)data3,1,size2,in3,&j)) {
	    fprintf(stderr,"Could not read data\n");
	    exit(-1);
	  }
	  bin_to_grey(data3,size,d3_8+size,0,1);
	}

    for(i=0;i<slices3;i++) {
      memcpy(d1_8, d1_8+size1, size1);
      memcpy(d2_8, d2_8+size, size);
	  if (two_flag == 3)
	    memcpy(d3_8, d3_8+size, size);
      if (i < slices-1)
      {
        if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
          fprintf(stderr,"Could not read data\n");
          exit(-1);
        }
        if (vh1.scn.num_of_bits==1)
          bin_to_grey(data1,size,d1_8+size,0,1);
        else
          memcpy(d1_8+size1, data1, size1);
      }

      if (i < slices2-1) {
        if (VReadData((char *)data2,1,size2,in2,&j)) {
          fprintf(stderr,"Could not read data\n");
          exit(-1);
        }
        bin_to_grey(data2,size,d2_8+size,0,1);
		if (two_flag == 3)
		{
		  if (VReadData((char *)data3,1,size2,in3,&j)) {
		    fprintf(stderr,"Could not read data\n");
			exit(-1);
		  }
		  bin_to_grey(data3,size,d3_8+size,0,1);
		}
      }
      else
	  {
        memset(d2_8+size,0,size);
		memset(d3_8+size,0,size);
	  }

      if (vh1.scn.num_of_bits==16)
      {
        Stat16((unsigned short *)d1_8,d2_8,size,two_flag==3?d3_8:NULL);
      }
      else
      {
        Stat8(d1_8,d2_8,size,two_flag==3?d3_8:NULL);
      }

    }

	free(data1);
	free(d1_8);
	free(data2);
	free(d2_8);
	free(data3);
	free(d3_8);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
	fclose(in1);
	fclose(in2);
	if (two_flag == 3)
	{
	  destroy_scene_header(&vh3);
	  fclose(in3);
	}
  }

  if (two_flag)
  {
    double xor, in_count, out_count;
    get_thresholds(&i, &j, &xor, mandate, &in_count, &out_count);
	printf("%d %d %.0f %f\n", i, j, xor,
	  (in_count<out_count? in_count:out_count)/xor);
  }
  else
    printf("%d\n", get_threshold());

  exit(0);
}

int get_threshold()
{
	int j, best_threshold=0, max_dens=0;
	double in_count=0, out_count=0, xor, min_xor;

	for (j=0; j<0x10000; j++)
	{
		if (in_hist[j])
		{
			in_count += in_hist[j];
			max_dens = j;
		}
		if (out_hist[j])
		{
			out_count += out_hist[j];
			max_dens = j;
		}
	}
	for (j=0,min_xor=xor=out_count; j<=max_dens; j++)
	{
		xor = xor-out_hist[j]+in_hist[j];
		if (xor < min_xor)
		{
			best_threshold = j+1;
			min_xor = xor;
		}
	}
	return best_threshold;
}

void get_thresholds(int *low, int *high, double *xor, int mandate,
		double *in_count, double *out_count)
{
	int j, max_dens=0, min_dens=0xffff;
	double *cum, cum_low=0;

	if (mandate >= 0)
	{
		min_dens = mandate;
		for (j=0; j<mandate; j++)
			if (in_hist[j] > out_hist[j])
			{
				min_dens = j;
				break;
			}
		max_dens = mandate;
		for (j=0xffff; j>=mandate; j--)
			if (in_hist[j] > out_hist[j])
			{
				max_dens = j;
				break;
			}
	}
	else
	{
		for (j=0; j<0x10000; j++)
			if (in_hist[j] > out_hist[j])
			{
				min_dens = j;
				break;
			}
		for (j=0xffff; j>=0; j--)
			if (in_hist[j] > out_hist[j])
			{
				max_dens = j;
				break;
			}
		if (min_dens > max_dens)
		{
			*low = min_dens;
			*high = max_dens;
			*xor = 0;
			for (j=0; j<0x10000; j++)
				*xor += in_hist[j];
			return;
		}
	}
	cum = (double *)malloc((max_dens-min_dens+1)*sizeof(double));
	cum[0] = in_hist[min_dens]-out_hist[min_dens];
	*low = min_dens;
	if (mandate >= 0)
	{
		*high = mandate;
		for (j=min_dens+1; j<=mandate; j++)
			cum[j-min_dens] = cum[j-min_dens-1]+in_hist[j]-out_hist[j];
		for (; j<=max_dens; j++)
		{
			cum[j-min_dens] = cum[j-min_dens-1]+in_hist[j]-out_hist[j];
			if (cum[j-min_dens] > cum[*high-min_dens])
				*high = j;
		}
		for (j=min_dens+1; j<=mandate; j++)
			if (cum[j-min_dens] <= cum_low)
			{
				cum_low = cum[j-min_dens-1];
				*low = j;
			}
	}
	else
	{
		*high = min_dens;
		for (j=min_dens+1; j<=max_dens; j++)
		{
			cum[j-min_dens] = cum[j-min_dens-1]+in_hist[j]-out_hist[j];
			if (cum[j-min_dens] > cum[*high-min_dens])
				*high = j;
		}
		for (j=min_dens+1; j<=*high; j++)
			if (cum[j-min_dens] <= cum_low)
			{
				cum_low = cum[j-min_dens-1];
				*low = j;
			}
	}
	free(cum);
	*xor = *in_count = *out_count = 0;
	for (j=0; j<*low; j++)
	{
		*xor += in_hist[j];
		*out_count += out_hist[j];
	}
	for (; j<=*high; j++)
	{
		*xor += out_hist[j];
		*in_count += in_hist[j];
	}
	for (; j<0x10000; j++)
	{
		*xor += in_hist[j];
		*out_count += out_hist[j];
	}
}


void Stat16(unsigned short *in, unsigned char *msk, int size,
    unsigned char *msk3)
{
  int i;

  for(i=0;i<size;msk++,in++,i++)
    if (msk3==NULL || msk3[i])
	{
	  if (*msk) {
        in_hist[*in]++;
      }
	  else
	  {
	    out_hist[*in]++;
	  }
	}
}


void Stat8(unsigned char *in, unsigned char *msk, int size,unsigned char *msk3)
{
  int i;

  for(i=0;i<size;msk++,in++,i++)
    if (msk3==NULL || msk3[i])
	{
	  if (*msk) {
        in_hist[*in]++;
      }
	  else
	  {
	    out_hist[*in]++;
	  }
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

