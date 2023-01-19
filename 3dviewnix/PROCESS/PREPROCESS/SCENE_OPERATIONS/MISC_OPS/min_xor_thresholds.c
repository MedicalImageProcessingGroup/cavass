/*
  Copyright 1993-2014, 2016 Medical Image Processing Group
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
 Stat16(unsigned short *in, unsigned short *msk, int size),
 Stat8(unsigned char *in, unsigned short *msk, int size);
void destroy_scene_header(ViewnixHeader *vh);
int get_thresholds(int *low, int *high, double *xor);

static double diff_hist[0x10000], cum_diff[0x10000];

int main(argc,argv)
int argc;
char *argv[];
{

  int i,j, slices, size, size1, size2, error, slices2 ,bytes, subject,
    offset[3], start_x, start_y, stop_x, stop_y;
  ViewnixHeader vh1, vh2;
  FILE *in1, *in2, *outstream;
  unsigned char *data1, *d1_8;
  unsigned short *data2, *shifted_data2;
  char group[6],elem[6];
  double tot_size=0;

  if (argc>2 && strcmp(argv[argc-2], "-o")==0)
  {
    outstream = fopen(argv[argc-1], "wb");
    argc -= 2;
  }
  else
    outstream = stdout;
  if (argc>6 && strcmp(argv[argc-4], "-offset")==0 &&
      sscanf(argv[argc-3], "%d", offset)==1 &&
	  sscanf(argv[argc-2], "%d", offset+1)==1 &&
	  sscanf(argv[argc-1], "%d", offset+2)==1)
    argc -= 4;
  else
    memset(offset, 0, sizeof(offset));
  if (argc<3 || argc%2==0) {
    fprintf(stderr,
	  "Usage: min_xor_thresholds <IM0_file> ... [-offset <x> <y> <z>] [-o <os>]\n");
    exit(-1);
  }

  for (subject=0; subject<(argc-1)/2; subject++)
  {
    in1=fopen(argv[1+subject],"rb");
    in2=fopen(argv[(argc+1)/2+subject],"rb");

    if (in1==NULL || in2==NULL ) {
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

    if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0) {
      fprintf(stderr,"Both input files should be IMAGE0\n");
      exit(-1);
    }
    if (vh2.scn.num_of_bits!=16) {
      fprintf(stderr,"The Mask file should be 16-bit\n");
      exit(-1);
    }

    slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
    slices2= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);

    size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
    if (vh1.scn.num_of_bits==16)
      bytes=2;
    else
      bytes=1;

    size1= (size*vh1.scn.num_of_bits+7)/8;
    data1= (unsigned char *)malloc(size1);
    d1_8= (unsigned char *)malloc(size1);

    size2= (vh2.scn.xysize[0]*vh2.scn.xysize[1]*vh2.scn.num_of_bits+7)/8;
    data2= (unsigned short *)malloc(size2);
	shifted_data2 = (unsigned short *)calloc(size, 2);

	tot_size += size*slices;
	start_x = offset[0]>0? offset[0]: 0;
	stop_x = offset[0]+vh2.scn.xysize[0]<vh1.scn.xysize[0]?
	         offset[0]+vh2.scn.xysize[0]:vh1.scn.xysize[0];
	start_y = offset[1]>0? offset[1]: 0;
	stop_y = offset[1]+vh2.scn.xysize[1]<vh1.scn.xysize[1]?
	         offset[1]+vh2.scn.xysize[1]:vh1.scn.xysize[1];

    VSeekData(in1,0);
	if (offset[2] < 0)
	  VSeekData(in2, -offset[2]*size2);
	else
      VSeekData(in2,0);

    for(i=0;i<slices;i++) {
        if (VReadData((char *)data1, bytes, size, in1, &j)) {
          fprintf(stderr,"Could not read data\n");
          exit(-1);
        }
        if (vh1.scn.num_of_bits==1)
          bin_to_grey(data1,size,d1_8,0,1);
        else
          memcpy(d1_8, data1, size1);
        if (i-offset[2]>=0 && i-offset[2]<slices2) {
          if (VReadData((char *)data2, 2, vh2.scn.xysize[0]*vh2.scn.xysize[1],in2,&j))
		  {
            fprintf(stderr,"Could not read data\n");
            exit(-1);
          }
		  for (j=start_y; j<stop_y; j++)
		    memcpy(shifted_data2+start_x+j*vh1.scn.xysize[0],
			  data2+start_x-offset[0]+vh2.scn.xysize[0]*(j-offset[1]),
			  2*(stop_x-start_x));
        }
        else
          memset(shifted_data2, 0, size1);
        if (vh1.scn.num_of_bits==16)
          Stat16((unsigned short *)d1_8, shifted_data2, size);
        else
          Stat8(d1_8, shifted_data2, size);
    }

	free(data1);
	free(d1_8);
	free(data2);
	free(shifted_data2);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
	fclose(in1);
	fclose(in2);
  }

  {
    double xor=tot_size*65534;
    get_thresholds(&i, &j, &xor);
	fprintf(outstream, "%d %d %.0f\n", i, j, xor);
  }

  exit(0);
}

int get_thresholds(int *low, int *high, double *xor)
{
	int j, max_dens=0, min_dens=0xffff;
	double cum_low=0;

	for (j=0; j<0x10000; j++)
		if (diff_hist[j] > 0)
		{
			min_dens = j;
			break;
		}
	for (j=0xffff; j>=0; j--)
		if (diff_hist[j] > 0)
		{
			max_dens = j;
			break;
		}
	if (min_dens > max_dens)
	{
		*low = min_dens;
		*high = max_dens;
		return -1;
	}
	cum_diff[min_dens] = diff_hist[min_dens];
	*low = *high = min_dens;
	for (j=min_dens+1; j<=max_dens; j++)
	{
		cum_diff[j] = cum_diff[j-1]+diff_hist[j];
		if (cum_diff[j] > cum_diff[*high])
			*high = j;
	}
	for (j=min_dens+1; j<=*high; j++)
		if (cum_diff[j] <= cum_low)
		{
			cum_low = cum_diff[j-1];
			*low = j;
		}
	for (j=0; j<*low; j++)
		*xor += diff_hist[j];
	for (; j<=*high; j++)
		*xor -= diff_hist[j];
	for (; j<0x10000; j++)
		*xor += diff_hist[j];
	return 0;
}


void Stat16(unsigned short *in, unsigned short *msk, int size)
{
  int i;

  for(i=0; i<size; msk++,in++,i++)
    diff_hist[*in] += 2.* *msk -65534;
}


void Stat8(unsigned char *in, unsigned short *msk, int size)
{
  int i;

  for(i=0; i<size; msk++,in++,i++)
    diff_hist[*in] += 2.* *msk -65534;
}


int get_slices(dim,list)
int dim;
short *list;
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

void bin_to_grey(bin_buffer, length, grey_buffer, min_value, max_value)
unsigned char *bin_buffer;
int length;
unsigned char *grey_buffer;
int min_value, max_value;
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

