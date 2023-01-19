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
 *      Filename  : cursors.c                                      	*
 *      Ext Funcs : VSleep, VSelectCursor.				*
 *      Int Funcs : v_dewey_cursor.					*
 *                                                                      *
 ************************************************************************/
 
#include "Vlibrary.h"
#include <sys/types.h>
#include <unistd.h>
#include "3dv.h"

/****
#ifndef POLL_NOT_FOUND
#include <sys/poll.h>
#endif
****/

static int v_check_button_occurance ( void );
static int v_dewey_cursor ( Display* display, Window window );

/************************************************************************
 *                                                                      *
 *      Function        : VSleep                        		*
 *      Description     : This function will sleep in microseconds.	*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  microsec - time to sleep in microsec.	*
 *      Side effects    : None.						*
 *      Entry condition : None.						*
 *      Related funcs   : None.						*
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/

int VSleep ( unsigned int microsec )
{
/*****
#ifndef POLL_NOT_FOUND
    static total = 0;       
    int millisec;          

    struct pollfd poll_3dviewnix;

    total += microsec;
    if (total < 1000) return(0);
    millisec = total/1000;
    total = total%1000;
    return poll(&poll_3dviewnix,(unsigned long)0,millisec);
#else
    return 0;
#endif
*****/
	
    return(0);

}

/************************************************************************
 *                                                                      *
 *      Function        : VSelectCursor					*
 *      Description     : This function selects a cursor and displays	*
 *			  it in the window requested. Besides the 	*
 *			  standard X Cursors, the user can request 	*
 *			  DEWEY_CURSOR (160) and DEFAULT_CURSOR 	*
 *			  (XC_top_left_arrow).				*
 *			  The window to be displayed can be :		*
 *				IMAGE_WINDOW (0),			*
 *				BUTTON_WINDOW (1),			*
 *				DIALOG_WINDOW (2),			*
 *				ALL_WINDOWS (3).			*
 *      Return Value    :  0 - work successfully.                       *
 *                         261 - invalid window type.			*
 *			   262 - invalid cursor type.			*
 *      Parameters      :  window_type - it can be IMAGE_WINDOW (0);	*
 *			 		 BUTTON_WINDOW (1); 		*
 *					 DIALOG_WINDOW (2);		*
 *					 ALL_WINDOWS (3).		*
 *			   cursor_type - it can be any of the standard	*
 *					 X cursors (0 to 152) or 	*
 *					 DEWEY_CURSOR (160).		*
 *      Side effects    : The cursor for the selected window is 	*
 *			  changed until it is reset by calling this	*
 *			  function again.				*
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.						*
 *      History         : Written on July 27, 1992 by Krishna Iyer.  	*
 *                                                                      *
 ************************************************************************/
