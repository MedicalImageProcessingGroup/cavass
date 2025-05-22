/*
  Copyright 1993-2015, 2017-2018 Medical Image Processing Group
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
 * \file   ExportFrame.h
 * \brief  ExportFrame definition.
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
#ifndef __ExportFrame_h
#define __ExportFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "ExportCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

enum {
	EXPORT_DICOM, EXPORT_MATHEMATICA, EXPORT_MATLAB, EXPORT_PGM, EXPORT_R,
    EXPORT_STL, EXPORT_VTK
};

#define OUTPUT_TYPE_PNM 0
#define OUTPUT_TYPE_TIFF 1

class  GrayMapControls;
class  SetIndexControls;
class  SetExportRangeControls;
class  SetUIDControls;

/** \brief ExportFrame class definition.
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
    ExportFrame.cpp,  ExportFrame.h, ExportCanvas.cpp, and ExportCanvas.h.
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
  \todo These other modules need to be updated to be in accord with the Export app.
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
  bar.  In this example, we'll add the Export app to the Help menu.
 </p>
 <ol>
  <li> Add ID_EXAMPLE to the enum in MainFrame.h (so it can be added to the
       menu bar).
       <pre>
       </pre> </li>
  <li> Add the following callback definition to MainFrame.h (this is the
       callback when Export is chosen from the menu bar):
       <pre>
           void OnExport ( wxCommandEvent& e );
       </pre> </li>
  <li> Add the following to the DefineStandardFrameCallbacks macro in
       MainFrame.h (to associate the ID with the callback function):
       <pre>
           EVT_MENU( ID_EXAMPLE,              MainFrame::OnExport      )    \
       </pre> </li>
  <li> Add the following to MainFrame.cpp (this is the callback function that
       will be called when Export is chosen from the menu bar):
       <pre>
           void MainFrame::OnExport ( wxCommandEvent& unused ) {
               ExportFrame::createExportFrame( this );
           }
       </pre>
       Using ExportFrame::createExportFrame() in this manner allows one 
       to customize file selection code.  For example, overlay requires
       two files so the OnOverlay method asks the user to select two files.
       Or you may wish to change the various file types that can be used as
       input to your particular app.
       <pre>
       </pre>
  </li>
  <li> Add the following to MainFrame.cpp:
       <pre>
           #include  "ExportFrame.h"
       </pre> </li>
  <li> Add the following to MainFrame::initializeMenu() in MainFrame.cpp:
       <pre>
           mHelpMenu->Append( ID_EXAMPLE, "&Export" );
       </pre> </li>
 </ol>
 <h3> A note about callbacks... </h3>
 <p>
  The macro, DefineStandardFrameCallbacks, is defined in MainFrame.h.  It is
  used to define standard callback table entries (so they need not be
  repeated in each app).  Callback functions are defined as being virtual
  so they can be completely replaced via inheritance (when necessary).
 </p>
 */
class ExportFrame : public MainFrame {
  protected:
    GrayMapControls*  mGrayMapControls;     ///< gray map controls
    SetExportRangeControls*  mSetRangeControls;
	SetIndexControls*    mSetIndexControls;
	SetUIDControls  *mSetUIDControls;
  public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_PREVIOUS=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_NEXT,
        ID_GRAYMAP, ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        ID_SET_INDEX, ID_SET_RANGE,
        ID_SLICE_SLIDER, ID_SLICE1_SLIDER, ID_SLICE2_SLIDER,
		ID_SCALE_SLIDER, ID_OVERLAY,
        ID_STRUCT_NUMBER,  ID_SERIES_UID, ID_SERIES_UID_TABLET,
        ID_OUT_TYPE, ID_SAVE
    };
    int  mFileOrDataCount;  ///< count data/datafiles used (1 for this example).

    ExportCanvas *tCanvas;
    wxString mInputBS2;
    wxString outputTypeName[2];
    int output_data_type;
	wxString mSeriesUID;

  protected:
    int m_frame_type;
    int first_sl, last_sl;
    int structure_number, number_of_structures;
    wxFlexGridSizer* m_fgs;
    wxButton*      m_prev;
    wxButton*      m_next;
    wxButton*      m_setIndex;
    wxButton*      m_grayMap;
	wxButton*      m_setRange;
    wxButton *m_structN;
	wxButton *m_suid;
    wxButton *m_save;
    wxButton *m_outputTypeBut;

    void initializeMenu ( void );
    void addButtonBox ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createExportFrame ( wxFrame* parentFrame, bool useHistory=true,
        int frame_type=EXPORT_DICOM );
    ExportFrame ( bool maximize=false, int w=800, int h=600,
        int frame_type=EXPORT_DICOM );
    ~ExportFrame ( void );
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };

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

    virtual void OnCenterSlider ( wxScrollEvent& e );
    virtual void OnWidthSlider  ( wxScrollEvent& e );
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );

    void OnExportSave   ( wxCommandEvent& unused );
    void OnOutputType   ( wxCommandEvent& e );
    void OnSetIndex     ( wxCommandEvent& unused );
	void OnSetRange     ( wxCommandEvent& unused );
	void OnSeriesUID    ( wxCommandEvent& unused );
	void OnSeriesUIDTablet( wxCommandEvent& e );
	virtual void OnSliceSlider  ( wxScrollEvent& e );
    void OnSlice1Slider  ( wxScrollEvent& e );
    void OnSlice2Slider  ( wxScrollEvent& e );
	virtual void OnScaleSlider  ( wxScrollEvent& e );
	void OnOverlay      ( wxCommandEvent& e );
    void OnStructNumber ( wxCommandEvent& unused );
