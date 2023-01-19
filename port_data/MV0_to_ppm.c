/*
  Copyright 1993-2004, 2015, 2017, 2021 Medical Image Processing Group
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
#include <cv3dv.h>

void BuildColorLookup(unsigned char ct[256][3], ViewnixHeader *vh);


ViewnixHeader vh;
FILE *fp,*outfp;
int error;

int main(argc,argv)
int argc;
char *argv[];
{
  
  int i,incr,st_frame,end_frame,fr,in_size,out_size,width,height,bytes;
  unsigned char *in_data,*out_data,*in,*out;
  char outfile[150],grp[6],elem[6];
  static unsigned char ctable[256][3];

  if (argc != 6) {
    printf("Usage:%s <MV0_file> 1st_frame last_frame  <ppm_without_extension> bg_flag\n",argv[0]);
    exit(-1);
  }

  fp=fopen(argv[1], "rb");
  if (fp==NULL) {
    printf("Could not open input file\n");
    exit(-1);
  }

  error=VReadHeader(fp,&vh,grp,elem);
  if (error && error!=106 && error!=107) {
    printf("Read Error %d in group %s element %s\n",error,grp,elem);
    exit(-1);
  }

  if (vh.gen.data_type!= MOVIE0) {
    printf("You must specify a movie file to convert\n");
    exit(-1);
  }
  if (sscanf(argv[2],"%d",&st_frame)!=1) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }
  if (sscanf(argv[3],"%d",&end_frame)!=1) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }
  
  if (st_frame==-1 && end_frame==-1) {
    st_frame=0;
    end_frame=vh.dsp.num_of_images-1;
  }	  
  if (st_frame < 0 ||
      st_frame >= vh.dsp.num_of_images ) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }

  if (end_frame < 0 ||
      end_frame >= vh.dsp.num_of_images ) {
    printf("Incorrect frame number specified\n");
    exit(-1);
  }
    
  
  if (vh.dsp.num_of_bits==8 && vh.dsp.num_of_elems==1)  {
    BuildColorLookup(ctable,&vh);
    bytes=1;
  }
  else if (vh.dsp.num_of_bits==24 && vh.dsp.num_of_elems==3) {
    bytes=3;
  }
  else {
    printf("Cannot handle this MOVIE format\n");
    exit(-1);
  }

  if (end_frame < st_frame) 
    incr= -1;
  else 
    incr=1;

  width=vh.dsp.xysize[0];
  height=vh.dsp.xysize[1];
  in_size =width*height*bytes;
  out_size=width*height*3;
  in_data=(unsigned char *)malloc(in_size);
  if (bytes==1) 
    out_data=(unsigned char *)calloc(out_size,1);
  else
    out_data=in_data;
  if (in_data==NULL || out_data == NULL) {
    printf("Could Not allocate space\n");
    exit(-1);
  }

  for(fr=st_frame;fr<=end_frame; fr+=incr) {
    sprintf(outfile,"%s_%d.ppm",argv[4],fr);
    printf("Writing ppm file %s   \n",outfile);
    fflush(stdout);
    outfp=fopen(outfile, "wb");
    if (outfp==NULL) {
      printf("Could not open output file\n");
      exit(-1);
    } 
    
    /* PPM Header */
    fprintf(outfp,"P6\n");
    fprintf(outfp,"%d\n%d\n255\n",width,height);
    
    
    /* Read Current 3dviewnix frame */
    VSeekData(fp,in_size*fr);
    if (VReadData((char *)in_data,1,in_size,fp,&i)) {
      printf("Error in reading data\n");
      exit(-1);
    }
    
    /* Convert data */
    if (vh.dsp.num_of_bits==8) {
      for(out=out_data,in=in_data,i=in_size;i>0;in++,out+=3,i--) {
	*out     = ctable[*in][0];
	*(out+1) = ctable[*in][1];
	*(out+2) = ctable[*in][2];
      }
    }
    
    if (fwrite(out_data,1,out_size,outfp)!=out_size) {
      printf("Error in writing data\n");
      exit(-1);
    }
    
    fclose(outfp);
    
  }
  printf("done writing raster files for frames %d through %d\n",st_frame,end_frame);
  fclose(fp);
  
  exit(0);

}





void BuildColorLookup(unsigned char ct[256][3], ViewnixHeader *vh)
{

  int i,shift, len;

  
  /* Generate Color Map */
  if (vh->gen.red_descriptor_valid &&
      vh->gen.green_descriptor_valid &&
      vh->gen.blue_descriptor_valid) {
    
    len=(vh->gen.red_descriptor[0]+vh->gen.red_descriptor[1]);
    shift= vh->gen.red_descriptor[2] - 8;
    for(i=vh->gen.red_descriptor[1]; i< len;
	i++) {
      ct[i][0]   = vh->gen.red_lookup_data[i]   >>  shift;
      ct[i][1]   = vh->gen.green_lookup_data[i] >>  shift;
      ct[i][2]   = vh->gen.blue_lookup_data[i]  >>  shift;
    }
    
  }
  else {
    for(i=0;i<256;i++)
      ct[i][0]=ct[i][1]=ct[i][2]=i;
  }


}

