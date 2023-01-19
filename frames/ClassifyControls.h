/*
  Copyright 1993-2012 Medical Image Processing Group
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
 * \file   ClassifyControls.h
 * \brief  Definition and implementation of ClassifyControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ClassifyControls_h
#define __ClassifyControls_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

#define BRUSHMIN  1
#define BRUSHMAX  60
#define BRUSHINIT 2
#define THRESINIT 0
#define THRESMIN  0
#define THRESMAX  100

/**
 * \brief Definition and implementation of ClassifyControls class.
 *
 * Classify controls consist of a setting box for FuzzComp parameters
 *
 * Note: Callbacks are handled by the caller.
 */
#include  "FuzzCompFrame.h"

class ClassifyControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor! 
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer  *mFgsSlider1, *mFgsSlider2;
    wxStaticText*     mParallelST;     ///< parallel checkbox label
    wxCheckBox*       mCB;            ///< parallel on/off checkbox
    wxStaticBox*      mClassifyBox;
    wxComboBox*       mMode;          ///< Mode combo box
    wxStaticText*     mModeST;       ///< Mode combot box label
    wxComboBox*       mAdjacency;    ///< Adjacency combo box
    wxStaticText*     mAdjST;       ///< Adjacency combo box label
    wxComboBox*       mAffinity;    ///< Affinity combo box
    wxStaticText*     mAffST;       ///< Affinity combo box label

    wxSlider*         mThres;         ///< Threshold slider
    wxStaticText*     mThresText0;    ///< scale slider label
    wxStaticText*     mThresText1;    ///< scale min value
    wxStaticText*     mThresText2;    ///< scale current value
    wxStaticText*     mThresText3;    ///< scale max value

    wxSlider*         mBrush;         ///< Brush level slider
    wxStaticText*     mBrushText0;    ///< scale slider label
    wxStaticText*     mBrushText1;    ///< scale min value
    wxStaticText*     mBrushText2;    ///< scale current value
    wxStaticText*     mBrushText3;    ///< scale max value

    wxSizer*          mClassifySizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    
