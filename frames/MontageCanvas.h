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
 * \file   MontageCanvas.h
 * \brief  MontageCanvas definition.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef __MontageCanvas_h
#define __MontageCanvas_h

#include  <deque>
#include  <math.h>
#include  "wx/image.h"
#include  "CavassData.h"
#include  <vector>

/** \brief MontageCanvas - the canvas on which images and other things
 *  are drawn (i.e., the drawing area of the window).
 */
class MontageCanvas : public MainCanvas {
public:
    wxImage**   m_images;    ///< images for displayed slices
    wxBitmap**  m_bitmaps;   ///< bitmaps for displayed slices
    int         m_tx, m_ty;  ///< translation for all images
protected:
    static const int  sSpacing;  ///< space, in pixels, between each slice
	bool  mInterpolate;  ///< true=interpolate; false=nearest-neighbor
    // - - - - - - - - - -
	/** \brief this class contains pairs of points in slices that the user
	 *  wishes to measure (by clicks).
	 */
	class Measure {
		friend class MontageCanvas;
		static int  sCount;       ///< used to generated a unique id (sequential #) for each instance
	public:
		int     mID;              ///< unique id
		int     mDataset;         ///< indicates which dataset is measured by this entry
        //first point
		bool    mFirstSpecified;  ///< has the first point been specified?
		double  mX1,  mY1,  mZ1;  ///< physical location of first point
		int     mPX1, mPY1, mPZ1; ///< subscript of first point
		int     mValue1;          ///< first point's gray value
		//second point
		bool    mSecondSpecified; ///< has the second point been specified?
		double  mX2,  mY2,  mZ2;  ///< physical location of second point
		int     mPX2, mPY2, mPZ2; ///< subscript of second point
		int     mValue2;          ///< second point's gray value
	private:
		double  mDistance;  ///< distance between points
		double  mAngle;     ///< angle between points

		void init ( void ) {
			mID = sCount;
			sCount += 2;
			mDataset = 0;

			mX1  = mY1  = mZ1  = 0.0;
			mPX1 = mPY1 = mPZ1 = mValue1 = 0;

			mX2  = mY2  = mZ2  = 0.0;
			mPX2 = mPY2 = mPZ2 = mValue2 = 0;
			mDistance = mAngle = 0.0;
			mFirstSpecified  = false;
			mSecondSpecified = false;
		}

		Measure ( void ) {
			init();
		}
	public:
		Measure ( int dataset, double x1, double y1, double z1,
			int px1, int py1, int pz1, int value1 )
		{
			init();
			mDataset = dataset;
			mX1  = x1;   mY1  = y1;   mZ1  = z1;
			mPX1 = px1;  mPY1 = py1;  mPZ1 = pz1;
			mValue1 = value1;
			mFirstSpecified = true;
		}

		void setSecondPoint ( double x2, double y2, double z2,
			int px2, int py2, int pz2, int value2 )
		{
			assert( mFirstSpecified );
			mX2  = x2;   mY2  = y2;   mZ2  = z2;
			mPX2 = px2;  mPY2 = py2;  mPZ2 = pz2;
			mValue2 = value2;
			mSecondSpecified = true;

			//calculate the angle
			if (mPZ1==mPZ2)
				mAngle = atan2( mY2-mY1, mX2-mX1 );
			else {
				double  sum  = mX1*mX2 + mY1*mY2 + mZ1*mZ2;
				double  mag1 = sqrt( mX1*mX1 + mY1*mY1 + mZ1*mZ1 );
				double  mag2 = sqrt( mX2*mX2 + mY2*mY2 + mZ2*mZ2 );
				mAngle = acos( sum / (mag1*mag2) );
			}

			//calculate the distance
			mDistance = sqrt( (mX1-mX2)*(mX1-mX2) + (mY1-mY2)*(mY1-mY2) + (mZ1*mZ2) );
		}

		double getAngle ( void ) const {
			assert( mFirstSpecified && mSecondSpecified );
			return mAngle;
		}

		double getDistance ( void ) const {
			assert( mFirstSpecified && mSecondSpecified );
			return mDistance;
		}
	};
	typedef  std::vector< Measure >  MeasureVector;
    // - - - - - - - - - -
	class Magnify {
	public:
		int        mDataset, mSlice;  //dataset and slice number
		double     mScale;            //scale (magnification factor)
		int        mSX, mSY;          //screen position corresponding to center of slice
		wxImage*   mImage;
		wxBitmap*  mBitmap;
		MeasureVector  mMeasureVector;  //just in case the user wishes to measure in a magnified image

		Magnify ( int dataset, int slice, double scale, int sx, int sy, wxImage* image, wxBitmap* bitmap )
		{
		    mDataset = dataset;
			mSlice   = slice;
		    mScale   = scale;
		    mSX      = sx;
			mSY      = sy;
			mImage   = image;
			mBitmap  = bitmap;
		}
	};
	typedef  std::vector< Magnify >  MagnifyVector;
    // - - - - - - - - - -
	MagnifyVector  mMagnifyVector;
	int            mMagnifyState;
	enum { MagnifyStateChoose, MagnifyStatePosition, MagnifyStateSize };

