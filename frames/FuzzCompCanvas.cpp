/*
  Copyright 1993-2017 Medical Image Processing Group
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
 * \file   FuzzCompCanvas.cpp
 * \brief  FuzzCompCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "ChunkData.h"
#include  "FuzzCompCanvas.h"

using namespace std;

#define MAX_REL_DIFF 65535
#define MAX_AFFINITY 65535
#define MIN_FUNCTION_WIDTH ((float).01)
#define MIN_CONNECTIVITY_WIDTH .001
#define FuzzyAnd(a, b) ((a)<(b)?(a):(b))
#define MAX_CONNECTIVITY 65535
#define ConF(x) (conn_t)(and_op!=2? (x):MAX_CONNECTIVITY*exp((x)*(1./MAX_AFFINITY)-1))
#define MAX(a, b) ( (a) < (b) ? (b) : (a) )

//----------------------------------------------------------------------
const int  FuzzCompCanvas::sSpacing=0;  //space, in pixels, between each slice
//----------------------------------------------------------------------
FuzzCompCanvas::FuzzCompCanvas ( void )  : MainCanvas()  { 
      puts("FuzzCompCanvas()");
          mCavassData = NULL;
}
//----------------------------------------------------------------------
FuzzCompCanvas::FuzzCompCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
{
	mCavassData = NULL;
	m_scale = 1.0;
	m_overlay        = true;
	m_training       = true;
	m_painting       = false;
	m_erasing        = false;
	m_weight[0] =  m_weight[1] = m_weight[2] = m_weight[3] = m_weight[4] = 100;
	m_level [0] =  m_level[1] = m_level[2] = m_level[3] = m_level[4] = 0;
	m_widthLevel[0]  =  m_widthLevel[1] = m_widthLevel[2] = m_widthLevel[3] = m_widthLevel[4] = 0;
	m_images         = (wxImage**)NULL;
	m_bitmaps        = (wxBitmap**)NULL;
	m_bitmaps1       = (wxBitmap**)NULL;
	m_bitmaps2       = (wxBitmap**)NULL;
	myFrame   = parent_frame;
	m_rows = m_cols = 0;
	m_tx = m_ty = 0;
	m_sliceNo        = 0;
	m_BrushSize    = 2;

    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	m_FuzzCompType = PARAMETER;
	m_bFuzzCompDone = false;
	m_bCostImgDone = false;

	nSeed = mNoPaintingPts = mNoErasingPts = 0;
	for (int i=0; i<MAXPOINTS; i++) 
	{
		mData[i][0] = mData[i][1] = mData[i][2] = -1;
	}
        
	H = NULL;
	affinity_type = 0;
	training_sample = NULL;
	training_samples = 0;
	max_training_samples = 0;
	training_image_flag = FALSE;
	training_image = NULL;
	histogram_counts = NULL;
	connectivity_data = NULL;
	affinity_data_valid = false;
	connectivity_data_valid = false;
	masked_original_valid = false;
	threshold = 0;
	computed_threshold = 100;
	and_op = 0;
	feature_selected = 0;

	affinity_data_across = affinity_data_down = NULL;
	for( int i=0; i<5; i++ )
	{
		function_selected[i] = 0;
		feature_status[i] = 0;
		feature_data_across[i] = NULL;
		feature_data_down[i] = NULL;
		feature_data_valid[i] = 0;
	}
	nbor[0].x = 1; nbor[0].y = 0;
	nbor[1].x = 0; nbor[1].y = 1;
	nbor[2].x = -1; nbor[2].y = 0;
	nbor[3].x = 0; nbor[3].y = -1;

	histogram_bins[0] = 50;   
	histogram_bins[1] = 50; //={50, 50};	
	for( int i=0; i<NUM_FEATURES; i++)
	{
		weight[i] = m_weight[i];
		function_level[i] = m_level[i];
		function_width[i] = m_widthLevel[i];
		training_min[i] = 65535;
		training_max[i] = 0;
		training_sum[i] = 0;

		for( int j=0; j<NUM_FEATURES; j++)
		{
			training_sum_sqr[i][j] = 0;
		}
	}	
	affinityImg = NULL;
	connectivityImg = NULL;

	m_bPararell = false;

	m_paintingPts = NULL;
	m_erasingPts = NULL;
	m_brushSizePts = NULL;
}
//----------------------------------------------------------------------
FuzzCompCanvas::~FuzzCompCanvas ( void ) 
{
    cout << "FuzzCompCanvas::~FuzzCompCanvas" << endl;
    wxLogMessage( "FuzzCompCanvas::~FuzzCompCanvas" );

	while (mCavassData!=NULL) {
        CavassData*  tmp = mCavassData;
        mCavassData = mCavassData->mNext;
        delete tmp;
    }

	if (training_image)
		free(training_image);
	training_image = NULL;

	if( m_paintingPts != NULL )
	delete []m_paintingPts;
	m_paintingPts = NULL;

	if( m_erasingPts != NULL )
	delete []m_erasingPts;
	m_erasingPts = NULL;

	if( m_brushSizePts != NULL )
	delete []m_brushSizePts;
	m_brushSizePts = NULL;

    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
        
}
//----------------------------------------------------------------------
void FuzzCompCanvas::release ( void ) {
}  

void FuzzCompCanvas::resetScale ( void ) 
{
    int  w, h;
        GetSize( &w, &h );
		m_scale = (double) h/mOverallYSize;
		if( m_scale > (double) w/3/mOverallXSize )
			m_scale = (double) w/3/mOverallXSize;

        CavassData*  tmp = mCavassData;
        while (tmp!=NULL) 
		{
            tmp->m_scale = m_scale;
                tmp = tmp->mNext;
        }
}
//----------------------------------------------------------------------
void FuzzCompCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void FuzzCompCanvas::loadFile ( const char* const fn ) 
{
	SetCursor( wxCursor(wxCURSOR_WAIT) );    
	wxYield();
	if (fn==NULL || strlen(fn)==0) {
		SetCursor( *wxSTANDARD_CURSOR );
		return;
	}
	ChunkData*  cd = new ChunkData( fn );
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file."); 
		return;
	}
	mCavassData = cd;


	/* for filtered CavassData, both will be deleted in MainCanvas */

	ChunkData*  cd2 = new ChunkData( fn );
	ChunkData*  cd3 = new ChunkData( fn );
	free(cd2->m_lut);
	free(cd3->m_lut);
	cd2->m_lut = (unsigned char*)malloc(2*65536);
	cd3->m_lut = (unsigned char*)malloc(65536);
	cd3->m_max = MAX_AFFINITY;
	cd2->m_min = 0;
	cd3->m_min = 0;
	cd3->m_center = MAX_AFFINITY/2;
	cd3->m_width = MAX_AFFINITY;
	cd2->initLUT();
	cd3->initLUT();

	cd3->mNext = NULL;
	cd2->mNext = cd3;
	mCavassData->mNext = cd2;

	mOverallXSize = mCavassData->m_xSize;
	mOverallYSize = mCavassData->m_ySize;
	mOverallZSize = mCavassData->m_zSize;

	int  w, h;
	GetSize( &w, &h );
	m_scale = (double) h/mOverallYSize;
	if( m_scale > (double) w/(mOverallXSize*3) )
		m_scale = (double) w/(mOverallXSize*3) ;

	CavassData*  tmp = mCavassData;
	while (tmp!=NULL) 
	{
		tmp->m_scale = m_scale;
		tmp = tmp->mNext;
	}

	training_image = (unsigned char *)calloc(mOverallXSize*mOverallYSize, 1);

	m_cols = 3; //(int)(w / (mOverallXSize * m_scale));
	m_rows = 1; //(int)(h / (mOverallYSize * m_scale));

	if (m_cols<1)  m_cols=1;
	if (m_rows<1)  m_rows=1;
	reload();

	SetCursor( *wxSTANDARD_CURSOR );

}
//----------------------------------------------------------------------
void FuzzCompCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 || which==2);
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else  if (which==1)
	{
	    switch (feature_selected)
		{
			case 3:
				mCavassData->mNext->m_max = mCavassData->m_max*2;
				break;
			case 4:
				mCavassData->mNext->m_max = MAX_AFFINITY;
				break;
			default:
				mCavassData->mNext->m_max = mCavassData->m_max;
		}
		mCavassData->mNext->initLUT();
    }
	else  if (which==2) mCavassData->mNext->mNext->initLUT();
}
//----------------------------------------------------------------------
void FuzzCompCanvas::CreateDisplayImage ( int which ) 
{
    assert( which==0 || which==1 || which==2 );
    int  k;
    if (which==0) 
	{
        m_images = (wxImage**)malloc( 3 * sizeof(wxImage*) );  //m_cols * m_rows
        for (k=0; k<3; k++)    
			m_images[k]=NULL;

        m_bitmaps = (wxBitmap**)malloc( 3 * sizeof(wxBitmap*) );  //m_cols * m_rows
        for (k=0; k<3; k++) 
			m_bitmaps[k]=NULL;
    }
	
    CavassData*  A = mCavassData;
    if (which==1)    A = mCavassData->mNext;
    if (which==2)    A = mCavassData->mNext->mNext;
    assert( A!=NULL );

    //note: image data is 24-bit rgb
	unsigned char*  slice=(unsigned char *)malloc(A->m_xSize*A->m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}
    assert( m_sliceNo<A->m_zSize );
    const int  offset = 0;
	if( which == 0 )
	{
		if (A->m_size==1) {
			unsigned char*  ucData = (unsigned char*)(A->getSlice( m_sliceNo ));
			for (int i=0,j=offset; i<A->m_xSize*A->m_ySize*3 && j<offset+A->m_xSize*A->m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A->m_lut[ucData[j]-A->m_min];
			} 
		} else if (A->m_size==2) {
			unsigned short* sData = (unsigned short*)(A->getSlice( m_sliceNo ));
			for (int i=0,j=offset; i<A->m_xSize*A->m_ySize*3 && j<offset+A->m_xSize*A->m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A->m_lut[sData[j]-A->m_min];
			}
		} else if (A->m_size==4) {
			int*  iData = (int*)(A->getSlice( m_sliceNo ));
			for (int i=0,j=offset; i<A->m_xSize*A->m_ySize*3 && j<offset+A->m_xSize*A->m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = A->m_lut[iData[j]-A->m_min];
			}
		}
		if (m_training)
			for (int i=0,j=offset; i<A->m_xSize*A->m_ySize*3 && j<offset+A->m_xSize*A->m_ySize; i+=3,j++)
			{
				if (training_image[j])
				{
					slice[i+1] = 200;
					slice[i+2] = 100;
				}
			}
	}
	else
	{
		conn_t cthreshold = (conn_t)(threshold*MAX_CONNECTIVITY*.01);
		bool do_threshold=false;
		unsigned short *sData1 = NULL, *sData2 = NULL;
		if( which == 1 )
		{
			sData1 = feature_data_across[feature_selected];
			sData2 = feature_data_down[feature_selected];
		}
		else
		{
			if (!m_training)
				for (int j=0; j<nSeed; j++)
					if (mData[j][2] == m_sliceNo)
					{
						do_threshold = true;
						break;
					}
			if (do_threshold)
			{
				sData1 = connectivity_data;
			}
			else
			{
			    if (nSeed == 0)
				{
					sData1 = affinity_data_across;
					sData2 = affinity_data_down;
				}
			}
		}
		if( sData1 == NULL )
			return;

		for (int i=0,j=offset; i<A->m_xSize*A->m_ySize*3 &&
			    j<offset+A->m_xSize*A->m_ySize; i+=3,j++) 
		{
			slice[i] = slice[i+1] = slice[i+2] =
			    do_threshold?
					(sData1[j]>=cthreshold? A->m_lut[sData1[j]]: 0):
					A->m_lut[((int)sData1[j]+sData2[j])/2-A->m_min];
		}
	}

    m_images[which] = new wxImage( A->m_xSize, A->m_ySize, slice );
    if (m_scale!=1.0)
	{
        m_images[which]->Rescale( (int)(ceil(m_scale*A->m_xSize)),
                                  (int)(ceil(m_scale*A->m_ySize)) );
    }
    //gjg: m_bitmaps[which] = new wxBitmap( m_images[which] );
    m_bitmaps[which] = new wxBitmap( *m_images[which] );
}  
 //----------------------------------------------------------------------
