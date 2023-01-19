/*
  Copyright 1993-2016 Medical Image Processing Group
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
 * \file   IRFCControls.h
 * \brief  Definition and implementation of IRFCControls class.
 * \author Andre Souza, Ph.D.
 *
 * Copyright: (C) 2007, CAVASS
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __IRFCControls_h
#define __IRFCControls_h

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

#define BRUSHMIN  1
#define BRUSHMAX  60
#define NOBJINIT 1
#define NOBJMIN  1
#define NOBJMAX  15

/**
 * \brief Definition and implementation of IRFCControls class.
 *
 * IRFC controls consist of a setting box for FuzzComp parameters
 *
 * Note: Callbacks are handled by the caller.
 */
#include  "FuzzCompFrame.h"

class IRFCControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor! 
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer  *mFgsSlider1, *mFgsSlider2;
    wxStaticText*     mParallelST;     ///< parallel checkbox label
    wxCheckBox*       mCB;            ///< parallel on/off checkbox
    wxStaticBox*      mIRFCBox;
    wxComboBox*       mMode;          ///< Mode combo box
    wxStaticText*     mModeST;       ///< Mode combot box label
    wxComboBox*       mAlgorithm;    ///< Algorithm combo box
    wxStaticText*     mAlgST;       ///< Algorithm combo box label
    wxComboBox*       mAffinity;    ///< Affinity combo box
    wxStaticText*     mAffST;       ///< Affinity combo box label

    wxSlider*         mNObj;         ///< Number of objects slider
    wxStaticText*     mNObjText0;    ///< scale slider label
    wxStaticText*     mNObjText1;    ///< scale min value
    wxStaticText*     mNObjText2;    ///< scale current value
    wxStaticText*     mNObjText3;    ///< scale max value

    wxSlider*         mBrush;         ///< Brush level slider
    wxStaticText*     mBrushText0;    ///< scale slider label
    wxStaticText*     mBrushText1;    ///< scale min value
    wxStaticText*     mBrushText2;    ///< scale current value
    wxStaticText*     mBrushText3;    ///< scale max value

    wxSizer*          mIRFCSizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    
