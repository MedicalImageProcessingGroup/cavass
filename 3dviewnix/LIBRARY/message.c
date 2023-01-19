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
 *      Filename  : message.c                                           *
 *      Ext Funcs : VDisplayImageMessage, VDisplayDialogMessage,        *
 *                  VDisplayButtonAction, VDisplayOverlayMessage, 	*
 *		    VClearOverlayMessage.       			*
 *      Int Funcs : v_draw_text,v_draw_textonly,v_make_button_press,    *
 *                  v_display_erase_ovl_msg.                            *
 *                                                                      *
 ************************************************************************/


#include "Vlibrary.h"
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include "3dv.h"

int VGetTextWidth ( Window win, char string[], int length, int* width );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayImageMessage                          *
 *      Description     : This function will display the messages, one  *
 *                        message per line, starting at the specified   *
 *                        location in the image window or in one of its *
 *                        subwinodws. If the dialog window is at the    *
 *                        front of the specified window and the message *
 *                        is cross the specified window and the         *
 *                        corresponding pixmap, this function will      *
 *                        display message in the pixmap and the         *
 *                        specified window.                             *
 *                        If pixmap is NULL and message covered by the  *
 *                        dialog window, this function will display     *
 *                        message in the specified window and return an *
 *                        error code. If message is out of the window   *
 *                        boundary, this function will truncate  the    *
 *                        the message, and return an error code 224.    *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window identification.           *
 *                         223 - message displayed in the image window  *
 *                               covered by the dialog window.          *
 *                         224 - message displayed is out of window     *
 *                               boundary.                              *
 *      Parameters      :  win - the image window ID or one of its      *
 *                               subwindows' ID                         *
 *                         msg - a message to be displayed in the       *
 *                               specified window.                      *
 *                         num_of_msg - number of messages to be        *
 *                               displayed.                             *
 *                         xloc, yloc - the starting location of        *
 *                               displaying message in the specified    *
 *                               window.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value xloc, or yloc is not valid,      *
 *                        num_of_msg is less than or equal to zero,     *
 *                        msg is NULL, function VCreateColormap is not  *
 *                        called earlier, or all the message is totally *
 *                        out of the window boundary, this function     *
 *                        will print a proper message in the            *
 *                        standard error stream, produce a core dump    *
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayDialogMessage, VDisplayButtonAction,  *
 *                        VDisplayOverlayMessage, VClearOverlayMessage. *
 *      History         : Written on December 12, 1988 by Hsiu-Mei Hung.*
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VDisplayImageMessage ( Window win, char msg[][MSG_WIDTH],
    int num_of_msg, int xloc, int yloc )
{
        int i, max_msg_len, msg_len, yloc1, result ;
        int width, height, abs_y , font_height;
        GC gc;
        char msg1[80] ;
        XFontStruct *font ;
        XTextItem tlabel;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplayImageMessage.\n") ; 
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (msg == NULL) 
            v_print_fatal_error("VDisplayImageMessage",
                "The pointer of msg should not be NULL.",(int)msg) ;
        if (num_of_msg <= 0) 
            v_print_fatal_error("VDisplayImageMessage",
                              "The range of num_of_msg > 0.",num_of_msg) ;
        max_msg_len=0 ;
        result=0 ;
        if (win == lib_wins[0].win) {
            abs_y=0 ; /* starting location relative to image window */
            width=lib_wins[0].width ;
            height=lib_wins[0].height ;
            gc=lib_wins[0].gc ;
            font=lib_wins[0].font ;
            font_height=lib_wins[0].font_height ;
        }
        else {  
            for (i=0; i<lib_num_subwins; i++) {
                if (win == lib_subwins[i].win) {
                    abs_y=lib_subwins[i].y ;
                    width=lib_subwins[i].width ;
                    height=lib_subwins[i].height ;
                    gc=lib_subwins[i].gc ;
                    font=lib_subwins[i].font ;
                    font_height=lib_subwins[i].font_height ;
                    break ;
                }
            }
            if (i == lib_num_subwins) return(6) ;
        }
        
        if (yloc >= height || yloc +font_height*num_of_msg <= 0) {
            sprintf(msg1,"Message is totally out of the specified window.") ;
            sprintf(msg1,"%s yloc range should be < %d && > %d",
                    msg1,height,(-font_height*num_of_msg)) ;
            v_print_fatal_error("VDisplayImageMessage",msg1,yloc) ;
        }               
        if (xloc >= width) {
            sprintf(msg1,"Message is totally out of the specified window.") ;
            sprintf(msg1,"%s xloc range should be < %d",msg1,width) ;
            v_print_fatal_error("VDisplayImageMessage",msg1,xloc) ;
        }
        for (i=0, result= -1; i<num_of_msg; i++) {
            msg_len=XTextWidth(font,msg[i],strlen(msg[i])) ;
            if (msg_len > max_msg_len) max_msg_len=msg_len ;
            if (xloc+msg_len > 0) result= 0 ;
        }
        if (result == -1) {
            sprintf(msg1,"Message is totally out of the specified window.") ;
            sprintf(msg1,"%s xloc range should be < %d && > %d",
                    msg1,width,-max_msg_len) ;
            v_print_fatal_error("VDisplayImageMessage",msg1,xloc) ;
        }
                                
        for (i=0; i<num_of_msg; i++) {
            msg_len=XTextWidth(font,msg[i],strlen(msg[i])) ;
            yloc1=yloc+font_height*i ;
                XSetForeground(lib_display,lib_wins[0].gc,
                        lib_reserved_colors[6].pixel);
                tlabel.chars = &msg[i][0];
                tlabel.nchars = strlen(msg[i]);
                tlabel.delta = 0;
                tlabel.font = lib_wins[0].font->fid;
                XDrawText(lib_display,win,lib_wins[0].gc,
                        xloc,yloc1+font->ascent,&tlabel,1);
                XFlush(lib_display);
/*
                XDrawString(lib_display,win,lib_wins[0].gc,
                            xloc,yloc1+font->ascent,
                            msg[i],strlen(msg[i])) ;
                XFlush(lib_display);
*/

            if (yloc1 >= lib_wins[0].height) return(224) ;
        }
        if (max_msg_len > lib_wins[0].width) result=224 ;
        return(result);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayDialogMessage                         *
 *      Description     : This function will check whether the the      *
 *                        dialog window is at the front of the image    *
 *                        window. If it is, then display the message    *
 *                        in the dialog window, else display the message*
 *                        into the temporary memory.                    *
 *                        The area reserved for the message displayed   *
 *                        in the dialog window is two lines. This       *
 *                        function will compute the maximum number of   *
 *                        characters allowed per line depending on      *
 *                        whether the panel is on or off. If the message*
 *                        is too long for one line, it will place the   *
 *                        rest of the message into the second line      *
 *                        automatically. If the message can not be      *
 *                        accommodated in the message rea, it will      *
 *                        truncate the message and return error code    *
 *                        224.                                          *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         224 - message displayed is out of window     *
 *                               boundary.                              *
 *                         235 - message is more than 199 characters.
 *                         258 - pixmap of the dialog window is NULL    *
 *                               and image window is at the front of the*
 *                               dialog window.                         *
 *      Parameters      :  msg - Specifies a string of message to       *
 *                               be displayed in the dialog window.     *
 *      Side effects    : None.                                         *
 *      Entry condition : If msg is NULL, or function VCreateColormap   *
 *                        is not called earlier, this function will     *
 *                        print a proper message in the standard error  *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VDisplayImageMessage, VDisplayButtonAction.   *
 *      History         : Written on December 12, 1988 by Hsiu-Mei Hung.*
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                        Modified 9/12/96 VGetTextWidth called
 *                        by Dewey Odhner.
 *                        Modified 4/19/99 message stored even if
 *                        2 lines long by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayDialogMessage ( char* msg )
{
        int count, max_chars, width, win_width;
        Drawable drawable ;
        GC gc ;
        int i, j, k ;
        XTextItem tlabel;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplayDialogMessage.\n") ; 
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (msg == NULL) 
            v_print_fatal_error("VDisplayDialogMessage",
                "The pointer of msg should not be NULL.",(int)msg) ;
        if (strlen(msg) >= 200)
            return (235);

        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ;
            gc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == NULL) return(258) ;
            drawable=lib_wins[1].pixmap ;
            gc=lib_wins[1].pixmap_gc ;
        }

        strcpy(LIB_DIAL_MSG,msg);
        if (lib_display_panel_on == 1) 
            win_width=lib_wins[1].width-LIB_PANEL_AREA;
        else win_width=lib_wins[1].width ;
        VGetTextWidth(drawable, msg, strlen(msg), &width);
        if (width <= win_width) {
            XSetForeground(lib_display,gc,lib_reserved_colors[1].pixel) ;
            XFillRectangle(lib_display,drawable,gc,
                           LIB_DIAL_MSG_XLOC,
                           LIB_DIAL_MSG_YLOC,
                           win_width-lib_wins[1].font_width,
                           2*lib_wins[1].font_height) ;
            /*************writing the text*************/
 
                tlabel.chars = msg;
                tlabel.nchars = strlen(msg);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[5].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                          LIB_DIAL_MSG_XLOC,
                          LIB_DIAL_MSG_YLOC+lib_wins[1].font->ascent,
                          &tlabel,1);

            v_draw_dialogwin_border();

            return(0) ;
        }

        for (count=strlen(msg)/2; count>0; count--)
        {
            if (msg[count] == ' ')
            {
                VGetTextWidth(drawable, msg+count+1, strlen(msg+count+1),
                    &width);
                if (width > win_width)
                    return (224);
                break;
            }
            if (msg[strlen(msg)-1-count] == ' ')
            {
                count = strlen(msg)-1-count;
                VGetTextWidth(drawable, msg, count, &width);
                if (width > win_width)
                    return (224);
                break;
            }
        }

        XSetForeground(lib_display,gc,lib_reserved_colors[1].pixel) ;
        XFillRectangle(lib_display,drawable,gc,
                       LIB_DIAL_MSG_XLOC,
                       LIB_DIAL_MSG_YLOC,
                       win_width-lib_wins[1].font_width,
                       lib_wins[1].font_height*2) ;

        /****writing the first line***/ 
        tlabel.chars = msg;
        tlabel.nchars = count;
        tlabel.delta = 0;
        tlabel.font = lib_wins[1].font->fid;
        XSetForeground(lib_display,gc,lib_reserved_colors[5].pixel);
        XDrawText(lib_display,lib_wins[1].win,gc,LIB_DIAL_MSG_XLOC,
                  LIB_DIAL_MSG_YLOC+lib_wins[1].font->ascent,&tlabel,1);

        /***writing the second line***/
        tlabel.chars = msg+count+1;
        tlabel.nchars = strlen(msg+count+1);
        tlabel.delta = 0;
        tlabel.font = lib_wins[1].font->fid;
        XSetForeground(lib_display,gc,lib_reserved_colors[5].pixel);
        XDrawText(lib_display,lib_wins[1].win,gc,LIB_DIAL_MSG_XLOC,
                  LIB_DIAL_MSG_YLOC+2*lib_wins[1].font->ascent,&tlabel,1);

        v_draw_dialogwin_border();
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayErrorMessage                          *
 *      Description     : This function will check whether the the      *
 *                        dialog window is at the front of the image    *
 *                        window. If it is, then display the message    *
 *                        in the dialog window, else display the message*
 *                        into the temporary memory.                    *
 *                        The area reserved for the message displayed   *
 *                        in the dialog window is two lines. This       *
 *                        function will compute the maximum number of   *
 *                        characters allowed per line depending on      *
 *                        whether the panel is on or off. If the message*
 *                        is too long for one line, it will place the   *
 *                        rest of the message into the second line      *
 *                        automatically. If the message can not be      *
 *                        accommodated in the message rea, it will      *
 *                        truncate the message and return error code    *
 *                        224.                                          *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         224 - message displayed is out of window     *
 *                               boundary.                              *
 *                         258 - pixmap of the dialog window is NULL    *
 *                               and image window is at the front of the*
 *                               dialog window.                         *
 *      Parameters      :  msg - Specifies a string of message to       *
 *                               be displayed in the dialog window.     *
 *      Side effects    : None.                                         *
 *      Entry condition : If msg is NULL, or function VCreateColormap   *
 *                        is not called earlier, this function will     *
 *                        print a proper message in the standard error  *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VDisplayImageMessage, VDisplayButtonAction.   *
 *      History         : Written on June 10, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
