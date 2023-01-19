/*
  Copyright 1993-2008, 2015-2016 Medical Image Processing Group
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
 * \file:  ITKFilterCanvas.cpp
 * \brief  ITKFilterCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ChunkData.h"

using namespace std;
//----------------------------------------------------------------------
const int  ITKFilterCanvas::sSpacing=50;  ///< space, in pixels, between each slice (on the screen in the frame in the canvas)
//----------------------------------------------------------------------
/** \brief ITKFilterCanvas ctor. */
ITKFilterCanvas::ITKFilterCanvas ( void ) {
    init();
}
//----------------------------------------------------------------------
/** \brief ITKFilterCanvas ctor. */
ITKFilterCanvas::ITKFilterCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    init();
}
//----------------------------------------------------------------------
/** \brief initialize members. */
void ITKFilterCanvas::init ( void ) {
    mFileOrDataCount = 0;
    mScale           = 1.0;
    mOverlay         = true;
    mRows            = 0;
    mCols            = 0;
    mImages[0]  = mImages[1]  = NULL;
    mBitmaps[0] = mBitmaps[1] = NULL;
    mTx = mTy        = 0;
    mXSize = mYSize  = mZSize = 0;
    mLastX = mLastY  = -1;
}
//----------------------------------------------------------------------
/** \brief ITKFilterCanvas dtor. */
ITKFilterCanvas::~ITKFilterCanvas ( void ) {
    cout << "ITKFilterCanvas::~ITKFilterCanvas" << endl;
    wxLogMessage( "ITKFilterCanvas::~ITKFilterCanvas" );
    release();
}
//----------------------------------------------------------------------
/** \brief release memory allocated to this object. */
void ITKFilterCanvas::release ( void ) {
#if 0
    while (mCavassData!=NULL) {
        CavassData*  next = mCavassData->mNext;
        delete mCavassData;
        mCavassData = next;
    }
    freeImagesAndBitmaps();
    init();
#endif
}
//----------------------------------------------------------------------
/** \brief load data from memory. */
void ITKFilterCanvas::loadData ( char* name,
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
void ITKFilterCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount>=0 && mFileOrDataCount<=1 );
	CavassData*  cd = new ChunkData( fn );
	if ((!cd->mIsCavassFile || !cd->m_vh_initialized) && !cd->mIsDicomFile &&
			!cd->mIsImageFile)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
    cd->mR = cd->mG = cd->mB = 1.0;
    if (mFileOrDataCount==0) {
        assert( mCavassData==NULL );
        mCavassData = cd;

        CavassData&  A = *mCavassData;
        mXSize = A.m_xSize;
        mYSize = A.m_ySize;
        mZSize = A.m_zSize;
        mCols  = mRows = 1;
    } else if (mFileOrDataCount==1) {
        assert( mCavassData!=NULL && mCavassData->mNext==NULL );
        mCavassData->mNext = cd;
        freeImagesAndBitmaps();
        mCols = 2;
    } else  {  assert( 0 );  }
    ++mFileOrDataCount;
    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief initialize the specified lookup table.
 *  \param which specifies the particular data set (if more than 1 read).
 */
void ITKFilterCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else             mCavassData->mNext->initLUT();
}
//----------------------------------------------------------------------
/** \brief free any allocated images (of type wxImage) and/or bitmaps
 *  (of type wxBitmap)
 */
