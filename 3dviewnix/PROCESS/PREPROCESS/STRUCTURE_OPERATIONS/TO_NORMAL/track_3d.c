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
#include <cv3dv.h>
#include "neighbors.h"
#if ! defined (WIN32) && ! defined (_WIN32)
 #include <unistd.h>
#endif

#define QSIZE 25000
#define INVALID_NORM (6<< 2*G_COMPONENT_BITS)
#define BINVALID_NORM (6<< 2*BG_COMPONENT_BITS)

#define PX_type 0
#define PY_type 1
#define PZ_type 2
#define NX_type 3
#define NY_type 4
#define NZ_type 5

static unsigned char bit_pos[8]= { 0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01 };
/* CHUNK_SIZE should always be odd (>=3) and gives the number of slices
   in the output resolution (half voxel steps) */
static int CHUNK_SIZE=25;


typedef unsigned short TSE_type[2];
#define TEMP_Z_NORM (unsigned short)0x0377
#define TEMP_Y_NORM (unsigned short)0x0477
#define TEMP_X_NORM (unsigned short)0x0577

extern unsigned short VoxOn(double min_thresh, double max_thresh, int chunk,
    int slice, int row, int col),
  G_code(),BG_code();

void Calculate_Norm0(unsigned short *norm, int vol, int sl, int row, int col, int B);

static struct IN {
  char *filename;
  FILE *fp;
  ViewnixHeader vh;
  int vh_len,skip_slices;
  unsigned short start,end;
  short width,height,slices,volume;
  unsigned int bytes_per_slice,bits_per_pixel;
  double Px,Py,Pz,slice_loc;
  short *ChunkOffsets,*ChunkSlices;
  unsigned char *Chunk;
} in;

static struct MED {
  char *filename;
  FILE *fp;
  short width,height,slices;
  unsigned int bytes_per_slice,bits_per_pixel,num_TSE;
  short *NTSE,min_x,max_x,min_z,max_z,min_y,max_y;
  double Px,Py,Pz;
  short *ChunkOffsets,ChunkSlices;
  int volume;
  unsigned char *Chunk;
} med;

