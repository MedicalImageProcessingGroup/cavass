/*
  Copyright 1993-2012 Medical Image Processing Group
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
 * \file   ParameterControls.h
 * \brief  Definition and implementation of ParameterControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ParameterControls_h
#define __ParameterControls_h

/**
 * \brief Definition and implementation of ParameterControls class.
 *
 * Parameter controls consist of a control box with window center and width
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class ParameterControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    wxCheckBox*       mCb0;        ///< invert the gray map checkbox
    wxCheckBox*       mCb1;        ///< invert the gray map checkbox
    wxCheckBox*       mCb2;        ///< invert the gray map checkbox
    wxCheckBox*       mCb3;        ///< invert the gray map checkbox
    wxCheckBox*       mCb4;        ///< invert the gray map checkbox
    wxComboBox*       mG0;
    wxComboBox*       mG1;
    wxComboBox*       mG2;
    wxComboBox*       mG3;
    wxComboBox*       mG4;
	wxComboBox*       mG5;
    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
    wxFlexGridSizer*  mContrastSizer;
    wxFlexGridSizer*  mfg;
	wxFlexGridSizer*  mfg1;
    wxStaticText*     mSt;        ///< center slider title
	wxStaticText*     mSt1;
	wxStaticText*     mSt2;
	wxStaticText*     mSt3;
	wxStaticText*     mSt4;
	wxSlider*         mSS;
	wxSlider*         mES;
    
public:
    /** \brief ParameterControls ctor. */
    ParameterControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const bool currentHigh, const bool currentLow, const bool currentDiff,
        const bool currentSum, const bool currentRelDiff,
		const int currentSslice, const int currentEslice, const int NoSlices,
		const int highID, const int lowID, const int diffID, const int sumID,
		const int reldiffID, const int highComboID, const int lowComboID,
		const int diffComboID, const int sumComboID, const int reldiffComboID,
		const int outAffID, const int startSliceID, const int endSliceID )
    {
        mBottomSizer = bottomSizer;
        mContrastBox = new wxStaticBox( cp, -1, title );
        ::setColor( mContrastBox );
        mContrastBoxSizer = new wxStaticBoxSizer( mContrastBox, wxHORIZONTAL );

        mContrastSizer = new wxFlexGridSizer( 1, 2, 2 );  //cols,vgap,hgap
        mContrastSizer->SetMinSize( controlsWidth/2, 0 );
        mContrastSizer->AddGrowableCol( 0 );

        
        
        // - - - - - - - - - -
        mfg = new wxFlexGridSizer( 2, 1, 1);  //cols,vgap,hgap
        mCb0 = new wxCheckBox( cp, highID, "high" );
        ::setColor( mCb0 );
        if (currentHigh)    mCb0->SetValue( 1 );
        else                  mCb0->SetValue( 0 );
        mfg->Add( mCb0, 0, wxALIGN_LEFT);

        wxArrayString choices3;
	    choices3.Add(wxT("Gaussian" ));
        choices3.Add(wxT("Ramp"));
        choices3.Add(wxT("Box"));

                
        mG0 = new wxComboBox( cp, highComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionHigh" );
           ::setColor( mG0);
        mfg->Add( mG0, 0, wxGROW|wxLEFT|wxRIGHT,3 );
		mG0->Enable( currentHigh );

        /*--------------------------------*/
        
        mCb1 = new wxCheckBox( cp, lowID, "low" );
        ::setColor( mCb1 );
        if (currentLow)    mCb1->SetValue( 1 );
        else                  mCb1->SetValue( 0 );
        mfg->Add( mCb1, 0, wxALIGN_LEFT );

        mG1 = new wxComboBox( cp, lowComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionLow" );
           ::setColor( mG1);
        mfg->Add( mG1, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        mG1->Enable( currentLow );
        /*--------------------------------*/
        
        mCb2 = new wxCheckBox( cp, diffID, "difference" );
        ::setColor( mCb2 );
        if (currentDiff)    mCb2->SetValue( 1 );
        else                  mCb2->SetValue( 0 );
        mfg->Add( mCb2, 0, wxALIGN_LEFT );
        
        mG2 = new wxComboBox( cp, diffComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionDiff" );
           ::setColor( mG2);
        mfg->Add( mG2, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        mG2->Enable( currentDiff );
        /*--------------------------------*/

        mCb3 = new wxCheckBox( cp, sumID, "sum" );
        ::setColor( mCb3 );
        if (currentSum)    mCb3->SetValue( 1 );
        else                  mCb3->SetValue( 0 );
        mfg->Add( mCb3, 0,wxALIGN_LEFT );
        
        mG3 = new wxComboBox( cp, sumComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionSum" );
           ::setColor( mG3);
        mfg->Add( mG3, 0, wxGROW|wxLEFT|wxRIGHT,3 );
		mG3->Enable( currentSum );
        /*--------------------------------*/

        mCb4 = new wxCheckBox( cp, reldiffID, "relative difference" );
        ::setColor( mCb4 );
        if (currentRelDiff)    mCb4->SetValue( 1 );
        else                  mCb4->SetValue( 0 );
        mfg->Add( mCb4, 0, wxALIGN_LEFT );
        
        mG4 = new wxComboBox( cp, reldiffComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionRelDiff" );
           ::setColor( mG4);
        mfg->Add( mG4, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        mG4->Enable( currentRelDiff );
		
		mContrastSizer->Add( mfg, 0, wxGROW );
		
        		
		mfg1 = new wxFlexGridSizer( 4, 2, 2);  //cols,vgap,hgap
		//---Starting slice---
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		
        mSt3 = new wxStaticText( cp, -1, "Starting slice:" );
        ::setColor( mSt3 );
        mfg1->Add( mSt3, 0, wxALIGN_LEFT );
        mSS = new wxSlider( cp, startSliceID, currentSslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Sslice" );
        mSS->SetPageSize( 10 );
        ::setColor( mSS );
        mfg1->Add( mSS, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        //---------------------------
     
	   //---Ending slice---
        mSt4 = new wxStaticText( cp, -1, "Ending slice:" );
        ::setColor( mSt4 );
        mfg1->Add( mSt4, 0, wxALIGN_LEFT );
        mES = new wxSlider( cp, endSliceID, currentEslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Eslice" );
        mES->SetPageSize( 10 );
        ::setColor( mES );
        mfg1->Add( mES, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        //---------------------------
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer

        /*--------------------------------*/
		
		 mSt1 = new wxStaticText( cp, -1, "Affinity Output:" );
        ::setColor( mSt1 );
        mfg1->Add( mSt1, 0, wxALIGN_LEFT );

		
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
        mfg1->Add( mG5, 0, wxGROW|wxLEFT|wxRIGHT);
		
        /*--------------------------------*/


        mContrastSizer->Add(mfg1, 0, wxGROW);

        
        mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    int getHighComboSelection    ( void ) const { return mG0 -> GetCurrentSelection ( );     }
    int getLowComboSelection    ( void ) const { return mG1 -> GetCurrentSelection ( );     }
    int getDiffComboSelection    ( void ) const { return mG2 -> GetCurrentSelection ( );     }
    int getSumComboSelection    ( void ) const { return mG3 -> GetCurrentSelection ( );     }
    int getRelDiffComboSelection    ( void ) const { return mG4 -> GetCurrentSelection ( );     }
	int getAffOutSelection    ( void ) const { return mG5 -> GetCurrentSelection ( );     }
	bool getOnHigh    ( void ) const { return mCb0 -> IsChecked ( );                       }
	bool getOnLow    ( void ) const { return mCb1 -> IsChecked ( );                       }
	bool getOnDiff    ( void ) const { return mCb2 -> IsChecked ( );                       }
	bool getOnSum    ( void ) const { return mCb3 -> IsChecked ( );                       }
	bool getOnRelDiff    ( void ) const { return mCb4 -> IsChecked ( );                       }
	void enableHighCombo (bool value) { mG0->Enable( value); }
    void enableLowCombo (bool value) { mG1->Enable( value); }
	void enableDiffCombo (bool value) { mG2->Enable( value); }
	void enableSumCombo (bool value) { mG3->Enable( value); }
	void enableRelDiffCombo (bool value) { mG4->Enable( value); }
	int getStartSlice    ( void ) const { return mSS -> GetValue ( );                           }
    int getEndSlice    ( void ) const { return mES -> GetValue ( );                             }
	void setHighComboSelection ( const wxString function ) { mG0->SetValue(function); }
	void setLowComboSelection ( const wxString function ) { mG1->SetValue(function); }
	void setDiffComboSelection ( const wxString function ) { mG2->SetValue(function); }
	void setSumComboSelection ( const wxString function ) { mG3->SetValue(function); }
	void setRelDiffComboSelection ( const wxString function ) { mG4->SetValue(function); }

    /** \brief ParameterControls dtor. */
    ~ParameterControls ( void ) {
        mBottomSizer->Remove( mContrastBoxSizer );
        mBottomSizer->Layout();
        delete mCb0;
        delete mCb1;
        delete mCb2;
        delete mCb3;
        delete mCb4;
        delete mG0;
        delete mG1;
        delete mG2;
        delete mG3;
        delete mG4;
		delete mG5;
  //      delete mContrastBox;
		delete mSt1;
		delete mSt3;
		delete mSt4;
		delete mSS;
		delete mES;
    }
};

#endif
