/*
  Copyright 1993-2015, 2021 Medical Image Processing Group
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
  Mask16(unsigned short *in, unsigned char *msk, unsigned char *outdata1,
    unsigned char *outdata2, int xsize, int ysize, int zsize),
  Mask8(unsigned char *in, unsigned char *msk, unsigned char *outdata1,
    unsigned char *outdata2, int xsize, int ysize, int zsize);
int get_slices(int dim, short *list);


/* Modified: 8/4/00 exit code 0 returned on completion by Dewey Odhner. */
int main(argc,argv)
int argc;
char *argv[];
{

  int k,j,slices,size,size1,size2,error,slices1,bytes;
  ViewnixHeader vh1,vh2, outvh;
  FILE *in1, *in2, *out1, *out2;
  unsigned char *data1, *data2, *d1_8, *d2_8, *outdata1, *outdata2, *binslc;
  char group[6],elem[6];


  if (argc != 5)
  {
    printf(
	  "Usage: direction <IM0_file> <BIM_file> <Inward_BIM> <Outward_BIM>\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  in2=fopen(argv[2],"rb");
  out1=fopen(argv[3],"wb+");
  out2=fopen(argv[4],"wb+");

  if (in1==NULL || in2==NULL || out1==NULL || out2==NULL) {
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

  outvh = vh1;
  outvh.scn.num_of_bits = 1;
  outvh.scn.bit_fields[0] = outvh.scn.bit_fields[1] = 0;
  if (outvh.scn.smallest_density_value_valid)
    outvh.scn.smallest_density_value[0] = 0;
  if (outvh.scn.largest_density_value_valid)
    outvh.scn.largest_density_value[0] = 1;

  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
  if (vh1.scn.num_of_bits==16)
    bytes=2;
  else
    bytes=1;

  size1= (size*vh1.scn.num_of_bits+7)/8;
  size2= (size+7)/8;
  d1_8 = (unsigned char *)malloc(size*4);
  binslc = (unsigned char *)malloc(size2);
  if (vh1.scn.num_of_bits==1)
    data1 = binslc;
  else
    data1 = d1_8+size1;

  data2= (unsigned char *)malloc(size2);
  d2_8 = (unsigned char *)malloc(size*2);
  outdata1 = (unsigned char *)malloc(size*2);
  outdata2 = (unsigned char *)malloc(size*2);

  if (data1==NULL || d1_8==NULL || data2==NULL || d2_8==NULL ||
      outdata1==NULL || outdata2==NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(-1);
  }

  strncpy(outvh.gen.filename,argv[3], sizeof(outvh.gen.filename)-1);
  outvh.gen.filename[sizeof(outvh.gen.filename)-1] = 0;
  outvh.gen.filename_valid=1;

  error=VWriteHeader(out1, &outvh,group,elem);
  if (error && error<=104) {
    printf("Fatal error in writing header\n");
    exit(-1);
  }

  strncpy(outvh.gen.filename,argv[4], sizeof(outvh.gen.filename)-1);
  outvh.gen.filename[sizeof(outvh.gen.filename)-1] = 0;

  error=VWriteHeader(out2, &outvh,group,elem);
  if (error && error<=104) {
    printf("Fatal error in writing header\n");
    exit(-1);
  }

  if (VSeekData(in1,0) || VSeekData(in2,0) || VSeekData(out1,0) ||
      VSeekData(out2,0))
  {
    fprintf(stderr, "File seek error.\n");
	exit(-1);
  }

  for (k=-1; k<slices; k++) {
    if (k < slices-1)
	{
	  if (vh1.scn.num_of_bits == 1)
	  {
        if (VReadData((char *)data1, bytes, size2, in1, &j)) {
          printf("Could not read data\n");
          exit(-1);
        }
        bin_to_grey(data1, size, d1_8+size1, 0, 1);
	  }
	  else
	    if (VReadData((char *)data1, bytes, size, in1, &j)) {
          printf("Could not read data\n");
          exit(-1);
        }
    }
	if (k < slices1-1) {
      if (VReadData((char *)data2, 1, size2, in2, &j)) {
		printf("Could not read data\n");
		exit(-1);
      }
      bin_to_grey(data2, size, d2_8+size, 0, 1);
    }
    else 
      memset(d2_8+size, 0, size);

	if (k >= 0)
	{
      if (vh1.scn.num_of_bits==16)
        Mask16((unsigned short *)d1_8, d2_8, outdata1, outdata2,
		  vh1.scn.xysize[0], vh1.scn.xysize[1], k<slices-1? 2:1);
      else
        Mask8(d1_8, d2_8, outdata1, outdata2,
		  vh1.scn.xysize[0], vh1.scn.xysize[1], k<slices-1? 2:1);
      VPackByteToBit(outdata1, size, binslc);
      if (VWriteData((char *)binslc, 1, size2, out1, &j)) {
        printf("Could not write data\n");
        exit(-1);
      }
      VPackByteToBit(outdata2, size, binslc);
      if (VWriteData((char *)binslc, 1, size2, out2, &j)) {
        printf("Could not write data\n");
        exit(-1);
      }
	  memcpy(outdata1, outdata1+size, size);
	  memcpy(outdata2, outdata2+size, size);
	}
	else
	{
	  memset(outdata1, 0, size);
	  memset(outdata2, 0, size);
	}
	memcpy(d1_8, d1_8+size1, size1);
	memcpy(d2_8, d2_8+size, size);
	memset(outdata1+size, 0, size);
	memset(outdata2+size, 0, size);
  }

  fclose(in1);
  fclose(in2);
  VCloseData(out1);
  VCloseData(out2);
  exit(0);
}

void Mask16(unsigned short *in, unsigned char *msk, unsigned char *outdata1,
    unsigned char *outdata2, int xsize, int ysize, int zsize)
{
	int x, y, j;

	for (y=j=0; y<ysize; y++)
		for (x=0; x<xsize; x++, j++)
		{
			if (x<xsize-1 && msk[j]!=msk[1+j])
			{
				if ((msk[j] && in[1+j]>in[j]) || (msk[1+j] && in[j]>in[1+j]))
					outdata1[j] = outdata1[1+j] = 1;
				if ((msk[1+j] && in[1+j]>in[j]) || (msk[j] && in[j]>in[1+j]))
					outdata2[j] = outdata2[1+j] = 1;
			}
			if (y<ysize-1 && msk[j]!=msk[xsize+j])
			{
				if ((msk[j] && in[xsize+j]>in[j]) ||
						(msk[xsize+j] && in[j]>in[xsize+j]))
					outdata1[j] = outdata1[xsize+j] = 1;
				if ((msk[xsize+j] && in[xsize+j]>in[j]) ||
						(msk[j] && in[j]>in[xsize+j]))
					outdata2[j] = outdata2[xsize+j] = 1;
			}
			if (zsize==2 && msk[j]!=msk[xsize*ysize+j])
			{
				if ((msk[j] && in[xsize*ysize+j]>in[j]) ||
						(msk[xsize*ysize+j] && in[j]>in[xsize*ysize+j]))
					outdata1[j] = outdata1[xsize*ysize+j] = 1;
				if ((msk[xsize*ysize+j] && in[xsize*ysize+j]>in[j]) ||
						(msk[j] && in[j]>in[xsize*ysize+j]))
					outdata2[j] = outdata2[xsize*ysize+j] = 1;
			}
		}
}


void Mask8(unsigned char *in, unsigned char *msk, unsigned char *outdata1,
    unsigned char *outdata2, int xsize, int ysize, int zsize)
{
	int x, y, j;

	for (y=j=0; y<ysize; y++)
		for (x=0; x<xsize; x++, j++)
		{
			if (x<xsize-1 && msk[j]!=msk[1+j])
			{
				if ((msk[j] && in[1+j]>in[j]) || (msk[1+j] && in[j]>in[1+j]))
					outdata1[j] = outdata1[1+j] = 1;
				if ((msk[1+j] && in[1+j]>in[j]) || (msk[j] && in[j]>in[1+j]))
					outdata2[j] = outdata2[1+j] = 1;
			}
			if (y<ysize-1 && msk[j]!=msk[xsize+j])
			{
				if ((msk[j] && in[xsize+j]>in[j]) ||
						(msk[xsize+j] && in[j]>in[xsize+j]))
					outdata1[j] = outdata1[xsize+j] = 1;
				if ((msk[xsize+j] && in[xsize+j]>in[j]) ||
						(msk[j] && in[j]>in[xsize+j]))
					outdata2[j] = outdata2[xsize+j] = 1;
			}
			if (zsize==2 && msk[j]!=msk[xsize*ysize+j])
			{
				if ((msk[j] && in[xsize*ysize+j]>in[j]) ||
						(msk[xsize*ysize+j] && in[j]>in[xsize*ysize+j]))
					outdata1[j] = outdata1[xsize*ysize+j] = 1;
				if ((msk[xsize*ysize+j] && in[xsize*ysize+j]>in[j]) ||
						(msk[j] && in[j]>in[xsize*ysize+j]))
					outdata2[j] = outdata2[xsize*ysize+j] = 1;
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

