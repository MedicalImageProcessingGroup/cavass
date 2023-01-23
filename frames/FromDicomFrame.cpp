/*
  Copyright 1993-2018 Medical Image Processing Group
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
 * \file   FrameDicomFrame.cpp
 * \brief  FrameDicomFrame class implementation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  "cavass.h"
#include  "FromDicomControls.h"
#include  "port_data/from_dicom.h"
#include  <wx/datetime.h>

extern Vector  gFrameList;
//----------------------------------------------------------------------
/** \brief Constructor for FromDicomFrame class.
 *
 *  Most of the work is in creating the control panel at the bottom of
 *  of the window.
 */
FromDicomFrame::FromDicomFrame ( bool maximize, int w, int h ): MainFrame( 0 ){
    mSaveScreenControls = NULL;
    mSetOutputControls = NULL;

    ::gFrameList.push_back( this );
    if (!Preferences::getSingleFrameMode()) {
        gWhere += cWhereIncr;
        if (gWhere>WIDTH || gWhere>HEIGHT)    gWhere = cWhereIncr;
    }
    initializeMenu();
    ::setColor( this );
    mSplitter = new MainSplitter( this );
    mSplitter->SetSashGravity( 1.0 );
    mSplitter->SetSashPosition( -dControlsHeight );
    ::setColor( mSplitter );

    //top of window contains image(s) displayed via FromDicomCanvas
    mCanvas = tCanvas = new FromDicomCanvas( mSplitter, this, -1, wxDefaultPosition, wxDefaultSize );

    m_input = NULL;
    mForeground = NULL;
	mPatient = NULL;

    m_buttonBox = NULL;
    m_buttonSizer = NULL;
    m_fgs = NULL;

    m_InDir = wxString("");

    nseries = 0;
    series_filename = NULL;
    series_slices = NULL;

    if (Preferences::getCustomAppearance()) {
        mCanvas->SetBackgroundColour( wxColour(DkBlue) );
        mCanvas->SetForegroundColour( wxColour(Yellow) );
    }
    wxSizer*  topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize( 700, 400 );
    topSizer->Add( mCanvas, 1, wxGROW );

    //bottom: controls - - - - - - - - - - - - - - - - - - - - - - - - -
    mControlPanel = new wxPanel( mSplitter, -1, wxDefaultPosition, wxDefaultSize );
    ::setColor( mControlPanel );
    
    mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight );
    mBottomSizer = new wxBoxSizer( wxHORIZONTAL );

    addButtonBox();
    mControlPanel->SetAutoLayout( true );
    mControlPanel->SetSizer( mBottomSizer );

    if (maximize)
        Maximize( true );
    else
        SetSize( w, h );
    mSplitter->SetSashPosition( -dControlsHeight );
    Show();
    Raise();
#if wxUSE_DRAG_AND_DROP
    SetDropTarget( new MainFileDropTarget );
