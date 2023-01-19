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
 * \file:  DensityCanvas.cpp
 * \brief  DensityCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"

int DensityCanvas::DrawAllLine()
{
	int cur_scn = 0;
	struct Z_POINT * zPoint = NULL;

	zPoint = &icon_zpoint[cur_scn];
	while(zPoint != NULL)
	{
		UpdateIconPoint(zPoint);
		if( zPoint->next != NULL )
		{
			UpdateIconLine(zPoint, zPoint->next);		
		}
		zPoint=zPoint->next;
	}

	return 1;

}

int DensityCanvas::DrawImgLine(const wxPoint  pos)
{   
	MagImg  *pt_icon;
	int   i,j;
	int   x1,y1;
	int found=1;
	int cur_scn, cur_slc;

	int dimension = mCavassData->m_vh.scn.dimension;
	x1= pos.x;
	y1= pos.y;	
	icon->slice_index[2] = mCavassData->m_sliceNo;

	//for(i=0,pt_roi=magimg;i<num_mag_images;i++,pt_roi=pt_roi->next)
	//{
	//	if(eve.xany.window==pt_roi->win)
	//	{
	//		cur_scn=pt_roi->scene_num;
	//		cur_slc=pt_roi->slice_index[2];
	//		if(dimension > 3){
	//			cur_vol=pt_roi->slice_index[3]; 
	//			if((first_point[cur_vol] == 0) && (very_first_point == 0))
	//				found = 0;
	//		}
	//		else
	//			if(first_point[cur_scn]==0)
	//				found=0;

	//		/* this i am intializing to 3 so that*/
	//		/* all the points are in the same 4th dimension */

	//		if((DISPLAY_MODE == 0) && (dimension > 3)
	//			&& (first_point[cur_vol] != 1)  && (very_first_point != 0)){
	//				XBell(display,0);
	//				VDisplayDialogMessage("select points in the same volume");
	//				XFlush(display);
	//				return(1);
	//		}
	//		if((DISPLAY_MODE == 1) && (dimension > 3)
	//			&& (first_point[cur_vol] == 1)  && (very_first_point != 0)){
	//				XBell(display,0);
	//				VDisplayDialogMessage("select points in different volumes");
	//				XFlush(display);
	//				return(1);
	//		}

	//		if((DISPLAY_MODE == 0)&&(dimension <= 3)){
	//			j=2;
	//			while((j<dimension)&&(found==1)){
	//				if(pt_roi->slice_index[j] != prev_point[cur_scn].index[j]) found=0;
	//				j++;
	//			}
	//		}

	//		if((DISPLAY_MODE == 0)&&(dimension > 3)){
	//			j=2;
	//			while((j<dimension)&&(found==1)){
	//				if(pt_roi->slice_index[j] != prev_point[cur_scn].index[j]) found=0;
	//				j++;
	//			}
	//		}

	//		point = &icon_zpoint[cur_scn];
	//		while(point->next!=NULL)point=point->next;
	//		if((point->next =(struct Z_POINT *) calloc(1,sizeof(struct Z_POINT)))==NULL)
	//			handle_error(process,"error in calloc",0,-1);
	//		else
	//			point->next->next=NULL;
	//		if((point->next->index = (short *)calloc(1,dimension * sizeof(short)))==NULL)
	//			handle_error(process,"error in calloc",0,-1);

	//		for(j=0;j<dimension;j++){
	//			point->index[j]=pt_roi->slice_index[j];
	//		}
	//		point->num=cur_scn;


	//		point->x = pt_roi->img_offset[0]+(int)rint((double)((x1+pt_roi->dx-pt_roi->x)*pt_roi->img_dim[0]*1.0/(pt_roi->width*1.0)));
	//		point->y = pt_roi->img_offset[1]+(int)rint((double)((y1+pt_roi->dy-pt_roi->y)*pt_roi->img_dim[1]*1.0/(pt_roi->height*1.0)));

	//		if((found==1)&&(DISPLAY_MODE==0)){
	//			UpdateRoiLine(&prev_point[cur_scn],point);
	//			UpdateIconLine(&prev_point[cur_scn],point);
	//		}
	//		else{
	//			if(dimension <= 3)
	//				first_point[cur_scn]=1;
	//			else{
	//				first_point[cur_vol]=1;
	//				very_first_point =1;
	//			}
	//			UpdateRoiPoint(point);
	//			UpdateIconPoint(point);
	//		}
	//		no_of_icon_points+=1;
	//		prev_point[cur_scn].x=point->x;
	//		prev_point[cur_scn].y=point->y;
	//		for(j=0;j<dimension;j++)
	//			prev_point[cur_scn].index[j]=point->index[j];
	//		point->num=cur_scn;
	//		if(FIRST_POINT){
	//			SCENE_NUM    = cur_scn;
	//			VOLUME_NUM[0]   = cur_vol;
	//			SLICE_NUM[0] = cur_slc;
	//			FIRST_POINT  = 0;
	//		}
	//		else{
	//			SLICE_NUM[1] = cur_slc;
	//			VOLUME_NUM[1] = cur_vol;
	//			FIRST_POINT  = 1;
	//		}
	//		return(cur_scn);
	//	}
	//}

	for(i=0,pt_icon=icon;i<num_of_scenes;i++,pt_icon=pt_icon->next)
	{
		if( pos.x>pt_icon->x && pos.x<pt_icon->dx1 && pos.y>pt_icon->y && pos.y<pt_icon->dy1 )
		{
			cur_vol = 0;
			cur_scn = 0; //pt_icon->scene_num;  // scene index
			cur_slc = mCavassData->m_sliceNo; //pt_icon->slice_index[2];
			if(dimension > 3)  ///////// currently don't support,  only support 3D
			{
				cur_vol=pt_icon->slice_index[3];
				if((first_point[cur_vol] == 0) && (very_first_point == 0))
					found = 0;
			}
			else
				if(first_point[cur_scn]==0)
					found=0;

			if((DISPLAY_MODE == 0) && (dimension > 3)
				&& (first_point[cur_vol] != 1) && (very_first_point != 0)){
				/*	XBell(display,0);
					VDisplayDialogMessage("select points in the same volume");
					XFlush(display);*/
					return(1);
			}

			if((DISPLAY_MODE == 1) && (dimension > 3) && 
				(first_point[cur_vol] == 1) && (very_first_point == 1)){
				/*	XBell(display,0);
					VDisplayDialogMessage("select points in different volumes");
					XFlush(display);*/
					return(1);
			}

			if((DISPLAY_MODE == 0)&&(dimension <= 3))
			{
				j=2;
				while((j<dimension)&&(found==1))
				{
					if(pt_icon->slice_index[j] != prev_point[cur_scn].index[j]) 
						found=0;
					j++;
				}
			}

			if((DISPLAY_MODE == 0)&&(dimension > 3))
			{
				j=2;
				while((j<dimension)&&(found==1)){
					if(pt_icon->slice_index[j] != prev_point[cur_scn].index[j]) found=0;
					j++;
				}
			}

			point = &icon_zpoint[cur_scn];
			while(point->next!=NULL)point=point->next;

			if((point->next = (struct Z_POINT *)calloc(1,sizeof(struct Z_POINT)))==NULL)
				return -1; //handle_error(process,"error in calloc",0,-1);
			else
				point->next->next=NULL;

			if((point->next->index =(short *) calloc(1,dimension * sizeof(short)))==NULL)
				return -1; //handle_error(process,"error in calloc",0,-1);
			for(j=0;j<dimension;j++){
				point->index[j]=pt_icon->slice_index[j];
			}
			point->num=cur_scn;

			point->x = pt_icon->img_offset[0]+(int)rint((double)((x1-pt_icon->x)*pt_icon->img_dim[0]*1.0/(pt_icon->width*1.0))); //+pt_icon->dx
			point->y = pt_icon->img_offset[1]+(int)rint((double)((y1-pt_icon->y)*pt_icon->img_dim[1]*1.0/(pt_icon->height*1.0)));  //+pt_icon->dy

			if((found==0)&&(DISPLAY_MODE==0))
			{
				if(dimension <= 3)
					first_point[cur_scn]=1;
				else
				{
					first_point[cur_vol]=1;
					very_first_point = 1;
				}
				UpdateIconPoint(point);
		//		UpdateRoiPoint(point);
			}
			else
			{
				if((found == 1) && (DISPLAY_MODE == 0))
				{
					UpdateIconLine(&prev_point[cur_scn],point);
			//		UpdateRoiLine(&prev_point[cur_scn],point);
				}
			}

			if(DISPLAY_MODE == 1)
			{
				first_point[cur_vol]=1;
				very_first_point = 1;
				UpdateIconPoint(point);
				//		UpdateRoiPoint(point);
			}
			no_of_icon_points+=1;
			prev_point[cur_scn].x = point->x;
			prev_point[cur_scn].y = point->y;
			for(j=0;j<dimension;j++)
				prev_point[cur_scn].index[j]=point->index[j];
			point->num=cur_scn;
			if(FIRST_POINT)
			{
				SCENE_NUM    = cur_scn;
				VOLUME_NUM[0]   = cur_vol;
				SLICE_NUM[0] = cur_slc;
				FIRST_POINT  = 0;
			}
			else
			{
				SLICE_NUM[1] = cur_slc;
				VOLUME_NUM[1]   = cur_vol;
				FIRST_POINT  = 1;
			}
			return(cur_scn);
		}
	}
	return(-1);
}