	MeasureVector  mMeasureVector;

    int  mMaxPixelWidth;     ///< scaled largest pixel width
    int  mMaxPixelHeight;    ///< scaled largest pixel height
    int  mMaxSlices;         ///< max no of slices for all data sets

    //when in plain old move mode:
    int  lastX, lastY;
    int  m_sliceNo;          ///< slice # of first displayed slice
    int  m_rows, m_cols;     ///< rows/cols of displayed images
    int  mFileOrDataCount;   ///< count data/datafiles associated w/ this canvas
    int  mFirstDisplayedDataset;  ///< # of first displayed dataset
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
    MontageCanvas ( void );
    MontageCanvas ( wxWindow* parent, MainFrame* parent_frame, wxWindowID id,
        const wxPoint &pos, const wxSize &size );

    ~MontageCanvas ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  protected:
    void handleMagnifyMode ( wxMouseEvent& e );
    void handleMeasureMode ( wxMouseEvent& e );
    void init ( void );
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
    void release ( void );

    CavassData* getDataset ( int which ) {
		if (!(0<=which && which<mFileOrDataCount)) {
            assert( 0<=which && which<mFileOrDataCount );
        }
        CavassData*  cd = mCavassData;
        assert( cd!=NULL );
        for (int i=0; i<mFileOrDataCount; i++) {
            if (i==which)    return cd;
            cd = cd->mNext;
        }
        return NULL;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  public:
	void OnReset ( void );
    void loadData ( char* name,
        const int xSize, const int ySize, const int zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh=NULL,
        const bool vh_initialized=false );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void loadFile ( const char* const fn );
    void reload ( void );
    bool mapWindowToData (          int wx, int wy, int& d, int& x, int& y, int& z );
	int  mapWindowToMagnifyVector ( int wx, int wy );
	int  mapWindowToMagnifyVector ( int wx, int wy, int& d, int& x, int& y, int& z );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    inline void mapDataToWindow ( int x, int y, int z, int& wx, int& wy ) const
    {
        //map the point in the 3d data set back into a point in the window
        wx = wy = -1;
        //if (!inBounds(x, y, z)) {
        //    wxLogMessage( "MontageCanvas::mapDataToWindow: out of bounds at (%d,%d,%d).", x, y, z );
        //}
        if      (x<0)         x=0;
        else if (x>=mCavassData->m_xSize)  x=mCavassData->m_xSize-1;
        if      (y<0)         y=0;
        else if (y>=mCavassData->m_ySize)  y=mCavassData->m_ySize-1;
        if      (z<0)         z=0;
        else if (z>=mCavassData->m_zSize)  z=mCavassData->m_zSize-1;
        const int lastSlice = m_sliceNo + m_rows*m_cols - 1;
        if (z<m_sliceNo || z>lastSlice)    return;
        const int  col    = (z-m_sliceNo) % m_cols;
        const int  row    = (z-m_sliceNo) / m_cols;
        const int  startX = (int)(col*(ceil(mCavassData->m_xSize*mCavassData->m_scale)+1));
        const int  startY = (int)(row*(ceil(mCavassData->m_ySize*mCavassData->m_scale)+1));
        wx = startX + (int)(x*mCavassData->m_scale+0.5);
        wy = startY + (int)(y*mCavassData->m_scale+0.5);
        wx += m_tx;
        wy += m_ty;
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void OnChar ( wxKeyEvent&   e );
    void OnMouseMove ( wxMouseEvent& e );
	void mouseMoveMagnify ( wxMouseEvent& e );
    void OnLeftDown ( wxMouseEvent& e );
    void OnLeftUp ( wxMouseEvent& e );
    void OnMiddleDown ( wxMouseEvent& e );
    void OnMiddleUp ( wxMouseEvent& unused );
    void OnRightDown ( wxMouseEvent& e );
    void OnRightUp ( wxMouseEvent& e );
    void OnPaint ( wxPaintEvent& e );
    void paint ( wxDC* dc );
    void paintMeasure ( wxDC* dc, int sx, int sy, int dataset, int slice );
    void paintMagnify ( wxDC* dc );
    void OnSize ( wxSizeEvent&  e );

    void moveToNextRow   ( void );
    void moveToNextSlice ( void );
    void moveToPrevRow   ( void );
    void moveToPrevSlice ( void );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void   initLUT ( int which );
    bool   isLoaded ( int which );

    int    getCenter ( int which );
	bool   getInterpolate ( void ) { return mInterpolate; }
    bool   getInvert ( int which );
	int    getMin ( int which );
    int    getMax ( int which );
	int    getNoSlices ( void );
    double getScale ( void );
    int    getSliceNo ( void );    //first displayed slice
    int    getWidth ( int which );

    void   setCenter ( int which, int    center  );
	void   setFirstDisplayedDataset ( int which );
	void   setInterpolate ( bool interpolate );
    void   setInvert ( int which, bool   invert  );
    void   setScale ( double scale );
    void   setSliceNo ( int sliceNo );
    void   setWidth ( int which, int    width   );
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    DECLARE_DYNAMIC_CLASS( MontageCanvas )
    DECLARE_EVENT_TABLE()
};

#endif
