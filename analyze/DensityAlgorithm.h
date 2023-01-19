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

//======================================================================
/**
 * \file   DensityAlgorithm.h
 * \brief  DensityAlgorithm definition.
 * \author Xinjian Chen, Ph.D
 *
 * Copyright: (C) 2008
 *
 * 
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __DensityAlgorithm_h
#define __DensityAlgorithm_h

/* Structures for building interpolation tables  */
typedef struct TABLE_INFO {
  short  oldDim[2],     /* old dimensions  */
         newDim[2];     /* new dimensions  */
  short  offset[2],  /* Top left pos of image to be displayed */
         dim[2];     /* dimensions of the part that is displayed */
  short *index_table[2];  /* for each pix in new index into old (lower value)*/
  unsigned int   *dist_table[2];   /* units = old_pix_w/h *old_w/h */
} Table_Info;
 

/* structures used in layout and roi */
typedef struct MAG_IMG_STRUCT 
{
	int scene_num;
	short *slice_index;
	int x,y,			/* screen loc of zoomed roi */
		width,height;		/* size of roi */
	int img_offset[2],		/* offset of roi in img. */
		img_dim[2];		/* diemnsions of roi in img. */
	int dx,dy,			/* upper left corner of displayed area */
		dx1,dy1;			/* lower right corner of displayed ares */
	int zoom_width ;		/* full zoomed_width of the whole image */
	Table_Info tbl;		/* table for interpolation */
	unsigned char *data;		/* data pointer */
	char locked;			/* flag if the slice is changing */
	// Window win;			/* image window */
	struct MAG_IMG_STRUCT *next;
} MagImg;

/* temp structure used for drawing bounding boxes */
typedef struct MAG_BOX 
{
	//  XImage *TOP,*BOT,*LEFT,*RIGHT;
	short x,y,x1,y1;
	GC gc;
	MagImg *img;
	unsigned top:1,bot:1,left:1,right:1;
} MagBox;

typedef	struct Z_POINT{
	int      num;
	short *index;
	int      x,y;
	struct Z_POINT *next;
} zpoint;

//class DensityCompute
/** \brief DensityCompute .
 */
class DensityCompute
{
public:
    DensityCompute ( void );  
    ~DensityCompute ( void );
 
	MagImg *icon;
	MagImg *magimg;
	struct Z_POINT *icon_zpoint;
	struct Z_POINT *prev_point;
	struct Z_POINT *temp_point;
	struct Z_POINT *point;
	int    *first_point;
	int    cur_vol;
	int    very_first_point;
	unsigned short no_of_icon_points;
	unsigned int plot_points;
	char   process[20];
	int    FIRST_POINT;	
	short   *graph_data;
	MagBox  graph_box;
	MagBox  measure_dist;
	MagBox  first;
	MagBox  second;
	int     SCENE_NUM;
	int     VOLUME_NUM[2];
	int     SLICE_NUM[2];
	int     num_of_scenes;

	int     DrawImgLine(const wxPoint  pos, int dimension);

	
protected:

private:
  

  

    DECLARE_DYNAMIC_CLASS(DensityCompute)
    DECLARE_EVENT_TABLE()
};

#endif
