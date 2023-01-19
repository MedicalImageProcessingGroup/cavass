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
 * \file   OffsetControls.h
 * \brief  Definition and implementation of OffsetControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __OffsetControls_h
#define __OffsetControls_h

/**
 * \brief Definition and implementation of OffsetControls class.
 *
 * Offset controls consist of a text control for Offset
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class OffsetControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mOffsetParBox;
    wxSizer*          mOffsetParBoxSizer;
    wxFlexGridSizer*  mOffsetParSizer;

    wxStaticText*     mOffsetST;       //static text for Offset
    wxTextCtrl*       mOffsetTC;

public:
    /** \brief OffsetControls ctor. */
    OffsetControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int OffsetID,int Offset)

    {
        mBottomSizer = bottomSizer;
        mOffsetParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mOffsetParBox );
        mOffsetParBoxSizer = new wxStaticBoxSizer( mOffsetParBox, wxHORIZONTAL );
        mOffsetParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mOffsetParSizer->SetMinSize( controlsWidth, 0 );
        mOffsetParSizer->AddGrowableCol( 0 );
        
        //Offset
        mOffsetST = new wxStaticText( cp, -1, "Offset:" );
        ::setColor( mOffsetST );
        
        mOffsetParSizer->Add( mOffsetST, 0, wxALIGN_RIGHT );
        
        wxString OffsetStr = wxString::Format("%d",Offset);
        mOffsetTC = new wxTextCtrl( cp, OffsetID, OffsetStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mOffsetTC );
        
        mOffsetParSizer->Add( mOffsetTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mOffsetParBoxSizer->Add( mOffsetParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mOffsetParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief OffsetControls dtor. */
    ~OffsetControls ( void ) 
	{
		if( mOffsetST != NULL )
			delete mOffsetST;
		if( mOffsetTC != NULL )
			delete mOffsetTC;        	

		mOffsetParSizer->Clear( true );
		mOffsetParBoxSizer->Clear( true );
        mOffsetParBoxSizer->Remove( mOffsetParSizer );
        mBottomSizer->Remove( mOffsetParBoxSizer );
    }
};

class CoefficientControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mCoefficientParBox;
    wxSizer*          mCoefficientParBoxSizer;
    wxFlexGridSizer*  mCoefficientParSizer;

    wxStaticText*     mCoefficientST;       //static text for Coefficient
    wxTextCtrl*       mCoefficientTC;

public:
    /** \brief CoefficientControls ctor. */
    CoefficientControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int CoefficientID, double Coefficient)

    {
        mBottomSizer = bottomSizer;
        mCoefficientParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mCoefficientParBox );
        mCoefficientParBoxSizer = new wxStaticBoxSizer( mCoefficientParBox, wxHORIZONTAL );
        mCoefficientParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mCoefficientParSizer->SetMinSize( controlsWidth, 0 );
        mCoefficientParSizer->AddGrowableCol( 0 );
        
        //Coefficient
        mCoefficientST = new wxStaticText( cp, -1, "Coefficient:" );
        ::setColor( mCoefficientST );
        
        mCoefficientParSizer->Add( mCoefficientST, 0, wxALIGN_RIGHT );

        wxString CoefficientStr = wxString::Format("%.8f", Coefficient);
        mCoefficientTC = new wxTextCtrl( cp, CoefficientID, CoefficientStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mCoefficientTC );
        
        mCoefficientParSizer->Add( mCoefficientTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mCoefficientParBoxSizer->Add( mCoefficientParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mCoefficientParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief CoefficientControls dtor. */
    ~CoefficientControls ( void ) 
	{
		if( mCoefficientST != NULL )
			delete mCoefficientST;
		if( mCoefficientTC != NULL )
			delete mCoefficientTC;        	

		mCoefficientParSizer->Clear( true );
		mCoefficientParBoxSizer->Clear( true );
        mCoefficientParBoxSizer->Remove( mCoefficientParSizer );
        mBottomSizer->Remove( mCoefficientParBoxSizer );
    }
};

#endif
