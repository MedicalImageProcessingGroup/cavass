/*
  Copyright 1993-2015, 2017-2018 Medical Image Processing Group
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

#if defined (WIN32) || defined (_WIN32) 
 #pragma  warning(disable:4996)  //necessary because bill's compiler deprecated stdio.h
#endif



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <cv3dv.h>
#include "neighbors.h"
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif

#define INVALID_NORM (6<< 2*G_COMPONENT_BITS)
#define BINVALID_NORM (6<< 2*BG_COMPONENT_BITS)

static int CHUNK_SIZE=25;

typedef unsigned short TSE_type[2];
extern unsigned short VoxOn(),G_code(),BG_code();
extern float VoxVal(int vol, int chunk, int slice, int row, int col);
extern int getpid(),
	SetupMedValues(float ratio),
	CheckUniformSpacing(SceneInfo *scn),
	ReadChunk(int vol, int chunk),
	InVoxVal(int vol, int chunk, int in_slice, int in_row, int in_col);
void InitGeneralHeader(short type),
	InitShell1(int vols, int shelltype),
	InitShell0(int vols),
	get_min_max_SHELL2_values(int vols),
	get_min_max_values(int vols),
	Append_Temp_TSE(int type),
	WriteTSE_SHELL1(int vol, int chunks, float min, float max),
	WriteTSE_SHELL0(int vol, int chunks, float min, float max),
	WriteTSE_SHELL2(int vol, int chunks, float min, float max),
	Get8Row(unsigned char *out, float min, float max, int vol, int chunk,
	    int slice, int row, float *p1, float *p2),
	Get16Row(unsigned char *out, float min, float max, int vol, int chunk,
	    int slice, int row, float *p1, float *p2),
	get_min_max_SHELL0_values(int vols),
	UnpackData(unsigned char *temp_slice, unsigned char *data,
	    int elems, int low, int hi),
	Create_Bin_Slice(int vol, int chunk, unsigned char *data, int slice,
		float min, float max),
	Calculate_Norm0(), Calculate_Norm8(), Calculate_Norm26();

static struct IN {
  char *filename;
  FILE *fp;
  ViewnixHeader vh;
  int vh_len,*skip_slices;
  short width,height,*slices,num_volumes;
  unsigned int bytes_per_slice,bits_per_pixel;
  float Px,Py,Pz,*slice_locs;
  short *ChunkOffsets,*ChunkSlices;
  unsigned char *Chunk;
} in;

typedef struct MED {
  short width,height,slices;
  unsigned int bytes_per_slice,bits_per_pixel,num_TSE;
  short *NTSE,min_x,max_x,min_z,max_z,min_y,max_y;
  float Px,Py,Pz;
  int volume;
} Med;

static Med *med;


static char *med_filename;
static FILE *med_fp;
static unsigned char *med_sl1,*med_sl2,*med_sl3;
static int MAX_IN_CHUNK,bg_flag;


static struct INTERPOLATE {
  short *ChunkOffsets,ChunkSlices;
  short x_flag,y_flag,z_flag;
  int xsize,ysize,zsize;
  short *x_table,*y_table,*z_table;
  float *x_dist,*y_dist,*z_dist;
} interp;

static struct OUT {
  char *filename;
  char *tempname;
  FILE *fp;
  ViewnixHeader vh;
  short file_type;
} out;


void (*Calculate_Norm)();
static int param_on;
static char param_file[400];
static float *str_volume, *str_surface_area;

				   
/************************************************************************
 *
 *      FUNCTION        : main
 *
 *      DESCRIPTION     : This program takes a scene and generates a
 *                        BS1, BS0, or a BS2 surface file. The valid
 *                        scene must have 1,8,16 bit data and have at 
 *                        least 2 slices. The output structure will
 *                        always have cubic voxels (faces).
 *
 *      RETURN VALUE    : 0 on normal completion.
 *
 *      PARAMETERS      : 9 essential and 1 optional parameter.
 *          argumnet 1 : The name of this process.
 *          argument 2 : Input scene. (IM0 or BIM file)
 *          argument 3 : Output structure file (BS0,BSI, BS1 or BS2 file)
 *          argument 4 : face size in units of the input scenes 
 *                       pixel width.
 *          argument 5 &
 *          argument 6 : minimum and maximum thresholds used for selecting
 *                       the intensity region of interest (inclusive of both
 *                       the min and the max values).
 *          argument 7 : Surfaced normal Type. (8 or 26 Neighbor normals)
 *                       8 neighbor normal cosiders intensity values at the
 *                       vertices of a cubic voxel with the same resoluton
 *                       as the output surface to compute the normal.
 *          argument 8 : Merge Flag. 
 *                       0 - Create New Structure System.
 *                       1 - Append to the Current Sturcuture system 
 *                           if one is available or crete a new one.
 *          argument 9 : Background Flag.
 *                       0 - running in the foreground.
 *                       1 - running in the backgroung.
 *          argument 10: Paramemter vector specification.
 *                       For each of the volumes in the input scene
 *                       the parameter vector associated with it.
 *                       Format: 
 *                         num_of_components_in_the_PV
 *                         descriptionPV[1] descriptionPV[2] .. .. ..
 *                         num_of_PV
 *                         PV1[1] PV1[2] .. ..
 *                         PV2[1] PV2[2] .. ..
 *                           :     :
 *                           :     :
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : If the merge flag is on and the current structure
 *                        is not consistant with the old.
 *
 *      RELATED FUNCS   : Calculate_Norm0,Calculate_Norm8,
 *                        Calculate_Norm26,SetupMedValues,
 *                        WriteTSE_SHELL1,WriteTSE_SHELL0,
 *                        InitGeneralHeader,InitShell1,InitShell0,
 *                        Append_Temp_TSE
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 2/21/00 negative pix_size allowed for
 *                           no interpolation by Dewey Odhner.
 *                        Modified: 8/16/07 param_file deleted by Dewey Odhner.
 *
 ************************************************************************/
int main(argc,argv)
int argc;
char *argv[];
{

  int i,norm_size,error,num_chunks,merge_flag, j, k;
  char grp[6],elem[6],*ptr, *exec_str, *tempname;
  float ratio,min_thresh,max_thresh;

  if (argc!=9 && argc!=10) {
    printf("Usage: %s <IMAGE_file> <BSn_file> pix_size min_thresh max_thresh normal merge_flag bg_flag [pv_file]\n",argv[0]);
    printf("IMAGE_file - This can be a binary or gray scene\n");
    printf("BSn_file   - This can be BS0, BS1, or BS2\n");
    printf("pix_size   - In units of the input pixel width.\n");
    printf("min_thresh max_thresh - Has to be between or equal to these values\n");
    printf("Normal     - It can be 8 or 26\n");
    printf("merge_flag - Merge new stucture to the one specified\n");
    printf("bg_flag    - 1 = running in the background\n");
    printf("             0 = running in the foreground\n");    
    printf("pv_file    - file containing the description of param vectors\n");
    printf("format:    num_pv\n");
    printf("           description_pv1 description_pv2 .. .. ..\n");
    printf("           volumes\n");
    printf("           pv1_vol1 pv2_vol1 .. .. \n");
    printf("           pv1_vol2 pv2_vol2 .. .. \n");
    printf("              :        :           \n");
    exit(-1);
  }

  if (argc==10) {
    param_on=TRUE;
    strcpy(param_file,argv[9]);
  }
  else
    param_on=FALSE;

  bg_flag=atoi(argv[8]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);

  merge_flag=atoi(argv[7]);

  if (!strcmp(argv[6],"26")) {
    Calculate_Norm=Calculate_Norm26; norm_size=26;
  }
  else if (!strcmp(argv[6],"8")) {
    Calculate_Norm=Calculate_Norm8; norm_size=8;
  }
  else if (!strcmp(argv[6],"0")) {
    Calculate_Norm=Calculate_Norm0; norm_size=0;
  }
  else  {
    printf("Only valid types for normals are 0, 8 and 26\n");
    exit(-1);
  }

#if defined (WIN32) || defined (_WIN32)
	k = '\\';
#else
	k = '/';
#endif
	for (j=(int)strlen(argv[1])-1; j>0; j--)
		if (argv[1][j] == k)
		{
			j++;
			break;
		}
  in.filename = (char *)malloc(strlen(argv[1])+1);
  strcpy(in.filename, argv[1]+j);
  if ((in.fp=fopen(argv[1],"rb"))==NULL) {
    printf("Could not open input file\n");
    exit(-1);
  }

  error=VReadHeader(in.fp,&in.vh,grp,elem);
  if (error<=104) {
    printf("Read Error %d ( group:%s element:%s )\n",error,grp,elem);
    printf("Cannot revoer from error\n");
  }

  
  min_thresh=(float)(atof(argv[4]));
  max_thresh=(float)(atof(argv[5]));
  
  if (in.vh.scn.dimension==3) 
    in.num_volumes=1;
  else 
    in.num_volumes=in.vh.scn.num_of_subscenes[0]; /* slices in 1st volume */
  


  if ((med=(Med *)calloc(sizeof(Med),in.num_volumes))==NULL ||
      (str_volume=(float *)malloc(sizeof(float)*in.num_volumes))==NULL ||
      (str_surface_area=(float *)malloc(sizeof(float)*in.num_volumes))==NULL) {
    printf("Could not allocate space for intermediate structures\n");
    exit(-1);
  }
  if ((in.slices=(short *)calloc(sizeof(short),in.num_volumes))==NULL) {
    printf("Could not allocate space for intermediate structures\n");
    exit(-1);
  }
  if ((in.slice_locs=(float *)calloc(sizeof(float),in.num_volumes))==NULL) {
    printf("Could not allocate space for intermediate structures\n");
    exit(-1);
  }
  if ((in.skip_slices=(int *)calloc(sizeof(int),in.num_volumes))==NULL) {
    printf("Could not allocate space for intermediate structures\n");
    exit(-1);
  }
  

  in.width=in.vh.scn.xysize[0]; in.height=in.vh.scn.xysize[1];
  VGetHeaderLength(in.fp,&in.vh_len);
  in.bits_per_pixel=in.vh.scn.num_of_bits;
  if (in.bits_per_pixel!=1) 
   Calculate_Norm= Calculate_Norm0;
  if (in.bits_per_pixel==1) 
    in.bytes_per_slice=in.width*in.height;
  else 
    in.bytes_per_slice= (in.width*in.height*in.vh.scn.num_of_bits+7)/8;
  in.Px=in.vh.scn.xypixsz[0];
  in.Py=in.vh.scn.xypixsz[1];
  if (CheckUniformSpacing(&in.vh.scn)<0) {
    printf("The Data has inconsistant slice spacing\n");
    exit(-1);
  }

  
  /* Setup intermidiate stucture */
  med_filename = (char *)malloc(24+strlen(argv[2]));
  sprintf(med_filename, "%sTRACK3D_%d.TEMP", argv[2], getpid());
  if ((med_fp=fopen(med_filename,"w+b"))==NULL) {
    printf("Could not open temp file %s for computations\n", med_filename);
    exit(-1);
  }
  if (sscanf(argv[3], "%f", &ratio) != 1) {
    printf("Specify magnification Correctly\n");
    exit(-1);
  }
  num_chunks=SetupMedValues(ratio);  

  if ((med_sl1=(unsigned char *)calloc(med[0].bytes_per_slice,1))==NULL) {
    printf("Could not allocate temp space\n");
    exit(-1);
  }
  if ((med_sl2=(unsigned char *)calloc(med[0].bytes_per_slice,1))==NULL) {
    printf("Could not allocate temp space\n");
    exit(-1);
  }
  if ((med_sl3=(unsigned char *)calloc(med[0].bytes_per_slice,1))==NULL) {
    printf("Could not allocate temp space\n");
    exit(-1);
  }



  /* Setup output structure */
  out.filename = (char *)malloc(strlen(argv[2])+1);
  strcpy(out.filename, argv[2]);
  ptr=strrchr(out.filename,'.');
  if (ptr==NULL) {
    printf("Output file should have BS0, BSI, BS1, or BS2 extension\n");
    exit(-1);
  }
  out.tempname = (char *)malloc(20+strlen(ptr));
  sprintf(out.tempname, "%sTEMP%dSHELL", ptr, getpid());
  if ((out.fp=fopen(out.tempname,"w+b"))==NULL) {
    printf("Could not open output file\n");
    exit(-1);
  }
  if (!strcmp(ptr,".BS0"))
    out.file_type=SHELL0;
  else if (!strcmp(ptr,".BSI"))
    out.file_type=SHELL0;
  else if (!strcmp(ptr,".BS1"))
    out.file_type=SHELL1;
  else if (!strcmp(ptr,".BS2"))
    out.file_type=SHELL2;
  else {
    printf("Output file should have BS0, BSI, BS1, or BS2 extension\n");
    exit(-1);
  }
    

  for(i=0;i<in.num_volumes;i++) {
    if (out.file_type==SHELL1) 
      WriteTSE_SHELL1(i,num_chunks,min_thresh,max_thresh);
    else if (out.file_type==SHELL0)
      WriteTSE_SHELL0(i,num_chunks,min_thresh,max_thresh);
	else
      WriteTSE_SHELL2(i,num_chunks,min_thresh,max_thresh);
  }

  
  InitGeneralHeader(out.file_type);
  switch (out.file_type) {
  case SHELL1:
    InitShell1(in.num_volumes, 1);
    printf("Volume in output voxels is %f\n",out.vh.str.volume[0]);
    error=VWriteHeader(out.fp,&out.vh,grp,elem);
    if (error <=104) {
      printf("Write Error %d (group:%s element:%s\n",error,grp,elem);
      printf("Cannot revoer from error\n");
      exit(-1);
    }
    break;
  case SHELL0:
    InitShell0(in.num_volumes);
    printf("Volume in output voxels is %f\n",out.vh.str.volume[0]);
    error=VWriteHeader(out.fp,&out.vh,grp,elem);
    if (error<=104) {
      printf("Write Error %d (group:%s element:%s\n",error,grp,elem);
      printf("Cannot revoer from error\n");
    }
	break;
  case SHELL2:
    InitShell1(in.num_volumes, 2);
    error=VWriteHeader(out.fp,&out.vh,grp,elem);
    if (error && error<=104) {
      printf("Write Error %d (group:%s element:%s\n",error,grp,elem);
      printf("Cannot recover from error\n");
      exit(-1);
    }
    break;
  }
  Append_Temp_TSE(out.file_type);
  fclose(in.fp);
  fclose(med_fp);
  fclose(out.fp);
  /* Remove Temp file */
  unlink(med_filename);


  /* Calculate Normals */
  if (in.bits_per_pixel!=1 && norm_size!=0) {
    exec_str = (char *)malloc(strlen(out.tempname)+strlen(argv[1])+
	  strlen(argv[6])+48);
    for(error=0,i=0;i<in.num_volumes && error==0;i++) {
      sprintf(exec_str,"to_normal \"%s\" %d \"%s\" %d \"%s\" 0",
	      out.tempname,i+1, argv[1], i, argv[6]);
      error=system(exec_str);
    }
  }
  else 
    error=0;
  if (error) return(error);

  if (merge_flag) {
    if ((out.fp=fopen(out.filename,"rb"))==NULL) {
      error = rename(out.tempname, out.filename);
    }
    else {
      fclose(out.fp);
	  tempname = (char *)malloc(strlen(out.tempname)+2);
      sprintf(tempname, "%sT", out.tempname);
      sprintf(exec_str, "merge_surface \"%s\" \"%s\" \"%s\" 0", out.filename,
	    out.tempname, tempname);
      error=system(exec_str);
      if (!error)  {
        if (!(error=unlink(out.tempname)))
		  error = unlink(out.filename);
		if (!error) {
		  error = rename(tempname, out.filename);
		}
      }
    }
  }
  else {
    unlink(out.filename);
    error = rename(out.tempname, out.filename);
  }
  if (param_on)
  	unlink(param_file);
  if (error) return(error);
  

  if (bg_flag) VDeleteBackgroundProcessInformation();
  return(0);
  
}


/************************************************************************
 *
 *      FUNCTION        : Append_Temp_TSE()
 *
 *      DESCRIPTION     : This function appends the TSE field from the 
 *                        temporary file onto the output file.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : type - SHELL0, SHELL1 or SHELL2.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in, out, med, med_fp, 
 *
 *      EXIT CONDITIONS : On read/write errors the process is aborted.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 7/1/03 SHELL2 added by Dewey Odhner.
 *                        Modified: 11/18/05 fread replaced by Dewey Odhner.
 *
 ************************************************************************/
void Append_Temp_TSE(int type)
{
  unsigned int max_TSE_buffer,current,i,j;
  int num;
  short height,slices,rows,*nt;

  /* Use in.chunk as the temp buffer */
  max_TSE_buffer=(MAX_IN_CHUNK*in.bytes_per_slice)>>2;
  
  VSeekData(out.fp,0L);      /* Go to the end of the file */
  fseek(med_fp,0L,0L);      /* Go the begining of TSE's  */
  for(i=0;i<(unsigned)in.num_volumes;i++) {
    if (type==SHELL1) 
      height=med[i].height;
    else  if (type==SHELL2) 
      height = (med[i].height>>1)+1;
    else
      height=med[i].height>>1;
    /* Write the TSE */
    slices=med[i].max_z-med[i].min_z+1;
    rows=med[i].max_y-med[i].min_y+1;
    if (VWriteData((char *)&slices,2,1,out.fp,&num)) {
        printf("Error in writing NTSE\n");
        exit(-1);
      }
    for(j=0;j<(unsigned)slices;j++)
      if (VWriteData((char *)&rows,2,1,out.fp,&num)) {
        printf("Error in writing NTSE\n");
        exit(-1);
      }
    for(j=med[i].min_z;j<=(unsigned)med[i].max_z;j++) {
      nt=med[i].NTSE+height*j+med[i].min_y;
      if (VWriteData((char *)nt,2,rows,out.fp,&num)) {
	printf("Error in writing NTSE\n");
	exit(-1);
      }
      
    }
    
    /* Append TSE */
    current=0;
	if (type == SHELL2)
	{
      while (current+max_TSE_buffer <= med[i].num_TSE) {
        if (VReadData((char *)in.Chunk,1,max_TSE_buffer,med_fp,&num)) {
          printf("Could not read temp TSE's\n");
          exit(-1);
        }
        if (VWriteData((char *)in.Chunk,1,max_TSE_buffer,out.fp,&num)) {
          printf("Could not append temp TSE's\n");
          exit(-1);
        }
        current += max_TSE_buffer;
      }
      if (current < med[i].num_TSE) {
        if (VReadData((char *)in.Chunk,1,med[i].num_TSE-current,med_fp,&num)) {
          printf("Could not read temp TSE's\n");
          exit(-1);
        }
        if (VWriteData((char *)in.Chunk,1,med[i].num_TSE-current,out.fp,&num)) {
          printf("Could not append temp TSE's\n");
          exit(-1);
        }
      }
	}
	else
	{
      while (current+max_TSE_buffer <= med[i].num_TSE) {
        if (VReadData((char *)in.Chunk,4,max_TSE_buffer,med_fp,&num)) {
          printf("Could not read temp TSE's\n");
          exit(-1);
        }
        if (VWriteData((char *)in.Chunk,4,max_TSE_buffer,out.fp,&num)) {
          printf("Could not append temp TSE's\n");
          exit(-1);
        }
        current += max_TSE_buffer;
      }
      if (current < med[i].num_TSE) {
        if (VReadData((char *)in.Chunk,4,med[i].num_TSE-current,med_fp,&num)) {
          printf("Could not read temp TSE's\n");
          exit(-1);
        }
        if (VWriteData((char *)in.Chunk,4,med[i].num_TSE-current,out.fp,&num) ) {
          printf("Could not append temp TSE's\n");
          exit(-1);
        }
      }
	}
  }



}





