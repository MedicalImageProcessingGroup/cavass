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

void Do_Invert(unsigned char *d1, int size);
int get_slices(int dim, short *list);

/* Modified: 3/25/98 exit(0) called by Dewey Odhner. */
int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,error;
  ViewnixHeader vh1,*outvh;
  FILE *in1,*out;
  unsigned char *data1;
  char group[6],elem[6];

  if (argc!= 3) {
    printf("Usage: invert <inputfile1> <output_file>\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  out=fopen(argv[2],"wb+");

  if (in1==NULL || out==NULL) {
    printf("Error in opening the files\n");
    exit(-1);
  }
  
  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  

  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  outvh= &vh1;
  
  size= (outvh->scn.xysize[0]* outvh->scn.xysize[1]+7)/8;
  data1= (unsigned char *)malloc(size);
  
  strcpy(outvh->gen.filename,argv[2]);
  outvh->gen.filename_valid=1;
  
  error=VWriteHeader(out,outvh,group,elem);
  if (error<=104) {
    printf("Fatal error in writing header\n");
    exit(-1);
  }
  
  VSeekData(in1,0);
  VSeekData(out,0);


  for(i=0;i<slices;i++) {
    printf("Processing slice %d\r",i);
    if (VReadData((char *)data1,1,size,in1,&j)) {
      printf("Could not read data\n");
      exit(-1);
    }
    Do_Invert(data1,size);
    if (VWriteData((char *)data1,1,size,out,&j)) {
      printf("Could not write data\n");
      exit(-1);
    }      
  }
  
  
  fclose(in1);
  VCloseData(out);
  exit(0);
}


void Do_Invert(unsigned char *d1, int size)
{
  int i;

  for(i=0;i<size;d1++,i++) 
    *d1 = ~(*d1);
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
