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
 * \file   ImportFrame.h
 * \brief  ImportFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This Import loads a 3D CAVASS data file and displays a single slice
 * from it.  The user can display previous and next slices and can change
 * the contrast (gray map).
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ImportFrame_h
#define __ImportFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
class  ImportCanvas;
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

enum {
    IMPORT_MATHEMATICA, IMPORT_MATLAB, IMPORT_R, IMPORT_VTK
};

class  GrayMapControls;

/** \brief ImportFrame class definition.
 */
class ImportFrame : public MainFrame {
  protected:
    wxTextCtrl*  mXSize;
    wxTextCtrl*  mYSize;
    wxTextCtrl*  mZSize;
    wxTextCtrl*  mXSpace;
    wxTextCtrl*  mYSpace;
    wxTextCtrl*  mZSpace;
    int          mFrameType;
    wxString     mFileName;
public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_INPUT_BUTTON=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_SAVE_BUTTON, ID_XSIZE, ID_YSIZE, ID_ZSIZE, ID_XSPACE, ID_YSPACE, ID_ZSPACE
    };
    int  mFileOrDataCount;  ///< count data/datafiles associated w/ this frame (1 for this example).
  protected:
    void initializeMenu ( void );
    void addButtonBox ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createImportFrame ( wxFrame* parentFrame, int frame_type );
    ImportFrame ( bool maximize=false, int w=800, int h=600, int frame_type=IMPORT_MATLAB );
    ~ImportFrame ( void );
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
    virtual void OnInput        ( wxCommandEvent& unused );
    virtual void OnOpen         ( wxCommandEvent& unused );
    virtual void OnPrint        ( wxCommandEvent& unused );
    virtual void OnPrintPreview ( wxCommandEvent& unused );
    virtual void OnSave         ( wxCommandEvent& unused );
    virtual void OnSaveScreen   ( wxCommandEvent& unused );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ImportFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
