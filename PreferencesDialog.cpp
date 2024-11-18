/*
  Copyright 1993-2014, 2017, 2020, 2023 Medical Image Processing Group
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

/**
 * \file  PreferencesDialog.cpp
 * \brief Implementation of PreferencesDialog class.
 *
 * This dialog allows the user to modify application preferences/settings.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
#include  "cavass.h"
#include  "PreferencesDialog.h"
#include  "wx/colordlg.h"
#include  "wx/dirdlg.h"
#include  "wx/radiobut.h"

#if wxVERSION_NUMBER >= 3100
    #define WXC_FROM_DIP(x) wxWindow::FromDIP(x, NULL)
#else
    #define WXC_FROM_DIP(x) x
#endif
//----------------------------------------------------------------------
/** \brief class ctor.
*/
PreferencesDialog::PreferencesDialog ( wxWindow* parent ) {
    mCustomAppearance   = Preferences::getCustomAppearance();
    mHome               = nullptr;
    mInputDirectory     = nullptr;
    mOutputDirectory    = nullptr;
    mParallelMode       = Preferences::getParallelMode();
    mSaveScreenFileName = nullptr;
    mShowLog            = Preferences::getShowLog();
    mShowSaveScreen     = Preferences::getShowSaveScreen();
    mShowToolTips       = Preferences::getShowToolTips();
    mSingleFrameMode    = Preferences::getSingleFrameMode();
    mDejaVuMode         = Preferences::getDejaVuMode();
    mUseInputHistory    = Preferences::getUseInputHistory();
    mStereoMode         = Preferences::getStereoMode();
    mStereoLeftOdd      = false;
    mSystemListCtrl     = nullptr;
    mMPIDirectory       = nullptr;
    mTest               = nullptr;

    //SetExtraStyle( wxDIALOG_EX_CONTEXTHELP|wxWS_EX_VALIDATE_RECURSIVELY );

    Create( parent, wxID_ANY, "Preferences", wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE
#ifndef __WXWINCE__
        | wxRESIZE_BORDER
#endif
        );
    //CreateButtons( wxOK );

    wxBookCtrlBase*  notebook = GetBookCtrl();

    wxPanel*  generalSettings     = CreateGeneralSettingsPage(     notebook );
    wxPanel*  directoriesSettings = CreateDirectoriesSettingsPage( notebook );
    wxPanel*  appearanceSettings  = CreateAppearanceSettingsPage(  notebook );
#ifdef  PARALLEL
    wxPanel*  parallelSettings    = CreateParallelSettingsPage(    notebook );
#endif
    wxPanel*  stereoSettings      = CreateStereoSettingsPage(      notebook );
	wxPanel*  CTWindowSettings    = CreateCTWindowSettingsPage(    notebook );
    wxPanel*  OverlaySettings     = CreateOverlaySettingsPage(     notebook );

    notebook->AddPage( generalSettings,     _("General")     );
    notebook->AddPage( directoriesSettings, _("Directories") );
    notebook->AddPage( appearanceSettings,  _("Appearance")  );
#ifdef  PARALLEL
    notebook->AddPage( parallelSettings,    _("Parallel")    );
#endif
    notebook->AddPage( stereoSettings,      _("Stereo")      );
	notebook->AddPage( CTWindowSettings,    _("CT Windows")  );
    notebook->AddPage( OverlaySettings,     _("Overlay")     );

    //LayoutDialog();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief first page of dialog.
 */
wxPanel* PreferencesDialog::CreateGeneralSettingsPage ( wxWindow* parent ) const {
    const int border = 15;
    auto panel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(parent, wxSize(-1,-1)), wxTAB_TRAVERSAL );
    auto sizer = new wxBoxSizer( wxVERTICAL );
    panel->SetSizer( sizer );

    //single frame mode
    auto cb = new wxCheckBox( panel, ID_SINGLE_FRAME_MODE, _("&single frame mode"), wxDefaultPosition, wxDefaultSize );
    cb->SetValue( mSingleFrameMode );
    sizer->Add( cb, 0, wxALL, WXC_FROM_DIP(border) );

    //deja vu mode
    cb = new wxCheckBox( panel, ID_DEJA_VU_MODE, _("&deja vu mode"), wxDefaultPosition, wxDefaultSize );
    cb->SetValue( mDejaVuMode );
    sizer->Add( cb, 0, wxALL, WXC_FROM_DIP(border) );

    //use input history
    cb = new wxCheckBox( panel, ID_USE_INPUT_HISTORY, _("&use input history"), wxDefaultPosition, wxDefaultSize );
    cb->SetValue( mUseInputHistory );
    sizer->Add( cb, 0, wxALL, WXC_FROM_DIP(border) );

    //show savescreen
    cb = new wxCheckBox( panel, ID_SHOW_SAVE_SCREEN, _("show &save screen"), wxDefaultPosition, wxDefaultSize );
    cb->SetValue( mShowSaveScreen );
    sizer->Add( cb, 0, wxALL, WXC_FROM_DIP(border) );

    //show information log
    cb = new wxCheckBox( panel, ID_SHOW_LOG, _("show information &log"), wxDefaultPosition, wxDefaultSize );
    cb->SetValue( mShowLog );
    sizer->Add( cb, 0, wxALL, WXC_FROM_DIP(border) );

    //show tooltips
    cb = new wxCheckBox( panel, ID_SHOW_TOOL_TIPS, _("show &tooltips"), wxDefaultPosition, wxDefaultSize );
    cb->SetValue( mShowToolTips );
    sizer->Add( cb, 0, wxALL, WXC_FROM_DIP(border) );

    sizer->Fit( panel );
    return panel;
}
//----------------------------------------------------------------------
/** \brief create the appearance settings page. */
wxPanel* PreferencesDialog::CreateAppearanceSettingsPage ( wxWindow* parent )
{
    wxPanel*       panel    = new wxPanel( parent, wxID_ANY );
    wxBoxSizer*    topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    item     = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    bs       = NULL;
    wxCheckBox*    cb       = NULL;
    wxString       tmp;

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //custom appearance on/off
    bs = new wxBoxSizer( wxHORIZONTAL );
    cb = new wxCheckBox( panel, ID_CUSTOM_APPEARANCE, _("&custom appearance") );
    cb->SetValue( mCustomAppearance );
    bs->Add( cb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //foreground color
    bs = new wxBoxSizer( wxHORIZONTAL );

    mFgSt = new wxStaticText( panel, wxID_ANY, "foreground color (RGB):" );
    bs->Add( mFgSt, 2, wxALIGN_CENTER_VERTICAL );

    tmp = wxString::Format( "%d", Preferences::getFgRed() );
    mFgRed = new wxTextCtrl( panel, ID_FG_RED, tmp, wxDefaultPosition, wxSize(50,-1), wxTEXT_ALIGNMENT_RIGHT );
    bs->Add( mFgRed, 1 );

    tmp = wxString::Format( "%d", Preferences::getFgGreen() );
    mFgGreen = new wxTextCtrl( panel, ID_FG_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mFgGreen, 1 );

    tmp = wxString::Format( "%d", Preferences::getFgBlue() );
    mFgBlue = new wxTextCtrl( panel, ID_FG_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mFgBlue, 1 );

    bs->Add( 10, 10, 0 );

    mFgB = new wxButton( panel, ID_CHOOSE_FG, "Choose", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    bs->Add( mFgB, 1 );

    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

    //background color
    bs = new wxBoxSizer( wxHORIZONTAL );
    mBgSt = new wxStaticText( panel, wxID_ANY, "background color (RGB):" );
    bs->Add( mBgSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getBgRed() );
    mBgRed = new wxTextCtrl( panel, ID_BG_RED, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mBgRed, 1 );
    tmp = wxString::Format( "%d", Preferences::getBgGreen() );
    mBgGreen = new wxTextCtrl( panel, ID_BG_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mBgGreen, 1 );
    tmp = wxString::Format( "%d", Preferences::getBgBlue() );
    mBgBlue = new wxTextCtrl( panel, ID_BG_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mBgBlue, 1 );
    bs->Add( 10, 10, 0 );
    mBgB = new wxButton( panel, ID_CHOOSE_BG, "Choose", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    bs->Add( mBgB, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 5 );
    bs->Add( 10, 10, 0 );
    wxButton*  dflt = new wxButton( panel, ID_DEFAULT, "Default", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    bs->Add( dflt, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //reminder message
    bs = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText*  st = new wxStaticText( panel, wxID_ANY, "Note: Any changes will affect new windows and upon restart." );
    bs->Add( st, wxALIGN_CENTER_HORIZONTAL );
    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //finish up
    topSizer->Add( item );
    panel->SetSizer( topSizer );
    topSizer->Fit( panel );

    if (mCustomAppearance)  return panel;
    mFgSt->Disable();
    mFgRed->Disable();
    mFgGreen->Disable();
    mFgBlue->Disable();
    mFgB->Disable();

    mBgSt->Disable();
    mBgRed->Disable();
    mBgGreen->Disable();
    mBgBlue->Disable();
    mBgB->Disable();

    return panel;
}
//----------------------------------------------------------------------
/** \brief create the CT window settings page. */
wxPanel* PreferencesDialog::CreateCTWindowSettingsPage ( wxWindow* parent )
{
    wxPanel*       panel    = new wxPanel( parent, wxID_ANY );
    wxBoxSizer*    topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    item     = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    bs       = NULL;
    wxString       tmp;

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //lung
    bs = new wxBoxSizer( wxHORIZONTAL );

    mCTLungSt = new wxStaticText( panel, wxID_ANY, "lung (center - width):" );
    bs->Add( mCTLungSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getCTLungCenter() );
    mCTLungCenter = new wxTextCtrl( panel, ID_CT_LUNG_CENTER, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mCTLungCenter, 1 );
    tmp = wxString::Format( "%d", Preferences::getCTLungWidth() );
    mCTLungWidth = new wxTextCtrl( panel, ID_CT_LUNG_WIDTH, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mCTLungWidth, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

    //soft tissue
    bs = new wxBoxSizer( wxHORIZONTAL );
    mCTSoftTissueSt = new wxStaticText( panel, wxID_ANY, "soft tissue (center - width):" );
    bs->Add( mCTSoftTissueSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getCTSoftTissueCenter() );
    mCTSoftTissueCenter = new wxTextCtrl( panel, ID_CT_SOFT_TISSUE_CENTER, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mCTSoftTissueCenter, 1 );
    tmp = wxString::Format( "%d", Preferences::getCTSoftTissueWidth() );
    mCTSoftTissueWidth = new wxTextCtrl( panel, ID_CT_SOFT_TISSUE_WIDTH, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mCTSoftTissueWidth, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

    //bone
    bs = new wxBoxSizer( wxHORIZONTAL );

    mCTBoneSt = new wxStaticText( panel, wxID_ANY, "bone (center - width):" );
    bs->Add( mCTBoneSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getCTBoneCenter() );
    mCTBoneCenter = new wxTextCtrl( panel, ID_CT_BONE_CENTER, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mCTBoneCenter, 1 );
    tmp = wxString::Format( "%d", Preferences::getCTBoneWidth() );
    mCTBoneWidth = new wxTextCtrl( panel, ID_CT_BONE_WIDTH, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mCTBoneWidth, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

    //PET
    bs = new wxBoxSizer( wxHORIZONTAL );

    mPETSt = new wxStaticText( panel, wxID_ANY, "PET (center - width):" );
    bs->Add( mPETSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getPETCenter() );
    mPETCenter = new wxTextCtrl( panel, ID_PET_CENTER, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mPETCenter, 1 );
    tmp = wxString::Format( "%d", Preferences::getPETWidth() );
    mPETWidth = new wxTextCtrl( panel, ID_PET_WIDTH, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mPETWidth, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 5 );
    bs->Add( 10, 10, 0 );
    wxButton*  dflt = new wxButton( panel, ID_CT_DEFAULT, "Default", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    bs->Add( dflt, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //reminder message
    bs = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText*  st = new wxStaticText( panel, wxID_ANY, "Note: Any changes will affect new windows and upon restart." );
    bs->Add( st, wxALIGN_CENTER_HORIZONTAL );
    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //finish up
    topSizer->Add( item );
    panel->SetSizer( topSizer );
    topSizer->Fit( panel );

    return panel;
}
//----------------------------------------------------------------------
/** \brief create the overlay settings page. */
wxPanel* PreferencesDialog::CreateOverlaySettingsPage ( wxWindow* parent )
{
    wxPanel*       panel    = new wxPanel( parent, wxID_ANY );
    wxBoxSizer*    topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    item     = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    bs       = NULL;
    wxString       tmp;

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //scale
    bs = new wxBoxSizer( wxHORIZONTAL );

    mOvrlScaleSt = new wxStaticText( panel, wxID_ANY, "scale (pixel replication factor):" );
    bs->Add( mOvrlScaleSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%.1f", Preferences::getOverlayScale() );
    mOverlayScale = new wxTextCtrl( panel, ID_OVERLAY_SCALE, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mOverlayScale, 1 );
    bs->Add( 10, 10, 0 );

    item->Add( bs, 1 );

	//color
	bs = new wxBoxSizer( wxHORIZONTAL );

	mIM0onIM0St = new wxStaticText( panel, wxID_ANY, "IM0 on IM0: (RGB)" );
	bs->Add( mIM0onIM0St, 2, wxALIGN_CENTER_VERTICAL );
	tmp = wxString::Format( "%d", Preferences::getIM0onIM0Red() );
	mIM0onIM0Red = new wxTextCtrl( panel, ID_IM0_ON_IM0_RED, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mIM0onIM0Red, 1 );
	tmp = wxString::Format( "%d", Preferences::getIM0onIM0Green() );
	mIM0onIM0Green = new wxTextCtrl( panel, ID_IM0_ON_IM0_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mIM0onIM0Green, 1 );
	tmp = wxString::Format( "%d", Preferences::getIM0onIM0Blue() );
	mIM0onIM0Blue = new wxTextCtrl( panel, ID_IM0_ON_IM0_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mIM0onIM0Blue, 1 );
	bs->Add( 10, 10, 0 );

	item->Add( bs, 1 );

	bs = new wxBoxSizer( wxHORIZONTAL );

	mBIMonIM0St = new wxStaticText( panel, wxID_ANY, "BIM on IM0: (RGB)" );
	bs->Add( mBIMonIM0St, 2, wxALIGN_CENTER_VERTICAL );
	tmp = wxString::Format( "%d", Preferences::getBIMonIM0Red() );
	mBIMonIM0Red = new wxTextCtrl( panel, ID_BIM_ON_IM0_RED, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mBIMonIM0Red, 1 );
	tmp = wxString::Format( "%d", Preferences::getBIMonIM0Green() );
	mBIMonIM0Green = new wxTextCtrl( panel, ID_BIM_ON_IM0_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mBIMonIM0Green, 1 );
	tmp = wxString::Format( "%d", Preferences::getBIMonIM0Blue() );
	mBIMonIM0Blue = new wxTextCtrl( panel, ID_BIM_ON_IM0_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mBIMonIM0Blue, 1 );
	bs->Add( 10, 10, 0 );

	item->Add( bs, 1 );

	bs = new wxBoxSizer( wxHORIZONTAL );

	mBIMonBIMSt = new wxStaticText( panel, wxID_ANY, "BIM on BIM: (RGB)" );
	bs->Add( mBIMonBIMSt, 2, wxALIGN_CENTER_VERTICAL );
	tmp = wxString::Format( "%d", Preferences::getBIMonBIMRed() );
	mBIMonBIMRed = new wxTextCtrl( panel, ID_BIM_ON_BIM_RED, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mBIMonBIMRed, 1 );
	tmp = wxString::Format( "%d", Preferences::getBIMonBIMGreen() );
	mBIMonBIMGreen = new wxTextCtrl( panel, ID_BIM_ON_BIM_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mBIMonBIMGreen, 1 );
	tmp = wxString::Format( "%d", Preferences::getBIMonBIMBlue() );
	mBIMonBIMBlue = new wxTextCtrl( panel, ID_BIM_ON_BIM_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
	bs->Add( mBIMonBIMBlue, 1 );
	bs->Add( 10, 10, 0 );

	item->Add( bs, 1 );

    //reminder message
    bs = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText*  st = new wxStaticText( panel, wxID_ANY, "Note: Any changes will affect new windows and upon restart." );
    bs->Add( st, wxALIGN_CENTER_HORIZONTAL );
    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //finish up
    topSizer->Add( item );
    panel->SetSizer( topSizer );
    topSizer->Fit( panel );

    return panel;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for changes to custom appearance checkbox. */
void PreferencesDialog::OnCustomAppearance ( wxCommandEvent& e ) {
    mCustomAppearance = e.IsChecked();
    if (mCustomAppearance) {
        mFgSt->Enable();
        mFgRed->Enable();
        mFgGreen->Enable();
        mFgBlue->Enable();
        mFgB->Enable();

        mBgSt->Enable();
        mBgRed->Enable();
        mBgGreen->Enable();
        mBgBlue->Enable();
        mBgB->Enable();
    } else {
        mFgSt->Disable();
        mFgRed->Disable();
        mFgGreen->Disable();
        mFgBlue->Disable();
        mFgB->Disable();

        mBgSt->Disable();
        mBgRed->Disable();
        mBgGreen->Disable();
        mBgBlue->Disable();
        mBgB->Disable();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for choose foreground color button press. */
void PreferencesDialog::OnChooseFg ( wxCommandEvent& unused ) {
    wxColourData  data;
    data.SetChooseFull( true );
    wxColour  c( Preferences::getFgRed(), Preferences::getFgGreen(),
                 Preferences::getFgBlue() );
    data.SetCustomColour( 0, c );
    wxColourDialog  dialog(this, &data);
    if (dialog.ShowModal() != wxID_OK)  return;
    wxColourData  retData = dialog.GetColourData();
    wxColour  clr   = retData.GetColour();
    wxString      tmp;
    //Note: I have to undef Red and Green using Red() and Green().
#ifdef Red
    #undef Red
#endif
#ifdef Green
    #undef Green
#endif
    tmp = wxString::Format( "%d", clr.Red() );
    mFgRed->SetValue( tmp );
    tmp = wxString::Format( "%d", clr.Green() );
    mFgGreen->SetValue( tmp );
    tmp = wxString::Format( "%d", clr.Blue() );
    mFgBlue->SetValue( tmp );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for choose background color button press. */
void PreferencesDialog::OnChooseBg ( wxCommandEvent& unused ) {
    wxColourData  data;
    data.SetChooseFull( true );
    wxColour  c( Preferences::getBgRed(), Preferences::getBgGreen(),
                 Preferences::getBgBlue() );
    data.SetCustomColour( 0, c );
    wxColourDialog  dialog(this, &data);
    if (dialog.ShowModal() != wxID_OK)  return;
    wxColourData  retData = dialog.GetColourData();
    wxColour  clr = retData.GetColour();
    wxString  tmp;
    //Note: I have to undef Red and Green using Red() and Green().
    tmp = wxString::Format( "%d", clr.Red() );
    mBgRed->SetValue( tmp );
    tmp = wxString::Format( "%d", clr.Green() );
    mBgGreen->SetValue( tmp );
    tmp = wxString::Format( "%d", clr.Blue() );
    mBgBlue->SetValue( tmp );
}
//----------------------------------------------------------------------
void PreferencesDialog::OnDefault ( wxCommandEvent& unused ) {
    mFgRed->SetValue(   "255" );
    mFgGreen->SetValue( "255" );
    mFgBlue->SetValue(  "167" );

    mBgRed->SetValue(    "69" );
    mBgGreen->SetValue(  "69" );
    mBgBlue->SetValue(   "89" );
}
//----------------------------------------------------------------------
void PreferencesDialog::OnCTDefault ( wxCommandEvent& unused ) {
    mCTLungCenter->SetValue(        "550" );
    mCTLungWidth->SetValue(         "1730" );

    mCTSoftTissueCenter->SetValue( "1000" );
    mCTSoftTissueWidth->SetValue(   "500" );

	mCTBoneCenter->SetValue(       "2000" );
	mCTBoneWidth->SetValue(        "4000" );

	mPETCenter->SetValue(       "1200" );
	mPETWidth->SetValue(        "3500" );
}
//----------------------------------------------------------------------
/** \brief create the directories settings page. */
wxPanel* PreferencesDialog::CreateDirectoriesSettingsPage ( wxWindow* parent )
{
#if 0
    wxPanel*       panel    = new wxPanel( parent, wxID_ANY );
    wxBoxSizer*    topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    item     = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*    bs = NULL;
    wxStaticText*  st = NULL;

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //CAVASS home directory
    bs = new wxBoxSizer( wxHORIZONTAL );
    st = new wxStaticText( panel, wxID_ANY, "CAVASS home directory:", wxDefaultPosition, wxSize(160,-1) );
    bs->Add( st, 0, wxALIGN_CENTER_VERTICAL );
    mHome = new wxTextCtrl( panel, ID_HOME, Preferences::getHome(),
        wxDefaultPosition, wxSize(400,-1) );
    bs->Add( mHome, 1 );
    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //input directory
    bs = new wxBoxSizer( wxHORIZONTAL );
    st = new wxStaticText( panel, wxID_ANY, "input directory:", wxDefaultPosition, wxSize(160,-1) );
    bs->Add( st, 0, wxALIGN_CENTER_VERTICAL );
    mInputDirectory = new wxTextCtrl( panel, ID_HOME,
        Preferences::getInputDirectory(), wxDefaultPosition, wxSize(300,-1) );
    bs->Add( mInputDirectory, 1 );
    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //output directory
    bs = new wxBoxSizer( wxHORIZONTAL );
    st = new wxStaticText( panel, wxID_ANY, "output directory:", wxDefaultPosition, wxSize(160,-1) );
    bs->Add( st, 0, wxALIGN_CENTER_VERTICAL );
    mOutputDirectory = new wxTextCtrl( panel, ID_HOME,
        Preferences::getOutputDirectory(), wxDefaultPosition, wxSize(300,-1) );
    bs->Add( mOutputDirectory, 1 );
    item->Add( bs, 1 );

    //topSizer->Add( item, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
    topSizer->Add( item );
    //topSizer->AddSpacer(5);
    panel->SetSizer( topSizer );
    topSizer->Fit( panel );
    return panel;
#endif
    auto panel = new wxPanel( parent, wxID_ANY );
    auto sizer = new wxFlexGridSizer(0, 2, 15, 15);
    sizer->SetFlexibleDirection( wxBOTH );
    sizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    panel->SetSizer( sizer );

    //CAVASS home directory
    auto st = new wxStaticText( panel, wxID_ANY, "CAVASS home directory:", wxDefaultPosition, wxSize(160,-1) );
    sizer->Add( st );
    mHome = new wxTextCtrl( panel, ID_HOME, Preferences::getHome(),wxDefaultPosition, wxSize(400,-1) );
    sizer->Add( mHome );

    //input directory
    st = new wxStaticText( panel, wxID_ANY, "input directory:", wxDefaultPosition, wxSize(160,-1) );
    sizer->Add( st );
    mInputDirectory = new wxTextCtrl( panel, ID_HOME, Preferences::getInputDirectory(), wxDefaultPosition, wxSize(300,-1) );
    sizer->Add( mInputDirectory );

    //output directory
    st = new wxStaticText( panel, wxID_ANY, "output directory:", wxDefaultPosition, wxSize(160,-1) );
    sizer->Add( st );
    mOutputDirectory = new wxTextCtrl( panel, ID_HOME, Preferences::getOutputDirectory(), wxDefaultPosition, wxSize(300,-1) );
    sizer->Add( mOutputDirectory );

    return panel;
}
//----------------------------------------------------------------------
/** \brief create the paallel settings page. */
wxPanel* PreferencesDialog::CreateParallelSettingsPage ( wxWindow* parent )
{
    wxPanel*     panel    = new wxPanel( parent, wxID_ANY );
    wxBoxSizer*  topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*  item     = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*  bs = NULL;
    //wxButton*    button = NULL;
    wxCheckBox*  cb = NULL;

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //parallel mode
    bs = new wxBoxSizer( wxHORIZONTAL );
    cb = new wxCheckBox( panel, ID_PARALLEL_MODE, _("&parallel mode available (on/off)") );
    cb->SetValue( mParallelMode );
    bs->Add( cb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //grid for system name & # of processes pairs
    bs = new wxBoxSizer( wxHORIZONTAL );
    const int  width = 550, height = 200;
    mSystemListCtrl = new wxGrid( panel, ID_SYSTEM_LIST, wxDefaultPosition, wxSize(width,height) );
    mSystemListCtrl->CreateGrid( Preferences::HostCount, 2 );
    mSystemListCtrl->SetColLabelValue( 0, "system name" );
    mSystemListCtrl->SetColLabelValue( 1, "# of processes" );
    mSystemListCtrl->SetColSize( 0, (int)(0.4*width) );
    mSystemListCtrl->SetColSize( 1, (int)(0.4*width) );
    //init the grid values
    for (int i=0; i<Preferences::HostCount; i++) {
        wxString  tmp = Preferences::getHostName(i);
        mSystemListCtrl->SetCellValue( i, 0, tmp );
        if (tmp.length()>0)
            mSystemListCtrl->SetCellValue( i, 1, Preferences::getHostProcessCount(i) );
    }
    mSystemListCtrl->Enable( mParallelMode );
    bs->Add( mSystemListCtrl, 0, wxALIGN_CENTER_HORIZONTAL );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //mpi directory
    bs = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText*  st = new wxStaticText( panel, wxID_ANY, "MPI directory: " );
    bs->Add( st, 0, wxALIGN_CENTER_VERTICAL );
    mMPIDirectory = new wxTextCtrl( panel, ID_MPI_DIRECTORY,
        Preferences::getMPIDirectory(), wxDefaultPosition, wxSize(300,-1) );
    mMPIDirectory->Enable( mParallelMode );
    bs->Add( mMPIDirectory, 1 );
//    item->Add( bs, 1 );

    //add some space
//    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
#if 0
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //buttons for entry deletion and to test the cluster
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 10, 10, 0 );

    wxButton*  mNewRow = new wxButton( panel, ID_NEW_ROW, "New Row", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    bs->Add( mNewRow, 1 );
    bs->Add( 10, 10, 0 );

    wxButton*  mParallelDeleteEntry = new wxButton( panel, ID_DELETE_ENTRY, "Delete Entry", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    bs->Add( mParallelDeleteEntry, 1 );
    bs->Add( 10, 10, 0 );
#endif
//    button = new wxButton( panel, ID_CLEAR_ROWS, "Clear Rows", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
//    bs->Add( button );
    mTest = new wxButton( panel, ID_TEST, "Test Cluster", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mTest->Enable( mParallelMode );
    bs->Add( mTest );

    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //topSizer->Add( item, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
    topSizer->Add( item );
    //topSizer->AddSpacer(5);
    panel->SetSizer( topSizer );
    topSizer->Fit( panel );
    return panel;
}
//----------------------------------------------------------------------
void PreferencesDialog::OnNewRow ( wxCommandEvent& unused ) {
    mSystemListCtrl->AppendRows( 1 );
}
//----------------------------------------------------------------------
static int CmpInts ( int* first, int* second ) {
    if (first<second)    return -1;
    if (first>second)    return 1;
    return 0;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PreferencesDialog::OnClearRows ( wxCommandEvent& unused ) {
    wxArrayInt  list = mSystemListCtrl->GetSelectedRows();
    if (list.GetCount()<1)    return;
    for (int i=0; i<(int)list.GetCount(); i++) {
        mSystemListCtrl->SetCellValue( list[i], 0, "" );
        mSystemListCtrl->SetCellValue( list[i], 1, "" );
    }
    mSystemListCtrl->ClearSelection();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PreferencesDialog::OnDeleteRows ( wxCommandEvent& unused ) {
    wxArrayInt  list = mSystemListCtrl->GetSelectedRows();
    if (list.GetCount()<1)    return;
    //must sort and then delete from bottom up (because rows get re-numbered as
    // we delete so if we delete from top-down, we may delete the wrong row(s).
    // example: we wish to delete 2,3, and 7.  top-down, this won't be correct.
    list.Sort( CmpInts );
    for (int i=list.GetCount()-1; i>=0; i--) {
        mSystemListCtrl->DeleteRows( list[i] );
    }
}
//----------------------------------------------------------------------
/*
  example command:
    "c:\Program Files\MPICH2\bin\mpiexec.exe" -n 3 p3dinterpolate c:/cavass-build/debug C:/data/regular.IM0 intrpl-tmp.IM0 0 0.97660 0.97660 0.97660 0 1 1 1 20
    mpiexec.exe n <# of processes> hostname
      or
    mpiexec.exe hosts <# of hosts> \
        <host1_name> <host1 # of processes> \
        <host2_name> <host2 # of processes> \
        ...
            hostname
*/
void PreferencesDialog::OnTest ( wxCommandEvent& unused ) {
#if defined (WIN32) || defined (_WIN32)
    //have to add cmd /c for windows.
    // otherwise, mpiexec won't terminate (forever)!
    // mMPIDirectory->GetValue()
    //wxString  s = "cmd /c \"\"" + Preferences::getMPIDirectory() + "/mpiexec\"";
    wxString  s = "cmd /c \"\"" + mMPIDirectory->GetValue() + "/mpiexec\"";
#else
    //wxString  s = "\"" + Preferences::getMPIDirectory() + "/mpiexec\"";
    wxString  s = "\"" + mMPIDirectory->GetValue() + "/mpiexec\"";
#endif
    s += " -genv VIEWNIX_ENV \"" + Preferences::getHome() + "\"";
    //determine # of hosts, host names, and process counts
    int  hostCount=0, processCount=0;
    wxString  hostList = "";
    for (int i=0; i<mSystemListCtrl->GetNumberRows(); i++) {
        wxString  tmp = mSystemListCtrl->GetCellValue( i, 0 );
        if (tmp.Len()<=0)    continue;
        hostList += " " + tmp;
        tmp = mSystemListCtrl->GetCellValue( i, 1 );
        hostList += " ";
        if (tmp.Len()>0) {
            hostList += tmp;
            int  x = 0;
            int  n = sscanf( (const char *)tmp.c_str(), "%d", &x );
            if (n!=1 || x<1) {  //conversion problem or bad value?
                wxString  tmp = wxString::Format( "Number of processes must be greater than 0." );
                wxMessageBox( tmp, "Bad number of processes specified!", wxICON_ERROR | wxOK );
                return;
            }
            processCount += x;
        } else {
            mSystemListCtrl->SetCellValue( i, 1, "1" );
            hostList += "1";
            ++processCount;
        }
        ++hostCount;
    }
    if (hostCount<1) {
        wxChar  buff[ 256 ];
        ::wxGetHostName( buff, sizeof buff );
        wxString  tmp = wxString("Please specify the name of at least one system.\n\n(The name of your system is ") + buff + ".)";
        wxMessageBox( tmp, "No systems specified!", wxICON_ERROR | wxOK );
        return;
    }
    if (processCount<3) {
        wxMessageBox( "Specifying a total of less than\n3 parallel processes may cause problems.",
                      "Potential problem detected.", wxICON_WARNING | wxOK );
    }
    //add -hosts # to command line
    s = wxString::Format( "%s -exitcodes -l -nopopup_debug -noprompt -hosts %d", (const char *)s.c_str(), hostCount );
    //add individual hosts and number of processes to command line
    s += hostList;
    s += " hostname\"";
    //s = "\"/Program Files/MPICH2/bin/mpiexec.exe\" -n 3 p3dinterpolate c:/cavass-build/debug C:/data/regular.IM0 intrpl-tmp.IM0 0 0.97660 0.97660 0.97660 0 1 1 1 20";
    wxLogMessage( s );
    wxLogMessage( "\nThe host name for each process should appear below \n followed by the final exist status for each process." );
    ProcessManager  p( "testing cluster", s, true );
    wxLogMessage( "End of test." );
}
//----------------------------------------------------------------------
/** \brief create the paallel settings page. */
wxPanel* PreferencesDialog::CreateStereoSettingsPage ( wxWindow* parent )
{
    wxPanel*        panel    = new wxPanel( parent, wxID_ANY );
    wxBoxSizer*     topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*     item     = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer*     bs = NULL;
    wxRadioButton*  rb = NULL;
    wxString        tmp;
    wxStaticText*   st = NULL;

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //stereo mode off
    bs = new wxBoxSizer( wxHORIZONTAL );
    rb = new wxRadioButton( panel, ID_STEREO_MODE_OFF, _("stereo mode off"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    rb->SetValue( mStereoMode==Preferences::StereoModeOff );
    bs->Add( rb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //stereo mode angle
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 10, 10, 0 );
    mStereoAngleSt = new wxStaticText( panel, wxID_ANY, "stereo angle (degrees):" );
    bs->Add( mStereoAngleSt, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%.4f", Preferences::getStereoAngle() );
    mStereoAngle = new wxTextCtrl( panel, ID_STEREO_ANGLE, tmp, wxDefaultPosition, wxSize(80,-1) );
    bs->Add( mStereoAngle, 1 );
    item->Add( bs, 1 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

	//interlaced
    bs = new wxBoxSizer( wxHORIZONTAL );
    rb = new wxRadioButton( panel, ID_STEREO_MODE_INTERLACED, _("interlaced stereo mode") );
    rb->SetValue( mStereoMode == Preferences::StereoModeInterlaced );
    bs->Add( rb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
    mStereoLeftOddCb = new wxCheckBox( panel, ID_STEREO_LEFT_ODD, _("&left is odd") );
    mStereoLeftOddCb->SetValue( mStereoLeftOdd );
    bs->Add( mStereoLeftOddCb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //anaglyph
    bs = new wxBoxSizer( wxHORIZONTAL );
    rb = new wxRadioButton( panel, ID_STEREO_MODE_ANAGLYPH, _("anaglyph stereo mode") );
    rb->SetValue( mStereoMode==Preferences::StereoModeAnaglyph );
    bs->Add( rb, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );
    //left anaglyph color
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    st = new wxStaticText( panel, wxID_ANY, " left color (RGB):" );
    bs->Add( st, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getStereoLeftRed() );
    mStereoLeftRed = new wxTextCtrl( panel, ID_STEREO_LEFT_RED, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mStereoLeftRed, 1 );
    bs->Add( 10, 10, 0 );
    tmp = wxString::Format( "%d", Preferences::getStereoLeftGreen() );
    mStereoLeftGreen = new wxTextCtrl( panel, ID_STEREO_LEFT_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mStereoLeftGreen, 1 );
    bs->Add( 10, 10, 0 );
    tmp = wxString::Format( "%d", Preferences::getStereoLeftBlue() );
    mStereoLeftBlue = new wxTextCtrl( panel, ID_STEREO_LEFT_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mStereoLeftBlue, 1 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //add some space
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 10, 10, 0 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //right anaglyph color
    bs = new wxBoxSizer( wxHORIZONTAL );
    bs->Add( 20, 20, 0 );
    st = new wxStaticText( panel, wxID_ANY, "right color (RGB):" );
    bs->Add( st, 2, wxALIGN_CENTER_VERTICAL );
    tmp = wxString::Format( "%d", Preferences::getStereoRightRed() );
    mStereoRightRed = new wxTextCtrl( panel, ID_STEREO_RIGHT_RED, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mStereoRightRed, 1 );
    bs->Add( 10, 10, 0 );
    tmp = wxString::Format( "%d", Preferences::getStereoRightGreen() );
    mStereoRightGreen = new wxTextCtrl( panel, ID_STEREO_RIGHT_GREEN, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mStereoRightGreen, 1 );
    bs->Add( 10, 10, 0 );
    tmp = wxString::Format( "%d", Preferences::getStereoRightBlue() );
    mStereoRightBlue = new wxTextCtrl( panel, ID_STEREO_RIGHT_BLUE, tmp, wxDefaultPosition, wxSize(50,-1) );
    bs->Add( mStereoRightBlue, 1 );
    item->Add( bs, 0, wxGROW|wxALL, 0 );

    //topSizer->Add( item, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
    topSizer->Add( item );
    //topSizer->AddSpacer(5);
    panel->SetSizer( topSizer );
    topSizer->Fit( panel );

    handleStereoModeChange();
    return panel;
}
//----------------------------------------------------------------------
/** \brief callback for OK button.
 *  \todo all values should be validated before _any_ are saved (and an
 *        error message should be displayed).
 */
void PreferencesDialog::OnOK ( wxCommandEvent& unused ) {
    if (!wxDir::Exists(mHome->GetValue())) {
       wxMessageBox( "Specified home directory doesn't exist.", "Sorry...",
                      wxOK | wxICON_ERROR );
       return;
    }
    SetCursor( wxCursor(wxCURSOR_WAIT) );
    Preferences::setCustomAppearance( mCustomAppearance );
    double   r, g, b;
    mFgRed->GetValue().ToDouble(   &r );
    mFgGreen->GetValue().ToDouble( &g );
    mFgBlue->GetValue().ToDouble(  &b );
    Preferences::setFgRed(   (int)r );
    Preferences::setFgGreen( (int)g );
    Preferences::setFgBlue(  (int)b );

    mBgRed->GetValue().ToDouble(   &r );
    mBgGreen->GetValue().ToDouble( &g );
    mBgBlue->GetValue().ToDouble(  &b );
    Preferences::setBgRed(   (int)r );
    Preferences::setBgGreen( (int)g );
    Preferences::setBgBlue(  (int)b );

	mCTLungCenter->GetValue().ToDouble(       &r );
	mCTSoftTissueCenter->GetValue().ToDouble( &g );
	mCTBoneCenter->GetValue().ToDouble(       &b );
	Preferences::setCTLungCenter(       (int)r );
	Preferences::setCTSoftTissueCenter( (int)g );
	Preferences::setCTBoneCenter(       (int)b );
	mPETCenter->GetValue().ToDouble(       &r );
	Preferences::setPETCenter(       (int)r );

	mCTLungWidth->GetValue().ToDouble(       &r );
	mCTSoftTissueWidth->GetValue().ToDouble( &g );
	mCTBoneWidth->GetValue().ToDouble(       &b );
	Preferences::setCTLungWidth(       (int)r );
	Preferences::setCTSoftTissueWidth( (int)g );
	Preferences::setCTBoneWidth(       (int)b );
	mPETWidth->GetValue().ToDouble(       &r );
	Preferences::setPETWidth(       (int)r );

	mOverlayScale->GetValue().ToDouble(       &r );
	Preferences::setOverlayScale( r );

	mIM0onIM0Red->GetValue().ToDouble(   &r );
	mIM0onIM0Green->GetValue().ToDouble( &g );
	mIM0onIM0Blue->GetValue().ToDouble(  &b );
	Preferences::setIM0onIM0Red(   (int)r );
	Preferences::setIM0onIM0Green( (int)g );
	Preferences::setIM0onIM0Blue(  (int)b );

	mBIMonIM0Red->GetValue().ToDouble(   &r );
	mBIMonIM0Green->GetValue().ToDouble( &g );
	mBIMonIM0Blue->GetValue().ToDouble(  &b );
	Preferences::setBIMonIM0Red(   (int)r );
	Preferences::setBIMonIM0Green( (int)g );
	Preferences::setBIMonIM0Blue(  (int)b );

	mBIMonBIMRed->GetValue().ToDouble(   &r );
	mBIMonBIMGreen->GetValue().ToDouble( &g );
	mBIMonBIMBlue->GetValue().ToDouble(  &b );
	Preferences::setBIMonBIMRed(   (int)r );
	Preferences::setBIMonBIMGreen( (int)g );
	Preferences::setBIMonBIMBlue(  (int)b );

    Preferences::setHome( mHome->GetValue() );
    //when the home directory changes, we must also change the 3dviewnix
    // environment variable
    ::modifyEnvironment();
    Preferences::setInputDirectory(       mInputDirectory->GetValue() );
#ifdef  PARALLEL
    Preferences::setMPIDirectory(         mMPIDirectory->GetValue() );
#endif
    Preferences::setOutputDirectory(      mOutputDirectory->GetValue() );
    Preferences::setParallelMode(         mParallelMode );
    //Preferences::setSaveScreenFileName( mSaveScreenFileName->GetValue() );
	Preferences::setShowLog(              mShowLog );
    Preferences::setShowSaveScreen(       mShowSaveScreen );
    Preferences::setShowToolTips(         mShowToolTips );
    Preferences::setSingleFrameMode(      mSingleFrameMode );
    Preferences::setDejaVuMode(           mDejaVuMode );
#ifdef  PARALLEL
    //save the grid values
    Preferences::clearHostNamesAndProcessCounts();
    for (int i=0; i<Preferences::HostCount; i++) {
        Preferences::setHostNameAndProcessCount( i,
            mSystemListCtrl->GetCellValue(i,0),
            mSystemListCtrl->GetCellValue(i,1) );
    }
#endif

    //save stereo mode
    Preferences::setStereoMode( mStereoMode );
    //save stereo angle
    double  d;
    mStereoAngle->GetValue().ToDouble( &d );
    Preferences::setStereoAngle( d );
    //save stereo left odd (for interlaced)
    Preferences::setStereoLeftOdd( mStereoLeftOdd );
    //save stereo left rgb (for anaglyph)
    mStereoLeftRed->GetValue().ToDouble(   &r );
    mStereoLeftGreen->GetValue().ToDouble( &g );
    mStereoLeftBlue->GetValue().ToDouble(  &b );
    Preferences::setStereoLeftRed(   (int)r );
    Preferences::setStereoLeftGreen( (int)g );
    Preferences::setStereoLeftBlue(  (int)b );
    //save stereo right rgb (for anaglyph)
    mStereoRightRed->GetValue().ToDouble(   &r );
    mStereoRightGreen->GetValue().ToDouble( &g );
    mStereoRightBlue->GetValue().ToDouble(  &b );
    Preferences::setStereoRightRed(   (int)r );
    Preferences::setStereoRightGreen( (int)g );
    Preferences::setStereoRightBlue(  (int)b );

    SetCursor( *wxSTANDARD_CURSOR );
    Close();
}
//----------------------------------------------------------------------
IMPLEMENT_CLASS(   PreferencesDialog, wxPropertySheetDialog )
BEGIN_EVENT_TABLE( PreferencesDialog, wxPropertySheetDialog )
    EVT_CHECKBOX( ID_SINGLE_FRAME_MODE, PreferencesDialog::OnSingleFrameMode )
    EVT_CHECKBOX( ID_DEJA_VU_MODE,      PreferencesDialog::OnDejaVuMode      )
    EVT_CHECKBOX( ID_SHOW_LOG,          PreferencesDialog::OnShowLog         )
    EVT_CHECKBOX( ID_SHOW_SAVE_SCREEN,  PreferencesDialog::OnShowSaveScreen  )
    EVT_CHECKBOX( ID_SHOW_TOOL_TIPS,    PreferencesDialog::OnShowToolTips    )
    EVT_CHECKBOX( ID_CUSTOM_APPEARANCE, PreferencesDialog::OnCustomAppearance)
    EVT_CHECKBOX( ID_PARALLEL_MODE,     PreferencesDialog::OnParallelMode    )
    EVT_BUTTON(   ID_NEW_ROW,           PreferencesDialog::OnNewRow          )
    EVT_BUTTON(   ID_CLEAR_ROWS,        PreferencesDialog::OnClearRows       )
    EVT_BUTTON(   ID_DELETE_ROWS,       PreferencesDialog::OnDeleteRows      )
    EVT_BUTTON(   ID_TEST,              PreferencesDialog::OnTest            )
    EVT_BUTTON(   ID_CHOOSE_FG,         PreferencesDialog::OnChooseFg        )
    EVT_BUTTON(   ID_CHOOSE_BG,         PreferencesDialog::OnChooseBg        )
    EVT_BUTTON(   ID_DEFAULT,           PreferencesDialog::OnDefault         )
	EVT_BUTTON(   ID_CT_DEFAULT,        PreferencesDialog::OnCTDefault       )
    EVT_BUTTON(   wxID_OK,              PreferencesDialog::OnOK              )
    EVT_RADIOBUTTON( ID_STEREO_MODE_OFF,        PreferencesDialog::OnStereoModeOff        )
    EVT_RADIOBUTTON( ID_STEREO_MODE_INTERLACED, PreferencesDialog::OnStereoModeInterlaced )
    EVT_RADIOBUTTON( ID_STEREO_MODE_ANAGLYPH,   PreferencesDialog::OnStereoModeAnaglyph   )
    EVT_CHECKBOX( ID_STEREO_LEFT_ODD,   PreferencesDialog::OnStereoLeftOdd   )
END_EVENT_TABLE()
//----------------------------------------------------------------------
