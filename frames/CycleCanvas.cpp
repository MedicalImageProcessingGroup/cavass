/*
  Copyright 1993-2011, 2015-2016, 2018, 2021 Medical Image Processing Group
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
 * \file   CycleCanvas.cpp
 * \brief  CycleCanvas class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "CycleFrame.h"
#include  "ChunkData.h"
#include  "CycleData.h"
#include  "CycleCanvas.h"
#include  "TextControls.h"

using namespace std;

extern int  gTimerInterval;
//----------------------------------------------------------------------
const int  CycleCanvas::sSpacing=1;  //space, in pixels, between each slice
int  CycleCanvas::Measure::sCount = 1;
//----------------------------------------------------------------------
CycleCanvas::CycleCanvas ( void ) : MainCanvas() {
    init();
}
//----------------------------------------------------------------------
CycleCanvas::CycleCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    init();
}
//----------------------------------------------------------------------
/** \brief initialize members. */
void CycleCanvas::init ( void ) {
    lastX = lastY = 0;

    m_bitmaps = NULL;
    m_cols = 0;
    m_images = NULL;
    m_rows = 0;
    m_sliceNo = 0;
    m_tx = m_ty = 0;
    mInterpolate = false;
    mFileOrDataCount = 0;
    mFirstDisplayedDataset = 0;
    mMaxPixelWidth = mMaxPixelHeight = mMaxSlices = 0;
    mMagnifyState = MagnifyStateChoose;
    mDataInitialized = false;
    mSaveFile = wxEmptyString;
}
//----------------------------------------------------------------------
CycleCanvas::~CycleCanvas ( void ) {
    cout << "CycleCanvas::~CycleCanvas" << endl;
    wxLogMessage( "CycleCanvas::~CycleCanvas" );
    while (mCavassData!=NULL) {
        CavassData*  next = mCavassData->mNext;
        delete mCavassData;
        mCavassData = next;
    }
    release();
}
//----------------------------------------------------------------------
void CycleCanvas::release ( void ) {
//    m_scale   = 1.0;
//    m_overlay = true;
    freeImagesAndBitmaps();
    m_rows = m_cols = 0;
    mMaxPixelWidth = mMaxPixelHeight = 0;
    mMaxSlices = 0;
    mFirstDisplayedDataset = 0;
//    m_center = m_width = 0;
//    if (m_lut!=NULL)    {  free(m_lut);      m_lut   = NULL;  }
    m_sliceNo = 0;
//    if (m_fname!=NULL)  {  free(m_fname);    m_fname = NULL;  }
//    if (m_data!=NULL)   {  free(m_data);     m_data  = NULL;  }
//    m_min = m_max = 0;
//    m_size = 0;
//    m_bytesPerSlice = 0;
    m_tx = m_ty = 0;
//    mInvert = false;
    lastX = lastY = -1;
}
//----------------------------------------------------------------------
void CycleCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    /** \todo support more than one image at a time */
    release();
	CycleData *cd=new CycleData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    if ((!cd->mIsCavassFile || !cd->m_vh_initialized) && !cd->mIsDicomFile &&
			!cd->mIsImageFile)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
    mCavassData = cd;
	mCavassData->mR = mCavassData->mG = mCavassData->mB = 1.0;