#endif
    SetStatusText( "", 2 );
    SetStatusText( "", 3 );
    SetStatusText( "", 4 );
    wxToolTip::Enable( Preferences::getShowToolTips() );
    mSplitter->SetSashPosition( -dControlsHeight );
    if (Preferences::getShowSaveScreen()) {
        wxCommandEvent  unused;
        OnSaveScreen( unused );
    }
}
//----------------------------------------------------------------------
/** \brief initialize menu bar and items (in addition to the standard ones). */
void FromDicomFrame::initializeMenu ( void ) {
    //init standard menu bar and menu items
    MainFrame::initializeMenu();
    //if we are in the mode that supports having more than one window open
    // at a time, we need to make other windows aware of this window.
    ::copyWindowTitles( this );

    wxString  tmp;
	int j;
	Vector::iterator  i;
	for (i=::gFrameList.begin(),j=1; i!=::gFrameList.end(); i++,j++)
		if (*i==this)
			break;
    tmp = wxString::Format( "CAVASS:FromDicom:%d", j);
    assert( !searchWindowTitles( tmp ) );
    mWindowTitle = tmp;
    ::addToAllWindowMenus( mWindowTitle );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 9/26/07 Dewey Odhner
void FromDicomFrame::addButtonBox ( void ) {
    //box for buttons
    mBottomSizer->Add( 0, 0, 10, wxGROW );  //spacer
    if (m_buttonBox == NULL)
        m_buttonBox = new wxStaticBox( mControlPanel, -1, "" );
    ::setColor( m_buttonBox );
    if (m_buttonSizer == NULL)
        m_buttonSizer = new wxStaticBoxSizer( m_buttonBox, wxHORIZONTAL );
    if (m_fgs == NULL)
        m_fgs = new wxFlexGridSizer( 2, 1, 1 );  //2 cols,vgap,hgap
    //row 1, col 1
    m_input = new wxButton( mControlPanel, ID_INPUT, "Input", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_input );
    m_fgs->Add( m_input, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 1, col 2
    m_saveBut = new wxButton( mControlPanel, ID_SAVE, "Save", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    ::setColor( m_saveBut );
    m_fgs->Add( m_saveBut, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0 );
    //row 2, col 1
    mForeground = new wxCheckBox( mControlPanel, ID_FOREGROUND, "Foreground", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mForeground->SetValue( true );
    ::setColor( mForeground );
    m_fgs->Add( mForeground, 0, wxALIGN_LEFT, 0 );

    //row 2, col 2
    mPatient = new wxCheckBox( mControlPanel, ID_PATIENT, "Patient", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
    mPatient->SetValue( false );
    ::setColor( mPatient );
    m_fgs->Add( mPatient, 0, wxALIGN_LEFT, 0 );

    m_buttonSizer->Add( m_fgs, 0, wxGROW|wxALL, 10 );
    mBottomSizer->Add( m_buttonSizer, 0, wxGROW|wxALL, 10 );
}

void FromDicomFrame::emptyButtonBox ( void )
{
    if (mPatient!=NULL) {
        m_fgs->Detach(mPatient);
        delete mPatient;
    }
    if (mForeground!=NULL) {
        m_fgs->Detach(mForeground);
        delete mForeground;
    }
    if (m_saveBut!=NULL) {
        m_fgs->Detach(m_saveBut);
        delete m_saveBut;
    }
    if (m_input!=NULL) {
        m_fgs->Detach(m_input);
        delete m_input;
    }
    m_input = NULL;
    m_saveBut = NULL;
    mForeground = NULL;
	mPatient = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief destructor for FromDicomFrame class. */
FromDicomFrame::~FromDicomFrame ( void ) {
    cout << "FromDicomFrame::~FromDicomFrame" << endl;
    wxLogMessage( "FromDicomFrame::~FromDicomFrame" );

    if (mCanvas!=NULL)        { delete mCanvas;        mCanvas=NULL;       }
    deleteFromDicomControls();
    emptyButtonBox();
    if (m_fgs!=NULL)
    {
        m_buttonSizer->Detach(m_fgs);
        delete m_fgs;
        m_fgs = NULL;
    }
    if (m_buttonSizer!=NULL)
    {
        mBottomSizer->Detach(m_buttonSizer);
        delete m_buttonSizer;
        m_buttonSizer = NULL;
    }
    if (mControlPanel!=NULL)  {
        mBottomSizer->Detach(mControlPanel);
        delete mControlPanel;
        mControlPanel=NULL;
    }
#if 0
    if (mBottomSizer!=NULL)   { delete mBottomSizer;   mBottomSizer=NULL;  }
#endif
    if (mSplitter!=NULL)      { delete mSplitter;      mSplitter=NULL;     }

    if (nseries)
    {
        for (int j=0; j<nseries; j++)
        {
            for (int k=0; k<series_slices[j]; k++)
                delete series_filename[j][k];
            free(series_filename[j]);
        }
        nseries = 0;
    }
    if (series_slices)
    {
        free(series_slices);
        free(series_filename);
        series_slices = NULL;
    }

    //if we are in the mode that supports having more than one window open
    // at a time, we need to remove this window from the list.
    Vector::iterator  i;
    for (i=::gFrameList.begin(); i!=::gFrameList.end(); i++) {
        if (*i==this) {
            ::gFrameList.erase( i );
            break;
        }
    }

    //if this window is the last remaining/only window, it is time for us
    // to exit.
    if (::gFrameList.begin() == ::gFrameList.end()) {
        exit(0);
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for key presses. */
void FromDicomFrame::OnChar ( wxKeyEvent& e ) {
    wxLogMessage( "FromDicomFrame::OnChar" );
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief This method should be called whenever one wishes to create a
 *  new FromDicomFrame.  It first searches the input file history for an
 *  acceptable input file.  If one is found, it is used.  If none is found
 *  then the user is prompted for an input file.
 *  \param parentFrame is an optional (it may also be NULL) parent frame
 *      which will be closed if we are in single frame mode.
 *      (Furthermore, if the parent is maximized (or not maximized),
 *      the child correspondingly will also be maximized (or not).)
 *  \param useHistory is an optional (default is true) flag to indicate 
 *      whether or not the input history should be used to obtain the most
 *      recent file to open.  If false, the history won't be used and a
 *      regular file open dialog box will be presented to the user.  This
 *      can be used by the traditional Open menu item.
 */
void FromDicomFrame::createFromDicomFrame ( wxFrame* parentFrame, bool useHistory )
{
    FromDicomFrame*  frame;
    if (parentFrame) {
        int  w, h;
        parentFrame->GetSize( &w, &h );
        frame = new FromDicomFrame( parentFrame->IsMaximized(), w, h );
    } else {
        frame = new FromDicomFrame( false, 800, 600 );
    }
    //if we are in single frame mode, close the parent frame
    if (parentFrame && Preferences::getSingleFrameMode())
        parentFrame->Close();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for Copy menu item. */
void FromDicomFrame::OnCopy ( wxCommandEvent& e ) {
    wxYield();

    wxLogMessage("Nothing to copy.");
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for toggling control panel on and off. */
void FromDicomFrame::OnHideControls ( wxCommandEvent& e ) {
    if (mSplitter->IsSplit()) {
        mSplitter->Unsplit();
        mHideControls->SetItemLabel( "Show Controls\tAlt-C" );
    } else {
        mCanvas->Show( true );
        mControlPanel->Show( true );
        mSplitter->SplitHorizontally( mCanvas, mControlPanel, -dControlsHeight );
        mHideControls->SetItemLabel( "Hide Controls\tAlt-C" );
    }
    Refresh();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*****************************************************************************
 * FUNCTION: get_element_from
 * DESCRIPTION: Returns the specified value from a DICOM file.
 * PARAMETERS:
 *    dir: the directory in which the file is
 *    filename: the file name
 *    group, element: the tag of element being read
 *    type: value representation of element being read (BI=16bits, BD=32bits,
 *       AN=ASCIInumeric, AT=ASCIItext)
 *    result: The value is stored here.
 *    maxlen: bytes at result
 *    items_read: The number of items read is stored here.
 * SIDE EFFECTS:
 * ENTRY CONDITIONS: rec_arch_type should be set.
 * RETURN VALUE:
 *    0: success
 *    1: memory allocation failure
 *    2: read error
 *    4: file open failure
 *    5: seek error
 *    100: group or element not found.
 *    235: element length greater than maxlen.
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 5/19/03 by Dewey Odhner
 *
 *****************************************************************************/
int get_element_from(const char dir[], const char filename[],
    unsigned short group, unsigned short element, int type, void *result,
    unsigned maxlen /* bytes at result */, int *items_read)
{
    int valid;
    char *tb;
    static unsigned int buf_size;
    static char *buf;
    FILE *fp;

    if (buf_size == 0)
    {
        buf = (char *)malloc(strlen(dir)+strlen(filename)+2);
        if (buf == NULL)
            return 1;
        buf_size = strlen(dir)+strlen(filename)+2;
        buf[buf_size-1] = 0;
    }
    if (buf_size < strlen(dir)+strlen(filename)+2)
    {
        tb = (char *)realloc(buf, strlen(dir)+strlen(filename)+2);
        if (tb == NULL)
            return 1;
        buf = tb;
        buf_size = strlen(dir)+strlen(filename)+2;
        buf[buf_size-1] = 0;
    }
    strcpy(buf, (const char *)wxFileName(dir, filename).GetFullPath().c_str());
    fp = fopen(buf, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Can't open file %s\n", filename);
        return 4;
    }
    valid = get_element(fp, group, element, type, result, maxlen, items_read);
    fclose(fp);
    return (valid);
}

// Modified: 1/31/08 Initialized descriptions where missing by Dewey Odhner.
// Modified: 11/24/08 Accept partial strings for description, date, time by Dewey Odhner.
void FromDicomFrame::OnInput ( wxCommandEvent& unused ) {
    // browse & scan directory
    wxFileDialog *dd;
    wxString path;
    wxString cwd = wxFileName::GetCwd();
#ifdef WXDIRDIALOG_WORKS
    if (m_InDir == wxString(""))
        m_InDir = cwd;
    wxFileName::SetCwd(m_InDir);
    wxDirDialog *dd = new wxDirDialog(this);
#else
    if (m_InDir == wxString(""))
        dd = new wxFileDialog(this, "Choose a directory",
            cwd, wxFileName(cwd).GetName(), "*", wxOPEN|wxFILE_MUST_EXIST);
    else
    {
        wxFileName::SetCwd(wxFileName(m_InDir).GetPath());
        dd = new wxFileDialog(this, "Choose a directory",
            wxFileName(m_InDir).GetPath(), wxFileName(m_InDir).GetName(), "*",
            wxOPEN|wxFILE_MUST_EXIST);
    }
#endif
    if (dd->ShowModal() != wxID_OK)
    {
        delete dd;
        return;
    }
    m_InDir = dd->GetPath();
#ifdef WXDIRDIALOG_WORKS
    path = m_InDir;
#else
    path = wxFileName(m_InDir).GetPath();
#endif
    delete dd;
    wxFileName::SetCwd(cwd);

    int series[MAX_SERIES];
    int series_done[MAX_SERIES];
    char **unrecognized_filename[MAX_SERIES];
    int max_unrecognized_names[MAX_SERIES];

    char *last_filename;
    int ii,j,k;
    int b;
    char **tmpptr;

    int nfiles=0,
        nacrnema_files=0,
        notacrnema_files=0;

    int ser;
    int new_series;

    wxDateTime time1,
        time2;
    wxTimeSpan
        time_gap,
        average_time_gap;

    wxString dir_entry;
    char *(series_uid[MAX_SERIES]), *suid,
        series_date[MAX_SERIES][10], series_time[MAX_SERIES][9];
    char series_description[MAX_SERIES][13];



    /* List all files */
    wxDir directory;
    directory.Open(path);
    if (!directory.IsOpened() )
    {
        wxMessageBox("ERROR: Could not open folder.");
        return;
    }
    SetStatusText("Searching directory...", 0);
    for (int n=0; n<MAX_SERIES; n++)
        series_uid[n] = NULL;
    if (nseries)
    {
        for (j=0; j<nseries; j++)
        {
            for (k=0; k<series_slices[j]; k++)
                delete series_filename[j][k];
            free(series_filename[j]);
        }
        nseries = 0;
    }
    else if (series_slices == NULL)
    {
        series_slices = (int *)malloc(MAX_SERIES*sizeof(int));
        series_filename =
            (wxFileName ***)malloc(MAX_SERIES*sizeof(wxFileName **));
    }

    k = 0;
    char star[2]="*";
    last_filename = star;
    bool got_first=false;
    /* For each file on the directory */
    while(got_first? directory.GetNext(&dir_entry):
        directory.GetFirst(&dir_entry, wxEmptyString, wxDIR_FILES))
    {
        got_first = true;

        /* Get information about ACRNEMA file */
        nfiles++;
        b = check_acrnema_file((const char *)path.c_str(), (const char *)dir_entry.c_str());

        /* Regular File but not ACR-NEMA */
        if(b != 0)
        {
            /* File is not an ACRNEMA file */

            notacrnema_files++;
        }
        else
        /* regular File and also ACR-NEMA */
        {
            nacrnema_files++;
            k++;

			char ser_str[16];
			if (get_element_from((const char *)path.c_str(),
					(const char *)dir_entry.c_str(),0x0020, 0x0011, AT,
					ser_str, sizeof(ser_str), &ii))
			{
				strcpy(ser_str, "0");
				ser = 0;
			}
			else
			{
				ser = atoi(ser_str);
			}
            suid = get_series_uid((const char *)path.c_str(), (const char *)dir_entry.c_str());
            if (suid)
            {
                for (j=0; j<nseries; j++)
                    if (series_uid[j] && strcmp(suid, series_uid[j])==0)
                        break;
                if (j < nseries)
                {
                    new_series = FALSE;
                    free(suid);
                }
                else
                {
                    new_series = TRUE;
                    series_uid[j] = suid;
					if (ser == 0)
					{
                    	suid = strrchr(suid, '.');
                    	ser = suid? atoi(suid+1): 0;
					}
                }
            }
            else
            {
                new_series =
                    !names_are_similar((const char *)dir_entry.c_str(), last_filename);
                ser = nseries+new_series;
                j = ser-1;
            }
    
            /* NEW SERIES */
            if(new_series == TRUE)
            {
                if (nseries == MAX_SERIES)
                {
                    wxMessageBox("ERROR: Too many series");
                    continue;
                }
                series[j] = ser;
                series_slices[j] = 0;
                unrecognized_filename[j] = (char **)malloc(256*sizeof(char *));
                series_filename[j] =
                    (wxFileName **)malloc(256*sizeof(wxFileName *));
                if (unrecognized_filename[j]==NULL ||
                        series_filename[j]==NULL)
                {
                    wxMessageBox("ERROR: Out of memory");
                    continue;
                }
                max_unrecognized_names[j] = 256;
                nseries++;
                int er = get_element_from((const char *)path.c_str(), (const char *)dir_entry.c_str(),
                    0x8, 0x103e, AT, series_description[j],
                    sizeof(series_description[j]), &ii);
                if (er!=0 && er!=235)
                    strcpy(series_description[j], "[none]");
                series_description[j][sizeof(series_description[j])-1] = 0;
                for (ii=strlen(series_description[j]);
                        ii<(int)sizeof(series_description[j])-1; ii++)
                    series_description[j][ii] = ' ';
                er=get_element_from((const char *)path.c_str(), (const char *)dir_entry.c_str(), 0x8, 0x21,
                    AT, series_date[j], sizeof(series_date[j]), &ii);
                if (er!=0 && er!=235)
                    strcpy(series_date[j], "[none]");
                series_date[j][sizeof(series_date[j])-1] = 0;
                for (ii=strlen(series_date[j]);
                        ii<(int)sizeof(series_date[j])-1; ii++)
                    series_date[j][ii] = ' ';
                er= get_element_from((const char *)path.c_str(), (const char *)dir_entry.c_str(), 0x8, 0x31,
                    AT, series_time[j], sizeof(series_time[j]), &ii);
                if (er!=0 && er!=235)
                    strcpy(series_time[j], "[none]");
                series_time[j][sizeof(series_time[j])-1] = 0;
                while (strlen(series_time[j]) < sizeof(series_time[j])-1)
                {
                    series_time[j][strlen(series_time[j])+1] = 0;
                    for (ii=strlen(series_time[j]); ii; ii--)
                        series_time[j][ii] = series_time[j][ii-1];
                    series_time[j][0] = ' ';
                }
            }
            if (series_slices[j] >= max_unrecognized_names[j])
            {
                tmpptr = (char **)realloc(unrecognized_filename[j],
                    (max_unrecognized_names[j]+256)*sizeof(char *));
                if (tmpptr == NULL)
                {
                    wxMessageBox("ERROR: Out of memory");
                    continue;
                }
                wxFileName **tmpfnptr =
                    (wxFileName **)realloc(series_filename[j],
                    (max_unrecognized_names[j]+256)*sizeof(wxFileName *));
                if (tmpfnptr == NULL)
                {
                    wxMessageBox("ERROR: Out of memory");
                    free(tmpptr);
                    continue;
                }
                max_unrecognized_names[j] += 256;
                unrecognized_filename[j] = tmpptr;
                series_filename[j] = tmpfnptr;
            }
            unrecognized_filename[j][series_slices[j]] =
                (char *)malloc(strlen((const char *)dir_entry.c_str())+1);
            if (unrecognized_filename[j][series_slices[j]] == NULL)
            {
                wxMessageBox("ERROR: Out of memory");
                continue;
            }
            strcpy(unrecognized_filename[j][series_slices[j]],
                (const char *)dir_entry.c_str());
            series_filename[j][series_slices[j]] =
                new wxFileName(path, dir_entry);
            last_filename = unrecognized_filename[j][series_slices[j]];
            series_slices[j]++;

        }
    }


    /* Get status (transmission completed or not) and number of slices */
    /* For each SERIES, find the average transmission time between slices */
    for(j=0; j<nseries; j++)
    {
        average_time_gap = time_gap = 0;
        k=0;
        time1 =
            wxFileName(path,unrecognized_filename[j][0]).GetModificationTime();

        /* For each SLICE on the Series */
        for(k=1; k<series_slices[j]; k++)
        {
            time2 = wxFileName(path,
                unrecognized_filename[j][k]).GetModificationTime();
            if (time2.IsValid())
            {
                time_gap = time2.Subtract(time1);
                average_time_gap += time_gap;
                time1 = time2;
            }
            else
            {
                average_time_gap += time_gap;
            }
        }

        if(series_slices[j] >1) {
            //gjg: average_time_gap = wxTimeSpan::Milliseconds(
            //gjg:	average_time_gap.GetMilliseconds()/(series_slices[j]-1));
                        wxLongLong  t = average_time_gap.GetMilliseconds()/(series_slices[j]-1)/1000;
            average_time_gap = wxTimeSpan::Seconds( t.ToLong() );
        } else
            //gjg: average_time_gap = wxTimeSpan::Milliseconds(0);
            average_time_gap = wxTimeSpan::Seconds(0);

        /* time1 = time of last slice on series */
        time2 = wxDateTime::Now();
        time_gap = time2.Subtract(time1);



        /* If gap between current time and last image is greater than the average gap */
        /* between transmitted images, then assume transmisison is complete. */
        if( average_time_gap.IsShorterThan(time_gap) )
            series_done[j] = 1;
        else
            series_done[j] = 0;
    }

    if (tCanvas->mListCtrl)
        delete tCanvas->mListCtrl;
    tCanvas->mListCtrl = new wxListCtrl(tCanvas, -1, wxDefaultPosition,
        wxSize(640, 360), wxLC_REPORT|wxLC_HRULES|wxLC_VRULES);

    wxListItem t_list_item;

    tCanvas->mListCtrl->InsertColumn(0, wxString("Description"),
        wxLIST_FORMAT_LEFT, 150);
    t_list_item.m_format = wxLIST_FORMAT_LEFT;
    t_list_item.m_mask = wxLIST_MASK_FORMAT;
    tCanvas->mListCtrl->SetColumn(0, t_list_item);
    tCanvas->mListCtrl->InsertColumn(1, wxString("Series"));
    tCanvas->mListCtrl->SetColumn(1, t_list_item);
    tCanvas->mListCtrl->InsertColumn(2, wxString("#of Slices"));
    tCanvas->mListCtrl->SetColumn(2, t_list_item);
    tCanvas->mListCtrl->InsertColumn(3, wxString("Series Date"));
    tCanvas->mListCtrl->SetColumn(3, t_list_item);
    tCanvas->mListCtrl->InsertColumn(4, wxString("Series Time"));
    tCanvas->mListCtrl->SetColumn(4, t_list_item);
    tCanvas->mListCtrl->InsertColumn(5, wxString("Status"));
    tCanvas->mListCtrl->SetColumn(5, t_list_item);

    /* For each SERIES, print the relevant information */
    for(j=0; j<nseries; j++)
    {
        tCanvas->mListCtrl->InsertItem(j, wxString(series_description[j]));
        tCanvas->mListCtrl->SetItem(j, 1, wxString::Format("%d",series[j]));
        tCanvas->mListCtrl->SetItem(j, 2,
            wxString::Format("%d", series_slices[j]));
        tCanvas->mListCtrl->SetItem(j, 3, wxString(series_date[j]));
        tCanvas->mListCtrl->SetItem(j, 4, wxString(series_time[j]));
        tCanvas->mListCtrl->SetItem(j, 5, wxString(
            series_done[j]? "OK": "incompl."));
    }

    SetStatusText("Done searching directory.", 0);

    for (j=0; j<nseries; j++)
    {
        for (k=0; k<series_slices[j]; k++)
            free(unrecognized_filename[j][k]);
        free(unrecognized_filename[j]);
        max_unrecognized_names[j] = 0;
    }
    for (ii=0; ii<nseries; ii++)
        if (series_uid[ii])
            free(series_uid[ii]);

}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// 6/28/07 Dewey Odhner 
void FromDicomFrame::deleteFromDicomControls(void)
{
    if (mSaveScreenControls!=NULL) { delete mSaveScreenControls;  mSaveScreenControls=NULL; }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FromDicomFrame::OnFromDicomSave(wxCommandEvent &e)
{
    if (tCanvas->mListCtrl==NULL ||
        tCanvas->mListCtrl->GetSelectedItemCount()==0)
    {
        wxMessageBox("Select series first.");
        return;
    }
    wxString defaultFile("frmdcm-tmp.IM0");
    if (tCanvas->mListCtrl->GetSelectedItemCount() == 1)
        for (int j=0; j<nseries; j++)
            if (tCanvas->mListCtrl->GetItemState(j, wxLIST_STATE_SELECTED) &&
                strncmp((const char *)tCanvas->mListCtrl->GetItemText(j).c_str(), "[none]", 6))
            {
                defaultFile = tCanvas->mListCtrl->GetItemText(j);
                defaultFile.Replace(" ", "_");
				defaultFile.Replace("/", "_");  // add by xinjian, July 6, 2009
                defaultFile += wxString(".IM0");
                break;
            }
    wxFileDialog saveDlg(this,
                _T("Save file"),
                wxEmptyString,
                defaultFile,
                _T("IM0 files (*.IM0)|*.IM0"),
                wxSAVE|wxOVERWRITE_PROMPT);
    if (saveDlg.ShowModal() == wxID_OK)
    {
		bool has_PET_info=false;
		double final_factor_bqml;
		int intercept=-1024;

		wxString cmnd = wxString( "\"" );
        cmnd += Preferences::getHome()+wxString( "/from_dicom\" " );
        for (int j=0; j<nseries; j++)
            if (tCanvas->mListCtrl->GetItemState(j, wxLIST_STATE_SELECTED))
                for (int k=0; k<series_slices[j]; k++)
                {
					int ris=100, ii;
					char buf[256];
					float av;
					if (k == 0)
					{
					  ris = get_element_from(
						(const char *)series_filename[j][k]->GetPath().c_str(),
						(const char *)series_filename[j][k]->GetFullName().
						c_str(), 0x0054, 0x0016, AT, buf, 256, &ii);
					  if (ris == 100)
					  {
					    if (get_element_from((const char *)series_filename[j][
						      k]->GetPath().c_str(), (const char *)
							  series_filename[j][k]->GetFullName().c_str(),
							  0x0008, 0x0060, AT, buf, 256, &ii)==0 &&
							  (strcmp(buf, "CT")==0||strcmp(buf, "CT ")==0) &&
							  get_element_from((const char *)series_filename[j]
							  [k]->GetPath().c_str(), (const char *)
							  series_filename[j][k]->GetFullName().c_str(),
							  0x0028, 0x1052, AN, buf, 256, &ii)==0)
						{
						  if (sscanf(buf, "%f", &av) != 1)
						    wxMessageBox("Bad rescale intercept value");
						  intercept = (int)rint(av);
						}
					  }
					}
					if (k==0 && ris!=100)
					{
					  float PatientWeight, SeriesTime,
						RadiopharmaceuticalStartTime,
						RadionuclideTotalDose, RadionuclideHalfLife,
						RadionuclidePositronFraction;
					  DicomDictionary dd=DicomDictionary(false);
					  DicomReader dr=DicomReader((const char *)
					    series_filename[j][k]->GetFullPath().c_str(), dd);
					  DicomDataElement *RST=dr.findEntry(&dr.mRoot, 0x0018,
					    0x1072);
					  DicomDataElement *RTD=dr.findEntry(&dr.mRoot, 0x0018,
					    0x1074);
					  DicomDataElement *RHL=dr.findEntry(&dr.mRoot, 0x0018,
					    0x1075);
					  DicomDataElement *RPF=dr.findEntry(&dr.mRoot, 0x0018,
					    0x1076);
					  if (RST && RST->cData && RTD && RTD->cData &&
					      RHL && RHL->cData && RPF && RPF->cData)
					   has_PET_info = get_element_from(
						(const char *)series_filename[j][k]->GetPath().c_str(),
						(const char *)series_filename[j][k]->GetFullName().
						c_str(), 0x0010, 0x1030, AN, buf, 256, &ii)==0 &&
						sscanf(buf, "%f", &PatientWeight)==1 &&
						get_element_from(
						(const char *)series_filename[j][k]->GetPath().c_str(),
						(const char *)series_filename[j][k]->GetFullName().
						c_str(), 0x0008, 0x0031, AN, buf, 256, &ii)==0 &&
						sscanf(buf, "%f", &SeriesTime)==1 &&
						sscanf(RST->cData, "%f", &RadiopharmaceuticalStartTime)
						==1 &&
						sscanf(RTD->cData, "%f", &RadionuclideTotalDose)==1 &&
						sscanf(RHL->cData, "%f", &RadionuclideHalfLife)==1 &&
						sscanf(RPF->cData, "%f", &RadionuclidePositronFraction)
						==1;
					  if (has_PET_info)
					  {
					    int days=0;
					    DicomDataElement *STD=dr.findEntry(&dr.mRoot, 0x0008,
						  0x0021);
					    DicomDataElement *RSD=dr.findEntry(&dr.mRoot, 0x0018,
						  0x1078);
						if (STD && STD->cData && RSD && RSD->cData)
						{
						  wxDateTime start_date, end_date;
						  wxString::const_iterator end;
						  if (start_date.ParseFormat(RSD->cData,
						      "%Y%m%d%H%M%S", &end) &&
							  end_date.ParseFormat(STD->cData,
							  "%Y%m%d", &end))
						  {
						    start_date.ResetTime();
							days = (end_date-start_date).GetDays();
						  }
						}
						double hours, minutes;
						hours = floor(SeriesTime/10000);
						SeriesTime -= hours*10000;
						minutes = floor(SeriesTime/100);
						SeriesTime -= minutes*100;
						if (minutes>=60 || SeriesTime>=60)
						  wxMessageBox("Invalid series time");
						minutes += hours*60;
						SeriesTime += minutes*60;
						hours = 24.*days;
						hours += floor(RadiopharmaceuticalStartTime/10000);
						RadiopharmaceuticalStartTime -= hours*10000;
						minutes = floor(RadiopharmaceuticalStartTime/100);
						RadiopharmaceuticalStartTime -= minutes*100;
						if (minutes>=60 || RadiopharmaceuticalStartTime>=60)
						  wxMessageBox(
						    "Invalid radiopharmaceutical start time");
					    minutes += hours*60;
						RadiopharmaceuticalStartTime += minutes*60;
						double total_diff_time,
						  time_factor, val_exp, activity_corr;
						total_diff_time =
						  SeriesTime-RadiopharmaceuticalStartTime;
						time_factor = total_diff_time/RadionuclideHalfLife;
						val_exp = pow(2.0, -time_factor);
						activity_corr = val_exp*RadionuclideTotalDose;
						final_factor_bqml = PatientWeight*1000/activity_corr/
						  RadionuclidePositronFraction;
					  }
					  delete RST;
					  delete RTD;
					  delete RHL;
					  delete RPF;
					}
                    cmnd += "\"";
                    cmnd += series_filename[j][k]->GetFullPath();
                    cmnd += "\" ";
                }
        cmnd += "\"";
        cmnd += saveDlg.GetPath();
        cmnd += "\"";
		if (mPatient->GetValue())
			cmnd += " -p";
        if (intercept != -1024)
            cmnd += wxString::Format(" +%d", 1024+intercept);
		wxLogMessage( "FromDicomFrame::OnFromDicomSave: " + cmnd );
        ProcessManager p( "fromDicom started", cmnd, mForeground->GetValue() );
		if (has_PET_info)
		{
			cmnd.Truncate(cmnd.Len()-(mPatient->GetValue()? 8:5));
			cmnd += "-slope.IM0\" -slopex1000";
			wxLogMessage( "FromDicomFrame::OnFromDicomSave: " + cmnd );
			ProcessManager p2 = ProcessManager( "fromDicom started", cmnd,
				mForeground->GetValue() );
			cmnd = saveDlg.GetPath();
			cmnd.Truncate(cmnd.Len()-4);
			cmnd += "_bqml.txt";
			FILE *fp=fopen((const char *)cmnd.c_str(), "wb");
			if (fp == NULL)
			{
				wxMessageBox("Could not create _bqml.txt file.");
				return;
			}
			fprintf(fp, "%.9f\n", final_factor_bqml);
			fclose(fp);
			cmnd.Truncate(cmnd.Len()-9);
			cmnd = wxString("algebra \"")+cmnd+".IM0\" \""+cmnd+
				"-slope.IM0\" \""+cmnd+"-SUV100.IM0\" "+
				wxString::Format("%.9fxy", .1*final_factor_bqml);
			wxLogMessage( "FromDicomFrame::OnFromDicomSave: " + cmnd );
			ProcessManager p3 = ProcessManager( "fromDicom started", cmnd,
				mForeground->GetValue() );
		}
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for print preview. */
void FromDicomFrame::OnPrintPreview ( wxCommandEvent& unused ) {
    // Pass two print objects: for preview, and possible printing.
    wxPrintDialogData  printDialogData( *g_printData );
    FromDicomCanvas*  canvas = dynamic_cast<FromDicomCanvas*>(mCanvas);
    wxPrintPreview*    preview = new wxPrintPreview(
        new FromDicomPrint(wxString("CAVASS").c_str(), canvas),
        new FromDicomPrint(wxString("CAVASS").c_str(), canvas),
        &printDialogData );
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_T("There was a problem previewing.\nPerhaps your current printer is not set correctly?"), _T("Previewing"), wxOK);
        return;
    }
    
    wxPreviewFrame*  frame = new wxPreviewFrame( preview, this,
        _T("CAVASS Print Preview"),
        wxPoint(100, 100), wxSize(600, 650) );
    frame->Centre( wxBOTH );
    frame->Initialize();
    frame->Show();
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** \brief callback for printing. */
void FromDicomFrame::OnPrint ( wxCommandEvent& unused ) {
    wxPrintDialogData  printDialogData( *g_printData );
    wxPrinter          printer( &printDialogData );
    FromDicomCanvas*  canvas = dynamic_cast<FromDicomCanvas*>(mCanvas);
    //FromDicomPrint       printout( _T("CAVASS:FromDicom"), canvas );
    FromDicomPrint*  printout;
    printout = new FromDicomPrint( wxString("CAVASS:FromDicom").c_str(), canvas );
    if (!printer.Print(this, printout, TRUE)) {
        if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
            wxMessageBox(_T("There was a problem printing.\nPerhaps your current printer is not set correctly?"), _T("Printing"), wxOK);
    } else {
        *g_printData = printer.GetPrintDialogData().GetPrintData();
    }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_DYNAMIC_CLASS ( FromDicomFrame, wxFrame )
BEGIN_EVENT_TABLE       ( FromDicomFrame, wxFrame )
  DefineStandardFrameCallbacks
  EVT_BUTTON( ID_INPUT,          FromDicomFrame::OnInput       )
  EVT_BUTTON( ID_SAVE,           FromDicomFrame::OnFromDicomSave  )
END_EVENT_TABLE()

wxFileHistory  FromDicomFrame::_fileHistory;
//======================================================================
