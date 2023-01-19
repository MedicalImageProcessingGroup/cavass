/*
  Copyright 1993-2013, 2015, 2017 Medical Image Processing Group
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
 * \file   ExampleFrame.h
 * \brief  ExampleFrame definition.
 * \author George J. Grevera, Ph.D.
 *
 * This example loads a 3D CAVASS data file and displays a single slice
 * from it.  The user can display previous and next slices and can change
 * the contrast (gray map).
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ExampleFrame_h
#define __ExampleFrame_h

#include  "MainFrame.h"

#include  <algorithm>
#include  <stack>
#include  "wx/dnd.h"
#include  "wx/docview.h"
#include  "wx/splitter.h"
class  ExampleCanvas;
#if ! defined (WIN32) && ! defined (_WIN32)
    #include  <unistd.h>
#endif
#include  <stdlib.h>

class  GrayMapControls;

/** \brief ExampleFrame class definition.
 *
 *  This simple example implements a simple frame that displays one
 *  slice from a data set.
 <p>
  This example loads a 3D CAVASS data file and displays a single slice
  from it.  The user can display previous and next slices and can change
  the contrast (gray map).  The user may also drag the image around the 
  canvas as well (to illustrate some interaction with mouse events).
  A frame (window) consists of a menu bar, a canvas (drawing area), and
  a control panel area towards the bottom of the frame.
  This example consists of the following files:
    ExampleFrame.cpp,  ExampleFrame.h, ExampleCanvas.cpp, and ExampleCanvas.h.
  This example also uses the CavassData class to read the image data.
  CavassData is a general wrapper class that one may use to read various
  image data file formats such as 3DVIEWNIX (.BIM, .IM0), DICOM (.dcm,
  .dicom), JPEG (.jpg, .jpeg), TIFF (.tif, .tiff), PNG (.png), VTK (.vtk),
  and GIF (.gif).

  CavassData may also be initialized directly from memory, rather
  than from by reading a file, as well.
 </p>
 <p>
  Our convention is that an <em>app</em> or <em>module</em> consists
  of an <em>App</em>Frame class and an <em>App</em>Canvas class.
  All of the *Frame classes are subclasses of MainFrame.
  *Frame classes are the main app frame (window) and consist of 3 regions:
  (a) menu bar, (b) drawing area (canvas), and a control are towards the
  bottom third of the window.
  All of the *Canvas classes are subclasses of MainCanvas.
  *Canvas classes perform drawing and handle mouse move and click events.
 </p>
 <p>
  Other CAVASS apps/modules consist of Overlay, SurfView, and Montage.
  Overlay files are
      OverlayFrame.cpp,  OverlayFrame.h,
      OverlayCanvas.cpp, OverlayCanvas.h, and
      OverlayPrint.h.
  SurfView files are
      SurfViewFrame.cpp,  SurfViewFrame.h,
      SurfViewCanvas.cpp, SurfViewCanvas.h, and
      SurfViewPrint.h.
  Montage files are:
      MontageFrame.cpp,  MontageFrame.h,
      MontageCanvas.cpp, MontageCanvas.h, and
      MontagePrint.h.
  \todo These other modules need to be updated to be in accord with the Example app.
 </p>
 <p>
  The standard controls (that one is encouraged to use) are
  CineControls (in CineControls.h),
  ColorControls (in ColorControls.h),
  GrayMapControls (in GrayMapControls.h),
  SaveScreenControls (in SaveScreenControls.h),
  and SetIndexControls (in SetIndexControls.h).
  This set is expected to expand and reuse and standardization is
  encouraged.
  This example illustrates GrayMapControls, and SaveScreenControls.
  Implementation of the callbacks are, of course, the responsibility
  of the app but defining the controls in this manner allows us to
  standardize the look-and-feel across all apps.
 </p>
 <p>
  Once the frame, canvas, and controls have been coded for an app,
  one must add the new app to the main, standard CAVASS application menu
  bar.  In this example, we'll add the Example app to the Help menu.
 </p>
 <ol>
  <li> Add ID_EXAMPLE to the enum in MainFrame.h (so it can be added to the
       menu bar).
       <pre>
       </pre> </li>
  <li> Add the following callback definition to MainFrame.h (this is the
       callback when Example is chosen from the menu bar):
       <pre>
           void OnExample ( wxCommandEvent& e );
       </pre> </li>
  <li> Add the following to the DefineStandardFrameCallbacks macro in
       MainFrame.h (to associate the ID with the callback function):
       <pre>
           EVT_MENU( ID_EXAMPLE,              MainFrame::OnExample      )    \
       </pre> </li>
  <li> Add the following to MainFrame.cpp (this is the callback function that
       will be called when Example is chosen from the menu bar):
       <pre>
           void MainFrame::OnExample ( wxCommandEvent& unused ) {
               ExampleFrame::createExampleFrame( this );
           }
       </pre>
       Using ExampleFrame::createExampleFrame() in this manner allows one 
       to customize file selection code.  For example, overlay requires
       two files so the OnOverlay method asks the user to select two files.
       Or you may wish to change the various file types that can be used as
       input to your particular app.
       <pre>
       </pre>
  </li>
  <li> Add the following to MainFrame.cpp:
       <pre>
           #include  "ExampleFrame.h"
       </pre> </li>
  <li> Add the following to MainFrame::initializeMenu() in MainFrame.cpp:
       <pre>
           help_menu->Append( ID_EXAMPLE, "&Example" );
       </pre> </li>
 </ol>
 <h3> A note about callbacks... </h3>
 <p>
  The macro, DefineStandardFrameCallbacks, is defined in MainFrame.h.  It is
  used to define standard callback table entries (so they need not be
  repeated in each app).  Callback functions are defined as being virtual
  so they can be completely replaced via inheritance (when necessary).
 </p>
 */
