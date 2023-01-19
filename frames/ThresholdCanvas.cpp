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
 * \file:  ThresholdCanvas.cpp
 * \brief  ThresholdCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

///* Structures for building interpolation tables  */
//typedef struct TABLE_INFO {
//  short  oldd[2],     /* old dimensions  */
//         newd[2];     /* new dimensions  */
//  short  offset[2],  /* Top left pos of image to be displayed */
//         dim[2];     /* dimensions of the part that is displayed */
//  short *index_table[2];  /* for each pix in new index into old (lower value)*/
//  unsigned int   *dist_table[2];   /* units = old_pix_w/h *old_w/h */
//} Table_Info;

using namespace std;

//----------------------------------------------------------------------
ThresholdCanvas::ThresholdCanvas ( void )  {  puts("ThresholdCanvas()");  }
//----------------------------------------------------------------------
ThresholdCanvas::ThresholdCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size, bool fuzzy )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
	fuzzy_flag = fuzzy;
    m_scale          = 1.0;
	m_overlay        = true;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 0;
	hist_size = 0;
	max_hist = -1;
	min_hist = 0;
	hist_scope = 0;
	hist_zoom_factor = 1.0;
	interval = 1;
	BLINK = false;
	opt_step = 1.0;
	BOUNDARY_TYPE = 0;
	num_track_points = 0;
	track_volume = 0/* current volume */;
	resetThresholds();

    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
}
//----------------------------------------------------------------------
ThresholdCanvas::~ThresholdCanvas ( void ) {
    cout << "ThresholdCanvas::~ThresholdCanvas" << endl;
    wxLogMessage( "ThresholdCanvas::~ThresholdCanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
}

void ThresholdCanvas::resetThresholds ( void ) {
	num_optima = 0;
	for (int i=1; i<=MAX_INTERVAL+1; i++)
	{
		min_table[i] = 1;
		max_table[i] = 0;
		tbar1x[i] = tbar2x[i] = -1;
		tbar3x[i] = -1;
		tbar4x[i] = -1;
		inter1_table[i] = 0;
		inter2_table[i] = 0;
	}
	thr_state = 1;
    SetStatusText( "Select Interval",   2 );
    SetStatusText( "Select Boundary", 3 );
    SetStatusText( "", 4 );
    lastX = lastY = -1;
	m_bThresholdDone = false;
}
//----------------------------------------------------------------------
double ThresholdCanvas::getHistCount( unsigned short value ) {
	return hist[value];
}
int ThresholdCanvas::getHistMin( void ) {
	return min_hist;
}
int ThresholdCanvas::getHistMax( void ) {
	return max_hist;
}
void ThresholdCanvas::setHistCount( double count, unsigned short value ) {
	hist[value] = count;
}
void ThresholdCanvas::setHistMin( int min ) {
	min_hist = min;
}
void ThresholdCanvas::setHistMax( int max ) {
	max_hist = max;
	hist_size = max+1;
}

// 10/2/07 Dewey Odhner
void ThresholdCanvas::moveThrBar1 ( double d )
{
	if (interval==0 || interval==MAX_INTERVAL+1)
		return;
	int intrvl=interval==MAX_INTERVAL+2? MAX_INTERVAL+1:interval;
	min_table[intrvl] = (int)d;
	for (tbar1x[intrvl]=histx;
			(tbar1x[intrvl]-histx)*(max_hist-min_hist)/histw+min_hist<
			min_table[intrvl]; tbar1x[intrvl]++)
		;
}

// 10/2/07 Dewey Odhner
void ThresholdCanvas::moveThrBar2 ( double d )
{
	if (interval==0 || interval==MAX_INTERVAL+1)
		return;
	int intrvl=interval==MAX_INTERVAL+2? MAX_INTERVAL+1:interval;
	inter1_table[intrvl] = (int)d;
	for (tbar2x[intrvl]=histx;
			(tbar2x[intrvl]-histx)*(max_hist-min_hist)/histw+min_hist<
			inter1_table[intrvl]; tbar2x[intrvl]++)
		;
}

// 10/2/07 Dewey Odhner
void ThresholdCanvas::moveThrBar3 ( double d )
{
	if (interval==0 || interval==MAX_INTERVAL+1)
		return;
	int intrvl=interval==MAX_INTERVAL+2? MAX_INTERVAL+1:interval;
	inter2_table[intrvl] = (int)d;
	for (tbar3x[intrvl]=histx;
			(tbar3x[intrvl]-histx)*(max_hist-min_hist)/histw+min_hist<
			inter2_table[intrvl]; tbar3x[intrvl]++)
		;
}

// 10/2/07 Dewey Odhner
void ThresholdCanvas::moveThrBar4 ( double d )
{
	if (interval==0 || interval==MAX_INTERVAL+1)
		return;
	int intrvl=interval==MAX_INTERVAL+2? MAX_INTERVAL+1:interval;
	max_table[intrvl] = (int)d;
	if (fuzzy_flag)
		for (tbar4x[intrvl]=histx;
				(tbar4x[intrvl]-histx)*(max_hist-min_hist)/histw+min_hist<
				max_table[intrvl]; tbar4x[intrvl]++)
		    ;
	else
		for (tbar2x[intrvl]=histx;
				(tbar2x[intrvl]-histx)*(max_hist-min_hist)/histw+min_hist<
				max_table[intrvl]; tbar2x[intrvl]++)
			;
}

void ThresholdCanvas::setHistZoomFactor( float f ) {
	hist_zoom_factor = f;
}
void ThresholdCanvas::setHistScope( int s /* 0= slice, 1= volume, 2= scene */ ) {
	hist_scope = s;
	compute_histogram(hist, mCavassData, s, 0, getSliceNo(0));
}
void ThresholdCanvas::setOptimum  ( int rank, int value ) {
	assert(rank < MAX_OPTIMA);
	optimum[rank] = value;
}
void ThresholdCanvas::setNumOptima( int n ) {
	num_optima = n;
}

// 7/24/04 Dewey Odhner
void ThresholdCanvas::findOptima( void )
{
    int low,hi, j;
    char cmnd[200];
    FILE *fp;

    sprintf(cmnd, "optimal_threshold \"%s\" ", mCavassData->m_fname);
    if (tbar1x[MAX_INTERVAL+1]==-1 || tbar2x[MAX_INTERVAL+1]==-1) {
        SetStatusText("Please specify the OPT range first", 0);
        return;
    }
	SetStatusText("", 0);
    low= min_table[MAX_INTERVAL+1];
    hi = max_table[MAX_INTERVAL+1];
    sprintf(cmnd, "%s%d %d %d > TMP_OPT_THR", cmnd, low, hi, (int)opt_step);
    printf("%s\n",cmnd);
    SetStatusText("Working...", 1);

    //construct the command string
#ifdef PROCESS_MANAGER_WORKS
#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
    wxString  cmd = wxString::Format( "%s/%s",
        (const char *)Preferences::getHome().c_str(), cmnd );
#else
	wxString  cmd(cmnd);
#endif
    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    ProcessManager  p( "optimal_threshold running...", cmd );
    int  error = p.getStatus();
    if (p.getCancel())    return;
    j = error;
#else
	j = system(cmnd);
#endif

    SetStatusText("", 1);
    if (j != 0)
    {
#if 0
	    wxMessageBox("Process failed.");
		return;
#endif
	}
	fp = fopen("TMP_OPT_THR", "rb");
	if (fp == NULL)
    {
	    wxMessageBox("Process failed.");
		return;
	}
	for (num_optima=0; num_optima<MAX_OPTIMA && !feof(fp); num_optima++)
		if (fscanf(fp, "%d\n", optimum+num_optima) != 1)
		{
			wxMessageBox("Process failed.");
			return;
		}
	unlink("TMP_OPT_THR");
	Refresh();
}
//----------------------------------------------------------------------
void ThresholdCanvas::release ( void ) {
}
//----------------------------------------------------------------------
void ThresholdCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void ThresholdCanvas::loadFile ( const char* const fn ) {
    SetCursor( wxCursor(wxCURSOR_WAIT) );    
    wxSafeYield(NULL, true);
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }
    /* really bad constructor of CavassData with a file name as parameter*/
    mCavassData = new CavassData( fn );

	min_hist = mCavassData->m_min;
	if (min_hist < 0)
		min_hist = 0;
	max_hist = mCavassData->m_max;
    mOverallXSize = mCavassData->m_xSize;
    mOverallYSize = mCavassData->m_ySize;
    mOverallZSize = mCavassData->m_zSize;
    m_rows = m_cols = 1;
	int         w, h;
	GetSize( &w, &h );
	resx = 2*w/3;
	resy = 0;
	histx = w/3;
	histy = GetCharHeight()*(11+2*(num_optima<1?1:num_optima))/2;
	histw = w/3-2;
	histh = 300;
	m_scale = (double)(histx-2)/mOverallXSize;

    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void ThresholdCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    if (which==0)    mCavassData->initLUT();
}


