/*
  Copyright 1993-2016 Medical Image Processing Group
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
 * \file:  VOIIOICanvas.cpp
 * \brief  VOIIOICanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "VOIIOICanvas.h"


#define MAX_INTERVAL 4
#define MAX_OPTIMA 16
#define Left  0
#define Right 1
#define MAX_TRACK_POINTS 50

using namespace std;
//----------------------------------------------------------------------
VOIIOICanvas::VOIIOICanvas ( void )  {  puts("VOIIOICanvas()");  }
//----------------------------------------------------------------------
VOIIOICanvas::VOIIOICanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
{
    m_scale          = 1.0;
    m_overlay        = true;
	m_bLayout        = false;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 40;
	m_nROIType = 0;
    m_nGradType = 0;
	m_nSH0Type = 0;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
	m_bVOIIOIDone = false;	

	m_bMouseMove = false;


	m_nStartSlice = 1;
	m_nEndSlice = 2;
	m_nROIWidth = 101;
	m_nROIHeight = 101;
    m_nROIL = 100;
	m_nROIR = 200;
	m_nROIT = 100;
	m_nROIB = 200;

	m_bLocOrSize = false;
	m_bROI = false;

	max_hist = -1;
	min_hist = 1;
	hist_scope = 0;
	m_fHistZoomFactor = 1.0;
	histx = 400;
	histy = 100;
	histw = 400;
	histh = 300;
	BLINK = false;
	interval = 1;
	resetThresholds();
}
//----------------------------------------------------------------------
VOIIOICanvas::~VOIIOICanvas ( void ) {
    cout << "VOIIOICanvas::~VOIIOICanvas" << endl;
    wxLogMessage( "VOIIOICanvas::~VOIIOICanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
}
//----------------------------------------------------------------------
void VOIIOICanvas::release ( void ) {
}
//----------------------------------------------------------------------
void VOIIOICanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void VOIIOICanvas::loadFile ( const char* const fn ) 
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    
    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }    
	
	assert( m_sliceIn == NULL );
	m_sliceIn = new SliceData( fn );
	if (!m_sliceIn->mIsCavassFile || !m_sliceIn->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
	mCavassData = m_sliceIn;
	
	assert( m_sliceOut == NULL );
	m_sliceOut = new SliceData( fn );	
	if (!m_sliceOut->mIsCavassFile || !m_sliceOut->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}

	m_sliceOut->mNext = NULL;	
	mCavassData->mNext = m_sliceOut;

	if (isLoaded(0)) 
	{
	    int  w, h;
		mOverallXSize = mCavassData->m_xSize;		
		GetSize( &w, &h );
		m_scale = (double)((w-150)*1.0 / (mOverallXSize * 3));     
		double yScale = (double)(h-100)*1.0/mCavassData->m_ySize;
		if( yScale < m_scale )
			m_scale = yScale;  
	}       

	min_hist = mCavassData->m_min;
	if (min_hist == 0)
		min_hist = 1;
	max_hist = mCavassData->m_max;
	m_nMIPMinThre = min_hist;
	m_nMIPMaxThre = max_hist;

	m_nStartSlice = 1;
	m_nEndSlice = getNoSlices(0);

	resetThresholds();

    reload();    
    SetCursor( *wxSTANDARD_CURSOR );

}
//----------------------------------------------------------------------
void VOIIOICanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}


/************************************************************************
 *                                                                      *
 *      Function        : initialize_lookup_table                       *
 *      Description     : This function will iniitalize the lookup table* 
 *                        used to create the result (binary) image.     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : max_hist
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::initialize_lookup_table()
{
	int i;

	for(i=0;i<max_hist+1;i++)
		lookup_table[i] = 0;
}


/************************************************************************
 *                                                                      *
 *      Function        : create_new_lookup_table                       *
 *      Description     : This function will create the new lokup table *
 *                        for the interval selected.                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : interval, min_table, max_table must be set
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified: 4/25/07 for CAVASS by Dewey Odhner
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::create_new_lookup_table(ThresholdColor c)
{
	int i;

	initialize_lookup_table();

	if(interval == MAX_INTERVAL+1 /* ALL */)
		for(i=1; i<=MAX_INTERVAL; i++)
			make_lookup_table(min_table[i],max_table[i], c);
	else if(interval == MAX_INTERVAL+2 /* OPT */)
		make_lookup_table(min_table[MAX_INTERVAL+1], max_table[MAX_INTERVAL+1], c);
	else if (fuzzy_flag)
		make_lookup_table(min_table[interval], max_table[interval], c, inter1_table[interval], inter2_table[interval]);
	else
		make_lookup_table(min_table[interval], max_table[interval], c);
}


