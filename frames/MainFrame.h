/*
  Copyright 1993-2016, 2023 Medical Image Processing Group
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
 * \file   MainFrame.h
 * \brief  Definition of MainFrame class and definition and implementation
 *         of MainSplitter.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#pragma once

#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/persist.h"
#include  "wx/splitter.h"
#include  "wx/thread.h"
#include  <cstdlib>
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif

class  MainCanvas;
class  SaveScreenControls;

/** \brief Definition of MainFrame. */
class MainFrame : public wxFrame {
  protected:
    SaveScreenControls*  mSaveScreenControls;  ///< controls to save screens
    wxSizer*             mBottomSizer;         ///< sizer at the bottom of the window for buttons and controls
  public:
    wxPersistentObject*  mPersistentMe;
    virtual string whatAmI ( ) { return "MainFrame"; }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    char*        mFileNameFilter; ///< input file name filter string
    wxMenuItem*  mHideControls;   ///< ptr to "hide controls" menu item (so it can be changed dynamically)
    wxMenu*      mFileMenu;       ///< ptr to file menu (so it can be changed dynamically)
    wxMenu*      mWindowMenu;     ///< ptr to window menu (so it can be changed dynamically)
    MainCanvas*  mCanvas;         ///< drawing area of window
    wxString     mWindowTitle;    ///< title of window
    wxString     mModuleName;     ///< name of module
    wxSplitterWindow*  mSplitter;     ///< mainly used to handle split events
    wxStaticBox* m_buttonBox;    ///< omnipresent control box at bottom-right
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief Define standard window menu and menu item ids.  We start at
     *  100 to avoid predefined ids.  We end with ID_LAST to allow apps/modules
     *  to add additional menu items (starting with ID_LAST).
     */
    enum {
      /*file*/ ID_NEW=100,  ///< avoid predefined ids
               ID_OPEN, ID_INPUT,
               ID_IMPORT, ID_IMPORT_DICOM, ID_IMPORT_EASYHEADER, ID_IMPORT_MATHEMATICA, ID_IMPORT_MATLAB,                ID_IMPORT_R, ID_IMPORT_STL, ID_IMPORT_VTK,
               ID_EXPORT, ID_EXPORT_DICOM,                       ID_EXPORT_MATHEMATICA, ID_EXPORT_MATLAB, ID_EXPORT_PGM, ID_EXPORT_R, ID_EXPORT_STL, ID_EXPORT_VTK,
               ID_SAVE_SCREEN, ID_CLOSE,
               ID_PAGE_SETUP, ID_PRINT_PREVIEW, ID_PRINT, ID_EXIT,
      /*edit*/ ID_COPY, ID_PASTE, ID_UNDO, ID_REDO, ID_ANIMATION, ID_PREFERENCES,
      /*tools*/ ID_TUTORIALS, ID_TASKS, ID_RECIPES, ID_ITK_FILTERS,
                ID_SHOW_SCREEN,
      /*preprocess*/
        /*scene operations*/ ID_PP_SCOPS,
        ID_PP_SCOPS_VOI_ROI, 	 ID_PP_SCOPS_VOI_IOI, 	ID_PP_SCOPS_VOI_PICKSLICES, 	ID_PP_SCOPS_VOI_STANDARDIZE,
          ID_PP_SCOPS_VOI, ID_PP_SCOPS_INTERPOLATE, ID_PP_SCOPS_FILTER,
          ID_PP_SCOPS_SEGMENT,  ID_PP_SCOPS_SEGMENT_THRESHOLD,
              ID_PP_SCOPS_SEGMENT_2DINTERACTIVE,
          ID_PP_SCOPS_CLASSIFY, ID_PP_SCOPS_CLASSIFY_1FEATURE,
          ID_PP_SCOPS_CLASSIFY_FUZZ_CONN, ID_PP_SCOPS_CLASSIFY_INTEN_MAP,
		  ID_PP_SCOPS_IRFC,
          ID_PP_SCOPS_ALGEBRA,  ID_PP_SCOPS_REGISTER,
        /*structure operations*/ ID_PP_STOPS,
          ID_PP_STOPS_SURFACE_NORMAL, ID_PP_STOPS_MERGE_STRUCTURES,
          ID_PP_STOPS_TO_STRUCTURE, ID_PP_STOPS_TO_SCENE,
      /*visualize*/
        /*slice*/
        ID_VIS_SLICE, ID_VIS_MONTAGE, ID_VIS_CYCLE, ID_VIS_RESLICE, ID_VIS_OVERLAY,
        /*surface*/
        ID_VIS_SURFACE, ID_VIS_SURF_VIEW, ID_SURF_MEASURE, ID_SURF_MOVIE,
        /*volume*/
        ID_VIS_VOLUME, ID_VIS_VOL_VIEW,   ID_VOL_MEASURE,  ID_VOL_MOVIE,
      /*manipulate*/
        ID_MANIPULATE, ID_MAN_SELECT, ID_MAN_MEASURE, ID_MAN_REFLECT,
        ID_MAN_CUT, ID_MAN_SEPARATE, ID_MAN_MOVE, ID_MAN_CREATE_MOVIE,
      /*analyze*/
        //scene
        ID_ANL_SCENE, ID_ANL_DENSITY, ID_ANL_ROI,
        //structure
        ID_ANL_STRUCTURE, ID_ANL_REGISTER, ID_ANL_KINEMATICS, ID_ANL_KINEMATICS_INTER, ID_ANL_KINEMATICS_INTRA,
        ID_ANL_STATIC,
      /*help*/ ID_HELP, ID_ABOUT, ID_INFORMATION, ID_EXAMPLE, ID_DOCUMENTATION,
      /*window*/  ID_WINDOW, ID_WINDOW_HIDE_CONTROLS,
        ID_FIRST_DYNAMIC_WINDOW_MENU,
        ID_LAST_DYNAMIC_WINDOW_MENU = ID_FIRST_DYNAMIC_WINDOW_MENU+200,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
        ID_LAST   ///< apps may define additional ids start with ID_LAST
    };