/************************************************************************
*    Function Name : UpdateIconPoint
*    Description   : Display a point inside the icon window. 
*    Return Value  : None.
*    Parameters    : cur_point = a pointer to the (x,y) coordinates of 
*                                the chosen point. 
*    Side Effects  : None.                        
*    Entry Cond.   : You should be running CheckImageWindowEvent().
*    Related Func. : CheckImageWindowEvent(), CheckWindowPointIndex() 
*    History       : Created by Srnivas and modified by Alexandre 
*                    Falcao in September, 1993.
************************************************************************/

void DensityCanvas::UpdateIconPoint(struct Z_POINT *cur_point)
{
	MagImg *icon_ptr;
	int i, draw_x,draw_y;
	wxClientDC  dc(this);
	PrepareDC(dc);
	dc.SetPen( wxPen(wxColour(Yellow)) );

	for(i=0,icon_ptr=icon;i<num_of_scenes;i++,icon_ptr=icon_ptr->next)
	{
		if(CheckWindowPointIndex(cur_point,icon_ptr)!=0)
		{
			draw_x = (int)rint((double)((cur_point->x - icon_ptr->img_offset[0])*((double)(1.0*icon_ptr->width)/((double)(1.0*icon_ptr->img_dim[0])))+icon_ptr->x)); //-icon_ptr->dx
			draw_y = (int)rint((double)((cur_point->y - icon_ptr->img_offset[1])*((double)(1.0*icon_ptr->height)/((double)(1.0*icon_ptr->img_dim[1])))+icon_ptr->y)); //-icon_ptr->dy

		//	if((draw_x<=icon_ptr->width)&&(draw_y<=icon_ptr->height))
			if((draw_x<=icon_ptr->dx1)&&(draw_y<=icon_ptr->dy1) && (draw_x>icon_ptr->x)&&(draw_y>icon_ptr->y)  )
			{
				dc.DrawPoint(draw_x,draw_y);
				//VDisplayOverlayLine(icon_ptr->win,ovl,draw_x,draw_y,draw_x,draw_y);
			}
		}
	}
}

