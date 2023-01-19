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
 * \file   FromDicomCanvas.h
 * \brief  FromDicomCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __FromDicomCanvas_h
#define __FromDicomCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "MainCanvas.h"
#include  "misc.h"

//class FromDicomCanvas : public wxScrolledWindow {
/** \brief FromDicomCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class FromDicomCanvas : public MainCanvas {
    int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z

public:
	wxListCtrl *mListCtrl;

    FromDicomCanvas ( void );
    FromDicomCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                      const wxPoint &pos, const wxSize &size );

    ~FromDicomCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void release ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	void SetStatusText(const wxString& text, int number)
	{
		m_parent_frame->SetStatusText(text, number);
	}

  public:
    void reload ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnChar       ( wxKeyEvent&   e );
    void OnLeftDown   ( wxMouseEvent& e );
    void OnLeftUp     ( wxMouseEvent& e );
    void OnMiddleDown ( wxMouseEvent& e );
    void OnMiddleUp   ( wxMouseEvent& e );
    void OnRightDown  ( wxMouseEvent& e );
    void OnRightUp    ( wxMouseEvent& e );
    void OnPaint      ( wxPaintEvent& e );
    void paint        ( wxDC* dc );
    void OnSize       ( wxSizeEvent&  e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	DECLARE_DYNAMIC_CLASS(FromDicomCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
