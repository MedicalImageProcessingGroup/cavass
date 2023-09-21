/*
  Copyright 2023 Medical Image Processing Group
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
 * \file:  ToSceneCanvas.cpp
 * \brief  ToSceneCanvas class implementation
 *
 * Copyright 2013 University of Pennsylvania
 *
 */
//======================================================================
#include  "cavass.h"
#include  "ToSceneCanvas.h"

using namespace std;
//----------------------------------------------------------------------
ToSceneCanvas::ToSceneCanvas ( void )  {  puts("ToSceneCanvas()");  }
//----------------------------------------------------------------------
ToSceneCanvas::ToSceneCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
{
    m_scale          = 1.0;
    m_overlay        = true;
	m_bLayout        = false;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 10;
	m_nROIType = 0;
    m_nGradType = 0;
	m_timeInstances = 2;
	m_nRefFrame = 1;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
	m_bToSceneDone = false;	

	surf = NULL;
	strDisplay = NULL;
	mListCtrl =NULL;
}
//----------------------------------------------------------------------
ToSceneCanvas::~ToSceneCanvas ( void ) {
    cout << "ToSceneCanvas::~ToSceneCanvas" << endl;
    wxLogMessage( "ToSceneCanvas::~ToSceneCanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
	
	if( m_sliceIn != NULL )
	{
		delete m_sliceIn;
		m_sliceIn = NULL;
	}
	if( m_sliceOut != NULL )
	{
		delete m_sliceOut;
		m_sliceOut = NULL;
	}

	free( surf );
	surf = NULL;
	//ToSceneRelease();	
}
//----------------------------------------------------------------------
void ToSceneCanvas::release ( void ) {
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void ToSceneCanvas::loadFile ( const char* const fn ) 
{
}
//----------------------------------------------------------------------
void ToSceneCanvas::initLUT ( const int which ) {
}
//----------------------------------------------------------------------
void ToSceneCanvas::CreateDisplayImage(int which)
{	
}
//----------------------------------------------------------------------
void ToSceneCanvas::reload ( void ) 
{  
}

//----------------------------------------------------------------------
void ToSceneCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void ToSceneCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "ToSceneCanvas::OnChar" << endl;
    wxLogMessage( "ToSceneCanvas::OnChar" );
    if (e.m_keyCode==' ') {
        if (isLoaded(1)) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    }
}
//----------------------------------------------------------------------
void ToSceneCanvas::OnMouseMove ( wxMouseEvent& e ) 
{   
   
}
//----------------------------------------------------------------------
void ToSceneCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events	
}
//----------------------------------------------------------------------
void ToSceneCanvas::OnRightUp ( wxMouseEvent& e ) 
{
}
//----------------------------------------------------------------------
void ToSceneCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(1)) 
	{
        mCavassData->mNext->mDisplay = false;
        reload();
    }	
}
//----------------------------------------------------------------------
void ToSceneCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;
        reload();
    }
}

//----------------------------------------------------------------------
void ToSceneCanvas::OnLeftDown ( wxMouseEvent& e ) 
{
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
	wxClientDC  dc(this);
	PrepareDC(dc);
 
	if( m_bLayout )  //
	{
		const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
		const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
		lastX = wx;
		lastY = wy;
		SetCursor( wxCursor(wxCURSOR_HAND) );
	}
	else
	{

	}

}
//----------------------------------------------------------------------
void ToSceneCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void ToSceneCanvas::OnPaint ( wxPaintEvent& e ) 
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
void ToSceneCanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	
	if( m_bToSceneDone )
	{
		for( int i=0; i<NUM_FEATURE_LINES; i++ )
		{
			dc->DrawText( m_DisplayStr[i], m_tx, m_ty+i*20 );
		}

	}
}


//----------------------------------------------------------------------
void ToSceneCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool ToSceneCanvas::isLoaded ( const int which ) const {
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

bool ToSceneCanvas::getToSceneDone (void) const
{
	return m_bToSceneDone;
}
//----------------------------------------------------------------------

//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( ToSceneCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( ToSceneCanvas, wxPanel )
    EVT_PAINT(            ToSceneCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           ToSceneCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        ToSceneCanvas::OnLeftDown     )
    EVT_LEFT_UP(          ToSceneCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      ToSceneCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        ToSceneCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       ToSceneCanvas::OnRightDown    )
    EVT_RIGHT_UP(         ToSceneCanvas::OnRightUp      )
    EVT_CHAR(             ToSceneCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
