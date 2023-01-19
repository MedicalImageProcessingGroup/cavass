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
 * \file   AlgebraFrame.h
 * \brief  AlgebraFrame definition.
 * \author Ying Zhuge, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __AlgebraFrame_h
#define __AlgebraFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "AlgebraCanvas.h"
//#include  "wxFPSlider.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  CineControls;
class  GrayMapControls;
class  SaveScreenControls;
class  SetFilterIndexControls;
class  OffsetControls;
class  CoefficientControls;
class  DivThreControls;

/** \brief AlgebraFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class AlgebraFrame : public MainFrame 
{
  CineControls*        mCineControls;
  GrayMapControls*     mGrayMap1Controls;
  GrayMapControls*     mGrayMap2Controls;
  GrayMapControls*     mGrayMapOutControls;
  SetFilterIndexControls*    mSetIndex1Controls;
  SetFilterIndexControls*    mSetIndex2Controls;  
  OffsetControls*             mOffsetControls;
  CoefficientControls *mCoefficientControls;
  DivThreControls*             mDivThreControls;

public:
	bool IsImgSizeEqual();

    static wxFileHistory  _fileHistory;
    enum {
        ID_PREVIOUS=ID_LAST, ID_NEXT,
        ID_SET_INDEX1,	ID_SET_INDEX2,       
        ID_GRAYMAP1,         ID_GRAYMAP2, ID_GRAYMAPOUT,
        ID_CINE,             ID_RESET,
        ID_CINE_TIMER, ID_CINE_SLIDER, ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD,
        ID_CENTER1_SLIDER, ID_CENTER2_SLIDER, ID_CENTEROUT_SLIDER,
        ID_WIDTH1_SLIDER,  ID_WIDTH2_SLIDER, ID_WIDTHOUT_SLIDER,
        ID_INVERT1,        ID_INVERT2, ID_INVERTOUT,
        ID_SLICE1_SLIDER,  ID_SLICE2_SLIDER,
        ID_SCALE1_SLIDER,  ID_SCALE2_SLIDER,
        ID_CT_LUNG1,        ID_CT_SOFT_TISSUE1, ID_CT_BONE1, ID_PET1,
        ID_CT_LUNG2,        ID_CT_SOFT_TISSUE2, ID_CT_BONE2, ID_PET2,
        ID_ZOOM_IN, ID_ZOOM_OUT,
        ID_OVERLAY, ID_BUSY_TIMER,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_ALGEBRA, ID_ALGEBRA_TYPE, ID_SAVE,ID_ALGEBRA_DIVTHRE,
        ID_ALGEBRA_OFFSET, ID_ALGEBRA_COEFFICIENT
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
  CavassAlgebraType  m_AlgebraType;
  enum           {TypeNumber = 10};
  wxString       algebraName[TypeNumber];

    int            mFileOrDataCount;    ///< two needed for overlay
//    wxSlider*      m_center;            ///< contrast center slider
//    wxSlider*      m_width;             ///< contrast width slider
  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxFlexGridSizer*  fgs;
    wxComboBox    *m_AlgebraTypeBut;

  //wxTextCtrl*    mSigmaText;

    //cine related items
    bool           mForward, mForwardBackward, mDirectionIsForward;  ///< cine direction
    wxTimer*       m_cine_timer;        ///< time for cine

    void initializeMenu ( void );
    void addButtonBox ( void );
    void hideParametersBox(void);	

    void OnCineTimer           ( wxTimerEvent& e    );
    void OnCineForward         ( wxCommandEvent& e  );
    void OnCineForwardBackward ( wxCommandEvent& e  );
    void OnCineSlider          ( wxScrollEvent& e   );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    AlgebraFrame ();
    ~AlgebraFrame ( void );

    int loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );

    void OnCenter1Slider ( wxScrollEvent& e );
    void OnCenter2Slider ( wxScrollEvent& e );
	void OnCenterOutSlider ( wxScrollEvent& e );
    void OnWidth1Slider  ( wxScrollEvent& e );
    void OnWidth2Slider  ( wxScrollEvent& e );
	void OnWidthOutSlider  ( wxScrollEvent& e );
    void OnSlice1Slider  ( wxScrollEvent& e );
    void OnSlice2Slider  ( wxScrollEvent& e );
    void OnScale1Slider  ( wxScrollEvent& e );	
    void OnScale2Slider  ( wxScrollEvent& e );


    void OnBusyTimer     ( wxTimerEvent& e  );
#ifdef __WXX11__
    void OnUpdateUICenter1Slider ( wxUpdateUIEvent& e );
    void OnUpdateUIWidth1Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUISlice1Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIScale1Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIRed1Slider    ( wxUpdateUIEvent& e );
    void OnUpdateUIGreen1Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIBlue1Slider   ( wxUpdateUIEvent& e );

    void OnUpdateUICenter2Slider ( wxUpdateUIEvent& e );
    void OnUpdateUIWidth2Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUISlice2Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIScale2Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIRed2Slider    ( wxUpdateUIEvent& e );
    void OnUpdateUIGreen2Slider  ( wxUpdateUIEvent& e );
    void OnUpdateUIBlue2Slider   ( wxUpdateUIEvent& e );

    void OnUpdateUICineSlider    ( wxUpdateUIEvent& e );
#endif
    void OnPrevious     ( wxCommandEvent& unused );
    void OnNext         ( wxCommandEvent& unused );
    void OnSetIndex1    ( wxCommandEvent& unused );
	void OnSetIndex2    ( wxCommandEvent& unused );
    void OnGrayMap1     ( wxCommandEvent& unused );
    void OnGrayMap2     ( wxCommandEvent& unused );
	void OnCTLung1      ( wxCommandEvent& unused );
	void OnCTSoftTissue1( wxCommandEvent& unused );
	void OnCTBone1      ( wxCommandEvent& unused );
    void OnPET1         ( wxCommandEvent& unused );
	void OnCTLung2      ( wxCommandEvent& unused );
	void OnCTSoftTissue2( wxCommandEvent& unused );
	void OnCTBone2      ( wxCommandEvent& unused );
    void OnPET2         ( wxCommandEvent& unused );
	void OnGrayMapOut   ( wxCommandEvent& unused );
    void OnCine         ( wxCommandEvent& unused );
    void OnReset        ( wxCommandEvent& unused );
    
    void OnRunAlgebra                 ( wxCommandEvent& e );
    void OnAlgebraType             (wxCommandEvent& e );
    void OnAlgebraSave             (wxCommandEvent& e );
	void OnAlgebraOffset           (wxCommandEvent& e);
	void OnAlgebraCoefficient      (wxCommandEvent& e);
	void OnAlgebraDivThre          (wxCommandEvent& e);

    void OnSaveScreen   ( wxCommandEvent& e );
    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );
    void OnZoomIn       ( wxCommandEvent& e );
    void OnZoomOut      ( wxCommandEvent& e );

    void OnInvert1      ( wxCommandEvent& e );
    void OnInvert2      ( wxCommandEvent& e );
	void OnInvertOut      ( wxCommandEvent& e );
    void OnOverlay      ( wxCommandEvent& e );

    void OnHideControls ( wxCommandEvent& e );
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( AlgebraFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
