/*
  Copyright 1993-2013, 2015-2016 Medical Image Processing Group
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
 * \file   OverlayFrame.h
 * \brief  OverlayFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __OverlayFrame_h
#define __OverlayFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "OverlayCanvas.h"
//#include  "wxFPSlider.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  CineControls;
class  ColorControls;
class  GrayMapControls;
class  SetIndexControls;

/** \brief OverlayFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class OverlayFrame : public MainFrame {
    CineControls*        mCineControls;
    ColorControls*       mColor1Controls;
    ColorControls*       mColor2Controls;
    GrayMapControls*     mGrayMap1Controls;
    GrayMapControls*     mGrayMap2Controls;
    SetIndexControls*    mSetIndex1Controls;
    SetIndexControls*    mSetIndex2Controls;
  public:
    enum {
        ID_PREVIOUS=ID_LAST, ID_NEXT,
        ID_SET_INDEX1,       ID_SET_INDEX2,
        ID_MATCH_INDEX1,     ID_MATCH_INDEX2,
        ID_GRAYMAP1,         ID_GRAYMAP2,
        ID_COLOR1,           ID_COLOR2,
        ID_BLINK1,           ID_BLINK2,
        ID_CINE,             ID_RESET,
        ID_CINE_TIMER, ID_CINE_SLIDER, ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD,
        ID_RED1_SLIDER,    ID_RED2_SLIDER,
        ID_GREEN1_SLIDER,  ID_GREEN2_SLIDER,
        ID_BLUE1_SLIDER,   ID_BLUE2_SLIDER,
        ID_CENTER1_SLIDER, ID_CENTER2_SLIDER,
        ID_WIDTH1_SLIDER,  ID_WIDTH2_SLIDER,
        ID_CT_LUNG1,       ID_CT_SOFT_TISSUE1, ID_CT_BONE1, ID_PET1,
        ID_CT_LUNG2,       ID_CT_SOFT_TISSUE2, ID_CT_BONE2, ID_PET2,
        ID_INVERT1,        ID_INVERT2,
        ID_SLICE1_SLIDER,  ID_SLICE2_SLIDER,
        ID_SCALE1_SLIDER,  ID_SCALE2_SLIDER,
        ID_ZOOM_IN, ID_ZOOM_OUT,
        ID_OVERLAY, ID_BUSY_TIMER
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
    int            mFileOrDataCount;    ///< two needed for overlay
//    wxSlider*      m_center;            ///< contrast center slider
//    wxSlider*      m_width;             ///< contrast width slider
  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxStaticBox*   mSetIndex2Box;
    wxSizer*       mSetIndex2Sizer;
    //cine related items
    bool           mForward, mForwardBackward, mDirectionIsForward;  ///< cine direction
    wxTimer*       m_cine_timer;        ///< time for cine

    void initializeMenu ( void );
    void addButtonBox ( void );

    void OnCineTimer           ( wxTimerEvent& e    );
    void OnCineForward         ( wxCommandEvent& e  );
    void OnCineForwardBackward ( wxCommandEvent& e  );
    void OnCineSlider          ( wxScrollEvent& e   );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    OverlayFrame ( );
    ~OverlayFrame ( void );

    void loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );

    void OnCenter1Slider ( wxScrollEvent& e );
    void OnCenter2Slider ( wxScrollEvent& e );
    void OnWidth1Slider  ( wxScrollEvent& e );
    void OnWidth2Slider  ( wxScrollEvent& e );
	void OnCTLung1       ( wxCommandEvent& unused );
	void OnCTSoftTissue1 ( wxCommandEvent& unused );
	void OnCTBone1       ( wxCommandEvent& unused );
    void OnPET1          ( wxCommandEvent& unused );
	void OnCTLung2       ( wxCommandEvent& unused );
	void OnCTSoftTissue2 ( wxCommandEvent& unused );
	void OnCTBone2       ( wxCommandEvent& unused );
    void OnPET2          ( wxCommandEvent& unused );
    void OnSlice1Slider  ( wxScrollEvent& e );
    void OnSlice2Slider  ( wxScrollEvent& e );
    void OnScale1Slider  ( wxScrollEvent& e );
    void OnScale2Slider  ( wxScrollEvent& e );

    void OnRed1Slider    ( wxScrollEvent& e );
    void OnRed2Slider    ( wxScrollEvent& e );
    void OnGreen1Slider  ( wxScrollEvent& e );
    void OnGreen2Slider  ( wxScrollEvent& e );
    void OnBlue1Slider   ( wxScrollEvent& e );
    void OnBlue2Slider   ( wxScrollEvent& e );

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
	virtual void OnMouseWheel      ( wxMouseEvent& e );
    void OnNext         ( wxCommandEvent& unused );
    void OnSetIndex1    ( wxCommandEvent& unused );
    void OnSetIndex2    ( wxCommandEvent& unused );
    void OnGrayMap1     ( wxCommandEvent& unused );
    void OnGrayMap2     ( wxCommandEvent& unused );
    void OnColor1       ( wxCommandEvent& unused );
    void OnColor2       ( wxCommandEvent& unused );
    void OnMatchIndex1  ( wxCommandEvent& unused );
    void OnMatchIndex2  ( wxCommandEvent& unused );
    void OnBlink1       ( wxCommandEvent& unused );
    void OnBlink2       ( wxCommandEvent& unused );
    void OnCine         ( wxCommandEvent& unused );
    void OnReset        ( wxCommandEvent& unused );
    void OnOpen         ( wxCommandEvent& e );
    void OnZoomIn       ( wxCommandEvent& e );
    void OnZoomOut      ( wxCommandEvent& e );
    void OnOverlay      ( wxCommandEvent& e );
    void OnInvert1      ( wxCommandEvent& e );
    void OnInvert2      ( wxCommandEvent& e );
    void OnHideControls ( wxCommandEvent& e );
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( OverlayFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
