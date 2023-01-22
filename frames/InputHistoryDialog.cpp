/*
  Copyright 1993-2014, 2016-2017 Medical Image Processing Group
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
 * \file   InputHistoryDialog.cpp
 * \brief  InputHistoryDialog class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool InputHistoryDialog::sSortNumeric[7] = {
    true, false, false, false, false, true, false
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief InputHistoryDialog ctor (w/out a parent frame). */
InputHistoryDialog::InputHistoryDialog ( void )
    : wxDialog ( NULL, -1, "Input File History", wxDefaultPosition,
                 wxDefaultSize ),
      mParent( 0 )
{
    mListCtrl  = NULL;
    mSizer     = NULL;
    mLastCol   = 0;
    mAscending = true;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief InputHistoryDialog ctor (w/ a parent frame). */
InputHistoryDialog::InputHistoryDialog ( MainFrame* parent )
    : wxDialog ( parent, -1, "Input File History", wxDefaultPosition,
                 wxDefaultSize ),
      mParent( parent )
{
    mLastCol   = 0;
    mAscending = true;
    wxPanel*  mainPanel = new wxPanel( this, -1 );
//    ::setBackgroundColor( this );
//    ::setBackgroundColor( mainPanel );
    mSizer = new wxFlexGridSizer( 1, 1, 5 );  //1 col,vgap,hgap

    mListCtrl = new wxListCtrl( mainPanel, ID_LIST, wxDefaultPosition,
        wxSize(640,400), wxLC_REPORT | wxSUNKEN_BORDER );
    int c=0;
#ifdef LIST_INPUT_FILE_NUMBER
    const int  width = 640/13;
    mListCtrl->InsertColumn( c++, "number",             wxLIST_FORMAT_LEFT, width );
#else
    const int  width = 640/12;
#endif
    mListCtrl->InsertColumn( c++, "path",             wxLIST_FORMAT_LEFT, width*3 );
    mListCtrl->InsertColumn( c++, "file name",        wxLIST_FORMAT_LEFT, width*3 );
    mListCtrl->InsertColumn( c++, "type",             wxLIST_FORMAT_LEFT, width*2/3 );
    mListCtrl->InsertColumn( c++, "modif. date/time", wxLIST_FORMAT_LEFT, width*7/3 );
    mListCtrl->InsertColumn( c++, "size (MB)",        wxLIST_FORMAT_LEFT, width );
    mListCtrl->InsertColumn( c++, "description",      wxLIST_FORMAT_LEFT, width*2 );

    //load the history from the configuration
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Load( *pConfig );
    bool *selected=NULL;
    if (::gInputFileHistory.GetNoHistoryFiles())
        selected =
          (bool *)malloc(::gInputFileHistory.GetNoHistoryFiles()*sizeof(bool));
    for (int i=0,j=0; j<(int)::gInputFileHistory.GetNoHistoryFiles(); j++) {
        wxString  filename = ::gInputFileHistory.GetHistoryFile( j );
        if (insert(filename, i))
            selected[i++] = ::gInputFileHistory.IsSelected( j );
    }

    //select all entries in the list
    for (int k=0; k<mListCtrl->GetItemCount(); k++) {
        if (selected[k])
            mListCtrl->SetItemState( k, wxLIST_STATE_SELECTED,
                wxLIST_STATE_SELECTED );
    }
    if (selected)
        free(selected);

    mSizer->Add( mListCtrl, 0, wxGROW|wxALL, 5 );

    wxSizer*   gs = new wxGridSizer( 7, 5, 5 );
    const int  bWidth = 600/7;
    wxButton*  b;

    //add user interface buttons
    b = new wxButton( mainPanel, ID_BROWSE, "Browse", wxDefaultPosition,
                      wxSize(bWidth,-1) );
    gs->Add( b, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    b = new wxButton( mainPanel, ID_INFO, "Info", wxDefaultPosition,
                      wxSize(bWidth,-1) );
    gs->Add( b, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    b = new wxButton( mainPanel, ID_UNSELECT, "UnselectAll",
                      wxDefaultPosition, wxSize(bWidth,-1) );
    gs->Add( b, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    b = new wxButton( mainPanel, ID_DELETE, "DeleteFiles", wxDefaultPosition,
                      wxSize(bWidth,-1) );
    gs->Add( b, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    gs->AddSpacer( -1 );

    b = new wxButton( mainPanel, wxID_OK, "OK", wxDefaultPosition,
                      wxSize(bWidth,-1) );
    gs->Add( b, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    b = new wxButton( mainPanel, wxID_CANCEL, "Cancel", wxDefaultPosition,
                      wxSize(bWidth,-1) );
    gs->Add( b, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );

    mSizer->Add( gs );

    mainPanel->SetAutoLayout( true );
    mainPanel->SetSizer( mSizer );
    mSizer->SetSizeHints( this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Callback for when the Browse button is pressed. */
void InputHistoryDialog::OnDoBrowse ( wxCommandEvent& unused ) {
	wxArrayString  filenames;
	wxFileDialog  fd( this, _T("Select image file(s)"), _T(""), _T(""),
		mParent->mFileNameFilter,
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );
	if (fd.ShowModal() != wxID_OK)  {  Raise();  return;  }
	fd.GetPaths( filenames );
	if (filenames.Count()==0)  {  Raise();  return;  }
	OnDoUnselect( unused );
	for (int i=filenames.Count()-1; i>=0; i--) {
		//if it already is in the list, then delete
		for (int k=i; k<mListCtrl->GetItemCount(); k++) {
			wxListItem info;
			info.SetId(k);
#ifdef LIST_INPUT_FILE_NUMBER
			info.SetColumn(1);
			mListCtrl->GetItem(info);
			wxString dir=info.GetText();
			info.SetColumn(2);
#else
			info.SetColumn(0);
			mListCtrl->GetItem(info);
			wxString dir=info.GetText();
			info.SetColumn(1);
#endif
			mListCtrl->GetItem(info);
			wxFileName fn=wxFileName(dir, info.GetText());
			if (fn.GetFullPath() == filenames[i])
				mListCtrl->DeleteItem(k--);
		}
	    //insert it
	    insert( filenames[i], 0 );
	    //select it
	    mListCtrl->SetItemState( 0, wxLIST_STATE_SELECTED,
	                                wxLIST_STATE_SELECTED );
	}
#ifdef LIST_INPUT_FILE_NUMBER
    //update numbering/renumber (otherwise, we'll have 2 #0's)
    for (int i=0; i<mListCtrl->GetItemCount(); i++) {
        mListCtrl->SetItem( i, 0, wxString::Format(_T("%d"),i+1) );
    }
#endif
    Raise();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Callback for when the Cancel button is pressed. */
void InputHistoryDialog::OnDoCancel ( wxCommandEvent& unused ) {
    EndModal( wxID_CANCEL );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Callback for when the Delete button is pressed. */
void InputHistoryDialog::OnDoDelete ( wxCommandEvent& unused ) {
    //build up a string of all file names of files to be deleted
    wxString  files = "Are you sure you want to permanently delete these files?\n\n";
    int  i, count;
    for (i=-1,count=0; ; count++) {
        i = mListCtrl->GetNextItem( i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
        if (i == -1)    break;
        wxListItem  item;
        item.m_itemId = i;
#ifdef LIST_INPUT_FILE_NUMBER
        item.m_col    = 1;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        files += "        ";
        files += item.m_text;
        files += wxFileName::GetPathSeparator();
        item.m_col    = 2;
#else
        item.m_col    = 0;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        files += "        ";
        files += item.m_text;
        files += wxFileName::GetPathSeparator();
        item.m_col    = 1;
#endif
        mListCtrl->GetItem( item );
        files += item.m_text;
        files += "\n";
    }
    if (count==0)    return;  //nothing to delete
    //ask the user if they are sure
    wxMessageDialog  md( this, files, "Permanently delete files...", wxOK|wxCANCEL );
    if (md.ShowModal()==wxID_CANCEL)    return;
    //the user is sure.  permanently delete the files.
    for (i=-1; ; ) {
        i = mListCtrl->GetNextItem( i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
        if (i ==-1)    break;
        wxListItem  item;
        item.m_itemId = i;
#ifdef LIST_INPUT_FILE_NUMBER
        item.m_col    = 1;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        wxString  fn = item.m_text;
        fn += wxFileName::GetPathSeparator();
        item.m_col    = 2;
#else
        item.m_col    = 0;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        wxString  fn = item.m_text;
        fn += wxFileName::GetPathSeparator();
        item.m_col    = 1;
#endif
        mListCtrl->GetItem( item );
        fn += item.m_text;
        unlink( fn.c_str() );
        mListCtrl->DeleteItem( i );
    }
    OnDoUnselect( unused );  //because some things remain incorrectly selected
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Callback for when the Info button is pressed.
 */
void InputHistoryDialog::OnDoInfo ( wxCommandEvent& unused ) {
	wxArrayString files;
    int  i, count;
    for (i=-1,count=0; ; count++) {
        i = mListCtrl->GetNextItem( i, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
        if (i ==-1)    break;
        wxListItem  item;
        item.m_itemId = i;
#ifdef LIST_INPUT_FILE_NUMBER
        item.m_col    = 1;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        wxString  fn = item.m_text;
        fn += wxFileName::GetPathSeparator();
        item.m_col    = 2;
#else
        item.m_col    = 0;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        wxString  fn = item.m_text;
        fn += wxFileName::GetPathSeparator();
        item.m_col    = 1;
#endif
        mListCtrl->GetItem( item );
        fn += item.m_text;
        files.Add( fn );
    }
	if (count == 0) {
		wxMessageBox( "No files selected." );
		return;
	}
	InformationDialog  id( files );
	id.ShowModal();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Callback for when the OK button is pressed. */
void InputHistoryDialog::OnDoOK ( wxCommandEvent& unused ) {
    //remove all files from the history (only those selected in the
    // dialog box list control will be added below)
    ::gInputFileHistory.clear( mParent->mFileNameFilter );
    //add selected files to history
    for (int i=mListCtrl->GetItemCount()-1; i>=0; i--) {
        if (mListCtrl->GetItemState(i, wxLIST_STATE_SELECTED)) {
            wxListItem  item;
            item.m_itemId = i;
#ifdef LIST_INPUT_FILE_NUMBER
            item.m_col    = 1;
            item.m_mask   = wxLIST_MASK_TEXT;
            mListCtrl->GetItem( item );
            wxString  dirStr = item.m_text;

            item.m_col    = 2;
#else
            item.m_col    = 0;
            item.m_mask   = wxLIST_MASK_TEXT;
            mListCtrl->GetItem( item );
            wxString  dirStr = item.m_text;

            item.m_col    = 1;
#endif
            mListCtrl->GetItem( item );

            wxString  temp = dirStr + wxFileName::GetPathSeparator()
                             + item.m_text;
            ::gInputFileHistory.AddFileToHistory( temp );
        }
    }

    //save in the configuration for future use
    wxConfigBase*  pConfig = wxConfigBase::Get();
    ::gInputFileHistory.Save( *pConfig );
    EndModal( wxID_OK );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Unselect/deselect everything in the list. */
void InputHistoryDialog::OnDoUnselect ( wxCommandEvent& unused ) {
    for (int i=0; i<mListCtrl->GetItemCount(); i++) {
        mListCtrl->SetItemState( i, 0, wxLIST_STATE_SELECTED );
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief   this function is used to compare 2 rows in the list control
 *  \param   item1 is the col 0 value and item data for row in list
 *  \param   item2 is the col 0 value and item data for row in list
 *  \param   sortData is a ptr to a list control w/ data to be sorted
 *  \returns -1 if item1<item2; 0 if item1==item2; +1 if item1>item2
 */
int wxCALLBACK InputHistoryDialog::compareFunction ( wxIntPtr item1,
                                                     wxIntPtr item2,
                                                     wxIntPtr sortData )
{
    InputHistoryDialog*  ihd = (InputHistoryDialog*)sortData;
    if (ihd->mListCtrl==NULL)    return 0;

    //find the row in the list whose col 0 value/item data is the
    // same as item1.
    int  row1 = -1;
    int  i;
    for (i=0; i<ihd->mListCtrl->GetItemCount(); i++) {
        int  data = ihd->mListCtrl->GetItemData( i );
        if (data==item1)    row1 = i;
    }
    if (row1==-1)    return 0;

    //find the row in the list whose col 0 value/item data is the
    // same as item2.
    int  row2 = -1;
    for (i=0; i<ihd->mListCtrl->GetItemCount(); i++) {
        int  data = ihd->mListCtrl->GetItemData( i );
        if (data==item2)    row2 = i;
    }
    if (row2==-1)    return 0;

    //get the string from the particular col that we wish to compare
    // (from the first row that we wish to compare)
    wxListItem  it1;
    it1.m_itemId = row1;
    it1.m_col    = ihd->mLastCol;
    it1.m_mask   = wxLIST_MASK_TEXT;
    ihd->mListCtrl->GetItem( it1 );
    wxString  str1 = it1.m_text;

    //get the string from the particular col that we wish to compare
    // (from the second row that we wish to compare)
    wxListItem  it2;
    it2.m_itemId = row2;
    it2.m_col    = ihd->mLastCol;
    it2.m_mask   = wxLIST_MASK_TEXT;
    ihd->mListCtrl->GetItem( it2 );
    wxString  str2 = it2.m_text;

    //do we need to sort numerically (or as strings)?
    if (sSortNumeric[ihd->mLastCol]) {
        //sort numerically
        double  d1 = atof( str1.c_str() );
        double  d2 = atof( str2.c_str() );
        if (ihd->mAscending) {
            //sort ascending
            if (d1<d2)    return -1;
            if (d1==d2)   return 0;
            return 1;
        }
        //sort descending
        if (d1<d2)    return 1;
        if (d1==d2)   return 0;
        return -1;
    }

	if (ihd->mLastCol == 4) {
		//sort by date
		wxDateTime dt1, dt2;
		dt1.ParseDateTime(str1);
		dt2.ParseDateTime(str2);
		if (dt1.IsEqualTo(dt2))   return 0;
		return ihd->mAscending==dt1.IsEarlierThan(dt2)? -1: 1;
	}

    //sort as strings
    if (ihd->mAscending)  return str1.Cmp( str2 );  //sort ascending
    return str2.Cmp( str1 );                        //sort descending
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Callback for when the user clicks in a column indicating
 *  that the contents should be sorted.
 */
void InputHistoryDialog::OnColClick ( wxListEvent& event ) {
    int  col = event.GetColumn();
    if (col==mLastCol) {
        mAscending = !mAscending;
    } else {
        mAscending = true;
        mLastCol   = col;
    }
    mListCtrl->SortItems( compareFunction, (wxIntPtr)this );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief InputHistoryDialog dtor. */
InputHistoryDialog::~InputHistoryDialog ( void ) {
    if (mListCtrl)  {  delete mListCtrl;  mListCtrl=NULL;  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief Insert a file name into the list.
 *  \param filename is the name of the file to be inserted into the list.
 *  \param where is where the item should be inserted in the list.
 *  \returns true if the item does not already appear in the list; false otherwise.
 */
bool InputHistoryDialog::insert ( const wxString filename, const int where ) {
    if (filename.length()<1)    return false;
    wxFileName  fn( filename );
    //make sure that an entry doesn't appear more than once
    if (duplicated(fn))    return false;
    mListCtrl->InsertItem( where, wxString::Format("%d",where+1) );
    mListCtrl->SetItemData( where, where );
#ifdef LIST_INPUT_FILE_NUMBER
    mListCtrl->SetItem( where, 1, fn.GetPath()     );
    mListCtrl->SetItem( where, 2, fn.GetFullName() );
    mListCtrl->SetItem( where, 3, fn.GetExt()      );

    //check to determine if the file still exists
    if (!::wxFileExists( filename )) {
        mListCtrl->SetItem( where, 4, "<unavailable>" );
        mListCtrl->SetItem( where, 5, "<unavailable>" );
        return true;
    }

    wxDateTime  dtAccess, dtMod, dtCreate;
    fn.GetTimes( &dtAccess, &dtMod, &dtCreate );
    mListCtrl->SetItem( where, 4, dtCreate.FormatDate()+" "+dtCreate.FormatTime() );
    //get the size of the file
#if defined (WIN32) || defined (_WIN32)
	struct _stat64  s;
    _stat64( filename, &s );
#else
    struct stat64  s;
    stat64( filename.c_str(), &s );
#endif
    mListCtrl->SetItem( where, 5, wxString::Format("%.2f",s.st_size/1000000.0) );
#else
    mListCtrl->SetItem( where, 0, fn.GetPath()     );
    mListCtrl->SetItem( where, 1, fn.GetFullName() );
    mListCtrl->SetItem( where, 2, fn.GetExt()      );

    //check to determine if the file still exists
    if (!::wxFileExists( filename )) {
        mListCtrl->SetItem( where, 3, "<unavailable>" );
        mListCtrl->SetItem( where, 4, "<unavailable>" );
        return true;
    }

    wxDateTime  dtAccess, dtMod, dtCreate;
    fn.GetTimes( &dtAccess, &dtMod, &dtCreate );
    mListCtrl->SetItem( where, 3, dtCreate.FormatDate()+" "+dtCreate.FormatTime() );
    //get the size of the file
#if defined (WIN32) || defined (_WIN32)
	struct _stat64  s;
    _stat64( filename, &s );
#elif defined( __MACH__ )
    struct stat  s;
    stat( filename.c_str(), &s );
#else
    struct stat64  s;
    stat64( filename.c_str(), &s );
#endif
    mListCtrl->SetItem( where, 4, wxString::Format("%.2f",s.st_size/1000000.0) );
#endif
    return true;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief   this function determines if the given file name is already
 *           in the list.
 *  \param   fn is the file name.
 *  \returns true if the given file name already appears in the list;
 *           false otherwise.
 */
bool InputHistoryDialog::duplicated ( wxFileName fn ) const {
    wxString  fnPath     = fn.GetPath();
    wxString  fnFullName = fn.GetFullName();

    for (int i=0; i<mListCtrl->GetItemCount(); i++) {
        wxListItem  item;
        item.m_itemId = i;
#ifdef LIST_INPUT_FILE_NUMBER
        item.m_col    = 1;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        wxString  dirStr = item.m_text;

        item.m_col    = 2;
#else
        item.m_col    = 0;
        item.m_mask   = wxLIST_MASK_TEXT;
        mListCtrl->GetItem( item );
        wxString  dirStr = item.m_text;

        item.m_col    = 1;
#endif
        mListCtrl->GetItem( item );

        //are they the same?
        if (dirStr.Cmp(fnPath)==0 && item.m_text.Cmp(fnFullName)==0)
            return true;  //this is a duplicate
    }
    return false;  //this is not a duplicate
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( InputHistoryDialog, wxDialog )
BEGIN_EVENT_TABLE       ( InputHistoryDialog, wxDialog )
    EVT_BUTTON( wxID_OK,     InputHistoryDialog::OnDoOK       )
    EVT_BUTTON( wxID_CANCEL, InputHistoryDialog::OnDoCancel   )
    EVT_BUTTON( ID_BROWSE,   InputHistoryDialog::OnDoBrowse   )
    EVT_BUTTON( ID_DELETE,   InputHistoryDialog::OnDoDelete   )
    EVT_BUTTON( ID_INFO,     InputHistoryDialog::OnDoInfo     )
    EVT_BUTTON( ID_UNSELECT, InputHistoryDialog::OnDoUnselect )

    EVT_LIST_COL_CLICK( ID_LIST, InputHistoryDialog::OnColClick )
END_EVENT_TABLE()
//======================================================================

