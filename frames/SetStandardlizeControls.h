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
 * \file   SetStandardlizeControls.h
 * \brief  Definition and implementation of SetLowHighControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * The world is so beautiful that I can not help stopping smile.
 */
//======================================================================
#ifndef __SetStandardlizeControl_h
#define __SetStandardlizeControl_h

/**
 * \brief Definition and implementation of SetLowHighControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */
class SetLowHighControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSetIndexBox;

	wxTextCtrl*       mLowCtrl;   ///< low %ile slider
	wxTextCtrl*       mHighCtrl;   ///< high %ile slider
	wxStaticText*     mSt3;       ///< low %ile slider label
	wxStaticText*     mSt4;       ///< high %ile slider label

    wxSizer*          mSetIndexSizer;

public:
    /** \brief SetLowHighControls ctor. */
    SetLowHighControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,  const float currentLowPctile, 
        const int LowPctileTextCtrlID, const float currentHighPctile, 
        const int HighPctileTextCtrlID  )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 4, 2, 2 );  //4 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

	    mSt3 = new wxStaticText( cp, -1, "Low %ile:" );
        ::setColor( mSt3 );
        mFgs->Add( mSt3, 0, wxALIGN_LEFT );
        mLowCtrl = new wxTextCtrl( cp, LowPctileTextCtrlID,
			wxString::Format("%.2f", currentLowPctile) );
        ::setColor( mLowCtrl );
        mFgs->Add( mLowCtrl, 0, wxGROW|wxLEFT|wxRIGHT,3 );

		mSt4 = new wxStaticText( cp, -1, "High %ile:" );
        ::setColor( mSt4 );
        mFgs->Add( mSt4, 0, wxALIGN_LEFT );
        mHighCtrl = new wxTextCtrl( cp, HighPctileTextCtrlID,
			wxString::Format("%.2f", currentHighPctile) );
        ::setColor( mHighCtrl );
        mFgs->Add( mHighCtrl, 0, wxGROW|wxLEFT|wxRIGHT,3 );

		mSetIndexSizer->Add(mFgs, 0, wxGROW);
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }   
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
    /** \brief SetLowHighControls dtor. */
    ~SetLowHighControls ( void ) {
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
		
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
};

#endif