/************************************************************************
 *                                                                      *
 *      Function        : Redraw_Threshold_Values                       *
 *      Description     : This function redraws the histograms and      *
 *                        displays the intervals that were selected     *
 *                        and also shows the icona nd result image with *
 *                        with the proper threshold (overlay) values.   *
 *      Return Value    :  None
 *      Parameters      : dc: 
 *      Side effects    : None.                                         *
 *      Entry condition : tbar1x, tbar2x, min_table, max_table,
 *                        must be properly set.
 *                        VCreateColormap must be called first.
 *      Related funcs   : None.                                         *
 *      History         : Written on November 20, 1992 by Krishna Iyer. *
 *                        Modified 9/18/97 interval value restored
 *                        by Dewey Odhner.
 *                        Modified 11/8/01 for optimal mode
 *                        by Dewey Odhner.
 *                        Modified 7/18/07 corrected for optimal mode
 *                        by Dewey Odhner.
 *                        Modified 9/13/07 for fuzzy thresholding
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::Redraw_Threshold_Values(wxDC & dc)
{
    int i;

    i = interval;
	if (interval == MAX_INTERVAL+2)
	{
        if (tbar1x[MAX_INTERVAL+1] != -1)
            draw_bar_end(dc, tbar1x[MAX_INTERVAL+1],
				Left, min_table[MAX_INTERVAL+1]);
        if (tbar2x[MAX_INTERVAL+1] != -1)
            draw_bar_end(dc, tbar2x[MAX_INTERVAL+1],
				Right, max_table[MAX_INTERVAL+1]);
        if (tbar1x[MAX_INTERVAL+1] != -1 && tbar2x[MAX_INTERVAL+1] != -1)
            draw_bar(dc, tbar1x[MAX_INTERVAL+1], tbar2x[MAX_INTERVAL+1]);
	}
	else
      for(interval=1; interval<=MAX_INTERVAL; interval++)
      {
		int *tbartx = (fuzzy_flag? tbar4x: tbar2x)+interval;
        if (tbar1x[interval] != -1)
            draw_bar_end(dc, tbar1x[interval], Left, min_table[interval]);
        if (*tbartx != -1)
            draw_bar_end(dc, *tbartx, Right, max_table[interval]);
		if (fuzzy_flag)
		{
			if (tbar2x[interval] >= 0)
				draw_bar_end(dc,tbar2x[interval],Right,inter1_table[interval]);
			if (tbar3x[interval] >= 0)
				draw_bar_end(dc,tbar3x[interval],Right,inter2_table[interval]);
		}
        if (tbar1x[interval] != -1 && *tbartx != -1)
            draw_bar(dc, tbar1x[interval], *tbartx);
      }

    interval = i;
}
//----------------------------------------------------------------------

/************************************************************************
 *                                                                      *
 *      Function        : initialize_lookup_table                       *
 *      Description     : This function will iniitalize the lookup table* 
 *                        used to create the result (binary) image.     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : max_hist
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::initialize_lookup_table()
{
	int i;

	for(i=0;i<max_hist+1;i++)
		lookup_table[i] = 0;
}


/************************************************************************
 *                                                                      *
 *      Function        : create_new_lookup_table                       *
 *      Description     : This function will create the new lokup table *
 *                        for the interval selected.                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : interval, min_table, max_table must be set
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified: 4/25/07 for CAVASS by Dewey Odhner
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::create_new_lookup_table(ThresholdColor c)
{
	int i;

	initialize_lookup_table();

	if(interval == MAX_INTERVAL+1 /* ALL */)
		for(i=1; i<=MAX_INTERVAL; i++)
			make_lookup_table(min_table[i],max_table[i], c);
	else if(interval == MAX_INTERVAL+2 /* OPT */)
		make_lookup_table(min_table[MAX_INTERVAL+1],
			max_table[MAX_INTERVAL+1], c);
	else if (fuzzy_flag)
		make_lookup_table(min_table[interval], max_table[interval], c,
			inter1_table[interval], inter2_table[interval]);
	else
		make_lookup_table(min_table[interval], max_table[interval], c);
}


/************************************************************************
 *                                                                      *
 *      Function        : make_lookup_table                             *
 *      Description     : This function will make the actual lookup     *
 *                        table for the whole image.                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  None.                                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::make_lookup_table(int min, int max, ThresholdColor c,
	int inter1, int inter2)
{
	int i, threshold = 255;

	unsigned char red, green, blue;
	switch (interval)
	{
		case 1:
			red = 1;
			green = 127;
			blue = 255;
			break;
		case 2:
			red = 1;
			green = 255;
			blue = 127;
			break;
		case 3:
			red = 255;
			green = 127;
			blue = 127;
			break;
		case 4:
			red = 1;
			green = 255;
			blue = 255;
			break;
		case MAX_INTERVAL+1:
			red = 255;
			green = 127;
			blue = 255;
			break;
		case MAX_INTERVAL+2:
			red = 127;
			green = 255;
			blue = 1;
			break;
		default:
			assert(FALSE);
			return;
	}
	switch (c)
	{
		case WHITE:
			threshold = 255;
			break;
		case RED:
			threshold = red;
			break;
		case GREEN:
			threshold = green;
			break;
		case BLUE:
			threshold = blue;
			break;
	}

	for (i=min;i<=max;i++) 
		lookup_table[i] = threshold;
	if (inter1>min && inter1<max)
		for (i=min; i<inter1; i++)
			lookup_table[i] = threshold*(1+i-min)/(1+inter1-min);
	if (inter2>min && inter2<max)
		for (i=inter2+1; i<=max; i++)
			lookup_table[i] = threshold*(1+max-i)/(1+max-inter2);
}

void build_table(Table_Info *tbl)
{
    int i1,d,i;

    for(d=0;d<2;d++)
       for(i1=0;i1<tbl->dim[d];i1++) { 
          i = i1 +tbl->offset[d];
	      tbl->index_table[d][i1]=
	         (short)((int)((2*i+1)*tbl->oldd[d])/(int)(2*tbl->newd[d])); 
       }
}

// Return non-zero on error.
// 7/17/07 Dewey Odhner
int compute_histogram(double hist[65536], CavassData *cd,
		int scope /* 0= slice, 1= volume, 2= scene */,
		int volume, int slice /* from 0 */)
{
	if (!cd->dataIsLoaded())
		return 1;
	const int slice_size = cd->m_vh.scn.xysize[0]*cd->m_vh.scn.xysize[1];
	long offset = 0;
	long end = 0;
	switch (scope)
	{
		case 2:
			volume = 0;
		case 1:
			slice = 0;
	}
	assert(slice < cd->m_zSize);
	if (cd->m_vh.scn.dimension == 4)
	{
		assert(volume < cd->m_vh.scn.num_of_subscenes[0]);
		for (int v=0; v<volume; v++)
			offset += cd->m_vh.scn.num_of_subscenes[v+1]*slice_size;
	}
	switch (scope)
	{
		case 0:
			offset += slice*slice_size;
			end = offset+slice_size;
			break;
		case 1:
			end = offset+slice_size*cd->m_vh.scn.num_of_subscenes[
				cd->m_vh.scn.dimension==4? volume+1: 0];
			break;
		case 2:
			if (cd->m_vh.scn.dimension == 4)
				for (int v=0; v<cd->m_vh.scn.num_of_subscenes[0]; v++)
					end += cd->m_vh.scn.num_of_subscenes[v+1]*slice_size;
			else
				end = cd->m_vh.scn.num_of_subscenes[0]*slice_size;
			break;
	}
	memset(hist, 0, sizeof(long)*(cd->m_max+1));
	if (cd->m_vh.scn.num_of_bits == 8)
	{
		unsigned char *data = (unsigned char *)cd->m_data;
		for (long j=offset; j<end; j++)
			if (data[j])
				hist[data[j]]++;
	}
	else if (cd->m_vh.scn.num_of_bits == 16)
	{
		unsigned short *data = (unsigned short *)cd->m_data;
		for (long j=offset; j<end; j++)
			if (data[j])
				hist[data[j]]++;
	}
	return 0;
}

