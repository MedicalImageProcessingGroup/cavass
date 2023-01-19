/*
  Copyright 1993-2014 Medical Image Processing Group
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
 * \file   SetThresholdIndexControls.h
 * \brief  Definition and implementation of SetThresholdIndexControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SetThresholdIndexControls_h
#define __SetThresholdIndexControls_h

#define resolutionIMin 0
#define resolutionIMax 1000
#define histZoomIMin 1
#define histZoomIMax 1000

extern char*  button_zoomin15_xpm[];
extern char*  button_zoomout15_xpm[];

/**
 * \brief Definition and implementation of SetThresholdIndexControls class.
 *
 * Set index controls consist of a control box with the name of the
 *
 * Note: Callbacks are handled by the caller.
 */
#include "ThresholdFrame.h"

class SetThresholdIndexControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxCheckBox*       mCB;            ///< overlay on/off checkbox
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxStaticText*     mOverlayST;     ///< overlay checkbox label
    wxStaticBox*      mSetIndexBox;
    wxSlider*         mSliceNumber;   ///< slice number slider
    wxStaticText*     mSliceST;       ///< slice number slider label
    wxSlider*         mScale;         ///< scale slider
    wxStaticText*     mScaleText0;    ///< scale slider label
    wxStaticText*     mScaleText1;    ///< scale min value
    wxStaticText*     mScaleText2;    ///< scale current value
    wxStaticText*     mScaleText3;    ///< scale max value
    wxStaticBoxSizer* mSetIndexSizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_scale_value;
    double            mScaleZoom;
