/*
  Copyright 1993-2016, 2018, 2021-2025 Medical Image Processing Group
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
 * \file:  Segment2dCanvas.cpp
 * \brief  Segment2dCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "Segment2dFrame.h"
#include  "Segment2dIntDLControls.h"

#define QueueItem X_Point
#define HandleQueueError {fprintf(stderr,"Out of memory.\n");exit(1);}
#include "port_data/queue.c"


using namespace std;

#define SCALE_TO_FIT 0

#define VolNo 0

#define close_contour_code \
    if (straight_path? FindStraightPath(&orig, pt, o_contour.vertex[0], CLOSE)\
		: FindShortestPath(pt, o_contour.vertex[0], CLOSE)) \
    { \
		if (detection_mode==ILW) \
			iterate_live_wire(&orig, 0, ilw_min_pts); \
        phase = Close_Phase(&orig); \
        if (phase == 3) \
        { \
            FreeCircular(); \
			SetStatusText( \
				"Specify cut or add operations with the mouse buttons", 0); \
        } \
        else \
        if (phase == 1) \
            SetStatusText("Back Trace the Contour", 0); \
    } \
	else \
	{ \
        phase = 1; \
        SetStatusText("Back Trace the Contour", 0); \
    } \
	straight_path = false; \
	reload();

#define MIN(x, y) ((y)<(x)? (y): (x))
#define MAX(x, y) ((y)>(x)? (y): (x))

//----------------------------------------------------------------------
#define log(S)  {  cout << S << endl;  wxLogMessage( S );  }  //for debugging
//#define log(S)  //when not debugging

//returns true if a "modifier" key is pressed during the mouse event;
// false otherwise.
static bool isModified ( wxMouseEvent& e ) {
    return e.AltDown() || e.ControlDown() || e.MetaDown()
        || e.ShiftDown() || wxGetKeyState( WXK_ALT )
        || wxGetKeyState( WXK_CONTROL ) || wxGetKeyState( WXK_SHIFT )
        || e.MiddleIsDown() || e.RightIsDown();
}

//primarily used by one button mice or tablets to simulate middle button
static bool leftToMiddleModifier ( wxMouseEvent& e ) {
    return e.ControlDown() || e.ShiftDown()
        || wxGetKeyState( WXK_CONTROL ) || wxGetKeyState( WXK_SHIFT )
        || e.MiddleIsDown();
}

//primarily used by one button mice to simulate right button
static bool leftToRightModifier ( wxMouseEvent& e ) {
    return e.AltDown() || wxGetKeyState( WXK_ALT ) || e.RightIsDown();
}
//----------------------------------------------------------------------
const int  Segment2dCanvas::sSpacing=1;  ///< space, in pixels, between each slice (on the screen in the frame in the canvas)
const unsigned char onbit[9] = { 1, 2, 4, 8, 16, 32, 64, 128, 255};
const unsigned char offbit[9] = { 254, 253, 251, 247, 239, 223, 191, 127, 0};
//----------------------------------------------------------------------
/** \brief Segment2dCanvas ctor. */
Segment2dCanvas::Segment2dCanvas ( void ) {
    init();
}
//----------------------------------------------------------------------
/** \brief Segment2dCanvas ctor. */
Segment2dCanvas::Segment2dCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
  : MainCanvas ( parent, parent_frame, id, pos, size )
{
    init();
}
//----------------------------------------------------------------------
/**
 * \brief initialize members. persisted values should only be initialized
 * when init() is called from a ctor.
 * @param everything should be true when called from a ctor; otherwise, it
 * should be false (when called from release() below).
 * \todo determine what should be left alone when everything is false.
 */
void Segment2dCanvas::init ( ) {
	num_detection_modes = 9;
	detection_modes[0] = TRAINING;
	detection_modes[1] = ILW;
	detection_modes[2] = LSNAKE;
	detection_modes[3] = Mode::PAINT;
	detection_modes[4] = SEL_FEATURES;
	detection_modes[5] = REVIEW;
	detection_modes[6] = REPORT;
    detection_modes[7] = PEEK;
    detection_modes[8] = Mode::INT_DL;

    mFileOrDataCount = 0;
    mScale           = 1.8;
	reviewScale      = 0.0;
	review_slice     = 0;
    mLabels          = true;
    mRows            = 0;
    mCols            = 0;
    mImages          = (wxImage**)NULL;
    mBitmaps         = (wxBitmap**)NULL;
    mTx = mTy        = 0;
	mTx_v = mTy_v    = 0;
	mTx_f = mTy_f    = 0;
	mTx_p = mTy_p    = 0;
	FixedPx          = -1;
	FixedPy          = -1;
    mXSize = mYSize  = mZSize = 0;
    mLastX = mLastY  = -1;
	mother_filename[100] = 0;
	memset(&o_contour, 0, sizeof(o_contour));
	o_contour.last = -1;
	memset(&temp_contours, 0, sizeof(temp_contours));
	temp_contours.last = -1;
	V_edges          = NULL;
	object_vertex    = NULL;
	mask             = NULL;
	object_mask      = NULL;
	hzl_edge_mask    = NULL;
	vert_edge_mask   = NULL;
	hzl_edge_cont    = NULL;
	vert_edge_cont   = NULL;
	memset(&circular, 0, sizeof(circular));
	hzl_sign_buffer  = NULL;
	vert_sign_buffer = NULL;
	hzl_grad         = NULL;
	vert_grad        = NULL;
	hzl_cost         = NULL;
	vert_cost        = NULL;
	hzl_tempf        = NULL;
	vert_tempf       = NULL;
	hzl_tempc        = NULL;
	vert_tempc       = NULL;
	totlhc           = NULL;
	totlvc           = NULL;
	cc_vtx           = NULL;
	dp_anchor_point  = NULL;
	dp_anchor_points = 0;
	anchor_point_array_size = 0;
	ilw_control_point[0] = ilw_control_point[1] = NULL;
	ilw_control_points[0] = ilw_control_points[1] = 0;
	ilw_control_slice[0] = -1;
	ilw_control_slice[1] = -2;
	ilw_iterations = 2;
	Range = 1;
	pRow = pCol = 0;
	dir_vtx          = NULL;
	tblcc            = NULL;
	tblr             = NULL;
	phase            = 0;
	memset(&orig, 0, sizeof(orig));
	object_number    = 0;
	NumPoints        = 0;
	detection_mode   = PAINT;
	default_mask_value= 0;
	object_mask_slice_index = -1;
	LAST_DP_slice_index = -1;
	output_type = BINARY;
	ilw_min_pts      = 10;
	lsnake_alpha     = 0.1;
	lsnake_beta      = 0.2;
	lsnake_gamma     = -0.5;
	Gradient         = NULL;
	ControlPoint     = NULL;
	curr_feature     = 5;
	switch_images_flag = false;
	train_brush_size = 0;
    paint_brush_size = 2;
	training_phase   = 0;
	region           = NULL;
	hist             = NULL;
	painting         = false;
	peek_point       = NULL;
	num_peek_points  = 0;
	layout_flag      = false;
	overlay_flag     = true;
	overlay_intensity= 0;
	straight_path    = false;
#ifdef FindStraightPath_TEST
	straight_path    = true;
#endif
}
//----------------------------------------------------------------------
/** \brief Segment2dCanvas dtor. */
Segment2dCanvas::~Segment2dCanvas ( void ) {
    cout << "Segment2dCanvas::~Segment2dCanvas" << endl;
    wxLogMessage( "Segment2dCanvas::~Segment2dCanvas" );
	if (mCavassData)
	{
		FILE *fp;

		/* Save Grey Window parameters (and slice index) onto file (min and max values) */
printf("create greymap.TMP\n");
		wxFileName in_fname(mCavassData->m_fname);
		wxFileName grey_fname(in_fname.GetPath(), "greymap.TMP");
		if( (fp = fopen((const char *)grey_fname.GetFullPath().c_str(), "wb"))
				== NULL)
		{
			wxMessageBox("CAN'T SAVE GREYMAP PARAMETERS !");
		}
		else
		{
			fprintf(fp, "%.f %.f\n", (float)(getCenter(0)-getWidth(0)*.5),
				(float)(getCenter(0)+getWidth(0)*.5));	/* Grey Window (Min and MAX) */
			fprintf(fp, "%d %d\n", VolNo, sl.slice_index[VolNo][mCavassData->m_sliceNo]); /* Slice Index */
			fprintf(fp, "%d\n", overlay_intensity);	/* Overlay COlor */
			fclose(fp);

			/****************************/
			/* Parameters for DP (DP.c) */
			WriteFeatureList();
			/****************************/
		}

		/* Save last modification */
		save_mask_proc(0);

	}
    release();
}
//----------------------------------------------------------------------
/** \brief release memory allocated to this object. */
void Segment2dCanvas::release ( void ) {
	if(hzl_edge_mask != NULL){
		for (int j=0; j < (mCavassData->m_ySize+1); j++)
			free(hzl_edge_mask[j]);
		free(hzl_edge_mask);
		hzl_edge_mask = NULL;
	}
	if(hzl_edge_cont != NULL){
		for (int j=0; j < (mCavassData->m_ySize+1); j++)
			free(hzl_edge_cont[j]);
		free(hzl_edge_cont);
		hzl_edge_cont = NULL;
	}
	if(vert_edge_mask != NULL){
		for (int j=0; j < mCavassData->m_ySize; j++)
			free(vert_edge_mask[j]);
		free(vert_edge_mask);
		vert_edge_mask = NULL;
	}
	if(vert_edge_cont != NULL){
		for (int j=0; j < mCavassData->m_ySize; j++)
			free(vert_edge_cont[j]);
		free(vert_edge_cont);
		vert_edge_cont = NULL;
	}
    while (mCavassData!=NULL) {
        CavassData*  next = mCavassData->mNext;
        delete mCavassData;
        mCavassData = next;
    }
    freeImagesAndBitmaps();
	if (mask)
		free(mask);
	mask = NULL;
	if (object_mask)
		free(object_mask);
	object_mask = NULL;
	if (o_contour.vertex)
		free(o_contour.vertex);
	o_contour.vertex = NULL;
	if (temp_contours.last >= 0)
		free(temp_contours.vertex);
	temp_contours.vertex = NULL;
	if (V_edges)
	{
		if (V_edges[0])
			free(V_edges[0]);
		free(V_edges);
		V_edges = NULL;
	}
	if (object_vertex)
		free(object_vertex);
	object_vertex = NULL;
	if (hzl_sign_buffer)
	{
		for (int i=0; i<orig.height+1; i++)
			free(hzl_sign_buffer[i]);
		free(hzl_sign_buffer);
		hzl_sign_buffer = NULL;
	}
	if (vert_sign_buffer)
	{
		for (int i=0; i<orig.height; i++)
			free(vert_sign_buffer[i]);
		free(vert_sign_buffer);
		vert_sign_buffer = NULL;
	}
	Dealloc_CostArrays();
	if (dp_anchor_point)
		free(dp_anchor_point);
	dp_anchor_point = NULL;
	if (ilw_control_point[0])
	{
		free(ilw_control_point[0]);
		if (ilw_control_slice[1] != ilw_control_slice[0])
			free(ilw_control_point[1]);
		ilw_control_point[0] = ilw_control_point[1] = NULL;
	}
	if (orig.original_data)
		free(orig.original_data);
	orig.original_data = NULL;
	if (orig.tblx)
		free(orig.tblx);
	orig.tblx = NULL;
	if (orig.tbly)
		free(orig.tbly);
	orig.tbly = NULL;
	if (orig.tbl2x)
		free(orig.tbl2x);
	orig.tbl2x = NULL;
	if (orig.tbl2y)
		free(orig.tbl2y);
	orig.tbl2y = NULL;
	if (ControlPoint)
		free(ControlPoint);
	ControlPoint = NULL;
	ResetPeekPoints();
    init();
}
//----------------------------------------------------------------------
/** \brief load data from memory. */
void Segment2dCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    assert( mFileOrDataCount==0 );
    SliceData*  cd = new SliceData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    assert( mCavassData==NULL );
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
    mCavassData = cd;
    mFileOrDataCount = 1;

    SliceData&  A = *cd;
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
void Segment2dCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    assert( mFileOrDataCount==0 );
    SliceData*  cd = new SliceData( fn );
    assert( mCavassData==NULL );
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return;
	}
    mCavassData = cd;
    mFileOrDataCount = 1;

    SliceData&  A = *cd;
    A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
    mXSize = A.m_xSize;
    mYSize = A.m_ySize;
    mZSize = A.m_zSize;
	mCols  = mRows = 1;
	Imin = A.m_vh.scn.smallest_density_value[0];
	Imax = A.m_vh.scn.largest_density_value[0];
	compute_slices(&A.m_vh, &sl);
	Range = (int)(Imax-Imin);

 	int i, msk_slice=0;

	int iw, ih;
	GetSize(&iw, &ih);
#if SCALE_TO_FIT
	mScale = (double)ih/mYSize;
#else
	mScale = 1.8;
#endif
	mTx = (int)(iw-mScale*mXSize)/2;
	mTx_f = (int)(iw/2-mScale*mXSize);

	roix = roiy = roiw = roih = -1;
	output_type = BINARY;

	for(i=0; i<9; i++) empty_mask_flag[i] = TRUE;

	/* MASK BUFFER (initially all bits are OFF) */
	mask = (unsigned char *) calloc( sl.width*sl.height, 1);

	/* OBJECT MASK BUFFER (initially all bits are OFF) */
	object_mask = (unsigned char *) calloc( sl.width*sl.height, 1);

	/* Check if MASK file is the same */
	i = check_mask_file(&msk_slice);
	if( i < 0 )
	{
		wxMessageBox("PROBLEMS LOADING 'OBJECT' FILE !");
	}
	else
	/* Chosen input scene is not the same as the file referred to by the MASK file */
	if( i == 2 )
	{
		if(ask_user_what_to_do() < 0)
			return;
		msk_slice = 0;
	}
	initLUT(0);
	setSliceNo(0, msk_slice);
	i = msk_slice;
	object_mask_slice_index = i;
	orig.width = sl.width;
	orig.height = sl.height;

	/* LOAD FIRST SLICE */
	if (sl.bits == 1)
		orig.bits = 8;
	else
		orig.bits = sl.bits;
	orig.original_data = NULL;
	orig.index = 0;

	const int imagew = (int)ceil(mScale*orig.width);
	const int imageh = (int)ceil(mScale*orig.height);
	set_image_output_size(&orig, imagew, imageh);
	set_image_table(&orig);

	/* Load the object_mask buffer from the file */
	load_object_mask(object_mask_slice_index);

	/*****************************************************/
	V_edges = (unsigned char **) malloc(orig.height*sizeof(unsigned char *));
	for(i=0; i<orig.height; i++)
		V_edges[i] = (unsigned char *) calloc(orig.width+1, 1);

	if( (object_vertex = (unsigned char **) malloc((orig.height+1)*sizeof(unsigned char *))) == NULL)
	{
		printf("ERROR: Can't allocate memory !\n");
	}
	for(i=0; i<orig.height+1; i++)
	{
		if( (object_vertex[i] = (unsigned char *) calloc(orig.width+1, 1)) == NULL)
		{
			printf("ERROR: Can't allocate memory !\n");
		}
	}


	o_contour.vertex = (X_Point *) calloc(MAX_POINTS*sizeof(X_Point), 1);
	o_contour.last = -1;
	o_contour.slice_index = -1;
	o_contour.img = 0;

	/* Initialize temporary contours structure */
	temp_contours.last = -1;

	Alloc_CostArrays();
	/*****************************************************/

    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
/** \brief initialize the specified lookup table.
 *  \param which specifies the particular data set (if more than 1 read).
 */
void Segment2dCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
    else             mCavassData->mNext->initLUT();
}
//----------------------------------------------------------------------
/** \brief free any allocated images (of type wxImage) and/or bitmaps
 *  (of type wxBitmap)
 */
