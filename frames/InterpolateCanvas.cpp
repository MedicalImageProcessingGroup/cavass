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
 * \file:  InterpolateCanvas.cpp
 * \brief  InterpolateCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ChunkData.h"
#include  <limits.h>
#include  "InterpolateCanvas.h"

using namespace std;
//----------------------------------------------------------------------
const int  InterpolateCanvas::sSpacing=1;  ///< space, in pixels, between each slice (on the screen in the frame in the canvas)
//----------------------------------------------------------------------
/** \brief InterpolateCanvas ctor. */
InterpolateCanvas::InterpolateCanvas ( void ) {
    init();
}
//----------------------------------------------------------------------
/** \brief InterpolateCanvas ctor. */
InterpolateCanvas::InterpolateCanvas ( wxWindow* parent,
    MainFrame* parent_frame, wxWindowID id, const wxPoint &pos,
    const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    init();
}
//----------------------------------------------------------------------
/** \brief initialize members. */
void InterpolateCanvas::init ( void ) {
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
/** \brief InterpolateCanvas dtor. */
InterpolateCanvas::~InterpolateCanvas ( void ) {
    cout << "InterpolateCanvas::~InterpolateCanvas" << endl;
    wxLogMessage( "InterpolateCanvas::~InterpolateCanvas" );
    release();
}
//----------------------------------------------------------------------
/** \brief release memory allocated to this object. */
void InterpolateCanvas::release ( void ) {
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
void InterpolateCanvas::loadData ( char* name,
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
 *  \param fn is the input data file name.
 */
void InterpolateCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount==0 );
    ChunkData*  cd = new ChunkData( fn, 2 );
    assert( mCavassData==NULL );
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
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
void InterpolateCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else             mCavassData->mNext->initLUT();
}
//----------------------------------------------------------------------
/** \brief free any allocated images (of type wxImage) and/or bitmaps
 *  (of type wxBitmap)
 */
void InterpolateCanvas::freeImagesAndBitmaps ( void ) {
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
/** \brief Reload the drawable image data. This function should be called
 *  whenever the contents of the canvas changes (for example, when data or
 *  or a file is first loaded, when the contents of the canvas changes,
 *  or when the geometry of the window changes.
 */
void InterpolateCanvas::reload ( void ) {
    if (!isLoaded(0))    return;
    freeImagesAndBitmaps();
    
    int  k;    
    mImages = (wxImage**)malloc( mCols * mRows * sizeof(wxImage*) );
    for (k=0; k<mCols*mRows; k++)    mImages[k]=NULL;
    
    mBitmaps = (wxBitmap**)malloc( mCols * mRows * sizeof(wxBitmap*) );
    for (k=0; k<mCols*mRows; k++)    mBitmaps[k]=NULL;
    
    CavassData&  A = *mCavassData;

    //this section determines an appropriate scale factor so the displayed slice
    // will always fit in the window.
    double  aspectRatio = A.m_xSpacing / A.m_ySpacing;
    int     scaledW = (int)ceil( mXSize * mScale * aspectRatio );
    int     scaledH = (int)ceil( mYSize * mScale );
    int     w, h;
    GetSize( &w, &h );
    w -= 10;
    h -= 10;
    if (scaledW > w) {
        mScale  = (double)w / (mXSize * aspectRatio);
        scaledW = (int)ceil( mXSize * mScale * aspectRatio );
        scaledH = (int)ceil( mYSize * mScale );
    }
    if (scaledH > h) {
        mScale  = (double)h / mYSize;
        scaledW = (int)ceil( mXSize * mScale * aspectRatio );
        scaledH = (int)ceil( mYSize * mScale );
    }

    for (k=0; k<mCols*mRows; k++) {
        if (A.m_sliceNo+k >= A.m_zSize)    break;
        //note: image data are 24-bit rgb
        mImages[k] = new wxImage( mXSize, mYSize, ::toRGB(A) );
        //determine the scale change (if any) including a global scale change
        // as well as the aspect ratio
        const double  aspectRatio = A.m_xSpacing / A.m_ySpacing;
        if (mScale!=1.0 || aspectRatio!=1.0) {
            const int  scaledW = (int)ceil( mXSize * mScale * aspectRatio );
            const int  scaledH = (int)ceil( mYSize * mScale );
            mImages[k]->Rescale( scaledW, scaledH );
        }
        mBitmaps[k] = new wxBitmap( (const wxImage&) *mImages[k] );
    }
    Refresh();
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button. */
void InterpolateCanvas::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "InterpolateCanvas::OnChar" );
    if (!isLoaded(0))        return;
    if (e.m_keyCode!=' ')    return;
    //spacebar was pressed.
    wxMouseEvent  m;
    if (e.ControlDown())    OnRightDown( m );
    else                    OnMiddleDown( m );
}
//----------------------------------------------------------------------
/** \brief callback for mouse move events */
void InterpolateCanvas::OnMouseMove ( wxMouseEvent& e ) {
    //if (m_data==NULL)    return;
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - mTx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - mTy;
    //if we are NOT in any mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    if (e.LeftIsDown()) {
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
/** \brief Callback to handle middle mouse button down events. */
void InterpolateCanvas::OnMiddleDown ( wxMouseEvent& unused ) {
    SetFocus();  //to regain/recapture keypress events
    //display the next slice
    if (mCavassData==NULL)    return;
    int  current = getSliceNo(0);
    int  max = getNoSlices(0);
    --current;
    if (current<0)    current = max-1;
    setSliceNo(0, current);
    reload();
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button up events. */
void InterpolateCanvas::OnRightUp ( wxMouseEvent& unused ) {
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button down events. */
void InterpolateCanvas::OnRightDown ( wxMouseEvent& unused ) {
    SetFocus();  //to regain/recapture keypress events
    //display the next slice
    if (mCavassData==NULL)    return;
    int  current = getSliceNo(0);
    int  max = getNoSlices(0);
    ++current;
    if (current>=max)    current = 0;
    setSliceNo(0, current);
    reload();
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button up events. */
void InterpolateCanvas::OnMiddleUp ( wxMouseEvent& unused ) {
}
//----------------------------------------------------------------------
/** \brief Callback to handle left mouse button down events. */
void InterpolateCanvas::OnLeftDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
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
void InterpolateCanvas::OnLeftUp ( wxMouseEvent& unused ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    mLastX = mLastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief callback for paint events. */
void InterpolateCanvas::OnPaint ( wxPaintEvent& unused ) {
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
void InterpolateCanvas::paint ( wxDC* dc ) {
    if (Preferences::getCustomAppearance())
#if wxCHECK_VERSION(2, 9, 0)
        dc->SetBrush( wxBrush(wxColour(DkBlue), wxBRUSHSTYLE_SOLID) );
    else
        dc->SetBrush( wxBrush(*wxBLACK, wxBRUSHSTYLE_SOLID) );
#else
        dc->SetBrush( wxBrush(wxColour(DkBlue), wxSOLID) );
    else
        dc->SetBrush( wxBrush(*wxBLACK, wxSOLID) );
#endif
    int  w, h;
    dc->GetSize( &w, &h );
    dc->DrawRectangle( 0, 0, w, h );

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
        drawGraphic( dc );
    } else if (m_backgroundLoaded) {
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}
//----------------------------------------------------------------------
static inline int Round ( const double d ) {
    return (int)(d+0.5);
}
//----------------------------------------------------------------------
/** \brief Draw graphic indicating slice locations. */
void InterpolateCanvas::drawGraphic ( wxDC* dc ) {
    if (mZSize<2)    return;
#if wxCHECK_VERSION(2, 9, 0)
    dc->SetBrush( wxBrush(wxColour(MenuBlue), wxBRUSHSTYLE_SOLID) );
#else
    dc->SetBrush( wxBrush(wxColour(MenuBlue), wxSOLID) );
#endif
    wxPen  yellow;
    yellow.SetColour( Yellow );
    dc->SetPen( yellow );

    wxPen  blue;
    blue.SetColour( MenuBlue );

    int  w, h;
    dc->GetSize( &w, &h );
    const int  size   = (w<h) ? w : h;
    const int  cw     = w/2;
    const int  ch     = h/2;
    const int  startX = (int)(cw - 0.35*size);
    const int  startY = (int)(ch + 0.35*size);
    const int  endX   = (int)(cw + 0.35*size);
    const int  endY   = (int)(ch - 0.35*size);
    const int  startXLine = startX - 100;
    const int  endXLine   = endX   - 100;
    dc->DrawLine( startXLine, startY, endXLine, endY );

    wxString  s;
    assert( mCavassData!=NULL && mCavassData->m_vh_initialized );

    //draw the intermediary tick marks, slice number, and slice locations
    const double  min = mCavassData->m_vh.scn.loc_of_subscenes[0];
    const double  max = mCavassData->m_vh.scn.loc_of_subscenes[mZSize-1];
    assert( min < max );

    const int  mod = mZSize<5? 1:Round(mZSize/10.0);
    double  lastProportion = -1.0;
    for (int i=mZSize-1; i>=0; i--) {
        const double  proportion = (mCavassData->m_vh.scn.loc_of_subscenes[i] - min) / (max - min);
        const int  x = Round(proportion*endX + (1.0-proportion)*startX);
        const int  y = Round(proportion*endY + (1.0-proportion)*startY);
        const int  xLine = x - 100;
        const int  yLine = y;
        //draw the first, last, and every 10% slice information
        if ( i==0 || i==mZSize-1 || (i%mod)==0 ) {
            if (lastProportion==-1.0 || fabs(lastProportion-proportion)>=0.05) {
                dc->SetPen( yellow );
                dc->DrawLine( xLine-5, yLine, xLine+5, yLine );
                s = wxString::Format( "%4d", (i+1) );
                dc->DrawText( s, xLine+20-80, y-10 );
                s = wxString::Format( "%.4f", mCavassData->m_vh.scn.loc_of_subscenes[i] );
                dc->DrawText( s, xLine+20, y-10 );
                dc->DrawRectangle( x, y, (int)(0.1*size), (int)(0.1*size) );
                lastProportion = proportion;
            } else {
                dc->SetPen( blue );
                dc->DrawLine( xLine-5, yLine, xLine+5, yLine );
                dc->DrawRectangle( x, y, (int)(0.1*size), (int)(0.1*size) );
            }
        } else {
            dc->SetPen( blue );
            dc->DrawLine( xLine-5, yLine, xLine+5, yLine );
            dc->DrawRectangle( x, y, (int)(0.1*size), (int)(0.1*size) );
        }
    }
}
//----------------------------------------------------------------------
/** \brief    determines if a particular data set has been loaded.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  true if the data set has been loaded; false otherwise.
 */
bool InterpolateCanvas::isLoaded ( const int which ) const {
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
int InterpolateCanvas::getCenter ( const int which ) const {
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
int InterpolateCanvas::getMax ( const int which ) const {
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
int InterpolateCanvas::getMin ( const int which ) const {
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
int InterpolateCanvas::getNoSlices ( const int which ) const {
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
bool InterpolateCanvas::getOverlay ( void ) const {
    return mOverlay;
}
//----------------------------------------------------------------------
/** \brief    get the overall scale of the displayed image(s).
 *  \returns  the overall scale value.
 */
double InterpolateCanvas::getScale ( void ) const {
    return mScale;
}
//----------------------------------------------------------------------
/** \brief    get the number of the first displayed slice.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the number of the first displayed slice.
 */
int InterpolateCanvas::getSliceNo ( const int which ) const {
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
int InterpolateCanvas::getWidth ( const int which ) const {
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
bool InterpolateCanvas::getInvert ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.mInvert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
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
double InterpolateCanvas::getB ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.getB();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
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
double InterpolateCanvas::getG ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.getG();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
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
double InterpolateCanvas::getR ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.getR();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
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
void InterpolateCanvas::setB ( const int which, const double b ) {
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
void InterpolateCanvas::setCenter ( const int which, const int center ) {
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
void InterpolateCanvas::setG ( const int which, const double g ) {
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
void InterpolateCanvas::setInvert ( const int which, const bool invert ) {
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
void InterpolateCanvas::setOverlay ( const bool overlay ) { 
    mOverlay = overlay;
}
//----------------------------------------------------------------------
/** \brief  set the current red emphasis value [0.0, ..., 1.0].
 *  \param  which specifies the particular data set (if more than one).
 *  \param  r the current red emphasis value.
 */
void InterpolateCanvas::setR ( const int which, const double r ) {
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
void InterpolateCanvas::setScale ( const double scale )  {
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
 *  \param  sliceNo specifies the number of the first displayed slice.
 */
void InterpolateCanvas::setSliceNo ( const int which, const int sliceNo ) {
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
/** \brief  set the current width contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  width is the contrast setting value.
 */
void InterpolateCanvas::setWidth ( const int which, const int width ) {
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
void InterpolateCanvas::OnMouseWheel ( wxMouseEvent& e ) {
    const int  rot   = e.GetWheelRotation();
    if (rot>0)         OnRightDown(e);
    else if (rot<0)    OnMiddleDown(e);
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( InterpolateCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( InterpolateCanvas, wxPanel )
    EVT_CHAR(             InterpolateCanvas::OnChar         )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground     )
    EVT_LEFT_DOWN(        InterpolateCanvas::OnLeftDown     )
    EVT_LEFT_UP(          InterpolateCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      InterpolateCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        InterpolateCanvas::OnMiddleUp     )
    EVT_MOTION(           InterpolateCanvas::OnMouseMove    )
    EVT_MOUSEWHEEL(       InterpolateCanvas::OnMouseWheel   )
    EVT_PAINT(            InterpolateCanvas::OnPaint        )
    EVT_RIGHT_DOWN(       InterpolateCanvas::OnRightDown    )
    EVT_RIGHT_UP(         InterpolateCanvas::OnRightUp      )
    EVT_SIZE(             MainCanvas::OnSize                )
END_EVENT_TABLE()
//======================================================================