//    m_lut = (unsigned char*)malloc((mCavassData->m_max-mCavassData->m_min+1)*sizeof(unsigned char));
//    if (mCavassData->m_min==0 && mCavassData->m_max==1)    m_center = 0;
//    else                         m_center = mCavassData->m_min + (mCavassData->m_max-mCavassData->m_min+1)/2;
//    m_width  = mCavassData->m_max - mCavassData->m_min + 1;
//    initLUT();
    
    //load the slices (starting with the first, zero)
    m_sliceNo = 0;
    int  w, h;
    GetSize( &w, &h );
    m_cols = (int)(w / (mCavassData->m_xSize * mCavassData->m_scale));
    if (m_cols<1)    m_cols = 1;
    else if (m_cols>mFileOrDataCount)    m_cols = mFileOrDataCount;
    m_rows = 1;
    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void CycleCanvas::loadFile ( const char* const fn ) {
    if (fn==NULL || strlen(fn)==0)    return;
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();

    ++mFileOrDataCount;
    if (mCavassData==NULL) {  //first one?
        CycleData *cd=new CycleData( fn );
		if ((!cd->mIsCavassFile || !cd->m_vh_initialized) &&
				!cd->mIsDicomFile && !cd->mIsImageFile)
		{
			wxMessageBox("Failed to load file.");
			return;
		}
		mCavassData = cd;
        mCavassData->mR = mCavassData->mG = mCavassData->mB = 1.0;
        mMaxSlices = mCavassData->m_zSize;
    } else {
        //add at the end of the list
        CycleData*  last = dynamic_cast< CycleData*>( mCavassData );
        CycleData*  next = dynamic_cast< CycleData*>( mCavassData->mNext );
        while (next!=NULL) {
            last = next;
            next = dynamic_cast< CycleData*>( next->mNext );
        }
        next = new CycleData( fn );
		if ((!next->mIsCavassFile || !next->m_vh_initialized) &&
				!next->mIsDicomFile && !next->mIsImageFile)
		{
			wxMessageBox("Failed to load file.");
			return;
		}
        last->mNext = next;
        next->mR = next->mG = next->mB = 1.0;
        if (next->m_zSize > mMaxSlices)
            mMaxSlices = next->m_zSize;
    }
    
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void CycleCanvas::reload ( void ) {
    if (!isLoaded(0))  return;
    freeImagesAndBitmaps();
    
    //get one slice from each data set
    m_rows = 1;
    m_cols = mFileOrDataCount;

    int  k;
    m_images = (wxImage**)malloc( m_cols * m_rows * sizeof(wxImage*) );
    for (k=0; k<m_cols*m_rows; k++)    m_images[k]=NULL;
    
    m_bitmaps = (wxBitmap**)malloc( m_cols * m_rows * sizeof(wxBitmap*) );
    for (k=0; k<m_cols*m_rows; k++)    m_bitmaps[k]=NULL;

    //assert( m_rows==1 && m_cols==mFileOrDataCount );
    CycleData*  data = dynamic_cast<CycleData*>( mCavassData );
    for (int i=0; i<mFileOrDataCount; i++) {
        assert( data!=NULL );
        //assert( 0<=data->m_sliceNo && data->m_sliceNo<data->m_zSize );
        if (data->m_sliceNo<0)    data->m_sliceNo = 0;
        if (data->m_sliceNo>=data->m_zSize)    data->m_sliceNo = data->m_zSize-1;
        const double  xFactor = data->m_scale * data->m_xSpacing;
        const double  yFactor = data->m_scale * data->m_ySpacing;

        //note: image data is 24-bit rgb
        if (xFactor==1.0 && yFactor==1.0) {
            m_images[i] = new wxImage( data->m_xSize, data->m_ySize, ::toRGB(*data) );
            //inset/offset into the roi if necessary
            if (data->mHasRoi) {
                assert( data->mRoiX + data->mRoiWidth  <= data->m_xSize );
                assert( data->mRoiY + data->mRoiHeight <= data->m_ySize );
                wxRect  r( data->mRoiX, data->mRoiY, data->mRoiWidth, data->mRoiHeight );
                wxImage*  tmp = new wxImage( m_images[i]->GetSubImage(r) );
                delete m_images[i];
                m_images[i] = tmp;
            }
        } else if (!mInterpolate) {
            m_images[i] = new wxImage( data->m_xSize, data->m_ySize, ::toRGB(*data) );
            int  scaledW=0, scaledH=0;
            if (data->mHasRoi) {
				assert( data->mRoiX>=0 && data->mRoiY>=0 );
                assert( data->mRoiX + data->mRoiWidth  <= data->m_xSize );
                assert( data->mRoiY + data->mRoiHeight <= data->m_ySize );
                wxRect  r( data->mRoiX, data->mRoiY, data->mRoiWidth, data->mRoiHeight );
                wxImage*  tmp = new wxImage( m_images[i]->GetSubImage(r) );
                delete m_images[i];
                m_images[i] = tmp;
                scaledW = (int)ceil( data->mRoiWidth  * xFactor );
                scaledH = (int)ceil( data->mRoiHeight * yFactor );
            } else {
                scaledW = (int)ceil( data->m_xSize * xFactor );
                scaledH = (int)ceil( data->m_ySize * yFactor );
            }
            m_images[i]->Rescale( scaledW, scaledH );
        } else {
            m_images[i] = new wxImage( (int)(data->m_xSize*xFactor),
                (int)(data->m_ySize*yFactor),
                ::toRGBInterpolated(*data, xFactor, yFactor) );
            if (data->mHasRoi) {
                assert( data->mRoiX + data->mRoiWidth  <= data->m_xSize );
                assert( data->mRoiY + data->mRoiHeight <= data->m_ySize );
                wxRect  r( (int)ceil( data->mRoiX * xFactor ),
                           (int)ceil( data->mRoiY * yFactor ),
                           (int)ceil( data->mRoiWidth * xFactor ),
                           (int)ceil( data->mRoiHeight * yFactor ) );
                wxImage*  tmp = new wxImage( m_images[i]->GetSubImage(r) );
                delete m_images[i];
                m_images[i] = tmp;
            }
        }
        m_bitmaps[i] = new wxBitmap( (const wxImage&) *m_images[i] );
        data = dynamic_cast<CycleData*>( data->mNext );
    }

    Refresh();
}
//----------------------------------------------------------------------
bool CycleCanvas::mapWindowToData ( int wx, int wy,
    const CycleData* const which, int& d, int& x, int& y, int& z )
{
    d = x = y = z = -1;
    //this algorithm basically mimics the paint method (which paints the
    // slices) and determine where the position lies
    if (m_bitmaps==NULL)    return false;
    if (!mDataInitialized)    return false;
    bool  found = false;
    int  i = 0;
    CycleData*  data = dynamic_cast<CycleData*>( mCavassData );
    while (data!=NULL) {
        if (data == which) {
            if (m_bitmaps!=NULL && m_bitmaps[i]!=NULL && m_bitmaps[i]->Ok()) {
                int  left   = data->mWX;
                int  top    = data->mWY;
                int  right  = left + m_bitmaps[i]->GetWidth();
                int  bottom = top  + m_bitmaps[i]->GetHeight();
                if (left<=wx && wx<=right && top<=wy && wy<=bottom) {
                    found = true;
                    d = i;
                    x = (int)((wx - data->mWX) / (data->getScale() * data->m_xSpacing));
                    y = (int)((wy - data->mWY) / (data->getScale() * data->m_ySpacing));
                    z = data->m_sliceNo;
                }
            }
            break;
        }

        data = dynamic_cast<CycleData*>( data->mNext );
        i++;
    }
    return found;
}
//----------------------------------------------------------------------
/** \brief map a point in the window back into the 3d data set.
 *  \param wx is the x coord in the window
 *  \param wy is the y coord in the window
 *  \param d (returned) is the number corresponding to the dataset
 *  \param x (returned) is the corresponding x coord in the dataset
 *  \param y (returned) is the corresponding y coord in the dataset
 *  \param z (returned) is the corresponding z coord in the dataset
 *  \returns true if position can be mapped to data; false otherwise
 */
bool CycleCanvas::mapWindowToData ( int wx, int wy,
                                    int& d, int& x, int& y, int& z )
{
    d = x = y = z = -1;
    //this algorithm basically mimics the paint method (which paints the
    // slices) and determine where the position lies
    if (m_bitmaps==NULL)    return false;
    if (!mDataInitialized)    return false;
    bool  found = false;
    int  i = 0;
    CycleData*  data = dynamic_cast<CycleData*>( mCavassData );
    while (data!=NULL) {
        if (m_bitmaps!=NULL && m_bitmaps[i]!=NULL && m_bitmaps[i]->Ok()) {
            int  left   = data->mWX;
            int  top    = data->mWY;
            int  right  = left + m_bitmaps[i]->GetWidth();
            int  bottom = top  + m_bitmaps[i]->GetHeight();
            if (left<=wx && wx<=right && top<=wy && wy<=bottom) {
                found = true;
                d = i;
                x = (int)((wx - data->mWX) / (data->getScale() * data->m_xSpacing));
                y = (int)((wy - data->mWY) / (data->getScale() * data->m_ySpacing));
				if (data->mHasRoi) {  //include offset to start of roi?
					x += data->mRoiX;
					y += data->mRoiY;
				}
                z = data->m_sliceNo;
            }
        }

        data = dynamic_cast<CycleData*>( data->mNext );
        i++;
    }
    return found;
#if 0
    int  dataset = mFirstDisplayedDataset;
    int  sliceNo = m_sliceNo;
    int  i = 0;
    for (int r=0; r<m_rows; r++) {
        const int     sy = r * (mMaxPixelHeight + sSpacing) + m_ty;  //screen y
        const int     lx = getDataset( dataset )->m_xSize-1;   //last x
        const int     ly = getDataset( dataset )->m_ySize-1;   //last y
        const int     lz = getDataset( dataset )->m_zSize-1;   //last z
        const double  scx= getDataset( dataset )->getScale() * getDataset( dataset )->m_xSpacing;  //scale in x direction
        const double  scy= getDataset( dataset )->getScale() * getDataset( dataset )->m_ySpacing;  //scale in y direction
        for (int c=0; c<m_cols; c++) {
            if (m_bitmaps!=NULL && m_bitmaps[i]!=NULL) {
                //(sx,sy) is (left,top) corresponds to (0,0) for a particular slice
                const int  sx = c * (mMaxPixelWidth + sSpacing) + m_tx;  //screen x
                if (wx>=sx && wy>=sy) {
                    //at this point we know that (sx,sy) is to the left and above (wx,wy)
                    int  px = (int)((wx-sx) / scx);
                    int  py = (int)((wy-sy) / scy);
                    if (0<=px && px<=lx && 0<=py && py<=ly && 0<=sliceNo+c && sliceNo+c<=lz) {
                        d = dataset;
                        x = px;
                        y = py;
                        z = sliceNo + c;
                        return true;
                    }
                }
            }
            i++;
        }  //end for c

        //typically, the next row is from the next dataset (although we may
        // not have another dataset or we may have another dataset but we
        //have not have any more slices for it)
        ++dataset;
        for ( ; dataset<mFileOrDataCount; dataset++) {
            if (sliceNo < getDataset(dataset)->m_zSize)    break;
        }
        if (dataset>=mFileOrDataCount) {  //find any more?
            sliceNo += m_cols;
            for (dataset=0; dataset<mFileOrDataCount; dataset++) {
                if (sliceNo < getDataset(dataset)->m_zSize)    break;
            }
        }
        if (dataset>=mFileOrDataCount)    break;
    }  //end for r

    return false;
#endif
}
//----------------------------------------------------------------------
/** \brief    map a point in the window back into an element of the
 *            magnify vector.
 *  \param    wx is the x coord in the window
 *  \param    wy is the y coord in the window
 *  \returns  the index into the magnify vector; -1 otherwise
 */
int  CycleCanvas::mapWindowToMagnifyVector ( int wx, int wy ) {
    if (mMagnifyVector.empty())    return -1;
    for (int i=(int)mMagnifyVector.size()-1; i>=0; i--) {
        Magnify*  m = &mMagnifyVector[i];
        int       w = m->mBitmap->GetWidth();
        int       h = m->mBitmap->GetHeight();
        int       x = m->mSX - w/2;
        int       y = m->mSY - h/2;
        if (wx>=x && wx>=y && wx<=(x+w) && wy<=(y+h))    return i;
    }
    return -1;
}
//----------------------------------------------------------------------
/** \brief    map a point in the window back into an element of the
 *            magnify vector.
 *  \param    wx is the x coord in the window
 *  \param    wy is the y coord in the window
 *  \param    d is the number corresponding to the dataset
 *  \param    x is the corresponding x coord in the dataset
 *  \param    y is the corresponding y coord in the dataset
 *  \param    z is the corresponding z coord in the dataset
 *  \returns  the index into the magnify vector; -1 otherwise
 */
int  CycleCanvas::mapWindowToMagnifyVector ( int wx, int wy,
                                               int& d, int& x, int& y, int& z )
{
    d = x = y = z = -1;
    if (mMagnifyVector.empty())    return -1;
    for (int i=(int)mMagnifyVector.size()-1; i>=0; i--) {
        Magnify*  m = &mMagnifyVector[i];
        int  w = m->mBitmap->GetWidth();
        int  h = m->mBitmap->GetHeight();
        //corner coordinates (in terms of the window) of magnified image
        int  cx = m->mSX - w/2;
        int  cy = m->mSY - h/2;
        if (wx>=cx && wx>=cy && wx<=(cx+w) && wy<=(cy+h)) {
            //determine corresponding dataset & image coorindates
            d = m->mDataset;
            CavassData*  cd = getDataset( d );
            assert( cd != NULL );
            double  scx = m->mScale * cd->m_xSpacing;  //scale in x direction
            double  scy = m->mScale * cd->m_ySpacing;  //scale in y direction
            x = (int)((wx-cx)/scx);
            y = (int)((wy-cy)/scy);
            if (x<0)    x = 0;
            if (y<0)    y = 0;
            if (x>=cd->m_xSize)    x = cd->m_xSize;
            if (y>=cd->m_ySize)    x = cd->m_ySize;
            z = m->mSlice;
            return i;
        }
    }
    return -1;
}
//----------------------------------------------------------------------
void CycleCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "CycleCanvas::OnChar" << endl;
    wxLogMessage( "CycleCanvas::OnChar" );
    //pass the event up to the parent frame
    //m_parent_frame->ProcessEvent( e );
}
//----------------------------------------------------------------------
/** \brief move the element in the list denoted by 'which' to the end of
 *  the list.  this will cause it to be drawn last which will make it
 *  appear as the frontmost one.
 */
