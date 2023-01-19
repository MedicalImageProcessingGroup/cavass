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
 * \file   VOIIOIFrame.h
 * \brief  VOIIOIFrame definition.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __VOIIOIFrame_h
#define __VOIIOIFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "VOIIOICanvas.h"
//#include  "wxFPSlider.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  CineControls;
class  GrayMapControls;
class  SaveScreenControls;
class  SetFilterIndexControls;
class  SetIOIControls;
class  SetSH0MIPControls;

/** \brief VOIIOIFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class VOIIOIFrame : public MainFrame 
{
  CineControls*             mCineControls;
  GrayMapControls*          mGrayMap1Controls;
  GrayMapControls*          mGrayMap2Controls;
  SetFilterIndexControls*   mSetIndex1Controls;
  SetIOIControls*           mSetIOIControls;
  SetSH0MIPControls*        mSetSH0MIPControls;
  HistZoomControls*         mHistZoomControls;
  int                       mWhichROI;     
  wxComboBox*               mROIName;   
  int                       mWhichGrad;     
  wxComboBox*               mSH0Type;

public:
    static wxFileHistory  _fileHistory;
    enum {
        ID_PREVIOUS=ID_LAST, ID_NEXT,
        ID_SET_INDEX1,       
        ID_GRAYMAP1,         ID_ERASE,
        ID_CINE,             ID_RESET,
        ID_CINE_TIMER, ID_CINE_SLIDER, ID_CINE_FORWARD, ID_CINE_FORWARD_BACKWARD,
        ID_CENTER1_SLIDER, ID_CENTER2_SLIDER, 
        ID_WIDTH1_SLIDER,  ID_WIDTH2_SLIDER,  
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        ID_INVERT1,        ID_INVERT2,        
        ID_SLICE1_SLIDER,  ID_SLICE2_SLIDER,  
        ID_SCALE1_SLIDER,  ID_SH0TYPE, ID_SH0_MINTHRE, ID_SH0_MAXTHRE,
		ID_GRAYMAP2, ID_VOIPARA, ID_HIST_ZOOM, ID_HIST_ZOOM_SLIDER,
        ID_STARTSLICE, ID_ENDSLICE, ID_LOCX, ID_LOCY, ID_WINWIDTH, ID_WINHEIGHT,
        ID_OVERLAY, ID_BUSY_TIMER,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN, ID_CHECKBOX_LAYOUT,
        ID_LAYOUT, ID_VOIIOI_ROI, ID_VOIIOI_GRAD, ID_SAVE
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
  
  enum           {TypeNumber = 20};
  wxString       VOIIOIName[TypeNumber];

    int            mFileOrDataCount;    ///< two needed for overlay
//    wxSlider*      m_center;            ///< contrast center slider
//    wxSlider*      m_width;             ///< contrast width slider
  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxFlexGridSizer*  fgs;
    wxButton*      m_VOIIOITypeBut;

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
    static void createVOIIOIFrame ( wxFrame* parentFrame, bool useHistory=true );
    VOIIOIFrame ( bool maximize=false, int w=800, int h=600 );
    ~VOIIOIFrame ( void );
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
    void OnWidth1Slider  ( wxScrollEvent& e );    
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
    void OnSlice1Slider  ( wxScrollEvent& e );
//	void OnSlice2Slider  ( wxScrollEvent& e );
    void OnScale1Slider  ( wxScrollEvent& e );
	void OnCenter2Slider ( wxScrollEvent& e );    
    void OnWidth2Slider  ( wxScrollEvent& e );    

	void OnStartSlider ( wxScrollEvent& e );    
    void OnEndSlider  ( wxScrollEvent& e );    
    void OnLocXSlider  ( wxScrollEvent& e );
    void OnLocYSlider  ( wxScrollEvent& e );
	void OnWinWidthSlider ( wxScrollEvent& e );    
    void OnWinHeightSlider  ( wxScrollEvent& e );    
	void OnHistZoom ( wxCommandEvent& unused );
	void OnHistZoomSlider ( wxScrollEvent& e );
	void OnSH0Type ( wxCommandEvent& e );
	void OnMIPMinThreSlider ( wxScrollEvent& e );
	void OnMIPMaxThreSlider ( wxScrollEvent& e );

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
	
	void OnSetVOIPara     ( wxCommandEvent& unused );

	void OnErase        ( wxCommandEvent& unused ); 
	void OnHideControls ( wxCommandEvent& e );

    void OnCine         ( wxCommandEvent& unused );
    void OnReset        ( wxCommandEvent& unused );
    
//    void OnLayout                 ( wxCommandEvent& e );
	//void OnVOIIOIGrad             (wxCommandEvent& e );
 //   void OnVOIIOIROI             (wxCommandEvent& e );
    void OnVOIIOISave             (wxCommandEvent& e );    

    void OnSaveScreen   ( wxCommandEvent& e );
    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );
 //   void OnZoomIn       ( wxCommandEvent& e );
 //   void OnZoomOut      ( wxCommandEvent& e );

    void OnInvert1      ( wxCommandEvent& e );      
	void OnInvert2      ( wxCommandEvent& e );  
	void OnOverlay      ( wxCommandEvent& e );
    
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( VOIIOIFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
