/*
  Copyright 1993-2013, 2017 Medical Image Processing Group
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
 *      Filename  : overlay.c                                           *
 *      Ext Funcs : VDisplayOverlayPoints, VClearOverlayPoints,         *
 *                  VDisplayOverlayLine, VClearOverlayLine,             *
 *                  VDisplayOverlayRectangle, VClearOverlayRectangle,   *
 *                  VTurnOnOverlay, VTurnOffOverlay, VDisplayOverlay,   *
 *                  VClearOverlay, VChangeNumberOfOverlays,             *
 *                  VFillOverlayRectangle, VPackByteToBit, VDrawCurve.  *
 *      Int Funcs : v_do_overlay_points,v_do_overlay_line,              *
 *                  v_draw_points_in_img_and_pixmap,                    *
 *                  v_do_overlay_rectangle,v_offovl,v_convert_1_bit,    *
 *                  v_draw_line,v_backtrace,v_close_curve,v_cross,      *
 *                  v_chain.                                            *
 *                                                                      *
 ************************************************************************/

#include <sys/types.h>
#if ! defined (WIN32) && ! defined (_WIN32)
    #include <unistd.h>
#endif
#include "Vlibrary.h"
#include "3dv.h"
#if defined(__cplusplus) || defined(c_plusplus)
    #define  CLASS  c_class
#else
    #define  CLASS  class
#endif


typedef struct { int x, y; } X_Point;

static int V_segment_intersection ( PointInfo* line1, PointInfo* line2,
    PointInfo** where1,  PointInfo** where2 );
int VComputeLine ( int x1, int y1, int x2, int y2, X_Point** points,
    int* npoints );


/************************************************************************
 *                                                                      *
 *      Function        : VPackByteToBit                                *
 *      Description     : This function packs byte data into bit string.*
 *                        If the value of byte is not 0, then the       *
 *                        corresponding bit will be set to 1, else it   *
 *                        will be set to 0. Packing order is most       *
 *                        significant bit first.                        *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  idata - Specifies an array of data to be     * 
 *                                packed.                               *
 *                         nbytes - Specifies the number of bytes       *
 *                                to be packed.                         *
 *                         odata - Returns an array of 1-bit packed     *
 *                                data.                                 *
 *      Side effects    : None.                                         *
 *      Entry condition : If idata or odata is NULL, or nbytes is       *
 *                        less than or equal to zero, this function     *
 *                        will print a proper message to the standard   *
 *                        error stream, produce a core dump, and exit   *
 *                        from the current process.                     *
 *      Related funcs   : VDisplayOverlay.                              *
 *      History         : Written on November 19, 1990 by Hsiu-Mei Hung.*
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/
int VPackByteToBit ( unsigned char* idata, int nbytes, unsigned char* odata )
{
        int bit, word, i ;
        static unsigned char mask[8]=
                        {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

        if (odata == NULL) 
            v_print_fatal_error("VPackByteToBit",
                "The pointer of odata should not be NULL.", 0) ;
        if (idata == NULL) 
            v_print_fatal_error("VPackByteToBit",
                "The pointer of idata should not be NULL.", 0) ;
        if (nbytes <= 0) 
            v_print_fatal_error("VPackByteToBit",
                "The value of nbytes should be greater than 0.",nbytes) ;
        for (i=0, bit=0, word=0; i<nbytes; i++) {
            if (bit == 0) odata[word]=0 ;
            if (idata[i] != 0) odata[word] |= mask[bit] ;
/*printf("odata[%d] = %d\n",word,odata[word]);*/
            bit++ ;
            if (bit == 8) {
                bit=0 ;
                word++ ;
            }
        }

        return(0);
}


