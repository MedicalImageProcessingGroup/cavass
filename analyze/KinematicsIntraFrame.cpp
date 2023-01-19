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
 * \file   KinematicsIntraFrame.cpp
 * \brief  KinematicsIntraFrame class implementation.
 *         external programs used:
 *            
 *            
 *            
 * \author Xinjian Chen, Ph.D 
 *
 * Copyright: (C) 
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ScaleADControls.h"
#include  "SetFilterIndexControls.h"
#include  "SetLayout.h"
#include  "InstancesControls.h"
#include  "RefFrameControls.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for KinematicsIntraFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
KinematicsIntraFrame::KinematicsIntraFrame (): MainFrame( 0 )
{
    mFileOrDataCount = 0;	
	
    mSaveScreenControls = NULL;

    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    
//    m_lastChoice = m_lastChoiceSetting = -1;    
//    m_zsub = m_ysub = NULL;
    mRefFrameControls = NULL;

    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -
    //wxSize  canvasSize( GetSize().GetWidth(), GetSize().GetHeight()-dControlsHeight );

    mCanvas = new KinematicsIntraCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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

	mInstancesControls = new InstancesControls( mControlPanel, mBottomSizer, "Instances", 2, ID_INSTANCES_SLIDER );	

	mRefFrameControls = new RefFrameControls( mControlPanel, mBottomSizer, "Reference Frame", 2, ID_REFFRAME_SLIDER, InstancesSliderMax );		

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
void KinematicsIntraFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:KinematicsIntra:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void KinematicsIntraFrame::addButtonBox ( void ) 
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
/** \brief destructor for KinematicsIntraFrame class. */
KinematicsIntraFrame::~KinematicsIntraFrame ( void ) {
    cout << "KinematicsIntraFrame::~KinematicsIntraFrame" << endl;
    wxLogMessage( "KinematicsIntraFrame::~KinematicsIntraFrame" );
    
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
void KinematicsIntraFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void KinematicsIntraFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
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
void KinematicsIntraFrame::OnOpen ( wxCommandEvent& e ) {
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""),
        _T(""), _T(""),
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
        wxFILE_MUST_EXIST );
    
    if (filename.Length()>0) {
        KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
        if (!canvas->isLoaded(0)) {
		    loadFile(filename.c_str());
        } else {
            KinematicsIntraFrame*  frame = new KinematicsIntraFrame();
            frame->loadFile(filename.c_str());
        }
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void KinematicsIntraFrame::loadFile ( const char* const fname ) 
{
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) 
	{
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:KinematicsIntra: ");
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
            tmp = wxString::Format( "CAVASS:KinematicsIntra:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    mRefFrameControls->UpdateFrameMax(mControlPanel, mBottomSizer,2);

    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void KinematicsIntraFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:KinematicsIntra: ");
    tmp += name;
    SetTitle( tmp );
    
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
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
void KinematicsIntraFrame::OnKinematicsIntraSave ( wxCommandEvent &e ) 
{
    wxFileDialog  saveDlg( this, _T("Save file"), wxEmptyString,
                           _T("KinematicsIntra.PLN"), _T("Kinematics files (*.PLN;*.TXT;*PAR;*.TRN)|*.PLN;*.TXT;*PAR;*.TRN"),
                           wxSAVE|wxOVERWRITE_PROMPT );

    saveDlg.SetFilterIndex(1);

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, KinematicsIntra %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;
	
	KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
	if( !canvas->getKinematicsIntraDone() )
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void KinematicsIntraFrame::OnIntancesSlider ( wxScrollEvent& e ) 
{    
	const int     newInstanceValue = e.GetPosition();
   // const double  newInstance = newInstanceValue/100.0;
    const wxString  s = wxString::Format( "%d", newInstanceValue );
 //   mInstancesControls->setInstanceText( s );
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
    canvas->setInstances( newInstanceValue );

	mRefFrameControls->UpdateFrameMax( mControlPanel, mBottomSizer, newInstanceValue );
	
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void KinematicsIntraFrame::OnRefFrameSlider ( wxScrollEvent& e )
{
    const int     newInstanceValue = e.GetPosition();
   // const double  newInstance = newInstanceValue/100.0;
    const wxString  s = wxString::Format( "%d", newInstanceValue );
 //   mInstancesControls->setInstanceText( s );
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
    canvas->setRefFrame( newInstanceValue );	
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void KinematicsIntraFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void KinematicsIntraFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void KinematicsIntraFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void KinematicsIntraFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void KinematicsIntraFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void KinematicsIntraFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void KinematicsIntraFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new KinematicsIntraPrint("CAVASS", canvas),
        new KinematicsIntraPrint("CAVASS", canvas),
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
void KinematicsIntraFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    KinematicsIntraCanvas*  canvas = dynamic_cast<KinematicsIntraCanvas*>(mCanvas);
    KinematicsIntraPrint       printout( _T("CAVASS:KinematicsIntra"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( KinematicsIntraFrame, wxFrame )
BEGIN_EVENT_TABLE       ( KinematicsIntraFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
#if 0
  EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           KinematicsIntraFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    KinematicsIntraFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           KinematicsIntraFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  KinematicsIntraFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          KinematicsIntraFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, KinematicsIntraFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )

  EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile )
#endif
//  EVT_BUTTON( ID_MATCH_INDEX2,   KinematicsIntraFrame::OnMatchIndex2 )
//  EVT_BUTTON( ID_BLINK1,         KinematicsIntraFrame::OnBlink1      )
//  EVT_BUTTON( ID_BLINK2,         KinematicsIntraFrame::OnBlink2      )
//  EVT_BUTTON( ID_LAYOUT,         KinematicsIntraFrame::OnLayout        )
  EVT_BUTTON( ID_SAVE,           KinematicsIntraFrame::OnKinematicsIntraSave  )


  EVT_COMMAND_SCROLL( ID_INSTANCES_SLIDER, KinematicsIntraFrame::OnIntancesSlider )
  EVT_COMMAND_SCROLL( ID_REFFRAME_SLIDER, KinematicsIntraFrame::OnRefFrameSlider )  
 
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, KinematicsIntraFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  KinematicsIntraFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  KinematicsIntraFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  KinematicsIntraFrame::OnUpdateUIScale1Slider  )
//   EVT_UPDATE_UI( ID_RED1_SLIDER,    KinematicsIntraFrame::OnUpdateUIRed1Slider    )
//   EVT_UPDATE_UI( ID_GREEN1_SLIDER,  KinematicsIntraFrame::OnUpdateUIGreen1Slider  )
//   EVT_UPDATE_UI( ID_BLUE1_SLIDER,   KinematicsIntraFrame::OnUpdateUIBlue1Slider   )

//  EVT_UPDATE_UI( ID_CENTER2_SLIDER, KinematicsIntraFrame::OnUpdateUICenter2Slider )
//  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  KinematicsIntraFrame::OnUpdateUIWidth2Sglider )
//   EVT_UPDATE_UI( ID_SLICE2_SLIDER,  KinematicsIntraFrame::OnUpdateUISlice2Slider  )
//   EVT_UPDATE_UI( ID_SCALE2_SLIDER,  KinematicsIntraFrame::OnUpdateUIScale2Slider  )
//   EVT_UPDATE_UI( ID_RED2_SLIDER,    KinematicsIntraFrame::OnUpdateUIRed2Slider    )
//   EVT_UPDATE_UI( ID_GREEN2_SLIDER,  KinematicsIntraFrame::OnUpdateUIGreen2Slider  )
//   EVT_UPDATE_UI( ID_BLUE2_SLIDER,   KinematicsIntraFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    KinematicsIntraFrame::OnUpdateUICineSlider    )
#endif

//  EVT_BUTTON( ID_ZOOM_IN,        KinematicsIntraFrame::OnZoomIn                 )
//  EVT_BUTTON( ID_ZOOM_OUT,       KinematicsIntraFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, KinematicsIntraFrame::OnBusyTimer )  
 
END_EVENT_TABLE()

wxFileHistory  KinematicsIntraFrame::_fileHistory;
//======================================================================
