/*
  Copyright 1993-2013, 2015-2017 Medical Image Processing Group
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
 * \file   RegisterFrame.cpp
 * \brief  RegisterFrame class implementation.
 * \author Andre Souza, Ph.D.
 *	Xiaofen Zheng added nonrigid registration part and combined the
 *		process button with save button after discussed with Jay. Oct 2008
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "CineControls.h"
#include  "RegistrationControls.h"

#include "RegistrationFfdControls.h"

#include  "SetIndexControls.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//int nonrigidID;
//----------------------------------------------------------------------
/** \brief Constructor for RegisterFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
RegisterFrame::RegisterFrame ( ) : MainFrame( 0 )
//    : wxFrame( NULL, -1, _T("CAVASS"), wxPoint(gWhere,gWhere), wxSize(WIDTH,HEIGHT), wxDEFAULT_FRAME_STYLE )
{
    mFileOrDataCount = 0;

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box         = mSetIndex2Box      = NULL;
    mSetIndex1Sizer       = mSetIndex2Sizer    = NULL;
    mRegistrationBox      = NULL;
    mRegistrationSizer    = NULL;

    mCineControls         = NULL;
    mColor1Controls       = mColor2Controls    = NULL;
    mGrayMap1Controls     = mGrayMap2Controls  = NULL;
    mSaveScreenControls   = NULL;
    mSetIndex1Controls    = mSetIndex2Controls = NULL;
    mRegistrationControls = NULL;
    mRegistrationFfdControls = NULL;
    nonrigidID=0;
    
    ::gFrameList.push_back( this );
    gWhere += cWhereIncr;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    
//    m_lastChoice = m_lastChoiceSetting = -1;    
//    m_zsub = m_ysub = NULL;
    
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 0.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -
    //wxSize  canvasSize( GetSize().GetWidth(), GetSize().GetHeight()-dControlsHeight );

    mCanvas = new OverlayCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight);
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();

    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    //------
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    Maximize( true );
    Show();
    Raise();
	mSplitter->SetSashPosition( -dControlsHeight-90 ); //mSplitter->SetSashPosition( -dControlsHeight-75 );
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Move",   2 );
    SetStatusText( "Flash1", 3 );
    SetStatusText( "Flash2", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void RegisterFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();
	mFileMenu->Enable( ID_INPUT, false );

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:Register:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::addButtonBox ( void ) {
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
    wxButton*  setIndex1 = new wxButton( mControlPanel, ID_SET_INDEX1, "SetIndex1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex1 );
    fgs->Add( setIndex1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    wxButton*  setIndex2 = new wxButton( mControlPanel, ID_SET_INDEX2, "SetIndex2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex2 );
    fgs->Add( setIndex2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
    wxButton*  grayMap1 = new wxButton( mControlPanel, ID_GRAYMAP1, "GrayMap1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap1 );
    fgs->Add( grayMap1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "GrayMap2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    wxButton*  color1 = new wxButton( mControlPanel, ID_COLOR1, "Color1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( color1 );
    fgs->Add( color1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    wxButton*  color2 = new wxButton( mControlPanel, ID_COLOR2, "Color2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( color2 );
    fgs->Add( color2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 1
    wxButton*  blink1 = new wxButton( mControlPanel, ID_BLINK1, "Blink1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( blink1 );
    fgs->Add( blink1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    wxButton*  blink2 = new wxButton( mControlPanel, ID_BLINK2, "Blink2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( blink2 );
    fgs->Add( blink2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 1
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 2
    mProcess = new wxButton( mControlPanel, ID_PROCESS, "Process", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mProcess->Enable( false );
    ::setColor( mProcess );
    fgs->Add( mProcess, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 7, col 1
    wxButton*  mRigid = new wxButton( mControlPanel, ID_RIGID, "Affine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mRigid );
    fgs->Add( mRigid, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 7, col 2
    wxButton*  mNonrigid = new wxButton( mControlPanel, ID_NONRIGID, "Deformable", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mNonrigid );
    fgs->Add( mNonrigid, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for RegisterFrame class. */
