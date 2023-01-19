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

#include <cv3dv.h>

//----------------------------------------------------------------------
void Scale8(in,out,size,min,max)
unsigned char *in,*out;
int size,min,max;
{
  int i;

  for(i=0;i<size;in++,out++,i++) {
    if ( *in <= min)
      *out= 0;
    else if ( *in < max) 
      *out = (unsigned char)( 255.0*((float)(*in) - min)/(max-min+1.0));
    else
      *out=255;
  }
}
//----------------------------------------------------------------------
void Scale16(in,out,size,min,max)
unsigned short *in;
unsigned char *out;
int size,min,max;
{
  int i;

  
  for(i=0;i<size;in++,out++,i++) {
    if (*in<=min)
      *out= 0;
    else if (*in < max) 
      *out = (unsigned char)( 255.0*((float)(*in) - min)/(max-min+1.0));
    else
      *out=255;
  }
}
//----------------------------------------------------------------------
void Scale16_16(in,out,size,min,max)
unsigned short *in;
unsigned short *out;
int size,min,max;
{
  int i;

  
  for(i=0;i<size;in++,out++,i++) {
    if (*in<=min)
      *out= 0;
    else if (*in < max) 
      *out = (unsigned short)( 65535.0*((float)(*in) - min)/(max-min+1.0));
    else
      *out=65535;
  }
}
//----------------------------------------------------------------------
void WritePGM(file,w,h,d)
char *file;
int w,h;
unsigned char *d;
{
  FILE *fp;

  fp=fopen(file,"wb");
  if (fp==NULL) {
    printf("Could not open %s\n",file);
    exit(-1);
  }

  fprintf(fp,"P5\n");
  fprintf(fp,"%d %d\n",w,h);
  fprintf(fp,"255\n");
  if (fwrite(d,1,w*h,fp)!=w*h) {
    fprintf(stderr,"Error in writing data\n");
    fclose(fp);
    exit(-1);
  }
  fclose(fp);
}
//----------------------------------------------------------------------
void WritePGM16(file,w,h,d,m)
char *file;
int w,h;
unsigned short *d;
int m;
{
  FILE *fp;
  int j;

  fp=fopen(file,"wb");
  if (fp==NULL) {
    printf("Could not open %s\n",file);
    exit(-1);
  }

  fprintf(fp,"P5\n");
  fprintf(fp,"%d %d\n",w,h);
  fprintf(fp,"%d\n", m);
  if (VWriteData((char *)d, 2, w*h, fp, &j)) {
    fprintf(stderr,"Error in writing data\n");
    fclose(fp);
    exit(-1);
  }
  fclose(fp);
}
//----------------------------------------------------------------------
void invert(s,d)
int s;
unsigned char *d;
{
  for(;s>0;d++,s--)
    *d = ~(*d);
}    
//----------------------------------------------------------------------
void WritePBM(file,w,h,d)
char *file;
int w,h;
unsigned char *d;
{
  int size;
  FILE *fp;

  fp=fopen(file,"wb");
  if (fp==NULL) {
    printf("Could not open %s\n",file);
    exit(-1);
  }

  fprintf(fp,"P4\n");
  fprintf(fp,"%d %d\n",w,h);

  size= (w+7)/8*h;
  if (fwrite(d,1,size,fp)!=size) {
    fprintf(stderr,"Error in writing data\n");
    fclose(fp);
    exit(-1);
  }

  fclose(fp);
}
//----------------------------------------------------------------------
int get_slices(dim,list)
int dim;
short *list;
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
//----------------------------------------------------------------------
int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,bytes,slices,size,from_slice,to_slice,min,max,error,bits, out_bits=8;
  ViewnixHeader vh;
  FILE *in;  //,*out;
  unsigned char *data,*data1, *data2;
