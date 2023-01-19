/*
  Copyright 1993-2013, 2015, 2018-2019 Medical Image Processing Group
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
 * \file   CycleFrame.h
 * \brief  CycleFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __CycleFrame_h
#define __CycleFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"

#include  "MainFrame.h"
#include  "MainCanvas.h"
class  CycleCanvas;
class  GrayMapControls;
class  SetIndexControls;
class  TextControls;
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class CycleFrame : public MainFrame {
    friend class CycleCanvas;
protected:
    wxComboBox*        mDataset;           ///< current dataset (all, 1, ..., N)
    int                mFileOrDataCount;   ///< count data/datafiles associated w/ this frame
    CineControls*      mCineControls;      ///< cine controls
    GrayMapControls*   mGrayMapControls;   ///< gray map controls
    SetIndexControls*  mSetIndexControls;  ///< set index controls
    TextControls*      mTextControls;      ///< text controls
    int                mWhichDataset;
    enum {             DatasetAll=0 };
public:
    wxComboBox*        mMode;              ///< current mode (see enum below)
    int                mWhichMode;
    enum { ModeLayout, ModeRoi, ModeCine, ModeContinuous, ModeDiscontinuous,
           ModeFastCine, ModeErase, ModeMeasure, ModeLock };

	int                mWhichRoiMode;
	enum { RoiModeNone, RoiModeSelect, RoiModeMoveOrResize };

    int                mWhichDimension;

    wxSlider*      m_center;
    wxSlider*      m_width;
    wxSlider*      m_sliceNo;

    #define        scaleIMin           10
    #define        scaleIMax         1000
    #define        scaleIDenominator  100
    wxSlider*      m_scale;
    wxStaticText*  m_scaleMinText;
    wxStaticText*  m_scaleText;
    wxStaticText*  m_scaleMaxText;
    int            m_old_scale_value;
    double         mScaleZoom;
    int            mZoomLevel;
    stack< pair<double,double> >  mZoomValues;

    wxArrayString  mMeasurements;

    void setSliceNo ( int sliceNo );
    void indicateMode ( void );
private:
    wxMenu*        m_options_menu;

    //cine related items
    bool           mForward, mForwardBackward, mDirectionIsForward;
    wxCheckBox*    mForwardCheck;
    wxCheckBox*    mForwardBackwardCheck;
    wxSlider*      mCineSlider;
    wxTimer*       m_cine_timer;
    bool           mCache;
    wxCheckBox*    mCacheCheck;

    wxCheckBox*    mLabelsCheck;
    wxCheckBox*    mInterpolateCheck;
    wxCheckBox*    mMotionCheck;
    wxCheckBox*    mLockCheck;
    wxCheckBox*    mMeasureCheck;

    void OnCineTimer           ( wxTimerEvent& e );
    void OnCineForward         ( wxCommandEvent& e );
    void OnCineForwardBackward ( wxCommandEvent& e );
    void OnCineSlider          ( wxScrollEvent& e );
    void OnPrintPreview        ( wxCommandEvent& e );
    void OnPrint               ( wxCommandEvent& e );
    void OnCineDimension       ( wxCommandEvent& e );
#ifdef __WXX11__
    void OnUpdateUICineSlider  ( wxUpdateUIEvent& e );
#endif

    enum {
        ID_RESET=ID_LAST, ID_ERASE, ID_LABELS, ID_INTERPOLATE, ID_GRAYMAP,
        ID_SET_INDEX, ID_DURATION, ID_MOTION, ID_LOCK, ID_MEASURE,

        ID_MAGNIFY, ID_MODE,
        ID_DATASET, ID_CINE,

        ID_CINE_TIMER, ID_CINE_SLIDER,
        ID_CENTER_SLIDER, ID_WIDTH_SLIDER,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
            ID_SLICE_SLIDER, ID_SCALE_SLIDER, ID_ZOOM_IN, ID_ZOOM_OUT,
            ID_NEW,
            ID_SAVE,
            ID_INVERT,
            ID_BUSY_TIMER,
            ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD, ID_CACHE,
            ID_CINE_DIMENSION
    };

    int  m_lastChoice, m_lastChoiceSetting;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
    static void createCycleFrame ( wxFrame* parentFrame, bool useHistory=true );
    CycleFrame ( bool maximize=false, int w=800, int h=600 );
    ~CycleFrame ( void );
	static bool match ( wxString filename ) {
		return MontageFrame::match( filename );
	};
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
		return MontageFrame::match( filename );
    };

    void loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );

    void OnCine         ( wxCommandEvent& unused );
    void OnGrayMap      ( wxCommandEvent& unused );
    void OnDataset      ( wxCommandEvent& unused );
//    void OnMagnify      ( wxCommandEvent& unused );
    void OnMode         ( wxCommandEvent& unused );
    void OnReset        ( wxCommandEvent& unused );
    void OnSetIndex     ( wxCommandEvent& unused );

    void OnCenterSlider ( wxScrollEvent& e );
    void OnWidthSlider  ( wxScrollEvent& e );
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
    void OnSliceSlider  ( wxScrollEvent& e );
    void OnScaleSlider  ( wxScrollEvent& e );
	void OnMouseWheel   ( wxMouseEvent& e );

    void OnBusyTimer    ( wxTimerEvent& e  );
#ifdef __WXX11__
    void OnUpdateUICenterSlider ( wxUpdateUIEvent& e );
    void OnUpdateUIWidthSlider  ( wxUpdateUIEvent& e );
    void OnUpdateUISliceSlider  ( wxUpdateUIEvent& e );
    void OnUpdateUIScaleSlider  ( wxUpdateUIEvent& e );
    void OnUpdateUIDemonsSlider ( wxUpdateUIEvent& e );
#endif
    virtual void OnInput                      ( wxCommandEvent& unused );
    void OnSave                               ( wxCommandEvent& e );
    void OnOpen                               ( wxCommandEvent& e );
    void OnZoomIn                             ( wxCommandEvent& e );
    void OnZoomOut                            ( wxCommandEvent& e );
    void OnLabels                             ( wxCommandEvent& e );
    void OnInvert                             ( wxCommandEvent& e );
    void OnHideControls                       ( wxCommandEvent& e );
    void OnCache                              ( wxCommandEvent& e );
    void OnInterpolateData                    ( wxCommandEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnPropagateContrast                  ( wxCommandEvent& e );
    void OnPropagateContrastSliceLabels       ( wxCommandEvent& e );
    void OnPropagateContrastSliceLabelsScale  ( wxCommandEvent& e );

    void OnChar ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
    wxStaticText*   m_demonsText;

    template< typename T >
    inline T getData ( const T* const p, const int x, const int y ) {
        if (x<0 || x>=mCanvas->mCavassData->m_xSize || y<0 || y>=mCanvas->mCavassData->m_ySize)
            return 0;
        return p[y*mCanvas->mCavassData->m_xSize + x];
    }

    template< typename T >
    inline T getData ( const T* const p, const int x, const int y, const int z ) {
        if ( x<0 || x>=mCanvas->mCavassData->m_xSize || y<0 || y>=mCanvas->mCavassData->m_ySize
          || z<0 || z>=mCanvas->mCavassData->m_zSize )
            return 0;
        return p[z*mCanvas->mCavassData->m_ySize*mCanvas->mCavassData->m_xSize + y*mCanvas->mCavassData->m_xSize + x];
//        return p[ m_zsub[z] + m_ysub[y] + x ];
    }

    template< typename T >
    inline T get2DSmoothedData ( const T* const p, const int x, const int y,
                                 const int z )
    {
        double  sum=0.0;

        sum += getData(p, x-1, y-1, z);
        sum += getData(p, x,   y-1, z);
        sum += getData(p, x+1, y-1, z);

        sum += getData(p, x-1, y,   z);
        sum += getData(p, x,   y,   z) * 2;
        sum += getData(p, x+1, y,   z);

        sum += getData(p, x-1, y+1, z);
        sum += getData(p, x,   y+1, z);
        sum += getData(p, x+1, y+1, z);

        return (int)(sum/10.0+0.5);
    }
#endif
  private:
    void initializeMenu ( void );
    void addButtonBox ( void );
//    void uncheckExcept ( const int leaveAloneID );
    void removeAll ( void );

public:
    DECLARE_DYNAMIC_CLASS( CycleFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
