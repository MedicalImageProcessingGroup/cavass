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
 * \file   ROIStatisticalFrame.cpp
 * \brief  ROIStatisticalFrame class implementation.
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
#include  "SetLayout.h"

extern Vector  gFrameList;
extern int     gTimerInterval;


const char* ROINameTable[] = 
{
	"Type:Free",
	"Type:5x5",
	"Type:5x5x5",
	"Type:7x7",
	"Type:7x7x7",
	"Type:9x9",
	"Type:9x9x9"	
};

const char* GradNameTable[] = 
{
	"Grad:2D",
	"Grad:3D"	
};


//----------------------------------------------------------------------
/** \brief Constructor for ROIStatisticalFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
ROIStatisticalFrame::ROIStatisticalFrame (): MainFrame( 0 )
{
    mFileOrDataCount = 0;	
	
    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box       /*= mSetIndex2Box*/      = NULL;
    mSetIndex1Sizer     /*= mSetIndex2Sizer*/    = NULL;

    mCineControls       = NULL;
    /*mColor1Controls     = mColor2Controls    = NULL;*/
    mGrayMap1Controls   = mGrayMap2Controls  = NULL;
    mSaveScreenControls = NULL;
    mSetIndex1Controls  /*= mSetIndex2Controls*/ = NULL;  

	mWhichROI = 0; 
    mROIName = NULL;   
	mWhichGrad = 0; 
    mGradName = NULL;   

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

    mCanvas = new ROIStatisticalCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
	mLayoutControls = new LayoutControls( mControlPanel, mBottomSizer, "Layout", ID_CHECKBOX_LAYOUT );

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
    SetStatusText( "Select ROI",   2 );
    SetStatusText( "Display Statistics", 3 );
    SetStatusText( "Done", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void ROIStatisticalFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:ROIStatistical:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ROIStatisticalFrame::addButtonBox ( void ) {
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
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
	wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 4, col 1
	mWhichROI = 0;
	mROIName = new wxComboBox( mControlPanel, ID_ROIStatistical_ROI,
		"Select ROI Type", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		0, NULL, wxCB_READONLY );
	::setColor( mROIName );
	for (int i=0; i < 7; i++) 
	{
		mROIName->Append( ROINameTable[i] );
	}
	mROIName->SetSelection( 0 );
    fgs->Add( mROIName, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	
	//row 4, col 2
	mWhichGrad = 0;
	mGradName = new wxComboBox( mControlPanel, ID_ROIStatistical_GRAD,
		"Select GRAD Type", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		0, NULL, wxCB_READONLY );
	::setColor( mGradName );
	for (int i=0; i < 2; i++) 
	{
		mGradName->Append( GradNameTable[i] );
	}
	mGradName->SetSelection( 0 );
    fgs->Add( mGradName, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

	 //row 5, col 1
    wxButton*  quitBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( quitBut );
    fgs->Add( quitBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
   

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ROIStatisticalFrame class. */
ROIStatisticalFrame::~ROIStatisticalFrame ( void ) {
    cout << "ROIStatisticalFrame::~ROIStatisticalFrame" << endl;
    wxLogMessage( "ROIStatisticalFrame::~ROIStatisticalFrame" );

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
void ROIStatisticalFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "ROIStatisticalFrame::OnChar" );
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
        cout << "ROIStatisticalFrame::OnChar: " << ::gTimerInterval << endl;
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
void ROIStatisticalFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void ROIStatisticalFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
void ROIStatisticalFrame::OnHideControls ( wxCommandEvent& e ) {
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
void ROIStatisticalFrame::OnOpen ( wxCommandEvent& e ) {
    wxString  filename = wxFileSelector( _T("Select image file"), _T(""),
        _T(""), _T(""),
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
        wxFILE_MUST_EXIST );
    
    if (filename.Length()>0) {
        ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
        if (!canvas->isLoaded(0)) {
		    loadFile(filename.c_str());
        } else {
            ROIStatisticalFrame*  frame = new ROIStatisticalFrame();
            frame->loadFile(filename.c_str());
        }
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  two data files are required by overlay.
 */
void ROIStatisticalFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:ROIStatistical: ");
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
            tmp = wxString::Format( "CAVASS:ROIStatistical:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
void ROIStatisticalFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:ROIStatistical: ");
    tmp += name;
    SetTitle( tmp );
    
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnPrevious ( wxCommandEvent& unused ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;

    if (sliceA<0 )
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );
	canvas->setROIStatisticalDone(false);
    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnNext ( wxCommandEvent& unused ) 
{
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;

    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );
	canvas->setROIStatisticalDone(false);

    canvas->reload();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }

 
   if (mSetIndex1Controls!=NULL) return;

    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFilterIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0), canvas->getNoSlices(0), ID_SLICE1_SLIDER,
			     ID_SCALE1_SLIDER, canvas->getScale(), ID_OVERLAY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
 

    if (mGrayMap1Controls!=NULL)  return;

    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap1", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnCine ( wxCommandEvent& unused ) {
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }    
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }   

    if (mCineControls!=NULL)      return;
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
	canvas->setROIStatisticalDone(false);

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnReset ( wxCommandEvent& unused ) 
{
    if (mCineControls!=NULL)       { delete mCineControls;        mCineControls=NULL;       }
    if (mGrayMap1Controls!=NULL)   { delete mGrayMap1Controls;    mGrayMap1Controls=NULL;   }    
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }    

    m_cine_timer->Stop();

    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->m_tx = canvas->m_ty = 40;
    canvas->mCavassData->mDisplay = true;
	//canvas->mCavassData->mNext->mDisplay = true;

    canvas->setROIStatisticalDone(false);   

    canvas->setSliceNo( 0, 0 );
    canvas->setCenter(  0, canvas->getMax(0)/2 );
    canvas->setWidth(   0, canvas->getMax(0)   );
    canvas->setInvert(  0, false );
    canvas->setScale( 1.0 );

	canvas->RemoveProfileInfo();   

    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ROIStatisticalFrame::OnROIStatisticalSave ( wxCommandEvent &e ) 
{
    wxFileDialog  saveDlg( this, _T("Save file"), wxEmptyString,
                           _T("ROIStatistical.txt"), _T("text files (*.txt)|*.TXT"),
                           wxSAVE|wxOVERWRITE_PROMPT );

    saveDlg.SetFilterIndex(1);

    if (saveDlg.ShowModal() == wxID_OK) 
	{
        wxLogMessage( _T("%s, ROIStatistical %d"), (const char *)saveDlg.GetPath().c_str(), saveDlg.GetFilterIndex() );
    }
	else
		return;
	
	ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
	if( !canvas->getROIStatisticalDone() )
	{
		wxMessageBox( "You should draw and display a profile before save it." );
	}	
	else
	{		
		canvas->SaveProfile((unsigned char*)(const char *)saveDlg.GetPath().c_str());
	}  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for  ROIStatistical button */
void ROIStatisticalFrame::OnLayout ( wxCommandEvent& e ) 
{
    bool  value = e.IsChecked();
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->setLayout( value );   

	canvas->RemoveProfileInfo();
	canvas->setROIStatisticalDone(false);  

	canvas->reload();  
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for  ROIStatisticalType button */
void ROIStatisticalFrame::OnROIStatisticalROI ( wxCommandEvent& e ) 
{
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->setROIStatisticalDone( false );

	mWhichROI = mROIName->GetSelection();     
    canvas->setROIType(mWhichROI);
   
}

/** \brief callback for  ROIStatisticalType button */
void ROIStatisticalFrame::OnROIStatisticalGrad ( wxCommandEvent& e ) 
{
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->setROIStatisticalDone( false );

	mWhichGrad = mGradName->GetSelection();  
    canvas->setGradType(mWhichGrad);
   
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void ROIStatisticalFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ROIStatisticalFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ROIStatisticalFrame::OnCTLung ( wxCommandEvent& unused ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
void ROIStatisticalFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
void ROIStatisticalFrame::OnCTBone ( wxCommandEvent& unused ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
void ROIStatisticalFrame::OnPET ( wxCommandEvent& unused ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void ROIStatisticalFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
	canvas->setROIStatisticalDone(false);
    canvas->reload();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
void ROIStatisticalFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void ROIStatisticalFrame::OnOverlay ( wxCommandEvent& e ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void ROIStatisticalFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void ROIStatisticalFrame::OnCineForward ( wxCommandEvent& e ) {
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
void ROIStatisticalFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void ROIStatisticalFrame::OnCineTimer ( wxTimerEvent& e ) {
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
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
void ROIStatisticalFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void ROIStatisticalFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void ROIStatisticalFrame::OnBusyTimer ( wxTimerEvent& e ) {
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
void ROIStatisticalFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void ROIStatisticalFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void ROIStatisticalFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void ROIStatisticalFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void ROIStatisticalFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
/*
    wxPrintDialogData  printDialogData( *g_printData );
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new ROIStatisticalPrint("CAVASS", canvas),
        new ROIStatisticalPrint("CAVASS", canvas),
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
void ROIStatisticalFrame::OnPrint ( wxCommandEvent& e ) {
/*
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ROIStatisticalCanvas*  canvas = dynamic_cast<ROIStatisticalCanvas*>(mCanvas);
    ROIStatisticalPrint       printout( _T("CAVASS:ROIStatistical"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( ROIStatisticalFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ROIStatisticalFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
#if 0
  EVT_MENU( ID_ABOUT,          MainFrame::OnAbout           )
  EVT_MENU( ID_EXIT,           MainFrame::OnQuit            )
  EVT_MENU( ID_NEW,            MainFrame::OnNew             )
  EVT_MENU( ID_OPEN,           ROIStatisticalFrame::OnOpen         )
  EVT_MENU( ID_SAVE_SCREEN,    ROIStatisticalFrame::OnSaveScreen   )
  EVT_MENU( ID_CLOSE,          MainFrame::OnClose           )
  EVT_MENU( ID_COPY,           ROIStatisticalFrame::OnCopy         )
  EVT_MENU( ID_PREFERENCES,    MainFrame::OnPreferences     )
  EVT_MENU( ID_PAGE_SETUP,     MainFrame::OnPageSetup       )
  EVT_MENU( ID_PRINT_PREVIEW,  ROIStatisticalFrame::OnPrintPreview )
  EVT_MENU( ID_PRINT,          ROIStatisticalFrame::OnPrint        )
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, ROIStatisticalFrame::OnHideControls )
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU, MainFrame::OnWindow )

  EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, MainFrame::OnMRUFile )
#endif
  EVT_BUTTON( ID_PREVIOUS,       ROIStatisticalFrame::OnPrevious    )
  EVT_BUTTON( ID_NEXT,           ROIStatisticalFrame::OnNext        )
  EVT_BUTTON( ID_SET_INDEX1,     ROIStatisticalFrame::OnSetIndex1   )
//  EVT_BUTTON( ID_SET_INDEX2,     ROIStatisticalFrame::OnSetIndex2   )
  EVT_BUTTON( ID_GRAYMAP1,       ROIStatisticalFrame::OnGrayMap1    )
//  EVT_BUTTON( ID_ERASE  ,       ROIStatisticalFrame::OnErase    )
//  EVT_BUTTON( ID_COLOR1,         ROIStatisticalFrame::OnColor1      )
//  EVT_BUTTON( ID_COLOR2,         ROIStatisticalFrame::OnColor2      )
//  EVT_BUTTON( ID_MATCH_INDEX1,   ROIStatisticalFrame::OnMatchIndex1 )
//  EVT_BUTTON( ID_MATCH_INDEX2,   ROIStatisticalFrame::OnMatchIndex2 )
//  EVT_BUTTON( ID_BLINK1,         ROIStatisticalFrame::OnBlink1      )
//  EVT_BUTTON( ID_BLINK2,         ROIStatisticalFrame::OnBlink2      )
//  EVT_BUTTON( ID_LAYOUT,         ROIStatisticalFrame::OnLayout        )
  EVT_COMBOBOX( ID_ROIStatistical_ROI,    ROIStatisticalFrame::OnROIStatisticalROI    )  
  EVT_COMBOBOX( ID_ROIStatistical_GRAD,    ROIStatisticalFrame::OnROIStatisticalGrad    )  
  EVT_BUTTON( ID_CINE,           ROIStatisticalFrame::OnCine        )
  EVT_BUTTON( ID_RESET,          ROIStatisticalFrame::OnReset       )
  EVT_BUTTON( ID_SAVE,           ROIStatisticalFrame::OnROIStatisticalSave  )


  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, ROIStatisticalFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  ROIStatisticalFrame::OnWidth1Slider  )
  EVT_BUTTON( ID_CT_LUNG,          ROIStatisticalFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   ROIStatisticalFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          ROIStatisticalFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              ROIStatisticalFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  ROIStatisticalFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  ROIStatisticalFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        ROIStatisticalFrame::OnInvert1       )
  EVT_CHECKBOX(       ID_CHECKBOX_LAYOUT,       ROIStatisticalFrame::OnLayout       )
//   EVT_COMMAND_SCROLL( ID_RED1_SLIDER,    ROIStatisticalFrame::OnRed1Slider    )
//   EVT_COMMAND_SCROLL( ID_GREEN1_SLIDER,  ROIStatisticalFrame::OnGreen1Slider  )
//   EVT_COMMAND_SCROLL( ID_BLUE1_SLIDER,   ROIStatisticalFrame::OnBlue1Slider   )

//  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, ROIStatisticalFrame::OnCenter2Slider )
//  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  ROIStatisticalFrame::OnWidth2Slider  )
//   EVT_COMMAND_SCROLL( ID_SLICE2_SLIDER,  ROIStatisticalFrame::OnSlice2Slider  )
//   EVT_COMMAND_SCROLL( ID_SCALE2_SLIDER,  ROIStatisticalFrame::OnScale2Slider  )
//  EVT_CHECKBOX(       ID_INVERT2,        ROIStatisticalFrame::OnInvert2       )
//   EVT_COMMAND_SCROLL( ID_RED2_SLIDER,    ROIStatisticalFrame::OnRed2Slider    )
//   EVT_COMMAND_SCROLL( ID_GREEN2_SLIDER,  ROIStatisticalFrame::OnGreen2Slider  )
//   EVT_COMMAND_SCROLL( ID_BLUE2_SLIDER,   ROIStatisticalFrame::OnBlue2Slider   )
  //EVT_TEXT( ID_ROIStatistical_SIGMA,            ROIStatisticalFrame::OnROIStatisticalSigma )
  //EVT_TEXT( ID_ROIStatistical_HOMOGEN,            ROIStatisticalFrame::OnROIStatisticalHomogen )
  //EVT_TEXT( ID_ROIStatistical_ITERATIONS,       ROIStatisticalFrame::OnROIStatisticalScaleIterations )
  //EVT_TEXT( ID_ROIStatistical_ZETAFACTOR,       ROIStatisticalFrame::OnROIStatisticalZetaFactor )
  //EVT_TEXT( ID_ROIStatistical_INCLUFACTOR,       ROIStatisticalFrame::OnROIStatisticalIncluFactor )
  //EVT_TEXT( ID_ROIStatistical_STDDEVIA,       ROIStatisticalFrame::OnROIStatisticalStdDevia )
  //EVT_TEXT( ID_ROIStatistical_NUMOFREGION,       ROIStatisticalFrame::OnROIStatisticalNumofRegion )  

  //EVT_LIST_ITEM_SELECTED( ID_ROIStatistical_MORPH,            ROIStatisticalFrame::OnROIStatisticalMorph )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, ROIStatisticalFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  ROIStatisticalFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  ROIStatisticalFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  ROIStatisticalFrame::OnUpdateUIScale1Slider  )
//   EVT_UPDATE_UI( ID_RED1_SLIDER,    ROIStatisticalFrame::OnUpdateUIRed1Slider    )
//   EVT_UPDATE_UI( ID_GREEN1_SLIDER,  ROIStatisticalFrame::OnUpdateUIGreen1Slider  )
//   EVT_UPDATE_UI( ID_BLUE1_SLIDER,   ROIStatisticalFrame::OnUpdateUIBlue1Slider   )

//  EVT_UPDATE_UI( ID_CENTER2_SLIDER, ROIStatisticalFrame::OnUpdateUICenter2Slider )
//  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  ROIStatisticalFrame::OnUpdateUIWidth2Sglider )
//   EVT_UPDATE_UI( ID_SLICE2_SLIDER,  ROIStatisticalFrame::OnUpdateUISlice2Slider  )
//   EVT_UPDATE_UI( ID_SCALE2_SLIDER,  ROIStatisticalFrame::OnUpdateUIScale2Slider  )
//   EVT_UPDATE_UI( ID_RED2_SLIDER,    ROIStatisticalFrame::OnUpdateUIRed2Slider    )
//   EVT_UPDATE_UI( ID_GREEN2_SLIDER,  ROIStatisticalFrame::OnUpdateUIGreen2Slider  )
//   EVT_UPDATE_UI( ID_BLUE2_SLIDER,   ROIStatisticalFrame::OnUpdateUIBlue2Slider   )

  EVT_UPDATE_UI( ID_CINE_SLIDER,    ROIStatisticalFrame::OnUpdateUICineSlider    )
#endif

//  EVT_BUTTON( ID_ZOOM_IN,        ROIStatisticalFrame::OnZoomIn                 )
//  EVT_BUTTON( ID_ZOOM_OUT,       ROIStatisticalFrame::OnZoomOut                )
//  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                      )

  EVT_SIZE(  MainFrame::OnSize  )
//  EVT_MOVE(  MainFrame::OnMove  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, ROIStatisticalFrame::OnBusyTimer )
  EVT_CHAR( ROIStatisticalFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   ROIStatisticalFrame::OnOverlay )


  EVT_TIMER(          ID_CINE_TIMER,            ROIStatisticalFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          ROIStatisticalFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, ROIStatisticalFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           ROIStatisticalFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  ROIStatisticalFrame::_fileHistory;
//======================================================================
