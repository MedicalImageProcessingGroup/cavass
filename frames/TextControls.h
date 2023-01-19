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
 * \file   TextControls.h
 * \brief  Definition and implementation of TextControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __TextControls_h
#define __TextControls_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

/**
 * \brief Definition and implementation of TextControls class.
 *
 * Set index controls consist of a control box with sliders for slide
 * number and scale, a check box for turning on and off overlay, and
 * a button to match the slice number.
 *
 * Note: Callbacks are handled by the caller.
 */
class TextControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxStaticBox*      mTextBox;
    wxFlexGridSizer*  mFgs;
    wxSizer*          mTextSizer;
//	wxListCtrl*       mList;
public:
	wxListBox*        mList;

    /** \brief TextControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title          is the title for the control area/box.
     *  \param currentSlice   is the current slice number.
     *  \param numberOfSlices is the number of slices.
     *  \param sliceSliderID  is the ID of the slice slider.
     *  \param scaleSliderID  is the ID of the scale slider.
     *  \param currentScale   is the current scale value.
     *  \param matchIndexID   is the match index button.
     */
    TextControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title )
    {
        mBottomSizer = bottomSizer;
        mTextBox = new wxStaticBox( cp, -1, title );
        ::setColor( mTextBox );
        mTextSizer = new wxStaticBoxSizer( mTextBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

		mList = new wxListBox( cp, wxID_ANY, wxDefaultPosition, wxSize(controlsWidth,100) );
		::setColor( mList );
		mFgs->Add( mList, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );

        mTextSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                if (mList!=NULL)
                    mList->SetToolTip( "measurements" );
            #endif
        #endif
        mBottomSizer->Prepend( mTextSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) previous and next buttons might change
     *  the current slice, the following method will allow an update of
     *  the slice slider.
     *  \param currenSlice must be in [0..slices-1].
     */
    void insert ( const wxString& t ) {  
		//mList->InsertItem( mCount++, t );
		mList->Append( t );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief TextControls dtor. */
    ~TextControls ( void ) {
        mFgs->Clear( true );
        mTextSizer->Clear( true );
        
        mTextSizer->Remove( mFgs );
        mBottomSizer->Remove( mTextSizer );
    }
};

#endif
