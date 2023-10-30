/*
  Copyright 1993-2017, 2020-2023 Medical Image Processing Group
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
 * \file   MainFrame.cpp
 * \brief  MainFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "CycleFrame.h"
#include  "ExampleFrame.h"
#include  "FilterFrame.h"
#include  "ImportFrame.h"
#include  "InterpolateFrame.h"
#include  "MontageCanvas.h"
#include  "PreferencesDialog.h"
#include  "VOIIOIFrame.h"
#include  "VOIPickSlicesFrame.h"
#include  "VOIROIFrame.h"
#include  "VOIStandardlizeFrame.h"
#include  "IntenMapFrame.h"
#include  "ToSceneFrame.h"
#include  "cavass_icon.xpm"    //icon
#include  "wx/statline.h"
#include  "wx/wxhtml.h"

Vector  gFrameList;    ///< list of frames (windows)
int     gTimerInterval = 250;
//----------------------------------------------------------------------
/** \brief MainFrame ctor. */
MainFrame::MainFrame ( int dummy )
    : wxFrame( NULL, -1, _T("CAVASS"), wxPoint(gWhere,gWhere),
               wxSize(WIDTH,HEIGHT) ),
      mModuleName(_T("CAVASS"))
{
    SetIcon( wxICON(cavass_icon) );  //set the icon
    mFileNameFilter     = (char *)"All files (*.*)|*.*";
    mBottomSizer        = NULL;
    mControlPanel       = NULL;
    mSaveScreenControls = NULL;
    mSplitter           = NULL;
	m_buttonBox         = NULL;
	mButtonRows         = 9;
    wxToolTip::Enable( Preferences::getShowToolTips() );
}
//----------------------------------------------------------------------
/** \brief MainFrame ctor. */
MainFrame::MainFrame ( )
    : wxFrame( NULL, -1, _T("CAVASS"), wxPoint(gWhere,gWhere),
      wxSize(WIDTH,HEIGHT) ),
      mModuleName(_T("CAVASS"))
{
    ::gFrameList.push_back( this );
    if (!Preferences::getSingleFrameMode()) {
        gWhere += cWhereIncr;
        if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    }

    SetIcon( wxICON(cavass_icon) );  //set the icon
    mFileNameFilter     = (char *)"All files (*.*)|*.*";
    mBottomSizer        = NULL;
    mControlPanel       = NULL;
    mSaveScreenControls = NULL;
    mSplitter           = NULL;
	m_buttonBox         = NULL;
	mButtonRows         = 9;

    initializeMenu();
    ::copyWindowTitles( this );
    wxString  tmp("CAVASS");
    //does a window with this title (file) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "CAVASS (%d)", i);
            if (!searchWindowTitles(tmp))    break;
        }
    }
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    ::addToAllWindowMenus( tmp );

    mMainPanel = new wxPanel( this, -1 );
    ::setColor( this );
    ::setColor( mMainPanel );
    wxSizer*  mainSizer = new wxBoxSizer(wxVERTICAL);

    //middle, image panel
    mCanvas = new MainCanvas( mMainPanel, this, -1, wxDefaultPosition,
        wxDefaultSize );
    if (Preferences::getCustomAppearance()) {
        mCanvas->SetBackgroundColour( wxColour(DkBlue) );
        mCanvas->SetForegroundColour( wxColour(Yellow) );
    }
    wxSizer*  middleSizer = new wxBoxSizer(wxVERTICAL);
    middleSizer->SetMinSize( 800, 600 );
    middleSizer->Add( mCanvas, 1, wxGROW );
    
    mainSizer->Add( middleSizer, 1,
        wxGROW | (wxALL & ~(wxTOP | wxBOTTOM)), 10 );
    
    mainSizer->Add( 0, 5, 0, wxGROW ); // spacer in between
    mMainPanel->SetAutoLayout( true );
    mMainPanel->SetSizer( mainSizer );
    mainSizer->Fit( this );
    mainSizer->SetSizeHints( this );
    
    Maximize( true );
    Show();
    Raise();
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    wxToolTip::Enable( Preferences::getShowToolTips() );
}
//----------------------------------------------------------------------
/** \brief Define standard menu bar. */
void MainFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    wxMenuBar*  menu_bar = new wxMenuBar();

    mFileMenu = new wxMenu();
    mFileMenu->Append( ID_NEW,           "&New..."   );
    mFileMenu->Append( ID_OPEN,          "&Open..."  );

    mFileMenu->Append( ID_INPUT,         "&Input..." );
    wxMenu*  import_menu = new wxMenu();
    mFileMenu->Append( ID_IMPORT,        "&Import", import_menu );
        import_menu->Append( ID_IMPORT_DICOM,       "DICOM"       );
        import_menu->Append( ID_IMPORT_EASYHEADER,  "EasyHeader"  );
        import_menu->Append( ID_IMPORT_MATLAB,      "MatLab"      );
        import_menu->Append( ID_IMPORT_MATHEMATICA, "Mathematica" );
        import_menu->Append( ID_IMPORT_R,           "R"           );
        import_menu->Append( ID_IMPORT_VTK,         "VTK"         );
    wxMenu*  export_menu = new wxMenu();
    mFileMenu->Append( ID_EXPORT,        "&Export", export_menu );
        export_menu->Append( ID_EXPORT_DICOM,       "DICOM"       );
        export_menu->Append( ID_EXPORT_MATHEMATICA, "Mathematica" );
        export_menu->Append( ID_EXPORT_MATLAB,      "MatLab"      );
        export_menu->Append( ID_EXPORT_PGM,         "PGM"         );
        export_menu->Append( ID_EXPORT_R,           "R"           );
        export_menu->Append( ID_EXPORT_STL,         "STL"         );
        export_menu->Append( ID_EXPORT_VTK,         "VTK"         );
    mFileMenu->Append( ID_CLOSE,         "&Close" );
    mFileMenu->AppendSeparator();
    mFileMenu->Append( ID_PAGE_SETUP,    "Page &Setup..." );
    mFileMenu->Append( ID_PRINT_PREVIEW, "Print Pre&view" );
    mFileMenu->Append( ID_PRINT,         "&Print" );
