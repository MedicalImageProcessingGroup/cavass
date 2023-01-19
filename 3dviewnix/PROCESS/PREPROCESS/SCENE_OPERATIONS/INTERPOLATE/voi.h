/*
  Copyright 1993-2008 Medical Image Processing Group
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

 
 



#define PI M_PI
#define LEFT_BUTTON 1
#define MIDDLE_BUTTON 2
#define RIGHT_BUTTON 3

extern Display *display ; /* a pointer to the Display structure */
extern long screen ; /* screen identification */
extern Window img, dial, butt ; /* window identifications */
 
 
 
typedef struct IMG_TREE {
   short dimension;     /* Dimention of the stucture    */
   short unit;          /* units associated with the data */
   float loc;           /* actual location of the data  */
   int children;        /* number of children           */
   struct IMG_TREE **child; /* children fo this node    */
   unsigned long ptr;   /* position relative to the begining 
                           of scene data set 		*/ 
   int size;            /* size of the data in bytes    */
} Img_Tree;
 
typedef struct SCENE_INFO {
  Img_Tree *tree,       /* Tree stucture of the image   */
           *current;    /* Current structure being used: 
                           could be slice,volume 	*/
  char file_name[30];   /* File name                    */
  ViewnixHeader vh;     /* File header                  */
  FILE *fp;             /* File pointer                 */
  short *slice_index,   /* current slice coords (-1 when 
                           undefined)			*/
        *min_index,     /* min slice coord for each dimension (default 0)  */
        *max_index,     /* max slice coord for each dimension  */
        *incr_index;    /* increment in each dimension  */
  int width,height,     /* slice dimentions             */
      pix_size;         /* bits per pixel               */
  unsigned long min,max;
  unsigned long bytes_per_slice;  
  float pix_width,pix_height; /* actual size of the pixel */
  char *data;                 /* current stucts data block  if loaded */
  char DATA_LOADED;           /* flag to indecate if current data is loaded */ 
} Scene_Info;
 
extern Scene_Info *scene;
extern int num_scenes;
extern char *data;
extern int error; 
extern int DATA_TYPE;
extern int INTERPOLATE_FLAG;

/* Structures for building interpolation tables  */
typedef struct TABLE_INFO {
  short  old[2],     /* old dimensions  */
         new[2];     /* new dimensions  */
  short  offset[2],  /* Top left pos of image to be displayed */
         dim[2];     /* dimensions of the part that is displayed */
  short *index_table[2];  /* for each pix in new index into old (lower value)*/
  unsigned int   *dist_table[2];   /* units = old_pix_w/h *old_w/h */
} Table_Info;
 
extern Table_Info *inter_table;

typedef struct MAG_IMG_STRUCT {
  int scene_num;
  short *slice_index;
  int x,y,			/* screen loc of zoomed roi */
      width,height;		/* size of roi */
  int img_offset[2],		/* offset of roi in img. */
      img_dim[2];		/* diemnsions of roi in img. */
  int dx,dy,			/* upper left cprner of dsiplayed area */
      dx1,dy1;			/* lower right corner of displayed ares */
  int zoom_width ;		/* full zoomed_width of the whole image */
  Table_Info tbl;		/* table for interpolation */
  unsigned char *data;		/* data pointer */
  char locked;			/* flag if the slice is changing */
  Window win;			/* image window */
  int filter_type;	    /* type of filter used (MODIFICATION OF ORIGINAL) */
  float param;			/* parameter for filter (gaussian only) */
  struct MAG_IMG_STRUCT *next;
} MagImg;

typedef struct MAG_BOX {
  XImage *TOP,*BOT,*LEFT,*RIGHT;
  short x,y,x1,y1;
  GC gc;
  MagImg *img;
  unsigned top:1,bot:1,left:1,right:1;
} MagBox;



 
#define	INPUT			0x00000000
#define	VOI			 0x01000000
#define	VOI_ROI			0x01010000
#define	VOI_IOI			0x01020000
#define	INTERPOLATE		0x02000000
#define	FILTER			0x03000000
#define	FILTER_MEDIAN		0x03010000
#define	FILTER_GRADIENT		0x03020000
#define	FILTER_GAUSSIAN		0x03030000
#define	FILTER_CANNY		0x03040000
#define	FILTER_MORPHOLOGICAL	0x03050000
#define	SEGMENT			0x04000000
#define	SEGMENT_THRESHOLD	0x04010000
#define	SEGMENT_MASK		0x04020000
#define	SEGMENT_MASK_2D		0x04020100
#define	SEGMENT_MASK_3D		0x04020200
#define	SEGMENT_2DFEATURES	0x04030000
#define	GREYTRANSFORMS		0x05000000
#define	GREYTRANSFORMS_CONSTANT	0x05010000
#define	GREYTRANSFORMS_DISTANCE	0x05020000
#define	QUIT			0x06000000


#define  LEVEL1_MASK         0xff000000
#define  LEVEL2_MASK         0x00ff0000
#define  LEVEL3_MASK         0x0000ff00
#define  LEVEL4_MASK         0x000000ff






#define CYCLE_DISPLAY_CINE 0
#define CYCLE_DISPLAY_CONT 1
#define CYCLE_DISPLAY_DISCONT 2






