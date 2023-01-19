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

/************************************************************************
 *                                                                      *
 *      Filename  : utils.c                                          	*
 *      Ext Funcs : VDisplayCaptionBar,VDisplayBox,			*
 *		    VDisplayDoubleScrollbarBox,VDisplayFixedColorBox,	*
 *		    VDisplayCenteredText,VDisplayFixedColorCenteredText,		*
 *		    VDisplayText,VDisplayFixedColorText,VBlinkBox,	*
 *		    VDisplayXORBox.				*
 *      Int Funcs : None 						*
 *                                                                      *
 ************************************************************************/

#include "Vlibrary.h"
#include <sys/types.h>
#include <unistd.h>
#include "3dv.h"

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayCaptionBar                               	*
 *      Description     : This function creates the top margin for a 	*
 *			  three-dimensional window. It also creates a	*
 *			  click point in the margin and returns the	*
 *			  coordinates of the click point. The user can	*
 *		          specify the label for the margin also.	*
 *      Return Value    :  0 - work successfully.                       *
 *                         280 - the click point was not specified.	*
 *      Parameters      :  win - the ID of the window to draw the margin*
 *			   gc - the GC to be used to draw the margin;	*
 *				If no GC is passed it is obtained from  *
 *				the window ID.				*
 *			   marginlabel - the label to be displayed in 	*
 *			   	the margin.				*
 *			   xloc,yloc - the xloc,yloc of the margin with *
 *				respect to the window.			*
 *			   width,height - the width and height of the	*
 *				margin with respect to the window. If no*
 *				height is specified, then a default	*
 *				height is assumed.			*
 *			   click - the structure which is returned	*
 *				with the coordinates of the click point *
 *				with respect to the window.		*
 *      Side effects    : None.						*
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.	*
 *      Related funcs   : None.						*
 *      History         : Written on June 1, 1993 by Krishna Iyer.	*
 *                                                                      *
 ************************************************************************/
