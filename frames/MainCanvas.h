/*
  Copyright 1993-2008 Medical Image Processing Group
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
 * \file   MainCanvas.h
 * \brief  MainCanvas class definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __MainCanvas_h
#define __MainCanvas_h

//#include  <deque>
//#include  <math.h>

#include  "wx/image.h"

//#include  "fft.h"
//#include  "BallScale.h"
#include  "misc.h"
//#include  "GScale.h"
//#include  "GBScale.h"

class CavassData;
class MainFrame;
/** \brief Definition of MainCanvas - the canvas on which images and other
 *  things are drawn (i.e., the drawing area of the window).
 */
class MainCanvas : public wxPanel {
  public:
    CavassData*      mCavassData;
    bool             m_backgroundLoaded;
    wxBitmap         m_backgroundBitmap;
  protected:
    wxImage          m_backgroundImage;  ///< background image (displayed when a new window is created)
    //wxFrame*         m_parent_frame;
    MainFrame*         m_parent_frame;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    MainCanvas ( void );
    MainCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                 const wxPoint &pos, const wxSize &size );
    ~MainCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    MainFrame* getParentFrame ( void ) {
        return m_parent_frame;
    }
    virtual void reload ( void ) {
        Refresh();
    }
    virtual void OnPaint ( wxPaintEvent& e );
    virtual void paint ( wxDC* dc );
    virtual void saveContents ( char* fname );
    virtual void appendContents ( char* fname, bool overwrite=false );
    virtual void OnSize ( wxSizeEvent& e );
    virtual void OnEraseBackground ( wxEraseEvent& event ) {
        //always ignore this event (to avoid flicker)
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( MainCanvas )
    DECLARE_EVENT_TABLE()
};

#endif