#ifndef __MACH__  //not necessary on Mac
    mFileMenu->AppendSeparator();
    mFileMenu->Append( ID_EXIT,          "&Exit" );
#endif
    menu_bar->Append( mFileMenu, "&File" );

    wxMenu*  edit_menu = new wxMenu();
    edit_menu->Append( ID_COPY,        "&Copy" );
    edit_menu->Append( ID_PASTE,       "&Paste" );
    edit_menu->AppendSeparator();
    edit_menu->Append( ID_PREFERENCES, "&Preferences..." );
    menu_bar->Append( edit_menu, "&Edit" );
    
    wxMenu*  tools_menu = new wxMenu();
    tools_menu->Append( ID_TUTORIALS,   "&Tutorials" );
    tools_menu->Append( ID_RECIPES,     "&Recipes" );
    tools_menu->Append( ID_TASKS,       "&Tasks" );
    tools_menu->Append( ID_ITK_FILTERS, "&ITK Filters" );
    tools_menu->Append( ID_SAVE_SCREEN, "&Save Screen" );
    tools_menu->Append( ID_SHOW_SCREEN, "&Show Screen" );
    menu_bar->Append( tools_menu, "&Tools" );

#ifndef BUILD_WITH_ITK
    tools_menu->Enable( ID_ITK_FILTERS, false );
