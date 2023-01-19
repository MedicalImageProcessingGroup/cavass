/*
  Copyright 1993-2014 Medical Image Processing Group
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
 * \file   FunctionControls.h
 * \brief  Definition and implementation of FunctionControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2003, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FunctionControls_h
#define __FunctionControls_h

/**
 * \brief Definition and implementation of FunctionControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */

class FunctionControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    
    wxSlider*         mWeight;    ///< window weight slider
    wxSlider*         mLevel;     ///< window level slider
    wxSlider*         mWidth;     ///< window width slider
    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
    wxFlexGridSizer*  mContrastSizer;
    wxStaticText*     mSt0;       ///< weight slider title
    wxStaticText*     mSt1;       ///< level slider title
    wxStaticText*     mSt2;       ///< width slider title
	wxStaticText*     mSt3;       ///< type slider title
	wxComboBox       *mObjType;
        
public:
    /** \brief FunctionControls ctor. */
    FunctionControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const int currentWeight, const int currentLevel, const int currentWidth,
        const int max, const int weightSliderID, const int levelSliderID,
        const int widthSliderID, const int minWeight=-100,
		const int objTypeID=0, const int currentObjType=0)
    {
        mBottomSizer = bottomSizer;
        mContrastBox = new wxStaticBox( cp, -1, title );
        ::setColor( mContrastBox );
        mContrastBoxSizer = new wxStaticBoxSizer( mContrastBox, wxHORIZONTAL );
        mContrastSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mContrastSizer->SetMinSize( controlsWidth/2, 0 );
        mContrastSizer->AddGrowableCol( 0 );

        //weight
        mSt0 = new wxStaticText( cp, -1, "Weight(%):" );
        ::setColor( mSt0 );
        mContrastSizer->Add( mSt0, 0, wxALIGN_RIGHT );
        mWeight = new wxSlider( cp, weightSliderID, currentWeight, minWeight, 100,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Weight" );
        ::setColor( mWeight );
        mWeight->SetPageSize(10);
        mContrastSizer->Add( mWeight, 0, wxGROW|wxLEFT|wxRIGHT );

        //level
		if (levelSliderID)
		{
	        mSt1 = new wxStaticText( cp, -1, "Level:" );
	        ::setColor( mSt1 );
	        mContrastSizer->Add( mSt1, 0, wxALIGN_RIGHT );
	        mLevel = new wxSlider( cp, levelSliderID, currentLevel, 0, max,
	            wxDefaultPosition, wxSize(sliderWidth,-1),
	            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
	            wxDefaultValidator, "Level" );
	        mLevel->SetPageSize( 10 );
	        ::setColor( mLevel );
	        mContrastSizer->Add( mLevel, 0, wxGROW|wxLEFT|wxRIGHT );
		}
		else
			mLevel = NULL;

        //width
        mSt2 = new wxStaticText( cp, -1, "Width:" );
        ::setColor( mSt2 );
        mContrastSizer->Add( mSt2, 0, wxALIGN_RIGHT );
        mWidth = new wxSlider( cp, widthSliderID, currentWidth, 0, max,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Width" );
        mWidth->SetPageSize( 10 );
        ::setColor( mWidth );
        mContrastSizer->Add( mWidth, 0, wxGROW|wxLEFT|wxRIGHT );

		if (objTypeID)
		{
			mSt3 = new wxStaticText( cp, -1, "Type:" );
			::setColor( mSt3 );
			mContrastSizer->Add( mSt3, 0, wxALIGN_RIGHT );
			wxArrayString choices1;
			choices1.Add(wxT("-1"));
			choices1.Add(wxT("0"));
			choices1.Add(wxT("1"));
			choices1.Add(wxT("-1 B"));
			choices1.Add(wxT("0 B"));
			choices1.Add(wxT("1 B"));
			mObjType = new wxComboBox( cp, objTypeID, choices1[
				currentObjType], wxDefaultPosition, wxSize(sliderWidth, -1),
				choices1, wxCB_DROPDOWN, wxDefaultValidator, "Type" );
			::setColor( mObjType );
			mContrastSizer->Add( mObjType, 0, wxGROW|wxLEFT|wxRIGHT );
		}

        mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }

    int getWeight   ( void ) const { return mWeight -> GetValue ( ); }
    int getLevel    ( void ) const { return mLevel -> GetValue ( );  }
    int getWidth    ( void ) const { return mWidth -> GetValue ( );  }
	int getObjType  ( void ) const { return mObjType->GetCurrentSelection(); }
	void setRange (int min , int max) { mLevel -> SetRange ( min, max );  mWidth -> SetRange ( min, max ); }
	void setValue (int weight, int level, int width) { mWeight -> SetValue ( weight );  mLevel -> SetValue ( level ); mWidth -> SetValue ( width ); }
    
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief FunctionControls dtor. */
    ~FunctionControls ( void ) {
		mContrastSizer->Clear( true );
		mContrastBoxSizer->Clear( true );
        mContrastBoxSizer->Remove( mContrastSizer );
        mBottomSizer->Remove( mContrastBoxSizer );
    }
};

#endif
