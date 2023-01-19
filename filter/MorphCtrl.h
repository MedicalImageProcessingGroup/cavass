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
 * \file   MorphControls.h
 * \brief  Definition and implementation of MorphControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __MorphCtrl_h
#define __MorphCtrl_h

/**
 * \brief Definition and implementation of MorphControls class.
 *
 * Morph controls consist of a text control for Morph
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class MorphControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mMorphParBox;
    wxSizer*          mMorphParBoxSizer;
    wxFlexGridSizer*  mMorphParSizer;

    wxStaticText*     mMorphST;       //static text for Morph
    wxComboBox*       mMorphCB;

	wxStaticText*     mIterationsST;       //static text for iterations
	wxTextCtrl*       mIterationsTC;

public:
    /** \brief MorphControls ctor. */
    MorphControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int MorphID,int Morph, int iterations,const int iterationsID)

    {
        mBottomSizer = bottomSizer;
        mMorphParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mMorphParBox );
        mMorphParBoxSizer = new wxStaticBoxSizer( mMorphParBox, wxHORIZONTAL );
        mMorphParSizer = new wxFlexGridSizer( 2, 1, 1 );  //cols,vgap,hgap
        mMorphParSizer->SetMinSize( controlsWidth, 0 );
        mMorphParSizer->AddGrowableCol( 0 );

        //Morph
        mMorphST = new wxStaticText( cp, -1, "N:" );
        ::setColor( mMorphST );

        mMorphParSizer->Add( mMorphST, 0, wxALIGN_RIGHT );

        wxString MorphStr = wxString::Format("%d",Morph);
        wxArrayString sa;
        sa.Add("5");
        sa.Add("7");
        sa.Add("9");
        sa.Add("19");
        sa.Add("27");
        mMorphCB = new wxComboBox( cp, MorphID, MorphStr, wxDefaultPosition,
            wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY);
        ::setColor( mMorphCB );

        mMorphParSizer->Add( mMorphCB, 0, wxGROW|wxLEFT|wxRIGHT );

        //iterations

        mIterationsST = new wxStaticText( cp, -1, "Iterations:" );
        ::setColor( mIterationsST );

        mMorphParSizer->Add( mIterationsST, 0, wxALIGN_RIGHT );

        wxString iterStr = wxString::Format("%d",iterations);
        mIterationsTC = new wxTextCtrl( cp, iterationsID, iterStr,
            wxDefaultPosition, wxDefaultSize);
        ::setColor( mIterationsTC );

        mMorphParSizer->Add( mIterationsTC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMorphParBoxSizer->Add( mMorphParSizer, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mMorphParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief MorphControls dtor. */
    ~MorphControls ( void ) 
    {
        if( mMorphCB != NULL )
        {
            delete mMorphCB;
            mMorphCB = NULL;
        }
        if( mMorphST != NULL )
        {
            delete mMorphST;
            mMorphST = NULL;
        }
        if (mIterationsST)
            delete mIterationsST;
        mMorphParSizer->Clear( true );
        mMorphParBoxSizer->Clear( true );
        mMorphParBoxSizer->Remove( mMorphParSizer );
        mBottomSizer->Remove( mMorphParBoxSizer );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void SelectN(const wxString N)
    {
        mMorphCB->SetValue(N);
    }
};

#endif
