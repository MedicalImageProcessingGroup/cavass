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
 * \file   DicomInfoFrame.h
 * \brief  Create a window containing DICOM header information.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __DicomInfoFrame_h
#define __DicomInfoFrame_h

#include  "wx/treectrl.h"
#include  <sstream>
#include  "Dicom.h"

using namespace std;

/**
 * \brief Class definition and implementaion (to create a window
 *        containing DICOM header information).
 */
class DicomInfoFrame : public wxFrame {
public:
    /**
     * \brief constructor
     * \param dr    reader instance
     * \param name  name associated with this reader instance
     */
    DicomInfoFrame ( DicomReader& dr, const char* const name )
        : wxFrame( NULL, -1, _T("DICOM info"), wxPoint(5,5),
        wxSize(400,200) )
    {
        wxTreeCtrl*  tree = new wxTreeCtrl( this, -1, wxDefaultPosition,
            wxDefaultSize,
            wxTR_DEFAULT_STYLE | wxTR_HAS_VARIABLE_ROW_HEIGHT | wxSUNKEN_BORDER );
        wxTreeItemId  root = tree->AddRoot( name );
        createTree( tree, root, dr.mRoot.mChild );
        tree->Expand( root );
        Show();
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
private:
    /**
     * \brief recursively create a tree control containing DICOM
     *        information.
     * \param tree
     * \param parent
     * \param node  root of DicomDataElement subtree
     * \returns nothing
     */
    void createTree ( wxTreeCtrl* tree, const wxTreeItemId parent,
                      const DicomDataElement* const node )
    {
        //base case for recursion
        if (node==NULL)    return;
        //convert the DicomDataElement to a string
        ostringstream  oss;
        oss << *node;
        string  s = oss.str();
        //remove any and all trailing \n's or \r's
        for ( ; ; ) {
            if (s.length() < 1)    break;
            if (s[s.length()-1] == '\n')    s[s.length()-1] = 0;
            else if (s[s.length()-1] == '\r')    s[s.length()-1] = 0;
            else  break;
        }
        //add this item to the tree
        wxTreeItemId  temp = tree->AppendItem( parent, (const char *)s.c_str() );
        //add children to the tree
        if (node->mChild!=NULL)      createTree( tree, temp,   node->mChild );
        //add siblings to the tree
        if (node->mSibling!=NULL)    createTree( tree, parent, node->mSibling );
        tree->Expand( temp );
    }

};

#endif
