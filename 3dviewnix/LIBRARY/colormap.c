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
 *      Filename  : colormap.c                                          *
 *      Ext Funcs : VCreateColormap, VRestoreDefaultColormap,           *
 *                  VGetReservedColors, VGetFreeColorcells,             *
 *                  VGetColormap, VPutColormap, VGetColorcellStatus,    *
 *                  v_set_visual_class, VGetVisual, VGetDepth,          *
 *                  VReadColorcomFile, VWriteColorcomFile.                       *
 *      Int Funcs : v_create_read_write_cmap,                           *
 *                  v_assigned_reserved_colorcells,                     *
 *                  v_set_reserved_colors, v_assigned_color,            *
 *                  v_set_visual, v_get_cmap_id.                        *
 *                                                                      *
 ************************************************************************/

#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include "Vlibrary.h"
#include "3dv.h"
#if defined(__cplusplus) || defined(c_plusplus)
    #define  CLASS  c_class
#else
    #define  CLASS  class
#endif

static int v_set_reserved_colors ( XColor* cmap_cells, char* cmap_status,
    int entries, int* avail_cells );
/************************************************************************
 *                                                                      *
 *      Function        : VCreateColormap                               *
 *      Description     : This function installs
 *                        the colormap using default gray values        *
 *                        with overlays specified by the argument novl  *
 *                        of v_create_windows. In 3DVIEWNIX, we
 *                        consider three visual classes - GrayScale,    *
 *                        PseudoColor, and DirectColor. This function   *
 *                        can be called once for each process and       *
 *                        should be called after functions VSetup, *
 *                        v_set_visual_class, and v_create_windows are called*
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - file open error.                         *
 *                         252 - colormap is already created.           *
 *      Parameters      :  None.                                        *
 *      Side effects    : Only consider 3 visual classes - grayscale,   *
 *                        pseudocolor, and directcolor are allowed.     * 
 *      Entry condition : If valid visual class is not available on     *
 *                        this device, or function v_create_windows is  *
 *                        not called earlier, this function will        *
 *                        output a proper message to the standard error *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VGetReservedColors, VGetFreeColorcells,       *
 *                        VGetColormap, VPutColormap, VWriteColorcomFile,    *
 *                        VReadColorcomFile, VGetColorcellStatus,           *
 *                        VGetVisual, VChangeNumberOfOverlays,          *
 *                        v_create_windows, v_set_visual_class.         * 
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 10/14/94 lib_colormap_colors
 *                        initialized by Dewey Odhner.
 *                        Modified 12/2/94 to handle visual class
 *                        changes by Dewey Odhner.
 *                        Modified 12/13/94 inheritance and
 *                        VWriteColorcomFile call removed-- default colormap
 *                        is now always installed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VCreateColormap ( void )
{
        int result, i, j;
        int visual_class,num_ovl,entries;
        unsigned long pixel;

        if (lib_wins_created == 0) {
            printf("The error occurred in the function VCreateColormap.\n") ;
            printf("Please call VSetup before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (lib_visual == NULL) {
            printf("The error occurred in the function VCreateColormap.\n") ;
            printf("Unfortunately the visual class specified is invalid.\n");
            printf("Please check with ") ;
            printf("the function VSetup.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (lib_cmap_created != 0) return(252);
        else lib_cmap_created=1;

        lib_cmap_entries=lib_cmap_gray_entries=lib_visual->map_entries;
        lib_cmap_status=(char *)malloc(sizeof(char)*(lib_visual->map_entries));
        if (lib_cmap_status == NULL) return(1);
        for (i=0;i<lib_visual->map_entries;i++) 
            lib_cmap_status[i]=0;
        result=v_create_read_write_cmap();
        if (result != 0) return(result);
        result=v_read_color_com(lib_reserved_colors);
        lib_inherit=0;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_create_read_write_cmap                      *
 *      Description     : Installs existing colormap only for GrayScale,*
 *                        PseudoColor or DirectColor visual class.      *
 *                        this function will install      *
 *                        colormap with the overlays specified by       *
 *                        v_create_windows.                             *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         2 - read error.                              *
 *                         4 - file open error.                         *
 *      Parameters      :  opt - Specifies teh flag to create and       *
 *                               initialize or initialize only.         *
 *                               0 - install the old colormap.          *
 *                               1 - create and install new colormap.   *
 *      Side effects    : lib_colormap_colors set
 *      Entry condition : If valid visual class is not available on     *
 *                        this device, or function v_create_windows is  *
 *                        not called earlier, this function will        *
 *                        not be responsible.
 *                        lib_cmap_entries, lib_cmap_status must be set.
 *      Related funcs   : VCreateColormap, v_create_windows.            *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 10/14/94 lib_colormap_colors set
 *                        by Dewey Odhner.
 *                        Modified 12/1/94 to handle visual class
 *                        changes by Dewey Odhner.
 *                        Modified 12/13/94 inheritance removed--
 *                        default colormap
 *                        is now always installed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_create_read_write_cmap ( void )
{
        int available_entries, entries ;
        int i, j, rgb, result, novl, visual_class;
        ViewnixColor reserved_colors[NUM_OF_RESERVED_COLORCELLS] ;
        unsigned long pixel ;

        entries=lib_visual->map_entries ;
        if (lib_colormap_colors == NULL)
            lib_colormap_colors=(XColor *)malloc(entries*sizeof(XColor)) ;
        if (lib_colormap_colors == NULL) return(1) ;
        v_set_reserved_colors(lib_colormap_colors,lib_cmap_status,entries,
                            &available_entries) ;
        switch (lib_num_of_overlays) {
            case 0 : 
                lib_cmap_gray_entries=available_entries ;
                for (i=0; i<lib_cmap_gray_entries; i++) {
                    lib_cmap_status[i]=1 ;   /* initialize colormap status 
                                                array, 1 is for gray/pseudo 
                                                colorcells */
                    if (lib_inherit == 0) {
                        rgb=(0xFFFF*i)/lib_cmap_gray_entries;
                        if ( (lib_visual->CLASS) == DirectColor)
                            pixel=
                            (i << lib_red_mask_shift)|
                            (i << lib_green_mask_shift)|
                            (i << lib_blue_mask_shift);
                        else pixel=i ;
                        v_assigned_color(&lib_colormap_colors[i],pixel,rgb,rgb,rgb) ;
                    }
                }
                break ;
            case 1 :
                lib_cmap_gray_entries=available_entries/2 ;
                for (i=0; i<lib_cmap_gray_entries; i++) {
                    lib_cmap_status[i*2]=1 ;  /* for gray/pseudo colorcells */
                    lib_cmap_status[i*2+1]=2 ; /* for overlay colorcells */
                    if (lib_inherit == 0) {
                        rgb=(0xFFFF*i)/lib_cmap_gray_entries;
                        if (lib_visual->CLASS == DirectColor)
                            pixel=
                            (i*2 << lib_red_mask_shift)|
                            (i*2 << lib_green_mask_shift)|
                            (i*2 << lib_blue_mask_shift);
                        else pixel=i*2 ;
                        v_assigned_color(&lib_colormap_colors[i*2],pixel,rgb,
                            rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*2+1],
                               pixel | lib_overlay_planes[0],rgb,rgb,rgb) ;
                    }
                }
                break ;
            default :
                lib_cmap_gray_entries=available_entries/4 ;
                for (i=0; i<lib_cmap_gray_entries; i++) {
                    lib_cmap_status[i*4]=1 ;  /* for gray/pseudo colorcells */
                    lib_cmap_status[i*4+1]=lib_cmap_status[i*4+2]
                                          =lib_cmap_status[i*4+3]
                                          =2 ; /* for overlay colorcells */
                    if (lib_inherit == 0) {
                        rgb=(0xFFFF*i)/lib_cmap_gray_entries;
                        if (lib_visual->CLASS == DirectColor)
                        pixel=
                        ((i*(1<<lib_num_of_overlays)) << lib_red_mask_shift)|
                        ((i*(1<<lib_num_of_overlays)) << lib_green_mask_shift)|
                        ((i*(1<<lib_num_of_overlays)) << lib_blue_mask_shift);
                        else pixel=(i*(1<<lib_num_of_overlays)) ;
                        v_assigned_color(&lib_colormap_colors[i*4],pixel,rgb,
                            rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*4+1],
                            pixel | lib_overlay_planes[0],rgb,rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*4+2],
                            pixel | lib_overlay_planes[1],rgb,rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*4+3],
                            pixel | lib_overlay_planes[0] | 
                            lib_overlay_planes[1],rgb,rgb,rgb) ;
                    }
                }
                break ;
        }
        XStoreColors(lib_display,lib_cmap,lib_colormap_colors,entries) ;
        XInstallColormap(lib_display,lib_cmap) ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_assigned_reserved_colorcells                *
 *      Description     : This function will compute and assign pixel   *
 *                        values of text, border, menu background,      *
 *                        dialog window background, button window       *
 *                        background, image window background, title    *
 *                        window background, overlay1, and if available *
 *                        overlay2. Because the maximum number of       *
 *                        overlays is 2, the pixel value of image window*
 *                        background mult be multiplied by 4, and the   *
 *                        following two pixels values are overlay1 and  *
 *                        overlay2. For machines with 8-bit visual class*
 *                        we have only one overlay.                     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  entries - Specifies the number of entries    *
 *                                   in the colormap.                   *
 *                         reserved_colors - return the array of colors *
 *                                   and colorcells reserved by the     *
 *                                   3dviewnix package.                 *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCreateColormap.                              *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int v_assigned_reserved_colorcells ( ViewnixColor* reserved_colors,
    int entries )
{
        int rest, i ;

        if(lib_depth <= 8)
          rest=entries%2; /*image window background has to be multiplied by 4,
                            and following by overlay1 and overlay2, then we
                            can on/off overlay without recomputing images,
                            just by changing colormap. */ 

        if(lib_depth > 8) rest=entries%4;

        switch (rest) {
            case 0 :
                reserved_colors[0].pixel=entries-10; /*text/border*/
                reserved_colors[1].pixel=entries-9; /*title/dialog/button bg*/
                reserved_colors[2].pixel=entries-8; /*special color 1*/ 
                reserved_colors[3].pixel=entries-7; /*special color 2*/
                reserved_colors[4].pixel=entries-6; /*special color 3*/
                reserved_colors[5].pixel=entries-5; /*menu background*/
                reserved_colors[6].pixel=entries-2; /*text on image color*/
                reserved_colors[7].pixel=entries-1; /*text image w/overlay*/
                reserved_colors[8].pixel=entries-3; /*for overlay1*/
                reserved_colors[9].pixel=entries-4; /*image win background*/
                reserved_colors[10].pixel=entries-12; /*highlight color*/
                reserved_colors[11].pixel=entries-11; /*color for help*/
                break ;
            case 1 :
                reserved_colors[0].pixel=entries-1;
                reserved_colors[1].pixel=entries-10;
                reserved_colors[2].pixel=entries-9;
                reserved_colors[3].pixel=entries-8;
                reserved_colors[4].pixel=entries-7;
                reserved_colors[5].pixel=entries-6;
                reserved_colors[6].pixel=entries-3;
                reserved_colors[7].pixel=entries-2;
                reserved_colors[8].pixel=entries-4;
                reserved_colors[9].pixel=entries-5;
                reserved_colors[10].pixel=entries-11;
                reserved_colors[11].pixel=entries-12;
                break ;
            case 2 :
                reserved_colors[0].pixel=entries-1;
                reserved_colors[1].pixel=entries-2;
                reserved_colors[2].pixel=entries-10;
                reserved_colors[3].pixel=entries-9;
                reserved_colors[4].pixel=entries-8;
                reserved_colors[5].pixel=entries-7;
                reserved_colors[6].pixel=entries-3;
                reserved_colors[7].pixel=entries-4;
                reserved_colors[8].pixel=entries-5;
                reserved_colors[9].pixel=entries-6;
                break ;
            case 3 :
                reserved_colors[0].pixel=entries-1;
                reserved_colors[1].pixel=entries-2;
                reserved_colors[2].pixel=entries-3;
                reserved_colors[3].pixel=entries-10;
                reserved_colors[4].pixel=entries-9;
                reserved_colors[5].pixel=entries-8;
                reserved_colors[6].pixel=entries-4;
                reserved_colors[7].pixel=entries-5;
                reserved_colors[8].pixel=entries-6;
                reserved_colors[9].pixel=entries-7;
                break ;
        }
        if (lib_visual->CLASS == DirectColor) {
            for (i=0; i<NUM_OF_RESERVED_COLORCELLS; i++) 
                reserved_colors[i].pixel=
                (reserved_colors[i].pixel << lib_red_mask_shift)|
                (reserved_colors[i].pixel << lib_green_mask_shift)|
                (reserved_colors[i].pixel << lib_blue_mask_shift) ;
        }

        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_set_reserved_colors                         *
 *      Description     : This function assigns 3dviewnix reserved      *
 *                        colors to an array cmap_cells, and modify     *
 *                        colormap entry status. Return the number of   *
 *                        colorcells available after the reserved colors*
 *                        allocated. No value returns to this function. *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  entries - Specifies the number of entries    *
 *                                   in the colormap.                   *
 *                         cmap_cells - Return an array of the color    *
 *                                   definition structures.             *
 *                         cmap_status - Return an array of the colormap*
 *                                   entry status.                      *
 *                         avail_cells - Returns the number of          *
 *                                   colorcells available.              *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCreateColormap.                              *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
static int v_set_reserved_colors ( XColor* cmap_cells, char* cmap_status,
    int entries, int* avail_cells )
{
        int i, pixel, pre_pixel,cel ;

        pre_pixel= -1 ;
        for (i=0; i<NUM_OF_RESERVED_COLORCELLS; i++) {
            pixel=lib_reserved_colors[i].pixel ;
            if (lib_visual->CLASS== DirectColor)
               cel= (pixel&lib_visual->red_mask)>>lib_red_mask_shift;
            else
               cel= pixel;    
            cmap_cells[cel].pixel=pixel ;
            cmap_cells[cel].red=lib_reserved_colors[i].red ;
            cmap_cells[cel].green=lib_reserved_colors[i].green ;
            cmap_cells[cel].blue=lib_reserved_colors[i].blue ;
            cmap_cells[cel].flags=DoRed | DoGreen | DoBlue ;
            cmap_status[cel]=3 ; /* reserved colorcell */
            if (pre_pixel != cel) {
                entries-- ;
                pre_pixel=cel ;
            }
        }
        *avail_cells=entries ;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VRestoreDefaultColormap                       *
 *      Description     : This function installs the default colormap   *
 *                        which is the grayscale colormap.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - file open error.                         *
 *      Parameters      :  None.                                        *
 *      Side effects    : lib_colormap_colors is set
 *      Entry condition : If valid visual class is not available on     *
 *                        this device, or function v_create_windows is    *
 *                        not called earlier, this function will        *
 *                        output a proper message to the standard error *
 *                        stream, produce a core dump file, and exit    *
 *                        from the current process.                     *
 *      Related funcs   : VCreateColormap, v_create_windows.            *
 *      History         : Written on April 7, 1993 by Krishna Iyer.     *
 *                        Modified 10/14/94 lib_colormap_colors set
 *                        by Dewey Odhner.
 *                        Modified 12/1/94 to handle visual class
 *                        changes by Dewey Odhner.
 *                        Modified 12/13/94 lib_inherit ignored
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VRestoreDefaultColormap ( void )
{
        int available_entries,entries;
        int i, j, rgb,result,novl,visual_class;
        ViewnixColor reserved_colors[NUM_OF_RESERVED_COLORCELLS];
        unsigned long pixel;
 
        if (lib_wins_created == 0) {
            printf(
              "The error occurred in the function VRestoreDefaultColormap.\n");
            printf("Programmer must call VSetup before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        entries=lib_visual->map_entries;
        v_set_reserved_colors(lib_colormap_colors,lib_cmap_status,entries,
                            &available_entries);

        switch (lib_num_of_overlays) {
            case 0 :
                lib_cmap_gray_entries=available_entries ;
                for (i=0; i<lib_cmap_gray_entries; i++) 
                {
                    lib_cmap_status[i]=1;
                    rgb=(0xFFFF*i)/lib_cmap_gray_entries;
                    pixel=i;
                    v_assigned_color(&lib_colormap_colors[i],
                            pixel,rgb,rgb,rgb);
                }
                break;

            case 1 :
                lib_cmap_gray_entries=available_entries/2;
                for (i=0; i<lib_cmap_gray_entries; i++) 
                {
                    lib_cmap_status[i*2]=1;
                    lib_cmap_status[i*2+1]=2;
                    rgb=(0xFFFF*i)/lib_cmap_gray_entries;
                    pixel=i*2;
                    v_assigned_color(&lib_colormap_colors[i*2],
                             pixel,rgb,rgb,rgb);
                    v_assigned_color(&lib_colormap_colors[i*2+1],
                             pixel | lib_overlay_planes[0],rgb,rgb,rgb);
                }
                break;

             default :
                lib_cmap_gray_entries=available_entries/4;
                for (i=0; i<lib_cmap_gray_entries; i++) 
                {
                    lib_cmap_status[i*4]=1;
                    lib_cmap_status[i*4+1]=lib_cmap_status[i*4+2]
                                          =lib_cmap_status[i*4+3]
                                          =2;
                    rgb=(0xFFFF*i)/lib_cmap_gray_entries;
                    pixel=(i*(1<<lib_num_of_overlays));
                    v_assigned_color(&lib_colormap_colors[i*4],pixel,rgb,rgb,rgb);
                    v_assigned_color(&lib_colormap_colors[i*4+1],
                         pixel | lib_overlay_planes[0],rgb,rgb,rgb);
                    v_assigned_color(&lib_colormap_colors[i*4+2],
                         pixel | lib_overlay_planes[1],rgb,rgb,rgb);
                    v_assigned_color(&lib_colormap_colors[i*4+3],
                         pixel | lib_overlay_planes[0] |
                         lib_overlay_planes[1],rgb,rgb,rgb);
                }
                break;
        }
 
        XStoreColors(lib_display,lib_cmap,lib_colormap_colors,entries) ;
        XInstallColormap(lib_display,lib_cmap) ;
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetReservedColors                            *
 *      Description     : This function returns the 3DVIEWNIX           *
 *                        reserved colors and colorcells.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         4 - file open error.                         *
 *      Parameters      :  colors - return a pointer to an array of the *
 *                                  3dviewnix color definition          *
 *                                  structures with the reserved        *
 *                                  colors. The following is the        *
 *                                  meaning of each element represented.*
 *                                      0 - border and text.            *
 *                                      1 - dialog/button/title window. *
 *                                      2 - special color for 3d effect.*
 *                                      3 - special color for 3d effect.*
 *                                      4 - special color for 3d effect.*
 *                                      5 - menu background color.      *
 *                                      6 - text color on image.        *
 *                                      7 - special color for 3d effect.*
 *                                      8 - image win color if ovl1 ON. *
 *                                      9 - image win color if ovl1 OFF.*
 *      Side effects    : None.                                         *
 *      Entry condition : If colors is NULL, or function VCreateColormap*
 *                        is not called earlier, this function will     *
 *                        print a proper message to the standard error  *
 *                        stream, produce a core dump, and exit from the*
 *                        current process.                              *
 *      Related funcs   : VCreateColormap, VGetFreeColorcells,          *
 *                        VGetColormap, VPutColormap, VWriteColorcomFile,    *
 *                        VReadColorcomFile, VGetColorcellStatus,           *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 12/14/94 colors returned Dewey Odhner
 *                                                                      *
 ************************************************************************/
int VGetReservedColors ( ViewnixColor colors[NUM_OF_RESERVED_COLORCELLS] )
{
        int result, j;

        if (lib_cmap_created == 0) {
           printf("The error occurred in the function VGetReservedColors.\n");
           printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (colors == NULL) 
            v_print_fatal_error("VGetReservedColors",
                "The pointer of colors should not be NULL.",0) ;
        result=v_read_color_com(colors);
        if (result)
            for (j=0; j<NUM_OF_RESERVED_COLORCELLS; j++) {
                colors[j].red = lib_reserved_colors[j].red;
                colors[j].green = lib_reserved_colors[j].green;
                colors[j].blue = lib_reserved_colors[j].blue;
            }
        v_assigned_reserved_colorcells(colors,lib_visual->map_entries);
        return(result);
} 

/************************************************************************
 *                                                                      *
 *      Function        : VGetFreeColorcells                            *
 *      Description     : This function is for obtaining the colorcells *
 *                        available (after the reserving colors and     *
 *                        overlays), and the number of colorcells       *
 *                        available. This function will allocate the    *
 *                        memory space for array and return a pointer to*
 *                        array at *avail_colorcells. The caller can use*
 *                        C function free to free space at              *
 *                        *avail_colorcells when done with this array.  *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  avail_colorcells - return a pointer to an    *
 *                                    array of the colorcells available *
 *                                    in the colormap.                  *
 *                         num_avail_colorcells - return the number of  *
 *                                    colorcells available in the array *
 *                                    avail_colorcells.                 *
 *      Side effects    : None.                                         *
 *      Entry condition : If avail_colorcells or num_avail_colorcells   *
 *                        is NULL, or function VCreateColormap is not   *
 *                        called earlier, this funciton will print a    *
 *                        proper message to the standard error stream,  *
 *                        produce a core dump, and exit from the        *
 *                        current process.                              *
 *      Related funcs   : VCreateColormap, VGetReservedColors,          *
 *                        VGetColormap, VPutColormap, VWriteColorcomFile,    *
 *                        VReadColorcomFile, VGetColorcellStatus.           *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VGetFreeColorcells ( unsigned long** avail_colorcells,
    long* num_avail_colorcells )
{
        int i ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VGetFreeColorcells.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (avail_colorcells == NULL) 
            v_print_fatal_error("VGetFreeColorcells",
                "The pointer of avail_colorcells should not be NULL.",
                0) ;
        if (num_avail_colorcells == NULL) 
            v_print_fatal_error("VGetFreeColorcells",
                "The pointer of num_avail_colorcells should not be NULL.",
                0) ;
        *num_avail_colorcells=lib_cmap_gray_entries ;
        *avail_colorcells=(unsigned long *)malloc(lib_cmap_gray_entries*
                                                  sizeof(long)) ;
        if (*avail_colorcells == NULL) return(1) ;
        for (i=0; i<lib_cmap_gray_entries; i++)
            if(lib_visual->CLASS != DirectColor)
            (*avail_colorcells)[i]=(i*(1<<lib_num_of_overlays)) ;

            else if(lib_visual->CLASS == DirectColor)
            (*avail_colorcells)[i]=
                ((i*(1<<lib_num_of_overlays)) << lib_red_mask_shift)|
                ((i*(1<<lib_num_of_overlays)) << lib_green_mask_shift)|
                ((i*(1<<lib_num_of_overlays)) << lib_blue_mask_shift);
        return(0) ;
} 
        
/************************************************************************
 *                                                                      *
 *      Function        : v_assigned_color                              *
 *      Description     : This function updates the colorcell values.   *
 *      Return Value    :  None.                                        *
 *      Parameters      :  pixel - Specifies the pixel value.           *
 *                         r, g, b - Specifies the RGB values.          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCreateColormap.                              *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int v_assigned_color ( XColor* color, unsigned long pixel,
    unsigned short r, unsigned short g, unsigned short b )
{
        color->pixel=pixel;
        color->red=r; 
        color->green=g;
        color->blue=b;
        color->flags=DoRed | DoGreen | DoBlue ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetColormap                                  *
 *      Description     : This function will return the R,G,B values in *
 *                        the colormap for the colorcell corresponding  *
 *                        to the pixel value specified in the pixel     *
 *                        member of each element of the ViewnixColor    *
 *                        structure colors. The R,G,B values will be    *
 *                        stored in the red, green, and blue members of *
 *                        the same structure.                           *
 *      Return Value    :  0 - work successfully.                       *
 *                         232 - the pixel is invalid index into        *
 *                               colormap.                              *
 *      Parameters      :  colors - an array of ViewnixColor structures.*
 *                                  In each one, caller sets pixel to   *
 *                                  indicate which colorcell in the     *
 *                                  colormap to return.                 *
 *                         ncolors - Specifies number of ViewnixColor   *
 *                                  structures in the array colors.     *
 *                         colors - Return an array of the RGB values   *
 *                                  in the red, green, and blue members *
 *                                  of the ViewnixColor structures.     *
 *      Side effects    : None.                                         *
 *      Entry condition : If ncolors is less than or equal to zero,     *
 *                        colors is NULL, or function VCreateColormap   *
 *                        is not called earlier, this funciton will     *
 *                        print a proper message to the standard error  *
 *                        stream, produce a core dump, and exit from    *
 *                        the current process.                          *
 *      Related funcs   : VGetReservedColormap, VGetFreeColorcells,     *
 *                        VReadColorcomFile, VPutColormap, VWriteColorcomFile,   *
 *                        VCreateColormap, VGetColorcellStatus.         *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 10/18/94 lib_colormap_colors used
 *                        by Dewey Odhner.
 *                        Modified 10/26/94 mask1 eliminated
 *                        by Dewey Odhner.
 *                        Modified 10/28/94 different DirectColor
 *                        components returned by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VGetColormap ( ViewnixColor* colors, int ncolors )
{
        int i;
        char msg[80] ;
        unsigned long max_pixel, red_pixel, green_pixel, blue_pixel;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VGetColormap.\n") ;
            printf("Programmer must call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (colors == NULL) 
            v_print_fatal_error("VGetColormap",
                "The parameter colors should not be NULL.",0) ;
        if (ncolors <= 0) {
            sprintf(msg,"The value of ncolors must be > 0\n") ;
            v_print_fatal_error("VGetColormap",msg,ncolors) ;
        }
        if (lib_visual->CLASS == DirectColor) { 
            max_pixel=
                ((lib_visual->map_entries-1) << lib_red_mask_shift)|
                ((lib_visual->map_entries-1) << lib_green_mask_shift)|
                ((lib_visual->map_entries-1) << lib_blue_mask_shift);
            for (i=0; i<ncolors; i++) {
                red_pixel=
                    (colors[i].pixel&lib_visual->red_mask)>>lib_red_mask_shift;
                if (red_pixel >= lib_visual->map_entries)
                    return(232) ;
                assert(lib_colormap_colors[red_pixel].pixel <= max_pixel);
                colors[i].red=lib_colormap_colors[red_pixel].red ;
                green_pixel=(colors[i].pixel&lib_visual->green_mask)>>
                    lib_green_mask_shift;
                if (green_pixel >= lib_visual->map_entries)
                    return(232) ;
                assert(lib_colormap_colors[green_pixel].pixel <= max_pixel);
                colors[i].green=lib_colormap_colors[green_pixel].green ;
                blue_pixel=(colors[i].pixel&lib_visual->blue_mask)>>
                    lib_blue_mask_shift;
                if (blue_pixel >= lib_visual->map_entries)
                    return(232) ;
                assert(lib_colormap_colors[blue_pixel].pixel <= max_pixel);
                colors[i].blue=lib_colormap_colors[blue_pixel].blue ;
            }
        }
        else {
            max_pixel=lib_visual->map_entries-1 ;
            for (i=0; i<ncolors; i++) {
                if (colors[i].pixel > max_pixel) return(232) ;
                colors[i].red=lib_colormap_colors[colors[i].pixel].red ;
                colors[i].green=lib_colormap_colors[colors[i].pixel].green ;
                colors[i].blue=lib_colormap_colors[colors[i].pixel].blue ;
            }
        }
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VPutColormap                                  *
 *      Description     : This function will update the RGB values of   *
 *                        each colormap entry specified by              *
 *                        colors[].pixel to the hardware colors closet  *
 *                        to those specified in red, green, and blue of *
 *                        the same structure. Each pixel value must be  *
 *                        a read/write cell(i.e. the status of that     *
 *                        colorcell is 1) or a reserved color cell      *
 *                        (i.e. the status of that colorcell is 3) and a*
 *                        valid index to colormap. If the colorcell to  *
 *                        be updated is reserved by an overlay or is a  *
 *                        read-only colorcell, this function will not   *
 *                        update that entry, but no error occurs.       *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         231 - try to update colormap when truecolor  *
 *                               is on.                                 *
 *                         232 - pixel invalid index into colormap.     *
 *      Parameters      :  colors - Specifies an array of ViewnixColor  *
 *                                  structures.                         *
 *                         ncolors - Specifies number of ViewnixColor   *
 *                                  structures in the array colors.     *
 *      Side effects    : lib_colormap_colors is set
 *      Entry condition : If ncolors is less than or equal to zero,     *
 *                        colors is NULL, or function VCreateColormap is*
 *                        not called earlier, this funciton will print  *
 *                        a proper message to the standard error stream,*
 *                        produce a core dump, and exit from the current*
 *                        process.                                      *
 *      Related funcs   : VGetReservedColormap, VGetFreeColorcells,     *
 *                        VReadColorcomFile, VGetColormap, VWriteColorcomFile,   *
 *                        VCreateColormap, VGetColorcellStatus.         *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 10/17/94 lib_colormap_colors set
 *                        by Dewey Odhner.
 *                        Modified 10/26/94 mask1 eliminated
 *                        by Dewey Odhner.
 *                        Modified 10/31/94 different DirectColor
 *                        components set by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VPutColormap ( ViewnixColor* colors, int ncolors )
{
        int j;
        char msg[80] ;
        unsigned long red_pixel, green_pixel, blue_pixel;
        unsigned long pixel, pixel1;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VPutColormap.\n") ;
            printf("Programmer must call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (colors == NULL) 
            v_print_fatal_error("VPutColormap",
                "The parameter colors should not be NULL.",0) ;
        if (ncolors <= 0) {
            sprintf(msg,"The value of ncolor must be > 0\n");
            v_print_fatal_error("VPutColormap",msg,ncolors) ;
        }
        if (lib_truecolor_on) return(231) ;
        switch (lib_visual->CLASS) {
            case GrayScale:
            case PseudoColor :
                for (j=0; j<ncolors; j++) {
                    if (colors[j].pixel >= lib_cmap_entries)
                        return(232);
                    pixel=colors[j].pixel ;
                    if (lib_cmap_status[pixel] == 3) {
                        lib_colormap_colors[pixel].red = colors[j].red;
                        lib_colormap_colors[pixel].green = colors[j].green;
                        lib_colormap_colors[pixel].blue = colors[j].blue;
                    }
                    if (lib_cmap_status[pixel] == 1) { 
                        lib_colormap_colors[pixel].red = colors[j].red;
                        lib_colormap_colors[pixel].green = colors[j].green;
                        lib_colormap_colors[pixel].blue = colors[j].blue;
                        if (lib_num_of_overlays > 0 && lib_overlay_on[0] == 0){
                            pixel1=pixel | lib_overlay_planes[0] ;
                            lib_colormap_colors[pixel1].red = colors[j].red;
                            lib_colormap_colors[pixel1].green= colors[j].green;
                            lib_colormap_colors[pixel1].blue = colors[j].blue;
                        }
                        if (lib_num_of_overlays== 2 && lib_overlay_on[1]== 0) {
                            pixel1=pixel | lib_overlay_planes[1] ;
                            lib_colormap_colors[pixel1].red = colors[j].red;
                            lib_colormap_colors[pixel1].green= colors[j].green;
                            lib_colormap_colors[pixel1].blue = colors[j].blue;
                        }
                        if (lib_num_of_overlays == 2 && lib_overlay_on[0] == 0
                                && lib_overlay_on[1] == 0) {
                            pixel1=pixel | lib_overlay_planes[0] | 
                                  lib_overlay_planes[1] ;
                            lib_colormap_colors[pixel1].red = colors[j].red;
                            lib_colormap_colors[pixel1].green= colors[j].green;
                            lib_colormap_colors[pixel1].blue = colors[j].blue;
                        }
                    }
                } 
                break ;
            case DirectColor :
                for (j=0; j<ncolors; j++) {
                    red_pixel=(colors[j].pixel&lib_visual->red_mask)>>
                        lib_red_mask_shift;
                    if (red_pixel >= lib_cmap_entries)
                        return(232) ;
                    green_pixel=(colors[j].pixel&lib_visual->green_mask)>>
                        lib_green_mask_shift;
                    if (green_pixel >= lib_cmap_entries)
                        return(232) ;
                    blue_pixel=(colors[j].pixel&lib_visual->blue_mask)>>
                        lib_blue_mask_shift;
                    if (blue_pixel >= lib_cmap_entries)
                        return(232) ;

                    if (lib_cmap_status[red_pixel] == 3) 
                        lib_colormap_colors[red_pixel].red = colors[j].red;
                    else
                    if (lib_cmap_status[red_pixel] == 1) 
                    {
                        lib_colormap_colors[red_pixel].red = colors[j].red;
                        pixel = red_pixel;

                        if (lib_num_of_overlays > 0 && lib_overlay_on[0] == 0){
                            pixel1=pixel | (lib_overlay_planes[0]&
                                lib_visual->red_mask)>>lib_red_mask_shift;
                            lib_colormap_colors[pixel1].red = colors[j].red;
                        }
                        if (lib_num_of_overlays == 2 && 
                            lib_overlay_on[1] == 0) {
                            pixel1=pixel | (lib_overlay_planes[1]&
                                lib_visual->red_mask)>>lib_red_mask_shift;
                            lib_colormap_colors[pixel1].red = colors[j].red;
                        }
                        if (lib_num_of_overlays == 2 && 
                            lib_overlay_on[0] == 0 && lib_overlay_on[1] == 0) {
                            pixel1=pixel | (lib_overlay_planes[0]&
                                lib_visual->red_mask)>>lib_red_mask_shift | 
                                  (lib_overlay_planes[1]&lib_visual->red_mask)
                                  >>lib_red_mask_shift;
                            lib_colormap_colors[pixel1].red = colors[j].red;
                        }
                    }
                    if (lib_cmap_status[green_pixel] == 3) 
                        lib_colormap_colors[green_pixel].green=colors[j].green;
                    else
                    if (lib_cmap_status[green_pixel] == 1) 
                    {
                        lib_colormap_colors[green_pixel].green=colors[j].green;
                        pixel = green_pixel;

                        if (lib_num_of_overlays > 0 && lib_overlay_on[0] == 0){
                            pixel1=pixel | (lib_overlay_planes[0]&
                                lib_visual->green_mask)>>lib_green_mask_shift;
                            lib_colormap_colors[pixel1].green= colors[j].green;
                        }
                        if (lib_num_of_overlays == 2 && 
                            lib_overlay_on[1] == 0) {
                            pixel1=pixel | (lib_overlay_planes[1]&
                                lib_visual->green_mask)>>lib_green_mask_shift;
                            lib_colormap_colors[pixel1].green= colors[j].green;
                        }
                        if (lib_num_of_overlays == 2 && 
                            lib_overlay_on[0] == 0 && lib_overlay_on[1] == 0) {
                            pixel1=pixel | (lib_overlay_planes[0]&
                                lib_visual->green_mask)>>lib_green_mask_shift |
                                 (lib_overlay_planes[1]&lib_visual->green_mask)
                                  >>lib_green_mask_shift;
                            lib_colormap_colors[pixel1].green= colors[j].green;
                        }
                    }
                    if (lib_cmap_status[blue_pixel] == 3) 
                        lib_colormap_colors[blue_pixel].blue = colors[j].blue;
                    else
                    if (lib_cmap_status[blue_pixel] == 1) 
                    {
                        lib_colormap_colors[blue_pixel].blue = colors[j].blue;
                        pixel = blue_pixel;

                        if (lib_num_of_overlays > 0 && lib_overlay_on[0] == 0){
                            pixel1=pixel | (lib_overlay_planes[0]&
                                lib_visual->blue_mask)>>lib_blue_mask_shift;
                            lib_colormap_colors[pixel1].blue = colors[j].blue;
                        }
                        if (lib_num_of_overlays == 2 && 
                            lib_overlay_on[1] == 0) {
                            pixel1=pixel | (lib_overlay_planes[1]&
                                lib_visual->blue_mask)>>lib_blue_mask_shift;
                            lib_colormap_colors[pixel1].blue = colors[j].blue;
                        }
                        if (lib_num_of_overlays == 2 && 
                            lib_overlay_on[0] == 0 && lib_overlay_on[1] == 0) {
                            pixel1=pixel | (lib_overlay_planes[0]&
                                lib_visual->blue_mask)>>lib_blue_mask_shift | 
                                  (lib_overlay_planes[1]&lib_visual->blue_mask)
                                  >>lib_blue_mask_shift;
                            lib_colormap_colors[pixel1].blue = colors[j].blue;
                        }
                    }
                }
                break ;
        }
        XStoreColors(lib_display,lib_cmap,lib_colormap_colors,
            lib_cmap_entries);
        return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetColorcellStatus                           *
 *      Description     : This function will get the status for the     *
 *                        specified colorcell.                          *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  status - Returns the status of the desired   *
 *                                  colorcell.                          *
 *                              0 - read only.                          *
 *                              1 - gray/ pseudo.                       *
 *                              2 - overlay.                            *
 *                              3 - 3dviewnix reserved colorcell.       *
 *      Side effects    : None.                                         *
 *      Entry condition : If cell value is invalid, status is NULL,     *
 *                        or function VCreateColormap is not called     *
 *                        earlier, this function will print a proper    *
 *                        message to the standard error stream, produce *
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VGetReservedColormap, VGetFreeColorcells,     *
 *                        VReadColorcomFile, VPutColormap, VWriteColorcomFile,   *
 *                        VCreateColormap, VGetColormap.                *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified on January 31, 1994 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VGetColorcellStatus ( unsigned long cell, int* status )
{
        char msg[80] ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VGetColorcellStatus.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (cell < 0 || cell >= lib_cmap_entries) {
            sprintf(msg,"The range of cell value is >=0 or < %d.",
                    lib_cmap_entries) ;
            v_print_fatal_error("VGetColorcellStatus",msg,cell) ;
        }
        if (status == NULL) 
            v_print_fatal_error("VGetColorcellStatus",
                "The pointer of status should not be NULL.",0) ;
        *status=lib_cmap_status[cell] ;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_set_visual_class                            *
 *      Description     : This function allows the application          *
 *                        programmer to set a preferred visual class.   *
 *                        The valid value is GrayScale, PseudoColor,    *
 *                        DirectColor, Default and INHERIT. This        *
 *                        function should be called before function     *
 *                        v_create_windows and can be called only once in *
 *                        the same process. If a visual class is        *
 *                        inherited from another process use INHERIT.   *
 *      Return Value    :  0 - work successfully.                       *
 *                         219 - Valid visual class not available on    *
 *                               this screen.                           *
 *                         221 - the depth of bitmap is less than 8.    *
 *                         250 - visual class is already set.           *
 *      Parameters      :  visual_class - the desired visual class.     *
 *                               The value should be GrayScale,         *
 *                               PseudoColor, DirectColor, Default or   *
 *                               Default.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : If visual_class is invalid, or function       *
 *                        VSetup is not called earlier, this       *
 *                        function will print a proper message to the   *
 *                        standard error stream, produce a core dump    *
 *                        and exit from the current process.            *
 *      Related funcs   : VGetVisual, VCreateColormap, v_create_windows.        *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int v_set_visual_class ( int visual_class )
#if 0
int visual_class ;  /* the desired visual class.
                        GrayScale - gray scale visual class,
                        PseudoColor - pseudo color visual class,
                        DirectColor - direct color visual class,
                        Default - default visual class assigned by the 
                                  current X server. 
                        INHERIT - inherit the visual class from another
                                  process. */
#endif
{
        int result ;

        if (lib_x_server_open == 0) {
            printf("The error occurred in the function v_set_visual_class.\n") ;
            printf("Please call VSetup before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        if (visual_class != PSEUDOCOLOR && visual_class != GRAYSCALE && 
            visual_class != DIRECTCOLOR && visual_class != DEFAULT &&
            visual_class != INHERIT) {
            printf("Error occurs in function v_set_visual_class.\n") ;
            printf("The valid values of visual class are GrayScale, ");
            printf(" PseudoColor, DirectColor, Default or INHERIT.\n") ;
            printf("Please note that the value assigned for visual_class is %d\n", visual_class) ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (lib_visual_class_set == 1) return(250) ;
        lib_visual_class_set=1 ;
        result=v_set_visual(visual_class) ;
        return(result) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : v_set_visual                                  *
 *      Description     : This function will try to find a best visual  *
 *                        matching with the specified visual class.     *
 *                        If there is no matched visual class, return   *
 *                        NULL to lib_visual, else return the pointer   *
 *                        to Visual structure to lib_visual.            *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - error reading WINS_ID.COM
 *                         4 - cannot open WINS_ID.COM
 *                         219 - valid visual class not available on    *
 *                               this screen.                           *
 *      Parameters      :  visual_class - the desired visual class      *
 *                               to be set or INHERIT.
 *      Side effects    : None.                                         *
 *      Entry condition : If visual_class is INHERIT, WINS_ID.COM must
 *                        contain the window id's, and lib_display must
 *                        be open.
 *      Related funcs   : v_set_visual_class.                           *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 9/1/94 vinfo_list freed Dewey Odhner.
 *                        Modified 10/3/94 visual kept before freeing
 *                           vinfo_list by Dewey Odhner.
 *                        Modified 10/20/94 lib_pixel_mask set by Dewey Odhner.
 *                        Modified 12/14/94 fopen return value checked
 *                           by Dewey Odhner.
 *                        Modified 12/15/94 to get inherited visual from
 *                           WINS_ID.COM by Dewey Odhner.
 *                        Modified 2/20/98 visual with colormap size 256
 *                           required by Dewey Odhner.
 *                        Modified 2/20/98 lib_depth set before used
 *                           by Dewey Odhner.
 *                        Modified 11/4/99 best_visual checked
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_set_visual ( int visual_class )
{
    int i, visuals_matched, error, vinfo_mask;
    XVisualInfo *vinfo_list, vinfo_template ;
    Visual *best_visual;
    static Visual kept_visual;
    FILE *fp;
    char avail_visual_class[100];
    XWindowAttributes window_attributes;

    if(visual_class == INHERIT) {
        fp=fopen("WINS_ID.COM","rb") ;
        if (fp == NULL)
            return (4);
        error = fscanf(fp,"%d %d %d %d %d %d %d %d %d %d %d",
                 &lib_wins[4].win,&lib_wins[0].win,&lib_wins[1].win,
                 &lib_wins[2].win,&lib_wins[3].win,&lib_wins[5].win,
                 &lib_wins[6].win,&lib_wins[0].pixmap,&lib_wins[1].pixmap,
                 &lib_wins[2].pixmap,&lib_wins[5].pixmap) ;
        if (error != 11)
            return (2);
        fclose(fp);
        error = XGetWindowAttributes(lib_display, lib_wins[0].win,
            &window_attributes);
        if (error == 0)
            return (219);
        switch (window_attributes.visual->CLASS) {
            case GrayScale:
                visual_class = GRAYSCALE;
                break;
            case PseudoColor:
                visual_class = PSEUDOCOLOR;
                break;
            case DirectColor:
                visual_class = DIRECTCOLOR;
                break;
            default:
                return (219);
        }
    }

    vinfo_mask = VisualClassMask|VisualColormapSizeMask;
    switch(visual_class)
    {
      case DEFAULT:
        if (DisplayCells(lib_display, lib_screen) == 256)
            switch (DefaultVisual(lib_display, lib_screen)->CLASS)
            {
                case GrayScale:
                case PseudoColor:
                case DirectColor:
                    lib_visual = DefaultVisual(lib_display, lib_screen);
                    lib_depth = DefaultDepth(lib_display, lib_screen);
                    lib_pixel_mask = lib_visual->CLASS==DirectColor
                                     ?  (1<<lib_visual->bits_per_rgb)-1
                                     :  (1<<lib_depth)-1;
                    return (0) ;
            }
        vinfo_mask = VisualColormapSizeMask;
        break;
      case GRAYSCALE:
        vinfo_template.CLASS = GrayScale;
        break;
      case PSEUDOCOLOR:
        vinfo_template.CLASS = PseudoColor;
        break;
      case DIRECTCOLOR:
        vinfo_template.CLASS = DirectColor;
        break;
    }

    vinfo_template.colormap_size = 256;
    vinfo_list = XGetVisualInfo(lib_display, vinfo_mask, &vinfo_template,
        &visuals_matched);
    if (visuals_matched == 0) {
        lib_visual=NULL ;
        unlink("3DVIEWNIX_RUN");
        return(219) ;
    }
    best_visual=NULL ;
    lib_depth=0 ;
    for (i=0; i<visuals_matched; i++)
        if ((vinfo_list[i].visual->CLASS==GrayScale ||
                vinfo_list[i].visual->CLASS==PseudoColor ||
                vinfo_list[i].visual->CLASS==DirectColor) &&
                (best_visual==NULL ||
                vinfo_list[i].visual->CLASS>best_visual->CLASS)) {
            best_visual = vinfo_list[i].visual ;
            lib_depth = vinfo_list[i].depth ;
        }
    if (best_visual == NULL)
    {
        lib_visual=NULL ;
        unlink("3DVIEWNIX_RUN");
        return(219) ;
    }
	kept_visual = *best_visual;
    XFree(vinfo_list);
    if (lib_depth < 8)
    {
        unlink("3DVIEWNIX_RUN");
        return(221) ;
    }
    lib_visual= &kept_visual;
    lib_pixel_mask = lib_visual->CLASS==DirectColor
                     ?  (1<<lib_visual->bits_per_rgb)-1
                     :  (1<<lib_depth)-1;
    return(0) ;
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetDepth                                     *
 *      Description     : This function returns the current depth of the*
 *                        windows.                                      *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  current_depth - the current depth returned   *
 *                                         by the function.             *
 *      Side effects    : None.                                         *
 *      Entry condition : v_create_windows and v_set_visual_class should be     *
 *                        called before this function is called.        *
 *      Related funcs   : v_create_windows, v_set_visual_class,                 *
 *                        VGetVisual.                                   *
 *      History         : Written on April 14, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
int VGetDepth ( unsigned int* current_depth )
{
 
        *current_depth=lib_depth;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VGetVisual                                    *
 *      Description     : This function will return the pointer of the  *
 *                        current visual class used in the system to    *
 *                        *visual_to_use.                               *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  visual_to_use - Returns a pointer to the     *
 *                                         Visual structure.            *
 *      Side effects    : None.                                         *
 *      Entry condition : If visual_to_use is NULL, or function         *
 *                        v_set_visual_class is not called earlier, this        *
 *                        function will print a proper message to the   *
 *                        standard error stream, produce a core dump,   *
 *                        and exit from the current process.            *
 *      Related funcs   : v_set_visual_class, VCreateColormap.          *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VGetVisual ( Visual** visual_to_use )
{
        if (lib_visual_class_set == 0) {
            printf("The error occurred in the function VGetVisual.\n") ;
            printf("Please call VSetup before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (visual_to_use == NULL) 
            v_print_fatal_error("VGetVisual",
                "The pointer of visual_to_use should not be NULL.",
                0) ;
        *visual_to_use=lib_visual;
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : VReadColorcomFile                                 *
 *      Description     : This function will read the COLOR.COM file    *
 *                        saved by the function VWriteColorcomFile to get the*
 *                        reserved colors, and update and install       *
 *                        the colormap.
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         2 - read error.                              *
 *                         3 - write error.                             *
 *                         4 - file open error.                         *
 *      Parameters      :  None.                                        *
 *      Side effects    : lib_colormap_colors set
 *      Entry condition : If function VCreateColormap is not called     *
 *                        earlier, this funciton will print a proper
 *                        message to the standard error stream, produce *
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VGetReservedColormap, VGetFreeColorcells,     *
 *                        VGetColormap, VPutColormap, VWriteColorcomFile,    *
 *                        VCreateColormap, VGetColorcellStatus.         *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 10/14/94 lib_colormap_colors set
 *                        by Dewey Odhner
 *                        Modified 12/1/94 to handle visual class
 *                        changes by Dewey Odhner.
 *                        Modified 12/13/94 reserved colors only read
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VReadColorcomFile ( void )
{
        int result, j;
        unsigned long pixel;
        
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VReadColorcomFile.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        result=v_read_color_com(lib_reserved_colors);
        if (result != 0) return(result);
        if (lib_colormap_colors == NULL)
            lib_colormap_colors =
                (XColor *)malloc(lib_visual->map_entries*sizeof(XColor));
        if (lib_colormap_colors == NULL)
            return (1);
        for (j=0; j<lib_visual->map_entries; j++) {
            pixel =
                lib_visual->CLASS==DirectColor
                ?   (j << lib_red_mask_shift)|
                    (j << lib_green_mask_shift)|
                    (j << lib_blue_mask_shift)
                :   j;
            lib_colormap_colors[j].pixel = pixel;
        }
        VRestoreDefaultColormap();

        VRefreshWindows(0);  //gjg: ask dewey.   VRefreshWindows();

        return(0) ;
}
        
/************************************************************************
 *                                                                      *
 *      Function        : VWriteColorcomFile                                 *
 *      Description     : This function will write the number of        *
 *                        overlays, the number of colormap entries, the *
 *                        contents of colormap, and the colors and      *
 *                        colorcells reserved by 3DVIEWNIX, to the file *
 *                        COLOR.COM which is an ASCII file.
 *      Return Value    :  0 - work successfully.                       *
 *                         3 - write error.                             *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition :  If function VCreateColormap is not called    *
 *                        earlier, this funciton will print a proper    *
 *                        message to the standard error stream, produce *
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VGetReservedColormap, VGetFreeColorcells,     *
 *                        VGetColormap, VPutColormap, VReadColorcomFile,    *
 *                        VCreateColormap, VGetColorcellStatus.         *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                        Modified 10/18/94 lib_colormap_colors used
 *                        by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int VWriteColorcomFile ( void )
{
        int i, error, pixel;
        FILE *fp ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VWriteColorcomFile.\n") ;
            printf("Please call VCreateColormap before ") ;
            printf("calling this function.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        fp=fopen("COLOR.COM","wb") ;
        if (fp==NULL) return(4);
        if(lib_visual->CLASS==GrayScale)
                error=fprintf(fp,"%s\n","GRAYSCALE");
        else if(lib_visual->CLASS==PseudoColor)
                error=fprintf(fp,"%s\n","PSEUDOCOLOR");
        else if(lib_visual->CLASS==DirectColor)
                error=fprintf(fp,"%s\n","DIRECTCOLOR");
        if (error == 0) return(3) ;
        error=fprintf(fp,"%d\n",lib_num_of_overlays);
        if (error == 0) return(3) ;
        error=fprintf(fp,"%d\n",lib_cmap_entries);
        if (error == 0) return(3);

        /***writing the reserved colors***/
        for(i=0;i<NUM_OF_RESERVED_COLORCELLS;i++)
        {
            pixel=lib_reserved_colors[i].pixel&lib_pixel_mask;
            error=fprintf(fp,"%u %u %u\n",lib_colormap_colors[pixel].red,
                        lib_colormap_colors[pixel].green,
                        lib_colormap_colors[pixel].blue);
            if (error == 0) return(3);
        }


        /***Writing the colormap into the file***/
        for(i=0;i<lib_cmap_entries;i++)
        {
            error=fprintf(fp,"%u %u %u %u\n",lib_colormap_colors[i].pixel,
                        lib_colormap_colors[i].red,
                        lib_colormap_colors[i].green,
                        lib_colormap_colors[i].blue);
            if (error == 0) return(3);
        }

        fclose(fp);
        return(0);
}

/************************************************************************
 *                                                                      *
 *      Function        : v_get_cmap_id                                 *
 *      Description     : This function will return a pointer to the    *
 *                        colormap ID. No value returns to this         *
 *                        function.                                     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  cmap - Returns a pointer to the colormap ID. *
 *      Side effects    : None.                                         *
 *      Entry condition : v_create_windows should be called before this         *
 *                        function is called.                           *
 *      Related funcs   : VGetReservedColormap, VGetFreeColorcells,     *
 *                        VGetColormap, VPutColormap, VReadColorcomFile,    *
 *                        VCreateColormap, VGetColorcellStatus,         *
 *                        VWriteColorcomFile.                                *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 20, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int v_get_cmap_id ( Colormap* cmap )
{
        if (cmap == NULL) 
            v_print_fatal_error("_get_cmap_id",
                "The pointer of cmap should not be NULL.",0) ;
        *cmap=lib_cmap ;
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_read_color_com                              *
 *      Description     : This function will read COLOR.COM file, and   *
 *                        return colors and colorcells reserved by the  *
 *                        3DVIEWNIX process to the array of             *
 *                        reserved_colors.                             *
 *      Return Value    :  0 - work successfully.                       *
 *                         2 - read error.                              *
 *                         4 - file open error.                         *
 *      Parameters      :  reserved_colors - The reserved colors
 *                            indicated in COLOR.COM file will go here.
 *      Side effects    : If the COLOR.COM file does not exist, this    *
 *                        function will return 4, or if the read error  *
 *                        occurred, then this function will return 2.   *
 *                        If there is no error, this function will      *
 *                        return 0.                                     *
 *      Entry condition : None.                                         *
 *      Related funcs   : VCreateColormap,VDisplayJobDoneMessage.       *
 *      History         : Written on June 7, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                        Modified 10/14/94 *colors assumed already
 *                        allocated if non-NULL by Dewey Odhner.
 *                        Modified 12/13/94 reserved colors only read
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int v_read_color_com (
    ViewnixColor reserved_colors[NUM_OF_RESERVED_COLORCELLS] )
{
        int i, error ;
        unsigned int pixel, red, green, blue ;
        FILE *fp;
        char avail_visual_class[100];
        int novl ;
        int entries ;

        if (reserved_colors == NULL)
            v_print_fatal_error("v_read_color_com",
                "The parameter reserved_colors should not be NULL.",
                (int)reserved_colors);
        fp=fopen("COLOR.COM","rb");
        if (fp == NULL) return(4);
        error=fscanf(fp,"%s",avail_visual_class);
        if (error == 0) return(2);
        error=fscanf(fp,"%d",&novl) ;
        if (error == 0) return(2) ;
        fscanf(fp,"%d",&entries) ;
        if (error == 0) return(2) ;
 
        for (i=0; i<NUM_OF_RESERVED_COLORCELLS; i++) {
            error=(fscanf(fp,"%d %d %d",&red,&green,&blue)==3);
            if (error == 0) return(2);
            /*reserved_colors[i].pixel=pixel;*/
            reserved_colors[i].red=red;
            reserved_colors[i].green=green;
            reserved_colors[i].blue=blue;
            /*lib_reserved_colors[i].pixel=pixel;*/
            lib_reserved_colors[i].red=red;
            lib_reserved_colors[i].green=green;
            lib_reserved_colors[i].blue=blue;
        }
 
        fclose(fp);
 
        return(0);
}

/*************************************************************/
