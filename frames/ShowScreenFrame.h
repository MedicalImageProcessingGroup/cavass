/*
  Copyright 1993-2009 Medical Image Processing Group
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
 * \file   ShowScreenFrame.h
 * \brief  ShowScreenFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This example loads a color 3D CAVASS save screen data file (a multiframe
 * TIFF file) and displays a single slice from it.  The user can display
 * previous and next slices.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ShowScreenFrame_h
#define __ShowScreenFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
class  ShowScreenCanvas;
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  FrameControls;

/** \brief ShowScreenFrame class definition.
 *
 *  This simple example implements a simple frame that displays one
 *  color slice from a save screen data set.
 */
class ShowScreenFrame : public MainFrame {
  protected:
    FrameControls*  mFrameControls;     ///< frame # controls
  public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_PREVIOUS=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_NEXT, ID_FRAME, ID_FRAME_SLIDER
    };
    int  mFileOrDataCount;  ///< count data/datafiles associated w/ this frame (1 for this example).
  protected:
    void initializeMenu ( void );
    void addButtonBox ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createShowScreenFrame ( wxFrame* parentFrame, bool useHistory=true );
    ShowScreenFrame ( bool maximize=false, int w=800, int h=600 );
    ~ShowScreenFrame ( void );
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
    virtual void OnMouseWheel   ( wxMouseEvent& e );
    virtual void OnNext         ( wxCommandEvent& unused );
    virtual void OnOpen         ( wxCommandEvent& unused );
    virtual void OnPrevious     ( wxCommandEvent& unused );
    virtual void OnPrint        ( wxCommandEvent& unused );
    virtual void OnPrintPreview ( wxCommandEvent& unused );
    virtual void OnSaveScreen   ( wxCommandEvent& unused );
    virtual void OnFrame        ( wxCommandEvent& unused );
    virtual void OnFrameSlider  ( wxScrollEvent& e );
#ifdef __WXX11__
    virtual void OnUpdateUIFrameSlider ( wxUpdateUIEvent& unused );
#endif
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ShowScreenFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================

