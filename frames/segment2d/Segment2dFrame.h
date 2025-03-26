/*
  Copyright 1993-2018, 2025 Medical Image Processing Group
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
 * \file   Segment2dFrame.h
 * \brief  Segment2dFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This code provides the user with a variety of segmentation tools/modes: <pre>
 *   Train
 *   Paint
 *   ILW
 *   LiveSnake
 *   SelFeatures
 *   Review
 *   Report
 *   Peek
 * </pre>
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#pragma once

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  <wx/persist.h>
#include  "wx/splitter.h"
#include  "Segment2dCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include <cstdlib>  // #include  <stdlib.h>
#include "CComboBox.h"

class  GrayMapControls;
class  SetIndexControls;
class  SetSegment2dOutputControls;
class  Segment2dAuxControls;
class  Segment2dIntDLControls;

/** \brief Segment2dFrame class definition.
 *
 <p>
  This app/module consists of the following files:
  Segment2dFrame.cpp,  Segment2dFrame.h, Segment2dCanvas.cpp,
  Segment2dCanvas.h, SetSegment2dOutputControls.cpp, and
  SetSegment2dOutputControls.h.
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
  Once the frame, canvas, and controls have been coded for an app,
  one must add the new app to the main, standard CAVASS application menu
  bar.  In this example, we'll add the Segment2d app to the Help menu.
 </p>
 <ol>
  <li> Add ID_PP_SCOPS_SEGMENT_2DINTERACTIVE to the enum in MainFrame.h (so it can be added to the
       menu bar). </li>
  <li> Add the following callback definition to MainFrame.h (this is the
       callback when Segment2d is chosen from the menu bar):
       <pre>
           void OnSegment2d ( wxCommandEvent& e );
       </pre> </li>
  <li> Add the following to the DefineStandardFrameCallbacks macro in
       MainFrame.h (to associate the ID with the callback function):
       <pre>
           EVT_MENU( ID_PP_SCOPS_SEGMENT_2DINTERACTIVE, MainFrame::OnSegment2d )  \
       </pre> </li>
  <li> Add the following to MainFrame.cpp (this is the callback function that
       will be called when Segment2d is chosen from the menu bar):
       <pre>
           void MainFrame::OnSegment2d ( wxCommandEvent& unused ) {
               Segment2dFrame::createSegment2dFrame( this );
           }
       </pre> </li>
  <li> Add the following to MainFrame.cpp:
       <pre>
           #include  "Segment2dFrame.h"
       </pre> </li>
  <li> Add the following to MainFrame::initializeMenu() in MainFrame.cpp:
       <pre>
           help_menu->Append( ID_PP_SCOPS_SEGMENT_2DINTERACTIVE, "&Segment2d" );
       </pre> </li>
 </ol>
 */
class Segment2dFrame : public MainFrame {
    friend class PersistentSegment2dFrame;

    void doAlpha       ( const wxString& e_GetString );
    void doBeta        ( const wxString& e_GetString );
    void doBrushSize   ( const char* c_str );
    void doDefaultFill ( bool isChecked );
    void doGamma       ( const wxString& e_GetString );
    void doIterates    ( const wxString& e_GetString );
    void doLabels      ( bool isChecked );
    void doLayout      ( bool isChecked );
    void doMinPoints   ( const wxString& e_GetString );
    void doMode        ( const wxString& e_GetString );
    void doObject      ( const wxString& e_GetString );
    void doOverlay     ( bool isChecked );  //used by Set Index
    void doOutputType  ( const wxString& type );  //used by Set Output

    void doFeature       ( const wxString& e_GetString );
    void doTransform     ( const wxString& e_GetString );
    void doFeatureStatus ( bool e_IsChecked );
    void doWeight        ( const wxString& value );
    void doMean          ( const wxString& value );
    void doStdDev        ( const wxString& value );
    void doFeatureMin    ( const wxString& value );
    void doFeatureMax    ( const wxString& value );

  protected:
    GrayMapControls*            mGrayMapControls;     ///< gray map controls
    SetIndexControls*           mSetIndexControls;    ///< set index controls
	SetSegment2dOutputControls* mSetOutputControls;   ///< set output controls
	Segment2dAuxControls*       mAuxControls;         ///< aux controls
public:
	Segment2dIntDLControls*     mIntDLControls;       ///< interactive deep learning controls
  public:
    string whatAmI ( ) override { return "Segment2dFrame"; }
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_PREVIOUS=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_NEXT,
        ID_SET_INDEX,
        ID_GRAYMAP, ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
        ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
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
		ID_WEIGHT, ID_MEAN, ID_STD_DEV, ID_FEATURE_MIN, ID_FEATURE_MAX,
        /** for interactive deep learning: */
    	ID_INTDL_ADD, ID_INTDL_BLINK, ID_INTDL_CHOOSE, ID_INTDL_CLEAR, ID_INTDL_RUN
    };

    static const wxArrayString featureNames;
    static const wxArrayString transformNames;
    int  mFileOrDataCount;  ///< count data/datafiles used (1 for this example).
	Segment2dCanvas *tCanvas;
	wxString modeName[NUM_SEG2D_MODES+Segment2dCanvas::LWOF];
	wxString objectName[9];

  protected:
	wxSizer*       m_buttonSizer;
    wxFlexGridSizer*  fgs;

	wxButton*      m_prev;
	wxButton*      m_next;
	wxButton*      m_setIndex;
	wxButton*      m_grayMap;
	wxStaticText  *mModeLabel{};
	CComboBox    *mMode;
	//CComboBox*     mMode;
	wxButton*      m_reset;
	wxStaticText  *m_objectLabel{};
	wxComboBox    *m_object;
	wxButton      *m_deleteObject;
	wxButton      *m_loadObject;
	wxCheckBox    *m_defaultFill;
	wxCheckBox    *m_overlay;  //used by Set Index
	wxCheckBox    *m_layout;
	wxButton*      m_setOutputBut;
	int out_object;
	int OUTPUT_DATA_TYPE{};

    void initializeMenu ( ) override;
    void addButtonBox ( );
	void deleteSegment2dControls ( );
    void restoreFrameSettings ( ) override;
    void restoreControlSettings ( );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createSegment2dFrame ( wxFrame* parentFrame, bool useHistory=true );
    explicit Segment2dFrame ( bool maximize=false, int w=800, int h=600 );
    ~Segment2dFrame ( ) override;
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };

	virtual void flush_temp_data ( );
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
    void OnOverlay      ( wxCommandEvent& e );  //used by Set Index
	void OnLabels       ( wxCommandEvent& e );
	void OnLayout       ( wxCommandEvent& ignored );

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
    void OnSegment2dSave( wxCommandEvent& e );
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

	void OnWeight       ( wxScrollEvent& unused );
	void OnMean         ( wxScrollEvent& unused );
	void OnStdDev       ( wxScrollEvent& unused );
	void OnFeatureMin   ( wxScrollEvent& unused );
	void OnFeatureMax   ( wxScrollEvent& unused );

    void OnMaximize     ( wxMaximizeEvent& unused ) override;

    static void OnHelp ( wxCommandEvent& e );

	/** for interactive deep learning: */
    void OnIntDLAdd    ( wxCommandEvent& unused );
    void OnIntDLBlink  ( wxCommandEvent& unused );
    void OnIntDLChoose ( wxCommandEvent& unused );
    void OnIntDLClear  ( wxCommandEvent& unused );
    void OnIntDLRun    ( wxCommandEvent& unused );
	int get_object_selection_number ( );
	void refresh_object_selection ( );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( Segment2dFrame )
    DECLARE_EVENT_TABLE()
};
//======================================================================