// 7/13/07 Dewey Odhner
void ThresholdCanvas::CreateDisplayImage(int which)
{
    //note: image data is 24-bit rgb
	const CavassData&  A = *mCavassData;


	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}
    const int offset = A.m_sliceNo * A.m_xSize * A.m_ySize;

	if (hist_scope==0 && which==0)
	{
		for (int i=0; i<=max_hist; i++)
			hist[i] = 0;
		total_count = 0;
	}
    if (A.m_size==1) 
    {
      unsigned char* cData = (unsigned char*)A.m_data;
      for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 &&
	     j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
	  {
		if(which==0)
		{
			slice[i] = slice[i+1] = slice[i+2] = A.m_lut[cData[j]-A.m_min];
			if (hist_scope == 0)
				hist[cData[j]]++;
		}
	  }
    } 
    else if (A.m_size==2) 
    {
      unsigned short* sData;
	  sData = (unsigned short*)(A.m_data);

      for (int i=0,j=offset; i<A.m_xSize*A.m_ySize*3 &&
	     j<offset+A.m_xSize*A.m_ySize; i+=3,j++) 
	  {
		  if(which==0)
		  {
			slice[i] = slice[i+1] = slice[i+2] = A.m_lut[sData[j]-A.m_min];
			if (hist_scope == 0)
				hist[sData[j]]++;
		  }
      }
    } 

	if(which==0 && !BLINK)
	{
		if(m_images[0]!=NULL)
			m_images[0]->Destroy();
		m_images[0] = new wxImage( A.m_xSize, A.m_ySize, slice );
		if (m_scale!=1.0) 
		{
			m_images[0]->Rescale( (int)(ceil(m_scale*A.m_xSize)),
							  (int)(ceil(m_scale*A.m_ySize)) );
		}
		m_bitmaps[0] = new wxBitmap( (const wxImage&) *m_images[0] );
	}
	else
	{
		if(m_images[which]!=NULL)
			m_images[which]->Destroy();
		if (!BLINK)
			create_new_lookup_table(WHITE);
        int buf_size = A.m_xSize*A.m_ySize*3;
		unsigned char *s_data = (unsigned char *)malloc(buf_size);
        unsigned short *pr16;
        unsigned char *pr8;
        int index;
		Table_Info tbl;

        /***************Thresholding to be done******************/
		tbl.newd[0] = tbl.oldd[0] = tbl.dim[0] = A.m_xSize;
		tbl.newd[1] = tbl.oldd[1] = tbl.dim[1] = A.m_ySize;
		tbl.offset[0] = tbl.offset[1] = 0;
        tbl.index_table[0]=(short *)malloc(sizeof(short)*
                                        A.m_xSize);
        tbl.index_table[1]=(short *)malloc(sizeof(short)*
                                        A.m_ySize);
        tbl.dist_table[0]=(unsigned int *)malloc(sizeof(int)*
                                        A.m_xSize);
        tbl.dist_table[1]=(unsigned int *)malloc(sizeof(int)*
                                        A.m_ySize);

        build_table(&tbl);

        /***********Using the lookup table***********/
		if (!BLINK)
		{
        	if (A.m_size == 1)
                for(pr8=(unsigned char *)A.m_data+offset,index=0;
                                index<buf_size;pr8++,index+=3)
                    s_data[index]=s_data[index+1]=s_data[index+2]=
						lookup_table[*pr8];
        	if(A.m_size == 2)
                for(pr16=(unsigned short *)A.m_data+offset,index=0;
                                index<buf_size;pr16++,index+=3)
                    s_data[index]=s_data[index+1]=s_data[index+2]=
						lookup_table[*pr16];
		}
		else
		{
			create_new_lookup_table(RED);
			if (which==0)
			{
				if (A.m_size == 1)
					for(pr8=(unsigned char *)A.m_data+offset,index=0;
									index<buf_size;pr8++,index+=3)
					{
						s_data[index] = lookup_table[*pr8];
						if (s_data[index] == 0)
							s_data[index] = slice[index];
					}
				if (A.m_size == 2)
					for(pr16=(unsigned short *)A.m_data+offset,index=0;
									index<buf_size;pr16++,index+=3)
					{
						s_data[index] = lookup_table[*pr16];
						if (s_data[index] == 0)
							s_data[index] = slice[index];
					}
				create_new_lookup_table(GREEN);
				if (A.m_size == 1)
					for(pr8=(unsigned char *)A.m_data+offset,index=0;
									index<buf_size;pr8++,index+=3)
					{
						s_data[index+1] = lookup_table[*pr8];
						if (s_data[index+1] == 0)
							s_data[index+1] = slice[index+1];
					}
				if (A.m_size == 2)
					for(pr16=(unsigned short *)A.m_data+offset,index=0;
									index<buf_size;pr16++,index+=3)
					{
						s_data[index+1] = lookup_table[*pr16];
						if (s_data[index+1] == 0)
							s_data[index+1] = slice[index+1];
					}
				create_new_lookup_table(BLUE);
				if (A.m_size == 1)
					for(pr8=(unsigned char *)A.m_data+offset,index=0;
									index<buf_size;pr8++,index+=3)
					{
						s_data[index+2] = lookup_table[*pr8];
						if (s_data[index+2] == 0)
							s_data[index+2] = slice[index+2];
					}
				if(A.m_size == 2)
					for(pr16=(unsigned short *)A.m_data+offset,index=0;
									index<buf_size;pr16++,index+=3)
					{
						s_data[index+2] = lookup_table[*pr16];
						if (s_data[index+2] == 0)
							s_data[index+2] = slice[index+2];
					}
			}
			else
			{
				if (A.m_size == 1)
					for(pr8=(unsigned char *)A.m_data+offset,index=0;
									index<buf_size;pr8++,index+=3)
						s_data[index] = lookup_table[*pr8];
				if (A.m_size == 2)
					for(pr16=(unsigned short *)A.m_data+offset,index=0;
									index<buf_size;pr16++,index+=3)
						s_data[index] = lookup_table[*pr16];
				create_new_lookup_table(GREEN);
				if (A.m_size == 1)
					for(pr8=(unsigned char *)A.m_data+offset,index=0;
									index<buf_size;pr8++,index+=3)
						s_data[index+1] = lookup_table[*pr8];
				if (A.m_size == 2)
					for(pr16=(unsigned short *)A.m_data+offset,index=0;
									index<buf_size;pr16++,index+=3)
						s_data[index+1] = lookup_table[*pr16];
				create_new_lookup_table(BLUE);
				if (A.m_size == 1)
					for(pr8=(unsigned char *)A.m_data+offset,index=0;
									index<buf_size;pr8++,index+=3)
						s_data[index+2] = lookup_table[*pr8];
				if(A.m_size == 2)
					for(pr16=(unsigned short *)A.m_data+offset,index=0;
									index<buf_size;pr16++,index+=3)
						s_data[index+2] = lookup_table[*pr16];
			}
		}
		m_images[which] = new wxImage( A.m_xSize, A.m_ySize, s_data );
		if (m_scale!=1.0) 
		{
			m_images[which]->Rescale( (int)(ceil(m_scale*A.m_xSize)),
							  (int)(ceil(m_scale*A.m_ySize)) );
		}
		m_bitmaps[which] = new wxBitmap( *m_images[which] );
	}
}
//----------------------------------------------------------------------
void ThresholdCanvas::reload ( void ) {
	freeImagesAndBitmaps();

	CreateDisplayImage(0);

	if(m_bThresholdDone) /* update filtering */
		RunThreshold(); 
	Refresh();
}

