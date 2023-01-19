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
#include <string.h>
#include <limits.h>
#include <cv3dv.h>
#include "manipulate.h"
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif
#include "neighbors.h"
typedef struct { unsigned short c,n; } Cord_with_Norm;
#define POSITIVE (unsigned short)(0x8000)
#define C_MASK (unsigned short)(0x7FFF)

void MakeMinMaxConsistant(StructureInfo *str, float *min_max),
  ReadData(Cord_with_Norm ***TSE, int *sl, int *rw), BuildBS0_with_norm();

int error, B_flag;
int SAME_MODE=FALSE;
char infile_name[100],outfile_name[100],normfile_name[100];
FILE *outfp,*infp;
static ViewnixHeader vh,outvh;
unsigned int WriteTSE(Cord_with_Norm **TSE, short *NTSE, int slices, int rows,
	int out_slices, int out_rows);

int main(argc,argv)
int argc;
char *argv[];
{
  char group[6],elem[6];
  int bg_flag;
  
  if (argc!=4) {
    fprintf(stderr,"Usage: BS1_TO_BS0 <BS1_file> <BS0_file> bg_flag\n");
    exit(-1);
  }


  strcpy(infile_name,argv[1]);
  strcpy(outfile_name,argv[2]);
  
  bg_flag=atoi(argv[3]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);

  
  
  if ((outfp=fopen(outfile_name,"w+"))==NULL) {
    fprintf(stderr,"Could not open %s\n",outfile_name);
    exit(-1);
  }    
  if ((infp=fopen(infile_name,"r"))==NULL) {
    fprintf(stderr,"Could not open %s\n",infile_name);
    exit(-1);
  }
  else {
    error=VReadHeader(infp,&vh,group,elem);
    if (error) printf("Read error %d group %s element %s\n",error,group,elem);
    if (error==104) exit(-1);
  }

  switch (vh.str.num_of_bits_in_TSE) {
  case 32:
    BuildBS0_with_norm();
    break;
  case 16:
  default:
    fprintf(stderr,"Unknown BS1 type. Could not convert file\n");
    exit(-1);
  }
  fseek(outfp,0L,2L);
  fclose(outfp);
  fclose(infp);

  if (bg_flag) VDeleteBackgroundProcessInformation();
  return(0);
 


}