void ITKFilterCanvas::freeImagesAndBitmaps ( void ) {
    if (mImages[0]!=NULL)   {  delete mImages[0];   mImages[0]=NULL;   }
    if (mImages[1]!=NULL)   {  delete mImages[1];   mImages[1]=NULL;   }

    if (mBitmaps[0]!=NULL)  {  delete mBitmaps[0];  mBitmaps[0]=NULL;  }
    if (mBitmaps[1]!=NULL)  {  delete mBitmaps[1];  mBitmaps[1]=NULL;  }
}
//----------------------------------------------------------------------
/** \brief reload the drawable image data. */
void ITKFilterCanvas::reload ( void ) {
    if (!isLoaded(0))    return;
    freeImagesAndBitmaps();
    
    //load the unfiltered, input data
    CavassData&  A = *mCavassData;
    assert( 0 <= A.m_sliceNo && A.m_sliceNo < A.m_zSize );
    //note: image data are 24-bit rgb
	mImages[0] = new wxImage( mXSize, mYSize, ::toRGB(A) );
    //determine the scale change (if any) including a global scale change,
	// a local scale, as well as the aspect ratio
    double  aspectRatio = A.m_xSpacing / A.m_ySpacing;
	if (mScale!=1.0 || A.m_scale!=1.0 || aspectRatio!=1.0) {
		const int  scaledW = (int)ceil( mXSize * mScale * A.m_scale * aspectRatio );
		const int  scaledH = (int)ceil( mYSize * mScale * A.m_scale );
        mImages[0]->Rescale( scaledW, scaledH );
        assert( mImages[0]!=NULL );
    }
    mBitmaps[0] = new wxBitmap( (const wxImage&) *mImages[0] );
    assert( mBitmaps[0]!=NULL );

    if (!isLoaded(1))  {  Refresh();  return;  }

    //load the filtered result
    assert( mCavassData->mNext != NULL );
    CavassData&  B = *(mCavassData->mNext);
    assert( 0<=B.m_sliceNo && B.m_sliceNo<B.m_zSize );

    //note: image data are 24-bit rgb
	mImages[1] = new wxImage( mXSize, mYSize, ::toRGB(B) );
    //determine the scale change (if any) including a global scale change,
	// a local scale, as well as the aspect ratio
    aspectRatio = B.m_xSpacing / B.m_ySpacing;
    if (mScale!=1.0 || B.m_scale!=1.0 || aspectRatio!=1.0) {
		const int  scaledW = (int)ceil( mXSize * mScale * B.m_scale * aspectRatio );
		const int  scaledH = (int)ceil( mYSize * mScale * B.m_scale );
        mImages[1]->Rescale( scaledW, scaledH );
        assert( mImages[1]!=NULL );
    }
    mBitmaps[1] = new wxBitmap( (const wxImage&) *mImages[1] );
    assert( mBitmaps[1]!=NULL );

    Refresh();
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button. */
void ITKFilterCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "ITKFilterCanvas::OnChar" << endl;
    wxLogMessage( "ITKFilterCanvas::OnChar" );
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
void ITKFilterCanvas::OnMouseMove ( wxMouseEvent& e ) {
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
void ITKFilterCanvas::OnRightDown ( wxMouseEvent& e ) {
    cout << "OnRightDown" << endl;    wxLogMessage("OnRightDown");
    SetFocus();  //to regain/recapture keypress events
    //pass the event up to the parent frame
    ITKFilterFrame*  p = (ITKFilterFrame*)m_parent_frame;
    wxCommandEvent  unused;
    //simulate the middle button for a two button mouse
    if (e.AltDown() || e.ControlDown() || e.ShiftDown())
        p->OnPrevious( unused );  //middle button
    else
        p->OnNext( unused );  //right button
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button up events. */
void ITKFilterCanvas::OnRightUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button down events. */
void ITKFilterCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    cout << "OnMiddleDown" << endl;    wxLogMessage("OnMiddleDown");
    SetFocus();  //to regain/recapture keypress events
    //pass the event up to the parent frame
    ITKFilterFrame*  p = (ITKFilterFrame*)m_parent_frame;
    wxCommandEvent  unused;
    p->OnPrevious( unused );
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button up events. */
void ITKFilterCanvas::OnMiddleUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
/** \brief Callback to handle left mouse button down events. */
void ITKFilterCanvas::OnLeftDown ( wxMouseEvent& e ) {
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events

    //simulate the middle button for a two button mouse
    if (e.ShiftDown()) {
        //pass the event up to the parent frame
        ITKFilterFrame*   p = (ITKFilterFrame*)m_parent_frame;
        wxCommandEvent  unused;
        p->OnPrevious( unused );  //middle button
        return;
    }
    if (e.ControlDown()) {
        //pass the event up to the parent frame
        ITKFilterFrame*   p = (ITKFilterFrame*)m_parent_frame;
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
void ITKFilterCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    mLastX = mLastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief callback for paint events. */
void ITKFilterCanvas::OnPaint ( wxPaintEvent& e ) {
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
void ITKFilterCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
    //anything loaded yet?
    if (mBitmaps[0]==NULL && mBitmaps[1]==NULL) {
		//just the background loaded?
        if (m_backgroundLoaded) {
            int  w, h;
            dc->GetSize( &w, &h );
            const int  bmW = m_backgroundBitmap.GetWidth();
            const int  bmH = m_backgroundBitmap.GetHeight();
            dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
        }
        return;
    }

	//display unfiltered data?
    if (mBitmaps[0]!=NULL && mBitmaps[0]->Ok()) {
        const int  x = sSpacing;
        const int  y = sSpacing;
        const int  slice = mCavassData->m_sliceNo;
        if (mCavassData->inBounds(0,0,slice))
			dc->DrawBitmap( *mBitmaps[0], x+mTx, y+mTy );
        //show the overlay?  (the overlay consists of numbers that indicate the slice)
        if (mOverlay) {
            //in bounds?
            if (mCavassData->inBounds(0,0,slice)) {
                //in bounds
				const wxString  s = wxString::Format( "(%d/%d)", slice+1, mCavassData->m_zSize );
                dc->DrawText( s, x+mTx, y+mTy );
            } else {
                //out of bounds
                const wxString  s = wxString::Format( "(-/-)" );
                dc->DrawText( s, x+mTx, y+mTy );
            }
        }
    }

	//display filtered data?
    if (mBitmaps[1]!=NULL && mBitmaps[1]->Ok()) {
		const int  x = (int)(ceil(mCavassData->m_xSize*mCavassData->m_scale*mScale)+2*sSpacing);
        const int  y = sSpacing;
		const int  slice = mCavassData->mNext->m_sliceNo;
        if (mCavassData->inBounds(0,0,slice))
            dc->DrawBitmap( *mBitmaps[1], x+mTx, y+mTy );
        //show the overlay?  (the overlay consists of numbers that indicate the slice)
        if (mOverlay) {
            //in bounds?
            if (mCavassData->mNext->inBounds(0,0,slice)) {
                //in bounds
				const wxString  s = wxString::Format( "(%d/%d)", slice+1, mCavassData->mNext->m_zSize );
                dc->DrawText( s, x+mTx, y+mTy );
            } else {
                //out of bounds
                const wxString  s = wxString::Format( "(-/-)" );
                dc->DrawText( s, x+mTx, y+mTy );
            }
        }
    }
}
//----------------------------------------------------------------------
/** \brief    determines if a particular data set has been loaded.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  true if the data set has been loaded; false otherwise.
 */
bool ITKFilterCanvas::isLoaded ( const int which ) const {
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
//----------------------------------------------------------------------
/** \brief    get the current center contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the current center contrast setting value.
 */
int ITKFilterCanvas::getCenter ( const int which ) const {
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
int ITKFilterCanvas::getMax ( const int which ) const {
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
int ITKFilterCanvas::getMin ( const int which ) const {
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
int ITKFilterCanvas::getNoSlices ( const int which ) const {
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
bool ITKFilterCanvas::getOverlay ( void ) const {
    return mOverlay;
}
//----------------------------------------------------------------------
/** \brief    get the overall scale of the displayed image(s).
 *  \returns  the overall scale value.
 */
double ITKFilterCanvas::getScale ( void ) const {
    return mScale;
}
//----------------------------------------------------------------------
/** \brief    get the individual scale of the displayed image(s).
 *  \returns  the individual scale value.
 */
double ITKFilterCanvas::getScale ( const int which ) const {
	if (which==0) {
		assert( mCavassData!=NULL );
		return mCavassData->m_scale;
	}
	if (which==1) {
		assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
		return mCavassData->mNext->m_scale;
	}
	assert( 0 );  //should never get here!
    return 0;
}
//----------------------------------------------------------------------
/** \brief    get the number of the first displayed slice.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the number of the first displayed slice.
 */
int ITKFilterCanvas::getSliceNo ( const int which ) const {
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
int ITKFilterCanvas::getWidth ( const int which ) const {
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
bool ITKFilterCanvas::getInvert ( const int which ) const {
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
double ITKFilterCanvas::getB ( const int which ) const {
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
double ITKFilterCanvas::getG ( const int which ) const {
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
double ITKFilterCanvas::getR ( const int which ) const {
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
void ITKFilterCanvas::setB ( const int which, const double b ) {
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
void ITKFilterCanvas::setCenter ( const int which, const int center ) {
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
void ITKFilterCanvas::setG ( const int which, const double g ) {
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
void ITKFilterCanvas::setInvert ( const int which, const bool invert ) {
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
void ITKFilterCanvas::setOverlay ( const bool overlay ) { 
    mOverlay = overlay;
}
//----------------------------------------------------------------------
/** \brief  set the current red emphasis value [0.0, ..., 1.0].
 *  \param  which specifies the particular data set (if more than one).
 *  \param  r the current red emphasis value.
 */
void ITKFilterCanvas::setR ( const int which, const double r ) {
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
void ITKFilterCanvas::setScale ( const double scale )  {
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    mScale = scale;
    reload();
}
//----------------------------------------------------------------------
/** \brief  set the overall scale (magnification) of the specific displayed
 *          image(s).
 *  \param  which specifies the data set for scale change
 *  \param  scale the overall scale value.
 */
void ITKFilterCanvas::setScale ( const int which, const double scale )  {
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

	if (which==0) {
		assert( mCavassData!=NULL );
		mCavassData->setScale( scale );
        reload();
		return;
	}
	if (which==1) {
		assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
		mCavassData->mNext->setScale( scale );
        reload();
		return;
	}
	assert( 0 );  //should never get here!
}
//----------------------------------------------------------------------
/** \brief  set the number of the first displayed slice.
 *  \param  which specifies the particular data set (if more than 1 read).
 *  \param  sliceNo specifies the number of the first displayed slice
 *          which should be in [0..slices-1].
 */
void ITKFilterCanvas::setSliceNo ( const int which, int sliceNo ) {
    if (sliceNo<0)    sliceNo=0;
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
void ITKFilterCanvas::setWidth ( const int which, const int width ) {
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
void ITKFilterCanvas::OnMouseWheel ( wxMouseEvent& e ) {
    ITKFilterFrame*  ef = dynamic_cast<ITKFilterFrame*>(m_parent_frame);
    ef->OnMouseWheel(e);
}
//----------------------------------------------------------------------
/** \brief Free/remove the filtered data. */
void ITKFilterCanvas::freeFilteredData ( void ) {
	if (mCavassData!=NULL && mCavassData->mNext!=NULL) {
		delete mCavassData->mNext;
		mCavassData->mNext = NULL;
		--mFileOrDataCount;
	}
	if (mImages[1]!=NULL)   {  delete mImages[1];   mImages[1]=NULL;   }
	if (mBitmaps[1]!=NULL)  {  delete mBitmaps[1];  mBitmaps[1]=NULL;  }
	Refresh();
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( ITKFilterCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( ITKFilterCanvas, wxPanel )
    EVT_CHAR(             ITKFilterCanvas::OnChar         )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_LEFT_DOWN(        ITKFilterCanvas::OnLeftDown     )
    EVT_LEFT_UP(          ITKFilterCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      ITKFilterCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        ITKFilterCanvas::OnMiddleUp     )
    EVT_MOTION(           ITKFilterCanvas::OnMouseMove    )
    EVT_MOUSEWHEEL(       ITKFilterCanvas::OnMouseWheel   )
    EVT_PAINT(            ITKFilterCanvas::OnPaint        )
    EVT_RIGHT_DOWN(       ITKFilterCanvas::OnRightDown    )
    EVT_RIGHT_UP(         ITKFilterCanvas::OnRightUp      )
    EVT_SIZE(             MainCanvas::OnSize            )
END_EVENT_TABLE()
//======================================================================

