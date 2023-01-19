/*
  Copyright 1993-2010, 2015-2016 Medical Image Processing Group
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
 * \file:  ExampleCanvas.cpp
 * \brief  ExampleCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ExampleFrame.h"
#include  "ExampleCanvas.h"
#include  "ChunkData.h"
#include  <limits.h>

using namespace std;
//----------------------------------------------------------------------
const int  ExampleCanvas::sSpacing=1;  ///< space, in pixels, between each slice (on the screen in the frame in the canvas)
//----------------------------------------------------------------------
/** \brief ExampleCanvas ctor. */
ExampleCanvas::ExampleCanvas ( void ) {
    init();
}
//----------------------------------------------------------------------
/** \brief ExampleCanvas ctor. */
ExampleCanvas::ExampleCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    init();
}
//----------------------------------------------------------------------
/** \brief initialize members. */
void ExampleCanvas::init ( void ) {
    mFileOrDataCount = 0;
    mScale           = 1.0;
    mOverlay         = true;
    mRows            = 0;
    mCols            = 0;
    mImages          = (wxImage**)NULL;
    mBitmaps         = (wxBitmap**)NULL;
    mTx = mTy        = 0;
    mXSize = mYSize  = mZSize = 0;
    mLastX = mLastY  = -1;
}
//----------------------------------------------------------------------
/** \brief ExampleCanvas dtor. */
ExampleCanvas::~ExampleCanvas ( void ) {
    cout << "ExampleCanvas::~ExampleCanvas" << endl;
    wxLogMessage( "ExampleCanvas::~ExampleCanvas" );
    release();
}
//----------------------------------------------------------------------
/** \brief release memory allocated to this object. */
void ExampleCanvas::release ( void ) {
    while (mCavassData!=NULL) {
        CavassData*  next = mCavassData->mNext;
        delete mCavassData;
        mCavassData = next;
    }
    freeImagesAndBitmaps();
    init();
}
//----------------------------------------------------------------------
/** \brief load data from memory. */
void ExampleCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    assert( mFileOrDataCount==0 );
    CavassData*  cd = new CavassData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    assert( mCavassData==NULL );
    mCavassData = cd;
    mFileOrDataCount = 1;

    CavassData&  A = *mCavassData;
    A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
    mXSize = A.m_xSize;
    mYSize = A.m_ySize;
    mZSize = A.m_zSize;
    mCols = mRows = 1;
    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief load data from a file.
 *  \param fn input data file name.
 */
void ExampleCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount==0 );
    ChunkData*  cd = new ChunkData( fn );
    assert( cd!=NULL );
	if ((!cd->mIsCavassFile || !cd->m_vh_initialized) && !cd->mIsDicomFile &&
			!cd->mIsImageFile)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
    mCavassData = cd;
    mFileOrDataCount = 1;

    CavassData&  A = *mCavassData;
    A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
    mXSize = A.m_xSize;
    mYSize = A.m_ySize;
    mZSize = A.m_zSize;
    mCols  = mRows = 1;
    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief initialize the specified lookup table.
 *  \param which specifies the particular data set (if more than 1 read).
 */
void ExampleCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else             mCavassData->mNext->initLUT();
}
//----------------------------------------------------------------------
/** \brief free any allocated images (of type wxImage) and/or bitmaps
 *  (of type wxBitmap)
 */
