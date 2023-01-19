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
 * \file:  VOIROICanvas.cpp
 * \brief  VOIROICanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "VOIROIFrame.h"
#include  "VOIROICanvas.h"

using namespace std;
//----------------------------------------------------------------------
VOIROICanvas::VOIROICanvas ( void )  {  puts("VOIROICanvas()");  }
//----------------------------------------------------------------------
VOIROICanvas::VOIROICanvas ( wxWindow* parent, MainFrame* parent_frame,
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

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
	m_bVOIROIDone = false;	

	m_bMouseMove = false;


	m_nStartSlice = 1;
	m_nEndSlice = 2;
	m_nStartVolume = 1;
	m_nEndVolume = 1;
	m_nROIWidth = 1;
	m_nROIHeight = 1;
    m_nROIL = 0;
	m_nROIR = 0;
	m_nROIT = 0;
	m_nROIB = 0;
	m_oROIWidth = 1;
	m_oROIHeight = 1;
    m_oROIL = 0;
	m_oROIT = 0;

	m_bLocOrSize = true;
	m_bROI = false;
}
//----------------------------------------------------------------------
VOIROICanvas::~VOIROICanvas ( void ) {
    cout << "VOIROICanvas::~VOIROICanvas" << endl;
    wxLogMessage( "VOIROICanvas::~VOIROICanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
	
}
//----------------------------------------------------------------------
void VOIROICanvas::release ( void ) {
}
//----------------------------------------------------------------------
void VOIROICanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void VOIROICanvas::loadFile ( const char* const fn ) 
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
		m_scale = (double)((w-150)*1.0 / (mOverallXSize * 2));     
		double yScale = (double)(h-100)*1.0/mCavassData->m_ySize;
		if( yScale < m_scale )
			m_scale = yScale;
		m_nStartSlice = 1;
		m_nStartVolume = 1;
		if (mCavassData->m_vh.scn.dimension == 4)
		{
			m_nEndVolume = mCavassData->m_vh.scn.num_of_subscenes[0];
			m_nEndSlice = 0;
			for (int v=0; v<m_nEndVolume; v++)
				if (mCavassData->m_vh.scn.num_of_subscenes[1+v] > m_nEndSlice)
					m_nEndSlice = mCavassData->m_vh.scn.num_of_subscenes[1+v];
		}
		else
		{
			m_nEndSlice = mCavassData->m_zSize;
			m_nEndVolume = 1;
		}
		m_nROIWidth = mCavassData->m_xSize;
		m_nROIHeight = mCavassData->m_ySize;
		m_nROIL = 0;
		m_nROIR = m_nROIWidth-1;
		m_nROIT = 0;
		m_nROIB = m_nROIHeight-1;
		m_oROIWidth = m_nROIWidth;
		m_oROIHeight = m_nROIHeight;
		m_oROIL = m_nROIL;
		m_oROIT = m_nROIT;
	}       

    reload();    
    SetCursor( *wxSTANDARD_CURSOR );

}
//----------------------------------------------------------------------
void VOIROICanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}
//----------------------------------------------------------------------
void VOIROICanvas::CreateDisplayImage(int which)
{
	//note: image data is 24-bit rgb
	CavassData&  A = *mCavassData;
	CavassData&  B = *(mCavassData->mNext);

	int winWidth = m_nROIR-m_nROIL+1;
	int winHeight = m_nROIB-m_nROIT+1;
	unsigned char*  slice = NULL;

	if( which == 1 )
		slice = (unsigned char *)malloc(winWidth*winHeight*3);
	else
		slice = (unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
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
	}
	else
	{	
		if (A.m_size==1) {
			unsigned char*  ucData = (unsigned char*)(A.getSlice( A.m_sliceNo )); //(A.m_data);			
			for(int i=0, n = m_nROIT; n <= m_nROIB; n++)
			{
				for( int m = m_nROIL; m <= m_nROIR; m++)
				{
					slice[i] = slice[i+1] = slice[i+2] = B.m_lut[ucData[n*A.m_xSize+m]-B.m_min];
					i+=3;
				}
			}			
		} else if (A.m_size==2) {
			unsigned short* sData = (unsigned short*)(A.getSlice( A.m_sliceNo )); //(A.m_data);
			for(int i=0, n = m_nROIT; n <= m_nROIB; n++)
			{
				for( int m = m_nROIL; m <= m_nROIR; m++)
				{
					slice[i] = slice[i+1] = slice[i+2] = B.m_lut[sData[n*A.m_xSize+m]-B.m_min];
					i+=3;
				}
			}			
		} else if (A.m_size==4) {
			int*  iData = (int*)(A.getSlice( A.m_sliceNo ));    //(A.m_data);
			for(int i=0, n = m_nROIT; n <= m_nROIB; n++)
			{
				for( int m = m_nROIL; m <= m_nROIR; m++)
				{
					slice[i] = slice[i+1] = slice[i+2] = B.m_lut[iData[n*A.m_xSize+m]-B.m_min];
					i+=3;
				}
			}			
		}	
	}
	

	if(which==0)
	{
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
	else if(which==1)
	{
		if(m_images[1]!=NULL)
			m_images[1]->Destroy();

		m_images[1] = new wxImage( winWidth, winHeight, slice );
		double win_scale = A.m_xSize*m_scale/winWidth;
		if( win_scale > A.m_ySize*m_scale/winHeight )
			win_scale = A.m_ySize*m_scale/winHeight;
		if (win_scale!=1.0) 
		{
			m_images[1]->Rescale( (int)(ceil(win_scale*winWidth)),
							  (int)(ceil(win_scale*winHeight)) );
		}
		m_bitmaps[1] = new wxBitmap( *m_images[1] );
		
	}
	//delete [] slice;
}

//----------------------------------------------------------------------
void VOIROICanvas::reload ( void ) 
{
  //if (!isLoaded(0) || !isLoaded(1))    return;
    freeImagesAndBitmaps();

	CreateDisplayImage(0);

	if(m_bVOIROIDone) /* update VOIROIing */		
		CreateDisplayImage(1);

    Refresh();
}

//----------------------------------------------------------------------
void VOIROICanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}

