/*
  Copyright 1993-2013, 2015, 2017 Medical Image Processing Group
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
 * \file   AnalyzeStaticFrame.cpp
 * \brief  AnalyzeStaticFrame class implementation.
 *         external programs used:
 *            
 *            
 *            
 * \author Xinjian Chen, Ph.D 
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "ScaleADControls.h"
#include  "SetFilterIndexControls.h"
#include  "SetLayout.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for AnalyzeStaticFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
AnalyzeStaticFrame::AnalyzeStaticFrame (): MainFrame( 0 )
{
    mFileOrDataCount = 0;	
	
    mSaveScreenControls = NULL;

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

    mCanvas = new AnalyzeStaticCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    SetStatusText( "Move",   2 );
    SetStatusText( "Flash1", 3 );
    SetStatusText( "Flash2", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void AnalyzeStaticFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:AnalyzeStatic:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void AnalyzeStaticFrame::addButtonBox ( void ) 
{
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
   
	 //row 1, col 1
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for AnalyzeStaticFrame class. */
AnalyzeStaticFrame::~AnalyzeStaticFrame ( void ) {
    cout << "AnalyzeStaticFrame::~AnalyzeStaticFrame" << endl;
    wxLogMessage( "AnalyzeStaticFrame::~AnalyzeStaticFrame" );
    
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
void AnalyzeStaticFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void AnalyzeStaticFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
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
void AnalyzeStaticFrame::OnOpen ( wxCommandEvent& e ) {
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""),
        _T(""), _T(""),
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
        wxFILE_MUST_EXIST );
    
    if (filename.Length()>0) {
        AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
        if (!canvas->isLoaded(0)) {
		    loadFile(filename.c_str());
        } else {
            AnalyzeStaticFrame*  frame = new AnalyzeStaticFrame();
            frame->loadFile(filename.c_str());
        }
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void AnalyzeStaticFrame::loadFile ( const char* const fname ) 
{
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) 
	{
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:AnalyzeStatic: ");
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
            tmp = wxString::Format( "CAVASS:AnalyzeStatic:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
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
void AnalyzeStaticFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (strlen(name) == 0)  name = (char *)"no name";
    wxString  tmp("CAVASS:AnalyzeStatic: ");
    tmp += name;
    SetTitle( tmp );
    
    AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnalyzeStaticFrame::OnAnalyzeStaticSave ( wxCommandEvent &e ) 
{
    wxFileDialog  saveDlg( this, _T("Save file"), wxEmptyString,
                           _T("OutPut1.STAT"), _T("Static files (*.STAT)|*.STAT"),
                           wxSAVE|wxOVERWRITE_PROMPT );

    saveDlg.SetFilterIndex(1);

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, AnalyzeStatic %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;
	
	AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
	if( !canvas->getAnalyzeStaticDone() )
	{
		wxMessageBox( "You should Get Kinematics Done before save it." );
	}	
	else
	{		
		canvas->SaveProfile((unsigned char*)(const char *)saveDlg.GetPath().c_str());
	}  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AnalyzeStaticFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void AnalyzeStaticFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void AnalyzeStaticFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void AnalyzeStaticFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void AnalyzeStaticFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void AnalyzeStaticFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void AnalyzeStaticFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new AnalyzeStaticPrint("CAVASS", canvas),
        new AnalyzeStaticPrint("CAVASS", canvas),
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
void AnalyzeStaticFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    AnalyzeStaticCanvas*  canvas = dynamic_cast<AnalyzeStaticCanvas*>(mCanvas);
    AnalyzeStaticPrint       printout( _T("CAVASS:AnalyzeStatic"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( AnalyzeStaticFrame, wxFrame )
BEGIN_EVENT_TABLE       ( AnalyzeStaticFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
#if 0
  EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           AnalyzeStaticFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    AnalyzeStaticFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           AnalyzeStaticFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  AnalyzeStaticFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          AnalyzeStaticFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, AnalyzeStaticFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )

  EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile )
#endif
//  EVT_BUTTON( ID_MATCH_INDEX2,   AnalyzeStaticFrame::OnMatchIndex2 )
//  EVT_BUTTON( ID_BLINK1,         AnalyzeStaticFrame::OnBlink1      )
//  EVT_BUTTON( ID_BLINK2,         AnalyzeStaticFrame::OnBlink2      )
//  EVT_BUTTON( ID_LAYOUT,         AnalyzeStaticFrame::OnLayout        )
  EVT_BUTTON( ID_SAVE,           AnalyzeStaticFrame::OnAnalyzeStaticSave  )


#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, AnalyzeStaticFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  AnalyzeStaticFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  AnalyzeStaticFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  AnalyzeStaticFrame::OnUpdateUIScale1Slider  )
//   EVT_UPDATE_UI( ID_RED1_SLIDER,    AnalyzeStaticFrame::OnUpdateUIRed1Slider    )
//   EVT_UPDATE_UI( ID_GREEN1_SLIDER,  AnalyzeStaticFrame::OnUpdateUIGreen1Slider  )
//   EVT_UPDATE_UI( ID_BLUE1_SLIDER,   AnalyzeStaticFrame::OnUpdateUIBlue1Slider   )

//  EVT_UPDATE_UI( ID_CENTER2_SLIDER, AnalyzeStaticFrame::OnUpdateUICenter2Slider )
//  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  AnalyzeStaticFrame::OnUpdateUIWidth2Sglider )
//   EVT_UPDATE_UI( ID_SLICE2_SLIDER,  AnalyzeStaticFrame::OnUpdateUISlice2Slider  )
//   EVT_UPDATE_UI( ID_SCALE2_SLIDER,  AnalyzeStaticFrame::OnUpdateUIScale2Slider  )
//   EVT_UPDATE_UI( ID_RED2_SLIDER,    AnalyzeStaticFrame::OnUpdateUIRed2Slider    )
//   EVT_UPDATE_UI( ID_GREEN2_SLIDER,  AnalyzeStaticFrame::OnUpdateUIGreen2Slider  )
//   EVT_UPDATE_UI( ID_BLUE2_SLIDER,   AnalyzeStaticFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    AnalyzeStaticFrame::OnUpdateUICineSlider    )
#endif

//  EVT_BUTTON( ID_ZOOM_IN,        AnalyzeStaticFrame::OnZoomIn                 )
//  EVT_BUTTON( ID_ZOOM_OUT,       AnalyzeStaticFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, AnalyzeStaticFrame::OnBusyTimer )  
 
END_EVENT_TABLE()

wxFileHistory  AnalyzeStaticFrame::_fileHistory;
//======================================================================