int VDisplayCaptionBar ( Window win, GC gc, char* marginlabel,
    int xloc, int yloc, int width, int height, MarginClickInfo* click )
{
	GC gcmargin;
        int offset=2;
        int n,dum,error;
        short label_width,label_height,label_x,label_y,label_ascent;
        XCharStruct overall;
        XTextItem tlabel;
	int click_point=TRUE;
 
	if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayCaptionBar.\n");
	    printf("Please call VCreateColormap before "); 
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }

	if(win == NULL)
            v_print_fatal_error("VDisplayCaptionBar",
                "Window ID was NULL.",win);

	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gcmargin);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gcmargin=gc;
        }

	if(marginlabel == NULL) strcpy(marginlabel,"");
        if(height==NULL) height=LIB_MARGIN_HEIGHT;
	if(click == NULL) click_point=FALSE;
 
         /*******************top margin*********************/
        XSetForeground(lib_display,gcmargin,lib_reserved_colors[1].pixel);
        XFillRectangle(lib_display,win,gcmargin,
                           xloc+offset,yloc+offset,
                           width-offset,height);
        XFlush(lib_display);
         /***********bottom dark lines***********/
            XSetForeground(lib_display,gcmargin,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcmargin,
                        xloc,
                        yloc+height,
                        xloc+width,
                        yloc+height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcmargin,
                        xloc+width,
                        yloc+height,
                        xloc+width,
                        yloc);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcmargin,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcmargin,
                        xloc,
                        yloc,
                        xloc+width,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcmargin,
                        xloc,
                        yloc,
                        xloc,
                        yloc+height);
            XFlush(lib_display);
        /*******************end top margin***************/
 
        /***************margin label*******************/
        n = strlen(marginlabel);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gcmargin),
                        marginlabel, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }
 
        label_x = (short)xloc + (width - label_width)/2;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent;
 
        tlabel.chars = &marginlabel[0];
        tlabel.nchars = strlen(marginlabel);
        tlabel.delta = 0;
        tlabel.font = None;
 
        XSetForeground(lib_display,gcmargin,lib_reserved_colors[0].pixel);
        XDrawText(lib_display,win,gcmargin,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
        /*******************click point***************/
       if(click_point==TRUE)
       {

        click[0].width=3*height/4;
        click[0].height=click[0].width;
        click[0].x=click[0].width*2;
        click[0].y=height/8;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcmargin,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcmargin,
                        click[0].x,
                        click[0].y+click[0].height,
                        click[0].x+click[0].width,
                        click[0].y+click[0].height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcmargin,
                        click[0].x+click[0].width,
                        click[0].y+click[0].height,
                        click[0].x+click[0].width,
                        click[0].y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcmargin,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcmargin,
                        click[0].x,
                        click[0].y,
                        click[0].x+click[0].width,
                        click[0].y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcmargin,
                        click[0].x,
                        click[0].y,
                        click[0].x,
                        click[0].y+click[0].height);
            XFlush(lib_display);
        /*******************end click point***************/

	if(lib_internal_gc==1) free(gcmargin);
       	return(0);

       }
       else 
       {
	if(lib_internal_gc==1) free(gcmargin);
	return(280);
       }
}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayBox                              *
 *      Description     : This function creates a three-dimensional	*
 *			  box (depressed) or a button. When a box is 	*
 *			  created, it can be created with a scrollbar.	*
 *			  If a scrollbar is desired, then the top and	*
 *			  bottom click points of the scroll bar are	*
 *			  returned.					*
 *      Return Value    :  0 - work successfully.                       *
 *			   281 - could not create box or button.	*
 *                         282 - scrollbar click point was not created. *
 *      Parameters      :  win - the ID of the window to draw the box	*
 *				or button in.				*
 *                         gc - the GC to be used to draw the same;   	*
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         xloc,yloc - the xloc,yloc of the box/button 	*
 *                              with respect to the window.             *
 *                         width,height - the width and height of the   *
 *                              bix/button with respect to the window. 	*
 *			   button_flag - to indicate that it is a 	*
 *				button.					*
 *			   scrollbar_flag - to indicate that a scroll	*
 *				is needed. 				*
 *			   rightscrollbar_width - the width of the	*
 *				scrollbar. If no width is specified	*
 *				then a default is used. The scrollbar is*
 *				always created on the right side.	*
 *                         click - the structure which is returned      *
 *                              with the coordinates of the click point *
 *                              with respect to the window.             *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height, int button_flag, int scrollbar_flag,
    int rightscrollbar_width, ScrollbarClickInfo* click )
{
 
	GC gcbox;
        int xloc2,yloc2,offset,scrollbar_width;
	int error;
	int click_point=TRUE;
 
        xloc2 = xloc+width;
        yloc2 = yloc+height;
        offset = 1;

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayBox.\n");
	    printf("Please call VCreateColormap before ");
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayBox",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gcbox);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gcbox=gc;
        }
	
	if(rightscrollbar_width == NULL)
                scrollbar_width=LIB_SCROLLBAR_WIDTH;
        else scrollbar_width=rightscrollbar_width;

	if(click == NULL) click_point=FALSE;
 
        /***********fill the rectangles***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,win,gcbox,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top dark lines***********/
            if(button_flag==1)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            else if(button_flag==0)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
	    else return(281);
            XDrawLine(lib_display,win,gcbox,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
        /***********bottom white lines***********/
            if(button_flag==1)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            else if(button_flag==0)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
	    else return(281);
            XDrawLine(lib_display,win,gcbox,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
      if(scrollbar_flag == 1)
      {
        /*************scroll bar******************/
         /***********top dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-scrollbar_width,
                        yloc,
                        xloc2,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-scrollbar_width,
                        yloc,
                        xloc2-scrollbar_width,
                        yloc2);
            XFlush(lib_display);
        /***********bottom white lines***********/
/*
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc2,
                        yloc2,
                        xloc2+width,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc,
                        yloc,
                        xloc,
                        yloc+LIB_MARGIN_HEIGHT);
            XFlush(lib_display);
*/
        /*******************end top margin***************/
 
       if(click_point==TRUE)
       { 
        /*******************click point***************/
        /***********bottom click point**********/
        click[0].top_x=xloc2-scrollbar_width+2;
        click[0].top_y=yloc+2;
        click[0].top_width=scrollbar_width-4;
        click[0].top_height=click[0].top_width;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x,
                        click[0].top_y+click[0].top_height,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y+click[0].top_height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y+click[0].top_height,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x,
                        click[0].top_y,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x,
                        click[0].top_y,
                        click[0].top_x,
                        click[0].top_y+click[0].top_height);
            XFlush(lib_display);
 
 
        /***********bottom click point**********/
        click[0].bottom_x=xloc2-scrollbar_width+2;
        click[0].bottom_width=scrollbar_width-4;
        click[0].bottom_height=click[0].bottom_width;
        click[0].bottom_y=yloc2-click[0].bottom_height-2;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x,
                        click[0].bottom_y+click[0].bottom_height,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y+click[0].bottom_height);
            XFlush(lib_display);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y+click[0].bottom_height,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x,
                        click[0].bottom_y,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x,
                        click[0].bottom_y,
                        click[0].bottom_x,
                        click[0].bottom_y+click[0].bottom_height);
            XFlush(lib_display);
 
        /*******************end click point***************/
	if(lib_internal_gc==1) free(gcbox);
	return(0);
	}

	else
	{	
		if(lib_internal_gc==1) free(gcbox);
		return(282);
	}
      }

      else
      {
	if(lib_internal_gc==1) free(gcbox);
      	return(0); 
      }
}
 

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayDoubleScrollbarBox                       *
 *      Description     : This function creates a three-dimensional     *
 *                        box (depressed) with scroll bars on both 	*
 *			  sides. The click points for scroll bars on	*
 *			  either side is returned.			*
 *      Return Value    :  0 - work successfully.                       *
 *                         282 - scrollbar click point was not created. *
 *      Parameters      :  win - the ID of the window to draw the box   *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         xloc,yloc - the xloc,yloc of the box		*
 *                              with respect to the window.             *
 *                         width,height - the width and height of the   *
 *                              box with respect to the window.  	*
 *                         leftscrollbar_width - the width of the      	*
 *				scrollbar on the left side. If no 	*
 *				width is specified, then a default value*
 *				is assumed.				*
 *                         rightscrollbar_width - the width of the right*
 *                              scrollbar. If no width is specified     *
 *                              then a default is used. 		*
 *                         left_click - the structure which is returned *
 *                              with the coordinates of the click point *
 *                              for the left scrollbar with respect to 	*
 *				the window.             		*
 *                         right_click - the structure which is returned*
 *                              with the coordinates of the click point *
 *                              for the right scrollbar with respect to *
 *                              the window.                             *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayDoubleScrollbarBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height,
    int leftscrollbar_width, int rightscrollbar_width,
    ScrollbarClickInfo* left_click, ScrollbarClickInfo* right_click )
{

	GC gcbox;
	int left_click_point=TRUE;
	int right_click_point=TRUE;
	int xloc2,yloc2,xloc3,offset;
	int l_scrollbar_width,r_scrollbar_width;
	int error;
 
	if(lib_cmap_created == 0) {
            printf("The error occurred in the function ");
	    printf("VDisplayDoubleScrollbarBox.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayDoubleScrollbarBox",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gcbox);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gcbox=gc;
        }

	if(leftscrollbar_width == NULL)
                l_scrollbar_width=LIB_SCROLLBAR_WIDTH;
        else l_scrollbar_width=leftscrollbar_width;
 
        if(rightscrollbar_width == NULL)
                r_scrollbar_width=LIB_SCROLLBAR_WIDTH;
        else r_scrollbar_width=rightscrollbar_width;

	if(left_click == NULL) left_click_point=FALSE;
	if(right_click == NULL) right_click_point=FALSE;

        xloc2 = xloc+width;
        yloc2 = yloc+height;
	xloc3 = xloc+l_scrollbar_width;
        offset = 1;

	/***********fill the rectangles***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,win,gcbox,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
        /***********bottom white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
	 /*************left scroll bar******************/
         /***********rightside white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc3,
                        yloc,
                        xloc3,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc3,
                        yloc+offset,
                        xloc3,
                        yloc2-offset);
            XFlush(lib_display);
        /***********bottom white lines***********/
/*
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc3,
                        yloc2,
                        xloc3+width,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc,
                        yloc,
                        xloc,
                        yloc+LIB_MARGIN_HEIGHT);
            XFlush(lib_display);
*/
        /*******************end top margin***************/
 
 
      if(left_click_point==TRUE)
      {
        /*******************click point***************/
        /***********top click point**********/
        left_click[0].top_x=xloc3-l_scrollbar_width+2;
        left_click[0].top_y=yloc+2;
        left_click[0].top_width=l_scrollbar_width-4;
        left_click[0].top_height=left_click[0].top_width;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].top_x,
                        left_click[0].top_y+left_click[0].top_height,
                        left_click[0].top_x+left_click[0].top_width,
                        left_click[0].top_y+left_click[0].top_height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].top_x+left_click[0].top_width,
                        left_click[0].top_y+left_click[0].top_height,
                        left_click[0].top_x+left_click[0].top_width,
                        left_click[0].top_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].top_x,
                        left_click[0].top_y,
                        left_click[0].top_x+left_click[0].top_width,
                        left_click[0].top_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].top_x,
                        left_click[0].top_y,
                        left_click[0].top_x,
                        left_click[0].top_y+left_click[0].top_height);
            XFlush(lib_display);
 
 
        /***********bottom click point**********/
        left_click[0].bottom_x=xloc3-l_scrollbar_width+2;
        left_click[0].bottom_width=l_scrollbar_width-4;
        left_click[0].bottom_height=left_click[0].bottom_width;
        left_click[0].bottom_y=yloc2-left_click[0].bottom_height-2;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].bottom_x,
                        left_click[0].bottom_y+left_click[0].bottom_height,
                        left_click[0].bottom_x+left_click[0].bottom_width,
                        left_click[0].bottom_y+left_click[0].bottom_height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].bottom_x+left_click[0].bottom_width,
                        left_click[0].bottom_y+left_click[0].bottom_height,
                        left_click[0].bottom_x+left_click[0].bottom_width,
                        left_click[0].bottom_y);
            XFlush(lib_display);
 	/***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].bottom_x,
                        left_click[0].bottom_y,
                        left_click[0].bottom_x+left_click[0].bottom_width,
                        left_click[0].bottom_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        left_click[0].bottom_x,
                        left_click[0].bottom_y,
                        left_click[0].bottom_x,
                        left_click[0].bottom_y+left_click[0].bottom_height);
            XFlush(lib_display);
 
        /*******************end click point***************/
       }

 	/*************right scroll bar******************/
         /***********top dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-r_scrollbar_width,
                        yloc,
                        xloc2,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-r_scrollbar_width,
                        yloc,
                        xloc2-r_scrollbar_width,
                        yloc2-offset);
            XFlush(lib_display);
        /***********bottom white lines***********/
