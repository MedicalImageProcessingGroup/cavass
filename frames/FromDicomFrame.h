/*
  Copyright 1993-2010, 2015, 2017 Medical Image Processing Group
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
 * \file   FromDicomFrame.h
 * \brief  FromDicomFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FromDicomFrame_h
#define __FromDicomFrame_h

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "FromDicomCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

#define PROCESS_MANAGER_TAKES_ABSOLUTE_PATH

class  SetFromDicomOutputControls;


/** \brief FromDicomFrame class definition.
 *
 *  a frame with an overlay of two data sets.
 */
class FromDicomFrame : public MainFrame {
	SetFromDicomOutputControls* mSetOutputControls;

  public:
    static wxFileHistory  _fileHistory;
    enum {
        ID_SAVE=ID_LAST,
		ID_FOREGROUND, ID_PATIENT,
        ID_OVERWRITE_SCREEN, ID_APPEND_SCREEN, ID_BROWSE_SCREEN
    };

	FromDicomCanvas *tCanvas;

  protected:
    wxMenu*        m_options_menu;      ///< options menu
    wxFlexGridSizer*  fgs;

	wxButton*      m_input;
	wxButton*      m_saveBut;
	wxCheckBox*    mForeground;   ///< run-in-foreground flag
	wxCheckBox*    mPatient;      ///< retain-patient-ID flag

	wxSizer*       m_buttonSizer;
	wxFlexGridSizer* m_fgs;

	wxString       m_InDir;

	int nseries;
	int *series_slices;
	wxFileName ***series_filename;

    void initializeMenu ( void );
    void addButtonBox ( void );
	void emptyButtonBox ( void );
	void deleteFromDicomControls(void);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createFromDicomFrame ( wxFrame* parentFrame,
        bool useHistory=true );
    FromDicomFrame ( bool maximize=false, int w=800, int h=600 );
    ~FromDicomFrame ( void );

    void OnFromDicomSave   ( wxCommandEvent& e );

    void OnCopy         ( wxCommandEvent& e );
	virtual void OnInput           ( wxCommandEvent& unused );

    void OnHideControls ( wxCommandEvent& e );
    void OnPrintPreview ( wxCommandEvent& e );
    void OnPrint        ( wxCommandEvent& e );
    void OnChar         ( wxKeyEvent& e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( FromDicomFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