void CycleCanvas::sendToEnd ( int which ) {
    assert( which>=0 );
    assert( mCavassData!=NULL );
    //special case for a list of only one element.
    if (mCavassData->mNext==NULL)    return;

    //find this data set in our list and delete it
    bool  foundIt = false;
    int   i = 0;
    CycleData*  last = NULL;
    CycleData*  data = dynamic_cast< CycleData*>( mCavassData );
    while (data!=NULL) {
        if (i==which) {
            foundIt = true;
            //remove from the head?
            if (i==0)    mCavassData = mCavassData->mNext;
            else         last->mNext = data->mNext;
            break;
        }
        i++;
        last = data;
        data = dynamic_cast< CycleData*>( data->mNext );
    }
    assert( foundIt );
    if (!foundIt)    return;

    //'data' points to the element that was removed from the list.
    //  'data' needs to be inserted at the end of the list.
    data->mNext = NULL;
    CycleData*  tmp = dynamic_cast< CycleData*>( mCavassData );
    while (tmp->mNext!=NULL) {
        tmp = dynamic_cast< CycleData*>( tmp->mNext );
    }
    tmp->mNext = data;
}
//----------------------------------------------------------------------
/** \brief respond to mouse move events. */
void CycleCanvas::OnMouseMove ( wxMouseEvent& e ) {
    wxLogMessage( "in CycleCanvas::OnMouseMove" );
    if (mCavassData==NULL || mCavassData->m_data==NULL)    return;

    //determine the location of the click in the window
    wxClientDC  dc(this);
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long   lx = dc.DeviceToLogicalX( pos.x );
    const long   ly = dc.DeviceToLogicalY( pos.y );
    const long   wx = lx - m_tx;
    const long   wy = ly - m_ty;
    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );

    if (mf->mWhichMode==CycleFrame::ModeContinuous && e.LeftIsDown()) {
        const int  slices = getNoSlices();
        int  s = getSliceNo();
        int  dx = wx - this->lastX;
        if (dx>0) {
            ++s;
            if (s < slices) {
                mf->setSliceNo( s );
                setSliceNo( s );
                reload();
            }
        }
        if (dx<0) {
            --s;
            if (s >= 0) {
                mf->setSliceNo( s );
                setSliceNo( s );
                reload();
            }
        }
        this->lastX = wx;
        this->lastY = wy;
        wxLogMessage( "out CycleCanvas::OnMouseMove 1" );
        return;
    }

    if (mf->mWhichMode==CycleFrame::ModeDiscontinuous && e.LeftIsDown()) {
        const int  slices = getNoSlices();
        int  s = getSliceNo();
        int  dx = wx - this->lastX;
        if (dx>0) {
            ++s;
            if (s < slices) {
                mf->setSliceNo( s );
                setSliceNo( s );
                reload();
            }
        }
        if (dx<0) {
            --s;
            if (s >= 0) {
                mf->setSliceNo( s );
                setSliceNo( s );
                reload();
            }
        }
        this->lastX = wx;
        this->lastY = wy;
        wxLogMessage( "out CycleCanvas::OnMouseMove 2" );
        return;
    }

    if (mf->mWhichMode==CycleFrame::ModeErase && e.LeftIsDown()) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z )) {
            this->lastX = wx;
            this->lastY = wy;
            return;
        }
        //find this data set in our list and delete it
        int  i = 0;
        CycleData*  last = NULL;
        CycleData*  data = dynamic_cast< CycleData*>( mCavassData );
        while (data!=NULL) {
            if (i==d) {
                //remove from the head?
                if (i==0)    mCavassData = mCavassData->mNext;
                else         last->mNext = data->mNext;
                delete data;
                data = NULL;
                --mFileOrDataCount;
                break;
            }
            i++;
            last = data;
            data = dynamic_cast< CycleData*>( data->mNext );
        }
        this->lastX = wx;
        this->lastY = wy;
        Refresh();
        wxLogMessage( "out CycleCanvas::OnMouseMove 3" );
        return;
    }

    //if we are in layout mode and the left mouse button is down,
    // then allow the user to move (translate) the image.
    if (mf->mWhichMode==CycleFrame::ModeLayout && e.LeftIsDown()) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z )) {
            this->lastX = wx;
            this->lastY = wy;
            return;
        }
        int  dx = wx - this->lastX;
        int  dy = wy - this->lastY;
        //move (translate) the image
        CycleData*  data = dynamic_cast<CycleData*>( getDataset(d) );
        data->mWX += dx;
        data->mWY += dy;

        this->lastX = wx;
        this->lastY = wy;
        Refresh();
        wxLogMessage( "out CycleCanvas::OnMouseMove 4" );
        return;
    }

    //if we are in layout mode and the right mouse button is down,
    // then allow the user to scale (resize) the image.
    if (mf->mWhichMode==CycleFrame::ModeLayout && e.RightIsDown()) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z )) {
            this->lastX = wx;
            this->lastY = wy;
            return;
        }
        CycleData*  data = dynamic_cast< CycleData*>( getDataset(d) );
        int  dx = wx - this->lastX;
        if (dx!=0) {  //scale the image
            dx *= 2;
            //determine the current width
            const double  cw = data->m_scale * data->m_xSpacing * data->m_xSize;
            //determine the target width
            const double  tw = cw + dx;
            //determine the new scale factor necessary to accomplish this change in size
            const double  sc = tw / (data->m_xSpacing * data->m_xSize);
            if (0.3 <= sc && sc <= 10.0) {
                data->m_scale = sc;
                data->mWX -= dx/2;
                data->mWY -= dx/2;
                reload();
            }
        }
        this->lastX = wx;
        this->lastY = wy;
        wxLogMessage( "out CycleCanvas::OnMouseMove 5" );
        return;
    }

    //roi mode
    if (mf->mWhichMode==CycleFrame::ModeRoi && e.LeftIsDown()) {
        //if we are moving (dragging) in a roi,
        // then move the roi in the original data (which may also be another roi)
        if (mf->mWhichRoiMode == CycleFrame::RoiModeMoveOrResize) {
            int  d, x, y, z;
            if (!mapWindowToData( lx, ly, d, x, y, z )) {  //in any data?
                this->lastX = wx;
                this->lastY = wy;
                return;
            }
            CycleData*  cd = dynamic_cast<CycleData*>( getDataset(d) );
            if (!cd->mHasRoi) {  //in a roi?
                this->lastX = wx;
                this->lastY = wy;
                return;
            }
            const int  dx = wx - this->lastX;
            const int  dy = wy - this->lastY;
            if (dx || dy) {  //did the mouse actually move at all?
                assert( cd->mRoiData != NULL );
                int   dp, xp, yp, zp;
                bool  result = mapWindowToData( lx, ly, cd->mRoiData, dp, xp, yp, zp );
                if (!result) {  //possibly moving outside of parental roi?
                    this->lastX = wx;
                    this->lastY = wy;
                    return;
                }

                const double  xFactor = cd->m_scale * cd->m_xSpacing;
                const double  yFactor = cd->m_scale * cd->m_ySpacing;
                //move (translate) the image
                //int  newWX = lx;
                int  newWX = 0;
                int  newWY = ly;
                //center the roi
                int  newRoiX = cd->mRoiData->mRoiX + xp - cd->mRoiWidth/2;
                int  newRoiY = cd->mRoiData->mRoiY + yp - cd->mRoiHeight/2;
                //when centered, does the upper left of the roi extend beyond
                // (before) the upper-left corner of the image?
                if (newRoiX < 0) {
                    newRoiX = 0;
                    newWX   = cd->mRoiData->mWX;
                } else if (newRoiX + cd->mRoiWidth >= cd->m_xSize) {
                    newRoiX = cd->m_xSize - cd->mRoiWidth;
                    newWX   = (int)(cd->mRoiData->mWX + newRoiX*xFactor);
                } else {
                    newWX = (int)(lx - cd->mRoiWidth/2 * xFactor);
                }
                if (newRoiY < 0) {
                    newRoiY = 0;
                    newWY   = cd->mRoiData->mWY;
                } else if (newRoiY + cd->mRoiHeight >= cd->m_ySize) {
                    newRoiY = cd->m_ySize - cd->mRoiHeight;
                    newWY   = (int)(cd->mRoiData->mWY + newRoiY*yFactor);
                } else {
                    newWY = (int)(ly - cd->mRoiHeight/2 * yFactor);
                }

                cd->mRoiX = newRoiX;
                cd->mRoiY = newRoiY;
                cd->mWX   = newWX;
                cd->mWY   = newWY;

                assert( cd->mRoiX + cd->mRoiWidth  <= cd->m_xSize );
                assert( cd->mRoiY + cd->mRoiHeight <= cd->m_ySize );
                reload();
            }
            this->lastX = wx;
            this->lastY = wy;
            wxLogMessage( "out CycleCanvas::OnMouseMove 6a" );
            return;
        }
#if 0
        //if we are moving (dragging) in a roi, then move the roi in the original data
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z ))    return;
        CycleData*  cd = dynamic_cast<CycleData*>( getDataset(d) );
        if (!cd->mHasRoi)    return;
        int  dx = wx - this->lastX;
        int  dy = wy - this->lastY;
        //move (translate) the image
        //cd->mWX += dx;
        //cd->mWY += dy;
        cd->mWX += 1;
        cd->mWY += 1;
        cd->mRoiX  += 1;
        cd->mRoiY  += 1;

        this->lastX = wx;
        this->lastY = wy;
        reload();