/************************************************************************
 *
 *      FUNCTION        : InitGeneralHeader()
 *
 *      DESCRIPTION     : Initialize the General Header. Is should be 
 *                        the same as  the input files exept for a few 
 *                        changes.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : type - SHELL0 or SHELL1.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in.vh, out.filename, in.filename must be initailized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera
 *                        Modified: 8/14/07 String length checked
 *                         by Dewey Odhner.
 *
 ************************************************************************/
void InitGeneralHeader(short type)
{

  /* You could use the same structure as the input Set */
  memcpy(&out.vh.gen,&in.vh.gen,sizeof(GeneralInfo));

  out.vh.gen.data_type=type;
  out.vh.gen.data_type_valid=1;

  strncpy(out.vh.gen.filename,out.filename, sizeof(out.vh.gen.filename)-1);
  out.vh.gen.filename[sizeof(out.vh.gen.filename)-1] = 0;
  out.vh.gen.filename_valid=1;

  strncpy(out.vh.gen.filename1,in.filename, sizeof(out.vh.gen.filename1)-1);
  out.vh.gen.filename1[sizeof(out.vh.gen.filename1)-1] = 0;
  out.vh.gen.filename1_valid=1;

}


/************************************************************************
 *
 *      FUNCTION        : InitShell1()
 *
 *      DESCRIPTION     : Initialize the StructureInfo segment of the
 *                        output header of SHELL1. This uses information 
 *                        generated by the tracking to initialize these 
 *                        values.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : vols - Number of volumes in the input scene.
 *                        shelltype - 1 or 2
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in,
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : get_min_max_values.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 4/9/98 in.filename copied to
 *                         out.vh.str.scene_file by Dewey Odhner.
 *                        Modified: 12/11/02 out.vh.str.domain adjustment
 *                         corrected by Dewey Odhner.
 *                        Modified: 7/8/03 for BS2 by Dewey Odhner.
 *                        Modified: 7/11/03 bit_fields_in_NTSE corrected
 *                          by Dewey Odhner.
 *                        Modified: 9/24/03 to store BS2 volume & area
 *                          by Dewey Odhner.
 *
 ************************************************************************/
void InitShell1(int vols, int shelltype)
{
  int i,j,temp_vols,elem;
  float min_slice_loc;
  StructureInfo *st;
  FILE *pfp;

  st= &out.vh.str;

  min_slice_loc = in.slice_locs[0];
  for(i=1;i<vols;i++) 
    if (min_slice_loc > in.slice_locs[i]) min_slice_loc=in.slice_locs[i];
  st->dimension=3;
  st->dimension_valid=1;

  st->num_of_structures=vols;
  st->num_of_structures_valid=1;

  st->domain=(float *)malloc(sizeof(float)*12*vols);
  for(j=0;j<vols;j++)  {
    if (in.vh.scn.dimension==3) {
      for(i=0;i<12;i++)
	st->domain[j*12+i]=in.vh.scn.domain[i];
    }
    else   /* 4D */
      for(i=0;i<4;i++) {
	st->domain[j*12+i*3]=in.vh.scn.domain[i*4];
	st->domain[j*12+i*3+1]=in.vh.scn.domain[i*4+1];
	st->domain[j*12+i*3+2]=in.vh.scn.domain[i*4+2];
      } 
    st->domain[j*12] -= st->domain[j*12+3]*med[0].Px +
      st->domain[j*12+6]*med[0].Py +
      st->domain[j*12+9]*(med[0].Pz-min_slice_loc);
    st->domain[j*12+1] -= st->domain[j*12+4]*med[0].Px +
      st->domain[j*12+7]*med[0].Py +
      st->domain[j*12+10]*(med[0].Pz-min_slice_loc);
    st->domain[j*12+2] -= st->domain[j*12+5]*med[0].Px +
      st->domain[j*12+8]*med[0].Py +
      st->domain[j*12+11]*(med[0].Pz-min_slice_loc);
  }
  st->domain_valid=1;

  if (in.vh.scn.axis_label_valid) {
    st->axis_label=(Char30 *)malloc(sizeof(Char30)*3);
    strcpy(st->axis_label[0],in.vh.scn.axis_label[0]);
    strcpy(st->axis_label[1],in.vh.scn.axis_label[1]);
    strcpy(st->axis_label[2],in.vh.scn.axis_label[2]);
    st->axis_label_valid=1;
  }
  else st->axis_label_valid=0;

  if (in.vh.scn.measurement_unit_valid) {
    st->measurement_unit=(short *)malloc(sizeof(short)*3);
    st->measurement_unit[0]=in.vh.scn.measurement_unit[0];
    st->measurement_unit[1]=in.vh.scn.measurement_unit[1];
    st->measurement_unit[2]=in.vh.scn.measurement_unit[2];
    st->measurement_unit_valid=1;
  }
  else st->measurement_unit_valid=0;

  if (in.vh.scn.num_of_bits >= 8) {
    st->scene_file=(Char30 *)malloc(sizeof(Char30)*vols);
    for(j=0;j<vols;j++) {
      strncpy((char *)st->scene_file[j],in.filename, sizeof(Char30));
      st->scene_file[j][sizeof(Char30)-1]=0;
    }
    st->scene_file_valid=1;
  }
  else if (in.vh.gen.filename1_valid) {
    st->scene_file=(Char30 *)malloc(sizeof(Char30)*vols);
    for(j=0;j<vols;j++) {
      strncpy((char *)st->scene_file[j],in.vh.gen.filename1, sizeof(Char30));
      st->scene_file[j][sizeof(Char30)-1]=0;
    }
    st->scene_file_valid=1;
  }
  else {
    st->scene_file=(Char30 *)malloc(sizeof(Char30)*vols);
    for(j=0;j<vols;j++) {
      strncpy((char *)st->scene_file[j],in.filename, sizeof(Char30));
      st->scene_file[j][sizeof(Char30)-1]=0;
    }
    st->scene_file_valid=1;
  }

  if (shelltype == 2)
    get_min_max_SHELL2_values(vols);
  else
    get_min_max_values(vols);
  

  st->num_of_TSE=(unsigned int *)malloc(vols*sizeof(int));
  st->num_of_NTSE=(unsigned int *)malloc(vols*sizeof(int));

  for(i=0;i<vols;i++) {
    st->num_of_TSE[i]=med[i].num_TSE;
    st->num_of_TSE_valid=1;
    
    st->num_of_NTSE[i]=1+(med[i].max_z-med[i].min_z+1)*(med[i].max_y-med[i].min_y+2);
    st->num_of_NTSE_valid=1;
  

  }
  if (shelltype == 2)
    st->num_of_components_in_TSE=32; /* ncode,y1,v1,v2,v3,n1,n2,n3,v1,v2,v3,
      n1,n2,n3,v1,v2,v3,n1,n2,n3,v1,v2,v3,n1,n2,n3,v1,v2,v3,n1,n2,n3 */
  else
    st->num_of_components_in_TSE=9; /* ncode,y1,tt,n1,n2,n3,gm,op,og */
  st->num_of_components_in_TSE_valid=1;
  
  st->num_of_components_in_NTSE=1;
  st->num_of_components_in_NTSE_valid=1;
  
  if (shelltype == 2) {
   st->smallest_value=(float *)malloc(sizeof(float)*32*vols);
   st->largest_value=(float *)malloc(sizeof(float)*32*vols);
   for(i=0;i<vols;i++) { 
    st->smallest_value[i*32+0]=1;   /* ncode */
    st->largest_value[i*32+0]=254;
    st->smallest_value[i*32+1]=med[i].min_x; /* y1 */
    st->largest_value[i*32+1]=med[i].max_x;
    for (j=0; j<5; j++) {
     st->smallest_value[i*32+j*6+2]= -3.0; st->largest_value[i*32+j*6+2]=3.0;
     st->smallest_value[i*32+j*6+3]= -3.0; st->largest_value[i*32+j*6+3]=3.0;
     st->smallest_value[i*32+j*6+4]= -3.0; st->largest_value[i*32+j*6+4]=3.0;
     st->smallest_value[i*32+j*6+5]=0.0; st->largest_value[i*32+j*6+5]=6.0;
     st->smallest_value[i*32+j*6+6]=0.0; st->largest_value[i*32+j*6+6]=63.0;
     st->smallest_value[i*32+j*6+7]=0.0; st->largest_value[i*32+j*6+7]=63.0;
    }
   }
   st->num_of_integers_in_TSE=32;
  }
  else {
   st->smallest_value=(float *)malloc(sizeof(float)*9*vols);
   st->largest_value=(float *)malloc(sizeof(float)*9*vols);
   for(i=0;i<vols;i++) { 
    st->smallest_value[i*9+0]=0;   /* ncode */
    st->largest_value[i*9+0]=1;
    st->smallest_value[i*9+1]=med[i].min_x; /* y1 */
    st->largest_value[i*9+1]=med[i].max_x;
    st->smallest_value[i*9+2]=st->largest_value[i*9+2]=0.0;
    st->smallest_value[i*9+3]=0.0; st->largest_value[i*9+3]=7.0; /* normals */
    st->smallest_value[i*9+4]=0.0; st->largest_value[i*9+4]=15.0;
    st->smallest_value[i*9+5]=0.0; st->largest_value[i*9+5]=15.0;
    st->smallest_value[i*9+6]=st->largest_value[i*9+6]=0.0;
    st->smallest_value[i*9+7]=st->largest_value[i*9+7]=0.0;
    st->smallest_value[i*9+8]=st->largest_value[i*9+8]=0.0;
   }
   st->num_of_integers_in_TSE=9;
  }
  st->smallest_value_valid=1;
  st->largest_value_valid=1;
  st->num_of_integers_in_TSE_valid=1;
  
  if (shelltype == 2)
  {
   st->signed_bits_in_TSE=(short *)malloc(sizeof(short)*32);
   st->signed_bits_in_TSE[0]=0;
   st->signed_bits_in_TSE[1]=0;
   for(i=0;i<5; i++) {
    st->signed_bits_in_TSE[i*6+2]=1;
    st->signed_bits_in_TSE[i*6+3]=1;
    st->signed_bits_in_TSE[i*6+4]=1;
    st->signed_bits_in_TSE[i*6+5]=0;
    st->signed_bits_in_TSE[i*6+6]=0;
    st->signed_bits_in_TSE[i*6+7]=0;
   }

   st->num_of_bits_in_TSE= 8;

   st->bit_fields_in_TSE=(short *)malloc(sizeof(short)*64);
   st->bit_fields_in_TSE[0]=0;  st->bit_fields_in_TSE[1]=7; /* ncode */
   st->bit_fields_in_TSE[2]=8;  st->bit_fields_in_TSE[3]=23; /* y1 */
   for(i=0;i<5; i++) {
    st->bit_fields_in_TSE[i*12+4] = i*24+24;
	  st->bit_fields_in_TSE[i*12+5] = i*24+26; /* yi detail */
    st->bit_fields_in_TSE[i*12+6] = i*24+27;
      st->bit_fields_in_TSE[i*12+7] = i*24+29; /* yi detail */
    st->bit_fields_in_TSE[i*12+8] = i*24+30;
      st->bit_fields_in_TSE[i*12+9] = i*24+32; /* yi detail */
    st->bit_fields_in_TSE[i*12+10] = i*24+33;
      st->bit_fields_in_TSE[i*12+11] = i*24+35; /* n1 */
    st->bit_fields_in_TSE[i*12+12] = i*24+36;
      st->bit_fields_in_TSE[i*12+13] = i*24+41; /* n2 */
    st->bit_fields_in_TSE[i*12+14] = i*24+42;
      st->bit_fields_in_TSE[i*12+15] = i*24+47; /* n3 */
   }
  }
  else
  {
   st->signed_bits_in_TSE=(short *)malloc(sizeof(short)*9);
   st->signed_bits_in_TSE[0]=0;
   st->signed_bits_in_TSE[1]=0;
   st->signed_bits_in_TSE[2]=0;
   st->signed_bits_in_TSE[3]=1;
   st->signed_bits_in_TSE[4]=1;
   st->signed_bits_in_TSE[5]=1;
   st->signed_bits_in_TSE[6]=st->signed_bits_in_TSE[7]=st->signed_bits_in_TSE[8]=0;
  
   st->num_of_bits_in_TSE= 32;

   st->bit_fields_in_TSE=(short *)malloc(sizeof(short)*18);
   st->bit_fields_in_TSE[0]=0;  st->bit_fields_in_TSE[1]=0; /* ncode */
   st->bit_fields_in_TSE[2]=1;  st->bit_fields_in_TSE[3]=15; /* y1 */
   st->bit_fields_in_TSE[4]=0;  st->bit_fields_in_TSE[5]=-1; /* tt */
   st->bit_fields_in_TSE[6]=21;  st->bit_fields_in_TSE[7]=23; 
   st->bit_fields_in_TSE[8]=24;  st->bit_fields_in_TSE[9]=27;
   st->bit_fields_in_TSE[10]=28;  st->bit_fields_in_TSE[11]=31;
   st->bit_fields_in_TSE[12]=0;  st->bit_fields_in_TSE[13]=-1; 
   st->bit_fields_in_TSE[14]=0;  st->bit_fields_in_TSE[15]=-1;
   st->bit_fields_in_TSE[16]=0;  st->bit_fields_in_TSE[17]=-1;
  }
  st->signed_bits_in_TSE_valid=1;
  st->num_of_bits_in_TSE_valid=1;
  st->bit_fields_in_TSE_valid=1;
 
  st->num_of_integers_in_NTSE=1;
  st->num_of_integers_in_NTSE_valid=1;
 
  st->signed_bits_in_NTSE=(short *)malloc(sizeof(short));
  st->signed_bits_in_NTSE[0]=0;
  st->signed_bits_in_NTSE_valid=1;
  
  st->num_of_bits_in_NTSE=16;
  st->num_of_bits_in_NTSE_valid=1;
  
  st->bit_fields_in_NTSE=(short *)malloc(sizeof(short)*2);
  st->bit_fields_in_NTSE[0]=0;
  st->bit_fields_in_NTSE[1]=15;
  st->bit_fields_in_NTSE_valid=1;

  st->num_of_samples=(short *)malloc(sizeof(short));
  st->num_of_samples[0]= shelltype==2? (interp.zsize>>1)+1: interp.zsize;
  st->num_of_samples_valid=1;
 
  st->xysize[0]= shelltype==2? 2*med[0].Px: med[0].Px;
  st->xysize[1]= shelltype==2? 2*med[0].Py: med[0].Py;
  st->xysize_valid=1;
 
  st->loc_of_samples=(float *)malloc(sizeof(float)*st->num_of_samples[0]);
  st->loc_of_samples[0]=0.0;
  for(i=1;i<st->num_of_samples[0];i++)
    st->loc_of_samples[i]= shelltype==2? 2*i*med[0].Pz: i*med[0].Pz;
  st->loc_of_samples_valid=1;
  
  if (!param_on) {
    if (vols==1) {
      st->num_of_elements=1;
      st->num_of_elements_valid=1;
      
      st->description_of_element=(short *)malloc(sizeof(short));
      st->description_of_element[0]=1; /* type == object label */
      st->description_of_element_valid=1;
      st->parameter_vectors=(float *)malloc(sizeof(float));
      st->parameter_vectors[0]= 0.0;
      st->parameter_vectors_valid=1;
      
    }
    else {
      st->num_of_elements=2;
      st->num_of_elements_valid=1;
      
      st->description_of_element=(short *)malloc(2*sizeof(short));
      st->description_of_element[0]=0; /* type == time label */
      st->description_of_element[1]=1; /* type == object label */    
      st->description_of_element_valid=1;
      st->parameter_vectors=(float *)malloc(sizeof(float)*2*vols);
      for(i=0;i<vols;i++) {
	st->parameter_vectors[2*i]= (float)i;
	st->parameter_vectors[2*i+1]= 0.0;
      }
      st->parameter_vectors_valid=1;
    
    }
  }
  else {
    st->num_of_elements_valid=1;
    st->description_of_element_valid=1;
    st->parameter_vectors_valid=1;
    pfp=fopen(param_file,"rb");
    if (pfp==NULL) {
      printf("Could not read parameters from file\n");
      st->num_of_elements_valid=0;
      st->description_of_element_valid=0;
      st->parameter_vectors_valid=0;
    }
    else if (fscanf(pfp,"%d",&elem)!=1) {
      printf("Could not read parameters from file\n");
      st->num_of_elements_valid=0;
      st->description_of_element_valid=0;
      st->parameter_vectors_valid=0;
    }
    else {
      st->num_of_elements=elem;
      st->description_of_element=(short *)calloc(st->num_of_elements,
						 sizeof(short));
      st->parameter_vectors=(float *)calloc(st->num_of_elements*vols,
					    sizeof(float));
      for(i=0;i<st->num_of_elements;i++) {
	if (fscanf(pfp,"%d",&elem)!=1) {
	  printf("Could not read parameters from file\n");
	  st->num_of_elements_valid=0;
	  st->description_of_element_valid=0;
	  st->parameter_vectors_valid=0;
	  break;
	}
	else
	  st->description_of_element[i]=elem;
      }
      if (st->parameter_vectors_valid==0 || fscanf(pfp,"%d",&temp_vols)!=1) {
	printf("Could not read parameters from file\n");
	st->num_of_elements_valid=0;
	st->description_of_element_valid=0;
	st->parameter_vectors_valid=0;
      }
      else {
	if (temp_vols>vols) temp_vols=vols;
	for(i=0;i<st->num_of_elements*temp_vols;i++)
	  if (fscanf(pfp,"%f",&st->parameter_vectors[i])!=1) {
	    printf("Could not read parameters from file\n");
	    st->num_of_elements_valid=0;
	    st->description_of_element_valid=0;
	    st->parameter_vectors_valid=0;
	    break;
	}
      }
    }
  }
  
  
  st->min_max_coordinates=(float *)malloc(sizeof(float)*6*vols);
  if (shelltype == 1)
    for(i=0;i<vols;i++) {
      st->min_max_coordinates[i*6]=med[i].Px*med[i].min_x;
      st->min_max_coordinates[i*6+1]=med[i].Py*med[i].min_y;
      st->min_max_coordinates[i*6+2]=med[i].Pz*med[i].min_z;
      st->min_max_coordinates[i*6+3]=med[i].Px*med[i].max_x;
      st->min_max_coordinates[i*6+4]=med[i].Py*med[i].max_y;
      st->min_max_coordinates[i*6+5]=med[i].Pz*med[i].max_z;
    }
  else
    for(i=0;i<vols;i++) {
      st->min_max_coordinates[i*6]=2*med[i].Px*med[i].min_x;
      st->min_max_coordinates[i*6+1]=2*med[i].Py*med[i].min_y;
      st->min_max_coordinates[i*6+2]=2*med[i].Pz*med[i].min_z;
      st->min_max_coordinates[i*6+3]=2*med[i].Px*med[i].max_x;
      st->min_max_coordinates[i*6+4]=2*med[i].Py*med[i].max_y;
      st->min_max_coordinates[i*6+5]=2*med[i].Pz*med[i].max_z;
    }
  st->min_max_coordinates_valid=1;

  if (shelltype == 1)
  {
    st->volume=(float *)malloc(vols*sizeof(float));
    for(i=0;i<vols;i++) {
     st->volume[i]= med[i].volume*med[i].Px*med[i].Py*med[i].Pz*4;
    }
    st->volume_valid=1;
  }
  else
  {
    st->volume = str_volume;
	st->surface_area = str_surface_area;
	st->surface_area_valid = st->volume_valid = 1;
  }

}