//----------------------------------------------------------------------
// 7/26/07 Dewey Odhner
void ThresholdCanvas::mapWindowToData ( int wx, int wy,
								 int& x, int& y, int& z, int& which ) {
    //map a point in the window back into the 3d data set
    wx -= m_tx;    wy -= m_ty;

	which = wx>=resx;
	if (which)
	{
		wx -= resx;
		wy -= resy;
    }
	x = (int)( wx / mCavassData->m_scale / m_scale );
    y = (int)( wy / mCavassData->m_scale / m_scale );
    z = mCavassData->m_sliceNo;
    //flag unacceptable values
    if (x<0 || x>=mCavassData->m_xSize || y<0 || y>=mCavassData->m_ySize ||
			z<0 || z>=mCavassData->m_zSize)
		which = -1;
}

void ThresholdCanvas::RunThreshold()
{
    CreateDisplayImage(1);
	m_bThresholdDone = true;
	Refresh();
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void ThresholdCanvas::OnChar ( wxKeyEvent& e ) {
}
//----------------------------------------------------------------------
// Modified 7/18/07 corrected for optimal mode by Dewey Odhner.
// Modified 9/14/07 for fuzzy thresholding by Dewey Odhner.
void ThresholdCanvas::OnMouseMove ( wxMouseEvent& e ) {
    //if (m_data==NULL)    return;
    
    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();

    if (e.LeftIsDown() || e.RightIsDown()) {
    } else {
        if (lastX!=-1 || lastY!=-1) {
            SetCursor( *wxSTANDARD_CURSOR );
            lastX = lastY = -1;
        }
		int intrvl = interval==MAX_INTERVAL+2? MAX_INTERVAL+1:interval;
		bool fuzzy = interval==MAX_INTERVAL+2? false: fuzzy_flag;
		int eventx, eventy;
		eventx = pos.x;
		eventy = pos.y;
        if (thr_state==2 || thr_state==3)
        {
           if(eventx < histx) eventx = histx;
           if(eventx > histx+histw) eventx = histx+histw;
        }
		int *tbartx = (fuzzy? tbar4x: tbar2x)+intrvl;
        if(thr_state == 2)
		{
			if (!fuzzy)
				substate = 0;
			int *tbarcx =
				(substate==1? tbar2x: substate==2? tbar3x: tbar1x)+intrvl;
			int *ctablem = (substate==1? inter1_table:
				substate==2? inter2_table: min_table)+intrvl;
			if (substate)
			{
			   if (eventx < tbar1x[intrvl])
			      eventx = tbar1x[intrvl];
			   if (substate>1 && eventx < tbar2x[intrvl])
			      eventx = tbar2x[intrvl];
			}

			/* erase previous bar end-point & connection-bar (if any) */
			if(*tbarcx >= 0)
			{
			   erase_bar_end(dc, *tbarcx, substate==0? Left:Right);
			   if(eventx>tbar1x[intrvl] && substate==0)
			      erase_bar(dc, tbar1x[intrvl], eventx);
			}

			if (fuzzy)
			{
			   if (substate<2 && tbar3x[intrvl]>=0 && eventx>tbar3x[intrvl])
			   {
			      erase_bar_end(dc, tbar3x[intrvl], Right);
				  tbar3x[intrvl] = eventx;
				  inter2_table[intrvl]=((tbar3x[intrvl] - histx)*
				            (max_hist-min_hist)/histw+min_hist);
				  draw_bar_end(dc, tbar3x[intrvl],Right,inter2_table[intrvl]);
			   }
			   if (substate==0 && tbar2x[intrvl]>=0 && eventx>tbar2x[intrvl])
			   {
			      erase_bar_end(dc, tbar2x[intrvl], Right);
				  tbar2x[intrvl] = eventx;
				  inter1_table[intrvl]=((tbar2x[intrvl] - histx)*
				            (max_hist-min_hist)/histw+min_hist);
				  draw_bar_end(dc, tbar2x[intrvl],Right,inter1_table[intrvl]);
			   }
			}

			/* if left bar-end pushes right bar-end to the right */
			if(*tbartx >= 0 && eventx > *tbartx)
			{
			   erase_bar_end(dc, *tbartx, Right);
			   *tbartx = eventx;
			   max_table[intrvl]=((*tbartx - histx)*
							(max_hist-min_hist)/histw+min_hist);
			   draw_bar_end(dc, *tbartx, Right,
				  max_table[intrvl]);
			}

			*tbarcx = eventx;
			*ctablem=((*tbarcx - histx)*
						 (max_hist-min_hist)/histw+min_hist);

			/* draw new bar end-point */
			draw_bar_end(dc, *tbarcx, substate==0? Left:Right, *ctablem);

			/* draw new connection-bar (if final point is specified) */
			if (*tbartx >= 0)
			   draw_bar(dc, tbar1x[intrvl], *tbartx);
			else if (fuzzy)
			{
			   if (tbar3x[intrvl] >= 0)
			      draw_bar(dc, tbar1x[intrvl], tbar3x[intrvl]);
			   else if (tbar2x[intrvl] >= 0)
			      draw_bar(dc, tbar1x[intrvl], tbar2x[intrvl]);
			}

			if (fuzzy)
				RedrawHistogram(&dc);
			display_percentile(dc, eventx);
			UpdateThresholdTablets();
		}
		else if(thr_state == 3)
		{ 
			if (eventx < tbar1x[intrvl])
			   eventx = tbar1x[intrvl];
			if (fuzzy && eventx<tbar3x[intrvl])
			   eventx = tbar3x[intrvl];

			/* erase previous connection-bar (if any) */
			if(tbar1x[intrvl] >= 0  &&  *tbartx >= 0)
			   erase_bar(dc, tbar1x[intrvl], *tbartx);

			/* erase previous bar end-point (if any) */
			if(*tbartx >= 0)
			{
			   erase_bar_end(dc, *tbartx, Right);
			   if (*tbartx == tbar1x[intrvl])
				  draw_bar_end(dc, tbar1x[intrvl], Left, min_table[intrvl]);
			}

			*tbartx = eventx;
			max_table[intrvl]=((*tbartx - histx)*
						 (max_hist-min_hist)/histw+min_hist);

			/* draw new bar end-point */
			draw_bar_end(dc, *tbartx, Right, max_table[intrvl]);

			/* draw new connection-bar (if final point is specified) */
			draw_bar(dc, tbar1x[intrvl], *tbartx);

			if (fuzzy)
				RedrawHistogram(&dc);
			display_percentile(dc, eventx);
			UpdateThresholdTablets();
		}
    }
}

/************************************************************************
 *                                                                      *
 *      Function        : draw_bar_end                                  *
 *      Description     : This function draws the end points in the     *
 *                        histogram, while selecting the intervals.     *
 *      Return Value    : None.
 *      Parameters      :  dc - the dc of the image window.             *
 *                         x - the location of the end point in pixels  *
 *                              with respect to histogram coordinates.  *
 *                         mode - whether right or left interval.       *
 *                         value - the interval end value.
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified: 11/7/01 value parameter added
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::draw_bar_end(wxDC &dc, int x, int mode, int value)
{
	char text[12];
	int yl;
	int font_h, font_w;

    dc.SetTextBackground( *wxBLACK );
    dc.SetTextForeground( wxColour(Yellow) );
	dc.SetPen( wxPen(wxColour(Yellow)) );
	font_h = GetCharHeight();
	font_w = GetCharWidth();

	/* keep bar inside histogram area */
	if(x < histx) x = histx;
	if(x > histx+histw) x = histx+histw;

	sprintf(text,"%d", value);
	yl= (int)(2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

	dc.DrawLine(x, histy+histh+3+yl, x, histy+histh+4+font_h+yl);
	if(mode == Left)
	    dc.DrawText(text, x-3-font_w*strlen(text),
	    	histy+histh+3+font_h+font_h/4+yl);
	else
	    dc.DrawText(text, x+3,  histy+histh+3+font_h+font_h/4+yl);
}


/************************************************************************
 *                                                                      *
 *      Function        : erase_bar_end                                 *
 *      Description     : This function erases the bar end in the       *
 *                        histogram.                                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  dc - the dc of the image window.             *
 *                         x - the coordinate of the interval with      *
 *                              respect to the histogram.               *
 *                         mode - whether it is the left or right       *
 *                              interval.                               *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified 9/18/97 gc deleted by Dewey Odhner.
 *                        Modified 10/7/97 5 characters width of text
 *                        cleared by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::erase_bar_end(wxDC &dc, int x, int mode)
{
	int font_h,font_w,yl;
 
	dc.SetPen( wxPen(GetBackgroundColour()) );
	dc.SetBrush( wxBrush(GetBackgroundColour()) );
	font_h = GetCharHeight();
	font_w = GetCharWidth();

	/* keep bar inside histogram area */
	if(x < histx) x = histx;
	if(x > histx+histw) x = histx+histw;

	/* clear bar */
	yl=(int) (2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

	dc.DrawRectangle(x, histy+histh+3+yl, 1, font_h+font_h/8);
	/* clear text under bar-end */
	if(mode == Left)
	   dc.DrawRectangle(x-3-5*font_w,  histy+histh+3+font_h+font_h/4+yl, 5*font_w, font_h+font_h/8);
	else
	   dc.DrawRectangle(x+3, histy+histh+3+font_h+font_h/4+yl, 5*font_w, font_h+font_h/8);

}

/************************************************************************
 *                                                                      *
 *      Function        : draw_bar                                      *
 *      Description     : This function draws the bar connecting the two*
 *                        end points while selecting the range in the   *
 *                        histogram.                                    *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  dc - the GC of the image window.             *
 *                         x1 - the left interval.                      *
 *                         x2 - the right interval.                     *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::draw_bar(wxDC &dc, int x1, int x2)
{
	int font_h, yl;
 
	dc.SetPen( wxPen(wxColour(Yellow)) );
	font_h = GetCharHeight();

	/* keep bar inside histogram area */
	if(x1 < histx) x1 = histx;
	if(x1 > histx+histw) x1 = histx+histw;
	if(x2 < histx) x2 = histx;
	if(x2 > histx+histw) x2 = histx+histw;

	yl = (int) (2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

	dc.DrawLine(x1, histy+histh+3+font_h/2+yl, x2+1,
					histy+histh+3+font_h/2+yl);
}


/************************************************************************
 *                                                                      *
 *      Function        : erase_bar                                     *
 *      Description     : This function erases the bar between the      *
 *                        end points in the histogram.                  *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  dc - the dc of the image window.             *
 *                         x1 - the left bar.                           *
 *                         x2 - the right bar.                          *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on August 20, 1992 by Krishna Iyer.   *
 *                        Modified 9/18/97 gc deleted by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
void ThresholdCanvas::erase_bar(wxDC &dc, int x1, int x2)
{
    int yl;

	dc.SetPen( wxPen(GetBackgroundColour()) );
	dc.SetBrush( wxBrush(GetBackgroundColour()) );
	int font_h = GetCharHeight();
    if(interval != 0)
    {
        yl = (int) (2.5*(interval==MAX_INTERVAL+2? 0: interval-1)*font_h);

        dc.DrawRectangle(x1, histy+histh+3+font_h/2+yl, x2-x1+3, 1);
    }
}

/*****************************************************************************
 * FUNCTION: display_percentile
 * DESCRIPTION: Displays the percentile of the nonzero counts at the historam
 *    position eventx in the image window.
 * PARAMETERS:
 *    eventx: The position on the histogram relative to the image window.
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: VCreateColormap must be called first.  The variables
 *    hist, min_hist, max_hist, total_count, display, img, histx, histw,
 *    must be properly set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: 
 * HISTORY:
 *    Created: 10/6/97 by Dewey Odhner
 *
 *****************************************************************************/
void ThresholdCanvas::display_percentile(wxDC &dc, int eventx)
{
	int j, jmax;
	double cum_count;
    int font_h;
	char msg[12];

	if (total_count == 0)
		return;
	jmax = (eventx-histx)*(max_hist-min_hist)/histw + min_hist;
	if (jmax > max_hist)
		jmax = max_hist;
	cum_count = 0;
	for (j=min_hist?min_hist:1; j<=jmax; j++)
		cum_count += hist[j];
    dc.SetTextBackground( *wxBLACK );
    dc.SetTextForeground( wxColour(Yellow) );
	dc.SetPen( wxPen(GetBackgroundColour()) );
	dc.SetBrush( wxBrush(GetBackgroundColour()) );
	font_h = GetCharHeight();
	sprintf(msg, "%%ile: %.1f", 100.*cum_count/total_count);
	dc.DrawRectangle(histx, font_h*9/2, histw, font_h);
	dc.DrawText( msg, histx, font_h*9/2);
}



//----------------------------------------------------------------------
void ThresholdCanvas::OnRightDown ( wxMouseEvent& e ) {
#if 0
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(0)) {
        mCavassData->mDisplay = false;  //!mCavassData->mDisplay;
        reload();
    }
#else
    //gjg: simulate the middle button (useful on a 2-button mouse)
    OnMiddleDown( e );
#endif
}
//----------------------------------------------------------------------
void ThresholdCanvas::OnRightUp ( wxMouseEvent& e ) {
    if (isLoaded(0)) {
        mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
// 7/26/07 Dewey Odhner
void ThresholdCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
    //remove translation
    wxClientDC  dc(this);
    PrepareDC(dc);
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
    lastX = wx;
    lastY = wy;
	int eventx, eventy;
	eventx = pos.x;
	eventy = pos.y;
	int  x, y, z, which;
	mapWindowToData( dc.DeviceToLogicalX( pos.x ),
	                 dc.DeviceToLogicalY( pos.y ), x, y, z, which );

	if (thr_state==1 && which>=0 &&
		(BOUNDARY_TYPE==1||mCavassData->m_vh.scn.num_of_bits==1))
	{
		if (num_track_points >= MAX_TRACK_POINTS)
		{
			wxMessageBox("Specifying too many points.");
			return;
		}
		if (mCavassData->m_vh.scn.dimension == 4) {
		  if (0/* current volume */ != track_volume) {
			wxMessageBox("Specifying points on multiple volumes. Reseting to current volume");
			track_volume = 0/* current volume */;
			num_track_points=0;
		  }
		}
		track_pt[num_track_points][0] = x;
		track_pt[num_track_points][1] = y;
		track_pt[num_track_points][2] = z;
		num_track_points++;

		dc.SetPen( wxPen(wxColour(Yellow)) );
		dc.DrawLine(eventx-2,eventy-2,eventx+3,eventy+3);
		dc.DrawLine(eventx+2,eventy-2,eventx-3,eventy+3);
	}
	if (thr_state != 3)
		return;
    SetCursor( wxCursor(wxCURSOR_HAND) );

    if(eventx < histx) eventx = histx;
    if(eventx > histx+histw) eventx = histx+histw;
	int intrvl = interval==MAX_INTERVAL+2? MAX_INTERVAL+1: interval;
	bool fuzzy = interval==MAX_INTERVAL+2? false: fuzzy_flag;
	int *tbartx = (fuzzy? tbar4x: tbar2x)+intrvl;
	assert(tbar1x[intrvl] >= 0);
	if (eventx < tbar1x[intrvl])
		eventx = tbar1x[intrvl];
	if (fuzzy && eventx < tbar3x[intrvl])
		eventx = tbar3x[intrvl];
	*tbartx = eventx;

	/* draw new bar end-point */
	draw_bar_end(dc, *tbartx, Right, (*tbartx
					- histx)*(max_hist-min_hist)/histw+min_hist);

	/* draw new connection-bar (if final point is specified) */
	draw_bar(dc, tbar1x[intrvl], *tbartx);

	/* PERFORM OPERATION */

	min_table[intrvl]=((tbar1x[intrvl] - histx)*
					(max_hist-min_hist)/histw+min_hist);
	max_table[intrvl]=((*tbartx - histx)*
					(max_hist-min_hist)/histw+min_hist);
	if (fuzzy)
	{
		inter1_table[intrvl]=((tbar2x[intrvl] - histx)*
		            (max_hist-min_hist)/histw+min_hist);
		inter2_table[intrvl]=((tbar3x[intrvl] - histx)*
		            (max_hist-min_hist)/histw+min_hist);
	}
	RunThreshold(); 

	/*The overlay for the icon is created as soon as the */
	/*lookup table is ready.                             */

	thr_state = 1;
    SetStatusText( "Select Interval",   2 );
    SetStatusText( "Select Boundary", 3 );
    SetStatusText( "", 4 );
	UpdateThresholdTablets();
}
//----------------------------------------------------------------------
void ThresholdCanvas::OnMiddleUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
void ThresholdCanvas::OnLeftDown ( wxMouseEvent& e ) {
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

	int eventx, eventy;
	eventx = pos.x;
	eventy = pos.y;
    if (eventx < histx)
		eventx = histx;
	if (eventx > histx+histw)
		eventx = histx+histw;
	int font_h = GetCharHeight();
	int intrvl = interval==MAX_INTERVAL+2? MAX_INTERVAL+1: interval;
	bool fuzzy = interval==MAX_INTERVAL+2? false: fuzzy_flag;
	int *tbartx = (fuzzy? tbar4x: tbar2x)+intrvl;
	if (thr_state == 0)
	{
		thr_state = 1;
	    SetStatusText( "Select Interval",   2 );
	    SetStatusText( "Select Boundary", 3 );
	    SetStatusText( "", 4 );
	}
	else if (thr_state == 1)
	{
		if(tbar1x[intrvl] >= 0 && *tbartx >= 0)
				erase_bar(dc, tbar1x[intrvl], *tbartx);

		if(tbar1x[intrvl] >= 0)
				erase_bar_end(dc, tbar1x[intrvl], Left);

		if(eventy>=font_h*11/2 && eventy<font_h*(11+2*num_optima)/2)
		{  int o;
			  for (o=0; eventy>=font_h*(13+2*o)/2; o++)
				;
			  if (fuzzy)
			  {
				erase_bar_end(dc, tbar3x[intrvl], Right);
			    int oval=optimum[o];
				if (oval < inter1_table[intrvl])
				  oval = inter1_table[intrvl];
				inter2_table[intrvl] = oval;
				for (tbar3x[intrvl]=histx; (tbar3x[intrvl]-histx)*
				      (max_hist-min_hist)/histw+min_hist<inter2_table[intrvl];
					  tbar3x[intrvl]++)
				  ;
				if (tbar2x[intrvl]>=0 && tbar1x[intrvl]>tbar2x[intrvl])
				{
				  erase_bar_end(dc, tbar2x[intrvl], Right);
				  tbar2x[intrvl] = tbar1x[intrvl];
				  inter1_table[intrvl] = ((tbar2x[intrvl] - histx)*
				    	(max_hist-min_hist)/histw+min_hist);
				}
			  }
			  else
			  {
			    min_table[intrvl] = optimum[o];
			    for (tbar1x[intrvl]=histx; (tbar1x[intrvl]-histx)
					  *(max_hist-min_hist)/histw+min_hist<
					  min_table[intrvl]; tbar1x[intrvl]++)
				  ;
			  }

			  /* if left bar-end pushes right bar-end to the right */
			  if (*tbartx >= 0 && tbar1x[intrvl] > *tbartx)
			  {
				erase_bar_end(dc, *tbartx, Right);
				*tbartx = tbar1x[intrvl];
				max_table[intrvl]=((*tbartx - histx)*
						(max_hist-min_hist)/histw+min_hist);
				draw_bar_end(dc, *tbartx, Right,
				  max_table[intrvl]);
			  }

			  if (*tbartx < 0)
			  {
				max_table[intrvl] = max_hist;
				*tbartx = histx+histw;
				draw_bar_end(dc, *tbartx, Right,
				  max_table[intrvl]);
			  }

			  /* draw new bar end-point */
			  draw_bar_end(dc, tbar1x[intrvl], Left,
				min_table[intrvl]);

			  /* draw new connection-bar (if final point is specified) */
			  draw_bar(dc, tbar1x[intrvl], *tbartx);

			  RunThreshold(); 
			  UpdateThresholdTablets();
		}
		else
		{
			if (fuzzy && intrvl>1 && tbar3x[intrvl-1]>=0 &&
					tbar4x[intrvl-1]>=0)
			{
				if (tbar1x[intrvl]<0 || tbar2x[intrvl]<0)
				{
					tbar1x[intrvl] = tbar3x[intrvl-1];
					tbar2x[intrvl] = tbar4x[intrvl-1];
				    min_table[intrvl]=((tbar1x[intrvl] - histx)*
							(max_hist-min_hist)/histw+min_hist);
				    inter1_table[intrvl]=((tbar2x[intrvl] - histx)*
				            (max_hist-min_hist)/histw+min_hist);
				}
				if (eventx < tbar2x[intrvl])
					eventx = tbar2x[intrvl];
				tbar3x[intrvl] = eventx;
				substate = 2;
				SetStatusText( "Third Point", 2 );
			}
			else
			{
				tbar1x[intrvl] = eventx; /* set location for initial bar */
				substate = 0;
				SetStatusText( "Initial Point", 2 );
			}

			/* draw new bar-end (if there is any) */
			draw_bar_end(dc, tbar1x[intrvl], Left, (tbar1x[intrvl] - histx)*
						(max_hist-min_hist)/histw+min_hist);

			/* erase previous connection-bar (if any) */
			if(tbar1x[intrvl] >= 0 && *tbartx >= 0)
				draw_bar(dc, tbar1x[intrvl], *tbartx);

			display_percentile(dc, eventx);
			UpdateThresholdTablets();

			thr_state = 2;
			SetStatusText( "", 3 );
			SetStatusText( "", 4 );
		}
	}
	else if (thr_state == 2)
	{
	  if (fuzzy)
	  {
	    switch (substate)
		{
			case 0:
				tbar1x[intrvl] = eventx;
				draw_bar_end(dc, tbar1x[intrvl], Left, min_table[intrvl]);
				substate = 1;
				SetStatusText( "Second Point", 2 );
				break;
			case 1:
				if (eventx < tbar1x[intrvl])
					eventx = tbar1x[intrvl];
				tbar2x[intrvl] = eventx;
				draw_bar_end(dc, tbar2x[intrvl], Right, inter1_table[intrvl]);
				substate = 2;
				SetStatusText( "Third Point", 2 );
				break;
			case 2:
				if (eventx < tbar2x[intrvl])
					eventx = tbar2x[intrvl];
				tbar3x[intrvl] = eventx;
				draw_bar_end(dc, tbar3x[intrvl], Right, inter2_table[intrvl]);
				thr_state = 3;
				SetStatusText( "", 2 );
				SetStatusText( "Final Point", 3 );
				break;
		}
		if (tbar4x[intrvl] > eventx)
			draw_bar(dc, tbar1x[intrvl], tbar4x[intrvl]);
		else
			draw_bar(dc, tbar1x[intrvl], eventx);
	  }
	  else
	  {
		tbar1x[intrvl] = eventx;

		/* draw new bar end-point */
		draw_bar_end(dc, tbar1x[intrvl],Left,min_table[intrvl]);

		/* draw new connection-bar (if final point is specified) */
		if(tbar2x[intrvl] > tbar1x[intrvl])
		{
			draw_bar(dc, tbar1x[intrvl], tbar2x[intrvl]);
		}

		thr_state = 3;
		SetStatusText( "", 2 );
		SetStatusText( "Final Point", 3 );
		SetStatusText( "", 4 );
	  }
	  UpdateThresholdTablets();
	}
}
//----------------------------------------------------------------------
void ThresholdCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void ThresholdCanvas::OnPaint ( wxPaintEvent& e ) {
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

    wxPaintDC  dc(this);
    PrepareDC(dc);
    //dc.BeginDrawing();  deprecated
    dc.Blit(0, 0, w, h, &m, 0, 0);  //works on windoze
    //dc.DrawBitmap( bitmap, 0, 0 );  //doesn't work on windblows

    paint( &dc );
    //dc.EndDrawing();  deprecated
}
//----------------------------------------------------------------------
void ThresholdCanvas::paint ( wxDC* dc ) {
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
		if (BOUNDARY_TYPE)
		  DisplayTrackPoints(0, dc);
	  }
	  if (m_bitmaps[1]!=NULL && m_bitmaps[1]->Ok()) 
	  {
	    dc->DrawBitmap( *m_bitmaps[1], m_tx+resx, m_ty+resy );
	    if(m_overlay)
		{
		  const wxString  s = wxString::Format( "%d/%d",getSliceNo(0)+1,getNoSlices(0));
		  dc->DrawText( s, m_tx+resx, m_ty+resy );
		}
		if (BOUNDARY_TYPE)
		{
		  DisplayTrackPoints(1, dc);
		  dc->SetPen( wxPen(wxColour(Yellow)) );
		}
	  }
	  if (max_hist > min_hist)
	  {
        int i, mode, min_postiv, max_postiv;
		double maxH, total_val;
		double total_valSquare, var;
		char msg[32];
        int font_h = GetCharHeight();
		histy = font_h*(11+2*(num_optima<1?1:num_optima))/2;
		total_val = total_count = maxH = mode = min_postiv = max_postiv = 0;
		total_valSquare = 0;
		for (i=min_hist?min_hist:1; i<=max_hist; i++)
			if (hist[i])
			{
				if (hist[i] > maxH)
					mode=i, maxH=hist[i];
				total_count += hist[i];
				total_val += (double)hist[i]*i;
				total_valSquare += (double)hist[i]*i*i;
				if (min_postiv == 0)
					min_postiv = i;
				max_postiv = i;
			}

		if (total_count)
		{
			sprintf(msg, "Mean: %.1f", total_val/total_count);
			dc->DrawText(msg, histx, font_h/2);
			var = total_valSquare/total_count - (total_val/total_count)*(total_val/total_count);
			sprintf(msg, "Std. Dev.: %.1f", sqrt(var));
			dc->DrawText(msg, histx, font_h*9/2);
		}
		sprintf(msg, "Mode: %d", mode);
		dc->DrawText(msg, histx, font_h*3/2);
		sprintf(msg, "Min.: %d", min_postiv);
		dc->DrawText(msg, histx, font_h*5/2);
		sprintf(msg, "Max.: %d", max_postiv);
		dc->DrawText(msg, histx, font_h*7/2);
		for (i=0; i<num_optima; i++)
		{
			sprintf(msg, "Opt.: %d", optimum[i]);
			dc->DrawText(msg, histx, font_h*(11+2*i)/2);
		}

		RedrawHistogram(dc);
		Redraw_Threshold_Values(*dc);
	  }
	}
	else if (m_backgroundLoaded) {
        int  w, h;
        dc->GetSize( &w, &h );
        const int  bmW = m_backgroundBitmap.GetWidth();
        const int  bmH = m_backgroundBitmap.GetHeight();
        dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
    }
}


