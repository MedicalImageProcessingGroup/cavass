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
 *      File            : GLOBALS.H                                     *
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
 *                        Modified 10/14/94 lib_colormap_colors added
 *                        by Dewey Odhner.
 *                        Modified: 10/20/94 lib_pixel_mask added
 *                        by Dewey Odhner.
 *                                                                      *
 *                                                                      *
 ************************************************************************/

/*************server related information****************/
extern int LIB_DEPENDENT_PROCESS;
extern char LIB_VISUAL[100],LIB_DISPLAY[100],LIB_GEOMETRY[100];
/*******************************************************/

extern int (*lib_error_function_ptr)(char*);
extern int lib_image_cursor,lib_dialog_cursor,lib_button_cursor;

extern ItemInfo *items ; /* contain the group # and elem # of each item in ACRNEMA */
extern Display *lib_display ;
extern long lib_screen ;
extern Visual *lib_visual ;
extern short lib_internal_gc;               /*indicates if the gc was used 
                                                internally*/
extern short lib_x_server_open ;             /* indicates if x server is  
                                                opened. 
                                                0 - no,
                                                1 - yes. */
extern short lib_visual_class_set ;          /* indicates if visual class is
                                                set. 
                                                0 - no,
                                                1 - yes. */
extern short lib_wins_created ;              /* indicates if windows are 
                                                created. 
                                                0 - no,
                                                1 - yes. */
extern short lib_cmap_created ;              /* indicates if colormap is 
                                                created. 
                                                0 - no,
                                                1 - yes. */
extern short lib_horizontal_menu_events_selected ; /* flag to indicate if 
                                                      XSelectInput is called or
                                                      not for horizontal menu.
                                                      0 - not called before
                                                      1 - called before. */
extern short lib_vertical_menu_events_selected ; /* flag to indicate if 
                                                    XSelectInput is called or 
                                                    not for vertical menu.
                                                    0 - not called before
                                                    1 - called before. */
extern unsigned int lib_depth ; /* the depth of bitmap */
extern WindowGcInfo lib_subwins[MAX_SUBWINS] ;
extern long lib_subwin_event_masks[MAX_SUBWINS];
extern short lib_num_subwins ;
extern WindowGcInfo lib_wins[8]; /* Store the windows' information,
                                    subscript 0 - image window,
                                    subscript 1 - dialogue window,
                                    subscript 2 - button window,
                                    subscript 3 - title window,
                                    subscript 4 - root window, 
                                    subscript 5 - horizontal menu window,
                                    subscript 6 - vertical menu window,
                                    subscript 7 - panel window. */
extern short lib_inherit ;                   /* Specifiy whether the colormap 
                                                inherit from other process. */
extern Colormap lib_cmap ;                   /* the colormap ID */
extern XColor *lib_colormap_colors;          /* list of colormap colors */
extern unsigned long lib_pixel_mask;         /* mask to get color index from
                                                pixel value */
extern short lib_cmap_entries ;              /* the # of entries of colormap.*/
extern short lib_cmap_gray_entries ;         /* the # of available colorcells 
                                                of colormap. */
extern short lib_num_of_overlays ;           /* the # of overlay user 
                                                specifies.*/
extern short lib_overlay_on[MAX_OVERLAYS] ;  /* Specify whether the overlay is
                                                on/off.
                                                0 - off,
                                                1 - on. */
extern unsigned long lib_overlay_planes[2] ; /* the planes to be masked for 
                                                voerlay. */
extern unsigned long lib_red_mask_shift ;    /* the # of bits to be shifted */
extern unsigned long lib_green_mask_shift ;  /* of rgb mask. only valid for */
extern unsigned long lib_blue_mask_shift ;   /* directcolor/truecolor visual */
extern short lib_truecolor_on ;              /* the flag for on/off truecolr 
                                                image displayed, only valid for
                                                pseudocolor visual.
                                                0 - off,
                                                1 - on. */
extern char *lib_cmap_status ;               /* the array to indicate if the 
                                                corresponding colormap entry is 
                                                read/write/overlay.
                                                0 - read only,
                                                1 - read/write (grayscale/
                                                                pseudocolor),
                                                2 - read/write (overlay). 
                                                3 - reserved colors. */
extern ViewnixColor lib_reserved_colors[NUM_OF_RESERVED_COLORCELLS] ;
extern GC lib_win_gc, lib_pixmap_gc ; /* graphic context for copying area in
                                         the image window part. */
extern short lib_3dviewnix_text_item;  /*to indicate if a text item list is 
                                  exclusive to 3dviewnix functions only
                                  1 - yes
                                  0 - no*/