#endif
    tools_menu->Enable( ID_TASKS,       false );
    tools_menu->Enable( ID_RECIPES,     false );
    
    wxMenu*  preprocess_menu = new wxMenu();
    wxMenu*  pp_scops_menu = new wxMenu();

    mVOIMenu = new wxMenu();
    mVOIMenu->Append( ID_PP_SCOPS_VOI_ROI, "&ROI" );
    mVOIMenu->Append( ID_PP_SCOPS_VOI_IOI, "&IOI" );
    mVOIMenu->Append( ID_PP_SCOPS_VOI_PICKSLICES, "&PickSlices" );
    pp_scops_menu->Append( ID_PP_SCOPS_VOI, "&VOI", mVOIMenu );

    pp_scops_menu->Append( ID_PP_SCOPS_VOI_STANDARDIZE, "&Standardize" );
    pp_scops_menu->Append( ID_PP_SCOPS_INTERPOLATE,     "&Interpolate" );
    pp_scops_menu->Append( ID_PP_SCOPS_FILTER, "&Filter" );
    mSegmentMenu = new wxMenu();
    mSegmentMenu->Append( ID_PP_SCOPS_SEGMENT_THRESHOLD, "&Threshold" );
    mSegmentMenu->Append( ID_PP_SCOPS_SEGMENT_2DINTERACTIVE,"&Interactive2D" );
    mSegmentMenu->Append( ID_PP_SCOPS_IRFC, "I&RFC" );
    pp_scops_menu->Append( ID_PP_SCOPS_SEGMENT,     "&Segment", mSegmentMenu );

    mClassifyMenu = new wxMenu();
    mClassifyMenu->Append( ID_PP_SCOPS_CLASSIFY_1FEATURE,  "&1-Feature" );
    mClassifyMenu->Append( ID_PP_SCOPS_CLASSIFY_FUZZ_CONN, "&FuzzConn" );
	mClassifyMenu->Append( ID_PP_SCOPS_CLASSIFY_INTEN_MAP, "&IntensityMap" );

    pp_scops_menu->Append( ID_PP_SCOPS_CLASSIFY, "&Classify", mClassifyMenu );

    pp_scops_menu->Append( ID_PP_SCOPS_ALGEBRA,  "&Algebra"  );
    
    pp_scops_menu->Append( ID_PP_SCOPS_REGISTER, "&Register" );
    wxMenu*  pp_stops_menu = new wxMenu();
    pp_stops_menu->Append( ID_PP_STOPS_SURFACE_NORMAL, "&Surface Normal" );
    pp_stops_menu->Append( ID_PP_STOPS_MERGE_STRUCTURES, "&Merge Structures" );
    pp_stops_menu->Append( ID_PP_STOPS_TO_STRUCTURE, "&To Structure" );
    pp_stops_menu->Append( ID_PP_STOPS_TO_SCENE,     "T&o Scene" );
    preprocess_menu->Append( ID_PP_SCOPS, "Scene Operations", pp_scops_menu );
    preprocess_menu->Append( ID_PP_STOPS, "Structure Operations", pp_stops_menu );
    menu_bar->Append( preprocess_menu, "&Preprocess" );
    
    wxMenu*  visualize_menu = new wxMenu();
    //slice: montage, cycle, reslice
    wxMenu*  vis_slice_menu = new wxMenu();
    vis_slice_menu->Append( ID_VIS_MONTAGE, "&Montage" );

    wxMenuItem* menuItem = new wxMenuItem( vis_slice_menu, ID_VIS_CYCLE, "&Cycle" );
    vis_slice_menu->Append( menuItem );

    //menuItem = new wxMenuItem( vis_slice_menu, ID_VIS_RESLICE, "&Reslice" );
    //vis_slice_menu->Append( menuItem );
    //menuItem->Enable( false );

    vis_slice_menu->Append( ID_VIS_OVERLAY, "&Overlay" );
    visualize_menu->Append( ID_VIS_SLICE, "&Slice", vis_slice_menu );
    //surface: view, measure, create movie
    visualize_menu->Append( ID_VIS_SURF_VIEW, "&Surface" );
    //volume: view, measure, create movie
    visualize_menu->Append( ID_VIS_VOL_VIEW, "&Volume" );
    menu_bar->Append( visualize_menu, "&Visualize" );
    
    wxMenu*  manipulate_menu = new wxMenu();
    //select slice, measure, reflect, cut, separate, move, create movie
    manipulate_menu->Append( ID_MANIPULATE, "&Manipulate" );
    menu_bar->Append( manipulate_menu, "&Manipulate" );

    wxMenu*  analyze_menu = new wxMenu();
    //scene: density profile, roi statistics
    wxMenu*  scene_menu = new wxMenu();
    scene_menu->Append( ID_ANL_DENSITY, "&Density Profile" );
    scene_menu->Append( ID_ANL_ROI,     "&ROI" );
    analyze_menu->Append( ID_ANL_SCENE, "&Scene", scene_menu );
    //structure: register, kinematics
    wxMenu*  structure_menu = new wxMenu();
    structure_menu->Append( ID_ANL_REGISTER,   "&Register" );
    structure_menu->Append( ID_ANL_STATIC,   "&Static" );

    wxMenu* KinematicsMenu = new wxMenu();
    KinematicsMenu->Append( ID_ANL_KINEMATICS_INTER, "&Inter" );
    KinematicsMenu->Append( ID_ANL_KINEMATICS_INTRA, "&Intra" );
    structure_menu->Append( ID_ANL_KINEMATICS, "&Kinematics", KinematicsMenu );

    analyze_menu->Append( ID_ANL_STRUCTURE, "&Structure", structure_menu );
    menu_bar->Append( analyze_menu, "&Analyze" );

    scene_menu->Enable( ID_ANL_DENSITY, true );
    scene_menu->Enable( ID_ANL_ROI,     true );
    structure_menu->Enable( ID_ANL_REGISTER,   false );
    structure_menu->Enable( ID_ANL_KINEMATICS, true );

    mWindowMenu = new wxMenu();
#ifndef __MACH__
    mHideControls = new wxMenuItem( mWindowMenu, ID_WINDOW_HIDE_CONTROLS,
                                    "Hide Controls\tAlt-C" );
    mWindowMenu->Append( mHideControls );
    mWindowMenu->AppendSeparator();
#endif
    menu_bar->Append( mWindowMenu, "&Window" );

    wxMenu*  help_menu = new wxMenu();
#ifdef __MACH__
    mHideControls = new wxMenuItem( mWindowMenu, ID_WINDOW_HIDE_CONTROLS,
                                    "Hide Controls\tAlt-C" );
    help_menu->Append( mHideControls );
    help_menu->AppendSeparator();
