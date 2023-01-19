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
// TrapezoidTransform
//----------------------------------------------------------------------
#ifndef __TrapezoidTransform_h
#define __TrapezoidTransform_h

#include  "GrayScaleTransform.h"

class TrapezoidTransform : public GrayScaleTransform {
  private:
    double            m_slope;
    wxStaticText*     m_slope_st;
    wxTextCtrl*       m_slope_text;
    wxSlider*         m_slope_slider;

    double            m_yintercept;
    wxStaticText*     m_yintercept_st;
    wxTextCtrl*       m_yintercept_text;
    wxSlider*         m_yintercept_slider;

    double            m_slope2;
    wxStaticText*     m_slope_st2;
    wxTextCtrl*       m_slope_text2;
    wxSlider*         m_slope_slider2;

    double            m_yintercept2;
    wxStaticText*     m_yintercept_st2;
    wxTextCtrl*       m_yintercept_text2;
    wxSlider*         m_yintercept_slider2;

    enum {
        ID_SLOPE=ID_NEXT,  ID_SLOPE_TEXT,
        ID_YINTERCEPT,     ID_YINTERCEPT_TEXT,

        ID_SLOPE2,         ID_SLOPE_TEXT2,
        ID_YINTERCEPT2,    ID_YINTERCEPT_TEXT2
    };

    void doTransform ( void );

    inline double trapezoid ( const double x, const int max, const int min ) const {
        //input :  x (typically) in the range min..max inclusive
        //output:  in the range of 0..1 inclusive
        double  value = (m_slope * x + m_yintercept) / (max-min);
        const double  value2 = (m_slope2 * x + m_yintercept2) / (max-min);
        if (value2<value)  value=value2;
        if (value<0.0)    value=0.0;
        if (value>1.0)    value=1.0;
        if (mInvert)    return 1.0 - value;
        return value;
    }

  public:
    TrapezoidTransform ( MontageFrame* parent=NULL );
    void init ( void );
    void setupSliders ( void );

    void OnSlopeSlider            ( wxScrollEvent&  e );
    void OnSlopeTextChanged       ( wxCommandEvent& e );
    void OnSlopeTextEnter         ( wxCommandEvent& e );

    void OnYInterceptSlider       ( wxScrollEvent&  e );
    void OnYInterceptTextChanged  ( wxCommandEvent& e );
    void OnYInterceptTextEnter    ( wxCommandEvent& e );

    void OnSlopeSlider2           ( wxScrollEvent&  e );
    void OnSlopeTextChanged2      ( wxCommandEvent& e );
    void OnSlopeTextEnter2        ( wxCommandEvent& e );

    void OnYInterceptSlider2      ( wxScrollEvent&  e );
    void OnYInterceptTextChanged2 ( wxCommandEvent& e );
    void OnYInterceptTextEnter2   ( wxCommandEvent& e );

  private:
    DECLARE_DYNAMIC_CLASS(TrapezoidTransform)
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
