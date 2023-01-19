/*
  Copyright 1993-2015 Medical Image Processing Group
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
 * \file   InhomoControls.h
 * \brief  Definition and implementation of InhomoControls class.
 * \author Xinjian Chen, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __InhomoCtrl_h
#define __InhomoCtrl_h

/**
 * \brief Definition and implementation of Inhomo2Controls class.
 *
 * Inhomo controls consist of a text control for Inhomo
 * sliders and a checkbox for inversion of the gray map.
 *
 * Note: Callbacks are handled by the caller.
 */
class Inhomo1Controls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mInhomoParBox;
    wxSizer*          mInhomoParBoxSizer;
    wxFlexGridSizer*  mInhomoParSizer;

    wxStaticText*     mVolThrST;       //static text for Inhomo
	wxStaticText*     mStopCndST;       //static text for Inhomo
	wxStaticText*     mLeftParST;       //static text for Inhomo
	wxStaticText*     mRightParST;       //static text for Inhomo
	wxStaticText*     mIterationST;       //static text for Inhomo

    wxTextCtrl*       mVolThreshTC;
	wxTextCtrl*       mStopCondTC;
	wxTextCtrl*       mLeftParTC;
	wxTextCtrl*       mRightParamTC;
	wxTextCtrl*       mIterationsTC;

