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
 * \file   ScaleADControls.h
 * \brief  Definition and implementation of ScaleADControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ScaleADControls_h
#define __ScaleADControls_h

/**
 * \brief Definition and implementation of ScaleADControls class.
 *
 * ScaleAD controls consist of a text control for Homogeneity and iterations
 *
 *
 * Note: Callbacks are handled by the caller.
 */
class ScaleADControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mScaleADParBox;
    wxSizer*          mScaleADParBoxSizer;
    wxFlexGridSizer*  mScaleADParSizer;

    wxStaticText*     mHomogeneityST;       //static text for Homogeneity
    wxTextCtrl*       mHomogeneityTC;

    wxStaticText*     mIterationsST;       //static text for iterations
    wxTextCtrl*       mIterationsTC;


public:
    /** \brief ScaleADControls ctor. */
    ScaleADControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
		      const int HomogeneityID,const int iterationsID, int nHomogen, int iterations)

    {
        mBottomSizer = bottomSizer;
        mScaleADParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mScaleADParBox );
        mScaleADParBoxSizer = new wxStaticBoxSizer( mScaleADParBox, wxHORIZONTAL );
        mScaleADParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mScaleADParSizer->SetMinSize( controlsWidth, 0 );
        mScaleADParSizer->AddGrowableCol( 0 );

        //Homogeneity
        mHomogeneityST = new wxStaticText( cp, -1, "Homogeneity:" );
        ::setColor( mHomogeneityST );

		mScaleADParSizer->Add( mHomogeneityST, 0, wxALIGN_RIGHT );

		wxString HomogeneityStr = wxString::Format("%d",nHomogen);
		mHomogeneityTC = new wxTextCtrl( cp, HomogeneityID, HomogeneityStr,wxDefaultPosition, wxDefaultSize);
		::setColor( mHomogeneityTC );

		mScaleADParSizer->Add( mHomogeneityTC, 0, wxGROW|wxLEFT|wxRIGHT );

		//iterations

		mIterationsST = new wxStaticText( cp, -1, "Iterations:" );
		::setColor( mIterationsST );

		mScaleADParSizer->Add( mIterationsST, 0, wxALIGN_RIGHT );

		wxString iterStr = wxString::Format("%d",iterations);
		mIterationsTC = new wxTextCtrl( cp, iterationsID, iterStr,wxDefaultPosition, wxDefaultSize);		
		::setColor( mIterationsTC );

		mScaleADParSizer->Add( mIterationsTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mScaleADParBoxSizer->Add( mScaleADParSizer, 0, wxGROW|wxALL, 10 );

		mBottomSizer->Prepend( mScaleADParBoxSizer, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Layout();
		cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief ScaleADControls dtor. */
    ~ScaleADControls ( void ) 
	{
        mBottomSizer->Remove( mScaleADParBoxSizer );
		if( mHomogeneityTC != NULL )
			delete mHomogeneityTC;
		if( mHomogeneityST != NULL )
			delete mHomogeneityST;        
		if( mIterationsTC != NULL )
			delete mIterationsTC;
		if( mIterationsST != NULL )
			delete mIterationsST;

		/*mScaleADParSizer->Clear( true );
		mScaleADParBoxSizer->Clear( true );
        mScaleADParBoxSizer->Remove( mScaleADParSizer );
		mBottomSizer->Remove( mScaleADParBoxSizer );
      */  

     //   delete mScaleADParBox;
    }
};

#endif