/************************************************************************
 *
 *      FUNCTION        : get_min_max_values()
 *
 *      DESCRIPTION     :  Get the bounding box of the tracked surface 
 *                         in y and z drections.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : vols - number of volumes in ht einput scene.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : med structure should be Initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *
 ************************************************************************/
void get_min_max_values(int vols)
{
  short *ptr;
  int i,j,v;

  for(v=0;v<vols;v++) {
    for(ptr=med[v].NTSE,i=0;i<med[v].slices;i++)
      for(j=0;j<med[v].height;j++,ptr++) {
	if (*ptr) {
	  if (i > med[v].max_z) med[v].max_z=i;
	  if (j > med[v].max_y) med[v].max_y=j;
	  if (i < med[v].min_z) med[v].min_z=i;
	  if (j < med[v].min_y) med[v].min_y=j;
	}
      }
  }
}


/************************************************************************
 *
 *      FUNCTION        : WriteTSE_SHELL1()
 *
 *      DESCRIPTION     : Given a volume and number of chunks the input 
 *                        data has been devided upto and the intensity
 *                        range this generates the faces of the surface
 *                        and writes to a temporary file.
 *
 *      RETURN VALUE    : None.
 * 
 *      PARAMETERS      : 4 parameters.
 *                 vol   : current volume being tracked.
 *                 chunks: number of chunks the scene is devided into.
 *                 min,max: intensity range if interest.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : memory allocation fault, or read/write
 *                        error.
 *
 *      RELATED FUNCS   : ReadChunk,Create_Bin_Slice,Calculate_Norm
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
void WriteTSE_SHELL1(int vol, int chunks, float min, float max)
{
  int CHUNK,i,j,k,num;
  short *ntse,*nt;
  TSE_type *TSE;
  unsigned char *sl1,*sl2,*sl3,*sl4,*temp_sl;

  TSE=(TSE_type *)calloc(sizeof(TSE_type),med[vol].width);
  if (TSE==NULL) {
    printf("Could not allocate temp space for TSE's\n");
    exit(-1);
  }
  CHUNK=0;
  ReadChunk(vol,CHUNK);
  Create_Bin_Slice(vol,CHUNK,med_sl2,-1,min,max);
  for(ntse=med[vol].NTSE,i=1;i<interp.zsize;i+=2) {
    while ( i > interp.ChunkOffsets[CHUNK]+interp.ChunkSlices-1) {
      CHUNK++;
      ReadChunk(vol,CHUNK);
    }
    temp_sl=med_sl1; med_sl1=med_sl2; med_sl2=temp_sl;
    Create_Bin_Slice(vol,CHUNK,med_sl2,i,min,max);
    /* Do Z_faces */
    for(nt=ntse+1,sl1=med_sl1,sl2=med_sl2,j=1;
	j<med[vol].height;j+=2,nt+=2) {
      for(k=1;k<med[vol].width;k+=2,sl1++,sl2++) {
	if ( (*sl1) ^ (*sl2) ) {
	  TSE[*nt][0]=k;
	  Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i-1,j,k, FALSE);
	  if (*sl1) TSE[*nt][0] |= 0x8000;
	  (*nt)++;
	}
      } 
      if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
	printf("Could not write TSE\n");
	exit(-1);
      }      
      med[vol].num_TSE += *nt;
    }
    ntse += med[vol].height;
    /* Do 1st Y-row */
    for(nt=ntse,sl1=med_sl2,k=1;k<med[vol].width;k+=2,sl1++) {
      if (*sl1) {
	TSE[*nt][0]=k;
	Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,0,k, FALSE);
	(*nt)++;
      }
    }
    if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
      printf("Could not write TSE\n");
      exit(-1);
    }      
    med[vol].num_TSE += *nt;
    
    for(sl1=med_sl2,sl2=med_sl2+(med[vol].width>>1),j=1;
	j<med[vol].height-2;j+=2) {
      /*Do X-rows */
       nt++;
      /* Do 1st X-col */
      if (*sl1) {
	TSE[*nt][0]=0;
	if (med[vol].min_x > 0) med[vol].min_x=0;
	if (med[vol].max_x < 0) med[vol].max_x=0;	
	Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j,0, FALSE);
	(*nt)++;
      }
      /* Do X-row */
      for(sl3=sl1,sl4=sl3+1,k=2;k<med[vol].width-1;sl3++,sl4++,k+=2)
	if ( (*sl3) ^ (*sl4) ) {
	  TSE[*nt][0]=k;
	  if (med[vol].min_x > k) med[vol].min_x=k;
	  if (med[vol].max_x < k) med[vol].max_x=k;	
	  Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j,k, FALSE);
	  if (*sl3) {
	    TSE[*nt][0] |= 0x8000;
	    med[vol].volume += k;
	  }
	  else
	    med[vol].volume -= k;
	  (*nt)++;
	}
      /* Do Last X-col */
      if (*sl3) {
	TSE[*nt][0]=med[vol].width-1;
	if (med[vol].min_x > med[vol].width-1) med[vol].min_x=med[vol].width-1;
	if (med[vol].max_x < med[vol].width-1) med[vol].max_x=med[vol].width-1;	
	Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j,med[vol].width-1, FALSE);
	TSE[*nt][0] |= 0x8000;
	med[vol].volume += med[vol].width-1;
	(*nt)++;
      }
      
      if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
	printf("Could not write TSE\n");
	exit(-1);
      }      
      med[vol].num_TSE += *nt;
      
      /* Do Y-row */
      nt++;
      for(k=1;k<med[vol].width;k+=2,sl1++,sl2++) {
	if ((*sl1)^(*sl2) ) {
	  TSE[*nt][0]=k;
	  Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j+1,k, FALSE);
	  if (*sl1) TSE[*nt][0] |= 0x8000;
	  (*nt)++;
	}
      }
      if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
	printf("Could not write TSE\n");
	exit(-1);
      }
      med[vol].num_TSE += *nt;
      
    }
    nt++;
    /* Do last X-row */
    /* Do 1st X-col */
    if (*sl1) {
      TSE[*nt][0]=0;
      if (med[vol].min_x > 0) med[vol].min_x=0;
      if (med[vol].max_x < 0) med[vol].max_x=0;	
      Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j,0, FALSE);
      (*nt)++;
    }
    /* Do X-row */
    for(sl3=sl1,sl4=sl3+1,k=2;k<med[vol].width-1;sl3++,sl4++,k+=2)
      if ( (*sl3) ^ (*sl4) ) {
	TSE[*nt][0]=k;
	if (med[vol].min_x > k) med[vol].min_x=k;
	if (med[vol].max_x < k) med[vol].max_x=k;	
	Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j,k, FALSE);
	if (*sl3) {
	  TSE[*nt][0] |= 0x8000;
	  med[vol].volume += k;
	}
	else
	  med[vol].volume -= k;
	(*nt)++;
      }
    /* Do Last X-col */
    if (*sl3) {
      TSE[*nt][0]=med[vol].width-1;
      if (med[vol].min_x > med[vol].width-1) med[vol].min_x=med[vol].width-1;
      if (med[vol].max_x < med[vol].width-1) med[vol].max_x=med[vol].width-1;	
      Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j,med[vol].width-1, FALSE);
      TSE[*nt][0] |= 0x8000;
      med[vol].volume += med[vol].width-1;
      (*nt)++;
    }
    
    if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
      printf("Could not write TSE\n");
      exit(-1);
    }      
    med[vol].num_TSE += *nt;
    
    /* Do Y-row */
    nt++;
    for(k=1;k<med[vol].width;k+=2,sl1++) {
      if (*sl1) {
	TSE[*nt][0]=k;
	Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i,j+1,k, FALSE);
	TSE[*nt][0] |= 0x8000;
	(*nt)++;
      }
    }
    if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
      printf("Could not write TSE\n");
      exit(-1);
    }
    med[vol].num_TSE += *nt;
      
    ntse += med[vol].height;
  }

  for(nt=ntse+1,sl1=med_sl1,sl2=med_sl2,j=1;
      j<med[vol].height;j+=2,nt+=2) {
    for(k=1;k<med[vol].width;k+=2,sl1++,sl2++) {
      if (*sl2)  {
	TSE[*nt][0]=k;
	Calculate_Norm(&TSE[*nt][1],vol,CHUNK,i-1,j,k, FALSE);
        TSE[*nt][0] |= 0x8000;
	(*nt)++;
      }
    }
    if (*nt && VWriteData((char *)TSE,sizeof(TSE_type)/2,*nt*2,med_fp,&num)) {
      printf("Could not write TSE\n");
      exit(-1);
    }      
    med[vol].num_TSE += *nt;
  }
  
  free(TSE);
}

/************************************************************************
 *
 *      FUNCTION        : WriteTSE_SHELL2()
 *
 *      DESCRIPTION     : Given a volume and number of chunks the input 
 *                        data have been divided upto and the intensity
 *                        range, this generates the faces of the surface
 *                        and writes to a temporary file.
 *
 *      RETURN VALUE    : None.
 * 
 *      PARAMETERS      : 4 parameters.
 *                 vol   : current volume being tracked.
 *                 chunks: number of chunks the scene is divided into.
 *                 min,max: intensity range if interest.
 *
 *      SIDE EFFECTS    : med_fp, str_volume, str_surface_area,
 *
 *      ENTRY CONDITION : med, med_fp,
 *
 *      EXIT CONDITIONS : memory allocation fault, or read/write
 *                        error.
 *
 *      RELATED FUNCS   : ReadChunk,Create_Bin_Slice,Calculate_Norm
 *
 *      History         : Created: 7/8/03 by Dewey Odhner.
 *                        Modified: 7/14/03 equivalent triangles identified
 *                           by Dewey Odhner.
 *                        Modified: 8/27/03 triangle vertex detail corrected
 *                           by Dewey Odhner.
 *                        Modified: 9/25/03 surface area and volume computed
 *                           by Dewey Odhner.
 *
 ************************************************************************/
