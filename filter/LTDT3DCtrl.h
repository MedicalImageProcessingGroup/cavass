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
 * \file   LTDT3DControls.h
 * \brief  Definition and implementation of LTDT3DControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2009
 *
 * I love this wonderful world!
 */
//======================================================================
#ifndef __LTDT3DCtrl_h
#define __LTDT3DCtrl_h

/**
 * \brief Definition and implementation of LTDT3DControls class.
 *
 * LTDT3D controls consist of a text control for LTDT3D
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class LTDT3DControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mLTDT3DParBox;
    wxSizer*          mLTDT3DParBoxSizer;
    wxFlexGridSizer*  mLTDT3DParSizer;

	wxComboBox*       mG5;

    wxCheckBox*       mCB;            ///< overlay on/off checkbox
	wxCheckBox*       mFTSaveCB;            ///< overlay on/off checkbox
    wxStaticText*     mPararellST;     ///< overlay checkbox label
	wxStaticText*     mFTSaveST;  

    wxStaticText*     mLTDT3DST;       //static text for LTDT3D    


public:
    /** \brief LTDT3DControls ctor. */
    LTDT3DControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int LTDT3DID, const int LTDT3D_PararellID, const int LTDT3D_FTSaveID, int LTDT3D)

    {
        mBottomSizer = bottomSizer;
        mLTDT3DParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mLTDT3DParBox );
        mLTDT3DParBoxSizer = new wxStaticBoxSizer( mLTDT3DParBox, wxHORIZONTAL );
        mLTDT3DParSizer = new wxFlexGridSizer( 2, 1, 1 );  //cols,vgap,hgap
        mLTDT3DParSizer->SetMinSize( controlsWidth, 0 );
        mLTDT3DParSizer->AddGrowableCol( 0 );
        
        //LTDT3D
        mLTDT3DST = new wxStaticText( cp, -1, "Distance Type:" );
        ::setColor( mLTDT3DST );
        
        mLTDT3DParSizer->Add( mLTDT3DST, 0, wxALIGN_RIGHT );        
		
		wxArrayString choices;
	    choices.Add(wxT("Background To Foreground" ));
        choices.Add(wxT("Foreground To Background"));
        choices.Add(wxT("Both"));
	choices.Add(wxT("Digital Boundary"));
		
        mG5 = new wxComboBox( cp, LTDT3DID, wxT("Background To Foreground" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices,wxCB_DROPDOWN,
            wxDefaultValidator,"Choose Distance Type:" );
        ::setColor( mG5);
        mLTDT3DParSizer->Add( mG5, 0, wxGROW|wxLEFT|wxRIGHT);

		mPararellST = new wxStaticText( cp, -1, "parallel:" );
        ::setColor( mPararellST );
        mLTDT3DParSizer->Add( mPararellST, 0, wxALIGN_RIGHT );
        
        //wxSizer*  gsLastRow = new wxGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mCB = new wxCheckBox( cp, LTDT3D_PararellID, "on/off" );
        ::setColor( mCB );
        mCB->SetValue( 0 );
		mLTDT3DParSizer->Add( mCB /*, 0, wxALIGN_RIGHT*/);

		mFTSaveST = new wxStaticText( cp, -1, "Save FT Image:" );
        ::setColor( mFTSaveST );
        mLTDT3DParSizer->Add( mFTSaveST, 0, wxALIGN_RIGHT );
        
        //wxSizer*  gsLastRow = new wxGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mFTSaveCB = new wxCheckBox( cp, LTDT3D_FTSaveID, "on/off" );
        ::setColor( mFTSaveCB );
        mFTSaveCB->SetValue( 0 );
		mLTDT3DParSizer->Add( mFTSaveCB /*, 0, wxALIGN_RIGHT*/);


        mLTDT3DParBoxSizer->Add( mLTDT3DParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mLTDT3DParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief LTDT3DControls dtor. */
    ~LTDT3DControls ( void ) 
	{
		if( mG5 != NULL )
			delete mG5;
		if( mLTDT3DST != NULL )
			delete mLTDT3DST;
		mLTDT3DParSizer->Clear( true );
		mLTDT3DParBoxSizer->Clear( true );
		mLTDT3DParBoxSizer->Remove( mLTDT3DParSizer );
		mBottomSizer->Remove( mLTDT3DParBoxSizer );
    }
};

#endif
