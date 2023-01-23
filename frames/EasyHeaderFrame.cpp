/*
  Copyright 1993-2013, 2017 Medical Image Processing Group
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
 * \file   EasyHeaderFrame.cpp
 * \brief  EasyHeaderFrame class implementation.
 * \author Dewey Odhner
 *
 * Copyright: (C) 2009 University of Pennsylvania
 *
 */
//======================================================================

#include  "cavass.h"
#include "EasyHeaderFrame.h"

#include  "wx/print.h"
#include  "wx/printdlg.h"

/** \brief EasyHeaderPrint class definitiion and implementation. */
class EasyHeaderPrint: public wxPrintout {
  protected:
    EasyHeaderCanvas*  m_canvas;  ///< canvas that is drawn

  public:
    /** \brief EasyHeaderPrint ctor. */
    EasyHeaderPrint ( const wxChar* const title = _T("EasyHeader"), EasyHeaderCanvas* mc = NULL )
        : wxPrintout(title), m_canvas(mc)
    {
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief this method prints the specified page.
     *  \param page the specific page to print.
     *  \returns true if the dc if valid; false otherwise.
     */
    bool OnPrintPage ( int page ) {
        wxDC*  dc = GetDC();
        if (!dc)    return false;

        if (m_canvas!=NULL) {
            //get the size of the canvas in pixels
            int  cw, ch;
            m_canvas->GetSize( &cw, &ch );
            //You might use THIS code if you were scaling graphics of 
            // known size to fit on the page.
            
            //50 device units margin
            const int  marginX=50, marginY=50;
            // Add the margin to the graphic size
            const double  maxX = cw + 2 * marginX;
            const double  maxY = ch + 2 * marginY;
            
            //get the size of the DC in pixels
            int  w, h;
            dc->GetSize( &w, &h );
            // Calculate a suitable scaling factor
            const double  scaleX = w / maxX;
            const double  scaleY = h / maxY;
            
            // Use x or y scaling factor, whichever fits on the DC
            const double  actualScale = wxMin( scaleX, scaleY );
            
            // Calculate the position on the DC for centering the graphic
            double  posX = (w - (cw*actualScale)) / 2.0;
            double  posY = (h - (ch*actualScale)) / 2.0;
            
            // Set the scale and origin
            dc->SetUserScale( actualScale, actualScale );
            dc->SetDeviceOrigin( (long)posX, (long)posY );
            m_canvas->paint( dc );
        }

        dc->SetDeviceOrigin( 0, 0 );
        dc->SetUserScale( 1.0, 1.0 );

        wxChar  buf[200];
        wxSprintf( buf, wxT("Page %d"), page );
        dc->SetTextBackground( *wxWHITE );
        dc->SetTextForeground( *wxBLACK );
        dc->DrawText( buf, 10, 10 );
        return true;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief determines if a particular page can be printed.
     *  \param pageNum the particular page number for possibly printing.
     *  \returns true if pageNum can be printed; false otherwise.
     */
    bool HasPage ( int pageNum ) {
        return (pageNum == 1);
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief this function allows the caller to obtain printing info. */
    void GetPageInfo ( int* minPage, int* maxPage, int* selPageFrom, int* selPageTo )
    {
        *minPage     = 1;
        *maxPage     = 1;
        *selPageFrom = 1;
        *selPageTo   = 1;
    }

};


extern Vector  gFrameList;
//----------------------------------------------------------------------
/** \brief Constructor for EasyHeaderFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
EasyHeaderFrame::EasyHeaderFrame ( bool maximize, int w, int h ): MainFrame( 0 ){
    mModuleName  = "CAVASS:Import:EasyHeader";
    mWindowTitle = mModuleName;
    SetTitle( mWindowTitle );
    mSaveScreenControls = NULL;

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

    //top of window contains image(s) displayed via EasyHeaderCanvas
    mCanvas = tCanvas = new EasyHeaderCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );

	mReadHeader = NULL;
	mSwapBytes = NULL;
	mWriteHeader = NULL;
	mWriteData = NULL;
	mHeaderLengthLabel = NULL;
	mHeaderLengthCtrl = NULL;
	mSkipLabel = NULL;
	mSkipCtrl = NULL;
	mDataSlicesLabel = NULL;
	mDataSlicesCtrl = NULL;
	mIncrementLabel = NULL;
	mIncrementCtrl = NULL;

    m_buttonBox = NULL;
    m_buttonSizer = NULL;
    m_fgs = NULL;
	mDataInFile = NULL;
	mOutFile = new wxFileName(wxFileName::GetCwd(), "easyhdr-tmp.IM0");
	mVH = NULL;

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


	tCanvas->mFgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
	tCanvas->mDateLabel = new wxStaticText( tCanvas, -1, "Date:" );
	::setColor( tCanvas->mDateLabel );
	tCanvas->mFgs->Add( tCanvas->mDateLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mDateCtrl = new wxTextCtrl( tCanvas, ID_DATE, "" );
	::setColor( tCanvas->mDateCtrl );
	tCanvas->mFgs->Add( tCanvas->mDateCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mTimeLabel = new wxStaticText( tCanvas, -1, "Time:" );
	::setColor( tCanvas->mTimeLabel );
	tCanvas->mFgs->Add( tCanvas->mTimeLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mTimeCtrl = new wxTextCtrl( tCanvas, ID_TIME, "" );
	::setColor( tCanvas->mTimeCtrl );
	tCanvas->mFgs->Add( tCanvas->mTimeCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mStudyLabel = new wxStaticText( tCanvas, -1, "Study:" );
	::setColor( tCanvas->mStudyLabel );
	tCanvas->mFgs->Add( tCanvas->mStudyLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mStudyCtrl = new wxTextCtrl( tCanvas, ID_STUDY, "" );
	::setColor( tCanvas->mStudyCtrl );
	tCanvas->mFgs->Add( tCanvas->mStudyCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mSeriesLabel = new wxStaticText( tCanvas, -1, "Series:" );
	::setColor( tCanvas->mSeriesLabel );
	tCanvas->mFgs->Add( tCanvas->mSeriesLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mSeriesCtrl = new wxTextCtrl( tCanvas, ID_SERIES, "" );
	::setColor( tCanvas->mSeriesCtrl );
	tCanvas->mFgs->Add( tCanvas->mSeriesCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mFgs->AddSpacer( 4 );
	tCanvas->mFgs->AddSpacer( 4 );
	tCanvas->mDimensionLabel = new wxStaticText( tCanvas, -1, "Dimension:" );
	::setColor( tCanvas->mDimensionLabel );
	tCanvas->mFgs->Add( tCanvas->mDimensionLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	wxArrayString sa;
	sa.Add("3");
	sa.Add("4");
	tCanvas->mDimensionCtrl = new wxComboBox( tCanvas, ID_DIMENSION, "3", wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
	::setColor( tCanvas->mDimensionCtrl );
	tCanvas->mFgs->Add( tCanvas->mDimensionCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mSliceWidthLabel = new wxStaticText( tCanvas, -1, "Slice Width:");
	::setColor( tCanvas->mSliceWidthLabel );
	tCanvas->mFgs->Add( tCanvas->mSliceWidthLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mSliceWidthCtrl = new wxTextCtrl( tCanvas, ID_SLICE_WIDTH, "256");
	::setColor( tCanvas->mSliceWidthCtrl );
	tCanvas->mFgs->Add( tCanvas->mSliceWidthCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mSliceHeightLabel = new wxStaticText( tCanvas, -1, "Slice Height:");
	::setColor( tCanvas->mSliceHeightLabel );
	tCanvas->mFgs->Add( tCanvas->mSliceHeightLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mSliceHeightCtrl = new wxTextCtrl( tCanvas, ID_SLICE_HEIGHT, "256" );
	::setColor( tCanvas->mSliceHeightCtrl );
	tCanvas->mFgs->Add( tCanvas->mSliceHeightCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mBitsPerPixelLabel = new wxStaticText( tCanvas, -1, "Bits Per Pixel:" );
	::setColor( tCanvas->mBitsPerPixelLabel );
	tCanvas->mFgs->Add( tCanvas->mBitsPerPixelLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mBitsPerPixelCtrl = new wxTextCtrl( tCanvas, ID_BITS, "8" );
	::setColor( tCanvas->mBitsPerPixelCtrl );
	tCanvas->mFgs->Add( tCanvas->mBitsPerPixelCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mPixelSizeLabel = new wxStaticText( tCanvas, -1, "Pixel Size:" );
	::setColor( tCanvas->mPixelSizeLabel );
	tCanvas->mFgs->Add( tCanvas->mPixelSizeLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mPixelSizeCtrl = new wxTextCtrl( tCanvas, ID_PIXEL_SIZE, "1.0" );
	::setColor( tCanvas->mPixelSizeCtrl );
	tCanvas->mFgs->Add( tCanvas->mPixelSizeCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mMinDensityLabel = new wxStaticText( tCanvas, -1, "Min. Density:" );
	::setColor( tCanvas->mMinDensityLabel );
	tCanvas->mFgs->Add( tCanvas->mMinDensityLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mMinDensityCtrl = new wxTextCtrl( tCanvas, ID_MIN_DENSITY, "0" );
	::setColor( tCanvas->mMinDensityCtrl );
	tCanvas->mFgs->Add( tCanvas->mMinDensityCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mMaxDensityLabel = new wxStaticText( tCanvas, -1, "Max. Density:" );
	::setColor( tCanvas->mMaxDensityLabel );
	tCanvas->mFgs->Add( tCanvas->mMaxDensityLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mMaxDensityCtrl = new wxTextCtrl( tCanvas, ID_MAX_DENSITY, "255");
	::setColor( tCanvas->mMaxDensityCtrl );
	tCanvas->mFgs->Add( tCanvas->mMaxDensityCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mFgs->AddSpacer( 4 );
	tCanvas->mFgs->AddSpacer( 4 );
	tCanvas->mNumSlicesLabel = new wxStaticText( tCanvas, -1, "# Slices:" );
	::setColor( tCanvas->mNumSlicesLabel );
	tCanvas->mFgs->Add( tCanvas->mNumSlicesLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mNumSlicesCtrl = new wxTextCtrl( tCanvas, ID_HEADER_SLICES, "1" );
	::setColor( tCanvas->mNumSlicesCtrl );
	tCanvas->mFgs->Add( tCanvas->mNumSlicesCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mFirstSliceLocLabel = new wxStaticText( tCanvas, -1, "1st Slice Loc.:" );
	::setColor( tCanvas->mFirstSliceLocLabel );
	tCanvas->mFgs->Add( tCanvas->mFirstSliceLocLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mFirstSliceLocCtrl = new wxTextCtrl( tCanvas, ID_FIRST_SLICE_LOC, "0.0" );
	::setColor( tCanvas->mFirstSliceLocCtrl );
	tCanvas->mFgs->Add( tCanvas->mFirstSliceLocCtrl, 0, wxGROW|wxLEFT|wxRIGHT);
	tCanvas->mSliceSpacingLabel = new wxStaticText( tCanvas, -1, "Slice Spacing:" );
	::setColor( tCanvas->mSliceSpacingLabel );
	tCanvas->mFgs->Add( tCanvas->mSliceSpacingLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mSliceSpacingCtrl = new wxTextCtrl( tCanvas, ID_SLICE_SPACING, "1.0" );
	::setColor( tCanvas->mSliceSpacingCtrl );
	tCanvas->mFgs->Add( tCanvas->mSliceSpacingCtrl, 0, wxGROW|wxLEFT|wxRIGHT);
	tCanvas->mFgs->AddSpacer( 4 );
	tCanvas->mFgs->AddSpacer( 4 );
	tCanvas->SetAutoLayout( true );
	tCanvas->SetSizer( tCanvas->mFgs );
	tCanvas->mFgs->Layout();

    addButtonBox();
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)
        Maximize( true );
    else
        SetSize( w, h );
    mSplitter->SetSashPosition( -dControlsHeight );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "", 2 );
    SetStatusText( "", 3 );
    SetStatusText( "", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
    mSplitter->SetSashPosition( -dControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void EasyHeaderFrame::initializeMenu ( void ) {
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
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EasyHeaderFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 0, 10, wxGROW );  //spacer
    if (m_buttonBox == NULL)
        m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    if (m_buttonSizer == NULL)
        m_buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    if (m_fgs == NULL)
        m_fgs = new wxFlexGridSizer( 3, 1, 1 );  //3 cols,vgap,hgap
    //row 1, col 1
	mHeaderLengthLabel = new wxStaticText( mControlPanel, -1, "Header Length:" );
	::setColor( mHeaderLengthLabel );
	m_fgs->Add( mHeaderLengthLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
	mHeaderLengthCtrl = new wxTextCtrl( mControlPanel, ID_HEADER_LENGTH, "0" );
	::setColor( mHeaderLengthCtrl );
	m_fgs->Add( mHeaderLengthCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
    //row 1, col 3
    mReadHeader = new wxButton( mControlPanel, ID_READ_HEADER, "Read Header", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mReadHeader );
    m_fgs->Add( mReadHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
	mSkipLabel = new wxStaticText( mControlPanel, -1, "Skip:" );
	::setColor( mSkipLabel );
	m_fgs->Add( mSkipLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	//row 2, col 2
	mSkipCtrl = new wxTextCtrl( mControlPanel, ID_SKIP, "0" );
	::setColor( mSkipCtrl );
	m_fgs->Add( mSkipCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	//row 2, col 3
    mWriteHeader = new wxButton( mControlPanel, ID_WRITE_HEADER, "Write Header", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mWriteHeader );
    m_fgs->Add( mWriteHeader, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
	mDataSlicesLabel = new wxStaticText( mControlPanel, -1, "Data Slices:" );
	::setColor( mDataSlicesLabel );
	m_fgs->Add( mDataSlicesLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	//row 3, col 2
	mDataSlicesCtrl = new wxTextCtrl( mControlPanel, ID_DATA_SLICES, "1" );
	::setColor( mDataSlicesCtrl  );
	m_fgs->Add( mDataSlicesCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
    //row 3, col 3
    mSwapBytes = new wxCheckBox( mControlPanel, ID_SWAP_BYTES, "Swap Bytes", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mSwapBytes );
    m_fgs->Add( mSwapBytes, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 4, col 1
	mIncrementLabel = new wxStaticText( mControlPanel, -1, "Increment:" );
	::setColor( mIncrementLabel );
	m_fgs->Add( mIncrementLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	//row 4, col 2
	mIncrementCtrl = new wxTextCtrl( mControlPanel, ID_INCREMENT, "1" );
	::setColor( mIncrementCtrl );
	m_fgs->Add( mIncrementCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	//row 4, col 3
	mWriteData = new wxButton( mControlPanel, ID_WRITE_DATA, "Write Data", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	::setColor( mWriteData );
	m_fgs->Add( mWriteData, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    m_buttonSizer->Add( m_fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( m_buttonSizer, 0, wxGROW|wxALL, 10 );
}

void EasyHeaderFrame::emptyButtonBox ( void )
{
    if (mWriteData!=NULL) {
        m_fgs->Detach(mWriteData);
        delete mWriteData;
    }
    if (mIncrementCtrl!=NULL) {
        m_fgs->Detach(mIncrementCtrl);
        delete mIncrementCtrl;
    }
    if (mIncrementLabel!=NULL) {
        m_fgs->Detach(mIncrementLabel);
        delete mIncrementLabel;
    }
    if (mSwapBytes!=NULL) {
        m_fgs->Detach(mSwapBytes);
        delete mSwapBytes;
    }
    if (mDataSlicesCtrl!=NULL) {
        m_fgs->Detach(mDataSlicesCtrl);
        delete mDataSlicesCtrl;
    }
    if (mDataSlicesLabel!=NULL) {
        m_fgs->Detach(mDataSlicesLabel);
        delete mDataSlicesLabel;
    }
    if (mWriteHeader!=NULL) {
        m_fgs->Detach(mWriteHeader);
        delete mWriteHeader;
    }
    if (mSkipCtrl!=NULL) {
        m_fgs->Detach(mSkipCtrl);
        delete mSkipCtrl;
    }
    if (mSkipLabel!=NULL) {
        m_fgs->Detach(mSkipLabel);
        delete mSkipLabel;
    }
    if (mReadHeader!=NULL) {
        m_fgs->Detach(mReadHeader);
        delete mReadHeader;
    }
    if (mHeaderLengthCtrl!=NULL) {
        m_fgs->Detach(mHeaderLengthCtrl);
        delete mHeaderLengthCtrl;
    }
    if (mHeaderLengthLabel!=NULL) {
        m_fgs->Detach(mHeaderLengthLabel);
        delete mHeaderLengthLabel;
    }
    mReadHeader = NULL;
	mSwapBytes = NULL;
    mWriteHeader = NULL;
	mWriteData = NULL;
	mHeaderLengthLabel = NULL;
	mHeaderLengthCtrl = NULL;
	mSkipLabel = NULL;
	mSkipCtrl = NULL;
	mDataSlicesLabel = NULL;
	mDataSlicesCtrl = NULL;
	mIncrementLabel = NULL;
	mIncrementCtrl = NULL;
}

void free_scene_header(ViewnixHeader *vh)
{
	if (vh->gen.description)
		free(vh->gen.description);
	if (vh->gen.comment)
		free(vh->gen.comment);
	if (vh->gen.imaged_nucleus)
		free(vh->gen.imaged_nucleus);
	if (vh->gen.gray_lookup_data)
		free(vh->gen.gray_lookup_data);
	if (vh->gen.red_lookup_data)
		free(vh->gen.red_lookup_data);
	if (vh->gen.green_lookup_data)
		free(vh->gen.green_lookup_data);
	if (vh->gen.blue_lookup_data)
		free(vh->gen.blue_lookup_data);
	if (vh->scn.domain)
		free(vh->scn.domain);
	if (vh->scn.axis_label)
		free(vh->scn.axis_label);
	if (vh->scn.measurement_unit)
		free(vh->scn.measurement_unit);
	if (vh->scn.density_measurement_unit)
		free(vh->scn.density_measurement_unit);
	if (vh->scn.smallest_density_value)
		free(vh->scn.smallest_density_value);
	if (vh->scn.largest_density_value)
		free(vh->scn.largest_density_value);
	if (vh->scn.signed_bits)
		free(vh->scn.signed_bits);
	if (vh->scn.bit_fields)
		free(vh->scn.bit_fields);
	if (vh->scn.num_of_subscenes)
		free(vh->scn.num_of_subscenes);
	if (vh->scn.loc_of_subscenes)
		free(vh->scn.loc_of_subscenes);
	if (vh->scn.description)
		free(vh->scn.description);
	free(vh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for EasyHeaderFrame class. */
EasyHeaderFrame::~EasyHeaderFrame ( void ) {
    cout << "EasyHeaderFrame::~EasyHeaderFrame" << endl;
    wxLogMessage( "EasyHeaderFrame::~EasyHeaderFrame" );

    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    deleteEasyHeaderControls();
    emptyButtonBox();
    if (m_fgs!=NULL)
    {
        m_buttonSizer->Detach(m_fgs);
        delete m_fgs;
        m_fgs = NULL;
    }
    if (m_buttonSizer!=NULL)
    {
        mBottomSizer->Detach(m_buttonSizer);
        delete m_buttonSizer;
        m_buttonSizer = NULL;
    }
    if (mControlPanel!=NULL)  {
        mBottomSizer->Detach(mControlPanel);
        delete mControlPanel;
        mControlPanel=NULL;
    }
	delete mDataInFile;
	mDataInFile = NULL;
	delete mOutFile;
	mOutFile = NULL;
	if (mVH)
		free_scene_header(mVH);
	mVH = NULL;

    if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }

    //if we are in the mode that supports having more than one window open
    // at a time, we need to remove this window from the list.
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
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
/** \brief callback for key presses. */
void EasyHeaderFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "EasyHeaderFrame::OnChar" );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new EasyHeaderFrame.  It first searches the input file history for an
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
void EasyHeaderFrame::createEasyHeaderFrame ( wxFrame* parentFrame, bool useHistory )
{
    EasyHeaderFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new EasyHeaderFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new EasyHeaderFrame( false, 800, 600 );
    }
    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void EasyHeaderFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();
    wxLogMessage("Nothing to copy.");
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for toggling control panel on and off. */
void EasyHeaderFrame::OnHideControls ( wxCommandEvent& e ) {
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
extern "C" {
int VGetHeaderLength ( FILE* fp, int* hdrlen );
}

void EasyHeaderFrame::OnInput ( wxCommandEvent& unused ) {
	wxFileDialog fd(this, "Select input file", "", "", "*",
		wxOPEN|wxFILE_MUST_EXIST);
	if (fd.ShowModal() == wxID_OK)
	{
		delete mDataInFile;
		mDataInFile = new wxFileName(fd.GetPath());
		FILE *fp = fopen((const char *)fd.GetPath().c_str(), "rb");
		if (fp == NULL)
			return;
		int hdrlen, error;
		error = VGetHeaderLength(fp, &hdrlen);
		if (error == 0)
			mHeaderLengthCtrl->SetValue(wxString::Format("%d", hdrlen));
		fclose(fp);
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// 6/28/07 Dewey Odhner 
void EasyHeaderFrame::deleteEasyHeaderControls(void)
{
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EasyHeaderFrame::OnReadHeader(wxCommandEvent &e)
{
	wxFileDialog fd(this, "Select input file", "", "",
		"Scene files (*.IM0;*.BIM)|*.IM0;*.BIM", wxOPEN|wxFILE_MUST_EXIST);
	if (fd.ShowModal() != wxID_OK)
		return;
	FILE *fp = fopen(fd.GetPath().c_str(), "rb");
	if (fp == NULL)
	{
		wxMessageBox("Could not open file.");
		return;
	}
	if (mVH)
		free_scene_header(mVH);
	mVH = (ViewnixHeader *)calloc(1, sizeof(*mVH));
	char grp[6], elem[6];
	int error = VReadHeader(fp, mVH, grp,elem);
	fclose(fp);
	switch (error)
	{
		case 0:
		case 106:
		case 107:
			break;
		default:
			wxMessageBox(wxString::Format(
				"VReadHeader returned error code %d\nGroup ", error)+grp+
				" Element "+elem);
			return;
	}
	tCanvas->mDateCtrl->SetValue(mVH->gen.study_date_valid?
		mVH->gen.study_date: "");
	tCanvas->mTimeCtrl->SetValue(mVH->gen.study_time_valid?
		mVH->gen.study_time: "");
	tCanvas->mStudyCtrl->SetValue(mVH->gen.study_valid? mVH->gen.study: "");
	tCanvas->mSeriesCtrl->SetValue(mVH->gen.series_valid? mVH->gen.series: "");
	tCanvas->mDimensionCtrl->SetValue(mVH->scn.dimension==4? "4": "3");
	tCanvas->mSliceWidthCtrl->SetValue(
		wxString::Format("%d", mVH->scn.xysize[0]));
	tCanvas->mSliceHeightCtrl->SetValue(
		wxString::Format("%d", mVH->scn.xysize[1]));
	tCanvas->mBitsPerPixelCtrl->SetValue(
		wxString::Format("%d", mVH->scn.num_of_bits));
	tCanvas->mPixelSizeCtrl->SetValue(
		wxString::Format("%f", mVH->scn.xypixsz[0]));
	tCanvas->mMinDensityCtrl->SetValue(mVH->scn.smallest_density_value_valid?
		wxString::Format("%.0f", mVH->scn.smallest_density_value[0]): "");
	tCanvas->mMaxDensityCtrl->SetValue(mVH->scn.largest_density_value_valid?
		wxString::Format("%.0f", mVH->scn.largest_density_value[0]): "");
	if (mVH->scn.dimension == 4)
	{
		if (tCanvas->mNumVolumesLabel == NULL)
			OnDimension(e);
		tCanvas->mNumSlicesCtrl->SetValue(
			wxString::Format("%d", mVH->scn.num_of_subscenes[1]));
		tCanvas->mFirstSliceLocCtrl->SetValue(wxString::Format("%f",
			mVH->scn.loc_of_subscenes[mVH->scn.num_of_subscenes[0]]));
		if (mVH->scn.num_of_subscenes[1] > 1)
			tCanvas->mSliceSpacingCtrl->SetValue(wxString::Format("%f",
				mVH->scn.loc_of_subscenes[mVH->scn.num_of_subscenes[0]+1]-
				mVH->scn.loc_of_subscenes[mVH->scn.num_of_subscenes[0]]));
		tCanvas->mNumVolumesCtrl->SetValue(
			wxString::Format("%d", mVH->scn.num_of_subscenes[0]));
		tCanvas->mFirstVolumeLocCtrl->SetValue(
			wxString::Format("%f", mVH->scn.loc_of_subscenes[0]));
		if (mVH->scn.num_of_subscenes[0] > 1)
			tCanvas->mVolumeSpacingCtrl->SetValue(wxString::Format("%f",
				mVH->scn.loc_of_subscenes[1]-mVH->scn.loc_of_subscenes[0]));
	}
	else
	{
		if (tCanvas->mNumVolumesLabel)
			tCanvas->removeVolumeControls();
		tCanvas->mNumSlicesCtrl->SetValue(
			wxString::Format("%d", mVH->scn.num_of_subscenes[0]));
		tCanvas->mFirstSliceLocCtrl->SetValue(
			wxString::Format("%f", mVH->scn.loc_of_subscenes[0]));
		if (mVH->scn.num_of_subscenes[0] > 1)
			tCanvas->mSliceSpacingCtrl->SetValue(wxString::Format("%f",
				mVH->scn.loc_of_subscenes[1]-mVH->scn.loc_of_subscenes[0]));
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EasyHeaderFrame::OnWriteHeader(wxCommandEvent &e)
{
	long bits = 8;
	tCanvas->mBitsPerPixelCtrl->GetValue().ToLong(&bits);
	if (bits==1 && mOutFile->GetExt()!="BIM")
		mOutFile->SetExt("BIM");
	else if (bits>1 && mOutFile->GetExt()!="IM0")
		mOutFile->SetExt("IM0");
	wxString pattern(bits==1? "BIM files (*.BIM)|*.BIM":
		"IM0 files (*.IM0)|*.IM0");
    wxFileDialog saveDlg(this,
                _T("Save header"),
                mOutFile->GetPath(),
                mOutFile->GetFullName(),
                pattern,
                wxSAVE|wxOVERWRITE_PROMPT);
    if (saveDlg.ShowModal() != wxID_OK)
		return;
	delete mOutFile;
	mOutFile = new wxFileName(saveDlg.GetPath());
	if (mVH == NULL)
		mVH = (ViewnixHeader *)calloc(1, sizeof(*mVH));
	if (mVH == NULL)
	{
		wxMessageBox("Out of Memory.");
		return;
	}
	strcpy(mVH->gen.recognition_code, "VIEWNIX1.0");
	mVH->gen.recognition_code_valid = 1;
	mVH->gen.data_type = IMAGE0;
	mVH->gen.data_type_valid = 1;
	mVH->scn.num_of_density_values = 1;
	mVH->scn.num_of_density_values_valid = 1;
	mVH->scn.num_of_integers = 1;
	mVH->scn.num_of_integers_valid = 1;
	mVH->scn.num_of_bits = bits;
	mVH->scn.num_of_bits_valid = 1;
	if (mVH->scn.bit_fields == NULL)
		mVH->scn.bit_fields = (short *)malloc(2*sizeof(float));
	mVH->scn.bit_fields[0] = 0;
	mVH->scn.bit_fields[1] = bits-1;
	mVH->scn.bit_fields_valid = 1;
	strncpy(mVH->gen.study_date, (const char *)tCanvas->mDateCtrl->GetValue().c_str(),
		sizeof(mVH->gen.study_date)-1);
	mVH->gen.study_date_valid = mVH->gen.study_date[0]!=0;
	strncpy(mVH->gen.study_time, (const char *)tCanvas->mTimeCtrl->GetValue().c_str(),
		sizeof(mVH->gen.study_time)-1);
	mVH->gen.study_time_valid = mVH->gen.study_time[0]!=0;
	strncpy(mVH->gen.study, (const char *)tCanvas->mStudyCtrl->GetValue().c_str(),
		sizeof(mVH->gen.study)-1);
	mVH->gen.study_valid = mVH->gen.study[0]!=0;
	strncpy(mVH->gen.series, (const char *)tCanvas->mSeriesCtrl->GetValue().c_str(),
		sizeof(mVH->gen.series)-1);
	mVH->gen.series_valid = mVH->gen.series[0]!=0;
	long dimension;
	tCanvas->mDimensionCtrl->GetValue().ToLong(&dimension);
	if (mVH->scn.dimension != dimension)
	{
		if (mVH->scn.domain)
			free(mVH->scn.domain);
		mVH->scn.domain = NULL;
		mVH->scn.domain_valid = 0;
		if (mVH->scn.axis_label)
			free(mVH->scn.axis_label);
		mVH->scn.axis_label = NULL;
		mVH->scn.axis_label_valid = 0;
		if (mVH->scn.measurement_unit)
			free(mVH->scn.measurement_unit);
		mVH->scn.measurement_unit = NULL;
		mVH->scn.measurement_unit_valid = 0;
		if (mVH->scn.num_of_subscenes)
			free(mVH->scn.num_of_subscenes);
		mVH->scn.num_of_subscenes = NULL;
		if (mVH->scn.loc_of_subscenes)
			free(mVH->scn.loc_of_subscenes);
		mVH->scn.loc_of_subscenes = NULL;
	}
	mVH->scn.dimension = dimension;
	mVH->scn.dimension_valid = 1;
	long width, height;
	if (tCanvas->mSliceWidthCtrl->GetValue().ToLong(&width) && width>0 &&
			tCanvas->mSliceHeightCtrl->GetValue().ToLong(&height) && height>0)
	{
		mVH->scn.xysize[0] = (short)width;
		mVH->scn.xysize[1] = (short)height;
		mVH->scn.xysize_valid = 1;
	}
	else
	{
		wxMessageBox("Image size is not valid.");
		return;
	}
	double pix_size;
	if (tCanvas->mPixelSizeCtrl->GetValue().ToDouble(&pix_size) && pix_size>0)
	{
		mVH->scn.xypixsz[0] = mVH->scn.xypixsz[1] = (float)pix_size;
		mVH->scn.xypixsz_valid = 1;
	}
	else
	{
		wxMessageBox("Pixel size is not valid.");
		return;
	}
	double min_v;
	if (tCanvas->mMinDensityCtrl->GetValue().ToDouble(&min_v))
	{
		if (mVH->scn.smallest_density_value == NULL)
			mVH->scn.smallest_density_value = (float *)malloc(sizeof(float));
		mVH->scn.smallest_density_value[0] = (float)min_v;
		mVH->scn.smallest_density_value_valid = 1;
	}
	double max_v;
	if (tCanvas->mMaxDensityCtrl->GetValue().ToDouble(&max_v))
	{
		if (mVH->scn.largest_density_value == NULL)
			mVH->scn.largest_density_value = (float *)malloc(sizeof(float));
		mVH->scn.largest_density_value[0] = (float)max_v;
		mVH->scn.largest_density_value_valid = 1;
	}
	long slices;
	if (!tCanvas->mNumSlicesCtrl->GetValue().ToLong(&slices) || slices<=0)
	{
		wxMessageBox("Number of slices is not valid.");
		return;
	}
	double sl_spacing;
	tCanvas->mSliceSpacingCtrl->GetValue().ToDouble(&sl_spacing);
	double sl_loc;
	tCanvas->mFirstSliceLocCtrl->GetValue().ToDouble(&sl_loc);
	if (dimension == 4)
	{
		long volumes;
		if (!tCanvas->mNumVolumesCtrl->GetValue().ToLong(&volumes)||volumes<=0)
		{
			wxMessageBox("Number of volumes is not valid.");
			return;
		}
		double vol_spacing;
		tCanvas->mVolumeSpacingCtrl->GetValue().ToDouble(&vol_spacing);
		double vol_loc;
		tCanvas->mFirstVolumeLocCtrl->GetValue().ToDouble(&vol_loc);
		if (mVH->scn.num_of_subscenes)
			free(mVH->scn.num_of_subscenes);
		if (mVH->scn.loc_of_subscenes)
			free(mVH->scn.loc_of_subscenes);
		mVH->scn.num_of_subscenes = (short *)malloc((volumes+1)*sizeof(short));
		mVH->scn.loc_of_subscenes =
			(float *)malloc((1+volumes*(1+slices))*sizeof(float));
		mVH->scn.num_of_subscenes[0] = (short)volumes;
		for (int j=0; j<volumes; j++)
		{
			mVH->scn.num_of_subscenes[1+j] = (short)slices;
			mVH->scn.loc_of_subscenes[j] = (float)(vol_loc+j*vol_spacing);
			for (int k=0; k<slices; k++)
				mVH->scn.loc_of_subscenes[volumes+(j*slices)+k] =
					(float)(sl_loc+k*sl_spacing);
		}
	}
	else
	{
		assert(dimension == 3);
		if (mVH->scn.num_of_subscenes)
		{
			if (mVH->scn.num_of_subscenes[0] != slices)
			{
				free(mVH->scn.loc_of_subscenes);
				mVH->scn.loc_of_subscenes = NULL;
			}
		}
		else
			mVH->scn.num_of_subscenes = (short *)malloc(sizeof(short));
		mVH->scn.num_of_subscenes[0] = (short)slices;
		if (mVH->scn.loc_of_subscenes == NULL)
			mVH->scn.loc_of_subscenes = (float *)malloc(slices*sizeof(float));
		for (int j=0; j<slices; j++)
			mVH->scn.loc_of_subscenes[j] = (float)(sl_loc+j*sl_spacing);
	}
	mVH->scn.num_of_subscenes_valid = mVH->scn.loc_of_subscenes_valid = 1;
	FILE *fp = fopen((const char *)mOutFile->GetFullPath().c_str(), "wb+");
	if (fp == NULL)
	{
		wxMessageBox("Could not open output file.");
		return;
	}
	char grp[6], elem[6];
	int error = VWriteHeader(fp, mVH, grp,elem);
	switch (error)
	{
		case 0:
		case 106:
		case 107:
			break;
		default:
			wxMessageBox(wxString::Format(
				"VWriteHeader returned error code %d\nGroup ", error)+grp+
				" Element "+elem);
			break;
	}
	fclose(fp);
}

void EasyHeaderFrame::OnWriteData ( wxCommandEvent& e )
{
	if (mDataInFile == NULL)
	{
		wxMessageBox("Select an input file first.");
		return;
	}
	long bits = 8;
	tCanvas->mBitsPerPixelCtrl->GetValue().ToLong(&bits);
	bool swap_bytes = mSwapBytes->IsChecked();
	if (swap_bytes && bits!=16)
	{
		wxMessageBox("Swap Bytes requires 16 bits per pixel.");
		return;
	}
	FILE *fpin = fopen((const char *)mDataInFile->GetFullPath().c_str(), "rb");
	if (fpin == NULL)
	{
		wxMessageBox("Could not open the input file.");
		return;
	}
	FILE *fpout;
	wxFileDialog saveDlg(this, "Save data", mOutFile->GetPath(),
		mOutFile->GetFullName(), "*", wxSAVE);
	if (saveDlg.ShowModal() != wxID_OK)
	{
		fclose(fpin);
		return;
	}
	fpout = fopen((const char *)saveDlg.GetPath().c_str(), "ab+");
	if (fpout == NULL)
	{
		wxMessageBox("Could not open the output file.");
		fclose(fpin);
		return;
	}
	long width, height;
	if (!tCanvas->mSliceWidthCtrl->GetValue().ToLong(&width) || width<=0 ||
			!tCanvas->mSliceHeightCtrl->GetValue().ToLong(&height) ||height<=0)
	{
		wxMessageBox("Slice dimensions are not valid.");
		fclose(fpin);
		fclose(fpout);
		return;
	}
	long hdr_len, skip, slices, incr;
	if (!mHeaderLengthCtrl->GetValue().ToLong(&hdr_len) ||
			!mSkipCtrl->GetValue().ToLong(&skip) ||
			!mDataSlicesCtrl->GetValue().ToLong(&slices) ||
			!mIncrementCtrl->GetValue().ToLong(&incr))
	{
		wxMessageBox("Specified values are not valid.");
		fclose(fpin);
		fclose(fpout);
		return;
	}
	unsigned long size=(width*height*bits+7)/8;
	char *sl=(char *)malloc(size);
	if (sl==NULL) {
		wxMessageBox("Out of memory.");
		fclose(fpin);
		fclose(fpout);
		return;
	}
	if (fseek(fpin,hdr_len+size*skip,0L))
	{
		wxMessageBox("File seek failed.");
		fclose(fpin);
		fclose(fpout);
		free(sl);
		return;
	}
	for (int j=0; j<slices; j++)
	{
		if (fread(sl,1,size,fpin)!=size)
		{
			wxMessageBox("Error in reading slice");
			break;
		}
		if (swap_bytes)
			for (unsigned int k=0; k<size; k+=2)
			{
				char c = sl[k];
				sl[k] = sl[k+1];
				sl[k+1] = c;
			}
		if (fwrite(sl,1,size,fpout)!=size)
		{
			wxMessageBox("Error in writing slice");
			break;
		}
		if (fseek(fpin,size*(incr-1),1)==-1)
		{
			wxMessageBox("Error in seeking to next slice");
			break;
		}
	}
	fclose(fpin);
	fclose(fpout);
	free(sl);
}

void EasyHeaderFrame::OnDimension ( wxCommandEvent& e )
{
	if (e.GetString() == "3")
	{
		tCanvas->removeVolumeControls();
		return;
	}
	tCanvas->mNumVolumesLabel = new wxStaticText( tCanvas, -1, "# Volumes:" );
	::setColor( tCanvas->mNumVolumesLabel );
	tCanvas->mFgs->Add( tCanvas->mNumVolumesLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mNumVolumesCtrl = new wxTextCtrl( tCanvas, ID_VOLUMES, "1" );
	::setColor( tCanvas->mNumVolumesCtrl );
	tCanvas->mFgs->Add( tCanvas->mNumVolumesCtrl, 0, wxGROW|wxLEFT|wxRIGHT );
	tCanvas->mFirstVolumeLocLabel = new wxStaticText( tCanvas, -1, "1st Volume Loc.:" );
	::setColor( tCanvas->mFirstVolumeLocLabel );
	tCanvas->mFgs->Add( tCanvas->mFirstVolumeLocLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mFirstVolumeLocCtrl = new wxTextCtrl( tCanvas, ID_FIRST_VOLUME_LOC, "0.0" );
	::setColor( tCanvas->mFirstVolumeLocCtrl );
	tCanvas->mFgs->Add(tCanvas->mFirstVolumeLocCtrl, 0, wxGROW|wxLEFT|wxRIGHT);
	tCanvas->mVolumeSpacingLabel = new wxStaticText( tCanvas, -1, "Volume Spacing:" );
	::setColor( tCanvas->mVolumeSpacingLabel );
	tCanvas->mFgs->Add( tCanvas->mVolumeSpacingLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 0 );
	tCanvas->mVolumeSpacingCtrl = new wxTextCtrl( tCanvas, ID_VOLUME_SPACING, "1.0" );
	::setColor( tCanvas->mVolumeSpacingCtrl );
	tCanvas->mFgs->Add( tCanvas->mVolumeSpacingCtrl, 0, wxGROW|wxLEFT|wxRIGHT);
	tCanvas->mFgs->Layout();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void EasyHeaderFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrintPreview*    preview = new wxPrintPreview(
        new EasyHeaderPrint(wxString("CAVASS").c_str(), tCanvas),
        new EasyHeaderPrint(wxString("CAVASS").c_str(), tCanvas),
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
void EasyHeaderFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    EasyHeaderPrint*  printout;
    printout = new EasyHeaderPrint( wxString("CAVASS:EasyHeader").c_str(), tCanvas );
    if (!printer.Print(this, printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( EasyHeaderFrame, wxFrame )
BEGIN_EVENT_TABLE       ( EasyHeaderFrame, wxFrame )
  DefineStandardFrameCallbacks
  EVT_BUTTON( ID_READ_HEADER,    EasyHeaderFrame::OnReadHeader   )
  EVT_BUTTON( ID_WRITE_HEADER,   EasyHeaderFrame::OnWriteHeader  )
  EVT_BUTTON( ID_WRITE_DATA,     EasyHeaderFrame::OnWriteData    )
  EVT_COMBOBOX( ID_DIMENSION,    EasyHeaderFrame::OnDimension    )
END_EVENT_TABLE()

wxFileHistory  EasyHeaderFrame::_fileHistory;
//======================================================================
