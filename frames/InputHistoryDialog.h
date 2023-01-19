/*
  Copyright 1993-2011 Medical Image Processing Group
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
 * \file   InputHistoryDialog.h
 * \brief  InputHistoryDialog class definition and implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
/** \brief This class allows the user to view and modify the input file
 *  history.
 */
class InputHistoryDialog : public wxDialog {
  public:
    /** \brief True indicates that this col should be sorted numerically;
     *  otherwise, it will be sorted lexically.
     */
    static const bool  sSortNumeric[7];  ///< a value of true indicates that this col should be sorted numerically
    int                mLastCol;    ///< col last sorted by
    bool               mAscending;  ///< sort order
    wxListCtrl*        mListCtrl;   ///< contains list of files
  protected:
    enum { ID_LIST=100, ID_BROWSE, ID_INFO, ID_UNSELECT, ID_DELETE };
    const MainFrame*   mParent;     ///< optional parent frame
    wxSizer*           mSizer;      ///< sizer
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    InputHistoryDialog ( void );
    InputHistoryDialog ( MainFrame* parent );
    ~InputHistoryDialog ( void );

    void OnDoBrowse   ( wxCommandEvent& unused );
    void OnDoCancel   ( wxCommandEvent& unused );
    void OnDoDelete   ( wxCommandEvent& unused );
    void OnDoInfo     ( wxCommandEvent& unused );
    void OnDoOK       ( wxCommandEvent& unused );
    void OnDoUnselect ( wxCommandEvent& unused );

    static int wxCALLBACK compareFunction ( wxIntPtr item1, wxIntPtr item2,
                                            wxIntPtr sortData );
    void OnColClick ( wxListEvent& event );
protected:
    bool insert ( const wxString filename, const int where );
    bool duplicated ( wxFileName fn ) const;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( InputHistoryDialog )
    DECLARE_EVENT_TABLE()
};
//======================================================================

