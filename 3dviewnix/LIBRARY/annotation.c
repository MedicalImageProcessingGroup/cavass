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
 *      Filename  : annotation.c                                        *
 *      Ext Funcs : VDisplayAnnotation.                                 *
 *      Int Funcs : v_list_fonts,v_save_panel_structure,                *
 *                  v_restore_panel_structure,v_get_grid_value,         *
 *                  v_set_title_graphics_panel,v_font_switchboard,      *
 *                  v_line_type_switchboard,v_line_mode_switchboard,    *
 *                  v_thick_switchboard,v_line_style_switchboard,       *
 *                  v_grid_switchboard,v_Quit_but_proc,                 *
 *                  v_correct_for_subwindow,v_get_key,v_delete_character*
 *                  v_HandleTextEvent,v_Draw_Line,v_rotate_2d,          *
 *                  v_Draw_Arrow,v_draw_thin_box,v_Draw_box,            *
 *                  v_HandleLineEvent,v_HandleBoxEvent,                 *
 *                  v_HandleEraseEvent,v_HandleButtonEvent.             *
 *                                                                      *
 ************************************************************************/

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
 
#include "Vlibrary.h"
#include "3dv.h"
 
#define ABS(x)  (x>=0) ? x : -x
 
typedef int (*FUNC_PTR)(char*);



/*******************/
/* LOCAL FUNCTIONS */
static int v_font_switchboard ( char* value );
static int v_grid_switchboard ( char* value );
static int v_HandleBoxEvent ( char* unused );
static int v_HandleButtonEvent ( XEvent* event );
static int v_HandleEraseEvent ( char* unused );
static int v_HandleLineEvent ( char* unused );
static int v_HandleTextEvent ( char* unused );
static int v_line_mode_switchboard ( char* value );
static int v_line_style_switchboard ( char* value );
static int v_line_type_switchboard ( char* value );
static int v_list_fonts ( char* mc, int nmax );
static int v_Quit_but_proc ( char* unused );
static int v_save_panel_structure ( void );
static int v_set_title_graphics_panel ( void );
static int v_thick_switchboard ( char* value );

/********************/
/* GLOBAL VARIABLES */

/* Structures that hold previous panel information (while in title-graphics mode) */
int lib_nsave;
int lib_nrows;
PanelCmdInfo *lib_cmdsave;
PanelSwitchInfo *lib_swsave;
static int panel_was_on;

int lib_font_type=0;
int lib_line_type, lib_line_mode, lib_line_thick, lib_line_style, lib_grid_value;
int lib_title_quit;
short lib_grid[] = {1, 5, 10, 15, 20, 25, 30, 40, 60};

XFontStruct **lib_title_fonts;
char **lib_fonts;
short lib_number_of_fonts;

/* Width of line used for drawing */
int lib_line_width[9] = {1, 2, 4, 6, 8, 12, 14, 18, 24};

XEvent lib_title_event;
Window lib_title_win;

GC lib_iigc,    /* image window GC */
        lib_xigc,       /* GC used for XOR on the image window */
        lib_eigc;       /* GC used to erase on the image window */

