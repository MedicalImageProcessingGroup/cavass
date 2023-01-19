/*
  Copyright 1993-2016, 2019 Medical Image Processing Group
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
 * \file   InterpolateFrame.cpp
 * \brief  InterpolateFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "InterpolateFrame.h"

extern Vector  gFrameList;
//----------------------------------------------------------------------
/** \brief Constructor for InterpolateFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
InterpolateFrame::InterpolateFrame ( bool maximize, int w, int h )
    : MainFrame( 0 )
{
    //init the types of input files that this app can accept
    mFileNameFilter     = (char *)"CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0";
    mFileOrDataCount    = 0;
    mModuleName         = "CAVASS:Preprocess:SceneOperations:Interpolate";
    mSaveScreenControls = NULL;

    //if we are in the mode that supports having more than one window open
    // at a time, we need to add this window to the list.
    ::gFrameList.push_back( this );
    if (!Preferences::getSingleFrameMode()) {
        gWhere += 20;
        if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    }
    
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via InterpolateCanvas
    mCanvas = new InterpolateCanvas( mSplitter, this, -1, wxDefaultPosition,
                                     wxDefaultSize );
    if (Preferences::getCustomAppearance()) {
        mCanvas->SetBackgroundColour( wxColour(DkBlue) );
        mCanvas->SetForegroundColour( wxColour(Yellow) );
    }
    wxSizer*  topSizer = new wxBoxSizer( wxVERTICAL );
    topSizer->SetMinSize( 700, 400 );
    topSizer->Add( mCanvas, 1, wxGROW );

    //bottom of window contains controls
    mControlPanel = new wxPanel( mSplitter, -1, wxDefaultPosition, wxDefaultSize );
    ::setColor( mControlPanel );
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

//    addButtonBox();  //usually here but delayed until file is loaded
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );
    mSplitter->SetSashPosition( -dControlsHeight );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Move",  2 );
    SetStatusText( "Prev",  3 );
    SetStatusText( "Next",  4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
    mSplitter->SetSashPosition( -dControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones).
 *  this function does not any additional, custom menu items (but could
 *  if necessary).
 */
