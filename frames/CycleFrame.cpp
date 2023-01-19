/*
  Copyright 1993-2016, 2018-2019, 2021 Medical Image Processing Group
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
 * \file   CycleFrame.cpp
 * \brief  CycleFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "ChunkData.h"
#include  "CycleFrame.h"
#include  "CyclePrint.h"
#include  "CycleCanvas.h"
#include  "CineControls.h"
#include  "SetIndexControls.h"
#include  "TextControls.h"

extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
// CycleFrame class implementation
//----------------------------------------------------------------------
CycleFrame::CycleFrame ( bool maximize, int w, int h ) : MainFrame ( 0 )
{
    mCineControls = NULL;
    mDataset = NULL;
    mFileOrDataCount = 0;
    mGrayMapControls = NULL;
    mMode = NULL;
    mSetIndexControls = NULL;
    mTextControls = NULL;
    mWhichDataset = DatasetAll;
    mWhichMode = ModeLayout;
    mWhichDimension = 0;

    mZoomLevel = 0;
    mForward = mForwardBackward = false;
    mDirectionIsForward = true;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    
    m_lastChoice = m_lastChoiceSetting = -1;
    initializeMenu();
    
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -

    mCanvas = new CycleCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    SetStatusText( "move",  2 );
    SetStatusText( "",      3 );
    SetStatusText( "scale", 4 );

    Maximize( true );
    Show();
    mSplitter->SetSashPosition( -dControlsHeight );
    Raise();
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
}
//----------------------------------------------------------------------
void CycleFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
    int j;
    Vector::iterator  i;
    for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
        if (*i==this)
            break;
    wxString  tmp = wxString::Format( "CAVASS:Cycle:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
//----------------------------------------------------------------------
/** \brief this method adds the button box that appears at the
 *  right-bottom of the window.
 *  <pre>
 *  buttons:
 *      Mode:           Layout/ROI/Cine/Continuous/Discontinuous/FastCine
 *
 *      Reset           Erase
 *      x labels        x interpolate
 *      GrayMap         all/file1/file2/.../filen
 *
 *      SetIndex        Duration
 *      x measure       x lock/unlock
 *      x motion f/f-b
 *  </pre>
 */