void Segment2dCanvas::freeImagesAndBitmaps ( void ) {
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
void Segment2dCanvas::reload ( void ) {
    if (!isLoaded(0))    return;
    freeImagesAndBitmaps();
    
    int  k;    
    mImages = (wxImage**)malloc( mCols * mRows * sizeof(wxImage*) );
    for (k=0; k<mCols*mRows; k++)    mImages[k]=NULL;
    
    mBitmaps = (wxBitmap**)malloc( mCols * mRows * sizeof(wxBitmap*) );
    for (k=0; k<mCols*mRows; k++)    mBitmaps[k]=NULL;
    
	if (detection_mode == REVIEW)
	{
		for (int j=0,n=review_slice; j<mRows; j++)
			for (int m=0; m<mCols; m++,n++)
			{
				if (n >= sl.total_slices)
				{
					j = mRows;
					break;
				}
				load_object_mask(object_mask_slice_index = n);
				const unsigned char bit = onbit[object_number];
				unsigned char *slice =
					(unsigned char *)malloc(orig.width*orig.height*3);
				for (int p=0,q=0; p<orig.width*orig.height; p++,q+=3)
					slice[q] = slice[q+1] = slice[q+2] =
						object_mask[p]&bit? 255: 0;
				if (mImages[j*mCols+m])
					mImages[j*mCols+m]->Destroy();
				mImages[j*mCols+m]= new wxImage(orig.width,orig.height, slice);
				mImages[j*mCols+m]->Rescale((int)ceil(reviewScale*orig.width),
					(int)ceil(reviewScale*orig.height));
				mBitmaps[j*mCols+m] =
					new wxBitmap( (const wxImage&) *mImages[j*mCols+m] );
			}
	}
	else
	{
		//note: image data is 24-bit rgb
		const SliceData&  A = *(SliceData *)mCavassData;

		const void *slice_data=mCavassData->getSlice(A.m_sliceNo);
		unsigned char*  slice = (unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
		const int offset = 0;
		const int which=0;

		if (A.m_size==1) 
		{
		  unsigned char* cData = (unsigned char *)slice_data;
		  for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 &&
		     j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
		  {
			if(which==0)
			{
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[cData[j]-A.m_min];
			}
		  }
		} 
		else if (A.m_size==2) 
		{
		  unsigned short* sData;
		  sData = (unsigned short *)slice_data;

		  for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 &&
		     j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
		  {
			  if(which==0)
			  {
				slice[i] = slice[i+1] = slice[i+2] = A.m_lut[sData[j]-A.m_min];
			  }
		  }
		} 

		const unsigned char tmp_ovl_color[3] = {0, 255, 0};
		const unsigned char bit = onbit[object_number];
		if (detection_mode==TRAINING || detection_mode==SEL_FEATURES)
		{
			for (int i=0,j=0; j<A.m_xSize*A.m_ySize; i+=3,j++)
				if (region[j])
					for (int k=0; k<3; k++)
						slice[i+k] = (1+slice[i+k]+tmp_ovl_color[k])/2;
		}
		else if (overlay_flag && !mask_index_different())
			for (int i=0,j=0; j<A.m_xSize*A.m_ySize; i+=3,j++)
				if (object_mask[j] & bit)
					for (int k=0; k<3; k++)
						slice[i+k] = (1+slice[i+k]+tmp_ovl_color[k])/2;
		if (mCols && mRows)
		{
			if(mImages[0]!=NULL)
				mImages[0]->Destroy();
			mImages[0] = new wxImage( A.m_xSize, A.m_ySize, slice );
			if (mScale!=1.0) 
			{
				mImages[0]->Rescale( (int)(ceil(mScale*A.m_xSize)),
								  (int)(ceil(mScale*A.m_ySize)) );
			}
			mBitmaps[0] = new wxBitmap( (const wxImage&) *mImages[0] );
		}
		if (mCols>1 && mRows)
		{
			Calc_Combined_Edge_Costs(accepted_list[object_number%8], 0);
			slice = (unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
		    int row, col;

		    for (int i=row=0; row<orig.height; row++)
		        for (col=0; col<orig.width; col++,i+=3)
		            slice[i] = slice[i+1] = slice[i+2] =
						A.m_lut[((int)hzl_cost[row][col]+hzl_cost[row+1][col]+
						vert_cost[row][col]+vert_cost[row][col+1])/4-A.m_min];
			if(mImages[1]!=NULL)
				mImages[1]->Destroy();
			mImages[1] = new wxImage( A.m_xSize, A.m_ySize, slice );
			if (mScale!=1.0) 
			{
				mImages[1]->Rescale( (int)(ceil(mScale*A.m_xSize)),
								  (int)(ceil(mScale*A.m_ySize)) );
			}
			mBitmaps[1] = new wxBitmap( (const wxImage&) *mImages[1] );
		}
	}
    Refresh();
	switch (detection_mode)
	{
		case ILW:
		case LSNAKE:
			switch (phase)
			{
				case 0:
					SetStatusText("SELECT POINT", 2);
					if (ilw_control_points[0] &&
							(sl.slice_index[VolNo][mCavassData->m_sliceNo]==
							 ilw_control_slice[0]-1||
							 sl.slice_index[VolNo][mCavassData->m_sliceNo]==
							 ilw_control_slice[1]+1))
						SetStatusText("COMPUTE", 3);
					else
						SetStatusText("", 3);
					SetStatusText("DONE", 4);
					break;
				case 1:
					SetStatusText("SELECT POINT", 2);
					SetStatusText("BACK TRACE", 3);
					SetStatusText("CLOSE", 4);
					break;
				case 2:
					SetStatusText("SELECT POINT", 2);
					SetStatusText("", 3);
					SetStatusText("QUIT", 4);
					break;
				case 3:
					SetStatusText("ADD", 2);
					SetStatusText("CUT", 3);
					SetStatusText("DONE", 4);
					break;
			}
			break;
		case PAINT:
			switch (paint_brush_size? phase? phase: 3: phase)
			{
				case 0:
					SetStatusText("TRACE", 2);
					SetStatusText("", 3);
					SetStatusText("DONE", 4);
					break;
				case 1:
	    			SetStatusText("SELECT POINT", 2);
		    		SetStatusText("BACK TRACE", 3);
					SetStatusText("CLOSE", 4);
					break;
				case 2:
					SetStatusText("SELECT POINT", 2);
					SetStatusText("", 3);
					SetStatusText("QUIT", 4);
					break;
				case 3:
					SetStatusText("ADD", 2);
					SetStatusText(paint_brush_size<0? "":"CUT", 3);
                    //SetStatusText(paint_brush_size==0 ? "DONE" : "---", 4);
                    SetStatusText("DONE", 4);
					break;
			}
			break;
		case TRAINING:
			switch (training_phase)
			{
				case 0:
					SetStatusText("TRACE", 2);
					SetStatusText("RUN STATS", 3);
					SetStatusText("", 4);
					break;
				case 1:
					SetStatusText("DRAW", 2);
					SetStatusText("ERASE", 3);
					SetStatusText("STOP", 4);
					break;
				case 2:
					SetStatusText("SELECT POINT", 2);
					SetStatusText("", 3);
					SetStatusText("DONE", 4);
					break;
			}
			break;
		case SEL_FEATURES:
			SetStatusText("BLINK", 2);
			SetStatusText("", 3);
			SetStatusText("", 4);
			break;
		case REPORT:
			SetStatusText("MOVE", 2);
			SetStatusText("", 3);
			SetStatusText("", 4);
			break;
		case PEEK:
			SetStatusText("MARK", 2);
			SetStatusText("", 3);
			SetStatusText("", 4);
			break;
	}
	if (layout_flag)
	{
		SetStatusText("SCROLL", 2);
		SetStatusText("FIX POINT", 3);
	}
}
//----------------------------------------------------------------------
static void reportKey ( long k ) {
    if (k == 32) {
        wxLogMessage( "Segment2dCanvas::OnChar: ' '" );
    } else if (wxIsprint(k)) {
        wxString s1( "Segment2dCanvas::OnChar: " );
        wxString s2( (char)k );
        wxLogMessage( s1 + s2 );
    } else if (k == 0) {
        wxLogMessage( "Segment2dCanvas::OnChar: nul" );
    } else if (k == 27) {
        wxLogMessage( "Segment2dCanvas::OnChar: Esc" );
    } else if (k == 127) {
        wxLogMessage( "Segment2dCanvas::OnChar: Del" );
    } else if (wxIscntrl(k) && k<27) {
        wxString s1( "Segment2dCanvas::OnChar: ctrl-" );
        cout << k << endl;
        wxString s2( (char)(k+'a'-1) );
        wxLogMessage( s1 + s2 );
    } else {
        wxLogMessage( "Segment2dCanvas::OnChar: ?" );
    }
}
/** \brief note: spacebar mimics middle mouse button. */
void Segment2dCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "Segment2dCanvas::OnChar" << endl;
    reportKey( e.m_keyCode );

    if (e.m_keyCode==' ') {
        if (isLoaded(1)) {
            mCavassData->mNext->mDisplay = !mCavassData->mNext->mDisplay;
            reload();
        }
    } else if (e.m_keyCode=='h' || e.m_keyCode == 'H' || e.m_keyCode == '/'
               || e.m_keyCode == '?') {
        wxCommandEvent e;
        Segment2dFrame::OnHelp( e );
    } else {
        //pass the event up to the parent frame
        //m_parent_frame->ProcessEvent( e );
    }
}
//----------------------------------------------------------------------
/** \brief callback for mouse move events */
void Segment2dCanvas::OnMouseMove ( wxMouseEvent& e ) {
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
	int *Tx=&mTx, *Ty=&mTy;
	switch (detection_mode)
	{
		case SEL_FEATURES:
			Tx = &mTx_f;
			Ty = &mTy_f;
			break;
		case REVIEW:
			Tx = &mTx_v;
			Ty = &mTy_v;
			break;
		case REPORT:
			Tx = &mTx_p;
			Ty = &mTy_p;
			break;
	}
    const long  wx = dc.DeviceToLogicalX( pos.x ) - *Tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - *Ty;
    //if we are NOT in any mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    if (e.LeftIsDown() && !isModified(e) && layout_flag)
    {
        if (mLastX==-1 || mLastY==-1) {
            SetCursor( wxCursor(wxCURSOR_HAND) );
            mLastX = wx;
            mLastY = wy;
            return;
        }
        
        bool  changed=false;
        if (abs(wx-mLastX)>2) {
            *Tx += (wx-mLastX)/2;  // / 4 * 3;
            changed = true;
        } else if (abs(wx-mLastX)>2) {
            *Tx -= (mLastX-wx)/2;  // / 4 * 3;
            changed = true;
        }
        if (abs(wy-mLastY)>2) {
            *Ty += (wy-mLastY)/2;  // / 4 * 3;
            changed = true;
        } else if (abs(wy-mLastY)>2) {
            *Ty -= (mLastY-wy)/2;  // / 4 * 3;
            changed = true;
        }
        
        if (changed)  Refresh();
        mLastX=wx;
        mLastY=wy;
        return;
    }
	if (mLastX!=-1 || mLastY!=-1) {
        SetCursor( *wxSTANDARD_CURSOR );
        mLastX = mLastY = -1;
    }
	if (wx >= orig.locx && wx < orig.locx+orig.w &&
		wy >= orig.locy && wy < orig.locy+orig.h)
	{
		X_Point pt;
		pt.y = orig.tbly[wy - orig.locy];
		pt.x = orig.tblx[wx - orig.locx];
		switch (detection_mode)
		{
			case ILW:
			case LSNAKE:
				if (phase == 1)
				{
					int path_found=FALSE;
					if (straight_path)
					{
						if (FindStraightPath(&orig, circular.pt, pt, OPEN) &&
								e.LeftIsDown() && 
								sl.slice_index[VolNo][mCavassData->m_sliceNo]==
								o_contour.slice_index)
						{
							path_found = !Live_Wire(&orig, wx, wy, OPEN);
							Point_Selected(&orig, wx, wy);
							UpdateCircular(pt, &orig);
						}
					}
					else
						path_found = FindShortestPath(circular.pt, pt, OPEN);
					if (path_found)
						Live_Wire(&orig, wx, wy, OPEN);
				}
				break;
			case PAINT:
				if ( (e.MiddleIsDown() && phase==0 && paint_brush_size>0)
                                     || (e.LeftIsDown() && leftToMiddleModifier(e) && phase==0 && paint_brush_size>0) )
					erase_mask(wx, wy);
				else if (e.LeftIsDown() && !leftToMiddleModifier(e) && !leftToRightModifier(e))
				{
					if (paint_brush_size == 0)
					{
						if (phase == 1)
						{
							draw_and_save_vertices(-1, -1, wx, wy);
							painting = true;
						}
					}
					else if (phase == 0)
						paint_mask(wx, wy);
				}
				if (paint_brush_size==0 && phase==1 && o_contour.slice_index==
						sl.slice_index[VolNo][mCavassData->m_sliceNo])
				{
				    X_Point *vline;
					int nv;

					if (VComputeLine(
							o_contour.vertex[o_contour.last].x,
							o_contour.vertex[o_contour.last].y,
							pt.x, pt.y, &vline, &nv))
					{
						wxMessageBox("Out of memory.");
						return;
					}
					for (int j=0; j<nv; j++)
					{
						Points[j].x = vline[j].x;
						Points[j].y = vline[j].y;
					}
					NumPoints = nv;
					free(vline);
					painting = true;
					Refresh();
				}
				break;
			case TRAINING:
				if (e.LeftIsDown() && !leftToMiddleModifier(e) && !leftToRightModifier(e))
				{
					if (train_brush_size == 0)
					{
						if (training_phase == 1)
							draw_and_save_vertices(-1, -1, wx, wy);
					}
					else
						switch (training_phase)
						{
							case 1:
								paint_region(wx, wy);
								break;
							case 2:
								Erase_PxRegion(wx, wy);
								break;
						}
				}
				if (training_phase==1 && train_brush_size==0 && o_contour.last>=0)
				{
					X_Point *pp, *ptr;
					int nnp;
					int i;
					int ix1, iy1, ix2, iy2; /* image coord after
					 mapping to NW pixel corner */

					ix1 = o_contour.vertex[o_contour.last].x;
					iy1 = o_contour.vertex[o_contour.last].y;
					ix2 = pt.x;
					iy2 = pt.y;
 
					/* points calculated are in image pixel coordinates */
					if (VComputeLine(ix1, iy1, ix2, iy2, &pp, &nnp))
					{
						wxMessageBox("Out of memory.");
						return;
					}
					NumPoints = 0;
					ptr = Points;
					ptr->x = ix1;
					ptr->y = iy1;
					ptr++;
					for (i=1; i<nnp; i++)
					{
						ix2 = pp[i].x;
						iy2 = pp[i].y;
						if ( iy1==iy2 || ix1==ix2 )
						/* Vertical or Horizontal edges */
						{
							ptr->x = ix2;
							ptr->y = iy2;
							ptr++;
							if( (ptr-Points+1)==MAX_POINTS)
							{
								SetStatusText(
									"ERROR: Points structure OVERFLOWED.", 0);
								free(pp);
								return;
							}
						} /* endif hzl/vert */
						else 
						/* diagonal (UP) edges or diagonal (DOWN) edges */
						{
							ptr->x = ix2;
							ptr->y = iy1;
							ptr++;
							if( (ptr-Points+1)==MAX_POINTS)
							{
								SetStatusText(
									"ERROR: Points structure OVERFLOWED.", 0);
								free(pp);
								return;
							}
							ptr->x = ix2;
							ptr->y = iy2;
							ptr++;
							if( (ptr-Points+1)==MAX_POINTS)
							{
								SetStatusText(
									"ERROR: Points structure OVERFLOWED.", 0);
								free(pp);
								return;
							}

						} /* end if diag up or down */

						ix1 = ix2;
						iy1 = iy2;
					}
					/* ptr points to very last Points-vertex */
					NumPoints = (int)(ptr-Points);
					free(pp);
					reload();
				}
				break;

            case INT_DL:
                // cerr << "Segment2dCanvas::OnMouseMove @todo handle INT_DL" << endl;
                {
                    auto f = dynamic_cast<Segment2dFrame*>( m_parent_frame );
                    if (f != nullptr && f->mIntDLControls != nullptr)
                        f->mIntDLControls->doMouseMove( e, pt.x, pt.y );
                }
                break;

			case PEEK:
				/* Show the density and the Object# of the pixel */
                {
                    wxString text = "";
                    if (orig.bits > 1) {
                        int value;
                        int vn, vs, ve, vw; /* values for: North, South, East and
						West */
                        double gradient;
                        const int x = pt.x,
                                y = pt.y,
                                xp = x >= orig.width - 1 ? orig.width - 1 : x + 1,
                                yp = y >= orig.height - 1 ? orig.height - 1 : y + 1,
                                xm = x <= 0 ? 0 : x - 1,
                                ym = y <= 0 ? 0 : y - 1;
                        if (orig.bits == 8) {
                            const unsigned char *t8 = (unsigned char *)
                                    mCavassData->getSlice(mCavassData->m_sliceNo);
                            value = t8[pt.y * orig.width + pt.x];
                            vn = t8[ym * orig.width + pt.x];
                            vs = t8[yp * orig.width + pt.x];
                            ve = t8[pt.y * orig.width + xp];
                            vw = t8[pt.y * orig.width + xm];
                            gradient = sqrt((double) (vs - vn) * (vs - vn) +
                                            (double) (ve - vw) * (ve - vw));
                        } else {
                            const unsigned short *t16 = (unsigned short *)
                                    mCavassData->getSlice(mCavassData->m_sliceNo);
                            value = t16[pt.y * orig.width + pt.x];
                            vn = t16[ym * orig.width + pt.x];
                            vs = t16[yp * orig.width + pt.x];
                            ve = t16[pt.y * orig.width + xp];
                            vw = t16[pt.y * orig.width + xm];
                            gradient = sqrt((double) (vs - vn) * (vs - vn) +
                                            (double) (ve - vw) * (ve - vw));
                        }

                        text = wxString::Format("(%3d,%3d): %3d : (%3.0f) ",
                                                pt.x, pt.y, value, gradient);
                    }
                    if (object_mask[pt.y * orig.width + pt.x] > 0) {
                        text += " [";
                        for (int i = 0; i < 8; i++) {
                            if ((object_mask[pt.y * orig.width + pt.x] & onbit[i]) > 0)
                                text += wxString::Format(" %d ", i + 1);
                        }
                        text += "]";
                    }
                    SetStatusText(text, 0);
                }
				break;

			case REPORT:
			case SEL_FEATURES:
				//nothing to do (but i don't want to see the default message)
				break;

            default:
                cerr << "Segment2dCanvas::OnMouseMove: unrecognized detection mode, " << detection_mode << endl;
                break;
        }  //end switch
	}  //end if
}  //end OnMouseMove

int Segment2dCanvas::get_closest_contour_point(int x, int y)
{
	int i,index, dx, dy;
	float min_dist, temp;

	if(o_contour.last < 0) return(-1);

	index = 0;
	min_dist = 9999999;
	for(i=0; i<=o_contour.last; i++)
	{
		dx = x-o_contour.vertex[i].x;
		dy = y-o_contour.vertex[i].y;
		temp = dx*dx + dy*dy;

		if(temp < min_dist)
		{
			index = i;
			min_dist = temp;
		}
	}

	return(index);
}
void Segment2dCanvas::delete_open_contour_to_the_end(IMAGE *image, int overlay,
	int n, OPEN_CONTOUR *cont)
{
	if(image == NULL) return;
	if(cont == NULL) return;
	if(cont->last < 0) return;
	if( n > cont->last) return;
	if(n<0) return;

	for (int i=n+1; i<=cont->last; i++)
		object_vertex[cont->vertex[i].y][cont->vertex[i].x] &=
			offbit[object_number];

	cont->last = n;
}

void Segment2dCanvas::erase_mask(int cx, int cy)
{
	if (paint_brush_size<0 ||
		cx<orig.locx || cx>=orig.locx+orig.w ||
		cy<orig.locy || cy>=orig.locy+orig.h)
		return;
	int i, j, xorig, yorig, xi, yi, xf, yf;
	xorig = orig.tblx[cx - orig.locx];
	yorig = orig.tbly[cy - orig.locy];
	xi = xorig-paint_brush_size/2;
	xf = xi+paint_brush_size;
	yi = yorig-paint_brush_size/2;
	yf = yi+paint_brush_size;
	if (xi < 0)
		xi = 0;
	if (yi < 0)
		yi = 0;
	if (xf > orig.width)
		xf = orig.width;
	if (yf > orig.height)
		yf = orig.height;

	/*------------------------------------------------------*/
	/*  if new slice, save object mask from previous slice  */
	/*  and load objects already saved for this slice       */
	/*------------------------------------------------------*/
	save_mask_proc(0);  /* saves object mask */
	if(mask_index_different())
	{
 
		object_mask_slice_index= sl.slice_index[VolNo][mCavassData->m_sliceNo];

		/* Load the appropriate object-mask buffer */
		load_object_mask(object_mask_slice_index);
	}
	for (j=yi; j<yf; j++)
		for (i=xi; i<xf; i++)
			if ((j-yorig)*(j-yorig)+(i-xorig)*(i-xorig) <=
					(paint_brush_size/2)*(paint_brush_size/2)+1)
				object_mask[*(tblr+j) + i] &= offbit[object_number];
	reload();
}
void Segment2dCanvas::paint_mask(int cx, int cy)
{
	if (cx<orig.locx || cx>=orig.locx+orig.w ||
		cy<orig.locy || cy>=orig.locy+orig.h)
		return;
	int i, j, xorig, yorig, xi, yi, xf, yf;
	xorig = orig.tblx[cx - orig.locx];
	yorig = orig.tbly[cy - orig.locy];
	xi = xorig-paint_brush_size/2;
	xf = xi+paint_brush_size;
	yi = yorig-paint_brush_size/2;
	yf = yi+paint_brush_size;
	if (xi < 0)
		xi = 0;
	if (yi < 0)
		yi = 0;
	if (xf > orig.width)
		xf = orig.width;
	if (yf > orig.height)
		yf = orig.height;

	/*------------------------------------------------------*/
	/*  if new slice, save object mask from previous slice  */
	/*  and load objects already saved for this slice       */
	/*------------------------------------------------------*/
	save_mask_proc(0);  /* saves object mask */
	if(mask_index_different())
	{
 
		object_mask_slice_index= sl.slice_index[VolNo][mCavassData->m_sliceNo];

		/* Load the appropriate object-mask buffer */
		load_object_mask(object_mask_slice_index);
	}
	if (paint_brush_size < 0)
	{
		X_Point cur, nex;
		unsigned char *component_mask, *background_mask;
		component_mask = (unsigned char *)calloc(orig.width*orig.height, 1);
		background_mask = (unsigned char *)calloc(orig.width*orig.height, 1);
		if (component_mask==NULL || background_mask==NULL)
		{
			wxMessageBox("Out of memory.");
			return;
		}
		ClearQueue;
		if (object_mask[*(tblr+yorig) + xorig] & onbit[object_number])
		{
			component_mask[orig.width*yorig + xorig] = onbit[object_number];
			cur.x = xorig;
			cur.y = yorig;
			Enqueue(cur);
		}
		while (!QueueIsEmpty)
		{
			Dequeue(cur);
			if (cur.x > 0)
			{
				nex.x = cur.x-1;
				nex.y = cur.y;
				if (component_mask[orig.width*nex.y + nex.x]==0 &&
					   (object_mask[*(tblr+nex.y)+nex.x]&onbit[object_number]))
				{
					component_mask[orig.width*nex.y+nex.x] =
						onbit[object_number];
					Enqueue(nex);
				}
			}
			if (cur.x < orig.width-1)
			{
				nex.x = cur.x+1;
				nex.y = cur.y;
				if (component_mask[orig.width*nex.y + nex.x]==0 &&
					   (object_mask[*(tblr+nex.y)+nex.x]&onbit[object_number]))
				{
					component_mask[orig.width*nex.y+nex.x] =
						onbit[object_number];
					Enqueue(nex);
				}
			}
			if (cur.y > 0)
			{
				nex.x = cur.x;
				nex.y = cur.y-1;
				if (component_mask[orig.width*nex.y + nex.x]==0 &&
					   (object_mask[*(tblr+nex.y)+nex.x]&onbit[object_number]))
				{
					component_mask[orig.width*nex.y+nex.x] =
						onbit[object_number];
					Enqueue(nex);
				}
			}
			if (cur.y < orig.height-1)
			{
				nex.x = cur.x;
				nex.y = cur.y+1;
				if (component_mask[orig.width*nex.y + nex.x]==0 &&
					   (object_mask[*(tblr+nex.y)+nex.x]&onbit[object_number]))
				{
					component_mask[orig.width*nex.y+nex.x] =
						onbit[object_number];
					Enqueue(nex);
				}
			}
		}
		for (cur.y=0; cur.y<orig.height; cur.y++)
			for (cur.x=0; cur.x<orig.width;
					cur.x += cur.y&&cur.y<orig.height-1? orig.height-1:1)
				if ((object_mask[*(tblr+cur.y)+cur.x]&onbit[object_number])==0)
				{
					background_mask[orig.width*cur.y+cur.x] = 1;
					Enqueue(cur);
				}
		while (!QueueIsEmpty)
		{
			Dequeue(cur);
			if (cur.x > 0)
			{
				nex.x = cur.x-1;
				nex.y = cur.y;
				if (background_mask[orig.width*nex.y+nex.x]==0 &&
						component_mask[orig.width*nex.y+nex.x]==0)
				{
					background_mask[orig.width*nex.y+nex.x] = 1;
					Enqueue(nex);
				}
			}
			if (cur.x < orig.width-1)
			{
				nex.x = cur.x+1;
				nex.y = cur.y;
				if (background_mask[orig.width*nex.y+nex.x]==0 &&
						component_mask[orig.width*nex.y+nex.x]==0)
				{
					background_mask[orig.width*nex.y+nex.x] = 1;
					Enqueue(nex);
				}
			}
			if (cur.y > 0)
			{
				nex.x = cur.x;
				nex.y = cur.y-1;
				if (background_mask[orig.width*nex.y+nex.x]==0 &&
						component_mask[orig.width*nex.y+nex.x]==0)
				{
					background_mask[orig.width*nex.y+nex.x] = 1;
					Enqueue(nex);
				}
			}
			if (cur.y < orig.height-1)
			{
				nex.x = cur.x;
				nex.y = cur.y+1;
				if (background_mask[orig.width*nex.y+nex.x]==0 &&
						component_mask[orig.width*nex.y+nex.x]==0)
				{
					background_mask[orig.width*nex.y+nex.x] = 1;
					Enqueue(nex);
				}
			}
		}
		for (cur.y=0; cur.y<orig.height; cur.y++)
			for (cur.x=0; cur.x<orig.width; cur.x++)
				if (background_mask[orig.width*cur.y+cur.x] == 0)
					object_mask[*(tblr+cur.y)+cur.x] |= onbit[object_number];

		free(background_mask);
		free(component_mask);
	}
	else
		for (j=yi; j<yf; j++)
			for (i=xi; i<xf; i++)
				if ((j-yorig)*(j-yorig)+(i-xorig)*(i-xorig) <=
						(paint_brush_size/2)*(paint_brush_size/2)+1)
					object_mask[*(tblr+j) + i] |= onbit[object_number];
	reload();
}

void Segment2dCanvas::Erase_PxRegion(int cx, int cy)
{
	if (train_brush_size<0 ||
		cx<orig.locx || cx>=orig.locx+orig.w ||
		cy<orig.locy || cy>=orig.locy+orig.h)
		return;
	int i, j, xorig, yorig, xi, yi, xf, yf;
	xorig = orig.tblx[cx - orig.locx];
	yorig = orig.tbly[cy - orig.locy];
	xi = xorig-train_brush_size/2;
	xf = xi+train_brush_size;
	yi = yorig-train_brush_size/2;
	yf = yi+train_brush_size;
	if (xi < 0)
		xi = 0;
	if (yi < 0)
		yi = 0;
	if (xf > orig.width)
		xf = orig.width;
	if (yf > orig.height)
		yf = orig.height;
	for (j=yi; j<yf; j++)
		for (i=xi; i<xf; i++)
			region[*(tblr+j) + i] = 0;
	reload();
}
void Segment2dCanvas::paint_region(int cx, int cy)
{
//@@ to do: implement fill
	if (cx<orig.locx || cx>=orig.locx+orig.w ||
		cy<orig.locy || cy>=orig.locy+orig.h)
		return;
	int i, j, xorig, yorig, xi, yi, xf, yf;
	xorig = orig.tblx[cx - orig.locx];
	yorig = orig.tbly[cy - orig.locy];
	xi = xorig-train_brush_size/2;
	xf = xi+train_brush_size;
	yi = yorig-train_brush_size/2;
	yf = yi+train_brush_size;
	if (xi < 0)
		xi = 0;
	if (yi < 0)
		yi = 0;
	if (xf > orig.width)
		xf = orig.width;
	if (yf > orig.height)
		yf = orig.height;
	for (j=yi; j<yf; j++)
		for (i=xi; i<xf; i++)
			region[*(tblr+j) + i] = 255;
	reload();
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button down events. */
void Segment2dCanvas::OnRightDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events

    log( "OnRightDown" );

    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
	int Tx=mTx, Ty=mTy;
	switch (detection_mode)
	{
		case SEL_FEATURES:
			Tx = mTx_f;
			Ty = mTy_f;
			break;
		case REVIEW:
			Tx = mTx_v;
			Ty = mTy_v;
			break;
		case REPORT:
			Tx = mTx_p;
			Ty = mTy_p;
			break;
	}
    const long  wx = dc.DeviceToLogicalX( pos.x ) - Tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - Ty;
    mLastX = wx;
    mLastY = wy;

    if (detection_mode == INT_DL) {
        auto f = dynamic_cast<Segment2dFrame*>( m_parent_frame );
        f->mIntDLControls->doRightDown();
        return;
    }

	if (detection_mode == TRAINING)
	{
		switch (training_phase)
		{
			case 1:
			    NumPoints=0;
				if (temp_contours.last<=0 && o_contour.last>=0)
				{
					copy_ocontour_into_temp_array();
					o_contour.last = -1;
				}
				training_phase = 0;
				SetStatusText("Select Point or Pixel Region", 0);
				reload();
				break;
			case 2:
				training_phase = 1;
				SetStatusText("Select point on Boundary", 0);
				break;
		}
	}
	else if (phase==0 && mask_index_different())
	{
		save_mask_proc(0);  /* saves object mask */
		object_mask_slice_index =
			sl.slice_index[VolNo][mCavassData->m_sliceNo];

		/* Load the appropriate object-mask buffer */
		load_object_mask(object_mask_slice_index);
		assert((ilw_control_points[0]>0) == (ilw_control_points[1]>0));
		reload();
	}
	else if (phase == 1)
	{
		if (sl.slice_index[VolNo][mCavassData->m_sliceNo]!=
				o_contour.slice_index || wx-orig.locx<0 || wy-orig.locy<0 ||
				wx-orig.locx>=orig.w || wy-orig.locy>=orig.h)
			return;
		X_Point pt;
		pt.y = orig.tbly[wy - orig.locy];
		pt.x = orig.tblx[wx - orig.locx];
		if ((detection_mode==ILW||detection_mode==LSNAKE) &&
				Is_Point_Selected_Valid(&orig, wx, wy) == 0)
		{
			if (straight_path? FindStraightPath(&orig, circular.pt, pt, OPEN):
					FindShortestPath(circular.pt, pt, OPEN))
			{
				Live_Wire(&orig, wx, wy, OPEN);
				Point_Selected(&orig, wx, wy);
				UpdateCircular(pt, &orig);
				add_anchor_point(pt.x, pt.y, &o_contour);
			    if (detection_mode == LSNAKE)
			 		iterate_live_snake(&orig, ilw_iterations,
						lsnake_alpha, lsnake_beta, lsnake_gamma);
				close_contour_code;
			}
		}
		else if (detection_mode == PAINT)
		{
			if (paint_brush_size == 0)
			{
				NumPoints = 0;
				draw_and_save_vertices(-1, -1, wx, wy);
				draw_and_save_vertices(-1, -1,
					orig.tbl2x[o_contour.vertex[0].x],
					orig.tbl2y[o_contour.vertex[0].y]);
				if (o_contour.last>0 && o_contour.vertex[o_contour.last].x==
						o_contour.vertex[0].x &&
						o_contour.vertex[o_contour.last].y==
						o_contour.vertex[0].y)
					o_contour.last--;
				phase = 3;
				painting = true;
				SetStatusText(
					"Specify cut or add operations with the mouse buttons", 0);
				reload();
			}
			return;
		}
	}
	else if (phase == 2)
	{
        phase = 1;
		if (detection_mode==ILW || detection_mode==LSNAKE)
			UpdateCircular(o_contour.vertex[o_contour.last], &orig);
		reload();
	}
	else if (phase == 3)
	{
		if (sl.slice_index[VolNo][mCavassData->m_sliceNo] !=
				o_contour.slice_index)
			return;
		o_contour.last = -1;
		NumPoints = 0;
		clear_Vedges_array();
		reset_object_vertex_of_temparrays();
		clear_temporary_contours_array();
		Reset_ObjectVertex();
		dp_anchor_points = 0;
		phase = 0;
		assert((ilw_control_points[0]>0) == (ilw_control_points[1]>0));
		reload();
		SetStatusText("Trace boundary on slice", 0);
	}
}
//----------------------------------------------------------------------
/** \brief Callback to handle right mouse button up events. */
void Segment2dCanvas::OnRightUp ( wxMouseEvent& e ) {
    log( "OnRightUp" );
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button down events. */
void Segment2dCanvas::OnMiddleDown ( wxMouseEvent& e ) {
	SetFocus();  //to regain/recapture keypress events

	log( "OnMiddleDown" );

	if (layout_flag && detection_mode!=SEL_FEATURES && detection_mode!=REVIEW
			&& detection_mode!=REPORT)
	{
		wxClientDC  dc(this);
		PrepareDC(dc);
		const long  wx = dc.DeviceToLogicalX(e.GetPosition().x) - mTx;
		const long  wy = dc.DeviceToLogicalY(e.GetPosition().y) - mTy;
		if (wx-orig.locx>=0 && wy-orig.locy>=0 &&
				wx-orig.locx<orig.w && wy-orig.locy<orig.h)
		{
			FixedPx = orig.tblx[wx - orig.locx];
			FixedPy = orig.tbly[wy - orig.locy];
		}
		return;
	}
	if (detection_mode == TRAINING)
	{
		switch (training_phase)
		{
			case 0:
				Run_Statistics();
				break;
			case 1:
			    NumPoints=0;
				training_phase = 2;
				SetStatusText("Select point up to which to be erased.", 0);
				reload();
				break;
		}
	}
	else if (phase == 0)
	{
		if (detection_mode==PAINT && paint_brush_size)
		{
			wxClientDC  dc(this);
			PrepareDC(dc);
			const long  wx = dc.DeviceToLogicalX(e.GetPosition().x) - mTx;
			const long  wy = dc.DeviceToLogicalY(e.GetPosition().y) - mTy;
			erase_mask(wx, wy);
			return;
		}
		X_Point pt;
		wxString message;
		if (ilw_control_points[0])
		{
			if (sl.slice_index[VolNo][mCavassData->m_sliceNo] !=
			    ilw_control_slice[0]-1)
			  if (sl.slice_index[VolNo][mCavassData->m_sliceNo] !=
			      ilw_control_slice[1]+1)
			  /* Not on next slice */
			  {
				if (ilw_control_slice[0] > 0)
				  if (ilw_control_slice[1] < sl.total_slices-1)
					message = wxString::Format("Do slice %d or %d.",
					  ilw_control_slice[0], ilw_control_slice[1]+2);
				  else
					message =
					  wxString::Format("Do slice %d.", ilw_control_slice[0]);
				else
				  if (ilw_control_slice[1] < sl.total_slices-1)
					message =
					  wxString::Format("Do slice %d.",ilw_control_slice[1]+2);
				  else
					message = "No next slice.";
				wxMessageBox(message);
			  }
			  else /* On next slice forward */
			  {
				dp_anchor_points = 0;
				assert(ilw_control_points[1]>0);
				pt.x = ilw_control_point[1][ilw_control_points[1]-1][0];
				pt.y = ilw_control_point[1][ilw_control_points[1]-1][1];
				if (Is_Point_Selected_Valid(NULL, pt.x, pt.y)==0)
				  InitialPoint_Selected(NULL, pt.x, pt.y);
				for (int j=0; j<ilw_control_points[1]; j++)
				{
				  pt.x = ilw_control_point[1][j][0];
				  pt.y = ilw_control_point[1][j][1];
				  if (Is_Point_Selected_Valid(NULL, pt.x, pt.y) == 0)
				  {
					if (straight_path?
					    FindStraightPath(NULL, circular.pt,pt,OPEN):
						FindShortestPath(circular.pt,pt,OPEN))
					{
					  Live_Wire(NULL, pt.x, pt.y, OPEN);
					  Point_Selected(NULL, pt.x, pt.y);
					  UpdateCircular(pt,&orig);
					  add_anchor_point(pt.x, pt.y, &o_contour);
					}
				  }
				}
				switch (detection_mode)
				{
				  case ILW:
					if(iterate_live_wire(&orig, ilw_iterations, ilw_min_pts))
					{
					  wxMessageBox("Iteration failed.");
					  return;
					}
					break;
				  case LSNAKE:
				    iterate_live_snake(&orig, ilw_iterations,
					  lsnake_alpha, lsnake_beta, lsnake_gamma);
					break;
				}
				phase = 3;
				close_contour_code;
			  }
			else /* On next slice backward */
			{
			  dp_anchor_points = 0;
			  assert(ilw_control_points[0]>0);
			  pt.x = ilw_control_point[0][ilw_control_points[0]-1][0];
			  pt.y = ilw_control_point[0][ilw_control_points[0]-1][1];
			  if (Is_Point_Selected_Valid(NULL, pt.x, pt.y)==0)
				InitialPoint_Selected(NULL, pt.x, pt.y);
			  for (int j=0; j<ilw_control_points[0]-1; j++)
			  {
				pt.x = ilw_control_point[0][j][0];
				pt.y = ilw_control_point[0][j][1];
				if (Is_Point_Selected_Valid(NULL, pt.x, pt.y) == 0)
				{
				  if (straight_path?
				      FindStraightPath(NULL, circular.pt,pt,OPEN):
					  FindShortestPath(circular.pt,pt,OPEN))
				  {
					Live_Wire(NULL, pt.x, pt.y, OPEN);
					Point_Selected(NULL, pt.x, pt.y);
					UpdateCircular(pt,&orig);
					add_anchor_point(pt.x, pt.y, &o_contour);
				  }
				}
			  }
			  switch (detection_mode)
			  {
				case ILW:
				  if(iterate_live_wire(&orig, ilw_iterations, ilw_min_pts))
				  {
					wxMessageBox("Iteration failed.");
					return;
				  }
				  break;
				case LSNAKE:
				  iterate_live_snake(&orig, ilw_iterations,
				    lsnake_alpha, lsnake_beta, lsnake_gamma);
				  break;
			  }
			  phase = 3;
			  close_contour_code;
			}
		}
	}
	else if (phase == 1)
	{
	    if( o_contour.last==0 ) 
	    {
	        o_contour.last = -1;
			object_vertex[o_contour.vertex[0].y][o_contour.vertex[0].x] &=offbit[object_number];
			o_contour.img=0;
			phase=0;
			straight_path = false;
			SetStatusText("Trace boundary on slice", 0);
	    }
	    else
	    {
			/*---------------------------------*/
	        /* Prompt to pick point on contour */
			/*---------------------------------*/
	        SetStatusText("Select point upto which the contour should be erased", 0);
	        phase = 2;
			straight_path = false;
	    }
		NumPoints = 0;
		reload();
	}
	else if (phase==3 && sl.slice_index[VolNo][mCavassData->m_sliceNo]==
			o_contour.slice_index)
	{
		if (sl.slice_index[VolNo][mCavassData->m_sliceNo] !=
				o_contour.slice_index)
			return;
   		if (detection_mode == PAINT)
		{
			if (o_contour.last >= 0)
			{
				if (temp_contours.last > 0)
					temp_contours.last = 0;
				copy_ocontour_into_temp_array();
				o_contour.last = -1;
				clear_Vedges_array();
				painting = true;
			}
		}
		else
			set_ilw_control_points();
		Cut_Contours();
		phase = 0;
		SetStatusText("Trace boundary on slice", 0);
		reload();
	}

	if (detection_mode == Mode::INT_DL) {
		auto f = dynamic_cast<Segment2dFrame*>( m_parent_frame );
		f->mIntDLControls->doMiddleDown();
	}
}
//----------------------------------------------------------------------
/** \brief Callback to handle middle mouse button up events. */
void Segment2dCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    log( "OnMiddleUp" );
    if (detection_mode == Mode::INT_DL) {
    	auto f = dynamic_cast<Segment2dFrame*>( m_parent_frame );
		f->mIntDLControls->doMiddleUp();
	}
}
//----------------------------------------------------------------------
/** \brief Callback to handle left mouse button down events. */
void Segment2dCanvas::OnLeftDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events

    log( "OnLeftDown" );
    if (leftToMiddleModifier( e )) {
        log( "  simulate the middle button" );
        OnMiddleDown( e );
        return;
    }
    if (leftToRightModifier( e )) {
        log( "  simulate the right button" );
        OnRightDown( e );
        return;
    }

    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
	int Tx=mTx, Ty=mTy;
	switch (detection_mode)
	{
		case SEL_FEATURES:
			Tx = mTx_f;
			Ty = mTy_f;
			break;
		case REVIEW:
			Tx = mTx_v;
			Ty = mTy_v;
			break;
		case REPORT:
			Tx = mTx_p;
			Ty = mTy_p;
			break;
	}
    const long  wx = dc.DeviceToLogicalX( pos.x ) - Tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - Ty;
    mLastX = wx;
    mLastY = wy;
	if (layout_flag)    return;

    if (detection_mode == Mode::INT_DL) {
        X_Point pt;
        pt.y = orig.tbly[wy - orig.locy];
        pt.x = orig.tblx[wx - orig.locx];
        auto f = dynamic_cast<Segment2dFrame*>( m_parent_frame );
        f->mIntDLControls->doLeftDown( pt.x, pt.y );
        return;
    }

    if (detection_mode == TRAINING) {
		if (train_brush_size == 0)
			switch (training_phase)
			{
				case 0:
					if (temp_contours.last>0 && o_contour.last>=0)
					{
						temp_contours.last = 0;
						copy_ocontour_into_temp_array();
						o_contour.last = -1;
					}
					training_phase = 1;
					SetStatusText("Select point on Boundary", 0);
					// fall through
				case 1:
				    NumPoints=0;
					if (wx-orig.locx>=0 && wy-orig.locy>=0 &&
							wx-orig.locx<orig.w && wy-orig.locy<orig.h)
						draw_and_save_vertices(-1, -1, wx, wy);
					reload();
					break;
				case 2:
					Erase_Segment(wx, wy);
					break;
			}
		else
			switch (training_phase)
			{
				case 0:
					training_phase = 1;
					SetStatusText("Select point on Boundary", 0);
					// fall through
				case 1:
					paint_region(wx, wy);
					break;
				case 2:
					Erase_PxRegion(wx, wy);
					break;
			}
	}
	else if (detection_mode==SEL_FEATURES && !switch_images_flag)
	{
		switch_images_flag = true;
		reload();
	}
	else if (detection_mode==PEEK && wx-orig.locx>=0 && wy-orig.locy>=0 &&
				wx-orig.locx<orig.w && wy-orig.locy<orig.h)
	{
		if (peek_point)
		{
			int (*tpp)[2] = (int (*)[2])
				realloc(peek_point, (num_peek_points+1)*sizeof(*tpp));
			if (tpp == NULL)
				return;
			peek_point = tpp;
		}
		else
			peek_point = (int (*)[2])malloc(sizeof(*peek_point));
		if (peek_point == NULL)
			return;
		peek_point[num_peek_points][0] = orig.tblx[wx - orig.locx];
		peek_point[num_peek_points][1] = orig.tbly[wy - orig.locy];
		num_peek_points++;
		reload();
	}
	else if (phase==0 || phase==1)
	{
		if (((detection_mode==ILW||detection_mode==LSNAKE) &&
				Is_Point_Selected_Valid(&orig, wx, wy)==0) ||
				(detection_mode==PAINT && wx-orig.locx>=0 && wy-orig.locy>=0 &&
				wx-orig.locx<orig.w && wy-orig.locy<orig.h))
		{
			X_Point pt;
			pt.y = orig.tbly[wy - orig.locy];
			pt.x = orig.tblx[wx - orig.locx];
			if (phase == 1)
			{
				if (sl.slice_index[VolNo][mCavassData->m_sliceNo] ==
						o_contour.slice_index)
				{
					if (detection_mode == PAINT)
					{
						if (paint_brush_size == 0)
							draw_and_save_vertices(-1, -1, wx, wy);
						painting = true;
						return;
					}
					else if (straight_path?
							FindStraightPath(&orig, circular.pt, pt, OPEN):
							FindShortestPath(circular.pt, pt, OPEN))
					{
						Live_Wire(&orig, wx, wy, OPEN);
						Point_Selected(&orig, wx, wy);
						UpdateCircular(pt, &orig);
					}
				}
			}
			else if (detection_mode!=PAINT || paint_brush_size==0)
			{
				phase = InitialPoint_Selected(&orig, wx, wy);
				straight_path = false;
				reload();
			}
			if (detection_mode != PAINT)
				add_anchor_point(pt.x, pt.y, &o_contour);
			else if (phase==0 && paint_brush_size)
				paint_mask(wx, wy);
		}
	}
	else if (phase==2 && sl.slice_index[VolNo][mCavassData->m_sliceNo]==
			o_contour.slice_index)
	{
	    int index_pt = 0;

		int x, y;	/* coordinates of specified point (in IMAGE coordinates) */
		x = orig.tblx[wx - orig.locx];
		y = orig.tbly[wy - orig.locy];

		int i, dx, dy;
		float min_dist, temp;

		if(o_contour.last >= 0)
		{
			min_dist = 9999999;
			for(i=0; i<=o_contour.last; i++)
			{
				dx = x-o_contour.vertex[i].x;
				dy = y-o_contour.vertex[i].y;
				temp = dx*dx + dy*dy;

				if(temp < min_dist)
				{
					index_pt = i;
					min_dist = temp;
				}
			}
		}

		int x1, y1, x2, y2;

		if (o_contour.last>=0 && index_pt<=o_contour.last && index_pt>=0)
		{
			x1 = orig.tbl2x[ o_contour.vertex[index_pt].x];
			y1 = orig.tbl2y[ o_contour.vertex[index_pt].y];
			for(i=index_pt+1; i<=o_contour.last; i++)
			{
				x2 = orig.tbl2x[ o_contour.vertex[i].x];
				y2 = orig.tbl2y[ o_contour.vertex[i].y];
				object_vertex[o_contour.vertex[i].y][o_contour.vertex[i].x] &=
					offbit[object_number];
				x1 = x2;
				y1 = y2;
			}
			o_contour.last = index_pt;
			add_anchor_point(o_contour.vertex[index_pt].x,
				o_contour.vertex[index_pt].y, &o_contour);
		}

		if(o_contour.last==-1)  /* all points erased */
			o_contour.img=0;   /* reset */

	    NumPoints=0;
	    if(o_contour.last==-1){  /* all points erased */
	        phase = 0;
	        FreeCircular();
	    }
		reload();
	}
	else if (phase == 3)
	{
		if (sl.slice_index[VolNo][mCavassData->m_sliceNo] !=
				o_contour.slice_index)
			return;
   		if (detection_mode == PAINT)
		{
			if (o_contour.last >= 0)
			{
				if (temp_contours.last > 0)
					temp_contours.last = 0;
				copy_ocontour_into_temp_array();
				o_contour.last = -1;
				clear_Vedges_array();
				painting = true;
			}
		}
		else
			set_ilw_control_points();
		Add_Contours();
		phase = 0;
		SetStatusText("Trace boundary on slice", 0);
		reload();
	}

    SetCursor( wxCursor(wxCURSOR_HAND) );
}
//----------------------------------------------------------------------
/** \brief Callback to handle left mouse button double click events. */
void Segment2dCanvas::OnLeftDClick ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events

    log( "OnLeftDClick" );
    if (leftToMiddleModifier( e )) {
        log( "  simulate the middle button" );
        return;
    }
    if (leftToRightModifier( e )) {
        log( "  simulate the right button" );
        return;
    }

    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
	int Tx=mTx, Ty=mTy;
	switch (detection_mode)
	{
		case SEL_FEATURES:
			Tx = mTx_f;
			Ty = mTy_f;
			break;
		case REVIEW:
			Tx = mTx_v;
			Ty = mTy_v;
			break;
		case REPORT:
			Tx = mTx_p;
			Ty = mTy_p;
			break;
	}
    const long  wx = dc.DeviceToLogicalX( pos.x ) - Tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - Ty;
    mLastX = wx;
    mLastY = wy;
	if (layout_flag)
		return;
	if (detection_mode==ILW && phase==1)
	{
		if (straight_path)
		{
//@@			InitCircular(pt0, &orig);
		}
		straight_path = !straight_path;
		reload();
	}

}
//----------------------------------------------------------------------
/** \brief callback to handle left mouse button up events. */
void Segment2dCanvas::OnLeftUp ( wxMouseEvent& e ) {
	log( "OnLeftUp" );
	if (leftToMiddleModifier( e )) {
		log( "  simulate the middle button" );
		OnMiddleUp( e );
		return;
	}
	if (leftToRightModifier( e )) {
		log( "  simulate the right button" );
		OnRightUp( e );
		return;
	}

	if (detection_mode==SEL_FEATURES && switch_images_flag) {
		switch_images_flag = false;
		reload();
	} else if (detection_mode == INT_DL) {
//        X_Point pt;
//        pt.y = orig.tbly[wy - orig.locy];
//        pt.x = orig.tblx[wx - orig.locx];
        auto f = dynamic_cast<Segment2dFrame*>( m_parent_frame );
        f->mIntDLControls->doLeftUp();
    }
	SetCursor( *wxSTANDARD_CURSOR );
}

