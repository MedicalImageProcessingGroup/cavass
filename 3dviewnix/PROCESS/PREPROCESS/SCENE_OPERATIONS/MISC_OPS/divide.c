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
 * DESCRIPTION: Implements image division.
 * PARAMETERS:
 *    argc: The number of command-line parameters
 *    argv: The command-line parameters
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: 
 * HISTORY:
 *    Created: 5/30/97 by Dewey Odhner
 *    Modified: 6/17/97 background option added by Dewey Odhner
 *    Modified: 7/2/97 bit fields set by Dewey Odhner
 *    Modified: 12/16/97 memory allocation corrected by Dewey Odhner
 *    Modified: 8/28/02 unequal number of slices handled by Dewey Odhner
 *
 *****************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{

  int sn,j,bytes,bytes1,bytes2,slices,size,error, threshold, bg_flag=FALSE,
    slic1, slic2;
  float max1, max2;
  float val1,val2;
  ViewnixHeader vh1,vh2;
  FILE *in1,*in2,*out;
  unsigned char *data1,*data2;
  char group[6],elem[6];

#define Handle_error(message) \
{ \
  char msg[200]; \
  fprintf(stderr, message); \
  if (bg_flag) \
	{ \
		sprintf(msg, "job_done %s -abort &", argv[0]); \
		system(msg); \
	    VDeleteBackgroundProcessInformation(); \
	} \
  exit(1); \
}

  if (strcmp(argv[argc-1], "-b") == 0) {
	bg_flag = TRUE;
	argc--;
  }
  if (argc != 5) {
    printf("Usage: %s inputfile1 inputfile2 output_file threshold [-b]\n",
		argv[0]);
    exit(-1);
  }
  if (bg_flag)
    VAddBackgroundProcessInformation(argv[0]);

  in1=fopen(argv[1],"rb");
  if (in1==NULL )
    Handle_error("Error in opening the input file\n");
  in2=fopen(argv[2],"rb");
  if (in2==NULL )
    Handle_error("Error in opening the input file\n");

  out=fopen(argv[3],"wb+");
  if ( out==NULL )
    Handle_error("Error in opening output file\n");

  error=VReadHeader(in1,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (vh1.gen.data_type!=IMAGE0)
    Handle_error("This is not an IMAGE0 file\n");

  if (sscanf(argv[4], "%d", &threshold)!=1 || threshold<=0)
    Handle_error("Bad threshold.\n");

  bytes1 = vh1.scn.num_of_bits/8;
  slices = slic1 = get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  size= (vh1.scn.xysize[0]*vh1.scn.xysize[1]);

  error=VReadHeader(in2,&vh2,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (vh2.gen.data_type!=IMAGE0 ||
      vh2.scn.xysize[0]!=vh1.scn.xysize[0] || 
      vh2.scn.xysize[1]!=vh1.scn.xysize[1])
    Handle_error("Second input file is incompatible with 1st input file");
  if ((slic2=get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes)) > slices)
    slices = slic2;

  bytes2 = vh2.scn.num_of_bits/8;
  max1 = vh1.scn.largest_density_value[0];
  max2 = vh2.scn.largest_density_value[0];

  vh1.scn.smallest_density_value[0] = 0;
  vh1.scn.largest_density_value[0] = 65535;

  bytes = 2;
  vh1.scn.num_of_bits=bytes*8;
  vh1.scn.bit_fields[0] = 0;
  vh1.scn.bit_fields[1] = vh1.scn.num_of_bits-1;
  strncpy(vh1.gen.filename, argv[3], sizeof(vh1.gen.filename));
  if (slic2 > slic1)
  {
    vh1.scn.dimension = vh2.scn.dimension;
	free(vh1.scn.num_of_subscenes);
	free(vh1.scn.loc_of_subscenes);
	vh1.scn.num_of_subscenes = vh2.scn.num_of_subscenes;
	vh1.scn.loc_of_subscenes = vh2.scn.loc_of_subscenes;
  }

  data1= (unsigned char *)malloc(size*2);
  if (data1==NULL)
    Handle_error("Could not allocate data. Aborting fuzz_ops\n");

  data2= (unsigned char *)malloc((size*vh2.scn.num_of_bits+7)/8);
  if (data2==NULL)
    Handle_error("Could not allocate output data. Aborting fuzz_ops\n");

  error=VWriteHeader(out,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in writing header\n");

  VSeekData(in1,0);
  VSeekData(in2,0);

  for(sn=0; sn<slices; sn++) {
    if (!bg_flag)
	{	printf("Computing slice %d\r", sn+1);
    	fflush(stdout);
	}

    if (sn >= slic1)
		memset(data1, 0, bytes1? bytes1*size: (size+7)/8);
	else if (bytes1)
		error = VReadData((char *)data1,bytes1,size,in1,&j);
	else
		error = VReadData((char *)data1, 1, (size+7)/8, in1, &j);
	if (error)
      Handle_error("Could not read data\n");
    if (sn >= slic2)
		memset(data2, 0, bytes2? bytes2*size: (size+7)/8);
	else if (bytes2)
		error = VReadData((char *)data2,bytes2,size,in2,&j);
	else
		error = VReadData((char *)data2, 1, (size+7)/8, in2, &j);
	if (error)
      Handle_error("Could not read data\n");

	for (j=size-1; j>=0; j--)
	{
		switch (bytes1)
		{
		  case 0:
			val1 = (float)((data1[j/8]&(128>>(j%8))) != 0);
			break;
		  case 1:
			val1 = data1[j];
			break;
		  case 2:
			val1 = ((unsigned short *)data1)[j];
			break;
		}
		switch (bytes2)
		{
		  case 0:
			val2 = (float)((data2[j/8]&(128>>(j%8))) != 0);
			break;
		  case 1:
			val2 = data2[j];
			break;
		  case 2:
			val2 = ((unsigned short *)data2)[j];
			break;
		}
		((unsigned short *)data1)[j] = (unsigned short)(
			val1<threshold || val2<threshold
			?	0
			:	threshold/max1*65535*val1/val2+.5);
	}
    
    if (VWriteData((char *)data1,bytes,size,out,&j))
      Handle_error("Could not write data\n");
  }

  fclose(in1);
  fclose(in2);
  VCloseData(out);

  if (bg_flag)
  {	char cmd[256];

	sprintf(cmd, "job_done %s &", argv[0]);
	system(cmd);
    VDeleteBackgroundProcessInformation();
  }
  else
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