int VDisplayErrorMessage ( char* msg )
{
        int count, max_chars, width ;
        Drawable drawable ;
        GC gc ;
        char *line1, *line2 ;
        int i, j, k ;
 
        char text[100];
        XTextItem tlabel;
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplayDialogMessage.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }

        if (msg == NULL)
            v_print_fatal_error("VDisplayDialogMessage",
                "The pointer of msg should not be NULL.",(int)msg) ;
 
        if (lib_dial_win_front) {
            drawable=lib_wins[1].win ;
            gc=lib_wins[1].gc ;
        }
        else {
            if (lib_wins[1].pixmap == NULL) return(258) ;
            drawable=lib_wins[1].pixmap ;
            gc=lib_wins[1].pixmap_gc ;
        }
 
        if (lib_display_panel_on == 1)
            width=lib_wins[1].width-LIB_PANEL_AREA;
        else width=lib_wins[1].width ;
 
        max_chars=width/lib_wins[1].font_width ;
        count=strlen(msg) ;
        if (count <= max_chars) {
            XSetForeground(lib_display,gc,lib_reserved_colors[1].pixel) ;
            XFillRectangle(lib_display,drawable,gc,
                           LIB_DIAL_MSG_XLOC,
                           LIB_DIAL_MSG_YLOC,
                           width-lib_wins[1].font_width,
                           2*lib_wins[1].font_height) ;
            /*************writing the text*************/
                strcpy(text,msg);
                tlabel.chars = &text[0];
                tlabel.nchars = strlen(text);
                tlabel.delta = 0;
                tlabel.font = lib_wins[1].font->fid;
 
                XSetForeground(lib_display,gc,
                        lib_reserved_colors[11].pixel);
                XDrawText(lib_display,lib_wins[1].win,gc,
                          LIB_DIAL_MSG_XLOC,
                          LIB_DIAL_MSG_YLOC+lib_wins[1].font->ascent,
                          &tlabel,1);
                XFlush(lib_display);
 
            return(0) ;
        }
        line1=(char *)malloc((max_chars+1)*sizeof(char)) ;
        if (line1 == NULL) return(1) ;
        line2=(char *)malloc((max_chars+1)*sizeof(char)) ;
        if (line2 == NULL) return(1) ;
        for (i=0; i<max_chars; i++) line1[i]=msg[i] ;
        if (msg[i] != ' ')
            for (i=max_chars-1; i>=0; i--) if (line1[i] == ' ') break ;
        line1[i]='\0' ;
        for (j=i+1, k=0; k<max_chars; j++, k++) {
            if (msg[j] != '\0') line2[k]=msg[j] ;
            else break ;
        }
        line2[k]='\0' ;
 
        XSetForeground(lib_display,gc,lib_reserved_colors[1].pixel) ;
        XFillRectangle(lib_display,drawable,gc,
                       LIB_DIAL_MSG_XLOC,
                       LIB_DIAL_MSG_YLOC,
                       width-lib_wins[1].font_width,
                       lib_wins[1].font_height*2) ;
 
        /****writing the first line***/
        strcpy(text,line1);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[1].font->fid;
 
        XSetForeground(lib_display,gc,lib_reserved_colors[11].pixel);
        XDrawText(lib_display,lib_wins[1].win,gc,LIB_DIAL_MSG_XLOC,
                  LIB_DIAL_MSG_YLOC+lib_wins[1].font->ascent,&tlabel,1);
        XFlush(lib_display);
 
        /***writing the second line***/
        strcpy(text,line2);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[1].font->fid;
 
        XSetForeground(lib_display,gc,lib_reserved_colors[11].pixel);
        XDrawText(lib_display,lib_wins[1].win,gc,LIB_DIAL_MSG_XLOC,
                  LIB_DIAL_MSG_YLOC+2*lib_wins[1].font->ascent,&tlabel,1);
        XFlush(lib_display);
 
        v_draw_dialogwin_border();

        if (k == max_chars && msg[j] != '\0') return(224) ;
 
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetTextWidth                               *
 *      Description     : This function will return the width of a text *
 *                        string in the font used in the window         *
 *                        specified.                                    *
 *      Return Value    : 0 - work successfully.                        *
 *                        6 - invalid window ID.                        *
 *      Parameters      : win - the window the fid is requested for.    *
 *                        string - the text string                      *
 *                        length - the number of characters in string   *
 *                        width - width of string in pixels goes here   *
 *      Side effects    : None.                                         *
 *      Entry condition : If VSetup is not called earlier, this *
 *                        function will print a proper message to the   *
 *                        standard error stream, produce a core dump    *
 *                        file, and exit from the current process.      *
 *      Related funcs   : VGetWindows, VDisplayText, VDisplayImageMessage, *
 *                        VDisplayOverlayMessage                        *
 *      History         : Written 8/25/93 by Dewey Odhner.
 *                        Modified 9/12/96 to work for blank strings
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VGetTextWidth ( Window win, char string[], int length, int* width )
{
        XCharStruct overall;
        int dum, error_code;
        XID id;

        error_code = VGetWindowFontID(win, &id);
        if (error_code)
                return (error_code);
        for (dum=0; dum<length; dum++)
            if (isgraph(string[dum]))
                break;
        if (dum == length)
        {
            *width = 0;
            return (0);
        }
        XQueryTextExtents(lib_display, id,
                string, length, &dum,&dum,&dum,&overall);
        *width = overall.width;
        return (0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayButtonAction                          *
 *      Description     : This function will display three button       *
 *                        action messages in the button window.         *
 *                        Each button action message can have up to     *
 *                        MAX_BUTT_MSG_LINES number of lines (see       *
 *                        LIBRARY.PAR, Appendix B).                     *
 *                        Each message line can have up to              *
 *                        MAX_BUTT_CHARS number of characters (see      *
 *                        LIBRARY.PAR, Appendix B). This function will  *
 *                        display just the first MAX_BUTT_CHARS         *
 *                        characters on the screen if the number of     *
 *                        characters ina message line exceeds this      *
 *                        number. In the latter case, error code 224    *
 *                        will be returned.                             *
 *                        The line seperator is "\n". For example,      *
 *                        if you have 2 lines of message for the left   *
 *                        button, one line for the middle button, and   *
 *                        no message for the right button, then this    *
 *                        function call should be                       *
 *                        VDisplayButtonAction("line1\nline2\n",        *
 *                        "line1\n","").                                *
 *                        If any message is more than                   *
 *                        MAX_BUTT_MSG_LINES lines, the first           *
 *                        MAX_BUTT_MMSG_LINES lines will be displayed   *
 *                        and the error code 212 will be returned.      *
 *                        This function allows highlighting a set of    *
 *                        characters in each message by enclosing them  *
 *                        within []. For example, to highlight "XYZ" in *
 *                        the left button message, do                   *
 *                        VDisplayButtonAction("ABC\nabc[XYZ]\n","123", *
 *                                      "123").                         *
 *                        If you just specify the "[" and not "]", then *
 *                        this function will highlight from the         *
 *                        character next to "[" to the end of the line. *
 *                        If you specify "]" and not "[", then this     *
 *                        function will not highlight any character in  *
 *                        the line.                                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         212 - button message is more than three      *
 *                               lines.                                 *
 *                         224 - message displayed is out of window     *
 *                               boundary.                              *
 *      Parameters      :  msg1, msg2, msg3 - Specifies button status   *
 *                               message to be displayed in the button  *
 *                               window.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : If msg1, msg2, or msg3 is NULL, or function   *
 *                        VCreateColormap is not called earlier, this   *
 *                        function will print a proper message in the   *
 *                        standard error stream, produce a core dump    *
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayImageMessage, VDisplayDialogMessage.  *
 *      History         : Written on December 12, 1988 by Hsiu-Mei Hung.*
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                        Modified 8/25/93 rewritten by Dewey Odhner    *
 *                                                                      *
 ************************************************************************/
int VDisplayButtonAction ( char* msg1, char* msg2, char* msg3 )
{
        int offset,error;
        GC butt_gc;
        char *msg[3], unbracketed[80];
        int yloc,xloc,myxloc,xlen;
        int highlight;
        int j, k, lines, line, line_start, line_end, seg_start, seg_end;
        int width,height;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VDisplayButtonAction.\n") ; 
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (msg1 == NULL) 
            v_print_fatal_error("VDisplayButtonAction",
                "The pointer of msg1 should not be NULL.",(int)msg1) ;
        if (msg2 == NULL) 
            v_print_fatal_error("VDisplayButtonAction",
                "The pointer of msg2 should not be NULL.",(int)msg2) ;
        if (msg3 == NULL) 
            v_print_fatal_error("VDisplayButtonAction",
                "The pointer of msg3 should not be NULL.",(int)msg3) ;
        butt_gc=lib_wins[2].gc ;
        offset=(lib_wins[2].width)/3+lib_wins[2].font_width*2;

        offset=(lib_wins[2].width)/3;
        width=(lib_wins[2].width)/3;
        height=(LIB_BUTT_BOX_HEIGHT*1.75)/3;

        XSetForeground(lib_display,lib_wins[2].gc,
                       lib_reserved_colors[0].pixel);
 
        v_draw_buttons();
 
        msg[0] = msg1;
        msg[1] = msg2;
        msg[2] = msg3;
        error = 0;
        for (j=0; j<3; j++) {
            myxloc=j*offset;
	        yloc=LIB_BUTT_BOX_HEIGHT*0.9;   /*leaves one line from
                                                top of button window.*/
            lines = MAX_BUTT_MSG_LINES;
            for (line=line_start=0; line<lines; line++, line_start=line_end+1){
                k = 0;
				highlight = FALSE;
				for (line_end=line_start; msg[j][line_end]!='\n'; line_end++) {
				    if (msg[j][line_end] == 0) {
					    lines = line+1;
						break;
				    }
					if (msg[j][line_end] == (highlight? ']': '['))
						highlight = !highlight;
					else
						unbracketed[k++] = msg[j][line_end];
				}
				if (k == 0)
					continue;
				VGetTextWidth(lib_wins[2].win, unbracketed, k, &xlen);
				if (xlen > width)
					error = 224;
				xloc = myxloc+offset/2-xlen/2;
				highlight = FALSE;
				seg_end = seg_start = line_start;
				for (; msg[j][seg_end]!=0 && msg[j][seg_end]!='\n';
				        highlight= !highlight, seg_start=seg_end+1) {
					for (seg_end=seg_start;
							msg[j][seg_end]!=(highlight? ']': '['); seg_end++){
						if (msg[j][seg_end]==0 || msg[j][seg_end]=='\n')
							break;
					}
					if (seg_end == seg_start)
						continue;
					VGetTextWidth(lib_wins[2].win, msg[j]+seg_start,
						seg_end-seg_start, &xlen);
					if (highlight) {
						XSetForeground(lib_display,lib_wins[2].gc,
							lib_reserved_colors[10].pixel);
						XFillRectangle(lib_display,lib_wins[2].win, butt_gc,
							xloc,  yloc, xlen, lib_wins[2].font_height);
						XSetForeground(lib_display,lib_wins[2].gc,
							lib_reserved_colors[0].pixel);
					}
					XDrawString(lib_display,lib_wins[2].win,butt_gc, xloc,
						yloc+lib_wins[2].font->ascent, msg[j]+seg_start,
						seg_end-seg_start);
					xloc += xlen;
				}
				yloc += lib_wins[2].font_height;
			}
			if (msg[j][line_end] && msg[j][line_end+1])
				error = 212;
        }
        strcpy(lib_butt_msg[0],msg1) ;
        strcpy(lib_butt_msg[1],msg2) ;
        strcpy(lib_butt_msg[2],msg3) ;
        return(error) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_draw_text                                   *
 *      Description     : This function will display the text in the    *
 *                        location specified by centering it within the *
 *                        area specified by width and height.           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  gc - the gc of the area to draw text in.     *
 *                         text - the text to draw.                     *
 *                         xloc,yloc - the top left corner of the       *
 *                                rectangle to draw the text in.        *
 *                         width,height - of the rectangle.             *
 *                         highlight_flag - to indicate whether to      *
 *                                highlight the text also.              *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayButtonAction.                         *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_draw_text ( GC gc, char text[20] , int xloc, int yloc,
    int width, int height, int highlight_flag )
{

        XCharStruct overall;
        int n,dum;
        short label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;

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
        label_x = (short)xloc + (width - label_width)/2;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent; 

        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;

        if(highlight_flag==1)
        {
                XSetForeground(lib_display,gc,lib_reserved_colors[10].pixel);
                XFillRectangle(lib_display,lib_wins[2].win,gc,
                               label_x,label_y-label_height-1,
                               label_width,label_height+2);

        } 

        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,gc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_draw_textonly                               *
 *      Description     : This function will display the text in the    *
 *                        location specified by centering it within the *
 *                        area specified by width and height. The xloc  *
 *                        is where the text begins from.                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  gc - the gc of the area to draw text in.     *
 *                         text - the text to draw.                     *
 *                         xloc,yloc - the top left corner of the       *
 *                                rectangle to draw the text in.        *
 *                         width,height - of the rectangle.             *
 *                         highlight_flag - to indicate whether to      *
 *                                highlight the text also.              *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayButtonAction.                         *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_draw_textonly ( GC gc, char text[20], int xloc, int yloc,
    int width, int height, int highlight_flag )
{
 
        XCharStruct overall;
        int n,dum;
        short label_width,label_height;
        short label_x,label_y,label_ascent;
        XTextItem tlabel;
 
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
        label_x = (short)xloc;
        label_y = (short)yloc + (height - label_height)/2 + label_ascent;
 
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        if(highlight_flag==1)
        {
                XSetForeground(lib_display,gc,lib_reserved_colors[10].pixel);
                XFillRectangle(lib_display,lib_wins[2].win,gc,
                               label_x,label_y-label_height-1,
                               label_width,label_height+2);
 
        }
 
        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel);
        XDrawText(lib_display,lib_wins[2].win,gc,
                  label_x,label_y,&tlabel,1);
        XFlush(lib_display);

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayStatus                                *
 *      Description     : This function will display the "Working..."   *
 *                        and "Ready..." message in the button window.  *
 *      Return Value    :  0 - work successfully.                       *
 *                         278 - invalid status number.                 *
 *      Parameters      :  status - indicates what status to display.   *
 *                                  0 - Ready.                          *
 *                                  1 - Working.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : If VCreateColormap is not called earlier, this*
 *                        function will print a proper message to the   *
 *                        standard error stream, produce a core dump    *
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on May 10, 1993 by Krishna Iyer.      *
 *                                                                      *
 ************************************************************************/
int VDisplayStatus ( int status )
{

        char msg[100];
        XTextItem tlabel;
        GC statusgc;
        int text_xloc,text_yloc;

        if(lib_cmap_created == 0) 
        {
           printf("The error occurred in the function ");
           printf("VDisplayStatus.\n");
           printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }


        if(status == 0) strcpy(msg,"Ready...");
        else if(status == 1) strcpy(msg,"Working...");
        else return(278);

        text_xloc = lib_wins[2].font_width;
        text_yloc = lib_wins[2].font_height*1.1;

        statusgc=lib_wins[2].gc;

        tlabel.chars = &msg[0];
        tlabel.nchars = strlen(msg);
        tlabel.delta = 0;
        tlabel.font = lib_wins[2].font->fid;
 
        XClearArea(lib_display,lib_wins[2].win,1,1,lib_wins[2].width,
                   (unsigned int)(lib_wins[2].font_height*1.5),False);

        XSetForeground(lib_display,statusgc,
                        lib_reserved_colors[5].pixel);
        XDrawText(lib_display,lib_wins[2].win,
                  statusgc,
                  text_xloc,text_yloc,&tlabel,1);
        XFlush(lib_display);

        lib_display_status = status;

	v_draw_buttonwin_border();

        return(0);
}

/*****************************************************************/
static int v_make_button_press ( int button )
{
        int butt_offset, box_width, box_height, box_xloc, box_yloc ;
        int i,diag_line_offset;
        GC threedbuttongc;
        XGCValues vals;

        XClearArea(lib_display,lib_wins[2].win,1,1,lib_wins[2].width,
                   LIB_BUTT_LINE_YLOC+1,False);
        butt_offset=(lib_wins[2].width-LIB_BUTT_MSG_XLOC)/3;
                        /*lib_wins[2].font_width*1.5;*/
        box_width=LIB_BUTT_BOX_WIDTH*5;
        box_height=LIB_BUTT_BOX_HEIGHT*2;
        box_xloc=LIB_BUTT_BOX_WIDTH*0.50;
        box_yloc=LIB_BUTT_BOX_HEIGHT*0.50;

        threedbuttongc = XCreateGC(lib_display,lib_wins[2].win,0,&vals);

        for (i=button; i<button; i++) {
            XSetForeground(lib_display,threedbuttongc,
                                lib_reserved_colors[3].pixel);
            XFillRectangle(lib_display,lib_wins[2].win,threedbuttongc,
                           box_xloc+i*butt_offset,box_yloc,
                           box_width,box_height);
        }

        vals.line_width=5;
        XChangeGC(lib_display,threedbuttongc,GCLineWidth,&vals);

        /********bottom bright lines********/
        for (i=button; i<button; i++) {
            XSetForeground(lib_display,threedbuttongc,
                                lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset,
                      box_yloc+box_height,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc+box_height);
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc+box_height);
        }

           vals.line_width=1;
           XChangeGC(lib_display,threedbuttongc,GCLineWidth,&vals);
        for (i=button; i<button; i++) {
           XSetForeground(lib_display,threedbuttongc,
                                lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset,
                      box_yloc+box_height,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc+box_height);
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc+box_height);
        }

        vals.line_width=3;
        diag_line_offset=vals.line_width;
        XChangeGC(lib_display,threedbuttongc,GCLineWidth,&vals);

        /********top dark lines********/
        for (i=button; i<button; i++) {
            XSetForeground(lib_display,threedbuttongc,
                                lib_reserved_colors[2].pixel);
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset-2,
                      box_yloc,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc);
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset,
                      box_yloc,
                      box_xloc+i*butt_offset,
                      box_yloc+box_height);
        }
            /*****diagonal lines*****/
        /*
        vals.line_width=1;
        XChangeGC(lib_display,threedbuttongc,GCLineWidth,&vals);
        for(i=button;i<button;i++){
            XDrawLine(lib_display,lib_wins[2].win,threedbuttongc,
                      box_xloc+i*butt_offset+
                        box_width-diag_line_offset,
                      box_yloc+box_height-diag_line_offset,
                      box_xloc+i*butt_offset+box_width,
                      box_yloc+box_height);
        }
        */
        XDrawLine(lib_display,lib_wins[2].win,lib_wins[2].gc,
                  LIB_BUTT_LINE_XLOC,LIB_BUTT_LINE_YLOC,lib_wins[2].width,
                  LIB_BUTT_LINE_YLOC);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_display_erase_ovl_msg                       *
 *      Description     : This function will display or erase overlay   *
 *                        message at the specified location of the      *
 *                        image window or one of its subwindows         *
 *                        according to the value of flag.               *
 *                        If the dialog window is at the front of the   *
 *                        specified window and the message is across the*
 *                        specified window and the corresponding pixmap,*
 *                        this function will display or erase message in*
 *                        the pixmap and the specified window.          *
 *                        If message is out     *
 *                        of the window boundary, this function will    *
 *                        truncate the message, and return an error     *
 *                        code 224.                                     *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *                         223 - message displayed in the image window  *
 *                               covered by the dialog window.          *
 *                         224 - Message displayed is out of window     *
 *                               boundary.                              *
 *                         225 - no overlay created.                    *
 *                         237 - the overlay does not turn on.          *
 *      Parameters      :  win - the image window ID or one of its      *
 *                               subwindows' ID.                        *
 *                         ovl - Specifies the overlay number to be     *
 *                               erased or displayed.                   *
 *                         xloc, yloc - Specifies x and y coordinates   *
 *                               of the message located.                *
 *                         msg - Specifies the message to be displayed  *
 *                               or erased.                             *
 *                         flag - Specifies the flag to indicate erasing*
 *                               or displaying the overlay message in   *
 *                               image window.                          *
 *                               0 - erasing the overlay message.       *
 *                               1 - displaying the overlay message.    *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value xloc, yloc, or ovl is not valid, *
 *                        msg is NULL, function VCreateColormap is      *
 *                        not called earlier, or the message is totally *
 *                        out of the window in the standard error       *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VClearOverlayMessage,                         *
 *                        VDisplayOverlayMessage.                       *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
static int v_display_erase_ovl_msg ( Window win, int ovl, int xloc, int yloc,
    char* msg, int flag, char* func )
#if 0
Window win;
int ovl;
int xloc,yloc;
char *msg;
int flag;    /* 0 - erase ; 1 - display */
char *func;
#endif
{
        int i, width, height, abs_y , font_height;
        GC gc;
        char msg1[80] ;
        XFontStruct *font ;
        int result, msg_len ;
        unsigned long foreground ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("%s.\n",func) ; 
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (ovl < 1 || ovl > lib_num_of_overlays) {
            sprintf(msg1,"The range of ovl is >=1 && <= %d.",
                    lib_num_of_overlays) ;
            v_print_fatal_error(func,msg1,ovl) ;
        }
        if (msg == NULL) 
            v_print_fatal_error(func,"The pointer of msg should not be NULL.",
                              (int)msg) ;
        if (lib_num_of_overlays == 0) return(225) ;
        if (lib_overlay_on[ovl-1] == 0) return(237) ;

        result=0 ;
        if (win == lib_wins[0].win) {
            abs_y=0 ; /* starting location relative to image window */
            width=lib_wins[0].width ;
            height=lib_wins[0].height ;
            gc=lib_wins[0].gc ;
            font=lib_wins[0].font ;
            font_height=lib_wins[0].font_height ;
        }
        else {  
            for (i=0; i<lib_num_subwins; i++) {
                if (win == lib_subwins[i].win) {
                    abs_y=lib_subwins[i].y ;
                    width=lib_subwins[i].width ;
                    height=lib_subwins[i].height ;
                    gc=lib_subwins[i].gc ;
                    font=lib_subwins[i].font ;
                    font_height=lib_subwins[i].font_height ;
                    break ;
                }
            }
            if (i == lib_num_subwins) return(6) ;
        }
        
        if (yloc >= height || yloc +font_height <= 0) {
            sprintf(msg1,"Message is totally out of the specified window.") ;
            sprintf(msg1,"%s yloc range should be < %d && > %d",
                    msg1,height,(-font_height)) ;
            v_print_fatal_error(func,msg1,yloc) ;
        }               
        msg_len=XTextWidth(font,msg,strlen(msg)) ;
        if (xloc >= width || xloc+msg_len <= 0) {
            sprintf(msg1,"Message is totally out of the specified window.") ;
            sprintf(msg1,"%s xloc range should be < %d",msg1,width) ;
            v_print_fatal_error(func,msg1,xloc) ;
        }
                                
            if (ovl == 1) {
                gc=lib_wins[0].ovl1_gc ;
                foreground=lib_reserved_colors[8].pixel ;
            }
            if (ovl == 2) {
                gc=lib_wins[0].ovl2_gc ;
                foreground=lib_reserved_colors[7].pixel;
            }
            if (flag == 0) foreground=0;
            XSetForeground(lib_display,gc,foreground) ;
            XDrawString(lib_display,win,gc,xloc,yloc+font->ascent,
                        msg,strlen(msg)) ;

        if (xloc+msg_len > lib_wins[0].width) result=224;
        return(result);
}

/************************************************************************
 *                                                                      *
 *      Function        : VDisplayOverlayMessage                        *
 *      Description     : This function will display overlay message    *
 *                        at the specified location of the image window *
 *                        or one of its subwindows. You should call     *
 *                        function VTurnOnOverlay to turn the overlay   *
 *                        on before one calls this function to display  *
 *                        an overlay message. If the dialog window      *
 *                        is at the front of the specified window and   *
 *                        the message is across the specified window and*
 *                        the corresponding pixmap, this function will  *
 *                        display message in the pixmap and the         *
 *                        specified window.                             *
 *                        If pixmap is NULL and message covered by the  *
 *                        dialog window, this function will display     *
 *                        message in the specified window and return    *
 *                        an error code. If message is out of the window*
 *                        boundary, this function will truncate the     *
 *                        message, and return an error code 224.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *                         223 - message displayed in the image window  *
 *                               covered by the dialog window.          *
 *                         224 - Message displayed is out of window     *
 *                               boundary.                              *
 *                         225 - no overlay created.                    *
 *                         237 - the overlay does not turn on.          *
 *      Parameters      :  win - the image window ID or one of its      *
 *                               subwindows' ID                         *
 *                         ovl - Specifies the overlay number to be     *
 *                               displayed themessage.                  *
 *                         xloc, yloc - Specifies x and y coordinates of*
 *                               the message located.                   *
 *                         msg - Specifies the message to be displayed. *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value xloc, yloc, or ovl is not valid, *
 *                        msg is NULL, function VCreateColormap is not  *
 *                        called earlier, or the message is totally     *
 *                        out of the window boundary, this function will*
 *                        print a proper message in the standard error  *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VClearOverlayMessage, VTurnOnOverlay,         *
 *                        VTurnOffOverlay, VDisplayImageMessage.        *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VDisplayOverlayMessage ( Window win, int ovl, int xloc, int yloc,
    char* msg )
{
        int result ;

        result=v_display_erase_ovl_msg(win,ovl,xloc,yloc,msg,1,
                                     "VDisplayOverlayMessage") ;
        return(result) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VClearOverlayMessage                          *
 *      Description     : This function will erase overlay message at   *
 *                        the specified location of the image window    *
 *                        or one of its subwindows. If the dialog window*
 *                        is at the front of the specified window and   *
 *                        the message is across the specified window    *
 *                        and the corresponding pixmap, this function   *
 *                        will erase message in the pixmap and the      *
 *                        specified window.                             *
 *                        If pixmap is NULL and message covered by the  *
 *                        dialog window, this function will erase       *
 *                        message in the specified window and return an *
 *                        error code. If message is out of the window   *
 *                        boundary, this function will truncate the     *
 *                        message, and return an error code 224.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *                         223 - message erased in the image window     *
 *                               covered by the dialog window.          *
 *                         224 - Message erased is out of window        *
 *                               boundary.                              *
 *                         225 - no overlay created.                    *
 *                         237 - the overlay does not turn on.          *
 *      Parameters      :  win - the image window ID or one of its      *
 *                               subwindows' ID.                        *
 *                         ovl - Specifies the overlay number to be     *
 *                               erased the message.                    *
 *                         xloc, yloc - Specifies x and y coordinates   *
 *                               of the message located.                *
 *                         msg - Specifies the message to be erased.    *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value xloc, yloc, or ovl is not valid, *
 *                        msg is NULL, function VCreateColormap is not  *
 *                        called earlier, or the message is totally out *
 *                        of the window boundary, this function will    *
 *                        print a proper message in the standard error  *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VDisplayOverlayMessage.                       *
 *      History         : Written on January 10, 1993 by Krishna Iyer.  *
 *                                                                      *
 ************************************************************************/
int VClearOverlayMessage ( Window win, int ovl, int xloc, int yloc,
    char* msg )
{
        int result ;

        result=v_display_erase_ovl_msg(win,ovl,xloc,yloc,msg,0,
                                     "VClearOverlayMessage") ;
        return(result) ;
}

/*************************************************************************/