#ifdef __WXX11__
    virtual void OnUpdateUICenterSlider ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIWidthSlider  ( wxUpdateUIEvent& unused );
#endif
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ExportFrame )
    DECLARE_EVENT_TABLE()
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ExportStlFrame : public ExportFrame {
  public:
    ExportStlFrame ( bool maximize=false, int w=800, int h=600 );
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };
    virtual void OnInput           ( wxCommandEvent& unused );
    virtual void OnOpen            ( wxCommandEvent& unused );
    virtual void OnInformation     ( wxCommandEvent& unused );
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class SetExportRangeControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSetIndexBox;
    wxStaticBoxSizer* mSetIndexSizer;
    wxSlider*         mSliceNumber1;  ///< first slice number slider
    wxStaticText*     mSliceST1;      ///< first slice number slider label
    wxSlider*         mSliceNumber2;  ///< last slice number slider
    wxStaticText*     mSliceST2;      ///< last slice number slider label
public:
    /** \brief SetExportRangeControls ctor. */
    SetExportRangeControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, int currentSlice1, int currentSlice2,
        int numberOfSlices, int sliceSliderID1, int sliceSliderID2 )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //first slice number
        mSliceST1 = new wxStaticText( cp, -1, "First slice:" );
        ::setColor( mSliceST1 );
        mFgs->Add( mSliceST1, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mSliceNumber1 = new wxSlider( cp, sliceSliderID1, currentSlice1, 1,
            numberOfSlices, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
            "slice" );
        ::setColor( mSliceNumber1 );
        mSliceNumber1->SetPageSize( 5 );
        mFgs->Add( mSliceNumber1, 0, wxGROW|wxLEFT|wxRIGHT );

        //last slice number
        mSliceST2 = new wxStaticText( cp, -1, "Last slice:" );
        ::setColor( mSliceST2 );
        mFgs->Add( mSliceST2, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mSliceNumber2 = new wxSlider( cp, sliceSliderID2, currentSlice2, 1,
            numberOfSlices, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
            "slice" );
        ::setColor( mSliceNumber2 );
        mSliceNumber2->SetPageSize( 5 );
        mFgs->Add( mSliceNumber2, 0, wxGROW|wxLEFT|wxRIGHT );

        mSetIndexSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mSliceNumber1->SetToolTip( "first slice number" );
                mSliceNumber2->SetToolTip( "last slice number" );
            #endif
        #endif
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SetExportRangeControls dtor. */
    ~SetExportRangeControls ( void ) {
        mBottomSizer->Detach( mSetIndexSizer );
        mFgs->Detach(mSliceNumber1);
        delete mSliceNumber1;
        mFgs->Detach(mSliceST1);
        delete mSliceST1;
        mFgs->Detach(mSliceNumber2);
        delete mSliceNumber2;
        mFgs->Detach(mSliceST2);
        delete mSliceST2;
        mSetIndexSizer->Detach(mFgs);
        delete mFgs;
//@        delete mSetIndexSizer;
        delete mSetIndexBox;
    }
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class SetUIDControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSetUIDBox;
    wxStaticBoxSizer* mSetUIDSizer;
	wxTextCtrl       *mUIDCtrl;
public:
    /** \brief SetUIDControls ctor. */
    SetUIDControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, wxString& curUID, int UID_ID )
    {
        mBottomSizer = bottomSizer;
        mSetUIDBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetUIDBox );
        mSetUIDSizer = new wxStaticBoxSizer( mSetUIDBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

		mUIDCtrl = new wxTextCtrl( cp, UID_ID, curUID );
		::setColor( mUIDCtrl );
		mFgs->Add( mUIDCtrl, 0, wxGROW|wxLEFT|wxRIGHT );

        mSetUIDSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mSetUIDSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SetUIDControls dtor. */
    ~SetUIDControls ( void ) {
        mBottomSizer->Detach( mSetUIDSizer );
        mFgs->Detach(mUIDCtrl);
        delete mUIDCtrl;
        mSetUIDSizer->Detach(mFgs);
        delete mFgs;
        delete mSetUIDBox;
    }
};

#endif
//======================================================================
