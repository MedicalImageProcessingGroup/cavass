/*
  Copyright 1993-2011, 2016 Medical Image Processing Group
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
 * \file   RegistrationControls.h
 * \brief  Definition and implementation of RegistrationControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __RegistrationControls_h
#define __RegistrationControls_h
#include  "OverlayFrame.h"

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

#define HISTMIN  0
#define HISTMAX  11
#define HISTINIT 5
#define PYRAINIT 1
#define PYRAMIN  0
#define PYRAMAX  5

/**
 * \brief Definition and implementation of RegistrationControls class.
 *
 * Registration controls consist of a setting box for registration parameters
 *
 * Note: Callbacks are handled by the caller.
 */
class RegistrationControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor! 
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxStaticText*     mParallelST;     ///< parallel checkbox label
    wxCheckBox*       mCB;            ///< parallel on/off checkbox

    wxStaticText*     mParST;     ///< parameter output checkbox label
    wxCheckBox*       mPCB;            ///< parameter on/off checkbox

    wxStaticBox*      mRegistrationBox;
    wxComboBox*       mTransformation; ///< Transformation combo box
    wxStaticText*     mTranfST;       ///< Transformation combot box label
    wxComboBox*       mInterpolation; ///< Interporlation combo box
    wxStaticText*     mInterST;       ///< Interpolation combo box label
    wxComboBox*       mSimilarity;    ///< Similarity combo box
    wxStaticText*     mSimilST;       ///< Similarity combo box label

    wxSlider*         mHisto;         ///< histogrom smapling slider
    wxStaticText*     mHistoText0;    ///< scale slider label
    wxStaticText*     mHistoText1;    ///< scale min value
    wxStaticText*     mHistoText2;    ///< scale current value
    wxStaticText*     mHistoText3;    ///< scale max value

    wxSlider*         mPyra;         ///< Pyramid level slider
    wxStaticText*     mPyraText0;    ///< scale slider label
    wxStaticText*     mPyraText1;    ///< scale min value
    wxStaticText*     mPyraText2;    ///< scale current value
    wxStaticText*     mPyraText3;    ///< scale max value

    wxSizer*          mRegistrationSizer;
    wxBitmapButton*   mZoomIn[2];        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut[2];       ///< \todo zoomable sliders
    wxPanel* cp; //TESTING