/* Modified: 5/15/98 smallest and largest values set by Dewey Odhner. */
/* Modified: 6/7/01 new bit fields accommodated by Dewey Odhner. */
void BuildBS0_with_norm()
{
  int i,j,inhdr_len,slices,rows,out_slices,out_rows,cpos,num;
  short *OUT_NTSE;
  char group[6],elem[6];
  StructureInfo *str;
  Cord_with_Norm **TSE;
  float z_pix_size;

  /* Get Input header and modify it */
  fseek(infp,0L,0L);
  error=VReadHeader(infp,&outvh,group,elem);
  if (error) fprintf(stderr,"error %d group %s element %s\n",error,group,elem);
  if (error==104) exit(-1);
  
  /* Changes to the general header */
  outvh.gen.data_type=SHELL0;
  strcpy(outvh.gen.filename,outfile_name);
  
  /* Changes to the struct header */
  str= &outvh.str;
  if (str->dimension!=3) {
    fprintf(stderr,"this module cannot handle %d stuctures\n",str->dimension);
    exit(-1);
  }

  if (str->domain_valid) {
    for(i=0;i<str->num_of_structures;i++) {
      str->domain[i*12] += (str->domain[i*12+3]*str->xysize[0] + str->domain[i*12+6]*str->xysize[1] +
			    str->domain[i*12+9]*(str->loc_of_samples[1]-str->loc_of_samples[0]));
      str->domain[i*12+1] += (str->domain[i*12+4]*str->xysize[0] + str->domain[i*12+7]*str->xysize[1] +
			 str->domain[i*12+10]*(str->loc_of_samples[1]-str->loc_of_samples[0]));
      str->domain[i*12+2] += (str->domain[i*12+5]*str->xysize[0] + str->domain[i*12+8]*str->xysize[1] +
			 str->domain[i*12+11]*(str->loc_of_samples[1]-str->loc_of_samples[0]));
    }
  }

  B_flag = FALSE;
  for(i=0;i<str->num_of_structures;i++)
    if (str->largest_value[i*9+1]/2 > 1023)
      B_flag = TRUE;
  str->num_of_bits_in_TSE=32;
  str->bit_fields_in_TSE[0]=0;    str->bit_fields_in_TSE[1]=5;
  if (B_flag)
  {
    str->bit_fields_in_TSE[2]=6;    str->bit_fields_in_TSE[3]=16;
    str->bit_fields_in_TSE[4]=16;   str->bit_fields_in_TSE[5]=15;
    str->bit_fields_in_TSE[6]=17;   str->bit_fields_in_TSE[7]=19;
    str->bit_fields_in_TSE[8]=20;   str->bit_fields_in_TSE[9]=25;
    str->bit_fields_in_TSE[10]=26;  str->bit_fields_in_TSE[11]=31;
  }
  else
  {
    str->bit_fields_in_TSE[2]=6;    str->bit_fields_in_TSE[3]=15;
    str->bit_fields_in_TSE[4]=16;   str->bit_fields_in_TSE[5]=15;
    str->bit_fields_in_TSE[6]=21;   str->bit_fields_in_TSE[7]=23;
    str->bit_fields_in_TSE[8]=24;   str->bit_fields_in_TSE[9]=27;
    str->bit_fields_in_TSE[10]=28;  str->bit_fields_in_TSE[11]=31;
  }
  str->bit_fields_in_TSE[12]=32;  str->bit_fields_in_TSE[13]=31;
  str->bit_fields_in_TSE[14]=40;  str->bit_fields_in_TSE[15]=39;
  str->bit_fields_in_TSE[16]=40;  str->bit_fields_in_TSE[17]=39;

  /* double pixel sizes */
  str->xysize[0] *=2;
  str->xysize[1] *=2;

  /* init slice locations */
  if (str->num_of_samples_valid)
    str->num_of_samples[0]= (str->num_of_samples[0]-1)/2;
  z_pix_size=(str->loc_of_samples[1]-str->loc_of_samples[0]);
  if (str->loc_of_samples_valid)
    for(i=0;i<str->num_of_samples[0];i++) {
      str->loc_of_samples[i]=str->loc_of_samples[2*i+1] - z_pix_size;
    }
  z_pix_size= (float)fabs(str->loc_of_samples[1]-str->loc_of_samples[0]);

  if (str->largest_value_valid == 0)
    str->largest_value=(float *)calloc(9*str->num_of_structures,sizeof(float));
  if (str->smallest_value_valid == 0)
    str->smallest_value =
	  (float *)calloc(9*str->num_of_structures, sizeof(float));
  if (str->largest_value==NULL || str->smallest_value==NULL)
  {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  for(i=0;i<str->num_of_structures;i++) {
    if (str->smallest_value_valid) {
      str->smallest_value[i*9]=0;
      str->smallest_value[i*9+1] /=2.0;
    }
    if (str->largest_value_valid) {
      str->largest_value[i*9]=63.0;
      str->largest_value[i*9+1]= (float)((str->largest_value[i*9+1]-2)/2.0);
    }

    /* Setup num min_max coords */
    if (str->min_max_coordinates_valid) {
      str->min_max_coordinates[i*6+3] -= str->xysize[0];
      str->min_max_coordinates[i*6+4] -= str->xysize[1];
      str->min_max_coordinates[i*6+5] -= z_pix_size;
    }
    MakeMinMaxConsistant(str,str->min_max_coordinates+i*6);

    if (str->smallest_value_valid == 0)
      str->smallest_value[i*9+1] =
	    (float)rint(str->min_max_coordinates[i*6]/str->xysize[0]);
    if (str->largest_value_valid == 0) {
      str->largest_value[i*9] = 63.0;
      str->largest_value[i*9+1] =
	    (float)rint(str->min_max_coordinates[i*6+3]/str->xysize[0]);
      str->largest_value[i*9+3] = 6;
      str->largest_value[i*9+4] = 15;
      str->largest_value[i*9+5] = 15;
    }
  }
  str->smallest_value_valid = str->largest_value_valid = 1;
  error=VWriteHeader(outfp,&outvh,group,elem);
  if (error) {
    fprintf(stderr,"Error %d in writing output header group %s element %s\n",error,group,elem);
    if (error<=104) {
      fprintf(stderr,"exiting program before completion\n");
      exit(-1);
    }
  }
  
  fseek(infp,0L,0L);
  VGetHeaderLength(infp,&inhdr_len);
  /* skip to the start of data */
  VSeekData(infp,0);
  VSeekData(outfp,0);
  /* Process each structure */
  for(i=0;i<str->num_of_structures;i++) {
    printf("Processing surface %d\n",i+1);
    fflush(stdout);
    ReadData(&TSE,&slices,&rows);
    /* Make Space for NTSE for this structure */
    out_slices= (slices-1)/2;
    out_rows  = (rows  -1)/2;
    str->num_of_NTSE[i]=1+out_slices+out_rows*out_slices;
    OUT_NTSE=(short *)malloc(str->num_of_NTSE[i]*sizeof(short));
    if (OUT_NTSE==NULL) {
      fprintf(stderr,"Could not allocate sufficient space for out data \n");
      exit(-1);
    }
    /* store the file pointer to the start of NTSE */
    cpos=ftell(outfp);
    /* Resereve space for NTSE */
    if (VWriteData((char *)OUT_NTSE,2,str->num_of_NTSE[i],outfp,&num)) {
      fprintf(stderr,"Could not allocate sufficient space for out data \n");
      exit(-1);
    }      
    /* Write the TSE segements */
    str->num_of_TSE[i]=WriteTSE(TSE,OUT_NTSE,slices,rows,out_slices,out_rows);
    /* Go back and modify NTSE's */
    fseek(outfp,cpos,0L);
    OUT_NTSE[0]=out_slices;
    for(j=0;j<out_slices;j++)
      OUT_NTSE[1+j]=out_rows;
    if (VWriteData((char *)OUT_NTSE,2,str->num_of_NTSE[i],outfp,&num)) {
      fprintf(stderr,"Could not allocate sufficient space for out data \n");
      exit(-1);
    }      
    /* Restore file pointer to the end of data */
    fseek(outfp,0L,2L);
    free(OUT_NTSE);
    free(TSE[0]);
    free(TSE);
  }
  fseek(outfp,0L,0L);
  error=VWriteHeader(outfp,&outvh,group,elem);
  if (error) {
    fprintf(stderr,"Error %d in writing output header group %s element %s\n",error,group,elem);
    if (error<=104) {
      fprintf(stderr,"exiting program before completion\n");
      exit(-1);
    }
  }
    
}


void ReadData(Cord_with_Norm ***TSE, int *sl, int *rw)
{
  int i,j,total,size,num;
  short count;
  short rows,slices,tmp_row;
  Cord_with_Norm *ptr;
  unsigned int *TSE_count;

  /* Asssume infp points to the first NTSE */
  if (VReadData((char *)&slices,sizeof(short),1,infp,&num)) {
    fprintf(stderr,"Could not read Data\n");
    exit(-1);
  }
  if (VReadData((char *)&rows,sizeof(short),1,infp,&num)) {
    fprintf(stderr,"Could not read Data\n");
    exit(-1);
  }

  for(j=1;j<slices;j++) {
    VReadData((char *)&tmp_row,sizeof(short),1,infp,&num);
    if (rows!=tmp_row) {
      fprintf(stderr,"The surface should have equal number of rows per slice\n"); 
      exit(-1);
    }
  }
  total=slices*rows+1;
  *TSE=(Cord_with_Norm **)malloc(sizeof(Cord_with_Norm *)*total);
  TSE_count=(unsigned int *)malloc(sizeof(int)*total);
  if ( *TSE == NULL || TSE_count==NULL) {
    fprintf(stderr,"Could Not Allocate Space for the input data\n");
    exit(-1);
  }
  
  TSE_count[0]=0;
  size=0;
  for(i=1;i<total;i++){
    VReadData((char *)&count,sizeof(short),1,infp,&num);
    TSE_count[i]= TSE_count[i-1] + count;
    size += count;
  }
  
  if (size!=0) {
    (*TSE)[0]=(Cord_with_Norm *)malloc(sizeof(Cord_with_Norm)*size);
    if ((*TSE)[0] == NULL) {
      fprintf(stderr,"Could not allocate space for the input data\n");
      exit(-1);
    }
    for(i=1;i<total;i++)
      (*TSE)[i]= (*TSE)[0] + TSE_count[i];
    for(ptr= (*TSE)[0],i=0;i<size;ptr++,i++) {
      if ( VReadData((char *)&ptr->c,2,1,infp,&num)) {
	fprintf(stderr,"Could not read input data\n");
	exit(-1);
      }
      if ( VReadData((char *)&ptr->n,2,1,infp,&num)) {
	fprintf(stderr,"Could not read input data\n");
	exit(-1);
      }
    }
  }
  free(TSE_count);
  *rw=rows; *sl=slices;
}
      
/* Modified: 6/7/01 new bit fields accommodated by Dewey Odhner. */
unsigned int WriteTSE(Cord_with_Norm **TSE, short *NTSE, int slices, int rows,
	int out_slices, int out_rows)
{
  int i,j,num;
  Cord_with_Norm **sl_ptr,**row_ptr,**p1,**p2,**p3,**p4;
  Cord_with_Norm *st,*end,*node_up,*node_dw,*node_pr,*node_ne;
  unsigned short MASK,up,dw,pr,ne,x,x1,x2,outTSE[2],G_code();
  double nx,ny,nz,gx,gy,gz;
  short *ntse;
  unsigned int sum;

  sum=0; /* Number of TSE's */
  ntse= NTSE + 1 + out_slices;
  /* for each slice */
  for(sl_ptr=TSE+rows,i=1;i<slices;sl_ptr += rows*2,i+=2) {
    printf("Processing slice %d\n",i>>1);
    fflush(stdout);
    /* for each row */
    for(row_ptr=sl_ptr+1,p1=row_ptr-rows,p2=row_ptr+rows,
	p3=row_ptr-1,p4=row_ptr+1,j=1;
	j<rows;
	row_ptr+=2,p1+=2,p2+=2,p3+=2,p4+=2,j+=2) {
      st= *row_ptr;
      end= *(row_ptr+1);
      node_up = *p1; node_dw= *p2;
      node_pr = *p3; node_ne= *p4;
      if (node_up < *(p1+1))
	up=node_up->c&C_MASK; 
      else 
	up=0;
      if (node_dw < *(p2+1))
	dw=node_dw->c&C_MASK;
      else
	dw=0;
      if (node_pr < *(p3+1))
	pr=node_pr->c&C_MASK;
      else
	pr=0;
      if (node_ne < *(p4+1))
	ne=node_ne->c&C_MASK;
      else
	ne=0;
      *ntse=0;
      /* foreach column */
      while ( st+1 < end)  {
	x1= st->c&C_MASK;
	x2= (st+1)->c&C_MASK;
	for(x=x1+1;x<x2;x+=2) {
	  while ( up < x && (node_up < *(p1+1))) {
	    node_up++; up=node_up->c&C_MASK;
	  } 
	  while (dw < x && (node_dw < *(p2+1))) {
	    node_dw++;dw=node_dw->c&C_MASK;
	  }
	  while (pr < x && (node_pr < *(p3+1))) {
	    node_pr++;pr=node_pr->c&C_MASK;
	  }
	  while (ne < x && (node_ne < *(p4+1))) {
	    node_ne++;ne=node_ne->c&C_MASK;
	  }
	  nx=ny=nz=0.0;
	  MASK= ALL_NEIGHBORS;
	  if (up==x) {
	    MASK ^=NZ;
	    G_decode(gx,gy,gz,node_up->n);
	    nx+= gx;
	    ny+= gy;
	    nz+= gz;
	  }
	  if (dw==x) {
	    MASK ^=PZ;
	    G_decode(gx,gy,gz,node_dw->n);
	    nx+= gx;
	    ny+= gy;
	    nz+= gz;
	  }
	  if (pr==x) {
	    MASK ^=NY;
	    G_decode(gx,gy,gz,node_pr->n);
	    nx+= gx;
	    ny+= gy;
	    nz+= gz;
	  }
	  if (ne==x) {
	    MASK ^=PY;
	    G_decode(gx,gy,gz,node_ne->n);
	    nx+= gx;
	    ny+= gy;
	    nz+= gz;
	  }
	  if (x==x1+1) {
	    MASK ^=NX;
	    G_decode(gx,gy,gz,st->n);
	    nx+= gx;
	    ny+= gy;
	    nz+= gz;
	  }
	  if (x==x2-1) {
	    MASK ^=PX;
	    G_decode(gx,gy,gz,(st+1)->n);
	    nx+= gx;
	    ny+= gy;
	    nz+= gz;
	  }
	  if (MASK!= ALL_NEIGHBORS) {
	    (*ntse)++; sum++;
	    if (B_flag)
	    {
	      outTSE[0]=(x-1)/4; outTSE[0]|=MASK;
	      outTSE[1]=BG_code(nx,ny,nz); if ((x-1) & 2) outTSE[1]|=0x8000;
	    }
	    else
	    {
	      outTSE[0]=(x-1)/2; outTSE[0]|=MASK;
	      outTSE[1]=G_code(nx,ny,nz);
	    }
	    if (VWriteData((char *)outTSE,2,2,outfp,&num)) {
	      fprintf(stderr,"Could not write output data\n");
	      exit(-1);
	    }
	  }
	}
	st += 2;
      }
      ntse++;
    }
  }
  printf("\n");
  return(sum);
}



void MakeMinMaxConsistant(StructureInfo *str, float *min_max)
{
  int i,best_min,best_max;
  float diff_min,diff_max;


  best_min=0;
  diff_min=(float)fabs(min_max[2]-str->loc_of_samples[best_min]);
  best_max=0;
  diff_max=(float)fabs(min_max[5]-str->loc_of_samples[best_max]);
  
  for(i=1;i<str->num_of_samples[0];i++) {
    if ( (float)fabs(min_max[2]-str->loc_of_samples[i]) < diff_min) {
      best_min=i;
      diff_min=(float)fabs(min_max[2]-str->loc_of_samples[best_min]);
    }
    if ( (float)fabs(min_max[5]-str->loc_of_samples[i]) < diff_max) {
      best_max=i;
      diff_max=(float)fabs(min_max[5]-str->loc_of_samples[best_max]);
    }
  }

  min_max[2]=str->loc_of_samples[best_min];
  min_max[5]=str->loc_of_samples[best_max];


}