void FuzzCompCanvas::reload ( void ) 
{
	 if (!isLoaded(0))   
		 return;
	 freeImagesAndBitmaps();

	 CreateDisplayImage(0);
	 if( feature_data_valid[feature_selected] )
		 CreateDisplayImage(1);
	 if( affinity_data_valid )
		 CreateDisplayImage(2);
    
	 Refresh();
}
//----------------------------------------------------------------------
void FuzzCompCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
  //map a point in the window back into the 3d data set
    wx -= m_tx;    wy -= m_ty;
    const int  col = (int)((double)wx / (ceil(mCavassData->m_xSize*mCavassData->m_scale)+sSpacing));
    const int  row = (int)((double)wy / (ceil(mCavassData->m_ySize*mCavassData->m_scale)+sSpacing));
    x = (int)( wx % ((int)(ceil(mCavassData->m_xSize*mCavassData->m_scale)+sSpacing)) / mCavassData->m_scale );
    y = (int)( wy % ((int)(ceil(mCavassData->m_ySize*mCavassData->m_scale)+sSpacing)) / mCavassData->m_scale );
    z = m_sliceNo + row*m_cols + col;
    
    //clamp the values to acceptable values
    if (x<0)  x=0;
    else if (x>=mCavassData->m_xSize)  x=mCavassData->m_xSize-1;
    if (y<0)  y=0;
    else if (y>=mCavassData->m_ySize)  y=mCavassData->m_ySize-1;
    if (z<0)  z=0;
    else if (z>=mCavassData->m_zSize)  z=mCavassData->m_zSize-1;
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void FuzzCompCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "FilterCanvas::OnChar" << endl;
    wxLogMessage( "FuzzCompCanvas::OnChar" );
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
void FuzzCompCanvas::OnMouseMove ( wxMouseEvent& e )
{
    if (mCavassData==NULL)    return;  //gjg
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
        //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
    
    int  x, y, z;
    mapWindowToData( dc.DeviceToLogicalX( pos.x ),
                     dc.DeviceToLogicalY( pos.y ), x, y, z );
    wxString  s;
        if ((wx>=0 && wy>=0) && (wx < mOverallXSize * m_scale && wy < mOverallYSize * m_scale)) 
		{
              s.Printf( wxT("(%d,%d,%d)=%d"), x, y, z, mCavassData->getData(x,y,z) );
			  m_parent_frame->SetStatusText( s, 1 );
        }
		else if ((wx >= mOverallXSize * m_scale && wy>=0) && 
                           (wx < 2* mOverallXSize * m_scale && wy < mOverallYSize * m_scale)) 
		{
			if( feature_data_valid[feature_selected] )
			{
			   s.Printf( wxT("(%d,%d),(%d,%d)"), x, y, feature_data_across[feature_selected][y*mCavassData->m_xSize + x],
						feature_data_down[feature_selected][y*mCavassData->m_xSize + x] );
			   m_parent_frame->SetStatusText( s, 1 );
			}
        }
		else if ((wx >= 2*mOverallXSize * m_scale && wy>=0) && 
                           (wx < 3* mOverallXSize * m_scale && wy < mOverallYSize * m_scale)) 
		{
			if( m_training && affinity_data_valid )
			{
			   s.Printf( wxT("(%d,%d),(%d,%d)"), x, y, affinity_data_across[y*mCavassData->m_xSize + x],
						           affinity_data_down[y*mCavassData->m_xSize + x] );
               m_parent_frame->SetStatusText( s, 1 );
			}
			else if( connectivity_data_valid )
			{
				  s.Printf( wxT("(%d,%d,%d)=%d"), x, y, z, connectivity_data[y*mCavassData->m_xSize + x] );
			  m_parent_frame->SetStatusText( s, 1 );
			}
        }
        if (m_painting || m_erasing)
		{
			int left, top, t_x, t_y, *t_flag;
			unsigned char **t_image;

			t_image = &training_image;
			t_flag = &training_image_flag;

			left = (int)rint(x-m_BrushSize*.5);
			top = (int)rint(y-m_BrushSize*.5);
			for (t_y=0; t_y<m_BrushSize; t_y++)
				if (top+t_y>=0 && top+t_y<mOverallYSize)
					for (t_x=0; t_x<m_BrushSize; t_x++)
						if (left+t_x>=0 && left+t_x<mOverallXSize)
							(*t_image)[(top+t_y)*mOverallXSize+left+t_x] =
								!m_erasing;
			if (m_painting)
				*t_flag = TRUE;
			assert( !(m_painting && m_erasing) );
            reload();
        }

        if (e.RightIsDown() && !m_training) {
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
void FuzzCompCanvas::ResetSeed ( void ) {

    for (int i=0; i<nSeed; i++)
        mData[i][0] = mData[i][1] = mData[i][2] = -1;
    nSeed = 0;
	connectivity_data_valid = FALSE;
}
//----------------------------------------------------------------------
void FuzzCompCanvas::ResetTraining ( void ) {

    memset(training_image, 0, mOverallXSize*mOverallYSize);
	training_image_flag = FALSE;
	affinity_data_valid = FALSE;
	connectivity_data_valid = FALSE;

	for( int i=0; i<NUM_FEATURES; i++)
	{		
		training_min[i] = 65535;
		training_max[i] = 0;
		training_sum[i] = 0;

		for( int j=0; j<NUM_FEATURES; j++)
		{
			training_sum_sqr[i][j] = 0;
		}
	}	
	training_samples = 0;
	if (training_sample)
		free(training_sample);
	training_sample = NULL;
	max_training_samples = 0;
	det_covariance = 0;
}

//----------------------------------------------------------------------
void FuzzCompCanvas::HandlePaint ( wxDC& mdc ) {
//draw the seed points indicated by the user
#if wxCHECK_VERSION(2, 9, 0)
    mdc.SetBrush( wxBrush(wxColour(Yellow), wxBRUSHSTYLE_SOLID) );
#else
    mdc.SetBrush( wxBrush(wxColour(Yellow), wxSOLID) );
#endif
        
	if( !m_training )
	{
		for (int i=0; i<nSeed ; i++) {  //points indicated by user
			int  wx, wy;
			mapDataToWindow(mData[i][0],mData[i][1], mData[i][2], wx, wy);
			if (wx>=0 && wy>=0 && wx < mOverallXSize * m_scale && wy < mOverallYSize * m_scale)
				mdc.DrawRectangle( wx-4, wy-4, 8, 8 );
		}
	}
}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnRightDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
        
        if (!m_training){
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

}
//----------------------------------------------------------------------
/** \brief load BIM mask when training mode is selected. Note that the mask must be of the same 
 * of the original volume.
 * /todo mask size checking and trainging parameters computation need to be implemented
 */
void FuzzCompCanvas::OnRightUp ( wxMouseEvent& e ) {
   SetFocus();  //to regain/recapture keypress events
}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
	SetFocus();  //to regain/recapture keypress events
	if (m_training) 
		setErasing(true);
	else
		FuzzyComp_update();
}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
        if (m_training) setErasing(false); 
}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnLeftDown ( wxMouseEvent& e ) {
	SetFocus();  //to regain/recapture keypress events


	wxClientDC  dc(this);
	PrepareDC(dc);
	const wxPoint  pos = e.GetPosition();
	int  x, y, z;
	mapWindowToData( dc.DeviceToLogicalX( pos.x ), dc.DeviceToLogicalY( pos.y ), x, y, z );
	if (m_training) 
	{
		if (mCavassData==NULL)    return;  //gjg

		setPainting(true);

		int left, top, t_x, t_y, *t_flag;
		unsigned char **t_image;

		t_image = &training_image;
		t_flag = &training_image_flag;

		left = (int)rint(x-m_BrushSize*.5);
		top = (int)rint(y-m_BrushSize*.5);
		for (t_y=0; t_y<m_BrushSize; t_y++)
			if (top+t_y>=0 && top+t_y<mOverallYSize)
				for (t_x=0; t_x<m_BrushSize; t_x++)
					if (left+t_x>=0 && left+t_x<mOverallXSize)
						(*t_image)[(top+t_y)*mOverallXSize+left+t_x] = 1;
		*t_flag = TRUE;

		reload();
	}

        if (!m_training) 
		{
		  if (nSeed >= MAXPOINTS-1)
		  {
		    wxMessageBox("Too many seed points.");
			return;
		  }
		  mData[nSeed][0] = x;
		  mData[nSeed][1] = y;
		  mData[nSeed][2] = z;
		  connectivity_data_valid = false;
          ++nSeed;
          FuzzCompFrame* frame = dynamic_cast<FuzzCompFrame*>(myFrame);
          frame->OnSeed();
          Refresh();
        }
}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnLeftUp ( wxMouseEvent& e ) {
        SetFocus();  //to regain/recapture keypress events
        if (m_training){
          setPainting(false);
          SetCursor( *wxSTANDARD_CURSOR );
        } 

}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnPaint ( wxPaintEvent& e ) {
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
    
    paint( m );

    wxPaintDC  dc(this);
    PrepareDC(dc);
    //dc.BeginDrawing();
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows
    //dc.EndDrawing();    
}
//----------------------------------------------------------------------
void FuzzCompCanvas::paint ( wxDC& dc ) {
   dc.SetTextBackground( *wxBLACK );
   dc.SetTextForeground( wxColour(Yellow) );

   if (m_bitmaps!=NULL) 
   {
       for (int c=0; c<3; c++) {
           if (m_bitmaps[c]!=NULL && m_bitmaps[c]->Ok() && mCavassData != NULL ) 
		   {
               const int  x = (int)(c*(ceil(mCavassData->m_xSize*mCavassData->m_scale)+sSpacing));
               const int  y = (int)(0*(ceil(mCavassData->m_ySize*mCavassData->m_scale)+sSpacing));
               dc.DrawBitmap( *m_bitmaps[c], x+m_tx, y+m_ty );
               const wxString  s = wxString::Format( "(%d)", m_sliceNo+1);
               if (m_overlay)    dc.DrawText(s, x+m_tx, y+m_ty);
           }
       }
//---------------------    
      int         w, h;
      GetSize( &w, &h );
 
      wxMemoryDC dcPaint;
  
      wxBitmap bitmapPaint(w, h);
  
      dcPaint.SelectObject(bitmapPaint);
      dcPaint.SetBackground(*wxTRANSPARENT_BRUSH);
	  dcPaint.Clear();

#if wxCHECK_VERSION(2, 9, 0)
	  dcPaint.SetBrush( wxBrush(wxColour(Green), wxBRUSHSTYLE_SOLID) );
	  dcPaint.SetPen( wxPen(wxColour(Green), 1, wxPENSTYLE_SOLID) );
#else
	  dcPaint.SetBrush( wxBrush(wxColour(Green), wxSOLID) );
	  dcPaint.SetPen( wxPen(wxColour(Green), 1, wxSOLID) );
#endif

	  dc.Blit( 0, 0, (int)(mOverallXSize * m_scale), (int)(mOverallYSize * m_scale), &dcPaint, 0, 0,wxAND_INVERT,true);

	  //-------------------------------------------

	  HandlePaint(dc);

   } 
   else if (m_backgroundLoaded) 
   {
	   int  w, h;
	   dc.GetSize( &w, &h );
	   const int  bmW = m_backgroundBitmap.GetWidth();
	   const int  bmH = m_backgroundBitmap.GetHeight();
	   dc.DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );

   } 
}
//----------------------------------------------------------------------
void FuzzCompCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool FuzzCompCanvas::isLoaded ( const int which ) const 
{
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
         } else if (which==2) {
        if (mCavassData==NULL || mCavassData->mNext==NULL ||
				mCavassData->mNext->mNext==NULL)
		    return false;
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else {
        assert( 0 );
        }
    return false;
}