void ExampleCanvas::freeImagesAndBitmaps ( void ) {
    if (mImages!=NULL) {
        for (int i=0; i<mRows*mCols; i++) {
            if (mImages[i]!=NULL) {
                delete mImages[i];
                mImages[i]=NULL;
            }
        }
        free( mImages );
        mImages = NULL;
    }

    if (mBitmaps!=NULL) {
        for (int i=0; i<mRows*mCols; i++) {
            if (mBitmaps[i]!=NULL) {
                delete mBitmaps[i];
                mBitmaps[i]=NULL;
            }
        }
        free( mBitmaps );
        mBitmaps = NULL;
    }
}
//----------------------------------------------------------------------
/** \brief reload the drawable image data. */
void ExampleCanvas::reload ( void ) {
    if (!isLoaded(0))    return;
    freeImagesAndBitmaps();
    
    int  k;    
    mImages = (wxImage**)malloc( mCols * mRows * sizeof(wxImage*) );
    for (k=0; k<mCols*mRows; k++)    mImages[k]=NULL;
    
    mBitmaps = (wxBitmap**)malloc( mCols * mRows * sizeof(wxBitmap*) );
    for (k=0; k<mCols*mRows; k++)    mBitmaps[k]=NULL;
    
    CavassData&  A = *mCavassData;
    for (k=0; k<mCols*mRows; k++) {
        if (A.m_sliceNo+k >= A.m_zSize)    break;
        //note: image data are 24-bit rgb
        mImages[k] = new wxImage( mXSize, mYSize, ::toRGB(A) );
        //scale according to the pixel size (to maintain aspect ratio) and the overall scale setting
        if (mScale!=1.0 || A.m_xSpacing!=1.0 || A.m_ySpacing!=1.0) {
            const int  scaledW = (int)ceil( mXSize * A.m_xSpacing * mScale );
            const int  scaledH = (int)ceil( mYSize * A.m_ySpacing * mScale );
            mImages[k]->Rescale( scaledW, scaledH );
        }
        mBitmaps[k] = new wxBitmap( (const wxImage&) *mImages[k] );
    }
    Refresh();
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button. */
void ExampleCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "ExampleCanvas::OnChar" << endl;
    wxLogMessage( "ExampleCanvas::OnChar" );
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
/** \brief callback for mouse move events */
void ExampleCanvas::OnMouseMove ( wxMouseEvent& e ) {
    //if (m_data==NULL)    return;
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - mTx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - mTy;
    //if we are NOT in any mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    if (e.LeftIsDown() && !e.ShiftDown() && !e.ControlDown()) {
        if (mLastX==-1 || mLastY==-1) {
            SetCursor( wxCursor(wxCURSOR_HAND) );
            mLastX = wx;
            mLastY = wy;
            return;
        }
        
        bool  changed=false;
        if (abs(wx-mLastX)>2) {
            mTx += (wx-mLastX)/2;  // / 4 * 3;
            changed = true;
        } else if (abs(wx-mLastX)>2) {
            mTx -= (mLastX-wx)/2;  // / 4 * 3;
            changed = true;
        }
        if (abs(wy-mLastY)>2) {
            mTy += (wy-mLastY)/2;  // / 4 * 3;
            changed = true;
        } else if (abs(wy-mLastY)>2) {
            mTy -= (mLastY-wy)/2;  // / 4 * 3;
            changed = true;
        }
        
        if (changed)  Refresh();
        mLastX=wx;
        mLastY=wy;
        return;
    } else {
        if (mLastX!=-1 || mLastY!=-1) {
            SetCursor( *wxSTANDARD_CURSOR );
            mLastX = mLastY = -1;
        }
    }
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button down events. */
void ExampleCanvas::OnRightDown ( wxMouseEvent& e ) {
    cout << "OnRightDown" << endl;    wxLogMessage("OnRightDown");
    SetFocus();  //to regain/recapture keypress events
    //pass the event up to the parent frame
    ExampleFrame*  p = (ExampleFrame*)m_parent_frame;
    wxCommandEvent  unused;
    //simulate the middle button for a two button mouse
    if (e.AltDown() || e.ControlDown() || e.ShiftDown())
        p->OnPrevious( unused );  //middle button
    else
        p->OnNext( unused );  //right button
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button up events. */
void ExampleCanvas::OnRightUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button down events. */
void ExampleCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    cout << "OnMiddleDown" << endl;    wxLogMessage("OnMiddleDown");
    SetFocus();  //to regain/recapture keypress events
    //pass the event up to the parent frame
    ExampleFrame*  p = (ExampleFrame*)m_parent_frame;
    wxCommandEvent  unused;
    p->OnPrevious( unused );
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button up events. */
void ExampleCanvas::OnMiddleUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
/** \brief Callback to handle left mouse button down events. */
void ExampleCanvas::OnLeftDown ( wxMouseEvent& e ) {
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events

    //simulate the middle button for a two button mouse
    if (e.ShiftDown()) {
        //pass the event up to the parent frame
        ExampleFrame*   p = (ExampleFrame*)m_parent_frame;
        wxCommandEvent  unused;
        p->OnPrevious( unused );  //middle button
        return;
    }
    if (e.ControlDown()) {
        //pass the event up to the parent frame
        ExampleFrame*   p = (ExampleFrame*)m_parent_frame;
        wxCommandEvent  unused;
        p->OnNext( unused );  //right button
        return;
    }

    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
    const long  wx = dc.DeviceToLogicalX( pos.x ) - mTx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - mTy;
    mLastX = wx;
    mLastY = wy;
    SetCursor( wxCursor(wxCURSOR_HAND) );
}
//----------------------------------------------------------------------
/** \brief callback to handle left mouse button up events. */
void ExampleCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    mLastX = mLastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief callback for paint events. */
void ExampleCanvas::OnPaint ( wxPaintEvent& e ) {
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
    //dc.BeginDrawing();
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
    //dc.EndDrawing();
}
//----------------------------------------------------------------------
/** \brief called in response to paint, print, or copy to the clipboard. */
void ExampleCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
    
    if (mBitmaps!=NULL) {
        int  i=0;
        for (int r=0; r<mRows; r++) {
            //const int y = (int)(r*(m_ySize*m_scale+1)+0.5);
            const int  y = (int)(r*(ceil(mYSize*mScale)+sSpacing));
            for (int c=0; c<mCols; c++) {
                if (mBitmaps[i]!=NULL && mBitmaps[i]->Ok()) {
                    //const int x = (int)(c*(m_xSize*m_scale+1)+0.5);
                    const int  x = (int)(c*(ceil(mXSize*mScale)+sSpacing));
                    dc->DrawBitmap( *mBitmaps[i], x+mTx, y+mTy );
                    //show the overlay?  (the overlay consists of numbers that indicate the slice)
                    if (mOverlay) {
                        const int sliceA = mCavassData->m_sliceNo+i;
                        //in bounds?
                        if (mCavassData->inBounds(0,0,sliceA)) {
                            //in bounds
                            const wxString  s = wxString::Format( "(%d/%d)", sliceA+1, mZSize );
                            dc->DrawText( s, x+mTx, y+mTy );
                        } else {
                            //out of bounds
                            const wxString  s = wxString::Format( "(-/-)" );
                            dc->DrawText( s, x+mTx, y+mTy );
                        }
                    }
                }
                i++;
            }
        }
    } else if (m_backgroundLoaded) {
        int  w, h;
        dc->GetSize( &w, &h );
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}
//----------------------------------------------------------------------
/** \brief    determines if a particular data set has been loaded.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  true if the data set has been loaded; false otherwise.
 */
bool ExampleCanvas::isLoaded ( const int which ) const {
    if (which==0) {
        if (mCavassData==NULL)    return false;
        const CavassData&  cd = *mCavassData;
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return (cd.mIsCavassFile && cd.m_vh_initialized) ||
				cd.mIsDicomFile || cd.mIsImageFile;
        return false;
    } else if (which==1) {
        if (mCavassData==NULL || mCavassData->mNext==NULL)    return false;
        const CavassData&  cd = *(mCavassData->mNext);
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return (cd.mIsCavassFile && cd.m_vh_initialized) ||
				cd.mIsDicomFile || cd.mIsImageFile;
        return false;
    } else {
        assert( 0 );
    }
    return false;
}
//----------------------------------------------------------------------
/** \brief    get the current center contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the current center contrast setting value.
 */
int ExampleCanvas::getCenter ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_center;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the maximum value in a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the maximum value.
 */
int ExampleCanvas::getMax ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_max;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_max;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the minimum value in a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the minimum value.
 */
int ExampleCanvas::getMin ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_min;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_min;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the number of slices in the entire data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the number of slices in the entire data set.
 */
int ExampleCanvas::getNoSlices ( const int which ) const {
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
//----------------------------------------------------------------------
/** \brief    get the status of the overlay.
 *  \returns  true if overlay is on; false otherwise.
 */
bool ExampleCanvas::getOverlay ( void ) const {
    return mOverlay;
}
//----------------------------------------------------------------------
/** \brief    get the overall scale of the displayed image(s).
 *  \returns  the overall scale value.
 */
double ExampleCanvas::getScale ( void ) const {
    return mScale;
}
//----------------------------------------------------------------------
/** \brief    get the number of the first displayed slice.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the number of the first displayed slice.
 */
int ExampleCanvas::getSliceNo ( const int which ) const {
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
/** \brief    get the current width contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  the current width contrast setting value.
 */
int ExampleCanvas::getWidth ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_width;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the current setting of the contrast inversion state.
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  true if invert is on; false otherwise.
 */
bool ExampleCanvas::getInvert ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.mInvert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.mInvert;
    } else {
        assert( 0 );
    }
    return false;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the current blue emphasis value [0.0, ..., 1.0].
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  the current blue emphasis value.
 */
double ExampleCanvas::getB ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.getB();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.getB();
    } else {
        assert( 0 );
    }
    return 0.0;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the current green emphasis value [0.0, ..., 1.0].
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  the current green emphasis value.
 */
double ExampleCanvas::getG ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.getG();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.getG();
    } else {
        assert( 0 );
    }
    return 0.0;  //should never get here
}
//----------------------------------------------------------------------
/** \brief    get the current red emphasis value [0.0, ..., 1.0].
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  the current red emphasis value.
 */