/************************************************************************
 *                                                                      *
 *      Function        : make_lookup_table                             *
 *      Description     : This function will make the actual lookup     *
 *                        table for the whole image.                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::make_lookup_table(int min, int max, ThresholdColor c, int inter1, int inter2)
{
	int i, threshold = 255;

	unsigned char red, green, blue;
	switch (interval)
	{
		case 1:
			red = 1;
			green = 127;
			blue = 255;
			break;
		case 2:
			red = 1;
			green = 255;
			blue = 127;
			break;
		case 3:
			red = 255;
			green = 127;
			blue = 127;
			break;
		case 4:
			red = 1;
			green = 255;
			blue = 255;
			break;
		case MAX_INTERVAL+1:
			red = 255;
			green = 127;
			blue = 255;
			break;
		case MAX_INTERVAL+2:
			red = 127;
			green = 255;
			blue = 1;
			break;
		default:
			assert(FALSE);
	}
	switch (c)
	{
		case WHITE:
			threshold = 255;
			break;
		case RED:
			threshold = red;
			break;
		case GREEN:
			threshold = green;
			break;
		case BLUE:
			threshold = blue;
			break;
	}

	for (i=min;i<=max;i++) 
		lookup_table[i] = 255*(i-min)/(max-min);
	for(i=0;i<min;i++)
		lookup_table[i] = 0;
	for(i=max+1; i<max_hist+1; i++)
		lookup_table[i] = 255;
}

void VOIIOICanvas::build_table(Table_Info *tbl)
{
    int i1,d,i;

    for(d=0;d<2;d++)
       for(i1=0;i1<tbl->dim[d];i1++) { 
          i = i1 +tbl->offset[d];
	      tbl->index_table[d][i1]=
	         (short)((int)((2*i+1)*tbl->oldd[d])/(int)(2*tbl->newd[d])); 
       }
}


//----------------------------------------------------------------------
void VOIIOICanvas::CreateDisplayImage(int which)
{
	//note: image data is 24-bit rgb
	CavassData&  A = *mCavassData;
	CavassData&  B = *(mCavassData->mNext);

	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}

	if (hist_scope==0 && which==0)
	{
		for (int i=0; i<=max_hist; i++)
			hist[i] = 0;
		total_count = 0;
	}

	assert( slice!=NULL );
	 const int  offset = 0; //(m_sliceNo) * A.m_xSize *A.m_ySize;
	if( which == 0 )
	{
		if (A.m_size==1) {
			unsigned char*  ucData = (unsigned char*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[ucData[j]-A.m_min];
				if (hist_scope == 0)
				hist[ucData[j]]++;
			} 
		} else if (A.m_size==2) {
			unsigned short* sData = (unsigned short*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[sData[j]-A.m_min];
				if (hist_scope == 0)
				hist[sData[j]]++;
			}
		} else if (A.m_size==4) {
			int*  iData = (int*)(A.getSlice( A.m_sliceNo ));    //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[iData[j]-A.m_min];
				if (hist_scope == 0)
				hist[iData[j]]++;
			}
		}

		if(m_images[0]!=NULL)
			m_images[0]->Destroy();
		m_images[0] = new wxImage( A.m_xSize, A.m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[0]->Rescale( (int)(ceil(m_scale*A.m_xSize)),
							  (int)(ceil(m_scale*A.m_ySize)) );
		}
		m_bitmaps[0] = new wxBitmap( *m_images[0] );
	}
	else
	{	
	
		if (A.m_size==1) {
			unsigned char*  ucData = (unsigned char*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				unsigned char value = ucData[j];
				if( value < B.m_min )
					value = B.m_min;
				else if( value > B.m_max )
					value = B.m_max;
					
				slice[i] = slice[i+1] = slice[i+2] = B.m_lut[value-B.m_min];
				
			} 
		} else if (A.m_size==2) {
			unsigned short* sData = (unsigned short*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				unsigned short value = sData[j];
				if( value < B.m_min )
					value = B.m_min;
				else if( value > B.m_max )
					value = B.m_max;
					
				slice[i] = slice[i+1] = slice[i+2] = B.m_lut[value-B.m_min];
				
			}
		} else if (A.m_size==4) {
			int*  iData = (int*)(A.getSlice( A.m_sliceNo ));    //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				int value = iData[j];
				if( value < B.m_min )
					value = B.m_min;
				else if( value > B.m_max )
					value = B.m_max;
					
				slice[i] = slice[i+1] = slice[i+2] = B.m_lut[value-B.m_min];
				
			}
		}
	
		if(m_images[which]!=NULL)
			m_images[which]->Destroy();

		m_images[which] = new wxImage( A.m_xSize, A.m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[which]->Rescale( (int)(ceil(m_scale*A.m_xSize)),
							  (int)(ceil(m_scale*A.m_ySize)) );
		}
		m_bitmaps[which] = new wxBitmap( *m_images[which] );
	}
	

}

//----------------------------------------------------------------------
void VOIIOICanvas::reload ( void ) 
{
    freeImagesAndBitmaps();

	CreateDisplayImage(0);

	if(m_bThresholdDone) /* update VOIIOIing */		
		CreateDisplayImage(1);

    Refresh();
}

//----------------------------------------------------------------------
void VOIIOICanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}