RegisterFrame::~RegisterFrame ( void ) {
    cout << "RegisterFrame::~RegisterFrame" << endl;
    wxLogMessage( "RegisterFrame::~RegisterFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
 //   if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }
 //   if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }

	/*
	if( nonrigidID == 2 ) {
		delete mRegistrationControls; //	mRegistrationControls=NULL; 
	}
	if( nonrigidID == 1 ) {
		delete mRegistrationFfdControls;//	mRegistrationFfdControls=NULL;
	}
	nonrigidID=0;
	*/

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
/** \brief callback for key presses. */
void RegisterFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "RegisterFrame::OnChar" );
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
        cout << "RegisterFrame::OnChar: " << ::gTimerInterval << endl;
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
void RegisterFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void RegisterFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnHideControls ( wxCommandEvent& e ) {
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight -75 );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item */
void RegisterFrame::OnOpen ( wxCommandEvent& e ) {
	OnRegister (e);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by Register.
 */
void RegisterFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
//    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:Register: ");
    ++mFileOrDataCount;
    if (mFileOrDataCount==1) {
        tmp += fname;
        mSourceName = fname;
    } else if (mFileOrDataCount==2) {
        tmp = mWindowTitle + ", " + fname;
        mTargetName = fname;
    } else {
        assert( 0 );
    }
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "CAVASS:Register:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void RegisterFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:Register: ");
    tmp += name;
    SetTitle( tmp );
    
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnPrevious ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;
    int  sliceB = canvas->getSliceNo(1) - 1;
    if (sliceA < 0)
		sliceA = canvas->getNoSlices(0)-1;
	if (sliceB < 0)
		sliceB = canvas->getNoSlices(1)-1;
    canvas->setSliceNo( 0, sliceA );
    canvas->setSliceNo( 1, sliceB );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)    mSetIndex1Controls->setSliceNo( sliceA );
    if (mSetIndex2Controls!=NULL)    mSetIndex2Controls->setSliceNo( sliceB );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnNext ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;
    int  sliceB = canvas->getSliceNo(1) + 1;
    if (sliceA >= canvas->getNoSlices(0))
		sliceA = 0;
	if (sliceB >= canvas->getNoSlices(1))
		sliceB = 0;
    canvas->setSliceNo( 0, sliceA );
    canvas->setSliceNo( 1, sliceB );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)    mSetIndex1Controls->setSliceNo( sliceA );
    if (mSetIndex2Controls!=NULL)    mSetIndex2Controls->setSliceNo( sliceB );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mSetIndex1Controls!=NULL) return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
#if 0
    mSetIndex1Controls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex1", canvas->getSliceNo(0), canvas->getNoSlices(0), ID_SLICE1_SLIDER,
        ID_SCALE1_SLIDER, canvas->getScale(), ID_MATCH_INDEX1 );
#else
    mSetIndex1Controls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex1", canvas->getSliceNo(0), canvas->getNoSlices(0), ID_SLICE1_SLIDER,
        ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY, ID_MATCH_INDEX1 );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnSetIndex2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
#if 0
    mSetIndex2Controls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex2", canvas->getSliceNo(1), canvas->getNoSlices(1), ID_SLICE2_SLIDER,
        ID_SCALE2_SLIDER, canvas->getScale(), ID_MATCH_INDEX2 );