int lib_num_title_cmds;
PanelCmdInfo lib_title_cmds[30];
FUNC_PTR lib_title_cmd_func[30];


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayAnnotation                            *
 *      Description     : This function is called when the user wants to*
 *                        ANNOTATE on the user interface. It takes over *
 *                        the interaction by handling all events and    *
 *                        creating a new panel. Once it is completed the*
 *                        old panel is reinstated and interaction is    *
 *                        given back to original program.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         264 - suitable fonts not found.
 *      Parameters      :  None.                                        *
 *      Side effects    : The program has to redisplay the mouse button *
 *                        messages after the completion of this         *
 *                        function.                                     *
 *      Entry condition : v_create_windows should be called before this   *
 *                        function is called. 3dviewnix interface must  *
 *                        exist.                                        *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on June 20, 1993, by Krishna Iyer.   *
 *                        Modified 1/24/95 subwindow mode changed for
 *                        lib_xigc and lib_eigc by Dewey Odhner.
 *                        Modified 3/14/95 lib_number_of_fonts freed
 *                        instead of 10 by Dewey Odhner.
 *                        Modified 1/17/96 font pattern changed Dewey Odhner.
 *                        Modified 1/4/99 image subwindow events cancelled
 *                           by Dewey Odhner.
 *                        Modified 4/26/04 not to use fonts that don't load
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayAnnotation ( int (*refresh_func)() )
{
    int i, j;
    XGCValues values;
    int  prev_subwindow_mode;
    ViewnixColor colors[NUM_OF_RESERVED_COLORCELLS];
	long subwin_event_masks[MAX_SUBWINS];

    if (lib_wins_created == 0) {
        printf("The error occurred in the function ");
        printf("VDisplayAnnotation.\n") ;
        printf("Please call VSetup before ") ;
        printf("calling this function.\n") ;
        kill(getpid(),LIB_EXIT) ;
    }

    if(lib_dial_win_on_off_selected==FALSE) return(0);
    lib_annotation_display=TRUE;

	for (i=0; i<lib_num_subwins; i++) {
		subwin_event_masks[i] = lib_subwin_event_masks[i];
		VSelectEvents(lib_subwins[i].win, ExposureMask|EnterWindowMask);
	}

    /* copy previous panel structure to temporary memory */
    v_save_panel_structure();

    /* Clear previous Panel area */
    VDeletePanel();


     /* Get copies of GCs */
    VGetWindowGC(lib_wins[0].win, &lib_iigc);
    VGetWindowGC(lib_wins[0].win, &lib_xigc);
    VGetWindowGC(lib_wins[0].win, &lib_eigc);


    /* Modify GC */
    XSetFunction(lib_display, lib_iigc, GXcopy);
    XSetFunction(lib_display, lib_eigc, GXcopy);
    XSetFunction(lib_display, lib_xigc, GXinvert);


    /* Get available fonts (with "x" in the name and 
       name_length <= 10 cha racters) */
    v_list_fonts("*x??", 10);
	if (lib_number_of_fonts == 0) {
        XFreeGC(lib_display, lib_iigc);
        XFreeGC(lib_display, lib_xigc);
        XFreeGC(lib_display, lib_eigc);
		return (264);
	}

    lib_line_type = lib_line_mode = lib_line_thick = lib_line_style = lib_grid_value = 0;

    for(i=0; i<lib_number_of_fonts; i++)
	    if((lib_title_fonts[i]= (XFontStruct *)XLoadQueryFont(lib_display, lib_fonts[i])) == NULL)
    	{
            printf("ERROR: Can't load font [%s] !!\n", lib_fonts[i]);
			for (j=i; j<lib_number_of_fonts-1; j++)
				strcpy(lib_fonts[j], lib_fonts[j+1]);
			lib_number_of_fonts--;
			i--;
		}

    XSetFont(lib_display, lib_iigc, lib_title_fonts[0]->fid);

    /* Buttons */
    v_set_title_graphics_panel();

    VDisplayButtonAction("Select","","");

    values.plane_mask = AllPlanes;
    values.subwindow_mode = IncludeInferiors;
    v_assigned_reserved_colorcells(colors, lib_visual->map_entries);
    values.foreground = colors[6].pixel;
    XChangeGC(lib_display, lib_iigc, GCPlaneMask|GCSubwindowMode|GCForeground,
        &values);
    values.foreground = colors[9].pixel;
    XChangeGC(lib_display, lib_eigc, GCPlaneMask|GCSubwindowMode|GCForeground,
        &values);
    values.plane_mask =  colors[6].pixel^colors[9].pixel;
    XChangeGC(lib_display, lib_xigc, GCPlaneMask|GCSubwindowMode, &values);

    /******************************************************/
    /*            EVENT HANDLER                           */
    /******************************************************/
    lib_title_quit = FALSE;
    while (lib_title_quit == FALSE)
    {
        VNextEvent(&lib_title_event);
 
        lib_title_win=lib_title_event.xany.window;
 
 
        /* NEW BUTTON WINDOW ITEMS */
        if(lib_title_win == lib_wins[2].win)
        VCheckEventsInButtonWindow(&lib_title_event,refresh_func);
 
        /* check for Button Panel events */
        if(lib_title_win == lib_wins[1].win)
            lib_title_quit = v_HandleButtonEvent(&lib_title_event);
 
    } /* endwhile !title_quit */
    /******************************************************/
    /*            END EVENT HANDLER                         */
    /******************************************************/



	for (i=0; i<lib_num_subwins; i++)
		VSelectEvents(lib_subwins[i].win, subwin_event_masks[i]);


    /* Free GCs */
    XFreeGC(lib_display, lib_iigc);
    XFreeGC(lib_display, lib_xigc);
    XFreeGC(lib_display, lib_eigc);
 
 
 
    /* FREE FONTS */
    for(i=0; i<lib_number_of_fonts; i++)
        XFreeFont(lib_display, lib_title_fonts[i]);
 
 
    lib_annotation_display=FALSE;
    XWarpPointer(lib_display,None,lib_wins[2].win,0,0,0,0,
        LIB_ANNOTATION_BUTTON_XLOC+LIB_STATICBUTTON_WIDTH/2,
        LIB_ANNOTATION_BUTTON_YLOC+LIB_PANEL_ITEM_HEIGHT/2);
    XFlush(lib_display);
    return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_list_fonts                                  *
 *      Description     : This function lists the fonts available on the*
 *                        server that match the specified "matching     *
 *                        string" and that are smaller than the given   *
 *                        length (ex. 6x10 has length = 4).             *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  mc - matching string.                        *
 *                         nmax - maximum length of font name.          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_list_fonts ( char* mc, int nmax )
#if 0
char *mc;       /* matching string for the font names */
int nmax;       /* maximum number of character allowed for the font name */
#endif
{
        char **list;
        int i;
        int n;
        int counter;

        list = XListFonts(lib_display, mc, 100, &n);

        if(list == NULL)
                return(1);

        /* Count how many fonts have "x" and are smaller than 'nmax' characters */
        counter = 0;
        for(i=0; i<n; i++)
        {
                if(strlen(list[i]) < nmax)
                        counter++;
        }

        /* Allocate memory */
        lib_fonts = (char **) malloc(sizeof(char *) * counter);
        lib_title_fonts = (XFontStruct **) malloc( sizeof(XFontStruct *) * counter);
        lib_number_of_fonts = counter;

        /* Get the font names */
        counter = 0;
        for(i=0; i<n; i++)
        {
                if(strlen(list[i]) < nmax)
                {
                        lib_fonts[counter] = (char *) malloc(sizeof(char) * nmax+1);
                        strcpy( lib_fonts[counter], list[i]);
                        counter++;
                }
        }
        

        XFreeFontNames(list);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_save_panel_structure                        *
 *      Description     : This function saves a copy of the Panel       *
 *                        Structures (PanelCmdInfo and PanelSwitchInfo) *
 *                        into temporary memory for later retrieval.    *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : Static variable panel_was_on and global variables
 *                        lib_nsave, lib_nrows, lib_cmdsave, lib_swsave
 *                        may be set.
 *      Entry condition : lib_display_panel_on must be properly set.
 *      Related funcs   : v_restore_panel_structure
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 1/5/95 check if panel is on by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_save_panel_structure ( void )
{
    int i, j;

    panel_was_on = FALSE;
    if (!lib_display_panel_on)
        return (0);

    lib_nsave = lib_panel_num_of_cmds;
    lib_nrows = lib_panel_rows;
    lib_cmdsave = (PanelCmdInfo *) malloc(lib_nsave * sizeof(PanelCmdInfo) );
    lib_swsave = (PanelSwitchInfo *) malloc(lib_nsave*sizeof(PanelSwitchInfo));

    if(lib_cmdsave == NULL || lib_swsave == NULL)
        return(1);

    panel_was_on = TRUE;

    for(i=0; i<lib_nsave; i++)
    {
        strcpy( lib_cmdsave[i].cmd, lib_panel_cmds[i].cmd);
        lib_cmdsave[i].type = lib_panel_cmds[i].type;
        lib_cmdsave[i].group = lib_panel_cmds[i].group;
        if(lib_cmdsave[i].type == 0)
        {
            lib_cmdsave[i].num_of_switches = lib_panel_cmds[i].num_of_switches;
            lib_cmdsave[i].switches = lib_panel_cmds[i].switches;
        }
        else
        {
            lib_cmdsave[i].num_of_switches = lib_panel_cmds[i].num_of_switches;
            lib_cmdsave[i].switches = (Char30 *)malloc(sizeof(Char30)*lib_panel_cmds[i].num_of_switches);
            for(j=0; j<lib_panel_cmds[i].num_of_switches; j++)
                strcpy(lib_cmdsave[i].switches[j], lib_panel_cmds[i].switches[j]);
        }
        lib_cmdsave[i].switch_item_displayed = lib_panel_cmds[i].switch_item_displayed;
        lib_swsave[i].x = lib_panel_switches[i].x;
        lib_swsave[i].width = lib_panel_switches[i].width;
    }

    return(0);
        
}


/************************************************************************
 *                                                                      *
 *      Function        : v_restore_panel_structure                     *
 *      Description     : This function restores Panel Structure data   *
 *                        that was previously saved. This function is   *
 *                        used when we want to erase the users panel    *
 *                        and restore it afterwards.                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : Changes the panel structure within the        *
 *                        library.                                      *
 *      Entry condition : None.                                         *
 *      Related funcs   : v_save_panel_structure.                       *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 1/5/95 check if panel is on by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_restore_panel_structure( void )
{
    int i, j;

    if (!panel_was_on)
        return (0);

    lib_panel_num_of_cmds = lib_nsave;
    lib_panel_rows = lib_nrows;
    lib_panel_cmds = (PanelCmdInfo *) malloc(lib_nsave * sizeof(PanelCmdInfo));
    lib_panel_switches = (PanelSwitchInfo *) malloc(lib_nsave * sizeof(PanelSwitchInfo) );
    if(lib_panel_cmds == NULL  ||  lib_panel_switches == NULL)
        return(1);

    for(i=0; i<lib_nsave; i++)
    {
        strcpy( lib_panel_cmds[i].cmd, lib_cmdsave[i].cmd);
        lib_panel_cmds[i].type = lib_cmdsave[i].type;
        lib_panel_cmds[i].group = lib_cmdsave[i].group;
        if(lib_cmdsave[i].type == 0)
        {
            lib_panel_cmds[i].num_of_switches = lib_cmdsave[i].num_of_switches;
            lib_panel_cmds[i].switches = lib_cmdsave[i].switches;
        }
        else
        {
            lib_panel_cmds[i].num_of_switches = lib_cmdsave[i].num_of_switches;
            lib_panel_cmds[i].switches = (Char30 *)malloc(sizeof(Char30)*lib_panel_cmds[i].num_of_switches);
            for(j=0; j<lib_panel_cmds[i].num_of_switches; j++)
                strcpy(lib_panel_cmds[i].switches[j], lib_cmdsave[i].switches[j]);
        }
        lib_panel_cmds[i].switch_item_displayed = lib_cmdsave[i].switch_item_displayed;
        lib_panel_switches[i].x = lib_swsave[i].x;
        lib_panel_switches[i].width = lib_swsave[i].width;
    }

    /* Free temporary storage */
    for(i=0; i<lib_nsave; i++)
        if(lib_cmdsave[i].type != 0)
            free(lib_cmdsave[i].switches);
    free(lib_cmdsave);
    free(lib_swsave);

    v_display_panel(FALSE);

    return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_get_grid_value                              *
 *      Description     : This function approximates the given point to *
 *                        the closest grid point, given a grid value 'g'*
 *                        in both directions (Ox and Oy) and given a    *
 *                        point (x,y).                                  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  g - grid value in pixels.                    *
 *                         x - pointer to the x coordinate.             *
 *                         y - pointer to the y coordinate.             *
 *      Side effects    : Values of "x" and "y" parameters are changed. *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_get_grid_value ( int g, int* x, int* y )
{
        int a, b, g_2;
        
        a = *x % g;
        b = *y % g;

        g_2 = g/2;


        /* << X >> */
        /* Round UP */
        if( a > g_2)
                *x = *x + (g-a);
        else
        /* Round DOWN */
                *x = *x - a;


        /* << Y >> */
        /* Round UP */
        if( b > g_2)
                *y = *y + (g-b);
        else
        /* Round DOWN */
                *y = *y - b;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_set_title_graphics_panel                    *
 *      Description     : This function creates the Panel for the       *
 *                        ANNOTATION session.                           *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         215 - cannot display panel when there is     * 
 *                              a scale in the same space.              *
 *                         242 - panel is out of the dialog window      *
 *                              boundary.                               *
 *                         248 - panel is already displayed in the      *
 *                              dialog window.                          *
 *                         254 - panel command has been truncated.      *
 *                         258 - pixmap is NULL and the image is at the *
 *                              front.                                  *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_set_title_graphics_panel ( void )
{
        int group = 0;
        int i;

        lib_num_title_cmds = 0;


    /*. . . . . . . . . . . . . . . . . . . . . . . . . */
    /* GROUP 0 */

        /* TEXT */
        lib_title_cmds[lib_num_title_cmds].group= group;
        lib_title_cmds[lib_num_title_cmds].type = 0;
        strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"TEXT");
        lib_title_cmd_func[lib_num_title_cmds] = v_HandleTextEvent;
        lib_num_title_cmds++;

        /* FONT TYPE */
    lib_title_cmds[lib_num_title_cmds].group= group;
    lib_title_cmds[lib_num_title_cmds].type = 1;
    strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"FONT");
        lib_title_cmds[lib_num_title_cmds].num_of_switches = lib_number_of_fonts;
        lib_title_cmds[lib_num_title_cmds].switches = (Char30 *) malloc(sizeof(Char30)*lib_title_cmds[lib_num_title_cmds].num_of_switches);
        for(i=0; i<lib_number_of_fonts; i++)
        {
                strncpy(lib_title_cmds[lib_num_title_cmds].switches[i], lib_fonts[i], 6);
                lib_title_cmds[lib_num_title_cmds].switches[i][6] = 0;
        }
    lib_title_cmd_func[lib_num_title_cmds] = v_font_switchboard;
    lib_num_title_cmds++;

    /*. . . . . . . . . . . . . . . . . . . . . . . . . */
    /* GROUP 1 */
        group++;

        lib_title_cmds[lib_num_title_cmds].group= group;
        lib_title_cmds[lib_num_title_cmds].type = 0;
        strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"LINE");
        lib_title_cmd_func[lib_num_title_cmds] = v_HandleLineEvent;
        lib_num_title_cmds++;

        /* LINE TYPE */
    lib_title_cmds[lib_num_title_cmds].group= group;
    lib_title_cmds[lib_num_title_cmds].type = 1;
    strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"TYPE");
        lib_title_cmds[lib_num_title_cmds].num_of_switches = 2;
        lib_title_cmds[lib_num_title_cmds].switches = (Char30 *) malloc(sizeof(Char30)*lib_title_cmds[lib_num_title_cmds].num_of_switches);
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[0], "Plain");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[1], "Arrow");
    lib_title_cmd_func[lib_num_title_cmds] = v_line_type_switchboard;
    lib_num_title_cmds++;

        lib_title_cmds[lib_num_title_cmds].group= group;
        lib_title_cmds[lib_num_title_cmds].type = 0;
        strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"BOX");
        lib_title_cmd_func[lib_num_title_cmds] = v_HandleBoxEvent;
        lib_num_title_cmds++;

    /*. . . . . . . . . . . . . . . . . . . . . . . . . */
    /* GROUP 2 */
        group++;

        /* LINE MODE */
    lib_title_cmds[lib_num_title_cmds].group= group;
    lib_title_cmds[lib_num_title_cmds].type = 1;
    strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"MODE");
        lib_title_cmds[lib_num_title_cmds].num_of_switches = 2;
        lib_title_cmds[lib_num_title_cmds].switches = (Char30 *) malloc(sizeof(Char30)*lib_title_cmds[lib_num_title_cmds].num_of_switches);
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[0], "Solid");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[1], "Dash");
    lib_title_cmd_func[lib_num_title_cmds] = v_line_mode_switchboard;
    lib_num_title_cmds++;

        /* THICKNESS */
    lib_title_cmds[lib_num_title_cmds].group= group;
    lib_title_cmds[lib_num_title_cmds].type = 1;
    strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"THICK");
        lib_title_cmds[lib_num_title_cmds].num_of_switches = 9;
        lib_title_cmds[lib_num_title_cmds].switches = (Char30 *) malloc(sizeof(Char30)*lib_title_cmds[lib_num_title_cmds].num_of_switches);
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[0], "1");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[1], "2");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[2], "4");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[3], "6");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[4], "8");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[5], "12");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[6], "14");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[7], "18");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[8], "24");
    lib_title_cmd_func[lib_num_title_cmds] = v_thick_switchboard;
    lib_num_title_cmds++;

        /* STYLE */
    lib_title_cmds[lib_num_title_cmds].group= group;
    lib_title_cmds[lib_num_title_cmds].type = 1;
    strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"STYLE");
        lib_title_cmds[lib_num_title_cmds].num_of_switches = 2;
        lib_title_cmds[lib_num_title_cmds].switches = (Char30 *) malloc(sizeof(Char30)*lib_title_cmds[lib_num_title_cmds].num_of_switches);
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[0], "Butt");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[1], "Round");
    lib_title_cmd_func[lib_num_title_cmds] = v_line_style_switchboard;
    lib_num_title_cmds++;
    /*. . . . . . . . . . . . . . . . . . . . . . . . . */
    /* GROUP 2 */
        group++;

    lib_title_cmds[lib_num_title_cmds].group= group;
    lib_title_cmds[lib_num_title_cmds].type = 1;
    strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"GRID");
        lib_title_cmds[lib_num_title_cmds].num_of_switches = 9;
        lib_title_cmds[lib_num_title_cmds].switches = (Char30 *) malloc(sizeof(Char30)*lib_title_cmds[lib_num_title_cmds].num_of_switches);
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[0], "1");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[1], "5");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[2], "10");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[3], "15");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[4], "20");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[5], "25");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[6], "30");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[7], "40");
        strcpy(lib_title_cmds[lib_num_title_cmds].switches[8], "60");
    lib_title_cmd_func[lib_num_title_cmds] = v_grid_switchboard;
    lib_num_title_cmds++;

        lib_title_cmds[lib_num_title_cmds].group= group;
        lib_title_cmds[lib_num_title_cmds].type = 0;
        strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"ERASE");
        lib_title_cmd_func[lib_num_title_cmds] = v_HandleEraseEvent;
        lib_num_title_cmds++;

        lib_title_cmds[lib_num_title_cmds].group= group;
        lib_title_cmds[lib_num_title_cmds].type = 0;
        strcpy(lib_title_cmds[lib_num_title_cmds].cmd,"QUIT");
        lib_title_cmd_func[lib_num_title_cmds] = v_Quit_but_proc;
        lib_num_title_cmds++;

        return( VDisplayPanel(lib_num_title_cmds, lib_title_cmds));

}