  protected:
    wxPanel*  mMainPanel;     ///< panel containing drawing and control areas
    wxPanel*  mControlPanel;  ///< control panel area of window
    wxMenu*   mSegmentMenu;   ///< ptr to segment menu (so it can be changed dynamically)
    wxMenu*   mClassifyMenu;  ///< ptr to segment menu (so it can be changed dynamically)
    wxMenu*   mVOIMenu;
    #define   dControlsHeight  ((buttonHeight+1)*mButtonRows+50)    ///< height of control panel (from bottom)
	int       mButtonRows;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    virtual void initializeMenu ( );
    virtual void restoreFrameSettings ( );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    MainFrame  ( );
    explicit MainFrame  ( int dummy );
    //MainFrame  ( bool dummy );  //used by persisted frames
    ~MainFrame ( ) override;
    static bool match ( wxString& filename );
    //"virtualize" a static method
    [[nodiscard]] virtual bool filenameMatch ( wxString& filename ) const {
        return match( filename );
    };
	virtual void flush_temp_data ( );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    //all of these callbacks are virtual so that they can be replaced when
    // subclassed.
    virtual void OnAbout           ( wxCommandEvent& unused );
    virtual void OnROIAnalysis     ( wxCommandEvent& unused );
    virtual void OnDensityAnalysis ( wxCommandEvent& unused );
    virtual void OnAlgebra         ( wxCommandEvent& unused );
    virtual void OnAppendScreen    ( wxCommandEvent& unused );
    virtual void OnBrowseScreen    ( wxCommandEvent& unused );
    virtual void OnClose           ( wxCommandEvent& unused );
    virtual void OnCopy            ( wxCommandEvent& unused );
    virtual void OnCycle           ( wxCommandEvent& unused );
    virtual void OnDocumentation   ( wxCommandEvent& unused );
    virtual void OnEasyHeader      ( wxCommandEvent& unused );
    virtual void OnExample         ( wxCommandEvent& unused );

    virtual void OnExportDicom       ( wxCommandEvent& unused );
    virtual void OnExportMathematica ( wxCommandEvent& unused );
    virtual void OnExportMatlab      ( wxCommandEvent& unused );
    virtual void OnExportPgm         ( wxCommandEvent& unused );
    virtual void OnExportR           ( wxCommandEvent& unused );
    virtual void OnExportStl         ( wxCommandEvent& unused );
    virtual void OnExportVtk         ( wxCommandEvent& unused );

	virtual void OnImportMathematica ( wxCommandEvent& unused );
    virtual void OnImportMatlab      ( wxCommandEvent& unused );
    virtual void OnImportR           ( wxCommandEvent& unused );
    virtual void OnImportVtk         ( wxCommandEvent& unused );

