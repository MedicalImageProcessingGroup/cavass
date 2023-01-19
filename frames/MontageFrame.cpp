/*
  Copyright 1993-2016 Medical Image Processing Group
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
 * \file   MontageFrame.cpp
 * \brief  MontageFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ChunkData.h"
#include  "MontageFrame.h"
#include  "MontagePrint.h"
#include  "MontageCanvas.h"
#include  "CineControls.h"
#include  "SetIndexControls.h"
#include  "TextControls.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
// MontageFrame class implementation
//----------------------------------------------------------------------
MontageFrame::MontageFrame ( bool maximize, int w, int h ) : MainFrame ( 0 )
{
    mCineControls = NULL;
    mDataset = NULL;
    mFileOrDataCount = 0;
    mGrayMapControls = NULL;
    mMode = NULL;
    mSetIndexControls = NULL;
    mTextControls = NULL;
    mWhichDataset = DatasetAll;
    mWhichMode = ModePage;

    mZoomLevel = 0;
    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    
    m_lastChoice = m_lastChoiceSetting = -1;
    initializeMenu();
    
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -

    mCanvas = new MontageCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    SetStatusText( "Next",     2 );
    SetStatusText( "Previous", 3 );
    SetStatusText( "Previous", 4 );

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
}
//----------------------------------------------------------------------
void MontageFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    wxString  tmp = wxString::Format( "CAVASS:Montage:%d", j );
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
//----------------------------------------------------------------------
/** \brief this method adds the button box that appears at the
 *  right-bottom of the window.
 *  <pre>
 *  buttons:
 *      Mode:          page/scroll/slice/measure/magnify/move
 *      Reset          SetIndex
 *      Cine           X cache data
 *      GrayMap        all/file1/file2/.../filen
 *      X interpolate
 *  </pre>
 */
void MontageFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap

    //row 1, col 1
    wxStaticText*  st = new wxStaticText( mControlPanel, wxID_ANY, "Mode:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 1, col 2
    mWhichMode = ModePage;
    mMode = new wxComboBox( mControlPanel, ID_MODE,
        "mode", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
        0, NULL, wxCB_READONLY );
    ::setColor( mMode );
    assert( ModePage==0 );       mMode->Append( "page"    );
    assert( ModeScroll==1 );     mMode->Append( "scroll"  );
    assert( ModeSlice==2 );      mMode->Append( "slice"   );
    assert( ModeMeasure==3 );    mMode->Append( "measure" );
    assert( ModeMagnify==4 );    mMode->Append( "magnify" );
    assert( ModeMove==5 );       mMode->Append( "move"    );
    mMode->SetSelection( mWhichMode );
    fgs->Add( mMode, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 2, col 1
    st = new wxStaticText( mControlPanel, wxID_ANY, "" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 2, col 2
    st = new wxStaticText( mControlPanel, wxID_ANY, "" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 3, col 1
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 3, col 2
    wxButton*  setIndex = new wxButton( mControlPanel, ID_SET_INDEX, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex );
    fgs->Add( setIndex, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 4, col 1
    wxButton*  cine = new wxButton( mControlPanel, ID_CINE, "Cine", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( cine );
    fgs->Add( cine, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 4, col 2
    mCacheCheck = new wxCheckBox( mControlPanel, ID_CACHE, "cache data" );
    mCacheCheck->SetValue( 0 );
    fgs->Add( mCacheCheck, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    ::setColor( mCacheCheck );
    mCache = false;
#if 0
    //row 5, col 1
    st = new wxStaticText( mControlPanel, wxID_ANY, "" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 5, col 2
    st = new wxStaticText( mControlPanel, wxID_ANY, "" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
#endif
    //row 6, col 1
    wxButton*  grayMap = new wxButton( mControlPanel, ID_GRAYMAP, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap );
    fgs->Add( grayMap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 6, col 2
    mWhichDataset = DatasetAll;
    mDataset = new wxComboBox( mControlPanel, ID_DATASET,
        "dataset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
        0, NULL, wxCB_READONLY );
    ::setColor( mDataset );
    assert( DatasetAll==0 );
    mDataset->Append( "all" );
    mDataset->SetSelection( mWhichDataset );
    fgs->Add( mDataset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 7, col 1
    mInterpolateCheck = new wxCheckBox( mControlPanel, ID_INTERPOLATE, "interpolate" );
    mInterpolateCheck->SetValue( 0 );
    fgs->Add( mInterpolateCheck, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    ::setColor( mInterpolateCheck );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MontageFrame::~MontageFrame ( void ) {
    cout << "MontageFrame::~MontageFrame" << endl;
    wxLogMessage( "MontageFrame::~MontageFrame" );
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
/** \brief This function determines if the given filename is of a type
 *  that can be read by this module/app.
 *
 *  Supported file extensions: BIM, BMP, DCM, DICOM, GIF, IM0, MV0, JPG, JPEG,
 *  PCX, TIF, and TIFF.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool MontageFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.BMP",   fn, false ))    return true;
    if (wxMatchWild( "*.DCM",   fn, false ))    return true;
    if (wxMatchWild( "*.DICOM", fn, false ))    return true;
    if (wxMatchWild( "*.GIF",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;
    if (wxMatchWild( "*.JPG",   fn, false ))    return true;
    if (wxMatchWild( "*.JPEG",  fn, false ))    return true;
	if (wxMatchWild( "*.MV0",   fn, false ))    return true;
    if (wxMatchWild( "*.PCX",   fn, false ))    return true;
    if (wxMatchWild( "*.PNG",   fn, false ))    return true;
    if (wxMatchWild( "*.TIF",   fn, false ))    return true;
    if (wxMatchWild( "*.TIFF",  fn, false ))    return true;
    //if (wxMatchWild( "*.VTK",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void MontageFrame::OnCenterSlider ( wxScrollEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            if (canvas->getCenter(i)==e.GetPosition())    continue;  //no change
            canvas->setCenter( i, e.GetPosition() );
            canvas->initLUT( i );
        }
    } else {
        int  i = mWhichDataset - 1;
        if (canvas->getCenter(i)==e.GetPosition())    return;  //no change
        canvas->setCenter( i, e.GetPosition() );
        canvas->initLUT( i );
    }
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider (used to change contrast). */
void MontageFrame::OnWidthSlider ( wxScrollEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            if (canvas->getWidth(i)==e.GetPosition())    continue;  //no change
            canvas->setWidth( i, e.GetPosition() );
            canvas->initLUT( i );
        }
    } else {
        int  i = mWhichDataset - 1;
        if (canvas->getWidth(i)==e.GetPosition())    return;  //no change
        canvas->setWidth( i, e.GetPosition() );
        canvas->initLUT( i );
    }
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTLung button */
void MontageFrame::OnCTLung ( wxCommandEvent& unused ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            if (canvas->getWidth(i)==Preferences::getCTLungWidth() &&
					canvas->getCenter(i)==Preferences::getCTLungCenter())
			    continue;  //no change
            canvas->setCenter( i, Preferences::getCTLungCenter() );
			canvas->setWidth( i, Preferences::getCTLungWidth() );
			canvas->setInvert( i, false );
            canvas->initLUT( i );
        }
    } else {
        int  i = mWhichDataset - 1;
        if (canvas->getWidth(i)==Preferences::getCTLungWidth() &&
				canvas->getCenter(i)==Preferences::getCTLungCenter())
		    return;  //no change
        canvas->setCenter( i, Preferences::getCTLungCenter() );
		canvas->setWidth( i, Preferences::getCTLungWidth() );
		canvas->setInvert( i, false );
        canvas->initLUT( i );
    }
	mGrayMapControls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTSoftTissue button */
void MontageFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            if (canvas->getWidth(i)==Preferences::getCTSoftTissueWidth() &&
					canvas->getCenter(i)==Preferences::getCTSoftTissueCenter())
			    continue;  //no change
            canvas->setCenter( i, Preferences::getCTSoftTissueCenter() );
			canvas->setWidth( i, Preferences::getCTSoftTissueWidth() );
			canvas->setInvert( i, false );
            canvas->initLUT( i );
        }
    } else {
        int  i = mWhichDataset - 1;
        if (canvas->getWidth(i)==Preferences::getCTSoftTissueWidth() &&
				canvas->getCenter(i)==Preferences::getCTSoftTissueCenter())
		    return;  //no change
        canvas->setCenter( i, Preferences::getCTSoftTissueCenter() );
		canvas->setWidth( i, Preferences::getCTSoftTissueWidth() );
		canvas->setInvert( i, false );
        canvas->initLUT( i );
    }
	mGrayMapControls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnCTBone button */
void MontageFrame::OnCTBone ( wxCommandEvent& unused ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            if (canvas->getWidth(i)==Preferences::getCTBoneWidth() &&
					canvas->getCenter(i)==Preferences::getCTBoneCenter())
			    continue;  //no change
            canvas->setCenter( i, Preferences::getCTBoneCenter() );
			canvas->setWidth( i, Preferences::getCTBoneWidth() );
			canvas->setInvert( i, false );
            canvas->initLUT( i );
        }
    } else {
        int  i = mWhichDataset - 1;
        if (canvas->getWidth(i)==Preferences::getCTBoneWidth() &&
				canvas->getCenter(i)==Preferences::getCTBoneCenter())
		    return;  //no change
        canvas->setCenter( i, Preferences::getCTBoneCenter() );
		canvas->setWidth( i, Preferences::getCTBoneWidth() );
		canvas->setInvert( i, false );
        canvas->initLUT( i );
    }
	mGrayMapControls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for OnPET button */
void MontageFrame::OnPET ( wxCommandEvent& unused ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            if (canvas->getWidth(i)==Preferences::getPETWidth() &&
					canvas->getCenter(i)==Preferences::getPETCenter())
			    continue;  //no change
            canvas->setCenter( i, Preferences::getPETCenter() );
			canvas->setWidth( i, Preferences::getPETWidth() );
			canvas->setInvert( i, true );
            canvas->initLUT( i );
        }
    } else {
        int  i = mWhichDataset - 1;
        if (canvas->getWidth(i)==Preferences::getPETWidth() &&
				canvas->getCenter(i)==Preferences::getPETCenter())
		    return;  //no change
        canvas->setCenter( i, Preferences::getPETCenter() );
		canvas->setWidth( i, Preferences::getPETWidth() );
		canvas->setInvert( i, true );
        canvas->initLUT( i );
    }
	mGrayMapControls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth(), true);
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "MontageFrame::OnChar" );
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
        cout << "MontageFrame::OnChar: " << ::gTimerInterval << endl;
    }

    switch (e.GetKeyCode()) {
    case 'a' :
    case 'A' :
        ++mZoomLevel;
        if (mZoomLevel>5)    mZoomLevel = 5;
        ::setZoomInColor( m_scale );
        break;
    case 'z' :
    case 'Z' :
        --mZoomLevel;
        if (mZoomLevel<0)    mZoomLevel = 0;
        if (mZoomLevel<1)    ::setColor( m_scale );
        break;
    case '>' :
    case '.' :
        if (!m_cine_timer->IsRunning()) {
            ++mZoomLevel;
            if (mZoomLevel>5)    mZoomLevel = 5;
            ::setZoomInColor( m_scale );
        }
        break;
    case '<' :
    case ',' :
        if (!m_cine_timer->IsRunning()) {
            --mZoomLevel;
            if (mZoomLevel<0)    mZoomLevel = 0;
            if (mZoomLevel<1)    ::setColor( m_scale );
        }
        break;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnHideControls ( wxCommandEvent& ignored ) {
    if (mSplitter==NULL)    return;
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel,
                                       -dControlsHeight-30 );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnSave ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    if (canvas==NULL || canvas->m_images[0]==NULL)  return;
    wxString  savefilename = wxFileSelector( wxT("Save Image"), wxT(""),
        wxT("temp"), (const wxChar *)NULL,
        wxT("TIFF files (*.tif)|*.tif|")
        wxT("BMP files (*.bmp)|*.bmp|")
        wxT("PNG files (*.png)|*.png|")
        wxT("JPEG files (*.jpg)|*.jpg|")
        wxT("PCX files (*.pcx)|*.pcx|"),
        wxSAVE );
    if (savefilename.empty())    return;
    const wxString  extention = savefilename.AfterLast('.').Lower();
    const bool  loaded = canvas->m_images[0]->SaveFile(savefilename);
    if (!loaded) {
        wxMessageBox( _T("No handler for this file type."),
            _T("File was not saved."), wxOK|wxCENTRE, this );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    if (mSaveScreenControls!=NULL) {  removeAll();  return;  }
	removeAll();
	MainFrame::OnSaveScreen( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnOpen ( wxCommandEvent& e ) {
    createMontageFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new MontageFrame.  It first searches the input file history for an
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
void MontageFrame::createMontageFrame ( wxFrame* parentFrame, bool useHistory )
{
    wxArrayString  filenames;
    if (useHistory) {
        //search the input history for acceptable input files
        const int  n = ::gInputFileHistory.GetNoHistoryFiles();
        for (int i=0; i<n; i++) {
            wxString  s = ::gInputFileHistory.GetHistoryFile( i );
            if (match(s) && ::gInputFileHistory.IsSelected( i )) {
                //before we insert it into the array, make sure that it exists
                if (wxFile::Exists(s)) {
                    filenames.Add( s );
                } else {
                    wxString  tmp = wxString::Format( "File %s could not be opened.",
                        (const char *)s.c_str() );
                    wxMessageBox( tmp, "File does not exist",
                        wxOK | wxCENTER | wxICON_EXCLAMATION );
                }
            }
        }
    }

    //did we find any acceptable input files in the input history?
    if (filenames.Count()==0) {
        //nothing acceptable so display a dialog which allows the user
        // to choose input files.
        wxFileDialog  fd( parentFrame, _T("Select image file(s)"), _T(""), _T(""),
            "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );
        if (fd.ShowModal() != wxID_OK)    return;
		wxArrayString recent_filenames;
        fd.GetFilenames( recent_filenames );
		for (unsigned int i=0; i<recent_filenames.Count(); i++)
			if (::gInputFileHistory.IsSelected( i ))
				filenames.Add(recent_filenames[i]);
        if (filenames.Count()==0)    return;
        //add the input files to the input history
        wxString  d = fd.GetDirectory();
        d += wxFileName::GetPathSeparator();
        for (int i=filenames.Count()-1; i>=0; i--) {
            filenames[i] = d + filenames[i];  //prepend the directory
			::gInputFileHistory.AddFileToHistory( filenames[i] );
			wxLogMessage( "AddFileToHistory( " + filenames[i] + " )" );
        }
        wxConfigBase*  pConfig = wxConfigBase::Get();
        ::gInputFileHistory.Save( *pConfig );
    }

    //at this point, filenames must have at least 1 file
    if (filenames.Count()==0)    return;

    //display a Montage frame using the specified file as input
    MontageFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new MontageFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new MontageFrame();
    }
    for (int i=filenames.Count()-1; i>=0; i--) {
        frame->loadFile( filenames[i].c_str() );
    }

    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( frame->mCanvas );
    assert( canvas != NULL );
    canvas->setScale( 1.0 );

    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxString  tmp = wxString::Format( "File %s could not be opened.", fname );
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }

    ++mFileOrDataCount;
    wxString  tmp;
    if (mFileOrDataCount==1)
        tmp = wxString::Format( "CAVASS:Montage:%s", fname );
    else
        tmp = wxString::Format( "%s %s", (const char *)mWindowTitle.c_str(), fname );

    //does a window with this title (file) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "%s (%d)", (const char *)mWindowTitle.c_str(), i );
            if (!searchWindowTitles(tmp))    break;
        }
    }
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );

    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    assert( canvas != NULL );
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}
    wxString  s = wxString::Format( "%d: %s", mDataset->GetCount(), fname );
    mDataset->Append( s );
    s = wxString::Format( "Done loading %s", fname );
    SetStatusText( s, 0 );
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void MontageFrame::loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized )
    {
        if (name==NULL || strlen(name)==0)  name=(char *)"no name";
        wxString  tmp("CAVASS:Montage: ");
        tmp += name;
        SetTitle( tmp );

        MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
        assert( canvas != NULL );
        canvas->loadData( name, xSize, ySize, zSize,
            xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
        m_center->SetRange( canvas->mCavassData->m_min, canvas->mCavassData->m_max );
        m_center->SetValue( canvas->mCavassData->getCenter() );
        m_width->SetRange(  canvas->mCavassData->m_min, canvas->mCavassData->m_max );
        m_width->SetValue(  canvas->mCavassData->getWidth() );
        m_sliceNo->SetValue( 0 );
        m_sliceNo->SetRange( 0, canvas->mCavassData->m_zSize-1 );

        const wxString  s = wxString::Format( "data %s", name );
        SetStatusText(s, 0);

        Show();
        Raise();
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnZoomIn ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnZoomOut ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnCine ( wxCommandEvent& e ) {
    if (mCineControls!=NULL) {  removeAll();  return;  }
	removeAll();
    //MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnReset ( wxCommandEvent& e ) {
	removeAll();
    m_cine_timer->Stop();

    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    mWhichMode = ModePage;
    mMode->SetSelection( mWhichMode );
    canvas->OnReset();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief this method is called when the SetIndex button is pressed to
 *  either show or unshow the SetIndex controls
 */
void MontageFrame::OnSetIndex ( wxCommandEvent& e ) {
    if (mSetIndexControls!=NULL) {  removeAll();  return;  }
	removeAll();
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    mSetIndexControls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex", canvas->getSliceNo(), canvas->getNoSlices(), ID_SLICE_SLIDER,
        ID_SCALE_SLIDER, canvas->getScale(), ID_OVERLAY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 *  \param unused is not used.
 */
void MontageFrame::OnGrayMap ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=NULL) {  removeAll();  return;  }
	removeAll();
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    int  center, width, max, min;
    bool invert;

    if (mWhichDataset!=DatasetAll) {
        int  i = mWhichDataset - 1;
        center = canvas->getCenter(i);
        width  = canvas->getWidth(i);
        max    = canvas->getMax(i);
        invert = canvas->getInvert(i);
		min    = canvas->getMin(i);
    } else {
        center = canvas->getCenter(0);
        width  = canvas->getWidth(0);
        max    = canvas->getMax(0);
		min    = canvas->getMin(0);
        for (int i=1; i<mFileOrDataCount; i++) {
            if (canvas->getMax(i) > max)    max = canvas->getMax(i);
			if (canvas->getMin(i) < min)    min = canvas->getMin(i);
        }
        invert = canvas->getInvert(0);
    }
    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", center, width, max, invert,
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET, min );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::removeAll ( void ) {
    if (mCineControls!=NULL) {
        delete mCineControls;
        mCineControls = NULL;
    }
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
	if (mSaveScreenControls!=NULL) {
		delete mSaveScreenControls;
		mSaveScreenControls = NULL;
	}
    if (mSetIndexControls!=NULL) {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
    if (mTextControls!=NULL) {
        delete mTextControls;
        mTextControls = NULL;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void MontageFrame::OnSliceSlider ( wxScrollEvent& e ) {
        //wxLogMessage( "in OnSliceSlider" );
        MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
        assert( canvas != NULL );
        canvas->setSliceNo( e.GetPosition()-1 );
        //wxLogMessage( "OnSliceSlider: reloading" );
        canvas->reload();
        //wxLogMessage( "out OnSliceSlider" );
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::setSliceNo ( int sliceNo ) {
    if (mSetIndexControls==NULL)    return;
    mSetIndexControls->setSliceNo( sliceNo );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider */
void MontageFrame::OnScaleSlider ( wxScrollEvent& e ) {
    SetStatusText( "scale...", 0 );
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue / 100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndexControls->setScaleText( s );
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    canvas->setScale( newScale );
    SetStatusText( "ready", 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnOverlay ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    assert( canvas != NULL );
    canvas->mCavassData->setOverlay( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnCache ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    assert( canvas != NULL );
    ChunkData*  chunk = dynamic_cast<ChunkData*>( canvas->mCavassData );
    while (chunk) {
        chunk->mFreeOldChunk = !e.IsChecked();
        chunk = dynamic_cast<ChunkData*>( chunk->mNext );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnInterpolateData ( wxCommandEvent& e ) {
    SetStatusText( "interpolate...", 0 );
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    assert( canvas != NULL );
    canvas->setInterpolate( e.IsChecked() );
    canvas->reload();
    SetStatusText( "ready", 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnInvert ( wxCommandEvent& e ) {
    SetStatusText( "invert...", 0 );
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>( mCanvas );
    assert( canvas != NULL );
    bool  value = false;
    if (e.IsChecked())    value = true;
    if (mWhichDataset==DatasetAll) {
        for (int i=0; i<mFileOrDataCount; i++) {
            canvas->setInvert( i, value );
            canvas->initLUT( i );
        }
    } else {
        canvas->setInvert( mWhichDataset-1, value );
        canvas->initLUT( mWhichDataset-1 );
    }
    canvas->reload();
    SetStatusText( "ready", 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnCineForward ( wxCommandEvent& e ) {
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
void MontageFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void MontageFrame::OnCineTimer ( wxTimerEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    const int  slices = canvas->getNoSlices();
    int  s = canvas->getSliceNo();

    if (mForward) {
        ++s;
        if (s>=slices)    s = 0;
    } else {
        if (mDirectionIsForward) {
            ++s;
            if (s>=slices) {
                s = slices-1;
                mDirectionIsForward = false;
            }
        } else {
            --s;
            if (s<0) {
                s = 0;
                mDirectionIsForward = true;
            }
        }
    }
    setSliceNo( s );
    canvas->setSliceNo( s );
    canvas->reload();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void MontageFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
        //#ifdef WIN32
        #if 1
            //very important on windoze to make it a oneshot
            m_cine_timer->Start(::gTimerInterval, true);
        #endif
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void MontageFrame::OnBusyTimer ( wxTimerEvent& e ) {
        cout << "OnBusyTimer" << endl;
        //static bool  flipFlop = true;
        //if (flipFlop)  SetCursor( wxCursor(wxCURSOR_WAIT) );
        //else           SetCursor( *wxSTANDARD_CURSOR );
        //flipFlop = !flipFlop;
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void MontageFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& e ) {
        if (m_center->GetValue() == mCanvas->getCenter())    return;
        mCanvas->setCenter( m_center->GetValue() );
        mCanvas->initLUT();
        mCanvas->reload();
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void MontageFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& e ) {
        if (m_width->GetValue() == mCanvas->getWidth())    return;
        mCanvas->setWidth( m_width->GetValue() );
        mCanvas->initLUT();
        mCanvas->reload();
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void MontageFrame::OnUpdateUISliceSlider ( wxUpdateUIEvent& e ) {
        if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
        mCanvas->setSliceNo( m_sliceNo->GetValue() );
        mCanvas->reload();
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void MontageFrame::OnUpdateUIScaleSlider ( wxUpdateUIEvent& e ) {
        //especially (only) need on X11 (w/out GTK) to get slider events
        if (m_scale->GetValue()/100.0 == mCanvas->getScale())    return;

        const wxString  s = wxString::Format( "scale: %8.2f",
                                              m_scale->GetValue()/100.0 );
        m_scaleText->SetLabel(s);
        mCanvas->setScale( m_scale->GetValue()/100.0 );
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void MontageFrame::OnUpdateUIDemonsSlider ( wxUpdateUIEvent& e ) {
        //especially (only) need on X11 (w/out GTK) to get slider events
        if ( m_demonsChoices->GetSelection() == m_lastChoice
             && m_demonsSlider->GetValue() == m_lastChoiceSetting)  return;
        OnDemonsChoices( e );
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnPrintPreview ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    assert( canvas != NULL );
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrintPreview*    preview = new wxPrintPreview(
        new MontagePrint(wxString("CAVASS").c_str(), canvas),
        new MontagePrint(wxString("CAVASS").c_str(), canvas),
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
void MontageFrame::OnPrint ( wxCommandEvent& e ) {
    MontageCanvas*  canvas = dynamic_cast<MontageCanvas*>(mCanvas);
    assert( canvas != NULL );
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    MontagePrint       printout( _T("Montage"), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void MontageFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "MontageFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "MontageFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnMontage( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnDataset ( wxCommandEvent& unused ) {
    bool  present = false;
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
        present = true;
    }
    mWhichDataset = mDataset->GetSelection();
    if (present)    OnGrayMap( unused );

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MontageFrame::OnMode ( wxCommandEvent& unused ) {
    mWhichMode = mMode->GetSelection();
    if (mWhichMode == ModeMeasure) {
        SetStatusText( "measure", 2 );
        SetStatusText( "",        3 );
        SetStatusText( "",        4 );
        if (mTextControls==NULL)
            mTextControls = new TextControls( mControlPanel, mBottomSizer, "Measure" );
    } else if (mWhichMode == ModeMagnify) {
        SetStatusText( "Position/Size", 2 );
        SetStatusText( "OK",            3 );
        SetStatusText( "Cancel",        4 );
    } else {
        SetStatusText( "Next",          2 );
        SetStatusText( "Previous",      3 );
        SetStatusText( "Previous",        4 );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( MontageFrame, wxFrame )
BEGIN_EVENT_TABLE       ( MontageFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_CINE,      MontageFrame::OnCine     )
  EVT_BUTTON( ID_GRAYMAP,   MontageFrame::OnGrayMap  )
//  EVT_BUTTON( ID_MAGNIFY,   MontageFrame::OnMagnify  )
  EVT_BUTTON( ID_RESET,     MontageFrame::OnReset    )
  EVT_BUTTON( ID_SET_INDEX, MontageFrame::OnSetIndex )

  EVT_COMBOBOX( ID_DATASET, MontageFrame::OnDataset )
  EVT_COMBOBOX( ID_MODE,    MontageFrame::OnMode    )

  EVT_MENU( ID_OVERLAY,        MontageFrame::OnOverlay       )
  EVT_MENU( ID_INVERT,         MontageFrame::OnInvert        )
  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, MontageFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  MontageFrame::OnWidthSlider  )
  EVT_BUTTON( ID_CT_LUNG,          MontageFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   MontageFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          MontageFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              MontageFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE_SLIDER,  MontageFrame::OnSliceSlider  )
  EVT_COMMAND_SCROLL( ID_SCALE_SLIDER,  MontageFrame::OnScaleSlider  )
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER_SLIDER, MontageFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  MontageFrame::OnUpdateUIWidthSlider  )
  EVT_UPDATE_UI( ID_SLICE_SLIDER,  MontageFrame::OnUpdateUISliceSlider  )
  EVT_UPDATE_UI( ID_SCALE_SLIDER,  MontageFrame::OnUpdateUIScaleSlider  )
#endif
  EVT_BUTTON( ID_ZOOM_IN,        MontageFrame::OnZoomIn                 )
  EVT_BUTTON( ID_ZOOM_OUT,       MontageFrame::OnZoomOut                )
  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                   )

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_CINE_TIMER, MontageFrame::OnCineTimer )
  EVT_TIMER( ID_BUSY_TIMER, MontageFrame::OnBusyTimer )

  EVT_CHAR( MontageFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,     MontageFrame::OnOverlay         )
  EVT_CHECKBOX( ID_INVERT,      MontageFrame::OnInvert          )
  EVT_CHECKBOX( ID_CACHE,       MontageFrame::OnCache           )
  EVT_CHECKBOX( ID_INTERPOLATE, MontageFrame::OnInterpolateData )

  EVT_CHECKBOX( ID_CINE_FORWARD,          MontageFrame::OnCineForward )
  EVT_CHECKBOX( ID_CINE_FORWARD_BACKWARD, MontageFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER, MontageFrame::OnCineSlider )
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CINE_SLIDER, MontageFrame::OnUpdateUICineSlider )
#endif
END_EVENT_TABLE()
//======================================================================
