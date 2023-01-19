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
 * \file   SizeControls.h
 * \brief  Definition and implementation of SizeControls class.
 * \author Ying Zhuge, Ph.D.
 *
 * Copyright: (C) 2007, Ying Zhuge
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SizeControls_h
#define __SizeControls_h

/**
 * \brief Definition and implementation of SizeControls class.
 *
 * Size controls consist of a text control for sigma and iterations
 *
 *
 * Note: Callbacks are handled by the caller.
 */
class SizeControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mSizeParBox;
    wxSizer*          mSizeParBoxSizer;
    wxFlexGridSizer*  mSizeParSizer;

    wxStaticText*     mSizeXST;       //static text for Size X
    wxTextCtrl*       mSizeXTC;

    wxStaticText*     mSizeYST;       //static text for Size Y
    wxTextCtrl*       mSizeYTC;

    wxStaticText*     mSizeZST;       //static text for Size Z
    wxTextCtrl*       mSizeZTC;


public:
    /** \brief SizeControls ctor. */
    SizeControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
           const int sizeXID,const int sizeYID, const int sizeZID, 
           int xSize, int ySize, int zSize)

    {
        mBottomSizer = bottomSizer;
        mSizeParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSizeParBox );
        mSizeParBoxSizer = new wxStaticBoxSizer( mSizeParBox, wxHORIZONTAL );
        mSizeParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mSizeParSizer->SetMinSize( controlsWidth, 0 );
        mSizeParSizer->AddGrowableCol( 0 );
        
        //Size X
        mSizeXST = new wxStaticText( cp, -1, "Size x:" );
        ::setColor( mSizeXST );
        mSizeParSizer->Add( mSizeXST, 0, wxALIGN_RIGHT );
        
        wxString sizeXStr = wxString::Format("%d",xSize);
        mSizeXTC = new wxTextCtrl( cp, sizeXID, sizeXStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mSizeXTC );
        mSizeParSizer->Add( mSizeXTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        //Size Y
        mSizeYST = new wxStaticText( cp, -1, "Size y:" );
        ::setColor( mSizeYST );
        mSizeParSizer->Add( mSizeYST, 0, wxALIGN_RIGHT );
        
        wxString sizeYStr = wxString::Format("%d",ySize);
        mSizeYTC = new wxTextCtrl( cp, sizeYID, sizeYStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mSizeYTC );
        mSizeParSizer->Add( mSizeYTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        //Size Z
        mSizeZST = new wxStaticText( cp, -1, "Size z:" );
        ::setColor( mSizeZST );
        mSizeParSizer->Add( mSizeZST, 0, wxALIGN_RIGHT );
        
        wxString sizeZStr = wxString::Format("%d",zSize);
        mSizeZTC = new wxTextCtrl( cp, sizeZID, sizeZStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mSizeZTC );
        mSizeParSizer->Add( mSizeZTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        
        mSizeParBoxSizer->Add( mSizeParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mSizeParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SizeControls dtor. */
    ~SizeControls ( void ) {
	    mSizeParSizer->Clear( true );
		mSizeParBoxSizer->Clear( true );

		mSizeParBoxSizer->Remove( mSizeParSizer );
        mBottomSizer->Remove( mSizeParBoxSizer );
    }
};

#endif
