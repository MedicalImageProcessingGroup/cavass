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
 * \file   MagnifyControls.h
 * \brief  Definition and implementation of MagnifyControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __MagnifyControls_h
#define  __MagnifyControls_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

/**
 * \brief Definition and implementation of MagnifyControls class.
 *
 * Magnify controls consist of a control box with a slider for scale.
 *
 * Note: Callbacks are handled by the caller.
 */
#define        magnifyIMin           10
#define        magnifyIMax         1000
#define        magnifyIDenominator  100

class MagnifyControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxStaticBox*      mMagnifyBox;
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxSlider*         mScale;         ///< scale slider
    wxStaticText*     mScaleText0;    ///< scale slider label
    wxStaticText*     mScaleText1;    ///< scale min value
    wxStaticText*     mScaleText2;    ///< scale current value
    wxStaticText*     mScaleText3;    ///< scale max value
    wxSizer*          mMagnifySizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_scale_value;
    double            mScaleZoom;
public:
    /** \brief MagnifyControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title          is the title for the control area/box.
     *  \param scaleSliderID  is the ID of the scale slider.
     *  \param currentScale   is the current scale value.
     *  \param zoomOutID      is the ID of the zoom out button (or -1).
     *  \param zoomInID       is the ID of the zoom in button (or -1).
     */
    MagnifyControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const int scaleSliderID, const double currentScale,
        const int zoomOutID, const int zoomInID )
    {
        mBottomSizer = bottomSizer;
        mMagnifyBox = new wxStaticBox( cp, -1, title );
        ::setColor( mMagnifyBox );
        mMagnifySizer = new wxStaticBoxSizer( mMagnifyBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //scale
        mScaleText0 = new wxStaticText( cp, -1, "scale:" );
        ::setColor( mScaleText0 );
        mFgs->Add( mScaleText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        if (zoomOutID != -1) {
            mZoomOut = new wxBitmapButton( cp, zoomOutID, wxBitmap( button_zoomout15_xpm ), wxDefaultPosition, wxSize(15,15) );
            ::setColor( mZoomOut );
            mFgsSlider->Add( mZoomOut, 0, wxALIGN_RIGHT );
        } else {
            mZoomOut = NULL;
            mFgsSlider->AddSpacer( -1 );
        }
        m_old_scale_value = 100;
        mScaleZoom = 1.0;
        mScale = new wxSlider( cp, scaleSliderID,
            (int)(currentScale*100+0.5), scaleIMin, scaleIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            //wxSL_HORIZONTAL|wxSL_AUTOTICKS, wxDefaultValidator,
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "scale" );
        ::setColor( mScale );
        mScale->SetPageSize( 10 );  //for incr/decr of 0.1
        mFgsSlider->Add( mScale, 0, wxGROW|wxLEFT|wxRIGHT );
        if (zoomOutID != -1) {
            mZoomIn = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15) );
            ::setColor( mZoomIn );
            mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
        } else {
            mZoomIn = NULL;
            mFgsSlider->AddSpacer( -1 );
        }
        
        wxString  s = wxString::Format( "%5.2f", scaleIMin/100.0 );
        mScaleText1 = new wxStaticText( cp, -1, s );
        ::setColor( mScaleText1 );
        mFgsSlider->Add( mScaleText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%5.2f", currentScale );
        mScaleText2 = new wxStaticText( cp, -1, s );
        ::setColor( mScaleText2 );
        mFgsSlider->Add( mScaleText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%5.2f", scaleIMax/100.0 );
        mScaleText3 = new wxStaticText( cp, -1, s );
        ::setColor( mScaleText3 );
        mFgsSlider->Add( mScaleText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mMagnifySizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        // - - - - - - - - - -
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mScale->SetToolTip( "scale" );
            #endif
        #endif
        mBottomSizer->Prepend( mMagnifySizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief get the current scale value. */
    int GetValue ( void ) const { return mScale->GetValue(); }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current scale value. */
    void setScaleText ( const wxString s ) {  mScaleText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief MagnifyControls dtor. */
    ~MagnifyControls ( void ) {
        mBottomSizer->Remove( mMagnifySizer );
        //delete mFgs;
        //delete mFgsSlider;
        //delete mMagnifyBox;
        delete mScale;
        delete mScaleText0;
        delete mScaleText1;
        delete mScaleText2;
        delete mScaleText3;
        if (mZoomIn  != NULL)    delete mZoomIn;
        if (mZoomOut != NULL)    delete mZoomOut;
    }
};

#endif