//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void VOIROICanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "VOIROICanvas::OnChar" << endl;
    wxLogMessage( "VOIROICanvas::OnChar" );
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
void VOIROICanvas::OnMouseMove ( wxMouseEvent& e ) 
{
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    long  wx = (long)((dc.DeviceToLogicalX( pos.x ) - m_tx)*1.0/m_scale);
    long  wy = (long)((dc.DeviceToLogicalY( pos.y ) - m_ty)*1.0/m_scale);

	if( m_bROI ) //&& !m_bVOIROIDone )
	{	
		if ( e.LeftIsDown() )
			return;

		if (wx >= mCavassData->m_xSize)
			wx = mCavassData->m_xSize-1;
		if (wy < 0)
			wy = 0;
		if( m_bLocOrSize == false )  // location
		{
			if (wx < m_nROIR-m_nROIL)
				wx = m_nROIR-m_nROIL;
			if (wy >= mCavassData->m_ySize-m_nROIB+m_nROIT)
				wy = mCavassData->m_ySize-m_nROIB+m_nROIT-1;
			if (wx==m_nROIR && wy==m_nROIT)
				return;
			m_nROIL += wx-m_nROIR;
			m_nROIB += wy-m_nROIT;
		}
		else   // size
		{
			if (wx <= m_nROIL)
				wx = m_nROIL+1;
			if (wy >= m_nROIB)
				wy = m_nROIB-1;
			if (wx==m_nROIR && wy==m_nROIT)
				return;
		}
		m_nROIR = wx;
		m_nROIT = wy;
		m_nROIWidth = m_nROIR-m_nROIL+1;
		m_nROIHeight = m_nROIB-m_nROIT+1;
	}
	else
	{		
	}

	Refresh();
}
//----------------------------------------------------------------------
void VOIROICanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
	m_nROIL = m_oROIL;
	m_nROIR = m_oROIL+m_oROIWidth-1;
	m_nROIT = m_oROIT;
	m_nROIB = m_oROIT+m_oROIHeight-1;
	m_nROIWidth = m_oROIWidth;
	m_nROIHeight = m_oROIHeight;
	m_bROI = false;
	reload();
	m_parent_frame->SetStatusText("Loc/Size", 2);
	VOIROIFrame *mf = dynamic_cast<VOIROIFrame*>( m_parent_frame );
	mf->updateROITablets();
}
//----------------------------------------------------------------------
void VOIROICanvas::OnRightUp ( wxMouseEvent& e ) 
{
}
//----------------------------------------------------------------------
void VOIROICanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
	m_oROIWidth = m_nROIR-m_nROIL+1;
	m_oROIHeight = m_nROIB-m_nROIT+1;
	m_oROIL = m_nROIL;
	m_oROIT = m_nROIT;
	m_bROI = false;
	m_bVOIROIDone = true;
	reload();
	VOIROIFrame *mf = dynamic_cast<VOIROIFrame*>( m_parent_frame );
	mf->updateROITablets();
	mf->SetStatusText("Loc/Size", 2);
}
//----------------------------------------------------------------------
void VOIROICanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
}