//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void VOIIOICanvas::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "VOIIOICanvas::OnChar" );
    if (e.m_keyCode==' ') {
        if (isLoaded(1)) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    }
}
//----------------------------------------------------------------------
void VOIIOICanvas::OnMouseMove ( wxMouseEvent& e ) 
{
    
   wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //if we are NOT in any mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    if (e.LeftIsDown() || e.RightIsDown()) {
    } else {
        if (lastX!=-1 || lastY!=-1) {
            SetCursor( *wxSTANDARD_CURSOR );
            lastX = lastY = -1;
        }
		int intrvl = interval==MAX_INTERVAL+2? MAX_INTERVAL+1:interval;
		bool fuzzy = interval==MAX_INTERVAL+2? false: fuzzy_flag;
		int eventx, eventy;
		eventx = pos.x;
		eventy = pos.y;
        if (thr_state==2 || thr_state==3)
        {
           if(eventx < histx) eventx = histx;
           if(eventx > histx+histw) eventx = histx+histw;
        }
		int *tbartx = (fuzzy? tbar4x: tbar2x)+intrvl;
        if(thr_state == 2)
		{
			if (!fuzzy)
				substate = 0;
			int *tbarcx =
				(substate==1? tbar2x: substate==2? tbar3x: tbar1x)+intrvl;
			int *ctablem = (substate==1? inter1_table:
				substate==2? inter2_table: min_table)+intrvl;
			if (substate)
			{
			   if (eventx < tbar1x[intrvl])
			      eventx = tbar1x[intrvl];
			   if (substate>1 && eventx < tbar2x[intrvl])
			      eventx = tbar2x[intrvl];
			}

			/* erase previous bar end-point & connection-bar (if any) */
			if(*tbarcx >= 0)
			{
			   erase_bar_end(dc, *tbarcx, substate==0? Left:Right);
			   if(eventx>tbar1x[intrvl] && substate==0)
			      erase_bar(dc, tbar1x[intrvl], eventx);
			}

			if (fuzzy)
			{
			   if (substate<2 && tbar3x[intrvl]>=0 && eventx>tbar3x[intrvl])
			   {
			      erase_bar_end(dc, tbar3x[intrvl], Right);
				  tbar3x[intrvl] = eventx;
				  inter2_table[intrvl]=((tbar3x[intrvl] - histx)*
				            (max_hist-min_hist)/histw+min_hist);
				  draw_bar_end(dc, tbar3x[intrvl],Right,inter2_table[intrvl]);
			   }
			   if (substate==0 && tbar2x[intrvl]>=0 && eventx>tbar2x[intrvl])
			   {
			      erase_bar_end(dc, tbar2x[intrvl], Right);
				  tbar2x[intrvl] = eventx;
				  inter1_table[intrvl]=((tbar2x[intrvl] - histx)*
				            (max_hist-min_hist)/histw+min_hist);
				  draw_bar_end(dc, tbar2x[intrvl],Right,inter1_table[intrvl]);
			   }
			}

			/* if left bar-end pushes right bar-end to the right */
			if(*tbartx >= 0 && eventx > *tbartx)
			{
			   erase_bar_end(dc, *tbartx, Right);
			   *tbartx = eventx;
			   max_table[intrvl]=((*tbartx - histx)*
							(max_hist-min_hist)/histw+min_hist);
			   draw_bar_end(dc, *tbartx, Right,
				  max_table[intrvl]);
			}

			*tbarcx = eventx;
			*ctablem=((*tbarcx - histx)*
						 (max_hist-min_hist)/histw+min_hist);

			/* draw new bar end-point */
			draw_bar_end(dc, *tbarcx, substate==0? Left:Right, *ctablem);

			/* draw new connection-bar (if final point is specified) */
			if (*tbartx >= 0)
			   draw_bar(dc, tbar1x[intrvl], *tbartx);
			else if (fuzzy)
			{
			   if (tbar3x[intrvl] >= 0)
			      draw_bar(dc, tbar1x[intrvl], tbar3x[intrvl]);
			   else if (tbar2x[intrvl] >= 0)
			      draw_bar(dc, tbar1x[intrvl], tbar2x[intrvl]);
			}

			if (fuzzy)
				RedrawHistogram(&dc);
		}
		else if(thr_state == 3)
		{ 
			if (eventx < tbar1x[intrvl])
			   eventx = tbar1x[intrvl];
			if (fuzzy && eventx<tbar3x[intrvl])
			   eventx = tbar3x[intrvl];

			/* erase previous connection-bar (if any) */
			if(tbar1x[intrvl] >= 0  &&  *tbartx >= 0)
			   erase_bar(dc, tbar1x[intrvl], *tbartx);

			/* erase previous bar end-point (if any) */
			if(*tbartx >= 0)
			{
			   erase_bar_end(dc, *tbartx, Right);
			   if (*tbartx == tbar1x[intrvl])
				  draw_bar_end(dc, tbar1x[intrvl], Left, min_table[intrvl]);
			}

			*tbartx = eventx;
			max_table[intrvl]=((*tbartx - histx)*
						 (max_hist-min_hist)/histw+min_hist);

			/* draw new bar end-point */
			draw_bar_end(dc, *tbartx, Right, max_table[intrvl]);

			/* draw new connection-bar (if final point is specified) */
			draw_bar(dc, tbar1x[intrvl], *tbartx);

			if (fuzzy)
				RedrawHistogram(&dc);
		}
    }

	Refresh();
}
//----------------------------------------------------------------------
void VOIIOICanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events

	m_bROI = false;
	Refresh();

}
//----------------------------------------------------------------------
void VOIIOICanvas::OnRightUp ( wxMouseEvent& e ) 
{
}
//----------------------------------------------------------------------
void VOIIOICanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
	   SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
    lastX = wx;
    lastY = wy;
	int eventx, eventy;
	eventx = pos.x;
	eventy = pos.y;
	int  x, y, z;
	mapWindowToData( dc.DeviceToLogicalX( pos.x ), dc.DeviceToLogicalY( pos.y ), x, y, z ); // ,which
	if (thr_state != 3)
		return;
    SetCursor( wxCursor(wxCURSOR_HAND) );

    if(eventx < histx) eventx = histx;
    if(eventx > histx+histw) eventx = histx+histw;
	int intrvl = interval==MAX_INTERVAL+2? MAX_INTERVAL+1: interval;
	bool fuzzy = false; //interval==MAX_INTERVAL+2? false: fuzzy_flag;
	int *tbartx = (fuzzy? tbar4x: tbar2x)+intrvl;
	assert(tbar1x[intrvl] >= 0);
	if (eventx < tbar1x[intrvl])
		eventx = tbar1x[intrvl];
	if (fuzzy && eventx < tbar3x[intrvl])
		eventx = tbar3x[intrvl];
	*tbartx = eventx;

	/* draw new bar end-point */
	draw_bar_end(dc, *tbartx, Right, (*tbartx
					- histx)*(max_hist-min_hist)/histw+min_hist);

	/* draw new connection-bar (if final point is specified) */
	draw_bar(dc, tbar1x[intrvl], *tbartx);

	/* PERFORM OPERATION */

	min_table[intrvl]=((tbar1x[intrvl] - histx)*
					(max_hist-min_hist)/histw+min_hist);
	max_table[intrvl]=((*tbartx - histx)*
					(max_hist-min_hist)/histw+min_hist);
	if (fuzzy)
	{
		inter1_table[intrvl]=((tbar2x[intrvl] - histx)*
		            (max_hist-min_hist)/histw+min_hist);
		inter2_table[intrvl]=((tbar3x[intrvl] - histx)*
		            (max_hist-min_hist)/histw+min_hist);
	}
	RunThreshold(); 

	/*The overlay for the icon is created as soon as the */
	/*lookup table is ready.                             */

	thr_state = 1;
    SetStatusText( "Select Interval",   2 );
    SetStatusText( "", 3 );
    SetStatusText( "", 4 );
}
//----------------------------------------------------------------------
void VOIIOICanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
 /*   if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    } */
}


