/*
  Copyright 1993-2012, 2021 Medical Image Processing Group
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

//======================================================================
/**
* \file:  ROIStatisticalCanvas.cpp
* \brief  ROIStatisticalCanvas class implementation
* \author Xinjian Chen, Ph.D.
*
* Copyright: (C) 2008
*
* The world is so beautiful that I can not help stopping smile.
*/
//======================================================================
#include  "cavass.h"
#define TEXT_LINES 10
#define LINE1      "                            Density                      Gradient              Area/Vol "
#define LINE2      "no   slice#   roi   min   max    mean  std.dev   min   max    mean  std.dev    sq/cu"


int ROIStatisticalCanvas::DrawImgLine(const wxPoint  pos)
{   
	int result;

	int x = pos.x;
	int y = pos.y;
	if((x<=icon->dx1)&&(y<=icon->dy1) && (x>icon->x)&&(y>icon->y) )	
	{

		result=v_draw_line(pos.x, pos.y, &tpoints, &tnpoints, &max_points) ;
		if (result != 0) 
			free(tpoints);
		return(result) ;
	}

	return 0;
}



/************************************************************************
*    Function Name : UpdateIconLine
*    Description   : Display a line inside the icon window. 
*    Return Value  : None.
*    Parameters    : cur_point  = a pointer to the (x,y) coordinates of 
*                                 the first chosen point. 
*                    cur_point1 = a pointer to the (x,y) coordinates of 
*                                 the second chosen point. 
*    Side Effects  : None.                        
*    Entry Cond.   : You should be running CheckImageWindowEvent().
*    Related Func. : CheckImageWindowEvent(), CheckWindowPointIndex() 
*    History       : Created by Srnivas and modified by Alexandre 
*                    Falcao in September, 1993.
************************************************************************/

void ROIStatisticalCanvas::UpdateIconLine(struct Z_POINT *cur_point, struct Z_POINT *cur_point1)
{
	MagImg *icon_ptr;
	int i, draw_x,draw_y, draw_x1,draw_y1;

	wxClientDC  dc(this);
	PrepareDC(dc);		
	dc.SetPen( wxPen(wxColour(Yellow)) );

	for(i=0,icon_ptr=icon;i<num_of_scenes;i++,icon_ptr=icon_ptr->next)
	{
		if( CheckWindowPointIndex(cur_point,icon_ptr)!=0 && CheckWindowPointIndex(cur_point1,icon_ptr)!=0 )
		{
			draw_x = (int)rint((double)((cur_point->x - icon_ptr->img_offset[0])*((double)(1.0*icon_ptr->width)/((double)(1.0*icon_ptr->img_dim[0])))+icon_ptr->x));
			draw_y = (int)rint((double)((cur_point->y - icon_ptr->img_offset[1])*((double)(1.0*icon_ptr->height)/((double)(1.0*icon_ptr->img_dim[1])))+icon_ptr->y));

			draw_x1 = (int)rint((double)((cur_point1->x - icon_ptr->img_offset[0])*((double)(1.0*icon_ptr->width)/((double)(1.0*icon_ptr->img_dim[0])))+icon_ptr->x));
			draw_y1 = (int)rint((double)((cur_point1->y - icon_ptr->img_offset[1])*((double)(1.0*icon_ptr->height)/((double)(1.0*icon_ptr->img_dim[1])))+icon_ptr->y));

			if((draw_x<=icon_ptr->dx1)&&(draw_y<=icon_ptr->dy1) && (draw_x>icon_ptr->x)&&(draw_y>icon_ptr->y) 
				&& (draw_x1<=icon_ptr->dx1)&&(draw_y1<=icon_ptr->dy1) && (draw_x1>icon_ptr->x)&&(draw_y1>icon_ptr->y) )
			{
				dc.DrawLine(draw_x,draw_y,draw_x1,draw_y1);
				//VDisplayOverlayLine(icon_ptr->win,ovl,draw_x,draw_y,draw_x1,draw_y1);
			}

		}
	}
}

/***************************************************************************
*    Function Name : CheckWindowPointIndex
*    Description   : Check if a point belongs to a window.
*    Return Value  : 1 = the point belongs.
*                    0 = the point does not belong.
*    Parameters    : c_point = a pointer to the (x,y) coordinates of 
*                              the point.
*                    c_win   = a pointer to the magimage structure 
*                              which contains the window.
*    Side Effects  : None.
*    Entry Cond.   : None.
*    Related Func. : CheckImageWindowEvent(), UpdateRoiPoint(), 
*                    UpdateRoiLine(), UpdateIconPoint(), UpdateIconLine() 
*    History       : Created by Srnivas and documented by Alexandre Falcao 
*                    in September 16th, 1993.
***************************************************************************/

int ROIStatisticalCanvas::CheckWindowPointIndex(struct Z_POINT *c_point, MagImg * c_win)
{
	int i;
	for(i=2;i<3;i++) //scene[c_win->scene_num].vh.scn.dimension
	{
		if(c_point->index[i] != c_win->slice_index[i])
			return(0);
	}
	return(1);
}



/************************************************************************
*    Function Name : CheckPointsIndex 
*    Description   : Check if two consectives points of the profile
*                    are in the same slice.   
*    Return Value  : 1 = they are. 
*                    0 = they are not. 
*    Parameters    : c_point  = pointer to the one profile point.
*                    c_point1 = pointer to the other profile point.
*    Side Effects  : None.
*    Entry Cond.   : You should be running FindROIStatisticalValues() before. 
*    Related Func. : FindROIStatisticalValues()
*    History       : Created by Srnivas and documented by Alexandre Falcao 
*                    in September 16th, 1993.
************************************************************************/
int ROIStatisticalCanvas::CheckPointsIndex(struct Z_POINT *c_point, struct Z_POINT *c_point1)
{
	int i;
	for(i=2;i<mCavassData->m_vh.scn.dimension;i++)
		if(c_point->index[i] != c_point1->index[i]) return(0);
	return(1);
}

/************************************************************************* 
*    Function Name : FindDistance
*    Description   : Find the distance between two points of the profile. 
*    Return Value  : a = the distance.          
*    Parameters    : gp1 = pointer to the one profile point.
*                    gp2 = pointer to the other profile point.
*                    z_diff = the difference between the z coordinate of 
*                             the points.  
*    Side Effects  : None.
*    Entry Cond.   : You should be running FindROIStatisticalValues() before. 
*    Related Func. : FindROIStatisticalValues(),GetScaleUnits()
*    History       : Created by Srnivas and modified by Alexandre Falcao 
*                    in September, 1993.
*************************************************************************/
double ROIStatisticalCanvas::FindDistance(struct G_POINT *gp1, struct G_POINT *gp2,float z_diff)
{
	double a,b;
	char msg[20];

	b = (GetScaleUnits(msg,mCavassData->m_vh.scn.measurement_unit[0]) / 
		GetScaleUnits(msg,mCavassData->m_vh.scn.measurement_unit[2]));

	a = sqrt(((gp1->x - gp2->x) * (gp1->x - gp2->x) * 
		mCavassData->m_vh.scn.xypixsz[0] * mCavassData->m_vh.scn.xypixsz[0]) + 
		((gp1->y - gp2->y) * (gp1->y - gp2->y) * 
		mCavassData->m_vh.scn.xypixsz[1] * mCavassData->m_vh.scn.xypixsz[1]) +
		(z_diff * z_diff) * b * b);
	return(a);

}

