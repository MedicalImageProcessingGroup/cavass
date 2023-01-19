/*
  Copyright 1993-2012, 2015, 2019 Medical Image Processing Group
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
 * \file   SurfOpacityControls.h
 * \brief  Definition and implementation of SurfOpacityControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SurfOpacityControls_h
#define __SurfOpacityControls_h

/**
 * \brief Definition and implementation of SurfOpacityControls class.
 *
 * Surface opacity controls consist of a control box with a slider for opacity.
 *
 * Note: Callbacks are handled by the caller.
 */
class SurfOpacityControls {
    #define  sliderMin   0       ///< min value for opacity slider value
    #define  sliderMax   100     ///< max value for opacity slider value
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mOpacityBox;
    wxStaticBoxSizer*  mOpacitySizer;
    wxFlexGridSizer*   mFgs;
	wxFlexGridSizer*   mFgsSlider;
    wxSlider          *mOpacity;        ///< opacity slider
    wxStaticText      *mOpacityText0;   ///< opacity slider label
    wxStaticText      *mOpacityText1;   ///< opacity min value
    wxStaticText      *mOpacityText2;   ///< opacity current value
    wxStaticText      *mOpacityText3;   ///< opacity max value
	int                m_old_opacity_value;
public:
    /** \brief SurfOpacityControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title         is the title for the control area/box.
     *  \param currentOpacitry    is the current opacity value.
     *  \param opacitySliderID   is the ID associated with the opacity slider.
     */
    SurfOpacityControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const double currentOpacity,
        const int opacitySliderID )
    {
        assert( 0.0<=currentOpacity   && currentOpacity  <=1.0 );
        mBottomSizer = bottomSizer;
        mOpacityBox = new wxStaticBox( cp, -1, title );
        ::setColor( mOpacityBox );
        mOpacitySizer = new wxStaticBoxSizer( mOpacityBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //opacity- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mOpacityText0 = new wxStaticText( cp, -1, "opacity:" );
        ::setColor( mOpacityText0 );
        mFgs->Add( mOpacityText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );

        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
		mFgsSlider->AddGrowableCol( 0 );
		mFgsSlider->AddSpacer( -1 );
		m_old_opacity_value = (int)(currentOpacity*sliderMax);
        mOpacity = new wxSlider( cp, opacitySliderID,
            (int)(currentOpacity*sliderMax),
            sliderMin, sliderMax, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, "opacity" );
        ::setColor( mOpacity );
        mOpacity->SetPageSize( 10 );
        mFgsSlider->Add( mOpacity, 0, wxGROW|wxLEFT|wxRIGHT );
		mFgsSlider->AddSpacer( -1 );

		wxString  s = "0";
		mOpacityText1 = new wxStaticText( cp, -1, s );
		::setColor( mOpacityText1 );
		mFgsSlider->Add( mOpacityText1, 0, wxALIGN_RIGHT );

		s = wxString::Format( "%5.1f", currentOpacity );
		mOpacityText2 = new wxStaticText( cp, -1, s );
		::setColor( mOpacityText2 );
		mFgsSlider->Add( mOpacityText2, 0, wxALIGN_CENTER );

		s = "100";
		mOpacityText3 = new wxStaticText( cp, -1, s );
		::setColor( mOpacityText3 );
		mFgsSlider->Add( mOpacityText3, 0, wxALIGN_LEFT );

		mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );

        mOpacitySizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mOpacity->SetToolTip( "set opacity amount" );
            #endif
        #endif
        mBottomSizer->Prepend( mOpacitySizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief values will be in [0.0 .. 1.0]. */
    double getOpacity ( void ) {
		mOpacityText2->SetLabel( wxString::Format( "%5.1f",
		       100.0 * mOpacity->GetValue() / sliderMax ) );
        return (double)mOpacity->GetValue() / sliderMax;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SurfOpacityControls dtor. */
    ~SurfOpacityControls ( void ) {
        mBottomSizer->Remove( mOpacitySizer );
        delete mOpacity;
        delete mOpacityText0;
        delete mOpacityText1;
        delete mOpacityText2;
        delete mOpacityText3;
    }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  VolumeRenderingControls {
	wxSizer          *mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mVolumeRenderingBox;
    wxSizer*          mVolumeRenderingSizer;
    wxFlexGridSizer*  mFgsSlider;
    wxComboBox       *mMaterial;
	wxStaticText     *mMaterialST;
	wxButton         *mOpacity;
	wxCheckBox       *mMIP;
	wxButton         *mColor;
	wxButton         *mSurfStrength;
    wxSlider         *mOpacityValue;   ///< opacity value slider
    wxStaticText     *mOpacityValueST; ///< opacity value slider label
    wxSlider         *mThreshold1;   ///< threshold 1 slider
    wxStaticText     *mThreshold1ST; ///< threshold 1 slider label
    wxSlider         *mThreshold2;   ///< threshold 2 slider
    wxStaticText     *mThreshold2ST; ///< threshold 2 slider label
    wxSlider         *mRed;   ///< red slider
    wxStaticText     *mRedST; ///< red slider label
    wxSlider         *mGreen;   ///< green slider
    wxStaticText     *mGreenST; ///< green slider label
    wxSlider         *mBlue;   ///< blue slider
    wxStaticText     *mBlueST; ///< blue slider label
    wxSlider         *mSurfStren;   ///< red slider
    wxStaticText     *mSurfStrenST; ///< red slider label
    wxSlider         *mEmissionPower;   ///< emission power slider
    wxStaticText     *mEmissionPowerST; ///< emission power slider label
    wxSlider         *mSurfPctPower;   ///< surface percent power slider
    wxStaticText     *mSurfPctPowerST; ///< surface percent power slider label
	wxFlexGridSizer  *mFgsRow1;
	wxFlexGridSizer  *mFgsRow2;
	bool              mDirect;
	double            mMinThreshold, mMaxThreshold;
	wxPanel          *mCp;
	wxCheckBox       *mInvert;
public:

    /** \brief VolumeRenderingControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param currentMaterial is current material minus 1 (0 for surface).
	 *  \param direct is whether input is scene.
	 *  \param minThreshold is minimum threshold value.
	 *  \param maxThreshold is maximum threshold value.
     */
    VolumeRenderingControls ( wxPanel* cp, wxSizer* bottomSizer,
		int currentMaterial, bool direct=false,
		double minThreshold=-1, double maxThreshold=-1, bool MIP_flag=false,
		bool invert_flag=false )
    {
		mOpacityValue = NULL;
		mThreshold1 = NULL;
		mRed = NULL;
		mSurfStren = NULL;
		mInvert = NULL;
		mCp = cp;
        mBottomSizer = bottomSizer;
		mDirect = direct;
		mMinThreshold = minThreshold;
		mMaxThreshold = maxThreshold;
        mVolumeRenderingBox = new wxStaticBox( cp, -1, "VolumeRendering Controls" );
        ::setColor( mVolumeRenderingBox );
        mVolumeRenderingSizer = new wxStaticBoxSizer( mVolumeRenderingBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
		mMaterialST = new wxStaticText( cp, -1, "Material:" );
		::setColor( mMaterialST );
		mFgs->Add( mMaterialST, 0, wxALIGN_RIGHT );
		mFgsRow1 = new wxFlexGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
		mFgs->Add( mFgsRow1 );

		wxArrayString materialOptions;
		materialOptions.Add("Surf");
		materialOptions.Add("2");
		materialOptions.Add("3");
		materialOptions.Add("4");
        mMaterial = new wxComboBox( cp, SurfViewFrame::ID_MATERIAL,
			materialOptions[currentMaterial], wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight), materialOptions, wxCB_READONLY );
        ::setColor( mMaterial );
        mFgsRow1->Add( mMaterial, 0, wxALIGN_LEFT );

		mOpacity = new wxButton( cp, SurfViewFrame::ID_MATERL_OPACITY,
			"Opacity", wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight) );
		::setColor( mOpacity );
		mFgsRow1->Add( mOpacity, 0, wxALIGN_RIGHT );

        mFgsRow2 = new wxFlexGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mFgs->AddSpacer( ButtonOffset );
        mFgs->AddSpacer( ButtonOffset );

		mMIP = new wxCheckBox( cp, SurfViewFrame::ID_MIP, "MIP",
		    wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( mMIP );
		mMIP->SetValue( MIP_flag );
		mFgs->Add( mMIP, 0, wxALIGN_RIGHT );

		mColor = new wxButton( cp, SurfViewFrame::ID_MATERL_COLOR,
			"Color", wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight) );
		::setColor( mColor );
		mFgsRow2->Add( mColor, 0, wxALIGN_RIGHT );

		mSurfStrength = new wxButton( cp, SurfViewFrame::ID_SURF_STRENGTH,
			"Surf.Strenth", wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight) );
		::setColor( mSurfStrength );
		mFgsRow2->Add( mSurfStrength, 0, wxALIGN_RIGHT );

        mFgs->Add( mFgsRow2 );

        mFgs->AddSpacer( ButtonOffset );
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );

		if (MIP_flag)
			displayInvert(invert_flag);

        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        mVolumeRenderingSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Prepend( mVolumeRenderingSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void removeMaterialSliders ( void )
	{
		if (mInvert)
		{
			mFgs->Detach(mInvert);
			delete mInvert;
			mInvert = NULL;
		}
		if (mThreshold1)
		{
			mFgs->Detach(mThreshold2);
			delete mThreshold2;
			mFgs->Detach(mThreshold2ST);
			delete mThreshold2ST;
			mFgs->Detach(mThreshold1);
			delete mThreshold1;
			mFgs->Detach(mThreshold1ST);
			delete mThreshold1ST;
			mThreshold1 = NULL;
		}
	    if (mOpacityValue)
	    {
		    mFgs->Detach(mOpacityValue);
			delete mOpacityValue;
			mFgs->Detach(mOpacityValueST);
			delete mOpacityValueST;
			mOpacityValue = NULL;
	    }
		if (mRed)
		{
			mFgs->Detach(mBlue);
			delete mBlue;
			mFgs->Detach(mBlueST);
			delete mBlueST;
			mFgs->Detach(mGreen);
			delete mGreen;
			mFgs->Detach(mGreenST);
			delete mGreenST;
			mFgs->Detach(mRed);
			delete mRed;
			mFgs->Detach(mRedST);
			delete mRedST;
			mRed = NULL;
		}
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void removeSliders ( void )
	{
		removeMaterialSliders();
		if (mSurfStren)
		{
			mFgs->Detach(mSurfPctPower);
			delete mSurfPctPower;
			mFgs->Detach(mSurfPctPowerST);
			delete mSurfPctPowerST;
			mFgs->Detach(mEmissionPower);
			delete mEmissionPower;
			mFgs->Detach(mEmissionPowerST);
			delete mEmissionPowerST;
			mFgs->Detach(mSurfStren);
			delete mSurfStren;
			mFgs->Detach(mSurfStrenST);
			delete mSurfStrenST;
			mSurfStren = NULL;
		}
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void displayInvert( bool invert_flag, bool layout=false )
	{
		removeSliders();
		mInvert = new wxCheckBox( mCp, SurfViewFrame::ID_MIP_INVERT, "Invert",
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
		::setColor( mInvert );
		mInvert->SetValue( invert_flag );
		mFgs->Add( mInvert, 0, wxALIGN_RIGHT );
		if (layout)
			mBottomSizer->Layout();
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	/** \brief display opacity slider(s).
     *  \param currentOpacity is current opacity.
     *  \param currentThreshold1 is lower endpoint of lower ramp of opacity
	 *         trapezoid for current material.
     *  \param currentThreshold2 is upper endpoint of lower ramp of opacity
	 *         trapezoid for current material.
	 */
    void displayOpacitySliders ( double currentOpacity, double currentThreshold1=-1, double currentThreshold2=-1 )
	{
		removeSliders();

        //opacity value
        mOpacityValueST = new wxStaticText( mCp, -1,
			wxString::Format( "Opacity: %.1f", currentOpacity*100 )+"%" );
        ::setColor( mOpacityValueST );
        mFgs->Add( mOpacityValueST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mOpacityValue = new wxSlider( mCp,
			SurfViewFrame::ID_MATERL_OPACITY_SLIDER,
			(int)(currentOpacity*1000), 0, 1000, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Opacity" );
        ::setColor( mOpacityValue );
        mOpacityValue->SetPageSize( 5 );
        mFgs->Add( mOpacityValue, 0, wxGROW|wxLEFT|wxRIGHT );

		if (mDirect)
		{
	        // Threshold 1
	        mThreshold1ST = new wxStaticText( mCp, -1, "Threshold 1:" );
	        ::setColor( mThreshold1ST );
	        mFgs->Add(mThreshold1ST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
	        mThreshold1 = new wxSlider( mCp,
				SurfViewFrame::ID_MATERL_THRESHOLD1_SLIDER,
				(int)currentThreshold1, (int)mMinThreshold, (int)mMaxThreshold,
				wxDefaultPosition, wxSize(sliderWidth, -1),
				wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
				"Threshold 1" );
	        ::setColor( mThreshold1 );
	        mThreshold1->SetPageSize( 5 );
	        mFgs->Add( mThreshold1, 0, wxGROW|wxLEFT|wxRIGHT );
	
	        // Threshold 2
	        mThreshold2ST = new wxStaticText( mCp, -1, "Threshold 2:" );
	        ::setColor( mThreshold2ST );
	        mFgs->Add(mThreshold2ST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
	        mThreshold2 = new wxSlider( mCp,
				SurfViewFrame::ID_MATERL_THRESHOLD2_SLIDER,
				(int)currentThreshold2, (int)mMinThreshold, (int)mMaxThreshold,
				wxDefaultPosition, wxSize(sliderWidth, -1),
				wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
				"Threshold 2" );
	        ::setColor( mThreshold2 );
	        mThreshold2->SetPageSize( 5 );
	        mFgs->Add( mThreshold2, 0, wxGROW|wxLEFT|wxRIGHT );
		}

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                if (mOpacityValue)
					mOpacityValue->SetToolTip( "material opacity" );
				if (mDirect)
				{
					mThreshold1->SetToolTip( "lower end of lower ramp" );
					mThreshold2->SetToolTip( "upper end of lower ramp" );
				}
            #endif
        #endif

		mBottomSizer->Layout();
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	/** \brief display material color sliders.
     *  \param currentRed is the current red value for current material.
     *  \param currentGreen is the current green value for current material.
     *  \param currentBlue is the current blue value for current material.
	 */
    void displayColorSliders(double currentRed, double currentGreen,
			double currentBlue)
	{
		removeSliders();

        //red value
        mRedST = new wxStaticText( mCp, -1, wxString::Format("Red: %.0f",
			currentRed*100 )+"%" );
        ::setColor( mRedST );
        mFgs->Add( mRedST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mRed = new wxSlider( mCp,
			SurfViewFrame::ID_MATERL_RED_SLIDER,
			(int)(currentRed*100), 0, 100, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Red" );
        ::setColor( mRed );
        mRed->SetPageSize( 5 );
        mFgs->Add( mRed, 0, wxGROW|wxLEFT|wxRIGHT );

        //green value
        mGreenST = new wxStaticText( mCp, -1, wxString::Format("Green: %.0f",
			currentGreen*100 )+"%" );
        ::setColor( mGreenST );
        mFgs->Add( mGreenST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mGreen = new wxSlider( mCp,
			SurfViewFrame::ID_MATERL_GREEN_SLIDER,
			(int)(currentGreen*100), 0, 100, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Green" );
        ::setColor( mGreen );
        mGreen->SetPageSize( 5 );
        mFgs->Add( mGreen, 0, wxGROW|wxLEFT|wxRIGHT );

        //blue value
        mBlueST = new wxStaticText( mCp, -1, wxString::Format("Blue: %.0f",
			currentBlue*100 )+"%" );
        ::setColor( mBlueST );
        mFgs->Add( mBlueST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mBlue = new wxSlider( mCp,
			SurfViewFrame::ID_MATERL_BLUE_SLIDER,
			(int)(currentBlue*100), 0, 100, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Blue" );
        ::setColor( mBlue );
        mBlue->SetPageSize( 5 );
        mFgs->Add( mBlue, 0, wxGROW|wxLEFT|wxRIGHT );

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
				mRed->SetToolTip( "material red component" );
				mGreen->SetToolTip( "material green component" );
				mBlue->SetToolTip( "material blue component" );
            #endif
        #endif

		mBottomSizer->Layout();
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	/** \brief display surface strength sliders.
     *  \param currentSurfStren is the current surface strength.
     *  \param currentEmisPower is the current emission exponent.
     *  \param currentSurfPctPower is the current surface percentage exponent.
	 */
    void displaySurfStrenSliders(double currentSurfStren,
			double currentEmisPower, double currentSurfPctPower)
	{
		removeSliders();

        //surface strength value
        mSurfStrenST = new wxStaticText( mCp, -1,
			wxString::Format("Surf.Stren: %.0f", currentSurfStren )+"%" );
        ::setColor( mSurfStrenST );
        mFgs->Add( mSurfStrenST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mSurfStren = new wxSlider( mCp,
			SurfViewFrame::ID_SURF_STREN_SLIDER,
			(int)(currentSurfStren), 0, 100, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Surf.Strength" );
        ::setColor( mSurfStren );
        mSurfStren->SetPageSize( 5 );
        mFgs->Add( mSurfStren, 0, wxGROW|wxLEFT|wxRIGHT );

        //emission exponent value
        mEmissionPowerST = new wxStaticText( mCp, -1,
			wxString::Format("Emis.Pwr: %.2f", currentEmisPower ) );
        ::setColor( mEmissionPowerST );
        mFgs->Add( mEmissionPowerST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
        mEmissionPower = new wxSlider( mCp,
			SurfViewFrame::ID_EMISSION_POWER_SLIDER,
			(int)(currentEmisPower*100), 100, 200, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Emis.Power" );
        ::setColor( mEmissionPower );
        mEmissionPower->SetPageSize( 5 );
        mFgs->Add( mEmissionPower, 0, wxGROW|wxLEFT|wxRIGHT );

        //surface percentage exponent
        mSurfPctPowerST = new wxStaticText( mCp, -1,
			wxString::Format("Srf.Pct.Pwr: %.2f", currentSurfPctPower ) );
        ::setColor( mSurfPctPowerST );
        mFgs->Add( mSurfPctPowerST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
        mSurfPctPower = new wxSlider( mCp,
			SurfViewFrame::ID_SURF_PCT_POWER_SLIDER,
			(int)(currentSurfPctPower*100), 0, 100, wxDefaultPosition,
			wxSize(sliderWidth, -1), wxSL_HORIZONTAL,
			wxDefaultValidator, "Surf.Pct.Power" );
        ::setColor( mSurfPctPower );
        mSurfPctPower->SetPageSize( 5 );
        mFgs->Add( mSurfPctPower, 0, wxGROW|wxLEFT|wxRIGHT );

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
				mSurfStren->SetToolTip( "surface strength" );
				mEmissionPower->SetToolTip( "emission exponent" );
				mSurfPctPower->SetToolTip( "surface percentage exponent" );
            #endif
        #endif

		mBottomSizer->Layout();
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetOpacityValue ( void ) {
		double newOpacity = .001*mOpacityValue->GetValue();
		assert(newOpacity>=0 && newOpacity<=1000);
		mOpacityValueST->SetLabel( wxString::Format( "Opacity: %.1f",
			newOpacity*100 )+"%" );
		return newOpacity;
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int GetThreshold1 ( void ) {
		return mThreshold1->GetValue();
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int GetThreshold2 ( void ) {
		return mThreshold2->GetValue();
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetRedValue ( void ) {
		double newRed = .01*mRed->GetValue();
		mRedST->SetLabel( wxString::Format( "Red: %.0f", newRed*100 )+"%" );
		return newRed;
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetGreenValue ( void ) {
		double newGreen = .01*mGreen->GetValue();
		mGreenST->SetLabel(wxString::Format("Green: %.0f", newGreen*100)+"%");
		return newGreen;
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetBlueValue ( void ) {
		double newBlue = .01*mBlue->GetValue();
		mBlueST->SetLabel( wxString::Format( "Blue: %.0f", newBlue*100 )+"%" );
		return newBlue;
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetSurfStrength ( void ) {
		double newSurfStren = mSurfStren->GetValue();
		mSurfStrenST->SetLabel(
			wxString::Format( "Surf.Stren: %.0f", newSurfStren )+"%" );
		return newSurfStren;
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetEmissionPower ( void ) {
		double newEmissionPower = .01*mEmissionPower->GetValue();
		mEmissionPowerST->SetLabel(
			wxString::Format("Emis.Pwr: %.2f", newEmissionPower ) );
		return newEmissionPower;
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    double GetSurfPctPower ( void ) {
		double newSurfPctPower = .01*mSurfPctPower->GetValue();
		mSurfPctPowerST->SetLabel(
			wxString::Format("Srf.Pct.Pwr: %.2f", newSurfPctPower ) );
		return newSurfPctPower;
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief VolumeRenderingControls dtor. */
    ~VolumeRenderingControls ( void ) {
		removeSliders();
		mFgsRow2->Detach( mColor );
		delete mColor;
		mFgsRow2->Detach( mSurfStrength );
		delete mSurfStrength;
		mFgsRow1->Detach( mOpacity );
		delete mOpacity;
		mFgsRow2->Detach( mMaterial );
		delete mMaterial;
		mFgs->Detach( mMaterialST );
		delete mMaterialST;
		mFgs->Remove( mFgsSlider );
		mVolumeRenderingSizer->Remove( mFgs );
		mBottomSizer->Remove( mVolumeRenderingSizer );
    }
};

#endif