/************************************************************************
 *                                                                      *
 *      Function        : v_font_switchboard                            *
 *      Description     : This function is callback function for a      *
 *                        switch in the Panel. It updates the value of  *
 *                        a global variable related to the switch. It   *
 *                        also checks if the chosen font is valid.      *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  value - string representing the option on the*
 *                               switch.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_font_switchboard ( char* value )
{
        int i;
        int sn= 1; /* <- INDEX OF THE BUTTON/SWITCH BEING CHECKED */

        int nfonts;
        int counter;


        /* Check the current value of the switch */
        for(i=0; i<lib_title_cmds[sn].num_of_switches; i++)
        {
                if( !strcmp(value, lib_title_cmds[sn].switches[i]) )
                        lib_font_type = i;
        }

        nfonts = lib_number_of_fonts;
        counter = 0;
        /* If font wasn't loaded, then get the next valid one */
        if(lib_title_fonts[lib_font_type] == NULL)
        {
                XBell(lib_display, 0);
                /* Get the next valid font */
                while(counter < nfonts && lib_title_fonts[lib_font_type] == NULL)
                {
                        XBell(lib_display, 0);
                        counter++;
                        lib_font_type++;
                        if(lib_font_type == nfonts) lib_font_type = 0;
                }

                if(lib_title_fonts[lib_font_type] == NULL)
                {
                        XBell(lib_display, 0);
                        VDisplayDialogMessage("ERROR: No Fonts Available !!! Check with you system's administrator.");
                }
                else
                        VChangePanelItem("FONT", "FONT", lib_font_type);
        }
        
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_line_type_switchboard                       *
 *      Description     : This function is a callback function for a    *
 *                        switch in the Panel. It updates the value of  *
 *                        a global variable related to the switch.      *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  value - string representing the option on the*
 *                               switch.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_line_type_switchboard ( char* value )
{
        int i;
        int sn= 3; /* <- INDEX OF THE BUTTON/SWITCH BEING CHECKED */

        for(i=0; i<lib_title_cmds[sn].num_of_switches; i++)
        {
                if( !strcmp(value, lib_title_cmds[sn].switches[i]) )
                        lib_line_type = i;
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_line_mode_switchboard                       *
 *      Description     : This function is a callback function for a    *
 *                        switch in the Panel. It updates the value of a*
 *                        global variable related to the switch.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  value - string representing the option on    *
 *                              the switch.                             *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_line_mode_switchboard ( char* value )
{
        int i;
        int sn= 5; /* <- INDEX OF THE BUTTON/SWITCH BEING CHECKED */

        for(i=0; i<lib_title_cmds[sn].num_of_switches; i++)
        {
                if( !strcmp(value, lib_title_cmds[sn].switches[i]) )
                        lib_line_mode = i;
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_thick_switchboard                           *
 *      Description     : This function is a callback function for a    *
 *                        switch in the Panel. It updates the value of a*
 *                        global variable related to the switch.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  value - string representing the option on    *
 *                              the switch.                             *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_thick_switchboard ( char* value )
{
        int i;
        int sn = 6; /* <- INDEX OF THE BUTTON/SWITCH BEING CHECKED */

        for(i=0; i<lib_title_cmds[sn].num_of_switches; i++)
        {
                if( !strcmp(value, lib_title_cmds[sn].switches[i]) )
                        lib_line_thick = i;
        }
        
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_line_style_switchboard                      *
 *      Description     : This function is a callback function for a    *
 *                        switch in the Panel. It updates the value of a*
 *                        global variable related to the switch.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  value - string representing the option on    *
 *                              the switch.                             *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_line_style_switchboard ( char* value )
{
        int i;
        int sn = 7; /* <- INDEX OF THE BUTTON/SWITCH BEING CHECKED */

        for(i=0; i<lib_title_cmds[sn].num_of_switches; i++)
        {
                if( !strcmp(value, lib_title_cmds[sn].switches[i]) )
                        lib_line_style = i;
        }
        
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_grid_switchboard                            *
 *      Description     : This function is a callback function for a    *
 *                        switch in the Panel. It updates the value of a*
 *                        global variable related to the switch.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  value - string representing the option on    *
 *                              the switch.                             *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_grid_switchboard ( char* value )
{
        int i;
        int sn = 8; /* <- INDEX OF THE BUTTON/SWITCH BEING CHECKED */

        for(i=0; i<lib_title_cmds[sn].num_of_switches; i++)
        {
                if( !strcmp(value, lib_title_cmds[sn].switches[i]) )
                        lib_grid_value = i;
        }

        return(0);      
}


/************************************************************************
 *                                                                      *
 *      Function        : v_Quit_but_proc                               *
 *      Description     : This function quits the ANNOTATE function and *
 *                        restores the Panel.                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_Quit_but_proc ( char* unused )
{
        /* Clear Panel area */
        VDeletePanel();

        v_restore_panel_structure();
        lib_title_quit = TRUE;

        v_draw_dialogwin_border();

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_correct_for_subwindow                       *
 *      Description     : This function corrects the given corrdinate   *
 *                        for occurrance inside any of the subwindows   *
 *                        of the image window. It returns the corrected *
 *                        value on the point itself.                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  win - window in which event occurred.        *
 *                         x - X coord. of event.                       *
 *                         y - Y coord. of event.                       *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_correct_for_subwindow ( Window win, int* x, int* y )
{
        register int i;

        for (i=0; i< lib_num_subwins; i++) 
                if(win == lib_subwins[i].win) 
                {
                        *x = *x + lib_subwins[i].x;
                        *y = *y + lib_subwins[i].y;
                        break ;
                }

        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : v_get_key                                     *
 *      Description     : This function checks if an XEvent is a Key    *
 *                        event and returns the ASCII code for the key. *
 *                        It also keeps track of Shift and CapsLock     *
 *                        keys (with static variables).                 *
 *      Return Value    :  0 - no alphanumeric key was pressed (no      *
 *                              press, Shift or Capslock).              *
 *                         ASCII - ascii code for the pressed key.      *
 *      Parameters      :  event - pointer to an XEvent structure.      *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_get_key ( XEvent* event )
{
        char c[50];
        int type;

        static int shift_flag = 0;


        type = event->type;

    if(type == KeyPress)
    {
        /* Get the KEY string */
        strcpy(c, XKeysymToString( XLookupKeysym((XKeyEvent*)event,0) ));
 
        /* Check for SHIFT */
        if( strcmp(c,"Shift_L") == 0 || strcmp(c,"Shift_R")==0)
        {
            shift_flag = 1;
            return(0);
        }
        /* Check for CAPS_LOCK */
        if( strcmp(c, "Caps_Lock")==0 )
        {
            if(shift_flag == 0) shift_flag = 1;
            else shift_flag = 0;
            return(0);
        }
 
        if( strcmp(c, "comma") == 0) strcpy(c, ",");
                else
        if( strcmp(c, "period") == 0) strcpy(c, ".");
                else
        if( strcmp(c, "space") == 0) strcpy(c, " ");
                else
        if( strcmp(c, "slash") == 0) strcpy(c, "/");
                else
        if( strcmp(c, "minus") == 0) strcpy(c, "-");
                else
        if( strcmp(c, "equal") == 0) strcpy(c, "=");
                else
        if( strcmp(c, "backslash") == 0) strcpy(c, "\\");
                else
        if( strcmp(c, "semicolon") == 0) strcpy(c, ";");
                else
        if( strcmp(c, "apostrophe") == 0) strcpy(c, "'");
                else
        if( strcmp(c, "grave") == 0) strcpy(c, "`");
 
        /* If UPPER CASE */
        if(shift_flag == 1)
        {
            /* if lower case letter, make Upper case */
            if( c[0] > 96 && c[0] < 123) c[0] -= 32;
                        else
                        {
                if( strcmp(c, ",") == 0) strcpy(c, "<");
                                else
                if( strcmp(c, ".") == 0) strcpy(c, ">");
                                else
                if( strcmp(c, " ") == 0) strcpy(c, " ");
                                else
                if( strcmp(c, "/") == 0) strcpy(c, "?");
                                else
                if( strcmp(c, "1") == 0) strcpy(c, "!");
                                else
                if( strcmp(c, "2") == 0) strcpy(c, "@");
                                else
                if( strcmp(c, "3") == 0) strcpy(c, "#");
                                else
                if( strcmp(c, "4") == 0) strcpy(c, "$");
                                else
                if( strcmp(c, "5") == 0) strcpy(c, "%");
                                else
                if( strcmp(c, "6") == 0) strcpy(c, "^");
                                else
                if( strcmp(c, "7") == 0) strcpy(c, "&");
                                else
                if( strcmp(c, "8") == 0) strcpy(c, "*");
                                else
                if( strcmp(c, "9") == 0) strcpy(c, "(");
                                else
                if( strcmp(c, "0") == 0) strcpy(c, ")");
                                else
                if( strcmp(c, "-") == 0) strcpy(c, "_");
                                else
                if( strcmp(c, "=") == 0) strcpy(c, "+");
                                else
                if( strcmp(c, "\\") == 0) strcpy(c, "|");
                                else
                if( strcmp(c, ";") == 0) strcpy(c, ":");
                                else
                if( strcmp(c, "'") == 0) strcpy(c, "\"");
                                else
                if( strcmp(c, "`") == 0) strcpy(c, "~");
                        }
 
        }

                if( strcmp(c, "Return") == 0)
                        return(13);

                if( strcmp(c, "Delete") == 0  ||  strcmp(c, "BackSpace") == 0)
                        return(8);

                return(c[0]);
        }
        else
        if(type == KeyRelease)
    {
        strcpy(c, XKeysymToString( XLookupKeysym((XKeyEvent*)event,0) ));
 
        /* Check for SHIFT or CAPS RELEASE */
        if(strcmp(c,"Shift_L") == 0 )
        {
            shift_flag = 0;
            return(0);
        }
 
        return(0);
    }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_delete_character                            *
 *      Description     : This function deletes the last character of a *
 *                        string drawned on the image window.           *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *                         258 - pixmap of dialog window is NULL and    *
 *                              image window is in front.               *
 *      Parameters      :  text - string to have its last character     *
 *                              deleted.                                *
 *                         tx - X coord. of location of character string*
 *                              on the image window.                    *
 *                         ty - Y coord. of location of character string*
 *                              on the image window.                    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_delete_character ( char* text, int tx, int ty )
{
        int a, b;
        int ascent, descent;

        /*************************************************************/
        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
        /*************************************************************/


        /* length before and after deleting */
        a = XTextWidth(lib_title_fonts[lib_font_type], text, strlen(text));
        text[ strlen(text) - 1 ] = 0;


        /*************************************************************/
        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
        if(1) return 0;
        /*************************************************************/

        b = XTextWidth(lib_title_fonts[lib_font_type], text, strlen(text));
        ascent = lib_title_fonts[lib_font_type]->ascent;
        descent = lib_title_fonts[lib_font_type]->descent;
        
        XFillRectangle(lib_display, lib_wins[0].win, lib_eigc, tx+b, ty-ascent, (a-b), ascent+descent);
        /*return(VClearWindow(lib_wins[0].win, tx+b, ty-ascent, (a-b), ascent+descent));*/

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_HandleTextEvent                             *
 *      Description     : This function is the Event Handler function   *
 *                        for writing text on the image window. It      *
 *                        allows the user to click the mouse left button*
 *                        for specifying the initial location of the    *
 *                        text to be written. After that the user can   *
 *                        type the characters and in case of a          *
 *                        <RETURN> key it will jump to the beginning of *
 *                        the next line.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 1/24/95 event window check restored
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_HandleTextEvent ( char* unused )
{
        XEvent event;
        Window win;
        int phase;
        int quit;
        int eventx, eventy;
        int prevx, prevy;
        int button;
        int type;
        char text[500];
        int c;
        int i;

        int tx, ty;
        

        tx = ty = -1;
        quit = FALSE;
        phase = 0;
        for(i=0; i<500; i++) text[i] = 0;

        VDisplayButtonAction("Position","","Done");

        while(quit == FALSE)
        {
                VNextEvent(&event);


                type = event.type;
                button = event.xbutton.button;
                eventx = event.xbutton.x;
                eventy = event.xbutton.y;
                win = event.xany.window;
                
                v_correct_for_subwindow(win, &eventx, &eventy);
                v_get_grid_value( lib_grid[lib_grid_value], &eventx, &eventy);

                /* check for Button Panel events */
                if(win == lib_wins[1].win)
                        quit = v_HandleButtonEvent(&event);
                else
                /* IMAGE WINDOW */
                if(win == lib_wins[0].win)
                {

                if(phase == 0)
                {
                        c = v_get_key(&event);
                        if(c > 0 && tx > 0)
                        {
                                /* RETURN */
                                if( c == 13)
                                {
                                        /* Delete CARET */
                                        v_delete_character(text, tx, ty);

                                        /* Draw definite string */
                                        XDrawString(lib_display, lib_wins[0].win, lib_iigc, tx, ty, text, strlen(text));
                                        
                                        for(i=0; i<500; i++) text[i] = 0;
                                        strcpy(text,"");
                                        ty += lib_title_fonts[lib_font_type]->ascent + lib_title_fonts[lib_font_type]->descent;

                                        /* Append CARET again */
                                        strcat(text, "|");
                                        
                                        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
                                }
                                else
                                /* DELETE */
                                if(c == 8)
                                {
                                        /* Delete CARET */
                                        v_delete_character(text, tx, ty);
                                        
                                        /* Delete last character */
                                        v_delete_character(text, tx, ty);

                                        /*************************************************************/
                                        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
                                        /*************************************************************/

                                        /* Append CARET again */
                                        strcat(text, "|");
                                        
                                        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
                                }
                                /* CHARACTER */
                                else
                                {
                                        /* Delete CARET */
                                        v_delete_character(text, tx, ty);
                                        
                                        /*************************************************************/
                                        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
                                        /*************************************************************/

                                        /* Append CHARACTER */
                                        text[ strlen(text) ] = c;
                                        text[ strlen(text) + 1] = 0;

                                        /* Append CARET again */
                                        strcat(text, "|");
                                        
                                        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
                                }

                        }


                        if(type == EnterNotify)
                    VDisplayButtonAction("Position","","Done");
                        else
                        if(type == ButtonPress)
                        {
                                /* SET INITIAL POSITION */
                                if(button == LEFT_BUTTON)
                                {
                                        /* Delete CARET (if applicable) */
                                        if( tx > 0)
                                        {
                                                v_delete_character(text, tx, ty);
                                                /* Draw definite string */
                                                XDrawString(lib_display, lib_wins[0].win, lib_iigc, tx, ty, text, strlen(text));
                                        }
                                        

                                        tx = eventx;
                                        ty = eventy;
                                        /*phase = 1;*/
                                        XGrabKeyboard(lib_display, win, False, GrabModeAsync,GrabModeAsync, CurrentTime);

                                        XSetFont(lib_display, lib_iigc, lib_title_fonts[lib_font_type]->fid);
                                        XSetFont(lib_display, lib_xigc, lib_title_fonts[lib_font_type]->fid);
                                        XSetFont(lib_display, lib_eigc, lib_title_fonts[lib_font_type]->fid);

                                        /* Empty line */
                                        for(i=0; i<500; i++) text[i] = 0;
                                        strcpy(text,"");

                                        /* Append CARET */
                                        strcat(text, "|");
                                        XDrawString(lib_display, lib_wins[0].win, lib_xigc, tx, ty, text, strlen(text));
                                }
                                else
                                if(button == MIDDLE_BUTTON)
                                {
                                }
                                else
                                if(button == RIGHT_BUTTON)
                                {
                                        /* Delete CARET */
                                        v_delete_character(text, tx, ty);

                                        /* Draw definite string */
                                        XDrawString(lib_display, lib_wins[0].win, lib_iigc, tx, ty, text, strlen(text));
                                        
                        VDisplayButtonAction("Select","","");
                                        quit = TRUE;
                                }
                
                        }
                        else
                        if(type == MotionNotify)
                        {
                        }
                }
                else
                if(phase == 1)
                {



                        if(type == EnterNotify)
                    VDisplayButtonAction("Position","","Done");
                        else
                        if(type == ButtonPress)
                        {
                                if(button == LEFT_BUTTON)
                                {
                                }
                                else
                                if(button == MIDDLE_BUTTON)
                                {
                                }
                                else
                                if(button == RIGHT_BUTTON)
                                {
                                        /* Delete CARET */
                                        v_delete_character(text, tx, ty);
                                        
                        VDisplayButtonAction("Select","","");
                                        quit = TRUE;
                                }
                
                        }
                        else
                        if(type == MotionNotify)
                        {
                        }
                }

                prevx = eventx;
                prevy = eventy;

                }


        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_Draw_Line                                   *
 *      Description     : This function draws a line on the image window*
 *                        according to some global GC definitions.      *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x1 - X coord. of the initial point.          *
 *                         y1 - Y coord. of the initial point.          *
 *                         x2 - X coord. of the final point.            *
 *                         y2 - Y coord. of the final point.            *
 *                         mode - function used when writing onto the   *
 *                              window (GXcopy, GXxor, etc.).           *
 *      Side effects    : The GC used for the image window have its     *
 *                        function always set to GXcopy after this      *
 *                        function is called.                           *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_Draw_Line ( int x1, int y1, int x2, int y2, int mode )
{
        unsigned int lw;        /* line width */
        int ls, /* line style */
                cs, /* cap style */
                js;     /* join style */


        lw = lib_line_width[lib_line_thick];
        
        switch(lib_line_mode)
        {
                case 0:
                        ls = LineSolid;
                        break;

                case 1:
                        ls = LineOnOffDash;
                        break;
        }

        switch(lib_line_style)
        {
                case 0:
                        cs = CapButt;
                        break;

                case 1:
                        cs = CapRound;
                        break;
        }
        
        js = JoinRound;

        XSetFunction(lib_display, lib_iigc, mode);
        XSetLineAttributes(lib_display, lib_iigc, lw, ls, cs, js);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x1, y1, x2, y2);
        
        XSetFunction(lib_display, lib_iigc, GXcopy);

        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : v_rotate_2d                                   *
 *      Description     : This function rotates a point about the origin*
 *                        by a given number of degrees.                 *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x - X coord. of the point to be rotated.     *
 *                         y - Y coord. of the point to be rotated.     *
 *                         angle - value used for rotation (radians).   *
 *                         xout - pointer to X coord. of result.        *
 *                         yout - pointer to Y coord. of result.        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_rotate_2d ( float x, float y, float angle,
    float* xout,  float* yout )
{
        *xout = x*cos(angle) - y*sin(angle);
        *yout = x*sin(angle) + y*cos(angle);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_Draw_Arrow                                  *
 *      Description     : This function draws an arrow on the image     *
 *                        window.                                       *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x - X coord. of the initial point.           *
 *                         y - Y coord. of the initial point.           *
 *                         xx - X coord. of the final point.            *
 *                         yy - Y coord. of the final point.            *
 *                         mode - function used when writing onto the   *
 *                              window (GXcopy, GXxor, etc.).           *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_Draw_Arrow ( int x, int y, int xx, int yy, int mode )
{
        float alpha, beta, gama1, gama2, pi, length;
        float x1, y1, x2, y2; /* end-points of the arrow head */
        int xx1, yy1, xx2, yy2;
        int dx, dy;
        float size; /* Minimum Head size */
        unsigned int lw;        /* line width */
        int ls, /* line style */
                cs, /* cap style */
                js;     /* join style */


        lw = lib_line_width[lib_line_thick];

        size = 7 + 3*lw;
        
        switch(lib_line_mode)
        {
                case 0:
                        ls = LineSolid;
                        break;

                case 1:
                        ls = LineOnOffDash;
                        break;
        }
        switch(lib_line_style)
        {
                case 0:
                        cs = CapButt;
                        break;

                case 1:
                        cs = CapRound;
                        break;
        }
        js = JoinRound;
        XSetFunction(lib_display, lib_iigc, mode);
        XSetLineAttributes(lib_display, lib_iigc, lw, ls, cs, js);


        dx = xx - x;
        dy = yy - y;


        if(dx == 0  && dy == 0) return 0; /* no displacement */

        /* adjust the size of the head of the arrow */
        length = sqrt((float) (dx*dx) + (float) (dy*dy));

        /* adjust arrow head length */
        if(length > size*2)
        size = size; /* fixed size */
        else
        size = size/2; /* head = 1/2 of the total length */

        pi = acos(-1.0);
        if(dx == 0) alpha = pi/2;
        else alpha = atan((float) dy/dx);
 
        beta = pi/8; /* 30 degrees */
 
        /* get angles for the arrow and one of the head segments */
        if(dx>=0 && dy>=0)
        {
                alpha = ABS(alpha);
                gama1 = pi + alpha - beta;
        }
        else
        if(dx>=0 && dy<0)
        {
                alpha =  - (ABS(alpha));
                gama1 = alpha + pi - beta;

        }
        else
        if(dx<0 && dy<0)
        {
                alpha = ABS(alpha);
                gama1 = alpha - beta;
        }
        else
        if(dx<0 && dy>=0)
        {
                alpha =  -(ABS(alpha));
                gama1 = alpha - beta;
        }
        gama2 = gama1 + 2*beta;
 
        /* draw body of the arrow */
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x, y, xx, yy);
 
        cs = CapRound;
        XSetLineAttributes(lib_display, lib_iigc, lw, ls, cs, js);

        /* find the location of the ends of the head */
        v_rotate_2d(size, 0.0, gama1, &x1, &y1);
        xx1 = (int)x1; yy1 = (int)y1;
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x+dx, y+dy, x+dx+xx1, y+dy+yy1);
 
        v_rotate_2d(size, 0.0, gama2, &x2, &y2);
        xx2 = (int)x2; yy2 = (int)y2;
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x+dx, y+dy, x+dx+xx2, y+dy+yy2);
 
        gama2 = gama1;
        while(gama2 <gama1+2*beta)
        {
                v_rotate_2d(size, 0.0, gama2, &x2, &y2);
                xx2 = (int)x2; yy2 = (int)y2;
                XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x+dx, y+dy, x+dx+xx2, y+dy+yy2);
                 
                gama2 += pi/90;
        }
 
        return(0); 
}


/************************************************************************
 *                                                                      *
 *      Function        : v_draw_thin_box                               *
 *      Description     : This function draws a thin box (1 pixel) on   *
 *                        the image window according to some global     *
 *                        definitions of the GC.                        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x1 - X coord. of the upper left corner.      *
 *                         y1 - Y coord. of the upper left corner.      *
 *                         x2 - X coord. of the lower right corner.     *
 *                         y2 - Y coord. of the lower right corner.     *
 *                         mode - function used when writing onto the   *
 *                              window (GXcopy, GXxor, etc.).           *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_draw_thin_box ( int x1, int y1, int x2, int y2, int mode )
{
        unsigned int lw;        /* line width */
        int ls, /* line style */
                cs, /* cap style */
                js;     /* join style */


        lw = 1;
        ls = LineSolid;
        cs = CapButt;
        js = JoinRound;

        XSetFunction(lib_display, lib_iigc, mode);
        XSetLineAttributes(lib_display, lib_iigc, lw, ls, cs, js);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x1, y1, x2, y1);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x2, y1, x2, y2);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x2, y2, x1, y2);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x1, y2, x1, y1);
        
        XSetFunction(lib_display, lib_iigc, GXcopy);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_Draw_box                                    *
 *      Description     : This function draws a box on the image window *
 *                        according to some global definitions of the   *
 *                        GC.                                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x1 - X coord. of the upper left corner.      *
 *                         y1 - Y coord. of the upper left corner.      *
 *                         x2 - X coord. of the lower right corner.     *
 *                         y2 - Y coord. of the lower right corner.     *
 *                         mode - function used when writing onto the   *
 *                              window (GXcopy, GXxor, etc.).           *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_Draw_box ( int x1, int y1, int x2, int y2, int mode )
{
        unsigned int lw;        /* line width */
        int ls, /* line style */
                cs, /* cap style */
                js;     /* join style */


        lw = lib_line_width[lib_line_thick];
        
        switch(lib_line_mode)
        {
                case 0:
                        ls = LineSolid;
                        break;

                case 1:
                        ls = LineOnOffDash;
                        break;
        }

        switch(lib_line_style)
        {
                case 0:
                        cs = CapButt;
                        break;

                case 1:
                        cs = CapRound;
                        break;
        }
        
        js = JoinRound;

        XSetFunction(lib_display, lib_iigc, mode);
        XSetLineAttributes(lib_display, lib_iigc, lw, ls, cs, js);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x1, y1, x2, y1);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x2, y1, x2, y2);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x2, y2, x1, y2);
        XDrawLine(lib_display, lib_wins[0].win, lib_iigc, x1, y2, x1, y1);
        
        XSetFunction(lib_display, lib_iigc, GXcopy);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_HandleLineEvent                             *
 *      Description     : This function is the Event Handler for the    *
 *                        line drawing function. It allows the user     *
 *                        to specify the end points of the line to be   *
 *                        drawn.                                        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 1/24/95 event window check restored
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_HandleLineEvent ( char* unused )
{
    XEvent event;
    int phase;
    int quit;
    int eventx, eventy;
    int prevx, prevy;
    int button;
    int type;
    Window win;
    int temp_line_drawn;
    int lx1, ly1, lx2, ly2;
    quit = FALSE;


    lx1=ly1=lx2=ly2=-1;

    phase = 0;
    temp_line_drawn = FALSE;

    while(quit == FALSE)
    {
        VNextEvent(&event);

        type = event.type;
        button = event.xbutton.button;
        eventx = event.xbutton.x;
        eventy = event.xbutton.y;
        win = event.xany.window;

        v_correct_for_subwindow(win, &eventx, &eventy);
        
        v_get_grid_value( lib_grid[lib_grid_value], &eventx, &eventy);

        /* check for Button Panel events */
        if(win == lib_wins[1].win)
            quit = v_HandleButtonEvent(&event);
        else
        /* IMAGE WINDOW */
        if(win == lib_wins[0].win)
        {
        if(phase == 0)
        {
            if(type == EnterNotify)
            VDisplayButtonAction("Initial\nPoint","","Done");
            else
            if(type == ButtonPress)
            {
                /* SET INITIAL POSITION */
                if(button == LEFT_BUTTON)
                {
                    lx1 = eventx;
                    ly1 = eventy;
                    phase = 1;
            VDisplayButtonAction("Final\nPoint","","Done");
                }
                else
                if(button == MIDDLE_BUTTON)
                {
                }
                else
                if(button == RIGHT_BUTTON)
                {
            VDisplayButtonAction("Select","","");
                    quit = TRUE;
                }
        
            }
            else
            if(type == MotionNotify)
            {
            }
        }
        else
        if(phase == 1)
        {
            if(type == EnterNotify)
            VDisplayButtonAction("Final\nPoint","","Done");
            else
            if(type == ButtonPress)
            {
                /* SET FINAL POINT */
                if(button == LEFT_BUTTON)
                {
                    if(temp_line_drawn)
                    {   /* Erase previous line */
                        v_Draw_Line(lx1, ly1, prevx, prevy, GXxor);
                        temp_line_drawn = FALSE;
                    }
                    lx2 = eventx;
                    ly2 = eventy;

                    if(lib_line_type == 0)                  
                        v_Draw_Line(lx1, ly1, lx2, ly2, GXcopy);
                    else
                        v_Draw_Arrow(lx1, ly1, lx2, ly2, GXcopy);
                

                    phase = 0;
            VDisplayButtonAction("Initial\nPoint","","Done");
                }
                else
                if(button == MIDDLE_BUTTON)
                {
                }
                else
                if(button == RIGHT_BUTTON)
                {
                    if(temp_line_drawn)
                    {   /* Erase previous line */
                        v_Draw_Line(lx1, ly1, prevx, prevy, GXxor);
                        temp_line_drawn = FALSE;
                    }
                
            VDisplayButtonAction("Select","","");
                    quit = TRUE;
                }
        
            }
            else
            /* MOTION */
            if(type == MotionNotify)
            {
                /* If first point was specified, then draw rubber band */
                if(lx1 >= 0)
                {
                    /* Erase previous line */
                    if (temp_line_drawn)
                        v_Draw_Line(lx1, ly1, prevx, prevy, GXxor);


                    /* Draw new line */
                    v_Draw_Line(lx1, ly1, eventx, eventy, GXxor);
                    temp_line_drawn = TRUE;
                    prevx = eventx;
                    prevy = eventy;

                }
            }
        }
        }
    }

    return(0);      
}


/************************************************************************
 *                                                                      *
 *      Function        : v_HandleBoxEvent                              *
 *      Description     : This function is the Event Handler for the box*
 *                        drawing function. It alows the user to specify*
 *                        the diagonal defining the box. A rubber-band  *
 *                        box is drawn during the diagonal definition.  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 1/24/95 event window check restored
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_HandleBoxEvent ( char* unused )
{
    XEvent event;
    int phase;
    int quit;
    int eventx, eventy;
    int prevx, prevy;
    int button;
    int type;
    Window win;
    int temp_box_drawn;
    int lx1, ly1, lx2, ly2;
    quit = FALSE;


    lx1=ly1=lx2=ly2=-1;

    phase = 0;
    temp_box_drawn = FALSE;

    while(quit == FALSE)
    {
        VNextEvent( &event);




        type = event.type;
        button = event.xbutton.button;
        eventx = event.xbutton.x;
        eventy = event.xbutton.y;
        win = event.xany.window;
        
        v_correct_for_subwindow(win, &eventx, &eventy);
        v_get_grid_value( lib_grid[lib_grid_value], &eventx, &eventy);

        /* check for Button Panel events */
        if(win == lib_wins[1].win)
            quit = v_HandleButtonEvent(&event);
        else
        /* IMAGE WINDOW */
        if(win == lib_wins[0].win)
        {
        if(phase == 0)
        {
            if(type == EnterNotify)
                VDisplayButtonAction("Initial\nPoint","","Done");
            else
            if(type == ButtonPress)
            {
                /* SET INITIAL POSITION */
                if(button == LEFT_BUTTON)
                {
                    lx1 = eventx;
                    ly1 = eventy;
                    phase = 1;
                    VDisplayButtonAction("Final\nPoint","","Done");
                }
                else
                if(button == MIDDLE_BUTTON)
                {
                }
                else
                if(button == RIGHT_BUTTON)
                {
                    VDisplayButtonAction("Select","","");
                    quit = TRUE;
                }
        
            }
            else
            if(type == MotionNotify)
            {
            }
        }
        else
        if(phase == 1)
        {
            if(type == EnterNotify)
                VDisplayButtonAction("Final\nPoint","","Done");
            else
            if(type == ButtonPress)
            {
                /* SET FINAL POINT */
                if(button == LEFT_BUTTON)
                {
                    if(temp_box_drawn)
                    {   /* Erase previous line */
                        v_Draw_box(lx1, ly1, prevx, prevy, GXxor);
                        temp_box_drawn = FALSE;
                    }
                    lx2 = eventx;
                    ly2 = eventy;
                    v_Draw_box(lx1, ly1, lx2, ly2, GXcopy);

                    phase = 0;
                    VDisplayButtonAction("Initial\nPoint","","Done");
                }
                else
                if(button == MIDDLE_BUTTON)
                {
                }
                else
                if(button == RIGHT_BUTTON)
                {
                    if(temp_box_drawn)
                    {   /* Erase previous line */
                        v_Draw_box(lx1, ly1, prevx, prevy, GXxor);
                        temp_box_drawn = FALSE;
                    }
                
                    VDisplayButtonAction("Select","","");
                    quit = TRUE;
                }
        
            }
            else
            /* MOTION */
            if(type == MotionNotify)
            {
                /* If first point was specified, then draw rubber band */
                if(lx1 >= 0)
                {
                    /* Erase previous line */
                    if(temp_box_drawn)
                        v_Draw_box(lx1, ly1, prevx, prevy, GXxor);


                    /* Draw new line */
                    v_Draw_box(lx1, ly1, eventx, eventy, GXxor);
                    temp_box_drawn = TRUE;
                    prevx = eventx;
                    prevy = eventy;
                }
            }
        }


        }
    }

    return(0);      
}