#endif
        wxLogMessage( "out CycleCanvas::OnMouseMove 6b" );
        return;
    }

    //roi mode
    if (mf->mWhichMode==CycleFrame::ModeRoi && e.MiddleIsDown()) {
        if (mf->mWhichRoiMode == CycleFrame::RoiModeMoveOrResize) {
            //if we are moving (dragging) in a roi,
            // then change the size of the roi (not magnify but increase
            // or decrease the number of pixels from the original data
            // used to create the roi)
            int  d, x, y, z;
            if (!mapWindowToData( lx, ly, d, x, y, z ))    return;
            CycleData*  cd = dynamic_cast<CycleData*>( getDataset(d) );
            if (!cd->mHasRoi)    return;
            //double  xFactor = cd->m_scale * cd->m_xSpacing;
            //double  yFactor = cd->m_scale * cd->m_ySpacing;
            int  dx = wx - this->lastX;
			if (dx==0) {
                this->lastX = wx;
                this->lastY = wy;
				return;
			}

			//within left-top bounds?
			if (cd->mRoiX-dx < 0)    dx += cd->mRoiX-dx;
			if (cd->mRoiY-dx < 0)    dx += cd->mRoiY-dx;

			int  newRoiX = cd->mRoiX - dx;
			int  newRoiY = cd->mRoiY - dx;
			int  newRoiWidth  = cd->mRoiWidth  + 2*dx;
			int  newRoiHeight = cd->mRoiHeight + 2*dx;
			if (newRoiX+newRoiWidth > cd->m_xSize || newRoiY+newRoiHeight > cd->m_ySize) {
                this->lastX = wx;
                this->lastY = wy;
				return;
			}

			cd->mRoiX = newRoiX;
            cd->mRoiY = newRoiY;
            cd->mWX   -= dx;
            cd->mWY   -= dx;
            cd->mRoiWidth  = newRoiWidth;
            cd->mRoiHeight = newRoiHeight;

            this->lastX = wx;
            this->lastY = wy;
            reload();
        }
        wxLogMessage( "out CycleCanvas::OnMouseMove 7" );
        return;
    }

    SetCursor( *wxSTANDARD_CURSOR );
    lastX = lastY = -1;
    int  d, x, y, z;
    if (mapWindowToData( lx, ly, d, x, y, z )) {
        wxString  s;
        s.Printf( wxT("%d:(%d,%d,%d)=%d -> %d"), d+1, x+1, y+1, z+1,
            getDataset(d)->getData(x,y,z),
            getDataset(d)->m_lut[ getDataset(d)->getData(x,y,z) - getDataset(d)->m_min ] );
        m_parent_frame->SetStatusText( s, 1 );
    } else {
        m_parent_frame->SetStatusText( "", 1 );
    }
#if 0
    if (mf->mWhichMode==CycleFrame::ModeMove && e.LeftIsDown()) {
        if (lastX==-1 || lastY==-1) {
            SetCursor( wxCursor(wxCURSOR_HAND) );
            lastX = wx;
            lastY = wy;
            return;
        }

        //wxLogMessage( "lastX=%d, lastY=%d, wx=%d, wy=%d", lastX, lastY, wx, wy );
        bool  changed=false;
        if ((wx-lastX)/2 != 0) {
            m_tx += (wx-lastX)/2;
            changed = true;
        }
        if ((wy-lastY)/2 != 0) {
            m_ty += (wy-lastY)/2;
            changed = true;
        }
        if (changed)  Refresh();
        lastX = wx;
        lastY = wy;
        m_parent_frame->SetStatusText( "", 1 );
    } else if (mf->mWhichMode==CycleFrame::ModeMagnify) {
        mouseMoveMagnify( e );
        m_parent_frame->SetStatusText( "", 1 );
    } else {  //show the pixel value under the pointer (if any)
        int  d, x, y, z;
        //check the magnify vector first.  if that fails, try the slice data.
        if ( mapWindowToMagnifyVector( lx, ly, d, x, y, z )>=0 ||
             mapWindowToData( lx, ly, d, x, y, z ) )
        {
            wxString  s;
            if (d>=0 && x>=0 && y>=0 && z>=0) {
                s.Printf( wxT("%d:(%d,%d,%d)=%d -> %d"), d+1, x+1, y+1, z+1,
                    getDataset(d)->getData(x,y,z),
                    getDataset(d)->m_lut[ getDataset(d)->getData(x,y,z) - getDataset(d)->m_min ] );
            }
            m_parent_frame->SetStatusText( s, 1 );
        } else {
            m_parent_frame->SetStatusText( "", 1 );
        }

        if (lastX!=-1 || lastY!=-1) {
            SetCursor( *wxSTANDARD_CURSOR );
            lastX = lastY = -1;
        }
    }
