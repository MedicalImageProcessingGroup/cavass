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
 * \file   ShowScreenCanvas.h
 * \brief  ShowScreenCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __ShowScreenCanvas_h
#define __ShowScreenCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "misc.h"

#include  "MainCanvas.h"
/** \brief the canvas on which images and other things are drawn (i.e., 
 *  the drawing area of the window).
 *
 *  This class is responsible for drawing images (typically) in the upper
 *  part of the window or frame.  The main method are the constuctors,
 *  loadFile, and OnPaint.  The set* methods (mutators) change the 
 *  appearance of the drawn images.  The get* methods (inspectors) return
 *  the values of the current settings.
 */
class ShowScreenCanvas : public MainCanvas {
    int  mXSize, mYSize, mZSize;         ///< max count of pixels of displayed images in x,y,z
public:
    int               mFileOrDataCount;  ///< count of files and/or data (1 needed for this ShowScreen)
    wxImage**         mImages;           ///< images for displayed slices
    wxBitmap**        mBitmaps;          ///< bitmaps for displayed slices
    int               mTx, mTy;          ///< translation for all images
protected:
    static const int  sSpacing;          ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    int               mLastX, mLastY;    ///< last (x,y) of mouse as it's dragged
    double            mScale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
    bool              mOverlay;          ///< toggle display overlay
    int               mRows, mCols;      ///< rows/cols of displayed images
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    ShowScreenCanvas ( void );
    ShowScreenCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                    const wxPoint &pos, const wxSize &size );

    ~ShowScreenCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    void init ( void );
    void freeImagesAndBitmaps ( void );
    void release ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh=NULL,
        const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void loadFile ( const char* const fn );
    void initLUT ( const int which );
    void reload ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnChar       ( wxKeyEvent&   e );
    void OnLeftDown   ( wxMouseEvent& e );
    void OnLeftUp     ( wxMouseEvent& e );
    void OnMiddleDown ( wxMouseEvent& e );
    void OnMiddleUp   ( wxMouseEvent& e );
    void OnMouseMove  ( wxMouseEvent& e );
    void OnMouseWheel ( wxMouseEvent& e );
    void OnPaint      ( wxPaintEvent& e );
    void OnRightDown  ( wxMouseEvent& e );
    void OnRightUp    ( wxMouseEvent& e );

    void paint        ( wxDC* dc );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool   isLoaded    ( const int which ) const;

    double getB        ( const int which ) const;
    int    getCenter   ( const int which ) const;
    double getG        ( const int which ) const;
    bool   getInvert   ( const int which ) const;
    int    getMax      ( const int which ) const;
    int    getMin      ( const int which ) const;
    int    getNoSlices ( const int which ) const;
    double getR        ( const int which ) const;
    int    getSliceNo  ( const int which ) const;    //first displayed slice
    int    getWidth    ( const int which ) const;
    bool   getOverlay  ( void ) const;
    double getScale    ( void ) const;

    void   setB        ( const int which, const double b       );
    void   setCenter   ( const int which, const int    center  );
    void   setG        ( const int which, const double g       );
    void   setInvert   ( const int which, const bool   invert  );
    void   setSliceNo  ( const int which, int          sliceNo );
    void   setWidth    ( const int which, const int    width   );
    void   setOverlay  ( const bool   overlay );
    void   setR        ( const int which, const double r       );
    void   setScale    ( const double scale );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS(ShowScreenCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
