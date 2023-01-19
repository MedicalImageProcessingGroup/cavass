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
 *      Filename  : dial_interf.c                                       *
 *      Ext Funcs : VDisplayPanel, VChangePanelItem, VCheckPanelEvent,  *
 *                  VDeletePanel, VDisplayScale,VCheckScaleEvent,       *
 *                  VCheckScaleEvents,VUndisplayScale,VDeleteScale,         *
 *                  VSetScaleInformation,VGetScaleInformation,          *
 *                  VGetScaleLayoutInformation,VGetPanelInformation.    *
 *      Int Funcs : v_display_panel,v_create_threedpanel,               *
 *                  v_check_scale_in_panel_area,v_rearrange_cmds,       *
 *                  v_assign_panel_struct,v_highlight_threedpanel_item, *
 *                  v_remove_panel,v_create_threedscale,v_display_scale,*
 *                  v_set_scale,v_display_dual_scale,                   *
 *                  v_compute_scale_value.                              *
 *                                                                      *
 ************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include "Vlibrary.h"
#include "3dv.h"

static int event_in_q ;
int group_spacing_factor=0;
unsigned long PANEL_COLOR;
int SCALE_CURSOR_HEIGHT, SCALE_CURSOR_YLOC;
int SMALLSCALE_CURSOR_HEIGHT, SMALLSCALE_CURSOR_YLOC;

static int v_assign_panel_struct ( PanelCmdInfo* temp, PanelCmdInfo* temp1 );
static int v_check_scale_in_panel_area ( void );
static int v_compute_scale_value ( int id, int curx, float* gap, int* opt );
static int v_create_threedpanel ( Drawable drawable );
static int v_display_dual_scale ( int opt, int id, float* gap );
static int v_highlight_threedpanel_item ( void );
static int v_rearrange_cmds ( int num_of_cmds, PanelCmdInfo* cmds,
    int* new_num_of_cmds, PanelCmdInfo** new_cmds );
static int v_set_scale ( int id, int step, int steps, float* gap );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayPanel                                 *
 *      Description     : This function displays the panel in the       *
 *                        dialog window. If there is a memory allocation*
 *                        error, this function returns 1. If the scale  *
 *                        is on in the panel area or panel height is    *
 *                        out of the dialog window, this function will  *
 *                        not display panel in the panel area and return*
 *                        an appropriate error code. Otherwise this     *
 *                        function will check whether the dialog window *
 *                        is at the front of the image window. If it    *
 *                        is, display the panel with the specified      *
 *                        commands in the dialog window, else display   *
 *                        the panel into the temporary memory. This     *
 *                        function will return 0 for both situtation.   *
 *                        Each row of the panel contains 2 commands, so *
 *                        if you have 9 or 10 commands to be displayed  *
 *                        in the panel, then you will have five rows in *
 *                        the panel, and for the 9 commands case, you   *
 *                        will have the tenth command is displayed      *
 *                        blank. The maximum number of characters for   *
 *                        each command is 10. If the command is greater *
 *                        than 10 characters, this command will be      *
 *                        truncated.                                    *
 *                        Each command has one command type. If type is *
 *                        0, then this command is purely button         *
 *                        command, and just display command name in the *
 *                        proper panel item area. If type is 1, then    *
 *                        this command has command and switching value. *
 *                        This function will display command name and   *
 *                        highlight default switching value in the      *
 *                        proper panel item area. When you call this    *
 *                        function, you should check the error code     *
 *                        returned from this function for each process, *
 *                        because this function probably works for one  *
 *                        display screen, but for another display       *
 *                        screen, there is not enough space to display  *
 *                        panel commands. When this situation happens,  *
 *                        you should give the error message and exit    *
 *                        from the current process.                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         215 - can not display panel when scale is on *
 *                              in the panel area.                      *
 *                         242 - panel hieght is out of the dialog      *
 *                              window boundary.                        *
 *                         248 - panel is already on.                   *
 *                         254 - panel command has been truncated.      *
 *                         258 - pixmap is NULL and image window is at  *
 *                              the front.                              *
 *      Parameters      :  num_of_cmds - Specifies the number of        *
 *                              commands to be displayed in the panel.  * 
 *                         cmds - an array of struct PanelCmdInfo       *
 *                              which contains command name, command    *
 *                              type, number of items for switches, and *
 *                              the content of switches.                *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VCheckPanelEvent,VDeletePanel,                 *
 *                        VGetPanelInformation,VChangePanelItem.        *
 *      History         : Written on June 5, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VDisplayPanel ( int num_of_cmds, PanelCmdInfo* cmds )
{
        int i, j, row, max_width, width ;
        /* Drawable drawable ; */
        /*GC gc ; */
        int error, count, result, result1, avail_chars, new_num_of_cmds ;
        PanelCmdInfo *new_cmds ;
        char msg[80] ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayPanel.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (num_of_cmds <= 0) 
            v_print_fatal_error("VDisplayPanel",
                "The value of num_of_cmds should be greater than 0.",
                num_of_cmds) ;
        if (cmds == NULL) 
            v_print_fatal_error("VDisplayPanel",
                "The pointer of cmds should not be NULL.",0) ;
        for (i=0; i<num_of_cmds; i++) {
            if (cmds[i].type == 0) continue ;
            if (cmds[i].num_of_switches < 2) {
                sprintf(msg,"The number of switches of the %2d command",i) ;
                sprintf(msg,"%s should be greater than or equal to 2.",msg) ;
                v_print_fatal_error("VDisplayPanel",msg,cmds[i].num_of_switches) ;
            }
            if (cmds[i].switches == NULL) {
                sprintf(msg,"The pointer of switches of the %2d command",i) ;
                sprintf(msg,"%s should not be NULL.",msg) ;
                v_print_fatal_error("VDisplayPanle",msg,0) ;
            }
            if (cmds[i].switch_item_displayed < 0 || 
                cmds[i].switch_item_displayed >= cmds[i].num_of_switches) {
                sprintf(msg,"The range of switch_item_displayed of the %2d",i);
                sprintf(msg,"%s should be >= 0 && < %d.",msg,
                        lib_panel_cmds[i].num_of_switches) ;
                v_print_fatal_error("VDisplayPanel",msg,
                                  cmds[i].switch_item_displayed) ;
            }
        }

        error=v_rearrange_cmds(num_of_cmds,cmds,&new_num_of_cmds,&new_cmds) ;
        if (error != 0) return(error) ;
        
        result=0 ;
        row=new_num_of_cmds/LIB_MAX_PANEL_ITEMS_PER_ROW ;
        if ((new_num_of_cmds%LIB_MAX_PANEL_ITEMS_PER_ROW) != 0) row++ ;
        if (row > LIB_MAX_PANEL_ROWS) result=242 ;
        lib_panel_rows=row ;
        lib_panel_num_of_cmds=new_num_of_cmds ;
        lib_panel_cmds=(PanelCmdInfo *)malloc(new_num_of_cmds*
                                              sizeof(PanelCmdInfo)) ;
        if (lib_panel_cmds == NULL) return(1) ;
        lib_panel_switches=(PanelSwitchInfo *)malloc(new_num_of_cmds*
                                        sizeof(PanelSwitchInfo)) ;
        if (lib_panel_switches == NULL) return(1) ;
        for (i=0; i<new_num_of_cmds; i++) {
            count=strlen(new_cmds[i].cmd) ;
            if (count > MAX_PANEL_CHARS) {
                strncpy(lib_panel_cmds[i].cmd,new_cmds[i].cmd,MAX_PANEL_CHARS) ;
                lib_panel_cmds[i].cmd[MAX_PANEL_CHARS]='\0' ;
                result=254 ;
            }
            else strcpy(lib_panel_cmds[i].cmd,new_cmds[i].cmd) ;
            lib_panel_cmds[i].type=new_cmds[i].type ;
            lib_panel_cmds[i].group=new_cmds[i].group ;
            if (lib_panel_cmds[i].type == 0) {
                lib_panel_cmds[i].num_of_switches=0 ;
                lib_panel_cmds[i].switches=NULL ;
                lib_panel_cmds[i].switch_item_displayed= -1 ;
                lib_panel_switches[i].x= -1 ;
                lib_panel_switches[i].width= -1 ;
                continue ;
            }
            lib_panel_cmds[i].num_of_switches=new_cmds[i].num_of_switches ;
            lib_panel_cmds[i].switches=(Char30 *)malloc(sizeof(Char30)*
                                      new_cmds[i].num_of_switches);
            if (lib_panel_cmds[i].switches == NULL) return(1) ;
            if (result == 254) {
                for (j=0; j<new_cmds[i].num_of_switches; j++) 
                    strcpy(lib_panel_cmds[i].switches[j],"") ;
                lib_panel_cmds[i].switch_item_displayed= -1 ;
                lib_panel_switches[i].x= -1 ;
                lib_panel_switches[i].width= -1 ;
                continue ;
            }
            avail_chars=MAX_PANEL_CHARS-strlen(lib_panel_cmds[i].cmd)-1 ;
            for (j=0, max_width=0; j<new_cmds[i].num_of_switches; j++) {
                count=strlen(new_cmds[i].switches[j]) ;
                if (count > avail_chars) {
                    strncpy(lib_panel_cmds[i].switches[j],
                            new_cmds[i].switches[j],avail_chars) ;
                    lib_panel_cmds[i].switches[j][avail_chars]='\0' ;
                    result=254 ;
                }
                else strcpy(lib_panel_cmds[i].switches[j],
                            new_cmds[i].switches[j]) ;
                width=XTextWidth(lib_wins[1].font,lib_panel_cmds[i].switches[j],
                                 strlen(lib_panel_cmds[i].switches[j])) ;
                if (width > max_width) max_width=width ;
            }
            lib_panel_cmds[i].switch_item_displayed=
                                new_cmds[i].switch_item_displayed ;
            lib_panel_switches[i].x=XTextWidth(lib_wins[1].font,
                        lib_panel_cmds[i].cmd,strlen(lib_panel_cmds[i].cmd))+
                        lib_wins[1].font_width ;
            lib_panel_switches[i].width=max_width ;
        }

        /* free space for new_cmds */
        for (i=0; i<new_num_of_cmds; i++) 
            if (new_cmds[i].switches != NULL) free(new_cmds[i].switches) ;
        free(new_cmds) ;

        result1=v_display_panel(FALSE) ; 
        if (result1 == 0) return(result) ;
        else {v_draw_dialogwin_border();
              return(result1);}
}


