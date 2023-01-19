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
 *      Filename  : info.c                                              *
 *      Ext Funcs : VDisplayInformation.                                *
 *      Int Funcs : v_list_files,v_CheckEventsInInformationWindow,      *
 *                  v_free_information,v_destroy_information_window,    *
 *                  v_initialize_file_flags,v_single_file_check,        *
 *                  v_display_information,v_clear_windows,              *
 *                  v_clear_filedisplay_window,                         *
 *                  v_display_general_information,                      *
 *                  v_display_scene_information,                        *
 *                  v_display_structure_information,                    *
 *                  v_display_display_information.                      *
 *                                                                      *
 ************************************************************************/
#include "Vlibrary.h"
#include <sys/types.h>
#include <unistd.h>
#include "3dv.h"

#define TEXT_LENGTH 28

GlobalInfo lib_infoglobal;/*the structure used in VGetGlobalInformation*/
FileInfo *lib_infofiles;  /*the structure used in VGetInputFiles*/
File_Info *lib_infofinfo; /*the structure used in notes and input*/
int lib_num_files;
int lib_max_files;
char *lib_infos[13],*infow[13];
WindowGcInfo *lib_information;
int INFOCLICK_XLOC,INFOCLICK_YLOC,INFOCLICK_WIDTH,INFOCLICK_HEIGHT;
int QUITINFO_XLOC,QUITINFO_YLOC,QUITINFO_WIDTH,QUITINFO_HEIGHT;
int RESETINFO_XLOC,RESETINFO_YLOC,RESETINFO_WIDTH,RESETINFO_HEIGHT;
int GENERALINFO_XLOC,GENERALINFO_YLOC,GENERALINFO_WIDTH,GENERALINFO_HEIGHT;
int SCENEINFO_XLOC,SCENEINFO_YLOC,SCENEINFO_WIDTH,SCENEINFO_HEIGHT;
int STRUCTUREINFO_XLOC,STRUCTUREINFO_YLOC,STRUCTUREINFO_WIDTH;
int STRUCTUREINFO_HEIGHT;
int DISPLAYINFO_XLOC,DISPLAYINFO_YLOC,DISPLAYINFO_WIDTH,DISPLAYINFO_HEIGHT;
int FILEDISPLAY_XLOC,FILEDISPLAY_YLOC,FILEDISPLAY_WIDTH,FILEDISPLAY_HEIGHT;
int FILEDISPLAY_XLOC2,FILEDISPLAY_YLOC2;
int FILEDISPLAYTOPCLICK_XLOC,FILEDISPLAYTOPCLICK_YLOC;
int FILEDISPLAYTOPCLICK_WIDTH,FILEDISPLAYTOPCLICK_HEIGHT;
int FILEDISPLAYBOTTOMCLICK_XLOC,FILEDISPLAYBOTTOMCLICK_YLOC;
int FILEDISPLAYBOTTOMCLICK_WIDTH,FILEDISPLAYBOTTOMCLICK_HEIGHT;
int FILEDISPLAY_SCROLL_ACTIVATE_FLAG=FALSE;
int FILEDISPLAY_SCROLL_INDEX=0;
int INFOSCROLLBAR_WIDTH;
int INFORMATION_SCROLLBAR=1; 
int INFORMATION_NOSCROLLBAR=0;
int INFORMATION_BUTTON=1; 
int INFORMATION_WINDOW=0;