    virtual void OnFromDicom       ( wxCommandEvent& unused );
    virtual void OnHideControls    ( wxCommandEvent& unused );
    virtual void OnInformation     ( wxCommandEvent& unused );
    virtual void OnInput           ( wxCommandEvent& unused );
    virtual void OnInterpolate     ( wxCommandEvent& unused );
    virtual void OnITKFilter       ( wxCommandEvent& unused );
    virtual void OnMontage         ( wxCommandEvent& unused );
    virtual void OnNew             ( wxCommandEvent& unused );
    virtual void OnOpen            ( wxCommandEvent& unused );
    virtual void OnOverwriteScreen ( wxCommandEvent& unused );
    virtual void OnPageSetup       ( wxCommandEvent& unused );
    virtual void OnPreferences     ( wxCommandEvent& unused );
    virtual void OnPrint           ( wxCommandEvent& unused );
    virtual void OnPrintPreview    ( wxCommandEvent& unused );
    virtual void OnQuit            ( wxCommandEvent& unused );
    virtual void OnRecipes         ( wxCommandEvent& unused );
    virtual void OnRedo            ( wxCommandEvent& unused );
    virtual void OnRegister        ( wxCommandEvent& unused );
    virtual void OnSaveScreen      ( wxCommandEvent& unused );
    virtual void OnSegment2d       ( wxCommandEvent& unused );
    virtual void OnShowScreen      ( wxCommandEvent& unused );
    virtual void OnTasks           ( wxCommandEvent& unused );
    virtual void OnTutorials       ( wxCommandEvent& unused );
    virtual void OnUndo            ( wxCommandEvent& unused );

    virtual void OnPPScopsVOIROI   ( wxCommandEvent& unused );
    virtual void OnPPScopsVOIIOI   ( wxCommandEvent& unused );
    virtual void OnPPScopsVOIPickSlices ( wxCommandEvent& unused );
    virtual void OnPPScopsVOIStandardize( wxCommandEvent& unused );

    virtual void OnPPScops1Feature ( wxCommandEvent& unused );
	virtual void OnPPScopsIntenMap ( wxCommandEvent& unused );
    virtual void OnPPScopsFilter   ( wxCommandEvent& unused );
    virtual void OnPPScopsFuzzComp ( wxCommandEvent& unused );
	virtual void OnPPScopsIRFC     ( wxCommandEvent& unused );
    virtual void OnPPScopsThreshold( wxCommandEvent& unused );
	//virtual void OnPPStopsToStructure ( wxCommandEvent& unused );
	virtual void OnPPStopsToScene  ( wxCommandEvent& unused );
    virtual void OnVisOverlay      ( wxCommandEvent& unused );
    virtual void OnVisSurfView     ( wxCommandEvent& unused );
    virtual void OnVisVolView      ( wxCommandEvent& unused );
    virtual void OnManipulate      ( wxCommandEvent& unused );
    virtual void OnKinematicsInter ( wxCommandEvent& unused );
    virtual void OnKinematicsIntra ( wxCommandEvent& unused );
    virtual void OnAnalyzeStatic   ( wxCommandEvent& unused );
    virtual void OnWindow          ( wxCommandEvent& e );

