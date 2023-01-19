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
 *                              SAMPLE                                  *
 *                                                                      *
 *      Function        : name_of_function                              *
 *      Description     : A brief statement of purpose of this function.*
 *      Return Value    : The meaning of the return value of the        *
 *                        function, if any.                             *
 *      Parameters      :                                               *
 *        first_param   : the meaning of the first argument,            *
 *                        and any constraints on it.                    *
 *        second_param  : ....                                          *
 *      Side effects    : Any global or static variables(*) changed by  *
 *                        a call to this function, and any output       *
 *                        performed.                                    *
 *      Entry condition : Any global or static variables required by    *
 *                        this function to be initialized, and any      *
 *                        files, server connections, or windws reqd.    *
 *                        for input; or an initialization function to   *
 *                        be called first which ensures the former.     *
 *      Related funcs   : Any related functions.                        *
 *      History         : Creation date and name of programmer;         *
 *                        description of each change, date of change,   *
 *                        and name of programmer.                       *
 *                                                                      *
 ************************************************************************/

/************************************************************************
 *                                                                      *
 *      Filename  : button_interf.c                                     *
 *      Ext Funcs : VDisplayRunModeCommand, VDeleteRunModeCommand,       *
 *                  VDisplaySaveScreenCommand, VDeleteSaveSreenCommand, *
 *                  VDisplaySaveScreenImage, v_OutputSaveScreenImage,    *
 *                  VDisplayOutputFilePrompt, VDeleteOutputFilePrompt,   *
 *                  VGetSaveSwitchValue, v_GetSaveScreenSwitchValue,         *
 *                  VGetSaveFilename, VGetRunModeValue,                 *
 *                  VGetSaveScreenFilename, VDisplayDialogOnOffCommand, *
 *                  VDeleteDialogOnOffCommand, v_DisplayStaticButton,     *
 *                  VCheckEventsInButtonWindow.                         *
 *      Int Funcs : v_SelectRunModeCommand, v_SelectSaveScreenCommand,  *
 *                  v_savescreen_callback_func,v_create_threedswitch,   *
 *                  v_SelectOutputFileCommand, v_highlight_threedswitch,*
 *                  v_SelectDialogOnOffCommand,                         *
 *                  v_DoAnnotation, v_DoHelp, v_DoNotes, v_DoInformation*
 *                  v_create_threed_static_button,                      *
 *                  v_SelectStaticButtonCommand,                        *
 *                  v_highlight_threedstaticbutton, v_SelectTextItem.   *
 *                                                                      *
 ************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include "Vlibrary.h"
#include "3dv.h"

TEXT *lib_output,*lib_output2;
BUTTON *lib_button,*lib_button2;
int MAX_NUM_OUTPUT_FILES;
char DEFAULT_NAME[MAX_DEFAULT_FILES][MAX_DEFAULT_CHAR];
typedef int (*FUNCTION_PTR)();
FUNCTION_PTR statbut_func[3];
int v_DoAnnotation(),v_DoHelp(),v_DoNotes(),v_DoInformation();
int v_savescreen_callback_func();
GC threedswitchgc,threeddialogonoffgc,threedsavescreengc,threedforebackgc;
GC threedannotationbuttongc,threedhelpbuttongc,threednotesbuttongc;
XGCValues vals;
int MAX_WIDTH_DIALOGONOFF_SWITCH,MAX_WIDTH_OUTPUTFILE_SWITCH;
int MAX_WIDTH_RUNMODE_SWITCH,MAX_WIDTH_SAVESCREEN_SWITCH;

static int v_create_threed_static_button ( GC gc, int xloc, int yloc,
    int xloc2, int yloc2, int offset );
static int v_create_threedswitch ( GC gc, int xloc, int yloc,
    int xloc2, int yloc2, int offset );
static int v_GetSaveScreenSwitchValue ( int* val );
static int v_highlight_threedstaticbutton ( GC gc, char* text,
    int xloc, int yloc, int xloc2, int yloc2, int offset );
static int v_highlight_threedswitch ( PanelCmdInfo* buttonwinswitch, GC gc,
    int max_switch_width, int xloc, int yloc, int xloc2, int yloc2,
    int offset );
static int v_OutputSaveScreenImage ( char* scrn_file, int xloc, int yloc,
    int width, int height, int opt );

int VGetSaveScreenFilename ( char* name );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayRunModeCommand                        *
 *      Description     : This function will display the MODE:FG/BG     *
 *                        switch in the button window and let the user  *
 *                        select the mode to run the process.           *
 *                        In order to change the value in the switch the*
 *                        user has to call the function                 *
 *                        VCheckEventsInButtonWindow and pass pointer   *
 *                        to the event structure. In order to delete    *
 *                        this switch call the function                 *
 *                        VDeleteRunModeCommand. Erasing implies         *
 *                        removing the related data structures from     *
 *                        memory.                                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDeleteRunModeCommand, VGetRunModeValue,       *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VDisplayRunModeCommand ( void )
{
        int xloc,yloc,xloc2,yloc2,offset,dum;
        int line_xloc,max_width;
        int switch_item_displayed;

        XCharStruct overall;
        char text[100];
        int n;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;
 
        if(lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplayRunModeCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
 
        /*threedforebackgc=XCreateGC(lib_display,lib_wins[2].win,0,&vals);*/
        threedforebackgc=lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threedforebackgc,GCLineWidth,&vals);
 
        xloc = LIB_FORE_BACK_BUTTON_XLOC;
        yloc = LIB_FORE_BACK_BUTTON_YLOC;
        xloc2 = xloc+LIB_STATICBUTTON_WIDTH;
        yloc2 = yloc+LIB_PANEL_ITEM_HEIGHT;
        offset = 1;
 
        v_create_threedswitch(threedforebackgc,xloc,yloc,xloc2,yloc2,offset);
 
        /*switch initialization*/
        lib_forebackswitch_num_of_cmds=1;
        lib_forebackswitch_cmds=(PanelCmdInfo *)malloc(1*
                                              sizeof(PanelCmdInfo));
        if (lib_forebackswitch_cmds == NULL) return(1);
        lib_forebackswitch_switches=(PanelSwitchInfo *)malloc(1*
                                        sizeof(PanelSwitchInfo));
        if (lib_forebackswitch_switches == NULL) return(1);
        strncpy(lib_forebackswitch_cmds[0].cmd,"RUN MODE",MAX_PANEL_CHARS);
        lib_forebackswitch_cmds[0].cmd[MAX_PANEL_CHARS]='\0';
        lib_forebackswitch_cmds[0].type=1;
        lib_forebackswitch_cmds[0].group=1;
        lib_forebackswitch_cmds[0].num_of_switches=2;
        lib_forebackswitch_cmds[0].switches=(Char30 *)malloc(sizeof(Char30)*2);
        if (lib_forebackswitch_cmds[0].switches == NULL) return(1);
 
        strcpy(lib_forebackswitch_cmds[0].switches[0],"FG");
        strcpy(lib_forebackswitch_cmds[0].switches[1],"BG");
        lib_forebackswitch_cmds[0].switch_item_displayed=0;
        lib_forebackswitch_switches[0].x=XTextWidth(lib_wins[2].font,
                        lib_forebackswitch_cmds[0].cmd,
                        strlen(lib_forebackswitch_cmds[0].cmd))+
                        lib_wins[2].font_width;
        max_width=XTextWidth(lib_wins[2].font,
                        lib_forebackswitch_cmds[0].switches[1],
                        strlen(lib_forebackswitch_cmds[0].switches[1]));
        lib_forebackswitch_switches[0].width=max_width;

        MAX_WIDTH_RUNMODE_SWITCH=max_width;
        /***************displaying the string*************/
        line_xloc=xloc2-(max_width + 4 + 4);
        XSetForeground(lib_display,lib_wins[2].gc,
                        lib_reserved_colors[0].pixel);
        XDrawLine(lib_display,lib_wins[2].win,lib_wins[2].gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT);
        XFlush(lib_display);
 
        /***displaying the cmd***/
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
        strcpy(text,lib_forebackswitch_cmds[0].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedforebackgc),
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
        label_y = (short)yloc + (but_height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedforebackgc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedforebackgc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);
 
        /***displaying the switch***/
        switch_item_displayed=lib_forebackswitch_cmds[0].switch_item_displayed;
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
        strcpy(text,lib_forebackswitch_cmds[0].switches[switch_item_displayed]);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedforebackgc),
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
 
        label_x = (short)line_xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedforebackgc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedforebackgc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /*******************************************************/
 
        lib_display_fg_bg_on=1 ;

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_SelectRunModeCommand                        *
 *      Description     : This function will check whether the          *
 *                        foreground or background processing is        *
 *                        chosen. If the user clicks the switch the     *
 *                        FG mode is changed to BG mode and the value   *
 *                        of the switch is updated. The user should     *
 *                        call VGetRunModeValue to get the value of the *
 *                        switch.                                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         206 - the cursor is not within the command.  *
 *                         207 - the "run mode" command switch is NULL. *
 *      Parameters      :  event -  event structure passed by the user. *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDeleteRunModeCommand, VGetRunModeValue,       *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_SelectRunModeCommand ( XEvent* event )
{
        int xloc,yloc,xloc2,yloc2;
        int curx,cury;
        int item;
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("v_SelectRunModeCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
 
        /*for the panel like button*/
        xloc=LIB_FORE_BACK_BUTTON_XLOC ;
        yloc=LIB_FORE_BACK_BUTTON_YLOC ;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;

        if(event->type == ButtonPress)
        {
          curx=event->xbutton.x;cury=event->xbutton.y;
          if(event->xbutton.button == 1)
          {
             /*checking button press in the panel like switch*/
             if(curx>xloc && curx<xloc2 && cury>yloc && cury<yloc2)
             {
              if(lib_forebackswitch_cmds != NULL && lib_forebackswitch_switches != NULL)
              {
                item=lib_forebackswitch_item_selected;
                if (lib_forebackswitch_cmds[item].type != 1) return(206);
                if (item < lib_forebackswitch_num_of_cmds && item >= 0)
                {
                  if (event->xbutton.button == 1) {
                    lib_forebackswitch_cmds[item].switch_item_displayed++;
                    if (lib_forebackswitch_cmds[item].switch_item_displayed >=
                        lib_forebackswitch_cmds[item].num_of_switches)
                        lib_forebackswitch_cmds[item].switch_item_displayed=0;
                  }
                  else {
                    lib_forebackswitch_cmds[item].switch_item_displayed--;
                    if(lib_forebackswitch_cmds[item].switch_item_displayed<0)
                        lib_forebackswitch_cmds[item].switch_item_displayed=
                            lib_forebackswitch_cmds[item].num_of_switches-1;
                  }
                }
                v_highlight_threedswitch(lib_forebackswitch_cmds,
                                       threedforebackgc,
                                       MAX_WIDTH_RUNMODE_SWITCH,
                                       xloc,yloc,xloc2,yloc2,1);
              }
              else return(207); /*run mode command switch is null*/
             }
             else return(206); /*the cursor is not within the command*/
          }
        return(0);
        }

        else return(204); /*invalid event type*/
}

/************************************************************************
 *                                                                      *
 *      Function        : VDeleteRunModeCommand                          *
 *      Description     : This function will erase the FG/BG command    *
 *                        switch from the button window and delete all  *
 *                        the associated data structures.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         207 - the "run mode" command switch is NULL. *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayRunModeCommand, VGetRunModeValue,     *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VDeleteRunModeCommand ( void )
{

        int xloc,yloc,xloc2,yloc2;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDeleteRunModeCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        xloc=LIB_FORE_BACK_BUTTON_XLOC-4;
        yloc=LIB_FORE_BACK_BUTTON_YLOC;
        xloc2=LIB_FORE_BACK_BUTTON_XLOC+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;

        if (!lib_display_fg_bg_on) return(207) ;
        lib_display_fg_bg_on=0 ;
        lib_fg_bg_selected=1 ;

        XClearArea(lib_display,lib_wins[2].win,
                   xloc,yloc,xloc2-xloc,yloc2-yloc,False);

        if(lib_forebackswitch_cmds != NULL) {
                free(lib_forebackswitch_cmds);
				lib_forebackswitch_cmds = NULL;
                return(0) ;
        }

        else return(207); /*the FG/BG switch is NULL*/
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetRunModeValue                              *
 *      Description     : This function will return the value of the    *
 *                        "MODE:FG/BG" switch.                          *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory aloocation error.                 *
 *      Parameters      :  val -  value of the switch returned.         *
 *                         0 - FG; 1 - BG;                              *
 *      Side effects    : None.                                         *
 *      Entry condition : VDisplayRunModeCommand should be called       *
 *                        earlier.                                      *
 *      Related funcs   : VDeleteRunModeCommand, VDisplayRunModeCommand, *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VGetRunModeValue ( int* val )
{
 
        if(lib_forebackswitch_cmds == NULL) return(1);
 
        *val = (int)lib_forebackswitch_cmds[0].switch_item_displayed;
 
                /*0 - FG */
                /*1 - BG */
 
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplaySaveScreenCommand                     *
 *      Description     : This function will display the                *
 *                        SCRN:APPND/OVERWR switch with APPND as the    *
 *                        default value. In order to change the option  *
 *                        the user has to call the function             *
 *                        VCheckEventsInButtonWindow.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDeleteSaveSreenCommand,                      *
 *                        v_GetSaveScreenSwitchValue,                    *
 *                        VGetSaveScreenFilename,                       *
 *                        VCheckEventsInButtonWindow,                   *
 *                        VDisplaySaveScreenImage,                      *
 *                        v_OutputSaveScreenImage.                       *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 12/22/94 private button list used
 *                        by Dewey Odhner.
 *                        Modified 3/5/97 lib_savescreenswitch_cmds,
 *                        lib_savescreenswitch_switches checked for
 *                        previous allocation by Dewey Odhner.
 *                        Modified 9/23/98 lib_output2,
 *                        lib_button2 checked for
 *                        previous allocation by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplaySaveScreenCommand ( void )
{

        int xloc,yloc,xloc2,yloc2,offset,dum;
        int line_xloc,max_width;
        int switch_item_displayed;
 
        XCharStruct overall;
        char text[100];
        int error,n;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

        if(lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplaySaveScreenCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        /*************initializing the switch***************/ 


        threedsavescreengc=lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threedsavescreengc,GCLineWidth,&vals);
        xloc=LIB_SAVE_SCREEN_SWITCH_XLOC;
        yloc=LIB_SAVE_SCREEN_SWITCH_YLOC;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;
        offset=1;
 
        v_create_threedswitch(threedsavescreengc,xloc,yloc,xloc2,yloc2,offset);
 
        /*switch initialization*/
        lib_savescreenswitch_num_of_cmds=1;
		if (lib_savescreenswitch_cmds == NULL)
        	lib_savescreenswitch_cmds=(PanelCmdInfo *)calloc(1,
                                              sizeof(PanelCmdInfo));
        if (lib_savescreenswitch_cmds == NULL) return(1);
        if (lib_savescreenswitch_switches == NULL)
			lib_savescreenswitch_switches=(PanelSwitchInfo *)malloc(1*
                                        sizeof(PanelSwitchInfo));
        if (lib_savescreenswitch_switches == NULL) return(1);
        strcpy(lib_savescreenswitch_cmds[0].cmd,"SVSCRN");
        lib_savescreenswitch_cmds[0].cmd[MAX_PANEL_CHARS]='\0';
        lib_savescreenswitch_cmds[0].type=1;
        lib_savescreenswitch_cmds[0].group=1;
        lib_savescreenswitch_cmds[0].num_of_switches=2;
        if (lib_savescreenswitch_cmds[0].switches == NULL)
			lib_savescreenswitch_cmds[0].switches=(Char30 *)malloc(
                                                        sizeof(Char30)*2);
        if (lib_savescreenswitch_cmds[0].switches == NULL) return(1);
        strcpy(lib_savescreenswitch_cmds[0].switches[0],"APND");
        strcpy(lib_savescreenswitch_cmds[0].switches[1],"OVWR");
        max_width=XTextWidth(lib_wins[2].font,
                        lib_savescreenswitch_cmds[0].switches[1],
                        strlen(lib_savescreenswitch_cmds[0].switches[1]));
        
        lib_savescreenswitch_cmds[0].switch_item_displayed=0;
        lib_savescreenswitch_switches[0].x=XTextWidth(lib_wins[2].font,
                        lib_savescreenswitch_cmds[0].cmd,
                        strlen(lib_savescreenswitch_cmds[0].cmd))+
                        lib_wins[2].font_width;
        lib_savescreenswitch_switches[0].width=max_width;
 
        MAX_WIDTH_SAVESCREEN_SWITCH=max_width;
        /***************displaying the string*************/
        line_xloc=xloc2-(max_width + 4 + 4);
        XSetForeground(lib_display,lib_wins[2].gc,
                        lib_reserved_colors[0].pixel);
        XDrawLine(lib_display,lib_wins[2].win,lib_wins[2].gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT);

 
        /***displaying the cmd***/
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
        strcpy(text,lib_savescreenswitch_cmds[0].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedsavescreengc),
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
        label_y = (short)yloc + (but_height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedsavescreengc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedsavescreengc,
                  label_x,label_y,&tlabel,1);

        /***displaying the switch***/
        switch_item_displayed=lib_savescreenswitch_cmds[0].switch_item_displayed;
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
        strcpy(text,lib_savescreenswitch_cmds[0].switches[switch_item_displayed]);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedsavescreengc),
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
 
        label_x = (short)line_xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedsavescreengc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedsavescreengc,
                  label_x,label_y,&tlabel,1);

      /*************************************************************/ 

        if (lib_display_save_screen_on)
            return (0);

        /*initializing the text item*/
        error = v_AddText0(lib_wins[2].win,
                 LIB_SAVE_SCREEN_TEXT_XLOC,
                 LIB_SAVE_SCREEN_TEXT_YLOC,
                 -LIB_STATICBUTTON_WIDTH,
                 -LIB_PANEL_ITEM_HEIGHT,
                 "",
                 "svscrn-tmp",
                 NULL,
                 &lib_output2, 1);

          if(error != 0) return(error); 
        /*initializing the button item*/
        if(v_savescreen_callback_func != NULL)
        {
        error = v_AddButton0(lib_wins[2].win,
                   LIB_SAVE_SCREEN_BUTTON_XLOC,
                   LIB_SAVE_SCREEN_BUTTON_YLOC,
                   -LIB_STATICBUTTON_WIDTH,
                   0,
                   "SAVE SCRN",
                   v_savescreen_callback_func,
                   0,
                   &lib_button2, 1);
          if(error != 0) return(error);
        }

        lib_display_save_screen_on=1;

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_SelectSaveScreenCommand                     *
 *      Description     : This function will check whether the cursor   *
 *                        is within the SCRN:APPND/OVRWR switch and the *
 *                        left button is pressed. If so, the function   *
 *                        will output the contents of the image window  *
 *                        to the file specified by the user (default    *
 *                        name is temp.SCRN).                           *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         206 - the cursor is not within the command.  *
 *                         208 - the savescreen command switch is NULL. *
 *      Parameters      :  event - the event passed by the user.        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        The user has to call VDisplaySaveScreenCommand*
 *                        and VSelectEvents before calling this         *
 *                        function.                                     *
 *      Related funcs   : VDeleteSaveSreenCommand,                      *
 *                        VDisplaySaveScreenCommand,                    *
 *                        v_GetSaveScreenSwitchValue,                    *
 *                        VGetSaveScreenFilename,                       *
 *                        VCheckEventsInButtonWindow,                   *
 *                        VDisplaySaveScreenImage,                      *
 *                        v_OutputSaveScreenImage.                       *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_SelectSaveScreenCommand ( XEvent* event )
{
        int xloc,yloc,xloc2,yloc2,xloc3,yloc3,xloc4,yloc4;
        int xloc5,yloc5,xloc6,yloc6;
        int curx,cury;
        int error,item;
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ");
            printf("v_SelectSaveScreenCommand.\n");
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT);
        }

        /*for the panel like button*/
        xloc=LIB_SAVE_SCREEN_SWITCH_XLOC;
        yloc=LIB_SAVE_SCREEN_SWITCH_YLOC;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;

        /*for the button item*/
        xloc3=LIB_SAVE_SCREEN_BUTTON_XLOC;
        yloc3=LIB_SAVE_SCREEN_BUTTON_YLOC;
        if(lib_button2 != NULL)
        {
           xloc4=xloc3+lib_button2->width;
           yloc4=yloc3+lib_button2->height;
        }
        else
        {
           xloc4=0;
           yloc4=0;
        }

        /*for the text item*/
        xloc5=LIB_SAVE_SCREEN_TEXT_XLOC;
        yloc5=LIB_SAVE_SCREEN_TEXT_YLOC;
        xloc6=xloc5+lib_output2->width;
        yloc6=yloc5+lib_wins[2].font_height;

        if(event->type == ButtonPress)
        {
          curx=event->xbutton.x;cury=event->xbutton.y;
          if(event->xbutton.button == 1 || event->xbutton.button == 2)
          {
             /*checking button press in the panel like switch*/
             if(event->xbutton.x>xloc && event->xbutton.x<xloc2 &&
                event->xbutton.y>yloc && event->xbutton.y<yloc2)
             {
              if(lib_savescreenswitch_cmds != NULL && lib_savescreenswitch_switches != NULL)
              {
                item=lib_savescreenswitch_item_selected;
                if (lib_savescreenswitch_cmds[item].type != 1) return(206);
                if (item < lib_savescreenswitch_num_of_cmds && item >= 0)
                {
                  if (event->xbutton.button == 1) {
                    lib_savescreenswitch_cmds[item].switch_item_displayed++;
                    if (lib_savescreenswitch_cmds[item].switch_item_displayed >=
                        lib_savescreenswitch_cmds[item].num_of_switches)
                        lib_savescreenswitch_cmds[item].switch_item_displayed=0;
                  }
                  else {
                    lib_savescreenswitch_cmds[item].switch_item_displayed--;
                    if (lib_savescreenswitch_cmds[item].switch_item_displayed < 0)
                        lib_savescreenswitch_cmds[item].switch_item_displayed=
                            lib_savescreenswitch_cmds[item].num_of_switches-1;
                  }
                }
                v_highlight_threedswitch(lib_savescreenswitch_cmds,
                                       threedsavescreengc,
                                       MAX_WIDTH_SAVESCREEN_SWITCH,
                                       xloc,yloc,xloc2,yloc2,1);
 
              }
              else return(207); /*savescreen filename command switch is null*/
             }
         
             /*checking button press in the button item*/
             else if(event->xbutton.x>xloc3 && event->xbutton.x<xloc4 &&
                   event->xbutton.y>yloc && event->xbutton.y<yloc2)
             {
                if(lib_button2 != NULL) {
                  error = VCheckButtonItemEvent(event);
                  if(error != 0) return(error);
                }
             }
 
             else return(206); /*the cursor is not within the command*/
          }
          return(279);
        }

        else return(204);  /*invalid event type*/

}

