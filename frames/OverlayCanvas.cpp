/*
  Copyright 1993-2017, 2022-2023 Medical Image Processing Group
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
 * \file:  OverlayCanvas.cpp
 * \brief  OverlayCanvas class implementation
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
const int  OverlayCanvas::sSpacing=1;  ///< space, in pixels, between each slice (on the screen)
//----------------------------------------------------------------------
OverlayCanvas::OverlayCanvas ( void )  {  puts("OverlayCanvas()");  }
//----------------------------------------------------------------------
OverlayCanvas::OverlayCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    mFileOrDataCount = 0;
    m_scale          = Preferences::getOverlayScale();
	if (m_scale == 0)
		m_scale      = 1.5;
    m_overlay        = true;
    m_rows           = 0;
    m_cols           = 0;
    m_images         = (wxImage**)NULL;
    m_bitmaps        = (wxBitmap**)NULL;
    m_tx = m_ty     = 0;
	previously_visited = new wxArrayString;

    mOverallXSize = mOverallYSize = mOverallZSize = 0;

    lastX = lastY = -1;
}
//----------------------------------------------------------------------
OverlayCanvas::~OverlayCanvas ( void ) {
    cout << "OverlayCanvas::~OverlayCanvas" << endl;
    wxLogMessage( "OverlayCanvas::~OverlayCanvas" );
    release();
	delete previously_visited;
	previously_visited = NULL;
}
//----------------------------------------------------------------------
void OverlayCanvas::release ( void ) {
}
//----------------------------------------------------------------------
void OverlayCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    assert( mFileOrDataCount==0 || mFileOrDataCount==1 );
    CavassData*  cd = new CavassData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    if (mFileOrDataCount==0) {
        assert( mCavassData==NULL );
        mCavassData = cd;
    } else {
        assert( mCavassData!=NULL && mCavassData->mNext==NULL );
        mCavassData->mNext = cd;
    }
    ++mFileOrDataCount;

    if (mFileOrDataCount==2) {
        CavassData&  A = *mCavassData;
        CavassData&  B = *(mCavassData->mNext);

        if (A.m_max <= 1)
        {
            if (B.m_max <= 1)
            {
				B.mR = .01*Preferences::getBIMonBIMRed();
				B.mG = .01*Preferences::getBIMonBIMGreen();
				B.mB = .01*Preferences::getBIMonBIMBlue();
				A.mR = 1.0 - B.mR;
				A.mG = 1.0 - B.mG;
				A.mB = 1.0 - B.mB;
            }
            else
            {
                A.mR = .01*Preferences::getBIMonIM0Red();
				A.mG = .01*Preferences::getBIMonIM0Green();
				A.mB = .01*Preferences::getBIMonIM0Blue();
                B.mR = 1.0;  B.mG = 1.0;  B.mB = 1.0;
            }
        }
        else
        {
            if (B.m_max <= 1)
            {
                A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
                B.mR = .01*Preferences::getBIMonIM0Red();
				B.mG = .01*Preferences::getBIMonIM0Green();
				B.mB = .01*Preferences::getBIMonIM0Blue();
            }
            else
            {
                A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
                B.mR = .01*Preferences::getIM0onIM0Red();
				B.mG = .01*Preferences::getIM0onIM0Green();
				B.mB = .01*Preferences::getIM0onIM0Blue();
            }
        }

        //determine the number of pixels in the overlay image
        mOverallXSize = (int)
            ( max( A.m_xSize*A.m_xSpacing, B.m_xSize*B.m_xSpacing )
            / min( A.m_xSpacing, B.m_xSpacing ) + 0.5);
        mOverallYSize = (int)
            ( max( A.m_ySize*A.m_ySpacing, B.m_ySize*B.m_ySpacing )
            / min( A.m_ySpacing, B.m_ySpacing ) + 0.5);
        mOverallZSize = A.m_zSize;
        if (B.m_zSize>mOverallZSize)    mOverallZSize = B.m_zSize;

        int  w, h;
        GetSize( &w, &h );
        m_cols = (int)(w / (mOverallXSize * m_scale));
        m_rows = (int)(h / (mOverallYSize * m_scale));
        if (m_cols!=1)  m_cols=1;
        if (m_rows!=1)  m_rows=1;
		m_tx = (w - m_scale*mOverallXSize)/2;
		m_ty = (h - m_scale*mOverallYSize)/2;
		if (Preferences::getCTSoftTissueCenter() < mCavassData->m_max)
		{
			mCavassData->m_center = Preferences::getCTSoftTissueCenter();
			mCavassData->m_width = Preferences::getCTSoftTissueWidth();
			mCavassData->initLUT();
		}
        reload();
    }
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void OverlayCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount>=0 && mFileOrDataCount<=1 );
    ChunkData*  cd = new ChunkData( fn );
	if ((!cd->mIsCavassFile || !cd->m_vh_initialized) && !cd->mIsDicomFile &&
			!cd->mIsImageFile)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
    if (mFileOrDataCount==0) {
        assert( mCavassData==NULL );
        mCavassData = cd;
    } else {
        assert( mCavassData!=NULL && mCavassData->mNext==NULL );
        mCavassData->mNext = cd;
    }
    ++mFileOrDataCount;

    if (isLoaded(0) && isLoaded(1)) {
        CavassData&  A = *mCavassData;  //mData[0];
        CavassData&  B = *(mCavassData->mNext);  //mData[1];

        if (A.m_max <= 1)
        {
            if (B.m_max <= 1)
            {
				B.mR = .01*Preferences::getBIMonBIMRed();
				B.mG = .01*Preferences::getBIMonBIMGreen();
				B.mB = .01*Preferences::getBIMonBIMBlue();
				A.mR = 1.0 - B.mR;
				A.mG = 1.0 - B.mG;
				A.mB = 1.0 - B.mB;
            }
            else
            {
                A.mR = .01*Preferences::getBIMonIM0Red();
				A.mG = .01*Preferences::getBIMonIM0Green();
				A.mB = .01*Preferences::getBIMonIM0Blue();
                B.mR = 1.0;  B.mG = 1.0;  B.mB = 1.0;
            }
        }
        else
        {
            if (B.m_max <= 1)
            {
                A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
                B.mR = .01*Preferences::getBIMonIM0Red();
				B.mG = .01*Preferences::getBIMonIM0Green();
				B.mB = .01*Preferences::getBIMonIM0Blue();
            }
            else
            {
                A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
                B.mR = .01*Preferences::getIM0onIM0Red();
				B.mG = .01*Preferences::getIM0onIM0Green();
				B.mB = .01*Preferences::getIM0onIM0Blue();
            }
        }

        //determine the number of pixels in the overlay image
        mOverallXSize = (int)max( A.m_xSize, B.m_xSize );
        mOverallYSize = (int)max( A.m_ySize, B.m_ySize );
        mOverallZSize = A.m_zSize;
        if (B.m_zSize>mOverallZSize)    mOverallZSize = B.m_zSize;

        int  w, h;
        GetSize( &w, &h );
        m_cols = (int)(w / (mOverallXSize * m_scale));
        m_rows = (int)(h / (mOverallYSize * m_scale));
        if (m_cols!=1)  m_cols=1;
        if (m_rows!=1)  m_rows=1;
		m_tx = (w - m_scale*mOverallXSize)/2;
		m_ty = (h - m_scale*mOverallYSize)/2;
		if (Preferences::getCTSoftTissueCenter() < mCavassData->m_max)
		{
			mCavassData->m_center = Preferences::getCTSoftTissueCenter();
			mCavassData->m_width = Preferences::getCTSoftTissueWidth();
			mCavassData->initLUT();
		}
        reload();
    }
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
bool OverlayCanvas::loadNext ( void ) {
	if (mCavassData==NULL || mCavassData->mNext==NULL)
		return false;
	wxFileName cur2=
		wxFileName(wxString(mCavassData->mNext->m_fname), wxPATH_NATIVE);
	wxString path=cur2.GetPath();
	if (path == "")
		path = ".";
	wxDir directory;
	directory.Open(path);
	if (!directory.IsOpened())
		return false;
	char star[2]="*";
	char *last_filename = star;
	bool got_first=false, got_cur=false;
	int nfiles=0;
	wxString dir_entry;
	wxFileName test2;
	while (true)
	{
		if (!(got_first? directory.GetNext(&dir_entry):
				directory.GetFirst(&dir_entry, wxEmptyString, wxDIR_FILES)))
		{
			if (!got_cur)
				return false;
			directory.GetFirst(&dir_entry, wxEmptyString, wxDIR_FILES);
		}
		got_first = true;
		test2 = wxFileName(path, dir_entry);
		if (test2 == cur2)
		{
			got_cur = true;
			continue;
		}
		if (!got_cur || test2.GetExt()!="BIM")
			continue;
		test2.SetExt("IM0");
		if (test2.FileExists())
			break;
	}
	ChunkData *cd = new ChunkData((const char *)test2.GetFullPath().c_str());
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return false;
	}
	test2.SetExt("BIM");
	cd->mNext = new ChunkData((const char *)test2.GetFullPath().c_str());
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return false;
	}
	if (previously_visited->Count() == 0)
		previously_visited->Add(cur2.GetFullPath());
	bool already_visited=false;
	for (unsigned int j=0; j<previously_visited->Count(); j++)
		if (wxFileName((*previously_visited)[j]) == test2)
		{
			already_visited = true;
			break;
		}

    CavassData&  A = *cd;  //mData[0];
    CavassData&  B = *(cd->mNext);  //mData[1];

	A.mR = mCavassData->mR;
	A.mG = mCavassData->mG;
	A.mB = mCavassData->mB;
	B.mR = mCavassData->mNext->mR;
	B.mG = mCavassData->mNext->mG;
	B.mB = mCavassData->mNext->mB;
    //determine the number of pixels in the overlay image
    mOverallXSize = (int)max( A.m_xSize, B.m_xSize );
    mOverallYSize = (int)max( A.m_ySize, B.m_ySize );
    mOverallZSize = A.m_zSize;
    if (B.m_zSize>mOverallZSize)    mOverallZSize = B.m_zSize;

    int  w, h;
    GetSize( &w, &h );
    m_cols = (int)(w / (mOverallXSize * m_scale));
    m_rows = (int)(h / (mOverallYSize * m_scale));
    if (m_cols!=1)  m_cols=1;
    if (m_rows!=1)  m_rows=1;
	if (Preferences::getCTSoftTissueCenter() < cd->m_max)
	{
		cd->m_center = Preferences::getCTSoftTissueCenter();
		cd->m_width = Preferences::getCTSoftTissueWidth();
		cd->initLUT();
	}
	if (mCavassData->m_center<=cd->m_max && mCavassData->m_width<=cd->m_max)
	{
		cd->m_center = mCavassData->m_center;
		cd->m_width = mCavassData->m_width;
		cd->initLUT();
    }
	delete mCavassData->mNext;
	mCavassData->mNext = NULL;
	delete mCavassData;
	mCavassData = cd;
	if (B.m_vh.scn.num_of_bits == 1)
	{
		unsigned char *slice=(unsigned char*)malloc((B.m_xSize*B.m_ySize+7)/8);
		FILE *fp=fopen(B.m_fname, "rb");
		if (fp == NULL)
			wxMessageBox("Open failure");
		else
		  for (int j=0; j<A.m_zSize&&j<B.m_zSize; j++)
		  {
			if (VLSeekData(fp, (double)j*((B.m_xSize*B.m_ySize+7)/8)))
			{
				wxMessageBox("Seek failure");
				break;
			}
			int n;
			if (VReadData((char*)slice, 1, (B.m_xSize*B.m_ySize+7)/8, fp,
					&n))
			{
				wxMessageBox("Read failure");
				break;
			}
			B.m_sliceNo = j;
			unsigned char b=0;
			for (int k=0; k<(B.m_xSize*B.m_ySize+7)/8; k++)
			{
				b = slice[k];
				if (b)
					break;
			}
			if (b)
				break;
		  }
		if (fp)
			fclose(fp);
		free(slice);
		A.m_sliceNo = B.m_sliceNo;
	}
	reload();
	OverlayFrame *p = (OverlayFrame *)m_parent_frame;
	p->loadFile("*");
	if (already_visited)
		wxMessageBox("File already visited.");
	else
		previously_visited->Add(test2.GetFullPath());
	return true;
}
//----------------------------------------------------------------------
void OverlayCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else             mCavassData->mNext->initLUT();
}
//----------------------------------------------------------------------
void OverlayCanvas::reload ( void ) {
    if (!isLoaded(0) || !isLoaded(1))    return;
    freeImagesAndBitmaps();
    
    int  k;
    m_images = (wxImage**)malloc( m_cols * m_rows * sizeof(wxImage*) );
    for (k=0; k<m_cols*m_rows; k++)    m_images[k]=NULL;
    
    m_bitmaps = (wxBitmap**)malloc( m_cols * m_rows * sizeof(wxBitmap*) );
    for (k=0; k<m_cols*m_rows; k++)    m_bitmaps[k]=NULL;
    
    const CavassData&  A = *mCavassData;
    const CavassData&  B = *(mCavassData->mNext);

    for (k=0; k<m_cols*m_rows; k++) {
        if (A.m_sliceNo+k >= A.m_zSize && B.m_sliceNo+k >= B.m_zSize)    break;
        //note: image data are 24-bit rgb
        unsigned char*  slice = NULL;
        //copy from the src slices in both A (if any) and B (if any) to the dst slice
        if (A.m_size==1 || A.m_size==3) {
            if (B.m_size==1 || B.m_size==3)
                                     slice = composite( k, (unsigned char*)A.m_data,  (unsigned char*) B.m_data );
            else if (B.m_size==2)    slice = composite( k, (unsigned char*)A.m_data,  (unsigned short*)B.m_data );
            else if (B.m_size==4)    slice = composite( k, (unsigned char*)A.m_data,  (unsigned int*)  B.m_data );
            else                     assert( 0 );
        } else if (A.m_size==2) {
            if (B.m_size==1 || B.m_size==3)
                                     slice = composite( k, (unsigned short*)A.m_data, (unsigned char*) B.m_data );
            else if (B.m_size==2)    slice = composite( k, (unsigned short*)A.m_data, (unsigned short*)B.m_data );
            else if (B.m_size==4)    slice = composite( k, (unsigned short*)A.m_data, (unsigned int*)  B.m_data );
            else                     assert( 0 );
        } else if (A.m_size==4) {
            if (B.m_size==1 || B.m_size==3)
                                     slice = composite( k, (unsigned int*)A.m_data,   (unsigned char*) B.m_data );
            else if (B.m_size==2)    slice = composite( k, (unsigned int*)A.m_data,   (unsigned short*)B.m_data );
            else if (B.m_size==4)    slice = composite( k, (unsigned int*)A.m_data,   (unsigned int*)  B.m_data );
            else                     assert( 0 );
        } else {
            assert( 0 );
        }
        assert( slice!=NULL );

        m_images[k] = new wxImage( mOverallXSize, mOverallYSize, slice );
        if (m_scale!=1.0) {
            m_images[k]->Rescale( (int)(ceil(m_scale*mOverallXSize)),
                                  (int)(ceil(m_scale*mOverallYSize)) );
        }
        m_bitmaps[k] = new wxBitmap( (const wxImage&) *m_images[k] );
    }
    Refresh();
}
//----------------------------------------------------------------------
void OverlayCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void OverlayCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "OverlayCanvas::OnChar" << endl;
    wxLogMessage( "OverlayCanvas::OnChar" );
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
void OverlayCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z, int which ) {
  //map a point in the window back into the 3d data set
    wx -= m_tx;    wy -= m_ty;
	CavassData *cd=which? mCavassData->mNext: mCavassData;
    x = (int)( wx / cd->m_scale );
    y = (int)( wy / cd->m_scale );
    z = getSliceNo(which);
    
    //clamp the values to acceptable values
    if (x<0)  x=0;
    else if (x>=cd->m_xSize)  x=cd->m_xSize-1;
    if (y<0)  y=0;
    else if (y>=cd->m_ySize)  y=cd->m_ySize-1;
    if (z<0)  z=0;
    else if (z>=cd->m_zSize)  z=cd->m_zSize-1;
}
//----------------------------------------------------------------------
void OverlayCanvas::OnMouseMove ( wxMouseEvent& e ) {
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;

	if (mCavassData && mCavassData->mNext)
	{
	    int  x, y, z, x2, y2, z2;
	    mapWindowToData( dc.DeviceToLogicalX( pos.x ),
	                     dc.DeviceToLogicalY( pos.y ), x, y, z, 0 );
	    mapWindowToData( dc.DeviceToLogicalX( pos.x ),
	                     dc.DeviceToLogicalY( pos.y ), x2, y2, z2, 1 );
	    wxString  s;
	    if ((wx>=0 && wy>=0) && (wx < mOverallXSize * m_scale &&
		     wy < mOverallYSize * m_scale))
	    {
	          s.Printf( wxT("(%d,%d,%d)=%d, (%d,%d,%d)=%d"), x, y, z,
			      mCavassData->getData(x,y,z), x2, y2, z2,
				  mCavassData->mNext->getData(x2,y2,z2));
	          m_parent_frame->SetStatusText( s, 1 );
	    }
	}

    //if we are NOT in any mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    if (e.LeftIsDown() || e.RightIsDown()) {
        if (lastX==-1 || lastY==-1) {
            SetCursor( wxCursor(wxCURSOR_HAND) );
            lastX = wx;
            lastY = wy;
            return;
        }
        
        bool  changed=false;
        if (abs(wx-lastX)>2) {
            m_tx += (wx-lastX)/2;  // / 4 * 3;
            changed = true;
        } else if (abs(wx-lastX)>2) {
            m_tx -= (lastX-wx)/2;  // / 4 * 3;
            changed = true;
        }
        if (abs(wy-lastY)>2) {
            m_ty += (wy-lastY)/2;  // / 4 * 3;
            changed = true;
        } else if (abs(wy-lastY)>2) {
            m_ty -= (lastY-wy)/2;  // / 4 * 3;
            changed = true;
        }
        
        if (changed)  Refresh();
        lastX=wx;
        lastY=wy;
        return;
    } else {
        if (lastX!=-1 || lastY!=-1) {
            SetCursor( *wxSTANDARD_CURSOR );
            lastX = lastY = -1;
        }
    }
}
//----------------------------------------------------------------------
void OverlayCanvas::OnRightDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = false;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void OverlayCanvas::OnRightUp ( wxMouseEvent& e ) {
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void OverlayCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(0)) {
        mCavassData->mDisplay = false;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void OverlayCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    if (isLoaded(0)) {
        mCavassData->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void OverlayCanvas::OnLeftDown ( wxMouseEvent& e ) {
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
    lastX = wx;
    lastY = wy;
    SetCursor( wxCursor(wxCURSOR_HAND) );
}
//----------------------------------------------------------------------
void OverlayCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void OverlayCanvas::OnLeftDClick ( wxMouseEvent& e ) {
	if (!loadNext())
	{
		wxLogMessage("loadNext failed");
		cout << "loadNext failed" << endl;
	}
}
//----------------------------------------------------------------------
void OverlayCanvas::OnPaint ( wxPaintEvent& e ) {
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
void OverlayCanvas::paint ( wxDC* dc ) {
    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
    
    if (m_bitmaps!=NULL) {
        int  i=0;
        for (int r=0; r<m_rows; r++) {
            const int y = (int)(r*(ceil(mOverallYSize*m_scale)+sSpacing));
            for (int c=0; c<m_cols; c++) {
                if (m_bitmaps[i]!=NULL && m_bitmaps[i]->Ok()) {
                    //const int x = (int)(c*(m_xSize*m_scale+1)+0.5);
                    const int x = (int)(c*(ceil(mOverallXSize*m_scale)+sSpacing));
                    dc->DrawBitmap( *m_bitmaps[i], x+m_tx, y+m_ty );
                    //show the overlay?  (the overlay consists of numbers that indicate the slice)
                    if (m_overlay) {
                        const int sliceA = mCavassData->m_sliceNo+i;
                        const int sliceB = mCavassData->mNext->m_sliceNo+i;
                        //both in bounds?
                        if (mCavassData->inBounds(0,0,sliceA) && mCavassData->mNext->inBounds(0,0,sliceB)) {
                            //both in bounds
                            const wxString  s = wxString::Format( "(%d/%d)", sliceA+1, sliceB+1 );
                            dc->DrawText( s, x+m_tx, y+m_ty );
                        } else if (mCavassData->inBounds(0,0,sliceA) && !mCavassData->mNext->inBounds(0,0,sliceB)) {
                            //only first in bounds
                            const wxString  s = wxString::Format( "(%d/-)", sliceA+1 );
                            dc->DrawText( s, x+m_tx, y+m_ty );
                        } else if (!mCavassData->inBounds(0,0,sliceA) && mCavassData->mNext->inBounds(0,0,sliceB)) {
                            //only second in bounds
                            const wxString  s = wxString::Format( "(-/%d)", sliceB+1 );
                            dc->DrawText( s, x+m_tx, y+m_ty );
                        } else {
                            //both out of bounds
                            const wxString  s = wxString::Format( "(-/-)" );
                            dc->DrawText( s, x+m_tx, y+m_ty );
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
void OverlayCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool OverlayCanvas::isLoaded ( const int which ) const {
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
int OverlayCanvas::getCenter ( const int which ) const {
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
int OverlayCanvas::getMax ( const int which ) const {
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
int OverlayCanvas::getMin ( const int which ) const {
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
//number of slices in entire data set
int OverlayCanvas::getNoSlices ( const int which ) const {
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
bool OverlayCanvas::getOverlay ( void ) const {
    return m_overlay;
}
//----------------------------------------------------------------------
double OverlayCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int OverlayCanvas::getSliceNo ( const int which ) const {
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
int OverlayCanvas::getWidth ( const int which ) const {
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
bool OverlayCanvas::getInvert ( const int which ) const {
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
double OverlayCanvas::getB ( const int which ) const {
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
double OverlayCanvas::getG ( const int which ) const {
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
double OverlayCanvas::getR ( const int which ) const {
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
void OverlayCanvas::setB ( const int which, const double b ) {
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
void OverlayCanvas::setCenter ( const int which, const int center ) {
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
void OverlayCanvas::setG ( const int which, const double g ) {
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
void OverlayCanvas::setInvert ( const int which, const bool invert ) {
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
void OverlayCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void OverlayCanvas::setR ( const int which, const double r ) {
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
void OverlayCanvas::setScale   ( const double scale )  {  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    m_scale = scale;
    CavassData*  tmp = mCavassData;
        while (tmp!=NULL) {
            tmp->m_scale = m_scale;
                tmp = tmp->mNext;
        }

    int  w, h;
    GetSize( &w, &h );
    m_cols  = (int)(w / (mOverallXSize * m_scale));
    m_rows  = (int)(h / (mOverallYSize * m_scale));
    if (m_cols!=1)  m_cols=1;
    if (m_rows!=1)  m_rows=1;
    reload();
}
//----------------------------------------------------------------------
void OverlayCanvas::setSliceNo ( const int which, const int sliceNo ) {
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
void OverlayCanvas::setWidth ( const int which, const int width ) {
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
/** \brief Allow the user to scroll through the slices with the mouse wheel. */void OverlayCanvas::OnMouseWheel ( wxMouseEvent& e ) {
	OverlayFrame*  of = dynamic_cast<OverlayFrame*>(m_parent_frame);
	of->OnMouseWheel(e);
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( OverlayCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( OverlayCanvas, wxPanel )
    EVT_PAINT(            OverlayCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           OverlayCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        OverlayCanvas::OnLeftDown     )
    EVT_LEFT_UP(          OverlayCanvas::OnLeftUp       )
	EVT_LEFT_DCLICK(      OverlayCanvas::OnLeftDClick   )
    EVT_MIDDLE_DOWN(      OverlayCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        OverlayCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       OverlayCanvas::OnRightDown    )
    EVT_RIGHT_UP(         OverlayCanvas::OnRightUp      )
    EVT_CHAR(             OverlayCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