extern short lib_display_status;    /*indicates ready/working status*/
extern short lib_display_fg_bg_on ; /* flag for checking if the fg/bg command
                                       is on/off in the button window
                                       0 - off,
                                       1 - on. */ 
extern short lib_fg_bg_selected ; /* latest fg/bg user selected. */
extern short lib_save_screen_append_selected ; /* latest append/overwrite
                                                  user selected. */
extern short lib_display_save_screen_on ; /* flag for checking if the save 
                                             screen command is on/off in the 
                                             button window.
                                             0 - off,
                                             1 - on. */ 
extern char lib_scrn_file[80] ; /* lastest filename user specified. */
extern short lib_display_temp_output_on ; /* flag for checking if the temp 
                                             output file description command is 
                                             on/off in the button window.
                                             0 - off,
                                             1 - on. */ 
extern short lib_display_output_file_prompt;/*flag for checking if the output 
                                                file description command is 
                                                on/off in the button window.
                                                0 - off,
                                                1 - on. */ 
extern short lib_display_static_button;        /*to check if static buttons
                                          in the button window are on.*/
extern char lib_output_file_desp[3][80] ; /* latest output file description.*/
extern short lib_output_file_selected ; /* latest output file command
                                           the user selects. */
extern short lib_display_dial_win_on ; /* flag for checking if the dialog 
                                          window on/off command is on/off in 
                                          the button window.
                                          0 - off,
                                          1 - on. */
extern short lib_dial_win_on_off_selected ; /* latest on/off user selected. */
extern short lib_dial_win_front ; /* flag for checking if the dialog
                                     window is at the front of image 
                                     window.
                                     0 - image window at the front,
                                     1 - dialog window at the front. */

extern short lib_butt_win_on_off_selected ; 
extern short lib_help_display;                 /*help window is up*/
extern short lib_information_display;          /*info window is up*/
extern short lib_annotation_display;           /*annotation mode is on*/
 
/***Minimality criterion related constants***/
extern short LIB_MIN_PANEL_HEIGHT,LIB_MIN_BUTT_HEIGHT,LIB_MIN_TITLE_HEIGHT;
extern short LIB_MIN_PANEL_WIDTH,LIB_MIN_BUTT_WIDTH,LIB_MIN_SCALE_WIDTH;
 
/***Menu related***/
extern short LIB_HORIZONTAL_MENU_WIDTH, LIB_HORIZONTAL_MENU_HEIGHT ;
extern short LIB_HORIZONTAL_MENU_SWITCH_HEIGHT ;

/***Panel related***/
extern short LIB_PANEL_XLOC, LIB_PANEL_YLOC ;
extern short LIB_PANEL_WIDTH,LIB_PANEL_HEIGHT,LIB_PANEL_AREA;
extern short LIB_PANEL_LINE_XLOC, LIB_PANEL_LINE_YLOC ;
extern short LIB_MAX_PANEL_ROWS ;
extern short LIB_MAX_PANEL_ITEMS_PER_ROW, LIB_PANEL_ITEM_HEIGHT ;
extern short lib_panel_rows ;
extern short lib_panel_num_of_cmds;
extern PanelCmdInfo *lib_panel_cmds;
extern PanelSwitchInfo *lib_panel_switches ;
extern short lib_display_panel_on;
extern short lib_panel_item_selected;

/*saveswitch in button window*/
extern short lib_saveswitch_num_of_cmds;
extern PanelCmdInfo *lib_saveswitch_cmds;
extern PanelSwitchInfo *lib_saveswitch_switches ;
extern short lib_display_saveswitch_on;
extern short lib_saveswitch_item_selected;

/*static buttons in button window*/
extern short lib_statbut_num_of_cmds;
extern PanelCmdInfo *lib_statbut_cmds;

/*dialogonoff in the button window related*/
extern short lib_dialogonoff_num_of_cmds;
extern PanelCmdInfo *lib_dialogonoff_cmds;
extern PanelSwitchInfo *lib_dialogonoff_switches;
extern short lib_display_dialogonoff_on;
extern short lib_dialogonoff_item_selected;
 
/*forebackswitch in the button window related*/
extern short lib_forebackswitch_num_of_cmds;
extern PanelCmdInfo *lib_forebackswitch_cmds;
extern PanelSwitchInfo *lib_forebackswitch_switches;
extern short lib_display_forebackswitch_on;
extern short lib_forebackswitch_item_selected;