//////////////////////  Fuzzy component algorithm ////////////////////////////
#define slice_data_8 ((unsigned char *)slice_data)
#define slice_data_16 ((unsigned short *)slice_data)
#define masked_original_8 ((unsigned char *)masked_original)
#define masked_original_16 ((unsigned short *)masked_original)

/*****************************************************************************
 * MACRO: Assign_feature
 * DESCRIPTION: Assigns the selected feature to out.
 * PARAMETERS:
 *    out: The variable to be assigned the feature value.
 *    high, low: The higher and lower values from which to compute the feature.
 *    feature_selected:
 *       0: high
 *       1: low
 *       2: difference
 *       3: sum
 *       4: relative difference (scaled to MAX_REL_DIFF)
 *       NUM_FEATURES: average
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/29/96 by Dewey Odhner
 *
 *****************************************************************************/
#define Assign_feature(out, high, low, feature_selected) \
switch (feature_selected) \
{ \
	case 0: \
		out = high; \
		break; \
	case 1: \
		out = low; \
		break; \
	case 2: \
		out = (high)-(low); \
		break; \
	case 3: \
		out = (high)+(low); \
		break; \
	case 4: \
		out = (high)+(low)==0? 0: MAX_REL_DIFF*((high)-(low))/((high)+(low)); \
		break; \
	case NUM_FEATURES: \
		out = ((high)+(low))/2; \
		break; \
	default: \
		assert(FALSE); \
}

int FuzzCompCanvas::ApplyTraining()
{
	int feature_n, feature_m, error_code, on_feature_n, on_feature_m,
		features_on, error_flag;
	long tsn;
	char msg[48];
	double covariance[NUM_FEATURES*NUM_FEATURES];


//	nbor[0] = { { 1, 0 }, { 0, 1 },  { -1, 0 }, { 0, -1 } };
	
	largest_density_value = (int)(mCavassData->m_vh.scn.largest_density_value[0] + 0.5);
	 smallest_density_value = (int)(mCavassData->m_vh.scn.smallest_density_value_valid? mCavassData->m_vh.scn.smallest_density_value[0]: 0);

	function_domain[0][0] = smallest_density_value;
    function_domain[0][1] = largest_density_value;
    if (function_domain[0][0] == function_domain[0][1])
		function_domain[0][1]++;
    function_domain[1][0] = function_domain[0][0];
    function_domain[1][1] = function_domain[0][1];
    function_domain[2][0] = 0;
    function_domain[2][1] = function_domain[0][1]-function_domain[0][0];
    function_domain[3][0] = 2*function_domain[0][0];
    function_domain[3][1] = 2*function_domain[0][1];
    function_domain[4][0] = 0;
    function_domain[4][1] = 100;

	accumulate_training();
	if (training_samples<=1 )
	{
		m_parent_frame->SetStatusText("Training samples not specified.", 1);		
		return -1;
    }
	error_flag = FALSE;
	if (training_sample)
	{
		if (histogram_counts == NULL)
			histogram_counts = (int *)malloc(histogram_bins[0]*histogram_bins[1]*sizeof(int));
		if (histogram_counts == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc error.", 1);		
			return -1;
		}
		else
		{
			memset(histogram_counts, 0, histogram_bins[0]*histogram_bins[1]*sizeof(int));
			
			for (tsn=0; tsn<training_samples; tsn++)
				histogram_counts[training_sample[2*tsn]* 
				(histogram_bins[0]-1)/largest_density_value
				*histogram_bins[1]+training_sample[2*tsn+1]*
				(histogram_bins[1]-1)/largest_density_value]++;
		}		
	}

	for (feature_n=features_on=0; feature_n<NUM_FEATURES; feature_n++)
		if (feature_status[feature_n])
			if (++features_on>2 && feature_n<4)
			{
				features_on = 0;
				break;
			}

	if (training_samples > 1)
	{
		for (feature_n=on_feature_n=0; feature_n<NUM_FEATURES; feature_n++)
		{
			if (function_selected[feature_n] == 0)
				switch (feature_n)
				{
					case 0:
					case 1:
					case 3:
						function_level[feature_n] = training_sum[feature_n]/training_samples;
						function_width[feature_n] =
							2*sqrt((training_sum_sqr[feature_n][feature_n]-
							training_sum[feature_n]*training_sum[feature_n]/
							training_samples)/(training_samples-1));
						if (feature_n==3 && largest_density_value>32767)
						{
							function_level[3] *= 2;
							function_width[3] *= 2;
						}
						break;
					case 2:
					case 4:
						function_width[feature_n] = 2*sqrt(training_sum_sqr[
							feature_n][feature_n]/training_samples);
						function_level[feature_n] = 0;
						if (feature_n == 4)
						{
							function_level[4] *= (float)100./MAX_REL_DIFF;
							function_width[4] *= (float)100./MAX_REL_DIFF;
						}
						break;
					default:
						assert(FALSE);
				}
			else
			{
				function_level[feature_n] =
					(training_max[feature_n]+training_min[feature_n])/2;
				function_width[feature_n] =
					training_max[feature_n]-training_min[feature_n];
				if (feature_n==3 && largest_density_value>32767)
				{
					function_level[3] *= 2;
					function_width[3] *= 2;
				}
				if (feature_n == 4)
				{
					function_level[4] *= (float)100./MAX_REL_DIFF;
					function_width[4] *= (float)100./MAX_REL_DIFF;
				}
			}
			if (features_on && feature_status[feature_n])
			{
				training_mean[feature_n] = training_sum[feature_n]/training_samples;
				for (feature_m=on_feature_m=0;feature_m<=feature_n;feature_m++)
					if (feature_status[feature_m])
					{
						covariance[on_feature_n*features_on+on_feature_m] =
							covariance[on_feature_m*features_on+on_feature_n] =
							(training_sum_sqr[feature_n][feature_m]-
							training_sum[feature_n]*training_sum[feature_m]/
							training_samples)/(training_samples-1);
						on_feature_m++;
					}
				on_feature_n++;
			}
			if (function_width[feature_n] < MIN_FUNCTION_WIDTH)
				function_width[feature_n] = MIN_FUNCTION_WIDTH;
		}
		det_covariance = 0;
		if (features_on)
			VComputeDeterminant(&det_covariance, covariance, features_on);
		if (det_covariance > 0)
		{
			error_code=VInvertMatrix(inv_covariance, covariance, features_on);
			if (error_code)
			{
				if (!error_flag)
					m_parent_frame->SetStatusText("Error.", 1); //display_error(error_code);
				error_flag = TRUE;
				det_covariance = 0;
			}
		}
		else if (!error_flag)
		{
			m_parent_frame->SetStatusText("Covariance matrix is singular.", 1);
			error_flag=TRUE;
		}


		//////// apply training to feature level and width  ////////
		for (feature_n=0; feature_n<NUM_FEATURES; feature_n++)
		{
			m_level[feature_n] = (int)function_level[feature_n];
			m_widthLevel[feature_n] = (int)function_width[feature_n];
		}

		Refresh();

	}
	
	function_update();
	if (!error_flag)
	{
		sprintf(msg, "%d samples used.", training_samples>1? training_samples:0 );
		m_parent_frame->SetStatusText(msg, 1);
	}

	FuzzyComp_update();

	return 1;
}

