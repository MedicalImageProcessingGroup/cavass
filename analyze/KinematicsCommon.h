/*
  Copyright 1993-2009 Medical Image Processing Group
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

#ifndef __KinematicsCommon_h
#define __KinematicsCommon_h

#include  <Viewnix.h>
//#include "cavass.h"
 
#ifndef PI
#define PI M_PI
#endif
#define GR_SHADE_LEN 512
#define MAX_PANELS 40
typedef double KMMATRIX[4][4];
typedef double KMPOINT[3];

typedef unsigned short Cord_with_Norm[2];

typedef struct MeasurePoints {
  double x,y,z;
  struct MeasurePoints *next;
} MPTS;
typedef struct TextTags {
  double x,y,z;
  char msg;
  struct TextTags *next;
} TEXTTAGS;


typedef struct SURF_TREE {
  short level;              // level in the tree. objects would be at the leaves /
  struct SURF_TREE *parent; // parent of current level /
  short children;           // number of children /
  struct SURF_TREE **child; // pointers to children (NULL if children==0) /
  short surf_index;         // index into the surface structures /
  KMMATRIX transf;            // global transformation at the current level /
} Surf_Tree;


typedef struct SURF_STRUCT {
  long start;       // Begining of the TSE part in the Data segment  /
  char DISPLAY_FLAG;      // Is this structure displayed /
  char DATA_LOADED;       // Flag to indecate if data is in memory //

  unsigned short r,g,b;   // color of the structure /
  int  color_seg;          // color_segment to map the object /
  long *seg_map;
  int seg_map_size; 
  long *seg_plane_map;
  int seg_plane_map_size; 

  float opacity;
  float spec_div,diff_div;
  float spec_exp,diff_exp;
  float spec_ratio;
  float gr_shade[GR_SHADE_LEN];

  unsigned short slices,rows,col_min,col_max;
  Cord_with_Norm ***node; // table pointing to the Slice,Row and Element /
  unsigned short **count;

  char AXIS_FLAG;
  double orig[3],axis[3][3]; // axis system of surface relative
			                  //to the scanner coordinate system /

  KMMATRIX to_scanner;
  float min_x,min_y,min_z;
  float max_x,max_y,max_z;
} Surf_Struct;


  
typedef struct PATCH_INFO {
  short num_lines;    // number of vertical lines in image /
  short offset;       // vertical offset from center of face //
  short *line_offset,*len;
} Patch_Info;



typedef struct SURF_INFO 
{
  char plan_file[200];  
  char plan_on;

  char file_name[200];
  ViewnixHeader vh;
  FILE *fp;

  Surf_Tree *tree;
  short *tree_index;  
  char  *index_lock;

  int num_structs;
  Surf_Struct *surf_struct;

  KMMATRIX aft; 
  KMMATRIX icon_aft;
  KMMATRIX mat;

  double Px,Py,Pz;
  double max_pix_size,pix_size;

  double min_x,max_x;  // bonding box limits /
  double min_y,max_y;
  double min_z,max_z;
  double cx,cy,cz;     // bounding box center /
  double diag;

  // Display variables //
  char WINDOW_ON;
  Window win;

  char ANTIALIAS_FLAG;
  int HI_RESOLUTION;
  int x,y;  // top left location of image /
  int off_x,off_y,dim_x,dim_y; // Region occupying the screen //
  int img_size,icon_size;

  unsigned char *img_buf;
  float *z_buf;
  unsigned char *obj_buf;
  float *(xmat[3]),*(ymat[3]),*(zmat[3]);
  int *img_tbl;  

  // Patch Variables /
  struct PATCH_INFO xp,yp,zp;

  // image size when it is displayed in real size /
  short life_size;
  
  // Measurement variables /
  MPTS *mpt;
  int num_mpt;
  double seg_length,full_length,angle;

  // Text Tag Info /
  TEXTTAGS *txt_list;
  int num_tags;

  // plane params //
  float pl1[3],pl2[3],pl3[3],pl4[3];

} Surf_Info;


typedef struct DEFAULTS {
  unsigned short r,g,b;
  float opacity;
  float spec_ratio,spec_exp,spec_div;
  float diff_exp,diff_div;
  unsigned short amb_r,amb_g,amb_b;
  unsigned short bg_r,bg_g,bg_b;
  unsigned short box_r,box_g,box_b;
} DEFS;


#endif

















