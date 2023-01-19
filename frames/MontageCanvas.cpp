/*
  Copyright 1993-2011, 2015-2016 Medical Image Processing Group
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
 * \file   MontageCanvas.cpp
 * \brief  MontageCanvas class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ChunkData.h"
#include  "MontageCanvas.h"
#include  "TextControls.h"

using namespace std;
//----------------------------------------------------------------------
const int  MontageCanvas::sSpacing=1;  //space, in pixels, between each slice
int  MontageCanvas::Measure::sCount = 1;
//----------------------------------------------------------------------
MontageCanvas::MontageCanvas ( void ) : MainCanvas() {
    init();
}
//----------------------------------------------------------------------
MontageCanvas::MontageCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    init();
}
//----------------------------------------------------------------------
/** \brief initialize members. */
void MontageCanvas::init ( void ) {
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
}
//----------------------------------------------------------------------
MontageCanvas::~MontageCanvas ( void ) {
    cout << "MontageCanvas::~MontageCanvas" << endl;
    wxLogMessage( "MontageCanvas::~MontageCanvas" );
    while (mCavassData!=NULL) {
        CavassData*  next = mCavassData->mNext;
        delete mCavassData;
        mCavassData = next;
    }
    release();
}
//----------------------------------------------------------------------
void MontageCanvas::release ( void ) {
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
void MontageCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    /** \todo support more than one image at a time */
    release();
	ChunkData *cd=new ChunkData( name, xSize, ySize, zSize,
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
    m_rows = (int)(h / (mCavassData->m_ySize * mCavassData->m_scale));
    if (m_cols<1)    m_cols=1;
    if (m_rows<1)    m_rows=1;
    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void MontageCanvas::loadFile ( const char* const fn ) {
    if (fn==NULL || strlen(fn)==0)    return;
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();

    ++mFileOrDataCount;
    if (mCavassData==NULL) {  //first one?
        ChunkData *cd=new ChunkData( fn );
        if ((!cd->mIsCavassFile || !cd->m_vh_initialized) && !cd->mIsDicomFile
				&& !cd->mIsImageFile)
		{
			wxMessageBox("Failed to load file.");
			return;
		}
		mCavassData = cd;
		mCavassData->mR = mCavassData->mG = mCavassData->mB = 1.0;
        mMaxSlices = mCavassData->m_zSize;
    } else {
        //add at the end of the list
        CavassData*  last = mCavassData;
        CavassData*  next = mCavassData->mNext;
        while (next!=NULL) {
            last = next;
            next = next->mNext;
        }
        next = last->mNext = new ChunkData( fn );
		if ((!next->mIsCavassFile || !next->m_vh_initialized) &&
				!next->mIsDicomFile && !next->mIsImageFile)
		{
			wxMessageBox("Failed to load file.");
			last->mNext = NULL;
			return;
		}
        next->mR = next->mG = next->mB = 1.0;
        if (next->m_zSize > mMaxSlices)
            mMaxSlices = next->m_zSize;
    }
    
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void MontageCanvas::reload ( void ) {
    if (!isLoaded(0))    return;
    freeImagesAndBitmaps();
    
    int  k;
    m_images = (wxImage**)malloc( m_cols * m_rows * sizeof(wxImage*) );
    for (k=0; k<m_cols*m_rows; k++)    m_images[k]=NULL;
    
    m_bitmaps = (wxBitmap**)malloc( m_cols * m_rows * sizeof(wxBitmap*) );
    for (k=0; k<m_cols*m_rows; k++)    m_bitmaps[k]=NULL;

    assert( 0<=mFirstDisplayedDataset && mFirstDisplayedDataset<mFileOrDataCount );
    //find the data set in the list that corresponds to the first row of displayed data
    CavassData*  data = mCavassData;
    for (int i=0; i<mFileOrDataCount; i++) {
        if (i==mFirstDisplayedDataset)    break;
        data = data->mNext;
    }
    int  sliceNo = m_sliceNo;
    data->m_sliceNo = sliceNo;

    //have we move beyond the slices in this data set?
    if (data->m_sliceNo>=data->m_zSize) {
        for (int count=0; count<mFileOrDataCount; count++) {
            data = data->mNext;
            if (data==NULL) {
                data = mCavassData;
                sliceNo += m_cols;
            }
            if (sliceNo<data->m_zSize) {
                data->m_sliceNo = sliceNo;
                break;
            }
        }
    }

    k = 0;
    for (int r=0; r<m_rows; r++) {
        for (int c=0; c<m_cols; c++) {
            if (data->m_sliceNo>=data->m_zSize) {
                k += m_cols-c;
                break;
            }
            const double  xFactor = data->m_scale * data->m_xSpacing;
            const double  yFactor = data->m_scale * data->m_ySpacing;

            //note: image data is 24-bit rgb
            if (xFactor==1.0 && yFactor==1.0) {
                m_images[k] = new wxImage( data->m_xSize, data->m_ySize, ::toRGB(*data) );
            } else if (!mInterpolate) {
                m_images[k] = new wxImage( data->m_xSize, data->m_ySize, ::toRGB(*data) );
                const int  scaledW = (int)ceil( data->m_xSize * xFactor );
                const int  scaledH = (int)ceil( data->m_ySize * yFactor );
                m_images[k]->Rescale( scaledW, scaledH );
            } else {
                m_images[k] = new wxImage( (int)(data->m_xSize*xFactor),
                    (int)(data->m_ySize*yFactor),
                    ::toRGBInterpolated(*data, xFactor, yFactor) );
            }

            m_bitmaps[k] = new wxBitmap( (const wxImage&) *m_images[k] );
            ++k;
            ++data->m_sliceNo;
        }

        for (int count=0; count<mFileOrDataCount; count++) {
            data = data->mNext;
            if (!data) {
                data = mCavassData;
                sliceNo += m_cols;
            }
            if (sliceNo<data->m_zSize) {
                data->m_sliceNo = sliceNo;
                break;
            }
        }

    }

    //determine the data set that corresponds to the first row of displayed data
    data = mCavassData;
    for (int i=0; i<mFileOrDataCount; i++) {
        if (i==mFirstDisplayedDataset) {
            data->m_sliceNo = m_sliceNo;
            break;
        }
        data = data->mNext;
    }

    Refresh();
}
//----------------------------------------------------------------------
/** \brief map a point in the window back into the 3d data set.
 *  \param wx is the x coord in the window
 *  \param wy is the y coord in the window
 *  \param d is the number corresponding to the dataset
 *  \param x is the corresponding x coord in the dataset
 *  \param y is the corresponding y coord in the dataset
 *  \param z is the corresponding z coord in the dataset
 *  \returns true if position can be mapped to data; false otherwise
 */
bool MontageCanvas::mapWindowToData ( int wx, int wy,
                                      int& d, int& x, int& y, int& z )
{
    d = x = y = z = -1;
    //this algorithm basically mimics the paint method (which paints the
    // slices) and determine where the position lies
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
}
//----------------------------------------------------------------------
/** \brief    map a point in the window back into an element of the
 *            magnify vector.
 *  \param    wx is the x coord in the window
 *  \param    wy is the y coord in the window
 *  \returns  the index into the magnify vector; -1 otherwise
 */
int  MontageCanvas::mapWindowToMagnifyVector ( int wx, int wy ) {
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
int  MontageCanvas::mapWindowToMagnifyVector ( int wx, int wy,
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
void MontageCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "MontageCanvas::OnChar" << endl;
    wxLogMessage( "MontageCanvas::OnChar" );
    //pass the event up to the parent frame
    //m_parent_frame->ProcessEvent( e );
}
//----------------------------------------------------------------------
/** \brief respond to mouse move events. */
void MontageCanvas::OnMouseMove ( wxMouseEvent& e ) {
    if (mCavassData==NULL || mCavassData->m_data==NULL)    return;

    wxClientDC  dc(this);
    PrepareDC( dc );
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  lx = dc.DeviceToLogicalX( pos.x );
    const long  ly = dc.DeviceToLogicalY( pos.y );
    const long  wx = lx - m_tx;
    const long  wy = ly - m_ty;

    //if we are in move mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    MontageFrame*  mf = dynamic_cast<MontageFrame*>( m_parent_frame );
    if (mf->mWhichMode==MontageFrame::ModeMove && e.LeftIsDown()) {
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
    } else if (mf->mWhichMode==MontageFrame::ModeMagnify) {
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
                CavassData*  cd = getDataset( d );
                double  px = x * cd->m_xSpacing, py = y * cd->m_ySpacing, pz = z * cd->m_zSpacing;
                if (cd->m_size==8) {
                    double  value = 0;
                    cd->getData( value, x, y, z );
                    if (!cd->m_vh_initialized) {
                        s.Printf( wxT("%d: (%d,%d,%d)=%.2f -> %d @ (%.2f,%.2f,%.2f)"),
                            d+1, x+1, y+1, z+1,
                            value, cd->m_lut[ (int)(value - cd->m_min) ],
                            px, py, pz );
                    } else {
                        s.Printf( wxT("%d: (%d,%d,%d)=%.2f -> %d @ (%.2f,%.2f,%.2f+%.2f)"),
                            d+1, x+1, y+1, z+1,
                            value, cd->m_lut[ (int)(value - cd->m_min) ],
                            px, py, pz, cd->m_vh.scn.loc_of_subscenes[0] );
                    }
                } else {
                    int  value = cd->getData( x, y, z );
                    if (!cd->m_vh_initialized || cd->m_vh.gen.data_type!=IMAGE0) {
                        s.Printf( wxT("%d: (%d,%d,%d)=%d -> %d @ (%.2f,%.2f,%.2f)"),
                            d+1, x+1, y+1, z+1,
                            value, cd->m_lut[ value - cd->m_min ],
                            px, py, pz );
                    } else {
                        s.Printf( wxT("%d: (%d,%d,%d)=%d -> %d @ (%.2f,%.2f,%.2f+%.2f)"),
                            d+1, x+1, y+1, z+1,
                            value, cd->m_lut[ value - cd->m_min ],
                            px, py, pz, cd->m_vh.scn.loc_of_subscenes[0] );
                    }
                }
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
}
//----------------------------------------------------------------------
/** \brief handle mouse move events when in magnify mode. */
void MontageCanvas::mouseMoveMagnify ( wxMouseEvent& e ) {
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
void MontageCanvas::moveToNextSlice ( void ) {
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
void MontageCanvas::moveToNextRow ( void ) {
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
void MontageCanvas::moveToPrevSlice ( void ) {
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
void MontageCanvas::moveToPrevRow ( void ) {
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
void MontageCanvas::OnReset ( void ) {
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
    MontageCanvas::Measure::sCount = 1;
}
//----------------------------------------------------------------------
void MontageCanvas::OnLeftDown ( wxMouseEvent& e ) {
    OnMouseMove( e );
    MontageFrame*  mf = dynamic_cast<MontageFrame*>( m_parent_frame );
    switch (mf->mWhichMode) {
        case MontageFrame::ModeMagnify :
            mf->SetStatusText( "magnify...", 0 );
            SetCursor( wxCursor(wxCURSOR_MAGNIFIER) );
            handleMagnifyMode( e );
            break;
        case MontageFrame::ModeMeasure :
            mf->SetStatusText( "measure...", 0 );
            SetCursor( wxCursor(wxCURSOR_CROSS) );
            handleMeasureMode( e );
            break;
        case MontageFrame::ModeMove :
            mf->SetStatusText( "move...", 0 );
            SetCursor( wxCursor(wxCURSOR_HAND) );
            break;
        case MontageFrame::ModePage :
            //page mode / display the next screenful
            mf->SetStatusText( "page...", 0 );
            for (int row=0; row<m_rows; row++) {
                moveToNextRow();
            }
            break;
        case MontageFrame::ModeScroll :
            mf->SetStatusText( "scroll...", 0 );
            moveToNextRow();  //display the next row
            break;
        case MontageFrame::ModeSlice :
            mf->SetStatusText( "slice...", 0 );
            moveToNextSlice();  //display next slice
            break;
        default :
            assert( 0 );
            break;
    }

    mf->setSliceNo( m_sliceNo );
    reload();
    mf->SetStatusText( "ready", 0 );
}
//----------------------------------------------------------------------
void MontageCanvas::handleMagnifyMode ( wxMouseEvent& e ) {
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
void MontageCanvas::handleMeasureMode ( wxMouseEvent& e ) {
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
    CavassData*  cd = getDataset( d );
    int  value = cd->getData( x, y, z );
    MontageFrame*  mf = dynamic_cast<MontageFrame*>( m_parent_frame );

    //is this the first or second point in a pair of points?
    if (mv->empty() || (*mv)[mv->size()-1].mSecondSpecified) {
        //specify the first point for a new entry
        Measure  m( d,
            x*cd->m_xSpacing, y*cd->m_ySpacing, z*cd->m_zSpacing,
            x, y, z, value );
        mv->push_back( m );
        wxString  s = wxString::Format( "%d: (%.2f,%.2f,%.2f) = %d", m.mID, //2*mMeasureVector.size()-1,
            m.mX1, m.mY1, m.mZ1, m.mValue1 );
        mf->mTextControls->insert( s );
    } else {
        //specify the second point for an existing entry
        const int  i = mv->size()-1;
        //are the points in the same dataset?
        int  oldD = (*mv)[i].mDataset;
        if (d != oldD)    return;  //ignore if not in same dataset
        (*mv)[i].setSecondPoint(
            x*cd->m_xSpacing, y*cd->m_ySpacing, z*cd->m_zSpacing,
            x, y, z, value );
        wxString  s = wxString::Format( "%d: (%.2f,%.2f,%.2f) = %d", (*mv)[i].mID+1, //2*mMeasureVector.size(),
            (*mv)[i].mX2, (*mv)[i].mY2, (*mv)[i].mZ2,
            (*mv)[i].mValue2 );
        mf->mTextControls->insert( s );
//		if (mMeasureVector[i].mPZ1 == mMeasureVector[i].mPZ2) {
            s = wxString::Format( "    d=%.2f, t=%.2f rad, t=%.2f deg",
                (*mv)[i].getDistance(),
                (*mv)[i].getAngle(),
                (*mv)[i].getAngle()/M_PI*180.0 );
//		} else {
//			s = wxString::Format( "    d=%.2f", mMeasureVector[i].getDistance() );
//		}
        mf->mTextControls->insert( s );
    }
    Refresh();
}
//----------------------------------------------------------------------
void MontageCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage( "OnLeftUp" );
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void MontageCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    wxLogMessage( "MontageCanvas::OnMiddleDown" );
    OnMouseMove( e );
    MontageFrame*  mf = dynamic_cast<MontageFrame*>( m_parent_frame );
    switch (mf->mWhichMode) {
        case MontageFrame::ModeMagnify :
            mMagnifyState = MagnifyStateChoose;
            break;
        case MontageFrame::ModeMeasure :
            break;
        case MontageFrame::ModeMove :
            break;
        case MontageFrame::ModePage :
            //page mode / display the next screenful
            for (int row=0; row<m_rows; row++) {
                moveToPrevRow();
            }
            break;
        case MontageFrame::ModeScroll :
            moveToPrevRow();  //display the next row
            break;
        case MontageFrame::ModeSlice :
            moveToPrevSlice();
            break;
        default :
            assert( 0 );
            break;
    }

    mf->setSliceNo( m_sliceNo );
    reload();
}
//----------------------------------------------------------------------
void MontageCanvas::OnMiddleUp ( wxMouseEvent& unused ) {
    cout << "OnMiddleUp" << endl;
    wxLogMessage( "OnMiddleUp" );
}
//----------------------------------------------------------------------
void MontageCanvas::OnRightDown ( wxMouseEvent& e ) {
    MontageFrame*  mf = dynamic_cast<MontageFrame*>( m_parent_frame );
    switch (mf->mWhichMode) {
        case MontageFrame::ModeMagnify :
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
}
//----------------------------------------------------------------------
void MontageCanvas::OnRightUp ( wxMouseEvent& unused ) {
    cout << "OnRightUp" << endl;
    wxLogMessage( "OnRightUp" );
}
//----------------------------------------------------------------------
void MontageCanvas::OnPaint ( wxPaintEvent& e ) {
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
void MontageCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
    
    if (m_bitmaps!=NULL) {
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
void MontageCanvas::paintMeasure ( wxDC* dc, int sx, int sy, int dataset, int slice )
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
            const wxString  s = wxString::Format( "x %d", m.mID );  //2*i+1 );
            dc->DrawText( s, sx+x, sy+y );
        }
        if (m.mSecondSpecified && m.mPZ2 == slice) {
            int  x = (int)(m.mPX2 * scaleX + 0.5);
            int  y = (int)(m.mPY2 * scaleY + 0.5);
            const wxString  s = wxString::Format( "x %d", m.mID+1 );  //2*i+2 );
            dc->DrawText( s, sx+x, sy+y );
        }
    }
}
//----------------------------------------------------------------------
/** \brief this function draws magnified slices (in the magnify vector).
 *  \param dc is the device context
 */
void MontageCanvas::paintMagnify ( wxDC* dc ) {
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
void MontageCanvas::OnSize ( wxSizeEvent& e ) {
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
void MontageCanvas::initLUT ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->initLUT();
}
//----------------------------------------------------------------------
bool MontageCanvas::isLoaded ( int which ) {
    int  i=0;
    for (CavassData* tmp=mCavassData; tmp!=NULL; tmp=tmp->mNext) {
        if (i++==which)
		    return (tmp->mIsCavassFile && tmp->m_vh_initialized) ||
				tmp->mIsDicomFile || tmp->mIsImageFile;
    }
    return false;
}
//----------------------------------------------------------------------
/** \brief    get the current center contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the current center contrast setting value.
 */
int MontageCanvas::getCenter ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_center;
}
//----------------------------------------------------------------------
/** \brief    get the current setting of the contrast inversion state.
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  true if invert is on; false otherwise.
 */
bool MontageCanvas::getInvert ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->mInvert;
}
//----------------------------------------------------------------------
/** \brief    get the minimum value in a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the minimum value.
 */
int MontageCanvas::getMin ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_min;
}
//----------------------------------------------------------------------
/** \brief    get the maximum value in a particular data set.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the maximum value.
 */
int MontageCanvas::getMax ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_max;
}
//----------------------------------------------------------------------
int MontageCanvas::getNoSlices ( void ) {
    return mMaxSlices;
}
//----------------------------------------------------------------------
//bool   MontageCanvas::getOverlay  ( void ) const {  return mCavassData->m_overlay; }
double MontageCanvas::getScale ( void ) {
    CavassData*  tmp = getDataset( 0 );
    assert( tmp!=NULL );
    return tmp->m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int MontageCanvas::getSliceNo ( void ) {
    return m_sliceNo;
}
//----------------------------------------------------------------------
/** \brief    get the current width contrast setting for a particular data set.
 *  \param    which specifies the particular data set (if more than one).
 *  \returns  the current width contrast setting value.
 */
int MontageCanvas::getWidth ( int which ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    return tmp->m_width;
}
//----------------------------------------------------------------------
void MontageCanvas::setInterpolate ( bool interpolate ) {
    mInterpolate = interpolate;
}
//----------------------------------------------------------------------
//void MontageCanvas::setCenter  ( const int center   )  {  m_center  = center;  }
//void MontageCanvas::setOverlay ( const bool overlay )  {  mCavassData->m_overlay = overlay; }
void MontageCanvas::setSliceNo ( int sliceNo ) {
    for (CavassData* tmp=mCavassData; tmp!=NULL; tmp=tmp->mNext) {
        tmp->m_sliceNo = sliceNo;
    }
    m_sliceNo = sliceNo;
}
//----------------------------------------------------------------------
/** \brief  set the current center contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  center is the contrast setting value.
 */
void MontageCanvas::setCenter ( int which, int center ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->m_center = center;
}
//----------------------------------------------------------------------
void MontageCanvas::setFirstDisplayedDataset ( int which ) {
    mFirstDisplayedDataset = which;
}
//----------------------------------------------------------------------
/** \brief    set the current setting of the contrast inversion state.
 *  \param    which specifies the particular data set (if more than one).
 *  \param    invert is true to turn invert on; false otherwise.
 */
void MontageCanvas::setInvert ( int which, bool invert ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->mInvert = invert;
}
//----------------------------------------------------------------------
/** \brief  set the current width contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  width is the contrast setting value.
 */
void MontageCanvas::setWidth ( int which, int width ) {
    CavassData*  tmp = getDataset( which );
    assert( tmp!=NULL );
    tmp->m_width = width;
}
//----------------------------------------------------------------------
void MontageCanvas::setScale ( double scale ) {  //aka magnification
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
    if (mMaxPixelHeight!=0)    m_rows = h / mMaxPixelHeight;

    if (m_cols<1)  m_cols=1;
    if (m_rows<1)  m_rows=1;
    reload();
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( MontageCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( MontageCanvas, wxPanel )
    EVT_PAINT(            MontageCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           MontageCanvas::OnMouseMove    )
    EVT_SIZE(             MontageCanvas::OnSize         )
    EVT_LEFT_DOWN(        MontageCanvas::OnLeftDown     )
    EVT_LEFT_UP(          MontageCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      MontageCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        MontageCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       MontageCanvas::OnRightDown    )
    EVT_RIGHT_UP(         MontageCanvas::OnRightUp      )
    EVT_CHAR(             MontageCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
