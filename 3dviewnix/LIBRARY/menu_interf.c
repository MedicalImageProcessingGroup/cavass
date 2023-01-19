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
 *      Filename : menu_interf.c                                        *
 *      Ext Funcs : VDisplayMenu, VDisplayTitleString, VCheckMenu, *
 *                  VRemoveMenu, VRemoveMenu, VRedisplayMenu,        *
 *                  v_ReadMenutreeFile, VWriteMenutreeFile,              *
 *                  VReadMenucomFile, VWriteMenucomFile,                *
 *                  VGetMenuInformation.                                *
 *      Int Funcs : v_DisplayHorizontalMenu,v_redraw_horizontal_menu,   *
 *                  v_DisplayVerticalMenu,v_highlight_vertical_menu_item*
 *                  v_RemoveVerticalMenu,v_RemoveHorizontalMenus.       *
 *                                                                      *
 ************************************************************************/

#include "Vlibrary.h"
#include <sys/types.h>
#include <unistd.h>
#include "3dv.h"

#define MSG_X_MRG 3
#define MSG_Y_MRG 7
static char *slash="/";
int vmenu = 0;
char titlestr[5][100];


static int v_DisplayHorizontalMenu ( int opt );
static int v_DisplayVerticalMenu ( void );
static int v_ReadMenutreeFile ( TreeInfo** treeinfo, short* num_of_treeinfo );
static int v_redraw_horizontal_menu ( int horizontal_menu, TreeInfo* treeinfo,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS], int opt );
static int v_RemoveHorizontalMenus ( void );
static int v_RemoveVerticalMenu ( VerticalMenuInfo* vertical_menu_info );

int VReadMenucomFile ( VerticalMenuInfo* vertical_menu_info,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS],
    char cmd[30], char process[30], char function[30], char filetype[100],
    char html_link[100], int* terminal_leaf_node );
int VWriteMenucomFile ( VerticalMenuInfo* vertical_menu_info,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS],
    char cmd[30], char process[30], char function[30], char filetype[100],
    char html_link[100], int terminal_leaf_node );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayMenu                                  *
 *      Description     : This function will save the area to be        *
 *                        displayed the current horizontal menu to the  *
 *                        temporary memory, then display the current    *
 *                        horizontal menu in the image window and update*
 *                        the MENU.COM file.                            *
 *                        This function will read some information from *
 *                        two common files MENU.COM and TREEINFO.COM    *
 *                        which are described in the Appendix C of      *
 *                        document "The 3DVIEWNIX Software System DATA-,*
 *                        GRAPGHICS-, AND PROCESS-INTERFACE FUNCTIONS". *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - can not open MENU.COM file.              *
 *                         238 - can not allocate the image.            *
 *                         260 - no need to call this function.         *
 *      Parameters      :  None.                                        *
 *      Side effects    : The maximum horizontal menu is 5 and the      *
 *                        related window is image window.               *
 *      Entry condition : None.                                         *
 *      Related funcs   : VRemoveMenu, VCheckMenu,                *
 *                        VRedisplayMenu, VGetMenuInformation,         *
 *                        VWriteMenucomFile, VReadMenucomFile.          *
 *      History         : Written on July 19, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on November 20, 1992 by Krishna Iyer.*
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VDisplayMenu ( void )
{
        VerticalMenuInfo vertical_menu_info ;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS] ;
        char cmd[30],process[30],function[30],filetype[100],html_link[100];
        int result,terminal_leaf_node;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayMenu.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        result=VReadMenucomFile(&vertical_menu_info,
                                horizontal_menu_info,
                                cmd,
                                process,
                                function,
                                filetype,
				html_link,
                                &terminal_leaf_node);
        if (result != 0) return(result) ;
        if (strcmp(function,"vertical") == 0) {
            result=v_DisplayVerticalMenu() ;
            return(result) ;
        }
        if (strcmp(function,"horizontal") == 0) {
            result=v_DisplayHorizontalMenu(0) ;
            return(result) ;
        }
        return(260) ;
}
        