/************************************************************************
 *                                                                      *
 *      Function        : v_HandleEraseEvent                            *
 *      Description     : This function is the Event Handler of the     *
 *                        erase function. It allows the user to         *
 *                        specify a region (rectangular) to be erased.  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 1/24/95 event window check restored
 *                        by Dewey Odhner.
 *                        Modified 1/24/95 XFillRectangle called instead
 *                        of VClearWindow by Dewey Odhner.
 *                        Modified 1/25/95 rubber band handling
 *                        changed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_HandleEraseEvent ( char* unused )
{
    XEvent event;
    int phase;
    int quit;
    int eventx, eventy;
    int prevx1, prevy1, prevx2, prevy2;
    int button;
    int type;
    Window win;
    int temp_box_drawn;
    int lx1, ly1, lx2, ly2;
    int dx, dy;
    int tx, ty;

    quit = FALSE;


    lx1=ly1=lx2=ly2=-1;

    phase = 0;
    temp_box_drawn = FALSE;

    while(quit == FALSE)
    {
        VNextEvent( &event);

        type = event.type;
        button = event.xbutton.button;
        eventx = event.xbutton.x;
        eventy = event.xbutton.y;
        win = event.xany.window;
        
        v_correct_for_subwindow(win, &eventx, &eventy);
        v_get_grid_value( lib_grid[lib_grid_value], &eventx, &eventy);

        /* check for Button Panel events */
        if(win == lib_wins[1].win)
            quit = v_HandleButtonEvent(&event);
        else
        /* IMAGE WINDOW */
        if(win == lib_wins[0].win)
        {
        if(phase == 0)
        {
            if(type == EnterNotify)
                VDisplayButtonAction("Initial\nPoint","","Done");
            else
            if(type == ButtonPress)
            {
                /* SET INITIAL POSITION */
                if(button == LEFT_BUTTON)
                {
                    lx1 = eventx;
                    ly1 = eventy;
                    phase = 1;
                    VDisplayButtonAction("Final\nPoint","","Done");
                }
                else
                if(button == MIDDLE_BUTTON)
                {
                }
                else
                if(button == RIGHT_BUTTON)
                {
                    VDisplayButtonAction("Select","","");
                    quit = TRUE;
                }
        
            }
            else
            if(type == MotionNotify)
            {
            }
        }
        else
        if(phase == 1)
        {
            if(type == EnterNotify)
                VDisplayButtonAction("Final\nPoint","","Done");
            else
            if(type == ButtonPress)
            {
                /* SET FINAL POINT */
                if(button == LEFT_BUTTON)
                {
                    if (!temp_box_drawn)
                    {
                        /* Draw new line */
                        v_draw_thin_box(lx1, ly1, eventx, eventy, GXxor);
                        prevx1 = lx1;
                        prevy1 = ly1;
                        prevx2 = eventx;
                        prevy2 = eventy;
                        temp_box_drawn = TRUE;
                    }



                    lx2 = eventx;
                    ly2 = eventy;

                    phase = 2;
                    VDisplayButtonAction("Confirm","Reject","Done");
                }
                else
                if(button == MIDDLE_BUTTON)
                {
                }
                else
                if(button == RIGHT_BUTTON)
                {
                    /* Erase previous line */
                    if (temp_box_drawn)
                        v_draw_thin_box(prevx1, prevy1, prevx2, prevy2, GXxor);

                    VDisplayButtonAction("Select","","");
                    quit = TRUE;
                }
        
            }
            else
            /* MOTION */
            if(type == MotionNotify)
            {
                /* If first point was specified, then draw rubber band */
                if(lx1 >= 0)
                {
                    /* Erase previous line */
                    if (temp_box_drawn)
                        v_draw_thin_box(prevx1, prevy1, prevx2, prevy2, GXxor);

                    /* Draw new line */
                    v_draw_thin_box(lx1, ly1, eventx, eventy, GXxor);
                    prevx1 = lx1;
                    prevy1 = ly1;
                    prevx2 = eventx;
                    prevy2 = eventy;
                    temp_box_drawn = TRUE;

                }
            }
        }
        else
        if(phase == 2)
        {
            if(type == EnterNotify)
                VDisplayButtonAction("Confirm","Reject","Done");
            else
            if(type == ButtonPress)
            {
                /* Erase previous line */
                if (temp_box_drawn)
                {   v_draw_thin_box(prevx1, prevy1, prevx2, prevy2, GXxor);
                    temp_box_drawn = FALSE;
                }
                if(button == LEFT_BUTTON)
                {
                    /* Correct for negative width and/or height */
                    dx = lx2 - lx1;
                    dy = ly2 - ly1;
                    if(dx < 0)
                    {
                        tx = lx2;
                        lx2 = lx1;
                        lx1 = tx;
                        dx = -dx;
                    }
                    if(dy < 0)
                    {
                        ty = ly2;
                        ly2 = ly1;
                        ly1 = ty;
                        dy = -dy;
                    }

                    XFillRectangle(lib_display, lib_wins[0].win, lib_eigc, lx1-1, ly1-1, (lx2-lx1)+2, (ly2-ly1)+2);
                    phase = 0;
                    VDisplayButtonAction("Initial\nPoint","","Done");
                }
                else
                if(button == MIDDLE_BUTTON || button == RIGHT_BUTTON)
                {
                    phase = 0;
                    if (temp_box_drawn)
                    {   v_draw_thin_box(lx1, ly1, lx2, ly2, GXxor);
                        temp_box_drawn = FALSE;
                    }
                    VDisplayButtonAction("Initial\nPoint","","Done");
                }
        
            }
            else
            if(type == MotionNotify)
            {
            }
        }

        }
    }

    return(0);      
}


