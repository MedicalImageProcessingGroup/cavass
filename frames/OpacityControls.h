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
 * \file   OpacityControls.h
 * \brief  Definition and implementation of OpacityControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __OpacityControls_h
#define  __OpacityControls_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

/**
 * \brief Definition and implementation of OpacityControls class.
 *
 * Opacity controls consist of a control box with a slider for opacity.
 *
 * Note: Callbacks are handled by the caller.
 */
#define  opacityIMin        0
#define  opacityIMax     1000
#define  opacityIFactor    10
#define  opacityFormat   "%4.1f"

class OpacityControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxStaticBox*      mOpacityBox;
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxSlider*         mOpacity;         ///< opacity slider
    wxStaticText*     mOpacityText0;    ///< opacity slider label
    wxStaticText*     mOpacityText1;    ///< opacity min value
    wxStaticText*     mOpacityText2;    ///< opacity current value
    wxStaticText*     mOpacityText3;    ///< opacity max value
    wxSizer*          mOpacitySizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_opacity_value;
    double            mOpacityZoom;
public:
    /** \brief OpacityControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title          is the title for the control area/box.
     *  \param opacitySliderID  is the ID of the opacity slider.
     *  \param currentOpacity   is the current opacity value.
     *  \param zoomOutID      is the ID of the zoom out button (or -1).
     *  \param zoomInID       is the ID of the zoom in button (or -1).
     */
    OpacityControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const int opacitySliderID, const double currentOpacity,
        const int zoomOutID, const int zoomInID )
    {
        mBottomSizer = bottomSizer;
        mOpacityBox = new wxStaticBox( cp, -1, title );
        ::setColor( mOpacityBox );
        mOpacitySizer = new wxStaticBoxSizer( mOpacityBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //opacity
        mOpacityText0 = new wxStaticText( cp, -1, "opacity:" );
        ::setColor( mOpacityText0 );
        mFgs->Add( mOpacityText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
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
        m_old_opacity_value = 100;
        mOpacityZoom = 1.0;
        mOpacity = new wxSlider( cp, opacitySliderID,
            (int)(currentOpacity*opacityIFactor+0.5), opacityIMin, opacityIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            //wxSL_HORIZONTAL|wxSL_AUTOTICKS, wxDefaultValidator,
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "opacity" );
        ::setColor( mOpacity );
        mOpacity->SetPageSize( 10 );
        mFgsSlider->Add( mOpacity, 0, wxGROW|wxLEFT|wxRIGHT );
        if (zoomOutID != -1) {
            mZoomIn = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15) );
            ::setColor( mZoomIn );
            mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
        } else {
            mZoomIn = NULL;
            mFgsSlider->AddSpacer( -1 );
        }
        
        wxString  s = wxString::Format( opacityFormat, opacityIMin/(double)opacityIFactor );
        mOpacityText1 = new wxStaticText( cp, -1, s );
        ::setColor( mOpacityText1 );
        mFgsSlider->Add( mOpacityText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( opacityFormat, currentOpacity );
        mOpacityText2 = new wxStaticText( cp, -1, s );
        ::setColor( mOpacityText2 );
        mFgsSlider->Add( mOpacityText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( opacityFormat, opacityIMax/(double)opacityIFactor );
        mOpacityText3 = new wxStaticText( cp, -1, s );
        ::setColor( mOpacityText3 );
        mFgsSlider->Add( mOpacityText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mOpacitySizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        // - - - - - - - - - -
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mOpacity->SetToolTip( "opacity" );
            #endif
        #endif
        mBottomSizer->Prepend( mOpacitySizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief get the current opacity value. */
    int GetValue ( void ) const { return mOpacity->GetValue(); }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current opacity value. */
    void setOpacityText ( const wxString s ) {  mOpacityText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief OpacityControls dtor. */
    ~OpacityControls ( void ) {
        mBottomSizer->Remove( mOpacitySizer );
        //delete mFgs;
        //delete mFgsSlider;
        delete mOpacityBox;
        delete mOpacity;
        delete mOpacityText0;
        delete mOpacityText1;
        delete mOpacityText2;
        delete mOpacityText3;
        if (mZoomIn  != NULL)    delete mZoomIn;
        if (mZoomOut != NULL)    delete mZoomOut;
    }
};

#endif
