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

#include <stdio.h>
#include <math.h>
#include <cv3dv.h>

int get_slices(int dim, short *list);

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Computes a magnitude scene from a signed vector scene.
 * PARAMETERS:
 *    argc: The number of command-line parameters
 *    argv: The command-line parameters
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: Environment variable VIEWNIX_ENV must be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: 
 * HISTORY:
 *    Created: 12/10/09 by Dewey Odhner
 *
 *****************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{

  int sn,j,bytes,bytes1,slices,size,error, bg_flag=FALSE;
  double val1, val2, val3;
  float min1[3], max1[3];
  ViewnixHeader vh1;
  FILE *in1, *out;
  signed char *data1;
  unsigned char *outdata;
  char group[6],elem[6];

#define Handle_error(message) \
{ \
  fprintf(stderr, message); \
  exit(1); \
}

  if (strcmp(argv[argc-1], "-b") == 0) {
	bg_flag = TRUE;
	argc--;
  }
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <infile> <outfile> [-b]\n", argv[0]);
    exit(-1);
  }

  in1=fopen(argv[1],"r");
  if (in1==NULL )
    Handle_error("Error in opening the input file\n");

  out=fopen(argv[2],"w+");
  if ( out==NULL )
    Handle_error("Error in opening output file\n");

  error=VReadHeader(in1,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (vh1.gen.data_type!=IMAGE0)
    Handle_error("This is not an IMAGE0 file\n");

  if (vh1.scn.num_of_integers!=3 || vh1.scn.signed_bits[0]==0)
    Handle_error("Expecting 3 signed integer values per voxel.\n");

  bytes1 = vh1.scn.num_of_bits/8;
  slices = get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  size= (vh1.scn.xysize[0]*vh1.scn.xysize[1]);
  if (vh1.scn.smallest_density_value_valid)
    for (j=0; j<3; j++)
      min1[j] = vh1.scn.smallest_density_value[j];
  else
    min1[0] = min1[1] = min1[2] = (float)(bytes1>1? -32768: -128);
  if (vh1.scn.largest_density_value_valid)
    for (j=0; j<3; j++)
	  max1[j] = vh1.scn.largest_density_value[j];
  else
    max1[0] = max1[1] = max1[2] = (float)(bytes1>1? 32767: 127);

  vh1.scn.num_of_density_values = 1;
  vh1.scn.smallest_density_value[0] = 0;
  vh1.scn.largest_density_value[0] = 0;
  for (j=0; j<3; j++)
    vh1.scn.largest_density_value[0] +=
	  -min1[j]>max1[j]? min1[j]*min1[j]: max1[j]*max1[j];
  vh1.scn.largest_density_value[0]= (float)sqrt(vh1.scn.largest_density_value[0]);
  vh1.scn.num_of_integers = 1;
  vh1.scn.signed_bits[0] = 0;
  bytes = bytes1/3;
  vh1.scn.num_of_bits=bytes*8;
  strncpy(vh1.gen.filename, argv[2], sizeof(vh1.gen.filename));

  data1= (signed char *)malloc(size*bytes1);
  outdata = (unsigned char *)malloc(size*bytes);
  if (data1==NULL || outdata==NULL)
    Handle_error("Could not allocate data. Aborting magnitude.\n");

  error=VWriteHeader(out,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in writing header\n");

  VSeekData(in1,0);

  for(sn=0; sn<slices; sn++) {
    if (!bg_flag)
	{	printf("Computing slice %d\r", sn+1);
    	fflush(stdout);
	}

	error = VReadData((char *)data1, bytes, size*3, in1, &j);
	if (error)
      Handle_error("Could not read data\n");

	for (j=0; j<size; j++)
	{
		switch (bytes)
		{
		  case 1:
			val1 = data1[3*j];
			val2 = data1[3*j+1];
			val3 = data1[3*j+2];
			outdata[j] = (unsigned char)sqrt(val1*val1+val2*val2+val3*val3);
			break;
		  case 2:
			val1 = ((short *)data1)[j];
			val2 = ((short *)data1)[3*j+1];
			val3 = ((short *)data1)[3*j+2];
			((unsigned short *)outdata)[j] =
			  (unsigned short)sqrt(val1*val1+val2*val2+val3*val3);
			break;
		}
	}

    if (VWriteData((char *)outdata,bytes,size,out,&j))
      Handle_error("Could not write data\n");
  }

  fclose(in1);
  VCloseData(out);

  if (!bg_flag)
	printf("\ndone\n");
  exit(0);
}



/*****************************************************************************
 * FUNCTION: get_slices
 * DESCRIPTION: Returns total number of slices in a scene.
 * PARAMETERS:
 *    dim: The dimension of the scene.
 *    list: The list containing the number of subscenes in the file.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The total number of slices in the file.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: unknown
 *
 *****************************************************************************/
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
