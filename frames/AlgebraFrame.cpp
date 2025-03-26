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
 * \file   AlgebraFrame.cpp
 * \brief  AlgebraFrame class implementation.
 *         external programs used:
 *
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2007, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "CineControls.h"
#include  "DivThreControls.h"
#include  "GrayMapControls.h"
#include  "SetFilterIndexControls.h"
#include  "OffsetControls.h"


extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for AlgebraFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
AlgebraFrame::AlgebraFrame ( ) 
: MainFrame( 0 ),
//    : wxFrame( NULL, -1, _T("CAVASS"), wxPoint(gWhere,gWhere), wxSize(WIDTH,HEIGHT), wxDEFAULT_FRAME_STYLE )
m_AlgebraType(ALGEBRA_APLUSB)
{
    mFileOrDataCount = 0;
	algebraName[0] = _T("Op: A+B");
	algebraName[1] = _T("Op: A-B");
	algebraName[2] = _T("Op: B-A");
	algebraName[3] = _T("Op: |A-B|");
	algebraName[4] = _T("Op: AxB");
	algebraName[5] = _T("Op: A/B");
	algebraName[6] = _T("Op: B/A");
	algebraName[7] = _T("Op: A>B");
	algebraName[8] = _T("Op: B>A");
	algebraName[9] = _T("Op: Mean");	

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       /*= mSetIndex2Box*/      = NULL;
    mSetIndex1Sizer     /*= mSetIndex2Sizer*/    = NULL;

    mCineControls       = NULL;
    /*mColor1Controls     = mColor2Controls    = NULL;*/
    mGrayMap1Controls   = mGrayMap2Controls  = mGrayMapOutControls = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls  = mSetIndex2Controls = NULL;

	mOffsetControls = NULL;
	mCoefficientControls = NULL;
    mDivThreControls = NULL;    

    ::gFrameList.push_back( this );
    gWhere += cWhereIncr;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    
//    m_lastChoice = m_lastChoiceSetting = -1;    
//    m_zsub = m_ysub = NULL;
    
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -
    //wxSize  canvasSize( GetSize().GetWidth(), GetSize().GetHeight()-dControlsHeight );

    mCanvas = new AlgebraCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
    if (Preferences::getCustomAppearance()) {
        mCanvas->SetBackgroundColour( wxColour(DkBlue) );
        mCanvas->SetForegroundColour( wxColour(Yellow) );
    }
    wxSizer*  topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize( 700, 400 );
    topSizer->Add( mCanvas, 1, wxGROW );

    //bottom: controls - - - - - - - - - - - - - - - - - - - - - - - - -
    mControlPanel = new wxPanel( mSplitter, -1, wxDefaultPosition, wxDefaultSize );
    ::setColor( mControlPanel );
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight ); 
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    //          mainSizer->Add( middleSizer, 1,
    //                          wxGROW | (wxALL & ~(wxTOP | wxBOTTOM)), 10 );
    //          mainSizer->Add( 0, 5, 0, wxGROW ); // spacer in between
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer);

    Maximize( true );
	SetSize( WIDTH, 600 );

    mSplitter->SetSashPosition( -dControlsHeight );
    Show();
    Raise();
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Move",   2 );
    SetStatusText( "", 3 );
    SetStatusText( "", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );

}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void AlgebraFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();
	mFileMenu->Enable( ID_INPUT, false );

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:Algebra:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    //row 1, col 1
    wxButton*  prev = new wxButton( mControlPanel, ID_PREVIOUS, "Previous", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( prev );
    fgs->Add( prev, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
    wxButton*  next = new wxButton( mControlPanel, ID_NEXT, "Next", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( next );
    fgs->Add( next, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
    wxButton*  setIndex1 = new wxButton( mControlPanel, ID_SET_INDEX1, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex1 );
    fgs->Add( setIndex1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	// row 2, col 2
    wxButton*  grayMap1 = new wxButton( mControlPanel, ID_GRAYMAP1, "GrayMap1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap1 );
    fgs->Add( grayMap1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "GrayMap2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 3, col 2
    wxButton*  grayMapOut = new wxButton( mControlPanel, ID_GRAYMAPOUT, "OutGrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMapOut );
    fgs->Add( grayMapOut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
	wxArrayString sa;
	for (int j=0; j<TypeNumber; j++)
		sa.Add(algebraName[j]);
	m_AlgebraTypeBut = new wxComboBox( mControlPanel, ID_ALGEBRA_TYPE, "Op: A+B", wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
    ::setColor( m_AlgebraTypeBut );
    fgs->Add( m_AlgebraTypeBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 1
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 5, col 2
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 1

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for AlgebraFrame class. */
AlgebraFrame::~AlgebraFrame ( void )
{
    cout << "AlgebraFrame::~AlgebraFrame" << endl;
    wxLogMessage( "AlgebraFrame::~AlgebraFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }

    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
            if (mCanvas!=NULL)  {  delete mCanvas;  mCanvas=NULL;  }
            ::gFrameList.erase( i );
            break;
        }
    }
    for (i=::demonsInputList.begin(); i!=::demonsInputList.end(); i++) {
        if (*i==this) {
            ::demonsInputList.erase( i );
            break;
        }
    }

    if (::gFrameList.begin() == ::gFrameList.end()) {
        exit(0);
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \bried remove the controls for algebra parameters */
void AlgebraFrame::hideParametersBox()
{ 
	if( mOffsetControls)
	{
		delete mOffsetControls;        
		mOffsetControls=NULL; 
	}
	if (mCoefficientControls)
	{
		delete mCoefficientControls;
		mCoefficientControls = NULL;
	}
	if(mDivThreControls)
	{
		delete mDivThreControls;
		mDivThreControls = NULL;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void AlgebraFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "AlgebraFrame::OnChar" );
    if (m_cine_timer->IsRunning()) {
        //speed up or slow down the cine timer
        const int  delta = 25;
        switch (e.GetKeyCode()) {
            case '-' :
            case '_' :
            case '<' :
            case ',' :
                ::gTimerInterval += delta;
                if (::gTimerInterval > 2000)    ::gTimerInterval = 2000;
                break;
            case '+' :
            case '=' :
            case '>' :
            case '.' :
                ::gTimerInterval -= delta;
                if (::gTimerInterval < 1)    ::gTimerInterval = delta;
                break;
        }
        cout << "AlgebraFrame::OnChar: " << ::gTimerInterval << endl;
    }

    switch (e.GetKeyCode()) {
    case 'a' :
    case 'A' :
        //++mZoomLevel;
        //if (mZoomLevel>5)    mZoomLevel = 5;
        //::setZoomInColor( m_scale );
        break;
    case 'z' :
    case 'Z' :
        //--mZoomLevel;
        //if (mZoomLevel<0)    mZoomLevel = 0;
        //if (mZoomLevel<1)    ::setColor( m_scale );
        break;
    case '>' :
    case '.' :
        if (!m_cine_timer->IsRunning()) {
            //++mZoomLevel;
            //if (mZoomLevel>5)    mZoomLevel = 5;
            //::setZoomInColor( m_scale );
        }
        break;
    case '<' :
    case ',' :
        if (!m_cine_timer->IsRunning()) {
            //--mZoomLevel;
            //if (mZoomLevel<0)    mZoomLevel = 0;
            //if (mZoomLevel<1)    ::setColor( m_scale );
        }
        break;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void AlgebraFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->m_bitmaps[0]==NULL) {
        wxLogMessage("Nothing to copy.");
        return;
    }
    if (!wxTheClipboard->Open()) {
        wxLogError(_T("Can't open clipboard."));
        return;
    }
    wxLogMessage( _T("Creating wxBitmapDataObject...") );
    wxYield();
    wxTheClipboard->Clear();
    cout << "is supported: " << wxTheClipboard->IsSupported( wxDF_TEXT )
        << endl;
    wxTheClipboard->AddData( new wxTextDataObject("copy image to clipboard") );
    cout << "is supported: " << wxTheClipboard->IsSupported( wxDF_BITMAP )
        << endl;
    if (!wxTheClipboard->AddData(
        new wxBitmapDataObject(*canvas->m_bitmaps[0])) ) {
        wxLogError(_T("Can't copy image to the clipboard."));
    } else {
        wxLogMessage(_T("Image has been put on the clipboard.") );
        wxLogMessage(_T("You can paste it now and look at it.") );
    }
    wxTheClipboard->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for toggling control panel on and off. */
void AlgebraFrame::OnHideControls ( wxCommandEvent& e ) {
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item */
void AlgebraFrame::OnOpen ( wxCommandEvent& e ) {
	OnAlgebra(e);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
int AlgebraFrame::loadFile ( const char* const fname ) 
{
    if (mFileOrDataCount == 0)
	{
		if (fname==NULL || strlen(fname)==0)
		    return ERR_FILENAME;
    	if (!wxFile::Exists(fname)) {
        	wxMessageBox( "File could not be opened.", "File does not exist",
            	wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        	return ERR_OPENFILE;
    	}
    }
    wxString  tmp("CAVASS:Algebra: ");
	if (fname && strlen(fname))
	{
	    _fileHistory.AddFileToHistory( fname );
	    ++mFileOrDataCount;
	    if (mFileOrDataCount==1) 
		{
	        tmp += fname;
	    } 
		else if (mFileOrDataCount==2)
		{
	        tmp = mWindowTitle + ", " + fname;
	    } 
		else 
		{
	        assert( 0 );
	    }
	}
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "CAVASS:Algebra:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    int err = canvas->loadFile( fname );
	if( err > 0 ) return err;

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
	
    Show();
    Raise();

	return 0;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void AlgebraFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:Algebra: ");
    tmp += name;
    SetTitle( tmp );
    
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnPrevious ( wxCommandEvent& unused )
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);

	int  sliceA = canvas->getSliceNo(0) - 1;
	if (sliceA < 0 )
	{
		sliceA = canvas->getNoSlices(
			canvas->getNoSlices(1)>canvas->getNoSlices(0)? 1:0) - 1;
	}
	canvas->setSliceNo( 0, sliceA );
	canvas->setSliceNo( 1, sliceA );

	canvas->RunAlgebra();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnNext ( wxCommandEvent& unused ) 
{
	AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);

	int  sliceA = canvas->getSliceNo(0) + 1;
	if (sliceA>=canvas->getNoSlices(0) && sliceA>=canvas->getNoSlices(1))
		sliceA = 0;
	canvas->setSliceNo( 0, sliceA );
	canvas->setSliceNo( 1, sliceA );

    canvas->RunAlgebra();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnSetIndex1 ( wxCommandEvent& unused ) 
{
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }   
	if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }  
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
	if (mSetIndex2Controls!=NULL)  { delete mSetIndex2Controls;   mSetIndex2Controls=NULL;  }
	if (mGrayMapOutControls!=NULL)  { delete mGrayMapOutControls;   mGrayMapOutControls=NULL;  }
 
   if (mSetIndex1Controls!=NULL) return;

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFilterIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
			     ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY );
}

void AlgebraFrame::OnSetIndex2 ( wxCommandEvent& unused ) 
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
	if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex2Controls!=NULL)  { delete mSetIndex2Controls;   mSetIndex2Controls=NULL;  }
	if (mGrayMapOutControls!=NULL)  { delete mGrayMapOutControls;   mGrayMapOutControls=NULL;  } 

    if (mGrayMap1Controls!=NULL)  return;

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map 1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG1, ID_CT_SOFT_TISSUE1, ID_CT_BONE1, ID_PET1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
	if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
	if (mGrayMapOutControls!=NULL)  { delete mGrayMapOutControls;   mGrayMapOutControls=NULL;  } 

    if (mGrayMap2Controls!=NULL)  return;

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map 2", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		ID_CT_LUNG2, ID_CT_SOFT_TISSUE2, ID_CT_BONE2, ID_PET2 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnGrayMapOut ( wxCommandEvent& unused ) 
{	
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
	if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }

    if (mGrayMapOutControls!=NULL)  return;

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
	if( ! canvas->getAlgebraDone() )
		canvas->RunAlgebra();

    mGrayMapOutControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map Out", canvas->getCenter(2), canvas->getWidth(2),
        canvas->getMax(2), canvas->getInvert(2),
        ID_CENTEROUT_SLIDER, ID_WIDTHOUT_SLIDER, ID_INVERTOUT,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnCine ( wxCommandEvent& unused ) 
{
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }	
	if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
	if (mGrayMapOutControls!=NULL)  { delete mGrayMapOutControls;   mGrayMapOutControls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
	if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }

    if (mCineControls!=NULL)      return;

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnReset ( wxCommandEvent& unused ) 
{
    if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }
    if (mGrayMap2Controls!=NULL)   { delete mGrayMap2Controls;    mGrayMap2Controls=NULL;   }
	if (mGrayMapOutControls!=NULL)  { delete mGrayMapOutControls;   mGrayMapOutControls=NULL;  }
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
	if (mSetIndex2Controls!=NULL)  { delete mSetIndex2Controls;   mSetIndex2Controls=NULL;  }
    if (mOffsetControls!=NULL)      { delete mOffsetControls;       mOffsetControls=NULL;       }
	if (mCoefficientControls!=NULL) { delete mCoefficientControls; mCoefficientControls=NULL; }
    if (mDivThreControls!=NULL)      { delete mDivThreControls;   mDivThreControls=NULL;       }

    m_cine_timer->Stop();

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 0;
    canvas->mCavassData->mDisplay = true;

    canvas->setSliceNo( 0, 0 );
    canvas->setCenter(  0, canvas->getMax(0)/2 );
    canvas->setWidth(   0, canvas->getMax(0)   );
    canvas->setInvert(  0, false );
    canvas->setSliceNo( 1, 0 );
    canvas->setCenter(  1, canvas->getMax(1)/2 );
    canvas->setWidth(   1, canvas->getMax(1)   );
    canvas->setInvert(  1, false );

	if( canvas->getAlgebraDone() )
	{
		canvas->setSliceNo( 2, 0 );
		canvas->setCenter(  2, canvas->getMax(2)/2 );
		canvas->setWidth(   2, canvas->getMax(2)   );
		canvas->setInvert(  2, false );
	}
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlgebraFrame::OnAlgebraSave ( wxCommandEvent &e ) 
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
	wxString exten= canvas->mCavassData->m_vh.scn.num_of_bits==1 &&
		(canvas->mCavassData->mNext==NULL||
		canvas->mCavassData->mNext->m_vh.scn.num_of_bits==1)? ".BIM": ".IM0";
    wxFileDialog  saveDlg( this, _T("Save file"), wxEmptyString,
                           _T("alg-tmp")+exten,
						   _T("CAVASS files (*")+exten+")|*"+exten,
                           wxSAVE|wxOVERWRITE_PROMPT );

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, Algebra %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;

    wxString  cmd("\"");
	cmd += Preferences::getHome();

	if (canvas->mCavassData->mNext == NULL)
	{
	  if (canvas->mCavassData->m_vh.scn.num_of_bits == 1)
	  {
	   switch (m_AlgebraType) {
	    case ALGEBRA_APLUSB:
	    case ALGEBRA_AMINUSB:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" or",
		      (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str() );
		  break;
	    case ALGEBRA_BMINUSA:
	    case ALGEBRA_ABSAMINUSB:
		  cmd += wxString("/invert\" \"")+canvas->mCavassData->m_fname+"\" \""+
			  saveDlg.GetPath()+"\"";
		  break;
	    case ALGEBRA_AXB:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" a-b",
		      (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;

	    default:
		  wxMessageBox( "This Kind of Algebra Operation can not be Done for the BIM images.", "Save Image Fail.", wxOK );
		  return;
		}
	  }
	  else // canvas->mCavassData->m_vh.scn.num_of_bits > 1
	  {
	   switch (m_AlgebraType) {
	    case ALGEBRA_APLUSB:
	    case ALGEBRA_ABSAMINUSB:
	    case ALGEBRA_AGREATB:
	    case ALGEBRA_MEAN:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" x",
		      (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str() );
		  break;
	    case ALGEBRA_AMINUSB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" x+%.0f",
		      (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(), canvas->getOffset() );
		  break;
	    case ALGEBRA_BMINUSA:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" -x+%.0f",
		      (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(), canvas->getOffset() );
		  break;
	    case ALGEBRA_AXB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" %.14fx", (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(),
			  canvas->getCoefficient());
		  break;
	    case ALGEBRA_ADIVIDEB:
		  wxMessageBox(
		      "This Kind of Algebra Operation can not be Done for the image.",
			  "Save Image Fail.", wxOK );
		  return;
	    case ALGEBRA_BDIVIDEA:
	    case ALGEBRA_BGREATA:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" y y-x 0", (canvas->mCavassData)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;

	    default:
		  break;
	   }
	  }
	}
	else if (canvas->mCavassData->m_vh.scn.num_of_bits ==1 &&
			canvas->mCavassData->mNext->m_vh.scn.num_of_bits==1)
	{
	switch (m_AlgebraType) {
	  case ALGEBRA_APLUSB:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" or", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str() );
		  break;
	  case ALGEBRA_AMINUSB:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" a-b", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;
	  case ALGEBRA_BMINUSA:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" a-b", (canvas->mCavassData->mNext)->m_fname, (canvas->mCavassData)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;
	  case ALGEBRA_ABSAMINUSB:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" xor", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;
	  case ALGEBRA_AXB:
		  cmd += wxString::Format("/bin_ops\" \"%s\" \"%s\" \"%s\" and", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;	  

	  default:
		  wxMessageBox( "This Kind of Algebra Operation can not be Done for the BIM images.", "Save Image Fail.", wxOK );		
		  return;
		  
		}
	}
	else
	{
	switch (m_AlgebraType) {
	  case ALGEBRA_APLUSB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" x+y", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str() );
		  break;
	  case ALGEBRA_AMINUSB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" x-y+%.0f", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(), canvas->getOffset() );
		  break;
	  case ALGEBRA_BMINUSA:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" y-x+%.0f", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(), canvas->getOffset() );
		  break;
	  case ALGEBRA_ABSAMINUSB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" x-y x-y y-x", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;
	  case ALGEBRA_AXB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" %.14fxy", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(),
			  canvas->getCoefficient());
		  break;
	  case ALGEBRA_ADIVIDEB:
		  cmd += wxString::Format("/divide\" \"%s\" \"%s\" \"%s\" %d", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str(), canvas->getDivThre() );
		  break;
	  case ALGEBRA_BDIVIDEA:
		  cmd += wxString::Format("/divide\" \"%s\" \"%s\" \"%s\" %d", (canvas->mCavassData->mNext)->m_fname, canvas->mCavassData->m_fname,
			  (const char *)saveDlg.GetPath().c_str(), canvas->getDivThre() );
		  break;
	  case ALGEBRA_AGREATB:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" x x-y 0", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;
	  case ALGEBRA_BGREATA:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" y y-x 0", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;
	  case ALGEBRA_MEAN:
		  cmd += wxString::Format("/algebra\" \"%s\" \"%s\" \"%s\" .5x+.5y", (canvas->mCavassData)->m_fname, (canvas->mCavassData->mNext)->m_fname, 
			  (const char *)saveDlg.GetPath().c_str());
		  break;

	  default:
		  break;
		}
	}

    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    ProcessManager  p( "Algebra running...", cmd );
    p.getStatus();
	
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for  Algebra button */
void AlgebraFrame::OnRunAlgebra ( wxCommandEvent& e ) {

	if (mGrayMapOutControls!=NULL)  { delete mGrayMapOutControls;   mGrayMapOutControls=NULL;  }
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    //canvas->setAlgebra( e.IsChecked() );	
    canvas->RunAlgebra();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for  AlgebraType button */
void AlgebraFrame::OnAlgebraType ( wxCommandEvent& e ) 
{
	for (m_AlgebraType=ALGEBRA_APLUSB;
	        algebraName[m_AlgebraType]!=e.GetString();
			m_AlgebraType=(CavassAlgebraType)((int)m_AlgebraType+1))
		;

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setAlgebraDone( false );

   /* modify the parameters box */
   hideParametersBox();
   switch (m_AlgebraType)
   {   
    case ALGEBRA_AMINUSB:
    case ALGEBRA_BMINUSA:
	 delete mCoefficientControls;
	 mCoefficientControls = NULL;
	 delete mDivThreControls;
	 mDivThreControls = NULL;
     if (mOffsetControls!=NULL || (canvas->mCavassData->m_vh.scn.num_of_bits==1
	                   && (canvas->mCavassData->mNext==NULL ||
					   canvas->mCavassData->mNext->m_vh.scn.num_of_bits==1)))
	   break;
     mOffsetControls = new OffsetControls( mControlPanel, mBottomSizer,"Offset Para", ID_ALGEBRA_OFFSET, (int)canvas->getOffset() );
     break;
	case ALGEBRA_AXB:
	 delete mOffsetControls;
	 mOffsetControls = NULL;
	 delete mDivThreControls;
	 mDivThreControls = NULL;
	 if (mCoefficientControls!=NULL)  return;

     mCoefficientControls = new CoefficientControls( mControlPanel,
	   mBottomSizer, "Coefficient", ID_ALGEBRA_COEFFICIENT,
	   canvas->getCoefficient() );
     break;
    case ALGEBRA_ADIVIDEB:
    case ALGEBRA_BDIVIDEA:
	 delete mOffsetControls;
	 mOffsetControls = NULL;
	 delete mCoefficientControls;
	 mCoefficientControls = NULL;
	 if (mDivThreControls!=NULL)  return;

     mDivThreControls = new DivThreControls( mControlPanel, mBottomSizer,"Threshold Para", ID_ALGEBRA_DIVTHRE, canvas->getDivThre() );
     break;
    default:
	 delete mOffsetControls;
	 mOffsetControls = NULL;
	 delete mCoefficientControls;
	 mCoefficientControls = NULL;
	 delete mDivThreControls;
	 mDivThreControls = NULL;
	 break;
   }
   mBottomSizer->Layout();
   canvas->setAlgebraType(m_AlgebraType);
   canvas->RunAlgebra();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom in button
 *  \todo implement multi resolution sliders
 */
void AlgebraFrame::OnZoomIn ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom out button
 *  \todo implement multi resolution sliders
 */
void AlgebraFrame::OnZoomOut ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void AlgebraFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 2 */
void AlgebraFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
void AlgebraFrame::OnCenterOutSlider ( wxScrollEvent& e ) 
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getCenter(2)==e.GetPosition())    return;  //no change
    canvas->setCenter( 2, e.GetPosition() );
    canvas->initLUT( 2 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void AlgebraFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 2 */
void AlgebraFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for out data set */
void AlgebraFrame::OnWidthOutSlider ( wxScrollEvent& e ) 
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getWidth(2)==e.GetPosition())    return;  //no change
    canvas->setWidth( 2, e.GetPosition() );
    canvas->initLUT( 2 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTLung button for data set 1 */
void AlgebraFrame::OnCTLung1 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getCTLungCenter() );
	canvas->setWidth( 0, Preferences::getCTLungWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTSoftTissue button for data set 1 */
void AlgebraFrame::OnCTSoftTissue1 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 0, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTBone button for data set 1 */
void AlgebraFrame::OnCTBone1 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getCTBoneCenter() );
	canvas->setWidth( 0, Preferences::getCTBoneWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnPET button for data set 1 */
void AlgebraFrame::OnPET1 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getPETCenter() );
	canvas->setWidth( 0, Preferences::getPETWidth() );
	canvas->setInvert( 0, true );
	mGrayMap1Controls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTLung button for data set 2 */
void AlgebraFrame::OnCTLung2 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getCTLungCenter() );
	canvas->setWidth( 1, Preferences::getCTLungWidth() );
	canvas->setInvert( 1, false );
	mGrayMap2Controls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTSoftTissue button for data set 2 */
void AlgebraFrame::OnCTSoftTissue2 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 1, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 1, false );
	mGrayMap2Controls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTBone button for data set 2 */
void AlgebraFrame::OnCTBone2 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getCTBoneCenter() );
	canvas->setWidth( 1, Preferences::getCTBoneWidth() );
	canvas->setInvert( 1, false );
	mGrayMap2Controls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnPET button for data set 2 */
void AlgebraFrame::OnPET2 ( wxCommandEvent& unused ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getPETCenter() );
	canvas->setWidth( 1, Preferences::getPETWidth() );
	canvas->setInvert( 1, true );
	mGrayMap2Controls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void AlgebraFrame::OnSlice1Slider ( wxScrollEvent& e ) 
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
	canvas->setSliceNo( 1, e.GetPosition()-1 );
    canvas->RunAlgebra();
}

void AlgebraFrame::OnSlice2Slider ( wxScrollEvent& e ) 
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    if (canvas->getSliceNo(1)==e.GetPosition())    return;  //no change
    canvas->setSliceNo( 1, e.GetPosition() );	
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void AlgebraFrame::OnScale1Slider ( wxScrollEvent& e ) 
{
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setScale( newScale );
}

void AlgebraFrame::OnScale2Slider ( wxScrollEvent& e ) 
{
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex2Controls->setScaleText( s );
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setScale( newScale );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void AlgebraFrame::OnOverlay ( wxCommandEvent& e ) {
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void AlgebraFrame::OnInvert1 ( wxCommandEvent& e ) 
{
    bool  value = e.IsChecked();
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 2 */
void AlgebraFrame::OnInvert2 ( wxCommandEvent& e ) 
{
    bool  value = e.IsChecked();
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}

/** \brief callback for invert checkbox for data set 2 */
void AlgebraFrame::OnInvertOut ( wxCommandEvent& e ) 
{
    bool  value = e.IsChecked();
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    canvas->setInvert( 2, value );
    canvas->initLUT( 2 );
    canvas->reload();
}

void AlgebraFrame::OnAlgebraOffset(wxCommandEvent& e)
{

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    wxString OffsetStr = e.GetString();
	if( OffsetStr.IsEmpty() )
		return;

    long offset; 
	OffsetStr.ToLong(&offset);
    canvas->setOffset(offset);
	canvas->RunAlgebra();
}

void AlgebraFrame::OnAlgebraCoefficient(wxCommandEvent& e)
{

    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    wxString CoefficientStr = e.GetString();
	if( CoefficientStr.IsEmpty() )
		return;

    double coefficient;
	CoefficientStr.ToDouble(&coefficient);
    canvas->setCoefficient(coefficient);
	canvas->RunAlgebra();
}

void AlgebraFrame::OnAlgebraDivThre(wxCommandEvent& e)
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
    wxString ThreStr = e.GetString();
	if( ThreStr.IsEmpty() )
		return;

    long threshold; 
	ThreStr.ToLong(&threshold);
	if( threshold < 1 )
		threshold = 1;
    canvas->setDivThre(threshold);
	canvas->RunAlgebra();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void AlgebraFrame::OnCineForward ( wxCommandEvent& e ) 
{
    if (e.IsChecked()) 
	{
        mForward            = true;
        mForwardBackward    = false;
        mDirectionIsForward = true;
        mCineControls->mForwardBackwardCheck->SetValue( 0 );
		
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        m_cine_timer->Start( ::gTimerInterval, true );
    } 
	else 
	{
        m_cine_timer->Stop();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward & backward checkbox for both data sets */
void AlgebraFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
    if (e.IsChecked()) {
        mForward            = false;
        mForwardBackward    = true;
        mDirectionIsForward = true;
        mCineControls->mForwardCheck->SetValue( 0 );
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        m_cine_timer->Start(::gTimerInterval, true);
    } else {
        m_cine_timer->Stop();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine timer */
void AlgebraFrame::OnCineTimer ( wxTimerEvent& e ) 
{
    AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);

    const int  slices0 = canvas->getNoSlices(0);
    int  s0 = canvas->getSliceNo(0);
    if (mForward)
	{
        ++s0;
        if (s0>=slices0) 
		{
            while (s0>0) 
			{
                --s0;  
            }
        }
    } 
	else 
	{
        if (mDirectionIsForward) 
		{
            ++s0;

            if (s0>=slices0) 
			{
                while (s0>=slices0) {  --s0; }
                mDirectionIsForward = false;
            }
        } 
		else 
		{
            --s0;

            if (s0<0) {
                while (s0<0) {  ++s0; }
                mDirectionIsForward = true;
            }
        }
    }
    canvas->setSliceNo( 0, s0 );

	 const int  slices1 = canvas->getNoSlices(1);
    int  s1 = canvas->getSliceNo(1);
    if (mForward)
	{
        ++s1;
        if (s1>=slices1) 
		{
            while (s1>0) 
			{
                --s1;  
            }
        }
    } 
	else 
	{
        if (mDirectionIsForward) 
		{
            ++s1;

            if (s1>=slices1) 
			{
                while (s1>=slices1) {  --s1; }
                mDirectionIsForward = false;
            }
        } 
		else 
		{
            --s1;

            if (s1<0) {
                while (s1<0) {  ++s1; }
                mDirectionIsForward = true;
            }
        }
    }
    canvas->setSliceNo( 1, s1 );

	if( canvas->getAlgebraDone() )
	{		
	}

    canvas->RunAlgebra();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}

bool AlgebraFrame::IsImgSizeEqual()
{
	AlgebraCanvas*  canvas = dynamic_cast<AlgebraCanvas*>(mCanvas);
	return canvas->IsImgSizeEqual();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void AlgebraFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AlgebraFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void AlgebraFrame::OnBusyTimer ( wxTimerEvent& e ) {
    cout << "OnBusyTimer" << endl;
    //static bool  flipFlop = true;
    //if (flipFlop)  SetCursor( wxCursor(wxCURSOR_WAIT) );
    //else           SetCursor( *wxSTANDARD_CURSOR );
    //flipFlop = !flipFlop;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AlgebraFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
    if (m_center->GetValue() == mCanvas->getCenter())    return;
    mCanvas->setCenter( m_center->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AlgebraFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
    if (m_width->GetValue() == mCanvas->getWidth())    return;
    mCanvas->setWidth( m_width->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slice slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AlgebraFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AlgebraFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
    //especially (only) need on X11 (w/out GTK) to get slider events
    if (m_scale->GetValue()/100.0 == mCanvas->getScale())    return;
    
    const wxString  s = wxString::Format( "scale: %8.2f",
        m_scale->GetValue()/100.0 );
    m_scaleText->SetLabel( s );
    mCanvas->setScale( m_scale->GetValue()/100.0 );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview */
void AlgebraFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing */
void AlgebraFrame::OnPrint ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( AlgebraFrame, wxFrame )
BEGIN_EVENT_TABLE       ( AlgebraFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_PREVIOUS,       AlgebraFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           AlgebraFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     AlgebraFrame::OnSetIndex1   )
  EVT_BUTTON( ID_SET_INDEX2,     AlgebraFrame::OnSetIndex2   )
  EVT_BUTTON( ID_GRAYMAP1,       AlgebraFrame::OnGrayMap1    )
  EVT_BUTTON( ID_GRAYMAP2,       AlgebraFrame::OnGrayMap2    )
  EVT_BUTTON( ID_GRAYMAPOUT,       AlgebraFrame::OnGrayMapOut    )
  EVT_BUTTON( ID_ALGEBRA,         AlgebraFrame::OnRunAlgebra        )
  EVT_COMBOBOX( ID_ALGEBRA_TYPE,    AlgebraFrame::OnAlgebraType    )
  EVT_BUTTON( ID_CINE,           AlgebraFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          AlgebraFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           AlgebraFrame::OnAlgebraSave  )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, AlgebraFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  AlgebraFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG1,          AlgebraFrame::OnCTLung1  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE1,   AlgebraFrame::OnCTSoftTissue1  )
  EVT_BUTTON( ID_CT_BONE1,          AlgebraFrame::OnCTBone1  )
  EVT_BUTTON( ID_PET1,              AlgebraFrame::OnPET1     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  AlgebraFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SLICE2_SLIDER,  AlgebraFrame::OnSlice2Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  AlgebraFrame::OnScale1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE2_SLIDER,  AlgebraFrame::OnScale2Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        AlgebraFrame::OnInvert1       )

  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, AlgebraFrame::OnCenter2Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  AlgebraFrame::OnWidth2Slider  )
  EVT_BUTTON( ID_CT_LUNG2,          AlgebraFrame::OnCTLung2  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE2,   AlgebraFrame::OnCTSoftTissue2  )
  EVT_BUTTON( ID_CT_BONE2,          AlgebraFrame::OnCTBone2  )
  EVT_BUTTON( ID_PET2,              AlgebraFrame::OnPET2     )
  EVT_COMMAND_SCROLL( ID_CENTEROUT_SLIDER, AlgebraFrame::OnCenterOutSlider )
  EVT_COMMAND_SCROLL( ID_WIDTHOUT_SLIDER,  AlgebraFrame::OnWidthOutSlider  )
  EVT_TEXT( ID_ALGEBRA_OFFSET,            AlgebraFrame::OnAlgebraOffset )
  EVT_TEXT( ID_ALGEBRA_COEFFICIENT, AlgebraFrame::OnAlgebraCoefficient )
  EVT_TEXT( ID_ALGEBRA_DIVTHRE,           AlgebraFrame::OnAlgebraDivThre )  
  EVT_CHECKBOX(       ID_INVERT2,        AlgebraFrame::OnInvert2       )
  EVT_CHECKBOX(       ID_INVERTOUT,        AlgebraFrame::OnInvertOut       )  

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, AlgebraFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  AlgebraFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  AlgebraFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  AlgebraFrame::OnUpdateUIScale1Slider  )

  EVT_UPDATE_UI( ID_CENTER2_SLIDER, AlgebraFrame::OnUpdateUICenter2Slider )
  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  AlgebraFrame::OnUpdateUIWidth2Sglider )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    AlgebraFrame::OnUpdateUICineSlider    )
#endif

  EVT_BUTTON( ID_ZOOM_IN,        AlgebraFrame::OnZoomIn                 )
  EVT_BUTTON( ID_ZOOM_OUT,       AlgebraFrame::OnZoomOut                )

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, AlgebraFrame::OnBusyTimer )
  EVT_CHAR( AlgebraFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   AlgebraFrame::OnOverlay )


  EVT_TIMER(          ID_CINE_TIMER,            AlgebraFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          AlgebraFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, AlgebraFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           AlgebraFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  AlgebraFrame::_fileHistory;
//======================================================================