#endif
    help_menu->Append( ID_ABOUT,         "&About"       );
    help_menu->Append( ID_HELP,          "&Help"        );
    help_menu->Append( ID_INFORMATION,   "&Information" );
    help_menu->Append( ID_DOCUMENTATION, "&Documentation" );
    help_menu->AppendSeparator();
    help_menu->Append( ID_EXAMPLE,       "&Example"     );
    menu_bar->Append( help_menu, "&Help" );

    help_menu->Enable( ID_HELP, false );
    
    SetMenuBar( menu_bar );
    
    CreateStatusBar( 5 );
    int  widths[] = { -1, 400, 100, 100, 100 };
    SetStatusWidths( 5, widths );
    SetStatusText( "Ready",  0 );
    SetStatusText( "Left",   2 );
    SetStatusText( "Middle", 3 );
    SetStatusText( "Right",  4 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief MainFrame dtor. */
MainFrame::~MainFrame ( void ) {
    cout << "MainFrame::~MainFrame" << endl;
    wxLogMessage( "MainFrame::~MainFrame" );
    if (mCanvas!=NULL) { delete mCanvas;  mCanvas=NULL; }
#if 1
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
        exit( 0 );
    }
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief this function determines if the given filename is of a type
 *  that can be read by this module/app.
 *  \param filename the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool MainFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.BMP",   fn, false ))    return true;
    if (wxMatchWild( "*.BS0",   fn, false ))    return true;
    if (wxMatchWild( "*.BS1",   fn, false ))    return true;
    if (wxMatchWild( "*.BS2",   fn, false ))    return true;
    if (wxMatchWild( "*.DCM",   fn, false ))    return true;
    if (wxMatchWild( "*.DICOM", fn, false ))    return true;
    if (wxMatchWild( "*.GIF",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;
    if (wxMatchWild( "*.JPG",   fn, false ))    return true;
    if (wxMatchWild( "*.JPEG",  fn, false ))    return true;
	if (wxMatchWild( "*.MV0",   fn, false ))    return true;
    if (wxMatchWild( "*.PCX",   fn, false ))    return true;
    if (wxMatchWild( "*.PLN",   fn, false ))    return true;
    if (wxMatchWild( "*.PNG",   fn, false ))    return true;
    if (wxMatchWild( "*.TIF",   fn, false ))    return true;
    if (wxMatchWild( "*.TIFF",  fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::flush_temp_data( void )
{
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses.  nothing is currently done. */
void MainFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "MainFrame::OnChar" );
    switch (e.GetKeyCode()) {
        case 'a' :
        case 'A' :
            break;
    }
}
//======================================================================
/** \brief callback for Copy menu item.  Typically, this will copy the 
 *  contents of the canvas to the clipboard.
 */
void MainFrame::OnCopy ( wxCommandEvent& unused ) {
    wxYield();
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

    //create a dc backed by a bitmap, and have the canvas draw into this dc
    //  (and, therefore, the backing bitmap).  then we can copy the contents
    //  of the backing bitmap to the clipboard.
    wxMemoryDC  m;
    int         w, h;
    mCanvas->GetSize( &w, &h );
    wxBitmap    bitmap( w, h );
    m.SelectObject( bitmap );
    if (Preferences::getCustomAppearance())
#if wxCHECK_VERSION(2, 9, 0)
        m.SetBrush( wxBrush(wxColour(DkBlue), wxBRUSHSTYLE_SOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxBRUSHSTYLE_SOLID) );
#else
        m.SetBrush( wxBrush(wxColour(DkBlue), wxSOLID) );
    else
        m.SetBrush( wxBrush(*wxBLACK, wxSOLID) );
#endif
    m.DrawRectangle( 0, 0, w, h );
    mCanvas->paint( &m );

    //use the following to copy only the first image
    //bool  copiedOK = wxTheClipboard->AddData( new wxBitmapDataObject(*canvas->mBitmaps[0]) );

    bool  copiedOK = wxTheClipboard->AddData( new wxBitmapDataObject(bitmap) );
    if (copiedOK)
        wxLogMessage( "Image has been put on the clipboard.\nYou can paste it now and look at it." );
    else
        wxLogError( "Can't copy image to the clipboard." );
    wxTheClipboard->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnAbout ( wxCommandEvent& unused ) {
    wxMessageBox(
"CAVASS version 1.1.1 \n\
\n\
  Xinjian Chen, Ph.D.,\n\
  George J. Grevera, Ph.D.,\n\
  Tad Iwanaga, M.S.,\n\
  Tingching Kao, M.S., \n\
  Shipra Mishra, B.S.,\n\
  Dewey Odhner, M.A.,\n\
  Andre Souza, Ph.D.,\n\
  Jayaram K. Udupa, Ph.D.,\n\
  Xiaofen Zheng, Ph.D.,\n\
  Ying (Ronald) Zhuge, Ph.D.                    \n\
\n\
Copyright 1993-2023 University of Pennsylvania",
        _T("About CAVASS"),
        wxICON_INFORMATION | wxOK );
}
//----------------------------------------------------------------------
void MainFrame::OnInformation ( wxCommandEvent& e ) {
    InformationDialog  id( this );
    id.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnNew ( wxCommandEvent& e ) {
    cout << "OnNew" << endl;
    wxLogMessage("OnNew");
    new MainFrame();
    if (Preferences::getSingleFrameMode())    Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnInput ( wxCommandEvent& e ) {
    InputHistoryDialog  ihd( this );
    ihd.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Handle File -> Open selection.
 */
void MainFrame::OnOpen ( wxCommandEvent& e ) {
    cout << "OnOpen" << endl;
    wxLogMessage( "OnOpen" );
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""),
        _T(""), _T(""),
        //"CAVASS files (*.BIM;*.BS0;*.BS2;*.IM0;*.SH0;*.MV0)|*.BIM;*.BS0;*.BS2;*.IM0;*.SH0;*.MV0",
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff", 
        wxFILE_MUST_EXIST );

    if (!filename || filename.Length() < 1) {
        Raise();
        return;
    }

    wxString  tmp = filename;
    tmp.LowerCase();
    if (tmp.EndsWith( ".bim" ) || tmp.EndsWith( ".im0" )) {
        MontageFrame*  frame = new MontageFrame();
        frame->loadFile( filename.c_str() );
        MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( frame->mCanvas );
        assert( canvas != NULL );
        canvas->setScale( 1.0 );
        if (Preferences::getSingleFrameMode())    Close();
    } else if (tmp.EndsWith( ".bs0" ) || tmp.EndsWith( ".bs2" ) || tmp.EndsWith( ".sh0" )) {
        SurfViewFrame*  frame = new SurfViewFrame();
        wxArrayString  f;
        f.Add( filename );
        frame->loadFiles( f );
        if (Preferences::getSingleFrameMode())    Close();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnCycle ( wxCommandEvent& e ) {
    CycleFrame::createCycleFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnMontage ( wxCommandEvent& e ) {
    MontageFrame::createMontageFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnDocumentation ( wxCommandEvent& unused ) {
 wxDialog       dlg( this, wxID_ANY, wxString(_("CAVASS Documentation")) );
    wxBoxSizer*    topsizer = new wxBoxSizer( wxVERTICAL );
    wxHtmlWindow*  html = new wxHtmlWindow( &dlg, wxID_ANY, wxDefaultPosition,
        wxSize(600, 400), wxHW_SCROLLBAR_AUTO );
    html->SetBorders( 0 );
    wxString  s = wxString::Format( "%s/docs/index.html", (const char *)Preferences::getHome().c_str() );
    bool  loaded = html->LoadPage( s );
    if (!loaded)
        wxMessageBox( s, "Missing documentation file", wxICON_ERROR | wxOK );
    topsizer->Add( html, 1, wxALL, 10 );

#if 1
#if wxUSE_STATLINE
    topsizer->Add( new wxStaticLine(&dlg, wxID_ANY), 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );
#endif // wxUSE_STATLINE
    wxButton*  bu1 = new wxButton( &dlg, wxID_OK, _("OK") );
    bu1->SetDefault();
    topsizer->Add( bu1, 0, wxALL|wxALIGN_RIGHT, 15 );
#endif

    dlg.SetSizer( topsizer );
    topsizer->Fit( &dlg );
    dlg.ShowModal();
    //dlg.SetSize( 800, 800 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnTutorials ( wxCommandEvent& unused ) {
    wxDialog       dlg( this, wxID_ANY, wxString(_("CAVASS Tutorials")) );
    wxBoxSizer*    topsizer = new wxBoxSizer( wxVERTICAL );
    wxHtmlWindow*  html = new wxHtmlWindow( &dlg, wxID_ANY, wxDefaultPosition,
        wxSize(600, 400), wxHW_SCROLLBAR_AUTO );
    html->SetBorders( 0 );
    wxString  s = wxString::Format( "%s/tutorials/Tutorial_intro.html", (const char *)Preferences::getHome().c_str() );
    bool  loaded = html->LoadPage( s );
    if (!loaded)
        wxMessageBox( s, "Missing tutorials file", wxICON_ERROR | wxOK );
    topsizer->Add( html, 1, wxALL, 10 );

#if 1
#if wxUSE_STATLINE
    topsizer->Add( new wxStaticLine(&dlg, wxID_ANY), 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );
#endif // wxUSE_STATLINE
    wxButton*  bu1 = new wxButton( &dlg, wxID_OK, _("OK") );
    bu1->SetDefault();
    topsizer->Add( bu1, 0, wxALL|wxALIGN_RIGHT, 15 );
#endif

    dlg.SetSizer( topsizer );
    topsizer->Fit( &dlg );
    dlg.ShowModal();
    //dlg.SetSize( 800, 800 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnTasks ( wxCommandEvent& unused ) {
    // \todo
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnRecipes ( wxCommandEvent& unused ) {
    // \todo
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnEasyHeader ( wxCommandEvent& unused ) {
    EasyHeaderFrame::createEasyHeaderFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExample ( wxCommandEvent& unused ) {
    ExampleFrame::createExampleFrame( this );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportDicom ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportPgm ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this, true, EXPORT_PGM );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportStl ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this, true, EXPORT_STL );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportMathematica ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this, true, EXPORT_MATHEMATICA );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportMatlab ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this, true, EXPORT_MATLAB );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportR ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this, true, EXPORT_R );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnExportVtk ( wxCommandEvent& unused ) {
    ExportFrame::createExportFrame( this, true, EXPORT_VTK );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnImportMathematica ( wxCommandEvent& unused ) {
    ImportFrame::createImportFrame( this, IMPORT_MATHEMATICA );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnImportMatlab ( wxCommandEvent& unused ) {
    ImportFrame::createImportFrame( this, IMPORT_MATLAB );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnImportR ( wxCommandEvent& unused ) {
    ImportFrame::createImportFrame( this, IMPORT_R );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnImportVtk ( wxCommandEvent& unused ) {
    ImportFrame::createImportFrame( this, IMPORT_VTK );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnFromDicom ( wxCommandEvent& unused ) {
    FromDicomFrame::createFromDicomFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnInterpolate ( wxCommandEvent& unused ) {
    InterpolateFrame::createInterpolateFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnITKFilter ( wxCommandEvent& unused ) {
    ITKFilterFrame::createITKFilterFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPPScops1Feature ( wxCommandEvent& unused ) {
    ThresholdFrame::createThresholdFrame( this, true, true );
}

void MainFrame::OnPPScopsIntenMap ( wxCommandEvent& unused ) {
    IntenMapFrame::createIntenMapFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPPScopsFilter ( wxCommandEvent& unused ) {
    FilterFrame::createFilterFrame( this );
}

void MainFrame::OnPPScopsVOIROI   ( wxCommandEvent& unused ) {
    VOIROIFrame::createVOIROIFrame( this );
}

void MainFrame::OnPPScopsVOIIOI   ( wxCommandEvent& unused ) {
    VOIIOIFrame::createVOIIOIFrame( this );
}

void MainFrame::OnPPScopsVOIPickSlices   ( wxCommandEvent& unused ) {
    VOIPickSlicesFrame::createVOIPickSlicesFrame( this );
}

void MainFrame::OnPPScopsVOIStandardize   ( wxCommandEvent& unused )
{
    wxFileDialog  f( this, "Select image files", _T(""), _T(""), 
        "CAVASS files (*.IM0)|*.IM0", 
        wxFILE_MUST_EXIST | wxMULTIPLE );
    int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;
    wxArrayString  names;
    f.GetPaths( names );

    if ( names.Count()>0)
    { 
        VOIStandardlizeFrame*  frame = new VOIStandardlizeFrame(); 
        frame->loadFile(names); 
        if (Preferences::getSingleFrameMode())    Close(); 
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MainFrame::OnPPStopsToScene ( wxCommandEvent& unused ) 
{
    wxString  filename = wxFileSelector( _T("Select structure file"), _T(""), 
        _T(""), _T(""),
        "CAVASS files (*.BS0)|*.BS0", 
        wxFILE_MUST_EXIST ); 
     
    if (filename.Length()>0) { 
        ToSceneFrame*  frame = new ToSceneFrame(); 
        frame->loadFile(filename); 
        if (Preferences::getSingleFrameMode())    Close(); 
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnDensityAnalysis ( wxCommandEvent& unused ) 
{
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""), 
        _T(""), _T(""), 
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff", 
        wxFILE_MUST_EXIST ); 
     
    if (filename.Length()>0) { 
        DensityFrame*  frame = new DensityFrame(); 
        frame->loadFile(filename.c_str()); 
        if (Preferences::getSingleFrameMode())    Close(); 
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MainFrame::OnROIAnalysis ( wxCommandEvent& unused ) 
{
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""), 
        _T(""), _T(""), 
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff", 
        wxFILE_MUST_EXIST ); 
     
    if (filename.Length()>0) { 
        ROIStatisticalFrame*  frame = new ROIStatisticalFrame(); 
        frame->loadFile(filename.c_str()); 
        if (Preferences::getSingleFrameMode())    Close(); 
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MainFrame::OnAnalyzeStatic ( wxCommandEvent& unused ) 
{
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""), 
        _T(""), _T(""), 
        "CAVASS files (*.BS1;*.PLN)|*.BS1;*.PLN", 
        wxFILE_MUST_EXIST ); 
     
    if (filename.Length()>0) { 
        AnalyzeStaticFrame*  frame = new AnalyzeStaticFrame(); 
        frame->loadFile(filename.c_str()); 
        if (Preferences::getSingleFrameMode())    Close(); 
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void MainFrame::OnKinematicsIntra ( wxCommandEvent& unused ) 
{
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""), 
        _T(""), _T(""), 
        "CAVASS files (*.BS1;*.PLN)|*.BS1;*.PLN", 
        wxFILE_MUST_EXIST ); 
     
    if (filename.Length()>0) { 
        KinematicsIntraFrame*  frame = new KinematicsIntraFrame(); 
        frame->loadFile(filename.c_str()); 
        if (Preferences::getSingleFrameMode())    Close(); 
    }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnKinematicsInter ( wxCommandEvent& unused ) 
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
    
    KinematicsInterFrame*  frame = new KinematicsInterFrame();
    frame->loadFile( names );
        
    if (Preferences::getSingleFrameMode())    Close();
}

////////////////////////////////////
void MainFrame::OnPPScopsFuzzComp ( wxCommandEvent& unused ) {
    FuzzCompFrame::createFuzzCompFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPPScopsThreshold ( wxCommandEvent& unused ) {
    ThresholdFrame::createThresholdFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnSegment2d ( wxCommandEvent& unused ) {
    Segment2dFrame::createSegment2dFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPPScopsIRFC ( wxCommandEvent& unused ) {
    IRFCFrame::createIRFCFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnAlgebra ( wxCommandEvent& unused )
{
     wxFileDialog  f( this, "Select two image files", _T(""), _T(""), 
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0",
        wxFILE_MUST_EXIST | wxMULTIPLE );
    int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;
    wxArrayString  names;
    f.GetPaths( names );
    if (names.Count()==0)    return;
    wxString  f1, f2("");
    if (names.Count()>2) 
    {
        wxMessageBox( "Algebra requires that you select only two files.", "More than 2 files selected.", wxOK );
        return;
    }
    else if (names.Count()==2)
    {
        f1 = names[0];
        f2 = names[1];
    } 
    else if (names.Count()==1) 
    {
        //allow the user to select the second file
        f1 = names[0];
        wxFileDialog  f( this, "Select the second image file", _T(""), _T(""),
            "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0", wxFILE_MUST_EXIST );
        while (f2.Length() <= 0)
        {
            ret = f.ShowModal();
            if (ret == wxID_CANCEL)
                break;
            f.GetPaths( names );
            if (names.Count())
                f2 = names[0];
        }
    }
    //at this point, f1 and f2 contain the names of the files
    AlgebraFrame*  frame = new AlgebraFrame();
    int err = frame->loadFile( f1.c_str() );
    if( err > 0 )
    {
        wxMessageBox( "Algebra can not load the first image.", "Load Image Fail.", wxOK );
        
        if( frame != NULL )
            delete frame;

        return;
    }

    err = frame->loadFile( f2.c_str() );
    if( err > 0 )
    {
        if( err == 100 )
            wxMessageBox( "The first image size is different with the second image.", "Algebra Operation Can not Proceed.", wxOK );
        else
            wxMessageBox( "Algebra can not load the second image.", "Load Image Fail.", wxOK );

        if( frame != NULL )
            delete frame;

        return;
    }

    if( frame->IsImgSizeEqual() == false )
    {
        wxMessageBox( "Algebra requires that you select two Equal Size Images.", "Require Equal Size Images.", wxOK );
        
        if( frame != NULL )
            delete frame;

        return;
    }
    if (Preferences::getSingleFrameMode())    
        Close();

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnRegister ( wxCommandEvent& unused ) {

    wxFileDialog  f( this, "Select two image files", _T(""), _T(""), 
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0",
        wxFILE_MUST_EXIST | wxMULTIPLE );
    int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;
    wxArrayString  names;
    //f.GetFilenames( names );
    f.GetPaths( names );
    if (names.Count()==0)    return;
    wxString  f1, f2;
    if (names.Count()>2) {
        wxMessageBox( "Register requires that you select only two files.", "More than 2 files selected.", wxOK );
        return;
    } else if (names.Count()==2) {
        f1 = names[0];
        f2 = names[1];
    } else if (names.Count()==1) {
        //allow the user to select the second file
        f1 = names[0];
        f2 = wxFileSelector( _T("Select the second image file"),
                 _T(""), _T(""), _T(""),
                 "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0",
                 wxFILE_MUST_EXIST );
        if (!f2 || f2.Length()<=0)    return;
    }
    //at this point, f1 and f2 contain the names of the files
    RegisterFrame*  frame = new RegisterFrame();
        frame->loadFile( f1.c_str() );
        frame->loadFile( f2.c_str() );
        if (Preferences::getSingleFrameMode())    Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnVisOverlay ( wxCommandEvent& e ) {
    wxFileDialog  f( this, "Select two image files", _T(""), _T(""), 
        "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
        wxFILE_MUST_EXIST | wxMULTIPLE );
    int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;
    wxArrayString  names;
    f.GetPaths( names );
    if (names.Count()==0)    return;
    wxString  f1, f2;
    if (names.Count()>2) {
        wxMessageBox( "Overlay requires that you select only two files.", "More than 2 files selected.", wxOK );
        return;
    } else if (names.Count()==2) {
        f1 = names[0];
        f2 = names[1];
    } else if (names.Count()==1) {
        //allow the user to select the second file
        f1 = names[0];
        f2 = wxFileSelector( _T("Select the second image file"),
            _T(""), _T(""), _T(""),
            "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
            wxFILE_MUST_EXIST );
        if (!f2 || f2.Length()<=0)    return;
    }
    //at this point, f1 and f2 contain the names of the files
    OverlayFrame*  frame = new OverlayFrame();
        frame->loadFile( f1.c_str() );
        frame->loadFile( f2.c_str() );
        if (Preferences::getSingleFrameMode())    Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnVisSurfView ( wxCommandEvent& e ) {
    SurfViewFrame::createSurfViewFrame( this, true, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnVisVolView ( wxCommandEvent& e ) {
    SurfViewFrame::createSurfViewFrame( this, true, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnManipulate ( wxCommandEvent& e ) {
    SurfViewFrame::createSurfViewFrame( this, true, true, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnQuit ( wxCommandEvent& e ) {
    Close( TRUE );
    exit( 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnMaximize ( wxMaximizeEvent& unused ) {
    if (mSplitter==NULL)    return;
    mSplitter->SetSashPosition( -dControlsHeight );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnMove ( wxMoveEvent& e ) {
    //when moving the entire window
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the save screen control to the control area of the bottom
 *  of the window.
 */
void MainFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    if (mSaveScreenControls!=NULL) {
        delete mSaveScreenControls;
        mSaveScreenControls = NULL;
        return;
    }

    if (mControlPanel!=NULL && mBottomSizer!=NULL) {
        mSaveScreenControls = new SaveScreenControls( mControlPanel,
            mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
            ID_OVERWRITE_SCREEN );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnShowScreen ( wxCommandEvent& unused ) {
    ShowScreenFrame::createShowScreenFrame( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnAppendScreen ( wxCommandEvent& unused ) {
    assert( mSaveScreenControls!=NULL );
    mCanvas->appendContents( (char*)(const char *)mSaveScreenControls->getFileName().c_str() );
    SetStatusText( "ready",  0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief allow the user to select a save screen file. */
void MainFrame::OnBrowseScreen ( wxCommandEvent& unused ) {
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""),
        _T(""), _T(""),
        "TIFF files (*.tif;*.tiff)|*.tif;*.tiff",
		wxSAVE );
    
    if (filename.Length()>0) {
        wxString  s = wxString::Format( "file selected %s", (const char *)filename.c_str() );
        SetStatusText( s,  0 );
        if (mSaveScreenControls!=NULL)
            mSaveScreenControls->setFileName( filename );
    }
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnOverwriteScreen ( wxCommandEvent& unused ) {
    assert( mSaveScreenControls!=NULL );
    mCanvas->saveContents( (char*)(const char *)mSaveScreenControls->getFileName().c_str() );
    SetStatusText( "ready",  0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnCloseEvent ( wxCloseEvent& unused ) {
    cout << "MainFrame::OnCloseEvent" << endl;
    ::removeFromAllWindowMenus( mWindowTitle );
    Destroy();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnClose ( wxCommandEvent& unused ) {
    cout << "OnClose" << endl;
    wxLogMessage( "OnClose" );
    Close( true );
#if 1
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
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
    Destroy();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPageSetup ( wxCommandEvent& unused ) {
    assert( g_pageSetupData != NULL );
    assert( g_printData     != NULL );
    *g_pageSetupData = *g_printData;
    wxPageSetupDialog  pageSetupDialog( this, g_pageSetupData );
    pageSetupDialog.ShowModal();
    *g_printData = pageSetupDialog.GetPageSetupData().GetPrintData();
    *g_pageSetupData = pageSetupDialog.GetPageSetupData();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two printout objects: for preview, and possible printing.
    assert( g_printData != NULL );
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrintPreview*    preview = new wxPrintPreview(
        new MainPrint(wxString("CAVASS").c_str(), mCanvas),
        new MainPrint(wxString("CAVASS").c_str(), mCanvas),
        &printDialogData );
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    wxPreviewFrame*  frame = new wxPreviewFrame( preview, this,
        _T("CAVASS Print Preview"),
        wxPoint(100, 100), wxSize(425, 550) );  //was 600x650
    frame->Centre( wxBOTH );
    frame->Initialize();
    frame->Show();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    MainPrint          printout( mModuleName.c_str(), mCanvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for toggling control panel on and off. */
void MainFrame::OnHideControls ( wxCommandEvent& unused ) {
    if (mSplitter==NULL)    return;
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
void MainFrame::OnWindow ( wxCommandEvent& e ) {
    wxMenuItem*  p_menuitem = GetMenuBar()->FindItem( e.GetId() );
    wxString  name = p_menuitem->GetItemLabelText();
    wxWindow*  w = wxWindow::FindWindowByLabel( name );
    if (w)
        w->Raise();
    wxLogMessage( "OnWindow:" + name );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MainFrame::OnPreferences ( wxCommandEvent& unused ) {
    PreferencesDialog  pd(this);
    pd.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( MainFrame, wxFrame )
BEGIN_EVENT_TABLE       ( MainFrame, wxFrame )
  DefineStandardFrameCallbacks
END_EVENT_TABLE()
//======================================================================

