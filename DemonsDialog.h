/*
  Copyright 1993-2008 Medical Image Processing Group
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
// DemonsDialog
//----------------------------------------------------------------------
#ifndef __DemonsDialog_h
#define __DemonsDialog_h

class DemonsDialog : public wxDialog {
  private:
    wxFrame*  m_parent;
    int       m_seconds;  //elapsed time in seconds (when busy processing)

    int            m_it_value;
    wxSlider*      m_it_slider;
      #define  DemonsITMin            1
      #define  DemonsITMax            500
      #define  DemonsITDefault        10

    int            m_mp_value;
    wxSlider*      m_mp_slider;
      #define  DemonsMPMin            5
      #define  DemonsMPMax            500
      #define  DemonsMPDefault        10

    int            m_hl_value;
    wxSlider*      m_hl_slider;
      #define  DemonsHLMin            10
      #define  DemonsHLMax            2000
      #define  DemonsHLDefault        50

    int            m_sd_value;
    wxStaticText*  m_sd_text;  // text is also neede for this one because
                               // we don't have a floating point slider
    wxSlider*      m_sd_slider;
      #define  DemonsSDMin            5    //actually 0.5
      #define  DemonsSDMax            100  //actually 10.0
      #define  DemonsSDDefault        10   //actually 1.0

    wxRadioBox*    m_dimension;        ///< 2d or 3d
    int            m_dim_value;
    wxRadioBox*    m_thresholdAtMean;  ///< yes or no
    bool           m_tam_value;
    wxRadioBox*    m_normalize;        ///< yes or no
    bool           m_norm_value;

    wxButton*      m_ok;
    wxButton*      m_cancel;

    enum {
        ID_ITERATIONS=200, ID_MATCH_POINTS, ID_HISTOGRAM_LEVELS,
        ID_STANDARD_DEVIATION, ID_DIMENSION, ID_THRESHOLD_AT_MEAN,
        ID_NORMALIZE, ID_BUSY_TIMER
    };

  public:
    DemonsDialog ( wxFrame* parent=NULL );
    void init ( void );

    void OnITSlider ( wxScrollEvent& e );
    void OnMPSlider ( wxScrollEvent& e );
    void OnHLSlider ( wxScrollEvent& e );
    void OnSDSlider ( wxScrollEvent& e );

    void OnDimRadiobox  ( wxCommandEvent& e );
    void OnTAMRadiobox  ( wxCommandEvent& e );
    void OnNormRadiobox ( wxCommandEvent& e );

    void OnDoDemonsRegistration ( wxCommandEvent& e );
    void OnDoDemons2DRegistration ( );
    void OnBusyTimer ( wxTimerEvent& e );

  private:
    DECLARE_DYNAMIC_CLASS(DemonsDialog)
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================

