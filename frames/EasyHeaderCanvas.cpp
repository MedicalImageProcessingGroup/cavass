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
 * \file:  EasyHeaderCanvas.cpp
 * \brief  EasyHeaderCanvas class implementation
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"

using namespace std;

//----------------------------------------------------------------------
EasyHeaderCanvas::EasyHeaderCanvas ( void )  {  puts("EasyHeaderCanvas()");  }
//----------------------------------------------------------------------
EasyHeaderCanvas::EasyHeaderCanvas ( wxWindow* parent, MainFrame* parent_frame,
    wxWindowID id, const wxPoint &pos, const wxSize &size )
    : MainCanvas ( parent, parent_frame, id, pos, size )
//    : wxPanel ( parent, id, pos, size )
//    : wxScrolledWindow ( parent, id, pos, size, wxSUNKEN_BORDER )
{
	mFgs = NULL;
	mDateLabel = NULL;
	mTimeLabel = NULL;
	mStudyLabel = NULL;
	mSeriesLabel = NULL;
	mDimensionLabel = NULL;
	mSliceWidthLabel = NULL;
	mSliceHeightLabel = NULL;
	mBitsPerPixelLabel = NULL;
	mPixelSizeLabel = NULL;
	mMinDensityLabel = NULL;
	mMaxDensityLabel = NULL;
	mNumSlicesLabel = NULL;
	mFirstSliceLocLabel = NULL;
	mSliceSpacingLabel = NULL;
	mNumVolumesLabel = NULL;
	mFirstVolumeLocLabel = NULL;
	mVolumeSpacingLabel = NULL;
	mDateCtrl = NULL;
	mTimeCtrl = NULL;
	mStudyCtrl = NULL;
	mSeriesCtrl = NULL;
	mDimensionCtrl = NULL;
	mSliceWidthCtrl = NULL;
	mSliceHeightCtrl = NULL;
	mBitsPerPixelCtrl = NULL;
	mPixelSizeCtrl = NULL;
	mMinDensityCtrl = NULL;
	mMaxDensityCtrl = NULL;
	mNumSlicesCtrl = NULL;
	mFirstSliceLocCtrl = NULL;
	mSliceSpacingCtrl = NULL;
	mNumVolumesCtrl = NULL;
	mFirstVolumeLocCtrl = NULL;
	mVolumeSpacingCtrl = NULL;
    mOverallXSize = mOverallYSize = mOverallZSize = 0;
  
}
//----------------------------------------------------------------------
EasyHeaderCanvas::~EasyHeaderCanvas ( void ) {
    cout << "EasyHeaderCanvas::~EasyHeaderCanvas" << endl;
    wxLogMessage( "EasyHeaderCanvas::~EasyHeaderCanvas" );
    release(); /* mCanvassData released in MainCanvas */
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::release ( void ) {
	if (mFgs == NULL)
		return;
	removeVolumeControls();
	mFgs->Detach(mSliceSpacingCtrl);
	delete mSliceSpacingCtrl;
	mFgs->Detach(mSliceSpacingLabel);
	delete mSliceSpacingLabel;
	mFgs->Detach(mFirstSliceLocCtrl);
	delete mFirstSliceLocCtrl;
	mFgs->Detach(mFirstSliceLocLabel);
	delete mFirstSliceLocLabel;
	mFgs->Detach(mNumSlicesCtrl);
	delete mNumSlicesCtrl;
	mFgs->Detach(mNumSlicesLabel);
	delete mNumSlicesLabel;
	mFgs->Detach(mMaxDensityCtrl);
	delete mMaxDensityCtrl;
	mFgs->Detach(mMaxDensityLabel);
	delete mMaxDensityLabel;
	mFgs->Detach(mMinDensityCtrl);
	delete mMinDensityCtrl;
	mFgs->Detach(mMinDensityLabel);
	delete mMinDensityLabel;
	mFgs->Detach(mPixelSizeCtrl);
	delete mPixelSizeCtrl;
	mFgs->Detach(mPixelSizeLabel);
	delete mPixelSizeLabel;
	mFgs->Detach(mBitsPerPixelCtrl);
	delete mBitsPerPixelCtrl;
	mFgs->Detach(mBitsPerPixelLabel);
	delete mBitsPerPixelLabel;
	mFgs->Detach(mSliceHeightCtrl);
	delete mSliceHeightCtrl;
	mFgs->Detach(mSliceHeightLabel);
	delete mSliceHeightLabel;
	mFgs->Detach(mSliceWidthCtrl);
	delete mSliceWidthCtrl;
	mFgs->Detach(mSliceWidthLabel);
	delete mSliceWidthLabel;
	mFgs->Detach(mDimensionCtrl);
	delete mDimensionCtrl;
	mFgs->Detach(mDimensionLabel);
	delete mDimensionLabel;
	mFgs->Detach(mSeriesCtrl);
	delete mSeriesCtrl;
	mFgs->Detach(mSeriesLabel);
	delete mSeriesLabel;
	mFgs->Detach(mStudyCtrl);
	delete mStudyCtrl;
	mFgs->Detach(mStudyLabel);
	delete mStudyLabel;
	mFgs->Detach(mTimeCtrl);
	delete mTimeCtrl;
	mFgs->Detach(mTimeLabel);
	delete mTimeLabel;
	mFgs->Detach(mDateCtrl);
	delete mDateCtrl;
	mFgs->Detach(mDateLabel);
	delete mDateLabel;
	mFgs = NULL;
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::removeVolumeControls ( void ) {
	if (mNumVolumesCtrl == NULL)
		return;
	mFgs->Detach(mVolumeSpacingCtrl);
	delete mVolumeSpacingCtrl;
	mFgs->Detach(mVolumeSpacingLabel);
	delete mVolumeSpacingLabel;
	mFgs->Detach(mFirstVolumeLocCtrl);
	delete mFirstVolumeLocCtrl;
	mFgs->Detach(mFirstVolumeLocLabel);
	delete mFirstVolumeLocLabel;
	mFgs->Detach(mNumVolumesCtrl);
	delete mNumVolumesCtrl;
	mFgs->Detach(mNumVolumesLabel);
	delete mNumVolumesLabel;
	mNumVolumesCtrl = NULL;
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::reload ( void ) {
	Refresh();
}

//----------------------------------------------------------------------
/** \brief note: spacebar mimics middle mouse button.
 */
void EasyHeaderCanvas::OnChar ( wxKeyEvent& e ) {
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
void EasyHeaderCanvas::OnRightDown ( wxMouseEvent& e ) {
#if 0
    SetFocus();  //to regain/recapture keypress events
    if (isLoaded(0)) {
        mCavassData->mDisplay = false;  //!mCavassData->mDisplay;
        reload();
    }
#else
    //gjg: simulate the middle button (useful on a 2-button mouse)
    OnMiddleDown( e );
#endif
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::OnRightUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
// 7/26/07 Dewey Odhner
void EasyHeaderCanvas::OnMiddleDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::OnMiddleUp ( wxMouseEvent& e ) {
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::OnLeftDown ( wxMouseEvent& e ) {
    SetFocus();  //to regain/recapture keypress events
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::OnLeftUp ( wxMouseEvent& e ) {
    cout << "OnLeftUp" << endl;
    wxLogMessage("OnLeftUp");
    SetCursor( *wxSTANDARD_CURSOR );
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::OnPaint ( wxPaintEvent& e ) {
}
//----------------------------------------------------------------------
void EasyHeaderCanvas::paint ( wxDC* dc ) {

}

//----------------------------------------------------------------------
void EasyHeaderCanvas::OnSize ( wxSizeEvent& e ) {
    //called when the size of the window/viewing area changes
    Layout();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS ( EasyHeaderCanvas, wxPanel )
BEGIN_EVENT_TABLE       ( EasyHeaderCanvas, wxPanel )
    EVT_SIZE(             EasyHeaderCanvas::OnSize         )
    EVT_LEFT_DOWN(        EasyHeaderCanvas::OnLeftDown     )
    EVT_LEFT_UP(          EasyHeaderCanvas::OnLeftUp       )
    EVT_MIDDLE_DOWN(      EasyHeaderCanvas::OnMiddleDown   )
    EVT_MIDDLE_UP(        EasyHeaderCanvas::OnMiddleUp     )
    EVT_RIGHT_DOWN(       EasyHeaderCanvas::OnRightDown    )
    EVT_RIGHT_UP(         EasyHeaderCanvas::OnRightUp      )
    EVT_CHAR(             EasyHeaderCanvas::OnChar         )
END_EVENT_TABLE()
//======================================================================