/************************************************************************
 *                                                                      
 *      Function        : draw_thresh_fn                             
 *      Description     : Draws a line between the specified points     
 *                        no restriction on the y-position
 *      Return Value    : None                                          
 *      Parameters      : x1, y1, x2, y2  the x,y co-ordinates
 *                        of the points between which the line is to 
 *                        be drawn
 *                  
 *      Side effects    : None                                          
 *      Limitations     :                               
 *      Entry condition :                       
 *                                                                      
 *      Related funcs   : draw_bar_end, erase_bar_end, draw_bar 
 *                                              
 *      History         : Written by K. P. Venugopal 27th July 1993 
 *                                                                      
 ************************************************************************/

void ThresholdCanvas::draw_thresh_fn(wxDC* dc, int x1, int y1, int x2, int y2)
{
    if(x1 < histx) x1 = histx;
    if(x1 > histx+histw) x1 = histx+histw;
    if(x2 < histx) x2 = histx;
    if(x2 > histx+histw) x2 = histx+histw;

	dc->DrawLine(x1, y1, x2, y2);
}

void ThresholdCanvas::RedrawHistogram( wxDC* dc )
{
	wxColour bg;
	if (Preferences::getCustomAppearance())
		bg = wxColour(DkBlue);
	else
		bg = *wxBLACK;
	dc->SetPen( wxPen(bg) );
	dc->SetBrush( wxBrush(bg) );
	dc->DrawRectangle( histx, histy, histw, histh );
    double maxH = 1;
    int  i;
    for (i=min_hist?min_hist:1; i<=max_hist; i++)
        if(hist[i]>maxH) maxH = hist[i];
	dc->SetPen( wxPen(wxColour(Yellow)) );
    for (i=min_hist?min_hist:1; i<=max_hist; i++)
    {
        int posX = (int) (histx+(i-min_hist)*histw/(max_hist-min_hist));
        int posY = (int) (histy+histh-(histh*hist[i]*hist_zoom_factor/maxH));
        if(posY < histy) posY = histy; 
        dc->DrawLine(posX, histy+histh, posX, posY+1);
    }
	dc->SetPen( wxPen(wxColour(32, 160, 255)) );
	if (fuzzy_flag)
		for (int intrvl=1; intrvl<=MAX_INTERVAL; intrvl++)
		{
	        if(tbar1x[intrvl] != -1 && tbar2x[intrvl] != -1) {
                draw_thresh_fn(dc, tbar1x[intrvl], histy+histh,
                                                tbar2x[intrvl], histy);
                  if(tbar3x[intrvl] != -1) {
    	            draw_thresh_fn(dc, tbar2x[intrvl], histy, 
                                                tbar3x[intrvl], histy);
                    if (tbar4x[intrvl] != -1)
					  draw_thresh_fn(dc, tbar3x[intrvl], histy, 
                                                tbar4x[intrvl], histy+histh);
                  }
	        }
		
		}
	dc->SetPen( wxPen(wxColour(Yellow)) );
}