/************************************************************************
 *                                                                      *
 *      Function        : v_DisplayHorizontalMenu                       *
 *      Description     : This function will save the area to be        *
 *                        displayed the current horizontal menu to the  *
 *                        temporary memory, then display the current    *
 *                        horizontal menu in the image window and update*
 *                        the MENU.COM file.                            *
 *                        This function will read some information from *
 *                        two common files MENU.COM and TREEINFO.COM    *
 *                        which are described in the Appendix C of      *
 *                        document "The 3DVIEWNIX Software System DATA-,*
 *                        GRAPGHICS-, AND PROCESS-INTERFACE FUNCTIONS". *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - can not open MENU.COM file.              *
 *                         238 - can not allocate the image.            *
 *      Parameters      :  None.                                        *
 *      Side effects    : The maximum horizontal menu is 5 and the      *
 *                        related window is image window.               *
 *      Entry condition : None.                                         *
 *      Related funcs   : VRemoveMenu, VCheckMenu,                *
 *                        VRedisplayMenu, VGetMenuInformation,         *
 *                        VWriteMenucomFile, VReadMenucomFile.          *
 *      History         : Written on July 1, 1989 by Hsiu-Mei Hung.     *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_DisplayHorizontalMenu ( int opt )
//int opt ; /*0-display menu,1-redisplay menu,2-redraw menu */
{
        VerticalMenuInfo vertical_menu_info ;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS] ;
        TreeInfo *treeinfo ;
        short num_of_treeinfo ;
        int horizontal_menu, i, result, height ;
        char cmd[30],process[30],function[30],filetype[100],html_link[100];
        int terminal_leaf_node;
 
        /* read tree structure of commands */
        result=v_ReadMenutreeFile(&treeinfo,&num_of_treeinfo) ;
        if (result != 0) return(result) ;
 
        /* read menu variables of vertical menu and horizontal_menu
           information */
        result=VReadMenucomFile(&vertical_menu_info,
                                horizontal_menu_info,
                                cmd,
                                process,
                                function,
                                filetype,
				html_link,
                                &terminal_leaf_node);
        if (result != 0) return(result) ;
 
        if (lib_horizontal_menu_events_selected == 0) {
            XSelectInput(lib_display,lib_wins[5].win,ButtonPressMask |
                         ExposureMask | OwnerGrabButtonMask) ;
            lib_horizontal_menu_events_selected=1 ;
            height=LIB_HORIZONTAL_MENU_HEIGHT*MAX_HORIZONTAL_MENUS ;
        }
 
        vmenu = 0; /*global variable*/
 
        if (opt == 1) horizontal_menu= -1 ;
        else horizontal_menu=0 ;
        /* get the horizontal menu to be displayed on the screen */
        for (i=0; i<MAX_HORIZONTAL_MENUS; i++) {
            if (horizontal_menu_info[i].item_selected == -1) break ;
            horizontal_menu++ ;
        }
        if (opt == 0) { /* set the horizontal_menu to be displayed on */
            horizontal_menu_info[horizontal_menu].on= 1 ;
            result=VWriteMenucomFile(&vertical_menu_info,
                                     horizontal_menu_info,
                                     cmd,
                                     process,
                                     function,
                                     filetype,
				     html_link,
                                     terminal_leaf_node);
            if (result != 0) return(result) ;
        }
        
        v_redraw_horizontal_menu(horizontal_menu,treeinfo,
                                horizontal_menu_info,opt);
 
        if (vertical_menu_info.up == 1){
                v_DisplayVerticalMenu();
                vertical_menu_info.up = 0;}
        free(treeinfo) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_redraw_horizontal_menu                      *
 *      Description     : This function will redraw the horizontal menu.*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  horizontal_menu - whether it is a            *
 *                              horizontal menu.                        *
 *                         treeinfo - the present tree in the menu      *
 *                              structure.                              *
 *                         horizontal_menu_info - the menu structure    *
 *                              that contains information about the     *
 *                              horizontal menu.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu, VCheckMenu.                *
 *      History         : Written on July 19, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                        Modified 10/31/94 illegal index into treeinfo
 *                        fixed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_redraw_horizontal_menu ( int horizontal_menu, TreeInfo* treeinfo,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS], int opt )
{
        int height, i, j, s_ptr, ptr, y;
        int spt,yloc;
        int menu_xloc,menu_yloc,menu_xloc2,menu_yloc2,offset;
        int menu_option_xloc,menu_option_yloc;
        int menu_option_xloc2,menu_option_yloc2;
        int menu_option_width,menu_option_height;
        short label_x,label_y,label_ascent;
        short label_width,label_height;
        XCharStruct overall;
        int copyright_yloc,dum;
        FILE *fp;
        char text[100];
        XTextItem tlabel;
 
        if(opt==0 || opt==1){
        v_RemoveHorizontalMenus();
        height=(LIB_HORIZONTAL_MENU_HEIGHT*(horizontal_menu+1));
        XResizeWindow(lib_display,lib_wins[5].win,lib_wins[5].width,height) ;
        XRaiseWindow(lib_display,lib_wins[5].win) ;
        XFlush(lib_display);
        }
 
        /* get the first sibling pointer of the horizontal menu to be
           displayed */
        for (i=0, s_ptr=0; i<=horizontal_menu; i++) {
            if (i == MAX_HORIZONTAL_MENUS) break ;
            XSetForeground(lib_display,lib_wins[5].gc,
                           lib_reserved_colors[0].pixel) ;
            y=LIB_HORIZONTAL_MENU_HEIGHT*i;

            offset=2;
            menu_xloc=0 + offset; menu_yloc=y + offset; 
            menu_xloc2=menu_xloc+lib_wins[5].width - offset;
            menu_yloc2=menu_yloc+LIB_HORIZONTAL_MENU_HEIGHT - offset;
            offset=1;
            /***********fill the rectangles***********/
            XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,lib_wins[5].win,
                           lib_wins[5].gc,
                           menu_xloc,menu_yloc,
                           menu_xloc2-menu_xloc,
                           menu_yloc2-menu_yloc);
            XFlush(lib_display);
            /***********top dark lines***********/
            XSetForeground(lib_display,lib_wins[5].gc,
                           lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,lib_wins[5].win,
                        lib_wins[5].gc,
                        menu_xloc-offset,
                        menu_yloc-offset,
                        menu_xloc2+offset,
                        menu_yloc-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[5].win,
                        lib_wins[5].gc,
                        menu_xloc-offset,
                        menu_yloc-offset,
                        menu_xloc-offset,
                        menu_yloc2+offset);
            XFlush(lib_display);
 
            /***********bottom white lines***********/
/*
            XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[5].win,
                        lib_wins[5].gc,
                        menu_xloc-offset,
                        menu_yloc2+offset,
                        menu_xloc2+offset,
                        menu_yloc2+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[5].win,
                        lib_wins[5].gc,
                        menu_xloc2+offset,
                        menu_yloc-offset,
                        menu_xloc2+offset,
                        menu_yloc2+offset);
            XFlush(lib_display);
*/
            ptr=s_ptr ;
            while (s_ptr != -1) {
                strcpy(text,treeinfo[s_ptr].cmd);
                XQueryTextExtents(lib_display,
                                  XGContextFromGC(lib_wins[5].gc),    
                                  text,strlen(text),&dum,&dum,&dum,&overall);

                menu_option_xloc=treeinfo[s_ptr].x;
                menu_option_yloc=y + (LIB_HORIZONTAL_MENU_HEIGHT -
                                  LIB_HORIZONTAL_MENU_SWITCH_HEIGHT)/2;

                menu_option_xloc2=menu_option_xloc+overall.width+
                                        2*lib_wins[0].font_width;
                menu_option_yloc2=menu_option_yloc+
                                  LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                menu_option_width=menu_option_xloc2-menu_option_xloc;
                menu_option_height=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;

                /***********fill the rectangles***********/
                XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[2].pixel);
                XFillRectangle(lib_display,lib_wins[5].win,
                           lib_wins[5].gc,
                           menu_option_xloc,menu_option_yloc,
                           menu_option_width,menu_option_height);
                XFlush(lib_display);
                /***********top white lines***********/
                XSetForeground(lib_display,lib_wins[5].gc,
                               lib_reserved_colors[4].pixel);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
 
                /***********bottom dark lines***********/
                XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[3].pixel);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc2-offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                /***************************************/


                label_width=overall.width;
                label_height=overall.ascent+overall.descent;
                label_ascent = overall.ascent;
                label_x=(short)menu_option_xloc + 
                                (menu_option_width - label_width)/2;
                label_y=(short)menu_option_yloc + 
                        (LIB_HORIZONTAL_MENU_SWITCH_HEIGHT - label_height)/2 +
                        label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[0].font->fid;
 
                XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[5].win,lib_wins[5].gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

                s_ptr=treeinfo[s_ptr].sibling ;
            }
            /* the sibling item user selected */
            for (j=0; j<horizontal_menu_info[i].item_selected; j++)
                ptr=treeinfo[ptr].sibling ;
 
                spt = lib_wins[3].width;
 

            /***highlighting the selected item***/
            if (horizontal_menu_info[i].item_selected != -1) {

                strcpy(text,treeinfo[ptr].cmd);
                XQueryTextExtents(lib_display,
                                  XGContextFromGC(lib_wins[5].gc),
                                  text,strlen(text),&dum,&dum,&dum,&overall);

                menu_option_xloc=treeinfo[ptr].x;
                menu_option_yloc=i*LIB_HORIZONTAL_MENU_HEIGHT + 
                                  (LIB_HORIZONTAL_MENU_HEIGHT -
                                  LIB_HORIZONTAL_MENU_SWITCH_HEIGHT)/2;
                menu_option_xloc2=menu_option_xloc+overall.width+
                                        2*lib_wins[0].font_width;
                menu_option_yloc2=menu_option_yloc+
                                  LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                menu_option_width=menu_option_xloc2-menu_option_xloc;
                menu_option_height=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
 
                /***********fill the rectangles***********/
                XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[10].pixel);
                XFillRectangle(lib_display,lib_wins[5].win,
                           lib_wins[5].gc,
                           menu_option_xloc,menu_option_yloc,
                           menu_option_width,menu_option_height);
                XFlush(lib_display);
                /***********top white lines***********/
                XSetForeground(lib_display,lib_wins[5].gc,
                               lib_reserved_colors[4].pixel);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
 
                /***********bottom dark lines***********/
                XSetForeground(lib_display,lib_wins[5].gc,
                        lib_reserved_colors[3].pixel);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc2-offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[5].win,lib_wins[5].gc,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);

                /***************************************/
                strcpy(text,treeinfo[ptr].cmd);
                XQueryTextExtents(lib_display,
                                  XGContextFromGC(lib_wins[5].gc),
                                  text,strlen(text),&dum,&dum,&dum,&overall);

                label_width=overall.width;
                label_height=overall.ascent+overall.descent;
                label_ascent = overall.ascent;
                label_x=(short)menu_option_xloc +
                                (menu_option_width - label_width)/2;
                label_y=(short)menu_option_yloc +
                        (LIB_HORIZONTAL_MENU_SWITCH_HEIGHT - label_height)/2 +
                        label_ascent;
        
                
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[0].font->fid;
 
                XSetForeground(lib_display,lib_wins[5].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[5].win,lib_wins[5].gc,
                     label_x,label_y,&tlabel,1);
                XFlush(lib_display);
             /***************************************/


        /***drawing the string in the title window***/
        XClearArea(lib_display,lib_wins[3].win,0,0,0,0,False);
        XFlush(lib_display);
        /***copyright notice first***/
        copyright_yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
        sprintf(text,"%s",COPYRIGHT_MSG);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[3].font->fid;
        XSetForeground(lib_display,lib_wins[3].gc,
                       lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                  1,copyright_yloc,&tlabel,1);
        XFlush(lib_display);
 
        if (treeinfo[ptr].y < 1*LIB_HORIZONTAL_MENU_HEIGHT) {
                /***title string next***/
                sprintf(titlestr[0],"/%s",treeinfo[ptr].cmd);
                strcpy(titlestr[4],titlestr[0]);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[0]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }
        if (treeinfo[ptr].y > 1*LIB_HORIZONTAL_MENU_HEIGHT &&
                treeinfo[ptr].y < 2*LIB_HORIZONTAL_MENU_HEIGHT) {
                /***title string next***/
                sprintf(titlestr[1],"%s->%s",titlestr[0],
                        treeinfo[ptr].cmd);
                strcpy(titlestr[4],titlestr[1]);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[1]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }
        if (treeinfo[ptr].y > 2*LIB_HORIZONTAL_MENU_HEIGHT &&
                treeinfo[ptr].y < 3*LIB_HORIZONTAL_MENU_HEIGHT) {
                /***title string next***/
                sprintf(titlestr[2],"%s->%s",titlestr[1],
                        treeinfo[ptr].cmd);
                strcpy(titlestr[4],titlestr[2]);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[2]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }
        if (treeinfo[ptr].y > 3*LIB_HORIZONTAL_MENU_HEIGHT &&
                treeinfo[ptr].y < 4*LIB_HORIZONTAL_MENU_HEIGHT) {
                /***title string next***/
                sprintf(titlestr[3],"%s->%s",titlestr[2],
                        treeinfo[ptr].cmd);
                strcpy(titlestr[4],titlestr[3]);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[3]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }
 
            }
            if (ptr >= 0)
                s_ptr=treeinfo[ptr].son ; /* the first sibling pointer of the
                                         next horizontal menu */
        }

        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : VDisplayTitleString                           *
 *      Description     : This function lets one append strings to the  *
 *                        string which is already displayed on the title*
 *                        window. The idea of this function is to make  *
 *                        the user aware of where in the program (s)he  *
 *                        is.                                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  string - the string to be appended.          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on July 28, 1992 by Krishna Iyer.     *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VDisplayTitleString ( char string[30] )
{
        char cmd[30],process[30],function[30],filetype[100];
        char apptitlestr[1][100];
        int result;
        int ptr = 0;
        int terminal_leaf_node;
        FILE *fp;
        int title_xloc,copyright_yloc;
        char text[100];
        XTextItem tlabel;

        if(strcmp(string,"") == 0)
                sprintf(apptitlestr[0],"%s%s",titlestr[4], string); 
        else
                sprintf(apptitlestr[0],"%s->%s",titlestr[4], string); 
        
        XClearArea(lib_display,lib_wins[3].win,0,0,0,0,False);
        /***copyright notice first***/
        copyright_yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
        sprintf(text,"%s",COPYRIGHT_MSG);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[3].font->fid;
        XSetForeground(lib_display,lib_wins[3].gc,
                       lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                       1,copyright_yloc,&tlabel,1);
        XFlush(lib_display);

        /***title string next***/
        title_xloc=lib_wins[3].width-(strlen(apptitlestr[0])*lib_wins[3].font_width);
        strcpy(text,apptitlestr[0]);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[3].font->fid;
        XSetForeground(lib_display,lib_wins[3].gc,
                       lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                       title_xloc,copyright_yloc,&tlabel,1);
        XFlush(lib_display);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DisplayVerticalMenu                         *
 *      Description     : This function will display vertical menu in   *
 *                        the image window. This function also stores   *
 *                        the image of the area to be displayed vertical*
 *                        menu into the temporary memory, so when the   *
 *                        function v_RemoveVerticalMenu is called, the  *
 *                        previous image in the vertical menu area can  *
 *                        be put back. This function will read some     *
 *                        information from two common files MENU.COM and*
 *                        TREEINFO.COM which are described in the       *
 *                        Appendix D of document "The 3DVIEWNIX Software*
 *                        System DATA-, GRAPHICS-, AND PROCESS-INTERFACE*
 *                        FUNCTIONS".                                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - 1 - memory allocation error.             *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - can not open MENU.COM file.              *
 *                         238 - can not allocate the image.            *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu,VCheckMenu.                 *
 *      History         : Written on July 19, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_DisplayVerticalMenu ( void )
{
        VerticalMenuInfo vertical_menu_info ;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS] ;
        TreeInfo *treeinfo ;
        short num_of_treeinfo ;
        int horizontal_menu, i, j, ptr, s_ptr, item, result ;
        char process[30],cmd[30],function[30],filetype[100],html_link[100];
        XGCValues vals;
        int menu_offset=1;
        int terminal_leaf_node;
        int menu_xloc,menu_xloc2,menu_yloc,menu_yloc2;
        int menu_option_xloc,menu_option_yloc;
        int menu_option_xloc2,menu_option_yloc2;
        int menu_option_width,menu_option_height;
        short label_x,label_y,label_ascent;
        short label_width,label_height;
        XCharStruct overall;
        int dum,offset,vbutton;
 
        char text[100];
        XTextItem tlabel;

 
        /* read tree structure of commands */
        result=v_ReadMenutreeFile(&treeinfo,&num_of_treeinfo) ;
        if (result != 0) return(result) ;
 
        /* read common variables of vertical menu and horizontal_menu
           information */
        result=VReadMenucomFile(&vertical_menu_info,
                                horizontal_menu_info,
                                cmd,
                                process,
                                function,
                                filetype,
				html_link,
                                &terminal_leaf_node);

        if (result != 0) return(result) ;
 
        if (lib_vertical_menu_events_selected == 0) {
            XSelectInput(lib_display,lib_wins[6].win,ButtonPressMask |
                         ExposureMask | OwnerGrabButtonMask) ;
            lib_vertical_menu_events_selected=1 ;
        }
 
        /* get the horizontal_menu displayed on the screen */
        for (i=0, horizontal_menu=0; i<MAX_HORIZONTAL_MENUS; i++) {
            if (horizontal_menu_info[i].item_selected == -1) break ;
            horizontal_menu++ ;
        }
 
        /* get the first sibling pointer of the horizontal menu to be
           displayed */
        for (i=0, s_ptr=ptr=0; i<horizontal_menu; i++) {
            /* the sibling item user selected */
            for (j=0, ptr=s_ptr; j<horizontal_menu_info[i].item_selected; j++)
 
                ptr=treeinfo[ptr].sibling ;
            s_ptr=treeinfo[ptr].son ; /* the first sibling pointer of the
                                         vertical menu */
        }
 
 
        /* compute the number of items in the vertical menu */
        item=0 ;
        ptr=s_ptr ;
        while (ptr != -1) {
            ptr=treeinfo[ptr].sibling ;
            item++ ;
        }
 
        /* the starting location of vertical menu to be displayed */
        vertical_menu_info.x=treeinfo[s_ptr].x-3 ;
        vertical_menu_info.y=horizontal_menu*LIB_HORIZONTAL_MENU_HEIGHT+3 ;
 
        /* the height of the vertical menu to be displayed */
        vertical_menu_info.height=lib_wins[0].font_height*item+4 ;
        vertical_menu_info.up=1 ; /* the vertical menu is up */
 
        lib_wins[6].width=vertical_menu_info.width*1.3;
        lib_wins[6].height=(item*LIB_HORIZONTAL_MENU_SWITCH_HEIGHT)+
                         (0.5*(item+1)*LIB_HORIZONTAL_MENU_SWITCH_HEIGHT);
        lib_wins[6].x=vertical_menu_info.x;
        lib_wins[6].y=vertical_menu_info.y+lib_wins[3].height;
        vertical_menu_info.height=lib_wins[6].height;

        /* update the common variables of vertical menu */
        result= VWriteMenucomFile(&vertical_menu_info,
                                  horizontal_menu_info,
                                  cmd,
                                  process,
                                  function,
                                  filetype,
				  html_link,
                                  terminal_leaf_node);
        if (result != 0) return(result);

        XMoveResizeWindow(lib_display,lib_wins[6].win,lib_wins[6].x,
                          lib_wins[6].y,lib_wins[6].width,lib_wins[6].height);
        XRaiseWindow(lib_display,lib_wins[6].win);
        XFlush(lib_display);
 
        vals.line_width=menu_offset;
        XChangeGC(lib_display,lib_wins[6].gc,GCLineWidth,&vals);

        menu_offset=1;
        menu_xloc=0; menu_yloc=0;
        menu_xloc2=menu_xloc+lib_wins[6].width;
        menu_yloc2=menu_yloc+lib_wins[6].height;

                /*top border-dark*/
        XSetForeground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc+menu_offset,
                  menu_yloc+menu_offset,
                  menu_xloc2-menu_offset,
                  menu_yloc+menu_offset);
        XFlush(lib_display);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc+menu_offset,
                  menu_yloc+menu_offset,
                  menu_xloc+menu_offset,
                  menu_yloc2-menu_offset);
        XFlush(lib_display);
                /*bottom border-light*/
        XSetForeground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[4].pixel);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc+menu_offset,
                  menu_yloc2-menu_offset,
                  menu_xloc2-menu_offset,
                  menu_yloc2-menu_offset);
        XFlush(lib_display);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc2-menu_offset,
                  menu_yloc2-menu_offset,
                  menu_xloc2-menu_offset,
                  menu_yloc+menu_offset);
        XFlush(lib_display);
 
        /* write the commands of menu to be displayed */

        
        vbutton=0;
        XSetForeground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[0].pixel);
        while (s_ptr != -1)
        {
                strcpy(text,treeinfo[s_ptr].cmd);
                XQueryTextExtents(lib_display,
                                  XGContextFromGC(lib_wins[6].gc),
                                  text,strlen(text),&dum,&dum,&dum,&overall);
 
                menu_option_width=vertical_menu_info.width;
                menu_option_height=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                menu_option_xloc=(lib_wins[6].width-menu_option_width)/2;
                menu_option_yloc=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT/2+
                        vbutton*(LIB_HORIZONTAL_MENU_SWITCH_HEIGHT*1.5); 
                menu_option_xloc2=menu_option_xloc+menu_option_width;
                menu_option_yloc2=menu_option_yloc+
                                  LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                offset=menu_offset;
 
                /***********fill the rectangles***********/
                XSetForeground(lib_display,lib_wins[6].gc,
                        lib_reserved_colors[2].pixel);
                XFillRectangle(lib_display,lib_wins[6].win,
                           lib_wins[6].gc,
                           menu_option_xloc,menu_option_yloc,
                           menu_option_width,menu_option_height);
                XFlush(lib_display);
                /***********top white lines***********/
                XSetForeground(lib_display,lib_wins[6].gc,
                               lib_reserved_colors[4].pixel);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
 
                /***********bottom dark lines***********/
                XSetForeground(lib_display,lib_wins[6].gc,
                        lib_reserved_colors[3].pixel);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc2-offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                /***************************************/
 
 
                label_width=overall.width;
                label_height=overall.ascent+overall.descent;
                label_ascent = overall.ascent;
                label_x=(short)menu_option_xloc +
                                (menu_option_width - label_width)/2;
                label_y=(short)menu_option_yloc +
                        (LIB_HORIZONTAL_MENU_SWITCH_HEIGHT - label_height)/2 +
                        label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[0].font->fid;
 
                XSetForeground(lib_display,lib_wins[6].gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
                /******************************************/

                vbutton++;

                s_ptr=treeinfo[s_ptr].sibling;
        }
        free(treeinfo) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_highlight_vertical_menu_item                *
 *      Description     : This function will clear the vertical menu    *
 *                        area, display the all commands in vertical    *
 *                        menu, highlight the specified command.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  treeinfo - the array contains the command    *
 *                              tree structure.                         *
 *                         s_ptr - the first sibling pointer of the     *
 *                              menu.                                   *
 *                         ptr - the pointer of the command to be       *
 *                              highlight.                              *
 *                         vertical_menu_info - the array contains the  *
 *                              menu information.                       *
 *      Side effects    : The related window is image window.           *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu, VCheckMenu.                *
 *      History         : Written on July 19, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_highlight_vertical_menu_item ( TreeInfo* treeinfo, short s_ptr,
    short ptr, int item, VerticalMenuInfo vertical_menu_info )
{
        GC gc ;
        int spt;
        int menu_offset=1;
        int menu_xloc,menu_xloc2,menu_yloc,menu_yloc2;
        int menu_option_xloc,menu_option_yloc;
        int menu_option_xloc2,menu_option_yloc2;
        int menu_option_width,menu_option_height;
        short label_x,label_y,label_ascent;
        short label_width,label_height;
        XCharStruct overall;
        int dum,offset,vbutton;
        int copyright_yloc,yloc;
 
        char text[100];
        XTextItem tlabel;
        
        gc=lib_wins[6].gc ;
        XClearArea(lib_display,lib_wins[6].win,0,0,lib_wins[6].width,
                   lib_wins[6].height,False) ;
        XFlush(lib_display);

        menu_offset=1;
        menu_xloc=0; menu_yloc=0;
        menu_xloc2=menu_xloc+lib_wins[6].width;
        menu_yloc2=menu_yloc+lib_wins[6].height;
 
                /*top border-dark*/
        XSetForeground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc+menu_offset,
                  menu_yloc+menu_offset,
                  menu_xloc2-menu_offset,
                  menu_yloc+menu_offset);
        XFlush(lib_display);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc+menu_offset,
                  menu_yloc+menu_offset,
                  menu_xloc+menu_offset,
                  menu_yloc2-menu_offset);
        XFlush(lib_display);
                /*bottom border-light*/
        XSetForeground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[4].pixel);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc+menu_offset,
                  menu_yloc2-menu_offset,
                  menu_xloc2-menu_offset,
                  menu_yloc2-menu_offset);
        XFlush(lib_display);
        XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  menu_xloc2-menu_offset,
                  menu_yloc2-menu_offset,
                  menu_xloc2-menu_offset,
                  menu_yloc+menu_offset);
        XFlush(lib_display);


        vbutton=0;

        /*write the commands of vertical menu to be displayed*/
        while (s_ptr != -1) {
                strcpy(text,treeinfo[s_ptr].cmd);
                XQueryTextExtents(lib_display,
                                  XGContextFromGC(lib_wins[6].gc),
                                  text,strlen(text),&dum,&dum,&dum,&overall);
 
                menu_option_width=vertical_menu_info.width;
                menu_option_height=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                menu_option_xloc=(lib_wins[6].width-menu_option_width)/2;
                menu_option_yloc=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT/2+
                        vbutton*(LIB_HORIZONTAL_MENU_SWITCH_HEIGHT*1.5);
                menu_option_xloc2=menu_option_xloc+menu_option_width;
                menu_option_yloc2=menu_option_yloc+
                                  LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                offset=menu_offset;
 
                /***********fill the rectangles***********/
                if(vbutton==item)
                  XSetForeground(lib_display,lib_wins[6].gc,
                                 lib_reserved_colors[10].pixel);
                else
                  XSetForeground(lib_display,lib_wins[6].gc,
                                 lib_reserved_colors[2].pixel);
                XFillRectangle(lib_display,lib_wins[6].win,
                           lib_wins[6].gc,
                           menu_option_xloc,menu_option_yloc,
                           menu_option_width,menu_option_height);
                XFlush(lib_display);
                /***********top white lines***********/
                XSetForeground(lib_display,lib_wins[6].gc,
                               lib_reserved_colors[4].pixel);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);

                /***********bottom dark lines***********/
                XSetForeground(lib_display,lib_wins[6].gc,
                        lib_reserved_colors[3].pixel);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc+offset,
                        menu_option_yloc2-offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                XDrawLine(lib_display,lib_wins[6].win,lib_wins[6].gc,
                        menu_option_xloc2-offset,
                        menu_option_yloc+offset,
                        menu_option_xloc2-offset,
                        menu_option_yloc2-offset);
                XFlush(lib_display);
                /***************************************/
 
 
                label_width=overall.width;
                label_height=overall.ascent+overall.descent;
                label_ascent = overall.ascent;
                label_x=(short)menu_option_xloc +
                                (menu_option_width - label_width)/2;
                label_y=(short)menu_option_yloc +
                        (LIB_HORIZONTAL_MENU_SWITCH_HEIGHT - label_height)/2 +
                        label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[0].font->fid;
 
                XSetForeground(lib_display,lib_wins[6].gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[6].win,lib_wins[6].gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
                /******************************************/
 
                vbutton++;
 
                s_ptr=treeinfo[s_ptr].sibling;

        }
        
        vmenu = 1;

        spt = lib_wins[3].width;
 
        /***drawing the string in the title window***/
        XClearArea(lib_display,lib_wins[3].win,0,0,0,0,False);
        XFlush(lib_display);
        /***copyright notice first***/
        copyright_yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
        sprintf(text,"%s",COPYRIGHT_MSG);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[3].font->fid;
        XSetForeground(lib_display,lib_wins[3].gc,
                       lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                  1,copyright_yloc,&tlabel,1);
        XFlush(lib_display);

 
        if (vertical_menu_info.y == 1*LIB_HORIZONTAL_MENU_HEIGHT+3)
        {
                /***title string next***/
                sprintf(titlestr[4],"%s->%s",titlestr[0],treeinfo[ptr].cmd);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[4]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }

        if (vertical_menu_info.y == 2*LIB_HORIZONTAL_MENU_HEIGHT + 3)
        {
                /***title string next***/
                sprintf(titlestr[4],"%s->%s",titlestr[1],treeinfo[ptr].cmd);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[4]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }

        if (vertical_menu_info.y == 3*LIB_HORIZONTAL_MENU_HEIGHT + 3)
        {
                /***title string next***/
                sprintf(titlestr[4],"%s->%s",titlestr[2],treeinfo[ptr].cmd);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[4]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }

        if (vertical_menu_info.y == 4*LIB_HORIZONTAL_MENU_HEIGHT + 3) 
        {
                /***title string next***/
                sprintf(titlestr[4],"%s->%s",titlestr[3],treeinfo[ptr].cmd);
                yloc=(LIB_MIN_TITLE_HEIGHT-lib_wins[3].font_height)/2;
                strcpy(text,titlestr[4]);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[3].font->fid;
                XSetForeground(lib_display,lib_wins[3].gc,
                               lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[3].win,lib_wins[3].gc,
                        spt-(strlen(text)*lib_wins[3].font_width),yloc,
                        &tlabel,1);
                XFlush(lib_display);
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckMenu                               *
 *      Description     : This function will allow the end user to      *
 *                        select a command in the vertical or horizontal*
 *                        menu area. If the cursor is within the        *
 *                        vertical or horizontal menu area, and the     *
 *                        left/middle button is pushed, then the        *
 *                        selected menu item will be highlighted,       *
 *                        and return the string of command selected by  *
 *                        the end user into *cmd, otherwise this        *
 *                        function will wait until the valid action     *
 *                        happens.                                      *
 *                        If the menu item is selected, the vertical    *
 *                        menu will be removed from the image window if *
 *                        it is on, the horizontal menus below the      *
 *                        current one will be erased, and the           *
 *                        communication file MENU.COM file will be      *
 *                        updated. You should call function             *
 *                        v_DisplayVerticalMenu or                      *
 *                        v_DisplayHorizontalMenu, and call function    *
 *                        VSelectEvents to select ButtonPress event in  *
 *                        the image window before you call this         *
 *                        function.                                     *
 *                        After calling this function and you want to   *
 *                        know the name of process or function          *
 *                        associated with the current command end user  *
 *                        selects, you should call function             *
 *                        VReadMenucomFile to get this information.     *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - can not open MENU.COM file.              *
 *                         251 - No menu is displayed in the image      *
 *                              window.                                 *
 *      Parameters      :  cmd - the string of command end user selects *
 *                              from the vertical menu.                 *
 *      Side effects    : The related window is image window.           *
 *                        The related event type is ButtonPress.        *
 *                        If cmd is NULL, this function will print the  *
 *                        proper message to the standard error stream,  *
 *                        produce the core dump file, and exit from the *
 *                        current process.                              *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu,RemoveMenus, VRedisplayMenu,    *
 *                        VWriteMenucomFile, VReadMenucomFile,          *
 *                        VGetMenuInformation.                          *
 *      History         : Written on July 19, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                        Modified 8/24/93 to get the pointer of the
 *                           item user selects based on treeinfo[ptr].x
 *                           by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int VCheckMenu ( char cmd[30] )
{
        XEvent event ;
        TreeInfo *treeinfo ;
        short num_of_treeinfo ;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS] ;
        VerticalMenuInfo vertical_menu_info ;
        int i, j, curx, cury, result, horizontal_menu ;
        int item=-1;
        int start_item_x, end_item_x, sibling_items ;
        short s_ptr, ptr;
        char tcmd[80],process[80],function[80],filetype[100],html_link[100];
        int terminal_leaf_node;
        int xloc,xloc2,yloc,yloc2;

        int shift_flag=FALSE;
        int caps_flag=FALSE;
        char c[50];
        XCharStruct overall;
        int dum;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VSelectMenu.\n") ; 
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (cmd == NULL)
            v_print_fatal_error("VSelectVerticalMenuItem",
                "The pointer of cmd should not be NULL.", (int)cmd) ;

        result=v_ReadMenutreeFile(&treeinfo,&num_of_treeinfo) ;
        if (result != 0) return(result) ;
        result=VReadMenucomFile(&vertical_menu_info,
                                horizontal_menu_info,
                                tcmd,
                                process,
                                function,
                                filetype,
				html_link,
                                &terminal_leaf_node);
        if (result != 0) return(result) ;
        for (i=0; i<MAX_HORIZONTAL_MENUS; i++) 
            if (horizontal_menu_info[i].on == 1) break ;
        if (i == MAX_HORIZONTAL_MENUS) return(251) ;
        if (lib_horizontal_menu_events_selected == 0) {
            XSelectInput(lib_display,lib_wins[5].win,ButtonPressMask | 
                         ExposureMask|OwnerGrabButtonMask|KeyPressMask|
                         KeyReleaseMask) ;
            lib_horizontal_menu_events_selected=1 ;
        }
        if (lib_vertical_menu_events_selected == 0) {
            XSelectInput(lib_display,lib_wins[6].win,ButtonPressMask | 
                         ExposureMask|OwnerGrabButtonMask|KeyPressMask|
                         KeyReleaseMask);
            lib_vertical_menu_events_selected=1 ;
        }
        while (True) {
            VNextEvent(&event) ;
            if (event.type == Expose && event.xexpose.count==0) {
              if(event.xany.window==lib_wins[5].win) 
                v_DisplayHorizontalMenu(2);
                XFlush(lib_display);
                /*XSync(lib_display,True);*/
            }

            if (event.type == ButtonPress) {
                if(event.xbutton.window == lib_wins[2].win)
                {
                        VCheckEventsInButtonWindow(&event,NULL);
                        VDisplayMenu();
                        v_DisplayHorizontalMenu(0);
                        XFlush(lib_display);
                }

                if (event.xbutton.window != lib_wins[5].win &&
                    event.xbutton.window != lib_wins[6].win &&
                    event.xbutton.window != lib_wins[2].win) continue;
                /*if (event.xbutton.button == 3) return(0);*/
                curx=event.xbutton.x;
                cury=event.xbutton.y;
                xloc=(lib_wins[6].width-vertical_menu_info.width)/2;
                xloc2=xloc+vertical_menu_info.width;
                if (vertical_menu_info.up==1 && curx>0 && 
                    curx < vertical_menu_info.width &&
                    cury > 0 && cury < vertical_menu_info.height &&
                    event.xbutton.window == lib_wins[6].win) {
                    /* get the starting sibling pointer of the menu */ 
                    for (i=s_ptr=ptr=0; horizontal_menu_info[i].on !=0; i++) {
                        /* the sibling item user selected */
                        for (j=0, ptr=s_ptr; 
                             j<horizontal_menu_info[i].item_selected; j++) 
                             ptr=treeinfo[ptr].sibling ;
                        s_ptr=treeinfo[ptr].son ; /*the first sibling pointer
                                                 of the next horizontal menu*/
                    }
                    /*the vertical menu item user selected and 
                      get its pointer*/ 
                    /*item=cury/lib_wins[6].font_height;*/
                    /*the new way of determining the item number-Krishna*/
                    for(i=0;i<20;i++)
                    {
                        yloc=LIB_HORIZONTAL_MENU_SWITCH_HEIGHT/2+
                             i*(LIB_HORIZONTAL_MENU_SWITCH_HEIGHT*1.5);
                        yloc2=yloc+LIB_HORIZONTAL_MENU_SWITCH_HEIGHT;
                        if(curx>xloc && curx<xloc2 && cury>yloc && 
                                cury<yloc2) item=i;
                    }
        
                    if(item == -1) return(0);
                    for (i=0, ptr=s_ptr; i<item; i++) 
                        ptr=treeinfo[ptr].sibling;
                    v_highlight_vertical_menu_item(treeinfo,s_ptr,ptr,item,
                                                 vertical_menu_info);
                    v_RemoveVerticalMenu(&vertical_menu_info);/*remove menu*/
                    result=VWriteMenucomFile(&vertical_menu_info,
                                            horizontal_menu_info,
                                            treeinfo[ptr].cmd,
                                            treeinfo[ptr].process,
                                            treeinfo[ptr].function,
                                            treeinfo[ptr].filetype,
                                            treeinfo[ptr].html_link,
                                            treeinfo[ptr].terminal_leaf_node);
                    if (result == 0) strcpy(cmd,treeinfo[ptr].cmd) ;
                    free(treeinfo) ;
                    return(result) ;
                }

                /* cursor is not in horizontal menus area */
                if (event.xbutton.window != lib_wins[5].win) continue ;
                horizontal_menu=cury/LIB_HORIZONTAL_MENU_HEIGHT ;
                if (horizontal_menu >= MAX_HORIZONTAL_MENUS) continue ;
                if (horizontal_menu_info[horizontal_menu].on == 0) continue ;
                if (vertical_menu_info.up == 1) {
                    v_RemoveVerticalMenu(&vertical_menu_info) ;
                        vertical_menu_info.up = 0;}
                /* cursor is in horizontal menus area and get 1st sibling 
                   pointer of the current horizontal menu */
                for (i=0, s_ptr=ptr=0; i<horizontal_menu; i++) {
                    /* the sibling item user selected */
                    for (j=0, ptr=s_ptr; 
                         j<horizontal_menu_info[i].item_selected; j++) 
                        ptr=treeinfo[ptr].sibling ;
                    /*the first sibling pointer of the next horizontal menu*/
                    s_ptr=treeinfo[ptr].son ; 
                }

                /* get total # of items in the current horizontal menu */
                ptr=s_ptr ;
                sibling_items=0 ;
                while (ptr != -1) {
                    sibling_items++ ;
                    ptr=treeinfo[ptr].sibling ;
                }


                /* get the pointer of the item user selects */ 
                item = sibling_items-1;
                for (i=0, ptr=s_ptr; i<sibling_items; i++)
                {       if (curx < treeinfo[ptr].x)
                        {       item = i-1;
                                break;
                        }
                        ptr = treeinfo[ptr].sibling;
                }
                if (item < 0) continue ; /* invalid item */ 
                for (i=0, ptr=s_ptr; i<item; i++) ptr=treeinfo[ptr].sibling ;
                start_item_x=treeinfo[ptr].x ;
                XQueryTextExtents(lib_display, XGContextFromGC(lib_wins[5].gc),
                        treeinfo[ptr].cmd, strlen(treeinfo[ptr].cmd),
                        &dum,&dum,&dum,&overall);
                end_item_x=start_item_x+overall.width+2*lib_wins[0].font_width;
                if (curx < start_item_x || curx > end_item_x) continue ; 
                for (i=horizontal_menu+1; i<MAX_HORIZONTAL_MENUS; i++) {
                    horizontal_menu_info[i].item_selected= -1;
                    horizontal_menu_info[i].on=0 ;
                }
                horizontal_menu_info[horizontal_menu].item_selected=item ;

                v_redraw_horizontal_menu(horizontal_menu,treeinfo,
                                       horizontal_menu_info,0);
                
                strcpy(cmd,treeinfo[ptr].cmd) ;
                result=VWriteMenucomFile(&vertical_menu_info,
                                         horizontal_menu_info,
                                         treeinfo[ptr].cmd,
                                         treeinfo[ptr].process,
                                         treeinfo[ptr].function,
                                         treeinfo[ptr].filetype,
                                         treeinfo[ptr].html_link,
                                         treeinfo[ptr].terminal_leaf_node);
                free(treeinfo) ;
                XCheckTypedEvent(lib_display,Expose,&event) ;
                return(result) ;
            }
        }
}


