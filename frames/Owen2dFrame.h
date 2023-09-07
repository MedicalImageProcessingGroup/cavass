/*
  Copyright 1993-2018, 2022 Medical Image Processing Group
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
 * \file   Owen2dFrame.h
 * \brief  Owen2dFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This example loads a 3D CAVASS data file and displays a single slice
 * from it.  The user can display previous and next slices and can change
 * the contrast (gray map).
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __Owen2dFrame_h
#define __Owen2dFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "Owen2dCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  GrayMapControls;
class  SetIndexControls;
class  SetOwen2dOutputControls;
class  Owen2dAuxControls;

/** \brief Owen2dFrame class definition.
 *
 *  This simple example implements a simple frame that displays one
 *  slice from a data set.
 <p>
  This example loads a 3D CAVASS data file and displays a single slice
  from it.  The user can display previous and next slices and can change
  the contrast (gray map).  The user may also drag the image around the 
  canvas as well (to illustrate some interaction with mouse events).
  A frame (window) consists of a menu bar, a canvas (drawing area), and
  a control panel area towards the bottom of the frame.
  This example consists of the following files:
    Owen2dFrame.cpp,  Owen2dFrame.h, Owen2dCanvas.cpp, and Owen2dCanvas.h.
  This example also uses the CavassData class to read the image data.
  CavassData is a general wrapper class that one may use to read various
  image data file formats such as 3DVIEWNIX (.BIM, .IM0), DICOM (.dcm,
  .dicom), JPEG (.jpg, .jpeg), TIFF (.tif, .tiff), PNG (.png), VTK (.vtk),
  and GIF (.gif).

  CavassData may also be initialized directly from memory, rather
  than from by reading a file, as well.
 </p>
 <p>
  Our convention is that an <em>app</em> or <em>module</em> consists
  of an <em>App</em>Frame class and an <em>App</em>Canvas class.
  All of the *Frame classes are subclasses of MainFrame.
  *Frame classes are the main app frame (window) and consist of 3 regions:
  (a) menu bar, (b) drawing area (canvas), and a control are towards the
  bottom third of the window.
  All of the *Canvas classes are subclasses of MainCanvas.
  *Canvas classes perform drawing and handle mouse move and click events.
 </p>
 <p>
  Other CAVASS apps/modules consist of Overlay, SurfView, and Montage.
  Overlay files are
      OverlayFrame.cpp,  OverlayFrame.h,
      OverlayCanvas.cpp, OverlayCanvas.h, and
      OverlayPrint.h.
  SurfView files are
      SurfViewFrame.cpp,  SurfViewFrame.h,
      SurfViewCanvas.cpp, SurfViewCanvas.h, and
      SurfViewPrint.h.
  Montage files are:
      MontageFrame.cpp,  MontageFrame.h,
      MontageCanvas.cpp, MontageCanvas.h, and
      MontagePrint.h.
  \todo These other modules need to be updated to be in accord with the Owen2d app.
 </p>
 <p>
  The standard controls (that one is encouraged to use) are
  CineControls (in CineControls.h),
  ColorControls (in ColorControls.h),
  GrayMapControls (in GrayMapControls.h),
  SaveScreenControls (in SaveScreenControls.h),
  and SetIndexControls (in SetIndexControls.h).
  This set is expected to expand and reuse and standardization is
  encouraged.
  This example illustrates GrayMapControls, and SaveScreenControls.
  Implementation of the callbacks are, of course, the responsibility
  of the app but defining the controls in this manner allows us to
  standardize the look-and-feel across all apps.
 </p>
 <p>
  Once the frame, canvas, and controls have been coded for an app,
  one must add the new app to the main, standard CAVASS application menu
  bar.  In this example, we'll add the Owen2d app to the Help menu.
 </p>
 <ol>
  <li> Add ID_PP_SCOPS_SEGMENT_OWEN to the enum in MainFrame.h (so it can be added to the
       menu bar).
       <pre>
       </pre> </li>
  <li> Add the following callback definition to MainFrame.h (this is the
       callback when Owen2d is chosen from the menu bar):
       <pre>
           void OnOwen2d ( wxCommandEvent& e );
       </pre> </li>
  <li> Add the following to the DefineStandardFrameCallbacks macro in
       MainFrame.h (to associate the ID with the callback function):
       <pre>
           EVT_MENU( ID_PP_SCOPS_SEGMENT_OWEN, MainFrame::OnOwen2d      )    \
       </pre> </li>
  <li> Add the following to MainFrame.cpp (this is the callback function that
       will be called when Owen2d is chosen from the menu bar):
       <pre>
           void MainFrame::OnOwen2d ( wxCommandEvent& unused ) {
               Owen2dFrame::createOwen2dFrame( this );
           }
       </pre>
       Using Owen2dFrame::createOwen2dFrame() in this manner allows one 
       to customize file selection code.  For example, overlay requires
       two files so the OnOverlay method asks the user to select two files.
       Or you may wish to change the various file types that can be used as
       input to your particular app.
       <pre>
       </pre>
  </li>
  <li> Add the following to MainFrame.cpp:
       <pre>
           #include  "Owen2dFrame.h"
       </pre> </li>
  <li> Add the following to MainFrame::initializeMenu() in MainFrame.cpp:
       <pre>
           help_menu->Append( ID_PP_SCOPS_SEGMENT_OWEN, "&Owen2d" );
       Owen</li>
 </ol>
 <h3> A note about callbacks... </h3>
 <p>
  The macro, DefineStandardFrameCallbacks, is defined in MainFrame.h.  It is
  used to define standard callback table entries (so they need not be
  repeated in each app).  Callback functions are defined as being virtual
  so they can be completely replaced via inheritance (when necessary).
 </p>
 */