void InterpolateFrame::initializeMenu ( void ) {
    //init standard menu bar and menu items
    MainFrame::initializeMenu();
    //if we are in the mode that supports having more than one window open
    // at a time, we need to make other windows aware of this window.
    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "%s:%d", (const char *)mModuleName.c_str(), j );
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );

    //enable the Open menu item
    wxMenuItem*  op = mFileMenu->FindItem( ID_OPEN );
    op->Enable( true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right. */
void InterpolateFrame::addButtonBox ( void ) {
    if (mCanvas==NULL || mCanvas->mCavassData==NULL)    return;
    //determine the overall min spacing
    assert( mCanvas!=NULL && mCanvas->mCavassData!=NULL );
    double  minSpacing = mCanvas->mCavassData->m_xSpacing;
    if (mCanvas->mCavassData->m_ySpacing<minSpacing)
        minSpacing = mCanvas->mCavassData->m_ySpacing;
    if (mCanvas->mCavassData->m_zSpacing<minSpacing)
        minSpacing = mCanvas->mCavassData->m_zSpacing;

    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "Interpolate" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 8, 2, 10 );  //cols, vgap, hgap

    wxStaticText*  st = NULL;  //used and initialized below
    wxString  s;               //used and initialized below
    //interpolate method choices
    const wxString  choices[] = { "NearestNeighbor", "Linear", "Cubic" };

    //row 1
    s = wxString::Format( "X1 (%.4f):", mCanvas->mCavassData->m_xSpacing );
    st = new wxStaticText( mControlPanel, wxID_ANY, s );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    s = wxString::Format( "%.4f", minSpacing );
    mX1Value = new wxTextCtrl( mControlPanel, ID_X1_VALUE, s );
    ::setColor( mX1Value );
    fgs->Add( mX1Value, 0, wxGROW );

    st = new wxStaticText( mControlPanel, wxID_ANY, "X1:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    mX1Choice = new wxComboBox( mControlPanel, ID_X1_CHOICE, "Linear", wxDefaultPosition, wxDefaultSize, 3, choices, wxCB_DROPDOWN|wxCB_READONLY );
    ::setColor( mX1Choice );
    fgs->Add( mX1Choice, 0, wxALIGN_LEFT );

    st = new wxStaticText( mControlPanel, wxID_ANY, "min:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    mMinValue = new wxTextCtrl( mControlPanel, wxID_ANY, "1" );
    ::setColor( mMinValue );
    fgs->Add( mMinValue, 0, wxGROW );

    st = new wxStaticText( mControlPanel, wxID_ANY, "max:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    int z_size= mCanvas->mCavassData->m_vh.scn.dimension==4?
        mCanvas->mCavassData->m_vh.scn.num_of_subscenes[1]:
        mCanvas->mCavassData->m_vh.scn.num_of_subscenes[0];
    s = wxString::Format( "%d", z_size );
    mMaxValue = new wxTextCtrl( mControlPanel, wxID_ANY, s );
    ::setColor( mMaxValue );
    fgs->Add( mMaxValue, 0, wxGROW );

    //row 2
    s = wxString::Format( "X2 (%.4f):", mCanvas->mCavassData->m_ySpacing );
    st = new wxStaticText( mControlPanel, wxID_ANY, s );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    s = wxString::Format( "%.4f", minSpacing );
    mX2Value = new wxTextCtrl( mControlPanel, ID_X2_VALUE, s );
    ::setColor( mX2Value );
    fgs->Add( mX2Value, 0, wxGROW );

    st = new wxStaticText( mControlPanel, wxID_ANY, "X2:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    mX2Choice = new wxComboBox( mControlPanel, ID_X2_CHOICE, "Linear", wxDefaultPosition, wxDefaultSize, 3, choices, wxCB_READONLY );
    ::setColor( mX2Choice );
    fgs->Add( mX2Choice, 0, wxALIGN_LEFT );

    fgs->AddStretchSpacer();

    mForeground = new wxCheckBox( mControlPanel, ID_FOREGROUND, "foreground", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mForeground->SetValue( true );
    ::setColor( mForeground );
    fgs->Add(   mForeground, 0, wxALIGN_LEFT, 0 );

    fgs->AddStretchSpacer();

    mParallel = new wxCheckBox( mControlPanel, ID_PARALLEL, "parallel", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
#ifdef  PARALLEL
    mParallel->Enable( Preferences::getParallelMode() );
#else
    mParallel->Enable( false );
#endif
    ::setColor( mParallel );
    fgs->Add(   mParallel, 0, wxALIGN_LEFT, 0 );

    //row 3
    s = wxString::Format( "X3 (%.4f):", mCanvas->mCavassData->m_zSpacing );
    st = new wxStaticText( mControlPanel, wxID_ANY, s );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    s = wxString::Format( "%.4f", minSpacing );
    mX3Value = new wxTextCtrl( mControlPanel, ID_X3_VALUE, s );
    ::setColor( mX3Value );
    fgs->Add( mX3Value, 0, wxGROW );

    st = new wxStaticText( mControlPanel, wxID_ANY, "X3:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    mX3Choice = new wxComboBox( mControlPanel, ID_X3_CHOICE, "Linear", wxDefaultPosition, wxDefaultSize, 3, choices, wxCB_READONLY );
    ::setColor( mX3Choice );
    fgs->Add( mX3Choice, 0, wxALIGN_LEFT );

    st = new wxStaticText( mControlPanel, wxID_ANY, "distance:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    const wxString  distance[] = { "cityblock", "chamfer" };
    mDistance = new wxComboBox( mControlPanel, ID_DISTANCE, "chamfer", wxDefaultPosition, wxDefaultSize, 2, distance, wxCB_READONLY );
    ::setColor( mDistance );
    fgs->Add(   mDistance, 0, wxGROW );

    fgs->AddStretchSpacer();

    mExtrapolate = new wxCheckBox( mControlPanel, ID_EXTRAPOLATE, "extrapolate", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mExtrapolate );
    fgs->Add(   mExtrapolate, 0, wxALIGN_LEFT, 0 );

    if (!mCanvas->mCavassData->isBinary()) {
        st->Enable( false );
        mDistance->Enable( false );
        mExtrapolate->Enable( false );
    }

    //row 4
    const int  nDim = mCanvas->mCavassData->getNumberOfDimensions();
    assert( 2<=nDim && nDim<=4 );
    if (nDim==4) {
        s = wxString::Format( "X4 (%.4f):", mCanvas->mCavassData->m_tSpacing );
        st = new wxStaticText( mControlPanel, wxID_ANY, s );
        ::setColor( st );
        fgs->Add( st, 0, wxALIGN_RIGHT );

        s = wxString::Format( "%.4f", mCanvas->mCavassData->m_tSpacing );
        mX4Value = new wxTextCtrl( mControlPanel, ID_X4_VALUE, s );
        ::setColor( mX4Value );
        fgs->Add( mX4Value, 0, wxGROW );

        st = new wxStaticText( mControlPanel, wxID_ANY, "X4:" );
        ::setColor( st );
        fgs->Add( st, 0, wxALIGN_RIGHT );

        mX4Choice = new wxComboBox( mControlPanel, ID_X4_CHOICE, "Linear", wxDefaultPosition, wxDefaultSize, 3, choices, wxCB_READONLY );
        ::setColor( mX4Choice );
        fgs->Add( mX4Choice, 0, wxALIGN_LEFT );

        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
    } else {
        mX4Value  = NULL;
        mX4Choice = NULL;

        st = new wxStaticText( mControlPanel, wxID_ANY, "" );
        ::setColor( st );
        fgs->Add( st, 0, wxALIGN_RIGHT );

        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
        fgs->AddStretchSpacer();
    }
#if 0
    st = new wxStaticText( mControlPanel, wxID_ANY, "output file:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_RIGHT );

    if (mCanvas->mCavassData->isBinary()) {
        mOutput = new wxTextCtrl( mControlPanel, ID_OUTPUT, "intrpl-tmp.BIM" );
    } else {
        mOutput = new wxTextCtrl( mControlPanel, ID_OUTPUT, "intrpl-tmp.IM0" );
    }
    ::setColor( mOutput );
    fgs->Add( mOutput, 0, wxGROW );

    wxButton*  save = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( save );
    fgs->Add( save, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
#endif
    wxButton*  save = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( save );
    fgs->Add( save, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddStretchSpacer();

//    mForeground = new wxCheckBox( mControlPanel, ID_FOREGROUND, "foreground", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
//    mForeground->SetValue( true );
//    ::setColor( mForeground );
//    fgs->Add(   mForeground, 0, wxALIGN_LEFT, 0 );

    fgs->AddStretchSpacer();

//    mParallel = new wxCheckBox( mControlPanel, ID_PARALLEL, "parallel", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
//#ifdef  PARALLEL
//    mParallel->Enable( Preferences::getParallelMode() );
//#else
//    mParallel->Enable( false );
//#endif
//    ::setColor( mParallel );
//    fgs->Add(   mParallel, 0, wxALIGN_LEFT, 0 );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for InterpolateFrame class. */
InterpolateFrame::~InterpolateFrame ( void ) {
    cout << "InterpolateFrame::~InterpolateFrame" << endl;
    wxLogMessage( "InterpolateFrame::~InterpolateFrame" );

    if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }
    //the destruction of the above splitter also causes the destruction of the
    // canvas.  so another destruction of the canvas causes BOOM!
    //if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    //it appears that this is already deleted as well.
    //if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }
    //if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
    mCanvas       = NULL;
    mControlPanel = NULL;
    mBottomSizer  = NULL;
    //if we are in the mode that supports having more than one window open
    // at a time, we need to remove this window from the list.
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
            if (mCanvas!=NULL)  {  delete mCanvas;  mCanvas=NULL;  }
            ::gFrameList.erase( i );
            break;
        }
    }
    //if this window is the last remaining/only window, it is time for us
    // to exit.
    if (::gFrameList.begin() == ::gFrameList.end()) {
        exit(0);
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new InterpolateFrame.  It first searches the input file history for an
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
void InterpolateFrame::createInterpolateFrame ( wxFrame* parentFrame, bool useHistory )
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
    InterpolateFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new InterpolateFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new InterpolateFrame();
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
bool InterpolateFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM", fn, false ))    return true;
    if (wxMatchWild( "*.IM0", fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item. */
void InterpolateFrame::OnOpen ( wxCommandEvent& unused ) {
    //OnInterpolate( unused );
    createInterpolateFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InterpolateFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "InterpolateFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "InterpolateFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK) {
        OnInterpolate( unused );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void InterpolateFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxString  tmp = wxString::Format( "File %s could not be opened.", fname );
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    wxString  tmp = wxString::Format( "%s: ", (const char *)mModuleName.c_str() );
    ++mFileOrDataCount;
    assert( mFileOrDataCount==1 );
    tmp += fname;
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "%s:%s (%d)", (const char *)mModuleName.c_str(), fname, i );
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    InterpolateCanvas*  canvas = dynamic_cast<InterpolateCanvas*>(mCanvas);
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
/** \brief load (and subsequently display) data directly from memory
 *  (instead of from a file).
 *  \todo this is incomplete.
 */
void InterpolateFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp = wxString::Format( "%s: ", (const char *)mModuleName.c_str() );
    tmp += name;
    SetTitle( tmp );
    
    InterpolateCanvas*  canvas = dynamic_cast<InterpolateCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Save button press. */
void InterpolateFrame::OnSave ( wxCommandEvent& unused ) {
    //gather and validate parameters from the various widgets
    double  min=0;
    if (!mMinValue->GetValue().ToDouble(&min)) {
        wxMessageBox( "Please specify a valid min value.",
            "Invalid value", wxICON_ERROR | wxOK );
        return;
    }
    double  max=0;
    if (!mMaxValue->GetValue().ToDouble(&max)) {
        wxMessageBox( "Please specify a valid max value.",
            "Invalid value", wxICON_ERROR | wxOK );
        return;
    }
    double  x1=0;
    if (!mX1Value->GetValue().ToDouble(&x1)) {
        wxMessageBox( "Please specify a valid X1 value.",
            "Invalid value", wxICON_ERROR | wxOK );
        return;
    }
    double  x2=0;
    if (!mX2Value->GetValue().ToDouble(&x2)) {
        wxMessageBox( "Please specify a valid X2 value.",
            "Invalid value", wxICON_ERROR | wxOK );
        return;
    }
    double  x3=0;
    if (!mX3Value->GetValue().ToDouble(&x3)) {
        wxMessageBox( "Please specify a valid X3 value.",
            "Invalid value", wxICON_ERROR | wxOK );
        return;
    }
    double  x4=0;
    if (mX4Value!=NULL) {
        if (!mX4Value->GetValue().ToDouble(&x4)) {
            wxMessageBox( "Please specify a valid X4 value.",
                "Invalid value", wxICON_ERROR | wxOK );
            return;
        }
    }
    const wxString  c1 = mX1Choice->GetValue();
    const wxString  c2 = mX2Choice->GetValue();
    const wxString  c3 = mX3Choice->GetValue();
    wxString  c4;
    if (mX4Choice!=NULL)    c4 = mX4Choice->GetValue();
    const bool  parallel   = mParallel->GetValue();
    const bool  foreground = mForeground->GetValue();

    //get the output file name
    wxFileDialog*  f = NULL;
    if (mCanvas->mCavassData->isBinary()) {
        f = new wxFileDialog( this,
            "Select interpolated output image file", "",
            "intrpl-tmp.BIM", "CAVASS binary files (*.BIM)|*.BIM",
            wxSAVE | wxOVERWRITE_PROMPT );
    } else {
        f = new wxFileDialog( this,
            "Select interpolated output image file", "",
            "intrpl-tmp.IM0", "CAVASS gray files (*.IM0)|*.IM0",
            wxSAVE | wxOVERWRITE_PROMPT );
    }
    assert( f != NULL );
    const int  ret = f->ShowModal();
    if (ret == wxID_CANCEL)    return;
    const wxString  out = f->GetPath();

    //perform the command
    assert( !parallel || mCanvas->mCavassData->getNumberOfDimensions() == 3 );  /** \todo add 4D support */
    wxString  in  = mCanvas->mCavassData->m_fname;
    //always use foreground (even for background jobs) mode because the
    // ProcessDialog below will handle notification.
    if (::equals(in,out)) {
        wxMessageBox( "Input and output file names must be different.",
            "Invalid value", wxICON_ERROR | wxOK );
        return;
    }
    wxString  cmd = "\"";
    cmd += Preferences::getHome()+"/";
    if (parallel) {
        cmd += wxString::Format( "p3dinterpolate\" \"%s\" \"%s\" 0 ", (const char *)in.c_str(), (const char *)out.c_str() );
    } else {
        cmd += wxString::Format( "ndinterpolate\"  \"%s\" \"%s\" 0 ", (const char *)in.c_str(), (const char *)out.c_str() );
    }
    //wxString  cmd = "ndinterpolate " + in + " " + out + " 0 ";
    cmd += wxString::Format( "%f", x1 ) + " "
         + wxString::Format( "%f", x2 ) + " "
         + wxString::Format( "%f", x3 ) + " ";
    if (mX4Choice!=NULL)
        cmd += wxString::Format( "%f ", x4 );
    const wxString  distanceChoice = mDistance->GetValue();
    if (distanceChoice=="chamfer")
        cmd += "1 ";  //method for distance map calculation (binary only; 0=city block, 1=chamfer)
    else
        cmd += "0 ";

    if      (c1=="NearestNeighbor")    cmd += "0 ";
    else if (c1=="Linear")             cmd += "1 ";
    else if (c1=="Cubic")              cmd += "2 ";
    else                               { assert( 0 ); }

    if      (c2=="NearestNeighbor")    cmd += "0 ";
    else if (c2=="Linear")             cmd += "1 ";
    else if (c2=="Cubic")              cmd += "2 ";
    else                               { assert( 0 ); }

    if      (c3=="NearestNeighbor")    cmd += "0 ";
    else if (c3=="Linear")             cmd += "1 ";
    else if (c3=="Cubic")              cmd += "2 ";
    else                               { assert( 0 ); }

    if (mX4Choice!=NULL)
    {
        if      (c4=="NearestNeighbor")    cmd += "0 ";
        else if (c4=="Linear")             cmd += "1 ";
        else if (c4=="Cubic")              cmd += "2 ";
        else                               assert( 0 );
        cmd += "0 ";
        cmd += wxString::Format( "%d ", mCanvas->mCavassData->m_tSize-1 );
    }

    for (int v=0; v<mCanvas->mCavassData->m_tSize; v++)
        cmd += wxString::Format( "%f ", (min-1) )
             + wxString::Format( "%f ", (max-1) );

    cout << cmd << endl;
    wxLogMessage( "command=%s", (const char *)cmd.c_str() );
    //system( cmd.c_str() );
    if (parallel) {
        /** \todo come up with a more inteligent way to come up with an spc value. */
        cmd += " 20";  //slices-per-chunk (spc)
        ParallelProcessManager  p( "interpolate started", cmd, foreground );
    } else {
        ProcessManager  p( "interpolate started", cmd, foreground );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void InterpolateFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData   printDialogData( *g_printData );
    InterpolateCanvas*  canvas = dynamic_cast<InterpolateCanvas*>(mCanvas);
    wxPrintPreview*     preview = new wxPrintPreview(
        new MainPrint(wxString("CAVASS").c_str(), canvas),
        new MainPrint(wxString("CAVASS").c_str(), canvas),
        &printDialogData );
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    wxPreviewFrame*  frame = new wxPreviewFrame( preview, this,
        _T("CAVASS Print Preview"), wxPoint(100, 100), wxSize(600, 650) );
    frame->Centre( wxBOTH );
    frame->Initialize();
    frame->Show();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing. */
void InterpolateFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData   printDialogData( *g_printData );
    wxPrinter           printer( &printDialogData );
    InterpolateCanvas*  canvas = dynamic_cast<InterpolateCanvas*>(mCanvas);
    MainPrint           printout( mModuleName.c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( InterpolateFrame, wxFrame )
BEGIN_EVENT_TABLE       ( InterpolateFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_SAVE,   InterpolateFrame::OnSave   )
END_EVENT_TABLE()
//======================================================================
