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
 * \file   VOIROIFrame.cpp
 * \brief  VOIROIFrame class implementation.
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
#include  "SetROIParaControls.h"
#include  "VOIROIFrame.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for VOIROIFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
VOIROIFrame::VOIROIFrame ( bool maximize, int w, int h ): MainFrame( 0 )
{
    mFileOrDataCount = 0;	
	
    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       /*= mSetIndex2Box*/      = NULL;
    mSetIndex1Sizer     /*= mSetIndex2Sizer*/    = NULL;

    mCineControls       = NULL;
    mGrayMap1Controls   = mGrayMap2Controls  = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls  /*= mSetIndex2Controls*/ = NULL;  
	mSetROIParaControls = NULL;

	mWhichROI = 0; 
    mROIName = NULL;   
	mWhichGrad = 0; 
    mGradName = NULL;   

    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;

    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -
    //wxSize  canvasSize( GetSize().GetWidth(), GetSize().GetHeight()-dControlsHeight );

    mCanvas = new VOIROICanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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

    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Loc/Size",   2 );
    SetStatusText( "ACCEPT", 3 );
    SetStatusText( "DONE", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight-30 );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void VOIROIFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:VOIROI:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VOIROIFrame::addButtonBox ( void ) {
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
	wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

	//row 4, col 1    
	wxButton*  VoiPara1 = new wxButton( mControlPanel, ID_VOIPARA, "SetVOIPara", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( VoiPara1 );
    fgs->Add( VoiPara1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    
	//row 4, col 2
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "GrayMap_ROI", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    
   
	 //row 5, col 1
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for VOIROIFrame class. */
VOIROIFrame::~VOIROIFrame ( void ) {
    cout << "VOIROIFrame::~VOIROIFrame" << endl;
    wxLogMessage( "VOIROIFrame::~VOIROIFrame" );

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
 *  new VOIROIFrame.  It first searches the input file history for an
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
void VOIROIFrame::createVOIROIFrame ( wxFrame* parentFrame, bool useHistory )
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
    VOIROIFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new VOIROIFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new VOIROIFrame();
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
bool VOIROIFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM", fn, false ))    return true;
    if (wxMatchWild( "*.IM0", fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void VOIROIFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "VOIROIFrame::OnChar" );
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
        cout << "VOIROIFrame::OnChar: " << ::gTimerInterval << endl;
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
void VOIROIFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void VOIROIFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
void VOIROIFrame::OnHideControls ( wxCommandEvent& e ) {
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
void VOIROIFrame::OnOpen ( wxCommandEvent& e ) {
	createVOIROIFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void VOIROIFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "VOIROIFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "VOIROIFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnPPScopsVOIROI( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void VOIROIFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:VOIROI: ");
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
            tmp = wxString::Format( "CAVASS:VOIROI:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
void VOIROIFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:VOIROI: ");
    tmp += name;
    SetTitle( tmp );
    
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnPrevious ( wxCommandEvent& unused ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;

    if (sliceA<0 )
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnNext ( wxCommandEvent& unused ) 
{
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;

    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );
	//canvas->setVOIROIDone(false);

    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
	if (mSetROIParaControls!=NULL){ delete mSetROIParaControls; mSetROIParaControls=NULL;}

 
   if (mSetIndex1Controls!=NULL) return;

    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    mSetIndex1Controls = new SetFilterIndexControls( mControlPanel, mBottomSizer,
        "SetIndex1", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
				     ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
	if (mSetROIParaControls!=NULL){ delete mSetROIParaControls; mSetROIParaControls=NULL;}


    if (mGrayMap1Controls!=NULL)  return;

    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnSetVOIPara ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
 

    if (mSetROIParaControls!=NULL)  return;

    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);

	int vol_slices;
	if (canvas->mCavassData->m_vh.scn.dimension == 4)
	{
		vol_slices = 0;
		for (int v=0; v<canvas->mCavassData->m_vh.scn.num_of_subscenes[0]; v++)
		  if (canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v] > vol_slices)
			  vol_slices = canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
	}
	else
		vol_slices = canvas->getNoSlices(0);

    mSetROIParaControls = new SetROIParaControls( mControlPanel, mBottomSizer,
		"ROI Para", canvas->getLocX(), canvas->mCavassData->m_xSize-1, ID_LOCX,
        canvas->getLocY(), canvas->mCavassData->m_ySize-1, ID_LOCY,
        canvas->getWinWidth(), canvas->mCavassData->m_xSize, ID_WINWIDTH,
        canvas->getWinHeight(), canvas->mCavassData->m_ySize, ID_WINHEIGHT,
        canvas->getStartSlice(), vol_slices, ID_STARTSLICE,
        canvas->getEndSlice(), ID_ENDSLICE,
        canvas->getStartVolume(), canvas->mCavassData->m_tSize, ID_STARTVOLUME,
	    canvas->getEndVolume(), ID_ENDVOLUME );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::updateROITablets ( void )
{
	if (mSetROIParaControls)
	{
		delete mSetROIParaControls;
		mSetROIParaControls = NULL;
		wxCommandEvent e;
		OnSetVOIPara( e );
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
	if (mSetROIParaControls!=NULL){ delete mSetROIParaControls; mSetROIParaControls=NULL;}


    if (mGrayMap2Controls!=NULL)  return;

    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap2", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnCine ( wxCommandEvent& unused ) {
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }    
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }   
	if (mSetROIParaControls!=NULL){ delete mSetROIParaControls; mSetROIParaControls=NULL;}

    if (mCineControls!=NULL)      return;

//	canvas->setVOIROIDone(false);

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnReset ( wxCommandEvent& unused ) 
{
    m_cine_timer->Stop();

    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 40;
    canvas->mCavassData->mDisplay = true;

    canvas->setVOIROIDone(false);   

	canvas->setLocX( 0 );
	canvas->setWinWidth( canvas->mCavassData->m_xSize );
	canvas->setLocY( 0 );
	canvas->setWinHeight( canvas->mCavassData->m_ySize );
	canvas->setStartSlice( 1 );
	int vol_slices;
	if (canvas->mCavassData->m_vh.scn.dimension == 4)
	{
		vol_slices = 0;
		for (int v=0; v<canvas->mCavassData->m_vh.scn.num_of_subscenes[0]; v++)
		  if (canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v] > vol_slices)
			  vol_slices = canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
	}
	else
		vol_slices = canvas->getNoSlices(0);
	canvas->setEndSlice( vol_slices );
	canvas->setStartVolume( 1 );
	canvas->setEndVolume( canvas->mCavassData->m_tSize );
	updateROITablets();
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIROIFrame::OnVOIROISave ( wxCommandEvent &e ) 
{
	wxString pattern, default_name;
	if (mCanvas->mCavassData->m_vh.scn.num_of_bits > 1)
	{
		pattern = wxString("CAVASS files (*.IM0)|*.IM0");
		default_name = wxString("voi-tmp.IM0");
	}
	else
	{
		pattern = wxString("CAVASS files (*.BIM)|*.BIM");
		default_name = wxString("voi-tmp.BIM");
    }
	wxFileDialog  saveDlg( this, _T("VOI_ROI Save"), wxEmptyString,
                           default_name, pattern,
                           wxSAVE|wxOVERWRITE_PROMPT );

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, VOIROI %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;
	
	VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);	
    canvas->SaveProfile((const char *)saveDlg.GetPath().c_str());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnStartTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j;
	if (sscanf((const char *)e.GetString().c_str(), "%d", &j)==1 && j>0 &&
		  canvas->mCavassData->sliceOfVolume(j-1)<canvas->mCavassData->m_zSize)
	{
	    int vol=canvas->mCavassData->volumeOfSlice(canvas->getSliceNo( 0 ));
		canvas->setStartSlice( j );
		canvas->setSliceNo( 0, canvas->mCavassData->sliceIndex(vol, j-1) );
		canvas->reload();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnEndTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j;
	if (sscanf((const char *)e.GetString().c_str(), "%d", &j)==1 && j>0 &&
		  canvas->mCavassData->sliceOfVolume(j-1)<canvas->mCavassData->m_zSize)
	{
	    int vol=canvas->mCavassData->volumeOfSlice(canvas->getSliceNo( 0 ));
		canvas->setEndSlice( j );
		canvas->setSliceNo( 0, canvas->mCavassData->sliceIndex(vol, j-1) );
		canvas->reload();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnStartVolTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j;
	if (sscanf((const char *)e.GetString().c_str(), "%d", &j)==1 &&
			j>0 && j<=canvas->mCavassData->m_tSize)
	{
	    int sl=canvas->mCavassData->sliceOfVolume(canvas->getSliceNo( 0 ));
		canvas->setStartVolume( j );
		canvas->setSliceNo( 0, canvas->mCavassData->sliceIndex( j-1, sl ) );
		canvas->reload();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnEndVolTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j;
	if (sscanf((const char *)e.GetString().c_str(), "%d", &j)==1 &&
			j>0 && j<=canvas->mCavassData->m_tSize)
	{
	    int sl=canvas->mCavassData->sliceOfVolume(canvas->getSliceNo( 0 ));
		canvas->setEndVolume( j );
		canvas->setSliceNo( 0, canvas->mCavassData->sliceIndex( j-1, sl ) );
		canvas->reload();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnLocXTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j;
	if (sscanf((const char *)e.GetString().c_str(), "%d", &j)==1 &&
			j>=0 && j<canvas->mCavassData->m_xSize)
	{
	    canvas->setLocX( j );
		if (mSetROIParaControls &&
				sscanf(mSetROIParaControls->getWidthValue().c_str(), "%d", &j)
				==1 && j>0 && j<=canvas->mCavassData->m_xSize)
			canvas->setWinWidth( j );
		canvas->reload();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnLocYTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j;
	if (sscanf((const char *)e.GetString().c_str(), "%d", &j)==1 &&
			j>=0 && j<canvas->mCavassData->m_ySize)
	{
	    canvas->setLocY( j );
		if (mSetROIParaControls &&
				sscanf(mSetROIParaControls->getHeightValue().c_str(), "%d", &j)
				==1 && j>0 && j<=canvas->mCavassData->m_ySize)
			canvas->setWinHeight( j );
		canvas->reload();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnWinWidthTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j=atoi((const char *)e.GetString().c_str());
	if (j <= 0)
		j = 1;
	if (j > canvas->mCavassData->m_xSize)
		j = canvas->mCavassData->m_xSize;
    canvas->setWinWidth( j );  
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for tablet */
void VOIROIFrame::OnWinHeightTablet ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
	int j=atoi((const char *)e.GetString().c_str());
	if (j <= 0)
		j = 1;
	if (j > canvas->mCavassData->m_ySize)
		j = canvas->mCavassData->m_ySize;
    canvas->setWinHeight( j ); 
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIROIFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIROIFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIROIFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIROIFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIROIFrame::OnCTLung ( wxCommandEvent& unused ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
void VOIROIFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
void VOIROIFrame::OnCTBone ( wxCommandEvent& unused ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
void VOIROIFrame::OnPET ( wxCommandEvent& unused ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
/** \brief callback for slide slider for data set 1 */
void VOIROIFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void VOIROIFrame::OnScale1Slider ( wxScrollEvent& e ) 
{
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void VOIROIFrame::OnOverlay ( wxCommandEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void VOIROIFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void VOIROIFrame::OnInvert2 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void VOIROIFrame::OnCineForward ( wxCommandEvent& e ) {
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
void VOIROIFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void VOIROIFrame::OnCineTimer ( wxTimerEvent& e ) {
    VOIROICanvas*  canvas = dynamic_cast<VOIROICanvas*>(mCanvas);
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
void VOIROIFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void VOIROIFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void VOIROIFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void VOIROIFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void VOIROIFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void VOIROIFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void VOIROIFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void VOIROIFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing */
void VOIROIFrame::OnPrint ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( VOIROIFrame, wxFrame )
BEGIN_EVENT_TABLE       ( VOIROIFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_PREVIOUS,       VOIROIFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           VOIROIFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     VOIROIFrame::OnSetIndex1   )
  EVT_BUTTON( ID_GRAYMAP1,       VOIROIFrame::OnGrayMap1    )
  EVT_BUTTON( ID_CINE,           VOIROIFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          VOIROIFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           VOIROIFrame::OnVOIROISave  )
  EVT_BUTTON( ID_VOIPARA,           VOIROIFrame::OnSetVOIPara  )
  EVT_BUTTON( ID_GRAYMAP2,           VOIROIFrame::OnGrayMap2  )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  VOIROIFrame::OnWidth2Slider  )
  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, VOIROIFrame::OnCenter2Slider )
  EVT_CHECKBOX(       ID_INVERT2,        VOIROIFrame::OnInvert2       )

  EVT_TEXT( ID_STARTSLICE, VOIROIFrame::OnStartTablet )
  EVT_TEXT( ID_ENDSLICE,   VOIROIFrame::OnEndTablet )
  EVT_TEXT( ID_STARTVOLUME,VOIROIFrame::OnStartVolTablet )
  EVT_TEXT( ID_ENDVOLUME,  VOIROIFrame::OnEndVolTablet )
  EVT_TEXT( ID_LOCX,       VOIROIFrame::OnLocXTablet )
  EVT_TEXT( ID_LOCY,       VOIROIFrame::OnLocYTablet )
  EVT_TEXT( ID_WINWIDTH,   VOIROIFrame::OnWinWidthTablet )
  EVT_TEXT( ID_WINHEIGHT,  VOIROIFrame::OnWinHeightTablet )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, VOIROIFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  VOIROIFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG,          VOIROIFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   VOIROIFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          VOIROIFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              VOIROIFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  VOIROIFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  VOIROIFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        VOIROIFrame::OnInvert1       )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, VOIROIFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  VOIROIFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  VOIROIFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  VOIROIFrame::OnUpdateUIScale1Slider  )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    VOIROIFrame::OnUpdateUICineSlider    )
#endif

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, VOIROIFrame::OnBusyTimer )
  EVT_CHAR( VOIROIFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   VOIROIFrame::OnOverlay )


  EVT_TIMER(          ID_CINE_TIMER,            VOIROIFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          VOIROIFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, VOIROIFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           VOIROIFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  VOIROIFrame::_fileHistory;
//======================================================================