void FuzzCompCanvas::display_image(int which, unsigned short* pData)
{
}
void FuzzCompCanvas::FuzzyComp_update()
{
	bool feature_is_on = false;
	for (int feature_n=0; feature_n<NUM_FEATURES; feature_n++)
	{
		if (feature_status[feature_n])
		{
			feature_is_on = true;
			break;
		}
	}

	if ( !feature_is_on )
	{
		reload();
		return;
	}

	if (feature_status[feature_selected] && !feature_data_valid[feature_selected])
		compute_feature_image(feature_selected);

	m_bCostImgDone = true;
	for (int j=0; j<NUM_FEATURES; j++)
		if (feature_status[j] && !feature_data_valid[j])
		{	
			compute_feature_image(j);
			if (!feature_data_valid[j])
			{
				m_bCostImgDone = false;
				affinity_data_valid = false;
				connectivity_data_valid = false;
				break;
			}
		}

	if (m_bCostImgDone && !affinity_data_valid)
		compute_affinity_image();

	if (affinity_data_valid && !m_training && IsSeed() &&
			!connectivity_data_valid)
	{
		for (int j=0; j<nSeed; j++)
			if (mData[j][2] == m_sliceNo)
			{
				compute_connectivity_image();
				break;
			}
	}

	reload();
	
}


/*****************************************************************************
 * FUNCTION: compute_feature_image
 * DESCRIPTION: Computes the indicated feature values for the current slice and
 *    stores them at feature_data_across[feature] and
 *    feature_data_down[feature].
 * PARAMETERS:
 *    feature:
 *       0: high
 *       1: low
 *       2: difference
 *       3: sum, or average if sum may be too large
 *       4: relative difference (scaled to MAX_REL_DIFF)
 * SIDE EFFECTS: An error message may be displayed.
 * ENTRY CONDITIONS: The variables vh, slice_data, argv0, windows_open
 *    must be properly set.
 *    The input file must not change from one call to another.
 *    If feature_data_across or feature_data_down is non-null, sufficient space
 *    must be allocated there.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/29/96 by Dewey Odhner
 *
 *****************************************************************************/
void FuzzCompCanvas::compute_feature_image(int feature)	
{
	int row, column, assign_feature;
	unsigned high, low;
	unsigned short *in_ptr_16, *out_ptr;
	unsigned char *in_ptr_8;

	slice_data = (void*)mCavassData->getSlice( m_sliceNo );

	if (feature_data_across[feature] == NULL)
	{
		feature_data_across[feature] = (unsigned short *)malloc(
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]*sizeof(short));
		if (feature_data_across[feature] == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);//display_error(1);
			return;
		}
	}
	if (feature_data_down[feature] == NULL)
	{
		feature_data_down[feature] = (unsigned short *)malloc(
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]*sizeof(short));
		if (feature_data_down[feature] == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);//display_error(1);
			return;
		}
	}
	assign_feature = feature;
	if (feature==3 && largest_density_value>32767)
		assign_feature = NUM_FEATURES;
	if (mCavassData->m_vh.scn.num_of_bits == 16)
	{
		for (row=0; row<mCavassData->m_vh.scn.xysize[1]; row++)
		{
			in_ptr_16 = slice_data_16+row*mCavassData->m_vh.scn.xysize[0];
			out_ptr=feature_data_across[feature]+row*mCavassData->m_vh.scn.xysize[0];
			for (column=0; column<mCavassData->m_vh.scn.xysize[0]-1; column++)
			{
				if (in_ptr_16[0] > in_ptr_16[1])
				{
					high = in_ptr_16[0];
					low = in_ptr_16[1];
				}
				else
				{
					high = in_ptr_16[1];
					low = in_ptr_16[0];
				}
				Assign_feature(*out_ptr, high, low, assign_feature);
				in_ptr_16++;
				out_ptr++;
			}
			*out_ptr = 0;
		}
		for (column=0; column<mCavassData->m_vh.scn.xysize[0]; column++)
		{
			in_ptr_16 = slice_data_16+column;
			out_ptr=feature_data_down[feature]+column;
			for (row=0; row<mCavassData->m_vh.scn.xysize[1]-1; row++)
			{
				if (in_ptr_16[0] > in_ptr_16[mCavassData->m_vh.scn.xysize[0]])
				{
					high = in_ptr_16[0];
					low = in_ptr_16[mCavassData->m_vh.scn.xysize[0]];
				}
				else
				{
					high = in_ptr_16[mCavassData->m_vh.scn.xysize[0]];
					low = in_ptr_16[0];
				}
				Assign_feature(*out_ptr, high, low, assign_feature);
				in_ptr_16 += mCavassData->m_vh.scn.xysize[0];
				out_ptr += mCavassData->m_vh.scn.xysize[0];
			}
			*out_ptr = 0;
		}
	}
	else
	{
		for (row=0; row<mCavassData->m_vh.scn.xysize[1]; row++)
		{
			in_ptr_8 = slice_data_8+row*mCavassData->m_vh.scn.xysize[0];
			out_ptr=feature_data_across[feature]+row*mCavassData->m_vh.scn.xysize[0];
			for (column=0; column<mCavassData->m_vh.scn.xysize[0]-1; column++)
			{
				if (in_ptr_8[0] > in_ptr_8[1])
				{
					high = in_ptr_8[0];
					low = in_ptr_8[1];
				}
				else
				{
					high = in_ptr_8[1];
					low = in_ptr_8[0];
				}
				Assign_feature(*out_ptr, high, low, assign_feature);
				in_ptr_8++;
				out_ptr++;
			}
			*out_ptr = 0;
		}
		for (column=0; column<mCavassData->m_vh.scn.xysize[0]; column++)
		{
			in_ptr_8 = slice_data_8+column;
			out_ptr=feature_data_down[feature]+column;
			for (row=0; row<mCavassData->m_vh.scn.xysize[1]-1; row++)
			{
				if (in_ptr_8[0] > in_ptr_8[mCavassData->m_vh.scn.xysize[0]])
				{
					high = in_ptr_8[0];
					low = in_ptr_8[mCavassData->m_vh.scn.xysize[0]];
				}
				else
				{
					high = in_ptr_8[mCavassData->m_vh.scn.xysize[0]];
					low = in_ptr_8[0];
				}
				Assign_feature(*out_ptr, high, low, assign_feature);
				in_ptr_8 += mCavassData->m_vh.scn.xysize[0];
				out_ptr += mCavassData->m_vh.scn.xysize[0];
			}
			*out_ptr = 0;
		}
	}
	feature_data_valid[feature] = TRUE;
}


