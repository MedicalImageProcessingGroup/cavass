/*
  Copyright 1993-2011 Medical Image Processing Group
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
 * \file   InstancesControls.h
 * \brief  Definition and implementation of InstancesControls class.
 * \author Xinjian, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __InstancesControls_h
#define __InstancesControls_h

/**
 * \brief Definition and implementation of InstancesControls class.
 *
 * Instances controls consist of a control box with sliders for Instance, green, and
 * blue.
 *
 * Note: Callbacks are handled by the caller.
 */
#define  InstancesSliderMin   2       ///< min for (rgb) Instances slider values
#define  InstancesSliderMax   200     ///< max for (rgb) Instances slider values
    

class InstancesControls {
    bool               mHorizontal;   ///< orientation of sliders
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mInstancesBox;
    wxStaticBoxSizer*  mInstancesSizer;
    wxFlexGridSizer*   mFgs;
    wxSlider           *mInstance;
    wxStaticText       *mInstanceText0;
public:
    /** \brief InstancesControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title         is the title for the control area/box.
     *  \param currentInstance    is the current Instance value.
     *  \param currentGreen  is the current green value.
     *  \param currentBlue   is the current blue value.
     *  \param InstanceSliderID   is the ID associated with the Instance slider.
     *  \param greenSliderID is the ID associated with the green slider.
     *  \param blueSliderID  is the ID associated with the blue slider.
     */
    InstancesControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,
        const int currentInstance, 
        const int InstanceSliderID,
        const bool horizontal=true )
    {   
        mHorizontal = horizontal;
        mBottomSizer = bottomSizer;
        mInstancesBox = new wxStaticBox( cp, -1, title );
        ::setColor( mInstancesBox );
        mInstancesSizer = new wxStaticBoxSizer( mInstancesBox, wxHORIZONTAL );
        if (mHorizontal)
            mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        else
            mFgs = new wxFlexGridSizer( 6, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        if (mHorizontal) 
		{
            mFgs->AddGrowableCol( 0 );          
        }

        //Instance- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mInstanceText0 = new wxStaticText( cp, -1, "Instances:" );
        ::setColor( mInstanceText0 );
        mFgs->Add( mInstanceText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        if (mHorizontal)
            mInstance = new wxSlider( cp, InstanceSliderID, currentInstance,
                InstancesSliderMin, InstancesSliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator, "Instances" );
        else
            mInstance = new wxSlider( cp, InstanceSliderID, currentInstance,
                InstancesSliderMin, InstancesSliderMax, wxDefaultPosition, wxSize(-1, -1),
                wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "Instances" );
        ::setColor( mInstance );
        mInstance->SetPageSize( 5 );
        mFgs->Add( mInstance, 0, wxGROW|wxLEFT|wxRIGHT );        
        
        mInstancesSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );        
        mBottomSizer->Prepend( mInstancesSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
   
   
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief InstancesControls dtor. */
    ~InstancesControls ( void ) 
	{
        mBottomSizer->Remove( mInstancesSizer );
     //   delete mInstancesBox;
        //delete mInstancesSizer;
        //delete mFgs;
        //delete mFgsGreenSlider;
        //delete mFgsInstanceSlider;
        //delete mFgsBlueSlider; 
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
		if( mInstanceText0 != NULL )
		{
			delete mInstanceText0;        
			mInstanceText0 = NULL;
		}        
    }
};

#endif