/*
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc2,
                        yloc2,
                        xloc2+width,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc,
                        yloc,
                        xloc,
                        yloc+LIB_MARGIN_HEIGHT);
            XFlush(lib_display);
*/
        /*******************end top margin***************/
 
 
       if(right_click_point==TRUE)
       {
        /*******************click point***************/
        /***********top click point**********/
        right_click[0].top_x=xloc2-r_scrollbar_width+2;
        right_click[0].top_y=yloc+2;
        right_click[0].top_width=r_scrollbar_width-4;
        right_click[0].top_height=right_click[0].top_width;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].top_x,
                        right_click[0].top_y+right_click[0].top_height,
                        right_click[0].top_x+right_click[0].top_width,
                        right_click[0].top_y+right_click[0].top_height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].top_x+right_click[0].top_width,
                        right_click[0].top_y+right_click[0].top_height,
                        right_click[0].top_x+right_click[0].top_width,
                        right_click[0].top_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].top_x,
                        right_click[0].top_y,
                        right_click[0].top_x+right_click[0].top_width,
                        right_click[0].top_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].top_x,
                        right_click[0].top_y,
                        right_click[0].top_x,
                        right_click[0].top_y+right_click[0].top_height);
            XFlush(lib_display);
 
 
        /***********bottom click point**********/
        right_click[0].bottom_x=xloc2-r_scrollbar_width+2;
        right_click[0].bottom_width=r_scrollbar_width-4;
        right_click[0].bottom_height=right_click[0].bottom_width;
        right_click[0].bottom_y=yloc2-right_click[0].bottom_height-2;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].bottom_x,
                        right_click[0].bottom_y+right_click[0].bottom_height,
                        right_click[0].bottom_x+right_click[0].bottom_width,
                        right_click[0].bottom_y+right_click[0].bottom_height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].bottom_x+right_click[0].bottom_width,
                        right_click[0].bottom_y+right_click[0].bottom_height,
                        right_click[0].bottom_x+right_click[0].bottom_width,
                        right_click[0].bottom_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].bottom_x,
                        right_click[0].bottom_y,
                        right_click[0].bottom_x+right_click[0].bottom_width,
                        right_click[0].bottom_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        right_click[0].bottom_x,
                        right_click[0].bottom_y,
                        right_click[0].bottom_x,
                        right_click[0].bottom_y+right_click[0].bottom_height);
            XFlush(lib_display);
 
        /*******************end click point***************/
       } 
	
	if(left_click_point==FALSE || right_click_point==FALSE)
	{
		if(lib_internal_gc==1) free(gcbox);
		return(282);
	}
	else {
		if(lib_internal_gc==1) free(gcbox);
		return(0);
	}
}
 

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayFixedColorBox                       *
 *      Description     : This function creates a three-dimensional     *
 *                        box (depressed) or a button. When a box is    *
 *                        created, it can be created with a scrollbar.  *
 *                        If a scrollbar is desired, then the top and   *
 *                        bottom click points of the scroll bar are     *
 *                        returned. The box/button is colored in the 	*
 *			  highlight color.				*
 *      Return Value    :  0 - work successfully.                       *
 *			   281 - could not create box or button.	*
 *                         282 - scrollbar click point was not created. *
 *      Parameters      :  win - the ID of the window to draw the box   *
 *                              or button in.                           *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         xloc,yloc - the xloc,yloc of the box/button  *
 *                              with respect to the window.             *
 *                         width,height - the width and height of the   *
 *                              bix/button with respect to the window.  *
 *                         button_flag - to indicate that it is a       *
 *                              button.                                 *
 *                         scrollbar_flag - to indicate that a scroll   *
 *                              is needed.                              *
 *                         rightscrollbar_width - the width of the      *
 *                              scrollbar. If no width is specified     *
 *                              then a default is used. The scrollbar is*
 *                              always created on the right side.       *
 *                         click - the structure which is returned      *
 *                              with the coordinates of the click point *
 *                              with respect to the window.             *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayFixedColorBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height, int button_flag, int scrollbar_flag,
    int rightscrollbar_width, ScrollbarClickInfo* click )
{
 
	GC gcbox;
	int click_point=TRUE;
        int xloc2,yloc2,offset,scrollbar_width;
	int error;
 
        xloc2 = xloc+width;
        yloc2 = yloc+height;
        offset = 1;

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function ");
	    printf("VDisplayFixedColorBox.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayFixedColorBox",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gcbox);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gcbox=gc;
        }

	if(rightscrollbar_width == NULL)
        	scrollbar_width=LIB_SCROLLBAR_WIDTH;
	else scrollbar_width=rightscrollbar_width;

	if(click == NULL) click_point=FALSE;
 
        /***********fill the rectangles***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[10].pixel);
            XFillRectangle(lib_display,win,gcbox,
                           xloc,yloc,xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top dark lines***********/
            if(button_flag==1)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            else if(button_flag==0)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
	    else return(281);
            XDrawLine(lib_display,win,gcbox,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
                /***********bottom white lines***********/
            if(button_flag==1)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            else if(button_flag==0)
                XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
	    else return(281);
            XDrawLine(lib_display,win,gcbox,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
      if(scrollbar_flag == 1)
      {
        /*************scroll bar******************/
         /***********top dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-scrollbar_width,
                        yloc,
                        xloc2,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc2-scrollbar_width,
                        yloc,
                        xloc2-scrollbar_width,
                        yloc2);
            XFlush(lib_display);
        /***********bottom white lines***********/
/*
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        xloc2,
                        yloc2,
                        xloc2+width,
                        yloc);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        xloc,
                        yloc,
                        xloc,
                        yloc+LIB_MARGIN_HEIGHT);
            XFlush(lib_display);
*/
        /*******************end top margin***************/
 
       if(click_point==TRUE)
       {
	/*******************click point***************/
        /***********bottom click point**********/
        click[0].top_x=xloc2-scrollbar_width+2;
        click[0].top_y=yloc+2;
        click[0].top_width=scrollbar_width-4;
        click[0].top_height=click[0].top_width;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x,
                        click[0].top_y+click[0].top_height,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y+click[0].top_height);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y+click[0].top_height,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x,
                        click[0].top_y,
                        click[0].top_x+click[0].top_width,
                        click[0].top_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].top_x,
                        click[0].top_y,
                        click[0].top_x,
                        click[0].top_y+click[0].top_height);
            XFlush(lib_display);
 
 
	/***********bottom click point**********/
        click[0].bottom_x=xloc2-scrollbar_width+2;
        click[0].bottom_width=scrollbar_width-4;
        click[0].bottom_height=click[0].bottom_width;
        click[0].bottom_y=yloc2-click[0].bottom_height-2;
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x,
                        click[0].bottom_y+click[0].bottom_height,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y+click[0].bottom_height);
            XFlush(lib_display);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y+click[0].bottom_height,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gcbox,lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x,
                        click[0].bottom_y,
                        click[0].bottom_x+click[0].bottom_width,
                        click[0].bottom_y);
            XFlush(lib_display);
            XDrawLine(lib_display,win,gcbox,
                        click[0].bottom_x,
                        click[0].bottom_y,
                        click[0].bottom_x,
                        click[0].bottom_y+click[0].bottom_height);
            XFlush(lib_display);
 
   	/*******************end click point***************/
	if(lib_internal_gc==1) free(gcbox);
	return(0);
       }

       else {
		if(lib_internal_gc==1) free(gcbox);
		return(282);
       }

      }
 
      else {
		if(lib_internal_gc==1) free(gcbox);
		return(0);
      }
}
 

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayCenteredText                             *
 *      Description     : This function creates a text centered the	*
 *			  box specified by xloc,yloc,width and height.	*
 *      Return Value    :  0 - work successfully.                       *
 *                         283 - could not draw text in the area 	*
 *				specified.				*
 *      Parameters      :  win - the ID of the window to draw the text. *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         xloc,yloc - the xloc,yloc of the box/button  *
 *                              with respect to the window.             *
 *                         width,height - the width and height of the   *
 *                              bix/button with respect to the window.  *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayCenteredText ( Window win, GC gc, char* text,
    int xloc, int yloc, int width, int height )
{
 
        XCharStruct overall;
        int n,dum;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;
	GC gctext;
	int error;
 
	if(lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayCenteredText.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayCenteredText",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gctext);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gctext=gc;
        }

	if(text==NULL) strcpy(text,"");

        but_height=(short)height; /*this is in pixels*/
        but_width=(short)width; /*this is in pixels*/
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gctext),
                        text, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }
 
        label_x = (short)xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;

	if(but_width<label_width || but_height<label_height) {
			if(lib_internal_gc==1) free(gctext);
			return(283);
	}

        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(lib_wins[0].font_height > 13)
          tlabel.font = None;
        else
          tlabel.font = lib_wins[0].font->fid;
 
        XSetForeground(lib_display,gctext,lib_reserved_colors[0].pixel);
        XDrawText(lib_display,win,gctext,label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
	if(lib_internal_gc==1) free(gctext);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayFixedColorCenteredText                      *
 *      Description     : This function creates a text centered the     *
 *                        box specified by xloc,yloc,width and height.  *
 *			  It then paints the text in a light color.	*
 *      Return Value    :  0 - work successfully.                       *
 *                         283 - could not draw text in the area        *
 *                              specified.                              *
 *      Parameters      :  win - the ID of the window to draw the text. *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *			   text - the text to be drawn.			*
 *                         xloc,yloc - the xloc,yloc of the box/button  *
 *                              with respect to the window.             *
 *                         width,height - the width and height of the   *
 *                              bix/button with respect to the window.  *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayFixedColorCenteredText ( Window win, GC gc, char* text,
    int xloc, int yloc, int width, int height )
{
 
	GC gctext;
        XCharStruct overall;
        int n,dum,error;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function ");
	    printf("VDisplayFixedColorCenteredText.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayFixedColorCenteredText",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gctext);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gctext=gc;
        }
 
        if(text==NULL) strcpy(text,"");
 
        but_height=(short)height; /*this is in pixels*/
        but_width=(short)width; /*this is in pixels*/
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gctext),
                        text, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }

        label_x = (short)xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
	if(but_width<label_width || but_height<label_height) {
		if(lib_internal_gc==1) free(gctext);
		return(283);
	}

        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(lib_wins[0].font_height > 13)
          tlabel.font = None;
        else
          tlabel.font = lib_wins[0].font->fid;
 
        XSetForeground(lib_display,gctext,lib_reserved_colors[5].pixel);
        XDrawText(lib_display,win,gctext,label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
	if(lib_internal_gc==1) free(gctext);
        return(0);
}
 

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayText                             	*
 *      Description     : This function creates a text in the location	*
 *			  (xloc,yloc) specified.			*
 *      Return Value    :  0 - work successfully.                       *
 *                         283 - could not draw text in the area        *
 *                              specified.                              *
 *      Parameters      :  win - the ID of the window to draw the text. *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *			   text - the text to be drawn.			*
 *                         xloc,yloc - the xloc,yloc of the text to be  *
 *				drawn.					*
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayText ( Window win, GC gc, char* text, int xloc, int yloc )
{
 
	GC gctext;
        XCharStruct overall;
        int n,dum,error;
        short label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayText.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayText",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gctext);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gctext=gc;
        }
 
        if(text==NULL) strcpy(text,"");
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gctext),
                        text, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }
 
        label_x = (short)xloc;
        label_y = (short)yloc;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(lib_wins[0].font_width > 13)
           tlabel.font = None;
        else
           tlabel.font = lib_wins[0].font->fid;
 
 
        XSetForeground(lib_display,gctext,lib_reserved_colors[0].pixel);
        XDrawText(lib_display,win,gctext,label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
	if(lib_internal_gc==1) free(gctext);
        return(0);
}
 

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayText2                                    *
 *      Description     : This function creates a text in the location  *
 *                        (xloc,yloc) specified.                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         283 - could not draw text in the area        *
 *                              specified.                              *
 *      Parameters      :  win - the ID of the window to draw the text. *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         text - the text to be drawn.                 *
 *                         xloc,yloc - the xloc,yloc of the text to be  *
 *                              drawn.                                  *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayText2 ( Window win, GC gc, char* text, int xloc, int yloc )
{
 
        GC gctext;
        XCharStruct overall;
        int n,dum,error;
        short label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;
 
        if(lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayText.\n");
            printf("Please call VCreateColormap before ");
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayText",
                "Window ID was NULL.",win);
 
        if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gctext);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gctext=gc;
        }
 
        if(text==NULL) strcpy(text,"");
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gctext),
                        text, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }
 
        label_x = (short)xloc;
        label_y = (short)yloc;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(lib_wins[0].font_width > 13)
           tlabel.font = None;
        else
           tlabel.font = lib_wins[0].font->fid;
 
 
        XSetForeground(lib_display,gctext,lib_reserved_colors[11].pixel);
        XDrawText(lib_display,win,gctext,label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
        if(lib_internal_gc==1) free(gctext);
        return(0);
}

 
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayText3                                    *
 *      Description     : This function creates a text in the location  *
 *                        (xloc,yloc) specified.                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         283 - could not draw text in the area        *
 *                              specified.                              *
 *      Parameters      :  win - the ID of the window to draw the text. *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         text - the text to be drawn.                 *
 *                         xloc,yloc - the xloc,yloc of the text to be  *
 *                              drawn.                                  *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayText3 ( Window win, GC gc, char* text, int xloc, int yloc )
{
 
        GC gctext;
        XCharStruct overall;
        int n,dum,error;
        short label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;
 
        if(lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayText.\n");
            printf("Please call VCreateColormap before ");
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayText",
                "Window ID was NULL.",win);
 
        if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gctext);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gctext=gc;
        }
 
        if(text==NULL) strcpy(text,"");
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gctext),
                        text, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }
 
        label_x = (short)xloc;
        label_y = (short)yloc;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(lib_wins[0].font_width > 13)
           tlabel.font = None;
        else
           tlabel.font = lib_wins[0].font->fid;
 
 
        XSetForeground(lib_display,gctext,lib_reserved_colors[4].pixel);
        XDrawText(lib_display,win,gctext,label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
        if(lib_internal_gc==1) free(gctext);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayFixedColorText                              *
 *      Description     : This function creates a text in the location  *
 *                        (xloc,yloc) specified. It then paints the text*
 *			  in a light color.				*
 *      Return Value    :  0 - work successfully.                       *
 *                         283 - could not draw text in the area        *
 *                              specified.                              *
 *      Parameters      :  win - the ID of the window to draw the text. *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *			   text - the text to be drawn.			*
 *                         xloc,yloc - the xloc,yloc of the text to be  *
 *                              drawn.                                  *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayFixedColorText ( Window win, GC gc, char* text,
    int xloc, int yloc )
{
 
	GC gctext;
        XCharStruct overall;
        int n,dum,error;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayFixedColorText.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayFixedColorText",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gctext);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gctext=gc;
        }
 
        if(text==NULL) strcpy(text,"");
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gctext),
                        text, n, &dum, &dum, &dum, &overall);
                label_width = overall.width;
                label_height = overall.ascent + overall.descent;
                label_ascent = overall.ascent;
        }
        else
        {
                label_width = 0;
                label_height = 0;
                label_ascent = 0;
        }
 
        label_x = (short)xloc;
        label_y = (short)yloc;
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(lib_wins[0].font_width > 13)
           tlabel.font = None;
        else
           tlabel.font = lib_wins[0].font->fid;
 
 
        XSetForeground(lib_display,gctext,lib_reserved_colors[5].pixel);
        XDrawText(lib_display,win,gctext,label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
	if(lib_internal_gc==1) free(gctext);
        return(0);
}
 