#endif
    wxLogMessage( "out CycleCanvas::OnMouseMove 8" );
}
//----------------------------------------------------------------------
/** \brief handle mouse move events when in magnify mode. */
void CycleCanvas::mouseMoveMagnify ( wxMouseEvent& e ) {
    if (e.LeftIsDown())    return;
    if (mMagnifyVector.empty())    return;
    if (mMagnifyState==MagnifyStateChoose)    return;

    wxClientDC  dc(this);
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  lx = dc.DeviceToLogicalX( pos.x );
    const long  ly = dc.DeviceToLogicalY( pos.y );
    //const long  wx = lx - m_tx;
    //const long  wy = ly - m_ty;

    //last element of magnify vector is always the one that is selected
    // (for move or mag)
    Magnify*  m = &mMagnifyVector[ mMagnifyVector.size() - 1 ];
//    int  oldX = m->mSX;
//    int  oldY = m->mSY;
    int  oldX = lastX,  oldY = lastY;
    if (lx==oldX && ly==oldY)    return;
    lastX = lx;  lastY = lx;
    if (mMagnifyState==MagnifyStatePosition) {
        //move the last element of the magnify vector
        m->mSX = lx;
        m->mSY = ly;
    } else if (mMagnifyState==MagnifyStateSize) {
        if      (lx>oldX)    m->mScale += 0.1;
        else if (lx<oldX)    m->mScale -= 0.1;
        else                 return;  //didn't move
//        m->mSX = lx;
//        m->mSY = ly;
//        WarpPointer( m->mSX, m->mSY );
        CavassData*   cd      = getDataset( m->mDataset );
        const double  xFactor = m->mScale * cd->m_xSpacing;
        const double  yFactor = m->mScale * cd->m_ySpacing;
        const int     scaledW = (int)ceil( cd->m_xSize * xFactor );
        const int     scaledH = (int)ceil( cd->m_ySize * yFactor );
        if (scaledW<100 || scaledH<100)    return;  //too small!

        const int  keep_sliceNo = cd->m_sliceNo;
        cd->m_sliceNo = m->mSlice;

        //note: image data is 24-bit rgb
        if (xFactor==1.0 && yFactor==1.0) {
            m->mImage = new wxImage( cd->m_xSize, cd->m_ySize, ::toRGB(*cd) );
        } else if (!mInterpolate) {
            m->mImage = new wxImage( cd->m_xSize, cd->m_ySize, ::toRGB(*cd) );
            m->mImage->Rescale( scaledW, scaledH );
        } else {
            m->mImage = new wxImage( (int)(cd->m_xSize*xFactor),
                (int)(cd->m_ySize*yFactor),
                ::toRGBInterpolated(*cd, xFactor, yFactor) );
        }

        m->mBitmap = new wxBitmap( (const wxImage&) *(m->mImage) );
        cd->m_sliceNo = keep_sliceNo;
    }
    Refresh();
}
//----------------------------------------------------------------------
void CycleCanvas::moveToNextSlice ( void ) {
    ++m_sliceNo;
    bool  found = false;
    int   i = -1;
    //search forward starting with the current dataset to find a dataset
    // with enough slices
    for (i=mFirstDisplayedDataset; i<mFileOrDataCount; i++) {
        if (m_sliceNo < getDataset(i)->m_zSize) {
            found = true;
            break;
        }
    }
    if (found)    mFirstDisplayedDataset = i;
    else {
        //we didn't find a dataset with enough slices so we must wrap
        // around to the beginning of the list and move forward through the
        // displayed slices
        for (i=0; i<mFileOrDataCount; i++) {
            if (m_sliceNo < getDataset(i)->m_zSize) {
                found = true;
                break;
            }
        }
        if (found)    mFirstDisplayedDataset = i;
    }

    if (mFirstDisplayedDataset >= mFileOrDataCount)    mFirstDisplayedDataset = mFileOrDataCount-1;
    if (mFirstDisplayedDataset < 0)                    mFirstDisplayedDataset = 0;
    if (m_sliceNo >= mMaxSlices)    m_sliceNo = mMaxSlices-1;
    if (m_sliceNo < 0)              m_sliceNo = 0;
}
//----------------------------------------------------------------------
void CycleCanvas::moveToNextRow ( void ) {
    bool  found = false;
    int   i = -1;
    //search forward starting after the current dataset to find a dataset
    // with enough slices
    for (i=mFirstDisplayedDataset+1; i<mFileOrDataCount; i++) {
        if (m_sliceNo < getDataset(i)->m_zSize) {
            found = true;
            break;
        }
    }
    if (found)    mFirstDisplayedDataset = i;
    else {
        //we didn't find a dataset with enough slices so we must wrap
        // around to the beginning of the list and move forward through the
        // displayed slices
        m_sliceNo += m_cols;
        for (i=0; i<mFileOrDataCount; i++) {
            if (m_sliceNo < getDataset(i)->m_zSize) {
                found = true;
                break;
            }
        }
        if (found)    mFirstDisplayedDataset = i;
    }

    if (mFirstDisplayedDataset >= mFileOrDataCount)    mFirstDisplayedDataset = mFileOrDataCount-1;
    if (mFirstDisplayedDataset < 0)                    mFirstDisplayedDataset = 0;
    if (m_sliceNo >= mMaxSlices)    m_sliceNo = mMaxSlices-1;
    if (m_sliceNo < 0)              m_sliceNo = 0;
}
//----------------------------------------------------------------------
void CycleCanvas::moveToPrevSlice ( void ) {
    --m_sliceNo;
    if (m_sliceNo<0)    m_sliceNo = 0;
    bool  found = false;
    int   i = -1;
    //search backward starting with the current dataset to find a dataset
    // with enough slices
    for (i=mFirstDisplayedDataset; i>=0; i--) {
        if (m_sliceNo < getDataset(i)->m_zSize) {
            found = true;
            break;
        }
    }
    if (found)    mFirstDisplayedDataset = i;
    else {
        //we didn't find a dataset with enough slices so we must wrap
        // around to the end of the list and move backward through the
        // displayed slices
        for (i=mFileOrDataCount-1; i>=0; i--) {
            if (m_sliceNo < getDataset(i)->m_zSize) {
                found = true;
                break;
            }
        }
        if (found)    mFirstDisplayedDataset = i;
    }
    if (!found)    --mFirstDisplayedDataset;

    if (mFirstDisplayedDataset >= mFileOrDataCount)    mFirstDisplayedDataset = mFileOrDataCount-1;
    if (mFirstDisplayedDataset < 0)                    mFirstDisplayedDataset = 0;
    if (m_sliceNo >= mMaxSlices)    m_sliceNo = mMaxSlices-1;
    if (m_sliceNo < 0)              m_sliceNo = 0;
}
//----------------------------------------------------------------------
void CycleCanvas::moveToPrevRow ( void ) {
    bool  found = false;
    int   i = -1;
    //search backward starting before the current dataset to find a dataset
    // with enough slices
    for (i=mFirstDisplayedDataset-1; i>=0; i--) {
        if (m_sliceNo < getDataset(i)->m_zSize) {
            found = true;
            break;
        }
    }
    if (found)    mFirstDisplayedDataset = i;
    else {
        //we didn't find a dataset with enough slices so we must wrap
        // around to the end of the list and move backward through the
        // displayed slices
        m_sliceNo -= m_cols;
        for (i=mFileOrDataCount-1; i>=0; i--) {
            if (0 <= m_sliceNo && m_sliceNo < getDataset(i)->m_zSize) {
                found = true;
                break;
            }
        }
        if (found)    mFirstDisplayedDataset = i;
    }

    if (mFirstDisplayedDataset >= mFileOrDataCount)    mFirstDisplayedDataset = mFileOrDataCount-1;
    if (mFirstDisplayedDataset < 0)                    mFirstDisplayedDataset = 0;
    if (m_sliceNo >= mMaxSlices)    m_sliceNo = mMaxSlices-1;
    if (m_sliceNo < 0)              m_sliceNo = 0;
}
//----------------------------------------------------------------------
void CycleCanvas::OnReset ( void ) {
    mInterpolate = false;
    mMagnifyVector.clear();
    mMagnifyState = MagnifyStateChoose;

    mMeasureVector.clear();
    m_tx = m_ty = 0;
    setFirstDisplayedDataset( 0 );
    setSliceNo( 0 );
    for (int i=0; i<mFileOrDataCount; i++) {
        int  max = getMax( i );
        setCenter( i, max/2 );
        setWidth(  i, max );
        setInvert( i, false );
        initLUT( i );
    }
    setScale( 1.0 );
    CycleCanvas::Measure::sCount = 1;
    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );
    if (mf->mTextControls && mf->mTextControls->mList)
        mf->mTextControls->mList->Clear();
}
//----------------------------------------------------------------------
void CycleCanvas::OnLeftDown ( wxMouseEvent& e ) {
    if (mCavassData==NULL || mCavassData->m_data==NULL)    return;

    //determine the location of the click in the window
    wxClientDC  dc(this);
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  lx = dc.DeviceToLogicalX( pos.x );
    const long  ly = dc.DeviceToLogicalY( pos.y );
    const long  wx = lx - m_tx;
    const long  wy = ly - m_ty;

    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );

    if (mf->mWhichMode == CycleFrame::ModeCine) {
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        mf->m_cine_timer->Start( ::gTimerInterval, true );
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeContinuous) {
        this->lastX = wx;
        this->lastY = wy;
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeDiscontinuous) {
        this->lastX = wx;
        this->lastY = wy;
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeErase) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z ))    return;
        //find this data set in our list and delete it
        int  i = 0;
        CycleData*  last = NULL;
        CycleData*  data = dynamic_cast< CycleData*>( mCavassData );
        while (data!=NULL) {
            if (i==d) {
                //remove from the head?
                if (i==0)    mCavassData = mCavassData->mNext;
                else         last->mNext = data->mNext;
                delete data;
                data = NULL;
                --mFileOrDataCount;
                break;
            }
            i++;
            last = data;
            data = dynamic_cast< CycleData*>( data->mNext );
        }
        reload();
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeFastCine) {
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        mf->m_cine_timer->Start( ::gTimerInterval, true );
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeLayout) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z ))    return;
        SetCursor( wxCursor(wxCURSOR_HAND) );
        mf->SetStatusText( "drag to move", 0 );
        sendToEnd( d );  //to draw it (entire image) frontmost
        this->lastX = wx;
        this->lastY = wy;
        reload();
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeLock) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z ))    return;
        CycleData*  data = dynamic_cast<CycleData*>( getDataset(d) );
        data->mLocked = !data->mLocked;
        Refresh();
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeMeasure) {
        handleMeasureMode( e );
        return;
    }

    //roi mode
    if (mf->mWhichMode == CycleFrame::ModeRoi) {
        SetCursor( wxCursor(wxCURSOR_HAND) );
        if (mf->mWhichRoiMode == CycleFrame::RoiModeSelect) {  //create a new roi?
            mf->mWhichRoiMode = CycleFrame::RoiModeMoveOrResize;
            mf->SetStatusText( "move roi",   2 );
            mf->SetStatusText( "resize roi", 3 );
            mf->SetStatusText( "done",       4 );
            int  d, x, y, z;
            if (!mapWindowToData( lx, ly, d, x, y, z )) {
                this->lastX = wx;
                this->lastY = wy;
                return;
            }
			//don't allow roi's of roi's
			if (dynamic_cast<CycleData*>( getDataset(d) )->mHasRoi) {
                mf->mWhichRoiMode = CycleFrame::RoiModeSelect;
				this->lastX = wx;
                this->lastY = wy;
                return;
			}
            //make a new entry in our list of data that is a duplicate and refers to
            // an entry that is already in our list.
            CycleData*  cd  = dynamic_cast<CycleData*>( getDataset(d) );
            CycleData*  tmp = new CycleData( *cd );
			if ((!tmp->mIsCavassFile || !tmp->m_vh_initialized) &&
					!tmp->mIsDicomFile && !cd->mIsImageFile)
			{
				wxMessageBox("Failed to load file.");
				return;
			}
            const double  xFactor = tmp->m_scale * tmp->m_xSpacing;
            const double  yFactor = tmp->m_scale * tmp->m_ySpacing;
            tmp->mHasRoi  = true;
            tmp->mNumber  = ++mFileOrDataCount;
            tmp->mRoiData = cd;
            tmp->mWX      = lx;
            tmp->mWY      = ly;
            tmp->mRoiX    += x;
            tmp->mRoiY    += y;
            //determine roi width & height
            if (cd->mHasRoi) {  //is this new roi created from an existing roi?
                tmp->mRoiWidth  -= x;
                tmp->mRoiHeight -= y;
            } else {  //new roi created from an image (not an existing roi)
                tmp->mRoiWidth  = cd->m_xSize - x;
                tmp->mRoiHeight = cd->m_ySize - y;
            }
            //make the roi square
            if (tmp->mRoiWidth > tmp->mRoiHeight)    tmp->mRoiWidth  = tmp->mRoiHeight;
            else                                     tmp->mRoiHeight = tmp->mRoiWidth;
            //make sure that the roi is not too big
            if (tmp->mRoiWidth  > 100)    tmp->mRoiWidth  = 100;
            if (tmp->mRoiHeight > 100)    tmp->mRoiHeight = 100;
            //center the roi about (WX,WY)
            tmp->mRoiX -= tmp->mRoiWidth;
            tmp->mRoiY -= tmp->mRoiHeight;
            //when centered, does the upper left of the roi extend beyond
            // (before) the upper-left corner of the image?
            if (tmp->mRoiX < 0) {
                tmp->mRoiWidth += tmp->mRoiX;
                tmp->mRoiX = 0;
            }
            if (tmp->mRoiY < 0) {
                tmp->mRoiHeight += tmp->mRoiY;
                tmp->mRoiY = 0;
            }
            //again, make sure the roi is square
            if (tmp->mRoiWidth > tmp->mRoiHeight)    tmp->mRoiWidth  = tmp->mRoiHeight;
            else                                     tmp->mRoiHeight = tmp->mRoiWidth;
            tmp->mWX   -= (int)(tmp->mRoiWidth  * xFactor);
            tmp->mWY   -= (int)(tmp->mRoiHeight * yFactor);
            tmp->mRoiWidth  *= 2;
            tmp->mRoiHeight *= 2;
            //insert it at the beginning and then move it to the end
            // so that it will be drawn frontmost.
            tmp->mNext  = mCavassData;
            mCavassData = tmp;
            sendToEnd( 0 );
            freeImagesAndBitmaps();  //based on current value of m_cols
            ++m_cols;  //update now
            reload();
            this->lastX = wx;
            this->lastY = wy;
            return;
        }
        if (mf->mWhichRoiMode == CycleFrame::RoiModeMoveOrResize) {  //move or resize an existing roi?
            this->lastX = wx;
            this->lastY = wy;
            return;
        }
    }

    SetCursor( *wxSTANDARD_CURSOR );

}
//----------------------------------------------------------------------
void CycleCanvas::handleMagnifyMode ( wxMouseEvent& e ) {
    //change to the next state
    if (mMagnifyState == MagnifyStateChoose) {
        wxClientDC  dc(this);
        PrepareDC( dc );
        const wxPoint  pos = e.GetPosition();
        int  lx = dc.DeviceToLogicalX( pos.x );
        int  ly = dc.DeviceToLogicalY( pos.y );
        //is the user trying to re-select an element of the magnify vector
        // that's already on the screen?
        int  i = mapWindowToMagnifyVector( lx, ly );
        if (i>=0) {
            int  k = 0;
            for (MagnifyVector::iterator j=mMagnifyVector.begin(); j!=mMagnifyVector.end(); j++) {
                if (k==i) {
                    Magnify  m = *j;
                    mMagnifyVector.erase( j );
                    mMagnifyVector.push_back( m );
                    mMagnifyState = MagnifyStatePosition;
                    Refresh();
                    return;
                }
                k++;
            }
            assert( 0 );  //should never get here!
        }

        int  d, x, y, z;
        mapWindowToData( lx, ly, d, x, y, z );

        if (d<0)    return;
        CavassData*  cd = getDataset( d );
        const int  keep_sliceNo = cd->m_sliceNo;
        cd->m_sliceNo = z;
        const double  xFactor = cd->m_scale * cd->m_xSpacing;
        const double  yFactor = cd->m_scale * cd->m_ySpacing;

        //note: image data is 24-bit rgb
        wxImage*  image = 0;
        if (xFactor==1.0 && yFactor==1.0) {
            image = new wxImage( cd->m_xSize, cd->m_ySize, ::toRGB(*cd) );
        } else if (!mInterpolate) {
            image = new wxImage( cd->m_xSize, cd->m_ySize, ::toRGB(*cd) );
            const int  scaledW = (int)ceil( cd->m_xSize * cd->m_xSpacing * cd->m_scale );
            const int  scaledH = (int)ceil( cd->m_ySize * cd->m_ySpacing * cd->m_scale );
            image->Rescale( scaledW, scaledH );
        } else {
            image = new wxImage( (int)(cd->m_xSize*xFactor),
                (int)(cd->m_ySize*yFactor),
                ::toRGBInterpolated(*cd, xFactor, yFactor) );
        }

        wxBitmap*  bitmap = new wxBitmap( (const wxImage&) *image );
        cd->m_sliceNo = keep_sliceNo;

        Magnify  m( d, z, cd->m_scale, lx, ly, image, bitmap );
        mMagnifyVector.push_back( m );

        mMagnifyState = MagnifyStatePosition;
    } else if (mMagnifyState == MagnifyStatePosition) {
        mMagnifyState = MagnifyStateSize;
    } else if (mMagnifyState == MagnifyStateSize) {
        Magnify*  m = &mMagnifyVector[ mMagnifyVector.size() - 1 ];
        WarpPointer( m->mSX, m->mSY );
        mMagnifyState = MagnifyStatePosition;
    }
    Refresh();
}
//----------------------------------------------------------------------
void CycleCanvas::handleMeasureMode ( wxMouseEvent& e ) {
    wxClientDC  dc(this);
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    int  lx = dc.DeviceToLogicalX( pos.x );
    int  ly = dc.DeviceToLogicalY( pos.y );
    int  d, x, y, z;
    MeasureVector*  mv = 0;
    //is the user trying to measure an element of the magnify vector
    // that's already on the screen?
    int  i = mapWindowToMagnifyVector( lx, ly, d, x, y, z );
    if (i>=0) {
        mv = &mMagnifyVector[i].mMeasureVector;
    } else if (mapWindowToData( lx, ly, d, x, y, z )) {
        mv = &mMeasureVector;
    } else {
        return;
    }
    CycleData*  cd = dynamic_cast<CycleData*>( getDataset( d ) );
    if (cd->mHasRoi) {  //include offset to roi (if any)
        x += cd->mRoiX;
        y += cd->mRoiY;
    }
    int  value = cd->getData( x, y, z );
    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );

    FILE *fp=NULL;
    if (mSaveFile != wxEmptyString)
    {
        fp = fopen((const char *)mSaveFile.c_str(), "a");
        if (fp == NULL)
            wxMessageBox("Could not open file.");
    }

    //is this the first or second point in a pair of points?
    if (mv->empty() || (*mv)[mv->size()-1].mSecondSpecified) {
        //specify the first point for a new entry
        Measure  m( d,
            x*cd->m_xSpacing, y*cd->m_ySpacing, z*cd->m_zSpacing,
            x, y, z, value );
        mv->push_back( m );
        wxString s= wxString::Format( "%d: (%.2f,%.2f,%.2f) = %d", (m.mID+1)/2,
            m.mX1, m.mY1, m.mZ1, m.mValue1 );
        mf->mTextControls->insert( s );
        mf->mMeasurements.Add( s );
        if (fp)
        {
            if (fprintf(fp, "%s\n",(const char *)s.c_str()) < 1)
                wxMessageBox("Could not save text.");
            fclose(fp);
        }
    } else {
        //specify the second point for an existing entry
        const int  i = mv->size()-1;
        //are the points in the same dataset?
        int  oldD = (*mv)[i].mDataset;
        if (d != oldD)    return;  //ignore if not in same dataset
        (*mv)[i].setSecondPoint(
            x*cd->m_xSpacing, y*cd->m_ySpacing, z*cd->m_zSpacing,
            x, y, z, value );
        wxString  s = wxString::Format( "%d: (%.2f,%.2f,%.2f) = %d",
            (*mv)[i].mID/2+2, (*mv)[i].mX2, (*mv)[i].mY2, (*mv)[i].mZ2,
            (*mv)[i].mValue2 );
        mf->mTextControls->insert( s );
        if (fp)
            if (fprintf(fp, "%s\n",(const char *)s.c_str()) < 1)
                wxMessageBox("Could not save text.");
        double cumul=0.;
        for (int j=0; j<mv->size(); j++)
            if ((*mv)[j].mDataset == d)
                cumul += (*mv)[j].getDistance();
        wxString  t = wxString::Format( "    d=%.2f", cumul );
        mf->mTextControls->insert( t );
        if (fp)
        {
            if (fprintf(fp, "%s\n",(const char *)t.c_str()) < 1)
                wxMessageBox("Could not save text.");
            fclose(fp);
        }
        mf->mMeasurements.Add( s+"\n"+t );
        //specify the first point for a new entry
        Measure  m( d,
            x*cd->m_xSpacing, y*cd->m_ySpacing, z*cd->m_zSpacing,
            x, y, z, value );
        mv->push_back( m );
    }
    Refresh();
}
//----------------------------------------------------------------------
void CycleCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage( "OnLeftUp" );
    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );
    mf->indicateMode();
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void CycleCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    wxLogMessage( "CycleCanvas::OnMiddleDown" );
//    OnMouseMove( e );
    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );
    if (mf->mWhichMode==CycleFrame::ModeCine) {
        mf->mDirectionIsForward = !mf->mDirectionIsForward;
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        mf->m_cine_timer->Start( ::gTimerInterval, true );
        return;
    }

    //previous slice
    if (mf->mWhichMode == CycleFrame::ModeDiscontinuous) {
        //const int  slices = getNoSlices();
        int  s = getSliceNo();
        --s;
        if (s >= 0) {
            mf->setSliceNo( s );
            setSliceNo( s );
            reload();
        }
        return;
    }

    //remove one of the measure points
    if (mf->mWhichMode==CycleFrame::ModeMeasure && !mMeasureVector.empty()) {

        // remove the first point for a new entry
        mf->mMeasurements.RemoveAt(mf->mMeasurements.GetCount()-1);
        mMeasureVector.pop_back();
        CycleCanvas::Measure::sCount -= 2;
        if (mMeasureVector.size() > 0)
            mMeasureVector[mMeasureVector.size()-1].mSecondSpecified = false;
        mf->mTextControls->mList->Clear();
        for (unsigned int j=0; j<mf->mMeasurements.GetCount(); j++)
            mf->mTextControls->insert( mf->mMeasurements[j] );
        Refresh();
        return;
    }

	//roi mode
    if (mf->mWhichMode == CycleFrame::ModeRoi) {
        SetCursor( wxCursor(wxCURSOR_HAND) );
		//determine the location of the click in the window
		wxClientDC  dc(this);
		PrepareDC( dc );
		const wxPoint  pos = e.GetPosition();
		//remove translation
		const long  lx = dc.DeviceToLogicalX( pos.x );
		const long  ly = dc.DeviceToLogicalY( pos.y );
		const long  wx = lx - m_tx;
		const long  wy = ly - m_ty;

        this->lastX = wx;
        this->lastY = wy;
        return;
	}

