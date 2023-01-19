/*
  Copyright 1993-2017, 2021 Medical Image Processing Group
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
#include  "cavass.h"
#include  "IRFCCanvas.h"
#undef M_PI
#include "3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/gqueue.h"
#include  "CavassData.h"

using namespace std;

#define MAX_REL_DIFF 65535
#define MAX_AFFINITY 65534
#define MIN_FUNCTION_WIDTH ((float).01)
#define MIN_CONNECTIVITY_WIDTH .001
#define FuzzyAnd(a, b) ((a)<(b)?(a):(b))
#define MAX_CONNECTIVITY 65534
#define ConF(x) (conn_t)(and_op!=2? (x):MAX_CONNECTIVITY*exp((x)*(1./MAX_AFFINITY)-1))

//----------------------------------------------------------------------
const int  IRFCCanvas::sSpacing=0;  //space, in pixels, between each slice
//----------------------------------------------------------------------
IRFCCanvas::IRFCCanvas ( void )  : MainCanvas()  { 
      puts("IRFCCanvas()");
          mCavassData = NULL;
}
//----------------------------------------------------------------------
IRFCCanvas::IRFCCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
{
	mCavassData = NULL;
	m_scale = 1.0;
	m_overlay        = true;
	m_training       = true;
	m_which_seed     = FG_SEED;
	m_painting       = false;
	m_erasing        = false;
	m_algorithm      = 0;
	obj_weight = 50;
	max_objects = 15;
	nObj = 1;
	obj_level = (int *)malloc(max_objects*sizeof(int));
	obj_width = (int *)malloc(max_objects*sizeof(int));
	obj_type = (int *)malloc(max_objects*sizeof(int));
	m_obj = 0;
	for (int j=0; j<max_objects; j++)
	{
		obj_level[j] = 0;
		obj_width[j] = 1;
		obj_type[j] = j? 3:0;
	}
	points_filename  = NULL;
	bg_filename      = NULL;
	model_filename   = NULL;
	pointsData       = NULL;
	bgData           = NULL;
	modelData        = NULL;
	outData          = NULL;
	slice_has_seeds  = -1;
	m_images         = (wxImage**)NULL;
	m_bitmaps        = (wxBitmap**)NULL;
	m_bitmaps1       = (wxBitmap**)NULL;
	m_bitmaps2       = (wxBitmap**)NULL;
	myFrame   = parent_frame;
	m_rows = m_cols = 0;
	m_tx = m_ty = 0;
	m_sliceNo        = 0;
	m_BrushSize    = 5;

    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	m_bIRFCDone = false;

	nFg_seed = nBg_seed = mNoPaintingPts = mNoErasingPts = 0;
        
	H = NULL;
	affinity_type = 3;
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
	feature_selected = 1;
	fuzzy_adjacency_flag = false;

	affinity_data_across = affinity_data_down = NULL;
	affinity_data_back = affinity_data_up = NULL;
	affinity_data_across2 = affinity_data_down2 = NULL;
	affinity_data_back2 = affinity_data_up2 = NULL;
	for( int i=0; i<5; i++ )
	{
		function_selected[i] = 0;
		feature_status[i] = i<3;
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
		function_level[i] = 0;
		function_width[i] = 1;
		training_min[i] = 65535;
		training_max[i] = 0;
		training_sum[i] = 0;

		for( int j=0; j<NUM_FEATURES; j++)
		{
			training_sum_sqr[i][j] = 0;
		}
	}
	weight[2] = (float)(100-obj_weight);
	m_inhomo = 1;
	affinityImg = NULL;
	connectivityImg = NULL;
	slice_buffer_16 = NULL;

	m_bPararell = false;

	m_paintingPts = NULL;
	m_erasingPts = NULL;
	m_brushSizePts = NULL;
}
//----------------------------------------------------------------------
IRFCCanvas::~IRFCCanvas ( void ) 
{
    cout << "IRFCCanvas::~IRFCCanvas" << endl;
    wxLogMessage( "IRFCCanvas::~IRFCCanvas" );

	while (mCavassData!=NULL) {
        CavassData*  tmp = mCavassData;
        mCavassData = mCavassData->mNext;
        delete tmp;
    }

	if (points_filename)
		free(points_filename);
	points_filename = NULL;
	if (bg_filename)
		free(bg_filename);
	bg_filename = NULL;
	if (model_filename)
		free(model_filename);
	model_filename = NULL;

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
void IRFCCanvas::release ( void ) {
}  

void IRFCCanvas::resetScale ( void ) 
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
void IRFCCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void IRFCCanvas::loadPoints ( const char* const fn )
{
	if (m_algorithm == FCAlgm_IIRFC)
	{
		// find points on adjacent slice
		int slice_fg_points[3]={0, 0, 0}, slice_bg_points[3]={0, 0, 0};
		for (int j=0; j<nFg_seed; j++)
			if (fg_seed[j][2]>=m_sliceNo-1 && fg_seed[j][2]<=m_sliceNo+1)
				slice_fg_points[fg_seed[j][2]-(m_sliceNo-1)]++;
		for (int j=0; j<nBg_seed; j++)
			if (bg_seed[j][2]>=m_sliceNo-1 && bg_seed[j][2]<=m_sliceNo+1)
				slice_bg_points[bg_seed[j][2]-(m_sliceNo-1)]++;
		int ss=slice_fg_points[0]||slice_bg_points[0]? 0:2;
		int t_which_seed=m_which_seed;
		if (slice_fg_points[ss])
		{
			m_which_seed = FG_SEED;
			for (int j=0; j<nFg_seed; j++)
				if (fg_seed[j][2]-(m_sliceNo-1) == ss)
					add_seed(fg_seed[j][0], fg_seed[j][1], m_sliceNo);
			m_which_seed = t_which_seed;
			slice_has_seeds |= 1;
			connectivity_data_valid = false;
		}
		if (slice_bg_points[ss])
		{
			m_which_seed = BG_SEED;
			for (int j=0; j<nBg_seed; j++)
				if (bg_seed[j][2]-(m_sliceNo-1) == ss)
					add_seed(bg_seed[j][0], bg_seed[j][1], m_sliceNo);
			m_which_seed = t_which_seed;
			slice_has_seeds |= 2;
			connectivity_data_valid = false;
		}
		IRFC_update();
		return;
	}
	delete pointsData;
	pointsData = new CavassData( fn );
	if (!pointsData->mIsCavassFile || !pointsData->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
	if (points_filename)
		free(points_filename);
	points_filename = (char *)malloc(strlen(fn)+1);
	strcpy(points_filename, fn);
	connectivity_data_valid = false;
	delete outData;
	outData = NULL;
}
//----------------------------------------------------------------------
void IRFCCanvas::loadBgPoints ( const char* const fn )
{
	delete bgData;
	bgData = new CavassData( fn );
	if (!bgData->mIsCavassFile || !bgData->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
	if (bg_filename)
		free(bg_filename);
	bg_filename = (char *)malloc(strlen(fn)+1);
	strcpy(bg_filename, fn);
	connectivity_data_valid = false;
	delete outData;
	outData = NULL;
}
//----------------------------------------------------------------------
void IRFCCanvas::loadModel ( const char* const fn )
{
	delete modelData;
	modelData = new CavassData( fn );
	if (!modelData->mIsCavassFile || !modelData->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
	if (model_filename)
		free(model_filename);
	model_filename = (char *)malloc(strlen(fn)+1);
	strcpy(model_filename, fn);
	function_update();
	delete outData;
	outData = NULL;
}
//----------------------------------------------------------------------
void IRFCCanvas::loadOutData ( const char* const fn )
{
	CavassData *t=new CavassData( fn );
	if (!t->mIsCavassFile || !t->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
	if (t)
		delete outData;
	outData = t;
	connectivity_data_valid = false;
	IRFC_update();
}
//----------------------------------------------------------------------
void IRFCCanvas::loadFile ( const char* const fn ) 
{
	SetCursor( wxCursor(wxCURSOR_WAIT) );    
	if (fn==NULL || strlen(fn)==0) {
		SetCursor( *wxSTANDARD_CURSOR );
		return;
	}
	CavassData*  cd = new CavassData( fn );
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
	mCavassData = cd;


	/* for filtered CavassData, both will be deleted in MainCanvas */

	CavassData*  cd2 = new CavassData( fn );
	CavassData*  cd3 = new CavassData( fn );
	free(cd2->m_lut);
	free(cd3->m_lut);
	cd2->m_lut = (unsigned char*)malloc(2*65536);
	cd3->m_lut = (unsigned char*)malloc(65536);
	cd2->m_max = MAX_AFFINITY;
	cd3->m_max = MAX_AFFINITY;
	cd2->m_min = 0;
	cd3->m_min = 0;
	cd2->m_center = MAX_AFFINITY/2;
	cd3->m_center = MAX_AFFINITY/2;
	cd2->m_width = MAX_AFFINITY;
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
void IRFCCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 || which==2);
    if (!isLoaded(which))    return;
    if (which==0)
        mCavassData->initLUT();
    else  if (which==1)
    {
        mCavassData->mNext->m_max = MAX_AFFINITY;
        mCavassData->mNext->initLUT();
    }
    else  if (which==2) mCavassData->mNext->mNext->initLUT();
}
//----------------------------------------------------------------------
void IRFCCanvas::CreateDisplayImage ( int which ) 
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
		bool do_threshold= !m_training && (slice_has_seeds==3 || outData);
		unsigned short *sData1 = NULL, *sData2 = NULL;
		if( which == 1 )
		{
			sData1 = affinity_data_across;
			sData2 = affinity_data_down;
		}
		else
		{
			if (!m_training && !do_threshold)
			{
				for (int j=0; j<nFg_seed; j++)
					if (fg_seed[j][2] == m_sliceNo)
					{
						do_threshold = true;
						slice_has_seeds = 1;
						break;
					}
				// check pointsData
				if (points_filename)
				{
					unsigned char *points_slice=(unsigned char *)pointsData->
						getSlice(m_sliceNo);
					for (int j=0; !do_threshold&&j<pointsData->m_ySize; j++)
						for (int k=0; !do_threshold&&k<pointsData->m_xSize;k++)
							if (points_slice[j*pointsData->m_xSize+k])
							{
								do_threshold = true;
								slice_has_seeds = 1;
							}
				}
				if (do_threshold)
				{
					do_threshold = false;
					for (int j=0; j<nBg_seed; j++)
						if (bg_seed[j][2] == m_sliceNo)
						{
							do_threshold = true;
							slice_has_seeds = 3;
							break;
						}
					// check bgData
					if (bg_filename)
					{
						unsigned char *points_slice=(unsigned char *)bgData->
							getSlice(m_sliceNo);
						for (int j=0; !do_threshold&&j<bgData->m_ySize; j++)
							for (int k=0; !do_threshold&&k<bgData->m_xSize;k++)
								if (points_slice[j*bgData->m_xSize+k])
								{
									do_threshold = true;
									slice_has_seeds = 3;
								}
					}
				}
			}
			if (do_threshold)
			{
				sData1 = connectivity_data;
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
void IRFCCanvas::reload ( void ) 
{
	 if (!isLoaded(0))   
		 return;
	 freeImagesAndBitmaps();

	 CreateDisplayImage(0);
	 if( affinity_data_valid )
	 {
		 CreateDisplayImage(1);
		 CreateDisplayImage(2);
	 }
    
	 Refresh();
}
//----------------------------------------------------------------------
void IRFCCanvas::mapWindowToData ( int wx, int wy,
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
void IRFCCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "FilterCanvas::OnChar" << endl;
    wxLogMessage( "IRFCCanvas::OnChar" );
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
void IRFCCanvas::OnMouseMove ( wxMouseEvent& e )
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
        if( affinity_data_valid )
        {
           s.Printf( wxT("(%d,%d),(%d,%d)"), x, y, affinity_data_across[y*mCavassData->m_xSize + x],
                    affinity_data_down[y*mCavassData->m_xSize + x] );
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

    if (e.LeftIsDown() && !m_training) {
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
                    {
					    if (add_seed(left+t_x, top+t_y, z))
							return;
                    }
        IRFCFrame* frame = dynamic_cast<IRFCFrame*>(myFrame);
        frame->OnSeed();
        Refresh();
	}
	else if (e.RightIsDown() && !m_training) {
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
    }
	else if (e.MiddleIsDown() && !m_training) {
        int left = (int)rint(x-m_BrushSize*.5);
        int top = (int)rint(y-m_BrushSize*.5);
        if (m_which_seed == FG_SEED)
        {
		    for (int j=0; j<nFg_seed; j++)
                if (fg_seed[j][2]==z &&
                        fg_seed[j][0]>=left && fg_seed[j][0]<left+m_BrushSize&&
                        fg_seed[j][1]>=top && fg_seed[j][1]<top+m_BrushSize)
                {
					--nFg_seed;
                    for (int k=j; k<nFg_seed; k++)
                    {
					    fg_seed[k][0] = fg_seed[k+1][0];
						fg_seed[k][1] = fg_seed[k+1][1];
						fg_seed[k][2] = fg_seed[k+1][2];
                    }
					connectivity_data_valid = false;
					j--;
                }
        }
		else
        {
		    for (int j=0; j<nBg_seed; j++)
                if (bg_seed[j][2]==z &&
                        bg_seed[j][0]>=left && bg_seed[j][0]<left+m_BrushSize&&
                        bg_seed[j][1]>=top && bg_seed[j][1]<top+m_BrushSize)
                {
					--nBg_seed;
                    for (int k=j; k<nBg_seed; k++)
                    {
					    bg_seed[k][0] = bg_seed[k+1][0];
						bg_seed[k][1] = bg_seed[k+1][1];
						bg_seed[k][2] = bg_seed[k+1][2];
                    }
                    connectivity_data_valid = false;
					j--;
                }
        }
		if (!connectivity_data_valid)
		{
        	delete outData;
        	outData = NULL;
        	IRFCFrame* frame = dynamic_cast<IRFCFrame*>(myFrame);
        	frame->OnSeed();
        	Refresh();
		}
    } else {
        if (lastX!=-1 || lastY!=-1) {
            SetCursor( *wxSTANDARD_CURSOR );
            lastX = lastY = -1;
        }
    }

}
//----------------------------------------------------------------------
void IRFCCanvas::ResetSeed ( void ) {

    if (nFg_seed)
	{
		free(fg_seed);
	    nFg_seed = 0;
	}
	delete pointsData;
	pointsData = NULL;
	if (points_filename)
		free(points_filename);
	points_filename = NULL;
	if (nBg_seed)
	{
		free(bg_seed);
		nBg_seed = 0;
	}
	delete bgData;
	bgData = NULL;
	if (bg_filename)
		free(bg_filename);
	bg_filename = NULL;
	slice_has_seeds = 0;
	connectivity_data_valid = false;
	delete outData;
	outData = NULL;
}
//----------------------------------------------------------------------
void IRFCCanvas::ResetTraining ( void ) {

    memset(training_image, 0, mOverallXSize*mOverallYSize);
	training_image_flag = FALSE;
	affinity_data_valid = FALSE;
	connectivity_data_valid = false;
	delete outData;
	outData = NULL;

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
void IRFCCanvas::HandlePaint ( wxDC& mdc ) {
//draw the seed points indicated by the user
        
	if( !m_training )
	{
#if wxCHECK_VERSION(2, 9, 0)
	    mdc.SetBrush( wxBrush(wxColour(0, 255, 255), wxBRUSHSTYLE_SOLID) );
		mdc.SetPen( wxPen(wxColour(0, 255, 255), 0, wxPENSTYLE_TRANSPARENT) );
#else
	    mdc.SetBrush( wxBrush(wxColour(0, 255, 255), wxSOLID) );
		mdc.SetPen( wxPen(wxColour(0, 255, 255), 0, wxTRANSPARENT) );
#endif
		for (int i=0; i<nBg_seed ; i++) {  //points indicated by user
			int  wx, wy;
			mapDataToWindow(bg_seed[i][0],bg_seed[i][1],bg_seed[i][2], wx, wy);
			if (wx>=0 && wy>=0 && wx < mOverallXSize * m_scale && wy < mOverallYSize * m_scale)
				mdc.DrawRectangle( wx-2, wy-2, 5, 5 );
		}
#if wxCHECK_VERSION(2, 9, 0)
	    mdc.SetBrush( wxBrush(wxColour(Yellow), wxBRUSHSTYLE_SOLID) );
#else
	    mdc.SetBrush( wxBrush(wxColour(Yellow), wxSOLID) );
#endif
		for (int i=0; i<nFg_seed ; i++) {  //points indicated by user
			int  wx, wy;
			mapDataToWindow(fg_seed[i][0],fg_seed[i][1],fg_seed[i][2], wx, wy);
			if (wx>=0 && wy>=0 && wx < mOverallXSize * m_scale && wy < mOverallYSize * m_scale)
				mdc.DrawRectangle( wx-2, wy-2, 5, 5 );
		}
	}
}
//----------------------------------------------------------------------
void IRFCCanvas::OnRightDown ( wxMouseEvent& e ) {
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
void IRFCCanvas::OnRightUp ( wxMouseEvent& e ) {
   SetFocus();  //to regain/recapture keypress events
}
//----------------------------------------------------------------------
void IRFCCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
	SetFocus();  //to regain/recapture keypress events
	if (m_training) 
		setErasing(true);
	else
	{
	  wxClientDC  dc(this);
	  PrepareDC(dc);
	  const wxPoint  pos = e.GetPosition();
	  int  x, y, z, (*nearest_seed)[3]=NULL, j;
	  mapWindowToData( dc.DeviceToLogicalX( pos.x ), dc.DeviceToLogicalY( pos.y ), x, y, z );

	  if (m_which_seed == FG_SEED)
	  {
	    for (j=0; j<nFg_seed; j++)
			if (fg_seed[j][2]==z && (nearest_seed==NULL ||
				    (fg_seed[j][0]-x)*(fg_seed[j][0]-x)+
					(fg_seed[j][1]-y)*(fg_seed[j][1]-y)<
					(nearest_seed[0][0]-x)*(nearest_seed[0][0]-x)+
					(nearest_seed[0][1]-y)*(nearest_seed[0][1]-y)))
				nearest_seed = fg_seed+j;
		if (nearest_seed == NULL)
		{
			wxMessageBox("No foreground seed found.");
			return;
		}
		for (--nFg_seed; nearest_seed<fg_seed+nFg_seed; nearest_seed++)
			for (j=0; j<3; j++)
				nearest_seed[0][j] = nearest_seed[1][j];
	  }
	  else
	  {
	    for (j=0; j<nBg_seed; j++)
			if (bg_seed[j][2]==z && (nearest_seed==NULL ||
				    (bg_seed[j][0]-x)*(bg_seed[j][0]-x)+
					(bg_seed[j][1]-y)*(bg_seed[j][1]-y)<
					(nearest_seed[0][0]-x)*(nearest_seed[0][0]-x)+
					(nearest_seed[0][1]-y)*(nearest_seed[0][1]-y)))
				nearest_seed = bg_seed+j;
		if (nearest_seed == NULL)
		{
			wxMessageBox("No background seed found.");
			return;
		}
		for (--nBg_seed; nearest_seed<bg_seed+nBg_seed; nearest_seed++)
			for (j=0; j<3; j++)
				nearest_seed[0][j] = nearest_seed[1][j];
	  }
	  connectivity_data_valid = false;
	  delete outData;
	  outData = NULL;
      IRFCFrame* frame = dynamic_cast<IRFCFrame*>(myFrame);
      frame->OnSeed();
      Refresh();

	}
}
//----------------------------------------------------------------------
void IRFCCanvas::OnMiddleUp ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
        if (m_training) setErasing(false); 
}
//----------------------------------------------------------------------
void IRFCCanvas::OnLeftDown ( wxMouseEvent& e ) {
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
	  if (add_seed(x, y, z))
	    return;
      IRFCFrame* frame = dynamic_cast<IRFCFrame*>(myFrame);
      frame->OnSeed();
      Refresh();
    }
}
//----------------------------------------------------------------------
void IRFCCanvas::OnLeftUp ( wxMouseEvent& e ) {
        SetFocus();  //to regain/recapture keypress events
        if (m_training){
          setPainting(false);
          SetCursor( *wxSTANDARD_CURSOR );
        } 

}
//----------------------------------------------------------------------
bool IRFCCanvas::add_seed( int x, int y, int z )
{
    if (m_which_seed == FG_SEED)
	{
		for (int j=0; j<nFg_seed; j++)
			if (fg_seed[j][0]==x && fg_seed[j][1]==y && fg_seed[j][2]==z)
				return false;
		int (*tmp_seed)[3]= (int (*)[3])(nFg_seed?
			realloc(fg_seed, (nFg_seed+1)*sizeof(*fg_seed)):
			malloc(sizeof(*fg_seed)));
		if (tmp_seed == NULL)
		{
		    wxMessageBox("Too many seed points.");
			return true;
		}
		fg_seed = tmp_seed;
		fg_seed[nFg_seed][0] = x;
		fg_seed[nFg_seed][1] = y;
		fg_seed[nFg_seed][2] = z;
        ++nFg_seed;
	}
	else
	{
		for (int j=0; j<nBg_seed; j++)
			if (bg_seed[j][0]==x && bg_seed[j][1]==y && bg_seed[j][2]==z)
				return false;
		int (*tmp_seed)[3]= (int (*)[3])(nBg_seed?
			realloc(bg_seed, (nBg_seed+1)*sizeof(*bg_seed)):
			malloc(sizeof(*bg_seed)));
		if (tmp_seed == NULL)
		{
		    wxMessageBox("Too many seed points.");
			return true;
		}
		bg_seed = tmp_seed;
		bg_seed[nBg_seed][0] = x;
		bg_seed[nBg_seed][1] = y;
		bg_seed[nBg_seed][2] = z;
        ++nBg_seed;
	}
	connectivity_data_valid = false;
	delete outData;
	outData = NULL;
	return false;
}
//----------------------------------------------------------------------
void IRFCCanvas::OnPaint ( wxPaintEvent& e ) {
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
void IRFCCanvas::paint ( wxDC& dc ) {
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
      wxMemoryDC dcErase;
  
      wxBitmap bitmapPaint(w, h);
      wxBitmap bitmapErase(w, h);
  
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
void IRFCCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool IRFCCanvas::isLoaded ( const int which ) const 
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
        if (mCavassData==NULL || mCavassData->mNext==NULL || mCavassData->mNext->mNext==NULL)    return false;
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

int IRFCCanvas::ApplyTraining()
{
	char msg[48];
	
	largest_density_value = (int)(mCavassData->m_vh.scn.largest_density_value[0] + 0.5);
	 smallest_density_value = (int)(mCavassData->m_vh.scn.smallest_density_value_valid? mCavassData->m_vh.scn.smallest_density_value[0]: 0);

	accumulate_training();
	if (training_samples<=1 )
	{
		m_parent_frame->SetStatusText("Training samples not specified.", 1);
		return -1;
    }

	if (training_samples > 1)
	{

		//////// apply training to feature level and width  ////////
		if (m_obj >= 0)
		{
			obj_level[m_obj] = (int)rint(training_sum[3]*.5/training_samples);
			obj_width[m_obj] = (int)rint(sqrt((training_sum_sqr[0][0]+
				training_sum_sqr[1][1]-
				training_sum[3]*training_sum[3]/(2*training_samples))/
				(2*training_samples-1)));
		}
		else
			m_inhomo= (float)(sqrt(training_sum_sqr[2][2]/training_samples));

		Refresh();

	}

	function_update();
	delete outData;
	outData = NULL;
	sprintf(msg, "%d samples used.", training_samples>1? training_samples:0 );
	m_parent_frame->SetStatusText(msg, 1);

	IRFC_update();

	return 1;
}

void IRFCCanvas::display_image(int which, unsigned short* pData)
{
}
void IRFCCanvas::IRFC_update()
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

	if (!affinity_data_valid)
		compute_affinity_image();

	if (affinity_data_valid && !m_training && (IsSeed()||outData) &&
			!connectivity_data_valid)
	{
		bool seeds_set=slice_has_seeds&1;
		for (int j=0; j<nFg_seed; j++)
			if (fg_seed[j][2] == m_sliceNo)
			{
				seeds_set = true;
				slice_has_seeds = 1;
				break;
			}
		if (!seeds_set && points_filename)
		{
			// check pointsData for seeds
			unsigned char *points_slice=(unsigned char *)pointsData->
				getSlice(m_sliceNo);
			for (int j=0; !seeds_set&&j<pointsData->m_ySize; j++)
				for (int k=0; !seeds_set&&k<pointsData->m_xSize; k++)
					if (points_slice[j*pointsData->m_xSize+k])
					{
						seeds_set = true;
						slice_has_seeds = 1;
					}
		}
		if (seeds_set)
		{
			seeds_set = false;
			for (int j=0; j<nBg_seed; j++)
				if (bg_seed[j][2] == m_sliceNo)
				{
					seeds_set = true;
					slice_has_seeds = 3;
					break;
				}
			if (!seeds_set && bg_filename)
			{
				// check bgData for seeds
				unsigned char *points_slice=(unsigned char *)bgData->
					getSlice(m_sliceNo);
				for (int j=0; !seeds_set&&j<bgData->m_ySize; j++)
					for (int k=0; !seeds_set&&k<bgData->m_xSize; k++)
						if (points_slice[j*bgData->m_xSize+k])
						{
							seeds_set = true;
							slice_has_seeds = 3;
						}
			}
		}
		if (seeds_set || outData)
			compute_connectivity_image();
	}

	reload();
	
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
void IRFCCanvas::compute_affinity_image()
{
	int row,column, feature_n, features_on;
	unsigned short *out_ptr_down, *out_ptr_across, *row_ptr_16,
		*out_ptr_up, *out_ptr_back, *out_ptr_down2, *out_ptr_across2,
		*out_ptr_up2, *out_ptr_back2;
	float on_function_level[NUM_FEATURES], on_function_width[NUM_FEATURES],
		on_weight[NUM_FEATURES], temp_affinity, temp_affinity2, xrel,
		temp_affinity3, temp_affinity4, mod_weight;

	slice_data = (void*)mCavassData->getSlice( m_sliceNo );
	if (slice_buffer_16 == NULL)
		slice_buffer_16 = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
	if (mCavassData->m_vh.scn.num_of_bits == 16)
		memcpy(slice_buffer_16, slice_data, sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
	else
		for (column=0; column<mCavassData->m_vh.scn.xysize[0]*
				mCavassData->m_vh.scn.xysize[1]; column++)
			slice_buffer_16[column] = slice_data_8[column];
	affinity_data_valid = FALSE;
	if (affinity_data_across == NULL)
	{
		affinity_data_across = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_across == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_down == NULL)
	{
		affinity_data_down = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_down == NULL)
		{
		    m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_back == NULL)
	{
		affinity_data_back = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_back == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_up == NULL)
	{
		affinity_data_up = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_up == NULL)
		{
		    m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_across2 == NULL)
	{
		affinity_data_across2 = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_across2 == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_down2 == NULL)
	{
		affinity_data_down2 = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_down2 == NULL)
		{
		    m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_back2 == NULL)
	{
		affinity_data_back2 = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_back2 == NULL)
		{
			m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}
	if (affinity_data_up2 == NULL)
	{
		affinity_data_up2 = (unsigned short *)malloc(sizeof(short)*
			mCavassData->m_vh.scn.xysize[0]*mCavassData->m_vh.scn.xysize[1]);
		if (affinity_data_up2 == NULL)
		{
		    m_parent_frame->SetStatusText("Memory alloc Error.", 1);
			return;
		}
	}

	features_on = 0;
	for (feature_n=0; feature_n<NUM_FEATURES; feature_n++)
		if (feature_status[feature_n])
		{
			on_function_level[features_on] = function_level[feature_n];
			on_function_width[features_on] = function_width[feature_n];
			features_on++;
		}

	if (features_on == 0)
		return;
	unsigned short *mod_ptr;
	if (model_filename)
		mod_ptr = (unsigned short *)modelData->getSlice( m_sliceNo );
	on_weight[1] = (float)(.01*obj_weight);
	on_weight[2] = (float)(.01*weight[2]);
	mod_weight = (float)(.01/65534)*(100-obj_weight-weight[2]);
	out_ptr_across = affinity_data_across;
	out_ptr_down = affinity_data_down;
	out_ptr_up = affinity_data_up;
	out_ptr_back = affinity_data_back;
	out_ptr_across2 = affinity_data_across2;
	out_ptr_down2 = affinity_data_down2;
	out_ptr_up2 = affinity_data_up2;
	out_ptr_back2 = affinity_data_back2;
	for (row=0; row<mCavassData->m_vh.scn.xysize[1]; row++)
	{
		row_ptr_16 = slice_buffer_16+row*mCavassData->m_vh.scn.xysize[0];
		for (column=0; column<mCavassData->m_vh.scn.xysize[0]; column++)
		{
		  if (column == mCavassData->m_vh.scn.xysize[0]-1)
		    *out_ptr_across = *out_ptr_across2 = 0;
		  else
		  {
			xrel = ((float)row_ptr_16[column+1]-row_ptr_16[column])/
				on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column+1]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && column<modelData->m_vh.scn.xysize[0]-1)
			{
				temp_affinity3 = *mod_ptr;
				temp_affinity4 = mod_ptr[1];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_across = (unsigned short)(temp_affinity*MAX_AFFINITY);

			xrel = ((float)row_ptr_16[column+1]-row_ptr_16[column])/
				on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column+1]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && column<modelData->m_vh.scn.xysize[0]-1)
			{
				temp_affinity3 = 65534-*mod_ptr;
				temp_affinity4 = 65534-mod_ptr[1];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_across2 = (unsigned short)(temp_affinity*MAX_AFFINITY);
		  }

		  if (column == 0)
		    *out_ptr_back = *out_ptr_back2 = 0;
		  else
		  {
			xrel = ((float)row_ptr_16[column-1]-row_ptr_16[column])/
				on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column-1]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && column<modelData->m_vh.scn.xysize[0]-1)
			{
				temp_affinity3 = *mod_ptr;
				temp_affinity4 = mod_ptr[-1];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_back = (unsigned short)(temp_affinity*MAX_AFFINITY);

			xrel = ((float)row_ptr_16[column-1]-row_ptr_16[column])/
				on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column-1]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && column<modelData->m_vh.scn.xysize[0]-1)
			{
				temp_affinity3 = 65534-*mod_ptr;
				temp_affinity4 = 65534-mod_ptr[-1];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_back2 = (unsigned short)(temp_affinity*MAX_AFFINITY);
		  }

		  if (row == mCavassData->m_vh.scn.xysize[1]-1)
		    *out_ptr_down = *out_ptr_down2 = 0;
		  else
		  {
			xrel = ((float)row_ptr_16[column+mCavassData->m_vh.scn.xysize[0]]-
				row_ptr_16[column])/on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column+mCavassData->m_vh.scn.xysize[0]]-
					obj_level[j])*(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && row<modelData->m_vh.scn.xysize[1]-1)
			{
				temp_affinity3 = *mod_ptr;
				temp_affinity4 = mod_ptr[modelData->m_vh.scn.xysize[0]];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_down = (unsigned short)(temp_affinity*MAX_AFFINITY);

			xrel = ((float)row_ptr_16[column+mCavassData->m_vh.scn.xysize[0]]-
				row_ptr_16[column])/on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column+mCavassData->m_vh.scn.xysize[0]]-
					obj_level[j])*(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && row<modelData->m_vh.scn.xysize[1]-1)
			{
				temp_affinity3 = 65534-*mod_ptr;
				temp_affinity4 = 65534-mod_ptr[modelData->m_vh.scn.xysize[0]];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_down2 = (unsigned short)(temp_affinity*MAX_AFFINITY);
		  }

		  if (row == 0)
		    *out_ptr_up = *out_ptr_up2 = 0;
		  else
		  {
			xrel = ((float)row_ptr_16[column-mCavassData->m_vh.scn.xysize[0]]-
				row_ptr_16[column])/on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column-mCavassData->m_vh.scn.xysize[0]]-
					obj_level[j])*(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 1:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case -1:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 0:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && row<modelData->m_vh.scn.xysize[1]-1)
			{
				temp_affinity3 = *mod_ptr;
				temp_affinity4 = mod_ptr[-modelData->m_vh.scn.xysize[0]];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_up = (unsigned short)(temp_affinity*MAX_AFFINITY);

			xrel = ((float)row_ptr_16[column-mCavassData->m_vh.scn.xysize[0]]-
				row_ptr_16[column])/on_function_width[2];
			if (xrel < 0)
				xrel = -xrel;
			temp_affinity = (float)exp(-.5*xrel*xrel)*on_weight[2];
			temp_affinity2 = temp_affinity;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column]-obj_level[j])*
					(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity += on_weight[1]*temp_affinity3;
			temp_affinity3 = 0;
			for (int j=0; j<nObj; j++)
			{
				xrel = (row_ptr_16[column-mCavassData->m_vh.scn.xysize[0]]-
					obj_level[j])*(float)(1./obj_width[j]);
				switch (obj_type[j])
				{
					case 4:
						temp_affinity4 = (float)(xrel>0? 1:exp(-.5*xrel*xrel));
						break;
					case 2:
						temp_affinity4 = (float)(xrel<0? 1:exp(-.5*xrel*xrel));
						break;
					case 3:
						temp_affinity4 = (float)exp(-.5*xrel*xrel);
						break;
					default:
						temp_affinity4 = 0;
				}
				if (temp_affinity4 > temp_affinity3)
					temp_affinity3 = temp_affinity4;
			}
			temp_affinity2 += on_weight[1]*temp_affinity3;
			if (model_filename && row<modelData->m_vh.scn.xysize[1]-1)
			{
				temp_affinity3 = 65534-*mod_ptr;
				temp_affinity4 = 65534-mod_ptr[-modelData->m_vh.scn.xysize[0]];
				temp_affinity += mod_weight*temp_affinity3;
				temp_affinity2 += mod_weight*temp_affinity4;
			}
			if (temp_affinity2 < temp_affinity)
				temp_affinity = temp_affinity2;
			*out_ptr_up2 = (unsigned short)(temp_affinity*MAX_AFFINITY);
		  }

          if (model_filename)
		    mod_ptr++;
		  out_ptr_across++;
		  out_ptr_down++;
		  out_ptr_back++;
		  out_ptr_up++;
		  out_ptr_across2++;
		  out_ptr_down2++;
		  out_ptr_back2++;
		  out_ptr_up2++;
		}
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
float IRFCCanvas::transform(float x, int type, float level, float width)
{
	float xrel;

	xrel = (x-level)/width;
	switch (type)
	{
		case 0: /* gaussian */
			return exp(-.5*xrel*xrel);
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


/*****************************************************************************
 * FUNCTION: compute_connectivity_image
 * DESCRIPTION: Computes the connectivity values for the current slice and
 *    stores them at connectivity_data.
 *
 *****************************************************************************/

typedef unsigned short OutCellType;

typedef struct {
  short x, y, z;
} Voxel;


void IRFCCanvas::compute_connectivity_image()
{

    if (connectivity_data == NULL)
    	connectivity_data = (conn_t *)malloc(mCavassData->m_vh.scn.xysize[0]*
			mCavassData->m_vh.scn.xysize[1]*sizeof(conn_t));
    if (connectivity_data == NULL)
    {
    	m_parent_frame->SetStatusText("Memory alloc Error.", 1);
		return;
    }
	if (outData)
	{
		unsigned short *out_ptr=(unsigned short *)outData->getSlice(m_sliceNo);
		if (out_ptr)
		{
			memcpy(connectivity_data, out_ptr, mCavassData->m_vh.scn.xysize[0]*
				mCavassData->m_vh.scn.xysize[1]*sizeof(conn_t));
			computed_threshold = threshold;
			connectivity_data_valid = true;
			return;
		}
	}
	
    dimensions.xdim = mCavassData->m_vh.scn.xysize[0];
    dimensions.ydim = mCavassData->m_vh.scn.xysize[1];
    dimensions.zdim = mCavassData->m_zSize;
    dimensions.slice_size = dimensions.xdim*dimensions.ydim;
    memset(connectivity_data, 0, dimensions.slice_size*sizeof(conn_t));
	float slice_spacing=dimensions.zdim==1? mCavassData->m_vh.scn.xypixsz[0]:
		mCavassData->m_vh.scn.loc_of_subscenes[1]-
		mCavassData->m_vh.scn.loc_of_subscenes[0];

	const int nil=-1;
	GQueue *Q;
	int *hl;
	int *R, *Pr;
	int c, d, j;
	float x_adjacency, y_adjacency;

	x_adjacency = y_adjacency = MAX_CONNECTIVITY;
	if (fuzzy_adjacency_flag)
	{
		if (mCavassData->m_vh.scn.xypixsz[0]<=mCavassData->m_vh.scn.xypixsz[1]
				&& mCavassData->m_vh.scn.xypixsz[0]<=slice_spacing)
		{
			y_adjacency *= mCavassData->m_vh.scn.xypixsz[0]/
				mCavassData->m_vh.scn.xypixsz[1];
		}
		else if (mCavassData->m_vh.scn.xypixsz[1]<=
				mCavassData->m_vh.scn.xypixsz[0] &&
				mCavassData->m_vh.scn.xypixsz[1]<=slice_spacing)
		{
			x_adjacency *= mCavassData->m_vh.scn.xypixsz[1]/
				mCavassData->m_vh.scn.xypixsz[0];
		}
		else
		{
			x_adjacency *= slice_spacing/mCavassData->m_vh.scn.xypixsz[0];
			y_adjacency *= slice_spacing/mCavassData->m_vh.scn.xypixsz[1];
		}
	}
	hl = (int *)malloc(dimensions.slice_size*sizeof(int));
	R = (int *)malloc(dimensions.slice_size*sizeof(int));
	Pr = (int *)malloc(dimensions.slice_size*sizeof(int));
	if (hl==NULL || R==NULL || Pr==NULL)
	{
	    m_parent_frame->SetStatusText("Heap alloc Error.", 1);
		return;
	}
	Q= CreateGQueue(MAX_CONNECTIVITY*2+4, dimensions.slice_size, hl);
	if (Q == NULL)
		exit(-1);
	SetRemovalPolicy(Q, MAXVALUE);

	// step 1.
	for (c=0; c<dimensions.slice_size; c++)
	{
		hl[c] = 0;
		R[c] = c;
		Pr[c] = c;
	}
	for (j=0; j<nFg_seed; j++)
		if (fg_seed[j][2] == m_sliceNo)
		{
			c = fg_seed[j][0]+dimensions.xdim*fg_seed[j][1];
			hl[c] = 2+2*MAX_CONNECTIVITY;
			Pr[c] = nil;
		}
	// load seeds from file
	if (points_filename)
	{
		unsigned char *points_slice=(unsigned char *)pointsData->
			getSlice(m_sliceNo);
		for (int j=c=0; j<pointsData->m_ySize; j++)
			for (int k=0; k<pointsData->m_xSize; k++,c++)
				if (points_slice[c])
				{
					hl[c] = 2+2*MAX_CONNECTIVITY;
					Pr[c] = nil;
				}
	}

	for (j=0; j<nBg_seed; j++)
		if (bg_seed[j][2] == m_sliceNo)
		{
			c = bg_seed[j][0]+dimensions.xdim*bg_seed[j][1];
			hl[c] = 3+2*MAX_CONNECTIVITY;
			Pr[c] = nil;
		}
	// load background seeds from file
	if (bg_filename)
	{
		unsigned char *points_slice=(unsigned char *)bgData->
			getSlice(m_sliceNo);
		for (int j=c=0; j<bgData->m_ySize; j++)
			for (int k=0; k<bgData->m_xSize; k++,c++)
				if (points_slice[c])
				{
					hl[c] = 3+2*MAX_CONNECTIVITY;
					Pr[c] = nil;
				}
	}

	// step 2.
	for (c=0; c<dimensions.slice_size; c++)
		InsertGQueue(&Q, c);
	// step 3.
	while (!EmptyGQueue(Q))
	{
		int cx, cy, cz, neighbor[4], kappa[4], ncount=0;
		int ch, dh, minh;
		// step 4.
		c = RemoveGQueue(Q);
		cz = m_sliceNo;
		cy= c/dimensions.xdim;
		cx = c-dimensions.xdim*cy;
		// step 5.
		if (cx>0 && hl[c-1]>=0)
		{
			neighbor[ncount] = c-1;
			kappa[ncount] =
				hl[c]&1? affinity_data_back2[c]:affinity_data_back[c];
			ncount++;
		}
		if (cx<dimensions.xdim-1 && hl[c+1]>=0)
		{
			neighbor[ncount] = c+1;
			kappa[ncount] = hl[c]&1?
				affinity_data_across2[c]:affinity_data_across[c];
			ncount++;
		}
		if (cy>0 && hl[c-dimensions.xdim]>=0)
		{
			neighbor[ncount] = c-dimensions.xdim;
			kappa[ncount] = hl[c]&1?
				affinity_data_up2[c]:affinity_data_up[c];
			ncount++;
		}
		if (cy<dimensions.ydim-1 && hl[c+dimensions.xdim]>=0)
		{
			neighbor[ncount] = c+dimensions.xdim;
			kappa[ncount] = hl[c]&1?
				affinity_data_down2[c]:affinity_data_down[c];
			ncount++;
		}
		ch = hl[c] & ~1;
		hl[c] -= MAX_CONNECTIVITY*2+4;
		for (j=0; j<ncount; j++)
		{
			d = neighbor[j];
			// step 6.
			dh = hl[d] & ~1;
			minh = ch<kappa[j]*2+2? ch:
			          kappa[j]*2+2;
			if (dh<minh || (dh==minh && (hl[R[d]]&1)==0 && (hl[R[c]]&1)))
			{
				int hld;

				hld = minh+(hl[c]&1);
				// step 8.
				R[d] = R[c]; Pr[d] = c;
				// steps 7, 9 & 10.
				UpdateGQueue(&Q, d, hld);
			// step 11.
			}
		// step 12.
		}
	// step 13.
	}
	// step 14.
	for (c=0; c<dimensions.slice_size; c++)
	{
		hl[c] += MAX_CONNECTIVITY*2+4;
		if ((hl[R[c]]&1) == 0)
			connectivity_data[c] = (OutCellType)(hl[c]/2-1);
		else
			connectivity_data[c] = 0;
	}
	free(hl);
	free(R);
	free(Pr);
	DestroyGQueue(&Q);

	computed_threshold = threshold;
    connectivity_data_valid = true;
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
void IRFCCanvas::function_update()
{
	affinity_data_valid = FALSE;
	connectivity_data_valid = false;
	masked_original_valid = FALSE;
}


void IRFCCanvas::accumulate_training()
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
int IRFCCanvas::VInvertMatrix(double Ainv[], double A[], int N)	
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
int IRFCCanvas::VComputeDeterminant(double *det, double A[], int N)	
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




bool IRFCCanvas::getOverlay ( void ) const {
    return m_overlay;
}
//----------------------------------------------------------------------
int IRFCCanvas::getCenter ( const int which ) const 
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
int IRFCCanvas::getMax ( const int which ) const {
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
int IRFCCanvas::getMin ( const int which ) const {
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
int IRFCCanvas::getNoSlices ( const int which ) const {
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
double IRFCCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int IRFCCanvas::getSliceNo ( const int which ) const {
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
int IRFCCanvas::getWidth ( const int which ) const {
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
bool IRFCCanvas::getInvert ( const int which ) const {
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
int IRFCCanvas::getWeight ( const int which ) const {
    return which==2? (int)weight[2]: obj_weight;
}
//----------------------------------------------------------------------
int IRFCCanvas::getLevel( const int which ) const {
	assert(m_obj >= 0);
    return obj_level[m_obj];
}
//----------------------------------------------------------------------
int IRFCCanvas::getWidthLevel( const int which ) const {
    if (which==2) {
        return (int)rint(m_inhomo);
    }
	assert(m_obj >= 0);
    return obj_width[m_obj];
}
//----------------------------------------------------------------------
int IRFCCanvas::getObjType( void ) const {
	assert(m_obj >= 0);
    return obj_type[m_obj];
}
//----------------------------------------------------------------------
void IRFCCanvas::setObjType( const int newtype ) {
	assert(m_obj >= 0);
	obj_type[m_obj] = newtype;
    function_update();
	delete outData;
	outData = NULL;
	IRFC_update();
}
//----------------------------------------------------------------------
void IRFCCanvas::setWeight ( const int which, const int new_weight ) {
    if (which == 2)
		weight[2] = (float)new_weight;
	else
		obj_weight = new_weight;
	function_update();
	delete outData;
	outData = NULL;
}
//----------------------------------------------------------------------

void IRFCCanvas::setLevel ( const int which, const int level ) {
    obj_level[m_obj] = level;
    if (which==0) {        
		function_level[0] = (float)level;
    } else if (which==1) {
		function_level[1] = (float)level;
		assert(m_obj >= 0);
		obj_level[m_obj] = level;
    } else if (which==2) {
		function_level[2] = (float)level;
    } else if (which==3) {
		function_level[3] = (float)level;
    } else if (which==4) {
		function_level[4] = (float)level;
    }
	if (feature_status[which])
	{
		function_update();
		delete outData;
		outData = NULL;
	}
}
//----------------------------------------------------------------------
void IRFCCanvas::setWidthLevel ( const int which, const int widthLevel ) {
    if (which==0) {
		function_width[0] = (float)widthLevel;
    } else if (which==1) {
		assert(m_obj >= 0);
		obj_width[m_obj] = widthLevel;
		function_width[1] = (float)widthLevel;
    } else if (which==2) {
        m_inhomo=(float)widthLevel;
		function_width[2] = (float)widthLevel;
    } else if (which==3) {
		obj_width[m_obj] = (int)rint(widthLevel/2);
		function_width[3] = (float)widthLevel;
    } else if (which==4) {
		function_width[4] = (float)widthLevel;
    }
	if (feature_status[which])
	{
		function_update();
		delete outData;
		outData = NULL;
	}
}
//----------------------------------------------------------------------

void IRFCCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------

void IRFCCanvas::setTraining ( const bool training ) { 
    m_training = training;
}
//----------------------------------------------------------------------

void IRFCCanvas::setWhichSeed ( const int which_seed ) { 
    m_which_seed = which_seed;
}
//----------------------------------------------------------------------
void IRFCCanvas::setPainting( const bool painting ){
     m_painting = painting;
	 if (painting)
	 	m_erasing = false;
}

//----------------------------------------------------------------------
void IRFCCanvas::setErasing( const bool erasing ){
     m_erasing = erasing;
	 if (erasing)
	 	m_painting = false;
}
//----------------------------------------------------------------------
void IRFCCanvas::setCenter ( const int which, const int center ) {
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
void IRFCCanvas::setInvert ( const int which, const bool invert ) {
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
bool IRFCCanvas::IsSeed() const {
        if (nFg_seed>0 || points_filename)
            return true;
        else
           return false;                
}
//----------------------------------------------------------------------
void IRFCCanvas::setScale   ( const double scale )  {  //aka magnification
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
void IRFCCanvas::setSliceNo ( const int which, const int sliceNo ) {
	if (m_training)
		accumulate_training();
	function_update();
    m_sliceNo = sliceNo;
	slice_has_seeds = -1;
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
void IRFCCanvas::setWidth ( const int which, const int width ) {
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
void IRFCCanvas::setBrushSize ( const int size ) {
     m_BrushSize = size;
    }
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( IRFCCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( IRFCCanvas, wxPanel )
    EVT_PAINT(            IRFCCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           IRFCCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        IRFCCanvas::OnLeftDown     )
    EVT_LEFT_UP(          IRFCCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      IRFCCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        IRFCCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       IRFCCanvas::OnRightDown    )
    EVT_RIGHT_UP(         IRFCCanvas::OnRightUp      )
    EVT_CHAR(             IRFCCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
