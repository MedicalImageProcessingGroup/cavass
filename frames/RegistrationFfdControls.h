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
 * \file   RegistrationFfdControls.h
 * \brief  Definition and implementation of RegistrationFfdControls class.
 * (based on RegistrationControls.h)
 * \author Xiaofen Zheng, Ph.D.
 *
 * Copyright: (C) 2008, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __RegistrationFfdControls_h
#define __RegistrationFfdControls_h
#include  "OverlayFrame.h"

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

#define CONVMIN  0
#define CONVMAX  50
#define CONVINIT 10

#define PYRA2INIT 3
#define PYRA2MIN  1
#define PYRA2MAX  7

#define STEPMIN  5
#define STEPMAX  100 ///////////?
#define STEPINIT 10

#define SPACEMIN  5
#define SPACEMAX  30
#define SPACEINIT 15

/**
 * \brief Definition and implementation of RegistrationFfdControls class.
 *
 * Registration controls consist of a setting box for registration parameters
 *
 * Note: Callbacks are handled by the caller.
 */
class RegistrationFfdControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor! 
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxStaticText*     mParallelST;     ///< parallel checkbox label
    wxCheckBox*       mCB;            ///< parallel on/off checkbox

    wxStaticText*     mParST;     ///< deformation field output checkbox label
    wxCheckBox*       mPCB;            ///< deformation field output on/off checkbox

    wxStaticBox*      mRegistrationBox;
    wxComboBox*       mSimilarity;    ///< Similarity combo box
    wxStaticText*     mSimilST;       ///< Similarity combo box label

    wxSlider*         mConv;         ///< Converge tolerance slider
    wxStaticText*     mConvText0;    ///< scale slider label
    wxStaticText*     mConvText1;    ///< scale min value
    wxStaticText*     mConvText2;    ///< scale current value
    wxStaticText*     mConvText3;    ///< scale max value

    wxSlider*         mPyra2;         ///< Pyramid level slider
    wxStaticText*     mPyra2Text0;    ///< scale slider label
    wxStaticText*     mPyra2Text1;    ///< scale min value
    wxStaticText*     mPyra2Text2;    ///< scale current value
    wxStaticText*     mPyra2Text3;    ///< scale max value

    wxSlider*         mStep;         ///< Optimize step slider
    wxStaticText*     mStepText0;    ///< scale slider label
    wxStaticText*     mStepText1;    ///< scale min value
    wxStaticText*     mStepText2;    ///< scale current value
    wxStaticText*     mStepText3;    ///< scale max value

    wxSlider*         mSpace;         ///< control spacing slider
    wxStaticText*     mSpaceText0;    ///< scale slider label
    wxStaticText*     mSpaceText1;    ///< scale min value
    wxStaticText*     mSpaceText2;    ///< scale current value
    wxStaticText*     mSpaceText3;    ///< scale max value


    wxSizer*          mRegistrationSizer;
    wxBitmapButton*   mZoomIn[4];        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut[4];       ///< \todo zoomable sliders
    wxPanel* cp; //TESTING
