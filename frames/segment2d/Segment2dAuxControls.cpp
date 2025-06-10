#include "cavass.h"
#include <wx/wx.h>
#include "Segment2dAuxControls.h"
#include "Segment2dSlider.h"
#include "Segment2dFrame.h"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** ctor for ILW (iterative live wire) controls */
Segment2dAuxControls::Segment2dAuxControls ( Segment2dFrame* frame,
    wxPanel* cp, wxSizer* bottomSizer,
    const char* const title, int currentIterates, int currentMinPoints )
    : mFrame( frame )
{
    init();
#if 1
    mCp = cp;
    mBottomSizer = bottomSizer;
    //box for ilw controls
    mAuxBox = new wxStaticBox( mCp, wxID_ANY, title );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on left

    mFgs = new wxFlexGridSizer( 0, 2, 0, 0 );  //rows, cols, vgap, hgap
    mFgs->SetFlexibleDirection( wxBOTH );
    mFgs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    //row 0, col 0
    mSt1 = new wxStaticText( mAuxBox, wxID_ANY, "Iterates:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    ::setBoxColor( mSt1 );
    mFgs->Add( mSt1, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    //row 0, col 1
    //prep strings for combo box
    wxArrayString as;
    for (int j = 1; j <= 8; j++)
        as.Add(wxString::Format("%d", j));
    //create combo box
    mIterates = new wxComboBox(mAuxBox, Segment2dFrame::ID_ITERATES,
                               wxString::Format("%d", currentIterates),
                               wxDefaultPosition, wxDefaultSize,
                               as, wxCB_READONLY);
    ::setColor(mIterates);
    mFgs->Add(mIterates, 0, wxALL|wxEXPAND, 5 );

    //row 1, col 0
    mSt2 = new wxStaticText( mAuxBox, wxID_ANY, _("min ctrl pts:"), wxDefaultPosition, wxDefaultSize, 0 );
    ::setBoxColor(mSt2);
    mFgs->Add( mSt2, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
    //row 1, col 1
    mMinPointsCtrl = new wxTextCtrl(mAuxBox, Segment2dFrame::ID_MIN_POINTS,
                                    wxString::Format("%d", currentMinPoints));
    ::setColor(mMinPointsCtrl);
    mFgs->Add( mMinPointsCtrl, 0, wxALL|wxEXPAND, 5 );

    mAuxSizer->Add( mFgs, 0, 0, 0 );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on right
    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    cp->Refresh();
#else
    setup_fgs(cp, bottomSizer, title, 2);
    mSt1 = new wxStaticText(cp, wxID_ANY, "Iterates:");
    ::setBoxColor(mSt1);
    mFgsButton->Add(mSt1, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString as;
    for (int j = 1; j <= 8; j++)
        as.Add(wxString::Format("%d", j));
    mIterates = new wxComboBox(cp, Segment2dFrame::ID_ITERATES,
                               wxString::Format("%d", currentIterates),
                               wxDefaultPosition, wxSize(buttonWidth, buttonHeight),
                               as, wxCB_READONLY);
    ::setColor(mIterates);
    mFgsButton->Add(mIterates, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mSt2 = new wxStaticText(cp, wxID_ANY, "Min. Ctrl. Pts.:");
    ::setBoxColor(mSt2);
    mFgsButton->Add(mSt2, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mMinPointsCtrl = new wxTextCtrl(cp, Segment2dFrame::ID_MIN_POINTS,
                                    wxString::Format("%d", currentMinPoints));
    ::setColor(mMinPointsCtrl);
    mFgsButton->Add(mMinPointsCtrl, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    display_fgs();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** ctor for live snake controls */
Segment2dAuxControls::Segment2dAuxControls ( Segment2dFrame* frame,
    wxPanel* cp, wxSizer* bottomSizer,
    const char* const title, int currentIterates, double currentAlpha,
    double currentBeta, double currentGamma )
    : mFrame( frame )
{
    init();
#if 1
    mCp = cp;
    mBottomSizer = bottomSizer;
    //box for ilw controls
    mAuxBox = new wxStaticBox( mCp, wxID_ANY, title );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on left

    mFgs = new wxFlexGridSizer( 0, 2, 0, 0 );  //rows, cols, vgap, hgap
    mFgs->AddGrowableCol(1);
    mFgs->SetFlexibleDirection( wxBOTH );
    mFgs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    //row 0, col 0
    mSt1 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, _("iterates:"), wxDefaultPosition, wxDefaultSize, 0 );
    ::setBoxColor( mSt1 );
    mFgs->Add( mSt1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
    //row 0, col 1
    //prep strings for combo box
    wxArrayString as;
    for (int j = 1; j <= 9; j++)
        as.Add(wxString::Format("%d", j < 6 ? j : (j - 4) * 5));
    //create combo box
    mIterates = new wxComboBox( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_ITERATES,
                        wxString::Format( "%d",
                                                currentIterates < 6 ? currentIterates : (currentIterates - 4) * 5),
                               wxDefaultPosition, wxDefaultSize, as, wxCB_READONLY );
    ::setColor( mIterates );
    mFgs->Add( mIterates, 0, wxALL|wxEXPAND, 5 );
    //row 1, col 0
    mSt2 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, _("alpha:"), wxDefaultPosition, wxDefaultSize, 0 );
    ::setBoxColor( mSt2 );
    mFgs->Add( mSt2, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
    //row 1, col 1
    mAlphaCtrl = new wxTextCtrl( mAuxSizer->GetStaticBox(),
                                 Segment2dFrame::ID_ALPHA,
                                 wxString::Format("%.4f", currentAlpha),
                                 wxDefaultPosition, wxSize(150,-1), wxTE_RIGHT );
    ::setColor(mAlphaCtrl);
    mFgs->Add( mAlphaCtrl, 0, wxALL|wxEXPAND, 5 );
    //row 2, col 0
    mSt3 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, _("beta:"), wxDefaultPosition, wxDefaultSize, 0 );
    ::setBoxColor( mSt3 );
    mFgs->Add( mSt3, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
    //row 2, col 1
    mBetaCtrl = new wxTextCtrl( mAuxSizer->GetStaticBox(),
                                Segment2dFrame::ID_BETA,
                                wxString::Format("%.4f", currentBeta),
                                wxDefaultPosition, wxSize(150,-1), wxTE_RIGHT );
    ::setColor(mBetaCtrl);
    mFgs->Add( mBetaCtrl, 0, wxALL|wxEXPAND, 5 );
    //row 3, col 0
    mSt4 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, _("gamma:"), wxDefaultPosition, wxDefaultSize, 0 );
    ::setBoxColor( mSt4 );
    mFgs->Add( mSt4, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
    //row 3, col 1
    mGammaCtrl = new wxTextCtrl(mAuxSizer->GetStaticBox(), Segment2dFrame::ID_GAMMA,
                                wxString::Format("%.4f", currentGamma),
                                wxDefaultPosition, wxSize(150,-1), wxTE_RIGHT );
    ::setColor(mGammaCtrl);
    mFgs->Add( mGammaCtrl, 0, wxALL|wxEXPAND, 5 );

    mAuxSizer->Add( mFgs, 0, 0, 0 );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on right
    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    cp->Refresh();
#else
    //original
    setup_fgs(cp, bottomSizer, title, 2);
    mSt1 = new wxStaticText(cp, wxID_ANY, "Iterates:");
    ::setBoxColor(mSt1);
    mFgsButton->Add(mSt1, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString as;
    for (int j = 1; j <= 9; j++)
        as.Add(wxString::Format("%d", j < 6 ? j : (j - 4) * 5));
    mIterates = new wxComboBox(cp, Segment2dFrame::ID_ITERATES,
                               wxString::Format("%d",
                                                currentIterates < 6 ? currentIterates : (currentIterates - 4) * 5),
                               wxDefaultPosition, wxSize(buttonWidth, buttonHeight),
                               as, wxCB_READONLY);
    ::setColor(mIterates);
    mFgsButton->Add(mIterates, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mSt2 = new wxStaticText(cp, wxID_ANY, "alpha:");
    ::setBoxColor(mSt2);
    mFgsButton->Add(mSt2, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mAlphaCtrl = new wxTextCtrl(cp, Segment2dFrame::ID_ALPHA,
                                wxString::Format("%f", currentAlpha));
    ::setColor(mAlphaCtrl);
    mFgsButton->Add(mAlphaCtrl, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mSt3 = new wxStaticText(cp, wxID_ANY, "beta:");
    ::setBoxColor(mSt3);
    mFgsButton->Add(mSt3, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mBetaCtrl = new wxTextCtrl(cp, Segment2dFrame::ID_BETA,
                               wxString::Format("%f", currentBeta));
    ::setColor(mBetaCtrl);
    mFgsButton->Add(mBetaCtrl, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mSt4 = new wxStaticText(cp, wxID_ANY, "gamma:");
    ::setBoxColor(mSt4);
    mFgsButton->Add(mSt4, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    mGammaCtrl = new wxTextCtrl(cp, Segment2dFrame::ID_GAMMA,
                                wxString::Format("%f", currentGamma));
    ::setColor(mGammaCtrl);
    mFgsButton->Add(mGammaCtrl, 0,
                    wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 0);
    display_fgs();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** ctor for paint & train controls */
Segment2dAuxControls::Segment2dAuxControls ( Segment2dFrame* frame,
    wxPanel* cp, wxSizer* bottomSizer,
	const char* const title, int currentBrushSize, bool update_flag )
    : mFrame( frame )
{
    init();
#if 1
    mCp = cp;
    mBottomSizer = bottomSizer;
    //box for paint or train controls
    mAuxBox = new wxStaticBox( mCp, wxID_ANY, title );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on left

    mFgs = new wxFlexGridSizer( 0, 2, 0, 0 );  //rows, cols, vgap, hgap
    mFgs->SetFlexibleDirection( wxBOTH );
    mFgs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    //mFgs = new wxFlexGridSizer( 2, 20, 20 );  //cols, vgap, hgap
    //gs->AddGrowableCol( 1 );
    //auto gs = new wxGridSizer( 2, 20, 20 );

    //row 0, col 0
    mSt1 = new wxStaticText( mAuxBox, wxID_ANY, "Brush Size:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    ::setBoxColor( mSt1 );
    mFgs->Add( mSt1, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    //row 0, col 1
    //prep strings for brush size combo box
    static const char sa[][5] = { "X", "1", "2", "5", "10", "15", "20", "25",
                                  "30", "35", "40", "50", "60", "Fill" };
    int ii = 0;
    assert( currentBrushSize <= atoi(sa[sizeof(sa)/sizeof(sa[0])-2]) );
    if (currentBrushSize < 0)
        ii = sizeof(sa)/sizeof(sa[0])-1;
    else if (currentBrushSize)
        for (ii=1; atoi(sa[ii])<currentBrushSize; ii++)
            ;
    wxArrayString as;
    for (unsigned int j=0; j<sizeof(sa)/sizeof(sa[0]); j++)
        as.Add(sa[j]);
    //brush size combo box
    mBrushSize = new wxComboBox(mAuxBox, Segment2dFrame::ID_BRUSH_SIZE,
                                currentBrushSize==0? wxString("X"):
                                currentBrushSize<0? wxString("Fill"):
                                wxString::Format("%d", currentBrushSize),
                                wxDefaultPosition, wxDefaultSize,
                                as, wxCB_READONLY );
    ::setColor( mBrushSize );
#if 0
    string tmp = title;
    if (tmp == "Paint Controls") {
        cout << tmp << endl;
//        mBrushSize->SetName( paintName );  //very important for persistence
    } else if (tmp == "Train Controls") {
        cout << "train" << endl;
//        mBrushSize->SetName( trainName );  //very important for persistence
    } else {
        cerr << tmp << "Segment2dAuxControls::Segment2dAuxControls unrecognized title." << endl;
    }
#endif
    //mFgs->Add( mBrushSize, 1, wxEXPAND, 10 );
    mFgs->Add( mBrushSize, 0, wxALL, 5 );

    //optionally add an 'update' button
    if (update_flag) {
        mFgs->Add( 0, 0, 1, wxEXPAND, 5 );  //spacer
        mUpdate = new wxButton( mAuxBox, Segment2dFrame::ID_FEATURE_UPDATE, "Update" );
        ::setColor( mUpdate );
        //mFgs->Add( mUpdate, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
        mFgs->Add( mUpdate, 0, wxALL|wxEXPAND, 5 );
    }
#if 0
    if (Preferences::getDejaVuMode()) {
        //cout << "before mLevel=" << mLevel->GetValue() << endl;
        int selection = mBrushSize->GetSelection();
        wxPersistenceManager::Get().RegisterAndRestore( mBrushSize );
        //cout << "after mLevel=" << mLevel->GetValue() << endl;
    }
#endif
    mAuxSizer->Add( mFgs, 0, 0, 0 );
    mAuxSizer->Add( 20, 0, 1, wxGROW|wxALL );  //spacer on right
    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    cp->Refresh();
#else
	//setup_fgs(cp, bottomSizer, title, 2);
    mCp = cp;
    mBottomSizer = bottomSizer;

    mAuxBox = new wxStaticBox( cp, wxID_ANY, title );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
//    mFgs = new wxFlexGridSizer( 2, 0, 5 );  //1 col,vgap,hgap
//    mFgs->SetMinSize( controlsWidth, 0 );
//    mFgs->AddGrowableCol( 0 );
//    mFgsButton = new wxFlexGridSizer( cols, 1, 1 );  // cols,vgap,hgap
    mFgs = nullptr;
    mFgsButton = nullptr;

    mSt1 = new wxStaticText( mAuxBox, wxID_ANY, "brush size:", wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    ::setBoxColor( mSt1 );
    mAuxSizer->Add( mSt1, 0, wxALL|wxEXPAND, 0 );

    //strings for brush size combo box
    static const char sa[][5] = { "X", "1", "2", "5", "10", "15", "20", "25",
                                  "30", "35", "40", "50", "60", "Fill" };
	int ii = 0;
	assert( currentBrushSize <= atoi(sa[sizeof(sa)/sizeof(sa[0])-2]) );
	if (currentBrushSize < 0)
		ii = sizeof(sa)/sizeof(sa[0])-1;
	else if (currentBrushSize)
		for (ii=1; atoi(sa[ii])<currentBrushSize; ii++)
			;
	wxArrayString as;
	for (unsigned int j=0; j<sizeof(sa)/sizeof(sa[0]); j++)
		as.Add(sa[j]);
    //brush size combo box
	mBrushSize = new wxComboBox(mAuxBox, Segment2dFrame::ID_BRUSH_SIZE,
		currentBrushSize==0? wxString("X"):
		currentBrushSize<0? wxString("Fill"):
		wxString::Format("%d", currentBrushSize),
		wxDefaultPosition, wxDefaultSize,
		as, wxCB_READONLY );
	::setColor( mBrushSize );
	mAuxSizer->Add( mBrushSize, 0, wxALL|wxEXPAND, 0 );

    //optionally add an 'update' button
	if (update_flag) {
		mUpdate = new wxButton( mAuxBox, Segment2dFrame::ID_FEATURE_UPDATE, "Update" );
		::setColor( mUpdate );
		mAuxSizer->Add( mUpdate, 1, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	}

	//display_fgs();
    //if (mFgsSliders)    mFgs->Add( mFgsSliders, 0, wxGROW|wxALL, 10 );
    //mFgs->Add( mFgsButton, 0, wxGROW|wxALL, 10 );
    //mAuxSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Layout();
    cp->Refresh();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** ctor for feature controls (most complicated set of controls: some fixed,
 *  some dynamic).
 *  features: Higher Density, Lower Density, Gradient1, Gradient2, Gradient3, Gradient4
 *  each feature can be independently on or off
 *  transforms: Linear, Gaussian, Inv. Linear,  Inv. Gaussian, Hyperbolic, Inv. Hyperbolic
 *  transform parameters:
 *      linear:          weight, feature min, feature max
 *      inv. linear:     weight, feature min, feature max
 *      gaussian:        weight, mean, std. dev.
 *      inv. gaussian:   weight, mean, std. dev.
 *      hyperbolic:      weight, mean, std. dev.
 *      inv. hyperbolic: weight, mean, std. dev.
 */
Segment2dAuxControls::Segment2dAuxControls ( Segment2dFrame* frame,
    wxPanel* cp, wxSizer* bottomSizer,
	const char* const title, int featureRange, int currentFeature,
    int currentFeatureStatus, int currentTransform, double currentWeight,
    double currentMean, double currentStdDev, double currentFeatureMin,
    double currentFeatureMax )
    : mFrame( frame )
{
    init();
#if 0  //original
	setup_fgs(cp, bottomSizer, title, 3);
    ::setSliderBoxColor( mAuxBox );
    mFgsSliders = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
	mSt1 = new wxStaticText( cp, wxID_ANY, "Feature:" );
	::setColor( mSt1 );
	mFgsButton->Add( mSt1, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

	wxArrayString as1;
    assert( (sizeof(Segment2dFeatureNames)/sizeof(Segment2dFeatureNames[0])) == SEGMENT2D_NAMES );
//	for (unsigned int j=0; j<sizeof(Segment2dFeatureNames)/sizeof(Segment2dFeatureNames[0]); j++)
    for (int j = 0; j < SEGMENT2D_NAMES; j++) {
        as1.Add(Segment2dFeatureNames[j]);
    }
	mFeature = new wxComboBox(cp, Segment2dFrame::ID_FEATURE,
		Segment2dFeatureNames[currentFeature],
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		as1, wxCB_READONLY );
	::setColor( mFeature );
	mFgsButton->Add( mFeature, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mFeatureStatus = new wxCheckBox( cp, Segment2dFrame::ID_FEATURE_STATUS,
		"Feature On" );
	::setColor( mFeatureStatus );
	mFeatureStatus->SetValue( 0!=currentFeatureStatus );
	mFgsButton->Add( mFeatureStatus, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mSt2 = new wxStaticText( cp, wxID_ANY, "Transform:" );
	::setColor( mSt2 );
	mFgsButton->Add( mSt2, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	wxArrayString as2;
	for (unsigned int j=0; j<sizeof(Segment2dTransformNames)/
			sizeof(Segment2dTransformNames[0]); j++)
		as2.Add(Segment2dTransformNames[j]);
	mTransform = new wxComboBox(cp, Segment2dFrame::ID_TRANSFORM,
		Segment2dTransformNames[currentTransform],
		wxDefaultPosition, wxSize(buttonWidth,buttonHeight),
		as2, wxCB_READONLY );
	::setColor( mTransform );
	mFgsButton->Add( mTransform, 0,
		wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
	mUpdate = new wxButton( cp, Segment2dFrame::ID_FEATURE_UPDATE, "Update" );
	::setColor( mUpdate );
    //mFgsButton->Add( mUpdate, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    mFgsButton->Add( mUpdate, 1, wxGROW|wxLEFT|wxRIGHT, 0 );
	SetFeature(currentFeature, 0!=currentFeatureStatus, currentTransform,
		featureRange, currentWeight,
		currentMean, currentStdDev, currentFeatureMin, currentFeatureMax);
	display_fgs();
#else
    //setup_fgs(cp, bottomSizer, "Feature Controls", 3);
    mCp = cp;
    mBottomSizer = bottomSizer;
    mAuxBox = new wxStaticBox( cp, -1, title );
    //::setSliderBoxColor( mAuxBox );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
    mFgs = new wxFlexGridSizer( 3, 0, 5 );  //cols,vgap,hgap
    mFgs->SetMinSize( controlsWidth, 0 );
    mFgs->AddGrowableCol( 0 );

    mFgsButton = new wxFlexGridSizer( 0, 3, 0, 0 );  // rows,cols,vgap,hgap
    mFgsButton->SetFlexibleDirection( wxBOTH );
    mFgsButton->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    //

    // right side (controls) - - - -

    //first row: feature
    mSt1 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, "Feature:", wxDefaultPosition, wxDefaultSize, 0 );
    ::setColor( mSt1 );
    mFgsButton->Add( mSt1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 0 );
    //feature combo box
    mFeature = new wxComboBox(mAuxSizer->GetStaticBox(), Segment2dFrame::ID_FEATURE,
                              Segment2dFrame::featureNames[currentFeature],
                              wxDefaultPosition, wxDefaultSize,
                              Segment2dFrame::featureNames, wxCB_READONLY );
    ::setColor( mFeature );
    mFgsButton->Add( mFeature, 0, wxALL, 5 );
    //feature status check box
    mFeatureStatus = new wxCheckBox( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_FEATURE_STATUS,
                                     "Feature On", wxDefaultPosition, wxDefaultSize, 0 );
    ::setColor( mFeatureStatus );
    mFeatureStatus->SetValue( 0!=currentFeatureStatus );
    mFgsButton->Add( mFeatureStatus, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

    //second row: transform
    mSt2 = new wxStaticText( mAuxSizer->GetStaticBox(), wxID_ANY, _("Transform:"), wxDefaultPosition, wxDefaultSize, 0 );
    mFgsButton->Add( mSt2, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 0 );
    //transform combo box
    mTransform = new wxComboBox(mAuxSizer->GetStaticBox(), Segment2dFrame::ID_TRANSFORM,
                                Segment2dFrame::transformNames[currentTransform],
                                wxDefaultPosition, wxDefaultSize,
                                Segment2dFrame::transformNames, wxCB_READONLY );
    ::setColor( mTransform );
    mFgsButton->Add( mTransform, 0, wxALL, 5 );
    //update button
    mUpdate = new wxButton( mAuxSizer->GetStaticBox(), Segment2dFrame::ID_FEATURE_UPDATE, "Update", wxDefaultPosition, wxDefaultSize, 0 );
    ::setColor( mUpdate );
    mFgsButton->Add( mUpdate, 0, wxALL|wxEXPAND, 5 );

    // left side (sliders) - - - -

    mFgsSliders = new wxFlexGridSizer( 0, 2, 0, 0 );  //rows,cols,vgap,hgap
    SetFeature( currentFeature, 0!=currentFeatureStatus, currentTransform,
               featureRange, currentWeight,
               currentMean, currentStdDev, currentFeatureMin, currentFeatureMax );

    // display_fgs();
    mFgs->Add( mFgsSliders, 1, wxEXPAND, 0 );  //left
    mFgs->Add( 50, 0, 1, wxEXPAND, 0 );  //middle spacer
    mFgs->Add( mFgsButton, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 10 );  //right
    mAuxSizer->Add( mFgs, 1, wxEXPAND, 0 );
    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 5 );  //was 1, expand, and 0
    mBottomSizer->Layout();
    mCp->Refresh();
    //
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \todo needs a great deal of work to clean up */
Segment2dAuxControls::~Segment2dAuxControls ( ) {
#if 1
    //saveCurrentAuxControls();
    if (mFrame->mPersistentMe != nullptr) {
        auto p = dynamic_cast<PersistentSegment2dFrame*>( mFrame->mPersistentMe );
        //update persisted values
        if (p != nullptr)    p->saveAuxControls();
    }

    //interesting use of comma operator (to make two statements into one)!
    if (mStdDev)
        delete mStdDev, mStdDev = nullptr;
    if (mMean)
        delete mMean, mMean = nullptr;
    if (mFeatureMax)
        delete mFeatureMax, mFeatureMax = nullptr;
    if (mFeatureMin)
        delete mFeatureMin, mFeatureMin = nullptr;
    if (mWeight)
        delete mWeight, mWeight = nullptr;
    if (mFgsSliders)
        mFgs->Remove(mFgsSliders), mFgsSliders = nullptr;
    if (mUpdate) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mUpdate);
        else                          mFgs->Detach(mUpdate);
        delete mUpdate;
        mUpdate = nullptr;
    }
    if (mTransformSelect) {
        mFgsButton->Detach(mTransformSelect);
        delete mTransformSelect;
        mTransformSelect = nullptr;
    }
    if (mTransform) {
        mFgsButton->Detach(mTransform);
        delete mTransform;
        mTransform = nullptr;
    }
    if (mGammaCtrl) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mGammaCtrl);
        else                          mFgs->Detach(mGammaCtrl);
        delete mGammaCtrl;
        mGammaCtrl = nullptr;
    }
    if (mSt4) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mSt4);
        else                          mFgs->Detach( mSt4 );
        delete mSt4;
        mSt4 = nullptr;
    }
    if (mBetaCtrl) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mBetaCtrl);
        else                          mFgs->Detach(mBetaCtrl);
        delete mBetaCtrl;
        mBetaCtrl = nullptr;
    }
    if (mSt3) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mSt3);
        else                          mFgs->Detach( mSt3 );
        delete mSt3;
        mSt3 = nullptr;
    }
    if (mAlphaCtrl) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mAlphaCtrl);
        else                          mFgs->Detach(mAlphaCtrl);
        delete mAlphaCtrl;
        mAlphaCtrl = nullptr;
    }
    if (mMinPointsCtrl) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mMinPointsCtrl);
        else                          mFgs->Detach(mMinPointsCtrl);
        delete mMinPointsCtrl;
        mMinPointsCtrl = nullptr;
    }
    if (mSt2) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mSt2);
        else                          mFgs->Detach( mSt2 );
        delete mSt2;
        mSt2 = nullptr;
    }
    if (mFeatureDisplay) {
        mFgsButton->Detach(mFeatureDisplay);
        delete mFeatureDisplay;
        mFeatureDisplay = nullptr;
    }
    if (mFeatureStatus) {
        mFgsButton->Detach(mFeatureStatus);
        delete mFeatureStatus;
        mFeatureStatus = nullptr;
    }
    if (mFeature) {
        mFgsButton->Detach(mFeature);
        delete mFeature;
        mFeature = nullptr;
    }
#if 1
    if (mBrushSize) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mBrushSize);
        else                          mFgs->Detach(mBrushSize);
        delete mBrushSize;
        mBrushSize = nullptr;
    }
#endif
    if (mIterates) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mIterates);
        else                          mFgs->Detach(mIterates);
        delete mIterates;
        mIterates = nullptr;
    }
    if (mSt1) {
        if (mFgsButton != nullptr)    mFgsButton->Detach(mSt1);
        else                          mFgs->Detach(mSt1);
        delete mSt1;
        mSt1 = nullptr;
    }
    if (mFgsButton != nullptr)    mFgs->Remove(mFgsButton);
    if (mFgs != nullptr)    mAuxSizer->Remove(mFgs);
    mBottomSizer->Remove(mAuxSizer);
#endif
    mBottomSizer->Layout();
    mCp->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * restore (persist) specified controls
 */
void Segment2dAuxControls::restoreAuxControls ( ) {
#if 0  //for debugging only
    cout << "Segment2dAuxControls::restoreAuxControls()" << endl;
    cout << mAuxBox->GetLabel() << endl;
#endif
    assert( mFrame != nullptr );
    assert( mFrame->mPersistentMe != nullptr );
    auto tmp = dynamic_cast<PersistentSegment2dFrame*>( mFrame->mPersistentMe );
    assert( tmp != nullptr);
    if (tmp == nullptr)    return;

    if (mAuxBox->GetLabel() == "Feature Controls")           tmp->restoreFeatureControls();
    else if (mAuxBox->GetLabel() == "ILW Controls")          tmp->restoreILWControls();
    else if (mAuxBox->GetLabel() == "LiveSnake Controls")    tmp->restoreLiveSnakeControls();
    else if (mAuxBox->GetLabel() == "Paint Controls")        tmp->restorePaintControls();
    else if (mAuxBox->GetLabel() == "Train Controls")        tmp->restoreTrainControls();
    else
        cerr << "Segment2dAuxControls::restoreAuxControls: "
             << mAuxBox->GetLabel() << "?" << endl;
}
#if 0  //unused
/**
 * \todo save (persist) state of current controls
 */
void Segment2dAuxControls::saveAuxControls ( ) {
    cout << "Segment2dAuxControls::saveAuxControls()" << endl;
    //note: nothing to save for Review, Report, and Peek
    if (mAuxBox->GetLabel() == "Feature Controls") {
        cout << mAuxBox->GetLabel() << endl;
    } else if (mAuxBox->GetLabel() == "ILW Controls") {
        cout << mAuxBox->GetLabel() << endl;
    } else if (mAuxBox->GetLabel() == "LiveSnake Controls") {
        cout << mAuxBox->GetLabel() << endl;
    } else if (mAuxBox->GetLabel() == "Paint Controls") {
        cout << mAuxBox->GetLabel() << endl;
    } else if (mAuxBox->GetLabel() == "Train Controls") {
        cout << mAuxBox->GetLabel() << endl;
    } else {
        cerr << "Segment2dAuxControls::~Segment2dAuxControls: "
             << "Can't save unrecognized controls: " << mAuxBox->GetLabel()
             << endl;
    }
    //SaveValue( mode, w->modeName[canvas->detection_mode] );
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dAuxControls::init ( ) {
    mAlphaCtrl = nullptr;
    mAuxBox = nullptr;
    mAuxSizer = nullptr;
    mBetaCtrl = nullptr;
    mBottomSizer = nullptr;
    mBrushSize = nullptr;
    mCp = nullptr;
    mFeatureDisplay = nullptr;
    mFeatureMax = nullptr;
    mFeatureMin = nullptr;
    mFeature = nullptr;
    mFeatureStatus = nullptr;
    mFgs = nullptr;
    mFgsButton = nullptr;
    mFgsSliders = nullptr;
    mGammaCtrl = nullptr;
    mIterates = nullptr;
    mMean = nullptr;
    mMinPointsCtrl = nullptr;
    mSt1 = nullptr;
    mSt2 = nullptr;
    mSt3 = nullptr;
    mSt4 = nullptr;
    mStdDev = nullptr;
    mTransform = nullptr;
    mTransformSelect = nullptr;
    mUpdate = nullptr;
    mWeight = nullptr;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dAuxControls::setup_fgs ( wxPanel* cp, wxSizer* bottomSizer,
                                       const char * const title, int cols )
{
    mCp = cp;
    mBottomSizer = bottomSizer;
    mAuxBox = new wxStaticBox( cp, -1, title );
    ::setBoxColor( mAuxBox );
    mAuxSizer = new wxStaticBoxSizer( mAuxBox, wxHORIZONTAL );
    mFgs = new wxFlexGridSizer( 1, 0, 5 );  //1 col,vgap,hgap
    mFgs->SetMinSize( controlsWidth, 0 );
    mFgs->AddGrowableCol( 0 );
    mFgsButton = new wxFlexGridSizer( cols, 1, 1 );  // cols,vgap,hgap
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dAuxControls::display_fgs ( ) {
    if (mFgsSliders)
        mFgs->Add( mFgsSliders, 0, wxGROW|wxALL, 10 );
    mFgs->Add( mFgsButton, 0, wxGROW|wxALL, 10 );
    mAuxSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Prepend( mAuxSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    mCp->Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** used by feature controls */
void Segment2dAuxControls::SetTransform ( int newTransform, int featureRange,
                  double newFeatureMean, double newFeatureStdDev,
                  double newFeatureMin, double newFeatureMax )
{
    //ditto on interesting use of comma operator (to make two statements into one)!
    if (mStdDev)
        delete mStdDev, mStdDev = nullptr;
    if (mMean)
        delete mMean, mMean = nullptr;
    if (mFeatureMax)
        delete mFeatureMax, mFeatureMax = nullptr;
    if (mFeatureMin)
        delete mFeatureMin, mFeatureMin = nullptr;

    switch (newTransform) {
        case 0: // Linear
        case 2: // Inv. Linear
            mFeatureMin = new Segment2dSlider( mAuxSizer->GetStaticBox(), mFgsSliders, "Feature Min.",
                                               Segment2dFrame::ID_FEATURE_MIN, 0, featureRange,
                                               newFeatureMin );
            mFeatureMax = new Segment2dSlider( mAuxSizer->GetStaticBox(), mFgsSliders, "Feature Max.",
                                               Segment2dFrame::ID_FEATURE_MAX, 0, featureRange,
                                               newFeatureMax );
            break;
        case 1: // Gaussian
        case 3: // Inv. Gaussian
        case 4: // Hyperbolic
        case 5: // Inv. Hyperbolic
            mMean = new Segment2dSlider( mAuxSizer->GetStaticBox(), mFgsSliders, "Mean",
                                         Segment2dFrame::ID_MEAN, 0, featureRange, newFeatureMean );
            mStdDev = new Segment2dSlider( mAuxSizer->GetStaticBox(), mFgsSliders, "Std. Dev.",
                                           Segment2dFrame::ID_STD_DEV, 0, featureRange,
                                           newFeatureStdDev);
            break;
        default:
            cerr << "Segment2dAuxControls::SetTransform: unrecognized value for variable, newTransform." << endl;
            assert( false );
            break;
    }
    mBottomSizer->Layout();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** used by feature controls */
void Segment2dAuxControls::SetFeature ( int newFeature, bool newFeatureStatus,
                int newFeatureTransform, int featureRange, double newFeatureWeight,
                double newFeatureMean, double newFeatureStdDev,
                double newFeatureMin, double newFeatureMax )
{
    mFeatureStatus->SetValue( newFeatureStatus );    //SetFeatureStatus(newFeatureStatus);
    //ditto on interesting use of comma operator (to make two statements into one)!
    if (mStdDev)
        delete mStdDev, mStdDev = nullptr;
    if (mMean)
        delete mMean, mMean = nullptr;
    if (mFeatureMax)
        delete mFeatureMax, mFeatureMax = nullptr;
    if (mFeatureMin)
        delete mFeatureMin, mFeatureMin = nullptr;
    if (mWeight)
        delete mWeight, mWeight = nullptr;

    mWeight = new Segment2dSlider( mAuxSizer->GetStaticBox(), mFgsSliders, "Weight",
                                   Segment2dFrame::ID_WEIGHT, 1, 100, newFeatureWeight );
    SetTransform(newFeatureTransform, featureRange, newFeatureMean,
                 newFeatureStdDev, newFeatureMin, newFeatureMax);
    mTransform->SetValue( Segment2dFrame::transformNames[newFeatureTransform] );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Segment2dAuxControls::UpdateWeight ( double newval ) { mWeight->UpdateValue(newval); }
void Segment2dAuxControls::UpdateMean ( double newval ) { mMean->UpdateValue(newval); }
void Segment2dAuxControls::UpdateStdDev ( double newval ) { mStdDev->UpdateValue(newval); }
void Segment2dAuxControls::UpdateFeatureMin ( double newval ) { mFeatureMin->UpdateValue(newval); }
void Segment2dAuxControls::UpdateFeatureMax ( double newval ) { mFeatureMax->UpdateValue(newval); }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

