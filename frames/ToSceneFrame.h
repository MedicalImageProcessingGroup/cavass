/*
  Copyright 2023 Medical Image Processing Group
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
#ifndef __ToSceneFrame_h
#define __ToSceneFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "ToSceneCanvas.h"

#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

/** \brief ToSceneFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class ToSceneFrame : public MainFrame 
{
  
  
public:
    static wxFileHistory  _fileHistory;
    enum {        
        ID_OVERLAY, ID_BUSY_TIMER,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN, 
        ID_STRUCT_NUMBER, ID_SAVE
    };
  
    int            mFileOrDataCount;    ///< two needed for overlay
    wxString mInputBS0;
  protected:
    wxString       mInputFile;
    wxMenu*        m_options_menu;      ///< options menu
    wxStaticBox*   mSetIndex1Box;
    wxSizer*       mSetIndex1Sizer;
    wxFlexGridSizer*  fgs;
    wxButton*      m_structN;
    int structure_number, number_of_structures;
    
    void initializeMenu ( void );
    void addButtonBox ( void );
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    ToSceneFrame ();
    ~ToSceneFrame ( void );

    void loadFile ( wxString& fname );
        
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
    
    void OnToSceneSave             (wxCommandEvent& e );    
    void OnStructNumber ( wxCommandEvent& unused );

    void OnSaveScreen   ( wxCommandEvent& e );
    void OnCopy         ( wxCommandEvent& e );
    void OnOpen         ( wxCommandEvent& e );
    virtual void OnInformation     ( wxCommandEvent& unused );
    
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ToSceneFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
