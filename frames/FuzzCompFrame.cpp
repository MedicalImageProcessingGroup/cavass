/*
  Copyright 1993-2017, 2021 Medical Image Processing Group
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
 * \file   FuzzCompFrame.cpp
 * \brief  FuzzCompFrame class implementation.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "FuzzCompFrame.h"
#include  "FuzzCompCanvas.h"
#include  "ClassifyControls.h"
#include  "CineControls.h"
#include  "CovarianceControls.h"
#include  "FunctionControls.h"
#include  "HistoSettingControls.h"
#include  "ParameterControls.h"
#include  "SetFuzzCompIndexControls.h"


extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for FuzzCompFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */


FuzzCompFrame::FuzzCompFrame ( bool maximize, int w, int h ) : MainFrame( 0 )
{
    mFileOrDataCount = 0;

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box         = NULL;
    mSetIndex1Sizer       = NULL;
    mClassifyBox      = NULL;
    mClassifySizer    = NULL;

    mCineControls         = NULL;
    mGrayMap1Controls     = mGrayMap2Controls  = NULL;
    mGrayMap3Controls     = mGrayMap4Controls  = NULL;
    mSaveScreenControls   = NULL;
    mSetIndex1Controls    = NULL;
    mHistoSettingControls = NULL;
    mCovarianceControls   = NULL;
    mParameterControls    = NULL;
    mFunctionControls     = NULL;
    mClassifyControls     = NULL;
	mTmpName.Empty();


    ::gFrameList.push_back( this );
    gWhere = 20;

    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 0.0 );
    ::setColor( mSplitter );
   mSplitter->SetSashPosition( -dControlsHeight-70 );
    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -

    mCanvas = new FuzzCompCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
	/*----Classify setting controls*/
    mClassifyControls = new ClassifyControls( mControlPanel, mBottomSizer,
        "FuzzComp Setting",ID_MODE_COMBOBOX,ID_ADJ_COMBOBOX,
        ID_AFFIN_COMBOBOX, ID_BRUSH_SLIDER, ID_THRES_SLIDER,ID_PARALLEL_INDEX );
   	//------------------------------------------
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
	else             SetSize( w, h );
    Show();
    Raise();
    mSplitter->SetSashPosition( -dControlsHeight-70);

