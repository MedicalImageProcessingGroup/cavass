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
 * \file   SigmaControls.h
 * \brief  Definition and implementation of SigmaControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SigmaControls_h
#define __SigmaControls_h

/**
 * \brief Definition and implementation of SigmaControls class.
 *
 * Sigma controls consist of a text control for sigma
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class SigmaControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mSigmaParBox;
    wxSizer*          mSigmaParBoxSizer;
    wxFlexGridSizer*  mSigmaParSizer;

    wxStaticText*     mSigmaST;       //static text for sigma
    wxTextCtrl*       mSigmaTC;

public:
    /** \brief SigmaControls ctor. */
    SigmaControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int sigmaID,double sigma)

    {
        mBottomSizer = bottomSizer;
        mSigmaParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSigmaParBox );
        mSigmaParBoxSizer = new wxStaticBoxSizer( mSigmaParBox, wxHORIZONTAL );
        mSigmaParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mSigmaParSizer->SetMinSize( controlsWidth, 0 );
        mSigmaParSizer->AddGrowableCol( 0 );
        
        //sigma
        mSigmaST = new wxStaticText( cp, -1, "sigma:" );
        ::setColor( mSigmaST );
        
        mSigmaParSizer->Add( mSigmaST, 0, wxALIGN_RIGHT );
        
        wxString sigmaStr = wxString::Format("%5.2f",sigma);
        mSigmaTC = new wxTextCtrl( cp, sigmaID, sigmaStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mSigmaTC );
        
        mSigmaParSizer->Add( mSigmaTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mSigmaParBoxSizer->Add( mSigmaParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mSigmaParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SigmaControls dtor. */
    ~SigmaControls ( void ) 
	{
		if( mSigmaTC != NULL )
			delete mSigmaTC;
		if( mSigmaST != NULL )
			delete mSigmaST;
		mSigmaParSizer->Clear( true );
		mSigmaParBoxSizer->Clear( true );
		mSigmaParBoxSizer->Remove( mSigmaParSizer );
		mBottomSizer->Remove( mSigmaParBoxSizer );
    }
};

#endif