public:
    /** \brief SetThresholdIndexControls ctor. */
    SetThresholdIndexControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, const int currentSlice, const int numberOfSlices,
        const int sliceSliderID, const int scaleSliderID, const double currentScale, const int overlayID, const int zoomInID, const int zoomOutID )
    {
        mBottomSizer = bottomSizer;
        mSetIndexBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetIndexBox );
        mSetIndexSizer = new wxStaticBoxSizer( mSetIndexBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
        //slice number
		if (numberOfSlices > 1)
		{
	        mSliceST = new wxStaticText( cp, -1, "slice:" );
	        ::setColor( mSliceST );
	        mFgs->Add( mSliceST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
	        mSliceNumber = new wxSlider( cp, sliceSliderID, currentSlice, 1,
	            numberOfSlices, wxDefaultPosition, wxSize(sliderWidth, -1),
	            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
	            "slice" );
	        ::setColor( mSliceNumber );
	        mSliceNumber->SetPageSize( 5 );
	        mFgs->Add( mSliceNumber, 0, wxGROW|wxLEFT|wxRIGHT );
		}
		else
		{
			mSliceST = NULL;
			mSliceNumber = NULL;
		}

        //scale
        mScaleText0 = new wxStaticText( cp, -1, "scale:" );
        ::setColor( mScaleText0 );
        mFgs->Add( mScaleText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        mZoomOut = new wxBitmapButton( cp, zoomOutID, wxBitmap( button_zoomout15_xpm ), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut );
        mFgsSlider->Add( mZoomOut, 0, wxALIGN_RIGHT );
        m_old_scale_value = 100;
        mScaleZoom = 1.0;
        mScale = new wxSlider( cp, scaleSliderID,
            (int)(currentScale*100+0.5), scaleIMin, scaleIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "scale" );
        ::setColor( mScale );
        mScale->SetPageSize( 5 );
        mFgsSlider->Add( mScale, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn = new wxBitmapButton( cp, zoomInID, wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomIn );
        mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
        
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
        
        mSetIndexSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        
        mFgs->AddSpacer( ButtonOffset );
        mFgs->AddSpacer( ButtonOffset );
        mOverlayST = new wxStaticText( cp, -1, "Labels:" );
        ::setColor( mOverlayST );
        mFgs->Add( mOverlayST, 0, wxALIGN_RIGHT );
        
        mCB = new wxCheckBox( cp, overlayID, "on/off" );
        ::setColor( mCB );
        mCB->SetValue( 1 );
		mFgs->Add( mCB /*, 0, wxALIGN_RIGHT*/);

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
	            mCB->SetToolTip(          "toggle image overlay on & off" );
				mOverlayST->SetToolTip(   "toggle image overlay on & off" );
                if (mSliceNumber)
					mSliceNumber->SetToolTip( "slice number" );
                mScale->SetToolTip(       "scale" );
            #endif
        #endif
        mBottomSizer->Prepend( mSetIndexSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) previous and next buttons might change
     *  the current slice, the following method will allow an update of
     *  the slice slider.
     */
    void setSliceNo ( const int currentSlice ) {  
        if (mSliceNumber)
			mSliceNumber->SetValue( currentSlice );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current scale value. */
    void setScaleText ( const wxString s ) {  mScaleText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SetThresholdIndexControls dtor. */
    ~SetThresholdIndexControls ( void ) {
#ifdef DETACH_CONTROLS_FROM_SIZERS
        mFgs->Detach(mCB);
		delete mCB;
	    mFgs->Detach(mOverlayST);
		delete mOverlayST;
		mFgsSlider->Detach(mScaleText3);
		mFgsSlider->Detach(mScaleText2);
		mFgsSlider->Detach(mScaleText1);
        delete mScaleText3;
        delete mScaleText2;
        delete mScaleText1;
		mFgsSlider->Detach(mZoomIn);
        delete mZoomIn;
		mFgsSlider->Detach(mScale);
        delete mScale;
		mFgsSlider->Detach(mZoomOut);
        delete mZoomOut;
		mFgs->Detach(mScaleText0);
        delete mScaleText0;
		if (mSliceNumber)
			mFgs->Detach(mSliceNumber);
		delete mSliceNumber;
		if (mSliceST)
			mFgs->Detach(mSliceST);
        delete mSliceST;
		delete mSetIndexBox;
		mSetIndexBox = NULL;
#else
		mFgsSlider->Clear( true );
		mFgs->Clear( true );
		mSetIndexSizer->Clear( true );
#endif
		mFgs->Remove( mFgsSlider );
		mSetIndexSizer->Remove( mFgs );
		mBottomSizer->Remove( mSetIndexSizer );
    }
};

// 8/10/07 Dewey Odhner
class SetThresholdParameterControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxStaticText*     mParam1Label;
	wxTextCtrl*       mParam1Ctrl;
    wxStaticText*     mParam2Label;
	wxTextCtrl*       mParam2Ctrl;
    wxStaticText*     mParam3Label;
	wxTextCtrl*       mParam3Ctrl;
	wxComboBox       *m_numParamsBut;
	wxComboBox       *m_curVolumeBut;
	wxComboBox       *m_whichParamBut;
	wxComboBox       *m_paramTypeBut;
	wxStaticBoxSizer* mSetParamBoxSizer;
    wxStaticBox*      mSetParamBox;
	char              (*param_type_name)[11];
public:
	// Modified: 8/29/07 numVolumes passed by Dewey Odhner.
    /** \brief SetThresholdParameterControls ctor. */
    SetThresholdParameterControls ( wxPanel* cp, wxSizer* bottomSizer,
        const char* const title, int currentNumParams, int currentParam,
        int currentParamType, int numParamsID, int whichParamID,
		int paramTypeID, int param1ID, int param2ID, int param3ID,
		int curVolumeID, double currentParam1, double currentParam2,
		double currentParam3, int currentVolume, int numVolumes=1 )
    {
		static char s_param_type_name[][11]=
			{ "Time", "Object", "Modality", "Long. Time", "Subject", "Other" };
        param_type_name = s_param_type_name;
		mBottomSizer = bottomSizer;
        mSetParamBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetParamBox );
        mSetParamBoxSizer = new wxStaticBoxSizer( mSetParamBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 5, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

		wxArrayString sa;
		for (int j=1; j<=3; j++)
			sa.Add(wxString::Format("%d Parameters", j));
		m_numParamsBut= new wxComboBox(cp, numParamsID, sa[currentNumParams-1],
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_numParamsBut );
		mFgs->Add(m_numParamsBut, 0, wxGROW|wxLEFT|wxRIGHT );
		sa.Empty();
		for (int j=1; j<=numVolumes; j++)
			sa.Add(wxString::Format("Volume %d", j));
		m_curVolumeBut = new wxComboBox( cp, curVolumeID, sa[currentVolume],
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_curVolumeBut );
		m_curVolumeBut->Enable(numVolumes>1);
		mFgs->Add(m_curVolumeBut, 0, wxGROW|wxLEFT|wxRIGHT );
		sa.Empty();
		for (int j=1; j<=currentNumParams; j++)
			sa.Add(wxString::Format("Parameter: %d", j));
		m_whichParamBut = new wxComboBox( cp, whichParamID, sa[currentParam],
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_whichParamBut );
		m_whichParamBut->Enable(currentNumParams>1);
		mFgs->Add( m_whichParamBut, 0, wxGROW|wxLEFT|wxRIGHT );
		sa.Empty();
		for (int j=0; j<6; j++)
			sa.Add(wxString("Type: ")+param_type_name[j]);
		m_paramTypeBut = new wxComboBox( cp, paramTypeID, sa[currentParamType],
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_paramTypeBut );
		mFgs->Add( m_paramTypeBut, 0, wxGROW|wxLEFT|wxRIGHT );

		mParam1Label = new wxStaticText( cp, -1, "Parameter 1:" );
        ::setColor( mParam1Label );
        mFgs->Add( mParam1Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mParam1Ctrl = new wxTextCtrl( cp, param1ID, 
			wxString::Format("%f",currentParam1) );
        ::setColor( mParam1Ctrl );
        mFgs->Add( mParam1Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );
		mParam2Label = new wxStaticText( cp, -1, "Parameter 2:" );
        ::setColor( mParam2Label );
        mFgs->Add( mParam2Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mParam2Ctrl = new wxTextCtrl( cp, param2ID, 
			wxString::Format("%f",currentParam2) );
        ::setColor( mParam2Ctrl );
		mParam2Ctrl->Enable(currentNumParams>1);
        mFgs->Add( mParam2Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );
		mParam3Label = new wxStaticText( cp, -1, "Parameter 3:" );
        ::setColor( mParam3Label );
        mFgs->Add( mParam3Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mParam3Ctrl = new wxTextCtrl( cp, param3ID, 
			wxString::Format("%f",currentParam3) );
        ::setColor( mParam3Ctrl );
		mParam3Ctrl->Enable(currentNumParams>2);
        mFgs->Add( mParam3Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
	            m_numParamsBut->SetToolTip("number of parameters" );
				m_whichParamBut->SetToolTip("select parameter number" );
				m_paramTypeBut->SetToolTip("type of parameter" );
                mParam1Ctrl->SetToolTip( "Parameter 1 value" );
                mParam2Ctrl->SetToolTip( "Parameter 2 value" );
                mParam3Ctrl->SetToolTip( "Parameter 3 value" );
            #endif
        #endif
		mSetParamBoxSizer->Add(mFgs, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Prepend( mSetParamBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SetThresholdParameterControls dtor. */
    ~SetThresholdParameterControls ( void ) {
        mBottomSizer->Detach( mSetParamBoxSizer );
		mFgs->Detach(mParam3Ctrl);
		delete mParam3Ctrl;
		mFgs->Detach(mParam3Label);
		delete mParam3Label;
		mFgs->Detach(mParam2Ctrl);
		delete mParam2Ctrl;
		mFgs->Detach(mParam2Label);
		delete mParam2Label;
		mFgs->Detach(mParam1Ctrl);
		delete mParam1Ctrl;
		mFgs->Detach(mParam1Label);
		delete mParam1Label;
		mFgs->Detach(m_paramTypeBut);
		delete m_paramTypeBut;
		mFgs->Detach(m_whichParamBut);
		delete m_whichParamBut;
		mFgs->Detach(m_curVolumeBut);
		delete m_curVolumeBut;
		mFgs->Detach(m_numParamsBut);
		delete m_numParamsBut;
		mSetParamBoxSizer->Detach(mFgs);
		delete mFgs;
        delete mSetParamBox;
    }

	int getParamType()
	{
		return m_paramTypeBut->GetSelection();
	}
	void setNumParamsLabel(int newNumParams)
	{
		mParam3Ctrl->Enable(newNumParams>2);
		mParam2Ctrl->Enable(newNumParams>1);
		m_whichParamBut->Enable(newNumParams>1);
	}
	void setCurVolumeLabel(int newVolume)
	{
		m_curVolumeBut->SetSelection(newVolume);
	}
	void setWhichParamLabel(int newParam)
	{
		m_whichParamBut->SetSelection(newParam);
	}
	void setParamTypeLabel(int newParamType /* 0 to 5 */)
	{
		m_paramTypeBut->SetSelection(newParamType);
	}
	void setParam1Value(double v)
	{
		mParam1Ctrl->SetValue(wxString::Format("%f", v));
	}
	void setParam2Value(double v)
	{
		mParam2Ctrl->SetValue(wxString::Format("%f", v));
	}
	void setParam3Value(double v)
	{
		mParam3Ctrl->SetValue(wxString::Format("%f", v));
	}
};

class  SetThresholdOutputControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
	wxFlexGridSizer*  mFgsButton;
	wxFlexGridSizer*  mFgsSliders;
    wxStaticBox*      mSetOutputBox;
	wxComboBox       *m_normalTypeBut;
	wxButton         *m_saveBut;
	wxComboBox       *m_outputModeBut;
	wxComboBox       *m_outputIntervalBut;
	wxSlider*         mResolution;         ///< resolution slider
    wxStaticText*     mResolutionText0;    ///< resolution slider label
    wxStaticText*     mResolutionText1;    ///< resolution min value
    wxStaticText*     mResolutionText2;    ///< resolution current value
    wxStaticText*     mResolutionText3;    ///< resolution max value
    wxStaticBoxSizer* mSetOutputSizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_resolution_value;
    double            mResolutionZoom;

public:
	// Modified: 8/29/07 enableStructOptions passed by Dewey Odhner.
	SetThresholdOutputControls( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, int resolutionSliderID, int normalTypeID,
		int saveID, int outputModeID, int outputIntervalID,
		double currentResolution, const char* currentNormal,
		int currentOutputMode/* 0=New 1=Merge */, int currentOutInterval,
		bool enableStructOptions )
	{
        mBottomSizer = bottomSizer;
        mSetOutputBox = new wxStaticBox( cp, -1, title );
        ::setColor( mSetOutputBox );
        mSetOutputSizer = new wxStaticBoxSizer( mSetOutputBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
		mFgsButton = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap

		wxArrayString sa;
		sa.Add("Out Mode: New");
		sa.Add("Out Mode: Merge");
		m_outputModeBut = new wxComboBox( cp, outputModeID,
			sa[currentOutputMode],
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_outputModeBut );
		m_outputModeBut->Enable(enableStructOptions);
		mFgsButton->Add( m_outputModeBut, 0,
		 	wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		sa.Empty();
		sa.Add("Out Interval: All");
		for (int j=1; j<=4; j++)
			sa.Add(wxString::Format("Out Interval: %d", j));
		m_outputIntervalBut= new wxComboBox(cp, outputIntervalID,
			sa[currentOutInterval],
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_outputIntervalBut );
		mFgsButton->Add( m_outputIntervalBut, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
		sa.Empty();
		sa.Add("Normal: 0");
		sa.Add("Normal: 8");
		sa.Add("Normal: 26");
		m_normalTypeBut = new wxComboBox( cp, normalTypeID,
			wxString::Format("Normal: %s", currentNormal),
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight), sa,
			wxCB_READONLY );
        ::setColor( m_normalTypeBut );
		m_normalTypeBut->Enable(enableStructOptions);
		mFgsButton->Add( m_normalTypeBut, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	    m_saveBut = new wxButton( cp, saveID, "Save",
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( m_saveBut );
	    mFgsButton->Add( m_saveBut, 0,
			wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

		mFgs->Add( mFgsButton, 0, wxGROW|wxALL, 10 );
		mFgsSliders = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap

        //resolution
        mResolutionText0 = new wxStaticText( cp, -1, "Resolution:" );
        ::setColor( mResolutionText0 );
        mFgsSliders->Add( mResolutionText0, 0,
			wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);

        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );
        mZoomOut = new wxBitmapButton( cp, ThresholdFrame::ID_ZOOM_OUT,
			wxBitmap(button_zoomout15_xpm), wxDefaultPosition, wxSize(15,15) );
        ::setColor( mZoomOut );
        mFgsSlider->Add( mZoomOut, 0, wxALIGN_RIGHT );
        m_old_resolution_value = 100;
        mResolutionZoom = 1.0;
        mResolution = new wxSlider( cp, resolutionSliderID,
            (int)(currentResolution*100+0.5), resolutionIMin, resolutionIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            //wxSL_HORIZONTAL|wxSL_AUTOTICKS, wxDefaultValidator,
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "Resolution" );
        ::setColor( mResolution );
		mResolution->Enable(enableStructOptions);
        mResolution->SetPageSize( 5 );
        mFgsSlider->Add( mResolution, 0, wxGROW|wxLEFT|wxRIGHT );
        mZoomIn = new wxBitmapButton( cp, ThresholdFrame::ID_ZOOM_IN,
			wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15));
        ::setColor( mZoomIn );
        mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );

        wxString  s = wxString::Format( "%5.2f", resolutionIMin/100.0 );
        mResolutionText1 = new wxStaticText( cp, -1, s );
        ::setColor( mResolutionText1 );
        mFgsSlider->Add( mResolutionText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%5.2f", currentResolution );
        mResolutionText2 = new wxStaticText( cp, -1, s );
        ::setColor( mResolutionText2 );
        mFgsSlider->Add( mResolutionText2, 0, wxALIGN_CENTER );

        s = wxString::Format( "%5.2f", resolutionIMax/100.0 );
        mResolutionText3 = new wxStaticText( cp, -1, s );
        ::setColor( mResolutionText3 );
        mFgsSlider->Add( mResolutionText3, 0, wxALIGN_LEFT );

        mFgsSliders->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );

		mFgs->Add( mFgsSliders, 0, wxGROW|wxALL, 10 );
        mSetOutputSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Prepend( mSetOutputSizer, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Layout();
		cp->Refresh();
	}
	~SetThresholdOutputControls() {
		if (mResolutionText0!=NULL) {
			mFgsSlider->Detach(mResolutionText3);
			delete mResolutionText3;
			mFgsSlider->Detach(mResolutionText2);
			delete mResolutionText2;
			mFgsSlider->Detach(mResolutionText1);
			delete mResolutionText1;
			mFgsSlider->Detach(mZoomIn);
			delete mZoomIn;
			mFgsSlider->Detach(mResolution);
			delete mResolution;
			mFgsSlider->Detach(mZoomOut);
			delete mZoomOut;
			mFgsSliders->Detach(mFgsSlider);
			delete mFgsSlider;

			mFgsSliders->Detach(mResolutionText0);
			delete mResolutionText0;
		}

		mFgs->Detach(mFgsSliders);
		delete mFgsSliders;
		mFgsButton->Detach(m_saveBut);
		delete m_saveBut;
		if (m_normalTypeBut!=NULL) {
			mFgsButton->Detach(m_normalTypeBut);
			delete m_normalTypeBut;
		}
		mFgsButton->Detach(m_outputIntervalBut);
		delete m_outputIntervalBut;
		if (m_outputModeBut!=NULL) {
			mFgsButton->Detach(m_outputModeBut);
			delete m_outputModeBut;
		}

		mFgs->Detach(mFgsButton);
		delete mFgsButton;
		mSetOutputSizer->Detach(mFgs);
		delete mFgs;
		mBottomSizer->Detach(mSetOutputSizer);
//@		delete mSetOutputSizer;
		delete mSetOutputBox;
	}

    /** \brief set the text of the current resolution value. */
    void setResolutionText ( const wxString s ) {
		mResolutionText2->SetLabel( s );
	}

	// Enable or disable controls that apply only to structure output.
	// 8/29/07 Dewey Odhner
	void EnableStructureOptions(bool flag) {
		m_outputModeBut->Enable(flag);
		m_normalTypeBut->Enable(flag);
		mResolution->Enable(flag);
	}
};

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

/* Implements the search step slider for automatic threshold selection.
 * 7/24/07 Dewey Odhner.
 */
class  SearchStepControls {
	wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
	wxFlexGridSizer*  mFgs;
	wxStaticBox*      mSearchStepBox;
	wxSlider*         mSearchStep;    ///< search step slider
	wxStaticText*     mStepST;        ///< search step label
	wxStaticBoxSizer* mSearchStepSizer;
    wxStaticText*     mParam1Label;
	wxTextCtrl*       mParam1Ctrl;
    wxStaticText*     mParam2Label;
	wxTextCtrl*       mParam2Ctrl;
    wxStaticText*     mParam3Label;
	wxTextCtrl*       mParam3Ctrl;
    wxStaticText*     mParam4Label;
	wxTextCtrl*       mParam4Ctrl;

public:
	SearchStepControls( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, int currentStep, int stepSliderID,
		int param1ID, int param2ID, int param3ID, int param4ID,
		double currentParam1, double currentParam2,
		double currentParam3, double currentParam4)
	{
		mBottomSizer = bottomSizer;
		mSearchStepBox = new wxStaticBox( cp, -1, title );
		::setColor( mSearchStepBox );
		mSearchStepSizer= new wxStaticBoxSizer( mSearchStepBox, wxHORIZONTAL );
		mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
		mFgs->SetMinSize( controlsWidth, 0 );
		mStepST = new wxStaticText( cp, -1, "Search Step:" );
		::setColor( mStepST );
		mFgs->Add( mStepST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mSearchStep = new wxSlider( cp, stepSliderID, currentStep, 1, 100,
			wxDefaultPosition, wxSize(sliderWidth, -1),
			wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
			"search step" );
		::setColor( mSearchStep );
		mSearchStep->SetPageSize( 5 );
		mFgs->Add( mSearchStep, 0, wxGROW|wxLEFT|wxRIGHT );
		mSearchStepSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
				mSearchStep->SetToolTip( "optimization search step" );
            #endif
        #endif

		mParam1Label = new wxStaticText( cp, -1, "Min. Threshold:" );
        ::setColor( mParam1Label );
        mFgs->Add( mParam1Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mParam1Ctrl = new wxTextCtrl( cp, param1ID, 
			wxString::Format("%.0f",currentParam1) );
        ::setColor( mParam1Ctrl );
        mFgs->Add( mParam1Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );
		if (param2ID)
		{
			mParam2Label = new wxStaticText( cp, -1, "Threshold 2:" );
	        ::setColor( mParam2Label );
	        mFgs->Add( mParam2Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
			mParam2Ctrl = new wxTextCtrl( cp, param2ID, 
				wxString::Format("%.0f",currentParam2) );
	        ::setColor( mParam2Ctrl );
	        mFgs->Add( mParam2Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );
			mParam3Label = new wxStaticText( cp, -1, "Threshold 3:" );
	        ::setColor( mParam3Label );
	        mFgs->Add( mParam3Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
			mParam3Ctrl = new wxTextCtrl( cp, param3ID, 
				wxString::Format("%.0f",currentParam3) );
	        ::setColor( mParam3Ctrl );
	        mFgs->Add( mParam3Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );
		}
		else
		{
			mParam2Ctrl = mParam3Ctrl = NULL;
			mParam2Label = mParam3Label = NULL;
		}
		mParam4Label = new wxStaticText( cp, -1, "Max. Threshold:" );
        ::setColor( mParam4Label );
        mFgs->Add( mParam4Label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mParam4Ctrl = new wxTextCtrl( cp, param4ID, 
			wxString::Format("%.0f",currentParam4) );
        ::setColor( mParam4Ctrl );
        mFgs->Add( mParam4Ctrl, 0, wxGROW|wxLEFT|wxRIGHT );

		mBottomSizer->Prepend( mSearchStepSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
	}
	~SearchStepControls()
	{
		mBottomSizer->Detach( mSearchStepSizer );
		mFgs->Detach(mParam4Ctrl);
		delete mParam4Ctrl;
		mFgs->Detach(mParam4Label);
		delete mParam4Label;
		if (mParam3Ctrl)
		{
			mFgs->Detach(mParam3Ctrl);
			delete mParam3Ctrl;
		}
		if (mParam3Label)
		{
			mFgs->Detach(mParam3Label);
			delete mParam3Label;
		}
		if (mParam2Ctrl)
		{
			mFgs->Detach(mParam2Ctrl);
			delete mParam2Ctrl;
		}
		if (mParam2Label)
		{
			mFgs->Detach(mParam2Label);
			delete mParam2Label;
		}
		mFgs->Detach(mParam1Ctrl);
		delete mParam1Ctrl;
		mFgs->Detach(mParam1Label);
		delete mParam1Label;
		mFgs->Detach(mStepST);
		delete mStepST;
		mFgs->Detach(mSearchStep);
		delete mSearchStep;
		mSearchStepSizer->Detach(mFgs);
		delete mFgs;
//@		delete mSearchStepSizer;
		delete mSearchStepBox;
	}
	void setParam1Value(double v)
	{
		mParam1Ctrl->SetValue(wxString::Format("%.0f", v));
	}
	void setParam2Value(double v)
	{
		mParam2Ctrl->SetValue(wxString::Format("%.0f", v));
	}
	void setParam3Value(double v)
	{
		mParam3Ctrl->SetValue(wxString::Format("%.0f", v));
	}
	void setParam4Value(double v)
	{
		mParam4Ctrl->SetValue(wxString::Format("%.0f", v));
	}
};

/* Implements the opacity slider for fuzzy shell creation.
 * 9/21/07 Dewey Odhner.
 */
class  ThrOpacityControls {
	wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
	wxFlexGridSizer*  mFgs;
	wxStaticBox*      mOpacityBox;
	wxSlider*         mOpacity;    ///< opacity slider
	wxStaticText*     mOpacityST;        ///< opacity label
	wxStaticBoxSizer* mOpacitySizer;

public:
	ThrOpacityControls( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, int currentOpacity, int opacitySliderID)
	{
		mBottomSizer = bottomSizer;
		mOpacityBox = new wxStaticBox( cp, -1, title );
		::setColor( mOpacityBox );
		mOpacitySizer= new wxStaticBoxSizer( mOpacityBox, wxHORIZONTAL );
		mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
		mFgs->SetMinSize( controlsWidth, 0 );
		mOpacityST = new wxStaticText( cp, -1, "opacity:" );
		::setColor( mOpacityST );
		mFgs->Add( mOpacityST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
		mOpacity = new wxSlider( cp, opacitySliderID, currentOpacity, 0, 100,
			wxDefaultPosition, wxSize(sliderWidth, -1),
			wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
			"opacity" );
		::setColor( mOpacity );
		mOpacity->SetPageSize( 5 );
		mFgs->Add( mOpacity, 0, wxGROW|wxLEFT|wxRIGHT );
		mOpacitySizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
				mOpacity->SetToolTip( "material opacity" );
            #endif
        #endif
		mBottomSizer->Prepend( mOpacitySizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
	}
	~ThrOpacityControls()
	{
		mBottomSizer->Detach( mOpacitySizer );
		mFgs->Detach(mOpacityST);
		delete mOpacityST;
		mFgs->Detach(mOpacity);
		delete mOpacity;
		mOpacitySizer->Detach(mFgs);
		delete mFgs;
//@		delete mOpacitySizer;
		delete mOpacityBox;
	}
	void SetValue(int newOpacity)
	{
		mOpacity->SetValue(newOpacity);
	}
};

/* Implements the surface strength slider for fuzzy shell creation.
 * 9/20/07 Dewey Odhner.
 */
class  SurfaceStrengthControls {
	wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
	wxFlexGridSizer*  mFgs;
	wxStaticBox*      mSurfaceStrengthBox;
	wxSlider*         mSurfaceStrength;    ///< surface strength slider
	wxStaticText*     mSurfaceStrengthST;        ///< surface strength label
	wxStaticBoxSizer* mSurfaceStrengthSizer;

public:
	SurfaceStrengthControls( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, int currentSurfaceStrength,
		int surfaceStrengthSliderID, int maxSurfaceStrength)
	{
		mBottomSizer = bottomSizer;
		mSurfaceStrengthBox = new wxStaticBox( cp, -1, title );
		::setColor( mSurfaceStrengthBox );
		mSurfaceStrengthSizer =
			new wxStaticBoxSizer( mSurfaceStrengthBox, wxHORIZONTAL );
		mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
		mFgs->SetMinSize( controlsWidth, 0 );
		mSurfaceStrengthST = new wxStaticText( cp, -1, "surface strength:" );
		::setColor( mSurfaceStrengthST );
		mFgs->Add(mSurfaceStrengthST,0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
		mSurfaceStrength = new wxSlider( cp, surfaceStrengthSliderID,
			currentSurfaceStrength, 1, maxSurfaceStrength,
			wxDefaultPosition, wxSize(sliderWidth, -1),
			wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
			"surface strength" );
		::setColor( mSurfaceStrength );
		mSurfaceStrength->SetPageSize( 5 );
		mFgs->Add( mSurfaceStrength, 0, wxGROW|wxLEFT|wxRIGHT );
		mSurfaceStrengthSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
				mSurfaceStrength->SetToolTip( "surface strength" );
            #endif
        #endif
		mBottomSizer->Prepend( mSurfaceStrengthSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
	}
	~SurfaceStrengthControls()
	{
		mBottomSizer->Detach( mSurfaceStrengthSizer );
		mFgs->Detach(mSurfaceStrengthST);
		delete mSurfaceStrengthST;
		mFgs->Detach(mSurfaceStrength);
		delete mSurfaceStrength;
		mSurfaceStrengthSizer->Detach(mFgs);
		delete mFgs;
//@		delete mSurfaceStrengthSizer;
		delete mSurfaceStrengthBox;
	}
};

#endif
