/*
  Copyright 1993-2013, 2015-2016 Medical Image Processing Group
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
 * \file   ImportFrame.cpp
 * \brief  ImportFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ImportFrame.h"
#include  "ImportCanvas.h"

extern Vector  gFrameList;
//----------------------------------------------------------------------
/** \brief Constructor for ImportFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
ImportFrame::ImportFrame ( bool maximize, int w, int h, int frame_type ) : MainFrame( 0 )
{
    //init the types of input files that this app can accept
    mFrameType = frame_type;
    mFileNameFilter = NULL;
    switch (frame_type) {
        case IMPORT_MATHEMATICA :
            mFileNameFilter = (char*) "Mathematica file (*.M;*.m)|*.M;*.m";
            mModuleName = "CAVASS:Import Mathematica";
            break;
        case IMPORT_MATLAB :
            mFileNameFilter = (char*) "MatLab file (*.MAT;*.mat)|*.MAT;*.mat";
            mModuleName = "CAVASS:Import MatLab";
            break;
        case IMPORT_R :
            mFileNameFilter = (char*) "R file (*.R;*.r)|*.R;*.r";
            mModuleName = "CAVASS:Import R";
            break;
        case IMPORT_VTK :
            mFileNameFilter = (char*) "VTK file (*.VTK;*.vtk)|*.VTK;*.vtk";
            mModuleName = "CAVASS:Import VTK";
            break;
        default :
            assert( 0 );
            break;
    }
    mWindowTitle = mModuleName;
    SetTitle( mWindowTitle );
    mFileOrDataCount    = 0;
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

    //top of window contains image(s) displayed via ImportCanvas
    mCanvas = new ImportCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
void ImportFrame::initializeMenu ( void ) {
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
void ImportFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 7, 1, 1 );  //cols,vgap,hgap

    //row 1
    wxStaticText*  st;

    st = new wxStaticText( mControlPanel, -1, "x-size (pixels/voxels):" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mXSize = new wxTextCtrl( mControlPanel, ID_XSIZE, "256", wxDefaultPosition, wxSize(50,-1) );
    ::setColor( mXSize );
    fgs->Add( mXSize, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddSpacer( ButtonOffset );

    st = new wxStaticText( mControlPanel, -1, "x-space (mm):" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mXSpace = new wxTextCtrl( mControlPanel, ID_XSPACE, "1.0", wxDefaultPosition, wxSize(50,-1) );
    ::setColor( mXSpace );
    fgs->Add( mXSpace, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddSpacer( ButtonOffset );

    wxButton*  input = new wxButton( mControlPanel, ID_INPUT_BUTTON, "Input", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( input );
    fgs->Add( input, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 2
    st = new wxStaticText( mControlPanel, -1, "y-size (pixels/voxels):" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mYSize = new wxTextCtrl( mControlPanel, ID_YSIZE, "256", wxDefaultPosition, wxSize(50,-1) );
    ::setColor( mYSize );
    fgs->Add( mYSize, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddSpacer( ButtonOffset );

    st = new wxStaticText( mControlPanel, -1, "y-space (mm):" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mYSpace = new wxTextCtrl( mControlPanel, ID_YSPACE, "1.0", wxDefaultPosition, wxSize(50,-1) );
    ::setColor( mYSpace );
    fgs->Add( mYSpace, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddSpacer( ButtonOffset );

    wxButton*  save = new wxButton( mControlPanel, ID_SAVE_BUTTON, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( save );
    fgs->Add( save, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 3
    st = new wxStaticText( mControlPanel, -1, "z-size (pixels/voxels):" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mZSize = new wxTextCtrl( mControlPanel, ID_ZSIZE, "256", wxDefaultPosition, wxSize(50,-1) );
    ::setColor( mZSize );
    fgs->Add( mZSize, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddSpacer( ButtonOffset );

    st = new wxStaticText( mControlPanel, -1, "z-space (mm):" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mZSpace = new wxTextCtrl( mControlPanel, ID_ZSPACE, "1.0", wxDefaultPosition, wxSize(50,-1) );
    ::setColor( mZSpace );
    fgs->Add( mZSpace, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    fgs->AddSpacer( ButtonOffset );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ImportFrame class. */
