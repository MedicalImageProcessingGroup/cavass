/*
  Copyright 1993-2018, 2022 Medical Image Processing Group
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
 * \file   Owen2dFrame.cpp
 * \brief  Owen2dFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "wx/html/htmlwin.h"

#define s2dControlsHeight ((buttonHeight+1)*8+50)

extern Vector  gFrameList;

char Owen2dFeatureNames[6][15]={"Higher Density", "Lower Density",
	"Gradient 1", "Gradient 2", "Gradient 3", "Gradient 4"};
char Owen2dTransformNames[6][16]={"Linear", "Gaussian", "Inv. Linear",
	"Inv. Gaussian", "Hyperbolic", "Inv. Hyperbolic"};

//----------------------------------------------------------------------
/** \brief Constructor for Owen2dFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
Owen2dFrame::Owen2dFrame ( bool maximize, int w, int h )
    : MainFrame( 0 )
{
    //init the types of input files that this app can accept
    mFileNameFilter     = (char *)"CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0";
    mFileOrDataCount    = 0;
	out_object          = 0;
    mGrayMapControls    = NULL;
	mSetIndexControls   = NULL;
	mSetOutputControls  = NULL;
    mModuleName         = "CAVASS:Owen2d";
    mSaveScreenControls = NULL;

	m_prev = NULL;
	m_next = NULL;
	m_setIndex = NULL;
	m_grayMap = NULL;
	mMode = NULL;
	m_reset = NULL;
	m_object = NULL;
	m_deleteObject = NULL;
	m_loadObject = NULL;
	m_defaultFill = NULL;
	m_overlay = NULL;
	m_layout = NULL;
	m_setOutputBut = NULL;
	m_buttonBox = NULL;
	m_buttonSizer = NULL;
	fgs = NULL;
	mAuxControls = NULL;
	modeName[Owen2dCanvas::LWOF] = "LWOF";
	modeName[Owen2dCanvas::TRAINING] = "Train";
	modeName[Owen2dCanvas::PAINT] = "Paint";
	modeName[Owen2dCanvas::ILW] = "ILW";
	modeName[Owen2dCanvas::LSNAKE] = "LiveSnake";
	modeName[Owen2dCanvas::SEL_FEATURES] = "SelFeatures";
	modeName[Owen2dCanvas::REVIEW] = "Review";
	modeName[Owen2dCanvas::REPORT] = "Report";
	modeName[Owen2dCanvas::PEEK] = "Peek";
	FILE *objnamfp=fopen("object_names.spec", "r");
	for (int j=0; j<8; j++)
	{
		char buf[100];
		objectName[j] = "";
		if (objnamfp && !feof(objnamfp) && fgets(buf, sizeof(buf), objnamfp))
			objectName[j] = wxString(buf);
		objectName[j].Replace("\n", "");
		if (objectName[j] == "")
			objectName[j] = wxString::Format("%d", j+1);
	}
	objectName[8] = "All";
	if (objnamfp)
		fclose(objnamfp);

    //if we are in the mode that supports having more than one window open
    // at a time, we need to add this window to the list.
    ::gFrameList.push_back( this );
    if (!Preferences::getSingleFrameMode()) {
        gWhere += cWhereIncr;
        if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    }
    
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -s2dControlsHeight );
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via Owen2dCanvas
    mCanvas = tCanvas = new Owen2dCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );

    if (Preferences::getCustomAppearance()) {
        mCanvas->SetBackgroundColour( wxColour(DkBlue) );
        mCanvas->SetForegroundColour( wxColour(Yellow) );
    }
    wxSizer*  topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize( 700, 400 );
    topSizer->Add( mCanvas, 1, wxGROW );

    //bottom of window contains controls
    mControlPanel = new wxPanel( mSplitter, -1, wxDefaultPosition, wxDefaultSize );
    ::setColor( mControlPanel );
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -s2dControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	mAuxControls = new Owen2dAuxControls( mControlPanel,
		mBottomSizer, (modeName[canvas->detection_mode]+" Controls").c_str(),
		canvas->paint_brush_size );
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );
    mSplitter->SetSashPosition( -s2dControlsHeight );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    wxToolTip::Enable( Preferences::getShowToolTips() );
    mSplitter->SetSashPosition( -s2dControlsHeight );
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
void Owen2dFrame::initializeMenu ( void ) {
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
    wxString  tmp = wxString::Format( "%s:%d", (const char *)mModuleName.c_str(), j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );

    //enable the Open menu item
    wxMenuItem*  op = mFileMenu->FindItem( ID_OPEN );
    op->Enable( true );

    wxMenuBar* mb = GetMenuBar();
    wxMenuItem* mi = mb->FindItem( ID_HELP );
    mi->Enable();
    Bind( wxEVT_COMMAND_MENU_SELECTED, Owen2dFrame::OnHelp, ID_HELP );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::OnHelp ( wxCommandEvent& e ) {
    wxDialog dlg( NULL, wxID_ANY, wxString(_("Owen2d help")), wxDefaultPosition,
                  wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    wxHtmlWindow* html = new wxHtmlWindow( &dlg, wxID_ANY, wxDefaultPosition ); //,
                                           //wxSize(600, 400) ); //, wxHW_SCROLLBAR_NEVER );
    html->SetInitialSize( wxSize(600,400) );

    //set the source of the html help
    wxString h( "file://" );
    h += Preferences::getHome();
    h += "/help/";
#ifdef WIN32
    h += "owen2dframe-win.html" );
#else
    h += "owen2dframe-mac-linux.html";
#endif
    if (!html->LoadPage(h)) {
        h += " not found.";
        html->SetPage( h );
    } else {
        printf( "loaded %s. \n", (const char*)h.c_str() );
    }

    sizer->Add( html, 1, wxALL | wxEXPAND, 10 );
    wxButton* but = new wxButton( &dlg, wxID_OK, _("OK") );
    but->SetDefault();
    sizer->Add( but, 0, wxALL | wxALIGN_RIGHT, 15 );
    dlg.SetSizer( sizer );
    sizer->Fit( &dlg );
    dlg.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right. */
