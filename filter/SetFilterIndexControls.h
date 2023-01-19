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
 * \file   SetFilterIndexControls.h
 * \brief  Definition and implementation of SetFilterIndexControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SetFilterIndexControls_h
#define __SetFilterIndexControls_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

/**
 * \brief Definition and implementation of SetFilterIndexControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */
#include "FilterFrame.h"

class SetFilterIndexControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
//    wxButton*         mMatchIndex;    ///< match index button
    wxStaticText*     mOverlayST;     ///< overlay checkbox label
    wxStaticBox*      mSetIndexBox;
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
    double            mScaleZoom;
public:
    /** \brief SetFilterIndexControls ctor. */
    SetFilterIndexControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int currentSlice, const int numberOfSlices,
        const int sliceSliderID, const int scaleSliderID, const double currentScale,
		const int labelsID, const int zoomInID=-1, const int zoomOutID=-1 )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
        //slice number
		if (numberOfSlices > 1)
		{
        mSliceST = new wxStaticText( cp, -1, "slice:" );
        ::setColor( mSliceST );
        mFgs->Add( mSliceST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mSliceNumber = new wxSlider( cp, sliceSliderID, currentSlice, 1,
            numberOfSlices, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
            "slice" );
        ::setColor( mSliceNumber );
        mSliceNumber->SetPageSize( 5 );
        mFgs->Add( mSliceNumber, 0, wxGROW|wxLEFT|wxRIGHT );
		}
		else
		{
			mSliceST = NULL;
			mSliceNumber = NULL;
		}

        //scale
        mScaleText0 = new wxStaticText( cp, -1, "scale:" );
        ::setColor( mScaleText0 );
        mFgs->Add( mScaleText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        mZoomOut = new wxBitmapButton( cp, zoomOutID, wxBitmap( button_zoomout15_xpm ), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut );
        mFgsSlider->Add( mZoomOut, 0, wxALIGN_RIGHT );
        m_old_scale_value = 100;
        mScaleZoom = 1.0;
        mScale = new wxSlider( cp, scaleSliderID,
            (int)(currentScale*100+0.5), scaleIMin, scaleIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            //wxSL_HORIZONTAL|wxSL_AUTOTICKS, wxDefaultValidator,
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "scale" );
        ::setColor( mScale );
        mScale->SetPageSize( 5 );
        mFgsSlider->Add( mScale, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn = new wxBitmapButton( cp, zoomInID, wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn );
        mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
        
        wxString  s = wxString::Format( "%5.2f", scaleIMin/100.0 );
        mScaleText1 = new wxStaticText( cp, -1, s );
        ::setColor( mScaleText1 );
        mFgsSlider->Add( mScaleText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%5.2f", currentScale );
        mScaleText2 = new wxStaticText( cp, -1, s );
        ::setColor( mScaleText2 );
        mFgsSlider->Add( mScaleText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%5.2f", scaleIMax/100.0 );
        mScaleText3 = new wxStaticText( cp, -1, s );
        ::setColor( mScaleText3 );
        mFgsSlider->Add( mScaleText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mSetIndexSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        
        mFgs->AddSpacer( ButtonOffset );
        mFgs->AddSpacer( ButtonOffset );
        mOverlayST = new wxStaticText( cp, -1, "Labels:" );
        ::setColor( mOverlayST );
        mFgs->Add( mOverlayST, 0, wxALIGN_RIGHT );
        
        mCB = new wxCheckBox( cp, labelsID, "on/off" );
        ::setColor( mCB );
        mCB->SetValue( 1 );
		mFgs->Add( mCB /*, 0, wxALIGN_RIGHT*/);

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
	            mCB->SetToolTip(          "toggle image overlay on & off" );
				mOverlayST->SetToolTip(   "toggle image overlay on & off" );
                if (mSliceNumber)
					mSliceNumber->SetToolTip( "slice number" );
                mScale->SetToolTip(       "scale" );
            #endif
        #endif
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) previous and next buttons might change
     *  the current slice, the following method will allow an update of
     *  the slice slider.
     */
    void setSliceNo ( const int currentSlice ) {  
        if (mSliceNumber)
			mSliceNumber->SetValue( currentSlice );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current scale value. */
    void setScaleText ( const wxString s ) {  mScaleText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SetFilterIndexControls dtor. */
    ~SetFilterIndexControls ( void ) {
		mFgsSlider->Clear( true );
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
		
		//mFgs->Remove( mFgsSlider );
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
};

#endif
