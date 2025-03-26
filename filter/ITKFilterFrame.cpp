/*
  Copyright 1993-2016, 2019 Medical Image Processing Group
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
 * \file   ITKFilterFrame.cpp
 * \brief  ITKFilterFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  <wx/combobox.h>
#include  "GrayMapControls.h"
#include  "SetIndexControls.h"

extern Vector  gFrameList;
//----------------------------------------------------------------------
#define  MaxITKStrValues   10  ///< max # of str values (for 1 param)

/** \brief table of ITK filters */
struct ITKFilter {
    char*  name;            ///< name that appears to user in window
    char*  progName;        ///< actual program file name
    /** \todo support file type filtering (especially BinaryOnly) */
    enum   { BinaryOnly=1, GrayOnly, BinaryOrGray, Unused };  ///< file types
    int    inputFileType;   ///< input file type
	int    inputFileType2;  ///< input file #2 type
    int    outputFileType;  ///< output file type
	int    outputFileType2; ///< output file #2 type

    struct Parameter {      ///< command line parameters
        char*   paramName;  ///< parameter name
        enum    { IntArg=1, DblArg, StrArg };  ///<  parameter types (integer, double, string)
        int     argType;    ///< type of parameter
        //for params that are ints or dbls (all null if unused)
        char*   idMin;      ///< min int or dbl value (may be nul)
        char*   idDefault;  ///< default int or dbl value (may ONLY be null if this is unused)
        char*   idMax;      ///< max int or dbl value (may be null)

