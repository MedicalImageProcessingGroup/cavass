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
 *      Name : globals.c                                                *
 *      Version : 1.0                                                   *
 *      Author : Hsiu-Mei Hung                                          *
 *      Date : February 28, 1989                                        *
 *    Modified: 10/14/94 lib_colormap_colors added by Dewey Odhner
 *    Modified: 10/20/94 lib_pixel_mask added by Dewey Odhner
 *      Input : none                                                    *
 *      Output : none                                                   *
 *      Description : Include file for common variables of 3DVIEWNIX    *
 *                    library software package.                         *
 *      Limitation : none                                               *
 *      Functions called : none                                         *
 *                                                                      *
 ************************************************************************/
#include <Viewnix.h>
#include "Vlibtypedef.h"
#include "Vlibparam.h"

/*************server related information****************/
int LIB_DEPENDENT_PROCESS;
char LIB_VISUAL[100],LIB_DISPLAY[100],LIB_GEOMETRY[100];
/*******************************************************/

int (*lib_error_function_ptr)(); /*to check if graphics interface is needed*/

int lib_image_cursor,lib_dialog_cursor,lib_button_cursor;

ItemInfo *items ; /* contain the group # and elem # of each item in ACRNEMA */
Display *lib_display ;
long lib_screen ;
Visual *lib_visual ;
short lib_internal_gc;               /*indicates if the gc was used internally                                  and not passed by the user*/
short lib_x_server_open ;            /* indicates if x server is opened. 
                                        0 - no,
                                        1 - yes. */
short lib_visual_class_set ;         /* indicates if visual class is
                                        set. 
                                        0 - no,
                                        1 - yes. */
short lib_wins_created ;             /* indicates if windows are created. 
                                        0 - no,
                                        1 - yes. */
short lib_cmap_created ;             /* indicates if colormap is created. 
                                        0 - no,
                                        1 - yes. */
short lib_horizontal_menu_events_selected ; /* flag to indicate if XSelectInput
                                               is called or not for horizontal 
                                               menu.
                                               0 - not called before
                                               1 - called before. */
short lib_vertical_menu_events_selected ; /* flag to indicate if XSelectInput
                                             is called or not for vertical 
                                             menu.
                                             0 - not called before
                                             1 - called before. */
unsigned int lib_depth ; /* the depth of bitmap */
WindowGcInfo lib_subwins[MAX_SUBWINS] ;
long lib_subwin_event_masks[MAX_SUBWINS];
short lib_num_subwins ;
WindowGcInfo lib_wins[8];            /* Store the windows' information,
                                        subscript 0 - image window,
                                        subscript 1 - dialogue window,
                                        subscript 2 - button window,
                                        subscript 3 - title window,
                                        subscript 4 - root window,
                                        subscript 5 - horizontal menu window,
                                        subscript 6 - vertical menu window. */
short lib_inherit ;                  /* Specifiy whether the colormap 
                                        inherit from other process. */
Colormap lib_cmap ;                  /* the colormap ID */
XColor *lib_colormap_colors;         /* the array of colors in the colormap */
unsigned long lib_pixel_mask;        /* mask to get color index from pixel
                                        value */
short lib_cmap_entries ;             /* the # of entries of colormap.*/
short lib_cmap_gray_entries ;        /* the # of available colorcells 
                                        of colormap. */
short lib_num_of_overlays ;          /* the # of overlay user 
                                        specifies.*/
short lib_overlay_on[MAX_OVERLAYS] ;  /* Specify whether the overlay is
                                         on/off.
                                         0 - off,
                                         1 - on. */
unsigned long lib_overlay_planes[2] ; /* the planes to be masked for 
                                         voerlay. */
unsigned long lib_red_mask_shift ;    /* the # of bits to be shifted */
unsigned long lib_green_mask_shift ;  /* of rgb mask. only valid for */
unsigned long lib_blue_mask_shift ;   /* directcolor/truecolor visual */
short lib_truecolor_on ;              /* the flag for on/off truecolr 
                                         image displayed, only valid for
                                         pseudocolor visual.
                                         0 - off,
                                         1 - on. */