void Segment2dCanvas::set_ilw_control_points()
{

	if (dp_anchor_points)
	{
	  if (ilw_control_points[0])
	  {
	    if (sl.slice_index[VolNo][mCavassData->m_sliceNo] <=
		    ilw_control_slice[0])
	    {
	      if (ilw_control_point[1] != ilw_control_point[0])
	        free(ilw_control_point[0]);
	      ilw_control_point[0] = (short unsigned int (*)[2])
	        malloc((dp_anchor_points+1)*sizeof(ilw_control_point[0][0]));
	      ilw_control_points[0] =
	        dp_anchor_points>1 &&
	          dp_anchor_point[dp_anchor_points-1][0]==
	          dp_anchor_point[0][0] &&
	          dp_anchor_point[dp_anchor_points-1][1]==
	          dp_anchor_point[0][1]?
	        dp_anchor_points-1: dp_anchor_points;
	      for (int j=0; j<ilw_control_points[0]; j++)
	      {
	        ilw_control_point[0][j][0] = dp_anchor_point[j][0];
	        ilw_control_point[0][j][1] = dp_anchor_point[j][1];
	      }
	      ilw_control_slice[0] = sl.slice_index[VolNo][mCavassData->m_sliceNo];
	    }
	    else if (sl.slice_index[VolNo][mCavassData->m_sliceNo] >=
			ilw_control_slice[1])
	    {
	      if (ilw_control_point[1] != ilw_control_point[0])
	        free(ilw_control_point[1]);
	      ilw_control_point[1] = (short unsigned int (*)[2])
	        malloc((dp_anchor_points+1)*sizeof(ilw_control_point[0][0]));
	      ilw_control_points[1] =
	        dp_anchor_points>1 &&
	          dp_anchor_point[dp_anchor_points-1][0]==
	          dp_anchor_point[0][0] &&
	          dp_anchor_point[dp_anchor_points-1][1]==
	          dp_anchor_point[0][1]?
	        dp_anchor_points-1: dp_anchor_points;
	      for (int j=0; j<ilw_control_points[1]; j++)
	      {
	        ilw_control_point[1][j][0] = dp_anchor_point[j][0];
	        ilw_control_point[1][j][1] = dp_anchor_point[j][1];
	      }
	      ilw_control_slice[1] = sl.slice_index[VolNo][mCavassData->m_sliceNo];
	    }
	  }
	  else
	  {
	    ilw_control_point[0] = ilw_control_point[1] = (short unsigned (*)[2])
	      malloc((dp_anchor_points+1)*sizeof(ilw_control_point[0][0]));

	    if( dp_anchor_points == 1 )
	    {
	        ilw_control_points[0] = ilw_control_points[1] = 1;
	    }
	    else
	    {
	        ilw_control_points[0] = ilw_control_points[1] =
	          dp_anchor_point[dp_anchor_points-1][0]==
	            dp_anchor_point[0][0] &&
	            dp_anchor_point[dp_anchor_points-1][1]==
	            dp_anchor_point[0][1]?
	          dp_anchor_points-1: dp_anchor_points;
	    }

	    for (int j=0; j<ilw_control_points[0]; j++)
	    {
	      ilw_control_point[0][j][0] = ilw_control_point[1][j][0] =
	        dp_anchor_point[j][0];
	      ilw_control_point[0][j][1] = ilw_control_point[1][j][1] =
	        dp_anchor_point[j][1];
	    }
	    ilw_control_slice[0] = ilw_control_slice[1] =
		  sl.slice_index[VolNo][mCavassData->m_sliceNo];
	  }
    }
}
//----------------------------------------------------------------------
/** \brief callback for paint events. */
void Segment2dCanvas::OnPaint ( wxPaintEvent& e ) {
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
void Segment2dCanvas::paint ( wxDC* dc ) {
    auto f = dc->GetFont();
    f = f.Scale( 0.9 );
    dc->SetFont( f );

    dc->SetTextBackground( *wxBLACK );
    dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(0, 255, 0)) );
	int Tx=mTx, Ty=mTy;
	switch (detection_mode)
	{
		case SEL_FEATURES:
			Tx = mTx_f;
			Ty = mTy_f;
			break;
		case REVIEW:
			Tx = mTx_v;
			Ty = mTy_v;
			break;
		case REPORT:
			Tx = mTx_p;
			Ty = mTy_p;
			break;
	}

        /** \todo replace w/ a wxGrid (like a spreadsheet) in its own window */
	if (detection_mode == REPORT)
	{
        //use a monospaced font so things line up nicely
        wxFont font = dc->GetFont();
        font.SetFamily( wxFONTFAMILY_TELETYPE );
        dc->SetFont( font );

        FILE *fp;
		char name[101];
		int n,			/* index of the current mask on the scene */
			size;		/* size of each mask */
		unsigned char bit;
		unsigned object_area[8];	/* area of each object in pixels */
		float total_object_area[8];	/* total area of each object in pixels */
		wxString line;
		int i, j, counter, new_format;
		int volume, slice;
		int go_flag;
		int ccolumn;	/* indicates current column */
		int line_pos;	/* indicates the line position for the text */
		int column_pos;	/* indicates column position for the text */
		int initial_line_pos,
			initial_column_pos,
			column_width;
		int last_line=-1;	/* index of the last line in a column */
		const int max_lines=100, max_cols=10;
		short line_index[max_lines][max_cols][2]; // [line][column][dimension]
		int maxobj;	/* indicates the maximum object present in the data */
		int maxmaxobj=0; /* indicates the overall # of objects */
		unsigned short short_area[8];
		const int ifh = GetCharHeight()+2;
		const int ifw = GetCharWidth()+1;
		static char units[5][8]={"km", "m", "cm", "mm", "microns"};
		const float pxw=mCavassData->m_vh.scn.xypixsz[0],
			pxh=mCavassData->m_vh.scn.xypixsz[1];
		const int xunit=mCavassData->m_vh.scn.measurement_unit[0];
		int iw, ih;
		GetSize(&iw, &ih);

		/* save last slice that was being worked on */
		save_mask_proc(0);

		for(i=0; i<max_lines; i++)
			for(j=0; j<max_cols; j++)
			{
				line_index[i][j][0] = -1;
				line_index[i][j][1] = -1;
			}

		for(i=0; i<8; i++)
			total_object_area[i] = object_area[i] = 0;

		/* Open file for reading the appropriate mask */
		if( (fp = fopen("object_mask.TMP", "rb")) == NULL)
		{
			wxMessageBox("CAN'T OPEN 'MASK' FILE !");
			return;
		}

		/* Get the last modification date */
		wxString date =
			wxFileName("object_mask.TMP").GetModificationTime().FormatDate();

		initial_line_pos = 20+Ty;
		initial_column_pos = 10+Tx;;

		line_pos = initial_line_pos;
		column_pos = initial_column_pos;

		/* Read the name of the corresponding file */
		if( fread(name, 100, 1, fp) == 1)
		{
			name[100] = 0;
			new_format = strcmp(name, "///")==0;

			/* Draw header of Status Table */
			dc->DrawText("<<<<  Status Report  >>>>", column_pos, line_pos);
			line_pos += ifh;
			dc->DrawText(wxString::Format("File: %s   Last Update: ",
				name+4*new_format)+date, column_pos, line_pos);
			line_pos += ifh;
			dc->DrawText(wxString::Format("sl# :   Object    : Area (%s2)",
				units[xunit]), column_pos, line_pos);
			line_pos += ifh;
		}
		else
		{
			printf("ERROR on Status Report !\n");
			fclose(fp);
			return;
		}

		bit = 0;
		size = mXSize * mYSize;

		/* Get the index of the current slice */
		n = object_mask_slice_index; // index of the slice on the scene (0=1st)

		/* Draw Status Table */
		volume = slice = 0;
		counter = 0;
		go_flag = TRUE;
		ccolumn = 0;
		while(go_flag == TRUE)
		{
			if (VLSeek(fp,
					(100 +(double)counter*(1+(new_format? 32:16)+size))))
				go_flag = FALSE;
			else
			if (fread(&bit, 1, 1, fp)!=1 || (new_format? 
					VReadData((char *)object_area, 4, 8, fp, &i)!=0:
					fread(short_area, 16, 1, fp)!=1))
				go_flag = FALSE;
			else
			{
				if (!new_format)
					for (i=0; i<8; i++)
						object_area[i] = short_area[i];

				/* Find maximum object present on the mask */
				for(i=0, j=0; i<8; i++)
				{
					if( (bit & onbit[i]) > 0)
						j=i;
				}
				maxobj = j + 1;
				if(maxobj > maxmaxobj) maxmaxobj = maxobj;
				column_width = (25+maxobj*7)*ifw;

				if(counter==0)
				{
					line = "--------------------";
					for(i=0; i<maxobj; i++)
						line += "-------";
					dc->DrawText(line, column_pos, line_pos);
					line_pos += ifh;
				}

				/* Create line containing the information about slice */

				if(sl.sd == 3)
					line = wxString::Format("%3d : %s", counter+1,
						n==counter? "[": " ");
				else
					line = wxString::Format("%d,%2d: %s", volume+1, slice+1,
						n==counter? "[": " ");

				/* Build Object MASK Description */
				for(i=0; i<8; i++)
				{
					/* If object is ON */
					if( (bit & onbit[i]) > 0)
						line += wxString::Format("%d", i+1);
					else
					/* If object is OFF */
						line += "-";
				}
				if(n == counter)
					line += "]  : ";
				else
					line += "   : ";

				/* Build Object AREA Description */
				for(i=0; i<maxobj; i++)
				{
					if(object_area[i] > 0)
						line += wxString::Format("%6.1f ",
							object_area[i]*pxw*pxh);
					else
						line += " ----  ";

					total_object_area[i] += object_area[i];
				}

				dc->DrawText(line, column_pos, line_pos);
				line_pos += ifh;

				/* UPDATE LINE POSITION */
				if (line_pos>ih-4*ifh || counter-last_line==max_lines)
				{
					ccolumn ++;
					if (ccolumn == max_cols)
						break;

					last_line = counter;

					line_pos = initial_line_pos + 2*ifh;
					column_pos += column_width;

					dc->DrawText("sl# :   Object    : Area",
						column_pos, line_pos);
					line_pos += ifh;
					line = "--------------------";
					for(i=0; i<maxobj; i++)
						line += "-------";
					dc->DrawText(line, column_pos, line_pos);
					line_pos += ifh;
				}

				line_index[counter-last_line][ccolumn][0] = slice;
				line_index[counter-last_line][ccolumn][1] = volume;

				counter++;

				/* Update the Volume and Slice indeces */
				if(slice == sl.slices[volume]-1)
				{
					slice = 0;
					volume++;
				}
				else
					slice++;
			}
		}

		if(last_line < 0) last_line = counter-1;

		/* Build TOTAL AREAS */
		line_pos += ifh;
		line = "--------------------";
		for(i=0; i<maxmaxobj; i++)
			line += "-------";
		dc->DrawText(line, column_pos, line_pos);
		line_pos += ifh;
		line = "Total Area .....  : ";
		for(i=0; i<maxmaxobj; i++)
		{
			if(total_object_area[i] > 0)
				line+=wxString::Format("%6.1f ", total_object_area[i]*pxw*pxh);
			else
				line += " ----  ";
		}
		line += wxString::Format("%s2", units[xunit]);
		dc->DrawText(line, column_pos, line_pos);
		fclose(fp);

		/* Build TOTAL VOLUMES */
		line_pos += ifh;
		line = "--------------------";
		for(i=0; i<maxmaxobj; i++)
			line += "-------";
		dc->DrawText(line, column_pos, line_pos);
		line_pos += ifh;
		line = "Total Volume .... : ";
		for(i=0; i<maxmaxobj; i++)
		{
			if(total_object_area[i] > 0)
				line += wxString::Format("%6.1f ",
					total_object_area[i]*pxw*pxh*sl.Max_spacing3);
			else
				line += " ----  ";
		}
		line += wxString::Format("%s3", units[xunit]);
		dc->DrawText(line, column_pos, line_pos);
	}
    else if (mBitmaps!=NULL) {  //draw slice(s)
        int  i=0;
        for (int r=0; r<mRows; r++) {
            const int  y = (int)(r*(ceil(mYSize*getScale())+sSpacing));
            for (int c=0; c<mCols; c++) {
                if (mBitmaps[i]!=NULL && mBitmaps[i]->Ok()) {
					const int u = switch_images_flag? c^1: c;
                    const int  x = (int)(u*(ceil(mXSize*getScale())+sSpacing));
                    //draw an image (2d slice)
                    dc->DrawBitmap( *mBitmaps[i], x+Tx, y+Ty );
                    //show the overlay?  (the overlay consists of numbers that indicate the slice)
                    if (mLabels) {
                        const int sliceA = detection_mode==REVIEW?
							review_slice+i: mCavassData->m_sliceNo;
                        //in bounds?
                        if (mCavassData->inBounds(0,0,sliceA)) {
                            //in bounds
                            const wxString  s =
								wxString::Format("(%d/%d)", sliceA+1, mZSize);
                            dc->DrawText( s, x+Tx, y+Ty );
                        } else {
                            //out of bounds
                            const wxString  s = wxString::Format( "(-/-)" );
                            dc->DrawText( s, x+Tx, y+Ty );
                        }
                    }
					if (i==0 && detection_mode!=REVIEW)  //first slice is special
					{
						bool drawing=false;
						if (o_contour.last!= -1 && o_contour.slice_index==
								sl.slice_index[VolNo][mCavassData->m_sliceNo])
							for (int k=0; k<o_contour.last; k++)
							{
								int y1 = y+Ty+
									orig.tbl2y[o_contour.vertex[k].y];
								int x1 = x+Tx+
									orig.tbl2x[o_contour.vertex[k].x];
								int y2 = y+Ty+
									orig.tbl2y[o_contour.vertex[k+1].y];
								int x2 = x+Tx+
									orig.tbl2x[o_contour.vertex[k+1].x];
								dc->DrawLine(x1, y1, x2, y2);
								drawing = true;
							}
						for (int k=0; k<NumPoints-1; k++)
							dc->DrawLine(
								x+Tx+orig.tbl2x[Points[k].x],
								y+Ty+orig.tbl2y[Points[k].y],
								x+Tx+orig.tbl2x[Points[k+1].x],
								y+Ty+orig.tbl2y[Points[k+1].y]), drawing=true;
						if (temp_contours.slice_index==
								sl.slice_index[VolNo][mCavassData->m_sliceNo])
							for (int k=0; k<temp_contours.last; k++)
							{
								int y1 = y+Ty+orig.
									tbl2y[temp_contours.vertex[k].y];
								int x1 = x+Tx+orig.
									tbl2x[temp_contours.vertex[k].x];
								int y2 = y+Ty+orig.
									tbl2y[temp_contours.vertex[k+1].y];
								int x2 = x+Tx+orig.
									tbl2x[temp_contours.vertex[k+1].x];
								dc->DrawLine(x1, y1, x2, y2);
								drawing = true;
							}
						if (drawing && (detection_mode==ILW ||
								detection_mode==LSNAKE))
							draw_anchor_points(*dc, x+Tx, y+Ty);
						if (detection_mode == PEEK)
						{
							const unsigned char *t8 = (unsigned char *)
								mCavassData->getSlice(mCavassData->m_sliceNo);
							const unsigned short *t16 = (unsigned short *)
								mCavassData->getSlice(mCavassData->m_sliceNo);
							for (int pp=0; pp<num_peek_points; pp++)
							{
								const int
									x = peek_point[pp][0],
								    y = peek_point[pp][1],
								    sx = orig.tbl2x[x]+(int)mScale/2+Tx,
									sy = orig.tbl2y[y]+(int)mScale/2+Ty,
									xp = x>=orig.width-1? orig.width-1: x+1,
									yp = y>=orig.height-1? orig.height-1: y+1,
									xm = x<=0? 0: x-1,
									ym = y<=0? 0: y-1;

								/* Show the density and the Object# of the
									pixel */
								wxString text="";
								if( orig.bits > 1)
								{
									int value;
									int vn, vs, ve, vw; /* values for:
										North, South, East and West */
									double gradient;
									if(orig.bits == 8)
									{
										value = t8[y*orig.width+x];
										vn = t8[ym*orig.width + x ];
										vs = t8[yp*orig.width + x ];
										ve = t8[y*orig.width + xp ];
										vw = t8[y*orig.width + xm ];
										gradient = sqrt((double)(vs-vn)*(vs-vn)
											+(double)(ve-vw)*(ve-vw));
									}
									else
									{
										value = t16[y*orig.width+x];
										vn = t16[ym*orig.width + x ];
										vs = t16[yp*orig.width + x ];
										ve = t16[y*orig.width + xp ];
										vw = t16[y*orig.width + xm ];
										gradient = sqrt((double)(vs-vn)*(vs-vn)
											+(double)(ve-vw)*(ve-vw));
									}

									text = wxString::Format(
										"(%3d,%3d): %3d : (%3.0f) ",
										x, y, value, gradient);
								}
								if(object_mask[y*orig.width+x] > 0)
								{
									text += " [";
									for(int i=0; i<8; i++)
									{
										if ((object_mask[y*orig.width+x] &
												onbit[i]) > 0)
											text +=
												wxString::Format(" %d ", i+1);
									}
									text += "]";
								}
								dc->DrawLine(sx-1, sy, sx+2, sy);
								dc->DrawLine(sx, sy-1, sx, sy+2);
								dc->DrawText(text, sx, sy);
							}
						} else if (detection_mode == INT_DL) {
                            //draw box (if necessary)
                            auto tmp = dynamic_cast<Segment2dFrame*>( m_parent_frame );
                            tmp->mIntDLControls->doPaint( dc, x+Tx, y+Ty );
                        }  //end elif INT_DL
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
bool Segment2dCanvas::isLoaded ( const int which ) const {
    if (which==0) {
        if (mCavassData==NULL)    return false;
        const SliceData&  cd = *(SliceData *)mCavassData;
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else if (which==1) {
        if (mCavassData==NULL || mCavassData->mNext==NULL)    return false;
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
int Segment2dCanvas::getCenter ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.m_center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
int Segment2dCanvas::getMax ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.m_max;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
int Segment2dCanvas::getMin ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.m_min;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
int Segment2dCanvas::getNoSlices ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.m_zSize;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
bool Segment2dCanvas::getLabels ( void ) const {
    return mLabels;
}
//----------------------------------------------------------------------
/** \brief    get the overall scale of the displayed image(s).
 *  \returns  the overall scale value.
 */
double Segment2dCanvas::getScale ( void ) const {
    return detection_mode==REVIEW? reviewScale: mScale;
}
//----------------------------------------------------------------------
/** \brief    get the number of the first displayed slice.
 *  \param    which specifies the particular data set (if more than 1 read).
 *  \returns  the number of the first displayed slice.
 */
int Segment2dCanvas::getSliceNo ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
		if (detection_mode == REVIEW)
			return review_slice;
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.m_sliceNo;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
int Segment2dCanvas::getWidth ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.m_width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
bool Segment2dCanvas::getInvert ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.mInvert;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
double Segment2dCanvas::getB ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.getB();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
double Segment2dCanvas::getG ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.getG();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
double Segment2dCanvas::getR ( const int which ) const {
    if (which==0) {
        assert( mCavassData!=NULL );
        const SliceData&  cd = *(SliceData *)mCavassData;
        return cd.getR();
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        const SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
void Segment2dCanvas::setB ( const int which, const double b ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.setB( b );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
void Segment2dCanvas::setCenter ( const int which, const int center ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.m_center = center;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
void Segment2dCanvas::setG ( const int which, const double g ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.setG( g );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        SliceData&  cd = *(SliceData *)(mCavassData->mNext);
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
void Segment2dCanvas::setInvert ( const int which, const bool invert ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.mInvert = invert;
    }
}
//----------------------------------------------------------------------
/** \brief    set the current setting of the overlay state.
 *  \param    overlay is true to turn overlay on; false otherwise.
 */
void Segment2dCanvas::setLabels ( const bool overlay ) { 
    mLabels = overlay;
	mCavassData->setOverlay( overlay );
	Refresh();
}
//----------------------------------------------------------------------
/** \brief  set the current red emphasis value [0.0, ..., 1.0].
 *  \param  which specifies the particular data set (if more than one).
 *  \param  r the current red emphasis value.
 */
void Segment2dCanvas::setR ( const int which, const double r ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.setR( r );
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        SliceData&  cd = *(SliceData *)(mCavassData->mNext);
        cd.setR( r );
    } else {
        assert( 0 );
    }
}
//----------------------------------------------------------------------
/** \brief  set the overall scale (magnification) of the displayed image(s).
 *  \param  scale the overall scale value.
 */
void Segment2dCanvas::setScale   ( const double scale )  {
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

	double old_scale=mScale;
	if (detection_mode == REVIEW)
	{
		int  w, h;
		GetSize( &w, &h );
		reviewScale = scale? scale: 1/9.*w/mCavassData->m_xSize;
		mCols = (int)(w / (mCavassData->m_xSize*reviewScale));
		mRows = (int)(h / (mCavassData->m_ySize*reviewScale));
		if (mCols == 0)
			mCols = 1;
		if (mRows == 0)
			mRows = 1;
	}
	else
	    mScale = scale;
	const int imagew = (int)ceil(scale*orig.width);
	const int imageh = (int)ceil(scale*orig.height);
	int *Tx=&mTx, *Ty=&mTy;
	switch (detection_mode)
	{
		case SEL_FEATURES:
			Tx = &mTx_f;
			Ty = &mTy_f;
			break;
		case REVIEW:
			Tx = &mTx_v;
			Ty = &mTy_v;
			break;
		case REPORT:
			Tx = &mTx_p;
			Ty = &mTy_p;
			break;
	}
	if (FixedPx>=0 && detection_mode!=REVIEW && detection_mode!=REPORT)
	{
		*Tx += (int)rint(FixedPx*(old_scale-scale));
		*Ty += (int)rint(FixedPy*(old_scale-scale));
	}
	else
	{
		if (*Tx < 0)
			*Tx = *Tx*imagew/orig.w;
		if (*Ty < 0)
			*Ty = *Ty*imageh/orig.h;
	}
	set_image_output_size(&orig, imagew, imageh);
	set_image_table(&orig);
    reload();
}
//----------------------------------------------------------------------
/** \brief  set the number of the first displayed slice.
 *  \param  which specifies the particular data set (if more than 1 read).
 *  \param  sliceNo specifies the number of the first displayed slice.
 */
void Segment2dCanvas::setSliceNo ( const int which, const int sliceNo ) {
    if (which==0) {
        assert( mCavassData!=NULL );
		if (detection_mode == REVIEW)
		{
			review_slice = sliceNo;
			return;
		}
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.m_sliceNo = sliceNo;
		if (object_mask_slice_index>=0 && object_mask_slice_index!=sliceNo)
		{
			save_mask_proc(0);  /* saves object mask */
			object_mask_slice_index = sl.slice_index[ VolNo ][ mCavassData->m_sliceNo ];
			/* Load the appropriate object-mask buffer */
			load_object_mask(object_mask_slice_index);
		}
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        SliceData&  cd = *(SliceData *)(mCavassData->mNext);
        cd.m_sliceNo = sliceNo;
    } else {
        assert( 0 );
    }
	assert((ilw_control_points[0]>0) == (ilw_control_points[1]>0));
	Reset_training_proc(0);
	ResetPeekPoints();
}
//----------------------------------------------------------------------
/** \brief  set the current width contrast setting for a particular data set.
 *  \param  which specifies the particular data set (if more than one).
 *  \param  width is the contrast setting value.
 */
void Segment2dCanvas::setWidth ( const int which, const int width ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        SliceData&  cd = *(SliceData *)mCavassData;
        cd.m_width = width;
    } else if (which==1) {
        assert( mCavassData!=NULL && mCavassData->mNext!=NULL );
        SliceData&  cd = *(SliceData *)(mCavassData->mNext);
        cd.m_width = width;
    } else {
        assert( 0 );
    }
}

void Segment2dCanvas::add_vertex_to_o_contour(int x, int y)
{
	if( o_contour.last == MAX_POINTS-1)
	{
		wxMessageBox("NO MORE VERTICES AVAILABLE !");
		return;
	}

	if( o_contour.last > -1 &&
			x == o_contour.vertex[o_contour.last].x &&
			y == o_contour.vertex[o_contour.last].y)
		return;

	if (o_contour.last < 0)
		o_contour.slice_index = sl.slice_index[VolNo][mCavassData->m_sliceNo];

	/* Append point to the contour */
	o_contour.last++;
	o_contour.vertex[o_contour.last].x = x;
	o_contour.vertex[o_contour.last].y = y;

	/* Update the vertex array */
	object_vertex[y][x] |= onbit[object_number];
}

int Segment2dCanvas::draw_and_save_vertices(int x1, int y1, int x2, int y2
	/* pixels in screen co-ordinates */ )
{
    X_Point *pp;
    int nnp;
    int i;
    int ix1, iy1, ix2, iy2; /* image coord after mapping to NW pixel corner */

	if (x1 < 0)
	{
		if (o_contour.last < 0)
		{
			x1 = x2;
			y1 = y2;
		}
		else
		{
			x1 = orig.tbl2x[o_contour.vertex[o_contour.last].x];
			y1 = orig.tbl2y[o_contour.vertex[o_contour.last].y];
		}
	}

    /* Map to image */
    ix1 = orig.tblx[x1-orig.locx];
    iy1 = orig.tbly[y1-orig.locy];
    ix2 = orig.tblx[x2-orig.locx];
    iy2 = orig.tbly[y2-orig.locy];

    /* points calculated are in image pixel coordinates */
    if (VComputeLine(ix1, iy1, ix2, iy2, &pp, &nnp))
	{
		wxMessageBox("Out of memory.");
		return 1;
	}

    /* From here on all coord. are in IMAGE pixel coordinates */
    ix1 = pp[0].x;
    iy1 = pp[0].y;
	add_vertex_to_o_contour( ix1, iy1);
    for(i=1; i<nnp; i++)
    {
		if (o_contour.last == MAX_POINTS-1)
		{
			wxMessageBox("NO MORE VERTICES AVAILABLE !");
			return 1;
		}
        ix2 = pp[i].x;
        iy2 = pp[i].y;
        if( iy1==iy2 || ix1==ix2 ) /* Vertical or Horizontal edges */
			add_vertex_to_o_contour( ix2, iy2);
        else /* diagonal (UP) edges or diagonal (DOWN) edges */
        {
			add_vertex_to_o_contour( ix2, iy1);
			add_vertex_to_o_contour( ix2, iy2);
        } /* end if diag up or down */

        ix1 = ix2;
        iy1 = iy2;
    }
 
    free(pp);
	reload();
    return 0;
}
//----------------------------------------------------------------------
/* Modified: 8/21/08 status, transform, weight initialized by Dewey Odhner.
 */
void Segment2dCanvas::Reset_training_proc(int accept)
{
    int i;

	/** reset object_vertex */
	for(i=0; i<=o_contour.last; i++)
		object_vertex[o_contour.vertex[i].y][o_contour.vertex[i].x]=0;
	o_contour.last=-1; 
	NumPoints = 0;
	clear_temporary_contours_array();
	dp_anchor_points = 0;
	phase = 0;
	straight_path = false;

	memset(region, 0, orig.height*orig.width);
	for (i=0; i<8; i++)
	 if (object_number==i || object_number==8)
	 {
	  if (accept)
	  {

		/* Reset Transform parmeters to DEFAULT values */
		accepted_list[i][0].rmin = temp_list[0].rmin =    (float)0.094;
		accepted_list[i][0].rmax = temp_list[0].rmax =    (float)0.745;
		accepted_list[i][0].rmean = temp_list[0].rmean =  (float)0.39;
		accepted_list[i][0].rstddev = temp_list[0].rstddev = (float)0.126;
    
		accepted_list[i][1].rmin = temp_list[1].rmin =    (float)0.034;
		accepted_list[i][1].rmax = temp_list[1].rmax =    (float)0.596;
		accepted_list[i][1].rmean = temp_list[1].rmean =  (float)0.262;
		accepted_list[i][1].rstddev = temp_list[1].rstddev = (float)0.121;
    
		accepted_list[i][2].rmin = temp_list[2].rmin =    (float)0.01;
		accepted_list[i][2].rmax = temp_list[2].rmax =    (float)0.435;
		accepted_list[i][2].rmean = temp_list[2].rmean =  (float)0.142;
		accepted_list[i][2].rstddev = temp_list[2].rstddev = (float)0.095;
    
		accepted_list[i][3].rmin = temp_list[3].rmin =    (float)0.01;
		accepted_list[i][3].rmax = temp_list[3].rmax =    (float)0.435;
		accepted_list[i][3].rmean = temp_list[3].rmean =  (float)0.0;
		accepted_list[i][3].rstddev = temp_list[3].rstddev = (float)0.077;
    
		accepted_list[i][4].rmin = temp_list[4].rmin =    (float)0.01;
		accepted_list[i][4].rmax = temp_list[4].rmax =    (float)0.435;
		accepted_list[i][4].rmean = temp_list[4].rmean =  (float)0.142;
		accepted_list[i][4].rstddev = temp_list[4].rstddev = (float)0.095;
    
		accepted_list[i][5].rmin = temp_list[5].rmin =    (float)0.01;
		accepted_list[i][5].rmax = temp_list[5].rmax =    (float)0.435;
		accepted_list[i][5].rmean = temp_list[5].rmean =  (float)0.0;
		accepted_list[i][5].rstddev = temp_list[5].rstddev = (float)0.025;
    
		accepted_list[i][6].rmin = temp_list[6].rmin =    (float)0.0;
		accepted_list[i][6].rmax = temp_list[6].rmax =    (float)0.07;
		accepted_list[i][6].rmean = temp_list[6].rmean =  (float)0.0;
		accepted_list[i][6].rstddev = temp_list[6].rstddev = (float)0.125;

	  }
	  if (accept > 1)
	  {

		/* Reset Transform parmeters to DEFAULT values */
		accepted_list[i][0].status = 0;
		accepted_list[i][0].transform = 3;
		accepted_list[i][0].weight = 1;
    
		accepted_list[i][1].status = 0;
		accepted_list[i][1].transform = 3;
		accepted_list[i][1].weight = 1;
    
		accepted_list[i][2].status = 0;
		accepted_list[i][2].transform = 3;
		accepted_list[i][2].weight = 1;
    
		accepted_list[i][3].status = 1;
		accepted_list[i][3].transform = 4;
		accepted_list[i][3].weight = 1;
    
		accepted_list[i][4].status = 0;
		accepted_list[i][4].transform = 3;
		accepted_list[i][4].weight = 1;
    
		accepted_list[i][5].status = 1;
		accepted_list[i][5].transform = 4;
		accepted_list[i][5].weight = 1;
    
		accepted_list[i][6].status = 0;
		accepted_list[i][6].transform = 3;
		accepted_list[i][6].weight = 1;

	  }
      memcpy(temp_list, accepted_list[i], 7*sizeof(struct FeatureList));
	 }
    Initialize_Edge_Masks();
}
//----------------------------------------------------------------------
int Segment2dCanvas::Initialize_Edge_Masks()
{
	int j; 

	if(hzl_edge_mask != NULL){
		for(j=0; j < (mCavassData->m_ySize+1); j++)
			free(hzl_edge_mask[j]);
		free(hzl_edge_mask);
	}
	if(hzl_edge_cont != NULL){
		for(j=0; j < (mCavassData->m_ySize+1); j++)
			free(hzl_edge_cont[j]);
		free(hzl_edge_cont);
	}
	if(vert_edge_mask != NULL){
		for(j=0; j < mCavassData->m_ySize; j++)
			free(vert_edge_mask[j]);
		free(vert_edge_mask);
	}
	if(vert_edge_cont != NULL){
		for(j=0; j < mCavassData->m_ySize; j++)
			free(vert_edge_cont[j]);
		free(vert_edge_cont);
	}

	hzl_edge_mask = (unsigned char **)calloc((mCavassData->m_ySize+1)*sizeof(unsigned char *),1);

	for(j=0; j < (mCavassData->m_ySize+1); j++)
		hzl_edge_mask[j] = (unsigned char *)calloc(mCavassData->m_xSize*sizeof(unsigned char),1); 

	vert_edge_mask = (unsigned char **)calloc(mCavassData->m_ySize*sizeof(unsigned char *),1);

	for(j=0; j < mCavassData->m_ySize; j++)
		vert_edge_mask[j] = (unsigned char *) calloc((mCavassData->m_xSize+1)*sizeof(unsigned char),1); 

	hzl_edge_cont = (unsigned char **)calloc((mCavassData->m_ySize+1)*sizeof(unsigned char *),1);

	for(j=0; j < (mCavassData->m_ySize+1); j++)
		hzl_edge_cont[j] = (unsigned char *)calloc(mCavassData->m_xSize*sizeof(unsigned char),1); 

	vert_edge_cont = (unsigned char **)calloc(mCavassData->m_ySize*sizeof(unsigned char *),1);

	for(j=0; j < mCavassData->m_ySize; j++)
		vert_edge_cont[j] = (unsigned char *) calloc((mCavassData->m_xSize+1)*sizeof(unsigned char),1); 

	return 1;
}
//----------------------------------------------------------------------
/* Clears the temporary array of contours */
void Segment2dCanvas::clear_temporary_contours_array()
{
	if(temp_contours.last > 0)
	{
		free(temp_contours.vertex);
		temp_contours.last = -1;
		temp_contours.slice_index = -1;
		temp_contours.img = NULL;
	}
}

/* Modified: 12/13/96 error code returned by Dewey Odhner */
/* Modified: 12/18/96 statistical calculations corrected by Dewey Odhner */
int Segment2dCanvas::compute_slices(ViewnixHeader *vh, SLICES *sl)
{

	int i,j,k;
	float spc;

    sl->sd = vh->scn.dimension;
 
	sl->width = vh->scn.xysize[0];
	sl->height = vh->scn.xysize[1];
	sl->bits = vh->scn.num_of_bits;
    sl->volumes = (sl->sd==3) ? 1 : vh->scn.num_of_subscenes[0];
    sl->slices = (int *) malloc( sl->volumes * sizeof(int) );
	sl->max_slices = 0;
    sl->min_location3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->max_location3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->min_spacing3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->max_spacing3 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->variable_spacing = (int *) malloc( sl->volumes * sizeof(int) );
    sl->fov3 = (float *) malloc( sl->volumes * sizeof(float) );

	if (sl->slices==NULL || sl->min_location3==NULL ||
			sl->max_location3==NULL || sl->min_spacing3==NULL ||
			sl->max_spacing3==NULL || sl->variable_spacing==NULL ||
			sl->fov3==NULL)
		return 1;

    /* read number of slices in each volume from the header */
    k = (sl->sd==4) ? 1 : 0;
    for(i=0; i<sl->volumes; i++)
    {
        sl->slices[i] = vh->scn.num_of_subscenes[k];
        k++;
    }
 
    /* Build location tree */
    sl->location4 = (float *) malloc( sl->volumes * sizeof(float) );
    sl->location4[0] = 0.0;
    sl->location3 = (float **) malloc( sl->volumes * sizeof(float *) );
    sl->slice_index = (int **) malloc( sl->volumes * sizeof(int *) );
	if (sl->location4==NULL || sl->location3==NULL || sl->slice_index==NULL)
		return 1;
    k = (sl->sd==4) ? sl->volumes : 0;
    for(i=0; i<sl->volumes; i++)
    {
        if(sl->sd==4)
            sl->location4[i] = vh->scn.loc_of_subscenes[i];
 
        sl->location3[i] = (float *) malloc(sl->slices[i] * sizeof(float));
        sl->slice_index[i] = (int *) malloc(sl->slices[i] * sizeof(int));
		if (sl->location3[i]==NULL || sl->slice_index[i]==NULL)
			return 1;
        for(j=0; j<sl->slices[i]; j++)
        {
    		sl->slice_index[i][j] = (sl->sd==4) ? k - sl->volumes : k;
            sl->location3[i][j] = vh->scn.loc_of_subscenes[k];
            k++;
        }
    }

	
 
    /* Get Min and Max LOCATIONS along each of the free dimensions */
	for(i=0; i<sl->volumes; i++)
	{
    	sl->min_location3[i] = sl->location3[i][0];
    	sl->max_location3[i] = sl->location3[i][0];
	}
    sl->Min_location3 = sl->location3[0][0];
    sl->Max_location3 = sl->location3[0][0];
    for(i=0; i<sl->volumes; i++)
    {
        for(j=1; j<sl->slices[i]; j++)
        {
			/* LOCATION */
            if(sl->location3[i][j] < sl->min_location3[i])
				sl->min_location3[i] = sl->location3[i][j];
            if(sl->location3[i][j] > sl->max_location3[i])
				sl->max_location3[i] = sl->location3[i][j];
        }
        if(sl->min_location3[i] < sl->Min_location3)
			sl->Min_location3 = sl->min_location3[i];
        if(sl->max_location3[i] > sl->Max_location3)
			sl->Max_location3 = sl->max_location3[i];
    }
 
    /* Get Global FOVs */
    sl->Fov3 = sl->Max_location3 - sl->Min_location3;

    /* Get Min and Max SPACING along each of the free dimensions */
	for(i=0; i<sl->volumes; i++)
	{
    	sl->min_spacing3[i] = sl->Fov3;
    	sl->max_spacing3[i] = 0;
	}
    sl->min_spacing4 = sl->location4[sl->volumes-1]-sl->location4[0];
	sl->Min_spacing3 = sl->Fov3;
    sl->max_spacing4 = sl->Max_spacing3 = 0;
    for(i=0; i<sl->volumes; i++)
    {
        if(i<sl->volumes-1)
        {
            spc = sl->location4[i+1] - sl->location4[i];
            if( spc < sl->min_spacing4 ) sl->min_spacing4 = spc;
            if( spc > sl->max_spacing4 ) sl->max_spacing4 = spc;
 
        }
 
        for(j=0; j<sl->slices[i]-1; j++)
        {
			/* SPACING */
            spc = sl->location3[i][j+1] - sl->location3[i][j];
            if( spc < sl->min_spacing3[i] ) sl->min_spacing3[i] = spc;
            if( spc > sl->max_spacing3[i] ) sl->max_spacing3[i] = spc;
            if( spc < sl->Min_spacing3 ) sl->Min_spacing3 = spc;
            if( spc > sl->Max_spacing3 ) sl->Max_spacing3 = spc;
        }
    }


	/* For Each Volume */
	sl->total_slices = 0;
    for(i=0; i<sl->volumes; i++)
	{
		/* Total Number of Slices */
		sl->total_slices += sl->slices[i];

		/* Field of View */
        sl->fov3[i] = sl->location3[i][sl->slices[i]-1] - sl->location3[i][0];

		/* Variable Spacing flag */
		sl->variable_spacing[i] = sl->min_spacing3[i]>.99*sl->max_spacing3[i];

		/* Maximum #of Slices */
		if( sl->slices[i] > sl->max_slices) sl->max_slices = sl->slices[i];
	}
 
	/* Global Variable Spacing Flag */
	sl->Variable_spacing = sl->Min_spacing3>.99*sl->Max_spacing3;

	return 0;
}



/*--------------------------------------------------------------------*/
/* This function is called when the "mother" file of the temporary MASK file is not */
/* the same as the chosen input scene. It prompts the user for directions: */
/* - continue and throw away the MASK file and its contents or; */
/* - exit without doing anything (the user probably wants to choose the correct input scene */
/*    Modified: 8/24/95 mask_display_next_previous_image call removed
 *       by Dewey Odhner */
/*    Modified: 4/11/97 graymap file check corrected
 *       by Dewey Odhner */
int Segment2dCanvas::ask_user_what_to_do()
{
	FILE *fp;

	/* Build Question */
	wxString text = wxString("INPUT SCENE IS [")+mCavassData->m_fname+
		"]. STORED MASK FILE REFERS TO ["+mother_filename+
		"]. CONTINUE AND LOSE CONTENTS OF MASK?";
	if (wxMessageBox(text, "Confirm", wxOK|wxCANCEL) == wxOK)
	{
		SetStatusText("Wait ...", 0);
		unlink("object_mask.TMP");
		/* File does not exist, so Create one */
		/* create the file */
		if( (fp = fopen("object_mask.TMP","wb")) == NULL)
		{
			wxMessageBox("CAN'T OPEN TEMPORARY 'MASK' FILE !");
			return(-1);
		}

		strncpy(mother_filename,
		 (const char *)wxFileName(mCavassData->m_fname).GetFullName().c_str(),
		 sizeof(mother_filename)-1);

		/* Write mother_file name onto the file */
		fwrite("///", 4, 1, fp);
		fwrite(mother_filename, 96, 1, fp);
		fflush(fp);
		fclose(fp);

		/* If GreyMap file exist, delete it */
		unlink("greymap.TMP");
	}
	else
	{
		roix = roiy = roiw = roih = -1;

		free_image(&orig);
		return(-1);
	}
	return(1);
}

/*****************************************************************************
 * FUNCTION: get_anchor_points
 * DESCRIPTION: Reads anchor points from the file specified.
 * PARAMETERS:
 *    npoints: The number of points will be stored here.
 *    point: The points will be stored here.  Memory will be allocated if
 *       points are read.
 *    filename: The file to read from
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 9/28/04 by Dewey Odhner.
 *****************************************************************************/
void Segment2dCanvas::get_anchor_points(int *npoints, int (**point)[2], char filename[])
{
	FILE *fp;
	int tp[2], (*tpoint)[2];

	*npoints = 0;
	fp = fopen(filename, "rb");
	if (fp == NULL)
		return;
	while (!feof(fp))
	{
		if (fscanf(fp, "%d %d\n", tp, tp+1) != 2)
		{
			fclose(fp);
			return;
		}
		tpoint = (int (*)[2])(*npoints? realloc(*point, (*npoints+1)*sizeof(tp)):
			malloc((*npoints+1)*sizeof(tp)));
		if (tpoint == NULL)
		{
			fclose(fp);
			return;
		}
		tpoint[*npoints][0] = tp[0];
		tpoint[*npoints][1] = tp[1];
		*point = tpoint;
		(*npoints)++;
	}
}

/*****************************************************************************
 * FUNCTION: draw_anchor_points
 * DESCRIPTION: Draws the anchor points specified by dp_anchor_point,
 *    dp_anchor_points.
 * PARAMETERS:
 *    image: image on which to display
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: dp_anchor_point, dp_anchor_points must be valid.
 *    VTurnOnOverlay should be called first.
 * RETURN VALUE: None
 * EXIT CONDITIONS: memory allocation failures ignored
 * HISTORY:
 *    Created: 10/4/04 by Dewey Odhner.
 *****************************************************************************/
void Segment2dCanvas::draw_anchor_points(wxDC & dc, int img_x, int img_y)
{
	int j, x, y;

	if (sl.slice_index[VolNo][mCavassData->m_sliceNo] != LAST_DP_slice_index)
		return;
	for (j=0; j<dp_anchor_points; j++)
	{
		x = img_x+orig.tbl2x[dp_anchor_point[j][0]];
		y = img_y+orig.tbl2y[dp_anchor_point[j][1]];
		dc.DrawLine(x-2, y-2, x+3, y+3);
		dc.DrawLine(x-2, y+2, x+3, y-3);
	}
}

int Segment2dCanvas::allocControlPoints()
{
	int (*tl)[2];
	const int needed = dp_anchor_points+1;

	if (anchor_point_array_size == 0)
	{
		dp_anchor_point = (int (*)[2])malloc(needed*sizeof(*tl));
		if (dp_anchor_point == NULL)
			return 1;
		ControlPoint = (CPoint *)malloc(needed*sizeof(CPoint));
		if (ControlPoint == NULL)
		{
			free(dp_anchor_point);
			dp_anchor_point = NULL;
			return 1;
		}
		anchor_point_array_size = needed;
	}
	else if (anchor_point_array_size < needed)
	{
		tl = (int (*)[2])realloc(dp_anchor_point, needed*sizeof(*tl));
		if (tl == NULL)
			return 1;
		dp_anchor_point = tl;
		tl= (int(*)[2])realloc(ControlPoint, needed*sizeof(CPoint));
		if (tl == NULL)
			return 1;
		ControlPoint = (CPoint *)tl;
		anchor_point_array_size = needed;
	}
	return 0;
}

/*****************************************************************************
 * FUNCTION: add_anchor_point
 * DESCRIPTION: Adds the specified point to the list of anchor points to be
 *    displayed.
 * PARAMETERS:
 *    coord: coordinates of the point
 *    cont: the contour
 * SIDE EFFECTS: dp_anchor_point, dp_anchor_points will be changed.  Former
 *    anchor points not on the contour will be deleted.
 * ENTRY CONDITIONS: dp_anchor_point, dp_anchor_points must be valid.
 * RETURN VALUE: None
 * EXIT CONDITIONS: memory allocation failures ignored
 * HISTORY:
 *    Created: 10/5/04 by Dewey Odhner.
 *    Modified: 5/6/05 check for realloc corrected by Dewey Odhner.
 *    Modified: 8/19/08 anchor_point_array_size corrected by Dewey Odhner.
 *****************************************************************************/
void Segment2dCanvas::add_anchor_point(int x, int y, OPEN_CONTOUR *cont)
{
	int j;

	if (allocControlPoints())
		return;
	for (j=0; j<dp_anchor_points; j++)
		if (!is_vertex_of_contour(dp_anchor_point[j][0], dp_anchor_point[j][1],
				cont))
			dp_anchor_points = j;
	if (dp_anchor_points && x==dp_anchor_point[dp_anchor_points-1][0] &&
			y==dp_anchor_point[dp_anchor_points-1][1])
		return;
	dp_anchor_point[dp_anchor_points][0] = x;
	dp_anchor_point[dp_anchor_points][1] = y;
	dp_anchor_points++;
}

/*****************************************************************************
 * FUNCTION: iterate_live_snake
 * DESCRIPTION: Performs iterative live snake
 * PARAMETERS:
 *    timg: which image (orig or compl)
 *    ilw_iterations: The number of iterations
 * SIDE EFFECTS: dp_anchor_point, dp_anchor_points, anchor_point, o_contour,
 *    circular will be changed.
 * ENTRY CONDITIONS: o_contour, circular, dp_anchor_point, dp_anchor_points
 *    must be valid.  Entry conditions of Is_Point_Selected_Valid,
 *    FindShortestPath, Live_Wire, Point_Selected, UpdateCircular must be met.
 *    Image size must not chenage between calls.
 * RETURN VALUE: None
 * EXIT CONDITIONS: memory allocation failures ignored
 * HISTORY:
 *    Created: 6/30/05 by Dewey Odhner.
 *    Modified: 12/2/05 bounds check corrected by Dewey Odhner.
 *****************************************************************************/
int Segment2dCanvas::iterate_live_snake(IMAGE *timg, int ilw_iterations,
	double alpha, double beta, double gamma)
{
	int j, k, tmp_anchor_points, tt;
	X_Point pt;
	unsigned char *data8;
	unsigned short *data16;

	if (dp_anchor_points < 2)
		return 0;
	if (Gradient == NULL)
	{
		Gradient = (double **)malloc(timg->height*sizeof(double *));
		if (Gradient == NULL)
			return 1;
		Gradient[0]= (double *)malloc(timg->height*timg->width*sizeof(double));
		if (Gradient[0] == NULL)
		{
			free(Gradient);
			Gradient = NULL;
			return 1;
		}
		for (k=1; k<timg->height; k++)
			Gradient[k] = Gradient[k-1]+timg->width;
	}
	if (allocControlPoints())
		return 1;
	tmp_anchor_points = dp_anchor_points;

	/* compute gradient image */
	data8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
	data16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
	for (k=0; k<timg->height; k++)
		for (j=0; j<timg->width; j++)
		{	double weight[2];
			int lf, rt, up, dn, sh;

			sh = k*timg->width;
			if (k == 0)
			{
				weight[0] = 1/4.;
				weight[1] = 1/3.;
				up = sh;
				dn = sh+timg->width;
			}
			else if (k == timg->height-1)
			{
				weight[0] = 1/4.;
				weight[1] = 1/3.;
				up = sh-timg->width;
				dn = sh;
			}
			else
			{
				weight[0] = 1/6.,
				weight[1] = 1/6.;
				up = sh-timg->width;
				dn = sh+timg->width;
			}
			if (j == 0)
			{
				weight[0] *= 2;
				weight[1] *= 1.5;
				lf = j;
				rt = j+1;
			}
			else if (j == timg->width-1)
			{
				weight[0] *= 2;
				weight[1] *= 1.5;
				lf = j-1;
				rt = j;
			}
			else
			{
				lf = j-1;
				rt = j+1;
			}
			if (timg->bits > 8)
			{
				weight[0] *=
					 (int)data16[up+rt]+data16[sh+rt]+data16[dn+rt]-
					((int)data16[up+lf]+data16[sh+lf]+data16[dn+lf]);
				weight[1] *=
					 (int)data16[dn+lf]+data16[dn+ j]+data16[dn+rt]-
					((int)data16[up+lf]+data16[up+ j]+data16[up+rt]);
			}
			else
			{
				weight[0] *=
				     (int)data8[up+rt]+data8[sh+rt]+data8[dn+rt]-
				    ((int)data8[up+lf]+data8[sh+lf]+data8[dn+lf]);
				weight[1] *=
				     (int)data8[dn+lf]+data8[dn+ j]+data8[dn+rt]-
				    ((int)data8[up+lf]+data8[up+ j]+data8[up+rt]);
			}
			Gradient[k][j] = sqrt(weight[0]*weight[0]+weight[1]*weight[1]);
		}

	for (j=0; j<tmp_anchor_points; j++)
	{
		ControlPoint[j].x = dp_anchor_point[j][0];
		ControlPoint[j].y = dp_anchor_point[j][1];
	}
	ControlPoint[j] = ControlPoint[0];
	SnakeDeformation(Gradient, tmp_anchor_points, ControlPoint,
		alpha, beta, gamma, timg->height, timg->width,
		ilw_iterations<6? ilw_iterations: (ilw_iterations-4)*5);

	NumPoints=0;
	o_contour.last = -1;
	clear_Vedges_array();
	clear_temporary_contours_array();
	Reset_ObjectVertex();
	dp_anchor_points = 0;
	pt.x = (int)rint(ControlPoint[tmp_anchor_points-1].x);
	pt.y = (int)rint(ControlPoint[tmp_anchor_points-1].y);
	tt = Is_Point_Selected_Valid(NULL, pt.x, pt.y) == 0;
	if (tt == 0)
		return 400;
	InitialPoint_Selected(NULL, pt.x, pt.y);
	add_anchor_point(pt.x, pt.y, &o_contour);
	for (j=0; j<tmp_anchor_points-1; j++)
	{
		pt.x = (int)rint(ControlPoint[j].x);
		pt.y = (int)rint(ControlPoint[j].y);
		tt = Is_Point_Selected_Valid(NULL, pt.x, pt.y) == 0;
		if (tt == 0)
			return 400;
        tt = straight_path? FindStraightPath(NULL, circular.pt,pt,OPEN):
			FindShortestPath(circular.pt,pt,OPEN);
		if (tt == 0)
			return 400;
		Live_Wire(NULL, pt.x, pt.y, OPEN);
		Point_Selected(NULL, pt.x, pt.y);
		UpdateCircular(pt, &orig);
		add_anchor_point(pt.x, pt.y, &o_contour);
	}
	return 0;
}

/*****************************************************************************
 * FUNCTION: iterate_live_wire
 * DESCRIPTION: Performs iterative live wire
 * PARAMETERS:
 *    timg: which image (orig or compl)
 *    ilw_iterations: The number of iterations
 *    min_pts: the minimum number of anchor points
 * SIDE EFFECTS: dp_anchor_point, dp_anchor_points, anchor_point, o_contour,
 *    circular will be changed.
 * ENTRY CONDITIONS: o_contour, circular, dp_anchor_point, dp_anchor_points
 *    must be valid.  Entry conditions of Is_Point_Selected_Valid,
 *    FindShortestPath, Live_Wire, Point_Selected, UpdateCircular must be met.
 * RETURN VALUE: 0 on normal completion.
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 5/19/05 by Dewey Odhner.
 *    Modified: 9/27/05 min_pts passed by Dewey Odhner.
 *    Modified: 4/29/08 error code returned by Dewey Odhner.
 *****************************************************************************/
int Segment2dCanvas::iterate_live_wire(IMAGE *timg, int ilw_iterations, int min_pts)
{
	static int array_size, (*tmp_anchor_point)[2], (*midpoint)[2];
	int j, k, nextj, (*tl)[2], tmp_anchor_points, last, tt;
	X_Point pt;

	if (dp_anchor_points < 2)
		return 400;
	tmp_anchor_points = dp_anchor_points<min_pts? min_pts:dp_anchor_points;
	if (array_size == 0)
	{
		tmp_anchor_point = (int (*)[2])malloc(tmp_anchor_points*sizeof(*tl));
		if (tmp_anchor_point == NULL)
			return 1;
		midpoint = (int (*)[2])malloc(tmp_anchor_points*sizeof(*tl));
		if (midpoint == NULL)
		{
			free(tmp_anchor_point);
			return 1;
		}
		array_size = tmp_anchor_points;
	}
	else if (array_size < tmp_anchor_points)
	{
		tl = (int (*)[2])
			realloc(tmp_anchor_point, tmp_anchor_points*sizeof(*tl));
		if (tl == NULL)
			return 1;
		tmp_anchor_point = tl;
		tl = (int (*)[2])realloc(midpoint, tmp_anchor_points*sizeof(*tl));
		if (tl == NULL)
			return 1;
		midpoint = tl;
		array_size = tmp_anchor_points;
	}

	while (dp_anchor_points < min_pts)
	{
		last = nextj = tt = 0;
		for (j=0; j<dp_anchor_points-1; j++)
		{
			tt = contour_length(dp_anchor_point[j][0], dp_anchor_point[j][1],
				dp_anchor_point[j+1][0], dp_anchor_point[j+1][1], &o_contour);
			if (tt == 0)
				return 400;
			if (tt > last)
			{
				last = tt;
				nextj = j;
			}
		}
		if (tt > contour_length(o_contour.vertex[0].x, o_contour.vertex[0].y,
				dp_anchor_point[0][0], dp_anchor_point[0][1], &o_contour))
			tt=find_contour_midpoint(tmp_anchor_point[0],tmp_anchor_point[0]+1,
				dp_anchor_point[nextj][0], dp_anchor_point[nextj][1],
				dp_anchor_point[nextj+1][0], dp_anchor_point[nextj+1][1],
				&o_contour);
		else
		{
			tt=find_contour_midpoint(tmp_anchor_point[0],tmp_anchor_point[0]+1,
				o_contour.vertex[0].x, o_contour.vertex[0].y,
				dp_anchor_point[0][0], dp_anchor_point[0][1], &o_contour);
			nextj = -1;
		}
		if (tt == 0)
			return 400;
		add_anchor_point(tmp_anchor_point[0][0], tmp_anchor_point[0][1],
			&o_contour);
		for (j=dp_anchor_points-2; j>nextj; j--)
		{
			dp_anchor_point[j+1][0] = dp_anchor_point[j][0];
			dp_anchor_point[j+1][1] = dp_anchor_point[j][1];
		}
		dp_anchor_point[j+1][0] = tmp_anchor_point[0][0];
		dp_anchor_point[j+1][1] = tmp_anchor_point[0][1];
	}
	for (k=0; k<ilw_iterations; k++)
	{
		for (j=0; j<tmp_anchor_points; j++)
		{
			tmp_anchor_point[j][0] = dp_anchor_point[j][0];
			tmp_anchor_point[j][1] = dp_anchor_point[j][1];
			if (j < tmp_anchor_points-1)
			{
				tt = find_contour_midpoint(midpoint[j], midpoint[j]+1,
					tmp_anchor_point[j][0], tmp_anchor_point[j][1],
					dp_anchor_point[j+1][0], dp_anchor_point[j+1][1],
					&o_contour);
				if (tt == 0)
					return 400;
			}
		}
		pt.x = dp_anchor_point[j-1][0];
		pt.y = dp_anchor_point[j-1][1];
		tt = straight_path?
			FindStraightPath(timg, pt, o_contour.vertex[0], CLOSE):
			FindShortestPath(pt, o_contour.vertex[0], CLOSE);
		if (tt == 0)
			return 400;
		tt = Close_Contour(timg) == 0;
		if (tt == 0)
			return 400;
		copy_ocontour_into_temp_array();
		last = o_contour.last;
		tt = find_contour_midpoint(midpoint[j-1], midpoint[j-1]+1,
			tmp_anchor_point[j-1][0], tmp_anchor_point[j-1][1],
			o_contour.vertex[last].x,
			o_contour.vertex[last].y, &o_contour);
		if (tt == 0)
			return 400;
		NumPoints=0;
		o_contour.last = -1;
        clear_Vedges_array();
        clear_temporary_contours_array();
        Reset_ObjectVertex();
		dp_anchor_points = 0;
		pt.x = midpoint[tmp_anchor_points-1][0];
		pt.y = midpoint[tmp_anchor_points-1][1];
		tt = Is_Point_Selected_Valid(NULL, pt.x, pt.y) == 0;
		if (tt == 0)
			return 400;
		InitialPoint_Selected(NULL, pt.x, pt.y);
		add_anchor_point(pt.x, pt.y, &o_contour);
		for (j=0; j<tmp_anchor_points-1; j++)
		{
			pt.x = midpoint[j][0];
			pt.y = midpoint[j][1];
			tt = Is_Point_Selected_Valid(NULL, pt.x, pt.y) == 0;
			if (tt == 0)
				return 400;
            tt = straight_path? FindStraightPath(NULL, circular.pt,pt,OPEN):
				FindShortestPath(circular.pt,pt,OPEN);
			if (tt == 0)
				return 400;
            Live_Wire(NULL, pt.x, pt.y, OPEN);
            Point_Selected(NULL, pt.x, pt.y);
            UpdateCircular(pt, &orig);
            add_anchor_point(pt.x, pt.y, &o_contour);
		}
	}
	return 0;
}

/*****************************************************************************
 * FUNCTION: is_vertex_of_contour
 * DESCRIPTION: Checks whether a point is one of the contour vertices.
 * PARAMETERS:
 *    x, y: coordinates of the point
 *    cont: the contour
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: 
 * RETURN VALUE: non-zero if the point is one of the contour vertices
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 10/4/04 by Dewey Odhner.
 *****************************************************************************/
int Segment2dCanvas::is_vertex_of_contour(int x, int y, OPEN_CONTOUR *cont)
{
	int j;

	for (j=0; j<=cont->last; j++)
		if (cont->vertex[j].x==x && cont->vertex[j].y==y)
			return (TRUE);
	return (FALSE);
}

/*****************************************************************************
 * FUNCTION: find_contour_midpoint
 * DESCRIPTION: Finds the midpoint between two vertices on a contour.
 * PARAMETERS:
 *    x, y: coordinates of the midpoint are stored here.
 *    x1, y1, x2, y2: coordinates of two vertices on a contour. (x1, y1) must
 *       come before (x2, y2) on the contour.
 *    cont: the contour
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: TRUE if (x1, y1) comes before (x2, y2) on the contour.
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 5/19/05 by Dewey Odhner.
 *****************************************************************************/
int Segment2dCanvas::find_contour_midpoint(int *x, int *y, int x1, int y1,
	int x2, int y2, OPEN_CONTOUR *cont)
{
	int j, j1, j2;

	if (cont->last < 0)
		return (FALSE);
	for (j1=0; cont->vertex[j1].x!=x1||cont->vertex[j1].y!=y1; j1++)
		if (j1 == cont->last)
			return (FALSE);
	for (j2=j1; cont->vertex[j2].x!=x2||cont->vertex[j2].y!=y2; j2++)
		if (j2 == cont->last)
			return (FALSE);
	j = j1+j2;
	if (j & 1)
		j |= 2;
	j /= 2;
	*x = cont->vertex[j].x;
	*y = cont->vertex[j].y;
	return (TRUE);
}

/*****************************************************************************
 * FUNCTION: contour_length
 * DESCRIPTION: Finds the length between two vertices on a contour.
 * PARAMETERS:
 *    x1, y1, x2, y2: coordinates of two vertices on a contour. (x1, y1) must
 *       come before (x2, y2) on the contour.
 *    cont: the contour
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The length between two vertices on a contour, or -1.
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 9/20/05 by Dewey Odhner.
 *****************************************************************************/
int Segment2dCanvas::contour_length(int x1, int y1, int x2, int y2, OPEN_CONTOUR *cont)
{
	int j1, j2;

	if (cont->last < 0)
		return (-1);
	for (j1=0; cont->vertex[j1].x!=x1||cont->vertex[j1].y!=y1; j1++)
		if (j1 == cont->last)
			return (-1);
	for (j2=j1; cont->vertex[j2].x!=x2||cont->vertex[j2].y!=y2;)
		if (++j2 == cont->last)
			return (-1);
	return (j2-j1);
}

/*****************************************************************************
 * FUNCTION: initdliststructure
 * DESCRIPTION: Initializes doubly-linked list for circular data
 * structure used in LWOF. 
 * PARAMETERS:
 * L: pointer to a doubly-linked list.
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: NONE
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

void Segment2dCanvas::initdliststructure(dliststructuretype *L)
{
	L->begin = NULL;
	L->numberelements = 0;
}

/*****************************************************************************
 * FUNCTION: vertexcoordinate
 * DESCRIPTION: Converts from linear position of a vertex to (x,y)
 * coordinates in LWOF.
 * PARAMETERS:
 * x,y: x,y coordinates of the vertex
 * vertex: linear vertex's position in the image array
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: x,y coordinates
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

void Segment2dCanvas::vertexcoordinate(int *x, int *y, int vertex)
{
  *(y) = (int) (vertex / circular.w);
  *(x) = (int) (vertex % circular.w);
}

/*****************************************************************************
 * FUNCTION: vertexposition
 * DESCRIPTION: Converts from (x,y) coordinates to linear position of
 * a vertex in LWOF.
 * PARAMETERS:
 * vertex: x,y coordinates of the vertex
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: tblcc must be initialized.
 * RETURN VARIABLE: linear position of the vertex in the image array
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

int Segment2dCanvas::vertexposition(X_Point *vertex)
{
  int vpos;  
  vpos = vertex->x + tblcc[vertex->y];
  return (vpos); 
}

/*****************************************************************************
 * FUNCTION: InitCircular
 * DESCRIPTION: Initializes circular data structure used in LWOF.
 * PARAMETERS:
 * pt0: first point selected on the boundary.
 * timg: working image
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: Initial point is valid
 * RETURN VARIABLE: NONE
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

int Segment2dCanvas::InitCircular(X_Point pt0, IMAGE *timg)
{
  int i,N,row,col;
  X_Point u; 

  circular.w  = timg->width+1;
  circular.pt.x = pt0.x;
  circular.pt.y = pt0.y;
  circular.qsize = (int)Imax+1;
  SumCost = OUTSIDE_MASK - Circular_qsize; 

  N = (timg->width+1)*(timg->height+1);
  if (circular.processed)
	free(circular.processed);
  circular.processed = (char *) calloc(1, N);
  if (circular.ccost)
    free(circular.ccost);
  circular.ccost = (dliststructuretype *)
    calloc(sizeof(dliststructuretype),Circular_qsize);
  if (circular.ptr_vtx)
	free(circular.ptr_vtx);
  circular.ptr_vtx = (dlisttype *) calloc(sizeof(dlisttype),N);

  if (circular.processed==NULL || 
      circular.ptr_vtx==NULL ||
      circular.ccost==NULL) {
    FreeCircular();
    return(0);
  }

  for (i=0;i<N;i++) {
    vertexcoordinate(&col,&row,i);
    dir_vtx[row][col] = 1; /* 1 means it has not been considered yet */
    circular.processed[i] = FALSE;
    cc_vtx[row][col] = (unsigned long)SumCost; 
  }
  for (i=0;i<Circular_qsize;i++) {
    initdliststructure(&(circular.ccost[i]));
  }
  circular.initial = 0;
  circular.initial_ccost = 0;

  u.x = pt0.x; u.y = pt0.y; 
  i = vertexposition(&u);
  dir_vtx[pt0.y][pt0.x] = 0;
  circular_insert(&u,0);

  return(1);

}

/*****************************************************************************
 * FUNCTION: UpdateCircular
 * DESCRIPTION: Updates cum. costs in the circular data structure used in LWOF.
 * PARAMETERS:
 * pt0: current point selected on the boundary.
 * timg: working image
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: Selected point is valid
 * RETURN VARIABLE: NONE
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

void Segment2dCanvas::UpdateCircular(X_Point pt0, IMAGE *timg)
{
  int i,z,N,row,col;
  X_Point u;

  N = (timg->width+1)*(timg->height+1);

  for (i=0;i<N;i++) {
    vertexcoordinate(&col,&row,i);
    dir_vtx[row][col] = 1;
    circular.processed[i] = FALSE;
    cc_vtx[row][col] = (unsigned long)SumCost; 
   }
  for (i=0;i<Circular_qsize;i++) {
    initdliststructure(&(circular.ccost[i]));
  }
 
  for(i=0; i < o_contour.last; i++){
    u.x = o_contour.vertex[i].x; u.y = o_contour.vertex[i].y; 
    z = vertexposition(&u); 
    circular.processed[z] = TRUE;
    cc_vtx[u.y][u.x] = OUTSIDE_MASK; 
  }

  circular.initial = 0;
  circular.initial_ccost = 0;
  circular.pt.x = pt0.x;
  circular.pt.y = pt0.y;

  u.x = pt0.x;  u.y = pt0.y;
  z = vertexposition(&u); 
  dir_vtx[u.y][u.x] = 0;
  circular.processed[z] = FALSE; 
  circular_insert(&u,0); 
}

/*****************************************************************************
 * FUNCTION: FreeCircular
 * DESCRIPTION: Free memory in circular data structure used in LWOF.
 * PARAMETERS: NONE
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: NONE
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

void Segment2dCanvas::FreeCircular()
{
    if (circular.processed) free(circular.processed);
	circular.processed = NULL;
    if (circular.ccost) free(circular.ccost);
	circular.ccost = NULL;
    if (circular.ptr_vtx) free(circular.ptr_vtx);
	circular.ptr_vtx = NULL;
}

/*****************************************************************************
 * FUNCTION: FindShortestPath
 * DESCRIPTION: Finds shortest-path from pt0 to pt1 in LWOF.
 * PARAMETERS: 
 * pt0: last point selected on the boundary
 * pt1: mouse position or current point selected on the boundary.
 * flag: if CLOSE, prepares to compute the last contour segment. If
 * OPEN, just computes the current segment.
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: indicates whether or not the computation was  succeeded
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 *    Modified: 4/22/08 check for image edges by Dewey Odhner.
 *****************************************************************************/

int Segment2dCanvas::FindShortestPath(X_Point pt0, X_Point pt1, int flag)
{
  unsigned long mincc; 

  /* ---------------------------------------- */
  int destination,source,pos_u,pos_v;
  X_Point u,v;

  /* ---------------------------------------- */
  /*  unsigned long dist; */
  u.x = pt0.x;  u.y = pt0.y;
  source = vertexposition(&u);
  u.x = pt1.x;  u.y = pt1.y;  
  destination = vertexposition(&u);


  if(destination==source)
    return(0); 

                          /* Condition for computing the last contour
                             segment */

 if(flag == CLOSE){
   dir_vtx[pt1.y][pt1.x] = 1;
   circular.processed[destination] = FALSE;
   cc_vtx[pt1.y][pt1.x] = (unsigned long)SumCost; 
 }

  if(cc_vtx[pt1.y][pt1.x]==OUTSIDE_MASK){  /* point is on the contour */ 
    return(0); 
  }

  while (!circular.processed[destination]) {

    if (circular_remove_first(&u)==FALSE) {
      return(0); 
    }
      
    pos_u = u.x+*(tblcc+u.y);   
    circular.processed[pos_u] = TRUE;

    /* WEST */

    v.x = u.x+1;   v.y = u.y;      pos_v = pos_u + 1;
    if (u.x<orig.width && !circular.processed[pos_v]) {
	  /* neighbor has not been visited yet */
	  mincc = cc_vtx[u.y][u.x];
	  mincc += hzl_sign_buffer[u.y][u.x]==NEG1? (unsigned long)Imax: hzl_cost[u.y][u.x];
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = WEST;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */ 
        if (cc_vtx[v.y][v.x] > mincc) { /* update position in the queue */
          dir_vtx[v.y][v.x] = WEST;
          circular_decrement(&v,mincc);
        }
      }
    }

    /* EAST */
    v.x = u.x-1;   v.y = u.y;      pos_v = pos_u - 1;
    if (u.x>0 && !circular.processed[pos_v]) {
	  /* neighbor has not been processed yet */
	  mincc = cc_vtx[u.y][u.x];
      mincc += hzl_sign_buffer[v.y][v.x]==1? (unsigned long)Imax: hzl_cost[v.y][v.x];
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = EAST;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */ 
        if (cc_vtx[v.y][v.x] > mincc) { /* update position in the queue */
          dir_vtx[v.y][v.x] = EAST;
          circular_decrement(&v,mincc);
        }
      }
    }

    /* SOUTH */
    v.x = u.x;   v.y = u.y-1;    pos_v = pos_u - circular.w;
    if (u.y>0 && !circular.processed[pos_v]) {
	  /* neighbor has not been visited yet */
	  mincc = cc_vtx[u.y][u.x];
	  mincc +=
		vert_sign_buffer[v.y][v.x]==NEG1? (unsigned long)Imax: vert_cost[v.y][v.x];
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = SOUTH;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */ 
        if (cc_vtx[v.y][v.x] > mincc) {  /* update position in the queue */
          dir_vtx[v.y][v.x] = SOUTH;
          circular_decrement(&v,mincc);
        }
      }
    }

    /* NORTH */
    v.x = u.x;   v.y = u.y+1;    pos_v = pos_u + circular.w;
    if (u.y<orig.height && !circular.processed[pos_v]) {
	  /* neighbor has not been visited yet */
	  mincc = cc_vtx[u.y][u.x];
	  mincc+= vert_sign_buffer[u.y][u.x]==1? (unsigned long)Imax:vert_cost[u.y][u.x];
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = NORTH;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */
        if (cc_vtx[v.y][v.x] > mincc) {  /* update position in the queue */
          dir_vtx[v.y][v.x] = NORTH;
          circular_decrement(&v,mincc);
        }
      }
    }

  }

  return(1); 

}

/************************************************************************
 *                                                                      *
 *      Function        : cvComputeLine                                  *
 *      Description     : This function will compute the points         *
 *                        coordinates of the line between the two       *
 *                        specified points. unction cvComputeLine also   *
 *                        allocates the memory space for buf and returns*
 *                        the x, y coordinates of each point along      *
 *                        the line between two specified points to the  *
 *                        *points, the number of points stored to buf to*
 *                        npoints. If the first point is the same as the*
 *                        second point, this function will return the   *
 *                        x, y coordinates of the first point to        *
 *                        the *points, and 1 to the nptr. You can use C *
 *                        function free to free points.                 *
 *                        If the memory allocation error happens, this  *
 *                        function will return 1. If there is no error, *
 *                        this function will return 0.                  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  x1, y1 - Specifies the first point of the    *
 *                              line.                                   *
 *                         x2, y2 - Specifies the second point of the   *
 *                              line.                                   *
 *                         points - Returns an array of points(x,y)     *
 *                              along the line to struct X_Point.        *
 *                         npoints - Returns the number of the points   *
 *                              along the line.                         *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on September 29,1989 by Hsiu-Mei Hung.*
 *                        Modified on March 10, 1993 by R.J.Goncalves.  *
 *                                                                      *
 ************************************************************************/
int cvComputeLine ( int x1, int y1, int x2, int y2, X_Point** points,
    int* npoints )
{
 
    int dx, dy, incr1, incr2, incr_y, incr_x;
    int x, y, d, xend, yend;
    int max_delta;
    int index;
    int index_incr, index_end;
 
 
    if (y2<y1 && x2>x1) incr_y = -1;
    else
    if (x2<x1 && y2>y1) incr_y = -1;
    else incr_y = 1;
 
    dx = abs(x2-x1);
    dy = abs(y2-y1);
    if(dx>=dy)
    {
        max_delta = dx;
        d = 2*dy-dx;
        incr1 = 2*dy;
        incr2 = 2*(dy-dx);
 
        if (x1>x2)
        {
            x = x2;
            y = y2;
            xend = x1;
			yend = y1;
        }
        else
        {  
            x = x1;
            y = y1;
            xend = x2;
			yend = y2;
        }
 
    }
    else
    {  
        max_delta = dy;
        d = 2*dx-dy;
        incr1 = 2*dx;
        incr2 = 2*(dx-dy);
 
 
        if (y1>y2)
        {
            x = x2;
            y = y2;
            yend = y1;
			xend = x1;
        }
        else   
        {
            x = x1;
            y = y1;
            yend = y2;
			xend = x2;
        }
    }

    *npoints = 0;
    *points=(X_Point *)malloc((dx+dy+1)*sizeof(X_Point)) ;
    if (*points == NULL) return(1) ;
    *npoints = dx+dy+1;

    index = 0;
    index_incr = 1;
	index_end = dx+dy;
    if( ( dy>dx && y2<y1) || (dy<=dx && x2<x1) )
    {
        index = dx+dy;
        index_incr = -1;
		index_end = 0;
    }

    (*points)[index].x=x ;
    (*points)[index].y=y ;

    if(dx>=dy)
    {
        while (index != index_end)
        {
            x++;
            index += index_incr;
            if (d<0)
                d += incr1;
            else
            {
                (*points)[index].x=x ;
                (*points)[index].y=y ;
				index += index_incr;
                y += incr_y;
                d += incr2;
            }
            (*points)[index].x=x ;
            (*points)[index].y=y ;
        }
    }
    else
    {
        incr_x = incr_y;
        while (index != index_end)
        {
            y++;
            index += index_incr;
            if (d<0)
                d += incr1;
            else
            {
                (*points)[index].x=x ;
                (*points)[index].y=y ;
				index += index_incr;
                x += incr_x;
                d += incr2;
            }
            (*points)[index].x=x ;
            (*points)[index].y=y ;
        }
    }

    return(0);
}

int Segment2dCanvas::FindStraightPath(IMAGE *timg, X_Point pt0, X_Point pt1, int flag)
{
  unsigned long mincc;

  /* ---------------------------------------- */
  int destination,source,pos_u,pos_v, nnp, pti;
  X_Point u,v, w, *pp;

  /* ---------------------------------------- */
  u.x = pt0.x;  u.y = pt0.y;
  source = vertexposition(&u);
  u.x = pt1.x;  u.y = pt1.y;  
  destination = vertexposition(&u);


  if(destination==source)
    return(0); 

  InitCircular(pt0, timg? timg: &orig);

  if(flag == CLOSE){
    dir_vtx[pt1.y][pt1.x] = 1;
    circular.processed[destination] = FALSE;
    cc_vtx[pt1.y][pt1.x] = (unsigned long)SumCost; 
  }

  if(cc_vtx[pt1.y][pt1.x]==OUTSIDE_MASK){  /* point is on the contour */ 
    return(0); 
  }
  if (cvComputeLine(pt0.x, pt0.y, pt1.x, pt1.y, &pp, &nnp))
  {
	wxMessageBox("Out of memory.");
	return 0;
  }
  for (pti=1; pti<nnp; pti++)
    if (cc_vtx[pp[pti].x][pp[pti].y] == OUTSIDE_MASK)
	{
	  free(pp);
	  return 0;
	}

  pti = 0;
  while (!circular.processed[destination]) {

    if (circular_remove_first(&u)==FALSE) {
	  free(pp);
      return 0;
    }
    pos_u = u.x+*(tblcc+u.y);   
    circular.processed[pos_u] = TRUE;

    bool on_line = pti<nnp-1 && u.x==pp[pti].x && u.y==pp[pti].y;

	if (on_line)
	  w = pp[++pti];

    /* WEST */

    v.x = u.x+1;   v.y = u.y;      pos_v = pos_u + 1;
    if (u.x<orig.width && !circular.processed[pos_v]) {
	  /* neighbor has not been visited yet */
	  mincc = cc_vtx[u.y][u.x];
	  if (!on_line || v.x!=w.x || v.y!=w.y)
	    mincc += (unsigned long)Imax;
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = WEST;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */ 
        if (cc_vtx[v.y][v.x] > mincc) { /* update position in the queue */
          dir_vtx[v.y][v.x] = WEST;
          circular_decrement(&v,mincc);
        }
      }
    }

    /* EAST */
    v.x = u.x-1;   v.y = u.y;      pos_v = pos_u - 1;
    if (u.x>0 && !circular.processed[pos_v]) {
	  /* neighbor has not been processed yet */
	  mincc = cc_vtx[u.y][u.x];
	  if (!on_line || v.x!=w.x || v.y!=w.y)
	    mincc += (unsigned long)Imax;
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = EAST;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */ 
        if (cc_vtx[v.y][v.x] > mincc) { /* update position in the queue */
          dir_vtx[v.y][v.x] = EAST;
          circular_decrement(&v,mincc);
        }
      }
    }

    /* SOUTH */
    v.x = u.x;   v.y = u.y-1;    pos_v = pos_u - circular.w;
    if (u.y>0 && !circular.processed[pos_v]) {
	  /* neighbor has not been visited yet */
	  mincc = cc_vtx[u.y][u.x];
	  if (!on_line || v.x!=w.x || v.y!=w.y)
	    mincc += (unsigned long)Imax;
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = SOUTH;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */ 
        if (cc_vtx[v.y][v.x] > mincc) {  /* update position in the queue */
          dir_vtx[v.y][v.x] = SOUTH;
          circular_decrement(&v,mincc);
        }
      }
    }

    /* NORTH */
    v.x = u.x;   v.y = u.y+1;    pos_v = pos_u + circular.w;
    if (u.y<orig.height && !circular.processed[pos_v]) {
	  /* neighbor has not been visited yet */
	  mincc = cc_vtx[u.y][u.x];
	  if (!on_line || v.x!=w.x || v.y!=w.y)
	    mincc += (unsigned long)Imax;
      if (dir_vtx[v.y][v.x] == 1) { /* neighbor is out of the queue */
        dir_vtx[v.y][v.x] = NORTH;
        circular_insert(&v,mincc);
      } else { /* neighbor is already in the queue */
        if (cc_vtx[v.y][v.x] > mincc) {  /* update position in the queue */
          dir_vtx[v.y][v.x] = NORTH;
          circular_decrement(&v,mincc);
        }
      }
    }

  }

  free(pp);
  return(1); 

}

/*****************************************************************************
 * FUNCTION: circular_remove_first
 * DESCRIPTION: Removes the first vertex from the queue in LWOF.
 * PARAMETERS: 
 * vertex: pointer to the x,y coordinates of the vertex to be removed
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: x,y coordinates of the vertex
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

int Segment2dCanvas::circular_remove_first(X_Point *vertex)
{
  dliststructuretype *L;
  dlisttype *first;
  int x,y;

  if (circular_update() != -1) {
    L = &circular.ccost[circular.initial];
    first = L->begin;
    L->begin = L->begin->next;
    if (L->begin!=NULL) {
      L->begin->previous = NULL;
    }
    L->numberelements--;

    vertexcoordinate(&x,&y,first-circular.ptr_vtx);
    vertex->x = x; 
    vertex->y = y; 
    return(TRUE);
  } else {
    /* queue is empty */
    return(FALSE);
  }
}

