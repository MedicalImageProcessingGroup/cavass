/*
  Copyright 1993-2013, 2015 Medical Image Processing Group
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
 * \file   GrayMapControls.h
 * \brief  Definition and implementation of GrayMapControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __GrayMapControls_h
#define __GrayMapControls_h

/**
 * \brief Definition and implementation of GrayMapControls class.
 *
 * Gray map controls consist of a control box with window level and width
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class GrayMapControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    wxCheckBox*       mCb;        ///< invert the gray map checkbox
    wxSlider*         mLevel;    ///< window level slider
    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
    wxFlexGridSizer*  mContrastSizer;
    wxStaticText*     mSt0;       ///< level slider title
    wxStaticText*     mSt1;       ///< width slider title
    wxStaticText*     mSt2;       ///< invert checkbox title
    wxSlider*         mWidth;     ///< window width slider
	wxButton*         mLung;      ///< CT lung button
	wxButton*         mSoftTissue;///< CT soft tissue button
	wxButton*         mBone;      ///< CT bone button
	wxButton*         mPET;       ///< PET button
public:
    /** \brief GrayMapControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title          is the desired box title.
     *  \param currentLevel  is the current level value.
     *  \param currentWidth   is the current width  value.
     *  \param max            is the max slider value.
     *  \param currentInvert  is the current invert setting for the LUT.
     *  \param levelSliderID is the ID assigned to the level slider.
     *  \param widthSliderID  is the ID assigned to the width  slider.
     *  \param invertID       is the ID assigned to the invert check box.
	 *  \param lungID         is the ID assigned to the lung button
	 *  \param softTissueID   is the ID assigned to the soft tissue button
	 *  \param boneID         is the ID assigned to the bone button
     *  \param min            is the min slider value (optional)
     */
    GrayMapControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,
        const int currentLevel, const int currentWidth,
        const int max, const bool currentInvert,
        const int levelSliderID, const int widthSliderID, const int invertID,
		const int lungID, const int softTissueID, const int boneID,
        const int PETID, int min=0 )
    {
   //     if (min>0)    min = 0;    // modified by xinjian 2008.8.15

        mBottomSizer = bottomSizer;
        mContrastBox = new wxStaticBox( cp, -1, title );
        ::setColor( mContrastBox );
        mContrastBoxSizer = new wxStaticBoxSizer( mContrastBox, wxHORIZONTAL );
        mContrastSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mContrastSizer->SetMinSize( controlsWidth, 0 );
        mContrastSizer->AddGrowableCol( 0 );

        //level
        mSt0 = new wxStaticText( cp, -1, "Level:" );
        ::setColor( mSt0 );
        mContrastSizer->Add( mSt0, 0, wxALIGN_RIGHT );
        mLevel = new wxSlider( cp, levelSliderID, currentLevel, min, max,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Level" );
        ::setColor( mLevel );
        mLevel->SetPageSize( 10 );
        mContrastSizer->Add( mLevel, 0, wxGROW|wxLEFT|wxRIGHT );

        //width
        mSt1 = new wxStaticText( cp, -1, "Width:" );
        ::setColor( mSt1 );
        mContrastSizer->Add( mSt1, 0, wxALIGN_RIGHT );
		int  maxWidth = max - min;
		if (maxWidth<256)    maxWidth = 256;
        mWidth = new wxSlider( cp, widthSliderID, currentWidth, 1, maxWidth,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Width" );
        mWidth->SetPageSize( 10 );
        ::setColor( mWidth );
        mContrastSizer->Add( mWidth, 0, wxGROW|wxLEFT|wxRIGHT );
		// - - - - - - - - - -
        if (invertID != wxID_ANY) {
            mSt2 = new wxStaticText( cp, -1, "Invert:" );
            ::setColor( mSt2 );
            mContrastSizer->Add( mSt2, 0, wxRIGHT );
        
            mCb = new wxCheckBox( cp, invertID, "On/Off" );
            ::setColor( mCb );
            if (currentInvert)    mCb->SetValue( 1 );
            else                  mCb->SetValue( 0 );
            mContrastSizer->Add( mCb, 0, wxALIGN_RIGHT, 2*ButtonOffset );
        } else {
            mSt2 = NULL;
            mCb  = NULL;
        }
		// - - - - - - - - - -
		if (lungID != wxID_ANY)
		{
			mLung = new wxButton( cp, lungID, "CT Lung", wxDefaultPosition,
				wxSize(buttonWidth,buttonHeight) );
        	::setColor( mLung );
			mContrastSizer->Add( mLung, 0, wxRIGHT );
		}
		else
			mLung = NULL;
		if (softTissueID != wxID_ANY)
		{
			mSoftTissue = new wxButton( cp, softTissueID, "CT Soft Tissue",
				wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
			::setColor( mSoftTissue );
			mContrastSizer->Add( mSoftTissue, 0, wxRIGHT );
		}
		else
			mSoftTissue = NULL;
		if (boneID != wxID_ANY)
		{
			mBone = new wxButton( cp, boneID, "CT Bone", wxDefaultPosition,
				wxSize(buttonWidth,buttonHeight) );
			::setColor( mBone );
			mContrastSizer->Add( mBone, 0, wxRIGHT );
		}
		else
			mBone  = NULL;
		if (PETID != wxID_ANY)
		{
			mPET = new wxButton( cp, PETID, "PET", wxDefaultPosition,
				wxSize(buttonWidth,buttonHeight) );
			::setColor( mPET );
			mContrastSizer->Add( mPET, 0, wxRIGHT );
		}
		else
			mPET = NULL;

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                if (mCb!=NULL)    mCb->SetToolTip(  "invert values" );
                if (mSt2!=NULL)   mSt2->SetToolTip( "invert values" );
                mLevel->SetToolTip( "level" );
                mWidth->SetToolTip(  "width"  );
            #endif
        #endif
        
        mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void update_sliders(int newLevel, int newWidth, bool invert=false)
	{
		mLevel->SetValue(newLevel);
		mWidth->SetValue(newWidth);
		if (mCb)
			mCb->SetValue( invert );
	}
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief GrayMapControls dtor. */
    ~GrayMapControls ( void ) {
        mContrastSizer->Clear( true );
        mContrastBoxSizer->Clear( true );
        mContrastBoxSizer->Remove( mContrastSizer );
        mBottomSizer->Remove( mContrastBoxSizer );
    }
};

#endif

