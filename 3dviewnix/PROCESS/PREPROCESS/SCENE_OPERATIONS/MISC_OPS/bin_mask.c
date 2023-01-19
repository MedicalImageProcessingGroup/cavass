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
  Mask16(unsigned short *in, unsigned char *msk, int size),
  Mask8(unsigned char *in, unsigned char *msk, int size);
int get_slices(int dim, short *list);

static int hash_bg, bg_low, bg_high;

/* Modified: 8/4/00 exit code 0 returned on completion by Dewey Odhner. */
int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,size2,error,slices1,bytes;
  ViewnixHeader vh1,vh2,*outvh;
  FILE *in1,*in2,*out;
  unsigned char *data1,*data2,*d1_8,*d2_8;
  char group[6],elem[6];


  if (argc!= 4 && (argc!=5 || strcmp(argv[4], "-h"))) {
    printf("Usage: bin_mask <IM0_file> <BIM_file> <Output_IM0> [-h]\n");
    exit(-1);
  }
  hash_bg = argc==5;

  in1=fopen(argv[1],"rb");
  in2=fopen(argv[2],"rb");
  out=fopen(argv[3],"wb+");

  if (in1==NULL || in2==NULL || out==NULL) {
    printf("Error in opening the files\n");
    exit(-1);
  }
  
  error=VReadHeader(in1,&vh1,group,elem);
  if (error && error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  
  error=VReadHeader(in2,&vh2,group,elem);
  if (error && error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  
  if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0) {
    printf("Both input files should be IMAGE0\n");
    exit(-1);
  }
  if (vh2.scn.num_of_bits!=1) {
    printf("The Mask file should be binary\n");
    exit(-1);
  }
  if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] || vh1.scn.xysize[1]!=vh2.scn.xysize[1]) {
    printf("Both Scenes should be of the same width and height\n");
    exit(-1);
  }

  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  slices1= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
  if (slices < slices1) { 
    printf( "Mask Scene has more slices\n");
    printf( "Ignoring the extra slices\n");
  }

  outvh= &vh1;

  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
  if (vh1.scn.num_of_bits==16)
    bytes=2;
  else
    bytes=1;

  size1= (size*vh1.scn.num_of_bits+7)/8;
  data1= (unsigned char *)malloc(size1);
  if (vh1.scn.num_of_bits==1) {
    d1_8=(unsigned char *)malloc(size);
  }
  else
    d1_8=data1;


  size2= (size*vh2.scn.num_of_bits+7)/8;
  data2= (unsigned char *)malloc(size2);
  d2_8 = (unsigned char *)malloc(size);

  if (data1==NULL || d1_8==NULL || data2==NULL || d2_8==NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(-1);
  }

  if (vh1.scn.largest_density_value_valid)
    bg_high = (int)vh1.scn.largest_density_value[0];
  else
    bg_high = 0xffff >> (16-vh1.scn.num_of_bits);
  if (vh1.scn.smallest_density_value_valid)
    bg_low = (int)vh1.scn.smallest_density_value[0];
  else
    bg_low = 0;

  strncpy(outvh->gen.filename,argv[3], sizeof(outvh->gen.filename-1));
  outvh->gen.filename[sizeof(outvh->gen.filename-1)] = 0;
  outvh->gen.filename_valid=1;
  
  error=VWriteHeader(out,outvh,group,elem);
  if (error<=104) {
    printf("Fatal error in writing header\n");
    exit(-1);
  }
  
  if (VSeekData(in1,0) || VSeekData(in2,0) || VSeekData(out,0))
  {
    fprintf(stderr, "File seek error.\n");
	exit(-1);
  }

  for(i=0;i<slices;i++) {
    printf("Processing slice %d\r",i);
    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      printf("Could not read data\n");
      exit(-1);
    }
    if (vh1.scn.num_of_bits==1) 
      bin_to_grey(data1,size,d1_8,0,1);

    if (i < slices1) {
      if (VReadData((char *)data2,1,size2,in2,&j)) {
	printf("Could not read data\n");
	exit(-1);
      }
      bin_to_grey(data2,size,d2_8,0,1);
    }
    else 
      memset(d2_8,0,size);

    
    if (vh1.scn.num_of_bits==16)
      Mask16((unsigned short *)d1_8,d2_8,size);
    else
      Mask8(d1_8,d2_8,size);


    if (vh1.scn.num_of_bits==1) 
      VPackByteToBit(d1_8,size,data1);

    if (VWriteData((char *)data1,bytes,(int)(size1/bytes),out,&j)) {
      printf("Could not write data\n");
      exit(-1);
    }      

  }
  
  
  fclose(in1);
  fclose(in2);
  VCloseData(out);

  printf("done\n");

  exit(0);
}

void Mask16(unsigned short *in, unsigned char *msk, int size)
{
  int i;

  for(i=0;i<size;msk++,in++,i++)
    if ( !(*msk) )
      *in = hash_bg? i&1? bg_low: bg_high: 0;

}


void Mask8(unsigned char *in, unsigned char *msk, int size)
{
  int i;

  for(i=0;i<size;msk++,in++,i++)
    if ( !(*msk) )
      *in = hash_bg? i&1? bg_low: bg_high: 0;

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