ImportFrame::~ImportFrame ( void ) {
    cout << "ImportFrame::~ImportFrame" << endl;
    wxLogMessage( "ImportFrame::~ImportFrame" );

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
 *  new ImportFrame.  It first searches the input file history for an
 *  acceptable input file.  If one is found, it is used.  If none is found
 *  then the user is prompted for an input file.
 *  \param  parentFrame is an optional (it may also be NULL) parent frame
 *      which will be closed if we are in single frame mode.
 *      (Furthermore, if the parent is maximized (or not maximized),
 *      the child correspondingly will also be maximized (or not).)
 *  \param  frame_type is one of IMPORT_MATHEMATICA, IMPORT_MATLAB, IMPORT_R,
 *      or IMPORT_VTK
 */
void ImportFrame::createImportFrame ( wxFrame* parentFrame, int frame_type )
{
	assert( frame_type == IMPORT_MATHEMATICA || frame_type == IMPORT_MATLAB
		 || frame_type == IMPORT_R || frame_type == IMPORT_VTK );

    wxString  filename;
    //did we find an acceptable input file in the input history?
    //display a dialog which allows the user to choose an input file.
    switch (frame_type) {
        case IMPORT_MATHEMATICA :
            filename = wxFileSelector( _T("Select Mathematica file"), _T(""),
                _T(""), _T(""), "Mathematica file (*.M;*.m)|*.M;*.m", wxFILE_MUST_EXIST );
            break;
        case IMPORT_MATLAB :
            filename = wxFileSelector( _T("Select MatLab file"), _T(""),
                _T(""), _T(""), "MatLab file (*.MAT;*.mat)|*.MAT;*.mat", wxFILE_MUST_EXIST );
            break;
        case IMPORT_R :
            filename = wxFileSelector( _T("Select R file"), _T(""),
                _T(""), _T(""), "R file (*.R;*.r)|*.R;*.r", wxFILE_MUST_EXIST );
            break;
        case IMPORT_VTK :
            filename = wxFileSelector( _T("Select VTK file"), _T(""),
                _T(""), _T(""), "VTK file (*.VTK;*.vtk)|*.VTK;*.vtk", wxFILE_MUST_EXIST );
            break;
        default :
            assert( 0 );
            break;
    }
    
    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //display an Import frame using the specified file as input
    ImportFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new ImportFrame( parentFrame->IsMaximized(), w, h, frame_type );
    } else {
        frame = new ImportFrame();
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
 *  Supported file extensions: BIM, BMP, DCM, DICOM, GIF, IM0, JPG, JPEG,
 *  PCX, TIF, TIFF, and VTK.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool ImportFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.BMP",   fn, false ))    return true;
    if (wxMatchWild( "*.DCM",   fn, false ))    return true;
    if (wxMatchWild( "*.DICOM", fn, false ))    return true;
    if (wxMatchWild( "*.GIF",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;
    if (wxMatchWild( "*.JPG",   fn, false ))    return true;
    if (wxMatchWild( "*.JPEG",  fn, false ))    return true;
    if (wxMatchWild( "*.PCX",   fn, false ))    return true;
    if (wxMatchWild( "*.PNG",   fn, false ))    return true;
    if (wxMatchWild( "*.TIF",   fn, false ))    return true;
    if (wxMatchWild( "*.TIFF",  fn, false ))    return true;
    //if (wxMatchWild( "*.VTK",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item.
 *  \param unused is not used.
 */
void ImportFrame::OnOpen ( wxCommandEvent& unused ) {
    createImportFrame( this, mFrameType );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void ImportFrame::OnInput ( wxCommandEvent& unused ) {
    createImportFrame( this, mFrameType );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void ImportFrame::loadFile ( const char* const fname ) {
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

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    Show();
    Raise();

	mFileName = fname;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load (and subsequently display) data directly from memory
 *  (instead of from a file).
 *  \todo this is incomplete.
 */
void ImportFrame::loadData ( char* name,
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
    
    ImportCanvas*  canvas = dynamic_cast<ImportCanvas*>( mCanvas );
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview.
 *  \param unused is not used.
 */
void ImportFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    ImportCanvas*  canvas = dynamic_cast<ImportCanvas*>( mCanvas );
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
void ImportFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ImportCanvas*     canvas = dynamic_cast<ImportCanvas*>( mCanvas );
    MainPrint          printout( mModuleName.c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImportFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    MainFrame::OnSaveScreen( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImportFrame::OnSave ( wxCommandEvent& unused ) {
    //get (required) user specifed values
    int     xSize, ySize, zSize;
    double  xSpace, ySpace, zSpace;
    int     n;

    n = sscanf( (const char *)mXSize->GetValue().c_str(), "%d", &xSize );
    if (n!=1 || xSize<=0) {
        wxMessageBox( "Please specify a valid value for x size.", "Bad x size value", wxICON_ERROR | wxOK );
        return;
    }

    n = sscanf( (const char *)mYSize->GetValue().c_str(), "%d", &ySize );
    if (n!=1 || ySize<=0) {
        wxMessageBox( "Please specify a valid value for y size.", "Bad y size value", wxICON_ERROR | wxOK );
        return;
    }

    n = sscanf( (const char *)mZSize->GetValue().c_str(), "%d", &zSize );
    if (n!=1 || zSize<=0) {
        wxMessageBox( "Please specify a valid value for z size.", "Bad z size value", wxICON_ERROR | wxOK );
        return;
    }

    n = sscanf( (const char *)mXSpace->GetValue().c_str(), "%lf", &xSpace );
    if (n!=1 || xSpace<=0) {
        wxMessageBox( "Please specify a valid value for x space.", "Bad x space value", wxICON_ERROR | wxOK );
        return;
    }

    n = sscanf( (const char *)mYSpace->GetValue().c_str(), "%lf", &ySpace );
    if (n!=1 || ySpace<=0) {
        wxMessageBox( "Please specify a valid value for y space.", "Bad y space value", wxICON_ERROR | wxOK );
        return;
    }

    n = sscanf( (const char *)mZSpace->GetValue().c_str(), "%lf", &zSpace );
    if (n!=1 || zSpace<=0) {
        wxMessageBox( "Please specify a valid value for z space.", "Bad z space value", wxICON_ERROR | wxOK );
        return;
    }

    //pop up a dialog to allow the user to specify the output file for the result.
    wxFileDialog  f( this, "Select import output image file", "", "imp_tmp.IM0",
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0", wxSAVE | wxOVERWRITE_PROMPT );
    const int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;

	// importMath  input-file  input-file-type  output-file-name  xSize ySize zSize  xSpace ySpace zSpace
	wxString  type;
	switch (mFrameType) {
		case IMPORT_MATHEMATICA :    type = "mathematica";    break;
		case IMPORT_MATLAB      :    type = "matlab";         break;
		case IMPORT_R           :    type = "r";              break;
		case IMPORT_VTK         :    type = "vtk";            break;
		default                 :    type = "unknown";        break;
	}
    wxString  cmd = wxString::Format( "\"%s/importMath\" \"%s\" %s \"%s\" %d %d %d %f %f %f",
		(const char *)Preferences::getHome().c_str(),
        (const char *)mFileName.c_str(), (const char *)type.c_str(), (const char *)f.GetPath().c_str(), xSize, ySize, zSize, xSpace, ySpace, zSpace );
  wxLogMessage( "command=%s", (const char *)cmd.c_str() );
  ProcessManager  p( "import running...", cmd );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( ImportFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ImportFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_INPUT_BUTTON, ImportFrame::OnInput )
  EVT_BUTTON( ID_SAVE_BUTTON,  ImportFrame::OnSave  )
END_EVENT_TABLE()
//======================================================================
