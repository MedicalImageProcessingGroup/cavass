/*
  Copyright 1993-2014, 2016 Medical Image Processing Group
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
    unsigned char *grey_buffer, int min_value, int max_value, int merge_data);

/* Modified: 1/24/01 exit(0) called at end by Dewey Odhner. */
/* Modified: 5/11/01 new format used (4 byte areas) by Dewey Odhner. */
int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,slices1,error,current_object,count, merge_data=0;
  ViewnixHeader vh1,vh2;
  FILE *in1,*in2,*out;
  unsigned char *data1,*d1_8;
  char group[6],elem[6], *IM0_name;
  static char msk_file_name[100];
  static unsigned char msk_obj_on_flag;
  static unsigned msk_obj_area[8];
  static unsigned char bit_msk[8] = { 1,2,4,8,16,32,64,128 };



  if (argc!= 5) {
    printf("Usage: bin_to_MSK <IM0_file> <BIM_file> <MSK_file> <object_number> \n");
    printf("     Object_number should be between 0 - 7\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  in2=fopen(argv[2],"rb");
  out=fopen(argv[3],"rb+");
  if (out == NULL)
    out = fopen(argv[3],"wb+");

  if (sscanf(argv[4],"%d",&current_object)!=1 || 
      current_object < 0 || current_object >7) {
    printf("Incorrect object number specified\n");
    exit(-1);
  }

  for (IM0_name=argv[1]+strlen(argv[1])-2; IM0_name>argv[1]; IM0_name--)
    if (*IM0_name=='/' || *IM0_name=='\\')
	{
	  IM0_name++;
	  break;
	}

  if (in1==NULL || in2==NULL || out==NULL) {
    printf("Error in opening the files\n");
    exit(-1);
  }
  
  error=VReadHeader(in1,&vh1,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  
  error=VReadHeader(in2,&vh2,group,elem);
  if (error<=104) {
    printf("Fatal error in reading header\n");
    exit(-1);
  }
  
  if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0) {
    printf("Both input files should be IMAGE0\n");
    exit(-1);
  }
  if (vh2.scn.num_of_bits!=1) {
    printf("The Mask file should be binary\n");
    exit(-1);
  }
  if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] || vh1.scn.xysize[1]!=vh2.scn.xysize[1]) {
    printf("Both Scenes should be of the same width and height\n");
    exit(-1);
  }

  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  slices1= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
  if (slices < slices1) { 
    printf( "Mask Scene has more slices\n");
    printf( "Ignoring the extra slices\n");
    slices1=slices;
  }


  size = vh2.scn.xysize[0]* vh2.scn.xysize[1];
  size1= (size+7)/8;
  data1= (unsigned char *)malloc(size1);
  d1_8=(unsigned char *)malloc(size);


  VSeekData(in2,0);

  fseek(out, 0, 0);
  if (fread(msk_file_name, 1, 100, out)==100 &&
      strcmp(msk_file_name, "///")==0 &&
	  strncmp(msk_file_name+4, IM0_name, 95)==0)
  {
    merge_data = 1;
  }
  else
  {
    fseek(out, 0, 0);
    strcpy(msk_file_name, "///");
    strncpy(msk_file_name+4, IM0_name, 95);
    msk_file_name[99] = 0;
    if (fwrite(msk_file_name,1,100,out)!=100) {
      printf("Error in writing MSK file\n");
      exit(-1);
    }
  }


  for(i=0;i<slices;i++) {
    printf("Processing slice %d\r",i);
    if (VReadData((char *)data1,1,size1,in2,&j)) {
      printf("Could not read data\n");
      exit(-1);
    }
    if (merge_data)
	{
	  fseek(out, 100+i*(33+size), 0);
	  if (fread(&msk_obj_on_flag,1,1,out)==1 &&
	      VReadData((char *)msk_obj_area,4,8,out,&j)==0 &&
	      fread(d1_8,1,size,out)==size)
	  {
	    msk_obj_on_flag &= ~bit_msk[current_object];
	  }
	  else
	  {
	    memset(d1_8, 0, size);
		memset(msk_obj_area, 0, 32);
		msk_obj_on_flag = 0;
	  }
	}
	else
	  msk_obj_on_flag = 0;
	fseek(out, 100+i*(33+size), 0);
    count = bin_to_grey(data1,size,d1_8,0,bit_msk[current_object], merge_data);
    msk_obj_area[current_object]=count;
    if (count) 
      msk_obj_on_flag |= bit_msk[current_object];

    if (fwrite(&msk_obj_on_flag,1,1,out)!=1) {
      printf("Error in writing MSK file\n");
      exit(-1);
    }
    if (VWriteData((char *)msk_obj_area,4,8,out,&j)) {
      printf("Error in writing MSK file\n");
      exit(-1);
    }
    if (VWriteData((char *)d1_8,1,size,out,&j)) {
      printf("Could not write data\n");
      exit(-1);
    }      

  }
  
  
  fclose(in1);
  fclose(in2);
  fclose(out);
  exit(0);
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

int bin_to_grey(unsigned char *bin_buffer, int length,
    unsigned char *grey_buffer, int min_value, int max_value, int merge_data)
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin, *grey;
  int count;

  bin = bin_buffer;
  grey = grey_buffer;
  
  count=0;
  for(j=7,i=length; i>0; i--)    {
    if( (*bin & mask[j]) != 0)  {
      *grey = merge_data? (*grey)|max_value: max_value;
      count++;
    }
    else 
      *grey = merge_data? (*grey)&~max_value: min_value;
    
    grey++;
    if (j==0) {
      bin++; j=7;
    }
    else
      j--;
    
  }

  return count;
}