    virtual void OnChar       ( wxKeyEvent&      unused );
    virtual void OnCloseEvent ( wxCloseEvent&    unused );
    virtual void OnMaximize   ( wxMaximizeEvent& unused );
    virtual void OnMove       ( wxMoveEvent&     unused );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( MainFrame )
    DECLARE_EVENT_TABLE()
};
//----------------------------------------------------------------------
/** \brief standard frame callback table entries. */
#define  DefineStandardFrameCallbacks                               \
  EVT_MENU( ID_ABOUT,                MainFrame::OnAbout        )    \
  EVT_MENU( ID_CLOSE,                MainFrame::OnClose        )    \
  EVT_MENU( ID_COPY,                 MainFrame::OnCopy         )    \
  EVT_MENU( ID_DOCUMENTATION,        MainFrame::OnDocumentation)    \
  EVT_MENU( ID_IMPORT_EASYHEADER,    MainFrame::OnEasyHeader   )    \
  EVT_MENU( ID_EXAMPLE,              MainFrame::OnExample      )    \
  EVT_MENU( ID_EXIT,                 MainFrame::OnQuit         )    \
  EVT_MENU( ID_EXPORT_DICOM,         MainFrame::OnExportDicom       )  \
  EVT_MENU( ID_EXPORT_MATHEMATICA,   MainFrame::OnExportMathematica )  \
  EVT_MENU( ID_EXPORT_MATLAB,        MainFrame::OnExportMatlab      )  \
  EVT_MENU( ID_EXPORT_PGM,           MainFrame::OnExportPgm         )  \
  EVT_MENU( ID_EXPORT_R,             MainFrame::OnExportR           )  \
  EVT_MENU( ID_EXPORT_STL,           MainFrame::OnExportStl         )  \
  EVT_MENU( ID_EXPORT_VTK,           MainFrame::OnExportVtk         )  \
  EVT_MENU( ID_IMPORT_MATHEMATICA,   MainFrame::OnImportMathematica )  \
  EVT_MENU( ID_IMPORT_MATLAB,        MainFrame::OnImportMatlab      )  \
  EVT_MENU( ID_IMPORT_R,             MainFrame::OnImportR           )  \
  EVT_MENU( ID_IMPORT_VTK,           MainFrame::OnImportVtk         )  \
  EVT_MENU( ID_IMPORT_DICOM,         MainFrame::OnFromDicom    )    \
  EVT_MENU( ID_INFORMATION,          MainFrame::OnInformation  )    \
  EVT_MENU( ID_INPUT,                MainFrame::OnInput        )    \
  EVT_MENU( ID_ITK_FILTERS,          MainFrame::OnITKFilter    )    \
  EVT_MENU( ID_NEW,                  MainFrame::OnNew          )    \
  EVT_MENU( ID_OPEN,                 MainFrame::OnOpen         )    \
  EVT_MENU( ID_PAGE_SETUP,           MainFrame::OnPageSetup    )    \
  EVT_MENU( ID_ANL_ROI,              MainFrame::OnROIAnalysis ) \
  EVT_MENU( ID_ANL_DENSITY,          MainFrame::OnDensityAnalysis ) \
  EVT_MENU( ID_ANL_STATIC,           MainFrame::OnAnalyzeStatic ) \
  EVT_MENU( ID_ANL_KINEMATICS_INTER,     MainFrame::OnKinematicsInter ) \
  EVT_MENU( ID_ANL_KINEMATICS_INTRA,     MainFrame::OnKinematicsIntra ) \
  EVT_MENU( ID_PP_SCOPS_VOI_ROI,         MainFrame::OnPPScopsVOIROI      )    \
  EVT_MENU( ID_PP_SCOPS_VOI_IOI,         MainFrame::OnPPScopsVOIIOI  )    \
  EVT_MENU( ID_PP_SCOPS_VOI_PICKSLICES,  MainFrame::OnPPScopsVOIPickSlices  ) \
  EVT_MENU( ID_PP_SCOPS_VOI_STANDARDIZE, MainFrame::OnPPScopsVOIStandardize ) \
  EVT_MENU( ID_PP_SCOPS_ALGEBRA,     MainFrame::OnAlgebra      )    \
  EVT_MENU( ID_PP_SCOPS_INTERPOLATE, MainFrame::OnInterpolate  )    \
  EVT_MENU( ID_PP_SCOPS_CLASSIFY_1FEATURE, MainFrame::OnPPScops1Feature  )  \
  EVT_MENU( ID_PP_SCOPS_CLASSIFY_FUZZ_CONN, MainFrame::OnPPScopsFuzzComp  ) \
  EVT_MENU( ID_PP_SCOPS_CLASSIFY_INTEN_MAP, MainFrame::OnPPScopsIntenMap )  \
  EVT_MENU( ID_PP_SCOPS_FILTER,      MainFrame::OnPPScopsFilter)    \
  EVT_MENU( ID_PP_SCOPS_REGISTER,    MainFrame::OnRegister     )    \
  EVT_MENU( ID_PP_SCOPS_SEGMENT_THRESHOLD, MainFrame::OnPPScopsThreshold )  \
  EVT_MENU( ID_PP_SCOPS_SEGMENT_2DINTERACTIVE,                      \
                                     MainFrame::OnSegment2d    )    \
  EVT_MENU( ID_PP_SCOPS_IRFC,        MainFrame::OnPPScopsIRFC  )    \
  EVT_MENU( ID_PP_STOPS_TO_SCENE,    MainFrame::OnPPStopsToScene )  \
  EVT_MENU( ID_PREFERENCES,          MainFrame::OnPreferences  )    \
  EVT_MENU( ID_PRINT,                MainFrame::OnPrint        )    \
  EVT_MENU( ID_PRINT_PREVIEW,        MainFrame::OnPrintPreview )    \
  EVT_MENU( ID_RECIPES,              MainFrame::OnRecipes      )    \
  EVT_MENU( ID_REDO,                 MainFrame::OnRedo         )    \
  EVT_MENU( ID_SAVE_SCREEN,          MainFrame::OnSaveScreen   )    \
  EVT_MENU( ID_SHOW_SCREEN,          MainFrame::OnShowScreen   )    \
  EVT_MENU( ID_TASKS,                MainFrame::OnTasks        )    \
  EVT_MENU( ID_TUTORIALS,            MainFrame::OnTutorials    )    \
  EVT_MENU( ID_UNDO,                 MainFrame::OnUndo         )    \
  EVT_MENU( ID_VIS_CYCLE,            MainFrame::OnCycle        )    \
  EVT_MENU( ID_VIS_MONTAGE,          MainFrame::OnMontage      )    \
  EVT_MENU( ID_VIS_OVERLAY,          MainFrame::OnVisOverlay   )    \
  EVT_MENU( ID_VIS_SURF_VIEW,        MainFrame::OnVisSurfView  )    \
  EVT_MENU( ID_VIS_VOL_VIEW,         MainFrame::OnVisVolView   )    \
  EVT_MENU( ID_MANIPULATE,           MainFrame::OnManipulate   )    \
  EVT_MENU( ID_WINDOW_HIDE_CONTROLS, MainFrame::OnHideControls )    \
                                                                    \
  EVT_MENU_RANGE( MainFrame::ID_FIRST_DYNAMIC_WINDOW_MENU,          \
                  MainFrame::ID_LAST_DYNAMIC_WINDOW_MENU,           \
                  MainFrame::OnWindow )                             \
                                                                    \
  EVT_CHAR(     MainFrame::OnChar       )                           \
  EVT_CLOSE(    MainFrame::OnCloseEvent )                           \
  EVT_MAXIMIZE( MainFrame::OnMaximize   )                           \
  EVT_MOVE(     MainFrame::OnMove       )                           \
                                                                    \
  EVT_BUTTON( ID_APPEND_SCREEN,    MainFrame::OnAppendScreen    )   \
  EVT_BUTTON( ID_BROWSE_SCREEN,    MainFrame::OnBrowseScreen    )   \
  EVT_BUTTON( ID_OVERWRITE_SCREEN, MainFrame::OnOverwriteScreen )
