/*
  Copyright 1993-2017, 2020 Medical Image Processing Group
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
 * \file   FilterFrame.cpp
 * \brief  FilterFrame class implementation.
 *         external programs used:
 *             b_scale_anisotrop_diffus_2D
 *             b_scale_anisotrop_diffus_3D
 *             distance3D
 *             gaussian2d
 *             gaussian3d
 *             gradient
 *             gradient3d
 *             scale_based_filtering_2D
 *             scale_based_filtering_3D
 *
 * \author Ying Zhuge, Ph.D.
 * \ Modify Xinjian Chen, Ph.D
 *
 * Copyright: (C) 2007, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "CineControls.h"
#include  "ScaleADControls.h"
#include  "SetFilterIndexControls.h"
#include  "SigmaControls.h"
#include  "HomogeneityCtrl.h"
#include  "MorphCtrl.h"
#include  "InhomoCtrl.h"
#include  "LTDT3DCtrl.h"
#include  "SBAv3DCtrl.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for FilterFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
FilterFrame::FilterFrame ( bool maximize, int w, int h ) 
: MainFrame( 0 ),
//    : wxFrame( NULL, -1, _T("CAVASS"), wxPoint(gWhere,gWhere), wxSize(WIDTH,HEIGHT), wxDEFAULT_FRAME_STYLE )
m_FilterType(FILTER_GRADIENT2D){
    mFileOrDataCount = 0;
	filterName[0] = _T("Type:Grad2D");
	filterName[1] = _T("Type:Grad3D");
	filterName[2] = _T("Type:Gauss2D");
	filterName[3] = _T("Type:Gauss3D");
	filterName[4] = _T("Type:Dist2Dxy");
	filterName[5] = _T("Type:Dist3D");
	filterName[6] = _T("Type:LTDT3D");
	filterName[7] = _T("Type:SBAv2D");
	filterName[8] = _T("Type:SBAv3D");
	filterName[9] = _T("Type:Scale2D");
	filterName[10] = _T("Type:Scale3D");
	filterName[11] = _T("Parallel BScale");
	filterName[12] = _T("Type:SBAD2D");
	filterName[13] = _T("Type:SBAD3D");

	filterName[14] = _T("Type:Sobel");
	filterName[15] = _T("Type:Tobog");
	filterName[16] = _T("Type:Dilate");
	filterName[17] = _T("Type:Erode");
	filterName[18] = _T("Type:Medn2D");
	filterName[19] = _T("Type:Medn3D");
	filterName[20] = _T("Type:Inhomo1");
	filterName[21] = _T("Type:Inhomo2");
	filterName[22] = _T("Type:Inhomo3");
	filterName[23] = _T("Type:BallEnh");
	filterName[24] = _T("Type:Maxima");

	mBGProcess = false;

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       /*= mSetIndex2Box*/      = NULL;
    mSetIndex1Sizer     /*= mSetIndex2Sizer*/    = NULL;

    mCineControls       = NULL;
    mGrayMap1Controls   = mGrayMap2Controls  = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls  /*= mSetIndex2Controls*/ = NULL;

    mSigmaControls = NULL;    
    mScaleADControls = NULL;  
	mHomogenControls = NULL;
	mMorphControls = NULL;
	mInhomo1Controls = NULL;
	mInhomo2Controls = NULL;
	mInhomo3Controls = NULL;
	mBallEnhanceControls = NULL;
	mLTDT3DControls = NULL;
	mSBAv3DControls = NULL;

    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;

    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -

    mCanvas = new FilterCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    SetStatusText( "Move",   2 );
    SetStatusText( "", 3 );
    SetStatusText( "", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void FilterFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:Filter:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::addButtonBox ( void ) {
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
    wxButton*  filterBut = new wxButton( mControlPanel, ID_FILTER, "Filter", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( filterBut );
    fgs->Add( filterBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
	wxArrayString sa;
	for (int j=0; j<TypeNumber; j++)
		sa.Add(filterName[j]);
	m_filterTypeBut = new wxComboBox( mControlPanel, ID_FILTER_TYPE, filterName[m_FilterType], wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
    ::setColor( m_filterTypeBut );
    fgs->Add( m_filterTypeBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    mBGSwitch = new wxCheckBox( mControlPanel, ID_BG_PROCESS, "Background" );
	mBGSwitch->SetValue( mBGProcess );
    ::setColor( mBGSwitch );
    fgs->Add( mBGSwitch, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 5, col 1
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "Filt.GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for FilterFrame class. */
FilterFrame::~FilterFrame ( void ) {
    cout << "FilterFrame::~FilterFrame" << endl;
    wxLogMessage( "FilterFrame::~FilterFrame" );

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
 *  new FilterFrame.  It first searches the input file history for an
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
void FilterFrame::createFilterFrame ( wxFrame* parentFrame, bool useHistory )
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
    }

    if (filename.length()==0) {
        //nothing suitable found
    } else if (!wxFile::Exists(filename)) {
        //alert the user that the file no longer exists
        wxString  tmp = wxString::Format( "File %s could not be opened.", (const char *)filename.c_str() );
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION );
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
    FilterFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new FilterFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new FilterFrame();
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
bool FilterFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM", fn, false ))    return true;
    if (wxMatchWild( "*.IM0", fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \bried remove the controls for filter parameters */
void FilterFrame::hideParametersBox()
{ 
	if( mSigmaControls)
	{
		delete mSigmaControls;   
		mSigmaControls=NULL; 
	}
	if(mScaleADControls)
	{
		delete mScaleADControls;
		mScaleADControls = NULL;
	}

	if(mHomogenControls)
	{
		delete mHomogenControls;
		mHomogenControls = NULL;
	}  
	if(mMorphControls)
	{
		delete mMorphControls;
		mMorphControls = NULL;
	}
	if(mInhomo1Controls)
	{
		delete mInhomo1Controls;
		mInhomo1Controls = NULL;
	}
	if(mInhomo2Controls)
	{
		delete mInhomo2Controls;
		mInhomo2Controls = NULL;
	}
	if(mInhomo3Controls)
	{
		delete mInhomo3Controls;
		mInhomo3Controls = NULL;
	}
	if (mBallEnhanceControls)
	{
		delete mBallEnhanceControls;
		mBallEnhanceControls = NULL;
	}
	if(mLTDT3DControls)
	{
		delete mLTDT3DControls;
		mLTDT3DControls = NULL;
	} 
	if(mSBAv3DControls)
	{
		delete mSBAv3DControls;
		mSBAv3DControls = NULL;
	} 	
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void FilterFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "FilterFrame::OnChar" );
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
        cout << "FilterFrame::OnChar: " << ::gTimerInterval << endl;
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
void FilterFrame::OnSaveScreen ( wxCommandEvent& e ) {
	if (mSaveScreenControls!=NULL) {  removeAll();  return;  }
	removeAll();
	MainFrame::OnSaveScreen( e );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void FilterFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::OnHideControls ( wxCommandEvent& e ) {
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
void FilterFrame::OnOpen ( wxCommandEvent& e ) {
    createFilterFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void FilterFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "FilterFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "FilterFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnPPScopsFilter( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void FilterFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:Filter: ");
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
            tmp = wxString::Format( "CAVASS:Filter:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:Filter: ");
    tmp += name;
    SetTitle( tmp );
    
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnPrevious ( wxCommandEvent& unused ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;

    if (sliceA<0 )
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );
	canvas->setFilterDone(false);
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnNext ( wxCommandEvent& unused ) 
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;

    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );
	canvas->setFilterDone(false);

    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::removeAll ( void ) {
    if (mCineControls!=NULL) {
        delete mCineControls;
        mCineControls = NULL;
    }
    if (mGrayMap1Controls!=NULL) {
        delete mGrayMap1Controls;
        mGrayMap1Controls = NULL;
    }
    if (mGrayMap2Controls!=NULL) {
        delete mGrayMap2Controls;
        mGrayMap2Controls = NULL;
    }
	if (mSaveScreenControls!=NULL) {
		delete mSaveScreenControls;
		mSaveScreenControls = NULL;
	}
    if (mScaleADControls!=NULL) {
		delete mScaleADControls;
		mScaleADControls=NULL;
	}
    if (mSetIndex1Controls!=NULL) {
        delete mSetIndex1Controls;
        mSetIndex1Controls = NULL;
    }
    if (mSigmaControls!=NULL) {
		delete mSigmaControls;
		mSigmaControls=NULL;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
	if (mSetIndex1Controls!=NULL) {  removeAll();  return;  }
	removeAll();

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFilterIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
						    ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
	if (mGrayMap1Controls!=NULL)  {  removeAll();  return;  }
	removeAll();

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
	if (mGrayMap2Controls!=NULL) {  removeAll();  return;  }
	removeAll();

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
	if( ! canvas->getFilterDone() )
		return;

    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap2", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnBG ( wxCommandEvent &e )
{
	mBGProcess = e.IsChecked();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnCine ( wxCommandEvent& unused ) {
	if (mCineControls!=NULL) {  removeAll();  return;  }
	removeAll();

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
	canvas->setFilterDone(false);

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnReset ( wxCommandEvent& unused ) 
{
    m_cine_timer->Stop();

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->mCavassData->mDisplay = true;

	canvas->initializeParameters();
	updateParametersBox();

    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnFilterSave ( wxCommandEvent &e ) 
{
	FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);

	switch (m_FilterType) 
	{
	case FILTER_LTDT3D:
	case FILTER_DIST3D:
	case FILTER_DIST2Dxy:
		if( canvas->mCavassData->m_vh.scn.num_of_bits != 1 )
		{
			wxLogMessage( "Only accept BIM format!" );
			wxMessageBox( "Only accept BIM format!" );
			return;
		}

	default:
		break;
	}

	wxString wFormat1, wFormat2;
	if (( ( m_FilterType == FILTER_DILATE || m_FilterType == FILTER_ERODE ) &&
            canvas->mCavassData->m_vh.scn.num_of_bits == 1 ) ||
            m_FilterType == FILTER_MAXIMA)
	{
		wFormat1 = "filter.BIM";	  wFormat2 = "BIM files (*.BIM)|*.BIM";  
	}
	else
	{
		wFormat1 = "filter.IM0";	  wFormat2 = "IM0 files (*.IM0)|*.IM0";  
	}

	
    wxFileDialog  saveDlg( this, _T("Save file"), wxEmptyString,
                           wFormat1, wFormat2,
                           wxSAVE|wxOVERWRITE_PROMPT );

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, filter %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;

    wxString  cmd("\""), rname, cmd1("\""), cmd2("\""), cmd3("\"");
	cmd += Preferences::getHome()+"/";
	int iterations=1;
	wxFileName temp_file1(mCanvas->mCavassData->m_fname),
	    temp_file2(mCanvas->mCavassData->m_fname);
    temp_file1.SetName(temp_file1.GetName()+"_temp1");
	temp_file2.SetName(temp_file2.GetName()+"_temp2");
    switch (m_FilterType) 
	{
      case FILTER_GAUSSIAN2D:
		  cmd += wxString::Format("gaussian2d\" \"%s\" \"%s\" %d %f", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(),0, canvas->getSigma());
          break;
      case FILTER_GAUSSIAN3D:
          cmd += wxString::Format("gaussian3d\" \"%s\" \"%s\" %d %f", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(),0, canvas->getSigma());
          break;
	  case FILTER_LTDT3D:
		  {
			  if( canvas->getLTDTPararell() )
				  cmd += wxString::Format("p3DLTDT\" \"%s\" \"%s\" %d %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(),0, canvas->getLTDTDistType());
			  else
				  cmd += wxString::Format("LTDT3D\" \"%s\" \"%s\" %d %d %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(),0, canvas->getLTDTDistType(), canvas->getLTDTFTSave() );
		  }
          break;
      case FILTER_SCALE2D:
          cmd += wxString::Format("scale_based_filtering_2D\" \"%s\" \"%s\" %s %d", (mCanvas->mCavassData)->m_fname,  
                   (const char *)saveDlg.GetPath().c_str(), "/dev/null", canvas->getHomogen());
          break;

      case FILTER_SCALE3D:
          cmd += wxString::Format("scale_based_filtering_3D\" \"%s\" \"%s\" %s %d", (mCanvas->mCavassData)->m_fname, 
                   (const char *)saveDlg.GetPath().c_str(), "/dev/null", canvas->getHomogen());
          break;

      case FILTER_SBAv2D:
          cmd += wxString::Format("scale_based_filtering_2D\" \"%s\" %s \"%s\" %d", (mCanvas->mCavassData)->m_fname, "/dev/null", 
                   (const char *)saveDlg.GetPath().c_str(),  canvas->getHomogen());
          break;

      case FILTER_SBAv3D:
		  {
			   if( canvas->getSBAv3DPararell() )
				  cmd += wxString::Format("p3dSBAFilter\" \"%s\" \"%s\"", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str() );
			  else
				  cmd += wxString::Format("scale_based_filtering_3D\" \"%s\" %s \"%s\" %d", (mCanvas->mCavassData)->m_fname,  "/dev/null",
                   (const char *)saveDlg.GetPath().c_str(),  canvas->getHomogen());
		  }
          break;
	  case FILTER_BScale3D:
		  {
			  // if( canvas->getSBAv3DPararell() )
				  cmd += wxString::Format("p3dBScaleCompute\" \"%s\" \"%s\"", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str() );
			  
		  }
          break;
      case FILTER_SBAD2D:
          cmd += wxString::Format("b_scale_anisotrop_diffus_2D\" \"%s\" \"%s\" %d %d", (mCanvas->mCavassData)->m_fname,  
                   (const char *)saveDlg.GetPath().c_str(), canvas->getHomogen(),canvas->getScaleADIterations());
          break;


      case FILTER_SBAD3D:
          cmd += wxString::Format("b_scale_anisotrop_diffus_3D\" \"%s\" \"%s\" %d %d", (mCanvas->mCavassData)->m_fname,  
                   (const char *)saveDlg.GetPath().c_str(), canvas->getHomogen(),canvas->getScaleADIterations());
          break;
      case FILTER_GRADIENT2D:
          cmd += wxString::Format("gradient\" \"%s\" \"%s\" %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(),0);
          break;

      case FILTER_GRADIENT3D:
          cmd += wxString::Format("gradient3d\" \"%s\" \"%s\" %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(),0);
          break;

      case FILTER_DIST2Dxy:
          cmd += wxString::Format("distance3D\" -e -p xy -s \"%s\" \"%s\"", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str());
          break;

      case FILTER_DIST3D:
          cmd += wxString::Format("distance3D\" -e -p xyz -s \"%s\" \"%s\"", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str());
          break;

	  case FILTER_SOBEL:
		  cmd += wxString::Format("sobel\" \"%s\" \"%s\" %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 0);
          break;		
	  case FILTER_TOBOG:   
		  cmd += wxString::Format("toboggan\" \"%s\" \"%s\" %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 0);
		  break;
	  case FILTER_MEAN2D:
		  cmd += wxString::Format("median2d\" \"%s\" \"%s\" %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 0);		 
		  break;
	  case FILTER_MEAN3D:  
		  cmd += wxString::Format("median3d\" \"%s\" \"%s\" %d", (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 0);
		  break;
	  case FILTER_DILATE:
	      iterations = canvas->getMorphIterations();
	      cmd += wxString::Format("mmorph\" \"%s\" \"%s\" %d %d",
		       (mCanvas->mCavassData)->m_fname,
			   (const char *)saveDlg.GetPath().c_str(),
		       canvas->getMorphN(), iterations);
		  break;
	  case FILTER_ERODE:
	      iterations = canvas->getMorphIterations();
	      cmd += wxString::Format("morph\" \"%s\" \"%s\" %d %d",
		       (mCanvas->mCavassData)->m_fname,
			   (const char *)saveDlg.GetPath().c_str(),
		       -canvas->getMorphN(), iterations);
		  break;
	  case FILTER_INHOMO1:
		  cmd += wxString::Format("inhomo_correct_whole\" \"%s\" \"%s\" -mode 1 -left %f -right %f -times %d -volume_threshold %f -stop_condition %f", 
			  (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 
			  canvas->getLeftParam(), canvas->getRightParam(), canvas->getScaleADIterations(),
			  canvas->getVolThresh(), canvas->getStopCond());
		  break;
	  case FILTER_INHOMO2:
		  if( canvas->getStdDevia() > 0 )
			  cmd += wxString::Format("inhomo_gscale_float\" \"%s\" \"%s\" -mode 1 -zeta %f -zetafactor %f -thetab %f -num_regions %d -times %d", 
			  (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 
			  canvas->getStdDevia(), canvas->getZetaFactor(), canvas->getIncluFactor(), canvas->getNumOfRegion(), canvas->getScaleADIterations());		  
		  else
			  cmd += wxString::Format("inhomo_gscale_float\" \"%s\" \"%s\" -mode 1 -zetafactor %f -thetab %f -num_regions %d -times %d", 
			  (mCanvas->mCavassData)->m_fname, (const char *)saveDlg.GetPath().c_str(), 
			  canvas->getZetaFactor(), canvas->getIncluFactor(), canvas->getNumOfRegion(), canvas->getScaleADIterations());		  
		  break;
	  case FILTER_INHOMO3:
		  cmd += wxString::Format("inhomo_correct_interac\" \"%s\" \"%s\"",
			  (mCanvas->mCavassData)->m_fname,
			  (const char *)saveDlg.GetPath().c_str());
		  rname = wxString(mCanvas->mCavassData->m_fname).BeforeLast('.');
		  for (int j=1; j<=canvas->num_interval; j++)
		      cmd += " \""+rname+wxString::Format("_tr%d.BIM\"", j);
		  break;
      case FILTER_BALL_ENH:
        {
	      wxFileName fn(saveDlg.GetPath());
          cmd += wxString::Format("ball_enhance\" \"%s\" \"%s\" %d %d -r \"",
		      (mCanvas->mCavassData)->m_fname,
			  (const char *)fn.GetFullPath().c_str(), canvas->m_MaxRadius,
			  canvas->m_MinRadius);
          fn.SetName(fn.GetName()+"-r");
		  cmd += fn.GetFullPath()+"\"";
        }
          break;
      case FILTER_MAXIMA:
          cmd += wxString::Format("local_maxima\" \"%s\" \"%s\"",
              (mCanvas->mCavassData)->m_fname,
              (const char *)saveDlg.GetPath().c_str());
          break;

      default:
          break;
    }

    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    printf("command=%s\n", (const char *)cmd.c_str() );
    fflush(stdout);

    if ( m_FilterType == FILTER_LTDT3D && canvas->getLTDTPararell()) 
    {
        ParallelProcessManager  p( "p3DLTDT started", cmd, 1 );
    }
    else if ( m_FilterType == FILTER_SBAv3D && canvas->getSBAv3DPararell()) 
    {
        ParallelProcessManager  p( "p3dSBAFilter started", cmd, 1 );
    }
    else if ( m_FilterType == FILTER_BScale3D ) 
    {
        ParallelProcessManager  p( "p3dBScaleCompute started", cmd, 1 );
    }
    else 
    {
        ProcessManager  p( "filter running...", cmd, !mBGProcess );
        if (p.getCancel())    return;    
        p.getStatus();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for  filter button */
void FilterFrame::OnFilter ( wxCommandEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->RunFilter();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::updateParametersBox ( void )
{
   FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);

   /* modify the parameters box */
   hideParametersBox();
   switch (m_FilterType)
   {
   case FILTER_GAUSSIAN3D:
   case FILTER_GAUSSIAN2D:   
     if (mSigmaControls!=NULL)  return;

     mSigmaControls = new SigmaControls( mControlPanel, mBottomSizer,
		"Gaussian Par.", ID_FILTER_SIGMA,canvas->getSigma());
     mBottomSizer->Layout();
     break;
   case FILTER_SCALE2D:
   case FILTER_SCALE3D:
   case FILTER_SBAv2D:   
   case FILTER_BScale3D:
	 if (mHomogenControls!=NULL)  return;

     mHomogenControls = new HomogenControls( mControlPanel, mBottomSizer,
	 	"B-Scale Par.", ID_FILTER_INHOMO2,canvas->getHomogen());
     mBottomSizer->Layout();
     break;
   case FILTER_SBAv3D:
	    if (mSBAv3DControls!=NULL)  return;

     mSBAv3DControls = new SBAv3DControls( mControlPanel, mBottomSizer,"SBAv3D Par.", ID_FILTER_INHOMO2,ID_FILTER_SBAV3D_PARARELLID, canvas->getHomogen());
     mBottomSizer->Layout();
     break;

   case FILTER_SBAD2D:
   case FILTER_SBAD3D:
	  if (mScaleADControls!=NULL)  return;

     mScaleADControls = new ScaleADControls( mControlPanel, mBottomSizer,
	 	"Scale Filtering Par.", ID_FILTER_INHOMO2,ID_FILTER_ITERATIONS,
					     canvas->getHomogen(),canvas->getScaleADIterations());
     mBottomSizer->Layout();
     break;

   case FILTER_DILATE:
   case FILTER_ERODE:
	 if (mMorphControls!=NULL)  return;

     mMorphControls = new MorphControls( mControlPanel, mBottomSizer,
		"Structuring Element", ID_FILTER_MORPH, canvas->getMorphN(),
		canvas->getMorphIterations(), ID_FILTER_ITERATIONS);
	 mBottomSizer->Layout();
     break;

   case FILTER_INHOMO1:
	   if (mInhomo1Controls!=NULL)  return;

	   mInhomo1Controls = new Inhomo1Controls( mControlPanel, mBottomSizer,"Inhomo. Correction Par.", 
		   ID_FILTER_VOL_THRESH,ID_FILTER_STOP_COND,ID_FILTER_LEFT_PAR,ID_FILTER_RIGHT_PAR, ID_FILTER_ITERATIONS,
		   canvas->getVolThresh(), canvas->getStopCond(), canvas->getLeftParam(), (int)canvas->getRightParam(), canvas->getScaleADIterations());
	   mBottomSizer->Layout();
	   break;
   case FILTER_INHOMO2:
	   if (mInhomo2Controls!=NULL)  return;

	   mInhomo2Controls = new Inhomo2Controls( mControlPanel, mBottomSizer,"Inhomo. Correction Par.", 
		   ID_FILTER_ZETAFACTOR,ID_FILTER_INCLUFACTOR,ID_FILTER_STDDEVIA,ID_FILTER_NUMOFREGION, ID_FILTER_ITERATIONS,
		   canvas->getZetaFactor(), canvas->getIncluFactor(), canvas->getStdDevia(), canvas->getNumOfRegion(), canvas->getScaleADIterations());
	   mBottomSizer->Layout();
	   break;
   case FILTER_INHOMO3:
	   if (mInhomo3Controls!=NULL)  return;

	   mInhomo3Controls = new Inhomo3Controls( mControlPanel, mBottomSizer,
		   "Inhomo. Correction Par.", 
		   ID_FILTER_MIN0, ID_FILTER_MIN1, ID_FILTER_MIN2, ID_FILTER_MIN3,
		   ID_FILTER_MAX0, ID_FILTER_MAX1, ID_FILTER_MAX2, ID_FILTER_MAX3,
		   ID_FILTER_NUMOFREGION, ID_FILTER_ITERATIONS,
		   canvas->min_thresh[0], canvas->min_thresh[1],
		   canvas->min_thresh[2], canvas->min_thresh[3],
		   canvas->max_thresh[0], canvas->max_thresh[1],
		   canvas->max_thresh[2], canvas->max_thresh[3],
		   canvas->num_interval, canvas->getScaleADIterations());
	   mBottomSizer->Layout();
	   break;
   case FILTER_BALL_ENH:
       mBallEnhanceControls = new BallEnhanceControls( mControlPanel,
	       mBottomSizer, "Ball Enhance Par.", ID_FILTER_MAX_BALL,
		   canvas->m_MaxRadius, ID_FILTER_MIN_BALL, canvas->m_MinRadius );
       mBottomSizer->Layout();
       break;
   case FILTER_LTDT3D:
	 if (mLTDT3DControls!=NULL)  return;

     mLTDT3DControls = new LTDT3DControls( mControlPanel, mBottomSizer,"LTDT3D Par.", ID_FILTER_LTDT3D, ID_FILTER_LTDT3D_PARARELLID, ID_FILTER_LTDT3D_FTSAVEID, 0);
     mBottomSizer->Layout();
     break;

   case FILTER_GRADIENT2D:
   case FILTER_GRADIENT3D:
   case FILTER_DIST2Dxy:
   case FILTER_DIST3D:
   case FILTER_SOBEL:
   case FILTER_TOBOG:   
   case FILTER_MEAN2D:
   case FILTER_MEAN3D:
   case FILTER_MAXIMA:
   default:
	   break;

   }

   mBottomSizer->Layout();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for  filterType button */
void FilterFrame::OnFilterType ( wxCommandEvent& e ) 
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->setFilterDone( false );
	for (m_FilterType=(CavassFilterType)0;
			filterName[m_FilterType]!=e.GetString();
			m_FilterType=(CavassFilterType)(m_FilterType+1))
		;

	updateParametersBox();
    canvas->setFilterType(m_FilterType);
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterSigma(wxCommandEvent& e)
{

	FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
	wxString sigmaStr = e.GetString();
	if( sigmaStr.IsEmpty() )
		return;

    double sigma; 
    sigmaStr.ToDouble(&sigma);
    canvas->setSigma(sigma);
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterHomogen(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString homogenStr = e.GetString();
	if( homogenStr.IsEmpty() )
		return;

    long homogen; 
    homogenStr.ToLong(&homogen);
    canvas->setHomogen(homogen);
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterMorph(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);

	long morphN;
	e.GetString().ToLong(&morphN);
    canvas->setMorphN((int)morphN);
    canvas->setFilterDone( false );
    canvas->reload();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnLTDT3DDistType(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);

	int distType; 
	distType = e.GetSelection();	
    
    canvas->setLTDTDistType(distType);
    canvas->setFilterDone( false );
    canvas->reload();
}

void FilterFrame::OnLTDT3DPararell(wxCommandEvent& e)
{
    bool  value = e.IsChecked();
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    
    canvas->setLTDTPararell(value);    
}

void FilterFrame::OnLTDT3DFTSave(wxCommandEvent& e)
{
    bool  value = e.IsChecked();
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    
    canvas->setLTDTFTSave(value);    
}

void FilterFrame::OnSBAV3DPararell(wxCommandEvent& e)
{
    bool  value = e.IsChecked();
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    
    canvas->setSBAv3DPararell(value);    
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterZetaFactor(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString zetaFactorStr = e.GetString();
	if( zetaFactorStr.IsEmpty() )
		return;

    double zetaFactor; 
    zetaFactorStr.ToDouble(&zetaFactor);
    canvas->setZetaFactor(zetaFactor);
    canvas->setFilterDone( false );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnFilterVolThresh(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString volThreshStr = e.GetString();
	if( volThreshStr.IsEmpty() )
		return;

    double volThresh; 
    volThreshStr.ToDouble(&volThresh);
    canvas->setVolThresh(volThresh);
    canvas->setFilterDone( false );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnFilterStopCond(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString stopCondStr = e.GetString();
	if( stopCondStr.IsEmpty() )
		return;

    double stopCond; 
    stopCondStr.ToDouble(&stopCond);
    canvas->setStopCond(stopCond);
    canvas->setFilterDone( false );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnFilterLeftParam(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString leftParamStr = e.GetString();
	if( leftParamStr.IsEmpty() )
		return;

    double leftParam; 
    leftParamStr.ToDouble(&leftParam);
    canvas->setLeftParam(leftParam);
    canvas->setFilterDone( false );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilterFrame::OnFilterRightParam(wxCommandEvent& e)
{
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString rightParamStr = e.GetString();
	if( rightParamStr.IsEmpty() )
		return;

    double rightParam; 
    rightParamStr.ToDouble(&rightParam);
    canvas->setRightParam(rightParam);
    canvas->setFilterDone( false );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterIncluFactor(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString incluFactorStr = e.GetString();
	if( incluFactorStr.IsEmpty() )
		return;

    double incluFactor; 
    incluFactorStr.ToDouble(&incluFactor);
    canvas->setIncluFactor(incluFactor);
    canvas->setFilterDone( false );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterStdDevia(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString stdDeviaStr = e.GetString();
	if( stdDeviaStr.IsEmpty() )
		return;

    double stdDevia; 
    stdDeviaStr.ToDouble(&stdDevia);
    canvas->setStdDevia(stdDevia);
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterNumofRegion(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString numOfRegionStr = e.GetString();
	if( numOfRegionStr.IsEmpty() )
		return;

    long numOfRegion; 
    numOfRegionStr.ToLong(&numOfRegion);
    canvas->setNumOfRegion(numOfRegion);
	canvas->num_interval = (int)numOfRegion;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterMaxRadius(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString max_radiusStr = e.GetString();
	if( max_radiusStr.IsEmpty() )
		return;

    long max_radius; 
    max_radiusStr.ToLong(&max_radius);
	canvas->m_MaxRadius = (int)max_radius;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale computation
*
*/
void FilterFrame::OnFilterMinRadius(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString min_radiusStr = e.GetString();
	if( min_radiusStr.IsEmpty() )
		return;

    long min_radius; 
    min_radiusStr.ToLong(&min_radius);
	canvas->m_MinRadius = (int)min_radius;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMin0(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->min_thresh[0] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMax0(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->max_thresh[0] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMin1(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->min_thresh[1] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMax1(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->max_thresh[1] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMin2(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->min_thresh[2] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMax2(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->max_thresh[2] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMin3(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->min_thresh[3] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of interval computation
*
*/
void FilterFrame::OnFilterMax3(wxCommandEvent& e)
{

    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxString thresholdStr = e.GetString();
	if( thresholdStr.IsEmpty() )
		return;

    long threshold; 
    thresholdStr.ToLong(&threshold);
    canvas->max_thresh[3] = (int)threshold;
    canvas->setFilterDone( false );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for TextControl of scale based anisotropic diffusion
*
*/
void FilterFrame::OnFilterScaleIterations(wxCommandEvent& e)
{
	FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);

	wxString iterStr = e.GetString();
	if( iterStr.IsEmpty() )
		return;

	long iter; 
	iterStr.ToLong(&iter);

	switch (m_FilterType)
	{
		case FILTER_DILATE:
		case FILTER_ERODE:
			canvas->setMorphIterations(iter);
			break;
		default:
			canvas->setScaleADIterations(iter);
			break;
	}
	canvas->setFilterDone( false );
	canvas->reload();	
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom in button
 *  \todo implement multi resolution sliders
 */
void FilterFrame::OnZoomIn ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom out button
 *  \todo implement multi resolution sliders
 */
void FilterFrame::OnZoomOut ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void FilterFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 2 */
void FilterFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void FilterFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 2 */
void FilterFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void FilterFrame::OnCTLung ( wxCommandEvent& unused ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::OnCTBone ( wxCommandEvent& unused ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::OnPET ( wxCommandEvent& unused ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
	canvas->setFilterDone(false);
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void FilterFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void FilterFrame::OnOverlay ( wxCommandEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void FilterFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 2 */
void FilterFrame::OnInvert2 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void FilterFrame::OnCineForward ( wxCommandEvent& e ) {
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
void FilterFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void FilterFrame::OnCineTimer ( wxTimerEvent& e ) {
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
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
void FilterFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void FilterFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void FilterFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void FilterFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void FilterFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void FilterFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue()-1 );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void FilterFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void FilterFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new FilterPrint("CAVASS", canvas),
        new FilterPrint("CAVASS", canvas),
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
void FilterFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    FilterCanvas*  canvas = dynamic_cast<FilterCanvas*>(mCanvas);
    FilterPrint       printout( _T("CAVASS:Filter"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( FilterFrame, wxFrame )
BEGIN_EVENT_TABLE       ( FilterFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_PREVIOUS,       FilterFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           FilterFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     FilterFrame::OnSetIndex1   )
  EVT_BUTTON( ID_GRAYMAP1,       FilterFrame::OnGrayMap1    )
  EVT_BUTTON( ID_GRAYMAP2,       FilterFrame::OnGrayMap2    )
  EVT_BUTTON( ID_FILTER,         FilterFrame::OnFilter        )
  EVT_COMBOBOX( ID_FILTER_TYPE,    FilterFrame::OnFilterType    )
  EVT_BUTTON( ID_CINE,           FilterFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          FilterFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           FilterFrame::OnFilterSave  )
  EVT_CHECKBOX( ID_BG_PROCESS,   FilterFrame::OnBG          )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, FilterFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  FilterFrame::OnWidth1Slider  )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  FilterFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  FilterFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        FilterFrame::OnInvert1       )

  EVT_BUTTON( ID_CT_LUNG,           FilterFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,    FilterFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,           FilterFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,               FilterFrame::OnPET     )

  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, FilterFrame::OnCenter2Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  FilterFrame::OnWidth2Slider  )
  EVT_CHECKBOX(       ID_INVERT2,        FilterFrame::OnInvert2       )
  EVT_TEXT( ID_FILTER_SIGMA,            FilterFrame::OnFilterSigma )
  EVT_TEXT( ID_FILTER_INHOMO2,            FilterFrame::OnFilterHomogen )
  EVT_TEXT( ID_FILTER_ITERATIONS,       FilterFrame::OnFilterScaleIterations )
  EVT_TEXT( ID_FILTER_ZETAFACTOR,       FilterFrame::OnFilterZetaFactor )
  EVT_TEXT( ID_FILTER_INCLUFACTOR,       FilterFrame::OnFilterIncluFactor )
  EVT_TEXT( ID_FILTER_STDDEVIA,       FilterFrame::OnFilterStdDevia )
  EVT_TEXT( ID_FILTER_NUMOFREGION,       FilterFrame::OnFilterNumofRegion )
  EVT_TEXT( ID_FILTER_MAX_BALL,          FilterFrame::OnFilterMaxRadius )
  EVT_TEXT( ID_FILTER_MIN_BALL,          FilterFrame::OnFilterMinRadius )
  EVT_TEXT( ID_FILTER_VOL_THRESH,        FilterFrame::OnFilterVolThresh )
  EVT_TEXT( ID_FILTER_STOP_COND,         FilterFrame::OnFilterStopCond )
  EVT_TEXT( ID_FILTER_LEFT_PAR,          FilterFrame::OnFilterLeftParam )
  EVT_TEXT( ID_FILTER_RIGHT_PAR,         FilterFrame::OnFilterRightParam )
  EVT_TEXT( ID_FILTER_MIN0,              FilterFrame::OnFilterMin0 )
  EVT_TEXT( ID_FILTER_MAX0,              FilterFrame::OnFilterMax0 )
  EVT_TEXT( ID_FILTER_MIN1,              FilterFrame::OnFilterMin1 )
  EVT_TEXT( ID_FILTER_MAX1,              FilterFrame::OnFilterMax1 )
  EVT_TEXT( ID_FILTER_MIN2,              FilterFrame::OnFilterMin2 )
  EVT_TEXT( ID_FILTER_MAX2,              FilterFrame::OnFilterMax2 )
  EVT_TEXT( ID_FILTER_MIN3,              FilterFrame::OnFilterMin3 )
  EVT_TEXT( ID_FILTER_MAX3,              FilterFrame::OnFilterMax3 )


  EVT_COMBOBOX( ID_FILTER_MORPH,            FilterFrame::OnFilterMorph )

  EVT_COMBOBOX( ID_FILTER_LTDT3D,       FilterFrame::OnLTDT3DDistType       )
  EVT_CHECKBOX( ID_FILTER_LTDT3D_PARARELLID,        FilterFrame::OnLTDT3DPararell       )
  EVT_CHECKBOX( ID_FILTER_LTDT3D_FTSAVEID,        FilterFrame::OnLTDT3DFTSave       )
  EVT_CHECKBOX( ID_FILTER_SBAV3D_PARARELLID,        FilterFrame::OnSBAV3DPararell       )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, FilterFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  FilterFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  FilterFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  FilterFrame::OnUpdateUIScale1Slider  )

  EVT_UPDATE_UI( ID_CENTER2_SLIDER, FilterFrame::OnUpdateUICenter2Slider )
  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  FilterFrame::OnUpdateUIWidth2Sglider )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    FilterFrame::OnUpdateUICineSlider    )
#endif

  EVT_BUTTON( ID_ZOOM_IN,        FilterFrame::OnZoomIn                 )
  EVT_BUTTON( ID_ZOOM_OUT,       FilterFrame::OnZoomOut                )

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, FilterFrame::OnBusyTimer )
  EVT_CHAR( FilterFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   FilterFrame::OnOverlay )

  EVT_TIMER(          ID_CINE_TIMER,            FilterFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          FilterFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, FilterFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           FilterFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  FilterFrame::_fileHistory;
//======================================================================
