/*
  Copyright 1993-2015 Medical Image Processing Group
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
 * \file   ThresholdFrame.h
 * \brief  ThresholdFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ThresholdFrame_h
#define __ThresholdFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "ThresholdCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

#define MAX_TRACK_POINTS 50

#define OUTPUT_TYPE_BS1 0
#define OUTPUT_TYPE_BS0 1
#define OUTPUT_TYPE_BS2 2
#define OUTPUT_TYPE_BIM 3
#define OUTPUT_TYPE_TXT 4
#define OUTPUT_TYPE_SH0 5
#define OUTPUT_TYPE_IM0 6



class  CineControls;
class  GrayMapControls;
class  SetThresholdIndexControls;
class  SetThresholdParameterControls;
class  SetThresholdOutputControls;
class  HistZoomControls;
class  SearchStepControls;
class  ThrOpacityControls;
class  SurfaceStrengthControls;


/** \brief ThresholdFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class ThresholdFrame : public MainFrame {
    CineControls*        mCineControls;
    GrayMapControls*     mGrayMap1Controls;
    SetThresholdIndexControls*    mSetIndex1Controls;
	SetThresholdParameterControls* mSetParameterControls;
	SetThresholdOutputControls* mSetOutputControls;
	HistZoomControls* mHistZoomControls;
	SearchStepControls* mSearchStepControls;
	ThrOpacityControls* mThrOpacityControls;
	SurfaceStrengthControls* mSurfaceStrengthControls;

  public:
    static wxFileHistory  _fileHistory;
    enum {
        ID_PREVIOUS=ID_LAST, ID_NEXT,
        ID_SET_INDEX1,
        ID_GRAYMAP1,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        ID_CINE,             ID_RESET,
        ID_CINE_TIMER, ID_CINE_SLIDER, ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD,
        ID_CENTER1_SLIDER,
        ID_WIDTH1_SLIDER,
        ID_INVERT1,
        ID_SLICE1_SLIDER,
        ID_SCALE1_SLIDER,
        ID_ZOOM_IN, ID_ZOOM_OUT,
        ID_OVERLAY, ID_BUSY_TIMER,
//        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_INTERVAL, ID_OUT_TYPE, ID_SAVE,
		ID_SEGMENT,
		ID_HIST_ZOOM, ID_HIST_ZOOM_SLIDER, ID_HIST_SCOPE,
		ID_BLINK, ID_OPT_STEP, ID_OPT_STEP_SLIDER, ID_FIND_OPT,
		ID_OUT_INTERVAL, ID_BOUNDARY_TYPE, ID_RESOLUTION, ID_NORMAL_TYPE,
		ID_PARAMS, ID_SET_PARAMS,
		ID_PARAM1_CTRL, ID_PARAM2_CTRL, ID_PARAM3_CTRL,
		ID_OUT_MODE, ID_RESET_POINTS,
		ID_NUM_PARAMS, ID_WHICH_PARAM, ID_PARAM_TYPE,
		ID_SET_OUTPUT, ID_VOLUME_NUMBER,
		ID_SET_OPACITY, ID_OPACITY_SLIDER,
		ID_SET_SURFACE_STRENGTH, ID_SURFACE_STRENGTH_SLIDER,
		ID_THRSLD_TABLET1, ID_THRSLD_TABLET2,
		ID_THRSLD_TABLET3, ID_THRSLD_TABLET4
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
	enum ButtonSet_t { PARAM_BUTTONS, OUT_BUTTONS };

    int            mFileOrDataCount;    ///< two needed for overlay

	ThresholdCanvas *tCanvas;
	wxString outputTypeName[7];
	wxString histScopeName[3];
	wxString intervalName[MAX_INTERVAL+2];

  protected:
    wxFlexGridSizer*  fgs;

	wxButton*      m_prev;
	wxButton*      m_next;
	wxButton*      m_setIndex;
	wxButton*      m_grayMap;
	wxButton*      m_cine;
	wxCheckBox*    m_blink;
	wxButton*      m_histZoomBut;
	wxButton*      m_histScopeBut;
	wxComboBox    *m_intervalBut;
	wxButton*      m_reset;
	wxButton*      m_searchStepBut;
	wxButton*      m_findOptBut;
	wxComboBox    *m_outputTypeBut;
	wxButton*      m_setOutputBut;
	wxComboBox    *m_boundaryTypeBut;
	wxCheckBox*    m_paramsBut;
	wxButton*      m_setParams;
	wxButton*      m_resetPointsBut;
	wxButton*      m_opacityBut;
	wxButton*      m_surfaceStrengthBut;

	wxSizer*       m_buttonSizer;
	wxFlexGridSizer* m_fgs;


    //cine related items
    bool           mForward, mForwardBackward, mDirectionIsForward;  ///< cine direction
    wxTimer*       m_cine_timer;        ///< time for cine

	int OUTPUT_DATA_TYPE;
	int BOUNDARY_TYPE;
	int MODE_TYPE; /* 0- New 1- Merge */
	int NORMAL_TYPE;
	int out_interval;
	int m_bg_flag;
	double resolution;
	char normal_dup[3][30];
	int points_file_tag;
	int params_on, params_set;
	int num_params, cur_param;
	short param_type[3];
	float *params[3];
	int num_volumes, cur_volume;
	bool fuzzy_flag;
	int nbits; /* normal bits per component */
	int grad_factor; /* surface strength */
	int first_sl, last_sl;
	float min_opac, max_opac;
	float opacity_factor[MAX_INTERVAL+2]; /* The opacity value for each fn. */
	bool programmatic_update;

    void initializeMenu ( void );
    void addButtonBox ( void );
	void emptyButtonBox ( void );
	int SaveBinaryFile(const char *name, int range, int bg_flag);
	int SaveBoundaryFile(const char *name, const char *ext, int range, int bg_flag);
	void SaveBoundaryFile_1(const char *name, const char *ext, int range, int bg_flag);
	int SaveTextFile(const char *name, const char *ext, int range, int bg_flag);
	void deleteThresholdControls(void);

    void OnCineTimer           ( wxTimerEvent& e    );
    void OnCineForward         ( wxCommandEvent& e  );
    void OnCineForwardBackward ( wxCommandEvent& e  );
    void OnCineSlider          ( wxScrollEvent& e   );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createThresholdFrame ( wxFrame* parentFrame,
        bool useHistory=true, bool fuzzy=false );
    ThresholdFrame ( bool maximize=false, int w=800, int h=600,
        bool fuzzy=false );
    ~ThresholdFrame ( void );
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };

    void UpdateThresholdTablets();
    void loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );

    void OnCenter1Slider ( wxScrollEvent& e );
    void OnWidth1Slider  ( wxScrollEvent& e );
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
    void OnSlice1Slider  ( wxScrollEvent& e );
    void OnScale1Slider  ( wxScrollEvent& e );
	void OnResolutionSlider ( wxScrollEvent& e );
	void OnHistZoomSlider ( wxScrollEvent& e );
	void OnSearchStepSlider ( wxScrollEvent& e );
	void OnOpacitySlider ( wxScrollEvent& e );
	void OnSurfaceStrengthSlider ( wxScrollEvent& e );

    void OnBusyTimer     ( wxTimerEvent& e  );