/*****************************************************************************
 * FUNCTION: compute_affinity_image
 * DESCRIPTION: Computes the affinity values for the current slice and
 *    stores them at affinity_data_across and affinity_data_down.
 * PARAMETERS: None
 * SIDE EFFECTS: The variable affinity_data_valid set on success.  The variable
 *    feature_status may be changed.  An error message may be displayed.
 * ENTRY CONDITIONS: The variables vh, feature_data_valid, feature_status,
 *    argv0, windows_open, affinity_type, histogram_bins,
 *    histogram_counts, reverse_histogram_counts, largest_density_value,
 *    det_covariance, det_reverse_covariance, training_mean, inv_covariance,
 *    reverse_training_mean, inv_reverse_covariance must be properly set.
 *    The input file must not change from one call to another.
 *    If affinity_data_across or affinity_data_down is non-null, sufficient
 *    space must be allocated there.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Fails if histogram_counts is NULL.
 * HISTORY:
 *    Created: 3/7/96 by Dewey Odhner
 *    Modified: 4/2/96 threshold applied by Dewey Odhner
 *    Modified: 5/22/96 histogram used by Dewey Odhner
 *    Modified: 5/28/96 reverse training allowed by Dewey Odhner
 *    Modified: 7/25/96 covariance affinity type allowed by Dewey Odhner
 *    Modified: 2/18/97 threshold applied covariance affinity type
 *       by Dewey Odhner
 *    Modified: 4/16/97 threshold not applied by Dewey Odhner
 *
 *****************************************************************************/
void FuzzCompCanvas::compute_affinity_image()
{
	int row,column, feature_n, features_on, on_function_selected[NUM_FEATURES];
	unsigned short *in_ptr_across[NUM_FEATURES], *in_ptr_down[NUM_FEATURES],
		*out_ptr_down, *out_ptr_across;
	float on_function_level[NUM_FEATURES], on_function_width[NUM_FEATURES],
		on_weight[NUM_FEATURES], temp_affinity_across, temp_affinity_down;
	double on_training_mean[NUM_FEATURES];		

	affinity_data_valid = FALSE;
	if (affinity_data_across == NULL)
	{
		affinity_data_across = (unsigned short *)malloc( mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]*sizeof(short));
		if (affinity_data_across == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_down == NULL)
	{
		affinity_data_down = (unsigned short *)malloc(mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]*sizeof(short));
		if (affinity_data_down == NULL)
		{
		    m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}

	features_on = 0;
	for (feature_n=0; feature_n<NUM_FEATURES; feature_n++)
		if (feature_status[feature_n])
		{
			if (!feature_data_valid[feature_n])
				compute_feature_image(feature_n);
			if (!feature_data_valid[feature_n])
				continue;
			in_ptr_across[features_on] = feature_data_across[feature_n];
			in_ptr_down[features_on] = feature_data_down[feature_n];
			switch (feature_n)
			{
				case 3:
					if (largest_density_value > 32767)
					{
						on_function_level[features_on] = function_level[3]*.5;
						on_function_width[features_on] = function_width[3]*.5;
						break;
					}
				default:
					on_function_level[features_on] = function_level[feature_n];
					on_function_width[features_on] = function_width[feature_n];
					break;
				case 4:
					on_function_level[features_on] = function_level[feature_n]*
						MAX_REL_DIFF/function_domain[feature_n][1];
					on_function_width[features_on] = function_width[feature_n]*
						MAX_REL_DIFF/function_domain[feature_n][1];
					break;
			}
			on_function_selected[features_on] = function_selected[feature_n];
			on_weight[features_on] = weight[feature_n]/100;   //here weight is from -100 - 100
			on_training_mean[features_on] = training_mean[feature_n];
			features_on++;
		}

	if (features_on == 0)
		return;
	out_ptr_across = affinity_data_across;
	out_ptr_down = affinity_data_down;
	for (row=0; row<mCavassData->m_vh.scn.xysize[1]; row++)
		for (column=0; column<mCavassData->m_vh.scn.xysize[0]; column++)
		{
			if (affinity_type == 0)
			{
				temp_affinity_across = temp_affinity_down = 1;
				for (feature_n=0; feature_n<features_on; feature_n++)
				{
					temp_affinity_across *= on_weight[feature_n]*
						transform((float)*in_ptr_across[feature_n],
						on_function_selected[feature_n],
						on_function_level[feature_n],
						on_function_width[feature_n])+1-
						(on_weight[feature_n]>0? on_weight[feature_n]:0);
					in_ptr_across[feature_n]++;
					temp_affinity_down *= on_weight[feature_n]*
						transform((float)*in_ptr_down[feature_n],
						on_function_selected[feature_n],
						on_function_level[feature_n],
						on_function_width[feature_n])+1-
						(on_weight[feature_n]>0? on_weight[feature_n]:0);
					in_ptr_down[feature_n]++;
				}
				*out_ptr_across =
				    (unsigned short)(temp_affinity_across*MAX_AFFINITY);
				*out_ptr_down =
				    (unsigned short)(temp_affinity_down*MAX_AFFINITY);
			}			
			
			out_ptr_across++;
			out_ptr_down++;
		}
	affinity_data_valid = TRUE;
}

/*****************************************************************************
 * FUNCTION: transform
 * DESCRIPTION: Returns the transform function value of x.
 * PARAMETERS:
 *    x: the feature value to transform
 *    type:
 *       0: gaussian
 *       1: ramp function
 *       2: box function
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The transform function value of x.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 3/5/96 by Dewey Odhner
 *
 *****************************************************************************/
float FuzzCompCanvas::transform(float x, int type, float level, float width)
{
	float xrel;

	xrel = (x-level)/width;
	switch (type)
	{
		case 0: /* gaussian */
			return exp(-2*xrel*xrel);
		case 1: /* ramp */
			xrel += .5;
			if (xrel < 0)
				return 0;
			if (xrel > 1)
				return 1;
			return xrel;
		case 2: /* box */
			if (xrel < -.5 || xrel > .5)
				return 0;
			return 1;
		default:
			assert(0);
			return -1;
	}
}


#define CHUNK_SIZE 1024
#define HEHS (2*sizeof(long)) /* heap element header size */
#define HQElem(s) (*((HheapElem*)(H->q + (s)*H->elem_size)))

