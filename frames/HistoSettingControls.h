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
 * \file   HistoSettingControls.h
 * \brief  Definition and implementation of HistoSettingControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2003, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __HistoSettingControls_h
#define __HistoSettingControls_h

/**
 * \brief Definition and implementation of HistoSettingControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */

class HistoSettingControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    
    wxSlider*         mCenter;    ///< window center slider
    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
    wxFlexGridSizer*  mContrastSizer;
    wxStaticText*     mSt0;       ///< center slider title
    wxStaticText*     mSt1;       ///< width slider title
    
    wxSlider*         mWidth;     ///< window width slider
	wxComboBox*       mG5;
	wxStaticText*     mSt;
	wxStaticText*     mSt2;
	wxStaticText*     mSt3;
	wxStaticText*     mSt4;
	wxSlider*         mSS;
	wxSlider*         mES;
	
public:
    /** \brief HistoSettingControls ctor. */
    HistoSettingControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const int currentCenter, const int currentWidth, const int max, 
		const int currentSslice, const int currentEslice, const int NoSlices,
        const int centerSliderID, const int widthSliderID, 
		const int outAffID, const int startSliceID, const int endSliceID)
    {
        mBottomSizer = bottomSizer;
        mContrastBox = new wxStaticBox( cp, -1, title );
        ::setColor( mContrastBox );
        mContrastBoxSizer = new wxStaticBoxSizer( mContrastBox, wxHORIZONTAL );
        mContrastSizer = new wxFlexGridSizer( 2, 2, 2 );  //cols,vgap,hgap
        mContrastSizer->SetMinSize( controlsWidth/2, 0 );
        mContrastSizer->AddGrowableCol( 0 );

        //center
        mSt0 = new wxStaticText( cp, -1, "Low bin:" );
        ::setColor( mSt0 );
        mContrastSizer->Add( mSt0, 0, wxALIGN_LEFT );
        mCenter = new wxSlider( cp, centerSliderID, currentCenter, 0, max,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "LowBin" );
        ::setColor( mCenter );
        mCenter->SetPageSize( 10 );
        mContrastSizer->Add( mCenter, 0, wxGROW|wxLEFT|wxRIGHT,3 );

        //width
        mSt1 = new wxStaticText( cp, -1, "High bin:" );
        ::setColor( mSt1 );
        mContrastSizer->Add( mSt1, 0, wxALIGN_LEFT );
        mWidth = new wxSlider( cp, widthSliderID, currentWidth, 1, max,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "HighBin" );
        mWidth->SetPageSize( 10 );
        ::setColor( mWidth );
        mContrastSizer->Add( mWidth, 0, wxGROW|wxLEFT|wxRIGHT,3 );
		mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
		mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
				
		//---Starting slice---
        mSt3 = new wxStaticText( cp, -1, "Starting slice:" );
        ::setColor( mSt3 );
        mContrastSizer->Add( mSt3, 0, wxALIGN_LEFT );
        mSS = new wxSlider( cp, startSliceID, currentSslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Sslice" );
        mSS->SetPageSize( 10 );
        ::setColor( mSS );
        mContrastSizer->Add( mSS, 0, wxGROW|wxLEFT|wxRIGHT,3  );
        //---------------------------
     
	    //---Ending slice---
        mSt4 = new wxStaticText( cp, -1, "Ending slice:" );
        ::setColor( mSt4 );
        mContrastSizer->Add( mSt4, 0, wxALIGN_LEFT );
        mES = new wxSlider( cp, endSliceID, currentEslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Eslice" );
        mES->SetPageSize( 10 );
        ::setColor( mES );
        mContrastSizer->Add( mES, 0, wxGROW|wxLEFT|wxRIGHT,3  );
        //---------------------------
		mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
		mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
		 /*--------------------------------*/
        mSt = new wxStaticText( cp, -1, "Affinity Output:" );
        ::setColor( mSt );
        mContrastSizer->Add( mSt, 0, wxALIGN_LEFT );

		wxArrayString choices;
	    choices.Add(wxT("x   component" ));
        choices.Add(wxT("y   component"));
        choices.Add(wxT("z   component"));
		choices.Add(wxT("xy  component" ));
        choices.Add(wxT("xyz component"));
		
        mG5 = new wxComboBox( cp, outAffID, wxT("xy component" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices,wxCB_DROPDOWN,
            wxDefaultValidator,"AffOut" );
           ::setColor( mG5);
		mContrastSizer->Add(mG5, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        // - - - - - - - - - -
	    
        mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }

    int getLowBinLevel     ( void ) const { return mCenter -> GetValue ( );                     }
    int getHighBinLevel    ( void ) const { return mWidth -> GetValue ( );                      }
	int getAffOutSelection    ( void ) const { return mG5 -> GetCurrentSelection ( );     }
	int getStartSlice    ( void ) const { return mSS -> GetValue ( );                           }
    int getEndSlice    ( void ) const { return mES -> GetValue ( );                             }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief HistoSettingControls dtor. */
    ~HistoSettingControls ( void ) {
		mContrastSizer->Clear( true );
		mContrastBoxSizer->Clear( true );
        mContrastBoxSizer->Remove( mContrastSizer );
        mBottomSizer->Remove( mContrastBoxSizer );
    }
};

#endif
