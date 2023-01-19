/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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
 * \file   VOIPickSlicesFrame.cpp
 * \brief  VOIPickSlicesFrame class implementation.
 *         external programs used:
 *           
 * \author Xinjian Chen, Ph.D 
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "CineControls.h"
#include  "ScaleADControls.h"
#include  "SetFilterIndexControls.h"
#include  "SetIOIParaControls.h"
//#include  "SetHistZoomControls.h"
#include  "ThresholdFrame.h"
//#include  "SetLayout.h"
#include  "VOIPickSlicesFrame.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for VOIPickSlicesFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
VOIPickSlicesFrame::VOIPickSlicesFrame ( bool maximize, int w, int h )
    : MainFrame( 0 )
{
    mFileOrDataCount = 0;	
	
    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       = NULL;
    mCineControls       = NULL;
    
    mGrayMap1Controls   = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls   = NULL;  
	
	mSetSH0MIPControls = NULL;

	mSH0Type = NULL;   
	
    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    
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

    mCanvas = new VOIPickSlicesCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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

    //addButtonBox();
    //          mainSizer->Add( middleSizer, 1,
    //                          wxGROW | (wxALL & ~(wxTOP | wxBOTTOM)), 10 );
    //          mainSizer->Add( 0, 5, 0, wxGROW ); // spacer in between
//	mLayoutControls = new LayoutControls( mControlPanel, mBottomSizer, "Layout", ID_CHECKBOX_LAYOUT );

    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );

    mSplitter->SetSashPosition( -dControlsHeight );
    Show();
    Raise();
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "", 2 );  //was LOC/Size
    SetStatusText( "", 3 );  //was ACCEPT
    SetStatusText( "", 4 );  //was DONE

    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void VOIPickSlicesFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:VOIPickSlices:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const static char* SH0TypeTable[] = 
{
	"SH0:NONE",
	"SH0:MIP",
	"SH0:CT BONE"
};

