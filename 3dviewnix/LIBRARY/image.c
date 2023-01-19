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
 *      Filename  : image.c                                             *
 *      Ext Funcs : VDisplayGrayImage, VDisplayColorImage,              *
 *                  VTurnOnTruecolor, VTurnOffTruecolor, VGetImage,     *
 *                  VPutImage, VPrepareGrayImage, VPrepareColorImage,   *
 *                  VFillImageRectangle, VScrollImage.                  *
 *      Int Funcs : v_cvt_1_bit_gray,v_cvt_8_bit_gray,v_cvt_16_bit_gray,*
 *                  v_make_8_bitmap_lookup_table,                       * 
 *                  v_make_16_bitmap_lookup_table,                      *
 *                  v_make_32_bitmap_lookup_table,                      *
 *                  v_cvt_8_bit_color,v_cvt_16_bit_color,               *
 *                  v_find_closest_color,v_update_windows,              *
 *                  v_draw_dialog_window,v_write_temp_cmap,             *
 *                  v_read_temp_cmap,v_check_size.                      *
 *                                                                      *
 ************************************************************************/


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

#define MSG_X_MRG 3 
static char *title_msg="MIPG 3DVIEWNIX     1.0" ;
static unsigned char rgb_msk8[8]={0x80, 0xc0, 0xe0, 0xf0, 
                                  0xf8, 0xfc, 0xfe, 0xff} ;
static unsigned short rgb_msk16[16]={0x8000, 0xc000, 0xe000, 0xf000, 
                                     0xf800, 0xfc00, 0xfe00, 0xff00,
                                     0xff80, 0xffc0, 0xffe0, 0xfff0,
                                     0xfff8, 0xfffc, 0xfffe, 0xffff} ;

static int v_check_size ( int x1, int* x2, int* size, int img_size, int win_size );
static int v_find_closest_color ( ViewnixColor* color, XColor* tcolor );
static int v_cvt_1_bit_gray ( unsigned char* data, unsigned char* img,
    unsigned long half_width, unsigned long level, int wd, int ht,
    int bitmap_unit, int opt );
static int v_cvt_16_bit_color ( unsigned short* data, unsigned char* img,
    int bitmap_unit, int depth, int* rgb_bits, int wd, int ht,
    int min, int max, int opt );
static int v_cvt_16_bit_gray ( unsigned char* data, unsigned char* img,
    unsigned long half_width, unsigned long level, int wd, int ht,
    int bitmap_unit, int opt );
static int v_cvt_8_bit_color ( unsigned char* data, unsigned char* img,
    int bitmap_unit, int depth, int* rgb_bits, int wd, int ht,
    int min, int max, int opt );
static int v_cvt_8_bit_gray ( unsigned char* data, unsigned char* img,
    unsigned long half_width, unsigned long level, int wd, int ht,
    int bitmap_unit, int opt );
static int v_make_16_bitmap_lookup_table ( unsigned short* lookup_table,
    int half_width, int level, int bits_per_pixel, long shift, int opt );
static int v_make_32_bitmap_lookup_table ( unsigned long* lookup_table,
    int half_width, int level, int bits_per_pixel, int opt );
static int v_make_8_bitmap_lookup_table ( unsigned char* lookup_table,
    unsigned long half_width, unsigned long level, int bits_per_pixel,
    int opt );
static int v_read_temp_cmap ( XColor* temp_cmap );
static int v_write_temp_cmap ( XColor* temp_cmap );

int VDisplayGrayWindowedImage ( Window win, unsigned char* data,
    int xloc, int yloc, int width, int height, int bits_per_pixel,
    unsigned long half_width, unsigned long level );
int VPrepareGrayImage ( unsigned char* data, int width, int height,
    int bits_per_pixel, unsigned long min, unsigned long max, XImage* ximage );
int VPrepareGrayWindowedImage ( unsigned char* data, int width, int height,
    int bits_per_pixel, unsigned long half_width, unsigned long level,
    XImage* ximage );