/************************************************************************
 *                                                                      *
 *      Function        : DisplayTrackPoints                            *
 *      Description     : This function will display the point on the   *
 *                        icon, selected by the user to track the       *
 *                        boundary.                                     *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      : which: image to mark, 0= original, 1=result
 *                        dc: device context
 *      Side effects    : None.                                         *
 *      Entry condition : track_volume, mCavassData->m_sliceNo,
 *                        num_track_points, track_pt, m_scale, m_tx, m_ty
 *                        mCavassData->m_scale, resx, resy
 *      Related funcs   : None.                                         *
 *      History         : Written on March 10, 1993 by S.Samarasekera.  *
 *                                                                      *
 ************************************************************************/
int ThresholdCanvas::DisplayTrackPoints(int which, wxDC *dc)
{
	int i, cur_vol, cur_slice, eventx,eventy;

	cur_vol=0;
	if (cur_vol!=track_volume) return(0);
	cur_slice = mCavassData->m_sliceNo;
	if (which)
		dc->SetPen( wxPen(wxColour(Red)) );
	else
		dc->SetPen( wxPen(wxColour(Yellow)) );
	for(i=0;i<num_track_points;i++) {
		if (cur_slice == track_pt[i][2]) {
			eventx = (int)(track_pt[i][0]*m_scale*mCavassData->m_scale)+m_tx;
			eventy = (int)(track_pt[i][1]*m_scale*mCavassData->m_scale)+m_ty;
			if (which)
			{
				eventx += resx;
				eventy += resy;
			}
			dc->DrawLine(eventx-2,eventy-2,eventx+3,eventy+3);
			dc->DrawLine(eventx+2,eventy-2,eventx-3,eventy+3);
		}
	}
	return(0); 
}
//----------------------------------------------------------------------
void ThresholdCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool ThresholdCanvas::isLoaded ( const int which ) const {
    if (which==0) {
        if (mCavassData==NULL)    return false;
        const CavassData&  cd = *mCavassData;
        if (cd.m_fname!=NULL && strlen(cd.m_fname)>0)
		    return cd.mIsCavassFile && cd.m_vh_initialized;
        return false;
    } else if (which==1) {
        return false;
    } else {
        assert( 0 );
    }
    return false;
}

