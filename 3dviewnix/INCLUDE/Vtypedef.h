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
#ifndef __Vtypedef_h
#define __Vtypedef_h
 
 
 

/************************************************************************
 *                                                                      *
 *      File            : TYPEDEF.H                                     *
 *      Description     : To be kept in the INCLUDE dir. Global file.   *
 *      Return Value    : None.                                         *
 *      Parameters      : None.                                         *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on February 28, 1989 by Hsiu-Mei Hung *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
typedef struct {
	unsigned long pixel;		/*pixel value*/
	unsigned short  red,green,blue;	/*RGB values*/
} ViewnixColor;
 
typedef char Char30[300];

typedef struct {
	Char30 filename;	/*input filename*/
	short num_of_elem;	/*the no. of elem. in min/max arrays(sd-2)*/
	short *min;		/*the min no. of n-1 scenes for each n*/
	short *max;		/*the max no. of n-1 scenes for each n*/
	short *incr;
} FileInfo ;

typedef struct {
	short num_of_ifiles;	/*no. of file selected from INPUT command*/
	FileInfo *ifiles;	/*array fo files selected from INPUT command*/
} GlobalInfo ;

typedef struct {
        short on;		/*horizontal menu on(1)/off(0)*/
        short item_selected;	/*horizontal menu selected*/
        short cmd_wd;		/*the width of horizontal command*/
} HorizontalMenuInfo;

typedef struct {
        short up;		/*vertical menu on(1)/off(0)*/
        short x,y;		/*top-left cood of vertical menu*/
        short width;		/*the width of the vertical menu*/
        short height;		/*the height of the vertical menu*/
} VerticalMenuInfo;

typedef struct {
        char cmd[30];		/*the current command selected*/
        char function[30];	/*the current function selected*/
        char process[30];	/*the current process being executed*/
        char filetype[100];	/*the types of files to be read*/
        char html_link[100];	/*the associated html file to be read*/
        short father;		/*the father fo the current process*/
        short sibling;		/*the first sibling of the current process*/
        short son;		/*the first child of the current process*/
        short x,y;		/*the top-left cood of the current command*/
        short terminal_leaf_node;/*terminal node(1) or not(0)*/
} TreeInfo;

typedef struct {
	short  group; 		/*the group number*/
	Char30 cmd; 		/*the name of command*/
	short  type; 		/*0 - button type; 1 - switch type.*/ 
	short  num_of_switches;	/*the number of switches per command*/
	Char30 *switches;	/*the names of the switches*/
	short  switch_item_displayed; /*the switch item to be displayed*/ 
} PanelCmdInfo;

typedef struct {
	Char30 machine_name;	/*machine on which tape is mounted*/
	Char30 tape_drive_name;	/*name of tape drive*/
	Char30 mt_option;	/*option to be used with mt command*/
	Char30 tape_size;	/*kind of drive (1/4,1/2inch,8mm)*/
} TapePathName;

typedef struct {
	short x,y;		/*the top-left coordinate of Click button*/
	short width;		/*the width of the Click button*/
	short height;		/*the height of the Click button*/
} MarginClickInfo;

typedef struct {
        short top_x,top_y;	/*the top-left cood of top Click button*/
        short top_width;	/*the width of the top Click button*/
        short top_height;	/*the height of the top Click button*/
	short bottom_x,bottom_y;/*the top-left cood of botton Click button*/
	short bottom_width;	/*the width of the bottom Click button*/
	short bottom_height;	/*the height of the bottom Click button*/
} ScrollbarClickInfo;

typedef struct _text {
        Window win;    		/*window in which item resides*/
        GC     *gc;
        int     x,y;    	/*location of upper-left corner of item*/
        int     w;      	/*width of the total item area in fonts*/
        int     h;      	/*height of the total item area in  PIXELS*/
        short   label_width; 	/*width of label in pixels*/
        short	label_height; 	/*height of label*/
        short	label_ascent;	/*ascent dimension of font*/
        short	label_x;	/*position of the label*/
        short	label_y;
	int 	thick;		/*thickness of the frame of the item*/
        int     width;  	/*current width of item in pixels*/
        int     fw,fh;  	/*size of font*/
        char    label[150]; 	/*label of the text item*/
        char    value[200]; 	/*value of the text item*/
	int	pos;		/*pos indicates the index of the first VALUE*/
        int     state;  	/*indicates if text item is OFF(0) or ON(1)*/
        int     mode;   	/*text item is DISABLED(0) or ENABLED (1)*/
        int     (*func)(void*);  	/*notify procedure*/
        struct _text *next; 	/*next text item*/
	XFontStruct *font_struct;	/*Font used on the window*/
        } TEXT;
 
typedef struct _button {
        Window  win;    	/*window in which item resides*/
        short   type;   	/*type of button(0=button, 1=toggle, etc..)*/
        short   x,y;    	/*location of upper-left corner of item*/
        short   w;      	/*width of the item area in fonts*/
        short   h;      	/*height of the item area in fonts*/
        short   label_width; 	/*width of label in pixels*/
        short   label_height; 	/*height of label*/
        short   label_ascent;	/*ascent dimension of font*/
        short   label_x;	/*position of the label*/
        short   label_y;
        short   thick;  	/*thickness of frame*/
        short   width;  	/*width of item in pixels*/
        short   height;  	/*height of item in pixels*/
        int     fw,fh;  	/*size of font*/
        char    label[150]; 	/*label of the text item*/
        short   state;  	/*indicates if text item is OFF(0) or ON(1)*/
        short   mode; 		/*0=RELEASED, 1=PRESSED*/
        int     (*func)();  	/*notify procedure*/
        struct _button *next; 	/*next text item*/
        } BUTTON;

#endif
