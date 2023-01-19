/*
  Copyright 1993-2011, 2016 Medical Image Processing Group
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
 * \file:  FromDicomCanvas.cpp
 * \brief  FromDicomCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

using namespace std;

//----------------------------------------------------------------------
FromDicomCanvas::FromDicomCanvas ( void )  {  puts("FromDicomCanvas()");  }
//----------------------------------------------------------------------
FromDicomCanvas::FromDicomCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
	mListCtrl = NULL;
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
}
//----------------------------------------------------------------------
FromDicomCanvas::~FromDicomCanvas ( void ) {
    cout << "FromDicomCanvas::~FromDicomCanvas" << endl;
    wxLogMessage( "FromDicomCanvas::~FromDicomCanvas" );
    release(); /* mCanvassData released in MainCanvas */
}
//----------------------------------------------------------------------
void FromDicomCanvas::release ( void ) {
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void FromDicomCanvas::reload ( void ) {
	Refresh();
}

//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void FromDicomCanvas::OnChar ( wxKeyEvent& e ) {
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void FromDicomCanvas::OnRightDown ( wxMouseEvent& e ) {
#if 0
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(0)) {
        mCavassData->mDisplay = false;  //!mCavassData->mDisplay;
        reload();
    }
#else
    //gjg: simulate the middle button (useful on a 2-button mouse)
    OnMiddleDown( e );
#endif
}
//----------------------------------------------------------------------
void FromDicomCanvas::OnRightUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
// 7/26/07 Dewey Odhner
void FromDicomCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
}
//----------------------------------------------------------------------
void FromDicomCanvas::OnMiddleUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
void FromDicomCanvas::OnLeftDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
}
//----------------------------------------------------------------------
void FromDicomCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void FromDicomCanvas::OnPaint ( wxPaintEvent& e ) {
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

    wxPaintDC  dc(this);
    PrepareDC(dc);
    //dc.BeginDrawing();  deprecated
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows

    paint( &dc );
    //dc.EndDrawing();  deprecated
}
//----------------------------------------------------------------------
void FromDicomCanvas::paint ( wxDC* dc ) {

}

//----------------------------------------------------------------------
void FromDicomCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( FromDicomCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( FromDicomCanvas, wxPanel )
    EVT_PAINT(            FromDicomCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        FromDicomCanvas::OnLeftDown     )
    EVT_LEFT_UP(          FromDicomCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      FromDicomCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        FromDicomCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       FromDicomCanvas::OnRightDown    )
    EVT_RIGHT_UP(         FromDicomCanvas::OnRightUp      )
    EVT_CHAR(             FromDicomCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