void WriteTSE_SHELL2(int vol, int chunks, float min, float max)
{

/*----------------------------------------------------------------------*/
/*
File  : marchingCubesGL.cpp
Author: George J. Grevera
Date  : 6/16/97
Desc. : Based on "Marching Cubes: A High Resolution 3D Surface Construction
        Algorithm," W.E. Lorensen, and H.E. Cline, Computer Graphics, 21(4),
        7/87, pp. 163-169.

        coordinate system:

             z
            +
           /
          /
         /
         ----- +x
        |
        |
        |
        +
        y

        vertices:

            v8-------v7
           /|        /|
          / |       / |
         /  |      /  |
        v4-------v3   |
        |   |     |   |
        |   v5----|--v6
        |  /      |  /
        | /       | /
        |/        |/
        v1-------v2

        edges:

            .----e7---.
           /|        /|
         e11|      e12|
         /  e8     /  e6
        .----e3---.   |
        |   |     |   |
        |   .---e5|---.
        e4 /      e2 /
        | e9      |e10
        |/        |/
        .----e1---.

//----------------------------------------------------------------------*
// each row corresponds to a configuration of a marching cube.  each column
// entry corresponds to an edge and will become the vertex of a triangle
// (once it has been interpolated along the edge).  then triples of these
// form the triangle. */

static const int triangle_edges[189][3] = {
{ 1,  2,  6},
{ 1,  2,  7},
{ 1,  2,  8},
{ 1,  2,  9},
{ 1,  2, 10},
{ 1,  2, 11}, /* 5 */
{ 1,  2, 12},
{ 1,  3,  5},
{ 1,  3,  6},
{ 1,  3,  7},
{ 1,  3,  8}, /* 10 */
{ 1,  3,  9},
{ 1,  3, 10},
{ 1,  3, 11},
{ 1,  3, 12},
{ 1,  4,  5}, /* 15 */
{ 1,  4,  6},
{ 1,  4,  7},
{ 1,  4,  8},
{ 1,  4,  9},
{ 1,  4, 10}, /* 20 */
{ 1,  4, 11},
{ 1,  4, 12},
{ 1,  5,  6},
{ 1,  5,  7},
{ 1,  5,  8}, /* 25 */
{ 1,  5, 11},
{ 1,  5, 12},
{ 1,  6,  7},
{ 1,  6,  8},
{ 1,  6,  9}, /* 30 */
{ 1,  6, 10},
{ 1,  6, 11},
{ 1,  6, 12},
{ 1,  7,  8},
{ 1,  7,  9}, /* 35 */
{ 1,  7, 10},
{ 1,  7, 11},
{ 1,  7, 12},
{ 1,  8,  9},
{ 1,  8, 10}, /* 40 */
{ 1,  8, 11},
{ 1,  8, 12},
{ 1,  9, 11},
{ 1,  9, 12},
{ 1, 10, 11}, /* 45 */
{ 1, 10, 12},
{ 1, 11, 12},
{ 2,  3,  5},
{ 2,  3,  7},
{ 2,  3,  8}, /* 50 */
{ 2,  3,  9},
{ 2,  3, 10},
{ 2,  3, 11},
{ 2,  3, 12},
{ 2,  4,  5}, /* 55 */
{ 2,  4,  6},
{ 2,  4,  7},
{ 2,  4,  8},
{ 2,  4,  9},
{ 2,  4, 10}, /* 60 */
{ 2,  4, 11},
{ 2,  4, 12},
{ 2,  5,  6},
{ 2,  5,  7},
{ 2,  5,  8}, /* 65 */
{ 2,  5,  9},
{ 2,  5, 10},
{ 2,  5, 11},
{ 2,  5, 12},
{ 2,  6,  7}, /* 70 */
{ 2,  6,  8},
{ 2,  6,  9},
{ 2,  6, 11},
{ 2,  7,  8},
{ 2,  7, 10}, /* 75 */
{ 2,  7, 11},
{ 2,  7, 12},
{ 2,  8,  9},
{ 2,  8, 10},
{ 2,  8, 11}, /* 80 */
{ 2,  8, 12},
{ 2,  9, 10},
{ 2,  9, 11},
{ 2,  9, 12},
{ 2, 10, 11}, /* 85 */
{ 2, 11, 12},
{ 3,  4,  5},
{ 3,  4,  6},
{ 3,  4,  7},
{ 3,  4,  8}, /* 90 */
{ 3,  4,  9},
{ 3,  4, 10},
{ 3,  4, 11},
{ 3,  4, 12},
{ 3,  5,  6}, /* 95 */
{ 3,  5,  7},
{ 3,  5,  8},
{ 3,  5,  9},
{ 3,  5, 10},
{ 3,  5, 11}, /* 100 */
{ 3,  5, 12},
{ 3,  6,  7},
{ 3,  6,  8},
{ 3,  6,  9},
{ 3,  6, 10}, /* 105 */
{ 3,  6, 11},
{ 3,  6, 12},
{ 3,  7,  8},
{ 3,  7,  9},
{ 3,  7, 10}, /* 110 */
{ 3,  8,  9},
{ 3,  8, 10},
{ 3,  8, 11},
{ 3,  8, 12},
{ 3,  9, 10}, /* 115 */
{ 3,  9, 11},
{ 3,  9, 12},
{ 3, 10, 11},
{ 3, 10, 12},
{ 4,  5,  6}, /* 120 */
{ 4,  5,  7},
{ 4,  5,  8},
{ 4,  5,  9},
{ 4,  5, 10},
{ 4,  5, 11}, /* 125 */
{ 4,  5, 12},
{ 4,  6,  7},
{ 4,  6,  8},
{ 4,  6,  9},
{ 4,  6, 10}, /* 130 */
{ 4,  6, 11},
{ 4,  6, 12},
{ 4,  7,  8},
{ 4,  7,  9},
{ 4,  7, 10}, /* 135 */
{ 4,  7, 11},
{ 4,  7, 12},
{ 4,  8, 10},
{ 4,  8, 12},
{ 4,  9, 12}, /* 140 */
{ 4, 10, 11},
{ 4, 10, 12},
{ 4, 11, 12},
{ 5,  6,  9},
{ 5,  6, 10}, /* 145 */
{ 5,  6, 11},
{ 5,  6, 12},
{ 5,  7,  9},
{ 5,  7, 10},
{ 5,  7, 12}, /* 150 */
{ 5,  8,  9},
{ 5,  8, 10},
{ 5,  8, 11},
{ 5,  8, 12},
{ 5,  9, 11}, /* 155 */
{ 5,  9, 12},
{ 5, 10, 11},
{ 5, 10, 12},
{ 5, 11, 12},
{ 6,  7,  9}, /* 160 */
{ 6,  7, 10},
{ 6,  7, 11},
{ 6,  7, 12},
{ 6,  8, 10},
{ 6,  8, 11}, /* 165 */
{ 6,  8, 12},
{ 6,  9, 10},
{ 6,  9, 11},
{ 6,  9, 12},
{ 6, 10, 11}, /* 170 */
{ 6, 11, 12},
{ 7,  8,  9},
{ 7,  8, 10},
{ 7,  8, 11},
{ 7,  8, 12}, /* 175 */
{ 7,  9, 10},
{ 7,  9, 11},
{ 7,  9, 12},
{ 7, 10, 11},
{ 8,  9, 10}, /* 180 */
{ 8,  9, 12},
{ 8, 10, 11},
{ 8, 10, 12},
{ 8, 11, 12},
{ 9, 10, 11}, /* 185 */
{ 9, 10, 12},
{ 9, 11, 12},
{10, 11, 12}};

static const int triangle_table[256][6]={
{-1}, /* 0 */
{19, -1}, /* 1 */
{4, -1}, /* 2 */
{82, 59, -1}, /* 3 */
{54, -1}, /* 4 */
{19, 54, -1}, /* 5 */
{119, 12, -1}, /* 6 */
{91, 117, 186, -1}, /* 7 */
{93, -1}, /* 8 */
{43, 13, -1}, /* 9 */
{4, 93, -1}, /* 10 */
{53, 85, 185, -1}, /* 11 */
{62, 143, -1}, /* 12 */
{6, 44, 187, -1}, /* 13 */
{20, 141, 188, -1}, /* 14 */
{186, 187, -1}, /* 15 */
{151, -1}, /* 16 */
{15, 122, -1}, /* 17 */
{151, 4, -1}, /* 18 */
{67, 65, 58, -1}, /* 19 */
{151, 54, -1}, /* 20 */
{122, 15, 54, -1}, /* 21 */
{119, 12, 151, -1}, /* 22 */
{122, 126, 158, 94, -1}, /* 23 */
{151, 93, -1}, /* 24 */
{153, 100, 7, -1}, /* 25 */
{4, 151, 93, -1}, /* 26 */
{53, 85, 182, 152, -1}, /* 27 */
{62, 143, 151, -1}, /* 28 */
{25, 42, 184, 6, -1}, /* 29 */
{151, 45, 188, 21, -1}, /* 30 */
{153, 157, 188, -1}, /* 31 */
{145, -1}, /* 32 */
{145, 19, -1}, /* 33 */
{0, 23, -1}, /* 34 */
{144, 129, 56, -1}, /* 35 */
{145, 54, -1}, /* 36 */
{145, 19, 54, -1}, /* 37 */
{107, 95, 7, -1}, /* 38 */
{91, 98, 101, 147, -1}, /* 39 */
{145, 93, -1}, /* 40 */
{13, 43, 145, -1}, /* 41 */
{23, 0, 93, -1}, /* 42 */
{155, 68, 63, 53, -1}, /* 43 */
{143, 62, 145, -1}, /* 44 */
{145, 84, 187, 3, -1}, /* 45 */
{171, 32, 23, 21, -1}, /* 46 */
{144, 169, 187, -1}, /* 47 */
{180, 164, -1}, /* 48 */
{20, 130, 128, -1}, /* 49 */
{39, 2, 71, -1}, /* 50 */
{71, 58, -1}, /* 51 */
{180, 164, 54, -1}, /* 52 */
{54, 31, 16, 128, -1}, /* 53 */
{166, 42, 14, 39, -1}, /* 54 */
{107, 88, 128, -1}, /* 55 */
{164, 180, 93, -1}, /* 56 */
{12, 112, 164, 113, -1}, /* 57 */
{93, 3, 78, 71, -1}, /* 58 */
{53, 80, 71, -1}, /* 59 */
{86, 61, 164, 180, -1}, /* 60 */
{29, 31, 41, 6, 47, -1}, /* 61 */
{47, 21, 33, 39, 29, -1}, /* 62 */
{165, 171, -1}, /* 63 */
{163, -1}, /* 64 */
{163, 19, -1}, /* 65 */
{4, 163, -1}, /* 66 */
{59, 82, 163, -1}, /* 67 */
{70, 49, -1}, /* 68 */
{70, 49, 19, -1}, /* 69 */
{161, 36, 9, -1}, /* 70 */
{102, 104, 167, 91, -1}, /* 71 */
{163, 93, -1}, /* 72 */
{43, 13, 163, -1}, /* 73 */
{163, 4, 93, -1}, /* 74 */
{163, 52, 118, 185, -1}, /* 75 */
{136, 127, 56, -1}, /* 76 */
{0, 32, 43, 162, -1}, /* 77 */
{136, 127, 16, 31, -1}, /* 78 */
{161, 179, 185, -1}, /* 79 */
{163, 151, -1}, /* 80 */
{15, 122, 163, -1}, /* 81 */
{151, 4, 163, -1}, /* 82 */
{163, 152, 79, 58, -1}, /* 83 */
{49, 70, 151, -1}, /* 84 */
{25, 18, 70, 49, -1}, /* 85 */
{151, 31, 28, 9, -1}, /* 86 */
{138, 152, 92, 161, 110, -1}, /* 87 */
{151, 163, 93, -1}, /* 88 */
{163, 97, 7, 113, -1}, /* 89 */
{93, 4, 151, 163, -1}, /* 90 */
{52, 118, 157, 153, 163, -1}, /* 91 */
{151, 162, 131, 56, -1}, /* 92 */
{73, 162, 5, 153, 26, -1}, /* 93 */
{162, 131, 130, 20, 151, -1}, /* 94 */
{161, 179, 152, 182, -1}, /* 95 */
{150, 158, -1}, /* 96 */
{150, 158, 19, -1}, /* 97 */
{6, 38, 24, -1}, /* 98 */
{150, 126, 62, 123, -1}, /* 99 */
{67, 48, 96, -1}, /* 100 */
{19, 99, 96, 52, -1}, /* 101 */
{24, 9, -1}, /* 102 */
{91, 98, 96, -1}, /* 103 */
{158, 150, 93, -1}, /* 104 */
{11, 116, 158, 150, -1}, /* 105 */
{93, 77, 1, 24, -1}, /* 106 */
{64, 77, 66, 53, 83, -1}, /* 107 */
{60, 135, 149, 136, -1}, /* 108 */
{83, 3, 76, 67, 64, -1}, /* 109 */
{136, 17, 24, -1}, /* 110 */
{148, 177, -1}, /* 111 */
{175, 181, 186, -1}, /* 112 */
{46, 42, 18, 175, -1}, /* 113 */
{6, 44, 178, 172, -1}, /* 114 */
{175, 81, 58, -1}, /* 115 */
{180, 112, 108, 52, -1}, /* 116 */
{110, 52, 173, 20, 138, -1}, /* 117 */
{39, 34, 9, -1}, /* 118 */
{108, 90, -1}, /* 119 */
{93, 172, 178, 186, -1}, /* 120 */
{10, 113, 40, 175, 183, -1}, /* 121 */
{172, 178, 44, 6, 93, -1}, /* 122 */
{53, 80, 77, 74, -1}, /* 123 */
{176, 172, 75, 136, 57, -1}, /* 124 */
{4, 174, -1}, /* 125 */
{136, 17, 172, 35, -1}, /* 126 */
{174, -1}, /* 127 */
{174, -1}, /* 128 */
{174, 19, -1}, /* 129 */
{4, 174, -1}, /* 130 */
{82, 59, 174, -1}, /* 131 */
{54, 174, -1}, /* 132 */
{19, 174, 54, -1}, /* 133 */
{12, 119, 174, -1}, /* 134 */
{174, 140, 186, 94, -1}, /* 135 */
{108, 90, -1}, /* 136 */
{39, 34, 9, -1}, /* 137 */
{108, 90, 4, -1}, /* 138 */
{180, 112, 108, 52, -1}, /* 139 */
{175, 81, 58, -1}, /* 140 */
{6, 44, 178, 172, -1}, /* 141 */
{46, 42, 18, 175, -1}, /* 142 */
{175, 181, 186, -1}, /* 143 */
{148, 177, -1}, /* 144 */
{136, 17, 24, -1}, /* 145 */
{177, 148, 4, -1}, /* 146 */
{60, 135, 149, 136, -1}, /* 147 */
{148, 177, 54, -1}, /* 148 */
{54, 37, 24, 21, -1}, /* 149 */
{46, 14, 148, 177, -1}, /* 150 */
{142, 94, 124, 136, 121, -1}, /* 151 */
{91, 98, 96, -1}, /* 152 */
{24, 9, -1}, /* 153 */
{4, 123, 87, 96, -1}, /* 154 */
{67, 48, 96, -1}, /* 155 */
{150, 126, 62, 123, -1}, /* 156 */
{6, 38, 24, -1}, /* 157 */
{121, 123, 137, 20, 142, -1}, /* 158 */
{150, 158, -1}, /* 159 */
{145, 174, -1}, /* 160 */
{145, 174, 19, -1}, /* 161 */
{0, 23, 174, -1}, /* 162 */
{174, 120, 56, 123, -1}, /* 163 */
{145, 54, 174, -1}, /* 164 */
{145, 174, 19, 54, -1}, /* 165 */
{174, 147, 101, 7, -1}, /* 166 */
{147, 101, 98, 91, 174, -1}, /* 167 */
{90, 108, 145, -1}, /* 168 */
{145, 35, 9, 172, -1}, /* 169 */
{133, 89, 23, 0, -1}, /* 170 */
{109, 172, 51, 144, 72, -1}, /* 171 */
{145, 74, 58, 77, -1}, /* 172 */
{3, 84, 181, 175, 145, -1}, /* 173 */
{27, 147, 22, 175, 139, -1}, /* 174 */
{175, 181, 147, 156, -1}, /* 175 */
{161, 179, 185, -1}, /* 176 */
{136, 127, 16, 31, -1}, /* 177 */
{0, 32, 43, 162, -1}, /* 178 */
{136, 127, 56, -1}, /* 179 */
{54, 170, 185, 162, -1}, /* 180 */
{31, 16, 127, 136, 54, -1}, /* 181 */
{168, 162, 30, 107, 8, -1}, /* 182 */
{107, 88, 162, 131, -1}, /* 183 */
{102, 104, 167, 91, -1}, /* 184 */
{161, 36, 9, -1}, /* 185 */
{72, 3, 160, 91, 109, -1}, /* 186 */
{70, 49, -1}, /* 187 */
{57, 77, 134, 161, 176, -1}, /* 188 */
{6, 38, 31, 28, -1}, /* 189 */
{163, 19, -1}, /* 190 */
{163, -1}, /* 191 */
{165, 171, -1}, /* 192 */
{171, 165, 19, -1}, /* 193 */
{165, 171, 4, -1}, /* 194 */
{166, 184, 82, 59, -1}, /* 195 */
{53, 80, 71, -1}, /* 196 */
{19, 50, 71, 113, -1}, /* 197 */
{12, 112, 164, 113, -1}, /* 198 */
{103, 113, 105, 91, 115, -1}, /* 199 */
{107, 88, 128, -1}, /* 200 */
{166, 42, 14, 39, -1}, /* 201 */
{4, 132, 128, 94, -1}, /* 202 */
{115, 52, 111, 107, 103, -1}, /* 203 */
{71, 58, -1}, /* 204 */
{39, 2, 71, -1}, /* 205 */
{20, 130, 128, -1}, /* 206 */
{180, 164, -1}, /* 207 */
{144, 169, 187, -1}, /* 208 */
{171, 32, 23, 21, -1}, /* 209 */
{4, 147, 156, 187, -1}, /* 210 */
{159, 147, 125, 67, 55, -1}, /* 211 */
{155, 68, 63, 53, -1}, /* 212 */
{26, 21, 146, 53, 73, -1}, /* 213 */
{8, 31, 106, 144, 168, -1}, /* 214 */
{145, 93, -1}, /* 215 */
{91, 98, 101, 147, -1}, /* 216 */
{107, 95, 7, -1}, /* 217 */
{123, 87, 95, 107, 4, -1}, /* 218 */
{67, 48, 147, 101, -1}, /* 219 */
{144, 129, 56, -1}, /* 220 */
{0, 23, -1}, /* 221 */
{144, 129, 31, 16, -1}, /* 222 */
{145, -1}, /* 223 */
{153, 157, 188, -1}, /* 224 */
{19, 152, 182, 188, -1}, /* 225 */
{25, 42, 184, 6, -1}, /* 226 */
{55, 123, 69, 153, 159, -1}, /* 227 */
{53, 85, 182, 152, -1}, /* 228 */
{152, 182, 85, 53, 19, -1}, /* 229 */
{153, 100, 7, -1}, /* 230 */
{153, 100, 123, 87, -1}, /* 231 */
{122, 126, 158, 94, -1}, /* 232 */
{183, 152, 114, 39, 10, -1}, /* 233 */
{139, 94, 154, 6, 27, -1}, /* 234 */
{151, 54, -1}, /* 235 */
{67, 65, 58, -1}, /* 236 */
{39, 2, 152, 79, -1}, /* 237 */
{15, 122, -1}, /* 238 */
{151, -1}, /* 239 */
{186, 187, -1}, /* 240 */
{20, 141, 188, -1}, /* 241 */
{6, 44, 187, -1}, /* 242 */
{62, 143, -1}, /* 243 */
{53, 85, 185, -1}, /* 244 */
{20, 141, 52, 118, -1}, /* 245 */
{43, 13, -1}, /* 246 */
{93, -1}, /* 247 */
{91, 117, 186, -1}, /* 248 */
{119, 12, -1}, /* 249 */
{91, 117, 3, 84, -1}, /* 250 */
{54, -1}, /* 251 */
{82, 59, -1}, /* 252 */
{4, -1}, /* 253 */
{19, -1}, /* 254 */
{-1} /* 255 */
};
/*----------------------------------------------------------------------*/



  int CHUNK,i,j,k,num, code, last_code, m, p, width, height, tse_bytes, d[3],
    neg_vol, total_triangles;
  short *ntse,*nt;
  unsigned char *TSE;
  float corner[9], corner_grad[9][3], w[3], a, b;
  unsigned short norm;
  static const int ev[13][2]={{0,0},
    {1,2}, {3,2}, {4,3}, {4,1},
	{5,6}, {7,6}, {8,7}, {8,5},
	{1,5}, {2,6}, {4,8}, {3,7}};
  double vertex_coord[3][3], Q, sidesquare[3], total_volume, total_area;
  static const int edge_coord[13][3]={
	{0, 0, 0},
	{2, 1, 0},
	{1, 2, 0},
	{2, 0, 0},
	{0, 2, 0},
	{2, 1, 1},
	{1, 2, 1},
	{2, 0, 1},
	{0, 2, 1},
	{0, 1, 2},
	{1, 1, 2},
	{0, 0, 2},
	{1, 0, 2}};


  width = (med[vol].width>>1)+1;
  height = (med[vol].height>>1)+1;
  TSE=(unsigned char *)malloc(18*width);
  if (TSE==NULL) {
    printf("Could not allocate temp space for TSE's\n");
    exit(-1);
  }
  total_volume = total_area = total_triangles = 0;
  CHUNK=0;
  ReadChunk(vol,CHUNK);
  for(ntse=med[vol].NTSE,i=0;i<interp.zsize;i+=2) {
    while (i > interp.ChunkOffsets[CHUNK]+interp.ChunkSlices-2) {
      CHUNK++;
      ReadChunk(vol,CHUNK);
    }
    for (nt=ntse,j=0; j<height; j++,nt++)
	{
      corner[2] = corner[3] = corner[6] = corner[7] = 0;
	  code = 0;
	  for (k=tse_bytes=0; k<width; k++)
	  {
        corner[1] = corner[2];
		corner[4] = corner[3];
		corner[5] = corner[6];
		corner[8] = corner[7];
		corner[2] = VoxVal(vol, CHUNK, i-1, 2*j+1, 2*k+1);
		corner[3] = VoxVal(vol, CHUNK, i-1, 2*j-1, 2*k+1);
		corner[6] = VoxVal(vol, CHUNK, i+1, 2*j+1, 2*k+1);
		corner[7] = VoxVal(vol, CHUNK, i+1, 2*j-1, 2*k+1);
		last_code = code;
		code = 0;
		for (num=1; num<=8; num++)
		  if (corner[num]>=min && corner[num]<=max)
		    code |= 1<<(num-1);
		if (triangle_table[code][0] < 0)
		  continue;
		if (k < med[vol].min_x)
		  med[vol].min_x = k;
		if (k > med[vol].max_x)
		  med[vol].max_x = k;
		if (triangle_table[last_code][0] < 0)
		{
		  Calculate_Norm(&norm, vol, CHUNK, i-1, 2*j+1, 2*k-1, TRUE);
		  BG_decode(corner_grad[1][0], corner_grad[1][1], corner_grad[1][2],
		    norm);
		  Calculate_Norm(&norm, vol, CHUNK, i-1, 2*j-1, 2*k-1, TRUE);
		  BG_decode(corner_grad[4][0], corner_grad[4][1], corner_grad[4][2],
		    norm);
		  Calculate_Norm(&norm, vol, CHUNK, i+1, 2*j+1, 2*k-1, TRUE);
		  BG_decode(corner_grad[5][0], corner_grad[5][1], corner_grad[5][2],
		    norm);
		  Calculate_Norm(&norm, vol, CHUNK, i+1, 2*j-1, 2*k-1, TRUE);
		  BG_decode(corner_grad[8][0], corner_grad[8][1], corner_grad[8][2],
		    norm);
		}
		else
		{
		  memcpy(corner_grad[1], corner_grad[2], sizeof(corner_grad[1]));
		  memcpy(corner_grad[4], corner_grad[3], sizeof(corner_grad[1]));
		  memcpy(corner_grad[5], corner_grad[6], sizeof(corner_grad[1]));
		  memcpy(corner_grad[8], corner_grad[7], sizeof(corner_grad[1]));
		}
		Calculate_Norm(&norm, vol, CHUNK, i-1, 2*j+1, 2*k+1, TRUE);
		BG_decode(corner_grad[2][0], corner_grad[2][1], corner_grad[2][2],
		  norm);
		Calculate_Norm(&norm, vol, CHUNK, i-1, 2*j-1, 2*k+1, TRUE);
		BG_decode(corner_grad[3][0], corner_grad[3][1], corner_grad[3][2],
		  norm);
		Calculate_Norm(&norm, vol, CHUNK, i+1, 2*j+1, 2*k+1, TRUE);
		BG_decode(corner_grad[6][0], corner_grad[6][1], corner_grad[6][2],
		  norm);
		Calculate_Norm(&norm, vol, CHUNK, i+1, 2*j-1, 2*k+1, TRUE);
		BG_decode(corner_grad[7][0], corner_grad[7][1], corner_grad[7][2],
		  norm);
		TSE[tse_bytes] = code;
		TSE[tse_bytes+1] = k>>8;
		TSE[tse_bytes+2] = k & 255;
		tse_bytes += 3;
		(*nt)++;
		for (num=0; triangle_table[code][num]>=0; num++)
		{
		  memset(corner_grad[0], 0, sizeof(corner_grad[0]));
		  for (m=0; m<3; m++)
		  {
		    a = corner[ev[triangle_edges[triangle_table[code][num]][m]][0]];
			b = corner[ev[triangle_edges[triangle_table[code][num]][m]][1]];
			assert((a>=min&&a<=max) != (b>=min&&b<=max));
			w[m] = (float)((a<min || b<min? min-a: b-max)/(b-a));
			for (p=0; p<3; p++)
			{
			  corner_grad[0][p] += w[m]*corner_grad[
			    ev[triangle_edges[triangle_table[code][num]][m]][1]][p]+
			    (1-w[m])*corner_grad[
				ev[triangle_edges[triangle_table[code][num]][m]][0]][p];
			  vertex_coord[m][p] =
			    edge_coord[triangle_edges[triangle_table[code][num]][m]][p]>1?
				w[m]:
				edge_coord[triangle_edges[triangle_table[code][num]][m]][p];
			}
			vertex_coord[m][0] = (vertex_coord[m][0]+k)*2*med[0].Px;
			vertex_coord[m][1] *= 2*med[0].Py;
			vertex_coord[m][2] *= 2*med[0].Pz;
		  }
		  for (m=0; m<3; m++)
		  {
		    d[m] = (int)rint((w[m]-.5)*7)&7;
			if (d[m] == 4)
				d[m] = 3;
		  }
		  TSE[tse_bytes] = d[0]<<5 | d[1]<<2 | d[2]>>1;
		  norm =
		    BG_code(corner_grad[0][0], corner_grad[0][1], corner_grad[0][2]);
		  TSE[tse_bytes+1] = (d[2]&1? 128: 0) | norm>>8;
		  TSE[tse_bytes+2] = norm & 255;
		  tse_bytes += 3;
		  total_triangles++;
		  for (m=0; m<3; m++)
		    if (triangle_edges[triangle_table[code][num]][m]==1 ||
			    triangle_edges[triangle_table[code][num]][m]==3 ||
			    triangle_edges[triangle_table[code][num]][m]==5 ||
			    triangle_edges[triangle_table[code][num]][m]==7)
			  break;
		  if (m < 3)
		    neg_vol =
			  corner[ev[triangle_edges[triangle_table[code][num]][m]][1]]>=min
			  &&
			  corner[ev[triangle_edges[triangle_table[code][num]][m]][1]]<=max;
		  else if (triangle_edges[triangle_table[code][num]][0]==2 &&
			  triangle_edges[triangle_table[code][num]][1]==4)
			neg_vol = (vertex_coord[1][1]>vertex_coord[0][1])==((code&2)!=0);
		  else if (triangle_edges[triangle_table[code][num]][0]==6 &&
			  triangle_edges[triangle_table[code][num]][1]==8)
			neg_vol = (vertex_coord[1][1]>vertex_coord[0][1])==((code&32)!=0);
		  else if (triangle_edges[triangle_table[code][num]][1]==6 &&
			  triangle_edges[triangle_table[code][num]][2]==8)
			neg_vol = (vertex_coord[2][1]>vertex_coord[1][1])==((code&32)!=0);
		  else if (triangle_edges[triangle_table[code][num]][0]==9 &&
			  triangle_edges[triangle_table[code][num]][1]==10)
			neg_vol = (vertex_coord[1][2]>vertex_coord[0][2])==(code&1);
		  else if (triangle_edges[triangle_table[code][num]][1]==9 &&
			  triangle_edges[triangle_table[code][num]][2]==10)
			neg_vol = (vertex_coord[2][2]>vertex_coord[1][2])==(code&1);
		  else if (triangle_edges[triangle_table[code][num]][1]==11 &&
			  triangle_edges[triangle_table[code][num]][2]==12)
			neg_vol = (vertex_coord[2][2]>vertex_coord[1][2])==((code&4)!=0);
		  else if (triangle_table[code][num]==129 /* 4, 6, 9 */ ||
		      triangle_table[code][num]==139 /* 4, 8, 12 */ ||
			  triangle_table[code][num]==140 /* 4, 9, 12 */ ||
			  triangle_table[code][num]==168 /* 6, 9, 11 */)
			neg_vol = code&1;
		  else if (triangle_table[code][num]==72 /* 2, 6, 9 */ ||
		      triangle_table[code][num]==81 /* 2, 8, 12 */ ||
		      triangle_table[code][num]==84 /* 2, 9, 12 */ ||
		      triangle_table[code][num]==130 /* 4, 6, 10 */ ||
		      triangle_table[code][num]==142 /* 4, 10, 12 */ ||
		      triangle_table[code][num]==170 /* 6, 10, 11 */)
			neg_vol = code&2;
		  else if (triangle_table[code][num]==73 /* 2, 6, 11 */ ||
		      triangle_table[code][num]==79 /* 2, 8, 10 */ ||
		      triangle_table[code][num]==85 /* 2, 10,11 */ ||
		      triangle_table[code][num]==132 /* 4, 6, 12 */ ||
		      triangle_table[code][num]==169 /* 6, 9, 12 */)
			neg_vol = code&4;
		  else if (triangle_table[code][num]==131 /* 4, 6, 11 */ ||
		      triangle_table[code][num]==138 /* 4, 8, 10 */ ||
		      triangle_table[code][num]==141 /* 4, 10, 11 */)
			neg_vol = code&8;
		  else if (triangle_table[code][num]==78 /* 2, 8, 9 */ ||
		      triangle_table[code][num]==83 /* 2, 9, 11 */ ||
		      triangle_table[code][num]==181 /* 8, 9, 12 */)
			neg_vol = code&16;
		  else if (triangle_table[code][num]==183 /* 8, 10, 12 */)
			neg_vol = code&32;
		  else if (triangle_table[code][num]==80 /* 2, 8, 11 */ ||
		      triangle_table[code][num]==182 /* 8, 10, 11 */)
			neg_vol = code&128;
		  else
		    assert(FALSE);
		  for (m=0; m<3; m++)
		  {
		    sidesquare[m] = 0;
			for (p=1; p<3; p++)
			  sidesquare[m] += (vertex_coord[(m+1)%3][p]-vertex_coord[m][p])*
			    (vertex_coord[(m+1)%3][p]-vertex_coord[m][p]);
		  }
		  Q = 0;
		  for (m=0; m<3; m++)
		  {
			  for (p=0; p<3; p++)
				  Q += sidesquare[m]*sidesquare[p];
			  Q -= 2*sidesquare[m]*sidesquare[m];
		  }
		  if (Q > 0)
			total_volume +=
			  (neg_vol? -.25*sqrt(Q): .25*sqrt(Q))*(1/3.)*
			  (vertex_coord[0][0]+vertex_coord[1][0]+vertex_coord[2][0]);
		  for (m=0; m<3; m++)
		  {
		    sidesquare[m] = 0;
			for (p=0; p<3; p++)
			  sidesquare[m] += (vertex_coord[(m+1)%3][p]-vertex_coord[m][p])*
			    (vertex_coord[(m+1)%3][p]-vertex_coord[m][p]);
		  }
		  Q = 0;
		  for (m=0; m<3; m++)
		  {
			  for (p=0; p<3; p++)
				  Q += sidesquare[m]*sidesquare[p];
			  Q -= 2*sidesquare[m]*sidesquare[m];
		  }
		  if (Q > 0)
			total_area += .25*sqrt(Q);
		}
	  }
      if (*nt && VWriteData((char *)TSE, 1, tse_bytes, med_fp, &num)) {
		printf("Could not write TSE\n");
		exit(-1);
      }
      med[vol].num_TSE += tse_bytes;
	}
	ntse += height;
  }
  printf("number of triangles = %d\n", total_triangles);
  printf("surface area = %.8f; volume = %.8f\n", total_area, total_volume);
  str_volume[vol] = (float)total_volume;
  str_surface_area[vol] = (float)total_area;
  free(TSE);
}