void Owen2dFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    m_buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    //row 1, col 1
    m_prev = new wxButton( mControlPanel, ID_PREVIOUS, "Previous", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_prev );
    fgs->Add( m_prev, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
    m_next = new wxButton( mControlPanel, ID_NEXT, "Next", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_next );
    fgs->Add( m_next, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
    m_setIndex = new wxButton( mControlPanel, ID_SET_INDEX, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_setIndex );
    fgs->Add( m_setIndex, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    m_grayMap = new wxButton( mControlPanel, ID_GRAYMAP, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_grayMap );
    fgs->Add( m_grayMap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3
	mModeLabel = new wxStaticText( mControlPanel, wxID_ANY, "Mode:" );
	fgs->Add( mModeLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	wxArrayString sa;
	for (int j=0; j<canvas->num_detection_modes; j++)
		sa.Add(modeName[canvas->detection_modes[j]]);
	mMode = new wxComboBox( mControlPanel, ID_MODE, modeName[canvas->detection_mode], wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
	::setColor( mMode );
    fgs->Add( mMode, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    m_reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_reset );
    fgs->Add( m_reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 4, col 2
	m_defaultFill = new wxCheckBox( mControlPanel, ID_DEFAULT_FILL, "Default Fill" );
	::setColor( m_defaultFill );
	m_defaultFill->SetValue( canvas->default_mask_value!=0 );
	fgs->Add( m_defaultFill, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 5
	m_objectLabel = new wxStaticText( mControlPanel, wxID_ANY, "Object:" );
	fgs->Add( m_objectLabel, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	sa.Clear();
	for (int j=0; j<9; j++)
		sa.Add(objectName[j]);
	m_object = new wxComboBox( mControlPanel, ID_OBJECT, objectName[canvas->object_number], wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
	::setColor( m_object );
	fgs->Add( m_object, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 6, col 1
	m_deleteObject = new wxButton( mControlPanel, ID_DELETE_OBJECT, "Del. Obj.", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( m_deleteObject );
	fgs->Add( m_deleteObject, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 6, col 2
	m_loadObject = new wxButton( mControlPanel, ID_LOAD_OBJECT, "Load Obj.", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( m_loadObject );
	fgs->Add( m_loadObject, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 1
	m_setOutputBut = new wxButton( mControlPanel, ID_SET_OUTPUT, "Set Output", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( m_setOutputBut );
    fgs->Add( m_setOutputBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 2
	m_layout = new wxCheckBox( mControlPanel, ID_LAYOUT, "Layout" );
	::setColor( m_layout );
	m_layout->SetValue( canvas->layout_flag );
	fgs->Add( m_layout, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 8, col 1
	m_overlay = new wxCheckBox( mControlPanel, ID_OVERLAY, "Overlay" );
	::setColor( m_overlay );
	m_overlay->SetValue( canvas->overlay_flag );
	fgs->Add( m_overlay, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    m_buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( m_buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for Owen2dFrame class. */
Owen2dFrame::~Owen2dFrame ( void ) {
    cout << "Owen2dFrame::~Owen2dFrame" << endl;
    wxLogMessage( "Owen2dFrame::~Owen2dFrame" );

    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
	if (mAuxControls)
		delete mAuxControls, mAuxControls = NULL;
    deleteOwen2dControls();
	if (m_overlay!=NULL)
	{
		fgs->Detach(m_overlay);
		delete m_overlay;
		m_overlay = NULL;
	}
	if (m_layout!=NULL)
	{
		fgs->Detach(m_layout);
		delete m_layout;
		m_layout = NULL;
	}
	if (m_setOutputBut!=NULL)
	{
		fgs->Detach(m_setOutputBut);
		delete m_setOutputBut;
		m_setOutputBut = NULL;
	}
	if (m_reset!=NULL)
	{
		fgs->Detach(m_reset);
		delete m_reset;
		m_reset = NULL;
	}
	if (mMode!=NULL)
	{
		fgs->Detach(mMode);
		delete mMode;
		mMode = NULL;
	}
	if (m_grayMap!=NULL)
	{
		fgs->Detach(m_grayMap);
		delete m_grayMap;
		m_grayMap = NULL;
	}
	if (m_setIndex!=NULL)
	{
		fgs->Detach(m_setIndex);
		delete m_setIndex;
		m_setIndex = NULL;
	}
	if (m_next!=NULL)
	{
		fgs->Detach(m_next);
		delete m_next;
		m_next = NULL;
	}
	if (m_prev!=NULL)
	{
		fgs->Detach(m_prev);
		delete m_prev;
		m_prev = NULL;
	}
	if (fgs!=NULL)
	{
		m_buttonSizer->Remove(fgs);
		fgs = NULL;
	}
	if (m_buttonSizer!=NULL)
	{
		mBottomSizer->Remove(m_buttonSizer);
		m_buttonSizer = NULL;
	}
    if (mControlPanel!=NULL)  {
		mBottomSizer->Detach(mControlPanel);
		delete mControlPanel;
		mControlPanel=NULL;
	}
    if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }
    //if we are in the mode that supports having more than one window open
    // at a time, we need to remove this window from the list.
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new Owen2dFrame.  It first searches the input file history for an
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
void Owen2dFrame::createOwen2dFrame ( wxFrame* parentFrame, bool useHistory )
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
                wxString tmp= wxString::Format( "File %s could not be opened.",
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
#if 0
	/* Save last modification */
	MainFrame *mFrame = dynamic_cast<MainFrame*>(parentFrame);
	if (mFrame)
		mFrame->flush_temp_data();
#endif
    //display a Owen2d frame using the specified file as input
    Owen2dFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new Owen2dFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new Owen2dFrame();
    }
    frame->loadFile( filename.c_str() );
    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This function determines if the given filename is of a type
 *  that can be read by this module/app.
 *
 *  Supported file extensions: BIM, IM0.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool Owen2dFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::flush_temp_data( void )
{
	Owen2dCanvas *canvas=dynamic_cast<Owen2dCanvas*>(mCanvas);
	if (canvas)
		canvas->save_mask_proc(0);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item. */
void Owen2dFrame::OnOpen ( wxCommandEvent& unused ) {
    createOwen2dFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "Owen2dFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "Owen2dFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0) {
        OnOwen2d( unused );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void Owen2dFrame::loadFile ( const char* const fname ) {
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
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->loadFile( fname );

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
void Owen2dFrame::loadData ( char* name,
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
    
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::deleteOwen2dControls(void)
{
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mGrayMapControls!=NULL)  { delete mGrayMapControls;   mGrayMapControls=NULL;  }
    if (mSetIndexControls!=NULL) {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
	if (mSetOutputControls!=NULL) {
		delete mSetOutputControls;
		mSetOutputControls = NULL;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Previous button press.  display the previous slice.
 */
void Owen2dFrame::OnPrevious ( wxCommandEvent& unused ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) - 1;
    if (slice<0)
		slice = canvas->getNoSlices(0) - 1;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Next button press.  display the next slice. */
void Owen2dFrame::OnNext ( wxCommandEvent& unused ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) + 1;
    if (slice >= canvas->getNoSlices(0))
		slice = 0;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::OnSetIndex ( wxCommandEvent& unused ) {
    if (mSetIndexControls!=NULL) {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
        return;
    }
	deleteOwen2dControls();

    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    mSetIndexControls = new SetIndexControls( mControlPanel,
	    mBottomSizer, "SetIndex", canvas->getSliceNo(0),
		canvas->getNoSlices(0), ID_SLICE_SLIDER, ID_SCALE_SLIDER,
		canvas->getScale(), ID_LABELS, wxID_ANY );
}

void Owen2dFrame::OnReset( wxCommandEvent& unused )
{
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);

	int iw, ih;
	canvas->GetSize(&iw, &ih);
	switch (canvas->detection_mode)
	{
	  case Owen2dCanvas::SEL_FEATURES:
		canvas->Reset_training_proc(2);
		if (canvas->detection_mode == Owen2dCanvas::SEL_FEATURES)
		delete mAuxControls;
		mAuxControls = new Owen2dAuxControls( mControlPanel,
			mBottomSizer, "Feature Controls", canvas->Range,
			canvas->curr_feature,
			canvas->temp_list[canvas->curr_feature].status,
			canvas->temp_list[canvas->curr_feature].transform,
			canvas->temp_list[canvas->curr_feature].weight,
			canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
			canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
			canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
			canvas->temp_list[canvas->curr_feature].rmax*canvas->Range );
		canvas->mTx_f =
		    (int)(iw/2-canvas->getScale()*canvas->mCavassData->m_xSize);
		canvas->mTy_f = 0;
		canvas->Reset_training_proc(1);
		break;
	  case Owen2dCanvas::TRAINING:
	    canvas->mTx =
	       (int)(iw-canvas->getScale()*canvas->mCavassData->m_xSize)/2;
	    canvas->mTy = 0;
		break;
	  case Owen2dCanvas::REPORT:
	    canvas->mTx_p = canvas->mTy_p = 0;
		break;
	  case Owen2dCanvas::REVIEW:
	    canvas->mTx_v = canvas->mTy_v = 0;
		break;
	  default:
		canvas->ResetPeekPoints();
		if (canvas->phase==0 && canvas->ilw_control_points[0])
		{
			canvas->ilw_control_points[0] = canvas->ilw_control_points[1] = 0;
			free(canvas->ilw_control_point[0]);
			if (canvas->ilw_control_slice[0] != canvas->ilw_control_slice[1])
				free(canvas->ilw_control_point[1]);
			canvas->ilw_control_point[0] = canvas->ilw_control_point[1] = NULL;
			canvas->ilw_control_slice[0] = -1;
			canvas->ilw_control_slice[1] = -2;
			canvas->dp_anchor_points = 0;
		}
		canvas->mTx = (int)(iw-canvas->getScale()*canvas->mCavassData->m_xSize)/2;
		canvas->mTy = 0;
	}
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 */
void Owen2dFrame::OnGrayMap ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
        return;
    }
    deleteOwen2dControls();
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void Owen2dFrame::OnCenterSlider ( wxScrollEvent& e ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider (used to change contrast). */
void Owen2dFrame::OnWidthSlider ( wxScrollEvent& e ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void Owen2dFrame::OnCTLung ( wxCommandEvent& unused ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTLungCenter() &&
			canvas->getWidth(0)==Preferences::getCTLungWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTLungCenter() );
	canvas->setWidth( 0, Preferences::getCTLungWidth() );
	canvas->setInvert( 0, false );
	mGrayMapControls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void Owen2dFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTSoftTissueCenter() &&
			canvas->getWidth(0)==Preferences::getCTSoftTissueWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 0, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 0, false );
	mGrayMapControls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void Owen2dFrame::OnCTBone ( wxCommandEvent& unused ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTBoneCenter() &&
			canvas->getWidth(0)==Preferences::getCTBoneWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTBoneCenter() );
	canvas->setWidth( 0, Preferences::getCTBoneWidth() );
	canvas->setInvert( 0, false );
	mGrayMapControls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void Owen2dFrame::OnPET ( wxCommandEvent& unused ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getPETCenter() &&
			canvas->getWidth(0)==Preferences::getPETWidth() &&
			canvas->getInvert(0))
	    return;  //no change
    canvas->setCenter( 0, Preferences::getPETCenter() );
	canvas->setWidth( 0, Preferences::getPETWidth() );
	canvas->setInvert( 0, true );
	mGrayMapControls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
/** \brief callback for center slider (used to change contrast).
 *  especially (only) need on X11 (w/out GTK) to get slider events.
 */
void Owen2dFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& unused ) {
    if (m_center->GetValue() == mCanvas->getCenter())    return;
    mCanvas->setCenter( m_center->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
/** \brief callback for width slider (used to change contrast).
 *  especially (only) need on X11 (w/out GTK) to get slider events.
 */
void Owen2dFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& unused ) {
    if (m_width->GetValue() == mCanvas->getWidth())    return;
    mCanvas->setWidth( m_width->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set  */
void Owen2dFrame::OnSliceSlider ( wxScrollEvent& e ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set  */
void Owen2dFrame::OnScaleSlider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndexControls->setScaleText( s );
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for labels checkbox */
void Owen2dFrame::OnLabels ( wxCommandEvent& e ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->setLabels( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox */
void Owen2dFrame::OnOverlay ( wxCommandEvent& e ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->overlay_flag = e.IsChecked();
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for layout checkbox */
void Owen2dFrame::OnLayout ( wxCommandEvent& e ) {
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->layout_flag = e.IsChecked();
    canvas->reload();
	if (canvas->layout_flag && mSetIndexControls==NULL)
		OnSetIndex(e);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set  */
void Owen2dFrame::OnInvert ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void Owen2dFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new MainPrint(wxString("CAVASS").c_str(), canvas), new MainPrint(wxString("CAVASS").c_str(), canvas),
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
/** \brief callback for printing. */
void Owen2dFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    Owen2dCanvas*     canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
    MainPrint          printout( mModuleName.c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}

void Owen2dFrame::OnOutputType ( wxCommandEvent& unused )
{
	Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	if (canvas->output_type == BINARY)
		canvas->output_type = GREY;
	else if (canvas->output_type == GREY)
		canvas->output_type = GREY+1;
	else
		canvas->output_type = BINARY;
	mSetOutputControls->SetOutType(canvas->output_type);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::OnSetOutput ( wxCommandEvent& unused )
{
	if (mSetOutputControls!=NULL) {
		delete mSetOutputControls;
		mSetOutputControls = NULL;
		return;
	}
	deleteOwen2dControls();
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	mSetOutputControls = new SetOwen2dOutputControls( mControlPanel,
		mBottomSizer, "Set Output", ID_SAVE,
		ID_OUT_OBJECT, (out_object+1)%9, canvas->output_type );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SetOwen2dOutputControls::SetOutType( int newOutType )
{
	switch (newOutType)
	{
		case BINARY:
			m_outputType->SetLabel("Type: BIM");
			break;
		case GREY:
			m_outputType->SetLabel("Type: IM0");
			break;
		case GREY+1:
			m_outputType->SetLabel("Type: PAR");
			break;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SetOwen2dOutputControls::SetOwen2dOutputControls( wxPanel* cp,
	wxSizer* bottomSizer, const char* const title, int saveID,
	int outputObjectID, int currentOutObject, int currentOutType,
	int outputTypeID )
{
    mBottomSizer = bottomSizer;
    mSetOutputBox = new wxStaticBox( cp, -1, title );
    ::setColor( mSetOutputBox );
    mSetOutputSizer = new wxStaticBoxSizer( mSetOutputBox, wxHORIZONTAL );
    mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
    mFgs->SetMinSize( controlsWidth, 0 );
    mFgs->AddGrowableCol( 0 );
	mFgsButton = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap

	st = new wxStaticText( cp, wxID_ANY, "Out Object:" );
	::setColor( st );
	mFgsButton->Add( st, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	wxString objNam = "All";
	wxArrayString as(1, &objNam);
	FILE *objnamfp=fopen("object_names.spec", "r");
	for (int j=1; j<=8; j++)
	{
		char buf[100];
		objNam = "";
		if (objnamfp && !feof(objnamfp) && fgets(buf, sizeof(buf), objnamfp))
			objNam = wxString(buf);
		objNam.Replace("\n", "");
		if (objNam == "")
			objNam = wxString::Format("%d", j);
		as.Add(objNam);
	}
	if (objnamfp)
		fclose(objnamfp);
	m_outputObject = new wxComboBox(cp, Owen2dFrame::ID_OUT_OBJECT,
		as[currentOutObject],
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		as, wxCB_READONLY );
    ::setColor( m_outputObject );
	mFgsButton->Add( m_outputObject, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    m_outputType = new wxButton( cp, outputTypeID,
		currentOutType==BINARY? "Type: BIM": "Type: IM0", wxDefaultPosition,
		wxSize(buttonWidth,buttonHeight) );
	::setColor( m_outputType );
	mFgsButton->Add( m_outputType, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	m_saveBut = new wxButton( cp, Owen2dFrame::ID_SAVE, "Save",
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_saveBut );
    mFgsButton->Add( m_saveBut, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

	mFgs->Add( mFgsButton, 0, wxGROW|wxALL, 10 );
    mSetOutputSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
	mBottomSizer->Prepend( mSetOutputSizer, 0, wxGROW|wxALL, 10 );
	mBottomSizer->Layout();
	cp->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Owen2dFrame::OnOwen2dSave(wxCommandEvent &e)
{
    Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	switch (canvas->output_type)
	{
		case BINARY:
			{
		      wxFileDialog saveDlg(this,
                      _T("Save file"),
                      wxEmptyString,
                      wxString::Format("intr2d-tmp%d.BIM", (out_object+1)%9),
                      _T("BIM files (*.BIM)|*.BIM|IM0 files (*.IM0)|*.IM0"),
                      wxSAVE|wxOVERWRITE_PROMPT);
			  if (saveDlg.ShowModal() == wxID_OK) {
			      canvas->generate_masked_scene((const char *)saveDlg.GetPath().c_str(),
				      out_object);
			  }
			}
			break;
		case GREY:
			{
		      wxFileDialog saveDlg(this,
                      _T("Save file"),
                      wxEmptyString,
                      wxString::Format("intr2d-tmp%d.IM0", (out_object+1)%9),
                      _T("IM0 files (*.IM0)|*.IM0|BIM files (*.BIM)|*.BIM"),
                      wxSAVE|wxOVERWRITE_PROMPT);
			  if (saveDlg.ShowModal() == wxID_OK) {
			      canvas->generate_masked_scene((const char *)saveDlg.GetPath().c_str(),
				      out_object);
			  }
			}
			break;
		case GREY+1:
			{
			  if (out_object < 8)
			  	  wxMessageBox(wxString::Format(
				      "Currently accepted parameters for object %d will be ",
					  out_object+1)+
					  "stored for all objects.  To save all object parameters"+
					  " select all objects.");
			  wxFileDialog saveDlg(this,
			          _T("Save parameters"),
					  wxEmptyString,
					  "intr2d.PAR",
					  _T("PAR files (*.PAR)|*.PAR"),
					  wxSAVE|wxOVERWRITE_PROMPT);
			  if (saveDlg.ShowModal() == wxID_OK) {
			      FILE *fp=fopen((const char *)saveDlg.GetPath().c_str(), "wb");
				  if (fp == NULL)
				  {
				      wxMessageBox("Could not create file.");
					  return;
				  }
				  fprintf(fp, "%d\n", MAX_NUM_FEATURES);

				  for (int j=0; j<8; j++)
				  {
				      int k=out_object==8? j: out_object;
				      for(int i=0; i<MAX_NUM_FEATURES; i++)
					      fprintf(fp, "%d %d %f %f %f %f %f\n",
				              canvas->accepted_list[k][i].status,
							  canvas->accepted_list[k][i].transform,
				              canvas->accepted_list[k][i].weight,
							  canvas->accepted_list[k][i].rmin,
				              canvas->accepted_list[k][i].rmax,
							  canvas->accepted_list[k][i].rmean,
				              canvas->accepted_list[k][i].rstddev);
				  }
				  fclose(fp);
			  }
			}
			break;
	}
}

void Owen2dFrame::OnMode( wxCommandEvent& e )
{
	Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	if (mAuxControls)
		delete mAuxControls, mAuxControls=NULL;
	canvas->ResetPeekPoints();
	if (canvas->detection_mode == Owen2dCanvas::TRAINING)
	{
		if (canvas->training_phase == 1)
		{
	    	canvas->NumPoints=0;
			if (canvas->temp_contours.last<=0 && canvas->o_contour.last>=0)
			{
				canvas->copy_ocontour_into_temp_array();
				canvas->o_contour.last = -1;
			}
		}
		canvas->training_phase = 0;
		canvas->Run_Statistics();
	}
	bool was_review = canvas->detection_mode==Owen2dCanvas::REVIEW;
	for (canvas->detection_mode=1; canvas->detection_mode<
			Owen2dCanvas::ROI; canvas->detection_mode++)
		if (modeName[canvas->detection_mode] == e.GetString())
			break;
	if (canvas->detection_mode == Owen2dCanvas::REVIEW)
	{
		delete mSetIndexControls, mSetIndexControls = NULL;
		canvas->save_mask_proc(0);
		canvas->setScale(canvas->getScale());
	}
	else if (canvas->detection_mode==Owen2dCanvas::SEL_FEATURES ||
			canvas->mCols>1)
	{
		canvas->freeImagesAndBitmaps();
		canvas->mCols =
			canvas->detection_mode==Owen2dCanvas::SEL_FEATURES? 2:1;
		m_loadObject->SetLabel("Load Obj.");
	}
	if (was_review)
	{
		delete mSetIndexControls, mSetIndexControls = NULL;
		canvas->save_mask_proc(0);
		canvas->object_mask_slice_index =
			canvas->sl.slice_index[0][canvas->mCavassData->m_sliceNo];
		canvas->load_object_mask(canvas->object_mask_slice_index);
		canvas->setScale(canvas->getScale());
	}
	if (canvas->painting && (canvas->detection_mode==Owen2dCanvas::ILW ||
			canvas->detection_mode==Owen2dCanvas::LSNAKE))
	{
		canvas->Reset_training_proc(0);
		canvas->painting = false;
	}
	switch (canvas->detection_mode)
	{
		case Owen2dCanvas::TRAINING:
			mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, (modeName[canvas->detection_mode]+" Controls").c_str(),
				canvas->train_brush_size, true );
			break;
		case Owen2dCanvas::PAINT:
			mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, (modeName[canvas->detection_mode]+" Controls").c_str(),
				canvas->paint_brush_size );
			break;
		case Owen2dCanvas::ILW:
			mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, "ILW Controls", canvas->ilw_iterations,
				canvas->ilw_min_pts );
			break;
		case Owen2dCanvas::LSNAKE:
			mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, "LSnake Controls", canvas->ilw_iterations,
				canvas->lsnake_alpha,canvas->lsnake_beta,canvas->lsnake_gamma);
			break;
		case Owen2dCanvas::SEL_FEATURES:
			mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, "Feature Controls", canvas->Range,
				canvas->curr_feature,
				canvas->temp_list[canvas->curr_feature].status,
				canvas->temp_list[canvas->curr_feature].transform,
				canvas->temp_list[canvas->curr_feature].weight,
				canvas->temp_list[canvas->curr_feature].rmean*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rstddev*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rmin*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rmax*
					canvas->Range );
			m_loadObject->SetLabel("Load Par.");
			break;
	}
	canvas->reload();
}

Owen2dAuxControls::Owen2dAuxControls( wxPanel* cp, wxSizer* bottomSizer,
	const char * const title, int currentBrushSize, bool update_flag)
{
	setup_fgs(cp, bottomSizer, title, 2);
	static const char sa[][5]={"X", "1", "2", "5", "10", "15", "20", "25",
		"30", "35", "40", "50", "60", "Fill"};
	int ii=0;
	assert(currentBrushSize <= atoi(sa[sizeof(sa)/sizeof(sa[0])-2]));
	if (currentBrushSize < 0)
		ii = sizeof(sa)/sizeof(sa[0])-1;
	else if (currentBrushSize)
		for (ii=1; atoi(sa[ii])<currentBrushSize; ii++)
			;
	wxArrayString as;
	for (unsigned int j=0; j<sizeof(sa)/sizeof(sa[0]); j++)
		as.Add(sa[j]);
	mSt1 = new wxStaticText( cp, wxID_ANY, "Brush Size:" );
	::setColor( mSt1 );
	mFgsButton->Add( mSt1, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mBrushSize = new wxComboBox(cp, Owen2dFrame::ID_BRUSH_SIZE,
		currentBrushSize==0? wxString("X"):
		currentBrushSize<0? wxString("Fill"):
		wxString::Format("%d", currentBrushSize),
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		as, wxCB_READONLY );
	::setColor( mBrushSize );
	mFgsButton->Add( mBrushSize, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	if (update_flag)
	{
		mUpdate = new wxButton( cp, Owen2dFrame::ID_FEATURE_UPDATE,
			"Update" );
		::setColor( mUpdate );
		mFgsButton->Add( mUpdate, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	}
	display_fgs();
}

Owen2dAuxControls::Owen2dAuxControls( wxPanel* cp, wxSizer* bottomSizer,
	const char * const title, int featureRange,
	int currentFeature, int currentFeatureStatus, int currentTransform,
	double currentWeight, double currentMean, double currentStdDev,
	double currentFeatureMin, double currentFeatureMax )
{
	setup_fgs(cp, bottomSizer, title, 3);
	mFgsSliders = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
	mSt1 = new wxStaticText( cp, wxID_ANY, "Feature:" );
	::setColor( mSt1 );
	mFgsButton->Add( mSt1, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	wxArrayString as1;
	for (unsigned int j=0; j<sizeof(Owen2dFeatureNames)/
			sizeof(Owen2dFeatureNames[0]); j++)
		as1.Add(Owen2dFeatureNames[j]);
	mFeature = new wxComboBox(cp, Owen2dFrame::ID_FEATURE,
		Owen2dFeatureNames[currentFeature],
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		as1, wxCB_READONLY );
	::setColor( mFeature );
	mFgsButton->Add( mFeature, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mFeatureStatus = new wxCheckBox( cp, Owen2dFrame::ID_FEATURE_STATUS,
		"Feature On" );
	::setColor( mFeatureStatus );
	mFeatureStatus->SetValue( 0!=currentFeatureStatus );
	mFgsButton->Add( mFeatureStatus, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mSt2 = new wxStaticText( cp, wxID_ANY, "Transform:" );
	::setColor( mSt2 );
	mFgsButton->Add( mSt2, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	wxArrayString as2;
	for (unsigned int j=0; j<sizeof(Owen2dTransformNames)/
			sizeof(Owen2dTransformNames[0]); j++)
		as2.Add(Owen2dTransformNames[j]);
	mTransform = new wxComboBox(cp, Owen2dFrame::ID_TRANSFORM,
		Owen2dTransformNames[currentTransform],
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		as2, wxCB_READONLY );
	::setColor( mTransform );
	mFgsButton->Add( mTransform, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mUpdate = new wxButton( cp, Owen2dFrame::ID_FEATURE_UPDATE, "Update" );
	::setColor( mUpdate );
	mFgsButton->Add( mUpdate, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	SetFeature(currentFeature, 0!=currentFeatureStatus, currentTransform,
		featureRange, currentWeight,
		currentMean, currentStdDev, currentFeatureMin, currentFeatureMax);
	display_fgs();
}

void Owen2dFrame::OnObject( wxCommandEvent& e )
{
	Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->object_number =
		e.GetString()=="All"? 8: atoi((const char *)e.GetString().c_str())-1;
	for (int j=0; j<6; j++)
		canvas->temp_list[j]=canvas->accepted_list[canvas->object_number%8][j];
	canvas->ReCalc_Edge_Costs();
	canvas->reload();
	if (canvas->detection_mode == Owen2dCanvas::SEL_FEATURES)
	{
		delete mAuxControls;
		mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, "Feature Controls", canvas->Range,
				canvas->curr_feature,
				canvas->temp_list[canvas->curr_feature].status,
				canvas->temp_list[canvas->curr_feature].transform,
				canvas->temp_list[canvas->curr_feature].weight,
				canvas->temp_list[canvas->curr_feature].rmean*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rstddev*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rmin*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rmax*
					canvas->Range );		
	}
}

void Owen2dFrame::OnDeleteObject( wxCommandEvent& unused )
{
	Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->Reset_object_proc();
}

void Owen2dFrame::OnLoadObject( wxCommandEvent& unused )
{
	Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	if (canvas->detection_mode == Owen2dCanvas::SEL_FEATURES)
	{
		wxString filename = wxFileSelector( _T("Select parameter file"),
			_T(""), _T(""), _T(""), "PAR files (*.PAR)|*.PAR",
			wxFILE_MUST_EXIST );
		if (!filename)
			return;
		canvas->LoadFeatureList((const char *)filename.c_str());
		canvas->reload();
		delete mAuxControls;
		mAuxControls = new Owen2dAuxControls( mControlPanel,
				mBottomSizer, "Feature Controls", canvas->Range,
				canvas->curr_feature,
				canvas->temp_list[canvas->curr_feature].status,
				canvas->temp_list[canvas->curr_feature].transform,
				canvas->temp_list[canvas->curr_feature].weight,
				canvas->temp_list[canvas->curr_feature].rmean*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rstddev*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rmin*
					canvas->Range,
				canvas->temp_list[canvas->curr_feature].rmax*
					canvas->Range );		
		return;
	}
	canvas->load_object_proc();
}

void Owen2dFrame::OnDefaultFill( wxCommandEvent& e )
{
	Owen2dCanvas*  canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->default_mask_value = e.IsChecked()? 255:0;
}

void Owen2dFrame::OnOutputObject( wxCommandEvent& e )
{
	out_object = e.GetString()=="All"? 8: atoi((const char *)e.GetString().c_str())-1;
}

void Owen2dFrame::OnBrushSize( wxCommandEvent& e )
{
	char brush[5];
	strncpy(brush, (const char *)e.GetString().c_str(), 5);
	assert(brush[4] == 0);
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	if (canvas->detection_mode == Owen2dCanvas::TRAINING)
	{
		if (strcmp(brush, "X") == 0)
			canvas->train_brush_size = 0;
		else if (strcmp(brush, "Fill") == 0)
			canvas->train_brush_size = -1;
		else
			canvas->train_brush_size = atoi(brush);
	}
	else
	{
		if (strcmp(brush, "X") == 0)
			canvas->paint_brush_size = 0;
		else if (strcmp(brush, "Fill") == 0)
			canvas->paint_brush_size = -1;
		else
			canvas->paint_brush_size = atoi(brush);
	}
}

void Owen2dFrame::OnIterates( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	if (canvas->detection_mode == Owen2dCanvas::ILW)
		canvas->ilw_iterations = atoi((const char *)e.GetString().c_str());
	else if (canvas->detection_mode == Owen2dCanvas::LSNAKE)
		for (canvas->ilw_iterations=1; canvas->ilw_iterations<6?
				canvas->ilw_iterations<atoi((const char *)e.GetString().c_str()):
				(canvas->ilw_iterations-4)*5<atoi((const char *)e.GetString().c_str()); )
			canvas->ilw_iterations++;
}

void Owen2dFrame::OnMinPoints( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->ilw_min_pts = atoi((const char *)e.GetString().c_str());
}
void Owen2dFrame::OnAlpha( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	sscanf((const char *)e.GetString().c_str(), "%lf", &canvas->lsnake_alpha);
}
void Owen2dFrame::OnBeta( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	sscanf((const char *)e.GetString().c_str(), "%lf", &canvas->lsnake_beta);
}
void Owen2dFrame::OnGamma( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	sscanf((const char *)e.GetString().c_str(), "%lf", &canvas->lsnake_gamma);
}
void Owen2dFrame::OnFeature( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	for (canvas->curr_feature=0;
			e.GetString()!=Owen2dFeatureNames[canvas->curr_feature];
			canvas->curr_feature++)
		;
	mAuxControls->SetFeature(canvas->curr_feature,
		0!=canvas->temp_list[canvas->curr_feature].status,
		canvas->temp_list[canvas->curr_feature].transform, canvas->Range,
		canvas->temp_list[canvas->curr_feature].weight,
		canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmax*canvas->Range);
}
void Owen2dFrame::OnFeatureStatus( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->temp_list[canvas->curr_feature].status = e.IsChecked();
	canvas->reload();
}
void Owen2dFrame::OnTransform( wxCommandEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	for (canvas->temp_list[canvas->curr_feature].transform=0;
			e.GetString()!=Owen2dTransformNames[
			canvas->temp_list[canvas->curr_feature].transform];
			canvas->temp_list[canvas->curr_feature].transform++)
		;
	mAuxControls->SetTransform(
		canvas->temp_list[canvas->curr_feature].transform, canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmax*canvas->Range);
	canvas->reload();
}
void Owen2dFrame::OnFeatureUpdate( wxCommandEvent& unused )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	for (int k=0; k<8; k++)
		if (canvas->object_number==k || canvas->object_number==8)
			for (int j=0; j<6; j++)
				canvas->accepted_list[k][j] = canvas->temp_list[j];
	canvas->ReCalc_Edge_Costs();
}
void Owen2dFrame::OnWeight( wxScrollEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->temp_list[canvas->curr_feature].weight = e.GetPosition()/100.0;
	mAuxControls->UpdateWeight(canvas->temp_list[canvas->curr_feature].weight);
	canvas->reload();
}
void Owen2dFrame::OnMean( wxScrollEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->temp_list[canvas->curr_feature].rmean =
		e.GetPosition()/(100.0*canvas->Range);
	mAuxControls->UpdateMean(canvas->temp_list[canvas->curr_feature].rmean*
		canvas->Range);
	canvas->reload();
}
void Owen2dFrame::OnStdDev( wxScrollEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->temp_list[canvas->curr_feature].rstddev =
		e.GetPosition()/(100.0*canvas->Range);
	mAuxControls->UpdateStdDev(canvas->temp_list[canvas->curr_feature].rstddev*
		canvas->Range);
	canvas->reload();
}
void Owen2dFrame::OnFeatureMin( wxScrollEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->temp_list[canvas->curr_feature].rmin =
		e.GetPosition()/(100.0*canvas->Range);
	mAuxControls->UpdateFeatureMin(canvas->temp_list[canvas->curr_feature].rmin
		*canvas->Range);
	canvas->reload();
}
void Owen2dFrame::OnFeatureMax( wxScrollEvent& e )
{
	Owen2dCanvas *canvas = dynamic_cast<Owen2dCanvas*>(mCanvas);
	canvas->temp_list[canvas->curr_feature].rmax =
		e.GetPosition()/(100.0*canvas->Range);
	mAuxControls->UpdateFeatureMax(canvas->temp_list[canvas->curr_feature].rmax
		*canvas->Range);
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Allow the user to scroll through the slices with the mouse wheel. */void Owen2dFrame::OnMouseWheel ( wxMouseEvent& e ) {
	const int  rot   = e.GetWheelRotation();
	wxCommandEvent  ce;
	if (rot>0)         OnPrevious(ce);
	else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( Owen2dFrame, wxFrame )
BEGIN_EVENT_TABLE       ( Owen2dFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_GRAYMAP,          Owen2dFrame::OnGrayMap    )
  EVT_MOUSEWHEEL(                  Owen2dFrame::OnMouseWheel )
  EVT_BUTTON( ID_NEXT,             Owen2dFrame::OnNext       )
  EVT_BUTTON( ID_PREVIOUS,         Owen2dFrame::OnPrevious   )
  EVT_BUTTON( ID_SET_INDEX,        Owen2dFrame::OnSetIndex   )
  EVT_BUTTON( ID_RESET,            Owen2dFrame::OnReset      )
  EVT_COMBOBOX( ID_OBJECT,         Owen2dFrame::OnObject     )
  EVT_BUTTON( ID_DELETE_OBJECT,    Owen2dFrame::OnDeleteObject)
  EVT_BUTTON( ID_LOAD_OBJECT,      Owen2dFrame::OnLoadObject )
  EVT_CHECKBOX( ID_DEFAULT_FILL,   Owen2dFrame::OnDefaultFill)
  EVT_CHECKBOX( ID_OVERLAY,        Owen2dFrame::OnOverlay    )
  EVT_CHECKBOX( ID_LAYOUT,         Owen2dFrame::OnLayout     )
  EVT_BUTTON( ID_SET_OUTPUT,       Owen2dFrame::OnSetOutput  )
  EVT_BUTTON( ID_OUT_TYPE,         Owen2dFrame::OnOutputType )
  EVT_BUTTON( ID_SAVE,             Owen2dFrame::OnOwen2dSave)
  EVT_COMBOBOX( ID_MODE,           Owen2dFrame::OnMode       )
  EVT_COMBOBOX( ID_OUT_OBJECT,     Owen2dFrame::OnOutputObject)
  EVT_COMBOBOX( ID_BRUSH_SIZE,     Owen2dFrame::OnBrushSize  )
  EVT_COMBOBOX( ID_ITERATES,       Owen2dFrame::OnIterates   )
  EVT_TEXT(   ID_MIN_POINTS,       Owen2dFrame::OnMinPoints  )
  EVT_TEXT(   ID_ALPHA,            Owen2dFrame::OnAlpha      )
  EVT_TEXT(   ID_BETA,             Owen2dFrame::OnBeta       )
  EVT_TEXT(   ID_GAMMA,            Owen2dFrame::OnGamma      )
  EVT_COMBOBOX( ID_FEATURE,        Owen2dFrame::OnFeature    )
  EVT_CHECKBOX( ID_FEATURE_STATUS, Owen2dFrame::OnFeatureStatus)
  EVT_COMBOBOX( ID_TRANSFORM,      Owen2dFrame::OnTransform  )
  EVT_BUTTON( ID_FEATURE_UPDATE,   Owen2dFrame::OnFeatureUpdate)
  EVT_COMMAND_SCROLL( ID_WEIGHT,   Owen2dFrame::OnWeight     )
  EVT_COMMAND_SCROLL( ID_MEAN,     Owen2dFrame::OnMean       )
  EVT_COMMAND_SCROLL( ID_STD_DEV,  Owen2dFrame::OnStdDev     )
  EVT_COMMAND_SCROLL( ID_FEATURE_MIN,   Owen2dFrame::OnFeatureMin   )
  EVT_COMMAND_SCROLL( ID_FEATURE_MAX,   Owen2dFrame::OnFeatureMax   )
  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, Owen2dFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  Owen2dFrame::OnWidthSlider  )
  EVT_BUTTON( ID_CT_LUNG,          Owen2dFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   Owen2dFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          Owen2dFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              Owen2dFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE_SLIDER,  Owen2dFrame::OnSliceSlider  )
  EVT_COMMAND_SCROLL( ID_SCALE_SLIDER,  Owen2dFrame::OnScaleSlider  )
  EVT_CHECKBOX(       ID_INVERT,        Owen2dFrame::OnInvert       )
  EVT_CHECKBOX(       ID_LABELS,        Owen2dFrame::OnLabels       )

#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_CENTER_SLIDER, Owen2dFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  Owen2dFrame::OnUpdateUIWidthSglider )
#endif
END_EVENT_TABLE()
//======================================================================
