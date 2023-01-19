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
 *      Filename  : help.c                                              *
 *      Ext Funcs : VDisplayHelp. 					*
 *      Int Funcs : v_fill_chapterlist, 				*
 *                                                                      *
 ************************************************************************/

#include "Vlibrary.h"
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "3dv.h"

#define TEXT_LENGTH 25
#define CHAPTERS 7
#define MAX_TOPICS 100

WindowGcInfo *lib_help;
Cursor helpmove_cursor;
int TOPIC_PAGE_LENGTH=0,HELP_PAGE_LENGTH=0;
int HELPCLICK_XLOC,HELPCLICK_YLOC,HELPCLICK_WIDTH,HELPCLICK_HEIGHT;
int CHAPTERBUTTON_BOX_XLOC,CHAPTERBUTTON_BOX_YLOC;
int CHAPTERBUTTON_BOX_WIDTH,CHAPTERBUTTON_BOX_HEIGHT;
int CHAPTERBUTTON_XLOC[CHAPTERS];
int CHAPTERBUTTON_YLOC,CHAPTERBUTTON_WIDTH,CHAPTERBUTTON_HEIGHT;
int TOPICDISPLAY_BOX_XLOC,TOPICDISPLAY_BOX_YLOC;
int TOPICDISPLAY_BOX_WIDTH,TOPICDISPLAY_BOX_HEIGHT;
int TOPICTOPCLICK_XLOC,TOPICTOPCLICK_YLOC;
int TOPICTOPCLICK_WIDTH,TOPICTOPCLICK_HEIGHT;
int TOPICBOTTOMCLICK_XLOC,TOPICBOTTOMCLICK_YLOC;
int TOPICBOTTOMCLICK_WIDTH,TOPICBOTTOMCLICK_HEIGHT;
int SUBTOPICTOPCLICK_XLOC,SUBTOPICTOPCLICK_YLOC;
int SUBTOPICTOPCLICK_WIDTH,SUBTOPICTOPCLICK_HEIGHT;
int SUBTOPICBOTTOMCLICK_XLOC,SUBTOPICBOTTOMCLICK_YLOC;
int SUBTOPICBOTTOMCLICK_WIDTH,SUBTOPICBOTTOMCLICK_HEIGHT;
int HELPDISPLAY_BOX_XLOC,HELPDISPLAY_BOX_YLOC;
int HELPDISPLAY_BOX_WIDTH,HELPDISPLAY_BOX_HEIGHT;
int HELPTOPCLICK_XLOC,HELPTOPCLICK_YLOC;
int HELPTOPCLICK_WIDTH,HELPTOPCLICK_HEIGHT;
int HELPBOTTOMCLICK_XLOC,HELPBOTTOMCLICK_YLOC;
int HELPBOTTOMCLICK_WIDTH,HELPBOTTOMCLICK_HEIGHT;
int QUITBUTTON_BOX_XLOC,QUITBUTTON_BOX_YLOC;
int QUITBUTTON_BOX_WIDTH,QUITBUTTON_BOX_HEIGHT;
int QUITBUTTON_YLOC,QUITBUTTON_WIDTH,QUITBUTTON_HEIGHT;
int QUITBUTTON1_XLOC,QUITBUTTON2_XLOC,QUITBUTTON3_XLOC;
int TOPIC_SCROLL_ACTIVATE_FLAG=FALSE;
int TOPIC_SCROLL_INDEX=0;
int SUBTOPIC_SCROLL_ACTIVATE_FLAG=FALSE;
int SUBTOPIC_SCROLL_INDEX=0;
int HELP_SCROLL_ACTIVATE_FLAG=FALSE;
int HELP_SCROLL_INDEX=0;
int HELPSCROLLBAR_WIDTH;
int HELP_SCROLLBAR=1; 
int HELP_NOSCROLLBAR=0;
int HELP_BUTTON=1; 
int HELP_WINDOW=0;
int CHAPTERINDEX=-1;
char HELPTEXT[6000][100],HELPTITLE[100];
int HELPLINES=0;
char CHAPTERLIST[CHAPTERS][20];
int HELPWINDOW_SELECTED=FALSE;

typedef struct {
        char topicname[100];
	short level;
	short flag;
        short xloc;
        short yloc;
} TopicList;
 
typedef struct {
	char chaptername[30];
	int num_of_topics;
	char oldtopic[MAX_TOPICS][100];
	short topiclevel[MAX_TOPICS];
	TopicList *topic;
} ChapterList;

ChapterList *lib_chapterlist[CHAPTERS];