/************************************************************************
 *
 *      FUNCTION        : Create_Bin_Slice()
 *
 *      DESCRIPTION     : This Generate a binary valued slice that falls
 *                        on the slice specified, using the 
 *                        thresholds specified.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      :
 *                  vol  : current volume.
 *                  chunk: current chunk.
 *                  data : array on which the bin slice is written to.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : med,in, interp should be setup.
 *
 *      EXIT CONDITIONS : memory allocation errors 
 *
 *      RELATED FUNCS   : Get8Row,Get16Row
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *
 ************************************************************************/
void Create_Bin_Slice(int vol, int chunk, unsigned char *data, int slice,
    float min, float max)
{
  int w,h,i;
  static float *r1,*r2;
  static int width=0;

  if (slice<0 || slice >= med[vol].slices)  {
    w= med[vol].width>>1; h= med[vol].height>>1;
    memset(data,0,w*h);
  }
  else {
    printf("Processing slice %d/%d of volume %d/%d\n",
	   (slice+1)/2,med[vol].slices/2,vol+1,in.num_volumes);
    fflush(stdout);  
    w=med[vol].width>>1;
    if (w > width) {
      if (width) { free(r1); free(r2); }
      if ((r1=(float *)calloc(sizeof(float),w))==NULL) {
	printf("Could not allocate temp space\n");
	exit(-1);
      }
      if ((r2=(float *)calloc(sizeof(float),w))==NULL) {
	printf("Could not allocate temp space\n");
	exit(-1);
      }
      width=w;
    }
    if (in.bits_per_pixel!=16)  {
      for(i=1;i<med[vol].height;i+=2,data+=width) {
	Get8Row(data,min,max,vol,chunk,slice,i,r1,r2);
      }
    }
    else  {
      for(i=1;i<med[vol].height;i+=2,data+=width) {
	Get16Row(data,min,max,vol,chunk,slice,i,r1,r2);
      }
    }
  }
  
}


/************************************************************************
 *
 *      FUNCTION        : Get8Row
 *
 *      DESCRIPTION     : Compute a row of data from an 8 bit scene.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      :
 *                 out   - pointer to the output row.
 *                 min,max- threshold interval.
 *                 vol   - current volume.
 *                 chunk - current chunk.
 *                 slice - current slice.
 *                 row   - current row.
 *                 p1,p2 - temp spece for the row.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : p1,p2 should be allocated.
 *                        in, med and interp should be setup.
 *
 *      EXIT CONDITIONS :
 *
 *      RELATED FUNCS   :
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
void Get8Row(unsigned char *out, float min, float max, int vol, int chunk,
    int slice, int row, float *p1, float *p2)
{

  float *xr,yr,yr1,*p,*pp;
  int i,ly,lz,in_slice;
  unsigned char *pt1,*pt2;

  ly=interp.y_table[row];
  lz=interp.z_table[slice];
  in_slice = lz - in.ChunkOffsets[chunk];
  pt1=in.Chunk+in_slice*in.bytes_per_slice + ly*in.width;
  pt2=pt1+in.bytes_per_slice;
  if (ly <0) {
    memset(p1, 0, (med[vol].width>>1)*sizeof(float));
  }
  else {
    if (interp.x_flag) {
      for(p=p1,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	if (interp.x_table[i]<0)
	  *p = *xr * pt1[interp.x_table[i]+1];
	else if (interp.x_table[i]>=in.width-1)
	  *p = (1 - *xr) * pt1[interp.x_table[i]];
	else 
	  *p = (1 - *xr) * pt1[interp.x_table[i]] + *xr * pt1[interp.x_table[i]+1];
      }
    }
    else {
      for(p=p1,i=1;i<med[vol].width;p++,i+=2) {
	if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	  *p = 0.0;
	else 
	  *p = pt1[interp.x_table[i]];
      }
    }
  }
  if (interp.y_flag) {
    pt1 +=in.width;
    yr=interp.y_dist[row];
    yr1= 1-yr;
    if (ly >= in.height-1) {
      for(p=p1,i=1;i<med[vol].width;p++,i+=2)
	*p *=  yr1;
    }
    else {
      if (interp.x_flag) {
	for(p=p1,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	  if (interp.x_table[i]<0)
	    *p = *p * yr1 + yr* *xr * pt1[interp.x_table[i]+1];
	  else if (interp.x_table[i]>=in.width-1)
	    *p = *p * yr1 + yr*(1- *xr) * pt1[interp.x_table[i]];
	  else 
	    *p = *p * yr1 +
	      yr*((1 - *xr) *pt1[interp.x_table[i]] + *xr * pt1[interp.x_table[i]+1]);
	}
      }
      else {
	for(p=p1,i=1;i<med[vol].width;p++,i+=2) {
	  if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	    *p = yr1 * *p; 
	  else 
	    *p = yr1* (*p) + yr * pt1[interp.x_table[i]];
	}
      }
    }
  }
  if (interp.z_flag) {
    if (ly <0) {
      memset(p2, 0, (med[vol].width>>1)*sizeof(float));
    }
    else {
      if (interp.x_flag) {
	for(p=p2,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	  if (interp.x_table[i]<0)
	    *p = *xr * pt2[interp.x_table[i]+1];
	  else if (interp.x_table[i]>=in.width-1)
	    *p = (1 - *xr) * pt2[interp.x_table[i]];
	  else 
	    *p = (1 - *xr) * pt2[interp.x_table[i]] + *xr * pt2[interp.x_table[i]+1];
	}
      }
      else {
	for(p=p2,i=1;i<med[vol].width;p++,i+=2) {
	  if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	    *p = 0.0;
	  else 
	    *p = pt2[interp.x_table[i]];
	}
      }
    }
    if (interp.y_flag) {
      pt2 +=in.width;
      yr=interp.y_dist[row];
      yr1= 1-yr;
      if (ly >= in.height-1) {
	for(p=p2,i=1;i<med[vol].width;p++,i+=2)
	  *p *= yr1;
      }
      else {
	if (interp.x_flag) {
	  for(p=p2,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	    if (interp.x_table[i]<0)
	      *p = *p * yr1 + yr* *xr * pt2[interp.x_table[i]+1];
	    else if (interp.x_table[i]>=in.width-1)
	      *p = *p * yr1 + yr*(1- *xr) * pt2[interp.x_table[i]];
	    else 
	      *p = *p * yr1 +
		yr*((1 - *xr) * pt2[interp.x_table[i]] + *xr * pt2[interp.x_table[i]+1]);
	  }
	}
	else {
	  for(p=p2,i=1;i<med[vol].width;p++,i+=2) {
	    if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	      *p = yr1 * *p; 
	    else 
	      *p = yr1* (*p) + yr * pt2[interp.x_table[i]];
	  }
	}
      }
    }
    
    yr=interp.z_dist[slice];
    yr1=1-yr;
    for(p=p1,pp=p2,i=1;i<med[vol].width;i+=2,p++,pp++)
      *p = yr1 * *p + yr * *pp;
  }

  for(p=p1,i=1;i<med[vol].width;i+=2,p++,out++) {
    if ( (*p >= min) && (*p <= max)) 
      *out=1;
    else
      *out=0;
  }
  /*
    printf("(%d , %d , %d) = %f\n",slice,row,i,*p);
    printf("\n");
 */

}