#if 0
    switch (mf->mWhichMode) {
        case CycleFrame::ModeMagnify :
            mMagnifyState = MagnifyStateChoose;
            break;
        case CycleFrame::ModeMeasure :
            break;
        case CycleFrame::ModeMove :
            break;
        case CycleFrame::ModePage :
            //page mode / display the next screenful
            for (int row=0; row<m_rows; row++) {
                moveToPrevRow();
            }
            break;
        case CycleFrame::ModeScroll :
            moveToPrevRow();  //display the next row
            break;
        case CycleFrame::ModeSlice :
            moveToPrevSlice();
            break;
        default :
            assert( 0 );
            break;
    }
    mf->setSliceNo( m_sliceNo );
    reload();
#endif
}
//----------------------------------------------------------------------
void CycleCanvas::OnMiddleUp ( wxMouseEvent& unused ) {
    cout << "OnMiddleUp" << endl;
    wxLogMessage( "OnMiddleUp" );
}
//----------------------------------------------------------------------
void CycleCanvas::OnRightDown ( wxMouseEvent& e ) {
    if (mCavassData==NULL || mCavassData->m_data==NULL)    return;

    //determine the location of the click in the window
    wxClientDC  dc(this);
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  lx = dc.DeviceToLogicalX( pos.x );
    const long  ly = dc.DeviceToLogicalY( pos.y );
    const long  wx = lx - m_tx;
    const long  wy = ly - m_ty;

    CycleFrame*  mf = dynamic_cast<CycleFrame*>( m_parent_frame );
    if (mf->mWhichMode == CycleFrame::ModeLayout) {
        int  d, x, y, z;
        if (!mapWindowToData( lx, ly, d, x, y, z )) {
            sendToEnd( 0 );
            reload();
            return;
        }
        SetCursor( wxCursor(wxCURSOR_HAND) );
        mf->SetStatusText( "drag to scale", 0 );
        sendToEnd( d );  //to draw it (entire image) frontmost
        this->lastX = wx;
        this->lastY = wy;
        reload();
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeCine) {
        mf->m_cine_timer->Stop();
        return;
    }

    //next slice
    if (mf->mWhichMode == CycleFrame::ModeDiscontinuous) {
        const int  slices = getNoSlices();
        int  s = getSliceNo();
        ++s;
        if (s < slices) {
            mf->setSliceNo( s );
            setSliceNo( s );
            reload();
        }
        return;
    }

    //remove all of the measure points
    if (mf->mWhichMode == CycleFrame::ModeMeasure) {
        if (mSaveFile == wxEmptyString)
        {
            wxCommandEvent ce;
            mf->OnSave(ce);
        }
        mf->mMeasurements.Empty();
        if (mf->mTextControls!=NULL) {
            mf->mTextControls->mList->Clear();
        }

        mMeasureVector.clear();
        CycleCanvas::Measure::sCount = 1;
        Refresh();
        return;
    }

    if (mf->mWhichMode == CycleFrame::ModeRoi) {
        mf->mWhichRoiMode = CycleFrame::RoiModeSelect;
        mf->SetStatusText( "select", 2 );
        mf->SetStatusText( "",       3 );
        mf->SetStatusText( "",       4 );
        return;
    }

#if 0
    switch (mf->mWhichMode) {
        case CycleFrame::ModeMagnify :
            //cancel
            if (!mMagnifyVector.empty()) {
                mMagnifyVector.pop_back();    //remove the last entry
            }
            mMagnifyState = MagnifyStateChoose;
            Refresh();
            break;
        default :
            OnMiddleDown( e );
            break;
    }
#endif
}
//----------------------------------------------------------------------
void CycleCanvas::OnRightUp ( wxMouseEvent& unused ) {
    cout << "OnRightUp" << endl;
    wxLogMessage( "OnRightUp" );
}
//----------------------------------------------------------------------
void CycleCanvas::OnPaint ( wxPaintEvent& e ) {
    int  w, h;
    GetSize( &w, &h );
    wxBitmap  bitmap( w, h );
    wxMemoryDC  m;
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
    
    wxPaintDC  dc( this );
    PrepareDC( dc );
    dc.Blit( 0, 0, w, h, &m, 0, 0 );  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
}
//----------------------------------------------------------------------
void CycleCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
    
    if (m_bitmaps!=NULL) {
        if (!mDataInitialized) {
            const int  y = sSpacing;
            int  i = 0;
            CycleData*  data = dynamic_cast<CycleData*>( mCavassData );
            while (data!=NULL) {
                assert( m_bitmaps[i]!=NULL );
                assert( m_bitmaps[i]->Ok() );
                const int  x = i * (mMaxPixelWidth + sSpacing);
                data->mWX = x+m_tx;
                data->mWY = y+m_ty;
                data->mNumber = i+1;

                data = dynamic_cast<CycleData*>( data->mNext );
                i++;
            }
            mDataInitialized = true;
        }
        int  i = 0;
        CycleData*  data = dynamic_cast<CycleData*>( mCavassData );
        while (data!=NULL) {
            if (m_bitmaps!=NULL && m_bitmaps[i]!=NULL && m_bitmaps[i]->Ok()) {
                if (data->mLocked)    dc->SetPen( wxPen(wxColour(Red)) );
                else                  dc->SetPen( wxPen(wxColour(Yellow)) );
                dc->DrawRectangle( data->mWX-1, data->mWY-1,
                    m_bitmaps[i]->GetWidth()+2, m_bitmaps[i]->GetHeight()+2 );
                dc->DrawBitmap( *m_bitmaps[i], data->mWX, data->mWY );
                if (mCavassData->m_overlay) {
                    if (data->mHasRoi) {
                        wxString  s = wxString::Format( "%d (ROI):%d", data->mNumber, data->m_sliceNo+1 );
                        dc->DrawText( s, data->mWX, data->mWY );
                    } else {
                        wxString  s = wxString::Format( "%d:%d", data->mNumber, data->m_sliceNo+1 );
                        dc->DrawText( s, data->mWX, data->mWY );
                    }
                    paintMeasure( dc, data->mWX, data->mWY, i, data->m_sliceNo );
                }
            }

            data = dynamic_cast<CycleData*>( data->mNext );
            i++;
        }
#if 0
        int  dataset = mFirstDisplayedDataset;
        int  sliceNo = m_sliceNo;
        int  i = 0;
        for (int r=0; r<m_rows; r++) {
            const int  y = r * (mMaxPixelHeight + sSpacing);
            for (int c=0; c<m_cols; c++) {
                if (m_bitmaps[i]!=NULL && m_bitmaps[i]->Ok()) {
                    const int  x = c * (mMaxPixelWidth + sSpacing);
                    dc->DrawBitmap( *m_bitmaps[i], x+m_tx, y+m_ty );
                    if (mCavassData->m_overlay) {
                        wxString  s = wxString::Format( "%d", sliceNo+c+1 );
                        if (mFileOrDataCount>=1 && c==0)
                            s = wxString::Format( "%d:%d", dataset+1, sliceNo+c+1 );
                        dc->DrawText( s, x+m_tx, y+m_ty );
                        paintMeasure( dc, x+m_tx, y+m_ty, dataset, sliceNo+c );
                    }
                }
                i++;
            }  //end for c

            ++dataset;
            for ( ; dataset<mFileOrDataCount; dataset++) {
                if (sliceNo < getDataset(dataset)->m_zSize)    break;
            }
            if (dataset>=mFileOrDataCount) {
                sliceNo += m_cols;
                for (dataset=0; dataset<mFileOrDataCount; dataset++) {
                    if (sliceNo < getDataset(dataset)->m_zSize)    break;
                }
            }
        }  //end for r
        paintMagnify( dc );
#endif
    } else if (m_backgroundLoaded) {
        int  w, h;
        dc->GetSize( &w, &h );
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}
//----------------------------------------------------------------------
/** \brief this function draws measure points (in the measure vector).
 *  \param dc is the device context
 *  \param sx is the screen x
 *  \param sy is the screen y
 *  \param dataset is the dataset number
 *  \param slice is the slice number
 */