public:

    /** \brief RegistrationFfdControls ctor. */
    RegistrationFfdControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int similarityID, const int ConvSliderID, const int pyra2SliderID,const int StepSliderID, const int SpaceSliderID, 
        const int parallelIndexID, const int parIndexID )
    {
        mBottomSizer = bottomSizer;
        mRegistrationBox = new wxStaticBox( cp, -1, title );
        ::setColor( mRegistrationBox );
        mRegistrationSizer = new wxStaticBoxSizer( mRegistrationBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2,2, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth/2, 0 );
        mFgs->AddGrowableCol( 0 );

       //Similarity
        mSimilST = new wxStaticText( cp, -1, "Similarity:" );
        ::setColor( mSimilST );
        mFgs->Add( mSimilST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        mSimilarity = new wxComboBox( cp, similarityID, "MI", wxDefaultPosition,
            wxSize(sliderWidth, -1), 0, NULL, wxCB_DROPDOWN,
            wxDefaultValidator,"Similarity" );
   
        ::setColor( mSimilarity);
        mFgs->Add( mSimilarity, 0, wxGROW|wxLEFT|wxRIGHT );

        //Converge tolerance rate slider
        mConvText0 = new wxStaticText( cp, -1, "Converge tolerance:" );
        ::setColor( mConvText0 );
        mFgs->Add( mConvText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        mZoomOut[0] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut[0] );
        mFgsSlider->Add( mZoomOut[0], 0, wxALIGN_RIGHT );
        
        mConv = new wxSlider( cp, ConvSliderID,
            CONVINIT, CONVMIN, CONVMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Converge" );
        ::setColor( mConv );
        mConv->SetPageSize( 5 );
        mFgsSlider->Add( mConv, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn[0] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn[0] );
        mFgsSlider->Add( mZoomIn[0], 0, wxALIGN_LEFT );
        
        wxString  s = wxString::Format( "%1d", CONVMIN );
        mConvText1 = new wxStaticText( cp, -1, s );
        ::setColor( mConvText1 );
        mFgsSlider->Add( mConvText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", CONVINIT );//current Converge tolerance rate
        mConvText2 = new wxStaticText( cp, -1, s );
        ::setColor( mConvText2 );
        mFgsSlider->Add( mConvText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", CONVMAX);
        mConvText3 = new wxStaticText( cp, -1, s );
        ::setColor( mConvText3 );
        mFgsSlider->Add( mConvText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        

        //Pyramid level slider
        mPyra2Text0 = new wxStaticText( cp, -1, "Pyramid level:" );
        ::setColor( mPyra2Text0 );
        mFgs->Add( mPyra2Text0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 1 );
        mZoomOut[1] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut[1] );
        mFgsSlider->Add( mZoomOut[1], 0, wxALIGN_RIGHT );
        
        mPyra2 = new wxSlider( cp, pyra2SliderID,
            PYRA2INIT, PYRA2MIN, PYRA2MAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Pyramid" );
        ::setColor( mPyra2 );
        mPyra2->SetPageSize( 5 );
        mFgsSlider->Add( mPyra2, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn[1] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn[1] );
        mFgsSlider->Add( mZoomIn[1], 0, wxALIGN_LEFT );
        
        s = wxString::Format( "%1d", PYRA2MIN );
        mPyra2Text1 = new wxStaticText( cp, -1, s );
        ::setColor( mPyra2Text1 );
        mFgsSlider->Add( mPyra2Text1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", PYRA2INIT );
        mPyra2Text2 = new wxStaticText( cp, -1, s );
        ::setColor( mPyra2Text2 );
        mFgsSlider->Add( mPyra2Text2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", PYRA2MAX);
        mPyra2Text3 = new wxStaticText( cp, -1, s );
        ::setColor( mPyra2Text3 );
        mFgsSlider->Add( mPyra2Text3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );



        //Step size slider
        mStepText0 = new wxStaticText( cp, -1, "Step size:" );
        ::setColor( mStepText0 );
        mFgs->Add( mStepText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 1 );
        mZoomOut[2] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut[2] );
        mFgsSlider->Add( mZoomOut[2], 0, wxALIGN_RIGHT );
        
        mStep = new wxSlider( cp, StepSliderID,
            STEPINIT, STEPMIN, STEPMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Step" );
        ::setColor( mStep );
        mStep->SetPageSize( 5 );
        mFgsSlider->Add( mStep, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn[2] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn[2] );
        mFgsSlider->Add( mZoomIn[2], 0, wxALIGN_LEFT );
        
        s = wxString::Format( "%1d", STEPMIN );
        mStepText1 = new wxStaticText( cp, -1, s );
        ::setColor( mStepText1 );
        mFgsSlider->Add( mStepText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", STEPINIT );//current Step size
        mStepText2 = new wxStaticText( cp, -1, s );
        ::setColor( mStepText2 );
        mFgsSlider->Add( mStepText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", STEPMAX);
        mStepText3 = new wxStaticText( cp, -1, s );
        ::setColor( mStepText3 );
        mFgsSlider->Add( mStepText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        
	//Spacing of control points slider
        mSpaceText0 = new wxStaticText( cp, -1, "Control spacing:" );
        ::setColor( mSpaceText0 );
        mFgs->Add( mSpaceText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 1 );
        mZoomOut[3] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut[3] );
        mFgsSlider->Add( mZoomOut[3], 0, wxALIGN_RIGHT );
        
        mSpace = new wxSlider( cp, SpaceSliderID,
            SPACEINIT, SPACEMIN, SPACEMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Space" );
        ::setColor( mSpace );
        mSpace->SetPageSize( 5 );
        mFgsSlider->Add( mSpace, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn[3] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn[3] );
        mFgsSlider->Add( mZoomIn[3], 0, wxALIGN_LEFT );
        
        s = wxString::Format( "%1d", SPACEMIN );
        mSpaceText1 = new wxStaticText( cp, -1, s );
        ::setColor( mSpaceText1 );
        mFgsSlider->Add( mSpaceText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", SPACEINIT );//current control spacing
        mSpaceText2 = new wxStaticText( cp, -1, s );
        ::setColor( mSpaceText2 );
        mFgsSlider->Add( mSpaceText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", SPACEMAX);
        mSpaceText3 = new wxStaticText( cp, -1, s );
        ::setColor( mSpaceText3 );
        mFgsSlider->Add( mSpaceText3, 0, wxALIGN_LEFT );
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );

        mRegistrationSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        mParST = new wxStaticText( cp, -1, "Deformation field output:" );
        ::setColor( mParST );
        mFgs->Add( mParST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL );
        
        
        mPCB = new wxCheckBox( cp, RegisterFrame::ID_PAR2_INDEX, "on/off" );
        ::setColor( mPCB );
        mPCB->SetValue( 0 );
        mFgs->Add( mPCB, 0, wxALIGN_LEFT );
        
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mPCB->SetToolTip(  "deformation field output file on & off" );
                mParST->SetToolTip(   "deformation field output file on & off" );
            #endif
        #endif

	wxSizer*  gsLastRow = new wxGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mParallelST = new wxStaticText( cp, -1, "Parallel:" );
        ::setColor( mParallelST );
        gsLastRow->Add( mParallelST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL );
        
        
        mCB = new wxCheckBox( cp, RegisterFrame::ID_PARALLEL_INDEX, "on/off" );
        ::setColor( mCB );
        mCB->SetValue( 0 );
        gsLastRow->Add( mCB, 0, wxALIGN_LEFT );
        
        mFgs->Add( gsLastRow );
        
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mCB->SetToolTip(          "toggle parallel registration on & off" );
                mParallelST->SetToolTip(   "toggle parallel registration on & off" );
                //mSliceNumber->SetToolTip( "slice number" );
                //mScale->SetToolTip(       "scale" );
            #endif
        #endif
        mBottomSizer->Prepend( mRegistrationSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
	//END OF NEW CLASS
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) previous and next buttons might change
     *  the current slice, the following method will allow an update of
     *  the slice slider.
     */
    //void setSliceNo ( const int currentSlice ) {  
    //    mSliceNumber->SetValue( currentSlice );
    // }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current Converge tolerance sampling rate value. */
    void setConvText ( const wxString s ) {  mConvText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    

    /** \brief set the text of the current Converge tolerance rate value. */
    void setPyra2Text ( const wxString s ) {  mPyra2Text2->SetLabel( s );  }

void setStepText ( const wxString s ) {  mStepText2->SetLabel( s );  }
void setSpaceText ( const wxString s ) {  mSpaceText2->SetLabel( s );  }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int getSimilarity     ( void ) const { return mSimilarity -> GetCurrentSelection ( );     }
    bool getOnParallel    ( void ) const { return mCB -> IsChecked ( );                       }
    bool getOnPar         ( void ) const { return mPCB -> IsChecked ( );                       }
    int getConvLevel     ( void ) const { return mConv -> GetValue ( );                     }
    int getPyra2Depth      ( void ) const { return mPyra2 -> GetValue ( );                      }

    int getStepLevel     ( void ) const { return mStep -> GetValue ( );                     }
    int getSpaceLevel     ( void ) const { return mSpace -> GetValue ( );                     }


    /** \brief RegistrationFfdControls dtor. */
    ~RegistrationFfdControls ( void ) {
	delete mCB;
	delete mPCB;
        delete mParallelST;
	delete mParST;
        mFgs->Detach( mSimilarity );
        delete mSimilarity;
        mFgs->Detach( mSimilST );
        delete mSimilST;
        mFgsSlider->Detach( mConv );
        delete mConv;
        mFgsSlider->Detach( mConvText0 );
        delete mConvText0;
        mFgsSlider->Detach( mConvText1 );
        delete mConvText1;
        mFgsSlider->Detach( mConvText2 );
        delete mConvText2;
        mFgsSlider->Detach( mConvText3 );
        delete mConvText3;
        mFgsSlider->Detach( mPyra2 );
        delete mPyra2;
        mFgsSlider->Detach( mPyra2Text0 );
        delete mPyra2Text0;
        mFgsSlider->Detach( mPyra2Text1 );
        delete mPyra2Text1;
        mFgsSlider->Detach( mPyra2Text2 );
        delete mPyra2Text2;
        mFgsSlider->Detach( mPyra2Text3 );
        delete mPyra2Text3;

        mFgsSlider->Detach( mStep );
        delete mStep;
        mFgsSlider->Detach( mStepText0 );
        delete mStepText0;
        mFgsSlider->Detach( mStepText1 );
        delete mStepText1;
        mFgsSlider->Detach( mStepText2 );
        delete mStepText2;
        mFgsSlider->Detach( mStepText3 );
        delete mStepText3;
        mFgsSlider->Detach( mSpace );
        delete mSpace;
        mFgsSlider->Detach( mSpaceText0 );
        delete mSpaceText0;
        mFgsSlider->Detach( mSpaceText1 );
        delete mSpaceText1;
        mFgsSlider->Detach( mSpaceText2 );
        delete mSpaceText2;
        mFgsSlider->Detach( mSpaceText3 );
        delete mSpaceText3;

        mFgsSlider->Detach( mZoomIn[0] );
        delete mZoomIn[0];
        mFgsSlider->Detach( mZoomOut[0] );
        delete mZoomOut[0];
	mFgsSlider->Detach( mZoomIn[1] );
        delete mZoomIn[1];
        mFgsSlider->Detach( mZoomOut[1] );
        delete mZoomOut[1];
        mFgsSlider->Detach( mZoomIn[2] );
        delete mZoomIn[2];
        mFgsSlider->Detach( mZoomOut[2] );
        delete mZoomOut[2];
	mFgsSlider->Detach( mZoomIn[3] );
        delete mZoomIn[3];
        mFgsSlider->Detach( mZoomOut[3] );
        delete mZoomOut[3];

        mFgs->Remove( mFgsSlider );
        mRegistrationSizer->Remove( mFgs );
        mBottomSizer->Remove( mRegistrationSizer );

    }
};

#endif