#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Paint",  2 );
    SetStatusText( "Erase", 3 );
    SetStatusText( "", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
	mSplitter->SetSashPosition( -dControlsHeight-70 );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void FuzzCompFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:Register:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 20, wxGROW );  //spacer
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
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
    wxButton*  grayMap1 = new wxButton( mControlPanel, ID_GRAYMAP1, "GMapOrig", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap1 );
    fgs->Add( grayMap1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "GMapFeat", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    wxButton*  grayMap3 = new wxButton( mControlPanel, ID_GRAYMAP3, "GMapAffn", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap3 );
    fgs->Add( grayMap3, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2

    mApplyTrain = new wxButton( mControlPanel, ID_APPLYTRAIN, "ApplyTrain", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mApplyTrain->Enable( true);
    ::setColor( mApplyTrain );
    fgs->Add( mApplyTrain, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

	  //row 5, col 1
    mClearSeed = new wxButton( mControlPanel, ID_CLEARSEED, "ClearSeed", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mClearSeed->Enable( false);
    ::setColor( mClearSeed );
    fgs->Add( mClearSeed, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 5, col 2
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 1    
    mSave = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mSave );
    fgs->Add( mSave, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 2


    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 20 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for FuzzCompFrame class. */
FuzzCompFrame::~FuzzCompFrame ( void ) {
    cout << "FuzzCompFrame::~FuzzCompFrame" << endl;
    wxLogMessage( "FuzzCompFrame::~FuzzCompFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }    
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }
 /*   if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
	if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }*/

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
/** \brief This method should be called whenever one wishes to create a
 *  new FuzzCompFrame.  It first searches the input file history for an
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
void FuzzCompFrame::createFuzzCompFrame ( wxFrame* parentFrame, bool useHistory )
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
                wxString  tmp = wxString::Format( "File %s could not be opened.",
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
            "CAVASS files (*.IM0)|*.IM0",
            wxFILE_MUST_EXIST );
    }

    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    //display an example frame using the specified file as input
    FuzzCompFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new FuzzCompFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new FuzzCompFrame();
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
 *  Supported file extensions: BIM and IM0.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool FuzzCompFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM", fn, false ))    return true;
    if (wxMatchWild( "*.IM0", fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void FuzzCompFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "FuzzCompFrame::OnChar" );
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
        cout << "FuzzCompFrame::OnChar: " << ::gTimerInterval << endl;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void FuzzCompFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
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
void FuzzCompFrame::OnHideControls ( wxCommandEvent& e ) {
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
void FuzzCompFrame::OnOpen ( wxCommandEvent& e ) {
	createFuzzCompFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void FuzzCompFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "FuzzCompFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "FuzzCompFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnPPScopsFuzzComp( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  one data files are required by FuzzComp.
 */
void FuzzCompFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:FuzzComp: ");
    ++mFileOrDataCount;
    if (mFileOrDataCount==1) {
        tmp += fname;
        mSourceName = fname;
    } else {
        assert( 0 );
    }
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "CAVASS:FuzzComp:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

	 //------ parameter affinity controls
	mParameterControls = new ParameterControls( mControlPanel, mBottomSizer,
          "Parameter Setting", 0, 0, 0, 0, 0,
		  1, canvas->getNoSlices(0), canvas->getNoSlices(0),
          ID_HIGH, ID_LOW, ID_DIFF, ID_SUM, ID_RELDIFF,ID_HIGH_COMBO, ID_LOW_COMBO,
          ID_DIFF_COMBO, ID_SUM_COMBO, ID_RELDIFF_COMBO, ID_AFFOUT_COMBO, ID_STARTSLICE_SLIDER, ID_ENDSLICE_SLIDER );
    //----------------------------------------------
    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void FuzzCompFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:FuzzComp: ");
    tmp += name;
    SetTitle( tmp );
    
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnPrevious ( wxCommandEvent& unused ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;    
    if (sliceA<0)
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );    
	
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->FuzzyComp_update();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnNext ( wxCommandEvent& unused ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;
    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );

	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->FuzzyComp_update();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }
    if (mSetIndex1Controls!=NULL) return;

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFuzzCompIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
        ID_SCALE1_SLIDER, canvas->getScale());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }
    if (mGrayMap1Controls!=NULL)  return;

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map (input)", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    //if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    //if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }
    //if (mParameterControls!=NULL)  { delete mParameterControls;   mParameterControls=NULL;  }
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }
    if (mGrayMap2Controls!=NULL)  return;

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map (feature)", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnGrayMap3 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    //if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    //if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }
    //if (mParameterControls!=NULL)  { delete mParameterControls;   mParameterControls=NULL;  }
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }
    if (mGrayMap3Controls!=NULL)  return;

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    mGrayMap3Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map (affinity/connectivity)", canvas->getCenter(2), canvas->getWidth(2),
        canvas->getMax(2), canvas->getInvert(2),
        ID_CENTER3_SLIDER, ID_WIDTH3_SLIDER, ID_INVERT3,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnGrayMap4 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    //if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    //if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }
    //if (mParameterControls!=NULL)  { delete mParameterControls;   mParameterControls=NULL;  }
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }
    if (mGrayMap4Controls!=NULL)  return;

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    mGrayMap4Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GMapcon", canvas->getCenter(2), canvas->getWidth(2),
        canvas->getMax(2), canvas->getInvert(2),
        ID_CENTER4_SLIDER, ID_WIDTH4_SLIDER, ID_INVERT4,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnCine ( wxCommandEvent& unused ) {
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    //if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    //if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }
    //if (mParameterControls!=NULL)  { delete mParameterControls;   mParameterControls=NULL;  }
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }
    if (mCineControls!=NULL)      return;

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnReset ( wxCommandEvent& unused ) {

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);

    m_cine_timer->Stop();
	
	
    canvas->m_tx = canvas->m_ty = 0;
    canvas->mCavassData->mDisplay = true;
	canvas->ResetTraining();
	canvas->ResetSeed();
	mClearSeed->Enable( false);
	
	canvas->connectivity_data_valid = FALSE;		 
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom in button
 *  \todo implement multi resolution sliders
 */
void FuzzCompFrame::OnZoomIn ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom out button
 *  \todo implement multi resolution sliders
 */
void FuzzCompFrame::OnZoomOut ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void FuzzCompFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 2 */
void FuzzCompFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 3 */
void FuzzCompFrame::OnCenter3Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getCenter(2)==e.GetPosition())    return;  //no change
    canvas->setCenter( 2, e.GetPosition() );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 4 */
void FuzzCompFrame::OnCenter4Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getCenter(2)==e.GetPosition())    return;  //no change
    canvas->setCenter( 2, e.GetPosition() );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void FuzzCompFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 2 */
void FuzzCompFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 3 */
void FuzzCompFrame::OnWidth3Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getWidth(2)==e.GetPosition())    return;  //no change
    canvas->setWidth( 2, e.GetPosition() );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 4 */
void FuzzCompFrame::OnWidth4Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getWidth(2)==e.GetPosition())    return;  //no change
    canvas->setWidth( 2, e.GetPosition() );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void FuzzCompFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->FuzzyComp_update();
}

/** \brief callback for scale slider for data set 1 */
void FuzzCompFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


/** \brief callback for Threshold level slider */
void FuzzCompFrame::OnThresSlider ( wxScrollEvent& e ) 
{
    int     newThres = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newThres );
    mClassifyControls->setThresText( s );

	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	canvas->threshold = newThres;
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for Brush size slider */
void FuzzCompFrame::OnBrushSlider ( wxScrollEvent& e ) 
{
    int     newBrush = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newBrush );
    mClassifyControls->setBrushText( s );
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setBrushSize( newBrush );    
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

