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
 * \file   InterpolateFrame.h
 * \brief  InterpolateFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This module loads a 3D or 4D CAVASS data file, displays a single slice
 * from it, and allow the user to specify interpolation parameters (before
 * allowing the user to run the interpolation program).
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __InterpolateFrame_h
#define __InterpolateFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
#include  "InterpolateCanvas.h"
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

/** \brief InterpolateFrame class definition.
 *
 *  This module is the graphical interface to the ndinterpolate command.
 <p>
  This module loads a 3D CAVASS data file and displays a single slice from
  it.  The user can display previous and next slices.  The user may also 
  drag the image around the canvas as well.  This module consists of the 
  following files:
    InterpolateFrame.cpp,  InterpolateFrame.h, InterpolateCanvas.cpp,
    and InterpolateCanvas.h.
  This module uses the CavassData class to read the image data.
 </p>
 */
class InterpolateFrame : public MainFrame {
  public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_MIN_VALUE=ID_LAST,    ///< continue from where MainFrame.h left off
        ID_MAX_VALUE, ID_X1_VALUE, ID_X2_VALUE, ID_X3_VALUE,
        ID_X4_VALUE, ID_X1_CHOICE, ID_X2_CHOICE, ID_X3_CHOICE, ID_X4_CHOICE,
        ID_DISTANCE, ID_EXTRAPOLATE, ID_OUTPUT, ID_SAVE,
        ID_FOREGROUND, ID_PARALLEL
    };
    int  mFileOrDataCount;  ///< count data/datafiles used (1 for this example).
  protected:
    wxTextCtrl*  mMinValue;  ///< first slice
    wxTextCtrl*  mMaxValue;  ///< last slice

    wxTextCtrl*  mX1Value;   ///< spacing in first direction
    wxTextCtrl*  mX2Value;   ///< spacing in second direction
    wxTextCtrl*  mX3Value;   ///< spacing in third direction
    wxTextCtrl*  mX4Value;   ///< spacing in fourth direction

    wxComboBox*  mX1Choice;  ///< interpolation method for first direction
    wxComboBox*  mX2Choice;  ///< interpolation method for second direction
    wxComboBox*  mX3Choice;  ///< interpolation method for third direction
    wxComboBox*  mX4Choice;  ///< interpolation method for fourth direction

    wxComboBox*  mDistanceChoice;  ///< distance metric choices

    wxTextCtrl*  mOutput;    ///< output file name
    wxComboBox*  mDistance;  ///< distance transform method
    wxCheckBox*  mExtrapolate;  ///< extrapolate flag
    wxCheckBox*  mForeground;   ///< run-in-foreground flag
    wxCheckBox*  mParallel;  ///< run-in-parallel flag

    void initializeMenu ( void );
    void addButtonBox ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createInterpolateFrame ( wxFrame* parentFrame, bool useHistory=true );
    InterpolateFrame ( bool maximize=false, int w=800, int h=600 );
    ~InterpolateFrame ( void );
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
    virtual void OnInput           ( wxCommandEvent& unused );
    virtual void OnOpen            ( wxCommandEvent& unused );
    virtual void OnPrint           ( wxCommandEvent& unused );
    virtual void OnPrintPreview    ( wxCommandEvent& unused );
    virtual void OnSave            ( wxCommandEvent& unused );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( InterpolateFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================