class ExampleFrame : public MainFrame {
  protected:
    GrayMapControls*  mGrayMapControls;     ///< gray map controls
  public:
    /** \brief additional ids for buttons, sliders, etc. */
    enum {
        ID_PREVIOUS=ID_LAST,  ///< continue from where MainFrame.h left off
        ID_NEXT,
        ID_GRAYMAP, ID_CENTER_SLIDER, ID_WIDTH_SLIDER, ID_INVERT,
        ID_CT_LUNG,        ID_CT_SOFT_TISSUE, ID_CT_BONE, ID_PET
    };
    int  mFileOrDataCount;  ///< count data/datafiles associated w/ this frame (1 for this example).
  protected:
    void initializeMenu ( void );
    void addButtonBox ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    static void createExampleFrame ( wxFrame* parentFrame, bool useHistory=true );
    ExampleFrame ( bool maximize=false, int w=800, int h=600 );
    ~ExampleFrame ( void );
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
    virtual void OnGrayMap         ( wxCommandEvent& unused );
    virtual void OnInput           ( wxCommandEvent& unused );
    virtual void OnMouseWheel      ( wxMouseEvent& e );
    virtual void OnNext            ( wxCommandEvent& unused );
    virtual void OnOpen            ( wxCommandEvent& unused );
    virtual void OnPrevious        ( wxCommandEvent& unused );
    virtual void OnPrint           ( wxCommandEvent& unused );
    virtual void OnPrintPreview    ( wxCommandEvent& unused );
    virtual void OnSaveScreen      ( wxCommandEvent& unused );

    virtual void OnCenterSlider ( wxScrollEvent& e );
    virtual void OnWidthSlider  ( wxScrollEvent& e );
	void OnCTLung       ( wxCommandEvent& unused );
	void OnCTSoftTissue ( wxCommandEvent& unused );
	void OnCTBone       ( wxCommandEvent& unused );
    void OnPET          ( wxCommandEvent& unused );
#ifdef __WXX11__
    virtual void OnUpdateUICenterSlider ( wxUpdateUIEvent& unused );
    virtual void OnUpdateUIWidthSlider  ( wxUpdateUIEvent& unused );
#endif
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( ExampleFrame )
    DECLARE_EVENT_TABLE()
};

#endif
//======================================================================
