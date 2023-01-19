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
 * \file:  VOIStandardlizeCanvas.cpp
 * \brief  VOIStandardlizeCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "VOIStandardlizeCanvas.h"
#include <wx/combo.h>
#include <wx/listctrl.h>

//#include  "VOIStandardlizeAlgorithm.h"
using namespace std;
//----------------------------------------------------------------------
VOIStandardlizeCanvas::VOIStandardlizeCanvas ( void )  {  puts("VOIStandardlizeCanvas()");  }
//----------------------------------------------------------------------
VOIStandardlizeCanvas::VOIStandardlizeCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
    m_scale          = 1.0;
    m_overlay        = true;
	m_bLayout        = false;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 40;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
	m_nLowIle = 0;
	m_nHighIle = (float)99.8;
//	m_VOIStandardlizeType = VOIStandardlize_GRADIENT2D;
	m_bVOIStandardlizeDone = false;	

	m_bMouseMove = false;
	BLINK = false;


}
//----------------------------------------------------------------------
VOIStandardlizeCanvas::~VOIStandardlizeCanvas ( void ) {
    cout << "VOIStandardlizeCanvas::~VOIStandardlizeCanvas" << endl;
    wxLogMessage( "VOIStandardlizeCanvas::~VOIStandardlizeCanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
	
	//if( m_sliceIn != NULL )
	//{
	//	delete m_sliceIn;
	//	m_sliceIn = NULL;
	//}
	//if( m_sliceOut != NULL )
	//{
	//	delete m_sliceOut;
	//	m_sliceOut = NULL;
	//}
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::release ( void ) {
#if 0
    m_scale   = 1.0;
    freeImagesAndBitmaps();
    m_center = m_width = 0;
    if (m_lut!=NULL)    {  free(m_lut);      m_lut   = NULL;  }
    m_sliceNo = 0;
    if (m_fname!=NULL)  {  free(m_fname);    m_fname = NULL;  }
    if (m_data!=NULL)   {  free(m_data);     m_data  = NULL;  }
    m_min = m_max = 0;
    m_size = 0;
    m_bytesPerSlice = 0;
    m_tx = m_ty = 0;
    mInvert = false;
    lastX = lastY = -1;
#endif
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
/*
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

        A.mR = 1.0;  A.mG = 0.5;  A.mB = 0.0;
        B.mR = 0.0;  B.mG = 0.5;  B.mB = 1.0;

        mOverallXSize = A.m_xSize;
        if (A.m_xSize*A.m_xSpacing/B.m_xSpacing > mOverallXSize)
            mOverallXSize = (int)(A.m_xSize*A.m_xSpacing/B.m_xSpacing + 0.5);
        if (B.m_xSize*B.m_xSpacing/A.m_xSpacing > mOverallXSize)
            mOverallXSize = (int)(B.m_xSize*B.m_xSpacing/A.m_xSpacing + 0.5);
        mOverallYSize = A.m_ySize;
        if (A.m_ySize*A.m_ySpacing/B.m_ySpacing > mOverallYSize)
            mOverallYSize = (int)(A.m_ySize*A.m_ySpacing/B.m_ySpacing + 0.5);
        if (B.m_ySize*B.m_ySpacing/A.m_ySpacing > mOverallYSize)
            mOverallYSize = (int)(B.m_ySize*B.m_ySpacing/A.m_ySpacing + 0.5);
        mOverallZSize = A.m_zSize;
        if (B.m_zSize>mOverallZSize)    mOverallZSize = B.m_zSize;

        int  w, h;
        GetSize( &w, &h );
        m_cols = (int)(w / (mOverallXSize * m_scale));
        m_rows = (int)(h / (mOverallYSize * m_scale));
        if (m_cols<1)  m_cols=1;
        if (m_rows<1)  m_rows=1;
        reload();
    }
    SetCursor( *wxSTANDARD_CURSOR );
*/
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::loadFile ( const char* const fn ) 
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

	m_sliceOut->mNext = NULL;	
	mCavassData->mNext = m_sliceOut;

	int  w, h;
	if (isLoaded(0)) 
	{
	    
		mOverallXSize = mCavassData->m_xSize;		
		GetSize( &w, &h );
		m_scale = (double)((w-150)*1.0 / (mOverallXSize * 3));     
		double yScale = (double)(h-100)*1.0/mCavassData->m_ySize;
		if( yScale < m_scale )
			m_scale = yScale;  
		//m_scale = 1.0;	
	}       

	min_hist = mCavassData->m_min;
	if (min_hist == 0)
		min_hist = 1;
	max_hist = mCavassData->m_max;

    reload();    
    SetCursor( *wxSTANDARD_CURSOR );
	
//	InitVOIStandardlize();
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}

//----------------------------------------------------------------------
void VOIStandardlizeCanvas::CreateDisplayImage(int which)
{
	//note: image data is 24-bit rgb
	CavassData&  A = *mCavassData;

	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
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

	//delete [] slice;
}

//----------------------------------------------------------------------
void VOIStandardlizeCanvas::reload ( void ) 
{
  //if (!isLoaded(0) || !isLoaded(1))    return;
    freeImagesAndBitmaps();

	CreateDisplayImage(0);

    Refresh();
}

//----------------------------------------------------------------------
void VOIStandardlizeCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
#if 0
    //map a point in the window back into the 3d data set
    wx -= m_tx;    wy -= m_ty;
    const int  col = (int)((double)wx / (ceil(m_xSize*m_scale)+sSpacing));
    const int  row = (int)((double)wy / (ceil(m_ySize*m_scale)+sSpacing));
    x = (int)( wx % ((int)(ceil(m_xSize*m_scale)+sSpacing)) / m_scale );
    y = (int)( wy % ((int)(ceil(m_ySize*m_scale)+sSpacing)) / m_scale );
    z = m_sliceNo + row*m_cols + col;
    
    //clamp the values to acceptable values
    if (x<0)  x=0;
    else if (x>=m_xSize)  x=m_xSize-1;
    if (y<0)  y=0;
    else if (y>=m_ySize)  y=m_ySize-1;
    if (z<0)  z=0;
    else if (z>=m_zSize)  z=m_zSize-1;
#endif
}

//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void VOIStandardlizeCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "VOIStandardlizeCanvas::OnChar" << endl;
    wxLogMessage( "VOIStandardlizeCanvas::OnChar" );
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
void VOIStandardlizeCanvas::OnMouseMove ( wxMouseEvent& e ) 
{
    
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
	
	Refresh();

}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::OnRightUp ( wxMouseEvent& e ) 
{
    //if (isLoaded(0)) {
    //    mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
    //    reload();
    //}
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
	   SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();                        

    SetStatusText( "Select Interval",   2 );
    SetStatusText( "Select Boundary", 3 );
    SetStatusText( "", 4 );
//	UpdateThresholdTablets();
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
 /*   if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    } */
}


//void VOIStandardlizeCanvas::RunThreshold()
//{	
//}


//----------------------------------------------------------------------
void VOIStandardlizeCanvas::OnLeftDown ( wxMouseEvent& e ) 
{
     SetFocus();  //to regain/recapture keypress events   
	
	Refresh();

}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::OnLeftUp ( wxMouseEvent& e ) 
{
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////

void VOIStandardlizeCanvas::OnPaint ( wxPaintEvent& e ) 
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
void VOIStandardlizeCanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	if(m_bitmaps[0]!=NULL)
	{
		if (m_bitmaps[0]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[0], m_tx, m_ty );
			if(m_overlay)
			{
				const wxString  s = wxString::Format( "%d/%d",getSliceNo(0)+1,getNoSlices(0));
				dc->DrawText( s, m_tx, m_ty );
			}
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
void VOIStandardlizeCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool VOIStandardlizeCanvas::isLoaded ( const int which ) const {
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

int VOIStandardlizeCanvas::SaveProfile ( unsigned char* cFilename )
{
	

	return 1;
}

bool VOIStandardlizeCanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int VOIStandardlizeCanvas::getCenter ( const int which ) const 
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
int VOIStandardlizeCanvas::getMax ( const int which ) const 
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
int VOIStandardlizeCanvas::getMin ( const int which ) const 
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
int VOIStandardlizeCanvas::getNoSlices ( const int which ) const {
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

bool VOIStandardlizeCanvas::getVOIStandardlizeDone (void) const
{
	return m_bVOIStandardlizeDone;
}
//----------------------------------------------------------------------
double VOIStandardlizeCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int VOIStandardlizeCanvas::getSliceNo ( const int which ) const {
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
int VOIStandardlizeCanvas::getWidth ( const int which ) const 
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
bool VOIStandardlizeCanvas::getInvert ( const int which ) const 
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
void VOIStandardlizeCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::setCenter ( const int which, const int center ) 
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
void VOIStandardlizeCanvas::setInvert ( const int which, const bool invert ) 
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
void VOIStandardlizeCanvas::setScale   ( const double scale )  {  //aka magnification
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

//	ChangeVOIStandardlize();
}
//----------------------------------------------------------------------
void VOIStandardlizeCanvas::setSliceNo ( const int which, const int sliceNo ) {
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
void VOIStandardlizeCanvas::setWidth ( const int which, const int width ) 
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
IMPLEMENT_DYNAMIC_CLASS ( VOIStandardlizeCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( VOIStandardlizeCanvas, wxPanel )
    EVT_PAINT(            VOIStandardlizeCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           VOIStandardlizeCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        VOIStandardlizeCanvas::OnLeftDown     )
    EVT_LEFT_UP(          VOIStandardlizeCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      VOIStandardlizeCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        VOIStandardlizeCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       VOIStandardlizeCanvas::OnRightDown    )
    EVT_RIGHT_UP(         VOIStandardlizeCanvas::OnRightUp      )
    EVT_CHAR(             VOIStandardlizeCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