//----------------------------------------------------------------------
void VOIROICanvas::OnLeftDown ( wxMouseEvent& e ) 
{
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
	wxClientDC  dc(this);
	PrepareDC(dc);
 
	if( m_bROI )
	{	
		m_bLocOrSize = !m_bLocOrSize; // false: location; true:size
	}
	else
	{
		m_bROI = true;		
	}
	m_parent_frame->SetStatusText(m_bLocOrSize? "SIZE/Loc": "LOC/Size", 2);

	OnMouseMove(e);

}
//----------------------------------------------------------------------
void VOIROICanvas::OnLeftUp ( wxMouseEvent& e ) 
{
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void VOIROICanvas::OnPaint ( wxPaintEvent& e ) 
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
void VOIROICanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	if(m_bitmaps[0]!=NULL)
	{
		wxString s;
		if (mCavassData->m_tSize == 1)
		    s = wxString::Format("%d/%d", getSliceNo(0)+1, getNoSlices(0));
		else
		{
			int volume, slice_of_volume;
			mCavassData->getVolumeAndSlice(volume, slice_of_volume,
				getSliceNo(0));
			s = wxString::Format( "(%d, %d)/(%d, %d)", volume+1,
				slice_of_volume+1, mCavassData->m_tSize,
				mCavassData->m_vh.scn.num_of_subscenes[1+volume]);
		}
		if (m_bitmaps[0]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[0], m_tx, m_ty );
			if(m_overlay)
				dc->DrawText( s, m_tx, m_ty );
		}		

		int device_ROIL, device_ROIR, device_ROIT, device_ROIB;
		device_ROIL = dc->LogicalToDeviceX( (int)(m_nROIL*m_scale + m_tx));
		device_ROIR = dc->LogicalToDeviceX( (int)((m_nROIR+1)*m_scale-1 + m_tx));
		device_ROIT = dc->LogicalToDeviceX( (int)(m_nROIT*m_scale + m_ty));
		device_ROIB = dc->LogicalToDeviceX( (int)((m_nROIB+1)*m_scale-1 + m_ty));

		dc->DrawLine( device_ROIL, device_ROIT, device_ROIR, device_ROIT);
		dc->DrawLine( device_ROIR, device_ROIT, device_ROIR, device_ROIB);
		dc->DrawLine( device_ROIR, device_ROIB, device_ROIL, device_ROIB);
		dc->DrawLine( device_ROIL, device_ROIB, device_ROIL, device_ROIT);

		if (m_bVOIROIDone && m_bitmaps[1]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[1], m_tx+w/2, m_ty );
			if(m_overlay)
				dc->DrawText( s, m_tx + w/2, m_ty );
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
void VOIROICanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool VOIROICanvas::isLoaded ( const int which ) const {
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

int VOIROICanvas::SaveProfile ( const char* cFilename )
{
	wxString voiCmd("\"");
	voiCmd += Preferences::getHome()+wxString::Format(
		"/ndvoi\" \"%s\" \"%s\" %d %d %d %d %d %d %d %d %d %d %d",
		mCavassData->m_fname, cFilename, 0, m_nROIL, m_nROIT,
		m_nROIWidth, m_nROIHeight, 0, 0, m_nStartSlice-1, m_nEndSlice-1,
		m_nStartVolume-1, m_nEndVolume-1 );
	printf("%s\n", (const char *)voiCmd.c_str());
	wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
	ProcessManager  q( "ndvoi running...", voiCmd );
	if (q.getCancel())    return 0;
#if 0
	int  error = q.getStatus();
	if (error != 0) {  wxMessageBox( "ndvoi failed." );  return 0;  }
#endif

	return 1;
}

bool VOIROICanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int VOIROICanvas::getCenter ( const int which ) const 
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
int VOIROICanvas::getMax ( const int which ) const 
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
int VOIROICanvas::getMin ( const int which ) const 
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
int VOIROICanvas::getNoSlices ( const int which ) const {
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

bool VOIROICanvas::getVOIROIDone (void) const
{
	return m_bVOIROIDone;
}
//----------------------------------------------------------------------
double VOIROICanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int VOIROICanvas::getSliceNo ( const int which ) const {
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
int VOIROICanvas::getWidth ( const int which ) const 
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
bool VOIROICanvas::getInvert ( const int which ) const 
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
void VOIROICanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void VOIROICanvas::setCenter ( const int which, const int center ) 
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
void VOIROICanvas::setInvert ( const int which, const bool invert ) 
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
void VOIROICanvas::setScale   ( const double scale )  {  //aka magnification
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

}
//----------------------------------------------------------------------
void VOIROICanvas::setSliceNo ( const int which, const int sliceNo ) {
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
void VOIROICanvas::setWidth ( const int which, const int width ) 
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
void VOIROICanvas::setLocX(int nLocX)
{
	m_nROIL = nLocX;
	if( m_nROIL +m_nROIWidth-1 < mCavassData->m_xSize )
		m_nROIR = m_nROIL +m_nROIWidth-1;
	else
	{
		m_nROIWidth = mCavassData->m_xSize - m_nROIL;
		m_nROIR = mCavassData->m_xSize-1;
	}
	m_nROIWidth = m_nROIR-m_nROIL+1;
	m_oROIWidth = m_nROIWidth;
	m_oROIHeight = m_nROIHeight;
	m_oROIL = m_nROIL;
	m_oROIT = m_nROIT;
}
void VOIROICanvas::setLocY(int nLocY)
{
	m_nROIT = nLocY;
	if( m_nROIT +m_nROIHeight-1 < mCavassData->m_ySize )
		m_nROIB = m_nROIT +m_nROIHeight-1;
	else
	{
		m_nROIHeight = mCavassData->m_ySize - m_nROIT;
		m_nROIB = mCavassData->m_ySize-1;
	}
	m_nROIHeight = m_nROIB-m_nROIT+1;
	m_oROIWidth = m_nROIWidth;
	m_oROIHeight = m_nROIHeight;
	m_oROIL = m_nROIL;
	m_oROIT = m_nROIT;
}
void VOIROICanvas::setWinWidth(int nWinWidth)
{
	m_nROIWidth = nWinWidth;
	if( m_nROIL +m_nROIWidth-1 < mCavassData->m_xSize )
		m_nROIR = m_nROIL +m_nROIWidth-1;
	else if( m_nROIR -m_nROIWidth+1 >= 0 )
		m_nROIL = m_nROIR -m_nROIWidth+1;
	else
	{
		m_nROIL = 0;
		m_nROIR = m_nROIL +m_nROIWidth-1;
	}
	m_oROIWidth = m_nROIWidth;
	m_oROIHeight = m_nROIHeight;
	m_oROIL = m_nROIL;
	m_oROIT = m_nROIT;
}
void VOIROICanvas::setWinHeight(int nWinHeight)
{
	m_nROIHeight = nWinHeight;
	if( m_nROIT +m_nROIHeight-1 < mCavassData->m_ySize )
		m_nROIB = m_nROIT +m_nROIHeight-1;
	else if( m_nROIB -m_nROIHeight+1 >= 0 )
		m_nROIT = m_nROIB -m_nROIHeight+1;
	else
	{
		m_nROIT = 0;
		m_nROIB = m_nROIT + m_nROIHeight-1;
	}
	m_oROIWidth = m_nROIWidth;
	m_oROIHeight = m_nROIHeight;
	m_oROIL = m_nROIL;
	m_oROIT = m_nROIT;
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( VOIROICanvas, wxPanel )
BEGIN_EVENT_TABLE       ( VOIROICanvas, wxPanel )
    EVT_PAINT(            VOIROICanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           VOIROICanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        VOIROICanvas::OnLeftDown     )
    EVT_LEFT_UP(          VOIROICanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      VOIROICanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        VOIROICanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       VOIROICanvas::OnRightDown    )
    EVT_RIGHT_UP(         VOIROICanvas::OnRightUp      )
    EVT_CHAR(             VOIROICanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