/************************************************************************
*    Function Name : UpdateRoiLine
*    Description   : Display a line inside the chosen roi window. 
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

//UpdateRoiLine(cur_point,cur_point1)
//struct Z_POINT *cur_point,*cur_point1;
//{
//	MagImg *roi_ptr;
//	int i,j,
//		draw_x,draw_y,
//		draw_x1,draw_y1;
//
//	for(i=0,roi_ptr=magimg;i<num_mag_images;i++,roi_ptr=roi_ptr->next){
//		if(CheckWindowPointIndex(cur_point,roi_ptr)!=0){
//			draw_x = (int)rint((double)((cur_point->x - roi_ptr->img_offset[0])*((double)(1.0*roi_ptr->width)/((double)(1.0*roi_ptr->img_dim[0])))+roi_ptr->x-roi_ptr->dx));
//			draw_y = (int)rint((double)((cur_point->y - roi_ptr->img_offset[1])*((double)(1.0*roi_ptr->height)/((double)(1.0*roi_ptr->img_dim[1])))+roi_ptr->y-roi_ptr->dy));
//
//			draw_x1 = (int)rint((double)((cur_point1->x - roi_ptr->img_offset[0])*((double)(1.0*roi_ptr->width)/((double)(1.0*roi_ptr->img_dim[0])))+roi_ptr->x-roi_ptr->dx));
//			draw_y1 = (int)rint((double)((cur_point1->y - roi_ptr->img_offset[1])*((double)(1.0*roi_ptr->height)/((double)(1.0*roi_ptr->img_dim[1])))+roi_ptr->y-roi_ptr->dy));
//
//			if((draw_x<=roi_ptr->width)&&(draw_y<=roi_ptr->height))
//				if((draw_x1<=roi_ptr->width)&&(draw_y1<=roi_ptr->height))
//					VDisplayOverlayLine(roi_ptr->win,ovl,draw_x,draw_y,draw_x1,draw_y1);         
//
//		}
//	}
//}


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

void DensityCanvas::UpdateIconLine(struct Z_POINT *cur_point, struct Z_POINT *cur_point1)
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

int DensityCanvas::CheckWindowPointIndex(struct Z_POINT *c_point, MagImg * c_win)
{
	int i;
	for(i=2;i<3;i++) //scene[c_win->scene_num].vh.scn.dimension
	{
		if(c_point->index[i] != c_win->slice_index[i])
			return(0);
	}
	return(1);
}


/******************************************************************************
*    Function Name : FindDensityValues
*    Description   : Build the graphic of the profile drawn, where the X 
*                    axis is the measure of spacing between each point and 
*                    the first point of the profile, and the Y axis is the 
*                    density value of each point of the profile. 
*                    The spacing between the profile points is measured in 
*                    distance units, if mode = static, otherwise the 
*                    spacing is measured in time units.
*                    
*    Return Value  : None.
*    Parameters    : None.    
*    Side Effects  : None. 
*    Entry Cond.   : You should draw the profile first. 
*    Related Func. : FindDistance(), CheckPointsIndex(), handle_error(),
*                    GetValue(), GetLocation(), GetTimeLocation(), 
*                    DisplayProfile(),CreateDisplayArea(),SortGraphPoints()
*    History       : Created by Srnivas and modified by Alexandre Falcao 
*                    in September, 1993.                          
*****************************************************************************/
int DensityCanvas::FindDensityValues()
{
	int k;
	unsigned int i;
	struct Z_POINT point1, *temp_p,*graph_point=NULL;
	struct G_POINT *g_point, *prev_g_point, *gapp, *g_pass, *gapp1; /* linked list for storing the graph points */
	wxPoint *xpoint;
	int npoints,diff;
	int x0,x1,y0,y1;
	float z0,z1,temp_z,prev_z,temp_time;
	int increment=1;

	plot_points=0;
	point=icon_zpoint;
	minValue = 100000;
	maxValue = -100000;  
	int min_dist_flag = 0;
	min_dist = 0;


	/* allocate memory for the first node of the graph points */

	if((g_point = (gpoint *)calloc(1,sizeof(gpoint)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
	else
		g_point->next = NULL;
	/* allocate memory for the index values in the temporary point */

	if((temp_p = (zpoint *)calloc(1,sizeof(zpoint)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
	else
		temp_p->next = NULL;

	if((temp_p->index = (short *)calloc(mCavassData->m_vh.scn.dimension+1,sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);

	if((point1.index = (short *)calloc(mCavassData->m_vh.scn.dimension+1,sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY;  //handle_error(process,"error in calloc",0,-1);

	/* prev_g_point is a following pointer to keep track of the previous point */
	prev_g_point = g_point;

	g_pass = g_point;
	gapp1 = g_point;
	tot_dist = 0.0;
	if(DISPLAY_MODE == 0)
	{
		while(point->next != NULL)
		{
			if(plot_points==0)
			{
				plot_points += 1;
				g_point->x = point->x;
				g_point->y = point->y;
				g_point->num = point->num;
				prev_z = 0;
				tot_dist += FindDistance(prev_g_point,g_point,prev_z);
				g_point->distance = (int)rint(tot_dist);
				g_point->value = GetValue(point);
				if(g_point->value < minValue) 
					minValue = (int) g_point->value;
				if(g_point->value > maxValue) 
					maxValue = (int) g_point->value;
				if((g_point->next = (gpoint *)calloc(1,sizeof(gpoint)))==NULL)
					return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
				else
					g_point->next->next = NULL;
				g_point = g_point->next;
			}
			else
			{
				if(CheckPointsIndex(graph_point,point)==1)
				{
					VComputeLine(graph_point->x,graph_point->y,point->x,point->y,&xpoint,&npoints);
					for(i=2;i<(unsigned int)mCavassData->m_vh.scn.dimension;i++)
						point1.index[i] = point->index[i];

					point1.num = point->num;
					for(k=0;k<npoints;k++)
					{
						plot_points += 1;
						g_point->num = point->num;
						g_point->x = xpoint[k].x;
						point1.x = xpoint[k].x;
						g_point->y = xpoint[k].y;
						point1.y = xpoint[k].y;
						g_point->value = GetValue(&point1); //1

						prev_z = 0;
						tot_dist += FindDistance(prev_g_point,g_point,prev_z);
						g_point->distance = (int)rint(tot_dist) ;
						if(g_point->value < minValue) 
							minValue = (int) g_point->value;
						if(g_point->value > maxValue)
							maxValue = (int) g_point->value;
						if((g_point->next = (gpoint *)calloc(1,sizeof(gpoint)))==NULL)
							return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
						else
							g_point->next->next = NULL;
						prev_g_point = prev_g_point->next;
						g_point = g_point->next;
					}
					free(xpoint);
				}
				else
				{
					x0 = graph_point->x; x1 = point->x;
					y0 = graph_point->y; y1 = point->y;
					z0 = graph_point->index[2]; //GetLocation(graph_point); 
					z1 = point->index[2]; //GetLocation(point);

					/* initialize the prev_z, the z value of the prev pixel */
					prev_z = z0;

					/* calculating the difference from the 3rd dimension */
					/* ==> index = 2, will have to change it for other interpretations */

					diff = graph_point->index[2] - point->index[2];

					if(diff>0)
						increment = -1;
					else{
						diff = -diff;
						increment=1;
					}


					for(i=0; i< (unsigned int)mCavassData->m_vh.scn.dimension;i++)
						temp_p->index[i] = graph_point->index[i];
					temp_p->num = graph_point->num;

					for(k=1;k<diff;k++)
					{
						temp_p->index[2] += increment;
						temp_z = temp_p->index[2]; //GetLocation(temp_p);
						temp_p->x = (int)rint((double)((x0 - x1)*(temp_z - z0)/(z0 - z1) + x0));
						temp_p->y = (int)rint((double)((y0 - y1)*(temp_z - z0)/(z0 - z1) + y0));
						plot_points += 1;
						g_point->num = point->num; 
						g_point->x = temp_p->x;
						g_point->y = temp_p->y;
						g_point->value = GetValue(temp_p);
						tot_dist += FindDistance(prev_g_point,g_point,(temp_z-prev_z));
						g_point->distance = (int)rint(tot_dist);
						prev_z = temp_z;
						if(g_point->value < minValue) minValue = (int) g_point->value;
						if(g_point->value > maxValue) maxValue = (int) g_point->value;
						if((g_point->next = (gpoint *)calloc(1,sizeof(gpoint)))==NULL)
							return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
						else
							g_point->next->next = NULL;
						prev_g_point = prev_g_point->next;
						g_point = g_point->next;
					}
					g_point->x = point->x;
					g_point->y = point->y;
					g_point->num = point->num;
					g_point->value = GetValue(point);
					plot_points += 1;
					tot_dist += FindDistance(prev_g_point,g_point,(z1-prev_z));
					g_point->distance = (int)rint(tot_dist);
					if(g_point->value < minValue)
						minValue = (int) g_point->value;
					if(g_point->value > maxValue) 
						maxValue = (int) g_point->value;
					if((g_point->next = (gpoint *)calloc(1,sizeof(gpoint)))==NULL)
						return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
					else
						g_point->next->next = NULL;
					prev_g_point = prev_g_point->next;
					g_point = g_point->next;
				}
			}
			graph_point = point;
			point = point->next;
		}
	}

	if(DISPLAY_MODE == 1)
	{
		while(point->next != NULL)
		{
			plot_points += 1;
			temp_p->x = g_point->x = point->x;
			temp_p->y = g_point->y = point->y;
			temp_p->num = g_point->num = point->num;
			for(i=0;i<(unsigned int)mCavassData->m_vh.scn.dimension;i++)
				temp_p->index[i]=point->index[i];
			temp_p->index[2] = -1;
			temp_time = temp_p->index[2]; //GetTimeLocation(temp_p);
			if(tot_dist < rint(temp_time))
				tot_dist = rint(temp_time);
			if(min_dist_flag==0)
			{
				min_dist_flag=1;
				min_dist=tot_dist;
			}
			g_point->distance = (int)temp_time;
			g_point->value = GetValue(point);
			if(g_point->value < minValue) 
				minValue = (int) g_point->value;
			if(g_point->value > maxValue) 
				maxValue = (int) g_point->value;
			if((g_point->next = (gpoint *)calloc(1,sizeof(gpoint)))==NULL)
				return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
			else
				g_point->next->next = NULL;
			g_point = g_point->next;
			point = point->next;
		}
	}
			
	if(DISPLAY_MODE==1)
	{
		i=0;
		if((gapp=(gpoint *)calloc(plot_points,sizeof(gpoint)))==NULL)
			return ERR_OUTOFMEMORY; //handle_error(process,"error in calloc",0,-1);
		else
			for(i=0;i<plot_points;i++)
			{
				gapp[i].x = gapp1->x;
				gapp[i].y = gapp1->y;
				gapp[i].num = gapp1->num;
				gapp[i].value = gapp1->value;
				gapp[i].distance = gapp1->distance;
				gapp[i].next = NULL;
				gapp1=gapp1->next;
			}

			SortGraphPoints(gapp);
			gapp1 = g_pass;
			for(i=0;i<plot_points;i++){
				gapp1->x = gapp[i].x;
				gapp1->y = gapp[i].y;
				gapp1->num = gapp[i].num;
				gapp1->value = gapp[i].value;
				gapp1->distance = gapp[i].distance;
				gapp1=gapp1->next;
			}
			gapp1 = g_pass;
	}
	
	if(plot_points >0)
	{			
	//	CreateDisplayArea();				
		profile = g_pass;
		DisplayProfile(g_pass);
	}	

	return 1;

}


/**************************************************************************
*    Function Name : SortGraphPoints
*    Description   : Sort the points of the profile by distance order.
*    Return Value  : None.                
*    Parameters    : gp = pointer to the profile graphic structure.                
*    Side Effects  : None.   
*    Entry Cond.   : You should be running FindDensityValues() before. 
*    Related Func. : FindDensityValues()
*    History       : Created by Srnivas and documented by Alexandre Falcao 
*                    in September 16th, 1993.
**************************************************************************/
void DensityCanvas::SortGraphPoints(struct G_POINT *gp)
{
	struct G_POINT tgp;
	unsigned int j,i=0;

	for(i=0;i<plot_points-1;i++)
		for(j=0;j<plot_points-1;j++)
			if(gp[j].distance > gp[j+1].distance)
			{
				tgp.x = gp[j].x; tgp.y = gp[j].y;
				tgp.num = gp[j].num; tgp.value = gp[j].value;
				tgp.distance = gp[j].distance;
				gp[j].x = gp[j+1].x; gp[j].y = gp[j+1].y;
				gp[j].num = gp[j+1].num; gp[j].value = gp[j+1].value;
				gp[j].distance = gp[j+1].distance;
				gp[j+1].x = tgp.x; gp[j+1].y = tgp.y;
				gp[j+1].num = tgp.num; gp[j+1].value = tgp.value;
				gp[j+1].distance = tgp.distance;
			}
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
*    Entry Cond.   : You should be running FindDensityValues() before. 
*    Related Func. : FindDensityValues()
*    History       : Created by Srnivas and documented by Alexandre Falcao 
*                    in September 16th, 1993.
************************************************************************/
int DensityCanvas::CheckPointsIndex(struct Z_POINT *c_point, struct Z_POINT *c_point1)
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
*    Entry Cond.   : You should be running FindDensityValues() before. 
*    Related Func. : FindDensityValues(),GetScaleUnits()
*    History       : Created by Srnivas and modified by Alexandre Falcao 
*                    in September, 1993.
*************************************************************************/
double DensityCanvas::FindDistance(struct G_POINT *gp1, struct G_POINT *gp2,float z_diff)
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
*    Function Name : DisplayProfile
*    Description   : This function displays the density profile graphic. 
*    Return Value  : None.
*    Parameters    : dp = a pointer to the profile structure.
*    Side Effects  : Create the image subwindow for display of the profile 
*                    and allocate memory for the global graph_data.          
*    Entry Cond.   : You should be running FindDensityValues().
*    Related Func. : FindDensityValues(), handle_error(), DisplayScale() 
*    History       : Created by Srnivas and documented by Alexandre 
*                    Falcao in September 16th, 1993.
*                    Modified: 3/25/97 parameter mismatch in
*                    VDisplayGrayImage call corrected by Dewey Odhner.
***************************************************************************/
int DensityCanvas::DisplayProfile(struct G_POINT *dp)
{
	int i;
	unsigned int count=0;
	int value_diff, x_diff, y_diff;
	double value_scale, dist_scale;
	int temp_x,temp_y,temp_x1,temp_y1;
	int npoint;
	wxPoint *xpoint;
	struct G_POINT *gdp, *pgdp; /* GraphDataPointer and PreviousGraphDataPointer */
	
	int w, h;
	wxClientDC  dc(this);	
	dc.GetSize( &w, &h );	

	value_diff = maxValue - minValue  + 1;

		/*int  w, h;
		mOverallXSize = mCavassData->m_xSize;
		GetSize( &w, &h );*/
		

	/* -1 to accomodate the window size */
	x_diff = w/2-100; //mCavassData->m_xSize; //(graph_box.x1 - graph_box.x ); 
	y_diff = h/2; //mCavassData->m_ySize; //(graph_box.y1 - graph_box.y );

	value_scale = (y_diff-1.0)/(1.0*value_diff);
	dist_scale = (x_diff-1.0)/(1.0*(ceil(tot_dist)-min_dist));

	if((graph_data = (short *)calloc(x_diff * y_diff,sizeof(short)))==NULL)
		return ERR_OUTOFMEMORY; //handle_error(process,"memory alloc error for the graph_data",0,-1);

	gdp = dp;
	pgdp = dp;

	temp_y = (int)floor((double)((gdp->value - minValue)*value_scale));
	temp_x = (int)floor((double)((gdp->distance - min_dist)*dist_scale)) ;

	/* to accomodate the correct displaying it should be (y*xwidth + x) */

	graph_data[temp_x + ((y_diff - 1 - temp_y ) * x_diff) ] = 255;  //white
	gdp = gdp->next;
	count++;

	while((gdp->next != NULL)&&(count < plot_points))
	{
		temp_x1 = (int)floor((double)((gdp->distance - min_dist) * dist_scale));
		temp_y1 = (int)floor((double)((gdp->value - minValue)*value_scale));
		VComputeLine(temp_x,temp_y,temp_x1,temp_y1,&xpoint,&npoint);
		for(i=0;i<npoint;i++)
		{
			graph_data[xpoint[i].x + ((y_diff - 1 - xpoint[i].y) * x_diff) ] = 255; //white;
		}
		free(xpoint);
		temp_x = temp_x1;
		temp_y = temp_y1;
		gdp = gdp->next;
		count++;
	}
	
	unsigned char*  slice = new unsigned char[x_diff*y_diff*3];
	assert( slice!=NULL );

	for (int i=0,j=0; i<x_diff*y_diff*3 && j<x_diff*y_diff; i+=3,j++) 
	{		
		slice[i] = slice[i+1] = slice[i+2] = (unsigned char)graph_data[j];
	}


	if(m_images[1]!=NULL)
		m_images[1]->Destroy();

	m_images[1] = new wxImage( x_diff, y_diff, slice );
	/*if (m_scale!=1.0) 
	{
		m_images[1]->Rescale( (int)(ceil(m_scale*x_diff)), (int)(ceil(m_scale*y_diff)) );
	}*/
	m_bitmaps[1] = new wxBitmap( *m_images[1] );

	m_DensityGraphBottom = m_DensityGraphTop +y_diff;

	DrawDensity();

	/*if((VCreateImageSubwindow(&graph,(graph_box.x-I_I_X),(graph_box.y-I_I_Y),x_diff,y_diff))!=0)
		handle_error(process,"couls not create window for displaying graph",0,-1);
	else{
		VDisplayGrayImage(graph,(char *)graph_data,0,0,x_diff,y_diff,16,(unsigned long)0, (unsigned long)white);
		if((VCreateImageSubwindow(&graph_under,(graph_box.x-I_I_X),(graph_box.y1-I_I_Y),x_diff,10))!=0)
			handle_error(process,"could not create window for displaying graph",0,-1);
		else
			DisplayScale();
	}*/

	m_bDensityDone = true;

	//Refresh();
	return 1;
}

/***************************************************************************
*    Function Name : GetValue
*    Description   : Get the density value for a point on the image display.
*    Return Value  : ret_value = the density value. 
*    Parameters    : gvpoint = pointer to the point.
*                    same_slice = indication to know if the current point
*                                 and the previous point are on the same
*                                 slice. 
*    Side Effects  : None.                                
*    Entry Cond.   : None.
*    Related Func. : FindDensityValues(), ComputeStats(), GetPointer()
*    History       : Created by Srnivas and documented by Alexandre Falcao 
*                    in September 16th, 1993.
*************************************************************************/
int DensityCanvas::GetValue(struct Z_POINT *gvpoint)
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
int DensityCanvas::VComputeLine ( int x1, int y1, int x2, int y2, wxPoint** points, int* npoints )
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

double DensityCanvas::GetScaleUnits(char *gsc, short g_int)
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
*    Related Func. : FreeGraphs(), FindDensityProfile()
*    History       : Created by Srnivas and documented by Alexandre 
*                    Falcao in September 15th, 1993. 
************************************************************************/

void DensityCanvas::RemoveProfileInfo()
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
*    Entry Cond.   : You should call FindDensityValues() before.
*    Related Func. : FindDensityValues(), GetScaleUnits()
*    History       : Created by Srnivas and documented by Alexandre 
*                    Falcao in September 20th, 1993.
*                    Modified: 3/25/97 parameter mismatch corrected in
*                       XWarpPointer call by Dewey Odhner.
*                    Modified: 2/3/04 correct pointer coordinates used
*                       by Dewey Odhner.
**************************************************************************/

int DensityCanvas::MeasureWidth(const wxPoint  pos)
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
	if( pos.x > m_DensityGraphLeft &&  pos.x < m_DensityGraphRight  && pos.y > m_DensityGraphTop &&  pos.y < m_DensityGraphBottom )		
	{
		if(two_lines==0)
		{
			firstMeasureX = pos.x;
			dc.DrawLine( pos.x, m_DensityGraphBottom +10, pos.x, m_DensityGraphBottom +20 );
			//XDrawLine(display,img,graph_box.gc,x,graph_box.y-I_I_Y,x,graph_box.y1+10-I_I_Y);
			two_lines=(two_lines+1)%2;	

			m_parent_frame->SetStatusText("Sel Measure Point 2.", 2);		
		}
		else if(two_lines==1)
		{
			m_parent_frame->SetStatusText("Sel Measure Point 1.", 2);		

			secondMeasureX = pos.x;

			//XDrawLine(display,img,gc,x,graph_box.y-I_I_Y,x,graph_box.y1+10-I_I_Y);			
			length = (1.0*tot_dist*abs(firstMeasureX-secondMeasureX));
			length = length / (m_scale*mCavassData->m_xSize); //(graph_box.x1 - graph_box.x - 1) ;
			GetScaleUnits(msg[1],((DISPLAY_MODE == 0) ? (mCavassData->m_vh.scn.measurement_unit[0]) : (mCavassData->m_vh.scn.measurement_unit[3])));
			sprintf(msg[0],"%5.1f%s",length,msg[1]);
			
			dc.DrawLine( firstMeasureX, m_DensityGraphBottom +10, firstMeasureX, m_DensityGraphBottom +20 );
			dc.DrawLine( pos.x, m_DensityGraphBottom +10, pos.x, m_DensityGraphBottom +20 );
			dc.DrawLine( firstMeasureX, m_DensityGraphBottom +15, secondMeasureX,m_DensityGraphBottom +15 );
			wxString s = wxString::Format( "%s", msg[0]);
			dc.DrawText( s, (firstMeasureX+secondMeasureX)/2-20, m_DensityGraphBottom +20 );

			//XDrawLine(display,img,gc,fx-I_I_X,temp_msg_y,fx-I_I_X,temp_msg_y+font_height);
			//XDrawLine(display,img,gc,sx-I_I_X,temp_msg_y,sx-I_I_X,temp_msg_y+font_height);
			//XDrawLine(display,img,gc,fx-I_I_X,temp_msg_y+font_height/2, sx-I_I_X,temp_msg_y+font_height/2);
			//VDisplayImageMessage(img,msg[0],1,(fx+sx)/2-I_I_X,temp_msg_y+font_height/2+1);			

			two_lines=(two_lines+1)%2;
		}
	}



	return 1;
}
