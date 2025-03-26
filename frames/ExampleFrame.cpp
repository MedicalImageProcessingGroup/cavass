/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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
 * \file   ExampleFrame.cpp
 * \brief  ExampleFrame class implementation.
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
#include "cavass.h"
#include "CButton.h"
#include "ExampleFrame.h"
#include "ExampleCanvas.h"
#include "GrayMapControls.h"
#include "PersistentExampleFrame.h"

extern Vector gFrameList;
//----------------------------------------------------------------------
/** \brief Constructor for ExampleFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
ExampleFrame::ExampleFrame ( bool maximize, int w, int h ) : MainFrame( 0 )  //false )
{
    cout << "ExampleFrame::ExampleFrame( maximize, w, h )" << endl;
    //init the types of input files that this app can accept
    mFileNameFilter     = (char*) "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff";
    mFileOrDataCount    = 0;
    mGrayMapControls    = nullptr;
    mModuleName         = "CAVASS:Example";
    mSaveScreenControls = nullptr;

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
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via ExampleCanvas
    mCanvas = new ExampleCanvas( mSplitter, this, wxID_ANY, wxDefaultPosition, wxDefaultSize );
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

    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );
    restoreFrameSettings();  // <-- _MUST_ be called before Show(); and Raise(); (otherwise SetPosition and other calls will be ignored)
    m_buttonBox->SetMinSize( wxSize(mCanvas->GetSize().x/6, 0) );  //because size of window may have changed

#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Move",     2 );
    SetStatusText( "Previous", 3 );
    SetStatusText( "Next",     4 );

    wxToolTip::Enable( Preferences::getShowToolTips() );
    //mSplitter->SetSashPosition( -dControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }

    Show();
    Raise();
    cout << "ExampleFrame::ExampleFrame done. \n";
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones).
 *  this function does not any additional, custom menu items (but could
 *  if necessary).
 */
