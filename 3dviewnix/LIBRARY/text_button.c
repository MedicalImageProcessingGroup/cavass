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
 *      Filename  : text_button.c                                       *
 *      Ext Funcs : VSetTabletCaret, VSetTabletLocation, VDeleteTabletList,         *
 *                  VDisplayTablet, VDeleteTablet, VGetTabletPointerFromLabel,           *
 *                  VGetTabletValue, VGetTabletValueByLabel, VSetTabletValue, *
 *                  VRemoveTablet, VRedisplayTablet, VCheckTabletEvent,           *
 *                  VSetButtonItemLocation, VDeleteButtonItemList, VDisplayButtonItem,        *
 *                  VDeleteButtonItem, VGetButtonItemLabel, VSetButtonItemLabel,    *
 *                  VRemoveButtonItem, VRedisplayButtonItem, VSetButtonItemState,        *
 *                  VGetButtonItemMode, VCheckButtonItemEvent.                  *
 *      Int Funcs : v_GetNextText0,    *
 *                  v_DisableAllText, v_libAddText, v_AddText0,         *
 *                  v_GetTextEnabled, v_libDeleteText, v_DeleteText0,   *
 *                  v_GetTextIdByIndex, v_libGetTextValue,              *
 *                  v_GetTextValue0, v_EraseText, v_DisableText,        *
 *                  v_EnableText, v_DisplayText, v_InsideText,          *
 *                  v_AppendText, v_DeleteLastTextCharacter,            *
 *                  v_libCheckTextEvent, v_CheckTextEvent_user,         *
 *                  v_EraseButton, v_DisplayButton, v_InsideButton,     *
 *                  v_libDisableAllText.                                *
 *                                                                      *
 ************************************************************************/
#include "Vlibrary.h"
#include "3dv.h"
#include <sys/types.h>
#include <unistd.h>

#define LEFT_BUTTON 1
#define MIDDLE_BUTTON 2
#define RIGHT_BUTTON 3

static char caret[100] = {"|"};
static int      cl=1;   /* caret length */


/* USER LEVEL VARIABLES */
static TEXT     *Text;
static int      Number_of_texts = 0;

/* LIBRARY LEVEL VARIABLES */
static TEXT     *ViewnixText;
static int      Number_of_viewnix_texts = 0;

/*******************BUTTON Related funtions**********************/
#define LEFT_BUTTON 1
#define MIDDLE_BUTTON 2
#define RIGHT_BUTTON 3
 
static BUTTON   *Button;
static int      Number_of_buttons = 0;

/* private button list: */
static BUTTON   *ViewnixButton;
static int      Number_of_viewnix_buttons = 0;

static int v_AppendText ( TEXT* t, char* c );
static int v_DeleteLastTextCharacter ( TEXT* t );
static int v_DisableText ( TEXT* ID );
static int v_EnableText ( TEXT* item );
static int v_EraseButton ( BUTTON* item );
static int v_EraseText ( TEXT* item );

/************************************************************************
 *                                                                      *
 *      Function        : VSetTabletCaret                                 *
 *      Description     : This function sets the caret used for the Text*
 *                        Items. The caret is a character string        *
 *                        composed of one or more characters. It        *
 *                        indicates that a Text Item is enabled and     *
 *                        shows where the text will be appended.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         265 - invalid caret                          *
 *      Parameters      :  c - character string representing the new    *
 *                              caret.                                  *
 *      Side effects    : Changes the value of the local Global         *
 *                        variable "char caret[100]".                   *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VSetTabletCaret ( char* c )
{
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VSetTabletCaret.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(strlen(c) > 99 || strlen(c) == 0)
                return(265);
        strcpy(caret, c);
        cl = strlen(caret);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VSetTabletLocation                                   *
 *      Description     : This function sets the x,y location of the    *
 *                        Text Item within a window. The location is    *
 *                        relative to the window coordinate system.     *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - Invalid Text Item ID.                  *
 *      Parameters      :  item - Text Item ID.                         *
 *                         x, y - new location of the Text item         *
 *                              (with respect to the item window).      *
 *      Side effects    : Changes the x,y fields in the Text Item       *
 *                        refered to by ID.                             *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VSetTabletLocation ( TEXT* item, int x, int y )
{
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VSetTabletLocation.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(item == NULL) return(266);

        v_EraseText(item);

        /* Set new location */
        item->x = x;
        item->y = y;

        v_DisplayText(item);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDeleteTabletList                                *
 *      Description     : This function deletes all the Text Items.     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : The Text Item list is erased and its          *
 *                        allocated space is freed.                     *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayTablet.                                     *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VDeleteTabletList ( void )
{

        TEXT *t1, *t2;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VDeleteTabletList.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        /* if list is already empty */
        if(Text == NULL) return(0);

        t1 = Text;
        t2 = t1->next;

        v_EraseText(t1);
        free(t1);
        Text = NULL;

        while(t2 != NULL)
        {
                t1 = t2;
                t2 = t2->next;
                v_EraseText(t1);
                free(t1);
        }

        Number_of_texts = 0;

        return(0);

}

int v_refresh_text_list ( Window win )
{
 
        TEXT *t1, *t2;
 
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VDeleteTabletList.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }
 
        /* if list is already empty */
        if(Text == NULL) return(0);
 
        t1 = Text;
        while (t1 != NULL)
        {
          v_DisplayText(t1);
          t1=t1->next;
        }
 
        return(0);
}

