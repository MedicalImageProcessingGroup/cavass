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
 * \file:  AlgebraCanvas.cpp
 * \brief  AlgebraCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "AlgebraCanvas.h"

using namespace std;
//----------------------------------------------------------------------
AlgebraCanvas::AlgebraCanvas ( void )  {  puts("AlgebraCanvas()");  }
//----------------------------------------------------------------------
AlgebraCanvas::AlgebraCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
	m_FileOrDataCount = 0;
    m_scale          = 1.0;
    m_overlay        = true;
    m_images[0] = m_images[1] = m_images[2] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = m_bitmaps[2] = NULL;	
    m_rows = 1;
	m_cols = 2;
    m_tx = m_ty     = 40;
	m_DivThre = 1;
	m_Offset = 0;
	m_Coefficient = 1;

	m_sliceData1 = m_sliceData2 = m_sliceOut = NULL;	
	m_sliceOutData = NULL;

    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

    m_AlgebraType = ALGEBRA_APLUSB;
    m_bAlgebraDone = false;    
}
//----------------------------------------------------------------------
AlgebraCanvas::~AlgebraCanvas ( void ) 
{
    cout << "AlgebraCanvas::~AlgebraCanvas" << endl;
    wxLogMessage( "AlgebraCanvas::~AlgebraCanvas" );
	 
	while (mCavassData!=NULL) {
        CavassData*  tmp = mCavassData;
        mCavassData = mCavassData->mNext;
        delete tmp;
    }

	if( m_sliceData1 != NULL )
	{
		delete m_sliceData1;
		m_sliceData1 = NULL;
	}
	if( m_sliceData2 != NULL )
	{
		delete m_sliceData2;
		m_sliceData2 = NULL;
	}
	if( m_sliceOut != NULL )
	{
		delete m_sliceOut;
		m_sliceOut = NULL;
	}
	if( m_sliceOutData != NULL )
	{
		free(m_sliceOutData);
		m_sliceOutData = NULL;
	}
	
    freeImagesAndBitmaps(); 

    release(); /* mCanvassData released in MainCanvas */
}
//----------------------------------------------------------------------
void AlgebraCanvas::release ( void )
{
}
//----------------------------------------------------------------------
void AlgebraCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
int AlgebraCanvas::loadFile ( const char* const fn )
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    
    wxYield();
    release();

    assert( m_FileOrDataCount>=0 && m_FileOrDataCount<=1 );

    if (fn==NULL || strlen(fn)==0) {
        if (m_FileOrDataCount == 0)
		{
			SetCursor( *wxSTANDARD_CURSOR );
        	return ERR_FILENAME;
		}
		m_sliceData2 = new SliceData();
		if (m_sliceData2 == NULL)
			return ERR_CLASSCONSTRUCTION;
    }
	else
	{
	    CavassData*  cd = new CavassData( fn, true );
		SliceData*  sd = new SliceData( fn );

		if( cd == NULL )
			return ERR_CLASSCONSTRUCTION;
		if (!cd->mIsCavassFile || !cd->m_vh_initialized)
		{
			wxMessageBox("Failed to load file.");
			return ERR_LOADCAVASSFILE;
		}

	    if (m_FileOrDataCount==0) 
		{
	        assert( mCavassData==NULL );
	        mCavassData = cd;

			m_sliceData1 = sd;
			m_sliceData1->m_sliceNo = -1;
	    } 
		else
		{
	        assert( mCavassData!=NULL && mCavassData->mNext==NULL );
	        mCavassData->mNext = cd;

			m_sliceData2 = sd;	
			m_sliceData2->m_sliceNo = -1;
	    }
	    ++m_FileOrDataCount;
	}

    if (isLoaded(0) && isLoaded(1)) 
	{
        CavassData&  A = *mCavassData;
        CavassData&  B = mCavassData->mNext? *(mCavassData->mNext): A;

		if( A.m_xSize != B.m_xSize || A.m_ySize != B.m_ySize ) //|| A.m_vh.scn.num_of_bits != B.m_vh.scn.num_of_bits )
			return 100;  // the image size is different

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

        int  w, h;
        GetSize( &w, &h );
        m_scale = (double)((w-100)*1.0 / (mOverallXSize * 3));        
        
		mOverallXSize = mCavassData->m_xSize;
		mOverallYSize = mCavassData->m_ySize;
		mOverallZSize = mCavassData->m_zSize;
		m_rows = 1;
		m_cols = 1;
	}

	if (m_sliceOut == NULL)
	{
		m_sliceOut = new SliceData(); 

		m_sliceOut->m_xSize = m_sliceData1-> m_xSize;
		m_sliceOut->m_ySize = m_sliceData1-> m_ySize;
		m_sliceOut->m_zSize = 1;
		m_sliceOut->m_xSpacing = m_sliceData1-> m_xSpacing;
		m_sliceOut->m_ySpacing = m_sliceData1-> m_ySpacing;
		m_sliceOut->m_size = 2;
		m_sliceOut->m_bytesPerSlice =
			m_sliceData1->m_xSize*m_sliceData1->m_ySize*2;
		m_sliceOut->mDataType = m_sliceData1-> mDataType;
		m_sliceOut->m_scale = m_sliceData1-> m_scale;
		m_sliceOut->mIsCavassFile = m_sliceData1-> mIsCavassFile;
		m_sliceOut->mIsDicomFile = m_sliceData1-> mIsDicomFile;
		m_sliceOut->mIsImageFile = m_sliceData1-> mIsImageFile;		
		m_sliceOut->mIsBinaryVtkFile = m_sliceData1->mIsBinaryVtkFile;
		m_sliceOut->mIsAsciiVtkFile = m_sliceData1->mIsAsciiVtkFile;
		m_sliceOut->m_min = 0;
		m_sliceOut->m_max = 65535;
		m_sliceOut->m_lut = (unsigned char*)malloc(65536);
	}

	if (mCavassData->m_vh.scn.num_of_bits == 1)
	{
		setCenter( 0, 0 );
		setWidth( 0, 1 );
		m_sliceData1->initLUT();
	}

	if (mCavassData->mNext && mCavassData->mNext->m_vh.scn.num_of_bits == 1)
	{
		setCenter( 1, 0 );
		setWidth( 1, 1 );
		m_sliceData2->initLUT();
	}

	if (mCavassData->m_vh.scn.num_of_bits == 1 && mCavassData->mNext &&
		mCavassData->mNext->m_vh.scn.num_of_bits == 1)
	{
		setCenter( 2, 0 );
		setWidth( 2, 1 );
		m_sliceOut->setCenter(0);
		m_sliceOut->setWidth(1);
	}
	else
	{
		m_sliceOut->setCenter(32768);
		m_sliceOut->setWidth(65536);
	}
	m_sliceOut->initLUT();

    reload();

    SetCursor( *wxSTANDARD_CURSOR );
	return 0;
}
//----------------------------------------------------------------------
void AlgebraCanvas::initLUT ( const int which ) 
{
    assert( which==0 || which==1 || which==2 );
    if (!isLoaded(which))    
		return;

    if (which==0) 
		m_sliceData1->initLUT();
    else if (which==1) 
		m_sliceData2->initLUT();
	else
		m_sliceOut->initLUT();
}
//----------------------------------------------------------------------
void AlgebraCanvas::CreateDisplayImage(int which)
{
    //note: image data is 24-bit rgb
	const CavassData&  A = *mCavassData;
    const CavassData&  B = mCavassData->mNext? *(mCavassData->mNext): A;
	
	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}

	if ( which == 0 ) 
	{
		if (A.m_sliceNo >= A.m_zSize)
			return;
		if (A.m_size==1) 
		{
			unsigned char*   cData = NULL;
			cData = (unsigned char*)m_sliceData1->getSlice(A.m_sliceNo); //A.m_data;
			if( m_sliceData1->m_sliceNo != A.m_sliceNo )   // It don't need to initial LUT when Adjust the GreyMap
			{	
				m_sliceData1->m_sliceNo = A.m_sliceNo;
			}

			if( cData == NULL )
				return;

			for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
			{			
				slice[i] = slice[i+1] = slice[i+2] = m_sliceData1->m_lut[cData[j] - m_sliceData1->m_min];			
			}

			cData = NULL;

		} 
		else if (A.m_size==2) 
		{
			unsigned short* sData = NULL;
			sData = (unsigned short*)m_sliceData1->getSlice(A.m_sliceNo); //(A.m_data);
			if( m_sliceData1->m_sliceNo != A.m_sliceNo )
			{						
				m_sliceData1->m_sliceNo = A.m_sliceNo;
			}

			if( sData == NULL )
				return;

			for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
			{			
				slice[i] = slice[i+1] = slice[i+2] = m_sliceData1->m_lut[sData[j] - m_sliceData1->m_min];			
			}

			sData = NULL;			
		} 
		else if (mCavassData->m_size==4) 
		{
			int* iData = NULL;
			iData = (int*)m_sliceData1->getSlice(A.m_sliceNo); //(A.m_data);
			if( m_sliceData1->m_sliceNo != A.m_sliceNo )
			{	
				m_sliceData1->m_sliceNo = A.m_sliceNo;
			}

			if( iData == NULL )
				return;

			for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
			{			
				slice[i] = slice[i+1] = slice[i+2] = m_sliceData1->m_lut[iData[j] - m_sliceData1->m_min];

			}

			iData = NULL;
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
	else if(which==1)
	{
		if (mCavassData->mNext==NULL || B.m_sliceNo >= B.m_zSize)
			return;
		if (B.m_size==1) 
		{
			unsigned char*   cData = NULL;
			cData = (unsigned char*)m_sliceData2->getSlice(B.m_sliceNo); //B.m_data;
			if( m_sliceData2->m_sliceNo != B.m_sliceNo )
			{						
				m_sliceData2->m_sliceNo = B.m_sliceNo;
			}

			if( cData == NULL )
				return;

			for (int i=0,j=0; i<B.m_xSize*B.m_ySize*3 && j<B.m_xSize*B.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = m_sliceData2->m_lut[cData[j] - m_sliceData2->m_min];
			}

			cData = NULL;

		} 
		else if (B.m_size==2) 
		{
			unsigned short* sData = NULL;
			sData = (unsigned short*)m_sliceData2->getSlice(B.m_sliceNo); //(B.m_data);	  
			if( m_sliceData2->m_sliceNo != B.m_sliceNo )
			{						   
				m_sliceData2->m_sliceNo = B.m_sliceNo;
			}			

			if( sData == NULL )
				return;

			for (int i=0,j=0; i<B.m_xSize*B.m_ySize*3 && j<B.m_xSize*B.m_ySize; i+=3,j++) 
			{				
				slice[i] = slice[i+1] = slice[i+2] = m_sliceData2->m_lut[sData[j] - m_sliceData2->m_min];		  
			}

			sData = NULL;			
		} 
		else if (B.m_size==4) 
		{
			int* iData = NULL;
			iData = (int*)m_sliceData2->getSlice(B.m_sliceNo); //(A.m_data);
			if( m_sliceData2->m_sliceNo != B.m_sliceNo )
			{	
				m_sliceData2->m_sliceNo = B.m_sliceNo;
			}

			if( iData == NULL )
				return;

			for (int i=0,j=0; i<B.m_xSize*B.m_ySize*3 && j<B.m_xSize*B.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = m_sliceData2->m_lut[iData[j] - m_sliceData2->m_min];
			}

			iData = NULL;
		}

		if(m_images[1]!=NULL)
			m_images[1]->Destroy();
		m_images[1] = new wxImage( B.m_xSize, B.m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[1]->Rescale( (int)(ceil(m_scale*B.m_xSize)),
				(int)(ceil(m_scale*B.m_ySize)) );
		}
		m_bitmaps[1] = new wxBitmap( *m_images[1] );

	}
	else if(which==2)
	{
		assert( m_sliceOut != NULL );		

		unsigned short*   sData;
		sData = (unsigned short*)m_sliceOutData;

		for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
		{	
			slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[sData[j] - m_sliceOut->m_min];
		}

		if(m_images[2]!=NULL)
			m_images[2]->Destroy();

		m_images[2] = new wxImage( m_sliceOut->m_xSize, m_sliceOut->m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[2]->Rescale( (int)(ceil(m_scale*m_sliceOut->m_xSize)),
				(int)(ceil(m_scale*m_sliceOut->m_ySize)) );
		}
		m_bitmaps[2] = new wxBitmap( *m_images[2] );
	}

	slice = NULL;	
	
}
//----------------------------------------------------------------------
void AlgebraCanvas::reload ( void ) 
{
    if (!isLoaded(0) || !isLoaded(1))    return;
	
	freeImagesAndBitmaps();
	CreateDisplayImage(0);
	CreateDisplayImage(1);

	if(m_bAlgebraDone) /* update */
	{		
		CreateDisplayImage(2);
	}

    Refresh();
}

//----------------------------------------------------------------------
void AlgebraCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}

