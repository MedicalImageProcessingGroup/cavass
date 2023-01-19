/*
  Copyright 1993-2011 Medical Image Processing Group
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

 
 

/************************************************************************
 *                                                                      *
 *      File            : LIB_TYPEDEF.H                                 *
 *      Description     : To be kept in the LIBRARY dir. Local file.    *
 *                        Include file for external common variables of *
 *                        3DVIEWNIX library software package.           *
 *      Return Value    :  None.                                        *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on February 28, 1989 by Hsiu-Mei Hung *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/

extern int lib_image_cursor,lib_dialog_cursor,lib_button_cursor; /* cursor types for the three windows */
typedef struct {
        Window win ;		  	/* window id */
	Pixmap pixmap ;			/* pixmap ID */
	short x, y;		  	/* window location */
	short width, height; 	  	/* window width and height */
	XFontStruct *font ;  	  	/* font id */
	short font_type ;		/* 0 - x default font, 
					   1 - fontname selected from user */
	short font_width, font_height;	/* font width and height */
        GC gc, pixmap_gc ;	  	/* image graphic context id */
	GC ovl1_gc, pixmap_ovl1_gc ; 	/* overlay 1 graphic context id */
 	GC ovl2_gc, pixmap_ovl2_gc ;	/* overlay 2 graphic context id */ 
} WindowGcInfo ;

typedef struct {
	Window win ;  /* scale subwindow id */
	GC gc ;
	Pixmap pixmap ;
	GC pixmap_gc ;
	short win_x, win_y ; /* scale subwindow upper left coordinate. */
	short win_width, win_height ; /* scale subwindow size */
	char label[30] ;
	short label_x, label_y ; /* upper left coordinate of label in
				    dialog window. */
	short x, y ; /* x, y coordinates of scale in dialog window. */
	short width, height ; /* scale size including layout */
	short decimal ; /* number of fraction number to be used after
			   decimal point. */
	char fmt[10] ; /* output format for all values in scale */
	short step ; /* the resolution step */
	float low_min, low_max ; /* the min and max values for the lowest 
				    resolution */
	short low_min_x, low_min_y ; /* the x, y location of the min value 
					string of the lowest resolution */
	short low_max_x, low_max_y ; /* the x, y location of the max value 
					string of the lowest resolution */
	float high_min, high_max ; /* the min and max values for the current
				      resolution. if current resolution is 
				      the lowest one, the values will be	
				      undefined. */
	short high_min_x, high_min_y ; /* the x, y location of the min value 
					  string of the current resolution.
					  if the current resolution is the 
					  lowest one, these values will be 
					  undefined which are -1. */
	short high_max_x, high_max_y ; /* the x, y location of the max value 
					  string of the current resolution.
					  if the current resolution is the 
					  lowest one, these values will be 
					  undefined which are -1. */
	short high_x, high_y ; /* the x, y location of the current resoluntion.
				  if the current resolution is the lowest one, 
			    	  the values will be undefined which are -1. */
	short high_width ; /* the width of the high resoluntion.*/
	short cursor_x, cursor_y ; /* the current cursor position */
	float value ; /* the current scale value */
	short value_x, value_y ; /* the x, y location of the value string */
} ScaleInfo ;

typedef struct {
	short x ;
	short width ;
} PanelSwitchInfo ;

typedef struct {
	unsigned short group ; /* group # */
	unsigned short elem ; /* element # */
	char type[3] ;	/* data type */
} ItemInfo ;

typedef struct {
        short x, y ;   /* x, y coordinates of curve */
        char vertex ;  /* flag to indicate if this point is vertex or not */
} PointInfo ;

/*************************notes related***********************/
#define MAX_FILE_NAMES 70
 
typedef struct FILE_INFO_LIST {
  FileInfo finfo;
  char SELECTED;
} FileInfoList;
 
typedef struct FILE_INFO {
  char full_file_name[30];   /*full file name with extension*/
  char file_name[30];   /*file name without extension*/
  char ftype[4];        /*file extension*/
  char month[15];       /*month in the date*/
  char day[5];          /*day in the date*/
  char time[8];         /*time in the date*/
  char size[15];        /*size of the file*/
  ViewnixHeader vh;     /*File header*/
  FILE *fp;             /*File pointer*/
  int flag;             /*to find out if a file is selected or not*/
  int dim;
  int minx3;
  int maxx3;
  int incrx3;
  int minx4;
  int maxx4;
  int incrx4;
  int xloc;
  int yloc;	
} File_Info;
 
extern File_Info *finfo;

/***************************************************************/
extern struct int_endian_struct {
  char c[4];
} lib_int_test;

extern struct short_endian_struct {
  char c[2];
} lib_short_test;

/***************************************************************/