class Owen2dFrame : public MainFrame {
  protected:
    GrayMapControls*  mGrayMapControls;     ///< gray map controls
    SetIndexControls*    mSetIndexControls;
	SetOwen2dOutputControls *mSetOutputControls;
	Owen2dAuxControls *mAuxControls;
  public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_PREVIOUS=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_NEXT,
        ID_SET_INDEX,
        ID_GRAYMAP, ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        ID_RESET,
		ID_SLICE_SLIDER,
        ID_SCALE_SLIDER,
        ID_ZOOM_IN, ID_ZOOM_OUT,
        ID_LABELS, ID_LAYOUT, ID_OVERLAY, ID_BUSY_TIMER,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
		ID_OBJECT, ID_OUT_OBJECT,
        ID_SET_OUTPUT, ID_OUT_TYPE, ID_SAVE,
		ID_OVERLAY_COLOR,
		ID_MODE, ID_BRUSH_SIZE, ID_ITERATES, ID_DELETE_OBJECT,
		ID_LOAD_OBJECT, ID_DEFAULT_FILL,
		ID_FEATURE_DISPLAY, ID_FEATURE, ID_FEATURE_STATUS,
		ID_TRANSFORM, ID_TRANSFORM_SELECT, ID_FEATURE_UPDATE,
		ID_MIN_POINTS, ID_ALPHA, ID_BETA, ID_GAMMA,
		ID_WEIGHT, ID_MEAN, ID_STD_DEV, ID_FEATURE_MIN, ID_FEATURE_MAX
    };
    int  mFileOrDataCount;  ///< count data/datafiles used (1 for this example).
	Owen2dCanvas *tCanvas;
	wxString modeName[NUM_SEG2D_MODES+Owen2dCanvas::LWOF];
	wxString objectName[9];

  protected:
	wxSizer*       m_buttonSizer;
    wxFlexGridSizer*  fgs;

	wxButton*      m_prev;
	wxButton*      m_next;
	wxButton*      m_setIndex;
	wxButton*      m_grayMap;
	wxStaticText  *mModeLabel;
	wxComboBox    *mMode;
	wxButton*      m_reset;
	wxStaticText  *m_objectLabel;
	wxComboBox    *m_object;
	wxButton      *m_deleteObject;
	wxButton      *m_loadObject;
	wxCheckBox    *m_defaultFill;
	wxCheckBox    *m_overlay;
	wxCheckBox    *m_layout;
	wxButton*      m_setOutputBut;
	int out_object;
	int OUTPUT_DATA_TYPE;

    void initializeMenu ( void );
    void addButtonBox ( void );
	void deleteOwen2dControls ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createOwen2dFrame ( wxFrame* parentFrame, bool useHistory=true );
    Owen2dFrame ( bool maximize=false, int w=800, int h=600 );
    ~Owen2dFrame ( void );
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };

	virtual void flush_temp_data( void );
    void loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    virtual void OnGrayMap         ( wxCommandEvent& unused );
    virtual void OnInput           ( wxCommandEvent& unused );
	virtual void OnMouseWheel      ( wxMouseEvent& e );
    virtual void OnNext            ( wxCommandEvent& unused );
    virtual void OnOpen            ( wxCommandEvent& unused );
    virtual void OnPrevious        ( wxCommandEvent& unused );
    virtual void OnPrint           ( wxCommandEvent& unused );
    virtual void OnPrintPreview    ( wxCommandEvent& unused );

    void OnInvert       ( wxCommandEvent& e );
    void OnOverlay      ( wxCommandEvent& e );
	void OnLabels       ( wxCommandEvent& e );
	void OnLayout       ( wxCommandEvent& e );

    virtual void OnCenterSlider ( wxScrollEvent& e );
    virtual void OnWidthSlider  ( wxScrollEvent& e );
	virtual void OnCTLung       ( wxCommandEvent& unused );
	virtual void OnCTSoftTissue ( wxCommandEvent& unused );
	virtual void OnCTBone       ( wxCommandEvent& unused );
    virtual void OnPET          ( wxCommandEvent& unused );
    virtual void OnSliceSlider  ( wxScrollEvent& e );
    virtual void OnScaleSlider  ( wxScrollEvent& e );
