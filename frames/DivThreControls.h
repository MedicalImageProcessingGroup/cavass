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
 * \file   DivThreControls.h
 * \brief  Definition and implementation of DivThreControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __DivThreControls_h
#define __DivThreControls_h

/**
 * \brief Definition and implementation of DivThreControls class.
 *
 * DivThre controls consist of a text control for DivThre
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class DivThreControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mDivThreParBox;
    wxSizer*          mDivThreParBoxSizer;
    wxFlexGridSizer*  mDivThreParSizer;

    wxStaticText*     mDivThreST;       //static text for DivThre
    wxTextCtrl*       mDivThreTC;

public:
    /** \brief DivThreControls ctor. */
    DivThreControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int DivThreID, int DivThre)

    {
        mBottomSizer = bottomSizer;
        mDivThreParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mDivThreParBox );
        mDivThreParBoxSizer = new wxStaticBoxSizer( mDivThreParBox, wxHORIZONTAL );
        mDivThreParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mDivThreParSizer->SetMinSize( controlsWidth, 0 );
        mDivThreParSizer->AddGrowableCol( 0 );
        
        //DivThre
        mDivThreST = new wxStaticText( cp, -1, "DivThre:" );
        ::setColor( mDivThreST );
        
        mDivThreParSizer->Add( mDivThreST, 0, wxALIGN_RIGHT );
        
        wxString DivThreStr = wxString::Format("%d",DivThre);
        mDivThreTC = new wxTextCtrl( cp, DivThreID, DivThreStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mDivThreTC );
        
        mDivThreParSizer->Add( mDivThreTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mDivThreParBoxSizer->Add( mDivThreParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mDivThreParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief DivThreControls dtor. */
    ~DivThreControls ( void ) 
	{
		if( mDivThreST != NULL )
			delete mDivThreST;
		if( mDivThreTC != NULL )
			delete mDivThreTC;        

		mDivThreParSizer->Clear( true );
		mDivThreParBoxSizer->Clear( true );
        mDivThreParBoxSizer->Remove( mDivThreParSizer );
        mBottomSizer->Remove( mDivThreParBoxSizer );
    }
};

#endif