/*****************************************************************************
 * FUNCTION: circular_update
 * DESCRIPTION: Updates index to the next bucket with a minimum cum. cost
 * vertex in LWOF.
 * PARAMETERS: NONE
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: returns index.
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

int Segment2dCanvas::circular_update()
{
  int fim;
  int i;
  
  if (circular.ccost[circular.initial].numberelements == 0) {
    fim = circular.initial;
    for (i=(circular.initial+1)%Circular_qsize; 
	 i!=fim && circular.ccost[i].numberelements == 0;
	 i = (i+1)%Circular_qsize);
    if (i!=fim) {
      circular.initial_ccost += ((Circular_qsize+i-circular.initial)% Circular_qsize);
      circular.initial = i;
      return(i);
    } else {
      /* empty queue */
      circular.initial = 0;
      return(-1);
    }
  } else {
    return(circular.initial);
  }
}

/*****************************************************************************
 * FUNCTION: circular_decrement
 * DESCRIPTION: Updates position of a vertex in the circular queue in LWOF.
 * PARAMETERS: 
 * vertex: vertex whose position should be updated
 * novocusto: new cost value.
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: NONE
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

int Segment2dCanvas::circular_decrement(X_Point *vertex, unsigned long novocusto)
{
  int remover,i;
  unsigned long custoremover;
  dliststructuretype *L;
  dlisttype *specific;

  remover = vertexposition(vertex);
  custoremover = cc_vtx[(*vertex).y][(*vertex).x];
  i = (int)(((custoremover - circular.initial_ccost)+circular.initial)%Circular_qsize);
  
  /* Removes the element */ 

  L = &(circular.ccost[i]);
  specific = &circular.ptr_vtx[remover];
  if (specific->previous == NULL) {
    L->begin = specific->next;
    if (specific->next) (specific->next)->previous = NULL;
  } else {
    if (specific->next == NULL) {
      (specific->previous)->next = NULL;
    } else {
      (specific->previous)->next = specific->next;
      (specific->next)->previous = specific->previous;
    }
  }
  L->numberelements--;

  /* Insert the element in the new position */

  circular_insert(vertex,novocusto); 
  return(TRUE);
}

/*****************************************************************************
 * FUNCTION: circular_insert
 * DESCRIPTION: Inserts vertex in the circular queue in LWOF.
 * PARAMETERS: 
 * vertex: vertex to be inserted
 * cost: current cumulative cost
 * SIDE EFFECTS: NONE
 * ENTRY CONDITIONS: NONE
 * RETURN VARIABLE: NONE
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by Alexandre Falcao in 28/02/2001.
 * Modified: mm/dd/yyyy by programmer name.
 *****************************************************************************/

int Segment2dCanvas::circular_insert(X_Point *vertex, unsigned long cost)
{
  int pos,vpos;
  dliststructuretype *L;
  dlisttype *newelement;

  pos = (int) ((cost - circular.initial_ccost + circular.initial) % Circular_qsize);
  vpos = vertexposition(vertex);

  L = &(circular.ccost[pos]);
  newelement = &circular.ptr_vtx[vpos];
  
  if (L->begin == NULL) {
    /* Primeiro elemento. */
    L->begin = newelement; 
    newelement->next = NULL;
    newelement->previous = NULL;
  } else {
    newelement->previous = NULL;
    newelement->next = L->begin;
    (L->begin)->previous = newelement;
    L->begin = newelement;
  }
  L->numberelements++;
  cc_vtx[(*vertex).y][(*vertex).x] = cost;
  return(TRUE);
}

 
/*****************************************************************************
 * FUNCTION: InitialPoint_Selected 
 * DESCRIPTION: Initializes global variables and procedures related to
 * LWOF,LLANE and ALANE for the initial point selected on the
 * boundary.
 * PARAMETERS:
 * timg: The working image.  
 * eventx,eventy: x and y coordinates of the event related to the
 * first point selected on the boundary.
 * SIDE EFFECTS: Calculate edge costs, sign buffer, initialize circular
 * data structure of LWOF.
 * ENTRY CONDITIONS: If the first point selected on the boundary is valid. 
 * RETURN VARIABLE: Returns to the phase 1 of the algorithm.
 * EXIT CONDITIONS: NONE 
 * HISTORY: Created  by ???
 * Modified: 16/02/2001 by Alexandre Falcao in order to include the
 * initialization of the circular data structure used for LWOF.
 *    Modified: 12/3/08 image coordinates used if timg is NULL by Dewey Odhner.
 *
 *****************************************************************************/

/*****************************************************************************/
int Segment2dCanvas::InitialPoint_Selected(IMAGE *timg, int eventx, int eventy)
{

	/*------------------------------------------------------*/
	/*  if new slice, save object mask from previous slice  */
	/*  and load objects already saved for this slice       */
	/*------------------------------------------------------*/
	if(mask_index_different())
	{
		save_mask_proc(0);  /* saves object mask */
 
		object_mask_slice_index = sl.slice_index[ VolNo ][ mCavassData->m_sliceNo ];
 
		/* Load the appropriate object-mask buffer */
		load_object_mask(object_mask_slice_index);
 
	}

    /*----------------------------------*/
    /*       FIRST TIME DP              */
    /*----------------------------------*/

/** if FIRST TIME DP, calc edge costs ***/
	if (LAST_DP_slice_index==-1 || LAST_DP_slice_index!=o_contour.slice_index
        || o_contour.slice_index != sl.slice_index[VolNo][mCavassData->m_sliceNo] )
	{
        SetStatusText("Calculating Edge Costs. Wait...", 0);
        Calc_Edge_Costs();
        SetStatusText("Select Point on Boundary", 0);
		/** Since this is Initial point, update o_contour.slice_index */
        o_contour.slice_index = sl.slice_index[VolNo][mCavassData->m_sliceNo];
		LAST_DP_slice_index = o_contour.slice_index;
    }


	/*----------------------------*/
	/*     SAVE INITIAL POINT     */
	/*----------------------------*/
	o_contour.last=0;
	if (timg)
	{
		o_contour.vertex[0].y = timg->tbly[eventy - timg->locy];
		o_contour.vertex[0].x = timg->tblx[eventx - timg->locx];
	}
	else
	{
		o_contour.vertex[0].y = eventy;
		o_contour.vertex[0].x = eventx;
	}

	if (detection_mode==LWOF || detection_mode==ILW || detection_mode==LSNAKE)
	 /* Initialize Circular Data Structure */
	  InitCircular(o_contour.vertex[0], timg? timg: &orig);

	object_vertex[o_contour.vertex[0].y][o_contour.vertex[0].x]     
                     |= onbit[object_number];
	o_contour.img = timg? timg: &orig; /* Sets Current Img - all action done on this */




        /* Create Sign Buffer Based on Sign of the Grad 1 */ 

	create_sign_buffer(); 

    return 1;
  
}
 




 
/*****************************************************************************
 * FUNCTION: Point_Selected
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::Point_Selected(IMAGE *timg, int eventx, int eventy)
{

	if (NumPoints!=0) /* Live wire exists */
	{
		if (timg)
			Store_Contour_Seg(timg, orig.tblx[eventx],orig.tbly[eventy], OPEN);
		else
			Store_Contour_Seg(NULL, eventx, eventy, OPEN);
	}
	/* this updates o_contour.last & corresponding vertex */

    return 0;

}