static int v_allocate_topics ( int topic_number );
static int v_CheckEventsInHelpWindow ( XEvent* event );
static int v_clean_help_window ( void );
static int v_clean_topic_window ( void );
static int v_clean_windows ( void );
static int v_copy_topics_to_structure ( int topic_number );
static int v_destroy_help_window ( void );
static int v_display_help ( int scroll_index );
static int v_display_help_title ( void );
static int v_fill_chapterlist ( void );
static int v_free_help ( void );
static int v_get_helptext_fromfile ( char* topicname );
static int v_get_help_page_length ( void );
static int v_get_topic_list ( int topic_number );
static int v_get_topic_page_length ( void );
static int v_get_topics ( char* chaptername, int chapternumber );
static int v_highlight_chapter_button ( int index );
static int v_highlight_topictext ( int index );
static int v_initialize_flags ( int chapterindex );
static int v_list_topic ( int index, int scroll_index );
static int v_single_topic_check ( int index );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayHelp                           	*
 *      Description     : This function will display the help window 	*
 *                        and display the various buttons that can be	*
 *			  pressed to list information on different 	*
 *			  topics. There is a list of topics that can	*
 *			  selected which will list the help text in the	*
 *			  help area. The help text is stored in the	*
 *			  HELPFILE in the FILES directory.		*
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         284 - help window is currently displayed.    *
 *                         285 - info window is currently displayed.    *
 *                         286 - annotation mode is on.                 *
 *      Parameters      :  None.					*
 *      Side effects    : The related window is image window.           *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDoHelp,VCheckEventsInButtonWindow.    	*
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
int VDisplayHelp ( void )
{

	XEvent event;
        int error,quit,junk,root_width,root_height;

	int chapterbutton_box_x,chapterbutton_box_y;
        int chapterbutton_box_x2,chapterbutton_box_y2;
        int chapterbutton_box_width,chapterbutton_box_height;

	int chapterbutton1_x,chapterbutton2_x,chapterbutton3_x;
        int chapterbutton4_x,chapterbutton5_x,chapterbutton6_x;
	int chapterbutton7_x;
        int chapterbutton_y,chapterbutton_width,chapterbutton_height;

	int topic_display_box_x,topic_display_box_y;
	int topic_display_box_width,topic_display_box_height;
	int topic_display_box_x2,topic_display_box_y2;

	int help_display_box_x,help_display_box_y;
	int help_display_box_width,help_display_box_height;
	int help_display_box_x2,help_display_box_y2;

	int quitbutton_box_x,quitbutton_box_y;
	int quitbutton_box_width,quitbutton_box_height;
	int quitbutton_box_x2,quitbutton_box_y2;

	int quitbutton_y,quitbutton_width,quitbutton_height;
	int quitbutton1_x,quitbutton2_x;
	int quitbutton3_x,quitbutton4_x;

	MarginClickInfo click;
	ScrollbarClickInfo left_click,right_click;

	if(lib_cmap_created == 0) {
           printf("The error occurred in the function ");
           printf("VDisplayHelp.\n");
	   printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

	if(lib_help_display==TRUE) return(284);
	if(lib_information_display==TRUE) return(285);
	if(lib_annotation_display==TRUE) return(286);

	lib_help_display = TRUE;

	lib_help=(WindowGcInfo *)calloc(1,sizeof(WindowGcInfo));
	if (lib_help == NULL) return(1);

	lib_help->x=80; 
	lib_help->y=50; 
	lib_help->font_width=lib_wins[0].font_width;
	lib_help->font_height=lib_wins[0].font_height;
	if(lib_help->font_height > 20) lib_help->font_height=13;
	if(lib_help->font_width > 10) lib_help->font_width=10;
	lib_help->width=lib_help->font_width*100;
	lib_help->height=lib_help->font_width*100;

	HELPSCROLLBAR_WIDTH = lib_help->font_width*2;

	VGetGeometry(&junk,&junk,&root_width,&root_height);

	if((lib_help->x+lib_help->width >= root_width) || 
		(lib_help->y+lib_help->height >= root_height))
	{
		VDisplayDialogMessage("The window size is too small to display HELP window!");
		XBell(lib_display,0);
		return(0);
	}

	VRemoveMenu();
	VRemoveDialogWindow();
	VRemoveButtonWindow();

        error=VCreate3DImageSubwindow(&lib_help->win,
				lib_help->x,
				lib_help->y,
				lib_help->width,
				lib_help->height);
	if(error != 0) return(error);

	error=VGetWindowGC(lib_wins[1].win,&lib_help->gc);
	if(error != 0) return(error);

	error=VDisplayCaptionBar(lib_help->win,lib_help->gc,"HELP",
			0,0,lib_help->width,NULL,&click);
	if(error != 0) return(error);

	helpmove_cursor = XCreateFontCursor(lib_display,XC_crosshair);

	HELPCLICK_XLOC=click.x;
        HELPCLICK_YLOC=click.y;
        HELPCLICK_WIDTH=click.width;
        HELPCLICK_HEIGHT=click.height;

	XUngrabPointer(lib_display,CurrentTime);
        XGrabPointer(lib_display,lib_help->win,True,
                        PointerMotionMask,GrabModeAsync,GrabModeAsync,
                        lib_help->win,None,CurrentTime);

	/*********chapter button box***********/
	chapterbutton_box_x = lib_help->font_width*2;
	chapterbutton_box_y = lib_help->font_width*5;
	chapterbutton_box_width = lib_help->width - lib_help->font_width*4;
	chapterbutton_box_height = lib_help->font_width*5;
	chapterbutton_box_x2 = chapterbutton_box_x + chapterbutton_box_width;
	chapterbutton_box_y2 = chapterbutton_box_y + chapterbutton_box_height;

	CHAPTERBUTTON_BOX_XLOC = chapterbutton_box_x;
	CHAPTERBUTTON_BOX_YLOC = chapterbutton_box_y;
	CHAPTERBUTTON_BOX_WIDTH = chapterbutton_box_width;
	CHAPTERBUTTON_BOX_HEIGHT = chapterbutton_box_height;

	/*********chapter buttons***********/
	chapterbutton_width = (chapterbutton_box_width -
			(CHAPTERS+1)*(lib_help->font_width*2))/CHAPTERS;
	chapterbutton_height = lib_help->font_width*3;
	chapterbutton_y = chapterbutton_box_y+
                          (chapterbutton_box_height - chapterbutton_height)/2;
	chapterbutton1_x=chapterbutton_box_x + lib_help->font_width*2; 
	chapterbutton2_x=chapterbutton1_x+chapterbutton_width+
				lib_help->font_width*2; 
	chapterbutton3_x=chapterbutton2_x+chapterbutton_width+
                                lib_help->font_width*2;
	chapterbutton4_x=chapterbutton3_x+chapterbutton_width+
                                lib_help->font_width*2;
	chapterbutton5_x=chapterbutton4_x+chapterbutton_width+
                                lib_help->font_width*2;
	chapterbutton6_x=chapterbutton5_x+chapterbutton_width+
                                lib_help->font_width*2;
	chapterbutton7_x=chapterbutton6_x+chapterbutton_width+
                                lib_help->font_width*2;
	CHAPTERBUTTON_WIDTH = chapterbutton_width;
	CHAPTERBUTTON_HEIGHT = chapterbutton_height;
	CHAPTERBUTTON_YLOC = chapterbutton_y;
	CHAPTERBUTTON_XLOC[0] = chapterbutton1_x;	
	CHAPTERBUTTON_XLOC[1] = chapterbutton2_x;	
	CHAPTERBUTTON_XLOC[2] = chapterbutton3_x;	
	CHAPTERBUTTON_XLOC[3] = chapterbutton4_x;	
	CHAPTERBUTTON_XLOC[4] = chapterbutton5_x;	
	CHAPTERBUTTON_XLOC[5] = chapterbutton6_x;	
	CHAPTERBUTTON_XLOC[6] = chapterbutton7_x;	

	/*********topic display box*********/
	topic_display_box_x = chapterbutton_box_x;
	topic_display_box_y = chapterbutton_box_y2 + lib_help->font_width*2;
	topic_display_box_width = chapterbutton_box_width;
	topic_display_box_height = lib_help->font_width*33;
	topic_display_box_x2 = topic_display_box_x + topic_display_box_width;
	topic_display_box_y2 = topic_display_box_y + topic_display_box_height;

	TOPICDISPLAY_BOX_XLOC = topic_display_box_x;
	TOPICDISPLAY_BOX_YLOC = topic_display_box_y;
	TOPICDISPLAY_BOX_WIDTH = topic_display_box_width;
	TOPICDISPLAY_BOX_HEIGHT = topic_display_box_height;

	/*********help display box***********/
	help_display_box_x = chapterbutton_box_x;
	help_display_box_y = topic_display_box_y2 + lib_help->font_width*2;
	help_display_box_width = chapterbutton_box_width;
	help_display_box_height = lib_help->font_width*44;
	help_display_box_x2 = help_display_box_x + help_display_box_width;
 	help_display_box_y2 = help_display_box_y + help_display_box_height;

	HELPDISPLAY_BOX_XLOC = help_display_box_x;
	HELPDISPLAY_BOX_YLOC = help_display_box_y;
	HELPDISPLAY_BOX_WIDTH = help_display_box_width;
	HELPDISPLAY_BOX_HEIGHT = help_display_box_height;

	/*********quit button box***********/
	quitbutton_box_x =  chapterbutton_box_x;
	quitbutton_box_y =  help_display_box_y2 + lib_help->font_width*2;
	quitbutton_box_width = chapterbutton_box_width;
	quitbutton_box_height = chapterbutton_box_height;
	quitbutton_box_x2 = quitbutton_box_x + quitbutton_box_width;
	quitbutton_box_y2 = quitbutton_box_y + quitbutton_box_height;

	QUITBUTTON_BOX_XLOC = quitbutton_box_x;
	QUITBUTTON_BOX_YLOC = quitbutton_box_y;
	QUITBUTTON_BOX_WIDTH = quitbutton_box_width;
	QUITBUTTON_BOX_HEIGHT = quitbutton_box_height;

	/*********quit buttons***********/
	quitbutton_width = (quitbutton_box_width-lib_help->font_width*8)/3;
	quitbutton_height = lib_help->font_width*3;
	quitbutton1_x =  quitbutton_box_x+lib_help->font_width*2;
	quitbutton_y =  quitbutton_box_y+
				(quitbutton_box_height- quitbutton_height)/2;

	quitbutton2_x = quitbutton1_x + quitbutton_width +
				lib_help->font_width*2;
	quitbutton3_x = quitbutton2_x + quitbutton_width +
                                lib_help->font_width*2;
	quitbutton4_x = quitbutton3_x + quitbutton_width +
                                lib_help->font_width*2;

	QUITBUTTON_YLOC = quitbutton_y;
        QUITBUTTON_WIDTH = quitbutton_width;
        QUITBUTTON_HEIGHT = quitbutton_height;
 
        QUITBUTTON1_XLOC = quitbutton1_x;
        QUITBUTTON2_XLOC = quitbutton2_x;
        QUITBUTTON3_XLOC = quitbutton3_x;

	/*********chapter button box*********/
	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton_box_x,
			chapterbutton_box_y,
			chapterbutton_box_width,
			chapterbutton_box_height,
			HELP_WINDOW,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);

	/*********topic display box*********/

        error=VDisplayBox(lib_help->win,lib_help->gc,
			topic_display_box_x,
			topic_display_box_y,
			topic_display_box_width,
			topic_display_box_height,
			HELP_WINDOW,HELP_SCROLLBAR,
			HELPSCROLLBAR_WIDTH,
			&right_click);
	if(error != 0) return(error);

	TOPICTOPCLICK_XLOC = right_click.top_x;
        TOPICTOPCLICK_YLOC = right_click.top_y;
        TOPICTOPCLICK_WIDTH = right_click.top_width;
        TOPICTOPCLICK_HEIGHT = right_click.top_height;
        TOPICBOTTOMCLICK_XLOC = right_click.bottom_x;
        TOPICBOTTOMCLICK_YLOC = right_click.bottom_y;
        TOPICBOTTOMCLICK_WIDTH = right_click.bottom_width;
        TOPICBOTTOMCLICK_HEIGHT = right_click.bottom_height;

        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			"TOPICS",
			topic_display_box_x,
			topic_display_box_y,
			(topic_display_box_width - HELPSCROLLBAR_WIDTH),
                        lib_help->font_height*2);
	if(error != 0) return(error);

	/*********help display box*********/
	error=VDisplayBox(lib_help->win,lib_help->gc,
			help_display_box_x,
			help_display_box_y,
			help_display_box_width,
			help_display_box_height,
			HELP_WINDOW,HELP_SCROLLBAR,
			HELPSCROLLBAR_WIDTH,
			&right_click);
	if(error != 0) return(error);

	HELPTOPCLICK_XLOC = right_click.top_x;
	HELPTOPCLICK_YLOC = right_click.top_y;
	HELPTOPCLICK_WIDTH = right_click.top_width;
	HELPTOPCLICK_HEIGHT = right_click.top_height;
	HELPBOTTOMCLICK_XLOC = right_click.bottom_x;
	HELPBOTTOMCLICK_YLOC = right_click.bottom_y;
	HELPBOTTOMCLICK_WIDTH = right_click.bottom_width;
	HELPBOTTOMCLICK_HEIGHT = right_click.bottom_height;

 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			"HELP",
			help_display_box_x,
			help_display_box_y,
			help_display_box_width - HELPSCROLLBAR_WIDTH,
			lib_help->font_height*2);
	if(error != 0) return(error);
                                if(error != 0) return(error);

	/*********quitbutton display box*********/
	error=VDisplayBox(lib_help->win,lib_help->gc,
			quitbutton_box_x,
			quitbutton_box_y,
			quitbutton_box_width,
			quitbutton_box_height,
			HELP_WINDOW,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
	/*********chapter buttons*********/

	error=v_fill_chapterlist();
	    if(error != 0) return(error);

	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton1_x,
			chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height,
			HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);

	error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[0],
			chapterbutton1_x,
			chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height);
	if(error != 0) return(error);
	
	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton2_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[1],
			chapterbutton2_x,
                  	chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height);
	if(error != 0) return(error);


	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton3_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[2],
			chapterbutton3_x,
                  	chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height);
	if(error != 0) return(error);


	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton4_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[3],
			chapterbutton4_x,
                  	chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height);
	if(error != 0) return(error);


	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton5_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[4],
			chapterbutton5_x,
                  	chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height);
	if(error != 0) return(error);
	
	error=VDisplayBox(lib_help->win,lib_help->gc,
			chapterbutton6_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[5],
			chapterbutton6_x,
                  	chapterbutton_y,
			chapterbutton_width,
			chapterbutton_height);
	if(error != 0) return(error);

	error=VDisplayBox(lib_help->win,lib_help->gc,
                        chapterbutton7_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
                        CHAPTERLIST[6],
                        chapterbutton7_x,
                        chapterbutton_y,
                        chapterbutton_width,
                        chapterbutton_height);
        if(error != 0) return(error);

	/*********quit buttons*********/
        error=VDisplayBox(lib_help->win,lib_help->gc,
			quitbutton1_x,
                        quitbutton_y,
                        quitbutton_width,
                        quitbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
	if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			"DISMISS",
			quitbutton1_x,
                  	quitbutton_y,
			quitbutton_width,
			quitbutton_height);
	if(error != 0) return(error);

	error=VDisplayBox(lib_help->win,lib_help->gc,
                        quitbutton2_x,
                        quitbutton_y,
                        quitbutton_width,
                        quitbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
                        "PAGE UP",
                        quitbutton2_x,
                        quitbutton_y,
                        quitbutton_width,
                        quitbutton_height);
        if(error != 0) return(error);

	error=VDisplayBox(lib_help->win,lib_help->gc,
                        quitbutton3_x,
                        quitbutton_y,
                        quitbutton_width,
                        quitbutton_height,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
                        &right_click);
        if(error != 0) return(error);
 
        error=VDisplayCenteredText(lib_help->win,lib_help->gc,
                        "PAGE DOWN",
                        quitbutton3_x,
                        quitbutton_y,
                        quitbutton_width,
                        quitbutton_height);
        if(error != 0) return(error);


	/*******get the topic list and display first topic********/

	VSelectCursor(ALL_WINDOWS, XC_watch);
	CHAPTERINDEX = 0;
	error=v_get_topic_list(CHAPTERINDEX);
	     if(error != 0) return(error);
	error=v_highlight_chapter_button(CHAPTERINDEX);
             if(error != 0) return(error);
        TOPIC_SCROLL_INDEX=FALSE;
        error=v_list_topic(CHAPTERINDEX,TOPIC_SCROLL_INDEX);
             if(error != 0) return(error);
	VSelectCursor(ALL_WINDOWS, DEFAULT_CURSOR);

	/*********************************************************/

	v_get_topic_page_length();
	v_get_help_page_length();

	error=VSelectEvents(lib_help->win,ButtonPressMask|KeyPressMask|
                        EnterWindowMask|OwnerGrabButtonMask|
                        PointerMotionMask);
	if(error != 0) return(error);

        XSelectInput(lib_display,lib_help->win,
			ButtonPressMask|KeyPressMask|
                        ExposureMask|OwnerGrabButtonMask|
                        EnterWindowMask|PointerMotionMask);	

	quit = FALSE;
        while(!quit) {
                VNextEvent(&event);
		switch(event.type)
		{

		  case ButtonPress:
		        /* if(event.xbutton.button == 3) quit=TRUE; */
                  	if(event.xbutton.window!=lib_help->win)continue;
                  	if(event.xbutton.window==lib_help->win)
                	  	quit=v_CheckEventsInHelpWindow(&event);
			break;
		}
	}

	XWarpPointer(lib_display,None,lib_wins[2].win,0,0,0,0,
                LIB_HELP_BUTTON_XLOC+LIB_STATICBUTTON_WIDTH/2,           
                LIB_HELP_BUTTON_YLOC+LIB_PANEL_ITEM_HEIGHT/2);
	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_fill_chapterlist                            *
 *      Description     : This function will copy the chapter names into*
 *			  the CHAPTERLIST structure.			*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.						*
 *      Entry condition : None.						*
 *      Related funcs   : VDisplayHelp.					*
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_fill_chapterlist ( void )
{
	strcpy(CHAPTERLIST[0],"GENERAL");
	strcpy(CHAPTERLIST[1],"PORT_DATA");
	strcpy(CHAPTERLIST[2],"PREPROCESS");
	strcpy(CHAPTERLIST[3],"VISUALIZE");
	strcpy(CHAPTERLIST[4],"MANIPULATE");
	strcpy(CHAPTERLIST[5],"ANALYZE");
	strcpy(CHAPTERLIST[6],"MISC.");

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_get_topic_list                              *
 *      Description     : This function will find the number of topics	*
 *			  available for each chapter and put it into	*
 *			  the topics structure.				*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist,
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                        Modified 2/18/00 null lib_chapterlist entry
 *                           not dereferenced by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_get_topic_list ( int topic_number )
{
        int i,error;

        for(i=0;i<CHAPTERS;i++)
        if(lib_chapterlist[i] != NULL)
        {
           if(lib_chapterlist[i]->topic != NULL)
                free(lib_chapterlist[i]->topic);
           free(lib_chapterlist[i]);
		   lib_chapterlist[i] = NULL;
        }

	if(topic_number==0) {
		error=v_get_topics("#GENERAL#",0);
                if(error != 0) return(error);
	}
	else if(topic_number==1) {
		error=v_get_topics("#PORT_DATA#",1);
		if(error != 0) return(error);
	}
	else if(topic_number==2) {
		error=v_get_topics("#PREPROCESS#",2);
		if(error != 0) return(error);
	}
	else if(topic_number==3) {
		error=v_get_topics("#VISUALIZE#",3);
		if(error != 0) return(error);
	}
	else if(topic_number==4) {
		error=v_get_topics("#MANIPULATE#",4);
		if(error != 0) return(error);
	}
	else if(topic_number==5) {
		error=v_get_topics("#ANALYZE#",5);
		if(error != 0) return(error);
	}
	else if(topic_number==6) {
		error=v_get_topics("#MISCELLANEOUS#",6);
		if(error != 0) return(error);
	}

	error=v_allocate_topics(topic_number);
		if(error != 0) return(error);
	error=v_copy_topics_to_structure(topic_number);
		if(error != 0) return(error);
	
	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_allocate_topics                             *
 *      Description     : This function will alocate space for the topic*
 *			  list.						*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist[topic_number] must be allocated.
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_allocate_topics ( int topic_number )
{
	int i;
        i=topic_number;

	if((lib_chapterlist[i]->topic =
		(TopicList *)calloc(lib_chapterlist[i]->num_of_topics,sizeof(TopicList))) == NULL)
        {
                fprintf(stderr,"Couldn't alloc. memory for TopicList\n");
                exit(-1);
        }

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_get_topics                                  *
 *      Description     : This function will get the list of topics for *
 *			  each chapter.					*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_get_topics ( char* chaptername, int chapternumber )
{
	char *env, txt[100],txt2[100],helpfile[100];
	int topicnumber,topic_search,len;
	FILE *fp,*fp2;

	env=getenv("VIEWNIX_ENV");
	if(chapternumber==0)
          sprintf(helpfile,"%s/%s/HELPFILES/help_general",env,FILES_PATH);
	else if(chapternumber==1)
          sprintf(helpfile,"%s/%s/HELPFILES/help_portdata",env,FILES_PATH);
	else if(chapternumber==2)
          sprintf(helpfile,"%s/%s/HELPFILES/help_preprocess",env,FILES_PATH);
	else if(chapternumber==3)
          sprintf(helpfile,"%s/%s/HELPFILES/help_visualize",env,FILES_PATH);
	else if(chapternumber==4)
          sprintf(helpfile,"%s/%s/HELPFILES/help_manipulate",env,FILES_PATH);
	else if(chapternumber==5)
          sprintf(helpfile,"%s/%s/HELPFILES/help_analyze",env,FILES_PATH);
	else if(chapternumber==6)
          sprintf(helpfile,"%s/%s/HELPFILES/help_misc",env,FILES_PATH);

        topic_search = FALSE;
        fp = fopen(helpfile,"rb");

	lib_chapterlist[chapternumber]=
			(ChapterList *)calloc(1,sizeof(ChapterList));
        if (lib_chapterlist[chapternumber] == NULL) return(1);

	while((fscanf(fp,"%s",txt)) != EOF)
        {
           if(strcmp(txt,chaptername)==0)
           {
                strcpy(lib_chapterlist[chapternumber]->chaptername,txt);
                topicnumber = 0;
                while(topic_search==FALSE)
                {
                   fscanf(fp,"%s",txt2);
                   txt2[strlen(txt2)]=0;
                   if((strcmp(txt2,"#Topic")==0) || 
		      (strcmp(txt2,"#Topiclevel2")==0) ||
		      (strcmp(txt2,"#Topiclevel3")==0))
                   {
                        fp2=fp;
                       /* fseek(fp2,ftell(fp)-strlen(txt2),0);*/
                        fseek(fp2,ftell(fp)+1,0);
                        fgets(lib_chapterlist[chapternumber]->oldtopic[topicnumber],100,fp2);
		       	if(strcmp(txt2,"#Topic")==0)
		       		lib_chapterlist[chapternumber]->topiclevel[topicnumber]=1;
		       	else if(strcmp(txt2,"#Topiclevel2")==0)
				lib_chapterlist[chapternumber]->topiclevel[topicnumber]=2;
			else if(strcmp(txt2,"#Topiclevel3")==0)
                                lib_chapterlist[chapternumber]->topiclevel[topicnumber]=3;

			
                        topicnumber++;
                   }
                   if(strcmp(txt2,"#endchapter#")==0) topic_search=TRUE;
                }
                lib_chapterlist[chapternumber]->num_of_topics = topicnumber;
           }

	}
	fclose(fp);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_copy_topics_to_structure                    *
 *      Description     : This function will copy the list of topics	*
 *			  into the topics structure.			*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      : 
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist,
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_copy_topics_to_structure ( int topic_number )
{
	
	int chapter,i;

        chapter=topic_number;

	for(i=0;i<lib_chapterlist[chapter]->num_of_topics;i++)
	{
	   	strcpy(lib_chapterlist[chapter]->topic[i].topicname,
			lib_chapterlist[chapter]->oldtopic[i]);
		lib_chapterlist[chapter]->topic[i].level=
			lib_chapterlist[chapter]->topiclevel[i];
	}

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_CheckEventsInHelpWindow                     *
 *      Description     : This function will handle all events in the 	*
 *			  help window.					*
 *      Return Value    :  0 - work successfully.                       *
 *			   1 - exit.					*
 *      Parameters      :  event - the event to be processed.		*
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist,
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                        Modified 2/18/00 CHAPTERINDEX passed to
 *                           v_copy_topics_to_structure by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_CheckEventsInHelpWindow ( XEvent* event )
{
	XEvent new_event;
	int curx,cury;
	int cur_xloc1,cur_xloc2,cur_yloc1,cur_yloc2;
	int i,error;

	if(event->type == ButtonPress &&
	    (event->xbutton.button == MIDDLE_BUTTON || 
	     event->xbutton.button == LEFT_BUTTON))
        {
          curx=event->xbutton.x;cury=event->xbutton.y;

	  /*******quitting the help window*******/
	  if(curx>HELPCLICK_XLOC && curx<HELPCLICK_XLOC+HELPCLICK_WIDTH &&
	      cury>HELPCLICK_YLOC && cury<HELPCLICK_YLOC+HELPCLICK_HEIGHT)
	  {
		error=v_destroy_help_window();
			if(error != 0) return(error);
		error=v_free_help();
			if(error != 0) return(error);
		if((lib_dialogonoff_cmds != NULL &&
                        lib_dialogonoff_switches != NULL &&
			lib_dialogonoff_cmds[0].switch_item_displayed == 0) ||
			(lib_dialogonoff_cmds==NULL && 
			lib_dialogonoff_switches==NULL))
			VRedisplayDialogWindow();
		error=VRedisplayButtonWindow();
                        if(error != 0) return(error);
		return(TRUE);
	  }

	  if(curx>QUITBUTTON1_XLOC && 
	      curx<QUITBUTTON1_XLOC+QUITBUTTON_WIDTH &&
 	      cury>QUITBUTTON_YLOC && cury<QUITBUTTON_YLOC+QUITBUTTON_HEIGHT)
	  {
		error=VBlinkBox(lib_help->win,lib_help->gc,
			QUITBUTTON1_XLOC,QUITBUTTON_YLOC,
			QUITBUTTON_WIDTH,QUITBUTTON_HEIGHT);
		   	if(error != 0) return(error);
		error=v_destroy_help_window();
			if(error != 0) return(error);
		error=v_free_help();
			if(error != 0) return(error);
		if((lib_dialogonoff_cmds != NULL &&
                        lib_dialogonoff_switches != NULL &&
                        lib_dialogonoff_cmds[0].switch_item_displayed == 0) ||
			(lib_dialogonoff_cmds==NULL && 
                        lib_dialogonoff_switches==NULL))
                        VRedisplayDialogWindow();
		error=VRedisplayButtonWindow();
                        if(error != 0) return(error);
	    	return(TRUE);
	  }
	
	  /************refreshing the help window*********/
/*
	  if(curx>QUITBUTTON2_XLOC &&
	      curx<QUITBUTTON2_XLOC+QUITBUTTON_WIDTH &&
	      cury>QUITBUTTON_YLOC && cury<QUITBUTTON_YLOC+QUITBUTTON_HEIGHT)
          {
		error=v_clean_windows();
			if(error != 0) return(error);
		error=v_initialize_flags(CHAPTERINDEX);
			if(error != 0) return(error);
		TOPIC_SCROLL_INDEX=FALSE;
		SUBTOPIC_SCROLL_INDEX=FALSE;
		HELP_SCROLL_INDEX=FALSE;
		CHAPTERINDEX=-1;
		error=v_highlight_chapter_button(CHAPTERINDEX);
			if(error != 0) return(error);
		error=VBlinkBox(lib_help->win,lib_help->gc,
			QUITBUTTON2_XLOC,QUITBUTTON_YLOC,
			QUITBUTTON_WIDTH,QUITBUTTON_HEIGHT);
			if(error != 0) return(error);
	  }
*/

	  /*********next page option*********/
	  if(curx>QUITBUTTON2_XLOC &&
                curx<QUITBUTTON2_XLOC+QUITBUTTON_WIDTH &&
                cury>QUITBUTTON_YLOC &&
                cury<QUITBUTTON_YLOC+QUITBUTTON_HEIGHT)
          {
                if(HELP_SCROLL_ACTIVATE_FLAG == TRUE) {
                        HELP_SCROLL_INDEX+=HELP_PAGE_LENGTH;
			if(HELP_SCROLL_INDEX > HELPLINES-1) {
                           	HELP_SCROLL_INDEX-=HELP_PAGE_LENGTH;
                                XBell(lib_display,0);
                        }
                        v_display_help(HELP_SCROLL_INDEX);
                }

		VBlinkBox(lib_help->win,lib_help->gc,
                        QUITBUTTON2_XLOC,QUITBUTTON_YLOC,
                        QUITBUTTON_WIDTH,QUITBUTTON_HEIGHT);

                return(FALSE);
          }

	  /*********previous page option*********/
	  if(curx>QUITBUTTON3_XLOC &&
                curx<QUITBUTTON3_XLOC+QUITBUTTON_WIDTH &&
                cury>QUITBUTTON_YLOC &&
                cury<QUITBUTTON_YLOC+QUITBUTTON_HEIGHT)
          {
                if(HELP_SCROLL_ACTIVATE_FLAG == TRUE) {
                        HELP_SCROLL_INDEX-=HELP_PAGE_LENGTH;
			if(HELP_SCROLL_INDEX < -1){
                                HELP_SCROLL_INDEX=0;
                                XBell(lib_display,0);
                        }
                        v_display_help(HELP_SCROLL_INDEX);
                }
 
		VBlinkBox(lib_help->win,lib_help->gc,
                        QUITBUTTON3_XLOC,QUITBUTTON_YLOC,
                        QUITBUTTON_WIDTH,QUITBUTTON_HEIGHT);

                return(FALSE);
          }

	  /*********checking the chapter buttons*********/
	  for(i=0;i<CHAPTERS;i++)
	  {
		cur_xloc1 = CHAPTERBUTTON_XLOC[i];
		cur_xloc2 = CHAPTERBUTTON_XLOC[i] + CHAPTERBUTTON_WIDTH;
		cur_yloc1 = CHAPTERBUTTON_YLOC;
		cur_yloc2 = CHAPTERBUTTON_YLOC + CHAPTERBUTTON_HEIGHT;
	 	if(curx>cur_xloc1 && curx<cur_xloc2 &&
		   cury>cur_yloc1 && cury<cur_yloc2)
		{
			error=v_clean_windows();
				if(error != 0) return(error);
                	CHAPTERINDEX = i;
			error=v_get_topic_list(CHAPTERINDEX);
                                if(error != 0) return(error);
                	error=v_initialize_flags(CHAPTERINDEX);
				if(error != 0) return(error);
			error=v_highlight_chapter_button(CHAPTERINDEX);
				if(error != 0) return(error);
			TOPIC_SCROLL_INDEX=FALSE;
                	error=v_list_topic(CHAPTERINDEX,TOPIC_SCROLL_INDEX);
				if(error != 0) return(error);
		}
	  }

	  /*********checking the topics list***********/
	  if(CHAPTERINDEX != -1) 
	  {
	    for(i=0+TOPIC_SCROLL_INDEX;
			i<lib_chapterlist[CHAPTERINDEX]->num_of_topics;i++)
            {
		cur_xloc1 = lib_chapterlist[CHAPTERINDEX]->topic[i].xloc;
                cur_xloc2 = TOPICDISPLAY_BOX_XLOC+TOPICDISPLAY_BOX_WIDTH/2;
                cur_yloc1 = lib_chapterlist[CHAPTERINDEX]->topic[i].yloc -
				lib_help->font_height;
                cur_yloc2 = cur_yloc1 + lib_help->font_height;
		
		if(curx>cur_xloc1 && curx<cur_xloc2 &&
                   cury>cur_yloc1 && cury<cur_yloc2)
                {
                     error=v_single_topic_check(CHAPTERINDEX);
			if(error != 0) return(error);
		     lib_chapterlist[CHAPTERINDEX]->topic[i].flag=1;
                     error=v_highlight_topictext(i);
			if(error != 0) return(error);
		     error=v_allocate_topics(CHAPTERINDEX);
			if(error != 0) return(error);
		     error=v_copy_topics_to_structure(CHAPTERINDEX);
			if(error != 0) return(error);
		     strcpy(HELPTITLE,lib_chapterlist[CHAPTERINDEX]->topic[i].topicname);
                     error=v_get_helptext_fromfile(HELPTITLE);
                        if(error != 0) return(error);
		     HELP_SCROLL_INDEX=FALSE;
		     error=v_display_help(HELP_SCROLL_INDEX);
			if(error != 0) return(error);
                     XFlush(lib_display);
                }
 
            }
	  }

	  /*********testing topic (left-top) scroll buttons***********/
	  if(curx>TOPICTOPCLICK_XLOC && 
		curx<TOPICTOPCLICK_XLOC+TOPICTOPCLICK_WIDTH &&
		cury>TOPICTOPCLICK_YLOC &&
		cury<TOPICTOPCLICK_YLOC+TOPICTOPCLICK_HEIGHT)
	  {
                error=VDisplayXORBox(lib_help->win,lib_help->gc,
                                          TOPICTOPCLICK_XLOC,
					  TOPICTOPCLICK_YLOC,
					  TOPICTOPCLICK_WIDTH,
					  TOPICTOPCLICK_HEIGHT);
                if(error != 0) return(error);

		if(TOPIC_SCROLL_ACTIVATE_FLAG == TRUE) {
                        --TOPIC_SCROLL_INDEX;
                        if(TOPIC_SCROLL_INDEX == -1){
                                TOPIC_SCROLL_INDEX=0;
                                XBell(lib_display,0);
                        }
                        v_list_topic(CHAPTERINDEX,TOPIC_SCROLL_INDEX);
 
                }
		
		error=VDisplayXORBox(lib_help->win,lib_help->gc,
                                          TOPICTOPCLICK_XLOC,
                                          TOPICTOPCLICK_YLOC,
                                          TOPICTOPCLICK_WIDTH,
                                          TOPICTOPCLICK_HEIGHT);
                if(error != 0) return(error);

		return(FALSE);
	  }

	  if(curx>TOPICBOTTOMCLICK_XLOC &&
                curx<TOPICBOTTOMCLICK_XLOC+TOPICBOTTOMCLICK_WIDTH &&
                cury>TOPICBOTTOMCLICK_YLOC &&
                cury<TOPICBOTTOMCLICK_YLOC+TOPICBOTTOMCLICK_HEIGHT)
          {
                error=VDisplayXORBox(lib_help->win,lib_help->gc,
					  TOPICBOTTOMCLICK_XLOC,
					  TOPICBOTTOMCLICK_YLOC,
					  TOPICBOTTOMCLICK_WIDTH,
				   	  TOPICBOTTOMCLICK_HEIGHT);
                if(error != 0) return(error);

		if(TOPIC_SCROLL_ACTIVATE_FLAG == TRUE) {
                        ++TOPIC_SCROLL_INDEX;
                        if(TOPIC_SCROLL_INDEX >
                           lib_chapterlist[CHAPTERINDEX]->num_of_topics-1)
                        {
                            TOPIC_SCROLL_INDEX=
                            lib_chapterlist[CHAPTERINDEX]->num_of_topics-1;
                            XBell(lib_display,0);
                        }
                        v_list_topic(CHAPTERINDEX,TOPIC_SCROLL_INDEX);
                }

		error=VDisplayXORBox(lib_help->win,lib_help->gc,
                                          TOPICBOTTOMCLICK_XLOC,
                                          TOPICBOTTOMCLICK_YLOC,
                                          TOPICBOTTOMCLICK_WIDTH,
                                          TOPICBOTTOMCLICK_HEIGHT);
                if(error != 0) return(error);

	  	return(FALSE);
	  }

	  /*********testing subtopic (right) scroll buttons***********/
	  if(curx>SUBTOPICTOPCLICK_XLOC &&
                curx<SUBTOPICTOPCLICK_XLOC+SUBTOPICTOPCLICK_WIDTH &&
                cury>SUBTOPICTOPCLICK_YLOC &&
                cury<SUBTOPICTOPCLICK_YLOC+SUBTOPICTOPCLICK_HEIGHT)
          {
                return(FALSE);
          }

	  /*********testing help (right-top) scroll buttons***********/
          if(curx>HELPTOPCLICK_XLOC &&
                curx<HELPTOPCLICK_XLOC+HELPTOPCLICK_WIDTH &&
                cury>HELPTOPCLICK_YLOC &&
                cury<HELPTOPCLICK_YLOC+HELPTOPCLICK_HEIGHT)
          {
		error=VDisplayXORBox(lib_help->win,lib_help->gc,
					  HELPTOPCLICK_XLOC,
					  HELPTOPCLICK_YLOC,
					  HELPTOPCLICK_WIDTH,
					  HELPTOPCLICK_HEIGHT);
                if(error != 0) return(error);

               	if(HELP_SCROLL_ACTIVATE_FLAG == TRUE) {
                        --HELP_SCROLL_INDEX;
                        if(HELP_SCROLL_INDEX == -1){
                                HELP_SCROLL_INDEX=0;
                                XBell(lib_display,0);
                        }
                        v_display_help(HELP_SCROLL_INDEX);
                }
		
		error=VDisplayXORBox(lib_help->win,lib_help->gc,
                                          HELPTOPCLICK_XLOC,
                                          HELPTOPCLICK_YLOC,
                                          HELPTOPCLICK_WIDTH,
                                          HELPTOPCLICK_HEIGHT);
                if(error != 0) return(error);
 
                return(FALSE);
          }
	  
	  /*********testing help (right-bottom) scroll buttons***********/
	  if(curx>HELPBOTTOMCLICK_XLOC &&
                curx<HELPBOTTOMCLICK_XLOC+HELPBOTTOMCLICK_WIDTH &&
                cury>HELPBOTTOMCLICK_YLOC &&
                cury<HELPBOTTOMCLICK_YLOC+HELPBOTTOMCLICK_HEIGHT)
          {
		error=VDisplayXORBox(lib_help->win,lib_help->gc,
					  HELPBOTTOMCLICK_XLOC,
					  HELPBOTTOMCLICK_YLOC,
					  HELPBOTTOMCLICK_WIDTH,
					  HELPBOTTOMCLICK_HEIGHT);
                if(error != 0) return(error);

		if(HELP_SCROLL_ACTIVATE_FLAG == TRUE) {
                        ++HELP_SCROLL_INDEX;
                        if(HELP_SCROLL_INDEX > HELPLINES-1) {
                                HELP_SCROLL_INDEX=HELPLINES-1;
                                XBell(lib_display,0);
                        }
                        v_display_help(HELP_SCROLL_INDEX);
                }
 
		error=VDisplayXORBox(lib_help->win,lib_help->gc,
                                          HELPBOTTOMCLICK_XLOC,
                                          HELPBOTTOMCLICK_YLOC,
                                          HELPBOTTOMCLICK_WIDTH,
                                          HELPBOTTOMCLICK_HEIGHT);
                if(error != 0) return(error);

                return(FALSE);
          }

	return(FALSE);

	}

	return(FALSE);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_free_help                                   *
 *      Description     : This function will free all the data 		*
 *			  structures used in the help module.		*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist, lib_help
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                        Modified 2/18/00 null lib_chapterlist entry
 *                           not dereferenced by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_free_help ( void )
{
	int i;

	for(i=0;i<CHAPTERS;i++)
	   if (lib_chapterlist[i])
	   {
	      free(lib_chapterlist[i]->topic);
	      free(lib_chapterlist[i]);
		   lib_chapterlist[i] = NULL;
	   }

	free(lib_help);

	lib_help_display=0;

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_destroy_help_window                         *
 *      Description     : This function will destroy the help window.	*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_destroy_help_window ( void )
{
	int center_x,center_y;
	int error;

	center_x = lib_help->x+lib_help->width/2;
        center_y = lib_help->y+lib_help->height/2;

	XSetForeground(lib_display,lib_help->gc,
                        lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,lib_help->win,lib_help->gc,
                        lib_help->x,lib_help->y,
                        center_x,center_y);
        XDrawLine(lib_display,lib_help->win,lib_help->gc,
                        lib_help->x+lib_help->width,
                        lib_help->y,center_x,center_y);
        XDrawLine(lib_display,lib_help->win,lib_help->gc,
                        lib_help->x,
                        lib_help->y+lib_help->height,
                        center_x,center_y);
        XDrawLine(lib_display,lib_help->win,lib_help->gc,
                        lib_help->x+lib_help->width,
                        lib_help->y+lib_help->height,
                        center_x,center_y);
        XFlush(lib_display);
 
        error=VDeleteImageSubwindow(lib_help->win);
		if(error != 0) return(error);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_highlight_chapter_button                    *
 *      Description     : This function will highlight the chapter	*
 *			  button, when it is pressed.			*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the chapter chosen.			*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_highlight_chapter_button ( int index )
{

	ScrollbarClickInfo right_click;
	int i,junk,error;

	for(i=0;i<CHAPTERS;i++)
	{
		error=VDisplayBox(lib_help->win,lib_help->gc,
			CHAPTERBUTTON_XLOC[i],
                        CHAPTERBUTTON_YLOC,
                        CHAPTERBUTTON_WIDTH,
                        CHAPTERBUTTON_HEIGHT,
                        HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
		if(error != 0) return(error);

        	error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[i],
			CHAPTERBUTTON_XLOC[i],
			CHAPTERBUTTON_YLOC,
			CHAPTERBUTTON_WIDTH,
			CHAPTERBUTTON_HEIGHT);
		if(error != 0) return(error);
	}

	if(index >= 0 && index < CHAPTERS)
	{
		error=VDisplayFixedColorBox(
			lib_help->win,lib_help->gc,
			CHAPTERBUTTON_XLOC[index],
			CHAPTERBUTTON_YLOC,
			CHAPTERBUTTON_WIDTH,
			CHAPTERBUTTON_HEIGHT,
			HELP_BUTTON,HELP_NOSCROLLBAR,NULL,
			&right_click);
		if(error != 0) return(error);

		error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			CHAPTERLIST[index],
			CHAPTERBUTTON_XLOC[index],
			CHAPTERBUTTON_YLOC,	
			CHAPTERBUTTON_WIDTH,
			CHAPTERBUTTON_HEIGHT);
		if(error != 0) return(error);
	}

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_initialize_flags                            *
 *      Description     : This function will initialize all the flags	*
 *			  in the topiclist data structure.		*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist[chapterindex]
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_initialize_flags ( int chapterindex )
{
	int i,j;
	i = chapterindex;

	  for(j=0;j<lib_chapterlist[i]->num_of_topics;j++)
	    lib_chapterlist[i]->topic[j].flag=0;

	return(0);
}
	

/************************************************************************
 *                                                                      *
 *      Function        : v_single_topic_check                          *
 *      Description     : This function will make sure that only one 	*
 *			  topic	is selected at any time.	  	*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the chapter chosen.			*
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist[index],
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_single_topic_check ( int index )
{
	int i,error,xloc1,xloc2,yloc1,yloc2;

	for(i=0;i<lib_chapterlist[index]->num_of_topics;i++){
            if(lib_chapterlist[index]->topic[i].flag == 1) {
                xloc1 = lib_chapterlist[index]->topic[i].xloc;
		xloc2 = TOPICDISPLAY_BOX_XLOC+TOPICDISPLAY_BOX_WIDTH/2;
                yloc1 = lib_chapterlist[index]->topic[i].yloc;
                yloc2 = yloc1 + lib_help->font_height;
 
		if(lib_chapterlist[index]->topic[i].level==1) {
                        error=VDisplayText(lib_help->win,lib_help->gc,
                                lib_chapterlist[index]->topic[i].topicname,
				xloc1,yloc1);
                        if(error != 0) return(error);
                  }
                  else if(lib_chapterlist[index]->topic[i].level==2) {
                        error=VDisplayText2(lib_help->win,lib_help->gc,
				lib_chapterlist[index]->topic[i].topicname,
                                xloc1,yloc1);
                        if(error != 0) return(error);
                  }
                  else if(lib_chapterlist[index]->topic[i].level==3) {
                        error=VDisplayText3(lib_help->win,lib_help->gc,
				lib_chapterlist[index]->topic[i].topicname,
                                xloc1,yloc1);
                        if(error != 0) return(error);
                  }

		XFlush(lib_display);
 
		lib_chapterlist[index]->topic[i].flag = 0;
             }
        }
 
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_clean_windows                               *
 *      Description     : This function will clean the windows of any	*
 *			  text.						*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.					*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_clean_windows ( void )
{
	int error;

	error=v_clean_topic_window();
		if(error != 0) return(error);
	error=v_clean_help_window();
		if(error != 0) return(error);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_clean_topic_window                          *
 *      Description     : This function will clean the topic window.	*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_clean_topic_window ( void )
{
	/****clear the topic display window****/
        XSetForeground(lib_display,lib_help->gc,
                       lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_help->win,lib_help->gc,
			TOPICDISPLAY_BOX_XLOC + HELPSCROLLBAR_WIDTH +
				lib_help->font_width,
			TOPICDISPLAY_BOX_YLOC + lib_help->font_height*2,
			TOPICDISPLAY_BOX_WIDTH - HELPSCROLLBAR_WIDTH -
				lib_help->font_width*3,
		(int)(TOPICDISPLAY_BOX_HEIGHT - lib_help->font_height*2.5));
 
	XFlush(lib_display);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_clean_subtopic_window                       *
 *      Description     : This function will clean the subtopic window. *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_clean_subtopic_window ( void )
{
        /****clear the topic display window****/
        XSetForeground(lib_display,lib_help->gc,
                       lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_help->win,lib_help->gc,
                        TOPICDISPLAY_BOX_XLOC + HELPSCROLLBAR_WIDTH +
                                lib_help->font_width,
                        TOPICDISPLAY_BOX_YLOC + lib_help->font_height*2,
                        TOPICDISPLAY_BOX_WIDTH - HELPSCROLLBAR_WIDTH -
                                lib_help->font_width*3,
                (int)(TOPICDISPLAY_BOX_HEIGHT - lib_help->font_height*2.5));
 
        XFlush(lib_display);
 
        return(0);
}
 

/************************************************************************
 *                                                                      *
 *      Function        : v_clean_help_window                           *
 *      Description     : This function will clean the help window. 	*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_clean_help_window ( void )
{
        /****clear the help display window****/
	XSetForeground(lib_display,lib_help->gc,
                       lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_help->win,lib_help->gc,
                        HELPDISPLAY_BOX_XLOC + lib_help->font_width,
                        HELPDISPLAY_BOX_YLOC + lib_help->font_height*2,
                        HELPDISPLAY_BOX_WIDTH - lib_help->font_width*4,
                 (int)(HELPDISPLAY_BOX_HEIGHT - lib_help->font_height*2.5));

	XFlush(lib_display);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_list_topic                                  *
 *      Description     : This function will list the topics for a	*
 *			  particular chapter selected.			*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the chapter selected.		*
 *			   scroll_index - the index where the chapter	*
 *			  	is currently selected on the window.	*
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist[index],
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_list_topic ( int index, int scroll_index )
{
	int i,yloc,xloc;
	int error;

	error=v_clean_topic_window();
        if(error != 0) return(error);

	xloc = TOPICDISPLAY_BOX_XLOC+HELPSCROLLBAR_WIDTH+
			lib_help->font_width*2;

	for(i=0;i<lib_chapterlist[index]->num_of_topics;i++)
	{
		lib_chapterlist[index]->topic[i].xloc = -1;
                lib_chapterlist[index]->topic[i].yloc = -1;
	}

	for(i=0+scroll_index;i<lib_chapterlist[index]->num_of_topics;i++)
	{
		yloc=TOPICDISPLAY_BOX_YLOC+
				(i-scroll_index+3)*(lib_help->font_height);
		if(yloc<TOPICDISPLAY_BOX_YLOC+
				TOPICDISPLAY_BOX_HEIGHT-lib_help->font_width)
		{
		  TOPIC_SCROLL_ACTIVATE_FLAG=FALSE;
		  if(i==lib_chapterlist[index]->num_of_topics-1 && 
					i>TOPIC_PAGE_LENGTH)
				TOPIC_SCROLL_ACTIVATE_FLAG=TRUE;
		  if(lib_chapterlist[index]->topic[i].level==1) {
                        xloc=TOPICDISPLAY_BOX_XLOC+HELPSCROLLBAR_WIDTH+
                                lib_help->font_width*2;
		   	error=VDisplayText(lib_help->win,lib_help->gc,
                        	lib_chapterlist[index]->oldtopic[i],
                        	xloc,yloc);
                  	if(error != 0) return(error);
		  }
		  else if(lib_chapterlist[index]->topic[i].level==2) {
			xloc=TOPICDISPLAY_BOX_XLOC+HELPSCROLLBAR_WIDTH+
				lib_help->font_width*4;
			error=VDisplayText2(lib_help->win,lib_help->gc,
                                lib_chapterlist[index]->oldtopic[i],
                                xloc,yloc);
                        if(error != 0) return(error);
		  }
		  else if(lib_chapterlist[index]->topic[i].level==3) {
			xloc=TOPICDISPLAY_BOX_XLOC+HELPSCROLLBAR_WIDTH+
				lib_help->font_width*6;
			error=VDisplayText3(lib_help->win,lib_help->gc,
                                lib_chapterlist[index]->oldtopic[i],
                                xloc,yloc);
                        if(error != 0) return(error);
		  }
        	  XFlush(lib_display);
		  lib_chapterlist[index]->topic[i].xloc = xloc;
		  lib_chapterlist[index]->topic[i].yloc = yloc;

		  if(lib_chapterlist[index]->topic[i].flag==1)
			v_highlight_topictext(i);
		}
		else {TOPIC_SCROLL_ACTIVATE_FLAG=TRUE;}
	}

	return(0);
} 


/************************************************************************
 *                                                                      *
 *      Function        : v_highlight_topictext                         *
 *      Description     : This function will highlight the topic 	*
 *			  selected by the user.				*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  index - the chapter selected.		*
 *      Side effects    : None.                                         *
 *      Entry condition : lib_chapterlist,
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_highlight_topictext ( int index )
{
	int xloc,yloc,error;

	xloc = lib_chapterlist[CHAPTERINDEX]->topic[index].xloc;
	yloc = lib_chapterlist[CHAPTERINDEX]->topic[index].yloc; 

	error=VDisplayFixedColorText(lib_help->win,lib_help->gc,
		lib_chapterlist[CHAPTERINDEX]->topic[index].topicname,
		xloc,yloc);
	if(error != 0) return(error);
	XFlush(lib_display);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_get_helptext_fromfile                       *
 *      Description     : This function will get the help text for the	*
 *			  corresponding topic selected from the HELPFILE*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  topicname - the topic selected.		*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_get_helptext_fromfile ( char* topicname )
{
	char *env, txt[100],txt2[100],txt3[100],helpfile[100];
        int help_search,helplines=0;
        FILE *fp,*fp2;

	HELPLINES = 0; 
        env=getenv("VIEWNIX_ENV");
	if(CHAPTERINDEX == 0)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_general",env,FILES_PATH);
	else if(CHAPTERINDEX == 1)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_portdata",env,FILES_PATH);
	else if(CHAPTERINDEX == 2)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_preprocess",env,FILES_PATH);
	else if(CHAPTERINDEX == 3)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_visualize",env,FILES_PATH);
	else if(CHAPTERINDEX == 4)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_manipulate",env,FILES_PATH);
	else if(CHAPTERINDEX == 5)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_analyze",env,FILES_PATH);
	else if(CHAPTERINDEX == 6)	
           sprintf(helpfile,"%s/%s/HELPFILES/help_misc",env,FILES_PATH);

        help_search = FALSE;
        fp = fopen(helpfile,"rb");

	topicname[strlen(topicname)-1]=0; 
        while((fscanf(fp,"%s",txt)) != EOF)
        {
	   if((strcmp(txt,"#Topic")==0) ||
	      (strcmp(txt,"#Topiclevel2")==0) ||
	      (strcmp(txt,"#Topiclevel3")==0))
           {
		fseek(fp,ftell(fp)+1,0);
		fgets(txt,100,fp);
		txt[strlen(txt)-1] = 0;
	   }
           if(strcmp(txt,topicname)==0)
           {
                while(help_search==FALSE)
                {
		    fgets(txt2,100,fp);
		    if(sscanf(txt2,"%s",txt3)==1 && strcmp(txt3,"#endtopic#")==0) 
			help_search=TRUE;
		    else 
		    {
			txt2[strlen(txt2)-1]=0;
			if (txt2[0]==0)
				strcpy(HELPTEXT[helplines]," ");
			else
				strcpy(HELPTEXT[helplines],txt2);
                        helplines++;
		    }
                }
                HELPLINES = helplines;
           }
 
        }
        fclose(fp);

	return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_display_help                                *
 *      Description     : This function will display the help text on 	*
 *			  the window.					*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  scroll_index - the index indicating the line	*
 *			  	being displayed in the help text.	*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayHelp.                                 *
 *      History         : Written on April 20, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_display_help ( int scroll_index )
{

	int i,yloc,xloc,error;
 
	error=v_clean_help_window();
	if(error != 0) return(error);
	
	error=v_display_help_title();
        if(error != 0) return(error);

        xloc = HELPDISPLAY_BOX_XLOC+lib_help->font_width*2;
 
        for(i=0+scroll_index;i<HELPLINES;i++)
        {
                yloc=HELPDISPLAY_BOX_YLOC+
				(i-scroll_index+3)*(lib_help->font_height);
                if(yloc<HELPDISPLAY_BOX_YLOC+
				HELPDISPLAY_BOX_HEIGHT-lib_help->font_height)
                {
		  HELP_SCROLL_ACTIVATE_FLAG=FALSE;
		  if(i==HELPLINES-1 && i>HELP_PAGE_LENGTH) 
				HELP_SCROLL_ACTIVATE_FLAG=TRUE;
                  error=VDisplayText(lib_help->win,lib_help->gc,
			HELPTEXT[i],
                        xloc,yloc);
		  if(error != 0) return(error);
                  XFlush(lib_display);
                }
		else { HELP_SCROLL_ACTIVATE_FLAG=TRUE;}
        }

	return(0);
}


/************************************************************************/
static int v_get_topic_page_length ( void )
{
	int i,yloc;

	for(i=0;i<50;i++)
        {
		yloc=TOPICDISPLAY_BOX_YLOC+
                                (i+3)*(lib_help->font_height);
                if(yloc<TOPICDISPLAY_BOX_YLOC+
                                TOPICDISPLAY_BOX_HEIGHT-lib_help->font_width)
			++TOPIC_PAGE_LENGTH;
	}

	return(0);
}

/************************************************************************/
int v_get_help_page_length ( void )
{
	int i,yloc;

        for(i=0;i<50;i++)
        {
                yloc=HELPDISPLAY_BOX_YLOC+
                                (i+3)*(lib_help->font_height);
                if(yloc<HELPDISPLAY_BOX_YLOC+
                                HELPDISPLAY_BOX_HEIGHT-lib_help->font_height)
                        ++HELP_PAGE_LENGTH;
        }

	return(0);
}

/************************************************************************/
static int v_display_help_title ( void )
{
	int error;

	/****clear the topic display window****/
        XSetForeground(lib_display,lib_help->gc,
                       lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_help->win,lib_help->gc,
			HELPDISPLAY_BOX_XLOC,
                        HELPDISPLAY_BOX_YLOC+lib_help->font_height/2,
                        HELPDISPLAY_BOX_WIDTH - HELPSCROLLBAR_WIDTH,
                        lib_help->font_height*2);
 
        XFlush(lib_display);

	error=VDisplayCenteredText(lib_help->win,lib_help->gc,
			HELPTITLE,
                        HELPDISPLAY_BOX_XLOC,
                        HELPDISPLAY_BOX_YLOC,
                        HELPDISPLAY_BOX_WIDTH - HELPSCROLLBAR_WIDTH,
                        lib_help->font_height*2);
        if(error != 0) return(error);
        XFlush(lib_display);

	return(0);
}
/************************************************************************/
