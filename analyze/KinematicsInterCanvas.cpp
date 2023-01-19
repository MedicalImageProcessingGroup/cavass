/*
  Copyright 1993-2013, 2016 Medical Image Processing Group
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
 * \file:  KinematicsInterCanvas.cpp
 * \brief  KinematicsInterCanvas class implementation
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "KinematicsInterCanvas.h"
//#include  "KinematicsInterAlgorithm.h"
#include "auto_register.h"

using namespace std;
//----------------------------------------------------------------------
KinematicsInterCanvas::KinematicsInterCanvas ( void )  {  puts("KinematicsInterCanvas()");  }
//----------------------------------------------------------------------
KinematicsInterCanvas::KinematicsInterCanvas ( wxWindow* parent, MainFrame* parent_frame,
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
	m_nROIType = 0;
    m_nGradType = 0;
	m_timeInstances = 2;
	m_nRefFrame = 1;

	m_sliceIn = m_sliceOut = NULL;	
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
    lastX = lastY = -1;

	mFileOrDataCount = 0;
//	m_KinematicsInterType = KinematicsInter_GRADIENT2D;
	m_bKinematicsInterDone = false;	

	surf = NULL;
	strDisplay = NULL;
	mListCtrl =NULL;
}
//----------------------------------------------------------------------
KinematicsInterCanvas::~KinematicsInterCanvas ( void ) {
    cout << "KinematicsInterCanvas::~KinematicsInterCanvas" << endl;
    wxLogMessage( "KinematicsInterCanvas::~KinematicsInterCanvas" );
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

	free( surf );
	surf = NULL;

	KinematicsInterRelease();
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::release ( void ) {
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
void KinematicsInterCanvas::loadData ( char* name,
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
void KinematicsInterCanvas::loadFile ( wxArrayString  FileNames ) 
{
    SetCursor( wxCursor(wxCURSOR_WAIT) );    
    wxYield();
    release();
	if ( FileNames.GetCount()==0)
	{
        SetCursor( *wxSTANDARD_CURSOR );
        return;
    }    
	
	init_defaults();

	int num_surf = FileNames.Count();
	int i;
	char *ind;
	char  group[6],elem[6];
	int  error;
	int  nSuccess = 0;
	//VDisplayDialogMessage("Initializing Data Structures.Wait.....");	
	if ((surf=(Surf_Info *)malloc(sizeof(Surf_Info)*(num_surf+1)))==NULL)
	{
		//fprintf(stderr,"Could not build scene data structuree\n");
		wxLogMessage("Could not build scene data structuree!");
		return; //exit(-1);
	}
	for(i=0;i<num_surf;i++) 
	{	
	    const char*fn = FileNames[i].c_str();
		ind=(char *)rindex(fn,'.');
		if (!strcmp(ind,".PLN")) 
		{
			strcpy(surf[i].plan_file,fn);
			surf[i].plan_on=1;
			if (InitPlan(&surf[i])==-1) 
			{
				//VDisplayDialogMessage("Incorrect Plan File");
				wxLogMessage("Incorrect Plan File!");
				num_surf--;i--;
			}
			else
				nSuccess++;
		}
		else 
		{
			strcpy(surf[i].file_name,fn);
			surf[i].plan_on=0;
			if ((surf[i].fp=fopen(surf[i].file_name,"rb"))==NULL) 
			{
				//fprintf(stderr,"Could not open %s\n",surf[i].file_name);				
				//fprintf(stderr,"Ignoring file\n");
				wxLogMessage("Could not open this File, Ignoring this file!");
				num_surf--; i--;
			}
			else 
			{
				error=VReadHeader(surf[i].fp,&surf[i].vh,group,elem);
				//VDecodeError("main","VReadHeader",error,msg);
				if (!error || error==106 || error==107) 
				{
					if (BuildSurfTree(&surf[i])==-1) 
					{
						fprintf(stderr,"Could Not build data structure for %s\n",surf[i].file_name);
						fprintf(stderr,"Ignoring file");
						num_surf--; i--;
					}
					else
						nSuccess++;
				}
				else 
				{
					num_surf--; i--;
				}
			}
		}
	}

	if( nSuccess == 0 )
		return;
	//free(files);
	wxLogMessage("Done Initializing Data Structures.");
	
	wxArrayString  strOutPut;	
	nInstancesNum = 0;
	Register_Inter(surf, &strOutPut, &nInstancesNum, num_surf);
	wxLogMessage("done");

	int  w, h;
	GetSize( &w, &h );

	wxPoint ListPos(30, 30);
	if (mListCtrl)
	{
		delete mListCtrl;
		mListCtrl =NULL;
	}

	mListCtrl = new wxListCtrl(this, -1, ListPos, wxSize(w-100, 360), wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);	

	wxListItem t_list_item;
	mListCtrl->InsertColumn(0, wxString("File Name"), wxLIST_FORMAT_LEFT, 200 );
	t_list_item.m_format = wxLIST_FORMAT_LEFT;
	t_list_item.m_mask = wxLIST_MASK_FORMAT;
	mListCtrl->SetColumn(0, t_list_item);
	mListCtrl->InsertColumn(1, wxString("Object"), wxLIST_FORMAT_LEFT, 40 );
	mListCtrl->SetColumn(1, t_list_item);
	mListCtrl->InsertColumn(2, wxString("Time Instance"), wxLIST_FORMAT_LEFT, 100 );
	mListCtrl->SetColumn(2, t_list_item);
	mListCtrl->InsertColumn(3, wxString("Translation"), wxLIST_FORMAT_LEFT, 200 );
	mListCtrl->SetColumn(3, t_list_item);
	mListCtrl->InsertColumn(4, wxString("Axis of Rotation(Center)"), wxLIST_FORMAT_LEFT, 200 );
	mListCtrl->SetColumn(4, t_list_item);
	mListCtrl->InsertColumn(5, wxString("Axis of Rotation(Direction)"), wxLIST_FORMAT_LEFT, 200 );
	mListCtrl->SetColumn(5, t_list_item);
	mListCtrl->InsertColumn(6, wxString("Angle of Rotation"), wxLIST_FORMAT_LEFT, 100 );
	mListCtrl->SetColumn(6, t_list_item);	

	/* For each SERIES, print the relevant information */	
	if( strOutPut.Count() > 0 )
	{
		int j=0;
		mListCtrl->InsertItem( j, wxString( strOutPut[0] ) );
		mListCtrl->SetItem(j, 1, wxString( strOutPut[1] ) );

		if( strOutPut.Count() > 2 )
		{
			mListCtrl->SetItem(j, 2, wxString( strOutPut[2] ) );
			mListCtrl->SetItem(j, 3, wxString( strOutPut[3] ) );
			mListCtrl->SetItem(j, 4, wxString( strOutPut[4] ) );
			mListCtrl->SetItem(j, 5, wxString( strOutPut[5] ) );
			mListCtrl->SetItem(j, 6, wxString( strOutPut[6] ) );

			for( j=1; j<nInstancesNum-1; j++)
			{
				mListCtrl->InsertItem( j, wxString("") );
				mListCtrl->SetItem(j, 2, wxString( strOutPut[j*5+2] ) );
				mListCtrl->SetItem(j, 3, wxString( strOutPut[j*5+3] ) );
				mListCtrl->SetItem(j, 4, wxString( strOutPut[j*5+4] ) );
				mListCtrl->SetItem(j, 5, wxString( strOutPut[j*5+5] ) );
				mListCtrl->SetItem(j, 6, wxString( strOutPut[j*5+6] ) );
			}
		}
	}
   
	m_bKinematicsInterDone = true;
    SetCursor( *wxSTANDARD_CURSOR );	
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::initLUT ( const int which ) {
    assert( which==0 || which==1 );
    if (!isLoaded(which))    return;
    
	if (which==0) 
		m_sliceIn->initLUT();   
	else
		m_sliceOut->initLUT();
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::CreateDisplayImage(int which)
{	
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::reload ( void ) 
{  
  //  Refresh();
}

//----------------------------------------------------------------------
void KinematicsInterCanvas::mapWindowToData ( int wx, int wy,
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
void KinematicsInterCanvas::OnChar ( wxKeyEvent& e ) {
    //cout << "KinematicsInterCanvas::OnChar" << endl;
    wxLogMessage( "KinematicsInterCanvas::OnChar" );
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
void KinematicsInterCanvas::OnMouseMove ( wxMouseEvent& e ) 
{   
   
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::OnRightDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events	
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::OnRightUp ( wxMouseEvent& e ) 
{
    //if (isLoaded(0)) {
    //    mCavassData->mDisplay = true;  //!mCavassData->mDisplay;
    //    reload();
    //}
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::OnMiddleDown ( wxMouseEvent& e ) 
{
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(1)) 
	{
        mCavassData->mNext->mDisplay = false;  //!mCavassData->mNext->mDisplay;
        reload();
    }	
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::OnMiddleUp ( wxMouseEvent& e ) {
    if (isLoaded(1)) {
        mCavassData->mNext->mDisplay = true;  //!mCavassData->mNext->mDisplay;
        reload();
    }
}

//----------------------------------------------------------------------
void KinematicsInterCanvas::OnLeftDown ( wxMouseEvent& e ) 
{
    cout << "OnLeftDown" << endl;    wxLogMessage("OnLeftDown");
    SetFocus();  //to regain/recapture keypress events
    const wxPoint  pos = e.GetPosition();
	wxClientDC  dc(this);
	PrepareDC(dc);
 
	if( m_bLayout )  //
	{
		const long  wx = dc.DeviceToLogicalX( pos.x ) - m_tx;
		const long  wy = dc.DeviceToLogicalY( pos.y ) - m_ty;
		lastX = wx;
		lastY = wy;
		SetCursor( wxCursor(wxCURSOR_HAND) );
	}
	else
	{

	}

}
//----------------------------------------------------------------------
void KinematicsInterCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    lastX = lastY = -1;
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void KinematicsInterCanvas::OnPaint ( wxPaintEvent& e ) 
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
void KinematicsInterCanvas::paint ( wxDC* dc ) 
{
	int  w, h;	
	dc->GetSize( &w, &h );

	dc->SetTextBackground( *wxBLACK );
	dc->SetTextForeground( wxColour(Yellow) );
	dc->SetPen( wxPen(wxColour(Yellow)) );
	
	/*if (m_backgroundLoaded) 
	{		
		const int  bmW = m_backgroundBitmap.GetWidth();
		const int  bmH = m_backgroundBitmap.GetHeight();
		dc->DrawBitmap( m_backgroundBitmap, (w-bmW)/2, (h-bmH)/2 );
	}*/
}


//----------------------------------------------------------------------
void KinematicsInterCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
}
//----------------------------------------------------------------------
bool KinematicsInterCanvas::isLoaded ( const int which ) const {
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

int KinematicsInterCanvas::SaveProfile ( unsigned char* cFilename )
{	
	SaveInterPlan(surf, (char*)cFilename, m_timeInstances, m_nRefFrame);

	return 1;
}


bool KinematicsInterCanvas::getKinematicsInterDone (void) const
{
	return m_bKinematicsInterDone;
}

//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( KinematicsInterCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( KinematicsInterCanvas, wxPanel )
    EVT_PAINT(            KinematicsInterCanvas::OnPaint        )
    EVT_ERASE_BACKGROUND( MainCanvas::OnEraseBackground )
    EVT_MOTION(           KinematicsInterCanvas::OnMouseMove    )
    EVT_SIZE(             MainCanvas::OnSize         )
    EVT_LEFT_DOWN(        KinematicsInterCanvas::OnLeftDown     )
    EVT_LEFT_UP(          KinematicsInterCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      KinematicsInterCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        KinematicsInterCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       KinematicsInterCanvas::OnRightDown    )
    EVT_RIGHT_UP(         KinematicsInterCanvas::OnRightUp      )
    EVT_CHAR(             KinematicsInterCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
