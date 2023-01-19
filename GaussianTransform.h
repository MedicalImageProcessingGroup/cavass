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
// GaussianTransform
//----------------------------------------------------------------------
#ifndef __GaussianTransform_h
#define __GaussianTransform_h

#include  "GrayScaleTransform.h"

class GaussianTransform : public GrayScaleTransform {
  private:
    static const int     sMaxSliderValue;
    static const double  sScaleMin;
    static const double  sScaleMax;

    double            mMean;
    wxStaticText*     m_mean_st;
    wxTextCtrl*       m_mean_text;
    wxSlider*         m_mean_slider;

    double            mStddev;
    wxStaticText*     m_stddev_st;
    wxTextCtrl*       m_stddev_text;
    wxSlider*         m_stddev_slider;

    double            mScale;
    wxStaticText*     m_scale_st;
    wxTextCtrl*       m_scale_text;
    wxSlider*         m_scale_slider;

    enum {
        ID_MEAN=ID_NEXT,   ID_MEAN_TEXT,
        ID_STDDEV,         ID_STDDEV_TEXT,
        ID_SCALE,          ID_SCALE_TEXT
    };

    void doTransform ( void );

    inline double gauss ( const double x ) const {
        //input : x (typically) in the range min..max inclusive
        //output: in the range of 0..1 inclusive
        const double  a     = (x-mMean)/mStddev;
        double        value = mScale * exp(-0.5*a*a);
        if (value>1.0)    value=1.0;
        if (value<0.0)    value=0.0;
        if (mInvert)    return 1.0 - value;
        return value;
    }

  public:
    GaussianTransform ( MontageFrame* parent=NULL );
    void init ( void );
    void setup ( void );

    void OnMeanSlider        ( wxScrollEvent&  e );
    void OnMeanTextChanged   ( wxCommandEvent& e );
    void OnMeanTextEnter     ( wxCommandEvent& e );
    void OnChar ( wxKeyEvent& e );

    void OnStddevSlider      ( wxScrollEvent&  e );
    void OnStddevTextChanged ( wxCommandEvent& e );
    void OnStddevTextEnter   ( wxCommandEvent& e );

    void OnScaleSlider       ( wxScrollEvent&  e );
    void OnScaleTextChanged  ( wxCommandEvent& e );
    void OnScaleTextEnter    ( wxCommandEvent& e );

  private:
    DECLARE_DYNAMIC_CLASS(GaussianTransform)
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