char *lib_cmap_status ;               /* the array to indicate if the 
                                         corresponding colormap entry is 
                                         read/write/overlay.
                                         0 - read only,
                                         1 - read/write (grayscale/
                                                         pseudocolor),
                                         2 - read/write (overlay). 
                                         3 - reserved colors. */
ViewnixColor lib_reserved_colors[NUM_OF_RESERVED_COLORCELLS] ;
GC lib_win_gc, lib_pixmap_gc ; /* graphic context for copying area in
                                  the image window part. */
short lib_3dviewnix_text_item;  /*to indicate if a text item list is 
                                  exclusive to 3dviewnix functions only
                                  1 - yes
                                  0 - no*/
short lib_display_status;       /*indicates ready/working status*/
short lib_display_fg_bg_on ; /* flag for checking if the fg/bg command
                                is on/off in the button window
                                0 - off, 1 - on. */ 
short lib_fg_bg_selected ; /* latest fg/bg user selected. */
short lib_save_screen_append_selected ; /* latest append/overwrite
                                           user selected. */
short lib_display_save_screen_on ; /* flag for checking if the save 
                                      screen command is on/off in the 
                                      button window.
                                      0 - off,
                                      1 - on. */ 
char lib_scrn_file[80] ; /* lastest filename user specified. */
short lib_display_output_file_prompt; /* flag for checking if the output 
                                         file description command is 
                                         on/off in the button window.
                                         0 - off
                                         1 - on. */ 
short lib_display_static_button;        /*to check if static buttons
                                          in the button window are on.*/
char lib_output_file_desp[3][80] ; /* latest output file description.*/
short lib_output_file_selected ; /* latest output file command
                                    the user selects. */
short lib_display_dial_win_on ; /* flag for checking if the dialog 
                                   window on/off command is on/off in 
                                   the button window.
                                   0 - off,
                                   1 - on. */
short lib_dial_win_on_off_selected ; /* latest on/off user selected. */
short lib_dial_win_front ; /* flag for checking if the dialog
                              window is at the front of image window.
                              0 - image window at the front,
                              1 - dialog window at the front. */

short lib_butt_win_on_off_selected ; 
short lib_help_display;                 /*help window is up*/
short lib_information_display;          /*info window is up*/
short lib_annotation_display;           /*annotation mode is on*/

/***Minimality criterion related constants***/
short LIB_MIN_PANEL_HEIGHT,LIB_MIN_BUTT_HEIGHT,LIB_MIN_TITLE_HEIGHT;
short LIB_MIN_PANEL_WIDTH,LIB_MIN_BUTT_WIDTH,LIB_MIN_SCALE_WIDTH;

/***Menu related***/
short LIB_HORIZONTAL_MENU_WIDTH,LIB_HORIZONTAL_MENU_HEIGHT ;
short LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;

/***Panel related***/
short LIB_PANEL_XLOC, LIB_PANEL_YLOC ;
short LIB_PANEL_WIDTH,LIB_PANEL_HEIGHT,LIB_PANEL_AREA;
short LIB_PANEL_LINE_XLOC, LIB_PANEL_LINE_YLOC ;
short LIB_MAX_PANEL_ROWS ;
short LIB_MAX_PANEL_ITEMS_PER_ROW, LIB_PANEL_ITEM_HEIGHT ;
short lib_panel_rows ;
short lib_panel_num_of_cmds ;
PanelCmdInfo *lib_panel_cmds ;
PanelSwitchInfo *lib_panel_switches ;
short lib_display_panel_on ; /* flag for checking if the panel displayed
                                in the dialog window
                                0 - off,
                                1 - on. */ 
short lib_panel_item_selected ; /* the item user selects */

/*saveswitch in the button window related*/
short lib_saveswitch_num_of_cmds;
PanelCmdInfo *lib_saveswitch_cmds;
PanelSwitchInfo *lib_saveswitch_switches;
short lib_display_saveswitch_on;
short lib_saveswitch_item_selected;