/************************************************************************
 *
 *      FUNCTION        : Get16Row
 *
 *      DESCRIPTION     : Compute a row of data from an 16 bit scene.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      :
 *                 out   - pointer to the output row.
 *                 min,max- threshold interval.
 *                 vol   - current volume.
 *                 chunk - current chunk.
 *                 slice - current slice.
 *                 row   - current row.
 *                 p1,p2 - temp spece for the row.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : p1,p2 should be allocated.
 *                        in, med and interp should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
void Get16Row(unsigned char *out, float min, float max, int vol, int chunk,
    int slice, int row, float *p1, float *p2)
{

  float *xr,yr,yr1,*p,*pp;
  int i,ly,lz,in_slice;
  unsigned short *pt1,*pt2;
  unsigned char *pt_tmp;

  ly=interp.y_table[row];
  lz=interp.z_table[slice];
  in_slice = lz - in.ChunkOffsets[chunk];
  pt_tmp=in.Chunk+in_slice*in.bytes_per_slice;
  pt1= (unsigned short *)(pt_tmp) + ly*in.width;
  pt_tmp+=in.bytes_per_slice;
  pt2=(unsigned short *)(pt_tmp) + ly*in.width;

  if (ly <0) {
    memset(p1, 0, (med[vol].width>>1)*sizeof(float));
  }
  else {
    if (interp.x_flag) {
      for(p=p1,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	if (interp.x_table[i]<0)
	  *p = *xr * pt1[interp.x_table[i]+1];
	else if (interp.x_table[i]>=in.width-1)
	  *p = (1 - *xr) * pt1[interp.x_table[i]];
	else 
	  *p = (1 - *xr) * pt1[interp.x_table[i]] + *xr * pt1[interp.x_table[i]+1];
      }
    }
    else {
      for(p=p1,i=1;i<med[vol].width;p++,i+=2) {
	if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	  *p = 0.0;
	else 
	  *p = pt1[interp.x_table[i]];
      }
    }
  }
  if (interp.y_flag) {
    pt1 +=in.width;
    yr=interp.y_dist[row];
    yr1= 1-yr;
    if (ly >= in.height-1) {
      for(p=p1,i=1;i<med[vol].width;p++,i+=2)
	*p *=  yr1;
    }
    else {
      if (interp.x_flag) {
	for(p=p1,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	  if (interp.x_table[i]<0)
	    *p = *p * yr1 + yr* *xr * pt1[interp.x_table[i]+1];
	  else if (interp.x_table[i]>=in.width-1)
	    *p = *p * yr1 + yr*(1- *xr) * pt1[interp.x_table[i]];
	  else 
	    *p = *p * yr1 +
	      yr*((1 - *xr) *pt1[interp.x_table[i]] + *xr * pt1[interp.x_table[i]+1]);
	}
      }
      else {
	for(p=p1,i=1;i<med[vol].width;p++,i+=2) {
	  if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	    *p = yr1 * *p; 
	  else 
	    *p = yr1* (*p) + yr * pt1[interp.x_table[i]];
	}
      }
    }
  }
  if (interp.z_flag) {
    if (ly <0) {
      memset(p2, 0, (med[vol].width>>1)*sizeof(float));
    }
    else {
      if (interp.x_flag) {
	for(p=p2,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	  if (interp.x_table[i]<0)
	    *p = *xr * pt2[interp.x_table[i]+1];
	  else if (interp.x_table[i]>=in.width-1)
	    *p = (1 - *xr) * pt2[interp.x_table[i]];
	  else 
	    *p = (1 - *xr) * pt2[interp.x_table[i]] + *xr * pt2[interp.x_table[i]+1];
	}
      }
      else {
	for(p=p2,i=1;i<med[vol].width;p++,i+=2) {
	  if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	    *p = 0.0;
	  else 
	    *p = pt2[interp.x_table[i]];
	}
      }
    }
    if (interp.y_flag) {
      pt2 +=in.width;
      yr=interp.y_dist[row];
      yr1= 1-yr;
      if (ly >= in.height-1) {
	for(p=p2,i=1;i<med[vol].width;p++,i+=2)
	  *p *= yr1;
      }
      else {
	if (interp.x_flag) {
	  for(p=p2,xr=interp.x_dist+1,i=1;i<med[vol].width;xr+=2,p++,i+=2) {
	    if (interp.x_table[i]<0)
	      *p = *p * yr1 + yr* *xr * pt2[interp.x_table[i]+1];
	    else if (interp.x_table[i]>=in.width-1)
	      *p = *p * yr1 + yr*(1- *xr) * pt2[interp.x_table[i]];
	    else 
	      *p = *p * yr1 +
		yr*((1 - *xr) * pt2[interp.x_table[i]] + *xr * pt2[interp.x_table[i]+1]);
	  }
	}
	else {
	  for(p=p2,i=1;i<med[vol].width;p++,i+=2) {
	    if (interp.x_table[i]<0 || interp.x_table[i]>=in.width)
	      *p = yr1 * *p; 
	    else 
	      *p = yr1* (*p) + yr * pt2[interp.x_table[i]];
	  }
	}
      }
    }
    
    yr=interp.z_dist[slice];
    yr1=1-yr;
    for(p=p1,pp=p2,i=1;i<med[vol].width;i+=2,p++,pp++)
      *p = yr1 * *p + yr * *pp;
  }

  for(p=p1,i=1;i<med[vol].width;i+=2,p++,out++) {
    if ( *p >= min && *p<= max) 
      *out=1;
    else
      *out=0;
  }

}




/************************************************************************
 *
 *      FUNCTION        : InitShell0()
 *
 *      DESCRIPTION     : Initialize the StructureInfo segment of the
 *                        output header of SHELL0. This uses information 
 *                        generated by the tracking to initialize these 
 *                        values.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : vols - Number of volumes in the input scene.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in,
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : get_min_max_values.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *                        Modified: 4/9/98 in.filename copied to
 *                         out.vh.str.scene_file by Dewey Odhner.
 *                        Modified: 5/21/01 new bit fields specified
 *                         by Dewey Odhner.
 *                        Modified: 12/11/02 out.vh.str.domain adjustment
 *                         corrected by Dewey Odhner.
 *
 ************************************************************************/
void InitShell0(int vols)
{
  int i,j,temp_vols,elem;
  float min_slice_loc;
  StructureInfo *st;
  FILE *pfp;

  st= &out.vh.str;

  min_slice_loc=8000.0;
  for(i=0;i<vols;i++) 
    if (min_slice_loc > in.slice_locs[i]) min_slice_loc=in.slice_locs[i];
  st->dimension=3;
  st->dimension_valid=1;


  st->num_of_structures=vols;
  st->num_of_structures_valid=1;

  st->domain=(float *)malloc(sizeof(float)*12*vols);
  if (in.vh.scn.dimension==3) {
    for(i=0;i<12;i++)
      st->domain[i]=in.vh.scn.domain[i];
  }
  else   /* 4D */
    for(j=0;j<vols;j++) {
      for(i=0;i<4;i++) {
	st->domain[j*12+i*3]=in.vh.scn.domain[i*4];
	st->domain[j*12+i*3+1]=in.vh.scn.domain[i*4+1];
	st->domain[j*12+i*3+2]=in.vh.scn.domain[i*4+2];
      }
    }
  for(j=0;j<vols;j++) {
    st->domain[j*12] += st->domain[j*12+9]*min_slice_loc;
    st->domain[j*12+1] += st->domain[j*12+10]*min_slice_loc;
    st->domain[j*12+2] += st->domain[j*12+11]*min_slice_loc;  
  }
  st->domain_valid=1;

  if (in.vh.scn.axis_label_valid) {
    st->axis_label=(Char30 *)malloc(sizeof(Char30)*3);
    strcpy(st->axis_label[0],in.vh.scn.axis_label[0]);
    strcpy(st->axis_label[1],in.vh.scn.axis_label[1]);
    strcpy(st->axis_label[2],in.vh.scn.axis_label[2]);
    st->axis_label_valid=1;
  }
  else st->axis_label_valid=0;

  if (in.vh.scn.measurement_unit_valid) {
    st->measurement_unit=(short *)malloc(sizeof(short)*3);
    st->measurement_unit[0]=in.vh.scn.measurement_unit[0];
    st->measurement_unit[1]=in.vh.scn.measurement_unit[1];
    st->measurement_unit[2]=in.vh.scn.measurement_unit[2];
    st->measurement_unit_valid=1;
  }
  else st->measurement_unit_valid=0;

  if (in.vh.scn.num_of_bits >= 8) {
    st->scene_file=(Char30 *)malloc(sizeof(Char30)*vols);
    for(j=0;j<vols;j++) {
      strncpy((char *)st->scene_file[j],in.filename, sizeof(Char30));
      st->scene_file[j][sizeof(Char30)-1]=0;
    }
    st->scene_file_valid=1;
  }
  else if (in.vh.gen.filename1_valid) {
    st->scene_file=(Char30 *)malloc(sizeof(Char30)*vols);
    for(j=0;j<vols;j++)
      strncpy((char *)st->scene_file,in.vh.gen.filename1, sizeof(Char30));
    st->scene_file_valid=1;
  }
  else {
    st->scene_file=(Char30 *)malloc(sizeof(Char30)*vols);
    for(j=0;j<vols;j++)
      strncpy((char *)st->scene_file[j],in.filename, sizeof(Char30));
    st->scene_file_valid=1;
  }
    

  get_min_max_SHELL0_values(vols);
  

  st->num_of_TSE=(unsigned int *)malloc(vols*sizeof(int));
  st->num_of_NTSE=(unsigned int *)malloc(vols*sizeof(int));

  for(i=0;i<vols;i++) {
    st->num_of_TSE[i]=med[i].num_TSE;
    printf("TSE's in volume %d = %u\n",i,st->num_of_TSE[i]);

    st->num_of_TSE_valid=1;
    
    st->num_of_NTSE[i]=1+(med[i].max_z-med[i].min_z+1)*(med[i].max_y-med[i].min_y+2);
    st->num_of_NTSE_valid=1;  

  }
  st->num_of_components_in_TSE=9; /* ncode,y1,tt,n1,n2,n3,gm,op,og */
  st->num_of_components_in_TSE_valid=1;
  
  st->num_of_components_in_NTSE=1;
  st->num_of_components_in_NTSE_valid=1;
  
  st->smallest_value=(float *)malloc(sizeof(float)*9*vols);
  st->largest_value=(float *)malloc(sizeof(float)*9*vols);

  for(i=0;i<vols;i++) {  
    st->smallest_value[i*9+0]=0;   /* ncode */
    st->largest_value[i*9+0]=63.0;
    st->smallest_value[i*9+1]=med[i].min_x; /* y1 */
    st->largest_value[i*9+1]=med[i].max_x;
    st->smallest_value[i*9+2]=st->largest_value[i*9+2]=0.0;
    st->smallest_value[i*9+3]=0.0; st->largest_value[i*9+3]=6.0; /* normals */
    st->smallest_value[i*9+4]=0.0; st->largest_value[i*9+4]=63.0;
    st->smallest_value[i*9+5]=0.0; st->largest_value[i*9+5]=63.0;
    st->smallest_value[i*9+6]=st->largest_value[i*9+6]=0.0;
    st->smallest_value[i*9+7]=st->largest_value[i*9+7]=0.0;
    st->smallest_value[i*9+8]=st->largest_value[i*9+8]=0.0;
  }
  st->smallest_value_valid=1;
  st->largest_value_valid=1;

  st->num_of_integers_in_TSE=9;
  st->num_of_integers_in_TSE_valid=1;
  
  st->signed_bits_in_TSE=(short *)malloc(sizeof(short)*9);
  st->signed_bits_in_TSE[0]=0;
  st->signed_bits_in_TSE[1]=0;
  st->signed_bits_in_TSE[2]=0;
  st->signed_bits_in_TSE[3]=0;
  st->signed_bits_in_TSE[4]=0;
  st->signed_bits_in_TSE[5]=0;
  st->signed_bits_in_TSE[6]=st->signed_bits_in_TSE[7]=st->signed_bits_in_TSE[8]=0;
  st->signed_bits_in_TSE_valid=1;

  
  st->num_of_bits_in_TSE=32;
  st->num_of_bits_in_TSE_valid=1;

  st->bit_fields_in_TSE=(short *)malloc(sizeof(short)*18);
  st->bit_fields_in_TSE[0]=0;    st->bit_fields_in_TSE[1]=5;
  st->bit_fields_in_TSE[2]=6;    st->bit_fields_in_TSE[3]=16;
  st->bit_fields_in_TSE[4]=16;   st->bit_fields_in_TSE[5]=15;
  st->bit_fields_in_TSE[6]=17;   st->bit_fields_in_TSE[7]=19;
  st->bit_fields_in_TSE[8]=20;   st->bit_fields_in_TSE[9]=25;
  st->bit_fields_in_TSE[10]=26;  st->bit_fields_in_TSE[11]=31;
  st->bit_fields_in_TSE[12]=32;  st->bit_fields_in_TSE[13]=31;
  st->bit_fields_in_TSE[14]=40;  st->bit_fields_in_TSE[15]=39;
  st->bit_fields_in_TSE[16]=40;  st->bit_fields_in_TSE[17]=39;
  st->bit_fields_in_TSE_valid=1;
 
  st->num_of_integers_in_NTSE=1;
  st->num_of_integers_in_NTSE_valid=1;
 
  st->signed_bits_in_NTSE=(short *)malloc(sizeof(short));
  st->signed_bits_in_NTSE[0]=0;
  st->signed_bits_in_NTSE_valid=1;
  
  st->num_of_bits_in_NTSE=16;
  st->num_of_bits_in_NTSE_valid=1;
  
  st->bit_fields_in_NTSE=(short *)malloc(sizeof(short)*2);
  st->bit_fields_in_NTSE[0]=0;
  st->bit_fields_in_NTSE[1]=15;
  st->bit_fields_in_NTSE_valid=1;

  st->num_of_samples=(short *)malloc(sizeof(short));
  st->num_of_samples[0]=(interp.zsize>>1);
  st->num_of_samples_valid=1;
 
  st->xysize[0]= 2*med[0].Px;
  st->xysize[1]= 2*med[0].Py;
  st->xysize_valid=1;
 
  st->loc_of_samples=(float *)malloc(sizeof(float)*interp.zsize);
  st->loc_of_samples[0]=0.0;
  for(i=1;i<st->num_of_samples[0];i++)
    st->loc_of_samples[i]=  2*i*med[0].Pz;
  st->loc_of_samples_valid=1;


  if (!param_on) {
    if (vols==1) {
      st->num_of_elements=1;
    st->num_of_elements_valid=1;
      
      st->description_of_element=(short *)malloc(sizeof(short));
      st->description_of_element[0]=1; /* type == object label */
      st->description_of_element_valid=1;
      st->parameter_vectors=(float *)malloc(sizeof(float));
      st->parameter_vectors[0]= 0.0;
      st->parameter_vectors_valid=1;
      
    }
    else {
      st->num_of_elements=2;
      st->num_of_elements_valid=1;
      
      st->description_of_element=(short *)malloc(2*sizeof(short));
      st->description_of_element[0]=0; /* type == time label */
      st->description_of_element[1]=1; /* type == object label */    
      st->description_of_element_valid=1;
      st->parameter_vectors=(float *)malloc(sizeof(float)*2*vols);
      for(i=0;i<vols;i++) {
	st->parameter_vectors[2*i]= (float)i;
	st->parameter_vectors[2*i+1]= 0.0;
      }
      st->parameter_vectors_valid=1;
      
    }
  }
  else {
    st->num_of_elements_valid=1;
    st->description_of_element_valid=1;
    st->parameter_vectors_valid=1;
    pfp=fopen(param_file,"rb");
    if (pfp==NULL) {
      printf("Could not read parameters from file\n");
      st->num_of_elements_valid=0;
      st->description_of_element_valid=0;
      st->parameter_vectors_valid=0;
    }
    else if (fscanf(pfp,"%d",&elem)!=1) {
      printf("Could not read parameters from file\n");
      st->num_of_elements_valid=0;
      st->description_of_element_valid=0;
      st->parameter_vectors_valid=0;
    }
    else {
      st->num_of_elements=elem;
      st->description_of_element=(short *)calloc(st->num_of_elements*vols,
						 sizeof(short));
      st->parameter_vectors=(float *)calloc(st->num_of_elements*vols,
					    sizeof(float));
      for(i=0;i<st->num_of_elements;i++) {
	if (fscanf(pfp,"%d",&elem)!=1) {
	  printf("Could not read parameters from file\n");
	  st->num_of_elements_valid=0;
	  st->description_of_element_valid=0;
	  st->parameter_vectors_valid=0;
	  break;
	}
	else
	  st->description_of_element[i]=elem;
      }
      if (st->parameter_vectors_valid==0 || fscanf(pfp,"%d",&temp_vols)!=1) {
	printf("Could not read parameters from file\n");
	st->num_of_elements_valid=0;
	st->description_of_element_valid=0;
	st->parameter_vectors_valid=0;
      }
      else {
	if (temp_vols>vols) temp_vols=vols;
	for(i=0;i<st->num_of_elements*temp_vols;i++)
	  if (fscanf(pfp,"%f",&st->parameter_vectors[i])!=1) {
	    printf("Could not read parameters from file\n");
	    st->num_of_elements_valid=0;
	    st->description_of_element_valid=0;
	    st->parameter_vectors_valid=0;
	    break;
	  }
      }
    }
  }
  
  
  st->min_max_coordinates=(float *)malloc(sizeof(float)*6*vols);
  for(i=0;i<vols;i++) {
    st->min_max_coordinates[i*6]=2*med[i].Px*med[i].min_x;
    st->min_max_coordinates[i*6+1]=2*med[i].Py*med[i].min_y;
    st->min_max_coordinates[i*6+2]=2*med[i].Pz*med[i].min_z;
    st->min_max_coordinates[i*6+3]=2*med[i].Px*med[i].max_x;
    st->min_max_coordinates[i*6+4]=2*med[i].Py*med[i].max_y;
    st->min_max_coordinates[i*6+5]=2*med[i].Pz*med[i].max_z;
  }
  st->min_max_coordinates_valid=1;

  st->volume=(float *)malloc(vols*sizeof(float));
  for(i=0;i<vols;i++) {
   st->volume[i]= med[i].volume*med[i].Px*med[i].Py*med[i].Pz*8;
  }
  st->volume_valid=1;

}

/************************************************************************
 *
 *      FUNCTION        : get_min_max_SHELL0_values()
 *
 *      DESCRIPTION     :  Get the bounding box of the tracked surface 
 *                         in y and z drections of SHELL0 files.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : vols - number of volumes in ht einput scene.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : med structure should be Initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
void get_min_max_SHELL0_values(int vols)
{
  short *ptr;
  int i,j,v,slices,height;

  
  for(v=0;v<vols;v++) {
    slices=med[v].slices>>1;
    height=med[v].height>>1;
    for(ptr=med[v].NTSE,i=0;i<slices;i++)
      for(j=0; j<height; j++,ptr++) {
        if (*ptr) {
          if (i > med[v].max_z)
            med[v].max_z=i;
          if (j > med[v].max_y)
            med[v].max_y=j;
          if (i < med[v].min_z)
            med[v].min_z=i;
          if (j < med[v].min_y)
            med[v].min_y=j;
        }
      }
  }
}

/************************************************************************
 *
 *      FUNCTION        : get_min_max_SHELL2_values()
 *
 *      DESCRIPTION     :  Get the bounding box of the tracked surface 
 *                         in y and z drections of SHELL2 files.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : vols - number of volumes in the input scene.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : med structure should be Initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : Created: 12/11/02 by Dewey Odhner.
 *
 ************************************************************************/
