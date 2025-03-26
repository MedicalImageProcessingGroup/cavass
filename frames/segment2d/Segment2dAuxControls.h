#pragma once

#include <wx/persist.h>
#include <wx/persist/toplevel.h>
#include "Segment2dFrame.h"
#include "PersistentSegment2dFrame.h"
//======================================================================
class Segment2dSlider;

class Segment2dAuxControls {
    friend PersistentSegment2dFrame;
    friend Segment2dFrame;

    Segment2dFrame*  mFrame;
    wxPanel          *mCp;
	wxSizer          *mBottomSizer;   //DO NOT DELETE in dtor!
    wxFlexGridSizer  *mFgs;
	wxFlexGridSizer  *mFgsButton;
	wxFlexGridSizer  *mFgsSliders;
    wxStaticBox      *mAuxBox;
	wxStaticBoxSizer *mAuxSizer;
	wxStaticText     *mSt1, *mSt2, *mSt3, *mSt4;
	wxComboBox       *mBrushSize;
	wxComboBox       *mIterates;
	wxButton         *mFeatureDisplay;
	wxComboBox       *mFeature;
	wxCheckBox       *mFeatureStatus;
	wxComboBox       *mTransform;
	wxButton         *mTransformSelect;
	wxButton         *mUpdate;
	wxTextCtrl       *mMinPointsCtrl;
	wxTextCtrl       *mAlphaCtrl;
	wxTextCtrl       *mBetaCtrl;
	wxTextCtrl       *mGammaCtrl;
	Segment2dSlider  *mWeight;
	Segment2dSlider  *mMean;
	Segment2dSlider  *mStdDev;
	Segment2dSlider  *mFeatureMin;
	Segment2dSlider  *mFeatureMax;

    void init ( );
	void setup_fgs ( wxPanel* cp, wxSizer* bottomSizer, const char* title, int cols );
	void display_fgs ( );
    void restoreAuxControls ( );
#if 0
    void saveAuxControls ( );  //unused
#endif

public:
    /** paint controls */
	Segment2dAuxControls ( Segment2dFrame* frame, wxPanel* cp, wxSizer* bottomSizer,
		const char* title, int currentBrushSize,
		bool update_flag=false );

    /** ilw controls */
	Segment2dAuxControls ( Segment2dFrame* frame, wxPanel* cp, wxSizer* bottomSizer,
		const char* title, int currentIterates, int currentMinPoints );

    /** live snake controls */
	Segment2dAuxControls ( Segment2dFrame* frame, wxPanel* cp, wxSizer* bottomSizer,
		const char* title, int currentIterates,
		double currentAlpha, double currentBeta, double currentGamma );

    /** feature controls */
	Segment2dAuxControls ( Segment2dFrame* frame, wxPanel* cp, wxSizer* bottomSizer,
		const char* title, int featureRange,
		int currentFeature, int currentFeatureStatus, int currentTransform,
		double currentWeight, double currentMean, double currentStdDev,
		double currentFeatureMin, double currentFeatureMax );

	~Segment2dAuxControls ( );

	void SetTransform ( int newTransform, int featureRange,
		double newFeatureMean, double newFeatureStdDev,
		double newFeatureMin, double newFeatureMax );

	void SetFeature ( int newFeature, bool newFeatureStatus,
		int newFeatureTransform, int featureRange, double newFeatureWeight,
		double newFeatureMean, double newFeatureStdDev,
		double newFeatureMin, double newFeatureMax );

	void UpdateWeight ( double newval );
	void UpdateMean ( double newval );
	void UpdateStdDev ( double newval );
	void UpdateFeatureMin ( double newval );
	void UpdateFeatureMax ( double newval );
};
//======================================================================