bool ThresholdCanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int ThresholdCanvas::getCenter ( const int which ) const {
    assert( mCavassData!=NULL );
    const CavassData&  cd = *mCavassData;
    return cd.m_center;
}
//----------------------------------------------------------------------
int ThresholdCanvas::getMax ( const int which ) const {
    assert( mCavassData!=NULL );
    const CavassData&  cd = *mCavassData;
    return cd.m_max;
}
//----------------------------------------------------------------------
int ThresholdCanvas::getMin ( const int which ) const {
    assert( mCavassData!=NULL );
    const CavassData&  cd = *mCavassData;
    return cd.m_min;
}
//----------------------------------------------------------------------
//number of slices in entire data set
int ThresholdCanvas::getNoSlices ( const int which ) const {
    assert( mCavassData!=NULL );
    const CavassData&  cd = *mCavassData;
    return cd.m_zSize;
}

bool ThresholdCanvas::getThresholdDone (void) const
{
	return m_bThresholdDone;
}
//----------------------------------------------------------------------
double ThresholdCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int ThresholdCanvas::getSliceNo ( const int which ) const {
    assert( mCavassData!=NULL );
    const CavassData&  cd = *mCavassData;
    return cd.m_sliceNo;
}
//----------------------------------------------------------------------
int ThresholdCanvas::getWidth ( const int which ) const {
    assert( mCavassData!=NULL );
    const CavassData&  cd = *mCavassData;
    return cd.m_width;
}
//----------------------------------------------------------------------
bool ThresholdCanvas::getInvert ( const int which ) const {
    assert( mCavassData!=NULL );
    CavassData&  cd = *mCavassData;
    return cd.mInvert;
}
void ThresholdCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void ThresholdCanvas::setCenter ( const int which, const int center ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_center = center;
    }
}

