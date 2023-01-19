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
 * \file   SetFilterIndexControls.h
 * \brief  Definition and implementation of SetFilterIndexControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SetLayoutControls_h
#define __SetLayoutControls_h

/**
 * \brief Definition and implementation of SetFilterIndexControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */

class LayoutControls 
{
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mLayoutParBox;
    wxSizer*          mLayoutParBoxSizer;
    wxFlexGridSizer*  mLayoutParSizer;

    wxCheckBox*       mCB;    

public:
    /** \brief LayoutControls ctor. */
    LayoutControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int LayoutID )

    {
        mBottomSizer = bottomSizer;
        mLayoutParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mLayoutParBox );
        mLayoutParBoxSizer = new wxStaticBoxSizer( mLayoutParBox, wxHORIZONTAL );
        mLayoutParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mLayoutParSizer->SetMinSize( controlsWidth, 0 );
        mLayoutParSizer->AddGrowableCol( 0 );
        
        //Layout
        mCB = new wxCheckBox( cp, LayoutID, "Adjust Layout" );
        ::setColor( mCB );
        mCB->SetValue( false );		
        
        mLayoutParSizer->Add( mCB, 0, wxGROW|wxLEFT|wxRIGHT );        
        mLayoutParBoxSizer->Add( mLayoutParSizer, 0, wxGROW|wxALL, 10 );        
        mBottomSizer->Prepend( mLayoutParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief LayoutControls dtor. */
    ~LayoutControls ( void ) 
	{
		if( mCB  != NULL )
			delete mCB ;
		
		mLayoutParSizer->Clear( true );
		mLayoutParBoxSizer->Clear( true );
		mLayoutParBoxSizer->Remove( mLayoutParSizer );
		mBottomSizer->Remove( mLayoutParBoxSizer );
    }
};

#endif
