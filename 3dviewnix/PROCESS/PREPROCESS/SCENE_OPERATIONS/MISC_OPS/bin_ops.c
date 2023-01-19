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

#include <cv3dv.h>


int get_slices(int dim, short *list);

void Do_Or(unsigned char *d1, unsigned char *d2, int size),
 Do_Nor(unsigned char *d1, unsigned char *d2, int size),
 Do_Xor(unsigned char *d1, unsigned char *d2, int size),
 Do_Xnor(unsigned char *d1, unsigned char *d2, int size),
 Do_And(unsigned char *d1, unsigned char *d2, int size),
 Do_Nand(unsigned char *d1, unsigned char *d2, int size),
 Do_AminusB(unsigned char *d1, unsigned char *d2, int size);

int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,OPT,error,slices1,max_slices;
  ViewnixHeader vh1,vh2,*outvh;
  FILE *in1,*in2,*out;
  unsigned char *data1,*data2;
  char group[6],elem[6];

  if (argc!= 5) {
    printf("Usage: bin_ops <inputfile1> <inputfile2> <output_file> option\n");
    printf("Valid options = or, nor , xor, xnor, and, nand, a-b\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  in2=fopen(argv[2],"rb");
  out=fopen(argv[3],"wb+");

  if (in1==NULL || in2==NULL || out==NULL) {
    printf("Error in opening the files\n");
    exit(-1);
  }
  
  if (!strcmp(argv[4],"or"))   // if (!strcasecmp(argv[4],"or")) 
    OPT=1;
  else if (!strcmp(argv[4],"nor")) 
    OPT=2;
  else if (!strcmp(argv[4],"xor")) 
    OPT=3;
  else if (!strcmp(argv[4],"xnor")) 
    OPT=4;
  else if (!strcmp(argv[4],"and")) 
    OPT=5;
  else if (!strcmp(argv[4],"nand")) 
    OPT=6;
  else if (!strcmp(argv[4],"a-b")) 
    OPT=7;
  else {
    printf("Invalid option specified\n");
    printf("Valid options = or, nor , xor, xnor, and, nand, a-b\n");
    exit(-1);
  }

  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  
  error=VReadHeader(in2,&vh2,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  

  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  slices1= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
  if (slices != slices1) { 
    printf( "Scenes have different number of slices\n");
    printf( "Padding the smaller data set with empt slices\n");
  }

  if (slices> slices1) {
    max_slices=slices;
    outvh= &vh1;
  }
  else {
    max_slices=slices1;
    outvh= &vh2;
  }
  
  size= (outvh->scn.xysize[0]* outvh->scn.xysize[1]+7)/8;
  data1= (unsigned char *)malloc(size);
  data2= (unsigned char *)malloc(size);
  
  strncpy(outvh->gen.filename,argv[3], sizeof(outvh->gen.filename)-1);
  outvh->gen.filename[sizeof(outvh->gen.filename)-1] = 0;
  outvh->gen.filename_valid=1;
  
  error=VWriteHeader(out,outvh,group,elem);
  if (error<=104) {
    printf("Fatal error in writing header\n");
    exit(-1);
  }
  
  VSeekData(in1,0);
  VSeekData(in2,0);
  VSeekData(out,0);


  for(i=0;i<max_slices;i++) {
    printf("Processing slice %d\r",i);
    if (i< slices) {
      if (VReadData((char *)data1,1,size,in1,&j)) {
	printf("Could not read data\n");
	exit(-1);
      }
    }
    else 
      memset(data1,0,size);

    if (i < slices1) {
      if (VReadData((char *)data2,1,size,in2,&j)) {
	printf("Could not read data\n");
	exit(-1);
      }
    }
    else 
      memset(data2,0,size);

    switch (OPT) {
    case 1:
      Do_Or(data1,data2,size);
      break;
    case 2:
      Do_Nor(data1,data2,size);
      break;
    case 3:
      Do_Xor(data1,data2,size);
      break;
    case 4:
      Do_Xnor(data1,data2,size);
      break;
    case 5:
      Do_And(data1,data2,size);
      break;
    case 6:
      Do_Nand(data1,data2,size);
      break;
    case 7:
      Do_AminusB(data1,data2,size);
      break;
    }      
    if (VWriteData((char *)data1,1,size,out,&j)) {
      printf("Could not write data\n");
      exit(-1);
    }      
  }
  
  
  fclose(in1);
  fclose(in2);
  VCloseData(out);

  exit(0);

}


void Do_Or(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 |= *d2;
}

void Do_And(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 &= *d2;
}


void Do_Xor(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 ^= *d2;
}

void Do_Nor(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 = ~(*d1|*d2) ;
}


void Do_Nand(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 = ~(*d1&*d2) ;
}


void Do_Xnor(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 = ~(*d1^*d2) ;
}


void Do_AminusB(unsigned char *d1, unsigned char *d2, int size)
{
  int i;

  for(i=0;i<size;d1++,d2++,i++) 
    *d1 &= ~(*d2) ;
}




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
