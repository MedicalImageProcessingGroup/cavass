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
 * \file   SBAv3DControls.h
 * \brief  Definition and implementation of SBAv3DControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SBAv3DCtl_h
#define __SBAv3DCtl_h

/**
 * \brief Definition and implementation of SBAv3DControls class.
 *
 * SBAv3D controls consist of a text control for SBAv3D
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class SBAv3DControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mSBAv3DParBox;
    wxSizer*          mSBAv3DParBoxSizer;
    wxFlexGridSizer*  mSBAv3DParSizer;

    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxStaticText*     mPararellST;     ///< overlay checkbox label
  
    wxStaticText*     mSBAv3DST;       //static text for SBAv3D
    wxTextCtrl*       mSBAv3DTC;

public:
    /** \brief SBAv3DControls ctor. */
    SBAv3DControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int SBAv3DID, const int SBAv3D_PararellID, int SBAv3D)

    {
        mBottomSizer = bottomSizer;
        mSBAv3DParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSBAv3DParBox );
        mSBAv3DParBoxSizer = new wxStaticBoxSizer( mSBAv3DParBox, wxHORIZONTAL );
        mSBAv3DParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mSBAv3DParSizer->SetMinSize( controlsWidth, 0 );
        mSBAv3DParSizer->AddGrowableCol( 0 );
        
        //SBAv3D
        mSBAv3DST = new wxStaticText( cp, -1, "Homogeneity:" );
        ::setColor( mSBAv3DST );
        
        mSBAv3DParSizer->Add( mSBAv3DST, 0, wxALIGN_RIGHT );
        
        wxString SBAv3DStr = wxString::Format("%d",SBAv3D);
        mSBAv3DTC = new wxTextCtrl( cp, SBAv3DID, SBAv3DStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mSBAv3DTC );
        
        mSBAv3DParSizer->Add( mSBAv3DTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mPararellST = new wxStaticText( cp, -1, "pararell:" );
        ::setColor( mPararellST );
        mSBAv3DParSizer->Add( mPararellST, 0, wxALIGN_RIGHT );
        
        //wxSizer*  gsLastRow = new wxGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mCB = new wxCheckBox( cp, SBAv3D_PararellID, "on/off" );
        ::setColor( mCB );
        mCB->SetValue( 0 );
		mSBAv3DParSizer->Add( mCB /*, 0, wxALIGN_RIGHT*/);

        
        mSBAv3DParBoxSizer->Add( mSBAv3DParSizer, 0, wxGROW|wxALL, 10 );
        
        mBottomSizer->Prepend( mSBAv3DParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SBAv3DControls dtor. */
    ~SBAv3DControls ( void ) 
	{
		if( mSBAv3DTC != NULL )
			delete mSBAv3DTC;
		if( mSBAv3DST != NULL )
			delete mSBAv3DST;
		mSBAv3DParSizer->Clear( true );
		mSBAv3DParBoxSizer->Clear( true );
		mSBAv3DParBoxSizer->Remove( mSBAv3DParSizer );
		mBottomSizer->Remove( mSBAv3DParBoxSizer );
    }
};

#endif