#ifdef __WXX11__
    virtual void OnUpdateUICenterSlider ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIWidthSlider  ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUISliceSlider  ( wxUpdateUIEvent& e );
    virtual void OnUpdateUIScaleSlider  ( wxUpdateUIEvent& e );
#endif
    void OnSetIndex     ( wxCommandEvent& unused );
	void OnMode         ( wxCommandEvent& e );
    void OnReset        ( wxCommandEvent& unused );
	void OnBrushSize    ( wxCommandEvent& e );
	void OnIterates     ( wxCommandEvent& e );
	void OnObject       ( wxCommandEvent& e );
	void OnDeleteObject ( wxCommandEvent& unused );
	void OnLoadObject   ( wxCommandEvent& unused );
	void OnDefaultFill  ( wxCommandEvent& e );
	void OnOutputType   ( wxCommandEvent& unused );
	void OnSetOutput    ( wxCommandEvent& unused );
	void OnOutputObject ( wxCommandEvent& e );
    void OnOwen2dSave( wxCommandEvent& e );
	void OnFeatureDisplay( wxCommandEvent& unused );
	void OnFeature      ( wxCommandEvent& e );
	void OnFeatureStatus( wxCommandEvent& e );
	void OnTransform    ( wxCommandEvent& e );
	void OnTransformSelect( wxCommandEvent& unused );
	void OnFeatureUpdate( wxCommandEvent& unused );
	void OnMinPoints    ( wxCommandEvent& e );
	void OnAlpha        ( wxCommandEvent& e );
	void OnBeta         ( wxCommandEvent& e );
	void OnGamma        ( wxCommandEvent& e );
	void OnWeight       ( wxScrollEvent& e );
	void OnMean         ( wxScrollEvent& e );
	void OnStdDev       ( wxScrollEvent& e );
	void OnFeatureMin   ( wxScrollEvent& e );
	void OnFeatureMax   ( wxScrollEvent& e );

    static void OnHelp ( wxCommandEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( Owen2dFrame )
    DECLARE_EVENT_TABLE()
};

extern char Owen2dFeatureNames[6][15];
extern char Owen2dTransformNames[6][16];