#else
    mSetIndex2Controls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex2", canvas->getSliceNo(1), canvas->getNoSlices(1), ID_SLICE2_SLIDER,
        ID_SCALE2_SLIDER, canvas->getScale(), ID_OVERLAY, ID_MATCH_INDEX2 );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mGrayMap1Controls!=NULL)  return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG1, ID_CT_SOFT_TISSUE1, ID_CT_BONE1, ID_PET1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mGrayMap2Controls!=NULL)  return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap2", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		ID_CT_LUNG2, ID_CT_SOFT_TISSUE2, ID_CT_BONE2, ID_PET2 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnColor1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mColor1Controls!=NULL)    return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mColor1Controls = new ColorControls( mControlPanel, mBottomSizer, "Color1",
        canvas->getR(0), canvas->getG(0), canvas->getB(0),
        ID_RED1_SLIDER, ID_GREEN1_SLIDER, ID_BLUE1_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnColor2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mColor2Controls!=NULL)    return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mColor2Controls = new ColorControls( mControlPanel, mBottomSizer, "Color2",
        canvas->getR(1), canvas->getG(1), canvas->getB(1),
        ID_RED2_SLIDER, ID_GREEN2_SLIDER, ID_BLUE2_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnRed1Slider ( wxScrollEvent& e ) {
#if 0
    const int       newRedValue = e.GetPosition();
    const double    newRed      = newRedValue / 100.0;
    const wxString  s           = wxString::Format( "%8.2f", newRed );
    mColor1Controls->setRedText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setR( 0, newRed );
    canvas->reload();
#else
    mColor1Controls->update();
    const int       newRedValue = e.GetPosition();
    const double    newRed      = (double)newRedValue / colorSliderMax;
    OverlayCanvas*  canvas      = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setR( 0, newRed );
    canvas->reload();

#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnRed2Slider ( wxScrollEvent& e ) {
#if 0
    const int       newRedValue = e.GetPosition();
    const double    newRed      = newRedValue / 100.0;
    const wxString  s           = wxString::Format( "%8.2f", newRed );
    mColor2Controls->setRedText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setR( 1, newRed );
    canvas->reload();
#else
    mColor2Controls->update();
    const int       newRedValue = e.GetPosition();
    const double    newRed      = (double)newRedValue / colorSliderMax;
    OverlayCanvas*  canvas      = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setR( 1, newRed );
    canvas->reload();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnGreen1Slider ( wxScrollEvent& e ) {
#if 0
    const int       newGreenValue = e.GetPosition();
    const double    newGreen      = newGreenValue / 100.0;
    const wxString  s             = wxString::Format( "%8.2f", newGreen );
    mColor1Controls->setGreenText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setG( 0, newGreen );
    canvas->reload();
#else
    mColor1Controls->update();
    const int       newGreenValue = e.GetPosition();
    const double    newGreen      = (double)newGreenValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setG( 0, newGreen );
    canvas->reload();

#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnGreen2Slider ( wxScrollEvent& e ) {
#if 0
    const int       newGreenValue = e.GetPosition();
    const double    newGreen      = newGreenValue / 100.0;
    const wxString  s             = wxString::Format( "%8.2f", newGreen );
    mColor2Controls->setGreenText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setG( 1, newGreen );
    canvas->reload();
#else
    mColor2Controls->update();
    const int       newGreenValue = e.GetPosition();
    const double    newGreen      = (double)newGreenValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setG( 1, newGreen );
    canvas->reload();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnBlue1Slider ( wxScrollEvent& e ) {
#if 0
    const int       newBlueValue = e.GetPosition();
    const double    newBlue      = newBlueValue / 100.0;
    const wxString  s            = wxString::Format( "%8.2f", newBlue );
    mColor1Controls->setBlueText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setB( 0, newBlue );
    canvas->reload();
#else
    mColor1Controls->update();
    const int       newBlueValue = e.GetPosition();
    const double    newBlue      = (double)newBlueValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setB( 0, newBlue );
    canvas->reload();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnBlue2Slider ( wxScrollEvent& e ) {
#if 0
    const int       newBlueValue = e.GetPosition();
    const double    newBlue      = newBlueValue / 100.0;
    const wxString  s            = wxString::Format( "%8.2f", newBlue );
    mColor2Controls->setBlueText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setB( 1, newBlue );
    canvas->reload();
#else
    mColor2Controls->update();
    const int       newBlueValue = e.GetPosition();
    const double    newBlue      = (double)newBlueValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setB( 1, newBlue );
    canvas->reload();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnMatchIndex1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
	int j=canvas->getSliceNo(0);
	while (j >= canvas->getNoSlices(1))
		j -= canvas->getNoSlices(1);
    canvas->setSliceNo( 1, j );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnMatchIndex2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
	int j=canvas->getSliceNo(1);
	while (j >= canvas->getNoSlices(0))
		j -= canvas->getNoSlices(0);
    canvas->setSliceNo( 0, j );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnBlink1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->mCavassData->mDisplay = !canvas->mCavassData->mDisplay;
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnBlink2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->mCavassData->mNext->mDisplay = !canvas->mCavassData->mNext->mDisplay;
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RegisterFrame::OnCine ( wxCommandEvent& unused ) {
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mCineControls!=NULL)      return;

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*void RegisterFrame::OnReset ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mColor1Controls!=NULL)     { delete mColor1Controls;      mColor1Controls=NULL;     }
    if (mColor2Controls!=NULL)     { delete mColor2Controls;      mColor2Controls=NULL;     }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }
    if (mGrayMap2Controls!=NULL)   { delete mGrayMap2Controls;    mGrayMap2Controls=NULL;   }
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mSetIndex2Controls!=NULL)  { delete mSetIndex2Controls;   mSetIndex2Controls=NULL;  }
//    mSave->Enable( false );
    m_cine_timer->Stop();

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 0;
    canvas->mCavassData->mDisplay = canvas->mCavassData->mNext->mDisplay = true;

    canvas->setSliceNo( 0, 0 );
    canvas->setCenter(  0, canvas->getMax(0)/2 );
    canvas->setWidth(   0, canvas->getMax(0)   );
    canvas->setInvert(  0, false );
    canvas->setR(       0, 1.0 );
    canvas->setG(       0, 0.5 );
    canvas->setB(       0, 0.0 );

    canvas->setSliceNo( 1, 0 );
    canvas->setCenter(  1, canvas->getMax(1)/2 );
    canvas->setWidth(   1, canvas->getMax(1)   );
    canvas->setInvert(  1, false );
    canvas->setR(       1, 0.0 );
    canvas->setG(       1, 0.5 );
    canvas->setB(       1, 1.0 );

    canvas->setScale( 1.0 );

    CavassData*  tmp = new CavassData( mSourceName );
    CavassData&  cd = *(canvas->mCavassData);
    cd.m_data = tmp -> m_data;

    canvas->reload();
	mSplitter->SetSashPosition( -dControlsHeight-75 );
} */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom in button
 *  \todo implement multi resolution sliders
 */
void RegisterFrame::OnZoomIn ( wxCommandEvent& e ) {
#if 0
    ++mZoomLevel;
    if (mZoomLevel==0) {
        const double  before = m_scale->GetFPValue();
        m_scale->ResetFPRange();
        double  after = m_scale->GetFPValue();
        if (before<=after) {
            m_scale->SetFPValue( after );
            mCanvas->setScale( after  );
        } else {
            m_scale->SetFPRange( m_scale->GetFPMin(), before );
            m_scale->SetFPValue(  before );
            mCanvas->setScale( before  );
        }
        while (!mZoomValues.empty())    mZoomValues.pop();
        return;
    }
    if (mZoomLevel<0) {
        mZoomValues.pop();
        double  fpMin = mZoomValues.top().first;
        double  fpV   = m_scale->GetFPValue();
        double  fpMax = mZoomValues.top().second;
        if (fpV<fpMin)    fpMin = fpV;
        if (fpV>fpMax)    fpMax = fpV;
        m_scale->SetFPRange( fpMin, fpMax );
        return;
    }

    int  iMin = m_scale->GetMin();
    int  iV   = m_scale->GetValue();
    int  iMax = m_scale->GetMax();
    if (iMin>=iV || iMax<=iV)    return;

    int  iMinDiff = iV   - iMin;
    int  iMaxDiff = iMax - iV;
    int  iDiff    = iMaxDiff;
    if (iMinDiff<iMaxDiff)    iDiff = iMinDiff;
    iDiff -= 0.10 * iDiff;  //take off 10%
    //map the int values back into fp values
    double  newMin = m_scale->GetFPValue( iV-iDiff );
    double  newMax = m_scale->GetFPValue( iV+iDiff );
    m_scale->SetFPRange( newMin, newMax );
    mZoomValues.push( pair<double,double>(newMin, newMax) );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom out button
 *  \todo implement multi resolution sliders
 */
void RegisterFrame::OnZoomOut ( wxCommandEvent& e ) {
#if 0
    --mZoomLevel;
    if (mZoomLevel==0) {
        const double  before = m_scale->GetFPValue();
        m_scale->ResetFPRange();
        const double  after = m_scale->GetFPValue();
        if (before!=after)    mCanvas->setScale( after );
        while (!mZoomValues.empty())    mZoomValues.pop();
        return;
    }
    if (mZoomLevel>0) {
        mZoomValues.pop();
        double  fpMin = mZoomValues.top().first;
        double  fpV   = m_scale->GetFPValue();
        double  fpMax = mZoomValues.top().second;
        if (fpV<fpMin)    fpMin = fpV;
        if (fpV>fpMax)    fpMax = fpV;
        m_scale->SetFPRange( fpMin, fpMax );
        return;
    }

    int  iV   = m_scale->GetValue();
    int  iMax = m_scale->GetMax();
    iMax += 0.5 * iMax;  //increase

    double  newMax = m_scale->GetFPValue( iMax );
    if (newMax>1.0)    newMax = (int)(newMax+0.5);  //round it
    m_scale->SetFPRange( m_scale->GetFPMin(), newMax );
    mZoomValues.push( pair<double,double>(m_scale->GetFPMin(), newMax) );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void RegisterFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 2 */
void RegisterFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void RegisterFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 2 */
void RegisterFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTLung button for data set 1 */
void RegisterFrame::OnCTLung1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnCTSoftTissue1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnCTBone1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnPET1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnCTLung2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnCTSoftTissue2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnCTBone2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnPET2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
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
void RegisterFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 2 */
void RegisterFrame::OnSlice2Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getSliceNo(1)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 1, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void RegisterFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 2 */
void RegisterFrame::OnScale2Slider ( wxScrollEvent& e ) {
    int     newScaleValue = e.GetPosition();
    double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex2Controls->setScaleText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for histogram slider */
void RegisterFrame::OnHistoSlider ( wxScrollEvent& e ) {
    int     newHisto = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newHisto );
    mRegistrationControls->setHistoText( s );
    
    
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for pyramid slider */
void RegisterFrame::OnPyraSlider ( wxScrollEvent& e ) {
    int     newPyra = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newPyra );
    mRegistrationControls->setPyraText( s );
    
    
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for histogram slider */
void RegisterFrame::OnConvSlider ( wxScrollEvent& e ) {
    int     newConv = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newConv );
    mRegistrationFfdControls->setConvText( s ); 
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for pyramid slider */
void RegisterFrame::OnPyra2Slider ( wxScrollEvent& e ) {
    int     newPyra2 = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newPyra2 );
    mRegistrationFfdControls->setPyra2Text( s );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for histogram slider */
void RegisterFrame::OnStepSlider ( wxScrollEvent& e ) {
    int     newStep = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newStep );
    mRegistrationFfdControls->setStepText( s );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for pyramid slider */
void RegisterFrame::OnSpaceSlider ( wxScrollEvent& e ) {
    int     newSpace = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newSpace );
    mRegistrationFfdControls->setSpaceText( s );
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Overlay checkbox for both data sets */
void RegisterFrame::OnOverlay ( wxCommandEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void RegisterFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 2 */
void RegisterFrame::OnInvert2 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void RegisterFrame::OnCineForward ( wxCommandEvent& e ) {
    if (e.IsChecked()) {
        mForward            = true;
        mForwardBackward    = false;
        mDirectionIsForward = true;
        mCineControls->mForwardBackwardCheck->SetValue( 0 );
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        m_cine_timer->Start( ::gTimerInterval, true );
    } else {
        m_cine_timer->Stop();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward & backward checkbox for both data sets */
void RegisterFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void RegisterFrame::OnCineTimer ( wxTimerEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    const int  slices0 = canvas->getNoSlices(0);
    const int  slices1 = canvas->getNoSlices(1);
    int  s0 = canvas->getSliceNo(0);
    int  s1 = canvas->getSliceNo(1);

    if (mForward) {
        ++s0;
        ++s1;
        if (s0>=slices0 && s1>=slices1) {
            while (s0>0 && s1>0) {
                --s0;  --s1;
            }
        }
    } else {
        if (mDirectionIsForward) {
            ++s0;
            ++s1;
            if (s0>=slices0 && s1>=slices1) {
                while (s0>=slices0 && s1>=slices1) {  --s0;  --s1;  }
                mDirectionIsForward = false;
            }
        } else {
            --s0;
            --s1;
            if (s0<0 && s1<0) {
                while (s0<0 && s1<0) {  ++s0;  ++s1;  }
                mDirectionIsForward = true;
            }
        }
    }
    canvas->setSliceNo( 0, s0 );
    canvas->setSliceNo( 1, s1 );
    canvas->reload();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void RegisterFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}


/** \brief callback for selecting something in the transformation box */
void RegisterFrame::OnNonrigid ( wxCommandEvent& e  ) {
		if (nonrigidID != 1) {
	    	if (nonrigidID == 2) delete mRegistrationControls;
			nonrigidID = 1;
			mRegistrationFfdControls = new RegistrationFfdControls( mControlPanel, mBottomSizer, 
				"Deformable Registration Setting", ID_SIMIL2_COMBOBOX, ID_CONV_SLIDER, ID_PYRA2_SLIDER,ID_STEP_SLIDER,ID_SPACE_SLIDER,ID_PARALLEL2_INDEX,ID_PAR2_INDEX );
			mProcess->Enable( True );
		}
}


void RegisterFrame::OnRigid ( wxCommandEvent& e  ) {
		if (nonrigidID != 2) {
			if (nonrigidID == 1) delete mRegistrationFfdControls;
			nonrigidID = 2;
			mRegistrationControls = new RegistrationControls( mControlPanel, mBottomSizer,
					"Affine Registration Setting",ID_TRANF_COMBOBOX,ID_INTER_COMBOBOX,
					ID_SIMIL_COMBOBOX, ID_HISTO_SLIDER, ID_PYRA_SLIDER,ID_PARALLEL_INDEX,ID_PAR_INDEX );
			mProcess->Enable( True );
		}
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void RegisterFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void RegisterFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void RegisterFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void RegisterFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void RegisterFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void RegisterFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void RegisterFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new OverlayPrint(wxString("CAVASS").c_str(), canvas),
        new OverlayPrint(wxString("CAVASS").c_str(), canvas),
        &printDialogData );
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    wxPreviewFrame*  frame = new wxPreviewFrame( preview, this,
        _T("CAVASS Print Preview"),
        wxPoint(100, 100), wxSize(600, 650) );
    frame->Centre( wxBOTH );
    frame->Initialize();
    frame->Show();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing */
void RegisterFrame::OnPrint ( wxCommandEvent& e ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    OverlayPrint       printout( (const wxChar *)"CAVASS:Register", canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for process */
void RegisterFrame::OnProcess ( wxCommandEvent& e ) {

  wxString s1, s2, s3, s4, s5;
int choice;

if (nonrigidID == 2) {
  enum {SSD, CC, MI, NMI, KS, FK, SAD};
  enum {NN, LINEAR, CUBIC};
  enum {RIGID, SCALE, RIGID_UNIF_SCALE, ANISOTROPIC, SHEAR, FULL_AFFINE,
    TRANSLATE_ONLY};

  SetStatusText( "source: " + mSourceName + ", " + "target: " + mTargetName, 0 );
  SetStatusText("working ...", 1);
  Update();
  
  wxFileDialog  f( this, "Save image file", _T(""), _T("reg-tmp.IM0"),
	  "CAVASS files (*.IM0)|*.IM0",
	  wxSAVE | wxOVERWRITE_PROMPT);
  int  ret = f.ShowModal();
  if (ret == wxID_CANCEL)    return;
  wxArrayString  names;
  f.GetPaths( names );
  if (names.Count()==0)    return;
  wxString  fname, p_fname;
  fname = names[0];
  bool par = mRegistrationControls->getOnPar();
  if (par)
  {
    wxFileDialog  f( this, "Save parameter file", _T(""), fname+".par",
	    "Parameter files (*.par)|*.par",
	    wxSAVE | wxOVERWRITE_PROMPT);
    int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;
    f.GetPaths( names );
	p_fname = names[0];
    if (names.Count()==0)    return;
  }
  else
    p_fname = "affine.par";

  wxString s1, s2, s3, s4, s5;
  choice = mRegistrationControls->getSimilarity ();
  switch (choice) {
    case SSD:
       s1=wxString::Format( "%c",'s');
       break;

    case CC:
       s1=wxString::Format( "%c",'c');
       break;

    case MI:
       s1=wxString::Format( "%c",'m');
       break;

    case NMI:
       s1=wxString::Format( "%c",'n');
       break;

    case KS:
       s1=wxString::Format( "%c",'k');
       break;

    case FK:
       s1=wxString::Format( "%c",'f');
       break;

    case SAD:
       s1=wxString::Format( "%c",'a');
       break;

    default:
       s1=wxString::Format( "%c",'c');
       break;
  }

  choice = mRegistrationControls->getInterpolation ();
  switch (choice) {
    case NN:
       s2=wxString::Format( "%d",0);
       break;

    case LINEAR:
       s2=wxString::Format( "%d",1);
       break;

    case CUBIC:
       s2=wxString::Format( "%d",2);
       break;
    default:
       s2=wxString::Format( "%d",2);
       break;

  } 
  choice = mRegistrationControls->getTransformation ();
  switch (choice) {
    case RIGID:
       s3=wxString::Format( "%d",0);
       break;

    case SCALE:
       s3=wxString::Format( "%d",1);
       break;

    case RIGID_UNIF_SCALE:
       s3=wxString::Format( "%d",5);
       break;

    case ANISOTROPIC:
       s3=wxString::Format( "%d",2);
       break;

    case SHEAR:
       s3=wxString::Format( "%d",3);
       break;

    case FULL_AFFINE:
       s3=wxString::Format( "%d",4);
       break;

    case TRANSLATE_ONLY:
       s3=wxString::Format( "%d",6);
       break;

    default:
       s3=wxString::Format( "%d",0);
       break;
  }
  s4 = wxString::Format( "%d", mRegistrationControls->getHistoLevel() );
  s5 = wxString::Format( "%d", mRegistrationControls->getPyraDepth()  );
 
 bool parallel = mRegistrationControls->getOnParallel ();
 if (parallel) {
      cout << "mpirun -v -s n0 ./bin/paffine "<< s1 <<" "<< mSourceName << " " << mTargetName;
      cout << " "+p_fname+" " << s2 << " " << s3 << " " << s4 << " " << s4 << " " << s5 << endl;

      system(("mpirun -v -s n0 ./bin/paffine " + s1 + " " + mSourceName + " " +
             mTargetName + " "+p_fname+" " + s2 + " " + s3 + " " + s4 + " " + s4 + " " + s5).c_str());

    //construct the command string
    wxString  cmd = "affine t \""+mSourceName+"\" \""+mTargetName+
		"\" \""+fname+"\" "+p_fname+" \""+s2+"\"";
    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    ProcessManager  p( "Registration step 2 running...", cmd );
    if (p.getCancel())    return;
  SetStatusText("done", 1);
	bool par = mRegistrationControls->getOnPar();
	if (!par) { //delete parameter file
		unlink("affine.par" );}

  } else {
    //construct the command string
    wxString  cmd("\"");
	cmd += Preferences::getHome()+"/affine\" \""+s1+"\" \""+mSourceName+"\" \""+mTargetName+"\" \""+fname+"\" \""+p_fname+"\" \""+s2+"\" \""+s3+"\" \""+s4+"\" \""+s4+"\" \""+s5+"\"";
    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    //system( cmd.c_str() );
    ProcessManager  p( "Affine registration running...", cmd);
    if (p.getCancel())    return;
  SetStatusText("done", 1);
	if (!par) { //delete parameter file
		unlink("affine.par" );}// system("rm affine.par");}

  }
/*****************************************
*/

}  //endif transform != NONRIGID

if (nonrigidID == 1) {
	SetStatusText( "source: " + mSourceName + ", " + "target: " + mTargetName, 0 );
	SetStatusText("working ...", 1);
	Update();

	wxFileDialog  f( this, "Save image files", _T(""), _T("reg-tmp.IM0"), "CAVASS files (*.IM0)|*.IM0", wxSAVE | wxOVERWRITE_PROMPT);
	int  ret = f.ShowModal();
	if (ret == wxID_CANCEL)    return; 
	wxArrayString  names;
	f.GetPaths( names );
	if (names.Count()==0)    return;
	wxString  fname;
	fname = names[0];

	s2 = wxString::Format( "%d", mRegistrationFfdControls->getConvLevel() );
	s3 = wxString::Format( "%d", mRegistrationFfdControls->getPyra2Depth() );
	s4 = wxString::Format( "%d", mRegistrationFfdControls->getStepLevel() );
	s5 = wxString::Format( "%d", mRegistrationFfdControls->getSpaceLevel() );
	bool parallel = mRegistrationFfdControls->getOnParallel ();
	if (parallel) {
		cout << "mpirun -v -s n0 ./bin/mpinonrigid "<< mSourceName << " " << mTargetName;
		cout << " " << fname << " "<< s2 << " " << s3 << " " << s4 << " " << s5 << " nonrigid.par"<< endl;
		system(("mpirun -v -s n0 ./bin/mpinonrigid " + mSourceName + " " +
		mTargetName + " " + fname + " " + s2 + " " + s3 + " " + s4 + " " + s5 +" nonrigid.par").c_str());
	}
	else {

	//construct the command string
	bool par = mRegistrationFfdControls->getOnPar();
	if (!par) {
		wxString  cmd("\"");
		cmd += Preferences::getHome()+wxString::Format("/nonrigid\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"", (const char *)mSourceName.c_str(), (const char *)mTargetName.c_str(), (const char *)fname.c_str(), (const char *)s2.c_str(), (const char *)s3.c_str(), (const char *)s4.c_str(), (const char *)s5.c_str() );
		wxLogMessage( "command=%s", (const char *)cmd.c_str() );
		ProcessManager  p( "Deformable registration running...", cmd); //, 0);// 1);
		if (p.getCancel())    return;
		SetStatusText("done", 1);
	}
	else {
		wxString  cmd("\"");
		cmd += Preferences::getHome()+wxString::Format("/nonrigid\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" deformationfield.IM0", (const char *)mSourceName.c_str(), (const char *)mTargetName.c_str(), (const char *)fname.c_str(), (const char *)s2.c_str(), (const char *)s3.c_str(), (const char *)s4.c_str(), (const char *)s5.c_str() );
		wxLogMessage( "command=%s", (const char *)cmd.c_str() );
		ProcessManager  p( "Deformable registration running...", cmd); //, 0);// 1);
		if (p.getCancel())    return;
		SetStatusText("done", 1);
	}
	}
} //endif transform == NONRIGID



}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( RegisterFrame, wxFrame )
BEGIN_EVENT_TABLE       ( RegisterFrame, wxFrame )
  DefineStandardFrameCallbacks
 /* EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_INFORMATION,    MainFrame::OnInformation     )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           RegisterFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    RegisterFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           RegisterFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  RegisterFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          RegisterFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, RegisterFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )*/

  EVT_BUTTON( ID_PREVIOUS,       RegisterFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           RegisterFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     RegisterFrame::OnSetIndex1   )
  EVT_BUTTON( ID_SET_INDEX2,     RegisterFrame::OnSetIndex2   )
  EVT_BUTTON( ID_GRAYMAP1,       RegisterFrame::OnGrayMap1    )
  EVT_BUTTON( ID_GRAYMAP2,       RegisterFrame::OnGrayMap2    )
  EVT_BUTTON( ID_COLOR1,         RegisterFrame::OnColor1      )
  EVT_BUTTON( ID_COLOR2,         RegisterFrame::OnColor2      )
  EVT_BUTTON( ID_MATCH_INDEX1,   RegisterFrame::OnMatchIndex1 )
  EVT_BUTTON( ID_MATCH_INDEX2,   RegisterFrame::OnMatchIndex2 )
  EVT_BUTTON( ID_BLINK1,         RegisterFrame::OnBlink1      )
  EVT_BUTTON( ID_BLINK2,         RegisterFrame::OnBlink2      )
  EVT_BUTTON( ID_CINE,           RegisterFrame::OnCine        )
  EVT_BUTTON( ID_PROCESS,        RegisterFrame::OnProcess     )
  EVT_BUTTON( ID_RIGID,          RegisterFrame::OnRigid       )
  EVT_BUTTON( ID_NONRIGID,           RegisterFrame::OnNonrigid        )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, RegisterFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  RegisterFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG1,          RegisterFrame::OnCTLung1  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE1,   RegisterFrame::OnCTSoftTissue1  )
  EVT_BUTTON( ID_CT_BONE1,          RegisterFrame::OnCTBone1  )
  EVT_BUTTON( ID_PET1,              RegisterFrame::OnPET1     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  RegisterFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  RegisterFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        RegisterFrame::OnInvert1       )
  EVT_COMMAND_SCROLL( ID_RED1_SLIDER,    RegisterFrame::OnRed1Slider    )
  EVT_COMMAND_SCROLL( ID_GREEN1_SLIDER,  RegisterFrame::OnGreen1Slider  )
  EVT_COMMAND_SCROLL( ID_BLUE1_SLIDER,   RegisterFrame::OnBlue1Slider   )

  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, RegisterFrame::OnCenter2Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  RegisterFrame::OnWidth2Slider  )
  EVT_BUTTON( ID_CT_LUNG2,          RegisterFrame::OnCTLung2  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE2,   RegisterFrame::OnCTSoftTissue2  )
  EVT_BUTTON( ID_CT_BONE2,          RegisterFrame::OnCTBone2  )
  EVT_BUTTON( ID_PET2,              RegisterFrame::OnPET2     )
  EVT_COMMAND_SCROLL( ID_SLICE2_SLIDER,  RegisterFrame::OnSlice2Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE2_SLIDER,  RegisterFrame::OnScale2Slider  )
  EVT_CHECKBOX(       ID_INVERT2,        RegisterFrame::OnInvert2       )
  EVT_COMMAND_SCROLL( ID_RED2_SLIDER,    RegisterFrame::OnRed2Slider    )
  EVT_COMMAND_SCROLL( ID_GREEN2_SLIDER,  RegisterFrame::OnGreen2Slider  )
  EVT_COMMAND_SCROLL( ID_BLUE2_SLIDER,   RegisterFrame::OnBlue2Slider   )
  
  EVT_COMMAND_SCROLL( ID_HISTO_SLIDER,  RegisterFrame::OnHistoSlider    )
  EVT_COMMAND_SCROLL( ID_PYRA_SLIDER,   RegisterFrame::OnPyraSlider     )

  EVT_COMMAND_SCROLL( ID_CONV_SLIDER,  RegisterFrame::OnConvSlider    )
  EVT_COMMAND_SCROLL( ID_PYRA2_SLIDER,   RegisterFrame::OnPyra2Slider     )
  EVT_COMMAND_SCROLL( ID_STEP_SLIDER,  RegisterFrame::OnStepSlider    )
  EVT_COMMAND_SCROLL( ID_SPACE_SLIDER,   RegisterFrame::OnSpaceSlider     )


  
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, RegisterFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  RegisterFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  RegisterFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  RegisterFrame::OnUpdateUIScale1Slider  )
  EVT_UPDATE_UI( ID_RED1_SLIDER,    RegisterFrame::OnUpdateUIRed1Slider    )
  EVT_UPDATE_UI( ID_GREEN1_SLIDER,  RegisterFrame::OnUpdateUIGreen1Slider  )
  EVT_UPDATE_UI( ID_BLUE1_SLIDER,   RegisterFrame::OnUpdateUIBlue1Slider   )

  EVT_UPDATE_UI( ID_CENTER2_SLIDER, RegisterFrame::OnUpdateUICenter2Slider )
  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  RegisterFrame::OnUpdateUIWidth2Sglider )
  EVT_UPDATE_UI( ID_SLICE2_SLIDER,  RegisterFrame::OnUpdateUISlice2Slider  )
  EVT_UPDATE_UI( ID_SCALE2_SLIDER,  RegisterFrame::OnUpdateUIScale2Slider  )
  EVT_UPDATE_UI( ID_RED2_SLIDER,    RegisterFrame::OnUpdateUIRed2Slider    )
  EVT_UPDATE_UI( ID_GREEN2_SLIDER,  RegisterFrame::OnUpdateUIGreen2Slider  )
  EVT_UPDATE_UI( ID_BLUE2_SLIDER,   RegisterFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    RegisterFrame::OnUpdateUICineSlider    )
#endif

  EVT_BUTTON( ID_ZOOM_IN,        RegisterFrame::OnZoomIn                 )
  EVT_BUTTON( ID_ZOOM_OUT,       RegisterFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, RegisterFrame::OnBusyTimer )

  EVT_MENU( ID_EXAMPLE,        MainFrame::OnExample      )
  EVT_MENU( ID_VIS_MONTAGE,        MainFrame::OnMontage          )
  EVT_MENU( ID_VIS_OVERLAY,        MainFrame::OnVisOverlay       )
  EVT_MENU( ID_VIS_SURF_VIEW,  MainFrame::OnVisSurfView  )

  EVT_CHAR( RegisterFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   RegisterFrame::OnOverlay )

  EVT_TIMER(          ID_CINE_TIMER,            RegisterFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          RegisterFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, RegisterFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           RegisterFrame::OnCineSlider )
//EVT_END_PROCESS(REGISTRATION_DONE, RegisterFrame::Done)
END_EVENT_TABLE()
//======================================================================