double ExampleCanvas::getR ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.getR();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.getR();
    } else {
        assert( 0 );
    }
    return 0.0;  //should never get here
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
/** \brief  set the current blue emphasis value [0.0, ..., 1.0].
 *  \param  which specifies the particular data set (if more than one).
 *  \param  b the current blue emphasis value.
 */
void ExampleCanvas::setB ( const int which, const double b ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.setB( b );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.setB( b );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief  set the current center contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  center is the contrast setting value.
 */
void ExampleCanvas::setCenter ( const int which, const int center ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_center = center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_center = center;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief  set the current green emphasis value [0.0, ..., 1.0].
 *  \param  which specifies the particular data set (if more than one).
 *  \param  g the current green emphasis value.
 */
void ExampleCanvas::setG ( const int which, const double g ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.setG( g );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.setG( g );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief    set the current setting of the contrast inversion state.
 *  \param    which specifies the particular data set (if more than one).
 *  \param    invert is true to turn invert on; false otherwise.
 */
void ExampleCanvas::setInvert ( const int which, const bool invert ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.mInvert = invert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.mInvert = invert;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief    set the current setting of the overlay state.
 *  \param    overlay is true to turn overlay on; false otherwise.
 */
void ExampleCanvas::setOverlay ( const bool overlay ) { 
    mOverlay = overlay;
}
//----------------------------------------------------------------------
/** \brief  set the current red emphasis value [0.0, ..., 1.0].
 *  \param  which specifies the particular data set (if more than one).
 *  \param  r the current red emphasis value.
 */
void ExampleCanvas::setR ( const int which, const double r ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.setR( r );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.setR( r );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief  set the overall scale (magnification) of the displayed image(s).
 *  \param  scale the overall scale value.
 */
void ExampleCanvas::setScale   ( const double scale )  {
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    mScale = scale;
    int  w, h;
    GetSize( &w, &h );
    mCols  = (int)(w / (mXSize * mScale));
    mRows  = (int)(h / (mYSize * mScale));
    if (mCols<1)  mCols=1;
    if (mRows<1)  mRows=1;
    reload();
}
//----------------------------------------------------------------------
/** \brief  set the number of the first displayed slice.
 *  \param  which specifies the particular data set (if more than 1 read).
 *  \param  sliceNo specifies the number of the first displayed slice
 *          which should be in [0..slices-1].
 */
void ExampleCanvas::setSliceNo ( const int which, int sliceNo ) {
    if (sliceNo<0)    sliceNo = 0;
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        if (sliceNo>=cd.m_zSize)    sliceNo = cd.m_zSize-1;
        cd.m_sliceNo = sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        if (sliceNo>=cd.m_zSize)    sliceNo = cd.m_zSize-1;
        cd.m_sliceNo = sliceNo;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief  set the current width contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  width is the contrast setting value.
 */
void ExampleCanvas::setWidth ( const int which, const int width ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_width = width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_width = width;
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief Allow the user to scroll through the slices with the mouse wheel. */
void ExampleCanvas::OnMouseWheel ( wxMouseEvent& e ) {
    ExampleFrame*  ef = dynamic_cast<ExampleFrame*>(m_parent_frame);
    ef->OnMouseWheel(e);
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( ExampleCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( ExampleCanvas, wxPanel )
    EVT_CHAR(             ExampleCanvas::OnChar         )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_LEFT_DOWN(        ExampleCanvas::OnLeftDown     )
    EVT_LEFT_UP(          ExampleCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      ExampleCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        ExampleCanvas::OnMiddleUp     )
    EVT_MOTION(           ExampleCanvas::OnMouseMove    )
    EVT_MOUSEWHEEL(       ExampleCanvas::OnMouseWheel   )
    EVT_PAINT(            ExampleCanvas::OnPaint        )
    EVT_RIGHT_DOWN(       ExampleCanvas::OnRightDown    )
    EVT_RIGHT_UP(         ExampleCanvas::OnRightUp      )
    EVT_SIZE(             MainCanvas::OnSize            )
END_EVENT_TABLE()
//======================================================================