/************************************************************************
 *                                                                      *
 *      Function        : v_display_panel                               *
 *      Description     : This function displays the panel in the       *
 *                        dialog window. It rearranges the panel        *
 *                        commands and writes them in the panel area.   *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  redisplay_panel_flag - indicates whether     *
 *                              to redisplay the panel or draw it for   *
 *                              the first time.                         *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel.                                *
 *      History         : Written on October 5, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
int v_display_panel ( int redisplay_panel_flag )
{
        Drawable drawable;
        GC gc;
        int i,j,k,yloc,error;
        int xloc,xloc2;
        int line_xloc;
        int switch_length;
        int group_spacing=0;

        XCharStruct overall;
        char text[100];
        int n,dum;
        short width,height,label_width,label_height;
        short but_height,but_width;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ;
            gc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == None) return(258) ;
            drawable=lib_wins[1].pixmap ;
            gc=lib_wins[1].pixmap_gc ;
        }
        if(lib_display_panel_on==1 && redisplay_panel_flag==FALSE)return(248);
        error=v_check_scale_in_panel_area() ;
        if (error != 0) return(error) ;

        XSetForeground(lib_display,gc,lib_reserved_colors[1].pixel) ;
        XFillRectangle(lib_display,drawable,gc,
                       LIB_PANEL_XLOC-(int)(lib_wins[1].font_width/4),
                       LIB_PANEL_YLOC,
                       LIB_PANEL_WIDTH+(int)(lib_wins[1].font_width/4),
                       lib_wins[1].height);

        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel) ;
        XSetBackground(lib_display,gc,lib_reserved_colors[1].pixel) ;
        LIB_PANEL_HEIGHT=LIB_PANEL_ITEM_HEIGHT*lib_panel_rows ;

        v_create_threedpanel(drawable);

        xloc=LIB_PANEL_XLOC;
        xloc2=LIB_PANEL_LINE_XLOC;

        height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        width=(short)LIB_PANEL_WIDTH/2; /*this is in pixels*/

        for (i=0, k=0; i<lib_panel_rows; i++, k += 2) {
            yloc=LIB_PANEL_YLOC+i*LIB_PANEL_ITEM_HEIGHT+group_spacing+1;

          if(lib_panel_cmds[k].type == 0) 
          {

                strcpy(text,lib_panel_cmds[k].cmd);
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
                label_y = (short)yloc + (height - label_height)/2 + 
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

          }

          if((k+1) < lib_panel_num_of_cmds && lib_panel_cmds[k+1].type == 0) 
          {
            /*if((k+1) < lib_panel_num_of_cmds) */
                if((k+1) == lib_panel_num_of_cmds)
                  strcpy(text,"");
                else strcpy(text,lib_panel_cmds[k+1].cmd);
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
 
                label_x = (short)xloc2 + (width - label_width)/2;
                label_y = (short)yloc + (height - label_height)/2 +
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

          }
            /****************handling switches****************/
                /*******left panel switch******/
            if (lib_panel_cmds[k].type != 0) 
            {
                j=(int)lib_panel_cmds[k].switch_item_displayed;
                switch_length=(int)((strlen(lib_panel_cmds[k].switches[j])+2)*lib_wins[1].font_width);

                line_xloc=xloc2-(lib_panel_switches[k].width + 4 + 4);
                XDrawLine(lib_display,lib_wins[1].win,gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT-1);
                XFlush(lib_display);
 
                /***displaying the cmd***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k].cmd);
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
 
                label_x = (short)xloc + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 + 
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
                /***displaying the switch***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k].switches[j]);
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
 
                label_x = (short)line_xloc + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

            }
                /*******right panel switch******/
            if ((k+1) < lib_panel_num_of_cmds && 
                lib_panel_cmds[k+1].type != 0) 
            {
                j=(int)lib_panel_cmds[k+1].switch_item_displayed;
                switch_length=(int)((strlen(lib_panel_cmds[k+1].switches[j])+2)*lib_wins[1].font_width);

                line_xloc=xloc+LIB_PANEL_WIDTH-
                          (lib_panel_switches[k+1].width + 4 + 4);
                XDrawLine(lib_display,lib_wins[1].win,gc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT-1);
                XFlush(lib_display);
 
                /***displaying the cmd***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(line_xloc - xloc2); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k+1].cmd);
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
 
                label_x = (short)xloc2 + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;

                XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
                /***displaying the switch***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(xloc+LIB_PANEL_WIDTH - line_xloc); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k+1].switches[j]);
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
 
                label_x = (short)line_xloc + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                      label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
            }

                if (i < lib_panel_rows-1) {
                j=i*2 ;
                if (lib_panel_cmds[j].group == lib_panel_cmds[j+2].group)
                    continue ;
                }
                group_spacing+=group_spacing_factor;
        }
        lib_display_panel_on=1 ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_create_threedpanel                          *
 *      Description     : This function displays the panel in the       *
 *                        dialog window. It creates the three-          *
 *                        dimensional buttons in the panels.            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  redisplay_panel_flag - indicates whether     *
 *                              to redisplay the panel or draw it for   *
 *                              the first time.                         *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel.                                *
 *      History         : Written on October 5, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
static int v_create_threedpanel ( Drawable drawable )
{
 
        GC threedpanelgc;
        XGCValues vals;
        int offset,xloc,xloc2,yloc;
        int i,j,group_spacing=0;
         
        threedpanelgc = XCreateGC(lib_display,lib_wins[1].win,0,&vals);

        vals.line_width=1;
        XChangeGC(lib_display,threedpanelgc,GCLineWidth,&vals);

        xloc=LIB_PANEL_XLOC;
        xloc2=LIB_PANEL_LINE_XLOC;
        offset=1;
        PANEL_COLOR = lib_reserved_colors[2].pixel;

        for (i=0; i<lib_panel_rows; i++) {
            yloc=LIB_PANEL_YLOC+(i+1)*LIB_PANEL_ITEM_HEIGHT+group_spacing ;

            /***********fill the rectangles***********/
            XSetForeground(lib_display,threedpanelgc,
                                PANEL_COLOR);
            /*first part of the panel button*/
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc,yloc-LIB_PANEL_ITEM_HEIGHT,
                           xloc2-xloc,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
            /*second part of the panel button*/
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc2,yloc-LIB_PANEL_ITEM_HEIGHT,
                           xloc+LIB_PANEL_WIDTH-xloc2,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
            /***********bottom dark lines***********/
            /*first part of the panel button*/
            XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc-offset,
                        xloc2-offset,
                        yloc-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2-offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset,
                        xloc2-offset,
                        yloc-offset);
            XFlush(lib_display);
            /*second part of the panel button*/
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc-offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc-offset);
            XFlush(lib_display);

          /***********top white lines***********/
            XSetForeground(lib_display,threedpanelgc,
                               lib_reserved_colors[4].pixel);

            /*first part of the panel button*/
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset,
                        xloc2-offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc-offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset,
                        xloc-offset,
                        yloc-offset);
            XFlush(lib_display);

            /*second part of the panel button*/
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc-LIB_PANEL_ITEM_HEIGHT+offset,
                        xloc2+offset,
                        yloc-offset);
            XFlush(lib_display);
        /***********************************************************/

            if (i < lib_panel_rows-1) {
                j=i*2 ;
                if(lib_panel_cmds[j].group == lib_panel_cmds[j+2].group)
                    continue ;

                else if(lib_panel_cmds[j].group != lib_panel_cmds[j+2].group)
                {
                   if(PANEL_COLOR == lib_reserved_colors[2].pixel)
                        PANEL_COLOR = lib_reserved_colors[10].pixel;
                   else if(PANEL_COLOR == lib_reserved_colors[10].pixel)
                        PANEL_COLOR = lib_reserved_colors[2].pixel;
                }

            group_spacing+=group_spacing_factor;
            XFlush(lib_display);
            }
         }

        return(0);
}
 

/************************************************************************
 *                                                                      *
 *      Function        : v_check_scale_in_panel_area                   *
 *      Description     : This function will check whether the scale is *
 *                        on in the panel area. If it is, this function *
 *                        will return error code 215. If it is not, this*
 *                        function will return 0.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         215 - scale is already in the panel area.    *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel.                                *
 *      History         : Written on April 15, 1991 by Hsiu-Mei Hung.   *
 *                                                                      *
 ************************************************************************/