/*****************************************************************************
 * FUNCTION: hheap_create
 * DESCRIPTION: Allocates and initializes a hashed heap structure.
 * PARAMETERS:
 *    hashsize: Number of hash bins
 *    itemsize: Size in bytes of items to be stored in the heap
 *    hf: Hash function; called as hf(hashsize, v) where v is a pointer to an
 *       item; must return a value between 0 and hashsize - 1.
 *    valcmp: returns relative ordering of two items by value of item;
 *       highest value item is popped first.  valcmp(v1, v2) returns positive
 *       if v1 has a greater value than v2, negative if less, 0 if same.
 *    idcmp: returns relative ordering of two items by identity (not value).
 *       Must return positive if only the second pointer is NULL.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Pointer to the heap structure.
 * EXIT CONDITIONS: Returns NULL on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns NULL on failure by Dewey Odhner
 *    Modified: 1/26/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
Hheap* FuzzCompCanvas::hheap_create(long hashsize, int itemsize )
{
  Hheap *H;
  long s;

  H = (Hheap*)malloc(sizeof(Hheap));
  if (H == NULL)
    return NULL;
  H->h = (long*)malloc(CHUNK_SIZE * sizeof(long));
  if (H->h == NULL)
  {
    free(H);
	return NULL;
  }
  H->hh = (long*)malloc(hashsize * sizeof(long));
  if (H->hh == NULL)
  {
	free(H->h);
    free(H);
	return NULL;
  }
  for (s = 0L; s < hashsize; s++)
    H->hh[s] = 0L;
  H->hashsize = hashsize;
  H->itemsize = itemsize;
  H->elem_size = HEHS+itemsize;
  while (H->elem_size % (sizeof(HheapElem)-HEHS))
    H->elem_size++;
  //H->hf = hf;
  //H->valcmp = valcmp;
  //H->idcmp = idcmp;
  H->q = (char*)malloc(CHUNK_SIZE * H->elem_size);
  if (H->q == NULL)
  {
	free(H->h);
	free(H->hh);
    free(H);
	return NULL;
  }
  for (s = 0L; s < CHUNK_SIZE - 1; s++)
    HQElem(s).l = s + 1;
  HQElem(s).l = 0L;
  H->allocsize = CHUNK_SIZE;
  H->size = 0L;
  return H;
}

/*****************************************************************************
 * FUNCTION: hheap_destroy
 * DESCRIPTION: Frees a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to allocated memory, and H->hh, H->h, H->q must be
 *       allocated or NULL.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
int  FuzzCompCanvas::hheap_destroy(  Hheap *H )
{
  if (H->hh)
    free(H->hh);
  if (H->h)
    free(H->h);
  if (H->q)
    free(H->q);
  free(H);
  return 1;
}

/*****************************************************************************
 * FUNCTION: hheap_isempty
 * DESCRIPTION: Checks whether the heap is empty.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if the heap is empty.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
int FuzzCompCanvas::hheap_isempty( Hheap *H )
{
  return H->size == 0L;
}

/*****************************************************************************
 * FUNCTION: hheap_enlarge
 * DESCRIPTION: Enlarges the capacity of a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns 1 on failure by Dewey Odhner
 *    Modified: 1/20/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::hheap_enlarge( Hheap *H )
{
  long s, *h;
  char *q;

  h = (long*)realloc(H->h, (H->allocsize + CHUNK_SIZE) * sizeof(long));
  if (h)
    H->h = h;
  q = (char*)realloc(H->q, (H->allocsize + CHUNK_SIZE) * H->elem_size);
  if (q)
    H->q = q;
  if (h==NULL || q==NULL)
    return 1;
  for (s = H->allocsize; s < H->allocsize + CHUNK_SIZE - 1; s++)
    HQElem(s).l = s + 1;
  HQElem(s).l = HQElem(0).l;
  HQElem(0).l = H->allocsize;
  H->allocsize += CHUNK_SIZE;
  return 0;
}

/*****************************************************************************
 * FUNCTION: hheap_push
 * DESCRIPTION: Stores an item in a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 *    v: Pointer to the item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns 1 on failure by Dewey Odhner
 *    Modified: 1/26/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::hheap_push( Hheap *H,  PointWithValue *v )
{
  PointWithValue *vv;
  long hr, t, tt, n, p, s;

  hr = hheap_hash_xy(H->hashsize, v);
  vv = NULL;
  for (t = H->hh[hr], tt = 0L; t > 0L; tt = t, t = HQElem(t).l)
  {
    vv = HQElem(t).c;
    if (point_cmp(v, vv) >= 0)
      break;
  }
  if (H->size == H->allocsize - 1 && hheap_enlarge(H))
    return 1;
  n = ++H->size;
  p = n / 2;
  while (n > 1L && point_value_cmp(HQElem(H->h[p]).c, v) < 0)
  {
    H->h[n] = H->h[p];
    HQElem(H->h[p]).i = n;
    n = p;
    p = n / 2;
  }
  s = HQElem(0).l;
  HQElem(0).l = HQElem(s).l;
  memcpy(HQElem(s).c, v, H->itemsize);
  H->h[n] = s;
  HQElem(s).i = n;
  HQElem(s).l = t;
  if (tt > 0L)
    HQElem(tt).l = s;
  else
    H->hh[hr] = s;
  return 0;
}

/*****************************************************************************
 * FUNCTION: hheap_repush
 * DESCRIPTION: Reorders an item in a hashed heap structure according to a
 *    new value.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure where the item is stored.
 *    v: Pointer to the item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/26/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::hheap_repush(Hheap *H, PointWithValue *v)
{
  PointWithValue *vv;
  long hr, t, n, p, s;
 
  hr = hheap_hash_xy(H->hashsize, v);
  vv = NULL;
  for (t = H->hh[hr]; t > 0L; t = HQElem(t).l)
  {
    vv = HQElem(t).c;
    if (point_cmp(v, vv) == 0)
      break;
  }
  n = HQElem(t).i;
  s = H->h[n];
  p = n / 2;
  while (n > 1L && point_value_cmp(HQElem(H->h[p]).c, v) < 0)
  {
    H->h[n] = H->h[p];
    HQElem(H->h[p]).i = n;
    n = p;
    p = n / 2;
  }
  H->h[n] = s;
  memcpy(HQElem(s).c, v, H->itemsize);
  HQElem(s).i = n;
  return 1;
}

/*****************************************************************************
 * FUNCTION: hheap_pop
 * DESCRIPTION: Retrieves an item from a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure where at least one item is
 *       stored.
 *    v: Stores the item at this address.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/27/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::hheap_pop(  Hheap *H, PointWithValue *v )
{
  long hr, t, tt, n, l, r, m, s;
  PointWithValue *w, *wm;
 
  s = H->h[1];
  memcpy(v, HQElem(s).c, H->itemsize);
  hr = hheap_hash_xy(H->hashsize, v);
  for (t = H->hh[hr], tt = 0L; t > 0L; tt = t, t = HQElem(t).l)
  {
    if (HQElem(t).i == 1)
      break;
  }
  if (tt > 0L)
    HQElem(tt).l = HQElem(t).l;
  else
    H->hh[hr] = HQElem(t).l;
  HQElem(s).l = HQElem(0).l;
  HQElem(0).l = s;
  s = H->size--;
  if (s > 2L)
  {
    w = HQElem(H->h[s]).c;
    m = 1L;
    n = 0L;
    while (m != n)
    {
      n = m;
      l = n + n;
      r = l + 1;
      if (l <= H->size && point_value_cmp(HQElem(H->h[l]).c, w) > 0)
      {
        m = l;
        wm = HQElem(H->h[l]).c;
      }
      else
      {
        m = n;
        wm = w;
      }
      if (r <= H->size && point_value_cmp(HQElem(H->h[r]).c, wm) > 0)
        m = r;
      if (m != n)
      {
        H->h[n] = H->h[m];
        HQElem(H->h[m]).i = n;
      }
      else
      {
        H->h[n] = H->h[s];
        HQElem(H->h[s]).i = n;
      }
    }
  }
  else if (s > 1L)
  {
    H->h[1] = H->h[s];
    HQElem(H->h[s]).i = 1;
  }

  return 1;
}

/*****************************************************************************
 * FUNCTION: hheap_hash_0
 * DESCRIPTION: Returns a hash value for a pixel.
 * PARAMETERS:
 *    hashsize: The number of hash bins
 *    v: The pixel
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable dimensions must be set.
 * RETURN VALUE: Hash value for the pixel.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/25/99 by Dewey Odhner
 *
 *****************************************************************************/
long FuzzCompCanvas::hheap_hash_xy(  long hashsize, PointWithValue *v )
{
  return (v->y * dimensions.xdim + v->x) % hashsize;
}

/*****************************************************************************
 * FUNCTION: point_value_cmp
 * DESCRIPTION: Returns relative ordering of two pixels by value
 * PARAMETERS:
 *    v, vv: Pointers to the pixels.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: -1, 0, or 1
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/27/99 by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::point_value_cmp(PointWithValue *v, PointWithValue *vv)	
{
	return v->val>vv->val? 1: v->val<vv->val? -1: 0;
}

/*****************************************************************************
 * FUNCTION: point_cmp
 * DESCRIPTION: Returns relative ordering of two pixels in a slice
 * PARAMETERS:
 *    v, vv: Pointers to the pixels.  A null pointer is smaller than any pixel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: -1, 0, or 1
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/25/99 by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::point_cmp(PointWithValue *v, PointWithValue *vv)	
{
	if (v==NULL && vv==NULL)
		return 0;
	if (v == NULL)
		return -1;
	if (vv == NULL)
		return 1;
	if (v->x > vv->x)
		return 1;
	if (v->x < vv->x)
		return -1;
	if (v->y > vv->y)
		return 1;
	if (v->y < vv->y)
		return -1;
	return 0;
}

/*****************************************************************************
 * FUNCTION: push_xy_hheap
 * DESCRIPTION: Stores a pixel in a hashed heap structure.
 * PARAMETERS:
 *    x, y: Coordinates of the pixel.
 *    w: Value of the pixel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be set.  H must be of type Hheap *.
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1/25/99 by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::push_xy_hheap(int x, int y, conn_t w) 
{
  PointWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.val = w;
  return hheap_push(H, &tmp);
}

/*****************************************************************************
 * FUNCTION: repush_xy_hheap
 * DESCRIPTION: Updates a pixel in a hashed heap structure.
 * PARAMETERS:
 *    x, y: Coordinates of the pixel.
 *    w: New value of the pixel.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variable H must be set.  H must be of type Hheap *.
 * RETURN VALUE: Voxel index into the slice
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/25/99 by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::repush_xy_hheap(int x, int y, conn_t w)
{
  PointWithValue tmp;

  tmp.x = x;
  tmp.y = y;
  tmp.val = w;
  hheap_repush(H, &tmp);

  return 1;
}

/*****************************************************************************
 * FUNCTION: pop_xy_hheap
 * DESCRIPTION: Retrieves a pixel from a hashed heap structure.
 * PARAMETERS:
 *    x, y: Coordinates of the popped pixel go here.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The variables H, dimensions must be set.  H must be of
 *    type Hheap *.
 * RETURN VALUE: Voxel index into the slice
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/25/99 by Dewey Odhner
 *
 *****************************************************************************/
long FuzzCompCanvas::pop_xy_hheap(int *x, int *y)  
{
  PointWithValue tmp;
  long kkk;

  hheap_pop(H, &tmp);
  *x = tmp.x;
  *y = tmp.y;
  kkk = tmp.y * dimensions.xdim + tmp.x;
  return kkk;
}

/*****************************************************************************
 * FUNCTION: compute_connectivity_image
 * DESCRIPTION: Computes the connectivity values for the current slice and
 *    stores them at connectivity_data.
 * PARAMETERS: None
 * SIDE EFFECTS: The variable connectivity_data_valid will be set.
 *    An error message may be displayed.
 * ENTRY CONDITIONS: The variables vh, affinity_data_across,
 *    affinity_data_down, point_picked, num_points_picked, current_slice,
 *    current_volume, argv0, windows_open, and_op must be properly set.
 *    The input file must not change from one call to another.
 *    If affinity_data_across or affinity_data_down is non-null, sufficient
 *    space must be allocated there.
 *    A successful call to VCreateColormap must be made first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: 
 * HISTORY:
 *    Created: 3/18/96 adapted from Supun Samarasekra by Dewey Odhner
 *    Modified: 4/17/97 alternate definition of fuzzy AND used by Dewey Odhner
 *    Modified: 1/26/99 hashed heap used instead of queue by Dewey Odhner
 *
 *****************************************************************************/
