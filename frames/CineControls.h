/*
  Copyright 1993-2011, 2019 Medical Image Processing Group
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
 * \file   CineControls.h
 * \brief  Definition and implementation of CineControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __CineControls_h
#define __CineControls_h

extern int  gTimerInterval;

/**
 * \brief Definition and implementation of CineControls class.
 *
 * Cine controls consist of a control box with a checkbox for either
 * forward only or forward-backward cine mode.  These are mutually
 * exclusive.  If neither is checked, then cine mode is currently off.
 * A slider is also available to control the delay between the display
 * of each frame.
 *
 * Note: Callbacks are handled by the caller.
 */
class CineControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*      mCineOptionsBox;
    wxSizer*          mCineOptionsSizer;
    wxFlexGridSizer*  mFgs;
    wxStaticText*     mSt0;    ///< forward checkbox text
    wxStaticText*     mSt1;    ///< forward/backward checkbox text
    wxStaticText*     mSt2;    ///< delay slider text
    wxStaticText*     mSt3;    ///< cache checkbox text
    wxTimer*          mTimer;
public:
    wxSlider*         mCineSlider;            ///< cine slider
    wxCheckBox*       mForwardBackwardCheck;  ///< forward-backward checkbox
    wxCheckBox*       mForwardCheck;          ///< forward checkbox
    wxCheckBox*       mCache;                 ///< cache slices (for fast cine)
    wxComboBox*       mDimension;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief CineControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param forwardID         is the ID of the forward checkbox.
     *  \param forwardBackwardID is the ID of the forward/backward checkbox.
     *  \param sliderID          is the ID of the delay slider.
     *  \param timer             is the cine timer.
     */
    CineControls ( wxPanel* cp, wxSizer* bottomSizer,
        const int forwardID, const int forwardBackwardID,
        const int sliderID, wxTimer* timer, const int cacheID=wxID_ANY,
        const int dimensionID=wxID_ANY, const int currentDimension=0 )
    {
        mCache = 0;
        mTimer = timer;
        mBottomSizer = bottomSizer;
        mCineOptionsBox = new wxStaticBox( cp, -1, "CineOptions" );
        ::setColor( mCineOptionsBox );
        mCineOptionsSizer = new wxStaticBoxSizer( mCineOptionsBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 8, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
        //row 1, col 1
        mForwardCheck = new wxCheckBox( cp, forwardID, "" );
        mForwardCheck->SetValue( 0 );
        mFgs->Add( mForwardCheck, 0, wxALIGN_LEFT );
        ::setColor( mForwardCheck );
        //row 1, col 2
        mSt0 = new wxStaticText( cp, -1, "forward (1 way motion)" );
        ::setColor( mSt0 );
        mFgs->Add( mSt0, 0, wxALIGN_LEFT );
        //row 2, col 1
        mForwardBackwardCheck = new wxCheckBox( cp, forwardBackwardID, "" );
        mForwardBackwardCheck->SetValue( 0 );
        mFgs->Add( mForwardBackwardCheck, 0, wxALIGN_LEFT );
        ::setColor( mForwardBackwardCheck );
        //row 2, col 2
        mSt1 = new wxStaticText( cp, -1, "forward/backward (2 way motion)" );
        ::setColor( mSt1 );
        mFgs->Add( mSt1, 0, wxALIGN_LEFT );
        //row 3, col 1
        mFgs->AddStretchSpacer();
        mFgs->AddStretchSpacer();
        mSt2 = new wxStaticText( cp, -1, "delay:" );
        ::setColor( mSt2 );
        mFgs->Add( mSt2, 0, wxALIGN_RIGHT );
        //row 3, col 2
        mCineSlider = new wxSlider( cp, sliderID, ::gTimerInterval,
            2, 2000, wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator, "delay" );
        ::setColor( mCineSlider );
        mCineSlider->SetPageSize( 5 );
        mCineSlider->SetValue( 120 );
        mFgs->Add( mCineSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        //row 4
        if (cacheID != wxID_ANY) {
            //row 4, col 1
            mCache= new wxCheckBox( cp, cacheID, "" );
            mCache->SetValue( 0 );
            mFgs->Add( mCache, 0, wxALIGN_LEFT );
            ::setColor( mCache );
            //row 4, col 2
            mSt3 = new wxStaticText( cp, -1, "cache" );
            ::setColor( mSt3 );
            mFgs->Add( mSt3, 0, wxALIGN_LEFT );
        }
        //row 5
        if (dimensionID != wxID_ANY) {
            wxArrayString sa;
            sa.Add("Index");
            sa.Add("x_3");
            sa.Add("x_4");
            sa.Add("Time stamp");
            mDimension = new wxComboBox( cp, dimensionID, sa[currentDimension],
                wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
                wxCB_READONLY );
            mFgs->Add( mDimension, 0, wxALIGN_LEFT );
            ::setColor( mDimension );
        }
        else
            mDimension = NULL;
        
        mCineOptionsSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mSt2->SetToolTip( _T("milliseconds") );
                mCineSlider->SetToolTip(  _T("set cine delay interval") );
            #endif
        #endif

        mBottomSizer->Prepend( mCineOptionsSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief CineControls dtor. */
    ~CineControls ( void ) {
        if (mTimer!=NULL)    mTimer->Stop();

#ifdef DETACH_CONTROLS_FROM_SIZERS
        if (mDimension)
        {
            mFgs->Detach( mDimension );
            delete mDimension;
        }
        mFgs->Detach( mCineSlider );
        delete mCineSlider;
        mFgs->Detach( mSt2 );
        delete mSt2;
        mFgs->Detach( mSt1 );
        delete mSt1;
        mFgs->Detach( mForwardBackwardCheck );
        delete mForwardBackwardCheck;
        mFgs->Detach( mForwardCheck );
        delete mForwardCheck;
        mFgs->Detach( mSt0 );
        delete mSt0;
        delete mCineOptionsBox;
        mCineOptionsBox = NULL;
#else
        mFgs->Clear( true );
        mCineOptionsSizer->Clear( true );
#endif
        mCineOptionsSizer->Remove( mFgs );
        mBottomSizer->Remove( mCineOptionsSizer );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void setDelay(int ms)
    {
        mCineSlider->SetValue( ms );
    }
};

#endif