void VOIIOICanvas::RunThreshold()
{
	CavassData&  B = *(mCavassData->mNext);
	B.resetLUT( min_table[interval], max_table[interval] );

    CreateDisplayImage(1);
	m_bThresholdDone = true;
	Refresh();
}


//----------------------------------------------------------------------
void VOIIOICanvas::OnLeftDown ( wxMouseEvent& e ) 
{
     SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
    lastX = wx;
    lastY = wy;
    SetCursor( wxCursor(wxCURSOR_HAND) );

	int eventx, eventy;
	eventx = pos.x;
	eventy = pos.y;
    if(eventx < histx) eventx = histx;
    if(eventx > histx+histw) eventx = histx+histw;
	int font_h = GetCharHeight();
	int intrvl = interval==MAX_INTERVAL+2? MAX_INTERVAL+1: interval;
	bool fuzzy = false; //interval==MAX_INTERVAL+2? false: fuzzy_flag;
	int *tbartx = (fuzzy? tbar4x: tbar2x)+intrvl;
	if (thr_state == 0)
	{
		thr_state = 1;
	    SetStatusText( "Select Interval",   2 );
	    SetStatusText( "", 3 );
	    SetStatusText( "", 4 );
	}
	else if (thr_state == 1)
	{
		if(tbar1x[intrvl] >= 0 && *tbartx >= 0)
				erase_bar(dc, tbar1x[intrvl], *tbartx);

		if(tbar1x[intrvl] >= 0)
				erase_bar_end(dc, tbar1x[intrvl], Left);

		if(eventy>=font_h*11/2 && eventy<font_h*(11+2*num_optima)/2)
		{  int o;
			  for (o=0; eventy>=font_h*(13+2*o)/2; o++)
				;
			  min_table[intrvl] = optimum[o];
			  for (tbar1x[intrvl]=histx; (tbar1x[intrvl]-histx)
					  *(max_hist-min_hist)/histw+min_hist<
					  min_table[intrvl]; tbar1x[intrvl]++)
				;

			  /* if left bar-end pushes right bar-end to the right */
			  if (*tbartx >= 0 && tbar1x[intrvl] > *tbartx)
			  {
				erase_bar_end(dc, *tbartx, Right);
				*tbartx = tbar1x[intrvl];
				max_table[intrvl]=((*tbartx - histx)*
						(max_hist-min_hist)/histw+min_hist);
				draw_bar_end(dc, *tbartx, Right,
				  max_table[intrvl]);
			  }
			  if (fuzzy)
			  {
				if (tbar2x[intrvl]>=0 && tbar1x[intrvl]>tbar2x[intrvl])
				{
				  erase_bar_end(dc, tbar2x[intrvl], Right);
				  tbar2x[intrvl] = tbar1x[intrvl];
				  inter1_table[intrvl] = ((tbar2x[intrvl] - histx)*
				    	(max_hist-min_hist)/histw+min_hist);
				}
				if (tbar3x[intrvl]>=0 && tbar1x[intrvl]>tbar3x[intrvl])
				{
				  erase_bar_end(dc, tbar3x[intrvl], Right);
				  tbar3x[intrvl] = tbar1x[intrvl];
				  inter2_table[intrvl] = ((tbar3x[intrvl] - histx)*
				    	(max_hist-min_hist)/histw+min_hist);
				}
			  }

			  if (*tbartx < 0)
			  {
				max_table[intrvl] = max_hist;
				*tbartx = histx+histw;
				draw_bar_end(dc, *tbartx, Right,
				  max_table[intrvl]);
			  }

			  /* draw new bar end-point */
			  draw_bar_end(dc, tbar1x[intrvl], Left,
				min_table[intrvl]);

			  /* draw new connection-bar (if final point is specified) */
			  draw_bar(dc, tbar1x[intrvl], *tbartx);

			  RunThreshold(); 
		}
		else
		{
			if (fuzzy && intrvl>1 && tbar3x[intrvl-1]>=0 &&
				tbar4x[intrvl-1]>=0)
			{
				if (tbar1x[intrvl]<0 || tbar2x[intrvl]<0)
				{
					tbar1x[intrvl] = tbar3x[intrvl-1];
					tbar2x[intrvl] = tbar4x[intrvl-1];
					min_table[intrvl]=((tbar1x[intrvl] - histx)*
						(max_hist-min_hist)/histw+min_hist);
					inter1_table[intrvl]=((tbar2x[intrvl] - histx)*
						(max_hist-min_hist)/histw+min_hist);
				}
				if (eventx < tbar2x[intrvl])
					eventx = tbar2x[intrvl];
				tbar3x[intrvl] = eventx;
				substate = 2;
				SetStatusText( "Third Point", 2 );
			}
			else
			{
				tbar1x[intrvl] = eventx; /* set location for initial bar */
				substate = 0;
				SetStatusText( "Initial Point", 2 );
			}

			/* draw new bar-end (if there is any) */
			draw_bar_end(dc, tbar1x[intrvl], Left, (tbar1x[intrvl] - histx)*
				(max_hist-min_hist)/histw+min_hist);

			/* erase previous connection-bar (if any) */
			if(tbar1x[intrvl] >= 0 && *tbartx >= 0)
				draw_bar(dc, tbar1x[intrvl], *tbartx);


			thr_state = 2;
			SetStatusText( "", 3 );
			SetStatusText( "", 4 );
		}
	}
	else if (thr_state == 2)
	{
		if (fuzzy)
		{
			switch (substate)
			{
			case 0:
				tbar1x[intrvl] = eventx;
				draw_bar_end(dc, tbar1x[intrvl], Left, min_table[intrvl]);
				substate = 1;
				SetStatusText( "Second Point", 2 );
				break;
			case 1:
				if (eventx < tbar1x[intrvl])
					eventx = tbar1x[intrvl];
				tbar2x[intrvl] = eventx;
				draw_bar_end(dc, tbar2x[intrvl], Right, inter1_table[intrvl]);
				substate = 2;
				SetStatusText( "Third Point", 2 );
				break;
			case 2:
				if (eventx < tbar2x[intrvl])
					eventx = tbar2x[intrvl];
				tbar3x[intrvl] = eventx;
				draw_bar_end(dc, tbar3x[intrvl], Right, inter2_table[intrvl]);
				thr_state = 3;
				SetStatusText( "", 2 );
				SetStatusText( "Final Point", 3 );
				break;
			}
			if (tbar4x[intrvl] > eventx)
				draw_bar(dc, tbar1x[intrvl], tbar4x[intrvl]);
			else
				draw_bar(dc, tbar1x[intrvl], eventx);
		}
		else
		{
			tbar1x[intrvl] = eventx;

			/* draw new bar end-point */
			draw_bar_end(dc, tbar1x[intrvl],Left,min_table[intrvl]);

			/* draw new connection-bar (if final point is specified) */
			if(tbar2x[intrvl] > tbar1x[intrvl])
			{
				draw_bar(dc, tbar1x[intrvl], tbar2x[intrvl]);
			}

			thr_state = 3;
			SetStatusText( "", 2 );
			SetStatusText( "Final Point", 3 );
			SetStatusText( "", 4 );
		}
	}

	Refresh();

}
//----------------------------------------------------------------------
void VOIIOICanvas::OnLeftUp ( wxMouseEvent& e ) 
{
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------


/************************************************************************
 *                                                                      
 *      Function        : draw_thresh_fn                             
 *      Description     : Draws a line between the specified points     
 *                        no restriction on the y-position
 *      Return Value    : None                                          
 *      Parameters      : x1, y1, x2, y2  the x,y co-ordinates
 *                        of the points between which the line is to 
 *                        be drawn
 *                  
 *      Side effects    : None                                          
 *      Limitations     :                               
 *      Entry condition :                       
 *                                                                      
 *      Related funcs   : draw_bar_end, erase_bar_end, draw_bar 
 *                                              
 *      History         : Written by K. P. Venugopal 27th July 1993 
 *                                                                      
 ************************************************************************/

void VOIIOICanvas::draw_thresh_fn(wxDC* dc, int x1, int y1, int x2, int y2)
{
    if(x1 < histx) x1 = histx;
    if(x1 > histx+histw) x1 = histx+histw;
    if(x2 < histx) x2 = histx;
    if(x2 > histx+histw) x2 = histx+histw;

	dc->DrawLine(x1, y1, x2, y2);
}


void VOIIOICanvas::RedrawHistogram( wxDC* dc )
{
	wxColour bg;
	if (Preferences::getCustomAppearance())
		bg = wxColour(DkBlue);
	else
		bg = *wxBLACK;
	dc->SetPen( wxPen(bg) );
	dc->SetBrush( wxBrush(bg) );
	dc->DrawRectangle( histx, histy, histw, histh );
    int maxH = 1;
    int  i;
    for (i=min_hist; i<=max_hist; i++)
        if(hist[i]>maxH) maxH = hist[i];
	dc->SetPen( wxPen(wxColour(Yellow)) );
    for (i=min_hist; i<=max_hist; i++)
    {
        int posX = (int) (histx+(i-min_hist)*histw/(max_hist-min_hist));
        int posY = (int) (histy+histh-(histh*hist[i]*m_fHistZoomFactor/maxH));
        if(posY < histy) posY = histy; 
        dc->DrawLine(posX, histy+histh, posX, posY+1);
    }
	dc->SetPen( wxPen(wxColour(32, 160, 255)) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
}




/************************************************************************
 *                                                                      *
 *      Function        : Redraw_Threshold_Values                       *
 *      Description     : This function redraws the histograms and      *
 *                        displays the intervals that were selected     *
 *                        and also shows the icona nd result image with *
 *                        with the proper threshold (overlay) values.   *
 *      Return Value    :  None
 *      Parameters      : dc: 
 *      Side effects    : None.                                         *
 *      Entry condition : tbar1x, tbar2x, min_table, max_table,
 *                        must be properly set.
 *                        VCreateColormap must be called first.
 *      Related funcs   : None.                                         *
 *      History         : Written on November 20, 1992 by Krishna Iyer. *
 *                        Modified 9/18/97 interval value restored
 *                        by Dewey Odhner.
 *                        Modified 11/8/01 for optimal mode
 *                        by Dewey Odhner.
 *                        Modified 7/18/07 corrected for optimal mode
 *                        by Dewey Odhner.
 *                        Modified 9/13/07 for fuzzy thresholding
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::Redraw_Threshold_Values(wxDC & dc)
{
    int i;

    i = interval;
	if (interval == MAX_INTERVAL+2)
	{
        if (tbar1x[MAX_INTERVAL+1] != -1)
            draw_bar_end(dc, tbar1x[MAX_INTERVAL+1],
				Left, min_table[MAX_INTERVAL+1]);
        if (tbar2x[MAX_INTERVAL+1] != -1)
            draw_bar_end(dc, tbar2x[MAX_INTERVAL+1],
				Right, max_table[MAX_INTERVAL+1]);
        if (tbar1x[MAX_INTERVAL+1] != -1 && tbar2x[MAX_INTERVAL+1] != -1)
            draw_bar(dc, tbar1x[MAX_INTERVAL+1], tbar2x[MAX_INTERVAL+1]);
	}
	else
      for(interval=1; interval<=MAX_INTERVAL; interval++)
      {
		int *tbartx = (fuzzy_flag? tbar4x: tbar2x)+interval;
        if (tbar1x[interval] != -1)
		{
			for (tbar1x[interval]=histx;
					min_table[interval]>(tbar1x[interval]-histx)*
					(max_hist-min_hist)/histw+min_hist; tbar1x[interval]++)
				;
		    draw_bar_end(dc, tbar1x[interval], Left, min_table[interval]);
		}
		if (*tbartx != -1)
        {
			for (*tbartx=histx; max_table[interval]>(*tbartx - histx)*
					(max_hist-min_hist)/histw+min_hist; tbartx[0]++)
				;
            draw_bar_end(dc, *tbartx, Right, max_table[interval]);
        }
		if (fuzzy_flag)
		{
			if (tbar2x[interval] >= 0)
				draw_bar_end(dc,tbar2x[interval],Right,inter1_table[interval]);
			if (tbar3x[interval] >= 0)
				draw_bar_end(dc,tbar3x[interval],Right,inter2_table[interval]);
		}
        if (tbar1x[interval] != -1 && *tbartx != -1)
            draw_bar(dc, tbar1x[interval], *tbartx);
      }

    interval = i;
}


/************************************************************************
 *                                                                      *
 *      Function        : draw_bar_end                                  *
 *      Description     : This function draws the end points in the     *
 *                        histogram, while selecting the intervals.     *
 *      Return Value    : None.
 *      Parameters      :  dc - the dc of the image window.             *
 *                         x - the location of the end point in pixels  *
 *                              with respect to histogram coordinates.  *
 *                         mode - whether right or left interval.       *
 *                         value - the interval end value.
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified: 11/7/01 value parameter added
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::draw_bar_end(wxDC &dc, int x, int mode, int value)
{
	char text[12];
	int yl;
	int font_h, font_w;

    dc.SetTextBackground( *wxBLACK );
    dc.SetTextForeground( wxColour(Yellow) );
	dc.SetPen( wxPen(wxColour(Yellow)) );
	font_h = GetCharHeight();
	font_w = GetCharWidth();

	/* keep bar inside histogram area */
	if(x < histx) x = histx;
	if(x > histx+histw) x = histx+histw;

	sprintf(text,"%d", value);
	yl= (int)(2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

	dc.DrawLine(x, histy+histh+3+yl, x, histy+histh+4+font_h+yl);
	if(mode == Left)
	    dc.DrawText(text, x-3-font_w*strlen(text),
	    	histy+histh+3+font_h+font_h/4+yl);
	else
	    dc.DrawText(text, x+3,  histy+histh+3+font_h+font_h/4+yl);
}


/************************************************************************
 *                                                                      *
 *      Function        : erase_bar_end                                 *
 *      Description     : This function erases the bar end in the       *
 *                        histogram.                                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  dc - the dc of the image window.             *
 *                         x - the coordinate of the interval with      *
 *                              respect to the histogram.               *
 *                         mode - whether it is the left or right       *
 *                              interval.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified 9/18/97 gc deleted by Dewey Odhner.
 *                        Modified 10/7/97 5 characters width of text
 *                        cleared by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::erase_bar_end(wxDC &dc, int x, int mode)
{
	int font_h,font_w,yl;
 
	dc.SetPen( wxPen(GetBackgroundColour()) );
	dc.SetBrush( wxBrush(GetBackgroundColour()) );
	font_h = GetCharHeight();
	font_w = GetCharWidth();

	/* keep bar inside histogram area */
	if(x < histx) x = histx;
	if(x > histx+histw) x = histx+histw;

	/* clear bar */
	yl=(int) (2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

	dc.DrawRectangle(x, histy+histh+3+yl, 1, font_h+font_h/8);
	/* clear text under bar-end */
	if(mode == Left)
	   dc.DrawRectangle(x-3-5*font_w,  histy+histh+3+font_h+font_h/4+yl, 5*font_w, font_h+font_h/8);
	else
	   dc.DrawRectangle(x+3, histy+histh+3+font_h+font_h/4+yl, 5*font_w, font_h+font_h/8);

}

/************************************************************************
 *                                                                      *
 *      Function        : draw_bar                                      *
 *      Description     : This function draws the bar connecting the two*
 *                        end points while selecting the range in the   *
 *                        histogram.                                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  dc - the GC of the image window.             *
 *                         x1 - the left interval.                      *
 *                         x2 - the right interval.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::draw_bar(wxDC &dc, int x1, int x2)
{
	int font_h, yl;
 
	dc.SetPen( wxPen(wxColour(Yellow)) );
	font_h = GetCharHeight();

	/* keep bar inside histogram area */
	if(x1 < histx) x1 = histx;
	if(x1 > histx+histw) x1 = histx+histw;
	if(x2 < histx) x2 = histx;
	if(x2 > histx+histw) x2 = histx+histw;

	yl = (int) (2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

	dc.DrawLine(x1, histy+histh+3+font_h/2+yl, x2+1,
					histy+histh+3+font_h/2+yl);
}


/************************************************************************
 *                                                                      *
 *      Function        : erase_bar                                     *
 *      Description     : This function erases the bar between the      *
 *                        end points in the histogram.                  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  dc - the dc of the image window.             *
 *                         x1 - the left bar.                           *
 *                         x2 - the right bar.                          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified 9/18/97 gc deleted by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void VOIIOICanvas::erase_bar(wxDC &dc, int x1, int x2)
{
    int yl;

	dc.SetPen( wxPen(GetBackgroundColour()) );
	dc.SetBrush( wxBrush(GetBackgroundColour()) );
	int font_h = GetCharHeight();
    if(interval != 0)
    {
        yl = (int) (2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

        dc.DrawRectangle(x1, histy+histh+3+font_h/2+yl, x2-x1+3, 1);
    }
}


void VOIIOICanvas::resetThresholds ( void ) 
{
	num_optima = 0;
	for (int i=1; i<=MAX_INTERVAL+1; i++)
	{
		min_table[i] = 1;
		max_table[i] = 0;
		tbar1x[i] = tbar2x[i] = -1;
		tbar3x[i] = -1;
		tbar4x[i] = -1;
		inter1_table[i] = 0;
		inter2_table[i] = 0;
	}
	min_table[1] = min_hist;
	max_table[1] = max_hist;
	tbar1x[1] = histx;
	tbar2x[1] = histx+histw;
	thr_state = 1;
    SetStatusText( "Select Interval",   2 );
    SetStatusText( "", 3 );
    SetStatusText( "", 4 );
    lastX = lastY = -1;
	m_bThresholdDone = false;
	fuzzy_flag = false;
}
//----------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////

void VOIIOICanvas::OnPaint ( wxPaintEvent& e ) 
{
    wxMemoryDC  m;
    int         w, h;
    GetSize( &w, &h );
    wxBitmap    bitmap(w, h);
    m.SelectObject( bitmap );
    if (Preferences::getCustomAppearance())
#if wxCHECK_VERSION(2, 9, 0)
        m.SetBrush( wxBrush(wxColour(DkBlue), wxBRUSHSTYLE_SOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxBRUSHSTYLE_SOLID) );
#else
        m.SetBrush( wxBrush(wxColour(DkBlue), wxSOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxSOLID) );
#endif
    m.DrawRectangle( 0, 0, w, h );
    
    paint( &m );

    wxPaintDC  dc(this);
    PrepareDC(dc);
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
}
//----------------------------------------------------------------------
void VOIIOICanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	if(m_bitmaps[0]!=NULL)
	{
		if (m_bitmaps[0]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[0], m_tx, m_ty );
			if(m_overlay)
			{
				const wxString  s = wxString::Format( "%d/%d",getSliceNo(0)+1,getNoSlices(0));
				dc->DrawText( s, m_tx, m_ty );
			}
		}		

	 if (max_hist > min_hist)
	  {
        int i, maxH, mode, min_postiv, max_postiv;
		double total_val;
		char msg[32];
        int font_h = GetCharHeight();

		histx = (short)(mCavassData->m_xSize * m_scale + 30 + m_tx);
		histw = (short)(mCavassData->m_xSize * m_scale);

		total_val = total_count = maxH = mode = min_postiv = max_postiv = 0;
		for (i=min_hist?min_hist:1; i<=max_hist; i++)
			if (hist[i])
			{
				if (hist[i] > maxH)
					mode=i, maxH=hist[i];
				total_count += hist[i];
				total_val += (double)hist[i]*i;
				if (min_postiv == 0)
					min_postiv = i;
				max_postiv = i;
			}

		if (total_count)
		{
			sprintf(msg, "Mean: %.1f", total_val/total_count);
			dc->DrawText(msg, histx, font_h/2);
		}
		sprintf(msg, "Mode: %d", mode);
		dc->DrawText(msg, histx, font_h*3/2);
		sprintf(msg, "Min.: %d", min_postiv);
		dc->DrawText(msg, histx, font_h*5/2);
		sprintf(msg, "Max.: %d", max_postiv);
		dc->DrawText(msg, histx, font_h*7/2);
		for (i=0; i<num_optima; i++)
		{
			sprintf(msg, "Opt.: %d", optimum[i]);
			dc->DrawText(msg, histx, font_h*(11+2*i)/2);
		}

		RedrawHistogram(dc);
		Redraw_Threshold_Values(*dc);
	  }

		if (m_bThresholdDone && m_bitmaps[1]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[1], w - (int)(mCavassData->m_xSize * m_scale) - m_tx, m_ty );
			if(m_overlay)
			{
				const wxString  s = wxString::Format( "%d/%d",getSliceNo(0)+1,getNoSlices(0));
				dc->DrawText( s, w - (int)(mCavassData->m_xSize * m_scale) - m_tx, m_ty );
			}
		}		
	
	}
	else if (m_backgroundLoaded) 
	{		
		const int  bmW = m_backgroundBitmap.GetWidth();
		const int  bmH = m_backgroundBitmap.GetHeight();
		dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
	}
}


//----------------------------------------------------------------------
void VOIIOICanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool VOIIOICanvas::isLoaded ( const int which ) const {
    if (which==0) {
        if (mCavassData==NULL)    return false;
        const CavassData&  cd = *mCavassData;
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else if (which==1) {
        if (mCavassData==NULL || mCavassData->mNext==NULL)    return false;
        const CavassData&  cd = *(mCavassData->mNext);
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else {
        assert( 0 );
    }
    return false;
}

int VOIIOICanvas::SaveProfile ( const char* cFilename )
{

	wxString  voiCmd = "\"";
	voiCmd += Preferences::getHome()+wxString::Format(
		"/ndvoi\" \"%s\" \"%s\" %d %d %d %d %d %d %d %d %d",
		mCavassData->m_fname, cFilename, 
		0, 0, 0, mCavassData->m_xSize, mCavassData->m_ySize, min_table[interval], max_table[interval] , m_nStartSlice-1, m_nEndSlice-1 );
	
	wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
	ProcessManager  q( "ndvoi running...", voiCmd );

	if( m_nSH0Type )
	{
		int no_ioi = 0;
		double SHI_voxel_size = pow((double)mCavassData->m_xSize*mCavassData->m_vh.scn.xypixsz[0] * mCavassData->m_ySize*mCavassData->m_vh.scn.xypixsz[1]*
			(m_nEndSlice-m_nStartSlice), (double)1/3.)/64;

		int low = min_table[interval];
		int high = max_table[interval];
		float sh_low, sh_high;

		if( m_nSH0Type == 1) //MIP
		{
			
			if(no_ioi)
			{
				sh_low = min_hist;  //min_threshold
				sh_high = max_hist; //max_threshold;
			}
			else
			{
				sh_low = (m_nMIPMinThre-low)*255/(high-low);  //min_table[interval], max_table[interval]
				sh_high = (m_nMIPMaxThre-low)*255/(high-low);
			}
			voiCmd = "\"";
			voiCmd += Preferences::getHome()+wxString::Format("/shell\" \"%s\" 1 0 1 0 0 0 0 %.0f %.0f 0 0 0 .99 1 9999 -1 0 4 \"%s.SH0\"",
				cFilename, sh_low, sh_high, cFilename);
			wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
			ProcessManager  q2( "shell running...", voiCmd );

			voiCmd = "\"";
			voiCmd += Preferences::getHome()+wxString::Format("/shell\" \"%s\" 1 0 1 0 0 0 0 %.0f %.0f 0 0 0 .99 1 9999 %f 0 4 \"%s.SHI\"",
				cFilename, sh_low, sh_high, SHI_voxel_size, cFilename);
			wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
			ProcessManager  q3( "shell running...", voiCmd );

		}
		else if( m_nSH0Type == 2) //CT BONE
		{			
			sh_low = min_hist+.313*(max_hist-min_hist);
			sh_high = min_hist+.342*(max_hist-min_hist);
			if(!no_ioi)
			{
				sh_low = (sh_low-low)*255/(high-low);
				sh_high = (sh_high-low)*255/(high-low);
			}
			voiCmd = "\"";
			voiCmd += Preferences::getHome()+wxString::Format("/shell\" \"%s\" 200 0 1 0 0 0 0 %.0f %.0f 0 0 0 .16 1 9999 -1 0 4 \"%s.SH0\"",
				cFilename, sh_low, sh_high, cFilename);
			wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
			ProcessManager  q4( "shell running...", voiCmd );

			voiCmd = "\"";
			voiCmd += Preferences::getHome()+wxString::Format("/shell\" \"%s\" 200 0 1 0 0 0 0 %.0f %.0f 0 0 0 .16 1 9999 %f 0 4 \"%s.SHI\"",
				cFilename, sh_low, sh_high, SHI_voxel_size, cFilename);		
			wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
			ProcessManager  q5( "shell running...", voiCmd );

		}

	}


	if (q.getCancel())    return 0;
#if 0
	int  error = q.getStatus();
	if (error != 0) {  wxMessageBox( "ndvoi failed." );  return 0;  }
#endif

	return 1;
}

bool VOIIOICanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int VOIIOICanvas::getCenter ( const int which ) const 
{
    if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_center;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_center;
    } 
	else 
	{
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int VOIIOICanvas::getMax ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_max;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_max;
    } 
	else 
	{
        assert( 0 );
		return -1;
    }  
}
//----------------------------------------------------------------------
int VOIIOICanvas::getMin ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_min;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_min;
    } 
	else 
	{
        assert( 0 );
		return -1;
    }    
}
//----------------------------------------------------------------------
//number of slices in entire data set
int VOIIOICanvas::getNoSlices ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_zSize;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_zSize;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}

bool VOIIOICanvas::getVOIIOIDone (void) const
{
	return m_bVOIIOIDone;
}
//----------------------------------------------------------------------
double VOIIOICanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int VOIIOICanvas::getSliceNo ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_sliceNo;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int VOIIOICanvas::getWidth ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->m_width;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_width;
    } 
	else 
	{
        assert( 0 );
		return -1;
    }       
}

//----------------------------------------------------------------------
bool VOIIOICanvas::getInvert ( const int which ) const 
{
	 if (which==0) 
	{        
		assert( m_sliceIn!=NULL );        
        return m_sliceIn->mInvert;
    } 
	else if (which==1) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->mInvert;
    } 
	else 
	{
        assert( 0 );
		return 0;
    }          
}
void VOIIOICanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void VOIIOICanvas::setCenter ( const int which, const int center ) 
{
    if (which==0) 
	{
		assert( m_sliceIn!=NULL );        
        m_sliceIn->m_center = center;
    } 
	else if (which==1) 
	{
        assert( m_sliceOut!=NULL );        
        m_sliceOut->m_center = center;
    } 
	else 
	{
        assert( 0 );
    }
}

//----------------------------------------------------------------------
void VOIIOICanvas::setInvert ( const int which, const bool invert ) 
{
	if (which==0) 
	{
		assert( m_sliceIn!=NULL );        
        m_sliceIn->mInvert = invert;
    } 
	else if (which==1) 
	{
        assert( m_sliceOut!=NULL );        
        m_sliceOut->mInvert = invert;
    } 
	else 
	{
        assert( 0 );
    }  
}
//----------------------------------------------------------------------
void VOIIOICanvas::setScale   ( const double scale )  {  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    m_scale = scale;
    int  w, h;
    GetSize( &w, &h );
    m_cols  = (int)(w / (mOverallXSize * m_scale));
    m_rows  = (int)(h / (mOverallYSize * m_scale));
    if (m_cols<1)  m_cols=1;
    if (m_rows<1)  m_rows=1;
    reload();

}
//----------------------------------------------------------------------
void VOIIOICanvas::setSliceNo ( const int which, const int sliceNo ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_sliceNo = sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_sliceNo = sliceNo;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void VOIIOICanvas::setWidth ( const int which, const int width ) 
{
	if (which==0) 
	{
		assert( m_sliceIn!=NULL );        
        m_sliceIn->m_width = width;
    } 
	else if (which==1) 
	{
        assert( m_sliceOut!=NULL );        
        m_sliceOut->m_width = width;
    } 
	else 
	{
        assert( 0 );
    }     
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( VOIIOICanvas, wxPanel )
BEGIN_EVENT_TABLE       ( VOIIOICanvas, wxPanel )
    EVT_PAINT(            VOIIOICanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           VOIIOICanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        VOIIOICanvas::OnLeftDown     )
    EVT_LEFT_UP(          VOIIOICanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      VOIIOICanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        VOIIOICanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       VOIIOICanvas::OnRightDown    )
    EVT_RIGHT_UP(         VOIIOICanvas::OnRightUp      )
    EVT_CHAR(             VOIIOICanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