void FuzzCompCanvas::compute_connectivity_image()
{

    int j;
    int ei, x, y;
    wxPoint cur;
    unsigned short *affp[4];
    conn_t pmin, Pmax;
	conn_t afn;

    if (connectivity_data == NULL)
    	connectivity_data = (conn_t *)malloc(mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]*sizeof(conn_t));
    if (connectivity_data == NULL)
    {
    	m_parent_frame->SetStatusText("Memory alloc Error.", 1);//display_error(1);
		return;
    }
	
	H = hheap_create(8999L, sizeof(PointWithValue) );
	if (H == NULL)
	{
	    m_parent_frame->SetStatusText("Heap alloc Error.", 1);
		return;
	}
    dimensions.xdim = mCavassData->m_vh.scn.xysize[0];
    dimensions.ydim = mCavassData->m_vh.scn.xysize[1];
    dimensions.zdim = mCavassData->m_zSize;
    dimensions.slice_size = dimensions.xdim*dimensions.ydim;
    memset(connectivity_data, 0, dimensions.xdim*dimensions.ydim*sizeof(conn_t));	
	
	affp[0] = affinity_data_across;
	affp[1] = affinity_data_down;
	affp[2] = affinity_data_across-1;
	affp[3] = affinity_data_down-dimensions.xdim;
    

    for (j=0; j<nSeed; j++)
	{
		if (mData[j][2] == m_sliceNo ) //slice_index(current_volume, current_slice))
		{
	    	cur.x = mData[j][0];
	    	cur.y = mData[j][1];
	    	connectivity_data[cur.y*dimensions.xdim+cur.x]=MAX_CONNECTIVITY;
	    	if (push_xy_hheap(cur.x, cur.y, (conn_t)MAX_CONNECTIVITY))
			{
				m_parent_frame->SetStatusText("Heap alloc Error.", 1);
				return;
			}
    	}
	}

	while (!hheap_isempty(H)) 
	{
            Pmax = connectivity_data[pop_xy_hheap(&cur.x, &cur.y)];
            for (ei = 0; ei < 4; ei++)
            {
              x = cur.x + nbor[ei].x;
              y = cur.y + nbor[ei].y;
              if (x >= 0 && x < dimensions.xdim &&
			      y >= 0 && y < dimensions.ydim)
              {
                j = cur.y*dimensions.xdim + cur.x;
                afn = ConF(affp[ei][j]);
                pmin = FuzzyAnd(Pmax, afn);
                j = y * dimensions.xdim + x;
                if (pmin > connectivity_data[j])
                {
                  if (connectivity_data[j] == 0)
                  {
				    if (push_xy_hheap(x, y, pmin))
					{
						m_parent_frame->SetStatusText("Heap alloc Error.", 1);
						return;
					}
                  }
				  else
                    repush_xy_hheap(x, y, pmin);
                  connectivity_data[j] = pmin;
                }
              }
            }
    }
	hheap_destroy(H);
	computed_threshold = threshold;
    connectivity_data_valid = TRUE;
}


/*****************************************************************************
 * FUNCTION: function_update
 * DESCRIPTION: Clears the affinity/connectivity display area and the flags
 *    image_displayed[2], affinity_data_valid, connectivity_data_valid,
 *    masked_original_valid
 * PARAMETERS: None
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A successful call to VCreateColormap must be made first.
 *    The variables image_displayed, image_layout, affinity_type,
 *    image_window must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 3/1/96 by Dewey Odhner
 *    Modified: 4/16/96 masked_original_valid cleared by Dewey Odhner
 *    Modified: 6/4/96 affinity displayed in middle image in histogram mode
 *       by Dewey Odhner
 *
 *****************************************************************************/
void FuzzCompCanvas::function_update()
{
	affinity_data_valid = FALSE;
	connectivity_data_valid = FALSE;
	masked_original_valid = FALSE;
}


void FuzzCompCanvas::accumulate_training()
{
	int row, column, feature_n, old_training_samples, *max_t_samples,
		*t_image_flag, feature_m, assign_feature[NUM_FEATURES];
	unsigned short *in_ptr_16, *ta, **t_sample;
	unsigned char *t_ptr, *t_image, *in_ptr_8;
	int *t_samples;
	unsigned high, low, feature_val[NUM_FEATURES];
	
	t_image = training_image;
	max_t_samples = &max_training_samples;
	t_image_flag = &training_image_flag;
	t_sample = &training_sample;
	t_samples = &training_samples;

	slice_data = (void*)mCavassData->getSlice( m_sliceNo );
		
	if (!*t_image_flag)
		return;

	old_training_samples = *t_samples;
	t_ptr = t_image;
	for (row=0; row<mOverallYSize; row++)
	{
		for (column=0; column < mOverallXSize; column++,t_ptr++)
		{
			if (*t_ptr)
			{
				if (column < mOverallXSize-1)
					(*t_samples)++;
				if (row < mOverallYSize-1)
					(*t_samples)++;
			}
		}
	}
	
	if (*t_samples*2 > *max_t_samples)
	{
		ta = (unsigned short *)(*t_sample?
			realloc(*t_sample, *t_samples*2*sizeof(short)):
			malloc(*t_samples*2*sizeof(short)));
		if (ta)
		{
			*t_sample = ta;
			*max_t_samples = *t_samples*2;
		}
		else
		{
			*t_samples = old_training_samples;
			return;
		}
	}

	for (feature_n=0; feature_n<NUM_FEATURES; feature_n++)
		assign_feature[feature_n] = feature_n;
	if (largest_density_value>32767)
		assign_feature[3] = NUM_FEATURES;
	ta = *t_sample+old_training_samples*2;
	t_ptr = t_image;
	for (row=0; row<mOverallYSize; row++)
	{
	    in_ptr_16 = slice_data_16+row*mOverallXSize;
		in_ptr_8 =  slice_data_8 +row*mOverallXSize;
		for (column=0; column<mOverallXSize;
		        column++,t_ptr++,in_ptr_8++,in_ptr_16++)
			if (*t_ptr)
			{
			  //if (affinity_type != 3)
			  {
				if (column < mOverallXSize-1)
				{
					if (mCavassData->m_vh.scn.num_of_bits == 16)
						if (in_ptr_16[0] > in_ptr_16[1])
						{
							high = in_ptr_16[0];
							low = in_ptr_16[1];
						}
						else
						{
							high = in_ptr_16[1];
							low = in_ptr_16[0];
						}
					else
						if (in_ptr_8[0] > in_ptr_8[1])
						{
							high = in_ptr_8[0];
							low = in_ptr_8[1];
						}
						else
						{
							high = in_ptr_8[1];
							low = in_ptr_8[0];
						}
					for (feature_n=0; feature_n<NUM_FEATURES; feature_n++)
					{
						Assign_feature(feature_val[feature_n], high, low,
							assign_feature[feature_n]);
						//if (training_mode == 1)
						{
							training_sum[feature_n] += feature_val[feature_n];
							if (feature_val[feature_n] < training_min[feature_n])
								training_min[feature_n]=feature_val[feature_n];
							if (feature_val[feature_n] > training_max[feature_n])
								training_max[feature_n]=feature_val[feature_n];
						}
						for(feature_m=0; feature_m<=feature_n; feature_m++)
							//if (training_mode == 1)
								training_sum_sqr[feature_m][feature_n] =
									training_sum_sqr[feature_n][feature_m] +=
									(double)feature_val[feature_n]*
									feature_val[feature_m];
					}
					ta[0] = feature_val[0];
					ta[1] = feature_val[1];
					ta += 2;
				}
				if (row < mOverallYSize-1)
				{
					if (mCavassData->m_vh.scn.num_of_bits == 16)
						if (in_ptr_16[0] > in_ptr_16[mOverallXSize])
						{
							high = in_ptr_16[0];
							low = in_ptr_16[mOverallXSize];
						}
						else
						{
							high = in_ptr_16[mOverallXSize];
							low = in_ptr_16[0];
						}
					else
						if (in_ptr_8[0] > in_ptr_8[mOverallXSize])
						{
							high = in_ptr_8[0];
							low = in_ptr_8[mOverallXSize];
						}
						else
						{
							high = in_ptr_8[mOverallXSize];
							low = in_ptr_8[0];
						}
					for (feature_n=0; feature_n<NUM_FEATURES; feature_n++)
					{
						Assign_feature(feature_val[feature_n], high, low,
						    assign_feature[feature_n]);
						//if (training_mode == 1)
							training_sum[feature_n] += feature_val[feature_n];
						for (feature_m=0; feature_m<=feature_n; feature_m++)
							//if (training_mode == 1)
								training_sum_sqr[feature_m][feature_n] =
									training_sum_sqr[feature_n][feature_m] +=
									(double)feature_val[feature_n]*
									feature_val[feature_m];
					}
					ta[0] = feature_val[0];
					ta[1] = feature_val[1];
					ta += 2;
				}
			  }
			}
	}
	memset(t_image, FALSE, mOverallXSize*mOverallYSize);
	*t_image_flag = FALSE;
}


 
/*****************************************************************************
 * FUNCTION: VInvertMatrix
 * DESCRIPTION: Computes the inverse of a matrix. (Gauss-Jordan method)
 * PARAMETERS:
 *    Ainv: The result
 *    A: The matrix to be inverted
 *    N: The dimension of the matrix
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: Successful completion
 *    1: Memory allocation failure
 *    402: The matrix is singular.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 6/18/96 by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::VInvertMatrix(double Ainv[], double A[], int N)	
{
	int ii, j, p;
	double *L, m;

	L = (double *)malloc(N*N*sizeof(double));
	if (L == 0)
		return (1);
	memcpy(L, A, N*N*sizeof(double));
	for (ii=0; ii<N; ii++)
		for (j=0; j<N; j++)
			Ainv[ii*N+j] = ii==j;
	for (ii=0; ii<N; ii++)
	{
		for (p=ii; p<N; p++)
			if (L[p*N+ii])
				break;
		if (p == N)
		{
			free(L);
			return (402);
		}
		if (p != ii)
			for (j=0; j<N; j++)
			{
				m = L[ii*N+j];
				L[ii*N+j] = L[p*N+j];
				L[p*N+j] = m;
				m = Ainv[ii*N+j];
				Ainv[ii*N+j] = Ainv[p*N+j];
				Ainv[p*N+j] = m;
			}
		m = 1/L[ii*N+ii];
		L[ii*N+ii] = 1;
		for (j=ii+1; j<N; j++)
			L[ii*N+j] *= m;
		for (j=0; j<N; j++)
			Ainv[ii*N+j] *= m;
		for (j=0; j<N; j++)
			if (j != ii)
			{
				m = L[j*N+ii];
				L[j*N+ii] = 0;
				for (p=ii+1; p<N; p++)
					L[j*N+p] -= m*L[ii*N+p];
				for (p=0; p<N; p++)
					Ainv[j*N+p] -= m*Ainv[ii*N+p];
			}
	}
	free(L);
	return (0);
}

/*****************************************************************************
 * FUNCTION: VComputeDeterminant
 * DESCRIPTION: Computes the determinant of a matrix.
 * PARAMETERS:
 *    det: The result
 *    A: The matrix to find the determinant of
 *    N: The dimension of the matrix
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: Successful completion
 *    1: Memory allocation failure
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 7/2/96 by Dewey Odhner
 *
 *****************************************************************************/
