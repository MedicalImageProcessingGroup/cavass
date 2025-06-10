/*
  Copyright 1993-2018, 2025 Medical Image Processing Group
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
 * \file   Segment2dFrame.cpp
 * \brief  Segment2dFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
/**
 * #include "cavass.h" MUST appear before any #includes of wx .h's (because
 * it defines a symbol, wxUSE_CONFIG_NATIVE, that is used by the wx headers )
 */
#include  "cavass.h"
#include  "CButton.h"
#include  "CCheckBox.h"
#include  "CComboBox.h"
#include  "GrayMapControls.h"
#include  "Segment2dAuxControls.h"
#include  "Segment2dIntDLControls.h"
#include  "Segment2dSlider.h"
#include  "SetSegment2dOutputControls.h"
#include  "SetIndexControls.h"
#include  "Segment2dFrame.h"
#include  "wx/html/htmlwin.h"

#include "PersistentSegment2dFrame.h"

#define s2dControlsHeight ((buttonHeight+1)*10+80)

extern Vector  gFrameList;

const wxArrayString Segment2dFrame::featureNames = { "Higher Density", "Lower Density", "Gradient 1", "Gradient 2", "Gradient 3", "Gradient 4" };
const wxArrayString Segment2dFrame::transformNames = { "Linear", "Gaussian", "Inv. Linear", "Inv. Gaussian", "Hyperbolic", "Inv. Hyperbolic" };
//----------------------------------------------------------------------
/** \brief Constructor for Segment2dFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
Segment2dFrame::Segment2dFrame ( bool maximize, int w, int h )
    : MainFrame( 0 )
{
    cout << "Segment2dFrame::Segment2dFrame( maximize, w, h )" << endl;
    //mPersistentMe = wxPersistenceManager::Get().Register( this );  <-- never use this version
    mPersistentMe = wxPersistenceManager::Get().Register( this, new PersistentSegment2dFrame(this) );
    //init the types of input files that this app can accept
    mFileNameFilter     = (char*) "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0";
    mFileOrDataCount    = 0;
    out_object          = 0;
    mGrayMapControls    = nullptr;
    mSetIndexControls   = nullptr;
    mSetOutputControls  = nullptr;
    mModuleName         = "CAVASS:Segment2d";
    mSaveScreenControls = nullptr;

    m_prev = nullptr;
    m_next = nullptr;
    m_setIndex = nullptr;
    m_grayMap = nullptr;
    mMode = nullptr;
    m_reset = nullptr;
    m_object = nullptr;
    m_deleteObject = nullptr;
    m_loadObject = nullptr;
    m_defaultFill = nullptr;
    m_overlay = nullptr;
    m_layout = nullptr;
    m_setOutputBut = nullptr;
    m_buttonBox = nullptr;
    m_buttonSizer = nullptr;
    fgs = nullptr;
    mAuxControls = nullptr;
    mIntDLControls = nullptr;

    modeName[Segment2dCanvas::LWOF]         = "LWOF";
    modeName[Segment2dCanvas::TRAINING]     = "Train";
    modeName[Segment2dCanvas::PAINT]        = "Paint";
    modeName[Segment2dCanvas::ILW]          = "ILW";
    modeName[Segment2dCanvas::LSNAKE]       = "LiveSnake";
    modeName[Segment2dCanvas::SEL_FEATURES] = "SelFeatures";
    modeName[Segment2dCanvas::REVIEW]       = "Review";
    modeName[Segment2dCanvas::REPORT]       = "Report";
    modeName[Segment2dCanvas::PEEK]         = "Peek";
    modeName[Segment2dCanvas::INT_DL]       = "IntDL";

    //fopen may return null and that's ok
    FILE* objnamfp = fopen( "object_names.spec", "rb" );
    for (int j=0; j<8; j++) {
        char buf[100];
        objectName[j] = "";
        if (objnamfp && !feof(objnamfp) && fgets(buf, sizeof(buf), objnamfp))
            objectName[j] = wxString(buf);
        objectName[j].Replace("\n", "");
        if (objectName[j] == "")
            objectName[j] = wxString::Format("%d", j+1);  //1..8
    }
    objectName[8] = "All";  //last entry
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
//    mSplitter->SetSashGravity( 1.0 );
//    mSplitter->SetSashPosition( -s2dControlsHeight );
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via Segment2dCanvas
    mCanvas = tCanvas = new Segment2dCanvas( mSplitter, this, wxID_ANY, wxDefaultPosition, wxDefaultSize );

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
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    mAuxControls = new Segment2dAuxControls( this, mControlPanel,
        mBottomSizer, (modeName[canvas->detection_mode]+" Controls").c_str(),
        canvas->paint_brush_size );
    mAuxControls->restoreAuxControls();
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    //if (maximize)    Maximize( true );
    //else             SetSize( w, h );
    //mSplitter->SetSashPosition( -s2dControlsHeight );
    restoreFrameSettings();  // <-- _MUST_ be called before Show(); and Raise(); (otherwise SetPosition and other calls will be ignored)
    //wxPersistenceManager::Get().Restore( this );    // <-- _MUST_ be called before Show(); and Raise(); (otherwise SetPosition and other calls will be ignored)

#if wxUSE_DRAG_AND_DROP
    m_dropTarget = new MainFileDropTarget;
    SetDropTarget( m_dropTarget );
#endif
    wxToolTip::Enable( Preferences::getShowToolTips() );
    //mSplitter->SetSashPosition( -s2dControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }

    //set different style for mouse button info in status bar
    auto tmp = GetStatusBar();
    int fields = tmp->GetFieldsCount();
    int* styles = new int[fields];
    styles[0] = styles[1] = wxSB_NORMAL;
    for (int i=2; i<fields; i++)    styles[i] = wxSB_SUNKEN;
    tmp->SetStatusStyles( tmp->GetFieldsCount(), styles );
    delete[] styles;    styles = nullptr;
    //wxClientDC dc( tmp );
    //dc.SetBackground( wxBrush(wxColour(LtBlue), wxBRUSHSTYLE_SOLID) );

    Show();
    Raise();

    delete topSizer;    topSizer = nullptr;
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones).
 *  this function does not any additional, custom menu items (but could
 *  if necessary).
 */
