/*
  Copyright 1993-2010 Medical Image Processing Group
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
 * \file   InformationDialog.h
 * \brief  InformationDialog class definition and implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
class MainFrame;

class InformationDialog : public wxDialog {
    wxTextCtrl*  mDisplay;
    wxTextCtrl*  mGeneral;
    wxListBox*   mListBox;
    wxNotebook*  mNotebook;
    const MainFrame*   mParent;
    wxTextCtrl*  mScene;
    wxSizer*     mSizer;
    wxTextCtrl*  mStructure;
    enum { ID_INFO_LIST=200 };
public:
    InformationDialog ( void )
      : wxDialog ( NULL, -1, "Information", wxDefaultPosition, wxDefaultSize ),
        mParent(NULL)
    {
        mDisplay   = NULL;
        mGeneral   = NULL;
        mListBox   = NULL;
        mNotebook  = NULL;
        mParent    = NULL;
        mScene     = NULL;
        mSizer     = NULL;
        mStructure = NULL;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    InformationDialog ( MainFrame* parent );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    InformationDialog ( wxArrayString& selected, MainFrame* parent=NULL );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void Prepare ( wxArrayString selected );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnSelect ( wxCommandEvent& unused );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ~InformationDialog ( void ) { }

private:
    DECLARE_DYNAMIC_CLASS( InformationDialog )
    DECLARE_EVENT_TABLE()
};
//======================================================================

