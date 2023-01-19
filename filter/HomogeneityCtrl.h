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
 * \file   HomogenControls.h
 * \brief  Definition and implementation of HomogenControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __HomogeneityCtl_h
#define __HomogeneityCtl_h

/**
 * \brief Definition and implementation of HomogenControls class.
 *
 * Homogen controls consist of a text control for Homogen
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class HomogenControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mHomogenParBox;
    wxSizer*          mHomogenParBoxSizer;
    wxFlexGridSizer*  mHomogenParSizer;

    wxStaticText*     mHomogenST;       //static text for Homogen
    wxTextCtrl*       mHomogenTC;

public:
    /** \brief HomogenControls ctor. */
    HomogenControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int HomogenID, int Homogen)

    {
        mBottomSizer = bottomSizer;
        mHomogenParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mHomogenParBox );
        mHomogenParBoxSizer = new wxStaticBoxSizer( mHomogenParBox, wxHORIZONTAL );
        mHomogenParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mHomogenParSizer->SetMinSize( controlsWidth, 0 );
        mHomogenParSizer->AddGrowableCol( 0 );
        
        //Homogen
        mHomogenST = new wxStaticText( cp, -1, "Homogeneity:" );
        ::setColor( mHomogenST );
        
        mHomogenParSizer->Add( mHomogenST, 0, wxALIGN_RIGHT );
        
        wxString HomogenStr = wxString::Format("%d",Homogen);
        mHomogenTC = new wxTextCtrl( cp, HomogenID, HomogenStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mHomogenTC );
        
        mHomogenParSizer->Add( mHomogenTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mHomogenParBoxSizer->Add( mHomogenParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mHomogenParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief HomogenControls dtor. */
    ~HomogenControls ( void ) 
	{
		if( mHomogenTC != NULL )
			delete mHomogenTC;
		if( mHomogenST != NULL )
			delete mHomogenST;
		mHomogenParSizer->Clear( true );
		mHomogenParBoxSizer->Clear( true );
		mHomogenParBoxSizer->Remove( mHomogenParSizer );
		mBottomSizer->Remove( mHomogenParBoxSizer );
    }
};

#endif
