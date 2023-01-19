/*
  Copyright 1993-2014, 2018 Medical Image Processing Group
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
typedef double VECTOR[3];


double VecMag(VECTOR u);
double DotProd(VECTOR u, VECTOR v);
int get_slices(int dim, short *list);
void CrossProd(VECTOR out, VECTOR u, VECTOR v),
  UpdateMinMax8(int *min, int *max, unsigned char *data, int size),
  UpdateMinMax16(int *min, int *max, unsigned short *data, int size,
    int signed_bits);

/*    Modified: 9/6/95 vh.scn.smallest_density_value_valid and
 *       vh.scn.largest_density_value_valid set by Dewey Odhner */
/*    Modified: 2/9/96 exit code 0 passed by Dewey Odhner */
/*    Modified: 8/4/00 test for negative value by Dewey Odhner */
/*    Modified: 2/20/02 negative values set to zero by Dewey Odhner */
int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,bytes,slices,size,min,max,error;
  ViewnixHeader vh;
  FILE *in,*out;
  unsigned char *data;
  char group[6],elem[6];
  int correct_flag, make_4D=0;
  double val;
  VECTOR u,v,w,temp;

  if (argc>3 && strcmp(argv[argc-1], "-make_4D")==0)
  {
    make_4D = 1;
	argc--;
  }

  if (argc!=3 && (argc!=4 || strcmp(argv[3], "-n"))) {
    printf("Usage: correct_min_max <inputfile> <output_file> [-n] [-make_4D]\n");
    exit(-1);
  }

  in=fopen(argv[1],"rb");
  out=fopen(argv[2],"w+b");
  if (in==NULL || out==NULL) {
    printf("Error in opening the files\n");
    exit(-1);
  }

  error=VReadHeader(in,&vh,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }

  if (vh.gen.data_type!=IMAGE0) {
    printf("This is not an IMAGE0 file\n");
    exit(-1);
  }

  if (vh.scn.num_of_bits!=8 && vh.scn.num_of_bits!=16) {
    printf("Cannont handle %d bit data\n",vh.scn.num_of_bits);
    exit(-1);
  }
  if (vh.scn.num_of_bits==8) 
    bytes=1;
  else
    bytes=2;


  slices=get_slices(vh.scn.dimension,vh.scn.num_of_subscenes);

  size= (vh.scn.xysize[0]*vh.scn.xysize[1]);
  data= (unsigned char *)malloc(size*bytes);

  if (vh.scn.dimension!=3 && make_4D)
  {
    fprintf(stderr, "Scene is not 3 dimentional.\n");
	exit(1);
  }

  if (vh.scn.domain_valid) {
    printf("Old Domain :\n");
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[0],vh.scn.domain[1],vh.scn.domain[2]);
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[3],vh.scn.domain[4],vh.scn.domain[5]);
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[6],vh.scn.domain[7],vh.scn.domain[8]);
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[9],vh.scn.domain[10],vh.scn.domain[11]);
    

    u[0]=vh.scn.domain[3];
    u[1]=vh.scn.domain[4];
    u[2]=vh.scn.domain[5];

    v[0]=vh.scn.domain[6];
    v[1]=vh.scn.domain[7];
    v[2]=vh.scn.domain[8];

    w[0]=vh.scn.domain[9];
    w[1]=vh.scn.domain[10];
    w[2]=vh.scn.domain[11];

    correct_flag=0;
    if ( fabs(VecMag(u)-1.0) > 0.0001 ) {
      printf("Vector in the X direction is not a unit\n");
      correct_flag=1;
    }
    if ( fabs(VecMag(v)-1.0) > 0.0001 ) {
      printf("Vector in the Y direction is not a unit\n");
      correct_flag=1;
    }
    if ( fabs(VecMag(w)-1.0) > 0.0001 ) {
      printf("Vector in the Z direction is not a unit\n");
      correct_flag=1;
    }
    
    CrossProd(temp,u,v);
    val=DotProd(temp,w);
    if ( fabs(val-1.0) > 0.0001 ) {
      printf("The domain vectors are not orthogonal\n");
      correct_flag=1;
    }
    
    if (correct_flag==1) {
      vh.scn.domain[3]=vh.scn.domain[7]=vh.scn.domain[11]=1.0;
      vh.scn.domain[4]=vh.scn.domain[5]=vh.scn.domain[6] =0.0;
      vh.scn.domain[8]=vh.scn.domain[9]=vh.scn.domain[10]=0.0;
    }

    printf("New Domain :\n");
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[0],vh.scn.domain[1],vh.scn.domain[2]);
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[3],vh.scn.domain[4],vh.scn.domain[5]);
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[6],vh.scn.domain[7],vh.scn.domain[8]);
    printf("\t(%lf,%lf,%lf)\n",vh.scn.domain[9],vh.scn.domain[10],vh.scn.domain[11]);

	if (make_4D)
	{
	  vh.scn.domain = (float *)realloc(vh.scn.domain, 12*slices*sizeof(float));
	  for (i=1; i<slices; i++)
	    memcpy(vh.scn.domain+12*i, vh.scn.domain, 12*sizeof(float));
	}

  }



  if (vh.scn.smallest_density_value_valid &&vh.scn.largest_density_value_valid)
    printf("Current min=%d max=%d\n",(int)vh.scn.smallest_density_value[0],
	  (int)vh.scn.largest_density_value[0]);

  if (make_4D)
  {
    vh.scn.num_of_subscenes =
	  (short *)realloc(vh.scn.num_of_subscenes, (slices+1)*sizeof(short));
	for (i=1; i<=slices; i++)
	  vh.scn.num_of_subscenes[i] = 1;
	vh.scn.loc_of_subscenes =
	  (float *)realloc(vh.scn.loc_of_subscenes, 2*slices*sizeof(float));
	for (i=0; i<slices; i++)
	{
	  vh.scn.loc_of_subscenes[slices+i] = vh.scn.loc_of_subscenes[i];
	  vh.scn.loc_of_subscenes[i] = (float)(i+1);
	}
	vh.scn.measurement_unit_valid = 0;
	vh.scn.dimension = 4;
  }  

  VSeekData(in,0);
  switch (bytes) {
  case 1:
    min=255; max=0;
    for(i=0;i<slices;i++) {
      printf("Computing min max for slice %d\r",i);
      if (VReadData((char *)data,bytes,size,in,&j)) {
	printf("Could not read data\n");
	exit(-1);
      }
      UpdateMinMax8(&min,&max,data,size);
    }
    break;
  case 2:
    min=65535; max=0;
    for(i=0;i<slices;i++) {
      printf("Computing min max for slice %d\r",i);
      if (VReadData((char *)data,bytes,size,in,&j)) {
	printf("Could not read data\n");
	exit(-1);
      }
      UpdateMinMax16(&min,&max, (unsigned short *)data, size,
	    vh.scn.signed_bits_valid && vh.scn.signed_bits[0]);
    }
    break;
  }
    
  vh.scn.smallest_density_value[0]=(float)min;
  vh.scn.largest_density_value[0] =(float)max;
  if (min >= 0)
	vh.scn.signed_bits[0] = 0;
  else if (argc == 3)
  {
    printf("\nNegative values being set to zero.");
	vh.scn.smallest_density_value[0] = 0;
	vh.scn.signed_bits[0] = 0;
  }
  vh.scn.smallest_density_value_valid = vh.scn.largest_density_value_valid = 1;
  printf("\nNew min=%d max=%d\n",min,max);
  
  error=VWriteHeader(out,&vh,group,elem);
  if (error<=104) {
    printf("Fatal error in writing header\n");
    exit(-1);
  }

  VSeekData(in,0);
  VSeekData(out,0);
  
  for(i=0;i<slices;i++) {
    printf("Writing slice %d\r",i);
    if (VReadData((char *)data,bytes,size,in, &j)) {
      printf("Could not read data\n");
      exit(-1);
    }
	if (min<0 && argc==3)
	  for (j=0; j<size; j++)
		if (((short *)data)[j] < 0)
		  ((short *)data)[j] = 0;
    if (VWriteData((char *)data,bytes,size,out, &j)) {
      printf("Could not read data\n");
      exit(-1);
    }
  }

  printf("\n");
  fclose(in);
  VCloseData(out);
  exit(0);
}


