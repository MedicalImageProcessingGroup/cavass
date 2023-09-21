/*
  Copyright 2023 Medical Image Processing Group
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
#include  "cavass.h"
#include  "port_data/from_dicom.h"
#include  "ToSceneFrame.h"
#include <wx/wfstream.h>

extern Vector  gFrameList;
extern int     gTimerInterval;

int get_num_of_structures(const char filename[]);

//----------------------------------------------------------------------
/** \brief Constructor for ToSceneFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
ToSceneFrame::ToSceneFrame (): MainFrame( 0 )
{
    mFileOrDataCount = 0;
	mInputBS0           = "";
	structure_number    = 1;
    mSaveScreenControls = NULL;
	m_structN           = NULL;

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

    mCanvas = new ToSceneCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    mControlPanel->SetSizer( mBottomSizer );

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
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );
}

//----------------------------------------------------------------------
void ToSceneFrame::OnInformation ( wxCommandEvent& unused ) {
	wxArrayString files;
    files.Add( mInputBS0 );
    InformationDialog  id( files );
    id.ShowModal();
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void ToSceneFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:ToScene:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ToSceneFrame::addButtonBox ( void ) 
{
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap

    m_structN = new wxButton( mControlPanel, ID_STRUCT_NUMBER,
		wxString::Format("Struct. num.: %d",structure_number),
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_structN );
	fgs->Add( m_structN, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    wxButton*  saveBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( saveBut );
    fgs->Add( saveBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ToSceneFrame class. */
ToSceneFrame::~ToSceneFrame ( void ) {
    cout << "ToSceneFrame::~ToSceneFrame" << endl;
    wxLogMessage( "ToSceneFrame::~ToSceneFrame" );
    
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ToSceneFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void ToSceneFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    ToSceneCanvas*  canvas = dynamic_cast<ToSceneCanvas*>(mCanvas);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item */
void ToSceneFrame::OnOpen ( wxCommandEvent& e ) {
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""),
        _T(""), _T(""),
        "CAVASS files (*.BS0)|*.BS0",
        wxFILE_MUST_EXIST );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void ToSceneFrame::loadFile ( wxString& fname ) 
{
    if (fname.IsEmpty())    return;
    if (!wxFile::Exists(fname)) 
	{
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    mInputBS0 = fname;
    _fileHistory.AddFileToHistory( fname );

	number_of_structures = get_num_of_structures((const char *)fname.c_str());

    wxString  tmp("CAVASS:ToScene: ");
    ++mFileOrDataCount;
    if (mFileOrDataCount==1) 
	{
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
            tmp = wxString::Format( "CAVASS:ToScene:%s (%d)",
			    (const char *)fname.c_str(), i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );

    SetStatusText( _T("file ")+fname, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ToSceneFrame::OnStructNumber( wxCommandEvent& unused )
{
    if (structure_number < number_of_structures)
        structure_number++;
    else
        structure_number = 1;
    m_structN->SetLabel(wxString::Format("Struct. num.: %d",structure_number));
}

void ToSceneFrame::OnToSceneSave ( wxCommandEvent &e ) 
{
	wxFileName in_fn(mInputBS0);
	wxString ext(in_fn.GetExt());
	if (ext.CmpNoCase("BS0"))
	{
		wxMessageBox(ext+" file conversion not implemented.");
		return;
	}
	in_fn.ClearExt();
	wxString program, default_name(in_fn.GetFullPath()), filter;
	program = "BS0_TO_BIM";
	filter = "(*.BIM)|*.BIM";

    wxFileDialog saveDlg(this, _T("Convert file"), wxEmptyString,
                     default_name, filter, wxSAVE|wxOVERWRITE_PROMPT );

    if (saveDlg.ShowModal() != wxID_OK) 
		return;
	program = program+" \""+mInputBS0+wxString::Format("\" %d \"",
		structure_number-1)+saveDlg.GetPath()+"\" 0";
	wxLogMessage("%s\n", program.c_str());
	system((const char *)program.c_str());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void ToSceneFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void ToSceneFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void ToSceneFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void ToSceneFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void ToSceneFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void ToSceneFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void ToSceneFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    ToSceneCanvas*  canvas = dynamic_cast<ToSceneCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new ToScenePrint("CAVASS", canvas),
        new ToScenePrint("CAVASS", canvas),
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
void ToSceneFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ToSceneCanvas*  canvas = dynamic_cast<ToSceneCanvas*>(mCanvas);
    ToScenePrint       printout( _T("CAVASS:ToScene"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( ToSceneFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ToSceneFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:

  EVT_BUTTON( ID_STRUCT_NUMBER,  ToSceneFrame::OnStructNumber )
  EVT_BUTTON( ID_SAVE,           ToSceneFrame::OnToSceneSave  )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, ToSceneFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  ToSceneFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  ToSceneFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  ToSceneFrame::OnUpdateUIScale1Slider  )
  EVT_UPDATE_UI( ID_CINE_SLIDER,    ToSceneFrame::OnUpdateUICineSlider    )
#endif

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )
  EVT_TIMER( ID_BUSY_TIMER, ToSceneFrame::OnBusyTimer )  
 
END_EVENT_TABLE()

wxFileHistory  ToSceneFrame::_fileHistory;
//======================================================================