/*****************************************************************************
 * FUNCTION: Is_Point_Selected_Valid
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified: 01/03/2001 band idea for LWOF by Alexandre Falcao.
 *    Modified: 4/1/08 eventx, eventy range checked by Dewey Odhner.
 *    Modified: 12/3/08 image coordinates used if timg is NULL by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::Is_Point_Selected_Valid(IMAGE *timg, int eventx, int eventy)
{
	int px, py;
 
 
	/*-----------------------------------------------------*/
    /*   Check if Point selected in VALID area of window   */
    /*-----------------------------------------------------*/
    if (timg == NULL)
	{
		px = eventx;
		py = eventy;
	}
	else
	{
		if (o_contour.last!=-1 && timg!=o_contour.img)
    	{
    	    // "MOUSE IN INVALID AREA!!"
    	    return (-1);
    	}
		if (  eventx - timg->locx < 0 || eventx - timg->locx >= timg->w ||
			  eventy - timg->locy < 0 || eventy - timg->locy >= timg->h)
			return (-1);
	    px = timg->tblx[eventx - timg->locx];
	    py = timg->tbly[eventy - timg->locy];
	    assert( px>=0 && px < orig.width && py >= 0 && py < orig.height );
	}


	/** if mask cut-out, & point not inside it */
	if( empty_mask_flag[object_number]==FALSE && (mask[*(tblr+py)+px] & onbit[object_number]) == 0 ) /*OUTSIDE*/
    {
        // "POINT SELECTED NOT IN MASK AREA"
        return (-1);
    }
 

	/*** following checks valid only if at least one DP has been done **/
	/*** which would have been done if at least one point selected
         in either MAN or LIVE mode  ***/
	if( o_contour.last != -1)
	{
    	if ( cc_vtx[py][px] == (unsigned long)OUTSIDE_MASK )
    	{
        	// "MOUSE LEFT MASK AREA OR ON CONTOUR SEGMENT"
        	return (-1);
    	}

    	if ( dir_vtx[py][px]==0 && (py!=o_contour.vertex[o_contour.last].y || px!=o_contour.vertex[o_contour.last].x) )
    	{
        	// "MOUSE CROSSED PREVIOUS CONTOUR SEGMENT!"
        	return (-1);
    	}
	}

	return 0;

}

/*****************************************************************************
 * FUNCTION: Live_Wire
 * DESCRIPTION: 
 * PARAMETERS:
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: 
 * RETURN VALUE: 
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 
 *    Modified: 6/6/02 line out of window not dislayed by Dewey Odhner
 *    Modified: 12/3/08 image coordinates used if timg is NULL by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::Live_Wire(IMAGE *timg, int scrx, int scry, int flag)
/* scrx, scry; Pixel-centre mouse points to, in screen coordinates */
/* flag; indicates if contour_seg to stay OPEN or to be CLOSEd */
{
    int x1,y1,x2,y2; /* current/derived x/y in original data coordinates */
    X_Point *ptr;

    if (timg && timg!=o_contour.img)
    {
        // "MOUSE LEFT IMAGE WINDOW!"
        return (-1);
    }

    /* if OPEN, convt scrxy to nw vertex of pix in orig data coods , else, Initx
y */
	if (flag != OPEN)
	{
		x1 = o_contour.vertex[0].x;
		y1 = o_contour.vertex[0].y;
	}
	else if (timg == NULL)
	{
		x1 = scrx;
		y1 = scry;
	}
	else
	{
		x1 = timg->tblx[scrx - timg->locx];
		y1 = timg->tbly[scry - timg->locy];
	}

    if ( cc_vtx[y1][x1] == (unsigned long)OUTSIDE_MASK )
    {
        // "MOUSE LEFT MASK AREA OR IS ON CONTOUR SEGMENT"
        return (-1);
    }

	/** if mask cut-out, & point not inside it */
	if( empty_mask_flag[object_number]==FALSE && (mask[*(tblr+y1)+x1] & onbit[object_number]) == 0 ) /*OUTSIDE*/
    {
        // "MOUSE LEFT MASK AREA"
        return (-1);
    }

    if ( dir_vtx[y1][x1]==0 && (y1!=o_contour.vertex[o_contour.last].y || x1!=o_contour.vertex[o_contour.last].x) )
    {
        // "MOUSE CROSSED PREVIOUS CONTOUR SEGMENT!"
        return (-1);
    }
 
 
    /**** if above conditions don't occur,
          i.e., if mouse is in a valid area of slice display ****/
    ptr = Points;
	if (timg)
	{
	    ptr->x = timg->tblx[scrx - timg->locx];
	    ptr->y = timg->tbly[scry - timg->locy];
	}
	else
	{
		ptr->x = scrx;
		ptr->y = scry;
	}

    while( o_contour.last>=0 && !(y1==o_contour.vertex[o_contour.last].y && x1==o_contour.vertex[o_contour.last].x && dir_vtx[y1][x1]==0) )
    {
        switch(dir_vtx[y1][x1]) {
            case EAST:
                y2=y1;
                x2=x1+1;
                break;
            case NORTH:
                y2=y1-1;
                x2=x1;
                break;
            case WEST:
                y2=y1;
                x2=x1-1;
                break;
            case SOUTH:
                y2=y1+1;
                x2=x1;
                break;
            default:
				fprintf(stderr, "x1=%d; y1=%d; dir_vtx[y1][x1]=%d\n",
					x1, y1, dir_vtx[y1][x1]);
				assert(FALSE);
                return(-1); 
        }
        ptr++;
        if( (ptr-Points+1)==MAX_POINTS)
          {
            wxMessageBox("ERROR: Points structure OVERFLOWED. Try again...");
            return(-1);
          }
        ptr->x = x2;
        ptr->y = y2;
 
        x1=x2;
        y1=y2;
    } /* endwhile */
    /* ptr points to very last Points-vertex */
    NumPoints = (int)(ptr-Points +1);

	Refresh();
    return 0;
}
 

int Segment2dCanvas::Store_Contour_Seg(IMAGE *timg, int imgx, int imgy, int flag)
/* imgx, imgy; Next pt selected by user, in img coordinates */
/* flag; indicates if contour_seg to stay OPEN or to be CLOSEd */
{
    int x1,y1,x2,y2; /* current/derived x/y in original data coordinates */
    int i=1;
 
    /* if OPEN, convt imgxy to orig data coods , else, Initxy */
    x1 = (flag==OPEN) ? imgx : o_contour.vertex[0].x;
    y1 = (flag==OPEN) ? imgy : o_contour.vertex[0].y;

    while( o_contour.last>=0 && !(y1==o_contour.vertex[o_contour.last].y && x1==o_contour.vertex[o_contour.last].x && dir_vtx[y1][x1]==0) )
    {
        o_contour.vertex[o_contour.last+NumPoints-i].x = x1;
        o_contour.vertex[o_contour.last+NumPoints-i].y = y1;
        object_vertex[y1][x1] |= onbit[object_number];

        switch(dir_vtx[y1][x1]) {
            case EAST:
                y2=y1;
                x2=x1+1;
                break;
            case NORTH:
                y2=y1-1;
                x2=x1;
                break;
            case WEST:
                y2=y1;
                x2=x1-1;
                break;
            case SOUTH:
                y2=y1+1;
                x2=x1;
                break;
            default:
                printf("ERROR in Store_Contour_Seg\n");
                exit(1);
        }

        i++;
        x1=x2;
        y1=y2;
    } /* endwhile */
 
    o_contour.last += (i-1);
    if( flag==CLOSE) o_contour.last--; /* removes Initxy from last position
                                          of o_contour.vertex array */
    NumPoints = 0;  /* reset to 0 for next segment */
 
 
    return 0;
}


int Segment2dCanvas::Close_Phase(IMAGE *timg)
{
	int flag=0; /* value of phase */

    if( o_contour.last==0 ) /* only first pt selected */
	{
        wxMessageBox("CANNOT CLOSE WITH ONLY INITIAL POINT");
		flag=1;
	}
    else
    {
        SetStatusText("Closing Contour.. wait...", 0);
        if( Close_Contour(timg)!=0)
	      flag=2;
	else
	  {
	    /* copy contour to temporary contour */
	    copy_ocontour_into_temp_array();

	    NumPoints=0;
	    o_contour.last = -1; /* setting o_contour.vertex to empty */
	    SetStatusText("Closed Contour", 0);
	    flag=3;
	  }
    }

	return (flag);
}

/*****************************************************************************
 * FUNCTION: Close_Contour
 * DESCRIPTION: Compute and store last contour segment of LW.
 * PARAMETERS:
 *    timg: working image
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: Last point selected on the boundary is valid.
 * RETURN VALUE: Returns 0 if the operation is succeeded.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: mm/dd/yy by ???
 *    Modified: 16/02/2001 by Alexandre Falcao. Avoid computation of
 *    cumulative costs in LWOF.
 *
 *****************************************************************************/
int Segment2dCanvas::Close_Contour(IMAGE *timg)
{
    int flag=0;


    flag= Live_Wire(timg, o_contour.vertex[0].x, o_contour.vertex[0].y, CLOSE);
 
    if(flag==0)
      {
	Store_Contour_Seg(timg, o_contour.vertex[0].x, o_contour.vertex[0].y, CLOSE); /* store closed cont_seg */
	return 0;
      }
    else
      return(-1); 
}
 
/*****************************************************************************
 * FUNCTION: Add_Contours
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified: 28/02/2001 by Alexandre Falcao
 *    
 *****************************************************************************/
int Segment2dCanvas::Add_Contours()
{

	if( temp_contours.last != -1 ) /* at least one contour exists */
	{
		Convert_Contours_to_Vedges();

		SaveContour2ObjectMask(); /* Vedges --> object_mask - CUT */

		clear_temporary_contours_array();
		clear_Vedges_array();
		Reset_ObjectVertex(); /* to be consistent with Roberto's prog */
	}

	return 0;
}

/*****************************************************************************
 * FUNCTION: Cut_Contours
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 28/02/2001 by Alexandre Falcao
 *    Modified: mm/dd/yy by programmer name
 *    
 *****************************************************************************/
int Segment2dCanvas::Cut_Contours()
{

	if( temp_contours.last != -1 ) /* at least one contour exists */
	{
		Convert_Contours_to_Vedges();

		DeleteContour2ObjectMask(); /* Vedges --> object_mask - CUT */

		clear_temporary_contours_array();
		clear_Vedges_array();
		Reset_ObjectVertex(); /* to be consistent with Roberto's prog */
	}

	return 0;
}


/*****************************************************************************
 * FUNCTION: Convert_Contours_to_Vedges
 * DESCRIPTION: Converts current contour information to V_edges array.
 * PARAMETERS: None
 * SIDE EFFECTS: Appends contour to file "DP_CONTOUR_RECORD".
 * ENTRY CONDITIONS: sl.slice_index, mCavassData,
 * RETURN VALUE: 0
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: ?
 *    Modified: 8/13/04 Appends contour to file "DP_CONTOUR_RECORD" by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::Convert_Contours_to_Vedges()
{
    int i, row, col;
	static int first_call=1;
	static FILE *fp;

	if (first_call)
	{
		first_call = 0;
		fp = fopen("DP_CONTOUR_RECORD", "rb");
		if (fp)
		{
			fclose(fp);
			fp = fopen("DP_CONTOUR_RECORD", "ab");
		}
	}
	if (fp)
		fprintf(fp, "\n");
	for(i=0; i<temp_contours.last; i++) /* stop 1 short of end */
   	{
       	if( temp_contours.vertex[i].x == temp_contours.vertex[i+1].x)
       	{
           	col =  temp_contours.vertex[i].x;
           	row = (temp_contours.vertex[i].y < temp_contours.vertex[i+1].y) ?
				   temp_contours.vertex[i].y : temp_contours.vertex[i+1].y;
           	if (detection_mode == PAINT)
				V_edges[row][col] ^=  onbit[object_number];
			else
				V_edges[row][col] |=  onbit[object_number];
       	}
		if (fp)
			fprintf(fp, "%5d %5d %5d\n", temp_contours.vertex[i].x,
				temp_contours.vertex[i].y,
				sl.slice_index[VolNo][mCavassData->m_sliceNo]);
   	}

   	i=temp_contours.last;
   	if ( temp_contours.vertex[i].x == temp_contours.vertex[0].x)
   	{
       	col =  temp_contours.vertex[i].x;
       	row = (temp_contours.vertex[i].y < temp_contours.vertex[0].y) ?
			   temp_contours.vertex[i].y : temp_contours.vertex[0].y;
       	if (detection_mode == PAINT)
			V_edges[row][col] ^=  onbit[object_number];
		else
			V_edges[row][col] |=  onbit[object_number];
   	}

    return 0;
}
 
 

/*****************************************************************************
 * FUNCTION: SaveContour2ObjectMask
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::SaveContour2ObjectMask()
{
    int inside=0;
    int i, j;
    unsigned char **Vptr, *Vptc, obj;
 
    obj = onbit[object_number]; /* to save computation time */
 
    for(Vptr=V_edges, j=0; j<orig.height; Vptr++, j++)
    {
        for(Vptc=*Vptr, i=0; i<orig.width; Vptc++, i++)
        {
            if ( (*Vptc&obj)!=0 && inside==0 ) /* Begin*/
                inside = 1;
            else
            if ( (*Vptc&obj)!=0 && inside==1 )  /* End */
                inside = 0;
 
            if (inside==1)
                object_mask[*(tblr+j) + i] |= obj;
        }
        inside = 0;    /* precaution */
    }
 
 
    return 0;
}

/*****************************************************************************
 * FUNCTION: DeleteContour2ObjectMask
 * DESCRIPTION: 
 * PARAMETERS:
 * SIDE EFFECTS: 
 * ENTRY CONDITIONS: 
 * RETURN VALUE: 
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 28/02/2001 by Alexandre Falcao
 *    Modified: 5/21/02 to alter only one object by Dewey Odhner
 *
 *****************************************************************************/
int Segment2dCanvas::DeleteContour2ObjectMask()
{
    int inside=0;
    int i, j;
    unsigned char **Vptr, *Vptc, obj;

    obj = onbit[object_number]; /* to save computation time */

    for(Vptr=V_edges, j=0; j<orig.height; Vptr++, j++)
    {
        for(Vptc=*Vptr, i=0; i<orig.width; Vptc++, i++)
        {
            if (*Vptc & obj)
                inside = !inside;
            if (inside)
                object_mask[*(tblr+j) + i] &= ~obj;

        }
        inside = 0;    /* precaution */
    }

    return 0;
}
 

/*****************************************************************************
 * FUNCTION: clear_Vedges_array
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::clear_Vedges_array()
{
	int i, j;
	unsigned char offobj;

	offobj = offbit[object_number];

	for(j=0; j<orig.height; j++)
	for(i=0; i<=orig.width; i++)
	{
		V_edges[j][i] &= offobj;
	}


	return 0;
}

void Segment2dCanvas::Reset_object_proc()
{
	/* Save object (if it was specified on different slice) */
	if (mask_index_different())
	{
		save_mask_proc(0);  /* saves object mask */
 
		object_mask_slice_index = sl.slice_index[ VolNo ][ mCavassData->m_sliceNo ];
	}

	/* Reset mask bit */
	set_object_mask_bit(object_number, default_mask_value); 

	/* Reset any contours */
	clear_temporary_contours_array();
	set_object_vertex_mask_bit(object_number, 0);

	int cs=ilw_control_slice[0]==sl.slice_index[VolNo][mCavassData->m_sliceNo]?
		0: ilw_control_slice[1]==sl.slice_index[VolNo][mCavassData->m_sliceNo]?
		1: -1;
	if (cs >= 0)
	{
		if (ilw_control_slice[1] == ilw_control_slice[0])
		{
			ilw_control_points[0] = ilw_control_points[1] = 0;
			free(ilw_control_point[0]);
			ilw_control_point[0] = ilw_control_point[1] = NULL;
			ilw_control_slice[0] = -1;
			ilw_control_slice[1] = -2;
			dp_anchor_points = 0;
		}
		else
		{
			ilw_control_slice[cs] = ilw_control_slice[!cs];
			ilw_control_points[cs] = ilw_control_points[!cs];
			free(ilw_control_point[cs]);
			ilw_control_point[cs] = ilw_control_point[!cs];
		}
	}

	save_mask_proc(0);  /* saves object mask */

	if (object_number < 8)
		SetStatusText(wxString::Format("Object #%d deleted.", object_number+1),
			0);
	else
		SetStatusText("All objects deleted.", 0);
	reload();
}

void Segment2dCanvas::set_object_vertex_mask_bit(int bit, int value)
{
	int i, j;

	if(bit>=0)
	{
		for(j=0; j<=orig.height; j++)
		  for(i=0; i<=orig.width; i++)
			object_vertex[j][i] = (value>0) ? (object_vertex[j][i] & onbit[bit]) : (object_vertex[j][i] & offbit[bit]);
	}
	else
	{
		if(value == 1) value = 255;
		for(j=0; j<=orig.height; j++)
		  for(i=0; i<=orig.width; i++)
			object_vertex[j][i] = value;
	}
}

bool Segment2dCanvas::mask_index_different()
{
	return
		object_mask_slice_index!=sl.slice_index[VolNo][mCavassData->m_sliceNo];
}






/*****************************************************************************/
/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*                         CALCULATION OF                                    */
/*                     FEATURES AND EDGE COSTS                               */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
/*****************************************************************************/



