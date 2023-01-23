/*
  Copyright 1993-2017 Medical Image Processing Group
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
#include  "SetIndexControls.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for OverlayFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
OverlayFrame::OverlayFrame ( ) : MainFrame( 0 )
{
    mFileOrDataCount = 0;

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       = mSetIndex2Box      = NULL;
    mSetIndex1Sizer     = mSetIndex2Sizer    = NULL;

    mCineControls       = NULL;
    mColor1Controls     = mColor2Controls    = NULL;
    mGrayMap1Controls   = mGrayMap2Controls  = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls  = mSetIndex2Controls = NULL;

    ::gFrameList.push_back( this );
    gWhere += cWhereIncr;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    
//    m_lastChoice = m_lastChoiceSetting = -1;    
//    m_zsub = m_ysub = NULL;
    
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -
    //wxSize  canvasSize( GetSize().GetWidth(), GetSize().GetHeight()-dControlsHeight );

    mCanvas = new OverlayCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight-30 );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    //          mainSizer->Add( middleSizer, 1,
    //                          wxGROW | (wxALL & ~(wxTOP | wxBOTTOM)), 10 );
    //          mainSizer->Add( 0, 5, 0, wxGROW ); // spacer in between
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    Maximize( true );
    Show();
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    Raise();
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Move/Next BIM",   2 );
    SetStatusText( "Flash1", 3 );
    SetStatusText( "Flash2", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void OverlayFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();
	mFileMenu->Enable( ID_INPUT, false );

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:Overlay:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right. */
void OverlayFrame::addButtonBox ( void ) {
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
    wxButton*  setIndex1 = new wxButton( mControlPanel, ID_SET_INDEX1, "SetIndex1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex1 );
    fgs->Add( setIndex1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    wxButton*  setIndex2 = new wxButton( mControlPanel, ID_SET_INDEX2, "SetIndex2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex2 );
    fgs->Add( setIndex2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
    wxButton*  grayMap1 = new wxButton( mControlPanel, ID_GRAYMAP1, "GrayMap1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap1 );
    fgs->Add( grayMap1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "GrayMap2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    wxButton*  color1 = new wxButton( mControlPanel, ID_COLOR1, "Color1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( color1 );
    fgs->Add( color1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    wxButton*  color2 = new wxButton( mControlPanel, ID_COLOR2, "Color2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( color2 );
    fgs->Add( color2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 1
    wxButton*  blink1 = new wxButton( mControlPanel, ID_BLINK1, "Blink1", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( blink1 );
    fgs->Add( blink1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    wxButton*  blink2 = new wxButton( mControlPanel, ID_BLINK2, "Blink2", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( blink2 );
    fgs->Add( blink2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 1
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 2
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for OverlayFrame class. */
OverlayFrame::~OverlayFrame ( void ) {
    cout << "OverlayFrame::~OverlayFrame" << endl;
    wxLogMessage( "OverlayFrame::~OverlayFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
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
/** \brief callback for key presses. */
void OverlayFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "OverlayFrame::OnChar" );
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
        cout << "OverlayFrame::OnChar: " << ::gTimerInterval << endl;
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for toggling control panel on and off. */
void OverlayFrame::OnHideControls ( wxCommandEvent& e ) {
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
/** \brief callback for Open menu item */
void OverlayFrame::OnOpen ( wxCommandEvent& e ) {
	OnVisOverlay(e);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void OverlayFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    wxString  tmp("CAVASS:Overlay: ");
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
	if (strcmp(fname, "*")==0 && mFileOrDataCount==2)
	{
		tmp = tmp+canvas->mCavassData->m_fname+", "+
			canvas->mCavassData->mNext->m_fname;
	}
	else
	{
	    if (!wxFile::Exists(fname)) {
	        wxString  tmp = wxString::Format( "File %s could not be opened.", fname );
	        wxMessageBox( tmp, "File does not exist",
	            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
	        return;
	    }
	    ++mFileOrDataCount;
	    if (mFileOrDataCount==1) {
	        tmp += fname;
	    } else if (mFileOrDataCount==2) {
	        tmp = mWindowTitle + ", " + fname;
	    } else {
	        assert( 0 );
	    }
	}
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "CAVASS:Overlay:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
	if (strcmp(fname, "*")==0 && mFileOrDataCount==2)
	{
		delete mGrayMap1Controls;
		mGrayMap1Controls = NULL;
		delete mGrayMap2Controls;
		mGrayMap2Controls = NULL;
		delete mSetIndex1Controls;
		mSetIndex1Controls = NULL;
		delete mSetIndex2Controls;
		mSetIndex2Controls = NULL;
		tmp = canvas->mCavassData->mNext->m_fname;
	}
	else
	{
	    canvas->loadFile( fname );
		tmp = fname;
	}
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    const wxString  s = wxString( "file " ) + tmp;
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void OverlayFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:Overlay: ");
    tmp += name;
    SetTitle( tmp );
    
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnPrevious ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;
    int  sliceB = canvas->getSliceNo(1) - 1;
	const int  slices0 = canvas->getNoSlices(0);
	const int  slices1 = canvas->getNoSlices(1);
    if (sliceA<0 && sliceB<0)
        while (sliceA<slices0-1 && sliceB<slices1-1)
		{
		    ++sliceA;  ++sliceB;
		}
    canvas->setSliceNo( 0, sliceA );
    canvas->setSliceNo( 1, sliceB );
    canvas->reload();
    if (mSetIndex1Controls!=NULL && sliceA<slices0)
	    mSetIndex1Controls->setSliceNo( sliceA );
    if (mSetIndex2Controls!=NULL && sliceB<slices1)
	    mSetIndex2Controls->setSliceNo( sliceB );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnNext ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;
    int  sliceB = canvas->getSliceNo(1) + 1;
	const int  slices0 = canvas->getNoSlices(0);
	const int  slices1 = canvas->getNoSlices(1);
    if (sliceA>=slices0 && sliceB>=slices1)
        while (sliceA>0 && sliceB>0)
		{
            --sliceA;  --sliceB;
        }
    canvas->setSliceNo( 0, sliceA );
    canvas->setSliceNo( 1, sliceB );
    canvas->reload();
    if (mSetIndex1Controls!=NULL && sliceA<slices0)
	    mSetIndex1Controls->setSliceNo( sliceA );
    if (mSetIndex2Controls!=NULL && sliceB<slices1)
	    mSetIndex2Controls->setSliceNo( sliceB );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnMouseWheel ( wxMouseEvent& e ) {
	const int  rot   = e.GetWheelRotation();
	wxCommandEvent  ce;
	if (rot>0)         OnPrevious(ce);
	else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mSetIndex1Controls!=NULL) return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mSetIndex1Controls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex1", canvas->getSliceNo(0), canvas->getNoSlices(0), ID_SLICE1_SLIDER,
        ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY, ID_MATCH_INDEX1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnSetIndex2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mSetIndex2Controls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex2", canvas->getSliceNo(1), canvas->getNoSlices(1), ID_SLICE2_SLIDER,
        ID_SCALE2_SLIDER, canvas->getScale(), ID_OVERLAY, ID_MATCH_INDEX2 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mGrayMap1Controls!=NULL)  return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG1, ID_CT_SOFT_TISSUE1, ID_CT_BONE1, ID_PET1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mGrayMap2Controls!=NULL)  return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap2", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		ID_CT_LUNG2, ID_CT_SOFT_TISSUE2, ID_CT_BONE2, ID_PET2 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnColor1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mColor1Controls!=NULL)    return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mColor1Controls = new ColorControls( mControlPanel, mBottomSizer, "Color1",
        canvas->getR(0), canvas->getG(0), canvas->getB(0),
        ID_RED1_SLIDER, ID_GREEN1_SLIDER, ID_BLUE1_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnColor2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mColor2Controls!=NULL)    return;

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    mColor2Controls = new ColorControls( mControlPanel, mBottomSizer, "Color2",
        canvas->getR(1), canvas->getG(1), canvas->getB(1),
        ID_RED2_SLIDER, ID_GREEN2_SLIDER, ID_BLUE2_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnRed1Slider ( wxScrollEvent& e ) {
    mColor1Controls->update();
    const int       newRedValue = e.GetPosition();
    const double    newRed      = (double)newRedValue / colorSliderMax;
    OverlayCanvas*  canvas      = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setR( 0, newRed );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnRed2Slider ( wxScrollEvent& e ) {
    mColor2Controls->update();
    const int       newRedValue = e.GetPosition();
    const double    newRed      = (double)newRedValue / colorSliderMax;
    OverlayCanvas*  canvas      = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setR( 1, newRed );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnGreen1Slider ( wxScrollEvent& e ) {
    mColor1Controls->update();
    const int       newGreenValue = e.GetPosition();
    const double    newGreen      = (double)newGreenValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setG( 0, newGreen );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnGreen2Slider ( wxScrollEvent& e ) {
    mColor2Controls->update();
    const int       newGreenValue = e.GetPosition();
    const double    newGreen      = (double)newGreenValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setG( 1, newGreen );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnBlue1Slider ( wxScrollEvent& e ) {
    mColor1Controls->update();
    const int       newBlueValue = e.GetPosition();
    const double    newBlue      = (double)newBlueValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setB( 0, newBlue );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnBlue2Slider ( wxScrollEvent& e ) {
    mColor2Controls->update();
    const int       newBlueValue = e.GetPosition();
    const double    newBlue      = (double)newBlueValue / colorSliderMax;
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setB( 1, newBlue );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnMatchIndex1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
	int j=canvas->getSliceNo(0);
	while (j >= canvas->getNoSlices(1))
		j -= canvas->getNoSlices(1);
    canvas->setSliceNo( 1, j );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnMatchIndex2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
	int j=canvas->getSliceNo(1);
	while (j >= canvas->getNoSlices(0))
		j -= canvas->getNoSlices(0);
    canvas->setSliceNo( 0, j );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnBlink1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->mCavassData->mDisplay = !canvas->mCavassData->mDisplay;
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnBlink2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->mCavassData->mNext->mDisplay = !canvas->mCavassData->mNext->mDisplay;
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnCine ( wxCommandEvent& unused ) {
    if (mColor1Controls!=NULL)    { delete mColor1Controls;     mColor1Controls=NULL;    }
    if (mColor2Controls!=NULL)    { delete mColor2Controls;     mColor2Controls=NULL;    }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mSetIndex2Controls!=NULL) { delete mSetIndex2Controls;  mSetIndex2Controls=NULL; }
    if (mCineControls!=NULL)      return;

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OverlayFrame::OnReset ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mColor1Controls!=NULL)     { delete mColor1Controls;      mColor1Controls=NULL;     }
    if (mColor2Controls!=NULL)     { delete mColor2Controls;      mColor2Controls=NULL;     }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }
    if (mGrayMap2Controls!=NULL)   { delete mGrayMap2Controls;    mGrayMap2Controls=NULL;   }
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mSetIndex2Controls!=NULL)  { delete mSetIndex2Controls;   mSetIndex2Controls=NULL;  }

    m_cine_timer->Stop();

    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 0;
    canvas->mCavassData->mDisplay = canvas->mCavassData->mNext->mDisplay = true;

    canvas->setSliceNo( 0, 0 );
	int max=canvas->getMax(0);
    canvas->setCenter(  0, max/2 );
    canvas->setWidth(   0, max<=1? 1:max );
    canvas->setInvert(  0, false );
	if (max <= 1)
    {
        canvas->setR(   0, 1.0 );
        canvas->setG(   0, 0.5 );
        canvas->setB(   0, 0.0 );
    }
    else
    {
        canvas->setR(   0, 1.0 );
        canvas->setG(   0, 1.0 );
        canvas->setB(   0, 1.0 );
    }
    canvas->setSliceNo( 1, 0 );
	max = canvas->getMax(1);
    canvas->setCenter(  1, max/2 );
    canvas->setWidth(   1, max<=1? 1:max );
    canvas->setInvert(  1, false );
    if (canvas->getMax(0) > 1)
    {
        canvas->setR(   1, 1.0 );
        canvas->setG(   1, 0.5 );
        canvas->setB(   1, 0.0 );
    }
    else if (max <= 1)
    {
        canvas->setR(   1, 0.0 );
        canvas->setG(   1, 0.5 );
        canvas->setB(   1, 1.0 );
    }
    else
    {
        canvas->setR(   1, 1.0 );
        canvas->setG(   1, 1.0 );
        canvas->setB(   1, 1.0 );
    }
    canvas->setScale( 1.0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom in button
 *  \todo implement multi resolution sliders
 */
void OverlayFrame::OnZoomIn ( wxCommandEvent& e ) {
#if 0
    ++mZoomLevel;
    if (mZoomLevel==0) {
        const double  before = m_scale->GetFPValue();
        m_scale->ResetFPRange();
        double  after = m_scale->GetFPValue();
        if (before<=after) {
            m_scale->SetFPValue( after );
            mCanvas->setScale( after  );
        } else {
            m_scale->SetFPRange( m_scale->GetFPMin(), before );
            m_scale->SetFPValue(  before );
            mCanvas->setScale( before  );
        }
        while (!mZoomValues.empty())    mZoomValues.pop();
        return;
    }
    if (mZoomLevel<0) {
        mZoomValues.pop();
        double  fpMin = mZoomValues.top().first;
        double  fpV   = m_scale->GetFPValue();
        double  fpMax = mZoomValues.top().second;
        if (fpV<fpMin)    fpMin = fpV;
        if (fpV>fpMax)    fpMax = fpV;
        m_scale->SetFPRange( fpMin, fpMax );
        return;
    }

    int  iMin = m_scale->GetMin();
    int  iV   = m_scale->GetValue();
    int  iMax = m_scale->GetMax();
    if (iMin>=iV || iMax<=iV)    return;

    int  iMinDiff = iV   - iMin;
    int  iMaxDiff = iMax - iV;
    int  iDiff    = iMaxDiff;
    if (iMinDiff<iMaxDiff)    iDiff = iMinDiff;
    iDiff -= 0.10 * iDiff;  //take off 10%
    //map the int values back into fp values
    double  newMin = m_scale->GetFPValue( iV-iDiff );
    double  newMax = m_scale->GetFPValue( iV+iDiff );
    m_scale->SetFPRange( newMin, newMax );
    mZoomValues.push( pair<double,double>(newMin, newMax) );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom out button
 *  \todo implement multi resolution sliders
 */
void OverlayFrame::OnZoomOut ( wxCommandEvent& e ) {
#if 0
    --mZoomLevel;
    if (mZoomLevel==0) {
        const double  before = m_scale->GetFPValue();
        m_scale->ResetFPRange();
        const double  after = m_scale->GetFPValue();
        if (before!=after)    mCanvas->setScale( after );
        while (!mZoomValues.empty())    mZoomValues.pop();
        return;
    }
    if (mZoomLevel>0) {
        mZoomValues.pop();
        double  fpMin = mZoomValues.top().first;
        double  fpV   = m_scale->GetFPValue();
        double  fpMax = mZoomValues.top().second;
        if (fpV<fpMin)    fpMin = fpV;
        if (fpV>fpMax)    fpMax = fpV;
        m_scale->SetFPRange( fpMin, fpMax );
        return;
    }

    int  iV   = m_scale->GetValue();
    int  iMax = m_scale->GetMax();
    iMax += 0.5 * iMax;  //increase

    double  newMax = m_scale->GetFPValue( iMax );
    if (newMax>1.0)    newMax = (int)(newMax+0.5);  //round it
    m_scale->SetFPRange( m_scale->GetFPMin(), newMax );
    mZoomValues.push( pair<double,double>(m_scale->GetFPMin(), newMax) );
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void OverlayFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 2 */
void OverlayFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void OverlayFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 2 */
void OverlayFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTLung button for data set 1 */
void OverlayFrame::OnCTLung1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getCTLungCenter() );
	canvas->setWidth( 0, Preferences::getCTLungWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTSoftTissue button for data set 1 */
void OverlayFrame::OnCTSoftTissue1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 0, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTBone button for data set 1 */
void OverlayFrame::OnCTBone1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getCTBoneCenter() );
	canvas->setWidth( 0, Preferences::getCTBoneWidth() );
	canvas->setInvert( 0, false );
	mGrayMap1Controls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnPET button for data set 1 */
void OverlayFrame::OnPET1 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 0, Preferences::getPETCenter() );
	canvas->setWidth( 0, Preferences::getPETWidth() );
	canvas->setInvert( 0, true );
	mGrayMap1Controls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTLung button for data set 2 */
void OverlayFrame::OnCTLung2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getCTLungCenter() );
	canvas->setWidth( 1, Preferences::getCTLungWidth() );
	canvas->setInvert( 1, false );
	mGrayMap2Controls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTSoftTissue button for data set 2 */
void OverlayFrame::OnCTSoftTissue2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 1, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 1, false );
	mGrayMap2Controls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTBone button for data set 2 */
void OverlayFrame::OnCTBone2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getCTBoneCenter() );
	canvas->setWidth( 1, Preferences::getCTBoneWidth() );
	canvas->setInvert( 1, false );
	mGrayMap2Controls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnPET button for data set 2 */
void OverlayFrame::OnPET2 ( wxCommandEvent& unused ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setCenter( 1, Preferences::getPETCenter() );
	canvas->setWidth( 1, Preferences::getPETWidth() );
	canvas->setInvert( 1, true );
	mGrayMap2Controls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void OverlayFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 2 */
void OverlayFrame::OnSlice2Slider ( wxScrollEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    if (canvas->getSliceNo(1)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 1, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void OverlayFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 2 */
void OverlayFrame::OnScale2Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex2Controls->setScaleText( s );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void OverlayFrame::OnOverlay ( wxCommandEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void OverlayFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 2 */
void OverlayFrame::OnInvert2 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void OverlayFrame::OnCineForward ( wxCommandEvent& e ) {
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
void OverlayFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void OverlayFrame::OnCineTimer ( wxTimerEvent& e ) {
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    const int  slices0 = canvas->getNoSlices(0);
    const int  slices1 = canvas->getNoSlices(1);
    int  s0 = canvas->getSliceNo(0);
    int  s1 = canvas->getSliceNo(1);

    if (mForward) {
        ++s0;
        ++s1;
        if (s0>=slices0 && s1>=slices1) {
            while (s0>0 && s1>0) {
                --s0;  --s1;
            }
        }
    } else {
        if (mDirectionIsForward) {
            ++s0;
            ++s1;
            if (s0>=slices0 && s1>=slices1) {
                while (s0>=slices0 && s1>=slices1) {  --s0;  --s1;  }
                mDirectionIsForward = false;
            }
        } else {
            --s0;
            --s1;
            if (s0<0 && s1<0) {
                while (s0<0 && s1<0) {  ++s0;  ++s1;  }
                mDirectionIsForward = true;
            }
        }
    }
    canvas->setSliceNo( 0, s0 );
    canvas->setSliceNo( 1, s1 );
    canvas->reload();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void OverlayFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void OverlayFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void OverlayFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void OverlayFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void OverlayFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void OverlayFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void OverlayFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void OverlayFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new OverlayPrint(wxString("CAVASS").c_str(), canvas),
        new OverlayPrint(wxString("CAVASS").c_str(), canvas),
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
/** \brief callback for printing */
void OverlayFrame::OnPrint ( wxCommandEvent& e ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    OverlayCanvas*  canvas = dynamic_cast<OverlayCanvas*>(mCanvas);
    OverlayPrint       printout( wxString("CAVASS:Overlay").c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( OverlayFrame, wxFrame )
BEGIN_EVENT_TABLE       ( OverlayFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_MOUSEWHEEL(                OverlayFrame::OnMouseWheel  )
  EVT_BUTTON( ID_PREVIOUS,       OverlayFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           OverlayFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     OverlayFrame::OnSetIndex1   )
  EVT_BUTTON( ID_SET_INDEX2,     OverlayFrame::OnSetIndex2   )
  EVT_BUTTON( ID_GRAYMAP1,       OverlayFrame::OnGrayMap1    )
  EVT_BUTTON( ID_GRAYMAP2,       OverlayFrame::OnGrayMap2    )
  EVT_BUTTON( ID_COLOR1,         OverlayFrame::OnColor1      )
  EVT_BUTTON( ID_COLOR2,         OverlayFrame::OnColor2      )
  EVT_BUTTON( ID_MATCH_INDEX1,   OverlayFrame::OnMatchIndex1 )
  EVT_BUTTON( ID_MATCH_INDEX2,   OverlayFrame::OnMatchIndex2 )
  EVT_BUTTON( ID_BLINK1,         OverlayFrame::OnBlink1      )
  EVT_BUTTON( ID_BLINK2,         OverlayFrame::OnBlink2      )
  EVT_BUTTON( ID_CINE,           OverlayFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          OverlayFrame::OnReset       )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, OverlayFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  OverlayFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG1,          OverlayFrame::OnCTLung1  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE1,   OverlayFrame::OnCTSoftTissue1  )
  EVT_BUTTON( ID_CT_BONE1,          OverlayFrame::OnCTBone1  )
  EVT_BUTTON( ID_PET1,              OverlayFrame::OnPET1     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  OverlayFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  OverlayFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        OverlayFrame::OnInvert1       )
  EVT_COMMAND_SCROLL( ID_RED1_SLIDER,    OverlayFrame::OnRed1Slider    )
  EVT_COMMAND_SCROLL( ID_GREEN1_SLIDER,  OverlayFrame::OnGreen1Slider  )
  EVT_COMMAND_SCROLL( ID_BLUE1_SLIDER,   OverlayFrame::OnBlue1Slider   )

  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, OverlayFrame::OnCenter2Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  OverlayFrame::OnWidth2Slider  )
  EVT_BUTTON( ID_CT_LUNG2,          OverlayFrame::OnCTLung2  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE2,   OverlayFrame::OnCTSoftTissue2  )
  EVT_BUTTON( ID_CT_BONE2,          OverlayFrame::OnCTBone2  )
  EVT_BUTTON( ID_PET2,              OverlayFrame::OnPET2     )
  EVT_COMMAND_SCROLL( ID_SLICE2_SLIDER,  OverlayFrame::OnSlice2Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE2_SLIDER,  OverlayFrame::OnScale2Slider  )
  EVT_CHECKBOX(       ID_INVERT2,        OverlayFrame::OnInvert2       )
  EVT_COMMAND_SCROLL( ID_RED2_SLIDER,    OverlayFrame::OnRed2Slider    )
  EVT_COMMAND_SCROLL( ID_GREEN2_SLIDER,  OverlayFrame::OnGreen2Slider  )
  EVT_COMMAND_SCROLL( ID_BLUE2_SLIDER,   OverlayFrame::OnBlue2Slider   )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, OverlayFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  OverlayFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  OverlayFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  OverlayFrame::OnUpdateUIScale1Slider  )
  EVT_UPDATE_UI( ID_RED1_SLIDER,    OverlayFrame::OnUpdateUIRed1Slider    )
  EVT_UPDATE_UI( ID_GREEN1_SLIDER,  OverlayFrame::OnUpdateUIGreen1Slider  )
  EVT_UPDATE_UI( ID_BLUE1_SLIDER,   OverlayFrame::OnUpdateUIBlue1Slider   )

  EVT_UPDATE_UI( ID_CENTER2_SLIDER, OverlayFrame::OnUpdateUICenter2Slider )
  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  OverlayFrame::OnUpdateUIWidth2Sglider )
  EVT_UPDATE_UI( ID_SLICE2_SLIDER,  OverlayFrame::OnUpdateUISlice2Slider  )
  EVT_UPDATE_UI( ID_SCALE2_SLIDER,  OverlayFrame::OnUpdateUIScale2Slider  )
  EVT_UPDATE_UI( ID_RED2_SLIDER,    OverlayFrame::OnUpdateUIRed2Slider    )
  EVT_UPDATE_UI( ID_GREEN2_SLIDER,  OverlayFrame::OnUpdateUIGreen2Slider  )
  EVT_UPDATE_UI( ID_BLUE2_SLIDER,   OverlayFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    OverlayFrame::OnUpdateUICineSlider    )
#endif

  EVT_BUTTON( ID_ZOOM_IN,        OverlayFrame::OnZoomIn                 )
  EVT_BUTTON( ID_ZOOM_OUT,       OverlayFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, OverlayFrame::OnBusyTimer )

  EVT_MENU( ID_EXAMPLE,        MainFrame::OnExample      )
  EVT_MENU( ID_VIS_MONTAGE,    MainFrame::OnMontage      )
  EVT_MENU( ID_VIS_OVERLAY,    MainFrame::OnVisOverlay   )
  EVT_MENU( ID_VIS_SURF_VIEW,  MainFrame::OnVisSurfView  )

  EVT_CHAR( OverlayFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   OverlayFrame::OnOverlay )

  EVT_TIMER(          ID_CINE_TIMER,            OverlayFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          OverlayFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, OverlayFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           OverlayFrame::OnCineSlider )
END_EVENT_TABLE()
//======================================================================