//----------------------------------------------------------------------
void ThresholdCanvas::setInvert ( const int which, const bool invert ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.mInvert = invert;
    }
}
void ThresholdCanvas::setThresholdDone(bool done)
{
	m_bThresholdDone = done;
}
//----------------------------------------------------------------------
void ThresholdCanvas::setScale   ( const double scale )  {  //aka magnification
    //must do this now before we (possibly) change m_rows and/or m_cols
    freeImagesAndBitmaps();

    m_scale = scale;
	histx = (int)rint(mOverallXSize*m_scale)+2;
	histw = histx-2;
	resx = 2*histx;
	int interval_was=interval;
	for (interval=1; interval<=MAX_INTERVAL+1; interval++)
	{
		int intrvl=interval;
		if (interval==MAX_INTERVAL+1)
			interval = MAX_INTERVAL+2;
		if (tbar1x[intrvl] == -1)
			continue;
		double t1=min_table[intrvl], t2=inter1_table[intrvl],
			t3=inter2_table[intrvl], t4=max_table[intrvl];
		moveThrBar1(t1);
		moveThrBar2(t2);
		moveThrBar3(t3);
		moveThrBar4(t4);
	}
	interval = interval_was;
    reload();
}
//----------------------------------------------------------------------
void ThresholdCanvas::setSliceNo ( const int which, const int sliceNo ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_sliceNo = sliceNo;
    }
}
//----------------------------------------------------------------------
void ThresholdCanvas::setWidth ( const int which, const int width ) {
    if (which==0) {
        assert( mCavassData!=NULL );
        CavassData&  cd = *mCavassData;
        cd.m_width = width;
    }
}
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( ThresholdCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( ThresholdCanvas, wxPanel )
    EVT_PAINT(            ThresholdCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           ThresholdCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        ThresholdCanvas::OnLeftDown     )
    EVT_LEFT_UP(          ThresholdCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      ThresholdCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        ThresholdCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       ThresholdCanvas::OnRightDown    )
    EVT_RIGHT_UP(         ThresholdCanvas::OnRightUp      )
    EVT_CHAR(             ThresholdCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
