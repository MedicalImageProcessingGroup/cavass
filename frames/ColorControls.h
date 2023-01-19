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
 * \file   ColorControls.h
 * \brief  Definition and implementation of ColorControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ColorControls_h
#define __ColorControls_h

/**
 * \brief Definition and implementation of ColorControls class.
 *
 * Color controls consist of a control box with sliders for red, green, and
 * blue.
 *
 * Note: Callbacks are handled by the caller.
 */
class ColorControls {
    #define  colorSliderMin   0       ///< min for (rgb) color slider values
    #define  colorSliderMax   100     ///< max for (rgb) color slider values
    bool               mHorizontal;   ///< orientation of sliders
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mColorBox;
    wxStaticBoxSizer*  mColorSizer;
    wxFlexGridSizer*   mFgs;
    wxSlider           *mRed,            *mGreen,        *mBlue;        ///< rgb sliders
    wxStaticText       *mRedText0,       *mGreenText0,   *mBlueText0;   ///< rgb sliders labels
public:
    /** \brief ColorControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title         is the title for the control area/box.
     *  \param currentRed    is the current red value.
     *  \param currentGreen  is the current green value.
     *  \param currentBlue   is the current blue value.
     *  \param redSliderID   is the ID associated with the red slider.
     *  \param greenSliderID is the ID associated with the green slider.
     *  \param blueSliderID  is the ID associated with the blue slider.
     */
    ColorControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,
        const double currentRed, const double currentGreen, const double currentBlue,
        const int redSliderID, const int greenSliderID, const int blueSliderID,
        const bool horizontal=true )
    {
        assert( 0.0<=currentRed   && currentRed  <=1.0 );
        assert( 0.0<=currentGreen && currentGreen<=1.0 );
        assert( 0.0<=currentBlue  && currentBlue <=1.0 );
        mHorizontal = horizontal;
        mBottomSizer = bottomSizer;
        mColorBox = new wxStaticBox( cp, -1, title );
        ::setColor( mColorBox );
        mColorSizer = new wxStaticBoxSizer( mColorBox, wxHORIZONTAL );
        if (mHorizontal)
            mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        else
            mFgs = new wxFlexGridSizer( 6, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        if (mHorizontal) {
            mFgs->AddGrowableCol( 0 );
            mFgs->AddGrowableRow( 0 );
            mFgs->AddGrowableRow( 1 );
            mFgs->AddGrowableRow( 2 );
        }

        //red- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mRedText0 = new wxStaticText( cp, -1, "red:" );
        ::setColor( mRedText0 );
        mFgs->Add( mRedText0, 0, wxALIGN_RIGHT );
        
        if (mHorizontal)
            mRed = new wxSlider( cp, redSliderID, (int)(currentRed*colorSliderMax),
                colorSliderMin, colorSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_HORIZONTAL, wxDefaultValidator, "red" );
        else
            mRed = new wxSlider( cp, redSliderID, (int)(currentRed*colorSliderMax),
                colorSliderMin, colorSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "red" );
        ::setColor( mRed );
        mRed->SetPageSize( 5 );
        mFgs->Add( mRed, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM );

        //green- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mGreenText0 = new wxStaticText( cp, -1, "green:" );
        ::setColor( mGreenText0 );
        mFgs->Add( mGreenText0, 0, wxALIGN_RIGHT );
        
        if (mHorizontal)
            mGreen = new wxSlider( cp, greenSliderID, (int)(currentGreen*colorSliderMax),
                colorSliderMin, colorSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL, wxDefaultValidator, "green" );
        else
            mGreen = new wxSlider( cp, greenSliderID, (int)(currentGreen*colorSliderMax),
                colorSliderMin, colorSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "green" );
        ::setColor( mGreen );
        mGreen->SetPageSize( 5 );
        mFgs->Add( mGreen, 0, wxGROW|wxLEFT|wxRIGHT, 0 );

        //blue - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mBlueText0 = new wxStaticText( cp, -1, "blue:" );
        ::setColor( mBlueText0 );
        mFgs->Add( mBlueText0, 0, wxALIGN_RIGHT );
        
        if (mHorizontal)
            mBlue = new wxSlider( cp, blueSliderID, (int)(currentBlue*colorSliderMax),
                colorSliderMin, colorSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_HORIZONTAL, wxDefaultValidator, "blue" );
        else
            mBlue = new wxSlider( cp, blueSliderID, (int)(currentBlue*colorSliderMax),
                colorSliderMin, colorSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "blue" );
        ::setColor( mBlue );
        mBlue->SetPageSize( 5 );
        mFgs->Add( mBlue, 0, wxGROW|wxLEFT|wxRIGHT, 0 );
        //- - - - -
        update();
        mColorSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mRed->SetToolTip(   "set red amount" );
                mGreen->SetToolTip( "set green amount" );
                mBlue->SetToolTip(  "set blue amount" );
            #endif
        #endif
        mBottomSizer->Prepend( mColorSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void update ( void ) {
        if (!mHorizontal)    return;
        int  value = mRed->GetValue();
        wxString  tmp = wxString::Format( "red: %3d", value );
        mRedText0->SetLabel( tmp );

        value = mGreen->GetValue();
        tmp = wxString::Format( "green: %3d", value );
        mGreenText0->SetLabel( tmp );

        value = mBlue->GetValue();
        tmp = wxString::Format( "blue: %3d", value );
        mBlueText0->SetLabel( tmp );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void getRGB ( double& r, double& g, double& b ) {
        //values will be in [0.0 .. 1.0]
        r = (double)mRed->GetValue()   / colorSliderMax;
        g = (double)mGreen->GetValue() / colorSliderMax;
        b = (double)mBlue->GetValue()  / colorSliderMax;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief ColorControls dtor. */
    ~ColorControls ( void ) {
        mBottomSizer->Remove( mColorSizer );
     //   delete mColorBox;
        //delete mColorSizer;
        //delete mFgs;
        //delete mFgsGreenSlider;
        //delete mFgsRedSlider;
        //delete mFgsBlueSlider;
        delete mGreen;
        delete mRed;
        delete mBlue;
        delete mRedText0;
        delete mGreenText0;
        delete mBlueText0;
    }
};

#endif