        /** \todo below (support for itk list of options, instead of just numeric
         *  values, needs to be implemented.
         */
        //for params that are selected from a set of strs (all null if unused)
        int     sDefault;   ///< subscript of default
        char*   sValues[ MaxITKStrValues ];  ///< possible str values
    } parameters[ MaxITKParameters ];
};
//----------------------------------------------------------------------
const static struct ITKFilter  ITKFilterTable[] = {
  { (char *)"binary dilate", (char *)"itkBinaryDilateFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { (char *)"radius", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"binary erode", (char *)"itkBinaryErodeFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { (char *)"radius", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"binary median", (char *)"itkBinaryMedianFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { (char *)"radius x", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius y", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius z", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"binary morph", (char *)"itkBinaryMorphFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { (char *)"radius x", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius y", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius z", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"binary opening", (char *)"itkBinaryOpeningFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { (char *)"radius", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"binary threshold", (char *)"itkBinaryThresholdFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOrGray, ITKFilter::Unused,
        { { (char *)"lowerTh",  ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"upper",    ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"outValue", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"inValue",  ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"binomial blur", (char *)"itkBinomialBlurFilter",
    ITKFilter::BinaryOrGray, ITKFilter::Unused, ITKFilter::GrayOnly, ITKFilter::Unused,
        { { (char *)"numberOfRepetitions", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"100" },
          { 0 } }
  },
/*
  { (char *)"Canny edge detection", (char *)"itkCannyEdgeDetectionFilter",
    ITKFilter::BinaryOrGray, ITKFilter::Unused, ITKFilter::BinaryOrGray, ITKFilter::Unused,
        { { (char *)"variance", ITKFilter::Parameter::DblArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"max err",  ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"outValue", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"upper th", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"lower th", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
*/
  { (char *)"gray dilate", (char *)"itkGrayDilateFilter", 
    ITKFilter::BinaryOrGray, ITKFilter::Unused, ITKFilter::GrayOnly, ITKFilter::Unused,
        { { (char *)"radius", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"gray erode", (char *)"itkGrayErodeFilter",
    ITKFilter::BinaryOrGray, ITKFilter::Unused, ITKFilter::GrayOnly, ITKFilter::Unused,
        { { (char *)"radius", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"mean", (char *)"itkMeanFilter",
    ITKFilter::BinaryOrGray, ITKFilter::Unused, ITKFilter::GrayOnly, ITKFilter::Unused,
        { { (char *)"radius x", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius y", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius z", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"median", (char *)"itkMedianFilter",
    ITKFilter::BinaryOrGray, ITKFilter::Unused, ITKFilter::BinaryOrGray, ITKFilter::Unused,
        { { (char *)"radius x", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius y", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius z", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"voting", (char *)"itkVotingBinaryHoleFillingFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { (char *)"radius x", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius y", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { (char *)"radius z", ITKFilter::Parameter::IntArg, (char *)"1", (char *)"1", (char *)"10" },
          { 0 } }
  },
  { (char *)"Maurer Distance", (char *)"itkSignedMaurerDistanceMapFilter",
    ITKFilter::BinaryOnly, ITKFilter::Unused, ITKFilter::BinaryOnly, ITKFilter::Unused,
        { { 0 } }
  },

  { 0 }  //marks the end of the entries
};
//----------------------------------------------------------------------
/** \brief callback id table for parameters.  this must be kept in sync
 *  with MaxITKParameters.
 */
static int  paramIDs[ MaxITKParameters ] = {
    ITKFilterFrame::ID_PARAM1, ITKFilterFrame::ID_PARAM2,
    ITKFilterFrame::ID_PARAM3, ITKFilterFrame::ID_PARAM4,
    ITKFilterFrame::ID_PARAM5, ITKFilterFrame::ID_PARAM6,
    ITKFilterFrame::ID_PARAM7, ITKFilterFrame::ID_PARAM8,
    ITKFilterFrame::ID_PARAM9, ITKFilterFrame::ID_PARAM10
};
//----------------------------------------------------------------------
/**
 * \brief Definition and implementation of ITKFilterControls class.
 * This control class provides a variable number of boxes that allow the
 * user to enter ITK filter parameters.
 * Note: Callbacks are handled by the caller (as usual).
 */
class ITKFilterControls {
private:
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mParamBox;
    wxStaticBoxSizer*  mSbs;
    wxFlexGridSizer*   mFgs;
public:
    /** \brief ITKFilterControls ctor. */
    ITKFilterControls ( wxPanel* cp, wxSizer* bottomSizer,
        const struct ITKFilter* const command )
    {
        mBottomSizer = bottomSizer;
        wxString  name = "ITK ";
        name += command->name;
        name += " parameters";
        mParamBox = new wxStaticBox( cp, -1, name );
        ::setColor( mParamBox );
        mSbs = new wxStaticBoxSizer( mParamBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        for (int i=0; i<MaxITKParameters; i++) {
            if (command->parameters[i].paramName==0)    break;
            wxStaticText*  st = new wxStaticText( cp, -1, command->parameters[i].paramName );
            ::setColor( st );
            mFgs->Add( st, 0, wxALIGN_RIGHT );

            wxTextCtrl*  tc = new wxTextCtrl( cp, paramIDs[i],
                command->parameters[i].idDefault, wxDefaultPosition,
                wxDefaultSize );
            ::setColor( tc );
            mFgs->Add( tc, 0, wxGROW|wxLEFT|wxRIGHT );
        }
        
        mSbs->Add( mFgs, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Prepend( mSbs, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief ITKFilterControls dtor. */
    ~ITKFilterControls ( void ) {
        mFgs->Clear( true );
        mSbs->Clear( true );

        mSbs->Remove( mFgs );
        mBottomSizer->Remove( mSbs );
    }
};
//----------------------------------------------------------------------
/** \brief Constructor for ITKFilterFrame class.
 *
 *  Most of the work in this method is in creating the control panel at
 *  the bottom of the window.
 */
ITKFilterFrame::ITKFilterFrame ( bool maximize, int w, int h )
    : MainFrame( 0 )
{
    //init the types of input files that this app can accept
    mFileNameFilter = (char *)"CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff";
    mFileOrDataCount          = 0;
    mFilteredGrayMapControls  = NULL;
    mFilteredSetIndexControls = NULL;
    mGrayMapControls          = NULL;
    mInputIsBinary            = false;
    mITKFilterControls        = NULL;
    mModuleName               = "CAVASS:Tools:ITK Filters";
    mSaveScreenControls       = NULL;
    mSetIndexControls         = NULL;
    mSyncChecked              = true;

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
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via ITKFilterCanvas
    mCanvas = new ITKFilterCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );
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
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight-30 );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "Move",     2 );
    SetStatusText( "Previous", 3 );
    SetStatusText( "Next",     4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
    mSplitter->SetSashPosition( -dControlsHeight-30 );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }

    //display controls (parameters) for 0th itk filter
    wxCommandEvent  unused;
    mWhichFilter = -1;  //because it will first get incremented
    OnFilterName( unused );
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones).
 *  this function does not any additional, custom menu items (but could
 *  if necessary).
 */
void ITKFilterFrame::initializeMenu ( void ) {
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
void ITKFilterFrame::addButtonBox ( void ) {
    const int  width = (int)(buttonWidth * 1.6);

    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    wxFlexGridSizer*  fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap

    //row 1, col 1
    wxButton*  prev = new wxButton( mControlPanel, ID_PREVIOUS, "Previous", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( prev );
    fgs->Add( prev, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
    wxButton*  next = new wxButton( mControlPanel, ID_NEXT, "Next", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( next );
    fgs->Add( next, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 2, col 1
    wxButton*  setIndex = new wxButton( mControlPanel, ID_SET_INDEX, "SetIndex", wxDefaultPosition, wxSize(width, buttonHeight) );
    ::setColor( setIndex );
    fgs->Add( setIndex, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    wxButton*  grayMap = new wxButton( mControlPanel, ID_GRAYMAP, "GrayMap", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( grayMap );
    fgs->Add( grayMap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 3, col 1
    mWhichFilter = 0;
    mFilterName = new wxComboBox( mControlPanel, ID_FILTER_NAME,
        "Select ITK filter", wxDefaultPosition, wxSize(width,buttonHeight),
        0, NULL, wxCB_READONLY );
    ::setColor( mFilterName );
    for (int i=0; ::ITKFilterTable[i].name!=0; i++) {
        mFilterName->Append( ::ITKFilterTable[i].name );
    }
    mFilterName->SetSelection( 0 );
    fgs->Add( mFilterName, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
    wxButton*  filter = new wxButton( mControlPanel, ID_FILTER, "Filter", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( filter );
    fgs->Add( filter, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 4, col 1
    wxButton*  save = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( save );
    fgs->Add( save, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    wxButton*  reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( reset );
    fgs->Add( reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 5, col 1
    wxButton*  setFilteredIndex = new wxButton( mControlPanel, ID_FILTERED_SET_INDEX, "SetFilteredIndex", wxDefaultPosition, wxSize(width, buttonHeight) );
    ::setColor( setFilteredIndex );
    fgs->Add( setFilteredIndex, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    wxButton*  filteredGrayMap = new wxButton( mControlPanel, ID_FILTERED_GRAYMAP, "FilteredGrayMap", wxDefaultPosition, wxSize(width,buttonHeight) );
    ::setColor( filteredGrayMap );
    fgs->Add( filteredGrayMap, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    //row 6, col 1
    wxCheckBox*  cb = new wxCheckBox( mControlPanel, ID_SYNC, "sync" );
    ::setColor( cb );
    fgs->Add( cb, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0 );
    #if defined(wxUSE_TOOLTIPS) && !defined(__WXX11__)
        cb->SetToolTip( _T("synchronize changes to gray map and index") );
    #endif
    cb->SetValue( mSyncChecked );

    buttonSizer->Add( fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for ITKFilterFrame class. */
ITKFilterFrame::~ITKFilterFrame ( void ) {
    cout << "ITKFilterFrame::~ITKFilterFrame" << endl;
    wxLogMessage( "ITKFilterFrame::~ITKFilterFrame" );

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
/** \brief This method should be called whenever one wishes to create a
 *  new ITKFilterFrame.  It first searches the input file history for an
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
void ITKFilterFrame::createITKFilterFrame ( wxFrame* parentFrame, bool useHistory )
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
            "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0|DICOM files (*.DCM;*.DICOM;*.dcm;*.dicom)|*.DCM;*.DICOM;*.dcm;*.dicom|image files (*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff;*vtk)|*.bmp;*.gif;*.jpg;*.jpeg;*png;*.pcx;*.tif;*.tiff;*vtk",
            wxFILE_MUST_EXIST );
    }
    
    //was an input file selected?
    if (!filename || filename.Length()==0)    return;
    //add the input file to the input history
    ::gInputFileHistory.AddFileToHistory( filename );
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    //display an example frame using the specified file as input
    ITKFilterFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new ITKFilterFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new ITKFilterFrame();
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
 *  PCX, TIF, TIFF, and VTK.
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool ITKFilterFrame::match ( wxString filename ) {
    wxString  fn = filename.Upper();
    if (wxMatchWild( "*.BIM",   fn, false ))    return true;
    if (wxMatchWild( "*.BMP",   fn, false ))    return true;
    if (wxMatchWild( "*.DCM",   fn, false ))    return true;
    if (wxMatchWild( "*.DICOM", fn, false ))    return true;
    if (wxMatchWild( "*.GIF",   fn, false ))    return true;
    if (wxMatchWild( "*.IM0",   fn, false ))    return true;
    if (wxMatchWild( "*.JPG",   fn, false ))    return true;
    if (wxMatchWild( "*.JPEG",  fn, false ))    return true;
    if (wxMatchWild( "*.PCX",   fn, false ))    return true;
    if (wxMatchWild( "*.PNG",   fn, false ))    return true;
    if (wxMatchWild( "*.TIF",   fn, false ))    return true;
    if (wxMatchWild( "*.TIFF",  fn, false ))    return true;
    //if (wxMatchWild( "*.VTK",   fn, false ))    return true;

    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item. */
void ITKFilterFrame::OnOpen ( wxCommandEvent& unused ) {
    createITKFilterFrame( this, false );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    cout << "ITKFilterFrame::OnInput: ret=" << ret << " wxID_OK=" << wxID_OK << endl;
    cout << "ITKFilterFrame::OnInput: ret=" << ret << " wxID_CANCEL=" << wxID_CANCEL << endl;
    if (ret==wxID_OK) {
        OnITKFilter( unused );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 *  \param fname input file name.
 */
void ITKFilterFrame::loadFile ( const char* const fname ) {
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
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    canvas->loadFile( fname );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}

    if (mFileOrDataCount==1) {
        if (canvas->mCavassData->m_min==0 && canvas->mCavassData->m_max==1) {
            mInputIsBinary = true;
        }
    }

    const wxString  s = wxString::Format( "file %s", fname );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load (and subsequently display) data directly from memory
 *  (instead of from a file).
 *  \todo this is incomplete.
 */
void ITKFilterFrame::loadData ( char* name,
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
    
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
void ITKFilterFrame::OnPrevious ( wxCommandEvent& unused ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) - 1;
    if (slice<0)
		slice = canvas->getNoSlices(0)-1;
    canvas->setSliceNo( 0, slice );
    if (mSyncChecked && mFileOrDataCount>1) {
        slice = canvas->getSliceNo(1) - 1;
        if (slice < 0)
		    slice = canvas->getNoSlices(1)-1;
		canvas->setSliceNo( 1, slice );
    }
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Next button press.  display the next slice. */
void ITKFilterFrame::OnNext ( wxCommandEvent& unused ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    int  slice = canvas->getSliceNo(0) + 1;
    if (slice >= canvas->getNoSlices(0))
		slice = 0;
    canvas->setSliceNo( 0, slice );
    if (mSyncChecked && mFileOrDataCount>1) {
        slice = canvas->getSliceNo(1) + 1;
        if (slice >= canvas->getNoSlices(1))
			slice = 0;
	    canvas->setSliceNo( 1, slice );
    }
    canvas->reload();
    if (mSetIndexControls!=NULL)    mSetIndexControls->setSliceNo( slice );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 */
void ITKFilterFrame::OnGrayMap ( wxCommandEvent& unused ) {
#if 1
    //first, remove any old controls that may be present
    const bool  wasPresent = (mGrayMapControls!=NULL);
    removeControls();
    if (wasPresent)    return;
#else
    //first, remove any old controls that may be present
    if (mFilteredGrayMapControls!=NULL)
                                  { delete mGrayMapControls;    mGrayMapControls=NULL;   }
    if (mITKFilterControls!=NULL) { delete mITKFilterControls;  mITKFilterControls=NULL; }
    if (mSetIndexControls!=NULL)  { delete mSetIndexControls;   mSetIndexControls=NULL;  }
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
        return;
    }
#endif
    if (mFileOrDataCount<1)    return;

    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    mGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "GrayMap", canvas->getCenter(0), canvas->getWidth(0),
        canvas->getMax(0), canvas->getInvert(0),
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
		ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void ITKFilterFrame::OnCenterSlider ( wxScrollEvent& e ) {
    if (mFileOrDataCount<1)    return;
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (canvas->getCenter(0)==e.GetPosition())    return;  //no change
    canvas->setCenter( 0, e.GetPosition() );
    canvas->initLUT(   0 );
    if (mSyncChecked && mFileOrDataCount>1) {
        canvas->setCenter( 1, e.GetPosition() );
        canvas->initLUT(   1 );
    }
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider (used to change contrast). */
void ITKFilterFrame::OnWidthSlider ( wxScrollEvent& e ) {
    if (mFileOrDataCount<1)    return;
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (canvas->getWidth(0)==e.GetPosition())    return;  //no change
    canvas->setWidth( 0, e.GetPosition() );
    canvas->initLUT(  0 );
    if (mSyncChecked && mFileOrDataCount>1) {
        canvas->setWidth( 1, e.GetPosition() );
        canvas->initLUT(  1 );
    }
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for width slider for data set 1 */
void ITKFilterFrame::OnCTLung ( wxCommandEvent& unused ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
void ITKFilterFrame::OnCTSoftTissue ( wxCommandEvent& unused ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
void ITKFilterFrame::OnCTBone ( wxCommandEvent& unused ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
void ITKFilterFrame::OnPET ( wxCommandEvent& unused ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
 */
void ITKFilterFrame::OnUpdateUICenterSlider ( wxUpdateUIEvent& unused ) {
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
void ITKFilterFrame::OnUpdateUIWidthSlider ( wxUpdateUIEvent& unused ) {
    if (m_width->GetValue() == mCanvas->getWidth())    return;
    mCanvas->setWidth( m_width->GetValue() );
    mCanvas->initLUT();
    mCanvas->reload();
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void ITKFilterFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
void ITKFilterFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    ITKFilterCanvas*     canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
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
void ITKFilterFrame::OnMouseWheel ( wxMouseEvent& e ) {
    const int  rot   = e.GetWheelRotation();
    wxCommandEvent  ce;
    if (rot>0)         OnPrevious(ce);
    else if (rot<0)    OnNext(ce);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief this method is called when the user presses the Filter button
 *  (to run the itk filter program on one sample slice and to display
 *  the result).
 */
void ITKFilterFrame::OnFilter (  wxCommandEvent& unused  ) {
    assert( mWhichFilter>=0 && ::ITKFilterTable[mWhichFilter].name!=0 );
    //remove anything that may be left behind from before
    unlink( "voi_tmp.IM0"  );
    unlink( "voi_tmp2.IM0" );

    //check for a binary-input-only filter
    if (!mInputIsBinary && ::ITKFilterTable[mWhichFilter].inputFileType == ::ITKFilterTable[mWhichFilter].BinaryOnly) {
        wxMessageBox( "ITK filter requires binary input but the input is not binary." );
        return;
    }

    //step 1:
    //  run the nvdoi program to obtain a file that contains only the
    //  current slice.
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    canvas->freeFilteredData();
    wxString  voiCmd("\"");
	voiCmd += Preferences::getHome()+wxString::Format(
        "/ndvoi\" \"%s\" voi_tmp.IM0 %d %d %d %d %d %d %d %d %d",
        canvas->mCavassData->m_fname, 0, 0, 0, canvas->mCavassData->m_xSize,
        canvas->mCavassData->m_ySize, 0, 0, canvas->getSliceNo(0),
        canvas->getSliceNo(0) );
    wxLogMessage( "command=%s", (const char *)voiCmd.c_str() );
    ProcessManager  q( "ndvoi running...", voiCmd, true, false, false );
    if (q.getCancel())    return;
    int  error = q.getStatus();
#if 0
    if (error != 0) {  wxMessageBox( "ndvoi failed." );  return;  }
#endif
    //step 2:
    //  run the selected itk filter program on this slice.
    wxString  itkCmd("\"");
	itkCmd += Preferences::getHome()+wxString::Format( "/%s\" %s %s",
        ::ITKFilterTable[mWhichFilter].progName, "voi_tmp.IM0", "voi_tmp2.IM0"
    );
    //use user-input parameters.  otherwise, use default parameters.
    for (int i=0; i<MaxITKParameters; i++) {
        if (::ITKFilterTable[mWhichFilter].parameters[i].paramName==0)    break;
        if (mP[i] != "")  //use user input
            itkCmd += " " + mP[i];
        else  //use default
            itkCmd += wxString::Format( " %s",
                ::ITKFilterTable[mWhichFilter].parameters[i].idDefault );
    }
    wxLogMessage( "command=%s", (const char *)itkCmd.c_str() );	

#if defined (WIN32) || defined (_WIN32)
    ProcessManager  p( "itk filter running...", itkCmd, true, false, false );
    if (p.getCancel())    return;
    error = p.getStatus();
#if 0
    if (error != 0) {  wxMessageBox( "itk filter failed." );  return;  }
#endif

#else
	error = system( (const char *)itkCmd.c_str() );
#endif

    //step 3:
    //  load the result.
    canvas->loadFile( "voi_tmp2.IM0" );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}
    ++mFileOrDataCount;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief this method is called when the user presses the itk filter
 *  name button (to select a different filter).
 */
void ITKFilterFrame::OnFilterName (  wxCommandEvent& unused ) {
    //first, remove any old controls that may be present
    if (mITKFilterControls!=NULL) { delete mITKFilterControls;  mITKFilterControls=NULL; }
    if (mGrayMapControls!=NULL)   { delete mGrayMapControls;    mGrayMapControls=NULL;   }
    if (mSetIndexControls!=NULL)  { delete mSetIndexControls;   mSetIndexControls=NULL;  }

    mWhichFilter = mFilterName->GetSelection();
    //reset user-input filter parameters
    for (int i=0; i<MaxITKParameters; i++) {
        mP[i] = "";
    }
    //add the ITK filter controls (param box)
    //ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    mITKFilterControls = new ITKFilterControls( mControlPanel, mBottomSizer,
        &::ITKFilterTable[mWhichFilter] );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief these callbacks are called in response to _any_ (even just
 *  typing just one character) changes to the itk parameter text fields.
 */
void ITKFilterFrame::OnITKParam1  ( wxCommandEvent& e ) { mP[0] = e.GetString(); }
void ITKFilterFrame::OnITKParam2  ( wxCommandEvent& e ) { mP[1] = e.GetString(); }
void ITKFilterFrame::OnITKParam3  ( wxCommandEvent& e ) { mP[2] = e.GetString(); }
void ITKFilterFrame::OnITKParam4  ( wxCommandEvent& e ) { mP[3] = e.GetString(); }
void ITKFilterFrame::OnITKParam5  ( wxCommandEvent& e ) { mP[4] = e.GetString(); }
void ITKFilterFrame::OnITKParam6  ( wxCommandEvent& e ) { mP[5] = e.GetString(); }
void ITKFilterFrame::OnITKParam7  ( wxCommandEvent& e ) { mP[6] = e.GetString(); }
void ITKFilterFrame::OnITKParam8  ( wxCommandEvent& e ) { mP[7] = e.GetString(); }
void ITKFilterFrame::OnITKParam9  ( wxCommandEvent& e ) { mP[8] = e.GetString(); }
void ITKFilterFrame::OnITKParam10 ( wxCommandEvent& e ) { mP[9] = e.GetString(); }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnReset ( wxCommandEvent& unused ) {
    //remove an displayed filtered data that may be present
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    canvas->freeFilteredData();
    //remove any old controls that may be present
    removeControls();

    mWhichFilter = 0;
    mFilterName->SetSelection( mWhichFilter );
    //reset user-input filter parameters
    for (int i=0; i<MaxITKParameters; i++) {
        mP[i] = "";
    }
    //display controls (parameters) for 0th itk filter
    mWhichFilter = -1;  //because it will first get incremented
    OnFilterName( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnSave (  wxCommandEvent& unused  ) {
    assert( mWhichFilter>=0 && ::ITKFilterTable[mWhichFilter].name!=0 );
    //remove anything that may be left behind from before
    unlink( "voi_tmp.IM0"  );
    unlink( "voi_tmp2.IM0" );

    //check for a binary-input-only filter
    if (!mInputIsBinary && ::ITKFilterTable[mWhichFilter].inputFileType == ::ITKFilterTable[mWhichFilter].BinaryOnly) {
        wxMessageBox( "ITK filter requires binary input but the input is not binary." );
        return;
    }

    //pop up a dialog to allow the user to specify the output file for the result.
    wxFileDialog  f( this, "Select filter output image file", "", "voi_tmp.IM0",
        "CAVASS files (*.BIM;*.IM0)|*.BIM;*.IM0", wxSAVE | wxOVERWRITE_PROMPT );
    const int  ret = f.ShowModal();
    if (ret == wxID_CANCEL)    return;

    //step 1:
    //  run the selected itk filter program on this slice.
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    canvas->freeFilteredData();
    wxString  itkCmd("\"");
	itkCmd += Preferences::getHome()+wxString::Format( "/%s\" \"%s\" \"%s\"",
        ::ITKFilterTable[mWhichFilter].progName, canvas->mCavassData->m_fname,
        (const char *)f.GetPath().c_str() );
    //use user-input parameters.  otherwise, use default parameters.
    for (int i=0; i<MaxITKParameters; i++) {
        if (::ITKFilterTable[mWhichFilter].parameters[i].paramName==0)    break;
        //get the parameter value (using default if necessary)
        wxString  p = ::ITKFilterTable[mWhichFilter].parameters[i].idDefault;
        if (mP[i] != "")    p = mP[i];  //use user input instead
        itkCmd += " " + p;
    }
    wxLogMessage( "command=%s", (const char *)itkCmd.c_str() );

	//time_t ltime;
 //   time( &ltime );

#if defined (WIN32) || defined (_WIN32)
    ProcessManager  p( "itk filter running...", itkCmd );
    if (p.getCancel())    return;
    int  error = p.getStatus();
#if 0
    if (error != 0) {  wxMessageBox( "itk filter failed." );  return;  }
#endif

#else
	//int error = system( itkCmd.c_str() );
	system( (const char *)itkCmd.c_str() );
#endif

	/*time_t ltime2;
    time( &ltime2 );

	FILE* fp = NULL;
	fp = fopen("C:/xinjian/Program/LinearDistanceTransform/DistTransformResult_3.23.2009.txt", "a+");
	if( fp == NULL )
	return;

	int nVoxelsNum = canvas->mCavassData->m_xSize * canvas->mCavassData->m_ySize * canvas->mCavassData->m_zSize;
	fprintf(fp, "%s, VoxelsNum = %d, x*y*z = %d x %d x %d, bitSize = %d, time = %d \n", canvas->mCavassData->m_fname, nVoxelsNum,canvas->mCavassData->m_xSize, canvas->mCavassData->m_ySize, canvas->mCavassData->m_zSize, canvas->mCavassData->m_size*8, ltime2-ltime );		
	fclose(fp);*/

    //step 2:
    //  load the result.
    canvas->loadFile( f.GetPath().c_str() );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}
    ++mFileOrDataCount;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnSetIndex ( wxCommandEvent& unused ) {
    //first, remove any old controls that may be present
    const bool  wasPresent = (mSetIndexControls!=NULL);
    removeControls();
    if (wasPresent)    return;

    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    mSetIndexControls = new SetIndexControls( mControlPanel, mBottomSizer,
        "Set Index", canvas->getSliceNo(0), canvas->getNoSlices(0), ID_SLICE_SLIDER,
        ID_SCALE_SLIDER, canvas->getScale(0), wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnInvert ( wxCommandEvent& e ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    assert( canvas!=NULL );
    assert( canvas->mCavassData!=NULL );

    canvas->mCavassData->mInvert = e.IsChecked();
    canvas->mCavassData->initLUT();

    if (mSyncChecked && canvas->mCavassData->mNext!=NULL) {
        canvas->mCavassData->mNext->mInvert = e.IsChecked();
        canvas->mCavassData->mNext->initLUT();
    }

    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for slice slider */
void ITKFilterFrame::OnSliceSlider ( wxScrollEvent& e ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (canvas->getSliceNo(0)==e.GetPosition())    return;  //no change
    canvas->setSliceNo( 0, e.GetPosition()-1 );
    if (mSyncChecked && mFileOrDataCount>1)
        canvas->setSliceNo( 1, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for scale slider */
void ITKFilterFrame::OnScaleSlider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue / 100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mSetIndexControls->setScaleText( s );
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    canvas->setScale( 0, newScale );
    if (mSyncChecked && mFileOrDataCount>1)  canvas->setScale( 1, newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for center slider (used to change contrast). */
void ITKFilterFrame::OnFilteredCenterSlider ( wxScrollEvent& e ) {
    if (mFileOrDataCount<2)    return;
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (canvas->getCenter(1)==e.GetPosition())    return;  //no change
    if (mSyncChecked) {
        canvas->setCenter( 0, e.GetPosition() );
        canvas->initLUT(   0 );
    }
    canvas->setCenter( 1, e.GetPosition() );
    canvas->initLUT(   1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    //first, remove any old controls that may be present
    const bool  wasPresent = (mSaveScreenControls!=NULL);
    removeControls();
    if (wasPresent)    return;

    MainFrame::OnSaveScreen( unused );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for gray map button press.  display controls that
 *  allow the user to modify the contrast (gray map).
 */
void ITKFilterFrame::OnFilteredGrayMap ( wxCommandEvent& unused ) {
    //first, remove any old controls that may be present
    const bool  wasPresent = (mFilteredGrayMapControls!=NULL);
    removeControls();
    if (wasPresent)    return;
    if (mFileOrDataCount<2)    return;

    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    mFilteredGrayMapControls = new GrayMapControls( mControlPanel, mBottomSizer,
        "FilteredGrayMap", canvas->getCenter(1), canvas->getWidth(1),
        canvas->getMax(1), canvas->getInvert(1),
        ID_FILTERED_CENTER_SLIDER, ID_FILTERED_WIDTH_SLIDER, ID_FILTERED_INVERT, wxID_ANY, wxID_ANY, wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnFilteredInvert ( wxCommandEvent& e ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    assert( canvas!=NULL );
    assert( canvas->mCavassData!=NULL );
    assert( canvas->mCavassData->mNext!=NULL );

    if (mSyncChecked) {
        canvas->mCavassData->mInvert = e.IsChecked();
        canvas->mCavassData->initLUT();
    }

    canvas->mCavassData->mNext->mInvert = e.IsChecked();
    canvas->mCavassData->mNext->initLUT();

    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnFilteredScaleSlider ( wxScrollEvent& e ) {
    const int     newScaleValue = e.GetPosition();
    const double  newScale = newScaleValue / 100.0;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mFilteredSetIndexControls->setScaleText( s );
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (mSyncChecked)    canvas->setScale( 0, newScale );
    canvas->setScale( 1, newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnFilteredSetIndex ( wxCommandEvent& unused ) {
    //first, remove any old controls that may be present
    const bool  wasPresent = (mFilteredSetIndexControls!=NULL);
    removeControls();
    if (wasPresent)    return;
    if (mFileOrDataCount<2)    return;

    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    mFilteredSetIndexControls = new SetIndexControls( mControlPanel, mBottomSizer,
        "Set Filtered Index", canvas->getSliceNo(1), canvas->getNoSlices(1),
        ID_FILTERED_SLICE_SLIDER, ID_FILTERED_SCALE_SLIDER,
        canvas->getScale(1), wxID_ANY, wxID_ANY );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnFilteredSliceSlider ( wxScrollEvent& e ) {
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (canvas->getSliceNo(1)==e.GetPosition())    return;  //no change
    if (mSyncChecked)    canvas->setSliceNo( 0, e.GetPosition()-1 );
    canvas->setSliceNo( 1, e.GetPosition()-1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnFilteredWidthSlider ( wxScrollEvent& e ) {
    if (mFileOrDataCount<2)    return;
    ITKFilterCanvas*  canvas = dynamic_cast<ITKFilterCanvas*>(mCanvas);
    if (canvas->getWidth(1)==e.GetPosition())    return;  //no change
    if (mSyncChecked) {
        canvas->setWidth( 0, e.GetPosition() );
        canvas->initLUT(  0 );
    }
    canvas->setWidth( 1, e.GetPosition() );
    canvas->initLUT(  1 );
    canvas->reload();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ITKFilterFrame::OnSync ( wxCommandEvent& e ) {
    mSyncChecked = e.IsChecked();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
void ITKFilterFrame::OnUpdateUIFilteredCenterSlider ( wxUpdateUIEvent& unused ) {
}
void ITKFilterFrame::OnUpdateUIFilteredWidthSlider  ( wxUpdateUIEvent& unused ) {
}
void ITKFilterFrame::OnUpdateUIFilteredScaleSlider  ( wxUpdateUIEvent& unused ) {
}
void ITKFilterFrame::OnUpdateUIFilteredSliceSlider  ( wxUpdateUIEvent& unused ) {
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief remove any old controls that may be present */
void ITKFilterFrame::removeControls ( void ) {
    if (mFilteredGrayMapControls!=NULL) {
        delete mFilteredGrayMapControls;
        mFilteredGrayMapControls = NULL;
    }
    if (mFilteredSetIndexControls!=NULL) {
        delete mFilteredSetIndexControls;
        mFilteredSetIndexControls = NULL;
    }
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
    if (mITKFilterControls!=NULL) {
        delete mITKFilterControls;
        mITKFilterControls = NULL;
    }
	if (mSaveScreenControls!=NULL) {
		delete mSaveScreenControls;
		mSaveScreenControls = NULL;
	}
    if (mSetIndexControls!=NULL) {
        delete mSetIndexControls;
        mSetIndexControls = NULL;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// event table and callbacks.
IMPLEMENT_DYNAMIC_CLASS ( ITKFilterFrame, wxFrame )
BEGIN_EVENT_TABLE       ( ITKFilterFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_GRAYMAP,            ITKFilterFrame::OnGrayMap          )
  EVT_BUTTON( ID_FILTERED_GRAYMAP,   ITKFilterFrame::OnFilteredGrayMap  )
  EVT_BUTTON( ID_NEXT,               ITKFilterFrame::OnNext             )
  EVT_BUTTON( ID_PREVIOUS,           ITKFilterFrame::OnPrevious         )
  EVT_BUTTON( ID_FILTER,             ITKFilterFrame::OnFilter           )
  EVT_BUTTON( ID_RESET,              ITKFilterFrame::OnReset            )
  EVT_BUTTON( ID_SAVE,               ITKFilterFrame::OnSave             )
  EVT_BUTTON( ID_SET_INDEX,          ITKFilterFrame::OnSetIndex         )
  EVT_BUTTON( ID_FILTERED_SET_INDEX, ITKFilterFrame::OnFilteredSetIndex )

  EVT_CHECKBOX( ID_INVERT,          ITKFilterFrame::OnInvert          )
  EVT_CHECKBOX( ID_FILTERED_INVERT, ITKFilterFrame::OnFilteredInvert  )
  EVT_CHECKBOX( ID_SYNC,            ITKFilterFrame::OnSync            )

  EVT_COMBOBOX( ID_FILTER_NAME, ITKFilterFrame::OnFilterName )
  EVT_MOUSEWHEEL( ITKFilterFrame::OnMouseWheel )

  EVT_COMMAND_SCROLL( ID_CENTER_SLIDER, ITKFilterFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH_SLIDER,  ITKFilterFrame::OnWidthSlider  )
  EVT_COMMAND_SCROLL( ID_SCALE_SLIDER,  ITKFilterFrame::OnScaleSlider  )
  EVT_COMMAND_SCROLL( ID_SLICE_SLIDER,  ITKFilterFrame::OnSliceSlider  )

  EVT_COMMAND_SCROLL( ID_FILTERED_CENTER_SLIDER, ITKFilterFrame::OnFilteredCenterSlider )
  EVT_COMMAND_SCROLL( ID_FILTERED_WIDTH_SLIDER,  ITKFilterFrame::OnFilteredWidthSlider  )
  EVT_COMMAND_SCROLL( ID_FILTERED_SCALE_SLIDER,  ITKFilterFrame::OnFilteredScaleSlider  )
  EVT_COMMAND_SCROLL( ID_FILTERED_SLICE_SLIDER,  ITKFilterFrame::OnFilteredSliceSlider  )

  EVT_BUTTON( ID_CT_LUNG,           ITKFilterFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,    ITKFilterFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,           ITKFilterFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,               ITKFilterFrame::OnPET     )

  EVT_TEXT( ID_PARAM1,  ITKFilterFrame::OnITKParam1  )
  EVT_TEXT( ID_PARAM2,  ITKFilterFrame::OnITKParam2  )
  EVT_TEXT( ID_PARAM3,  ITKFilterFrame::OnITKParam3  )
  EVT_TEXT( ID_PARAM4,  ITKFilterFrame::OnITKParam4  )
  EVT_TEXT( ID_PARAM5,  ITKFilterFrame::OnITKParam5  )
  EVT_TEXT( ID_PARAM6,  ITKFilterFrame::OnITKParam6  )
  EVT_TEXT( ID_PARAM7,  ITKFilterFrame::OnITKParam7  )
  EVT_TEXT( ID_PARAM8,  ITKFilterFrame::OnITKParam8  )
  EVT_TEXT( ID_PARAM9,  ITKFilterFrame::OnITKParam9  )
  EVT_TEXT( ID_PARAM10, ITKFilterFrame::OnITKParam10 )
#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_CENTER_SLIDER, ITKFilterFrame::OnUpdateUICenterSlider )
  EVT_UPDATE_UI( ID_WIDTH_SLIDER,  ITKFilterFrame::OnUpdateUIWidthSlider  )
  EVT_UPDATE_UI( ID_SCALE_SLIDER,  ITKFilterFrame::OnUpdateUIScaleSlider  )
  EVT_UPDATE_UI( ID_SLICE_SLIDER,  ITKFilterFrame::OnUpdateUISliceSlider  )

  EVT_UPDATE_UI( ID_FILTERED_CENTER_SLIDER, ITKFilterFrame::OnUpdateUIFilteredCenterSlider )
  EVT_UPDATE_UI( ID_FILTERED_WIDTH_SLIDER,  ITKFilterFrame::OnUpdateUIFilteredWidthSlider  )
  EVT_UPDATE_UI( ID_FILTERED_SCALE_SLIDER,  ITKFilterFrame::OnUpdateUIFilteredScaleSlider  )
  EVT_UPDATE_UI( ID_FILTERED_SLICE_SLIDER,  ITKFilterFrame::OnUpdateUIFilteredSliceSlider  )
#endif
END_EVENT_TABLE()
//======================================================================