int v_refresh_button_list ( Window win )
{
 
        BUTTON *t1, *t2;
 
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VDeleteTabletList.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }
 
        /* if list is already empty */
        if(Button == NULL) return(0);
 
        t1 = Button;
        while(t1 != NULL)
        {
          v_DisplayButton(t1);
          t1 = t1->next;
        }
 
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_GetNextText0                                *
 *      Description     : This function returns the next item (following*
 *                        the given item) on the list whose state is ON.*
 *      Return Value    :  0 - work successfully.                       *
 *                         269 - no text item was found.                *
 *      Parameters      :  item - Text Item ID.                         *
 *                         ID - Text Item ID of item to be found.       *
 *                         mode - whether internal or external.         *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_GetNextText0 ( TEXT* item, TEXT** ID, int mode )
{
        TEXT *t, *head_of_list;
        int N;
        int found=0;
        int counter=0;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_GetNextText0.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(item==NULL) return(266);


        if(mode == 0)
        {
                head_of_list = Text;
                N = Number_of_texts;
        }
        else
        {
                head_of_list = ViewnixText;
                N = Number_of_viewnix_texts;
        }


        /* Get next item (wrap around if necessary) */
        t = item->next;
        if(t==NULL) t=head_of_list;
        counter++;


        /* Loop until ON text item */
        while(t->state == 0 && counter<N)
        {

                /* Get next item (wrap around if necessary) */
                t = t->next;
                if(t == NULL) t = head_of_list;

                counter++;
        }

        *ID = t;

        if(t->state == 1) 
                return(0);
        else
                return(269);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DisableAllText                              *
 *      Description     : This function disable all Text Items currently*
 *                        in the list.                                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  None.                                        *
 *      Side effects    : All Text items have their 'mode' structure    *
 *                        element changed. All Items are redisplayed.   *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_DisableAllText ( void )
{
        TEXT *t1;
        
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_DisableAllText.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        t1 = Text;

        while(t1 != NULL)
        {
                if(t1->mode == 1)
                {
                        if(v_DisableText(t1) > 0)
                                return(266);
                }

                t1 = t1->next;
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_AddText                                     *
 *      Description     : This function adds a new Text Item to the     *
 *                        list.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  window - window containing the Text Item.    *
 *                         x,y - location of the Item within the window.*
 *                         width - maximum width of item (label+value)  *
 *                              in font_width units.                    *
 *                         height - maximum height of item in Fonts     *
 *                              x10(>0) or pixes (<0).                  *
 *                         label - string representing the label of the *
 *                              Item.                                   *
 *                         value - string representing the contents of  *
 *                              the Item.                               *
 *                         proc - notify procedure(when "return" is hit)*
 *                         ID - Text Item ID.                           *
 *      Side effects    : A new structure is appended to the Text       *
 *                        Item list.                                    *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int VDisplayTablet ( Window win, int x, int y, int width, char* label,
    char* value, int (*proc)(), TEXT** ID )
#if 0
Window win; /* window in which item resides */
int x, y,   /* location of item */
    width;  /* width of item in fonts (if negative number then it is in pixels) */
char *label,/* label for the item */
     *value;/* contents of the item */
int     (*proc)();  /* notify procedure */
TEXT    **ID;
#endif
{
        return(v_AddText0(win, x, y, width, 0, label, value, proc, ID, 0));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_AddText0                                    *
 *      Description     : This function adds a new Text Item to the     *
 *                        list.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  window - window containing the Text Item.    *
 *                         x,y - location of the Item within the window.*
 *                         width - maximum width of item (label+value)  *
 *                              in font_width units.                    *
 *                         height - maximum height of item in Fonts     *
 *                              x10(>0) or pixes (<0).                  *
 *                         label - string representing the label of the *
 *                              Item.                                   *
 *                         value - string representing the contents of  *
 *                              the Item.                               *
 *                         proc - notify procedure(when "return" is hit)*
 *                         ID - Text Item ID.                           *
 *                         mode - whether it is a library call or from  *
 *                              outside.                                *
 *      Side effects    : A new structure is appended to the Text       *
 *                        Item list.                                    *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on February 10, 1993 by Krishna Iyer.*
 *                        Modified 11/25/98 error code returned if
 *                           label does not fit by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_AddText0 ( Window win, int x, int y, int width, int height,
    char* label, char* value, int (*proc)(void*), TEXT** ID, int mode )
#if 0
Window win;     /* window in which item resides */
int     x, y,   /* location of item */
        width,  /* width of item in fonts (if negative number then it is in pixels) */
        height; /* height of item in fonts (x10, +) or pixels (-). 0 is the default */
char *label,/* label for the item */
         *value;/* contents of the item */
int     (*proc)();  /* notify procedure */
TEXT    **ID;
int mode;       /* 0=USER lEVEL,  1=LIBRARY LEVEL */
#endif
{
        TEXT *t, *head_of_list;
        int N;  /* Number of Text items */
        int i,j, dum;
        unsigned long ldum;
        char text[200];
        int n;
        int limit_width_in_pixels;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_AddText0.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(mode == 0) 
        {
                head_of_list = Text;
                N = Number_of_texts;
        }
        else 
        {
                head_of_list = ViewnixText;
                N = Number_of_viewnix_texts;
        }

        /* Head of the list */
        if(head_of_list == NULL)
        {
                /* USER LEVEL */
                if(mode == 0) 
                {
                        Number_of_texts = 0;
                        Text = (TEXT *) malloc(sizeof(TEXT));
                        t = Text;
                }
                else
                /* LIBRARY LEVEL */
                {
                        Number_of_viewnix_texts = 0;
                        ViewnixText = (TEXT *) malloc(sizeof(TEXT));
                        t = ViewnixText;
                }


                if(t==NULL) return(1);
        }
        /* Other entries */
        else
        {
                t = head_of_list;

                /* traverse the text item list till its end */
                for(i=0; i<N-1; i++)
                {
                        if(t->next == NULL)
                        {
                                fprintf(stderr, "ERROR: VDisplayTablet !\n");
                                return(268);
                        }
                        t = t->next;
                }

                t->next = (TEXT *) malloc(sizeof(TEXT));
                t = t->next;
        }

        t->win = win;
        t->x = x;
        t->y = y;
        t->w = width;
        strcpy(t->label, label);
        strcpy(t->value, value);
        t->pos = 0;
        t->state = 1;   /* ON */
        t->mode = 0;    /* INACTIVE */
        t->func = proc;

        /* Get XFontStruct for the item */
        for(i=0; i<3; i++)
        {
                if(t->win == lib_wins[i].win)
                        t->font_struct = lib_wins[i].font;
        }

        /**subwindows**/
        if (i == 3)
        {
           for (j=0; j<lib_num_subwins; j++)
           {
                if (t->win == lib_subwins[j].win)
                        t->font_struct = lib_wins[0].font; /*image window*/
           }
           /*if (j == lib_num_subwins) return(6);*/
        }


        /* Get size of font */
        VGetWindowInformation(t->win,
                            &dum,&dum,
                            &dum,&dum, 
                            &t->fw, &t->fh,
                            &ldum,&ldum);
        /* Get the width in pixels of the text item */
        strcpy(text, t->label);
        strcat(text, (char *) &(t->value[t->pos]));
        n = strlen(text);
/*
        VGetWindowGC(t->win,&gc);
        XQueryTextExtents(lib_display, XGContextFromGC(gc), text, n,
                                &dum, &dum, &dum, &overall);
        XFreeGC(lib_display,gc);
*/
        t->width = XTextWidth(t->font_struct, text, n);
        t->next = NULL;
        t->thick = 1;
        t->label_height =  t->fh;
        if(height == 0)
                t->h = t->label_height;
        else
        if(height > 0)
                t->h = t->fh * height / 10;
        else
        if(height < 0)
                t->h = - height;
        t->label_x = t->x + t->fw/2;
        t->label_y = t->y + (t->h - t->label_height)/2 + t->font_struct->ascent;

        /* If width==0, take the enclosing width for the given text */
        if(t->w == 0) t->w = -(t->width + t->fw);

        if(t->w < 0) limit_width_in_pixels = -t->w;
        else limit_width_in_pixels = t->w * t->fw;

        /* If the given text is larger then the given width */
        if( t->width >= limit_width_in_pixels )
        {
                /* scroll text untill it fits */
                while(t->width >= limit_width_in_pixels)
                {
                        t->pos++;
                        if (t->pos >= strlen(t->value))
                            return(283);
                        strcpy(text, t->label);
                        strcat(text, (char *) &(t->value[t->pos]));
                        n = strlen(text);
                        t->width = XTextWidth(t->font_struct, text, n);
                }
        }

        /* Display the text item */
        v_DisplayText(t);

        if(mode == 0) Number_of_texts ++;
        else Number_of_viewnix_texts ++;

        *ID = t;
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_GetTextEnabled                              *
 *      Description     : This function finds the next Text Item on the *
 *                        list (starting from the root) whose mode is   *
 *                        ENABLED (1).                                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         269 - no Text item found.                    *
 *      Parameters      :  ID - Text Item ID of item to be found.       *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_GetTextEnabled ( int* ID )
{
        TEXT *t;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_GetTextEnabled.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        t = Text;

        while(t != NULL)
        {
                if(t->mode == 1)
                {
                        *ID = (int) t;
                        return(0);
                }

                t = t->next;
        }


        return(269);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DeleteText                                  *
 *      Description     : This function :Removes a given Text Item from *
 *                        the list. Its memory space is freed.          *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  ID - Text Item ID of item being deleted.     *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VDeleteTablet ( TEXT* ID )
{
        return(v_DeleteText0(ID, 0));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DeleteText0                                 *
 *      Description     : This function :Removes a given Text Item from *
 *                        the list. Its memory space is freed.          *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  ID - Text Item ID of item being deleted.     *
 *                         mode - whether it is a library call or       *
 *                              from outside.                           *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int v_DeleteText0 ( TEXT* ID, int mode )
#if 0
TEXT *ID;
int mode;       /* 0=USER LEVEL,  1=LIBRARY LEVEL */
#endif
{
        int i;
        TEXT *t, *t2;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_DeleteText0.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(mode == 0) t = t2 = Text;
        else t = t2 = ViewnixText;

        if(ID == NULL) return(266);


        /* find the ITEM to DELETE */
        i=0;
        while(t!=NULL && t != ID)
        {
                t2 = t;
                t = t->next;
                i++;
        }

        if(t==NULL) return(266);

        v_EraseText(t);

        if(i==0)
        {
                if(mode == 0) Text = t->next;
                else ViewnixText = t->next;
                free(t); t=NULL;
        }
        else
        {
                t2->next = t->next;
                free(t); t=NULL;
        }

        if(mode == 0) Number_of_texts--;
        else Number_of_viewnix_texts--;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_GetTextIdByIndex                            *
 *      Description     : This function returns the corresponding ID    *
 *                        for the Text Item given an index on the list. *
 *      Return Value    :  0 - work successfully.                       *
 *                         268 - inconsistent Text Item list.           *
 *                         270 - bad Text Item list index.              *
 *      Parameters      :  n - index of the Item.                       *
                           ID - Text Item ID of item to be found.       *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_GetTextIdByIndex ( int n, int* ID )
{
        int i;
        TEXT *t;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_GetTextIdByIndex.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(n >= Number_of_texts) return(270);

        t = Text;
        for(i=0; i<n; i++)
        {
                t = t->next;
                if(t == (TEXT *) NULL)
                        return(268);
        }

        *ID =(int) t;
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetTabletPointerFromLabel                             *
 *      Description     : This function returns the corresponding ID    *
 *                        for the Text Item given a label.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *                         269 - no Text Item found.                    *
 *      Parameters      :  text - string representing the label of the  *
 *                              Item.                                   *
 *                         ID - Text Item ID of item to be found.       *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VGetTabletPointerFromLabel ( char* text, int* ID )
{
        TEXT *t;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VGetTabletPointerFromLabel.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        t = Text;

        if(t == NULL) return(266);


        while( t != NULL)
        {
                /* if the labels match, then text item was found */
                if(strcmp(text, t->label) == 0)
                {
                        *ID = (int) t;
                        return(0);
                }

                /* go to next text item on the list */
                t = t->next;
        }

        return(269);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_GetTextValue                                *
 *      Description     : This function returns the contents (value) of *
 *                        a given Text Item.                            *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *                         269 - no Text item found.                    *
 *                         271 - invalid Text Item value pointer.       *
 *      Parameters      :  ID - ID of the given Text Item.              *
 *                         val - character string in which value is     *
 *                              returned.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VGetTabletValue ( TEXT* ID, char* val )
{
        return(v_GetTextValue0(ID, val, 0));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_GetTextValue0                               *
 *      Description     : This function returns the contents (value) of *
 *                        a given Text Item.                            *
 *      Return Value    :  0 - work successfully.                       *
 *                         235 - array bound exceeded.
 *                         266 - invalid Text Item ID.                  *
 *                         269 - no Text item found.                    *
 *                         271 - invalid Text Item value pointer.       *
 *      Parameters      :  ID - ID of the given Text Item.              *
 *                         val - character string in which value is     *
 *                              returned.                               *
 *                         mode - whether it is a library call or       *
 *                              from outside.                           *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 3/14/95 val size fixed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_GetTextValue0 ( TEXT* ID, char val[100], int mode )
#if 0
TEXT *ID;
char val[100];
int mode;
#endif
{
        TEXT *t;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_GetTextValue0.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(mode == 0) t = Text;
        else t = ViewnixText;

        if(val == NULL) return(271);
        if(t==NULL) return(266);

        while(t != ID)
        {
                t = t->next;
                if(t == (TEXT *) NULL)
                        return(269);
        }
        if (strlen(t->value) >= 100)
                return (235);

        /* If the item is ENABLED, get rid of the CARET */
        if(t->mode == 1)
        {
                v_DisableText(t);
                strcpy(val, t->value);
                v_EnableText(t);
                return(0);
        }

        strcpy(val, t->value);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetTabletValueByLabel                          *
 *      Description     : This function returns the contents (value) of *
 *                        a given Text Item.                            *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *                         269 - no Text item found.                    *
 *                         271 - invalid Text Item value pointer.       *
 *      Parameters      :  text - string representing the label of the  *
 *                              given Item.                             *
 *                         val  - character string in which value is    *
 *                              returned.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VGetTabletValueByLabel ( char* text, char* val )
{
        TEXT *t;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VGetTabletValueByLabel.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        t = Text;

        if(val == NULL) return(271);
        if(t == NULL) return(266);


        while( t != NULL)
        {
                /* if the labels match, then text item was found */
                if(strcmp(text, t->label) == 0)
                {
                        /* If the item is ENABLED, DISABLE it */
                        if(t->mode == 1)
                                v_DisableText(t);

                        strcpy(val, t->value);
                        return(0);
                }

                /* go to next text item on the list */
                t = t->next;
        }

        return(269);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_EraseText                                   *
 *      Description     : This function erases the Item from the screen.*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  item - ID for the Text item.                 *
 *      Side effects    : The region occupied by the Item is erased.    *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_EraseText ( TEXT* item )
{

        if(item->w > 0)
                VClearWindow(item->win, item->x, item->y, item->w*item->fw, item->h);
        if(item->w < 0)
                VClearWindow(item->win, item->x, item->y, -item->w, item->h);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DisableText                                 *
 *      Description     : This function disables a text item, i.e. it   *
 *                        will no longer take input from the keyboard.  *
 *                        The caret at the end of the value is removed  *
 *                        indicating its disability.                    *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  ID - Text Item ID of item to be disabled.    *
 *      Side effects    : Changes the 'mode' element in the Text Item   *
 *                        structure.                                    *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_DisableText ( TEXT* ID )
{
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_DisableText.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(ID == 0) return(-1);

        if(ID->mode == 1)
        {
                ID->mode = 0;

                v_EraseText(ID);

                /* remove the caret */
                v_DeleteLastTextCharacter(ID);
        }

        /* redisplay item */
        return(v_DisplayText(ID));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_EnableText                                  *
 *      Description     : This function enables a Text Item for keyboard*
 *                        entry.                                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  item - ID for the Text item.                 *
 *      Side effects    : A caret is appended to the Item's value.      *
 *                        The Item is redisplayed.                      *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_EnableText ( TEXT* item )
{
        if(item == NULL) return(266);

        if(item->mode == 0)
        {
                item->mode = 1;
                v_AppendText(item, caret);
        }

        /* redisplay item */
        return(v_DisplayText(item));
}



/************************************************************************
 *                                                                      *
 *      Function        : VSetTabletValue                                 *
 *      Description     : This function sets the value of a given Text  *
 *                        Item.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  item  - ID for the Text item.                *
 *                         value - character string of new value.       *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VSetTabletValue ( TEXT* item, char* value )
{

        GC gc;
        XCharStruct overall;
        char text[200];
        int n,dum;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VSetTabletValue.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(item == NULL) return(266);

        VGetWindowGC(item->win,&gc);

        /* Get the width in pixels of the new text item */
        strcpy(text, item->label);
        strcat(text, (char *) &(item->value[item->pos]));
        n = strlen(text);
        XQueryTextExtents(lib_display, XGContextFromGC(gc), text, n,
                                &dum, &dum, &dum, &overall);
        /*item->width = overall.width;*/
        item->width = XTextWidth(item->font_struct, text, n);

        v_EraseText(item);

        /* If Text is ENABLED */
        if(item->mode == 1)
        {
                /* remove the caret */
                item->value[0] = 0;
                strcpy(item->value, value);
                v_AppendText(item, caret);
        }
        else
        /* If Text is DISABLED */
                strcpy(item->value, value);

        v_DisplayText(item);

        XFreeGC(lib_display,gc);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VRemoveTablet                                   *
 *      Description     : This function turns a Text item OFF.          *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  item - ID for the Text item.                 *
 *      Side effects    : The 'state' element of the Item's structure   *
 *                        is changed. he Item is erased, and it won't   *
 *                        "exist" for interaction purposes until it is  *
 *                        turned ON.                                    *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VRedisplayTablet.                                   *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VRemoveTablet ( TEXT* item )
{
        if(item == NULL) return(266);

        if(item->state == 1)
        {
                item->state = 0;
                v_EraseText(item);
        }
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VRedisplayTablet                                    *
 *      Description     : This function turns a Text item ON.           *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  ID - Text Item ID of item to be found.       *
 *      Side effects    : The 'state' element of the Item's structure is*
 *                        changed.                                      *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VRemoveTablet.                                  *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VRedisplayTablet ( TEXT* item )
{
        if(item == NULL) return(266);

        if(item->state == 0)
                item->state = 1;
        v_DisplayText(item);
        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : v_DisplayText                                 *
 *      Description     : This function displays a given Text Item.     *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  item - ID for the Text item.                 *
 *      Side effects    : Displays th Text Item on the window           *
 *                        (if item->state==ON).                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on March 12, 1993 by                 *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int v_DisplayText ( TEXT* item )
{
        XTextItem tlabel;
        GC gc, buttonitemgc;
        XGCValues vals;
        int l;
        char text[500];
        int x1,y1, x2,y2, x3,y3, x4,y4;
        int t;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_DisplayText.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(item == NULL) return(266);

        /* if Text Item is OFF, don't display it */
        if(item->state == 0) return(0);

        /* Get GC */
        VGetWindowGC(item->win,&gc);    /* graphics context */

        strcpy(text, item->label);
        strcat(text, (char *) (&item->value[item->pos]));
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;

        tlabel.font = item->font_struct->fid;
        /* lear previous image on text item */
        v_EraseText(item);

        t = item->thick; /* thickness of frame */
        x1 = item->x;
        y1 = item->y;
        /* Width in FONTS */
        if(item->w > 0)
        {
                x2 = x1 + item->w * item->fw - 1;
                y2 = y1;
                x3 = x2;
                y3 = y2 + item->h - 1;
                x4 = x1;
                y4 = y3;
        }
        else
        /* Width in PIXELS */
        {
                x2 = x1 - item->w - 1;
                y2 = y1;
                x3 = x2;
                y3 = y2 + item->h - 1;
                x4 = x1;
                y4 = y3;
        }

        buttonitemgc = XCreateGC(lib_display,item->win,0,&vals);
        vals.line_width=t;
        XChangeGC(lib_display,buttonitemgc,GCLineWidth,&vals);

         /*
 
                (x1,y1)----------(x2,y2)
                   |                |
                   |                |
                (x4,y4)----------(x3,y3)
 
 
        */

                /***********fill the rectangles***********/
        XSetForeground(lib_display,buttonitemgc, lib_reserved_colors[2].pixel);
        XFillRectangle(lib_display,item->win,buttonitemgc, x1,y1,(x2-x1),(y4-y1));
        XFlush(lib_display);
        /***********top dark lines***********/
        XSetForeground(lib_display,buttonitemgc, lib_reserved_colors[3].pixel);
                /* North */
        XDrawLine(lib_display,item->win,buttonitemgc, x1, y1, x2, y2);
                /* West */
        XDrawLine(lib_display,item->win,buttonitemgc, x1, y1, x4, y4);
        XFlush(lib_display);
 
        /***********bottom bright lines***********/
        XSetForeground(lib_display,buttonitemgc, lib_reserved_colors[4].pixel);
                /* East */
        XDrawLine(lib_display,item->win,buttonitemgc, x2,y2+1,x3,y3);
                /* South */
        XDrawLine(lib_display,item->win,buttonitemgc, x3,y3,x4+1,y4);
        XFlush(lib_display);

                XFreeGC(lib_display,buttonitemgc);

        XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel);
        XDrawText(lib_display,item->win, gc, item->label_x, 
                        item->label_y, &tlabel, 1);

        /* If text is scrolled, Indicate by drawing a left-arrow under the Label */
        if(item->pos > 0)
        {
                l = XTextWidth( item->font_struct, item->label, strlen(item->label) );
        XDrawLine(lib_display,item->win, gc, x1+l, y4-2, x1+l+4, y4-2);
        XDrawLine(lib_display,item->win, gc, x1+l, y4-2, x1+l+1, y4-3);
        XDrawLine(lib_display,item->win, gc, x1+l, y4-2, x1+l+1, y4-1);
        }

        XFlush(lib_display);
        XFreeGC(lib_display,gc);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_InsideText                                  *
 *      Description     : This function checks if a coordinate is       *
 *                        inside a given Text item, given the item is   *
 *                        within a given window and is ON.              *
 *      Return Value    :  TRUE - is inside.                            *
 *                         FALSE - is not inside, item is OFF or        *
 *                              different window.                       *
 *                         266 - invalid Text Item ID.                  *
 *      Parameters      :  item - ID for the Text item.                 *
 *                         x,y - corrdinate.                            *
 *                         window - window in which coordinate is being *
 *                              checked.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_InsideText ( TEXT* item, int x, int y, Window window )
{
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_InsideText.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(item == NULL) return(266);

        /* If not the same WINDOW or item is DISABLED */
        if( item->win != window  ||  item->state == 0)
                return(0);

        /* Width in FONTS */
        if( item->w > 0)
        {
                if(     x > item->x  &&  x < item->x + item->w*item->fw  &&
                        y > item->y  &&  y < (item->y + item->h)  )
                        return(1);
        }
        /* Width in PIXELS */
        else
        {
                if(     x > item->x  &&  x < item->x - item->w  &&
                        y > item->y  &&  y < (item->y + item->h)  )
                        return(1);
        }

        return(0);
                
}


/************************************************************************
 *                                                                      *
 *      Function        : v_AppendText                                  *
 *      Description     : This function will append the character c     *
 *                        to the text item t.                           *
 *      Return Value    :  0 - works successfully.                      *
 *      Parameters      :  t - the text item to be appended to.         *
 *                         c - the character to append.                 *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_AppendText ( TEXT* t, char* c )
{
        char text[200];
        int n;
        GC gc;
        XCharStruct overall;
        int limit_width_in_pixels, actual_width_in_pixels;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_AppendText.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        /* Get GC */
        VGetWindowGC(t->win,&gc);

        if(strlen(t->value) == 199) return(0);

        /* Append character */
        strcpy(text, t->label);
        strcat( t->value, c);
        strcat(text, (char *) &(t->value[t->pos]));
        n = strlen(text);
        /*
        XQueryTextExtents(lib_display, XGContextFromGC(gc), text, n,
                        &dum, &dum, &dum, &overall);
        */
        actual_width_in_pixels = XTextWidth(t->font_struct, text, n);

        /* If width is in FONTS */
        if( t->w > 0)
                limit_width_in_pixels = (t->w*t->fw)-t->fw;
        else
                limit_width_in_pixels = - t->w - t->fw;
        

        /* If width exceeds Item Width(In pixels), then increment start character for drawing string (scroll) */
        if( actual_width_in_pixels >= limit_width_in_pixels )
        {
                /* scroll text untill it fits */
                while(actual_width_in_pixels >= limit_width_in_pixels)
                {
                        t->pos++;
                        strcpy(text, t->label);
                        strcat(text, (char *) &(t->value[t->pos]));
                        n = strlen(text);
                        actual_width_in_pixels = XTextWidth(t->font_struct, text, n);
                }
        }
        else
        /* If doesn't exceed and was scrolled before, then scroll back one position */
        if( actual_width_in_pixels <= limit_width_in_pixels )
        {
                /* scroll text untill it fits */
                while(t->pos > 0 && actual_width_in_pixels <= limit_width_in_pixels)
                {
                        t->pos--;
                        strcpy(text, t->label);
                        strcat(text, (char *) &(t->value[t->pos]));
                        n = strlen(text);
                        actual_width_in_pixels = XTextWidth(t->font_struct, text, n);
                }
                if(t->pos > 0) t->pos++;
        }


        strcpy(text, t->label);
        strcat(text, (char *) &(t->value[t->pos]));
        n = strlen(text);
        
        /*
        XQueryTextExtents(lib_display, XGContextFromGC(gc), text, n,
                                &dum, &dum, &dum, &overall);
        t->width = overall.width;
        */
        t->width = XTextWidth(t->font_struct, text, n);
        XFreeGC(lib_display,gc);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DeleteLastTextCharacter                     *
 *      Description     : This function will delete the alst character  *
 *                        in the text item t.                           *
 *      Return Value    :  0 - works successfully.                      *
 *      Parameters      :  t - the text item.                           *
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_DeleteLastTextCharacter ( TEXT* t )
{
        char text[200];
        int n, dum;
        GC gc;
        XCharStruct overall;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_DeleteLastTextCharacter.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        /* Get GC */
        VGetWindowGC(t->win,&gc);

        /* Delete last character (cl=caret length) */
        if(strlen(t->value) > cl)
                t->value[strlen(t->value)-cl] = 0;
        else
                t->value[0] = 0;

        strcpy(text, t->label);
        strcat(text, (char *) &(t->value[t->pos]));
        n = strlen(text);
        
        XQueryTextExtents(lib_display, XGContextFromGC(gc), text, n,
                                &dum, &dum, &dum, &overall);
        /*t->width = overall.width;*/
        t->width = XTextWidth(t->font_struct, text, n);

        XFreeGC(lib_display,gc);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckTabletEvent                               *
 *      Description     : This function checks if an event occured on an*
 *                        Item. If it did, then take the appropriate    *
 *                        actions according to the type of event.       *
 *      Return Value    :  0 - work successfully.                       *
 *                         269 - no Text item found.                    *
 *      Parameters      :  event - XWindow event.                       *
 *      Side effects    : The value, mode (enabled, disabled) of an Item*
 *                        is changed according to the type of event.    *
 *                         LEFT-BUTTON - enables a Text Item            *
 *                              (if Item is disabled).                  *
 *                         LEFT-BUTTON - disables a Text Item           *
 *                              (if Item is enabled)                    *
 *                         CHARACTERS - if an Item is enabled, they     *
 *                              are appended to its value.              *
 *                         DELETE - if an Item is enabled, its last     *
 *                              character is deleted.                   *
 *                         SHIFT-DELETE - if an Item is enabled, its    *
 *                              entire value is deleted.                *
 *                         RETURN - if an Item is enabled, it becomes   *
 *                              disabled.                               *
 *                         RETURN - f an item was just disabled by a    *
 *                              previous  RETURN key, it then           *
 *                              enables the next Text Item on the list. *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on April 6, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
int VCheckTabletEvent ( XEvent* event )
{
        return(v_CheckTextEvent_user(event, 0));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_CheckTextEvent_user                         *
 *      Description     : This function checks if an event occured on an*
 *                        Item. If it did, then take the appropriate    *
 *                        actions according to the type of event.       *
 *      Return Value    :  0 - work successfully.                       *
 *                         269 - no Text item found.                    *
 *      Parameters      :  event - XWindow event.                       *
 *                         mode - whether it is a library call or from  *
 *                              outside.                                *
 *      Side effects    : The value, mode (enabled, disabled) of an Item*
 *                        is changed according to the type of event.    *
 *                         LEFT-BUTTON - enables a Text Item            *
 *                              (if Item is disabled).                  *
 *                         LEFT-BUTTON - disables a Text Item           *
 *                              (if Item is enabled)                    *
 *                         CHARACTERS - if an Item is enabled, they     *
 *                              are appended to its value.              *
 *                         DELETE - if an Item is enabled, its last     *
 *                              character is deleted.                   *
 *                         SHIFT-DELETE - if an Item is enabled, its    *
 *                              entire value is deleted.                *
 *                         RETURN - if an Item is enabled, it becomes   *
 *                              disabled.                               *
 *                         RETURN - f an item was just disabled by a    *
 *                              previous  RETURN key, it then           *
 *                              enables the next Text Item on the list. *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on April 6, 1993 by Krishna Iyer.    *
 *                        Modified 3/14/95 test of unset value fixed
 *                        by Dewey Odhner.
 *                        Modified 11/1/95 XLookupString called
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_CheckTextEvent_user ( XEvent* event, int mode )
{
        int i;
        int type;
        TEXT *found;
        TEXT *t, *tt;
        TEXT *head_of_list;
        int N;
        int px, py;
        char c[50];
        int sl;
        static int item_enabled=0;
        static TEXT *enabled_text;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_CheckTextEvent_user.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        type = event->type;

        /* if event is of no interest */        
        if(type != KeyPress && type != ButtonPress  &&  type != KeyRelease) return(0);

        if(mode == 0)
        {
                head_of_list = Text;
                N = Number_of_texts;
        }
        else
        {
                head_of_list = ViewnixText;
                N = Number_of_viewnix_texts;
        }

        /* if no Text items around */
        if(N == 0) return(0);

        /* get location of the event */ 
        px = event->xkey.x;
        py = event->xkey.y;


        /* If no TEXT ITEM is ENABLED or there was a LEFT-BUTTON */
        /* Find the TEXT ITEM being SELECTED  */
        /* for both Key and Button events */
        if(item_enabled == 0  ||  (type == ButtonPress && event->xbutton.button == LEFT_BUTTON))
        {
                t = head_of_list;
                i = 0;
                found = NULL;
                while(found == NULL  &&  t != NULL)
                {
                        if(v_InsideText(t, px, py, event->xany.window) )
                                found = t;
                        else
                                t = t->next;

                        i++;
                }

                /* NO TEXT ITEM WAS FOUND */
                if(found == NULL) return(269);

        }
        /* Use PREVIOUSLY selected Text item */
        else
        {
                t = enabled_text;
        }



        /*--------------------------------*/
        /* LEFT BUTTON */
        /*                      Just TURN THE ITEM ON */
        if(type == ButtonPress && event->xbutton.button == LEFT_BUTTON)
        {

                /* ENABLE Text Item */
                if(t->mode == 0) 
                {
                        v_DisableAllText();

                        /* ENABLE Text Item */
                        v_EnableText(t);

                        item_enabled = 1;
                        enabled_text = t;

                        /* Grab the keyboard events for the window of the Item */
                        XGrabKeyboard(lib_display, t->win, False, GrabModeAsync,GrabModeAsync, CurrentTime);
                }
                else
                /* DISABLE Text Item */
                {
                        /*v_DisableAllText();*/

                        /* DISABLE  Text Item */
                        v_DisableText(t);

                        item_enabled = 0;

                        /* Ungrab the keyboard events */
                        XUngrabKeyboard(lib_display, CurrentTime);
                }
                
        }
        else
        /*--------------------------------*/
        /* KEYBOARD  and Text Item is ENABLED */ 
        if(type == KeyPress  &&  t->mode == 1) 
        {
                /* Get the KEY string */
                sl = XLookupString(&event->xkey, c, sizeof(c), (KeySym *)NULL,
                        (XComposeStatus *)NULL);
                if (sl == sizeof(c))
                    sl--;
                c[sl] = 0;

                /* Check for SHIFT */
                if(sl == 0)
                        return(0);

                /* RETURN -> DISABLE Text Item */
                if( strcmp(c, "Return") == 0)
                {
                        v_DisableText(t);
                        item_enabled = 0;

                        /* Call Notify Procedure */
                        if(t->func != NULL)
                                (t->func)(t);
                

                        /* Ungrab the keyboard events */
                        XUngrabKeyboard(lib_display, CurrentTime);
                }
                else
                /* DELETE -> Delete last character on string */
                if((strcmp(c, "\010") == 0) || (strcmp(c, "\177") == 0))

                {
                        /* Remove the Item before its Width changes */
                        v_EraseText(t);

                        /* If NO SHIFT */
                        if((event->xkey.state&ShiftMask) == 0)
                        {
                                /* Delete last TWO characters (because there is a CARET at the end) */
                                if(strlen(t->value) > cl)
                                        t->value[strlen(t->value)-cl-1] = 0;
                                else
                                {
                                        t->pos = 0;
                                        t->value[0] = 0;
                                }

                        }
                        /* If SHIFT is ON */
                        else
                        {
                                t->pos = 0;
                                t->value[0] = 0;
                        }


                        /* Append CARET */
                        /*strcat(t->value, "CARET");*/
                        v_AppendText(t, caret);

                }
                else
                /* APPEND THE CHARACTER TO THE CURRENT VALUE OF THE ITEM */
                {
                        /* Ignore ESCAPEs, CTRLs, ARROWs, etc... */
                        if( strlen(c) > 1) strcpy(c, "");
                        
                        /* delete CARET */
                        t->value[strlen(t->value)-cl] = 0;

                        /* append CHARACTER */
                        strcat(t->value, c);

                        /* append CARET */
                        v_AppendText(t, caret);
                }


                /* redisplay text item with updated value */
                v_DisplayText(t);
        
        }
        else
        /* Text Item is OFF and RETURN Key, then go to NEXT TEXT ITEM */ 
        if(type == KeyPress  &&  t->mode == 0) 
        {
                /* Grab the keyboard events for the window of the Item */
                XGrabKeyboard(lib_display, t->win, False, GrabModeAsync,GrabModeAsync, CurrentTime);

                strcpy(c, XKeysymToString( XLookupKeysym((XKeyEvent*)event,0) ));

                if( strcmp(c, "Return") == 0)
                {
                        /*
                        tt = t->next;
                        if(tt == NULL) tt = Text;
                        */
                        if(mode == 0)
                                v_GetNextText0(t, &tt, 0);
                        else
                                v_GetNextText0(t, &tt, 1);
                        v_EnableText(tt);

                        enabled_text = tt;
                        item_enabled = 1;

                        /* Warp the pointer to that item */
                        XWarpPointer(lib_display,None,tt->win,0,0,0,0,tt->x+1, tt->y+tt->fh-1);
                }
        }
        
        /* return the index of the text item */
        return(0);

}


/************************************************************************
 *                                                                      *
 *      Function        : VSetButtonItemLocation                                 *
 *      Description     : This function sets the x,y location of the    *
 *                        Button Item within a window. The location is  *
 *                       relative to the window coordinate system.      *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - Invalid Button Item ID.                *
 *      Parameters      :  item - Button Item ID.                       *
 *                         x, y - new location of the Button item       *
 *                              (within item window).                   *
 *      Side effects    : Changes the x,y fields in the Button Item     *
 *                        refered to by ID.                             *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VSetButtonItemLocation ( BUTTON* item, int x, int y )
{
        if(item == NULL) return(266);

        v_EraseButton(item);

        /* Set new location */
        item->x = x;
        item->y = y;

        v_DisplayButton(item);

        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : VDeleteButtonItemList                              *
 *      Description     : This function deletes all the button items.   *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : The Button Item list is erased and its        *
 *                        allocated space is freed.                     *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VDeleteButtonItemList ( void )
{

        BUTTON *t1, *t2;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VDeleteButtonItemList.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        /* if list is already empty */
        if(Button == NULL) return(0);

        t1 = Button;
        t2 = t1->next;

        v_EraseButton(t1);
        free(t1);
        Button = NULL;

        while(t2 != NULL)
        {
                t1 = t2;
                t2 = t2->next;
                v_EraseButton(t1);
                free(t1);
        }

        Number_of_buttons = 0;

        return(0);

}


/************************************************************************
 *                                                                      *
 *      Function        : VDisplayButtonItem                                    *
 *      Description     : This function adds a new Button Item to the   *
 *                        list.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  win - window containing the Text Item.       *
 *                         x,y - location of the Item within the window.*
 *                         label - string representing the label of the *
 *                              Item.                                   *
 *                         value - string representing the contents of  *
 *                              the Item.                               *
 *                         ID - address of the Item ID.                 *
 *      Side effects    : A new structure is appended to the Text Item  *
 *                        list.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on February 16, 1993 by Krishna Iyer.*
 *                        Modified 12/21/94 v_AddButton0 called
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDisplayButtonItem ( Window win, int x, int y, int w, int h,
    char* label, int (*proc)(), int type, BUTTON** ID)
#if 0
Window win;             /* window in which item resides */
int             x, y,   /* location of item */
                w,h;    /* width and height of item in fonts */
char    *label; /* label for the item */
int     (*proc)();      /* notify procedure */
int             type;   /* type of button (0=button, 1=toggle, ...) */
BUTTON  **ID;
#endif
{
    return (v_AddButton0(win, x, y, w, h, label, proc, type, ID, 0));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_AddButton0                                  *
 *      Description     : This function adds a new Button Item to the   *
 *                        public or private list.                       *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  win - window containing the Text Item.       *
 *                         x,y - location of the Item within the window.*
 *                         label - string representing the label of the *
 *                              Item.                                   *
 *                         value - string representing the contents of  *
 *                              the Item.                               *
 *                         ID - address of the Item ID.                 *
 *                         mode - 0 for public list, 1 for private list
 *      Side effects    : A new structure is appended to the Text Item  *
 *                        list.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDisplayButtonItem
 *      History         : Written 12/21/94 (taken from VDisplayButtonItem)
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_AddButton0 ( Window win, int x, int y, int w, int h, char* label,
    int (*proc)(), int type, BUTTON** ID, int mode )
#if 0
Window win;             /* window in which item resides */
int             x, y,   /* location of item */
                w,h;    /* width and height of item in fonts */
char    *label; /* label for the item */
int     (*proc)();      /* notify procedure */
int             type;   /* type of button (0=button, 1=toggle, ...) */
BUTTON  **ID;
int mode;       /* 0=USER lEVEL,  1=LIBRARY LEVEL */
#endif
{
        BUTTON *t, *head_of_list;
        GC gc;
        int N;  /* Number of Button items */
        int i, dum;
        unsigned long ldum;
        char text[200];
        int n;
        int thick;
        XCharStruct overall;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VDisplayButtonItem.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(mode == 0) 
        {
                head_of_list = Button;
                N = Number_of_buttons;
        }
        else 
        {
                head_of_list = ViewnixButton;
                N = Number_of_viewnix_buttons;
        }

        /* Head of the list */
        if(head_of_list == NULL)
        {
                /* USER LEVEL */
                if(mode == 0) 
                {
                    Number_of_buttons = 0;
                    Button = (BUTTON *) malloc(sizeof(BUTTON));
                    t = Button;
                }
                else
                /* LIBRARY LEVEL */
                {
                    Number_of_viewnix_buttons = 0;
                    ViewnixButton = (BUTTON *) malloc(sizeof(BUTTON));
                    t = ViewnixButton;
                }

                if(t==NULL) return(1);
        }
        /* Other entries */
        else
        {
                t = head_of_list;

                /* traverse the text item list till its end */
                for(i=0; i<N-1; i++)
                {
                        if(t->next == NULL)
                        {
                                fprintf(stderr, "ERROR: VDisplayButtonItem !\n");
                                return(268);
                        }
                        t = t->next;
                }

                t->next = (BUTTON *) malloc(sizeof(BUTTON));
                t = t->next;
        }


        thick = 2;

        t->win = win;
        t->type = type;
        t->x = x;
        t->y = y;
        t->w = w;
        t->h = h;
        strcpy(t->label, label);
        t->state = 1;   /* ON */
        t->mode = 0;    /* RELEASED */

        /* Get size of font */
        VGetWindowInformation(t->win,
                            &dum,&dum,
                            &dum,&dum, 
                            &t->fw, &t->fh,
                            &ldum,&ldum);

        VGetWindowGC(t->win,&gc);
        /* Get the width in pixels of the text item */
        strcpy(text, t->label);
        n = strlen(text);
        if(n>0)
        {
                XQueryTextExtents(lib_display, XGContextFromGC(gc), text, n,
                                &dum, &dum, &dum, &overall);
                t->label_width = overall.width;
                t->label_height = overall.ascent + overall.descent;
                t->label_ascent = overall.ascent;
        }
        else
        {
                t->label_width = 0;
                t->label_height = 0;
                t->label_ascent = 0;
        }
        t->thick = thick;
        t->func = proc;
        t->next = NULL;
        /* Width */
        if(w == 0)
                /* If there is a label */
                if(t->label_width > 0)
                        /*t->width = t->label_width + t->fw + 2*thick;*/
                        t->width = t->label_width + t->fw;
                /* If there is no label, make the button width smaller than usual  */
                else
                        t->width = t->fh;

        else
        {
                if(w>0)
                        t->width = w * t->fw + t->fw+2*thick;
                else
                        t->width = -w;
        }
        /* Height */
        if(h == 0)
                /* If there is a label */
                if(t->label_width > 0)
                        /*t->height = t->label_height + t->fw/2 + 2*thick;*/
                        t->height = LIB_PANEL_ITEM_HEIGHT;
                /* If there is no label, make the button height equal to font height  */
                else
                        /*t->height = t->fh;*/
                        t->height = LIB_PANEL_ITEM_HEIGHT;
        else
        {
                if(h>0)
                        t->height = h * t->fh + t->fw/2+2*thick;
                else
                        t->height = -h;
        }
        t->label_x = t->x + (t->width - t->label_width)/2;
        t->label_y = t->y + (t->height - t->label_height)/2 + t->label_ascent;

        /* Display the text item */
        v_DisplayButton(t);

        if(mode == 0)
            Number_of_buttons ++;
        else
            Number_of_viewnix_buttons ++;

        *ID = t;
        XFreeGC(lib_display,gc);
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VDeleteButtonItem                                 *
 *      Description     : This function removes a given Button Item from*
 *                        the list. Its memory space is freed.          *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  ID - Button Item ID.                         *
 *      Side effects    : An entry is removed from the Button Item      *
 *                        list. The region occupied by the Item is      *
 *                        erased.                                       *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified 12/21/94 v_DeleteButton0 called
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VDeleteButtonItem ( BUTTON* ID )
{
        return (v_DeleteButton0(ID, 0));
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DeleteButton0                               *
 *      Description     : This function removes a given Button Item from*
 *                        the public or private list.
 *                        Its memory space is freed.
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  ID - Button Item ID.                         *
 *                         mode - 0 for public list, 1 for private list
 *      Side effects    : An entry is removed from the Button Item      *
 *                        list. The region occupied by the Item is      *
 *                        erased.                                       *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDeleteButtonItem
 *      History         : Written 12/21/94 (taken from VDeleteButtonItem)
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_DeleteButton0 ( BUTTON* ID, int mode )
#if 0
BUTTON *ID;
int mode;       /* 0=USER LEVEL,  1=LIBRARY LEVEL */
#endif
{
        int i;
        BUTTON *t, *t2;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VDeleteButtonItem.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(mode == 0) t = t2 = Button;
        else t = t2 = ViewnixButton;

        if(ID == NULL) return(266);


        /* find the ITEM to DELETE */
        i=0;
        while(t!=NULL && t != ID)
        {
                t2 = t;
                t = t->next;
                i++;
        }

        if(t==NULL) return(266);

        v_EraseButton(t);

        if(i==0)
        {
                if(mode == 0) Button = t->next;
                else ViewnixButton = t->next;
                free(t); t=NULL;
        }
        else
        {
                t2->next = t->next;
                free(t); t=NULL;
        }

        if(mode == 0) Number_of_buttons--;
        else Number_of_viewnix_buttons--;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetButtonItemLabel                               *
 *      Description     : This function returns the label of a given    *
 *                        Button Item.                                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *                         269 - no Text item found.                    *
 *                         271 - invalid Button Item value pointer.     *
 *      Parameters      :  ID - ID of the given button item.            * 
 *                         val - character string in which label is     *
 *                              returned.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VGetButtonItemLabel ( BUTTON* ID, char* val )
{
        BUTTON *t;


        t = Button;
        if(val == NULL) return(271);
        if(t==NULL) return(266);

        while(t != ID)
        {
                t = t->next;
                if(t == (BUTTON *) NULL)
                        return(269);
        }

        strcpy(val, t->label);

        return(0);
}



/************************************************************************
 *                                                                      *
 *      Function        : v_EraseButton                                 *
 *      Description     : This function erases the Item from the screen.*
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  item - ID for the Button item.               *
 *      Side effects    : The region occupied by the Item is erased.    *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_EraseButton ( BUTTON* item )
{

        VClearWindow(item->win, item->x, item->y, item->width+1, item->height+1);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VSetButtonItemLabel                               *
 *      Description     : This function sets the label of a given Button*
 *                        Item.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  item - ID for the Button item.               *
 *                         label - character string of new value.       *
 *      Side effects    : The value of the Item is changed. The Item is *
 *                        redisplayed.                                  *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VSetButtonItemLabel ( BUTTON* item, char* label )
{
        if(item == NULL) return(266);

        /* Erase previous Button */
        if(item->state == 1)
                v_EraseButton(item);

        /* Change its Label */
        item->label[0] = 0;
        strcpy(item->label, label);


        if(item->state == 1)
                v_DisplayButton(item);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VRemoveButtonItem                                 *
 *      Description     : This function turns a Button item OFF.        *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  item - ID for the Button item.               *
 *      Side effects    : The 'state' element of the Item's structure   *
 *                        is changed. he Item is erased, and it         *
 *                        won't "exist" for interaction purposes until  *
 *                        it is turned ON.                              *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VRemoveButtonItem ( BUTTON* item )
{
        if(item == NULL) return(266);

        if(item->state == 1)
        {
                item->state = 0;
                v_EraseButton(item);
        }
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VRedisplayButtonItem                                  *
 *      Description     : This function turns a Button item ON.         *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  item - ID for the Button item.               *
 *      Side effects    : The 'state' element of the Item's structure   *
 *                        is changed. The item is redisplayed.          *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VRedisplayButtonItem ( BUTTON* item )
{
        if(item == NULL) return(266);

        if(item->state == 0)
        {
                item->state = 1;
                v_DisplayButton(item);
        }
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VSetButtonItemState                                *
 *      Description     : This function turns a Button item to Pressed  *
 *                        or Released.                                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  item - ID for the Button item.               *
 *                         mode - whether it is a library call or from  *
 *                              outside.                                *
 *      Side effects    : The 'mode' element of the Item's structure    *
 *                        is changed.                                   *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VSetButtonItemState ( BUTTON* item, int mode )
#if 0
BUTTON *item;
int     mode; /* 0=RELEASED,  1=PRESSED */
#endif
{
        if(item == NULL) return(266);

        v_EraseButton(item);
        item->mode = mode;
        v_DisplayButton(item);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetButtonItemMode                                *
 *      Description     : This function gets a Button item mode (Pressed*
 *                        or Released).                                 *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  item - ID for the Button item.               *
 *                         mode - variable in which mode is to be       *
 *                              returned.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int VGetButtonItemMode ( BUTTON* item, int* mode )
{
        if(item == NULL) return(266);

        *mode = item->mode;
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_DisplayButton                               *
 *      Description     : This function displays a given Button Item.   *
 *      Return Value    :  0 - work successfully.                       *
 *                         266 - invalid Button Item ID.                *
 *      Parameters      :  item - ID for the Button item.               *
 *      Side effects    : Displays th Button Item on the window         *
 *                        (if item->state==ON).                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on February 16, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
int v_DisplayButton ( BUTTON* item )
{
        XTextItem tlabel;
        GC gc;
        int i;
        int offset;
        char text[500];
        XGCValues vals;
        GC buttonitemgc;

        int     x1,y1,
                x2,y2,
                x3,y3,
                x4,y4,
                x5,y5,
                x6,y6,
                x7,y7,
                x8,y8;
                
        if (lib_cmap_created == 0) {
           printf("The error occurred in the function v_DisplayButton.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        if(item == NULL) return(266);

        /* if Text Item is OFF, don't display it */
        if(item->state == 0) return(0);

        /* Get GC */
        VGetWindowGC(item->win,&gc);    /* graphics context */

        strcpy(text, item->label);
        tlabel.chars = &text[0];
        tlabel.nchars = strlen(text);
        tlabel.delta = 0;
        if(item->win == lib_wins[0].win)
                tlabel.font = lib_wins[0].font->fid;
        else if(item->win == lib_wins[1].win)
                tlabel.font = lib_wins[1].font->fid;
        else if(item->win == lib_wins[2].win)
                tlabel.font = lib_wins[2].font->fid;
        else if(item->win == lib_wins[3].win)
                tlabel.font = lib_wins[3].font->fid;

        /* Draw FRAME */
        x1 = item->x;
        y1 = item->y;
        x2 = item->x + item->width;
        y2 = y1;
        x3 = x2;
        y3 = item->y + item->height;
        x4 = x1;
        y4 = y3;
        x5 = x1+item->thick;
        y5 = y1+item->thick;
        x6 = x2-item->thick;
        y6 = y2+item->thick;
        x7 = x3-item->thick;
        y7 = y3-item->thick;
        x8 = x4+item->thick;
        y8 = y4-item->thick;

        buttonitemgc = XCreateGC(lib_display,item->win,0,&vals);
        vals.line_width=1;
        XChangeGC(lib_display,buttonitemgc,GCLineWidth,&vals);
        
        item->thick = 1;
        offset = item->thick;

        if(item->mode == 0)
        {
        /***********fill the rectangles***********/
            XSetForeground(lib_display,buttonitemgc,
                        lib_reserved_colors[2].pixel);
            XFillRectangle(lib_display,item->win,buttonitemgc,
                           item->x,item->y,item->width,item->height);
            XFlush(lib_display);
        /***********top white lines***********/
            XSetForeground(lib_display,buttonitemgc,
                               lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x+offset,
                        item->y+offset,
                        item->x+item->width-offset,
                        item->y+offset);
            XFlush(lib_display);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x-offset,
                        item->y+offset,
                        item->x-offset,
                        item->y+item->height-offset);
            XFlush(lib_display);
 
        /***********bottom dark lines***********/
            XSetForeground(lib_display,buttonitemgc,
                        lib_reserved_colors[3].pixel);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x+offset,
                        item->y+item->height-offset,
                        item->x+item->width-offset,
                        item->y+item->height-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x+item->width-offset,
                        item->y+offset,
                        item->x+item->width-offset,
                        item->y+item->height-offset);
            XFlush(lib_display);

        /****************Draw LABEL******************/
            XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel);
            XDrawText(lib_display,item->win,gc,item->label_x,item->label_y, 
                        &tlabel,1);
            XFlush(lib_display);
        }

        /*******If PRESSED mode**********/
        if(item->mode == 1)
        {
                /*invert content only if BUTTON(no inversion if TOGGLE)*/
                if(item->type == 0)
                        XSetFunction(lib_display, gc, GXxor);

                /*fill with lines only if button/toggle has empty label*/
                if(strlen(item->label) == 0)
                        for(i=x5+1; i<x6; i+=2)
                        XDrawLine(lib_display, item->win, gc, i,y5+1, i,y8-1);

                XSetFunction(lib_display, gc, GXcopy);

        /***********fill the rectangles***********/
            XSetForeground(lib_display,buttonitemgc,
                        lib_reserved_colors[3].pixel);
            XFillRectangle(lib_display,item->win,buttonitemgc,
                           item->x,item->y,item->width,item->height);
            XFlush(lib_display);
        /***********top dark lines***********/
            XSetForeground(lib_display,buttonitemgc,
                               lib_reserved_colors[2].pixel);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x+offset,
                        item->y-offset,
                        item->x+item->width-offset,
                        item->y-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x-offset,
                        item->y+offset,
                        item->x-offset,
                        item->y+item->height-offset);
            XFlush(lib_display);

        /***********bottom white lines***********/
            XSetForeground(lib_display,buttonitemgc,
                        lib_reserved_colors[4].pixel);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x+offset,
                        item->y+item->height-offset,
                        item->x+item->width-offset,
                        item->y+item->height-offset);
            XFlush(lib_display);
            XDrawLine(lib_display,item->win,buttonitemgc,
                        item->x+item->width-offset,
                        item->y+offset,
                        item->x+item->width-offset,
                        item->y+item->height-offset);
            XFlush(lib_display);

        /*******Draw LABEL********/
            XSetForeground(lib_display,buttonitemgc,
                        lib_reserved_colors[0].pixel);
            XDrawText(lib_display,item->win,buttonitemgc,
                        item->label_x,item->label_y,&tlabel,1);
            XFlush(lib_display);
        }

        XFreeGC(lib_display,gc);
        XFreeGC(lib_display,buttonitemgc);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_InsideButton                                *
 *      Description     : This function checks if a coordinate is inside*
 *                        a given Button item, given the item is within *
 *                        a given window and is ON.                     *
 *      Return Value    :  TRUE - is inside.                            *
 *                         FALSE - is not inside, item is OFF or        *
 *                              different window.                       *
 *                         266 - invalid Button Item ID.                *
 *                         269 - no Text item found.                    *
 *      Parameters      :  item - ID for the Button item.               *
 *                         x,y - corrdinate.                            *
 *                         window - window in which coordinate is being *
 *                              checked.                                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
static int v_InsideButton ( BUTTON* item, int x, int y, Window window )
{
        if(item == NULL) return(266);

        if(     x > item->x  &&  x < item->x + item->width  &&
                y > item->y  &&  y < (item->y + item->height) &&
                item->win == window && item->state == 1)
                        return(1);
        else
                        return(0);
                
}


/************************************************************************
 *                                                                      *
 *      Function        : VCheckButtonItemEvent                             *
 *      Description     : This function checks if an event occured on an*
 *                        Item. If it did, then take the appropriate    *
 *                        actions according to the type of event.       *
 *      Return Value    :  0 - work successfully.                       *
 *                         269 - no Text item found.                    *
 *      Parameters      :  event - XWindow event.                       *
 *      Side effects    : The value, mode (enabled, disabled) of an     *
 *                        Item is changed according to the type of event*
 *                         BUTTON:                                      *
 *                              LEFT-BUTTON - Calls the "Notify         *
 *                                Procedure" associated with the button.*
 *                         TOGGLE:                                      *
 *                              LEFT-BUTTON - If mode is ON => change it*
 *                                to OFF.                               *
 *                              LEFT-BUTTON - If mode is OFf => change  *
 *                                it to ON                              *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 11, 1992 by              *
 *                        Roberto J. Goncalves.                         *
 *                        Modified on February 16, 1993 by Krishna Iyer.*
 *                        Modified 12/22/94 private list checked
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VCheckButtonItemEvent ( XEvent* event )
{
        int i;
        int type, mode;
        int found=0;
        BUTTON *t;
        int px, py;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VCheckButtonItemEvent.\n");
           printf("Please call VCreateColormap before ");
           printf("calling this function.\n");
           kill(getpid(),LIB_EXIT);
        }

        type = event->type;


        /* if event is of no interest */        
        if(type != KeyPress && type != ButtonPress  &&  type != KeyRelease) return(0);

        /* if no Text items around */
        if(Number_of_buttons==0 && Number_of_viewnix_buttons==0) return(0);

        /* get location of the event */ 
        px = event->xkey.x;
        py = event->xkey.y;


 
    if(type == ButtonPress && event->xbutton.button == LEFT_BUTTON)
    {
        mode = Number_of_buttons? 0: 1;
        t = mode? ViewnixButton: Button;
        i = 0;
        while(found == 0  &&  t != NULL)
        {
            if(v_InsideButton(t, px, py, event->xany.window) )
                found = 1;
            else
            {   t = t->next;
                if (t==NULL && mode==0 && Number_of_viewnix_buttons)
                {   t = ViewnixButton;
                    mode = 1;
                }
            }
 
            i++;
        }
 
    }
 


        /* NO BUTTON ITEM WAS FOUND */
        if(found == 0) return(269);


        /* BUTTON */
        if(t->type == 0)
        {
                /* Change mode to Pressed */
                v_EraseButton(t);
                VSetButtonItemState(t, 1);

                /* Call Notufy Procedure */
                if( t->func != NULL)
                        (t->func)();

                /* DRAW NORMAL BUTTON */
                v_EraseButton(t);
                VSetButtonItemState(t, 0);
        }
        /* TOGGLE */
        else
        if(t->type == 1)
        {
                if(t->mode == 0)
                {
                        v_EraseButton(t);
                        t->mode = 1;
                }
                else
                {
                        v_EraseButton(t);
                        t->mode = 0;
                }
                v_DisplayButton(t);

                /* Call Notify Procedure */
                if( t->func != NULL)
                        (t->func)();
        }

        /* return the index of the text item */
        return(0);

}
