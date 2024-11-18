/*
  Copyright 1993-2015, 2017, 2023 Medical Image Processing Group
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
 * \file   PreferencesDialog.h
 * \brief  Definition PreferencesDialog class.
 * \author George J. Grevera, Ph.D.
 *
 * This dialog allows the user to modify application preferences/settings.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
#pragma once
//#ifndef __PreferencesDialog_h
//#define __PreferencesDialog_h

#if wxUSE_SPINCTRL
    #include  "wx/spinctrl.h"
#endif
#include  "wx/propdlg.h"

extern wxLogWindow *gLogWindow;

/** \brief Definition and implementation of PreferencesDialog class.
 *
 * This dialog allows the user to modify application preferences/settings.
 * Note:  Any and all changes will immediately be saved via the Preferences
 * singleton.
 * <pre>
 * Usage:  PreferencesDialog  dialog(this);
 *         dialog.ShowModal();
 * </pre>
 */
class PreferencesDialog: public wxPropertySheetDialog {
    DECLARE_CLASS( PreferencesDialog )
protected:
    bool           mCustomAppearance;
    wxTextCtrl*    mHome;
    wxTextCtrl*    mInputDirectory;
    wxTextCtrl*    mOutputDirectory;
    wxTextCtrl*    mSaveScreenFileName;
    bool           mShowLog;
    bool           mShowSaveScreen;
    bool           mShowToolTips;
    bool           mSingleFrameMode;
    bool           mDejaVuMode;
    bool           mUseInputHistory;

    wxStaticText*  mFgSt;
    wxTextCtrl*    mFgRed;
    wxTextCtrl*    mFgGreen;
    wxTextCtrl*    mFgBlue;
    wxButton*      mFgB;

    wxStaticText*  mBgSt;
    wxTextCtrl*    mBgRed;
    wxTextCtrl*    mBgGreen;
    wxTextCtrl*    mBgBlue;
    wxButton*      mBgB;

    wxStaticText*  mCTLungSt;
    wxTextCtrl*    mCTLungCenter;
    wxTextCtrl*    mCTLungWidth;

    wxStaticText*  mCTSoftTissueSt;
    wxTextCtrl*    mCTSoftTissueCenter;
    wxTextCtrl*    mCTSoftTissueWidth;

    wxStaticText*  mCTBoneSt;
    wxTextCtrl*    mCTBoneCenter;
    wxTextCtrl*    mCTBoneWidth;

    wxStaticText*  mPETSt;
    wxTextCtrl*    mPETCenter;
    wxTextCtrl*    mPETWidth;

	wxStaticText*  mOvrlScaleSt;
	wxTextCtrl*    mOverlayScale;

    wxStaticText*  mIM0onIM0St;
    wxTextCtrl*    mIM0onIM0Red;
    wxTextCtrl*    mIM0onIM0Green;
    wxTextCtrl*    mIM0onIM0Blue;

    wxStaticText*  mBIMonIM0St;
    wxTextCtrl*    mBIMonIM0Red;
    wxTextCtrl*    mBIMonIM0Green;
    wxTextCtrl*    mBIMonIM0Blue;

    wxStaticText*  mBIMonBIMSt;
    wxTextCtrl*    mBIMonBIMRed;
    wxTextCtrl*    mBIMonBIMGreen;
    wxTextCtrl*    mBIMonBIMBlue;

    bool           mParallelMode;
    wxGrid*        mSystemListCtrl;
    wxTextCtrl*    mMPIDirectory;
    wxButton*      mTest;

    int            mStereoMode;
    wxStaticText*  mStereoAngleSt;
    wxTextCtrl*    mStereoAngle;
    wxTextCtrl*    mStereoLeftRed;
    wxTextCtrl*    mStereoLeftGreen;
    wxTextCtrl*    mStereoLeftBlue;
    wxTextCtrl*    mStereoRightRed;
    wxTextCtrl*    mStereoRightGreen;
    wxTextCtrl*    mStereoRightBlue;
    bool           mStereoLeftOdd;
    wxCheckBox*    mStereoLeftOddCb;
public:
    explicit PreferencesDialog ( wxWindow* parent );
    //~PreferencesDialog ( void )  {  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    wxPanel* CreateGeneralSettingsPage     ( wxWindow* parent ) const;
    wxPanel* CreateDirectoriesSettingsPage ( wxWindow* parent );
    wxPanel* CreateAppearanceSettingsPage  ( wxWindow* parent );
    wxPanel* CreateCTWindowSettingsPage    ( wxWindow* parent );
    wxPanel* CreateOverlaySettingsPage     ( wxWindow* parent );
    wxPanel* CreateStereoSettingsPage      ( wxWindow* parent );

    void OnChooseFg    ( wxCommandEvent& unused );
    void OnChooseBg    ( wxCommandEvent& unused );
    void OnDefault     ( wxCommandEvent& unused );

	void OnCTDefault   ( wxCommandEvent& unused );

	//void OnOverlaySettings  ( wxCommandEvent& unused );

    wxPanel* CreateParallelSettingsPage ( wxWindow* parent );

