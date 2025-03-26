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

#include  "cavass.h"
#include  "IRFCFrame.h"
#include  "IRFCCanvas.h"
#include  "IRFCControls.h"
#include  "CineControls.h"
#include  "CovarianceControls.h"
#include  "FunctionControls.h"
#include  "GrayMapControls.h"
#include  "HistoSettingControls.h"
#include  "SetFuzzCompIndexControls.h"


extern Vector  gFrameList;
extern int     gTimerInterval;
//----------------------------------------------------------------------
/** \brief Constructor for IRFCFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */


IRFCFrame::IRFCFrame ( bool maximize, int w, int h ) : MainFrame( 0 )
{
    mFileOrDataCount = 0;

    mForward = mForwardBackward = false;
    m_cine_timer = new wxTimer( this, ID_CINE_TIMER );

    mSetIndex1Box         = NULL;
    mSetIndex1Sizer       = NULL;
    mIRFCBox      = NULL;
    mIRFCSizer    = NULL;

    mCineControls         = NULL;
    mGrayMap1Controls     = mGrayMap2Controls  = NULL;
    mGrayMap3Controls     = mGrayMap4Controls  = NULL;
    mSaveScreenControls   = NULL;
    mSetIndex1Controls    = NULL;
    mHistoSettingControls = NULL;
    mCovarianceControls   = NULL;
    mParameterControls    = NULL;
    mFunctionControls     = NULL;
    mIRFCControls     = NULL;
	mTmpName.Empty();


    ::gFrameList.push_back( this );
    gWhere = cWhereIncr;

    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 0.0 );
    ::setColor( mSplitter );
   mSplitter->SetSashPosition( -dControlsHeight-70 );
    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -

    mCanvas = new IRFCCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
	/*----IRFC setting controls*/
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mIRFCControls = new IRFCControls( mControlPanel, mBottomSizer,
        "IRFC Setting",ID_MODE_COMBOBOX,ID_ALG_COMBOBOX,
        ID_AFFIN_COMBOBOX, ID_BRUSH_SLIDER, ID_NOBJ_SLIDER,ID_PARALLEL_INDEX,
		canvas->max_objects );
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
void IRFCFrame::initializeMenu ( void ) {
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
void IRFCFrame::addButtonBox ( void ) {
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
#if 0
    wxButton*  grayMap2 = new wxButton( mControlPanel, ID_GRAYMAP2, "GMapFeat", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap2 );
    fgs->Add( grayMap2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
#endif
    wxButton*  grayMap3 = new wxButton( mControlPanel, ID_GRAYMAP3, "GMapAffn", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( grayMap3 );
    fgs->Add( grayMap3, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1

    mApplyTrain = new wxButton( mControlPanel, ID_APPLYTRAIN, "ApplyTrain", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mApplyTrain->Enable( true);
    ::setColor( mApplyTrain );
    fgs->Add( mApplyTrain, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 4, col 2
    mClearSeed = new wxButton( mControlPanel, ID_CLEARSEED, "ClearSeed", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mClearSeed->Enable( false);
    ::setColor( mClearSeed );
    fgs->Add( mClearSeed, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 5, col 1
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 5, col 2
    mSave = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mSave );
    fgs->Add( mSave, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 1    
	mLoadSeeds = new wxButton( mControlPanel, ID_LOAD_SEEDS, "LoadSeeds", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( mLoadSeeds );
	fgs->Add( mLoadSeeds, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 2
	mLoadPar = new wxButton( mControlPanel, ID_LOAD_PAR, "LoadPar", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( mLoadPar );
	fgs->Add( mLoadPar, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 7, col 1
	mLoadModel = new wxButton( mControlPanel, ID_LOAD_MODEL, "LoadModel", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( mLoadModel );
	fgs->Add( mLoadModel, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 2

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 20 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for IRFCFrame class. */
IRFCFrame::~IRFCFrame ( void ) {
    cout << "IRFCFrame::~IRFCFrame" << endl;
    wxLogMessage( "IRFCFrame::~IRFCFrame" );

    if (m_cine_timer!=NULL)   { delete m_cine_timer;   m_cine_timer=NULL;  }    
    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }

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
 *  new IRFCFrame.  It first searches the input file history for an
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
void IRFCFrame::createIRFCFrame ( wxFrame* parentFrame, bool useHistory )
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
    IRFCFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new IRFCFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new IRFCFrame();
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
bool IRFCFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM", fn, false ))    return true;
    if (wxMatchWild( "*.IM0", fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void IRFCFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "IRFCFrame::OnChar" );
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
        cout << "IRFCFrame::OnChar: " << ::gTimerInterval << endl;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnSaveScreen ( wxCommandEvent& e ) {
    if (mSaveScreenControls!=NULL)    return;

    mSaveScreenControls = new SaveScreenControls( mControlPanel,
        mBottomSizer, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_OVERWRITE_SCREEN );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void IRFCFrame::OnCopy ( wxCommandEvent& e ) {
    wxSafeYield(NULL, true);
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->m_bitmaps[0]==NULL) {
        wxLogMessage("Nothing to copy.");
        return;
    }
    if (!wxTheClipboard->Open()) {
        wxLogError(_T("Can't open clipboard."));
        return;
    }
    wxLogMessage( _T("Creating wxBitmapDataObject...") );
    wxSafeYield(NULL, true);
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
void IRFCFrame::OnHideControls ( wxCommandEvent& e ) {
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
void IRFCFrame::OnOpen ( wxCommandEvent& e ) {
	createIRFCFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void IRFCFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "IRFCFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "IRFCFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
        OnPPScopsIRFC( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnLoadSeeds ( wxCommandEvent& unused )
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	if (canvas->m_algorithm == FCAlgm_IIRFC)
	{
		canvas->loadPoints("");
		Refresh();
		return;
	}
	wxString filename = wxFileSelector( _T("Select foreground seed file"),
		_T(""), _T(""), _T(""), "BIM files (*.BIM)|*.BIM", wxFILE_MUST_EXIST );
	if (!filename.empty())
	{
		canvas->loadPoints(filename.c_str());
	}
	filename = wxFileSelector( _T("Select background seed file"),
		_T(""), _T(""), _T(""), "BIM files (*.BIM)|*.BIM", wxFILE_MUST_EXIST );
	if (!filename.empty())
	{
		canvas->loadBgPoints(filename.c_str());
	}
	Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnLoadModel ( wxCommandEvent& unused )
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	wxString filename = wxFileSelector( _T("Select model file"),
		_T(""), _T(""), _T(""), "IM0 files (*.IM0)|*.IM0", wxFILE_MUST_EXIST );
	if (!filename.empty())
		canvas->loadModel(filename.c_str());
	Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnLoadPar ( wxCommandEvent& unused )
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	wxString filename = wxFileSelector( _T("Select parameter file"),
		_T(""), _T(""), _T(""), "def files (*.def)|*.def", wxFILE_MUST_EXIST );
	if (filename.empty())
		return;
	FILE *fp=fopen(filename.c_str(), "rb");
	int nobj;
	float lev, wid;
	if (fscanf(fp, "%d\n", &nobj) != 1)
	{
		wxMessageBox("Can't read parameter file");
		fclose(fp);
		return;
	}
	if (nobj > canvas->max_objects)
	{
		canvas->obj_level =
			(int *)realloc(canvas->obj_level, nobj*sizeof(int));
		canvas->obj_width =
			(int *)realloc(canvas->obj_width, nobj*sizeof(int));
		canvas->obj_type =
			(int *)realloc(canvas->obj_type, nobj*sizeof(int));
		canvas->max_objects = nobj;
	}
	canvas->nObj = nobj;
	if (canvas->m_obj >= nobj)
		canvas->m_obj = nobj-1;
	for (int j=0; j<nobj; j++)
	{
		if (fscanf(fp, "%f %f\n", &lev, &wid) != 2)
		{
			wxMessageBox("Can't read parameter file");
			fclose(fp);
			return;
		}
		canvas->obj_level[j] = (int)rint(lev);
		canvas->obj_width[j] = (int)rint(wid);
	}
	int fg_tissues;
	if (fscanf(fp, "%d\n", &fg_tissues) != 1)
	{
		wxMessageBox("Can't read parameter file");
		fclose(fp);
		return;
	}
	for (int j=0; j<nobj; j++)
	{
		if (fscanf(fp, " %d", canvas->obj_type+j) != 1)
		{
			wxMessageBox("Can't read parameter file");
			fclose(fp);
			return;
		}
		if (j >= fg_tissues)
			canvas->obj_type[j] += 3;
	}
	fclose(fp);
	if (canvas->m_obj >= 0)
	{
		delete mFunctionControls;
		mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
			wxString::Format("Object %d settings", canvas->m_obj+1).c_str(),
			canvas->getWeight(1),
			canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
			ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER, 0,
			ID_OBJ_TYPE_COMBO, canvas->getObjType()+1 );
	}
	canvas->function_update();
	mIRFCControls->setNObjSlider(nobj);
	mIRFCControls->setNObj(mControlPanel, ID_AFFIN_COMBOBOX, nobj,
		canvas->m_obj+1);
	canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.  one data files are required by IRFC.
 */
void IRFCFrame::loadFile ( const char* const fname ) {
    if (fname==NULL || strlen(fname)==0)    return;
    if (!wxFile::Exists(fname)) {
        wxMessageBox( "File could not be opened.", "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    _fileHistory.AddFileToHistory( fname );

    wxString  tmp("CAVASS:IRFC: ");
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
            tmp = wxString::Format( "CAVASS:IRFC:%s (%d)", fname, i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

	mFunctionControls = new FunctionControls( mControlPanel, mBottomSizer,
		wxString::Format("Object %d settings", canvas->m_obj+1).c_str(),
		canvas->getWeight(1),
		canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
		ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER, 0,
		ID_OBJ_TYPE_COMBO, 1 );
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
void IRFCFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    wxString  tmp("CAVASS:IRFC: ");
    tmp += name;
    SetTitle( tmp );
    
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnPrevious ( wxCommandEvent& unused ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) - 1;    
    if (sliceA<0)
		sliceA = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, sliceA );    
	
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->IRFC_update();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnNext ( wxCommandEvent& unused ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    int  sliceA = canvas->getSliceNo(0) + 1;
    if (sliceA>=canvas->getNoSlices(0))
        sliceA = 0;
    canvas->setSliceNo( 0, sliceA );

	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->IRFC_update();
    if (mSetIndex1Controls!=NULL)   mSetIndex1Controls->setSliceNo( sliceA+1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnSetIndex1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }    
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  } 
    if (mSetIndex1Controls!=NULL) return;

    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mSetIndex1Controls = new SetFuzzCompIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0)+1, canvas->getNoSlices(0), ID_SLICE1_SLIDER,
        ID_SCALE1_SLIDER, canvas->getScale());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnGrayMap1 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mGrayMap1Controls!=NULL)  return;

    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mGrayMap1Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map (input)", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, ID_INVERT1,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnGrayMap2 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mGrayMap2Controls!=NULL)  return;

    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mGrayMap2Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map (feature)", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_CENTER2_SLIDER, ID_WIDTH2_SLIDER, ID_INVERT2,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnGrayMap3 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mGrayMap3Controls!=NULL)  return;

    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mGrayMap3Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "Gray Map (affinity/connectivity)", canvas->getCenter(2), canvas->getWidth(2),
        canvas->getMax(2), canvas->getInvert(2),
        ID_CENTER3_SLIDER, ID_WIDTH3_SLIDER, ID_INVERT3,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnGrayMap4 ( wxCommandEvent& unused ) {
    if (mCineControls!=NULL)      { delete mCineControls;       mCineControls=NULL;      }
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mGrayMap4Controls!=NULL)  return;

    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mGrayMap4Controls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GMapcon", canvas->getCenter(2), canvas->getWidth(2),
        canvas->getMax(2), canvas->getInvert(2),
        ID_CENTER4_SLIDER, ID_WIDTH4_SLIDER, ID_INVERT4,
		wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnCine ( wxCommandEvent& unused ) {
    if (mGrayMap1Controls!=NULL)  { delete mGrayMap1Controls;   mGrayMap1Controls=NULL;  }
    if (mGrayMap2Controls!=NULL)  { delete mGrayMap2Controls;   mGrayMap2Controls=NULL;  }
    if (mGrayMap3Controls!=NULL)  { delete mGrayMap3Controls;   mGrayMap3Controls=NULL;  }
    if (mGrayMap4Controls!=NULL)  { delete mGrayMap4Controls;   mGrayMap4Controls=NULL;  }
    if (mSetIndex1Controls!=NULL) { delete mSetIndex1Controls;  mSetIndex1Controls=NULL; }
    if (mCineControls!=NULL)      return;

    mCineControls = new CineControls( mControlPanel, mBottomSizer,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CINE_SLIDER, m_cine_timer );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnReset ( wxCommandEvent& unused ) {

    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);

    m_cine_timer->Stop();
	
	
    canvas->m_tx = canvas->m_ty = 0;
    canvas->mCavassData->mDisplay = true;
	canvas->ResetTraining();
	canvas->ResetSeed();
	mClearSeed->Enable( false);
	
	canvas->connectivity_data_valid = FALSE;		 
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom in button
 *  \todo implement multi resolution sliders
 */
void IRFCFrame::OnZoomIn ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for zoom out button
 *  \todo implement multi resolution sliders
 */
void IRFCFrame::OnZoomOut ( wxCommandEvent& e ) {
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
void IRFCFrame::OnCenter1Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 2 */
void IRFCFrame::OnCenter2Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 3 */
void IRFCFrame::OnCenter3Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getCenter(2)==e.GetPosition())    return;  //no change
    canvas->setCenter( 2, e.GetPosition() );
	canvas->setCenter( 1, e.GetPosition() );
	canvas->initLUT( 1 );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 4 */
void IRFCFrame::OnCenter4Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getCenter(2)==e.GetPosition())    return;  //no change
    canvas->setCenter( 2, e.GetPosition() );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void IRFCFrame::OnWidth1Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 2 */
void IRFCFrame::OnWidth2Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT( 1 );
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 3 */
void IRFCFrame::OnWidth3Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getWidth(2)==e.GetPosition())    return;  //no change
    canvas->setWidth( 2, e.GetPosition() );
	canvas->setWidth( 1, e.GetPosition() );
	canvas->initLUT( 1 );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 4 */
void IRFCFrame::OnWidth4Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getWidth(2)==e.GetPosition())    return;  //no change
    canvas->setWidth( 2, e.GetPosition() );
	canvas->function_update();
    canvas->initLUT( 2 );
    canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set 1 */
void IRFCFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    canvas->IRFC_update();
}

/** \brief callback for scale slider for data set 1 */
void IRFCFrame::OnScale1Slider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndex1Controls->setScaleText( s );
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


/** \brief callback for Number of objects slider */
void IRFCFrame::OnNObjSlider ( wxScrollEvent& e ) 
{
    int     newNObj = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newNObj );
    mIRFCControls->setNObjText( s );

	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	if (newNObj > canvas->max_objects)
	{
		canvas->obj_level =
			(int *)realloc(canvas->obj_level, newNObj*sizeof(int));
		canvas->obj_width =
			(int *)realloc(canvas->obj_width, newNObj*sizeof(int));
		canvas->obj_type =
			(int *)realloc(canvas->obj_type, newNObj*sizeof(int));
		for (int j=canvas->max_objects; j<newNObj; j++)
		{
			canvas->obj_level[j] = 0;
			canvas->obj_width[j] = 1;
			canvas->obj_type[j] = 3;
		}
		canvas->max_objects = newNObj;
	}
	canvas->nObj = newNObj;
	if (canvas->m_obj >= newNObj)
	{
		canvas->m_obj = newNObj-1;
		delete mFunctionControls;
		mFunctionControls = canvas->m_obj<0?
			new FunctionControls( mControlPanel, mBottomSizer, "Inhomogeneity",
			canvas->getWeight(2),
			0, canvas->getWidthLevel(2), canvas->getMax(0),
			ID_WEIGHT3_SLIDER, 0, ID_WIDTHLEVEL3_SLIDER, 0):
			new FunctionControls( mControlPanel, mBottomSizer,
			wxString::Format("Object %d settings", canvas->m_obj+1).c_str(),
			canvas->getWeight(1),
			canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
			ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER, 0,
			ID_OBJ_TYPE_COMBO, canvas->getObjType()+1 );
	}
	canvas->function_update();
	mIRFCControls->setNObj(mControlPanel, ID_AFFIN_COMBOBOX, canvas->nObj,
		canvas->m_obj+1);
	canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for Brush size slider */
void IRFCFrame::OnBrushSlider ( wxScrollEvent& e ) 
{
    int     newBrush = e.GetPosition();
    const wxString  s = wxString::Format( "%1d", newBrush );
    mIRFCControls->setBrushText( s );
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setBrushSize( newBrush );    
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

/** \brief callback for Overlay checkbox for both data sets */
void IRFCFrame::OnOverlay ( wxCommandEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 1 */
void IRFCFrame::OnInvert1 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setInvert( 0, value );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 2 */
void IRFCFrame::OnInvert2 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setInvert( 1, value );
    canvas->initLUT( 1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 3 */
void IRFCFrame::OnInvert3 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setInvert( 2, value );
    canvas->initLUT( 2 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for invert checkbox for data set 4 */
void IRFCFrame::OnInvert4 ( wxCommandEvent& e ) {
    bool  value = e.IsChecked();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setInvert( 2, value );
    canvas->initLUT( 2 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine forward (only) checkbox for both data sets */
void IRFCFrame::OnCineForward ( wxCommandEvent& e ) {
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
void IRFCFrame::OnCineForwardBackward ( wxCommandEvent& e ) {
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
void IRFCFrame::OnCineTimer ( wxTimerEvent& e ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
    canvas->IRFC_update();
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
void IRFCFrame::OnCineSlider ( wxScrollEvent& e ) {
    ::gTimerInterval = mCineControls->mCineSlider->GetValue();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cine slider */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void IRFCFrame::OnUpdateUICineSlider ( wxUpdateUIEvent& e ) {
    //very important on windoze to make it a oneshot
    m_cine_timer->Start( ::gTimerInterval, true );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for busy timer */
void IRFCFrame::OnBusyTimer ( wxTimerEvent& e ) {
    cout << "OnBusyTimer" << endl;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void IRFCFrame::OnUpdateUICenter1Slider ( wxUpdateUIEvent& e ) {
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
void IRFCFrame::OnUpdateUIWidth1Slider ( wxUpdateUIEvent& e ) {
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
void IRFCFrame::OnUpdateUISlice1Slider ( wxUpdateUIEvent& e ) {
    if (m_sliceNo->GetValue() == mCanvas->getSliceNo())    return;
    mCanvas->setSliceNo( m_sliceNo->GetValue() );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
    mCanvas->IRFC_update();
	if (mSetIndex1Controls!=NULL)
	    mSetIndex1Controls->setSliceNo( m_sliceNo->GetValue() );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider for data set 1 */
#ifdef __WXX11__
//especially (only) need on X11 (w/out GTK) to get slider events
void IRFCFrame::OnUpdateUIScale1Slider ( wxUpdateUIEvent& e ) {
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
void IRFCFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
    /*wxPrintDialogData  printDialogData( *g_printData );
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
void IRFCFrame::OnPrint ( wxCommandEvent& e ) {
    /*wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
void IRFCFrame::OnAffinity ( wxCommandEvent& unused ) {

	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    int choice = mIRFCControls->getAffinity ();
	canvas->m_obj = choice-1;
	canvas->feature_selected = choice==0? 2:1;
	canvas->function_update();
	canvas->ResetTraining();
	delete mFunctionControls;
	mFunctionControls = canvas->m_obj<0?
		new FunctionControls( mControlPanel, mBottomSizer, "Inhomogeneity",
		canvas->getWeight(2),
		0, canvas->getWidthLevel(2), canvas->getMax(0),
		ID_WEIGHT3_SLIDER, 0, ID_WIDTHLEVEL3_SLIDER, 0):
		new FunctionControls( mControlPanel, mBottomSizer,
		wxString::Format("Object %d settings", canvas->m_obj+1).c_str(),
		canvas->getWeight(1),
		canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
		ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER, 0,
		ID_OBJ_TYPE_COMBO, canvas->getObjType()+1 );
	canvas->IRFC_update();
}

void IRFCFrame::OnObjType ( wxCommandEvent& unused )
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	int choice = mFunctionControls->getObjType()-1;
	canvas->setObjType(choice);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnStartSliceSlider ( wxScrollEvent& e ) 
{
  IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
  canvas->reload();
}

void IRFCFrame::OnEndSliceSlider ( wxScrollEvent& e ) 
{
  IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
  canvas->reload();
}


void IRFCFrame::OnHigh ( wxCommandEvent& e ) {
}

void IRFCFrame::OnLow ( wxCommandEvent& e ) {
}

void IRFCFrame::OnDiff ( wxCommandEvent& e ) {
}

void IRFCFrame::OnSum ( wxCommandEvent& e ) {
}

void IRFCFrame::OnRelDiff ( wxCommandEvent& e ) {
}        

void IRFCFrame::OnPararell ( wxCommandEvent& e )
{
	   bool  value = e.IsChecked();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    
    canvas->setPararell(value);  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IRFCFrame::OnHighCombo ( wxCommandEvent& unused ) {
}     


void IRFCFrame::OnLowCombo ( wxCommandEvent& unused ) {
}     


void IRFCFrame::OnDiffCombo ( wxCommandEvent& unused ) {
}     


void IRFCFrame::OnSumCombo ( wxCommandEvent& unused ) {
}     


void IRFCFrame::OnRelDiffCombo ( wxCommandEvent& unused ) {
}     


/** \brief callback for weight slider*/

void IRFCFrame::OnWeight2Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setWeight( 1, newWeight );
	if (canvas->model_filename==NULL || newWeight+canvas->weight[2]>100)
		canvas->setWeight( 2, 100-newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->IRFC_update();
}

void IRFCFrame::OnWeight3Slider ( wxScrollEvent& e ) {
    const int     newWeight = e.GetPosition();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	canvas->setWeight( 2, newWeight );
	if (canvas->model_filename==NULL || newWeight+canvas->obj_weight>100)
		canvas->setWeight( 1, 100-newWeight );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for level slider*/

void IRFCFrame::OnLevel2Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setLevel( 1, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->IRFC_update();
}

void IRFCFrame::OnLevel3Slider ( wxScrollEvent& e ) {
    const int     newLevel = e.GetPosition();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setLevel( 2, newLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/** \brief callback for widthLevel slider*/

void IRFCFrame::OnWidthLevel2Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setWidthLevel( 1, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->IRFC_update();
}

void IRFCFrame::OnWidthLevel3Slider ( wxScrollEvent& e ) {
    const int     newWidthLevel = e.GetPosition();
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    canvas->setWidthLevel( 2, newWidthLevel );
	canvas->connectivity_data_valid = FALSE;
	canvas->setCostImgDone( false );
	canvas->function_update();
	canvas->IRFC_update();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void IRFCFrame::OnCTLung ( wxCommandEvent& unused ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
void IRFCFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
void IRFCFrame::OnCTBone ( wxCommandEvent& unused ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
void IRFCFrame::OnPET ( wxCommandEvent& unused ) {
    IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
void IRFCFrame::OnProcess ( wxCommandEvent& e ) {

  IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);

  int start_slice, end_slice;
  if (mTmpName.IsEmpty())
  {
	start_slice = end_slice = canvas->getSliceNo(0);
	mTmpName =
	  Preferences::getOutputDirectory() + wxString("/FUZZ-TMP-OUT.IM0");
  }
  else if (mParameterControls)
  {
	start_slice = mParameterControls->getStartSlice()-1;
	end_slice = mParameterControls->getEndSlice()-1;
  }
  else
  {
    start_slice = 0;
	end_slice = canvas->getNoSlices(0)-1;
  }

  bool IsSeed=canvas->points_filename!=NULL;
  for (int i=0; i<canvas->nFg_seed ; i++)
	if (canvas->fg_seed[i][2]>=start_slice && canvas->fg_seed[i][2]<=end_slice)
	{
		IsSeed = true;
		break;
	}

  if (!IsSeed)
  {
    wxMessageBox("Select seed first.");
	return;
  }
  IsSeed = canvas->bg_filename!=NULL;
  for (int i=0; i<canvas->nBg_seed ; i++)
	if (canvas->bg_seed[i][2]>=start_slice && canvas->bg_seed[i][2]<=end_slice)
	{
		IsSeed = true;
		break;
	}
  if (!IsSeed)
  {
    wxMessageBox("Select background seed first.");
	return;
  }

  FILE *fp=fopen((mTmpName+".def").c_str(), "wb");
  if (fp == NULL)
  {
    wxMessageBox("Can't create object def. file.");
	return;
  }
  fprintf(fp, "%d\n", canvas->nObj);
  int fg_tissues=0;
  for (int j=0; j<canvas->nObj; j++)
    if (canvas->obj_type[j] <= 1)
	{
	  fg_tissues++;
      fprintf(fp, "%d %d\n", canvas->obj_level[j], canvas->obj_width[j]);
	}
  for (int j=0; j<canvas->nObj; j++)
    if (canvas->obj_type[j] > 1)
	  fprintf(fp, "%d %d\n", canvas->obj_level[j], canvas->obj_width[j]);
  fprintf(fp, "%d\n", fg_tissues);
  for (int j=0; j<canvas->nObj; j++)
    if (canvas->obj_type[j] <= 1)
	  fprintf(fp, " %d", canvas->obj_type[j]);
  for (int j=0; j<canvas->nObj; j++)
    if (canvas->obj_type[j] > 1)
	  fprintf(fp, " %d", canvas->obj_type[j]-3);
  fprintf(fp, "\n");
  fclose(fp);

  wxString ss_str = wxString::Format( "%d", start_slice );
  wxString es_str = wxString::Format( "%d", end_slice );

  SetStatusText("processing ...", 1);

  //construct the command string
  wxString cmd("\"");
  cmd += Preferences::getHome()+"/fuzz_track_3d\" \""+mSourceName+"\" 1 "+
    ss_str+" "+es_str+" 1 \""+mTmpName+"\" 0 0 -multitissue "+
	wxString::Format( "%1.2f \"", (float)canvas->getWeight(1)/100 )+
	mTmpName+".def\" "+wxString::Format( "-feature 2 0 %1.2f 0 %d ",
    (float)canvas->getWeight(2)/100, canvas->getWidthLevel(2));

  if (canvas->model_filename)
  {
    cmd += wxString::Format("-feature 6 0 %1.2f 1 1 -pfom \"%s\" 0 0 0 ",
	  .01*(100-canvas->getWeight(1)-canvas->getWeight(2)),
	  canvas->model_filename);
  }

  if (canvas->m_algorithm == FCAlgm_IIRFC)
    cmd += wxString("-2D_adjacency ");

  if (canvas->bg_filename)
    cmd += wxString(" -bg_points_file \"")+canvas->bg_filename+"\"";

  if (canvas->points_filename)
    cmd += wxString(" \"")+canvas->points_filename+"\"";

  for (int i=0; i<canvas->nFg_seed ; i++) //seed points indicated by user
	if (canvas->fg_seed[i][2]>=start_slice && canvas->fg_seed[i][2]<=end_slice)
	  cmd += wxString::Format( " %d %d %d" ,
	    canvas->fg_seed[i][0], canvas->fg_seed[i][1], canvas->fg_seed[i][2]);

  int nbg_seed=0;
  for (int i=0; i<canvas->nBg_seed; i++)
    if (canvas->bg_seed[i][2]>=start_slice && canvas->bg_seed[i][2]<=end_slice)
	  nbg_seed++, cmd += wxString::Format( " %d %d %d" ,
	    canvas->bg_seed[i][0], canvas->bg_seed[i][1], canvas->bg_seed[i][2]);
  if (nbg_seed)
    cmd += wxString::Format( " -bg_points %d", nbg_seed );

  if (canvas->m_algorithm == FCAlgm_MOFS)
  	cmd += wxString(" -mofs");

  wxLogMessage( "command=%s", (const char *)cmd.c_str() );
  cout << cmd << endl;

  ProcessManager  p("fuzz_track_3d processing...", (const char *)cmd.c_str() );

  if (canvas->m_algorithm != FCAlgm_IIRFC)
    canvas->loadOutData(mTmpName.c_str());
  mTmpName.Empty();
  SetStatusText("done", 1);

}


void IRFCFrame::OnSeed ( ) 
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
    mClearSeed->Enable( True);
	canvas->connectivity_data_valid = FALSE;
	canvas->IRFC_update();
}

void IRFCFrame::OffSave ( ) 
{
    mSave->Enable( false );
}


/** \brief callback for save */
void IRFCFrame::OnMode ( wxCommandEvent& e )
{
enum {TRACKING, TRAINING};
IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
  int choice = mIRFCControls->getMode ();
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
	canvas->setWhichSeed( choice? BG_SEED: FG_SEED );
	SetStatusText( "Seed",  2 );
    SetStatusText( "Erase", 3 );
    SetStatusText( "Move", 4 );
  }	

  Refresh();
  canvas->IRFC_update();
}

void IRFCFrame::OnAlgorithm( wxCommandEvent& e ) 
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	canvas->m_algorithm = mIRFCControls->getAlgorithm();
}


/** \brief callback for ClearSeed */
void IRFCFrame::OnClearSeed( wxCommandEvent& e ) 
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	canvas->ResetSeed();
	canvas->IRFC_update();
	mClearSeed->Enable( false);

}

/** \brief callback for ClearSeed */
void IRFCFrame::OnApplyTrain( wxCommandEvent& e ) 
{
	IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
	canvas->ApplyTraining();
	delete mFunctionControls;
	mFunctionControls = canvas->m_obj<0?
		new FunctionControls( mControlPanel, mBottomSizer, "Inhomogeneity",
		canvas->getWeight(2),
		0, canvas->getWidthLevel(2), canvas->getMax(0),
		ID_WEIGHT3_SLIDER, 0, ID_WIDTHLEVEL3_SLIDER, 0):
		new FunctionControls( mControlPanel, mBottomSizer,
		wxString::Format("Object %d settings", canvas->m_obj+1).c_str(),
		canvas->getWeight(1),
		canvas->getLevel(1), canvas->getWidthLevel(1), canvas->getMax(0),
		ID_WEIGHT2_SLIDER, ID_LEVEL2_SLIDER, ID_WIDTHLEVEL2_SLIDER, 0,
		ID_OBJ_TYPE_COMBO, canvas->getObjType()+1 );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for save */
void IRFCFrame::OnSave ( wxCommandEvent& e ) {
  IRFCCanvas*  canvas = dynamic_cast<IRFCCanvas*>(mCanvas);
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
IMPLEMENT_DYNAMIC_CLASS ( IRFCFrame, wxFrame )
BEGIN_EVENT_TABLE       ( IRFCFrame, wxFrame )
DefineStandardFrameCallbacks

  EVT_BUTTON( ID_PREVIOUS,       IRFCFrame::OnPrevious       )
  EVT_BUTTON( ID_NEXT,           IRFCFrame::OnNext           )
  EVT_BUTTON( ID_SET_INDEX1,     IRFCFrame::OnSetIndex1      )
  EVT_BUTTON( ID_GRAYMAP1,       IRFCFrame::OnGrayMap1       )
  EVT_BUTTON( ID_GRAYMAP2,       IRFCFrame::OnGrayMap2       )
  EVT_BUTTON( ID_GRAYMAP3,       IRFCFrame::OnGrayMap3       )
  EVT_BUTTON( ID_GRAYMAP4,       IRFCFrame::OnGrayMap4       )
  EVT_BUTTON( ID_CINE,           IRFCFrame::OnCine           )
  EVT_BUTTON( ID_RESET,          IRFCFrame::OnReset          )
  EVT_BUTTON( ID_SAVE,           IRFCFrame::OnSave           )
  EVT_BUTTON( ID_LOAD_SEEDS,     IRFCFrame::OnLoadSeeds      )
  EVT_BUTTON( ID_LOAD_MODEL,     IRFCFrame::OnLoadModel      )
  EVT_BUTTON( ID_LOAD_PAR,       IRFCFrame::OnLoadPar        )
  EVT_BUTTON( ID_CLEARSEED,      IRFCFrame::OnClearSeed      )
  EVT_BUTTON( ID_APPLYTRAIN,     IRFCFrame::OnApplyTrain      )

  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER, IRFCFrame::OnCenter1Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,  IRFCFrame::OnWidth1Slider  )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER,  IRFCFrame::OnSlice1Slider  )
  EVT_COMMAND_SCROLL( ID_SCALE1_SLIDER,  IRFCFrame::OnScale1Slider  )
  EVT_CHECKBOX(       ID_INVERT1,        IRFCFrame::OnInvert1       )
  

  EVT_COMMAND_SCROLL( ID_CENTER2_SLIDER, IRFCFrame::OnCenter2Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH2_SLIDER,  IRFCFrame::OnWidth2Slider  )
  EVT_CHECKBOX(       ID_INVERT2,        IRFCFrame::OnInvert2       )
  
  EVT_COMMAND_SCROLL( ID_CENTER3_SLIDER, IRFCFrame::OnCenter3Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH3_SLIDER,  IRFCFrame::OnWidth3Slider  )
  EVT_CHECKBOX(       ID_INVERT3,        IRFCFrame::OnInvert3       )
  
  EVT_COMMAND_SCROLL( ID_CENTER4_SLIDER, IRFCFrame::OnCenter4Slider )
  EVT_COMMAND_SCROLL( ID_WIDTH4_SLIDER,  IRFCFrame::OnWidth4Slider  )
  EVT_CHECKBOX(       ID_INVERT4,        IRFCFrame::OnInvert4       )
    
  EVT_COMBOBOX( ID_MODE_COMBOBOX,        IRFCFrame::OnMode          )
  EVT_COMBOBOX( ID_ALG_COMBOBOX,        IRFCFrame::OnAlgorithm          )
  
  
  EVT_COMMAND_SCROLL( ID_NOBJ_SLIDER,   IRFCFrame::OnNObjSlider    )
  EVT_COMMAND_SCROLL( ID_BRUSH_SLIDER,   IRFCFrame::OnBrushSlider    )
  EVT_COMBOBOX( ID_AFFIN_COMBOBOX,       IRFCFrame::OnAffinity       )

  EVT_COMBOBOX( ID_OBJ_TYPE_COMBO,       IRFCFrame::OnObjType )

  EVT_CHECKBOX( ID_HIGH,                 IRFCFrame::OnHigh     )
  EVT_COMBOBOX( ID_HIGH_COMBO,           IRFCFrame::OnHighCombo      )
  
  EVT_CHECKBOX( ID_LOW,                 IRFCFrame::OnLow    )
  EVT_COMBOBOX( ID_LOW_COMBO,           IRFCFrame::OnLowCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT2_SLIDER,  IRFCFrame::OnWeight2Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL2_SLIDER,   IRFCFrame::OnLevel2Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL2_SLIDER,  IRFCFrame::OnWidthLevel2Slider  )
  
  EVT_CHECKBOX( ID_DIFF,                 IRFCFrame::OnDiff     )
  EVT_COMBOBOX( ID_DIFF_COMBO,           IRFCFrame::OnDiffCombo      )

  EVT_CHECKBOX( ID_SUM,                 IRFCFrame::OnSum     )
  EVT_COMBOBOX( ID_SUM_COMBO,           IRFCFrame::OnSumCombo      )
  EVT_COMMAND_SCROLL( ID_WEIGHT3_SLIDER,  IRFCFrame::OnWeight3Slider  )
  EVT_COMMAND_SCROLL( ID_LEVEL3_SLIDER,   IRFCFrame::OnLevel3Slider  )
  EVT_COMMAND_SCROLL( ID_WIDTHLEVEL3_SLIDER,  IRFCFrame::OnWidthLevel3Slider  )

  EVT_BUTTON( ID_CT_LUNG,          IRFCFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   IRFCFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          IRFCFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              IRFCFrame::OnPET     )

  EVT_CHECKBOX( ID_RELDIFF,                 IRFCFrame::OnRelDiff     )
  EVT_COMBOBOX( ID_RELDIFF_COMBO,           IRFCFrame::OnRelDiffCombo      )
  
  EVT_COMMAND_SCROLL( ID_STARTSLICE_SLIDER,  IRFCFrame::OnStartSliceSlider )
  EVT_COMMAND_SCROLL( ID_ENDSLICE_SLIDER,   IRFCFrame::OnEndSliceSlider )

#ifdef __WXX11__
  EVT_UPDATE_UI( ID_CENTER1_SLIDER, IRFCFrame::OnUpdateUICenter1Slider )
  EVT_UPDATE_UI( ID_WIDTH1_SLIDER,  IRFCFrame::OnUpdateUIWidth1Sglider )
  EVT_UPDATE_UI( ID_SLICE1_SLIDER,  IRFCFrame::OnUpdateUISlice1Slider  )
  EVT_UPDATE_UI( ID_SCALE1_SLIDER,  IRFCFrame::OnUpdateUIScale1Slider  )
  
  EVT_UPDATE_UI( ID_CENTER2_SLIDER, IRFCFrame::OnUpdateUICenter2Slider )
  EVT_UPDATE_UI( ID_WIDTH2_SLIDER,  IRFCFrame::OnUpdateUIWidth2Sglider )
    

  EVT_UPDATE_UI( ID_CINE_SLIDER,    IRFCFrame::OnUpdateUICineSlider    )
#endif

  EVT_SIZE(  MainFrame::OnSize  )
  EVT_CLOSE( MainFrame::OnCloseEvent )

  EVT_TIMER( ID_BUSY_TIMER, IRFCFrame::OnBusyTimer )

  EVT_CHAR( IRFCFrame::OnChar )
  EVT_CHECKBOX( ID_OVERLAY,   IRFCFrame::OnOverlay )

  EVT_TIMER(          ID_CINE_TIMER,            IRFCFrame::OnCineTimer )
  EVT_CHECKBOX(       ID_CINE_FORWARD,          IRFCFrame::OnCineForward )
  EVT_CHECKBOX(       ID_CINE_FORWARD_BACKWARD, IRFCFrame::OnCineForwardBackward )
  EVT_COMMAND_SCROLL( ID_CINE_SLIDER,           IRFCFrame::OnCineSlider )
END_EVENT_TABLE()

wxFileHistory  IRFCFrame::_fileHistory;
//======================================================================
