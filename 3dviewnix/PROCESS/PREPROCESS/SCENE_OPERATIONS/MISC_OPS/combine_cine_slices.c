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
#include <assert.h>

void destroy_scene_header(ViewnixHeader *vh);


/*
 *  Combine 3-D cine slice files with the same time instances, having z-
 *  locations in the order listed on the command line.
 */
int main(argc,argv)
int argc;
char *argv[];
{
  int i, j, error;
  ViewnixHeader vh1, outvh;
  FILE *in1, *outfp;
  char group[6],elem[6];
  unsigned short *d1_16;
  int slc;
  int ns; // number of samples
  int *pixel_bytes;

  if (argc < 3) {
    fprintf(stderr,
	  "Usage: combine_cine_slices <input IM0_file> ... <output IM0_file>\n");
    exit(-1);
  }
  ns = argc-2;
  memset(&outvh, 0, sizeof(outvh));
  memset(&vh1, 0, sizeof(vh1));
  pixel_bytes = (int *)malloc(ns*sizeof(int));

  in1 = fopen(argv[1], "rb");
  if (in1==NULL) {
    fprintf(stderr, "Error in opening the files\n");
    exit(-1);
  }
  error = VReadHeader(in1, &vh1, group, elem);
  if (error && error<=105) {
    fprintf(stderr, "Fatal error in reading header\n");
    exit(-1);
  }
  fclose(in1);
  if (vh1.gen.data_type!=IMAGE0) {
    fprintf(stderr, "Input file should be IMAGE0\n");
    exit(-1);
  }
  if (vh1.scn.dimension != 3)
  {
    fprintf(stderr, "Input file should be 3-D.\n");
	exit(-1);
  }
  if (vh1.scn.num_of_density_values!=1 || vh1.scn.num_of_integers!=1 ||
      vh1.scn.signed_bits[0] || vh1.scn.bytes_in_alignment>2 ||
	  (vh1.scn.num_of_bits!=8 && vh1.scn.num_of_bits!=16))
  {
    fprintf(stderr, "Input file has unhandled storage scheme.\n");
	exit(-1);
  }

  memcpy(&outvh.gen, &vh1.gen, sizeof(outvh.gen));
  strncpy(outvh.gen.filename, argv[argc-1], sizeof(outvh.gen.filename));
  outvh.gen.filename_valid = 1;
  strncpy(outvh.gen.filename1, argv[1], sizeof(outvh.gen.filename1));
  outvh.gen.filename1_valid = 1;
  vh1.gen.description = NULL;
  vh1.gen.comment = NULL;
  vh1.gen.imaged_nucleus = NULL;
  vh1.gen.gray_lookup_data = NULL;
  vh1.gen.red_lookup_data = NULL;
  vh1.gen.green_lookup_data = NULL;
  vh1.gen.blue_lookup_data = NULL;

  outvh.scn.dimension = 4;
  outvh.scn.dimension_valid = 1;
  outvh.scn.measurement_unit = (short *)malloc(4*sizeof(short));
  outvh.scn.measurement_unit[0] = vh1.scn.measurement_unit[0];
  outvh.scn.measurement_unit[1] = vh1.scn.measurement_unit[1];
  outvh.scn.measurement_unit[2] = vh1.scn.measurement_unit[1];
  outvh.scn.measurement_unit[3] = vh1.scn.measurement_unit[2];
  outvh.scn.measurement_unit_valid = vh1.scn.measurement_unit_valid;
  outvh.scn.num_of_density_values = 1;
  outvh.scn.num_of_density_values_valid = 1;
  outvh.scn.density_measurement_unit = vh1.scn.density_measurement_unit;
  outvh.scn.density_measurement_unit_valid =
    vh1.scn.density_measurement_unit_valid;
  vh1.scn.density_measurement_unit = NULL;
  outvh.scn.smallest_density_value = vh1.scn.smallest_density_value;
  outvh.scn.smallest_density_value_valid =
    vh1.scn.smallest_density_value_valid;
  vh1.scn.smallest_density_value = NULL;
  outvh.scn.largest_density_value = vh1.scn.largest_density_value;
  outvh.scn.largest_density_value_valid =
    vh1.scn.largest_density_value_valid;
  vh1.scn.largest_density_value = NULL;
  outvh.scn.num_of_integers = 1;
  outvh.scn.num_of_integers_valid = 1;
  outvh.scn.signed_bits = vh1.scn.signed_bits;
  outvh.scn.signed_bits_valid = vh1.scn.signed_bits_valid;
  vh1.scn.signed_bits = NULL;
  pixel_bytes[0] = vh1.scn.num_of_bits/8;
  outvh.scn.num_of_bits = 16;
  outvh.scn.num_of_bits_valid = 1;
  outvh.scn.bit_fields = vh1.scn.bit_fields;
  vh1.scn.bit_fields = NULL;
  outvh.scn.bit_fields[0] = 0;
  outvh.scn.bit_fields[1] = 15;
  outvh.scn.bit_fields_valid = 1;
  outvh.scn.xysize[0] = vh1.scn.xysize[0];
  outvh.scn.xysize[1] = vh1.scn.xysize[1];
  outvh.scn.xysize_valid = vh1.scn.xysize_valid;
  outvh.scn.num_of_subscenes =
    (short *)malloc((vh1.scn.num_of_subscenes[0]+1)*sizeof(short));
  outvh.scn.num_of_subscenes[0] = vh1.scn.num_of_subscenes[0];
  for (j=1; j<=vh1.scn.num_of_subscenes[0]; j++)
    outvh.scn.num_of_subscenes[j] = ns;
  outvh.scn.num_of_subscenes_valid = 1;
  outvh.scn.xypixsz[0] = vh1.scn.xypixsz[0];
  outvh.scn.xypixsz[1] = vh1.scn.xypixsz[1];
  outvh.scn.xypixsz_valid = vh1.scn.xypixsz_valid;
  outvh.scn.loc_of_subscenes =
    (float *)malloc((ns+1)*(vh1.scn.num_of_subscenes[0])*sizeof(float));
  for (j=0; j<outvh.scn.num_of_subscenes[0]; j++)
  {
    outvh.scn.loc_of_subscenes[j] = vh1.scn.loc_of_subscenes[j];
	outvh.scn.loc_of_subscenes[outvh.scn.num_of_subscenes[0]+j*ns] =
	  vh1.scn.domain_valid? vh1.scn.domain[2]: 0;
  }
  outvh.scn.description = vh1.scn.description;
  outvh.scn.description_valid = vh1.scn.description_valid;
  vh1.scn.description = NULL;
  d1_16 = (unsigned short *)malloc(vh1.scn.xysize[0]*vh1.scn.xysize[1]*2);
  if (d1_16 == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(-1);
  }
  destroy_scene_header(&vh1);
  for (j=1; j<ns; j++)
  {
    in1 = fopen(argv[j+1], "rb");
    if (in1==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
    error=VReadHeader(in1,&vh1,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
	fclose(in1);
    if (vh1.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	if (vh1.scn.dimension != 3)
	{
	  fprintf(stderr, "Input file should be 3-D.\n");
	  exit(-1);
	}
    if (vh1.scn.num_of_density_values!=1 || vh1.scn.num_of_integers!=1 ||
        vh1.scn.signed_bits[0] || vh1.scn.bytes_in_alignment>2 ||
	    (vh1.scn.num_of_bits!=8 && vh1.scn.num_of_bits!=16))
    {
      fprintf(stderr, "Input file has unhandled storage scheme.\n");
	  exit(-1);
    }
	if (vh1.scn.xysize[0]!=outvh.scn.xysize[0] ||
	    vh1.scn.xysize[1]!=outvh.scn.xysize[1] ||
		vh1.scn.num_of_subscenes[0]!=outvh.scn.num_of_subscenes[0])
	{
	  fprintf(stderr, "Input files do not match.\n");
	  exit(-1);
	}
	pixel_bytes[j] = vh1.scn.num_of_bits/8;
	if (vh1.scn.smallest_density_value_valid &&
	    outvh.scn.smallest_density_value_valid &&
		vh1.scn.smallest_density_value[0]< outvh.scn.smallest_density_value[0])
	  outvh.scn.smallest_density_value[0] = vh1.scn.smallest_density_value[0];
	if (vh1.scn.largest_density_value_valid &&
	    outvh.scn.largest_density_value_valid &&
		vh1.scn.largest_density_value[0] > outvh.scn.largest_density_value[0])
	  outvh.scn.largest_density_value[0] = vh1.scn.largest_density_value[0];
	for (slc=0; slc<outvh.scn.num_of_subscenes[0]; slc++)
	  outvh.scn.loc_of_subscenes[outvh.scn.num_of_subscenes[0]+slc*ns+j] =
	    vh1.scn.domain_valid? vh1.scn.domain[2]: j;
    destroy_scene_header(&vh1);
  }
  outvh.scn.loc_of_subscenes_valid = 1;
  outfp = fopen(argv[argc-1], "wb+");
  if (outfp == NULL)
  {
    fprintf(stderr, "Could not create file %s\n", argv[argc-1]);
	exit(-1);
  }
  error = VWriteHeader(outfp, &outvh, group,elem);
  if (error && error<106)
  {
    fprintf(stderr, "Error %d writing header, group %s element %s\n",
	  error, group,elem);
	exit(error);
  }
  for (slc=0; slc<outvh.scn.num_of_subscenes[0]; slc++)
    for (j=0; j<ns; j++)
    {
      in1 = fopen(argv[j+1], "rb");
      if (in1==NULL) {
        fprintf(stderr, "Error in opening the files\n");
        exit(-1);
      }
	  error = VSeekData(in1, outvh.scn.xysize[0]*outvh.scn.xysize[1]*
	    (pixel_bytes[j])*slc);
	  if (error || VReadData((char *)d1_16, pixel_bytes[j],
	      outvh.scn.xysize[0]*outvh.scn.xysize[1], in1, &i))
	  {
	    fprintf(stderr, "Error reading data.\n");
		exit(-1);
	  }
	  if (pixel_bytes[j] == 1)
	    for (i=outvh.scn.xysize[0]*outvh.scn.xysize[1]-1; i>=0; i--)
		  d1_16[i] = ((unsigned char *)d1_16)[i];
	  if (VWriteData((char *)d1_16,2,outvh.scn.xysize[0]*outvh.scn.xysize[1],outfp,&i))
	  {
	    fprintf(stderr, "Error writing data.\n");
		exit(-1);
	  }
	  fclose(in1);
    }
  VCloseData(outfp);
  exit(0);
}




/*****************************************************************************
 * FUNCTION: destroy_scene_header
 * DESCRIPTION: Frees the memory referenced by the pointer fields of a scene
 *    file header.
 * PARAMETERS:
 *    vh: The scene file header.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The pointer fields of vh must point to allocated memory or
 *    be NULL.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/11/93 by Dewey Odhner
 *
 *****************************************************************************/
void destroy_scene_header(ViewnixHeader *vh)
{
	if (vh->gen.description)
		free(vh->gen.description);
	if (vh->gen.comment)
		free(vh->gen.comment);
	if (vh->gen.imaged_nucleus)
		free(vh->gen.imaged_nucleus);
	if (vh->gen.gray_lookup_data)
		free(vh->gen.gray_lookup_data);
	if (vh->gen.red_lookup_data)
		free(vh->gen.red_lookup_data);
	if (vh->gen.green_lookup_data)
		free(vh->gen.green_lookup_data);
	if (vh->gen.blue_lookup_data)
		free(vh->gen.blue_lookup_data);
	if (vh->scn.domain)
		free(vh->scn.domain);
	if (vh->scn.axis_label)
		free(vh->scn.axis_label);
	if (vh->scn.measurement_unit)
		free(vh->scn.measurement_unit);
	if (vh->scn.density_measurement_unit)
		free(vh->scn.density_measurement_unit);
	if (vh->scn.smallest_density_value)
		free(vh->scn.smallest_density_value);
	if (vh->scn.largest_density_value)
		free(vh->scn.largest_density_value);
	if (vh->scn.signed_bits)
		free(vh->scn.signed_bits);
	if (vh->scn.bit_fields)
		free(vh->scn.bit_fields);
	if (vh->scn.num_of_subscenes)
		free(vh->scn.num_of_subscenes);
	if (vh->scn.loc_of_subscenes)
		free(vh->scn.loc_of_subscenes);
	if (vh->scn.description)
		free(vh->scn.description);
	memset(vh, 0, sizeof(*vh));
}
