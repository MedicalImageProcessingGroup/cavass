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
 * \file   VOIStandardlizeFrame.cpp
 * \brief  VOIStandardlizeFrame class implementation.
 *         external programs used:
 *           
 * \author Xinjian Chen, Ph.D 
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#include  "cavass.h"
#include  "CineControls.h"
#include  "ScaleADControls.h"
#include  "SetFilterIndexControls.h"
#include  "SetStandardlizeControls.h"
//#include  "SetHistZoomControls.h"
#include  "ThresholdFrame.h"
//#include  "SetLayout.h"
#include  "VOIStandardlizeFrame.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for VOIStandardlizeFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
VOIStandardlizeFrame::VOIStandardlizeFrame (): MainFrame( 0 )
{
    mFileOrDataCount = 0;	
	
    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       = NULL;
    mCineControls       = NULL;
    
    mGrayMap1Controls   = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls   = NULL;  
	
	mSetLowHighControls = NULL;	
	mParaFileName = "";
	mTrainFileNames.Clear();

	mModeType = NULL;   
	
    ::gFrameList.push_back( this );
    gWhere += cWhereIncr;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    
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

    mCanvas = new VOIStandardlizeCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
//	mLayoutControls = new LayoutControls( mControlPanel, mBottomSizer, "Layout", ID_CHECKBOX_LAYOUT );

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
    SetStatusText( "LOC/Size",   2 );
    SetStatusText( "ACCEPT", 3 );
    SetStatusText( "DONE", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void VOIStandardlizeFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:VOIStandardlize:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const static char* ModeTypeTable[] = 
{	
	"Mode:Transform",
	"Mode:Training"
};

const static char* MethodsTypeTable[] = 
{
	"Method:MEDIAN",
	"Method:4-ILE",
	"Method:DECILE",
	"Method:GBSCALE",
	"Method:INTRCT"
};

void VOIStandardlizeFrame::addButtonBox ( void ) {
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
    wxButton*  setIndex1 = new wxButton( mControlPanel, ID_SET_INDEX1, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex1 );
    fgs->Add( setIndex1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    wxButton*  grayMap1 = new wxButton( mControlPanel, ID_GRAYMAP1, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap1 );
    fgs->Add( grayMap1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1	
	mModeType = new wxComboBox( mControlPanel, ID_MODETYPE, "Mode", wxDefaultPosition, wxSize(buttonWidth,buttonHeight), 0, NULL, wxCB_READONLY );
	::setColor( mModeType );
	for (int i=0; i < 2; i++) 
	{
		mModeType->Append( ModeTypeTable[i] );
	}
	mModeType->SetSelection( 0 );
    fgs->Add( mModeType, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   
	 //row 3, col 2    
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   
	//row 4, col 1
	msetParaFileB = new wxButton( mControlPanel, ID_SET_PARAFILE, "SelParFile", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( msetParaFileB );
    fgs->Add( msetParaFileB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    mLowHighB = new wxButton( mControlPanel, ID_LOWHIGN, "Low/High", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mLowHighB );
	mLowHighB->Enable(false);
    fgs->Add( mLowHighB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    
	//row 5, col 1	
	mTrainMethodCB = new wxComboBox( mControlPanel, ID_TRAINMETHOD, "Method", wxDefaultPosition, wxSize(buttonWidth,buttonHeight), 0, NULL, wxCB_READONLY );
	::setColor( mTrainMethodCB );
	for (int i=0; i < 5; i++) 
	{
		mTrainMethodCB->Append( MethodsTypeTable[i] );
	}
	mTrainMethodCB->SetSelection( 0 );
	mTrainMethodCB->Enable(false);
    fgs->Add( mTrainMethodCB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

	 //row 5, col 2
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for VOIStandardlizeFrame class. */
VOIStandardlizeFrame::~VOIStandardlizeFrame ( void ) {
    cout << "VOIStandardlizeFrame::~VOIStandardlizeFrame" << endl;
    wxLogMessage( "VOIStandardlizeFrame::~VOIStandardlizeFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
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
/** \brief callback for key presses. */
void VOIStandardlizeFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "VOIStandardlizeFrame::OnChar" );
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
        cout << "VOIStandardlizeFrame::OnChar: " << ::gTimerInterval << endl;
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
void VOIStandardlizeFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void VOIStandardlizeFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
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
void VOIStandardlizeFrame::OnHideControls ( wxCommandEvent& e ) {
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
void VOIStandardlizeFrame::OnOpen ( wxCommandEvent& e ) 
{
    OnPPScopsVOIStandardize( e );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void VOIStandardlizeFrame::loadFile ( wxArrayString  FileNames ) 
{
	if (FileNames.GetCount()<1) 
	{
        wxMessageBox( "At least 1 file should be specified." );
        return;
    } 	
    
	for( unsigned int i=0; i<FileNames.Count(); i++ )
    _fileHistory.AddFileToHistory( FileNames[i] );

	mTrainFileNames = FileNames;

    wxString  tmp("CAVASS:Standardize: ");
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

            wxString  tmp = wxString("CAVASS:Standardize: ");
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
   
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    canvas->loadFile( FileNames[0].c_str() );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    const wxString  s = wxString( "file %s") + wxString(FileNames[0]) ;
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void VOIStandardlizeFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:VOIStandardlize: ");
    tmp += name;
    SetTitle( tmp );
    
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIStandardlizeFrame::OnPrevious ( wxCommandEvent& unused ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;

    if (sliceA<0 )
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIStandardlizeFrame::OnNext ( wxCommandEvent& unused ) 
{
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;

    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIStandardlizeFrame::OnSetIndex1 ( wxCommandEvent& unused ) 
{
	 if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
	if (mSetLowHighControls!=NULL)   { delete mSetLowHighControls;    mSetLowHighControls=NULL;   }    
	
   if (mSetIndex1Controls!=NULL) return;

    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFilterIndexControls( mControlPanel, mBottomSizer,
        "SetIndex1", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
			     ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIStandardlizeFrame::OnGrayMap1 ( wxCommandEvent& unused )
{
	 if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
//    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
	if (mSetLowHighControls!=NULL)   { delete mSetLowHighControls;    mSetLowHighControls=NULL;   }    

    if (mGrayMap1Controls!=NULL)  return;

    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIStandardlizeFrame::OnCine ( wxCommandEvent& unused )
{
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
	
	if (mSetLowHighControls!=NULL)   { delete mSetLowHighControls;    mSetLowHighControls=NULL;   }    

    if (mCineControls!=NULL)      return;

//	canvas->setVOIStandardlizeDone(false);

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void VOIStandardlizeFrame::OnModeType ( wxCommandEvent& e ) 
{

 //   canvas->setROIStatisticalDone( false );

	int nWhich = mModeType->GetSelection();  

	if( nWhich == 1 ) // training 
	{
		msetParaFileB->Enable(false);
		mLowHighB->Enable(true);
		mTrainMethodCB->Enable(true);

	}
	else  // transform
	{
		msetParaFileB->Enable(true);
		mLowHighB->Enable(false);
		mTrainMethodCB->Enable(false);	
	}
 
//	canvas->setModeType(nWhich);
   
}

//-------------------------------------------------------------------------------------------
void VOIStandardlizeFrame::OnReset ( wxCommandEvent& unused ) 
{
    if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    
	if (mSetLowHighControls!=NULL)   { delete mSetLowHighControls;    mSetLowHighControls=NULL;   }    
	

    m_cine_timer->Stop();

    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 40;
    canvas->mCavassData->mDisplay = true;
	//canvas->mCavassData->mNext->mDisplay = true;

    canvas->setVOIStandardlizeDone(false);   

    canvas->setSliceNo( 0, 0 );
    canvas->setCenter(  0, canvas->getMax(0)/2 );
    canvas->setWidth(   0, canvas->getMax(0)   );
    canvas->setInvert(  0, false );
 //   canvas->setScale( 1.0 );

//	canvas->RemoveProfileInfo();   

    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VOIStandardlizeFrame::OnVOIStandardlizeSave ( wxCommandEvent &e ) 
{  
	
	VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);	

	wxString command, file;
	int j, k, back_flag, num_files, num_masks;
	FILE *fp;
	float ff;

	int standardize_mode = mModeType->GetSelection();   // 0: transform; 1: training

	if (standardize_mode)
	{
		num_files = mTrainFileNames.Count();     
	}
	else
	{
		num_files = 1;
	}

	for (j=k=0; j<num_files; j++)
		if ((int)mTrainFileNames[j].Len() > k)
			k = mTrainFileNames[j].Len();
	char *mask_file=(char *)malloc(k+10);

	if (standardize_mode==0 && mParaFileName == "" )
	{
		wxMessageBox("Select a parameter file first.");		
		free(mask_file);
		return;
	}

	wxFileDialog *saveDlg;
	if (standardize_mode == 0)
		saveDlg= new wxFileDialog( this, _T("Standardize Scene"), wxEmptyString,
                           _T("stdize-tmp.IM0"),
						   _T("CAVASS files (*.IM0)|*.IM0"),
                           wxSAVE|wxOVERWRITE_PROMPT );
	else
		saveDlg= new wxFileDialog( this, _T("Save Parameters"), wxEmptyString,
                           _T("stdize-tmp.PAR"),
						   _T("Parameter files (*.PAR)|*.PAR"),
                           wxSAVE|wxOVERWRITE_PROMPT );

    if (saveDlg->ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, Standardize %d"), (const char *)saveDlg->GetPath().c_str(), saveDlg->GetFilterIndex() );
    }
	else
	{
		delete saveDlg;
		free(mask_file);
		return;
	}

	file = saveDlg->GetPath();
	delete saveDlg;

	/* Check if input file name and output file name are the same */
	if(strcmp( ((const char *)mTrainFileNames[0].c_str()), (const char *)file.c_str()) == 0)
	{
		wxMessageBox("INPUT AND OUTPUT FILES HAVE THE SAME NAME. PLEASE CHANGE OUTPUT NAME.");		
		free(mask_file);
		return;
	}

	num_masks = 0;
	int standardize_method = mTrainMethodCB->GetSelection();
	if (standardize_method == 4)
	{
		for (;; num_masks++)
		{
			strcpy(mask_file, ((const char *)mTrainFileNames[0].c_str()) );
			sprintf(mask_file+strlen(mask_file)-4,"_tr%d.BIM",num_masks+1);
			fp = fopen(mask_file, "rb");
			if (fp == NULL)
				break;
			fclose(fp);
		}
		if (num_masks == 0)
		{
			wxMessageBox(wxString("Cannot find ")+mask_file);
			free(mask_file);
			return;
		}
	}

	back_flag = 0;

	if (standardize_mode)
	{
		float low_percentile, high_percentile;
		low_percentile = (float)canvas->getLowIle();
		high_percentile = (float)canvas->getHighIle();

		command = "\"";
		command += Preferences::getHome();
		if (standardize_method == 3)
			command += wxString("/gbscale2_train\" \"")+file+"\" -files";
		else if (standardize_method == 4)
			command += wxString::Format("/mrscaleprog\" -train -percentile %.2f %.2f -new_scale 0 4095 -method 6 -paramfile_out \"%s\" -landmark_number %d -files",
				low_percentile, high_percentile, (const char *)file.c_str(), num_masks);
		else
			command += wxString::Format("/mrscaleprog\" -train -percentile %.2f %.2f -new_scale 1 4095 -method %d -percentile_num %d -first_slice 0 -last_slice 100 -paramfile_out \"%s\" -files",
				low_percentile,high_percentile, standardize_method?5:4,
				standardize_method? standardize_method==2? 10:4:2, (const char *)file.c_str());
		for (j=0; j<num_files; j++)
		{
			command += wxString(" \"")+mTrainFileNames[j]+"\"";
			strcpy(mask_file, ((const char *)mTrainFileNames[j].c_str()) );
			mask_file[strlen(mask_file)-4] = 0;
			for (k=1; k<=num_masks; k++)
			{
				wxString mf(wxString::Format("%s_tr%d.BIM", mask_file,k));
				fp = fopen((const char *)mf.c_str(), "rb");
				if (fp == NULL)
				{
					wxMessageBox(wxString("Cannot find ")+mf);
					free(mask_file);
					return;
				}
				fclose(fp);
				command = command+" \""+mf+"\"";
			}
		}
	}
	else
	{
		fp = fopen((const char *)(mParaFileName.c_str()), "rb");
		j = fscanf(fp, "%f", &ff);
		if (j == 1)
		{
			command = "\"";
			command += Preferences::getHome()+wxString("/gbscale2_scale\" \"")+mTrainFileNames[0]+"\" \""+
				file+"\" \""+mParaFileName+
				"\" -radius 2 -hist_thresh 0.6 -num_regions 1";
		}
		else
		{
			j = fscanf(fp, "Method:               %d\n", &standardize_method);
			if (j != 1)
			{
				wxMessageBox("Cannot read parameter file.");
				fclose(fp);
				free(mask_file);
				return;
			}
			command = "\"";
			command += Preferences::getHome()+wxString("/mrscaleprog\"  -apply -paramfile_in \"")+
				mParaFileName+"\" -files \""+mTrainFileNames[0]+"\" \""+
				file+"\"";
			if (standardize_method == 6)
			{
				fscanf(fp, "First slice for hist: %f\n", &ff);
				fscanf(fp, "Last slice for hist:  %f\n", &ff);
				fscanf(fp, "Minimum percentile:   %f\n", &ff);
				fscanf(fp, "Maximum percentile:   %f\n", &ff);
				fscanf(fp, "Minimum scale:        %f\n", &ff);
				fscanf(fp, "Maximum scale:        %f\n", &ff);
				if (fscanf(fp, "Number of landmarks:  %d", &num_masks) != 1)
				{
					wxMessageBox("Cannot read parameter file.");
					fclose(fp);
					free(mask_file);
					return;
				}
				strcpy(mask_file, ((const char *)mTrainFileNames[0].c_str()));
				mask_file[strlen(mask_file)-4] = 0;
				for (k=1; k<=num_masks; k++)
					command +=
						wxString::Format(" \"%s_tr%d.BIM\"", mask_file, k);
			}
		}
		fclose(fp);
	}
	printf("command=%s\n", (const char *)command.c_str());
	fflush(stdout);
	free(mask_file);
	wxLogMessage( "command=%s", (const char *)command.c_str() );
	ProcessManager  q( "Standardlize running...", (const char *)command.c_str() );
	if (q.getCancel())    return;
#if 0
	int  error = q.getStatus();
	if (error != 0) {  wxMessageBox( "Standardize failed." );  return;  }
#endif

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void VOIStandardlizeFrame::OnSetParaFileName ( wxCommandEvent &e ) 
{
    wxFileDialog  selDlg( this, _T("Please Select Par File"), wxEmptyString,
                           _T("VOI_Slices"), _T("PAR files (*.PAR;*.par)|*.PAR;*.par"),
                           wxOPEN );

    if (selDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, VOIStandardlize %d"), (const char *)selDlg.GetPath().c_str(), selDlg.GetFilterIndex() );
    }
	else
		return;
	
	mParaFileName = selDlg.GetPath();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void VOIStandardlizeFrame::OnSetLowHighIle ( wxCommandEvent &e ) 
{
	if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
	if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
	if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
	if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    

	if (mSetLowHighControls!=NULL)  return;

	VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);

	mSetLowHighControls = new SetLowHighControls( mControlPanel, mBottomSizer,
		"Set Low/High", canvas->getLowIle(), ID_SET_LOWILE, canvas->getHighIle(), ID_SET_HIGHILE );
}

/** \brief callback for center slider for data set 1 */
void VOIStandardlizeFrame::OnSetLowPctile ( wxCommandEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
	float l;
	sscanf((const char *)e.GetString().c_str(), "%f", &l);
    canvas->setLowIle( l );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIStandardlizeFrame::OnSetHighPctile ( wxCommandEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
	float h;
	sscanf((const char *)e.GetString().c_str(), "%f", &h);
    canvas->setHighIle( h );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void VOIStandardlizeFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIStandardlizeFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void VOIStandardlizeFrame::OnCTLung ( wxCommandEvent& unused ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
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
void VOIStandardlizeFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
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
void VOIStandardlizeFrame::OnCTBone ( wxCommandEvent& unused ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
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
void VOIStandardlizeFrame::OnPET ( wxCommandEvent& unused ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
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
void VOIStandardlizeFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void VOIStandardlizeFrame::OnScale1Slider ( wxScrollEvent& e ) 
{
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void VOIStandardlizeFrame::OnOverlay ( wxCommandEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void VOIStandardlizeFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void VOIStandardlizeFrame::OnCineForward ( wxCommandEvent& e ) {
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
void VOIStandardlizeFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void VOIStandardlizeFrame::OnCineTimer ( wxTimerEvent& e ) {
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
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
void VOIStandardlizeFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void VOIStandardlizeFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void VOIStandardlizeFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void VOIStandardlizeFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void VOIStandardlizeFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void VOIStandardlizeFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void VOIStandardlizeFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void VOIStandardlizeFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new VOIStandardlizePrint("CAVASS", canvas),
        new VOIStandardlizePrint("CAVASS", canvas),
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
void VOIStandardlizeFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    VOIStandardlizeCanvas*  canvas = dynamic_cast<VOIStandardlizeCanvas*>(mCanvas);
    VOIStandardlizePrint       printout( _T("CAVASS:VOIStandardlize"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( VOIStandardlizeFrame, wxFrame )
BEGIN_EVENT_TABLE       ( VOIStandardlizeFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
#if 0
  EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           VOIStandardlizeFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    VOIStandardlizeFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           VOIStandardlizeFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  VOIStandardlizeFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          VOIStandardlizeFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, VOIStandardlizeFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )

  EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile )
#endif
  EVT_BUTTON( ID_PREVIOUS,       VOIStandardlizeFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           VOIStandardlizeFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     VOIStandardlizeFrame::OnSetIndex1   )

  EVT_BUTTON( ID_GRAYMAP1,       VOIStandardlizeFrame::OnGrayMap1    )
 EVT_BUTTON( ID_CINE,           VOIStandardlizeFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          VOIStandardlizeFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           VOIStandardlizeFrame::OnVOIStandardlizeSave  )

  EVT_BUTTON( ID_SET_PARAFILE,           VOIStandardlizeFrame::OnSetParaFileName  )
  EVT_BUTTON( ID_LOWHIGN,           VOIStandardlizeFrame::OnSetLowHighIle  )
  EVT_TEXT( ID_SET_LOWILE, VOIStandardlizeFrame::OnSetLowPctile )
  EVT_TEXT( ID_SET_HIGHILE,   VOIStandardlizeFrame::OnSetHighPctile )
  
  
  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, VOIStandardlizeFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  VOIStandardlizeFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG,          VOIStandardlizeFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   VOIStandardlizeFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          VOIStandardlizeFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              VOIStandardlizeFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  VOIStandardlizeFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  VOIStandardlizeFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        VOIStandardlizeFrame::OnInvert1       )
  EVT_COMBOBOX( ID_MODETYPE,    VOIStandardlizeFrame::OnModeType    )  
//  EVT_CHECKBOX(       ID_CHECKBOX_LAYOUT,       VOIStandardlizeFrame::OnLayout       )
//   EVT_COMMAND_SCROLL( ID_RED1_SLIDER,    VOIStandardlizeFrame::OnRed1Slider    )
//   EVT_COMMAND_SCROLL( ID_GREEN1_SLIDER,  VOIStandardlizeFrame::OnGreen1Slider  )
//   EVT_COMMAND_SCROLL( ID_BLUE1_SLIDER,   VOIStandardlizeFrame::OnBlue1Slider   )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, VOIStandardlizeFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  VOIStandardlizeFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  VOIStandardlizeFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  VOIStandardlizeFrame::OnUpdateUIScale1Slider  )
//   EVT_UPDATE_UI( ID_RED1_SLIDER,    VOIStandardlizeFrame::OnUpdateUIRed1Slider    )
//   EVT_UPDATE_UI( ID_GREEN1_SLIDER,  VOIStandardlizeFrame::OnUpdateUIGreen1Slider  )
//   EVT_UPDATE_UI( ID_BLUE1_SLIDER,   VOIStandardlizeFrame::OnUpdateUIBlue1Slider   )

//  EVT_UPDATE_UI( ID_CENTER2_SLIDER, VOIStandardlizeFrame::OnUpdateUICenter2Slider )
//  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  VOIStandardlizeFrame::OnUpdateUIWidth2Sglider )
//   EVT_UPDATE_UI( ID_SLICE2_SLIDER,  VOIStandardlizeFrame::OnUpdateUISlice2Slider  )
//   EVT_UPDATE_UI( ID_SCALE2_SLIDER,  VOIStandardlizeFrame::OnUpdateUIScale2Slider  )
//   EVT_UPDATE_UI( ID_RED2_SLIDER,    VOIStandardlizeFrame::OnUpdateUIRed2Slider    )
//   EVT_UPDATE_UI( ID_GREEN2_SLIDER,  VOIStandardlizeFrame::OnUpdateUIGreen2Slider  )
//   EVT_UPDATE_UI( ID_BLUE2_SLIDER,   VOIStandardlizeFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    VOIStandardlizeFrame::OnUpdateUICineSlider    )
#endif

//  EVT_BUTTON( ID_ZOOM_IN,        VOIStandardlizeFrame::OnZoomIn                 )
//  EVT_BUTTON( ID_ZOOM_OUT,       VOIStandardlizeFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, VOIStandardlizeFrame::OnBusyTimer )
  EVT_CHAR( VOIStandardlizeFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   VOIStandardlizeFrame::OnOverlay )


  EVT_TIMER(          ID_CINE_TIMER,            VOIStandardlizeFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          VOIStandardlizeFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, VOIStandardlizeFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           VOIStandardlizeFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  VOIStandardlizeFrame::_fileHistory;
//======================================================================
