/*
  Copyright 1993-2015 Medical Image Processing Group
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
 * \file   OverlayFrame.cpp
 * \brief  OverlayFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "CineControls.h"

#define tControlsHeight ((buttonHeight+1)*9+50)

int slice_spacing_is_equal(SceneInfo *scn);

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for ThresholdFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
ThresholdFrame::ThresholdFrame ( bool maximize, int w, int h, bool fuzzy ): MainFrame( 0 ){
	fuzzy_flag = fuzzy;

	//init the types of input files that this app can accept
	mFileNameFilter = (char *)"CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0";
    mFileOrDataCount = 0;

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mCineControls       = NULL;
    mGrayMap1Controls   = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls = NULL;
	mSetParameterControls = NULL;
	mSetOutputControls = NULL;
	mHistZoomControls = NULL;
	mSearchStepControls = NULL;
	mThrOpacityControls = NULL;
	mSurfaceStrengthControls = NULL;

    ::gFrameList.push_back( this );
    if (!Preferences::getSingleFrameMode()) {
        gWhere += 20;
        if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    }
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -tControlsHeight );
    ::setColor( mSplitter );

	//top of window contains image(s) displayed via ThresholdCanvas
    mCanvas = tCanvas = new ThresholdCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize, fuzzy );

	m_prev = NULL;
	m_next = NULL;
	m_setIndex = NULL;
	m_grayMap = NULL;
	m_cine = NULL;
	m_blink = NULL;
	m_histZoomBut = NULL;
	m_histScopeBut = NULL;
	m_intervalBut = NULL;
	m_reset = NULL;
	m_searchStepBut = NULL;
	m_findOptBut = NULL;
	m_outputTypeBut = NULL;
	m_boundaryTypeBut = NULL;
	m_paramsBut = NULL;
	m_setParams = NULL;
	m_resetPointsBut = NULL;

	m_buttonBox = NULL;
	m_buttonSizer = NULL;
	m_fgs = NULL;


	OUTPUT_DATA_TYPE = fuzzy? OUTPUT_TYPE_SH0: OUTPUT_TYPE_BIM;
	MODE_TYPE=0;
	NORMAL_TYPE=2;
	resolution=1.0;
	strcpy(normal_dup[0], "0");
	strcpy(normal_dup[1], "8");
	strcpy(normal_dup[2], "26");
	params_on=FALSE;
	params_set=FALSE;
	num_params=1;
	cur_param=0;
	param_type[0] = 1; /*object*/
	param_type[1] = 0; /*time*/
	param_type[2] = 2; /*modality*/
	params[0] = params[1] = params[2] = NULL;
	num_volumes = 0;
	cur_volume = 0;
	out_interval = fuzzy? 0: 1;
	m_bg_flag = 0;
	points_file_tag = 100;
	outputTypeName[OUTPUT_TYPE_BS1] = _T("Output: BS1");
	outputTypeName[OUTPUT_TYPE_BS0] = _T("Output: BS0");
	outputTypeName[OUTPUT_TYPE_BS2] = _T("Output: BS2");
	outputTypeName[OUTPUT_TYPE_BIM] = _T("Output: BIM");
	outputTypeName[OUTPUT_TYPE_TXT] = _T("Output: TXT");
	outputTypeName[OUTPUT_TYPE_SH0] = _T("Output: SH0");
	outputTypeName[OUTPUT_TYPE_IM0] = _T("Output: IM0");
	histScopeName[0] = _T("Scope: Slice");
	histScopeName[1] = _T("Scope: Vol.");
	histScopeName[2] = _T("Scope: Scene");
	intervalName[0] = _T("Interval: 1");
	intervalName[1] = _T("Interval: 2");
	intervalName[2] = _T("Interval: 3");
	intervalName[3] = _T("Interval: 4");
	intervalName[MAX_INTERVAL] = _T("Interval: All");
	intervalName[MAX_INTERVAL+1] = _T("Interval: Opt.");
	nbits = 4;
	min_opac = 0.0;
	max_opac = 0.99f;
	for (int i=1; i<=MAX_INTERVAL+1; i++)
		opacity_factor[i] = 1.0;
	programmatic_update = false;

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
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -tControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)
		Maximize( true );
    else
		SetSize( w, h );
	mSplitter->SetSashPosition( -tControlsHeight );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Select Interval",   2 );
    SetStatusText( "Select Boundary", 3 );
    SetStatusText( "", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -tControlsHeight );
	if (Preferences::getShowSaveScreen()) {
		wxCommandEvent  unused;
		OnSaveScreen( unused );
	}
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void ThresholdFrame::initializeMenu ( void ) {
    //init standard menu bar and menu items
    MainFrame::initializeMenu();
	//if we are in the mode that supports having more than one window open
	// at a time, we need to make other windows aware of this window.
    ::copyWindowTitles( this );

    wxString  tmp;
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    if (!fuzzy_flag)    tmp = wxString::Format( "CAVASS:Threshold:%d", j);
    else                tmp = wxString::Format( "CAVASS:1Feature:%d",  j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
	//enable the Open menu item
	wxMenuItem*  op = mFileMenu->FindItem( ID_OPEN );
	op->Enable( true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 9/26/07 Dewey Odhner
void ThresholdFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 0, 10, wxGROW );  //spacer
    if (m_buttonBox == NULL)
		m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    if (m_buttonSizer == NULL)
		m_buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    if (m_fgs == NULL)
		m_fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    //row 1, col 1
    m_prev = new wxButton( mControlPanel, ID_PREVIOUS, "Previous", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_prev );
    m_fgs->Add( m_prev, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
    m_next = new wxButton( mControlPanel, ID_NEXT, "Next", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_next );
    m_fgs->Add( m_next, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
    m_setIndex = new wxButton( mControlPanel, ID_SET_INDEX1, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_setIndex );
    m_fgs->Add( m_setIndex, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    m_grayMap = new wxButton( mControlPanel, ID_GRAYMAP1, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_grayMap );
    m_fgs->Add( m_grayMap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
    m_cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_cine );
    m_fgs->Add( m_cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
	m_blink = new wxCheckBox( mControlPanel, ID_BLINK, "Blink" );
    ::setColor( m_blink );
	m_blink->SetValue( 0 );
	::setColor( m_blink );
	m_fgs->Add( m_blink, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
	if (m_histZoomBut == NULL) {
		m_histZoomBut = new wxButton( mControlPanel, ID_HIST_ZOOM, "Hist. Zoom", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_histZoomBut );
    }
	m_fgs->Add( m_histZoomBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
	m_histScopeBut = new wxButton( mControlPanel, ID_HIST_SCOPE, "Scope: Slice", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_histScopeBut );
	m_fgs->Add( m_histScopeBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 1
	wxArrayString sa;
	for (int j=0; j<MAX_INTERVAL+2; j++)
		sa.Add(intervalName[j]);
	m_intervalBut = new wxComboBox( mControlPanel, ID_INTERVAL, "Interval: 1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
    ::setColor( m_intervalBut );
	m_fgs->Add( m_intervalBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    m_reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_reset );
    m_fgs->Add( m_reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 6, col 1
	m_searchStepBut = new wxButton( mControlPanel, ID_OPT_STEP, "Step/Key-In", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_searchStepBut );
	m_fgs->Add( m_searchStepBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 6, col 2
	m_findOptBut = new wxButton( mControlPanel, ID_FIND_OPT, "Find Optimum", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_findOptBut );
	m_fgs->Add( m_findOptBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 1
	sa.Clear();
	if (fuzzy_flag)
	{
		sa.Add(outputTypeName[OUTPUT_TYPE_SH0]);
		sa.Add(outputTypeName[OUTPUT_TYPE_IM0]);
	}
	else
	{
		sa.Add(outputTypeName[OUTPUT_TYPE_BS1]);
		sa.Add(outputTypeName[OUTPUT_TYPE_BS0]);
		sa.Add(outputTypeName[OUTPUT_TYPE_BS2]);
		sa.Add(outputTypeName[OUTPUT_TYPE_BIM]);
		sa.Add(outputTypeName[OUTPUT_TYPE_TXT]);
	}
	if (m_outputTypeBut == NULL) {
		m_outputTypeBut = new wxComboBox( mControlPanel, ID_OUT_TYPE,
			outputTypeName[OUTPUT_DATA_TYPE], wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
        ::setColor( m_outputTypeBut );
    }
    m_fgs->Add( m_outputTypeBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 2
	m_setOutputBut = new wxButton( mControlPanel, ID_SET_OUTPUT, "Set Output", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_setOutputBut );
	m_fgs->Add( m_setOutputBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	if (fuzzy_flag)
	{
		//row 8, col 1
		m_opacityBut = new wxButton( mControlPanel, ID_SET_OPACITY, "Opacity", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_opacityBut );
		m_fgs->Add( m_opacityBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		m_opacityBut->Enable(tCanvas->interval <= MAX_INTERVAL);
		//row 8, col 2
		m_surfaceStrengthBut= new wxButton( mControlPanel, ID_SET_SURFACE_STRENGTH, "Surface Strength", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_surfaceStrengthBut );
		m_fgs->Add( m_surfaceStrengthBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		m_boundaryTypeBut = NULL;
		m_resetPointsBut = NULL;
	}
	else
	{
		//row 8, col 1
		sa.Clear();
		sa.Add("Boundary: All");
		sa.Add("Boundary: Connected");
		m_boundaryTypeBut = new wxComboBox( mControlPanel, ID_BOUNDARY_TYPE,
		  "Boundary: All", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		  sa, wxCB_READONLY );
        ::setColor( m_boundaryTypeBut );
		m_fgs->Add( m_boundaryTypeBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		//row 8, col 2
		m_resetPointsBut = new wxButton( mControlPanel, ID_RESET_POINTS, "Reset Points", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_resetPointsBut );
		m_fgs->Add( m_resetPointsBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		m_opacityBut = NULL;
		m_surfaceStrengthBut = NULL;
	}
	//row 9, col 1
	m_paramsBut = new wxCheckBox( mControlPanel, ID_PARAMS, "Parameters" );

    if (params_on)    m_paramsBut->SetValue( true );
    else              m_paramsBut->SetValue( false );
	m_paramsBut->Enable(
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS1||OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS0||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS2||OUTPUT_DATA_TYPE==OUTPUT_TYPE_SH0 );

	::setColor( m_paramsBut );
	m_fgs->Add( m_paramsBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 9, col 2
	m_setParams = new wxButton( mControlPanel, ID_SET_PARAMS, "Set Parameters", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_setParams );
	if (params_on)    m_setParams->Enable( true );
	else              m_setParams->Enable( false );
	m_fgs->Add( m_setParams, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    m_buttonSizer->Add( m_fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( m_buttonSizer, 0, wxGROW|wxALL, 10 );
}

void ThresholdFrame::emptyButtonBox ( void )
{
	if (m_setParams!=NULL) {
		m_fgs->Detach(m_setParams);
		delete m_setParams;
	}
	if (m_paramsBut!=NULL) {
		m_fgs->Detach(m_paramsBut);
		delete m_paramsBut;
	}
	if (m_surfaceStrengthBut!=NULL) {
		m_fgs->Detach(m_surfaceStrengthBut);
		delete m_surfaceStrengthBut;
	}
	if (m_opacityBut!=NULL) {
		m_fgs->Detach(m_opacityBut);
		delete m_opacityBut;
	}
	if (m_resetPointsBut!=NULL) {
		m_fgs->Detach(m_resetPointsBut);
		delete m_resetPointsBut;
	}
	if (m_boundaryTypeBut!=NULL) {
		m_fgs->Detach(m_boundaryTypeBut);
		delete m_boundaryTypeBut;
	}
	if (m_setOutputBut!=NULL) {
		m_fgs->Detach(m_setOutputBut);
		delete m_setOutputBut;
	}
	if (m_outputTypeBut!=NULL) {
		m_fgs->Detach(m_outputTypeBut);
		delete m_outputTypeBut;
	}
	if (m_findOptBut!=NULL) {
		m_fgs->Detach(m_findOptBut);
		delete m_findOptBut;
	}
	if (m_searchStepBut!=NULL) {
		m_fgs->Detach(m_searchStepBut);
		delete m_searchStepBut;
	}
	if (m_reset!=NULL) {
		m_fgs->Detach(m_reset);
		delete m_reset;
	}
	if (m_intervalBut) {
		m_fgs->Detach(m_intervalBut);
		delete m_intervalBut;
	}
	if (m_histScopeBut) {
		m_fgs->Detach(m_histScopeBut);
		delete m_histScopeBut;
	}
	if (m_histZoomBut != NULL) {
		m_fgs->Detach(m_histZoomBut);
		delete m_histZoomBut;
	}
	if (m_blink!=NULL) {
		m_fgs->Detach(m_blink);
		delete m_blink;
	}
	if (m_cine!=NULL) {
		m_fgs->Detach(m_cine);
		delete m_cine;
	}
	if (m_grayMap!=NULL) {
		m_fgs->Detach(m_grayMap);
		delete m_grayMap;
	}
	if (m_setIndex!=NULL) {
		m_fgs->Detach(m_setIndex);
		delete m_setIndex;
	}
	if (m_next!=NULL) {
		m_fgs->Detach(m_next);
		delete m_next;
	}
	if (m_prev!=NULL) {
		m_fgs->Detach(m_prev);
		delete m_prev;
	}
	m_prev = NULL;
	m_next = NULL;
	m_setIndex = NULL;
	m_grayMap = NULL;
	m_cine = NULL;
	m_blink = NULL;
	m_histZoomBut = NULL;
	m_histScopeBut = NULL;
	m_intervalBut = NULL;
	m_reset = NULL;
	m_searchStepBut = NULL;
	m_findOptBut = NULL;
	m_outputTypeBut = NULL;
	m_setOutputBut = NULL;
	m_boundaryTypeBut = NULL;
	m_opacityBut = NULL;
	m_surfaceStrengthBut = NULL;
	m_resetPointsBut = NULL;
	m_paramsBut = NULL;
	m_setParams = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ThresholdFrame class. */
ThresholdFrame::~ThresholdFrame ( void ) {
    cout << "ThresholdFrame::~ThresholdFrame" << endl;
    wxLogMessage( "ThresholdFrame::~ThresholdFrame" );

	if (params[0])
		free(params[0]);
	if (params[1])
		free(params[1]);
	if (params[2])
		free(params[2]);
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    deleteThresholdControls();
    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
	emptyButtonBox();
	if (m_fgs!=NULL)
	{
		m_buttonSizer->Detach(m_fgs);
		delete m_fgs;
		m_fgs = NULL;
	}
	if (m_buttonSizer!=NULL)
	{
		mBottomSizer->Detach(m_buttonSizer);
		delete m_buttonSizer;
		m_buttonSizer = NULL;
	}
    if (mControlPanel!=NULL)  {
		mBottomSizer->Detach(mControlPanel);
		delete mControlPanel;
		mControlPanel=NULL;
	}
#if 0
    if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
#endif
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
/** \brief callback for key presses. */
void ThresholdFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "ThresholdFrame::OnChar" );
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
        cout << "ThresholdFrame::OnChar: " << ::gTimerInterval << endl;
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
/** \brief This method should be called whenever one wishes to create a
 *  new ThresholdFrame.  It first searches the input file history for an
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
void ThresholdFrame::createThresholdFrame ( wxFrame* parentFrame, bool useHistory, bool fuzzy )
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
            "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
            wxFILE_MUST_EXIST );
    }
    
    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    //display an example frame using the specified file as input
    ThresholdFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new ThresholdFrame( parentFrame->IsMaximized(), w, h, fuzzy );
    } else {
        frame = new ThresholdFrame( false, 800, 600, fuzzy );
    }
    frame->loadFile( filename.c_str() );
    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void ThresholdFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
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
/** \brief callback for toggling control panel on and off. */
void ThresholdFrame::OnHideControls ( wxCommandEvent& e ) {
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel, -tControlsHeight );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This function determines if the given filename is of a type
 *  that can be read by this module/app.
 *
 *  Supported file extensions: BIM, BMP, DCM, DICOM, GIF, IM0, JPG, JPEG,
 *  PCX, TIF, and TIFF.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool ThresholdFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item */
void ThresholdFrame::OnOpen ( wxCommandEvent& unused ) {
	createThresholdFrame( this, false, fuzzy_flag );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "ThresholdFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "ThresholdFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK) {
        if (fuzzy_flag)
			OnPPScops1Feature( unused );
		else
			OnPPScopsThreshold( unused );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void ThresholdFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxString  tmp = wxString::Format( "File %s could not be opened.", fname );
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp;
    if (!fuzzy_flag)    tmp = "CAVASS:Threshold: ";
    else                tmp = "CAVASS:1Feature: ";
    ++mFileOrDataCount;
    if (mFileOrDataCount==1) {
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
            if (!fuzzy_flag)
                tmp = wxString::Format( "CAVASS:Threshold:%s (%d)", fname, i);
            else
                tmp = wxString::Format( "CAVASS:1Feature:%s (%d)",  fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    tCanvas->loadFile( fname );

	num_volumes = mCanvas->mCavassData->m_vh.scn.dimension==4?
		mCanvas->mCavassData->m_vh.scn.num_of_subscenes[0]: 1;
	grad_factor = mCanvas->mCavassData->m_max/2;
	first_sl = 1;
	last_sl = mCanvas->mCavassData->m_zSize;
	params[0] = (float *)malloc(num_volumes*sizeof(float));
	params[1] = (float *)malloc(num_volumes*sizeof(float));
	params[2] = (float *)malloc(num_volumes*sizeof(float));
	for (int j=0; j<num_volumes; j++)
		for (int k=0; k<3; k++)
			params[k][j] = param_type[k]==0/*time*/? 1.0+j:
						   param_type[k]==1/*object*/? tCanvas->interval: 0.0;

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void ThresholdFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";

    wxString  tmp;
    if (!fuzzy_flag)    tmp = "CAVASS:Threshold: ";
    else                tmp = "CAVASS:1Feature: ";
    tmp += name;
    SetTitle( tmp );
    
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnPrevious ( wxCommandEvent& unused ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;

    if (sliceA<0 )
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );

    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnNext ( wxCommandEvent& unused ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;

    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );

    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mSetIndex1Controls!=NULL) {
        delete mSetIndex1Controls;
        mSetIndex1Controls = NULL;
        return;
    }
	deleteThresholdControls();

    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    mSetIndex1Controls = new SetThresholdIndexControls( mControlPanel,
		mBottomSizer, "Set Index", canvas->getSliceNo(0)+1,
		canvas->getNoSlices(0), ID_SLICE1_SLIDER, ID_SCALE1_SLIDER,
		canvas->getScale(), ID_OVERLAY, ID_ZOOM_IN, ID_ZOOM_OUT );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mGrayMap1Controls!=NULL) {
        delete mGrayMap1Controls;
        mGrayMap1Controls = NULL;
        return;
    }
	deleteThresholdControls();

    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnCine ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL) {
        delete mCineControls;
        mCineControls = NULL;
        return;
    }
	deleteThresholdControls();

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 7/12/07 Dewey Odhner
void ThresholdFrame::OnBlink ( wxCommandEvent& e ) {
	tCanvas->BLINK = e.IsChecked();
	tCanvas->initLUT( 0 );
	tCanvas->reload();
}
void ThresholdFrame::OnHistZoom ( wxCommandEvent& unused ) {
	if (mHistZoomControls != NULL) {
		delete mHistZoomControls;
		mHistZoomControls = NULL;
		return;
	}
	deleteThresholdControls();
	mHistZoomControls = new HistZoomControls ( mControlPanel, mBottomSizer,
        "Histogram Zoom", ID_HIST_ZOOM_SLIDER, tCanvas->getHistZoomFactor() );
}

// 7/13/07 Dewey Odhner
void ThresholdFrame::OnHistScope ( wxCommandEvent& unused ) {
	/* 0= slice, 1= volume, 2= scene */
    if (tCanvas->getHistScope() < 2)
		tCanvas->setHistScope(tCanvas->getHistScope()+1);
    else
		tCanvas->setHistScope(0);
    m_histScopeBut->SetLabel(histScopeName[tCanvas->getHistScope()]);
	tCanvas->reload();
}

// 7/18/07 Dewey Odhner
void ThresholdFrame::OnInterval ( wxCommandEvent& e ) {
	for (tCanvas->interval=1; intervalName[tCanvas->interval-1]!=e.GetString();
			tCanvas->interval++)
		;
	if (mThrOpacityControls)
	{
		if (tCanvas->interval > MAX_INTERVAL)
		{
			delete mThrOpacityControls;
			mThrOpacityControls = NULL;
		}
		else
			mThrOpacityControls->
				SetValue((int)rint(opacity_factor[tCanvas->interval]*100));
	}
	if (m_opacityBut)
		m_opacityBut->Enable(tCanvas->interval <= MAX_INTERVAL);
	tCanvas->reload();
	if (tCanvas->interval == MAX_INTERVAL+1)
	{
		delete mSearchStepControls;
		mSearchStepControls = NULL;
	}
	UpdateThresholdTablets();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnReset ( wxCommandEvent& unused ) {
	deleteThresholdControls();
    m_cine_timer->Stop();

    ThresholdCanvas*  canvas = tCanvas;
	canvas->resetThresholds();
    canvas->m_tx = canvas->m_ty = 0;
    canvas->mCavassData->mDisplay = true;
    canvas->setCenter(  0, canvas->getMax(0)/2 );
    canvas->setWidth(   0, canvas->getMax(0)   );
    canvas->setInvert(  0, false );
    canvas->setCenter(  1, canvas->getMax(1)/2 );
    canvas->setWidth(   1, canvas->getMax(1)   );
    canvas->setInvert(  1, false );
    canvas->setScale( 1.0 );
    canvas->reload();
}

// 7/24/07 Dewey Odhner
void ThresholdFrame::OnSearchStep ( wxCommandEvent& unused )
{
	if (mSearchStepControls!=NULL || tCanvas->interval==MAX_INTERVAL+1) {
		delete mSearchStepControls;
		mSearchStepControls = NULL;
		return;
	}
	deleteThresholdControls();
	mSearchStepControls = new SearchStepControls( mControlPanel, mBottomSizer,
		"Step / Key-In", (int)tCanvas->getOptStep(), ID_OPT_STEP_SLIDER,
		ID_THRSLD_TABLET1, fuzzy_flag? ID_THRSLD_TABLET2:0,
		fuzzy_flag? ID_THRSLD_TABLET3:0, ID_THRSLD_TABLET4,
		(double)tCanvas->min_table[tCanvas->interval],
		(double)tCanvas->inter1_table[tCanvas->interval],
		(double)tCanvas->inter2_table[tCanvas->interval],
		(double)tCanvas->max_table[tCanvas->interval] );
}

// 9/28/07 Dewey Odhner
void cvUpdateThresholdTablets(MainFrame *mf)
{
    ThresholdFrame *frame = dynamic_cast<ThresholdFrame*>(mf);
	frame->UpdateThresholdTablets();
}

// 10/3/07 Dewey Odhner
void ThresholdFrame::UpdateThresholdTablets()
{
	if (mSearchStepControls == NULL)
		return;
	int intrvl =
		tCanvas->interval==MAX_INTERVAL+2? MAX_INTERVAL+1:tCanvas->interval;
	programmatic_update = true;
	mSearchStepControls->
		setParam1Value((double)tCanvas->min_table[intrvl]);
	if (fuzzy_flag)
	{
		mSearchStepControls->
			setParam2Value((double)tCanvas->inter1_table[intrvl]);
		mSearchStepControls->
			setParam3Value((double)tCanvas->inter2_table[intrvl]);
	}
	mSearchStepControls->
		setParam4Value((double)tCanvas->max_table[intrvl]);
	programmatic_update = false;
}

// 10/2/07 Dewey Odhner
void ThresholdFrame::OnSetThreshold1( wxCommandEvent& e )
{
	double d;
	if (!programmatic_update && e.GetString().ToDouble(&d) && d>=0)
	{
		if (d < tCanvas->getHistMin())
			d = tCanvas->getHistMin();
		if (d > tCanvas->getHistMax())
			d = tCanvas->getHistMax();
		tCanvas->moveThrBar1(d);
		tCanvas->reload();
	}
}

// 10/2/07 Dewey Odhner
void ThresholdFrame::OnSetThreshold2( wxCommandEvent& e )
{
	double d;
	if (!programmatic_update && e.GetString().ToDouble(&d) && d>=0)
	{
		if (d < tCanvas->getHistMin())
			d = tCanvas->getHistMin();
		if (d > tCanvas->getHistMax())
			d = tCanvas->getHistMax();
		tCanvas->moveThrBar2(d);
		tCanvas->reload();
	}
}

// 10/2/07 Dewey Odhner
void ThresholdFrame::OnSetThreshold3( wxCommandEvent& e )
{
	double d;
	if (!programmatic_update && e.GetString().ToDouble(&d) && d>=0)
	{
		if (d < tCanvas->getHistMin())
			d = tCanvas->getHistMin();
		if (d > tCanvas->getHistMax())
			d = tCanvas->getHistMax();
		tCanvas->moveThrBar3(d);
		tCanvas->reload();
	}
}

// 10/2/07 Dewey Odhner
void ThresholdFrame::OnSetThreshold4( wxCommandEvent& e )
{
	double d;
	if (!programmatic_update && e.GetString().ToDouble(&d) && d>=0)
	{
		if (d < tCanvas->getHistMin())
			d = tCanvas->getHistMin();
		if (d > tCanvas->getHistMax())
			d = tCanvas->getHistMax();
		tCanvas->moveThrBar4(d);
		tCanvas->reload();
	}
}

// 9/21/07 Dewey Odhner
void ThresholdFrame::OnSetOpacity ( wxCommandEvent& unused )
{
	if (mThrOpacityControls != NULL) {
		delete mThrOpacityControls;
		mThrOpacityControls = NULL;
		return;
	}
	deleteThresholdControls();
	mThrOpacityControls = new ThrOpacityControls( mControlPanel, mBottomSizer,
		"Material Opacity", (int)rint(opacity_factor[tCanvas->interval]*100),
		ID_OPACITY_SLIDER );
}

// 9/21/07 Dewey Odhner
void ThresholdFrame::OnSetSurfaceStrength ( wxCommandEvent& unused )
{
	if (mSurfaceStrengthControls != NULL) {
		delete mSurfaceStrengthControls;
		mSurfaceStrengthControls = NULL;
		return;
	}
	deleteThresholdControls();
	mSurfaceStrengthControls = new SurfaceStrengthControls( mControlPanel,
	mBottomSizer, "Surface Strength", grad_factor,
	ID_SURFACE_STRENGTH_SLIDER, mCanvas->mCavassData->m_max );
}

void ThresholdFrame::OnFindOpt ( wxCommandEvent& unused )
{
	 tCanvas->findOptima();
}

/** \brief callback for  outputType button */
// 9/26/07 Dewey Odhner
void ThresholdFrame::OnOutputType ( wxCommandEvent& e ) {

	for (OUTPUT_DATA_TYPE=0; outputTypeName[OUTPUT_DATA_TYPE]!=e.GetString();
			OUTPUT_DATA_TYPE++)
		;
	bool struct_out =
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS1||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS0||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS2||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_SH0;
	if (mSetOutputControls)
		mSetOutputControls->EnableStructureOptions(struct_out);
	if (mSetParameterControls)
	{
		if (!struct_out)
		{
			delete mSetParameterControls;
			mSetParameterControls = NULL;
		}
	}
	m_paramsBut->Enable( struct_out );
	m_setParams->Enable( params_on && struct_out );
	if (fuzzy_flag)
	{
		if (OUTPUT_DATA_TYPE == OUTPUT_TYPE_IM0)
		{
			if (mThrOpacityControls)
			{
				delete mThrOpacityControls;
				mThrOpacityControls = NULL;
			}
			if (mSurfaceStrengthControls)
			{
				delete mSurfaceStrengthControls;
				mSurfaceStrengthControls = NULL;
			}
			m_opacityBut->Enable(false);
			m_surfaceStrengthBut->Enable(false);
		}
		else
		{
			m_opacityBut->Enable(true);
			m_surfaceStrengthBut->Enable(true);
		}
	}
}

void ThresholdFrame::OnBoundaryType ( wxCommandEvent& e )
{
	tCanvas->BOUNDARY_TYPE = e.GetString()=="Boundary: Connected";
}

void ThresholdFrame::OnNormalType ( wxCommandEvent& unused )
{
	for (NORMAL_TYPE=0; wxString::Format("Normal: %s", normal_dup[NORMAL_TYPE])
			!=unused.GetString(); NORMAL_TYPE++)
		if (NORMAL_TYPE >= 3)
		{
			assert(0);
			break;
		}
}

// 6/28/07 Dewey Odhner 
void ThresholdFrame::deleteThresholdControls(void)
{
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) {
        delete mSetIndex1Controls;
        mSetIndex1Controls = NULL;
    }
	if (mSetParameterControls!=NULL) {
		delete mSetParameterControls;
		mSetParameterControls = NULL;
	}
	if (mSetOutputControls!=NULL) {
		delete mSetOutputControls;
		mSetOutputControls = NULL;
	}
	if (mHistZoomControls!=NULL) {
		delete mHistZoomControls;
		mHistZoomControls = NULL;
	}
	if (mSearchStepControls!=NULL) {
		delete mSearchStepControls;
		mSearchStepControls = NULL;
	}
	if (mThrOpacityControls!=NULL) {
		delete mThrOpacityControls;
		mThrOpacityControls = NULL;
	}
	if (mSurfaceStrengthControls!=NULL) {
		delete mSurfaceStrengthControls;
		mSurfaceStrengthControls = NULL;
	}
}

// 8/10/07 Dewey Odhner
void ThresholdFrame::OnVolumeNumber ( wxCommandEvent& unused )
{
	cur_volume = cur_volume+1>=num_volumes? 0: cur_volume+1;
	mSetParameterControls->setCurVolumeLabel(cur_volume);
	mSetParameterControls->setParam1Value(params[0][cur_volume]);
	mSetParameterControls->setParam2Value(params[1][cur_volume]);
	mSetParameterControls->setParam3Value(params[2][cur_volume]);
}

// 8/9/07 Dewey Odhner
// Modified: 8/28/07 m_setParams->Enable( params_on ) called by Dewey Odhner.
void ThresholdFrame::OnParams ( wxCommandEvent& e )
{
	params_on = e.IsChecked();
	if (!params_on && mSetParameterControls!=NULL) {
		delete mSetParameterControls;
		mSetParameterControls = NULL;
	}
	if (params_on)    m_setParams->Enable( true );
	else              m_setParams->Enable( false );
}

// 8/9/07 Dewey Odhner
void ThresholdFrame::OnSetParams ( wxCommandEvent& unused )
{
	if (mSetParameterControls!=NULL) {
		delete mSetParameterControls;
		mSetParameterControls = NULL;
		return;
	}
	if (!params_on)
		return;
	deleteThresholdControls();

	mSetParameterControls = new SetThresholdParameterControls( mControlPanel,
		mBottomSizer, "Set Parameters", num_params, cur_param,
		param_type[cur_param], ID_NUM_PARAMS, ID_WHICH_PARAM, ID_PARAM_TYPE,
		ID_PARAM1_CTRL, ID_PARAM2_CTRL, ID_PARAM3_CTRL, ID_VOLUME_NUMBER,
		params[0][cur_volume], params[1][cur_volume], params[2][cur_volume],
		cur_volume );
}

// 8/10/07 Dewey Odhner
void ThresholdFrame::OnSetParam1 ( wxCommandEvent& e )
{
	double d;
	if (e.GetString().ToDouble(&d))
		params[0][cur_volume] = d;
}

// 8/10/07 Dewey Odhner
void ThresholdFrame::OnSetParam2 ( wxCommandEvent& e )
{
	double d;
	if (e.GetString().ToDouble(&d))
		params[1][cur_volume] = d;
}

// 8/10/07 Dewey Odhner
void ThresholdFrame::OnSetParam3 ( wxCommandEvent& e )
{
	double d;
	if (e.GetString().ToDouble(&d))
		params[2][cur_volume] = d;
}

void ThresholdFrame::OnOutputMode ( wxCommandEvent& unused )
{
	/* MODE_TYPE 0- New 1- Merge */
	MODE_TYPE = unused.GetString()=="Out Mode: Merge";
}

// 8/8/07 Dewey Odhner
void ThresholdFrame::OnResetPoints ( wxCommandEvent& unused )
{
	tCanvas->num_track_points = 0;
	tCanvas->RunThreshold();
}

void ThresholdFrame::OnOutputInterval(wxCommandEvent& unused )
{
	if (sscanf((const char *)unused.GetString().c_str(), "Out Interval: %d",
	    	&out_interval) != 1)
	{
		out_interval = 0;
		assert(unused.GetString() == "Out Interval: All");
	}
}

void ThresholdFrame::OnSetOutput ( wxCommandEvent& unused )
{
	if (mSetOutputControls!=NULL) {
		delete mSetOutputControls;
		mSetOutputControls = NULL;
		return;
	}
	deleteThresholdControls();
	mSetOutputControls = new SetThresholdOutputControls( mControlPanel,
		mBottomSizer, "Set Output", ID_RESOLUTION, ID_NORMAL_TYPE, ID_SAVE,
		ID_OUT_MODE, ID_OUT_INTERVAL, 1.0/resolution,
		normal_dup[NORMAL_TYPE], MODE_TYPE, out_interval,
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS1||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS0||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_BS2||
		OUTPUT_DATA_TYPE==OUTPUT_TYPE_SH0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ThresholdFrame::OnThresholdSave(wxCommandEvent &e)
{
	int o_p = MODE_TYPE? 0: wxOVERWRITE_PROMPT;
	switch (OUTPUT_DATA_TYPE)
	{
		case OUTPUT_TYPE_BIM:
			{
		      wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.BIM"),
                        _T("BIM files (*.BIM)|*.BIM"),
                        wxSAVE|wxOVERWRITE_PROMPT);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveBinaryFile((const char *)saveDlg.GetPath().c_str(), out_interval,
					m_bg_flag);
			}
			break;
		case OUTPUT_TYPE_IM0:
			{
		      wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.IM0"),
                        _T("IM0 files (*.IM0)|*.IM0"),
                        wxSAVE|wxOVERWRITE_PROMPT);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveBinaryFile((const char *)saveDlg.GetPath().c_str(), out_interval,
					m_bg_flag);
			}
			break;
		case OUTPUT_TYPE_BS1:
		    {
			  wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.BS1"),
                        _T("BS1 files (*.BS1)|*.BS1"),
                        wxSAVE|o_p);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveBoundaryFile((const char *)saveDlg.GetPath().c_str(), "BS1",
					out_interval, m_bg_flag);
			}
			break;
		case OUTPUT_TYPE_BS0:
		    {
			  wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.BS0"),
                        _T("BS0 files (*.BS0)|*.BS0"),
                        wxSAVE|o_p);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveBoundaryFile((const char *)saveDlg.GetPath().c_str(), "BS0",
					out_interval, m_bg_flag);
			}
			break;
		case OUTPUT_TYPE_BS2:
		    {
			  wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.BS2"),
                        _T("BS2 files (*.BS2)|*.BS2"),
                        wxSAVE|o_p);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveBoundaryFile((const char *)saveDlg.GetPath().c_str(), "BS2",
					out_interval, m_bg_flag);
			}
			break;
		case OUTPUT_TYPE_SH0:
		    {
			  wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.SH0"),
                        _T("SH0 files (*.SH0)|*.SH0"),
                        wxSAVE|o_p);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveBoundaryFile_1((const char *)saveDlg.GetPath().c_str(), "SH0",
					out_interval, m_bg_flag);
			}
			break;
		case OUTPUT_TYPE_TXT:
		    {
			  wxFileDialog saveDlg(this,
                        _T("Save file"),
                        wxEmptyString,
                        _T("thrsld-tmp.TXT"),
                        _T("TXT files (*.TXT)|*.TXT"),
                        wxSAVE|o_p);
			  if (saveDlg.ShowModal() == wxID_OK)
				SaveTextFile((const char *)saveDlg.GetPath().c_str(), "TXT", out_interval,
					m_bg_flag);
			}
			break;
	}
}

// 8/9/07 Dewey Odhner
void ThresholdFrame::OnNumParams ( wxCommandEvent& unused )
{
	if (sscanf((const char *)unused.GetString().c_str(), "%d", &num_params)!=1)
		assert(0);
	mSetParameterControls->setNumParamsLabel(num_params);
	if (cur_param >= num_params)
	{
		cur_param = num_params-1;
		mSetParameterControls->setWhichParamLabel(cur_param);
		mSetParameterControls->setParamTypeLabel(param_type[cur_param]);
	}
}

// 8/9/07 Dewey Odhner
void ThresholdFrame::OnWhichParam ( wxCommandEvent& unused )
{
	if (sscanf((const char *)unused.GetString().c_str(), "Parameter: %d",
	    	&cur_param) != 1)
		assert(0);
	assert(cur_param > 0);\
	cur_param--;
	mSetParameterControls->setParamTypeLabel(param_type[cur_param]);
}

// 8/10/07 Dewey Odhner
void ThresholdFrame::OnParamType ( wxCommandEvent& unused )
{
	param_type[cur_param] = mSetParameterControls->getParamType();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/************************************************************************
 *                                                                      *
 *      Function        : SaveBinaryFile                                *
 *      Description     : This function will save the binary file       *
 *                        when the SAVE button is pressed in the        *
 *                        button window.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  name - the name of the file to be saved.     *
 *                         range - the range of threshold values to be  *
 *                              used to create the binary file.         *
 *                         bg_flag - whether to do this in the          *
 *                              background mode.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : tCanvas->min_table, tCanvas->max_table, tCanvas->num_track_points, tCanvas->track_pt
 *      Related funcs   : None.                                         *
 *      History         : Written on March 10, 1993 by S.Samarasekera.  *
 *                        Modified: 11/3/00 tCanvas->min_table, tCanvas->max_table used
 *                           by Dewey Odhner.
 *                        Modified: 6/19/01 connected components tracked
 *                           by Dewey Odhner.
 *                        Modified: 9/25/07 fuzzy thresholding done
 *                           by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int ThresholdFrame::SaveBinaryFile(const char *name, int range, int bg_flag)
{

  int low,hi,flag, j;
  wxString cmnd, tmpfile;

  cmnd = "";
  if (mCanvas->mCavassData->m_vh.scn.num_of_bits > 1)
  {
    if (fuzzy_flag)
#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
	  cmnd = wxString::Format("\"%s/ndclass\" \"%s\"  \"%s\" %d 0 0 %d %d",
        (const char *)Preferences::getHome().c_str(),
#else
	  cmnd = wxString::Format("ndclass \"%s\" \"%s\" %d 0 0 %d %d",
#endif
        mCanvas->mCavassData->m_fname, name, bg_flag,
		mCanvas->mCavassData->m_vh.scn.xysize[0],
		mCanvas->mCavassData->m_vh.scn.xysize[1]);
    else
	{
	  tmpfile = wxString::Format("%s%s", name, tCanvas->BOUNDARY_TYPE?"_":"");
#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
	  cmnd += wxString::Format("\"%s/ndthreshold\" \"%s\" \"%s\" 0",
        (const char *)Preferences::getHome().c_str(),
#else
	  cmnd += wxString::Format("ndthreshold \"%s\" \"%s%s\" 0",
#endif
        mCanvas->mCavassData->m_fname, (const char *)tmpfile.c_str());
    }
	if(range) {
      if (tCanvas->tbar1x[range]==-1 || tCanvas->tbar2x[range]==-1) {
        wxMessageBox("Please specify the range first");
        return(-1);
      }
      else {
        low= tCanvas->min_table[range];
        hi = tCanvas->max_table[range];
        if (fuzzy_flag)
		  cmnd += wxString::Format(" %d %d %d %d", low,
		    tCanvas->inter1_table[range], tCanvas->inter2_table[range], hi);
		else
		  cmnd += wxString::Format(" %d %d", low, hi);
      }
    }
    else {
      if (fuzzy_flag)
	  {
	    wxMessageBox(
		  "CAN SAVE ONLY CLASSIFIED IMAGE CORRESPONDING TO ONE FUNCTION");
		return(-1);
	  }
	  flag=0;
      for(range=1;range<=4;range++) 
        if (tCanvas->tbar1x[range]!=-1 && tCanvas->tbar2x[range]!=-1) {
          low= tCanvas->min_table[range];
          hi = tCanvas->max_table[range];
          cmnd += wxString::Format(" %d %d", low, hi);
          flag=1;
        }
      if (!flag) {
        wxMessageBox("Please specify at least one range first");
        return(-1);
      } 
    }
  }
  else
  {
    tmpfile = wxString(mCanvas->mCavassData->m_fname);
  }
  if (fuzzy_flag)
    cmnd += wxString::Format(" %d %d", first_sl, last_sl);

  if (tCanvas->BOUNDARY_TYPE || mCanvas->mCavassData->m_vh.scn.num_of_bits==1) { /* Connected */
    if (tCanvas->num_track_points==0) {
      wxMessageBox("Select Points before tracking\n");
      return (-1);
    }
  }

  if (mCanvas->mCavassData->m_vh.scn.num_of_bits > 1)
  {
    wxLogMessage( "command=%s", (const char *)cmnd.c_str() );
    printf("%s\n", (const char *)cmnd.c_str());
    ProcessManager  p( "creating classified output...", (const char *)cmnd.c_str() );
    if (p.getCancel()) {
        return 0;
    }
  }
  if (tCanvas->BOUNDARY_TYPE || mCanvas->mCavassData->m_vh.scn.num_of_bits==1) { /* Connected */
#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
    cmnd = wxString::Format("\"%s/fg_components\" \"%s\"",
        (const char *)Preferences::getHome().c_str(),
#else
	cmnd = wxString::Format("fg_components \"%s\"",
#endif
        (const char *)tmpfile.c_str());
    cmnd += wxString::Format(" \"%s\" 0 6", name);
    for (j=0; j<tCanvas->num_track_points; j++)
	  cmnd += wxString::Format(" %d %d %d",
	    tCanvas->track_pt[j][0], tCanvas->track_pt[j][1], tCanvas->track_pt[j][2]);
	printf("%s\n", (const char *)cmnd.c_str());
	ProcessManager  p( "tracking connected components...", (const char *)cmnd.c_str() );
    if (p.getCancel()) {
        return 0;
    }
#if 0
    int  error = p.getStatus();
    if (error != 0)
       wxMessageBox("Save failed.");
#endif
    if (mCanvas->mCavassData->m_vh.scn.num_of_bits > 1)
	{
	  cmnd = wxString::Format("%s_", name);
	  unlink((const char *)cmnd.c_str());
	}
  }
  return(0);
}


/************************************************************************
 *                                                                      *
 *      Function        : SaveBoundaryFile                              *
 *      Description     : This function will save the boundary file(BS0,*
 *                        BS1) when the SAVE button is pressed in the   *
 *                        button window.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  name - the name of the file to be saved.     *
 *                         ext - the extension of the file to be        *
 *                              to be created - BS0 or BS1.             *
 *                         range - the range of threshold values to be  *
 *                              used to create the boundary file.       *
 *                         bg_flag - whether to do this in the          *
 *                              background mode.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on March 10, 1993 by S.Samarasekera.  *
 *                        Modified 9/11/97 slice spacing checked
 *                        by Dewey Odhner.
 *                        Modified 10/22/99 hollow shell creation
 *                        implemented by Dewey Odhner.
 *                        Modified 2/1/00 distance map created for
 *                        hollow shell by Dewey Odhner.
 *                        Modified 2/11/00 distance map created
 *                        from uninterpolated scene by Dewey Odhner.
 *                        Modified 2/24/00 coat program called by Dewey Odhner.
 *                        Modified 3/14/00 fg_distance program called
 *                        by Dewey Odhner.
 *                        Modified 8/28/00 second filtering pass done for
 *                        hollow shell by Dewey Odhner.
 *                        Modified 11/2/00 filtering changed for
 *                        hollow shell by Dewey Odhner.
 *                        Modified: 11/3/00 tCanvas->min_table, tCanvas->max_table used
 *                           by Dewey Odhner.
 *                        Modified: 6/20/01 fg_components called
 *                           by Dewey Odhner.
 *                        Modified: 3/1/02 merge done for distance map
 *                           by Dewey Odhner.
 *                        Modified: 4/12/02 BS0h output by track_specified
 *                           & track_coats by Dewey Odhner.
 *                        Modified: 10/6/03 icon for BS2 made by Dewey Odhner.
 *                        Modified: 8/16/07 for CAVASS by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int ThresholdFrame::SaveBoundaryFile(const char *name, const char *ext,
	int range, int bg_flag)
{
  FILE *fp;
  int i,j,low,hi;
  wxString cmnd, points_file, pv_file;

  if (range==0) {
    wxMessageBox("Cannot create binary shell with multiple ranges.");
    return (0);
  }
    
  if (mCanvas->mCavassData->m_vh.scn.num_of_bits!=1 &&  (tCanvas->tbar1x[range]==-1 || tCanvas->tbar2x[range]==-1) ) {
    wxMessageBox("Specify range first.\n");
    return (0);
  }

  if (!slice_spacing_is_equal(&mCanvas->mCavassData->m_vh.scn))
  {
    wxMessageBox("Slices must be equally spaced for shell output.");
	return (0);
  }

  low= tCanvas->min_table[range];
  hi = tCanvas->max_table[range];
  if (tCanvas->BOUNDARY_TYPE==0 || strcmp(ext,"BS2")==0) { /* All faces */
    SetStatusText("Tracking Boundary. Wait....", 1);
    points_file = wxString::Format("%s_%d.TEMP", name, points_file_tag++);

    if (params_on) {
      pv_file = points_file+"PV";
      fp=fopen((const char *)pv_file.c_str(), "wb");
      if (fp==NULL) {
        wxMessageBox("Could not open parameter file");
		return (0);
      }
      fprintf(fp,"%d\n",num_params);
      for(i=0;i<num_params;i++) 
        fprintf(fp,"%d  ",param_type[i]);
      fprintf(fp,"\n");
      fprintf(fp,"%d\n",num_volumes);
      for(j=0;j<num_volumes;j++) {
        for(i=0;i<num_params;i++) 
          fprintf(fp,"%f  ",params[i][j]);
        fprintf(fp,"\n");
      }
      fclose(fp);
    }

#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
    cmnd = wxString::Format("\"%s/track_all\" ",
        (const char *)Preferences::getHome().c_str() );
#else
    cmnd = wxString::Format("track_all ");
#endif
	if (mCanvas->mCavassData->m_vh.scn.num_of_bits>1)      
        cmnd += wxString::Format("\"%s\" \"%s\" %f %f %f %s %d %d ",
            mCanvas->mCavassData->m_fname, name, resolution>1000? -1: resolution,
                (float)low, (float)hi, normal_dup[NORMAL_TYPE],
                MODE_TYPE, bg_flag);
    else 
        cmnd += wxString::Format("\"%s\" \"%s\" %f %f %f %s %d %d ",
            mCanvas->mCavassData->m_fname, name, resolution>1000? -1: resolution,
                (float)0.5, (float)1.0, normal_dup[NORMAL_TYPE],
                MODE_TYPE, bg_flag);

    if (params_on)
        cmnd += wxString::Format("\"%s\"", (const char *)pv_file.c_str());
	printf("%s\n", (const char *)cmnd.c_str());

    //construct the command string

    wxLogMessage( "command=%s", (const char *)cmnd.c_str() );
    ProcessManager  p( "track_all running...", cmnd );
    p.getStatus();
    if (p.getCancel()) {
        return 0;
    }

#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
    cmnd = wxString::Format("\"%s/track_all\" ",
        (const char *)Preferences::getHome().c_str() );
#else
    cmnd = wxString::Format("track_all ");
#endif
    if (strcmp(ext,"BS0")==0 || strcmp(ext,"BS2")==0) {
      if (mCanvas->mCavassData->m_vh.scn.num_of_bits>1)      
      {
	      cmnd += wxString::Format("\"%s\" \"%s", mCanvas->mCavassData->m_fname, name);
          cmnd.RemoveLast(); cmnd += wxString::Format("I\" %f %f %f %s %d %d ",
                mCanvas->mCavassData->m_vh.scn.xysize[0]/64.0,
                (float)low,(float)hi,normal_dup[NORMAL_TYPE],
                MODE_TYPE,bg_flag);
      }
	  else
      {
	      cmnd += wxString::Format("\"%s\" \"%s", mCanvas->mCavassData->m_fname,name);
          cmnd.RemoveLast(); cmnd += wxString::Format("I\" %f %f %f %s %d %d ",
                mCanvas->mCavassData->m_vh.scn.xysize[0]/64.0,
                (float)0.5,(float)1.0,normal_dup[NORMAL_TYPE],
                MODE_TYPE,bg_flag);
      }
	  if (params_on)
          cmnd += pv_file;
      printf("%s\n", (const char *)cmnd.c_str());

      //construct the command string
      wxLogMessage( "command=%s", (const char *)cmnd.c_str() );
      ProcessManager  p( "track_all running...", cmnd );
      p.getStatus();
      if (p.getCancel()) {
          return 0;
      }
    }
  }
  else
  {
    if (tCanvas->num_track_points==0) {
      wxMessageBox("Select Points before tracking\n");
	  return (0);
    }
    points_file = wxString::Format( "%s_%d.TEMP", name, points_file_tag++);
    fp=fopen((const char *)points_file.c_str(), "wb");
    if (fp==NULL) {
      wxMessageBox("Could not open points file");
	  return (0);
    }
    SetStatusText("Tracking. Wait .....", 1);
    fprintf(fp,"%d\n",tCanvas->num_track_points);
    
    for(i=0;i<tCanvas->num_track_points;i++)
      fprintf(fp,"%d %d %d\n",tCanvas->track_pt[i][2],tCanvas->track_pt[i][1],tCanvas->track_pt[i][0]);
    fclose(fp);
    
    if (params_on) {
      pv_file = points_file+"PV";
      fp=fopen((const char *)pv_file.c_str(), "wb");
      if (fp==NULL) {
        wxMessageBox("Could not open parameter file");
		SetStatusText("", 1);
		return (0);
      }
      fprintf(fp,"%d\n",num_params);
      for(i=0;i<num_params;i++) 
        fprintf(fp,"%d  ",param_type[i]);
      fprintf(fp,"\n");
      for(i=0;i<num_params;i++) 
        fprintf(fp,"%f  ",params[i][cur_volume]);
      fprintf(fp,"\n");
      fclose(fp);
    }

#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
    cmnd = wxString::Format("\"%s/track_3d\" ",
        (const char *)Preferences::getHome().c_str() );
#else
    cmnd = wxString::Format("track_3d ");
#endif
    if (mCanvas->mCavassData->m_vh.scn.num_of_bits>1)
      cmnd += wxString::Format("\"%s\" \"%s\" %f %f %f %d \"%s\" %s %d %d ",
                mCanvas->mCavassData->m_fname, name,
				resolution>1000? -1: resolution,
                (float)low,(float)hi,tCanvas->track_volume,
                (const char *)points_file.c_str(),
				normal_dup[NORMAL_TYPE],MODE_TYPE,bg_flag);
    else 
      cmnd += wxString::Format("\"%s\" \"%s\" %f %f %f %d \"%s\" %s %d %d ",
                mCanvas->mCavassData->m_fname, name,
				resolution>1000? -1: resolution,
                (float)0.5,(float)1.0,tCanvas->track_volume,
				(const char *)points_file.c_str(),
                normal_dup[NORMAL_TYPE],MODE_TYPE,bg_flag);
    
    if (params_on)
      cmnd += wxString::Format("\"%s\"", (const char *)pv_file.c_str());
    printf("%s\n", (const char *)cmnd.c_str());

    //construct the command string
    wxLogMessage( "command=%s", (const char *)cmnd.c_str() );
    ProcessManager  p( "track_3d running...", (const char *)cmnd.c_str() );
    p.getStatus();
    if (p.getCancel()) {
        return 0;
    }

    if (!strcmp(ext,"BS0")) {
      SetStatusText("Creating the icon. Wait ...\n", 1);
#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
    cmnd = wxString::Format("\"%s/track_3d\" ",
        (const char *)Preferences::getHome().c_str() );
#else
    cmnd = wxString::Format("track_3d ");
#endif
      if (mCanvas->mCavassData->m_vh.scn.num_of_bits>1)
      {
	      cmnd += wxString::Format("\"%s\" \"%s", mCanvas->mCavassData->m_fname, name);
	      cmnd.RemoveLast();
		  cmnd += wxString::Format("I\" %f %f %f %d \"%s\" %s %d %d ",
                mCanvas->mCavassData->m_vh.scn.xysize[0]/64.0,
                (float)low,(float)hi,tCanvas->track_volume,
				(const char *)points_file.c_str(),
                normal_dup[NORMAL_TYPE],MODE_TYPE,bg_flag);
      }
	  else
      {
	      cmnd += wxString::Format("\"%s\" \"%s", mCanvas->mCavassData->m_fname, name);
	      cmnd.RemoveLast();
		  cmnd += wxString::Format("I\" %f %f %f %d \"%s\" %s %d %d ",
                mCanvas->mCavassData->m_vh.scn.xysize[0]/64.0,
                (float)0.5,(float)1.0,tCanvas->track_volume,
				(const char *)points_file.c_str(),
                normal_dup[NORMAL_TYPE],MODE_TYPE,bg_flag);
      }
	  if (params_on)
	      cmnd += wxString::Format("\"%s\"", (const char *)pv_file.c_str());
      printf("%s\n", (const char *)cmnd.c_str());

      //construct the command string
      wxLogMessage( "command=%s", (const char *)cmnd.c_str() );
      ProcessManager  p( "track_3d running...", (const char *)cmnd.c_str() );
      p.getStatus();
      if (p.getCancel()) {
          return 0;
      }
    }
  }
  SetStatusText("", 1);
  return (0);
}

/************************************************************************
 *                                                                      *
 *      Function        : SaveBoundaryFile_1                            *
 *      Description     : Creates the shell (SH0 or SHI) from the       *
 *                        scene data according to the classification    *
 *                        function(s) specified.                        *
 *      Return Value    : None                                          *
 *      Parameters      : name:   Name of the saving file               *
 *                        ext:    File name extension (SH0 or SHI)      *
 *                        range:  The classification function chosen    *
 *                                0 - for all the functions             *
 *      Side effects    : None                                          *
 *      Limitations     : Creates only shell 0 files. `Gray' option     *
 *                        not implemented yet                           *
 *      Entry condition : Function returns with error message, when     *
 *                        1. the classification functions (one function *
 *                           in the case of single function mode) are   *
 *                           not specified or                           *
 *                        2. the opacities for the function(s) are not  *
 *                           specified or                               *
 *                        3. the resolution for the output cells is not *
 *                           specified.                                 *
 *      Related funcs   : VGetSaveFilename, VGetSaveSwitchValue,            *
 *                        VDisplayDialogMessage,                        *
 *                        callback_func, VDisplayStatus                 *
 *      History         : Written on June 28, 1993 by K P Venugopal     *
 *                        Modified 2/28/95 return without creating SHI
 *                        if SH0 fails by Dewey Odhner.
 *                        Modified 9/12/97 check for equal slice spacing
 *                        by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
/*@ \todo save parameters
 *@ \todo do background execution */
void ThresholdFrame::SaveBoundaryFile_1(const char *name, const char *ext,
	int range, int bg_flag)
{
    int           i;
    wxString      cmnd;
    float         sqr_pix, z_slice;
    char          msg[100];

    if (!slice_spacing_is_equal(&mCanvas->mCavassData->m_vh.scn))
    {
        wxMessageBox("Slices must be equally spaced for shell output.");
	    return;
    }
    if (range==0) {
        for (i = 1; i <= MAX_INTERVAL; i++) {
            if (tCanvas->tbar1x[i] < 0 || tCanvas->tbar2x[i] < 0) 
                if(tCanvas->tbar3x[i] < 0 || tCanvas->tbar4x[i] < 0) {
                    wxMessageBox("SPECIFY ALL THE FUNCTIONS BEFORE SAVING");
                	return;
                }
        }
        for (i = 1; i <= MAX_INTERVAL; i++) {
            if (opacity_factor[i] < 0.0) {
                opacity_factor[i] = 1.0;
            }
        }
        SetStatusText("Creating Shell with all the functions", 1);
    }
    else {
        if (tCanvas->tbar1x[range] < 0 || tCanvas->tbar2x[range] < 0) 
            if(tCanvas->tbar3x[range] < 0 || tCanvas->tbar4x[range] < 0) {
                sprintf(msg, "SPECIFY FUNCTION: %d ", range);
                wxMessageBox(msg);
                return;
            }
        if (opacity_factor[range] <= 0) {
            opacity_factor[range] = 0.98f;
        }
		SetStatusText("Creating Shell with the specified function", 1);

        if (range == 1) {
            opacity_factor[2] = opacity_factor[3] = 
                                opacity_factor[4] = 0.0;
            tCanvas->inter2_table[2] = 0;
            tCanvas->max_table[2] = 0;
            tCanvas->inter2_table[3] = 0;
            tCanvas->max_table[3] = 0;
        }
        if (range == 2) {
            opacity_factor[1] = opacity_factor[3] = 
                                opacity_factor[4] = 0.0;
            tCanvas->inter2_table[1] = tCanvas->min_table[2];
            tCanvas->max_table[1] = tCanvas->inter1_table[2];
            tCanvas->inter2_table[3] = 0;
            tCanvas->max_table[3] = 0;
        }
        if (range == 3) {
            opacity_factor[1] = opacity_factor[2] = 
                                opacity_factor[4] = 0.0;
            tCanvas->inter2_table[1] = 0;
            tCanvas->max_table[1] = 0;
            tCanvas->inter2_table[2] = tCanvas->min_table[3];
            tCanvas->max_table[2] = tCanvas->inter1_table[3];
        }
        if (range == 4) {
            opacity_factor[1] = opacity_factor[2] = 
                                opacity_factor[3] = 0.0;
            tCanvas->inter2_table[1] = 0;
            tCanvas->max_table[1] = 0;
            tCanvas->inter2_table[2] = 0;
            tCanvas->max_table[2] = 0;
            tCanvas->inter2_table[3] = tCanvas->min_table[4];
            tCanvas->max_table[3] = tCanvas->inter1_table[4];
        }
    }
    opacity_factor[1] = 0.0;
	SceneInfo *scn = &mCanvas->mCavassData->m_vh.scn;
	if (resolution > 1000)
	{
		sqr_pix = scn->xypixsz[0];
		z_slice = scn->dimension==4 && scn->num_of_subscenes[1]>1?
			scn->loc_of_subscenes[scn->num_of_subscenes[0]+1]-
			scn->loc_of_subscenes[scn->num_of_subscenes[0]]:
			scn->dimension==3 && scn->num_of_subscenes[0]>1?
			scn->loc_of_subscenes[1]-scn->loc_of_subscenes[0]:
			scn->xypixsz[0];
	}
	else
		sqr_pix = z_slice = scn->xypixsz[0]/resolution;

    if (scn->num_of_bits>1) {
#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
        cmnd = wxString::Format( "\"%s/shell\" ", (const char *)Preferences::getHome().c_str() );
#else
        cmnd = wxString::Format( "%s/shell " );
#endif
        cmnd += wxString::Format(
	        "\"%s\" %d %f %f %d %d %d %d %d %d %f %f %f %f %d %d %f %f %d \"%s\"",
	        mCanvas->mCavassData->m_fname, grad_factor, min_opac, max_opac,
			tCanvas->inter2_table[1], tCanvas->max_table[1],
			tCanvas->inter2_table[2], tCanvas->max_table[2], 
	        tCanvas->inter2_table[3], tCanvas->max_table[3], opacity_factor[1],
			opacity_factor[2], opacity_factor[3], opacity_factor[4], first_sl,
			last_sl, sqr_pix, z_slice, nbits, name );
        printf("%s\n", (const char *)cmnd.c_str());
        wxLogMessage( (const char *)cmnd.c_str() );
        ProcessManager  p( "shell running...", (const char *)cmnd.c_str() );

#ifdef PROCESS_MANAGER_TAKES_ABSOLUTE_PATH
        cmnd = wxString::Format( "\"%s/create_icon\" \"%s\"",
            (const char *)Preferences::getHome().c_str(), name );
#else
        cmnd = wxString::Format( "create_icon \"%s\"", name );
#endif
        printf("%s\n", (const char *)cmnd.c_str());
        wxLogMessage( (const char *)cmnd.c_str() );
        ProcessManager  p2( "creating icon...", (const char *)cmnd.c_str() );
    }
	SetStatusText("", 1);
}

/************************************************************************
 *                                                                      *
 *      Function        : SaveTextFile                                  *
 *      Description     : This function will save the text file
 *                        when the SAVE button is pressed in the
 *                        button window.                                *
 *      Return Value    :  0 - work successfully.                       *
 *      Parameters      :  name - the name of the file to be saved.     *
 *                         ext - the extension of the file to be        *
 *                              to be created - BS0 or BS1.             *
 *                         range - the range of threshold values to be  *
 *                              used to create the boundary file.       *
 *                         bg_flag - whether to do this in the          *
 *                              background mode.                        *
 *      Side effects    : None.                                         *
 *      Entry condition : mCanvas->mCavassData->m_fname, mCanvas->mCavassData->m_vh.scn.num_of_bits, tCanvas->tbar1x, tCanvas->tbar2x
 *      Related funcs   : None.                                         *
 *      History         : Written 11/12/03 by Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int ThresholdFrame::SaveTextFile(const char *name, const char *ext, int range, int bg_flag)
{
  FILE *fp;
  int low,hi;


  if (range==0) {
    wxMessageBox("Cannot Create TXT with multiple ranges");
    return (1);
  }
    
  if (mCanvas->mCavassData->m_vh.scn.num_of_bits!=1 &&  (tCanvas->tbar1x[range]==-1 || tCanvas->tbar2x[range]==-1) ) {
    wxMessageBox("Specify Range first\n");
    return (1);
  }

  low= tCanvas->min_table[range];
  hi = tCanvas->max_table[range];

  fp = fopen(name, MODE_TYPE? "ab": "wb");
  if (fp == NULL) {
    wxMessageBox("Could not open text file");
    return (1);
  }
  if (fprintf(fp, "%s %d %d\n", mCanvas->mCavassData->m_fname, low, hi) < 10)
  {
    wxMessageBox("Could not write text file");
	return (1);
  }
  fclose(fp);
  wxMessageBox("Interval written to TXT file");
  return (0);
}


/*****************************************************************************
 * FUNCTION: slice_spacing_is_equal
 * DESCRIPTION: Checks a 3-D scene or a 4-D scene for
 *    equal slice spacing (within 1 percent).
 * PARAMETERS:
 *    scn: The scene information
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if slice spacing is equal.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 9/11/97 by Dewey Odhner
 *
 *****************************************************************************/
int slice_spacing_is_equal(SceneInfo *scn)
{
	float min_space, max_space;
	int j, k, slices_left;

	if (scn->dimension!=3 && scn->dimension!=4)
		return (FALSE);
	slices_left = scn->num_of_subscenes[scn->dimension-3];
	if (slices_left < 2)
		return (FALSE);
	if (scn->dimension==4)
		for (j=1; j<scn->num_of_subscenes[0]; j++)
		{
			if (scn->num_of_subscenes[j+1] != scn->num_of_subscenes[1])
				return (FALSE);
			for (k=0; k<slices_left; k++)
				if (scn->loc_of_subscenes[scn->num_of_subscenes[0]+
						j*slices_left+k] !=
						scn->loc_of_subscenes[scn->num_of_subscenes[0]+k])
					return (FALSE);
		}
	j = scn->dimension==4? scn->num_of_subscenes[0]+1: 1;
	min_space = max_space= scn->loc_of_subscenes[j]-scn->loc_of_subscenes[j-1];
	for (j++,slices_left-=2; slices_left; j++,slices_left--)
		if (scn->loc_of_subscenes[j]-scn->loc_of_subscenes[j-1] < min_space)
			min_space = scn->loc_of_subscenes[j]-scn->loc_of_subscenes[j-1];
		else if(scn->loc_of_subscenes[j]-scn->loc_of_subscenes[j-1]> max_space)
			max_space = scn->loc_of_subscenes[j]-scn->loc_of_subscenes[j-1];
	return (min_space > .99*max_space);
}

// 7/11/07 Dewey Odhner
void ThresholdFrame::OnResolutionSlider ( wxScrollEvent& e ) {
    const int     newResolutionValue = e.GetPosition();
    const double  newResolution = newResolutionValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newResolution );
    mSetOutputControls->setResolutionText( s );
    resolution = newResolution>0? 1.0/newResolution: 1000000.;
}


// 7/13/07 Dewey Odhner
void ThresholdFrame::OnHistZoomSlider ( wxScrollEvent& e ) {
    const int     newHistZoomValue = e.GetPosition();
    const double  newHistZoom = newHistZoomValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newHistZoom );
    mHistZoomControls->setHistZoomText( s );
    tCanvas->setHistZoomFactor(newHistZoom);
	tCanvas->reload();
}

// 7/24/07 Dewey Odhner
void ThresholdFrame::OnSearchStepSlider ( wxScrollEvent& e ) {
	tCanvas->setOptStep( e.GetPosition() );
}

// 9/21/07 Dewey Odhner
void ThresholdFrame::OnOpacitySlider ( wxScrollEvent& e ) {
	assert(tCanvas->interval && tCanvas->interval<=MAX_INTERVAL);
	opacity_factor[tCanvas->interval] = .01*e.GetPosition();
}

// 9/20/07 Dewey Odhner
void ThresholdFrame::OnSurfaceStrengthSlider ( wxScrollEvent& e ) {
	grad_factor = e.GetPosition();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void ThresholdFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ThresholdFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ThresholdFrame::OnCTLung ( wxCommandEvent& unused ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTLungCenter() &&
			canvas->getWidth(0)==Preferences::getCTLungWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTLungCenter() );
	canvas->setWidth( 0, Preferences::getCTLungWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ThresholdFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTSoftTissueCenter() &&
			canvas->getWidth(0)==Preferences::getCTSoftTissueWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 0, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ThresholdFrame::OnCTBone ( wxCommandEvent& unused ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTBoneCenter() &&
			canvas->getWidth(0)==Preferences::getCTBoneWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getCTBoneCenter() );
	canvas->setWidth( 0, Preferences::getCTBoneWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ThresholdFrame::OnPET ( wxCommandEvent& unused ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getPETCenter() &&
			canvas->getWidth(0)==Preferences::getPETWidth())
	    return;  //no change
    canvas->setCenter( 0, Preferences::getPETCenter() );
	canvas->setWidth( 0, Preferences::getPETWidth() );
	canvas->setInvert( 0, true );
	mGrayMap1Controls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void ThresholdFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void ThresholdFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void ThresholdFrame::OnOverlay ( wxCommandEvent& e ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void ThresholdFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void ThresholdFrame::OnCineForward ( wxCommandEvent& e ) {
    if (e.IsChecked()) {
        mForward            = true;
        mForwardBackward    = false;
        mDirectionIsForward = true;
        mCineControls->mForwardBackwardCheck->SetValue( 0 );
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        m_cine_timer->Start( ::gTimerInterval, true );
    } else {
        m_cine_timer->Stop();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward & backward checkbox for both data sets */
void ThresholdFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
    if (e.IsChecked()) {
        mForward            = false;
        mForwardBackward    = true;
        mDirectionIsForward = true;
        mCineControls->mForwardCheck->SetValue( 0 );
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        m_cine_timer->Start(::gTimerInterval, true);
    } else {
        m_cine_timer->Stop();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine timer */
void ThresholdFrame::OnCineTimer ( wxTimerEvent& e ) {
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    const int  slices0 = canvas->getNoSlices(0);

    int  s0 = canvas->getSliceNo(0);


    if (mForward) {
        ++s0;

        if (s0>=slices0) {
            while (s0>0) {
                --s0;  
            }
        }
    } else {
        if (mDirectionIsForward) {
            ++s0;

            if (s0>=slices0) {
                while (s0>=slices0) {  --s0; }
                mDirectionIsForward = false;
            }
        } else {
            --s0;

            if (s0<0) {
                while (s0<0) {  ++s0; }
                mDirectionIsForward = true;
            }
        }
    }
    canvas->setSliceNo( 0, s0 );

    canvas->reload();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void ThresholdFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void ThresholdFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void ThresholdFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void ThresholdFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void ThresholdFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void ThresholdFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void ThresholdFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
    //especially (only) need on X11 (w/out GTK) to get slider events
    if (m_scale->GetValue()/100.0 == mCanvas->getScale())    return;
    
    const wxString  s = wxString::Format( "scale: %8.2f",
        m_scale->GetValue()/100.0 );
    m_scaleText->SetLabel( s );
    mCanvas->setScale( m_scale->GetValue()/100.0 );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void ThresholdFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new ThresholdPrint(wxString("CAVASS").c_str(), canvas),
        new ThresholdPrint(wxString("CAVASS").c_str(), canvas),
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
void ThresholdFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ThresholdCanvas*  canvas = dynamic_cast<ThresholdCanvas*>(mCanvas);
    //ThresholdPrint       printout( _T("CAVASS:Threshold"), canvas );
    ThresholdPrint*  printout;
    if (!fuzzy_flag)
        printout = new ThresholdPrint( wxString("CAVASS:Threshold").c_str(), canvas );
    else
        printout = new ThresholdPrint( wxString("CAVASS:1Feature").c_str(), canvas );
    if (!printer.Print(this, printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( ThresholdFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ThresholdFrame, wxFrame )
  DefineStandardFrameCallbacks
  EVT_BUTTON( ID_PREVIOUS,       ThresholdFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           ThresholdFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     ThresholdFrame::OnSetIndex1   )
  EVT_BUTTON( ID_GRAYMAP1,       ThresholdFrame::OnGrayMap1    )
  EVT_CHECKBOX( ID_BLINK,        ThresholdFrame::OnBlink       )
  EVT_BUTTON( ID_CINE,           ThresholdFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          ThresholdFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           ThresholdFrame::OnThresholdSave  )
  EVT_BUTTON( ID_SET_PARAMS,     ThresholdFrame::OnSetParams   )
  EVT_CHECKBOX( ID_PARAMS,       ThresholdFrame::OnParams      )
  EVT_BUTTON( ID_HIST_ZOOM,      ThresholdFrame::OnHistZoom    )
  EVT_BUTTON( ID_HIST_SCOPE,     ThresholdFrame::OnHistScope   )
  EVT_COMBOBOX( ID_INTERVAL,     ThresholdFrame::OnInterval    )
  EVT_BUTTON( ID_OPT_STEP,       ThresholdFrame::OnSearchStep  )
  EVT_BUTTON( ID_FIND_OPT,       ThresholdFrame::OnFindOpt     )
  EVT_COMBOBOX( ID_OUT_MODE,     ThresholdFrame::OnOutputMode  )
  EVT_COMBOBOX( ID_OUT_INTERVAL, ThresholdFrame::OnOutputInterval)
  EVT_COMBOBOX( ID_NORMAL_TYPE,  ThresholdFrame::OnNormalType  )
  EVT_COMBOBOX( ID_BOUNDARY_TYPE,ThresholdFrame::OnBoundaryType)
  EVT_BUTTON( ID_RESET_POINTS,   ThresholdFrame::OnResetPoints )
  EVT_BUTTON( ID_SET_OPACITY,    ThresholdFrame::OnSetOpacity )
  EVT_BUTTON( ID_SET_SURFACE_STRENGTH,ThresholdFrame::OnSetSurfaceStrength )
  EVT_BUTTON( ID_NUM_PARAMS,     ThresholdFrame::OnNumParams   )
  EVT_BUTTON( ID_WHICH_PARAM,    ThresholdFrame::OnWhichParam  )
  EVT_COMBOBOX( ID_PARAM_TYPE,   ThresholdFrame::OnParamType   )
  EVT_TEXT( ID_PARAM1_CTRL,      ThresholdFrame::OnSetParam1   )
  EVT_TEXT( ID_PARAM2_CTRL,      ThresholdFrame::OnSetParam2   )
  EVT_TEXT( ID_PARAM3_CTRL,      ThresholdFrame::OnSetParam3   )
  EVT_TEXT( ID_THRSLD_TABLET1,   ThresholdFrame::OnSetThreshold1 )
  EVT_TEXT( ID_THRSLD_TABLET2,   ThresholdFrame::OnSetThreshold2 )
  EVT_TEXT( ID_THRSLD_TABLET3,   ThresholdFrame::OnSetThreshold3 )
  EVT_TEXT( ID_THRSLD_TABLET4,   ThresholdFrame::OnSetThreshold4 )
  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, ThresholdFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  ThresholdFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG,          ThresholdFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   ThresholdFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          ThresholdFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              ThresholdFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  ThresholdFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  ThresholdFrame::OnScale1Slider  )
  EVT_COMMAND_SCROLL( ID_RESOLUTION,   ThresholdFrame::OnResolutionSlider)
  EVT_COMMAND_SCROLL(ID_HIST_ZOOM_SLIDER,ThresholdFrame::OnHistZoomSlider)
  EVT_COMMAND_SCROLL(ID_OPT_STEP_SLIDER,ThresholdFrame::OnSearchStepSlider)
  EVT_CHECKBOX(       ID_INVERT1,        ThresholdFrame::OnInvert1       )
  EVT_COMMAND_SCROLL( ID_OPACITY_SLIDER, ThresholdFrame::OnOpacitySlider )
  EVT_COMMAND_SCROLL(ID_SURFACE_STRENGTH_SLIDER,
    ThresholdFrame::OnSurfaceStrengthSlider)

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, ThresholdFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  ThresholdFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  ThresholdFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  ThresholdFrame::OnUpdateUIScale1Slider  )


  EVT_UPDATE_UI( ID_CINE_SLIDER,    ThresholdFrame::OnUpdateUICineSlider    )
#endif

  EVT_COMBOBOX( ID_OUT_TYPE,     ThresholdFrame::OnOutputType             )
  EVT_BUTTON( ID_VOLUME_NUMBER,  ThresholdFrame::OnVolumeNumber           )
  EVT_BUTTON( ID_SET_OUTPUT,     ThresholdFrame::OnSetOutput              )

  EVT_TIMER( ID_BUSY_TIMER, ThresholdFrame::OnBusyTimer )

  EVT_CHECKBOX( ID_OVERLAY,   ThresholdFrame::OnOverlay )


  EVT_TIMER(          ID_CINE_TIMER,            ThresholdFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          ThresholdFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, ThresholdFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           ThresholdFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  ThresholdFrame::_fileHistory;
//======================================================================