void UpdateMinMax8(int *min, int *max, unsigned char *data, int size)
{
  int i;
  
  for(i=0;i<size;data++,i++) {
    if (*data < *min) *min=*data;
    if (*data > *max) *max=*data;
  }

}

/* Modified: 2/20/02 signed data handled by Dewey Odhner. */
void UpdateMinMax16(int *min, int *max, unsigned short *data, int size,
    int signed_bits)
{
  int i;
  short *sdata;
  
  sdata = (short *)data;
  if (signed_bits)
    for(i=0;i<size;sdata++,i++) {
      if (*sdata < *min) *min=*sdata;
      if (*sdata > *max) *max=*sdata;
    }
  else
    for(i=0;i<size;data++,i++) {
      if (*data < *min) *min=*data;
      if (*data > *max) *max=*data;
    }

}
      
double VecMag(VECTOR u)
{
  return  sqrt((double)(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]));


}


double DotProd(VECTOR u, VECTOR v)
{
  return ( u[0]*v[0] + u[1]*v[1] + u[2]*v[2] );

}

void CrossProd(VECTOR out, VECTOR u, VECTOR v)
{
  out[0] = u[1]*v[2] - u[2]*v[1];
  out[1] = u[2]*v[0] - u[0]*v[2];
  out[2] = u[0]*v[1] - u[1]*v[0];

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