int FuzzCompCanvas::VComputeDeterminant(double *det, double A[], int N)	
{
	int ii, j, k;
	double *sub, sum, subdet;

	if (N == 1)
	{
		*det = A[0];
		return (0);
	}
	if (N == 2)
	{
		*det = A[0]*A[3]-A[1]*A[2];
		return (0);
	}
	sub = (double *)malloc((N-1)*(N-1)*sizeof(double));
	if (sub == 0)
		return (1);
	for (j=0; j<N-1; j++)
		for (k=0; k<N-1; k++)
			sub[j*(N-1)+k] = A[(1+j)*N+1+k];
	sum = 0;
	for (ii=0; ; ii++)
	{
		if (VComputeDeterminant(&subdet, sub, N-1))
		{
			free(sub);
			return (1);
		}
		sum += A[ii]*(ii%2? -subdet: subdet);
		if (ii == N-1)
			break;
		for (j=0; j<N-1; j++)
			sub[j*(N-1)+ii] = A[(1+j)*N+ii];
	}
	*det = sum;
	free(sub);
	return (0);
}

//////////////////////  Fuzzy component algorithm ////////////////////////////




bool FuzzCompCanvas::getOverlay ( void ) const {
    return m_overlay;
}
//----------------------------------------------------------------------
int FuzzCompCanvas::getCenter ( const int which ) const 
{
    if (which==0) 
	{
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_center;
    } 
	else if (which==1) 
	{
		assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
		const CavassData&  cd = *(mCavassData->mNext);
		return cd.m_center;
	} 
	else if (which==2) 
	{
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.m_center;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int FuzzCompCanvas::getMax ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_max;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_max;
        } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL);
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.m_max;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int FuzzCompCanvas::getMin ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_min;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_min;
        } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL);
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.m_min;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
//number of slices in entire data set
int FuzzCompCanvas::getNoSlices ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_zSize;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_zSize;
        } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL);
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.m_zSize;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}

//----------------------------------------------------------------------
double FuzzCompCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int FuzzCompCanvas::getSliceNo ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_sliceNo;
         } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL);
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.m_sliceNo;
    } else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
int FuzzCompCanvas::getWidth ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const CavassData&  cd = *mCavassData;
        return cd.m_width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const CavassData&  cd = *(mCavassData->mNext);
        return cd.m_width;
         } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL);
        const CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.m_width;
    }  else {
        assert( 0 );
    }
    return 0;  //should never get here
}
//----------------------------------------------------------------------
bool FuzzCompCanvas::getInvert ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        return cd.mInvert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        return cd.mInvert;
         } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL && mCavassData->mNext->mNext!=NULL);
        CavassData&  cd = *(mCavassData->mNext->mNext);
        return cd.mInvert;
    }  else {
        assert( 0 );
    }
    return false;  //should never get here
}


//----------------------------------------------------------------------
int FuzzCompCanvas::getWeight ( const int which ) const {
    if (which==0) {        
        return m_weight[0];
    } else if (which==1) {
        return m_weight[1];
    } else if (which==2) {
        return m_weight[2];
    } else if (which==3) {
        return m_weight[3];
        } else if (which==4) {
        return m_weight[4];
        }
    return 1;
}
//----------------------------------------------------------------------
int FuzzCompCanvas::getLevel( const int which ) const {
    if (which==0) {        
        return m_level[0];
    } else if (which==1) {
        return m_level[1];
    } else if (which==2) {
        return m_level[2];
    } else if (which==3) {
        return m_level[3];
    } else if (which==4) {
        return m_level[4];
    } 
    return 0;
}
//----------------------------------------------------------------------
int FuzzCompCanvas::getWidthLevel( const int which ) const {
    if (which==0) {        
        return m_widthLevel[0];
    } else if (which==1) {
        return m_widthLevel[1];
    } else if (which==2) {
        return m_widthLevel[2];
    } else if (which==3) {
        return m_widthLevel[3];
    } else if (which==4) {
        return m_widthLevel[4];
    }
    return 0;
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setWeight ( const int which, const int new_weight ) {
    if (which==0) {        
        m_weight[0]=new_weight;
		weight[0] = (float)new_weight;
    } else if (which==1) {
        m_weight[1]=new_weight;
		weight[1] = (float)new_weight;
    } else if (which==2) {
        m_weight[2]=new_weight;
		weight[2] = (float)new_weight;
    } else if (which==3) {
        m_weight[3]=new_weight;
		weight[3] = (float)new_weight;
    } else if (which==4) {
        m_weight[4]=new_weight;
		weight[4] = (float)new_weight;
    }
	if (feature_status[which])
		function_update();
}
//----------------------------------------------------------------------

void FuzzCompCanvas::setLevel ( const int which, const int level ) {
    if (which==0) {        
        m_level[0]=level;
		function_level[0] = (float)level;
    } else if (which==1) {
        m_level[1]=level;
		function_level[1] = (float)level;
    } else if (which==2) {
        m_level[2]=level;
		function_level[2] = (float)level;
    } else if (which==3) {
        m_level[3]=level;
		function_level[3] = (float)level;
    } else if (which==4) {
        m_level[4]=level;
		function_level[4] = (float)level;
    }
	if (feature_status[which])
		function_update();
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setWidthLevel ( const int which, const int widthLevel ) {
    if (which==0) {        
        m_widthLevel[0]=widthLevel;
		function_width[0] = (float)widthLevel;
    } else if (which==1) {
        m_widthLevel[1]=widthLevel;
		function_width[1] = (float)widthLevel;
    } else if (which==2) {
        m_widthLevel[2]=widthLevel;
		function_width[2] = (float)widthLevel;
    } else if (which==3) {
        m_widthLevel[3]=widthLevel;
		function_width[3] = (float)widthLevel;
    } else if (which==4) {
        m_widthLevel[4]=widthLevel;
		function_width[4] = (float)widthLevel;
    }
	if (feature_status[which])
		function_update();
}
//----------------------------------------------------------------------

void FuzzCompCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------

void FuzzCompCanvas::setTraining ( const bool training ) { 
    m_training = training;
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setPainting( const bool painting ){
     m_painting = painting;
	 if (painting)
	 	m_erasing = false;
}

//----------------------------------------------------------------------
void FuzzCompCanvas::setErasing( const bool erasing ){
     m_erasing = erasing;
	 if (erasing)
	 	m_painting = false;
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setCenter ( const int which, const int center ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_center = center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_center = center;
        } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL &&  mCavassData->mNext->mNext!=NULL);
        CavassData&  cd = *(mCavassData->mNext->mNext);
        cd.m_center = center;   
    } else {
        assert( 0 );
    }
}

//----------------------------------------------------------------------
void FuzzCompCanvas::setInvert ( const int which, const bool invert ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.mInvert = invert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.mInvert = invert;
         } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL &&  mCavassData->mNext->mNext!=NULL);
        CavassData&  cd = *(mCavassData->mNext->mNext);
        cd.mInvert = invert;
    }  else {
        assert( 0 );
    }
}

//-------------------------------------------------------
bool FuzzCompCanvas::IsSeed() const {
        if (nSeed > 0)
            return true;
        else
           return false;                
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setScale   ( const double scale )  {  //aka magnification
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
    if (m_cols<1)  m_cols=1;
    if (m_rows<1)  m_rows=1;
    reload();
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setSliceNo ( const int which, const int sliceNo ) {
	if (m_training)
		accumulate_training();
	function_update();
    m_sliceNo = sliceNo;
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_sliceNo = sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_sliceNo = sliceNo;
         } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL &&  mCavassData->mNext->mNext!=NULL);
        CavassData&  cd = *(mCavassData->mNext->mNext);
        cd.m_sliceNo = sliceNo;
    }  else 
        assert( 0 );
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setWidth ( const int which, const int width ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_width = width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        CavassData&  cd = *(mCavassData->mNext);
        cd.m_width = width;
        } else if (which==2) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL &&  mCavassData->mNext->mNext!=NULL);
        CavassData&  cd = *(mCavassData->mNext->mNext);
        cd.m_width = width;
    } else
        assert( 0 );
}
//----------------------------------------------------------------------
void FuzzCompCanvas::setBrushSize ( const int size ) {
     m_BrushSize = size;
    }
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( FuzzCompCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( FuzzCompCanvas, wxPanel )
    EVT_PAINT(            FuzzCompCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           FuzzCompCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        FuzzCompCanvas::OnLeftDown     )
    EVT_LEFT_UP(          FuzzCompCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      FuzzCompCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        FuzzCompCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       FuzzCompCanvas::OnRightDown    )
    EVT_RIGHT_UP(         FuzzCompCanvas::OnRightUp      )
    EVT_CHAR(             FuzzCompCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
