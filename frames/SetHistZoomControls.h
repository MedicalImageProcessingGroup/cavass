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
 * \file   SetHistZoomControls.h
 * \brief  Definition and implementation of SetHistZoomControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2008
 *
 * 
 */
//======================================================================
#ifndef __SetHistZoomControls_h
#define __SetHistZoomControls_h

#define resolutionIMin 0
#define resolutionIMax 1000
#define histZoomIMin 1
#define histZoomIMax 1000


class  HistZoomControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxStaticBox*      mHistZoomBox;
    wxSlider*         mHistZoom;         ///< histogram zoom slider
    wxStaticText*     mHistZoomText0;    ///< histogram zoom slider label
    wxStaticText*     mHistZoomText1;    ///< histogram zoom min value
    wxStaticText*     mHistZoomText2;    ///< histogram zoom current value
    wxStaticText*     mHistZoomText3;    ///< histogram zoom max value
    wxStaticBoxSizer* mHistZoomSizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_histZoom_value;
    double            mHistZoomZoom;
public:
    /** \brief HistZoomControls ctor. */
    HistZoomControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, int histZoomSliderID, double currentHistZoom )
    {
        mBottomSizer = bottomSizer;
        mHistZoomBox = new wxStaticBox( cp, -1, title );
        ::setColor( mHistZoomBox );
        mHistZoomSizer = new wxStaticBoxSizer( mHistZoomBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //histogram zoom
        mHistZoomText0 = new wxStaticText( cp, -1, "histogram zoom:" );
        ::setColor( mHistZoomText0 );
        mFgs->Add( mHistZoomText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        mZoomOut = new wxBitmapButton( cp, ThresholdFrame::ID_ZOOM_OUT,
			wxBitmap(button_zoomout15_xpm), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut );
        mFgsSlider->Add( mZoomOut, 0, wxALIGN_RIGHT );
        m_old_histZoom_value = 100;
        mHistZoomZoom = 1.0;
        mHistZoom = new wxSlider( cp, histZoomSliderID,
            (int)(currentHistZoom*100+0.5), histZoomIMin, histZoomIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator, "histogram zoom" );
        ::setColor( mHistZoom );
        mHistZoom->SetPageSize( 5 );
        mFgsSlider->Add( mHistZoom, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn = new wxBitmapButton( cp, ThresholdFrame::ID_ZOOM_IN,
			wxBitmap(button_zoomin15_xpm), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn );
        mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
        
        wxString  s = wxString::Format( "%5.2f", histZoomIMin/100.0 );
        mHistZoomText1 = new wxStaticText( cp, -1, s );
        ::setColor( mHistZoomText1 );
        mFgsSlider->Add( mHistZoomText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%5.2f", currentHistZoom );
        mHistZoomText2 = new wxStaticText( cp, -1, s );
        ::setColor( mHistZoomText2 );
        mFgsSlider->Add( mHistZoomText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%5.2f", histZoomIMax/100.0 );
        mHistZoomText3 = new wxStaticText( cp, -1, s );
        ::setColor( mHistZoomText3 );
        mFgsSlider->Add( mHistZoomText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mHistZoomSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );


        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mHistZoom->SetToolTip(       "histogram zoom" );
            #endif
        #endif
        mBottomSizer->Prepend( mHistZoomSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current histogram zoom value. */
    void setHistZoomText ( const wxString s ) { mHistZoomText2->SetLabel(s); }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief HistZoomControls dtor. */
    ~HistZoomControls ( void ) {
        mBottomSizer->Detach( mHistZoomSizer );
		mFgsSlider->Detach(mHistZoomText3);
		mFgsSlider->Detach(mHistZoomText2);
		mFgsSlider->Detach(mHistZoomText1);
        delete mHistZoomText3;
        delete mHistZoomText2;
        delete mHistZoomText1;
		mFgsSlider->Detach(mZoomIn);
        delete mZoomIn;
		mFgsSlider->Detach(mHistZoom);
        delete mHistZoom;
		mFgsSlider->Detach(mZoomOut);
        delete mZoomOut;
		mFgs->Detach(mFgsSlider);
		delete mFgsSlider;
		mFgs->Detach(mHistZoomText0);
        delete mHistZoomText0;
		mHistZoomSizer->Detach(mFgs);
		delete mFgs;
//@        delete mHistZoomSizer;
        delete mHistZoomBox;
    }
};
