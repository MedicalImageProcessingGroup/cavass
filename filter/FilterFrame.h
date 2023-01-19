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
 * \file   FilterFrame.h
 * \brief  FilterFrame definition.
 * \author Ying Zhuge, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FilterFrame_h
#define __FilterFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "FilterCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  CineControls;
class  GrayMapControls;
class  SaveScreenControls;
class  SetFilterIndexControls;
class  SigmaControls;
class  ScaleADControls;
class  HomogenControls;
class  MorphControls;
class  Inhomo1Controls;
class  Inhomo2Controls;
class  Inhomo3Controls;
class  LTDT3DControls;
class  SBAv3DControls;
class  BallEnhanceControls;

/** \brief FilterFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class FilterFrame : public MainFrame 
{
  CineControls*             mCineControls;
  GrayMapControls*          mGrayMap1Controls;
  GrayMapControls*          mGrayMap2Controls;
  SetFilterIndexControls*   mSetIndex1Controls;
  SigmaControls*            mSigmaControls;
  ScaleADControls*          mScaleADControls;
  HomogenControls*          mHomogenControls;
  MorphControls*            mMorphControls;
  Inhomo1Controls*          mInhomo1Controls;
  Inhomo2Controls*          mInhomo2Controls;
  Inhomo3Controls*          mInhomo3Controls;
  LTDT3DControls*           mLTDT3DControls;
  SBAv3DControls*           mSBAv3DControls;
  BallEnhanceControls      *mBallEnhanceControls;

public:
    static wxFileHistory  _fileHistory;
    enum {
        ID_PREVIOUS=ID_LAST, ID_NEXT,
        ID_SET_INDEX1,       
        ID_GRAYMAP1,         ID_GRAYMAP2,
        ID_CINE,             ID_RESET,
        ID_CINE_TIMER, ID_CINE_SLIDER,
        ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD,
        ID_CENTER1_SLIDER, ID_CENTER2_SLIDER,
        ID_WIDTH1_SLIDER,  ID_WIDTH2_SLIDER,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        ID_INVERT1,        ID_INVERT2,
        ID_SLICE1_SLIDER,  /*ID_SLICE2_SLIDER,*/
        ID_SCALE1_SLIDER,  /*ID_SCALE2_SLIDER,*/
        ID_ZOOM_IN, ID_ZOOM_OUT,
        ID_OVERLAY, ID_BUSY_TIMER,
        ID_FILTER, ID_FILTER_TYPE, ID_SAVE,ID_FILTER_SIGMA, ID_FILTER_INHOMO1,
        ID_FILTER_VOL_THRESH, ID_FILTER_STOP_COND, ID_FILTER_LEFT_PAR,
        ID_FILTER_RIGHT_PAR, ID_FILTER_ITERATIONS, ID_FILTER_INHOMO2,
        ID_FILTER_MORPH, ID_FILTER_ZETAFACTOR, ID_FILTER_INCLUFACTOR,
        ID_FILTER_STDDEVIA, ID_FILTER_NUMOFREGION,
        ID_FILTER_MAX_BALL, ID_FILTER_MIN_BALL,
        ID_FILTER_LTDT3D,
        ID_FILTER_LTDT3D_PARARELLID, ID_FILTER_LTDT3D_FTSAVEID,
        ID_FILTER_SBAV3D_PARARELLID, ID_FILTER_INHOMO3, ID_BALL_ENH,
        ID_FILTER_MIN0, ID_FILTER_MIN1, ID_FILTER_MIN2, ID_FILTER_MIN3,
        ID_FILTER_MAX0, ID_FILTER_MAX1, ID_FILTER_MAX2, ID_FILTER_MAX3,
        ID_BG_PROCESS
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
  CavassFilterType  m_FilterType;
  enum           { TypeNumber = 25 };
  wxString       filterName[ TypeNumber ];

    int            mFileOrDataCount;    ///< two needed for overlay
protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxFlexGridSizer*  fgs;
    wxComboBox*      m_filterTypeBut;
    wxCheckBox    *mBGSwitch;

    bool           mBGProcess;

    //cine related items
    bool           mForward, mForwardBackward, mDirectionIsForward;  ///< cine direction
    wxTimer*       m_cine_timer;        ///< time for cine

    void initializeMenu ( void );
    void addButtonBox ( void );
    void hideParametersBox(void);
    void updateParametersBox(void);

    void OnCineTimer           ( wxTimerEvent& e    );
    void OnCineForward         ( wxCommandEvent& e  );
    void OnCineForwardBackward ( wxCommandEvent& e  );
    void OnCineSlider          ( wxScrollEvent& e   );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
private:
    void removeAll ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    static void createFilterFrame ( wxFrame* parentFrame, bool useHistory=true );
    FilterFrame ( bool maximize=false, int w=800, int h=600 );
    ~FilterFrame ( void );
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };

    virtual void OnInput ( wxCommandEvent& unused );
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
    void OnCTLung       ( wxCommandEvent& unused );
    void OnCTSoftTissue ( wxCommandEvent& unused );
    void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
    void OnSlice1Slider  ( wxScrollEvent& e );
    void OnScale1Slider  ( wxScrollEvent& e );

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
    void OnGrayMap1     ( wxCommandEvent& unused );
    void OnGrayMap2     ( wxCommandEvent& unused );
    void OnCine         ( wxCommandEvent& unused );
    void OnReset        ( wxCommandEvent& unused );
    
    void OnFilter                 ( wxCommandEvent& e );
    void OnFilterType             ( wxCommandEvent& e );
    void OnFilterSave             ( wxCommandEvent& e );
    void OnFilterSigma            ( wxCommandEvent& e );
    void OnFilterScaleIterations  ( wxCommandEvent& e );
    void OnFilterHomogen          ( wxCommandEvent& e );
    void OnFilterMorph            ( wxCommandEvent& e );
    void OnLTDT3DDistType         ( wxCommandEvent& e );
    void OnLTDT3DPararell         ( wxCommandEvent& e );
    void OnLTDT3DFTSave           ( wxCommandEvent& e );    
    void OnSBAV3DPararell         ( wxCommandEvent& e );
    void OnBG                     ( wxCommandEvent& e );

    void OnFilterZetaFactor       ( wxCommandEvent& e );
    void OnFilterIncluFactor      ( wxCommandEvent& e );
    void OnFilterStdDevia         ( wxCommandEvent& e );
    void OnFilterNumofRegion      ( wxCommandEvent& e );
    void OnFilterMaxRadius        ( wxCommandEvent& e );
    void OnFilterMinRadius        ( wxCommandEvent& e );
    void OnFilterVolThresh        ( wxCommandEvent& e );
    void OnFilterStopCond         ( wxCommandEvent& e );
    void OnFilterLeftParam        ( wxCommandEvent& e );
    void OnFilterRightParam       ( wxCommandEvent& e );
    void OnFilterMin0             ( wxCommandEvent& e );
    void OnFilterMax0             ( wxCommandEvent& e );
    void OnFilterMin1             ( wxCommandEvent& e );
    void OnFilterMax1             ( wxCommandEvent& e );
    void OnFilterMin2             ( wxCommandEvent& e );
    void OnFilterMax2             ( wxCommandEvent& e );
    void OnFilterMin3             ( wxCommandEvent& e );
    void OnFilterMax3             ( wxCommandEvent& e );

    void OnSaveScreen   ( wxCommandEvent& e );

    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );
    void OnZoomIn       ( wxCommandEvent& e );
    void OnZoomOut      ( wxCommandEvent& e );

    void OnInvert1      ( wxCommandEvent& e );
    void OnInvert2      ( wxCommandEvent& e );
    void OnOverlay      ( wxCommandEvent& e );

    void OnHideControls ( wxCommandEvent& e );
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( FilterFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