/************************************************************************
 *      Function        : v_draw_line                                   *
 *      Description     : This function will draw the line from         *
 *                        (curx,cury) to the end points along the points*
 *                        specified by *points. Th enumber of points    *
 *                        will be indicated by npoints. The line has    *
 *                        the color specified by the foreground pixel.  *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation failure
 *                         276 - bad curve data, there are points that  *
 *                               are repeated.                          *
 *      Parameters      :  win - the subwindows ID.                     *
 *                         gc - the gc of the window (win).             *
 *                         curx - the x location of the cursor.         *
 *                         cury - the y location of the cursor.         *
 *                         foreground - the pixel value of the color    *
 *                                cell that indicates foreground color. *
 *                         points - the points along which the curve    *
 *                                has to be drawn.                      *
 *                         npoints - the number of points to be joined. *
 *             max_points - the size of the array at points
 *      Side effects    : None.                                         *
 *      Entry condition : VCreateColormap should be called earlier,else *
 *                        this function will print a proper message to  *
 *                        the standard error stream, produce a core dump*
 *                        file, and exit from the current process.      *
 *      Related funcs   : VDrawCurve.                                   *
 *      History         : Written on November 27, 1991 by Hsiu-Mei Hung.*
 *                        Modified on March 7, 1993 by Krishna Iyer.    *
 *            Modified 6/21/94 to use dynamic memory allocation by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int v_draw_line ( int curx, int cury, PointInfo** points, int* npoints,
    int* max_points )
{
        X_Point *tpoints ;
        int tnpoints, result, i ;
        PointInfo *more_points;

        if (*npoints == 0) {
            (*points)[*npoints].x=curx ;
            (*points)[*npoints].y=cury ;
            (*points)[*npoints].vertex=1 ;
            (*npoints)++ ;
            return(0) ;
        }
        if ((*points)[*npoints-1].x == curx &&
            (*points)[*npoints-1].y == cury) return(0) ;
        result=VComputeLine((*points)[*npoints-1].x,(*points)[*npoints-1].y,
                            curx,cury,&tpoints,&tnpoints) ;
        if (result != 0)
            return(result) ;
        if ((*npoints+tnpoints-1) >= *max_points) {
            while (*max_points < *npoints+tnpoints-1)
                *max_points += 256;
            more_points =
                (PointInfo *)realloc(*points, *max_points*sizeof(PointInfo));
            if (more_points == NULL) {
                free(tpoints) ;
                return(1) ;
            }
            *points = more_points;
        }
        for (i=1; i<tnpoints; i++, (*npoints)++) {
            (*points)[*npoints].x=tpoints[i].x ;
            (*points)[*npoints].y=tpoints[i].y ;
            (*points)[*npoints].vertex=0 ;
        }
        free(tpoints);
        (*points)[*npoints-1].vertex=1 ;
        return(0) ;
}

/************************************************************************
 *      Function        : v_close_curve                                 *
 *      Description     : This function will close the curve between    *
 *                        the lines jpined by points tpoints to the line*
 *                        joined by point points. It returns the        *
 *                        vertices and number of vertices for the closed*
 *                        curve.                                        *
 *      Return Value    :  0 - work successfully.                       *
 *                         1 - memory allocation failure
 *                         277 - no point is selected.                  *
 *      Parameters      :  tpoints - the points of the first line.      *
 *                         tnpoints - the number of points of first     *
 *                                line.                                 *
 *                         max_points - the maximum number of points in
 *                            the tpoints array before reallocation
 *                         points - the points of the second line.      *
 *                         npoints - the number of points of second     *
 *                                line.                                 *
 *                         vertices - the vertex locations within the   *
 *                                closed curve is returned.             *
 *                         nvertices - the number of vertices within    *
 *                                the closed curve is also returned.    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDrawCurve.                                   *
 *      History         : Written on November 27, 1991 by Hsiu-Mei Hung.*
 *                        Modified on March 7, 1993 by Krishna Iyer.    *
 *                        Modified 6/22/94 to use dynamically allocated
 *                           memory by Dewey Odhner
 *                        Modified 6/24/94 to use correct algorithm
 *                           by Dewey Odhner
 *                        Modified 7/6/94 to allocate correct amount of
 *                           memory at *vertices by Dewey Odhner
 *                                                                      *
 ************************************************************************/