public:

    /** \brief RegistrationControls ctor. */
    RegistrationControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int transformationID,const int interpolationID,
        const int similarityID, const int histoSliderID, const int pyraSliderID,
        const int paralelIndexID, const int parIndexID )
    {
        mBottomSizer = bottomSizer;
        mRegistrationBox = new wxStaticBox( cp, -1, title );
        ::setColor( mRegistrationBox );
        mRegistrationSizer = new wxStaticBoxSizer( mRegistrationBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2,2, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth/2, 0 );
        mFgs->AddGrowableCol( 0 );
        //Tranformation
        mTranfST = new wxStaticText( cp, -1, "Transformation:" );
        ::setColor( mTranfST );
        mFgs->Add( mTranfST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices1;
	choices1.Add(wxT("Rigid" ));
        choices1.Add(wxT("Scale only"));
        choices1.Add(wxT("Homothetic (similarity)"));
        choices1.Add(wxT("Anisotropic scale"));
        choices1.Add(wxT("Volume preserving"));
        choices1.Add(wxT("Affine"));
		choices1.Add(wxT("Translation only"));
        
        mTransformation = new wxComboBox( cp, transformationID, wxT("Rigid" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices1,wxCB_READONLY,
            wxDefaultValidator,"Transformation" );

      //START OF NEW CLASS LIKE GRAYMAPWHATEVER 
            
        ::setColor( mTransformation );
        mFgs->Add( mTransformation, 0, wxGROW|wxLEFT|wxRIGHT );

        //Interpolation
        mInterST = new wxStaticText( cp, -1, "Interpolation:" );
        ::setColor( mTranfST );
        mFgs->Add( mInterST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices2;
	choices2.Add(wxT("Nearest Neighbours" ));
        choices2.Add(wxT("Linear"));
        choices2.Add(wxT("Cubic"));
                
        mInterpolation = new wxComboBox( cp, interpolationID, wxT("Cubic" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices2,wxCB_DROPDOWN,
            wxDefaultValidator,"Interpolation;" );
        

        ::setColor( mInterpolation);
        mFgs->Add( mInterpolation, 0, wxGROW|wxLEFT|wxRIGHT );

       //Similarity
        mSimilST = new wxStaticText( cp, -1, "Similarity:" );
        ::setColor( mSimilST );
        mFgs->Add( mSimilST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices3;
        choices3.Add(wxT("SSD" ));
        choices3.Add(wxT("CC"));
        choices3.Add(wxT("MI"));
        choices3.Add(wxT("NMI"));
        choices3.Add(wxT("Binary Kappa"));
        choices3.Add(wxT("Fuzzy Kappa"));
        choices3.Add(wxT("SAD"));
                
        mSimilarity = new wxComboBox( cp, similarityID, wxT("CC" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"Similarity;" );
        
   
        ::setColor( mSimilarity);
        mFgs->Add( mSimilarity, 0, wxGROW|wxLEFT|wxRIGHT );

        //Histogram sampling rate slider
        mHistoText0 = new wxStaticText( cp, -1, "Histogram sampling rate:" );
        ::setColor( mHistoText0 );
        mFgs->Add( mHistoText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        mZoomOut[0] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut[0] );
        mFgsSlider->Add( mZoomOut[0], 0, wxALIGN_RIGHT );
        
        mHisto = new wxSlider( cp, histoSliderID,
            HISTINIT, HISTMIN, HISTMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Histogram" );
        ::setColor( mHisto );
        mHisto->SetPageSize( 5 );
        mFgsSlider->Add( mHisto, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn[0] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn[0] );
        mFgsSlider->Add( mZoomIn[0], 0, wxALIGN_LEFT );
        
        wxString  s = wxString::Format( "%1d", HISTMIN );
        mHistoText1 = new wxStaticText( cp, -1, s );
        ::setColor( mHistoText1 );
        mFgsSlider->Add( mHistoText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", HISTINIT );//current histogram sampling rate
        mHistoText2 = new wxStaticText( cp, -1, s );
        ::setColor( mHistoText2 );
        mFgsSlider->Add( mHistoText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", HISTMAX);
        mHistoText3 = new wxStaticText( cp, -1, s );
        ::setColor( mHistoText3 );
        mFgsSlider->Add( mHistoText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        

        //Pyramid level slider
        mPyraText0 = new wxStaticText( cp, -1, "Pyramid level:" );
        ::setColor( mPyraText0 );
        mFgs->Add( mPyraText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 1 );
        mZoomOut[1] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_OUT, wxBitmap( button_zoomout15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut[1] );
        mFgsSlider->Add( mZoomOut[1], 0, wxALIGN_RIGHT );
        
        mPyra = new wxSlider( cp, pyraSliderID,
            PYRAINIT, PYRAMIN, PYRAMAX,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Pyramid" );
        ::setColor( mPyra );
        mPyra->SetPageSize( 5 );
        mFgsSlider->Add( mPyra, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn[1] = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ),
           wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn[1] );
        mFgsSlider->Add( mZoomIn[1], 0, wxALIGN_LEFT );
        
        s = wxString::Format( "%1d", PYRAMIN );
        mPyraText1 = new wxStaticText( cp, -1, s );
        ::setColor( mPyraText1 );
        mFgsSlider->Add( mPyraText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", PYRAINIT );//current histogram sampling rate
        mPyraText2 = new wxStaticText( cp, -1, s );
        ::setColor( mPyraText2 );
        mFgsSlider->Add( mPyraText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", PYRAMAX);
        mPyraText3 = new wxStaticText( cp, -1, s );
        ::setColor( mPyraText3 );
        mFgsSlider->Add( mPyraText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );


        mRegistrationSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        // - - - - - - - - - -
        //mFgs->AddSpacer( ButtonOffset );
        //mFgs->AddSpacer( ButtonOffset );
        mParST = new wxStaticText( cp, -1, "Transformation parameter output:" );
        ::setColor( mParST );
        mFgs->Add( mParST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL );
        
        mPCB = new wxCheckBox( cp, RegisterFrame::ID_PAR_INDEX, "on/off" );
        ::setColor( mPCB );
        mPCB->SetValue( 0 );
        mFgs->Add( mPCB, 0, wxALIGN_LEFT );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mPCB->SetToolTip(  "transformation parameter output file on & off" );
                mParST->SetToolTip(   "transformation parameter output file on & off" );
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

        /*mMatchIndex = new wxButton( cp, matchIndexID, "MatchIndex", wxDefaultPosition,
             wxSize(buttonWidth,buttonHeight) );
        ::setColor( mMatchIndex );
        gsLastRow->Add( mMatchIndex, 0, wxALIGN_RIGHT );*/
        
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
    /** \brief set the text of the current histogram sampling rate value. */
    void setHistoText ( const wxString s ) {  mHistoText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    /** Deletes the histogram slider TESTING WORK HERE NOT WORKING YET REMOVE(mFgsSlider) KIND OF WORKS
    void deleteHistoSlider ( void ) { 
	
	mFgsSlider->Detach(mSimilarity); 
	delete(mSimilarity);mSimilarity=NULL;
	mFgsSlider->Detach(mSimilST); 
	delete(mSimilST);mSimilST=NULL;
//	mFgsSlider->Detach(mHistoText0); 
//	delete(mHistoText0);mHistoText0=NULL;
	mFgsSlider->Detach(mHistoText1); 
	delete(mHistoText1);mHistoText1=NULL;
	mFgsSlider->Detach(mHistoText2); 
	delete(mHistoText2);mHistoText2=NULL;
	mFgsSlider->Detach(mHistoText3); 
	delete(mHistoText3);mHistoText3=NULL;
	mFgsSlider->Detach(mHisto);
	delete(mHisto);mHisto=NULL;
	mFgsSlider->Detach(mZoomIn[0]);
	delete(mZoomIn[0]);mZoomIn[0]=NULL;
	mFgsSlider->Detach(mZoomIn[1]);
	delete(mZoomIn[1]);mZoomIn[1]=NULL;
	mFgsSlider->Detach(mZoomOut[0]);
	delete(mZoomOut[0]);mZoomOut[0]=NULL;
	mFgsSlider->Detach(mZoomOut[1]);
	delete(mZoomOut[1]);mZoomOut[1]=NULL;
//	mFgsSlider->Detach(mHisto);

	mFgsSlider->Detach(mPyraText1); 
	delete(mPyraText1);mPyraText1=NULL;
	mFgsSlider->Detach(mPyraText2); 
	delete(mPyraText2);mPyraText2=NULL;
	mFgsSlider->Detach(mPyraText3); 
	delete(mPyraText3);mPyraText3=NULL;
	mFgsSlider->Detach(mPyra);
	delete(mPyra);mPyra=NULL;
	
	mFgs->Remove( mFgsSlider );
	delete(mFgsSlider);mFgsSlider=NULL;

	mRegistrationSizer->Remove( mFgs );
        delete mFgs;

       // mHistoText0 = new wxStaticText( cp, -1, "Test:" );
       // ::setColor( mHistoText0 );
       // mFgsSlider->Add( mHistoText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
	//mFgsSlider->Layout();
     }
*/

    /** \brief set the text of the current histogram sampling rate value. */
    void setPyraText ( const wxString s ) {  mPyraText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int getSimilarity     ( void ) const { return mSimilarity -> GetCurrentSelection ( );     }
    int getTransformation ( void ) const { return mTransformation -> GetCurrentSelection ( ); }
    int getInterpolation  ( void ) const { return mInterpolation -> GetCurrentSelection ( );  }
    bool getOnParallel    ( void ) const { return mCB -> IsChecked ( );                       }
    bool getOnPar    	  ( void ) const { return mPCB -> IsChecked ( );                       }
    int getHistoLevel     ( void ) const { return mHisto -> GetValue ( );                     }
    int getPyraDepth      ( void ) const { return mPyra -> GetValue ( );                      }

    /** \brief RegistrationControls dtor. */
    ~RegistrationControls ( void ) {
	//mFgs->Clear(true);
	//mFgsSlider->Clear(true);
        //cp->Refresh();
        //gsLastRow->Remove( mCB );
        /*
        delete mRegistrationBox;
     */ 
        //mBottomSizer->Remove( mRegistrationSizer );
	delete mCB;
	delete mPCB;
        mFgs->Detach( mTransformation );
        delete mTransformation;
        mFgs->Detach( mTranfST );
        delete mTranfST;
        mFgs->Detach( mInterpolation );
        delete mInterpolation;
        //gsLastRow->Detach( mParallelST );
        delete mParallelST;
	delete mParST;
        mFgs->Detach( mInterST );
        delete mInterST;
        mFgs->Detach( mSimilarity );
        delete mSimilarity;
        mFgs->Detach( mSimilST );
        delete mSimilST;
        mFgsSlider->Detach( mHisto );
        delete mHisto;
        mFgsSlider->Detach( mHistoText0 );
        delete mHistoText0;
        mFgsSlider->Detach( mHistoText1 );
        delete mHistoText1;
        mFgsSlider->Detach( mHistoText2 );
        delete mHistoText2;
        mFgsSlider->Detach( mHistoText3 );
        delete mHistoText3;
        mFgsSlider->Detach( mPyra );
        delete mPyra;
        mFgsSlider->Detach( mPyraText0 );
        delete mPyraText0;
        mFgsSlider->Detach( mPyraText1 );
        delete mPyraText1;
        mFgsSlider->Detach( mPyraText2 );
        delete mPyraText2;
        mFgsSlider->Detach( mPyraText3 );
        delete mPyraText3;
        mFgsSlider->Detach( mZoomIn[0] );
        delete mZoomIn[0];
        mFgsSlider->Detach( mZoomOut[0] );
        delete mZoomOut[0];
	mFgsSlider->Detach( mZoomIn[1] );
        delete mZoomIn[1];
        mFgsSlider->Detach( mZoomOut[1] );
        delete mZoomOut[1];
        mFgs->Remove( mFgsSlider );
        //delete mFgsSlider;
        mRegistrationSizer->Remove( mFgs );
        mBottomSizer->Remove( mRegistrationSizer );
	//delete mRegistrationSizer;
      //  delete mFgs;
    }
};

#endif
