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

int VBlinkBox ( Window win, GC gc, int xloc, int yloc, int width, int height );
int VCancelEvents ( void );
int VCallProcess ( char* text, int mode, char* hostname, char* directory,
    char* label );
int VChangeNumberOfOverlays ( int num_ovl );
int VChangePanelItem ( char* old_cmd, char* new_cmd, int switch_item );
int VCheckButtonItemEvent ( XEvent* event );
int VCheckEventsInButtonWindow ( XEvent* event, int (*refresh_func)() );
int VCheckPanelEvent ( XEvent* event, char cmd_selected[80],
    char switch_selected[80] );
int VClearWindow ( Window win, int xloc, int yloc, int width, int height );
int VCreate3DImageSubwindow ( Window* win, int xloc, int yloc,
    int width, int height);
int VDeleteImageSubwindow ( Window win );
int VDisplayBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height, int button_flag, int scrollbar_flag,
    int rightscrollbar_width, ScrollbarClickInfo* click );
int VDisplayDialogOnOffCommand ( void );
int VDisplaySaveScreenCommand ( void );
int VDisplayTitleString ( char string[30] );
int VDisplayOutputFilePrompt ( PanelCmdInfo* saveswitch,
    char def_name[][MAX_DEFAULT_CHAR], int (*callback_func)() );
int VDeletePanel ( void );
int VDisplayAnnotation ( int (*refresh_func)() );
int VDisplayButtonAction ( char* msg1, char* msg2, char* msg3 );
int VDisplayCaptionBar ( Window win, GC gc, char* marginlabel,
    int xloc, int yloc, int width, int height, MarginClickInfo* click );
int VDisplayCenteredText ( Window win, GC gc, char* text,
    int xloc, int yloc, int width, int height );
int VDisplayDialogMessage ( char* msg );
int VDisplayErrorMessage ( char* msg );
int VDisplayFixedColorBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height, int button_flag, int scrollbar_flag,
    int rightscrollbar_width, ScrollbarClickInfo* click );
int VDisplayFixedColorText ( Window win, GC gc, char* text,
    int xloc, int yloc );
int VDisplayHelp ( void );
int VDisplayInformation ( char outside_files[MAX_DEFAULT_FILES][20],
    int num_outside_files );
int VDisplayPanel ( int num_of_cmds, PanelCmdInfo* cmds );
int VDisplayRunModeCommand ( void );
int VDisplayStatus ( int status );
int VDisplayText ( Window win, GC gc, char* text, int xloc, int yloc );
int VDisplayText2 ( Window win, GC gc, char* text, int xloc, int yloc );
int VDisplayText3 ( Window win, GC gc, char* text, int xloc, int yloc );
int VDisplayXORBox ( Window win, GC gc, int xloc, int yloc,
    int width, int height );
int VGetColormap ( ViewnixColor* colors, int ncolors );
int VGetGeometry ( int* x, int* y, int* width, int* height );
int VGetHeaderLength ( FILE* fp, int* hdrlen );
int VGetInputFiles( FileInfo** files, int* num_of_files, int opt,
    int type_mask );
int VGetWindowFontID ( Window win, XID* id );
int VGetWindowGC ( Window win, GC* gc );
int VGetWindowInformation ( Window win, int* xloc, int* yloc,
    int* width, int* height, int* font_width, int* font_height,
    unsigned long* bg, unsigned long* fg );
int VNextEvent ( XEvent* event );
int VPutImage ( Window win, XImage* ximage, int img_xloc, int img_yloc,
    int win_xloc, int win_yloc, int width, int height );
int VReadData ( char* data, int size, int items,FILE* fp, int* items_read );
int VReadHeader( FILE* fp, ViewnixHeader* vh, char group[5], char element[5] );
int VReadMenucomFile ( VerticalMenuInfo* vertical_menu_info,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS],
    char cmd[30], char process[30], char function[30], char filetype[100],
    char html_link[100], int* terminal_leaf_node );
int VReadTapePathName ( int* num_of_path_name, TapePathName** path_name );
int VRedisplayButtonWindow ( void );
int VRedisplayDialogWindow ( void );
int VRefreshWindows ( Window win );
int VRemoveButtonWindow ( void );
int VRemoveDialogWindow ( void );
int VRemoveMenu ( void );
int VSelectCursor ( int window_type, int cursor_type );
int VSelectEvents ( Window win, unsigned long event_mask );
int VSetTabletValue ( TEXT* item, char* value );
int VSleep ( unsigned int microsec );

int v_assigned_color ( XColor* color, unsigned long pixel,
    unsigned short r, unsigned short g, unsigned short b );
int v_AddButton0 ( Window win, int x, int y, int w, int h, char* label,
    int (*proc)(), int type, BUTTON** ID, int mode );
int v_AddText0 ( Window win, int x, int y, int width, int height,
    char* label, char* value, int (*proc)(void*), TEXT** ID, int mode );
int v_assigned_reserved_colorcells ( ViewnixColor* reserved_colors,
    int entries );
int v_CheckTextEvent_user ( XEvent* event, int mode );
int v_create_read_write_cmap ( void );
int v_DeleteButton0 ( BUTTON* ID, int mode );
int v_DeleteText0 ( TEXT* ID, int mode );
int v_display_panel ( int redisplay_panel_flag );
int v_display_scale ( Drawable drawable, GC gc, Drawable drawable1, GC gc1,
    int id );
int v_DisplayButton ( BUTTON* item );
int v_DisplayStaticButton ( void );
int v_DisplayText ( TEXT* item );
int v_draw_button_window ( int first_time );
int v_draw_buttons ( void );
int v_draw_buttonwin_border ( void );
int v_draw_dialog_window ( void );
int v_draw_dialogwin_border ( void );
int v_draw_imagewin_border ( void );
int v_create_threedscale ( int id );
int v_draw_window_border ( void );
int v_GetTextValue0 ( TEXT* ID, char val[100], int mode );
void v_print_fatal_error ( char* function, char* msg, int value );
int v_read_color_com ( ViewnixColor reserved_colors[NUM_OF_RESERVED_COLORCELLS] );
int v_read_wins_id_com ( void );
int v_ReadData ( char* data, int size, int items, FILE* fp );
int v_refresh_button_list ( Window win );
int v_refresh_text_list ( Window win );
int v_remove_panel ( void );
int v_set_visual_class ( int visual_class );
int v_update_windows ( Window win );
int v_write_wins_id_com ( void );
int v_WriteData ( unsigned char* data, int size, int items, FILE* fp );