public:

    /** \brief RegistrationControls ctor. */
    IRFCControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int modeID,const int algorithmID,
        const int affinityID, const int brushSliderID, const int nObjSliderID,
        const int paralelIndexID, const int max_objects, const int cur_brush=5)
    {
        mBottomSizer = bottomSizer;
        mIRFCBox = new wxStaticBox( cp, -1, title );
        ::setColor( mIRFCBox );
        mIRFCSizer = new wxStaticBoxSizer( mIRFCBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2,10, 2 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth/2, 0 );
        mFgs->AddGrowableCol( 0 );
        //Mode
        mModeST = new wxStaticText( cp, -1, "Mode:" );
        ::setColor( mModeST );
        mFgs->Add( mModeST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices1;
        choices1.Add(wxT("Foreground seeds" ));
        choices1.Add(wxT("Training"));
		choices1.Add(wxT("Background seeds" ));
        
        
        mMode = new wxComboBox( cp, modeID, wxT("Training" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices1,wxCB_DROPDOWN,
            wxDefaultValidator,"Mode" );

       
            
        ::setColor( mMode );
        mFgs->Add( mMode, 0, wxGROW|wxLEFT|wxRIGHT );

        //Algorithm
        mAlgST = new wxStaticText( cp, -1, "Algorithm:" );
        ::setColor( mAlgST );
        mFgs->Add( mAlgST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices2;
        choices2.Add(wxT("IRFC" ));
        choices2.Add(wxT("MOFS"));
		choices2.Add(wxT("IIRFC" ));
        
                
        mAlgorithm = new wxComboBox( cp, algorithmID, wxT("IRFC" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices2,wxCB_DROPDOWN,
            wxDefaultValidator,"Algorithm" );
        

        ::setColor( mAlgorithm);
        mFgs->Add( mAlgorithm, 0, wxGROW|wxLEFT|wxRIGHT );

       //Affinity
        mAffST = new wxStaticText( cp, -1, "Object:" );
        ::setColor( mAffST );
        mFgs->Add( mAffST, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
        wxArrayString choices3;
		choices3.Add(wxT("0"));
        choices3.Add(wxT("1"));

        mAffinity = new wxComboBox( cp, affinityID, wxT("1" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"Object" );
        
   
        ::setColor( mAffinity);
        mFgs->Add( mAffinity, 0, wxGROW|wxLEFT|wxRIGHT );

        //Brush size slider
        mBrushText0 = new wxStaticText( cp, -1, "Brush size:" );
        ::setColor( mBrushText0 );
        mFgs->Add( mBrushText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider1 = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider1->AddGrowableCol( 1 );

        mBrush = new wxSlider( cp, brushSliderID,
            cur_brush, BRUSHMIN, BRUSHMAX,
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
        
        s = wxString::Format( "%1d", cur_brush );//current brush size
        mBrushText2 = new wxStaticText( cp, -1, s );
        ::setColor( mBrushText2 );
        mFgsSlider1->Add( mBrushText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", BRUSHMAX);
        mBrushText3 = new wxStaticText( cp, -1, s );
        ::setColor( mBrushText3 );
        mFgsSlider1->Add( mBrushText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider1, 0, wxGROW|wxLEFT|wxRIGHT );
        

        //Number of objects slider
        mNObjText0 = new wxStaticText( cp, -1, "Number of objects:" );
        ::setColor( mNObjText0 );
        mFgs->Add( mNObjText0, 0, wxALIGN_LEFT|wxALIGN_TOP,2 );
        
        mFgsSlider2 = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider2->AddGrowableCol( 1 );

        mNObj = new wxSlider( cp, nObjSliderID,
            NOBJINIT, NOBJMIN, max_objects,
            wxDefaultPosition, wxSize(sliderWidth/2, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Number of objects" );
        ::setColor( mNObj );
        mNObj->SetPageSize( 5 );
		mFgsSlider2->AddStretchSpacer();
        mFgsSlider2->Add( mNObj, 0, wxGROW|wxLEFT|wxRIGHT );
		mFgsSlider2->AddStretchSpacer();

        s = wxString::Format( "%1d", NOBJMIN );
        mNObjText1 = new wxStaticText( cp, -1, s );
        ::setColor( mNObjText1 );
        mFgsSlider2->Add( mNObjText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%1d", NOBJINIT );//current histogram sampling rate
        mNObjText2 = new wxStaticText( cp, -1, s );
        ::setColor( mNObjText2 );
        mFgsSlider2->Add( mNObjText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%1d", max_objects);
        mNObjText3 = new wxStaticText( cp, -1, s );
        ::setColor( mNObjText3 );
        mFgsSlider2->Add( mNObjText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider2, 0, wxGROW|wxLEFT|wxRIGHT );


        mIRFCSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        // - - - - - - - - - -
        mParallelST = NULL;
        mCB = NULL;

        mBottomSizer->Prepend( mIRFCSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void setNObj( wxPanel* cp, int affinityID, int nObj, int obj )
	{
		wxArrayString choices_n;
		for (int j=0; j<=nObj; j++)
			choices_n.Add(wxString::Format("%d", j));
		wxComboBox *tAffinity = new wxComboBox( cp, affinityID, choices_n[obj],
			wxDefaultPosition, wxSize(sliderWidth, -1), choices_n,
			wxCB_DROPDOWN, wxDefaultValidator, "Object" );
		::setColor( tAffinity);
		mFgs->Replace( mAffinity, tAffinity );
		delete mAffinity;
		mAffinity = tAffinity;
		mBottomSizer->Layout();
		cp->Refresh();
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void setNObjSlider( int nObj )
	{
		mNObj->SetValue( nObj );
		setNObjText( wxString::Format( "%1d", nObj ) );
	}

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current brush size. */
    void setBrushText ( const wxString s ) {  mBrushText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    /** \brief set the text of the current number of objects. */
    void setNObjText ( const wxString s ) {  mNObjText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    int getAffinity    ( void ) const { return mAffinity -> GetCurrentSelection ( );     }
    int getAlgorithm ( void ) const { return mAlgorithm -> GetCurrentSelection ( ); }
    int getMode  ( void ) const { return mMode -> GetCurrentSelection ( );  }
    bool getOnParallel    ( void ) const { return mCB -> IsChecked ( );                       }
    int getBrushSize      ( void ) const { return mBrush -> GetValue ( );                      }

    /** \brief RegistrationControls dtor. */
    ~IRFCControls ( void ) {
        mBottomSizer->Remove( mIRFCSizer );
        mBottomSizer->Layout(); 
        delete mCB;
        delete mFgs;
        delete mFgsSlider1;
		delete mFgsSlider2;
        delete mIRFCBox;
        delete mMode;
        delete mModeST;
        delete mAlgorithm;
        delete mAlgST;
        delete mAffinity;
        delete mAffST;
        delete mBrush;
        delete mBrushText0;
        delete mBrushText1;
        delete mBrushText2;
        delete mBrushText3;
        delete mNObj;
        delete mNObjText0;
        delete mNObjText1;
        delete mNObjText2;
        delete mNObjText3;
    }
};

/**
 * \brief Definition and implementation of IRFCParamControls class.
 *
 * Parameter controls consist of a control box with window center and width
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class IRFCParamControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!
    wxCheckBox*       mCb0;        ///< invert the gray map checkbox
    wxCheckBox*       mCb1;        ///< invert the gray map checkbox
    wxCheckBox*       mCb2;        ///< invert the gray map checkbox
    wxCheckBox*       mCb3;        ///< invert the gray map checkbox
    wxCheckBox*       mCb4;        ///< invert the gray map checkbox
    wxComboBox*       mG0;
    wxComboBox*       mG1;
    wxComboBox*       mG2;
    wxComboBox*       mG3;
    wxComboBox*       mG4;
	wxComboBox*       mG5;
    wxStaticBox*      mContrastBox;
    wxSizer*          mContrastBoxSizer;
    wxFlexGridSizer*  mContrastSizer;
    wxFlexGridSizer*  mfg;
	wxFlexGridSizer*  mfg1;
    wxStaticText*     mSt;        ///< center slider title
	wxStaticText*     mSt1;
	wxStaticText*     mSt2;
	wxStaticText*     mSt3;
	wxStaticText*     mSt4;
	wxSlider*         mSS;
	wxSlider*         mES;
    
public:
    /** \brief IRFCParamControls ctor. */
    IRFCParamControls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
        const bool currentHigh, const bool currentLow, const bool currentDiff,
        const bool currentSum, const bool currentRelDiff,
		const int currentSslice, const int currentEslice, const int NoSlices,
		const int highID, const int lowID, const int diffID, const int sumID,
		const int reldiffID, const int highComboID, const int lowComboID,
		const int diffComboID, const int sumComboID, const int reldiffComboID,
		const int outAffID, const int startSliceID, const int endSliceID )
    {
        mBottomSizer = bottomSizer;
        mContrastBox = new wxStaticBox( cp, -1, title );
        ::setColor( mContrastBox );
        mContrastBoxSizer = new wxStaticBoxSizer( mContrastBox, wxHORIZONTAL );

        mContrastSizer = new wxFlexGridSizer( 1, 2, 2 );  //cols,vgap,hgap
        mContrastSizer->SetMinSize( controlsWidth/2, 0 );
        mContrastSizer->AddGrowableCol( 0 );

        
        
        // - - - - - - - - - -
        mfg = new wxFlexGridSizer( 2, 1, 1);  //cols,vgap,hgap
        mCb0 = new wxCheckBox( cp, highID, "high" );
        ::setColor( mCb0 );
        if (currentHigh)    mCb0->SetValue( 1 );
        else                  mCb0->SetValue( 0 );
        mfg->Add( mCb0, 0, wxALIGN_LEFT);

        wxArrayString choices3;
	    choices3.Add(wxT("Gaussian" ));
        choices3.Add(wxT("Ramp"));
        choices3.Add(wxT("Box"));

                
        mG0 = new wxComboBox( cp, highComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionHigh" );
           ::setColor( mG0);
        mfg->Add( mG0, 0, wxGROW|wxLEFT|wxRIGHT,3 );
		mG0->Enable( currentHigh );

        /*--------------------------------*/
        
        mCb1 = new wxCheckBox( cp, lowID, "low" );
        ::setColor( mCb1 );
        if (currentLow)    mCb1->SetValue( 1 );
        else                  mCb1->SetValue( 0 );
        mfg->Add( mCb1, 0, wxALIGN_LEFT );

        mG1 = new wxComboBox( cp, lowComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionLow" );
           ::setColor( mG1);
        mfg->Add( mG1, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        mG1->Enable( currentLow );
        /*--------------------------------*/
        
        mCb2 = new wxCheckBox( cp, diffID, "difference" );
        ::setColor( mCb2 );
        if (currentDiff)    mCb2->SetValue( 1 );
        else                  mCb2->SetValue( 0 );
        mfg->Add( mCb2, 0, wxALIGN_LEFT );
        
        mG2 = new wxComboBox( cp, diffComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionDiff" );
           ::setColor( mG2);
        mfg->Add( mG2, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        mG2->Enable( currentDiff );
        /*--------------------------------*/

        mCb3 = new wxCheckBox( cp, sumID, "sum" );
        ::setColor( mCb3 );
        if (currentSum)    mCb3->SetValue( 1 );
        else                  mCb3->SetValue( 0 );
        mfg->Add( mCb3, 0,wxALIGN_LEFT );
        
        mG3 = new wxComboBox( cp, sumComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionSum" );
           ::setColor( mG3);
        mfg->Add( mG3, 0, wxGROW|wxLEFT|wxRIGHT,3 );
		mG3->Enable( currentSum );
        /*--------------------------------*/

        mCb4 = new wxCheckBox( cp, reldiffID, "relative difference" );
        ::setColor( mCb4 );
        if (currentRelDiff)    mCb4->SetValue( 1 );
        else                  mCb4->SetValue( 0 );
        mfg->Add( mCb4, 0, wxALIGN_LEFT );
        
        mG4 = new wxComboBox( cp, reldiffComboID, wxT("Gaussian" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices3,wxCB_DROPDOWN,
            wxDefaultValidator,"FunctionRelDiff" );
           ::setColor( mG4);
        mfg->Add( mG4, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        mG4->Enable( currentRelDiff );
		
		mContrastSizer->Add( mfg, 0, wxGROW );
		
        		
		mfg1 = new wxFlexGridSizer( 4, 2, 2);  //cols,vgap,hgap
		//---Starting slice---
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		
        mSt3 = new wxStaticText( cp, -1, "Starting slice:" );
        ::setColor( mSt3 );
        mfg1->Add( mSt3, 0, wxALIGN_LEFT );
        mSS = new wxSlider( cp, startSliceID, currentSslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Sslice" );
        mSS->SetPageSize( 10 );
        ::setColor( mSS );
        mfg1->Add( mSS, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        //---------------------------
     
	   //---Ending slice---
        mSt4 = new wxStaticText( cp, -1, "Ending slice:" );
        ::setColor( mSt4 );
        mfg1->Add( mSt4, 0, wxALIGN_LEFT );
        mES = new wxSlider( cp, endSliceID, currentEslice, 1, NoSlices,
            wxDefaultPosition, wxSize(sliderWidth,-1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP,
            wxDefaultValidator, "Eslice" );
        mES->SetPageSize( 10 );
        ::setColor( mES );
        mfg1->Add( mES, 0, wxGROW|wxLEFT|wxRIGHT,3 );
        //---------------------------
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer
		mfg1->Add( 0, 5, 20, wxGROW );  //spacer

        /*--------------------------------*/
		
		 mSt1 = new wxStaticText( cp, -1, "Affinity Output:" );
        ::setColor( mSt1 );
        mfg1->Add( mSt1, 0, wxALIGN_LEFT );

		
		wxArrayString choices;
	    choices.Add(wxT("x   component" ));
        choices.Add(wxT("y   component"));
        choices.Add(wxT("z   component"));
		choices.Add(wxT("xy  component" ));
        choices.Add(wxT("xyz component"));
		
        mG5 = new wxComboBox( cp, outAffID, wxT("xy component" ), wxDefaultPosition,
            wxSize(sliderWidth, -1),choices,wxCB_DROPDOWN,
            wxDefaultValidator,"AffOut" );
           ::setColor( mG5);
        mfg1->Add( mG5, 0, wxGROW|wxLEFT|wxRIGHT);
		
        /*--------------------------------*/


        mContrastSizer->Add(mfg1, 0, wxGROW);

        
        mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

        mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    int getHighComboSelection    ( void ) const { return mG0 -> GetCurrentSelection ( );     }
    int getLowComboSelection    ( void ) const { return mG1 -> GetCurrentSelection ( );     }
    int getDiffComboSelection    ( void ) const { return mG2 -> GetCurrentSelection ( );     }
    int getSumComboSelection    ( void ) const { return mG3 -> GetCurrentSelection ( );     }
    int getRelDiffComboSelection    ( void ) const { return mG4 -> GetCurrentSelection ( );     }
	int getAffOutSelection    ( void ) const { return mG5 -> GetCurrentSelection ( );     }
	bool getOnHigh    ( void ) const { return mCb0 -> IsChecked ( );                       }
	bool getOnLow    ( void ) const { return mCb1 -> IsChecked ( );                       }
	bool getOnDiff    ( void ) const { return mCb2 -> IsChecked ( );                       }
	bool getOnSum    ( void ) const { return mCb3 -> IsChecked ( );                       }
	bool getOnRelDiff    ( void ) const { return mCb4 -> IsChecked ( );                       }
	void enableHighCombo (bool value) { mG0->Enable( value); }
    void enableLowCombo (bool value) { mG1->Enable( value); }
	void enableDiffCombo (bool value) { mG2->Enable( value); }
	void enableSumCombo (bool value) { mG3->Enable( value); }
	void enableRelDiffCombo (bool value) { mG4->Enable( value); }
	int getStartSlice    ( void ) const { return mSS -> GetValue ( );                           }
    int getEndSlice    ( void ) const { return mES -> GetValue ( );                             }
	void setHighComboSelection ( const wxString function ) { mG0->SetValue(function); }
	void setLowComboSelection ( const wxString function ) { mG1->SetValue(function); }
	void setDiffComboSelection ( const wxString function ) { mG2->SetValue(function); }
	void setSumComboSelection ( const wxString function ) { mG3->SetValue(function); }
	void setRelDiffComboSelection ( const wxString function ) { mG4->SetValue(function); }

    /** \brief IRFCParamControls dtor. */
    ~IRFCParamControls ( void ) {
        mBottomSizer->Remove( mContrastBoxSizer );
        mBottomSizer->Layout();
        delete mCb0;
        delete mCb1;
        delete mCb2;
        delete mCb3;
        delete mCb4;
        delete mG0;
        delete mG1;
        delete mG2;
        delete mG3;
        delete mG4;
		delete mG5;
  //      delete mContrastBox;
		delete mSt1;
		delete mSt3;
		delete mSt4;
		delete mSS;
		delete mES;
    }
};

#endif
