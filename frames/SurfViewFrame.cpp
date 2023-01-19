/*
  Copyright 1993-2020 Medical Image Processing Group
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
 * \file   SurfViewFrame.cpp
 * \brief  SurfViewFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

#include  "SurfOpacityControls.h"
#include  "SurfPropertiesControls.h"
#include  "SurfSpeedControls.h"

#define mControlsHeight ((buttonHeight+1)*7+50)

extern Vector  gFrameList;
extern int     gTimerInterval;

//----------------------------------------------------------------------
/** \brief Constructor for SurfViewFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
SurfViewFrame::SurfViewFrame ( bool maximize, int w, int h, bool surface, bool manipulate )
  : MainFrame( 0 )
{
    mFileOrDataCount = 0;

    mAmbientControls    = NULL;
    mBackgroundControls = NULL;
    mColorControls      = NULL;
    mDiffuseControls    = NULL;
    mLight              = NULL;
    mMagnifyControls    = NULL;
    mObject             = NULL;
    mOpacityControls    = NULL;
    mSaveScreenControls = NULL;
    mSpecularControls   = NULL;
    mSpeedControls      = NULL;
	mRotate             = NULL;
	mCutDepthControls   = NULL;
	mStatus             = NULL;
	mMobile             = NULL;
	mSceneStatus        = NULL;
	mSliceOutputControls= NULL;
	mTumbleControls     = NULL;
	mVRControls         = NULL;
	mGrayMapControls    = NULL;

    mScale              = -1;
    mSpeed              = (speedSliderMax - speedSliderDefault + 1) * 2.0;
    mWhichLight         = 1;
    mWhichObject        = 1;
    mSurface            = surface;
    manip_flag          = manipulate;
	matched_output      = false;
	intermediate_views  = 10;
	mWhichMaterial      = 0;
	all_objects_selected= false;
	mMIP                = false;
	mMIP_Invert         = false;

    ::gFrameList.push_back( this );
    gWhere += 20;
    if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = 20;
    
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -mControlsHeight );
    ::setColor( mSplitter );

    //top: image(s)  - - - - - - - - - - - - - - - - - - - - - - - - - -
    //wxSize  canvasSize( GetSize().GetWidth(), GetSize().GetHeight()-dControlsHeight );

    mCanvas = new SurfViewCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize, manipulate );
    if (Preferences::getCustomAppearance()) {
        mCanvas->SetBackgroundColour( wxColour(DkBlue) );
        mCanvas->SetForegroundColour( wxColour(Yellow) );
    }
    wxSizer*  topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize( 700, 400 );
    topSizer->Add( mCanvas, 1, wxGROW );

    //bottom: controls - - - - - - - - - - - - - - - - - - - - - - - - -
    mControlPanel = new wxPanel( mSplitter, -1, wxDefaultPosition, wxDefaultSize );
    ::setColor( mControlPanel );
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -mControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    //          mainSizer->Add( middleSizer, 1,
    //                          wxGROW | (wxALL & ~(wxTOP | wxBOTTOM)), 10 );
    //          mainSizer->Add( 0, 5, 0, wxGROW ); // spacer in between
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)    Maximize( true );
    else             SetSize( w, h );
    mSplitter->SetSashPosition( -mControlsHeight );
    Show();
    Raise();
#ifdef WIN32
    //DragAcceptFiles( true );
#endif
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
	if (manipulate)
	{
	    SetStatusText( "Draw curve",     2 );
	    SetStatusText( "Select Object", 3 );
	    SetStatusText( "", 4 );
    }
	else
	{
		SetStatusText( "Scroll",        2 );
		SetStatusText( "Select Object", 3 );
		SetStatusText( "Rotate Object", 4 );
	}
	wxToolTip::Enable( Preferences::getShowToolTips() );
    //will this work for linux/gtk?
    mSplitter->SetSashPosition( -mControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void SurfViewFrame::initializeMenu ( void ) {
    //init menu bar and menu items
    MainFrame::initializeMenu();

    ::copyWindowTitles( this );
    wxString  tmp;
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    if (mSurface)  tmp = wxString::Format( "CAVASS:SurfView:%d", j );
    else           tmp = wxString::Format( "CAVASS:VolView:%d",  j );
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}

//----------------------------------------------------------------------
void SurfViewFrame::OnInformation ( wxCommandEvent& unused ) {
    InformationDialog  id( mFilenames );
    id.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class manipObjectSwitch: public wxComboBox {
public:
	manipObjectSwitch( wxPanel *cp, SurfViewFrame *svf ) : wxComboBox( )
	{
		SurfViewCanvas *canvas = dynamic_cast<SurfViewCanvas*>(svf->mCanvas);
		cvRenderer *rr = canvas->mRenderer;
		wxArrayString sa;
		wxString so, to;
		Shell *obj = rr->object_list;
		for (int j=1; obj; j++, obj=obj->next)
		{
			to = wxString::Format("%d", j);
			sa.Add(to);
			if (rr->selected_object == j)
				so = to;
			if (obj->reflection)
			{
				sa.Add(wxString::Format("-%d", j));
				if (rr->selected_object == -j)
					so = wxString::Format("-%d", j);
			}
		}
		sa.Add("All");
		Create( cp, SurfViewFrame::ID_OBJECT, so, wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight), sa, wxCB_READONLY );
		::setColor( this );
	}
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#define        depthIMin            0
#define        depthIDenominator  100
class CutDepthControls {
    wxSizer*          mBottomSizer;   //DO NOT DELETE in dtor!
    wxStaticBox*      mCutDepthBox;
    wxFlexGridSizer*  mFgs;
    wxFlexGridSizer*  mFgsSlider;
    wxSlider*         mDepth;         ///< depth slider
    wxStaticText*     mDepthText0;    ///< depth slider label
    wxStaticText*     mDepthText1;    ///< depth min value
    wxStaticText*     mDepthText2;    ///< depth current value
    wxStaticText*     mDepthText3;    ///< depth max value
    wxSizer*          mCutDepthSizer;
    wxBitmapButton*   mZoomIn;        ///< \todo zoomable sliders
    wxBitmapButton*   mZoomOut;       ///< \todo zoomable sliders
    int               m_old_depth_value;
    double            mDepthZoom;
	int               depthIMax;
public:
    /** \brief CutDepthControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param title          is the title for the control area/box.
     *  \param depthSliderID  is the ID of the depth slider.
     *  \param currentDepth   is the current depth value.
     *  \param zoomOutID      is the ID of the zoom out button (or -1).
     *  \param zoomInID       is the ID of the zoom in button (or -1).
     */
    CutDepthControls ( wxPanel* cp, wxSizer* bottomSizer,
		const char* const title, const int depthSliderID,
		const double maxDepth, const double currentDepth,
        const int zoomOutID=-1, const int zoomInID=-1 )
    {
        mBottomSizer = bottomSizer;
        mCutDepthBox = new wxStaticBox( cp, -1, title );
        ::setColor( mCutDepthBox );
        mCutDepthSizer = new wxStaticBoxSizer( mCutDepthBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //depth
        mDepthText0 = new wxStaticText( cp, -1, "depth:" );
        ::setColor( mDepthText0 );
        mFgs->Add( mDepthText0, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        
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
		depthIMax = (int)(maxDepth*depthIDenominator);
        m_old_depth_value = (int)(currentDepth*depthIDenominator);
        mDepthZoom = 1.0;
        mDepth = new wxSlider( cp, depthSliderID,
            (int)(currentDepth*100+0.5), depthIMin, depthIMax,
            wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_TOP, wxDefaultValidator,
            "depth" );
        ::setColor( mDepth );
        mDepth->SetPageSize( 10 );  //for incr/decr of 0.1
        mFgsSlider->Add( mDepth, 0, wxGROW|wxLEFT|wxRIGHT );
        if (zoomOutID != -1) {
            mZoomIn = new wxBitmapButton( cp, OverlayFrame::ID_ZOOM_IN, wxBitmap( button_zoomin15_xpm ), wxDefaultPosition, wxSize(15,15) );
            ::setColor( mZoomIn );
            mFgsSlider->Add( mZoomIn, 0, wxALIGN_LEFT );
        } else {
            mZoomIn = NULL;
            mFgsSlider->AddSpacer( -1 );
        }
        
        wxString  s = wxString::Format( "%5.2f", depthIMin/100.0 );
        mDepthText1 = new wxStaticText( cp, -1, s );
        ::setColor( mDepthText1 );
        mFgsSlider->Add( mDepthText1, 0, wxALIGN_RIGHT );
        
        s = wxString::Format( "%5.2f", currentDepth );
        mDepthText2 = new wxStaticText( cp, -1, s );
        ::setColor( mDepthText2 );
        mFgsSlider->Add( mDepthText2, 0, wxALIGN_CENTER );
        
        s = wxString::Format( "%5.2f", depthIMax/100.0 );
        mDepthText3 = new wxStaticText( cp, -1, s );
        ::setColor( mDepthText3 );
        mFgsSlider->Add( mDepthText3, 0, wxALIGN_LEFT );
        
        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        
        mCutDepthSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
        // - - - - - - - - - -
        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mDepth->SetToolTip( "depth" );
            #endif
        #endif
        mBottomSizer->Prepend( mCutDepthSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief get the current depth value. */
    int GetValue ( void ) const { return mDepth->GetValue(); }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief set the text of the current depth value. */
    void setDepthText ( const wxString s ) {  mDepthText2->SetLabel( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief CutDepthControls dtor. */
    ~CutDepthControls ( void ) {
        mBottomSizer->Remove( mCutDepthSizer );
        //delete mFgs;
        //delete mFgsSlider;
        //delete mCutDepthBox;
        delete mDepth;
        delete mDepthText0;
        delete mDepthText1;
        delete mDepthText2;
        delete mDepthText3;
        if (mZoomIn  != NULL)    delete mZoomIn;
        if (mZoomOut != NULL)    delete mZoomOut;
    }
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  SliceOutputControls {
	wxSizer          *mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mSliceOutputBox;
    wxSizer*          mSliceOutputSizer;
    Segment2dSlider  *mPixelSize;         ///< pixel size slider
	Segment2dSlider  *mSliceSpacing;      ///< slice spacing slider
	wxButton         *mPrevious;
	wxButton         *mNext;
public:
    /** \brief SliceOutputControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param minPixelSize   is the minimum pixel size value.
	 *  \param maxPixelSize   is the maximum pixel size value.
     *  \param currentPixelSize   is the current pixel size value.
     *  \param minSliceSpacing  is the minimum slice spacing value.
	 *  \param maxSliceSpacing  is the maximum slice spacing value.
     *  \param currentSliceSpacing  is the current slice spacing value.
     */
    SliceOutputControls ( wxPanel* cp, wxSizer* bottomSizer,
		SurfViewCanvas *canvas )
    {
		double minPixelSize, maxPixelSize;
		float minSliceSpacing, maxSliceSpacing;
		minPixelSize =  minSliceSpacing =
			canvas->mRenderer->scene[0].vh.scn.xypixsz[0]/2;
		//@ assuming scene dimension == 3
        maxSliceSpacing = (float)
			((canvas->mRenderer->scene[0].vh.scn.loc_of_subscenes[
			canvas->mRenderer->scene[0].vh.scn.num_of_subscenes[0]-1]-
			canvas->mRenderer->scene[0].vh.scn.loc_of_subscenes[0])*0.5);
		if (maxSliceSpacing > 0)
			maxPixelSize= (canvas->mRenderer->scene[0].vh.scn.loc_of_subscenes[
				1]-canvas->mRenderer->scene[0].vh.scn.loc_of_subscenes[0])*2;
		else
			maxPixelSize = maxSliceSpacing = (float)minPixelSize*10;
		if (canvas->mRenderer->out_slice_spacing <= 0)
			canvas->mRenderer->out_slice_spacing =
				1/canvas->mRenderer->scale;
		if (canvas->out_pixel_size <= 0)
			canvas->out_pixel_size = canvas->mRenderer->out_slice_spacing;
		if (canvas->mRenderer->out_slice_spacing < minSliceSpacing)
			canvas->mRenderer->out_slice_spacing = minSliceSpacing;
		if (canvas->mRenderer->out_slice_spacing > maxSliceSpacing)
			canvas->mRenderer->out_slice_spacing = maxSliceSpacing;
		if (canvas->out_pixel_size < minPixelSize)
			canvas->out_pixel_size = minPixelSize;
		if (canvas->out_pixel_size > maxPixelSize)
			canvas->out_pixel_size = maxPixelSize;
		mBottomSizer = bottomSizer;
        mSliceOutputBox = new wxStaticBox( cp, -1, "Slice Output Controls" );
        ::setColor( mSliceOutputBox );
        mSliceOutputSizer= new wxStaticBoxSizer(mSliceOutputBox, wxHORIZONTAL);
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
		mPixelSize = new Segment2dSlider( cp, mFgs, "Pixel Size",
			SurfViewFrame::ID_PIXEL_SIZE_SLIDER, minPixelSize,
			maxPixelSize, canvas->out_pixel_size );
		mSliceSpacing = new Segment2dSlider( cp, mFgs, "Slice Spacing",
			SurfViewFrame::ID_SLICE_SPACING_SLIDER, minSliceSpacing,
			maxSliceSpacing, canvas->mRenderer->out_slice_spacing );
		mPrevious = new wxButton( cp, SurfViewFrame::ID_BACKWARD_SLICE,
			"Previous", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
		::setColor( mPrevious );
		mFgs->Add( mPrevious, 0, wxRIGHT );
        mNext = new wxButton( cp, SurfViewFrame::ID_FORWARD_SLICE, "Next",
			wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
		::setColor( mNext );
		mFgs->Add( mNext, 0, wxRIGHT );
        mSliceOutputSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
		mBottomSizer->Prepend( mSliceOutputSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SliceOutputControls dtor. */
    ~SliceOutputControls ( void ) {
        //delete mFgs;
        //delete mSliceOutputBox;
		delete mNext;
		mNext = NULL;
		delete mPrevious;
		mPrevious = NULL;
        delete mPixelSize;
		mPixelSize = NULL;
        delete mSliceSpacing;
		mSliceSpacing = NULL;
        mBottomSizer->Remove( mSliceOutputSizer );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void UpdateSliceSpacing(double currentSliceSpacing)
	{
		mSliceSpacing->UpdateValue(currentSliceSpacing);
	}
	void UpdatePixelSize(double currentPixelSize)
	{
		mPixelSize->UpdateValue(currentPixelSize);
	}
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class  TumbleControls {
	wxSizer          *mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer*  mFgs;
    wxStaticBox*      mTumbleBox;
    wxSizer*          mTumbleSizer;
    wxCheckBox*       mCB;            ///< preview on/off checkbox
    wxFlexGridSizer*  mFgsSlider;
    wxComboBox       *mPoses;
	wxComboBox       *mIconMode;
    wxStaticText*     mPreviewST;     ///< preview checkbox label
    wxSlider*         mViewsNumber;   ///< views number slider
    wxStaticText*     mViewsST;       ///< views number slider label
	wxSizer          *mGsLastRow;
	wxStaticText     *mPosesST;
	wxButton         *mDeletePose;
public:
    /** \brief TumbleControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param currentViews   is the current number of intermediate views.
     *  \param currentPreview is currently whether in preview mode.
     *  \param currentAllPoses is currently if intermediate views previewed.
     *  \param currentIconMode is currently if preview is of icons.
     */
    TumbleControls ( wxPanel* cp, wxSizer* bottomSizer, int currentViews,
		bool currentPreview, bool currentAllPoses, bool currentIconMode=false )
    {
        mBottomSizer = bottomSizer;
        mTumbleBox = new wxStaticBox( cp, -1, "Tumble Controls" );
        ::setColor( mTumbleBox );
        mTumbleSizer = new wxStaticBoxSizer( mTumbleBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //views number
        mViewsST = new wxStaticText( cp, -1, "Intermediate\nViews:" );
        ::setColor( mViewsST );
        mFgs->Add( mViewsST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );
        mViewsNumber = new wxSlider( cp, SurfViewFrame::ID_INTR_VIEWS_SLIDER,
			currentViews, 0, 200, wxDefaultPosition, wxSize(sliderWidth, -1),
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_TOP, wxDefaultValidator,
            "intermediate views" );
        ::setColor( mViewsNumber );
        mViewsNumber->SetPageSize( 5 );
        mFgs->Add( mViewsNumber, 0, wxGROW|wxLEFT|wxRIGHT );

        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );

        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        mTumbleSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        mFgs->AddSpacer( ButtonOffset );
        mFgs->AddSpacer( ButtonOffset );
        mGsLastRow = new wxGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mPreviewST = new wxStaticText( cp, -1, "Preview Mode:" );
        ::setColor( mPreviewST );
        mGsLastRow->Add( mPreviewST, 0, wxALIGN_RIGHT );
        mCB = new wxCheckBox( cp, SurfViewFrame::ID_PREVIEW, "on/off" );
        ::setColor( mCB );
        mCB->SetValue( currentPreview );
        mGsLastRow->Add( mCB, 0, wxALIGN_LEFT );

		mPosesST = new wxStaticText( cp, -1, "Preview Poses:" );
		::setColor( mPosesST );
		mGsLastRow->Add( mPosesST, 0, wxALIGN_RIGHT );
		wxArrayString posesOptions;
		posesOptions.Add("All");
		posesOptions.Add("Key");
        mPoses = new wxComboBox( cp, SurfViewFrame::ID_POSES,
			currentAllPoses? "All":"Key", wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight), posesOptions, wxCB_READONLY );
        ::setColor( mPoses );
        mGsLastRow->Add( mPoses, 0, wxALIGN_RIGHT );

		mDeletePose = new wxButton( cp, SurfViewFrame::ID_DELETE_POSE,
			"Delete Pose", wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight) );
		::setColor( mDeletePose );
		mGsLastRow->Add( mDeletePose, 0, wxALIGN_RIGHT );

        mFgs->Add( mGsLastRow );


        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                if (mCB!=NULL)
					mCB->SetToolTip( "toggle preview mode on & off" );
                if (mPreviewST!=NULL)
					mPreviewST->SetToolTip( "toggle preview mode on & off" );
                mViewsNumber->SetToolTip( "number of intermediate views" );
            #endif
        #endif
        mBottomSizer->Prepend( mTumbleSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    TumbleControls ( wxPanel* cp, wxSizer* bottomSizer,
		bool currentAllPoses, bool currentIconMode=false )
    {
        mBottomSizer = bottomSizer;
        mTumbleBox = new wxStaticBox( cp, -1, "Previous Sequence Controls" );
        ::setColor( mTumbleBox );
        mTumbleSizer = new wxStaticBoxSizer( mTumbleBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 0, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );

        //views number
        mViewsST = NULL;
        mFgs->AddSpacer( ButtonOffset );
        mViewsNumber = NULL;
        mFgs->AddSpacer( ButtonOffset );

        mFgsSlider = new wxFlexGridSizer( 3, 0, 0 );  //3 cols,vgap,hgap
        mFgsSlider->AddGrowableCol( 0 );

        mFgs->Add( mFgsSlider, 0, wxGROW|wxLEFT|wxRIGHT );
        mTumbleSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        mFgs->AddSpacer( ButtonOffset );
        mFgs->AddSpacer( ButtonOffset );
        mGsLastRow = new wxGridSizer( 2, 0, 0 );  //2 cols,vgap,hgap
        mPreviewST = NULL;
        mGsLastRow->AddSpacer( ButtonOffset );
        mCB = NULL;
        mGsLastRow->AddSpacer( ButtonOffset );

		mPosesST = new wxStaticText( cp, -1, "Preview Poses:" );
		::setColor( mPosesST );
		mGsLastRow->Add( mPosesST, 0, wxALIGN_RIGHT );
		wxArrayString posesOptions;
		posesOptions.Add("All");
		posesOptions.Add("Key");
        mPoses = new wxComboBox( cp, SurfViewFrame::ID_POSES,
			currentAllPoses? "All":"Key", wxDefaultPosition,
			wxSize(buttonWidth,buttonHeight), posesOptions, wxCB_READONLY );
        ::setColor( mPoses );
        mGsLastRow->Add( mPoses, 0, wxALIGN_RIGHT );

		mDeletePose = NULL;
		mGsLastRow->AddSpacer( ButtonOffset );

        mFgs->Add( mGsLastRow );

        mBottomSizer->Prepend( mTumbleSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief get the current depth value. */
    int GetValue ( void ) const { return mViewsNumber?
		mViewsNumber->GetValue(): -1; }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief since the (external) key pose deletion might change
     *  the current views, the following method will allow an update of
     *  the views slider.
	 *  \param currenViews must be in [0..200].
     */
    void setViewsNo ( const int currentViews ) {
		if (mViewsNumber == NULL)
			return;
		assert( 0<=currentViews && currentViews<=200 );
        mViewsNumber->SetValue( currentViews );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief TumbleControls dtor. */
    ~TumbleControls ( void ) {
		if (mDeletePose)
			mGsLastRow->Detach( mDeletePose );
		delete mDeletePose;
		mGsLastRow->Detach( mPoses );
		delete mPoses;
		if (mCB)
			mGsLastRow->Detach( mCB );
		delete mCB;
		if (mPreviewST)
			mFgs->Detach( mPreviewST );
		delete mPreviewST;
		if (mViewsNumber)
			mFgs->Detach( mViewsNumber );
		delete mViewsNumber;
		if (mViewsST)
			mGsLastRow->Detach( mViewsST );
		delete mViewsST;
		mGsLastRow->Detach( mPosesST );
		delete mPosesST;
		mFgs->Remove( mFgsSlider );
		mTumbleSizer->Remove( mFgs );
		mBottomSizer->Remove( mTumbleSizer );
    }
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
wxString SurfViewFrame::modeName( int mode ) {
	switch (mode) {
		case SEPARATE:
			return "Separate";
		case FUZZY_CONNECT:
			return "Fuzzy Connect";
		case SurfViewCanvas::VIEW:
			return "View";
		case SurfViewCanvas::SELECT_SLICE:
			return "Select Slice";
		case SurfViewCanvas::MEASURE:
			return "Measure";
		case SurfViewCanvas::ROI_STATISTICS:
			return "ROI Statistics";
		case SurfViewCanvas::REFLECT:
			return "Reflect";
		case SurfViewCanvas::CUT_PLANE:
			return "Cut Plane";
		case SurfViewCanvas::CUT_CURVED:
			return "Cut Curved";
		case SurfViewCanvas::MOVE:
			return "Move";
		case SurfViewCanvas::CREATE_MOVIE_TUMBLE:
			return "Create Movie Tumble";
		case SurfViewCanvas::PREVIOUS_SEQUENCE:
			return "Create Movie Prev.Seq.";
		default:
			return "";
	}
	return "";
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::updateObjectSwitch ( void ) {
	if (mObject)
	{
		bfgs->Detach(mObject);
		delete mObject;
	}
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	mWhichObject = canvas->mRenderer->selected_object;
	mObject = new manipObjectSwitch(mControlPanel, this);
	bfgs->Insert(4, mObject, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);
	if (mDiffuseControls || mSpecularControls || (mSurface && (mColorControls
			|| mOpacityControls)))
		removeAll();
	if (canvas->mWhichMode==SurfViewCanvas::SELECT_SLICE ||
			canvas->mWhichMode==SurfViewCanvas::MEASURE ||
			canvas->mWhichMode==SurfViewCanvas::ROI_STATISTICS)
		displayGrayMapControls();
	mBottomSizer->Layout();
	mStatus->SetValue(canvas->mRenderer->object_from_label(mWhichObject)->on !=
		0);
	mMobile->SetValue(canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject))->mobile != 0);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::displayGrayMapControls()
{
	delete mGrayMapControls;
	mGrayMapControls = NULL;
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	float gray_min, gray_max=0, gray_level, gray_width;
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
		{
			gray_min = canvas->mRenderer->slice_list->gray_min[0];
			gray_max = canvas->mRenderer->slice_list->gray_max[0];
			gray_level = canvas->mRenderer->slice_list->gray_level[0];
			gray_width = canvas->mRenderer->slice_list->gray_width[0];
		}
		else if (canvas->mRenderer->slice_list->object[1] == obj)
		{
			gray_min = canvas->mRenderer->slice_list->gray_min[1];
			gray_max = canvas->mRenderer->slice_list->gray_max[1];
			gray_level = canvas->mRenderer->slice_list->gray_level[1];
			gray_width = canvas->mRenderer->slice_list->gray_width[1];
		}
	}
	if (gray_max)
		mGrayMapControls = new GrayMapControls(mControlPanel, mBottomSizer,
			"GrayMap", (int)gray_level, (int)gray_width, (int)gray_max,
			false, ID_CENTER1_SLIDER, ID_WIDTH1_SLIDER, wxID_ANY,
			ID_CT_LUNG, ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::updateIntermediateViews ( void ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (mTumbleControls == NULL)
		return;
	intermediate_views =
		canvas->mControlState==12 && canvas->preview_key_pose>=0?
		canvas->key_pose[canvas->preview_key_pose].views:
		canvas->key_pose[canvas->key_poses-1].views;
	mTumbleControls->setViewsNo(intermediate_views);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnDeletePose ( wxCommandEvent& unused ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (canvas->key_poses == 0)
		return;
	SetStatusText(
		wxString::Format( "Key pose %d deleted.", canvas->key_poses ), 0 );
	canvas->key_poses--;
	if (canvas->key_poses == 0)
		return;
	if (canvas->key_pose[canvas->key_poses-1].views != intermediate_views)
	{
		intermediate_views = canvas->key_pose[canvas->key_poses-1].views;
		if (mTumbleControls)
			mTumbleControls->setViewsNo(intermediate_views);
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::displaySliceOutputControls ( void ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (mSliceOutputControls==NULL &&
			canvas->mRenderer->scene[0].vh.scn.dimension==3)
		mSliceOutputControls = new SliceOutputControls( mControlPanel,
			mBottomSizer, canvas );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 5, 10, wxGROW );  //spacer
    m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    wxSizer*  buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    bfgs = new wxFlexGridSizer( 3, 1, 5 );  //3 cols,vgap,hgap

    //row 1, col 1
	wxStaticText *st1 = new wxStaticText( mControlPanel, -1, "Mode:" );
    ::setColor( st1 );
    bfgs->Add( st1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 1, col 2
	wxArrayString modenames;
	if (manip_flag)
		modenames.Add(modeName(SurfViewCanvas::SELECT_SLICE));
	else
		modenames.Add(modeName(SurfViewCanvas::VIEW));
	modenames.Add(modeName(SurfViewCanvas::MEASURE));
	if (manip_flag)
	{
		modenames.Add(modeName(SurfViewCanvas::ROI_STATISTICS));
		modenames.Add(modeName(SurfViewCanvas::REFLECT));
		modenames.Add(modeName(SurfViewCanvas::CUT_PLANE));
		modenames.Add(modeName(SurfViewCanvas::CUT_CURVED));
		modenames.Add(modeName(SEPARATE));
		modenames.Add(modeName(SurfViewCanvas::MOVE));
	}
	modenames.Add(modeName(SurfViewCanvas::CREATE_MOVIE_TUMBLE));
	modenames.Add(modeName(SurfViewCanvas::PREVIOUS_SEQUENCE));
	SurfViewCanvas *canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	wxComboBox *mode = new wxComboBox( mControlPanel, ID_MODE,
		modeName(canvas->mWhichMode), wxDefaultPosition,
		wxSize(buttonWidth,buttonHeight), modenames, wxCB_READONLY );
	bfgs->Add( mode, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 3
    wxCheckBox* antialias = new wxCheckBox( mControlPanel, ID_ANTIALIAS, "Antialias", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( antialias );
    bfgs->Add(   antialias, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
	wxStaticText *st2 = new wxStaticText( mControlPanel, -1, "Object:" );
    ::setColor( st2 );
    bfgs->Add(   st2, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 2
    mObject = new manipObjectSwitch(mControlPanel, this);
    bfgs->Add(   mObject, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 3
    mStatus = new wxCheckBox( mControlPanel, ID_STATUS, "Status", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	mStatus->SetValue( true );
    ::setColor( mStatus );
    bfgs->Add(   mStatus, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 1
    wxButton*   color = new wxButton( mControlPanel, ID_COLOR, "Color", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( color );
    bfgs->Add(   color, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 2
    wxButton*   opacity = new wxButton( mControlPanel, ID_OPACITY, "Opacity", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( opacity );
    bfgs->Add(   opacity, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 3, col 3
    mMobile = new wxCheckBox( mControlPanel, ID_MOBILE, "Mobile", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
	mMobile->SetValue( true );
    ::setColor( mMobile );
    bfgs->Add(   mMobile, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 1
    wxButton*   diffuse = new wxButton( mControlPanel, ID_DIFFUSE, "Diffuse", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( diffuse );
    bfgs->Add(   diffuse, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 2
    wxButton*   specular = new wxButton( mControlPanel, ID_SPECULAR, "Specular", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( specular );
    bfgs->Add(   specular, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 4, col 3
    wxButton*   remove = new wxButton( mControlPanel, ID_REMOVE, "Remove", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( remove );
    bfgs->Add(   remove, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 1
    wxButton*   ambient = new wxButton( mControlPanel, ID_AMBIENT, "Ambient", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( ambient );
    bfgs->Add(   ambient, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 2
    wxButton*   magnify = new wxButton( mControlPanel, ID_MAGNIFY, "Magnify", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( magnify );
    bfgs->Add(   magnify, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 5, col 3
    wxButton*   speed = new wxButton( mControlPanel, ID_SPEED, "Speed", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( speed );
    bfgs->Add(   speed, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 1
    wxButton *save = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( save );
    bfgs->Add(   save, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 2
    mRotate = new wxCheckBox( mControlPanel, ID_ROTATE, "Rotate", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( mRotate );
	mRotate->SetValue(canvas->mWhichMode==SurfViewCanvas::VIEW);
    bfgs->Add(   mRotate, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 6, col 3
    wxButton*   reset = new wxButton( mControlPanel, ID_RESET, "Reset", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( reset );
    bfgs->Add(   reset, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 1
    wxButton*   background = new wxButton( mControlPanel, ID_BACKGROUND, "Background", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( background );
    bfgs->Add(   background, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 2
    wxCheckBox* box = new wxCheckBox( mControlPanel, ID_BOX, "Box", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( box );
    bfgs->Add(   box, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	//row 7, col 3
	switch (::st_cl(&canvas->mRenderer->object_list->main_data))
	{
	  case GRADIENT:
	  case PERCENT:
	  case DIRECT:
		{
		  wxButton *vrp = new wxButton( mControlPanel, ID_VOL_REND_PARAMS,
		   "Rend.Params", wxDefaultPosition, wxSize(buttonWidth,buttonHeight));
		  ::setColor( vrp );
		  bfgs->Add(vrp, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL,0);
        }
		break;
	  default:
	    break;
	}

    buttonSizer->Add( bfgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( buttonSizer, 0, wxGROW|wxALL, 10 );
	mBottomSizer->Layout();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for SurfViewFrame class. */
SurfViewFrame::~SurfViewFrame ( void ) {
    cout << "SurfViewFrame::~SurfViewFrame" << endl;
    wxLogMessage( "SurfViewFrame::~SurfViewFrame" );

	delete mCutDepthControls;
	mCutDepthControls = NULL;
	delete mTumbleControls;
	mTumbleControls = NULL;
	delete mGrayMapControls;
	mGrayMapControls = NULL;
    if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }
    //the destruction of the above splitter also causes the destruction of the
    // canvas.  so another destruction of the canvas causes BOOM!
    //if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    //it appears that this is already deleted as well.
    //if (mControlPanel!=NULL)  { delete mControlPanel;  mControlPanel=NULL; }
    //if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
    mCanvas       = NULL;
    mControlPanel = NULL;
    mBottomSizer  = NULL;
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
            if (mCanvas!=NULL)  {  delete mCanvas;  mCanvas=NULL;  }
            ::gFrameList.erase( i );
            break;
        }
    }
    for (i=::demonsInputList.begin(); i!=::demonsInputList.end(); i++) {
        if (*i==this) {
            ::demonsInputList.erase( i );
            break;
        }
    }

    if (::gFrameList.begin() == ::gFrameList.end()) {
        exit(0);
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void SurfViewFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "SurfViewFrame::OnChar" );

    switch (e.GetKeyCode()) {
    case 'a' :
    case 'A' :
        //++mZoomLevel;
        //if (mZoomLevel>5)    mZoomLevel = 5;
        //::setZoomInColor( m_scale );
        break;
    case 'z' :
    case 'Z' :
        //--mZoomLevel;
        //if (mZoomLevel<0)    mZoomLevel = 0;
        //if (mZoomLevel<1)    ::setColor( m_scale );
        break;
    case '>' :
    case '.' :
        break;
    case '<' :
    case ',' :
        break;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnSaveScreen ( wxCommandEvent& unused ) {
    if (mGrayMapControls!=NULL) {
        delete mGrayMapControls;
        mGrayMapControls = NULL;
    }
	MainFrame::OnSaveScreen( unused );
    Raise();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for toggling control panel on and off. */
void SurfViewFrame::OnHideControls ( wxCommandEvent& e ) {
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel, -mControlsHeight );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Open menu item */
void SurfViewFrame::OnOpen ( wxCommandEvent& e ) {
    createSurfViewFrame( this, false, mSurface, manip_flag );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new SurfViewFrame.  It first searches the input file history for an
 *  acceptable input file.  If one is found, it is used.  If none is found
 *  then the user is prompted for an input file.
 *  \param parentFrame is an optional (it may also be NULL) parent frame
 *      which will be closed if we are in single frame mode.
 *      (Furthermore, if the parent is maximized (or not maximized),
 *      the child correspondingly will also be maximized (or not).)
 *  \param useHistory is an optional (default is true) flag to indicate 
 *      whether or not the input history should be used to obtain the most
 *      recent file to open.  If false, the history won't be used and a
 *      regular file open dialog box will be presented to the user.  This
 *      can be used by the traditional Open menu item.
 */
void SurfViewFrame::createSurfViewFrame ( wxFrame* parentFrame, bool useHistory, bool surface, bool manipulate )
{
	wxArrayString  filenames;
    if (useHistory) {
        //search the input history for an acceptable input file
        const int  n = ::gInputFileHistory.GetNoHistoryFiles();
		if (surface || manipulate)
		{
	        for (int i=0; i<n; i++) {
	            wxString  s = ::gInputFileHistory.GetHistoryFile( i );
	            if (match(s, true, false) && ::gInputFileHistory.IsSelected( i )) {
	                //before we insert it into the array, make sure that it exists
	                if (wxFile::Exists(s)) {
	                    filenames.Add( s );
	                } else {
	                    wxString  tmp = wxString::Format(
	                        "File %s could not be opened.", (const char *)s.c_str() );
	                    wxMessageBox( tmp, "File does not exist",
	                        wxOK | wxCENTER | wxICON_EXCLAMATION );
	                }
	            }
	        }
		}
		if ((!surface||manipulate) && filenames.Count()==0) {
			for (int i=0; i<n; i++) {
				wxString  s = ::gInputFileHistory.GetHistoryFile( i );
				if (match(s, false, manipulate) && ::gInputFileHistory.IsSelected( i )) {
	                //before we insert it into the array, make sure that it exists
					if (wxFile::Exists(s)) {
						filenames.Add( s );
					} else {
	                    wxString  tmp = wxString::Format(
	                        "File %s could not be opened.", (const char *)s.c_str() );
	                    wxMessageBox( tmp, "File does not exist",
	                        wxOK | wxCENTER | wxICON_EXCLAMATION );
	                }
				}
			}
		}
    }

    //did we find an acceptable input file in the input history?
    if (filenames.Count()==0) {
        //nothing acceptable so display a dialog which allows the user
        // to choose an input file.
		const wxString pattern =
			manipulate?
				"CAVASS structure files (*.BS0;*.BS2;*.SH0)|*.BS0;*.BS2;*.SH0":
			surface? "CAVASS surface files (*.BS0;*.BS2)|*.BS0;*.BS2":
			"CAVASS volume files (*.SH0;*.IM0)|*.SH0;*.IM0";
		wxFileDialog fd( parentFrame, _T("Select surface/volume file(s)"),
			_T(""), _T(""), pattern, wxFD_OPEN | wxFD_FILE_MUST_EXIST |
			wxFD_MULTIPLE );
        if (fd.ShowModal() != wxID_OK)    return;
        fd.GetFilenames( filenames );
        if (filenames.Count()==0)    return;
        //add the input files to the input history
        wxString  d = fd.GetDirectory();
        d += wxFileName::GetPathSeparator();
        for (int i=filenames.Count()-1; i>=0; i--) {
            filenames[i] = d + filenames[i];  //prepend the directory
            ::gInputFileHistory.AddFileToHistory( filenames[i] );
        }
        wxConfigBase*  pConfig = wxConfigBase::Get();
        ::gInputFileHistory.Save( *pConfig );
    }

    //at this point, filenames must have at least 1 file
    if (filenames.Count()==0)    return;

	wxArrayString firsttolast;
	for (int i=filenames.Count()-1; i>=0; i--)
		firsttolast.Add(filenames[i]);
    
    //display an example frame using the specified file as input
    SurfViewFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new SurfViewFrame( parentFrame->IsMaximized(), w, h, surface, manipulate );
    } else {
        frame = new SurfViewFrame();
    }
    frame->loadFiles( firsttolast );
    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This function determines if the given filename is of a type
 *  that can be read by this module/app.
 *  Supported surface file extension(s): *.BS0, *.BS2, *.SH0
 *  Supported volume file extension(s):  *.IM0
 *  \param filename is the file name which may match
 *  \returns true if the filename matches; false otherwise
 */
bool SurfViewFrame::match ( wxString filename, bool surface, bool manipulate ) {
    wxString  fn = filename.Upper();
    if (surface) {
        if (wxMatchWild( "*.BS0",   fn, false ))    return true;
        if (wxMatchWild( "*.BS2",   fn, false ))    return true;
    } else {
        if (!manipulate && wxMatchWild( "*.IM0",   fn, false ))    return true;
        if (wxMatchWild( "*.SH0",   fn, false ))    return true;
    }
	if (wxMatchWild( "*.PLN",   fn, false ))    return true;
    return false;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data from a file.
 */
void SurfViewFrame::loadFiles ( wxArrayString &filenames ) {
    if (filenames.Count() == 0)    return;
    if (!wxFile::Exists(filenames[0])) {
        wxString  tmp =
			wxString( "File " )+filenames[0]+" could not be opened.";
        wxMessageBox( tmp, "File does not exist",
            wxOK | wxCENTER | wxICON_EXCLAMATION, this );
        return;
    }
    
    wxString  tmp;
	if (manip_flag)    tmp = "CAVASS:Manipulate: ";
    else if (mSurface) tmp = "CAVASS:SurfView: ";
    else               tmp = "CAVASS:VolView: ";
    mFileOrDataCount += filenames.Count();
    tmp += filenames[0];
    if (mFileOrDataCount >= 2) {
        tmp += ", " + filenames[1];
    }
    //does a window with this title (file(s)) already exist?
    if (searchWindowTitles(tmp)) {
        //yes, so open a duplicate with a unique name
        for (int i=2; i<100; i++) {
			if (manip_flag)
				tmp = "CAVASS:Manipulate:";
            else if (mSurface)
                tmp = "CAVASS:SurfView:";
            else
                tmp = "CAVASS:VolView:";
			tmp += filenames[0] + wxString::Format(" (%d)", i);
            if (!searchWindowTitles(tmp))    break;
        }
    }

    //changeAllWindowMenus( mWindowTitle, tmp );
    ::removeFromAllWindowMenus( mWindowTitle );
    ::addToAllWindowMenus( tmp );
    mWindowTitle = tmp;
    SetTitle( mWindowTitle );
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (manip_flag)
		canvas->mWhichMode = SurfViewCanvas::CUT_CURVED;
    canvas->loadFiles( filenames );
	if (!canvas->isLoaded(0))
	{
		delete m_buttonBox;
		m_buttonBox = NULL;
		return;
	}
	mFilenames = filenames;
	if (manip_flag)
	{
		wxString ext = wxFileName(filenames[0]).GetExt();
		mSurface = ext=="BS0"||ext=="BS2";
    }
    addButtonBox();
	if (canvas->mWhichMode==SEPARATE ||
			canvas->mWhichMode==SurfViewCanvas::CUT_CURVED)
		mCutDepthControls = new CutDepthControls( mControlPanel, mBottomSizer, "Cut Depth", ID_CUT_DEPTH_SLIDER, Z_BUFFER_LEVELS/canvas->mRenderer->depth_scale, canvas->cut_depth );
	const wxString  s = wxString( "file ")+ filenames[0];
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief load data directly (instead of from a file).
 *  \todo needs work to support one or both of the two data sources from data
 */
void SurfViewFrame::loadData ( char* name,
    const int xSize, const int ySize, const int zSize,
    const double xSpacing, const double ySpacing, const double zSpacing,
    const int* const data, const ViewnixHeader* const vh,
    const bool vh_initialized )
{
    assert( 0 );
    
    if (name==NULL || strlen(name)==0)  name=(char *)"no name";
    //wxString  tmp("CAVASS:SurfView: ");
    wxString  tmp;
    if (mSurface)    tmp = "CAVASS:SurfView: ";
    else             tmp = "CAVASS:VolView: ";
    tmp += name;
    SetTitle( tmp );
    
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->loadData( name, xSize, ySize, zSize,
        xSpacing, ySpacing, zSpacing, data, vh, vh_initialized );
    //        initSubs();
    
    const wxString  s = wxString::Format( "data %s", name );
    SetStatusText( s, 0 );
    
    Show();
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview */
void SurfViewFrame::OnPrintPreview ( wxCommandEvent& e ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new SurfViewPrint(wxString("CAVASS").c_str(), canvas),
        new SurfViewPrint(wxString("CAVASS").c_str(), canvas),
        &printDialogData );
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    wxPreviewFrame*  frame = new wxPreviewFrame( preview, this,
        _T("CAVASS Print Preview"),
        wxPoint(100, 100), wxSize(600, 650) );
    frame->Centre( wxBOTH );
    frame->Initialize();
    frame->Show();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing */
void SurfViewFrame::OnPrint ( wxCommandEvent& e ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    SurfViewCanvas*    canvas = dynamic_cast<SurfViewCanvas*>( mCanvas );
    if (mSurface) {
        SurfViewPrint  printout( (const wxChar*)"CAVASS:SurfView", canvas );
        if (!printer.Print(this, &printout, TRUE)) {
            if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
                wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
        } else {
            *g_printData = printer.GetPrintDialogData().GetPrintData();
        }
    } else {
        SurfViewPrint  printout( (const wxChar*)"CAVASS:VolView", canvas );
        if (!printer.Print(this, &printout, TRUE)) {
            if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
                wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
        } else {
            *g_printData = printer.GetPrintDialogData().GetPrintData();
        }
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief display the input dialog that
 *  (i)  allows the user to gather info about existing files, and
 *  (ii) allows the user to choose input files for this module.
 *  \param unused is not used.
 */
void SurfViewFrame::OnInput ( wxCommandEvent& unused ) {
    InputHistoryDialog  ihd( this );
    int  ret = ihd.ShowModal();
    if (ret==wxID_OK && ::gInputFileHistory.GetNoHistoryFiles()>0)
    	createSurfViewFrame( this, true, mSurface, manip_flag );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief remove all controls. */
void SurfViewFrame::removeAll ( void ) {
    if (mAmbientControls!=NULL)     {  delete mAmbientControls;     mAmbientControls=NULL;     }
    if (mBackgroundControls!=NULL)  {  delete mBackgroundControls;  mBackgroundControls=NULL;  }
    if (mColorControls!=NULL)       {  delete mColorControls;       mColorControls=NULL;       }
    if (mDiffuseControls!=NULL)     {  delete mDiffuseControls;     mDiffuseControls=NULL;     }
    if (mMagnifyControls!=NULL)     {  delete mMagnifyControls;     mMagnifyControls=NULL;     }
    if (mOpacityControls!=NULL)     {  delete mOpacityControls;     mOpacityControls=NULL;     }
    if (mSpecularControls!=NULL)    {  delete mSpecularControls;    mSpecularControls=NULL;    }
    if (mSpeedControls!=NULL)       {  delete mSpeedControls;       mSpeedControls=NULL;       }
	if (mVRControls!=NULL)          {  delete mVRControls;          mVRControls=NULL;          }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SurfViewFrame::getIntermediateViews ( void )
{
	return intermediate_views;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the ambient controls */
void SurfViewFrame::OnAmbient ( wxCommandEvent& unused ) {
    if (mAmbientControls!=NULL)     return;
    removeAll();

    unsigned char  newRed, newGreen, newBlue;
    ((SurfViewCanvas *)(mCanvas))->mRenderer->getAmbientLight( newRed, newGreen, newBlue );

    mAmbientControls = new ColorControls( mControlPanel, mBottomSizer, "Ambient",
        newRed/255.0, newGreen/255.0, newBlue/255.0,
        ID_AMB_RED_SLIDER, ID_AMB_GREEN_SLIDER, ID_AMB_BLUE_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which sets the desired antialias mode and re-renders */
void SurfViewFrame::OnAntialias ( wxCommandEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->setAntialias( e.IsChecked() );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMIP ( wxCommandEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	mMIP = e.IsChecked();
	if (mMIP)
		mVRControls->displayInvert(mMIP_Invert, true);
	else
	{
		mVRControls->removeSliders();
		mMIP_Invert = false;
		canvas->setMIP_Invert(false);
	}
	canvas->setMIP(mMIP);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMIP_Invert ( wxCommandEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	mMIP_Invert = e.IsChecked();
	canvas->setMIP_Invert(mMIP_Invert);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnStatus ( wxCommandEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	cvRenderer *rr = canvas->mRenderer;
	Shell *obj = rr->object_list;
	if (all_objects_selected)
	{
		for (int j=1; obj; j++, obj=obj->next)
		{
			canvas->setObject( j );
			canvas->setStatus( e.IsChecked() );
		}
		canvas->setObject( mWhichObject );
	}
	else
		canvas->setStatus( e.IsChecked() );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMobile ( wxCommandEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	cvRenderer *rr = canvas->mRenderer;
	Shell *obj = rr->object_list;
	if (all_objects_selected)
	{
		for (int j=1; obj; j++, obj=obj->next)
		{
			canvas->setObject( j );
			canvas->setMobile( e.IsChecked() );
		}
		canvas->setObject( mWhichObject );
	}
	else
		canvas->setMobile( e.IsChecked() );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnRemove ( wxCommandEvent& unused ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (all_objects_selected)
	{
		wxMessageBox("Select one object!");
		return;
	}
	canvas->removeObject();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnRotate ( wxCommandEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setRotate(e.IsChecked());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which sets the desired box mode and re-renders */
void SurfViewFrame::OnBox ( wxCommandEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setBox(e.IsChecked());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the background controls */
void SurfViewFrame::OnBackground ( wxCommandEvent& unused ) {
    if (mBackgroundControls!=NULL) return;
    removeAll();

    unsigned char  newRed, newGreen, newBlue;
    ((SurfViewCanvas *)(mCanvas))->mRenderer->getBackgroundColor( newRed, newGreen, newBlue );

    mBackgroundControls = new ColorControls( mControlPanel, mBottomSizer, "Background",
        newRed/255.0, newGreen/255.0, newBlue/255.0,
        ID_BG_RED_SLIDER, ID_BG_GREEN_SLIDER, ID_BG_BLUE_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the color controls */
void SurfViewFrame::OnColor ( wxCommandEvent& unused ) {
    if (mColorControls!=NULL)       return;
	if (all_objects_selected)
	{
		wxMessageBox("Select one object!");
		return;
	}
    removeAll();

    unsigned char  newRed, newGreen, newBlue;

    ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectColor( newRed, newGreen, newBlue, mWhichObject );

    mColorControls = new ColorControls( mControlPanel, mBottomSizer, "Color",
        newRed/255.0, newGreen/255.0, newBlue/255.0,
        ID_COLOR_RED_SLIDER, ID_COLOR_GREEN_SLIDER, ID_COLOR_BLUE_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the diffuse controls. */
void SurfViewFrame::OnDiffuse ( wxCommandEvent& unused ) {
    if (mDiffuseControls!=NULL)     return;
    removeAll();

    mDiffuseControls = new SurfPropertiesControls ( mControlPanel,
        mBottomSizer, "Diffuse", getObjectDiffuseFraction( ((SurfViewCanvas *)(mCanvas))->mRenderer, mWhichObject ),
        ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectDiffuseExponent( mWhichObject ), ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectDiffuseDivisor( mWhichObject ),
        0.0, 0.0, 1.0, 1.0, 2.0, 10.0,
        ID_DIFFUSE_PERCENT_SLIDER, ID_DIFFUSE_EXPONENT_SLIDER, ID_DIFFUSE_DIVISOR_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to change in object number (via
 *  button press).
 */
void SurfViewFrame::OnLayout ( wxCommandEvent& unused ) {
    if (mObject==NULL)    return;
    mLayoutOn = !mLayoutOn;
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->setLayout( mLayoutOn );
    if (mLayoutOn)    mLayout->SetLabel( "Layout On"  );
    else              mLayout->SetLabel( "Layout Off" );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to changes in light selection.
 *  \todo  call function to change light.
 */
void SurfViewFrame::OnLight ( wxCommandEvent& unused ) {
    if (mLight==NULL)    return;
    ++mWhichLight;
    if (mWhichLight>MAX_LIGHT)    mWhichLight = 1;
    const wxString  s = wxString::Format( "Light %d", mWhichLight );
    mLight->SetLabel( s );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the magnify controls. */
void SurfViewFrame::OnMagnify ( wxCommandEvent& unused ) {
    if (mMagnifyControls!=NULL)     return;
    removeAll();

    mMagnifyControls = new MagnifyControls( mControlPanel, mBottomSizer, "Magnify",
        ID_MAGNIFY_SLIDER, ((SurfViewCanvas *)(mCanvas))->mRenderer->getScale(), -1, -1 );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to change in object number (via
 *  button press).
 */
void SurfViewFrame::OnObject ( wxCommandEvent& e ) {
	delete mGrayMapControls;
	mGrayMapControls = NULL;
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (e.GetString() == "All")
	{
		all_objects_selected = true;
		if (mSurface && (mColorControls || mOpacityControls))
			removeAll();
		return;
	}
	all_objects_selected = false;
    canvas->setObject( (mWhichObject = atoi((const char *)e.GetString().c_str())) );
	mStatus->SetValue(canvas->mRenderer->object_from_label(mWhichObject)->on !=
		0);
	mMobile->SetValue(canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject))->mobile != 0);
	if (mDiffuseControls || mSpecularControls || (mSurface && (mColorControls
			|| mOpacityControls)))
		removeAll();
	if (canvas->mWhichMode==SurfViewCanvas::SELECT_SLICE ||
			canvas->mWhichMode==SurfViewCanvas::MEASURE ||
			canvas->mWhichMode==SurfViewCanvas::ROI_STATISTICS)
	{
		displayGrayMapControls();
		mBottomSizer->Layout();
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMode ( wxCommandEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	int mode;
	for (mode=SEPARATE; mode<=SurfViewCanvas::MOVE; mode++)
		if (modeName(mode) == e.GetString())
			break;
	if (mode!=SEPARATE && mode!=SurfViewCanvas::CUT_CURVED &&
			mCutDepthControls)
	{
		delete mCutDepthControls;
		mCutDepthControls = NULL;
	}
	if (mode != SurfViewCanvas::SELECT_SLICE)
	{
		delete mSliceOutputControls;
		mSliceOutputControls = NULL;
	}
	delete mTumbleControls;
	mTumbleControls = NULL;
	delete mGrayMapControls;
	mGrayMapControls = NULL;
	if (canvas->mWhichMode==SurfViewCanvas::VIEW || mode==SurfViewCanvas::VIEW)
	{
		canvas->setRotate(mode==SurfViewCanvas::VIEW);
		mRotate->SetValue(mode==SurfViewCanvas::VIEW);
	}
	canvas->setMode(mode);
	if (mode == SurfViewCanvas::SELECT_SLICE)
        displaySliceOutputControls();
	if ((mode==SEPARATE || mode==SurfViewCanvas::CUT_CURVED) &&
			mCutDepthControls==NULL)
		mCutDepthControls = new CutDepthControls( mControlPanel, mBottomSizer, "Cut Depth", ID_CUT_DEPTH_SLIDER, Z_BUFFER_LEVELS/canvas->mRenderer->depth_scale, canvas->cut_depth );
	if (canvas->mWhichMode == SurfViewCanvas::CREATE_MOVIE_TUMBLE)
		mTumbleControls = new TumbleControls( mControlPanel, mBottomSizer,
		canvas->key_poses? canvas->key_pose[canvas->key_poses-1].views:
		intermediate_views, false, canvas->preview_all_poses );
	else if (canvas->mWhichMode == SurfViewCanvas::PREVIOUS_SEQUENCE)
		mTumbleControls = new TumbleControls( mControlPanel, mBottomSizer,
			canvas->preview_all_poses );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the opacity controls. */
void SurfViewFrame::OnOpacity ( wxCommandEvent& unused ) {
	if (all_objects_selected)
	{
		wxMessageBox("Select one object!");
		return;
	}
    if (mOpacityControls!=NULL)     return;
    removeAll();

    mOpacityControls = new SurfOpacityControls( mControlPanel, mBottomSizer,
        "Surface opacity percentage", ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectOpacity( mWhichObject ),
        ID_OPACITY_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which displays the volume rendering parameter controls. */
void SurfViewFrame::OnVRParams ( wxCommandEvent& unused ) {
    if (mVRControls!=NULL)     return;
    removeAll();
	SurfViewCanvas *canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell_data *sd = &canvas->mRenderer->object_list->main_data;
	if (st_cl(sd) == DIRECT)
		mVRControls = new VolumeRenderingControls( mControlPanel, mBottomSizer,
			mWhichMaterial, true, 0.0,
			sd->file->file_header.scn.largest_density_value[0], mMIP,
			mMIP_Invert );
	else
		mVRControls = new VolumeRenderingControls( mControlPanel, mBottomSizer,
			mWhichMaterial, false, mMIP, mMIP_Invert );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief call back which displays the specular controls. */
void SurfViewFrame::OnSpecular ( wxCommandEvent& unused ) {
    if (mSpecularControls!=NULL)    return;
    removeAll();

    mSpecularControls = new SurfPropertiesControls ( mControlPanel,
        mBottomSizer, "Specular", ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectSpecularFraction( mWhichObject ),
        ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectSpecularExponent( mWhichObject ), ((SurfViewCanvas *)(mCanvas))->mRenderer->getObjectSpecularDivisor( mWhichObject ),
        0.0, 0.0, 1.0, 1.0, 10.0, 3.0,
        ID_SPECULAR_PERCENT_SLIDER, ID_SPECULAR_EXPONENT_SLIDER, ID_SPECULAR_DIVISOR_SLIDER );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief call back which displays the speed controls. */
void SurfViewFrame::OnSpeed ( wxCommandEvent& unused ) {
    if (mSpeedControls!=NULL)    return;
    removeAll();

    mSpeedControls = new SurfSpeedControls ( mControlPanel, mBottomSizer,
        "Mouse Speed", speedSliderDefault, ID_SPEED_SLIDER );
    mSpeed = (speedSliderMax - speedSliderDefault + 1) * 2.0;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnReset(wxCommandEvent &unused)
{
	SurfViewCanvas *canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->reset();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnSave(wxCommandEvent &unused)
{
	SurfViewCanvas *canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Classification_type stcl=st_cl(&canvas->mRenderer->object_list->main_data);
	wxString defaultFile("mnp-tmp.");
	wxString pattern;
	int nobjs = 0, err;
	switch (canvas->mWhichMode)
	{
		case SurfViewCanvas::CREATE_MOVIE_TUMBLE:
		case SurfViewCanvas::PREVIOUS_SEQUENCE:
			defaultFile += "tif";
			pattern =
			  "TIFF files (*.tif;*.tiff)|*.tif;*.tiff|MV0 files (*.MV0)|*.MV0";
			break;
		case SurfViewCanvas::MEASURE:
		case SurfViewCanvas::ROI_STATISTICS:
			defaultFile += "TXT";
			pattern = "TXT files (*.TXT)|*.TXT";
			break;
		case SurfViewCanvas::SELECT_SLICE:
			if (canvas->mRenderer->scene[0].vh.scn.dimension != 3)
			{
				wxMessageBox("No valid scene loaded.");
				return;
			}
			defaultFile += "IM0";
			pattern = "IM0 files (*.IM0)|*.IM0";
			break;
		default:
			switch (stcl)
			{
				case BINARY_A:
				case BINARY_B:
				case GRADIENT:
				case PERCENT:
				case T_SHELL:
					defaultFile += "PLN";
					pattern = "PLN files (*.PLN)|*.PLN";
					Shell *obj, *first_obj;
					first_obj = NULL;
					for(obj=canvas->mRenderer->object_list; obj; obj=obj->next)
						if (obj->O.on)
						{
							if (first_obj == NULL)
								first_obj = obj;
							else if (incompatible(&obj->main_data, &first_obj->main_data) || (canvas->mRenderer->icons_exist && incompatible(&obj->icon_data, &first_obj->icon_data)))
							{
								wxMessageBox("Objects are not compatible.");
								return;
							}
							nobjs++;
						}
					break;
				case DIRECT:
					wxMessageBox("Not implemented.");
					return;
			}
	}
    wxFileDialog saveDlg(this,
                _T("Save file"),
                wxEmptyString,
                defaultFile,
                pattern,
                wxSAVE|wxOVERWRITE_PROMPT);
    if (saveDlg.ShowModal() != wxID_OK)
		return;
    switch (canvas->mWhichMode)
	{
		case SurfViewCanvas::SELECT_SLICE:
			canvas->mRenderer->do_slice_output(saveDlg.GetPath().c_str(),
				canvas->first_loc, canvas->last_loc, canvas->out_pixel_size,
				matched_output);
			wxSafeYield(NULL, true);
			break;
		case SurfViewCanvas::MEASURE:
			FILE *fp;
			fp = fopen((const char *)saveDlg.GetPath().c_str(), "w");
			for (int j=0; j<canvas->mRenderer->nmeasurement_points; j++)
				fprintf(fp,	"(%f, %f, %f)\n",
						canvas->mRenderer->measurement_point[j][0]-
						canvas->mRenderer->glob_displacement[0],
						canvas->mRenderer->measurement_point[j][1]-
						canvas->mRenderer->glob_displacement[1],
						canvas->mRenderer->measurement_point[j][2]-
						canvas->mRenderer->glob_displacement[2]);
			fclose(fp);
			break;
		case SurfViewCanvas::ROI_STATISTICS:
			if (!canvas->stats_valid)
			{
				wxMessageBox("No valid statistics computed.");
				break;
			}
			fp = fopen((const char *)saveDlg.GetPath().c_str(), "w");
			fprintf(fp,
			"Total density %.0f; Mean %.1f; St.dev. %.1f; Min %.0f; Max %.0f\n"
				,
				canvas->total_density,
				canvas->mean_density,
				canvas->standard_deviation,
				canvas->min_density,
				canvas->max_density);
			fclose(fp);
			break;
		case SurfViewCanvas::CREATE_MOVIE_TUMBLE:
		case SurfViewCanvas::PREVIOUS_SEQUENCE:
			canvas->save_movie(saveDlg.GetPath().c_str());
			break;
		default:
			err=canvas->mRenderer->write_plan(saveDlg.GetPath().c_str(),nobjs);
			if (err)
				wxMessageBox(wxString::Format("Error %d", err));
			break;
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnPreview ( wxCommandEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->mControlState = e.IsChecked()? 12: 0;
	canvas->setActionText();
	canvas->preview_view = canvas->preview_key_pose = -1;
	updateIntermediateViews();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnPoses ( wxCommandEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->preview_all_poses = e.GetString()=="All";
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMaterial ( wxCommandEvent& e ) {
	mWhichMaterial = e.GetString()=="Surf"? 0: atoi((const char *)e.GetString().c_str())-1;
	mVRControls->removeMaterialSliders();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMaterialOpacity(wxCommandEvent &unused)
{
	if (mWhichMaterial == 0)
		return;
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	int *th = canvas->mRenderer->object_list->main_data.threshold;
	mVRControls->displayOpacitySliders(
		canvas->mRenderer->tissue_opacity[mWhichMaterial],
		th[2*mWhichMaterial-2], th[2*mWhichMaterial-1]);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnMaterialColor (wxCommandEvent &unused)
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	mVRControls->displayColorSliders(
		canvas->mRenderer->tissue_red[mWhichMaterial]*
		(1./(V_OBJECT_IMAGE_BACKGROUND-1)),
		canvas->mRenderer->tissue_green[mWhichMaterial]*
		(1./(V_OBJECT_IMAGE_BACKGROUND-1)),
		canvas->mRenderer->tissue_blue[mWhichMaterial]*
		(1./(V_OBJECT_IMAGE_BACKGROUND-1)));
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnSurfaceStrength (wxCommandEvent &unused)
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	mVRControls->displaySurfStrenSliders(canvas->mRenderer->surface_strength,
		canvas->mRenderer->emission_power,
		canvas->mRenderer->surf_pct_power);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for opacity slider. */
void SurfViewFrame::OnOpacitySlider ( wxScrollEvent& e ) {

    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setObjectOpacity( mOpacityControls->getOpacity(), mWhichObject );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for magnify slider. */
void SurfViewFrame::OnMagnifySlider ( wxScrollEvent& e ) {
    const int       newScaleValue = e.GetPosition();
    const double    newScale = newScaleValue/100.0;
    //avoid duplicate events that don't change the current scale/magnify value
    if (newScale == mScale) {
        //cout << "ignoring duplicate" << endl;
        return;
    }
    mScale = newScale;
    const wxString  s = wxString::Format( "%5.2f", newScale );
    mMagnifyControls->setScaleText( s );

    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->OnMagnify( newScale );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for cut depth slider. */
void SurfViewFrame::OnCutDepthSlider ( wxScrollEvent& e ) {
    const int       newDepthValue = e.GetPosition();
    const double    newDepth = newDepthValue/100.0;
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    //avoid duplicate events that don't change the current cut depth value
    if (newDepth == canvas->cut_depth) {
        return;
    }
    canvas->cut_depth = newDepth;
    const wxString  s = wxString::Format( "%5.2f", newDepth );
    mCutDepthControls->setDepthText( s );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to pixel size slider change. */
void SurfViewFrame::OnPixelSizeSlider ( wxScrollEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->out_pixel_size = e.GetPosition()/100.0;
	mSliceOutputControls->UpdatePixelSize(canvas->out_pixel_size);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to slice spacing slider change. */
void SurfViewFrame::OnSliceSpacingSlider ( wxScrollEvent& e ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->mRenderer->out_slice_spacing = (float)(e.GetPosition()/100.0);
	mSliceOutputControls->UpdateSliceSpacing(
		canvas->mRenderer->out_slice_spacing);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any ambient slider change. */
void SurfViewFrame::OnAnyAmbientSlider ( wxScrollEvent& e ) {
    mAmbientControls->update();
    double  r, g, b;
    mAmbientControls->getRGB( r, g, b );
    unsigned char  red   = (unsigned char)(r * 255);
    unsigned char  green = (unsigned char)(g * 255);
    unsigned char  blue  = (unsigned char)(b * 255);
    //((SurfViewCanvas *)(mCanvas))->mRenderer->setAmbientLight( red, green, blue );

    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->setAmbientLight( red, green, blue );
    wxString  tmp = wxString::Format( "SurfViewFrame::OnAnyAmbientSlider:%d", red );
    wxLogMessage( tmp );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any background slider change. */
void SurfViewFrame::OnAnyBackgroundSlider ( wxScrollEvent& e ) {
    mBackgroundControls->update();
    double  r, g, b;
    mBackgroundControls->getRGB( r, g, b );
    unsigned char  red   = (unsigned char)(r * 255);
    unsigned char  green = (unsigned char)(g * 255);
    unsigned char  blue  = (unsigned char)(b * 255);
    //((SurfViewCanvas *)(mCanvas))->mRenderer->setBackgroundColor( red, green, blue );

    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->setBackgroundColor( red, green, blue );
    wxString  tmp = wxString::Format( "SurfViewFrame::OnAnyBackgroundSlider:%d", red );
    wxLogMessage( tmp );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any color slider change. */
void SurfViewFrame::OnAnyColorSlider ( wxScrollEvent& e ) {
    mColorControls->update();
    double  r, g, b;
    mColorControls->getRGB( r, g, b );
    unsigned char  red   = (unsigned char)(r * 255);
    unsigned char  green = (unsigned char)(g * 255);
    unsigned char  blue  = (unsigned char)(b * 255);

    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->setObjectColor( red, green, blue, mWhichObject );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any diffuse slider change. */
void SurfViewFrame::OnAnyDiffuseSlider ( wxScrollEvent& e ) {
    mDiffuseControls->update();
	double percent=mDiffuseControls->getPercent(),
	       exponent=mDiffuseControls->getExponent(),
		   divisor=mDiffuseControls->getDivisor();
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (all_objects_selected)
		canvas->setAllDiffuse( percent, exponent, divisor );
	else
	    canvas->setObjectDiffuse( percent, exponent, divisor, mWhichObject );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any specular slider change. */
void SurfViewFrame::OnAnySpecularSlider ( wxScrollEvent& e ) {
    mSpecularControls->update();
	double percent=mSpecularControls->getPercent(),
	       exponent=mSpecularControls->getExponent(),
		   divisor=mSpecularControls->getDivisor();
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    if (all_objects_selected)
		canvas->setAllSpecular( percent, exponent, divisor );
	else
		canvas->setObjectSpecular( percent, exponent, divisor, mWhichObject );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnNext ( wxCommandEvent& unused ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);

	canvas->nextSlice();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnPrevious ( wxCommandEvent& unused ) {
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);

	canvas->previousSlice();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnCenterSlider( wxScrollEvent& e )
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
			canvas->setGrayMap(0, e.GetPosition(), -1, -1);
		else if (canvas->mRenderer->slice_list->object[1] == obj)
			canvas->setGrayMap(1, e.GetPosition(), -1, -1);
	}
}
void SurfViewFrame::OnWidthSlider( wxScrollEvent& e )
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
			canvas->setGrayMap(0, -1, e.GetPosition(), -1);
		else if (canvas->mRenderer->slice_list->object[1] == obj)
			canvas->setGrayMap(1, -1, e.GetPosition(), -1);
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnCTLung ( wxCommandEvent& unused )
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
			canvas->setGrayMap(0, Preferences::getCTLungCenter(),
				Preferences::getCTLungWidth(), 0);
		else if (canvas->mRenderer->slice_list->object[1] == obj)
			canvas->setGrayMap(1, Preferences::getCTLungCenter(),
			    Preferences::getCTLungWidth(), 0);
	}
	mGrayMapControls->update_sliders(Preferences::getCTLungCenter(),
		Preferences::getCTLungWidth());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnCTSoftTissue ( wxCommandEvent& unused )
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
			canvas->setGrayMap(0,
				Preferences::getCTSoftTissueCenter(),
				Preferences::getCTSoftTissueWidth(), 0);
		else if (canvas->mRenderer->slice_list->object[1] == obj)
			canvas->setGrayMap(1,
				Preferences::getCTSoftTissueCenter(),
				Preferences::getCTSoftTissueWidth(), 0);
	}
	mGrayMapControls->update_sliders(Preferences::getCTSoftTissueCenter(),
		Preferences::getCTSoftTissueWidth());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnCTBone ( wxCommandEvent& unused )
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
			canvas->setGrayMap(0, Preferences::getCTBoneCenter(),
				Preferences::getCTBoneWidth(), 0);
		else if (canvas->mRenderer->slice_list->object[1] == obj)
			canvas->setGrayMap(1, Preferences::getCTBoneCenter(),
				Preferences::getCTBoneWidth(), 0);
	}
	mGrayMapControls->update_sliders(Preferences::getCTBoneCenter(),
		Preferences::getCTBoneWidth());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnPET ( wxCommandEvent& unused )
{
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	Shell *obj=canvas->mRenderer->actual_object(
		canvas->mRenderer->object_from_label(mWhichObject));
	if (canvas->mRenderer->slice_list)
	{
		if (canvas->mRenderer->slice_list->object[0] == obj)
			canvas->setGrayMap(0, Preferences::getPETCenter(),
				Preferences::getPETWidth(), 1);
		else if (canvas->mRenderer->slice_list->object[1] == obj)
			canvas->setGrayMap(1, Preferences::getPETCenter(),
				Preferences::getPETWidth(), 1);
	}
	mGrayMapControls->update_sliders(Preferences::getPETCenter(),
		Preferences::getPETWidth());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any surface strength slider change. */
void SurfViewFrame::OnSurfaceStrengthSlider ( wxScrollEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setSurfStrength(mVRControls->GetSurfStrength(),
		mVRControls->GetEmissionPower(), mVRControls->GetSurfPctPower());
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any material opacity slider change. */
void SurfViewFrame::OnMaterialOpacitySlider ( wxScrollEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setMaterialOpacity(mVRControls->GetOpacityValue(),
		mWhichMaterial);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any 
 slider change. */
void SurfViewFrame::OnAnyMatlThresholdSlider ( wxScrollEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setMaterialThresholds(mVRControls->GetThreshold1(),
		mVRControls->GetThreshold2(), mWhichMaterial);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to any 
 slider change. */
void SurfViewFrame::OnAnyMatlColorSlider ( wxScrollEvent& e ) {
    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	canvas->setMaterialColor(
		(int)(mVRControls->GetRedValue()*(V_OBJECT_IMAGE_BACKGROUND-1)),
		(int)(mVRControls->GetGreenValue()*(V_OBJECT_IMAGE_BACKGROUND-1)),
		(int)(mVRControls->GetBlueValue()*(V_OBJECT_IMAGE_BACKGROUND-1)),
		mWhichMaterial);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback which responds to a mouse speed slider change.
 *  \todo  implement this functionality.
 */
void SurfViewFrame::OnSpeedSlider ( wxScrollEvent& e ) {
    assert( mSpeedControls );
    int  value = mSpeedControls->getSpeed();
    assert( speedSliderMin<=value && value<=speedSliderMax );
    mSpeed = (speedSliderMax - value + 1) * 2.0;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SurfViewFrame::OnIntermediateViews ( wxScrollEvent& e ) {
	intermediate_views = mTumbleControls->GetValue();
	SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
	if (canvas->key_poses)
	{
		if (canvas->mControlState==12 && canvas->preview_key_pose>=0)
			canvas->key_pose[canvas->preview_key_pose].views =
				intermediate_views;
		else
			canvas->key_pose[canvas->key_poses-1].views = intermediate_views;
	}
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef __WXX11__
/** \brief callback for magnify slider.
 *  especially (only) need on X11 (w/out GTK) to get slider events.
 */
void SurfViewFrame::OnUpdateUIMagnifySlider ( wxUpdateUIEvent& unused ) {
    //especially (only) need on X11 (w/out GTK) to get slider events
    const double  newScale = mMagnifyControls->GetValue() / 100.0;
    if (newScale == ((SurfViewCanvas *)(mCanvas))->mRenderer->getScale())    return;
    
    const wxString  s = wxString::Format( "scale: %8.2f", newScale );
    mMagnifyControls->setScaleText( s );

    SurfViewCanvas*  canvas = dynamic_cast<SurfViewCanvas*>(mCanvas);
    canvas->OnMagnify( newScale );
}
#endif



/*****************************************************************************
 * FUNCTION: ok_to_write
 * DESCRIPTION: Checks whether there is already a file of the same name, and
 *    if so, checks with the user whether it is OK to overwrite.
 * PARAMETERS:
 *    file_name: The file name
 * SIDE EFFECTS: Extraneous events may be removed from the queue.
 * ENTRY CONDITIONS: The global variable dialog_window and the static
 *    variables image_button_action, dialog_button_action must be appropriately
 *    initialized. A successful call to VCreateColormap must be made first.
 *    ButtonPress events should be selected.
 * RETURN VALUE: TRUE if the file does not exist or user presses middle mouse
 *    button.
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 2/8/93 by Dewey Odhner
 *    Modified: 3/6/97 previous button action restored by Dewey Odhner
 *
 *****************************************************************************/
bool ok_to_write(const char file_name[])
{
	if (!wxFile::Exists(file_name))
		return (true);
	return wxMessageBox(
		wxString("File ")+file_name+"already exists. Overwrite?",
		"Replace existing file?", wxYES_NO | wxICON_QUESTION ) == wxYES;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( SurfViewFrame, wxFrame )
BEGIN_EVENT_TABLE       ( SurfViewFrame, wxFrame )
  DefineStandardFrameCallbacks
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //app specific callbacks:
  EVT_BUTTON( ID_AMBIENT,      SurfViewFrame::OnAmbient      )
  EVT_BUTTON( ID_BACKGROUND,   SurfViewFrame::OnBackground   )
  EVT_BUTTON( ID_COLOR,        SurfViewFrame::OnColor        )
  EVT_BUTTON( ID_DIFFUSE,      SurfViewFrame::OnDiffuse      )
  EVT_BUTTON( ID_LAYOUT,       SurfViewFrame::OnLayout       )
  EVT_BUTTON( ID_LIGHT,        SurfViewFrame::OnLight        )
  EVT_BUTTON( ID_MAGNIFY,      SurfViewFrame::OnMagnify      )
  EVT_COMBOBOX( ID_OBJECT,     SurfViewFrame::OnObject       )
  EVT_COMBOBOX( ID_MODE,       SurfViewFrame::OnMode         )
  EVT_BUTTON( ID_OPACITY,      SurfViewFrame::OnOpacity      )
  EVT_BUTTON( ID_SPECULAR,     SurfViewFrame::OnSpecular     )
  EVT_BUTTON( ID_SPEED,        SurfViewFrame::OnSpeed        )
  EVT_BUTTON( ID_SAVE,         SurfViewFrame::OnSave         )
  EVT_BUTTON( ID_REMOVE,       SurfViewFrame::OnRemove       )
  EVT_BUTTON( ID_RESET,        SurfViewFrame::OnReset        )

  EVT_CHECKBOX( ID_ANTIALIAS,  SurfViewFrame::OnAntialias    )
  EVT_CHECKBOX( ID_STATUS,     SurfViewFrame::OnStatus       )
  EVT_CHECKBOX( ID_BOX,        SurfViewFrame::OnBox          )
  EVT_CHECKBOX( ID_ROTATE,     SurfViewFrame::OnRotate       )
  EVT_CHECKBOX( ID_MOBILE,     SurfViewFrame::OnMobile       )
  EVT_CHAR( SurfViewFrame::OnChar )
  EVT_CHECKBOX( ID_PREVIEW,    SurfViewFrame::OnPreview      )
  EVT_COMBOBOX( ID_POSES,      SurfViewFrame::OnPoses        )
  EVT_BUTTON( ID_DELETE_POSE,  SurfViewFrame::OnDeletePose   )
  EVT_BUTTON( ID_VOL_REND_PARAMS,SurfViewFrame::OnVRParams   )
  EVT_COMBOBOX( ID_MATERIAL,   SurfViewFrame::OnMaterial     )
  EVT_CHECKBOX( ID_MIP,        SurfViewFrame::OnMIP          )
  EVT_CHECKBOX( ID_MIP_INVERT, SurfViewFrame::OnMIP_Invert   )
  EVT_BUTTON( ID_MATERL_OPACITY,SurfViewFrame::OnMaterialOpacity)
  EVT_BUTTON( ID_MATERL_COLOR, SurfViewFrame::OnMaterialColor)
  EVT_BUTTON( ID_SURF_STRENGTH,SurfViewFrame::OnSurfaceStrength)

  EVT_COMMAND_SCROLL( ID_AMB_RED_SLIDER,     SurfViewFrame::OnAnyAmbientSlider )
  EVT_COMMAND_SCROLL( ID_AMB_GREEN_SLIDER,   SurfViewFrame::OnAnyAmbientSlider )
  EVT_COMMAND_SCROLL( ID_AMB_BLUE_SLIDER,    SurfViewFrame::OnAnyAmbientSlider )

  EVT_COMMAND_SCROLL( ID_BG_RED_SLIDER,      SurfViewFrame::OnAnyBackgroundSlider )
  EVT_COMMAND_SCROLL( ID_BG_GREEN_SLIDER,    SurfViewFrame::OnAnyBackgroundSlider )
  EVT_COMMAND_SCROLL( ID_BG_BLUE_SLIDER,     SurfViewFrame::OnAnyBackgroundSlider )

  EVT_COMMAND_SCROLL( ID_COLOR_RED_SLIDER,   SurfViewFrame::OnAnyColorSlider )
  EVT_COMMAND_SCROLL( ID_COLOR_GREEN_SLIDER, SurfViewFrame::OnAnyColorSlider )
  EVT_COMMAND_SCROLL( ID_COLOR_BLUE_SLIDER,  SurfViewFrame::OnAnyColorSlider )

  EVT_COMMAND_SCROLL( ID_DIFFUSE_PERCENT_SLIDER,  SurfViewFrame::OnAnyDiffuseSlider )
  EVT_COMMAND_SCROLL( ID_DIFFUSE_EXPONENT_SLIDER, SurfViewFrame::OnAnyDiffuseSlider )
  EVT_COMMAND_SCROLL( ID_DIFFUSE_DIVISOR_SLIDER,  SurfViewFrame::OnAnyDiffuseSlider )

  EVT_COMMAND_SCROLL( ID_MAGNIFY_SLIDER, SurfViewFrame::OnMagnifySlider )

  EVT_COMMAND_SCROLL( ID_OPACITY_SLIDER, SurfViewFrame::OnOpacitySlider )

  EVT_COMMAND_SCROLL( ID_SPECULAR_PERCENT_SLIDER,  SurfViewFrame::OnAnySpecularSlider )
  EVT_COMMAND_SCROLL( ID_SPECULAR_EXPONENT_SLIDER, SurfViewFrame::OnAnySpecularSlider )
  EVT_COMMAND_SCROLL( ID_SPECULAR_DIVISOR_SLIDER,  SurfViewFrame::OnAnySpecularSlider )

  EVT_COMMAND_SCROLL( ID_SPEED_SLIDER,             SurfViewFrame::OnSpeedSlider )
  EVT_COMMAND_SCROLL( ID_CUT_DEPTH_SLIDER,         SurfViewFrame::OnCutDepthSlider )
  EVT_COMMAND_SCROLL( ID_PIXEL_SIZE_SLIDER,        SurfViewFrame::OnPixelSizeSlider )
  EVT_COMMAND_SCROLL( ID_SLICE_SPACING_SLIDER,     SurfViewFrame::OnSliceSpacingSlider )
  EVT_COMMAND_SCROLL( ID_INTR_VIEWS_SLIDER,        SurfViewFrame::OnIntermediateViews )
  EVT_COMMAND_SCROLL( ID_MATERL_OPACITY_SLIDER,    SurfViewFrame::OnMaterialOpacitySlider )
  EVT_COMMAND_SCROLL( ID_MATERL_THRESHOLD1_SLIDER, SurfViewFrame::OnAnyMatlThresholdSlider )
  EVT_COMMAND_SCROLL( ID_MATERL_THRESHOLD2_SLIDER, SurfViewFrame::OnAnyMatlThresholdSlider )
  EVT_COMMAND_SCROLL( ID_MATERL_RED_SLIDER,        SurfViewFrame::OnAnyMatlColorSlider )
  EVT_COMMAND_SCROLL( ID_MATERL_GREEN_SLIDER,      SurfViewFrame::OnAnyMatlColorSlider )
  EVT_COMMAND_SCROLL( ID_MATERL_BLUE_SLIDER,       SurfViewFrame::OnAnyMatlColorSlider )
  EVT_COMMAND_SCROLL( ID_SURF_STREN_SLIDER,        SurfViewFrame::OnSurfaceStrengthSlider )
  EVT_COMMAND_SCROLL( ID_EMISSION_POWER_SLIDER,    SurfViewFrame::OnSurfaceStrengthSlider )
  EVT_COMMAND_SCROLL( ID_SURF_PCT_POWER_SLIDER,    SurfViewFrame::OnSurfaceStrengthSlider )
  EVT_COMMAND_SCROLL( ID_CENTER1_SLIDER,           SurfViewFrame::OnCenterSlider )
  EVT_COMMAND_SCROLL( ID_WIDTH1_SLIDER,            SurfViewFrame::OnWidthSlider )
  EVT_BUTTON( ID_CT_LUNG,          SurfViewFrame::OnCTLung  )
  EVT_BUTTON( ID_CT_SOFT_TISSUE,   SurfViewFrame::OnCTSoftTissue  )
  EVT_BUTTON( ID_CT_BONE,          SurfViewFrame::OnCTBone  )
  EVT_BUTTON( ID_PET,              SurfViewFrame::OnPET     )
  EVT_BUTTON( ID_BACKWARD_SLICE,   SurfViewFrame::OnPrevious)
  EVT_BUTTON( ID_FORWARD_SLICE,    SurfViewFrame::OnNext    )
#ifdef __WXX11__
  //especially (only) need on X11 (w/out GTK) to get slider events.
  EVT_UPDATE_UI( ID_MAGNIFY_SLIDER, SurfViewFrame::OnUpdateUIMagnifySlider )
#endif
END_EVENT_TABLE()
//======================================================================
