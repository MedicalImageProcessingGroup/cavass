/*
  Copyright 1993-2018 Medical Image Processing Group
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
 * \file   ExportFrame.cpp
 * \brief  ExportFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "port_data/from_dicom.h"
#include  <sys/types.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include  <unistd.h>
#else
extern "C" {
	int getpid();
}
#endif
#include  <time.h>

extern Vector  gFrameList;


int get_element_from(const char dir[], const char filename[],
    unsigned short group, unsigned short element, int type, void *result,
    unsigned maxlen /* bytes at result */, int *items_read);

int get_num_of_structures(const char filename[])
{
    FILE *infile;
    ViewnixHeader file_header;
    int num_of_structures, error_code;
    char bad_group[5], bad_element[5];

    if ((infile=fopen(filename, "rb"))==NULL)
    {
        fprintf(stderr, "Could not open %s\n", filename);
        return (0);
    }
    error_code = VReadHeader(infile, &file_header, bad_group, bad_element);
    switch (error_code)
    {   case 0:
        case 107:
        case 106:
            break;
        default:
            fprintf(stderr, "Group %s element %s undefined in VReadHeader\n",
                bad_group, bad_element);
            fclose(infile);
            return (0);
    }
    fclose(infile);
    if (file_header.str.num_of_structures_valid)
        num_of_structures = file_header.str.num_of_structures;
    else
        num_of_structures = 0;
    destroy_file_header(&file_header);
    return (num_of_structures);
}


//----------------------------------------------------------------------
/** \brief Constructor for ExportFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
ExportFrame::ExportFrame ( bool maximize, int w, int h, int frame_type )
    : MainFrame( 0 )
{
    m_frame_type = frame_type;

    //init the types of input files that this app can accept
    switch (frame_type)
    {
        case EXPORT_DICOM:
            mFileNameFilter = (char *)"Scene files (*.BIM;*.IM0)|*.BIM;*.IM0|Display files (*.MV0)|*.MV0";
            mModuleName  = "CAVASS:Export DICOM";
            break;
        case EXPORT_MATHEMATICA:
            mFileNameFilter = (char *)"Scene files (*.BIM;*.IM0)|*.BIM;*.IM0";
            mModuleName  = "CAVASS:Export Mathematica";
            break;
        case EXPORT_MATLAB:
            mFileNameFilter = (char *)"Scene files (*.BIM;*.IM0)|*.BIM;*.IM0";
            mModuleName  = "CAVASS:Export Matlab";
            break;
        case EXPORT_PGM:
            mFileNameFilter = (char *)"Scene files (*.BIM;*.IM0)|*.BIM;*.IM0|Display files (*.MV0)|*.MV0";
            mModuleName  = "CAVASS:Export PGM";
            break;
        case EXPORT_R:
            mFileNameFilter = (char *)"Scene files (*.BIM;*.IM0)|*.BIM;*.IM0";
            mModuleName  = "CAVASS:Export R";
            break;
        case EXPORT_STL:
            mFileNameFilter = (char *)"Surface files  (*.BS2)|*.BS2";
            mModuleName  = "CAVASS:Export STL";
            break;
        case EXPORT_VTK:
            mFileNameFilter = (char *)"Scene files (*.BIM;*.IM0)|*.BIM;*.IM0";
            mModuleName  = "CAVASS:Export VTK";
            break;
		default:
			assert(0);
    }
    mFileOrDataCount    = 0;
    mGrayMapControls    = NULL;
    mSetIndexControls   = NULL;
    mSetRangeControls   = NULL;
	mSetUIDControls     = NULL;
	m_suid              = NULL;
	mSeriesUID          = wxEmptyString;
    mInputBS2           = "";
    output_data_type    = OUTPUT_TYPE_PNM;
    m_outputTypeBut     = NULL;
    outputTypeName[OUTPUT_TYPE_PNM] = _T("Output: PPM");
    outputTypeName[OUTPUT_TYPE_TIFF] = _T("Output: TIFF");
    m_structN           = NULL;
    structure_number    = 1;

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
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via ExportCanvas
    mCanvas = tCanvas = new ExportCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    mSplitter->SetSashPosition( -dControlsHeight );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    if (frame_type != EXPORT_STL)
    {
        SetStatusText( "Move",     2 );
        SetStatusText( "Previous", 3 );
        SetStatusText( "Next",     4 );
    }
    wxToolTip::Enable( Preferences::getShowToolTips() );
    mSplitter->SetSashPosition( -dControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }
}

//----------------------------------------------------------------------
void ExportStlFrame::OnInformation ( wxCommandEvent& unused ) {
	wxArrayString files;
    files.Add( mInputBS2 );
    InformationDialog  id( files );
    id.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ExportStlFrame::ExportStlFrame ( bool maximize, int w, int h )
		: ExportFrame ( maximize, w, h, EXPORT_STL )
{
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones).
 *  this function does not any additional, custom menu items (but could
 *  if necessary).
 */
