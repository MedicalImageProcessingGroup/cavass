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
    wxTextCtrl*       mDisplay;
    wxTextCtrl*       mGeneral;
    wxListBox*        mListBox;
    wxNotebook*       mNotebook;
    const MainFrame*  mParent;
    wxTextCtrl*       mScene;
    wxSizer*          mSizer;
    wxTextCtrl*       mStructure;
    enum { ID_INFO_LIST=200 };
public:
    InformationDialog ( )
      : wxDialog ( nullptr, -1, "Information", wxDefaultPosition, wxDefaultSize ),
        mParent( nullptr )
    {
        mDisplay   = nullptr;
        mGeneral   = nullptr;
        mListBox   = nullptr;
        mNotebook  = nullptr;
        mParent    = nullptr;
        mScene     = nullptr;
        mSizer     = nullptr;
        mStructure = nullptr;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    explicit InformationDialog ( MainFrame* parent );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    explicit InformationDialog ( wxArrayString& selected, MainFrame* parent=nullptr );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void Prepare ( wxArrayString selected );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnSelect ( wxCommandEvent& unused );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ~InformationDialog ( );

private:
    DECLARE_DYNAMIC_CLASS( InformationDialog )
    DECLARE_EVENT_TABLE()
};
//======================================================================