//  unsigned short *data16;
  char outfile[150],group[6],elem[6];

  if (argc>5 && sscanf(argv[argc-1], "out_bits=%d", &out_bits)==1)
    argc--;
  if (out_bits!=8 && out_bits!=16)
  {
    fprintf(stderr,"%d bit output not supported.\n", out_bits);
	exit(-1);
  }
  if (argc!=5 && argc!=7) {
    fprintf(stderr,"Usage: IM0_to_pgm <inputfile> from_slice to_slice <output_without_extension> [min max] [out_bits=16]\n");
    exit(-1);
  }
  sscanf(argv[2],"%d",&from_slice);
  sscanf(argv[3],"%d",&to_slice);


  in=fopen(argv[1],"rb");
  if (in==NULL) {
    fprintf(stderr,"Error in opening the files\n");
    exit(-1);
  }

  error=VReadHeader(in,&vh,group,elem);
  if (error<=104) {
    fprintf(stderr,"Fatal error in reading header\n");
    exit(-1);
  }

  if (vh.gen.data_type!=IMAGE0) {
    fprintf(stderr,"This is not an IMAGE0 file\n");
    exit(-1);
  }

  if (vh.scn.num_of_bits!=1 && vh.scn.num_of_bits!=8 && vh.scn.num_of_bits!=16) {
    fprintf(stderr,"Cannont handle %d bit data\n",vh.scn.num_of_bits);
    exit(-1);
  }
  bits=vh.scn.num_of_bits;

  if (vh.scn.num_of_bits==16) 
    bytes=2;
  else
    bytes=1;

  if (argc==7)  {
    sscanf(argv[5],"%d",&min);
    sscanf(argv[6],"%d",&max);
  }
  else {
    min = (int) vh.scn.smallest_density_value[0];
    max = (int) vh.scn.largest_density_value[0];
  }

  slices=get_slices(vh.scn.dimension,vh.scn.num_of_subscenes);

  if (from_slice==-1 && to_slice==-1) {
    from_slice=0; to_slice=slices-1;
  }

  if (from_slice < 0 || from_slice > slices-1) {
    fprintf(stderr,"Incorrect slices specified\n");
    exit(-1);
  }

  if (to_slice < from_slice ||
      to_slice >= slices) {
    fprintf(stderr,"Incorrect slices specified\n");
    exit(-1);
  }

  if (bits!=1) {
    size= (vh.scn.xysize[0]*vh.scn.xysize[1]);
    data= (unsigned char *)malloc(size*bytes);
    data1= (unsigned char *)malloc(size);
    if (data==NULL || data1==NULL) {
      fprintf(stderr,"Could not allocate data. Aborting conversion\n");
      exit(-1);
    }
  }
  else {
    size = (vh.scn.xysize[0]*vh.scn.xysize[1]+7)/8;
    data = (unsigned char *)malloc(size);
    data1= (unsigned char *)malloc(vh.scn.xysize[0]*vh.scn.xysize[1]);
	data2= (unsigned char *)malloc((vh.scn.xysize[0]+7)/8);
    if (data==NULL || data1==NULL) {
      fprintf(stderr,"Could not allocate data. Aborting conversion\n");
      exit(-1);
    }

  }

  VSeekData(in,from_slice*size*bytes);

  switch (bytes) {
  case 1:
    for(i=from_slice;i<=to_slice;i++) {
      printf("Computing pgm file for slice %d\n",i);

      if (VReadData((char *)data,bytes,size,in,&j)) {
        fprintf(stderr,"Could not read data\n");
        exit(-1);
      }
      if (bits==8) {
        sprintf(outfile,"%s_%d.pgm",argv[4],i);
        Scale8(data,data1,size,min,max);
        WritePGM(outfile,vh.scn.xysize[0],vh.scn.xysize[1],data1);
      }
      else {
        sprintf(outfile,"%s_%d.pbm",argv[4],i);
        for (j=0; j<vh.scn.xysize[0]*vh.scn.xysize[1]; j++)
		  data1[j] = data[j/8]&(128>>(j%8));
		for (j=0; j<vh.scn.xysize[1]; j++) {
          VPackByteToBit(data1+j*vh.scn.xysize[0], vh.scn.xysize[0], data2);
          memcpy(data1+(vh.scn.xysize[0]+7)/8*j, data2,
            (vh.scn.xysize[0]+7)/8*j);
        }
        invert((vh.scn.xysize[0]+7)/8*vh.scn.xysize[1], data1);
        WritePBM(outfile,vh.scn.xysize[0],vh.scn.xysize[1],data1);
      }
    }
    break;
  case 2:
    for(i=from_slice;i<=to_slice;i++) {
      printf("Computing pgm file for slice %d\n",i);
      sprintf(outfile,"%s_%d.pgm",argv[4],i);
      if (VReadData((char *)data,bytes,size,in,&j)) {
        fprintf(stderr,"Could not read data\n");
        exit(-1);
      }
	  if (out_bits == 16)
	  {
	    WritePGM16(outfile,vh.scn.xysize[0],vh.scn.xysize[1],
		  (unsigned short *)data, max);
	  }
	  else
	  {
        Scale16((unsigned short *)data,data1,size,min,max);
        WritePGM(outfile,vh.scn.xysize[0],vh.scn.xysize[1],data1);
	  }
    }
    break;
  }
  
  
  fclose(in);
  exit(0);
}
//----------------------------------------------------------------------