/************************************************************************
 *                                                                      *
 *      Function        : VBlinkBox                             *
 *      Description     : This function creates a box/button, highlights*
 *			  the box/button using the Xor function, sleeps	*
 *			  for some time and then unhighlights the box/	*
 *			  button.					*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  win - the ID of the window to draw the box	*
 *				or button.				*
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         xloc,yloc - the xloc,yloc of the box/button	*
 *				to be drawn.				*
 *			   width,height - the width,height of the box/	*
 *			 	button to be drawn.			*
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayXORBox.                          *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VBlinkBox ( Window win, GC gc, int xloc, int yloc, int width, int height )
{

	GC gcbox;
	int error;

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function VBlinkBox.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VBlinkBox",
                "Window ID was NULL.",win);
 
	if(gc==NULL){
                lib_internal_gc=1;
                error=VGetWindowGC(win,&gcbox);
                if(error!=0) return(error);
        }
        else {
                lib_internal_gc=0;
                gcbox=gc;
        }

	error=VDisplayXORBox(win,gcbox,xloc,yloc,width,height);
	if(error!=0) return(error);
	error=VSleep(100000);
	if(error!=0) return(error);
	error=VDisplayXORBox(win,gcbox,xloc,yloc,width,height);
	if(error!=0) return(error);
	
	if(lib_internal_gc==1) free(gcbox);
	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayXORBox                           *
 *      Description     : This function creates a box/button, highlights*
 *                        the box/button by Xoring the foreground and	*
 *			  background color of the GC.			*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  win - the ID of the window to draw the box   *
 *                              or button.                              *
 *                         gc - the GC to be used to draw the same;     *
 *                              If no GC is passed it is obtained from  *
 *                              the window ID.                          *
 *                         xloc,yloc - the xloc,yloc of the box/button  *
 *                              to be drawn.                            *
 *                         width,height - the width,height of the box/  *
 *                              button to be drawn.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VBlinkBox.                            *
 *      History         : Written on June 1, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayXORBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height )
{
	GC gcbox;
	unsigned long bg,fg;
        XGCValues vals;
	int junk,error;	

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function ");
	    printf("VDisplayXORBox.\n");
	    printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
 
        if (win == NULL)
            v_print_fatal_error("VDisplayXORBox",
                "Window ID was NULL.",win);
 
        if(gc==NULL){
		lib_internal_gc=1;
                error=VGetWindowGC(win,&gcbox);
		if(error!=0) return(error);
	}
        else {
		lib_internal_gc=0;
		gcbox=gc;
	}

	error=VGetWindowInformation(win,&junk,&junk,&junk,&junk,
                        &junk,&junk,&bg,&fg);
	if(error!=0) return(error);
	
	XSetFunction(lib_display,gcbox,GXinvert);
        vals.plane_mask= bg^fg;
        vals.fill_style=FillStippled;
        XChangeGC(lib_display,gcbox,GCPlaneMask|GCFillStyle,&vals);
        XFillRectangle(lib_display,win,gcbox,xloc,yloc,width,height);
        XSetFunction(lib_display,gcbox,GXcopy);
        vals.plane_mask= 0xFFFFFFFF;
        vals.fill_style= FillSolid;
        XChangeGC(lib_display,gcbox,GCPlaneMask|GCFillStyle,&vals);
 
        XFlush(lib_display);

	if(lib_internal_gc==1) free(gcbox);

        return(0);
}

/************************************************************************/
