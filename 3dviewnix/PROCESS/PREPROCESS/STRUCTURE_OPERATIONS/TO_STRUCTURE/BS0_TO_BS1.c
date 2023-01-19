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
#include "neighbors.h"
typedef struct { unsigned short c,n; } Cord_with_Norm;
typedef struct { unsigned short c,n; } Out_Cord;
 
#define POSITIVE (unsigned short)(0x8000)
#define C_MASK (unsigned short)(0x7FFF)
#if CHAR_MIN < 0
#define SIGNED_CHAR(c)  (c)
#else
#define SIGNED_CHAR(c) ( (c)<=127 ? (c) : (256 - (c)) )
#endif

void BuildBS1_with_norm(), ReadData(Cord_with_Norm ***TSE, int *sl, int *rw),
  Get8bitNormals(char n[3], double nx, double ny, double nz),
  MakeMinMaxConsistant(StructureInfo *str, float *min_max);

int error;
int SAME_MODE=FALSE;
char infile_name[100],outfile_name[100],normfile_name[100];
FILE *outfp,*infp;
static ViewnixHeader vh,outvh;
Out_Cord *list;
extern unsigned int WriteTSE(Cord_with_Norm **TSE, short *NTSE, int slices,
	int rows, int out_slices, int out_rows);

int main(argc,argv)
int argc;
char *argv[];
{
  char group[6],elem[6];
  int bg_flag;

  if (argc!=3 && argc!=4) {
    fprintf(stderr,"Usage: BS0_TO_BS1 <BS0_file> <BS1_file> bg_flag\n");
    exit(-1);
  }

  bg_flag=atoi(argv[3]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);
 

  strcpy(infile_name,argv[1]);
  strcpy(outfile_name,argv[2]);
  
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
    BuildBS1_with_norm();
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


void BuildBS1_with_norm()
{
  int i,j,inhdr_len,slices,rows,out_slices,out_rows,cpos,min_x,max_x,num;
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
  outvh.gen.data_type=SHELL1;
  strcpy(outvh.gen.filename,outfile_name);
  
  /* Changes to the struct header */
  str= &outvh.str;
  if (str->dimension!=3) {
    fprintf(stderr,"this module cannot handle %d stuctures\n",str->dimension);
    exit(-1);
  }

  if (str->domain_valid) {
    for(i=0;i<str->num_of_structures;i++) {
      str->domain[i*12+0] -= (float)((str->domain[i*12+3]*str->xysize[0] +
        str->domain[i*12+6]*str->xysize[1] +
          str->domain[i*12+9]*(str->loc_of_samples[1]-str->loc_of_samples[0]))/2.0);
      str->domain[i*12+1] -= (float)((str->domain[i*12+4]*str->xysize[0] +
        str->domain[i*12+7]*str->xysize[1] +
          str->domain[i*12+10]*(str->loc_of_samples[1]-str->loc_of_samples[0]))/2.0);
      str->domain[i*12+2] -= (float)((str->domain[i*12+5]*str->xysize[0] +
        str->domain[i*12+8]*str->xysize[1] +
        str->domain[i*12+11]*(str->loc_of_samples[1]-str->loc_of_samples[0]))/2.0);
    }
  }
  

  str->signed_bits_in_TSE[3]=str->signed_bits_in_TSE[4]=str->signed_bits_in_TSE[5]=1;
  
  str->num_of_bits_in_TSE=32;  
  str->bit_fields_in_TSE[0]=0;    str->bit_fields_in_TSE[1]=0;
  str->bit_fields_in_TSE[2]=1;    str->bit_fields_in_TSE[3]=15;

  /* double pixel sizes */
  str->xysize[0] /=2;
  str->xysize[1] /=2;

  /* init slice locations */
  if (str->num_of_samples_valid)
    str->num_of_samples[0]= str->num_of_samples[0]*2+1;
  if (str->loc_of_samples_valid) {
    /* allocate more space for locations */
    free(str->loc_of_samples);
    str->loc_of_samples=(float *)malloc(sizeof(float)*str->num_of_samples[0]);
    /* first location */
    str->loc_of_samples[0]= (float)((3*vh.str.loc_of_samples[0]-vh.str.loc_of_samples[1])/2.0);
    for(j=0,i=1;i<str->num_of_samples[0];j++,i+=2)
      str->loc_of_samples[i]=vh.str.loc_of_samples[j];
    for(i=2;i<str->num_of_samples[0]-1;j++,i+=2)
      str->loc_of_samples[i]= (float)((str->loc_of_samples[i-1]+str->loc_of_samples[i+1])/2.0);
    str->loc_of_samples[str->num_of_samples[0]-1]=
      2*str->loc_of_samples[str->num_of_samples[0]-2] - 
    str->loc_of_samples[str->num_of_samples[0]-3];
    /* shift the locations of the data to accomodate the change in 
       the domain of the system */
    z_pix_size=(str->loc_of_samples[1]-str->loc_of_samples[0]);
    for(i=0;i<str->num_of_samples[0];i++) 
      str->loc_of_samples[i] += z_pix_size;
      
  }
  z_pix_size= (float)fabs(str->loc_of_samples[1]-str->loc_of_samples[0]);

  min_x=0xFFFF;
  max_x=0;
  for(i=0;i<str->num_of_structures;i++) {
    if (str->smallest_value_valid) {
      str->smallest_value[i*9+0]=0;
      str->smallest_value[i*9+1]*=2;
    }
    if (min_x>str->smallest_value[i*9+1]) min_x=(int)str->smallest_value[i*9+1];

    if (str->largest_value_valid) {
      str->largest_value[i*9+0]=1.0;
      str->largest_value[i*9+1]=str->largest_value[i*9+1]*2+2;
    }
    if (max_x<str->largest_value[i*9+1]) max_x=(int)str->largest_value[i*9+1];
    
    /* Setup num min_max coords */
    if (str->min_max_coordinates_valid) {
      str->min_max_coordinates[i*6+3] += (2*str->xysize[0]);
      str->min_max_coordinates[i*6+4] += (2*str->xysize[1]);
      str->min_max_coordinates[i*6+5] += (2*z_pix_size);
    }
    MakeMinMaxConsistant(str,str->min_max_coordinates+i*6);
  } 


  list=(Out_Cord *)malloc((int)(max_x-min_x+1)*sizeof(Out_Cord));
  if (list==NULL) {
    fprintf(stderr,"Memory allocation error\n");
    exit(-1);
  }
  error=VWriteHeader(outfp,&outvh,group,elem);
  if (error) {
    printf("Error %d in writing output header group %s element %s\n",error,group,elem);
    if (error<=104) {
      fprintf(stderr,"exiting program before completion\n");
      exit(-1);
    }
  }
  
  fseek(infp,0L,0L);
  VGetHeaderLength(infp,&inhdr_len);
   /* skip to the start of data */
  fseek(infp,inhdr_len,0L);
  fseek(outfp,0L,2L);
  /* Process each structure */
  for(i=0;i<str->num_of_structures;i++) {
    printf("Processing surface %d    ",i+1); 
    fflush(stdout);
    ReadData(&TSE,&slices,&rows);
    /* Make Space for NTSE for this structure */
    out_slices= 2*slices+1;
    out_rows  = 2*rows+1;
    /* Update NTSE */
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
      fprintf(stderr,"Error in writing data\n");
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
      fprintf(stderr,"Error in writing data\n");
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
    printf("Error %d in writing output header group %s element %s\n",error,group,elem);
    if (error<=104) {
      fprintf(stderr,"exiting program before completion\n");
      exit(-1);
    }
  }
  printf("\n");
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
  int i,j,j1,k,num_list, cmp(const void *, const void *), num, x;
  double nx,ny,nz;
  Cord_with_Norm *st,*end,**sl_ptr,**ptr,**ptr1,**row_ptr;
  unsigned short coord,SL_TYPE, gcode;
  short *ntse;
  unsigned int sum;

#define Assign_x_gcode \
if (vh.str.bit_fields_in_TSE[3] > 15) \
{ \
  x = (st->c&XMASK)<<1|((st->n&0x8000)!=0); \
  BG_decode(nx,ny,nz, st->n&0x7fff) \
  gcode = G_code(nx,ny,nz); \
} \
else \
{ \
  x = st->c&XMASK; \
  gcode = st->n; \
}


  sum=0;
  ntse= NTSE + 1 + out_slices;
  memset(ntse,0,2*out_slices*out_rows);
  /* Do first slice (can have only Z-faces)*/
  for(ptr=TSE,j1=1,j=0;j<rows;ptr++,j1+=2,j++) {
    st= *ptr; end= *(ptr+1);
    while ( st < end) {
      if (!(st->c&NZ)) { /* There is a Z-face */
        ntse[j1]++;sum++;
        Assign_x_gcode
        coord= (x<<1)+1;
        if (VWriteData((char *)&coord,2,1,outfp,&num)) {
          fprintf(stderr,"Could not write output data\n");
          exit(-1);
        }
        if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
          fprintf(stderr,"Could not write output data\n");
          exit(-1);
        }
      }
      st++;
    }
  }
  for(sl_ptr=TSE,SL_TYPE=1,i=1,ntse+=out_rows;
      i<out_slices-1; SL_TYPE= ~SL_TYPE,ntse+=out_rows,i++) {
    printf("Processing slice %d\n",i);
    fflush(stdout);
    if (SL_TYPE==1) { /* it is a X or Y face */
      /* Do Fisrt row (has to be Y faces) */
      st=*sl_ptr; end= *(sl_ptr+1);
      while (st<end) {
        if (!(st->c&NY)) {
          ntse[0]++;sum++;
          Assign_x_gcode
          coord= (x<<1)+1;
          if (VWriteData((char *)&coord,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
          if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
        }
        st++;
      }
      for(row_ptr=sl_ptr,j=1;j<out_rows-2;row_ptr++,j+=2) {
        /* Do X faces */
        st= *row_ptr; end= *(row_ptr+1); ntse[j]=0;
        while ( st< end) {
          if (!(st->c&NX)) {
            ntse[j]++;sum++;
            Assign_x_gcode
            coord= (x<<1) ;
            if (VWriteData((char *)&coord,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
            if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
          }
          if (!(st->c&PX)) {
            ntse[j]++;sum++;
            Assign_x_gcode
            coord= POSITIVE + (x<<1) +2;
            if (VWriteData((char *)&coord,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
            if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
          }
          st++;
        }
        /* Do Y faces */
        st= *row_ptr; end= *(row_ptr+1);
        num_list=0;
        while ( st< end) {
          if (!(st->c&PY)) {
            ntse[j+1]++;sum++;
            Assign_x_gcode
            list[num_list].c= POSITIVE+(x<<1)+1;
            list[num_list].n=gcode;
            num_list++;
          }
          st++;
        }
        st= *(row_ptr+1); end= *(row_ptr+2);
        while ( st< end) {
          if (!(st->c&NY)) {
            ntse[j+1]++;sum++;
            Assign_x_gcode
            list[num_list].c= (x<<1)+1;
            list[num_list].n=gcode;
            num_list++;
          }
          st++;
        }
        if (num_list) {
          qsort(list,num_list,sizeof(Out_Cord),cmp);
          for(k=0;k<num_list;k++) {
            if (VWriteData((char *)&list[k].c,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
            if (VWriteData((char *)&list[k].n,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
          }
        }
      }
      /* Last X face row */
      st = *(sl_ptr + rows-1); end=  *(sl_ptr+rows);
      while ( st< end) {
        if (!(st->c&NX)) {
          ntse[out_rows-2]++;sum++;
          Assign_x_gcode
          coord= (x<<1);
          if (VWriteData((char *)&coord,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
          if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
        }
        if (!(st->c&PX)) {
          ntse[out_rows-2]++;sum++;
          Assign_x_gcode
          coord= POSITIVE+(x<<1) +2;
          if (VWriteData((char *)&coord,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
          if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
        }
        st++;
      }
      /* Last +Y face row */
      st = *(sl_ptr + rows-1); end=  *(sl_ptr+rows);
      while (st < end) {
        if (!(st->c&PY)) {
          ntse[out_rows-1]++;sum++;
          Assign_x_gcode
          coord= POSITIVE+(x<<1)+1;
          if (VWriteData((char *)&coord,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
          if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
            fprintf(stderr,"Could not write output data\n");
            exit(-1);
          }
        }
        st++;
      }
    }
    else { /* it is a Z face */
      for(ptr=sl_ptr,ptr1=sl_ptr+rows,j1=1,j=0;j<rows;ptr1++,ptr++,j1+=2,j++) {
        st= *ptr; end= *(ptr+1);
        num_list=0;
        while ( st < end) {
          if (!(st->c&PZ)) { /* There is a Z-face */
            ntse[j1]++;sum++;
            Assign_x_gcode
            list[num_list].c= POSITIVE +(x<<1)+1;
            list[num_list].n=gcode;
            num_list++;
          }
          st++;
        }
        st= *ptr1; end= *(ptr1+1);
        while ( st < end) {
          if (!(st->c&NZ)) { /* There is a Z-face */
            ntse[j1]++;sum++;
            Assign_x_gcode
            list[num_list].c=(x<<1)+1;
            list[num_list].n=gcode;
            num_list++;
          }
          st++;
        }
        if (num_list) {
          qsort(list,num_list,sizeof(Out_Cord),cmp);
          for(k=0;k<num_list;k++) {
            if (VWriteData((char *)&list[k].c,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
            if (VWriteData((char *)&list[k].n,2,1,outfp,&num)) {
              fprintf(stderr,"Could not write output data\n");
              exit(-1);
            }
          }
        }
      }
      sl_ptr+=rows; /* only gets incremented on EVEN slices */
    }
  }
  /* Last slice (has to be Z-faces */
  for(ptr=sl_ptr,j1=1,j=0;j<rows;ptr++,j1+=2,j++) {
    st= *ptr; end= *(ptr+1);
    while ( st < end) {
      if (!(st->c&PZ)) { /* There is a Z-face */
        ntse[j1]++;sum++;
        Assign_x_gcode
        coord= POSITIVE +(x<<1)+1;
        if (VWriteData((char *)&coord,2,1,outfp,&num)) {
          fprintf(stderr,"Could not write output data\n");
          exit(-1);
        }
        if (VWriteData((char *)&gcode,2,1,outfp,&num)) {
          fprintf(stderr,"Could not write output data\n");
          exit(-1);
        }
      }
      st++;
    }
  }
  
  printf("total number of faces written=%d\n",sum);

  return(sum);
}






void Get8bitNormals(char n[3], double nx, double ny, double nz)
{
  double factor;

  factor= 126.0/sqrt( nx*nx + ny*ny + nz*nz);
  n[0]= (char)rint(factor*nx);
  n[1]= (char)rint(factor*ny);
  n[2]= (char)rint(factor*nz);
}

int cmp(const void *vf1, const void *vf2)
{
  Out_Cord *f1=(Out_Cord *)vf1, *f2=(Out_Cord *)vf2;

  return( (int)(f1->c&C_MASK)- (int)(f2->c&C_MASK) );

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