int v_close_curve ( PointInfo** tpoints, int tnpoints, int max_points,
    X_Point** points, int* npoints, X_Point** vertices, int* nvertices )
{
        X_Point *tpts ; 
        int tnpts, i, j, result, loop_found,
            vertices_passed, j2, vertices_passed2;
        PointInfo *more_points, *loop_start, *loop_end,
            *crossing1, *crossing2;

        if (tnpoints == 0) return(277) ;

        /* link up starting and ending points and draw the connecting line */
        result=VComputeLine((*tpoints)[tnpoints-1].x,(*tpoints)[tnpoints-1].y,
                            (*tpoints)[0].x,(*tpoints)[0].y,&tpts,&tnpts) ;
        if (result != 0) {
            if (tpts != NULL) free(tpts) ;
            return(result) ;
        }
        if (tnpoints+tnpts-1 >= max_points) {
            while (max_points < tnpoints+tnpts-1)
                max_points += 256;
            more_points =
                (PointInfo *)realloc(*tpoints, max_points*sizeof(PointInfo));
            if (more_points == NULL) {
                free(tpts) ;
                return(1) ;
            }
            *tpoints = more_points;
        }
        for (i=1; i<tnpts; i++, tnpoints++) {
            (*tpoints)[tnpoints].x=tpts[i].x ;
            (*tpoints)[tnpoints].y=tpts[i].y ;
            (*tpoints)[tnpoints].vertex=0 ;
        }
        (*tpoints)[tnpoints-1].vertex = 1;
        free(tpts) ;

        /* Find first loop. */
        loop_start = *tpoints;
        loop_end = *tpoints+tnpoints-1;
        vertices_passed = loop_found = 0;
        for (j=0; j<tnpoints-1; j++)
            if ((*tpoints)[j].vertex) {
                for (vertices_passed2=0,j2=0;
                        vertices_passed2<vertices_passed-1; j2++)
                    if ((*tpoints)[j2].vertex) {
                        if (V_segment_intersection(*tpoints+j,
                                *tpoints+j2, &crossing1, &crossing2)) {
                            if (crossing1 < loop_end) {
                                loop_end = crossing1;
                                loop_start = crossing2;
                                *nvertices= vertices_passed-vertices_passed2+1;
                                loop_found = 1;
                            }
                        }
                        vertices_passed2++;
                    }
                if (loop_found)
                    break;
                vertices_passed++;
            }
        loop_start->vertex = loop_end->vertex = 1;
        if (!loop_found)
            *nvertices = vertices_passed;

        *npoints = (int)(loop_end-loop_start);
        if (loop_end->x!=loop_start->x || loop_end->y!=loop_start->y) {
            (*npoints)++;
            (*nvertices)++;
        }
        *points=(X_Point *)malloc(*npoints*sizeof(X_Point)) ;
        if (*points == NULL) return(1) ;
        *vertices=(X_Point *)malloc(*nvertices*sizeof(X_Point)) ;
        if (*vertices == NULL) {
            free(*points);
            return(1) ;
        }
        for (i=j=0; i<*npoints; i++) {
            (*points)[i].x=loop_start[i].x ;
            (*points)[i].y=loop_start[i].y ;
            if (loop_start[i].vertex == 1) {
                (*vertices)[j].x=loop_start[i].x ;
                (*vertices)[j].y=loop_start[i].y ;
                j++ ;
            }
        }
        return(0) ;
}

/************************************************************************
 *      Function        : V_segment_intersection
 *      Description     : This function will detect if there was a      *
 *                        crossover between two digital lines, and if
 *                        so, where.
 *      Return Value    : 0 - no intersection
 *                        276 - crossover occured.
 *      Parameters      : line1, line2 - the two digital lines.  The
 *                           first and last (exactly two) pixels of each
 *                           line must have the vertex flags set.
 *                        where1, where2 - the location in each line
 *                           where the crossover occured.
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : VDrawCurve.                                   *
 *      History         : Written 6/24/94 by Dewey Odhner
 *                                                                      *
 ************************************************************************/