void ExportFrame::initializeMenu ( void ) {
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
    j = searchWindowTitles( tmp );
	if (j)
	{
	  fprintf(stderr, "Window %s alredy exists!\n", (const char *)tmp.c_str());
	}
	assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );

    //enable the Open menu item
    wxMenuItem*  op = mFileMenu->FindItem( ID_OPEN );
    op->Enable( true );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief add the button box that appears on the lower right. */
void ExportFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    m_fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    if (m_frame_type != EXPORT_STL)
    {
        //row 1, col 1
        m_prev = new wxButton( mControlPanel, ID_PREVIOUS, "Previous", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_prev );
        m_fgs->Add(m_prev,0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
        //row 1, col 2
        m_next = new wxButton( mControlPanel, ID_NEXT, "Next",
            wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_next );
        m_fgs->Add( m_next, 0,
            wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
        //row 2, col 1
        m_setIndex = new wxButton( mControlPanel, ID_SET_INDEX, "SetIndex",
            wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_setIndex );
        m_fgs->Add( m_setIndex, 0,
            wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
        //row 2, col 2
        m_grayMap = new wxButton( mControlPanel, ID_GRAYMAP,
            "GrayMap", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_grayMap );
        if (mCanvas->mCavassData==NULL ||
                !mCanvas->mCavassData->m_vh_initialized ||
                mCanvas->mCavassData->m_vh.gen.data_type == MOVIE0)
            m_grayMap->Enable(false);
        m_fgs->Add( m_grayMap, 0,
            wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
        //row 3, col 1
        m_setRange = new wxButton( mControlPanel, ID_SET_RANGE,
            "Range", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_setRange );
        m_fgs->Add( m_setRange, 0,
            wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
        //row 3, col 2
        if (m_frame_type == EXPORT_PGM)
        {
            m_outputTypeBut = new wxButton( mControlPanel, ID_OUT_TYPE,
                outputTypeName[output_data_type], wxDefaultPosition,
                wxSize(buttonWidth,buttonHeight) );
            if (mCanvas->mCavassData && mCanvas->mCavassData->m_vh_initialized
                    && mCanvas->mCavassData->m_vh.gen.data_type==IMAGE0)
            {
                m_outputTypeBut->Enable(false); //@ for now
                m_outputTypeBut->SetLabel(mCanvas->mCavassData->isBinary()?
                    "Output: PBM": "Output: PGM");
            }
            ::setColor( m_outputTypeBut );
            m_fgs->Add( m_outputTypeBut, 0,
                wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
        }
        else
        {
		    m_outputTypeBut = NULL;
			if (m_frame_type == EXPORT_DICOM)
			{
				m_suid = new wxButton( mControlPanel, ID_SERIES_UID,
					"Series UID", wxDefaultPosition,
					wxSize(buttonWidth,buttonHeight) );
				::setColor( m_suid );
				m_fgs->Add( m_suid, 0,
					wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
			}
        }
		m_structN = NULL;
    }
    else
    {
        m_prev = NULL;
        m_next = NULL;
        m_grayMap = NULL;
        m_setIndex = NULL;
        m_setRange = NULL;
        m_outputTypeBut = NULL;
        m_structN = new wxButton( mControlPanel, ID_STRUCT_NUMBER,
            wxString::Format("Struct. num.: %d",structure_number),
            wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_structN );
        m_fgs->Add( m_structN, 0,
            wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    }
    m_save = new wxButton( mControlPanel, ID_SAVE, "Save",
        wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_save );
    m_fgs->Add( m_save, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    buttonSizer->Add( m_fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ExportFrame class. */
ExportFrame::~ExportFrame ( void ) {
    cout << "ExportFrame::~ExportFrame" << endl;
    wxLogMessage( "ExportFrame::~ExportFrame" );

    if (mGrayMapControls)
    {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
    if (mSetIndexControls)
    {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
    if (mSetRangeControls)
    {
        delete mSetRangeControls;
        mSetRangeControls = NULL;
    }
    if (m_outputTypeBut!=NULL) {
        m_fgs->Detach(m_outputTypeBut);
        delete m_outputTypeBut;
    }
    if (m_structN!=NULL) {
        m_fgs->Detach(m_structN);
        delete m_structN;
    }
    if (m_save!=NULL) {
        m_fgs->Detach(m_save);
        delete m_save;
    }
    if (m_setRange!=NULL) {
        m_fgs->Detach(m_setRange);
        delete m_setRange;
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
    m_structN = NULL;
    m_outputTypeBut = NULL;
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
    //if we are in the mode that supports having more than one window open
    // at a time, we need to remove this window from the list.
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
            if (mCanvas!=NULL)  {  delete mCanvas;  mCanvas=NULL;  }
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
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new ExportFrame.  It first searches the input file history for an
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
void ExportFrame::createExportFrame ( wxFrame* parentFrame, bool useHistory,
    int frame_type )
{
    wxString  filename="";
    if (useHistory) {
        //search the input history for an acceptable input file
        const int  n = ::gInputFileHistory.GetNoHistoryFiles();
        for (int i=0; i<n; i++) {
            wxString  s = ::gInputFileHistory.GetHistoryFile( i );
            if (::gInputFileHistory.IsSelected( i ) && (
				frame_type==EXPORT_STL? ExportStlFrame::match(s):match(s))) {
                filename = s;
                break;
            }
        }
    }

    //alert the user that the file no longer exists
    if (filename.Length()>0 && !wxFile::Exists(filename)) {
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
            frame_type==EXPORT_STL? "Surface files  (*.BS2)|*.BS2":
            "Scene files (*.BIM;*.IM0)|*.BIM;*.IM0|Display files (*.MV0)|*.MV0"
            , wxFILE_MUST_EXIST );
    }
    
    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );

    //display an export frame using the specified file as input
    if (frame_type == EXPORT_STL)
    {
        ExportStlFrame*  frame;
        if (parentFrame) {
            int  w, h;
            parentFrame->GetSize( &w, &h );
            frame = new ExportStlFrame( parentFrame->IsMaximized(), w, h );
        } else {
            frame = new ExportStlFrame( false, 800, 600 );
        }

        frame->number_of_structures = get_num_of_structures((const char *)filename.c_str());
        frame->mInputBS2 = filename;
		frame->loadFile( filename.c_str() );
    }
    else
    {
        ExportFrame*  frame;
        if (parentFrame) {
            int  w, h;
            parentFrame->GetSize( &w, &h );
            frame = new ExportFrame( parentFrame->IsMaximized(), w, h,
                frame_type );
        } else {
            frame = new ExportFrame( false, 800, 600, frame_type );
        }
		frame->mInputBS2 = "";
        frame->loadFile( filename.c_str() );
    }
    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This function determines if the given filename is of a type
 *  that can be read by this module/app.
 *
 *  Supported file extensions: BIM, BMP, DCM, DICOM, GIF, IM0, JPG, JPEG,
 *  PCX, TIF, TIFF, and VTK.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool ExportFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;
    if (wxMatchWild( "*.MV0",   fn, false ))    return true;
    return false;
}
bool ExportStlFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    return wxMatchWild( "*.BS2",   fn, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item. */
void ExportStlFrame::OnOpen ( wxCommandEvent& unused ) {
    createExportFrame( this, false, EXPORT_STL );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ExportStlFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "ExportStlFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "ExportStlFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK) {
        createExportFrame( this, true, EXPORT_STL );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item. */
void ExportFrame::OnOpen ( wxCommandEvent& unused ) {
    createExportFrame( this, false, m_frame_type );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ExportFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "ExportFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "ExportFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK) {
        createExportFrame( this, true, m_frame_type );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void ExportFrame::loadFile ( const char* const fname ) {
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
    unsigned short s;
    int ii;
    wxFileName wxfn(fname);
    if (m_frame_type == EXPORT_STL)
    {
        number_of_structures = get_num_of_structures(fname);
        return;
    }
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    // set number_of_structures
    number_of_structures = 4;
    if (!canvas->mCavassData->m_vh_initialized)
    {
        canvas->mCavassData->m_vh.dsp.num_of_images = 100;
        if (get_element_from((const char *)wxfn.GetPath().c_str(),
                (const char *)wxfn.GetFullName().c_str(),0x2d,0x8090, BI, &s, 2, &ii)==0)
            canvas->mCavassData->m_vh.dsp.num_of_images = s;
    }

    structure_number = 1;
    first_sl = 0;
    if (canvas->mCavassData->m_vh_initialized)
        last_sl = canvas->getNoSlices(0)-1;
    else
        last_sl = canvas->mCavassData->m_vh.dsp.num_of_images-1;

    if (m_grayMap)
        m_grayMap->Enable(mCanvas->mCavassData &&
            mCanvas->mCavassData->m_vh_initialized &&
            mCanvas->mCavassData->m_vh.gen.data_type!=MOVIE0);
    if (m_outputTypeBut && mCanvas->mCavassData)
    {
        if (mCanvas->mCavassData->m_vh_initialized &&
                mCanvas->mCavassData->m_vh.gen.data_type==IMAGE0)
        {
            m_outputTypeBut->Enable(false); //@ for now
            m_outputTypeBut->SetLabel(mCanvas->mCavassData->
                isBinary()? "Output: PBM": "Output: PGM");
        }
        else
        {
            m_outputTypeBut->Enable(true);
            m_outputTypeBut->SetLabel(
                outputTypeName[output_data_type]);
        }
    }

    const wxString  ss = wxString::Format( "file %s", fname );
    SetStatusText( ss, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load (and subsequently display) data directly from memory
 *  (instead of from a file).
 *  \todo this is incomplete.
 */
void ExportFrame::loadData ( char* name,
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
    
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );

    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Previous button press.  display the previous slice.
 */
void ExportFrame::OnPrevious ( wxCommandEvent& unused ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) - 1;
    if (slice<0)
        slice = canvas->getNoSlices(0) - 1;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Next button press.  display the next slice. */
void ExportFrame::OnNext ( wxCommandEvent& unused ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) + 1;
    if (slice >= canvas->getNoSlices(0))
        slice = 0;
    canvas->setSliceNo( 0, slice );
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 */
void ExportFrame::OnGrayMap ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
        return;
    }
    if (mSetIndexControls)
    {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
    if (mSetRangeControls)
    {
        delete mSetRangeControls;
        mSetRangeControls = NULL;
    }
	if (mSetUIDControls!=NULL)
	{
		delete mSetUIDControls;
		mSetUIDControls = NULL;
	}

    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", tCanvas->getCenter(0), tCanvas->getWidth(0),
        tCanvas->getMax(0), tCanvas->getInvert(0),
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, wxID_ANY /*ID_INVERT*/,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void ExportFrame::OnCenterSlider ( wxScrollEvent& e ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider (used to change contrast). */
void ExportFrame::OnWidthSlider ( wxScrollEvent& e ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT( 0 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ExportFrame::OnCTLung ( wxCommandEvent& unused ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
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
void ExportFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
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
void ExportFrame::OnCTBone ( wxCommandEvent& unused ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
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
void ExportFrame::OnPET ( wxCommandEvent& unused ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
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
/** \brief callback for scale slider for data set  */
void ExportFrame::OnScaleSlider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue/100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndexControls->setScaleText( s );
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    canvas->setScale( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for overlay checkbox for both data sets */
void ExportFrame::OnOverlay ( wxCommandEvent& e ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    canvas->setOverlay( e.IsChecked() );
    canvas->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slide slider for data set  */
void ExportFrame::OnSliceSlider ( wxScrollEvent& e ) {
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition()-1)    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slider for first slice */
void ExportFrame::OnSlice1Slider ( wxScrollEvent& e ) {
    first_sl = e.GetPosition()-1;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slider for last slice */
void ExportFrame::OnSlice2Slider ( wxScrollEvent& e ) {
    last_sl = e.GetPosition()-1;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
/** \brief callback for center slider (used to change contrast).
 *  especially (only) need on X11 (w/out GTK) to get slider events.
 */
void ExportFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& unused ) {
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
void ExportFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& unused ) {
    if (m_width->GetValue() == mCanvas->getWidth())    return;
    mCanvas->setWidth( m_width->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void ExportFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
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
void ExportFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ExportCanvas*     canvas = dynamic_cast<ExportCanvas*>(mCanvas);
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
void ExportFrame::OnMouseWheel ( wxMouseEvent& e ) {
    const int  rot   = e.GetWheelRotation();
    wxCommandEvent  ce;
    if (rot>0)         OnPrevious(ce);
    else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ExportFrame::OnSetIndex ( wxCommandEvent& unused ) {
    if (mSetIndexControls!=NULL) {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
        return;
    }
    if (mGrayMapControls)
    {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
    if (mSetRangeControls)
    {
        delete mSetRangeControls;
        mSetRangeControls = NULL;
    }
	if (mSetUIDControls!=NULL)
	{
		delete mSetUIDControls;
		mSetUIDControls = NULL;
	}

    ExportCanvas*  canvas = dynamic_cast<ExportCanvas*>(mCanvas);
    mSetIndexControls = new SetIndexControls( mControlPanel,
        mBottomSizer, "SetIndex", canvas->getSliceNo(0),
        canvas->getNoSlices(0), ID_SLICE_SLIDER, ID_SCALE_SLIDER,
        canvas->getScale(), ID_OVERLAY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ExportFrame::OnSetRange ( wxCommandEvent& unused ) {
    if (mSetRangeControls!=NULL) {
        delete mSetRangeControls;
        mSetRangeControls = NULL;
        return;
    }
    if (mGrayMapControls)
    {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
    if (mSetIndexControls)
    {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
	if (mSetUIDControls!=NULL)
	{
		delete mSetUIDControls;
		mSetUIDControls = NULL;
	}

    int nslices;
    nslices = tCanvas->getNoSlices(0);
    mSetRangeControls= new SetExportRangeControls( mControlPanel, mBottomSizer,
        "Slice range", first_sl+1, last_sl+1, nslices,
        ID_SLICE1_SLIDER, ID_SLICE2_SLIDER );
}

void ExportFrame::OnSeriesUID( wxCommandEvent& unused )
{
	if (mSetUIDControls!=NULL)
	{
		delete mSetUIDControls;
		mSetUIDControls = NULL;
		return;
	}
    if (mGrayMapControls)
    {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
    if (mSetIndexControls)
    {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
    if (mSetRangeControls)
    {
        delete mSetRangeControls;
        mSetRangeControls = NULL;
    }
	if (mSeriesUID == wxEmptyString)
		mSeriesUID = wxString::Format("1.2.276.0.7230010.3.1.4.%d.%ld",
			getpid(), time(NULL));
	mSetUIDControls = new SetUIDControls( mControlPanel, mBottomSizer,
		"Series Instance UID", mSeriesUID, ID_SERIES_UID_TABLET );
}

void ExportFrame::OnSeriesUIDTablet( wxCommandEvent& e )
{
	mSeriesUID = e.GetString();
}

/** \brief callback for  outputType button */
// 1/16/08 Dewey Odhner
void ExportFrame::OnOutputType ( wxCommandEvent& e ) {

    if (output_data_type < 1)
        output_data_type++;
    else
        output_data_type = 0;

    m_outputTypeBut->SetLabel(outputTypeName[output_data_type]);
}

void ExportFrame::OnStructNumber( wxCommandEvent& unused )
{
    if (structure_number < number_of_structures)
        structure_number++;
    else
        structure_number = 1;
    m_structN->SetLabel(wxString::Format("Struct. num.: %d",structure_number));
}

void ExportFrame::OnExportSave( wxCommandEvent& unused )
{
    if (m_frame_type==EXPORT_STL? mInputBS2=="":
            mCanvas->mCavassData==NULL||mCanvas->mCavassData->m_fname==NULL||
            strlen(mCanvas->mCavassData->m_fname)==0)
    {
        wxMessageBox("Select input first.");
        return;
    }
    wxString defaultFile;
    wxFileDialog *saveDlg=NULL;
    wxString cmd;
    switch (m_frame_type)
    {
        case EXPORT_DICOM:
            defaultFile = "expdcm-tmp";
            saveDlg = new wxFileDialog(this,
                _T("Convert to DICOM"),
                wxEmptyString,
                defaultFile,
                _T("DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom"),
                wxSAVE | wxOVERWRITE_PROMPT );
            if (saveDlg->ShowModal() == wxID_OK)
            {
                cmd = "mipg2dicom\" \"";
                cmd += mCanvas->mCavassData->m_fname;
                cmd += "\" \"";
                cmd += saveDlg->GetPath();
                cmd += wxString::Format("\" 0 %d %d ", first_sl, last_sl);
				if (mSeriesUID != wxEmptyString)
					cmd += wxString("-SeriesInstanceUID=")+mSeriesUID;
	            wxLogMessage( cmd );
                ProcessManager( "Converting to DICOM.", _T("\"")+Preferences::getHome()+
                    "/"+cmd, true );			
                mSeriesUID = wxEmptyString;
				delete mSetUIDControls;
				mSetUIDControls = NULL;
            }
            break;
        case EXPORT_PGM:
            defaultFile = "exppgm-tmp";
            switch (output_data_type)
            {
                case OUTPUT_TYPE_PNM:
                    if (!mCanvas->mCavassData->m_vh_initialized ||
                            mCanvas->mCavassData->m_vh.gen.data_type!=IMAGE0)
                    {
                        saveDlg = new wxFileDialog(this, _T("Convert to PPM"),
                            wxEmptyString, defaultFile,
                            _T("PPM files (*.PPM;*.ppm)|*.PPM;*.ppm"), wxSAVE | wxOVERWRITE_PROMPT );
                        if (saveDlg->ShowModal() == wxID_OK)
                        {
                            cmd = "MV0_to_ppm\" \"";
                            cmd += mCanvas->mCavassData->m_fname;
                            cmd += wxString::Format(
                                "\" %d %d \"", first_sl, last_sl);
                            cmd += saveDlg->GetPath();
                            cmd+="\" 0"; //@ To do: Let user specify fg/bg flag
				            wxLogMessage( cmd );
                            ProcessManager( "Converting to PPM.",
                                _T("\"")+Preferences::getHome()+
                                "/"+cmd, true );
                        }
                    }
                    else
                    {
                        if (mCanvas->mCavassData->isBinary())
                            saveDlg =
                                new wxFileDialog(this, _T("Convert to PBM"),
                                wxEmptyString, defaultFile,
                                _T("PBM files (*.PBM;*.pbm)|*.PBM;*.pbm"),
                                wxSAVE | wxOVERWRITE_PROMPT );
                        else
                            saveDlg =
                                new wxFileDialog(this, _T("Convert to PGM"),
                                wxEmptyString, defaultFile,
                                _T("PGM files (*.PGM;*.pgm)|*.PGM;*.pgm"),
                                wxSAVE | wxOVERWRITE_PROMPT );
                        if (saveDlg->ShowModal() == wxID_OK)
                        {
                            cmd = "IM0_to_pgm\" \"";
                            cmd += mCanvas->mCavassData->m_fname;
                            cmd += wxString::Format(
                                "\" %d %d \"", first_sl, last_sl);
                            cmd += saveDlg->GetPath();
                            cmd += '"';
				            wxLogMessage( cmd );
                            ProcessManager( "Converting to PGM.",
                                _T("\"")+Preferences::getHome()+
                                "/"+cmd, true );
                        }
                    }
                    break;
                case OUTPUT_TYPE_TIFF:
                    if (mCanvas->mCavassData->m_vh_initialized &&
                            mCanvas->mCavassData->m_vh.gen.data_type==IMAGE0)
                    {
                        wxMessageBox("Not yet implemented.");
                    }
                    else
                    {
                        saveDlg = new wxFileDialog(this, _T("Convert to TIFF"),
                            wxEmptyString, defaultFile,
                            _T("TIFF files (*.TIF;*.TIFF;*.tif;*.tiff)|*.TIF;*.TIFF;*.tif;*.tiff"),
                            wxSAVE | wxOVERWRITE_PROMPT );
                        if (saveDlg->ShowModal() == wxID_OK)
                        {
                            cmd = "MV0_to_tiff\" \"";
                            cmd += mCanvas->mCavassData->m_fname;
                            cmd += wxString::Format(
                                "\" %d %d \"", first_sl, last_sl);
                            cmd += saveDlg->GetPath();
                            cmd += '"';
				            wxLogMessage( cmd );

                ProcessManager( "Converting to TIFF.", _T("\"")+
                    Preferences::getHome()+"/"+cmd, true );
                        }
                    }
                    break;
            }
            break;
        case EXPORT_STL:
            defaultFile = "expstl-tmp.stl";
            saveDlg = new wxFileDialog(this,
                _T("Convert to STL"),
                wxEmptyString,
                defaultFile,
                _T("STL files (*.STL;*.stl)|*.STL;*.stl"),
                wxSAVE | wxOVERWRITE_PROMPT );
            if (saveDlg->ShowModal() == wxID_OK)
            {
                cmd = "from_BS2\" \"";
                cmd += mInputBS2;
                cmd += wxString::Format("\" %d \"", structure_number);
                cmd += saveDlg->GetPath();
                cmd += "\" 0"; //@ To do: Let user specify fg/bg flag
	            wxLogMessage( cmd );
                ProcessManager( "Converting to STL.", _T("\"")+
                    Preferences::getHome()+"/"+cmd, true );
            }
            break;

        case EXPORT_MATHEMATICA :
            defaultFile = "expmth-tmp.m";
            saveDlg = new wxFileDialog( this, _T("Convert to Mathematica"),
                wxEmptyString, defaultFile,
                _T("Mathematica files (*.M;*.m)|*.M;*.m"), wxSAVE | wxOVERWRITE_PROMPT );
            if (saveDlg->ShowModal() != wxID_OK)    break;
            cmd = "exportMath\" \"";
            cmd += mCanvas->mCavassData->m_fname;
            cmd += "\" mathematica \"";
            cmd += saveDlg->GetPath();
            cmd += wxString::Format( "\" %d %d", first_sl, last_sl );
            wxLogMessage( cmd );
            ProcessManager( "Converting to Mathematica.",
			    _T("\"")+Preferences::getHome()+"/"+cmd, true );
            break;
        case EXPORT_MATLAB :
            defaultFile = "expmth-tmp.mat";
            saveDlg = new wxFileDialog( this, _T("Convert to MatLab"),
                wxEmptyString, defaultFile,
                _T("MatLab files (*.MAT;*.mat)|*.MAT;*.mat"), wxSAVE | wxOVERWRITE_PROMPT );
            if (saveDlg->ShowModal() != wxID_OK)    break;
            cmd = "exportMath\" \"";
            cmd += mCanvas->mCavassData->m_fname;
            cmd += "\" matlab \"";
            cmd += saveDlg->GetPath();
            cmd += wxString::Format( "\" %d %d", first_sl, last_sl );
            wxLogMessage( cmd );
            ProcessManager( "Converting to MatLab.",
			    _T("\"")+Preferences::getHome()+"/"+cmd, true );
            break;
        case EXPORT_R :
            defaultFile = "expmth-tmp.r";
            saveDlg = new wxFileDialog( this, _T("Convert to R"),
                wxEmptyString, defaultFile,
                _T("R files (*.R;*.r)|*.R;*.r"), wxSAVE | wxOVERWRITE_PROMPT );
            if (saveDlg->ShowModal() != wxID_OK)    break;
            cmd = "exportMath\" \"";
            cmd += mCanvas->mCavassData->m_fname;
            cmd += "\" r \"";
            cmd += saveDlg->GetPath();
            cmd += wxString::Format( "\" %d %d", first_sl, last_sl );
            wxLogMessage( cmd );
            ProcessManager( "Converting to R.",
			    _T("\"")+Preferences::getHome()+"/"+cmd, true );
            break;
        case EXPORT_VTK :
            defaultFile = "expmth-tmp.vtk";
            saveDlg = new wxFileDialog( this, _T("Convert to VTK"),
                wxEmptyString, defaultFile,
                _T("VTK files (*.VTK;*.vtk)|*.VTK;*.vtk"), wxSAVE | wxOVERWRITE_PROMPT );
            if (saveDlg->ShowModal() != wxID_OK)    break;
            cmd = "exportMath\" \"";
            cmd += mCanvas->mCavassData->m_fname;
            cmd += "\" vtk \"";
            cmd += saveDlg->GetPath();
            cmd += wxString::Format( "\" %d %d", first_sl, last_sl );
            wxLogMessage( cmd );
            ProcessManager( "Converting to VTK.",
			    _T("\"")+Preferences::getHome()+"/"+cmd );
            break;

        default:
            assert( 0 );
            break;
    }

    if (saveDlg)
        delete saveDlg;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( ExportFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ExportFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_GRAYMAP,          ExportFrame::OnGrayMap    )
  EVT_BUTTON( ID_CT_LUNG,          ExportFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   ExportFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          ExportFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              ExportFrame::OnPET     )
  EVT_MOUSEWHEEL(                  ExportFrame::OnMouseWheel )
  EVT_BUTTON( ID_NEXT,             ExportFrame::OnNext       )
  EVT_BUTTON( ID_PREVIOUS,         ExportFrame::OnPrevious   )
  EVT_BUTTON( ID_OUT_TYPE,         ExportFrame::OnOutputType )
  EVT_BUTTON( ID_SAVE,             ExportFrame::OnExportSave )
  EVT_BUTTON( ID_SET_INDEX,        ExportFrame::OnSetIndex   )
  EVT_BUTTON( ID_SET_RANGE,        ExportFrame::OnSetRange   )
  EVT_BUTTON( ID_STRUCT_NUMBER,    ExportFrame::OnStructNumber)
  EVT_BUTTON( ID_SERIES_UID,       ExportFrame::OnSeriesUID  )
  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, ExportFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  ExportFrame::OnWidthSlider  )
  EVT_COMMAND_SCROLL( ID_SLICE1_SLIDER, ExportFrame::OnSlice1Slider )
  EVT_COMMAND_SCROLL( ID_SLICE2_SLIDER, ExportFrame::OnSlice2Slider )
  EVT_COMMAND_SCROLL( ID_SLICE_SLIDER,  ExportFrame::OnSliceSlider  )
  EVT_COMMAND_SCROLL( ID_SCALE_SLIDER,  ExportFrame::OnScaleSlider  )
  EVT_CHECKBOX( ID_OVERLAY,        ExportFrame::OnOverlay    )
  EVT_TEXT( ID_SERIES_UID_TABLET,  ExportFrame::OnSeriesUIDTablet   )

#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_CENTER_SLIDER, ExportFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  ExportFrame::OnUpdateUIWidthSglider )
#endif
END_EVENT_TABLE()
//======================================================================