void ExampleFrame::initializeMenu ( ) {
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
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right. */
void ExampleFrame::addButtonBox ( ) {
    //add spacer
    //mBottomSizer->Add( 500, 0, 1, wxALL|wxEXPAND );
    mBottomSizer->Add( 0, 0, 1 );

    //box for buttons
    int sz = mCanvas->GetSize().x;  //value of sz is not useful now (but useful later)
    m_buttonBox = new wxStaticBox( mControlPanel, wxID_ANY, "Example" );
    //    wxDefaultPosition, wxSize(1500,0) );
    //m_buttonBox->SetMinSize( wxSize(1500, 0) );
    ::setBoxColor( m_buttonBox );
    auto buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxVERTICAL );
    //buttonSizer->SetMinSize( mControlPanel->GetSize().x/2, 0 );

    auto gs = new wxGridSizer( 2, 0, 0 );  //gap between rows and cols of buttons
    //gs->SetMinSize( mControlPanel->GetSize().x/2, 0 );
    //row 0, col 0
    auto prev = new CButton( m_buttonBox, ID_PREVIOUS, "Previous" );
    //::setColor( prev );
    gs->Add( prev, 1, wxEXPAND|wxALL, 5 );
    //row 0, col 1
    auto next = new CButton( m_buttonBox, ID_NEXT, "Next" );
    //::setColor( next );
    gs->Add( next, 1, wxEXPAND|wxALL, 5 );
    //row 1, col 0
    auto grayMap = new CButton( m_buttonBox, ID_GRAYMAP, "GrayMap" );
    //::setColor( grayMap );
    gs->Add( grayMap, 1, wxEXPAND|wxALL, 5 );

    buttonSizer->Add( gs, 0, wxEXPAND|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxEXPAND|wxALL, 0 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ExampleFrame class. */
ExampleFrame::~ExampleFrame ( ) {
    cout << "ExampleFrame::~ExampleFrame" << endl;
    wxLogMessage( "ExampleFrame::~ExampleFrame" );

    //if (mSplitter!=nullptr)      { delete mSplitter;      mSplitter=nullptr;     }
    //the destruction of the above splitter also causes the destruction of the
    // canvas.  so another destruction of the canvas causes BOOM!
    //if (mCanvas!=nullptr)        { delete mCanvas;        mCanvas=nullptr;       }
    //it appears that this is already deleted as well.
    //if (mControlPanel!=nullptr)  { delete mControlPanel;  mControlPanel=nullptr; }
    //if (mBottomSizer!=nullptr)   { delete mBottomSizer;   mBottomSizer=nullptr;  }
    //mCanvas       = nullptr;
    mControlPanel = nullptr;
    mBottomSizer  = nullptr;
    //if we are in the mode that supports having more than one window open
    // at a time, we need to remove this window from the list.
#if 0
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
            //if (mCanvas!=nullptr)  {  delete mCanvas;  mCanvas=nullptr;  }
            ::gFrameList.erase( i );
            break;
        }
    }
    //if this window is the last remaining/only window, it is time for us
    // to exit.
    if (::gFrameList.begin() == ::gFrameList.end()) {
        exit(0);
    }
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new ExampleFrame.  It first searches the input file history for an
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
void ExampleFrame::createExampleFrame ( wxFrame* parentFrame, bool useHistory )
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
            "CAVASS files (*.BIM;*.IM0;*.MV0)|*.BIM;*.IM0;*.MV0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff",
            wxFILE_MUST_EXIST );
    }
    
    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    //display an example frame using the specified file as input
    ExampleFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new ExampleFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new ExampleFrame(true);
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
 *  Supported file extensions: BIM, BMP, DCM, DICOM, GIF, IM0, JPG, JPEG,
 *  PCX, TIF, TIFF, MV0, and VTK.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool ExampleFrame::match ( wxString& filename ) {
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
/** \brief callback for Open menu item.
 *  \param unused is not used.
 */
void ExampleFrame::OnOpen ( wxCommandEvent& unused ) {
    createExampleFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void ExampleFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "ExampleFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "ExampleFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnExample( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** restore/recall/reload window frame settings (e.g., size, maximized, etc.)
 */
void ExampleFrame::restoreFrameSettings ( ) {
    cout << "ExampleFrame::restoreFrameSettings() \n";
//    SetName( whatAmI() );  //important for persistence
    //mPersistentMe = wxPersistenceManager::Get().Register( this );  <-- never use this version
    mPersistentMe = wxPersistenceManager::Get().Register( this, new PersistentExampleFrame(this) );
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
void ExampleFrame::restoreControlSettings ( ) {
//    if (!Preferences::getDejaVuMode()) return;
//    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
//    assert( canvas != nullptr );
//    if (canvas == nullptr)    return;
    auto tmp = dynamic_cast<PersistentExampleFrame*>( mPersistentMe );
    assert( tmp != nullptr);
    if (tmp == nullptr)    return;
    tmp->restoreControlSettings();
#if 0
    cout << "ExampleFrame::restoreControlSettings()" << endl;
    if (!Preferences::getDejaVuMode())    return;
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    assert( canvas != nullptr );
    if (canvas == nullptr)    return;

    //graymap controls
    //get invert
    int invert;
    bool invertOK = Preferences::getPersistence(
            PersistentExampleFrame::group, PersistentExampleFrame::gm_invert, invert,
            canvas->getInvert(0) );
    if (invertOK) {
        cout << "invert=" << (invert!=0) << endl;
        canvas->setInvert( 0, invert!=0 );
    }
    //get level
    int level;
    bool levelOK = Preferences::getPersistence(
            PersistentExampleFrame::group, PersistentExampleFrame::gm_level, level,
            canvas->getCenter(0) );
    if (levelOK) {
        cout << "level=" << level << endl;
        canvas->setCenter(0, level);
    }
    //get width
    int width;
    bool widthOK = Preferences::getPersistence(
            PersistentExampleFrame::group, PersistentExampleFrame::gm_width, width,
            canvas->getWidth(0));
    if (widthOK) {
        cout << "width=" << width << endl;
        canvas->setWidth(0, width);
    }
    //update contrast (if necessary)
    if (levelOK || widthOK || invertOK) {
        canvas->initLUT(0);
        canvas->reload();
    }

    //get index of slice number to display
    int slice;
    bool sliceOK = Preferences::getPersistence(
            PersistentExampleFrame::group, PersistentExampleFrame::c_slice,
            slice, canvas->getSliceNo(0) );
    if (sliceOK) {
        canvas->setSliceNo( 0, slice );
    }

    //get image display position tx, ty
    int tx;
    bool txOK = Preferences::getPersistence(
            PersistentExampleFrame::group, PersistentExampleFrame::c_t_x,
            tx, canvas->mTx );
    int ty;
    bool tyOK = Preferences::getPersistence(
            PersistentExampleFrame::group, PersistentExampleFrame::c_t_y,
            ty, canvas->mTy );
    if (txOK && tyOK) {
        canvas->mTx = tx;
        canvas->mTy = ty;
        //wxObject unused;
        //canvas->OnPaint( (wxPaintEvent&) unused );  //n.g. on win
        canvas->Refresh();
    }

    //graymap controls visible (or not)?
    int graymap;
    bool graymapOK = Preferences::getPersistence(
            PersistentExampleFrame::group,
            PersistentExampleFrame::gm_visible, graymap, -1 );
    if (graymapOK) {
        if (graymap && mGrayMapControls==nullptr) {
            wxCommandEvent unused;
            OnGrayMap( unused );
        } else if (!graymap && mGrayMapControls!=nullptr) {
            wxCommandEvent unused;
            OnGrayMap( unused );
        }
    }
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void ExampleFrame::loadFile ( const char* const fname ) {
    if (fname==nullptr || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxString  tmp = wxString::Format( "File %s could not be opened.", fname );
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    ++mFileOrDataCount;
    assert( mFileOrDataCount==1 );
    wxString  tmp = wxString::Format( "%s: ", (const char *)mModuleName.c_str() );
    tmp += fname;
    //does a window with this title (file) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
            tmp = wxString::Format( "%s:%s (%d)", (const char *)mModuleName.c_str(), fname, i );
            if (!searchWindowTitles(tmp))    break;
        }
    }
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );


    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    assert( canvas != nullptr );
    cout << "loadFile before: size is now (w,h)=" << GetSize().GetWidth() << "," << GetSize().GetHeight() << endl;
    canvas->loadFile( fname );
    cout << "loadFile after: size is now (w,h)=" << GetSize().GetWidth() << "," << GetSize().GetHeight() << endl;
	if (!canvas->isLoaded(0)) {
		delete m_buttonBox;
		m_buttonBox = nullptr;
		mFileOrDataCount = 0;
		return;
	}
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
void ExampleFrame::loadData ( char* name,
    int xSize, int ySize, int zSize,
    double xSpacing, double ySpacing, double zSpacing,
    int* const data, ViewnixHeader* const vh,
    bool vh_initialized )
{
    assert( 0 );
#if 0
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp = wxString::Format( "%s: ", (const char *)mModuleName.c_str() );
    tmp += name;
    SetTitle( tmp );
    
    ExampleCanvas*  canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Previous button press.  display the previous slice.
 *  \param unused is not used.
 */
void ExampleFrame::OnPrevious ( wxCommandEvent& unused ) {
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    int  slice = canvas->getSliceNo(0) - 1;
    if (slice<0)
		slice = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Next button press.  display the next slice.
 *  \param unused is not used.
 */
void ExampleFrame::OnNext ( wxCommandEvent& unused ) {
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    int  slice = canvas->getSliceNo(0) + 1;
    if (slice >= canvas->getNoSlices(0))
		slice = 0;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 *  \param unused is not used.
 */
void ExampleFrame::OnGrayMap ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=nullptr) {
        delete mGrayMapControls;
        mGrayMapControls = nullptr;
        return;
    }
	if (mSaveScreenControls!=nullptr) {
		delete mSaveScreenControls;
		mSaveScreenControls = nullptr;
	}
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        0 );  /*,
        PersistentExampleFrame::group,  PersistentExampleFrame::level,    //for persistence
        PersistentExampleFrame::group,  PersistentExampleFrame::width,
        PersistentExampleFrame::group,  PersistentExampleFrame::invert
        ); */
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void ExampleFrame::OnCenterSlider ( wxScrollEvent& e ) {
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider (used to change contrast). */
void ExampleFrame::OnWidthSlider ( wxScrollEvent& e ) {
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ExampleFrame::OnCTLung ( wxCommandEvent& unused ) {
    auto canvas = dynamic_cast<ExampleCanvas*>(mCanvas);
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
void ExampleFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    auto canvas = dynamic_cast<ExampleCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getCTSoftTissueCenter() &&
        canvas->getWidth(0)==Preferences::getCTSoftTissueWidth()) {
        return;  //no change
    }
    canvas->setCenter( 0, Preferences::getCTSoftTissueCenter() );
	canvas->setWidth( 0, Preferences::getCTSoftTissueWidth() );
	canvas->setInvert( 0, false );
	mGrayMapControls->update_sliders( Preferences::getCTSoftTissueCenter(),
                                     Preferences::getCTSoftTissueWidth() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ExampleFrame::OnCTBone ( wxCommandEvent& unused ) {
    auto canvas = dynamic_cast<ExampleCanvas*>(mCanvas);
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
void ExampleFrame::OnPET ( wxCommandEvent& unused ) {
    auto canvas = dynamic_cast<ExampleCanvas*>(mCanvas);
    if (canvas->getCenter(0)==Preferences::getPETCenter() &&
			canvas->getWidth(0)==Preferences::getPETWidth())
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
 *  \param unused is not used.
 */
void ExampleFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& unused ) {
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
 *  \param unused is not used.
 */
void ExampleFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& unused ) {
    if (m_width->GetValue() == mCanvas->getWidth())    return;
    mCanvas->setWidth( m_width->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview.
 *  \param unused is not used.
 */
void ExampleFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    auto canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    auto preview = new wxPrintPreview(
        new MainPrint(wxString("CAVASS").c_str(), canvas), new MainPrint(wxString("CAVASS").c_str(), canvas),
        &printDialogData );
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    auto frame = new wxPreviewFrame( preview, this,
        _T("CAVASS Print Preview"),
        wxPoint(100, 100), wxSize(600, 650) );
    frame->Centre( wxBOTH );
    frame->Initialize();
    frame->Show();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set  */
void ExampleFrame::OnInvert ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    auto  canvas = dynamic_cast<ExampleCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing.
 *  \param unused is not used.
 */
void ExampleFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    auto               canvas = dynamic_cast<ExampleCanvas*>( mCanvas );
    MainPrint          printout( mModuleName.c_str(), canvas );
    if (!printer.Print(this, &printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Allow the user to scroll through the slices with the mouse wheel. */
void ExampleFrame::OnMouseWheel ( wxMouseEvent& e ) {
    const int  rot   = e.GetWheelRotation();
    wxCommandEvent  ce;
    if (rot>0)         OnPrevious(ce);
    else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ExampleFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=nullptr) {
        delete mGrayMapControls;
        mGrayMapControls = nullptr;
    }
	MainFrame::OnSaveScreen( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** must be overridden by frames that persist the sash position */
void ExampleFrame::OnMaximize ( wxMaximizeEvent& unused ) {
    //cout << "ExampleFrame::OnMaximize()" << endl;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( ExampleFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ExampleFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_GRAYMAP,        ExampleFrame::OnGrayMap       )
  EVT_BUTTON( ID_CT_LUNG,        ExampleFrame::OnCTLung        )
  EVT_BUTTON( ID_CT_SOFT_TISSUE, ExampleFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,        ExampleFrame::OnCTBone        )
  EVT_BUTTON( ID_PET,            ExampleFrame::OnPET           )
  EVT_MOUSEWHEEL( ExampleFrame::OnMouseWheel )
  EVT_BUTTON( ID_NEXT,           ExampleFrame::OnNext          )
  EVT_BUTTON( ID_PREVIOUS,       ExampleFrame::OnPrevious      )
  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, ExampleFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  ExampleFrame::OnWidthSlider  )
  EVT_CHECKBOX(       ID_INVERT,        ExampleFrame::OnInvert       )

#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_CENTER_SLIDER, ExampleFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  ExampleFrame::OnUpdateUIWidthSglider )
#endif
END_EVENT_TABLE()
//======================================================================
