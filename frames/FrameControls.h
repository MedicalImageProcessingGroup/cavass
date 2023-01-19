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
 * \file   FrameControls.h
 * \brief  Definition and implementation of FrameControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FrameControls_h
#define __FrameControls_h

/**
 * \brief Definition and implementation of FrameControls class.
 *
 * Frame controls consist of a control box with a slider for the frame
 * number.
 *
 * Note: Callbacks are handled by the caller.
 */
class FrameControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxStaticBox*      mFrameBox;
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxSlider*         mFrameNumber;   ///< frame number slider
    wxStaticText*     mFrameST;       ///< frame number slider label
    wxSizer*          mFrameSizer;
	int               mFrameCount;    ///< number of frames
public:
    /** \brief FrameControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title          is the title for the control area/box.
     *  \param currentFrame   is the current frame number.
     *  \param numberOfFrames is the number of frames.
     *  \param frameSliderID  is the ID of the frame slider.
     */
    FrameControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const int currentFrame,  const int numberOfFrames,
		const int frameSliderID )
    {
		mFrameCount  = numberOfFrames;
        mBottomSizer = bottomSizer;
        mFrameBox = new wxStaticBox( cp, -1, title );
        ::setColor( mFrameBox );
        mFrameSizer = new wxStaticBoxSizer( mFrameBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //frame number
        mFrameST = new wxStaticText( cp, -1, "frame:" );
        ::setColor( mFrameST );
        mFgs->Add( mFrameST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mFrameNumber = new wxSlider( cp, frameSliderID, currentFrame+1, 1,
            numberOfFrames, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
            "frame" );
        ::setColor( mFrameNumber );
        mFrameNumber->SetPageSize( 5 );
        mFgs->Add( mFrameNumber, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mFrameSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mFrameNumber->SetToolTip( "frame number" );
            #endif
        #endif
        mBottomSizer->Prepend( mFrameSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) previous and next buttons might change
     *  the current frame, the following method will allow an update of
     *  the frame slider.
	 *  \param currenFrame must be in [0..frames-1] (but appears to the user as [1..frames]).
     */
    void setFrameNo ( const int currentFrame ) {  
		assert( 0<=currentFrame && currentFrame<mFrameCount );
        mFrameNumber->SetValue( currentFrame+1 );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief FrameControls dtor. */
    ~FrameControls ( void ) {
		mFgs->Clear( true );
		mFrameSizer->Clear( true );
		
		mFrameSizer->Remove( mFgs );
		mBottomSizer->Remove( mFrameSizer );
    }
};

#endif
