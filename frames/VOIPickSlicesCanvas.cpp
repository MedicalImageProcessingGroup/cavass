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
 * \file:  VOIPickSlicesCanvas.cpp
 * \brief  VOIPickSlicesCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "VOIPickSlicesCanvas.h"
#include <wx/combo.h>
#include <wx/listctrl.h>

//#include  "VOIPickSlicesAlgorithm.h"
using namespace std;
//----------------------------------------------------------------------
VOIPickSlicesCanvas::VOIPickSlicesCanvas ( void )  {  puts("VOIPickSlicesCanvas()");  }
//----------------------------------------------------------------------
VOIPickSlicesCanvas::VOIPickSlicesCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
    m_scale          = 1.0;
    m_overlay        = true;
	m_bLayout        = false;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 40;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
//	m_VOIPickSlicesType = VOIPickSlices_GRADIENT2D;
	m_bVOIPickSlicesDone = false;	

	m_bMouseMove = false;
	BLINK = false;
	m_nSH0Type = 0;

}
//----------------------------------------------------------------------
VOIPickSlicesCanvas::~VOIPickSlicesCanvas ( void ) {
    cout << "VOIPickSlicesCanvas::~VOIPickSlicesCanvas" << endl;
    wxLogMessage( "VOIPickSlicesCanvas::~VOIPickSlicesCanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::release ( void ) {
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::loadFile ( const char* const fn ) 
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

	int  w, h;
	if (isLoaded(0)) 
	{
	    
		mOverallXSize = mCavassData->m_xSize;		
		GetSize( &w, &h );
		m_scale = (double)((w-150)*1.0 / (mOverallXSize * 3));     
		double yScale = (double)(h-100)*1.0/mCavassData->m_ySize;
		if( yScale < m_scale )
			m_scale = yScale;  
		//m_scale = 1.0;	
	}       

	min_hist = mCavassData->m_min;
	if (min_hist == 0)
		min_hist = 1;
	max_hist = mCavassData->m_max;

    reload();    
    SetCursor( *wxSTANDARD_CURSOR );

	wxPoint ListPos(w/2+50, m_ty+30);
	mListCtrl = new wxListCtrl(this, -1, ListPos, wxSize(220, h/2), wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);

	wxListItem t_list_item;
	mListCtrl->InsertColumn(0, wxString("Slice#"), wxLIST_FORMAT_LEFT, 100 );
	t_list_item.m_format = wxLIST_FORMAT_LEFT;
	t_list_item.m_mask = wxLIST_MASK_FORMAT;
	mListCtrl->SetColumn(0, t_list_item);
	mListCtrl->InsertColumn(1, wxString("Pos"), wxLIST_FORMAT_LEFT, 100 );
	mListCtrl->SetColumn(1, t_list_item);
	
	int j=0;
	int sliceNum = getNoSlices(0);
	for( j=0; j<sliceNum; j++)
	{
		mListCtrl->InsertItem( j, wxString::Format("%d",j+1) );
		mListCtrl->SetItem(j, 1, wxString::Format("%f", mCavassData->m_vh.scn.loc_of_subscenes[mCavassData->m_vh.scn.dimension==4? mCavassData->m_vh.scn.num_of_subscenes[0]+j: j]));
	}	   

//	InitVOIPickSlices();
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}

//----------------------------------------------------------------------
void VOIPickSlicesCanvas::CreateDisplayImage(int which)
{
	//note: image data is 24-bit rgb
	CavassData&  A = *mCavassData;

	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}
	 const int  offset = 0; //(m_sliceNo) * A.m_xSize *A.m_ySize;
	if( which == 0 )
	{
		if (A.m_size==1) {
			unsigned char*  ucData = (unsigned char*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[ucData[j]-A.m_min];			
			} 
		} else if (A.m_size==2) {
			unsigned short* sData = (unsigned short*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[sData[j]-A.m_min];				
			}
		} else if (A.m_size==4) {
			int*  iData = (int*)(A.getSlice( A.m_sliceNo ));    //(A.m_data);
			for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 && j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[iData[j]-A.m_min];				
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

	//delete [] slice;
}

//----------------------------------------------------------------------
void VOIPickSlicesCanvas::reload ( void ) 
{
  //if (!isLoaded(0) || !isLoaded(1))    return;
    freeImagesAndBitmaps();

	CreateDisplayImage(0);

    Refresh();
}

//----------------------------------------------------------------------
void VOIPickSlicesCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
#if 0
    //map a point in the window back into the 3d data set
    wx -= m_tx;    wy -= m_ty;
    const int  col = (int)((double)wx / (ceil(m_xSize*m_scale)+sSpacing));
    const int  row = (int)((double)wy / (ceil(m_ySize*m_scale)+sSpacing));
    x = (int)( wx % ((int)(ceil(m_xSize*m_scale)+sSpacing)) / m_scale );
    y = (int)( wy % ((int)(ceil(m_ySize*m_scale)+sSpacing)) / m_scale );
    z = m_sliceNo + row*m_cols + col;
    
    //clamp the values to acceptable values
    if (x<0)  x=0;
    else if (x>=m_xSize)  x=m_xSize-1;
    if (y<0)  y=0;
    else if (y>=m_ySize)  y=m_ySize-1;
    if (z<0)  z=0;
    else if (z>=m_zSize)  z=m_zSize-1;
#endif
}

//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void VOIPickSlicesCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "VOIPickSlicesCanvas::OnChar" << endl;
    wxLogMessage( "VOIPickSlicesCanvas::OnChar" );
    if (e.m_keyCode==' ') {
        if (isLoaded(1)) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    }
    //pass the event up to the parent frame
    //m_parent_frame->ProcessEvent( e );
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnMouseMove ( wxMouseEvent& e ) 
{
    
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
	
	Refresh();

}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnRightUp ( wxMouseEvent& e ) 
{
    //if (isLoaded(0)) {
    //    mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
    //    reload();
    //}
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
	   SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();                        

    SetStatusText( "Select Interval",   2 );
    SetStatusText( "Select Boundary", 3 );
    SetStatusText( "", 4 );
//	UpdateThresholdTablets();
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
 /*   if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    } */
}


//void VOIPickSlicesCanvas::RunThreshold()
//{	
//}


//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnLeftDown ( wxMouseEvent& e ) 
{
     SetFocus();  //to regain/recapture keypress events   
	
	Refresh();

}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::OnLeftUp ( wxMouseEvent& e ) 
{
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////

void VOIPickSlicesCanvas::OnPaint ( wxPaintEvent& e ) 
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
void VOIPickSlicesCanvas::paint ( wxDC* dc ) 
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

			dc->DrawText( "Instruction: Select slices by using left button", w/2+50, m_ty-5 );
			dc->DrawText( "If you want to select more, press Ctrl Key when selecting", w/2+50, m_ty+10 );
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
void VOIPickSlicesCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool VOIPickSlicesCanvas::isLoaded ( const int which ) const {
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

int VOIPickSlicesCanvas::SaveProfile ( unsigned char* cFilename )
{

	int *slice_picked = NULL;
	slice_picked = (int *)malloc( mCavassData->m_zSize *sizeof(int));
	if (slice_picked == NULL)
		return -1;

	for(int i=0; i<mCavassData->m_zSize; i++)
		slice_picked[i] = 0;

	long item = -1;
    for ( ;; )
    {
        item = mListCtrl->GetNextItem(item,
                                     wxLIST_NEXT_ALL,
                                     wxLIST_STATE_SELECTED);
        if ( item == -1 )
            break;

		slice_picked[item] = 1;        
        wxLogMessage("Item %ld is selected.", item);
    }

	int nSelectedItemNum = mListCtrl->GetSelectedItemCount();
	if( nSelectedItemNum == 0 )
	{
		free( slice_picked );
		slice_picked = NULL;
		return 0;
	}

	int extent[2][2];
	extent[0][0] = extent[1][0] = 0;
	extent[0][1] = mCavassData->m_zSize-1; 	extent[1][1] = 0;
	int j;
	char *range_string = NULL;

	range_string = (char *)malloc(5*(extent[0][1]-extent[0][0]+2)+1);
	if (range_string == NULL)
		return (-1);
	range_string[0] = 0;
	for (j=extent[0][0]; j<=extent[0][1]; j++)
		if (slice_picked[j] && (j==extent[0][0] || !slice_picked[j-1]))
			sprintf(range_string+strlen(range_string), "%d/", j);
	range_string[strlen(range_string)-1] = ' ';
	for (j=extent[0][0]; j<=extent[0][1]; j++)
		if (slice_picked[j] && (j==extent[0][1] || !slice_picked[j+1]))
		{
			assert((int)strlen(range_string)+5 <
			    5*(extent[0][1]-extent[0][0]+2)+1);
			sprintf(range_string+strlen(range_string), "%d/", j);
		}
	range_string[strlen(range_string)-1] = 0;	

	wxString  voiCmd("\"");
	voiCmd += Preferences::getHome()+wxString("/ndvoi\" \"")+
		mCavassData->m_fname+"\" \""+
		cFilename+"\" 0 0 0 "+wxString::Format("%d %d",
		mCavassData->m_xSize, mCavassData->m_ySize)+" 0 0 "+range_string;
	printf("%s\n", (const char *)voiCmd.c_str());

	wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
	ProcessManager  q( "ndvoi running...", voiCmd );

	if( m_nSH0Type )
	{
		int no_ioi = 1;
		double SHI_voxel_size = pow((double)mCavassData->m_xSize*mCavassData->m_vh.scn.xypixsz[0] * mCavassData->m_ySize*mCavassData->m_vh.scn.xypixsz[1]*
			getNoSlices(0), (double)1/3.)/64;

		int low = 0; //min_table[interval];
		int high = getMax(0); //max_table[interval];
		float sh_low, sh_high;

		/*@ Check for equal slice spacing */
		int j, slices_checked;
		float last_loc=0, min_space, max_space;

		min_space = max_space = 0;
		slices_checked = 0;
		for (j=extent[0][0]; j<=extent[0][1]; j++)
		{
			if (slice_picked==NULL || slice_picked[j])
			{
				if (slices_checked)
				{
					if (slices_checked == 1)
						min_space = max_space = mCavassData->m_vh.scn.loc_of_subscenes[j]-last_loc;
					else
						if (mCavassData->m_vh.scn.loc_of_subscenes[j]-last_loc < min_space)
							min_space = mCavassData->m_vh.scn.loc_of_subscenes[j]-last_loc;
						else if (mCavassData->m_vh.scn.loc_of_subscenes[j]-last_loc > max_space)
							max_space = mCavassData->m_vh.scn.loc_of_subscenes[j]-last_loc;
				}
				last_loc = mCavassData->m_vh.scn.loc_of_subscenes[j]; //sl.location3[0][j];
				slices_checked++;
			}
		}
		if (min_space <= .99*max_space)
		{
			wxLogMessage("Slices must have equal spacing for SH0 output.");			
			free(range_string);			
			return(-1);
		}


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

	if( range_string != NULL )
		free( range_string );
	range_string = NULL;

	if (q.getCancel())    return 0;
#if 0
	int  error = q.getStatus();
	if (error != 0) {  wxMessageBox( "ndvoi failed." );  return 0;  }
#endif

	return 1;
}

bool VOIPickSlicesCanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int VOIPickSlicesCanvas::getCenter ( const int which ) const 
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
int VOIPickSlicesCanvas::getMax ( const int which ) const 
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
int VOIPickSlicesCanvas::getMin ( const int which ) const 
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
int VOIPickSlicesCanvas::getNoSlices ( const int which ) const {
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

bool VOIPickSlicesCanvas::getVOIPickSlicesDone (void) const
{
	return m_bVOIPickSlicesDone;
}
//----------------------------------------------------------------------
double VOIPickSlicesCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int VOIPickSlicesCanvas::getSliceNo ( const int which ) const {
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
int VOIPickSlicesCanvas::getWidth ( const int which ) const 
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
bool VOIPickSlicesCanvas::getInvert ( const int which ) const 
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
void VOIPickSlicesCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::setCenter ( const int which, const int center ) 
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
void VOIPickSlicesCanvas::setInvert ( const int which, const bool invert ) 
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
void VOIPickSlicesCanvas::setScale   ( const double scale )  {  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    m_scale = scale;
	//mCavassData->m_scale = scale;
    int  w, h;
    GetSize( &w, &h );
    m_cols  = (int)(w / (mOverallXSize * m_scale));
    m_rows  = (int)(h / (mOverallYSize * m_scale));
    if (m_cols<1)  m_cols=1;
    if (m_rows<1)  m_rows=1;
    reload();

//	ChangeVOIPickSlices();
}
//----------------------------------------------------------------------
void VOIPickSlicesCanvas::setSliceNo ( const int which, const int sliceNo ) {
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
void VOIPickSlicesCanvas::setWidth ( const int which, const int width ) 
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
IMPLEMENT_DYNAMIC_CLASS ( VOIPickSlicesCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( VOIPickSlicesCanvas, wxPanel )
    EVT_PAINT(            VOIPickSlicesCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           VOIPickSlicesCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        VOIPickSlicesCanvas::OnLeftDown     )
    EVT_LEFT_UP(          VOIPickSlicesCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      VOIPickSlicesCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        VOIPickSlicesCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       VOIPickSlicesCanvas::OnRightDown    )
    EVT_RIGHT_UP(         VOIPickSlicesCanvas::OnRightUp      )
    EVT_CHAR(             VOIPickSlicesCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