/************************************************************************
 *                                                                      *
 *      Function        : VDisplayGrayImage                             *
 *      Description     : This function will display a gray-level image *
 *                        within a rectangle in the image window or in  *
 *                        one of its subwindows. If the dialog window   *
 *                        is at the front of the image window and some  *
 *                        part of image to be displayed overlaps the    *
 *                        dialog window, then this function will draw   *
 *                        the image covered by the dialog window into   *
 *                        the temporary memory for later use. The pixel *
 *                        values are assigned to color cells in the     *
 *                        colorcell array returned by the function      *
 *                        VGetFreeColorcells in the increasing order of *
 *                        gray values.                                  *
 *                        If the bits_per_pixel is 8, the data will be  *
 *                        assigned character pointer type. If the       *
 *                        bits_per_pixel is 16, the data will be        *
 *                        assigned to short pointer type.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation fault.                 *
 *                         6 - invalid window ID.                       *
 *                         240 - image displayed is out of window       *
 *                              boundary.                               *
 *      Parameters      :  win - the image window ID or one of its      *
 *                              subwindows' ID.                         *
 *                         data - Specifies an array of data to be      *
 *                              displayed in the image window.          *
 *                         xloc, yloc - Specifies x and y coordinates of*
 *                              the upper left corner of the rectangle, *
 *                              relative to the origin of the image     *
 *                              window.                                 *
 *                         width, height - Specifies the width and      *
 *                              height in pixels of the image.          *
 *                         bits_per_pixel - Specifies the number of bits*
 *                              per pixel, the value will be 8 or 16.   *
 *                         min, max - the minimum and maximum gray      *
 *                              values of the image data.               *
 *      Side effects    : The related window is image window.           *
 *      Entry condition : If the value xloc, yloc, width, height, or    *
 *                        bits_per_pixel is not valid, or data is NULL, *
 *                        or the image to be displayed in totally out of*
 *                        the specified window boundary, or function    *
 *                        VCreateColormap is not called earlier, then   *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core     *
 *                        dump file, and exit from the current process. *
 *      Related funcs   : VDisplayColorImage, VPrepareColorImage,       *
 *                        VPrepareGrayImage, VPutImage, VGetImage.      *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on July 27, 1992 by Krishna Iyer.    *
 *                        Modified on October 16, 1992 by Krishna Iyer. *
 *                        Modified on March 12, 1993 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
int VDisplayGrayImage ( Window win, unsigned char* data, int xloc, int yloc,
    int width, int height, int bits_per_pixel, unsigned long min,
    unsigned long max )
{
        int i, error;
        char msg1[80];
        int win_width,win_height;
		unsigned long level, half_width ; /* gray level and half width */

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayGrayImage.\n");
            printf("Please call VCreateColormap before ");         
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
        if (data == NULL) 
            v_print_fatal_error("VDisplayGrayImage",
                "The pointer of data should not be NULL.", 0) ;  //gjg
        if (width <= 0) {
            sprintf(msg1,"The range of width is > 0.");
            v_print_fatal_error("VDisplayGrayImage",msg1,width) ;
        }
        if (height <= 0) {
            sprintf(msg1,"The range of height is > 0.");
            v_print_fatal_error("VDisplayGrayImage",msg1,height) ;
        }
        if (bits_per_pixel != 8 && bits_per_pixel != 16 && bits_per_pixel != 1) 
            v_print_fatal_error("VDisplayGrayImage",
                              "The value of bits_per_pixel is 8 or 16.",
                              bits_per_pixel) ;
        if (win == lib_wins[0].win) {
            win_width=lib_wins[0].width ;
            win_height=lib_wins[0].height ;
        }
        else {
            for (i=0; i< lib_num_subwins; i++) {
                if (win == lib_subwins[i].win) {
                    win_width=lib_subwins[i].width ;
                    win_height=lib_subwins[i].height ;
                    break ;
                }
            }
            if (i == lib_num_subwins) return(6) ;
        }
        if ((xloc+width) <= 0 || xloc >= win_width ||
            (yloc+height) <= 0 || yloc >= win_height) {
            printf("The error occurred in the function VDisplayGrayImage.\n");
            printf("Unfortunately there is no image is in the ");
            printf("specified window.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

	half_width = (max-min)/2;
	level = (min+max+1)/2;
	return (VDisplayGrayWindowedImage(win, data, xloc, yloc, width, height,
		bits_per_pixel, half_width, level));
}

/*****************************************************************************
 * FUNCTION: VDisplayGrayWindowedImage
 * DESCRIPTION: Displays a gray-level image within a rectangle in the image
 *    window or in one of its subwindows.  The pixel values are assigned to
 *    color cells in the colorcell array returned by the function
 *    VGetFreeColorcells in the increasing order of gray values.
 * PARAMETERS:
 *    win: the image window ID or one of its subwindows' ID.
 *    data: Specifies an array of data to be displayed in the image window.
 *    xloc, yloc: Specifies x and y coordinates of the upper left corner of
 *       the rectangle, relative to the origin of the image window.
 *    width, height: Specifies the width and height in pixels of the image.
 *    bits_per_pixel: 8 or 16.
 *    half_width, level: the gray window.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: VCreateColormap must be called first.
 * RETURN VALUE:
 *    0: worked successfully.
 *    1: memory allocation fault.
 *    6: invalid window ID.
 *    240: image displayed is out of window boundary.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/4/04 by Dewey Odhner
 *
 *****************************************************************************/
int VDisplayGrayWindowedImage ( Window win, unsigned char* data,
    int xloc, int yloc, int width, int height, int bits_per_pixel,
    unsigned long half_width, unsigned long level )
{
    XImage ximage;
    unsigned char *img;
    int bytes,bitmap_unit;
    int depth,i,y,error;
    char msg1[80];
    int win_width,win_height;
    GC gc;

    if (lib_cmap_created == 0) {
        printf("The error occurred in the function VDisplayGrayWindowedImage.\n");
        printf("Please call VCreateColormap before ");         
        printf("calling this function.\n");
        kill(getpid(),LIB_EXIT);
    }
    if (data == NULL)
        v_print_fatal_error("VDisplayGrayWindowedImage",
            "The pointer of data should not be NULL.", 0) ;  //gjg
    if (width <= 0) {
        sprintf(msg1,"The range of width is > 0.");
        v_print_fatal_error("VDisplayGrayWindowedImage",msg1,width) ;
    }
    if (height <= 0) {
        sprintf(msg1,"The range of height is > 0.");
        v_print_fatal_error("VDisplayGrayWindowedImage",msg1,height);
    }
    if (bits_per_pixel != 8 && bits_per_pixel != 16 && bits_per_pixel != 1) 
        v_print_fatal_error("VDisplayGrayWindowedImage",
                          "The value of bits_per_pixel must be 1, 8 or 16.",
                          bits_per_pixel) ;

    if (win == lib_wins[0].win) {
        win_width=lib_wins[0].width ;
        win_height=lib_wins[0].height ;
        gc=lib_wins[0].gc ;
        y=0 ;
    }
    else {
        for (i=0; i< lib_num_subwins; i++) {
            if (win == lib_subwins[i].win) {
                win_width=lib_subwins[i].width ;
                win_height=lib_subwins[i].height ;
                gc=lib_subwins[i].gc ;
                y=lib_subwins[i].y ;
                break ;
            }
        }
        if (i == lib_num_subwins) return(6) ;
    }
    if ((xloc+width) <= 0 || xloc >= win_width ||
        (yloc+height) <= 0 || yloc >= win_height) {
        printf("The error occurred in the function VDisplayGrayWindowedImage.\n");
        printf("Unfortunately there is none of the image in the ");
        printf("specified window.\n") ;
        kill(getpid(),LIB_EXIT) ;
    }

    depth=lib_depth;
    if (depth == 8) { /* GrayScale/PseudoColor Visual */
        bitmap_unit=8;
        bytes=1;
    }
    if (depth > 8 && depth <= 16) { /* GrayScale/PseudoColor Visual */
        bitmap_unit=16;
        bytes=2;
    }
    if (depth > 16) { /* DirectColor Visual */
        bitmap_unit=32;
        bytes=4;
    }
    ximage.height = height;
    ximage.width = width;
    ximage.xoffset = 0;
    ximage.format = ZPixmap;
    ximage.byte_order = XImageByteOrder(lib_display) ;
    ximage.bitmap_unit = bitmap_unit;
    ximage.bitmap_bit_order = MSBFirst;
    ximage.bitmap_pad = bitmap_unit;
    ximage.bits_per_pixel = bitmap_unit;
    ximage.depth = depth;
    ximage.bytes_per_line = width*bytes ;
    ximage.red_mask=lib_visual->red_mask ;
    ximage.green_mask=lib_visual->green_mask ;
    ximage.blue_mask=lib_visual->blue_mask ;
    img=(unsigned char *)malloc(width*height*bytes) ;
    if (img == NULL) return(1) ;
    switch (bits_per_pixel) {
        case 1 :
            error=v_cvt_1_bit_gray(data,img,half_width,level,width,
                            height,bitmap_unit,0);
            if (error != 0) return(error);
            break;
        case 8 : 
            error=v_cvt_8_bit_gray(data,img,half_width,level,
                                 width,height,bitmap_unit,0) ;
            if (error != 0) return(error) ;
            break ;
        case 16 : 
            error=v_cvt_16_bit_gray(data,img,half_width,level,
                                  width,height,bitmap_unit,0) ;
            if (error != 0) return(error) ;
            break ;
        default : break ;
    }
    ximage.data=(char *)img ;       
    XPutImage(lib_display,win,gc,&ximage,0,0,xloc,yloc,width,height) ;
    free(img) ;

    v_draw_imagewin_border();

    if (xloc < 0 || (xloc+width) > win_width || 
            yloc < 0 || (yloc+height) > win_height) return(240) ;
    else return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_1_bit_gray                              *
 *      Description     : This function lets one convert binary data to *
 *                        be put into either 8, 16 or 32 bit visual     *
 *                        class display data.                           *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  data - the data to be converted.             *
 *                         img - the converted data.                    *
 *                         half_width,level-- the gray window.
 *                         wd - the width.                              *
 *                         ht - the height.                             *
 *                         bitmap_unit - the visual class.              *
 *                         opt - overlay desired or not.                *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayGrayImage.                            *
 *      History         : Written on July 27, 1992 by Krishna Iyer.     *
 *                        Modified 9/30/98 byte order corrected
 *                           by Dewey Odhner.
 *                        Modified: 5/4/04 half_width,level passed
 *                           instead of max,min by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_cvt_1_bit_gray ( unsigned char* data, unsigned char* img,
    unsigned long half_width, unsigned long level, int wd, int ht,
    int bitmap_unit, int opt )
{
        int pixels;  /* total pixels to be display */
        register int i ;  /* index */
        register unsigned char *img8 ;  /* 8 bits output buffer */
        register unsigned char *lookup_table8; /* 8 bits pixel value lookup
                                                  table */
        register unsigned short *img16 ; /* 16 bits output buffer */
        register unsigned short *lookup_table16 ; /* 16 bits pixel value lookup
                                                     table */
        register unsigned long *img32 ; /* 32 bits output buffer */
        register unsigned long *lookup_table32 ; /* 32 bits pixel value looup
                                                   table */
        static unsigned char tbl[8]= {128,64,32,16,8,4,2,1};
        int j,k;
        union {unsigned w; unsigned char b[4];} O, U1, U2; /* byte ordering */

        pixels=wd*ht ;

        switch (bitmap_unit) {
            case 8 : /* grayscale/pseudocolor visual */
                img8=(unsigned char *)img ;
                lookup_table8=(unsigned char *)malloc(2*sizeof(char)) ;
                if (lookup_table8 == NULL) return(1) ;
                v_make_8_bitmap_lookup_table(lookup_table8,half_width,
                                           level,1,opt) ;
                for (j=0,k=0,i=0; i<pixels; i++){
                  img8[i]=lookup_table8[(data[j]&tbl[k])] ;
                  if ( ++k > 7) {
                    j++; k=0;
                  } 
                } 
                free(lookup_table8) ;
                break ;
            case 16 : /* grayscale/pseudocolor visual */
                img16=(unsigned short *)img ;
                lookup_table16=(unsigned short *)malloc(2*sizeof(short)) ;
                if (lookup_table16 == NULL) return(1) ;
                v_make_16_bitmap_lookup_table(lookup_table16,half_width,
                                        level,1,1,opt);
                for (j=0,k=0,i=0; i<pixels; i++){ 
                        img16[i]=lookup_table16[(data[j]&tbl[k])] ;
                        if (++k > 7) {
                                j++; k=0;
                        }
                }
                free(lookup_table16) ;
                break ;
            case 32 : /* directcolor/truecolor visual */
                img32=(unsigned long *)img ;
                lookup_table32=(unsigned long *)malloc(2*sizeof(long)) ;                if (lookup_table32 == NULL) return(1) ;
                v_make_32_bitmap_lookup_table(lookup_table32,half_width,
                                        level,1,opt);
                O.w = XImageByteOrder(lib_display)==LSBFirst?
                      0x03020100: 0x00010203;
                for (i=0; i<2; i++) {
                    U1.w = lookup_table32[i];
                    for (k=0; k<4; k++)
                        U2.b[O.b[k]] = U1.b[k];
                    lookup_table32[i] = U2.w;
                }
                for (j=0,k=0,i=0; i<pixels; i++) {
                        img32[i]=lookup_table32[(data[j]&tbl[k])] ;
                        if(++k > 7) {
                                j++; k=0;
                        }
                }
                free(lookup_table32) ;
                break ;
        }
        return(0) ;
}
 

/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_8_bit_gray                              *
 *      Description     : This function computes 8-bit-per-pixel of     *
 *                        input image data to he output frame buffer    *
 *                        (8-, 16-, 32- bit per pixel).If bitmap_unit is*
 *                        8 or 16, then we just consider GrayScale      *
 *                        and PseudoColor visual classes. The rest      *
 *                        visual lasses are not include. If             *
 *                        bitmap_unit > 16, we just consider DirectColor*
 *                        or TrueColor visual.                          *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  data - Specifies an array of data to         *
 *                              be converted.                           *
 *                         wd, ht - Specifies the width and height in   *
 *                              pixels of the data.                     *
 *                         half_width,level-- the gray window.
 *                         bitmap_unit - Specifies the storage type of  *
 *                              the output buffer.                      *
 *                         depth - Specifies the depth of bitmap.       *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         img - Returns an array of computed data.     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayGrayImage.                            *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on June 27, 1993 by Krishna Iyer.    *
 *                        Modified 9/30/98 byte order corrected
 *                           by Dewey Odhner.
 *                        Modified 4/8/04 bits_per_pixel set to 1 if
 *                           width is 1 by Dewey Odhner.
 *                        Modified: 5/4/04 half_width,level passed
 *                           instead of max,min by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_cvt_8_bit_gray ( unsigned char* data, unsigned char* img,
    unsigned long half_width, unsigned long level, int wd, int ht,
    int bitmap_unit, int opt )
{
        int pixels;  /* total pixels to be display */
        register int i, k;  /* index */
        register unsigned char *data8 ;  /* 8 bits data pointer */
        register unsigned char *img8 ;  /* 8 bits output buffer */
        register unsigned char *lookup_table8; /* 8 bits pixel value lookup 
                                                  table */
        register unsigned short *img16 ; /* 16 bits output buffer */
        register unsigned short *lookup_table16 ; /* 16 bits pixel value lookup
                                                     table */
        register unsigned long *img32 ; /* 32 bits output buffer */
        register unsigned long *lookup_table32 ; /* 32 bits pixel value looup
                                                   table */
        union {unsigned w; unsigned char b[4];} O, U1, U2; /* byte ordering */
		int bits_per_pixel;

        pixels=wd*ht ;
		bits_per_pixel = 8;
        data8=data ;
        switch (bitmap_unit) {
            case 8 : /* grayscale/pseudocolor visual */
                img8=(unsigned char *)img ;
                lookup_table8=(unsigned char *)malloc(256*sizeof(char)) ;
                if (lookup_table8 == NULL) return(1) ;
                v_make_8_bitmap_lookup_table(lookup_table8,half_width,
                                           level,bits_per_pixel,opt) ;
                for (i=0; i<pixels; i++) img8[i]=lookup_table8[data8[i]] ;
                free(lookup_table8) ;
                break ;
            case 16 : /* grayscale/pseudocolor visual */
                img16=(unsigned short *)img ;
                lookup_table16=(unsigned short *)malloc(256*sizeof(short)) ;
                if (lookup_table16 == NULL) return(1) ;
                v_make_16_bitmap_lookup_table(lookup_table16,half_width,
                                        level,8,bits_per_pixel,opt);
                for (i=0; i<pixels; i++) img16[i]=lookup_table16[data8[i]] ;
                free(lookup_table16) ;
                break ;
            case 32 : /* directcolor/truecolor visual */
                img32=(unsigned long *)img ;
                lookup_table32=(unsigned long *)malloc(256*sizeof(long)) ;
                O.w = XImageByteOrder(lib_display)==LSBFirst?
                      0x03020100: 0x00010203;
                for (i=0; i<256; i++) {
                    U1.w = lookup_table32[i];
                    for (k=0; k<4; k++)
                        U2.b[O.b[k]] = U1.b[k];
                    lookup_table32[i] = U2.w;
                }
                if (lookup_table32 == NULL) return(1) ;
                v_make_32_bitmap_lookup_table(lookup_table32,half_width,
                                        level,bits_per_pixel,opt);
                for (i=0; i<pixels; i++) img32[i]=lookup_table32[data8[i]] ;
                free(lookup_table32) ;
                break ;
        }
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_16_bit_gray                             *
 *      Description     : This function computes 16-bit-per-pixel of    *
 *                        input image data to he output frame buffer    *
 *                        (8-, 16-, 32- bit per pixel).If bitmap_unit is*
 *                        8 or 16, then we just consider GrayScale      *
 *                        and PseudoColor visual classes. The rest      *
 *                        visual lasses are not include. If             *
 *                        bitmap_unit > 16, we just consider DirectColor*
 *                        or TrueColor visual.                          *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  data - Specifies an array of data to         *
 *                              be converted.                           *
 *                         wd, ht - Specifies the width and height in   *
 *                              pixels of the data.                     *
 *                         half_width,level-- the gray window
 *                         bitmap_unit - Specifies the storage type of  *
 *                              the output buffer.                      *
 *                         depth - Specifies the depth of bitmap.       *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         img - Returns an array of computed data.     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayGrayImage.                            *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on June 27, 1993 by Krishna Iyer.    *
 *                        Modified 9/30/98 byte order corrected
 *                           by Dewey Odhner.
 *                        Modified: 5/4/04 half_width,level passed
 *                           instead of max,min by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_cvt_16_bit_gray ( unsigned char* data, unsigned char* img,
    unsigned long half_width, unsigned long level, int wd, int ht,
    int bitmap_unit, int opt )
{
        int pixels;  /* total pixels to be display */
        register int i, k;  /* index */
        register unsigned short *data16 ;  /* 16 bits data pointer */
        register unsigned char *img8 ;  /* 8 bits output buffer */
        register unsigned char *lookup_table8; /* 8 bits pixel value lookup 
                                                  table */
        register unsigned short *img16 ; /* 16 bits output buffer */
        register unsigned short *lookup_table16 ; /* 16 bits pixel value lookup
                                                     table */
        register unsigned long *img32 ; /* 32 bits output buffer */
        register unsigned long *lookup_table32 ; /* 32 bits pixel value looup
                                                   table */
        union {unsigned w; unsigned char b[4];} O, U1, U2; /* byte ordering */

        pixels=wd*ht ;
        data16=(unsigned short *)data ;
        switch (bitmap_unit) {
            case 8 : /* grayscale/pseudocolor visual */
                lookup_table8=(unsigned char *)malloc(65536*sizeof(char)) ;
                if (lookup_table8 == NULL) return(1) ;
                img8=(unsigned char *)img ;
                v_make_8_bitmap_lookup_table(lookup_table8,half_width,level,16,
                                           opt) ;
                for (i=0; i<pixels; i++) img8[i]=lookup_table8[data16[i]] ;
                free(lookup_table8) ;
                break ;
            case 16 : /* grayscale/pseudocolor visual */
                img16=(unsigned short *)img ;
                lookup_table16=(unsigned short *)malloc(65536*sizeof(short)) ;
                if (lookup_table16 == NULL) return(1) ;
                v_make_16_bitmap_lookup_table(lookup_table16,half_width,level,
                                            16,0,opt) ;
                for (i=0; i<pixels; i++) img16[i]=lookup_table16[data16[i]] ;
                free(lookup_table16) ;
                break ;
            case 32 : /* directcolor/truecolor visual */
                img32=(unsigned long *)img ;
                lookup_table32=(unsigned long *)malloc(65536*sizeof(long)) ;
                O.w = XImageByteOrder(lib_display)==LSBFirst?
                      0x03020100: 0x00010203;
                for (i=0; i<65536; i++) {
                    U1.w = lookup_table32[i];
                    for (k=0; k<4; k++)
                        U2.b[O.b[k]] = U1.b[k];
                    lookup_table32[i] = U2.w;
                }
                if (lookup_table32 == NULL) return(1) ;
                v_make_32_bitmap_lookup_table(lookup_table32,half_width,
                                            level,16,opt) ;
                for (i=0; i<pixels; i++) img32[i]=lookup_table32[data16[i]] ;
                free(lookup_table32) ;
                break ;
        }
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_make_8_bitmap_lookup_table                  *
 *      Description     : This function computes the lookup table for   *
 *                        for 8-bit bitmap screen given the gray level  *
 *                        and half-width and number of bits per pixel   *
 *                        of input data.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  half_width, level - Specifies the values of  *
 *                              gray level and half width.              *
 *                         bits_per_pixel - Specifies the number of bits* 
 *                              per pixel of input data.                *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         lookup_table - Returns an unsigned char array*
 *                              of lookup table for 8-bit bitmap.       * 
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayGrayImage.                            *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on June 22, 1993 by Krishna Iyer.    *
 *                        Modified on June 27, 1993 by S. Samarasekera. *
 *                        Modified: 5/4/04 half_width, level parameters
 *                           defined as specified by Dewey Odhner.
 *                        Modified 5/11/04 level < half_width allowed
 *                           by Dewey Odhner.
 *                        Modified 5/26/04 level adjusted by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_make_8_bitmap_lookup_table ( unsigned char* lookup_table,
    unsigned long half_width, unsigned long level, int bits_per_pixel, int opt )
{
        long i, j, imn, imx, max;
        float dp ;
        unsigned long ovl ;

        if (opt == 1) {
            if (lib_num_of_overlays == 0) opt = 0, ovl = ~0;
            if (lib_num_of_overlays == 1) ovl= ~lib_overlay_planes[0] ;
            if (lib_num_of_overlays == 2) 
                ovl= ~lib_overlay_planes[0] & ~lib_overlay_planes[1] ;
        }
		else ovl= ~0;

        switch (bits_per_pixel) {
            case 16 :
                max=65535 ;
                break ;
            case 8 :
                max=255 ;
                break ;
            case 1 :
                lookup_table[0] = 0;
				lookup_table[1] = lib_cmap_gray_entries-1<<lib_num_of_overlays;
				return (0);
            default : break ; 
        }
        dp = (float) (lib_cmap_entries-1)/ (half_width? half_width*2: 1);
        imn= (int)level - (int)half_width < 0? 0: (int)level - (int)half_width;
        imx = level+half_width > max ? max : level+half_width;
        for (i=0; i<=imn; i++) lookup_table[i] = 0 ;
        if (opt == 0) {
            for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                lookup_table[i] = (int)((j+1)*dp);
                if (lookup_table[i] >
                    (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                    lookup_table[i]=
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays) ;
            }
        }
        else {
            for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                lookup_table[i] = (int)((j+1)*dp)  & (int)ovl ;
                if (lookup_table[i] >
                    (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                    lookup_table[i]=
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays) ;
            }
        }
        for (i= imx+1; i<=max; i++)
            lookup_table[i] = (lib_cmap_gray_entries-1<<lib_num_of_overlays);

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_make_16_bitmap_lookup_table                 *
 *      Description     : This function computes the lookup table for   *
 *                        for 16-bit bitmap screen given the gray level *
 *                        and half-width and number of bits per pixel   *
 *                        of input data.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  half_width, level - Specifies the values of  *
 *                              gray level and half width.              *
 *                         bits_per_pixel - Specifies the number of bits*
 *                              per pixel of input data.                *
 *                         shift - SPecifies the number of bits         *
 *                              to be shifted.                          *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         lookup_table - Returns an unsigned char array*
 *                              of lookup table for 16-bit bitmap.      *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayGrayImage.                            *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on June 22, 1993 by Krishna Iyer.    *
 *                        Modified 5/11/04 level < half_width allowed
 *                           by Dewey Odhner.
 *                        Modified 5/26/04 level adjusted by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_make_16_bitmap_lookup_table ( unsigned short* lookup_table,
    int half_width, int level, int bits_per_pixel, long shift, int opt )
{
        long i, j, imn, imx, max ;
        float dp ;
        unsigned long ovl ;

        if (opt == 1) {
            if (lib_num_of_overlays == 0) opt = 0 ;
            if (lib_num_of_overlays == 1) ovl= ~lib_overlay_planes[0] ;
            if (lib_num_of_overlays == 2) 
                ovl= ~lib_overlay_planes[0] & ~lib_overlay_planes[1] ;
        }
        dp = (float)(lib_cmap_entries-1)/(2.0 * half_width);
        imn = ((level - half_width) < 0) ? 0 : level - half_width;
        switch (bits_per_pixel) {
            case 1 :
                max=1;
                imx = ((level+half_width) > max) ? max : level+half_width;
            for (i=0; i<=imn; i++)
                    lookup_table[i] = 0;
                if (opt == 0) {
                    for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                        lookup_table[i] = (int)((j+1)*dp);
                        if (lookup_table[i] >
                            (lib_cmap_gray_entries-1<<lib_num_of_overlays))
                            lookup_table[i]=
                                (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                        lookup_table[i]=lookup_table[i] << shift ;
                    }
                }
                else {
                    for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                        lookup_table[i] = (int)((j+1)*dp);
                        if (lookup_table[i] >
                            (lib_cmap_gray_entries-1<<lib_num_of_overlays))
                            lookup_table[i]=
                                (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                        lookup_table[i]=(lookup_table[i] << shift) & ovl ;
                    }
                }
                for (i= imx+1; i<=max; i++)
                    lookup_table[i] =
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays) << shift;
                break ; 
            case 8 :
                max=255 ;
                imx = ((level+half_width) > max) ? max : level+half_width;
                for (i=0; i<=imn; i++) 
                    lookup_table[i] = 0;
                if (opt == 0) {
                    for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                        lookup_table[i] = (int)((j+1)*dp);
                        if (lookup_table[i] > 
                            (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                            lookup_table[i]=
                                (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                        lookup_table[i]=lookup_table[i] << shift ;
                    }
                }
                else {
                    for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                        lookup_table[i] = (int)((j+1)*dp);
                        if (lookup_table[i] > 
                            (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                            lookup_table[i]=
                                (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                        lookup_table[i]=(lookup_table[i] << shift) & ovl ;
                    }
                }
                for (i= imx+1; i<=max; i++) 
                    lookup_table[i] = 
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays) << shift;
                break ;
            case 16 :
                max=65535 ;
                imx = ((level + half_width) > max) ? max : level + half_width;
                for (i=0; i<=imn; i++) 
                    lookup_table[i] = 0 ;
                if (opt == 0) {
                    for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                        lookup_table[i] = (int)((j+1)*dp) ;
                        if (lookup_table[i] > 
                            (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                            lookup_table[i]=
                                (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                    }
                } 
                else {
                    for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                        lookup_table[i] = (int)((j+1)*dp) & ovl ;
                        if (lookup_table[i] > 
                            (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                            lookup_table[i]=
                                (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                    }
                } 
                for (i= imx+1; i<=max; i++) 
                    lookup_table[i] = 
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays) ;
                break ;
            default : break ; 
        }
}


/************************************************************************
 *                                                                      *
 *      Function        : v_make_32_bitmap_lookup_table                 *
 *      Description     : This function computes the lookup table for   *
 *                        for 32-bit bitmap screen given the gray level *
 *                        and half-width and number of bits per pixel   *
 *                        of input data.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  half_width, level - Specifies the values of  *
 *                              gray level and half width.              *
 *                         bits_per_pixel - Specifies the number of bits*
 *                              per pixel of input data.                *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         lookup_table - Returns an unsigned char array*
 *                              of lookup table for 32-bit bitmap.      *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayGrayImage.                            *
 *      History         : Written on June 1, 1990 by Hsiu-Mei Hung.     *
 *                        Modified on June 22, 1993 by Krishna Iyer.    *
 *                        Modified 10/13/98 ovl initialized by Dewey Odhner.
 *                        Modified 4/8/04 ovl masking corrected by Dewey Odhner
 *                        Modified 5/11/04 level < half_width allowed
 *                           by Dewey Odhner.
 *                        Modified 5/26/04 level adjusted by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_make_32_bitmap_lookup_table ( unsigned long* lookup_table,
    int half_width, int level, int bits_per_pixel, int opt )
{
        long i, j, imn, imx, max ;
        float dp ;
        unsigned long ovl ;

        if (opt == 1) {
            if (lib_num_of_overlays == 0) opt = 0, ovl = ~0;
            if (lib_num_of_overlays == 1) ovl= ~lib_overlay_planes[0] ;
            if (lib_num_of_overlays == 2) 
                ovl= ~lib_overlay_planes[0] & ~lib_overlay_planes[1] ;
        }
        else ovl= ~((int)0);

        switch (bits_per_pixel) {
            case 16 :
                max=65535 ;
                break ;
            case 8 :
                max=255 ;
                break ;
            case 1 :
                lookup_table[0] = 0;
				j=lib_cmap_gray_entries-1;
				lookup_table[1] =
                   ((j*(1<<lib_num_of_overlays)) << lib_red_mask_shift |
                    (j*(1<<lib_num_of_overlays)) << lib_green_mask_shift |
                    (j*(1<<lib_num_of_overlays)) << lib_blue_mask_shift)&ovl;
				return (0);
            default : break ; 
        }
        dp = (float) (lib_cmap_entries-1)/ (half_width? half_width*2: 1);
        imn= (int)level - (int)half_width < 0? 0: (int)level - (int)half_width;
        imx = level+half_width > max ? max : level+half_width;
        for (i=0; i<imn; i++)
            lookup_table[i] = 0;
        if (opt == 0) {
            for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                lookup_table[i] = (int)((j+1)*dp);
                if (lookup_table[i] < 0) 
                    lookup_table[i]=0 ;
                if (lookup_table[i] >
                    (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                    lookup_table[i]=
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays) ;
                lookup_table[i]=(lookup_table[i] << lib_red_mask_shift)|
                                (lookup_table[i] << lib_green_mask_shift)|
                                (lookup_table[i] << lib_blue_mask_shift);
            }
        }
        else {
            for (i=imn,j=imn+half_width-level; i<=imx; i++,j++) {
                lookup_table[i] = (int)((j+1)*dp);
                if (lookup_table[i] < 0) 
                    lookup_table[i]=0 ;
                if (lookup_table[i] >
                    (lib_cmap_gray_entries-1<<lib_num_of_overlays)) 
                    lookup_table[i]=
                        (lib_cmap_gray_entries-1<<lib_num_of_overlays);
                lookup_table[i]=((lookup_table[i] << lib_red_mask_shift)|
                                 (lookup_table[i] << lib_green_mask_shift)|
                                 (lookup_table[i] << lib_blue_mask_shift)) & 
                                ovl ;
            }
        }
        for (j=lib_cmap_gray_entries-1,i= imx+1; i<=max; i++) 
            lookup_table[i] = 
                ((j*(1<<lib_num_of_overlays)) << lib_red_mask_shift |
                 (j*(1<<lib_num_of_overlays)) << lib_green_mask_shift |
                 (j*(1<<lib_num_of_overlays)) << lib_blue_mask_shift)&ovl;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_make_8_bitmap_lookup_table                  *
 *      Description     : This function will display a true-color image *
 *                        within a rectangle in the image window or one *
 *                        of its sunwindows. If the dialog window is    *
 *                        at the front of the image window, and some    *
 *                        part of image o be displayed overlaps the     *
 *                        dialog window, this function will draw the    *
 *                        image covered by the dialog window into the   *
 *                        temporary memory for later use. If the        *
 *                        temporary memory is NULL, this function will  *
 *                        return an error code. If the visual class     *
 *                        is PseudoColor or DirectColor, then before you*
 *                        call this function, you should call function  *
 *                        VTurnOnTruecolor first to turn the true-color *
 *                        on and update the colormap. After you finish  *
 *                        displaying the color image, you should call   *
 *                        function VTurnOffTruecolor to turn it off     *
 *                        and put the previous colormap back.           *
 *                        The input data format is RGB, RGB, ...., RGB. *
 *                        If bits_per_rgb is 8, the input data will     *
 *                        be assigned to character pointer type. If     *
 *                        bits_per_rgb is 16, the input data will be    *
 *                        assigned to short pointer type.               *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         6 - invalid window ID.                       *
 *                         222 - try to display a true-color image      *
 *                              in GrayScale visual class.              *
 *                         230 - try to display a true-color image in   *
 *                              the PseudoColor or DirectColor visual   *
 *                              class and truecolor is not on.          *
 *                         240 - image displayed is out of window       *
 *                              boundary.                               *
 *      Parameters      :  win - the image window ID or one of its      *
 *                              subwindows' ID.                         *
 *                         data - Specifies an array of data to be      *
 *                              displayed in the image window.          *
 *                         xloc, yloc - Specifies x and y coordinates of*
 *                              the upper left corner of the rectangle  *
 *                              relative to the origin of the image     *
 *                              window.                                 *
 *                         width, height - Specifies the width and      *
 *                              height in pixels of the image.          *
 *                         bits_per_rgb - Specifies the number of bits  *
 *                              of each RGB component, the value will   *
 *                              be 8 or 16.                             *
 *                         min, max - the minimum and maximum gray      *
 *                              values of the image data.               *
 *      Side effects    : The related window is image window.           *
 *      Entry condition : If the value xloc, yloc, width, height, or    *
 *                        bits_per_rgb is not valid, or data is NULL,   *
 *                        or the image to be displayed in totally out of*
 *                        the specified indow boundary, or function     *
 *                        VCreateColormap is not called earlier, then   *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core     *
 *                        dump file, and exit from the current process. *
 *      Related funcs   : VDisplayGrayImage, VPrepareColorImage,        *
 *                        VPrepareGrayImage, VPutImage, VGetImage,      *
 *                        VTurnOnTruecolor, VTurnOffTruecolor.          *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on June 22, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
int VDisplayColorImage ( Window win, unsigned char* data, int xloc, int yloc,
    int width, int height, int bits_per_rgb, unsigned long min,
    unsigned long max )
{
        XImage ximage;
        unsigned char *img;
        int bitmap_unit,bytes,i,rgb_bits[3];
        int depth,error;
        char msg1[80];
        int y,win_width,win_height;
        GC gc;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VDisplayColorImage.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (data == NULL) 
            v_print_fatal_error("VDisplayColorImage",
                "The pointer of data should not be NULL.", 0) ;  //gjg
        if (width <= 0) {
            sprintf(msg1,"The range of width is > 0.");
            v_print_fatal_error("VDisplayColorImage",msg1,width) ;
        }
        if (height <= 0) {
            sprintf(msg1,"The range of height is > 0.");
            v_print_fatal_error("VDisplayColorImage",msg1,height) ;
        }
        if (bits_per_rgb !=8 && bits_per_rgb != 16 ) 
            v_print_fatal_error("VDisplayColorImage",
                              "The value of bits_per_rgb is 8 or 16.",
                              bits_per_rgb) ;

        if (win == lib_wins[0].win) {
            win_width=lib_wins[0].width ;
            win_height=lib_wins[0].height ;
            gc=lib_wins[0].gc ;
            y=0 ;
        }
        else {
            for (i=0; i<lib_num_subwins; i++) {
                if (win == lib_subwins[i].win) {
                    win_width=lib_subwins[i].width ;
                    win_height=lib_subwins[i].height ;
                    gc=lib_subwins[i].gc ;
                    y=lib_subwins[i].y ;
                    break ;
                }
            }
            if (i == lib_num_subwins) return(6) ;
        }
        if ((xloc+width) <= 0 || xloc >= win_width ||
            (yloc+height) <= 0 || yloc >= win_height) {
            printf("The error occurred in the function VDisplayGrayImage.\n") ;
            printf("Unfortunately there is no  image is in the ");
            printf("specified window.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }

        if (lib_visual->CLASS == GrayScale) return(222) ;
        if (lib_visual->CLASS == PseudoColor && lib_truecolor_on == 0)
            return(230);
        depth=lib_depth ;
        if (depth <= 8) { /* pseudocolor visual */
            bitmap_unit=8 ;
            bytes=1 ;
        }
        if (depth > 8 && depth <= 16) { /* pseudocolor visual */
            bitmap_unit=16 ;
            bytes=2 ;
        }
        if (depth > 16) { /* directcolor/truecolor visual */
            bitmap_unit=32 ;
            bytes=4 ;
        }
        for (i=0; i<3; i++) rgb_bits[i]=0 ;
        if (depth <= 16) {
            rgb_bits[0]=depth/3 ;   /* blue bits */
            rgb_bits[1]=(depth-rgb_bits[0])/2 ; /* green bits */
            rgb_bits[2]=depth-rgb_bits[1]-rgb_bits[0] ; /*red bits */
        }

            ximage.height = height;
            ximage.width = width;
            ximage.xoffset = 0;
            ximage.format = ZPixmap;
            ximage.byte_order = XImageByteOrder(lib_display) ;
            ximage.bitmap_unit = bitmap_unit;
            ximage.bitmap_bit_order = MSBFirst;
            ximage.bitmap_pad = bitmap_unit;
            ximage.bits_per_pixel = bitmap_unit;
            ximage.depth = depth;
            ximage.bytes_per_line = width*bytes;
            ximage.red_mask=lib_visual->red_mask ;
            ximage.green_mask=lib_visual->green_mask ;
            ximage.blue_mask=lib_visual->blue_mask ;
            img=(unsigned char *)malloc(width*height*bytes) ;
            if (img == NULL) return(1) ;
            switch (bits_per_rgb) {
                case 8 : 
                    error=v_cvt_8_bit_color(data,img,bitmap_unit,depth,
                                          rgb_bits,width,height,min,max,0) ;
                    if (error != 0) return(error) ;
                    break ;
                case 16 : 
                    error=v_cvt_16_bit_color( (unsigned short*)data,img,bitmap_unit,depth,
                                           rgb_bits,width,height,min,max,0) ;
                    if (error != 0) return(error) ;
                    break ;
                default : break ;
            }
            ximage.data=(char *)img ;
            XPutImage(lib_display,win,gc,&ximage,0,0,xloc,yloc,
                      width,height) ;
            free(img) ;

        v_draw_imagewin_border();

        if (xloc < 0 || (xloc+width) > win_width ||
            yloc < 0 || (yloc+height) > win_height) return(240) ;
        else return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_8_bit_color                             *
 *      Description     : This function computes 8-bit-per-pixel of     *
 *                        input image data to he output frame buffer    *
 *                        (8-, 16-, 32- bit per pixel).If bitmap_unit is*
 *                        8 or 16, then we just consider GrayScale      *
 *                        and PseudoColor visual classes. The rest      *
 *                        visual lasses are not included, and consider  *
 *                        no overlay planes, even lib_num_of_overlays is*
 *                        not equal to 0. If bitmap_unit == 24, the we  *
 *                        just consider DirectColor visual, and consider*
 *                        overlay planes if lib_num_of_overlays != 0.   *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  data - Specifies an array of data to         *
 *                              be converted.                           *
 *                         bitmap_unit - Specifies the storage type of  *
 *                              the output buffer.                      *
 *                         depth - Specifies the depth of bitmap.       *
 *                         rgb_bits - Specifies the number of bits for  *
 *                              RGB components per pixel of the output  *
 *                              buffer.                                 *
 *                         wd, ht - Specifies the width and height in   *
 *                              pixels of the data.                     *
 *                         min, max - the minimum and maximum gray      *
 *                              values of the image data.               *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         img - Returns an array of computed data.     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayColorImage.                           *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on June 27, 1993 by Krishna Iyer.    *
 *                        Modified 9/30/98 byte order corrected
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_cvt_8_bit_color ( unsigned char* data, unsigned char* img,
    int bitmap_unit, int depth, int* rgb_bits, int wd, int ht,
    int min, int max, int opt )
{
        int i, j, pixels, half_width, level, k;
        int r, g, b, shift, shiftg, shiftb  ;
        unsigned char rtmp8, gtmp8, btmp8, *img8, *lookup_table8 ;
        unsigned short rtmp16, gtmp16, btmp16, *img16, *lookup_table16 ;        
        unsigned long rtmp32, gtmp32, btmp32, *img32 ;
        unsigned long ovl ;
        unsigned long *lookup_table32 ;
        union {unsigned w; unsigned char b[4];} O, U1, U2; /* byte ordering */

        r=rgb_bits[2]-1 ; g=rgb_bits[1]-1 ; b=rgb_bits[0]-1 ;
        shiftg=rgb_bits[2] ; shiftb=rgb_bits[2]+rgb_bits[1] ;
        pixels=wd*ht ;
        half_width=(max-min+1)/2 ;
        level=min+half_width ;
        switch (bitmap_unit) {
            case 8 : /* PseudoColor visual */
                img8=(unsigned char *)img ;
                lookup_table8=(unsigned char *)malloc(256*sizeof(char)) ;
                if (lookup_table8 == NULL) return(1) ;
                v_make_8_bitmap_lookup_table(lookup_table8,half_width,level,
					8,0) ;
                for (i=0, j=0; i<pixels; i++, j += 3) {
                    img8[i]=rtmp8=gtmp8=btmp8=0 ;
                    rtmp8=lookup_table8[data[j]] & rgb_msk8[r];
                    gtmp8=(lookup_table8[data[j+1]] & rgb_msk8[g]) >> shiftg ;
                    btmp8=(lookup_table8[data[j+2]] & rgb_msk8[b]) >> shiftb ;
                    img8[i]=rtmp8+gtmp8+btmp8 ;
                }
                free(lookup_table8) ;
                break ;
            case 16 : /* PseudoColor visual */
                img16=(unsigned short *)img ;
                shift=depth-8 ;
                lookup_table16=(unsigned short *)malloc(256*sizeof(short)) ;
                v_make_16_bitmap_lookup_table(lookup_table16,half_width,
                                            level,8,shift,0) ;
                if (lookup_table16 == NULL) return(1) ;
                for (i=0, j=0; i<pixels; i++, j += 3) {
                    img16[i]=rtmp16=gtmp16=btmp16=0 ;
                    rtmp16=lookup_table16[data[j]] & rgb_msk16[r];
                    gtmp16=(lookup_table16[data[j+1]] & rgb_msk16[g]) >> shiftg;
                    btmp16=(lookup_table16[data[j+2]] & rgb_msk16[b]) >> shiftb;
                    img16[i]=rtmp16+gtmp16+btmp16 ;
                }
                free(lookup_table16) ;
                break ;
            case 32 : /* directcolor/truecolor visual */
                img32=(unsigned long *)img ;
                if (lib_visual->CLASS == DirectColor) {
                    depth=depth/3 ;
                    shift=depth-8 ;
                  lookup_table32=(unsigned long *)malloc(256*sizeof(long)) ;
                  if (lookup_table32 == NULL) return(1) ;
                  v_make_32_bitmap_lookup_table(lookup_table32,half_width,
                                        level,8,opt);
                  O.w = XImageByteOrder(lib_display)==LSBFirst?
                        0x03020100: 0x00010203;
                    if (opt == 1) {
                        if (lib_num_of_overlays == 0) ovl= 0xffffffff ;
                        if (lib_num_of_overlays == 1) 
                            ovl= ~lib_overlay_planes[0] ;
                        if (lib_num_of_overlays == 2) 
                            ovl= ~lib_overlay_planes[0] & 
                                 ~lib_overlay_planes[1] ;
                          for (i=0, j=0; i<pixels; i++, j += 3) {
                          rtmp32=lookup_table32[data[j]] & lib_visual->red_mask; 
                          gtmp32=lookup_table32[data[j+1]] & lib_visual->green_mask; 
                          btmp32=lookup_table32[data[j+2]] & lib_visual->blue_mask;
                          U1.w = (rtmp32+gtmp32+btmp32) & ovl ;
                          for (k=0; k<4; k++)
                              U2.b[O.b[k]] = U1.b[k];
                          img32[i] = U2.w;
                          }
                    }
                    else {
                        for (i=0, j=0; i<pixels; i++, j += 3) {
                          rtmp32=lookup_table32[data[j]] & lib_visual->red_mask; 
                          gtmp32=lookup_table32[data[j+1]] & lib_visual->green_mask; 
                          btmp32=lookup_table32[data[j+2]] & lib_visual->blue_mask;
                            U1.w = (rtmp32+gtmp32+btmp32) ;
                          for (k=0; k<4; k++)
                              U2.b[O.b[k]] = U1.b[k];
                          img32[i] = U2.w;
                        }
                    }
                    free(lookup_table32) ;
                }
                else { /* TrueColor */
                    printf("NOT IMPLEMENT.\n") ;
                }
                break ;
        }
      return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_16_bit_color                            *
 *      Description     : This function computes 16-bit-per-pixel of    *
 *                        input image data to he output frame buffer    *
 *                        (8-, 16-, 32- bit per pixel).If bitmap_unit is*
 *                        8 or 16, then we just consider GrayScale      *
 *                        and PseudoColor visual classes. The rest      *
 *                        visual lasses are not included, and consider  *
 *                        no overlay planes, even lib_num_of_overlays is*
 *                        not equal to 0. If bitmap_unit == 24, the we  *
 *                        just consider DirectColor visual, and consider*
 *                        overlay planes if lib_num_of_overlays != 0.   *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  data - Specifies an array of data to         *
 *                              be converted.                           *
 *                         bitmap_unit - Specifies the storage type of  *
 *                              the output buffer.                      *
 *                         depth - Specifies the depth of bitmap.       *
 *                         rgb_bits - Specifies the number of bits for  *
 *                              RGB components per pixel of the output  *
 *                              buffer.                                 *
 *                         wd, ht - Specifies the width and height in   *
 *                              pixels of the data.                     *
 *                         min, max - the minimum and maximum gray      *
 *                              values of the image data.               *
 *                         opt - indicate whether do and operation for  *
 *                              overlay:        0 - no; 1 - yes.        *
 *                         img - Returns an array of computed data.     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDisplayColorImage.                           *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on June 27, 1993 by Krishna Iyer.    *
 *                        Modified 9/30/98 byte order corrected
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_cvt_16_bit_color ( unsigned short* data, unsigned char* img,
    int bitmap_unit, int depth, int* rgb_bits, int wd, int ht,
    int min, int max, int opt )
{
        int i, j, pixels, half_width, level, k;
        int r, g, b, shiftg, shiftb  ;
        unsigned char rtmp8, gtmp8, btmp8, *img8, *lookup_table8 ;
        unsigned short rtmp16, gtmp16, btmp16, *img16, *lookup_table16 ;        
        unsigned long rtmp32, gtmp32, btmp32, *img32 ;
        unsigned long ovl ;
        union {unsigned w; unsigned char b[4];} O, U1, U2; /* byte ordering */

        r=rgb_bits[2]-1 ; g=rgb_bits[1]-1 ; b=rgb_bits[0]-1 ;
        shiftg=rgb_bits[2] ; shiftb=rgb_bits[2]+rgb_bits[1] ;
        pixels=wd*ht ;
        half_width=(max-min+1)/2 ;
        level=min+half_width ;
        switch (bitmap_unit) {
            case 8 : /* PseudoColor visual */
                img8=(unsigned char *)img ;
                lookup_table8=(unsigned char *)malloc(65536*sizeof(char)) ;
                if (lookup_table8 == NULL) return(1) ;
                v_make_8_bitmap_lookup_table(lookup_table8,half_width,level,
                                           16,0) ;
                for (i=0, j=0; i<pixels; i++, j += 3) {
                    img8[i]=rtmp8=gtmp8=btmp8=0 ;
                    rtmp8=lookup_table8[data[j]] & rgb_msk8[r];
                    gtmp8=(lookup_table8[data[j+1]] & rgb_msk8[g]) >> shiftg ;
                    btmp8=(lookup_table8[data[j+2]] & rgb_msk8[b]) >> shiftb ;
                    img8[i]=rtmp8+gtmp8+btmp8 ;
                }
                free(lookup_table8) ;
                break ;
            case 16 : /* PseudoColor visual */
                img16=(unsigned short *)img ;
                lookup_table16=(unsigned short *)malloc(65536*sizeof(short)) ;
                if (lookup_table16 == NULL) return(1) ;
                v_make_16_bitmap_lookup_table(lookup_table16,half_width,
                                            level,16,0,0) ;
                for (i=0, j=0; i<pixels; i++, j += 3) {
                    img16[i]=rtmp16=gtmp16=btmp16=0 ;
                    rtmp16=lookup_table16[data[j]] & rgb_msk16[r];
                    gtmp16=(lookup_table16[data[j+1]] & rgb_msk16[g]) >> shiftg;
                    btmp16=(lookup_table16[data[j+2]] & rgb_msk16[b]) >> shiftb;
                    img16[i]=rtmp16+gtmp16+btmp16 ;
                }
                free(lookup_table16) ;
                break ;
            case 32 : /* directcolor/truecolor visual */
                img32=(unsigned long *)img ;
                depth=depth/3 ;
                if (depth == 8) {
                    lookup_table8=(unsigned char *)malloc(65536*sizeof(char));
                    v_make_8_bitmap_lookup_table(lookup_table8,half_width,
						level,16,0);
                }
                else {
                    lookup_table16=(unsigned short *)malloc(65536*sizeof(short));
                    v_make_16_bitmap_lookup_table(lookup_table16,half_width,
                                                level,16,0,0) ;
                }
                
                if (lib_visual->CLASS == DirectColor) {
                  O.w = XImageByteOrder(lib_display)==LSBFirst?
                        0x03020100: 0x00010203;
                    if (opt == 1) {
                        if (lib_num_of_overlays == 0) ovl=0xffffffff ;
                        if (lib_num_of_overlays == 1) 
                            ovl= ~lib_overlay_planes[0] ;
                        if (lib_num_of_overlays == 2) 
                            ovl= ~lib_overlay_planes[0] & 
                                 ~lib_overlay_planes[1] ;
                        if (depth == 8) {
                            for (i=0, j=0; i<pixels; i++, j += 3) {
                                rtmp32=lookup_table8[data[j]] << 
                                       lib_red_mask_shift ;
                                gtmp32=lookup_table8[data[j+1]] << 
                                       lib_green_mask_shift;
                                btmp32=lookup_table8[data[j+2]] << 
                                       lib_blue_mask_shift ;
                                U1.w = (rtmp32+gtmp32+btmp32) & ovl ;
                                for (k=0; k<4; k++)
                                    U2.b[O.b[k]] = U1.b[k];
                                img32[i] = U2.w;
                            }
                        }
                        else {
                            for (i=0, j=0; i<pixels; i++, j += 3) {
                                rtmp32=lookup_table16[data[j]] << 
                                       lib_red_mask_shift ;
                                gtmp32=lookup_table16[data[j+1]] << 
                                       lib_green_mask_shift ;
                                btmp32=lookup_table16[data[j+2]] << 
                                       lib_blue_mask_shift ;
                                U1.w = (rtmp32+gtmp32+btmp32) & ovl ;
                                for (k=0; k<4; k++)
                                    U2.b[O.b[k]] = U1.b[k];
                                img32[i] = U2.w;
                            }
                        }
                    }
                    else {
                        if (depth == 8) {
                            for (i=0, j=0; i<pixels; i++, j += 3) {
                                rtmp32=lookup_table8[data[j]] << 
                                       lib_red_mask_shift ;
                                gtmp32=lookup_table8[data[j+1]] << 
                                       lib_green_mask_shift;
                                btmp32=lookup_table8[data[j+2]] << 
                                       lib_blue_mask_shift ;
                                U1.w = rtmp32+gtmp32+btmp32;
                                for (k=0; k<4; k++)
                                    U2.b[O.b[k]] = U1.b[k];
                                img32[i] = U2.w;
                            }
                        }
                        else {
                            for (i=0, j=0; i<pixels; i++, j += 3) {
                                rtmp32=lookup_table16[data[j]] << 
                                       lib_red_mask_shift ;
                                gtmp32=lookup_table16[data[j+1]] << 
                                       lib_green_mask_shift ;
                                btmp32=lookup_table16[data[j+2]] << 
                                       lib_blue_mask_shift ;
                                U1.w = rtmp32+gtmp32+btmp32;
                                for (k=0; k<4; k++)
                                    U2.b[O.b[k]] = U1.b[k];
                                img32[i] = U2.w;
                            }
                        }
                    }
                }
                else { /* TrueColor */
                    printf("NOT IMPLEMENT.\n") ;
                }
                if (depth == 8) free(lookup_table8) ;
                else free(lookup_table16) ;
                break ;
        }
      return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VTurnOnTruecolor                              *
 *      Description     : This function turns on the truecolor mode.    *
 *                        If the visual class is PseudoColor and        *
 *                        truecolor is not on, this function will turn  *
 *                        the truecolor on, backup the colormap content *
 *                        and 3DVIEWNIX reserved colors to the temporary*
 *                        file, and will update the colormap according  *
 *                        to the bit planes divided into red bits in the*
 *                        left most (most significant), green bits in   *
 *                        the middle, and blue bits in the right        *
 *                        most(least significant). If the number of bit *
 *                        planes is 8, then red bits occupy the most    *
 *                        left 3 bits, green bits occupy the next 3 bits*
 *                        and blue bits occupy the most right 2 bits.   *
 *                        This fucntion will define a fixed colormap    *
 *                        according to the above method to divide RGB   *
 *                        into 3-3-2, then try to find the closest      *
 *                        colors and colorcells from the fixed truecolor*
 *                        colormap for 3DVIEWNIX reserved colors. If    *
 *                        the visual class is GrayScale, the function   *
 *                        will return an error code 222. With           *
 *                        PseudoColor visual class, this function will  *
 *                        reset all background and foreground colors    *
 *                        for all windows, and redraw the content of the*
 *                        button window, dialog window, title window    *
 *                        and menu. If there is any message in the      *
 *                        dialog window, the application programmer     *
 *                        needs to call VDisplayDialogMessage to redraw *
 *                        it, otherwise a wrong color for that message  *
 *                        may result.                                   *
 *                        If the visual class is DirectColor and        *
 *                        truecolor is on, this function will turn the  *
 *                        truecolor on, backup the colormap content, and*
 *                        update the colormap to be indentity colormap. *
 *                        This function will not set the overlay if the *
 *                        visual class is PseudoColor, but will for     *
 *                        DirectColor visual class. If the visual class *
 *                        is PseudoColor or DirectColor, before you call*
 *                        function VDisplayColorImage to display a color*
 *                        image in the image window or one of its       *
 *                        you should call this function first to turn   *
 *                        the truecolor on. When the truecolor is on,   *
 *                        the colormap cannot be modified.              *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         220 - truecolor option is already on.        *
 *                         222 - try to turn truecolor on in GrayScale  *
 *                              visual class.                           *
 *      Parameters      :  None.                                        *
 *      Side effects    : lib_colormap_colors, lib_num_of_overlays,
 *                        lib_wins[0].gc, lib_subwins[i].gc may change.
 *                        File "TEMP_CMAP.PAR" will be written.
 *      Entry condition : If function VCreateColormap is not called     *
 *                        earlier this funciton will print a proper     *
 *                        message to the standard error stream, produce *
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VDisplayColorImage, VTurnOffTruecolor.        *
 *      History         : Written on October 17, 1990 by Hsiu-Mei Hung. *
 *                        Modified on May 22, 1993 by Krishna Iyer.     *
 *                        Modified 10/14/94 lib_colormap_colors set
 *                        by Dewey Odhner.
 *                        Modified 12/13/94 VWriteColorcomFile call removed
 *                        by Dewey Odhner.
 *                        Modified 12/20/94 image window and subwindow
 *                        GC's changed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VTurnOnTruecolor ( void )
{
        int rgb_bits[3], rgb_iter[3], depth, entries ;
        unsigned long i, pixel ; 
        unsigned short r, g, b, red, green, blue, rgb ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VTurnOnTruecolor.\n");
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT);
        }
        if (lib_visual->CLASS == GrayScale) return(229) ;
        if (lib_truecolor_on == 1) return(220) ;

        v_write_temp_cmap(lib_colormap_colors) ;
        if (lib_visual->CLASS == PseudoColor) {
            VChangeNumberOfOverlays(0);
            depth=lib_depth ;
            rgb_bits[0]=depth/3 ; /* blue bits */
            rgb_bits[1]=(depth-rgb_bits[0])/2 ; /* green bits */
            rgb_bits[2]=depth-rgb_bits[0]-rgb_bits[1] ; /* red bits */
            for (i=0; i<3; i++) rgb_iter[i]=(int)pow(2.,(double)rgb_bits[i]) ;
            for (r=0, pixel=0; r<rgb_iter[2]; r++) {
                red = r*65535/(rgb_iter[2]-1);
                for (g=0; g<rgb_iter[1]; g++) {
                    green = g*65535/(rgb_iter[1]-1);
                    for (b=0; b<rgb_iter[0]; b++, pixel++) {
                        blue = b*65535/(rgb_iter[0]-1);
                        v_assigned_color(&lib_colormap_colors[pixel],pixel,red,green,blue);
                    }
                }
            }
            XStoreColors(lib_display,lib_cmap,lib_colormap_colors,pixel);
            for (i=0; i<NUM_OF_RESERVED_COLORCELLS; i++) 
              v_find_closest_color(&lib_reserved_colors[i],lib_colormap_colors);
              for(i=0;i<4;i++) v_update_windows(lib_wins[i].win); 
        }
        if (lib_visual->CLASS == DirectColor) {
            switch (lib_num_of_overlays) {
                case 0 : 
                    for (i=0; i<lib_cmap_gray_entries; i++) {
                         rgb=i << 
                             (16-lib_visual->bits_per_rgb) ;
                         pixel=
                        (i << lib_red_mask_shift)|
                        (i << lib_green_mask_shift)| 
                        (i << lib_blue_mask_shift);
                         v_assigned_color(&lib_colormap_colors[i],pixel,rgb,rgb,rgb) ;
                    }
                    entries=lib_cmap_gray_entries ;
                    break ;
                case 1 :
                    for (i=0; i<lib_cmap_gray_entries; i++) {
                        rgb=i*2 << 
                            (16-lib_visual->bits_per_rgb);
                        pixel=
                        (i*2 << lib_red_mask_shift)| 
                        (i*2 << lib_green_mask_shift)|
                        (i*2 << lib_blue_mask_shift);
                        v_assigned_color(&lib_colormap_colors[i*2],pixel,rgb,rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*2+1],
                                       pixel | lib_overlay_planes[0],
                                       rgb,rgb,rgb) ;
                    }
                    entries=lib_cmap_gray_entries*2 ;
                    break ;
                default :
                    for (i=0; i<lib_cmap_gray_entries; i++) {
                        rgb=(i*(1<<lib_num_of_overlays)) <<
                            (16-lib_visual->bits_per_rgb) ;
                        pixel=
                        ((i*(1<<lib_num_of_overlays)) << lib_red_mask_shift)|
                        ((i*(1<<lib_num_of_overlays)) << lib_green_mask_shift)| 
                        ((i*(1<<lib_num_of_overlays)) << lib_blue_mask_shift);
                        v_assigned_color(&lib_colormap_colors[i*4],pixel,rgb,rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*4+1],
                                       pixel | lib_overlay_planes[0],
                                       rgb,rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*4+2],
                                       pixel | lib_overlay_planes[1],
                                       rgb,rgb,rgb) ;
                        v_assigned_color(&lib_colormap_colors[i*4+3],
                                       pixel | lib_overlay_planes[0] | 
                                       lib_overlay_planes[1],rgb,rgb,rgb) ;
                    }
                    entries=lib_cmap_gray_entries*4 ;
                    break ;
            }
            XStoreColors(lib_display,lib_cmap,lib_colormap_colors,entries) ;
        }
        lib_truecolor_on=1 ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_find_closest_color                          *
 *      Description     : This function tries try to find the closest   *
 *                        RGB value in the colormap for a given RGB     *
 *                        value.                                        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  color - Specifies the RGB value to be looked *
 *                              for.                                    *
 *                         tcolor - Specifies the colors in the         *
 *                              colormap.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VTurnOnTrueColor.                             *
 *      History         : Written on October 17, 1990 by Hsiu-Mei Hung. *
 *                        Modified on June 27, 1993 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
static int v_find_closest_color ( ViewnixColor* color, XColor* tcolor )
{
        int red, green, blue, red1, green1, blue1 ;
        int min, i, value;
        unsigned long pixel ;

        red=color->red ;
        green=color->green ;
        blue=color->blue ;
        min=1000000 ;
        for (i=0; i<lib_cmap_entries; i++) {
            red1=tcolor[i].red ;
            green1=tcolor[i].green ;
            blue1=tcolor[i].blue ;
            value=abs(red-red1)+abs(green-green1)+abs(blue-blue1) ;
            if (value < min) {
                min=value ;
                pixel=tcolor[i].pixel ;
            }
        }
        color->pixel=tcolor[pixel].pixel ;
        color->red=tcolor[pixel].red ;
        color->green=tcolor[pixel].green ;
        color->blue=tcolor[pixel].blue ;

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_update_windows                              *
 *      Description     : This function resets all the window           *
 *                        background and foreground colors and redraw   *
 *                        the button, dialog, and title windows and     *
 *                        menu.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VTurnOnTrueColor.                             *
 *      History         : Written on October 17, 1990 by Hsiu-Mei Hung. *
 *                        Modified on May 27, 1993 by Krishna Iyer.     *
 *                                                                      *
 ************************************************************************/
int v_update_windows ( Window win )
{
        /*TreeInfo *treeinfo ;
        int num_of_treeinfo ;
        HorizontalMenuInfo horizontal_menu_info[MAX_HORIZONTAL_MENUS] ;
        VerticalMenuInfo vertical_menu_info ;*/
        int /*butt_offset, ptr, wd,*/ i,j;
        TreeInfo treeinfo;
        short num_of_treeinfo;
        char butt_msg[3][20] ;
        char text[100];
        XTextItem tlabel;
        int copyright_yloc;
        /*XImage *ximage ;
        unsigned long planes ;
        int box_width, box_height, box_xloc, box_yloc ;*/

        /*refresh image window*/
        if(win==lib_wins[1].win) {
                XSetWindowBackground(lib_display,lib_wins[0].win,
                       lib_reserved_colors[9].pixel);
                XSetWindowBackground(lib_display,lib_wins[6].win,
                       lib_reserved_colors[2].pixel);
                XSetForeground(lib_display,lib_wins[0].gc,
                       lib_reserved_colors[0].pixel);
                XSetBackground(lib_display,lib_wins[0].gc,
                       lib_reserved_colors[9].pixel);
                XSetForeground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[0].pixel);
                XSetBackground(lib_display,lib_wins[6].gc,
                       lib_reserved_colors[9].pixel);

                for(j=0;j<lib_num_subwins;j++) {
                        XSetWindowBackground(lib_display,lib_subwins[j].win,
                                lib_reserved_colors[9].pixel) ;
                        XSetForeground(lib_display,lib_subwins[j].gc,
                                lib_reserved_colors[0].pixel) ;
                        XSetBackground(lib_display,lib_subwins[j].gc,
                                lib_reserved_colors[9].pixel) ;
                }

        }

        /*refresh title window*/        
        if(win==lib_wins[3].win) {
                XSetWindowBackground(lib_display,lib_wins[3].win,
                        lib_reserved_colors[1].pixel);
                XSetForeground(lib_display,lib_wins[3].gc,
                        lib_reserved_colors[0].pixel);
                XSetBackground(lib_display,lib_wins[3].gc,
                        lib_reserved_colors[1].pixel);

                XClearArea(lib_display,lib_wins[3].win,1,1,lib_wins[3].width,
                   lib_wins[3].height,False) ;
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

                VDisplayTitleString("");

                XFlush(lib_display);
        }

        /*refresh button window*/
        if(win==lib_wins[2].win) {
                for (i=0; i<3; i++) strcpy(butt_msg[i],lib_butt_msg[i]);

                XSetWindowBackground(lib_display,lib_wins[2].win,
                        lib_reserved_colors[1].pixel);
                XSetForeground(lib_display,lib_wins[2].gc,
                        lib_reserved_colors[0].pixel);
                XSetBackground(lib_display,lib_wins[2].gc,
                        lib_reserved_colors[1].pixel);

                v_draw_button_window(0) ; /* update button window */

                XFlush(lib_display);
        }

        /*refresh dialog window*/
        if(win==lib_wins[1].win) {
                XSetWindowBackground(lib_display,lib_wins[1].win,
                       lib_reserved_colors[1].pixel);
                XSetForeground(lib_display,lib_wins[1].gc,
                       lib_reserved_colors[0].pixel);
                XSetBackground(lib_display,lib_wins[1].gc,
                       lib_reserved_colors[1].pixel);

                v_draw_dialog_window() ; /* update dialog window */

                /*update the scales*/
                for (i=0; i<MAX_SCALES; i++) {
                  if(lib_display_scale_on[i]==TRUE) {
                        v_create_threedscale(i);
                        v_display_scale(lib_scale[i].win,lib_scale[i].gc,
                             lib_wins[1].win,lib_wins[1].gc,i);
                        XFlush(lib_display);
                  }
                }
                XFlush(lib_display);
                
                VDisplayDialogMessage(LIB_DIAL_MSG);
        }

        /*update the borders of the four main windows*/
        v_draw_window_border();

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_draw_dialog_window                          *
 *      Description     : This function redraws the dialog window       *
 *                        after the trucolor mode is turned off.        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VTurnOnTrueColor.                             *
 *      History         : Written on October 17, 1990 by Hsiu-Mei Hung. *
 *                        Modified on April 16, 1993 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
int v_draw_dialog_window ( void )
{
        int i ;

        XClearArea(lib_display,lib_wins[1].win,1,1,lib_wins[1].width,
                   lib_wins[1].height,False) ;
        if (lib_display_panel_on) 
            v_display_panel(TRUE) ;
        for (i=0; i<MAX_SCALES; i++) {
            if (lib_display_scale_on[i]==TRUE) 
            /*XClearArea(lib_display,lib_scale[i].win,0,0,lib_scale[i].width,
                   lib_scale[i].height,False);*/
                v_display_scale(lib_scale[i].win,lib_scale[i].gc,
                              lib_wins[1].win,lib_wins[1].gc,i) ;
        }

        v_draw_dialogwin_border();

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_write_temp_cmap                             *
 *      Description     : This function writes the old colormap         *
 *                        parameters to the file "TEMP_CMAP.PAR".       *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  temp_cmap - Specifes an array of X color     *
 *                              defined structure.                      *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VTurnOnTrueColor.                             *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                                                                      *
 ************************************************************************/
static int v_write_temp_cmap ( XColor* temp_cmap )
{
        FILE *fp ;

        fp=fopen("TEMP_CMAP.PAR","wb") ;
        fwrite(&lib_cmap_entries,sizeof(short),1,fp) ;
        fwrite(&lib_cmap_gray_entries,sizeof(short),1,fp) ;
        fwrite(&lib_num_of_overlays,sizeof(short),1,fp) ;
        fwrite(lib_cmap_status,sizeof(char),lib_visual->map_entries,fp) ;
        fwrite(temp_cmap,sizeof(XColor),lib_cmap_entries,fp) ;
        fwrite(lib_reserved_colors,sizeof(ViewnixColor),
               NUM_OF_RESERVED_COLORCELLS,fp);
        fclose(fp) ;
}
        

/************************************************************************
 *                                                                      *
 *      Function        : VTurnOffTruecolor                             *
 *      Description     : This function turns off hte truecolor mode.   *
 *                        If the visual class is PseudoColor or         *
 *                        DirectColor and truecolor is on, this function*
 *                        will turn the truecolor off, put the previous *
 *                        colormap content and 3DVIEWNIX reserved colors*
 *                        back from the temporary file, reset all       *
 *                        background and foreground colors for all      *
 *                        windows, and redraw the content of the button *
 *                        window, dialog window, title window and menu. *
 *                        If there is any message in the dialog window, *
 *                        the application programmer needs to call      *
 *                        VDisplayDialogMessage to redraw it, otherwise *
 *                        a wrong color for that message may result.    *
 *                        If the visual class is DirectColor and        *
 *                        truecolor is on, this function will turn      *
 *                        the truecolor off and put the previous        *
 *                        colormap contents back from the temporary     *
 *                        file.                                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         236 - truecolor is not on.                   *
 *      Parameters      :  None.                                        *
 *      Side effects    : lib_colormap_colors set
 *      Entry condition : If function VCreateColormap is not called     *
 *                        earlier,this funciton will print a proper     *
 *                        message to the standard error stream, produce *
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VDisplayColorImage, VTurnOnTruecolor.         *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on March 10, 1993 by Krishna Iyer.   *
 *                        Modified 10/14/94 lib_colormap_colors set
 *                        by Dewey Odhner.
 *                        Modified 12/13/94 VWriteColorcomFile call removed
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int VTurnOffTruecolor ( void )
{
        int i;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VTurnOffTruecolor.\n");
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (!lib_truecolor_on) return(236) ;
        v_read_temp_cmap(lib_colormap_colors) ;
        XStoreColors(lib_display,lib_cmap,lib_colormap_colors,lib_cmap_entries);
        if (lib_visual->CLASS == PseudoColor) {
                for(i=0;i<4;i++) v_update_windows(lib_wins[i].win);
        }
        lib_truecolor_on=0;
        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_read_temp_cmap                              *
 *      Description     : This function reads the old colormap          *
 *                        parameters from the file "TEMP_CMAP.PAR"
 *                        and removes it.       *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  temp_cmap - pointer to an array of *
 *                              X color defined structure to be filled.
 *      Side effects    : lib_colormap_colors, lib_num_of_overlays,
 *                        lib_cmap_entries, lib_cmap_gray_entries,
 *                        lib_cmap_status, lib_reserved_colors,
 *                        lib_wins[0].gc, lib_subwins[i].gc may change.
 *      Entry condition : None.                                         *
 *      Related funcs   : VTurnOnTrueColor.                             *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified 12/20/94 image window and subwindow
 *                        GC's changed by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
static int v_read_temp_cmap ( XColor* temp_cmap )
{
        FILE *fp ;
        short num_of_overlays;

        fp=fopen("TEMP_CMAP.PAR","rb") ;
        fread(&lib_cmap_entries,sizeof(short),1,fp) ;
        fread(&lib_cmap_gray_entries,sizeof(short),1,fp) ;
        fread(&num_of_overlays,sizeof(short),1,fp) ;
        fread(lib_cmap_status,sizeof(char),lib_visual->map_entries,fp) ;
        fread(temp_cmap,sizeof(XColor),lib_cmap_entries,fp) ;
        fread(lib_reserved_colors,sizeof(ViewnixColor),
              NUM_OF_RESERVED_COLORCELLS,fp);
        fclose(fp) ;
        unlink("TEMP_CMAP.PAR") ;
        VChangeNumberOfOverlays(num_of_overlays);
}


/************************************************************************
 *                                                                      *
 *      Function        : VGetImage                                     *
 *      Description     : This function will read the image in the      *
 *                        specified window and within the rectangle to  *
 *                        *ximage. When you are done with the image,    *
 *                        you can call X function Xfree to free memory  *
 *                        space at *ximage.                             *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *                         238 - cannot get enough memory for the image.*
 *      Parameters      :  win - Specifies the ID of the window.        *
 *                         xloc,yloc - Specifies x and y coordinates    *
 *                              of the upper left corner of the         *
 *                              rectangle, relative to the origin of the*
 *                              window.                                 *
 *                         width, height - Specifies the width and      *
 *                              height in pixels of the image.          *
 *                         ximage - Returns a pointer to an XImage      *
 *                              pointer structure. This structure       *
 *                              provides you with the contents of the   *
 *                              specified rectangle of the window       *
 *                              in the format you specify.              *
 *      Side effects    : The related window are image window,          *
 *                        dialog window, and button window.             *
 *      Entry condition : If xloc, yloc, width, or height is not valid, *
 *                        or function VCreateColormap is not called     *
 *                        earlier, this funciton will print a proper    *
 *                        message to the  standard error stream, produce*
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VPutImage, VPrepareGrayImage,                 *
 *                        VPrepareColorImage, VDisplayGrayImage,        *
 *                        VDisplayGrayImage.                            *
 *      History         : Written on August 9, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on July 28, 1992 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
int VGetImage ( Window win, XImage** ximage, int xloc, int yloc,
    int width, int height )
{
        GC gc;
        int win_width,win_height;
        char msg1[80];
        int i,j,y;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VGetImage.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        for (i=0; i<3; i++) {
            if (win == lib_wins[i].win) {
                if (xloc < 0 || xloc >= lib_wins[i].width) {
                    sprintf(msg1,"The range of xloc is >= 0 && < %d.",
                            lib_wins[i].width);
                    v_print_fatal_error("VGetImage",msg1,xloc) ;
                }
                if (yloc < 0 || yloc >= lib_wins[i].height) {
                    sprintf(msg1,"The range of yloc is >= 0 && < %d.",
                            lib_wins[i].height);
                    v_print_fatal_error("VGetImage",msg1,yloc) ;
                }
                if (width <= 0 || width > lib_wins[i].width) {
                   sprintf(msg1,"The range of width is > 0 && <= %d.",
                            lib_wins[i].width);
                    v_print_fatal_error("VGetImage",msg1,width) ;
                }
                if (height <= 0 || height > lib_wins[i].height) {
                    sprintf(msg1,"The range of height is > 0 && <= %d.",
                            lib_wins[i].height);
                    v_print_fatal_error("VGetImage",msg1,height) ;
                }
                break ;
            }
        }
        if (i == 3) {
            for (j=0; j<lib_num_subwins; j++) {
                if (lib_subwins[j].win == win) {
                    if (xloc < 0 || xloc >= lib_subwins[j].width) {
                        sprintf(msg1,"The range of xloc is >= 0 && < %d.",
                                lib_subwins[j].width);
                        v_print_fatal_error("VGetImage",msg1,xloc) ;
                    }
                    if (yloc < 0 || yloc >= lib_subwins[j].height) {
                        sprintf(msg1,"The range of yloc is >= 0 && < %d.",
                                lib_subwins[j].height);
                        v_print_fatal_error("VGetImage",msg1,yloc) ;
                    }
                    if (width <= 0 || width > lib_subwins[j].width) {
                        sprintf(msg1,"The range of width is > 0 && <= %d.",
                                lib_subwins[j].width);
                        v_print_fatal_error("VGetImage",msg1,width) ;
                    }
                    if (height <= 0 || height > lib_subwins[j].height) {
                        sprintf(msg1,"The range of height is > 0 && <= %d.",
                                lib_subwins[j].height);
                        v_print_fatal_error("VGetImage",msg1,height) ;
                    }
                    break ;
                }
            }
            if (j == lib_num_subwins) return(6) ;
        }

/*****Bug fixed on July 8, 1992 by SS and KI******/

                for (i=0; i<3; i++) {
                        if(win == lib_wins[i].win) {
                                win_width=lib_wins[i].width;
                                win_height=lib_wins[i].height;
                                gc=lib_wins[i].gc;
                                y=0;
                                break;
                        }
                }
                if (i==3) {
                   for (j=0; j<lib_num_subwins; j++){
                        if(lib_subwins[j].win == win){
                                win_width=lib_subwins[j].width;
                                win_height=lib_subwins[j].height;
                                gc=lib_subwins[j].gc;
                                y=lib_subwins[j].y;
                                break;
                        }
                   }
                }
                if (j == lib_num_subwins) return(6);
        

        *ximage=XGetImage(lib_display,win,xloc,yloc,width,height,0xffff,
                          ZPixmap) ;
        if (*ximage == NULL) return(238) ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : v_cvt_1_bit_gray                              *
 *      Description     : This function will put a part of an image     *
 *                        in a  rectangle in a specified window. If you *
 *                        have a prepared image generated from          *
 *                        library function VPrepareGrayImage or         *
 *                        VPrepareColorImage, you can use this function *
 *                        to display the image. This function just puts *
 *                        the image in the specified window, and does   *
 *                        not do any scaling or overlay computation. If *
 *                        win is the image window, and the dialog window*
 *                        is at the front of lower part of the image    *
 *                        window, then this function will draw the image*
 *                        in the lower part of the image window to the  *
 *                        temporary memory for later use.  If the       *
 *                        temporary memory is NULL, this function will  *
 *                        return an error code.                         *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *      Parameters      :  win - Specifies the ID of the window.        *
 *                         img_xloc, img_yloc - Specifies x and y       *
 *                              coordinates of the upper left corner of *
 *                              the rectangle, relative to the origin   *
 *                              of the image.                           *
 *                         win_xloc, win_yloc - Specifies x and y       *
 *                              coordinates of the upper left corner of *
 *                              the rectangle, relative to the origin   *
 *                              of the window.                          *
 *                         width, height - Specifies the width and      *
 *                              height in pixels of the image.          *
 *                         ximage - Specifies the image you want        *
 *                              combined with the rectangle.            *
 *      Side effects    : The related window are image window, dialog   *
 *                        window, and button window.                    *
 *      Entry condition : If win_xloc, win_yloc, img_xloc, img_yloc,    *
 *                        width, or height is not valid, or function    *
 *                        VCreateColormap is not called earlier, or the *
 *                        image to be displayed is totally out of the   *
 *                        specified window boundary, this function      *
 *                        will print a proper message to the  standard  *
 *                        error stream, produce a core dump, and exit   *
 *                        from the current process.                     *
 *      Related funcs   : VGetImage, VPrepareGrayImage,                 *
 *                        VPrepareColorImage, VDisplayGrayImage,        *
 *                        VDisplayGrayImage.                            *
 *      History         : Written on August 9, 1989 by Hsiu-Mei Hung.   *
 *                        Modified on July 27, 1992 by Krishna Iyer.    *
 *                                                                      *
 ************************************************************************/
int VPutImage ( Window win, XImage* ximage, int img_xloc, int img_yloc,
    int win_xloc, int win_yloc, int width, int height )
{
        GC gc;
        int bnd,i,j,y,win_width,win_height;
        char msg1[80];

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VGetPutImage.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (ximage == NULL) 
            v_print_fatal_error("VPutImage",
                "The pointer of ximage should not be NULL.", 0) ;  //gjg
        if (img_xloc < 0 || img_xloc >= ximage->width) {
            sprintf(msg1,"The range of xloc is >= 0 && < %d.",
                    ximage->width);
            v_print_fatal_error("VPutImage",msg1,img_xloc) ;
        }
        if (img_yloc < 0 || img_yloc >= ximage->height) {
            sprintf(msg1,"The range of yloc is >= 0 && < %d.",
                    ximage->height);
            v_print_fatal_error("VPutImage",msg1,img_yloc) ;
        }
        if (width <= 0) 
            v_print_fatal_error("VPutImage","width should be > 0.",width) ;
        if (height <= 0) 
            v_print_fatal_error("VPutImage","height should be > 0.",height) ;

        for (i=0; i<3; i++) {
            if (win == lib_wins[i].win) {
                win_width=lib_wins[i].width ;
                win_height=lib_wins[i].height ;
                gc=lib_wins[i].gc ;
                y=0 ;
                break ;
            }
        }
        if (i == 3) {
            for (j=0; j<lib_num_subwins; j++) {
                if (lib_subwins[j].win == win) {
                    win_width=lib_subwins[j].width ;
                    win_height=lib_subwins[j].height ;
                    gc=lib_subwins[j].gc ;
                    y=lib_subwins[j].y ;
                    break ;
                }
            }
            if (j == lib_num_subwins) return(6) ;
        }
        if ((win_xloc+width) <= 0 || win_xloc >= win_width ||
            (win_yloc+height) <= 0 || win_yloc >= win_height) 
        {
            printf("The error occurred in the function VPutImage.\n");
            printf("Unfortunately there is no image is in the ");
            printf("specified window.\n");
            kill(getpid(),LIB_EXIT);
        }

        if (win == lib_wins[2].win) {
            XPutImage(lib_display,win,gc,ximage,img_xloc,img_yloc,
                      win_xloc,win_yloc,width,height) ;
            return(0) ;
        }
        if (win == lib_wins[1].win) {
            XPutImage(lib_display,win,gc,ximage,img_xloc,img_yloc,
                      win_xloc,win_yloc,width,height) ;
            return(0) ;
        }

        bnd=lib_wins[0].height-lib_wins[1].height+1 ;
        XPutImage(lib_display,win,gc,ximage,img_xloc,img_yloc,
                      win_xloc,win_yloc,width,height) ;

        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VPrepareGrayImage                             *
 *      Description     : This function will compute a prepared         *
 *                        gray-level image. If bits_per_pixel is 8, the *
 *                        input data will be assigned to character      *
 *                        pointer type. If bits_per_pixel is 16, the    * 
 *                        input data will be assigned to short pointer  *
 *                        type. The output data byte order will depend  *
 *                        on the X server and this function will get    *
 *                        this information from X function              *
 *                        XImageByteOrder. When you want to display this*
 *                        prepared image stored in ximage, you can      *
 *                        call library function PutImage. When you are  *
 *                        with this image, you can call C function free *
 *                        ximage->data.                                 *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  data - the data to be converted.             *
 *                         width, height - Specifies the width and      *
 *                              height in pixels of the image.          *
 *                         bits_per_pixel - Specifies the number of bits*
 *                              per pixel, the value will be 8 or 16.   *
 *                         min,max - the minimum and maximum gray values*
 *                              of the image data.                      *
 *                         image - Returns precomputed image into the   *
 *                              struct XImage.                          *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value width, height, or bit_per_pixel  *
 *                        is not valid, or function VCreateColormap is  *
 *                        not called earlier, this funciton will print a*
 *                        proper message to the standard error stream,  *
 *                        produce a core dump, and exit from the current*
 *                        process.                                      *
 *      Related funcs   : VPrepareColorImage, VDisplayGrayImage,        *
 *                        VDisplayColorImage, VPutImage, VGetImage.     *
 *      History         : Written on November 19, 1990 by Hsiu-Mei Hung.*
 *                                                                      *
 ************************************************************************/
int VPrepareGrayImage ( unsigned char* data, int width, int height,
    int bits_per_pixel, unsigned long min, unsigned long max, XImage* ximage )
{
		unsigned long level, half_width ; /* gray level and half width */

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VPrepareGrayImage.\n");
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (data == 0) 
            v_print_fatal_error("VPrepareGrayImage",
                "The pointer of data should not be NULL.", 0) ;  //gjg
        if (width <= 0) 
            v_print_fatal_error("VPrepareGrayImage",
                              "The value of width is > 0.",width) ;
        if (height <= 0) 
            v_print_fatal_error("VPrepareGrayImage",
                              "The value of height is > 0.",height) ;
        if (bits_per_pixel != 8 && bits_per_pixel != 16) 
            v_print_fatal_error("VPrepareGrayImage",
                              "The value of bits_per_pixel is 8 or 16.",
                              bits_per_pixel) ;
        if (ximage == 0) 
            v_print_fatal_error("VPrepareGrayImage",
                "The pointer of ximage should not be NULL.", 0) ;  //gjg

		half_width = (max-min)/2;
		level = (min+max+1)/2;
		return (VPrepareGrayWindowedImage(data,width,height,bits_per_pixel,
			half_width,level,ximage));
}

/*****************************************************************************
 * FUNCTION: VPrepareGrayWindowedImage
 * DESCRIPTION: Displays a gray-level image within a rectangle in the image
 *    window or in one of its subwindows.  The pixel values are assigned to
 *    color cells in the colorcell array returned by the function
 *    VGetFreeColorcells in the increasing order of gray values.
 * PARAMETERS:
 *    win: the image window ID or one of its subwindows' ID.
 *    data: Specifies an array of data to be displayed in the image window.
 *    xloc, yloc: Specifies x and y coordinates of the upper left corner of
 *       the rectangle, relative to the origin of the image window.
 *    width, height: Specifies the width and height in pixels of the image.
 *    bits_per_pixel: 8 or 16.
 *    half_width, level: the gray window.
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: VCreateColormap must be called first.
 * RETURN VALUE:
 *    0: worked successfully.
 *    1: memory allocation fault.
 *    6: invalid window ID.
 *    240: image displayed is out of window boundary.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/5/04 by Dewey Odhner
 *
 *****************************************************************************/
int VPrepareGrayWindowedImage ( unsigned char* data, int width, int height,
    int bits_per_pixel, unsigned long half_width, unsigned long level,
    XImage* ximage )
{
    unsigned char *img ;
    int bytes, bitmap_unit, depth ;

    if (lib_cmap_created == 0) {
        printf("The error occurred in the function VPrepareGrayWindowedImage.\n");
        printf("Please call VCreateColormap before ");
        printf("calling this function.\n");
        kill(getpid(),LIB_EXIT) ;
    }
    if (data == 0) 
        v_print_fatal_error("VPrepareGrayImage",
            "The pointer of data should not be NULL.", 0) ;  //gjg
    if (width <= 0) 
        v_print_fatal_error("VPrepareGrayImage",
                          "The value of width is > 0.",width) ;
    if (height <= 0) 
        v_print_fatal_error("VPrepareGrayImage",
                          "The value of height is > 0.",height) ;
    if (bits_per_pixel != 8 && bits_per_pixel != 16) 
        v_print_fatal_error("VPrepareGrayImage",
                          "The value of bits_per_pixel is 8 or 16.",
                          bits_per_pixel) ;
    if (ximage == 0) 
        v_print_fatal_error("VPrepareGrayImage",
            "The pointer of ximage should not be NULL.", 0) ;  //gjg

    depth=lib_depth ;
    if (depth == 8) { /* GrayScale/PseudoColor Visual */
        bitmap_unit=8 ;
        bytes=1 ;
    }
    if (depth > 8 && depth <= 16) { /* GrayScale/PseudoColor Visual */
        bitmap_unit=16 ;
        bytes=2 ;
    }
    if (depth > 16) { /* DirectColor/TrueColor Visual */
        bitmap_unit=32 ;
        bytes=4 ;
    }
    ximage->height = height;
    ximage->width = width;
    ximage->xoffset = 0;
    ximage->format = ZPixmap;
    ximage->byte_order = XImageByteOrder(lib_display) ;
    ximage->bitmap_unit = bitmap_unit;
    ximage->bitmap_bit_order = MSBFirst;
    ximage->bitmap_pad = bitmap_unit;
    ximage->bits_per_pixel = bitmap_unit;
    ximage->depth = depth;
    ximage->bytes_per_line = width*bytes;
    ximage->red_mask=lib_visual->red_mask ;
    ximage->green_mask=lib_visual->green_mask ;
    ximage->blue_mask=lib_visual->blue_mask ;
    img=(unsigned char *)malloc(width*height*bytes) ;
    if (img == NULL) return(1) ;
    switch (bits_per_pixel) {
        case 8 : 
            v_cvt_8_bit_gray(data,img,half_width,level,width,height,
				bitmap_unit,1);
            break ;
        case 16 : 
            v_cvt_16_bit_gray(data,img,half_width,level,width,height,
				bitmap_unit,1);
            break ;
        default : break ;
    }
    ximage->data=(char *)img ;
    return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VPrepareColorImage                            *
 *      Description     : This function will compute a prepared image   *
 *                        to be displayed with truecolor on. Input      *
 *                        data format is RGB, RGB, ...., RGB.           *
 *                        If bits_per_rgb is 8, the input data will be  *
 *                        assigned to character pointer type. If        *
 *                        bits_per_rgb is 16, the input data will be    *
 *                        assigned to short pointer type. The output    *
 *                        data byte order will depend on the X server   *
 *                        and this function will get this information   *
 *                        from X function XImageByteOrder.              *
 *                        When you want to display this prepared        *
 *                        image stored in ximage, you can call library  *
 *                        function VPutImage. When you are done with    *
 *                        this image, you can call C function free to   *
 *                        free ximage->data.                            *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation error.                 *
 *      Parameters      :  data - Specifies an array of data to be      *
 *                              computed.                               *
 *                         width, height - Specifies the width and      *
 *                              height in pixels of the image.          *
 *                         bits_per_rgb - Specifies the number of bits  *
 *                              of each RGB component, the value will   *
 *                              be 8 or 16.                             *
 *                         min, max - the minimum and maximum gray      *
 *                              values of the image data.               *
 *                         ximage - Returns precomputed image into      *
 *                              the struct XImage.                      *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value width, height, or bit_per_rgb is *
 *                        not valid, or function VCreateColormap is     *
 *                        not called arlier, this funciton will print   *
 *                        a proper message to the standard error stream,*
 *                        produce a core dump, and exit from the current*
 *                        process.                                      *
 *      Related funcs   : VPrepareGrayImage, VDisplayGrayImage,         *
 *                        VDisplayColorImage, VPutImage, VGetImage,     *
 *                        VTurnOnTruecolor, VTurnOffTruecolor.          *
 *      History         : Written on November 19, 1990 by Hsiu-Mei Hung.*
 *                                                                      *
 ************************************************************************/
int VPrepareColorImage ( unsigned char* data, int width, int height,
    int bits_per_rgb, unsigned long min, unsigned long max, XImage* ximage )
{
        unsigned char *img ;
        int bitmap_unit, bytes, rgb_bits[3], depth, i ;

        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VPrepareColorImage.\n");   
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (data == 0) 
            v_print_fatal_error("VPrepareColorImage",
                "The pointer of data should not be NULL.", (int)data) ;
        if (width <= 0) 
            v_print_fatal_error("VPrepareColorImage",
                              "The value of width is > 0.",width) ;
        if (height <= 0) 
            v_print_fatal_error("VPrepareColorImage",
                              "The value of height is > 0.",height) ;
        if (bits_per_rgb !=8 && bits_per_rgb != 16 ) 
            v_print_fatal_error("VPrepareColorImage",
                              "The value of bits_per_rgb is 8 or 16.",
                              bits_per_rgb) ;
        if (ximage == 0) 
            v_print_fatal_error("VPrepareColorImage",
                "The pointer of ximage should not be NULL.", 0) ;  //gjg

        depth=lib_depth ;
        if (depth == 8) { /* pseudocolor visual */
            bitmap_unit=8 ;
            bytes=1 ;
        }
        if (depth > 8 && depth <= 16) { /* pseudocolor visual */
            bitmap_unit=16 ;
            bytes=2 ;
        }
        if (depth > 16) { /*directcolor/truecolor visual */
            bitmap_unit=32 ;
            bytes=4 ;
        }
        if (depth <= 16) {
            for (i=0; i<3; i++) rgb_bits[i]=0 ;
            rgb_bits[0]=depth/3 ;   /* blue bits */
            rgb_bits[1]=(depth-rgb_bits[0])/2 ; /* green bits */
            rgb_bits[2]=depth-rgb_bits[1]-rgb_bits[0] ; /*red bits */
        }
        ximage->height = height;
        ximage->width = width;
        ximage->xoffset = 0;
        ximage->format = ZPixmap;
        ximage->byte_order = XImageByteOrder(lib_display) ;
        ximage->bitmap_unit = bitmap_unit;
        ximage->bitmap_bit_order = MSBFirst;
        ximage->bitmap_pad = bitmap_unit;
        ximage->bits_per_pixel = bitmap_unit;
        ximage->depth = depth;
        ximage->bytes_per_line = width*bytes ;
        ximage->red_mask=lib_visual->red_mask ;
        ximage->green_mask=lib_visual->green_mask ;
        ximage->blue_mask=lib_visual->blue_mask ;
        img=(unsigned char *)malloc(width*height*bytes) ;
        if (img == NULL) return(1) ;
        switch (bits_per_rgb) {
            case 8 : 
                v_cvt_8_bit_color(data,img,bitmap_unit,depth,
                                rgb_bits,width,height,min,max,1);
                break ;
            case 16 : 
                v_cvt_16_bit_color( (unsigned short*)data,img,bitmap_unit,depth,
                                 rgb_bits,width,height,min,max,1);
                break ;
            default : break ;
        }
        ximage->data=(char *)img ;
        return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VFillImageRectangle                           *
 *      Description     : This function will fill the specified         *
 *                        rectangle in the image window or one of its   *
 *                        subwindows with the spcified pixel value.     *
 *                        If the area to be filled is in the left       *
 *                        lower part of the image window covered by     *
 *                        the dialog window, then this function will    *
 *                        fill an appropriate temporary memory, else    *
 *                        fill the window itself. If the width and      *
 *                        height are equal to 0, this function will fill*
 *                        the whole specified window from the location  *
 *                        you specifies. If the image is out of the     *
 *                        window boundary, this function will clip the  *
 *                        image, display in the window, and return an   *
 *                        error code.                                   *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *                         240 - image displayed is out of window       *
 *                              boundary.                               *
 *      Parameters      :  win - the image window ID or one of its      *
 *                              subwindows' ID.                         *
 *                         xloc, yloc - Specifies x and y coordinates of*
 *                              the upper left corner of the rectangle, *
 *                              relative to the origin of the specified *
 *                              window.                                 *
 *                         value - Specifies the pixel value to fill    *
 *                              the image window.                       *
 *      Side effects    : None.                                         *
 *      Entry condition : If the value xloc, yloc, width, height, or    *
 *                        value is not valid, or function               *
 *                        VCreateColormap is not called earlier, or the *
 *                        rectangle to be filled is totally out         *
 *                        of the specified window boundary, this        *
 *                        function, will print a proper message to the  *
 *                        standard error stream, produce a core dump,and*
 *                        exit from the current process.                *
 *      Related funcs   : VFillOverlayRectangle, VClearWindow.          *
 *      History         : Written on November 1, 1990 by Hsiu-Mei Hung. *
 *                                                                      *
 ************************************************************************/
int VFillImageRectangle ( Window win, int xloc, int yloc, int width, int height,
    unsigned long value )
{
        int max_range;
        char msg1[80];
        int win_width,win_height,abs_y,i;
        GC gc;
        
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function ") ;
            printf("VFillImageRectangle.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (win == lib_wins[0].win) {
            win_width=lib_wins[0].width ;
            win_height=lib_wins[0].height ;
            gc=lib_wins[0].gc ;
            abs_y=0 ;
        } 
        else {
            for (i=0; i<lib_num_subwins; i++) {
                if (lib_subwins[i].win == win) {
                    win_width=lib_subwins[i].width ;
                    win_height=lib_subwins[i].height ;
                    gc=lib_subwins[i].gc ;
                    abs_y=lib_subwins[i].y ;
                    break ;
                }
            }
            if (i == lib_num_subwins) return(6) ;
        }
        
        max_range=(int)pow(2.,(double)lib_depth) ;
        if (value >= max_range) {
            sprintf(msg1,"The range of value is >= 0 && < %d.",max_range) ;
            v_print_fatal_error("VFillImageRectangle",msg1,value) ;
        }
        if (width == 0 && height == 0) {
            width=win_width-xloc ;
            height=win_height-yloc ;
        }
        else {
            if (width < 0) {
                sprintf(msg1,"The value of width is >= 0.");
                v_print_fatal_error("VFillImageRectangle",msg1,width) ;
            }
            if (height < 0) {
                sprintf(msg1,"The value of height is > 0.");
                v_print_fatal_error("VFillImageRectangle",msg1,height) ;
            }
        }
        if ((xloc+width) <= 0 || xloc >= win_width ||
            (yloc+height) <= 0 || yloc >= win_height) {
            printf("The error occurred in the function VDisplayGrayImage.\n") ;
            printf("Unfortunately there is no image is in the specified ");
            printf("window.\n");
            kill(getpid(),LIB_EXIT);
        }

            XSetForeground(lib_display,gc,value) ;
            XFillRectangle(lib_display,win,gc,xloc,yloc,width,height) ;
            XSetForeground(lib_display,gc,lib_reserved_colors[0].pixel) ;

        if (xloc < 0 || (xloc+width) > win_width || 
            yloc < 0 || (yloc+height) > win_height) return(240) ;
        else return(0) ;
}


/************************************************************************
 *                                                                      *
 *      Function        : VScrollImage                                  *
 *      Description     : This function will scroll up or down the image*
 *                        according to delta_x and delta_y.             *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - invalid window ID.                       *
 *      Parameters      :  win - the image window ID or one of its      *
 *                              subwindows' ID.                         *
 *                         ximage - Specifies the image you want        *
 *                              combined with the rectangle.            *
 *                         x1, y1 - Specifies x and y coordinates of the*
 *                              upper left corner of the image, relative*
 *                              to the origin of the specified window.  *
 *                         delta_x, delta_y - the offset between the old*
 *                              cursor position and the new cursor      *
 *                              position.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : If function VCreateColormap is not called     *
 *                        earlier, or the image to be scrolled is       *
 *                        totally out of the specified window           *
 *                        boundary, this funciton will print a proper   *
 *                        message to the standard error stream, produce *
 *                        a core dump, and exit from the current        *
 *                        process.                                      *
 *      Related funcs   : VPutImage, VClearWindow.                      *
 *      History         : Written on September 5, 1991 by Hsiu-Mei Hung.*
 *                        Modified on March 19, 1993 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
int VScrollImage ( Window win, XImage* image, int x1, int y1,
    int delta_x, int delta_y )
{
        int nx[4], ny[4], nwidth[4], nheight[4] ;
        int nx1,ny1,nx2,ny2,nx3,ny3,nwidth3,nheight3;
        int x2,y2,height,width,absy,i;
        int win_width,win_height,win_height1,xlen,ylen;
        GC gc;
 
        if (lib_cmap_created == 0) {
            printf("The error occurred in the function VScrollImage.\n") ;
            printf("Please call VCreateColormap before ");             
            printf("calling this function.\n");
            kill(getpid(),LIB_EXIT) ;
        }
        if (win == lib_wins[0].win) {
            win_width=lib_wins[0].width ;
            win_height1=lib_wins[0].height ;
            gc=lib_wins[0].gc ;
            absy=0 ;
            if (lib_dial_win_front)
                win_height=lib_wins[0].height-lib_wins[1].height ;
            else win_height=lib_wins[0].height ;
        }
        else {
            for (i=0; i<lib_num_subwins; i++) {
                if (lib_subwins[i].win == win) {
                    win_width=lib_subwins[i].width ;
                    win_height1=lib_subwins[i].height ;
                    gc=lib_subwins[i].gc;
                    absy=lib_subwins[i].y;
                    if (lib_dial_win_front &&
                        absy+lib_subwins[i].height > lib_wins[1].y)
                        win_height=lib_wins[0].height-lib_wins[1].height-absy;
                    else win_height=lib_subwins[i].height;
                    break ;
                }
            }
            if (i == lib_num_subwins) return(6) ;
        }
        if (x1+image->width < 0 || x1 >= win_width ||
            (y1+image->height) < 0 || y1 >= win_height1 ) {
            printf("The error occurred in the function VScrollImage.\n") ;
            printf("Unfortunately there is no image is in the ");
            printf("specified window.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        if (x1+image->width+delta_x < 0 || x1+delta_x >= win_width ||
            (y1+image->height+delta_y) < 0 || y1+delta_y >= win_height1 ) {
            printf("The error occurred in the function VScrollImage.\n") ;
            printf("Unfortunately no image will be displayed in ");
            printf("the specified window.\n") ;
            kill(getpid(),LIB_EXIT) ;
        }
        v_check_size(x1,&x2,&width,image->width,win_width) ;
        v_check_size(y1,&y2,&height,image->height,win_height) ;
        nx1=x1+delta_x ;
        ny1=y1+delta_y ;
        nx2=x2+delta_x ;
        ny2=y2+delta_y ;
        v_check_size(nx1,&nx3,&nwidth3,image->width,win_width) ;
        v_check_size(ny1,&ny3,&nheight3,image->height,win_height) ;
        for (i=0; i<4; i++) {
            nx[i]=ny[i]= -1 ;
            nwidth[i]=nheight[i]=0 ;
        }
        if (ny2 != ny3) {
            nx[0]=nx3 ;
            ny[0]=ny3 ;
            nheight[0]=ny2-ny3+1 ;
            nwidth[0]=nwidth3 ;
        }
        if ((ny2+height) < (ny3+nheight3)) {
            nx[3]=nx3 ;
            ny[3]=ny2+height-2 ;
            nheight[3]=ny3+nheight3-ny2-height+2 ;
            nwidth[3]=nwidth3 ;
        }
        if (nx2 != nx3) {
            nx[1]=nx3 ;
            ny[1]=ny2 ;
            nheight[1]=nheight3 ;
            nwidth[1]=nx2-nx3+1 ;
        }
        if ((nx2+width) < (nx3+nwidth3)) {
            nx[2]=nx2+width-2 ;
            ny[2]=ny2 ;
            nheight[2]=height ;
            nwidth[2]=nx3+nwidth3-nx[2]+2 ;
        }
        XCopyArea(lib_display,win,win,gc,x2,y2,width,height,nx2,ny2) ;
        for (i=0; i<4; i++) {
            if (nwidth[i] != 0 && nheight[i] != 0)
                XPutImage(lib_display,win,gc,image,nx[i]-nx1,ny[i]-ny1,
                          nx[i],ny[i],nwidth[i],nheight[i]) ;
        }
        xlen=nx1-x1 ;
        ylen=ny1-y1 ;
        if (xlen > image->width || xlen < -image->width || ylen > image->height|| ylen < -image->height)
                XClearArea(lib_display,win,x1,y1,image->width,image->height,
                           False);
        else {
            if (xlen > 0)
                XClearArea(lib_display,win,x1,y1,xlen,image->height,False) ;
            if (xlen < 0)
                XClearArea(lib_display,win,nx1+image->width,y1,-xlen,
                           image->height,False) ;
            if (ylen > 0)
                XClearArea(lib_display,win,x1,y1,image->width,ylen,False) ;
            if (ylen < 0)
                XClearArea(lib_display,win,x1,ny1+image->height,image->width,
                           -ylen,False) ;
        }

        v_draw_imagewin_border();

        if (!lib_dial_win_front ||
            (absy+y1+image->height+lib_wins[0].y <= lib_wins[1].y &&
             absy+ny1+image->height+lib_wins[0].y <= lib_wins[1].y))

        {
            return(0) ;
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : v_check_size                                  *
 *      Description     : This function finds out if the image is       *
 *                        inside the window and returns the size of     *
 *                        image inside the window.                      *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x1,x2 - upper left coordinate of the image.  *
 *                         size - the size of the image that is returned*
 *                         img_size - the size of the original image.   *
 *                         win_size - the size of the window.           *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VScrollImage.                                 *
 *      History         : Written on August 10, 1989 by Hsiu-Mei Hung.  *
 *                        Modified on February 20, 1993 by Krishna Iyer.*
 *                                                                      *
 ************************************************************************/
static int v_check_size ( int x1, int* x2, int* size, int img_size, int win_size )
{
        if (x1 < 0) {
            *x2=0 ;
            *size=img_size+x1 ;
            if (*size > win_size) *size=win_size ;
        }
        else {
            *x2=x1 ;
            if (x1+img_size > win_size) *size=win_size-x1 ;
            else *size=img_size ;
        }

        return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VCopyImageWindowArea                          *
 *      Description     : This function copies an area of the image     *
 *                        window or one of its subwindows to a new      *
 *                        region in the same window. This can be used   *
 *                        to scroll parts of a window.                  *
 *      Return Value    :  0 - work successfully.                       *
 *                         6 - Invalid window ID.
 *      Parameters      :  win - window ID.                             *
 *                         x_src  - x coord of top left point of the    *
 *                              source region.                          *
 *                         y_src  - y coord of top left point of the    *
 *                              source region.                          *
 *                         width  - width of region.                    *
 *                         height - height of region.                   *
 *                         x_dst  - x coord of top left point of the    *
 *                              destination region.                     *
 *                         y_dst  - y coord of top left point of the    *
 *                              destination region.                     *
 *      Side effects    : Copying Region would destroy what is under the*
 *                        destination window.                           *
 *      Entry condition : win should be the image window or one of its  *
 *                        subwindows. copying retion should be within   *
 *                        the speciefied window. Destination region     *
 *                        should be at least partially within the       *
 *                        window.                                       *
 *      Related funcs   : VSetup.                                       *
 *      History         : Written on May 10, 1992 by S. Samarasekera.   *
 *                                                                      *
 ************************************************************************/
int VCopyImageWindowArea ( Window win, int x_src, int y_src,
    int width, int height, int x_dst, int y_dst )
{
 
  WindowGcInfo *cur;
  int i,abs_y;
 
  if (lib_wins_created == 0) {
    printf("The error occurred in the function ") ;
    printf("VClearWindow.\n") ;
    printf("Please call VSetup before ");             
            printf("calling this function.\n");
    kill(getpid(),LIB_EXIT) ;
  }
 
  if (width < 0)
    v_print_fatal_error("VClearWindow","width should be >= 0.",width) ;
  if (height < 0)
    v_print_fatal_error("VClearWindow","height should be >= 0.",height);
 
  cur=NULL;
  if (win==lib_wins[0].win) {  /* image window ? */
    abs_y=0;
    cur= &lib_wins[0];
  }
  else      /* image subwindow ? */
    for(i=0;i<lib_num_subwins;i++)
      if (win == lib_subwins[i].win) {
        abs_y= lib_subwins[i].y;
        cur= &lib_subwins[i];
      }
 
  if (cur==NULL) return(6);   /* not image window or a subwindow */
 
  XCopyArea(lib_display,win,win,cur->gc,x_src,y_src,
            width,height,x_dst,y_dst);
  return(0);
 
}
 
/*************************************************************************/
 