class Owen2dSlider {
	wxFlexGridSizer  *mFgs;
	wxFlexGridSizer  *mFgsSlider;
	wxStaticText     *mSt;
	wxStaticText     *mSt1;
	wxStaticText     *mSt2;
	wxStaticText     *mSt3;
	wxSlider         *mSlider;

public:
	Owen2dSlider( wxPanel* cp, wxFlexGridSizer *fgs, const wxString title,
		int actionId, double min_val, double max_val, double current_val )
	{
		mFgs = fgs;
		mSt = new wxStaticText( cp, -1, title+":" );
		::setColor( mSt );
		mFgs->Add( mSt, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );

		mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
		mFgsSlider->AddGrowableCol( 0 );

		mFgsSlider->AddStretchSpacer();

		mSlider = new wxSlider( cp, actionId, (int)(current_val*100+0.5),
			(int)(min_val*100+0.5), (int)(max_val*100+0.5),
			wxDefaultPosition, wxSize(sliderWidth, -1),
			wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, title );
		::setColor( mSlider );
		mSlider->SetPageSize( 5 );
		mFgsSlider->Add( mSlider, 0, wxGROW|wxLEFT|wxRIGHT );

		mFgsSlider->AddStretchSpacer();

		wxString  s = wxString::Format( "%5.2f", min_val );
		mSt1 = new wxStaticText( cp, -1, s );
		::setColor( mSt1 );
		mFgsSlider->Add( mSt1, 0, wxALIGN_RIGHT );

		s = wxString::Format( "%5.2f", current_val );
		mSt2 = new wxStaticText( cp, -1, s );
		::setColor( mSt2 );
		mFgsSlider->Add( mSt2, 0, wxALIGN_CENTER );

		s = wxString::Format( "%5.2f", max_val );
		mSt3 = new wxStaticText( cp, -1, s );
		::setColor( mSt3 );
		mFgsSlider->Add( mSt3, 0, wxALIGN_LEFT );

		mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
	}
	~Owen2dSlider()
	{
		mFgsSlider->Detach(mSt3);
		delete mSt3;
		mFgsSlider->Detach(mSt2);
		delete mSt2;
		mFgsSlider->Detach(mSt1);
		delete mSt1;
		mFgsSlider->Detach(mSlider);
		delete mSlider;
		mFgs->Detach(mSt);
		delete mSt;
		mFgs->Remove( mFgsSlider );
	}
	void UpdateValue( double new_val )
	{
		mSt2->SetLabel( wxString::Format( "%5.2f", new_val ) );
	}
};

class Owen2dAuxControls {
    wxPanel          *mCp;
	wxSizer          *mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer  *mFgs;
	wxFlexGridSizer  *mFgsButton;
	wxFlexGridSizer  *mFgsSliders;
    wxStaticBox      *mAuxBox;
	wxStaticBoxSizer *mAuxSizer;
	wxStaticText     *mSt1, *mSt2, *mSt3, *mSt4;
	wxComboBox       *mBrushSize;
	wxComboBox       *mIterates;
	wxButton         *mFeatureDisplay;
	wxComboBox       *mFeature;
	wxCheckBox       *mFeatureStatus;
	wxComboBox       *mTransform;
	wxButton         *mTransformSelect;
	wxButton         *mUpdate;
	wxTextCtrl       *mMinPointsCtrl;
	wxTextCtrl       *mAlphaCtrl;
	wxTextCtrl       *mBetaCtrl;
	wxTextCtrl       *mGammaCtrl;
	Owen2dSlider  *mWeight;
	Owen2dSlider  *mMean;
	Owen2dSlider  *mStdDev;
	Owen2dSlider  *mFeatureMin;
	Owen2dSlider  *mFeatureMax;

