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
 * \file   ITKFilterFrame.h
 * \brief  ITKFilterFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This class creates a frame for ITK filters.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ITKFilterFrame_h
#define __ITKFilterFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "ITKFilterCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  GrayMapControls;
class  ITKFilterControls;
class  SetIndexControls;

#define  MaxITKParameters  10  ///< max # of command line parameters

/** \brief ITKFilterFrame class definition.
 *
 * This class creates a frame for ITK filters.
 */
class ITKFilterFrame : public MainFrame {
  protected:
    GrayMapControls*    mGrayMapControls;           ///< gray map controls
    GrayMapControls*    mFilteredGrayMapControls;   ///< gray map controls for filtered data
	ITKFilterControls*  mITKFilterControls;         ///< filter controls
    SetIndexControls*   mSetIndexControls;          ///< set index controls
    SetIndexControls*   mFilteredSetIndexControls;  ///< set index controls for filtered data
    int                 mWhichFilter;               ///< which filter is selected from table
    wxComboBox*         mFilterName;                ///< button displays current filter name
	wxString            mP[ MaxITKParameters ];     ///< parameter strings
	bool                mInputIsBinary;             ///< true when input is binary
	bool                mSyncChecked;               ///< true when sync is checked (to sync scale and other changes)
  public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_PREVIOUS=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_NEXT,
        ID_SET_INDEX, ID_SLICE_SLIDER, ID_SCALE_SLIDER,
        ID_GRAYMAP, ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET,
        ID_FILTER, ID_FILTER_NAME,
        ID_SAVE, ID_RESET,
        ID_FILTERED_SET_INDEX, ID_FILTERED_SLICE_SLIDER, ID_FILTERED_SCALE_SLIDER,
        ID_FILTERED_GRAYMAP, ID_FILTERED_CENTER_SLIDER, ID_FILTERED_WIDTH_SLIDER, ID_FILTERED_INVERT,
		ID_SYNC,
		//the following MUST be kept in sync with MaxITKParameters in ITKFilterFrame.cpp.
		ID_PARAM1, ID_PARAM2, ID_PARAM3, ID_PARAM4, ID_PARAM5,
		ID_PARAM6, ID_PARAM7, ID_PARAM8, ID_PARAM9, ID_PARAM10
    };
    int  mFileOrDataCount;  ///< count data/datafiles used (1 for loaded data; 2 for loaded and filtered data).
  protected:
    void addButtonBox   ( void );
    void initializeMenu ( void );
    void removeControls ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createITKFilterFrame ( wxFrame* parentFrame, bool useHistory=true );
    ITKFilterFrame ( bool maximize=false, int w=800, int h=600 );
    ~ITKFilterFrame ( void );
    static bool match ( wxString filename );
    //"virtualize" a static method
    virtual bool filenameMatch ( wxString filename ) const {
        return match( filename );
    };

    void loadFile ( const char* const fname );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data,
        const ViewnixHeader* const vh=NULL, const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    virtual void OnFilter          ( wxCommandEvent& unused );
    virtual void OnFilteredGrayMap ( wxCommandEvent& unused );
    virtual void OnFilteredInvert  ( wxCommandEvent& e      );
    virtual void OnFilteredSetIndex( wxCommandEvent& unused );
    virtual void OnFilterName      ( wxCommandEvent& unused );
    virtual void OnGrayMap         ( wxCommandEvent& unused );
    virtual void OnInput           ( wxCommandEvent& unused );
    virtual void OnInvert          ( wxCommandEvent& e      );
    virtual void OnITKParam1       ( wxCommandEvent& e      );
    virtual void OnITKParam2       ( wxCommandEvent& e      );
    virtual void OnITKParam3       ( wxCommandEvent& e      );
    virtual void OnITKParam4       ( wxCommandEvent& e      );
    virtual void OnITKParam5       ( wxCommandEvent& e      );
    virtual void OnITKParam6       ( wxCommandEvent& e      );
    virtual void OnITKParam7       ( wxCommandEvent& e      );
    virtual void OnITKParam8       ( wxCommandEvent& e      );
    virtual void OnITKParam9       ( wxCommandEvent& e      );
    virtual void OnITKParam10      ( wxCommandEvent& e      );
    virtual void OnMouseWheel      ( wxMouseEvent&   e      );
    virtual void OnNext            ( wxCommandEvent& unused );
    virtual void OnOpen            ( wxCommandEvent& unused );
    virtual void OnPrevious        ( wxCommandEvent& unused );
    virtual void OnPrint           ( wxCommandEvent& unused );
    virtual void OnPrintPreview    ( wxCommandEvent& unused );
    virtual void OnReset           ( wxCommandEvent& unused );
    virtual void OnSave            ( wxCommandEvent& unused );
    virtual void OnSaveScreen      ( wxCommandEvent& unused );
	virtual void OnSetIndex        ( wxCommandEvent& unused );
    virtual void OnSync            ( wxCommandEvent& e      );

    virtual void OnCenterSlider ( wxScrollEvent& e );
    virtual void OnWidthSlider  ( wxScrollEvent& e );
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
    virtual void OnScaleSlider  ( wxScrollEvent& e );
    virtual void OnSliceSlider  ( wxScrollEvent& e );

    virtual void OnFilteredCenterSlider ( wxScrollEvent& e );
    virtual void OnFilteredWidthSlider  ( wxScrollEvent& e );
    virtual void OnFilteredScaleSlider  ( wxScrollEvent& e );
    virtual void OnFilteredSliceSlider  ( wxScrollEvent& e );
#ifdef __WXX11__
    virtual void OnUpdateUICenterSlider ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIWidthSlider  ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIScaleSlider  ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUISliceSlider  ( wxUpdateUIEvent& unused );

    virtual void OnUpdateUIFilteredCenterSlider ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIFilteredWidthSlider  ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIFilteredScaleSlider  ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIFilteredSliceSlider  ( wxUpdateUIEvent& unused );
#endif
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ITKFilterFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