/** \brief callback for Overlay checkbox for both data sets */
void FuzzCompFrame::OnOverlay ( wxCommandEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void FuzzCompFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 2 */
void FuzzCompFrame::OnInvert2 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 3 */
void FuzzCompFrame::OnInvert3 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setInvert( 2, value );
    canvas->initLUT( 2 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 4 */
void FuzzCompFrame::OnInvert4 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setInvert( 2, value );
    canvas->initLUT( 2 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void FuzzCompFrame::OnCineForward ( wxCommandEvent& e ) {
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
void FuzzCompFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void FuzzCompFrame::OnCineTimer ( wxTimerEvent& e ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
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
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->FuzzyComp_update();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void FuzzCompFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void FuzzCompFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void FuzzCompFrame::OnBusyTimer ( wxTimerEvent& e ) {
    cout << "OnBusyTimer" << endl;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void FuzzCompFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void FuzzCompFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void FuzzCompFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    mCanvas->FuzzyComp_update();
	if (mSetIndex1Controls!=NULL)
	    mSetIndex1Controls->setSliceNo( m_sliceNo->GetValue() );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void FuzzCompFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void FuzzCompFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
    /*wxPrintDialogData  printDialogData( *g_printData );
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new OverlayPrint("CAVASS", canvas),
        new OverlayPrint("CAVASS", canvas),
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
    frame->Show();*/
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing */
void FuzzCompFrame::OnPrint ( wxCommandEvent& e ) {
    /*wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    OverlayPrint       printout( _T("CAVASS:Register"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for affinity selection
*  \todo Histogram and Covariance callbacks need to be implemented
*/
void FuzzCompFrame::OnAffinity ( wxCommandEvent& unused ) {

	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    enum {HISTOGRAM, COVARIANCE, PARAMETER};

    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL;  }
    if (mFunctionControls!=NULL)  { delete mFunctionControls;   mFunctionControls=NULL;  }

    int choice = mClassifyControls->getAffinity ();
    wxArrayString choices3;
    choices3.Add(wxT("Gaussian" ));
    choices3.Add(wxT("Ramp"));
    choices3.Add(wxT("Box"));
    switch (choice) {
      case HISTOGRAM:
	      if (mHistoSettingControls!=NULL)  { break;  }
		  if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL; }
		  if (mParameterControls!=NULL)  { delete mParameterControls;   mParameterControls=NULL; }
          mHistoSettingControls = new HistoSettingControls( mControlPanel, mBottomSizer,
          "Histogram Setting", canvas->getCenter(0), canvas->getWidth(0), canvas->getMax(0),
		  0, canvas->getNoSlices(0)-1,canvas->getNoSlices(0)-1,
		  ID_LOWBIN_SLIDER, ID_HIGHBIN_SLIDER, ID_AFFOUT_COMBO, ID_STARTSLICE_SLIDER, ID_ENDSLICE_SLIDER );
          break;

       case COVARIANCE:
	       if (mCovarianceControls!=NULL)  { break; }
		   if (mParameterControls!=NULL)  { delete mParameterControls;   mParameterControls=NULL; }
		   if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL; }
           mCovarianceControls = new CovarianceControls( mControlPanel, mBottomSizer,
          "Covariance Setting", 0, 0, 0, 0, 0,
		  0, canvas->getNoSlices(0)-1,canvas->getNoSlices(0)-1,
          ID_HIGH, ID_LOW, ID_DIFF, ID_SUM, ID_RELDIFF, ID_AFFOUT_COMBO, ID_STARTSLICE_SLIDER, ID_ENDSLICE_SLIDER );
          break;
       case PARAMETER:
	       if (mParameterControls!=NULL)  { break; }
		   if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL; }
		   if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL; }
           mParameterControls = new ParameterControls( mControlPanel, mBottomSizer,
          "Parameter Setting", canvas->feature_status[0]!=0,
		  canvas->feature_status[1]!=0, canvas->feature_status[2]!=0,
		  canvas->feature_status[3]!=0, canvas->feature_status[4]!=0,
		  0, canvas->getNoSlices(0)-1,canvas->getNoSlices(0)-1,
          ID_HIGH, ID_LOW, ID_DIFF, ID_SUM, ID_RELDIFF,ID_HIGH_COMBO, ID_LOW_COMBO,
          ID_DIFF_COMBO, ID_SUM_COMBO, ID_RELDIFF_COMBO, ID_AFFOUT_COMBO, ID_STARTSLICE_SLIDER, ID_ENDSLICE_SLIDER );
		  mParameterControls->setHighComboSelection(choices3[canvas->function_selected[0]]);
		  mParameterControls->setLowComboSelection(choices3[canvas->function_selected[1]]);
		  mParameterControls->setDiffComboSelection(choices3[canvas->function_selected[2]]);
		  mParameterControls->setSumComboSelection(choices3[canvas->function_selected[3]]);
		  mParameterControls->setRelDiffComboSelection(choices3[canvas->function_selected[4]]);
          break;

       default:
          break;
    }
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->reload();
}     

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnStartSliceSlider ( wxScrollEvent& e ) 
{
  FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
  canvas->reload();
}

void FuzzCompFrame::OnEndSliceSlider ( wxScrollEvent& e ) 
{
  FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
  canvas->reload();
}


void FuzzCompFrame::OnHigh ( wxCommandEvent& e ) {
    int choice = mClassifyControls->getAffinity();
    if (choice < 0) choice=2;
    if (choice < 2) return;
	
	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);

    bool  value = e.IsChecked();
    mParameterControls->enableHighCombo (value);
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
	if (value)
	{
      if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
	  if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	  if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
      delete mFunctionControls;
	  canvas->feature_selected = 0;
      mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
          "Function Setting (high)", canvas->getWeight(0), canvas->getLevel(0),
		  canvas->getWidthLevel(0), canvas->getMax(0),
          ID_WEIGHT1_SLIDER, ID_LEVEL1_SLIDER, ID_WIDTHLEVEL1_SLIDER);
	}

	canvas->feature_status[0] = value;
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnLow ( wxCommandEvent& e ) {
    
	int choice = mClassifyControls->getAffinity();
    if (choice < 0) choice=2;
    if (choice < 2) return;
	
	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);

    bool  value = e.IsChecked();
    mParameterControls->enableLowCombo (value);
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
	if (value)
	{
      if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
	  if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	  if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
      delete mFunctionControls;
	  canvas->feature_selected = 1;
      mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
          "Function Setting (low)", canvas->getWeight(1), canvas->getLevel(1),
		  canvas->getWidthLevel(1), canvas->getMax(0),
          ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER);
    }     

	canvas->feature_status[1] = value;
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnDiff ( wxCommandEvent& e ) {

    int choice = mClassifyControls->getAffinity();
    if (choice < 0) choice=2;
    if (choice < 2) return;
	
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);

    bool  value = e.IsChecked();
    mParameterControls->enableDiffCombo (value);
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
	if (value)
	{
      if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
	  if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	  if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
      delete mFunctionControls;
      canvas->feature_selected = 2;
      mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
          "Function Setting (difference)", canvas->getWeight(2),
		  canvas->getLevel(2), canvas->getWidthLevel(2), canvas->getMax(0),
          ID_WEIGHT3_SLIDER, ID_LEVEL3_SLIDER, ID_WIDTHLEVEL3_SLIDER);

    }     
    canvas->feature_status[2] = value;
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnSum ( wxCommandEvent& e ) {

    int choice = mClassifyControls->getAffinity();
    if (choice < 0) choice=2;
    if (choice < 2) return;

	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);

    bool  value = e.IsChecked();
    mParameterControls->enableSumCombo (value);
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
	if (value)
	{
      if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
	  if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	  if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
      delete mFunctionControls;
      canvas->feature_selected = 3;
      mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
          "Function Setting (sum)", canvas->getWeight(3), canvas->getLevel(3),
		  canvas->getWidthLevel(3), 2*canvas->getMax(0),
          ID_WEIGHT4_SLIDER, ID_LEVEL4_SLIDER, ID_WIDTHLEVEL4_SLIDER);
    }     

	canvas->feature_status[3] = value;
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnRelDiff ( wxCommandEvent& e ) {

	int choice = mClassifyControls->getAffinity();
    if (choice < 0) choice=2;
    if (choice < 2) return;

	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);

    bool  value = e.IsChecked();
    mParameterControls->enableRelDiffCombo (value);
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
	if (value)
	{
      if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
	  if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
	  if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
      delete mFunctionControls;
      canvas->feature_selected = 4;
      mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
          "Function Setting (rel. diff.)", canvas->getWeight(4),
		  canvas->getLevel(4), canvas->getWidthLevel(4),
          100, ID_WEIGHT5_SLIDER, ID_LEVEL5_SLIDER, ID_WIDTHLEVEL5_SLIDER);
    }     

	canvas->feature_status[4] = value;
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}        