void AlgebraCanvas::RunAlgebra()
{
	assert( m_sliceData1 != NULL && m_sliceData2 != NULL );
	const CavassData&  A = *mCavassData;
    const CavassData&  B = mCavassData->mNext? *(mCavassData->mNext): A;			

	int bytesPerSlice = m_sliceData1->m_xSize * m_sliceData1->m_ySize * 2;
	if( m_sliceOutData == NULL )
	{
		m_sliceOutData = (unsigned short *)malloc(bytesPerSlice); 

		if( m_sliceOutData == NULL )
			return;
	}
	/* initialize the memory of Algebra Data */
	memset( m_sliceOutData, 0, bytesPerSlice );

	long size = m_sliceData1->m_xSize * m_sliceData1->m_ySize;	
	
	unsigned short *AData = (unsigned short*)malloc(bytesPerSlice); 
	if( AData == NULL )
			return;

	unsigned short *BData = (unsigned short*)malloc(bytesPerSlice); 
	if( BData == NULL )
			return;

	m_bAlgebraDone = true;
	if (mCavassData->m_sliceNo >= mCavassData->m_zSize)
		memset(AData, 0, bytesPerSlice);
	else if (m_sliceData1->m_size==1)
	{
		const unsigned char *AData8  = (unsigned char*)m_sliceData1->getSlice(A.m_sliceNo); //(A.m_data);; // + offset;					
		for(int i = 0;i<size;i++)
			AData[i] = AData8[i];
	}
	else if (m_sliceData1->m_size==2)
	{
		const unsigned short *AData16  = (unsigned short*)m_sliceData1->getSlice(A.m_sliceNo); //(A.m_data);; // + offset;					
		for(int i = 0;i<size;i++)
			AData[i] = AData16[i];
	}

	if (mCavassData->mNext==NULL ||
			mCavassData->mNext->m_sliceNo>=mCavassData->mNext->m_zSize)
		memset(BData, 0, bytesPerSlice);
	else if (m_sliceData2->m_size==1)
	{
		const unsigned char *BData8  = (unsigned char*)m_sliceData2->getSlice(B.m_sliceNo); //(A.m_data);; // + offset;					
		for(int i = 0;i<size;i++)
			BData[i] = BData8[i];
	}
	else if (m_sliceData2->m_size==2)
	{
		const unsigned short *BData16  = (unsigned short*)m_sliceData2->getSlice(B.m_sliceNo); //(A.m_data);; // + offset;					
		for(int i = 0;i<size;i++)
			BData[i] = BData16[i];
	}

	AlgebraCompute_Sw( AData, BData, m_sliceData1->m_xSize, m_sliceData1->m_ySize, m_sliceOutData );

	reload();
	Refresh();

	if( AData != NULL )
		free( AData );

	if( BData != NULL )
		free( BData );

}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void AlgebraCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "AlgebraCanvas::OnChar" << endl;
    wxLogMessage( "AlgebraCanvas::OnChar" );
    if (e.m_keyCode==' ') {
        if (isLoaded(1) && mCavassData->mNext) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    }
    //pass the event up to the parent frame
    //m_parent_frame->ProcessEvent( e );
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnMouseMove ( wxMouseEvent& e ) {
    //if (m_data==NULL)    return;
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
#if 0    
    //show the pixel value under the pointer
    int  x, y, z;
    mapWindowToData( dc.DeviceToLogicalX( pos.x ),
                     dc.DeviceToLogicalY( pos.y ), x, y, z );
    wxString  s;
    s.Printf( wxT("(%d,%d,%d)=%d -> %d"), x, y, z, getData(x,y,z),
              m_lut[getData(x,y,z)-m_min] );
    m_parent_frame->SetStatusText( s, 1 );
#endif
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
void AlgebraCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(0)) {
        mCavassData->mDisplay = false;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnRightUp ( wxMouseEvent& e ) {
    if (isLoaded(0)) {
        mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(1) && mCavassData->mNext) {
        mCavassData->mNext->mDisplay = false;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    if (isLoaded(1) && mCavassData->mNext) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnLeftDown ( wxMouseEvent& e ) {
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
void AlgebraCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnPaint ( wxPaintEvent& e )
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
void AlgebraCanvas::paint ( wxDC* dc )
{
      dc->SetTextBackground( *wxBLACK );
      dc->SetTextForeground( wxColour(Yellow) );
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

		if (m_bitmaps[1]!=NULL && m_bitmaps[1]->Ok()) 
		{
			   
			dc->DrawBitmap( *m_bitmaps[1], m_tx+(int)(mCavassData->m_xSize * m_scale)+20, m_ty );
		}

		if (m_bitmaps[2]!=NULL && m_bitmaps[2]->Ok()) 
		{
			dc->DrawBitmap( *m_bitmaps[2], m_tx+(int)(2*mCavassData->m_xSize * m_scale)+40, m_ty );			
		}
	}
	else if (m_backgroundLoaded)
	{
        int  w, h;
        dc->GetSize( &w, &h );
		h = 600;
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}
//----------------------------------------------------------------------
void AlgebraCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool AlgebraCanvas::isLoaded ( const int which ) const 
{
    if (which==0)
	{
		if (m_sliceData1==NULL)    
			return false;
		else
			return true;
    } 
	else if (which==1)
	{
       	if (m_sliceData2==NULL)    
			return false;
		else
			return true;
    }
	else if (which==2)
	{
        if (m_sliceOut==NULL )    
			return false;
        else
			return true;        
    }
	else 
	{
        assert( 0 );
    }
    return false;
}

bool AlgebraCanvas::getOverlay ( void ) const
{
    return m_overlay;
}

//----------------------------------------------------------------------
int AlgebraCanvas::getCenter ( const int which ) const
{
    if (which==0) 
	{
        assert( m_sliceData1!=NULL );        
        return m_sliceData1->m_center;
    } 
	else if (which==1) 
	{
        assert( m_sliceData2!=NULL );        
        return m_sliceData2->m_center;
    } 
	else if (which==2)
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
int AlgebraCanvas::getMax ( const int which ) const {
    if (which==0) 
	{
         assert( m_sliceData1!=NULL );        
        return m_sliceData1->m_max;
    } 
	else if (which==1)
	{
       assert( m_sliceData2!=NULL );        
        return m_sliceData2->m_max;
    } 
	else if (which==2)
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_max;
    } 
	else
	{
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int AlgebraCanvas::getMin ( const int which ) const {
    if (which==0) 
	{
         assert( m_sliceData1!=NULL );        
        return m_sliceData1->m_min;
    } 
	else if (which==1)
	{
         assert( m_sliceData2!=NULL );        
        return m_sliceData2->m_min;
    } 
	else if (which==2)
	{
         assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_min;
    } 
	else 
	{
        assert( 0 );
    } 
    return 0;  //should never get here
}
//----------------------------------------------------------------------
//number of slices in entire data set
int AlgebraCanvas::getNoSlices ( const int which ) const 
{
    if (which==0) 
	{
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_zSize;
    } 
	else if (which==1) 
	{
		if (mCavassData->mNext == NULL)
			return 0;
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_zSize;
    } 
	else if (which==2) 
	{        
        return 1;
    } 
	else 
	{
        assert( 0 );
    }
    return 0;  //should never get here
}

bool AlgebraCanvas::getAlgebraDone (void) const
{
	return m_bAlgebraDone;
}
//----------------------------------------------------------------------
double AlgebraCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int AlgebraCanvas::getSliceNo ( const int which ) const 
{
    if (which==0) 
	{
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_sliceNo;
    } 
	else if (which==1) 
	{
		if (mCavassData->mNext == NULL)
			return 0;
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_sliceNo;
    } 
	else if (which==2) 
	{        
        return 0;
    } 
	else 
	{
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int AlgebraCanvas::getWidth ( const int which ) const 
{
    if (which==0) 
	{
         assert( m_sliceData1!=NULL );        
        return m_sliceData1->m_width;
    } 
	else if (which==1) 
	{
          assert( m_sliceData2!=NULL );        
        return m_sliceData2->m_width;
    } 
	else if (which==2) 
	{
         assert( m_sliceOut!=NULL );        
        return m_sliceOut->m_width;
    } 
	else 
	{
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
bool AlgebraCanvas::getInvert ( const int which ) const 
{
    if (which==0) 
	{
          assert( m_sliceData1!=NULL );        
        return m_sliceData1->mInvert;
    } 
	else if (which==1) 
	{
       assert( m_sliceData2!=NULL );        
        return m_sliceData2->mInvert;
    }
	else if (which==2) 
	{
       assert( m_sliceOut!=NULL );        
        return m_sliceOut->mInvert;
    } 
	else 
	{
        assert( 0 );
    }
    return false;  //should never get here
}
void AlgebraCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void AlgebraCanvas::setCenter ( const int which, const int center ) 
{
    if (which==0) 
	{
      assert( m_sliceData1!=NULL );                
        m_sliceData1->m_center = center;
    } 
	else if (which==1) 
	{
      assert( m_sliceData2!=NULL );                
        m_sliceData2->m_center = center;
    } 
	else if (which==2) 
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
void AlgebraCanvas::setInvert ( const int which, const bool invert ) 
{
    if (which==0) 
	{
       assert( m_sliceData1!=NULL );                
        m_sliceData1->mInvert = invert;
    } 
	else if (which==1) 
	{
        assert( m_sliceData2!=NULL );                
        m_sliceData2->mInvert = invert;
    }
	else if (which==2) 
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
void AlgebraCanvas::setScale   ( const double scale )  
{  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
//    freeImagesAndBitmaps();

    m_scale = scale;

	//mCavassData->m_scale = scale;
    int  w, h;
    GetSize( &w, &h );
    m_cols  = (int)(w / (mOverallXSize * m_scale));
    m_rows  = (int)(h / (mOverallYSize * m_scale));
    if (m_cols<1)  m_cols=1;
    if (m_rows<1)  m_rows=1;
    reload();
}
//----------------------------------------------------------------------
void AlgebraCanvas::setSliceNo ( const int which, const int sliceNo ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_sliceNo = sliceNo;		
    } else if (which==1) {
        if (mCavassData->mNext == NULL)
			return;
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_sliceNo = sliceNo;
    } 
	else if (which==2) 
	{       
    } 
	else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
void AlgebraCanvas::setWidth ( const int which, const int width ) 
{
    if (which==0) 
	{
         assert( m_sliceData1!=NULL );                
        m_sliceData1->m_width = width;
    } 
	else if (which==1) 
	{
         assert( m_sliceData2!=NULL );                
        m_sliceData2->m_width = width;
    } 
	else if (which==2) 
	{
         assert( m_sliceOut!=NULL );                
        m_sliceOut->m_width = width;
    } 
	else {
        assert( 0 );
    }
}

typedef struct Quadratic {float xx, xy, yy, x, y, cnst;} Quadratic;

#define Quadratic_value(X, Y, q) \
	((X)*((q).xx*(X)+(q).xy*(Y)+(q).x)+(Y)*((q).yy*(Y)+(q).y)+(q).cnst)


int AlgebraCanvas::AlgebraCompute_Sw( unsigned short *inA, unsigned short *inB, int width, int height, unsigned short *pOut)
{
	long j;
	long length = width * height;	

	Quadratic q_out, q_mask, q_alt;
	static Quadratic q_null;	
	int div, rev=0;
	int threshold = m_DivThre;
	int offset = (int)m_Offset;
	float max1, max2, val1, val2;
	max1 = m_sliceData1->m_max;
	max2 = m_sliceData2->m_max;

	div = FALSE;
	q_out = q_mask = q_alt = q_null;
	switch (m_AlgebraType)
	{
	case ALGEBRA_APLUSB:
		{
			q_out.x = q_out.y = 1;
			q_mask.cnst = 1;
		}
		break;
	case ALGEBRA_AMINUSB:
		{
			q_out.x = 1;
			q_out.y = -1;
			q_out.cnst = offset;
			q_mask.cnst = 1;
		}
		break;
	case ALGEBRA_BMINUSA:
		{
			q_out.x = -1;
			q_out.y = 1;
			q_out.cnst = offset;
			q_mask.cnst = 1;
		}
		break;		
	case ALGEBRA_ABSAMINUSB:
		{
			q_out.x = 1;
			q_out.y = -1;
			q_mask.x = 1;
			q_mask.y = -1;
			q_alt.x = -1;
			q_alt.y = 1;
		}
		break;
	case ALGEBRA_AXB:
		{
			q_out.xy = m_Coefficient;
			q_mask.cnst = 1;
		}
		break;
	case ALGEBRA_ADIVIDEB:
		{
			div = TRUE;
			rev = FALSE;
		}
		break;
	case ALGEBRA_BDIVIDEA:
		{
			div = TRUE;
			rev = TRUE;
		}
		break;
	case ALGEBRA_AGREATB:
		{
			q_out.x = 1;
			q_mask.x = 1;
			q_mask.y = -1;
		}
		break;
	case ALGEBRA_BGREATA:
		{
			q_out.y = 1;
			q_mask.x = -1;
			q_mask.y = 1;
		}
		break;
	case ALGEBRA_MEAN:
		{
			q_out.x = q_out.y = .5;
			q_mask.cnst = 1;
		}
		break;	
	default:
		m_bAlgebraDone = false;
		break;
	}

	for (j=length-1; j>=0; j--)
	{
		val1 = inA[j];
		val2 = inB[j];
		if (div)
			val1 =
			val1<threshold || val2<threshold
			?	0
			:	rev
			?	threshold/max2*65535*val2/val1
			:	threshold/max1*65535*val1/val2;
		else if (Quadratic_value(val1, val2, q_mask) > 0)
			val1 = Quadratic_value(val1, val2, q_out);
		else
			val1 = Quadratic_value(val1, val2, q_alt);
		if (val1 < 0)
			val1 = 0;
		if (val1 > 65535)
			val1 = 65535;
		pOut[j] = (unsigned short)val1;
	}

	return 1;
}


bool AlgebraCanvas::IsImgSizeEqual()
{
	assert( mCavassData != NULL);
	if (mCavassData->mNext == NULL)
		return true;
	
	if( mCavassData->m_xSize == mCavassData->mNext->m_xSize && mCavassData->m_ySize == mCavassData->mNext->m_ySize )
		return true;
	else
		return false;
}

//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( AlgebraCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( AlgebraCanvas, wxPanel )
    EVT_PAINT(            AlgebraCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           AlgebraCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        AlgebraCanvas::OnLeftDown     )
    EVT_LEFT_UP(          AlgebraCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      AlgebraCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        AlgebraCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       AlgebraCanvas::OnRightDown    )
    EVT_RIGHT_UP(         AlgebraCanvas::OnRightUp      )
    EVT_CHAR(             AlgebraCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
