/*
  Copyright 1993-2008, 2017 Medical Image Processing Group
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
 * \file   AnalyzeStaticFrame.h
 * \brief  AnalyzeStaticFrame definition.
 * \author Ying Zhuge, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __AnalyzeStaticFrame_h
#define __AnalyzeStaticFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "AnalyzeStaticCanvas.h"
//#include  "wxFPSlider.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

/** \brief AnalyzeStaticFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class AnalyzeStaticFrame : public MainFrame 
{
  
  
public:
    static wxFileHistory  _fileHistory;
    enum {        
     //   ID_ZOOM_IN, ID_ZOOM_OUT,
        ID_OVERLAY, ID_BUSY_TIMER,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN, 
         ID_SAVE
    };
    enum {
        WHICH_BOTH, WHICH_FIRST, WHICH_SECOND
    };
  
    int            mFileOrDataCount;    ///< two needed for overlay
//    wxSlider*      m_center;            ///< contrast center slider
//    wxSlider*      m_width;             ///< contrast width slider
  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxSizer*       mBottomSizer;
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxFlexGridSizer*  fgs;
    wxButton*      m_AnalyzeStaticTypeBut;

  //wxTextCtrl*    mSigmaText;
    
    
    void initializeMenu ( void );
    void addButtonBox ( void );
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    AnalyzeStaticFrame ();
    ~AnalyzeStaticFrame ( void );

    void loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );
        
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
    
    void OnAnalyzeStaticSave             (wxCommandEvent& e );    

    void OnSaveScreen   ( wxCommandEvent& e );
    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );
 //   void OnZoomIn       ( wxCommandEvent& e );
 //   void OnZoomOut      ( wxCommandEvent& e );

    
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( AnalyzeStaticFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