	void setup_fgs(wxPanel* cp, wxSizer* bottomSizer, const char * const title,
		int cols)
	{
		mSt1 = mSt2 = mSt3 = mSt4 = NULL;
		mBrushSize = NULL;
		mIterates = NULL;
		mFeatureDisplay = NULL;
		mFeature = NULL;
		mFeatureStatus = NULL;
		mTransform = NULL;
		mTransformSelect = NULL;
		mUpdate = NULL;
		mMinPointsCtrl = NULL;
		mAlphaCtrl = NULL;
		mBetaCtrl = NULL;
		mGammaCtrl = NULL;
		mWeight = NULL;
		mMean = NULL;
		mStdDev = NULL;
		mFeatureMin = NULL;
		mFeatureMax = NULL;
		mFgsSliders = NULL;

		mCp = cp;
        mBottomSizer = bottomSizer;
        mAuxBox = new wxStaticBox( cp, -1, title );
        ::setColor( mAuxBox );
        mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
		mFgsButton = new wxFlexGridSizer( cols, 1, 1 );  // cols,vgap,hgap
	}
	void display_fgs()
	{
		if (mFgsSliders)
			mFgs->Add( mFgsSliders, 0, wxGROW|wxALL, 10 );
		mFgs->Add( mFgsButton, 0, wxGROW|wxALL, 10 );
        mAuxSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Layout();
		mCp->Refresh();
	}

public:
	Owen2dAuxControls( wxPanel* cp, wxSizer* bottomSizer,
		const char * const title, int currentBrushSize,
		bool update_flag=false);