/************************************************************************
 *                                                                      *
 *      Function        : v_HandleButtonEvent                           *
 *      Description     : This function handles the events that occur   *
 *                        inside the panel buttons and swtiches. It     *
 *                        checks which button was pressed and calls its *
 *                        callback function.                            *
 *      Return Value    :  FALSE - Quit button wasn't pressed.          *
 *                         TRUE - Quit button was pressed.              *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by                    *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_HandleButtonEvent ( XEvent* event )
{
         int i,error;
        char sel_cmd[80],sel_swt[80];
 
 
        switch (event->type)
        {
                /* GETTING INTO THE WINDOW */
                case EnterNotify:
                error = VDisplayButtonAction("Select\n","Select\n","Update\n");
                /*if (error) handle_error("VDisplayButtonAction","HandleEvent_dial",error,0);*/
                break;
 
                /* BUTTON PRESS */
                case ButtonPress:
                        /* call function that based on Event, gives back Function to call */
                if (!VCheckPanelEvent(event,sel_cmd,sel_swt))
                {
                                /* Check WHICH BUTTON was pressed */
                        for(i=0;i<lib_num_title_cmds;i++)
                                {
                                        /* if BUTTON was pressed */
                                if (!strcmp(sel_cmd,lib_title_cmds[i].cmd))
                                {
                                                /* If PLAIN BUTTON */
                                        if (lib_title_cmds[i].type==0)
                                        {
                                                if (lib_title_cmd_func[i] != NULL)  (lib_title_cmd_func[i])(NULL);
                                        }
                                        else 
                                                /* If SWITCH */
                                                if (lib_title_cmd_func[i] != NULL) 
                                                {
                                                        (lib_title_cmd_func[i])(sel_swt);
                                                }
                                        break;
                                }
                                }
                }
                break;
        }
 
        if(lib_title_quit == TRUE) return(TRUE);
        else  return(FALSE);
 
}

/***********************************************************************/