/***************************************************************************
*    Function Name : GetValue
*    Description   : Get the ROIStatistical value for a point on the image display.
*    Return Value  : ret_value = the ROIStatistical value. 
*    Parameters    : gvpoint = pointer to the point.
*                    same_slice = indication to know if the current point
*                                 and the previous point are on the same
*                                 slice. 
*    Side Effects  : None.                                
*    Entry Cond.   : None.
*    Related Func. : FindROIStatisticalValues(), ComputeStats(), GetPointer()
*    History       : Created by Srnivas and documented by Alexandre Falcao 
*                    in September 16th, 1993.
*************************************************************************/
int ROIStatisticalCanvas::GetValue(struct Z_POINT *gvpoint)
{
	int value = 0;

	if (mCavassData->m_size==1) 
	{
		unsigned char*   cData = NULL;
		cData = (unsigned char*)m_sliceIn->getSlice(gvpoint->index[2]); //A.m_data;		 
		value = cData[ gvpoint->y*mCavassData->m_xSize + gvpoint->x ];
	}
	else if (mCavassData->m_size==2) 
	{
		unsigned short*   sData = NULL;
		sData = (unsigned short*)m_sliceIn->getSlice(gvpoint->index[2]); //A.m_data;		 
		value = sData[ gvpoint->y*mCavassData->m_xSize + gvpoint->x ];
	}
	else if (mCavassData->m_size==4) 
	{
		int*   iData = NULL;
		iData = (int*)m_sliceIn->getSlice(gvpoint->index[2]); //A.m_data;		 
		value = iData[ gvpoint->y*mCavassData->m_xSize + gvpoint->x ];
	}

	return value;

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
int ROIStatisticalCanvas::v_draw_line ( int curx, int cury, PointInfo** points, int* npoints, int* max_points )
{
	wxPoint *tpoints ;
	int tnpoints, result, i ;
	PointInfo *more_points;
	int w, h;

	wxClientDC  dc(this);
	PrepareDC(dc);		
	dc.SetPen( wxPen(wxColour(Yellow)) );
	dc.GetSize( &w, &h );	
	dc.SetTextBackground( *wxBLACK );
	dc.SetTextForeground( wxColour(Yellow) );

	//  XSetForeground(lib_display,gc,foreground) ;
	if (*npoints == 0)
	{
		dc.DrawPoint(curx, cury);
		//    XDrawPoint(lib_display,win,gc,curx,cury) ;
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
	if ((*npoints+tnpoints-1) >= *max_points) 
	{
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
	//   XDrawPoints(lib_display,win,gc,tpoints,tnpoints,CoordModeOrigin) ;
	dc.DrawPoint(tpoints[0].x, tpoints[0].y);
	for (i=1; i<tnpoints; i++, (*npoints)++)
	{
		dc.DrawPoint(tpoints[i].x, tpoints[i].y);
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
*      Parameters      :  win - the subwindows ID.                     *
*                         gc - the gc of the window (win).             *
*                         foreground - the pixel value of the color    *
*                                cell that indicates foreground color. *
*                         tpoints - the points of the first line.      *
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
int ROIStatisticalCanvas::v_close_curve ( PointInfo** tpoints, int tnpoints, int max_points, wxPoint** points,
									   int* npoints, wxPoint** vertices, int* nvertices)
{
	wxClientDC  dc(this);
	int w, h;
	PrepareDC(dc);		
	dc.SetPen( wxPen(wxColour(Green)) );
	dc.GetSize( &w, &h );	
	dc.SetTextBackground( *wxBLACK );
	dc.SetTextForeground( wxColour(Yellow) );

	wxPoint *tpts ; 
	int tnpts, i, j, result, loop_found,
		vertices_passed, j2, vertices_passed2;
	PointInfo *more_points, *loop_start, *loop_end,
		*crossing1, *crossing2;

	if (tnpoints == 0) return(277) ;

	/* Erase curve. */
	tpts=(wxPoint *)malloc(tnpoints*sizeof(wxPoint)) ;
	if (tpts == NULL) return(1) ;
	for (i=0; i<tnpoints; i++) {
		tpts[i].x=(*tpoints)[i].x ;
		tpts[i].y=(*tpoints)[i].y ;
	}
	//  XSetForeground(lib_display,gc,0) ;
	//  XDrawPoints(lib_display,win,gc,tpts,tnpoints,CoordModeOrigin) ;
	/*for (i=0; i<tnpoints; i++, (*npoints)++)
	{
	dc.DrawPoint(tpts[i].x, tpts[i].y);         
	}*/
	free(tpts);

	/* link up starting and ending points and draw the connecting line */
	result=VComputeLine((*tpoints)[tnpoints-1].x,(*tpoints)[tnpoints-1].y,
		(*tpoints)[0].x,(*tpoints)[0].y,&tpts,&tnpts) ;
	if (result != 0) 
	{
		if (tpts != NULL) free(tpts) ;
		return(result) ;
	}
	if (tnpoints+tnpts-1 >= max_points) 
	{
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

		*npoints = loop_end-loop_start;
		if (loop_end->x!=loop_start->x || loop_end->y!=loop_start->y) {
			(*npoints)++;
			(*nvertices)++;
		}
		*points=(wxPoint *)malloc(*npoints*sizeof(wxPoint)) ;
		if (*points == NULL) return(1) ;
		*vertices=(wxPoint *)malloc(*nvertices*sizeof(wxPoint)) ;
		if (*vertices == NULL) 
		{
			free(*points);
			return(1) ;
		}
		for (i=j=0; i<*npoints; i++) 
		{
			(*points)[i].x=loop_start[i].x ;
			(*points)[i].y=loop_start[i].y ;
			if (loop_start[i].vertex == 1) {
				(*vertices)[j].x=loop_start[i].x ;
				(*vertices)[j].y=loop_start[i].y ;
				j++ ;
			}
		}
		//   XSetForeground(lib_display,gc,foreground) ;
		//   XDrawPoints(lib_display,win,gc,*points,*npoints,CoordModeOrigin) ;
		for (i=0; i<*npoints; i++)
		{
			dc.DrawPoint((*points)[i].x, (*points)[i].y);         
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
int ROIStatisticalCanvas::V_segment_intersection ( PointInfo* line1, PointInfo* line2,
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
*                              along the line to struct XPoint.        *
*                         npoints - Returns the number of the points   *
*                              along the line.                         *
*      Side effects    : None.                                         *
*      Entry condition : None.                                         *
*      Related funcs   : None.                                         *
*      History         : Written on September 29,1989 by Hsiu-Mei Hung.*
*                        Modified on March 10, 1993 by R.J.Goncalves.  *
*                                                                      *
************************************************************************/
int ROIStatisticalCanvas::VComputeLine ( int x1, int y1, int x2, int y2, wxPoint** points, int* npoints )
{

	int dx, dy, incr1, incr2, incr_y, incr_x;
	int x, y, d, xend=0, yend=0;
	int max_delta;
	int index;
	int index_incr;


	if (y2<y1 && x2>x1) 
		incr_y = -1;
	else
	{
		if (x2<x1 && y2>y1) 
			incr_y = -1;
		else 
			incr_y = 1;
	}

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
	*points=(wxPoint *)malloc((max_delta+1)*sizeof(wxPoint)) ;
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


/*****************************************************************************
*    Function Name : GetScaleUnits
*    Description   : Get the correct unit to a measure. This measure can
*                    be of distance or of time. This function also can be
*                    used to convert distance measures.
*    Return Value  : scale factors.                                
*    Parameters    : gsc   = string for keep the measure name. 
*                    g_int = code of the 3DVIEWNIX header to know the 
*                            unit of the scene measures. 
*    Side Effects  : None.                           
*    Entry Cond.   : None.                           
*    Related Func. : DisplayScale(), FindDistance(), DisplayRoiStatistics(), 
*                    DisplayStats(), ComputeStats(), SaveStats()    
*    History       : Created by Srnivas and modified by Alexandre 
*                    Falcao in September, 1993.
*****************************************************************************/

double ROIStatisticalCanvas::GetScaleUnits(char *gsc, short g_int)
{                                    /* Distance Units */
	if(g_int == 0)
	{
		sprintf(gsc,"km");
		return(1000000.0);
	}
	else
		if(g_int == 1){
			sprintf(gsc,"m");
			return(1000.0);
		}
		else
			if(g_int == 2){
				sprintf(gsc,"cm");
				return(100.0);
			}
			else
				if(g_int == 3){
					sprintf(gsc,"mm");
					return(1.0);
				}
				else
					if(g_int == 4){
						sprintf(gsc,"u");
						return(.001);
					}
					else                               /* Time Units */
						if(g_int == 5){
							sprintf(gsc,"sec");
							return 1;
						}
						else
							if(g_int == 6){
								sprintf(gsc,"msec");
								return 1;
							}
							else
								if(g_int == 7){
									sprintf(gsc,"usec");
									return 1;
								}
								else{
									/*XBell(display,0);
									VDisplayDialogMessage("not unit found");
									XFlush(display);*/
									return(1);
								}   
}


/************************************************************************
*    Function Name : RemoveProfileInfo
*    Description   : Removes the profile information.
*    Return Value  : None.                       
*    Parameters    : None.                       
*    Side Effects  : Destroys the profile window and reset the globals 
*                    icon_zpoint, prev_point, first_point, 
*                    very_first_point, and no_of_icon_points. 
*    Entry Cond.   : The profile is being displayed.
*    Related Func. : FreeGraphs(), FindROIStatisticalProfile()
*    History       : Created by Srnivas and documented by Alexandre 
*                    Falcao in September 15th, 1993. 
************************************************************************/

void ROIStatisticalCanvas::RemoveProfileInfo()
{
	struct Z_POINT *ptr,*ptr1;
	int i,j;

	//FreeGraphs();
	//VClearWindow(img,0,0,0,0);

	for(i=0;i<num_of_scenes;i++) 
	{
		ptr=icon_zpoint[i].next;
		while (ptr!=NULL) 
		{
			ptr1=ptr;
			ptr=ptr->next;
			free(ptr1);
		}
		icon_zpoint[i].next=NULL;
		ptr=prev_point[i].next;
		while (ptr!=NULL) 
		{
			ptr1=ptr;
			ptr=ptr->next;
			free(ptr1);
		}
		prev_point[i].next=NULL;
	}
	if(mCavassData->m_vh.scn.dimension > 3)
		i=mCavassData->m_vh.scn.num_of_subscenes[0];
	else
		i=1;
	for(j=0;j<i;j++)
		first_point[j]=0;

	very_first_point = 0;  
	no_of_icon_points=0;

	Stat *tmpstat = NULL;	
	Stat *tmpstat2 = NULL;	
	tmpstat = RoiStats;	
	while(tmpstat!= NULL) 
	{
		if( tmpstat->next!= NULL )
			tmpstat2 = tmpstat->next;
		else
			tmpstat2 = NULL;		

		if( tmpstat->pVertices != NULL )
			free( tmpstat->pVertices );

		if( tmpstat != NULL )
			free( tmpstat );		

		tmpstat = tmpstat2;		
	}	
	RoiStats = NULL;

	if(TotStats != NULL)
		free( TotStats );

	GetPrevRoistats();  
	num_of_roistats=0;    

}


/**************************************************************************
*    Function Name : CheckMeasureWindowEvent
*    Description   : This function checks points chosen inside the 
*                    profile window and displays the length measure, 
*                    computed a long the X axis, between them.   
*    Return Value  : None.
*    Parameters    : None.
*    Side Effects  : Display the results under the profile window and Take 
*                    free the allocated space of the global graph_data. 
*    Entry Cond.   : You should call FindROIStatisticalValues() before.
*    Related Func. : FindROIStatisticalValues(), GetScaleUnits()
*    History       : Created by Srnivas and documented by Alexandre 
*                    Falcao in September 20th, 1993.
*                    Modified: 3/25/97 parameter mismatch corrected in
*                       XWarpPointer call by Dewey Odhner.
*                    Modified: 2/3/04 correct pointer coordinates used
*                       by Dewey Odhner.
**************************************************************************/

int ROIStatisticalCanvas::MeasureWidth(const wxPoint  pos)
{
	int  w, h;	
	char msg[2][80];
	double length;
	wxClientDC  dc(this);
	PrepareDC(dc);		
	dc.SetPen( wxPen(wxColour(Yellow)) );
	dc.GetSize( &w, &h );	
	dc.SetTextBackground( *wxBLACK );
	dc.SetTextForeground( wxColour(Yellow) );

	//VCheckEventsInButtonWindow(&event,refresh_prof);
	if( pos.x > m_ROIStatisticalGraphLeft &&  pos.x < m_ROIStatisticalGraphRight  && pos.y > m_ROIStatisticalGraphTop &&  pos.y < m_ROIStatisticalGraphBottom )		
	{
		if(two_lines==0)
		{
			firstMeasureX = pos.x;
			dc.DrawLine( pos.x, m_ROIStatisticalGraphBottom +10, pos.x, m_ROIStatisticalGraphBottom +20 );
			//XDrawLine(display,img,graph_box.gc,x,graph_box.y-I_I_Y,x,graph_box.y1+10-I_I_Y);
			two_lines=(two_lines+1)%2;			
		}
		else if(two_lines==1)
		{
			secondMeasureX = pos.x;

			//XDrawLine(display,img,gc,x,graph_box.y-I_I_Y,x,graph_box.y1+10-I_I_Y);			
			length = (1.0*tot_dist*abs(firstMeasureX-secondMeasureX));
			length = length / (m_scale*mCavassData->m_xSize); //(graph_box.x1 - graph_box.x - 1) ;
			GetScaleUnits(msg[1],((DISPLAY_MODE == 0) ? (mCavassData->m_vh.scn.measurement_unit[0]) : (mCavassData->m_vh.scn.measurement_unit[3])));
			sprintf(msg[0],"%5.1f%s",length,msg[1]);

			dc.DrawLine( firstMeasureX, m_ROIStatisticalGraphBottom +10, firstMeasureX, m_ROIStatisticalGraphBottom +20 );
			dc.DrawLine( pos.x, m_ROIStatisticalGraphBottom +10, pos.x, m_ROIStatisticalGraphBottom +20 );
			dc.DrawLine( firstMeasureX, m_ROIStatisticalGraphBottom +15, secondMeasureX,m_ROIStatisticalGraphBottom +15 );
			wxString s = wxString::Format( "%s", msg[0]);
			dc.DrawText( s, (firstMeasureX+secondMeasureX)/2-20, m_ROIStatisticalGraphBottom +20 );

			//XDrawLine(display,img,gc,fx-I_I_X,temp_msg_y,fx-I_I_X,temp_msg_y+font_height);
			//XDrawLine(display,img,gc,sx-I_I_X,temp_msg_y,sx-I_I_X,temp_msg_y+font_height);
			//XDrawLine(display,img,gc,fx-I_I_X,temp_msg_y+font_height/2, sx-I_I_X,temp_msg_y+font_height/2);
			//VDisplayImageMessage(img,msg[0],1,(fx+sx)/2-I_I_X,temp_msg_y+font_height/2+1);			

			two_lines=(two_lines+1)%2;
		}
	}



	return 1;
}


/*  Calculating the Statistics */

/************************************************************************
*    Function Name :                                                    *
*    Description   : None.                                              *
*    Return Value  : None.                                              *
*    Parameters    : None.                                              *
*    Side Effects  : None.                                              *
*    Entry Cond.   : None.                                              *
*    Related Func. : None.                                              *
*    History       : None.                                              *
************************************************************************/

int ROIStatisticalCanvas::ComputeStats()
{

	MagImg *cur_img = NULL;
	wxRegion region;
	Stat   *tmpstat;

	int    min_x=0,max_x=0,min_y=0,max_y=0,
		index_x,index_y,index1;

	double min_val=0,max_val=0,mean_val=0,temp_val,
		tot_val=0,tot_sqr=0,std_dev;
	int    count=0;

	int    lslice,rslice,tmp_slice;

	/*
	* Other statistics
	*/

	double pixelsize, area;

	/*
	* variables for gradient magnitude calculation
	*/

	double gsu_x_dist, gsu_y_dist, gsu_z_dist1, gsu_z_dist2;
	double min_grad=0,max_grad=0,mean_grad=0,
		temp_grad,temp_ugrad,
		tot_grad=0,tot_sqr_grad=0,std_dev_grad,
		val1,val2,
		dist1,dist2,
		dx,dy,dz,
		fx,fy,fz,cf;

	char msg[20];
	bool first_z=TRUE;
	struct Z_POINT z_point,
		left_p,
		right_p,
		dleft_p,
		dright_p;


	/*
	* Initialize the size and dimension of the roi to be selected
	*/

	cur_img = icon;
	if((z_point.index = (short *)calloc((mCavassData->m_vh.scn.dimension+1),sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);

	if((left_p.index = (short *)calloc((mCavassData->m_vh.scn.dimension+1),sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);

	if((dleft_p.index = (short *)calloc((mCavassData->m_vh.scn.dimension+1),sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);

	if((right_p.index = (short *)calloc((mCavassData->m_vh.scn.dimension+1),sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);

	if((dright_p.index = (short *)calloc((mCavassData->m_vh.scn.dimension+1),sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);

	z_point.num = cur_img->scene_num;
	left_p.num = cur_img->scene_num;
	right_p.num = cur_img->scene_num;
	dleft_p.num = cur_img->scene_num;
	dright_p.num = cur_img->scene_num;

	for(index1=0; index1 < mCavassData->m_vh.scn.dimension; index1++)
	{ 
		z_point.index[index1] = cur_img->slice_index[index1];
		left_p.index[index1] = cur_img->slice_index[index1];
		dleft_p.index[index1] = cur_img->slice_index[index1];
		right_p.index[index1] = cur_img->slice_index[index1];
		dright_p.index[index1] = cur_img->slice_index[index1];
	}

	if( left_p.index[2] > 0 )
		left_p.index[2] -= 1;
	if( right_p.index[2] < mCavassData->m_zSize )
		right_p.index[2] += 1;


	/*if((GetPointer(mCavassData->tree,left_p.index)==NULL)||(GetPointer(mCavassData->tree,right_p.index)!=NULL))
	{
	if(GradientTypeSwitch==1)
	{
	wxMessageBox( "Can't use 3D Gradient method from this slice!" );
	SwitchGradientType();			
	}
	}*/

	if(nvertices==0)
	{
		wxMessageBox( "No roi chosen!" );	   
		return 0;
	}

	if(RoiDimension == 3)
	{
		lslice = (0 <= (z_point.index[2] - RoiSize/2)?(z_point.index[2] - RoiSize/2):0) ;
		rslice = (mCavassData->m_zSize > (z_point.index[2] + RoiSize/2)?(z_point.index[2] + RoiSize/2):mCavassData->m_zSize-1) ;
	}
	else
	{
		lslice = z_point.index[2];
		rslice = z_point.index[2];
	}



	tmpstat = RoiStats;	
	while(tmpstat->next != NULL) tmpstat=tmpstat->next;

	if((tmpstat->pVertices = (wxPoint *)calloc(nvertices,sizeof(wxPoint)))==NULL)  // for drawing the vertices
		return ERR_OUTOFMEMORY; //handle_error(process,"calloc error for RoiStats");
	else
	{
		for(index1=0; index1 < nvertices; index1++)
		{
			tmpstat->pVertices[index1].x = vertices[index1].x;
			tmpstat->pVertices[index1].y = vertices[index1].y;
		}
		tmpstat->nVertices = nvertices;
	}

	/* Compute pixel area */

	pixelsize = mCavassData->m_vh.scn.xypixsz[0] *  mCavassData->m_vh.scn.xypixsz[1];

	/* Compute the correction factor from z to x units scale */

	cf = (GetScaleUnits(msg,mCavassData->m_vh.scn.measurement_unit[0])/GetScaleUnits(msg,mCavassData->m_vh.scn.measurement_unit[2]));

	printf("   %d,%d\n",lslice,rslice);
	for(tmp_slice=lslice;tmp_slice<=rslice;tmp_slice++)
	{
		z_point.index[2] = tmp_slice;

		//if(GetPointer(smCavassData->tree,z_point.index)==NULL)
		//{
		//	wxMessageBox( "Cannot locate slice %d", tmp_slice );	   			
		//	//sprintf(mesg[0],"Cannot locate slice %d",tmp_slice);

		//}
		//else
		{
			if(tmp_slice == lslice)
			{
				//    if(m_nROIType == 0)
				for(index1=0; index1 < nvertices; index1++)
				{
					vertices[index1].x = cur_img->img_offset[0]+(int)rint((double)((vertices[index1].x-cur_img->x)*cur_img->img_dim[0]*1.0/(cur_img->width*1.0)));
					vertices[index1].y = cur_img->img_offset[1]+(int)rint((double)((vertices[index1].y-cur_img->y)*cur_img->img_dim[1]*1.0/(cur_img->height*1.0)));

					//	point->x = pt_icon->img_offset[0]+(int)rint((double)((x1-pt_icon->x)*pt_icon->img_dim[0]*1.0/(pt_icon->width*1.0))); //+pt_icon->dx
					//	point->y = pt_icon->img_offset[1]+(int)rint((double)((y1-pt_icon->y)*pt_icon->img_dim[1]*1.0/(pt_icon->height*1.0)));  //+pt_icon->dy

				}

				min_x=vertices[0].x;
				min_y=vertices[0].y;
				max_x=vertices[0].x;
				max_y=vertices[0].y; 

				for(index1=0; index1 < nvertices; index1++)
				{
					if(vertices[index1].x < min_x)
						min_x = vertices[index1].x;
					if(vertices[index1].y < min_y)
						min_y = vertices[index1].y;
					if(vertices[index1].x > max_x)
						max_x = vertices[index1].x;
					if(vertices[index1].y > max_y)
						max_y = vertices[index1].y;
				}

				//  VDisplayDialogMessage("Please wait calculating the statistics");
				//  XFlush(display);

				/* create a region for cheking points */

				region = wxRegion(nvertices, vertices, wxWINDING_RULE);

				area=0;
				tot_val=0;tot_grad=0;
				tot_sqr=0;tot_sqr_grad=0;
				std_dev=0;std_dev_grad=0;
				count=0;first_z=TRUE;
				z_point.x=min_x;
				z_point.y=min_y;
				min_val=max_val=(double)GetValue(&z_point);
			}

			gsu_x_dist = mCavassData->m_vh.scn.xypixsz[0];

			gsu_y_dist = mCavassData->m_vh.scn.xypixsz[1];

			if(m_nGradType==1)
		    {
			 left_p.x = min_x;
			 left_p.y = min_y;
			 gsu_z_dist1 = cf * abs(z_point.index[2] - left_p.index[2]);

			 right_p.x = min_x;
			 right_p.y = min_y;
			 gsu_z_dist2 = cf * abs( right_p.index[2] - z_point.index[2]);

			 for(index_x=min_x; index_x<=max_x; index_x++)
			 {
				 for(index_y=min_y; index_y<=max_y; index_y++)
				 {
					 if( region.Contains(index_x,index_y) == wxInRegion )
					 {
						 z_point.x = index_x;
						 z_point.y = index_y;
						 temp_val =(double) GetValue(&z_point);
						 if(min_val > temp_val)
							 min_val = temp_val;
						 if(max_val < temp_val)
							 max_val = temp_val;
						 tot_val += temp_val;
						 tot_sqr += temp_val * temp_val;

						 dleft_p.x = index_x - 1;
						 dleft_p.y = index_y;
						 val1 = (double)GetValue(&dleft_p);
						 dleft_p.x = index_x + 1;
						 dleft_p.y = index_y;
						 val2 = (double)GetValue(&dleft_p);
						 dx = gsu_x_dist + gsu_x_dist;
						 fx = fabs((val1 - val2)*gsu_x_dist*2/dx);

						 dright_p.x = index_x;
						 dright_p.y = index_y - 1;
						 val1 = (double)GetValue(&dright_p);
						 dright_p.x = index_x;
						 dright_p.y = index_y + 1;
						 val2 = (double)GetValue(&dright_p);
						 dy = gsu_y_dist + gsu_y_dist;
						 fy = fabs((val1 - val2)*gsu_y_dist*2/dy);

						 left_p.x = index_x;
						 left_p.y = index_y;
						 val1 = (double)GetValue(&left_p);
						 right_p.x = index_x;
						 right_p.y = index_y;
						 val2 = (double)GetValue(&right_p);
						 dz = fabs(gsu_z_dist1) + fabs(gsu_z_dist2);
						 fz = fabs((val1*gsu_z_dist2 - val2*gsu_z_dist1)*2/dz);

						 temp_ugrad = ( (fx/dx * fx/dx) + (fy/dy * fy/dy) + (fz/dz * fz/dz)) / ((1/dx + 1/dy + 1/dz) * (1/dx + 1/dy + 1/dz));
						 temp_grad = sqrt(temp_ugrad);
						 if(first_z==TRUE){
							 min_grad=temp_grad;
							 max_grad=temp_grad;
							 first_z=FALSE;
						 }
						 if(min_grad > temp_grad)
							 min_grad = temp_grad;
						 if(max_grad < temp_grad)
							 max_grad = temp_grad;
						 tot_grad += temp_grad;
						 tot_sqr_grad += temp_ugrad;

						 count++;
					 }
				 }
			 }
		 }
			else
		 {
			 temp_val =(double) GetValue(&z_point);
			 for(index_x=min_x; index_x<=max_x; index_x++)
			 {
				 for(index_y=min_y; index_y<=max_y; index_y++)
				 {
					 if( region.Contains(index_x,index_y) == wxInRegion )
						 //if(XPointInRegion(region,index_x,index_y)==True)
					 {
						 z_point.x = index_x;
						 z_point.y = index_y;
						 temp_val =(double) GetValue(&z_point);
						 if(min_val > temp_val)
							 min_val = temp_val;
						 if(max_val < temp_val)
							 max_val = temp_val;
						 tot_val += temp_val;
						 tot_sqr += temp_val * temp_val;

						 dleft_p.x = index_x - 1;
						 dleft_p.y = index_y;
						 val1 = (double)GetValue(&dleft_p);
						 dist1 = gsu_x_dist;
						 dleft_p.x = index_x + 1;
						 dleft_p.y = index_y;
						 val2 = (double)GetValue(&dleft_p);
						 dist2 = gsu_x_dist;
						 dx = gsu_x_dist + gsu_x_dist;
						 fx = fabs((val1*dist2 - val2*dist1)*2/dx);

						 dright_p.x = index_x;
						 dright_p.y = index_y - 1;
						 val1 = (double)GetValue(&dright_p);
						 dist1 = gsu_y_dist;
						 dright_p.x = index_x;
						 dright_p.y = index_y + 1;
						 val2 = (double)GetValue(&dright_p);
						 dist2 = gsu_y_dist;
						 dy = gsu_y_dist + gsu_y_dist;
						 fy = fabs((val1*dist2 - val2*dist1)*2/dy);

						 temp_ugrad = ( (fx/dx * fx/dx) + (fy/dy * fy/dy) )/ ((1/dx + 1/dy) * (1/dx + 1/dy));
						 temp_grad = sqrt(temp_ugrad);
						 if(first_z==TRUE)
						 {
							 min_grad=temp_grad;
							 max_grad=temp_grad;
							 first_z=FALSE;
						 }
						 if(min_grad > temp_grad)
							 min_grad = temp_grad;
						 if(max_grad < temp_grad)
							 max_grad = temp_grad;
						 tot_grad += temp_grad;
						 tot_sqr_grad += temp_ugrad;

						 count++;
					 }
				 }
			 }
			}
			mean_val = tot_val/count;
			mean_grad = tot_grad/count;
		}
	}


	if(abs(lslice - rslice) > 0)
	{ 
		left_p.index[2]=lslice;
		right_p.index[2]=rslice;
		dist1 = (cf * abs( left_p.index[2] - right_p.index[2]))/(rslice-lslice); 	
		area = (pixelsize * count) * dist1;  
	}
	else
		area = pixelsize * count;


	std_dev = sqrt((double)((tot_sqr - (2*tot_val*mean_val) + count*mean_val*mean_val)/count));
	std_dev_grad = sqrt((double)((tot_sqr_grad - (2*tot_grad*mean_grad) + count*mean_grad*mean_grad)/count));

	//	VDisplayDialogMessage("Done calculating the stats");
	//	XFlush(display);

	//tmpstat = RoiStats;

	if (mCavassData->m_vh.scn.dimension > 3)
		tmpstat->volume = cur_img->slice_index[3];
	else
		tmpstat->volume = 0;
	tmpstat->slice = cur_img->slice_index[2];
	strcpy(tmpstat->roi_type,ROINameTable[m_nROIType]+5);
	tmpstat->min_density = min_val;
	tmpstat->max_density = max_val;
	tmpstat->mean_density = mean_val;
	tmpstat->stdev_density = std_dev;
	tmpstat->min_gradient = min_grad;
	tmpstat->max_gradient = max_grad;
	tmpstat->mean_gradient = mean_grad;
	tmpstat->stdev_gradient = std_dev_grad;
	tmpstat->area = area;
	if(abs(lslice-rslice)>0)
		tmpstat->AorV = 2;
	else
		tmpstat->AorV = 1;  

	num_of_roistats++;

	if((tmpstat->next = (Stat *)calloc(1,sizeof(Stat)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"calloc error for RoiStats");
	else
	{
		tmpstat->next->roi_num=0;
		tmpstat->next->volume = 0;
		tmpstat->next->slice = 0;
		strcpy(tmpstat->next->roi_type,tmpstat->roi_type);
		tmpstat->next->mean_density=0.0;
		tmpstat->next->max_density=0.0;
		tmpstat->next->min_density=0.0;
		tmpstat->next->stdev_density=0.0;
		tmpstat->next->mean_gradient=0.0;
		tmpstat->next->max_gradient=0.0;
		tmpstat->next->min_gradient=0.0;
		tmpstat->next->stdev_gradient=0.0;
		tmpstat->next->area=0.0;
		tmpstat->next->AorV=1;
		tmpstat->next->next=NULL;
	}

	//m_bROIStatisticalDone = true;
	return 1;
}

int ROIStatisticalCanvas::GetPrevRoistats()
{
	//	FILE     *fp;

	num_of_roistats=0;

	if((RoiStats = (Stat *)calloc(1,sizeof(Stat)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"memory allocation error for RoiStats");
	else{
		RoiStats->roi_num=0;
		RoiStats->volume=0;
		RoiStats->slice=0;
		strcpy(RoiStats->roi_type,ROINameTable[0]+5);
		RoiStats->mean_density=0.0;
		RoiStats->max_density=0.0;
		RoiStats->min_density=0.0;
		RoiStats->stdev_density=0.0;
		RoiStats->mean_gradient=0.0;
		RoiStats->max_gradient=0.0;
		RoiStats->min_gradient=0.0;
		RoiStats->stdev_gradient=0.0;
		RoiStats->area=0.0;
		RoiStats->AorV=1;
		RoiStats->next=NULL;
		RoiStats->pVertices = NULL;
		RoiStats->nVertices = 0;
	}

	if((TotStats = (Total *)calloc(1,sizeof(Total)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"memory allocation error for TotStats");
	else{
		TotStats->volume = 0;
		TotStats->min_density = 10000;
		TotStats->max_density = 0;
		TotStats->mean_density = 0;
		TotStats->stdev_density = 0;
		TotStats->min_gradient = 10000;
		TotStats->max_gradient = 0;
		TotStats->mean_gradient = 0;
		TotStats->stdev_gradient = 0;
	}


	/* if((fp=fopen(OutStats,"r"))!=NULL){
	tmp_int=0;
	while((tmp_int<2)&&(!feof(fp)))
	if( fgetc(fp) == '\n') tmp_int++;

	tmpstat = RoiStats;

	fscanf(fp,"%d",&num_of_roistats);	

	for (i=0; i < num_of_roistats; i++){
	if((tmp_int=fscanf(fp,"%d%d%d%s%lf%lf%lf%lf%lf%lf%lf%lf%lf%d",&(tmpstat->roi_num),&(tmpstat->volume),&(tmpstat->slice),(tmpstat->roi_type),&(tmpstat->min_density),&(tmpstat->max_density),&(tmpstat->mean_density),&(tmpstat->stdev_density),&(tmpstat->min_gradient),&(tmpstat->max_gradient),&(tmpstat->mean_gradient),&(tmpstat->stdev_gradient),&(tmpstat->area),&(tmpstat->AorV)))==14){
	if((tmpstat->next = (Stat *)calloc(1,sizeof(Stat)))==NULL)
	handle_error(process,"memory allocation error for Previous RoiStats");
	else
	tmpstat->next->next=NULL;
	tmpstat=tmpstat->next;
	}
	else
	VDisplayDialogMessage("Cannot read the preview statistics file"); 
	}           
	free(tmpstat->next);
	fclose(fp);
	}*/
	//  m_bROIStatisticalDone = true;
	return 1;
}

void ROIStatisticalCanvas::GetStandardRoi(const wxPoint  pos)
{
	int   x,y;	

	x = pos.x;	y = pos.y;

	if((x<=icon->dx1)&&(y<=icon->dy1) && (x>icon->x)&&(y>icon->y) )	
	{
		MagImg *cur_img = icon;

		if((vertices = (wxPoint *)calloc(4,sizeof(wxPoint)))==NULL)
			return; // ERR_OUTOFMEMORY; //handle_error(process,"error in calloc for vertices",0,-1);

		nvertices = 4;  

		vertices[0].x = (int)(x-RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);
		vertices[0].y = (int)(y-RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);

		vertices[1].x = (int)(x + RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);
		vertices[1].y = (int)(y -RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);

		vertices[2].x = (int)(x + RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);
		vertices[2].y = (int)(y + RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);

		vertices[3].x = (int)(x -RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);
		vertices[3].y = (int)(y + RoiSize*(cur_img->width*1.0/cur_img->img_dim[0])/2);

		ComputeStats();	
	}	
}


void ROIStatisticalCanvas::DisplayRoiStatistics()
{
	
	int         w, h;
	GetSize( &w, &h );

	DisplayStats(w/2+20,40);	

}


void ROIStatisticalCanvas::DisplayStats(int x, int y)
{
	Stat *tmpstat;	
	int  tmp_int;
	char msg[20];
	int  tmp_pixels,tmp_acc_pixels;
	double   pixelsize,
		zsize,cf;
	
	tmpstat = RoiStats;
	
	wxPoint ListPos(x, y);
	if (mListCtrl)
		delete mListCtrl;
	mListCtrl = new wxListCtrl(this, -1, ListPos, wxSize(600, 360), wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);

	wxListItem t_list_item;
	mListCtrl->InsertColumn(0, wxString("No."), wxLIST_FORMAT_LEFT, 60 );
	t_list_item.m_format = wxLIST_FORMAT_LEFT;
	t_list_item.m_mask = wxLIST_MASK_FORMAT;
	mListCtrl->SetColumn(0, t_list_item);
	mListCtrl->InsertColumn(1, wxString("Slice#"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(1, t_list_item);
	mListCtrl->InsertColumn(2, wxString("RoiType"), wxLIST_FORMAT_LEFT, 80 );
	mListCtrl->SetColumn(2, t_list_item);
	mListCtrl->InsertColumn(3, wxString("Density Min"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(3, t_list_item);
	mListCtrl->InsertColumn(4, wxString("Density Max"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(4, t_list_item);
	mListCtrl->InsertColumn(5, wxString("Density Mean"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(5, t_list_item);
	mListCtrl->InsertColumn(6, wxString("Density StdDev"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(6, t_list_item);
	mListCtrl->InsertColumn(7, wxString("Gradient Min"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(7, t_list_item);
	mListCtrl->InsertColumn(8, wxString("Gradient Max"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(8, t_list_item);
	mListCtrl->InsertColumn(9, wxString("Gradient Mean"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(9, t_list_item);
	mListCtrl->InsertColumn(10, wxString("Gradient StdDev"), wxLIST_FORMAT_LEFT, 60 );
	mListCtrl->SetColumn(10, t_list_item);
	mListCtrl->InsertColumn(11, wxString("Area/Vol sq/cu"), wxLIST_FORMAT_LEFT, 80 );
	mListCtrl->SetColumn(11, t_list_item);
	
	/* For each SERIES, print the relevant information */
	int j=0;
	for( j=0; j<num_of_roistats && (tmpstat->next !=NULL); j++)
	{
		mListCtrl->InsertItem( j, wxString::Format("%d",j) );
		mListCtrl->SetItem(j, 1, wxString::Format("(%2d,%3d)", tmpstat->volume+1,tmpstat->slice+1));
		mListCtrl->SetItem(j, 2, wxString::Format("%s", tmpstat->roi_type));
		mListCtrl->SetItem(j, 3, wxString::Format("%5.0lf", tmpstat->min_density));
		mListCtrl->SetItem(j, 4, wxString::Format("%5.0lf", tmpstat->max_density));
		mListCtrl->SetItem(j, 5, wxString::Format("%7.1lf", tmpstat->mean_density));
		mListCtrl->SetItem(j, 6, wxString::Format("%8.2lf", tmpstat->stdev_density));
		mListCtrl->SetItem(j, 7, wxString::Format("%5.0lf", tmpstat->min_gradient));
		mListCtrl->SetItem(j, 8, wxString::Format("%5.0lf", tmpstat->max_gradient));
		mListCtrl->SetItem(j, 9, wxString::Format("%7.1lf", tmpstat->mean_gradient));
		mListCtrl->SetItem(j, 10, wxString::Format("%8.2lf", tmpstat->stdev_gradient));
		mListCtrl->SetItem(j, 11, wxString::Format("%12.2lf", tmpstat->area));
		
		tmpstat=tmpstat->next;
	}

	/* Compute pixel area and the correction factor from z to x units scale */

	cf = (GetScaleUnits(msg,mCavassData->m_vh.scn.measurement_unit[0]) / 
		GetScaleUnits(msg,mCavassData->m_vh.scn.measurement_unit[2]));

	pixelsize = mCavassData->m_vh.scn.xypixsz[0] * mCavassData->m_vh.scn.xypixsz[1];

	/* Initialize variables to comupte total */

	tmp_int = 1;
	tmpstat = RoiStats;

	tmp_acc_pixels = 0;

	TotStats->volume = 0;
	TotStats->min_density = 10000;
	TotStats->max_density = 0;
	TotStats->mean_density = 0;
	TotStats->stdev_density = 0;
	TotStats->min_gradient = 10000;
	TotStats->max_gradient = 0;
	TotStats->mean_gradient = 0;
	TotStats->stdev_gradient = 0;


	/* Compute total */

	while((tmp_int<=num_of_roistats) && (tmpstat->next !=NULL))
	{

		/* zsize = medium distance between the adjacents slices if slices# > 1 
		otherwise (slices# == 1) zsize = the slice thickness */   

		/*	if(slices.slices[tmpstat->volume] > 1){
		if(tmpstat->slice == slices.slices[tmpstat->volume])
		zsize = ((slices.location3[0][tmpstat->slice] - slices.location3[0][tmpstat->slice - 1]) * cf);
		else
		if(tmpstat->slice == 0)
		zsize = ((slices.location3[0][tmpstat->slice + 1] - slices.location3[0][tmpstat->slice]) * cf);
		else
		zsize = (((slices.location3[0][tmpstat->slice] - slices.location3[0][tmpstat->slice - 1])/2. +
		(slices.location3[0][tmpstat->slice + 1] - slices.location3[0][tmpstat->slice])/2.) 
		* cf );
		}
		else*/
		zsize = mCavassData->m_vh.gen.slice_thickness; 

		if (tmpstat->AorV == 1)
		{
			tmp_pixels = (int)(tmpstat->area / pixelsize);
			tmp_acc_pixels = tmp_acc_pixels + tmp_pixels;
			TotStats->volume = TotStats->volume + (tmpstat->area * zsize);
		}
		else
		{          /* Then AorV = 2 and in this case the variable tmpstat->area keep a volume value */

			tmp_pixels = (int)(tmpstat->area / (pixelsize * zsize));
			tmp_acc_pixels = tmp_acc_pixels + tmp_pixels;
			TotStats->volume = TotStats->volume + tmpstat->area;
		}

		if (tmpstat->min_density < TotStats->min_density)
			TotStats->min_density = tmpstat->min_density;
		if (tmpstat->max_density > TotStats->max_density)
			TotStats->max_density = tmpstat->max_density;
		TotStats->mean_density = TotStats->mean_density + (tmp_pixels * tmpstat->mean_density);
		TotStats->stdev_density = TotStats->stdev_density + (tmp_pixels * ((tmpstat->stdev_density * tmpstat->stdev_density) + 
			(tmpstat->mean_density * tmpstat->mean_density)));  

		if (tmpstat->min_gradient < TotStats->min_gradient)
			TotStats->min_gradient = tmpstat->min_gradient;
		if (tmpstat->max_gradient > TotStats->max_gradient)
			TotStats->max_gradient = tmpstat->max_gradient;
		TotStats->mean_gradient = TotStats->mean_gradient + (tmp_pixels * tmpstat->mean_gradient);
		TotStats->stdev_gradient = TotStats->stdev_gradient + (tmp_pixels * ((tmpstat->stdev_gradient * tmpstat->stdev_gradient) + 
			(tmpstat->mean_gradient * tmpstat->mean_gradient)));  
		tmp_int++;
		tmpstat=tmpstat->next;
	}

	TotStats->mean_density = TotStats->mean_density / tmp_acc_pixels;
	TotStats->stdev_density = sqrt((TotStats->stdev_density / tmp_acc_pixels) - (TotStats->mean_density * TotStats->mean_density)); 
	TotStats->mean_gradient = TotStats->mean_gradient / tmp_acc_pixels;
	TotStats->stdev_gradient = sqrt((TotStats->stdev_gradient / tmp_acc_pixels) - (TotStats->mean_gradient * TotStats->mean_gradient)); 

	/* Display the total */

	mListCtrl->InsertItem( j, wxString::Format("Total") );
	//mListCtrl->SetItem(j, 1, wxString::Format("(%2d,%3d)", tmpstat->volume+1,tmpstat->slice+1));
	//mListCtrl->SetItem(j, 2, wxString::Format("%s", tmpstat->roi_type));
	mListCtrl->SetItem(j, 3, wxString::Format("%5.0lf", TotStats->min_density));
	mListCtrl->SetItem(j, 4, wxString::Format("%5.0lf", TotStats->max_density));
	mListCtrl->SetItem(j, 5, wxString::Format("%7.1lf", TotStats->mean_density));
	mListCtrl->SetItem(j, 6, wxString::Format("%8.2lf", TotStats->stdev_density));
	mListCtrl->SetItem(j, 7, wxString::Format("%5.0lf", TotStats->min_gradient));
	mListCtrl->SetItem(j, 8, wxString::Format("%5.0lf", TotStats->max_gradient));
	mListCtrl->SetItem(j, 9, wxString::Format("%7.1lf", TotStats->mean_gradient));
	mListCtrl->SetItem(j, 10, wxString::Format("%8.2lf", TotStats->stdev_gradient));
	mListCtrl->SetItem(j, 11, wxString::Format("%12.2lf", TotStats->volume));

	
	m_bROIStatisticalDone = true;

}

/************************************************************************
*    Function Name :                                                    *
*    Description   : None.                                              *
*    Return Value  : None.                                              *
*    Parameters    : None.                                              *
*    Side Effects  : None.                                              *
*    Entry Cond.   : None.                                              *
*    Related Func. : None.                                              *
*    History       : None.                                              *
************************************************************************/

void ROIStatisticalCanvas::SaveStats(unsigned char *OutStats )
{
	Stat *tmpstat;
	int  tmp_int;
	char scale[12];
	FILE *ofp_stats;

	tmpstat = RoiStats;
	tmp_int=1;   

	if((ofp_stats=fopen((const char*)OutStats,"w"))==NULL)
	{
		wxMessageBox("Can't open file to save statistics");		
	}
	else
	{
		fprintf(ofp_stats,"%s\n",LINE1);
		GetScaleUnits(scale,mCavassData->m_vh.scn.measurement_unit[0]);
		fprintf(ofp_stats,"%s(%s)\n",LINE2,scale);

		fprintf(ofp_stats,"%d\n",num_of_roistats);
		while((tmp_int<=num_of_roistats) && (tmpstat->next !=NULL)){
			fprintf(ofp_stats,"%2d  %2d  %3d %5s %5.0lf %5.0lf %7.1lf %8.2lf %5.0lf %5.0lf %7.1lf %8.2lf %12.2lf %1d\n",tmp_int,tmpstat->volume+1,tmpstat->slice+1,tmpstat->roi_type,tmpstat->min_density,tmpstat->max_density,tmpstat->mean_density,tmpstat->stdev_density,tmpstat->min_gradient,tmpstat->max_gradient,tmpstat->mean_gradient,tmpstat->stdev_gradient,tmpstat->area,tmpstat->AorV);

			tmp_int++;
			tmpstat=tmpstat->next;
		}
		/* Write the total in the RoiStats file */

		fprintf(ofp_stats,"-------------------------------------------------------------------------------------------------------------------\n");
		fprintf(ofp_stats,"Total             %5.0lf %5.0lf %7.1lf %8.2lf %5.0lf %5.0lf %7.1lf %8.2lf %12.2lf\n",TotStats->min_density,TotStats->max_density,TotStats->mean_density,TotStats->stdev_density,TotStats->min_gradient,TotStats->max_gradient,TotStats->mean_gradient,TotStats->stdev_gradient,TotStats->volume);

		//VDisplayDialogMessage(" ");
		fclose(ofp_stats);
	}   
}


int ROIStatisticalCanvas::DrawRoiVertices()
{	

	return 1;
}
