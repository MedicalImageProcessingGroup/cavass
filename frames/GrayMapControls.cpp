/*
  Copyright 1993-2013, 2015 Medical Image Processing Group
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
 * \file   GrayMapControls.cpp
 * \brief  Implementation of GrayMapControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include "cavass.h"
#include "GrayMapControls.h"
#include "CButton.h"
#include "CCheckBox.h"

/** strings for preferences */
//const string GrayMapControls::levelGroupDefault  = "Persistent_Options/wxSlider/graymap.level";
//const string GrayMapControls::widthGroupDefault  = "Persistent_Options/wxSlider/graymap.width";
//const string GrayMapControls::invertGroupDefault = "Persistent_Options/wxCheckBox/graymap.invert";
/** strings for persistence */
//const string GrayMapControls::levelNameDefault  = "graymap.level";   //must match (end of) above
//const string GrayMapControls::widthNameDefault  = "graymap.width";   //ditto
//const string GrayMapControls::invertNameDefault = "graymap.invert";  //ditto

/** \brief GrayMapControls ctor.
 *  \param cp is the parent panel for these controls.
 *  \param bottomSizer is the existing sizer to which this the
 *         sizer associated with these controls will be added
 *         (and deleted upon destruction).
 *  \param title          is the desired box title.
 *  \param currentLevel   is the current level value.
 *  \param currentWidth   is the current width  value.
 *  \param max            is the max slider value.
 *  \param currentInvert  is the current invert setting for the LUT.
 *  \param levelSliderID  is the ID assigned to the level slider.
 *  \param widthSliderID  is the ID assigned to the width  slider.
 *  \param invertID       is the ID assigned to the invert check box.
 *  \param lungID         is the ID assigned to the lung button
 *  \param softTissueID   is the ID assigned to the soft tissue button
 *  \param boneID         is the ID assigned to the bone button
 *  \param min            is the min slider value (optional)
 */