	Owen2dAuxControls( wxPanel* cp, wxSizer* bottomSizer,
		const char * const title, int currentIterates, int currentMinPoints )
	{
		setup_fgs(cp, bottomSizer, title, 2);
		mSt1 = new wxStaticText( cp, wxID_ANY, "Iterates:" );
		::setColor( mSt1 );
		mFgsButton->Add( mSt1, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		wxArrayString as;
		for (int j=1; j<=8; j++)
			as.Add(wxString::Format("%d", j));
		mIterates = new wxComboBox(cp, Owen2dFrame::ID_ITERATES,
			wxString::Format("%d", currentIterates),
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
			as, wxCB_READONLY );
		::setColor( mIterates );
		mFgsButton->Add( mIterates, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mSt2 = new wxStaticText( cp, wxID_ANY, "Min. Ctrl. Pts.:" );
		::setColor( mSt2 );
		mFgsButton->Add( mSt2, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mMinPointsCtrl = new wxTextCtrl( cp, Owen2dFrame::ID_MIN_POINTS,
			wxString::Format("%d", currentMinPoints) );
		::setColor( mMinPointsCtrl );
		mFgsButton->Add( mMinPointsCtrl, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		display_fgs();
	}

	Owen2dAuxControls( wxPanel* cp, wxSizer* bottomSizer,
		const char * const title, int currentIterates,
		double currentAlpha, double currentBeta, double currentGamma )
	{
		setup_fgs(cp, bottomSizer, title, 2);
		mSt1 = new wxStaticText( cp, wxID_ANY, "Iterates:" );
		::setColor( mSt1 );
		mFgsButton->Add( mSt1, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		wxArrayString as;
		for (int j=1; j<=9; j++)
			as.Add(wxString::Format("%d", j<6? j: (j-4)*5));
		mIterates = new wxComboBox(cp, Owen2dFrame::ID_ITERATES,
			wxString::Format("%d",
				currentIterates<6? currentIterates: (currentIterates-4)*5),
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
			as, wxCB_READONLY );
		::setColor( mIterates );
		mFgsButton->Add( mIterates, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mSt2 = new wxStaticText( cp, wxID_ANY, "alpha:" );
		::setColor( mSt2 );
		mFgsButton->Add( mSt2, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mAlphaCtrl = new wxTextCtrl( cp, Owen2dFrame::ID_ALPHA,
			wxString::Format("%f", currentAlpha) );
		::setColor( mAlphaCtrl );
		mFgsButton->Add( mAlphaCtrl, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mSt3 = new wxStaticText( cp, wxID_ANY, "beta:" );
		::setColor( mSt3 );
		mFgsButton->Add( mSt3, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mBetaCtrl = new wxTextCtrl( cp, Owen2dFrame::ID_BETA,
			wxString::Format("%f", currentBeta) );
		::setColor( mBetaCtrl );
		mFgsButton->Add( mBetaCtrl, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mSt4 = new wxStaticText( cp, wxID_ANY, "gamma:" );
		::setColor( mSt4 );
		mFgsButton->Add( mSt4, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		mGammaCtrl = new wxTextCtrl( cp, Owen2dFrame::ID_GAMMA,
			wxString::Format("%f", currentGamma) );
		::setColor( mGammaCtrl );
		mFgsButton->Add( mGammaCtrl, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		display_fgs();
	}

	Owen2dAuxControls( wxPanel* cp, wxSizer* bottomSizer,
		const char * const title, int featureRange,
		int currentFeature, int currentFeatureStatus, int currentTransform,
		double currentWeight, double currentMean, double currentStdDev,
		double currentFeatureMin, double currentFeatureMax );

	~Owen2dAuxControls()
	{
		if (mStdDev)
			delete mStdDev, mStdDev = NULL;
		if (mMean)
			delete mMean, mMean = NULL;
		if (mFeatureMax)
			delete mFeatureMax, mFeatureMax = NULL;
		if (mFeatureMin)
			delete mFeatureMin, mFeatureMin = NULL;
		if (mWeight)
			delete mWeight, mWeight = NULL;
		if (mFgsSliders)
			mFgs->Remove(mFgsSliders), mFgsSliders = NULL;
		if (mUpdate)
		{
			mFgsButton->Detach(mUpdate);
			delete mUpdate;
			mUpdate = NULL;
		}
		if (mTransformSelect)
		{
			mFgsButton->Detach(mTransformSelect);
			delete mTransformSelect;
			mTransformSelect = NULL;
		}
		if (mTransform)
		{
			mFgsButton->Detach(mTransform);
			delete mTransform;
			mTransform = NULL;
		}
		if (mGammaCtrl)
		{
			mFgsButton->Detach(mGammaCtrl);
			delete mGammaCtrl;
			mGammaCtrl = NULL;
		}
		if (mSt4)
		{
			mFgsButton->Detach(mSt4);
			delete mSt4;
			mSt4 = NULL;
		}
		if (mBetaCtrl)
		{
			mFgsButton->Detach(mBetaCtrl);
			delete mBetaCtrl;
			mBetaCtrl = NULL;
		}
		if (mSt3)
		{
			mFgsButton->Detach(mSt3);
			delete mSt3;
			mSt3 = NULL;
		}
		if (mAlphaCtrl)
		{
			mFgsButton->Detach(mAlphaCtrl);
			delete mAlphaCtrl;
			mAlphaCtrl = NULL;
		}
		if (mMinPointsCtrl)
		{
			mFgsButton->Detach(mMinPointsCtrl);
			delete mMinPointsCtrl;
			mMinPointsCtrl = NULL;
		}
		if (mSt2)
		{
			mFgsButton->Detach(mSt2);
			delete mSt2;
			mSt2 = NULL;
		}
		if (mFeatureDisplay)
		{
			mFgsButton->Detach(mFeatureDisplay);
			delete mFeatureDisplay;
			mFeatureDisplay = NULL;
		}
		if (mFeatureStatus)
		{
			mFgsButton->Detach(mFeatureStatus);
			delete mFeatureStatus;
			mFeatureStatus = NULL;
		}
		if (mFeature)
		{
			mFgsButton->Detach(mFeature);
			delete mFeature;
			mFeature = NULL;
		}
		if (mBrushSize)
		{
			mFgsButton->Detach(mBrushSize);
			delete mBrushSize;
			mBrushSize = NULL;
		}
		if (mIterates)
		{
			mFgsButton->Detach(mIterates);
			delete mIterates;
			mIterates = NULL;
		}
		if (mSt1)
		{
			mFgsButton->Detach(mSt1);
			delete mSt1;
			mSt1 = NULL;
		}
		mFgs->Remove(mFgsButton);
		mAuxSizer->Remove(mFgs);
		mBottomSizer->Remove(mAuxSizer);
	}
	void SetFeatureStatus(bool newFeatureStatus)
	{
		mFeatureStatus->SetValue(newFeatureStatus);
	}
	void SetTransform(int newTransform, int featureRange,
		double newFeatureMean, double newFeatureStdDev,
		double newFeatureMin, double newFeatureMax)
	{
		if (mStdDev)
			delete mStdDev, mStdDev = NULL;
		if (mMean)
			delete mMean, mMean = NULL;
		if (mFeatureMax)
			delete mFeatureMax, mFeatureMax = NULL;
		if (mFeatureMin)
			delete mFeatureMin, mFeatureMin = NULL;
		switch (newTransform) {
			case 0: // Linear
			case 2: // Inv. Linear
				mFeatureMin = new Owen2dSlider( mCp, mFgsSliders, "Feature Min.",
					Owen2dFrame::ID_FEATURE_MIN, 0, featureRange,
					newFeatureMin );
				mFeatureMax = new Owen2dSlider( mCp, mFgsSliders, "Feature Max.",
					Owen2dFrame::ID_FEATURE_MAX, 0, featureRange,
					newFeatureMax );
				break;
			case 1: // Gaussian
			case 3: // Inv. Gaussian
			case 4: // Hyperbolic
			case 5: // Inv. Hyperbolic
				mMean = new Owen2dSlider( mCp, mFgsSliders, "Mean",
					Owen2dFrame::ID_MEAN, 0, featureRange, newFeatureMean );
				mStdDev = new Owen2dSlider( mCp, mFgsSliders, "Std. Dev.",
					Owen2dFrame::ID_STD_DEV, 0, featureRange,
					newFeatureStdDev);
		}
		mBottomSizer->Layout();
	}
	void SetFeature(int newFeature, bool newFeatureStatus,
		int newFeatureTransform, int featureRange, double newFeatureWeight,
		double newFeatureMean, double newFeatureStdDev,
		double newFeatureMin, double newFeatureMax)
	{
		SetFeatureStatus(newFeatureStatus);
		if (mStdDev)
			delete mStdDev, mStdDev = NULL;
		if (mMean)
			delete mMean, mMean = NULL;
		if (mFeatureMax)
			delete mFeatureMax, mFeatureMax = NULL;
		if (mFeatureMin)
			delete mFeatureMin, mFeatureMin = NULL;
		if (mWeight)
			delete mWeight, mWeight = NULL;
		mWeight = new Owen2dSlider( mCp, mFgsSliders, "Weight",
			Owen2dFrame::ID_WEIGHT, 1, 100, newFeatureWeight );
		SetTransform(newFeatureTransform, featureRange, newFeatureMean,
			newFeatureStdDev, newFeatureMin, newFeatureMax);
		mTransform->SetValue(Owen2dTransformNames[newFeatureTransform]);
	}
	void UpdateWeight( double newval )     {mWeight->UpdateValue(newval);}
	void UpdateMean( double newval )       {mMean->UpdateValue(newval);}
	void UpdateStdDev( double newval )     {mStdDev->UpdateValue(newval);}
	void UpdateFeatureMin( double newval ) {mFeatureMin->UpdateValue(newval);}
	void UpdateFeatureMax( double newval ) {mFeatureMax->UpdateValue(newval);}
};



class  SetOwen2dOutputControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
	wxFlexGridSizer*  mFgsButton;
    wxStaticBox*      mSetOutputBox;
	wxButton*         m_saveBut;
	wxComboBox *      m_outputObject;
    wxStaticBoxSizer* mSetOutputSizer;
	wxStaticText     *st;
	wxButton         *m_outputType;

public:
	SetOwen2dOutputControls( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, int saveID, int outputObjectID,
		int currentOutObject, int currentOutType=BINARY,
		int outputTypeID=Owen2dFrame::ID_OUT_TYPE );

	void SetOutType( int newOutType );

	// Modified: 7/18/08 mSetOutputSizer detached instead of removed
	//    by Dewey Odhner.
	~SetOwen2dOutputControls() {

		mFgsButton->Detach(m_saveBut);
		delete m_saveBut;
		mFgsButton->Detach(m_outputType);
		delete m_outputType;
		mFgsButton->Detach(m_outputObject);
		delete m_outputObject;
		mFgsButton->Detach(st);
		delete st;

		mFgs->Detach(mFgsButton);
		delete mFgsButton;
		mSetOutputSizer->Detach(mFgs);
		delete mFgs;
		mBottomSizer->Detach(mSetOutputSizer);
		delete mSetOutputBox;
	}
};


#endif
//======================================================================
