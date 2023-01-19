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

int get_slices(int dim, short *list);
int bin_to_grey(unsigned char *bin_buffer, int length,
    unsigned char *grey_buffer, int min_value, int max_value);

int main(int argc, char *argv[])
{

  double count=0;

  int slc,j, size, size1, error, slices1;
  ViewnixHeader vh1;
  FILE *in1, *outfp;
  unsigned char *data1, *d1_8;
  char group[6],elem[6];
  int bounds[6];

  if (argc != 4)
  {
    fprintf(stderr,
	  "Usage: BIM_to_rover <BIM_file> <rdf_file> <name>\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  if (in1==NULL) {
    fprintf(stderr, "Error in opening the file %s\n", argv[1]);
    exit(-1);
  }
  
  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    fprintf(stderr, "Fatal error in reading header\n");
    exit(-1);
  }

  outfp = fopen(argv[2], "wb");
  if (outfp == NULL)
  {
    fprintf(stderr, "Error in opening the file %s\n", argv[2]);
	exit(-1);
  }  
  
  if (vh1.gen.data_type!=IMAGE0) {
    fprintf(stderr, "Input file should be IMAGE0\n");
    exit(-1);
  }
  if (vh1.scn.num_of_bits != 1) {
    fprintf(stderr, "The Mask file should be binary\n");
    exit(-1);
  }

  slices1= get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);


  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];

  size1 = (size*vh1.scn.num_of_bits+7)/8;
  data1= (unsigned char *)malloc(size1);
  d1_8 = (unsigned char *)malloc(size);

  VSeekData(in1,0);

  bounds[0] = vh1.scn.xysize[0];
  bounds[2] = vh1.scn.xysize[1];
  bounds[4] = slices1;
  bounds[1] = bounds[3] = bounds[5] = 0;
  count=0;
  for (slc=0; slc<slices1; slc++)
  {
    int x, y, c;
    if (VReadData((char *)data1, 1, size1, in1, &j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    count += c=bin_to_grey(data1,size,d1_8,0,1);
    for (y=0; y<vh1.scn.xysize[1]; y++)
    {
      int rc=0;
      for (x=0; x<vh1.scn.xysize[0]; x++)
        if (d1_8[y*vh1.scn.xysize[0]+x])
        {
          rc++;
          if (x < bounds[0])
            bounds[0] = x;
          if (x > bounds[1])
            bounds[1] = x;
        }
      if (rc)
      {
        if (y < bounds[2])
          bounds[2] = y;
        if (y > bounds[3])
          bounds[3] = y;
      }
    }
    if (c)
    {
      if (slc < bounds[4])
        bounds[4] = slc;
      if (slc > bounds[5])
        bounds[5] = slc;
    }
  }
  VSeekData(in1, bounds[4]*size1);
  if (fprintf(outfp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")!=39 ||
      fprintf(outfp, "<rover version=\"rover 2.1.18\" fileVersion=\"1.3\">\n")
      !=49 || fprintf(outfp, " <study>\n  <volume>\n")!=20 ||
      fprintf(outfp, "   <mask type=\"Square\" name=\"%s\">\n", argv[3])<33 ||
      fprintf(outfp, "    <pos_center>\n")!=17 ||
      fprintf(outfp, "     <voxel x=\"%d\" y=\"%d\" z=\"%d\"/>\n",
        bounds[0]+1, bounds[2]+1, slices1-bounds[5])<32 ||
      fprintf(outfp, "    </pos_center>\n    <pos_x>\n")!=30 ||
      fprintf(outfp, "     <voxel x=\"%d\" y=\"%d\" z=\"%d\"/>\n",
        bounds[1]+1, bounds[2]+1, slices1-bounds[5])<32 ||
      fprintf(outfp, "    </pos_x>\n    <pos_y>\n")!=25 ||
      fprintf(outfp, "     <voxel x=\"%d\" y=\"%d\" z=\"%d\"/>\n",
        bounds[0]+1, bounds[3]+1, slices1-bounds[5])<32 ||
      fprintf(outfp, "    </pos_y>\n    <pos_z>\n")!=25 ||
      fprintf(outfp, "     <voxel x=\"%d\" y=\"%d\" z=\"%d\"/>\n",
        bounds[0]+1, bounds[2]+1, slices1-bounds[4])<32 ||
      fprintf(outfp, "    </pos_z>\n")!=13 ||
      fprintf(outfp, "    <roi voxel_count=\"%.0f\" name=\"%s.1\">\n", count,
        argv[3])<35)
  {
    fprintf(stderr, "Write failure\n");
    exit(-1);
  }
  for (slc=bounds[4]; slc<=bounds[5]; slc++)
  {
    int x, y;
    if (VReadData((char *)data1, 1, size1, in1, &j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    bin_to_grey(data1,size,d1_8,0,1);
    for (y=bounds[2]; y<=bounds[3]; y++)
      for (x=bounds[0]; x<=bounds[1]; x++)
        if (d1_8[y*vh1.scn.xysize[0]+x])
        {
		  if (fprintf(outfp, "     <roi_voxel value=\"1\">\n")!=27 ||
		      fprintf(outfp, "      <voxel x=\"%d\" y=\"%d\" z=\"%d\"/>\n",
			    x+1, y+1, slices1-slc)<33 ||
			  fprintf(outfp, "     </roi_voxel>\n")!=18)
		  {
		    fprintf(stderr, "Write failure\n");
			exit(-1);
		  }
        }
  }
  if (fprintf(outfp,
      "    </roi>\n   </mask>\n  </volume>\n </study>\n</rover>\n") !=53)
  {
    fprintf(stderr, "Write failure\n");
	exit(-1);
  }

  fclose(in1);
  fclose(outfp);
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

int bin_to_grey(unsigned char *bin_buffer, int length,
    unsigned char *grey_buffer, int min_value, int max_value)
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin, *grey;
  int c;

  bin = bin_buffer;
  grey = grey_buffer;
  
  c=0;
  for(j=7,i=length; i>0; i--)    {
    if( (*bin & mask[j]) != 0)  {
      *grey = max_value;
      c++;
    }
    else 
      *grey = min_value;
    
    grey++;
    if (j==0) {
      bin++; j=7;
    }
    else
      j--;
    
  }

  return c;

}