static struct INTERPOLATE {
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

typedef int StartPoint[3];



typedef unsigned short Qelem[4]; /*
				   0 - face type
				   1 - colunm
 				   2 - row
				   3 - slice  */
typedef struct {
  Qelem Q[QSIZE];
  unsigned short push_at,pop_at;
} Qtype;


static int min_x,min_y,min_z,max_x,max_y,max_z;
static int bg_flag,param_on;
static char param_file[400];

int getpid(),
  InVoxVal(int chunk, int in_slice, int in_row, int in_col),
  InitChunks(Qtype **QC),
  ReadStartPointList(char *pt_file, StartPoint **list),
  get_starting_point(int num_chunks, int slice, int row, int col,
      double min_thresh, double max_thresh, short st[5]),
  PopQ(Qtype *QChunks, int chunk, short st[5]),
  ReadChunk(int chunk),
  WriteChunk(int chunk),
  GetNextNonEmptyQ(Qtype *QChunks, int num_chunks);
void (*Calculate_Norm)(),Calculate_Norm0(),
  Calculate_Norm8(),Calculate_Norm26(),
  InitGeneralHeader(short type),
  InitShell1(),
  InitShell0(),
  get_min_max_values(short *max_y, short *min_y, short *max_z, short *min_z),
  WriteTSE_SHELL1(int chunks),
  WriteTSE_SHELL0(int chunks),
  Create_Bin_Slice(unsigned char *sl, int num),
  PushQ(Qtype *QChunks, short st[5]),
  SetupMedValues(float ratio);
extern float VoxVal(int chunk, int slice, int row, int col);

/************************************************************************
 *
 *      FUNCTION        : main
 *
 *      DESCRIPTION     : This program takes a scene and and a set 
 *                        of stating points and generates a
 *                        BS1, BS0 or a BSI surface file. The valid
 *                        scene must have 1,8,16 bit data and have at 
 *                        least 2 slices. The output structure will
 *                        always have cubic voxels (faces). The tracking
 *                        algorithm is chunk based. connected bounday 
 *                        segment of on chunk is tracked competely before
 *                        moving on to the next chunk. Within a chunk a
 *                        Queue is used to keep track of the faces to
 *                        track next.
 *
 *      RETURN VALUE    : 0 on normal completion.
 *
 *      PARAMETERS      : 11 essential and 1 optional parameter.
 *          argumnet 1 : The name of this process.
 *          argument 2 : Input scene. (IM0 or BIM file)
 *          argument 3 : Output structure file (BS0,BSI or BS1 file)
 *          argument 4 : face size in units of the input scenes 
 *                       pixel width.
 *          argument 5 &
 *          argument 6 : minimum and maximum thresholds used for selecting
 *                       the intensity region of interest (inclusive of both
 *                       the min and the max values).
 *          argumnet 7 : Current Volume of Interest (0 if 3D)
 *          argumnet 8 : Strating points file.
 *                       Format:
 *       	           num_of_points
 *                         z1 y1 x1
 *                         z2 y2 x2
 *                         :  :  :
 *          argument 9 : Surfaced normal Type. (8 or 26 Neighbor normals)
 *                       8 neighbor normal cosiders intensity values at the
 *                       vertices of a cubic voxel with the same resoluton
 *                       as the output surface to compute the normal.
 *          argument 10 : Merge Flag. 
 *                       0 - Create New Structure System.
 *                       1 - Append to the Current Sturcuture system 
 *                           if one is available or crete a new one.
 *          argument 11: Background Flag.
 *                       0 - running in the foreground.
 *                       1 - running in the backgroung.
 *          argument 12: Paramemter vector specification.(OPTIONAL)
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
 *                        InitChunks,ReadStartPointList,
 *                        get_starting_point,PushQ,PopQ,VoxOn,
 *                        ReadChunk,WriteChunk,GetNextNonEmptyQ,
 *                        InitGeneralHeader,InitShell1,InitShell0,
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 8/16/07 param_file deleted by Dewey Odhner.
 *
 ************************************************************************/			   
int main(argc,argv)
int argc;
char *argv[];
{

  Qtype *QChunks;
  StartPoint *st_list;
  int norm_size,i,CPOINT,num_chunks,merge_flag, j, k;
  int CURRENT_CHUNK,error,code,num_start_pt;
  short st[5],nxt[5];
  char grp[6],elem[6],*ptr, *exec_str, *tempname;
  float ratio;
  double min_thresh,max_thresh;
  double tolarance;

  if (argc!=11 && argc!=12) {
    printf("Usage: %s <IMAGE_file> <BSn_file> pix_size min_thresh max_thresh volume <pt_file> normal merge_flag bg_flag [pv_vector_file]\n",argv[0]);
    printf("IMAGE_file - This can be a binary or gray scene\n");
    printf("BSn_file   - This can be BS1 or BS0\n");
    printf("pix_size   - In units of the input pixel width.\n");
    printf("min_thresh max_thresh - Has to be between or equal to these values\n");
    printf("volume     - if 4D () the volume number (>=0) if 3D 0.\n");
    printf("pt_file    - file containig starting points\n");
    printf("           format:  num_points\n");
    printf("                    z1 y1 x1\n");
    printf("                    z2 y2 x2\n");
    printf("                    :  :  :\n");
    printf("normal     - Can be 8 or 26\n");
    printf("merge_flag - Merge new stucture to the one specified\n");
    printf("bg_flag    - 1 = running in the background");
    printf("             0 = running in the foreground");
    printf("pv_file    - file containing the description of param vectors\n");
    printf("format:    num_pv\n");
    printf("           description_pv1 description_pv2 .. .. ..\n");
    printf("           pv1 pv2 .. .. \n");
    exit(-1);
  }
  if (argc==12) {
    param_on=TRUE;
    strcpy(param_file,argv[11]);
  }
  else
    param_on=FALSE;

  bg_flag=atoi(argv[10]);
  if (bg_flag) 
    VAddBackgroundProcessInformation(argv[0]);
  merge_flag=atoi(argv[9]);

  if (!strcmp(argv[8],"26")) {
    Calculate_Norm=Calculate_Norm26; norm_size=26;
  }
  else if (!strcmp(argv[8],"8")) {
    Calculate_Norm=Calculate_Norm8; norm_size=8;
  }
  else if (!strcmp(argv[8],"0")) {
    Calculate_Norm=Calculate_Norm0; norm_size=0;
  }
  else  {
    printf("Only valid types for normals are 8 and 26\n");
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
  if (error) {
    printf("Read Error %d ( group:%s element:%s )\n",error,grp,elem);
    if (error!=106 && error!=107) {
      printf("Cannot revoer from error\n");
      exit(-1);
    }
  }

  /* Setup Input structure */
  if (in.vh.scn.dimension>4) {
    printf("Cannot handle %d data\n",in.vh.scn.dimension);
    exit(-1);
  }
  
  min_thresh=atof(argv[4]);
  max_thresh=atof(argv[5]);
  in.volume=atoi(argv[6]);
  
  /*
  if (argc==9) {
    in.start=atoi(argv[7]);
    in.end=atoi(argv[8]);
    if (in.vh.scn.dimension==3 && in.end>in.vh.scn.num_of_subscenes[0])
      in.end=in.vh.scn.num_of_subscenes[0];
    else if (in.end>in.vh.scn.num_of_subscenes[1+in.volume])
      in.end=in.vh.scn.num_of_subscenes[1+in.volume]; 
  }
  */
  
  in.start=0;
  if (in.vh.scn.dimension==3) 
    in.end=in.vh.scn.num_of_subscenes[0]-1; /* number of slices */
  else 
    in.end=in.vh.scn.num_of_subscenes[1]-1; /* slices in 1st volume */
  

  in.width=in.vh.scn.xysize[0]; in.height=in.vh.scn.xysize[1];
  in.slices=in.end-in.start+1;
  VGetHeaderLength(in.fp,&in.vh_len);
  in.bits_per_pixel=in.vh.scn.num_of_bits;
  in.bytes_per_slice= (in.width*in.height*in.vh.scn.num_of_bits+7)/8;
  in.Px=in.vh.scn.xypixsz[0];
  in.Py=in.vh.scn.xypixsz[1];
  if (in.vh.scn.dimension==3) {
    in.Pz=fabs((double)(in.vh.scn.loc_of_subscenes[in.start+1]-
			in.vh.scn.loc_of_subscenes[in.start]));
    tolarance= in.Pz/200.0;
    in.slice_loc=in.vh.scn.loc_of_subscenes[in.start];
    for(i=in.start;i<in.end;i++)
      if (fabs(fabs((double)(in.vh.scn.loc_of_subscenes[i+1]-
			     in.vh.scn.loc_of_subscenes[i]))-(double)in.Pz)
	  > tolarance) {
	printf("The slices need to be equally spaced\n");
	exit(-1);
      }
  }
  else {
    in.skip_slices=0;
    for(i=0;i<in.volume;i++) 
      in.skip_slices+=in.vh.scn.num_of_subscenes[i+1];
    in.Pz=
      in.vh.scn.loc_of_subscenes[in.vh.scn.num_of_subscenes[0]+
				 in.skip_slices+in.start+1]-
				   in.vh.scn.loc_of_subscenes[in.vh.scn.num_of_subscenes[0]+
							      in.skip_slices+in.start];
    in.slice_loc= in.vh.scn.loc_of_subscenes[in.vh.scn.num_of_subscenes[0]+
					     in.skip_slices+in.start];
    tolarance= in.Pz/200.0;
    for(i=in.vh.scn.num_of_subscenes[0]+in.skip_slices+in.start;i<in.vh.scn.num_of_subscenes[0]+in.skip_slices+in.end;i++)
      if (fabs(fabs(in.vh.scn.loc_of_subscenes[i+1]-in.vh.scn.loc_of_subscenes[i])-
	       in.Pz)
	  > tolarance) {
	printf("The slices need to be equally spaced\n");
	exit(-1);
      }
  }

  if (in.bits_per_pixel!=1) 
   Calculate_Norm= Calculate_Norm0;

  /* Setup intermidiate stucture */
  med.filename = (char *)malloc(24+strlen(argv[2]));
  sprintf(med.filename,"%sTRACK3D_%d.TEMP", argv[2], getpid());
  if ((med.fp=fopen(med.filename,"w+b"))==NULL) {
    printf("Could not open temp file %s for computations\n", med.filename);
    exit(-1);
  }
  if (sscanf(argv[3], "%f", &ratio) != 1) {
    printf("Specify magnification Correctly\n");
    exit(-1);
  }
  SetupMedValues(ratio);  



  /* Setup output structure */
  out.filename = (char *)malloc(strlen(argv[2])+1);
  sprintf(out.filename, "%s", argv[2]);
  ptr=strrchr(out.filename,'.');
  if (ptr==NULL) {
    printf("Output file should have BS0, BSI or BS1 extension\n");
    exit(-1);
  }
  out.tempname = (char *)malloc(20+strlen(ptr));
  sprintf(out.tempname,"TEMP%dSHELL%s",getpid(),ptr);  
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
  else {
    printf("Output file should have BS0, BSI or BS1 extension\n");
    exit(-1);
  }
    


  num_chunks=InitChunks(&QChunks);
  if (num_chunks==-1) return(-1);


  num_start_pt=ReadStartPointList(argv[7],&st_list);
  /* Gives the starting face as a [4]chunk, [3]slice, [2]row, [1]col, [0]face_type */
  if (num_start_pt==0) {
    return(0);
  }
  CPOINT=0;
  while (CPOINT<num_start_pt) {
    /* This would get a stating point and read in the corresponding slice */
    CURRENT_CHUNK=get_starting_point(num_chunks,st_list[CPOINT][2],st_list[CPOINT][1],
				     st_list[CPOINT][0],min_thresh,max_thresh,st);

    /* Push Current Face into the Q */
    if (CURRENT_CHUNK!=-1) 
      PushQ(QChunks,st);
    
    while (CURRENT_CHUNK!=-1) {
      while (PopQ(QChunks,CURRENT_CHUNK,st)) {
	/* Get Next face(s) */
	switch (st[0]) {
	case PX_type:
	  /* row based circuit */
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]+2,st[2],st[1]-1)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]+2,st[2],st[1]+1);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]+1; nxt[2]=st[2]; nxt[1]=st[1]-1; nxt[0]=PZ_type; break;
	  case 1:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]+1; nxt[2]=st[2]; nxt[1]=st[1]+1; nxt[0]=NZ_type; break;
	  case 2:
	    nxt[4]=st[4]; nxt[3]=st[3]+2; nxt[2]=st[2]; nxt[1]=st[1];   nxt[0]=PX_type; break;
	  }
	  PushQ(QChunks,nxt);
	  
	  /* slice based circuit */
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]+2,st[1]-1)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]+2,st[1]+1);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]+1; nxt[1]=st[1]-1; nxt[0]=PY_type; break;
	  case 1:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]+1; nxt[1]=st[1]+1; nxt[0]=NY_type; break;
	  case 2:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]+2; nxt[1]=st[1];   nxt[0]=PX_type; break;
	  }
	  PushQ(QChunks,nxt);
	  break;
	  
	case NX_type:
	  /* row based circuit */
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]-2,st[2],st[1]-1)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]-2,st[2],st[1]+1);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]-1; nxt[2]=st[2]; nxt[1]=st[1]+1; nxt[0]=NZ_type; break;
	  case 1:
	    nxt[4]=st[4]; nxt[3]=st[3]-2; nxt[2]=st[2]; nxt[1]=st[1];   nxt[0]=NX_type; break;
	  case 2:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]-1; nxt[2]=st[2]; nxt[1]=st[1]-1; nxt[0]=PZ_type; break;
	  }
	  PushQ(QChunks,nxt);
	  
	  /* slice based circuit */
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]-2,st[1]-1)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]-2,st[1]+1);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]-1; nxt[1]=st[1]+1; nxt[0]=NY_type; break;
	  case 1:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]-2; nxt[1]=st[1];   nxt[0]=NX_type; break;
	  case 2:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]-1; nxt[1]=st[1]-1; nxt[0]=PY_type; break;
	  }
	  PushQ(QChunks,nxt);
	  break;
	  
	case PY_type: /* Positive Y Face */
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]-1,st[1]-2)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]+1,st[1]-2);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]-1; nxt[1]=st[1]-1; nxt[0]=NX_type; break;
	  case 1:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]+1; nxt[1]=st[1]-1; nxt[0]=PX_type; break;
	  case 2:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2];   nxt[1]=st[1]-2; nxt[0]=PY_type; break;
	  }
	  PushQ(QChunks,nxt);
	  break;
	  
	case NY_type:
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]-1,st[1]+2)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3],st[2]+1,st[1]+2);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]+1; nxt[1]=st[1]+1; nxt[0]=PX_type; break;
	  case 1:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2];   nxt[1]=st[1]+2; nxt[0]=NY_type; break;
	  case 2:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]; nxt[2]=st[2]-1; nxt[1]=st[1]+1; nxt[0]=NX_type; break;
	  }
	  PushQ(QChunks,nxt);
	  break;
	  
	case PZ_type:
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]-1,st[2],st[1]-2)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]+1,st[2],st[1]-2);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]-1; nxt[2]=st[2]; nxt[1]=st[1]-1; nxt[0]=NX_type; break;
	  case 1:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]+1; nxt[2]=st[2]; nxt[1]=st[1]-1; nxt[0]=PX_type; break;
	  case 2:
	    nxt[4]=st[4]; nxt[3]=st[3];   nxt[2]=st[2]; nxt[1]=st[1]-2; nxt[0]=PZ_type; break;
	  }
	  PushQ(QChunks,nxt);
	  break;
	  
	case NZ_type:
	  code= (VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]-1,st[2],st[1]+2)<<1) +
	    VoxOn(min_thresh,max_thresh,CURRENT_CHUNK,st[3]+1,st[2],st[1]+2);
	  switch (code) {
	  case 0:
	    nxt[4]=st[4]; nxt[3]=st[3]+1; nxt[2]=st[2]; nxt[1]=st[1]+1; nxt[0]=PX_type; break;
	  case 1:
	    nxt[4]=st[4]; nxt[3]=st[3];   nxt[2]=st[2]; nxt[1]=st[1]+2; nxt[0]=NZ_type; break;
	  case 2:
	  case 3:
	    nxt[4]=st[4]; nxt[3]=st[3]-1; nxt[2]=st[2]; nxt[1]=st[1]+1; nxt[0]=NX_type; break;
	  }
	  PushQ(QChunks,nxt);
	  break;
	  
	}
      }
      if (WriteChunk(CURRENT_CHUNK)==-1) {
	exit(-1);
      }
      CURRENT_CHUNK=GetNextNonEmptyQ(QChunks,num_chunks);
      printf("Tracking on Chunk %d/%d\n",CURRENT_CHUNK+1,num_chunks);
      fflush(stdout);
      if (CURRENT_CHUNK!=-1)  
	if (ReadChunk(CURRENT_CHUNK)==-1) {
	  exit(-1);
	}
    } /* done tracking boundary of that point*/
    CPOINT++;
  } /* Get Next Point */
  
  printf("Done tracking. %d faces found\n",med.num_TSE);
  if (med.num_TSE!=0) {
    InitGeneralHeader(out.file_type);
    switch (out.file_type) {
    case SHELL0:
      InitShell0();
      error=VWriteHeader(out.fp,&out.vh,grp,elem);
      if (error) {
	printf("Write Error %d (group:%s element:%s\n",error,grp,elem);
	if (error!=106 && error!=107) {
	  printf("Cannot revoer from error\n");
	  exit(-1);
	}
      }
      WriteTSE_SHELL0(num_chunks);
      break;
    case SHELL1:
      InitShell1();
      printf("Volume in output voxels is %f\n",out.vh.str.volume[0]);
      error=VWriteHeader(out.fp,&out.vh,grp,elem);
      if (error) {
	printf("Write Error %d (group:%s element:%s\n",error,grp,elem);
	if (error!=106 && error!=107) {
	  printf("Cannot revoer from error\n");
	  exit(-1);
	}
      }
      WriteTSE_SHELL1(num_chunks);
      break;
    }
  }
  else {
    printf("No Surface generated\n");
  }
  fclose(in.fp);
  fclose(med.fp);
  fclose(out.fp);
  /* Remove Temp file */
  unlink(med.filename);

  exec_str = (char *)malloc(36+strlen(out.tempname)+strlen(argv[1])+strlen(argv[8]));

  if (in.bits_per_pixel!=1 && norm_size!=0) {
    sprintf(exec_str,"to_normal \"%s\" 1 \"%s\" %d \"%s\" 0",
	    out.tempname, argv[1], in.volume, argv[8]);
    error=system(exec_str);
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
 *                        Modified: 7/4/09 st->domain corrected
 *                         by Dewey Odhner.
 *
 ************************************************************************/
void InitShell1()
{

  int i,elem;
  StructureInfo *st;
  int slices;
  FILE *pfp;

  st= &out.vh.str;

  st->dimension=3;
  st->dimension_valid=1;


  st->num_of_structures=1;
  st->num_of_structures_valid=1;


  st->domain=(float *)malloc(sizeof(float)*12);
  if (in.vh.scn.dimension==3) {
    for(i=0;i<12;i++)
      st->domain[i]=in.vh.scn.domain[i];
  }
  else   /* 4D */
    for(i=0;i<4;i++) {
      st->domain[i*3]=in.vh.scn.domain[i*4];
      st->domain[i*3+1]=in.vh.scn.domain[i*4+1];
      st->domain[i*3+2]=in.vh.scn.domain[i*4+2];
    }
  st->domain[0] = (float)(st->domain[0] - st->domain[3]*med.Px +
      st->domain[6]*med.Py +
      st->domain[9]*(med.Pz-in.slice_loc));
  st->domain[1] = (float)(st->domain[1] - st->domain[4]*med.Px +
      st->domain[7]*med.Py +
      st->domain[10]*(med.Pz-in.slice_loc));
  st->domain[2] = (float)(st->domain[2] - st->domain[5]*med.Px +
      st->domain[8]*med.Py +
      st->domain[11]*(med.Pz-in.slice_loc));
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
    st->scene_file=(Char30 *)malloc(sizeof(Char30));
    strncpy((char *)st->scene_file[0],in.filename, sizeof(Char30));
    st->scene_file[0][sizeof(Char30)-1]=0;
    st->scene_file_valid=1;
  }
  else if (in.vh.gen.filename1_valid) {
    st->scene_file=(Char30 *)malloc(sizeof(Char30));
    strncpy((char *)st->scene_file[0],in.vh.gen.filename1, sizeof(Char30));
    st->scene_file[0][sizeof(Char30)-1]=0;
    st->scene_file_valid=1;
  }
  else {
    st->scene_file=(Char30 *)malloc(sizeof(Char30));
    strncpy((char *)st->scene_file[0],in.filename, sizeof(Char30));
    st->scene_file[0][sizeof(Char30)-1]=0;
    st->scene_file_valid=1;
  }



  st->num_of_TSE=(unsigned int *)malloc(sizeof(int));
  st->num_of_TSE[0]=med.num_TSE;
  st->num_of_TSE_valid=1;
  
  get_min_max_values(&med.max_y,&med.min_y,&med.max_z,&med.min_z);
  
  st->num_of_NTSE=(unsigned int *)malloc(sizeof(int));
  st->num_of_NTSE[0]=1+(med.max_z-med.min_z+1)*(med.max_y-med.min_y+2);
  st->num_of_NTSE_valid=1;
  
  st->num_of_components_in_TSE=9; /* ncode,y1,tt,n1,n2,n3,gm,op,og */
  st->num_of_components_in_TSE_valid=1;
  
  st->num_of_components_in_NTSE=1;
  st->num_of_components_in_NTSE_valid=1;
  
  st->smallest_value=(float *)malloc(sizeof(float)*9);
  st->largest_value=(float *)malloc(sizeof(float)*9);

  st->smallest_value[0]=0;   /* ncode */
  st->largest_value[0]=1;
  st->smallest_value[1]=med.min_x; /* y1 */
  st->largest_value[1]=med.max_x;
  st->smallest_value[2]=st->largest_value[2]=0.0;
  st->smallest_value[3]=0.0; st->largest_value[3]=7.0; /* normals */
  st->smallest_value[4]=0.0; st->largest_value[4]=15.0;
  st->smallest_value[5]=0.0; st->largest_value[5]=15.0;
  st->smallest_value[6]=st->largest_value[6]=0.0;
  st->smallest_value[7]=st->largest_value[7]=0.0;
  st->smallest_value[8]=st->largest_value[8]=0.0;
  st->smallest_value_valid=1;
  st->largest_value_valid=1;

  st->num_of_integers_in_TSE=9;
  st->num_of_integers_in_TSE_valid=1;
  
  st->signed_bits_in_TSE=(short *)malloc(sizeof(short)*9);
  st->signed_bits_in_TSE[0]=0;
  st->signed_bits_in_TSE[1]=0;
  st->signed_bits_in_TSE[2]=0;
  st->signed_bits_in_TSE[3]=1;
  st->signed_bits_in_TSE[4]=1;
  st->signed_bits_in_TSE[5]=1;
  st->signed_bits_in_TSE[6]=st->signed_bits_in_TSE[7]=st->signed_bits_in_TSE[8]=0;
  st->signed_bits_in_TSE_valid=1;

  
  st->num_of_bits_in_TSE=32;
  st->num_of_bits_in_TSE_valid=1;

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

  slices=(med.slices<<1)|1;
  st->num_of_samples=(short *)malloc(sizeof(short));
  st->num_of_samples[0]=slices;
  st->num_of_samples_valid=1;
 
  st->xysize[0]= (float)med.Px;
  st->xysize[1]= (float)med.Py;
  st->xysize_valid=1;
 
  st->loc_of_samples=(float *)malloc(sizeof(float)*slices);
  st->loc_of_samples[0]=0.0;
  for(i=1;i<slices;i++) {
    st->loc_of_samples[i]= (float)(i*med.Pz);
  }
  st->loc_of_samples_valid=1;

  if (!param_on) {
    if (in.vh.scn.dimension==3) {
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
      st->parameter_vectors=(float *)malloc(sizeof(float)*2);
      st->parameter_vectors[0]= (float)in.volume;
      st->parameter_vectors[1]= 0.0;
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
      st->description_of_element=(short *)malloc(st->num_of_elements*sizeof(short));
      st->parameter_vectors=(float *)malloc(st->num_of_elements*sizeof(float));
      for(i=0;i<st->num_of_elements;i++)
	if (fscanf(pfp,"%d",&elem)!=1) {
	  printf("Could not read parameters from file\n");
	  st->num_of_elements_valid=0;
	  st->description_of_element_valid=0;
	  st->parameter_vectors_valid=0;
	  break;
	}
	else
	  st->description_of_element[i]=elem;
      if (st->parameter_vectors_valid!=0) {
	for(i=0;i<st->num_of_elements;i++)
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
  
  st->min_max_coordinates=(float *)malloc(sizeof(float)*6);
  st->min_max_coordinates[0] = (float)(med.Px*med.min_x);
  st->min_max_coordinates[1] = (float)(med.Py*med.min_y);
  st->min_max_coordinates[2] = (float)(med.Pz*med.min_z);
  st->min_max_coordinates[3] = (float)(med.Px*med.max_x);
  st->min_max_coordinates[4] = (float)(med.Py*med.max_y);
  st->min_max_coordinates[5] = (float)(med.Pz*med.max_z);
  st->min_max_coordinates_valid=1;

  st->volume=(float *)malloc(sizeof(float));
  st->volume[0]= (float)(med.volume*med.Px*med.Py*med.Pz*4);
  st->volume_valid=1;

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
 *      History         : 07/12/1993 Supun Samarasekera                  
 *
 ************************************************************************/
void get_min_max_values(short *max_y, short *min_y, short *max_z, short *min_z)
{
  short *ptr;
  int i,j;

  *min_y = *min_z = 0x7FFF;
  *max_y = *max_z=0;
  for(ptr=med.NTSE,i=0;i<2*med.slices+1;i++)
    for(j=0;j<2*med.height+1;j++,ptr++) {
      if (*ptr) {
	if (i > *max_z) *max_z=i;
	if (j > *max_y) *max_y=j;
	if (i < *min_z) *min_z=i;
	if (j < *min_y) *min_y=j;
      }
    }
  
}


/************************************************************************
 *
 *      FUNCTION        : WriteTSE_SHELL1()
 *
 *      DESCRIPTION     : Given  the  number of chunks the that have been
 *                        tracked generate the TSE for SHELL1 data.
 *
 *      RETURN VALUE    : None.
 * 
 *      PARAMETERS      : 1 parameter.
 *                 chunks : number of chunks the scene is devided into.
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
 *
 ************************************************************************/
void WriteTSE_SHELL1(int chunks)
{
  int i,j,k,l,num_TSE,sum,size,slice,row,num;
  short *ptr,temp;
  unsigned char *sl1,*sl2,*sl_temp,*slp1,*slp2,*slp3;
  TSE_type *TSE;

  sum =0;
  fseek(out.fp,0L,2L);
  /* Write out the NTSE of the shell */
  /* Top level */
  temp= med.max_z-med.min_z+1;
  if (VWriteData((char *)&temp,2,1,out.fp,&num)) {
    printf("Could not write NTSE\n");
    exit(-1);
  }
  /* 2nd level */
  for(i=med.min_z,temp=med.max_y-med.min_y+1;i<=med.max_z;i++)
    if (VWriteData((char *)&temp,2,1,out.fp,&num)) {
      printf("Could not write NTSE\n");
       exit(-1);
    }
  /* 3rd level */
  for(i=med.min_z;i<=med.max_z;i++) {
    ptr=med.NTSE+i*(2*med.height+1)+med.min_y;
    if (VWriteData((char *)ptr,2,(med.max_y-med.min_y+1),out.fp,&num)) {
      printf("Could not write NTSE\n");
       exit(-1);
    }
  }
  TSE=(TSE_type *)malloc(sizeof(TSE_type)*(med.max_x-med.min_x+1));
  if (TSE==NULL) {
    printf("Could Not Allocate space for TSE's\n");
     exit(-1);
  }
  size= (med.width-1)*med.height;
  if ((sl1=(unsigned char *)malloc(size))==NULL) {
    printf("Could not allocate space for slices\n");
     exit(-1);
  }
  if ((sl2=(unsigned char *)malloc(size))==NULL) {
    printf("Could not allocate space for slices\n");
     exit(-1);
  }


  for(i=0;i<chunks;i++) {
    ReadChunk(i);
    Create_Bin_Slice(sl2,0);
    printf("Processing slice 1/%d of chunk %d/%d\n",med.ChunkSlices,i+1,chunks);
    fflush(stdout);
    for(j=0;j<med.ChunkSlices-2;j++) {
      sl_temp=sl1;
      sl1=sl2;
      sl2=sl_temp;
      slice=2*(med.ChunkOffsets[i]+j)+2;
      Create_Bin_Slice(sl2,j+1);
      printf("Processing slice %d/%d of chunk %d/%d\n",j+1,med.ChunkSlices,i+1,chunks);
      fflush(stdout);
      /* Do Z-faces fist */
      for(slp1=sl1,slp2=sl2,k=0;k<med.height;k++) {
	row=(k<<1)|1;
	num_TSE=0;
	for(l=0;l<med.width-1;l++,slp1++,slp2++)
	  if ( (*slp1) ^ (*slp2) ) { /* Z-face found */
	    TSE[num_TSE][0]= (l<<1)|1;
	    Calculate_Norm(&TSE[num_TSE][1],i,slice,row,(int)TSE[num_TSE][0], FALSE);
	    if (*slp1) TSE[num_TSE][0] |= 0x8000;
	    num_TSE++;
	  }
	if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	  printf("Could not write TSE\n");
	   exit(-1);
	}      
	sum += num_TSE;
      }
      
      /* Do first Y-row */
      num_TSE=0;
      for(slp2=sl2,l=0;l<med.width-1;slp2++,l++) {
	if (*slp2) { /* Found -Y face */
	  TSE[num_TSE][0]= (l<<1)|1;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice+1,0,(int)TSE[num_TSE][0], FALSE);
	  num_TSE++;
	}
      }
      if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	printf("Could not write TSE\n");
	 exit(-1);
      }      
      sum += num_TSE;
      
      
      for(slp1=sl2,slp2=sl2+med.width-1,k=0;k<med.height-1;k++) {
	row=(k<<1);
	num_TSE=0;
	/* Do 1st X- Faces */
	if (*slp1)  {
	  TSE[num_TSE][0]= 0;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+1,(int)TSE[num_TSE][0], FALSE);
	  num_TSE++;
	}
        /*Do X-face row */
	for(slp3=slp1,l=0;l<med.width-2;slp3++,l++) 
	  if ((*slp3)^(*(slp3+1))) { /* Found X-face */
	    TSE[num_TSE][0]= (l<<1) +2;
	    Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+1,(int)TSE[num_TSE][0], FALSE);
	    if (*slp3) TSE[num_TSE][0] |= 0x8000;
	    num_TSE++;
	  }
	/* Do last X-face */
	if ( *slp3) {
	  TSE[num_TSE][0]= (l<<1) +2;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+1,(int)TSE[num_TSE][0], FALSE);
	  TSE[num_TSE][0] |= 0x8000;
	  num_TSE++;
	}
	if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	  printf("Could not write TSE\n");
	   exit(-1);
	}      
	sum += num_TSE;

	/* Do Y-face row */
	num_TSE=0;
	for(l=0;l<med.width-1;l++,slp1++,slp2++) 
	  if ( (*slp1)^(*slp2) ) { /* Found Y- face */
	    TSE[num_TSE][0]=(l<<1)|1;
	    Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+2,(int)TSE[num_TSE][0], FALSE);
	    if (*slp1) TSE[num_TSE][0] |= 0x8000;
	    num_TSE++;
	  }
	if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	  printf("Could not write TSE\n");
	   exit(-1);
	}      
	sum += num_TSE;
      }

      /* Do last X-row */
      row=(k<<1);
      num_TSE=0;
      /* Do 1st X- Face */
      if (*slp1)  {
	TSE[num_TSE][0]= 0;
	Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+1,(int)TSE[num_TSE][0], FALSE);
	num_TSE++;
      }
      /*Do X-face row */
      for(slp3=slp1,l=0;l<med.width-2;slp3++,l++) 
	if ((*slp3)^(*(slp3+1))) { /* Found X-face */
	  TSE[num_TSE][0]= (l<<1) +2;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+1,(int)TSE[num_TSE][0], FALSE);
	  if (*slp3) TSE[num_TSE][0] |= 0x8000;
	  num_TSE++;
	}
      /* Do last X-face */
      if ( *slp3) {
	TSE[num_TSE][0]= (l<<1) +2;
	Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+1,(int)TSE[num_TSE][0], FALSE);
	TSE[num_TSE][0] |= 0x8000;
	num_TSE++;
      }
      if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	printf("Could not write TSE\n");
	 exit(-1);
      }      
      sum += num_TSE;
      

      /* Do last Y-face row */
      num_TSE=0;
      for(l=0;l<med.width-1;l++,slp1++) 
	if (*slp1) { /* Found Y- face */
	  TSE[num_TSE][0]=(l<<1)|1;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice+1,row+2,(int)TSE[num_TSE][0], FALSE);
	  TSE[num_TSE][0] |= 0x8000;
	  num_TSE++;
	}
      if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	printf("Could not write TSE\n");
	 exit(-1);
      }      
      sum += num_TSE;
      
    }
  }

  /* Do last Z- slice */
  slice=(med.ChunkOffsets[chunks-1]+med.ChunkSlices-1)<<1;
  for(slp1=sl2,k=0;k<med.height;k++) {
    row=(k<<1)|1;
    num_TSE=0;
    for(l=0;l<med.width-1;l++,slp1++)
      if ( (*slp1) ) { /* Z-face found */
	TSE[num_TSE][0]= (l<<1)|1;
	Calculate_Norm(&TSE[num_TSE][1],chunks-1,slice,row,(int)TSE[num_TSE][0], FALSE);
	TSE[num_TSE][0] |= 0x8000;
	num_TSE++;
      }
    if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
      printf("Could not write TSE\n");
       exit(-1);
    }      
    sum += num_TSE;
  }


  printf("%d faces witten\n",sum);
}