//======================================================================
/** \brief Definition and implementation of MainSplitter (event table in
 *  MainFrame.cpp).
 */
class MainSplitter : public wxSplitterWindow {
  private:
    MainFrame*  mFrame;  ///< frame to split
  public:
    /** \brief MainSplitter ctor.
     *  \param parent frame to split
     */
    explicit MainSplitter ( MainFrame* parent=nullptr )
        : wxSplitterWindow( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxSP_3D | wxSP_LIVE_UPDATE | wxCLIP_CHILDREN )
    {
        mFrame = parent;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief called when the splitter position has changed. */
    virtual void OnPositionChanged ( wxSplitterEvent& event ) {
        //cout << typeid(this->mFrame).name() << "::OnPositionChanged " << event.GetSashPosition() << std::endl;
        if (mFrame != nullptr)    mFrame->Refresh();
        event.Skip();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief called as the splitter position is changed. */
    virtual void OnPositionChanging ( wxSplitterEvent& event ) {
        //cout << "MainFrame::OnPositionChanging \n";
        if (mFrame != nullptr)    mFrame->Refresh();
        event.Skip();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief double click causes the controls to be hidden. */
    virtual void OnDClick ( wxSplitterEvent& event ) {
        mFrame->mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
        mFrame->Refresh();
        event.Skip();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief called when no longer split. */
    virtual void OnUnsplitEvent ( wxSplitterEvent& event ) {
        if (mFrame != nullptr)    mFrame->Refresh();
        event.Skip();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( MainSplitter )
    DECLARE_EVENT_TABLE()
};
//======================================================================
