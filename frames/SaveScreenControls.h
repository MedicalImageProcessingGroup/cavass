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
 * \file   SaveScreenControls.h
 * \brief  Definition and implementation of SaveScreenControls class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SaveScreenControls_h
#define __SaveScreenControls_h

/**
 * \brief Definition and implementation of SaveScreenControls class.
 *
 * Save screen controls consist of a control box with the name of the
 * save screen file, an append button, an overwrite button, and a browse
 * button.
 *
 * Note: Callbacks are handled by the caller.
 */
class SaveScreenControls {
    wxButton*          mAppend;        ///< append button
    wxSizer*           mBottomSizer;   //DO NOT DELETE in dtor!
    wxButton*          mBrowse;        ///< browse for a screen file button
    wxStaticBox*       mSaveScreenBox;
    wxStaticBoxSizer*  mSaveScreenSizer;
    wxFlexGridSizer*   mFgs;
    wxStaticText*      mFileNameST;    ///< save screen file name caption
    wxTextCtrl*        mFileNameTC;    ///< actual save screen file name
    wxButton*          mOverwrite;     ///< overwrite button
public:
    /** \brief SaveScreenControls ctor.
     *  \param cp is the parent panel for these controls.
     *  \param bottomSizer is the existing sizer to which this the 
     *         sizer associated with these controls will be added
     *         (and deleted upon destruction).
     *  \param appendID    is the ID assigned to the append button.
     *  \param browseID    is the ID assigned to the browse button.
     *  \param overwriteID is the ID assigned to the overwrite button.
     */
    SaveScreenControls ( wxPanel* cp, wxSizer* bottomSizer,
        const int appendID, const int browseID, const int overwriteID )
    {
        mBottomSizer = bottomSizer;
        mSaveScreenBox = new wxStaticBox( cp, -1, "SaveScreen" );
        ::setColor( mSaveScreenBox );
        mSaveScreenSizer = new wxStaticBoxSizer( mSaveScreenBox, wxHORIZONTAL );
        mFgs = new wxFlexGridSizer( 2, 5, 5 );  //2 cols,vgap,hgap
        mFgs->SetMinSize( controlsWidth, 0 );
        mFgs->AddGrowableCol( 0 );
        //output file name
        mFileNameST = new wxStaticText( cp, -1, "output file name:" );
        ::setColor( mFileNameST );
        mFgs->Add( mFileNameST, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL );

        mFileNameTC = new wxTextCtrl( cp, -1, Preferences::getSaveScreenFileName() );
        ::setColor( mFileNameTC );
        mFgs->Add( mFileNameTC, 0, wxGROW );

        mBrowse = new wxButton( cp, browseID, "Browse", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( mBrowse );
        mFgs->Add( mBrowse, 0, wxGROW|wxLEFT|wxRIGHT );

        mAppend = new wxButton( cp, appendID, "Append", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( mAppend );
        mFgs->Add( mAppend, 0, wxGROW|wxLEFT|wxRIGHT );

        mFgs->AddStretchSpacer();

        mOverwrite = new wxButton( cp, overwriteID, "Overwrite", wxDefaultPosition, wxSize(buttonWidth,buttonHeight) );
        ::setColor( mOverwrite );
        mFgs->Add( mOverwrite, 0, wxGROW|wxLEFT|wxRIGHT );

        mSaveScreenSizer->Add( mFgs, 0, wxGROW|wxALL, 10 );

        #ifdef wxUSE_TOOLTIPS
            #ifndef __WXX11__
                mAppend->SetToolTip(    "add screen to end of existing file"     );
                mBrowse->SetToolTip(    "browse folders for a screen file"       );
                mOverwrite->SetToolTip( "erase any existing file and add screen" );
            #endif
        #endif
        mBottomSizer->Prepend( mSaveScreenSizer, 0, wxGROW|wxALL, 10 );
        mBottomSizer->Layout();
        cp->Refresh();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief return the save screen file name. */
    wxString getFileName ( void ) {  return mFileNameTC->GetValue();  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief change the displayed save screen file name. */
    void setFileName ( wxString s ) {  mFileNameTC->SetValue( s );  }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief SaveScreenControls dtor. */
    ~SaveScreenControls ( void ) {
        mBottomSizer->Remove( mSaveScreenSizer );
        delete mAppend;
        delete mBrowse;
        //delete mSaveScreenBox;
        //delete mSaveScreenSizer;
        //delete mFgs;
        delete mFileNameST;
        delete mFileNameTC;
        delete mOverwrite;
    }
};

#endif