void CycleCanvas::paintMeasure ( wxDC* dc, int sx, int sy, int dataset, int slice )
{
    if (mMeasureVector.empty())    return;
    for (int i=0; i<(int)mMeasureVector.size(); i++) {
        if (mMeasureVector[i].mDataset != dataset)  continue;
        const CavassData* const  cd = getDataset( dataset );
        if (!cd->m_overlay)        continue;
        double  scaleX = cd->m_scale * cd->m_xSpacing;
        double  scaleY = cd->m_scale * cd->m_ySpacing;
        const Measure  m = mMeasureVector[i];
        if (m.mFirstSpecified && m.mPZ1 == slice) {
            int  x = (int)(m.mPX1 * scaleX + 0.5);
            int  y = (int)(m.mPY1 * scaleY + 0.5);
            const wxString  s = wxString::Format( "x %d", (m.mID+1)/2 );
            dc->DrawText( s, sx+x, sy+y );
        }
    }
}
//----------------------------------------------------------------------
/** \brief this function draws magnified slices (in the magnify vector).
 *  \param dc is the device context
 */
void CycleCanvas::paintMagnify ( wxDC* dc ) {
    if (mMagnifyVector.empty())    return;
    const int  penWidth = 2;
    int  widthX, heightX;
    dc->GetTextExtent( "x", &widthX, &heightX );

    for (int i=0; i<(int)mMagnifyVector.size(); i++) {
        if (i==(int)(mMagnifyVector.size()-1)) {  //last one?
            if (mMagnifyState == MagnifyStateChoose) {
                wxPen  pen( *wxGREEN, penWidth );
                dc->SetPen( pen );
            } else if (mMagnifyState == MagnifyStatePosition) {
                wxPen  pen( *wxCYAN, penWidth );
                dc->SetPen( pen );
            } else {
                wxPen  pen( *wxBLUE, penWidth );
                dc->SetPen( pen );
            }
        } else {
            wxPen  pen( *wxGREEN, penWidth );
            dc->SetPen( pen );
        }

        Magnify*  mag = &mMagnifyVector[i];
        int  w = mag->mBitmap->GetWidth();
        int  h = mag->mBitmap->GetHeight();
        int  sx = mag->mSX - w/2;
        int  sy = mag->mSY - h/2;
        dc->DrawRectangle( sx-penWidth, sy-penWidth, w+2*penWidth, h+2*penWidth );
        dc->DrawBitmap( *(mag->mBitmap), sx, sy );
        CavassData*  cd = getDataset( mag->mDataset );
        if (cd->m_overlay) {
            wxString  s = wxString::Format( "%d", mag->mSlice+1 );
            if (mFileOrDataCount>=1)
                s = wxString::Format( "%d:%d", mag->mDataset+1, mag->mSlice+1 );
            dc->DrawText( s, sx, sy );

            //draw the measure vector (if any) for this slice
            for (int i=0; i<(int)mag->mMeasureVector.size(); i++) {
                CavassData*  cd = getDataset( mag->mMeasureVector[i].mDataset );
                double  scaleX = mag->mScale * cd->m_xSpacing;
                double  scaleY = mag->mScale * cd->m_ySpacing;

                const Measure  m = mag->mMeasureVector[i];
                if (m.mFirstSpecified && m.mPZ1 == mag->mSlice) {
                    int  x = (int)(m.mPX1 * scaleX + 0.5);
                    int  y = (int)(m.mPY1 * scaleY + 0.5);
                    const wxString  s = wxString::Format( "x %d", m.mID );
                    dc->DrawText( s, sx+x, sy+y );
                }
                if (m.mSecondSpecified && m.mPZ2 == mag->mSlice) {
                    int  x = (int)(m.mPX2 * scaleX + 0.5);
                    int  y = (int)(m.mPY2 * scaleY + 0.5);
                    const wxString  s = wxString::Format( "x %d", m.mID+1 );
                    dc->DrawText( s, sx+x, sy+y );
                }
            }
        }
    }
}
//----------------------------------------------------------------------
void CycleCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes

    //only call setScale when absolutely necessary (i.e., when the # of
    // rows and/or cols changes) to avoid reallocating tons of stuff.
    int  w, h;
    GetSize( &w, &h );
    int  rows=0, cols=0;
    if (mCavassData!=NULL && mMaxPixelWidth && mMaxPixelHeight) {
         cols  = (int)(w / mMaxPixelWidth );
         rows  = (int)(h / mMaxPixelHeight);
    }
    if (cols<1)  cols=1;
    if (rows<1)  rows=1;
    if (cols!=m_cols || rows!=m_rows) {
        if (mCavassData!=NULL) {
            /** \todo probably needs work */
            setScale( mCavassData->m_scale );
        } else {
            /** \todo probably needs work */
            setScale( 1 );
        }
    } else {
        Refresh();
    }
}
//----------------------------------------------------------------------
/** \brief initialize the specified lookup table.
 *  \param which specifies the particular data set (if more than 1 read).
 */
