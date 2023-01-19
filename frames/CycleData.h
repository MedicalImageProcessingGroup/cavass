/*
  Copyright 1993-2008, 2017 Medical Image Processing Group
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
 * \file   CycleData.h
 * \brief  Definition and implementation of CycleData class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __CycleData_h
#define  __CycleData_h

class CycleData : public ChunkData {
public:
    int         mWX, mWY;      ///< image (x,y) position/translation w/in window
    int         mNumber;       ///< consecutive image number (doesn't change)
    bool        mLocked;       ///< image is locked at this slice

    bool        mHasRoi;       ///< ROI data flag
    int         mRoiX, mRoiY;  ///< ROI (x,y) image position offset/inset (in pixels)
    int         mRoiWidth;     ///< ROI width (in pixels)
    int         mRoiHeight;    ///< ROI height (in pixels)
    CycleData*  mRoiData;      ///< ptr to data source for this ROI
    //------------------------------------------------------------------
    CycleData ( int slicesPerChunk=defaultSlicesPerChunk,
                int overlapSliceCount=defaultOverlapSliceCount )
        : ChunkData( slicesPerChunk, overlapSliceCount )
    {
        init();
    }
    //------------------------------------------------------------------
    CycleData ( const char* const fn, int slicesPerChunk=defaultSlicesPerChunk,
        int overlapSliceCount=defaultOverlapSliceCount, bool grayOnly=false )
        : ChunkData( fn, slicesPerChunk,overlapSliceCount, grayOnly )
    {
        init();
    }
    //------------------------------------------------------------------
    CycleData ( char* name,
        const int    xSize,    const int    ySize,    const int    zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized )
        : ChunkData( name, xSize, ySize, zSize, xSpacing, ySpacing, zSpacing,
                     data, vh, vh_initialized )
    {
        init();
    }
    //------------------------------------------------------------------
    virtual ~CycleData ( void ) {
        if (mHasRoi) {
            /** \todo this is an artifact of a roi being created via a 
             *  copy ctor and should be corrected to create an independent
             *  instance.
             */
            //"pretend" that allocated data has already been deallocated
            // so that the dtors in the super classes won't actually
            // deallocate the data (it will be deallocated by the dtors
            // of the data from which the roi was derived).

            //from ~ChunkData
            mFp    = 0;
            m_data = 0;
            //from ~CavassData
            m_ysub = 0;
            m_zsub = 0;
            m_lut  = 0;
        }
    }
    //------------------------------------------------------------------
    void init ( void ) {
        mWX = mWY =  mNumber = 0;
        mLocked = mHasRoi = false;
        mRoiX = mRoiY = mRoiWidth = mRoiHeight = 0;
        mRoiData = 0;
    }

};

#endif