/*static button in the button window related*/ 
short lib_statbut_num_of_cmds;
PanelCmdInfo *lib_statbut_cmds;

/*dialogonoff in the button window related*/
short lib_dialogonoff_num_of_cmds;
PanelCmdInfo *lib_dialogonoff_cmds;
PanelSwitchInfo *lib_dialogonoff_switches;
short lib_display_dialogonoff_on;
short lib_dialogonoff_item_selected;

/*forebackswitch in the button window related*/
short lib_forebackswitch_num_of_cmds;
PanelCmdInfo *lib_forebackswitch_cmds;
PanelSwitchInfo *lib_forebackswitch_switches;
short lib_display_forebackswitch_on;
short lib_forebackswitch_item_selected;

/*savescreenswitch in the button window related*/
short lib_savescreenswitch_num_of_cmds;
PanelCmdInfo *lib_savescreenswitch_cmds;
PanelSwitchInfo *lib_savescreenswitch_switches;
short lib_display_savescreenswitch_on;
short lib_savescreenswitch_item_selected;

/*scales related*/ 
short LIB_SCALE_XLOC, LIB_SCALE_YLOC ;
short LIB_SCALE_HEIGHT ;
ScaleInfo lib_scale[MAX_SCALES] ;       /* x, y locations and min, max
                                           values of scales. */
short lib_num_of_scales ; /* the number of scales are used */
short lib_display_scale_on[MAX_SCALES] ; /* flag for checking if 
                                            scales displayed in the 
                                            dialog window.
                                            0 - off, 
                                            1 - on. */

short LIB_MARGIN_HEIGHT,LIB_SCROLLBAR_WIDTH;
short LIB_DIAL_MSG_XLOC,LIB_DIAL_MSG_YLOC ;
char LIB_DIAL_MSG[200];

short LIB_BUTT_BOX_XLOC,LIB_BUTT_BOX_YLOC ;
short LIB_BUTT_BOX_WIDTH,LIB_BUTT_BOX_HEIGHT ;
short LIB_BUTT_MSG_XLOC,LIB_BUTT_MSG_YLOC ;
short LIB_BUTT_LINE_XLOC,LIB_BUTT_LINE_YLOC ;

/*****static buttons in the button window*****/
short LIB_STATICBUTTON_WIDTH;
short LIB_ANNOTATION_BUTTON_XLOC,LIB_ANNOTATION_BUTTON_YLOC;
short LIB_HELP_BUTTON_XLOC,LIB_HELP_BUTTON_YLOC;
short LIB_NOTES_BUTTON_XLOC, LIB_NOTES_BUTTON_YLOC;

short LIB_OUTPUT_FILE_SWITCH_XLOC,LIB_OUTPUT_FILE_SWITCH_YLOC;
short LIB_OUTPUT_FILE_TEXT_XLOC,LIB_OUTPUT_FILE_TEXT_YLOC;
short LIB_OUTPUT_FILE_BUTTON_XLOC,LIB_OUTPUT_FILE_BUTTON_YLOC;

short LIB_SAVE_SCREEN_SWITCH_XLOC,LIB_SAVE_SCREEN_SWITCH_YLOC;
short LIB_SAVE_SCREEN_TEXT_XLOC,LIB_SAVE_SCREEN_TEXT_YLOC;
short LIB_SAVE_SCREEN_BUTTON_XLOC,LIB_SAVE_SCREEN_BUTTON_YLOC;

short LIB_DIAL_WIN_ON_OFF_XLOC,LIB_DIAL_WIN_ON_OFF_YLOC;

short LIB_FORE_BACK_TEXT_XLOC,LIB_FORE_BACK_TEXT_YLOC;
short LIB_FORE_BACK_BUTTON_XLOC,LIB_FORE_BACK_BUTTON_YLOC;

char lib_butt_msg[3][80] ;

struct int_endian_struct lib_int_test = {{ 0,1,2,3}};

struct short_endian_struct lib_short_test = {{ 0,1}};
