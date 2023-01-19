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
 * \file   CovarianceControls.h
 * \brief  Definition and implementation of CovarianceControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __CovarianceControls_h
#define __CovarianceControls_h

/**
 * \brief Definition and implementation of CovarianceControls class.
 *
 * Covariance controls consist of a control box with window center and width
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class CovarianceControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    wxCheckBox*       mCb0;        ///< invert the gray map checkbox
    wxCheckBox*       mCb1;        ///< invert the gray map checkbox
    wxCheckBox*       mCb2;        ///< invert the gray map checkbox
    wxCheckBox*       mCb3;        ///< invert the gray map checkbox
    wxCheckBox*       mCb4;        ///< invert the gray map checkbox
    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
    wxFlexGridSizer*  mContrastSizer;
    //wxFlexGridSizer*  mfg;
    wxComboBox*       mG5;
    //wxFlexGridSizer*  mfg1;
    wxStaticText*     mSt1;
    wxStaticText*     mSt2;
    wxStaticText*     mSt3;
    wxStaticText*     mSt4;
    wxSlider*         mSS;
    wxSlider*         mES;
public:
    /** \brief CovarianceControls ctor. */
    CovarianceControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const bool currentHigh, const bool currentLow, const bool currentDiff, const bool currentSum, const bool currentRelDiff,
        const int currentSslice, const int currentEslice, const int NoSlices,
        const int highID, const int lowID, const int diffID, const int sumID, const int reldiffID, const int outAffID,
        const int startSliceID, const int endSliceID )
    {
        mBottomSizer = bottomSizer;
        mContrastBox = new wxStaticBox( cp, -1, title );
        ::setColor( mContrastBox );
        mContrastBoxSizer = new wxStaticBoxSizer( mContrastBox, wxHORIZONTAL );

        mContrastSizer = new wxFlexGridSizer( 2, 2, 2);  //cols,vgap,hgap
        mContrastSizer->SetMinSize( controlsWidth/2, 0 );
        mContrastSizer->AddGrowableCol( 0 );

        // - - - - - - - - - -
        //mfg = new wxFlexGridSizer( 1, 1, 1 );  //cols,vgap,hgap
        mCb0 = new wxCheckBox( cp, highID, "high" );
        ::setColor( mCb0 );
        if (currentHigh)    mCb0->SetValue( 1 );
        else                  mCb0->SetValue( 0 );
        mContrastSizer->Add( mCb0, 0, wxALIGN_LEFT);

        mCb1 = new wxCheckBox( cp, lowID, "low" );
        ::setColor( mCb1 );
        if (currentLow)    mCb1->SetValue( 1 );
        else                  mCb1->SetValue( 0 );
        mContrastSizer->Add( mCb1, 0, wxALIGN_LEFT);

        mCb2 = new wxCheckBox( cp, diffID, "difference" );
        ::setColor( mCb2 );
        if (currentDiff)    mCb2->SetValue( 1 );
        else                  mCb2->SetValue( 0 );
        mContrastSizer->Add( mCb2, 0, wxALIGN_LEFT);
        
        mCb3 = new wxCheckBox( cp, sumID, "sum" );
        ::setColor( mCb3 );
        if (currentSum)    mCb3->SetValue( 1 );
        else                  mCb3->SetValue( 0 );
        mContrastSizer->Add( mCb3, 0,wxALIGN_LEFT);

        mCb4 = new wxCheckBox( cp, reldiffID, "relative difference" );
        ::setColor( mCb4 );
        if (currentSum)    mCb4->SetValue( 1 );
        else                  mCb4->SetValue( 0 );
        mContrastSizer->Add( mCb4, 0, wxALIGN_LEFT);

        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
       
        //---Starting slice---
        mSt3 = new wxStaticText( cp, -1, "Starting slice:" );
        ::setColor( mSt3 );
        mContrastSizer->Add( mSt3, 0, wxALIGN_LEFT);
        mSS = new wxSlider( cp, startSliceID, currentSslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Sslice" );
        mSS->SetPageSize( 10 );
        ::setColor( mSS );
        mContrastSizer->Add( mSS, 0, wxGROW|wxLEFT|wxRIGHT,3);
        //---------------------------
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        //---Ending slice---
        mSt4 = new wxStaticText( cp, -1, "Ending slice:" );
        ::setColor( mSt4 );
        mContrastSizer->Add( mSt4, 0, wxALIGN_LEFT);
        mES = new wxSlider( cp, endSliceID, currentEslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Eslice" );
        mES->SetPageSize( 10 );
        ::setColor( mES );
        mContrastSizer->Add( mES, 0, wxGROW|wxLEFT|wxRIGHT,3);
        //---------------------------
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer
        mContrastSizer->Add( 0, 5, 20, wxGROW );  //spacer

        mSt1 = new wxStaticText( cp, -1, "  Affinity Output:" );
        ::setColor( mSt1 );
        mContrastSizer->Add( mSt1, 0, wxALIGN_LEFT );

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
        mContrastSizer->Add( mG5, 0, wxGROW|wxLEFT|wxRIGHT,3);
        
        // - - - - - - - - - -
    
        mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    int getAffOutSelection    ( void ) const { return mG5 -> GetCurrentSelection ( );     }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief CovarianceControls dtor. */
    ~CovarianceControls ( void ) {
        mContrastSizer->Clear( true );
        mContrastBoxSizer->Clear( true );
        mContrastBoxSizer->Remove( mContrastSizer );
        mBottomSizer->Remove( mContrastBoxSizer );
    }
};

#endif
