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
#ifndef WIN32
	#include <unistd.h>
#endif

#include "render/matrix.cpp"

void destroy_scene_header(ViewnixHeader *vh);

int main(argc,argv)
int argc;
char *argv[];
{
  int i, j, size, error, slices1;
  ViewnixHeader vh1, outvh;
  FILE *in1, *outfp;
  char group[6],elem[6];
  unsigned char *out_data;
  int out_slices;
  int slc, row, col;
  double bbox[6], max_voxel_size[3];
  float (*voxsz)[3];
  int ns; // number of samples

  if (argc < 3) {
    fprintf(stderr, "Usage: merge <input BIM_file> ... <output BIM_file>\n");
    exit(-1);
  }
  ns = argc-2;
  memset(max_voxel_size, 0, sizeof(max_voxel_size));
  memset(&outvh, 0, sizeof(outvh));
  strcpy(outvh.gen.recognition_code, "VIEWNIX1.0");
  outvh.gen.recognition_code_valid = 1;
  outvh.gen.data_type = IMAGE0;
  outvh.gen.data_type_valid = 1;
  outvh.scn.dimension = 3;
  outvh.scn.dimension_valid = 1;
  outvh.scn.num_of_density_values = 1;
  outvh.scn.num_of_density_values_valid = 1;
  outvh.scn.smallest_density_value = (float *)malloc(sizeof(float));
  outvh.scn.smallest_density_value[0] = 0;
  outvh.scn.smallest_density_value_valid = 1;
  outvh.scn.largest_density_value = (float *)malloc(sizeof(float));
  outvh.scn.largest_density_value[0] = 1;
  outvh.scn.largest_density_value_valid = 1;
  outvh.scn.num_of_integers = 1;
  outvh.scn.num_of_integers_valid = 1;
  outvh.scn.signed_bits = (short *)calloc(1, sizeof(short));
  outvh.scn.signed_bits_valid = 1;
  outvh.scn.num_of_bits = 1;
  outvh.scn.num_of_bits_valid = 1;
  outvh.scn.bit_fields = (short *)malloc(2*sizeof(short));
  outvh.scn.bit_fields[0] = 0;
  outvh.scn.bit_fields[1] = 0;
  outvh.scn.bit_fields_valid = 1;
  voxsz = (float (*)[3])malloc(ns*sizeof(float)*3);
  for (j=0; j<ns; j++)
  {
    FILE *tfp;
	int len1=strlen(argv[j+1]);
    char *cmd=(char *)malloc(len1+36);
	char line[80];
	float bbj[6];

	sprintf(cmd, "bin_volume %s -b > MRG-TMP-INFO", argv[j+1]);
	system(cmd);
	tfp = fopen("MRG-TMP-INFO", "rb");
	fgets(line, 80, tfp);
	fgets(line, 80, tfp);
	fgets(line, 80, tfp);
	if (fscanf(tfp,"Bounding Box            -> [%f, %f] x [%f, %f] x [%f, %f]",
	    bbj, bbj+1, bbj+2, bbj+3, bbj+4, bbj+5) != 6)
	{
	  fprintf(stderr, "Could not find bounding box for %s\n", argv[j+1]);
	  exit(-1);
	}
	fclose(tfp);
	unlink("MRG-TMP-INFO");
	if (j==0 || bbj[0]<bbox[0])
	  bbox[0] = bbj[0];
	if (j==0 || bbj[1]>bbox[1])
	  bbox[1] = bbj[1];
	if (j==0 || bbj[2]<bbox[2])
	  bbox[2] = bbj[2];
	if (j==0 || bbj[3]>bbox[3])
	  bbox[3] = bbj[3];
	if (j==0 || bbj[4]<bbox[4])
	  bbox[4] = bbj[4];
	if (j==0 || bbj[5]>bbox[5])
	  bbox[5] = bbj[5];
	sprintf(cmd, "get_slicenumber %s -s > SAMPLING-TEMP", argv[j+1]);
	system(cmd);
	tfp = fopen("SAMPLING-TEMP", "rb");
	if (fscanf(tfp, "%f %f %f", voxsz[j], voxsz[j]+1, voxsz[j]+2) != 3)
	{
		fprintf(stderr, "failure reading temorary file.\n");
		exit(-1);
	}
	fclose(tfp);
	unlink("SAMPLING-TEMP");
	free(cmd);
	if (j==0 || max_voxel_size[0]<voxsz[j][0])
	  max_voxel_size[0] = voxsz[j][0];
	if (j==0 || max_voxel_size[1]<voxsz[j][1])
	  max_voxel_size[1] = voxsz[j][1];
	if (j==0 || max_voxel_size[2]<voxsz[j][2])
	  max_voxel_size[2] = voxsz[j][2];
  }
  outvh.scn.xypixsz[0] = (float)max_voxel_size[0];
  outvh.scn.xypixsz[1] = (float)max_voxel_size[1];
  outvh.scn.xypixsz_valid = 1;
  outvh.scn.xysize[0] = (short)rint((bbox[1]-bbox[0])/max_voxel_size[0])+1;
  outvh.scn.xysize[1] = (short)rint((bbox[3]-bbox[2])/max_voxel_size[1])+1;
  outvh.scn.xysize_valid = 1;
  out_slices = (int)rint((bbox[5]-bbox[4])/max_voxel_size[2])+1;
  outvh.scn.num_of_subscenes = (short *)malloc(sizeof(short));
  outvh.scn.num_of_subscenes[0] = out_slices;
  outvh.scn.num_of_subscenes_valid = 1;
  outvh.scn.loc_of_subscenes = (float *)malloc(out_slices*sizeof(float));
  for (j=0; j<out_slices; j++)
    outvh.scn.loc_of_subscenes[j] = (float)(j*max_voxel_size[2]);
  outvh.scn.loc_of_subscenes_valid = 1;
  size = outvh.scn.xysize[0]*outvh.scn.xysize[1];
  out_data = (unsigned char *)malloc((size+7)/8*out_slices);
  if (out_data == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(1);
  }
  memset(out_data, 0, (size+7)/8*out_slices*sizeof(*out_data));
  memset(&vh1, 0, sizeof(vh1));
  outvh.scn.domain = (float *)calloc(12, sizeof(float));
  outvh.scn.domain[3] = 1;
  outvh.scn.domain[7] = 1;
  outvh.scn.domain[11] = 1;

  for (j=0; j<ns; j++)
  {
	unsigned char *bin_data;

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
    if (vh1.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	slices1 = vh1.scn.num_of_subscenes[0];
    bin_data = (unsigned char *)
	  malloc((vh1.scn.xysize[0]*vh1.scn.xysize[1]+7)/8*slices1);
    VSeekData(in1,0);
	if (VReadData((char *)bin_data, 1,
	    (vh1.scn.xysize[0]*vh1.scn.xysize[1]+7)/8*slices1, in1, &error))
	{
	  fprintf(stderr, "Could not read data\n");
	  exit(-1);
	}
	fclose(in1);
	if (j==0 && vh1.scn.domain_valid)
	{
	  for (i=0; i<3; i++)
	    outvh.scn.domain[i] = (float)(vh1.scn.domain[3+i]*bbox[0]+
	                                  vh1.scn.domain[6+i]*bbox[2]+
									  vh1.scn.domain[9+i]*bbox[4]);
	  memcpy(outvh.scn.domain+3, vh1.scn.domain+3, 9*sizeof(float));
	  outvh.scn.domain_valid = 1;
	}
	for (slc=0; slc<vh1.scn.num_of_subscenes[0]; slc++)
	{
	  for (row=0; row<vh1.scn.xysize[1]; row++)
		for (col=0; col<vh1.scn.xysize[0]; col++)
		  if (bin_data[(vh1.scn.xysize[0]*vh1.scn.xysize[1]+7)/8*slc+
		      (row*vh1.scn.xysize[0]+col)/8] & (128>>
			  (row*vh1.scn.xysize[0]+col)%8))
		  {
		    double x[3];
			int outx[3];
		    x[0] = col*voxsz[j][0];
		    x[1] = row*voxsz[j][1];
		    x[2] = vh1.scn.loc_of_subscenes[slc];
		    if (vh1.scn.domain_valid)
		     for (i=0; i<3; i++)
			  x[i] +=
			    vh1.scn.domain[3+3*i]*(vh1.scn.domain[0]-outvh.scn.domain[0])+
				vh1.scn.domain[4+3*i]*(vh1.scn.domain[1]-outvh.scn.domain[1])+
				vh1.scn.domain[5+3*i]*(vh1.scn.domain[2]-outvh.scn.domain[2]);
		    x[2] -= outvh.scn.loc_of_subscenes[0];
		    for (i=0; i<3; i++)
			  outx[i] = (int)rint(x[i]/max_voxel_size[i]);
		    assert(outx[0]>=0 && outx[0]<outvh.scn.xysize[0]);
			assert(outx[1]>=0 && outx[1]<outvh.scn.xysize[1]);
			assert(outx[2]>=0 && outx[2]<outvh.scn.num_of_subscenes[0]);
			out_data[(size+7)/8*outx[2]+
			  (outvh.scn.xysize[0]*outx[1]+outx[0])/8] |=
			  (128>>(outvh.scn.xysize[0]*outx[1]+outx[0])%8);
		  }
	}
	free(bin_data);
	destroy_scene_header(&vh1);
  }
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
  if (VWriteData((char *)out_data, 1, (size+7)/8*out_slices, outfp, &i))
  {
    fprintf(stderr, "Write error.\n");
	exit(-1);
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