void FuzzCompFrame::OnPararell ( wxCommandEvent& e )
{
	   bool  value = e.IsChecked();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    
    canvas->setPararell(value);  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FuzzCompFrame::OnHighCombo ( wxCommandEvent& unused ) {

    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	if (canvas->feature_selected != 0)
	{
		delete mFunctionControls;
	    mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
	        "Function Setting (high)", canvas->getWeight(0),
			canvas->getLevel(0), canvas->getWidthLevel(0), canvas->getMax(0),
	        ID_WEIGHT1_SLIDER, ID_LEVEL1_SLIDER, ID_WIDTHLEVEL1_SLIDER);
	    canvas->feature_selected = 0;
	}
	canvas->function_selected[0] = mParameterControls->getHighComboSelection();
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
    canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}     


void FuzzCompFrame::OnLowCombo ( wxCommandEvent& unused ) {

    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	if (canvas->feature_selected != 1)
	{
		delete mFunctionControls;
	    mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
	        "Function Setting (low)", canvas->getWeight(1),
			canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
	        ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER);
	    canvas->feature_selected = 1;
	}
	canvas->function_selected[1] = mParameterControls->getLowComboSelection();
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}     


void FuzzCompFrame::OnDiffCombo ( wxCommandEvent& unused ) {

    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	if (canvas->feature_selected != 2)
	{
		delete mFunctionControls;
	    mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
	        "Function Setting (difference)", canvas->getWeight(2),
			canvas->getLevel(2), canvas->getWidthLevel(2), canvas->getMax(0),
	        ID_WEIGHT3_SLIDER, ID_LEVEL3_SLIDER, ID_WIDTHLEVEL3_SLIDER);
		canvas->feature_selected = 2;
	}
	canvas->function_selected[2] = mParameterControls->getDiffComboSelection();
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
    canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}     