/*savescreenswitch in the button window related*/
extern short lib_savescreenswitch_num_of_cmds;
extern PanelCmdInfo *lib_savescreenswitch_cmds;
extern PanelSwitchInfo *lib_savescreenswitch_switches;
extern short lib_display_savescreenswitch_on;
extern short lib_savescreenswitch_item_selected;

/*scales related*/
extern short LIB_SCALE_XLOC, LIB_SCALE_YLOC ;
extern short LIB_SCALE_HEIGHT ;
extern ScaleInfo lib_scale[MAX_SCALES] ;        /* x, y locations and min, max
                                                   values of scales. */
extern short lib_num_of_scales ;                /* the number of scales are 
                                                   used */
extern short lib_display_scale_on[MAX_SCALES] ; /* flag for checking if 
                                                   scales displayed in the 
                                                   dialog window.
                                                   0 - off, 
                                                   1 - on. */

extern short LIB_MARGIN_HEIGHT,LIB_SCROLLBAR_WIDTH;
extern short LIB_DIAL_MSG_XLOC,LIB_DIAL_MSG_YLOC ;
extern char LIB_DIAL_MSG[200];

extern short LIB_BUTT_BOX_XLOC,LIB_BUTT_BOX_YLOC ;
extern short LIB_BUTT_BOX_WIDTH,LIB_BUTT_BOX_HEIGHT ;
extern short LIB_BUTT_MSG_XLOC,LIB_BUTT_MSG_YLOC ;
extern short LIB_BUTT_LINE_XLOC,LIB_BUTT_LINE_YLOC ;

/*****static buttons in the button window*****/
extern short LIB_STATICBUTTON_WIDTH;
extern short LIB_ANNOTATION_BUTTON_XLOC, LIB_ANNOTATION_BUTTON_YLOC;
extern short LIB_HELP_BUTTON_XLOC, LIB_HELP_BUTTON_YLOC;
extern short LIB_NOTES_BUTTON_XLOC, LIB_NOTES_BUTTON_YLOC;

extern short LIB_OUTPUT_FILE_SWITCH_XLOC,LIB_OUTPUT_FILE_SWITCH_YLOC;
extern short LIB_OUTPUT_FILE_TEXT_XLOC,LIB_OUTPUT_FILE_TEXT_YLOC;
extern short LIB_OUTPUT_FILE_BUTTON_XLOC,LIB_OUTPUT_FILE_BUTTON_YLOC;
 
extern short LIB_SAVE_SCREEN_SWITCH_XLOC,LIB_SAVE_SCREEN_SWITCH_YLOC;
extern short LIB_SAVE_SCREEN_TEXT_XLOC,LIB_SAVE_SCREEN_TEXT_YLOC;
extern short LIB_SAVE_SCREEN_BUTTON_XLOC,LIB_SAVE_SCREEN_BUTTON_YLOC;
 
extern short LIB_DIAL_WIN_ON_OFF_XLOC,LIB_DIAL_WIN_ON_OFF_YLOC;
 
extern short LIB_FORE_BACK_TEXT_XLOC,LIB_FORE_BACK_TEXT_YLOC;
extern short LIB_FORE_BACK_BUTTON_XLOC,LIB_FORE_BACK_BUTTON_YLOC;

extern char lib_butt_msg[3][80] ;

/*****************Text Item related*********************/
int VDisplayTablet ( Window win, int x, int y, int width, char* label,
    char* value, int (*proc)(), TEXT** ID );
int VGetTabletPointerFromLabel ( char* text, int* ID );
int VGetTabletValue ( TEXT* ID, char* val );
int VGetTabletValueByLabel ( char* text, char* val );

//extern VGetTextOn(), VGetTextIdByIndex();

//extern VDeleteTabletList(), VDisableAllText();
//extern erase_text_item(), VDisableText(), VEnableText();
//extern VSetTabletValue(), VRemoveTablet(), VSetTextEnabled();
//extern VDisplayText(), VInsideText(), VAppendText();
//extern VDeleteLastTextCharacter(), VCheckTabletEvent();
//extern VSetTabletCaret(), VSetTabletLocation();
//extern VDeleteTablet(), VGetNextText();

/****************Button Item related********************/
//extern VSetButtonItemLocation();
//extern VDeleteButtonItemItemList();
//extern VDisplayButtonItem();
//extern VDeleteButtonItem();
//extern VGetButtonItemLabel();
//extern VSetButtonItemLabel();
//extern VRemoveButtonItem();
//extern VRedisplayButtonItem();
//extern VDisplayButton();
//extern VCheckButtonItemEvent();
/**************************************************/

