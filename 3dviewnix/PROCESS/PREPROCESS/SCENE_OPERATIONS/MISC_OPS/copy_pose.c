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

int get_slices(int dim, short *list);


int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,error,slices2,bytes;
  ViewnixHeader vh1,vh2,*outvh;
  FILE *in1,*in2,*out;
  unsigned char *data1;
  char group[6],elem[6];


  if (argc != 4) {
    fprintf(stderr, "Usage: copy_pose <skew_scene> <good_scene> <output>\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  in2=fopen(argv[2],"rb");
  out=fopen(argv[3],"wb+");

  if (in1==NULL || in2==NULL || out==NULL) {
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
  
  if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0) {
    fprintf(stderr, "Both input files should be IMAGE0\n");
    exit(-1);
  }
  if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] || vh1.scn.xysize[1]!=vh2.scn.xysize[1]) {
    fprintf(stderr, "Both Scenes should be of the same width and height\n");
    exit(-1);
  }

  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  slices2= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
  if (slices > slices2) { 
    fprintf(stderr,  "Skew scene has more slices\n");
    exit(-1);
  }
  if (vh1.scn.dimension != vh2.scn.dimension)
  {
    fprintf(stderr, "Both Scenes should be of the same dimension\n");
	exit(-1);
  }

  outvh= &vh1;

  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
  if (vh1.scn.num_of_bits==16)
    bytes=2;
  else
    bytes=1;

  size1= (size*vh1.scn.num_of_bits+7)/8;
  data1= (unsigned char *)malloc(size1);
  if (data1 == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(-1);
  }

  outvh->gen.filename_valid=0;
  outvh->scn.domain = vh2.scn.domain;
  outvh->scn.domain_valid = vh2.scn.domain_valid;
  outvh->scn.xypixsz[0] = vh2.scn.xypixsz[0];
  outvh->scn.xypixsz[1] = vh2.scn.xypixsz[1];
  outvh->scn.loc_of_subscenes = vh2.scn.loc_of_subscenes;
  
  error=VWriteHeader(out,outvh,group,elem);
  if (error<=104) {
    fprintf(stderr, "Fatal error in writing header\n");
    exit(-1);
  }
  
  if (VSeekData(in1,0) || VSeekData(out,0))
  {
    fprintf(stderr, "File seek error.\n");
	exit(-1);
  }

  for(i=0;i<slices;i++) {
    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    if (VWriteData((char *)data1,bytes,(int)(size1/bytes),out,&j)) {
      fprintf(stderr, "Could not write data\n");
      exit(-1);
    }      

  }
  
  
  fclose(in1);
  fclose(in2);
  VCloseData(out);

  exit(0);
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