static int V_segment_intersection ( PointInfo* line1, PointInfo* line2,
    PointInfo** where1,  PointInfo** where2 )
{
    int box_left, box_right, box_top, box_bottom, l1_last, l2_last,
        l1_first, l2_first;
    enum {GO_RIGHT, GO_LEFT, GO_UP, GO_DOWN} direction;

    for (l1_last=1; !line1[l1_last].vertex; l1_last++)
        ;
    for (l2_last=1; !line2[l2_last].vertex; l2_last++)
        ;
    box_left = box_right = line1[0].x;
    if (line1[l1_last].x > line1[0].x)
    {   box_left = line1[0].x;
        box_right = line1[l1_last].x;
    }
    else
    {   box_left = line1[l1_last].x;
        box_right = line1[0].x;
    }
    if (line2[l2_last].x > line2[0].x)
    {   if (line2[0].x > box_left)
            box_left = line2[0].x;
        if (line2[l2_last].x < box_right)
            box_right = line2[l2_last].x;
    }
    else
    {   if (line2[l2_last].x > box_left)
            box_left = line2[l2_last].x;
        if (line2[0].x < box_right)
            box_right = line2[0].x;
    }
    if (box_left > box_right)
        return (0);
    box_top = box_bottom = line1[0].y;
    if (line1[l1_last].y > line1[0].y)
    {   box_top = line1[0].y;
        box_bottom = line1[l1_last].y;
    }
    else
    {   box_top = line1[l1_last].y;
        box_bottom = line1[0].y;
    }
    if (line2[l2_last].y > line2[0].y)
    {   if (line2[0].y > box_top)
            box_top = line2[0].y;
        if (line2[l2_last].y < box_bottom)
            box_bottom = line2[l2_last].y;
    }
    else
    {   if (line2[l2_last].y > box_top)
            box_top = line2[l2_last].y;
        if (line2[0].y < box_bottom)
            box_bottom = line2[0].y;
    }
    if (box_top > box_bottom)
        return (0);
    if (line1[l1_last].x > line1[0].x)
        if (line1[l1_last].y-line1[0].y > line1[l1_last].x-line1[0].x)
            direction = GO_DOWN;
        else
            if (line1[0].y-line1[l1_last].y > line1[l1_last].x-line1[0].x)
                direction = GO_UP;
            else
                direction = GO_RIGHT;
    else
        if (line1[l1_last].y-line1[0].y > line1[0].x-line1[l1_last].x)
            direction = GO_DOWN;
        else
            if (line1[0].y-line1[l1_last].y > line1[0].x-line1[l1_last].x)
                direction = GO_UP;
            else
                direction = GO_LEFT;
    switch (direction) {
        case GO_RIGHT:
        case GO_LEFT:
            if ((direction==GO_RIGHT) == (line2[l2_last].x>line2[0].x))
            { /* Go forward through line2. */
                for (l2_first=0; line2[l2_first].x<box_left||
                        line2[l2_first].x>box_right; l2_first++)
                    ;
                for (l1_first=0; l1_first<=l1_last; l1_first++)
                    for (; l2_first<=l2_last &&
                            line2[l2_first].x==line1[l1_first].x; l2_first++)
                    {   if (line2[l2_first].y == line1[l1_first].y)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_first;
                            return (276);
                        }
                        if (l1_first<l1_last && l2_first<l2_last &&
                                line1[l1_first+1].x==line2[l2_first+1].x &&
                                line2[l2_first+1].y==line1[l1_first].y &&
                                line1[l1_first+1].y==line2[l2_first].y)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_first+1;
                            return (276);
                        }
                    }
            }
            else
            { /* Go backward through line2. */
                while (line2[l2_last].x<box_left ||
                        line2[l2_last].x>box_right)
                    l2_last--;
                for (l1_first=0; l1_first<=l1_last; l1_first++)
                    for (; l2_last>=0 && line2[l2_last].x==line1[l1_first].x;
                            l2_last--)
                    {   if (line2[l2_last].y == line1[l1_first].y)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_last;
                            return (276);
                        }
                        if (l1_first<l1_last && l2_last>0 &&
                                line1[l1_first+1].x==line2[l2_last-1].x &&
                                line2[l2_last-1].y==line1[l1_first].y &&
                                line1[l1_first+1].y==line2[l2_last].y)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_last;
                            return (276);
                        }
                    }
            }
            break;
        case GO_UP:
        case GO_DOWN:
            if ((direction==GO_UP) == (line2[l2_last].y<line2[0].y))
            { /* Go forward through line2. */
                for (l2_first=0; line2[l2_first].y<box_top||
                        line2[l2_first].y>box_bottom; l2_first++)
                    ;
                for (l1_first=0; l1_first<=l1_last; l1_first++)
                    for (; l2_first<=l2_last &&
                            line2[l2_first].y==line1[l1_first].y; l2_first++)
                    {   if (line2[l2_first].x == line1[l1_first].x)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_first;
                            return (276);
                        }
                        if (l1_first<l1_last && l2_first<l2_last &&
                                line1[l1_first+1].y==line2[l2_first+1].y &&
                                line2[l2_first+1].x==line1[l1_first].x &&
                                line1[l1_first+1].x==line2[l2_first].x)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_first+1;
                            return (276);
                        }
                    }
            }
            else
            { /* Go backward through line2. */
                while (line2[l2_last].y<box_top ||
                        line2[l2_last].y>box_bottom)
                    l2_last--;
                for (l1_first=0; l1_first<=l1_last; l1_first++)
                    for (; l2_last>=0 && line2[l2_last].y==line1[l1_first].y;
                            l2_last--)
                    {   if (line2[l2_last].x == line1[l1_first].x)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_last;
                            return (276);
                        }
                        if (l1_first<l1_last && l2_last>0 &&
                                line1[l1_first+1].y==line2[l2_last-1].y &&
                                line2[l2_last-1].x==line1[l1_first].x &&
                                line1[l1_first+1].x==line2[l2_last].x)
                        {   *where1 = line1+l1_first;
                            *where2 = line2+l2_last;
                            return (276);
                        }
                    }
            }
            break;
    }
    return (0);
}


