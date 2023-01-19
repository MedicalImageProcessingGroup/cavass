/*
  Copyright 1993-2009, 2015, 2017 Medical Image Processing Group
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
 * \file   EasyHeaderFrame.h
 * \brief  EasyHeaderFrame definition.
 * \author Dewey Odhner
 *
 * Copyright: (C) 2009 University of Pennsylvania
 *
 */
//======================================================================
#ifndef __EasyHeaderFrame_h
#define __EasyHeaderFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "EasyHeaderCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>


/** \brief EasyHeaderFrame class definition.
 *
 */
class EasyHeaderFrame : public MainFrame {

  public:
    static wxFileHistory  _fileHistory;
    enum {
        ID_SAVE=ID_LAST,
		ID_FOREGROUND,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN,
		ID_DATE, ID_TIME, ID_STUDY, ID_SERIES,
		ID_DIMENSION,
		ID_SLICE_WIDTH, ID_SLICE_HEIGHT,
		ID_BITS, ID_PIXEL_SIZE,
		ID_MIN_DENSITY, ID_MAX_DENSITY,
		ID_HEADER_SLICES, ID_FIRST_SLICE_LOC, ID_SLICE_SPACING,
		ID_VOLUMES, ID_FIRST_VOLUME_LOC, ID_VOLUME_SPACING,
		ID_READ_HEADER, ID_SWAP_BYTES, ID_WRITE_HEADER, ID_WRITE_DATA,
		ID_HEADER_LENGTH, ID_SKIP, ID_DATA_SLICES, ID_INCREMENT
    };

	EasyHeaderCanvas *tCanvas;

  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxFlexGridSizer*  fgs;

	wxButton*      mReadHeader;
	wxCheckBox    *mSwapBytes;
	wxButton*      mWriteHeader;
	wxButton*      mWriteData;
	wxStaticText  *mHeaderLengthLabel;
	wxTextCtrl    *mHeaderLengthCtrl;
	wxStaticText  *mSkipLabel;
	wxTextCtrl    *mSkipCtrl;
	wxStaticText  *mDataSlicesLabel;
	wxTextCtrl    *mDataSlicesCtrl;
	wxStaticText  *mIncrementLabel;
	wxTextCtrl    *mIncrementCtrl;

	wxSizer*       m_buttonSizer;
	wxFlexGridSizer* m_fgs;

	wxFileName    *mDataInFile;
	wxFileName    *mOutFile;
	ViewnixHeader *mVH;

    void initializeMenu ( void );
    void addButtonBox ( void );
	void emptyButtonBox ( void );
	void deleteEasyHeaderControls(void);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createEasyHeaderFrame ( wxFrame* parentFrame,
        bool useHistory=true );
    EasyHeaderFrame ( bool maximize=false, int w=800, int h=600 );
    ~EasyHeaderFrame ( void );

    void OnCopy         ( wxCommandEvent& e );
	virtual void OnInput           ( wxCommandEvent& unused );

    void OnHideControls ( wxCommandEvent& e );
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
	void OnDimension    ( wxCommandEvent& e );
	void OnReadHeader   ( wxCommandEvent& e );
	void OnWriteHeader  ( wxCommandEvent& e );
	void OnWriteData    ( wxCommandEvent& e );
	void OnHeaderLength ( wxCommandEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( EasyHeaderFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