void get_min_max_SHELL2_values(int vols)
{
  short *ptr;
  int i,j,v,slices,height;

  
  for(v=0; v<vols; v++) {
    slices = (med[v].slices>>1)+1;
    height = (med[v].height>>1)+1;
    for(ptr=med[v].NTSE,i=0; i<slices; i++)
      for(j=0; j<height; j++,ptr++) {
        if (*ptr) {
          if (i > med[v].max_z) med[v].max_z=i;
          if (j > med[v].max_y) med[v].max_y=j;
          if (i < med[v].min_z) med[v].min_z=i;
          if (j < med[v].min_y) med[v].min_y=j;
        }
      }
  }
}



/************************************************************************
 *
 *      FUNCTION        : WriteTSE_SHELL0()
 *
 *      DESCRIPTION     : Given a volume and number of chunks the input 
 *                        data has been devided upto and the intensity
 *                        range this generates the voxels of the surface
 *                        and writes to a temporary file.
 *
 *      RETURN VALUE    : None.
 * 
 *      PARAMETERS      : 4 parameters.
 *                 vol    : current volume being tracked.
 *                 chunks : number of chunks the scene is devided into.
 *                 min,max: intensity range if interest.
 *
 *      SIDE EFFECTS    : med structure gets updated.
 *
 *      ENTRY CONDITION : in and med structures should be setup.
 *
 *      EXIT CONDITIONS : memory allocation fault, or read/write
 *                        error.
 *
 *      RELATED FUNCS   : ReadChunk,Create_Bin_Slice,Calculate_Norm
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 5/21/01 new bit fields used by Dewey Odhner
 *
 ************************************************************************/
void WriteTSE_SHELL0(int vol, int chunks, float min, float max)
{
  int CHUNK,i,k,l,l1,k1,num_TSE,sum,width,height,num;
  short *ntse;
  unsigned short NCODE;
  TSE_type *TSE;
  unsigned char *temp_sl,*nz,*pz,*ny,*py,*nx,*px,*cur;



  sum=0;
  TSE=(TSE_type *)calloc(sizeof(TSE_type),med[vol].width);
  if (TSE==NULL) {
    printf("Could not allocate temp space for TSE's\n");
    exit(-1);
  }
  CHUNK=0;
  width=med[vol].width>>1;
  height=med[vol].height>>1;

  ReadChunk(vol,CHUNK);
  Create_Bin_Slice(vol,CHUNK,med_sl2,-1,min,max);
  Create_Bin_Slice(vol,CHUNK,med_sl3,1,min,max);
  for(ntse=med[vol].NTSE,i=1;i<interp.zsize;i+=2) {
    while ( i > interp.ChunkOffsets[CHUNK]+interp.ChunkSlices-2) {
      CHUNK++;
      ReadChunk(vol,CHUNK);
    }
    temp_sl=med_sl1; med_sl1=med_sl2; med_sl2=med_sl3; med_sl3=temp_sl;
    Create_Bin_Slice(vol,CHUNK,med_sl3,i+2,min,max);
    
    num_TSE=0;
    nz=med_sl1; 
    cur=med_sl2; 
    pz=med_sl3;
    ny=cur-width;
    py=cur+width;
    nx=cur-1;
    px=cur+1;
    /* Do 1st Col of the first row */
    if (*cur) { /* This voxel is on */
      NCODE=0;
      if (*nz) NCODE|= NZ;
      if (*pz) NCODE|= PZ;
      if (*py) NCODE|= PY;
      if (*px) NCODE|= PX;
      TSE[num_TSE][0]= NCODE | 0x0000;
      Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,1,1, TRUE);
      if (med[vol].min_x>0) med[vol].min_x=0;
      if (med[vol].max_x<0) med[vol].max_x=0;
      med[vol].volume++;
      num_TSE++;
    }
    nz++,pz++,ny++,py++,nx++,px++,cur++;
    /* Do all but last colunm */
    for(l1=3,l=1;l1<med[vol].width-2;l1+=2,l++,nz++,pz++,ny++,py++,nx++,px++,cur++) {
      if (*cur) { /* This voxel is on */
        med[vol].volume++;
        NCODE=0;
        if (*nz) NCODE|= NZ;
        if (*pz) NCODE|= PZ;
        if (*py) NCODE|= PY;
        if (*px) NCODE|= PX;
        if (*nx) NCODE|= NX;
        TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
        Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,1,l1, TRUE);
        if (l & 1) TSE[num_TSE][1] |= 0x8000;
        if (med[vol].min_x>l) med[vol].min_x=l;
        if (med[vol].max_x<l) med[vol].max_x=l;
        num_TSE++;
      }
    }
    /* Do last Col of the first row */
    if (*cur) { /* This voxel is on */
      med[vol].volume++;
      NCODE=0;
      if (*nz) NCODE|= NZ;
      if (*pz) NCODE|= PZ;
      if (*py) NCODE|= PY;
      if (*nx) NCODE|= NX;
      TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
      Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,1,l1, TRUE);
      if (l & 1) TSE[num_TSE][1] |= 0x8000;
      if (med[vol].min_x>l) med[vol].min_x=l;
      if (med[vol].max_x<l) med[vol].max_x=l;
      num_TSE++;
    }
    nz++,pz++,ny++,py++,nx++,px++,cur++;
    *ntse=num_TSE; 
    if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,num_TSE*2,med_fp,&num)) {
      printf("Could not write TSE\n");
    }      
    ntse++;
    sum += num_TSE;
    
    
    /* Do all rows exept the last row */
    for(k1=3,k=1;k1<med[vol].height-2;k++,k1+=2) {
      num_TSE=0;
      /* Do 1st Col of the row */
      if (*cur) { /* This voxel is on */
        med[vol].volume++;
        NCODE=0;
        if (*pz) NCODE|= PZ;
        if (*nz) NCODE|= NZ;
        if (*py) NCODE|= PY;
        if (*ny) NCODE|= NY;
        if (*px) NCODE|= PX;
        TSE[num_TSE][0]= NCODE | 0x0000;
        Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,k1,1, TRUE);
        if (med[vol].min_x>0) med[vol].min_x=0;
        if (med[vol].max_x<0) med[vol].max_x=0;
        num_TSE++;
      }
      nz++,pz++,ny++,py++,nx++,px++,cur++;
      /* Do all but last colunm */
      for(l1=3,l=1;l1<med[vol].width-2;l1+=2,l++,nz++,pz++,ny++,py++,nx++,px++,cur++) {
        if (*cur) { /* This voxel is on */
          med[vol].volume++;
          NCODE=0;
          if (*pz) NCODE|= PZ;
          if (*nz) NCODE|= NZ;
          if (*py) NCODE|= PY;
          if (*ny) NCODE|= NY;
          if (*px) NCODE|= PX;
          if (*nx) NCODE|= NX;
          if (NCODE != ALL_NEIGHBORS) {
            TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
            Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,k1,l1, TRUE);
            if (l & 1) TSE[num_TSE][1] |= 0x8000;
            if (med[vol].min_x>l) med[vol].min_x=l;
            if (med[vol].max_x<l) med[vol].max_x=l;
            num_TSE++;
          }
        }
      }
      /* Do last Col of the row */
      if (*cur) { /* This voxel is on */
        med[vol].volume++;
        NCODE=0;
        if (*pz) NCODE|= PZ;
        if (*nz) NCODE|= NZ;
        if (*py) NCODE|= PY;
        if (*ny) NCODE|= NY;
        if (*nx) NCODE|= NX;
        TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
        Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,k1,l1, TRUE);
        if (l & 1) TSE[num_TSE][1] |= 0x8000;
        if (med[vol].min_x>l) med[vol].min_x=l;
        if (med[vol].max_x<l) med[vol].max_x=l;
        num_TSE++;
      }
      nz++,pz++,ny++,py++,nx++,px++,cur++;
      *ntse=num_TSE;
      if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,med_fp,&num)) {
        printf("Could not write TSE\n");
      }      
      ntse++;
      sum += num_TSE;
    }
    
    
    /* Do last row */
    
    num_TSE=0;
    /* Do 1st Col of the row */
    if (*cur) { /* This voxel is on */
      med[vol].volume++;
      NCODE=0;
      if (*nz) NCODE|= NZ;
      if (*pz) NCODE|= PZ;
      if (*ny) NCODE|= NY;
      if (*px) NCODE|= PX;
      TSE[num_TSE][0]= NCODE | 0x0000;
      Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,k1,1, TRUE);
      if (med[vol].min_x>0) med[vol].min_x=0;
      if (med[vol].max_x<0) med[vol].max_x=0;
      num_TSE++;
    }
    nz++,pz++,ny++,py++,nx++,px++,cur++;
    /* Do all but last colunm */
    for(l1=3,l=1;l1<med[vol].width-2;l1+=2,l++,nz++,pz++,ny++,py++,nx++,px++,cur++) {
      if (*cur) { /* This voxel is on */
        med[vol].volume++;
        NCODE=0;
        if (*nz) NCODE|= NZ;
        if (*pz) NCODE|= PZ;
        if (*ny) NCODE|= NY;
        if (*px) NCODE|= PX;
        if (*nx) NCODE|= NX;
        TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
        Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,k1,l1, TRUE);
        if (l & 1) TSE[num_TSE][1] |= 0x8000;
        if (med[vol].min_x>l) med[vol].min_x=l;
        if (med[vol].max_x<l) med[vol].max_x=l;
        num_TSE++;
      }
    }
    /* Do last Col of the row */
    if (*cur) { /* This voxel is on */
      med[vol].volume++;
      NCODE=0;
      if (*nz) NCODE|= NZ;
      if (*pz) NCODE|= PZ;
      if (*ny) NCODE|= NY;
      if (*nx) NCODE|= NX;
      TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
      Calculate_Norm(&TSE[num_TSE][1],vol,CHUNK,i,k1,l1, TRUE);
      if (l & 1) TSE[num_TSE][1] |= 0x8000;
      if (med[vol].min_x>l) med[vol].min_x=l;
      if (med[vol].max_x<l) med[vol].max_x=l;
      num_TSE++;
    }
    nz++,pz++,ny++,py++,nx++,px++,cur++;
    *ntse=num_TSE;
    if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,med_fp,&num)) {
      printf("Could not write TSE\n");
    } 
    ntse++;
    sum += num_TSE;
    
    
  }
  
  med[vol].num_TSE=sum;
  free(TSE);



}





/************************************************************************
 *
 *      FUNCTION        : ReadChunk()
 *
 *      DESCRIPTION     : Read a chunk from a volume of the input data.
 *                        If a slice is out of range it would initialize
 *                        the slice to be blank (Zero's).
 *
 *      RETURN VALUE    : 0 if successful -1 otherwise.
 *
 *      PARAMETERS      : vol - current volume. 
 *                        chunk - chunk to be read.
 *
 *      SIDE EFFECTS    : in structure is updated.
 *
 *      ENTRY CONDITION : in and med structures should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera          
 *                        Modified: 11/18/05 fread replaced by Dewey Odhner.
 *
 ************************************************************************/
int ReadChunk(int vol, int chunk)
{
  unsigned char *data,*temp_slice;
  int i,start,end,size,input_bytes,num;

  /* Read in the Input data */
  data=in.Chunk;
  start=in.ChunkOffsets[chunk];
  end= start + in.ChunkSlices[chunk];
  /* Fill in the blank slices */
  for(i=start;i<0;i++) {
    memset(data,0,in.bytes_per_slice);
    data += in.bytes_per_slice;
  }
  if (in.bits_per_pixel==16)
    size=2;
  else 
    size=1;

  if (in.bits_per_pixel!=1) {
    VSeekData(in.fp,(long)((in.skip_slices[vol]+i)*in.bytes_per_slice));
    for(;i<end && i<in.slices[vol];i++) {
      if (VReadData((char *)data,size,in.bytes_per_slice/size,in.fp,&num)) {
	printf("Could not read Current Chunk\n");
	return(-1);
      }
      data += in.bytes_per_slice;
    }
  }
  else {
    input_bytes=(in.width*in.height+7)/8;
    temp_slice=(unsigned char *)calloc(sizeof(char),input_bytes);
    if (temp_slice==NULL) {
      printf("Memory Allocation error in ReadData\n");
      exit(-1);
    }
    VSeekData(in.fp,(long)((in.skip_slices[vol]+i)*input_bytes));
    for(;i<end && i<in.slices[vol];i++) {
      if (VReadData((char *)temp_slice,1,input_bytes,in.fp,&num)) {
	printf("Could not read Current Chunk\n");
	return(-1);
      }
      UnpackData(temp_slice,data,in.bytes_per_slice,0,1);
      data += in.bytes_per_slice;
    }
    free(temp_slice);
  }    
  for(;i<end;i++) {
    memset(data,0,in.bytes_per_slice);
    data += in.bytes_per_slice;
  }
  
  
  return(0);


}


/************************************************************************
 *
 *      FUNCTION        : Calculate_Norm0()
 *
 *      DESCRIPTION     : This assigns a invalid constant normal
 *                        to the pointer norm.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : norm,vol,chunk,sl,row,col: Not Used.
 *                        B: Use encoding B.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *
 ************************************************************************/
void Calculate_Norm0(norm,vol,chunk,sl,row,col, B)
unsigned short *norm;
int vol,chunk,sl,row,col, B;
{

  *norm= B? BINVALID_NORM: INVALID_NORM;

}


/************************************************************************
 *
 *      FUNCTION        : Calculate_Norm8()
 *
 *      DESCRIPTION     : This computes a 8 Neighbor normal using
 *                        the intensities on the vertices of a
 *                        cube centered at the specified point.
 *                        (Now used only by binary scenes.)
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      :
 *               norm  : pointer to the normal to be updated.
 *               vol   : current colume.
 *               chunk : current chunk.
 *               sl    : current slice.
 *               row   : current row.
 *               col   : current col.
 *               B     : flag to use better encoding.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in and med structures should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : VoxVal,G_code
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 5/21/01 B passed by Dewey Odhner.
 *
 ************************************************************************/
void Calculate_Norm8(norm,vol,chunk,sl,row,col, B)
unsigned short *norm;
int vol,chunk,sl,row,col, B;
{
  float p111,p222,p122,p211,p121,p212,p221,p112;
  float v1,v2,v3,v4;
  const int d=sl&row&col&1? 2:1;


  p111=VoxVal(vol,chunk,sl+d,row+d,col+d);
  p222=VoxVal(vol,chunk,sl-d,row-d,col-d);
  p122=VoxVal(vol,chunk,sl+d,row-d,col-d);
  p211=VoxVal(vol,chunk,sl-d,row+d,col+d);
  p121=VoxVal(vol,chunk,sl+d,row-d,col+d);
  p212=VoxVal(vol,chunk,sl-d,row+d,col-d);
  p221=VoxVal(vol,chunk,sl-d,row-d,col+d);
  p112=VoxVal(vol,chunk,sl+d,row+d,col-d);
  v1= (p111-p222); v2= (p211-p122);
  v3= (p221-p112); v4= (p121-p212);
  *norm= B? BG_code((v1+v2+v3+v4),(v1+v2-v3-v4),(v1-v2-v3+v4)):
    G_code((v1+v2+v3+v4),(v1+v2-v3-v4),(v1-v2-v3+v4));
}


#define ROOT2 0.7071067811865475244008443
#define ROOT3 0.5773502691896257645091487

/************************************************************************
 *
 *      FUNCTION        : Calculate_Norm26()
 *
 *      DESCRIPTION     : This computes a 26 Neighbor normal using
 *                        the intensities along the edges of a
 *                        cube centered at the specified point.
 *                        (Now used only by binary scenes.)
 *
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      :
 *               norm  : pointer to the normal to be updated.
 *               vol   : current colume.
 *               chunk : current chunk.
 *               sl    : current slice.
 *               row   : current row.
 *               col   : current col.
 *               B     : flag to use better encoding.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in and med structures should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : VoxVal,BG_code
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 5/21/01 B passed by Dewey Odhner.
 *
 ************************************************************************/