/************************************************************************
 *                                                                      *
 *      Function        : VComputeLine                                  *
 *      Description     : This function will compute the points         *
 *                        coordinates of the line between the two       *
 *                        specified points. unction VComputeLine also   *
 *                        allocates the memory space for buf and returns*
 *                        the x, y coordinates of each point along      *
 *                        the line between two specified points to the  *
 *                        *points, the number of points stored to buf to*
 *                        npoints. If the first point is the same as the*
 *                        second point, this function will return the   *
 *                        x, y coordinates of the first point to        *
 *                        the *points, and 1 to the nptr. You can use C *
 *                        function free to free points.                 *
 *                        If the memory allocation error happens, this  *
 *                        function will return 1. If there is no error, *
 *                        this function will return 0.                  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x1, y1 - Specifies the first point of the    *
 *                              line.                                   *
 *                         x2, y2 - Specifies the second point of the   *
 *                              line.                                   *
 *                         points - Returns an array of points(x,y)     *
 *                              along the line to struct X_Point.        *
 *                         npoints - Returns the number of the points   *
 *                              along the line.                         *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 29,1989 by Hsiu-Mei Hung.*
 *                        Modified on March 10, 1993 by R.J.Goncalves.  *
 *                                                                      *
 ************************************************************************/
int VComputeLine ( int x1, int y1, int x2, int y2, X_Point** points,
    int* npoints )
{
 
        int dx, dy, incr1, incr2, incr_y, incr_x;
        register int x, y, d, xend, yend;
        int max_delta;
        int index;
        int index_incr;
 
 
        if (y2<y1 && x2>x1) incr_y = -1;
        else
        if (x2<x1 && y2>y1) incr_y = -1;
        else incr_y = 1;
 
        dx = abs(x2-x1);
        dy = abs(y2-y1);
        if(dx>=dy)
        {
                max_delta = dx;
                d = 2*dy-dx;
                incr1 = 2*dy;
                incr2 = 2*(dy-dx);
 
                if (x1>x2)
                {
                        x = x2;
                        y = y2;
                        xend = x1;
                }
                else
                {  
                        x = x1;
                        y = y1;
                        xend = x2;
                }
 
        }
        else
        {  
                max_delta = dy;
                d = 2*dx-dy;
                incr1 = 2*dx;
                incr2 = 2*(dx-dy);
 
 
                if (y1>y2)
                {
                        x = x2;
                        y = y2;
                        yend = y1;
                }
                else   
                {
                        x = x1;
                        y = y1;
                        yend = y2;
                }
        }
 
        *npoints = 0;
        *points=(X_Point *)malloc((max_delta+1)*sizeof(X_Point)) ;
        if (*points == NULL) return(1) ;
        *npoints = max_delta+1;
 
        index = 0;
        index_incr = 1;
        if( ( dy>dx && y2<y1) || (dy<=dx && x2<x1) )
        {
                index = max_delta;
                index_incr = -1;
        }
 
 
        (*points)[index].x=x ;
        (*points)[index].y=y ;
 
        if(dx>=dy)
        {
                while( x < xend)
                {
                        x++;
                        index += index_incr;
                        if (d<0)
                        d += incr1;
                        else
                        {
                                y += incr_y;
                                d += incr2;
                        }
                        (*points)[index].x=x ;
                        (*points)[index].y=y ;
                }
        }
        else
        {
                incr_x = incr_y;
                while( y < yend)
                {
                        y++;
                        index += index_incr;
                        if (d<0)
                        d += incr1;
                        else
                        {
                                x += incr_x;
                                d += incr2;
                        }
                        (*points)[index].x=x ;
                        (*points)[index].y=y ;
                }
        } 
 
        return(0);
}


/*************************************************************************/