int VSelectCursor ( int window_type, int cursor_type )
{
	Cursor curs[5];

	if(lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VSelectCursor.\n") ;
	    printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

	if(window_type != IMAGE_WINDOW && window_type != DIALOG_WINDOW && 
	   window_type != BUTTON_WINDOW && window_type != ALL_WINDOWS)
		return(261);

	else if((cursor_type < 0) || (cursor_type > 152 && cursor_type !=160))
		return(262);

	if(window_type == IMAGE_WINDOW){
		if(cursor_type>=0 && cursor_type <= 152){
			lib_image_cursor=cursor_type;
			curs[0]=XCreateFontCursor(lib_display, 
				lib_image_cursor);
			XDefineCursor(lib_display,lib_wins[0].win,
				curs[0]); 
		}	
		if(cursor_type==160){
                        curs[0]=v_dewey_cursor(lib_display, 
				lib_wins[0].win);
                        XDefineCursor(lib_display,lib_wins[0].win,
                                curs[0]); 
		}
	}

	else if (window_type == DIALOG_WINDOW){
                if(cursor_type>=0 && cursor_type <= 152){
                        lib_dialog_cursor=cursor_type;
                        curs[1]=XCreateFontCursor(lib_display,
                                lib_dialog_cursor);
                        XDefineCursor(lib_display,lib_wins[1].win,
                                curs[1]); 
                }
                if(cursor_type==160){
                        curs[1]=v_dewey_cursor(lib_display, 
                                lib_wins[1].win);        
                        XDefineCursor(lib_display,lib_wins[1].win, 
                                curs[1]); 
                }
        }

	else if (window_type == BUTTON_WINDOW){
                if(cursor_type>=0 && cursor_type <= 152){
                        lib_button_cursor=cursor_type;
                        curs[2]=XCreateFontCursor(lib_display,
                                lib_button_cursor);
                        XDefineCursor(lib_display,lib_wins[2].win,
                                curs[2]); 
                }
                if(cursor_type==160){
                        curs[2]=v_dewey_cursor(lib_display, 
                                lib_wins[2].win);        
                        XDefineCursor(lib_display,lib_wins[2].win, 
                                curs[2]); 
                }
        }
		
	else if (window_type == ALL_WINDOWS){
                if(cursor_type>=0 && cursor_type <= 152){
                        lib_image_cursor=cursor_type;
                        lib_dialog_cursor=cursor_type;
                        lib_button_cursor=cursor_type;
			curs[0]=XCreateFontCursor(lib_display, 
                                lib_image_cursor); 
                        XDefineCursor(lib_display,lib_wins[0].win, 
                                curs[0]);
			curs[1]=XCreateFontCursor(lib_display, 
                                lib_dialog_cursor); 
                        XDefineCursor(lib_display,lib_wins[1].win, 
                                curs[1]);
                        curs[2]=XCreateFontCursor(lib_display,
                                lib_button_cursor);
                        XDefineCursor(lib_display,lib_wins[2].win,
                                curs[2]);
                }
                if(cursor_type==160){
                        curs[0]=v_dewey_cursor(lib_display,
                                lib_wins[0].win);
                        XDefineCursor(lib_display,lib_wins[0].win,
                                curs[0]);

                        curs[1]=v_dewey_cursor(lib_display,
                                lib_wins[1].win);
                        XDefineCursor(lib_display,lib_wins[1].win,
                                curs[1]);

                        curs[2]=v_dewey_cursor(lib_display,
                                lib_wins[2].win);
                        XDefineCursor(lib_display,lib_wins[2].win,
                                curs[2]);
                }
        }
	XFlush(lib_display);
	return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_dewey_cursor				*
 *      Description     : This function will create a pixmap cursor.	*
 *			  The unique feature of this cursor is that it 	*
 *			  is visible against both dark and light 	*
 *			  backgrounds. Since it was written by 		*
 *			  Dewey Odhner here at MIPG, it is called 	*
 *			  dewey_cursor and has a value 160 assigned to  *
 *			  it.						*
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  display - the display to be selected.	*
 *			   window - the window to be displayed on.	*
 *      Side effects    : It changes the cursor of the selected window	*
 *			  to dewey_cursor and it is reset.		*
 *      Entry condition : None.						*
 *      Related funcs   : VSelectCursor.				*
 *      History         : Written on May 15, 1992 by Dewey Odhner.  	*
 *                                                                      *
 ************************************************************************/
static int v_dewey_cursor ( Display* display, Window window )
{
        Pixmap source, mask;
        unsigned long fg, bg;
        int j;
        Cursor cursor;
        XColor foreground_color, background_color;
        ViewnixColor colors[2];
        GC source_gc, mask_gc;
        static XPoint points[]={
                { 7,  0}, { 7,  1}, { 7,  2}, { 7,  3}, { 7,  4},
                { 7, 10}, { 7, 11}, { 7, 12}, { 7, 13}, { 7, 14},
                { 0,  7}, { 1,  7}, { 2,  7}, { 3,  7}, { 4,  7},
                {10,  7}, {11,  7}, {12,  7}, {13,  7}, {14,  7},
                { 1,  1}, { 2,  2}, { 3,  3}, { 4,  4}, { 5,  5},
                { 9,  9}, {10, 10}, {11, 11}, {12, 12}, {13, 13},
                { 1, 13}, { 2, 12}, { 3, 11}, { 4, 10}, { 5,  9},
                { 9,  5}, {10,  4}, {11,  3}, {12,  2}, {13,  1}};

        VGetWindowInformation(window, &j, &j, &j, &j, &j, &j, 
				&bg, &fg);
        source = XCreatePixmap(display, window, 15, 15, 1);
        mask = XCreatePixmap(display, window, 15, 15, 1);
        source_gc = XCreateGC(display, source, 0, NULL);
        mask_gc = XCreateGC(display, mask, 0, NULL);
        XSetFunction(display, source_gc, GXclear);
        XSetFunction(display, mask_gc, GXclear);
        XFillRectangle(display, source, source_gc, 0, 0, 15, 15);
        XFillRectangle(display, mask, mask_gc, 0, 0, 15, 15);
        XSetFunction(display, source_gc, GXset);
        XSetFunction(display, mask_gc, GXset);
        XDrawPoints(display, source, source_gc, points, 20,
                CoordModeOrigin);
        XDrawPoints(display, mask, mask_gc, points, 40, CoordModeOrigin);
        colors[0].pixel = fg;
        colors[1].pixel = bg;
        VGetColormap(colors, 2); 
        foreground_color.pixel = fg;
        background_color.pixel = bg;
        foreground_color.red = colors[0].red;
        foreground_color.green = colors[0].green;
        foreground_color.blue = colors[0].blue;
        background_color.red = colors[1].red;
        background_color.green = colors[1].green;
        background_color.blue = colors[1].blue;
        foreground_color.flags = background_color.flags = DoRed|DoGreen|DoBlue;
        cursor = XCreatePixmapCursor(display, source, mask,
                &foreground_color, &background_color, 7, 7);
        XDefineCursor(display, window, cursor);
	XFreePixmap(display, source);
        XFreePixmap(display, mask);
        return (cursor);
}

/**********************************************************************/
/************************************************************************
 *                                                                      *
 *      Function        : VCancelEvents                                 *
 *      Description     : This function will free all the events except *
 *                        Expose event in the image window, dialog      *
 *                        window, and button window to prevent more than*
 *                        one clients select on                         *
 *                        SubstructureRedirectMask, ResizeRedirectMask, *
 *                        or ButtonPressMask on the same window.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VSelectEvents.                                *
 *      History         : Written on June 7, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VCancelEvents ( void )
{
        XSetWindowAttributes attributes ;
        XWindowAttributes attribs ;
 
        if (lib_wins_created == 0) {
            printf("The error occurred in the function VCancelEvents.\n") ;
            printf("Please call VSetup before ");
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        attributes.event_mask=ExposureMask ;
        XChangeWindowAttributes(lib_display,lib_wins[0].win,
                                CWEventMask,&attributes) ;
        XChangeWindowAttributes(lib_display,lib_wins[5].win,
                                CWEventMask,&attributes) ;
        XChangeWindowAttributes(lib_display,lib_wins[6].win,
                                CWEventMask,&attributes) ;
        XChangeWindowAttributes(lib_display,lib_wins[1].win,
                                CWEventMask,&attributes) ;
        XChangeWindowAttributes(lib_display,lib_wins[2].win,
                                CWEventMask,&attributes) ;
        XGetWindowAttributes(lib_display,lib_wins[0].win,&attribs) ;
        lib_horizontal_menu_events_selected=0 ;
        lib_vertical_menu_events_selected=0 ;
 
        return(0);
}
 
/************************************************************************
 *                                                                      *
 *      Function        : VSelectEvents                                 *
 *      Description     : If the window id is not valid, then this      *
 *                        function will return 6. Otherwise this        *
 *                        function will select the event types to be    *
 *                        sent to a speicifed window and return 0.      *
 *                        get_event defines which input events the      *
 *                        window is interested in. The bits of the mask *
 *                        are defined in <X11/X.h> :                    *
 *                         ButtonPressMask         NoEventMask          *
 *                         ButtonReleaseMask       KeyPressMask         *
 *                         EnterWindowMask         KeyReleaseMask       *
 *                         LeaveWindowMask         ExposureMask         *
 *                         PointerMotionMask       VisibilityChangeMask *
 *                         PointerMotionHintMask   StructureNotifyMask  *
 *                         Button1MotionMask       ResizeRedirectMask   *
 *                         Button2MotionMask    SubstructureNotifyMask  *
 *                         Button3MotionMask    SubstructureRedirectMask*
 *                         Button4MotionMask       FocusChangeMask      *
 *                         Button5MotionMask       PropertyChangeMask   *
 *                         ButtonMotionMask        ColormapChangeMask   *
 *                         KeymapStateMask         OwnerGrabButtonMask  *
 *                      A call on get_event overrides any previous      *
 *                        call on get_event for the same window from the*
 *                        same client but not for other clients.        *
 *                        Multiple clients can select input on the same *
 *                        window; the event_mask passed are disjoint.   *
 *                        When an event is generated it will be         *
 *                        reported to all interested lients. However,   *
 *                        only one client at a time can select for each *
 *                        of SubstructureRedirectMask,                  *
 *                        ResizeRedirectMask, and ButtonPressMask. If a *
 *                        client attempts to select on none of three    *
 *                        event_masks and some other client has already *
 *                        selected it on the same window, the X server  *
 *                        generates a BadAccess error.                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *                         4 - file open error.                         *
 *      Parameters      :  win - Specifies the ID of the window         *
 *                              interested in the input events.         *
 *                         event_mask - Specifies the event mask. This  *
 *                              mask is the bitwise OR of one or more of*
 *                              the valid event mask bits.              *
 *      Side effects    : If the window id is invalid, return 6, else   *
 *                        return 0.None.                                *
 *                        lib_subwin_event_masks may be set.
 *      Entry condition : None.                                         *
 *      Related funcs   : VCancelEvents.                                *
 *      History         : Written on June 7, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                        Modified: 1/4/99 lib_subwin_event_masks set
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VSelectEvents ( Window win, unsigned long event_mask )
{
        XSetWindowAttributes attributes ;
        int i, j ;
 
        if (lib_wins_created == 0) {
            printf("The error occurred in the function VSelectEvents.\n") ;
            printf("Please call VSetup before ");
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        for (i=0; i<3; i++) if (win == lib_wins[i].win) break ;
        if (i == 3) {
            for (j=0; j<lib_num_subwins; j++)
                if (win == lib_subwins[j].win) break ;
            if (j == lib_num_subwins) return(6) ;
			lib_subwin_event_masks[j] = event_mask;
        }
        attributes.event_mask=event_mask;
        XChangeWindowAttributes(lib_display,win,CWEventMask,&attributes) ;
        return(0) ;
}
 
/************************************************************************
 *                                                                      *
 *      Function        : VCallProcess                                  *
 *      Description     : This function will make a system call and     *
 *                        allow the user to abort the process by        *
 *                        pressing the right mouse button.              *
 *      Return Value    :  279 - dialog window message area was used.   *
 *                         1 - memory allocation fault.                 *
 *                         400 - could not execute the process.         *
 *      Parameters      :  text - string containing the process to be   *
 *                                executed.                             *
 *                         mode - execution mode.                       *
 *                                0 - foreground.                       *
 *                                1 - background.                       *
 *                         hostname - host in which process should run, *
 *                                default (NULL) is taken to be the     *
 *                                "local" host.                         *
 *                         directory - directory used for the process,  *
 *                                default (NULL) is the current dir.    *
 *                         label - string identifying the process. It   *
 *                                 is used in messages shown to the     *
 *                                 user by this function.               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on January 10, 1993 by R.J.Goncalves. *
 *                        Modified: 6/16/99 command string dynamically
 *                           allocated by Dewey Odhner.
 *                        Modified: 6/30/00 line[499] initialized
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VCallProcess ( char* text, int mode, char* hostname, char* directory,
    char* label )
#if 0
char *text;     /* command string */
int mode;               /* execution mode: 0=foreground, 1=background */
char *hostname; /* host running the process (default="local") */
char *directory;        /* current directory (default = "") */
char *label;    /* process name */
#endif
{
        char temp[500]; /* temporary string */
        char *command;      /* command to be issued */
        char line[500]; /* line read from the pipe */
        int remote_flag=0;      /* indicates remote execution */
        int abort_flag; /* indicates process was aborted by user */
        char *flag;     /* indicates if shell was executed properly */
        int i,error;
        FILE *pipe;     /* pipe to process being executed */
 
 
        /* Check Remote execution */
        if(strlen(hostname) == 0  ||  strcmp(hostname, "local") == 0)
                remote_flag = 0;
        else
                remote_flag = 1;

	line[499] = 0;
	command = (char*)malloc(strlen(text)+strlen(hostname)+strlen(directory)+14);
	if (command == NULL)
		return(1);

        VDisplayButtonAction("","","ABORT");
 
        /* BACKGROUND MODE */
    if(mode == 1)
    {
        /* LOCAL HOST */
        if( remote_flag ==0)
        {
            if(strcmp(directory,"")==0)
            sprintf(command, "%s &",text);
            else
            sprintf(command, "cd %s;%s &",directory,text);
            i = system(command);
            if(i==0)
            {
              sprintf(temp, "%s Process is running in the background.",label);
              VDisplayDialogMessage(temp);
              free(command);
              return(279);
            }
            else
            {
              sprintf(temp, "Can't initiate %s Process !", label);
              XBell(lib_display,0);
              VDisplayDialogMessage(temp);
              free(command);
              return(400);
            }
        }
        /* REMOTE HOST */
        else
        {
            if(strcmp(directory,"")==0)
             sprintf(command,"rsh %s cd `pwd` ';' %s &",hostname,text);
            else
             sprintf(command,"rsh %s 'cd %s;%s' &", hostname, directory,text);
            i = system(command);
            if(i==0)
            {
              sprintf(temp, "%s Process is running on host [%s].",label,hostname);
              VDisplayDialogMessage(temp);
              free(command);
              return(279);
            }
            else
            {
              sprintf(temp, "Can't initiate %s process on Remote Host [%s] ! ", label, hostname);
              XBell(lib_display,0);
              VDisplayDialogMessage(temp);
              free(command);
              return(400);
            }
        }
    }
    /* FOREGROUND MODE */
    else if(mode == 0)
    {
        /* If REMOTE HOST */
        if(remote_flag == 1)
        {
           if(strcmp(directory,"")==0)
            sprintf(command,"rsh %s cd `pwd` ';' %s",hostname,text);
           else
            sprintf(command,"rsh %s 'cd %s;%s'", hostname, directory, text);
                pipe = popen(command, "r");
        }
        else
        {
                if(strcmp(directory,"")==0)
                sprintf(command,"%s",text);
                else
                sprintf(command,"cd %s;%s",directory,text);
                pipe = popen(command, "r");
        }
 
        abort_flag = 0;
        /*Read from Pipe while RIGHT button is not pressed and
          Pipe isn't closed */
        while( abort_flag != RIGHT_BUTTON  && (flag = fgets(line, 499, pipe))
!= NULL)
        {
            line[ strlen(line) -1 ] = 0; /*get rid of last char of the str*/
            VDisplayDialogMessage(line);
            XFlush(lib_display);
            abort_flag = v_check_button_occurance();
        }
 
        if(abort_flag == RIGHT_BUTTON)
        {
          sprintf(temp, "%s Process Aborted.", label);
          XBell(lib_display, 0);
          VDisplayDialogMessage(temp);
          XFlush(lib_display);
          error=pclose(pipe);
          free(command);
          if (error != 0) return(400);
          else return(401);
        }
        else
          if(pipe == NULL)
            {
              sprintf(temp, "Can't initiate %s Process !", label);
              XBell(lib_display, 0);
              VDisplayDialogMessage(temp);
              XFlush(lib_display);
              free(command);
              return(400);
            }
          else
            {
              free(command);
              error=pclose(pipe);
              if(error != 0) return(400);
              VDisplayDialogMessage("Done.");
              return(279);
            }
 
  }
 
  else /*unknown mode*/
  {
     free(command);
     return(400);  /*fatal error*/
  }
}
 
 
/************************************************************************
 *                                                                      *
 *      Function        : v_check_button_occurance                      *
 *      Description     : This function reads the entire event queue and*
 *                        returns :                                     *
 *                              0 - if no buttons were pressed.         *
 *                              RIGHT_BUTTON (3) - if right button was  *
 *                                  pressed.                            *
 *                              MIDDLE_BUTTON (2) - if middle button was*
 *                                  pressed.                            *
 *                              LEFT_BUTTON (1) - if left button was    *
 *                                  pressed.                            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCallProcess.                                 *
 *      History         : Written on January 10, 1993 by R.J.Goncalves. *
 *                                                                      *
 ************************************************************************/
static int v_check_button_occurance ( void )
{
    register int i, j;
    XEvent xevent;
    int found;
 
    /* check for the existance of any events on the queue */
    i = XEventsQueued(lib_display, QueuedAfterFlush);
    found = 0;
    /* check all events found on the queue */
    if( i > 0 )
    {
        for(j=0; j<i && found == 0; j++)
        {
           VNextEvent(&xevent);
 
           if(xevent.type == ButtonPress)
            found = 1;
        }
 
    }
 
 
    /* Return the button that was pressed */
    if(found == 1)
        return(xevent.xbutton.button);
    else
        return(0);
 
}
 
/*********************************************************************/
 
 
 