/************************************************************************
 *
 *      FUNCTION        : Create_Bin_Slice()
 *
 *      DESCRIPTION     : This Generate a binary valued slice that falls
 *                        on the slice specified, using the information
 *                        of the tracked data.
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      :
 *                   sl   - pointer to the slice data.
 *                   num  - current slice number.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : med should be setup.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  *
 *
 ************************************************************************/
void Create_Bin_Slice(unsigned char *sl, int num)
{

  int i,j,pos;
  unsigned char *ptr,flag;

  for(i=0;i<med.height;i++){
    flag=0;   /* no object */
    for(j=0;j<med.width-1;j++,sl++) {
      pos= (i*med.width+j);
      ptr = med.Chunk + med.bytes_per_slice*num + (pos>>3);
      if (*ptr&bit_pos[pos&7]) {
	flag= ~flag;
      }
      if (flag)
	*sl = 255;
      else
	*sl = 0;
    }
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
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 4/9/98 in.filename copied to
 *                         out.vh.str.scene_file by Dewey Odhner.
 *                        Modified: 6/5/01 new bit fields specified
 *                         by Dewey Odhner.
 *                        Modified: 7/4/09 st->domain corrected
 *                         by Dewey Odhner.
 *
 ************************************************************************/
void InitShell0()
{

  int i,elem;
  StructureInfo *st;
  FILE *pfp;

  st= &out.vh.str;

  st->dimension=3;
  st->dimension_valid=1;


  st->num_of_structures=1;
  st->num_of_structures_valid=1;


  st->domain=(float *)malloc(sizeof(float)*12);
  if (in.vh.scn.dimension==3) {
    for(i=0;i<12;i++)
      st->domain[i]=in.vh.scn.domain[i];
  }
  else   /* 4D */
    for(i=0;i<4;i++) {
      st->domain[i*3]=in.vh.scn.domain[i*4];
      st->domain[i*3+1]=in.vh.scn.domain[i*4+1];
      st->domain[i*3+2]=in.vh.scn.domain[i*4+2];
    }
  st->domain[0] = (float)(st->domain[0]+st->domain[9]*in.slice_loc);
  st->domain[1] = (float)(st->domain[1]+st->domain[10]*in.slice_loc);
  st->domain[2] = (float)(st->domain[2]+st->domain[11]*in.slice_loc);
  st->domain_valid=1;

  if (in.vh.scn.axis_label_valid) {
    st->axis_label=(Char30 *)calloc(3, sizeof(Char30));
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
    st->scene_file=(Char30 *)malloc(sizeof(Char30));
    strncpy((char *)st->scene_file[0],in.filename, sizeof(Char30));
    st->scene_file[0][sizeof(Char30)-1]=0;
    st->scene_file_valid=1;
  }
  else if (in.vh.gen.filename1_valid) {
    st->scene_file=(Char30 *)malloc(sizeof(Char30));
    strncpy((char *)st->scene_file[0],in.vh.gen.filename1, sizeof(Char30));
    st->scene_file[0][sizeof(Char30)-1]=0;
    st->scene_file_valid=1;
  }
  else {
    st->scene_file=(Char30 *)malloc(sizeof(Char30));
    strncpy((char *)st->scene_file[0],in.filename, sizeof(Char30));
    st->scene_file[0][sizeof(Char30)-1]=0;
    st->scene_file_valid=1;
  }


  get_min_max_values(&med.max_y,&med.min_y,&med.max_z,&med.min_z);
  min_x=med.min_x>>1; max_x= (med.max_x>>1) -1;
  min_y=med.min_y>>1; max_y= (med.max_y>>1) -1;
  min_z=med.min_z>>1; max_z= (med.max_z>>1) -1;




  /* This field will be corrected */
  st->num_of_TSE=(unsigned int *)malloc(sizeof(int));
  st->num_of_TSE[0]=0;
  st->num_of_TSE_valid=1;
  

  st->num_of_NTSE=(unsigned int *)malloc(sizeof(int));
  st->num_of_NTSE[0]=1+(max_z-min_z+1)*(max_y-min_y+2);
  st->num_of_NTSE_valid=1;
  
  st->num_of_components_in_TSE=9; /* ncode,y1,tt,n1,n2,n3,gm,op,og */
  st->num_of_components_in_TSE_valid=1;
  
  st->num_of_components_in_NTSE=1;
  st->num_of_components_in_NTSE_valid=1;


  
  st->smallest_value=(float *)malloc(sizeof(float)*9);
  st->largest_value=(float *)malloc(sizeof(float)*9);

  st->smallest_value[0]=0;   /* ncode */
  st->largest_value[0]=1;
  st->smallest_value[1]= (float)min_x; /* y1 */
  st->largest_value[1]= (float)max_x;
  st->smallest_value[2]=st->largest_value[2]=0.0;
  st->smallest_value[3]=0.0; st->largest_value[3]=6.0; /* normals */
  st->smallest_value[4]=0.0; st->largest_value[4]=63.0;
  st->smallest_value[5]=0.0; st->largest_value[5]=63.0;
  st->smallest_value[6]=st->largest_value[6]=0.0;
  st->smallest_value[7]=st->largest_value[7]=0.0;
  st->smallest_value[8]=st->largest_value[8]=0.0;
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
  st->num_of_samples[0]=med.slices;
  st->num_of_samples_valid=1;
 
  st->xysize[0]= (float)(2*med.Px);
  st->xysize[1]= (float)(2*med.Py);
  st->xysize_valid=1;
 
  st->loc_of_samples=(float *)malloc(sizeof(float)*med.slices);
  st->loc_of_samples[0]=0.0;
  for(i=1;i<med.slices;i++)
    st->loc_of_samples[i]= (float)(2*i*med.Pz);
  st->loc_of_samples_valid=1;

  if (!param_on) {
    if (in.vh.scn.dimension==3) {
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
      st->parameter_vectors=(float *)malloc(sizeof(float)*2);
      st->parameter_vectors[0]= (float)in.volume;
      st->parameter_vectors[1]= 0.0;
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
      st->description_of_element=(short *)malloc(st->num_of_elements*sizeof(short));
      st->parameter_vectors=(float *)malloc(st->num_of_elements*sizeof(float));
      for(i=0;i<st->num_of_elements;i++)
	if (fscanf(pfp,"%d",&elem)!=1) {
	  printf("Could not read parameters from file\n");
	  st->num_of_elements_valid=0;
	  st->description_of_element_valid=0;
	  st->parameter_vectors_valid=0;
	  break;
	}
	else
	  st->description_of_element[i]=elem;
      if (st->parameter_vectors_valid!=0) {
	for(i=0;i<st->num_of_elements;i++)
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
  
  
  st->min_max_coordinates=(float *)malloc(sizeof(float)*6);
  st->min_max_coordinates[0]= (float)(2*med.Px*min_x);
  st->min_max_coordinates[1]= (float)(2*med.Py*min_y);
  st->min_max_coordinates[2]= (float)(2*med.Pz*min_z);
  st->min_max_coordinates[3]= (float)(2*med.Px*max_x);
  st->min_max_coordinates[4]= (float)(2*med.Py*max_y);
  st->min_max_coordinates[5]= (float)(2*med.Pz*max_z);
  st->min_max_coordinates_valid=1;

  st->volume=(float *)malloc(sizeof(float));
  st->volume[0]= (float)(med.volume*med.Px*med.Py*med.Pz*4);
  st->volume_valid=1;

}






/************************************************************************
 *
 *      FUNCTION        : WriteTSE_SHELL0()
 *
 *      DESCRIPTION     : Given  the  number of chunks the that have been
 *                        tracked generate the TSE for SHELL0 data.
 *
 *      RETURN VALUE    : None.
 * 
 *      PARAMETERS      : 1 parameter.
 *                 chunks : number of chunks the scene is devided into.
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
 *      History         : 07/12/1993 Supun Samarasekera.
 *                        10/24/95 neighbors of last row corrected
 *                           by Dewey Odhner.
 *                        Modified: 5/21/01 new bit fields used by Dewey Odhner
 *                        Modified: 7/5/09 ntse corrected by Dewey Odhner.
 *
 ************************************************************************/
void WriteTSE_SHELL0(int chunks)
{
  int i,j,k,l,k1,l1,num_TSE,sum,size,slice,error,num;
  unsigned int pos;
  short *ntse,slices,rows;
  unsigned short NCODE;
  unsigned char *sl1,*sl2,*sl3,*sl_temp,*nz,*ny,*nx,*pz,*py,*px,*cur;
  char grp[6],elem[6];
  TSE_type *TSE;
  
  sum =0;
  fseek(out.fp,0L,2L);
  slices=max_z-min_z+1;
  rows=max_y-min_y+1;
  /* top level */
  if (VWriteData((char *)&slices,2,1,out.fp,&num)) {
    printf("Could not write NTSE\n");
     exit(-1);
  }
  /* second level */
  for(i=0;i<slices;i++)
    if (VWriteData((char *)&rows,2,1,out.fp,&num)) {
      printf("Could not write NTSE\n");
       exit(-1);
    }

  /* Store position to come back and update */
  pos=ftell(out.fp);
  /* Write out the temp space for NTSE of the shell */
  if (VWriteData((char *)med.NTSE,2,slices*rows,out.fp,&num)) {
    printf("Could not write NTSE\n");
     exit(-1);
  }
  
  
  TSE=(TSE_type *)malloc(sizeof(TSE_type)*(med.max_x-med.min_x+1));
  if (TSE==NULL) {
    printf("Could Not Allocate space for TSE's\n");
     exit(-1);
  }
  size= (med.width-1)*med.height;
  if ((sl1=(unsigned char *)malloc(size))==NULL) {
    printf("Could not allocate space for slices\n");
     exit(-1);
  }
  if ((sl2=(unsigned char *)malloc(size))==NULL) {
    printf("Could not allocate space for slices\n");
     exit(-1);
  }
  if ((sl3=(unsigned char *)malloc(size))==NULL) {
    printf("Could not allocate space for slices\n");
     exit(-1);
  }
  
  
  for(i=0;i<chunks;i++) {
    ReadChunk(i);
    Create_Bin_Slice(sl2,0);
    Create_Bin_Slice(sl3,1);
    for(j=0;j<med.ChunkSlices-2;j++) {
      sl_temp=sl1;
      sl1=sl2;
      sl2=sl3;
      sl3=sl_temp;
      slice= 2*(med.ChunkOffsets[i]+j)+3;
      Create_Bin_Slice(sl3,j+2);
      ntse= med.NTSE + med.height*(med.ChunkOffsets[i]+j+1);
      /* Do 1st Row */
      num_TSE=0;
      nz=sl1; 
      cur=sl2; 
      pz=sl3;
      ny=cur-med.width+1;
      py=cur+med.width-1;
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
	Calculate_Norm(&TSE[num_TSE][1],i,slice,1,1, TRUE);
	num_TSE++;
      }
      nz++,pz++,ny++,py++,nx++,px++,cur++;
      /* Do all but last colunm */
      for(l1=3,l=1;l<med.width-2;l1+=2,l++,nz++,pz++,ny++,py++,nx++,px++,cur++) {
	if (*cur) { /* This voxel is on */
	  NCODE=0;
	  if (*nz) NCODE|= NZ;
	  if (*pz) NCODE|= PZ;
	  if (*py) NCODE|= PY;
	  if (*px) NCODE|= PX;
	  if (*nx) NCODE|= NX;
	  TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice,1,l1, TRUE);
      if (l & 1) TSE[num_TSE][1] |= 0x8000;
	  num_TSE++;
	}
      }
      /* Do last Col of the first row */
      if (*cur) { /* This voxel is on */
	NCODE=0;
	if (*nz) NCODE|= NZ;
	if (*pz) NCODE|= PZ;
	if (*py) NCODE|= PY;
	if (*nx) NCODE|= NX;
	TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
	Calculate_Norm(&TSE[num_TSE][1],i,slice,1,l1, TRUE);
	if (l & 1) TSE[num_TSE][1] |= 0x8000;
	num_TSE++;
      }
      nz++,pz++,ny++,py++,nx++,px++,cur++;
      *ntse=num_TSE;
      if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	printf("Could not write TSE\n");
	 exit(-1);
      }      
      ntse++;
      sum += num_TSE;
      
      
      /* Do all rows exept the last row */
      for(k1=3,k=1;k<med.height-1;k++,k1+=2) {
	num_TSE=0;
	/* Do 1st Col of the row */
	if (*cur) { /* This voxel is on */
	  NCODE=0;
	  if (*pz) NCODE|= PZ;
	  if (*nz) NCODE|= NZ;
	  if (*py) NCODE|= PY;
	  if (*ny) NCODE|= NY;
	  if (*px) NCODE|= PX;
	  TSE[num_TSE][0]= NCODE | 0x0000;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice,k1,1, TRUE);
	  num_TSE++;
	}
	nz++,pz++,ny++,py++,nx++,px++,cur++;
	/* Do all but last colunm */
	for(l1=3,l=1;l<med.width-2;l1+=2,l++,nz++,pz++,ny++,py++,nx++,px++,cur++) {
	  if (*cur) { /* This voxel is on */
	    NCODE=0;
	    if (*pz) NCODE|= PZ;
	    if (*nz) NCODE|= NZ;
	    if (*py) NCODE|= PY;
	    if (*ny) NCODE|= NY;
	    if (*px) NCODE|= PX;
	    if (*nx) NCODE|= NX;
	    if (NCODE != ALL_NEIGHBORS) {
	      TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
	      Calculate_Norm(&TSE[num_TSE][1],i,slice,k1,l1, TRUE);
	      if (l & 1) TSE[num_TSE][1] |= 0x8000;
	      num_TSE++;
	    }
	  }
	}
	/* Do last Col of the row */
	if (*cur) { /* This voxel is on */
	  NCODE=0;
	  if (*pz) NCODE|= PZ;
	  if (*nz) NCODE|= NZ;
	  if (*py) NCODE|= PY;
	  if (*ny) NCODE|= NY;
	  if (*nx) NCODE|= NX;
	  TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice,k1,l1, TRUE);
	  if (l & 1) TSE[num_TSE][1] |= 0x8000;
	  num_TSE++;
	}
	nz++,pz++,ny++,py++,nx++,px++,cur++;
	*ntse=num_TSE;
	if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	  printf("Could not write TSE\n");
	   exit(-1);
	}      
	ntse++;
	sum += num_TSE;
      }
      
      
      /* Do last row */
      
      num_TSE=0;
      /* Do 1st Col of the row */
      if (*cur) { /* This voxel is on */
	NCODE=0;
	if (*nz) NCODE|= NZ;
	if (*pz) NCODE|= PZ;
	if (*ny) NCODE|= NY;
	if (*px) NCODE|= PX;
	TSE[num_TSE][0]= NCODE | 0x0000;
	Calculate_Norm(&TSE[num_TSE][1],i,slice,k1,1, TRUE);
	num_TSE++;
      }
      nz++,pz++,ny++,py++,nx++,px++,cur++;
      /* Do all but last colunm */
      for(l1=3,l=1;l<med.width-2;l1+=2,l++,nz++,pz++,ny++,py++,nx++,px++,cur++) {
	if (*cur) { /* This voxel is on */
	  NCODE=0;
	  if (*nz) NCODE|= NZ;
	  if (*pz) NCODE|= PZ;
	  if (*ny) NCODE|= NY;
	  if (*px) NCODE|= PX;
	  if (*nx) NCODE|= NX;
	  TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
	  Calculate_Norm(&TSE[num_TSE][1],i,slice,k1,l1, TRUE);
	  if (l & 1) TSE[num_TSE][1] |= 0x8000;
	  num_TSE++;
	}
      }
      /* Do last Col of the row */
      if (*cur) { /* This voxel is on */
	NCODE=0;
	if (*nz) NCODE|= NZ;
	if (*pz) NCODE|= PZ;
	if (*ny) NCODE|= NY;
	if (*nx) NCODE|= NX;
	TSE[num_TSE][0]= NCODE | (unsigned short)l>>1;
	Calculate_Norm(&TSE[num_TSE][1],i,slice,k1,l1, TRUE);
	if (l & 1) TSE[num_TSE][1] |= 0x8000;
	num_TSE++;
      }
      nz++,pz++,ny++,py++,nx++,px++,cur++;
      *ntse=num_TSE;
      if (num_TSE && VWriteData((char *)TSE,sizeof(TSE_type)/2,2*num_TSE,out.fp,&num)) {
	printf("Could not write TSE\n");
	 exit(-1);
      } 
      sum += num_TSE;
      
    }
    
    
  }

  printf("minx %d maxx %d miny %d maxy %d minz %d maxz %d\n",min_x,max_x,min_y,max_y,
	 min_z,max_z);
  printf("minx %d maxx %d miny %d maxy %d minz %d maxz %d\n",med.min_x,med.max_x,med.min_y,
	 med.max_y,med.min_z,med.max_z);
  printf("Total Voxels found %d\n",sum);
  /* Go back to the NTSE and update it */
  fseek(out.fp,pos,0L);
  for(ntse=med.NTSE+min_z*med.height+min_y,i=min_z;i<=max_z;i++) {
    if (VWriteData((char *)ntse,2,rows,out.fp,&num)) {
      printf("Could not write NTSE\n");
       exit(-1);
    }      
    ntse+= med.height;
  }


  /* Go back and update the header */
  fseek(out.fp,0L,0L);
  out.vh.str.num_of_TSE[0]=sum;



  error=VWriteHeader(out.fp,&out.vh,grp,elem);
  if (error) {
    printf("Write Error %d (group:%s element:%s\n",error,grp,elem);
    if (error!=106 && error!=107) {
      printf("Cannot revoer from error\n");
      
      exit(-1);
    }
  }

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
 *      PARAMETERS      : norm,vol,sl,row,col: Not Used.
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
 *                        06/13/2018 parameter "chunk" removed
 *                          by Dewey Odhner
 *
 ************************************************************************/
