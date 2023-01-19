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
 * \file:  FilterCanvas.cpp
 * \brief  FilterCanvas class implementation
 * 
 * \author Zhuge Yin, Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "FilterCanvas.h"

using namespace std;
//----------------------------------------------------------------------
FilterCanvas::FilterCanvas ( void )  {  puts("FilterCanvas()");  }
//----------------------------------------------------------------------
FilterCanvas::FilterCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
    m_scale          = 1.0;
    m_overlay        = true;
    m_images[0] = m_images[1] = NULL;
    m_bitmaps[0] = m_bitmaps[1] = NULL;
    m_rows = m_cols = 1;
    m_tx = m_ty     = 40;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
	m_filterType = FILTER_GRADIENT2D;
	initializeParameters();
	m_nLTDTPararell = false;
	m_nLTDTFTSave = false;
	m_nSBAv3DPararell = false;
}
//----------------------------------------------------------------------
FilterCanvas::~FilterCanvas ( void ) {
    cout << "FilterCanvas::~FilterCanvas" << endl;
    wxLogMessage( "FilterCanvas::~FilterCanvas" );
    freeImagesAndBitmaps(); 
    release(); /* mCanvassData released in MainCanvas */
	
	if( m_sliceIn != NULL )
	{
		delete m_sliceIn;
		m_sliceIn = NULL;
	}
	if( m_sliceOut != NULL )
	{
		delete m_sliceOut;
		m_sliceOut = NULL;
	}
}
//----------------------------------------------------------------------
void FilterCanvas::initializeParameters ( void )
{
	m_bFilterDone = false;
	m_Sigma = 1.5;
	m_ScaleADIterations = 10;
	m_Homogen = 50;
	m_MorphN = 5;
	m_MaxRadius = 10;
	m_MinRadius = 1;
	m_MorphIterations = 1;
	m_nLTDTDistType = 0;
	m_ZetaFactor = 2.0;
	m_IncluFactor = 0.6;
	m_StdDevia = 0;
	m_NumOfRegion = 1;
	m_VolThresh = 0.005;
	m_StopCond = 0.001;
	m_LeftParam = 1.0;
	m_RightParam = 1.0;

	min_thresh[0] = 400;
	max_thresh[0] = 673;
	min_thresh[1] = 674;
	max_thresh[1] = 734;
	min_thresh[2] = 735;
	max_thresh[2] = 826;
	min_thresh[3] = 827;
	max_thresh[3] = 1250;
	num_interval = 3;
}
//----------------------------------------------------------------------
void FilterCanvas::release ( void ) {
}
//----------------------------------------------------------------------
void FilterCanvas::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
}
//----------------------------------------------------------------------
void FilterCanvas::loadFile ( const char* const fn ) 
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    
    wxYield();
    release();
    if (fn==NULL || strlen(fn)==0) {
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }    
	
    CavassData*  cd = new CavassData( fn, true );
	if( cd == NULL ) //|| cd != NULL && cd->m_data == NULL )
		return; // ERR_CLASSCONSTRUCTION;
   
	SliceData*  sd = new SliceData( fn );	
	if( sd == NULL ) //|| cd != NULL && cd->m_data == NULL )
		return; // ERR_CLASSCONSTRUCTION;
	if (!cd->mIsCavassFile || !cd->m_vh_initialized)
	{
		wxMessageBox("Failed to load file.");
		return; // ERR_LOADCAVASSFILE;
	}

    if (mFileOrDataCount==0)
	{
        assert( mCavassData==NULL );
        mCavassData = cd;

        CavassData&  A = *mCavassData;
        A.mR = 1.0;  A.mG = 1.0;  A.mB = 1.0;
	    m_cols  = m_rows = 1;
		
		m_sliceIn = sd;
		m_sliceIn->m_sliceNo = -1;
	}
	else
	{
		if( mFileOrDataCount >= 2 )
		{
			if (mCavassData->mNext!=NULL)
			{
				delete mCavassData->mNext;
				mCavassData->mNext = NULL;
			}
		}
		mFileOrDataCount = 1;
		if (m_sliceOut)
		{
			cd->m_center = sd->m_center = m_sliceOut->m_center;
			cd->m_width = sd->m_width = m_sliceOut->m_width;
			delete m_sliceOut;
		}
		mCavassData->mNext = cd;
		freeImagesAndBitmaps();
		m_cols = 2;

		m_sliceOut = sd;
		m_sliceOut->m_sliceNo = -1;
		initLUT(1);
    }
    ++mFileOrDataCount;

	if (isLoaded(0))
	{
		int  w, h;
		mOverallXSize = mCavassData->m_xSize;
		GetSize( &w, &h );
		m_scale = (double)((w-150)*1.0 / (mOverallXSize * 2));
	}

    reload();
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void FilterCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}
//----------------------------------------------------------------------
void FilterCanvas::CreateDisplayImage(int which)
{
	//note: image data is 24-bit rgb
	const CavassData&  A = *mCavassData;
	const CavassData&  B = *(mCavassData->mNext);

	unsigned char*  slice=(unsigned char *)malloc(A.m_xSize*A.m_ySize*3);
    if (slice == NULL)
	{
		wxMessageBox("Out of memory.");
		return;
	}

	if(which==0)
	{
		if (A.m_size==1) 
		{
			unsigned char*   cData = NULL;

			cData = (unsigned char*)m_sliceIn->getSlice(A.m_sliceNo); //A.m_data;
			if( m_sliceIn->m_sliceNo != A.m_sliceNo )   // It don't need to initial LUT when Adjust the GreyMap
			{				
				m_sliceIn->m_sliceNo = A.m_sliceNo;
			}		

			if( cData == NULL )
				return;

			for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
			{				
				slice[i] = slice[i+1] = slice[i+2] = m_sliceIn->m_lut[cData[j] - m_sliceIn->m_min];			
			}

			cData = NULL;		
		} 
		else if (A.m_size==2) 
		{
			unsigned short* sData = NULL;

			sData = (unsigned short*)m_sliceIn->getSlice(A.m_sliceNo); //(A.m_data);
			if( m_sliceIn->m_sliceNo != A.m_sliceNo )
			{					
				m_sliceIn->m_sliceNo = A.m_sliceNo;
			}		

			if( sData == NULL )
				return;

			for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
			{			
				slice[i] = slice[i+1] = slice[i+2] = m_sliceIn->m_lut[sData[j] - m_sliceIn->m_min];			  
			}

			sData = NULL;		
		} 
		else
		{
			int* iData = NULL;		
			iData = (int*)m_sliceIn->getSlice(A.m_sliceNo); //(A.m_data);
			if( m_sliceIn->m_sliceNo != A.m_sliceNo )
			{	
				m_sliceIn->m_sliceNo = A.m_sliceNo;
			}

			if( iData == NULL )
				return;

			for (int i=0,j=0; i<A.m_xSize*A.m_ySize*3 && j<A.m_xSize*A.m_ySize; i+=3,j++) 
			{
				slice[i] = slice[i+1] = slice[i+2] = m_sliceIn->m_lut[iData[j] - m_sliceIn->m_min];

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
		if (B.m_size==1) 
		{
			unsigned char*   cData = NULL;

			cData = (unsigned char*)m_sliceOut->getSlice(B.m_sliceNo); //B.m_data;
			if( m_sliceOut->m_sliceNo != B.m_sliceNo )
			{				
				m_sliceOut->m_sliceNo = B.m_sliceNo;
			}

			if( cData == NULL )
				return;

			for (int i=0,j=0; i<B.m_xSize*B.m_ySize*3 && j<B.m_xSize*B.m_ySize; i+=3,j++) 
			{			
				slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[cData[j] - m_sliceOut->m_min];
			}

			cData = NULL;		
		} 
		else if (B.m_size==2) 
		{
			unsigned short* sData = NULL;

			sData = (unsigned short*)m_sliceOut->getSlice(B.m_sliceNo); //(B.m_data);	  
			if( m_sliceOut->m_sliceNo != B.m_sliceNo )
			{					   
				m_sliceOut->m_sliceNo = B.m_sliceNo;
			}

			if( sData == NULL )
				return;

			for (int i=0,j=0; i<B.m_xSize*B.m_ySize*3 && j<B.m_xSize*B.m_ySize; i+=3,j++) 
			{			
				slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[sData[j] - m_sliceOut->m_min];		  
			}

			sData = NULL;		
		} 
		else 
		{
			int* iData = NULL;

			iData = (int*)m_sliceOut->getSlice(B.m_sliceNo); //(B.m_data);	  
			if( m_sliceOut->m_sliceNo != B.m_sliceNo )
			{	
				m_sliceOut->m_sliceNo = B.m_sliceNo;
			}	

			if( iData == NULL )
				return;

			for (int i=0,j=0; i<B.m_xSize*B.m_ySize*3 && j<B.m_xSize*B.m_ySize; i+=3,j++) 
			{

				slice[i] = slice[i+1] = slice[i+2] = m_sliceOut->m_lut[iData[j] - m_sliceOut->m_min];
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
	//delete [] slice;
}
//----------------------------------------------------------------------
void FilterCanvas::reload ( void ) 
{
  //if (!isLoaded(0) || !isLoaded(1))    return;
    freeImagesAndBitmaps();

	CreateDisplayImage(0);

	if(m_bFilterDone) /* update filtering */		
		CreateDisplayImage(1);

    Refresh();
}

//----------------------------------------------------------------------
void FilterCanvas::mapWindowToData ( int wx, int wy,
                                 int& x, int& y, int& z ) {
}

void FilterCanvas::RunFilter()
{

	switch (m_filterType) 
	{
	case FILTER_LTDT3D:
	case FILTER_DIST3D:
	case FILTER_DIST2Dxy:
		if( mCavassData->m_vh.scn.num_of_bits != 1 )
		{
			wxLogMessage( "Only accept BIM format!" );
			wxMessageBox( "Only accept BIM format!" );
			return;
		}
		break;

	  case FILTER_BALL_ENH:
	    if (mCavassData->m_vh.scn.num_of_bits == 1)
		{
		    wxLogMessage( "Not implemented for BIM format!" );
			wxMessageBox( "Not implemented for BIM format!" );
			return;
		}
	default:
		break;
	}


	switch (m_filterType) 
	{
		case FILTER_BALL_ENH:
			if (mCavassData->m_vh.scn.num_of_subscenes[0] == 1)
				break;
			// fall through
		case FILTER_LTDT3D:
		case FILTER_SCALE3D: 
		case FILTER_SBAv3D:
		case FILTER_SBAD3D:
		case FILTER_DIST3D:
		case FILTER_INHOMO1:
		case FILTER_INHOMO2:
		case FILTER_INHOMO3:
		case FILTER_MAXIMA:
			wxLogMessage( "Cannot do slice mode operation for this filter!" );
			wxMessageBox( "Cannot do slice mode operation for this filter!" );
			return;

		default:
			break;
	}

	if( m_filterType == FILTER_DILATE || m_filterType == FILTER_ERODE  )
	{
		int n = getMorphN();
		if( n == 7 || n == 19 || n == 27 )
		{
			wxLogMessage( "It is a 3D morph operation. Cannot do slice mode!" );
			wxMessageBox( "It is a 3D morph operation. Cannot do slice mode!" );
			return;
		}
	}
	  
		//remove anything that may be left behind from before
	unlink( "voi_tmp.IM0"  );
	unlink( "voi_tmp2.IM0" );
	//step 1:
	//  run the nvdoi program to obtain a file that contains only the
	//  current slice.    
	wxString  voiCmd("\"");
	voiCmd += Preferences::getHome()+wxString::Format( "/ndvoi\" \"%s\" %s %d %d %d %d %d %d %d %d %d",mCavassData->m_fname, "voi_tmp.IM0", 
		0, 0, 0, mCavassData->m_xSize,mCavassData->m_ySize, 0, 0, mCavassData->m_sliceNo, mCavassData->m_sliceNo );
	wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
	ProcessManager  q( "ndvoi running...", voiCmd, true, false, false );
	if (q.getCancel())    return;
	int  error = q.getStatus();
#if 0
	if (error != 0) {  wxMessageBox( "ndvoi failed." );  return;  }
#endif

	// step 2:
	wxString  cmd;
	int iterations=1;
    switch (m_filterType) 
	{
      case FILTER_GAUSSIAN2D:
          cmd = wxString::Format("gaussian2d %s %s %d %f", "voi_tmp.IM0", "voi_tmp2.IM0", 0, getSigma());
          break;
      case FILTER_GAUSSIAN3D:
          cmd = wxString::Format("gaussian3d %s %s %d %f", "voi_tmp.IM0", "voi_tmp2.IM0", 0, getSigma());
          break;
	  case FILTER_LTDT3D:
		  cmd = wxString::Format("LTDT3D %s %s %d %d %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0, getLTDTDistType(), 0);
          break;
      case FILTER_SCALE2D:
          cmd = wxString::Format("scale_based_filtering_2D %s %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", "scale_tmp", getHomogen());
          break;
      case FILTER_SCALE3D:
          cmd = wxString::Format("scale_based_filtering_3D %s %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", "scale_tmp", getHomogen());
          break;

      case FILTER_SBAv2D:
          cmd = wxString::Format("scale_based_filtering_2D %s %s %s %d", "voi_tmp.IM0",  "scale_tmp", "voi_tmp2.IM0",  getHomogen());
          break;

      case FILTER_SBAv3D:
          cmd = wxString::Format("scale_based_filtering_3D %s %s %s %d", "voi_tmp.IM0",   "scale_tmp", "voi_tmp2.IM0",  getHomogen());
          break;

      case FILTER_SBAD2D:
          cmd = wxString::Format("b_scale_anisotrop_diffus_2D %s %s %d %d", "voi_tmp.IM0", "voi_tmp2.IM0", getHomogen(), getScaleADIterations());
          break;
      case FILTER_SBAD3D:
          cmd = wxString::Format("b_scale_anisotrop_diffus_3D %s %s %d %d", "voi_tmp.IM0", "voi_tmp2.IM0", getHomogen(), getScaleADIterations());
          break;
      case FILTER_GRADIENT2D:
          cmd = wxString::Format("gradient %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0);
          break;

      case FILTER_GRADIENT3D:
          cmd = wxString::Format("gradient3d %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0);
          break;

      case FILTER_DIST2Dxy:
          cmd = wxString::Format("distance3D -e -p xy -s %s %s", "voi_tmp.IM0", "voi_tmp2.IM0");
          break;

      case FILTER_DIST3D:
          cmd = wxString::Format("distance3D -e -p xyz -s %s %s", "voi_tmp.IM0", "voi_tmp2.IM0");
          break;

	  case FILTER_SOBEL:
		  cmd = wxString::Format("sobel %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0);
		  break;		
	  case FILTER_TOBOG:   
		  cmd = wxString::Format("toboggan %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0);
		  break;
	  case FILTER_MEAN2D:
		  cmd = wxString::Format("median2d %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0);		 
		  break;
	  case FILTER_MEAN3D:  
		  cmd = wxString::Format("median3d %s %s %d", "voi_tmp.IM0", "voi_tmp2.IM0", 0);
		  break;
	  case FILTER_DILATE:
		  cmd = wxString::Format("morph %s %s %d %d", "voi_tmp.IM0", "voi_tmp2.IM0", getMorphN(), 0);
		  iterations = m_MorphIterations;
		  break;
	  case FILTER_ERODE:
		  cmd = wxString::Format("morph %s %s %d %d", "voi_tmp.IM0", "voi_tmp2.IM0", -getMorphN(), 0);
		  iterations = m_MorphIterations;
		  break;
	  case FILTER_BALL_ENH:
	      cmd = wxString::Format("ball_enhance %s %s %d %d", "voi_tmp.IM0", "voi_tmp2.IM0", m_MaxRadius, m_MinRadius);
		  break;
      case FILTER_MAXIMA:
          cmd = wxString::Format("local_maxima %s %s", "voi_tmp.IM0", "voi_tmp2.BIM");
          break;
	  case FILTER_INHOMO1:
	  case FILTER_INHOMO2:
	  case FILTER_INHOMO3:
	      assert(false);
		  break;
      default:
          break;
    }

    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    ProcessManager  p( "filter running...", cmd, true, false, false );
    if (p.getCancel())    return;
    error = p.getStatus();
	while (iterations > 1)
	{
		unlink("voi_tmp.IM0");
		rename("voi_tmp2.IM0", "voi_tmp.IM0");
		p = ProcessManager( "filter running...", cmd, true, false, false );
		if (p.getCancel())    return;
		if (!error)
		    error = p.getStatus();
		iterations--;
	}
#if 0
    if (error != 0) {  wxMessageBox( "Filter Failed." );  return;  }
#endif
 
	//step 3:
    //  load the result.
	m_bFilterDone = true;
    loadFile( "voi_tmp2.IM0" );
}
//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void FilterCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "FilterCanvas::OnChar" << endl;
    wxLogMessage( "FilterCanvas::OnChar" );
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
void FilterCanvas::OnMouseMove ( wxMouseEvent& e ) {

    wxClientDC  dc(this);
    PrepareDC(dc);
    const wxPoint  pos = e.GetPosition();
    //remove translation
    const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
    const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
    //if we are NOT in any mode, then allow the user to move (translate)
    // the image (if a mouse button is down)
    if (e.LeftIsDown()) {
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
void FilterCanvas::OnRightDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(0)) {
        mCavassData->mDisplay = false;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void FilterCanvas::OnRightUp ( wxMouseEvent& e ) {
    if (isLoaded(0)) {
        mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void FilterCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = false;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void FilterCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}
//----------------------------------------------------------------------
void FilterCanvas::OnLeftDown ( wxMouseEvent& e ) {
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
void FilterCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void FilterCanvas::OnPaint ( wxPaintEvent& e ) {
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
void FilterCanvas::paint ( wxDC* dc ) 
{
	int  w, h;
	dc->GetSize( &w, &h );

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
			dc->DrawBitmap( *m_bitmaps[1], m_tx+w/2+10, m_ty );
			if(m_overlay)
			{
				const wxString  s = wxString::Format( "%d/%d",getSliceNo(0)+1,getNoSlices(0));
				dc->DrawText( s, m_tx+w/2+10, m_ty );
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
void FilterCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool FilterCanvas::isLoaded ( const int which ) const {
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

bool FilterCanvas::getOverlay ( void ) const {
    return m_overlay;
}

//----------------------------------------------------------------------
int FilterCanvas::getCenter ( const int which ) const 
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
int FilterCanvas::getMax ( const int which ) const 
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
int FilterCanvas::getMin ( const int which ) const 
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
int FilterCanvas::getNoSlices ( const int which ) const {
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

bool FilterCanvas::getFilterDone (void) const
{
	return m_bFilterDone;
}
//----------------------------------------------------------------------
double FilterCanvas::getScale ( void ) const {
    return m_scale;
}
//----------------------------------------------------------------------
//first displayed slice
int FilterCanvas::getSliceNo ( const int which ) const {
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
int FilterCanvas::getWidth ( const int which ) const 
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
bool FilterCanvas::getInvert ( const int which ) const 
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
void FilterCanvas::setOverlay ( const bool overlay ) { 
    m_overlay = overlay;
}
//----------------------------------------------------------------------
void FilterCanvas::setCenter ( const int which, const int center ) 
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
void FilterCanvas::setInvert ( const int which, const bool invert ) 
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
void FilterCanvas::setScale   ( const double scale )  {  //aka magnification
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
}
//----------------------------------------------------------------------
void FilterCanvas::setSliceNo ( const int which, const int sliceNo ) {
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
void FilterCanvas::setWidth ( const int which, const int width ) 
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
IMPLEMENT_DYNAMIC_CLASS ( FilterCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( FilterCanvas, wxPanel )
    EVT_PAINT(            FilterCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           FilterCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        FilterCanvas::OnLeftDown     )
    EVT_LEFT_UP(          FilterCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      FilterCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        FilterCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       FilterCanvas::OnRightDown    )
    EVT_RIGHT_UP(         FilterCanvas::OnRightUp      )
    EVT_CHAR(             FilterCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
