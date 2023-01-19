/*
  Copyright 1993-2011, 2016, 2017 Medical Image Processing Group
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

/****************************************************************
   Filename: get_slicenumber.c                                                  
   Function: Get the slice number of the file
   Input:  An IM0 or BIM image filename
   Output: number of the slices in the file
   Date: 04/05/99

*****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cv3dv.h>


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

int main(argc,argv)
int argc;
char *argv[];
{
  int error;
  ViewnixHeader vh1;
  int slices;
  float z_size,x_size,y_size;
  int width;
  int height;
  FILE *in1, *out;
  char group[6],elem[6];
  /**********************************************************************/
  /*  Input parameters   */
  if (argc>3 && strcmp(argv[argc-2],"-o")==0 && (out=fopen(argv[argc-1],"wb")))
    argc -= 2;
  else
    out = stdout;
  if (argc!=2 && (argc!=3 || strcmp(argv[2], "-s"))) {
    fprintf(stderr, "\nUsage: get_slices <input> [-s] [-o <output>]\n");
    return (-1);
    }
  in1=fopen(argv[1],"rb");
  if (in1==NULL) {
    fprintf(stderr, "Error in opening the files\n");
    return (-1);
    }

  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    fprintf(stderr, "Fatal error in reading header\n");
    return (-1);
    }
  if (vh1.gen.data_type!=IMAGE0) {
    fprintf(stderr, "This is not an IMAGE0 file\n");
    return (-1); 
    }
  x_size = vh1.scn.xypixsz[0];
  y_size = vh1.scn.xypixsz[1];
  z_size = vh1.scn.loc_of_subscenes[1]-vh1.scn.loc_of_subscenes[0];
  width = vh1.scn.xysize[0];
  height = vh1.scn.xysize[1];
  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  if (argc == 3)
  {
    fprintf(out, "%f %f %f\n", x_size, y_size, z_size);
	fprintf(out, "%f %f %f\n",
	  vh1.scn.domain[0]+vh1.scn.loc_of_subscenes[0]*vh1.scn.domain[9],
	  vh1.scn.domain[1]+vh1.scn.loc_of_subscenes[0]*vh1.scn.domain[10],
	  vh1.scn.domain[2]+vh1.scn.loc_of_subscenes[0]*vh1.scn.domain[11]);
	fprintf(out, "%d %d %d\n", width, height, slices);
  }
  else
    fprintf(out, "0 %d\n",slices-1);
  exit(0);
}
