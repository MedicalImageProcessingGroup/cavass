/*
  Copyright 1993-2012, 2016, 2018 Medical Image Processing Group
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
 * \file   SurfViewCanvas.h
 * \brief  SurfViewCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __SurfViewCanvas_h
#define __SurfViewCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  "misc.h"
#include  "render/cvRender.h"

class  RenderThread;
class  RenderingEventData;
typedef struct {double angle[3]; int views;} Key_pose;

//class SurfViewCanvas : public wxScrolledWindow {
/** \brief SurfViewCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class SurfViewCanvas : public MainCanvas {
public:
    const bool       manip_flag;
    int              mFileOrDataCount;   ///< count of files and/or data (two needed for overlay)
    wxImage**        m_images;           ///< images for displayed slices
    wxBitmap**       m_bitmaps;          ///< bitmaps for displayed slices
    int              m_tx, m_ty;         ///< translation for all images
    double           mRotatingFrom[3];   ///< start of rotation
    int              mOverallXSize, mOverallYSize, mOverallZSize;
                                         ///< max count of pixels of displayed images in x,y,z
    int              mInterruptRenderingFlag;  ///< true=interrupt rendering; false=render
    cvRenderer      *mRenderer;

    enum { VIEW=SECTION+1, MEASURE, ROI_STATISTICS, CREATE_MOVIE_TUMBLE,
        PREVIOUS_SEQUENCE, SELECT_SLICE, REFLECT, CUT_PLANE, CUT_CURVED, MOVE,
        ID_CINE_TIMER=MOVE+100};
    int              mWhichMode;
    int              mRotateMode;        ///< layout vs. one of the rotate modes (values below)
    enum { layoutMode, rxMode, ryMode, rzMode, roMode, toMode };
    bool             mRotateOn;
    int              mControlState;
    int              mLastRotateMode;
    double           mRotateAngle;
    double           mLineDX, mLineDY;
    double           cut_depth;
    int              mLinePixelCount;
    X_Point         *mLinePixels;
    double           first_loc, last_loc;
    double           out_pixel_size;
    int              key_poses;
    Key_pose        *key_pose;
    int              key_pose_array_size;
    bool             preview_all_poses;
    bool             preview_icon_mode;
    int              preview_key_pose;
    int              preview_view;
    int              preview_frame_displayed;
    wxTimer         *m_cine_timer;
    double           total_density;
    double           mean_density;
    double           standard_deviation;
    double           min_density;
    double           max_density;
    bool             stats_valid;
protected:
    X_Point         *vertices;
    int              nvertices;
    bool             inside;
    static const int sSpacing;           ///< space, in pixels, between each slice (on the screen)
    //when in plain old move mode:
    int  lastX, lastY;
    double           m_scale;            ///< scale for both directions
                                         ///< \todo make scale independent in each direction
    bool             m_overlay;          ///< toggle display overlay
    int              m_rows, m_cols;     ///< rows/cols of displayed images

	bool             m_MIP_Invert;

    RenderThread*    mRenderThread;      ///< rendering thread

    wxCriticalSection      mCriticalSection;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    SurfViewCanvas ( void );
    SurfViewCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
                     const wxPoint &pos, const wxSize &size, bool manipulate );

    ~SurfViewCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    /** \brief free any allocated images (of type wxImage) and/or bitmaps (of type wxBitmap) */
    void freeImagesAndBitmaps ( void ) {
        wxCriticalSectionLocker  lock( mCriticalSection );  //critical section
                                                            //vvvvvvvvvvvvvvvv
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
    unsigned char* composite ( const int k, const T1* const aData, const T2* const bData ) {
        unsigned char*  slice = (unsigned char*)malloc( mOverallXSize*mOverallYSize*3 );
        assert( slice!=NULL );

        const CavassData&  A = *mCavassData;
        const CavassData&  B = *(mCavassData->mNext);
        int  a=-1;  //offset into a data
        int  b=-1;  //offset into b data
        if (A.m_sliceNo+k < A.m_zSize)    a = (A.m_sliceNo+k) * A.m_xSize * A.m_ySize;
        if (B.m_sliceNo+k < B.m_zSize)    b = (B.m_sliceNo+k) * B.m_xSize * B.m_ySize;
        int  dst=0;  //offset into result rgb data
        for (int y=0; y<mOverallYSize; y++) {
            for (int x=0; x<mOverallXSize; x++) {
                double  red=0, green=0, blue=0;
                if ( A.mDisplay && x<A.m_xSize && y<A.m_ySize && A.m_sliceNo+k>=0 && A.m_sliceNo+k<A.m_zSize )
                {
                    red   = A.mR * A.m_lut[ aData[a]-A.m_min ];
                    green = A.mG * A.m_lut[ aData[a]-A.m_min ];
                    blue  = A.mB * A.m_lut[ aData[a]-A.m_min ];
                    a++;
                }
                if ( B.mDisplay && x<B.m_xSize && y<B.m_ySize && B.m_sliceNo+k>=0 && B.m_sliceNo+k<B.m_zSize )
                {
                    red   += B.mR * B.m_lut[ bData[b]-B.m_min ];
                    green += B.mG * B.m_lut[ bData[b]-B.m_min ];
                    blue  += B.mB * B.m_lut[ bData[b]-B.m_min ];
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
                if (ib>255)  ig=255;
                slice[dst]   = (unsigned char)ir;
                slice[dst+1] = (unsigned char)ig;
                slice[dst+2] = (unsigned char)ib;
                dst += 3;
            }
        }

        return slice;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void release ( void );
    void ensureRenderThreadRunning ( void );
    void selectObject(long wx, long wy, int xpos2, long timestamp);
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh=NULL,
        const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void loadFile ( const char* const fn );
    void loadFiles ( wxArrayString &filenames );
    void initLUT ( const int which );
    void reload    ( unsigned char* slice, unsigned char *slice2,
                     int overallXSize, int overallYSize );
    //void reloadNow ( unsigned char* slice, unsigned char *slice2 );
    void reloadNow ( RenderingEventData* re );
    void mapWindowToData ( int wx, int wy, int& x, int& y, int& z );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnChar       ( wxKeyEvent&   e );
    void OnDoRender   ( wxCommandEvent& e );
    void OnMagnify    ( double scale );
    void OnMouseMove  ( wxMouseEvent& e );
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
    bool   isLoaded    ( const int which ) const;

    double getB        ( const int which );
    int    getCenter   ( const int which ) const;
    double getG        ( const int which ) const;
    bool   getInvert   ( const int which ) const;
    int    getMax      ( const int which ) const;
    int    getMin      ( const int which ) const;
    int    getNoSlices ( const int which ) const;    //number of slices in entire data set
    double getR        ( const int which ) const;
    int    getSliceNo  ( const int which ) const;    //first displayed slice
    int    getWidth    ( const int which ) const;
    bool   getSurfView ( void ) const;
    double getScale    ( void ) const;

    void   setB        ( const int which, const double b       );
    void   setCenter   ( const int which, const int    center  );
    void   setG        ( const int which, const double g       );
    void   setInvert   ( const int which, const bool   invert  );
    void   setSliceNo  ( const int which, const int    sliceNo );
    void   setWidth    ( const int which, const int    width   );
    void   setSurfView ( const bool overlay );
    void   setR        ( const int which, const double r       );
    void   setAmbientLight ( int r, int g, int b );
    void   setAntialias( bool state );
	void   setMIP      ( bool state );
	void   setMIP_Invert ( bool state );
    void   setBackgroundColor ( int r, int g, int b );
    void   setObjectColor ( int r, int g, int b, int whichObject );
    void   setObjectDiffuse ( double percent, double exponent, double divisor, int whichObject );
    void   setObjectOpacity ( double opacity, int whichObject );
    void   setObjectSpecular ( double percent, double exponent, double divisor, int whichObject );
	void   setAllDiffuse ( double percent, double exponent, double divisor );
	void   setAllSpecular ( double percent, double exponent, double divisor );
    void   rerender    ( void );
    void   setLayout   ( bool state ) {
        if (state)    mRotateOn = 0;
        else          mRotateOn = 1;
    }
    void   setObject   ( int );
    void   setStatus   ( bool );
    void   setMode     ( int );
    void   setRotate   ( bool on );
    void   setMobile   ( bool mobile );
    void   setBox      ( bool on );
    void   removeObject( void );
    void   setSceneStatus( bool on );
    void   setActionText();
    void   save_movie  ( const char path[] );
    void   OnCineTimer ( wxTimerEvent& e );
    void   reset       ( void );
    void   setMaterialOpacity( double opacity, int whichMaterial );
    void   setMaterialThresholds( double th1, double th2, int whichMaterial );
    void   setMaterialColor( int r, int g, int b, int whichMaterial );
    void   setSurfStrength( double surfStr, double emisPwr, double surfPctPwr);
	void   displayGrayMapControls( void );
	void   setGrayMap( const int which, const int center, const int width,
	           const int invert );
	void   nextSlice   ( void );
	void   previousSlice( void );

    void   handleStereo ( char* rendered );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS(SurfViewCanvas)
    DECLARE_EVENT_TABLE()
};


double getObjectDiffuseFraction ( cvRenderer *r, int n );
void   setObjectDiffuseFraction ( cvRenderer *r, double v, int n );

#endif