void FuzzCompFrame::OnSumCombo ( wxCommandEvent& unused ) {

    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }

    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	if (canvas->feature_selected != 3)
	{
		delete mFunctionControls;
	    mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
           "Function Setting (sum)", canvas->getWeight(3), canvas->getLevel(3),
		   canvas->getWidthLevel(3), 2*canvas->getMax(0),
	       ID_WEIGHT4_SLIDER, ID_LEVEL4_SLIDER, ID_WIDTHLEVEL4_SLIDER);
		canvas->feature_selected = 3;
	}
	canvas->function_selected[3] = mParameterControls->getSumComboSelection();
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}     


void FuzzCompFrame::OnRelDiffCombo ( wxCommandEvent& unused ) {

    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL)  { delete mSetIndex1Controls;   mSetIndex1Controls=NULL;  }
    if (mHistoSettingControls!=NULL)  { delete mHistoSettingControls;   mHistoSettingControls=NULL;  }
    if (mCovarianceControls!=NULL)  { delete mCovarianceControls;   mCovarianceControls=NULL;  }
    
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	if (canvas->feature_selected != 4)
	{
	    delete mFunctionControls;
	    mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
            "Function Setting (rel. diff.)", canvas->getWeight(4),
		    canvas->getLevel(4), canvas->getWidthLevel(4),
            100, ID_WEIGHT5_SLIDER, ID_LEVEL5_SLIDER, ID_WIDTHLEVEL5_SLIDER);
		canvas->feature_selected = 4;
	}
	canvas->function_selected[4] =
		    mParameterControls->getRelDiffComboSelection();
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->initLUT(1);
	canvas->FuzzyComp_update();
}     