static int v_CheckEventsInInformationWindow ( XEvent* event );
static int v_clear_filedisplay_window ( void );
static int v_clear_windows ( void );
static int v_destroy_information_window ( void );
static int v_display_display_information ( int index );
static int v_display_general_information ( int index );
static int v_display_information ( int index );
static int v_display_scene_information ( int index );
static int v_display_structure_information ( int index );
static int v_free_information ( void );
static int v_list_files ( int scroll_index );
static int v_single_file_check ( void );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayInformation                           *
 *      Description     : This function will display information window *
 *                        and list all the files in the file display    *
 *                        area. The first file is highlighted and       *
 *                        the corresponding information is displayed in *
 *                        the information display area. The list of     *
 *                        files to be displayed can be passed from      *
 *                        outside (as is done by the INPUT process)     *
 *                        or if it is not passed, then the files are    *
 *                        read from the GLOBAL.COM file.                *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         284 - help window is currently displayed.    *
 *                         285 - info window is currently displayed.    *
 *                         286 - annotation mode is on.                 *
 *      Parameters      :  outside_files - the list of files which are  *
 *                              to be displayed. If this is NULL, then  *
 *                              the files are read from the             *
 *                              GLOBAL.COM file.                        *
 *                         num_outside_files - the number of files      *
 *                              that are passed from outside. If this   *
 *                              is NULL, then the number of files is    *
 *                              computed from the GLOBAL.COM file.      *
 *      Side effects    : The related window is image window.           *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDoInformation,VCheckEventsInButtonWindow.    *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified 9/1/94 lib_infofiles freed by Dewey
 *                        Odhner.
 *                        Modified 2/14/00 not to list files when none
 *                        are selected by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayInformation ( char outside_files[MAX_DEFAULT_FILES][20],
    int num_outside_files )
{

        XEvent event;
        int yloc=10,xloc=20; /*the location for displying the strings*/
        int quit,len,error;
        int i,junk,root_width,root_height;
        int file_display_box_x,file_display_box_y;
        int file_display_box_width,file_display_box_height;
        int file_display_box_x2,file_display_box_y2;
        int generalinfo_display_box_x,generalinfo_display_box_y;
        int generalinfo_display_box_width,generalinfo_display_box_height;
        int generalinfo_display_box_x2,generalinfo_display_box_y2;
        int sceneinfo_display_box_x,sceneinfo_display_box_y;
        int sceneinfo_display_box_width,sceneinfo_display_box_height;
        int sceneinfo_display_box_x2,sceneinfo_display_box_y2;
        int structureinfo_display_box_x,structureinfo_display_box_y;
        int structureinfo_display_box_width,structureinfo_display_box_height;
        int structureinfo_display_box_x2,structureinfo_display_box_y2;
        int displayinfo_display_box_x,displayinfo_display_box_y;
        int displayinfo_display_box_width,displayinfo_display_box_height;
        int displayinfo_display_box_x2,displayinfo_display_box_y2;

        int button_box_x,button_box_y;
        int button_box_width,button_box_height;
        int button1_x,button1_y,button1_width,button1_height;
        int button2_x;

        MarginClickInfo click;
        ScrollbarClickInfo right_click;

        if(lib_cmap_created == 0) {
           printf("The error occurred in the function ") ;
           printf("VDisplayInformation.\n") ;
           printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
           kill(getpid(),LIB_EXIT) ;
        }

        if(lib_information_display==TRUE) return(285);
        if(lib_help_display==TRUE) return(284);
        if(lib_annotation_display==TRUE) return(286);

        lib_information_display = TRUE;

        if(outside_files == NULL)
        { if (lib_infofiles)
              free(lib_infofiles);
          error=VGetInputFiles(&lib_infofiles, &lib_num_files, MULTIPLE_FILES,(IM0 | CIM | MV0 | BIM | CBI | SH0 | BS0 | CV0 | S03 | S04 | BS1 | S13 | BS2));
          if(error == 4) {
                VDisplayDialogMessage("GLOBAL.COM was not found");
                return(0);
          }
        }

        lib_information=(WindowGcInfo *)calloc(1,sizeof(WindowGcInfo));
        if (lib_information == NULL) return(1);

        lib_information->x=100; 
        lib_information->y=50; 
        lib_information->font_width=lib_wins[0].font_width;
        lib_information->font_height=lib_wins[0].font_height;
        if(lib_information->font_height > 20) lib_information->font_height=13;
        if(lib_information->font_width > 10) lib_information->font_width=10;
        lib_information->width=lib_information->font_width*100;
        lib_information->height=lib_information->font_width*70;

        INFOSCROLLBAR_WIDTH = lib_information->font_width*2; 

	VGetGeometry(&junk,&junk,&root_width,&root_height);
        if((lib_information->x+lib_information->width >= root_width) ||
                (lib_information->y+lib_information->height >= root_height))
        {
                VDisplayDialogMessage("The window size is too small to display INFORMATION window!");
                XBell(lib_display,0);
                return(0);
        }

        error=VCreate3DImageSubwindow(&lib_information->win,
                                lib_information->x,
                                lib_information->y,
                                lib_information->width,
                                lib_information->height);
        if(error != 0) return(error);

        /*VGetWindowGC(lib_information->win,&lib_information->gc);*/
        error=VGetWindowGC(lib_wins[1].win,&lib_information->gc);
        if(error != 0) return(error);
        XFlush(lib_display);

        error=VDisplayCaptionBar(lib_information->win,lib_information->gc,
                        "INFORMATION",
                        0,0,lib_information->width,NULL,&click);
        if(error != 0) return(error);

        INFOCLICK_XLOC = click.x;
        INFOCLICK_YLOC = click.y;
        INFOCLICK_WIDTH = click.width;
        INFOCLICK_HEIGHT = click.height;

        XUngrabPointer(lib_display,CurrentTime);
        XGrabPointer(lib_display,lib_information->win,True,
                        PointerMotionMask,GrabModeAsync,GrabModeAsync,
                        lib_information->win,None,CurrentTime);
        /*********file display box***********/
        file_display_box_x = lib_information->font_width*2;
        file_display_box_y = lib_information->font_width*5;
        file_display_box_width = lib_information->font_width*30;
        file_display_box_height = lib_information->font_width*60;
        file_display_box_x2 = file_display_box_x+file_display_box_width;
        file_display_box_y2 = file_display_box_y+file_display_box_height;

        FILEDISPLAY_XLOC = file_display_box_x;
        FILEDISPLAY_YLOC = file_display_box_y;
        FILEDISPLAY_WIDTH = file_display_box_width;
        FILEDISPLAY_HEIGHT = file_display_box_height;
        FILEDISPLAY_XLOC2 = file_display_box_x2;
        FILEDISPLAY_YLOC2 = file_display_box_y2;
        /*********sceneinfo display box*********/
        generalinfo_display_box_x = file_display_box_x2 +
                        lib_information->font_width*2;
        generalinfo_display_box_y = file_display_box_y;
        generalinfo_display_box_width = (lib_information->width -
                        generalinfo_display_box_x - file_display_box_x)/2 - 
                        lib_information->font_width;
        generalinfo_display_box_height = lib_information->font_width*25;
        generalinfo_display_box_x2 = generalinfo_display_box_x +
                                generalinfo_display_box_width;
        generalinfo_display_box_y2 = generalinfo_display_box_y +
                                generalinfo_display_box_height;

        button_box_height = lib_information->font_width*5;

        GENERALINFO_XLOC = generalinfo_display_box_x; 
        GENERALINFO_YLOC = generalinfo_display_box_y;
        GENERALINFO_WIDTH = generalinfo_display_box_width;
        GENERALINFO_HEIGHT = generalinfo_display_box_height;
        /*********structureinfo display box***********/
        sceneinfo_display_box_x = generalinfo_display_box_x2+
                        lib_information->font_width*2;
        sceneinfo_display_box_y = generalinfo_display_box_y;
        sceneinfo_display_box_width = generalinfo_display_box_width;
        sceneinfo_display_box_height = generalinfo_display_box_height;
        sceneinfo_display_box_y2 = sceneinfo_display_box_y+
                                 sceneinfo_display_box_height;

        SCENEINFO_XLOC = sceneinfo_display_box_x;
        SCENEINFO_YLOC = sceneinfo_display_box_y;
        SCENEINFO_WIDTH = sceneinfo_display_box_width;
        SCENEINFO_HEIGHT = sceneinfo_display_box_height;
        /*********structureinfo display box***********/
        structureinfo_display_box_x = generalinfo_display_box_x;
        structureinfo_display_box_y = generalinfo_display_box_y2+
                                        lib_information->font_width*2;
        structureinfo_display_box_width = generalinfo_display_box_width;
        structureinfo_display_box_height = generalinfo_display_box_height;
        structureinfo_display_box_y2 = structureinfo_display_box_y+
                                 structureinfo_display_box_height;

        STRUCTUREINFO_XLOC = structureinfo_display_box_x;
        STRUCTUREINFO_YLOC = structureinfo_display_box_y;
        STRUCTUREINFO_WIDTH = structureinfo_display_box_width;
        STRUCTUREINFO_HEIGHT = structureinfo_display_box_height;
        /*********displayinfo display box***********/
        displayinfo_display_box_x = sceneinfo_display_box_x;
        displayinfo_display_box_y = structureinfo_display_box_y;
        displayinfo_display_box_width = generalinfo_display_box_width;
        displayinfo_display_box_height = generalinfo_display_box_height; 
        displayinfo_display_box_y2 = displayinfo_display_box_y +
                                displayinfo_display_box_height;

        DISPLAYINFO_XLOC = displayinfo_display_box_x;
        DISPLAYINFO_YLOC = displayinfo_display_box_y;
        DISPLAYINFO_WIDTH = displayinfo_display_box_width;
        DISPLAYINFO_HEIGHT = displayinfo_display_box_height;
        /*********button box***********/
        button_box_x =  generalinfo_display_box_x;
        button_box_y =  structureinfo_display_box_y2+
                        lib_information->font_width*2;
        button_box_width = lib_information->width - 
                        generalinfo_display_box_x - file_display_box_x;

        button1_width = (button_box_width-lib_information->font_width*4)/1;
        button1_height = lib_information->font_width*3;
        button1_x =  button_box_x+lib_information->font_width*2;
        button1_y =  button_box_y+(button_box_height- button1_height)/2;

        button2_x = button1_x+button1_width+lib_information->font_width*2;

        QUITINFO_XLOC = button1_x;
        QUITINFO_YLOC = button1_y;
        QUITINFO_WIDTH = button1_width;
        QUITINFO_HEIGHT = button1_height;

        RESETINFO_XLOC = button2_x;
        RESETINFO_YLOC = QUITINFO_YLOC;
        RESETINFO_WIDTH = QUITINFO_WIDTH;
        RESETINFO_HEIGHT = QUITINFO_HEIGHT;

        /*********file display box*********/
        error=VDisplayBox(lib_information->win,lib_information->gc,
                        file_display_box_x, 
                        file_display_box_y,
                        file_display_box_width,
                        file_display_box_height,
                        INFORMATION_WINDOW,INFORMATION_SCROLLBAR,
                        INFOSCROLLBAR_WIDTH,
                        &right_click);
        if(error != 0) return(error);
        XFlush(lib_display);

        FILEDISPLAYTOPCLICK_XLOC = right_click.top_x;
        FILEDISPLAYTOPCLICK_YLOC = right_click.top_y;
        FILEDISPLAYTOPCLICK_WIDTH = right_click.top_width;
        FILEDISPLAYTOPCLICK_HEIGHT = right_click.top_height;
        FILEDISPLAYBOTTOMCLICK_XLOC = right_click.bottom_x;
        FILEDISPLAYBOTTOMCLICK_YLOC = right_click.bottom_y;
        FILEDISPLAYBOTTOMCLICK_WIDTH = right_click.bottom_width;
        FILEDISPLAYBOTTOMCLICK_HEIGHT = right_click.bottom_height;
        
        error=VDisplayCenteredText(lib_information->win,lib_information->gc,
                        "FILES SELECTED",
                        file_display_box_x,
                        file_display_box_y,
                        file_display_box_width-INFOSCROLLBAR_WIDTH,
                        lib_information->font_height*2);
        if(error != 0) return(error);
        XFlush(lib_display);

        /*********sceneinfo display box*********/
        error=VDisplayBox(lib_information->win,lib_information->gc,
                        generalinfo_display_box_x,
                        generalinfo_display_box_y,
                        generalinfo_display_box_width,
                        generalinfo_display_box_height,
                        INFORMATION_WINDOW,INFORMATION_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
        XFlush(lib_display);
 
        error=VDisplayCenteredText(lib_information->win,lib_information->gc,
                        "GENERAL INFORMATION",
                        generalinfo_display_box_x,
                        generalinfo_display_box_y,
                        generalinfo_display_box_width,
                        lib_information->font_height*2);
        if(error != 0) return(error);
        XFlush(lib_display);

        /*********sceneinfo display box*********/
        error=VDisplayBox(lib_information->win,lib_information->gc,
                        sceneinfo_display_box_x,
                        sceneinfo_display_box_y,
                        sceneinfo_display_box_width,
                        sceneinfo_display_box_height,
                        INFORMATION_WINDOW,INFORMATION_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
        XFlush(lib_display);
 
        error=VDisplayCenteredText(lib_information->win,lib_information->gc,
                        "SCENE INFORMATION",
                        sceneinfo_display_box_x,
                        sceneinfo_display_box_y,
                        sceneinfo_display_box_width,
                        lib_information->font_height*2);
        if(error != 0) return(error);
        XFlush(lib_display);

        /*********structureinfo display box*********/
        error=VDisplayBox(lib_information->win,lib_information->gc,
                        structureinfo_display_box_x,
                        structureinfo_display_box_y,
                        structureinfo_display_box_width,
                        structureinfo_display_box_height,
                        INFORMATION_WINDOW,INFORMATION_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
        XFlush(lib_display);
 
        error=VDisplayCenteredText(lib_information->win,lib_information->gc,
                        "STRUCTURE INFORMATION",
                        structureinfo_display_box_x,
                        structureinfo_display_box_y,
                        structureinfo_display_box_width,
                        lib_information->font_height*2);
        if(error != 0) return(error);
        XFlush(lib_display);

        /*********displayinfo display box*********/
        error=VDisplayBox(lib_information->win,lib_information->gc,
                        displayinfo_display_box_x,
                        displayinfo_display_box_y,
                        displayinfo_display_box_width,
                        displayinfo_display_box_height,
                        INFORMATION_WINDOW,INFORMATION_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
        XFlush(lib_display);
 
        error=VDisplayCenteredText(lib_information->win,lib_information->gc,
                        "DISPLAY INFORMATION",
                        displayinfo_display_box_x,
                        displayinfo_display_box_y,
                        displayinfo_display_box_width,
                        lib_information->font_height*2);
        if(error != 0) return(error);
        XFlush(lib_display);

        /*********button display box*********/
        error=VDisplayBox(lib_information->win,lib_information->gc,
                        button_box_x,
                        button_box_y,
                        button_box_width,
                        button_box_height,
                        INFORMATION_WINDOW,INFORMATION_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);

        /*********buttons*********/
         error=VDisplayBox(lib_information->win,lib_information->gc,
                        button1_x,
                        button1_y,
                        button1_width,
                        button1_height,
                        INFORMATION_BUTTON,INFORMATION_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);

        error=VDisplayCenteredText(lib_information->win,lib_information->gc,
                        "DISMISS",
                        button1_x,
                        button1_y,
                        button1_width,
                        button1_height);
        if(error != 0) return(error);


        /****************************************************************/
        /*   Reading the files from the GLOBAL.COM file                 */
        /****************************************************************/
        if(outside_files == NULL)
        {
          if (lib_infofiles)
              free(lib_infofiles);
          error=VGetInputFiles(&lib_infofiles, &lib_num_files, MULTIPLE_FILES, (IM0 | CIM | MV0 | BIM | CBI | SH0 | BS0 | CV0 | S03 | S04 | BS1 | S13 | BS2));
          if(error == 4) {
                VDisplayDialogMessage("GLOBAL.COM was not found");
                lib_num_files=0;                
          }

          xloc=file_display_box_x+lib_information->font_width*2;
          yloc=lib_information->font_height*6;  
          lib_max_files = lib_num_files;

          if((lib_infofinfo =
                (File_Info *)calloc(lib_max_files,sizeof(File_Info)))==NULL) 
          {
                fprintf(stderr,"Could not create File_Info data structue");
                exit(-1);      
          }

          for(i=0;i<lib_max_files;i++)
            strcpy(lib_infofinfo[i].full_file_name,lib_infofiles[i].filename);

          FILEDISPLAY_SCROLL_INDEX=0;
          lib_infofinfo[0].flag=TRUE; /*first file always highlighted*/
          v_list_files(FILEDISPLAY_SCROLL_INDEX);
          if (lib_num_files)
            v_display_information(0);
        }

        /****************************************************************/
        /*   Reading the files from outside                             */
        /****************************************************************/
        if(outside_files != NULL)
        {
 
          xloc=file_display_box_x+lib_information->font_width*2;
          yloc=lib_information->font_height*6;
          lib_max_files = num_outside_files;
 
          if((lib_infofinfo =
                (File_Info *)calloc(lib_max_files,sizeof(File_Info)))==NULL)
          {
                fprintf(stderr,"Could not create File_Info data structue");
                exit(-1);
          }

          for(i=0;i<lib_max_files;i++) 
            strcpy(lib_infofinfo[i].full_file_name,outside_files[i]);
          FILEDISPLAY_SCROLL_INDEX=0;
          lib_infofinfo[0].flag=TRUE; /*first file always highlighted*/
          v_list_files(FILEDISPLAY_SCROLL_INDEX);
          v_display_information(0);
        }

        error=VSelectEvents(lib_information->win,ButtonPressMask|KeyPressMask|
                        EnterWindowMask|OwnerGrabButtonMask|
                        PointerMotionMask);
        if(error != 0) return(error);

        XSelectInput(lib_display,lib_information->win,
                        ButtonPressMask|KeyPressMask|
                        ExposureMask|OwnerGrabButtonMask|
                        EnterWindowMask|PointerMotionMask);     

        quit = FALSE;
        while(!quit) {
                VNextEvent(&event);
                switch(event.type)
                {
 
                  case ButtonPress:
                       if(event.xbutton.button == 3) quit=TRUE;
                       if(event.xbutton.window!=lib_information->win)continue;
                       if(event.xbutton.window==lib_information->win)
                                quit=v_CheckEventsInInformationWindow(&event);
                        break;
                }
        }

        XWarpPointer(lib_display,None,lib_wins[2].win,0,0,0,0,
                LIB_NOTES_BUTTON_XLOC+LIB_STATICBUTTON_WIDTH/2,
                LIB_NOTES_BUTTON_YLOC+LIB_PANEL_ITEM_HEIGHT/2);
        XFlush(lib_display);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_list_files                                  *
 *      Description     : This function will list the files to be       *
 *                        displayed.                                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  scroll_index - where it is currently in      *
 *                              the scroll mode.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_list_files ( int scroll_index )
{

        int error,i,xloc,yloc;

        error=v_clear_filedisplay_window();
        if(error != 0) return(error);

        xloc=FILEDISPLAY_XLOC+lib_information->font_width*2;
        yloc=FILEDISPLAY_YLOC+lib_information->font_height*4;

        for(i=0;i<lib_max_files;i++)
        {
                lib_infofinfo[i].xloc = -1;
                lib_infofinfo[i].yloc = -1;
        }

        for(i=0+scroll_index;i<lib_max_files;i++) {
             if(yloc<FILEDISPLAY_YLOC2 - lib_information->font_height) 
             {
                FILEDISPLAY_SCROLL_ACTIVATE_FLAG=FALSE;
                if(i==lib_max_files-1 && i > 16)
                        FILEDISPLAY_SCROLL_ACTIVATE_FLAG=TRUE;
                error=VDisplayText(lib_information->win,lib_information->gc,
                             lib_infofinfo[i].full_file_name,
                             xloc,yloc);
                if(error != 0) return(error);
 
                lib_infofinfo[i].xloc = xloc;
                lib_infofinfo[i].yloc = yloc;
 
                if(lib_infofinfo[i].flag==1)
                        VDisplayFixedColorText(lib_information->win,
                                lib_information->gc,
                                lib_infofinfo[i].full_file_name,
                                lib_infofinfo[i].xloc,
                                lib_infofinfo[i].yloc);

                yloc += (int)(lib_information->font_height*1.5);
             }
             else { FILEDISPLAY_SCROLL_ACTIVATE_FLAG=TRUE;}
        }

        XFlush(lib_display);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_CheckEventsInInformationWindow              *
 *      Description     : This function will process all the events     *
 *                        that occur in the information window.         *
 *      Return Value    :  0 - work successfully.                       *
 *                         TRUE - exit the information window.          *
 *      Parameters      :  event - the current events in the            *
 *                              information window.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified: 10/26/99 quit with button 3
 *                           implemented by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_CheckEventsInInformationWindow ( XEvent* event )
{
        int curx,cury;
        int cur_xloc1,cur_xloc2,cur_yloc1,cur_yloc2;
        int i,error;

        if(event->type == ButtonPress)
        {
          curx=event->xbutton.x;cury=event->xbutton.y;

          /*********quitting the information window************/
          if(event->xbutton.button==Button3 ||
              curx>INFOCLICK_XLOC && curx<INFOCLICK_XLOC+INFOCLICK_WIDTH &&
              cury>INFOCLICK_YLOC && cury<INFOCLICK_YLOC+INFOCLICK_HEIGHT)
          {
                error=v_destroy_information_window();
                        if(error != 0) return(error);
                error=v_free_information();
                        if(error != 0) return(error);
                return(TRUE);
          }

          if(curx>QUITINFO_XLOC && curx<QUITINFO_XLOC+QUITINFO_WIDTH &&
              cury>QUITINFO_YLOC && cury<QUITINFO_YLOC+QUITINFO_HEIGHT)
          {
                error=VBlinkBox(lib_information->win,
                        lib_information->gc,
                        QUITINFO_XLOC,QUITINFO_YLOC,
                        QUITINFO_WIDTH,QUITINFO_HEIGHT);
                if(error != 0) return(error);
                error=v_destroy_information_window();
                        if(error != 0) return(error);
                error=v_free_information();
                        if(error != 0) return(error);
                return(TRUE);
          }

          /*********checking the list of files***********/
          for(i=0;i<lib_max_files;i++)
          {
                cur_xloc1 = lib_infofinfo[i].xloc-lib_information->font_width;
                cur_xloc2 = FILEDISPLAY_XLOC2-INFOSCROLLBAR_WIDTH-
                                lib_information->font_width;
                cur_yloc1 = lib_infofinfo[i].yloc-
                                lib_information->font_height;
                cur_yloc2 = lib_infofinfo[i].yloc;

                if(curx>cur_xloc1 && curx<cur_xloc2 &&
                   cury>cur_yloc1 && cury<cur_yloc2)
                {
                     error=v_single_file_check();
                        if(error != 0) return(error);
                     lib_infofinfo[i].flag = 1;
                     error=VDisplayFixedColorText(lib_information->win,
                                lib_information->gc,
                                lib_infofinfo[i].full_file_name,
                                lib_infofinfo[i].xloc,
                                lib_infofinfo[i].yloc);
                        if(error != 0) return(error);
                     XFlush(lib_display);
                     error=v_display_information(i);
                        if(error != 0) return(error);
                     XFlush(lib_display);
                }
 
          }

          /*********testing filedisplay(right-top) scroll buttons***********/
          if(curx>FILEDISPLAYTOPCLICK_XLOC &&
                curx<FILEDISPLAYTOPCLICK_XLOC+FILEDISPLAYTOPCLICK_WIDTH &&
                cury>FILEDISPLAYTOPCLICK_YLOC &&
                cury<FILEDISPLAYTOPCLICK_YLOC+FILEDISPLAYTOPCLICK_HEIGHT)
          {
		error=VDisplayXORBox(lib_information->win,
					  lib_information->gc,
					  FILEDISPLAYTOPCLICK_XLOC,
					  FILEDISPLAYTOPCLICK_YLOC,
					  FILEDISPLAYTOPCLICK_WIDTH,
					  FILEDISPLAYTOPCLICK_HEIGHT);
                if(error != 0) return(error);

                if(FILEDISPLAY_SCROLL_ACTIVATE_FLAG == TRUE) {
                        --FILEDISPLAY_SCROLL_INDEX;
                        if(FILEDISPLAY_SCROLL_INDEX == -1){
                                FILEDISPLAY_SCROLL_INDEX=0;
                                XBell(lib_display,0);
                        }
                        v_list_files(FILEDISPLAY_SCROLL_INDEX);
                }

		error=VDisplayXORBox(lib_information->win,
					  lib_information->gc,
                                          FILEDISPLAYTOPCLICK_XLOC,
                                          FILEDISPLAYTOPCLICK_YLOC,
                                          FILEDISPLAYTOPCLICK_WIDTH,
                                          FILEDISPLAYTOPCLICK_HEIGHT);
                if(error != 0) return(error);
 
                return(0);
          }
 
          /*********testing help (right-bottom) scroll buttons***********/
          if(curx>FILEDISPLAYBOTTOMCLICK_XLOC &&
              curx<FILEDISPLAYBOTTOMCLICK_XLOC+FILEDISPLAYBOTTOMCLICK_WIDTH &&
              cury>FILEDISPLAYBOTTOMCLICK_YLOC &&
              cury<FILEDISPLAYBOTTOMCLICK_YLOC+FILEDISPLAYBOTTOMCLICK_HEIGHT)
          {
                if(FILEDISPLAY_SCROLL_ACTIVATE_FLAG == TRUE) {
                        ++FILEDISPLAY_SCROLL_INDEX;
                        if(FILEDISPLAY_SCROLL_INDEX > lib_max_files-1) {
                                FILEDISPLAY_SCROLL_INDEX=lib_max_files-1;
                                XBell(lib_display,0);
                        }
                        v_list_files(FILEDISPLAY_SCROLL_INDEX);
                }

                return(0);
          }
 
        }
        return(FALSE);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_free_information                            *
 *      Description     : This function will free the data structures   *
 *                        associated with the information window and    *
 *                        turn the information flag off.                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified: 11/30/00 lib_information_display
 *                           checked by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_free_information ( void )
{

        if (lib_information_display)
			free(lib_information);

        lib_information_display=0;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_destroy_information_window                  *
 *      Description     : This function will destroy the information    *
 *                        window which is a subwindow of the image      *
 *                        window.                                       *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified: 11/30/00 v_free_information called
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_destroy_information_window ( void )
{
        int center_x,center_y;
        int error;

        center_x = lib_information->x+lib_information->width/2;
        center_y = lib_information->y+lib_information->height/2;

        XSetForeground(lib_display,lib_information->gc,
                        lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,lib_information->win,lib_information->gc,
                        lib_information->x,lib_information->y,
                        center_x,center_y);
        XDrawLine(lib_display,lib_information->win,lib_information->gc,
                        lib_information->x+lib_information->width,
                        lib_information->y,center_x,center_y);
        XDrawLine(lib_display,lib_information->win,lib_information->gc,
                        lib_information->x,
                        lib_information->y+lib_information->height,
                        center_x,center_y);
        XDrawLine(lib_display,lib_information->win,lib_information->gc,
                        lib_information->x+lib_information->width,
                        lib_information->y+lib_information->height,
                        center_x,center_y);
        XFlush(lib_display);
 
            
        error=VDeleteImageSubwindow(lib_information->win);
        if(error != 0) return(error);
        v_free_information();
        
        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : v_initialize_file_flags                       *
 *      Description     : This function will initialize the flags       *
 *                        associalted with all the files. It makes it   *
 *                        FALSE (unselected).                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_initialize_file_flags ( void )
{
        int i;

        for(i=0;i<lib_max_files;i++)
            lib_infofinfo[i].flag = 0;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_single_file_check                           *
 *      Description     : This function will check if there are any     *
 *                        other file file selected. If so it makes them *
 *                        FALSE (unselected).                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_single_file_check ( void )
{
        int i,error;

        for(i=0;i<lib_max_files;i++){
            if(lib_infofinfo[i].flag == 1) {
                error=VDisplayText(lib_information->win,lib_information->gc,
                        lib_infofinfo[i].full_file_name,
                        lib_infofinfo[i].xloc,
                        lib_infofinfo[i].yloc);
                if(error != 0) return(error);
                XFlush(lib_display);

                lib_infofinfo[i].flag = 0;
             }
        }


        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_display_information                         *
 *      Description     : This function will display the information    *
 *                        for the selected file. It displays general    *
 *                        scene information for IM0 and BIM files,      *
 *                        general and structure info for SH0,BS0,BS1,BS2*
 *                        files and general and display information     *
 *                        for MV0 files.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the file for which the information   *
 *                              is to be displayed.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified: 10/27/99 file extension corrected
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_display_information ( int index )
{
        char *extension, filename[100],group[6],element[6];
        int len,error;

        strcpy(filename,lib_infofinfo[index].full_file_name);
        extension = strrchr(filename, '.');
		*extension++ = 0;
        strcpy(lib_infofinfo[index].file_name, filename);
        strcpy(lib_infofinfo[index].ftype, extension);

        lib_infofinfo[index].fp =
                        fopen(lib_infofinfo[index].full_file_name,"rb");
        if (lib_infofinfo[index].fp==NULL) 
        {
           fprintf(stderr,"Can't open %s\n",
                        lib_infofinfo[index].full_file_name);
           fprintf(stderr,"Ignoring file\n");
         }
         else if(strcmp(lib_infofinfo[index].ftype,"IM0")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"CIM")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"MV0")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"BIM")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"CBI")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"SH0")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"BS0")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"CV0")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"S03")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"S04")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"BS1")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"S13")==0 ||
                 strcmp(lib_infofinfo[index].ftype,"BS2")==0)
         {
                VReadHeader(lib_infofinfo[index].fp,
                                &lib_infofinfo[index].vh,group,element);
                VGetHeaderLength(lib_infofinfo[index].fp,&len);
          }

        /****if it is scene data file***/
        if(strcmp(lib_infofinfo[index].ftype,"IM0")==0 ||
           strcmp(lib_infofinfo[index].ftype,"BIM")==0)
        {
           error=v_clear_windows();
                if(error != 0) return(error);
           error=v_display_general_information(index);
                if(error != 0) return(error);
           error=v_display_scene_information(index);
                if(error != 0) return(error);
        }

        /****if it is structure data file***/
        if(strcmp(lib_infofinfo[index].ftype,"SH0")==0 ||
           strcmp(lib_infofinfo[index].ftype,"BS0")==0 ||
           strcmp(lib_infofinfo[index].ftype,"BS1")==0 ||
           strcmp(lib_infofinfo[index].ftype,"BS2")==0)
        {
           error=v_clear_windows();
                if(error != 0) return(error);
           error=v_display_general_information(index);
                if(error != 0) return(error);
           error=v_display_structure_information(index);
                if(error != 0) return(error);
        }

        /****if it is display data file***/
        if(strcmp(lib_infofinfo[index].ftype,"MV0")==0)
        {
           error=v_clear_windows();
                if(error != 0) return(error);
           error=v_display_general_information(index);
                if(error != 0) return(error);
           error=v_display_display_information(index);
                if(error != 0) return(error);
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_clear_windows                               *
 *      Description     : This function will clear the four windows -   *
 *                        general,scene,structure and display.          *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_clear_windows ( void )
{

        /****clear the general info window****/
        XSetForeground(lib_display,lib_information->gc,
                       lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_information->win,
                       lib_information->gc,
                       GENERALINFO_XLOC + lib_information->font_width,
                       GENERALINFO_YLOC + lib_information->font_height*2,
                       GENERALINFO_WIDTH - lib_information->font_width*2,
                       GENERALINFO_HEIGHT - 
                                (int)(lib_information->font_height*2.5));

        /****clear the scene info window****/
        XFillRectangle(lib_display,lib_information->win,
                       lib_information->gc,
                       SCENEINFO_XLOC + lib_information->font_width,
                       SCENEINFO_YLOC + lib_information->font_height*2,
                       SCENEINFO_WIDTH - lib_information->font_width*2,
                       SCENEINFO_HEIGHT - 
                                (int)(lib_information->font_height*2.5));

        /****clear the structure info window****/
        XFillRectangle(lib_display,lib_information->win,
                       lib_information->gc,
                       STRUCTUREINFO_XLOC + lib_information->font_width,
                       STRUCTUREINFO_YLOC + lib_information->font_height*2,
                       STRUCTUREINFO_WIDTH - lib_information->font_width*2,
                       STRUCTUREINFO_HEIGHT - 
                                (int)(lib_information->font_height*2.5));

        /****clear the display info window****/
        XFillRectangle(lib_display,lib_information->win,
                       lib_information->gc,
                       DISPLAYINFO_XLOC + lib_information->font_width,
                       DISPLAYINFO_YLOC + lib_information->font_height*2,
                       DISPLAYINFO_WIDTH - lib_information->font_width*2,
                       DISPLAYINFO_HEIGHT - 
                                (int)(lib_information->font_height*2.5));

        XFlush(lib_display);

        return(0); 
}


/************************************************************************
 *                                                                      *
 *      Function        : v_clear_filedisplay_window                    *
 *      Description     : This function will clear the file display     *
 *                        area in the information window.               *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_clear_filedisplay_window ( void )
{
        XSetForeground(lib_display,lib_information->gc,
                       lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_information->win,
                       lib_information->gc,
                       FILEDISPLAY_XLOC + lib_information->font_width,
                       FILEDISPLAY_YLOC + lib_information->font_height*3,
                       FILEDISPLAY_WIDTH - INFOSCROLLBAR_WIDTH -
                                        lib_information->font_width*2,
                       FILEDISPLAY_HEIGHT - lib_information->font_height*4);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_display_general_information                 *
 *      Description     : This function will display the general        *
 *                        information for the selected file.            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the file for which the information   *
 *                              is to be displayed.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified: 9/24/02 SHELL2 added by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_display_general_information ( int index )
{
        char text[100],txt[TEXT_LENGTH];
        int xloc,yloc,i,error;

        xloc = GENERALINFO_XLOC+lib_information->font_width;

        i = index;

        /****patient id****/
        if(lib_infofinfo[i].vh.gen.patient_id_valid == 1)
                sprintf(text,"Data Set ID : %s",
                                lib_infofinfo[i].vh.gen.patient_id);
        else    sprintf(text,"Data Set ID : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc = GENERALINFO_YLOC+lib_information->font_height*3;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****description****/
        if(lib_infofinfo[i].vh.gen.description_valid == 1)
                sprintf(text,"Description : %s",
                                lib_infofinfo[i].vh.gen.description);
        else    sprintf(text,"Description : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****institution****/
        if(lib_infofinfo[i].vh.gen.institution_valid == 1)
                sprintf(text,"Institution : %s",
                                lib_infofinfo[i].vh.gen.institution);
        else    sprintf(text,"Institution : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****data type****/
        if(lib_infofinfo[i].vh.gen.data_type_valid == 1)
        {
              if(lib_infofinfo[i].vh.gen.data_type==0)
                sprintf(text,"Data Type   : IMAGE0");
              else if(lib_infofinfo[i].vh.gen.data_type==1)
                sprintf(text,"Data Type   : IMAGE1");
              else if(lib_infofinfo[i].vh.gen.data_type==200)
                sprintf(text,"Data Type   : MOVIE0");
              else if(lib_infofinfo[i].vh.gen.data_type==120)
                sprintf(text,"Data Type   : SHELL0");
              else if(lib_infofinfo[i].vh.gen.data_type==121)
                sprintf(text,"Data Type   : SHELL1");
              else if(lib_infofinfo[i].vh.gen.data_type==122)
                sprintf(text,"Data Type   : SHELL2");
              else if(lib_infofinfo[i].vh.gen.data_type==100)
                sprintf(text,"Data Type   : CURVE0");
              else if(lib_infofinfo[i].vh.gen.data_type==110)
                sprintf(text,"Data Type   : SURFACE0");
              else if(lib_infofinfo[i].vh.gen.data_type==111)
                sprintf(text,"Data Type   : SURFACE1");
                
        }
        else    sprintf(text,"Data Type   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****study_date****/
        if(lib_infofinfo[i].vh.gen.study_date_valid == 1)
                sprintf(text,"Study Date  : %s",
                                lib_infofinfo[i].vh.gen.study_date);
        else    sprintf(text,"Study Date  : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        return(0);
}       


/************************************************************************
 *                                                                      *
 *      Function        : v_display_scene_information                   *
 *      Description     : This function will display the scene          *
 *                        information for the selected file.            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the file for which the information   *
 *                              is to be displayed.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                        Modified 4/30/98 1% tolerance allowed for
 *                        uniform spacing by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_display_scene_information ( int index )
{
        char text[100],text2[100],text3[100],txt[TEXT_LENGTH];
        char meas_x[10],meas_y[10],meas_z[10],meas_t[10];
        int xloc,yloc,i,j,k,error;
        float first_slice_spacing,slice_spacing;
        int nonuniform_slice_spacing=FALSE;
        float first_volume_spacing,volume_spacing;
        int nonuniform_volume_spacing=FALSE;
        int first_volume,volume,nonuniform_volume=FALSE;
        int mink,maxk;
        int total_slices=0;
 
        xloc = SCENEINFO_XLOC+lib_information->font_width;
 
        i = index;

        /****cell size****/
        if(lib_infofinfo[i].vh.scn.xypixsz_valid == 1)
                sprintf(text,"Cell Size   : %3.2f X %3.2f X",
                        lib_infofinfo[i].vh.scn.xypixsz[0],
                        lib_infofinfo[i].vh.scn.xypixsz[1]);
        else    sprintf(text,"Cell Size   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc = SCENEINFO_YLOC + lib_information->font_height*3;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****slice/volume spacing****/
        if(lib_infofinfo[i].vh.scn.dimension == 3) 
        {
                if(lib_infofinfo[i].vh.scn.num_of_subscenes[0]==1)
                  sprintf(text,"             : Only One Slice");
                else {
                  first_slice_spacing=
                        lib_infofinfo[i].vh.scn.loc_of_subscenes[1] -
                        lib_infofinfo[i].vh.scn.loc_of_subscenes[0];
                  for(j=1;j<lib_infofinfo[i].vh.scn.num_of_subscenes[0];j++)
                  {
                     slice_spacing=lib_infofinfo[i].vh.scn.loc_of_subscenes[j]
                        - lib_infofinfo[i].vh.scn.loc_of_subscenes[j-1];
                     if (slice_spacing<=.99*first_slice_spacing ||
                         .99*slice_spacing>=first_slice_spacing)
                        nonuniform_slice_spacing=TRUE;
                  }
                  if(nonuniform_slice_spacing==TRUE)
                    sprintf(text,"Slice Spg   : Not Uniform");
                  else sprintf(text,"            : %3.2f", slice_spacing);
                }
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
        }

        if(lib_infofinfo[i].vh.scn.dimension == 4)
        {
                if(lib_infofinfo[i].vh.scn.num_of_subscenes[0]==1)
                  sprintf(text,"1 Vol");
                else {
                  first_volume_spacing=
                        lib_infofinfo[i].vh.scn.loc_of_subscenes[1] -
                        lib_infofinfo[i].vh.scn.loc_of_subscenes[0];
                  for(j=0;j<lib_infofinfo[i].vh.scn.num_of_subscenes[0];j++)
                  {
                     if(j+1 < lib_infofinfo[i].vh.scn.num_of_subscenes[0])
                     {
                        volume_spacing=
                          lib_infofinfo[i].vh.scn.loc_of_subscenes[j+1] -
                          lib_infofinfo[i].vh.scn.loc_of_subscenes[j];
                        if (volume_spacing<=.99*first_volume_spacing ||
                            .99*volume_spacing>=first_volume_spacing)
                          nonuniform_volume_spacing=TRUE;
                     }
                  }
                  if(nonuniform_volume_spacing==TRUE)
                    sprintf(text,"*");
                  else sprintf(text,"%3.2f", volume_spacing);
                }
                mink=lib_infofinfo[i].vh.scn.num_of_subscenes[0];
                maxk=lib_infofinfo[i].vh.scn.num_of_subscenes[0]+       
                     lib_infofinfo[i].vh.scn.num_of_subscenes[1];
                for(j=1;j<=lib_infofinfo[i].vh.scn.num_of_subscenes[0];j++)
                {
                        if(lib_infofinfo[i].vh.scn.num_of_subscenes[j] == 1)
                          sprintf(text2,"1 Slc");
                        else {
                          first_slice_spacing=
                            lib_infofinfo[i].vh.scn.loc_of_subscenes[mink+1] -
                            lib_infofinfo[i].vh.scn.loc_of_subscenes[mink];
                            for(k=mink;k<maxk;k++)
                            {
                                if(k+1 < maxk)
                                {
                                slice_spacing=
                                lib_infofinfo[i].vh.scn.loc_of_subscenes[k+1]-
                                lib_infofinfo[i].vh.scn.loc_of_subscenes[k];
                                if (slice_spacing<=.99*first_slice_spacing ||
                                    .99*slice_spacing>=first_slice_spacing)
                                        nonuniform_slice_spacing=TRUE;
                                }
                            }
                            if(nonuniform_slice_spacing==TRUE)
                            sprintf(text2,"*");
                            else sprintf(text2,"%3.2f", slice_spacing);
                        }
                        mink=maxk;
                        maxk+=lib_infofinfo[i].vh.scn.num_of_subscenes[j];
                        
                 }      

        sprintf(text3,"            : %s X %s",text2,text);
        strncpy(txt,text3,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        }

        /****Measurement Unit****/
        if(lib_infofinfo[i].vh.scn.measurement_unit_valid==1)
        {
                if(lib_infofinfo[i].vh.scn.measurement_unit[0]==0)
                        sprintf(meas_x,"km");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==1)
                        sprintf(meas_x,"m");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==2)
                        sprintf(meas_x,"cm");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==3)
                        sprintf(meas_x,"mm");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==4)
                        sprintf(meas_x,"micron");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==5)
                        sprintf(meas_x,"sec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==6)
                        sprintf(meas_x,"msec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[0]==7)
                        sprintf(meas_x,"microsec");

                if(lib_infofinfo[i].vh.scn.measurement_unit[1]==0)
                        sprintf(meas_y,"km");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==1)
                        sprintf(meas_y,"m");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==2)
                        sprintf(meas_y,"cm");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==3)
                        sprintf(meas_y,"mm");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==4)
                        sprintf(meas_y,"micron");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==5)
                        sprintf(meas_y,"sec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==6)
                        sprintf(meas_y,"msec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[1]==7)
                        sprintf(meas_y,"microsec");

                sprintf(text,"Meas Unit   : %s X %s X",meas_x,meas_y);

            if(lib_infofinfo[i].vh.scn.dimension == 3 ||
                lib_infofinfo[i].vh.scn.dimension == 4)  
            {
                if(lib_infofinfo[i].vh.scn.measurement_unit[2]==0)
                        sprintf(meas_z,"km");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==1)
                        sprintf(meas_z,"m");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==2)
                        sprintf(meas_z,"cm");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==3)
                        sprintf(meas_z,"mm");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==4)
                        sprintf(meas_z,"micron");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==5)
                        sprintf(meas_z,"sec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==6)
                        sprintf(meas_z,"msec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[2]==7)
                        sprintf(meas_z,"microsec");

                sprintf(text2,"            : %s",meas_z);
            }
           
            if(lib_infofinfo[i].vh.scn.dimension == 4)  
            {
                if(lib_infofinfo[i].vh.scn.measurement_unit[3]==0)     
                        sprintf(meas_t,"km");  
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==1)
                        sprintf(meas_t,"m");   
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==2)
                        sprintf(meas_t,"cm");  
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==3)
                        sprintf(meas_t,"mm");  
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==4)
                        sprintf(meas_t,"micron");      
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==5)
                        sprintf(meas_t,"sec"); 
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==6)
                        sprintf(meas_t,"msec");
                else if(lib_infofinfo[i].vh.scn.measurement_unit[3]==7)
                        sprintf(meas_t,"microsec");    
                
                sprintf(text2,"            : %s X %s",meas_z,meas_t);
            }

        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        strncpy(txt,text2,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
        }

        /****Scene Size****/
        if(lib_infofinfo[i].vh.scn.xysize_valid == 1)
                sprintf(text,"Scene Size  : %d X %d X",
                        lib_infofinfo[i].vh.scn.xysize[0],
                        lib_infofinfo[i].vh.scn.xysize[1]);
        else    sprintf(text,"Scene Size  : Not Found");

        if(lib_infofinfo[i].vh.scn.dimension == 3)
                sprintf(text2,"            : %d",
                        lib_infofinfo[i].vh.scn.num_of_subscenes[0]);
        if(lib_infofinfo[i].vh.scn.dimension == 4)
        {
                first_volume=lib_infofinfo[i].vh.scn.num_of_subscenes[1];

                for(j=1;j<=lib_infofinfo[i].vh.scn.num_of_subscenes[0];j++)
                {
                  volume=lib_infofinfo[i].vh.scn.num_of_subscenes[j];
                  if(volume != first_volume) nonuniform_volume=TRUE; 
                }

                if(nonuniform_volume==TRUE)
                  sprintf(text2,"            : %d X *" , first_volume,
                        lib_infofinfo[i].vh.scn.num_of_subscenes[0]);
                else if(nonuniform_volume==FALSE)
                  sprintf(text2,"            : %d X %d", first_volume,
                        lib_infofinfo[i].vh.scn.num_of_subscenes[0]);
        }

        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        strncpy(txt,text2,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****Total Num of Slices****/
/*
        if(lib_infofinfo[i].vh.scn.dimension == 3)
                sprintf(text,"Tot Slices  : %d",
                        lib_infofinfo[i].vh.scn.num_of_subscenes[0]);
        if(lib_infofinfo[i].vh.scn.dimension == 4)
        {
                for(j=1;j<=lib_infofinfo[i].vh.scn.num_of_subscenes[0];j++)
                  total_slices+=lib_infofinfo[i].vh.scn.num_of_subscenes[j];
                
                sprintf(text,"Tot Slices  : %d", total_slices);
        }
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
*/
        /****Minimum density values****/
        if(lib_infofinfo[i].vh.scn.smallest_density_value_valid == 1)
                sprintf(text,"Min Density : %3.2f",
                        lib_infofinfo[i].vh.scn.smallest_density_value[0]);
        else    sprintf(text,"Min Density : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****Maximum density values****/
        if(lib_infofinfo[i].vh.scn.largest_density_value_valid == 1)
                sprintf(text,"Max Density : %3.2f",
                        lib_infofinfo[i].vh.scn.largest_density_value[0]);
        else    sprintf(text,"Max Density : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        /****Bits per cell****/
        if(lib_infofinfo[i].vh.scn.num_of_bits_valid == 1)
                sprintf(text,"Bits/Cell   : %d",
                        lib_infofinfo[i].vh.scn.num_of_bits);
        else    sprintf(text,"Bits/Cell   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        return(0);
}       


/************************************************************************
 *                                                                      *
 *      Function        : v_display_structure_information               *
 *      Description     : This function will display the structure      *
 *                        information for the selected file.            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the file for which the information   *
 *                              is to be displayed.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_display_structure_information ( int index )
{
        char text[100],text2[100],text3[100],txt[TEXT_LENGTH];
        char meas_x[10],meas_y[10],meas_z[10],meas_t[10];
        int xloc,yloc,i,j,k,error;
        float first_slice_spacing,slice_spacing;
        char char_first_slice_spacing[100],char_slice_spacing[100];
        float float_first_slice_spacing,float_slice_spacing;
        int nonuniform_slice_spacing=FALSE;
        float first_volume_spacing,volume_spacing;
        int nonuniform_volume_spacing=FALSE;
        int first_volume,volume,nonuniform_volume=FALSE;
        int mink,maxk,size;
        int total_slices=0;

        xloc = STRUCTUREINFO_XLOC+lib_information->font_width;
 
        i = index;

        strcpy(text,""); 
        /****cell size****/
        if(lib_infofinfo[i].vh.str.xysize_valid == 1)
                sprintf(text,"Cell Size   : %3.2f X %3.2f",
                        lib_infofinfo[i].vh.str.xysize[0],
                        lib_infofinfo[i].vh.str.xysize[1]);
        else    sprintf(text,"Cell Size   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc = STRUCTUREINFO_YLOC + lib_information->font_height*3;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****sample/volume spacing****/
        if(lib_infofinfo[i].vh.str.dimension == 3)
        {
                if(lib_infofinfo[i].vh.str.num_of_samples[0]==1)
                  sprintf(text,"            : Only One Slice");
                else {
                  first_slice_spacing=
                        lib_infofinfo[i].vh.str.loc_of_samples[1] -
                        lib_infofinfo[i].vh.str.loc_of_samples[0];
                  sprintf(char_first_slice_spacing,"%5.4f",
                                first_slice_spacing);
                  float_first_slice_spacing=(float)(atoi(char_first_slice_spacing));
                  for(j=1;j<lib_infofinfo[i].vh.str.num_of_samples[0];j++)
                  {
                     slice_spacing=lib_infofinfo[i].vh.str.loc_of_samples[j]
                        - lib_infofinfo[i].vh.str.loc_of_samples[j-1];
                     sprintf(char_slice_spacing,"%5.4f",slice_spacing);
                     float_slice_spacing=(float)(atoi(char_slice_spacing));
                     if(float_slice_spacing != float_first_slice_spacing)
                        nonuniform_slice_spacing=TRUE;
                  }
                  if(nonuniform_slice_spacing==TRUE)
                    sprintf(text,"            : *");
                  else sprintf(text,"            : %3.2f", slice_spacing);
                }
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
        }
 
        if(lib_infofinfo[i].vh.str.dimension == 4)
        {
                if(lib_infofinfo[i].vh.str.num_of_samples[0]==1)
                  sprintf(text,"*");
                else {
                  first_volume_spacing=
                        lib_infofinfo[i].vh.str.loc_of_samples[1] -
                        lib_infofinfo[i].vh.str.loc_of_samples[0];
                  for(j=0;j<lib_infofinfo[i].vh.str.num_of_samples[0];j++)
                  {
                     if(j+1 < lib_infofinfo[i].vh.scn.num_of_subscenes[0])
                     {
                        volume_spacing=
                          lib_infofinfo[i].vh.str.loc_of_samples[j+1] -
                          lib_infofinfo[i].vh.str.loc_of_samples[j];
                        if(volume_spacing != first_volume_spacing)
                          nonuniform_volume_spacing=TRUE;
                     }
                  }
                  if(nonuniform_volume_spacing==TRUE)
                    sprintf(text,"*");
                  else sprintf(text,"%3.2f", volume_spacing);
                }
                mink=lib_infofinfo[i].vh.str.num_of_samples[0];
                maxk=lib_infofinfo[i].vh.str.num_of_samples[0]+
                     lib_infofinfo[i].vh.str.num_of_samples[1];
                for(j=1;j<=lib_infofinfo[i].vh.str.num_of_samples[0];j++)
                {
                        if(lib_infofinfo[i].vh.str.num_of_samples[j] == 1)
                          sprintf(text2,"1 Slc");
                        else {
                          first_slice_spacing=
                            lib_infofinfo[i].vh.str.loc_of_samples[mink+1] -
                            lib_infofinfo[i].vh.str.loc_of_samples[mink];
                            for(k=mink;k<maxk;k++)
                            {
                                if(k+1 < maxk)
                                {
                                slice_spacing=
                                lib_infofinfo[i].vh.str.loc_of_samples[k+1]-
                                lib_infofinfo[i].vh.str.loc_of_samples[k];
                                if(slice_spacing != first_slice_spacing)
                                        nonuniform_slice_spacing=TRUE;
                                }
                            }
                            if(nonuniform_slice_spacing==TRUE)
                            sprintf(text2,"*");
                            else sprintf(text2,"%3.2f", slice_spacing);
                        }
                        mink=maxk;
                        maxk+=lib_infofinfo[i].vh.str.num_of_samples[j];
 
                 }
 
        sprintf(text3,"            : %s X %s",text2,text);
        strncpy(txt,text3,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        }

        /****Measurement Unit****/
                if(lib_infofinfo[i].vh.str.measurement_unit[0]==0)
                        sprintf(meas_x,"km");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==1)
                        sprintf(meas_x,"m");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==2)
                        sprintf(meas_x,"cm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==3)
                        sprintf(meas_x,"mm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==4)
                        sprintf(meas_x,"micron");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==5)
                        sprintf(meas_x,"sec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==6)
                        sprintf(meas_x,"msec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[0]==7)
                        sprintf(meas_x,"microsec");
 
                if(lib_infofinfo[i].vh.str.measurement_unit[1]==0)
                        sprintf(meas_y,"km");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==1)
                        sprintf(meas_y,"m");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==2)
                        sprintf(meas_y,"cm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==3)
                        sprintf(meas_y,"mm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==4)
                        sprintf(meas_y,"micron");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==5)
                        sprintf(meas_y,"sec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==6)
                        sprintf(meas_y,"msec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[1]==7)
                        sprintf(meas_y,"microsec");
 
                sprintf(text,"Meas Unit   : %s X %s X",meas_x,meas_y);
 
            if(lib_infofinfo[i].vh.str.dimension == 3 ||
                lib_infofinfo[i].vh.str.dimension == 4)
            {
                if(lib_infofinfo[i].vh.str.measurement_unit[2]==0)
                        sprintf(meas_z,"km");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==1)
                        sprintf(meas_z,"m");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==2)
                        sprintf(meas_z,"cm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==3)
                        sprintf(meas_z,"mm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==4)
                        sprintf(meas_z,"micron");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==5)
                        sprintf(meas_z,"sec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==6)
                        sprintf(meas_z,"msec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[2]==7)
                        sprintf(meas_z,"microsec");
 
                sprintf(text2,"            : %s",meas_z);
            }
 
            if(lib_infofinfo[i].vh.str.dimension == 4)
            {
                if(lib_infofinfo[i].vh.str.measurement_unit[3]==0)
                        sprintf(meas_t,"km");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==1)
                        sprintf(meas_t,"m");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==2)
                        sprintf(meas_t,"cm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==3)
                        sprintf(meas_t,"mm");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==4)
                        sprintf(meas_t,"micron");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==5)
                        sprintf(meas_t,"sec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==6)
                        sprintf(meas_t,"msec");
                else if(lib_infofinfo[i].vh.str.measurement_unit[3]==7)
                        sprintf(meas_t,"microsec");
 
                sprintf(text2,"            : %s X %s",meas_z,meas_t);
            }
 
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        strncpy(txt,text2,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        /****Structure Size****/
        size=0;
        for(j=0;j<lib_infofinfo[i].vh.str.num_of_structures;j++){
                if(lib_infofinfo[i].vh.str.num_of_TSE_valid==1)
                        size+=lib_infofinfo[i].vh.str.num_of_TSE[j];
        }

        sprintf(text,"Struc Size  : %d",size);
/*
        if(lib_infofinfo[i].vh.str.xysize_valid == 1)
                sprintf(text,"Struc Size  : %3.2f X %3.2f X",
                        lib_infofinfo[i].vh.str.xysize[0],
                        lib_infofinfo[i].vh.str.xysize[1]);
        else    sprintf(text,"Struc Size  : Not Found");
 
        if(lib_infofinfo[i].vh.str.dimension == 3)
                sprintf(text2,"            : %d",
                        lib_infofinfo[i].vh.str.num_of_samples[0]);
        if(lib_infofinfo[i].vh.str.dimension == 4)
        {
                first_volume=lib_infofinfo[i].vh.str.num_of_samples[1];
 
                for(j=1;j<=lib_infofinfo[i].vh.str.num_of_samples[0];j++)
                {
                  volume=lib_infofinfo[i].vh.str.num_of_samples[j];
                  if(volume != first_volume) nonuniform_volume=TRUE;
                }
 
                if(nonuniform_volume==TRUE)
                  sprintf(text2,"            : %d X *" , first_volume,
                        lib_infofinfo[i].vh.str.num_of_samples[0]);
                else if(nonuniform_volume==FALSE)
                  sprintf(text2,"            : %d X %d", first_volume,
                        lib_infofinfo[i].vh.str.num_of_samples[0]);
        }
*/ 
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
/*
        strncpy(txt,text2,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
*/

        /****Total Samples****/
/*
        if(lib_infofinfo[i].vh.str.dimension == 3)
                sprintf(text,"Tot Samples : %d",
                        lib_infofinfo[i].vh.str.num_of_samples[0]);
        if(lib_infofinfo[i].vh.str.dimension == 4)
        {
                for(j=1;j<=lib_infofinfo[i].vh.str.num_of_samples[0];j++)
                  total_slices+=lib_infofinfo[i].vh.str.num_of_samples[j];
 
                sprintf(text,"Tot Samples : %d", total_slices,
                        lib_infofinfo[i].vh.str.num_of_samples[0]);
        }
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc); 
*/
        /****Number of structures****/
        if(lib_infofinfo[i].vh.str.num_of_structures_valid == 1)
                sprintf(text,"Num Struc   : %d",
                        lib_infofinfo[i].vh.str.num_of_structures);
        else    sprintf(text,"Num Struc   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        /****Volume****/
        if(lib_infofinfo[i].vh.str.volume_valid == 1)
        {
             if(lib_infofinfo[i].vh.str.num_of_structures == 1)
             {
                sprintf(text,"Volume(1)   : %3.2f",
                        lib_infofinfo[i].vh.str.volume[0]);
                sprintf(text2,"");
                sprintf(text3,"");
             }
             else if(lib_infofinfo[i].vh.str.num_of_structures == 2)
             {
                sprintf(text,"Volume(1)   : %3.2f",
                        lib_infofinfo[i].vh.str.volume[0]);
                sprintf(text2,"Volume(2)   : %3.2f",
                        lib_infofinfo[i].vh.str.volume[1]);
                sprintf(text3,"");
             }
             else if(lib_infofinfo[i].vh.str.num_of_structures == 3)
             {
                sprintf(text,"Volume(1)   : %3.2f",
                        lib_infofinfo[i].vh.str.volume[0]);
                sprintf(text2,"Volume(2)   : %3.2f",
                        lib_infofinfo[i].vh.str.volume[1]);
                sprintf(text,"Volume(3)   : %3.2f",
                        lib_infofinfo[i].vh.str.volume[2]);
             }
        }
        else
        {    
                sprintf(text,"Volume      : Not Found");
                sprintf(text2,"");
                sprintf(text3,"");
        }
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
        strncpy(txt,text2,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
        strncpy(txt,text3,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        return(0);
}       


/************************************************************************
 *                                                                      *
 *      Function        : v_display_display_information                 *
 *      Description     : This function will display the display        *
 *                        information for the selected file.            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the file for which the information   *
 *                              is to be displayed.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayInformation.                          *
 *      History         : Written on April 10, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_display_display_information ( int index )
{
        char text[100],txt[TEXT_LENGTH];
        char meas_x[10],meas_y[10],meas_z[10],meas_t[10];
        int xloc,yloc,i,error;
 
        xloc = DISPLAYINFO_XLOC+lib_information->font_width;
 
        i = index;
 
        /****Cell size****/
        if(lib_infofinfo[i].vh.dsp.xypixsz_valid == 1)
                sprintf(text,"Cell Size   : %3.2f X %3.2f",
                        lib_infofinfo[i].vh.dsp.xypixsz[0],
                        lib_infofinfo[i].vh.dsp.xypixsz[1]);
        else    sprintf(text,"Cell Size   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc = DISPLAYINFO_YLOC + lib_information->font_height*3;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        /****Measurement Unit****/
                if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==0)
                        sprintf(meas_x,"km");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==1)
                        sprintf(meas_x,"m");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==2)
                        sprintf(meas_x,"cm");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==3)
                        sprintf(meas_x,"mm");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==4)
                        sprintf(meas_x,"micron");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==5)
                        sprintf(meas_x,"sec");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==6)
                        sprintf(meas_x,"msec");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[0]==7)
                        sprintf(meas_x,"microsec");
 
                if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==0)
                        sprintf(meas_y,"km");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==1)
                        sprintf(meas_y,"m");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==2)
                        sprintf(meas_y,"cm");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==3)
                        sprintf(meas_y,"mm");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==4)
                        sprintf(meas_y,"micron");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==5)
                        sprintf(meas_y,"sec");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==6)
                        sprintf(meas_y,"msec");
                else if(lib_infofinfo[i].vh.dsp.measurement_unit[1]==7)
                        sprintf(meas_y,"microsec");
 
                sprintf(text,"Meas Unit   : %s X %s",meas_x,meas_y);
 
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 

        /****Image Size****/
        if(lib_infofinfo[i].vh.dsp.xysize_valid == 1)
                sprintf(text,"Image Size  : %d X %d",
                        lib_infofinfo[i].vh.dsp.xysize[0],
                        lib_infofinfo[i].vh.dsp.xysize[1]);
        else    sprintf(text,"Image Size  : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****Num of Images****/
        if(lib_infofinfo[i].vh.dsp.num_of_images_valid == 1)
                sprintf(text,"Num Images  : %d",
                        lib_infofinfo[i].vh.dsp.num_of_images);
        else    sprintf(text,"Num Images  : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);

        /****Minimum density values****/
        if(lib_infofinfo[i].vh.dsp.smallest_value_valid == 1)
                sprintf(text,"Min Density : %3.2f",
                        lib_infofinfo[i].vh.dsp.smallest_value[0]);
        else    sprintf(text,"Min Density : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        /****Maximum density values****/
        if(lib_infofinfo[i].vh.dsp.largest_value_valid == 1)
                sprintf(text,"Max Density : %3.2f",
                        lib_infofinfo[i].vh.dsp.largest_value[0]);
        else    sprintf(text,"Max Density : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 
        /****Bits per cell****/
        if(lib_infofinfo[i].vh.dsp.num_of_bits_valid == 1)
                sprintf(text,"Bits/Cell   : %d",
                        lib_infofinfo[i].vh.dsp.num_of_bits);
        else    sprintf(text,"Bits/Cell   : Not Found");
        strncpy(txt,text,TEXT_LENGTH);
        txt[TEXT_LENGTH-1]=0;
        yloc += lib_information->font_height;
        error=VDisplayText(lib_information->win,lib_information->gc,
                        txt, xloc, yloc);
        if(error != 0) return(error);
 

        return(0);
}       

/***********************************************************************/
