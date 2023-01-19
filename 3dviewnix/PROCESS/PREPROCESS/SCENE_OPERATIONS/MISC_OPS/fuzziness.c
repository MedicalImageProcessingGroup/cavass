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
void bin_to_grey(unsigned char *bin_buffer, int length,
    unsigned char *grey_buffer, int min_value, int max_value),
 Stat16(unsigned short *in, int size), Stat8(unsigned char *in, int size);

double fuzziness1=0, fuzziness2=0, fuzziness3=0, support=0;


int main(argc,argv)
int argc;
char *argv[];
{

  int i, j, slices, size, size1, error, bytes;
  ViewnixHeader vh1;
  FILE *in1;
  unsigned char *data1, *d1_8;
  char group[6],elem[6];

  if (argc!= 2) {
    printf("Usage: fuzziness <IM0_file>\n");
    exit(-1);
  }

  in1=fopen(argv[1],"r");

  if (in1==NULL) {
    printf("Error in opening the files\n");
    exit(-1);
  }
  
  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  
  if (vh1.gen.data_type!=IMAGE0) {
    printf("Both input files should be IMAGE0\n");
    exit(-1);
  }

  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);

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


  VSeekData(in1,0);


  for(i=0;i<slices;i++) {
    printf("Processing slice %d\r",i);
    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      printf("Could not read data\n");
      exit(-1);
    }
    if (vh1.scn.num_of_bits==1) 
      bin_to_grey(data1,size,d1_8,0,1);

    if (vh1.scn.num_of_bits==16)
      Stat16((unsigned short *)d1_8,size);
    else
      Stat8(d1_8,size);
  }
  
  
  fclose(in1);

  printf("Index of fuzziness 1 = %f\n", fuzziness1/(support*log(2.0)));
  printf("Index of fuzziness 2 = %f\n", 2*fuzziness2/(support));
  printf("Index of fuzziness 3 = %f\n", 2*sqrt(fuzziness3/(support)));
  exit(0);
}



void Stat16(unsigned short *in, int size)
{
  int i;
  double x;

  for(i=0;i<size;in++,i++) {
    x = *in*(1./65534);
	if (x > 0)
	{
	  support++;
	  if (1-x > 0)
	  {
	    fuzziness1 += -x*log(x)-(1-x)*log(1-x);
	    fuzziness2 += x>.5? 1-x: x;
	    fuzziness3 += x>.5? (1-x)*(1-x): x*x;
	  }
	}
  }

}


void Stat8(unsigned char *in, int size)
{
  
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