public:
    /** \brief Inhomo1Controls ctor. */
    Inhomo1Controls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int volThreshID, const int StopCondID, const int LeftParID, const int RightParamID, const int IterationsID,
			double volThresh, double stopCond, double leftParam, int rightParam, int iterations)

    {
        mBottomSizer = bottomSizer;
        mInhomoParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mInhomoParBox );
        mInhomoParBoxSizer = new wxStaticBoxSizer( mInhomoParBox, wxHORIZONTAL );
        mInhomoParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mInhomoParSizer->SetMinSize( controlsWidth, 0 );
        mInhomoParSizer->AddGrowableCol( 0 );
        
        //Inhomo
        mVolThrST = new wxStaticText( cp, -1, "Volume Threshold:" );
        ::setColor( mVolThrST );        
        mInhomoParSizer->Add( mVolThrST, 0, wxALIGN_RIGHT );        
        wxString VolThreshStr = wxString::Format("%5.2f",volThresh);
        mVolThreshTC = new wxTextCtrl( cp, volThreshID, VolThreshStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mVolThreshTC );
		mInhomoParSizer->Add( mVolThreshTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mStopCndST = new wxStaticText( cp, -1, "Stop Condition:" );
        ::setColor( mStopCndST );        
        mInhomoParSizer->Add( mStopCndST, 0, wxALIGN_RIGHT );        
		wxString StopCondStr = wxString::Format("%5.2f",stopCond);
        mStopCondTC = new wxTextCtrl( cp, StopCondID, StopCondStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mStopCondTC );
		mInhomoParSizer->Add( mStopCondTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mLeftParST = new wxStaticText( cp, -1, "Left:" );
        ::setColor( mLeftParST );        
        mInhomoParSizer->Add( mLeftParST, 0, wxALIGN_RIGHT );  
		wxString LeftParStr = wxString::Format("%5.2f",leftParam);
        mLeftParTC = new wxTextCtrl( cp, LeftParID, LeftParStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mLeftParTC );
		mInhomoParSizer->Add( mLeftParTC, 0, wxGROW|wxLEFT|wxRIGHT );
		
		mRightParST = new wxStaticText( cp, -1, "Right:" );
        ::setColor( mRightParST );        
        mInhomoParSizer->Add( mRightParST, 0, wxALIGN_RIGHT );  
		wxString RightParamStr = wxString::Format("%d",rightParam);
        mRightParamTC = new wxTextCtrl( cp, RightParamID, RightParamStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mRightParamTC );
		mInhomoParSizer->Add( mRightParamTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mIterationST = new wxStaticText( cp, -1, "Iterations:" );
        ::setColor( mIterationST );        
        mInhomoParSizer->Add( mIterationST, 0, wxALIGN_RIGHT );  
		wxString IterationsStr = wxString::Format("%d",iterations);
        mIterationsTC = new wxTextCtrl( cp, IterationsID, IterationsStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mIterationsTC );
		mInhomoParSizer->Add( mIterationsTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mInhomoParBoxSizer->Add( mInhomoParSizer, 0, wxGROW|wxALL, 10 );        
        mBottomSizer->Prepend( mInhomoParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Inhomo1Controls dtor. */
    ~Inhomo1Controls ( void ) 
	{
		if( mVolThreshTC != NULL )
			delete mVolThreshTC;
		if( mStopCondTC != NULL )
			delete mStopCondTC;
		if( mLeftParTC != NULL )
			delete mLeftParTC;
		if( mRightParamTC != NULL )
			delete mRightParamTC;
		if( mIterationsTC != NULL )
			delete mIterationsTC;
		
		if( mVolThrST != NULL )
			delete mVolThrST;
		if( mStopCndST != NULL )
			delete mStopCndST;
		if( mLeftParST != NULL )
			delete mLeftParST;
		if( mRightParST != NULL )
			delete mRightParST;
		if( mIterationST != NULL )
			delete mIterationST;

		mInhomoParSizer->Clear( true );
		mInhomoParBoxSizer->Clear( true );
		mInhomoParBoxSizer->Remove( mInhomoParSizer );
		mBottomSizer->Remove( mInhomoParBoxSizer );
    }
};

class Inhomo2Controls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mInhomoParBox;
    wxSizer*          mInhomoParBoxSizer;
    wxFlexGridSizer*  mInhomoParSizer;

    wxStaticText*     mZetaST;       //static text for Inhomo
	wxStaticText*     mIncluST;       //static text for Inhomo
	wxStaticText*     mStdDeviaST;       //static text for Inhomo
	wxStaticText*     mNumofRegST;       //static text for Inhomo
	wxStaticText*     mIterationST;       //static text for Inhomo

    wxTextCtrl*       mZetaFactorTC;
	wxTextCtrl*       mIncluFactorTC;
	wxTextCtrl*       mStdDeviaTC;
	wxTextCtrl*       mNumOfRegionTC;
	wxTextCtrl*       mIterationsTC;

public:
    /** \brief Inhomo2Controls ctor. */
    Inhomo2Controls ( wxPanel* cp, wxSizer* bottomSizer, const char* const title,
            const int zetaFactorID, const int IncluFactorID, const int StdDeviaID, const int NumOfRegionID, const int IterationsID,
			double zetaFactor, double incluFactor, double stdDevia, int numOfRegion, int iterations)

    {
        mBottomSizer = bottomSizer;
        mInhomoParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mInhomoParBox );
        mInhomoParBoxSizer = new wxStaticBoxSizer( mInhomoParBox, wxHORIZONTAL );
        mInhomoParSizer = new wxFlexGridSizer( 2, 1, 5 );  //cols,vgap,hgap
        mInhomoParSizer->SetMinSize( controlsWidth, 0 );
        mInhomoParSizer->AddGrowableCol( 0 );
        
        //Inhomo
        mZetaST = new wxStaticText( cp, -1, "Zeta Factor:" );
        ::setColor( mZetaST );        
        mInhomoParSizer->Add( mZetaST, 0, wxALIGN_RIGHT );        
        wxString ZetaFactorStr = wxString::Format("%5.2f",zetaFactor);
        mZetaFactorTC = new wxTextCtrl( cp, zetaFactorID, ZetaFactorStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mZetaFactorTC );
		mInhomoParSizer->Add( mZetaFactorTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mIncluST = new wxStaticText( cp, -1, "Inclusion Factor:" );
        ::setColor( mIncluST );        
        mInhomoParSizer->Add( mIncluST, 0, wxALIGN_RIGHT );        
		wxString IncluFactorStr = wxString::Format("%5.2f",incluFactor);
        mIncluFactorTC = new wxTextCtrl( cp, IncluFactorID, IncluFactorStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mIncluFactorTC );
		mInhomoParSizer->Add( mIncluFactorTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mStdDeviaST = new wxStaticText( cp, -1, "Std Deviation:" );
        ::setColor( mStdDeviaST );        
        mInhomoParSizer->Add( mStdDeviaST, 0, wxALIGN_RIGHT );  
		wxString StdDeviaStr = wxString::Format("%5.2f",stdDevia);
        mStdDeviaTC = new wxTextCtrl( cp, StdDeviaID, StdDeviaStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mStdDeviaTC );
		mInhomoParSizer->Add( mStdDeviaTC, 0, wxGROW|wxLEFT|wxRIGHT );
		
		mNumofRegST = new wxStaticText( cp, -1, "Num of Regions:" );
        ::setColor( mNumofRegST );        
        mInhomoParSizer->Add( mNumofRegST, 0, wxALIGN_RIGHT );  
		wxString NumOfRegionStr = wxString::Format("%d",numOfRegion);
        mNumOfRegionTC = new wxTextCtrl( cp, NumOfRegionID, NumOfRegionStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mNumOfRegionTC );
		mInhomoParSizer->Add( mNumOfRegionTC, 0, wxGROW|wxLEFT|wxRIGHT );

		mIterationST = new wxStaticText( cp, -1, "Iterations:" );
        ::setColor( mIterationST );        
        mInhomoParSizer->Add( mIterationST, 0, wxALIGN_RIGHT );  
		wxString IterationsStr = wxString::Format("%d",iterations);
        mIterationsTC = new wxTextCtrl( cp, IterationsID, IterationsStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mIterationsTC );
		mInhomoParSizer->Add( mIterationsTC, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mInhomoParBoxSizer->Add( mInhomoParSizer, 0, wxGROW|wxALL, 10 );        
        mBottomSizer->Prepend( mInhomoParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Inhomo2Controls dtor. */
    ~Inhomo2Controls ( void ) 
	{
		if( mZetaFactorTC != NULL )
			delete mZetaFactorTC;
		if( mIncluFactorTC != NULL )
			delete mIncluFactorTC;
		if( mStdDeviaTC != NULL )
			delete mStdDeviaTC;
		if( mNumOfRegionTC != NULL )
			delete mNumOfRegionTC;
		if( mIterationsTC != NULL )
			delete mIterationsTC;
		
		if( mZetaST != NULL )
			delete mZetaST;
		if( mIncluST != NULL )
			delete mIncluST;
		if( mStdDeviaST != NULL )
			delete mStdDeviaST;
		if( mNumofRegST != NULL )
			delete mNumofRegST;
		if( mIterationST != NULL )
			delete mIterationST;

		mInhomoParSizer->Clear( true );
		mInhomoParBoxSizer->Clear( true );
		mInhomoParBoxSizer->Remove( mInhomoParSizer );
		mBottomSizer->Remove( mInhomoParBoxSizer );
    }
};

class Inhomo3Controls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mInhomoParBox;
    wxSizer*          mInhomoParBoxSizer;
    wxFlexGridSizer*  mInhomoParSizer;

	wxStaticText*     mIterationST;       //static text for Inhomo
	wxStaticText*     mNumofRegST;       //static text for Inhomo
    wxStaticText      *mMin0ST, *mMin1ST, *mMin2ST, *mMin3ST;
    wxStaticText      *mMax0ST, *mMax1ST, *mMax2ST, *mMax3ST;

	wxTextCtrl*       mIterationsTC;
	wxTextCtrl*       mNumOfRegionTC;
    wxTextCtrl        *mMin0TC, *mMin1TC, *mMin2TC, *mMin3TC;
    wxTextCtrl        *mMax0TC, *mMax1TC, *mMax2TC, *mMax3TC;

public:
    /** \brief Inhomo3Controls ctor. */
    Inhomo3Controls(wxPanel* cp, wxSizer* bottomSizer, const char* const title,
           const int min0ID,const int min1ID,const int min2ID,const int min3ID,
           const int max0ID,const int max1ID,const int max2ID,const int max3ID,
		   const int NumOfRegionID, const int IterationsID,
		   const int min0, const int min1, const int min2, const int min3,
		   const int max0, const int max1, const int max2, const int max3,
		   int numOfRegion, int iterations)

    {
        mBottomSizer = bottomSizer;
        mInhomoParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mInhomoParBox );
        mInhomoParBoxSizer = new wxStaticBoxSizer(mInhomoParBox, wxHORIZONTAL);
        mInhomoParSizer = new wxFlexGridSizer( 4, 1, 5 );  //cols,vgap,hgap
        mInhomoParSizer->SetMinSize( controlsWidth, 0 );
        mInhomoParSizer->AddGrowableCol( 0 );

        //Inhomo

#if 0
		mIterationST = new wxStaticText( cp, -1, "Iterations:" );
        ::setColor( mIterationST );
        mInhomoParSizer->Add( mIterationST, 0, wxALIGN_RIGHT );  
		wxString IterationsStr = wxString::Format("%d",iterations);
        mIterationsTC = new wxTextCtrl( cp, IterationsID, IterationsStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mIterationsTC );
		mInhomoParSizer->Add( mIterationsTC, 0, wxGROW|wxLEFT|wxRIGHT );
#else
		mIterationST = NULL;
		mMin0ST = NULL;
		mMin1ST = NULL;
		mMin2ST = NULL;
		mMin3ST = NULL;
		mMax0ST = NULL;
		mMax1ST = NULL;
		mMax2ST = NULL;
		mMax3ST = NULL;
		mIterationsTC = NULL;
		mMin0TC = NULL;
		mMin1TC = NULL;
		mMin2TC = NULL;
		mMin3TC = NULL;
		mMax0TC = NULL;
		mMax1TC = NULL;
		mMax2TC = NULL;
		mMax3TC = NULL;
#endif

		mNumofRegST = new wxStaticText( cp, -1, "Num of Regions:" );
        ::setColor( mNumofRegST );
        mInhomoParSizer->Add( mNumofRegST, 0, wxALIGN_RIGHT );  
		wxString NumOfRegionStr = wxString::Format("%d",numOfRegion);
        mNumOfRegionTC = new wxTextCtrl( cp, NumOfRegionID, NumOfRegionStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mNumOfRegionTC );
		mInhomoParSizer->Add( mNumOfRegionTC, 0, wxGROW|wxLEFT|wxRIGHT );

#if 0
        mMin0ST = new wxStaticText( cp, -1, "Min 1:" );
        ::setColor( mMin0ST );
        mInhomoParSizer->Add( mMin0ST, 0, wxALIGN_RIGHT );
        wxString Min0Str = wxString::Format("%d",min0);
        mMin0TC = new wxTextCtrl( cp, min0ID, Min0Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMin0TC );
		mInhomoParSizer->Add( mMin0TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMax0ST = new wxStaticText( cp, -1, "Max 1:" );
        ::setColor( mMax0ST );
        mInhomoParSizer->Add( mMax0ST, 0, wxALIGN_RIGHT );
        wxString Max0Str = wxString::Format("%d",max0);
        mMax0TC = new wxTextCtrl( cp, max0ID, Max0Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMax0TC );
		mInhomoParSizer->Add( mMax0TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMin1ST = new wxStaticText( cp, -1, "Min 2:" );
        ::setColor( mMin1ST );
        mInhomoParSizer->Add( mMin1ST, 0, wxALIGN_RIGHT );
        wxString Min1Str = wxString::Format("%d",min1);
        mMin1TC = new wxTextCtrl( cp, min1ID, Min1Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMin1TC );
		mInhomoParSizer->Add( mMin1TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMax1ST = new wxStaticText( cp, -1, "Max 2:" );
        ::setColor( mMax1ST );
        mInhomoParSizer->Add( mMax1ST, 0, wxALIGN_RIGHT );
        wxString Max1Str = wxString::Format("%d",max1);
        mMax1TC = new wxTextCtrl( cp, max1ID, Max1Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMax1TC );
		mInhomoParSizer->Add( mMax1TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMin2ST = new wxStaticText( cp, -1, "Min 3:" );
        ::setColor( mMin2ST );
        mInhomoParSizer->Add( mMin2ST, 0, wxALIGN_RIGHT );
        wxString Min2Str = wxString::Format("%d",min2);
        mMin2TC = new wxTextCtrl( cp, min2ID, Min2Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMin2TC );
		mInhomoParSizer->Add( mMin2TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMax2ST = new wxStaticText( cp, -1, "Max 3:" );
        ::setColor( mMax2ST );
        mInhomoParSizer->Add( mMax2ST, 0, wxALIGN_RIGHT );
        wxString Max2Str = wxString::Format("%d",max2);
        mMax2TC = new wxTextCtrl( cp, max2ID, Max2Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMax2TC );
		mInhomoParSizer->Add( mMax2TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMin3ST = new wxStaticText( cp, -1, "Min 4:" );
        ::setColor( mMin3ST );
        mInhomoParSizer->Add( mMin3ST, 0, wxALIGN_RIGHT );
        wxString Min3Str = wxString::Format("%d",min3);
        mMin3TC = new wxTextCtrl( cp, min3ID, Min3Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMin3TC );
		mInhomoParSizer->Add( mMin3TC, 0, wxGROW|wxLEFT|wxRIGHT );

        mMax3ST = new wxStaticText( cp, -1, "Max 4:" );
        ::setColor( mMax3ST );
        mInhomoParSizer->Add( mMax3ST, 0, wxALIGN_RIGHT );
        wxString Max3Str = wxString::Format("%d",max3);
        mMax3TC = new wxTextCtrl( cp, max3ID, Max3Str,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMax3TC );
		mInhomoParSizer->Add( mMax3TC, 0, wxGROW|wxLEFT|wxRIGHT );
#endif

        mInhomoParBoxSizer->Add( mInhomoParSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Prepend( mInhomoParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Inhomo3Controls dtor. */
    ~Inhomo3Controls ( void ) 
	{
		if( mMax3TC != NULL )
			delete mMax3TC;
		if( mMax2TC != NULL )
			delete mMax2TC;
		if( mMax1TC != NULL )
			delete mMax1TC;
		if( mMax0TC != NULL )
			delete mMax0TC;
		if( mMin3TC != NULL )
			delete mMin3TC;
		if( mMin2TC != NULL )
			delete mMin2TC;
		if( mMin1TC != NULL )
			delete mMin1TC;
		if( mMin0TC != NULL )
			delete mMin0TC;
		if( mNumOfRegionTC != NULL )
			delete mNumOfRegionTC;
		if( mIterationsTC != NULL )
			delete mIterationsTC;

		if( mMax0ST != NULL )
			delete mMax0ST;
		if( mMax1ST != NULL )
			delete mMax1ST;
		if( mMax2ST != NULL )
			delete mMax2ST;
		if( mMax3ST != NULL )
			delete mMax3ST;
		if( mMin0ST != NULL )
			delete mMin0ST;
		if( mMin1ST != NULL )
			delete mMin1ST;
		if( mMin2ST != NULL )
			delete mMin2ST;
		if( mMin3ST != NULL )
			delete mMin3ST;
		if( mNumofRegST != NULL )
			delete mNumofRegST;
		if( mIterationST != NULL )
			delete mIterationST;

		mInhomoParSizer->Clear( true );
		mInhomoParBoxSizer->Clear( true );
		mInhomoParBoxSizer->Remove( mInhomoParSizer );
		mBottomSizer->Remove( mInhomoParBoxSizer );
    }
};

class BallEnhanceControls {
    wxSizer*          mBottomSizer;  //DO NOT DELETE in dtor!

    wxStaticBox*      mBallEnhParBox;
    wxSizer*          mBallEnhParBoxSizer;
    wxFlexGridSizer*  mBallEnhParSizer;

	wxStaticText*     mMaxRadST;       //static text for BallEnh
    wxStaticText*     mMinRadST;       //static text for BallEnh

	wxTextCtrl*       mMaxRadiusTC;
    wxTextCtrl*       mMinRadiusTC;

public:
    /** \brief BallEnhanceControls ctor. */
    BallEnhanceControls(wxPanel* cp, wxSizer* bottomSizer,
           const char* const title, const int MaxRadiusID, int maxRadius,
           const int MinRadiusID, int minRadius)

    {
        mBottomSizer = bottomSizer;
        mBallEnhParBox = new wxStaticBox( cp, -1, title );
        ::setColor( mBallEnhParBox );
        mBallEnhParBoxSizer = new wxStaticBoxSizer(mBallEnhParBox, wxHORIZONTAL);
        mBallEnhParSizer = new wxFlexGridSizer( 4, 1, 5 );  //cols,vgap,hgap
        mBallEnhParSizer->SetMinSize( controlsWidth, 0 );
        mBallEnhParSizer->AddGrowableCol( 0 );

        //BallEnh

        mMaxRadST = new wxStaticText( cp, -1, "Max Radius:" );
        ::setColor( mMaxRadST );
        mBallEnhParSizer->Add( mMaxRadST, 0, wxALIGN_RIGHT );  
        wxString MaxRadiusStr = wxString::Format("%d",maxRadius);
        mMaxRadiusTC = new wxTextCtrl( cp, MaxRadiusID, MaxRadiusStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMaxRadiusTC );
        mBallEnhParSizer->Add( mMaxRadiusTC, 0, wxGROW|wxLEFT|wxRIGHT );
        mMinRadST = new wxStaticText( cp, -1, "Min Radius:" );
        ::setColor( mMinRadST );
        mBallEnhParSizer->Add( mMinRadST, 0, wxALIGN_RIGHT );
        wxString MinRadiusStr = wxString::Format("%d",minRadius);
        mMinRadiusTC = new wxTextCtrl( cp, MinRadiusID, MinRadiusStr,wxDefaultPosition, wxDefaultSize);
        ::setColor( mMinRadiusTC );
		mBallEnhParSizer->Add( mMinRadiusTC, 0, wxGROW|wxLEFT|wxRIGHT );


        mBallEnhParBoxSizer->Add( mBallEnhParSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Prepend( mBallEnhParBoxSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief BallEnhanceControls dtor. */
    ~BallEnhanceControls ( void ) 
    {
        if( mMinRadiusTC != NULL )
            delete mMinRadiusTC;

        if( mMinRadST != NULL )
            delete mMinRadST;

        if( mMaxRadiusTC != NULL )
            delete mMaxRadiusTC;

        if( mMaxRadST != NULL )
            delete mMaxRadST;

        mBallEnhParSizer->Clear( true );
        mBallEnhParBoxSizer->Clear( true );
        mBallEnhParBoxSizer->Remove( mBallEnhParSizer );
        mBottomSizer->Remove( mBallEnhParBoxSizer );
    }
};

#endif