void CycleFrame::addButtonBox ( void ) {
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
    mWhichMode = ModeLayout;
    mWhichRoiMode = RoiModeNone;
    mMode = new wxComboBox( mControlPanel, ID_MODE,
        "mode", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
        0, NULL, wxCB_READONLY );
    ::setColor( mMode );
    assert( ModeLayout==0 );           mMode->Append( "layout"        );
    assert( ModeRoi==1 );              mMode->Append( "roi"           );
    assert( ModeCine==2 );             mMode->Append( "cine"          );
    assert( ModeContinuous==3 );       mMode->Append( "continuous"    );
    assert( ModeDiscontinuous==4 );    mMode->Append( "discontinuous" );
    assert( ModeFastCine==5 );         mMode->Append( "fast cine"     );
    assert( ModeErase==6 );            mMode->Append( "erase"         );
    assert( ModeMeasure==7 );          mMode->Append( "measure"       );
    assert( ModeLock==8 );             mMode->Append( "lock/unlock"   );
    mMode->SetSelection( mWhichMode );
    fgs->Add( mMode, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 2, col 1
    mLabelsCheck = new wxCheckBox( mControlPanel, ID_LABELS, "labels" );
    mLabelsCheck->SetValue( 1 );
    fgs->Add( mLabelsCheck, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0 );
    ::setColor( mLabelsCheck );
    //row 2, col 2
    mInterpolateCheck = new wxCheckBox( mControlPanel, ID_INTERPOLATE, "interpolate" );
    mInterpolateCheck->SetValue( 0 );
    fgs->Add( mInterpolateCheck, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0 );
    ::setColor( mInterpolateCheck );

    //row 3, col 1
    wxButton*  grayMap = new wxButton( mControlPanel, ID_GRAYMAP, "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap );
    fgs->Add( grayMap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
    wxButton*  setIndex = new wxButton( mControlPanel, ID_SET_INDEX, "SetIndex", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( setIndex );
    fgs->Add( setIndex, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 4, col 1
    st = new wxStaticText( mControlPanel, wxID_ANY, "DataSet:" );
    ::setColor( st );
    fgs->Add( st, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    mWhichDataset = DatasetAll;
    mDataset = new wxComboBox( mControlPanel, ID_DATASET,
        "dataset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
        0, NULL, wxCB_READONLY );
    ::setColor( mDataset );
    assert( DatasetAll==0 );
    mDataset->Append( "all" );
    mDataset->SetSelection( mWhichDataset );
    fgs->Add( mDataset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 5, col 1
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    wxButton*  duration = new wxButton( mControlPanel, ID_DURATION, "Duration", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( duration );
    fgs->Add( duration, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 6, col 1
    wxButton *saveBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( saveBut );
    fgs->Add( saveBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CycleFrame::~CycleFrame ( void ) {
    cout << "CycleFrame::~CycleFrame" << endl;
    wxLogMessage( "CycleFrame::~CycleFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }
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
/** \brief callback for center slider (used to change contrast). */
void CycleFrame::OnCenterSlider ( wxScrollEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
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
void CycleFrame::OnWidthSlider ( wxScrollEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
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
void CycleFrame::OnCTLung ( wxCommandEvent& unused ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
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
void CycleFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
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
void CycleFrame::OnCTBone ( wxCommandEvent& unused ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
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
void CycleFrame::OnPET ( wxCommandEvent& unused ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
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
void CycleFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "CycleFrame::OnChar" );
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
        cout << "CycleFrame::OnChar: " << ::gTimerInterval << endl;
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
void CycleFrame::OnHideControls ( wxCommandEvent& ignored ) {
    if (mSplitter==NULL)    return;
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel,
                                       -dControlsHeight );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnSave ( wxCommandEvent& e ) {

    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    if (canvas==NULL || canvas->m_images[0]==NULL)  return;

#ifdef SAVE_IMAGE_IMPLEMEMTED

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

#else

    wxFileName fn(canvas->mCavassData->m_fname);

    wxFileDialog saveDlg(this,
                _T("Save file"),
                wxEmptyString,
                fn.GetName()+".TXT",
                _T("TXT files (*.TXT)|*.TXT"),
                wxSAVE);
    if (saveDlg.ShowModal() == wxID_OK)
    {
        FILE *fp=fopen((const char *)saveDlg.GetPath().c_str(), "a");
        if (fp == NULL)
        {
            wxMessageBox("Could not open file.");
            return;
        }
        for (int j=0; j<(int)mMeasurements.GetCount(); j++)
            if (fprintf(fp, "%s\n",(const char *)mMeasurements[j].c_str()) < 1)
            {
                wxMessageBox("Could not save text.");
                break;
            }
        fprintf(fp, "\n");
        fclose(fp);
		canvas->mSaveFile = saveDlg.GetPath();
    }

#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnOpen ( wxCommandEvent& e ) {
    createCycleFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new CycleFrame.  It first searches the input file history for an
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
void CycleFrame::createCycleFrame ( wxFrame* parentFrame, bool useHistory )
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
            "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff;*vtk)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff;*vtk",
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

    //display a Cycle frame using the specified file as input
    CycleFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new CycleFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new CycleFrame();
    }
    for (int i=filenames.Count()-1; i>=0; i--) {
        frame->loadFile( filenames[i].c_str() );
    }

    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( frame->mCanvas );
    assert( canvas != NULL );
    canvas->setScale( 1.0 );

    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::loadFile ( const char* const fname ) {
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
        tmp = wxString::Format( "CAVASS:Cycle:%s", fname );
    else
        tmp = wxString::Format( "%s %s", (const char *)mWindowTitle.c_str(), fname );

    //does a window with this title (file) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "%s (%d)", (const char *)mWindowTitle.c_str(), i);
            if (!searchWindowTitles(tmp))    break;
        }
    }
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );

    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
    assert( canvas != NULL );
    canvas->loadFile( fname );
    if (!canvas->isLoaded(0))
    {
        delete m_buttonBox;
        m_buttonBox = NULL;
        return;
    }
    if (canvas->mCavassData->m_vh.scn.dimension==4 &&
            canvas->mCavassData->m_vh.scn.num_of_subscenes[0]>1 &&
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[1]-
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[0]<=2)
    {
        ::gTimerInterval = (int)rint(1000*(
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[1]-
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[0]));
    }
#if 0
    //    initSubs();
    //windblows seems to have a problem with ranges outside of short.
#if defined (WIN32) || defined (_WIN32)
    if (canvas->mCavassData->m_max<SHRT_MAX)
        m_center->SetRange( canvas->mCavassData->m_min, canvas->mCavassData->m_max );
    else
        m_center->SetRange( canvas->mCavassData->m_min, SHRT_MAX );
#else
    m_center->SetRange( canvas->mCavassData->m_min, canvas->mCavassData->m_max );
#endif
    m_center->SetValue( canvas->mCavassData->getCenter() );
#if defined (WIN32) || defined (_WIN32)
    if (canvas->mCavassData->m_max<SHRT_MAX)
        m_width->SetRange( 1, canvas->mCavassData->m_max );
    else
        m_width->SetRange( 1, SHRT_MAX );
#else
    m_width->SetRange( 1, canvas->mCavassData->m_max );
#endif
    m_width->SetValue(  canvas->mCavassData->getWidth() );
    
    m_sliceNo->SetValue( 0 );
    m_sliceNo->SetRange( 0, canvas->mCavassData->m_zSize-1 );
#endif
    //mDataset->Append( fname );
    wxString  s = wxString::Format( "%d: %s", mDataset->GetCount(), fname );
    mDataset->Append( s );
    s = wxString::Format( "Done loading %s", fname );
    SetStatusText( s, 0 );
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void CycleFrame::loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized )
    {
        if (name==NULL || strlen(name)==0)  name=(char *)"no name";
        wxString  tmp("CAVASS:Cycle: ");
        tmp += name;
        SetTitle( tmp );

        CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
        assert( canvas != NULL );
        canvas->loadData( name, xSize, ySize, zSize,
            xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
//        initSubs();
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
void CycleFrame::OnZoomIn ( wxCommandEvent& e ) {
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
void CycleFrame::OnZoomOut ( wxCommandEvent& e ) {
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
void CycleFrame::OnCine ( wxCommandEvent& e ) {
    if (mCineControls!=NULL) {  removeAll();  return;  }
    removeAll();
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER,
        m_cine_timer, wxID_ANY, canvas->mCavassData->m_vh.scn.dimension==4?
        ID_CINE_DIMENSION: (int)wxID_ANY, mWhichDimension );
    mCineControls->setDelay( ::gTimerInterval );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
void CycleFrame::OnMagnify ( wxCommandEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
    canvas->setScale( 0.3 );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnReset ( wxCommandEvent& e ) {
    removeAll();
    m_cine_timer->Stop();

    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
    if (canvas->mCavassData->m_vh.scn.dimension==4 &&
            canvas->mCavassData->m_vh.scn.num_of_subscenes[0]>1 &&
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[1]-
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[0]<=2)
    {
        ::gTimerInterval = (int)rint(1000*(
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[1]-
            canvas->mCavassData->m_vh.scn.loc_of_subscenes[0]));
    }
    else
        ::gTimerInterval = 250;

    canvas->OnReset();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnMouseWheel ( wxMouseEvent& e ) {
    const int  rot   = e.GetWheelRotation();
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo();
    int  slices = canvas->getNoSlices();
    if (rot > 0)
    {
        // Previous slice
        if (slice <= 0)
            slice = slices-1;
        else
            slice--;
        setSliceNo(slice);
        canvas->setSliceNo(slice);
        canvas->reload();
    }
    else if (rot < 0)
    {
        // Next slice
        if (slice >= slices-1)
            slice = 0;
        else
            slice++;
        setSliceNo(slice);
        canvas->setSliceNo(slice);
        canvas->reload();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief this method is called when the SetIndex button is pressed to
 *  either show or unshow the SetIndex controls
 */
void CycleFrame::OnSetIndex ( wxCommandEvent& e ) {
    if (mSetIndexControls!=NULL) {  removeAll();  return;  }
    removeAll();
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    mSetIndexControls = new SetIndexControls( mControlPanel, mBottomSizer,
        "SetIndex", canvas->getSliceNo(), canvas->getNoSlices(), ID_SLICE_SLIDER,
        ID_SCALE_SLIDER, canvas->getScale(), ID_LABELS, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 *  \param unused is not used.
 */
void CycleFrame::OnGrayMap ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=NULL) {  removeAll();  return;  }
    removeAll();
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
    int  center, width, max;
    bool invert;

    if (mWhichDataset!=DatasetAll) {
        int  i = mWhichDataset - 1;
        center = canvas->getCenter( i );
        width  = canvas->getWidth( i );
        max    = canvas->getMax( i );
        invert = canvas->getInvert( i );
    } else {
        center = canvas->getCenter( 0 );
        width  = canvas->getWidth( 0 );
        max    = canvas->getMax( 0 );
        for (int i=1; i<mFileOrDataCount; i++) {
            if (canvas->getMax(i) > max)    max = canvas->getMax(i);
        }
        invert = canvas->getInvert( 0 );
    }
    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", center, width, max, invert,
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
        ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::removeAll ( void ) {
    if (mCineControls!=NULL) {
        delete mCineControls;
        mCineControls = NULL;
    }
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
    if (mSetIndexControls!=NULL) {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
//    if (mTextControls!=NULL) {
//        delete mTextControls;
//        mTextControls = NULL;
//    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void CycleFrame::OnSliceSlider ( wxScrollEvent& e ) {
        //wxLogMessage( "in OnSliceSlider" );
        CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
        assert( canvas != NULL );
        canvas->setSliceNo( e.GetPosition()-1 );
        //wxLogMessage( "OnSliceSlider: reloading" );
        canvas->reload();
        //wxLogMessage( "out OnSliceSlider" );
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::setSliceNo ( int sliceNo ) {
    if (mSetIndexControls==NULL)    return;
    mSetIndexControls->setSliceNo( sliceNo );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider */
void CycleFrame::OnScaleSlider ( wxScrollEvent& e ) {
    SetStatusText( "scale...", 0 );
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue / 100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndexControls->setScaleText( s );
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    canvas->setScale( newScale );
    SetStatusText( "ready", 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnLabels ( wxCommandEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    assert( canvas != NULL );
    canvas->mCavassData->setOverlay( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnCache ( wxCommandEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
    assert( canvas != NULL );
    ChunkData*  chunk = dynamic_cast<ChunkData*>( canvas->mCavassData );
    while (chunk) {
        chunk->mFreeOldChunk = !e.IsChecked();
        chunk = dynamic_cast<ChunkData*>( chunk->mNext );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnInterpolateData ( wxCommandEvent& e ) {
    SetStatusText( "interpolate...", 0 );
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
    assert( canvas != NULL );
    canvas->setInterpolate( e.IsChecked() );
    canvas->reload();
    SetStatusText( "ready", 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnInvert ( wxCommandEvent& e ) {
    SetStatusText( "invert...", 0 );
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
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
#if 0
void CycleFrame::uncheckExcept ( const int leaveAloneID ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    assert( canvas != NULL );
    //uncheck Cine if necessary
    if (leaveAloneID!=ID_CINE && m_options_menu->IsChecked(ID_CINE)) {
        m_options_menu->Check(ID_CINE, false);
        m_cine_timer->Stop();
    }
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnCineForward ( wxCommandEvent& e ) {
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
void CycleFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
    if (e.IsChecked()) {
        mForward            = false;
        mForwardBackward    = true;
        mDirectionIsForward = true;
        mCineControls->mForwardCheck->SetValue( 0 );
        //very important on windoze to make it a oneshot
        //1000 = 1 sec interval; true = oneshot
        m_cine_timer->Start( ::gTimerInterval, true );
    } else {
        m_cine_timer->Stop();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnCineTimer ( wxTimerEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    const int  slices = canvas->getNoSlices();
    int  s = canvas->getSliceNo();

    if (canvas->mCavassData->m_vh.scn.dimension==4 && mWhichDimension)
    {
        int v, vs, pvs;
        for (v=pvs=0; v<canvas->mCavassData->m_vh.scn.num_of_subscenes[0]-1;
                pvs+=canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v],v++)
            if (s<pvs+canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v])
                break;
        vs = s-pvs;
        switch (mWhichDimension)
        {
          case 1: // x_3
            if (mForward)
            {
                vs++;
                if (vs >= canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v])
                    vs = 0;
            }
            else if (mDirectionIsForward)
            {
                vs++;
                if (vs >= canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v])
                {
                    vs--;
                    mDirectionIsForward = false;
                }
            }
            else
            {
                vs--;
                if (vs < 0)
                {
                    vs = 0;
                    mDirectionIsForward = true;
                }
            }
            break;
          case 2: // x_4
            if (mForward)
            {
                pvs += canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                v++;
                if (v >= canvas->mCavassData->m_vh.scn.num_of_subscenes[0])
                    v = pvs = 0;
            }
            else if (mDirectionIsForward)
            {
                pvs += canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                v++;
                if (v >= canvas->mCavassData->m_vh.scn.num_of_subscenes[0])
                {
                    v--;
                    pvs -= canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                    mDirectionIsForward = false;
                }
            }
            else
            {
                if (v == 0)
                    mDirectionIsForward = true;
                else
                {
                    v--;
                    pvs -= canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                }
            }
            break;
          case 3: // time stamp
            if (mForward)
            {
                pvs += canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                v++;
                if (v < canvas->mCavassData->m_vh.scn.num_of_subscenes[0])
                  ::gTimerInterval = (int)rint(1000*(
                    canvas->mCavassData->m_vh.scn.loc_of_subscenes[v]-
                    canvas->mCavassData->m_vh.scn.loc_of_subscenes[v-1]));
                if (v+1 >= canvas->mCavassData->m_vh.scn.num_of_subscenes[0])
                    v = pvs = 0;
            }
            else if (mDirectionIsForward)
            {
                pvs += canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                v++;
                if (v < canvas->mCavassData->m_vh.scn.num_of_subscenes[0])
                  ::gTimerInterval = (int)rint(1000*(
                    canvas->mCavassData->m_vh.scn.loc_of_subscenes[v]-
                    canvas->mCavassData->m_vh.scn.loc_of_subscenes[v-1]));
                if (v >= canvas->mCavassData->m_vh.scn.num_of_subscenes[0])
                {
                    v--;
                    pvs -= canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                    mDirectionIsForward = false;
                }
            }
            else
            {
                if (v == 0)
                {
                    mDirectionIsForward = true;
                }
                else
                {
                    v--;
                    if (v > 0)
                      ::gTimerInterval = (int)rint(1000*(
                        canvas->mCavassData->m_vh.scn.loc_of_subscenes[v]-
                        canvas->mCavassData->m_vh.scn.loc_of_subscenes[v-1]));
                    pvs -= canvas->mCavassData->m_vh.scn.num_of_subscenes[1+v];
                }
            }
            break;
          default:
            assert(0);
        }
        s = pvs+vs;
    } else if (mForward) {
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
void CycleFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void CycleFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
        //#ifdef WIN32
        #if 1
            //very important on windoze to make it a oneshot
            m_cine_timer->Start(::gTimerInterval, true);
        #endif
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void CycleFrame::OnBusyTimer ( wxTimerEvent& e ) {
        cout << "OnBusyTimer" << endl;
        //static bool  flipFlop = true;
        //if (flipFlop)  SetCursor( wxCursor(wxCURSOR_WAIT) );
        //else           SetCursor( *wxSTANDARD_CURSOR );
        //flipFlop = !flipFlop;
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void CycleFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& e ) {
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
    void CycleFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& e ) {
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
    void CycleFrame::OnUpdateUISliceSlider ( wxUpdateUIEvent& e ) {
        if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
        mCanvas->setSliceNo( m_sliceNo->GetValue() );
        mCanvas->reload();
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
    //especially (only) need on X11 (w/out GTK) to get slider events
    void CycleFrame::OnUpdateUIScaleSlider ( wxUpdateUIEvent& e ) {
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
    void CycleFrame::OnUpdateUIDemonsSlider ( wxUpdateUIEvent& e ) {
        //especially (only) need on X11 (w/out GTK) to get slider events
        if ( m_demonsChoices->GetSelection() == m_lastChoice
             && m_demonsSlider->GetValue() == m_lastChoiceSetting)  return;
        OnDemonsChoices( e );
    }
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnPrintPreview ( wxCommandEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    assert( canvas != NULL );
    // Pass two printout objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrintPreview*    preview = new wxPrintPreview(
        new CyclePrint(wxString("CAVASS").c_str(), canvas),
        new CyclePrint(wxString("CAVASS").c_str(), canvas),
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
void CycleFrame::OnPrint ( wxCommandEvent& e ) {
    CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>(mCanvas);
    assert( canvas != NULL );
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    CyclePrint       printout( _T("Cycle"), canvas );
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
void CycleFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "CycleFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "CycleFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnCycle( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnDataset ( wxCommandEvent& unused ) {
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
void CycleFrame::OnMode ( wxCommandEvent& unused ) {
    m_cine_timer->Stop();
    mWhichMode = mMode->GetSelection();
    indicateMode();
    mWhichRoiMode = RoiModeNone;

    //turn caching off for everything except fast cine
    if (mWhichMode == ModeFastCine) {
        CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
        assert( canvas != NULL );
        ChunkData*  chunk = dynamic_cast<ChunkData*>( canvas->mCavassData );
        while (chunk) {
            chunk->mFreeOldChunk = true;
            chunk = dynamic_cast<ChunkData*>( chunk->mNext );
        }
    } else {
        CycleCanvas*  canvas = dynamic_cast<CycleCanvas*>( mCanvas );
        assert( canvas != NULL );
        ChunkData*  chunk = dynamic_cast<ChunkData*>( canvas->mCavassData );
        while (chunk) {
            chunk->mFreeOldChunk = false;
            chunk = dynamic_cast<ChunkData*>( chunk->mNext );
        }
    }

    if (mWhichMode == ModeCine || mWhichMode == ModeFastCine) {
        SetStatusText( "resume",  2 );
        SetStatusText( "reverse", 3 );
        SetStatusText( "done",    4 );
        return;
    }

    if (mWhichMode == ModeContinuous) {
        SetStatusText( "start", 2 );
        SetStatusText( "",      3 );
        SetStatusText( "",      4 );
        return;
    }

    if (mWhichMode == ModeDiscontinuous) {
        SetStatusText( "hop",      2 );
        SetStatusText( "previous", 3 );
        SetStatusText( "next",     4 );
        return;
    }

    if (mWhichMode == ModeErase) {
        SetStatusText( "erase", 2 );
        SetStatusText( "",      3 );
        SetStatusText( "",      4 );
        return;
    }

    if (mWhichMode == ModeLayout) {
        SetStatusText( "move",  2 );
        SetStatusText( "",      3 );
        SetStatusText( "scale", 4 );
        return;
    }

    if (mWhichMode == ModeLock) {
        SetStatusText( "lock", 2 );
        SetStatusText( "",     3 );
        SetStatusText( "",     4 );
        return;
    }

    if (mWhichMode == ModeMeasure) {
        SetStatusText( "select point", 2 );
        SetStatusText( "undo point", 3 );
        SetStatusText( "done",         4 );
        if (mTextControls==NULL)
            mTextControls = new TextControls( mControlPanel, mBottomSizer, "Measure" );
        return;
    }

    if (mWhichMode == ModeRoi) {
        mWhichRoiMode = RoiModeSelect;
        SetStatusText( "select", 2 );
        SetStatusText( "",       3 );
        SetStatusText( "",       4 );
        return;
    }

#if 0
    if (mWhichMode == ModeMeasure) {
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
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::OnCineDimension ( wxCommandEvent& unused ) {
    m_cine_timer->Stop();
    mCineControls->mForwardCheck->SetValue( 0 );
    mCineControls->mForwardBackwardCheck->SetValue( 0 );
    mWhichDimension = mCineControls->mDimension->GetSelection();
    if (mWhichDimension != 3)
        ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CycleFrame::indicateMode ( void ) {
    switch (mWhichMode) {
        case CycleFrame::ModeCine:
            SetStatusText( "left click to resume; middle to reverse; right to pause", 0 );
            break;
        case CycleFrame::ModeContinuous:
            SetStatusText( "right click to start", 0 );
            break;
        case CycleFrame::ModeDiscontinuous:
            SetStatusText( "left click to hop; middle for previous; right for next", 0 );
            break;
        case CycleFrame::ModeErase:
            SetStatusText( "left click image to erase", 0 );
            break;
        case CycleFrame::ModeFastCine:
            SetStatusText( "left click to resume; middle to reverse; right to pause", 0 );
            break;
        case CycleFrame::ModeLayout :
            SetStatusText( "left click to move; right click to scale", 0 );
            break;
        case CycleFrame::ModeLock:
            SetStatusText( "left click on image to lock", 0 );
            break;
        case CycleFrame::ModeMeasure:
            SetStatusText( "", 0 );
            break;
        case CycleFrame::ModeRoi:
            SetStatusText( "left click to move; right click to size", 0 );
            break;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( CycleFrame, wxFrame )
BEGIN_EVENT_TABLE       ( CycleFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_MOUSEWHEEL(           CycleFrame::OnMouseWheel )
  EVT_BUTTON( ID_DURATION,      CycleFrame::OnCine     )
  EVT_BUTTON( ID_GRAYMAP,   CycleFrame::OnGrayMap  )
//  EVT_BUTTON( ID_MAGNIFY,   CycleFrame::OnMagnify  )
  EVT_BUTTON( ID_RESET,     CycleFrame::OnReset    )
  EVT_BUTTON( ID_SET_INDEX, CycleFrame::OnSetIndex )
  EVT_BUTTON( ID_SAVE,      CycleFrame::OnSave     )

  EVT_COMBOBOX( ID_DATASET, CycleFrame::OnDataset )
  EVT_COMBOBOX( ID_MODE,    CycleFrame::OnMode    )
  EVT_COMBOBOX( ID_CINE_DIMENSION, CycleFrame::OnCineDimension )

  EVT_MENU( ID_LABELS,         CycleFrame::OnLabels        )
  EVT_MENU( ID_INVERT,         CycleFrame::OnInvert        )
  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, CycleFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  CycleFrame::OnWidthSlider  )
  EVT_BUTTON( ID_CT_LUNG,          CycleFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   CycleFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          CycleFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              CycleFrame::OnPET     )
  EVT_COMMAND_SCROLL( ID_SLICE_SLIDER,  CycleFrame::OnSliceSlider  )
  EVT_COMMAND_SCROLL( ID_SCALE_SLIDER,  CycleFrame::OnScaleSlider  )
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER_SLIDER, CycleFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  CycleFrame::OnUpdateUIWidthSlider  )
  EVT_UPDATE_UI( ID_SLICE_SLIDER,  CycleFrame::OnUpdateUISliceSlider  )
  EVT_UPDATE_UI( ID_SCALE_SLIDER,  CycleFrame::OnUpdateUIScaleSlider  )
#endif
  EVT_BUTTON( ID_ZOOM_IN,        CycleFrame::OnZoomIn                 )
  EVT_BUTTON( ID_ZOOM_OUT,       CycleFrame::OnZoomOut                )
  EVT_BUTTON( ID_EXIT,           MainFrame::OnQuit                   )

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_CINE_TIMER, CycleFrame::OnCineTimer )
  EVT_TIMER( ID_BUSY_TIMER, CycleFrame::OnBusyTimer )

  EVT_CHAR( CycleFrame::OnChar )
  EVT_CHECKBOX( ID_LABELS,      CycleFrame::OnLabels          )
  EVT_CHECKBOX( ID_INVERT,      CycleFrame::OnInvert          )
  EVT_CHECKBOX( ID_CACHE,       CycleFrame::OnCache           )
  EVT_CHECKBOX( ID_INTERPOLATE, CycleFrame::OnInterpolateData )

  EVT_CHECKBOX( ID_CINE_FORWARD,          CycleFrame::OnCineForward )
  EVT_CHECKBOX( ID_CINE_FORWARD_BACKWARD, CycleFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER, CycleFrame::OnCineSlider )
#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CINE_SLIDER, CycleFrame::OnUpdateUICineSlider )
#endif
END_EVENT_TABLE()
//======================================================================
