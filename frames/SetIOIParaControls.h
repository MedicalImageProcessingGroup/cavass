/*
  Copyright 1993-2015 Medical Image Processing Group
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
 * \file   SetIOIParaControls.h
 * \brief  Definition and implementation of SetIOIParaControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SetIOIParaControls_h
#define __SetIOIParaControls_h

/**
 * \brief Definition and implementation of SetIOIParaControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */
class SetIOIControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSetIndexBox;
    
	wxSlider*         mSS1;   ///< slice number slider
	wxSlider*         mES2;   ///< slice number slider
	wxSlider*         mSS3;   ///< slice number slider
	wxSlider*         mES4;   ///< slice number slider
	wxSlider*         mSS5;   ///< slice number slider
	wxSlider*         mES6;   ///< slice number slider
    wxStaticText*     mSt1;       ///< slice number slider label
	wxStaticText*     mSt2;       ///< slice number slider label
	wxStaticText*     mSt3;       ///< slice number slider label
	wxStaticText*     mSt4;       ///< slice number slider label
	wxStaticText*     mSt5;       ///< slice number slider label
	wxStaticText*     mSt6;       ///< slice number slider label

    wxSizer*          mSetIndexSizer;
    
public:
    /** \brief SetIOIControls ctor. */
    SetIOIControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,  const int currentStartSlice, const int numberOfSlices,
        const int StartsliceSliderID, const int currentEndSlice, 
        const int EndsliceSliderID  )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 2, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
      
		 //---Starting slice---
		 mSt3 = new wxStaticText( cp, -1, "Starting slice:" );
        ::setColor( mSt3 );
        mFgs->Add( mSt3, 0, wxALIGN_LEFT );
        mSS3 = new wxSlider( cp, StartsliceSliderID, currentStartSlice, 1, numberOfSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Sslice" );
        mSS3->SetPageSize( 10 );
        ::setColor( mSS3 );
        mFgs->Add( mSS3, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        //---------------------------
     
	   //---Ending slice---
        mSt4 = new wxStaticText( cp, -1, "Ending slice:" );
        ::setColor( mSt4 );
        mFgs->Add( mSt4, 0, wxALIGN_LEFT );
        mES4 = new wxSlider( cp, EndsliceSliderID, currentEndSlice, 1, numberOfSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Eslice" );
        mES4->SetPageSize( 10 );
        ::setColor( mES4 );
        mFgs->Add( mES4, 0, wxGROW|wxLEFT|wxRIGHT,3 );

		mSetIndexSizer->Add(mFgs, 0, wxGROW);
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }   
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
    /** \brief SetIOIControls dtor. */
    ~SetIOIControls ( void ) {
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
		
		//mFgs->Remove( mFgsSlider );
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
};


class SetSH0MIPControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSetIndexBox;
    
	wxSlider*         mSS1;   ///< slice number slider
	wxSlider*         mES2;   ///< slice number slider
	wxSlider*         mSS3;   ///< slice number slider
	wxSlider*         mES4;   ///< slice number slider
	wxSlider*         mSS5;   ///< slice number slider
	wxSlider*         mES6;   ///< slice number slider
    wxStaticText*     mSt1;       ///< slice number slider label
	wxStaticText*     mSt2;       ///< slice number slider label
	wxStaticText*     mSt3;       ///< slice number slider label
	wxStaticText*     mSt4;       ///< slice number slider label
	wxStaticText*     mSt5;       ///< slice number slider label
	wxStaticText*     mSt6;       ///< slice number slider label

    wxSizer*          mSetIndexSizer;
    
public:
    /** \brief SetIOIControls ctor. */
    SetSH0MIPControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,  const int currentMinThre, const int maxIntensity,
        const int MinThreSliderID, const int currentMaxThre, 
        const int MaxThreSliderID  )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 2, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
      
		 //---Starting slice---
		 mSt3 = new wxStaticText( cp, -1, "Min Threshold:" );
        ::setColor( mSt3 );
        mFgs->Add( mSt3, 0, wxALIGN_LEFT );
        mSS3 = new wxSlider( cp, MinThreSliderID, currentMinThre, 1, maxIntensity,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "MinThre" );
        mSS3->SetPageSize( 10 );
        ::setColor( mSS3 );
        mFgs->Add( mSS3, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        //---------------------------
     
	   //---Ending slice---
		mSt4 = new wxStaticText( cp, -1, "Max Threshold:" );
        ::setColor( mSt4 );
        mFgs->Add( mSt4, 0, wxALIGN_LEFT );
        mES4 = new wxSlider( cp, MaxThreSliderID, currentMaxThre, 1, maxIntensity,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "MaxThre" );
        mES4->SetPageSize( 10 );
        ::setColor( mES4 );
        mFgs->Add( mES4, 0, wxGROW|wxLEFT|wxRIGHT,3 );

		mSetIndexSizer->Add(mFgs, 0, wxGROW);
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }   
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
    /** \brief SetSH0MIPControls dtor. */
    ~SetSH0MIPControls ( void ) {
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
		
		//mFgs->Remove( mFgsSlider );
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
};

#endif