void VOIPickSlicesFrame::addButtonBox ( void ) {
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
    //row 2, col 2
    wxButton*  grayMap1 = new wxButton( mControlPanel, ID_GRAYMAP1, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap1 );
    fgs->Add( grayMap1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1    
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2	
	mSH0Type = new wxComboBox( mControlPanel, ID_SH0TYPE, "Select SH0 Type", wxDefaultPosition, wxSize(buttonWidth,buttonHeight), 0, NULL, wxCB_READONLY );
	::setColor( mSH0Type );
	for (int i=0; i < 3; i++) 
	{
		mSH0Type->Append( SH0TypeTable[i] );
	}
	mSH0Type->SetSelection( 0 );
    fgs->Add( mSH0Type, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   
	 //row 4, col 1
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for VOIPickSlicesFrame class. */
VOIPickSlicesFrame::~VOIPickSlicesFrame ( void ) {
    cout << "VOIPickSlicesFrame::~VOIPickSlicesFrame" << endl;
    wxLogMessage( "VOIPickSlicesFrame::~VOIPickSlicesFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }
    //if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
    //if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }

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
/** \brief This method should be called whenever one wishes to create a
 *  new VOIPickSlicesFrame.  It first searches the input file history for an
 *  acceptable input file.  If one is found, it is used.  If none is found
 *  then the user is prompted for an input file.
 *  \param parentFrame is an optional (it may also be NULL) parent frame
 *      which will be closed if we are in single frame mode.
 *      (Furthermore, if the parent is maximized (or not maximized),
 *      the child correspondingly will also be maximized (or not).)
 *  \param useHistory is an optional (default is true) flag to indicate
 *      whether or not the input history should be used to obtain the most
 *      recent file to open.  If false, the history won't be used and a
 *      regular file open dialog box will be presented to the user.  This
 *      can be used by the traditional Open menu item.
 */
void VOIPickSlicesFrame::createVOIPickSlicesFrame ( wxFrame* parentFrame,
                                                    bool useHistory )
{
    wxString  filename;
    if (useHistory) {
        //search the input history for an acceptable input file
        const int  n = ::gInputFileHistory.GetNoHistoryFiles();
        for (int i=0; i<n; i++) {
            wxString  s = ::gInputFileHistory.GetHistoryFile( i );
            if (match(s) && ::gInputFileHistory.IsSelected( i )) {
                filename = s;
                break;
            }
        }
		//check the result
		if (n>0) {  //at least one file in history?
            if (filename.length()==0) {  //nothing suitable found?
			} else if (!wxFile::Exists(filename)) {
                //alert the user that the file no longer exists
                wxString  tmp = wxString::Format( "File %s could not be opened.",
                    (const char *)filename.c_str() );
                wxMessageBox( tmp, "File does not exist",
                    wxOK | wxCENTER | wxICON_EXCLAMATION );
			}
        }
    }

    //did we find an acceptable input file in the input history?
    if (filename.Length()==0 || !wxFile::Exists(filename)) {
        //nothing acceptable so display a dialog which allows the user
        // to choose an input file.
        filename = wxFileSelector( _T("Select image file"), _T(""),
            _T(""), _T(""),
            "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0",
            wxFILE_MUST_EXIST );
    }

    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    //display an example frame using the specified file as input
    VOIPickSlicesFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new VOIPickSlicesFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new VOIPickSlicesFrame();
    }
    frame->loadFile( filename.c_str() );

    //This is a little different than usual.
    // This frame has dynamic controls (i.e., the initial control
    // values/setting are dependent upon input file values).  Usually,
    // the controls are static so they can be created in the constructor.
    // Here we delay the creation of the controls until input has been read.
    frame->addButtonBox();
    frame->mBottomSizer->Layout();
    frame->mControlPanel->Refresh();

    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This function determines if the given filename is of a type
 *  that can be read by this module/app.
 *
 *  Supported file extensions: BIM and IM0.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool VOIPickSlicesFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM", fn, false ))    return true;
    if (wxMatchWild( "*.IM0", fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void VOIPickSlicesFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "VOIPickSlicesFrame::OnChar" );
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
        cout << "VOIPickSlicesFrame::OnChar: " << ::gTimerInterval << endl;
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
void VOIPickSlicesFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void VOIPickSlicesFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
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
void VOIPickSlicesFrame::OnHideControls ( wxCommandEvent& e ) {
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
void VOIPickSlicesFrame::OnOpen ( wxCommandEvent& e ) {
	createVOIPickSlicesFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void VOIPickSlicesFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "VOIPickSlicesFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "VOIPickSlicesFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnPPScopsVOIPickSlices( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void VOIPickSlicesFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:VOIPickSlices: ");
    ++mFileOrDataCount;
    if (mFileOrDataCount==1) {
        tmp += fname;
    } else if (mFileOrDataCount==2) {
        tmp = mWindowTitle + ", " + fname;
    } else {
        assert( 0 );
    }
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "CAVASS:VOIPickSlices:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
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
void VOIPickSlicesFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:VOIPickSlices: ");
    tmp += name;
    SetTitle( tmp );
    
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIPickSlicesFrame::OnPrevious ( wxCommandEvent& unused ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;

    if (sliceA<0 )
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIPickSlicesFrame::OnNext ( wxCommandEvent& unused ) 
{
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;

    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIPickSlicesFrame::OnSetIndex1 ( wxCommandEvent& unused ) 
{
	 if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
	if (mSetSH0MIPControls!=NULL)   { delete mSetSH0MIPControls;    mSetSH0MIPControls=NULL;   }    
	
   if (mSetIndex1Controls!=NULL) return;

    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFilterIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
		     ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIPickSlicesFrame::OnGrayMap1 ( wxCommandEvent& unused )
{
	 if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
//    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
	if (mSetSH0MIPControls!=NULL)   { delete mSetSH0MIPControls;    mSetSH0MIPControls=NULL;   }    

    if (mGrayMap1Controls!=NULL)  return;

    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIPickSlicesFrame::OnCine ( wxCommandEvent& unused )
{
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
	
	if (mSetSH0MIPControls!=NULL)   { delete mSetSH0MIPControls;    mSetSH0MIPControls=NULL;   }    

    if (mCineControls!=NULL)      return;

//	canvas->setVOIPickSlicesDone(false);

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VOIPickSlicesFrame::OnSH0Type ( wxCommandEvent& e ) 
{
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
 //   canvas->setROIStatisticalDone( false );

	int nWhich = mSH0Type->GetSelection();  

	if( nWhich == 1 ) // max Intensity projection
	{
		if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
		if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
		if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
		if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
		
		if (mSetSH0MIPControls!=NULL)  return;

		VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);

		mSetSH0MIPControls = new SetSH0MIPControls( mControlPanel, mBottomSizer,
			"MIP Para",canvas->getMIPMinThre(), canvas->getMax(0),
			ID_SH0_MINTHRE, canvas->getMIPMaxThre(),
			ID_SH0_MAXTHRE );

	}
	else
	{
		if (mSetSH0MIPControls!=NULL)      { delete mSetSH0MIPControls;       mSetSH0MIPControls=NULL;      }

	}
 
	canvas->setSH0Type(nWhich);
   
}

//-------------------------------------------------------------------------------------------
void VOIPickSlicesFrame::OnReset ( wxCommandEvent& unused ) 
{
    if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
	if (mSetSH0MIPControls!=NULL)   { delete mSetSH0MIPControls;    mSetSH0MIPControls=NULL;   }    
	

    m_cine_timer->Stop();

    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 40;
    canvas->mCavassData->mDisplay = true;
	//canvas->mCavassData->mNext->mDisplay = true;

    canvas->setVOIPickSlicesDone(false);   

    canvas->setSliceNo( 0, 0 );
    canvas->setCenter(  0, canvas->getMax(0)/2 );
    canvas->setWidth(   0, canvas->getMax(0)   );
    canvas->setInvert(  0, false );
 //   canvas->setScale( 1.0 );

//	canvas->RemoveProfileInfo();   

    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIPickSlicesFrame::OnVOIPickSlicesSave ( wxCommandEvent &e ) 
{
	VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
	wxString exten=canvas->mCavassData->m_vh.scn.num_of_bits==1? ".BIM":".IM0";
    wxFileDialog  saveDlg( this, _T("Pick Slices Save"), wxEmptyString,
                           _T("pckslc-tmp")+exten,
						   _T("CAVASS files (*")+exten+")|*"+exten,
                           wxSAVE|wxOVERWRITE_PROMPT );

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, VOIPickSlices %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;

    canvas->SaveProfile((unsigned char*)(const char *)saveDlg.GetPath().c_str());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIPickSlicesFrame::OnMIPMinThreSlider ( wxScrollEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);    
    canvas->setMIPMinThre( e.GetPosition() );    
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIPickSlicesFrame::OnMIPMaxThreSlider ( wxScrollEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);    
    canvas->setMIPMaxThre( e.GetPosition() );    
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIPickSlicesFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIPickSlicesFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIPickSlicesFrame::OnCTLung ( wxCommandEvent& unused ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTLungCenter() &&
			canvas->getWidth(0)==Preferences::getCTLungWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTLungCenter() );
	canvas->setWidth( 0, Preferences::getCTLungWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIPickSlicesFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTSoftTissueCenter() &&
			canvas->getWidth(0)==Preferences::getCTSoftTissueWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 0, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIPickSlicesFrame::OnCTBone ( wxCommandEvent& unused ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTBoneCenter() &&
			canvas->getWidth(0)==Preferences::getCTBoneWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTBoneCenter() );
	canvas->setWidth( 0, Preferences::getCTBoneWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIPickSlicesFrame::OnPET ( wxCommandEvent& unused ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getPETCenter() &&
			canvas->getWidth(0)==Preferences::getPETWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getPETCenter() );
	canvas->setWidth( 0, Preferences::getPETWidth() );
	canvas->setInvert( 0, true );
	mGrayMap1Controls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void VOIPickSlicesFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void VOIPickSlicesFrame::OnScale1Slider ( wxScrollEvent& e ) 
{
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void VOIPickSlicesFrame::OnOverlay ( wxCommandEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void VOIPickSlicesFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void VOIPickSlicesFrame::OnCineForward ( wxCommandEvent& e ) {
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
void VOIPickSlicesFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void VOIPickSlicesFrame::OnCineTimer ( wxTimerEvent& e ) {
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    const int  slices0 = canvas->getNoSlices(0);

    int  s0 = canvas->getSliceNo(0);


    if (mForward) {
        ++s0;

        if (s0>=slices0) {
            while (s0>0) {
                --s0;  
            }
        }
    } else {
        if (mDirectionIsForward) {
            ++s0;

            if (s0>=slices0) {
                while (s0>=slices0) {  --s0; }
                mDirectionIsForward = false;
            }
        } else {
            --s0;

            if (s0<0) {
                while (s0<0) {  ++s0; }
                mDirectionIsForward = true;
            }
        }
    }
    canvas->setSliceNo( 0, s0 );

    canvas->reload();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void VOIPickSlicesFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void VOIPickSlicesFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void VOIPickSlicesFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void VOIPickSlicesFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void VOIPickSlicesFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void VOIPickSlicesFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void VOIPickSlicesFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void VOIPickSlicesFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new VOIPickSlicesPrint("CAVASS", canvas),
        new VOIPickSlicesPrint("CAVASS", canvas),
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
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing */
void VOIPickSlicesFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    VOIPickSlicesCanvas*  canvas = dynamic_cast<VOIPickSlicesCanvas*>(mCanvas);
    VOIPickSlicesPrint       printout( _T("CAVASS:VOIPickSlices"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( VOIPickSlicesFrame, wxFrame )
BEGIN_EVENT_TABLE       ( VOIPickSlicesFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
#if 0
  EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           VOIPickSlicesFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    VOIPickSlicesFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           VOIPickSlicesFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  VOIPickSlicesFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          VOIPickSlicesFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, VOIPickSlicesFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )

  EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile )
#endif
  EVT_BUTTON( ID_PREVIOUS,       VOIPickSlicesFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           VOIPickSlicesFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     VOIPickSlicesFrame::OnSetIndex1   )

  EVT_BUTTON( ID_GRAYMAP1,       VOIPickSlicesFrame::OnGrayMap1    )
 EVT_BUTTON( ID_CINE,           VOIPickSlicesFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          VOIPickSlicesFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           VOIPickSlicesFrame::OnVOIPickSlicesSave  )

  EVT_COMMAND_SCROLL( ID_SH0_MINTHRE, VOIPickSlicesFrame::OnMIPMinThreSlider )
  EVT_COMMAND_SCROLL( ID_SH0_MAXTHRE,   VOIPickSlicesFrame::OnMIPMaxThreSlider )
  
  
  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, VOIPickSlicesFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  VOIPickSlicesFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG,          VOIPickSlicesFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   VOIPickSlicesFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          VOIPickSlicesFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              VOIPickSlicesFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  VOIPickSlicesFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  VOIPickSlicesFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        VOIPickSlicesFrame::OnInvert1       )
  EVT_COMBOBOX( ID_SH0TYPE,    VOIPickSlicesFrame::OnSH0Type    )  
//  EVT_CHECKBOX(       ID_CHECKBOX_LAYOUT,       VOIPickSlicesFrame::OnLayout       )
//   EVT_COMMAND_SCROLL( ID_RED1_SLIDER,    VOIPickSlicesFrame::OnRed1Slider    )
//   EVT_COMMAND_SCROLL( ID_GREEN1_SLIDER,  VOIPickSlicesFrame::OnGreen1Slider  )
//   EVT_COMMAND_SCROLL( ID_BLUE1_SLIDER,   VOIPickSlicesFrame::OnBlue1Slider   )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, VOIPickSlicesFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  VOIPickSlicesFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  VOIPickSlicesFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  VOIPickSlicesFrame::OnUpdateUIScale1Slider  )
//   EVT_UPDATE_UI( ID_RED1_SLIDER,    VOIPickSlicesFrame::OnUpdateUIRed1Slider    )
//   EVT_UPDATE_UI( ID_GREEN1_SLIDER,  VOIPickSlicesFrame::OnUpdateUIGreen1Slider  )
//   EVT_UPDATE_UI( ID_BLUE1_SLIDER,   VOIPickSlicesFrame::OnUpdateUIBlue1Slider   )

//  EVT_UPDATE_UI( ID_CENTER2_SLIDER, VOIPickSlicesFrame::OnUpdateUICenter2Slider )
//  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  VOIPickSlicesFrame::OnUpdateUIWidth2Sglider )
//   EVT_UPDATE_UI( ID_SLICE2_SLIDER,  VOIPickSlicesFrame::OnUpdateUISlice2Slider  )
//   EVT_UPDATE_UI( ID_SCALE2_SLIDER,  VOIPickSlicesFrame::OnUpdateUIScale2Slider  )
//   EVT_UPDATE_UI( ID_RED2_SLIDER,    VOIPickSlicesFrame::OnUpdateUIRed2Slider    )
//   EVT_UPDATE_UI( ID_GREEN2_SLIDER,  VOIPickSlicesFrame::OnUpdateUIGreen2Slider  )
//   EVT_UPDATE_UI( ID_BLUE2_SLIDER,   VOIPickSlicesFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    VOIPickSlicesFrame::OnUpdateUICineSlider    )
#endif

//  EVT_BUTTON( ID_ZOOM_IN,        VOIPickSlicesFrame::OnZoomIn                 )
//  EVT_BUTTON( ID_ZOOM_OUT,       VOIPickSlicesFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, VOIPickSlicesFrame::OnBusyTimer )
  EVT_CHAR( VOIPickSlicesFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   VOIPickSlicesFrame::OnOverlay )


  EVT_TIMER(          ID_CINE_TIMER,            VOIPickSlicesFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          VOIPickSlicesFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, VOIPickSlicesFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           VOIPickSlicesFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  VOIPickSlicesFrame::_fileHistory;
//======================================================================