static int v_check_scale_in_panel_area ( void )
{
        int i/*, xloc, width, height*/ ;

        for (i=0; i<MAX_SCALES; i++) {
            if (lib_display_scale_on[i] == 1) 
                if (lib_scale[i].x+lib_scale[i].width > LIB_PANEL_XLOC) 
                    return(215) ;
        }
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_rearrange_cmds                              *
 *      Description     : This function will check rearrange the        *
 *                        original commands to accomodate for odd/even  *
 *                        buttons in a group. If there are odd number   *
 *                        of buttons, one button is left unused.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel.                                *
 *      History         : Written on April 15, 1991 by Hsiu-Mei Hung.   *
 *                        Modified 11/9/94 group count fixed by Dewey Odhner
 *                        Modified 11/10/94 cmds not changed by Dewey Odhner
 *                                                                      *
 ************************************************************************/
static int v_rearrange_cmds ( int num_of_cmds, PanelCmdInfo* cmds,
    int* new_num_of_cmds, PanelCmdInfo** new_cmds )
{
        int i, j, k ;
        int min_group, max_group, num_of_groups;
        PanelCmdInfo temp, *temp0, *temp1 ;

        if(num_of_cmds == LIB_MAX_PANEL_ROWS*LIB_MAX_PANEL_ITEMS_PER_ROW)
                num_of_cmds=LIB_MAX_PANEL_ROWS*LIB_MAX_PANEL_ITEMS_PER_ROW;

        /*****compute the number of groups*****/
        min_group = max_group = cmds[0].group;
        for (i=1; i<num_of_cmds; i++) {
            if (cmds[i].group > max_group)
                max_group = cmds[i].group;
            if (cmds[i].group < min_group)
                min_group = cmds[i].group;
        }
        num_of_groups = max_group-min_group+1;

        temp0=(PanelCmdInfo *)malloc(num_of_cmds*sizeof(PanelCmdInfo));
        if (temp0 == NULL) return(1) ;
        temp1=(PanelCmdInfo *)malloc((LIB_MAX_PANEL_ROWS * 
                                     LIB_MAX_PANEL_ITEMS_PER_ROW+num_of_groups)
                                     *sizeof(PanelCmdInfo));
        if (temp1 == NULL) {
            free(temp0);
            return(1) ;
        }

        memcpy(temp0, cmds, num_of_cmds*sizeof(PanelCmdInfo));

        /* sorting the group type in increasing order */
        for (i=0; i<num_of_cmds-1; i++) {
            for (j=i+1; j<num_of_cmds; j++) {
                if (temp0[i].group > temp0[j].group) { /* exchange two 
                                                        structure contents */
                    v_assign_panel_struct(&temp,&temp0[i]) ;
                    v_assign_panel_struct(&temp0[i],&temp0[j]) ;
                    v_assign_panel_struct(&temp0[j],&temp) ;
                }
            }
        }

        /* number of items in the same group should be even, otherwise 
           padding one item with "" command name, button type */  
        for (i=j=0; i<num_of_cmds; i++) {
            if ((i != 0) && (temp0[i-1].group != temp0[i].group)) {
                if (j%2 == 1) {
                    temp1[j].group=temp0[i-1].group ;
                    strcpy(temp1[j].cmd,"") ;
                    temp1[j].type=0 ;
                    temp1[j].num_of_switches=0 ;
                    temp1[j].switches=NULL ;
                    temp1[j].switch_item_displayed= -1 ;
                    j++ ;
                }
            }
            temp1[j].group=temp0[i].group ;
            strcpy(temp1[j].cmd,temp0[i].cmd) ;
            temp1[j].type=temp0[i].type ;
            if (temp0[i].type==1) {
              temp1[j].num_of_switches=temp0[i].num_of_switches ;
              temp1[j].switches=(Char30 *)malloc(temp0[i].num_of_switches*
                                                 sizeof(Char30)) ;
              if (temp1[j].switches == NULL) return(1) ;
              for (k=0; k<temp0[i].num_of_switches; k++)
                strcpy(temp1[j].switches[k],temp0[i].switches[k]) ;
              temp1[j].switch_item_displayed=temp0[i].switch_item_displayed ;
            }
            else {
                temp1[j].num_of_switches=0;
                temp1[j].switches=NULL;
                temp1[j].switch_item_displayed= -1;
            }
            j++ ;
        }

        /* assign exactly number of items to *new_cmds and free temp1 */
        *new_num_of_cmds=j ;
        (*new_cmds)=(PanelCmdInfo *)malloc(*new_num_of_cmds*
                                           sizeof(PanelCmdInfo)) ;
        if (*new_cmds == NULL) return(1) ;

        for (i=0; i< *new_num_of_cmds; i++)
            v_assign_panel_struct(&(*new_cmds)[i],&temp1[i]) ;
        free(temp0);
        free(temp1) ;
        return(0) ;
} 


/************************************************************************
 *                                                                      *
 *      Function        : v_assign_panel_struct                         *
 *      Description     : This function will copy the contents of       *
 *                        temp1 structure into temp structure.          *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  temp - the structure to be copied into.      *
 *                         temp1 - the structure to be copied from.     *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel.                                *
 *      History         : Written on April 15, 1991 by Hsiu-Mei Hung.   *
 *                                                                      *
 ************************************************************************/
static int v_assign_panel_struct ( PanelCmdInfo* temp, PanelCmdInfo* temp1 )
{
        temp->group=temp1->group ;
        temp->type=temp1->type ;
        strcpy(temp->cmd,temp1->cmd) ;
        temp->num_of_switches=temp1->num_of_switches ;
        temp->switches=temp1->switches ;
        temp->switch_item_displayed=temp1->switch_item_displayed ;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VChangePanelItem                              *
 *      Description     : This function will change panel command name  *
 *                        for the specified command item. If the        *
 *                        switch_item is -1, this function will assume  *
 *                        the specified command is button type and only *
 *                        update the command part.                      *
 *                        If switch_item is not -1, this function will  *
 *                        assume the type of specified command is switch*
 *                        type and update the command and the switching *
 *                        value of the specified switch value item.     *
 *                        If the type of the specified command is not   *
 *                        the same as command displayed by function     *
 *                        VDisplayPanel, then there will be an error    *
 *                        occurrd. If the whole command field is greater*
 *                        than MAX_PANEL_CHARS (10 characters), then    *
 *                        this function will display first ten          *
 *                        characters, truncate the rest of command      *
 *                        field, and return error code 254.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         216 - panel is not displayed in the dialog   *
 *                              window.                                 *
 *                         253 - invalid panel command type.            *
 *                         254 - panel command has been truncated.      *
 *                         255 - the specified panel item cannot be     *
 *                              found.                                  *
 *                         258 - pixmap is NULL and image window is at  *
 *                              the front.                              *
 *      Parameters      :  old_cmd - the old command to be changed.     *
 *                         new_cmd - the command name to be changed.    *
 *                         switch_item - the value of switch item to be *
 *                              displayed.                              *
 *      Side effects    :  If switch_item is invalid, old_cmd or new_cmd*
 *                         is NULL, or function VCreateColormap is not  *
 *                         called prior to calling this function, it    *
 *                         will print a proper message to the standard  *
 *                         error stream, produce a core dump, and exit  *
 *                         from the current process.                    *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel, VDeletePanel,                   *
 *                        VGetPanelInformation, VCheckPanelEvent.       *
 *      History         : Written on June 10, 1991, by Hsiu-Mei Hung.   *
 *                                                                      *
 ************************************************************************/
int VChangePanelItem ( char* old_cmd, char* new_cmd, int switch_item )
{
        int result,i,j,k,yloc,xloc,xloc2,width,offset,count;
        int switch_item_displayed;
        int item,avail_chars,max_width;
        char msg[80];
        Drawable drawable; 
        GC threedpanelgc;
        int left_panel_button_clicked,right_panel_button_clicked;
        int line_xloc=0;
        unsigned long COLOR;
 
        XCharStruct overall;
        char text[100];
        int n,dum;
        short label_width,label_height;
        short but_height,but_width;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VChangePanelItem.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (old_cmd == NULL) 
            v_print_fatal_error("VChangePanelItem",
                "The pointer of old_cmd should not be NULL.",0) ;
        if (new_cmd == NULL) 
            v_print_fatal_error("VChangePanelItem",
                "The pointer of new_cmd should not be NULL.",0) ;
        if (switch_item < -1) {
            sprintf(msg,"The range of switch_item should be >= 0.") ;
            v_print_fatal_error("VChangePanelItem",msg,switch_item) ;
        }

        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ; 
            threedpanelgc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == None) return(258) ;
            drawable=lib_wins[1].pixmap ;
            threedpanelgc=lib_wins[1].pixmap_gc ;
        }
        if (lib_display_panel_on != 1) return(216) ;
        for (i=0; i<lib_panel_num_of_cmds; i++) {
            if (strcmp(old_cmd,lib_panel_cmds[i].cmd) == 0) {
                item=i ;
                break ;
            }
        }
        if (i == lib_panel_num_of_cmds) return(255) ;
        if (switch_item > lib_panel_cmds[item].num_of_switches) {
            sprintf(msg,"The range of switch_item should be >= 0 && < %d.",
                    lib_panel_cmds[item].num_of_switches) ;
            v_print_fatal_error("VChangePanelItem",msg,switch_item) ;
        }

        if (switch_item == -1 && lib_panel_cmds[item].type != 0) return(253) ;
        i=item/LIB_MAX_PANEL_ITEMS_PER_ROW ;
        yloc=LIB_PANEL_YLOC+i*LIB_PANEL_ITEM_HEIGHT+group_spacing_factor;
        i=item%LIB_MAX_PANEL_ITEMS_PER_ROW ;

        if (i == 0) {
                left_panel_button_clicked=TRUE;
                right_panel_button_clicked=FALSE;
        }
        else {
                right_panel_button_clicked=TRUE;
                left_panel_button_clicked=FALSE;
        }

        if(lib_panel_cmds[item].group % 2 == 0)
                COLOR = lib_reserved_colors[2].pixel;
        else    COLOR = lib_reserved_colors[10].pixel;

        xloc=LIB_PANEL_XLOC;
        xloc2=LIB_PANEL_LINE_XLOC;
        offset=1;

        switch_item_displayed=switch_item;

         /***********fill the rectangles***********/
            XSetForeground(lib_display,threedpanelgc,
                                COLOR);
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc,yloc,
                           xloc2-xloc,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
            /*second part of the panel button*/
            if(right_panel_button_clicked==TRUE)
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc2,yloc,
                           xloc+LIB_PANEL_WIDTH-xloc2,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
        /***********bottom dark lines***********/
            XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[3].pixel);
 
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset,
                        xloc2-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
            /*second part of the panel button*/
            else if(right_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc+offset,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }

         
          /***********top white lines***********/
            XSetForeground(lib_display,threedpanelgc,
                               lib_reserved_colors[4].pixel);
 
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
 
            /*second part of the panel button*/
            else if(right_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+offset,
                        xloc2+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
        /***********************************************************/
        
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
        XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
 
        /*********left panel button***********/
        if(left_panel_button_clicked==TRUE &&
           lib_panel_cmds[item].type==0)
        {
                count=strlen(new_cmd) ;
                if (count > MAX_PANEL_CHARS)
                {
                   strncpy(lib_panel_cmds[item].cmd,new_cmd,MAX_PANEL_CHARS);
                   lib_panel_cmds[item].cmd[MAX_PANEL_CHARS]='\0';
                   result=254;
                }
                else strcpy(lib_panel_cmds[item].cmd,new_cmd);
 
                strcpy(text,lib_panel_cmds[item].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
        }
 
        /*********left panel switch***********/
        if(left_panel_button_clicked==TRUE &&
           lib_panel_cmds[item].type!=0)
        {
                k=item;
                j=(int)lib_panel_cmds[k].switch_item_displayed;

                count=strlen(new_cmd) ;
                if (count > MAX_PANEL_CHARS) 
                {
                   strncpy(lib_panel_cmds[item].cmd,new_cmd,MAX_PANEL_CHARS);
                   lib_panel_cmds[item].cmd[MAX_PANEL_CHARS]='\0';
                   result=254;
                }
                else strcpy(lib_panel_cmds[item].cmd,new_cmd);

                avail_chars=MAX_PANEL_CHARS-
                                strlen(lib_panel_cmds[item].cmd)-1;
               for(j=0,max_width=0;j<lib_panel_cmds[item].num_of_switches;j++)          {
                    count=strlen(lib_panel_cmds[item].switches[j]);
                    if (count > avail_chars) 
                    {
                        lib_panel_cmds[item].switches[j][avail_chars]='\0' ;
                        result=254 ;
                    }
                    width=XTextWidth(lib_wins[1].font,
                             lib_panel_cmds[item].switches[j],
                             strlen(lib_panel_cmds[item].switches[j]));
                    if (width > max_width) max_width=width;
                }

                lib_panel_cmds[item].switch_item_displayed=switch_item; 
                lib_panel_switches[item].x=
                    XTextWidth(lib_wins[1].font,lib_panel_cmds[item].cmd,
                    strlen(lib_panel_cmds[item].cmd))+lib_wins[1].font_width;
                lib_panel_switches[item].width=max_width;

 
                line_xloc=xloc2-(lib_panel_switches[k].width + 4 + 4);
                XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT-1);
                XFlush(lib_display);
 
                /***displaying the cmd***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(line_xloc - xloc); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
                /***displaying the switch***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/
 
                switch_item_displayed=lib_panel_cmds[k].switch_item_displayed;
 
                strcpy(text,lib_panel_cmds[k].switches[switch_item_displayed])
;
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
        }
 
        /*********right panel button***********/
        if(right_panel_button_clicked==TRUE &&
           lib_panel_cmds[item].type==0)
        {
                count=strlen(new_cmd) ;
                if (count > MAX_PANEL_CHARS)
                {
                   strncpy(lib_panel_cmds[item].cmd,new_cmd,MAX_PANEL_CHARS);
                   lib_panel_cmds[item].cmd[MAX_PANEL_CHARS]='\0';
                   result=254;
                }
                else strcpy(lib_panel_cmds[item].cmd,new_cmd);

                strcpy(text,lib_panel_cmds[item].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                label_x = (short)xloc2 + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
        }
 
        /*********right panel switch***********/
        if(right_panel_button_clicked==TRUE &&
           lib_panel_cmds[item].type!=0)
        {
                k=item;
                j=(int)lib_panel_cmds[k].switch_item_displayed;
        
                count=strlen(new_cmd) ;
                if (count > MAX_PANEL_CHARS)
                {
                   strncpy(lib_panel_cmds[item].cmd,new_cmd,MAX_PANEL_CHARS);
                   lib_panel_cmds[item].cmd[MAX_PANEL_CHARS]='\0';
                   result=254;
                }
                else strcpy(lib_panel_cmds[item].cmd,new_cmd);
 
                avail_chars=MAX_PANEL_CHARS-
                                strlen(lib_panel_cmds[item].cmd)-1;
               for(j=0,max_width=0;j<lib_panel_cmds[item].num_of_switches;j++)
                {
                    count=strlen(lib_panel_cmds[item].switches[j]);
                    if (count > avail_chars)
                    {
                        lib_panel_cmds[item].switches[j][avail_chars]='\0' ;
                        result=254 ;
                    }
                    width=XTextWidth(lib_wins[1].font,
                             lib_panel_cmds[item].switches[j],
                             strlen(lib_panel_cmds[item].switches[j]));
                    if (width > max_width) max_width=width;
                }
 
                lib_panel_cmds[item].switch_item_displayed=switch_item;
                lib_panel_switches[item].x=
                    XTextWidth(lib_wins[1].font,lib_panel_cmds[item].cmd,
                    strlen(lib_panel_cmds[item].cmd))+lib_wins[1].font_width;
                lib_panel_switches[item].width=max_width;

 
                line_xloc=xloc+LIB_PANEL_WIDTH-
                          (lib_panel_switches[k].width + 4 + 4);
                XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT-1);
                XFlush(lib_display);
 
                /***displaying the cmd***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(line_xloc - xloc2); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                label_x = (short)xloc2 + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
                /***displaying the switch***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(xloc+LIB_PANEL_WIDTH - line_xloc); /*this is
 in pixels*/
 
                switch_item_displayed=lib_panel_cmds[k].switch_item_displayed;
 
                strcpy(text,lib_panel_cmds[k].switches[switch_item_displayed])
;
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
        }

        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckPanelEvent                              *
 *      Description     : This function will allow user to push LEFT or *
 *                        MIDDLE button to select the command he wants  *
 *                        and return the command user selects to        *
 *                        cmd_selected, and the switching value to      *
 *                        switch_selected if command type is 1.         *
 *                        If the type of the command user selected is 0,*
 *                        then when LEFT or MIDDLE button is pushed,    *
 *                        this function will highlight the command      *
 *                        user selects, remove the hightlight, and      *
 *                        return selected command to *cmd_selected, and *
 *                        NULL string to *switch_selected. If the type  *
 *                        of the command user selects is 1, then when   *
 *                        LEFT button is pushed, this function will     *
 *                        switch to one higher value, when MIDDLE button*
 *                        is pushed, this function will switch to one   *
 *                        lower value, will not highlight command itself*
 *                        instead of displaying new switch value,       *
 *                        and return the selected switching value to    *
 *                        *switch_selected and corresponding command to *
 *                        *cmd_selected.                                *
 *                        If there is an error happens, this function   *
 *                        will return NULL string to cmd_selected and   *
 *                        *switch_selected. If the dialog window is not *
 *                        at front of the image window, this function   *
 *                        will return 214, if the event type is not the *
 *                        ButtonPress or the LEFT/MIDDLE button is not  *
 *                        pressed, this function will return 204, if    *
 *                        event does not happen in the dialog window,   *
 *                        this function will return 205, if tha panel   *
 *                        does not display in the dialog window, this   *
 *                        function will return 216, or if the           *
 *                        LEFT/MIDDLE button is pressed and the cursor  *
 *                        is not within panel, this function will return*
 *                        206.                                          *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         205 - the window in which event happens is   *
 *                              invalid.                                *
 *                         206 - the cursor is not within the command.  *
 *                         214 - the dialog window is not displayed at  *
 *                              the front the image window.             *
 *                         216 - the panel is not displayed in dialog   *
 *                              window.                                 *
 *      Parameters      :  cmd_selected - Returns a character string of *
 *                              the command to be selected.             *
 *                         switch_selected - Returns a character string *
 *                              of the switching value to be selected.  *
 *      Side effects    : The related window is dialog window.          *
 *                        The related event is ButtonPress.             *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel,VDeletePanel,                    *
 *                        VGetPanelInformation,VChangePanelItem.        *
 *      History         : Written on June 5, 1991 by Hsiu_Mei Hung.     *
 *                        Modified on October 21, 1992 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VCheckPanelEvent ( XEvent* event, char cmd_selected[80],
    char switch_selected[80] )
{
        int curx, cury, i, j, item ;
        /*XEvent event1 ; */

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VCheckPanelEvent.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (cmd_selected == NULL)
            v_print_fatal_error("VCheckPanelEvent",
                "The pointer of cmd_selected should not be NULL.",
                0) ;
        if (switch_selected == NULL)
            v_print_fatal_error("VCheckPanelEvent",
                "The pointer of switch_selected should not be NULL.",
                0) ;

        strcpy(cmd_selected,"") ;
        strcpy(switch_selected,"") ;
        if (event->type != ButtonPress) return(204) ;
        if (event->type == ButtonPress && event->xbutton.button == 3) 
            return(204) ;
        if (event->xbutton.window != lib_wins[1].win) return(205) ;
        if (!lib_dial_win_front) return(214) ;
        if (lib_display_panel_on == 0) return(216) ;
        curx=event->xbutton.x ;
        cury=event->xbutton.y ;
        if (curx < LIB_PANEL_XLOC || curx > LIB_PANEL_XLOC+LIB_PANEL_WIDTH || 
            cury < LIB_PANEL_YLOC || 
            cury > LIB_PANEL_YLOC+lib_panel_rows*LIB_PANEL_ITEM_HEIGHT) {
            lib_panel_item_selected= -1 ;
            return(206) ;
        }
        i=(cury-LIB_PANEL_YLOC)/LIB_PANEL_ITEM_HEIGHT ;
        j=(curx-LIB_PANEL_XLOC)/(LIB_PANEL_LINE_XLOC-LIB_PANEL_XLOC) ;
        lib_panel_item_selected = i*LIB_MAX_PANEL_ITEMS_PER_ROW+j ;
        item=lib_panel_item_selected ;
        if (item < lib_panel_num_of_cmds && item >= 0) {
            if (lib_panel_cmds[item].type == 1) {
                if (event->xbutton.button == 1) {
                    lib_panel_cmds[item].switch_item_displayed++ ;
                    if (lib_panel_cmds[item].switch_item_displayed >=
                        lib_panel_cmds[item].num_of_switches)
                        lib_panel_cmds[item].switch_item_displayed=0 ;
                }
                else {
                    lib_panel_cmds[item].switch_item_displayed-- ;
                    if (lib_panel_cmds[item].switch_item_displayed < 0)
                        lib_panel_cmds[item].switch_item_displayed=
                            lib_panel_cmds[item].num_of_switches-1 ;
                }
            }
            v_highlight_threedpanel_item() ;
            strcpy(cmd_selected,lib_panel_cmds[item].cmd) ;
            if (lib_panel_cmds[item].num_of_switches != 0) {
                i=lib_panel_cmds[item].switch_item_displayed ;
                strcpy(switch_selected,lib_panel_cmds[item].switches[i]) ;
            }
            return(0) ;
        } 
        else {
            lib_panel_item_selected = -1 ;
            return(206) ;
        }
}


/************************************************************************
 *                                                                      *
 *      Function        : v_highlight_threedpanel_item                  *
 *      Description     : This function finds out which button in the   *
 *                        was selected and highlights that button for   *
 *                        sometime and then resores the color of the    *
 *                        button.                                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         206 - cursor is not within the command area. *
 *                         258 - Pixmap of dialog window is NULL and    *
 *                              image window is at front.               *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VSelectPanelPanelItem.                        *
 *      History         : Written on October 21, 1992 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_highlight_threedpanel_item ( void )
{
        Drawable drawable;
        GC threedpanelgc;
        XGCValues vals;
        int i,j,k,yloc,xloc,xloc2,offset;
        int switch_item_displayed;
        GC gc;
        int left_panel_button_clicked,right_panel_button_clicked;
        int left_panel_midpoint,right_panel_midpoint,offset_from_midpoint;
        int new_left_xloc,new_right_xloc,line_xloc;
        int switch_length;
        unsigned long COLOR;
 
        XCharStruct overall;
        char text[100];
        int n,dum;
        short label_width,label_height;
        short but_height,but_width;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ;
            gc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == None) return(258) ;
            drawable=lib_wins[1].pixmap ;
            gc=lib_wins[1].pixmap_gc ;
        }

        if (lib_panel_item_selected == -1) return(206);
        gc=lib_wins[1].gc ;
        i=lib_panel_item_selected/LIB_MAX_PANEL_ITEMS_PER_ROW ;
        yloc=LIB_PANEL_YLOC+i*LIB_PANEL_ITEM_HEIGHT;
        i=lib_panel_item_selected%LIB_MAX_PANEL_ITEMS_PER_ROW ;

        threedpanelgc = XCreateGC(lib_display,lib_wins[1].win,0,&vals);
        threedpanelgc=lib_wins[1].gc; /*to overcome gc problems*/
        vals.line_width=1;
        XChangeGC(lib_display,threedpanelgc,GCLineWidth,&vals);

         if (i == 0) {
                left_panel_button_clicked=TRUE;
                right_panel_button_clicked=FALSE;
         }
         else {
                right_panel_button_clicked=TRUE;
                left_panel_button_clicked=FALSE;
         }

        if(lib_panel_cmds[lib_panel_item_selected].group % 2 == 0)
                COLOR = lib_reserved_colors[2].pixel;
        else    COLOR = lib_reserved_colors[10].pixel;


        /*group correction for yloc*/
        yloc+=group_spacing_factor*lib_panel_cmds[lib_panel_item_selected].group;
        xloc=LIB_PANEL_XLOC;
        xloc2=LIB_PANEL_LINE_XLOC;
        offset=1;
 
        but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
        but_width=(short)LIB_PANEL_WIDTH/2; /*this is in pixels*/

        left_panel_midpoint=(xloc2-xloc)/2;
        right_panel_midpoint=(xloc+LIB_PANEL_WIDTH-xloc2)/2;
 
        offset_from_midpoint=
                (int)(strlen(lib_panel_cmds[lib_panel_item_selected].cmd)*
                lib_wins[1].font_width/2);

        new_left_xloc=xloc + left_panel_midpoint - offset_from_midpoint;
        new_right_xloc=xloc2 + right_panel_midpoint - offset_from_midpoint;

        /*show that the button is pressed*/
         /***********fill the rectangles***********/
            XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[3].pixel);
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc,yloc,
                           xloc2-xloc,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
            /*second part of the panel button*/
            if(right_panel_button_clicked==TRUE)
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc2,yloc,
                           xloc+LIB_PANEL_WIDTH-xloc2,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
         /***********bottom white lines***********/
            XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[2].pixel);

            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset,
                        xloc2-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
            /*second part of the panel button*/
            else if(right_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc+offset,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            } 
          /***********top dark lines***********/
            XSetForeground(lib_display,threedpanelgc,
                               lib_reserved_colors[4].pixel);
 
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
            /*second part of the panel button*/
            else if(right_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+offset,
                        xloc2+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
        
        XFlush(lib_display);

        /*write the text*/
/*
        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel);
        if(left_panel_button_clicked==TRUE)
        XDrawString(lib_display,lib_wins[1].win,gc,new_left_xloc,
                         yloc+lib_wins[1].font->ascent,
                         lib_panel_cmds[lib_panel_item_selected].cmd,
                         strlen(lib_panel_cmds[lib_panel_item_selected].cmd));
        XFlush(lib_display);
        if(right_panel_button_clicked==TRUE)
        XDrawString(lib_display,lib_wins[1].win,gc,new_right_xloc,
                         yloc+lib_wins[1].font->ascent,
                         lib_panel_cmds[lib_panel_item_selected].cmd,
                         strlen(lib_panel_cmds[lib_panel_item_selected].cmd));
        XFlush(lib_display);
*/

        /***********************************/
        /*do a microsleep*/
        VSleep(100000);
        /***********************************/

        /*redraw the original button*/
         /***********fill the rectangles***********/
            XSetForeground(lib_display,threedpanelgc,
                                COLOR);
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc,yloc,
                           xloc2-xloc,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
            /*second part of the panel button*/
            if(right_panel_button_clicked==TRUE)
            XFillRectangle(lib_display,lib_wins[1].win,threedpanelgc,
                           xloc2,yloc,
                           xloc+LIB_PANEL_WIDTH-xloc2,LIB_PANEL_ITEM_HEIGHT);
            XFlush(lib_display);
        /***********bottom dark lines***********/
            XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[3].pixel);

            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset,
                        xloc2-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2-offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
            /*second part of the panel button*/
            else if(right_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc+offset,
                        xloc+LIB_PANEL_WIDTH+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
        
          /***********top white lines***********/
            XSetForeground(lib_display,threedpanelgc,
                               lib_reserved_colors[4].pixel);
 
            /*first part of the panel button*/
            if(left_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc+offset,
                        yloc+offset,
                        xloc2-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc-offset,
                        yloc+offset,
                        xloc-offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
 
            /*second part of the panel button*/
            else if(right_panel_button_clicked==TRUE)
            {
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+offset,
                        xloc+LIB_PANEL_WIDTH-offset,
                        yloc+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        xloc2+offset,
                        yloc+offset,
                        xloc2+offset,
                        yloc+LIB_PANEL_ITEM_HEIGHT-offset);
            XFlush(lib_display);
            }
        /***********************************************************/

        XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);

        /*********left panel button***********/
        if(left_panel_button_clicked==TRUE && 
           lib_panel_cmds[lib_panel_item_selected].type==0)
        {
                strcpy(text,lib_panel_cmds[lib_panel_item_selected].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

        }

        /*********left panel switch***********/
        if(left_panel_button_clicked==TRUE && 
           lib_panel_cmds[lib_panel_item_selected].type!=0)
        {
                k=lib_panel_item_selected;
                j=(int)lib_panel_cmds[k].switch_item_displayed;
                switch_length=(int)((strlen(lib_panel_cmds[k].switches[j])+2)*lib_wins[1].font_width);
                
                line_xloc=xloc2-(lib_panel_switches[k].width + 4 + 4);
                XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT-1);
                XFlush(lib_display);
 
                /***displaying the cmd***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(line_xloc - xloc); /*this is in pixels*/

                strcpy(text,lib_panel_cmds[k].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;

                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
                /***displaying the switch***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(xloc2 - line_xloc); /*this is in pixels*/

                switch_item_displayed=lib_panel_cmds[k].switch_item_displayed;

                strcpy(text,lib_panel_cmds[k].switches[switch_item_displayed]);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

        }

        /*********right panel button***********/
        if(right_panel_button_clicked==TRUE &&
           lib_panel_cmds[lib_panel_item_selected].type==0)
        {
                strcpy(text,lib_panel_cmds[lib_panel_item_selected].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                label_x = (short)xloc2 + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid; 
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
        }

        /*********right panel switch***********/
        if(right_panel_button_clicked==TRUE &&
           lib_panel_cmds[lib_panel_item_selected].type!=0)
        {
                k=lib_panel_item_selected;
                j=(int)lib_panel_cmds[k].switch_item_displayed;

                line_xloc=xloc+LIB_PANEL_WIDTH-
                          (lib_panel_switches[k].width + 4 + 4);
                XDrawLine(lib_display,lib_wins[1].win,threedpanelgc,
                        line_xloc,
                        yloc,
                        line_xloc,
                        yloc+LIB_PANEL_ITEM_HEIGHT-1);
                XFlush(lib_display);
 
                /***displaying the cmd***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(line_xloc - xloc2); /*this is in pixels*/
 
                strcpy(text,lib_panel_cmds[k].cmd);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
 
                label_x = (short)xloc2 + (but_width - label_width)/2;
                label_y = (short)yloc + (but_height - label_height)/2 +
                                         label_ascent;
 
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;

                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);
 
                /***displaying the switch***/
                but_height=(short)LIB_PANEL_ITEM_HEIGHT; /*this is in pixels*/
                but_width=(short)(xloc+LIB_PANEL_WIDTH - line_xloc); /*this is
 in pixels*/

                switch_item_displayed=lib_panel_cmds[k].switch_item_displayed;
 
                strcpy(text,lib_panel_cmds[k].switches[switch_item_displayed]);
                n = strlen(text);
                if(n>0)
                {
                        XQueryTextExtents(lib_display,
                            XGContextFromGC(threedpanelgc),
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
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,threedpanelgc,
                        lib_reserved_colors[0].pixel);
                XDrawText(lib_display,lib_wins[1].win,threedpanelgc,
                  label_x,label_y,&tlabel,1);
                XFlush(lib_display);

        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDeletePanel                                   *
 *      Description     : This function will erase the panel from the   *
 *                        dialog window. If the panel is not displayed  *
 *                        in the dialog window, this function will      *
 *                        return 216. Otherwise this function will erase*
 *                        the panel from the dialog window if the dialog*
 *                        window is at the front of the image window    *
 *                        or from the temporary memory called pixmap if *
 *                        the image window is at the front, and return 0*
 *                        to this function.                             *
 *      Return Value    :  0 - work successfully.                       *
 *                         216 - the panel is not displayed in the      *
 *                              dialog window.                          *
 *                         258 - pixmap is null and image window is at  *
 *                              the front.                              *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel,VCheckPanelEvent,               *
 *                        VGetPanelInformation,VChangePanelItem.        *
 *      History         : Written on November 15, 1989 by Hsiu-Mei Hung.*
 *                        Modified on Mar 30, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
int VDeletePanel ( void )
{
        int i ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDeletePanel.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (!lib_display_panel_on) return(216) ;

        for (i=0; i<lib_panel_num_of_cmds; i++) 
            if (lib_panel_cmds[i].switches != NULL)
                free(lib_panel_cmds[i].switches) ;
        free(lib_panel_cmds) ;
        free(lib_panel_switches) ;

        lib_panel_item_selected= -1 ;
        v_remove_panel() ;
        v_draw_dialogwin_border();
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_remove_panel                                *
 *      Description     : This function will remove the panel from      *
 *                        the dialog window.                            *
 *      Return Value    :  0 - work successfully.                       *
 *                         258 - pixmap is null and image window is at  *
 *                              the front.                              *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is dialog window.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDeletePanel.                                  *
 *      History         : Written on November 15, 1989 by Hsiu-Mei Hung.*
 *                                                                      *
 ************************************************************************/
int v_remove_panel ( void ) 
{
        Drawable drawable ;
        GC gc ;
        int background, foreground ;
 
        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ;
            gc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == None) return(258) ;
            drawable=lib_wins[1].pixmap ;
            gc=lib_wins[1].pixmap_gc ;
        }
        foreground=lib_reserved_colors[0].pixel ; /* text color */
        background=lib_reserved_colors[1].pixel ; /* dialogue window bg */
        XSetForeground(lib_display,gc,background) ;
        XFillRectangle(lib_display,drawable,gc,
                       LIB_PANEL_XLOC-(int)(lib_wins[1].font_width/4),
                       LIB_PANEL_YLOC,
                       LIB_PANEL_WIDTH+(int)(lib_wins[1].font_width/4),
                       lib_wins[1].height) ;
        XSetForeground(lib_display,gc,foreground) ;
        lib_display_panel_on=0 ;
        LIB_PANEL_HEIGHT=0 ;
        XFlush(lib_display);
      return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayScale                                 *
 *      Description     : This function will display default (the lowest*
 *                        resolution) scale in the dialog window.       *
 *                        This function will check if the dialog window *
 *                        is at the front of the image window. If it is,*
 *                        then display the default resolution scale     *
 *                        (the lowest one) with the specified value,    *
 *                        and the related values (label, min, max,      *
 *                        width, x and y location) specified by function*
 *                        VSetScaleInformation in the dialog window,    *
 *                        else display the scale into the temporary     *
 *                        memory.                                       *
 *                        When you call this function, you should       *
 *                        check the error code returned from this       *
 *                        function for each process, because this       *
 *                        function probably works for one display       *
 *                        screen, but for another display screen, there *
 *                        is not enough space to display scale.         *
 *                        When this situation happens, you should give  *
 *                        the error message and exit from the current   *
 *                        process.                                      *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         217 - can not display scale when panel is on *
 *                              in the panel area.                      *
 *                         243 - scale is out of the dialog window      *
 *                              boundary.                               *
 *                         244 - scale does not exist.                  *
 *                         246 - scale width is greater than the        *
 *                              available width of the dialog window.   *
 *                         247 - scale width  is too small to put       *
 *                              minimum requirement.                    *
 *                         249 - scale is already on.                   *
 *                         258 - pixmap is NULL and image window is     *
 *                              at the front.                           *
 *      Parameters      :  None.                                        *
 *      Side effects    : The related window is dialog window.          *
 *                        If label is NULL, then this function will     *
 *                        print the proper message in the standard      *
 *                        output device, produce the core dump file,    *
 *                        and exit from the current process.            *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCheckScaleEvent, VUndisplayScale,               *
 *                        VGetScaleInformation, VSetScaleInformation,   *
 *                        VGetScaleLayoutInformation,                   *
 *                        VCheckScaleEvents.                            *
 *      History         : Written on May 30, 1991 by Hsiu-Mei Hung.     *
 *                        Modified on November 20, 1992 by Krishna Iyer.*
 *                        Modified on March 30, 1993 by Krishna Iyer.   *
 *                        Modified on 8/25/94 window size corrected
 *                        bu Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayScale ( double value, char* label )
{

        unsigned long valuemask ; 
        XSetWindowAttributes xswa ;
        int i, id, avail_width;
        Drawable drawable, drawable1 ;
        GC gc, gc1;


        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayScale.\n") ;
            printf("Programmer must call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (label == NULL) 
            v_print_fatal_error("VDisplayScale",
                "The parameter \"label\" should not be NULL.", (int)label) ;

        for (i=0, id= -1; i<lib_num_of_scales; i++) {
            if (strcmp(lib_scale[i].label,label) == 0) {
                id=i ;
                break ;
            }
        }
        if (id == -1) return(244) ;
        if (lib_display_scale_on[id] == 1) return(249) ;
        if (lib_scale[id].width == 0)  return(246) ;
        if (lib_scale[id].width == -1) return(247) ;
        if (lib_display_panel_on == 1) {
            avail_width=lib_wins[1].width-LIB_PANEL_WIDTH+1 ; 
            if ((lib_scale[id].x+lib_scale[id].width) > avail_width) 
                return(217) ;
        }
        else {
            avail_width=lib_wins[1].width ; 
            if ((lib_scale[id].x+lib_scale[id].width) > avail_width) 
                return(243) ;
        }
        if ((lib_scale[id].y+lib_scale[id].height) > lib_wins[1].height) 
            return(243) ;
        
        valuemask=0 ;
        xswa.override_redirect=True ;
        valuemask |= CWOverrideRedirect ;
        xswa.event_mask=ExposureMask | ButtonPressMask ;
        valuemask |= CWEventMask ;
        xswa.background_pixel=lib_reserved_colors[1].pixel ;
        valuemask |= CWBackPixel ;
        xswa.border_pixel=lib_reserved_colors[0].pixel ;
        valuemask |= CWBorderPixel ;

        lib_scale[id].win=XCreateWindow(lib_display,lib_wins[1].win,
                lib_scale[id].win_x,lib_scale[id].win_y,
                lib_scale[id].win_width, lib_scale[id].win_height,
                0,lib_depth,InputOutput,CopyFromParent,valuemask,&xswa) ;

        lib_scale[id].pixmap=XCreatePixmap(lib_display,
                        lib_scale[id].win,lib_scale[id].win_width,
                        lib_scale[id].win_height,lib_depth) ;
        XMapSubwindows(lib_display,lib_wins[1].win) ; 
        XMapWindow(lib_display,lib_wins[1].win) ; 

        v_create_threedscale(id);

        valuemask=0 ;
        lib_scale[id].gc=lib_wins[1].gc ;
        XSetForeground(lib_display,lib_scale[id].gc,
                       lib_reserved_colors[0].pixel);
        XSetBackground(lib_display,lib_scale[id].gc,
                       lib_reserved_colors[1].pixel);
        lib_scale[id].pixmap_gc=lib_wins[1].pixmap_gc;
        if (lib_scale[id].gc != NULL) {
            XSetForeground(lib_display,lib_scale[id].pixmap_gc,
                           lib_reserved_colors[1].pixel);
            XSetBackground(lib_display,lib_scale[id].pixmap_gc,
                           lib_reserved_colors[0].pixel);
            XFillRectangle(lib_display,lib_scale[id].pixmap,
                           lib_scale[id].pixmap_gc,0,0,lib_scale[id].win_width,
                           lib_scale[id].win_height);
            XSetForeground(lib_display,lib_scale[id].pixmap_gc,
                           lib_reserved_colors[0].pixel);
            XSetBackground(lib_display,lib_scale[id].pixmap_gc,
                           lib_reserved_colors[1].pixel);
        }

        lib_scale[id].value=value;
        lib_scale[id].cursor_x=(lib_scale[id].value-lib_scale[id].low_min)*
                               (lib_scale[id].win_width-1)/
                               (lib_scale[id].low_max-lib_scale[id].low_min);
        if (lib_dial_win_front) {
            drawable=lib_scale[id].win ;
            gc=lib_scale[id].gc ;
            drawable1=lib_wins[1].win ;
            gc1=lib_wins[1].gc ;
        }
        else {
            if (lib_scale[id].pixmap == None) return(258) ;
            drawable=lib_scale[id].pixmap ;
            gc=lib_scale[id].pixmap_gc ;
            drawable1=lib_wins[1].pixmap ;
            gc1=lib_wins[1].pixmap_gc ;
        }
        v_display_scale(drawable,gc,drawable1,gc1,id) ;
        lib_display_scale_on[id]=1 ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_create_threedscale                          *
 *      Description     : This function displays the scale in the       *
 *                        dialog window. It creates the three-          *
 *                        dimensional scales.                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  id - the ID of the scale to be displayed.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayScale.                                *
 *      History         : Written on November 20, 1992 by Krishna Iyer. *
 *                        Modified 8/25/94 GC freed by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int v_create_threedscale ( int id )
{

        GC threedscalegc;
        XGCValues vals;
        int xloc,yloc;

        XClearArea(lib_display,lib_scale[id].win,0,0,
                        lib_scale[id].win_width,
                        lib_scale[id].win_height,False);
        XFlush(lib_display);

        threedscalegc = XCreateGC(lib_display,lib_scale[id].win,0,&vals);

        xloc = 0; yloc = 0;

        XSetForeground(lib_display,threedscalegc,
                          lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,lib_scale[id].win,threedscalegc,
                       xloc,yloc,
                       lib_scale[id].win_width,
                       lib_scale[id].win_height);

        /***********bottom white lines***********/
        vals.line_width=3;
        XChangeGC(lib_display,threedscalegc,GCLineWidth,&vals);
        XSetForeground(lib_display,threedscalegc,
                               lib_reserved_colors[4].pixel);
        XDrawLine(lib_display,lib_scale[id].win,threedscalegc,
                        xloc,
                        lib_scale[id].win_height-1,
                        lib_scale[id].win_width-1,
                        lib_scale[id].win_height-1);
        XDrawLine(lib_display,lib_scale[id].win,threedscalegc,
                        lib_scale[id].win_width-1,
                        yloc,
                        lib_scale[id].win_width-1,
                        lib_scale[id].win_height-1);

         /***********top dark lines********/
        vals.line_width=3;
        XChangeGC(lib_display,threedscalegc,GCLineWidth,&vals);
        XSetForeground(lib_display,threedscalegc,
                               lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,lib_scale[id].win,threedscalegc,
                        xloc,
                        yloc,
                        lib_scale[id].win_width-1,
                        yloc);
        XDrawLine(lib_display,lib_scale[id].win,threedscalegc,
                        xloc,
                        yloc,
                        xloc,
                        lib_scale[id].win_height-1);

        XFreeGC(lib_display, threedscalegc);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_display_scale                               *
 *      Description     : This function will create the scale in the    *
 *                        dialog window.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  drawable - the window/pixmap ID of scale.    *
 *                         gc - the GC of window/pixmap.                *
 *                         drawable1 - the window ID to be drawn in(here*
 *                              it is dialog window).                   *
 *                         gc1 - the GC of the window to be drawn in.   *
 *                         id - the ID of the scale.                    *
 *      Side effects    : None.                                         *
 *      Entry condition : LIB_SCALE_HEIGHT, lib_scale[id], lib_display,
 *                        lib_wins[1], lib_reserved_colors,
 *      Related funcs   : VDisplayScale.                                *
 *      History         : Written on April 15, 1991 by Hsiu-Mei Hung.   *
 *                        Modified on Novemebr 20, 1992 by Krishna Iyer.*
 *                        Modified on March 30, 1993 by Krishna Iyer.   *
 *                        Modified 8/25/94 decimal 0 special case
 *                        removed by Dewey Odhner.
 *                        Modified 3/17/95 float passed to sprintf
 *                        cast to double by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_display_scale ( Drawable drawable, GC gc,
    Drawable drawable1, GC gc1, int id )
{
        GC threedscalegc;
        XGCValues vals;
        char min1[80], max1[80], value1[80], max_width ;

        SCALE_CURSOR_YLOC = (LIB_SCALE_HEIGHT/3)/8;
        SCALE_CURSOR_HEIGHT=(LIB_SCALE_HEIGHT/3)-SCALE_CURSOR_YLOC*2.5;
        SMALLSCALE_CURSOR_YLOC = (LIB_SCALE_HEIGHT/3)/6;
        SMALLSCALE_CURSOR_HEIGHT=(LIB_SCALE_HEIGHT/3)-SMALLSCALE_CURSOR_YLOC*3;

        sprintf(min1,lib_scale[id].fmt, (double)lib_scale[id].low_min) ;
        sprintf(max1,lib_scale[id].fmt, (double)lib_scale[id].low_max) ;
        sprintf(value1,lib_scale[id].fmt, (double)lib_scale[id].value) ;

        if (atof(value1)>atof(max1) || atof(value1)<atof(min1)) {
            printf("The error occurred in the function VDisplayScale.\n");
            printf("Please note that the value is out of scale ");
            printf("range assigned by function VSetScaleInformation.\n");
            kill(getpid(),LIB_EXIT) ;
        }

        threedscalegc = XCreateGC(lib_display,drawable1,0,&vals);

        XDrawString(lib_display,drawable1,gc1,lib_scale[id].label_x,
                    lib_scale[id].label_y+lib_wins[1].font->ascent,
                    lib_scale[id].label,strlen(lib_scale[id].label)) ;
        XDrawString(lib_display,drawable1,gc1,lib_scale[id].low_min_x,
                    lib_scale[id].low_min_y+lib_wins[1].font->ascent,
                    min1,strlen(min1)) ;
        XDrawString(lib_display,drawable1,gc1,lib_scale[id].low_max_x,
                    lib_scale[id].low_max_y+lib_wins[1].font->ascent,
                    max1,strlen(max1)) ;
        max_width=XTextWidth(lib_wins[1].font,max1,strlen(max1))+2; 


        /*******************3d scale value*****************/
        XSetForeground(lib_display,threedscalegc,
                               lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,drawable1,threedscalegc,
                       lib_scale[id].value_x-1,
                       lib_scale[id].value_y,max_width+1,
                       lib_wins[1].font_height);

        vals.line_width=2;
        XChangeGC(lib_display,threedscalegc,GCLineWidth,&vals);

        /***********bottom dark lines***********/
        XSetForeground(lib_display,threedscalegc,
                               lib_reserved_colors[0].pixel);

        XDrawLine(lib_display,drawable1,threedscalegc,
                        lib_scale[id].value_x-1,
                        lib_scale[id].value_y+lib_wins[1].font_height,
                        lib_scale[id].value_x-1+max_width+1,
                        lib_scale[id].value_y+lib_wins[1].font_height);
        XDrawLine(lib_display,drawable1,threedscalegc,
                        lib_scale[id].value_x-1+max_width+1,
                        lib_scale[id].value_y,
                        lib_scale[id].value_x-1+max_width+1,
                        lib_scale[id].value_y+lib_wins[1].font_height);

        /***********top white lines********/
        XSetForeground(lib_display,threedscalegc,
                               lib_reserved_colors[4].pixel);

        XDrawLine(lib_display,drawable1,threedscalegc,
                        lib_scale[id].value_x-1,
                        lib_scale[id].value_y,
                        lib_scale[id].value_x-1+max_width+1,
                        lib_scale[id].value_y);
        XDrawLine(lib_display,drawable1,threedscalegc,
                        lib_scale[id].value_x-1,
                        lib_scale[id].value_y,
                        lib_scale[id].value_x-1,
                        lib_scale[id].value_y+lib_wins[1].font_height);

        /**********end 3d scale value creation*********/

        /*XDrawRectangle(lib_display,drawable1,gc1,lib_scale[id].value_x-1,
                       lib_scale[id].value_y,max_width+1,
                       lib_wins[1].font_height) ;
        */      
        XDrawString(lib_display,drawable1,gc1,lib_scale[id].value_x,
                    lib_scale[id].value_y+lib_wins[1].font->ascent,
                    value1,strlen(value1)) ;

        /*****************scale cursor creation*******************/
        XSetForeground(lib_display,gc,lib_reserved_colors[10].pixel);
        XFillRectangle(lib_display,drawable,gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,
                        SCALE_CURSOR_HEIGHT,
                        SCALE_CURSOR_HEIGHT);

        /*******top white lines********/
        XSetForeground(lib_display,gc,lib_reserved_colors[4].pixel);
        XDrawLine(lib_display,drawable,gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC);
        XDrawLine(lib_display,drawable,gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,      
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT);

        /*******bottom dark lines*******/
        XSetForeground(lib_display,gc,lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,drawable,gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT,      
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT);
        XDrawLine(lib_display,drawable,gc,
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,      
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT);
        
        /*******the line in the middle********/ 
        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel);
        XDrawLine(lib_display,drawable,gc,lib_scale[id].cursor_x,
                        SCALE_CURSOR_YLOC+1,
                        lib_scale[id].cursor_x,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT-1);

        XFreeGC(lib_display, threedscalegc);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckScaleEvent                             *
 *      Description     : This function will allow user to select scale *
 *                        value if the LEFT button is pushed and cursor *
 *                        is within the scale. The cursor will confine  *
 *                        to the scale window. Moving cursor is to      *
 *                        select scale value. Pushing the LEFT button is*
 *                        to get one step higher resolution scale,      *
 *                        pushing MIDDLE button is to get one step lower*
 *                        resolution, and pushing RIGHT button is to    *
 *                        quit. When the x pointer is oving along       *
 *                        horizontal direction, corresponding selected  *
 *                        value will be displayed in the box at the     *
 *                        upper right corner of the scale and the       *
 *                        corresponding cursor (vertical bar) will be   *
 *                        displayed in the scale.                       *
 *                        This function will compute the resolution     *
 *                        steps for scale and if the resolution step    *
 *                        user specifies is higher than the highest     *
 *                        resolution this function defines, this        *
 *                        function will assign he current resolution    *
 *                        step is the highest one, and so does the      *
 *                        lowest resolution. The width of the high      *
 *                        resolution scale is half of the scale width.  *
 *                        This function allow user to pass an user-     *
 *                        defined function to decide what action should *
 *                        be taken when the scale value is selected.    *
 *                        The specified function update will be called  *
 *                        each time a selected scale is updated, with   *
 *                        the arguments value selected by the end user  *
 *                        and args. Application programmer should call  *
 *                        function VCheckScaleEvents in the user-defined*
 *                        function update once for a while to check     *
 *                        whether there is an event (LEFT or MIDDLE     *
 *                        button pushed or MotionNotify) waiting in the *
 *                        event queue to interrupt the current action.  *
 *                        If it is, then function update should be      *
 *                        interrupted and return to this function,      *
 *                        otherwise continue executing the function     *
 *                        update.                                       *
 *                        The type of the value selected by end user is *
 *                        decided by the argument decimal of the        *
 *                        function VSetScaleInformation. If decimal is  *
 *                        0, then this function will return the value of*
 *                        integer rounded to nearest the selected value.*
 *                        If the decimal is not 0, this function will   *
 *                        return the floating point number.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         204 - the type of event is invalid.          *
 *                         205 - the window in which event happens      *
 *                              is invalid.                             *
 *                         214 - the dialog window is not displayed at  *
 *                              the front the image window.             *
 *                         218 - the specified scale is not displayed in*
 *                              dialog window.                          *
 *                         244 - the scale does not exist.              *
 *      Parameters      :  event - Specifies the current event removed  *
 *                              from the event queue.                   *
 *                         label - Specifies a scale label which is to  *
 *                              be used.                                *
 *                         update - user-defined function to determine  *
 *                              what action should be taken when the    *
 *                              scale value is changed.                 *
 *                         args - Specifies the user-specified arguments*
 *                              that will be passed to the update       *
 *                              procedure.                              *
 *                         value - Returns the value user selects from  *
 *                              the scale.                              *
 *      Side effects    : The related window is dialog window.          *
 *                        The related event is ButtonPress.             *
 *                        If the label or value is NULL, this function  *
 *                        will print the proper message in the standard *
 *                        output device, produce the core dump file,    *
 *                        and exit from the current process.            *
 *      Entry condition : The scale should be displayed with VDisplayScale.
 *                        VCreateColormap must be called first.
 *      Related funcs   : VDisplayScale,VUndisplayScale,                    *
 *                        VGetScaleInformation, VSetScaleInformation,   *
 *                        VGetScaleLayoutInformation,                   *
 *                        VCheckScaleEvents.                            *
 *      History         : Written on October 5, 1992 by Krishna Iyer.   *
 *                        Modified 8/24/94 to warp cursor by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int VCheckScaleEvent ( XEvent* event, char* label, int (*update)(), void* args,
    float* value )
{
        XSetWindowAttributes xswa;
        int id, step, i, opt;
        XEvent event1,report;
        int curx,high_width,steps;
        float gap[50],max_res;
        char msg[3][80];

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VCheckScaleEvent.\n");
            printf("Programmer must call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (value == NULL) 
            v_print_fatal_error("VCheckScaleEvent",
                "The parameter \"value\" should not be NULL.", (int)value) ;
        if (label == NULL) 
            v_print_fatal_error("VCheckScaleEvent",
                "The parameter \"label\" should not be NULL.", (int)label) ;

        for (i=0, id= -1; i<lib_num_of_scales; i++) {
            if (strcmp(label,lib_scale[i].label) == 0) {
                id=i ;
                break ;
            }
        }
        if (id == -1) return(244) ;
        if (!lib_dial_win_front) return(214) ;
        if (!lib_display_scale_on[id]) return(218) ;
        if (event->type != ButtonPress || event->xbutton.button != 1) 
            return(204) ;
        if (event->xbutton.window != lib_scale[id].win) return(205) ;
        XGrabPointer(lib_display,lib_scale[id].win,False,
                     ButtonPressMask | PointerMotionMask | 
                     ButtonReleaseMask,GrabModeAsync,GrabModeAsync,
                     lib_scale[id].win,None,CurrentTime) ;
        high_width=lib_scale[id].high_width ;
        max_res=(lib_scale[id].low_max-lib_scale[id].low_min)/
                (high_width*high_width) ;
        gap[0]=(lib_scale[id].low_max-lib_scale[id].low_min)/
               (lib_scale[id].win_width-1) ;
        for (steps=0; (gap[steps]/2.) >= max_res; steps++) 
            gap[steps+1]=gap[steps]/2. ;
        step=lib_scale[id].step ;
        for (i=0; i<3; i++) strcpy(msg[i],lib_butt_msg[i]) ;
        VDisplayButtonAction("UP\n","DOWN\n","DONE\n") ;
        while (True) {
            VNextEvent(&event1) ;
            event_in_q=XEventsQueued(lib_display,QueuedAlready) ;
            switch (event1.type) {
                case ButtonPress :
                    if (event1.xbutton.button == 1) {
                        if (step < steps) step++ ;
                        v_set_scale(id,step,steps,gap) ;
                        v_display_dual_scale(1,id,gap) ;
                        XWarpPointer(lib_display, lib_scale[id].win,
                            lib_scale[id].win,0,0,0,0, lib_scale[id].cursor_x,
                            SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT/2);
                        XFlush(lib_display);
                    }
                    if (event1.xbutton.button == 2) {
                        if (step > 0) step-- ;
                        v_set_scale(id,step,steps,gap) ;
                        v_display_dual_scale(1,id,gap) ;
                        XWarpPointer(lib_display, lib_scale[id].win,
                            lib_scale[id].win,0,0,0,0, lib_scale[id].cursor_x,
                            SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT/2);
                        XFlush(lib_display);
                    }
                    if (event1.xbutton.button == 3) {
                        *value=lib_scale[id].value ;
                        XUngrabPointer(lib_display,CurrentTime) ;
                        xswa.event_mask=ExposureMask | ButtonPressMask;
                        XChangeWindowAttributes(lib_display,lib_scale[id].win,
                                                CWEventMask,&xswa) ;
                        VDisplayButtonAction(msg[0],msg[1],msg[2]) ;
                        return(0) ;
                    }
                    break ;
                case MotionNotify :
                    curx=event1.xmotion.x ;
                    if (event_in_q == 0) {
                        v_compute_scale_value(id,curx,gap,&opt) ;
                        v_display_dual_scale(opt,id,gap) ;
                        if (update != NULL) 
                            (*update)(lib_scale[id].value,args) ;
                    }
                    else {
                        XPeekEvent(lib_display,&report) ;
                        if (report.type != MotionNotify) {
                            v_compute_scale_value(id,curx,gap,&opt) ;
                            v_display_dual_scale(opt,id,gap) ;
                            if (update != NULL)
                                (*update)(lib_scale[id].value,args) ;
                        }
                    }
                    break ;
            }
        }
}

/************************************************************************
 *                                                                      *
 *      Function        : v_set_scale                                   *
 *      Description     : This function will compute the min and max    *
 *                        values, string locations, and x, y location   *
 *                        of rectangle, and x thumb location (cursor_x)
 *                        for high resolution scale.
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  id - the identification of the scale.        *
 *                         step - the current scale resolution step.    *
 *                         steps - the total number of scale resolution *
 *                              steps.                                  *
 *                         gap - an array of the interval between two   *
 *                              pixels of each scale resolution.        *
 *      Side effects    : None.                                         *
 *      Entry condition : lib_scale[id].high_width, lib_scale[id].low_max,
 *                        lib_scale[id].low_min, lib_scale[id].decimal,
 *                        lib_scale[id].value, lib_scale[id].fmt,
 *                        lib_scale[id].win_width,
 *                        lib_wins[1].font must be properly set.
 *      Related funcs   : VCheckScaleEvent.                            *
 *      History         : Written on June 1, 1991 by Hsiu-Mei Hung.     *
 *                        Modified on May 30, 1993 by Krishna Iyer.     *
 *                        Modified 8/24/94 restored commented-out code
 *                           to set cursor_x by Dewey Odhner.
 *                        Modified 8/29/94 decimal 0 special case
 *                        removed by Dewey Odhner.
 *                        Modified 8/30/94 high_max value corrected
 *                        by Dewey Odhner.
 *                        Modified 3/17/95 float passed to sprintf
 *                        cast to double by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_set_scale ( int id, int step, int steps, float* gap )
{
        int high_width,width, high_min_pix;
        float low_res,high_min;
        char min1[80];

        if (step >= steps) step=steps-1;
        if (step <= 0) {
            lib_scale[id].step=0;
            lib_scale[id].high_min_x= -1;
            /*lib_scale[id].low_min_x= -1;*/
            lib_scale[id].high_x= -1;
            lib_scale[id].cursor_x=
                       rint((lib_scale[id].value-lib_scale[id].low_min)/
                       (lib_scale[id].low_max-lib_scale[id].low_min)*
                       (lib_scale[id].win_width-1));
            return (0);
        }
        high_width=lib_scale[id].high_width ;
        low_res=(lib_scale[id].low_max-lib_scale[id].low_min-
                gap[step]*(high_width-1))/
                (lib_scale[id].win_width-high_width);
        high_min=lib_scale[id].value-gap[step]*((high_width-1)/2);
        high_min_pix=rint((high_min-lib_scale[id].low_min)/low_res);
        high_min=lib_scale[id].low_min+high_min_pix*low_res;
        if (high_min <= lib_scale[id].low_min) {
            lib_scale[id].high_min=lib_scale[id].low_min ;
            lib_scale[id].high_x=0 ;
        }
        else
            if (high_min >= lib_scale[id].low_max-gap[step]*(high_width-1)) {
                lib_scale[id].high_min=lib_scale[id].low_max-
                                       gap[step]*(high_width-1);
                lib_scale[id].high_x=lib_scale[id].win_width-high_width ;
            }
            else {
                lib_scale[id].high_min=high_min ;
                lib_scale[id].high_x=high_min_pix;
            }
        sprintf(min1,lib_scale[id].fmt, (double)lib_scale[id].high_min);
        width=XTextWidth(lib_wins[1].font,min1,strlen(min1));
        lib_scale[id].high_min_x=lib_scale[id].win_x+lib_scale[id].high_x-
                                 width;
        lib_scale[id].high_max_x=lib_scale[id].high_min_x+width+high_width;
        lib_scale[id].high_max=lib_scale[id].high_min+gap[step]*(high_width-1);
        if (lib_scale[id].high_max > lib_scale[id].low_max) 
            lib_scale[id].high_max=lib_scale[id].low_max;
        lib_scale[id].cursor_x=lib_scale[id].high_x+rint((lib_scale[id].value-
                               lib_scale[id].high_min)/gap[step]);
        lib_scale[id].step=step;
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_display_dual_scale                          *
 *      Description     : This function will display the single or dual *
 *                        scale in the dialog window and according      *
 *                        to the resolution step user specifies.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  opt - 0/1 to indicate if scale subwindow     *
 *                              should be cleared and the min max string*
 *                              of high resolution scale should be      *
 *                              repainted.                              *
 *                         id - the identification of the scale.        *
 *                         gap - an array of the interval between two   *
 *                              pixels of each scale resolution.        *
 *      Side effects    : The pointer may be moved to the thumb center.
 *      Entry condition : VSetScaleInformation must be called first.
 *      Related funcs   : VCheckScaleEvent.                            *
 *      History         : Written on June 1, 1991 by Hsiu-Mei Hung.     *
 *                        Modified on March 30, 1993 by Krishna Iyer.   *
 *                        Modified 8/29/94 decimal 0 special case
 *                        removed by Dewey Odhner.
 *                        Modified 3/17/95 float passed to sprintf
 *                        cast to double by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_display_dual_scale ( int opt, int id, float* gap )
{
        char min1[80], max1[80], value1[80], max2[80];
        int max_width;

        if (opt == 1) {
            XClearArea(lib_display,lib_scale[id].win,0,0,
                     lib_scale[id].win_width,
                     lib_scale[id].win_height,
                     False);
            XClearArea(lib_display,lib_wins[1].win,
                        lib_scale[id].low_min_x-lib_wins[1].font_width*5,
                        lib_scale[id].high_min_y,
                        lib_scale[id].width+lib_wins[1].font_width*5,
                        lib_wins[1].font_height,False);

                v_create_threedscale(id);
        }
        if (lib_scale[id].step == 0) {
            sprintf(value1,lib_scale[id].fmt, (double)lib_scale[id].value) ;
            sprintf(min1,lib_scale[id].fmt, (double)lib_scale[id].low_min) ;
            sprintf(max1,lib_scale[id].fmt, (double)lib_scale[id].low_max) ;
            max_width=XTextWidth(lib_wins[1].font,max1,strlen(max1)); 
            XClearArea(lib_display,lib_wins[1].win,lib_scale[id].value_x,
                       lib_scale[id].value_y+1,max_width,
                       lib_wins[1].font_height-2,False) ;
            XSetForeground(lib_display,lib_scale[id].gc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,lib_wins[1].win,lib_wins[1].gc,
                        lib_scale[id].value_x,lib_scale[id].value_y+1,
                        max_width,lib_wins[1].font_height-2);
            if (opt == 0) 
                XClearArea(lib_display,lib_scale[id].win,0,0,
                           lib_scale[id].win_width,lib_scale[id].win_height,
                           False) ;

                v_create_threedscale(id);

                /***********scale cursor creation***********/
            XSetForeground(lib_display,lib_scale[id].gc,
                        lib_reserved_colors[10].pixel);
            XFillRectangle(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,
                        SCALE_CURSOR_HEIGHT,
                        SCALE_CURSOR_HEIGHT);

                /*******top white lines********/
                XSetForeground(lib_display,lib_scale[id].gc,
                                lib_reserved_colors[4].pixel);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,      
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,      
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT);
 
                /*******bottom dark lines*******/
                XSetForeground(lib_display,lib_scale[id].gc,
                                lib_reserved_colors[3].pixel);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT,
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC,      
                        lib_scale[id].cursor_x+SCALE_CURSOR_HEIGHT/2,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT);
 
                /*******the line in the middle********/
            XSetForeground(lib_display,lib_scale[id].gc,
                        lib_reserved_colors[0].pixel);
            XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x,
                        SCALE_CURSOR_YLOC+1,
                        lib_scale[id].cursor_x,
                        SCALE_CURSOR_YLOC+SCALE_CURSOR_HEIGHT-1);

                /*********end cursor creation**********/ 

            XDrawString(lib_display,lib_wins[1].win,lib_wins[1].gc,
                        lib_scale[id].value_x,
                        lib_scale[id].value_y+lib_wins[1].font->ascent,
                        value1,strlen(value1));
            return (0);
        }
        sprintf(value1,lib_scale[id].fmt, (double)lib_scale[id].value) ;
        sprintf(min1,lib_scale[id].fmt, (double)lib_scale[id].high_min) ;
        sprintf(max1,lib_scale[id].fmt, (double)lib_scale[id].high_max) ;
        sprintf(max2,lib_scale[id].fmt, (double)lib_scale[id].low_max) ;
        max_width=XTextWidth(lib_wins[1].font,max2,strlen(max2)); 
        XClearArea(lib_display,lib_wins[1].win,lib_scale[id].value_x,
                   lib_scale[id].value_y+1,max_width,
                   lib_wins[1].font_height-2,
                   False) ;
        v_create_threedscale(id);

        /*********the second scale inside the first*************/
        XSetForeground(lib_display,lib_scale[id].gc,
                       lib_reserved_colors[5].pixel);
        XFillRectangle(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].high_x,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].high_width,
                        SMALLSCALE_CURSOR_HEIGHT);
        /******top dark lines*******/
        XSetForeground(lib_display,lib_scale[id].gc,
                       lib_reserved_colors[3].pixel);
        XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].high_x,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].high_x,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT);
        XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].high_x,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].high_x+lib_scale[id].high_width,
                        SMALLSCALE_CURSOR_YLOC);
        /********bottom white lines*********/
        XSetForeground(lib_display,lib_scale[id].gc,
                       lib_reserved_colors[4].pixel);
        XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT,
                        lib_scale[id].high_x+lib_scale[id].high_width,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT);
        XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].high_x+lib_scale[id].high_width,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].high_x+lib_scale[id].high_width,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT);


            /***********scale cursor creation***********/
            XSetForeground(lib_display,lib_scale[id].gc,
                        lib_reserved_colors[10].pixel);
            XFillRectangle(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC,
                        SMALLSCALE_CURSOR_HEIGHT,
                        SMALLSCALE_CURSOR_HEIGHT);

                   /*******top white lines********/
                XSetForeground(lib_display,lib_scale[id].gc,
                                lib_reserved_colors[4].pixel);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].cursor_x+SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].cursor_x-SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT);
 
                /*******bottom dark lines*******/
                XSetForeground(lib_display,lib_scale[id].gc,
                                lib_reserved_colors[3].pixel);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x-SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT,
                        lib_scale[id].cursor_x+SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT);
                XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x+SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC,
                        lib_scale[id].cursor_x+SMALLSCALE_CURSOR_HEIGHT/2,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT);
 
                /*******the line in the middle********/
            XSetForeground(lib_display,lib_scale[id].gc,
                        lib_reserved_colors[0].pixel);
            XDrawLine(lib_display,lib_scale[id].win,lib_scale[id].gc,
                        lib_scale[id].cursor_x,
                        SMALLSCALE_CURSOR_YLOC+1,
                        lib_scale[id].cursor_x,
                        SMALLSCALE_CURSOR_YLOC+SMALLSCALE_CURSOR_HEIGHT-1);
 
                /*********end cursor creation**********/

        XSetForeground(lib_display,lib_scale[id].gc,
                       lib_reserved_colors[0].pixel) ;
        XDrawString(lib_display,lib_wins[1].win,lib_wins[1].gc,
                    lib_scale[id].value_x,
                    lib_scale[id].value_y+lib_wins[1].font->ascent,
                    value1,strlen(value1)) ;
        if (opt == 1) {
            XDrawString(lib_display,lib_wins[1].win,lib_wins[1].gc,
                       lib_scale[id].high_min_x,
                       lib_scale[id].high_min_y+lib_wins[1].font->ascent,
                       min1,strlen(min1)) ;
            XDrawString(lib_display,lib_wins[1].win,lib_wins[1].gc,
                       lib_scale[id].high_max_x,
                       lib_scale[id].high_max_y+lib_wins[1].font->ascent,
                       max1,strlen(max1)) ;
        }
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_compute_scale_value                         *
 *      Description     : This function will compute the selected value *
 *                        according to the cursor position and the      *
 *                        resolution step user specifies, and change    *
 *                        the min and max values of the high resolution *
 *                        scale if it is necessary.                     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  id - the identification of the scale.        *
 *                         curx - the x location of the current pointer.*
 *                         gap - an array of the interval between two   *
 *                              pixels of each scale resolution.        *
 *                         opt - returns 0/1 to indicate if scale       *
 *                              subwindow should be clearned and the min*
 *                              max string of high resolution scale     *
 *                              should be repainted.                    *
 *      Side effects    : None.                                         *
 *      Entry condition : The scale must be displayed with VDisplayScale.
 *      Related funcs   : VCheckScaleEvent.                            *
 *      History         : Written on June 1, 1991 by Hsiu-Mei Hung.     *
 *                        Modified on May 30, 1993 by Krishna Iyer.     *
 *                        Modified 8/29/94 decimal 0 special case
 *                        removed by Dewey Odhner.
 *                        Modified 3/17/95 float passed to sprintf
 *                        cast to double by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_compute_scale_value ( int id, int curx, float* gap, int* opt )
{
        int step, width ;
        float low_res ;
        char min1[80] ;

        if (curx < 0) curx=0 ;
        if (curx >= lib_scale[id].win_width) curx=lib_scale[id].win_width-1;
        lib_scale[id].cursor_x=curx ;
        if (lib_scale[id].step == 0) {
            sprintf(min1,lib_scale[id].fmt,
                (double)lib_scale[id].low_min+curx*gap[0]);
            lib_scale[id].value=atof(min1);
            *opt =0 ;
            return 0;  //gjg: ask dewey?
        }
        step=lib_scale[id].step ;
        low_res=(lib_scale[id].low_max-lib_scale[id].low_min-
                 (lib_scale[id].high_width-1)*gap[step])/
                (lib_scale[id].win_width-lib_scale[id].high_width);
        if (curx >= lib_scale[id].high_x && 
                curx < (lib_scale[id].high_x+lib_scale[id].high_width)) {
            sprintf(min1,lib_scale[id].fmt, (double)lib_scale[id].high_min+
                                (curx-lib_scale[id].high_x)*gap[step]);
            *opt=0 ;
        }
        else {
            if (curx < lib_scale[id].high_x) {
                lib_scale[id].high_min=lib_scale[id].low_min+curx*low_res;
                lib_scale[id].high_x=curx ;
                sprintf(min1,lib_scale[id].fmt,(double)lib_scale[id].high_min);
                width=XTextWidth(lib_wins[1].font,min1,strlen(min1)) ;
                lib_scale[id].high_min_x=lib_scale[id].win_x+curx-width ;
                lib_scale[id].high_max_x=lib_scale[id].win_x+curx+
                                         lib_scale[id].high_width ;
                lib_scale[id].high_max=lib_scale[id].high_min+
                                       (lib_scale[id].high_width-1)*gap[step] ;
            }
            else {
                lib_scale[id].high_max=lib_scale[id].low_max-low_res*
                                       (lib_scale[id].win_width-curx-1);
                lib_scale[id].high_x=curx-(lib_scale[id].high_width-1);
                lib_scale[id].high_min=lib_scale[id].high_max-
                                       (lib_scale[id].high_width-1)*gap[step] ;
                sprintf(min1,lib_scale[id].fmt,(double)lib_scale[id].high_min);
                width=XTextWidth(lib_wins[1].font,min1,strlen(min1)) ;
                lib_scale[id].high_min_x=lib_scale[id].win_x+
                                         lib_scale[id].high_x-width ;
                lib_scale[id].high_max_x=lib_scale[id].win_x+
                                         lib_scale[id].high_x+
                                         lib_scale[id].high_width ;
                sprintf(min1,lib_scale[id].fmt,(double)lib_scale[id].high_max);
            }
            *opt=1 ;
        }
        lib_scale[id].value=atof(min1);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckScaleEvents                             *
 *      Description     : This function will check whether the next     *
 *                        event in the event queue is MotionNotify, or  *
 *                        Left or iddle button pushed by end user. If   *
 *                        it is, then this function will return 1 to    *
 *                        *event_waiting, else return 0 to              *
 *                        *event_waiting.                               *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  event_waiting - returns 1 to indicate there  *
 *                              is an event waiting, and 0 for no event *
 *                              waiting.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayScale,VCheckScaleEvent,              *
 *                        VGetScaleInformation, VSetScaleInformation,   *
 *                        VGetScaleLayoutInformation, VUndisplayScale.      *
 *      History         : Written on June 3, 1991 by Hsiu-Mei Hung.     *
 *                        Modified on Novemebr 20, 1992 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
void VCheckScaleEvents ( int* event_waiting )
{
        XEvent report ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VCheckScaleEvents.\n");
            printf("Please call VCreateColormap before ");
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        *event_waiting=0 ;
        if (event_in_q == 0) return ;
        XPeekEvent(lib_display,&report) ;
        if (report.type == MotionNotify) *event_waiting=1 ;
        if (report.type == ButtonPress && (report.xbutton.button == 1 ||
            report.xbutton.button == 2)) *event_waiting=1 ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VUndisplayScale                                   *
 *      Description     : This function will erase the specified scale  *
 *                        from the dialog window.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         218 - the specified scale is not displayed   *
 *                              in dialog window.                       *
 *                         244 - the scale does not exist.              *
 *                         258 - pixmap is NULL and image window is at  *
 *                              the front.                              *
 *      Parameters      :  label - Specifies a scale label to be erased.*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayScale,VCheckScaleEvent,              *
 *                        VGetScaleInformation, VSetScaleInformation,   *
 *                        VGetScaleLayoutInforamtion,                   *
 *                        VCheckScaleEvents.                            *
 *      History         : Written on August 1, 1990 by Hsiu-Mei Hung.   *
 *                        Modified on March 30, 1993 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
int VUndisplayScale ( char* label )
{
        int i,id,offset;
        Drawable drawable ;
        GC gc ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VUndisplayScale.\n") ;
            printf("Please call VCreateColormap before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (label == NULL) 
            v_print_fatal_error("VUndisplayScale",
                "The pointer of label should not be NULL.", (int)label) ;
        for (i=0, id= -1; i<lib_num_of_scales; i++) {
            if (strcmp(label,lib_scale[i].label) == 0) {
                id=i ;
                break ;
            }
        }
        if (id == -1) return(244) ;
        if (!lib_display_scale_on[id]) return(218) ;
        if (lib_scale[id].pixmap != None)
            XFreePixmap(lib_display,lib_scale[id].pixmap) ;
        XUnmapWindow(lib_display,lib_scale[id].win) ;
        XDestroyWindow(lib_display,lib_scale[id].win) ;
XFlush(lib_display);
        lib_display_scale_on[id]=0 ;
        lib_scale[id].cursor_x= -1 ;
        lib_scale[id].cursor_y= 0 ;
        lib_scale[id].high_min_x= -1 ;
        lib_scale[id].high_min_y=lib_scale[id].low_min_y+LIB_SCALE_HEIGHT/3 ;
        lib_scale[id].high_max_x= -1 ;
        lib_scale[id].high_max_y=lib_scale[id].high_min_y ;
        lib_scale[id].high_x= -1 ;
        lib_scale[id].high_y= 0 ;
        lib_scale[id].step=0 ;
        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ;
            gc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == None) return(258) ;
            drawable=lib_wins[1].pixmap ;
            gc=lib_wins[1].pixmap_gc ;
        }

        offset=-2;
        XSetForeground(lib_display,gc,lib_reserved_colors[1].pixel) ;
        XFillRectangle(lib_display,drawable,gc,
                       lib_scale[id].x,
                       lib_scale[id].y+offset,
                       lib_scale[id].width,
                       lib_scale[id].height) ;
XFlush(lib_display);
        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel) ;

        v_draw_dialogwin_border();

        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VDeleteScale                                  *
 *      Description     : This function will remove the specified scale *
 *                        from memory.                                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         244 - the scale does not exist.              *
 *                         259 - cannot remove scale when this scale is *
 *                              on.                                     *
 *      Parameters      :  label - Specifies a scale label to be        *
 *                              removed.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayScale,VCheckScaleEvent,              *
 *                        VGetScaleInformation, VSetScaleInformation,   *
 *                        VGetScaleLayoutInforamtion,                   *
 *                        VCheckScaleEvents, VUndisplayScale.               *
 *      History         : Written on July 15, 1991 by Hsiu-Mei Hung.    *
 *                        Modified on March 30, 1993 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
int VDeleteScale ( char* label )
{
        int i, id ;

        if (label == NULL) 
            v_print_fatal_error("VDeleteScale",
                "The pointer of label should not be NULL.", (int)label) ;
        for (i=0, id= -1; i<lib_num_of_scales; i++) {
            if (strcmp(label,lib_scale[i].label) == 0) {
                id=i ;
                break ;
            }
        }
        if (id == -1) return(244) ;
        if (lib_display_scale_on[id] == 1) return(259) ;

        for (i=id+1 ; i<lib_num_of_scales; i++) {
            strcpy(lib_scale[i-1].label,lib_scale[i].label) ;
            strcpy(lib_scale[i-1].fmt,lib_scale[i].fmt) ;
            lib_scale[i-1].label_x=lib_scale[i].label_x ;
            lib_scale[i-1].label_y=lib_scale[i].label_y ;
            lib_scale[i-1].x=lib_scale[i].x ;
            lib_scale[i-1].y=lib_scale[i].y ;
            lib_scale[i-1].width=lib_scale[i].width ;
            lib_scale[i-1].height=lib_scale[i].height ;
            lib_scale[i-1].win=lib_scale[i].win ;
            lib_scale[i-1].gc=lib_scale[i].gc ;
            lib_scale[i-1].pixmap=lib_scale[i].pixmap ;
            lib_scale[i-1].pixmap_gc=lib_scale[i].pixmap_gc ;
            lib_scale[i-1].win_x=lib_scale[i].win_x ;
            lib_scale[i-1].win_y=lib_scale[i].win_y ;
            lib_scale[i-1].win_width=lib_scale[i].win_width ;
            lib_scale[i-1].win_height=lib_scale[i].win_height ;
            lib_scale[i-1].decimal=lib_scale[i].decimal ;
            lib_scale[i-1].step=lib_scale[i].step ;
            lib_scale[i-1].low_min=lib_scale[i].low_min ;
            lib_scale[i-1].low_max=lib_scale[i].low_max ;
            lib_scale[i-1].low_min_x=lib_scale[i].low_min_x ;
            lib_scale[i-1].low_min_y=lib_scale[i].low_min_y ;
            lib_scale[i-1].low_max_x=lib_scale[i].low_max_x ;
            lib_scale[i-1].low_max_y=lib_scale[i].low_max_y ;
            lib_scale[i-1].high_width=lib_scale[i].high_width ;
            lib_scale[i-1].high_x=lib_scale[i].high_x ;
            lib_scale[i-1].high_y=lib_scale[i].high_y ;
            lib_scale[i-1].high_min=lib_scale[i].high_min ;
            lib_scale[i-1].high_max=lib_scale[i].high_max ;
            lib_scale[i-1].high_max_x=lib_scale[i].high_max_x ;
            lib_scale[i-1].high_max_y=lib_scale[i].high_max_y ;
            lib_scale[i-1].value=lib_scale[i].value ;
            lib_scale[i-1].value_x=lib_scale[i].value_x ;
            lib_scale[i-1].value_y=lib_scale[i].value_y ;
            lib_scale[i-1].cursor_x=lib_scale[i].cursor_x ;
            lib_scale[i-1].cursor_y=lib_scale[i].cursor_y ;
            lib_display_scale_on[i-1]=lib_display_scale_on[i] ;
        }
        lib_num_of_scales-- ;    
        for (i=lib_num_of_scales; i<MAX_SCALES; i++)
            lib_display_scale_on[i]=0 ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VSetScaleInformation                          *
 *      Description     : This function will set x and y location, min  *
 *                        and max values, width and height, and the     *
 *                        decimal number for the scale specified by     *
 *                        label. If the number of scale is greater      *
 *                        than 10, this specifies is greater than the   *
 *                        width of the dialog window available, this    *
 *                        function will set the scale width to be 0 and *
 *                        return 246. If the scale width is less than   * 
 *                        the minimum requirement, this function will   *
 *                        set the scale width to be -1 and return 247.  *
 *                        The maximum number of scale is 10.            *
 *      Return Value    :  0 - work successfully.                       *
 *                         243 - scale is out of the window.            *
 *                         245 - Exceed the maximum number of scales    *
 *                              (10).                                   *
 *                         246 - scale width is greater than the width  *
 *                              of the dialog window available.         *
 *                         247 - scale width is too small to put        *
 *                              minimum requirement.                    *
 *      Parameters      :  label - the label of a certain scale.        *
 *                         xloc, yloc - Specifeis the upper left        *
 *                              coordinates of the specifed scale in    *
 *                              pixels.                                 *
 *                         min, max - the minimum and maximum values    *
 *                              in the scale.                           *
 *                         decimal - the number of decimals is going to *
 *                              use after the fraction point.           *
 *                         frac_of_dial_idth - fraction of the dialog   *
 *                              window width.                           *
 *      Side effects    : If label is NULL, or xloc, yloc, or decimal   *
 *                        is less than 0, or frac_of_dial_width is less *
 *                        than 0 or greater than 1, this function will  *
 *                        print the proper message to the standard error*
 *                        stream, producce the core dump, and exit from *
 *                        the current process.                          *
 *      Entry condition : VSetup must be called first.
 *      Related funcs   : VDisplayScale,VGetScaleInformation,           *
 *                        VCheckScaleEvent, VUndisplayScale,               *
 *                        VGetScaleLayoutInformation.                   *
 *      History         : Written on April 15, 1991 by Hsiu-Mei Hung.   *
 *                        Modified on Novemebr 20, 1992 by Krishna Iyer.*
 *                        Modified on March 30, 1993 by Krishna Iyer.   *
 *                        Modified 8/29/94 decimal 0 special case
 *                        removed by Dewey Odhner.
 *                        Modified 3/20/95 float passed to sprintf
 *                        cast to double by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VSetScaleInformation ( char* label, int xloc, int yloc,
    double min, double max, int decimal, double frac_of_dial_width )
{
        int width, avail_width, min_width, max_width, result, id ;
        char temp[80], fmt[80] ;

        if (lib_x_server_open == 0) {
            printf("The error occurred in the function ") ;
            printf("VSetScaleInformation.\n") ;
            printf("Programmer must call VSetup before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (label == NULL) 
            v_print_fatal_error("VSetScaleInformation",
                "The parameter \"avail_width\" should not be NULL.", (int)label) ;
        if (xloc < 0) 
            v_print_fatal_error("VSetScaleInformation",
                              "xloc should be >= 0",xloc) ;
        if (yloc < 0) 
            v_print_fatal_error("VSetScaleInformation",
                              "yloc should be >= 0",yloc) ;
        if (decimal < 0) 
            v_print_fatal_error("VSetScaleInformation",
                              "decimal should be >= 0",decimal) ;
        if (frac_of_dial_width <= 0 || frac_of_dial_width > 1) 
            v_print_fatal_error("VSetScaleInformation",
                "frac_of_dial_width should be > 0 && <= 1.", (int)frac_of_dial_width) ;  //gjg: ask dewey?
        for (id=0; id<lib_num_of_scales; id++) 
            if (strcmp(label,lib_scale[id].label) == 0) break ;
        if (id >= MAX_SCALES) return(245) ;
        if (lib_display_scale_on[id]) return(259) ;

/*
        if (lib_display_panel_on == 1) 
            width=(lib_wins[1].width-LIB_PANEL_WIDTH)*frac_of_dial_width ;
        else width=lib_wins[1].width*frac_of_dial_width ;
*/

        width=(lib_wins[1].width-LIB_PANEL_WIDTH-lib_wins[1].font_width) *
                                        frac_of_dial_width;

        sprintf(fmt,"%%.%df",decimal) ;
        sprintf(temp,fmt, (double)min) ;
        min_width=XTextWidth(lib_wins[1].font,temp,strlen(temp))+2; 
        sprintf(temp,fmt, (double)max) ;
        max_width=XTextWidth(lib_wins[1].font,temp,strlen(temp))+2; 
        strcpy(lib_scale[id].fmt,fmt) ;
        lib_scale[id].win_x=xloc+min_width ;
        lib_scale[id].win_y=yloc+LIB_SCALE_YLOC+LIB_SCALE_HEIGHT/3;
        lib_scale[id].win_width=width-min_width-max_width-8 ;
        lib_scale[id].win_height=LIB_SCALE_HEIGHT/3-4 ;
        strcpy(lib_scale[id].label,label);
        lib_scale[id].x=xloc;
        lib_scale[id].y=yloc+LIB_SCALE_YLOC;
        lib_scale[id].width=width;
        lib_scale[id].height=LIB_SCALE_HEIGHT;
        lib_scale[id].decimal=decimal;
        lib_scale[id].low_min=min;
        lib_scale[id].low_max=max;
        lib_scale[id].low_min_x=xloc;
        lib_scale[id].low_min_y=lib_scale[id].y+LIB_SCALE_HEIGHT/3 ;
        lib_scale[id].low_max_x=lib_scale[id].win_x+lib_scale[id].win_width ;
        lib_scale[id].low_max_y=lib_scale[id].low_min_y ;
        lib_scale[id].value_x=lib_scale[id].low_max_x ;
        lib_scale[id].value_y=lib_scale[id].y ;
        lib_scale[id].cursor_x= -1 ;
        lib_scale[id].cursor_y= 0 ;
        lib_scale[id].label_x=lib_scale[id].win_x ;
        lib_scale[id].label_y=lib_scale[id].y ;
        lib_scale[id].high_min_x= -1 ;
        lib_scale[id].high_min_y=lib_scale[id].low_min_y+LIB_SCALE_HEIGHT/3 ;
        lib_scale[id].high_max_x= -1 ;
        lib_scale[id].high_max_y=lib_scale[id].high_min_y ;
        lib_scale[id].high_x= -1 ;
        lib_scale[id].high_y= 0 ;
        lib_scale[id].high_width=lib_scale[id].win_width/2 ;
        lib_scale[id].step=0 ;
        result=0 ;
        if (width < (min_width+max_width*2)) {
            lib_scale[id].width= -1 ;
            result=247 ;
        }
/*
        if (lib_display_panel_on == 1) 
            avail_width=lib_wins[1].width-LIB_PANEL_WIDTH+1 ;
        else avail_width=lib_wins[1].width ;
*/

        avail_width=lib_wins[1].width-LIB_PANEL_WIDTH;

        if (width > avail_width) {
            lib_scale[id].width= 0 ;
            result=246 ;
        }
        if (xloc+width > avail_width) result=243 ;
        if (id == lib_num_of_scales) lib_num_of_scales++ ;
        return(result) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetScaleInformation                          *
 *      Description     : This function will return the upper left      *
 *                        coordinates, width and height , min and max,  *
 *                        and the number of decimal of the specified    *
 *                        scale. If width is 0, this value is undefined.*
 *      Return Value    :  0 - work successfully.                       *
 *                         244 - scale does not exist.                  *
 *      Parameters      :  xloc, yloc - Return the upper left           *
 *                              coordinates of the specifed scale.      *
 *                         width, height - Return the width and height  *
 *                              of the specified scale.                 *
 *                         decimal - Returns the number of decimal      *
 *                              after the fraction point of the         *
 *                              specified scale.                        *
 *      Side effects    : If label, xloc, yloc, width, height, min,     *
 *                        max, or decimal is NULL, this function will   *
 *                        print the proper message to the standard error*
 *                        stream, produce the core dump, and exit from  *
 *                        the current process.                          *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayScale, VUndisplayScale,                   *
 *                        VSetScaleInformation, VCheckScaleEvent,      *
 *                        VGetLayoutScaleInformation.                   *
 *      History         : Written on August 20, 1990 by Hsiu_Mei Hung.  *
 *                        Modified on May 10, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
int VGetScaleInformation ( char* label, int* xloc, int* yloc,
    int* width, int* height, float* min, float* max, int* decimal )
{
        int i ;

        if (lib_x_server_open == 0) {
            printf("The error occurred in the function ") ;
            printf("VGetScaleInformation.\n") ;
            printf("Please call VSetup before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (label == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of label should not be NULL.", (int)label) ;
        if (xloc == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of xloc should not be NULL.", (int)xloc) ;
        if (yloc == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of yloc should not be NULL.", (int)yloc) ;
        if (width == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of width should not be NULL.", (int)width) ;
        if (height == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of height should not be NULL.", (int)height) ;
        if (min == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of min should not be NULL.", (int)min) ;
        if (max == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of max should not be NULL.", (int)max) ;
        if (decimal == NULL) 
            v_print_fatal_error("VGetScaleInformation",
                "The pointer of decimal should not be NULL.", (int)decimal) ;

        for (i=0; i<lib_num_of_scales; i++) {
            if (strcmp(label,lib_scale[i].label) == 0) {
                *xloc=lib_scale[i].x ;
                *yloc=lib_scale[i].y ;
                *width=lib_scale[i].width ;
                *height=lib_scale[i].height ;
                *min=lib_scale[i].low_min ;
                *max=lib_scale[i].low_max ;
                *decimal=lib_scale[i].decimal ;
                return(0) ;
            }
        }
        return(244) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetScaleLayoutInformation                    *
 *      Description     : This function will return the avialable width *
 *                        and avialable height of the dialog window     *
 *                        and the height of the scale.                  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  avail_width - width of the dialog window     *
 *                              available for scale.                    *
 *                         avail_height - height of the dialog window   *
 *                              available for scale.                    *
 *                         scale_height - height of the scale.          *
 *      Side effects    : If avail_width, avail_height, or scale_height *
 *                        is NULL, this function will print the proper  *
 *                        message to the standard error stream, producce*
 *                        the core dump, and exit from the current      *
 *                        process.                                      *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayScale, VUndisplayScale,                   *
 *                        VSetScaleInformation, VCheckScaleEvent,      *
 *                        VGetScaleInforamtion.                         *
 *      History         : Written on May 29, 1991 by Hsiu-Mei Hung.     *
 *                        Modified on May 10, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
int VGetScaleLayoutInformation ( int* avail_width, int* avail_height,
    int* scale_height )
{
        if (lib_x_server_open == 0) {
            printf("The error occurred in the function ") ;
            printf("VGetScaleLayoutInformation.\n") ;
            printf("Please call VSetup before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (avail_width == NULL) 
            v_print_fatal_error("VGetScaleLayoutInformation",
                "The pointer of avail_width should not be NULL.", (int)avail_width) ;
        if (avail_height == NULL) 
            v_print_fatal_error("VGetScaleLayoutInformation",
                "The pointer of avail_height should not be NULL.",
                 (int)avail_height) ;
        if (scale_height == NULL) 
            v_print_fatal_error("VGetScaleLayoutInformation",
                "The pointer of scale_height should not be NULL.",
                 (int)scale_height) ;
        *scale_height=LIB_SCALE_HEIGHT ;
        /*if (lib_display_panel_on == 1) 
            *avail_width=lib_wins[1].width-LIB_PANEL_WIDTH ;
        else *avail_width=lib_wins[1].width ;
        */

        *avail_width=lib_wins[1].width-LIB_PANEL_WIDTH;
        *avail_height=lib_wins[1].height-
                      lib_wins[1].font_height*MAX_DIAL_MSG_LINES ;

        return(0);
}
        

/************************************************************************
 *                                                                      *
 *      Function        : VGetPanelInformation                          *
 *      Description     : This function will return the upper left      *
 *                        coordinates, width and height of the panel in *
 *                        the dialog window.                            *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  xloc, yloc - Return the upper left           *
 *                              coordinates of the panel.               *
 *                         width, height - Return the width and height  *
 *                              of the panel.                           *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayPanel,VCheckPanelEvent,VDeletePanel.   *
 *      History         : Written on August 20, 1990 by Hsiu-Mei Hung.  *
 *                                                                      *
 ************************************************************************/
int VGetPanelInformation ( int* xloc, int* yloc, int* width, int* max_height )
{
        if (lib_x_server_open == 0) {
            printf("The error occurred in the function ") ;
            printf("VGetPanelInformation.\n") ;
            printf("Please call VSetup before ");    
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }

        if(!lib_display_panel_on) return(216);

        if (xloc == NULL) 
            v_print_fatal_error("VGetPanelInformation",
                "The pointer of xloc should not be NULL.", (int)xloc) ;
        if (yloc == NULL) 
            v_print_fatal_error("VGetPanelInformation",
                "The pointer of yloc should not be NULL.", (int)yloc) ;
        if (width == NULL) 
            v_print_fatal_error("VGetPanelInformation",
                "The pointer of width should not be NULL.", (int)width) ;
        if (max_height == NULL) 
            v_print_fatal_error("VGetPanelInformation",
                "The pointer of height should not be NULL.", (int)max_height) ;

        *xloc=LIB_PANEL_XLOC ;
        *yloc=LIB_PANEL_YLOC ;
        *width=LIB_PANEL_WIDTH ;
        *max_height=lib_panel_rows*LIB_PANEL_ITEM_HEIGHT;

        return(0);
}

/************************************************************************/
