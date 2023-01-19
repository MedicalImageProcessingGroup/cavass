/*
  Copyright 1993-2015 Medical Image Processing Group
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

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Implements intensity mapping.
 * PARAMETERS:
 *    argc: The number of command-line parameters
 *    argv: The command-line parameters
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: Environment variable VIEWNIX_ENV must be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: 
 * HISTORY:
 *    Created: 1/7/04 (modified from algebra) by Dewey Odhner
 *
 *****************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{

  int sn,j,bytes,bytes1,slices,size,error, bg_flag=FALSE, slic1,
	start, stop, step;
  float ramp_low, ramp_high, gauss_mean, gauss_sd;
  int val1, min1, max1;
  ViewnixHeader vh1;
  FILE *in1,*in2,*out;
  unsigned char *data1;
  char group[6],elem[6];
  static unsigned short table[65536];
  unsigned u1, u2;

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
  if ((argc!=5||argv[3][0]!='t') &&
      (argc!=6||(argv[3][0]!='r'&&argv[3][0]!='g'))) {
    fprintf(stderr,
"Usage: %s <infile> <outfile> [r <low> <high> | [g|gl|gu] <mean> <sd> | t <mapfile>] [-b]\n",
	  argv[0]);
    exit(-1);
  }
  if (bg_flag)
    VAddBackgroundProcessInformation(argv[0]);

  in1=fopen(argv[1],"r");
  if (in1==NULL )
    Handle_error("Error in opening the input file\n");
  if (argv[3][0] == 'g') /* gaussian */
  {
    if (sscanf(argv[4], "%f", &gauss_mean)!=1 ||
	    sscanf(argv[5], "%f", &gauss_sd)!=1)
	  Handle_error("Error reading parameters\n");
    ramp_low = 0;
	ramp_high = 65535;
	if (argv[3][1] == 'l') /* lower half gaussian */
	  ramp_high = (float)floor(gauss_mean);
	else if (argv[3][1] == 'u') /* upper half gaussian */
	  ramp_low = (float)ceil(gauss_mean);
	if (ramp_high > 65535)
	  ramp_high = 65535;
	if (ramp_low < 0)
	  ramp_low = 0;
	for (j=0; j<(int)ramp_low; j++)
	  table[j] = gauss_sd<0? 0:65534;
	for (; j<=(int)ramp_high; j++)
	{
	  table[j] = (unsigned short)rint(
	    65534*exp(-.5/(gauss_sd*gauss_sd)*((j-gauss_mean)*(j-gauss_mean))));
	  if (gauss_sd < 0)
	    table[j] = 65534-table[j];
	}
	for (; j<=65535; j++)
	  table[j] = gauss_sd<0? 0:65534;
  }
  else if (argv[3][0] == 'r') /* ramp */
  {
    if (sscanf(argv[4], "%f", &ramp_low)!=1 ||
	    sscanf(argv[5], "%f", &ramp_high)!=1)
	  Handle_error("Error reading parameters\n");
	for (j=0; j<65536; j++)
	  table[j] = (unsigned short)(
	    ramp_low>ramp_high? j<=ramp_high? 65534: j>=ramp_low? 0:
		  rint((ramp_low-j)*(65534/(ramp_low-ramp_high))):
		j<=ramp_low? 0: j>=ramp_high? 65534:
		  rint((j-ramp_low)*(65534/(ramp_high-ramp_low))));
  }
  else /* table */
  {
    in2=fopen(argv[4],"r");
    if (in2==NULL )
      Handle_error("Error in opening the map file\n");
	while (!feof(in2))
	{
	  if (fscanf(in2, "%u %u\n", &u1, &u2) != 2)
	  {
	    fprintf(stderr, "Error reading map file\n");
		break;
	  }
	  if (u1 < 65536)
	  {
	    table[u1] = u2<65536? u2: 65535;
	  }
	}
	fclose(in2);
  }

  out=fopen(argv[2],"w+");
  if ( out==NULL )
    Handle_error("Error in opening output file\n");

  error=VReadHeader(in1,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (vh1.gen.data_type!=IMAGE0)
    Handle_error("This is not an IMAGE0 file\n");

  bytes1 = vh1.scn.num_of_bits/8;
  slices = slic1 = get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  size= (vh1.scn.xysize[0]*vh1.scn.xysize[1]);
  if (vh1.scn.smallest_density_value_valid)
  min1 = vh1.scn.smallest_density_value_valid?
    (int)vh1.scn.smallest_density_value[0]: 0;
  max1 = vh1.scn.largest_density_value_valid?
    (int)vh1.scn.largest_density_value[0]: bytes1>1? 65535: 255;

  vh1.scn.smallest_density_value[0] = 65535;
  vh1.scn.largest_density_value[0] = 0;
  for (j=min1; j<=max1; j++)
  {
    if (table[j] < vh1.scn.smallest_density_value[0])
	  vh1.scn.smallest_density_value[0] = table[j];
	if (table[j] > vh1.scn.largest_density_value[0])
	  vh1.scn.largest_density_value[0] = table[j];
  }

  bytes = vh1.scn.largest_density_value[0]>=256? 2:1;
  vh1.scn.num_of_bits=bytes*8;
  vh1.scn.bit_fields[0] = 0;
  vh1.scn.bit_fields[1] = vh1.scn.num_of_bits-1;
  strncpy(vh1.gen.filename, argv[3], sizeof(vh1.gen.filename));

  data1= (unsigned char *)malloc(size*2);
  if (data1==NULL)
    Handle_error("Could not allocate data. Aborting fuzz_ops\n");

  error=VWriteHeader(out,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in writing header\n");

  VSeekData(in1,0);

  if (bytes1 > bytes)
  {
	start = 0;
	stop = size;
	step = 1;
  }
  else
  {
	start = size-1;
	stop = -1;
	step = -1;
  }
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

	for (j=start; j!=stop; j+=step)
	{
		switch (bytes1)
		{
		  case 0:
			val1 = (data1[j/8]&(128>>(j%8))) != 0;
			break;
		  case 1:
			val1 = data1[j];
			break;
		  case 2:
			val1 = ((unsigned short *)data1)[j];
			break;
		}
		val1 = table[val1];
		if (bytes == 2)
			((unsigned short *)data1)[j] = val1;
		else
			data1[j] = val1;
	}

    if (VWriteData((char *)data1,bytes,size,out,&j))
      Handle_error("Could not write data\n");
  }

  fclose(in1);
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