#ifdef __WXX11__
    void OnUpdateUICenter1Slider ( wxUpdateUIEvent& e );
    void OnUpdateUIWidth1Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUISlice1Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIScale1Slider  ( wxUpdateUIEvent& e );

    void OnUpdateUICineSlider    ( wxUpdateUIEvent& e );
#endif
    void OnPrevious     ( wxCommandEvent& unused );
    void OnNext         ( wxCommandEvent& unused );
    void OnSetIndex1    ( wxCommandEvent& unused );
    void OnGrayMap1     ( wxCommandEvent& unused );
    void OnCine         ( wxCommandEvent& unused );
	void OnBlink        ( wxCommandEvent& e );
	void OnHistZoom     ( wxCommandEvent& unused );
	void OnHistScope    ( wxCommandEvent& unused );
	void OnInterval     ( wxCommandEvent& e );
    void OnReset        ( wxCommandEvent& unused );
	void OnSearchStep   ( wxCommandEvent& unused );
	void OnFindOpt      ( wxCommandEvent& unused );
	void OnOutputType   ( wxCommandEvent& e );
	void OnBoundaryType ( wxCommandEvent& e );
	void OnNormalType   ( wxCommandEvent& unused );
	void OnParams       ( wxCommandEvent& unused );
	void OnSetParams    ( wxCommandEvent& e );
	void OnOutputMode   ( wxCommandEvent& unused );
	void OnResetPoints  ( wxCommandEvent& unused );
	void OnSetOutput    ( wxCommandEvent& unused );
	void OnOutputInterval(wxCommandEvent& unused );
    void OnThresholdSave   ( wxCommandEvent& e );
	void OnNumParams    ( wxCommandEvent& unused );
	void OnWhichParam   ( wxCommandEvent& unused );
	void OnParamType    ( wxCommandEvent& unused );
	void OnVolumeNumber ( wxCommandEvent& unused );
	void OnSetParam1    ( wxCommandEvent& e );
	void OnSetParam2    ( wxCommandEvent& e );
	void OnSetParam3    ( wxCommandEvent& e );
	void OnSetOpacity   ( wxCommandEvent& unused );
	void OnSetSurfaceStrength(wxCommandEvent& unused );
	void OnSetThreshold1( wxCommandEvent& e );
	void OnSetThreshold2( wxCommandEvent& e );
	void OnSetThreshold3( wxCommandEvent& e );
	void OnSetThreshold4( wxCommandEvent& e );

    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );
	virtual void OnInput           ( wxCommandEvent& unused );

    void OnInvert1      ( wxCommandEvent& e );
    void OnOverlay      ( wxCommandEvent& e );

    void OnHideControls ( wxCommandEvent& e );
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ThresholdFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
