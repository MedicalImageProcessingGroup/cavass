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
 * \file   ShowScreenFrame.cpp
 * \brief  ShowScreenFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "FrameControls.h"
#include  "ShowScreenCanvas.h"

extern Vector  gFrameList;
//----------------------------------------------------------------------
/** \brief Constructor for ShowScreenFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
ShowScreenFrame::ShowScreenFrame ( bool maximize, int w, int h ) : MainFrame( 0 )
{
    //init the types of input files that this app can accept
    mFileNameFilter     = (char *)"screen files (*.tif;*.tiff)|*.tif;*.tiff";
    mFileOrDataCount    = 0;
    mFrameControls      = NULL;
    mModuleName         = "CAVASS:ShowScreen";
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

    //top of window contains image(s) displayed via ShowScreenCanvas
    mCanvas = new ShowScreenCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
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
    SetStatusText( "Move",     2 );
    SetStatusText( "Previous", 3 );
    SetStatusText( "Next",     4 );
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
void ShowScreenFrame::initializeMenu ( void ) {
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
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right. */
void ShowScreenFrame::addButtonBox ( void ) {
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
    wxButton*  setFrame = new wxButton( mControlPanel, ID_FRAME, "SetFrame",
        wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setFrame );
    fgs->Add( setFrame, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ShowScreenFrame class. */
ShowScreenFrame::~ShowScreenFrame ( void ) {
    cout << "ShowScreenFrame::~ShowScreenFrame" << endl;
    wxLogMessage( "ShowScreenFrame::~ShowScreenFrame" );

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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new ShowScreenFrame.  It first searches the input file history for an
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
void ShowScreenFrame::createShowScreenFrame ( wxFrame* parentFrame, bool useHistory )
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
        //alert the user that the file no longer exists
        if (n>0 && !wxFile::Exists(filename)) {
            wxString  tmp = wxString::Format( "File %s could not be opened.",
                (const char *)filename.c_str() );
            wxMessageBox( tmp, "File does not exist",
                wxOK | wxCENTER | wxICON_EXCLAMATION );
        }
    }

    //did we find an acceptable input file in the input history?
    if (filename.Length()==0 || !wxFile::Exists(filename)) {
        //nothing acceptable so display a dialog which allows the user
        // to choose an input file.
        filename = wxFileSelector( _T("Select image file"), _T(""),
            _T(""), _T(""),
            "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff;*vtk)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff;*vtk",
            wxFILE_MUST_EXIST );
    }
    
    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    //display an ShowScreen frame using the specified file as input
    ShowScreenFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new ShowScreenFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new ShowScreenFrame();
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
 *  Supported file extensions: BIM, BMP, DCM, DICOM, GIF, IM0, MV0, JPG, JPEG,
 *  PCX, TIF, TIFF, and VTK.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool ShowScreenFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.TIF",   fn, false ))    return true;
    if (wxMatchWild( "*.TIFF",  fn, false ))    return true;
	if (wxMatchWild( "*.MV0",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnOpen ( wxCommandEvent& unused ) {
    createShowScreenFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "ShowScreenFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "ShowScreenFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnShowScreen( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void ShowScreenFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxString  tmp = wxString::Format( "File %s could not be opened.", fname );
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    ++mFileOrDataCount;
    assert( mFileOrDataCount==1 );
    wxString  tmp = wxString::Format( "%s: ", (const char *)mModuleName.c_str() );
    tmp += fname;
    //does a window with this title (file) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "%s:%s (%d)", (const char *)mModuleName.c_str(), fname, i );
            if (!searchWindowTitles(tmp))    break;
        }
    }
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );

    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    assert( canvas != NULL );
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
void ShowScreenFrame::loadData ( char* name,
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
    
    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Previous button press.  display the previous slice.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnPrevious ( wxCommandEvent& unused ) {
    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    const int  slice = canvas->getSliceNo(0) - 1;
    if (slice<0)    return;
    const wxString  s = wxString::Format( "frame %d", slice+1 );
    SetStatusText( s, 0 );
    if (mFrameControls!=NULL)    mFrameControls->setFrameNo( slice );
    canvas->setSliceNo( 0, slice );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Next button press.  display the next slice.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnNext ( wxCommandEvent& unused ) {
    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    const int  slice = canvas->getSliceNo(0) + 1;
    if (slice >= canvas->getNoSlices(0))    return;
    const wxString  s = wxString::Format( "frame %d", slice+1 );
    SetStatusText( s, 0 );
    if (mFrameControls!=NULL)    mFrameControls->setFrameNo( slice );
    canvas->setSliceNo( 0, slice );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 *  \param unused is not used.
 */
void ShowScreenFrame::OnFrame ( wxCommandEvent& unused ) {
    if (mFrameControls!=NULL) {
        delete mFrameControls;
        mFrameControls = NULL;
        return;
    }
    if (mSaveScreenControls!=NULL) {
        delete mSaveScreenControls;
        mSaveScreenControls = NULL;
    }
    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    mFrameControls = new FrameControls( mControlPanel, mBottomSizer,
        "SetFrame", 0, canvas->getNoSlices(0), ID_FRAME_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for frame slider. */
void ShowScreenFrame::OnFrameSlider ( wxScrollEvent& e ) {
    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    const int  slice = e.GetPosition() - 1;
    if (slice<0 || slice>=canvas->getNoSlices(0))    return;
    const wxString  s = wxString::Format( "frame %d", slice+1 );
    SetStatusText( s, 0 );
    canvas->setSliceNo( 0, slice );
    canvas->reload();
}
#ifdef __WXX11__
/** \brief callback for frame slider.
 *  especially (only) need on X11 (w/out GTK) to get slider events.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnUpdateUIFrameSlider ( wxUpdateUIEvent& unused ) {
    if (m_center->GetValue() == mCanvas->getCenter())    return;
    mCanvas->setCenter( m_center->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    ShowScreenCanvas*  canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
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
/** \brief callback for printing.
 *  \param unused is not used.
 */
void ShowScreenFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ShowScreenCanvas*     canvas = dynamic_cast<ShowScreenCanvas*>( mCanvas );
    MainPrint          printout( mModuleName.c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Allow the user to scroll through the slices with the mouse wheel. */
void ShowScreenFrame::OnMouseWheel ( wxMouseEvent& e ) {
    const int  rot   = e.GetWheelRotation();
    wxCommandEvent  ce;
    if (rot>0)         OnPrevious(ce);
    else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ShowScreenFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    if (mFrameControls!=NULL) {
        delete mFrameControls;
        mFrameControls = NULL;
    }
    MainFrame::OnSaveScreen( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( ShowScreenFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ShowScreenFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON(         ID_FRAME,         ShowScreenFrame::OnFrame        )
  EVT_MOUSEWHEEL(                       ShowScreenFrame::OnMouseWheel   )
  EVT_BUTTON(         ID_NEXT,          ShowScreenFrame::OnNext         )
  EVT_BUTTON(         ID_PREVIOUS,      ShowScreenFrame::OnPrevious     )
  EVT_COMMAND_SCROLL( ID_FRAME_SLIDER,  ShowScreenFrame::OnFrameSlider  )
#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_FRAME_SLIDER, ShowScreenFrame::OnUpdateUIFrameSlider)
#endif
END_EVENT_TABLE()
//======================================================================

