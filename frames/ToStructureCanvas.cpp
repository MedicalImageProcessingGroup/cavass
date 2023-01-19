/*
  Copyright 1993-2013, 2015-2016 Medical Image Processing Group
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
 * \file:  ToStructureCanvas.cpp
 * \brief  ToStructureCanvas class implementation
 *
 * Copyright 2013 University of Pennsylvania
 *
 */
//======================================================================
#include  "cavass.h"
#include  "ToStructureCanvas.h"

using namespace std;
//----------------------------------------------------------------------
ToStructureCanvas::ToStructureCanvas ( void )  {  puts("ToStructureCanvas()");  }
//----------------------------------------------------------------------
ToStructureCanvas::ToStructureCanvas ( wxWindow* parent, MainFrame* parent_frame,
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
	m_bToStructureDone = false;	

	surf = NULL;
	strDisplay = NULL;
	mListCtrl =NULL;
}
//----------------------------------------------------------------------
ToStructureCanvas::~ToStructureCanvas ( void ) {
    cout << "ToStructureCanvas::~ToStructureCanvas" << endl;
    wxLogMessage( "ToStructureCanvas::~ToStructureCanvas" );
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
	//ToStructureRelease();	
}
//----------------------------------------------------------------------
void ToStructureCanvas::release ( void ) {
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void ToStructureCanvas::loadFile ( const char* const fn ) 
{
}
//----------------------------------------------------------------------
void ToStructureCanvas::initLUT ( const int which ) {
}
//----------------------------------------------------------------------
void ToStructureCanvas::CreateDisplayImage(int which)
{	
}
//----------------------------------------------------------------------
void ToStructureCanvas::reload ( void ) 
{  
}

//----------------------------------------------------------------------
void ToStructureCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void ToStructureCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "ToStructureCanvas::OnChar" << endl;
    wxLogMessage( "ToStructureCanvas::OnChar" );
    if (e.m_keyCode==' ') {
        if (isLoaded(1)) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    }
}
//----------------------------------------------------------------------
void ToStructureCanvas::OnMouseMove ( wxMouseEvent& e ) 
{   
   
}
//----------------------------------------------------------------------
void ToStructureCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events	
}
//----------------------------------------------------------------------
void ToStructureCanvas::OnRightUp ( wxMouseEvent& e ) 
{
}
//----------------------------------------------------------------------
void ToStructureCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(1)) 
	{
        mCavassData->mNext->mDisplay = false;
        reload();
    }	
}
//----------------------------------------------------------------------
void ToStructureCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;
        reload();
    }
}

//----------------------------------------------------------------------
void ToStructureCanvas::OnLeftDown ( wxMouseEvent& e ) 
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
void ToStructureCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void ToStructureCanvas::OnPaint ( wxPaintEvent& e ) 
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
void ToStructureCanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	
	if( m_bToStructureDone )
	{
		for( int i=0; i<NUM_FEATURE_LINES; i++ )
		{
			dc->DrawText( m_DisplayStr[i], m_tx, m_ty+i*20 );
		}

	}
}


//----------------------------------------------------------------------
void ToStructureCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool ToStructureCanvas::isLoaded ( const int which ) const {
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

bool ToStructureCanvas::getToStructureDone (void) const
{
	return m_bToStructureDone;
}
//----------------------------------------------------------------------

//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( ToStructureCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( ToStructureCanvas, wxPanel )
    EVT_PAINT(            ToStructureCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           ToStructureCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        ToStructureCanvas::OnLeftDown     )
    EVT_LEFT_UP(          ToStructureCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      ToStructureCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        ToStructureCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       ToStructureCanvas::OnRightDown    )
    EVT_RIGHT_UP(         ToStructureCanvas::OnRightUp      )
    EVT_CHAR(             ToStructureCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
