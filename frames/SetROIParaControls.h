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
 * \file   SetROIParaControls.h
 * \brief  Definition and implementation of SetROIParaControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SetROIParaControls_h
#define __SetROIParaControls_h

/**
 * \brief Definition and implementation of SetROIParaControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */
class SetROIParaControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSetIndexBox;

	wxTextCtrl*       mSS1;
	wxTextCtrl*       mES2;
	wxTextCtrl*       mSS3;
	wxTextCtrl*       mES4;
	wxTextCtrl*       mSS5;
	wxTextCtrl*       mES6;
	wxTextCtrl*       mSS7;
	wxTextCtrl*       mES8;
    wxStaticText*     mSt1;
	wxStaticText*     mSt2;
	wxStaticText*     mSt3;
	wxStaticText*     mSt4;
	wxStaticText*     mSt5;
	wxStaticText*     mSt6;
	wxStaticText*     mSt7;
	wxStaticText*     mSt8;

    wxSizer*          mSetIndexSizer;
    
public:
    /** \brief SetROIParaControls ctor. */
    SetROIParaControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,
		const int currentLocX, const int nMaxLocX, const int LocXID,
        const int currentLocY, const int nMaxLocY, const int LocYID,
		const int currentWidth, const int nWidth, const int WinWidthID,
		const int currentHeight,const int nHeight, const int WinHeightID,
		const int currentStartSlice, const int numberOfSlices,
          const int StartsliceSliderID,
		const int currentEndSlice, const int EndsliceSliderID,
		const int currentStartVolume, const int numberOfVolumes,
		  const int StartvolumeSliderID,
		const int currentEndVolume, const int EndvolumeSliderID )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 4, 2, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

		//---Loc X---
		 mSt1 = new wxStaticText( cp, -1, "Loc X:" );
        ::setColor( mSt1 );
        mFgs->Add( mSt1, 0, wxALIGN_LEFT );
        mSS1 = new wxTextCtrl( cp, LocXID, wxString::Format("%d",currentLocX));
        ::setColor( mSS1 );
        mFgs->Add( mSS1, 0, wxGROW|wxLEFT|wxRIGHT );
        //---------------------------

	    //---Loc Y---
        mSt2 = new wxStaticText( cp, -1, "Loc Y:" );
        ::setColor( mSt2 );
        mFgs->Add( mSt2, 0, wxALIGN_LEFT );
        mES2 = new wxTextCtrl( cp, LocYID, wxString::Format("%d",currentLocY));
        ::setColor( mES2 );
        mFgs->Add( mES2, 0, wxGROW|wxLEFT|wxRIGHT );

		//---Width---
		 mSt5 = new wxStaticText( cp, -1, "Width:" );
        ::setColor( mSt5 );
        mFgs->Add( mSt5, 0, wxALIGN_LEFT );
        mSS5 = new wxTextCtrl( cp, WinWidthID, wxString::Format("%d",currentWidth));
        ::setColor( mSS5 );
        mFgs->Add( mSS5, 0, wxGROW|wxLEFT|wxRIGHT );
        //---------------------------
     
	   //---Height---
        mSt6 = new wxStaticText( cp, -1, "Height:" );
        ::setColor( mSt6 );
        mFgs->Add( mSt6, 0, wxALIGN_LEFT );
        mES6 = new wxTextCtrl( cp, WinHeightID, wxString::Format("%d",currentHeight));
        ::setColor( mES6 );
        mFgs->Add( mES6, 0, wxGROW|wxLEFT|wxRIGHT );

		 //---Starting slice---
		 mSt3 = new wxStaticText( cp, -1, "Starting slice:" );
        ::setColor( mSt3 );
        mFgs->Add( mSt3, 0, wxALIGN_LEFT );
        mSS3 = new wxTextCtrl( cp, StartsliceSliderID, wxString::Format("%d",currentStartSlice));
        ::setColor( mSS3 );
        mFgs->Add( mSS3, 0, wxGROW|wxLEFT|wxRIGHT );
        //---------------------------
     
	   //---Ending slice---
        mSt4 = new wxStaticText( cp, -1, "Ending slice:" );
        ::setColor( mSt4 );
        mFgs->Add( mSt4, 0, wxALIGN_LEFT );
        mES4 = new wxTextCtrl( cp, EndsliceSliderID, wxString::Format("%d",currentEndSlice));
        ::setColor( mES4 );
        mFgs->Add( mES4, 0, wxGROW|wxLEFT|wxRIGHT );

		if (numberOfVolumes > 1)
		{
		  //---Starting volume---
		  mSt7 = new wxStaticText( cp, -1, "Starting volume:" );
          ::setColor( mSt7 );
          mFgs->Add( mSt7, 0, wxALIGN_LEFT );
          mSS7 = new wxTextCtrl( cp, StartvolumeSliderID, wxString::Format("%d",currentStartVolume));
          ::setColor( mSS7 );
          mFgs->Add( mSS7, 0, wxGROW|wxLEFT|wxRIGHT );

	      //---Ending volume---
          mSt8 = new wxStaticText( cp, -1, "Ending volume:" );
          ::setColor( mSt8 );
          mFgs->Add( mSt8, 0, wxALIGN_LEFT );
          mES8 = new wxTextCtrl( cp, EndvolumeSliderID, wxString::Format("%d",currentEndVolume));
          ::setColor( mES8 );
          mFgs->Add( mES8, 0, wxGROW|wxLEFT|wxRIGHT );
		}
		else
		{
		  mSt7 = NULL;
		  mSS7 = NULL;
		  mSt8 = NULL;
		  mES8 = NULL;
		}

		mSetIndexSizer->Add(mFgs, 0, wxGROW);
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }   
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
    /** \brief SetROIParaControls dtor. */
    ~SetROIParaControls ( void ) {
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
		
		//mFgs->Remove( mFgsSlider );
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
	wxString getWidthValue ( void ) const {
		return mSS5->GetValue();
	}
	wxString getHeightValue ( void ) const {
		return mES6->GetValue();
	}
};

#endif
