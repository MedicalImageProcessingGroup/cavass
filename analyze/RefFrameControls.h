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
 * \file   RefFrameControls.h
 * \brief  Definition and implementation of RefFrameControls class.
 * \author Xinjian, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __RefFrameControls_h
#define __RefFrameControls_h

/**
 * \brief Definition and implementation of RefFrameControls class.
 *
 * RefFrame controls consist of a control box with sliders for Instance, green, and
 * blue.
 *
 * Note: Callbacks are handled by the caller.
 */
class RefFrameControls 
{
    bool               mHorizontal;   ///< orientation of sliders
    wxSizer*           mBottomSizer;  //DO NOT DELETE in dtor!
    wxStaticBox*       mRefFrameBox;
    wxStaticBoxSizer*  mRefFrameSizer;
    wxFlexGridSizer*   mFgs;
    wxSlider           *mInstance;
    wxStaticText       *mInstanceText0;
public:
    /** \brief RefFrameControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title         is the title for the control area/box.
     *  \param currentInstance    is the current Instance value.
     */
    RefFrameControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title,
        const int currentInstance, 
        const int RefFrameliderID,
		const int RefFrameMax,
        const bool horizontal=true )
    {   
        mHorizontal = horizontal;
        mBottomSizer = bottomSizer;
        mRefFrameBox = new wxStaticBox( cp, -1, title );
        ::setColor( mRefFrameBox );
        mRefFrameSizer = new wxStaticBoxSizer( mRefFrameBox, wxHORIZONTAL );
        if (mHorizontal)
            mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        else
            mFgs = new wxFlexGridSizer( 6, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        if (mHorizontal) 
		{
            mFgs->AddGrowableCol( 0 );          
        }

        //Instance- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        mInstanceText0 = new wxStaticText( cp, -1, "RefFrame:" );
        ::setColor( mInstanceText0 );
        mFgs->Add( mInstanceText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        if (mHorizontal)
            mInstance = new wxSlider( cp, RefFrameliderID, currentInstance,
                1, RefFrameMax, wxDefaultPosition, wxSize(sliderWidth, -1),
                wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator, "RefFrame" );
        else
            mInstance = new wxSlider( cp, RefFrameliderID, currentInstance,
                1, RefFrameMax, wxDefaultPosition, wxSize(-1, -1),
                wxSL_VERTICAL|wxSL_LABELS|wxSL_AUTOTICKS|wxSL_INVERSE,
                wxDefaultValidator, "RefFrame" );
        ::setColor( mInstance );
        mInstance->SetPageSize( 5 );
        mFgs->Add( mInstance, 0, wxGROW|wxLEFT|wxRIGHT );        
        
        mRefFrameSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );        
        mBottomSizer->Prepend( mRefFrameSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
   
   
	void UpdateFrameMax( wxPanel* cp, wxSizer* bottomSizer, const int RefFrameMax )
	{	
		mInstance->SetRange( 1, RefFrameMax );
		mInstance->SetValue(1);	

	}

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief RefFrameControls dtor. */
    ~RefFrameControls ( void ) 
	{
        
		
		 if( mInstanceText0 != NULL )
		 {
	//		 delete mInstanceText0;        
			 mInstanceText0 = NULL;
		 }		
	
		 if( mRefFrameBox != NULL )
		 {
			// delete mRefFrameBox;
			 mRefFrameBox = NULL;
		 }
		  if( mFgs != NULL )
		 {
		//	 delete mFgs;
			 mFgs = NULL;
		 }

		   if( mInstance != NULL )
		 {
	//		 delete mInstance;
			 mInstance = NULL;
		 }

	//	 mBottomSizer->Remove( mRefFrameSizer );		

		 if( mRefFrameSizer != NULL )
		 {
			// delete mRefFrameSizer;
			 mRefFrameSizer = NULL;
		 }


	}
};

#endif
