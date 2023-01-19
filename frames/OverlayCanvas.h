/*
  Copyright 1993-2013, 2015-2016 Medical Image Processing Group
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
 * \file   OverlayCanvas.h
 * \brief  OverlayCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __OverlayCanvas_h
#define __OverlayCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "misc.h"
//class OverlayCanvas : public wxScrolledWindow {
/** \brief OverlayCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class OverlayCanvas : public MainCanvas {
    int  mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z
public:
    int              mFileOrDataCount;   ///< count of files and/or data (two needed for overlay)
    wxImage**        m_images;           ///< images for displayed slices
    wxBitmap**       m_bitmaps;          ///< bitmaps for displayed slices
    int              m_tx, m_ty;         ///< translation for all images
protected:
    static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    int  lastX, lastY;
    double           m_scale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
    bool             m_overlay;          ///< toggle display overlay
    int              m_rows, m_cols;     ///< rows/cols of displayed images
	wxArrayString   *previously_visited;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    OverlayCanvas ( void );
    OverlayCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                    const wxPoint &pos, const wxSize &size );

    ~OverlayCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    /** \brief free any allocated images (of type wxImage) and/or bitmaps (of type wxBitmap) */
    void freeImagesAndBitmaps ( void ) {
        if (m_images!=NULL) {
            for (int i=0; i<m_rows*m_cols; i++) {
                if (m_images[i]!=NULL) {
                    delete m_images[i];
                    m_images[i]=NULL;
                }
            }
            free(m_images);
            m_images = NULL;
        }

        if (m_bitmaps!=NULL) {
            for (int i=0; i<m_rows*m_cols; i++) {
                if (m_bitmaps[i]!=NULL) {
                    delete m_bitmaps[i];
                    m_bitmaps[i]=NULL;
                }
            }
            free(m_bitmaps);
            m_bitmaps = NULL;
        }
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief form the composited/displayed data from both image #1's data
     *  and image #2's data.
     *  \param k the kth currently displayed slice (starting with 0 for the top, left slice)
     *  \param aData data from image #1
     *  \param bData data from image #2
     *  \returns one composited slice of rgb data (malloc'd by this function so the caller must free it)
     */
    template< typename T1, typename T2 >
    unsigned char* composite ( const int k, const T1* const aData, const T2* const bData )
    {
        unsigned char*  slice = (unsigned char*)malloc( mOverallXSize*mOverallYSize*3 );
        assert( slice!=NULL );
        memset( slice, 0, mOverallXSize*mOverallYSize*3 );

        CavassData&  A = *mCavassData;
        CavassData&  B = *(mCavassData->mNext);
        T1*  aPtr = (T1*)A.getSlice( A.m_sliceNo+k );
        T2*  bPtr = (T2*)B.getSlice( B.m_sliceNo+k );
        int YSub=0;
        int  dst=0;  //offset into result rgb data

        for (int y=0; y<mOverallYSize; y++) {
            int XSub=0;
            for (int x=0; x<mOverallXSize; x++) {
                double  red=0, green=0, blue=0;

                int  a = (YSub * A.m_xSize + XSub)*A.mSamplesPerPixel;
                if ( A.mDisplay && XSub<A.m_xSize && YSub<A.m_ySize && A.m_sliceNo+k>=0 && A.m_sliceNo+k<A.m_zSize )
                {
                    red   = A.mR * A.m_lut[ aPtr[a]-A.m_min ];
                    if (A.mSamplesPerPixel < 3)
                    {
                        green = A.mG * A.m_lut[ aPtr[a]-A.m_min ];
                        blue  = A.mB * A.m_lut[ aPtr[a]-A.m_min ];
                    }
                    else
                    {
                        green = A.mG * A.m_lut[ aPtr[a+1]-A.m_min ];
                        blue  = A.mB * A.m_lut[ aPtr[a+2]-A.m_min ];
                    }
                    a++;
                }

                int  b = (YSub * B.m_xSize + XSub)*B.mSamplesPerPixel;
                if ( B.mDisplay && XSub<B.m_xSize && YSub<B.m_ySize && B.m_sliceNo+k>=0 && B.m_sliceNo+k<B.m_zSize )
                {
                    red   += B.mR * B.m_lut[ bPtr[b]-B.m_min ];
                    if (B.mSamplesPerPixel < 3)
                    {
                        green += B.mG * B.m_lut[ bPtr[b]-B.m_min ];
                        blue  += B.mB * B.m_lut[ bPtr[b]-B.m_min ];
                    }
                    else
                    {
                        green += B.mG * B.m_lut[ bPtr[b+1]-B.m_min ];
                        blue  += B.mB * B.m_lut[ bPtr[b+2]-B.m_min ];
                    }
                    b++;
                }

                int  ir = (int)(red+0.5);
                int  ig = (int)(green+0.5);
                int  ib = (int)(blue+0.5);
                if (ir<0)    ir=0;
                if (ir>255)  ir=255;
                if (ig<0)    ig=0;
                if (ig>255)  ig=255;
                if (ib<0)    ib=0;
                if (ib>255)  ib=255;
                slice[dst]   = (unsigned char)ir;
                slice[dst+1] = (unsigned char)ig;
                slice[dst+2] = (unsigned char)ib;
                dst += 3;

                XSub++;
            }
            YSub++;
        }

        return slice;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
	bool loadNext ( void );
    void initLUT ( const int which );
    void reload ( void );
    void mapWindowToData ( int wx, int wy, int& x, int& y, int& z );
	void mapWindowToData ( int wx, int wy, int& x, int& y, int& z, int which );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnChar       ( wxKeyEvent&   e );
    void OnMouseMove  ( wxMouseEvent& e );
	void OnMouseWheel ( wxMouseEvent& e );
    void OnLeftDown   ( wxMouseEvent& e );
    void OnLeftUp     ( wxMouseEvent& e );
	void OnLeftDClick ( wxMouseEvent& e );
    void OnMiddleDown ( wxMouseEvent& e );
    void OnMiddleUp   ( wxMouseEvent& e );
    void OnRightDown  ( wxMouseEvent& e );
    void OnRightUp    ( wxMouseEvent& e );
    void OnPaint      ( wxPaintEvent& e );
    void paint        ( wxDC* dc );
    void OnSize       ( wxSizeEvent&  e );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    bool   isLoaded    ( const int which ) const;

    double getB        ( const int which ) const;
    int    getCenter   ( const int which ) const;
    double getG        ( const int which ) const;
    bool   getInvert   ( const int which ) const;
    int    getMax      ( const int which ) const;
    int    getMin      ( const int which ) const;
    int    getNoSlices ( const int which ) const;    //number of slices in entire data set
    double getR        ( const int which ) const;
    int    getSliceNo  ( const int which ) const;    //first displayed slice
    int    getWidth    ( const int which ) const;
    bool   getOverlay  ( void ) const;
    double getScale    ( void ) const;

    void   setB        ( const int which, const double b       );
    void   setCenter   ( const int which, const int    center  );
    void   setG        ( const int which, const double g       );
    void   setInvert   ( const int which, const bool   invert  );
    void   setSliceNo  ( const int which, const int    sliceNo );
    void   setWidth    ( const int which, const int    width   );
    void   setOverlay  ( const bool   overlay );
    void   setR        ( const int which, const double r       );
    void   setScale    ( const double scale );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS(OverlayCanvas)
    DECLARE_EVENT_TABLE()
};

#endif