/*****************************************************************************
 * FUNCTION: Alloc_CostArrays
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
/** allocate space for edge_cost, cum_cost & dir_vtx arrays ****/
int Segment2dCanvas::Alloc_CostArrays()
{
	int i;

/**** TESTING - June 28, '93 changed costs & features from short 
                             to unsigned shorts ***/

	/**** Allocate space for hzl & vert costs ****/
    if( (hzl_cost=(unsigned short **)malloc((orig.height+1)*sizeof(unsigned short*)))==NULL)
	{
		wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
	}
    for(i=0; i<(orig.height+1); i++)
        if( (hzl_cost[i]=(unsigned short *)malloc(orig.width*sizeof(unsigned short)))==NULL)
		{
			wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
			return 1;
		}
 
    if( (vert_cost=(unsigned short **)malloc(orig.height*sizeof(unsigned short*)))==NULL)
	{
		wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
    for(i=0; i<orig.height; i++)
        if( (vert_cost[i]=(unsigned short *)malloc((orig.width+1)*sizeof(unsigned short)))==NULL)
		{
			wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
			return 1;
        }

	/**** Allocate space for hzl & vert sign cost buffers ****/

    if( (hzl_sign_buffer=(unsigned char **)malloc((orig.height+1)*sizeof(unsigned char*)))==NULL)
      {
	wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
	return 1;
      }
    for(i=0; i<(orig.height+1); i++)
      if( (hzl_sign_buffer[i]=(unsigned char *)malloc(orig.width*sizeof(unsigned char)))==NULL)
	{
	  wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
	  return 1;
	}
 
     if( (vert_sign_buffer=(unsigned char **)malloc(orig.height*sizeof(unsigned char*)))==NULL)
       {
	 wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
	 return 1;
       }
    for(i=0; i<orig.height; i++)
        if( (vert_sign_buffer[i]=(unsigned char *)malloc((orig.width+1)*sizeof(unsigned char)))==NULL)
	  {
	    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
	  }


    /**** Allocate space for vertex Cumulative Costs ****/
    if( (cc_vtx=(unsigned long **)malloc((orig.height+1)*sizeof(unsigned long *)))==NULL)
	{
		wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
    for(i=0; i<(orig.height+1); i++)
        if( (cc_vtx[i]=(unsigned long *)malloc((orig.width+1)*sizeof(unsigned long)))==NULL)
		{
			wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
			return 1;
        }
 

    /**** Allocate space for vertex Mask: This has to do with the band idea for LWOF 

    if( (vtx_mask=(unsigned char **)malloc((orig.height+1)*sizeof(unsigned char *)))==NULL)
	{
	  wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
	  return 1;
	}
	for(i=0; i<(orig.height+1); i++)
	  if( (vtx_mask[i]=(unsigned char *)malloc((orig.width+1)*sizeof(unsigned char)))==NULL)
	    {
	      wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		  return 1;
	    }
 ****/
	
    /**** Allocate space for vertex Directions & init to 0 ****/
    if( (dir_vtx=(char **)malloc((orig.height+1)*sizeof(char *)))==NULL)
	{
		wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
    for(i=0; i<(orig.height+1); i++)
        if( ( dir_vtx[i]=(char *)malloc((orig.width+1)*sizeof(char)))==NULL)
		{
			wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
			return 1;
        }


	/*============   Build table for row*(orig.width+1) -used for DP ======*/
	if( (tblcc=(int *)malloc((orig.height+1)*sizeof(int)))==NULL)
	{
		wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
	else
		for(i=0; i<(orig.height+1); i++)
			tblcc[i] = i*(orig.width+1);

	/*============   Build table for row*orig.width -used for DP ======*/
    if( (tblr=(int *)malloc((orig.height+1)*sizeof(int)))==NULL)
	{
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
    else
        for(i=0; i<(orig.height+1); i++)
            tblr[i] = i*orig.width;
 

	/**** Allocate space for array region[] used in TRAIN mode *******/
	if( (region=(unsigned char *)calloc(orig.height*orig.width, sizeof(unsigned char)))==NULL)
    {
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE TRAIN MODE");
		return 1;
    }
	return 0;
}


/*****************************************************************************
 * FUNCTION: Dealloc_CostArrays
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
/** de-allocate space for edge_cost, cum_cost & dir_vtx arrays ****/
void Segment2dCanvas::Dealloc_CostArrays()
{
	int j;

    /*** Free alloced space of Hzl/Vert Edge Costs ****/
	if (hzl_cost!=NULL)
	{
    	for(j=0; j<(orig.height+1); j++)
        	free(hzl_cost[j]);
    	free(hzl_cost);
		hzl_cost=NULL;
	}
    if (vert_cost!=NULL)
    {
        for(j=0; j<orig.height; j++)
        	free(vert_cost[j]);
    	free(vert_cost);
		vert_cost=NULL;  /* set to NULL ptr for TESTING */
	}
 
    /**** Free alloced space of Cum Costs *****/
	if (cc_vtx!=NULL)
	{
    	for(j=0; j<(orig.height+1); j++)
        	free(cc_vtx[j]);
    	free(cc_vtx);
		cc_vtx = NULL;
	}


    /**** Free alloced space of vertex mask: This has to do with the band idea for LWOF 
 
	if (vtx_mask!=NULL)
	{
    	for(j=0; j<(orig.height+1); j++)
        	free(vtx_mask[j]);
    	free(vtx_mask);
		vtx_mask = NULL;
	}
    *****/

    /**** Free alloced space of Directions of Vertices *****/
	if (dir_vtx!=NULL)
	{
    	for(j=0; j<(orig.height+1); j++)
        	free(dir_vtx[j]);
    	free(dir_vtx);
		dir_vtx = NULL;
	}

	/*======= Free space alloced for tblcc & tblr ========*/
	if(tblcc!=NULL)
		free(tblcc);
	tblcc = NULL;
	if(tblr!=NULL)
        free(tblr);
	tblr = NULL;

	if(region!=NULL)
        free(region);
	region = NULL;
}








/*****************************************************************************
 * FUNCTION: Alloc_Edge_Features
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::Alloc_Edge_Features()
{
    int i;
 
/*** TESTING  June 28, '93 changed features short to unsinged short */

    /*** Allocate space for hzl & vert gradients ***/
    if( (hzl_grad=(unsigned short **)malloc((orig.height+1)*sizeof(unsigned short*)))==NULL)
    {
	    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
	for(i=0; i<(orig.height+1); i++)
        if( (hzl_grad[i]=(unsigned short *)malloc(orig.width*sizeof(unsigned short)))==NULL)
        {
		    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
			return 1;
		}
 
    if( (vert_grad=(unsigned short **)malloc(orig.height*sizeof(unsigned short*)))==NULL)
	{
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
		return 1;
    }
	for(i=0; i<orig.height; i++)
        if( (vert_grad[i]=(unsigned short *)malloc((orig.width+1)*sizeof(unsigned short)))==NULL)
        {
		    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
			return 1;
		}
 
    /*** Allocate space for hist values ***/
    if( (hist=(int *)calloc(Range+1, sizeof(int)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    return hist==NULL;
}
 
 
 
 



/*****************************************************************************
 * FUNCTION: Dealloc_Edge_Features
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
void Segment2dCanvas::Dealloc_Edge_Features()
{
    int i;
 
    for(i=0; i<(orig.height+1); i++)
        free(hzl_grad[i]);
    free(hzl_grad);
    for(i=0; i<orig.height; i++)
        free(vert_grad[i]);
    free(vert_grad);
 
    free(hist);
}
 
 





/*****************************************************************************
 * FUNCTION: Alloc_Temp_Arrays
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
void Segment2dCanvas::Alloc_Temp_Arrays()
{
    int i;
 
/*** TESTING June 28, '93 . changed short to unsigned short */

    /*** Allocate space for temporary hzl & vert features ***/
    if( (hzl_tempf=(unsigned short **)malloc((orig.height+1)*sizeof(unsigned short*)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    for(i=0; i<(orig.height+1); i++)
        if( (hzl_tempf[i]=(unsigned short *)malloc(orig.width*sizeof(unsigned short)))==NULL)
            wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
 
    if( (vert_tempf=(unsigned short **)malloc(orig.height*sizeof(unsigned short*)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    for(i=0; i<orig.height; i++)
        if( (vert_tempf[i]=(unsigned short *)malloc((orig.width+1)*sizeof(unsigned short)))==NULL)
            wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
 
 
    /*** Allocate space for temporary hzl & vert costs ***/
    if( (hzl_tempc=(unsigned short **)malloc((orig.height+1)*sizeof(unsigned short*)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    for(i=0; i<(orig.height+1); i++)
        if( (hzl_tempc[i]=(unsigned short *)malloc(orig.width*sizeof(unsigned short)))==NULL)
            wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
 
    if( (vert_tempc=(unsigned short **)malloc(orig.height*sizeof(unsigned short*)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    for(i=0; i<orig.height; i++)
        if( (vert_tempc[i]=(unsigned short *)malloc((orig.width+1)*sizeof(unsigned short)))==NULL)
            wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");


	/*** Allocate space for temporary total costs ****/
	if( (totlhc=(double **)malloc((orig.height+1)*sizeof(double *)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    for(i=0; i<(orig.height+1); i++)
        if( (totlhc[i]=(double *)malloc(orig.width*sizeof(double)))==NULL)
            wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");

	if( (totlvc=(double **)malloc(orig.height*sizeof(double *)))==NULL)
        wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    for(i=0; i<orig.height; i++)
        if( (totlvc[i]=(double *)malloc((orig.width+1)*sizeof(double)))==NULL)
            wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
 

	return;

}
 
 
 
 




/*****************************************************************************
 * FUNCTION: Dealloc_Temp_Arrays
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
void Segment2dCanvas::Dealloc_Temp_Arrays()
{
    int i;
 
    for(i=0; i<(orig.height+1); i++)
        free(hzl_tempf[i]);
    free(hzl_tempf);
    for(i=0; i<orig.height; i++)
        free(vert_tempf[i]);
    free(vert_tempf);
 
    for(i=0; i<(orig.height+1); i++)
        free(hzl_tempc[i]);
    free(hzl_tempc);
    for(i=0; i<orig.height; i++)
        free(vert_tempc[i]);
    free(vert_tempc);

    for(i=0; i<(orig.height+1); i++)
      free(totlhc[i]);
    free(totlhc);
	for(i=0; i<orig.height; i++)
        free(totlvc[i]);
    free(totlvc);

	return;
}
 
 
 
 

 






/*****************************************************************************
 * FUNCTION: Calc_Edge_Features
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: ??/??/?? Shoba Sharma
 *    Modified: 05/04/94 by Supun Samarasekera
 *
 *****************************************************************************/
/** Edge features are calculated for the entire slice *******/
int Segment2dCanvas::Calc_Edge_Features(int num /* feature num */,
	int flag /* 1 => grad, 2=> tempf */, struct FeatureList *list)
{

	if( list->status == OFF)
	{
		wxMessageBox("STATUS of current feature is OFF");
		return 0;
	}


	switch(num) {
 
		case 0:
            Density1(flag); /* Hi Density */
            break;
        case 1:
            Density2(flag); /* Lo Density */
            break;
    	case 2:
	    AbsGrad1(flag);
	    break;
	case 3:
	    AbsGrad2(flag);
	    break;
	case 4:
            AbsGrad3(flag);
            break;
    	case 5:
            AbsGrad4(flag);
	    break;
    	} /* end swich */


	/* Scaling of features have been removed by Supun.
	   It would use the aboslute feature values for
	   trasforming now. (same as using a global sclae
	   factor */
	/* Scale_Features(num, flag); */

 
	return 0;
}
 
 
 





/*****************************************************************************
 * FUNCTION: Calc_Edge_Costs_from_Features
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
/** Edge features are calculated for the entire slice *******/
int Segment2dCanvas::Calc_Edge_Costs_from_Features(int num /* feature */,
	int flag /* flag, 1=>cost,  2=>tempc */, struct FeatureList *list)
{
    
    if( list->status == OFF)
      {
        wxMessageBox("STATUS of current feature is OFF");
        return 0;
      }
    
    
    switch(temp_list[num].transform) { /* on transform */
      
    case 0:
      LinearTransform(num, flag);
      break;
    case 1:
      GaussianTransform(num, flag);
      break;
    case 2:
      InvLinearTransform(num, flag);
      break;
    case 3:
      InvGaussianTransform(num,flag);
      break;
    case 4:
      HyperTransform(num,flag);
      break;
    case 5:
      InvHyperTransform(num,flag);
      break;
    }
    
    
    return 0;
}
 
 




/*****************************************************************************
 * FUNCTION: Calc_Combined_Edge_Costs
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified: 4/22/08 added vert. costs for last column by Dewey Odhner.
 *
 *****************************************************************************/
/* flag: 0 => ALL features considered for calculations
         1 => all EXCEPT curr_feature considered for calculations   */
void Segment2dCanvas::Calc_Combined_Edge_Costs(struct FeatureList *list, int flag)
{
  int row, col, i;
  float total_weight=0; /* total of feature weighting factors */
  
  Alloc_Temp_Arrays();
  
  /** Calculate total of all weights of features **/
  total_weight = (flag==0) ? 0 : list[curr_feature].weight;
  for(i=0; i<MAX_NUM_FEATURES; i++)
    if(list[i].status==ON && (flag==0 || i!=curr_feature) )
      total_weight += list[i].weight;
  const float wf = 1/total_weight;
  for(row=0; row<=orig.height; row++)
    for(col=0; col<orig.width; col++)
      {
        if(flag==0)
          totlhc[row][col] = 0;
        else
          totlhc[row][col] = hzl_cost[row][col]*(
						 list[curr_feature].weight*wf);
      }
  for(row=0; row<orig.height; row++)
    for(col=0; col<=orig.width; col++)
      {
        if(flag==0)
          totlvc[row][col] = 0;
        else
          totlvc[row][col] = vert_cost[row][col]*(
						  list[curr_feature].weight*wf);
      }

  for(i=0; i<MAX_NUM_FEATURES; i++)
    {
      if(list[i].status==ON && (flag==0 || i!=curr_feature) )
        {
          Calc_Edge_Features(i, 2, &list[i]);
          Calc_Edge_Costs_from_Features(i, 2, &list[i]);
  
          /*** Add temp to actual Costs  ***/
          for(row=0; row<=orig.height; row++)
            for(col=0; col<orig.width; col++)
              totlhc[row][col] += hzl_tempc[row][col]*(
						       list[i].weight*wf);
          for(row=0; row<orig.height; row++)
            for(col=0; col<=orig.width; col++)
              totlvc[row][col] += vert_tempc[row][col]*(
							list[i].weight*wf);
        }
    }
  
  /*** set total back to hzl_cost **/
  for(row=0; row<=orig.height; row++)
    for(col=0; col<orig.width; col++)
      hzl_cost[row][col] = (unsigned short)totlhc[row][col];
  for(row=0; row<orig.height; row++)
    for(col=0; col<=orig.width; col++)
      vert_cost[row][col] = (unsigned short)totlvc[row][col];
  
  Dealloc_Temp_Arrays();
  
  
}




 
/*****************************************************************************
 * FUNCTION: Calc_Edge_Costs
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
/** Edge features are calculated for the entire slice *******/
void Segment2dCanvas::Calc_Edge_Costs()
{
	/* list used here is always accepted_list only */

	Alloc_Edge_Features();

	Calc_Combined_Edge_Costs(accepted_list[object_number%8], 0);
	
	/** Find the total sum of all edge costs -for use in Calc_Cum_Cost***/
	Calc_SumCost();

	Dealloc_Edge_Features();

 
}






/*****************************************************************************
 * FUNCTION: ReCalc_Edge_Costs
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
/** Edge features are calculated for the entire slice *******/
/** same as Calc_Edge_Costs, but Features already allocated **/
void Segment2dCanvas::ReCalc_Edge_Costs()
{
    /* list used here is always accepted_list only */
 
    Calc_Combined_Edge_Costs(accepted_list[object_number%8], 0);
 
    /** Find the total sum of all edge likelihoods -for use in Calc_Cum_Cost***/
    Calc_SumCost();
 
 
}
 






/******************************************************************************/
/******************************************************************************/
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                            DYNAMIC PROGRAMMING                             */
/*                                                                            */
/*              CALCULATING CUMULATIVE COSTS OF VERTICES                      */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
 
 

/*****************************************************************************
 * FUNCTION: Initialize_Costs
 * DESCRIPTION:
 * PARAMETERS:
 * SIDE EFFECTS:
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created:
 *    Modified:
 *
 *****************************************************************************/
/*    Modified: 3/18/08 Removed zoomed slice initialization in case of no ROI
 *       by Dewey Odhner.
 */
int Segment2dCanvas::Initialize_Costs(IMAGE *timg, int flag)
{
    int row, col, width, height, initcol, initrow;

	/** Check for ROI ***/
	/** If ROI exists, & no mask cut, then work only inside roi */
	if (roix!=-1 && roiy!=-1) /* ROI has been cut */
	{
		/** initialize only border of roi **/
		for(col=roix; col<(roix+roiw+1); col++)
		{
			cc_vtx[roiy][col] = (unsigned long)OUTSIDE_MASK;
			cc_vtx[roiy+roih][col] = (unsigned long)OUTSIDE_MASK;
		}
		for(row=roiy+1; row<(roiy+roih); row++)
		{
			cc_vtx[row][roix] = (unsigned long)OUTSIDE_MASK;
			cc_vtx[row][roix+roiw] = (unsigned long)OUTSIDE_MASK;
		}
 
		/** init all IN-Mask inside periphery of ROI to SumCost */
		for(row=roiy+1; row<(roiy+roih); row++)
		for(col=roix+1; col<(roix+roiw); col++)
		{
			if( (mask[*(tblr+row)+col] & onbit[object_number]) == 0 )/*OUTSIDE*/
                {
                    /*** Set all 4 vertices cc_vtx = OUTSIDE_MASK ***/
                    cc_vtx[row][col] = (unsigned long)OUTSIDE_MASK;
                    cc_vtx[row][col+1] = (unsigned long)OUTSIDE_MASK;
                    cc_vtx[row+1][col] = (unsigned long)OUTSIDE_MASK;
                    cc_vtx[row+1][col+1] = (unsigned long)OUTSIDE_MASK;
                } /* endif OUTSIDE */
                else  /* INSIDE */
                {
                    cc_vtx[row][col] = (unsigned long)SumCost;
                    dir_vtx[row][col] = 0;
                }
		}
	} /* endif ROI? */
	else
	{

        /* WHOLE unzoomed slice */
        width = timg->width;
        height = timg->height;
        initcol = 0;
        initrow = 0;
 
 
        /****  IF mask exists  ***/
        if( empty_mask_flag[object_number] == FALSE)
        {
            for(row=initrow; row<(initrow+height); row++)
            for(col=initcol; col<(initcol+width); col++)
            {
                if( (mask[*(tblr+row)+col] & onbit[object_number]) == 0 ) /*OUTS
IDE*/
                {
                    /*** Set all 4 vertices cc_vtx = OUTSIDE_MASK ***/
                    cc_vtx[row][col] = (unsigned long)OUTSIDE_MASK;
                    cc_vtx[row][col+1] = (unsigned long)OUTSIDE_MASK;
                    cc_vtx[row+1][col] = (unsigned long)OUTSIDE_MASK;
                    cc_vtx[row+1][col+1] = (unsigned long)OUTSIDE_MASK;
                } /* endif OUTSIDE */
                else  /* INSIDE */
                {
                    cc_vtx[row][col] = (unsigned long)SumCost;
                    dir_vtx[row][col] = 0;
                }
            } /* end for loop */
        } /* endif mask exists */
        else  /** NO mask exists */
        {
            /** init all inside periphery to SumCost */
            for(row=initrow+1; row<(initrow+height); row++)
            for(col=initcol+1; col<(initcol+width); col++)
            {
                 cc_vtx[row][col] = (unsigned long)SumCost;
                 dir_vtx[row][col] = 0;
            }
        } /* end -else ... No Mask exists */
 
 
 
		/************************************************************/
        /** initialize everything along periphery to OUTSIDE_MASK  **/
		/************************************************************/
        for(col=initcol; col<(initcol+width+1); col++)
        {
            cc_vtx[initrow][col] = (unsigned long)OUTSIDE_MASK;
            cc_vtx[initrow+height][col] = (unsigned long)OUTSIDE_MASK;
        }
        for(row=initrow+1; row<(initrow+height); row++)
        {
            cc_vtx[row][initcol] = (unsigned long)OUTSIDE_MASK;
            cc_vtx[row][initcol+width] = (unsigned long)OUTSIDE_MASK;
        }
 
    } /* endif NOT ROI*/


    /**** Initialize all vertices already on contour segmt to cc=OutsideMask */
    /****    this prevents crossover of contours of same object ***/
    for(row=0; row<(orig.height+1); row++)
        for(col=0; col<(orig.width+1); col++)
            if( (object_vertex[row][col]&onbit[object_number]) == onbit[object_number] )
                cc_vtx[row][col] = (unsigned long)OUTSIDE_MASK;
 
 
    /****  if contour is being closed (flag==CLOSE) set cc_vtx of initialxy = SumCost */
    if( flag==CLOSE ) /* if CLOSE */
        cc_vtx[o_contour.vertex[0].y][o_contour.vertex[0].x] = (unsigned long)SumCost;
 
 
    return 0;
}



 
 
/*****************************************************************************
 * FUNCTION: create_sign_buffer
 * DESCRIPTION: Finds the sign of the density gradient across the pixel edge,
 *              based on a 2-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     sign = 1 if b > e and NEG1 if b < e 
 *
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 01/26/95  Alexandre Falcao
 *    Modified: 4/10/08 0 for no gradient by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::create_sign_buffer()
{
    int row, col;
    int nrow1, nrow2;  /* variables used to decrease computing time */
    unsigned char *ptr8, *ptr8_1, *ptr8_2;
    unsigned short *ptr16, *ptr16_1, *ptr16_2;
 

    for (col=0; col<orig.width; col++)
	{
		hzl_sign_buffer[0][col] = 0;
		hzl_sign_buffer[orig.height][col] = 0;
    }
	for (row=0; row<orig.height; row++)
	{
		vert_sign_buffer[row][0] = 0;
		vert_sign_buffer[row][orig.width] = 0;
	}

    /** Handle 8-bit & 16-bit data appropriately  **/

    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
		for(row=1; row<orig.height; row++)        /* Horizontal */ 
	  {
	    nrow1 = *(tblr+row-1); /* *orig.width*/
	    nrow2 = *(tblr+row);   /* *orig.width */

	    for(ptr8_1=ptr8+nrow1,ptr8_2=ptr8+nrow2, col=0; col<orig.width;ptr8_1++,ptr8_2++, col++)
                hzl_sign_buffer[row][col] = (*ptr8_2 > *ptr8_1 ) ? 1 : (*ptr8_2 < *ptr8_1 ) ? NEG1 : 0;
	  }

        for(row=0; row<orig.height; row++)        /* Vertical */ 
	  {
	    nrow1 = *(tblr+row);   /* *orig.width */
			
	    for(ptr8_1=ptr8+nrow1,ptr8_2=ptr8+nrow1+1,col=1; col<orig.width; ptr8_1++,ptr8_2++, col++)
	      vert_sign_buffer[row][col] = (*ptr8_2 > *ptr8_1 ) ? 1 : (*ptr8_2 < *ptr8_1 ) ? NEG1 : 0;
	  }

    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);

        for(row=1; row<orig.height; row++)        /* Horizontal */ 
	  {
	    nrow1 = *(tblr+row-1); /* *orig.width*/
	    nrow2 = *(tblr+row);   /* *orig.width */

	    for(ptr16_1=ptr16+nrow1,ptr16_2=ptr16+nrow2, col=0; col<orig.width;ptr16_1++,ptr16_2++, col++)
                hzl_sign_buffer[row][col] = (*ptr16_2 > *ptr16_1 ) ? 1 : (*ptr16_2 < *ptr16_1 ) ? NEG1 : 0;
	  }
 
        for(row=0; row<orig.height; row++)        /* Vertical */ 
	  {
	    nrow1 = *(tblr+row);   /* *orig.width */
			
	    for(ptr16_1=ptr16+nrow1,ptr16_2=ptr16+nrow1+1,col=1; col<orig.width; ptr16_1++,ptr16_2++, col++)
	      vert_sign_buffer[row][col] = (*ptr16_2 > *ptr16_1 ) ? 1 : (*ptr16_2 < *ptr16_1 ) ? NEG1 : 0;
	  }


    }/** endif 16-bit data **/
 

    return 0;
}



/******************************************************************************
*******************************************************************************
*                                                                             *
*                                                                             *
*                                                                             *
*                                                                             *
*                             FEATURES                                        *
*                                                                             *
*                                                                             *
*                                                                             *
*                                                                             *
*******************************************************************************
*******************************************************************************/




/*****************************************************************************
 * FUNCTION: AbsGrad1
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 2-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = |b - e|
 *
 * PARAMETERS:
 *    flag  :  indicates whether to use feature or temp arrays 
 *             1 - use hzl_grad & vert_grad arrays to calculate 
 *                 current feature
 *             2 - use hzl_tempf & vert_tempf arrays to calculate
 *                 intermediate features, which are then converted to
 *                 costs and combined to give Joint Cost
 * SIDE EFFECTS: Global arrays hzl_grad & vert_grad, OR, arrays hzl_tempf &
 *               vert_tempf are updated.
 * ENTRY CONDITIONS: hzl_tempf & vert_tempf are globally declared as pointers.
 *                   Space has to be allocated to these arrays before they
 *                   can be used.
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::AbsGrad1(int flag /* indicates whether to use feature or temp arrays */)
{
    int row, col;
	unsigned short **hzl, **vert; /* set to feature or temp depend. on flag */
	unsigned short **ptr, *ptc, *ptc1;
	int nrow1, nrow2;  /* variables used to decrease computing time */
    unsigned char *ptr8, *ptr8_1, *ptr8_2;
    unsigned short *ptr16, *ptr16_1, *ptr16_2;
 

	if(flag==1)
	{
		hzl = hzl_grad;
		vert = vert_grad;
	}
	else
	{
        hzl = hzl_tempf;
        vert = vert_tempf;
    }

	/***** Initialize hzl on row 0 & height to 0, and
                      vert on col 0 & width to 0 */
	for(col=0, ptc=*hzl, ptc1=*(hzl+orig.height); col<orig.width; ptc++,ptc1++,col++)
	{
		*ptc=0;		/* hzl[0][col]=0 */
		*ptc1=0;	/* hzl[orig.height][col]=0 */
	}
	for(ptr=vert, row=0; row<orig.height; ptr++, row++)
	{
		*((*ptr)) = 0;		/*  vert[row][0] */
		*((*ptr)+orig.width) = 0;	/* vert[row][orig.width] */
	}

    /***** Calculate unsigned absolute gradients *****/
    /** Handle 8-bit & 16-bit data appropriately  **/
    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++,row++)
		{
			nrow1 = *(tblr+row-1); /* *orig.width*/
			nrow2 = *(tblr+row); /* *orig.width */
			for(ptr8_1=ptr8+nrow1+0,ptr8_2=ptr8+nrow2+0,ptc=*ptr, col=0; col<orig.width;ptr8_1++,ptr8_2++, ptc++, col++)
                *ptc = abs( (int)*ptr8_1 - (int)*ptr8_2 );

            		/*for(col=0; col<orig.width; col++)
                		hzl[row][col]=abs((short)ptr8[nrow1 + col] -
                                   (short)ptr8[nrow2 + col]);*/
		}
 
        for(ptr=vert, row=0; row<orig.height; ptr++, row++)
		{
			nrow1 = *(tblr+row); /*    *orig.width */
			for(ptr8_1=ptr8+nrow1+1-1,ptr8_2=ptr8+nrow1+1, ptc=(*ptr)+1, col=1; col<orig.width; ptr8_1++,ptr8_2++, ptc++, col++)
				*ptc = abs((int)*ptr8_1 - (int)*ptr8_2 );

            		/*for(col=1; col<orig.width; col++)
                		vert[row][col]=abs((short)ptr8[nrow1 + col-1] -
                                   (short)ptr8[nrow1 + col]);*/
		}
    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
		{
			nrow1 = *(tblr+row-1);
			nrow2 = *(tblr+row);
			for(ptr16_1=ptr16+nrow1+0,ptr16_2=ptr16+nrow2+0, ptc=*ptr, col=0; col<orig.width;ptr16_1++,ptr16_2++, ptc++, col++)
				*ptc = abs((int)*ptr16_1 - (int)*ptr16_2);

            		/*for(col=0; col<orig.width; col++)
                		hzl[row][col]=abs((short)ptr16[nrow1 + col] -
                                  (short)ptr16[nrow2 + col]);*/
		}
 
        for(ptr=vert, row=0; row<orig.height; ptr++, row++)
		{
			nrow1 = *(tblr+row);
			for(ptr16_1=ptr16+nrow1+1-1,ptr16_2=ptr16+nrow1+1, ptc=(*ptr)+1, col=1; col<orig.width; ptr16_1++,ptr16_2++, ptc++, col++)
				*ptc = abs((int)*ptr16_1 - (int)*ptr16_2);

            		/*for(col=1; col<orig.width; col++)
                		vert[row][col]=abs((short)ptr16[nrow1 + col-1] -
                                   (short)ptr16[nrow1 + col]);*/
		}
    }/** endif 16-bit data **/
 

    return 0;
}
 
 
 



/*****************************************************************************
 * FUNCTION: AbsGrad2
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 6-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = |(a+b+c) - (d+e+f)| * 1/3
 *
 * PARAMETERS:
 *    flag  :  indicates whether to use feature or temp arrays 
 *             1 - use hzl_grad & vert_grad arrays to calculate 
 *                 current feature
 *             2 - use hzl_tempf & vert_tempf arrays to calculate
 *                 intermediate features, which are then converted to
 *                 costs and combined to give Joint Cost
 * SIDE EFFECTS: Global arrays hzl_grad & vert_grad, OR, arrays hzl_tempf &
 *               vert_tempf are updated.
 * ENTRY CONDITIONS: hzl_tempf & vert_tempf are globally declared as pointers.
 *                   Space has to be allocated to these arrays before they
 *                   can be used.
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::AbsGrad2(int flag /* indicates whether to use feature or temp arrays */)
{
    int row, col;
	unsigned short **hzl, **vert; /* set to feature or temp depend. on flag */
    unsigned short **ptr, *ptc, *ptc1;
    int nrow1, nrow2, nrow3;  /* variables used to decrease computing time */
    unsigned char *ptr8; /*, *ptr8_1, *ptr8_2;*/
    unsigned short *ptr16; /*, *ptr16_1, *ptr16_2;*/

    if(flag==1)
    {
        hzl = hzl_grad;
        vert = vert_grad;
    }
    else
    {
        hzl = hzl_tempf;
        vert = vert_tempf;
    }

    /***** Initialize hzl on row 0 & height to 0,
                                on col 0 & width to 0, and
                      vert on col 0 & width to 0
                                 on row 0 & height to 0 */
    for(col=0, ptc=*hzl, ptc1=*(hzl+orig.height); col<orig.width; ptc++,ptc1++,col++)
    {
        *ptc=0;		/* hzl[0][col] */
        *ptc1=0;	/* hzl[orig.height][col] */
    }
	for (row=1, ptr=hzl+1; row<orig.height; ptr++, row++)
    {
        *((*ptr))=0;	/* hzl[row][0] */
        *((*ptr)+orig.width-1)=0;	/* hzl[row][orig.width-1] */
    }
    for (row=0, ptr=vert; row<orig.height; ptr++, row++)
    {
        *((*ptr))=0;	/* vert[row][0] */
        *((*ptr)+orig.width)=0;	/* vert[row][orig.width] */
    }
	for(col=1, ptc=*vert+1, ptc1=*(vert+orig.height-1)+1; col<orig.width; ptc++, ptc1++, col++)
    {
        *ptc=0;	/* vert[0][col] */
        *ptc1=0;	/* vert[orig.height-1][col] */
    }
 
    /***** Calculate unsigned absolute gradients *****/
    /** Handle 8-bit & 16-bit data appropriately  **/
    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(ptc=(*ptr)+1, col=1; col<(orig.width-1); ptc++, col++)
					/*  hzl[row][col] */
                *ptc= (int)((1.0/3) *  abs(
              (int)ptr8[nrow1+col-1]+(int)ptr8[nrow1+col]+(int)ptr8[nrow1+col+1]
          -(int)ptr8[nrow2+col-1]-(int)ptr8[nrow2+col]-(int)ptr8[nrow2+col+1] ));
        }
 
        for(ptr=vert+1, row=1; row<(orig.height-1); ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+1);
            for(ptc=(*ptr)+1, col=1; col<orig.width; ptc++, col++)
					/*  vert[row][col] */
                *ptc= (int)((1.0/3) * abs(
            (int)ptr8[nrow1+col-1]+(int)ptr8[nrow2+col-1]+(int)ptr8[nrow3+col-1]
           -(int)ptr8[nrow1+col]-(int)ptr8[nrow2+col]-(int)ptr8[nrow3+col] ));
        }
    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(ptc=(*ptr)+1, col=1; col<(orig.width-1); ptc++, col++)
					/*  hzl[row][col]  */
                *ptc= (int)((1.0/3) * abs(
           (int)ptr16[nrow1+col-1]+(int)ptr16[nrow1+col]+(int)ptr16[nrow1+col+1]
        -(int)ptr16[nrow2+col-1]-(int)ptr16[nrow2+col]-(int)ptr16[nrow2+col+1]));
        }
 
		for(ptr=vert+1, row=1; row<(orig.height-1); ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+1);
            for(ptc=(*ptr)+1, col=1; col<orig.width; ptc++, col++)
					/*  vert[row][col]  */
                *ptc= (int)((1.0/3) * abs(
         (int)ptr16[nrow1+col-1]+(int)ptr16[nrow2+col-1]+(int)ptr16[nrow3+col-1]
        -(int)ptr16[nrow1+col]-(int)ptr16[nrow2+col]-(int)ptr16[nrow3+col] ));
        }
    }/** endif 16-bit data **/
 
 
    return 0;
}
 





/*****************************************************************************
 * FUNCTION: AbsGrad3
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 6-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = |(0.5a+b+0.5c) - (0.5d+e+0.5f)|/2
 *
 * PARAMETERS:
 *    flag  :  indicates whether to use feature or temp arrays 
 *             1 - use hzl_grad & vert_grad arrays to calculate 
 *                 current feature
 *             2 - use hzl_tempf & vert_tempf arrays to calculate
 *                 intermediate features, which are then converted to
 *                 costs and combined to give Joint Cost
 * SIDE EFFECTS: Global arrays hzl_grad & vert_grad, OR, arrays hzl_tempf &
 *               vert_tempf are updated.
 * ENTRY CONDITIONS: hzl_tempf & vert_tempf are globally declared as pointers.
 *                   Space has to be allocated to these arrays before they
 *                   can be used.
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::AbsGrad3(int flag /* indicates whether to use feature or temp arrays */)
{
    int row, col;
	unsigned short **hzl, **vert; /* set to feature or temp depend. on flag */
	unsigned short **ptr, *ptc, *ptc1;
    int nrow1, nrow2, nrow3;  /* variables used to decrease computing time */
    unsigned char *ptr8; 
    unsigned short *ptr16;
 
    if(flag==1 /*curr_feature*/ ) 
    {
        hzl = hzl_grad;
        vert = vert_grad;
    }
    else
    {
        hzl = hzl_tempf;
        vert = vert_tempf;
    }

    /***** Initialize hzl on row 0 & height to 0,
                                on col 0 & width to 0, and
                      vert on col 0 & width to 0
                                 on row 0 & height to 0 */
    for (col=0, ptc=*hzl, ptc1=*(hzl+orig.height); col<orig.width; ptc++,ptc1++,col++)
    {
        *ptc=0;	/* hzl[0][col] */
        *ptc1=0;	/* hzl[orig.height][col] */
    }
    for (row=1, ptr=hzl+1; row<orig.height; ptr++, row++)
    {
        *((*ptr))=0;	/* hzl[row][0] */
        *((*ptr)+orig.width-1)=0;	/* hzl[row][orig.width-1] */
    }
    for (row=0, ptr=vert; row<orig.height; ptr++, row++)
    {
        *((*ptr))=0;	/* vert[row][0] */
        *((*ptr)+orig.width)=0;	/* vert[row][orig.width] */
    }
    for (col=1, ptc=*vert+1, ptc1=*(vert+orig.height-1)+1; col<orig.width; ptc++, ptc1++, col++)
    {
        *ptc=0;	/* vert[0][col] */
        *ptc1=0;	/* vert[orig.height-1][col] */
    }
 
    /***** Calculate unsigned absolute gradients *****/
    /** Handle 8-bit & 16-bit data appropriately  **/
    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(ptc=(*ptr)+1, col=1; col<(orig.width-1); ptc++, col++)
					/*  hzl[row][col]  */
                *ptc =  abs(
				    ( (int)ptr8[nrow1+col-1]+(int)ptr8[nrow1+col+1]
				     -(int)ptr8[nrow2+col-1]-(int)ptr8[nrow2+col+1] )/2
				  +(int)ptr8[nrow1+col]-(int)ptr8[nrow2+col] )/2 ;
        }
 
        for(ptr=vert+1, row=1; row<(orig.height-1); ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
			nrow3 = *(tblr+row+1);
            for(ptc=(*ptr)+1, col=1; col<orig.width; ptc++, col++)
					/*  vert[row][col]  */
                *ptc =  abs(
				     ((int)ptr8[nrow1+col-1]+(int)ptr8[nrow3+col-1]
				    -(int)ptr8[nrow1+col]-(int)ptr8[nrow3+col] )/2
				    +(int)ptr8[nrow2+col-1]-(int)ptr8[nrow2+col] )/2;
 
        }
    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(ptc=(*ptr)+1, col=1; col<(orig.width-1); ptc++, col++)
					/*  hzl[row][col] */
                *ptc =  abs(
				  ( (int)ptr16[nrow1+col-1]+(int)ptr16[nrow1+col+1]
                   -(int)ptr16[nrow2+col-1]-(int)ptr16[nrow2+col+1] )/2
                  +(int)ptr16[nrow1+col]-(int)ptr16[nrow2+col] )/2;
        }
 
        for(ptr=vert+1, row=1; row<(orig.height-1); ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
			nrow3 = *(tblr+row+1);
            for(ptc=(*ptr)+1, col=1; col<orig.width; ptc++, col++)
					/*  vert[row][col]  */
				*ptc =  abs(
                     ((int)ptr16[nrow1+col-1]+(int)ptr16[nrow3+col-1]
                    -(int)ptr16[nrow1+col]-(int)ptr16[nrow3+col] )/2
                    +(int)ptr16[nrow2+col-1]-(int)ptr16[nrow2+col] )/2;
 
        }
    }/** endif 16-bit data **/
 
 
    return 0;
}








/*****************************************************************************
 * FUNCTION: AbsGrad4
 * DESCRIPTION: Finds the unsigned density gradient across the pixel edge, 
 *              based on a 6-pixel neighbourhood. Pictorially shown below:
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = ( |a-e| + |b-d| + |b-f| + |c-e| )/4
 *
 * PARAMETERS:
 *    flag  :  indicates whether to use feature or temp arrays 
 *             1 - use hzl_grad & vert_grad arrays to calculate 
 *                 current feature
 *             2 - use hzl_tempf & vert_tempf arrays to calculate
 *                 intermediate features, which are then converted to
 *                 costs and combined to give Joint Cost
 * SIDE EFFECTS: Global arrays hzl_grad & vert_grad, OR, arrays hzl_tempf &
 *               vert_tempf are updated.
 * ENTRY CONDITIONS: hzl_tempf & vert_tempf are globally declared as pointers.
 *                   Space has to be allocated to these arrays before they
 *                   can be used.
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::AbsGrad4(int flag /* indicates whether to use feature or temp arrays */)
{
    int row, col;
	unsigned short **hzl, **vert; /* set to feature or temp depend. on flag */
	unsigned short **ptr, *ptc, *ptc1;
    int nrow1, nrow2, nrow3;  /* variables used to decrease computing time */
    unsigned char *ptr8; /*, *ptr8_1, *ptr8_2;*/
    unsigned short *ptr16; /*, *ptr16_1, *ptr16_2;*/
 
    if(flag==1)
    {
        hzl = hzl_grad;
        vert = vert_grad;
    }
    else
    {
        hzl = hzl_tempf;
        vert = vert_tempf;
    }

    /***** Initialize hzl on row 0 & height to 0,
                                on col 0 & width to 0, and
                      vert on col 0 & width to 0
                                 on row 0 & height to 0 */
    for (col=0, ptc=*hzl, ptc1=*(hzl+orig.height); col<orig.width; ptc++,ptc1++,col++)
    {
        *ptc=0;	/* hzl[0][col] */
        *ptc1=0;	/* hzl[orig.height][col] */
    }
    for (row=1, ptr=hzl+1; row<orig.height; ptr++, row++)
    {
		*((*ptr))=0;	/* hzl[row][0] */
		*((*ptr)+orig.width-1)=0;	/* hzl[row][orig.width-1] */
    }
    for (row=0, ptr=vert; row<orig.height; ptr++, row++)
    {
        *((*ptr))=0;	/* vert[row][0] */
        *((*ptr)+orig.width)=0;	/* vert[row][orig.width] */
    }
    for (col=1, ptc=*vert+1, ptc1=*(vert+orig.height-1)+1; col<orig.width; ptc++, ptc1++, col++)
    {
        *ptc=0;	/* vert[0][col] */
        *ptc1=0;	/* vert[orig.height-1][col] */
    }
 
    /***** Calculate unsigned absolute gradients *****/
    /** Handle 8-bit & 16-bit data appropriately  **/
    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(ptc=(*ptr)+1, col=1; col<(orig.width-1); ptc++, col++)
					/* hzl[row][col] */
                *ptc = ( abs((int)ptr8[nrow1+col-1]-(int)ptr8[nrow2+col]) +
                         abs((int)ptr8[nrow1+col]-(int)ptr8[nrow2+col-1]) +
                         abs((int)ptr8[nrow1+col]-(int)ptr8[nrow2+col+1]) +
                         abs((int)ptr8[nrow1+col+1]-(int)ptr8[nrow2+col]) )/4;
        }
 
        for(ptr=vert+1, row=1; row<(orig.height-1); ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+1);
            for(ptc=(*ptr)+1, col=1; col<orig.width; ptc++, col++)
					/*  vert[row][col]  */
				*ptc = ( abs((int)ptr8[nrow1+col-1]-(int)ptr8[nrow2+col]) +
                         abs((int)ptr8[nrow2+col-1]-(int)ptr8[nrow1+col]) +
                         abs((int)ptr8[nrow2+col-1]-(int)ptr8[nrow3+col]) +
                         abs((int)ptr8[nrow3+col-1]-(int)ptr8[nrow2+col]) )/4;
 
            /*for(ptr8_1=ptr8+nrow1+1-1,ptr8_2=ptr8+nrow1+1,col=1; col<orig.widt
h; ptr8_1++,ptr8_2++, col++)
                vert[row][col]=abs((int)(*ptr8_1) - (int)(*ptr8_2));*/
        }
    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
        for(ptr=hzl+1, row=1; row<orig.height; ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(ptc=(*ptr)+1, col=1; col<(orig.width-1); ptc++, col++)
					/*  hzl[row][col]  */
				*ptc = ( abs((int)ptr16[nrow1+col-1]-(int)ptr16[nrow2+col]) +
                         abs((int)ptr16[nrow1+col]-(int)ptr16[nrow2+col-1]) +
                         abs((int)ptr16[nrow1+col]-(int)ptr16[nrow2+col+1]) +
                         abs((int)ptr16[nrow1+col+1]-(int)ptr16[nrow2+col]) )/4;
        }
 
        for(ptr=vert+1, row=1; row<(orig.height-1); ptr++, row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+1);
            for(ptc=(*ptr)+1, col=1; col<orig.width; ptc++, col++)
					/*  vert[row][col]  */
				*ptc = ( abs((int)ptr16[nrow1+col-1]-(int)ptr16[nrow2+col]) +
                         abs((int)ptr16[nrow2+col-1]-(int)ptr16[nrow1+col]) +
                         abs((int)ptr16[nrow2+col-1]-(int)ptr16[nrow3+col]) +
                         abs((int)ptr16[nrow3+col-1]-(int)ptr16[nrow2+col]) )/4;
        }
    }/** endif 16-bit data **/
 
 
    return 0;
}
 

 






/*****************************************************************************
 * FUNCTION: Density1
 * DESCRIPTION: Finds the higher of the densities on either side of an edge
 *              (viz., head of the gradient vector), and assigns that value
 *              to the edge feature.
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = 1/3 * [((a+b+c) > (d+e+f)) ? (a+b+c) : (d+e+f)]
 *
 * PARAMETERS:
 *    flag  :  indicates whether to use feature or temp arrays 
 *             1 - use hzl_grad & vert_grad arrays to calculate 
 *                 current feature
 *             2 - use hzl_tempf & vert_tempf arrays to calculate
 *                 intermediate features, which are then converted to
 *                 costs and combined to give Joint Cost
 * SIDE EFFECTS: Global arrays hzl_grad & vert_grad, OR, arrays hzl_tempf &
 *               vert_tempf are updated.
 * ENTRY CONDITIONS: hzl_tempf & vert_tempf are globally declared as pointers.
 *                   Space has to be allocated to these arrays before they
 *                   can be used.
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 5/22/08 border conditions and overflow corrected
 *       by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::Density1(int flag /* indicates whether to use feature or temp arrays */)
{
    int row, col;
    unsigned short **hzl, **vert; /* set to feature or temp depend. on flag */
    int nrow1, nrow2, nrow3;  /* variables used to decrease computing time */
    unsigned char *ptr8;
    unsigned short *ptr16;
	unsigned int val1, val2;
 
    if(flag==1)
    {
        hzl = hzl_grad;
        vert = vert_grad;
    }
    else
    {
        hzl = hzl_tempf;
        vert = vert_tempf;
    }

    /***** Calculate unsigned absolute gradients *****/
    /** Handle 8-bit & 16-bit data appropriately  **/
    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
		for(col=0; col<orig.width; col++)
		{
			val2 = (int)ptr8[col? col-1: 0]+
			       (int)ptr8[col]+
				   (int)ptr8[col<orig.width-1? col+1: col];
			hzl[0][col] = val2/3;
			nrow1 = tblr[orig.height-1];
			val1 = (int)ptr8[col? nrow1+col-1: nrow1+col]+
			       (int)ptr8[nrow1+col]+
				   (int)ptr8[col<orig.width-1? nrow1+col+1: nrow1+col];
			hzl[orig.height][col] = val1/3;
		}
        for(row=1; row<orig.height; row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
			val1 = 2*ptr8[nrow1]+ptr8[nrow1+1];
			val2 = 2*ptr8[nrow2]+ptr8[nrow2+1];
			hzl[row][0]= (val1>val2 ? val1 : val2)/3;
			col = orig.width-1;
			val1 = ptr8[nrow1+col-1]+2*ptr8[nrow1+col];
			val2 = ptr8[nrow2+col-1]+2*ptr8[nrow2+col];
			hzl[row][col]= (val1>val2 ? val1 : val2)/3;
            for(col=1; col<(orig.width-1); col++)
			{
				val1 = (int)ptr8[nrow1+col-1]+(int)ptr8[nrow1+col]+(int)ptr8[nrow1+col+1];
				val2 = (int)ptr8[nrow2+col-1]+(int)ptr8[nrow2+col]+(int)ptr8[nrow2+col+1];
				hzl[row][col]= (val1>val2 ? val1 : val2)/3; 
			}
        }

        for(row=0; row<orig.height; row++)
        {
            nrow1 = *(tblr+row-(row>0));
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+(row<orig.height-1));
			val2 = (int)ptr8[nrow1]+(int)ptr8[nrow2]+(int)ptr8[nrow3];
			vert[row][0] = val2/3;
			col = orig.width;
			val1 = (int)ptr8[nrow1+col-1]+(int)ptr8[nrow2+col-1]+(int)ptr8[nrow3+col-1];
			vert[row][col] = val1/3;
            for(col=1; col<orig.width; col++)
			{
				val1 = (int)ptr8[nrow1+col-1]+(int)ptr8[nrow2+col-1]+(int)ptr8[nrow3+col-1];
				val2 = (int)ptr8[nrow1+col]+(int)ptr8[nrow2+col]+(int)ptr8[nrow3+col];
                vert[row][col]= (val1>val2 ? val1 : val2)/3;
			}
        }
    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
		for(col=0; col<orig.width; col++)
		{
			val2 = (int)ptr16[col? col-1: 0]+
			       (int)ptr16[col]+
				   (int)ptr16[col<orig.width-1? col+1: col];
			hzl[0][col] = val2/3;
			nrow1 = tblr[orig.height-1];
			val1 = (int)ptr16[col? nrow1+col-1: nrow1+col]+
			       (int)ptr16[nrow1+col]+
				   (int)ptr16[col<orig.width-1? nrow1+col+1: nrow1+col];
			hzl[orig.height][col] = val1/3;
		}
        for(row=1; row<orig.height; row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
			val1 = 2*ptr16[nrow1]+ptr16[nrow1+1];
			val2 = 2*ptr16[nrow2]+ptr16[nrow2+1];
			hzl[row][0]= (val1>val2 ? val1 : val2)/3;
			col = orig.width-1;
			val1 = ptr16[nrow1+col-1]+2*ptr16[nrow1+col];
			val2 = ptr16[nrow2+col-1]+2*ptr16[nrow2+col];
			hzl[row][col]= (val1>val2 ? val1 : val2)/3;
            for(col=1; col<(orig.width-1); col++)
			{
				val1 = (int)ptr16[nrow1+col-1]+(int)ptr16[nrow1+col]+(int)ptr16[nrow1+col+1];
				val2 = (int)ptr16[nrow2+col-1]+(int)ptr16[nrow2+col]+(int)ptr16[nrow2+col+1];
				hzl[row][col]= (val1>val2 ? val1 : val2)/3; 
			}
        }

        for(row=0; row<orig.height; row++)
        {
            nrow1 = *(tblr+row-(row>0));
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+(row<orig.height-1));
			val2 = (int)ptr16[nrow1]+(int)ptr16[nrow2]+(int)ptr16[nrow3];
			vert[row][0] = val2/3;
			col = orig.width;
			val1 = (int)ptr16[nrow1+col-1]+(int)ptr16[nrow2+col-1]+(int)ptr16[nrow3+col-1];
			vert[row][col] = val1/3;
            for(col=1; col<orig.width; col++)
			{
                val1 = (int)ptr16[nrow1+col-1]+(int)ptr16[nrow2+col-1]+(int)ptr16[nrow3+col-1];
                val2 = (int)ptr16[nrow1+col]+(int)ptr16[nrow2+col]+(int)ptr16[nrow3+col];
                vert[row][col]= (val1>val2 ? val1 : val2)/3;
            }
        }
    }/** endif 16-bit data **/
 
 
    return 0;
}









/*****************************************************************************
 * FUNCTION: Density2
 * DESCRIPTION: Finds the lower of the densities on either side of an edge
 *              (viz., tail of the gradient vector), and assigns that value
 *              to the edge feature.
 *       For edge between pixels [b] and [e] --
 *       _____________
 *       | a | b | c |
 *       _____________
 *       | d | e | f |
 *       _____________
 *
 *     feature = 1/3 * [((a+b+c) < (d+e+f)) ? (a+b+c) : (d+e+f)]
 *
 * PARAMETERS:
 *    flag  :  indicates whether to use feature or temp arrays 
 *             1 - use hzl_grad & vert_grad arrays to calculate 
 *                 current feature
 *             2 - use hzl_tempf & vert_tempf arrays to calculate
 *                 intermediate features, which are then converted to
 *                 costs and combined to give Joint Cost
 * SIDE EFFECTS: Global arrays hzl_grad & vert_grad, OR, arrays hzl_tempf &
 *               vert_tempf are updated.
 * ENTRY CONDITIONS: hzl_tempf & vert_tempf are globally declared as pointers.
 *                   Space has to be allocated to these arrays before they
 *                   can be used.
 * RETURN VALUE: none
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 5/22/08 border conditions and overflow corrected
 *       by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::Density2(int flag /* indicates whether to use feature or temp arrays */)
{
    int row, col;
    unsigned short **hzl, **vert; /* set to feature or temp depend. on flag */
    unsigned short **ptr1, *ptc1, *ptc2;
    int nrow1, nrow2, nrow3;  /* variables used to decrease computing time */
    unsigned char *ptr8;
    unsigned short *ptr16;
    unsigned int val1, val2;
 
    if(flag==1)
    {
        hzl = hzl_grad;
        vert = vert_grad;
    }
    else
    {
        hzl = hzl_tempf;
        vert = vert_tempf;
    }
 
    /***** Initialize hzl on row 0 & height to Imin (not to 0!!),
                                on col 0 & width to Imin, and
                      vert on col 0 & width to Imin 
                                 on row 0 & height to Imin */
    for (col=0, ptc1=&hzl[0][0], ptc2=&hzl[orig.height][0]; col<orig.width;
		ptc1++,ptc2++,col++)
    {
        *ptc1=(unsigned short)Imin;
        *ptc2=(unsigned short)Imin;
    }
    for (row=0, ptr1=&hzl[0]; row<orig.height; ptr1++, row++)
    {
        vert[row][0]=(unsigned short)Imin;
        vert[row][orig.width]=(unsigned short)Imin;
    }
 
    /***** Calculate unsigned absolute gradients *****/
    /** Handle 8-bit & 16-bit data appropriately  **/
    if (orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
	    for (row=1; row<orig.height; row++)
	    {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
			val1 = (int)ptr8[nrow1]+(int)ptr8[nrow1+1];
			val2 = (int)ptr8[nrow2]+(int)ptr8[nrow2+1];
	        hzl[row][0] = (val1<val2 ? val1 : val2)/3;
			val1 = (int)ptr8[nrow1+orig.width-2]+(int)ptr8[nrow1+orig.width-1];
			val2 = (int)ptr8[nrow2+orig.width-2]+(int)ptr8[nrow2+orig.width-1];
	        hzl[row][orig.width-1] = (val1<val2 ? val1 : val2)/3;
	    }
		nrow1 = tblr[orig.height-2];
		nrow2 = tblr[orig.height-1];
	    for (col=1; col<orig.width; col++)
	    {
			val1 = (int)ptr8[col-1]+(int)ptr8[tblr[1]+col-1];
			val2 = (int)ptr8[col]+(int)ptr8[tblr[1]+col];
	        vert[0][col] = (val1<val2 ? val1 : val2)/3;
			val1 = (int)ptr8[nrow1+col-1]+(int)ptr8[nrow2+col-1];
			val2 = (int)ptr8[nrow1+col]+(int)ptr8[nrow2+col];
	        vert[orig.height-1][col] = (val1<val2 ? val1 : val2)/3;
	    }
        for(row=1; row<orig.height; row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(col=1; col<(orig.width-1); col++)
            {
                val1 = (int)ptr8[nrow1+col-1]+(int)ptr8[nrow1+col]+(int)ptr8[nrow1+col+1];
                val2 = (int)ptr8[nrow2+col-1]+(int)ptr8[nrow2+col]+(int)ptr8[nrow2+col+1];
                hzl[row][col]= (val1<val2 ? val1 : val2)/3;
            }
        }
 
        for(row=1; row<(orig.height-1); row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+1);
            for(col=1; col<orig.width; col++)
            {
                val1 = (int)ptr8[nrow1+col-1]+(int)ptr8[nrow2+col-1]+(int)ptr8[nrow3+col-1];
                val2 = (int)ptr8[nrow1+col]+(int)ptr8[nrow2+col]+(int)ptr8[nrow3+col];
                vert[row][col]= (val1<val2 ? val1 : val2)/3;
            }
        }
    }/** endif 8-bit data **/
    else
    if (orig.bits == 16)
    {
        ptr16 = (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
	    for (row=1; row<orig.height; row++)
	    {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
			val1 = (int)ptr16[nrow1]+(int)ptr16[nrow1+1];
			val2 = (int)ptr16[nrow2]+(int)ptr16[nrow2+1];
	        hzl[row][0] = (val1<val2 ? val1 : val2)/3;
			val1=(int)ptr16[nrow1+orig.width-2]+(int)ptr16[nrow1+orig.width-1];
			val2=(int)ptr16[nrow2+orig.width-2]+(int)ptr16[nrow2+orig.width-1];
	        hzl[row][orig.width-1] = (val1<val2 ? val1 : val2)/3;
	    }
		nrow1 = tblr[orig.height-2];
		nrow2 = tblr[orig.height-1];
	    for (col=1; col<orig.width; col++)
	    {
			val1 = (int)ptr16[col-1]+(int)ptr16[tblr[1]+col-1];
			val2 = (int)ptr16[col]+(int)ptr16[tblr[1]+col];
	        vert[0][col] = (val1<val2 ? val1 : val2)/3;
			val1 = (int)ptr16[nrow1+col-1]+(int)ptr16[nrow2+col-1];
			val2 = (int)ptr16[nrow1+col]+(int)ptr16[nrow2+col];
	        vert[orig.height-1][col] = (val1<val2 ? val1 : val2)/3;
	    }
        for(row=1; row<orig.height; row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            for(col=1; col<(orig.width-1); col++)
            {
                val1 = (int)ptr16[nrow1+col-1]+(int)ptr16[nrow1+col]+(int)ptr16[nrow1+col+1];
                val2 = (int)ptr16[nrow2+col-1]+(int)ptr16[nrow2+col]+(int)ptr16[nrow2+col+1];
                hzl[row][col]= (val1<val2 ? val1 : val2)/3;
            }
        }
 
        for(row=1; row<(orig.height-1); row++)
        {
            nrow1 = *(tblr+row-1);
            nrow2 = *(tblr+row);
            nrow3 = *(tblr+row+1);
            for(col=1; col<orig.width; col++)
            {
                val1 = (int)ptr16[nrow1+col-1]+(int)ptr16[nrow2+col-1]+(int)ptr16[nrow3+col-1];
                val2 = (int)ptr16[nrow1+col]+(int)ptr16[nrow2+col]+(int)ptr16[nrow3+col];
                vert[row][col]= (val1<val2 ? val1 : val2)/3;
            }
        }
    }/** endif 16-bit data **/
 
 
    return 0;
}


/*****************************************************************************
 * FUNCTION: Reset_ObjectVertex
 * DESCRIPTION: Resets the entire object_vertex array to OFF for all objects
 * PARAMETERS: None
 * SIDE EFFECTS: Global object_vertex[][] is reset to 0
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
/* resets object_vertex array to 0 */
void Segment2dCanvas::Reset_ObjectVertex()
{
	int row, col;

    /******* reset object_vertex array to 0 ***********/
    for(row=0; row<=orig.height; row++)
        for(col=0; col<=orig.width; col++)
            object_vertex[row][col]=0;
 

}