/************************************************************************
 *                                                                      *
 *      Function        : v_savescreen_callback_func                    *
 *      Description     : This function is the callback function used   *
 *                        by VDisplaySaveScreenFilename.                *
 *      Return Value    :  0 - work successfully.                       *
 *                         279 - dialog window message area was used.   *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplaySaveScreenCommand.                    *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 1/26/95 file created in overwrite
 *                        mode by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_savescreen_callback_func ( void )
{
        char name[20],msg[100];
        int val,result,error;
        FILE *fp;
 
        error=VGetSaveScreenFilename(name);
          if(error != 0) return(error);
        error=v_GetSaveScreenSwitchValue(&val);
          if(error != 0) return(error);

        if(val == 0) lib_save_screen_append_selected=1; /*append*/
        else lib_save_screen_append_selected=0;         /*overwrite*/

        sprintf(lib_scrn_file,"%s.SCRN",name);

        /*****overwriting a file*********/
        if(lib_save_screen_append_selected==0)
        {
            fp=fopen(lib_scrn_file,"rb");
            if(fp==NULL)
            {
                 sprintf(msg,
                        "%s.SCRN does not exist. Using APPEND mode to create the file.... \n",name);
            }
            else 
            {
                sprintf(msg,
                        "Overwiting %s.SCRN in the OVERWRITE mode.\n",name);
                fclose(fp);   
            }
            VDisplayDialogMessage(msg);
        }


        result=v_OutputSaveScreenImage(lib_scrn_file,0,0,
                        lib_wins[0].width,lib_wins[0].height,
                        lib_save_screen_append_selected);

        if(result == 0) 
        {
            sprintf(msg,"%s.SCRN was created successfully.\n",name);
            error=VDisplayDialogMessage(msg);
              if(error != 0) return(error);
              else return(279);
        }

        return(result) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VDeleteSaveSreenCommand                       *
 *      Description     : This function will erase the                  *
 *                        SCRN:APPND/OVRWR switch and delete all        *
 *                        the associated data structures.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         208 - the savescreen command switch is NULL. *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplaySaveScreenCommand,                    *
 *                        v_GetSaveScreenSwitchValue,                    *
 *                        VGetSaveScreenFilename,                       *
 *                        VDisplaySaveScreenImage,                      *
 *                        v_OutputSaveScreenImage,                       *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 9/1/94 lib_savescreenswitch_cmds[
 *                        0].switches.switches freed by Dewey Odhner.
 *                        Modified 12/22/94 private button list used
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDeleteSaveSreenCommand ( void )
{
        int xloc,yloc,xloc2,yloc2;
        int error,width,height;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDeleteSaveSreenCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        if(!lib_display_save_screen_on) return(208);

        xloc=LIB_SAVE_SCREEN_SWITCH_XLOC-4;
        yloc=LIB_SAVE_SCREEN_SWITCH_YLOC;
        if(lib_button2 != NULL)
        {
          width = lib_button2->width;
          height = lib_button2->height;
        }
        else
        {
          width = 0;
          height = 0;
        }
        xloc2=LIB_SAVE_SCREEN_BUTTON_XLOC+width;
        yloc2=LIB_SAVE_SCREEN_BUTTON_YLOC+height;

        XClearArea(lib_display,lib_wins[2].win,
                   xloc,yloc,xloc2-xloc,yloc2-yloc,False);

        if(lib_savescreenswitch_cmds != NULL) {
                        free(lib_savescreenswitch_cmds[0].switches);
                        free(lib_savescreenswitch_cmds);
						lib_savescreenswitch_cmds = NULL;
        }
        else return(208);
        if(lib_savescreenswitch_switches !=NULL) 
        {
		                free(lib_savescreenswitch_switches);
						lib_savescreenswitch_switches = NULL;
        }
		else return(208);

        error=v_DeleteText0(lib_output2, 1);
          if(error != 0) return(error);

        if(lib_button2 != NULL)
            error=v_DeleteButton0(lib_button2, 1);
              lib_button2=NULL;
              if(error != 0) return(error);


        lib_display_save_screen_on=0 ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplaySaveScreenImage                       *
 *      Description     : This function will read the specified frame of*
 *                        save screen image file, display it in the     *
 *                        image window at the location you specify,     *
 *                        and update the colormap with    *
 *                        the values associated with the frame to be    *
 *                        displayed. The save screen file contains      *
 *                        the number of frames in the first 4 bytes,    *
 *                        followed by the image frames. Each frame      *
 *                        contains the image, the number of the         *
 *                        colormap entries, and the colormap contents.  *
 *                        One can use this function to read file and    *
 *                        display images saved by the function          *
 *                        v_OutputSaveScreenImage.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         4 - file open error.                         *
 *                         209 - invalid frame number in the save       *
 *                               screen file.                           *
 *                         240 - frame outside the image window         *
 *                               boundary.                              *
 *      Parameters      :  scrn_file - Specifies the name of file to    *
 *                                     be displayed.                    *
 *                         frame - Specifies the frame number to be     *
 *                                 displayed.                           *
 *                         xloc, yloc - Specifies x and y coordinates   *
 *                                      of the upper left corner of the *
 *                                      rectangle, relative to the      *
 *                                      origin of the image window.     *
 *      Side effects    : lib_colormap_colors set
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        If the value xloc, or yloc is not valid,      *
 *                        or scrn_file is NULL, a core dump is produced.*
 *      Related funcs   : VDeleteSaveSreenCommand,                      *
 *                        v_GetSaveScreenSwitchValue,                    *
 *                        VGetSaveScreenFilename,                       *
 *                        VCheckEventsInButtonWindow,                   *
 *                        VDisplaySaveScreenCommand,                    *
 *                        v_OutputSaveScreenImage.                       *
 *      History         : Written on November 7, 1990 by Hsiu-Mei Hung. *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                        Modified 10/18/94 lib_colormap_colors set
 *                        by Dewey Odhner
 *                        Modified 12/13/94 VWriteColorcomFile call removed
 *                        by Dewey Odhner
 *                        Modified: 3/5/03 v_ReadData used to read
 *                           image data by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplaySaveScreenImage ( char* scrn_file, int frame, int xloc, int yloc )
{
        FILE *fp ;
        static XImage ximage ;
        int totbytes, error, total_frame, i ;
        short entries ;
        char msg1[80] ;
        unsigned long pixel;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplaySaveScreenImage.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (scrn_file == NULL) 
            v_print_fatal_error("VDisplaySaveScreenImage",
                "The pointer of scrn_file should not be NULL.",0) ;
        if (frame <= 0 ) 
            v_print_fatal_error("VDisplaySaveScreenImage",
                "The value of frame should be greater than 0.",0) ;
        if (xloc < 0 || xloc >= lib_wins[0].width) {
            sprintf(msg1,"The range of xloc is >= 0 && < %d.",
                    lib_wins[0].width);
            v_print_fatal_error("VDisplaySaveScreenImage",msg1,xloc) ;
        }
        if (yloc < 0 || yloc >= lib_wins[0].height) {
            sprintf(msg1,"The range of yloc is >= 0 && < %d.",
                    lib_wins[0].height);
            v_print_fatal_error("VDisplaySaveScreenImage",msg1,yloc) ;
        }

        fp=fopen(scrn_file,"rb") ;
        if (fp == NULL) return(4) ;
        error=v_ReadData( (char*)&total_frame,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        if (frame < 1 || frame > total_frame) return(209) ;
        for (i=0; i<frame-1; i++) {
          /* Added by supun - to make it machine independent */
          error=v_ReadData( (char*)&ximage.width,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.height,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.xoffset,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.format,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.byte_order,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.bitmap_unit,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.bitmap_bit_order,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.bitmap_pad,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.depth,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.bytes_per_line,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.bits_per_pixel,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.red_mask,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.green_mask,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;
          error=v_ReadData( (char*)&ximage.blue_mask,sizeof(int),1,fp) ;
          if (error == 0) return(2) ;

            
          if (error == 0) return(2) ;
          totbytes=ximage.bytes_per_line*ximage.height;

          error=fseek(fp,totbytes,1L) ;
          if (error == -1) return(5) ;

          error=v_ReadData( (char*)&entries,sizeof(short),1,fp) ;
          if (error == 0) return(2) ;
          error=fseek(fp,11*entries,1L) ;
          if (error == -1) return(5) ;
        }
        /* Added by supun - to make it machine independent */
        error=v_ReadData( (char*)&ximage.width,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.height,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.xoffset,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.format,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.byte_order,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.bitmap_unit,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.bitmap_bit_order,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.bitmap_pad,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.depth,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.bytes_per_line,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.bits_per_pixel,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.red_mask,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.green_mask,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        error=v_ReadData( (char*)&ximage.blue_mask,sizeof(int),1,fp) ;
        if (error == 0) return(2) ;
        
        
        totbytes=ximage.bytes_per_line*ximage.height;
        if (xloc+ximage.width  < 0 || xloc > lib_wins[0].width ||
            yloc+ximage.height < 0 || yloc > lib_wins[0].height)
          return(240) ;
        if (ximage.depth != lib_depth) 
          return(219);

        ximage.data=(char *)malloc(totbytes*sizeof(char)) ;
        if (ximage.data == NULL) return(1) ;
        error = v_ReadData(ximage.data, ximage.bits_per_pixel/8,
          totbytes/(ximage.bits_per_pixel/8), fp) ;
        if (error == 0) return(2) ;

        error=v_ReadData( (char*)&entries,sizeof(short),1,fp) ;
        if (error == 0) return(2) ;
        for(i=0;i<entries;i++) {
          error=v_ReadData( (char*)&pixel,sizeof(int),1,fp) ;
          if (error != 1) return(2) ;
          lib_colormap_colors[pixel&lib_pixel_mask].pixel =
            pixel;
          error=v_ReadData(
             (char*)&lib_colormap_colors[pixel&lib_pixel_mask].red,
            sizeof(short),1,fp) ;
          if (error != 1) return(2) ;
          error=v_ReadData(
             (char*)&lib_colormap_colors[pixel&lib_pixel_mask].green,
            sizeof(short),1,fp) ;
          if (error != 1) return(2) ;
          error=v_ReadData(
             (char*)&lib_colormap_colors[pixel&lib_pixel_mask].blue,
            sizeof(short),1,fp) ;
          if (error != 1) return(2) ;
          error=v_ReadData(
            &lib_colormap_colors[pixel&lib_pixel_mask].flags,
            sizeof(char),1,fp) ;
          if (error != 1) return(2) ;
        }
        fclose(fp) ;
        XStoreColors(lib_display,lib_cmap,lib_colormap_colors,entries) ;
        for (i=0; i<NUM_OF_RESERVED_COLORCELLS; i++) {
          pixel = lib_reserved_colors[i].pixel&lib_pixel_mask;
          lib_reserved_colors[i].red=lib_colormap_colors[pixel].red ;
          lib_reserved_colors[i].green=lib_colormap_colors[pixel].green ;
          lib_reserved_colors[i].blue=lib_colormap_colors[pixel].blue ;
        }
        error=VPutImage(lib_wins[0].win,&ximage,0,0,xloc,yloc,ximage.width,
                        ximage.height) ;
         
        free(ximage.data) ;
        return(error) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_OutputSaveScreenImage                        *
 *      Description     :  This function will output the image from the *
 *                         specified rectangular region of the image    *
 *                         window at the specified location to the      *
 *                         specified disk file. If opt==1, this         *
 *                         function will check whether the specified    *
 *                         file exists. If the file exists, the function*
 *                         will append the specified data to the end of *
 *                         file, else it will create  the file and write*
 *                         the data to the file. If opt==0, then the    *
 *                         function will overwrite the specified file   *
 *                         with the data if the file exists, or it will *
 *                         create the file and write the data to the    *
 *                         file if the file does not exist.             *
 *                         The save screen file will contain the number *
 *                         of frames in the first 4 bytes, followed by  *
 *                         the contents of the frames. Each frame will  *
 *                         contain the image, the number of the         *
 *                         colormap entries, and the colormap contents. *     
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         5 - improper file seek.                      *
 *      Parameters      :  scrn_file - Specifies the name of file to    *
 *                                     be output.                       *
 *                         xloc, yloc - Specifies x and y coordinates   *
 *                                      of the upper left corner of the *
 *                                      rectangle, relative to the      *
 *                                      origin of the image window.     *
 *                         width,height - Specifies the width and       *
 *                                        height in pixels of image     *
 *                                        window to be output.          *
 *                         opt - Specifies flag to indicate appending   *
 *                               screen to file or overwrite the file.  *
 *                               1 - append; 0 - overwrite.             *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        If the value xloc, yloc, width, height or opt *
 *                        is not valid, or scrn_file is NULL, a core    *
 *                        dump file is produced.                        *
 *      Related funcs   : VDeleteSaveSreenCommand,                      *
 *                        v_GetSaveScreenSwitchValue,                    *
 *                        VGetSaveScreenFilename,                       *
 *                        VCheckEventsInButtonWindow,                   *
 *                        VDisplaySaveScreenCommand,                    *
 *                        VDisplaySaveScreenImage.                      *
 *      History         : Written on November 7, 1990 by Hsiu-Mei Hung. *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                        Modified 10/18/94 lib_colormap_colors used
 *                        by Dewey Odhner
 *                        Modified: 3/6/03 XGetImage plane mask
 *                           corrected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_OutputSaveScreenImage ( char* scrn_file, int xloc, int yloc,
    int width, int height, int opt )
//int opt ; /* 1 - append, 0 - overwrite */
{
        FILE *fp ;
        XImage *ximage ;
        int totbytes, error, frame, i ;
        char msg1[80] ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("v_OutputSaveScreenImage.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (scrn_file == NULL) 
            v_print_fatal_error("v_OutputSaveScreenImage",
                "The pointer of scrn_file should not be NULL.",0) ;
        if (xloc < 0 || xloc >= lib_wins[0].width) {
            sprintf(msg1,"The range of xloc is >= 0 && < %d.",
                    lib_wins[0].width);
            v_print_fatal_error("v_OutputSaveScreenImage",msg1,xloc) ;
        }
        if (yloc < 0 || yloc >= lib_wins[0].height) {
            sprintf(msg1,"The range of yloc is >= 0 && < %d.",
                    lib_wins[0].height);
            v_print_fatal_error("v_OutputSaveScreenImage",msg1,yloc) ;
        }
        if (width <= 0 || width > lib_wins[0].width) {
            sprintf(msg1,"The range of width is > 0 && <= %d.",
                    lib_wins[0].width);
            v_print_fatal_error("v_OutputSaveScreenImage",msg1,width) ;
        }
        if (height <= 0 || height > lib_wins[0].height) {
            sprintf(msg1,"The range of height is > 0 && <= %d.",
                    lib_wins[0].height);
            v_print_fatal_error("v_OutputSaveScreenImage",msg1,height) ;
        }
        if (opt != 0 && opt != 1) 
            v_print_fatal_error("v_OutputSaveScreenImage",
                              "The value of opt is 0 or 1.",opt) ;

        if (opt == 1) {
            fp=fopen(scrn_file,"rb") ;
            if (fp == NULL) {
                fp=fopen(scrn_file,"wb") ;
                opt=0 ;
            }
            else {
                fclose(fp) ;
                fp=fopen(scrn_file,"r+b") ;
            }
        }
        else fp=fopen(scrn_file,"wb") ;
                if(fp == NULL) return(4);

        ximage=XGetImage(lib_display,lib_wins[0].win,xloc,yloc,width,
                         height, ~0, ZPixmap) ;
        if (ximage == NULL) {fclose(fp); return(238) ;}
        totbytes=ximage->bytes_per_line*ximage->height;

        if (opt == 0) {
            frame=1 ;
            error=v_WriteData( (unsigned char*)&frame,sizeof(int),1,fp) ;
            if (error != 1) return(3) ;
        }
        else {
            error=v_ReadData( (char*)&frame,sizeof(int),1,fp) ;
            if (error != 1) return(2) ;
            frame++ ;
            error=fseek(fp,0,0) ;
            if (error == -1) return(5) ;
            error=v_WriteData( (unsigned char*)&frame,sizeof(int),1,fp) ;
            if (error == 0) return(2) ;
            error= fseek(fp,0,2) ;
            if (error == -1) return(5) ;
        }
        /*
        error=fwrite(ximage,sizeof(XImage),1,fp) ;
        if (error == 0) return(3) ;
        */
        /* Added by supun - to make it machine independent */
        error=v_WriteData( (unsigned char*)&ximage->width,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->height,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->xoffset,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->format,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->byte_order,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->bitmap_unit,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->bitmap_bit_order,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->bitmap_pad,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->depth,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->bytes_per_line,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->bits_per_pixel,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->red_mask,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->green_mask,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&ximage->blue_mask,sizeof(int),1,fp) ;
        if (error == 0) return(3) ;
        
        error = v_WriteData( (unsigned char*)ximage->data, ximage->bits_per_pixel/8,
          totbytes/(ximage->bits_per_pixel/8), fp) ;
        if (error == 0) return(3) ;
        error=v_WriteData( (unsigned char*)&lib_cmap_entries,sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        for(i=0;i<lib_cmap_entries;i++) {
          error=v_WriteData( (unsigned char*)&lib_colormap_colors[i].pixel,sizeof(int),1,fp) ;
          error=v_WriteData( (unsigned char*)&lib_colormap_colors[i].red,sizeof(short),1,fp) ;
          error=v_WriteData( (unsigned char*)&lib_colormap_colors[i].green,sizeof(short),1,fp) ;
          error=v_WriteData( (unsigned char*)&lib_colormap_colors[i].blue,sizeof(short),1,fp) ;
          error=v_WriteData( (unsigned char*)&lib_colormap_colors[i].flags,sizeof(char),1,fp) ;
        }
        if (error == 0) return(3) ;
        fclose(fp) ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_GetSaveScreenSwitchValue                     *
 *      Description     : This function will return the value of the    *
 *                        "SCRN:APPND/OVERWR" switch.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory aloocation error.                 *
 *      Parameters      :  val -  value of the switch returned.         *
 *                         0 - APPND; 1 - OVERWR;                       *
 *      Side effects    : None.                                         *
 *      Entry condition : VDisplaySaveScreenCommand should be called    *
 *                        earlier.                                      *
 *      Related funcs   : VDeleteSaveSreenCommand,                      *
 *                        VDisplayScreenSwitchCommand,                  *
 *                        VGetSaveScreenFilename,                       *
 *                        VDisplaySaveScreenImage,                      *
 *                        v_OutputSaveScreenImage.                       *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_GetSaveScreenSwitchValue ( int* val )
{
 
        if(lib_savescreenswitch_cmds == NULL) return(1);

        *val = (int)lib_savescreenswitch_cmds[0].switch_item_displayed;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetSaveScreenFilename                        *
 *      Description     : This function will return the name entered    *
 *                        by the user in the text item area.            *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory aloocation error.                 *
 *      Parameters      :  name -  name of the file chosen by the user. *
 *                                 The default is temp.SCRN.            *
 *      Side effects    : None.                                         *
 *      Entry condition : VDisplaySaveScreenCommand should be called    *
 *                        earlier.                                      *
 *      Related funcs   : VDeleteSaveSreenCommand,                      *
 *                        VDisplayScreenSwitchCommand,                  *
 *                        v_GetSaveScreenSwitchValue,                    *
 *                        VDisplaySaveScreenImage,                      *
 *                        v_OutputSaveScreenImage.                       *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VGetSaveScreenFilename ( char* name )
{
        char value[100];

        if(lib_output2->value == NULL) return(1);
 
        if(!lib_display_save_screen_on) return(208);

        v_GetTextValue0(lib_output2,value, 1);
        strcpy(name,value);
        return(0);
}
 
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayOutputFilePrompt                      *
 *      Description     : This function will display the output file    *
 *                        switch, the filename to be filled in by the   *
 *                        user, and the save button.                    *
 *                        In order to change the option on the switch   *
 *                        the user will have to call the function       *
 *                        VCheckEventsInButtonWindow.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  saveswitch - an array of struct PanelCmdInfo *
 *                                      which contains the value to be  *
 *                                      displayed in the switch.        *
 *                                      If NULL is the value passed,    *
 *                                      then the switch is not displayed*
 *                         def_name -  a 2-D array of default filenames *
 *                                     to be displaed in the text item  *
 *                                     area. If NULL is the value passed*
 *                                     by the user, then "temp" is the  *
 *                                     filename assumed.                *
 *                         callback_func - the user callable function   *
 *                                         to be executed when the SAVE *
 *                                         button is pressed.           *
 *                                         The user can pass NULL also. * 
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDeleteOutputFilePrompt, VGetSaveSwitchValue,      *
 *                        VGetSaveFilename, VCheckEventsInButtonWindow, *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 12/22/94 private button list used
 *                        by Dewey Odhner.
 *                        Modified 12/22/94 private call for refresh
 *                        allowed with saveswitch=lib_saveswitch_cmds
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayOutputFilePrompt ( PanelCmdInfo* saveswitch,
    char def_name[][MAX_DEFAULT_CHAR], int (*callback_func)() )
{
    int xloc,yloc,xloc2,yloc2,offset,dum;
    int line_xloc;
    int switch_item_displayed,error;
    int i,j,count,width,max_width,avail_chars;

    XCharStruct overall;
    char text[100];
    int n,result;
    short but_width,but_height,label_width,label_height;
    short label_x,label_y,label_ascent;
    XTextItem tlabel;

    if(lib_cmap_created == 0) {
            fprintf(stderr, "The error occurred in the function ") ;
            fprintf(stderr, "VDisplayOutputFilePrompt.\n") ;
            fprintf(stderr, "Please call VCreateColormap before ") ;
            fprintf(stderr, "calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
    }

    if (lib_display_output_file_prompt && saveswitch!=lib_saveswitch_cmds) {
            fprintf(stderr, "The error occurred in the function ") ;
            fprintf(stderr, "VDisplayOutputFilePrompt.\n") ;
            fprintf(stderr, "Output file prompt is already displayed.\n") ;
            kill(getpid(),LIB_EXIT) ;
    }

    if(saveswitch != NULL && saveswitch[0].switches != NULL)
    {
        threedswitchgc=lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threedswitchgc,GCLineWidth,&vals);
        xloc=LIB_OUTPUT_FILE_SWITCH_XLOC;
        yloc=LIB_OUTPUT_FILE_SWITCH_YLOC;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;
        offset=1;

        v_create_threedswitch(threedswitchgc,xloc,yloc,xloc2,yloc2,offset);

        /*switch initialization*/
        if (!lib_display_output_file_prompt)
        { lib_saveswitch_num_of_cmds=1;
          if (lib_saveswitch_cmds == NULL)
		  	lib_saveswitch_cmds=(PanelCmdInfo *)calloc(1,
                                              sizeof(PanelCmdInfo));
          if (lib_saveswitch_cmds == NULL) return(1);
          lib_saveswitch_switches=(PanelSwitchInfo *)malloc(1*
                                        sizeof(PanelSwitchInfo));
          if (lib_saveswitch_switches == NULL) return(1);
          strncpy(lib_saveswitch_cmds[0].cmd,"OUTPUT",MAX_PANEL_CHARS);
          lib_saveswitch_cmds[0].cmd[MAX_PANEL_CHARS]='\0';
          if(saveswitch[0].type == 1)
                lib_saveswitch_cmds[0].type=saveswitch[0].type;
          else return(1); 
          lib_saveswitch_cmds[0].group=saveswitch[0].group;
          lib_saveswitch_cmds[0].num_of_switches=saveswitch[0].num_of_switches;
          if (lib_saveswitch_cmds[0].switches == NULL)
			lib_saveswitch_cmds[0].switches=(Char30 *)malloc(sizeof(Char30)*
                                      saveswitch[0].num_of_switches);
          if (lib_saveswitch_cmds[0].switches == NULL) return(1);
          avail_chars=MAX_PANEL_CHARS-strlen(lib_saveswitch_cmds[0].cmd)-1;
          for (j=0, max_width=0; j<saveswitch[0].num_of_switches; j++) 
          {
                count=strlen(saveswitch[0].switches[j]);
                if (count > avail_chars) 
                {
                  strncpy(lib_saveswitch_cmds[0].switches[j],
                            saveswitch[0].switches[j],avail_chars) ;
                  lib_saveswitch_cmds[0].switches[j][avail_chars]='\0';
                  result=254 ;
                }
                else strcpy(lib_saveswitch_cmds[0].switches[j],
                            saveswitch[0].switches[j]);
                width=XTextWidth(lib_wins[2].font,
                        lib_saveswitch_cmds[0].switches[j],
                        strlen(lib_saveswitch_cmds[0].switches[j]));
                if (width > max_width) max_width=width;
          }
          lib_saveswitch_cmds[0].switch_item_displayed=
                                saveswitch[0].switch_item_displayed;
          lib_saveswitch_switches[0].x=XTextWidth(lib_wins[2].font,
                        lib_saveswitch_cmds[0].cmd,
                        strlen(lib_saveswitch_cmds[0].cmd))+
                        lib_wins[2].font_width;
          lib_saveswitch_switches[0].width=max_width;
          MAX_WIDTH_OUTPUTFILE_SWITCH=max_width;
        }
        max_width = MAX_WIDTH_OUTPUTFILE_SWITCH;

        /***************displaying the string*************/
        line_xloc=xloc2-(max_width + 4 + 4);
        XSetForeground(lib_display,lib_wins[2].gc,
                        lib_reserved_colors[0].pixel);
        XDrawLine(lib_display,lib_wins[2].win,lib_wins[2].gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT);
 
        /***displaying the cmd***/
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
        strcpy(text,lib_saveswitch_cmds[0].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedswitchgc),
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
        label_y = (short)yloc + (but_height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedswitchgc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedswitchgc,
                  label_x,label_y,&tlabel,1);
        /***displaying the switch***/
        switch_item_displayed=lib_saveswitch_cmds[0].switch_item_displayed;
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
        strcpy(text,lib_saveswitch_cmds[0].switches[switch_item_displayed]);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedswitchgc),
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
 
        label_x = (short)line_xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedswitchgc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedswitchgc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /*initializing the text item*/
                MAX_NUM_OUTPUT_FILES=saveswitch[0].num_of_switches;
    }

    /*initializing the text item*/
    else    MAX_NUM_OUTPUT_FILES=1;

    if (lib_display_output_file_prompt)
    {
        v_DisplayText(lib_output);
        v_DisplayButton(lib_button);
    }
    else
    {
        for(i=0;i<MAX_NUM_OUTPUT_FILES;i++)
        {
            if(def_name == NULL) 
                    strcpy(DEFAULT_NAME[i],"temp");
            else
                    strcpy(DEFAULT_NAME[i],def_name[i]);
        }

        if(saveswitch != NULL && saveswitch[0].switches != NULL)
        {
            error = v_AddText0(lib_wins[2].win,
                 LIB_OUTPUT_FILE_TEXT_XLOC,
                 LIB_OUTPUT_FILE_TEXT_YLOC,
                 -LIB_STATICBUTTON_WIDTH,
                 -LIB_PANEL_ITEM_HEIGHT,
                 "",
                 DEFAULT_NAME[saveswitch[0].switch_item_displayed],     
                 NULL,
                 &lib_output, 1);

            if(error != 0) return(error); 
        }
        else
        {
            error = v_AddText0(lib_wins[2].win,
                 LIB_OUTPUT_FILE_TEXT_XLOC,
                 LIB_OUTPUT_FILE_TEXT_YLOC,
                 -LIB_STATICBUTTON_WIDTH,
                 -LIB_PANEL_ITEM_HEIGHT,
                 "",
                 DEFAULT_NAME[0],
                 NULL,
                 &lib_output, 1);
 
            if(error != 0) return(error);
        }


        /*initializing the button item*/
        if(callback_func != NULL)
        {
             error = v_AddButton0(lib_wins[2].win,
                   LIB_OUTPUT_FILE_BUTTON_XLOC,
                   LIB_OUTPUT_FILE_BUTTON_YLOC,
                   -LIB_STATICBUTTON_WIDTH,
                   0,
                   "  SAVE  ",
                   callback_func,
                   0,
                   &lib_button, 1);
        
            if(error != 0) return(error);
        }

        lib_display_output_file_prompt=1;
    }

    return(0);

}
/*********************************************************/
static int v_create_threedswitch ( GC gc, int xloc, int yloc,
    int xloc2, int yloc2, int offset )
{
        /***********fill the rectangles***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,gc,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);

        /***********bottom dark lines***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_SelectOutputFileCommand                     *
 *      Description     : This function will check whether the cursor   *
 *                        is within the switch created by the user when *
 *                        left button is pressed. The value of the      *
 *                        switch can be changed by calling the function *
 *                        VCheckEventsInButtonWindow.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         206 - the cursor is not within the command.  *
 *                         210 - the user specified switch is NULL.     *
 *      Parameters      :  event - the event passed by the user.        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        The user has to call VDisplayOutputFilePrompt *
 *                        and VSelectEvents before calling this         *
 *                        function.                                     *
 *      Related funcs   : VDeleteOutputFilePrompt,                       *
 *                        VDisplayOutputFilePrompt,                     *
 *                        VGetSaveFilename, VGetSaveSwitchValue,            *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_SelectOutputFileCommand ( XEvent* event )
{
        int xloc,yloc,xloc2,yloc2,xloc3,yloc3,xloc4,yloc4;
        int xloc5,yloc5,xloc6,yloc6;
        int curx,cury;
        int item,index,error;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("v_SelectOutputFileCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        /*for the panel like button*/
        xloc=LIB_OUTPUT_FILE_SWITCH_XLOC ;
        yloc=LIB_OUTPUT_FILE_SWITCH_YLOC ;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;
        /*for the button item*/
        xloc3=LIB_OUTPUT_FILE_BUTTON_XLOC;
        yloc3=LIB_OUTPUT_FILE_BUTTON_YLOC;
        if(lib_button != NULL)
        {
           xloc4=xloc3+lib_button->width;
           yloc4=yloc3+lib_button->height;
        }
        else
        {
           xloc4=0;
           yloc4=0;
        }
        /*for the text item*/
        xloc5=LIB_OUTPUT_FILE_TEXT_XLOC;
        yloc5=LIB_OUTPUT_FILE_TEXT_YLOC;
        xloc6=xloc5+lib_output->width;
        yloc6=yloc5+lib_wins[2].font_height;

        if(event->type == ButtonPress)
        {
          curx=event->xbutton.x;cury=event->xbutton.y;
          if(event->xbutton.button == 1 || event->xbutton.button == 2) 
          {
             /*checking button press in the panel like switch*/
             if(event->xbutton.x>xloc && event->xbutton.x<xloc2 &&
                event->xbutton.y>yloc && event->xbutton.y<yloc2)
             {
              if(lib_saveswitch_cmds != NULL && lib_saveswitch_switches != NULL)
              {
                item=lib_saveswitch_item_selected;
                if (lib_saveswitch_cmds[item].type != 1) return(206);
                if (item < lib_saveswitch_num_of_cmds && item >= 0) 
                {
                  if (event->xbutton.button == 1) {
                    lib_saveswitch_cmds[item].switch_item_displayed++;
                    if (lib_saveswitch_cmds[item].switch_item_displayed >=
                        lib_saveswitch_cmds[item].num_of_switches)
                        lib_saveswitch_cmds[item].switch_item_displayed=0;
                  }
                  else {
                    lib_saveswitch_cmds[item].switch_item_displayed--;
                    if (lib_saveswitch_cmds[item].switch_item_displayed < 0)
                        lib_saveswitch_cmds[item].switch_item_displayed=
                            lib_saveswitch_cmds[item].num_of_switches-1;
                  }
                }
                v_highlight_threedswitch(lib_saveswitch_cmds,
                                       threedswitchgc,
                                       MAX_WIDTH_OUTPUTFILE_SWITCH,
                                       xloc,yloc,xloc2,yloc2,1);
                 /**********changing the default file name***********/
                index =lib_saveswitch_cmds[0].switch_item_displayed; 
                if(strcmp(DEFAULT_NAME[index],"")!=0) {
                  error = VSetTabletValue(lib_output,DEFAULT_NAME[index]);
                  if(error != 0) return(error);
                }
                else {
                  error = VSetTabletValue(lib_output,"temp");
                  if (error != 0) return(error);
                }
 
                XFlush(lib_display);

              }
              else return(210); /*switch command is NULL*/
             }
        
             /*checking button press in the button item*/
             else if(event->xbutton.x>xloc3 && event->xbutton.x<xloc4 &&
                   event->xbutton.y>yloc && event->xbutton.y<yloc2)
             {
                if(lib_button != NULL) {
                  error = VCheckButtonItemEvent(event);
                  if(error != 0) return(error);
                }
             }

             else return(206); /*the cursor is not within the command*/
          }
          return(0);
        }

        else return(204);  /*invalid event type*/
}               

/***********************************************************************/
static int v_highlight_threedswitch ( PanelCmdInfo* buttonwinswitch, GC gc,
    int max_switch_width, int xloc, int yloc, int xloc2, int yloc2,
    int offset )
{
        int j,line_xloc;
        int dum;

        XCharStruct overall;
        char text[100];
        int n;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

        /***********fill the rectangles***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[3].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,gc,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top dark lines***********/
            XSetForeground(lib_display,gc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
        /***********bottom white lines***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[2].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);

        /***********************************/
        /*do a microsleep*/
            VSleep(100000);
        /***********************************/
 
        /*redraw the original button*/
         /***********fill the rectangles***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,gc,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
        
        /********writing the text again*/
        line_xloc=xloc2-(max_switch_width + 4 + 4);
        XSetForeground(lib_display,lib_wins[2].gc,
                      lib_reserved_colors[0].pixel);
        XDrawLine(lib_display,lib_wins[2].win,lib_wins[2].gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT);
        XFlush(lib_display);
 
        /***displaying the cmd***/
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
        strcpy(text,buttonwinswitch[0].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(gc),
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
        label_y = (short)yloc + (but_height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,gc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /***displaying the switch***/
        j=(int)buttonwinswitch[0].switch_item_displayed;
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
        strcpy(text,buttonwinswitch[0].switches[j]);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(gc),
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
 
        label_x = (short)line_xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,gc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);
        /*********************************************************/

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDeleteOutputFilePrompt                        *
 *      Description     : This function will erase the user specified   *
 *                        switch, the text item area and the SAVE       *
 *                        button and delete all the associated data     *
 *                        structures.                                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         210 - the user specified switch is NULL.     *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayoutputFilePrompt,                     *
 *                        VGetSaveSwitchValue, VGetSaveFilename,            *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 9/1/94 lib_saveswitch_cmds[
 *                        0].switches freed by Dewey Odhner.
 *                        Modified 12/22/94 private button list used
 *                        by Dewey Odhner.
 *                        Modified 1/3/95 lib_display_output_file_prompt
 *                        cleared by Dewey Odhner.
 *                        Modified 9/12/95 false error returns
 *                        removed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDeleteOutputFilePrompt ( void )
{
        int xloc,yloc,xloc2,yloc2;
        int error,width,height;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDeleteOutputFilePrompt.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        if(lib_display_output_file_prompt == 0) return(210);
        lib_display_output_file_prompt = 0;

        xloc=LIB_OUTPUT_FILE_SWITCH_XLOC-4;
        yloc=LIB_OUTPUT_FILE_SWITCH_YLOC;
        if(lib_button != NULL) 
        {
          width = lib_button->width;    
          height = lib_button->height;
        }
        else
        {
          width = 0;
          height = 0;
        }
        xloc2=LIB_OUTPUT_FILE_BUTTON_XLOC+width;
        yloc2=LIB_OUTPUT_FILE_BUTTON_YLOC+height;
        XClearArea(lib_display,lib_wins[2].win,xloc,yloc,
                   xloc2-xloc,yloc2-yloc,False) ;


        if(lib_saveswitch_cmds != NULL) {
            free(lib_saveswitch_cmds[0].switches);
            free(lib_saveswitch_cmds);
            lib_saveswitch_cmds = NULL;
        }
        if(lib_saveswitch_switches !=NULL) {
            free(lib_saveswitch_switches);
            lib_saveswitch_switches = NULL;
        }

        error = v_DeleteText0(lib_output, 1);
        if(error != 0) return(error);

        if(lib_button != NULL) {
            error = v_DeleteButton0(lib_button, 1);
            lib_button=NULL;
            if(error != 0) return(error);
        }

        lib_display_output_file_prompt=0;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetSaveSwitchValue                               *
 *      Description     : This function will return the value of the    *
 *                        user specified switch.                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  val -  value of the switch returned.         *
 *      Side effects    : None.                                         *
 *      Entry condition : VDisplayOutputFilePrompt should be called     *
 *                        earlier.                                      *
 *      Related funcs   : VDeleteOutputFilePrompt,                       *
 *                        VDisplayOutputFilePrompt,                     *
 *                        VGetSaveFilename, VGetSaveSwitchValue,            *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VGetSaveSwitchValue ( int* val )
{
 
        if(lib_saveswitch_cmds == NULL) return(1);
 
        *val = (int)lib_saveswitch_cmds[0].switch_item_displayed;
        return(0);
}
 
/************************************************************************
 *                                                                      *
 *      Function        : VGetSaveFilename                              *
 *      Description     : This function will return the name entered by *
 *                        the user in the text-item area. The default   *
 *                        is "temp".                                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  name -  name as seen in the text-item area.  *
 *      Side effects    : None.                                         *
 *      Entry condition : VDisplayOutputFilePrompt should be called     *
 *                        earlier.                                      *
 *      Related funcs   : VDeleteOutputFilePrompt,                       *
 *                        VDisplayOutputFilePrompt,                     *
 *                        VGetSaveFilename, VGetSaveSwitchValue,            *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VGetSaveFilename ( char* name )
{
        char value[100];

        if(lib_output->value == NULL) return(1);

        if(!lib_display_output_file_prompt) return(210);
 
        v_GetTextValue0(lib_output,value, 1);
        strcpy(name,value);
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetSaveFilenameList                          *
 *      Description     : This function will return the name entered by *
 *                        the user in the text-item area. The default   *
 *                        is "temp".                                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  name -  name as seen in the text-item area.  *
 *      Side effects    : None.                                         *
 *      Entry condition : VDisplayOutputFilePrompt should be called     *
 *                        earlier.                                      *
 *      Related funcs   : VDeleteOutputFilePrompt,                       *
 *                        VDisplayOutputFilePrompt,                     *
 *                        VGetSaveFilename, VGetSaveSwitchValue,            *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified: 8/15/95 number of names copied
 *                        changed by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int VGetSaveFilenameList ( char list[][MAX_DEFAULT_CHAR] )
{
        int i;
        char value[100];
 
        if(lib_output->value == NULL) return(1);
 
        if(!lib_display_output_file_prompt) return(210);
        
        for(i=0;i<lib_saveswitch_cmds[0].num_of_switches;i++)
                strcpy(list[i],DEFAULT_NAME[i]);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayDialogOnOffCommand                    *
 *      Description     : This function will display the                *
 *                        DIAL_WN:ON/OFF switch with
 *                        default value depending on
 *                        lib_dial_win_on_off_selected.
 *                        In order to change the option  *
 *                        the user has to call the function             *
 *                        VCheckEventsInButtonWindow.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDeleteDialogOnOffCommand,                     *
 *                        VCheckEventsInButtonWindow,                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 1/30/95 lib_dial_win_on_off_selected
 *                        checked by Dewey Odhner.
 *                        Modified 3/5/97 lib_dialogonoff_cmds,
 *                        lib_dialogonoff_switches checked for
 *                        previous allocation by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayDialogOnOffCommand ( void )
{
        int xloc,yloc,xloc2,yloc2,offset,dum;
        int line_xloc;
        int switch_item_displayed;
        int max_width;

        XCharStruct overall;
        char text[100];
        int n;
        short but_width,but_height,label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;
 
        if(lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplayDialogOnOffCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
 
       /*threeddialogonoffgc=XCreateGC(lib_display,lib_wins[2].win,0,&vals);*/
        threeddialogonoffgc=lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threeddialogonoffgc,GCLineWidth,&vals);
 
        xloc = LIB_DIAL_WIN_ON_OFF_XLOC;
        yloc = LIB_DIAL_WIN_ON_OFF_YLOC;
        xloc2 = xloc+LIB_STATICBUTTON_WIDTH;
        yloc2 = yloc+LIB_PANEL_ITEM_HEIGHT;
        offset = 1;

        v_create_threedswitch(threeddialogonoffgc,xloc,yloc,xloc2,yloc2,offset);

        /*switch initialization*/
        lib_dialogonoff_num_of_cmds=1;
        if (lib_dialogonoff_cmds == NULL)
			lib_dialogonoff_cmds=(PanelCmdInfo *)calloc(1,
                                              sizeof(PanelCmdInfo));
        if (lib_dialogonoff_cmds == NULL) return(1);
        if (lib_dialogonoff_switches == NULL)
			lib_dialogonoff_switches=(PanelSwitchInfo *)malloc(1*
                                        sizeof(PanelSwitchInfo));
        if (lib_dialogonoff_switches == NULL) return(1);
        strncpy(lib_dialogonoff_cmds[0].cmd,"DIAL_WN",MAX_PANEL_CHARS);
        lib_dialogonoff_cmds[0].cmd[MAX_PANEL_CHARS]='\0';
        lib_dialogonoff_cmds[0].type=1;
        lib_dialogonoff_cmds[0].group=1;
        lib_dialogonoff_cmds[0].num_of_switches=2;
		if (lib_dialogonoff_cmds[0].switches == NULL)
           lib_dialogonoff_cmds[0].switches=(Char30 *)malloc(sizeof(Char30)*2);
        if (lib_dialogonoff_cmds[0].switches == NULL) return(1);

        strcpy(lib_dialogonoff_cmds[0].switches[0],"ON");
        strcpy(lib_dialogonoff_cmds[0].switches[1],"OFF");      

        lib_dialogonoff_cmds[0].switch_item_displayed=
            lib_dial_win_on_off_selected? 0: 1;

        lib_dialogonoff_switches[0].x=XTextWidth(lib_wins[2].font,
                        lib_dialogonoff_cmds[0].cmd,
                        strlen(lib_dialogonoff_cmds[0].cmd))+
                        lib_wins[2].font_width;
        max_width=XTextWidth(lib_wins[2].font,
                        lib_dialogonoff_cmds[0].switches[1],
                        strlen(lib_dialogonoff_cmds[0].switches[1]));
        lib_dialogonoff_switches[0].width=max_width;

        MAX_WIDTH_DIALOGONOFF_SWITCH=max_width;
        /***************displaying the string*************/
        line_xloc=xloc2-(max_width + 4 + 4);
        XSetForeground(lib_display,lib_wins[2].gc,
                        lib_reserved_colors[0].pixel);
        XDrawLine(lib_display,lib_wins[2].win,lib_wins[2].gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT);
        XFlush(lib_display);

        /***displaying the cmd***/
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
        strcpy(text,lib_dialogonoff_cmds[0].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threeddialogonoffgc),
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
        label_y = (short)yloc + (but_height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threeddialogonoffgc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threeddialogonoffgc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /***displaying the switch***/
        switch_item_displayed=lib_dialogonoff_cmds[0].switch_item_displayed;
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
        strcpy(text,lib_dialogonoff_cmds[0].switches[switch_item_displayed]);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threeddialogonoffgc),
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
 
        label_x = (short)line_xloc + (but_width - label_width)/2;
        label_y = (short)yloc + (but_height - label_height)/2 + 
                                      label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threeddialogonoffgc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threeddialogonoffgc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);
        
        lib_display_dial_win_on=1 ;
        lib_dial_win_front=lib_dial_win_on_off_selected ;

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_SelectDialogOnOffCommand                    *
 *      Description     : This function will check whether the cursor   *
 *                        is within the "DIAL_WN:ON/OFF" switch and the *
 *                        left button is pressed. The value of the      *
 *                        switch can be changed by calling the function *
 *                        VCheckEventsInButtonWindow.                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         206 - the cursor is not within the command.  *
 *                         211 - the dialog on/off  switch is NULL.     *
 *      Parameters      :  event - the event passed by the user.        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        The user has to call                          *
 *                        VDisplayDialogOnOffCommand                    *
 *                        and VSelectEvents before calling this         *
 *                        function.                                     *
 *      Related funcs   : VDeleteDialogOnOffCommand,                     *
 *                        VDisplayDialogOnOffCommand,                   *
 *                        VCheckEventsInButtonWindow,                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 1/27/95 mouse button 2 enabled
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_SelectDialogOnOffCommand ( XEvent* event )
{
        int xloc,yloc,xloc2,yloc2;
        int curx,cury;
        int item,i,index;
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("v_SelectDialogOnOffCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
 
        /*for the panel like button*/
        xloc=LIB_DIAL_WIN_ON_OFF_XLOC ;
        yloc=LIB_DIAL_WIN_ON_OFF_YLOC ;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;

        if(event->type == ButtonPress)
        {
          if(lib_annotation_display==TRUE) return(286);
          curx=event->xbutton.x;cury=event->xbutton.y;
          if(event->xbutton.button==1 || event->xbutton.button==2)
          {
             /*checking button press in the panel like switch*/
             if(curx>xloc && curx<xloc2 && cury>yloc && cury<yloc2)
             {
              if(lib_dialogonoff_cmds != NULL && 
                        lib_dialogonoff_switches != NULL)
              {
                item=lib_dialogonoff_item_selected;
                if (lib_dialogonoff_cmds[item].type != 1) return(206);
                if (item < lib_dialogonoff_num_of_cmds && item >= 0)
                {
                  if (event->xbutton.button == 1) {
                    lib_dialogonoff_cmds[item].switch_item_displayed++;
                    if (lib_dialogonoff_cmds[item].switch_item_displayed >=
                        lib_dialogonoff_cmds[item].num_of_switches)
                        lib_dialogonoff_cmds[item].switch_item_displayed=0;
                  }
                  else {
                    lib_dialogonoff_cmds[item].switch_item_displayed--;
                    if (lib_dialogonoff_cmds[item].switch_item_displayed < 0)
                        lib_dialogonoff_cmds[item].switch_item_displayed=
                            lib_dialogonoff_cmds[item].num_of_switches-1;
                  }
                }
                v_highlight_threedswitch(lib_dialogonoff_cmds,
                                       threeddialogonoffgc,
                                       MAX_WIDTH_DIALOGONOFF_SWITCH,
                                       xloc,yloc,xloc2,yloc2,1);
        
                index = lib_dialogonoff_cmds[0].switch_item_displayed;
                if(index==0)
                {
                   if(lib_dial_win_on_off_selected == 1) return(0);
                   VRedisplayDialogWindow();
                }

                else if(index==1)
                {
                   if(lib_dial_win_on_off_selected == 0) return(0);
                   VRemoveDialogWindow();
                }
              }
              else return(211); /*dialog on/off  command switch is null*/
             }
             else return(206); /*the cursor is not within the command*/
          }
          return(0);
        }

        else return(204);  /*invalid event type*/

}

/************************************************************************
 *                                                                      *
 *      Function        : VDeleteDialogOnOffCommand                      *
 *      Description     : This function will erase the FG/BG command    *
 *                        switch from the button window and delete all  *
 *                        the associated data structures.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         211 - the dialog on/off switch is NULL.      *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayDialogOnOffCommand,                   *
 *                        VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 9/1/94 lib_dialogonoff_cmds[
 *                        0].switches freed by Dewey Odhner.
 *                        Modified 3/5/97 lib_dialogonoff_cmds
 *                        freed only once by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDeleteDialogOnOffCommand ( void )
{
        int xloc,yloc,xloc2,yloc2;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDeleteDialogOnOffCommand.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if(!lib_display_dial_win_on) return(211);
        lib_display_dial_win_on=0 ;
        lib_dial_win_front=1 ;

        xloc=LIB_DIAL_WIN_ON_OFF_XLOC-4;
        yloc=LIB_DIAL_WIN_ON_OFF_YLOC;
        xloc2=LIB_DIAL_WIN_ON_OFF_XLOC+LIB_STATICBUTTON_WIDTH;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;

        XClearArea(lib_display,lib_wins[2].win,
                   xloc,yloc,xloc2-xloc,yloc2-yloc,False);

        if(lib_dialogonoff_cmds != NULL) {
                free(lib_dialogonoff_cmds[0].switches);
                free(lib_dialogonoff_cmds);
				lib_dialogonoff_cmds = NULL;
        }
        if(lib_dialogonoff_switches !=NULL) free(lib_dialogonoff_switches);
		lib_dialogonoff_switches = NULL;
        if (lib_dial_win_on_off_selected == 1) return(0) ;
        lib_dial_win_on_off_selected=1 ;
        XRaiseWindow(lib_display,lib_wins[1].win) ;

        if(lib_dialogonoff_cmds != NULL)
                return(0);
        else return(211);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_DisplayStaticButton                          *
 *      Description     : This function will display the static buttons *
 *                        HELP, NOTES/INFO and ANNOTATION.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCheckEventsInButtonWindow.                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                        Modified 3/5/97 lib_statbut_cmds checked for
 *                        previous allocation by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_DisplayStaticButton ( void )
{
  PanelCmdInfo statbut[3];
  int num_of_statbut;
  int xloc,yloc,xloc2,yloc2,offset;
  int i,result;
  int count;

  XCharStruct overall;
  char text[100];
  int n,dum;
  short width,height,label_width,label_height;
  short label_x,label_y,label_ascent;
  XTextItem tlabel;

        statbut[0].group= 0;
        statbut[0].type = 0;
        strcpy(statbut[0].cmd,"ANNOTATION");
        statbut_func[0] = v_DoAnnotation;

        statbut[1].group= 0;
        statbut[1].type = 0;
        strcpy(statbut[1].cmd,"HELP");
        statbut_func[1] = v_DoHelp;

        statbut[2].group= 0;
        statbut[2].type = 0;
        strcpy(statbut[2].cmd,"INFORMATION");
        statbut_func[2] = v_DoInformation;

        num_of_statbut = 3;

        lib_statbut_num_of_cmds=num_of_statbut;
		if (lib_statbut_cmds == NULL)
        	lib_statbut_cmds=(PanelCmdInfo *)malloc(num_of_statbut*
                                              sizeof(PanelCmdInfo)) ;
        if (lib_statbut_cmds == NULL) return(1) ;

        for (i=0; i<num_of_statbut; i++) {
            count=strlen(statbut[i].cmd) ;
            if (count > MAX_PANEL_CHARS) {
                strncpy(lib_statbut_cmds[i].cmd,statbut[i].cmd,
                        MAX_PANEL_CHARS);
                lib_statbut_cmds[i].cmd[MAX_PANEL_CHARS]='\0';
                result=254;
            }
            else strcpy(lib_statbut_cmds[i].cmd,statbut[i].cmd) ;
            lib_statbut_cmds[i].type=statbut[i].type ;
            lib_statbut_cmds[i].group=statbut[i].group ;
        }

        /*****************Create Annotation button****************/
        /*threedannotationbuttongc = XCreateGC(lib_display,
                                lib_wins[2].win,0,&vals);*/
        threedannotationbuttongc=lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threedannotationbuttongc,GCLineWidth,&vals);
 
        xloc=LIB_ANNOTATION_BUTTON_XLOC;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc=LIB_ANNOTATION_BUTTON_YLOC;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;
        offset=1;
        v_create_threed_static_button(threedannotationbuttongc,
                                xloc,yloc,xloc2,yloc2,offset);

        /***displaying the string***/
        height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        width=(short)LIB_STATICBUTTON_WIDTH; /*this is in pixels*/

        strcpy(text,lib_statbut_cmds[0].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display, 
                        XGContextFromGC(threedannotationbuttongc),
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

        label_x = (short)(xloc + (width - label_width)/2);
        label_y = (short)(yloc + (height - label_height)/2 + label_ascent);

        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;

        XSetForeground(lib_display,threedannotationbuttongc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedannotationbuttongc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /*****************Create Help button********************/
        /*threedhelpbuttongc = XCreateGC(lib_display,
                                lib_wins[2].win,0,&vals);*/
        threedhelpbuttongc = lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threedhelpbuttongc,GCLineWidth,&vals);

        xloc=LIB_HELP_BUTTON_XLOC;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc=LIB_HELP_BUTTON_YLOC;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;
        offset=1;
        v_create_threed_static_button(threedhelpbuttongc,
                                xloc,yloc,xloc2,yloc2,offset);
 
        /***displaying the string***/
        height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        width=(short)LIB_STATICBUTTON_WIDTH; /*this is in pixels*/
 
        strcpy(text,lib_statbut_cmds[1].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threedhelpbuttongc),
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
 
        label_x = (short)xloc + (width - label_width)/2;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threedhelpbuttongc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threedhelpbuttongc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /*******************Create Notes button****************/
        /*threednotesbuttongc = XCreateGC(lib_display,
                                lib_wins[2].win,0,&vals);*/
        threednotesbuttongc = lib_wins[2].gc;
        vals.line_width=1;
        XChangeGC(lib_display,threednotesbuttongc,GCLineWidth,&vals);

        xloc=LIB_NOTES_BUTTON_XLOC;
        xloc2=xloc+LIB_STATICBUTTON_WIDTH;
        yloc=LIB_NOTES_BUTTON_YLOC;
        yloc2=yloc+LIB_PANEL_ITEM_HEIGHT;
        offset=1;
        v_create_threed_static_button(threednotesbuttongc,
                                xloc,yloc,xloc2,yloc2,offset);
 
        /***displaying the string***/
        height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        width=(short)LIB_STATICBUTTON_WIDTH; /*this is in pixels*/
 
        strcpy(text,lib_statbut_cmds[2].cmd);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,
                        XGContextFromGC(threednotesbuttongc),
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
 
        label_x = (short)xloc + (width - label_width)/2;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,threednotesbuttongc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  threednotesbuttongc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /*************************************************************/

        lib_display_static_button=1;
        
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_DoHelp                                      *
 *      Description     : This function will display the help window    *
 *                        with a series of options. The event loop      *
 *                        is now inside the help function.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCheckEventsInButtonWindow,                   *
 *                        v_DisplayStaticButton.                         *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_DoHelp ( void )
{
	VerticalMenuInfo vertical_menu_info;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS];
        char cmd[30],process[30],function[30],filetype[100],html_link[100];
        char context[100];
	FILE *fp;
	char path[100],def_path[100];
	char help_viewer[100],launch_help_viewer[100];
        int i,error,junk;

        if(lib_help_display == FALSE && lib_information_display == FALSE &&
                lib_annotation_display == FALSE)
        {
                /*lib_help_display = TRUE;*/
        	/* Get VIEWNIX_ENV value*/
        	strcpy(path, (char *)getenv("VIEWNIX_ENV"));
		strcpy(def_path,path);
        	strcat(path, "/FILES/DEFAULT");
        	if((fp = fopen(path, "rb")) == NULL)
        	{
		     error=VDisplayHelp();
                     if(error != 0) return(error);
                     VDisplayDialogMessage("Using 3dviewnix HELP Viewer.");
        	}
        	else
        	{
		/*Get the help displayer from the $VIEWNIX_ENV/FILES/DEFAULT*/ 
		/*file (14th line)*/
                     for(i=0; i<14; i++)
			fscanf(fp,"%s",help_viewer);
               	     fclose(fp);

		     if(strcmp(help_viewer,"DEFAULT_HELP_VIEWER")==0) {
			VDisplayDialogMessage("Using 3dviewnix HELP Viewer."); 
                        error=VDisplayHelp();
                        if(error != 0) return(error);
		     }
		     else {
		        VDisplayDialogMessage("Launching HELP Viewer specified in DEFAULT file.");
			/*Context sensitive information*/
			error=VReadMenucomFile(&vertical_menu_info,
                               horizontal_menu_info,
                               cmd,
                               process,
                               function,
                               filetype,
			       html_link,
                               &junk);

			/*v_get_context_in_html(context);*/
		        if(strcmp(cmd,"")==0)
			strcpy(html_link,"user_manual_contents.html");
			sprintf(launch_help_viewer,"%s file://localhost%s/FILES/HTML/user_manual/%s &",help_viewer,def_path,html_link);
		        error=system(launch_help_viewer);
		        if(error != 0) {
			   VDisplayDialogMessage("Couldn't use HELP Viewer specified in DEFAULT file. Using 3dviewnix HELP Viewer."); 
		           error=VDisplayHelp();
                           if(error != 0) return(error);
		        }
		     }
        	}

                lib_help_display = FALSE;
        }

        return(0);
}
 
static int v_get_context_in_html ( char* text )
{

	VerticalMenuInfo vertical_menu_info;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS];
        char cmd[30],process[30],function[30],filetype[100],html_link[100];
	int error,junk;

 	error=VReadMenucomFile(&vertical_menu_info,
                               horizontal_menu_info,
                               cmd,
                               process,
                               function,
                               filetype,
			       html_link,
                               &junk);

	/****port data commands****/
	if(strcmp(cmd,"FromAcrnema")==0)
	  sprintf(text,"%s","user_manual_chapter8.html#FromAcrnema");
	else if(strcmp(cmd,"EasyHeader")==0)
          sprintf(text,"%s","user_manual_chapter8.html#EasyHeader");
	else if(strcmp(cmd,"CreateFileHeader")==0)
          sprintf(text,"%s","user_manual_chapter8.html#CreateFileHeader");
	else if(strcmp(cmd,"modify_fileheader")==0)
          sprintf(text,"%s","user_manual_chapter8.html#modify_fileheader");
	else if(strcmp(cmd,"retrieve")==0)
          sprintf(text,"%s","user_manual_chapter8.html#retrieve");

	else if(strcmp(cmd,"backup")==0)
          sprintf(text,"%s","user_manual_chapter8.html#backup");
	else if(strcmp(cmd,"change_tape")==0)
          sprintf(text,"%s","user_manual_chapter8.html#change_tape");

	/****preprocess commands****/

	/****no known commands****/
	else 
          sprintf(text,"%s","user_manual_contents.html");

	return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_DoNotes                                     *
 *      Description     : This function will display the notes window   *
 *                        with a series of options. The event loop      *
 *                        is now inside the notes function.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCheckEventsInButtonWindow,                   *
 *                        v_DisplayStaticButton.                         *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_DoNotes ( void )
{
        int error;
 
        error=VDisplayDialogMessage("Notes function has been disabled.");
        if (error != 0) return(error);
        else if(error == 0) return(279);

}
 
/************************************************************************
 *                                                                      *
 *      Function        : v_DoInformation                               *
 *      Description     : This function will display the info window    *
 *                        with a series of options. The event loop      *
 *                        is now inside the info function.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCheckEventsInButtonWindow,                   *
 *                        v_DisplayStaticButton.                         *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_DoInformation ( void )
{
        int error;
 
        if(lib_help_display == FALSE && lib_information_display == FALSE &&
                lib_annotation_display == FALSE)
        {
        /*        lib_information_display = TRUE;*/
                error = VDisplayInformation(NULL,0);  //gjg
                if(error != 0) return(error);
                lib_information_display = FALSE;
        }
 
        return(0);

}

/************************************************************************
 *                                                                      *
 *      Function        : v_DoAnnotation                                *
 *      Description     : This function will display the help window    *
 *                        with a series of options. The event loop      *
 *                        is now inside the help function.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCheckEventsInButtonWindow,                   *
 *                        v_DisplayStaticButton.                         *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_DoAnnotation ( void )
{
        int error;

        if(lib_help_display == FALSE && lib_information_display == FALSE &&
                lib_annotation_display == FALSE)
        {
                lib_annotation_display = TRUE;
                error=VDisplayAnnotation(NULL);
                if(error != 0) return(error);
                lib_annotation_display = FALSE;
        }

        return(0);
}
 

/*********************************************************/
static int v_create_threed_static_button ( GC gc, int xloc, int yloc,
    int xloc2, int yloc2, int offset )
{

         /***********fill the rectangles***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,gc,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_SelectStaticButtonCommand                   *
 *      Description     : This function will check whether the cursor   *
 *                        is within the HELP; NOTES/INFO; ANNOTATION    *
 *                        button area and whether the                   *
 *                        left button is pressed. If so, the function   *
 *                        will display a window in the image window area*
 *                        and the event handler is controlled from      *
 *                        insid the new windows.                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         206 - the cursor is not within the command.  *
 *                         275 - the static buttons are not displayed.  *
 *      Parameters      :  event - the event passed by the user.        *
 *      Side effects    : The related type of event is ButtonPress, and *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        The user has to call v_DisplayStaticButton     *
 *                        and VSelectEvents before calling this         *
 *                        function.                                     *
 *      Related funcs   : v_DisplayStaticButton,                         *
 *                        VCheckEventsInButtonWindow,                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_SelectStaticButtonCommand ( XEvent* event, int (*refresh_func)() )
{
        int xloc_annotation,yloc_annotation,xloc2_annotation,yloc2_annotation;
        int xloc_help,yloc_help,xloc2_help,yloc2_help;
        int xloc_notes,yloc_notes,xloc2_notes,yloc2_notes;
        int curx,cury;
        int offset;
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VGetOutputFileDescription.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
 
        /*for the static buttons*/
        xloc_annotation=LIB_ANNOTATION_BUTTON_XLOC ;
        yloc_annotation=LIB_ANNOTATION_BUTTON_YLOC ;
        xloc2_annotation=xloc_annotation+LIB_STATICBUTTON_WIDTH;
        yloc2_annotation=yloc_annotation+LIB_PANEL_ITEM_HEIGHT;

        xloc_help=LIB_HELP_BUTTON_XLOC ;   
        yloc_help=LIB_HELP_BUTTON_YLOC ;   
        xloc2_help=xloc_help+LIB_STATICBUTTON_WIDTH; 
        yloc2_help=yloc_help+LIB_PANEL_ITEM_HEIGHT;

        xloc_notes=LIB_NOTES_BUTTON_XLOC ;   
        yloc_notes=LIB_NOTES_BUTTON_YLOC ;   
        xloc2_notes=xloc_notes+LIB_STATICBUTTON_WIDTH; 
        yloc2_notes=yloc_notes+LIB_PANEL_ITEM_HEIGHT;

        offset = 1; 

        if(event->type == ButtonPress)
        {
          if(lib_annotation_display==TRUE) return(286);
          curx=event->xbutton.x;cury=event->xbutton.y;
          if(event->xbutton.button != 3)
          {
             /*checking button press in the annotation button*/
             if(curx>xloc_annotation && curx<xloc2_annotation && 
                cury>yloc_annotation && cury<yloc2_annotation)
             {

                v_highlight_threedstaticbutton(threedannotationbuttongc,
                                        lib_statbut_cmds[0].cmd,
                                        xloc_annotation, yloc_annotation,
                                        xloc2_annotation,yloc2_annotation,
                                        offset);

                if(statbut_func[0] != NULL) {
                        (statbut_func[0])();
                        if(refresh_func != NULL) (refresh_func)();
                }
                else return(275); /*static buttons are not displayed*/

             }

             else if(curx>xloc_help && curx<xloc2_help && 
                cury>yloc_help && cury<yloc2_help)
             {
                 
                v_highlight_threedstaticbutton(threedhelpbuttongc,
                                        lib_statbut_cmds[1].cmd,
                                        xloc_help, yloc_help,
                                        xloc2_help,yloc2_help,
                                        offset);

                if(statbut_func[1] != NULL) {
                        (statbut_func[1])();
                        if(refresh_func != NULL) (refresh_func)();
                }
                else return(275); /*static buttons are not displayed*/

             }

             else if(curx>xloc_notes && curx<xloc2_notes && 
                cury>yloc_notes && cury<yloc2_notes)
             {
                 
                v_highlight_threedstaticbutton(threednotesbuttongc,
                                        lib_statbut_cmds[2].cmd,
                                        xloc_notes, yloc_notes,
                                        xloc2_notes,yloc2_notes,
                                        offset);

                if(statbut_func[2] != NULL) {
                        (statbut_func[2])();
                        if(refresh_func != NULL) (refresh_func)();
                }
                else return(275); /*static buttons are not displayed*/
             }

             else return(206); /*the cursor is not within the command*/
          }     
          return(0);
        }

        else return(204);  /*invalid event type*/
}

/*******************************************************/
static int v_highlight_threedstaticbutton ( GC gc, char* text,
    int xloc, int yloc, int xloc2, int yloc2, int offset )
{
  XCharStruct overall;
  int n,dum;
  short width,height,label_width,label_height;
  short label_x,label_y,label_ascent;
  XTextItem tlabel;


        /***********fill the rectangles***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[3].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,gc,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top dark lines***********/
            XSetForeground(lib_display,gc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
        /***********bottom white lines***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[2].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
        /*************display the string****************/
        height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        width=(short)LIB_STATICBUTTON_WIDTH; /*this is in pixels*/
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gc),
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
 
        label_x = (short)xloc + (width - label_width)/2;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,gc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        /***********************************/
        /*do a microsleep*/
        VSleep(100000);
        /***********************************/
 
        /*redraw the original button*/
         /***********fill the rectangles***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,gc,
                           xloc,yloc,
                           xloc2-xloc,yloc2-yloc);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,gc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc2-offset);
            XFlush(lib_display);
 
        /***********bottom dark lines***********/
            XSetForeground(lib_display,gc,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc+offset,
                        yloc2-offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[2].win,gc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc2-offset);
            XFlush(lib_display);
        
         /*************display the string****************/
        height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        width=(short)LIB_STATICBUTTON_WIDTH; /*this is in pixels*/
 
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display,XGContextFromGC(gc),
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
 
        label_x = (short)xloc + (width - label_width)/2;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,gc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);
        /*******************************************************/


        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckEventsInButtonWindow                    *
 *      Description     : This function will check which button/switche *
 *                        has been turned on by the user and pass the   *
 *                        the event to the selected button/switch.      *
 *                        It checks for all the events selected by the  *
 *                        user using VSelectEvents.                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         207 - the "run mode" command switch is NULL. *
 *                         208 - the savescreen command switch is NULL. *
 *                         210 - the user specified switch is NULL.     *
 *                         211 - the dialog on/off  switch is NULL.     *
 *                         275 - the static buttons are not displayed.  * 
 *      Parameters      :  event - the event passed by the user.        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayRunModeCommand, VDeleteRunModeCommand, *
 *                        VDisplaySaveScreenCommand,                    *
 *                        VDeleteSaveSreenCommand,                      *
 *                        VDisplayOutputFilePrompt,                     *
 *                        VDeleteOutputFilePrompt,                       *
 *                        VDisplayDialogOnOffCommand,                   *
 *                        VDeleteDialogOnOffCommand,v_DisplayStaticButton.*
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VCheckEventsInButtonWindow ( XEvent* event, int (*refresh_func)() )
{

        char value[100];
        int error,index;

        /*****checking button/keypress events in the text items******/
        if(lib_display_output_file_prompt==1 ||
           lib_display_save_screen_on==1) 
        {
        /*   error = v_SelectTextItems(event);*/
             error = v_CheckTextEvent_user(event, 1);
             if(event->type == KeyPress || event->type == KeyRelease)
             { 
                if(lib_saveswitch_cmds != NULL && 
                                lib_saveswitch_switches != NULL)
                {
                    v_GetTextValue0(lib_output,value, 1);
                    index = lib_saveswitch_cmds[0].switch_item_displayed;
                    strcpy(DEFAULT_NAME[index],value);
                }
             }
        }

        if(lib_display_output_file_prompt==1){
                error = v_SelectOutputFileCommand(event);
                if(error==0 || error==279) return(error);
        }
 
        if(lib_display_static_button==1){
                error = v_SelectStaticButtonCommand(event,refresh_func);
                if(error==0 || error==279) return(error);
        }
 
        if(lib_display_dial_win_on==1){
                error = v_SelectDialogOnOffCommand(event);
                if(error==0 || error==279) return(error);
        }
 
        if(lib_display_fg_bg_on==1){
                error = v_SelectRunModeCommand(event);
                if(error==0 || error==279) return(error);
        }
 
        if(lib_display_save_screen_on==1){
                error = v_SelectSaveScreenCommand(event);
                if(error==0 || error==279) return(error);
        }

        return(error);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_SelectTextItem                              *
 *      Description     : This function will check whether the cursor   *
 *                        is within the text item area in the SaveScreen*
 *                        or SaveFile command and will make sure that   *
 *                        text item list is unique for these two items  *
 *                        and update the text item list.                *
 *      Return Value    :  0 - work successfully.                       *
 *                         206 - cursor not within command area.        *
 *                         271 - the text item is empty.                *
 *      Parameters      :  event - the event passed by the user.        *
 *      Side effects    : The related type of event is ButtonPress and  *
 *                        keypress, and                                 *
 *                        the related window is button window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *                        The user has to call v_DisplayStaticButton     *
 *                        and VSelectEvents before calling this         *
 *                        function.                                     *
 *      Related funcs   : v_DisplayStaticButton,                         *
 *                        VCheckEventsInButtonWindow,                   *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_SelectTextItems ( XEvent* event )
{

        int curx,cury,error;
        int xloco1,xloco2,yloco1,yloco2; /*output file text item*/
        int xlocs1,xlocs2,ylocs1,ylocs2; /*save screen text item*/

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("v_SelectTextItems.\n");
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        if(lib_display_output_file_prompt==1)
        {
                xloco1=LIB_OUTPUT_FILE_TEXT_XLOC;
                xloco2=xloco1+lib_output->width;
                yloco1=LIB_OUTPUT_FILE_TEXT_YLOC;
                yloco2=yloco1+lib_wins[2].font_height;
        }
        else
                xloco1=xloco2=yloco1=yloco2=0;

        if(lib_display_save_screen_on==1)
        {
                xlocs1=LIB_SAVE_SCREEN_TEXT_XLOC;
                xlocs2=xlocs1+lib_output2->width;
                ylocs1=LIB_SAVE_SCREEN_TEXT_YLOC;
                ylocs2=ylocs1+lib_wins[2].font_height;
        }
        else
                xlocs1=xlocs2=ylocs1=ylocs2=0;



             curx=event->xbutton.x;cury=event->xbutton.y;
             if((curx>xloco1 && curx<xloco2 && cury>yloco1 && cury<yloco2) ||
                (curx>xlocs1 && curx<xlocs2 && cury>ylocs1 && cury<ylocs2))
             {  
                 error = v_CheckTextEvent_user(event, 1);
                 if(error != 0) return(error);
                 else return(0);
             }
        
             else return(206);  /*incorrect location of cursor*/

}
/*******************************************************************/