/************************************************************************
 *                                                                      *
 *      Function        : v_redraw_horizontal_menu                      *
 *      Description     : This function will remove the verticl menu.   *
 *                        If the vertical menu is not displayed in the  *
 *                        image wndow, then this function will return   *
 *                        213. Otherwise this function will remove the  *
 *                        vertical menu from the image window, put the  *
 *                        previous image stored by the function         *
 *                        VDisplayVerticalMenu back, and return 0.      *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         4 - can not open MENU.COM file.              *
 *                         213 - the vertical menu is not displayed     *
 *                              in the image window.                    *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is image window.           *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu,VCheckMenu.                 *
 *      History         : Written on July 1, 1989 by Hsiu-Mei Hung.     *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_RemoveVerticalMenu ( VerticalMenuInfo* vertical_menu_info )
{
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("v_RemoveVerticalMenu.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (!vertical_menu_info->up) return(213) ;
        XLowerWindow(lib_display,lib_wins[6].win) ;
        XFlush(lib_display);

        lib_vertical_menu_events_selected=0 ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VRemoveMenu                                  *
 *      Description     : This function will read the image of the      *
 *                        horizontal menu(s) area into temporary        *
 *                        memory for the function VRedisplayMenu.      *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         4 - can not open MENU.COM file.              *
 *                         238 - can not allocate the image.            *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu,VCheckMenu,                 *
 *                        VCheckMenu, VGetMenuInformation,         *
 *                        VWriteMenucomFile, VReadMenucomFile.          *
 *      History         : Written on July 1, 1989 by Hsiu-Mei Hung.     *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VRemoveMenu ( void )
{
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VRemoveMenu.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        v_RemoveHorizontalMenus() ;
        lib_horizontal_menu_events_selected=0 ;

        v_draw_imagewin_border();

        return(0) ; 
}


/************************************************************************
 *                                                                      *
 *      Function        : v_RemoveHorizontalMenus                       *
 *      Description     : This function will read the image of the      *
 *                        horizontal menu(s) area into temporary        *
 *                        memory for the function VRedisplayMenu.      *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         4 - can not open MENU.COM file.              *
 *                         238 - can not allocate the image.            *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VRemoveMenu.                                 *
 *      History         : Written on April 21, 1991 by Hsiu-Mei Hung.   *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_RemoveHorizontalMenus ( void )
{
        VerticalMenuInfo vertical_menu_info;
        int height;
 
        XLowerWindow(lib_display,lib_wins[5].win) ;
        XFlush(lib_display);

        height=LIB_HORIZONTAL_MENU_HEIGHT*MAX_HORIZONTAL_MENUS ;

        vertical_menu_info.up = 0;
}  


/************************************************************************
 *                                                                      *
 *      Function        : VRedisplayMenu                               *
 *      Description     : This function will put back the image of the  *
 *                        horizontal menu(s) area stored by the         *
 *                        function VRemoveMenu.                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         4 - can not open MENU.COM file.              *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu.                                 *
 *      History         : Written on July 1, 1989 by Hsiu-Mei Hung.     *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VRedisplayMenu ( void )
{
        VerticalMenuInfo vertical_menu_info;
        int result; 

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VRedisplayMenu.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        
          result=v_DisplayHorizontalMenu(1) ;
          v_draw_imagewin_border();

          return(result);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_ReadMenutreeFile                             *
 *      Description     : This function will check whether the input    *
 *                        file TREEINFO.COM exists. If it does not,     *
 *                        then this will allocate the memory space for  *
 *                        treeinfo, read TREEINFO.COM file into the     *
 *                        treeinfo array, assign the number of element  *
 *                        in the array to the num_of_treeinfo, and      *
 *                        return 0 if there is no read error or memory  *
 *                        allocation error occurred.                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         4 - can not open TREEINFO.COM file.          *
 *      Parameters      :  treeinfo - the pointer points to the array   *
 *                              which stores the command tree structure.* 
 *                         num_of_treeinfo - the pointer points to the  *
 *                              short integer which stores the number of*
 *                              elements of the avbove array.           *
 *      Side effects    : If treeinfo or num_of_treeinfo is NULL, this  *
 *                        function will print the proper message to the *
 *                        standard error stream, produce the core dump  *
 *                        file, and exit from the current process.      *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on July 10, 1990 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_ReadMenutreeFile ( TreeInfo** treeinfo, short* num_of_treeinfo )
{
        FILE *fp ;
        int error ;

        if (treeinfo == NULL)
            v_print_fatal_error("v_ReadMenutreeFile",
                "The pointer of treeinfo should not be NULL.", (int)treeinfo) ;
        if (num_of_treeinfo == NULL)
            v_print_fatal_error("v_ReadMenutreeFile",
                "The pointer of num_of_treeinfo should not be NULL.",
                (int)num_of_treeinfo) ;
        *treeinfo=NULL ;
        *num_of_treeinfo=0 ;
        fp=fopen("TREEINFO.COM","rb") ;
        if (fp == NULL) return(4) ;
        error=fread(num_of_treeinfo,1,sizeof(short),fp) ;
        if (error == 0) return(2) ;
        *treeinfo=(TreeInfo *)malloc(sizeof(TreeInfo)*(*num_of_treeinfo)) ;
        if (*treeinfo == NULL) return(1) ;
        error=fread(*treeinfo,*num_of_treeinfo,sizeof(TreeInfo),fp) ;
        if (error == 0) return(2) ;
        fclose(fp) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VWriteMenutreeFile                            *
 *      Description     : This function will write all command tree     *
 *                        structure to the output file TREEINFO.COM     *
 *                        according to the treeinfo information, and    *
 *                        return 0 if there is no write error occurred. *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  treeinfo - the array which contains the      *
 *                              command tree structure.                 *
 *                         num_of_treeinfo - the number of elements of  *
 *                              the above array.                        *
 *      Side effects    : If treeinfo is NULL, this function will print *
 *                        the proper message to the standard error      *
 *                        stream, produce the core dump file, and exit  *
 *                        from the current process.                     *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on July 10, 1990 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VWriteMenutreeFile ( TreeInfo* treeinfo, short num_of_treeinfo )
{
        FILE *fp ;
        int error ;

        if (treeinfo == NULL)
            v_print_fatal_error("VWriteMenutreeFile",
                "The pointer of treeinfo should not be NULL.", (int)treeinfo) ;
        fp=fopen("TREEINFO.COM","wb") ;
        error=fwrite(&num_of_treeinfo,sizeof(short),1,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(treeinfo,sizeof(TreeInfo),num_of_treeinfo,fp) ;
        if (error == 0) return(3) ;
        fclose(fp) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VReadMenucomFile                              *
 *      Description     : This function will read vertical menu and     *
 *                        horizontal menu information, and current      *
 *                        command, rocess, and function names selected  *
 *                        by end user, from file MENU.COM and store into*
 *                        *vertical_menu_info, *horizontal_menu_info,   *
 *                        *cmd, *process, and *function. The value of   *
 *                        process will be the name of process if the    *
 *                        associated command is going to call a process,*
 *                        otherwise it will be empty string for no      *
 *                        process associated with this command. The     *
 *                        value of function will be "vertical" if the   *
 *                        associated command is going to call function  *
 *                        VDisplayVerticalMenu, "horizontal" if the     *
 *                        associated command will call function         *
 *                        VDisplayHorizontalMenu, or empty string for no*
 *                        function calling. After calling function      *
 *                        VCheckMenu and you want to get the       *
 *                        name of process or function associated with   *
 *                        the current command end user selects, you     *
 *                        should call this function to get this         *
 *                        information.                                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         4 - can not open MENU.COM file.              *
 *      Parameters      :  num_of_horizontal_menu_info - the number of  *
 *                              elements in the array                   *
 *                              horizontal_menu_info.                   *
 *                         vertical_menu_info - the pointer points to   *
 *                              the structure which reads the vertical  *
 *                              menu information from teh input file    *
 *                              MENU.COM.                               *
 *                         horizontal_menu_info - the pointer points to * 
 *                              the structure which reads the horizontal*
 *                              menu information from the input file    *
 *                              MENU.COM.                               *
 *      Side effects    : If vertical_menu_info, horizontal_menu_info,  *
 *                        cmd, process, or function is NULL, this       *
 *                        function will print the proper message to the *
 *                        standard error stream, produce the core dump  *
 *                        file, and exit from the current process.      *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu,VRemoveMenu, VCheckMenu,   *
 *                        VRedisplayMenu, VWriteMenucomFile,           *
 *                        VGetMenuInformation.                          *
 *      History         : Written on July 10, 1990 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VReadMenucomFile ( VerticalMenuInfo* vertical_menu_info,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS],
    char cmd[30], char process[30], char function[30], char filetype[100],
    char html_link[100], int* terminal_leaf_node )
{
        FILE *fp;
        int error;

        if (vertical_menu_info == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of vertical_menu_info should not be NULL.",
                 (int)vertical_menu_info);
        if (horizontal_menu_info == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of horizontal_menu_info should not be NULL.",
                 (int)horizontal_menu_info);
        if (cmd == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of cmd should not be NULL.", (int)cmd);
        if (process == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of process should not be NULL.", (int)process);
        if (function == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of function should not be NULL.", (int)function);
	if (filetype == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of filetype should not be NULL.", (int)function);
	if (html_link == NULL)
            v_print_fatal_error("VReadMenucomFile",
                "The pointer of html_link should not be NULL.", (int)function);

        fp=fopen("MENU.COM","rb");
        if (fp == NULL) return(4);
        error=fread(vertical_menu_info,sizeof(VerticalMenuInfo),1,fp) ;
        if (error == 0) return(2);
        error=fread(horizontal_menu_info,sizeof(HorizontalMenuInfo),
                    MAX_HORIZONTAL_MENUS,fp);
        if (error == 0) return(2);
        error=fread(cmd,sizeof(char),30,fp);
        if (error == 0) return(2);
        error=fread(process,sizeof(char),30,fp);
        if (error == 0) return(2);
        error=fread(function,sizeof(char),30,fp);
        if (error == 0) return(2);
        error=fread(filetype,sizeof(char),100,fp);
        if (error == 0) return(2);
        error=fread(html_link,sizeof(char),100,fp);
        if (error == 0) return(2);
        error=fread(terminal_leaf_node,sizeof(int),1,fp);
        if (error == 0) return(2);
        fclose(fp) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VWriteMenucomFile                             *
 *      Description     : This function will write vertical menu and    *
 *                        horizontal menu information, and current      *
 *                        command, process, and function names selected *
 *                        by end user into file MENU.COM. The value of  *
 *                        function will be "vertical" if the associated *
 *                        command is going to call function             *
 *                        VDisplayVerticalMenu, "horizontal" if the     *
 *                        associated command will call function         *
 *                        VDisplayHorizontalMenu, or empty string for no*
 *                        function calling. The function VCheckMenu*
 *                        will call this function to update the command,*
 *                        process, and function associated with the     *
 *                        recent command end user selects.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  vertical_menu_info - the structure contains  *
 *                              the vertical menu information to be     *
 *                              written.                                *
 *                         horizontal_menu_info - the array of structure*
 *                              contain the horizontal_menu information *
 *                              to be output.                           *
 *                         horizontal_menu_info - the menu structure    *
 *                              that contains information about the     *
 *                              horizontal menu.                        *
 *      Side effects    : If vertical_menu_info, horizontal_menu_info,  *
 *                        cmd,  process, or function is NULL, this      *
 *                        function will print the proper message to the *
 *                        standard error stream, produce the core dump  *
 *                        file, and exit from the current process.      *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu, VRemoveMenu, VCheckMenu,  *
 *                        VRedisplayMenu, VReadMenucomFile,            *
 *                        VGetMenuInformation.                          *
 *      History         : Written on July 19, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VWriteMenucomFile ( VerticalMenuInfo* vertical_menu_info,
    HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS],
    char cmd[30], char process[30], char function[30], char filetype[100],
    char html_link[100], int terminal_leaf_node )
{
        FILE *fp ;
        int error ;

        if (horizontal_menu_info == NULL)
            v_print_fatal_error("VWriteMenucomFile",
                "The pointer of horizontal_menu_info should not be NULL.",
                 (int)horizontal_menu_info) ;
        if (cmd == NULL)
            v_print_fatal_error("VWriteMenucomFile",
                "The pointer of cmd should not be NULL.", (int)cmd) ;
        if (process == NULL)
            v_print_fatal_error("VWriteMenucomFile",
                "The pointer of process should not be NULL.", (int)process) ;
        if (function == NULL)
            v_print_fatal_error("VWriteMenucomFile",
                "The pointer of function should not be NULL.", (int)function) ;
	if (filetype == NULL)
            v_print_fatal_error("VWriteMenucomFile",
                "The pointer of filetype should not be NULL.", (int)function) ;
	if (html_link == NULL)
            v_print_fatal_error("VWriteMenucomFile",
                "The pointer of html_link should not be NULL.", (int)function) ;

        fp=fopen("MENU.COM","wb") ;
        error=fwrite(vertical_menu_info,sizeof(VerticalMenuInfo),1,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(horizontal_menu_info,sizeof(HorizontalMenuInfo),
                     MAX_HORIZONTAL_MENUS,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(cmd,sizeof(char),30,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(process,sizeof(char),30,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(function,sizeof(char),30,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(filetype,sizeof(char),100,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(html_link,sizeof(char),100,fp) ;
        if (error == 0) return(3) ;
        error=fwrite(&terminal_leaf_node,sizeof(int),1,fp);
        if (error == 0) return(3);
        fclose(fp) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetMenuInformation                           *
 *      Description     : This function will return maximum number of   *
 *                        horizontal menus to *max_hm, width and height *
 *                        of horizontal menu to *hm_width and           *
 *                        *hm_height, and width and height of font used *
 *                        by horizontal menu to *font_width and         *
 *                        *font_height.                                 *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  hm_width - returns width of horizontal menu. *
 *                         hm_height - returns height of horizontal     *
 *                              menu.                                   *
 *                         font_width - returns width of font used by   *
 *                              horizontal menu.                        *
 *                         font_height - returns height of font used by *
 *                              horizontal menu.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayMenu, VRemoveMenu, VCheckMenu,  *
 *                        VRedisplayMenu, VReadMenucomFile,            *
 *                        VWriteMenucomFile.                            *
 *      History         : Written on August 16, 1991 by Hsiu-Mei Hung.  *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VGetMenuInformation ( int* hm_width, int* hm_height,
    int* font_width, int* font_height )
{
        if (lib_x_server_open == 0) {
            printf("The error occurred in function VGetMenuInformation.\n");
            printf("Please call VSetup before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (hm_width == NULL)
            v_print_fatal_error("VGetMenuInformation",
                "The pointer of hm_width should not be NULL.", (int)hm_width) ;
        if (hm_height == NULL)
            v_print_fatal_error("VGetMenuInformation",
                "The pointer of hm_height should not be NULL.", (int)hm_height) ;
        if (font_width == NULL)
            v_print_fatal_error("VGetMenuInformation",
                "The pointer of font_width should not be NULL.", (int)font_width) ;
        if (font_height == NULL)
            v_print_fatal_error("VGetMenuInformation",
                "The pointer of font_height should not be NULL.", (int)font_height) ;
        *hm_width=LIB_HORIZONTAL_MENU_WIDTH ;
        *hm_height=LIB_HORIZONTAL_MENU_HEIGHT ;
        *font_height=lib_wins[0].font_height ;
        *font_width=lib_wins[0].font_width ;
}
/***************************************************************************/

