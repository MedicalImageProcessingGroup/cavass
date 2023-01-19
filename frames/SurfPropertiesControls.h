/*
  Copyright 1993-2012 Medical Image Processing Group
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
 * \file   SurfPropertiesControls.h
 * \brief  Definition and implementation of SurfPropertiesControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SurfPropertiesControls_h
#define __SurfPropertiesControls_h

/**
 * \brief Definition and implementation of SurfPropertiesControls class.
 *
 * Surface properties controls consist of a control box with sliders for
 * percentage, exponent, and divisor.  All slider values are doubles with
 * 2 digits of precision.
 *
 * Note: Callbacks are handled by the caller.
 */
class SurfPropertiesControls {
    #define  min       0  ///< min value for properties slider values
    #define  factor  100  ///< multiplier (for 2 digits of precision)
    bool               mHorizontal;   ///< orientation of sliders
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mPropertiesBox;
    wxStaticBoxSizer*  mPropertiesSizer;
    wxFlexGridSizer*   mFgs;
    wxSlider           *mPercent,     *mExponent,     *mDivisor;      ///< sliders
    wxStaticText       *mPercentText, *mExponentText, *mDivisorText;  ///< slider labels
    double             mPercentMin,   mExponentMin,   mDivisorMin,
                       mPercentMax,   mExponentMax,   mDivisorMax;
public:
    /** \brief SurfPropertiesControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title         is the title for the control area/box.
     *  \param currentPercent    is the current percent value.
     *  \param currentExponent  is the current exponent value.
     *  \param currentDivisor   is the current divisor value.
     *  \param percentSliderID   is the ID associated with the percent slider.
     *  \param exponentSliderID is the ID associated with the exponent slider.
     *  \param divisorSliderID  is the ID associated with the divisor slider.
     */
    SurfPropertiesControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,
        const double currentPercent,  const double currentExponent,  const double currentDivisor,
        const double minPercent,      const double minExponent,      const double minDivisor,
        const double maxPercent,      const double maxExponent,      const double maxDivisor,
        const int    percentSliderID, const int    exponentSliderID, const int    divisorSliderID,
        const bool horizontal=true )
    {
        assert( minPercent  <= currentPercent  && currentPercent  <= maxPercent  );
        assert( minExponent <= currentExponent && currentExponent <= maxExponent );
        assert( minDivisor  <= currentDivisor  && currentDivisor  <= maxDivisor  );

        mPercentMin  = minPercent;
        mExponentMin = minExponent;
        mDivisorMin  = minDivisor;
        mPercentMax  = maxPercent;
        mExponentMax = maxExponent;
        mDivisorMax  = maxDivisor;

        mHorizontal = horizontal;
        mBottomSizer = bottomSizer;
        mPropertiesBox = new wxStaticBox( cp, -1, title );
        ::setColor( mPropertiesBox );
        mPropertiesSizer = new wxStaticBoxSizer( mPropertiesBox, wxHORIZONTAL );
        if (mHorizontal)
            mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        else
            mFgs = new wxFlexGridSizer( 6, 0, 5 );  //6 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        if (mHorizontal) {
            mFgs->AddGrowableCol( 0 );
            mFgs->AddGrowableRow( 0 );
            mFgs->AddGrowableRow( 1 );
            mFgs->AddGrowableRow( 2 );
        }
        //percent - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mPercentText = new wxStaticText( cp, -1, "percent:" );
        ::setColor( mPercentText );
        mFgs->Add( mPercentText, 0, wxALIGN_RIGHT );
        
        if (mHorizontal)
            mPercent = new wxSlider( cp, percentSliderID,
                (int)(currentPercent*factor), (int)(minPercent*factor), (int)(maxPercent*factor),
                wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL, wxDefaultValidator, "percent" );
        else
            mPercent = new wxSlider( cp, percentSliderID,
                (int)(currentPercent*factor), (int)(minPercent*factor), (int)(maxPercent*factor),
                wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "percent" );
        ::setColor( mPercent );
        mPercent->SetPageSize( 5 );
        mFgs->Add( mPercent, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM );
        //exponent - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mExponentText = new wxStaticText( cp, -1, "exponent:" );
        ::setColor( mExponentText );
        mFgs->Add( mExponentText, 0, wxALIGN_RIGHT );
        
        if (mHorizontal)
            mExponent = new wxSlider( cp, exponentSliderID,
                (int)(currentExponent*factor), (int)(minExponent*factor), (int)(maxExponent*factor),
                wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL, wxDefaultValidator, "exponent" );
        else
            mExponent = new wxSlider( cp, exponentSliderID,
                (int)(currentExponent*factor), (int)(minExponent*factor), (int)(maxExponent*factor),
                wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "exponent" );
        ::setColor( mExponent );
        mExponent->SetPageSize( 5 );
        mFgs->Add( mExponent, 0, wxGROW|wxLEFT|wxRIGHT, 0 );
        //divisor - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mDivisorText = new wxStaticText( cp, -1, "divisor:" );
        ::setColor( mDivisorText );
        mFgs->Add( mDivisorText, 0, wxALIGN_RIGHT );
        
        if (mHorizontal)
            mDivisor = new wxSlider( cp, divisorSliderID,
                (int)(currentDivisor*factor), (int)(minDivisor*factor), (int)(maxDivisor*factor),
                wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_HORIZONTAL, wxDefaultValidator, "divisor" );
        else
            mDivisor = new wxSlider( cp, divisorSliderID,
                (int)(currentDivisor*factor), (int)(minDivisor*factor), (int)(maxDivisor*factor),
                wxDefaultPosition, wxSize(sliderWidth, -1), wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "divisor" );
        ::setColor( mDivisor );
        mDivisor->SetPageSize( 5 );
        mFgs->Add( mDivisor, 0, wxGROW|wxLEFT|wxRIGHT, 0 );
        //- - - - -
        update();
        mPropertiesSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mPercent->SetToolTip(  "set percent amount"  );
                mExponent->SetToolTip( "set exponent amount" );
                mDivisor->SetToolTip(  "set divisor amount"  );
            #endif
        #endif
        mBottomSizer->Prepend( mPropertiesSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void update ( void ) {
        assert( factor==100 );
        if (!mHorizontal)    return;
        int  value = mPercent->GetValue();
        wxString  tmp = wxString::Format( "percent: %6.2f", (double)value/factor );
        mPercentText->SetLabel( tmp );

        value = mExponent->GetValue();
        tmp = wxString::Format( "exponent: %6.2f", (double)value/factor );
        mExponentText->SetLabel( tmp );

        value = mDivisor->GetValue();
        tmp = wxString::Format( "divisor: %6.2f", (double)value/factor );
        mDivisorText->SetLabel( tmp );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief values will be in [mPercentMin .. mPercentMax]. */
    double getPercent ( void ) {
        return (double)mPercent->GetValue() / factor;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief values will be in [mExponentMin .. mExponentMax]. */
    double getExponent ( void ) {
        return (double)mExponent->GetValue() / factor;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief values will be in [mDivisorMin .. mDivisorMax]. */
    double getDivisor ( void ) {
        return (double)mDivisor->GetValue() / factor;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SurfPropertiesControls dtor. */
    ~SurfPropertiesControls ( void ) {
        mBottomSizer->Remove( mPropertiesSizer );
        //delete mPropertiesBox;
        //delete mPropertiesSizer;
        //delete mFgs;
        //delete mFgsExponentSlider;
        //delete mFgsPercentSlider;
        //delete mFgsDivisorSlider;
        delete mExponent;
        delete mPercent;
        delete mDivisor;
        delete mPercentText;
        delete mExponentText;
        delete mDivisorText;
    }
};

#endif