#if 0
GrayMapControls::GrayMapControls ( wxPanel* cp, wxSizer* bottomSizer,
                                   const char* const title,
                                   const int currentLevel, const int currentWidth,
                                   const int max, const bool currentInvert,
                                   const int levelSliderID, const int widthSliderID, const int invertID,
                                   const int lungID, const int softTissueID, const int boneID,
                                   const int PETID, int min,
                                   const string& levelGroup,  const string& levelName,  //for persistence
                                   const string& widthGroup,  const string& widthName,
                                   const string& invertGroup, const string& invertName )
{
//     if (min>0)    min = 0;    // modified by xinjian 2008.8.15
#if 1
    mBottomSizer = bottomSizer;

    auto mContrastBox = new wxStaticBox(cp, wxID_ANY, title);
    //::setSliderBoxColor( mContrastBox );
    //::setBackgroundColor( mContrastBox );
    //::setBoxColor( mContrastBox );
    mContrastBoxSizer = new wxStaticBoxSizer(mContrastBox, wxVERTICAL);
    mContrastBoxSizer->SetMinSize(controlsWidth*3, 0);

    auto gmFGSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
    gmFGSizer->AddGrowableCol( 0 );
    gmFGSizer->SetFlexibleDirection( wxHORIZONTAL );
    gmFGSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    auto levelWidthSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
    levelWidthSizer->AddGrowableCol( 1 );
    levelWidthSizer->SetFlexibleDirection( wxHORIZONTAL );
    levelWidthSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    auto stLevel = new wxStaticText( mContrastBox, wxID_ANY, _("level"), wxDefaultPosition, wxDefaultSize, 0 );
    levelWidthSizer->Add( stLevel, 0, wxALIGN_CENTER|wxALL, 0 );

    mLevel = new wxSlider( mContrastBox, levelSliderID, currentLevel, min, max, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_MIN_MAX_LABELS|wxSL_VALUE_LABEL );
    mLevel->SetName( levelName );  //very important for persistence
    levelWidthSizer->Add( mLevel, 1, wxALL|wxEXPAND, 0 );

    auto stWidth = new wxStaticText( mContrastBox, wxID_ANY, _("width"), wxDefaultPosition, wxDefaultSize, 0 );
    levelWidthSizer->Add( stWidth, 0, wxALIGN_CENTER|wxALL, 5 );

    mWidth = new wxSlider( mContrastBox, widthSliderID, currentWidth, min, max, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_MIN_MAX_LABELS|wxSL_VALUE_LABEL );
    mWidth->SetName( widthName );  //very important for persistence
    levelWidthSizer->Add( mWidth, 1, wxALL|wxEXPAND, 0 );

    gmFGSizer->Add( levelWidthSizer, 1, wxEXPAND, 5 );

    auto presetsSizer = new wxGridSizer( 0, 2, 0, 0 );
    //add optional preset buttons
    if (lungID != wxID_ANY) {  //ct lung
        auto mLung = new CButton(mContrastBox, lungID, _("CT lung"), wxDefaultPosition, wxDefaultSize, 0);
        presetsSizer->Add(mLung, 1, wxALL|wxEXPAND, 15);
    }
    if (softTissueID != wxID_ANY) {  //ct soft tissue
        auto mSoftTissue = new CButton(mContrastBox, softTissueID, _("CT soft tissue"), wxDefaultPosition,
                                        wxDefaultSize, 0);
        presetsSizer->Add(mSoftTissue, 1, wxALL|wxEXPAND, 15);
    }
    if (boneID != wxID_ANY) {  //ct bone
        auto mBone = new CButton(mContrastBox, boneID, _("CT bone"), wxDefaultPosition, wxDefaultSize, 0);
        presetsSizer->Add(mBone, 1, wxALL|wxEXPAND, 15);
    }
    if (PETID != wxID_ANY) {  //pet
        auto mPET = new CButton(mContrastBox, PETID, _("PET"), wxDefaultPosition, wxDefaultSize, 0);
        presetsSizer->Add(mPET, 0, wxALL|wxEXPAND, 15);
    }
    if (invertID != wxID_ANY) {
        presetsSizer->Add( 0, 5, 10, wxGROW );  //spacer
        mCb = new CCheckBox( mContrastBox, invertID, _("invert on/off"), wxDefaultPosition, wxDefaultSize, 0 );
//::setColor( mCb );
        mCb->SetValue( currentInvert );
        mCb->SetName( invertName );  //very important for persistence
        presetsSizer->Add( mCb, 0, wxALL|wxEXPAND, 15 );
    }
    gmFGSizer->Add( presetsSizer, 0, wxALIGN_CENTER|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    mContrastBoxSizer->Add( gmFGSizer, 1, wxEXPAND, 5 );

    if (Preferences::getDejaVuMode()) {
        wxPersistenceManager::Get().Register( mLevel );
        wxPersistenceManager::Get().Register( mWidth );
        wxPersistenceManager::Get().Register( mCb );
    }

    //mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
    //mBottomSizer->Prepend( mContrastBoxSizer, 0, 0, 0 );
    mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Layout();
    cp->Refresh();
#endif

#if 0  //original code below
    mBottomSizer = bottomSizer;
    mContrastBox = new wxStaticBox(cp, wxID_ANY, title);
    //::setColor( mContrastBox );
    ::setSliderBoxColor( mContrastBox );

    mContrastBoxSizer = new wxStaticBoxSizer(mContrastBox, wxHORIZONTAL);
    mContrastSizer = new wxFlexGridSizer(2, 1, 1);  //cols,vgap,hgap
    mContrastSizer->SetMinSize(controlsWidth, 0);
    mContrastSizer->AddGrowableCol(0);

    //level
    mSt0 = new wxStaticText(cp, wxID_ANY, "Level:");
    ::setColor(mSt0);
    mContrastSizer->Add(mSt0, 0, wxALIGN_RIGHT);
    mLevel = new wxSlider(cp, levelSliderID, currentLevel, min, max,
                          wxDefaultPosition, wxSize(sliderWidth, -1),
                          wxSL_HORIZONTAL | wxSL_LABELS | wxSL_TOP,
                          wxDefaultValidator, "Level");
    ::setColor(mLevel);

    mLevel->SetPageSize(10);
    mContrastSizer->Add(mLevel, 0, wxGROW | wxLEFT | wxRIGHT);

    //width
    mSt1 = new wxStaticText(cp, wxID_ANY, "Width:");
    ::setColor(mSt1);
    mContrastSizer->Add(mSt1, 0, wxALIGN_RIGHT);
    int maxWidth = max - min;
    if (maxWidth < 256) maxWidth = 256;
    mWidth = new wxSlider(cp, widthSliderID, currentWidth, 1, maxWidth,
                          wxDefaultPosition, wxSize(sliderWidth, -1),
                          wxSL_HORIZONTAL | wxSL_LABELS | wxSL_TOP,
                          wxDefaultValidator, "Width");
    mWidth->SetPageSize(10);
    ::setColor(mWidth);
    mContrastSizer->Add(mWidth, 0, wxGROW | wxLEFT | wxRIGHT);
    // - - - - - - - - - -
    if (invertID != wxID_ANY) {
        mSt2 = new wxStaticText(cp, wxID_ANY, "Invert:");
        ::setColor(mSt2);
        mContrastSizer->Add(mSt2, 0, wxRIGHT);

        mCb = new CCheckBox(cp, invertID, "On/Off");
//::setColor(mCb);
        mCb->SetValue(currentInvert);
        mContrastSizer->Add(mCb, 0, wxALIGN_RIGHT, 2 * ButtonOffset);
    } else {
        mSt2 = nullptr;
        mCb = nullptr;
    }
    // - - - - - - - - - -
    if (lungID != wxID_ANY) {
        mLung = new CButton(cp, lungID, "CT Lung", wxDefaultPosition,
                             wxSize(buttonWidth, buttonHeight));
        ::setColor(mLung);
        mContrastSizer->Add(mLung, 0, wxRIGHT);
    } else {
        mLung = nullptr;
    }
    if (softTissueID != wxID_ANY) {
        mSoftTissue = new CButton(cp, softTissueID, "CT Soft Tissue",
                                   wxDefaultPosition, wxSize(buttonWidth, buttonHeight));
        ::setColor(mSoftTissue);
        mContrastSizer->Add(mSoftTissue, 0, wxRIGHT);
    } else {
        mSoftTissue = nullptr;
    }
    if (boneID != wxID_ANY) {
        mBone = new CButton( cp, boneID, "CT Bone", wxDefaultPosition,
            wxSize(buttonWidth,buttonHeight) );
        ::setColor( mBone );
        mContrastSizer->Add( mBone, 0, wxRIGHT );
    } else {
        mBone = nullptr;
    }
    if (PETID != wxID_ANY) {
        mPET = new CButton( cp, PETID, "PET", wxDefaultPosition,
            wxSize(buttonWidth,buttonHeight) );
        ::setColor( mPET );
        mContrastSizer->Add( mPET, 0, wxRIGHT );
    } else {
        mPET = nullptr;
    }

    #ifdef wxUSE_TOOLTIPS
        #ifndef __WXX11__
            if (mCb!=nullptr)    mCb->SetToolTip(  "invert values" );
            if (mSt2!=nullptr)   mSt2->SetToolTip( "invert values" );
            mLevel->SetToolTip( "level" );
            mWidth->SetToolTip(  "width"  );
        #endif
    #endif

    mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

    mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Layout();
    cp->Refresh();
#endif
}
#endif
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GrayMapControls::GrayMapControls ( wxPanel* cp, wxSizer* bottomSizer,
    const char* const title,
    const int currentLevel, const int currentWidth,
    const int max, const bool currentInvert,
    const int levelSliderID, const int widthSliderID, const int invertID,
    const int lungID, const int softTissueID, const int boneID,
    const int PETID, int min )
{
//     if (min>0)    min = 0;    // modified by xinjian 2008.8.15
#if 1
    mBottomSizer = bottomSizer;

    auto mContrastBox = new wxStaticBox(cp, wxID_ANY, title);
    //::setSliderBoxColor( mContrastBox );
    //::setBackgroundColor( mContrastBox );
    ::setBoxColor( mContrastBox );
    mContrastBoxSizer = new wxStaticBoxSizer(mContrastBox, wxVERTICAL);
    //mContrastBoxSizer->SetMinSize(controlsWidth*3, 0);
    mContrastBoxSizer->SetMinSize((int)(0.30*cp->GetSize().x), 0);

    auto gmFGSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
    gmFGSizer->AddGrowableCol( 0 );
    gmFGSizer->SetFlexibleDirection( wxHORIZONTAL );
    gmFGSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    auto levelWidthSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
    levelWidthSizer->AddGrowableCol( 1 );
    levelWidthSizer->SetFlexibleDirection( wxHORIZONTAL );
    levelWidthSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    auto stLevel = new wxStaticText( mContrastBox, wxID_ANY, _("level"), wxDefaultPosition, wxDefaultSize, 0 );
    levelWidthSizer->Add( stLevel, 0, wxALIGN_CENTER|wxALL, 0 );

    mLevel = new wxSlider( mContrastBox, levelSliderID, currentLevel, min, max, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_MIN_MAX_LABELS|wxSL_VALUE_LABEL );
    levelWidthSizer->Add( mLevel, 1, wxALL|wxEXPAND, 0 );

    auto stWidth = new wxStaticText( mContrastBox, wxID_ANY, _("width"), wxDefaultPosition, wxDefaultSize, 0 );
    levelWidthSizer->Add( stWidth, 0, wxALIGN_CENTER|wxALL, 5 );

    mWidth = new wxSlider( mContrastBox, widthSliderID, currentWidth, min, max, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_MIN_MAX_LABELS|wxSL_VALUE_LABEL );
    levelWidthSizer->Add( mWidth, 1, wxALL|wxEXPAND, 0 );

    gmFGSizer->Add( levelWidthSizer, 1, wxEXPAND, 5 );

    auto presetsSizer = new wxGridSizer( 0, 2, 0, 0 );
    //add optional preset buttons
    if (lungID != wxID_ANY) {  //ct lung
        auto mLung = new CButton(mContrastBox, lungID, _("CT lung"), wxDefaultPosition, wxDefaultSize, 0);
        presetsSizer->Add(mLung, 1, wxALL|wxEXPAND, 15);
    }
    if (softTissueID != wxID_ANY) {  //ct soft tissue
        auto mSoftTissue = new CButton(mContrastBox, softTissueID, _("CT soft tissue"), wxDefaultPosition,
                                        wxDefaultSize, 0);
        presetsSizer->Add(mSoftTissue, 1, wxALL|wxEXPAND, 15);
    }
    if (boneID != wxID_ANY) {  //ct bone
        auto mBone = new CButton(mContrastBox, boneID, _("CT bone"), wxDefaultPosition, wxDefaultSize, 0);
        presetsSizer->Add(mBone, 1, wxALL|wxEXPAND, 15);
    }
    if (PETID != wxID_ANY) {  //pet
        auto mPET = new CButton(mContrastBox, PETID, _("PET"), wxDefaultPosition, wxDefaultSize, 0);
        presetsSizer->Add(mPET, 0, wxALL|wxEXPAND, 15);
    }
    if (invertID != wxID_ANY) {
        presetsSizer->Add( 0, 5, 10, wxGROW );  //spacer
        m_cb_invert = new CCheckBox( mContrastBox, invertID, _("invert on/off"), wxDefaultPosition, wxDefaultSize, 0 );
//::setSliderBoxColor( mCb );
        m_cb_invert->SetValue( currentInvert );
        presetsSizer->Add( m_cb_invert, 0, wxALL|wxEXPAND, 15 );
    }
    gmFGSizer->Add( presetsSizer, 0, wxALIGN_CENTER|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    mContrastBoxSizer->Add( gmFGSizer, 1, wxEXPAND, 5 );

    //mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
    //mBottomSizer->Prepend( mContrastBoxSizer, 0, 0, 0 );
    mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 5 );  //was 10
    mBottomSizer->Layout();
    cp->Refresh();
#endif

#if 0  //original code below
    mBottomSizer = bottomSizer;
    mContrastBox = new wxStaticBox(cp, wxID_ANY, title);
    //::setColor( mContrastBox );
    ::setSliderBoxColor( mContrastBox );

    mContrastBoxSizer = new wxStaticBoxSizer(mContrastBox, wxHORIZONTAL);
    mContrastSizer = new wxFlexGridSizer(2, 1, 1);  //cols,vgap,hgap
    mContrastSizer->SetMinSize(controlsWidth, 0);
    mContrastSizer->AddGrowableCol(0);

    //level
    mSt0 = new wxStaticText(cp, wxID_ANY, "Level:");
    ::setColor(mSt0);
    mContrastSizer->Add(mSt0, 0, wxALIGN_RIGHT);
    mLevel = new wxSlider(cp, levelSliderID, currentLevel, min, max,
                          wxDefaultPosition, wxSize(sliderWidth, -1),
                          wxSL_HORIZONTAL | wxSL_LABELS | wxSL_TOP,
                          wxDefaultValidator, "Level");
    ::setColor(mLevel);

    mLevel->SetPageSize(10);
    mContrastSizer->Add(mLevel, 0, wxGROW | wxLEFT | wxRIGHT);

    //width
    mSt1 = new wxStaticText(cp, wxID_ANY, "Width:");
    ::setColor(mSt1);
    mContrastSizer->Add(mSt1, 0, wxALIGN_RIGHT);
    int maxWidth = max - min;
    if (maxWidth < 256) maxWidth = 256;
    mWidth = new wxSlider(cp, widthSliderID, currentWidth, 1, maxWidth,
                          wxDefaultPosition, wxSize(sliderWidth, -1),
                          wxSL_HORIZONTAL | wxSL_LABELS | wxSL_TOP,
                          wxDefaultValidator, "Width");
    mWidth->SetPageSize(10);
    ::setColor(mWidth);
    mContrastSizer->Add(mWidth, 0, wxGROW | wxLEFT | wxRIGHT);
    // - - - - - - - - - -
    if (invertID != wxID_ANY) {
        mSt2 = new wxStaticText(cp, wxID_ANY, "Invert:");
        ::setColor(mSt2);
        mContrastSizer->Add(mSt2, 0, wxRIGHT);

        mCb = new CCheckBox(cp, invertID, "On/Off");
//::setColor(mCb);
        mCb->SetValue(currentInvert);
        mContrastSizer->Add(mCb, 0, wxALIGN_RIGHT, 2 * ButtonOffset);
    } else {
        mSt2 = nullptr;
        mCb = nullptr;
    }
    // - - - - - - - - - -
    if (lungID != wxID_ANY) {
        mLung = new CButton(cp, lungID, "CT Lung", wxDefaultPosition,
                             wxSize(buttonWidth, buttonHeight));
        ::setColor(mLung);
        mContrastSizer->Add(mLung, 0, wxRIGHT);
    } else {
        mLung = nullptr;
    }
    if (softTissueID != wxID_ANY) {
        mSoftTissue = new CButton(cp, softTissueID, "CT Soft Tissue",
                                   wxDefaultPosition, wxSize(buttonWidth, buttonHeight));
        ::setColor(mSoftTissue);
        mContrastSizer->Add(mSoftTissue, 0, wxRIGHT);
    } else {
        mSoftTissue = nullptr;
    }
    if (boneID != wxID_ANY) {
        mBone = new CButton( cp, boneID, "CT Bone", wxDefaultPosition,
            wxSize(buttonWidth,buttonHeight) );
        ::setColor( mBone );
        mContrastSizer->Add( mBone, 0, wxRIGHT );
    } else {
        mBone = nullptr;
    }
    if (PETID != wxID_ANY) {
        mPET = new CButton( cp, PETID, "PET", wxDefaultPosition,
            wxSize(buttonWidth,buttonHeight) );
        ::setColor( mPET );
        mContrastSizer->Add( mPET, 0, wxRIGHT );
    } else {
        mPET = nullptr;
    }

    #ifdef wxUSE_TOOLTIPS
        #ifndef __WXX11__
            if (mCb!=nullptr)    mCb->SetToolTip(  "invert values" );
            if (mSt2!=nullptr)   mSt2->SetToolTip( "invert values" );
            mLevel->SetToolTip( "level" );
            mWidth->SetToolTip(  "width"  );
        #endif
    #endif

    mContrastBoxSizer->Add( mContrastSizer, 0, wxGROW|wxALL, 10 );

    mBottomSizer->Prepend( mContrastBoxSizer, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Layout();
    cp->Refresh();
#endif
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GrayMapControls::update_sliders ( int newLevel, int newWidth, bool invert ) {
    mLevel->SetValue(newLevel);
    mWidth->SetValue(newWidth);
    if (m_cb_invert)    m_cb_invert->SetValue( invert );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief GrayMapControls dtor. */
GrayMapControls::~GrayMapControls ( ) {
    //mContrastSizer->Clear( true );
    mContrastBoxSizer->Clear( true );
    //mContrastBoxSizer->Remove( mContrastSizer );
    mBottomSizer->Remove( mContrastBoxSizer );
}