void CycleCanvas::initLUT ( int which ) {
    CycleData*  tmp = dynamic_cast< CycleData*>( getDataset(which) );
    assert( tmp!=NULL );
    tmp->initLUT();
}
//----------------------------------------------------------------------
bool CycleCanvas::isLoaded ( int which ) {
    int  i=0;
    for (CavassData* tmp=mCavassData; tmp!=NULL; tmp=tmp->mNext) {
        if (i++==which)
            return (tmp->mIsCavassFile&&tmp->m_vh_initialized) ||
                tmp->mIsDicomFile || tmp->mIsImageFile;
    }
    return false;
}
//----------------------------------------------------------------------
/** \brief    get the current center contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the current center contrast setting value.
 */
int CycleCanvas::getCenter ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_center;
}
//----------------------------------------------------------------------
/** \brief    get the current setting of the contrast inversion state.
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  true if invert is on; false otherwise.
 */
bool CycleCanvas::getInvert ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->mInvert;
}
//----------------------------------------------------------------------
/** \brief    get the maximum value in a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the maximum value.
 */
int CycleCanvas::getMax ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_max;
}
//----------------------------------------------------------------------
int CycleCanvas::getNoSlices ( void ) {
    return mMaxSlices;
}
//----------------------------------------------------------------------
//bool   CycleCanvas::getOverlay  ( void ) const {  return mCavassData->m_overlay; }
double CycleCanvas::getScale ( void ) {
    CavassData*  tmp = getDataset( 0 );
    assert( tmp!=NULL );
    return tmp->m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int CycleCanvas::getSliceNo ( void ) {
    return m_sliceNo;
}
//----------------------------------------------------------------------
/** \brief    get the current width contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  the current width contrast setting value.
 */
int CycleCanvas::getWidth ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_width;
}
//----------------------------------------------------------------------
void CycleCanvas::setInterpolate ( bool interpolate ) {
    mInterpolate = interpolate;
}
//----------------------------------------------------------------------
//void CycleCanvas::setCenter  ( const int center   )  {  m_center  = center;  }
//void CycleCanvas::setOverlay ( const bool overlay )  {  mCavassData->m_overlay = overlay; }
void CycleCanvas::setSliceNo ( int sliceNo ) {
    for ( CycleData* tmp=dynamic_cast<CycleData*>( mCavassData ); tmp!=NULL;
          tmp=dynamic_cast<CycleData*>( tmp->mNext ))
    {
        if (!tmp->mLocked)    tmp->m_sliceNo = sliceNo;
    }
    m_sliceNo = sliceNo;
}
//----------------------------------------------------------------------
/** \brief  set the current center contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  center is the contrast setting value.
 */
void CycleCanvas::setCenter ( int which, int center ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->m_center = center;
}
//----------------------------------------------------------------------
void CycleCanvas::setFirstDisplayedDataset ( int which ) {
    mFirstDisplayedDataset = which;
}
//----------------------------------------------------------------------
/** \brief    set the current setting of the contrast inversion state.
 *  \param    which specifies the particular data set (if more than one).
 *  \param    invert is true to turn invert on; false otherwise.
 */
void CycleCanvas::setInvert ( int which, bool invert ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->mInvert = invert;
}
//----------------------------------------------------------------------
/** \brief  set the current width contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  width is the contrast setting value.
 */
void CycleCanvas::setWidth ( int which, int width ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->m_width = width;
}
//----------------------------------------------------------------------
void CycleCanvas::setScale ( double scale ) {  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    int  w, h;
    GetSize( &w, &h );
    m_cols = m_rows = 0;
    mMaxPixelWidth = mMaxPixelHeight = 0;

    for (CavassData* tmp=mCavassData; tmp!=NULL; tmp=tmp->mNext) {
        tmp->m_scale = scale;
        const int  scaledW = (int)ceil( tmp->m_xSize * tmp->m_xSpacing * tmp->m_scale );
        const int  scaledH = (int)ceil( tmp->m_ySize * tmp->m_ySpacing * tmp->m_scale );
        if (scaledW > mMaxPixelWidth)     mMaxPixelWidth  = scaledW;
        if (scaledH > mMaxPixelHeight)    mMaxPixelHeight = scaledH;
    }
    if (mMaxPixelWidth!=0)     m_cols = w / mMaxPixelWidth;

    if (m_cols<1)  m_cols = 1;
    else if (m_cols>mFileOrDataCount)    m_cols = mFileOrDataCount;
    m_rows=1;
    reload();
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS( CycleCanvas, wxPanel )
BEGIN_EVENT_TABLE( CycleCanvas, wxPanel )
    EVT_PAINT(            CycleCanvas::OnPaint          )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           CycleCanvas::OnMouseMove      )
    EVT_SIZE(             CycleCanvas::OnSize           )
    EVT_LEFT_DOWN(        CycleCanvas::OnLeftDown       )
    EVT_LEFT_UP(          CycleCanvas::OnLeftUp         )
    EVT_MIDDLE_DOWN(      CycleCanvas::OnMiddleDown     )
    EVT_MIDDLE_UP(        CycleCanvas::OnMiddleUp       )
    EVT_RIGHT_DOWN(       CycleCanvas::OnRightDown      )
    EVT_RIGHT_UP(         CycleCanvas::OnRightUp        )
    EVT_CHAR(             CycleCanvas::OnChar           )
END_EVENT_TABLE()
//======================================================================
