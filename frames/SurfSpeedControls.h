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
 * \file   SurfSpeedControls.h
 * \brief  Definition and implementation of mouse SurfSpeedControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SurfSpeedControls_h
#define __SurfSpeedControls_h

/**
 * \brief Definition and implementation of SurfSpeedControls class.
 *
 * Spped controls consist of a control box with a slider for mouse speed.
 *
 * Note: Callbacks are handled by the caller.
 */
class SurfSpeedControls {
    #define  speedSliderMin      1    ///< min speed slider value
    #define  speedSliderDefault  5    ///< default speed slider value
    #define  speedSliderMax      20   ///< max speed slider value
    bool               mHorizontal;   ///< orientation of slider
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mSpeedBox;
    wxStaticBoxSizer*  mSpeedSizer;
    wxFlexGridSizer*   mFgs;
    wxSlider           *mSpeed;       ///< speed slider
    wxStaticText       *mSpeedText;   ///< slider label
public:
    /** \brief SurfSpeedControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title         is the title for the control area/box.
     *  \param currentSpeed  is the current speed value.
     *  \param speedSliderID is the ID associated with the speed slider.
     */
    SurfSpeedControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int currentSpeed,
        const int speedSliderID, const bool horizontal=true )
    {
        assert( speedSliderMin<=currentSpeed && currentSpeed<=speedSliderMax );
        mHorizontal = horizontal;
        mBottomSizer = bottomSizer;
        mSpeedBox = new wxStaticBox( cp, -1, title );
		::setColor( mSpeedBox );
        mSpeedSizer = new wxStaticBoxSizer( mSpeedBox, wxHORIZONTAL );
        if (mHorizontal)
            mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        else
            mFgs = new wxFlexGridSizer( 6, 0, 5 );  //6 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        if (mHorizontal) {
            mFgs->AddGrowableCol( 0 );
        }

        //speed - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mSpeedText = new wxStaticText( cp, -1, "speed:" );
        ::setColor( mSpeedText );
        mFgs->Add( mSpeedText, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        if (mHorizontal)
            mSpeed = new wxSlider( cp, speedSliderID, currentSpeed,
                speedSliderMin, speedSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
				"speed" );
        else
            mSpeed = new wxSlider( cp, speedSliderID, currentSpeed,
                speedSliderMin, speedSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "speed" );
        ::setColor( mSpeed );
        mSpeed->SetPageSize( 5 );
        mFgs->Add( mSpeed, 0, wxGROW|wxLEFT|wxRIGHT, 0 );
        //- - - - -
        mSpeedSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mSpeed->SetToolTip(   "set mouse speed amount" );
            #endif
        #endif
        mBottomSizer->Prepend( mSpeedSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int getSpeed ( void ) {
        //values will be in [0 .. max]
        return mSpeed->GetValue();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief ColorControls dtor. */
    ~SurfSpeedControls ( void ) {
        mBottomSizer->Remove( mSpeedSizer );
        //delete mSpeedBox;
        //delete mColorSizer;
        //delete mFgs;
        //delete mFgsGreenSlider;
        //delete mFgsRedSlider;
        //delete mFgsBlueSlider;
        delete mSpeed;
        delete mSpeedText;
    }
};

#endif
