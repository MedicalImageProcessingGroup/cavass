/*
  Copyright 1993-2013 Medical Image Processing Group
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
 * \file   SetIndexControls.h
 * \brief  Definition and implementation of SetIndexControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#pragma once

//extern char*  button_zoomin15_xpm[];
//extern char*  button_zoomout15_xpm[];

/**
 * \brief Definition (and some implementations) of SetIndexControls class.
 *
 * Set index controls consist of a control box with sliders for slide
 * number and scale, a check box for turning on and off overlay, and
 * a button to match the slice number.
 *
 * Note: Callbacks are handled by the caller.
 */
class SetIndexControls {
//#define  SetIndexControlsZoom
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxStaticBox*      mSetIndexBox;
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxButton*         mMatchIndex;    ///< match index button
    wxStaticText*     mOverlayST;     ///< overlay checkbox label
    wxSlider*         mSliceNumber;   ///< slice number slider
    wxStaticText*     mSliceST;       ///< slice number slider label
    wxSlider*         mScale;         ///< scale slider
    wxStaticText*     mScaleText0;    ///< scale slider label
    wxStaticText*     mScaleText1;    ///< scale min value
    wxStaticText*     mScaleText2;    ///< scale current value
    wxStaticText*     mScaleText3;    ///< scale max value
    wxSizer*          mSetIndexSizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_scale_value;
	int               mSliceCount;    ///< number of slices
    double            mScaleZoom;
public:
    wxCheckBox*       mCB;            ///< overlay on/off checkbox

    SetIndexControls ( wxPanel* cp, wxSizer* bottomSizer, const char* title,
        int currentSlice,  int numberOfSlices, int sliceSliderID,
        int scaleSliderID, double currentScale, int overlayID,
        int matchIndexID );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) previous and next buttons might change
     *  the current slice, the following method will allow an update of
     *  the slice slider.
	 *  \param currenSlice must be in [0..slices-1].
     */
    void setSliceNo ( const int currentSlice ) {  
		assert( 0<=currentSlice && currentSlice<mSliceCount );
        if (mSliceNumber != nullptr)
			mSliceNumber->SetValue( currentSlice+1 );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current scale value. */
    void setScaleText ( const wxString& s ) {  mScaleText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SetIndexControls dtor. */
    ~SetIndexControls ( ) {
		mFgsSlider->Clear( true );
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
		
		//mFgs->Remove( mFgsSlider );
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
};