void Calculate_Norm0(unsigned short *norm, int vol, int sl, int row, int col, int B)
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
void Calculate_Norm8(norm,chunk,sl,row,col, B)
unsigned short *norm;
int chunk,sl,row,col, B;
{
  double p111,p222,p122,p211,p121,p212,p221,p112;
  double v1,v2,v3,v4;
  const int d=sl&row&col&1? 2:1;

  p111=VoxVal(chunk,sl+d,row+d,col+d);
  p222=VoxVal(chunk,sl-d,row-d,col-d);
  p122=VoxVal(chunk,sl+d,row-d,col-d);
  p211=VoxVal(chunk,sl-d,row+d,col+d);
  p121=VoxVal(chunk,sl+d,row-d,col+d);
  p212=VoxVal(chunk,sl-d,row+d,col-d);
  p221=VoxVal(chunk,sl-d,row-d,col+d);
  p112=VoxVal(chunk,sl+d,row+d,col-d);
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
 *      RELATED FUNCS   : VoxVal,G_code,BG_code
 *
 *      History         : 07/12/1993 Supun Samarasekera                  
 *                        Modified: 5/21/01 B passed by Dewey Odhner.
 *
 ************************************************************************/
void Calculate_Norm26(norm,chunk,sl,row,col, B)
unsigned short *norm;
int chunk,sl,row,col, B;
{
  double p001,p002,p010,p011,p012,p020,p021,p022;
  double p100,p101,p102,p110,p111,p112,p120,p121,p122;
  double p200,p201,p202,p210,p211,p212,p220,p221,p222;
  double l1,l2,l3,l4,l5,l6,l7,l8,l9,l10,l11,l12,l13;
  const int d=sl&row&col&1? 2:1;

  p001=VoxVal(chunk,sl  ,row  ,col+d);
  p002=VoxVal(chunk,sl  ,row  ,col-d);
  p010=VoxVal(chunk,sl  ,row+d,col  );
  p011=VoxVal(chunk,sl  ,row+d,col+d);
  p012=VoxVal(chunk,sl  ,row+d,col-d);
  p020=VoxVal(chunk,sl  ,row-d,col  );
  p021=VoxVal(chunk,sl  ,row-d,col+d);
  p022=VoxVal(chunk,sl  ,row-d,col-d);
  p100=VoxVal(chunk,sl+d,row  ,col  );
  p101=VoxVal(chunk,sl+d,row  ,col+d);
  p102=VoxVal(chunk,sl+d,row  ,col-d);
  p110=VoxVal(chunk,sl+d,row+d,col  );
  p111=VoxVal(chunk,sl+d,row+d,col+d);
  p112=VoxVal(chunk,sl+d,row+d,col-d);
  p120=VoxVal(chunk,sl+d,row-d,col  );
  p121=VoxVal(chunk,sl+d,row-d,col+d);
  p122=VoxVal(chunk,sl+d,row-d,col-d);
  p200=VoxVal(chunk,sl-d,row  ,col  );
  p201=VoxVal(chunk,sl-d,row  ,col+d);
  p202=VoxVal(chunk,sl-d,row  ,col-d);
  p210=VoxVal(chunk,sl-d,row+d,col  );
  p211=VoxVal(chunk,sl-d,row+d,col+d);
  p212=VoxVal(chunk,sl-d,row+d,col-d);
  p220=VoxVal(chunk,sl-d,row-d,col  );
  p221=VoxVal(chunk,sl-d,row-d,col+d);
  p222=VoxVal(chunk,sl-d,row-d,col-d);
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
 *      FUNCTION        :GetNextNonEmptyQ()
 *
 *      DESCRIPTION     : Find the next non-empty Queue Chunk to track the 
 *                        boundary. If Boundary Tracking is over the 
 *                        function would return -1.
 *
 *      RETURN VALUE    : -1 No more chunks to track.
 *                        chunk number otherwise.
 *
 *      PARAMETERS      : 
 *                    QChunks    - Queue list of the Chunks.
 *                    num_chunks - total number of Chunks.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : QChunks should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *
 ************************************************************************/
int GetNextNonEmptyQ(Qtype *QChunks, int num_chunks)
{
  int i;


  for(i=0;i<num_chunks;QChunks++,i++) 
    if (QChunks->pop_at!=QChunks->push_at)
      return(i);
  return(-1);
  
}



/****************************************************************************
 The Current Chunk is already in memory. This includes the last row of the
 previous chunk and the first row of the next chunk. Check if the face was 
 visited. If not add to the Q. The Q can over flow or under flow to the
 next or the previous chunk. Then push to the respective Q instead.
 ****************************************************************************/

/************************************************************************
 *
 *      FUNCTION        :
 *
 *      DESCRIPTION     :
 *
 *      RETURN VALUE    :
 *
 *      PARAMETERS      :
 *
 *      SIDE EFFECTS    :
 *
 *      ENTRY CONDITION :
 *
 *      EXIT CONDITIONS :
 *
 *      RELATED FUNCS   :
 *
 *      History         : 07/12/1993 Supun Samarasekra                  *
 *
 ************************************************************************/
void PushQ(Qtype *QChunks, short st[5])
{
  Qtype *q;
  int CURRENT_CHUNK,CHUNK_IN_Q,slice_in_chunk;
  unsigned int pos;
  unsigned char *ptr,elem;

  CHUNK_IN_Q=CURRENT_CHUNK=st[4];
  slice_in_chunk= st[3]-2*med.ChunkOffsets[CURRENT_CHUNK]-1;
  if (slice_in_chunk==0)
    CHUNK_IN_Q--;
  else if (slice_in_chunk/2 == med.ChunkSlices-1)
    CHUNK_IN_Q++;
  if (st[0]==NX_type || st[0]==PX_type) {  /* check if already visited */
    pos= (st[2]>>1)*med.width+ (st[1]>>1);
    ptr= med.Chunk+(slice_in_chunk>>1)*med.bytes_per_slice + (pos>>3);
    elem= bit_pos[ pos&7 ];
    if ((*ptr)&elem) /* Already Visited */
      return;
    *ptr |= elem;  /* Set as Visited */
  }
  if (st[0]==PX_type) {
    med.volume += st[1];
    if (med.min_x > st[1]) med.min_x=st[1];
    if (med.max_x < st[1]) med.max_x=st[1];
  }
  else if (st[0]==NX_type) {
    med.volume -= st[1];
    if (med.min_x > st[1]) med.min_x=st[1];
    if (med.max_x < st[1]) med.max_x=st[1];
  }
  q= QChunks+CHUNK_IN_Q;
  q->Q[q->push_at][0]=st[0];
  q->Q[q->push_at][1]=st[1];
  q->Q[q->push_at][2]=st[2];
  q->Q[q->push_at][3]=st[3];
  
  q->push_at++;
  if (q->push_at==QSIZE)
    q->push_at=0;
  /* Compute some information needed for the output structure */ 
  med.num_TSE++;
  med.NTSE[st[3]*(2*med.height+1)+st[2]]++;
/*
  printf("slice %d row %d col %d\n",st[3],st[2],st[1]); 
*/


}
  


/************************************************
 Pop next element from the Q. If the Q is empty
 return 0. else return 1.
 ************************************************/
int PopQ(Qtype *QChunks, int chunk, short st[5])
{
  Qtype *q;
  
  q= QChunks + chunk;
  if (q->push_at == q->pop_at) 
    return(0);
  else {
    st[0]=q->Q[q->pop_at][0];
    st[1]=q->Q[q->pop_at][1];
    st[2]=q->Q[q->pop_at][2];
    st[3]=q->Q[q->pop_at][3];
    st[4]=chunk;
    q->pop_at++;
    if (q->pop_at==QSIZE) 
      q->pop_at=0;
  }
  return(1);
}





/*************************************************************************
  The Chunks would have overlapping partitions. Each Chunk Consists of 
   a set that starts and end with a "Z-face" slice. But as the next slice
   on both directions can be set they have to be brought in. 
   Queues of adjecent Chunks would share a "Z-face" slice. When iterating
   it would go into the current Queue.
 **************************************************************************/
int InitChunks(Qtype **QC)
{
  int csize,i,num_chunks;

  if (CHUNK_SIZE> med.slices) CHUNK_SIZE=med.slices;
  
  /* Add room for one previous and next slice */
  if ((med.Chunk=(unsigned char *)malloc(med.bytes_per_slice*(CHUNK_SIZE+2)))==NULL) {
    printf("Could not Allocate space for the Chunks\n");
    return(-1);
  }
  med.ChunkSlices=CHUNK_SIZE+2;

  num_chunks= (int)ceil((double)med.slices/CHUNK_SIZE);

  if ((med.ChunkOffsets= (short *)malloc(sizeof(short)*num_chunks))==NULL) {
    printf("Could not Allocate space for the Chunk Offsets\n");
    return(-1);
  }    
  if ((*QC= (Qtype *)malloc(sizeof(Qtype)*num_chunks))==NULL) {
    printf("Could not Allocate space for the Queue\n");
    return(-1);
  }

  if ((in.ChunkSlices=(short *)malloc(sizeof(short)*num_chunks))==NULL) {
    printf("Could not Allocate space for the Chunk Offsets\n");
    return(-1);
  }
  if ((in.ChunkOffsets= (short *)malloc(sizeof(short)*num_chunks))==NULL) {
    printf("Could not Allocate space for the Chunk Offsets\n");
    return(-1);
  }
  for(i=0;i<num_chunks;i++) {
    med.ChunkOffsets[i]=i*CHUNK_SIZE - 1;
    (*QC)[i].push_at=(*QC)[i].pop_at=0; /* Initially all Q's are empty */
  }
  csize=0;
  for(i=0;i<num_chunks;i++) {
    if (med.ChunkOffsets[i] <0)
      in.ChunkOffsets[i]= -1;
    else
      in.ChunkOffsets[i]= interp.z_table[2*med.ChunkOffsets[i]+1];
    if ( (med.ChunkOffsets[i] + med.ChunkSlices+1) >= med.slices ) 
      in.ChunkSlices[i]= in.slices - in.ChunkOffsets[i] +1;
    else 
      in.ChunkSlices[i]= interp.z_table[2*(med.ChunkOffsets[i] + med.ChunkSlices)+1] -
	in.ChunkOffsets[i] + 2;
    if (in.ChunkSlices[i] > csize)
      csize=in.ChunkSlices[i];
  }
  
  
      
  /* Add room for one previous and next slice */
  if ((in.Chunk=(unsigned char *)malloc(in.bytes_per_slice*csize))==NULL) {
    printf("Could not Allocate space for the Chunks\n");
    return(-1);
  }
  

  /* Write out the data_space for the med data */
  memset(med.Chunk,0,med.bytes_per_slice);
  for(i=0;i<med.slices;i++) 
    if (fwrite(med.Chunk,1,med.bytes_per_slice,med.fp)!=med.bytes_per_slice) {
      printf("Could not write intermideate structures\n");
      return(-1);
    }
	
	
  return(num_chunks);

}





/*******************************************************************
 This read in the Current Chunk for both the input and tracked(med)
 data. According to the offsets specified at the begining and the 
 end of the data set blank padding  may be added 
 *******************************************************************/
int ReadChunk(int chunk)
{
  unsigned char *data;
  int i,start,end,size,num;

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
  
  
  fseek(in.fp,(long)(in.vh_len+in.skip_slices+i*in.bytes_per_slice),0L);
  for(;i<end && i<in.slices;i++) {
    if (VReadData((char *)data,size,in.bytes_per_slice/size,in.fp,&num)) {
      printf("Could not read Current Chunk\n");
      return(-1);
    }
    data += in.bytes_per_slice;
  }
  for(;i<end;i++) {
    memset(data,0,in.bytes_per_slice);
    data += in.bytes_per_slice;
  }

  /* Read in the tracked data */
  data=med.Chunk;
  start=med.ChunkOffsets[chunk];
  end= start + med.ChunkSlices;
  /* Fill in the blank slices */
  for(i=start;i<0;i++) {
    memset(data,0,med.bytes_per_slice);
    data += med.bytes_per_slice;
  }
  
  fseek(med.fp,(long)(i*med.bytes_per_slice),0L);
  for(;i<end && i<med.slices;i++) {
    if (fread(data,1,med.bytes_per_slice,med.fp)!=
	med.bytes_per_slice ) {
      printf("Could not read Current Chunk\n");
      return(-1);
    }
    data += med.bytes_per_slice;
  }
  for(;i<end;i++) {
    memset(data,0,med.bytes_per_slice);
    data += med.bytes_per_slice;
  }


  return(1);

}
    
 







int WriteChunk(int chunk)
{
  unsigned char *data;
  int start,end;

  data=med.Chunk;
  start=med.ChunkOffsets[chunk];
  end= start + med.ChunkSlices;
  if (start <0) { /* If at the begining pad a slice with zero's */
    data += ( -start*med.bytes_per_slice);
    start=0;
  }
  
  fseek(med.fp,(long)(start*med.bytes_per_slice),0L);
  if (end >= med.slices) {
    if (fwrite(data,1,med.bytes_per_slice*(med.slices-start),med.fp)!=
	med.bytes_per_slice*(med.slices-start) ) {
      printf("Could not write Current Chunk\n");
      return(-1);
    }
  }
  else 
    if (fwrite(data,1,med.bytes_per_slice*(end-start),med.fp)!=
	med.bytes_per_slice*(end-start) ) {
      printf("Could not read Current Chunk\n");
      return(-1);
    }
  return(1);

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
unsigned short VoxOn(double min_thresh, double max_thresh, int chunk,
    int slice, int row, int col)
{

  float val,VoxVal();

  val=VoxVal(chunk,slice,row,col);
  
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
float VoxVal(int chunk, int slice, int row, int col)
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
    val= (float)InVoxVal(chunk,lz,ly,lx);
    break;
  case 1:  /* Z interpolation only */
    zr=interp.z_dist[slice];
    val= (float)(InVoxVal(chunk,lz,ly,lx)*(1.0-zr)+InVoxVal(chunk,lz+1,ly,lx)*zr);
    break;
  case 7: /* Interpolate in All Directions */
    p000=InVoxVal(chunk,lz  ,ly  ,lx);  p001=InVoxVal(chunk,lz  ,ly  ,lx+1);
    p010=InVoxVal(chunk,lz  ,ly+1,lx);  p011=InVoxVal(chunk,lz  ,ly+1,lx+1);
    p100=InVoxVal(chunk,lz+1,ly  ,lx);  p101=InVoxVal(chunk,lz+1,ly  ,lx+1);
    p110=InVoxVal(chunk,lz+1,ly+1,lx);  p111=InVoxVal(chunk,lz+1,ly+1,lx+1);
    xr= interp.x_dist[col];
    yr= interp.y_dist[row];
    zr= interp.z_dist[slice];
    val= 
      ((1-zr)*((1-yr)*((1-xr)*p000 + xr*p001) + yr*((1-xr)*p010 + xr*p011)) +
       zr*((1-yr)*((1-xr)*p100 + xr*p101) + yr*((1-xr)*p110 + xr*p111)));
    break;
  case 2: /* Y interpolation only */
    yr=interp.y_dist[row];
    val= (float)(InVoxVal(chunk,lz,ly,lx)*(1.0-yr)+InVoxVal(chunk,lz,ly+1,lx)*yr);
    break;
  case 4: /* X interpolation only */
    xr=interp.x_dist[col];
    val= (float)(InVoxVal(chunk,lz,ly,lx)*(1.0-xr)+InVoxVal(chunk,lz,ly,lx+1)*xr);
    break;
  case 3: /* Y and Z interpolation */
    p000=InVoxVal(chunk,lz  ,ly  ,lx);
    p010=InVoxVal(chunk,lz  ,ly+1,lx);
    p100=InVoxVal(chunk,lz+1,ly  ,lx);
    p110=InVoxVal(chunk,lz+1,ly+1,lx);
    yr= interp.y_dist[row];
    zr= interp.z_dist[slice];
    val= 
      ((1-zr)*((1-yr)*p000 + yr*p010) +
       zr*((1-yr)*p100 + yr*p110));
    break;
  case 5: /* X and Z interpolation */
    p000=InVoxVal(chunk,lz  ,ly  ,lx);  p001=InVoxVal(chunk,lz  ,ly  ,lx+1);
    p100=InVoxVal(chunk,lz+1,ly  ,lx);  p101=InVoxVal(chunk,lz+1,ly  ,lx+1);
    xr= interp.x_dist[col];
    zr= interp.z_dist[slice];
    val= 
      ((1-zr)*((1-xr)*p000 + xr*p001) +
       zr*((1-xr)*p100 + xr*p101));
    break;
  case 6: /* X and Y interpolation */
    p000=InVoxVal(chunk,lz  ,ly  ,lx);  p001=InVoxVal(chunk,lz  ,ly  ,lx+1);
    p010=InVoxVal(chunk,lz  ,ly+1,lx);  p011=InVoxVal(chunk,lz  ,ly+1,lx+1);
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
int InVoxVal(int chunk, int in_slice, int in_row, int in_col)
{

  int pos;
  unsigned char *ptr;
  unsigned short *ptr16;

  if (in_slice<0 || in_row < 0 || in_col<0 ||
      in_slice>=in.slices || in_row >= in.height || in_col >= in.width)
    return(0);

  in_slice -= in.ChunkOffsets[chunk];
  pos= in_row*in.width + in_col;
  switch (in.bits_per_pixel) {
  case 1:
    ptr = in.Chunk + in_slice*in.bytes_per_slice + (pos>>3);
    return(((*ptr)&bit_pos[pos&7]) ? 1 : 0);
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
  
  





/******************************************************************
 The process that calls the this program send the starting points 
 through a fiel called .start_point . This file would contain
 the number of points as the 1st field and a list of 3d points.
 3d points are given as slice row col.
 *******************************************************************/
int ReadStartPointList(char *pt_file, StartPoint **list)
{

  FILE *fp;
  int i,num;

  fp=fopen(pt_file,"rb");
  if (fp==NULL) return(0);
  if (fscanf(fp,"%d",&num)==0 || num<=0) { fclose(fp); return(0);}
  *list = (StartPoint *)malloc(sizeof(StartPoint)*num);
  if (*list == NULL) { fclose(fp); return(0);}
  for(i=0;i<num;i++) {
    if ( fscanf(fp,"%d %d %d",(*list)[i]+2,(*list)[i]+1,(*list)[i])!=3) {
      printf("Error in reading start point list\n");
      fclose(fp);
      return(0);
    }
  }
  fclose(fp);
  return(num);


}


/* Modified: 4/21/98 search extended past edge of scene by Dewey Odhner. */
int get_starting_point(int num_chunks, int slice, int row, int col,
	double min_thresh, double max_thresh, short st[5])
{

  int i,val,val1,out_slice,out_row,out_col,cur_chunk;

  /* Select the Chunk the slice belongs to */

  out_slice= 2*(int)(floor(0.5*(slice*in.Pz + in.Pz/2.0)/med.Pz)) +1;
  out_row  = 2*(int)(floor(0.5*(row*in.Py + in.Py/2.0)/med.Py)) + 1;
  out_col  = 2*(int)(floor(0.5*(col*in.Px + in.Px/2.0)/med.Px))+1;


  for(i=0;i<num_chunks;i++) {
    if ( (out_slice > (2*med.ChunkOffsets[i]+1) ) &&
	(out_slice < (2*(med.ChunkOffsets[i] + med.ChunkSlices -1)+1)) )
      break;
  }
  if (i==num_chunks) {
    printf("Could not locate Stating point");
    return(-1);
  }
  cur_chunk=i;
  if (ReadChunk(cur_chunk)== -1) {
     exit(-1);
  }

  val=VoxOn(min_thresh,max_thresh,cur_chunk,out_slice,out_row,out_col);
  for(i=out_col+2;i<=2*med.width+1;i+=2){
    val1=VoxOn(min_thresh,max_thresh,cur_chunk,out_slice,out_row,i);
    if (val!=val1) {
      st[4]=cur_chunk; st[3]= out_slice; st[2]=out_row; st[1]=i-1;
      if (val) 
	st[0]=PX_type;
      else
	st[0]=NX_type;
      return(cur_chunk);
    }
  }

  return(-1);


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
 *                        of the input pixel size.
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
 *                        Modified 8/13/07 unresized voxels allowed
 *                           by Dewey Odhner.
 *
 ************************************************************************/
void SetupMedValues(float ratio)
{
  

  int i;

  if (ratio > 0)
	med.Pz=med.Py=med.Px= in.vh.scn.xypixsz[0]*ratio/2.0;
  else
  {
	med.Pz = in.Pz/2.0;
	med.Py = in.Py/2.0;
	med.Px = in.Px/2.0;
  }
  med.width = (short)floor(in.Px*in.width*0.5/med.Px)+1;
  med.height= (short)floor(in.Py*in.height*0.5/med.Py);
  med.slices= (short)floor(in.Pz*in.slices*0.5/med.Pz);
  med.bits_per_pixel=1;
  med.bytes_per_slice= (med.width*med.height+7)/8;
  med.volume=med.num_TSE=0;
  med.max_x=0;med.min_x=0x7FFF;
  if ((med.NTSE=(short *)calloc((2*med.height+1)*(2*med.slices+1),sizeof(short)))==NULL) {
    printf("Could not Allocate space for NTSE");
     exit(-1);
  }
  
  interp.xsize=2*(int)floor(in.Px*in.width*0.5/med.Px)+1;
  interp.ysize=2*med.height+1;
  interp.zsize=2*med.slices+1;
  interp.x_table=(short *)malloc(sizeof(short)*interp.xsize);
  interp.y_table=(short *)malloc(sizeof(short)*interp.ysize);
  interp.z_table=(short *)malloc(sizeof(short)*interp.zsize);
  if (in.slices >= interp.zsize) {
    interp.z_flag=0;
    interp.z_dist=NULL;
    for(i=0;i<interp.zsize;i++)
      interp.z_table[i] = (short)rint((2*i*med.Pz-in.Pz)*0.5/in.Pz);
  }
  else {
    interp.z_flag=1;
    interp.z_dist=(float *)malloc(sizeof(float)*interp.zsize);
    for(i=0;i<interp.zsize;i++)  {
      interp.z_dist[i] = (float)((2*i*med.Pz-in.Pz)*0.5/in.Pz);
      interp.z_table[i]= (short)floor(interp.z_dist[i]);
      interp.z_dist[i] -= interp.z_table[i];
    }
  }
  if (in.height >= interp.ysize) {
    interp.y_flag=0;
    interp.y_dist=NULL;
    for(i=0;i<interp.ysize;i++) 
      interp.y_table[i]= (short)rint((2*i*med.Py-in.Py)*0.5/in.Py);
  }
  else {
    interp.y_flag=2;
    interp.y_dist=(float *)malloc(sizeof(float)*interp.ysize);
    for(i=0;i<interp.ysize;i++)  {
      interp.y_dist[i] = (float)((2*i*med.Py-in.Py)*0.5/in.Py);
      interp.y_table[i]= (short)floor(interp.y_dist[i]);
      interp.y_dist[i] -= interp.y_table[i];
    }
  }

  if (in.width >= interp.xsize) {
    interp.x_flag=0;
    interp.x_dist=NULL;
    for(i=0;i<interp.xsize;i++)
      interp.x_table[i]= (short)rint((2*i*med.Px-in.Px)*0.5/in.Px);
  }
  else {
    interp.x_flag=4;
    interp.x_dist=(float *)malloc(sizeof(float)*interp.xsize);
    for(i=0;i<interp.xsize;i++)  {
      interp.x_dist[i] = (float)((2*i*med.Px-in.Px)*0.5/in.Px);
      interp.x_table[i]= (short)floor(interp.x_dist[i]);
      interp.x_dist[i] -= interp.x_table[i];
    }
  }
  
      
}
