/** \brief callback for weight slider*/
void FuzzCompFrame::OnWeight1Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWeight( 0, newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWeight2Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWeight( 1, newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWeight3Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWeight( 2, newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWeight4Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWeight( 3, newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWeight5Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWeight( 4, newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for level slider*/
void FuzzCompFrame::OnLevel1Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setLevel( 0, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnLevel2Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setLevel( 1, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnLevel3Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setLevel( 2, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnLevel4Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setLevel( 3, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnLevel5Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setLevel( 4, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for widthLevel slider*/
void FuzzCompFrame::OnWidthLevel1Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWidthLevel( 0, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWidthLevel2Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWidthLevel( 1, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWidthLevel3Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWidthLevel( 2, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWidthLevel4Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWidthLevel( 3, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnWidthLevel5Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    canvas->setWidthLevel( 4, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->FuzzyComp_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void FuzzCompFrame::OnCTLung ( wxCommandEvent& unused ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
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
void FuzzCompFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
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
void FuzzCompFrame::OnCTBone ( wxCommandEvent& unused ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
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
void FuzzCompFrame::OnPET ( wxCommandEvent& unused ) {
    FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
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

/** \brief callback for process
 * \todo parameters computation from training mask/painting need to be implemented
 */
void FuzzCompFrame::OnProcess ( wxCommandEvent& e ) {

  
  FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
  //canvas->RunFuzzComp();
  
  wxString s1, s2;
  

  s1 =  wxString::Format( "%d", canvas->getNoSlices(0)-1);
  s2 =  wxString::Format( "%d", mClassifyControls->getThresLevel());


  enum {HISTOGRAM, COVARIANCE, PARAMETER};
  
  wxString s3, s5;
  s5.Empty();
  int choice1 = mClassifyControls->getAffinity();
  if (choice1 < 0) choice1 = 2;
  switch (choice1) {
    case HISTOGRAM:
       s3=wxT("-hist");
//@@
       break;
    case COVARIANCE:
       s3=wxT("-covariance");
//@@
       break;

    case PARAMETER:
       s3=wxT("-feature");
       if( mParameterControls->getOnHigh() )
	     s5 += s3 + wxString::Format( " 0 %d %1.2f %d %d ",
		   mParameterControls->getHighComboSelection()>0?
		   mParameterControls->getHighComboSelection():0,
		   (float)canvas->getWeight(0)/100, canvas->getLevel(0),
		   canvas->getWidthLevel(0));
	   if( mParameterControls->getOnLow() )
	     s5 += s3 + wxString::Format( " 1 %d %1.2f %d %d ",
		   mParameterControls->getLowComboSelection()>0?
		   mParameterControls->getLowComboSelection():0,
		   (float)canvas->getWeight(1)/100, canvas->getLevel(1),
		   canvas->getWidthLevel(1));
	   if( mParameterControls->getOnDiff() )
	     s5 += s3 + wxString::Format( " 2 %d %1.2f %d %d ",
		   mParameterControls->getDiffComboSelection()>0?
		   mParameterControls->getDiffComboSelection():0,
		   (float)canvas->getWeight(2)/100, canvas->getLevel(2),
		   canvas->getWidthLevel(2));
	   if( mParameterControls->getOnSum() )
	     s5 += s3 + wxString::Format( " 3 %d %1.2f %d %d ",
		   mParameterControls->getSumComboSelection()>0?
		   mParameterControls->getSumComboSelection():0,
		   (float)canvas->getWeight(3)/100, canvas->getLevel(3),
		   canvas->getWidthLevel(3));
	   if( mParameterControls->getOnRelDiff() )
	     s5 += s3 + wxString::Format( " 4 %d %1.2f %d %d ",
		   mParameterControls->getRelDiffComboSelection()>0?
		   mParameterControls->getRelDiffComboSelection():0,
		   (float)canvas->getWeight(4)/100, canvas->getLevel(4),
		   canvas->getWidthLevel(4));
       break;
  }

  enum {HARD, FUZZY};
  int choice2 = mClassifyControls->getAdjacency();
  if (choice2 < 0) choice2 = 0;
  switch (choice2) {
      case HARD:
		 break;
      case FUZZY:
         s5 += wxString::Format( " -fuzzy_adjacency");
         break;
  }

  enum {X, Y, Z, XY, XYZ};

  bool IsSeed=false;
  int start_slice, end_slice;
  if (mTmpName.IsEmpty())
  {
	start_slice = end_slice = canvas->getSliceNo(0);
	mTmpName =
	  Preferences::getOutputDirectory() + wxString("/FUZZ-TMP-OUT.IM0");
  }
  else
  {
	start_slice = mParameterControls->getStartSlice()-1;
	end_slice = mParameterControls->getEndSlice()-1;
  }

  for (int i=0; i<canvas->nSeed ; i++)
		if (canvas->mData[i][2]>=start_slice && canvas->mData[i][2]<=end_slice)
		{
			IsSeed = true;
			break;
		}
  int choice3 = mParameterControls->getAffOutSelection ();
  if (!IsSeed)
	  switch (choice3) {
        case X:
         s5 += wxString::Format( " -out_affinity x ");
         break;
        case Y:
         s5 += wxString::Format( " -out_affinity y ");
         break;
        case Z:
         s5 += wxString::Format( " -out_affinity z ");
         break;
        case XY:
         s5 += wxString::Format( " -out_affinity xy ");
         break;
        case XYZ:
         s5 += wxString::Format( " -out_affinity xyz ");
         break;
	    default:
         s5 += wxString::Format( " -out_affinity xy ");
         break;
	  }
  
  wxString s6, s7;

  s6 = wxString::Format( "%d", start_slice );
  s7 = wxString::Format( "%d", end_slice );

  SetStatusText("processing ...", 1);

  //construct the command string
  wxString  cmd = wxString::Format(
      "\"%s/fuzz_track_3d\" \"%s\" 1 %s %s 1 \"%s\" %s 0 %s ",
	  (const char *)Preferences::getHome().c_str(),
	  (const char *)mSourceName.c_str(), (const char *)s6.c_str(),
	  (const char *)s7.c_str(), (const char *)mTmpName.c_str(),
	  (const char *)s2.c_str(), (const char *)s5.c_str() );

  for (int i=0; i<canvas->nSeed ; i++) //seed points indicated by user
	if (canvas->mData[i][2]>=start_slice && canvas->mData[i][2]<=end_slice)
	  cmd += wxString::Format( "%d %d %d " ,
	    canvas->mData[i][0], canvas->mData[i][1], canvas->mData[i][2]);

  wxLogMessage( "command=%s", (const char *)cmd.c_str() );
  cout << cmd << endl;
  fflush(stdout);

  ProcessManager  p("fuzz_track_3d processing...", (const char *)cmd.c_str() );

  mTmpName.Empty();
  SetStatusText("done", 1);

}


void FuzzCompFrame::OnSeed ( ) 
{
	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
    mClearSeed->Enable( True);
	canvas->connectivity_data_valid = FALSE;
	canvas->FuzzyComp_update();
}

void FuzzCompFrame::OffSave ( ) 
{
    mSave->Enable( false );
}


/** \brief callback for save */
void FuzzCompFrame::OnMode ( wxCommandEvent& e )
{
enum {TRACKING, TRAINING};
FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
  int choice = mClassifyControls->getMode ();
  if (choice == TRAINING)
  {    
	mApplyTrain->Enable( true);
    canvas->setTraining( true );
	SetStatusText( "Paint",  2 );
    SetStatusText( "Erase", 3 );
    SetStatusText( "", 4 );
  } 
  else 
  {
	mApplyTrain->Enable( false);
	canvas->setTraining( false); 
	SetStatusText( "Seed",  2 );
    SetStatusText( "", 3 );
    SetStatusText( "Move", 4 );
  }	

  Refresh();
  canvas->FuzzyComp_update();
}

void FuzzCompFrame::OnAdjacency( wxCommandEvent& e ) 
{
	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	canvas->fuzzy_adjacency_flag = mClassifyControls->getAdjacency();
}


/** \brief callback for ClearSeed */
void FuzzCompFrame::OnClearSeed( wxCommandEvent& e ) 
{
	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	canvas->ResetSeed();
	canvas->FuzzyComp_update();
	mClearSeed->Enable( false);

}

/** \brief callback for ClearSeed */
void FuzzCompFrame::OnApplyTrain( wxCommandEvent& e ) 
{
	FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
	canvas->ApplyTraining();
//	if (mClassifyControls->getAffinity() == 2)
	{
	    delete mGrayMap2Controls;
		mGrayMap2Controls=NULL;
	    delete mGrayMap3Controls;
		mGrayMap3Controls=NULL;
	    delete mGrayMap4Controls;
		mGrayMap4Controls=NULL;
		delete mCineControls;
		mCineControls=NULL;
		delete mGrayMap1Controls;
		mGrayMap1Controls=NULL;
		delete mSetIndex1Controls;
		mSetIndex1Controls=NULL;
        delete mFunctionControls;
		switch (canvas->feature_selected)
		{
		  case 0:
            mFunctionControls= new FunctionControls(mControlPanel,mBottomSizer,
              "Function Setting (high)", canvas->getWeight(0),
			  canvas->getLevel(0), canvas->getWidthLevel(0), canvas->getMax(0),
              ID_WEIGHT1_SLIDER, ID_LEVEL1_SLIDER, ID_WIDTHLEVEL1_SLIDER);
		    break;
		  case 1:
            mFunctionControls= new FunctionControls(mControlPanel,mBottomSizer,
              "Function Setting (low)", canvas->getWeight(1),
			  canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
              ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER);
		    break;
		  case 2:
            mFunctionControls= new FunctionControls(mControlPanel,mBottomSizer,
              "Function Setting (difference)", canvas->getWeight(2),
		      canvas->getLevel(2), canvas->getWidthLevel(2), canvas->getMax(0),
              ID_WEIGHT3_SLIDER, ID_LEVEL3_SLIDER, ID_WIDTHLEVEL3_SLIDER);
		    break;
		  case 3:
            mFunctionControls= new FunctionControls(mControlPanel,mBottomSizer,
              "Function Setting (sum)", canvas->getWeight(3),
			  canvas->getLevel(3),canvas->getWidthLevel(3),2*canvas->getMax(0),
              ID_WEIGHT4_SLIDER, ID_LEVEL4_SLIDER, ID_WIDTHLEVEL4_SLIDER);
		    break;
		  case 4:
            mFunctionControls= new FunctionControls(mControlPanel,mBottomSizer,
              "Function Setting (rel. diff.)", canvas->getWeight(4),
		      canvas->getLevel(4), canvas->getWidthLevel(4), 100,
              ID_WEIGHT5_SLIDER, ID_LEVEL5_SLIDER, ID_WIDTHLEVEL5_SLIDER);
		    break;
		  default:
		    assert(0);
		    break;
		}
	}
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for save */
void FuzzCompFrame::OnSave ( wxCommandEvent& e ) {
  FuzzCompCanvas*  canvas = dynamic_cast<FuzzCompCanvas*>(mCanvas);
  if (canvas->IsSeed()) {
     wxFileDialog  f( this, "Select connectivity image file", _T(""),
	      _T("fuzcon-tmp.IM0"), 
          "CAVASS files (*.IM0)|*.IM0",
          wxSAVE | wxOVERWRITE_PROMPT);
     int  ret = f.ShowModal();
     if (ret == wxID_CANCEL)    return;
     wxArrayString  names;
     f.GetPaths( names );
     if (names.Count()==0)    return;
     mTmpName = names[0];
  } else {
  
     wxFileDialog  f( this, "Select affinity image file", _T(""),
	      _T("fuzcon-tmp.IM0"),
          "CAVASS files (*.IM0)|*.IM0",
          wxSAVE | wxOVERWRITE_PROMPT);
     int  ret = f.ShowModal();
     if (ret == wxID_CANCEL)    return;
     wxArrayString  names;
     f.GetPaths( names );
     if (names.Count()==0)    return;
     mTmpName = names[0];
  }
  OnProcess(e);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( FuzzCompFrame, wxFrame )
BEGIN_EVENT_TABLE       ( FuzzCompFrame, wxFrame )
DefineStandardFrameCallbacks

  EVT_BUTTON( ID_PREVIOUS,       FuzzCompFrame::OnPrevious       )
  EVT_BUTTON( ID_NEXT,           FuzzCompFrame::OnNext           )
  EVT_BUTTON( ID_SET_INDEX1,     FuzzCompFrame::OnSetIndex1      )
  EVT_BUTTON( ID_GRAYMAP1,       FuzzCompFrame::OnGrayMap1       )
  EVT_BUTTON( ID_GRAYMAP2,       FuzzCompFrame::OnGrayMap2       )
  EVT_BUTTON( ID_GRAYMAP3,       FuzzCompFrame::OnGrayMap3       )
  EVT_BUTTON( ID_GRAYMAP4,       FuzzCompFrame::OnGrayMap4       )
  EVT_BUTTON( ID_CINE,           FuzzCompFrame::OnCine           )
  EVT_BUTTON( ID_RESET,          FuzzCompFrame::OnReset          )
  EVT_BUTTON( ID_SAVE,           FuzzCompFrame::OnSave           )
  EVT_BUTTON( ID_CLEARSEED,      FuzzCompFrame::OnClearSeed      )
  EVT_BUTTON( ID_APPLYTRAIN,     FuzzCompFrame::OnApplyTrain      )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, FuzzCompFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  FuzzCompFrame::OnWidth1Slider  )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  FuzzCompFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  FuzzCompFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        FuzzCompFrame::OnInvert1       )
  

  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, FuzzCompFrame::OnCenter2Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  FuzzCompFrame::OnWidth2Slider  )
  EVT_CHECKBOX(       ID_INVERT2,        FuzzCompFrame::OnInvert2       )
  
  EVT_COMMAND_SCROLL( ID_CENTER3_SLIDER, FuzzCompFrame::OnCenter3Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH3_SLIDER,  FuzzCompFrame::OnWidth3Slider  )
  EVT_CHECKBOX(       ID_INVERT3,        FuzzCompFrame::OnInvert3       )
  
  EVT_COMMAND_SCROLL( ID_CENTER4_SLIDER, FuzzCompFrame::OnCenter4Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH4_SLIDER,  FuzzCompFrame::OnWidth4Slider  )
  EVT_CHECKBOX(       ID_INVERT4,        FuzzCompFrame::OnInvert4       )
    
  EVT_COMBOBOX( ID_MODE_COMBOBOX,        FuzzCompFrame::OnMode          )
  EVT_COMBOBOX( ID_ADJ_COMBOBOX,        FuzzCompFrame::OnAdjacency          )
  
  
  EVT_COMMAND_SCROLL( ID_THRES_SLIDER,   FuzzCompFrame::OnThresSlider    )
  EVT_COMMAND_SCROLL( ID_BRUSH_SLIDER,   FuzzCompFrame::OnBrushSlider    )
  EVT_COMBOBOX( ID_AFFIN_COMBOBOX,       FuzzCompFrame::OnAffinity       )
  
  EVT_CHECKBOX( ID_HIGH,                 FuzzCompFrame::OnHigh     )
  EVT_COMBOBOX( ID_HIGH_COMBO,           FuzzCompFrame::OnHighCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT1_SLIDER,  FuzzCompFrame::OnWeight1Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL1_SLIDER,   FuzzCompFrame::OnLevel1Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL1_SLIDER,  FuzzCompFrame::OnWidthLevel1Slider  )
  
  EVT_CHECKBOX( ID_LOW,                 FuzzCompFrame::OnLow    )
  EVT_COMBOBOX( ID_LOW_COMBO,           FuzzCompFrame::OnLowCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT2_SLIDER,  FuzzCompFrame::OnWeight2Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL2_SLIDER,   FuzzCompFrame::OnLevel2Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL2_SLIDER,  FuzzCompFrame::OnWidthLevel2Slider  )
  
  EVT_CHECKBOX( ID_DIFF,                 FuzzCompFrame::OnDiff     )
  EVT_COMBOBOX( ID_DIFF_COMBO,           FuzzCompFrame::OnDiffCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT3_SLIDER,  FuzzCompFrame::OnWeight3Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL3_SLIDER,   FuzzCompFrame::OnLevel3Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL3_SLIDER,  FuzzCompFrame::OnWidthLevel3Slider  )

  EVT_CHECKBOX( ID_SUM,                 FuzzCompFrame::OnSum     )
  EVT_COMBOBOX( ID_SUM_COMBO,           FuzzCompFrame::OnSumCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT4_SLIDER,  FuzzCompFrame::OnWeight4Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL4_SLIDER,   FuzzCompFrame::OnLevel4Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL4_SLIDER,  FuzzCompFrame::OnWidthLevel4Slider  )

  EVT_CHECKBOX( ID_RELDIFF,                 FuzzCompFrame::OnRelDiff     )
  EVT_COMBOBOX( ID_RELDIFF_COMBO,           FuzzCompFrame::OnRelDiffCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT5_SLIDER,  FuzzCompFrame::OnWeight5Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL5_SLIDER,   FuzzCompFrame::OnLevel5Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL5_SLIDER,  FuzzCompFrame::OnWidthLevel5Slider  )
  
  EVT_BUTTON( ID_CT_LUNG,          FuzzCompFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   FuzzCompFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          FuzzCompFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              FuzzCompFrame::OnPET     )

  EVT_COMMAND_SCROLL( ID_STARTSLICE_SLIDER,  FuzzCompFrame::OnStartSliceSlider )
  EVT_COMMAND_SCROLL( ID_ENDSLICE_SLIDER,   FuzzCompFrame::OnEndSliceSlider )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, FuzzCompFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  FuzzCompFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  FuzzCompFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  FuzzCompFrame::OnUpdateUIScale1Slider  )
  
  EVT_UPDATE_UI( ID_CENTER2_SLIDER, FuzzCompFrame::OnUpdateUICenter2Slider )
  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  FuzzCompFrame::OnUpdateUIWidth2Sglider )
  
  

  EVT_UPDATE_UI( ID_CINE_SLIDER,    FuzzCompFrame::OnUpdateUICineSlider    )
#endif

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, FuzzCompFrame::OnBusyTimer )

  EVT_CHAR( FuzzCompFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   FuzzCompFrame::OnOverlay )

  EVT_TIMER(          ID_CINE_TIMER,            FuzzCompFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          FuzzCompFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, FuzzCompFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           FuzzCompFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  FuzzCompFrame::_fileHistory;
//======================================================================