/*****************************************************************************
 * FUNCTION: reset_object_vertex_of_temparrays
 * DESCRIPTION: Scans the temp_contours[] arrays, and sets the corresponding
 *              vertices to OFF
 * PARAMETERS: None
 * SIDE EFFECTS: Global object_vertex[][] is changed
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
/* resets object_vertex array corresp to temp arrays to 0 */
void Segment2dCanvas::reset_object_vertex_of_temparrays()
{
	int i;

    if(temp_contours.last > 0)
		for(i=0; i<temp_contours.last; i++) /* stop 1 short of end */
   		{
			object_vertex[temp_contours.vertex[i].y][temp_contours.vertex[i].x] &= offbit[object_number];
		}
}


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                                                                           **
**                                                                           **
**                            TRANSFORMS                                     **
**                                                                           **
**                                                                           **
**                                                                           **
**                                                                           **
*******************************************************************************
*******************************************************************************/
 
 
/*****************************************************************************
 * FUNCTION: InvLinearTransform
 * DESCRIPTION: Applies an InverseLinear Transform to convert Edge Features
 *              into Edge Costs.
 *
 *              |
 *              |   +    
 *         COST |    +  
 *              |     +
 *              |      +
 *              |       +
 *              _________________
 *                  FEATURE
 *
 *       If x < fmin,   cost(x) =  Imax-Imin
 *       If x > fmax,   cost(x) =  Imax-Imin
 *       Else           cost(x) =  slope*(x-fmax)
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    flag  - determines whether the actual edge features and costs arrays
 *            will be used, or the temporary features and costs arrays.
 *            The rationale for using temporary arrays is that, the actual
 *            features/costs arrays store the accumulated costs for several
 *            features, while the individual features & associated costs are
 *            stored in temporary arrays, before being accumulated into the
 *            actual arrays.
 *            When = 1  ==> use actual arrays
 *            When = 2  ==> use temporary arrays
 * SIDE EFFECTS: Updates values of individual or accumulated costs, depending
 *               upon whether flag is set to 1 or 2 (see above).
 * ENTRY CONDITIONS:  None
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::InvLinearTransform(int featr, int flag)
{
  int row, col;
  float slope, Fmax, Fmin;
  unsigned short **hzlf, **vertf, **hzlc, **vertc;
  unsigned short *ptc1, **ptr1, *ptc2, **ptr2;
  
  if(flag==1)
    {
      hzlf = hzl_grad;
      vertf = vert_grad;
      hzlc = hzl_cost;
      vertc = vert_cost;
    }
  else
    {
      hzlf = hzl_tempf;
      vertf = vert_tempf;
      hzlc = hzl_tempc;
      vertc = vert_tempc;
    }
  
  
  
  slope = 1.0/(temp_list[featr].rmin-temp_list[featr].rmax);
  
  if( featr==0 || featr==1 )
    {
      Fmin = Imin + (temp_list[featr].rmin*Range);
      Fmax = Imin + (temp_list[featr].rmax*Range);
    }
  else
    {
      Fmin = (temp_list[featr].rmin*Range);
      Fmax = (temp_list[featr].rmax*Range);
    }

  for(ptr1=hzlf, ptr2=hzlc, row=0; row<=orig.height; ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<orig.width; ptc1++,ptc2++, col++)
      {
	if(*ptc1 < Fmin )
	  *ptc2 = (unsigned short)(Imax-Imin);
	else
	  if(*ptc1 > Fmax )
	    *ptc2 = (unsigned short)(Imax-Imin);
	  else
	    *ptc2= (unsigned short)(slope*(*ptc1-Fmax));
      }

  for(ptr1=vertf, ptr2=vertc, row=0; row<orig.height; ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<=orig.width; ptc1++,ptc2++,col++)
      {
	if(*ptc1 < Fmin )
	  *ptc2 = (unsigned short)(Imax-Imin);
	else
	  if(*ptc1 > Fmax )
	    *ptc2 = (unsigned short)(Imax-Imin);
	  else
	    *ptc2= (unsigned short)(slope*(*ptc1-Fmax));
      }

  return 0;
}






/*****************************************************************************
 * FUNCTION: InvGaussianTransform
 * DESCRIPTION: Applies an Inverse Gaussian to transform Edge Features
 *              into Edge Costs.
 *
 *              |
 *              |   --             --
 *              |      \          /
 *              |       \        /
 *         COST |         \    /
 *              |           --
 *              ________________________
 *                     FEATURE
 *
 *        cost(x) = Imax - (Imax-Imin)* e**(-(x-mean)**2/(2*stddev**2) )
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    flag  - determines whether the actual edge features and costs arrays
 *            will be used, or the temporary features and costs arrays.
 *            The rationale for using temporary arrays is that, the actual
 *            features/costs arrays store the accumulated costs for several
 *            features, while the individual features & associated costs are
 *            stored in temporary arrays, before being accumulated into the
 *            actual arrays.
 *            When = 1  ==> use actual arrays
 *            When = 2  ==> use temporary arrays
 * SIDE EFFECTS: Updates values of individual or accumulated costs, depending
 *               upon whether flag is set to 1 or 2 (see above).
 * ENTRY CONDITIONS:  None
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 05/05/94 Modified by Supun to use table lookup.
 *    Modified: 05/27/08 table expanded, freed by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::InvGaussianTransform(int featr, int flag)
{
  int row, col, tbl_offset,i;
  double square, fvar;
  unsigned short *ptc1, *ptc2, **ptr1, **ptr2, *tbl;
  unsigned short **hzlf, **vertf, **hzlc, **vertc;
  float fmean,mean_offset;
  
  if(flag==1)
    {
      hzlf = hzl_grad;
      vertf = vert_grad;
      hzlc = hzl_cost;
      vertc = vert_cost;
    }
  else
    {
      hzlf = hzl_tempf;
      vertf = vert_tempf;
      hzlc = hzl_tempc;
      vertc = vert_tempc;
    }
  
  /** Setup these constants for the comptation **/
  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  
  fvar = -2.0*(temp_list[featr].rstddev*Range)*(temp_list[featr].rstddev*Range);
 
  /** Table Lookup added by supun */
  if( featr==0 || featr==1 ) {
    tbl_offset=(int)Imin;
    mean_offset=Imin-fmean;
  }
  else {
    tbl_offset= 0;
    mean_offset= -fmean;
  }

  tbl=(unsigned short *)malloc(sizeof(short)*(Range+1));
  if (tbl==NULL) {
    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    return 1;
  }
  for (i=0; i<=Range; i++) {
    square= (i+mean_offset)*(i+mean_offset);
    tbl[i]=(unsigned short) rint(Range*(1.0 - exp((double)square/fvar)));
  }
  
  /*** Use inverse Gaussian transform to compute Costs **/
  for(ptr1=hzlf, ptr2=hzlc, row=0; row<(orig.height+1); ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<orig.width; ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }
  
  for(ptr1=vertf, ptr2=vertc, row=0; row<orig.height; ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<(orig.width+1); ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }

  free(tbl);

  return 0;
}







/*****************************************************************************
 * FUNCTION: LinearTransform
 * DESCRIPTION: Applies a Linear transform to convert Edge Features
 *              into Edge Costs.
 *
 *              |
 *              |       +     
 *         COST |      +        
 *              |     +           
 *              |    +             
 *              |   +                
 *              _________________
 *                  FEATURE
 *
 *       If x < fmin,   cost(x) =  Imax-Imin
 *       If x > fmax,   cost(x) =  Imax-Imin
 *       Else           cost(x) =  slope*(x-fmin)
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    flag  - determines whether the actual edge features and costs arrays
 *            will be used, or the temporary features and costs arrays.
 *            The rationale for using temporary arrays is that, the actual
 *            features/costs arrays store the accumulated costs for several
 *            features, while the individual features & associated costs are
 *            stored in temporary arrays, before being accumulated into the
 *            actual arrays.
 *            When = 1  ==> use actual arrays
 *            When = 2  ==> use temporary arrays
 * SIDE EFFECTS: Updates values of individual or accumulated costs, depending
 *               upon whether flag is set to 1 or 2 (see above).
 * ENTRY CONDITIONS:  None
 * RETURN VALUE:
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
int Segment2dCanvas::LinearTransform(int featr, int flag)
{
    int row, col;
    float slope, Fmax, Fmin;
    unsigned short **hzlf, **vertf, **hzlc, **vertc;
	unsigned short *ptc1, **ptr1, *ptc2, **ptr2;
 
    if(flag==1)
    {
        hzlf = hzl_grad;
        vertf = vert_grad;
        hzlc = hzl_cost;
        vertc = vert_cost;
    }
    else
    {
        hzlf = hzl_tempf;
        vertf = vert_tempf;
        hzlc = hzl_tempc;
        vertc = vert_tempc;
    }

    slope = 1.0/(temp_list[featr].rmax-temp_list[featr].rmin);
 
	if( featr==0 || featr==1 )
	{
		Fmin = Imin + (temp_list[featr].rmin*Range);
		Fmax = Imin + (temp_list[featr].rmax*Range);
	}
	else
	{
        Fmin = (temp_list[featr].rmin*Range);
        Fmax = (temp_list[featr].rmax*Range);
    }

    for(ptr1=hzlf, ptr2=hzlc, row=0; row<=orig.height; ptr1++, ptr2++, row++)
        for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<orig.width; ptc1++,ptc2++, col++)
        {
			if(*ptc1 < Fmin )
                *ptc2 = (unsigned short)(Imax-Imin);
            else
            if(*ptc1 > Fmax )
                *ptc2 = (unsigned short)(Imax-Imin);
            else
                *ptc2= (unsigned short)(slope*(*ptc1-Fmin));
        }
 
	for(ptr1=vertf, ptr2=vertc, row=0; row<orig.height; ptr1++, ptr2++, row++)
        for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<=orig.width; ptc1++,ptc2++,col++)
        {
            if(*ptc1 < Fmin )
                *ptc2 = (unsigned short)(Imax-Imin);
            else
            if(*ptc1 > Fmax )
                *ptc2 = (unsigned short)(Imax-Imin);
            else
                *ptc2= (unsigned short)(slope*(*ptc1-Fmin));
        }

    return 0;
}


/*****************************************************************************
 * FUNCTION: GaussianTransform
 * DESCRIPTION: Applies an Upright Gaussian to transform Edge Features
 *              into Edge Costs.
 *               
 *              |
 *              |           --
 *         COST |         /    \
 *              |       /        \
 *              |      /          \
 *              |   --             --
 *              ________________________
 *                     FEATURE
 *
 *        cost(x) = Imin + (Imax-Imin)* e**(-(x-mean)**2/(2*stddev**2) )
 * PARAMETERS:
 *    featr - is the feature number, used to access the parameters
 *            mean and std_dev appropriate to the feature.
 *    flag  - determines whether the actual edge features and costs arrays
 *            will be used, or the temporary features and costs arrays.
 *            The rationale for using temporary arrays is that, the actual
 *            features/costs arrays store the accumulated costs for several
 *            features, while the individual features & associated costs are
 *            stored in temporary arrays, before being accumulated into the
 *            actual arrays.
 *            When = 1  ==> use actual arrays
 *            When = 2  ==> use temporary arrays
 * SIDE EFFECTS: Updates values of individual or accumulated costs, depending
 *               upon whether flag is set to 1 or 2 (see above).
 * ENTRY CONDITIONS:  None
 * RETURN VALUE: 
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93  Shoba Sharma
 *    Modified: 05/05/94 Modified by Supun to use table lookup.
 *    Modified: 05/27/08 table expanded, freed by Dewey Odhner.
 *
 *****************************************************************************/
int Segment2dCanvas::GaussianTransform(int featr, int flag)
{
  int row, col, i, tbl_offset;
  double square, fvar;
  unsigned short *ptc1, **ptr1, *ptc2, **ptr2,*tbl;
  unsigned short **hzlf, **vertf, **hzlc, **vertc;
  float fmean,mean_offset;
  
  if(flag==1)
    {
      hzlf = hzl_grad;
      vertf = vert_grad;
      hzlc = hzl_cost;
      vertc = vert_cost;
    }
  else
    {
      hzlf = hzl_tempf;
      vertf = vert_tempf;
      hzlc = hzl_tempc;
      vertc = vert_tempc;
    }
  
  /*** set up constants to speed up computation ***/
  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  fvar = -2.0*(temp_list[featr].rstddev*Range)*(temp_list[featr].rstddev*Range);
  
  /** Table Lookup added by supun */
  if( featr==0 || featr==1 ) {
    tbl_offset=(int)Imin;
    mean_offset=Imin-fmean;
  }
  else {
    tbl_offset= 0;
    mean_offset= -fmean;
  }
  
  tbl = (unsigned short *)malloc(sizeof(short)*(Range+1));
  if (tbl==NULL) {
    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    return 1;
  }
  for (i=0; i<=Range; i++) {
    square= (i+mean_offset)*(i+mean_offset);
    tbl[i]=(unsigned short) rint(Range*exp((double)square/fvar));
  }

  /*** Use Gaussian transform to compute Costs **/
  for(ptr1=hzlf, ptr2=hzlc, row=0; row<(orig.height+1); ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<orig.width; ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }
  
  for(ptr1=vertf, ptr2=vertc, row=0; row<orig.height; ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<(orig.width+1); ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }

  free(tbl);

  return 0;
}


/* Modified: 5/23/08 HWHM set to specified stddev by Dewey Odhner.
 *    Modified: 05/27/08 table expanded, freed by Dewey Odhner.
 */
int Segment2dCanvas::HyperTransform(int featr, int flag)
{
  int row, col, i, tbl_offset;
  double Abx, K;
  unsigned short *ptc1, **ptr1, *ptc2, **ptr2,*tbl;
  unsigned short **hzlf, **vertf, **hzlc, **vertc;
  double fmean,mean_offset;
  
  if(flag==1)
    {
      hzlf = hzl_grad;
      vertf = vert_grad;
      hzlc = hzl_cost;
      vertc = vert_cost;
    }
  else
    {
      hzlf = hzl_tempf;
      vertf = vert_tempf;
      hzlc = hzl_tempc;
      vertc = vert_tempc;
    }
  
  /*** set up constants to speed up computation ***/
  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  K = 1.0/(Range*temp_list[featr].rstddev+.01);
  
  /** Table Lookup added by supun */
  if( featr==0 || featr==1 ) {
    tbl_offset=(int)Imin;
    mean_offset=Imin-fmean;
  }
  else {
    tbl_offset= 0;
    mean_offset= -fmean;
  }
  
  tbl = (unsigned short *)malloc(sizeof(short)*(Range+1));
  if (tbl==NULL) {
    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    return 1;
  }
  for (i=0; i<=Range; i++) {
    Abx = fabs(i+mean_offset);
    tbl[i]=(unsigned short) rint(Range/(K*Abx+1));
  }

  /*** Use Gaussian transform to compute Costs **/
  for(ptr1=hzlf, ptr2=hzlc, row=0; row<(orig.height+1); ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<orig.width; ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }
  
  for(ptr1=vertf, ptr2=vertc, row=0; row<orig.height; ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<(orig.width+1); ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }

  free(tbl);

  return 0;
}
 
 
 

/* Modified: 5/23/08 HWHM set to specified stddev by Dewey Odhner.
 *    Modified: 05/27/08 table expanded, freed by Dewey Odhner.
 */
int Segment2dCanvas::InvHyperTransform(int featr, int flag)
{
  int row, col, i, tbl_offset;
  double Abx, K;
  unsigned short *ptc1, **ptr1, *ptc2, **ptr2,*tbl;
  unsigned short **hzlf, **vertf, **hzlc, **vertc;
  double fmean,mean_offset;
  
  if(flag==1)
    {
      hzlf = hzl_grad;
      vertf = vert_grad;
      hzlc = hzl_cost;
      vertc = vert_cost;
    }
  else
    {
      hzlf = hzl_tempf;
      vertf = vert_tempf;
      hzlc = hzl_tempc;
      vertc = vert_tempc;
    }
  
  /*** set up constants to speed up computation ***/
  if( featr==0 || featr==1 )
    fmean = Imin + temp_list[featr].rmean*Range;
  else
    fmean = temp_list[featr].rmean*Range;
  K = 1.0/(Range*temp_list[featr].rstddev+.01);
  
  /** Table Lookup added by supun */
  if( featr==0 || featr==1 ) {
    tbl_offset=(int)Imin;
    mean_offset=Imin-fmean;
  }
  else {
    tbl_offset= 0;
    mean_offset= -fmean;
  }
  
  tbl = (unsigned short *)malloc(sizeof(short)*(Range+1));
  if (tbl==NULL) {
    wxMessageBox("INSUFFICIENT MEMORY. CANNOT USE LIVE MODE");
    return 1;
  }
  for (i=0; i<=Range; i++) {
    Abx = fabs(i+mean_offset);
    tbl[i]=(unsigned short) rint(Range*( 1.0 - 1/(K*Abx+1)));
  }

  /*** Use Gaussian transform to compute Costs **/
  for(ptr1=hzlf, ptr2=hzlc, row=0; row<(orig.height+1); ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<orig.width; ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }
  
  for(ptr1=vertf, ptr2=vertc, row=0; row<orig.height; ptr1++, ptr2++, row++)
    for(ptc1=*ptr1, ptc2=*ptr2, col=0; col<(orig.width+1); ptc1++, ptc2++, col++)
      {
	*ptc2 = tbl[*ptc1-tbl_offset];
      }

  free(tbl);

  return 0;
}



/*****************************************************************************
 * FUNCTION: Calc_SumCost
 * DESCRIPTION: Sets values for Global variables Outside_Mask & SumCost
 *              Formerly, calculated the total of all horizontal and vertical
 *              edge costs.
 *              NOTE - This function's existence is actually quite unecessary
 *                     now. For the next version of the software, it can
 *                     be removed & Globals Outside_Mask & SumCost set inside
 *                     function Find_Slice_MaxMin()
 * PARAMETERS:  None
 * SIDE EFFECTS: Updates the value of <global>SumCost and <global>Outside_Mask
 * ENTRY CONDITIONS:
 * RETURN VALUE: 
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 07/20/93 Shoba Sharma
 *    Modified:
 *
 *****************************************************************************/
void Segment2dCanvas::Calc_SumCost()
{

/*
  ....... finding sum of all Edge Costs is unecessary ...
*/


	/*SumCost = OUTSIDE_MASK - max of all edge costs - 1 (to be safe) */
	SumCost = (double)OUTSIDE_MASK - (Imax-Imin) - 1.0;

}

void Segment2dCanvas::LoadFeatureList()
{
  static char feature_def_file[80]=".FEATURES.DEF";
  LoadFeatureList(feature_def_file);
}

/* Modified: 4/11/97 file closed by Dewey Odhner. */
void Segment2dCanvas::LoadFeatureList(const char *feature_def_file)
{
  int i, j;
  FILE *fp;
  struct FeatureList flist[8][MAX_NUM_FEATURES];

  fp=fopen(feature_def_file,"rb");
  if (fp==NULL) return;
  if (fscanf(fp,"%d",&i)!=1 || i!=MAX_NUM_FEATURES)
  {
	fclose(fp);
    return;
  }
  for (j=0; j<8; j++)
  {
   for(i=0;i<MAX_NUM_FEATURES;i++) {
    if (fscanf(fp,"%d %d %f %f %f %f %f",
               &flist[j][i].status,&flist[j][i].transform,
               &flist[j][i].weight,&flist[j][i].rmin,
               &flist[j][i].rmax,&flist[j][i].rmean,&flist[j][i].rstddev)!=7)
    {
	  fclose(fp);
	  return;
    }
   }
   #if (MAX_NUM_FEATURES>=7)
     flist[j][6].status=OFF;
   #endif
  }
  memcpy(accepted_list, flist, 8*MAX_NUM_FEATURES*sizeof(struct FeatureList));
  memcpy(temp_list, flist[object_number%8],
    MAX_NUM_FEATURES*sizeof(struct FeatureList));
  fclose(fp);
}
 
 
 
/* Modified: 4/11/97 file closed by Dewey Odhner. */
void Segment2dCanvas::WriteFeatureList()
{
  int i, j;
  FILE *fp;
  static char feature_def_file[80]=".FEATURES.DEF";

  fp=fopen(feature_def_file,"wb");
  if (fp==NULL) return;
  fprintf(fp,"%d\n",MAX_NUM_FEATURES);
 
  for (j=0; j<8; j++)
   for(i=0;i<MAX_NUM_FEATURES;i++)
    fprintf(fp,"%d %d %f %f %f %f %f\n",
            accepted_list[j][i].status,accepted_list[j][i].transform,
            accepted_list[j][i].weight,accepted_list[j][i].rmin,
            accepted_list[j][i].rmax,accepted_list[j][i].rmean,
            accepted_list[j][i].rstddev);
 
  fclose(fp);
}

/*****************************************************************************
 * FUNCTION: SnakeDeformation
 * DESCRIPTION: Classic snakes implementation 
 *              
 * PARAMETERS: Gradient image, Number of ControlPoints, and Control Points (x, y)
 *    alpha, beta, gamma: weights for continuity, curvature, and gradient
 *       energy, respectively
 *    nrows, ncolumns: number of rows & columns in gradient image
 *    niterations: number of iterations to be done
 * SIDE EFFECTS: pRow, pCol set.
 * FUNCTIONS CALEED: None
 * ENTRY CONDITIONS:  
 *           
 * RETURN VALUE: New control Points (x, y)
 * EXIT CONDITIONS: 
 * HISTORY:
 *  Created: 06/02/05 by Bipul Das and Andre Souza
 *  Modified: 6/27/05 alpha, beta, gamma, nrows, ncolumns, niterations passed
 *     as parameters by Dewey Odhner.
 *
 *****************************************************************************/


void Segment2dCanvas::SnakeDeformation(double **Gradient, int NumberOfControlPoints, CPoint *ControlPoint, double alpha, double beta, double gamma, int nrows, int ncolumns, int niterations)
{
  int iteration,N,SearchPixelsPerRow,SearchArea;
  int pTerminationFlag = FALSE;
  int index,neighbor;
  double *SnakeEnergy,Emin,Dmax,GradientMagnitude;
  CPoint present,previous,next,cTemp={0,0}, new_point;


  pRow = nrows;
  pCol = ncolumns;
  SearchPixelsPerRow=3; 
  SearchArea=(int)pow(SearchPixelsPerRow,2.);

  SnakeEnergy=(double *)malloc(SearchArea*sizeof(double));

  iteration = 0;

  while(!pTerminationFlag)
    {
      N=NumberOfControlPoints; 

      for(index=0;index<N;index++)
	{
	  /*****************************************************************
	   * Compute the energy for a 3x3 neighborhood
	   * Return the region with minimum energy in the neighborhood as the
	   * desired new position for the control point
	   *
	   *              --------------------------
	   *              |  0    |   1    |   2   |
	   *              |       |        |       |
	   *              --------------------------
	   *              |  3    |   4    |   5   |
	   *              |       |        |       |
	   *              --------------------------
	   *              |  6    |   7    |   8   |
	   *              |       |        |       |
	   *              --------------------------
  
	   *****************************************************************/

	  /* 
	   * Dmax : Max Euclidean distance from point 4 (above) to the farthest
	   * neighboring point -> Diagonal points in this case 
	   */
/*@
	  Dmax=GetDmaxForNeighborHood(index%N,SearchArea,SearchPixelsPerRow);
*/	  Dmax = sqrt(2.);

	  present = ControlPoint[index];
	  previous = ControlPoint[(index-1+N)%N];
	  next = ControlPoint[(index+1)%N];

	  Emin=6553500.0;
	  for(neighbor=0; neighbor<SearchArea; neighbor++)
	    {
	      new_point = get_new_point(present,neighbor,SearchPixelsPerRow);

	      GradientMagnitude = Gradient[(int)(new_point.y+0.5)][(int)(new_point.x+0.5)];

	      SnakeEnergy[neighbor] = ComputeEnergy(new_point, previous, next, GradientMagnitude, Dmax, alpha, beta, gamma);
		      
	      if((SnakeEnergy[neighbor]<Emin) ||
		 (SnakeEnergy[neighbor]==Emin && neighbor==4))
		{
		  Emin = SnakeEnergy[neighbor];
		  cTemp = new_point;
		}
	    }
	  /* Update the control point location */    
	  ControlPoint[index%N] = cTemp;
	}

      /*ReSampleControlPoints(NumberOfControlPoints);  not implemented */
      iteration++;
      if(iteration>=niterations) pTerminationFlag=TRUE;
    }

}

/***************************************************************
 * Compute snake energy for a control point given by "present"
 **************************************************************/
double Segment2dCanvas::ComputeEnergy(CPoint present,CPoint previous,CPoint next, double GradientMagnitude, double Dmax, double alpha, double beta, double gamma)
{
  double Energy,ContinuityEnergy,CurvatureEnergy,GradientEnergy;

  ContinuityEnergy = ComputeContinuityEnergy(present,previous);
  CurvatureEnergy = ComputeCurvatureEnergy(present, previous, next);

  GradientEnergy = GradientMagnitude*GradientMagnitude;

  Energy = alpha * ContinuityEnergy/pow(Dmax,2.) +
           beta * CurvatureEnergy/pow(Dmax,4.) +
           gamma * GradientEnergy;

  return Energy;
}

/************************************************************
 * Program: ComputeCurvatureEnergy()
 * Function: Computes Curvature energy over the entire deformed arc
 *           and normalizes it with the Gaussian window weight factor
 *
 *           Energy=|(s-(s-d))/d|^2
 *           d is the interval, which is 1 in this case 
 ***********************************************************/
double Segment2dCanvas::ComputeContinuityEnergy(CPoint present, CPoint previous)
{
  return pow(present.x-previous.x,2.)+pow(present.y-previous.y,2.);
}

/************************************************************
 * Program: ComputeCurvatureEnergy()
 * Function: Computes Curvature energy over the entire deformed arc
 *           and normalizes it with the Gaussian window weight factor
 *
 *      Energy = |(s-d)-2(s)+(s+d)/d^2|^2
 *      d is the interval, which is 1 in this case
 ***********************************************************/
double Segment2dCanvas::ComputeCurvatureEnergy(CPoint present, CPoint previous, CPoint next)
{
  double xfactor,yfactor;
  
  xfactor=previous.x-2*present.x+next.x;
  yfactor=previous.y-2*present.y+next.y;

  return (pow(xfactor,2.)+pow(yfactor,2.));
}

/***************************************************************
 * GetNewPoint : Find the coordinate position corresponding to 
 *               a defined neighborhood
 ***************************************************************/
CPoint Segment2dCanvas::get_new_point(CPoint present, int j, int limit)
{
  CPoint 	new_point;

  new_point.x = present.x+((j%limit)-1) ;
  new_point.y = present.y+(int)(j/limit)-1 ;

  if(new_point.x<0.0)
    new_point.x=0.0 ;
  else if(new_point.x>=pCol)
    new_point.x = pCol-1 ;
  
  if(new_point.y<0.0)
    new_point.y=0.0 ;
  else if(new_point.y>=pRow)
    new_point.y = pRow - 1;
  
  return new_point ;
}


/*--------------------------------------------------------------------*/
/* Copies the "o_contour" contour points into a temporary array of contours "temp_contours" (global) */
void Segment2dCanvas::copy_ocontour_into_temp_array()
{
	int i;

	assert(temp_contours.last <= 0);
	if (o_contour.last < 0)
		return;
	if (temp_contours.last >= 0)
		free(temp_contours.vertex);

	/* Allocate memory for vertices */
	temp_contours.vertex = (X_Point *) malloc((o_contour.last+1) * sizeof(X_Point));

	if(temp_contours.vertex == NULL)
	{
		wxMessageBox("ERROR: CAN'T ALLOCATE MEMORY !!");
	}

	/* Copy Vertices */
	for(i=0; i<=o_contour.last; i++)
	{
		temp_contours.vertex[i].x = o_contour.vertex[i].x;
		temp_contours.vertex[i].y = o_contour.vertex[i].y;
	}
	temp_contours.last = o_contour.last;
	temp_contours.slice_index = o_contour.slice_index;
	temp_contours.img = o_contour.img;
	
}

/*---------------------------------------------------------------------------*/
/* Saves the temporary MASK file (relative to a given scene) */
/* Modified: 12/10/99 pixel count changed to 4 bytes by Dewey Odhner. */
/* Modified: 10/8/01 missing slices set to default value by Dewey Odhner. */
void Segment2dCanvas::save_mask_proc(int fill_remainder)
{
	FILE *fp;
	char code[4];
	int n,			/* index of the current mask on the scene */
		size,		/* size of each mask */
		n_slices,	/* number of mask slices currently on the file */
		missing_slices;	/* #of slices missing from the file */
	unsigned char *tmask;	/* temporary mask buffer */
	unsigned char bit_mask;	/* onbits corresponding to the masks */
	int i, new_format, j;
	unsigned short short_area[8];
	unsigned u[8];
	long file_size;	/* size of the file */
	

	SetStatusText("Saving Object. Wait ...", 0);

	wxFileName in_fname(mCavassData->m_fname);
	wxFileName mask_fname(in_fname.GetPath(), "object_mask.TMP");

	/* Open file for updating */
	if( (fp = fopen((const char *)mask_fname.GetFullPath().c_str(), "rb+")) ==
			NULL)
	{
		wxMessageBox("CAN'T OPEN TEMPORARY 'OBJECT' FILE !");
		return;
	}

	fread(code, 4, 1, fp);
	new_format = strncmp(code, "///", 4)==0;
	size = orig.width * orig.height;
	n = object_mask_slice_index;
	tmask = NULL;

	/* If file is not big enough, append blank masks */
	if( (VLSeek(fp, 100+(double)n*(size+1+(new_format? 32:16)))) ||
			(default_mask_value && n<sl.slice_index[sl.sd==3?
			0:sl.max_slices-1][sl.volumes-1]))
	{
		fseek(fp, 0, 2);
		file_size = ftell(fp);
		n_slices = (int)((file_size-100) / (size+1+(new_format? 32:16)));

		missing_slices = n - n_slices;
		tmask = (unsigned char *) malloc(size);
		if( tmask == NULL)
		{
			wxMessageBox("CAN'T ALLOCATE MEMORY DURING 'SAVE' OPERATION !");
			fclose(fp);
			return;
		}
		if (default_mask_value)
		{
			memset(tmask, 255, size);
			for(i=0; i<8; i++)
				short_area[i] = u[i] = size;
		}
		else
		{
			memset(tmask, 0, size);
			for(i=0; i<8; i++)
				short_area[i] = u[i] = 0;
		}

		/* Fill in the missing mask slices */
		VLSeek(fp, (100+
			(double)(n>n_slices?n_slices:n)*(size+1+(new_format?32:16))));
		for (i=0; i<missing_slices; i++)
		{
			if (fwrite(tmask, 1, 1, fp) != 1 ||
					(new_format? VWriteData((char *)u, 4, 8, fp, &j) != 0:
					fwrite(short_area, 16, 1, fp) != 1) ||
					fwrite(tmask, size, 1, fp) != 1)
			{
				wxMessageBox("CAN'T WRITE 'OBJECT' FILE !");
				free(tmask);
				fclose(fp);
				return;
			}
		}
	}

	/* At this point the file is ready to receive the current MASK buffer */


	/* Saves current object_mask buffer into appropriate place on MASK file */
	check_objects(&bit_mask);
	check_area(area);
	if (default_mask_value)
	{
		for (i=0; i<size; i++)
			object_mask[i] |= ~bit_mask;
		for (i=0; i<8; i++)
			if (onbit[i] & ~bit_mask)
				area[i] = size;
	}
	if( fwrite(&bit_mask, 1, 1, fp) != 1)
	{
		wxMessageBox("CAN'T WRITE 'OBJECT' FILE !");
		fclose(fp);
		if (tmask)
			free(tmask);
		return;
	}
	if (new_format)
	{
		if( VWriteData((char *)area, 4, 8, fp, &i) != 0)
		{
			wxMessageBox("CAN'T WRITE 'OBJECT' FILE !");
			fclose(fp);
			if (tmask)
				free(tmask);
			return;
		}
	}
	else
	{
		for (i=0; i<8; i++)
			short_area[i] = area[i];
		if( fwrite(short_area, 16, 1, fp) != 1)
		{
			wxMessageBox("CAN'T WRITE 'OBJECT' FILE !");
			fclose(fp);
			if (tmask)
				free(tmask);
			return;
		}
	}
	if( fwrite(object_mask, size, 1, fp) != 1)
	{
		wxMessageBox("CAN'T WRITE 'OBJECT' FILE !");
		fclose(fp);
		if (tmask)
			free(tmask);
		return;
	}
	if (fill_remainder)
	  if (default_mask_value && n<sl.slice_index[sl.sd==3?
			0: sl.max_slices-1][sl.volumes-1])
		for(i=n+1; i<=sl.slice_index[sl.sd==3? 0:
			sl.max_slices-1][sl.volumes-1]; i++)
		{
			if (fwrite(tmask, 1, 1, fp) != 1 ||
					(new_format? VWriteData((char *)u, 4, 8, fp, &j) != 0:
					fwrite(short_area, 16, 1, fp) != 1) ||
					fwrite(tmask, size, 1, fp) != 1)
			{
				wxMessageBox("CAN'T WRITE 'OBJECT' FILE !");
				free(tmask);
				fclose(fp);
				return;
			}
		}

	fflush(fp);
	fclose(fp);
	if (tmask)
		free(tmask);

	SetStatusText("Done saving object.", 0);
}

/*---------------------------------------------------------------------------*/
/* Modified: 12/10/99 pixel count changed to 4 bytes by Dewey Odhner. */
int Segment2dCanvas::load_object_mask(int n)
/* n: index of the mask to be loaded (1dimensional index, 0=first) */
{
	FILE *fp;
	char filename[200];
	int	size;		/* size of each mask */
	unsigned char temp;
	char text[100], tt[20];
	int i, new_format;
	unsigned short short_area[8];


	/* Open file for reading the appropriate mask */
	strcpy(filename, "object_mask.TMP");
	if( (fp = fopen(filename, "rb")) == NULL)
	{
		wxMessageBox("CAN'T OPEN MASK FILE !");
		return(-1);
	}

	fread(tt, 4, 1, fp);
	new_format = strncmp(tt, "///", 4)==0;
	temp = 0;
	size = orig.width * orig.height;


	/* If can't seek to the appropriate location, then mask slice is not there */
	if (VLSeek(fp, 100+(double)n*(size+1+(new_format? 32:16))))
	{
		/* therefore create a CLEAN one */
		set_object_mask_bit(-1, default_mask_value);
	}
	else
	/* Try to load the mask slice */
	{
		/* Load the appropriate TAG byte, AREA and MASK buffer */
		if (fread(&temp, 1, 1, fp) == 1  &&  (new_format?
		    VReadData((char *)area, 4, 8, fp, &i)==0: fread(short_area, 16, 1, fp)== 1)
			&&  fread(object_mask, size, 1, fp) == 1)
		{
			if (!new_format)
				for (i=0; i<8; i++)
					area[i] = short_area[i];
			strcpy(text, "Loaded masks for object#: ");

			if(temp != 0)
			{
				for(i=0; i<8; i++)
				{
					if( (temp & onbit[i]) > 0 )
					{
						sprintf(tt, "%d  ", i+1);
						strcat(text, tt);
					}
				}
			}
			else
				strcat(text, " none");
			SetStatusText(text, 0);
		}
		else
		/* Couldn't load mask slice, therfore create CLEAN one */
		{
			set_object_mask_bit(-1, default_mask_value);
		}
	}

	fclose(fp);

	/* If Clear MASK is loaded */
	if((temp & onbit[object_number]) == 0)
		return(0);
	else
	/* If a Non-Clear MASK is loaded */
		return(1);
}

/*---------------------------------------------------------------------------*/
/* This function checks what objects are represented in an object_mask buffer */
/* and returns the result in the form of a byte, where each object is represented */
/* as a bit, 0=not present, 1=present) */
void Segment2dCanvas::check_objects(unsigned char *result)
{
	int i;
	int size;

	*result = 0;
	size = mCavassData->m_xSize*mCavassData->m_ySize;
	for(i=0; i<size; i++)
		*result |= object_mask[i];

}

/*---------------------------------------------------------------------------*/
/* Calculates the area of each object in terms of #of pixels */
/* Modified: 12/10/99 pixel count changed to 4 bytes by Dewey Odhner. */
void Segment2dCanvas::check_area(unsigned area[8] /* area of each object */)
{
	int i;
	unsigned char value;
	int size;


	/* reset Area buffer (area of each object in the mask in pixels) */
	for(i=0; i<8; i++) area[i] = 0;

	size = mCavassData->m_xSize*mCavassData->m_ySize;
	for(i=0; i< size; i++)
	{
		value = object_mask[i];

		/* Get Area for each object */
		area[0] += (value & onbit[0]) ? 1 : 0;
		area[1] += (value & onbit[1]) ? 1 : 0;
		area[2] += (value & onbit[2]) ? 1 : 0;
		area[3] += (value & onbit[3]) ? 1 : 0;
		area[4] += (value & onbit[4]) ? 1 : 0;
		area[5] += (value & onbit[5]) ? 1 : 0;
		area[6] += (value & onbit[6]) ? 1 : 0;
		area[7] += (value & onbit[7]) ? 1 : 0;
	}

}

/*---------------------------------------------------------------------------*/
void Segment2dCanvas::set_object_mask_bit(int bit /* 0,1,2,3,4,5,6,7, -1 or 8 = all */, int value /* 0=OFF, 1=ON */)
{
	int i;

	if(bit>=0 && bit<8)
	{
		for(i=mCavassData->m_xSize*mCavassData->m_ySize-1; i>=0; i--)
			object_mask[i] = (value>0) ? object_mask[i] | onbit[bit] : (object_mask[i] & offbit[bit]);
	}
	else
	{
		if(value == 1) value = 255;
		for(i=mCavassData->m_xSize*mCavassData->m_ySize-1; i>=0; i--)
			object_mask[i] = value;
	}
}
void Segment2dCanvas::check_overlay_color(int overlay_intensity)
{
//@
}