public:

    /** \brief RegistrationControls ctor. */
    ClassifyControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int modeID,const int adjacencyID,
        const int affinityID, const int brushSliderID, const int thresSliderID,
        const int paralelIndexID )
    {
        mBottomSizer = bottomSizer;
        mClassifyBox = new wxStaticBox( cp, -1, title );
        ::setColor( mClassifyBox );
        mClassifySizer = new wxStaticBoxSizer( mClassifyBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2,10, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth/2, 0 );
        mFgs->AddGrowableCol( 0 );
        //Mode
        mModeST = new wxStaticText( cp, -1, "Mode:" );
        ::setColor( mModeST );
        mFgs->Add( mModeST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices1;
        choices1.Add(wxT("Tracking" ));
        choices1.Add(wxT("Training"));
        
        
        mMode = new wxComboBox( cp, modeID, wxT("Training" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices1,wxCB_DROPDOWN,
            wxDefaultValidator,"Mode" );

       
            
        ::setColor( mMode );
        mFgs->Add( mMode, 0, wxGROW|wxLEFT|wxRIGHT );

        //Adjacency
        mAdjST = new wxStaticText( cp, -1, "Adjacency:" );
        ::setColor( mAdjST );
        mFgs->Add( mAdjST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices2;
        choices2.Add(wxT("Hard" ));
        choices2.Add(wxT("Fuzz"));
        
                
        mAdjacency = new wxComboBox( cp, adjacencyID, wxT("Hard" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices2,wxCB_DROPDOWN,
            wxDefaultValidator,"Adjacency" );
        

        ::setColor( mAdjacency);
        mFgs->Add( mAdjacency, 0, wxGROW|wxLEFT|wxRIGHT );

       //Affinity
        mAffST = new wxStaticText( cp, -1, "Affinity:" );
        ::setColor( mAffST );
        mFgs->Add( mAffST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices3;
        choices3.Add(wxT("Histogram" ));
        choices3.Add(wxT("Covariance"));
        choices3.Add(wxT("Parameter"));

                
        mAffinity = new wxComboBox( cp, affinityID, wxT("Parameter" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"Affinity" );
        
   
        ::setColor( mAffinity);
        mFgs->Add( mAffinity, 0, wxGROW|wxLEFT|wxRIGHT );
		mAffinity->Enable(false); // until other affinities implemented

        //Brush size slider
        mBrushText0 = new wxStaticText( cp, -1, "Brush size:" );
        ::setColor( mBrushText0 );
        mFgs->Add( mBrushText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider1 = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider1->AddGrowableCol( 1 );

        mBrush = new wxSlider( cp, brushSliderID,
            BRUSHINIT, BRUSHMIN, BRUSHMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Brush" );
        ::setColor( mBrush );
        mBrush->SetPageSize( 5 );
		mFgsSlider1->AddStretchSpacer();
        mFgsSlider1->Add( mBrush, 0, wxGROW|wxLEFT|wxRIGHT );
		mFgsSlider1->AddStretchSpacer();

        wxString  s = wxString::Format( "%1d", BRUSHMIN );
        mBrushText1 = new wxStaticText( cp, -1, s );
        ::setColor( mBrushText1 );
        mFgsSlider1->Add( mBrushText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", BRUSHINIT );//current brush size
        mBrushText2 = new wxStaticText( cp, -1, s );
        ::setColor( mBrushText2 );
        mFgsSlider1->Add( mBrushText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", BRUSHMAX);
        mBrushText3 = new wxStaticText( cp, -1, s );
        ::setColor( mBrushText3 );
        mFgsSlider1->Add( mBrushText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider1, 0, wxGROW|wxLEFT|wxRIGHT );
        

        //Threshold slider
        mThresText0 = new wxStaticText( cp, -1, "Threshold level:" );
        ::setColor( mThresText0 );
        mFgs->Add( mThresText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider2 = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider2->AddGrowableCol( 1 );

        mThres = new wxSlider( cp, thresSliderID,
            THRESINIT, THRESMIN, THRESMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Threshold" );
        ::setColor( mThres );
        mThres->SetPageSize( 5 );
		mFgsSlider2->AddStretchSpacer();
        mFgsSlider2->Add( mThres, 0, wxGROW|wxLEFT|wxRIGHT );
		mFgsSlider2->AddStretchSpacer();

        s = wxString::Format( "%1d", THRESMIN );
        mThresText1 = new wxStaticText( cp, -1, s );
        ::setColor( mThresText1 );
        mFgsSlider2->Add( mThresText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", THRESINIT );//current histogram sampling rate
        mThresText2 = new wxStaticText( cp, -1, s );
        ::setColor( mThresText2 );
        mFgsSlider2->Add( mThresText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", THRESMAX);
        mThresText3 = new wxStaticText( cp, -1, s );
        ::setColor( mThresText3 );
        mFgsSlider2->Add( mThresText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider2, 0, wxGROW|wxLEFT|wxRIGHT );


        mClassifySizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        // - - - - - - - - - -
        mParallelST = NULL;
        mCB = NULL;

        mBottomSizer->Prepend( mClassifySizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current brush size. */
    void setBrushText ( const wxString s ) {  mBrushText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    /** \brief set the text of the current threshold value. */
    void setThresText ( const wxString s ) {  mThresText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int getAffinity    ( void ) const { return mAffinity -> GetCurrentSelection ( );     }
    int getAdjacency ( void ) const { return mAdjacency -> GetCurrentSelection ( ); }
    int getMode  ( void ) const { return mMode -> GetCurrentSelection ( );  }
    bool getOnParallel    ( void ) const { return mCB -> IsChecked ( );                       }
    int getThresLevel     ( void ) const { return mThres -> GetValue ( );                     }
    int getBrushSize      ( void ) const { return mBrush -> GetValue ( );                      }

    /** \brief RegistrationControls dtor. */
    ~ClassifyControls ( void ) {
        mBottomSizer->Remove( mClassifySizer );
        mBottomSizer->Layout(); 
        delete mCB;
        delete mFgs;
        delete mFgsSlider1;
		delete mFgsSlider2;
        delete mClassifyBox;
        delete mMode;
        delete mModeST;
        delete mAdjacency;
        delete mAdjST;
        delete mAffinity;
        delete mAffST;
        delete mBrush;
        delete mBrushText0;
        delete mBrushText1;
        delete mBrushText2;
        delete mBrushText3;
        delete mThres;
        delete mThresText0;
        delete mThresText1;
        delete mThresText2;
        delete mThresText3;
    }
};

#endif
