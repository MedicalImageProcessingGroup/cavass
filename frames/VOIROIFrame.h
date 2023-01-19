/*
  Copyright 1993-2013, 2015, 2017 Medical Image Processing Group
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
 * \file   VOIROIFrame.h
 * \brief  VOIROIFrame definition.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __VOIROIFrame_h
#define __VOIROIFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "VOIROICanvas.h"
//#include  "wxFPSlider.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  CineControls;
class  GrayMapControls;
class  SaveScreenControls;
class  SetFilterIndexControls;
class  SetROIParaControls;

/** \brief VOIROIFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class VOIROIFrame : public MainFrame 
{
  CineControls*             mCineControls;
  GrayMapControls*          mGrayMap1Controls;
  GrayMapControls*          mGrayMap2Controls;
  SetFilterIndexControls*   mSetIndex1Controls;
  SetROIParaControls*       mSetROIParaControls;
  int                       mWhichROI;     
  wxComboBox*               mROIName;   
  int                       mWhichGrad;     
  wxComboBox*               mGradName;   

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
        ID_SCALE1_SLIDER,  
		ID_GRAYMAP2, ID_VOIPARA,
        ID_STARTSLICE, ID_ENDSLICE, ID_STARTVOLUME, ID_ENDVOLUME,
		ID_LOCX, ID_LOCY, ID_WINWIDTH, ID_WINHEIGHT,
        ID_OVERLAY, ID_BUSY_TIMER,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN, ID_CHECKBOX_LAYOUT,
        ID_LAYOUT, ID_VOIROI_ROI, ID_VOIROI_GRAD, ID_SAVE
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
  
  enum           {TypeNumber = 20};
  wxString       VOIROIName[TypeNumber];

    int            mFileOrDataCount;    ///< two needed for overlay
  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxFlexGridSizer*  fgs;
    wxButton*      m_VOIROITypeBut;

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
    static void createVOIROIFrame ( wxFrame* parentFrame, bool useHistory=true );
    VOIROIFrame ( bool maximize=false, int w=800, int h=600 );
    ~VOIROIFrame ( void );
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
	void updateROITablets ( void );

    void OnCenter1Slider ( wxScrollEvent& e );
    void OnWidth1Slider  ( wxScrollEvent& e );
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
    void OnSlice1Slider  ( wxScrollEvent& e );
    void OnScale1Slider  ( wxScrollEvent& e );
	void OnCenter2Slider ( wxScrollEvent& e );
    void OnWidth2Slider  ( wxScrollEvent& e );

	void OnStartTablet     ( wxCommandEvent& e );
    void OnEndTablet       ( wxCommandEvent& e );
	void OnStartVolTablet  ( wxCommandEvent& e );
	void OnEndVolTablet    ( wxCommandEvent& e );
    void OnLocXTablet      ( wxCommandEvent& e );
    void OnLocYTablet      ( wxCommandEvent& e );
	void OnWinWidthTablet  ( wxCommandEvent& e );
    void OnWinHeightTablet ( wxCommandEvent& e );

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
    
    void OnVOIROISave             (wxCommandEvent& e );    

    void OnSaveScreen   ( wxCommandEvent& e );
    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );

    void OnInvert1      ( wxCommandEvent& e );      
	void OnInvert2      ( wxCommandEvent& e );  
	void OnOverlay      ( wxCommandEvent& e );
    
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( VOIROIFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