/*---------------------------------------------------------------------------*/
/* This function checks if the MASK file is relative to the scene in question */
/* and in the negative case it updates the file to reflect the New scene */
/* Returned value:  0 -> file is the same */
/*					1 -> file didn't exist and it was created */
/*					2 -> file exists and is not the same */
/*				   -1 -> if error */
int Segment2dCanvas::check_mask_file(int *slice, int *volume)
{
	FILE *fp, *fpgm;
	char mother_file[101];
	int i, new_format;

	for(i=0; i<101; i++) mother_file[i] = 0;

	/***************************/
	/* Parameters for DP (DP.c) */
	int initial_object=object_number;
	object_number = 8;
	Reset_training_proc(2);
	object_number = initial_object;
	LoadFeatureList();
	/***************************/

	/* Open file for updating */

	wxFileName m_fname = wxFileName(mCavassData->m_fname);

	/* If file exist, check if is the same as current */
	if( (fp = fopen("object_mask.TMP", "rb")) != NULL)
	{
		/* Read in the mother file name */
		if( fread(mother_file, 100, 1, fp) == 1)
		{
			fclose(fp);
			new_format = strcmp(mother_file, "///")==0;

			/* Copy the mother file into the global name 'mother_filename' */
			strncpy(mother_filename, mother_file+4*new_format, 100);
			mother_filename[100] = 0;
			wxFileName motherFileName(mother_filename);
			if (wxFileName::FileExists(motherFileName.GetFullPath()))
				strncpy(mother_filename,
					(const char *)motherFileName.GetFullName().c_str(), 100);

			/* Mother_file is the same as current scene */
			if(strcmp(mother_filename,
					(const char *)m_fname.GetFullName().c_str()) == 0)
			{

				/* Load Grey Window parameters from file (min and max values) */
				/* If File DOESN'T EXIST */
				if( (fpgm = fopen("greymap.TMP", "rb")) == NULL)
				{
					SetStatusText("Using Default Grey Map.", 0);
					*slice = 0;
					if (volume)
						*volume = 0;
				}
				else
				/* If File EXIST */
				{
					int j;
					float min, max;
					if (volume == NULL)
						volume = &j;
					fscanf(fpgm, "%f %f", &min, &max);
					fscanf(fpgm, "%d %d", volume, slice);
					if (*volume>=sl.volumes || *slice>=sl.slices[*volume])
						*volume = *slice = 0;
					fscanf(fpgm, "%d", &overlay_intensity);
					if(overlay_intensity < 0  || overlay_intensity > 8) overlay_intensity = 1;
					fclose(fpgm);

					mCavassData->m_center = (int)(min + (max-min)/2);
					mCavassData->m_width = (int)(max-min);
					if (mCavassData->m_vh.scn.largest_density_value_valid &&
							mCavassData->m_vh.scn.smallest_density_value_valid)
					{
					  if (mCavassData->m_center >
							mCavassData->m_vh.scn.largest_density_value[0])
						mCavassData->m_center = (int)(
							mCavassData->m_vh.scn.largest_density_value[0]);
					  if (mCavassData->m_center <
							mCavassData->m_vh.scn.smallest_density_value[0])
						mCavassData->m_center = (int)(
							mCavassData->m_vh.scn.smallest_density_value[0]);
					  if (mCavassData->m_width >
							mCavassData->m_vh.scn.largest_density_value[0]-
							mCavassData->m_vh.scn.smallest_density_value[0]+1)
						mCavassData->m_width = (int)(
							mCavassData->m_vh.scn.largest_density_value[0]-
							mCavassData->m_vh.scn.smallest_density_value[0]+1);
					}
//@					VChangePanelItem("OVL.COLOR", "OVL.COLOR", overlay_intensity);
					check_overlay_color(overlay_intensity);
			      }
				return(0);
		    }
			else
				return(2);
		}
		fclose(fp);
	}
	else
	{

		/* Load Grey Window parameters from file (min and max values) */
		/* If File EXISTS */
printf("Open greymap.TMP L8597\n");
		if ( (fpgm = fopen("greymap.TMP", "rb")) )
		{
			int j;
			float min, max;
			if (volume == NULL)
				volume = &j;
			fscanf(fpgm, "%f %f", &min, &max);
			fscanf(fpgm, "%d %d", volume, slice);
			if (*volume>=sl.volumes || *slice>=sl.slices[*volume])
				*volume = *slice = 0;
			fscanf(fpgm, "%d", &overlay_intensity);
			if(overlay_intensity < 0  || overlay_intensity > 8) overlay_intensity = 1;
			fclose(fpgm);

			mCavassData->m_center = (int)(min + (max-min)/2);
			mCavassData->m_width = (int)(max-min);
			if (mCavassData->m_vh.scn.largest_density_value_valid &&
					mCavassData->m_vh.scn.smallest_density_value_valid)
			{
			  if (mCavassData->m_center >
					mCavassData->m_vh.scn.largest_density_value[0])
				mCavassData->m_center = (int)(
					mCavassData->m_vh.scn.largest_density_value[0]);
			  if (mCavassData->m_center <
					mCavassData->m_vh.scn.smallest_density_value[0])
				mCavassData->m_center = (int)(
					mCavassData->m_vh.scn.smallest_density_value[0]);
			  if (mCavassData->m_width >
					mCavassData->m_vh.scn.largest_density_value[0]-
					mCavassData->m_vh.scn.smallest_density_value[0]+1)
				mCavassData->m_width = (int)(
					mCavassData->m_vh.scn.largest_density_value[0]-
					mCavassData->m_vh.scn.smallest_density_value[0]+1);
			}
			check_overlay_color(overlay_intensity);
		}
	}

	/* At this point the file either doesn't exist  */
	/* therefore a new file needs to be created */

	/* File does not exist, so Create one */
	/* create the file */
	if( (fp = fopen("object_mask.TMP","wb")) == NULL)
	{
		wxMessageBox("CAN'T  OPEN TEMPORARY 'MASK' FILE !");
		return(-1);
	}

	strcpy(mother_file, "///");
	strncpy(mother_file+4, (const char *)m_fname.GetFullName().c_str(), 97);

	/* Write mother_file name onto the file */
	fwrite(mother_file, 100, 1, fp);
	fflush(fp);
	fclose(fp);

	return(1);
}

void Segment2dCanvas::load_object_proc()
{
	int i;
	if( mask_index_different() )
		save_mask_proc(0);
	object_mask_slice_index = sl.slice_index[VolNo][mCavassData->m_sliceNo];
	i = load_object_mask(object_mask_slice_index);
	if (i==0 && mCavassData->m_sliceNo>0 && detection_mode==PAINT &&
			paint_brush_size==0)
		i = load_object_mask(object_mask_slice_index-1);
	if (i==0 && mCavassData->m_sliceNo<mCavassData->m_zSize-1 &&
	        detection_mode==PAINT && paint_brush_size==0)
		i = load_object_mask(object_mask_slice_index+1);

	/* clear the entire object vertex array */
	set_object_vertex_mask_bit(-1, 0);

	if (i == 0)
	{
		load_mask_from_BIM();
		i = load_object_mask(object_mask_slice_index);
		if (i==0 && mCavassData->m_sliceNo>0 && detection_mode==PAINT &&
				paint_brush_size==0)
			i = load_object_mask(object_mask_slice_index-1);
	}
	reload();
}

void Segment2dCanvas::load_mask_from_BIM()
{
	wxString filename = wxFileSelector( _T("Select mask file"), _T(""),
		_T(""), _T(""),
		"CAVASS files (*.BIM)|*.BIM",
		wxFILE_MUST_EXIST );
	if (!filename || filename.Length()==0 || (wxFile::Exists("object_mask.TMP")
			&&wxMessageBox("Overwrite current mask?","Confirm",wxOK|wxCANCEL)
			!=wxOK))
		return;
	wxString b = wxString("bin_to_MSK \"")+
		wxFileName(mCavassData->m_fname).GetFullName()+"\" \""+filename+
		wxString::Format("\" object_mask.TMP %d", object_number);
	system((const char *)b.c_str());
	load_object_mask(object_mask_slice_index);
}

/*---------------------------------------------------------------------------*/
void Segment2dCanvas::set_image_location(IMAGE *img, int px, int py)
{
	img->locx = px;
	img->locy = py;
}
/*---------------------------------------------------------------------------*/
void Segment2dCanvas::set_image_output_size(IMAGE *img, int w, int h)
{
	img->w = img->framew = w;
	img->h = img->frameh = h;

	img->scale = (float)h/img->height;

	img->wp = (int) rint(img->w / img->scale);
	img->hp = (int) rint(img->h / img->scale);

	img->output_size_change_flag = 1;
}

/*---------------------------------------------------------------------------*/
/* This function should be called when any 9of the following functions is called:
	- set_image_scale()
	- set_image_offset()
	- set_image_output_size()


	SCREEN => IMAGE

 Modified: 5/23/02 half pixel offset removed by Dewey Odhner.
 Modified: 4/3/08 output image size not changed by Dewey Odhner.
*/
void Segment2dCanvas::set_image_table(IMAGE *img)
{
	float npixel;	/* new pixel size (in respect to the original) */
	int i;
	bool flag;

	/* npixel = how many screen pixels correspond to a pixel on the original image */
	npixel = 1.0 / img->scale;

	if(img->tblx != NULL)
		free(img->tblx);
	if(img->tbly != NULL)
		free(img->tbly);

	img->tblx = (unsigned short *) malloc( img->w * sizeof(short));
	img->tbly = (unsigned short *) malloc( img->h * sizeof(short));
	if( img->tblx == NULL || img->tbly == NULL)
	{
		printf("ERROR: Can't allocate memory for interpolation tables !! ");
		exit(3);
	}


	/* Horizontal Table */
	for(i=0; i<img->w; i++)
		img->tblx[i] = (int) ( i*npixel + img->offx);

	/* account for roundoff error (i*npixel) */
	/* Scan created table from the end up and stop as soon as roundoff error is not present anymore */
	for(i=img->w-1, flag=TRUE; flag==TRUE && i>0; i--)
	{
		if(img->tblx[i] >= img->width)
			img->tblx[i] = img->width-1;
		else
			flag = FALSE;
	}

	/* Vertical Table */
	for(i=0; i<img->h; i++)
		img->tbly[i] = (int) ( i*npixel + img->offy); 

	/* account for roundoff error (i*npixel) */
	/* Scan created table from the end up and stop as soon as roundoff error is not present anymore */
	for(i=img->h-1, flag=TRUE; flag==TRUE && i>0; i--)
	{
		if(img->tbly[i] >= img->height)
			img->tbly[i] = img->height-1;
		else
			flag = FALSE;
	}

	set_image_table2(img);
}
/*---------------------------------------------------------------------------*/
/*
	IMAGE => SCREEN 
 Modified: 10/17/00 tables enlarged by Dewey Odhner.
 Modified: 5/23/02 half pixel offset removed by Dewey Odhner.
 Modified: 2/17/04 rounding of img->tbl2x, img->tbl2y changed by Dewey Odhner.
 Modified: 4/7/08 output image size not changed by Dewey Odhner.
*/
void Segment2dCanvas::set_image_table2(IMAGE *img)
{
	int nw, nh;
	int i;

	/* nw,nh is the size in pixels of the output image after scaling */
	nw = img->w;
	nh = img->h;
	

	img->wp = (int) rint(img->w / img->scale);
	img->hp = (int) rint(img->h / img->scale);

	if(img->tbl2x == NULL)
		img->tbl2x = ( short *) malloc( (img->width+1) * sizeof(short));
	if(img->tbl2y == NULL)
		img->tbl2y = ( short *) malloc( (img->height+1) * sizeof(short));

	if(img->tbl2x == NULL  ||  img->tbl2y == NULL)
	{
		printf("ERROR: Can't allocate memory for interpolation tables !! ");
		exit(3);
	}
	
	/* Horizontal Table */
	for(i=0; i<=img->width; i++)
	{
		if(i < img->offx  ||  i > img->offx+img->wp)
			img->tbl2x[i] = 9999;
		else
			img->tbl2x[i] = (short)ceil(img->locx + (i-img->offx)*img->scale);
	}
		

	/* Vertical Table */
	for(i=0; i<=img->height; i++)
	{
		if(i < img->offy  ||  i > img->offy+img->hp)
			img->tbl2y[i] = 9999;
		else
			img->tbl2y[i] = (short)ceil(img->locy + (i-img->offy)*img->scale);
	}

}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Free an image */
void Segment2dCanvas::free_image(IMAGE *img)
{
	if(img->original_data != NULL)
		free(img->original_data);
	if(img->data != NULL)
		free(img->data);

	if(img->tblx != NULL)
		free(img->tblx);
	if(img->tbly != NULL)
		free(img->tbly);

	if(img->tbl2x != NULL)
		free(img->tbl2x);
	if(img->tbl2y != NULL)
		free(img->tbl2y);

	if(img->tblofx != NULL)
		free(img->tblofx);
	if(img->tblofy != NULL)
		free(img->tblofy);

	img->original_data =
	img->data = NULL;
	img->tblx =
	img->tbly = NULL;
	img->tbl2x =
	img->tbl2y = NULL;
	img->tblofx =
	img->tblofy = NULL;
	img->bits =
	img->width = 
	img->height = 
	img->offx =
	img->offy =
	img->w =
	img->h =
	img->wp =
	img->hp =
	img->framew =
	img->frameh =
	img->locx =
	img->locy =
	img->type = 0;
	img->scale = 0.0;
}

void Segment2dCanvas::bin_to_grey(unsigned char *bin_buffer,
	int length_grey, unsigned char *grey_buffer, int min_value, int max_value)
{
        int j;
        unsigned char mask[8];
        unsigned char *bin, *grey;
 
        bin = bin_buffer;
        grey = grey_buffer;
 
        /* initialize masks */
        mask[0] = 1;
        for(j=1; j<8; j++)
           mask[j] = mask[j-1] * 2;
 
        for(; ; )
        {
           for(j=7; j>=0; j--)
           {
                if( (*bin & mask[j]) != 0) *grey = max_value;
                else *grey = min_value;
 
                grey++;
				if (grey-grey_buffer == length_grey)
					return;
           }
           bin++;
        }
 
 
}

int copy_file(const char src[], const char dst[])
{
	FILE *fp1 = fopen(src, "rb");
	if (fp1 == NULL)
		return -1;
	FILE *fp2 = fopen(dst, "wb");
	if (fp2 == NULL)
	{
		fclose(fp1);
		return -1;
	}
	int ec=0;
	while (!feof(fp1))
	{
		int c = fgetc(fp1);
		if (fputc(c, fp2) == EOF)
		{
			ec = -1;
			break;
		}
	}
	fclose(fp1);
	fclose(fp2);
	return ec;
}

void Segment2dCanvas::generate_masked_scene(const char *out_file_name,
	int out_object)
{
    wxString text;

	/* save last slice that was being worked on */
	save_mask_proc(default_mask_value);

	/*  Build Command  */

	/* SAVE THE OBJECT_MASK.TMP INTO ANOTHER FILE */
	if(out_object == 8)
	{
		if (copy_file("object_mask.TMP", out_file_name))
			wxMessageBox("File copy failed.");
		return;
	}

	if (strlen(out_file_name) < 4)
	{
		wxMessageBox("Not a valid scene file name");
		return;
	}
	if (strcmp(out_file_name+strlen(out_file_name)-4, ".IM0") == 0)
		output_type = GREY;
	else if (strcmp(out_file_name+strlen(out_file_name)-4, ".BIM") == 0)
		output_type = BINARY;
	else
	{
		wxMessageBox("Not a valid scene file name");
		return;
	}
	char *name_wo_ext = (char *)malloc(strlen(out_file_name)-3);
	strncpy(name_wo_ext, out_file_name, strlen(out_file_name)-4);
	name_wo_ext[strlen(out_file_name)-4] = 0;

	if(output_type == BINARY)
		text = wxString::Format("mask2bin \"%s/object_mask.TMP\" \"%s\" %d 0",
			(const char *)wxFileName::GetCwd().c_str(), name_wo_ext, out_object);
	else
		text = wxString::Format("mask2grey \"%s/object_mask.TMP\" \"%s\" %d 0",
			(const char *)wxFileName::GetCwd().c_str(), name_wo_ext, out_object);
	free(name_wo_ext);

	/* Check if INPUT and OUTPUT filenames are different */
	if(strcmp((const char *)wxFileName(wxFileName::GetCwd(), mother_filename).GetFullPath().c_str(), out_file_name) == 0)
	{
		wxMessageBox("INPUT AND OUTPUT FILES MUST HAVE DIFFERENT NAMES. CHANGE OUTPUT NAME.");
		return;
	}

	cout << text << endl;
	system((const char *)text.c_str());
}

void Segment2dCanvas::Run_Statistics()
{
    unsigned char **hzl_aux, **vert_aux;
	int j;


    hzl_aux= (unsigned char **)malloc((orig.height+1)*sizeof(unsigned char *));
    for(j=0; j < (orig.height+1); j++)
        hzl_aux[j] = (unsigned char *) calloc(orig.width, 1); 
    vert_aux = (unsigned char **)malloc(orig.height*sizeof(unsigned char *));
    for(j=0; j < orig.height; j++)
        vert_aux[j] = (unsigned char *) calloc(orig.width+1, 1);


	int nedges=0;  /* number of edges in contours & painted region */
	double sum[6], sumsq[6], nsum[6], nsumsq[6];
	float fmin[6], fmax[6], nfmin[6], nfmax[6];
    int dum;
	double mean;
	float stddev;

	for (j=0; j<6; j++)
	{
		sum[j] = sumsq[j] = nsum[j] = nsumsq[j] = 0;
		fmin[j] = fmax[j] = nfmin[j] = nfmax[j] = -1.0;
	}


	/*======================================================*/
	/*====    Run Statisitcs on all edges in region[]   ====*/
	/*======================================================*/

	int i, count=0;
    int inside;

    /* scan region[] & calculate total number of edges */

    /* scan hzlly to count VERT edges in region[] */
    for(j=1; j<(orig.height-1); j++)
    for(i=1,inside=0; i<orig.width; i++)
    {
        if (region[*(tblr+j)+i] != 0) /* Begin */
        {
            count++;
			calculate_vert_features(sum, sumsq, fmin, fmax, j, i);
			vert_aux[j][i] = 1;
            inside = 1;
		}
        else
        if (inside) /* End */
        {
            count++;
			calculate_vert_features(sum, sumsq, fmin, fmax, j, i);
			vert_aux[j][i] = 1;
            inside = 0;
        }
    } /* end for */
 
    /* scan vertically to count HZL edges in region[]  */

    for(i=1; i<(orig.width-1); i++)
    for(j=1,inside=0; j<orig.height; j++)
    {
        if (region[*(tblr+j)+i] != 0) /* Begin */
        {
            count++;
			calculate_hzl_features(sum, sumsq, fmin, fmax, j, i);
			hzl_aux[j][i] = 1;
            inside = 1;
		}
        else
        if (inside) /* End */
        {
            count++;
			calculate_hzl_features(sum, sumsq, fmin, fmax, j, i);
			hzl_aux[j][i] = 1;
            inside = 0;
        }
    } /* end for */
 
	nedges += count;


	/*=============================================================*/
	/*====    Run Statisitcs on all edges in temp_contours[]   ====*/
	/*=============================================================*/


	int row, col;
	int cont_last=temp_contours.last-1;
	OPEN_CONTOUR *cont=&temp_contours;
    for (int cont_num=0; cont_num<2;
		cont_num++,cont=&o_contour,cont_last=cont->last)
    {
 
		/*======================================================*/
		/*===   scan temp_contour, but stop 1 short of end =====*/
		/*======================================================*/
        for(i=0; i<cont_last; i++) /* stop 1 short of end */
        {
            if( cont->vertex[i].x == cont->vertex[i+1].x)
			/*----   VERTICAL EDGE  -----*/
            {
              col =  cont->vertex[i].x;
              row = (cont->vertex[i].y < cont->vertex[i+1].y) ?
			    cont->vertex[i].y : cont->vertex[i+1].y ;

              calculate_vert_features(sum, sumsq, fmin, fmax, row, col);
			  vert_aux[row][col] = 1;

              nedges++;
            }
            else
              if( cont->vertex[i].y == cont->vertex[i+1].y)
                /*----   HORIZONTAL EDGE  ----*/
                {
                  row = cont->vertex[i].y;
                  col = (cont->vertex[i].x < cont->vertex[i+1].x) ?
				    cont->vertex[i].x : cont->vertex[i+1].x ;

                  calculate_hzl_features(sum, sumsq, fmin, fmax, row, col);
				  hzl_aux[row][col] = 1;

                  nedges++;
                }
              else
                assert(FALSE);
        }
    } /* end for */


	/*======================================================*/
	/*====  Run Statisitcs on all edges in neighborhood ====*/
	/*======================================================*/

    /* scan hzlly to count VERT edges in neighborhood */
    for(j=1; j<(orig.height-1); j++)
    for(i=1; i<orig.width; i++)
    {
        if (vert_aux[j][i]==0 && (vert_aux[j-1][i] || vert_aux[j][i-1] ||
			vert_aux[j][i+1] || vert_aux[j+1][i] || hzl_aux[j][i-1] ||
			hzl_aux[j][i] || hzl_aux[j+1][i-1] || hzl_aux[j+1][i]))
        {
			calculate_vert_features(nsum, nsumsq, nfmin, nfmax, j, i);
		}
    } /* end for */
 
    /* scan vertically to count HZL edges in neighborhood  */

    for(i=1; i<(orig.width-1); i++)
    for(j=1,inside=0; j<orig.height; j++)
    {
        if (hzl_aux[j][i]==0 && (hzl_aux[j-1][i] || hzl_aux[j][i-1] ||
			hzl_aux[j][i+1] || hzl_aux[j+1][i] || vert_aux[j-1][i] ||
			vert_aux[j-1][i+1] || vert_aux[j][i] || vert_aux[j][i+1]))
        {
            calculate_hzl_features(nsum, nsumsq, nfmin, nfmax, j, i);
		}
    } /* end for */

    if (nedges > 0)
    {
        temp_list[0].status = ON;
        temp_list[1].status = ON;
        temp_list[2].status = OFF;
        temp_list[3].status = ON;
        temp_list[4].status = OFF;
        temp_list[5].status = ON;
        for(dum=0; dum<6; dum++)
        {
            mean = sum[dum]/nedges;
            stddev = sqrt((double)((sumsq[dum]/nedges) - (mean*mean)) );

            /*----------------------------------------*/
            /*--  Transfer Parameters to LIVE-wire ---*/
            /*----------------------------------------*/
            if(dum==0 || dum==1)
            {
                /*-- theoritical lo=Imin, hi=Imax  ---*/
                temp_list[dum].rmin = (fmin[dum]-Imin)/Range;
                temp_list[dum].rmax = (fmax[dum]-Imin)/Range;
                temp_list[dum].rmean = (mean-Imin)/Range;
                temp_list[dum].rstddev = stddev/Range;
            }
            else
            {
                /*-- theoritical lo=0, hi=(Imax-Imin) ---*/
                temp_list[dum].rmin = (fmin[dum])/Range;
                temp_list[dum].rmax = (fmax[dum])/Range;
                temp_list[dum].rmean = (mean)/Range;
                temp_list[dum].rstddev = stddev/Range;
            }

            if (temp_list[dum].status == ON)
                temp_list[dum].transform = 3;
        }
    }

	o_contour.last = -1;

	/*** Erase Temp Contours and Regions ***/
	clear_temporary_contours_array();
	for(dum=0; dum<(orig.width*orig.height); dum++)
	    region[dum] = 0;

    for(j=0; j < (orig.height+1); j++)
        free(hzl_aux[j]);  
    free(hzl_aux);
    for(j=0; j < orig.height; j++)
        free(vert_aux[j]);  
    free(vert_aux);

	reload();
}

void Segment2dCanvas::calculate_hzl_features(double sum[6], double sumsq[6],
	float fmin[6], float fmax[6], int j, int i)
{
	int val1, val2;
	float fval;
	unsigned char *ptr8;
	unsigned short *ptr16;


	if(orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
		val1 = (int)
			ptr8[*(tblr+j-1)+i-1]+ptr8[*(tblr+j-1)+i]+ptr8[*(tblr+j-1)+i+1];
		val2 = (int)
			ptr8[*(tblr+j)+i-1] + ptr8[*(tblr+j)+i] + ptr8[*(tblr+j)+i+1];

		/*====== Hi Density ======*/
		fval = (1.0/3)*(val1>val2 ? val1 : val2);
		fmin[0] = (fmin[0]==-1.0) ? fval : MIN( fmin[0], fval);
		fmax[0] = (fmax[0]==-1.0) ? fval : MAX( fmax[0], fval);
		sum[0] += (double)fval;
		sumsq[0] += (double)(fval*fval);

		/*====== Lo Density ======*/
    	fval = (1.0/3)*(val1<val2 ? val1 : val2);
		fmin[1] = (fmin[1]==-1.0) ? fval : MIN( fmin[1], fval);
        fmax[1] = (fmax[1]==-1.0) ? fval : MAX( fmax[1], fval);
    	sum[1] += (double)fval;
    	sumsq[1] += (double)(fval*fval);
 
		/*====== Abs_Grad1 =======*/
		fval = abs( (int)ptr8[*(tblr+j-1)+i] - (int)ptr8[*(tblr+j)+i] );
		fmin[2] = (fmin[2]==-1.0) ? fval : MIN( fmin[2], fval);
        fmax[2] = (fmax[2]==-1.0) ? fval : MAX( fmax[2], fval);
		sum[2] += (double)fval;
		sumsq[2] += (double)(fval*fval);

		/*====== Abs_Grad2 =======*/
		fval = (1.0/3)*abs((int)val1 - (int)val2);
		fmin[3] = (fmin[3]==-1.0) ? fval : MIN( fmin[3], fval);
        fmax[3] = (fmax[3]==-1.0) ? fval : MAX( fmax[3], fval);
		sum[3] += (double)fval;
		sumsq[3] += (double)(fval*fval);

		/*====== Abs_Grad3 =======*/
		fval = 0.5*abs((int)ptr8[*(tblr+j-1)+i] - (int)ptr8[*(tblr+j)+i] +
                 ((int)ptr8[*(tblr+j-1)+i-1] + (int)ptr8[*(tblr+j-1)+i+1]
                 - (int)ptr8[*(tblr+j)+i-1] - (int)ptr8[*(tblr+j)+i+1])/2 );
		fmin[4] = (fmin[4]==-1.0) ? fval : MIN( fmin[4], fval);
        fmax[4] = (fmax[4]==-1.0) ? fval : MAX( fmax[4], fval);
		sum[4] += (double)fval;
		sumsq[4] += (double)(fval*fval);

		/*====== Abs_Grad4 =======*/
		fval = (1.0/4) * (
	                abs((int)ptr8[*(tblr+j-1)+i-1] - (int)ptr8[*(tblr+j)+i])
	               +abs((int)ptr8[*(tblr+j-1)+i] - (int)ptr8[*(tblr+j)+i-1])
	               +abs((int)ptr8[*(tblr+j-1)+i] - (int)ptr8[*(tblr+j)+i+1])
		           +abs((int)ptr8[*(tblr+j-1)+i+1] - (int)ptr8[*(tblr+j)+i]) );
		fmin[5] = (fmin[5]==-1.0) ? fval : MIN( fmin[5], fval);
        fmax[5] = (fmax[5]==-1.0) ? fval : MAX( fmax[5], fval);
		sum[5] += (double)fval;
		sumsq[5] += (double)(fval*fval);
	}
	else
	if (orig.bits == 16)
    {
        ptr16= (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
        val1 = (int)
			ptr16[*(tblr+j-1)+i-1]+ptr16[*(tblr+j-1)+i]+ptr16[*(tblr+j-1)+i+1];
        val2 = (int)
			ptr16[*(tblr+j)+i-1] + ptr16[*(tblr+j)+i] + ptr16[*(tblr+j)+i+1];

		/*====== Hi Density ======*/
        fval = (1.0/3)*(val1>val2 ? val1 : val2);
        fmin[0] = (fmin[0]==-1.0) ? fval : MIN( fmin[0], fval);
        fmax[0] = (fmax[0]==-1.0) ? fval : MAX( fmax[0], fval);
        sum[0] += (double)fval;
        sumsq[0] += (double)(fval*fval);
 
        /*====== Lo Density ======*/
        fval = (1.0/3)*(val1<val2 ? val1 : val2);
        fmin[1] = (fmin[1]==-1.0) ? fval : MIN( fmin[1], fval);
        fmax[1] = (fmax[1]==-1.0) ? fval : MAX( fmax[1], fval);
        sum[1] += (double)fval;
        sumsq[1] += (double)(fval*fval);
 
        /*====== Abs_Grad1 =======*/
        fval = abs( (int)ptr16[*(tblr+j-1)+i] - (int)ptr16[*(tblr+j)+i] );
        fmin[2] = (fmin[2]==-1.0) ? fval : MIN( fmin[2], fval);
        fmax[2] = (fmax[2]==-1.0) ? fval : MAX( fmax[2], fval);
        sum[2] += (double)fval;
        sumsq[2] += (double)(fval*fval);
 
        /*====== Abs_Grad2 =======*/
        fval = (1.0/3)*abs((int)val1 - (int)val2);
        fmin[3] = (fmin[3]==-1.0) ? fval : MIN( fmin[3], fval);
        fmax[3] = (fmax[3]==-1.0) ? fval : MAX( fmax[3], fval);
        sum[3] += (double)fval;
        sumsq[3] += (double)(fval*fval);
 
        /*====== Abs_Grad3 =======*/
		fval = 0.5*abs((int)ptr16[*(tblr+j-1)+i] - (int)ptr16[*(tblr+j)+i] +
                 ((int)ptr16[*(tblr+j-1)+i-1] + (int)ptr16[*(tblr+j-1)+i+1]
                - (int)ptr16[*(tblr+j)+i-1] - (int)ptr16[*(tblr+j)+i+1])/2 );
        fmin[4] = (fmin[4]==-1.0) ? fval : MIN( fmin[4], fval);
        fmax[4] = (fmax[4]==-1.0) ? fval : MAX( fmax[4], fval);
        sum[4] += (double)fval;
        sumsq[4] += (double)(fval*fval);
 
        /*====== Abs_Grad4 =======*/
        fval = (1.0/4) * (
                 (int)abs(ptr16[*(tblr+j-1)+i-1] - (int)ptr16[*(tblr+j)+i])
                +(int)abs(ptr16[*(tblr+j-1)+i] - (int)ptr16[*(tblr+j)+i-1])
                +(int)abs(ptr16[*(tblr+j-1)+i] - (int)ptr16[*(tblr+j)+i+1])
                +(int)abs(ptr16[*(tblr+j-1)+i+1] - (int)ptr16[*(tblr+j)+i]) );
        fmin[5] = (fmin[5]==-1.0) ? fval : MIN( fmin[5], fval);
        fmax[5] = (fmax[5]==-1.0) ? fval : MAX( fmax[5], fval);
        sum[5] += (double)fval;
        sumsq[5] += (double)(fval*fval);
    }
}

void Segment2dCanvas::calculate_vert_features(double sum[6], double sumsq[6],
	float fmin[6], float fmax[6], int j, int i)
{
    int val1, val2;
	double fval;
    unsigned char *ptr8;
    unsigned short *ptr16;
 
 
    if(orig.bits == 8)
    {
        ptr8 = (unsigned char *)mCavassData->getSlice(mCavassData->m_sliceNo);
		val1 = (int)
			ptr8[*(tblr+j-1)+i-1]+ptr8[*(tblr+j)+i-1]+ptr8[*(tblr+j+1)+i-1];
        val2 = (int)
			ptr8[*(tblr+j-1)+i] + ptr8[*(tblr+j)+i] + ptr8[*(tblr+j+1)+i];


        /*====== Hi Density ======*/
        fval = (1.0/3) * (val1>val2 ? val1 : val2);
		fmin[0] = (fmin[0]==-1.0) ? fval : MIN( fmin[0], fval);
        fmax[0] = (fmax[0]==-1.0) ? fval : MAX( fmax[0], fval);
        sum[0] += (double)fval;
        sumsq[0] += (double)(fval*fval);
 
        /*====== Lo Density ======*/
        fval = (1.0/3)*(val1<val2 ? val1 : val2);
		fmin[1] = (fmin[1]==-1.0) ? fval : MIN( fmin[1], fval);
        fmax[1] = (fmax[1]==-1.0) ? fval : MAX( fmax[1], fval);
        sum[1] += (double)fval;
        sumsq[1] += (double)(fval*fval);
 
        /*====== Abs_Grad1 =======*/
        fval = abs( (int)ptr8[*(tblr+j)+i-1] - (int)ptr8[*(tblr+j)+i] );
		fmin[2] = (fmin[2]==-1.0) ? fval : MIN( fmin[2], fval);
        fmax[2] = (fmax[2]==-1.0) ? fval : MAX( fmax[2], fval);
        sum[2] += (double)fval;
        sumsq[2] += (double)(fval*fval);
 
        /*====== Abs_Grad2 =======*/
        fval = (1.0/3)*abs((int)val1-(int)val2);
		fmin[3] = (fmin[3]==-1.0) ? fval : MIN( fmin[3], fval);
        fmax[3] = (fmax[3]==-1.0) ? fval : MAX( fmax[3], fval);
        sum[3] += (double)fval;
        sumsq[3] += (double)(fval*fval);
 
        /*====== Abs_Grad3 =======*/
		fval = 0.5*abs((int)ptr8[*(tblr+j)+i-1] - (int)ptr8[*(tblr+j)+i] +
		         ( (int)ptr8[*(tblr+j-1)+i-1] + (int)ptr8[*(tblr+j+1)+i-1]
		          -(int)ptr8[*(tblr+j-1)+i] - (int)ptr8[*(tblr+j+1)+i] )/2 );
		fmin[4] = (fmin[4]==-1.0) ? fval : MIN( fmin[4], fval);
        fmax[4] = (fmax[4]==-1.0) ? fval : MAX( fmax[4], fval);
        sum[4] += (double)fval;
        sumsq[4] += (double)(fval*fval);
 
        /*====== Abs_Grad4 =======*/
        fval = (1.0/4) * (
                     abs((int)ptr8[*(tblr+j-1)+i-1] - (int)ptr8[*(tblr+j)+i])
                    +abs((int)ptr8[*(tblr+j)+i-1] - (int)ptr8[*(tblr+j-1)+i])
                    +abs((int)ptr8[*(tblr+j)+i-1] - (int)ptr8[*(tblr+j+1)+i])
                    +abs((int)ptr8[*(tblr+j+1)+i-1] - (int)ptr8[*(tblr+j)+i]));
		fmin[5] = (fmin[5]==-1.0) ? fval : MIN( fmin[5], fval);
        fmax[5] = (fmax[5]==-1.0) ? fval : MAX( fmax[5], fval);
        sum[5] += (double)fval;
        sumsq[5] += (double)(fval*fval);
    }
    else
    if (orig.bits == 16)
    {
        ptr16= (unsigned short *)mCavassData->getSlice(mCavassData->m_sliceNo);
        val1 = (int)
			ptr16[*(tblr+j-1)+i-1]+ptr16[*(tblr+j)+i-1]+ptr16[*(tblr+j+1)+i-1];
        val2 = (int)
			ptr16[*(tblr+j-1)+i] + ptr16[*(tblr+j)+i] + ptr16[*(tblr+j+1)+i];
 
 
        /*====== Hi Density ======*/
        fval = (1.0/3)*(val1>val2 ? val1 : val2);
        fmin[0] = (fmin[0]==-1.0) ? fval : MIN( fmin[0], fval);
        fmax[0] = (fmax[0]==-1.0) ? fval : MAX( fmax[0], fval);
        sum[0] += (double)fval;
        sumsq[0] += (double)(fval*fval);
 
        /*====== Lo Density ======*/
        fval = (1.0/3)*(val1<val2 ? val1 : val2);
        fmin[1] = (fmin[1]==-1.0) ? fval : MIN( fmin[1], fval);
        fmax[1] = (fmax[1]==-1.0) ? fval : MAX( fmax[1], fval);
        sum[1] += (double)fval;
        sumsq[1] += (double)(fval*fval);
 
        /*====== Abs_Grad1 =======*/
        fval = abs( (int)ptr16[*(tblr+j)+i-1] - (int)ptr16[*(tblr+j)+i] );
        fmin[2] = (fmin[2]==-1.0) ? fval : MIN( fmin[2], fval);
        fmax[2] = (fmax[2]==-1.0) ? fval : MAX( fmax[2], fval);
        sum[2] += (double)fval;
        sumsq[2] += (double)(fval*fval);
 
        /*====== Abs_Grad2 =======*/
        fval = (1.0/3)*abs((int)val1 - (int)val2);
        fmin[3] = (fmin[3]==-1.0) ? fval : MIN( fmin[3], fval);
        fmax[3] = (fmax[3]==-1.0) ? fval : MAX( fmax[3], fval);
        sum[3] += (double)fval;
        sumsq[3] += (double)(fval*fval);
 
        /*====== Abs_Grad3 =======*/
		fval = 0.5*abs((int)ptr16[*(tblr+j)+i-1] - (int)ptr16[*(tblr+j)+i] +
                 ((int)ptr16[*(tblr+j-1)+i-1] + (int)ptr16[*(tblr+j+1)+i-1]
                - (int)ptr16[*(tblr+j-1)+i] - (int)ptr16[*(tblr+j+1)+i])/2 );
        fmin[4] = (fmin[4]==-1.0) ? fval : MIN( fmin[4], fval);
        fmax[4] = (fmax[4]==-1.0) ? fval : MAX( fmax[4], fval);
        sum[4] += (double)fval;
        sumsq[4] += (double)(fval*fval);
 
        /*====== Abs_Grad4 =======*/
        fval = (1.0/4) * (
                  abs((int)ptr16[*(tblr+j-1)+i-1] - (int)ptr16[*(tblr+j)+i])
                 +abs((int)ptr16[*(tblr+j)+i-1] - (int)ptr16[*(tblr+j-1)+i])
                 +abs((int)ptr16[*(tblr+j)+i-1] - (int)ptr16[*(tblr+j+1)+i])
                 +abs((int)ptr16[*(tblr+j+1)+i-1] - (int)ptr16[*(tblr+j)+i]) );
        fmin[5] = (fmin[5]==-1.0) ? fval : MIN( fmin[5], fval);
        fmax[5] = (fmax[5]==-1.0) ? fval : MAX( fmax[5], fval);
        sum[5] += (double)fval;
        sumsq[5] += (double)(fval*fval);
    }
}
//----------------------------------------------------------------------
/** \brief Allow the user to scroll through the slices with the mouse wheel. */
void Segment2dCanvas::OnMouseWheel ( wxMouseEvent& e ) {
	Segment2dFrame*  sf = dynamic_cast<Segment2dFrame*>(m_parent_frame);
	sf->OnMouseWheel(e);
}


//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( Segment2dCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( Segment2dCanvas, wxPanel )
    EVT_CHAR(             Segment2dCanvas::OnChar         )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_LEFT_DOWN(        Segment2dCanvas::OnLeftDown     )
    EVT_LEFT_UP(          Segment2dCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      Segment2dCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        Segment2dCanvas::OnMiddleUp     )
    EVT_MOTION(           Segment2dCanvas::OnMouseMove    )
	EVT_MOUSEWHEEL(       Segment2dCanvas::OnMouseWheel   )
	EVT_LEFT_DCLICK(      Segment2dCanvas::OnLeftDClick   )
    EVT_PAINT(            Segment2dCanvas::OnPaint        )
    EVT_RIGHT_DOWN(       Segment2dCanvas::OnRightDown    )
    EVT_RIGHT_UP(         Segment2dCanvas::OnRightUp      )
    EVT_SIZE(             MainCanvas::OnSize            )
END_EVENT_TABLE()
//======================================================================