void Calculate_Norm26(norm,vol,chunk,sl,row,col, B)
unsigned short *norm;
int vol,chunk,sl,row,col, B;
{
  float p001,p002,p010,p011,p012,p020,p021,p022;
  float p100,p101,p102,p110,p111,p112,p120,p121,p122;
  float p200,p201,p202,p210,p211,p212,p220,p221,p222;
  float l1,l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13;
  const int d=sl&row&col&1? 2:1;

  p001=VoxVal(vol,chunk,sl  ,row  ,col+d);
  p002=VoxVal(vol,chunk,sl  ,row  ,col-d);
  p010=VoxVal(vol,chunk,sl  ,row+d,col  );
  p011=VoxVal(vol,chunk,sl  ,row+d,col+d);
  p012=VoxVal(vol,chunk,sl  ,row+d,col-d);
  p020=VoxVal(vol,chunk,sl  ,row-d,col  );
  p021=VoxVal(vol,chunk,sl  ,row-d,col+d);
  p022=VoxVal(vol,chunk,sl  ,row-d,col-d);
  p100=VoxVal(vol,chunk,sl+d,row  ,col  );
  p101=VoxVal(vol,chunk,sl+d,row  ,col+d);
  p102=VoxVal(vol,chunk,sl+d,row  ,col-d);
  p110=VoxVal(vol,chunk,sl+d,row+d,col  );
  p111=VoxVal(vol,chunk,sl+d,row+d,col+d);
  p112=VoxVal(vol,chunk,sl+d,row+d,col-d);
  p120=VoxVal(vol,chunk,sl+d,row-d,col  );
  p121=VoxVal(vol,chunk,sl+d,row-d,col+d);
  p122=VoxVal(vol,chunk,sl+d,row-d,col-d);
  p200=VoxVal(vol,chunk,sl-d,row  ,col  );
  p201=VoxVal(vol,chunk,sl-d,row  ,col+d);
  p202=VoxVal(vol,chunk,sl-d,row  ,col-d);
  p210=VoxVal(vol,chunk,sl-d,row+d,col  );
  p211=VoxVal(vol,chunk,sl-d,row+d,col+d);
  p212=VoxVal(vol,chunk,sl-d,row+d,col-d);
  p220=VoxVal(vol,chunk,sl-d,row-d,col  );
  p221=VoxVal(vol,chunk,sl-d,row-d,col+d);
  p222=VoxVal(vol,chunk,sl-d,row-d,col-d);
  l1=p100-p200;  l2=p110-p220;  l3=p120-p210;  l4=p101-p202;
  l5=p102-p201;  l6=p111-p222;  l7=p112-p221;  l8=p121-p212;
  l9=p122-p211; l10=p010-p020; l11=p011-p022; l12=p012-p021;
  l13=p001-p002;
  *norm= B? BG_code(l13+ ROOT2*(l11+l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
		l10+ ROOT2*(l2-l3+l11+l12) + ROOT3*(l6+l7-l8-l9),
		l1 + ROOT2*(l2+l3+l4 +l5 ) + ROOT3*(l6+l7+l8+l9)):
		G_code(l13+ ROOT2*(l11+l12+l4-l5) + ROOT3*(l6-l7+l8-l9),
		l10+ ROOT2*(l2-l3+l11+l12) + ROOT3*(l6+l7-l8-l9),
		l1 + ROOT2*(l2+l3+l4 +l5 ) + ROOT3*(l6+l7+l8+l9));
  
}





/************************************************************************
 *
 *      FUNCTION        : VoxOn()
 *
 *      DESCRIPTION     : Given a point and a threshold limits find
 *                        if the voxel is within that interval.
 *
 *      RETURN VALUE    : 0 - outside the interval.
 *                        1 - inside the interval.
 *
 *      PARAMETERS      : 
 *              min_thresh : lower threshold limit.
 *              max_thresh : upper threshold limit.
 *              vol        : current volume.
 *              chunk      : current chunk.
 *              slice      : current slice.
 *              row        : current row.
 *              col        : current colunm.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   :VoxVal
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *
 ************************************************************************/
unsigned short VoxOn(min_thresh,max_thresh,vol,chunk,slice,row,col)
float min_thresh,max_thresh;
int vol,chunk,slice,row,col;
{

  float val;

  val=VoxVal(vol,chunk,slice,row,col);
  if (val >= min_thresh && val <= max_thresh)
    return(1);
  else
    return(0);
  
  
}


/************************************************************************
 *
 *      FUNCTION        : VoxVal()
 *
 *      DESCRIPTION     : Given a point use bi-lenear interpolation to
 *                        find the intensity of that point.
 *
 *      RETURN VALUE    : Value of the Point specified.
 *
 *      PARAMETERS      : 
 *              vol        : current volume.
 *              chunk      : current chunk.
 *              slice      : current slice.
 *              row        : current row.
 *              col        : current colunm.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in ,med and interp structures should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : InVoxVal
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
float VoxVal(int vol, int chunk, int slice, int row, int col)
{
  int lx,ly,lz,p000,p001,p010,p011,p100,p101,p110,p111;
  short flag;
  float val,xr,yr,zr;

  if (col<0 || row<0 || slice <0 || 
      slice >= interp.zsize || col >= interp.xsize || row >= interp.ysize)
    return(0.0);
  lx=interp.x_table[col];
  ly=interp.y_table[row];
  lz=interp.z_table[slice];

  flag=interp.x_flag|interp.y_flag|interp.z_flag;
  switch (flag) {
  case 0: /* no interpolation in X,Y,Z */
    val=(float)InVoxVal(vol,chunk,lz,ly,lx);
    break;
  case 1:  /* Z interpolation only */
    zr=interp.z_dist[slice];
    val= (float)(InVoxVal(vol,chunk,lz,ly,lx)*(1.0-zr)+InVoxVal(vol,chunk,lz+1,ly,lx)*zr);
    break;
  case 7: /* Interpolate in All Directions */
    p000=InVoxVal(vol,chunk,lz  ,ly  ,lx);  p001=InVoxVal(vol,chunk,lz  ,ly  ,lx+1);
    p010=InVoxVal(vol,chunk,lz  ,ly+1,lx);  p011=InVoxVal(vol,chunk,lz  ,ly+1,lx+1);
    p100=InVoxVal(vol,chunk,lz+1,ly  ,lx);  p101=InVoxVal(vol,chunk,lz+1,ly  ,lx+1);
    p110=InVoxVal(vol,chunk,lz+1,ly+1,lx);  p111=InVoxVal(vol,chunk,lz+1,ly+1,lx+1);
    xr= interp.x_dist[col];
    yr= interp.y_dist[row];
    zr= interp.z_dist[slice];
    val= 
      ((1-zr)*((1-yr)*((1-xr)*p000 + xr*p001) + yr*((1-xr)*p010 + xr*p011)) +
       zr*((1-yr)*((1-xr)*p100 + xr*p101) + yr*((1-xr)*p110 + xr*p111)));
    break;
  case 2: /* Y interpolation only */
    yr=interp.y_dist[row];
    val= (float)(InVoxVal(vol,chunk,lz,ly,lx)*(1.0-yr)+InVoxVal(vol,chunk,lz,ly+1,lx)*yr);
    break;
  case 4: /* X interpolation only */
    xr=interp.x_dist[col];
    val= (float)(InVoxVal(vol,chunk,lz,ly,lx)*(1.0-xr)+InVoxVal(vol,chunk,lz,ly,lx+1)*xr);
    break;
  case 3: /* Y and Z interpolation */
    p000=InVoxVal(vol,chunk,lz  ,ly  ,lx);
    p010=InVoxVal(vol,chunk,lz  ,ly+1,lx);
    p100=InVoxVal(vol,chunk,lz+1,ly  ,lx);
    p110=InVoxVal(vol,chunk,lz+1,ly+1,lx);
    yr= interp.y_dist[row];
    zr= interp.z_dist[slice];
    val= 
      ((1-zr)*((1-yr)*p000 + yr*p010) +
       zr*((1-yr)*p100 + yr*p110));
    break;
  case 5: /* X and Z interpolation */
    p000=InVoxVal(vol,chunk,lz  ,ly  ,lx);  p001=InVoxVal(vol,chunk,lz  ,ly  ,lx+1);
    p100=InVoxVal(vol,chunk,lz+1,ly  ,lx);  p101=InVoxVal(vol,chunk,lz+1,ly  ,lx+1);
    xr= interp.x_dist[col];
    zr= interp.z_dist[slice];
    val= 
      ((1-zr)*((1-xr)*p000 + xr*p001) +
       zr*((1-xr)*p100 + xr*p101));
    break;
  case 6: /* X and Y interpolation */
    p000=InVoxVal(vol,chunk,lz  ,ly  ,lx);  p001=InVoxVal(vol,chunk,lz  ,ly  ,lx+1);
    p010=InVoxVal(vol,chunk,lz  ,ly+1,lx);  p011=InVoxVal(vol,chunk,lz  ,ly+1,lx+1);
    xr= interp.x_dist[col];
    yr= interp.y_dist[row];
    val= 
      (1-yr)*((1-xr)*p000 + xr*p001) + yr*((1-xr)*p010 + xr*p011);
    break;
  default:
    return(0.0);
  }  
  
  
  return(val);
   
  
}


/************************************************************************
 *
 *      FUNCTION        : InVoxVal()
 *
 *      DESCRIPTION     : Given a point on the input scene
 *                        find the intensity of that point.
 *
 *      RETURN VALUE    : Value of the Point specified.
 *
 *      PARAMETERS      : 
 *              vol        : current volume.
 *              chunk      : current chunk.
 *              in_slice   : current input slice.
 *              in_row     : current input row.
 *              in_col     : current input colunm.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in  & med structures should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
int InVoxVal(int vol, int chunk, int in_slice, int in_row, int in_col)
{

  int pos;
  unsigned char *ptr;
  unsigned short *ptr16;

  if (in_slice<0 || in_row < 0 || in_col<0 ||
      in_slice>=in.slices[vol] || in_row >= in.height || in_col >= in.width)
    return(0);

  in_slice -= in.ChunkOffsets[chunk];
  assert(in_slice>=0 && in_slice<in.ChunkSlices[chunk]);
  pos= in_row*in.width + in_col;
  switch (in.bits_per_pixel) {
  case 1:
    ptr = in.Chunk + in_slice*in.bytes_per_slice + pos;
    return((int)(*ptr));
  case 8:
    ptr = in.Chunk + in_slice*in.bytes_per_slice + pos;
    return((int)(*ptr));
  case 16:
    ptr16  = (unsigned short *)(in.Chunk + in_slice*in.bytes_per_slice);
    ptr16 += pos;
    return((int)(*ptr16));
  }
  return(0);
}
  
  





/************************************************************************
 *
 *      FUNCTION        : SetupMedValues()
 *
 *      DESCRIPTION     : This initializes med and interp strucutres
 *                        med strucutre sets up values for the output
 *                        structure while interp setup tables for 
 *                        interpolating input data to the structure
 *                        resolution.
 *
 *      RETURN VALUE    : none.
 *
 *      PARAMETERS      : ratio. structure pixel size as a ratio 
 *                        of the input pixel size.  If negative,
 *                        voxels will not be resized.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : None.
 *
 *      EXIT CONDITIONS : memory allocation faults.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *                        2/6/96 max_slices initialized to 2 Dewey Odhner.
 *                        Modified 2/21/00 unresized voxels allowed
 *                           by Dewey Odhner.
 *                        Modified 10/3/02 chunks expanded by Dewey Odhner.
 *
 ************************************************************************/
int SetupMedValues(float ratio)
{
  

  int num_chunks,i,max_slices,v,w,h;


  max_slices=2;
  for(v=0;v<in.num_volumes;v++) {
    if (ratio > 0)
	  med[v].Pz=med[v].Py=med[v].Px= (float)(in.vh.scn.xypixsz[0]*ratio/2.0);
    else
	{
	  med[v].Pz = (float)(in.Pz/2.0);
	  med[v].Py = (float)(in.Py/2.0);
	  med[v].Px = (float)(in.Px/2.0);
	}
	med[v].width= (short)(2*rint(in.Px*in.width*0.5/med[v].Px)+1);
    med[v].height=(short)(2*rint(in.Py*in.height*0.5/med[v].Py)+1);
    w= med[v].width>>1; h=med[v].height>>1;
    med[v].bytes_per_slice=w*h;
    med[v].slices=(short)(2*rint(in.Pz*in.slices[v]*0.5/med[v].Pz)+1);
    if (med[v].slices > max_slices)
      max_slices=med[v].slices;
    med[v].volume=med[v].num_TSE=0;
    med[v].max_x=0;med[v].min_x=0x7FFF;
    med[v].max_y=0;med[v].min_y=0x7FFF;
    med[v].max_z=0;med[v].min_z=0x7FFF;
    if ((med[v].NTSE=(short *)calloc(med[v].height*med[v].slices,sizeof(short)))==NULL) {
      printf("Could not Allocate space for NTSE");
      return (-1);
    }
  }
  
  
  interp.xsize=med[0].width;
  interp.ysize=med[0].height;
  interp.zsize=max_slices;
  if (CHUNK_SIZE> interp.zsize) CHUNK_SIZE=interp.zsize;
  interp.ChunkSlices=CHUNK_SIZE+4;
  num_chunks = (int)ceil((double)(interp.zsize-1)/(CHUNK_SIZE-1));
  interp.x_table=(short *)calloc(sizeof(short),interp.xsize);
  interp.y_table=(short *)calloc(sizeof(short),interp.ysize);
  interp.z_table=(short *)calloc(sizeof(short),interp.zsize);
  if (in.Pz <= med[0].Pz) {
    interp.z_flag=0;
    interp.z_dist=NULL;
    for(i=0;i<interp.zsize;i++) 
      interp.z_table[i]= (short)rint((2*i*med[0].Pz-in.Pz)*0.5/in.Pz);
  }
  else {
    interp.z_flag=1;
    interp.z_dist=(float *)calloc(sizeof(float),interp.zsize);
    for(i=0;i<interp.zsize;i++)  {
      interp.z_dist[i] = (float)((2*i*med[0].Pz-in.Pz)*0.5/in.Pz);
      interp.z_table[i]= (short)floor(interp.z_dist[i]);
      interp.z_dist[i] -= interp.z_table[i];
    }
  }
  if (in.height >= interp.ysize) {
    interp.y_flag=0;
    interp.y_dist=NULL;
    for(i=0;i<interp.ysize;i++) 
      interp.y_table[i]= (short)rint((2*i*med[0].Py-in.Py)*0.5/in.Py);
  }
  else {
    interp.y_flag=2;
    interp.y_dist=(float *)calloc(sizeof(float),interp.ysize);
    for(i=0;i<interp.ysize;i++)  {
      interp.y_dist[i] = (float)((2*i*med[0].Py-in.Py)*0.5/in.Py);
      interp.y_table[i]= (short)floor(interp.y_dist[i]);
      interp.y_dist[i] -= interp.y_table[i];
    }
  }

  if (in.width >= interp.xsize) {
    interp.x_flag=0;
    interp.x_dist=NULL;
    for(i=0;i<interp.xsize;i++) 
      interp.x_table[i]= (short)rint((2*i*med[0].Px-in.Px)*0.5/in.Px);
  }
  else {
    interp.x_flag=4;
    interp.x_dist=(float *)calloc(sizeof(float),interp.xsize);
    for(i=0;i<interp.xsize;i++)  {
      interp.x_dist[i] = (float)((2*i*med[0].Px-in.Px)*0.5/in.Px);
      interp.x_table[i]= (short)floor(interp.x_dist[i]);
      interp.x_dist[i] -= interp.x_table[i];
    }
  }
  
  if ((interp.ChunkOffsets= (short *)calloc(sizeof(short),num_chunks))==NULL) {
    printf("Could not Allocate space for the Chunk Offsets\n");
    return(-1);
  }    
  if ((in.ChunkSlices=(short *)calloc(sizeof(short),num_chunks))==NULL) {
    printf("Could not Allocate space for the Chunk Offsets\n");
    return(-1);
  }
  if ((in.ChunkOffsets= (short *)calloc(sizeof(short),num_chunks))==NULL) {
    printf("Could not Allocate space for the Chunk Offsets\n");
    return(-1);
  }

  for(i=0;i<num_chunks;i++)
    interp.ChunkOffsets[i]=i*(CHUNK_SIZE-1) -2;

  MAX_IN_CHUNK=0;
  for(i=0;i<num_chunks;i++) {
    if (interp.ChunkOffsets[i] <0)
      in.ChunkOffsets[i]= -1;
    else
      in.ChunkOffsets[i]= interp.z_table[interp.ChunkOffsets[i]];
    if (interp.ChunkOffsets[i] + interp.ChunkSlices+1 >= interp.zsize)
      in.ChunkSlices[i]=in.slices[0]-in.ChunkOffsets[i]+1;
    else 
      in.ChunkSlices[i]= interp.z_table[interp.ChunkOffsets[i] + interp.ChunkSlices+1] -
	in.ChunkOffsets[i] + 2;
    if (in.ChunkSlices[i] > MAX_IN_CHUNK)
      MAX_IN_CHUNK=in.ChunkSlices[i];
  }


  
  
      
  /* Add room for one previous and next slice */
  if ((in.Chunk=(unsigned char *)calloc(in.bytes_per_slice*MAX_IN_CHUNK,1))==NULL) {
    printf("Could not Allocate space for the Chunks\n");
    return(-1);
  }

  return(num_chunks);

}





/************************************************************************
 *
 *      FUNCTION        : CheckUniformSpacing()
 *
 *      DESCRIPTION     : Check if the input data has uniform slice
 *                        spacings.
 *
 *      RETURN VALUE    : 0 - uniformally spaced.
 *                       -1 - otherwise.
 *
 *      PARAMETERS      : scn - SceneInfo strcutre of the input data
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : in should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
int CheckUniformSpacing(SceneInfo *scn)
{

  int i,j,skip;
  double tolarance;

  if (scn->dimension==3) {
    in.slices[0]=scn->num_of_subscenes[0];
    in.slice_locs[0]=scn->loc_of_subscenes[0];
    in.Pz=scn->loc_of_subscenes[1]-scn->loc_of_subscenes[0];
    in.skip_slices[0]=0;
    tolarance= in.Pz/200.0;
    for(j=0;j<in.slices[0]-1;j++) 
      if (fabs(fabs(scn->loc_of_subscenes[j+1]-
		    scn->loc_of_subscenes[j])-in.Pz)> 
	  tolarance) {
	printf("The slices are not uniformally spaced\n");
	printf("Use interpolate to make them uniform first\n");
	return(-1);
      }

  }
  else {
    skip=in.num_volumes;
    in.Pz=scn->loc_of_subscenes[skip+1]-scn->loc_of_subscenes[skip];
    tolarance=in.Pz/200.0;
    for(i=0;i<in.num_volumes;i++) {
      in.slices[i]=scn->num_of_subscenes[1+i];
      in.slice_locs[i]=scn->loc_of_subscenes[skip];
      in.skip_slices[i]= skip - in.num_volumes;
      for(j=0;j<in.slices[i]-1;j++) 
	if (fabs(fabs(scn->loc_of_subscenes[skip+j+1]-
		      scn->loc_of_subscenes[skip+j])-in.Pz)> 
	    tolarance) {
	  printf("diff is %lf\n",fabs(fabs(scn->loc_of_subscenes[skip+j+1]-
					   scn->loc_of_subscenes[skip+j])-in.Pz));
	  printf("The slices are not uniformally spaced\n");
	  printf("Use interpolate to make them uniform first\n");
	  return(-1);
	}
      skip +=in.slices[i];
    }
  }
  
  return(0);

}


/************************************************************************
 *
 *      FUNCTION        : UnpackData()
 *
 *      DESCRIPTION     : Unpack a binary data set (1 bit pixel)
 *                        to make it 8 bit. 0ff bits will be mapped to 
 *                        low and On bits will be mapped to hi.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : 
 *               temp_data : packed data set pointer.
 *               data      : output data pointer.
 *               elem      : number of data elements.
 *               low       : mapping of the Off pixel.
 *               hi        : mapping of the On pixel.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : temp_slice should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *
 ************************************************************************/
void UnpackData(unsigned char *temp_slice, unsigned char *data,
	int elems, int low, int hi)
{
  static unsigned char bit_field[8]= { 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };
  int cur,i;

  cur=0;
  while (cur<elems) {
    for(i=0;i<8 && cur<elems;data++,cur++,i++)
      *data = ( (*temp_slice & bit_field[i]) ? hi : low );
    temp_slice++;
  }

}