void Segment2dFrame::initializeMenu ( void ) {
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
    Bind( wxEVT_COMMAND_MENU_SELECTED, Segment2dFrame::OnHelp, ID_HELP );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnHelp ( wxCommandEvent& e ) {
#ifdef WIN32
    string help = "<body bgcolor=\"ivory\"><pre>\
<b>map 1 button (B) to 3 button (L M R) mouse:</b> \n\
  B &rarr; L \n\
  ctrl+B &rarr; M (or shift+B &rarr; M) \n\
  alt+B &rarr; R\
<hr/>\
<b>map 2 button (L R) to 3 button (L M R) mouse:</b> \n\
  L &rarr; L \n\
  ctrl+L &rarr; M (or shift+L &rarr; M) \n\
  R &rarr; R\
<hr/>\
<b>One by WACOM tablet (mod. no. CTL-472)</b> \n\
<b>map stylus/pen w/ 2 buttons (a,b) + tip (t) pressed on tablet:</b> \n\
  standard tablet behavior: \n\
    t &rarr; L \n\
    pen hovering over tablet no more than 0.5cm away and pressing a &rarr; M \n\
    pen hovering over tablet no more than 0.5cm away and pressing b &rarr; R \n\
  additionally (in interactive 2d only): \n\
    ctrl+t (or shift+t) &rarr; M \n\
    alt+t &rarr; R \n\
  where a=button closer to tip, b=button further from tip, and t=tip pressed on tablet\
<hr/>\
<b>Gaomo Graphics IPS Pen Display (mod. no. PDP1161).</b> \n\
  Note: You <em>must</em> install their driver. \n\
  in interactive 2d <em>only</em>: \n\
    t &rarr; L \n\
    ctrl+t (or shift+t) &rarr; M \n\
    alt+t &rarr; R \n\
  where a=button closer to tip, b=button further from tip, and t=tip pressed on tablet \n\
</pre></body>";
#else  //linux (and mac?)
    string help = "<body bgcolor=\"ivory\"><pre>\
<b>laptop touchpads:</b> \n\
  one finger:    L \n\
  three fingers: M \n\
  two fingers:   R\
<hr/>\
<b>map 1 button (B) to 3 button (L M R) mouse:</b> \n\
  B &rarr; L \n\
  ctrl+B &rarr; M (or shift+B &rarr; M) \n\
  alt+B &rarr; R\
<hr/>\
<b>map 2 button (L R) to 3 button (L M R) mouse:</b> \n\
  L &rarr; L \n\
  ctrl+L &rarr; M (or shift+L &rarr; M) \n\
  R &rarr; R\
<hr/>\
<b>One by WACOM tablet (mod. no. CTL-472).</b> \n\
<b>Gaomo Graphics IPS Pen Display (mod. no. PDP1161).</b> \n\
<b>map stylus/pen w/ 2 buttons (a,b) + tip (t) pressed on tablet:</b> \n\
  standard tablet behavior: \n\
    t &rarr; L \n\
    pen hovering over tablet no more than 0.5cm away and pressing a &rarr; M \n\
    pen hovering over tablet no more than 0.5cm away and pressing b &rarr; R \n\
  additionally (in interactive 2d only): \n\
    ctrl+t (or shift+t) &rarr; M \n\
    alt+t &rarr; R \n\
  where a=button closer to tip, b=button further from tip, and t=tip pressed on tablet \n\
</pre></body>";
#endif

    wxDialog dlg( NULL, wxID_ANY, wxString(_("Segment2d help")), wxDefaultPosition,
                  wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    wxHtmlWindow* html = new wxHtmlWindow( &dlg, wxID_ANY, wxDefaultPosition ); //,
                                           //wxSize(600, 400) ); //, wxHW_SCROLLBAR_NEVER );
    html->SetInitialSize( wxSize(800,600) );
    html->SetPage( help );
    sizer->Add( html, 1, wxALL | wxEXPAND, 10 );
    wxButton* but = new wxButton( &dlg, wxID_OK, _("OK") );
    but->SetDefault();
    sizer->Add( but, 0, wxALL | wxALIGN_RIGHT, 15 );
    dlg.SetSizer( sizer );
    sizer->Fit( &dlg );
    dlg.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right.
 *
 * Interactive2D
 * Previous    Next
 * SetIndex    GrayMap
 * Mode:       modeName (LWOF, Train, Paint, ILW, LiveSnake, SelFeatures, Review, Report, Peek, IntDL)
 * Reset       xDefaultFill
 * Object:     1..8 All
 * DelObj      LoadObj
 * SetOut      xLayout
 * xOverlay
 */
void Segment2dFrame::addButtonBox ( ) {
#if 0
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "Interactive2D" );
    ::setColor( m_buttonBox );
    m_buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    //row 1, col 1
    m_prev = new wxButton( mControlPanel, ID_PREVIOUS, "Previous", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_prev );
    fgs->Add( m_prev, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
    m_next = new wxButton( mControlPanel, ID_NEXT, "Next", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_next );
    fgs->Add( m_next, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
    m_setIndex = new wxButton( mControlPanel, ID_SET_INDEX, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_setIndex );
    fgs->Add( m_setIndex, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    m_grayMap = new wxButton( mControlPanel, ID_GRAYMAP, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_grayMap );
    fgs->Add( m_grayMap, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3
	mModeLabel = new wxStaticText( mControlPanel, wxID_ANY, "Mode:" );
	fgs->Add( mModeLabel, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
	wxArrayString sa;
	for (int j=0; j<canvas->num_detection_modes; j++)
		sa.Add(modeName[canvas->detection_modes[j]]);
	mMode = new wxComboBox( mControlPanel, ID_MODE, modeName[canvas->detection_mode], wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
	::setColor( mMode );
    fgs->Add( mMode, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    m_reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_reset );
    fgs->Add( m_reset, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 4, col 2
	m_defaultFill = new wxCheckBox( mControlPanel, ID_DEFAULT_FILL, "Default Fill" );
	::setColor( m_defaultFill );
	m_defaultFill->SetValue( canvas->default_mask_value!=0 );
	fgs->Add( m_defaultFill, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 5
	m_objectLabel = new wxStaticText( mControlPanel, wxID_ANY, "Object:" );
	fgs->Add( m_objectLabel, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	sa.Clear();
	for (int j=0; j<9; j++)
		sa.Add(objectName[j]);
	m_object = new wxComboBox( mControlPanel, ID_OBJECT, objectName[canvas->object_number], wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
	::setColor( m_object );
	fgs->Add( m_object, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 6, col 1
	m_deleteObject = new wxButton( mControlPanel, ID_DELETE_OBJECT, "Del. Obj.", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( m_deleteObject );
	fgs->Add( m_deleteObject, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 6, col 2
	m_loadObject = new wxButton( mControlPanel, ID_LOAD_OBJECT, "Load Obj.", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( m_loadObject );
	fgs->Add( m_loadObject, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 1
	m_setOutputBut = new wxButton( mControlPanel, ID_SET_OUTPUT, "Set Output", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( m_setOutputBut );
    fgs->Add( m_setOutputBut, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 2
	m_layout = new wxCheckBox( mControlPanel, ID_LAYOUT, "Layout" );
	::setColor( m_layout );
	m_layout->SetValue( canvas->layout_flag );
	fgs->Add( m_layout, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 8, col 1
	m_overlay = new wxCheckBox( mControlPanel, ID_OVERLAY, "Overlay" );
	::setColor( m_overlay );
	m_overlay->SetValue( canvas->overlay_flag );
	fgs->Add( m_overlay, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    m_buttonSizer->Add( fgs, 1, wxGROW|wxALL, 10 );
    mBottomSizer->Add( m_buttonSizer, 1, wxGROW|wxALL, 10 );
#else
    const int border = 5;
    const int columns = 4;
    mBottomSizer->Add( 0, 0, 1, wxALL );  //spacer
    //box for buttons
    m_buttonBox = new wxStaticBox( mControlPanel, wxID_ANY, "Interactive2D" );
    ::setBoxColor( m_buttonBox );
    auto buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxVERTICAL );
    buttonSizer->SetMinSize( mControlPanel->GetSize().x/5, 0 );

    auto gs = new wxGridSizer( columns, 5, 5 );
    //row 1, col 1
    auto prev = new CButton( m_buttonBox, ID_PREVIOUS, "Previous" );
    //::setColor( prev );
    gs->Add( prev, 1, wxEXPAND, border );
    //row 1, col 2
    auto b = new CButton( m_buttonBox, ID_NEXT, "Next" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );
    //row 2, col 1
    b = new CButton( m_buttonBox, ID_SET_INDEX, "Set Index" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );
    //row 2, col 2
    b = new CButton( m_buttonBox, ID_GRAYMAP, "Gray Map" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );
    //row 3, col 1
    auto st = new wxStaticText( m_buttonBox, wxID_ANY, "Mode:" );
    ::setBoxColor( st );
    gs->Add( st, 1, wxEXPAND|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, border );
    //row 3, col 2
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    wxArrayString sa;
    for (int j=0; j<canvas->num_detection_modes; j++) {
        cout << modeName[canvas->detection_modes[j]] << endl;
        sa.Add(modeName[canvas->detection_modes[j]]);
    }
    mMode = new CComboBox( m_buttonBox, ID_MODE, modeName[canvas->detection_mode], wxDefaultPosition, wxDefaultSize, sa, wxCB_READONLY );
    //mMode = new CComboBox( m_buttonBox, ID_MODE, modeName[canvas->detection_mode], wxDefaultPosition, wxDefaultSize, sa, wxCB_READONLY );
//    ::setColor( mMode );
    gs->Add( mMode, 1, wxEXPAND, border );
#if defined(wxUSE_TOOLTIPS) && !defined(__WXX11__)
    mMode->SetToolTip(  "disabled when Layout is checked" );
#endif
    //row 4, col 1
    b = new CButton( m_buttonBox, ID_RESET, "Reset" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );
    //row 4, col 2
    auto ch = new CCheckBox( m_buttonBox, ID_DEFAULT_FILL, _("Default Fill"), wxDefaultPosition, wxDefaultSize, 0 );
    //ch->SetValue(false);
    ch->SetValue( canvas->default_mask_value!=0 );
    m_defaultFill = ch;

    gs->Add( ch, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, border );
    //row 5, col 1
    st = new wxStaticText( m_buttonBox, wxID_ANY, "Object:" );
    ::setBoxColor( st );
    gs->Add( st, 1, wxEXPAND|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, border );
    //row 5, col 2
    sa.Clear();
    for (int j=0; j<9; j++)    sa.Add(objectName[j]);
    m_object = new wxComboBox( m_buttonBox, ID_OBJECT, objectName[canvas->object_number], wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
    ::setColor( m_object );
    gs->Add( m_object, 1, wxEXPAND, border );
    //row 6, col 1
    b = new CButton( m_buttonBox, ID_DELETE_OBJECT, "Del Obj" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );
    //row 6, col 2
    b = new CButton( m_buttonBox, ID_LOAD_OBJECT, "Load Obj" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );
    m_loadObject = b;  //needed later
    //row 7, col 1
    b = new CButton( m_buttonBox, ID_SET_OUTPUT, "Set Out" );
    //::setColor( b );
    gs->Add( b, 1, wxEXPAND, border );

    ch = new CCheckBox( m_buttonBox, ID_LAYOUT, _("Layout"), wxDefaultPosition, wxDefaultSize, 0 );
    ch->SetValue( canvas->layout_flag );
    m_layout = ch;
    gs->Add( ch, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, border );
    mMode->Enable( !canvas->layout_flag );  //other modes won't work when this is checked
#if defined(wxUSE_TOOLTIPS) && !defined(__WXX11__)
    ch->SetToolTip(  "when checked, other modes are disabled" );
#endif

    ch = new CCheckBox( m_buttonBox, ID_OVERLAY, _("Overlay"), wxDefaultPosition, wxDefaultSize, 0 );
    ch->SetValue( canvas->overlay_flag );
    m_overlay = ch;
    gs->Add( ch, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, border );

    //buttonSizer->Add( gs, 1, wxGROW|wxALL, 10 );
    //mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
    buttonSizer->Add( gs, 0, 0, 0 );
    //mBottomSizer->Add( buttonSizer, 0, 0, 0 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, border );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for Segment2dFrame class. */
Segment2dFrame::~Segment2dFrame ( ) {
    cout << "Segment2dFrame::~Segment2dFrame" << endl;
    wxLogMessage( "Segment2dFrame::~Segment2dFrame" );

return; //gjg: free nada!
#if wxUSE_DRAG_AND_DROP
    delete m_dropTarget;    m_dropTarget = nullptr;
#endif
    if (mCanvas != nullptr) {
        delete mCanvas;
        mCanvas = nullptr;
    }
	if (mAuxControls != nullptr) {
        delete mAuxControls;
        mAuxControls = nullptr;
    }
    deleteSegment2dControls();
#if 0
	if (m_overlay != nullptr) {
		fgs->Detach(m_overlay);
		delete m_overlay;
		m_overlay = nullptr;
	}
	if (m_layout != nullptr) {
		fgs->Detach(m_layout);
		delete m_layout;
		m_layout = nullptr;
	}
#endif
	if (m_setOutputBut!=NULL) {
		fgs->Detach(m_setOutputBut);
		delete m_setOutputBut;
		m_setOutputBut = NULL;
	}
	if (m_reset!=NULL) {
		fgs->Detach(m_reset);
		delete m_reset;
		m_reset = NULL;
	}
	if (mMode != nullptr) {
		if (fgs != nullptr)    fgs->Detach(mMode);
		delete mMode;
		mMode = nullptr;
	}
	if (m_grayMap!=NULL) {
		fgs->Detach(m_grayMap);
		delete m_grayMap;
		m_grayMap = NULL;
	}
	if (m_setIndex!=NULL) {
		fgs->Detach(m_setIndex);
		delete m_setIndex;
		m_setIndex = NULL;
	}
	if (m_next!=NULL) {
		fgs->Detach(m_next);
		delete m_next;
		m_next = NULL;
	}
	if (m_prev!=NULL) {
		fgs->Detach(m_prev);
		delete m_prev;
		m_prev = NULL;
	}
	if (fgs!=NULL) {
		m_buttonSizer->Remove(fgs);
		fgs = NULL;
	}
	if (m_buttonSizer!=NULL) {
		mBottomSizer->Remove(m_buttonSizer);
		m_buttonSizer = NULL;
	}
    if (mControlPanel!=NULL)  {
		mBottomSizer->Detach(mControlPanel);
		delete mControlPanel;
		mControlPanel=NULL;
	}
    if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }
#if 0  //this should happen in superclass (MainFrame) dtor
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
        /** \todo check. exit() call may bypass persistent saves. */
        ~MainFrame();
        exit(0);
    }
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new Segment2dFrame.  It first searches the input file history for an
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
void Segment2dFrame::createSegment2dFrame ( wxFrame* parentFrame, bool useHistory )
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
    //display a Segment2d frame using the specified file as input
    Segment2dFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new Segment2dFrame( parentFrame->IsMaximized(), w, h );
    } else {
        /** \todo save window location, window size, and maximized or not */
        frame = new Segment2dFrame(true);
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
bool Segment2dFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::flush_temp_data( void )
{
	Segment2dCanvas *canvas=dynamic_cast<Segment2dCanvas*>(mCanvas);
	if (canvas)
		canvas->save_mask_proc(0);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item. */
void Segment2dFrame::OnOpen ( wxCommandEvent& unused ) {
    createSegment2dFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "Segment2dFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "Segment2dFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0) {
        OnSegment2d( unused );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void Segment2dFrame::loadFile ( const char* const fname ) {
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
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    canvas->loadFile( fname );

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    
    Show();
    Raise();

    restoreControlSettings();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load (and subsequently display) data directly from memory
 *  (instead of from a file).
 *  \todo this is incomplete.
 */
void Segment2dFrame::loadData ( char* name,
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
    
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::deleteSegment2dControls(void)
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

    if (mIntDLControls != nullptr) {
        delete mIntDLControls;
        mIntDLControls = nullptr;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** restore/recall/reload general window frame settings (e.g., size, maximized etc.)
 *  \todo is this really necessary?
 */
void Segment2dFrame::restoreFrameSettings ( ) {
    cout << "Segment2dFrame::restoreFrameSettings() \n";
//    SetName( whatAmI() );  //important for persistence
    //mPersistentMe = wxPersistenceManager::Get().Register( this );  <-- never use this version
    //mPersistentMe = wxPersistenceManager::Get().Register( this, new PersistentSegment2dFrame(this) );
    bool ok = wxPersistenceManager::Get().Restore( this );
    assert( ok );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** restore/recall/reload graymap settings, graymap controls visible or
 *  or not, slice number displayed, position of displayed slice, etc.
 *  this is separate from restoreFrameSettings() until after data are
 *  loaded and displayed.
 *  \todo code to handle graymap should probably be refactored into one place
 *  (GrayMapControls?)
 */
void Segment2dFrame::restoreControlSettings ( ) {
    cout << "Segment2dFrame::restoreControlSettings()" << endl;
    if (!Preferences::getDejaVuMode()) return;
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    assert( canvas != nullptr );
    if (canvas == nullptr)    return;

    auto tmp = dynamic_cast<PersistentSegment2dFrame*>( mPersistentMe );
    assert( tmp != nullptr);
    if (tmp == nullptr)    return;
    tmp->restoreControlSettings();
#if 0
    cout << "Segment2dFrame::restoreControlSettings()" << endl;
    if (!Preferences::getDejaVuMode()) return;
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    assert( canvas != nullptr );
    if (canvas == nullptr)    return;

    //graymap controls -----
    //get level
    int level;
    bool levelOK = Preferences::getPersistence(
            GrayMapControls::levelGroupDefault, "value", level,
            canvas->getCenter(0) );
    if (levelOK) {
        cout << "level=" << level << endl;
        canvas->setCenter(0, level);
    }
    //get width
    int width;
    bool widthOK = Preferences::getPersistence(
            GrayMapControls::widthGroupDefault, "value", width,
            canvas->getWidth(0));
    if (widthOK) {
        cout << "width=" << width << endl;
        canvas->setWidth(0, width);
    }
    //get invert
    int invert;
    bool invertOK = Preferences::getPersistence(
            GrayMapControls::invertGroupDefault, "isChecked", invert,
            canvas->getInvert(0));
    if (invertOK) {
        cout << "invert=" << (invert != 0) << endl;
        canvas->setInvert(0, invert != 0);
    }
    //update contrast (if necessary)
    if (levelOK || widthOK || invertOK) {
        canvas->initLUT(0);
        canvas->reload();
    }
    //report any problems
    if (!levelOK || !widthOK || !invertOK) {
        cerr << "At least one widget not found in .cavass.ini." << endl
             << "Did you ever create the widget, and call wxPersistenceManager::Get().RegisterAndRestore(widget); to create the entry in .cavass.ini?"
             << endl;
    }

    //paint controls -----
    //get brush size
    /** \todo paint controls */
    string brushSize;
    string dflt = "2";
    bool ok = Preferences::getPersistence( Segment2dAuxControls::paintGroup,
                                           "stringSelection", brushSize, dflt );

    //for train controls -----
    //get brush size
    /** \todo train controls */
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Previous button press.  display the previous slice.
 */
void Segment2dFrame::OnPrevious ( wxCommandEvent& unused ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) - 1;
    if (slice<0)
		slice = canvas->getNoSlices(0) - 1;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Next button press.  display the next slice. */
void Segment2dFrame::OnNext ( wxCommandEvent& unused ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) + 1;
    if (slice >= canvas->getNoSlices(0))
		slice = 0;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnSetIndex ( wxCommandEvent& unused ) {
    if (mSetIndexControls != nullptr) {
        delete mSetIndexControls;
        mSetIndexControls = nullptr;
        mBottomSizer->Layout();
        mControlPanel->Refresh();
        return;
    }
	deleteSegment2dControls();
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    mSetIndexControls = new SetIndexControls( mControlPanel,
	    mBottomSizer, "Set Index", canvas->getSliceNo(0),
		canvas->getNoSlices(0), ID_SLICE_SLIDER, ID_SCALE_SLIDER,
		canvas->getScale(), ID_LABELS, wxID_ANY );
}

/**
 * \brief what occurs when the reset button is pressed depends on the current
 * detection mode.
 * report: report position on canvas is reset (to 0,0)
 * review: position of images on canvas is reset (to 0,0)
 * sel_features: \todo ?
 * training: the position of the image on the canvas is reset (to center,top)
 * default: \todo ?
 * @param unused is not used
 */
void Segment2dFrame::OnReset( wxCommandEvent& unused )
{
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);

	int iw, ih;
	canvas->GetSize(&iw, &ih);
	switch (canvas->detection_mode) {
	  case Segment2dCanvas::SEL_FEATURES:
		canvas->Reset_training_proc(2);
		if (canvas->detection_mode == Segment2dCanvas::SEL_FEATURES)    delete mAuxControls;
		mAuxControls = new Segment2dAuxControls( this, mControlPanel,
			mBottomSizer, "Feature Controls", canvas->Range,
			canvas->curr_feature,
			canvas->temp_list[canvas->curr_feature].status,
			canvas->temp_list[canvas->curr_feature].transform,
			canvas->temp_list[canvas->curr_feature].weight,
			canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
			canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
			canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
			canvas->temp_list[canvas->curr_feature].rmax*canvas->Range );
		canvas->mTx_f = (int)(iw/2-canvas->getScale()*canvas->mCavassData->m_xSize);
		canvas->mTy_f = 0;
		canvas->Reset_training_proc(1);
		break;
	  case Segment2dCanvas::TRAINING:
	    canvas->mTx = (int)(iw-canvas->getScale()*canvas->mCavassData->m_xSize)/2;
	    canvas->mTy = 0;
		break;
	  case Segment2dCanvas::REPORT:
	    canvas->mTx_p = canvas->mTy_p = 0;
		break;
	  case Segment2dCanvas::REVIEW:
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
	    break;
	}  //end switch
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 */
void Segment2dFrame::OnGrayMap ( wxCommandEvent& unused ) {
    if (mGrayMapControls != nullptr) {
        delete mGrayMapControls;
        mGrayMapControls = nullptr;
        mBottomSizer->Layout();
        mControlPanel->Refresh();
        return;
    }
    deleteSegment2dControls();
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void Segment2dFrame::OnCenterSlider ( wxScrollEvent& e ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider (used to change contrast). */
void Segment2dFrame::OnWidthSlider ( wxScrollEvent& e ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void Segment2dFrame::OnCTLung ( wxCommandEvent& unused ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
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
void Segment2dFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
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
void Segment2dFrame::OnCTBone ( wxCommandEvent& unused ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
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
void Segment2dFrame::OnPET ( wxCommandEvent& unused ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
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
void Segment2dFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& unused ) {
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
void Segment2dFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& unused ) {
    if (m_width->GetValue() == mCanvas->getWidth())    return;
    mCanvas->setWidth( m_width->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set  */
void Segment2dFrame::OnSliceSlider ( wxScrollEvent& e ) {
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set  */
void Segment2dFrame::OnScaleSlider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue / 100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndexControls->setScaleText( s );
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * callback for labels checkbox.
 * turn slice number labels on and off.
 * used by set index control.
 */
void Segment2dFrame::OnLabels ( wxCommandEvent& e ) {
    doLabels( e.IsChecked() );
}

/**
 * in addition to event handling, this can also be called from persistence.
 * turn slice number labels on and off.
 * used by set index control.
 */
void Segment2dFrame::doLabels ( bool isChecked ) {
    if (mSetIndexControls != nullptr) {
        mSetIndexControls->mCB->SetValue( isChecked );  //only necessary when not called from event handler (viz., persistence)
    }
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->setLabels( isChecked );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * callback for overlay checkbox.
 * causes overlay (i.e., segmentation mask) on and off.
 */
void Segment2dFrame::OnOverlay ( wxCommandEvent& e ) {
    //doOverlay( e.IsChecked() );
    doOverlay( m_overlay->GetValue() );
}

/**
 * set state (on/off) of overlay (i.e., segmentation mask).
 * in addition to event handling, this can also be called from persistence.
 */
void Segment2dFrame::doOverlay ( bool isChecked ) {
    m_overlay->SetValue( isChecked );  //only necessary when not called from event handler (viz., persistence)
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->overlay_flag = isChecked;
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** callback for layout checkbox. when this mode is enabled, the
 *  others (e.g., paint, etc.) are disabled (and vice versa).
 */
void Segment2dFrame::OnLayout ( wxCommandEvent& ignored ) {
    //doLayout( e.IsChecked() );
    doLayout( m_layout->GetValue() );
}

/** in addition to event handling, this can also be called from persistence. */
void Segment2dFrame::doLayout ( bool isChecked ) {
    m_layout->SetValue( isChecked );  //only necessary when not called from event handler (viz., persistence)
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->layout_flag = isChecked;
    canvas->reload();
#if 0
    if (canvas->layout_flag && mSetIndexControls==nullptr) {
        wxCommandEvent unused;
        OnSetIndex( unused );
    }
#endif
    //when this is checked (i.e., when in this additional "mode"), disable mode combo box so the user isn't confused.
    mMode->Enable( !canvas->layout_flag );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set  */
void Segment2dFrame::OnInvert ( wxCommandEvent& e ) {
    bool  value  = mGrayMapControls->m_cb_invert->GetValue();
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void Segment2dFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
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
void Segment2dFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    Segment2dCanvas*     canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    MainPrint          printout( mModuleName.c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * this function is called when the 'type' button is pressed in the Set Output
 * controls. it causes the type to be advanced to the next one in sequence
 * (with wraparound to the beginning). BIM --> IM0 --> PAR --> BIM ...
 */
void Segment2dFrame::OnOutputType ( wxCommandEvent& unused ) {
	auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
	if (canvas->output_type == BINARY)       canvas->output_type = GREY;
	else if (canvas->output_type == GREY)    canvas->output_type = GREY + 1;
	else                                     canvas->output_type = BINARY;
	mSetOutputControls->SetOutType( canvas->output_type );
}

void Segment2dFrame::doOutputType ( const wxString& type ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    if (type == "Type: BIM") {
        canvas->output_type = BINARY;
        mSetOutputControls->SetOutType( canvas->output_type );
    } else if (type == "Type: IM0") {
        canvas->output_type = GREY;
        mSetOutputControls->SetOutType( canvas->output_type );
    } else if (type == "Type: PAR") {
        canvas->output_type = GREY + 1;
        mSetOutputControls->SetOutType( canvas->output_type );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnSetOutput ( wxCommandEvent& unused )
{
	if (mSetOutputControls != nullptr) {
		delete mSetOutputControls;
		mSetOutputControls = nullptr;
        mBottomSizer->Layout();
        mControlPanel->Refresh();
        return;
	}
	deleteSegment2dControls();
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
	mSetOutputControls = new SetSegment2dOutputControls( mControlPanel,
		mBottomSizer, "Set Output", ID_SAVE, ID_OUT_OBJECT, (out_object+1)%9,
        canvas->output_type );

    //restoreSetOutputControls
    if (Preferences::getDejaVuMode()) {
        auto tmp = dynamic_cast<PersistentSegment2dFrame *>( mPersistentMe );
        if (tmp == nullptr)    return;
        tmp->restoreSetOutputControls();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnSegment2dSave(wxCommandEvent &e)
{
    Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
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
				      for (int i=0; i<MAX_NUM_FEATURES; i++)
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief called in response to a mode combo box selection event. */
void Segment2dFrame::OnMode( wxCommandEvent& e ) {
    doMode( e.GetString() );
}
/**
 * \brief called in response to a mode combo box selection event OR called to
 * restore persistence values.
 * @param e_GetString is the new mode that was selected (e.g., "Train").
 */
void Segment2dFrame::doMode ( const wxString& e_GetString ) {
    //set combo box selection to specified string.
    // (only necessary when _not_ called from event handler (viz., persistence).)
    int which = mMode->FindString( e_GetString );
    if (which == wxNOT_FOUND) {
        cerr << "Segment2dFrame::doMode: can't select this mode - " << e_GetString << "." << endl;
        return;
    }
    mMode->SetSelection( which );

    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    assert( canvas != nullptr );
    if (mAuxControls) {
        delete mAuxControls;
        mAuxControls = nullptr;
    }
    if (mIntDLControls) {
        delete mIntDLControls;
        mIntDLControls = nullptr;
    }
    canvas->ResetPeekPoints();
    if (canvas->detection_mode == Segment2dCanvas::TRAINING) {  //was old mode training?
        if (canvas->training_phase == 1) {
            canvas->NumPoints=0;
            if (canvas->temp_contours.last<=0 && canvas->o_contour.last>=0) {
                canvas->copy_ocontour_into_temp_array();
                canvas->o_contour.last = -1;
            }
        }
        canvas->training_phase = 0;
        canvas->Run_Statistics();
    }
    //remember if the old mode was review (or not)
    bool was_review = canvas->detection_mode==Segment2dCanvas::REVIEW;
    //find this new mode name in the 'list'
    for (canvas->detection_mode = 1;
         canvas->detection_mode < Segment2dCanvas::ROI;
         canvas->detection_mode++)
    {
        if (modeName[canvas->detection_mode] == e_GetString) {
            break;
        }
    }

    if (canvas->detection_mode == Segment2dCanvas::REVIEW) {
        delete mSetIndexControls;
        mSetIndexControls = nullptr;
        canvas->save_mask_proc(0);
        canvas->setScale(canvas->getScale());
    } else if (canvas->detection_mode==Segment2dCanvas::SEL_FEATURES ||
               canvas->mCols>1)
    {
        canvas->freeImagesAndBitmaps();
        canvas->mCols =
                (canvas->detection_mode==Segment2dCanvas::SEL_FEATURES) ? 2 : 1;
        m_loadObject->SetLabel("Load Obj.");
    }
    if (was_review) {
        delete mSetIndexControls;    mSetIndexControls = nullptr;
        canvas->save_mask_proc(0);
        canvas->object_mask_slice_index =
                canvas->sl.slice_index[0][canvas->mCavassData->m_sliceNo];
        canvas->load_object_mask(canvas->object_mask_slice_index);
        canvas->setScale(canvas->getScale());
    }
    if (canvas->painting && (canvas->detection_mode==Segment2dCanvas::ILW ||
                             canvas->detection_mode==Segment2dCanvas::LSNAKE))
    {
        canvas->Reset_training_proc(0);
        canvas->painting = false;
    }
    switch (canvas->detection_mode) {
        case Segment2dCanvas::TRAINING:
            mAuxControls = new Segment2dAuxControls( this, mControlPanel,
                                                     mBottomSizer, (modeName[canvas->detection_mode]+" Controls").c_str(),
                                                     canvas->train_brush_size, true );
            mAuxControls->restoreAuxControls();
            break;
        case Segment2dCanvas::PAINT:
            mAuxControls = new Segment2dAuxControls( this, mControlPanel,
                                                     mBottomSizer, (modeName[canvas->detection_mode]+" Controls").c_str(),
                                                     canvas->paint_brush_size );
            mAuxControls->restoreAuxControls();
            break;
        case Segment2dCanvas::ILW:
            mAuxControls = new Segment2dAuxControls( this, mControlPanel,
                                                     mBottomSizer, "ILW Controls", canvas->ilw_iterations,
                                                     canvas->ilw_min_pts );
            mAuxControls->restoreAuxControls();
            break;
        case Segment2dCanvas::LSNAKE:
            mAuxControls = new Segment2dAuxControls( this, mControlPanel,
                                                     mBottomSizer, "LiveSnake Controls", canvas->ilw_iterations,
                                                     canvas->lsnake_alpha,canvas->lsnake_beta,canvas->lsnake_gamma );
            mAuxControls->restoreAuxControls();
            break;
        case Segment2dCanvas::SEL_FEATURES:
            mAuxControls = new Segment2dAuxControls( this, mControlPanel,
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
            mAuxControls->restoreAuxControls();
            m_loadObject->SetLabel("Load Par");
            break;
        case Segment2dCanvas::INT_DL:
            mIntDLControls = new Segment2dIntDLControls( this, mControlPanel, mBottomSizer );
            SetStatusText( "choose a model; select a rectangle; run", 0 );
            SetStatusText( "select", 2 );  //blows up if i put this in the ctor above!
            SetStatusText( "", 3 );
            SetStatusText( "", 4 );
            break;
    }
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * called in response to changes in object no. (1..8 All) combo box in
 * Interactive2D controls (not Set Index).
 * */
void Segment2dFrame::OnObject ( wxCommandEvent& e ) {
    doObject( e.GetString() );
}

void Segment2dFrame::doObject ( const wxString& e_GetString ) {
    //set combo box selection to specified string.
    // (only necessary when not called from event handler (viz., persistence).)
    int which = m_object->FindString( e_GetString );
    if (which != wxNOT_FOUND)    m_object->SetSelection( which );

    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
    canvas->object_number = e_GetString == "All" ? 8
                                                 : atoi((const char *)e_GetString.c_str())-1;
    for (int j=0; j<6; j++) {
        canvas->temp_list[j] = canvas->accepted_list[canvas->object_number % 8][j];
    }
    canvas->ReCalc_Edge_Costs();
    canvas->reload();
    if (canvas->detection_mode == Segment2dCanvas::SEL_FEATURES) {
        delete mAuxControls;
        mAuxControls = new Segment2dAuxControls( this, mControlPanel,
                               mBottomSizer, "Feature Controls",
                               canvas->Range, canvas->curr_feature,
                               canvas->temp_list[canvas->curr_feature].status,
                               canvas->temp_list[canvas->curr_feature].transform,
                               canvas->temp_list[canvas->curr_feature].weight,
                               canvas->temp_list[canvas->curr_feature].rmean * (float)canvas->Range,
                               canvas->temp_list[canvas->curr_feature].rstddev * (float)canvas->Range,
                               canvas->temp_list[canvas->curr_feature].rmin * (float)canvas->Range,
                               canvas->temp_list[canvas->curr_feature].rmax * (float)canvas->Range );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnDeleteObject( wxCommandEvent& unused )
{
	Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
	canvas->Reset_object_proc();
}

void Segment2dFrame::OnLoadObject( wxCommandEvent& unused )
{
	Segment2dCanvas*  canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
	if (canvas->detection_mode == Segment2dCanvas::SEL_FEATURES)
	{
		wxString filename = wxFileSelector( _T("Select parameter file"),
			_T(""), _T(""), _T(""), "PAR files (*.PAR)|*.PAR",
			wxFILE_MUST_EXIST );
		if (!filename)
			return;
		canvas->LoadFeatureList((const char *)filename.c_str());
		canvas->reload();
		delete mAuxControls;
		mAuxControls = new Segment2dAuxControls( this, mControlPanel,
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

void Segment2dFrame::OnDefaultFill ( wxCommandEvent& e ) {
    //doDefaultFill( e.IsChecked() );
    doDefaultFill( m_defaultFill->GetValue() );
}

void Segment2dFrame::doDefaultFill ( bool isChecked ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->default_mask_value = isChecked ? 255 : 0;
}

void Segment2dFrame::OnOutputObject( wxCommandEvent& e )
{
	out_object = e.GetString()=="All"? 8: atoi((const char *)e.GetString().c_str())-1;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnBrushSize ( wxCommandEvent& e ) {
    doBrushSize( e.GetString().c_str() );
}

void Segment2dFrame::doBrushSize ( const char* const c_str ) {
    const int n = 5;
    char brush[n];
    strncpy(brush, c_str, n);
    assert( brush[n-1] == 0 );
    if (brush[n-1] != 0) {
        cerr << "Segment2dFrame::doBrushSize: bad brush size, " << c_str << endl;
        return;
    }

    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    if (canvas->detection_mode == Segment2dCanvas::TRAINING) {
        if (strcmp(brush, "X") == 0)
            canvas->train_brush_size = 0;
        else if (strcmp(brush, "Fill") == 0)
            canvas->train_brush_size = -1;
        else {
            //canvas->train_brush_size = atoi(brush);
            char* ptr = nullptr;
            int tmp = (int) strtol( brush, &ptr, 10 );
            if (*ptr != 0) {  //unsuccessful conversion
                cerr << "Segment2dFrame::doBrushSize: bad brush size, " << brush << endl;
                return;
            }
            canvas->train_brush_size = tmp;
        }
    } else {
        assert( canvas->detection_mode == Segment2dCanvas::PAINT );
        //SetStatusText( canvas->paint_brush_size==0 ? "DONE" : "---", 4);
        if (strcmp(brush, "X") == 0)
            canvas->paint_brush_size = 0;
        else if (strcmp(brush, "Fill") == 0)
            canvas->paint_brush_size = -1;
        else {
            //canvas->paint_brush_size = atoi(brush);
            char* ptr = nullptr;
            int tmp = (int) strtol( brush, &ptr, 10 );
            if (*ptr != 0) {  //unsuccessful conversion
                cerr << "Segment2dFrame::doBrushSize: bad brush size, " << brush << endl;
                return;
            }
            canvas->paint_brush_size = tmp;
        }
    }

    //brush size ok. set combo box selection to specified string.
    // (only necessary when not called from event handler (viz., persistence).)
    wxString s = c_str;
    int which = mAuxControls->mBrushSize->FindString( s );
    if (which != wxNOT_FOUND)    mAuxControls->mBrushSize->SetSelection( which );
    else                         cerr << "Segment2dFrame::doBrushSize: bad combobox selection." << endl;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnIterates ( wxCommandEvent& e ) {
    doIterates( e.GetString() );
}

/** ILW or LSNAKE only */
void Segment2dFrame::doIterates ( const wxString& e_GetString ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    if (canvas->detection_mode == Segment2dCanvas::ILW) {
        canvas->ilw_iterations = atoi( (const char*) e_GetString.c_str() );
        //set combo box selection to specified string.
        // (only necessary when not called from event handler (viz., persistence).)
        int which = mAuxControls->mIterates->FindString( e_GetString );
        if (which != wxNOT_FOUND)    mAuxControls->mIterates->SetSelection( which );
    } else if (canvas->detection_mode == Segment2dCanvas::LSNAKE) {
        for (canvas->ilw_iterations = 1; canvas->ilw_iterations < 6 ?
                                         canvas->ilw_iterations < atoi((const char*) e_GetString.c_str()) :
                                         (canvas->ilw_iterations - 4) * 5 < atoi((const char*) e_GetString.c_str()); )
        {
            canvas->ilw_iterations++;
        }
        //set combo box selection to specified string.
        // (only necessary when not called from event handler (viz., persistence).)
        int which = mAuxControls->mIterates->FindString( e_GetString );
        if (which != wxNOT_FOUND)    mAuxControls->mIterates->SetSelection( which );
    } else {
        cerr << "Segment2dFrame::doIterates: bad mode" << endl;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * note: ilw_min_pts, mAlphaCtrl, mBetaCtrl, and mGammaCtrl are all text
 * controls. any call to SetValue will cause the event handler to be called.
 * therefore, OnMinPoints can't call doMinPoints (for example) because an
 * infinite sequence of calls will result (similar to unbounded recursion):
 *     doMinPoints -> OnMinPoints -> doMinPoints -> OnMinPoints -> ...
 * (this is the behavior on linuxmint w/ gtk. \todo check other controls on other os's.)
 */
void Segment2dFrame::OnMinPoints ( wxCommandEvent& e ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->ilw_min_pts = atoi( (const char*)e.GetString().c_str() );
}
/**
 * careful: mMinPointsCtrl is a text control. a call to SetValue will cause
 * the OnMinPoints event handler to be called.
 */
void Segment2dFrame::doMinPoints ( const wxString& e_GetString ) {
    //set text to specified string
    mAuxControls->mMinPointsCtrl->SetValue( e_GetString );
    //above causes OnMinPoints to be called which will execute the following:
    //    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    //    canvas->ilw_min_pts = atoi( (const char*)e_GetString.c_str() );
}

void Segment2dFrame::OnAlpha ( wxCommandEvent& e ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    sscanf( (const char*)e.GetString().c_str(), "%lf", &canvas->lsnake_alpha );
}
/**
 * careful: mAlphaCtrl is a text control. a call to SetValue will cause
 * the OnAlpha event handler to be called.
 */
void Segment2dFrame::doAlpha ( const wxString& e_GetString ) {
    //set text to specified string
    mAuxControls->mAlphaCtrl->SetValue( e_GetString );  //causes OnAlpha to be called
    //above causes OnAlpha to be called which will execute the following:
    //    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    //    sscanf( (const char*)e_GetString.c_str(), "%lf", &canvas->lsnake_alpha );
}

void Segment2dFrame::OnBeta ( wxCommandEvent& e ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    sscanf( (const char*)e.GetString().c_str(), "%lf", &canvas->lsnake_beta );
}
/**
 * careful: mBetaCtrl is a text control. a call to SetValue will cause
 * the OnBeta event handler to be called.
 */
void Segment2dFrame::doBeta ( const wxString& e_GetString ) {
    //set text to specified string
    mAuxControls->mBetaCtrl->SetValue( e_GetString );  //causes OnBeta to be called
    //above causes OnBeta to be called which will execute the following:
    //    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    //    sscanf( (const char*)e_GetString.c_str(), "%lf", &canvas->lsnake_beta );
}

void Segment2dFrame::OnGamma ( wxCommandEvent& e ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    sscanf( (const char*)e.GetString().c_str(), "%lf", &canvas->lsnake_gamma );
}
/**
 * careful: mGammaCtrl is a text control. a call to SetValue will cause
 * the OnGamma event handler to be called.
 */
void Segment2dFrame::doGamma ( const wxString& e_GetString ) {
    //set text to specified string
    mAuxControls->mGammaCtrl->SetValue( e_GetString );  //causes OnGamma to be called
    //above causes OnGamma to be called which will execute the following:
    //    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    //    sscanf( (const char*)e_GetString.c_str(), "%lf", &canvas->lsnake_gamma );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnFeature ( wxCommandEvent& e ) {
    doFeature( e.GetString() );
#if 0
	auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    //find the selected feature in the list
	for (canvas->curr_feature = 0;
			e.GetString() != Segment2dFrame::featureNames[canvas->curr_feature];
			canvas->curr_feature++)
		;
    int i = e.GetSelection();
    assert( canvas->curr_feature == i );
    canvas->curr_feature = i;

	mAuxControls->SetFeature(canvas->curr_feature,
		0!=canvas->temp_list[canvas->curr_feature].status,
		canvas->temp_list[canvas->curr_feature].transform, canvas->Range,
		canvas->temp_list[canvas->curr_feature].weight,
		canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmax*canvas->Range);
#endif
}
void Segment2dFrame::doFeature ( const wxString& e_GetString ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    if (canvas == nullptr)    return;

    //find the specified string in the array
    int i;
    for (i=0; i<featureNames.GetCount(); i++) {
        if (e_GetString == featureNames[i])    break;
    }
    assert( 0 <= i && i < featureNames.GetCount() );
    assert( featureNames[i] == e_GetString );
    canvas->curr_feature = i;
    mAuxControls->mFeature->SetSelection( i );  //not necessary when called in response to event handling (only persistence)
#if 0
    mAuxControls->SetFeature( canvas->curr_feature,
                             0!=canvas->temp_list[canvas->curr_feature].status,
                             canvas->temp_list[canvas->curr_feature].transform, canvas->Range,
                             canvas->temp_list[canvas->curr_feature].weight,
                             canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
                             canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
                             canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
                             canvas->temp_list[canvas->curr_feature].rmax*canvas->Range );
#endif
    mAuxControls->SetFeature( i,
                              0 != canvas->temp_list[i].status,
                              canvas->temp_list[i].transform,
                              canvas->Range,
                              canvas->temp_list[i].weight,
                              canvas->temp_list[i].rmean * (double)canvas->Range,
                              canvas->temp_list[i].rstddev * (double)canvas->Range,
                              canvas->temp_list[i].rmin * (double)canvas->Range,
                              canvas->temp_list[i].rmax * (double)canvas->Range );
    canvas->reload();
}

void Segment2dFrame::OnFeatureStatus ( wxCommandEvent& e ) {
    doFeatureStatus( e.IsChecked() );
}
void Segment2dFrame::doFeatureStatus ( bool e_IsChecked ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    canvas->temp_list[ canvas->curr_feature ].status = e_IsChecked;
    //set checkbox appropriately (not necessary when call in response to an event, only for persistence)
    mAuxControls->mFeatureStatus->SetValue( e_IsChecked );
    canvas->reload();
}

void Segment2dFrame::OnTransform ( wxCommandEvent& e ) {
    doTransform( e.GetString() );
}
void Segment2dFrame::doTransform ( const wxString& e_GetString ) {  /** similar to doFeature() */
	auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );
    if (canvas == nullptr)    return;
#if 0
	for (canvas->temp_list[canvas->curr_feature].transform = 0;
			e_GetString != Segment2dFrame::transformNames[ canvas->temp_list[canvas->curr_feature].transform ];
			canvas->temp_list[canvas->curr_feature].transform++)
		;
	mAuxControls->SetTransform(
		canvas->temp_list[canvas->curr_feature].transform, canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmean*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmin*canvas->Range,
		canvas->temp_list[canvas->curr_feature].rmax*canvas->Range);
#endif
    //find the specified string in the array
    int i;
    for (i=0; i<transformNames.GetCount(); i++) {
        if (e_GetString == transformNames[i])    break;
    }
    assert( 0 <= i && i < transformNames.GetCount() );
    assert( transformNames[i] == e_GetString );
    canvas->temp_list[canvas->curr_feature].transform = i;
    mAuxControls->mTransform->SetSelection( i );  //not necessary when called in response to event handling (only persistence)
    mAuxControls->SetTransform( i,
                                 canvas->Range,
                                 canvas->temp_list[canvas->curr_feature].rmean * (double)canvas->Range,
                                 canvas->temp_list[canvas->curr_feature].rstddev * (double)canvas->Range,
                                 canvas->temp_list[canvas->curr_feature].rmin * (double)canvas->Range,
                                 canvas->temp_list[canvas->curr_feature].rmax * (double)canvas->Range );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnFeatureUpdate( wxCommandEvent& unused )
{
	Segment2dCanvas *canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);
	for (int k=0; k<8; k++)
		if (canvas->object_number==k || canvas->object_number==8)
			for (int j=0; j<6; j++)
				canvas->accepted_list[k][j] = canvas->temp_list[j];
	canvas->ReCalc_Edge_Costs();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dFrame::OnWeight ( wxScrollEvent& unused ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );

    //get value from event
    //canvas->temp_list[canvas->curr_feature].weight = (float)(e.GetPosition() / 100.0);  //value from event

    //get value directly from slider
    canvas->temp_list[canvas->curr_feature].weight = (float)(mAuxControls->mWeight->mSlider->GetValue() / 100.0);

    mAuxControls->UpdateWeight( canvas->temp_list[canvas->curr_feature].weight );
    canvas->reload();
}
/**
 * two digits of precision
 * @param value is the actual f.p. value as a string
 */
void Segment2dFrame::doWeight ( const wxString& value ) {
    double d;
    if (!value.ToDouble( &d )) {
        cerr << "Segment2dFrame::doWeight: bad double value -> " << value << endl;
        return;
    }
    mAuxControls->mWeight->mSlider->SetValue( (int)(d*100) );  //update slider
    wxScrollEvent unused;    OnWeight( unused );  //update data
}

void Segment2dFrame::OnMean ( wxScrollEvent& unused ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );

    //get value from event
    //canvas->temp_list[canvas->curr_feature].rmean = (float)(e.GetPosition() / (100.0*canvas->Range));

    //get value directly from slider
    canvas->temp_list[canvas->curr_feature].rmean = (float)(mAuxControls->mMean->mSlider->GetValue() / (100.0*canvas->Range));

    mAuxControls->UpdateMean( canvas->temp_list[canvas->curr_feature].rmean * canvas->Range );
    canvas->reload();
}
/**
 * two digits of precision
 * @param value is the actual f.p. value as a string
 */
void Segment2dFrame::doMean ( const wxString& value ) {
    double d;
    if (!value.ToDouble( &d )) {
        cerr << "Segment2dFrame::doMean: bad double value -> " << value << endl;
        return;
    }
    mAuxControls->mMean->mSlider->SetValue( (int)(d*100) );  //update slider
    wxScrollEvent unused;    OnMean( unused );  //update data
}

void Segment2dFrame::OnStdDev ( wxScrollEvent& unused ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);

    //get value from event
    //canvas->temp_list[canvas->curr_feature].rstddev = (float)(e.GetPosition() / (100.0*canvas->Range));

    //get value directly from slider
    canvas->temp_list[canvas->curr_feature].rstddev = (float)(mAuxControls->mStdDev->mSlider->GetValue() / (100.0*canvas->Range));

    mAuxControls->UpdateStdDev( canvas->temp_list[canvas->curr_feature].rstddev*canvas->Range );
    canvas->reload();
}
/**
 * two digits of precision
 * @param value is the actual f.p. value as a string
 */
void Segment2dFrame::doStdDev ( const wxString& value ) {
    double d;
    if (!value.ToDouble( &d )) {
        cerr << "Segment2dFrame::doStdDev: bad double value -> " << value << endl;
        return;
    }
    mAuxControls->mStdDev->mSlider->SetValue( (int)(d*100) );  //update slider
    wxScrollEvent unused;    OnStdDev( unused );  //update data
}

void Segment2dFrame::OnFeatureMin ( wxScrollEvent& unused ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>(mCanvas);

    //get value from event
    //canvas->temp_list[canvas->curr_feature].rmin = (float)(e.GetPosition() / (100.0*canvas->Range));

    //get value directly from slider
    canvas->temp_list[canvas->curr_feature].rmin = (float)(mAuxControls->mFeatureMin->mSlider->GetValue() / (100.0*canvas->Range));

    mAuxControls->UpdateFeatureMin(canvas->temp_list[canvas->curr_feature].rmin * canvas->Range);
    canvas->reload();
}
/**
 * two digits of precision
 * @param value is the actual f.p. value as a string
 */
void Segment2dFrame::doFeatureMin ( const wxString& value ) {
    double d;
    if (!value.ToDouble( &d )) {
        cerr << "Segment2dFrame::doFeatureMin: bad double value -> " << value << endl;
        return;
    }
    mAuxControls->mFeatureMin->mSlider->SetValue( (int)(d*100) );  //update slider
    wxScrollEvent unused;    OnFeatureMin( unused );  //update data
}

void Segment2dFrame::OnFeatureMax ( wxScrollEvent& unused ) {
    auto canvas = dynamic_cast<Segment2dCanvas*>( mCanvas );

    //get value from event
    //canvas->temp_list[canvas->curr_feature].rmax = (float)(e.GetPosition() / (100.0*canvas->Range));

    //get value directly from slider
    canvas->temp_list[canvas->curr_feature].rmax = (float)(mAuxControls->mFeatureMax->mSlider->GetValue() / (100.0*canvas->Range));

    mAuxControls->UpdateFeatureMax(canvas->temp_list[canvas->curr_feature].rmax * canvas->Range);
	canvas->reload();
}
/**
 * two digits of precision
 * @param value is the actual f.p. value as a string
 */
void Segment2dFrame::doFeatureMax ( const wxString& value ) {
    double d;
    if (!value.ToDouble( &d )) {
        cerr << "Segment2dFrame::doFeatureMax: bad double value -> " << value << endl;
        return;
    }
    mAuxControls->mFeatureMax->mSlider->SetValue( (int)(d*100) );  //update slider
    wxScrollEvent unused;    OnFeatureMax( unused );  //update data
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Allow the user to scroll through the slices with the mouse wheel. */
void Segment2dFrame::OnMouseWheel ( wxMouseEvent& e ) {
	const int  rot   = e.GetWheelRotation();
	wxCommandEvent  ce;
	if (rot>0)         OnPrevious(ce);
	else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - -
/** must be overridden by frames that persist the sash position */
void Segment2dFrame::OnMaximize ( wxMaximizeEvent& unused ) {
    //cout << "Segment2dFrame::OnMaximize()" << endl;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @param unused is not used
 */
void Segment2dFrame::OnIntDLAdd ( wxCommandEvent& unused ) {
    mIntDLControls->doAdd();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @param unused is not used
 */
void Segment2dFrame::OnIntDLBlink ( wxCommandEvent& unused ) {
    mIntDLControls->doBlink();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @param unused is not used
 */
void Segment2dFrame::OnIntDLChoose ( wxCommandEvent& unused ) {
    //display a dialog that allows the user to choose an input file
    auto str = wxFileSelector( _T("Select model/checkpoint file"), _T(""), _T(""),
                               _T(""),
                               /** \todo check if upper/lower case mix is necessary on win and mac */
                               "model files (*.pt;*.pth;*.PT;*.PTH)|*.pt;*.pth;*.PT;*.PTH",
                               wxFILE_MUST_EXIST );
    mIntDLControls->doChoose( str );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @param unused is not used
 */
void Segment2dFrame::OnIntDLClear ( wxCommandEvent& unused ) {
    mIntDLControls->doClear();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * \brief allow user to run dl seg.
 *
 * @param unused is not used
 */
void Segment2dFrame::OnIntDLRun ( wxCommandEvent& unused ) {
    mIntDLControls->doRun();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Segment2dFrame::get_object_selection_number ( ) {
    return m_object->GetSelection();
}

void Segment2dFrame::refresh_object_selection ( ) {
    doObject( m_object->GetStringSelection() );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( Segment2dFrame, wxFrame )
BEGIN_EVENT_TABLE       ( Segment2dFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_GRAYMAP,          Segment2dFrame::OnGrayMap    )
  EVT_MOUSEWHEEL(                  Segment2dFrame::OnMouseWheel )
  EVT_BUTTON( ID_NEXT,             Segment2dFrame::OnNext       )
  EVT_BUTTON( ID_PREVIOUS,         Segment2dFrame::OnPrevious   )
  EVT_BUTTON( ID_SET_INDEX,        Segment2dFrame::OnSetIndex   )
  EVT_BUTTON( ID_RESET,            Segment2dFrame::OnReset      )
  EVT_COMBOBOX( ID_OBJECT,         Segment2dFrame::OnObject     )
  EVT_BUTTON( ID_DELETE_OBJECT,    Segment2dFrame::OnDeleteObject)
  EVT_BUTTON( ID_LOAD_OBJECT,      Segment2dFrame::OnLoadObject )
  EVT_CHECKBOX( ID_DEFAULT_FILL,   Segment2dFrame::OnDefaultFill)
  EVT_CHECKBOX( ID_OVERLAY,        Segment2dFrame::OnOverlay    )
  EVT_CHECKBOX( ID_LAYOUT,         Segment2dFrame::OnLayout     )
  EVT_BUTTON( ID_SET_OUTPUT,       Segment2dFrame::OnSetOutput  )
  EVT_BUTTON( ID_OUT_TYPE,         Segment2dFrame::OnOutputType )
  EVT_BUTTON( ID_SAVE,             Segment2dFrame::OnSegment2dSave)
  EVT_COMBOBOX( ID_MODE,           Segment2dFrame::OnMode       )
  EVT_COMBOBOX( ID_OUT_OBJECT,     Segment2dFrame::OnOutputObject)
  EVT_COMBOBOX( ID_BRUSH_SIZE,     Segment2dFrame::OnBrushSize  )
  EVT_COMBOBOX( ID_ITERATES,       Segment2dFrame::OnIterates   )
  EVT_TEXT(   ID_MIN_POINTS,       Segment2dFrame::OnMinPoints  )
  EVT_TEXT(   ID_ALPHA,            Segment2dFrame::OnAlpha      )
  EVT_TEXT(   ID_BETA,             Segment2dFrame::OnBeta       )
  EVT_TEXT(   ID_GAMMA,            Segment2dFrame::OnGamma      )
  EVT_COMBOBOX( ID_FEATURE,        Segment2dFrame::OnFeature    )
  EVT_CHECKBOX( ID_FEATURE_STATUS, Segment2dFrame::OnFeatureStatus)
  EVT_COMBOBOX( ID_TRANSFORM,      Segment2dFrame::OnTransform  )
  EVT_BUTTON( ID_FEATURE_UPDATE,   Segment2dFrame::OnFeatureUpdate)
  EVT_COMMAND_SCROLL( ID_WEIGHT,   Segment2dFrame::OnWeight     )
  EVT_COMMAND_SCROLL( ID_MEAN,     Segment2dFrame::OnMean       )
  EVT_COMMAND_SCROLL( ID_STD_DEV,  Segment2dFrame::OnStdDev     )
  EVT_COMMAND_SCROLL( ID_FEATURE_MIN,   Segment2dFrame::OnFeatureMin   )
  EVT_COMMAND_SCROLL( ID_FEATURE_MAX,   Segment2dFrame::OnFeatureMax   )
  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, Segment2dFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  Segment2dFrame::OnWidthSlider  )
  EVT_BUTTON( ID_CT_LUNG,          Segment2dFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   Segment2dFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          Segment2dFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              Segment2dFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE_SLIDER,  Segment2dFrame::OnSliceSlider  )
  EVT_COMMAND_SCROLL( ID_SCALE_SLIDER,  Segment2dFrame::OnScaleSlider  )
  EVT_CHECKBOX(       ID_INVERT,        Segment2dFrame::OnInvert       )
  EVT_CHECKBOX(       ID_LABELS,        Segment2dFrame::OnLabels       )

  /** for interactive deep learning: */
  EVT_BUTTON( ID_INTDL_ADD,    Segment2dFrame::OnIntDLAdd    )
  EVT_BUTTON( ID_INTDL_BLINK,  Segment2dFrame::OnIntDLBlink  )
  EVT_BUTTON( ID_INTDL_CHOOSE, Segment2dFrame::OnIntDLChoose )
  EVT_BUTTON( ID_INTDL_CLEAR,  Segment2dFrame::OnIntDLClear  )
  EVT_BUTTON( ID_INTDL_RUN,    Segment2dFrame::OnIntDLRun    )
#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_CENTER_SLIDER, Segment2dFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  Segment2dFrame::OnUpdateUIWidthSglider )
#endif
END_EVENT_TABLE()
//======================================================================
