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
 * \file   KinematicsInterFrame.cpp
 * \brief  KinematicsInterFrame class implementation.
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
/** \brief Constructor for KinematicsInterFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
KinematicsInterFrame::KinematicsInterFrame (): MainFrame( 0 )
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

    mCanvas = new KinematicsInterCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
	//mLayoutControls = new LayoutControls( mControlPanel, mBottomSizer, "Layout", ID_CHECKBOX_LAYOUT );
		
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
void KinematicsInterFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:KinematicsInter:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void KinematicsInterFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    //row 1, col 1  

	 //row 1, col 1
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for KinematicsInterFrame class. */
KinematicsInterFrame::~KinematicsInterFrame ( void ) 
{
    cout << "KinematicsInterFrame::~KinematicsInterFrame" << endl;
    wxLogMessage( "KinematicsInterFrame::~KinematicsInterFrame" );
    
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }
    //if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
    //if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }
	if( mRefFrameControls != NULL ) { delete mRefFrameControls; mRefFrameControls = NULL; }


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
/** \brief callback for key presses. */
void KinematicsInterFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "KinematicsInterFrame::OnChar" );
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
        cout << "KinematicsInterFrame::OnChar: " << ::gTimerInterval << endl;
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
void KinematicsInterFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void KinematicsInterFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
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
void KinematicsInterFrame::OnOpen ( wxCommandEvent& e ) 
{
	wxFileDialog  f( this, "Select many image files", _T(""), _T(""), 
        "CAVASS files (*.BS1;*.PLN)|*.BS1;*.PLN", 
        wxFILE_MUST_EXIST | wxMULTIPLE );
    int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;

    wxArrayString  names;    
    f.GetPaths( names );
    if (names.Count()==0)    return;    
    if (names.Count()<2) 
	{
        wxMessageBox( "At least 2 files should be specified." );
        return;
    } 	
    
    loadFile( names );	
    
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void KinematicsInterFrame::loadFile (  wxArrayString  FileNames ) 
{
//    if (FileNames==NULL )    return;
    if (FileNames.GetCount()<2) 
	{
        wxMessageBox( "At least 2 files should be specified." );
        return;
    } 	
    
	for( unsigned int i=0; i<FileNames.Count(); i++ )
    _fileHistory.AddFileToHistory( FileNames[i] );

    wxString  tmp("CAVASS:KinematicsInter: ");
    ++mFileOrDataCount;
    if (mFileOrDataCount==1) 
	{
        tmp += FileNames[0];
    } else if (mFileOrDataCount==2) {
        tmp = mWindowTitle + ", " + FileNames[1];
    } else {
        assert( 0 );
    }
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) 
        {

            wxString  tmp = wxString("CAVASS:KinematicsInter: ");
            tmp+= (wxString)FileNames[0];
            tmp+= wxString::Format(" %d", i);
       
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );

    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
    canvas->loadFile( FileNames );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    const wxString  s = wxString( "file %s") + wxString(FileNames[0]) ;
    SetStatusText( s, 0 );
	mRefFrameControls->UpdateFrameMax(mControlPanel, mBottomSizer,2);
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void KinematicsInterFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:KinematicsInter: ");
    tmp += name;
    SetTitle( tmp );
    
    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KinematicsInterFrame::OnKinematicsInterSave ( wxCommandEvent &e ) 
{
    wxFileDialog  saveDlg( this, _T("Save file"), wxEmptyString,
                           _T("KinematicsInter.PLN"), _T("PLN files (*.PLN)|*.PLN|TXT files (*.TXT)|*.TXT|PAR files (*.PAR)|*.PAR|TRN files (*.TRN)|*.TRN"),
                           wxSAVE|wxOVERWRITE_PROMPT );

    saveDlg.SetFilterIndex(0);

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, KinematicsInter %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;
	
	KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
	if( !canvas->getKinematicsInterDone() )
	{
		wxMessageBox( "You should get Kinematics Done before save it." );
	}	
	else
	{		
		//if( saveDlg.GetFilterIndex() == 3 )
		
		canvas->SaveProfile((unsigned char*)(const char *)saveDlg.GetPath().c_str());
		
	}  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void KinematicsInterFrame::OnIntancesSlider ( wxScrollEvent& e )
{
    const int     newInstanceValue = e.GetPosition();
   // const double  newInstance = newInstanceValue/100.0;
    const wxString  s = wxString::Format( "%d", newInstanceValue );
 //   mInstancesControls->setInstanceText( s );
    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
    canvas->setInstances( newInstanceValue );

	mRefFrameControls->UpdateFrameMax( mControlPanel, mBottomSizer, newInstanceValue );

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for scale slider for data set 1 */
void KinematicsInterFrame::OnRefFrameSlider ( wxScrollEvent& e )
{
    const int     newInstanceValue = e.GetPosition();
   // const double  newInstance = newInstanceValue/100.0;
    const wxString  s = wxString::Format( "%d", newInstanceValue );
 //   mInstancesControls->setInstanceText( s );
    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
    canvas->setRefFrame( newInstanceValue );	
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void KinematicsInterFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void KinematicsInterFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void KinematicsInterFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void KinematicsInterFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void KinematicsInterFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void KinematicsInterFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void KinematicsInterFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new KinematicsInterPrint("CAVASS", canvas),
        new KinematicsInterPrint("CAVASS", canvas),
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
void KinematicsInterFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    KinematicsInterCanvas*  canvas = dynamic_cast<KinematicsInterCanvas*>(mCanvas);
    KinematicsInterPrint       printout( _T("CAVASS:KinematicsInter"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( KinematicsInterFrame, wxFrame )
BEGIN_EVENT_TABLE       ( KinematicsInterFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
#if 0
  EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           KinematicsInterFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    KinematicsInterFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           KinematicsInterFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  KinematicsInterFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          KinematicsInterFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, KinematicsInterFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )

  EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile )
#endif
//  EVT_COMBOBOX( ID_KinematicsInter_ROI,    KinematicsInterFrame::OnKinematicsInterROI    )  
  EVT_BUTTON( ID_SAVE,           KinematicsInterFrame::OnKinematicsInterSave  )


  EVT_COMMAND_SCROLL( ID_INSTANCES_SLIDER, KinematicsInterFrame::OnIntancesSlider )  
  EVT_COMMAND_SCROLL( ID_REFFRAME_SLIDER, KinematicsInterFrame::OnRefFrameSlider )  
  

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, KinematicsInterFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  KinematicsInterFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  KinematicsInterFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  KinematicsInterFrame::OnUpdateUIScale1Slider  )
//   EVT_UPDATE_UI( ID_RED1_SLIDER,    KinematicsInterFrame::OnUpdateUIRed1Slider    )
//   EVT_UPDATE_UI( ID_GREEN1_SLIDER,  KinematicsInterFrame::OnUpdateUIGreen1Slider  )
//   EVT_UPDATE_UI( ID_BLUE1_SLIDER,   KinematicsInterFrame::OnUpdateUIBlue1Slider   )

//  EVT_UPDATE_UI( ID_CENTER2_SLIDER, KinematicsInterFrame::OnUpdateUICenter2Slider )
//  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  KinematicsInterFrame::OnUpdateUIWidth2Sglider )
//   EVT_UPDATE_UI( ID_SLICE2_SLIDER,  KinematicsInterFrame::OnUpdateUISlice2Slider  )
//   EVT_UPDATE_UI( ID_SCALE2_SLIDER,  KinematicsInterFrame::OnUpdateUIScale2Slider  )
//   EVT_UPDATE_UI( ID_RED2_SLIDER,    KinematicsInterFrame::OnUpdateUIRed2Slider    )
//   EVT_UPDATE_UI( ID_GREEN2_SLIDER,  KinematicsInterFrame::OnUpdateUIGreen2Slider  )
//   EVT_UPDATE_UI( ID_BLUE2_SLIDER,   KinematicsInterFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    KinematicsInterFrame::OnUpdateUICineSlider    )
#endif

//  EVT_BUTTON( ID_ZOOM_IN,        KinematicsInterFrame::OnZoomIn                 )
//  EVT_BUTTON( ID_ZOOM_OUT,       KinematicsInterFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, KinematicsInterFrame::OnBusyTimer )
  EVT_CHAR( KinematicsInterFrame::OnChar )
  
END_EVENT_TABLE()

wxFileHistory  KinematicsInterFrame::_fileHistory;
//=====================================================================