    void OnNewRow      ( wxCommandEvent& unused );
    void OnClearRows   ( wxCommandEvent& unused );
    void OnDeleteRows  ( wxCommandEvent& unused );
    void OnTest        ( wxCommandEvent& unused );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to single frame mode. */
    void OnSingleFrameMode ( wxCommandEvent& e ) {
        mSingleFrameMode = e.IsChecked();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to deja vu mode. */
    void OnDejaVuMode ( wxCommandEvent& e ) {
        mDejaVuMode = e.IsChecked();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to use input history. */
    void OnUseInputHistory ( wxCommandEvent& e ) {
        mUseInputHistory = e.IsChecked();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to show log. */
    void OnShowLog ( wxCommandEvent& e ) {
        mShowLog = e.IsChecked();
		if (mShowLog && gLogWindow)
			gLogWindow->Show();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to show savescreen. */
    void OnShowSaveScreen ( wxCommandEvent& e ) {
        mShowSaveScreen = e.IsChecked();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to show tooltips. */
    void OnShowToolTips ( wxCommandEvent& e ) {
        mShowToolTips = e.IsChecked();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to custom appearance. */
    void OnCustomAppearance ( wxCommandEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to parallel mode. */
    void OnParallelMode ( wxCommandEvent& e ) {
        mParallelMode = e.IsChecked();
        mSystemListCtrl->Enable( mParallelMode );
        mMPIDirectory->Enable( mParallelMode );
        mTest->Enable( mParallelMode );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to stereo mode off. */
    void OnStereoModeOff ( wxCommandEvent& e ) {
        mStereoMode = Preferences::StereoModeOff;
        handleStereoModeChange();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to stereo mode interlaced. */
    void OnStereoModeInterlaced ( wxCommandEvent& e ) {
        mStereoMode = Preferences::StereoModeInterlaced;
        handleStereoModeChange();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to stereo mode anaglyph. */
    void OnStereoModeAnaglyph ( wxCommandEvent& e ) {
        mStereoMode = Preferences::StereoModeAnaglyph;
        handleStereoModeChange();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief callback for changes to stereo mode anaglyph. */
    void OnStereoLeftOdd ( wxCommandEvent& e ) {
        mStereoLeftOdd = e.IsChecked();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void handleStereoModeChange ( void ) {
        mStereoAngleSt->Enable(    mStereoMode != Preferences::StereoModeOff        );
        mStereoAngle->Enable(      mStereoMode != Preferences::StereoModeOff        );

        mStereoLeftOddCb->Enable(  mStereoMode == Preferences::StereoModeInterlaced );

        mStereoLeftRed->Enable(    mStereoMode == Preferences::StereoModeAnaglyph   );
        mStereoLeftGreen->Enable(  mStereoMode == Preferences::StereoModeAnaglyph   );
        mStereoLeftBlue->Enable(   mStereoMode == Preferences::StereoModeAnaglyph   );
        mStereoRightRed->Enable(   mStereoMode == Preferences::StereoModeAnaglyph   );
        mStereoRightGreen->Enable( mStereoMode == Preferences::StereoModeAnaglyph   );
        mStereoRightBlue->Enable(  mStereoMode == Preferences::StereoModeAnaglyph   );
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnOK ( wxCommandEvent& unused );

  protected:
    enum {
        ID_CUSTOM_APPEARANCE = 100,
        ID_HOME,
        ID_SHOW_LOG,
        ID_SHOW_SAVE_SCREEN,
        ID_SHOW_TOOL_TIPS,
        ID_SINGLE_FRAME_MODE,
        ID_DEJA_VU_MODE,
        ID_USE_INPUT_HISTORY,

        ID_STEREO_MODE_OFF, ID_STEREO_ANGLE,
        ID_STEREO_MODE_INTERLACED, ID_STEREO_LEFT_ODD,
        ID_STEREO_MODE_ANAGLYPH, ID_STEREO_LEFT_RED,  ID_STEREO_LEFT_GREEN,  ID_STEREO_LEFT_BLUE,
                                 ID_STEREO_RIGHT_RED, ID_STEREO_RIGHT_GREEN, ID_STEREO_RIGHT_BLUE,

        ID_APPLY_SETTINGS_TO,
        ID_BACKGROUND_STYLE,
        ID_FONT_SIZE,

        ID_FG_RED, ID_FG_GREEN, ID_FG_BLUE, ID_CHOOSE_FG,
        ID_BG_RED, ID_BG_GREEN, ID_BG_BLUE, ID_CHOOSE_BG,
        ID_DEFAULT,

        ID_PARALLEL_MODE, ID_SYSTEM_LIST, ID_MPI_DIRECTORY,

        ID_CT_LUNG_CENTER, ID_CT_LUNG_WIDTH,
        ID_CT_SOFT_TISSUE_CENTER, ID_CT_SOFT_TISSUE_WIDTH,
        ID_CT_BONE_CENTER, ID_CT_BONE_WIDTH,
        ID_PET_CENTER, ID_PET_WIDTH,
        ID_CT_DEFAULT,

		ID_OVERLAY_SCALE,
		ID_IM0_ON_IM0_RED, ID_IM0_ON_IM0_GREEN, ID_IM0_ON_IM0_BLUE,
		ID_BIM_ON_IM0_RED, ID_BIM_ON_IM0_GREEN, ID_BIM_ON_IM0_BLUE,
		ID_BIM_ON_BIM_RED, ID_BIM_ON_BIM_GREEN, ID_BIM_ON_BIM_BLUE,

        ID_NEW_ROW, ID_CLEAR_ROWS, ID_DELETE_ROWS, ID_TEST
    };

    DECLARE_EVENT_TABLE()
};

//#endif
