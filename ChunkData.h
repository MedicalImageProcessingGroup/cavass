/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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
 * \file   ChunkData.h
 * \brief  Definition and implementation of ChunkData class.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __ChunkData_h
#define  __ChunkData_h

/** \brief This class can be used to manage contiguous slices called 
 *  "chunks" (rather loading the entire volume or dealing with the data
 *  one slice at a time).
 */
class ChunkData : public CavassData {
protected:
    FILE*        mFp;                 ///< cache the file pointer so slices may be loaded as needed
    const int    mSlicesPerChunk;     ///< # of slices in a chunk
    const int    mOverlapSliceCount;  ///< # of slices that overlap (before and after this chunk)
    //------------------------------------------------------------------
    void init ( void ) {
        mFreeOldChunk = true;
        mFp = 0;
        if (m_zSize>0) {
            m_data = malloc( m_zSize * sizeof(void*) );
            if (m_data == NULL)
			{
				fprintf(stderr, "Out of memory.\n");
				exit(1);
			}
            void** tmp = (void**)m_data;
            for (int i=0; i<m_zSize; i++) {
                tmp[i] = 0;
            }
        }

        /** \todo add support for files other than cavass files (e.g., vtk, tiff, etc) */
        assert( mIsCavassFile && m_vh_initialized );
        if (m_vh.gen.data_type == IMAGE0)
        {
	        if (m_vh.scn.smallest_density_value_valid)
				m_min = (int)(m_vh.scn.smallest_density_value[0]);
	        else
				m_min = 0;
			if (m_vh.scn.largest_density_value_valid)
				m_max = (int)(m_vh.scn.largest_density_value[0] + 0.5);
			else
				m_max = (1<<m_vh.scn.num_of_bits)-1;
        }
        else
        {
			m_min = 0;
			m_max = 255;
        }

        m_lut = (unsigned char*)malloc( (m_max-m_min+1)*sizeof(unsigned char) );
        if (m_lut == NULL)
		{
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
        if (m_min==0 && m_max<=1)
        {
            m_center = 0;
            m_width  = 1;
        }
        else
        {
            m_center = m_min + (m_max-m_min+1)/2;
            m_width  = m_max-m_min+1;
        }
		initLUT();
    }
    //------------------------------------------------------------------
public:
    enum { defaultSlicesPerChunk=10, defaultOverlapSliceCount=2 };
    bool  mFreeOldChunk;       ///< free old chunk, otherwise entire volume may eventually be loaded; default=true;
    //------------------------------------------------------------------
    ChunkData ( int slicesPerChunk=defaultSlicesPerChunk,
                int overlapSliceCount=defaultOverlapSliceCount )
        : CavassData(), mSlicesPerChunk(slicesPerChunk),
          mOverlapSliceCount(overlapSliceCount)
    {
        if (m_vh_initialized)
			init();
    }
    //------------------------------------------------------------------
    /** \brief load only the header. */
    ChunkData ( const char* const fn, int slicesPerChunk=defaultSlicesPerChunk,
        int overlapSliceCount=defaultOverlapSliceCount, bool grayOnly=false,
		int loglevel=2 )
        : CavassData( fn, true, grayOnly, loglevel ),
          mSlicesPerChunk(slicesPerChunk), mOverlapSliceCount(overlapSliceCount)
    {
        if (mEntireVolumeIsLoaded) {
		    mFp = fopen( fn, "rb" );
			return;
        }
		if (m_vh_initialized)
		{
			init();
        	mFp = fopen( fn, "rb" );
		}
    }
    //------------------------------------------------------------------
    ChunkData ( char* name,
        const int    xSize,    const int    ySize,    const int    zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized )
        : CavassData ( name, xSize, ySize, zSize, xSpacing, ySpacing, zSpacing,
                       data, vh, vh_initialized ),
          mSlicesPerChunk(10), mOverlapSliceCount(2)
    {
        if (m_vh_initialized)
			init();
    }
    //------------------------------------------------------------------
    virtual ~ChunkData ( void ) {
        if (mFp) {
            fclose( mFp );
            mFp = 0;
        }
        if (!mEntireVolumeIsLoaded && m_data) {
            for (int i=0; i<m_zSize; i++) {
                freeSlice( i );
            }
            free( m_data );
            m_data = 0;
        }
    }
    //------------------------------------------------------------------
    /** \brief    this function determines if a particular slice has already
     *            been read from disk and stored in memory.
     *  \param    which is the specific slice number.
     *  \returns  true if the slice has already been loaded; false otherwise.
     */
    bool sliceIsLoaded ( const int which ) const {
        if (which<0 || which>=m_zSize || m_data==0)    return false;
        //really slice data?
        if (mEntireVolumeIsLoaded)    return true;
        //determine if particular slice has been loaded
        void** tmp = (void**)m_data;
        if (tmp[which])    return true;
        return false;
    }
    //------------------------------------------------------------------
    /** \brief    this function frees the memory used by the specified slice.
     *  \param    which is the specific slice number.
     *  \returns  true if memory was freed (because the specified slice had 
     *            been loaded); false otherwise.
     */
    bool freeSlice ( const int which ) {
        if (which<0 || which>=m_zSize || m_data==0)    return false;
        //really slice data?
        if (mEntireVolumeIsLoaded)    return false;
        //if particular slice has been loaded then free it.
        void** tmp = (void**)m_data;
        if (tmp[which]) {
            free( tmp[which] );
            tmp[which] = 0;
            return true;
        }
        return false;
    }
    //------------------------------------------------------------------
    /** \brief    this function returns a pointer to the beginning of the
     *            slice data.  this function will load the slice from disk
     *            into memory if it hasn't already been loaded.
     *  \param    which is the specific slice number.
     *  \returns  the pointer to the slice data; 0 if the slice doesn't
     *            exist (on disk), i.e., is before or beyond the limits of
     *            the data set.
     */
    virtual void* getSlice ( const int which ) {
        if (which<0 || which>=m_zSize || m_data==0) {
            return 0;
        }
        //volume data (or chunk data)?
        if (mEntireVolumeIsLoaded)    return CavassData::getSlice( which );

        //chunk data
        void** tmp = (void**)m_data;
        if (tmp[which])    return tmp[which];
        //so m_data[which] is 0 indicating that this slice hasn't 
        // been loaded yet.  so let's go ahead and load it.
        assert( mFileOffsetToData >= 0 );
        assert( mIsBinaryVtkFile || mIsCavassFile );
        
        int  firstSlice = which / mSlicesPerChunk * mSlicesPerChunk;
        int  lastSlice  = firstSlice + mSlicesPerChunk - 1;
        firstSlice -= mOverlapSliceCount;
        lastSlice  += mOverlapSliceCount;
        if (firstSlice < 0)          firstSlice = 0;
        if (lastSlice >= m_zSize)    lastSlice = m_zSize-1;
        if (mFreeOldChunk) {
            //free any loaded slices that are before the first slice in the chunk
            for (int i=0; i<firstSlice; i++) {
                if (tmp[i])    freeSlice( i );
            }
            //free any loaded slices that are after the last slice in the chunk
            for (int i=lastSlice+1; i<m_zSize; i++) {
                if (tmp[i])    freeSlice( i );
            }
        }

        //load any slices that are missing from this chunk
        for (int i=firstSlice; i<=lastSlice; i++) {
            if (!tmp[i]) {
                if (mIsCavassFile) {
                    assert( m_vh_initialized );
                    if (m_vh.scn.num_of_bits!=1) {
                        //gray (more than 1 bit per pixel) data
                        tmp[i] = malloc( m_bytesPerSlice );
                        if (tmp[i] == NULL)
						{
							fprintf(stderr, "Out of memory.\n");
							return NULL;
						}
                        int  err = VLSeekData( mFp, (double)i*m_bytesPerSlice );
                        int  num;
                        if (m_size==8) {
                            err = VReadData( (char*)tmp[i], m_size/2,
                                      2*m_bytesPerSlice/m_size, mFp, &num );
                            if (err == 0)
								assert( num == 2*m_bytesPerSlice/m_size );
                        } else if (m_size%2) {
                            err = VReadData( (char*)tmp[i], 1,
                                      m_bytesPerSlice, mFp, &num );
                        } else {
                            err = VReadData( (char*)tmp[i], m_size,
                                      m_bytesPerSlice/m_size, mFp, &num );
                            if (err == 0)
								assert( num == m_bytesPerSlice/m_size );
                        }
						if (err)
						{
							fprintf(stderr, "Error reading file.\n");
							free(tmp[i]);
							tmp[i] = NULL;
							return NULL;
						}
                    } else {  //this is binary (1 bit per pixel) data
                        if (!( m_vh.scn.dimension_in_alignment == 2 &&
							   m_vh.scn.bytes_in_alignment     == 1 ))
						{
							fprintf(stderr, "Unsupported alignment.\n");
							return NULL;
                        }
                        //unpack the packed binary data bits to either 0 or 1 in 8 bit data
                        //static const int  masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
                        //allow for widths that are not evenly divisible by (not multiples of) 8
                        int  pBytesPerSlice = m_xSize * m_ySize / 8;
                        if ((m_xSize*m_ySize)%8)    ++pBytesPerSlice;
                        //allocate temp space for the packed data
                        unsigned char*  pTmp = (unsigned char*)malloc( pBytesPerSlice );
                        if (pTmp == NULL)
						{
							fprintf(stderr, "Out of memory.\n");
							return NULL;
						}
                        //allocate the unpacked data
                        tmp[i] = malloc( m_bytesPerSlice );
                        if (tmp[i] == NULL)
						{
							fprintf(stderr, "Out of memory.\n");
							free(pTmp);
							return NULL;
                        }
						//move to the beginning of the specific slice of packed data
                        int  err = VLSeekData( mFp, (double)i*pBytesPerSlice );
                        //read the packed data
                        int  num;
                        err = VReadData( (char*)pTmp, 1, pBytesPerSlice, mFp, &num );
                        if (err == 0)
						{
							assert( num == pBytesPerSlice );
                        }
						else
						{
							fprintf(stderr, "Error reading file.\n");
							free(pTmp);
							free(tmp[i]);
							tmp[i] = NULL;
							return NULL;
						}
						//move/change from packed to unpacked
                        unpack( (unsigned char*)tmp[i], pTmp, m_xSize, m_ySize );
                        free( pTmp );    pTmp=NULL;
                    }  //end else
                } else {
                    /** \todo handle vtk file */
                    assert( 0 );
                }
            }
        }

        return tmp[ which ];
    }
    //------------------------------------------------------------------
    /** \brief get the data value at a particular 3D (x,y,z) location.
     *  \param p is the source of the data of type T.
     *  \param x is the x location.
     *  \param y is the y location.
     *  \param z is the z location.
     *  \returns the data value of type T.
     */
    template< typename T >
    inline T getData ( const T* const p, const int x, const int y, const int z )
    const {
        assert( mEntireVolumeIsLoaded );
        return CavassData::getData( p, x, y, z );
    }
    //------------------------------------------------------------------
    /** \brief    get the data value at a particular (x,y,z) location.
     *  \returns  the int data value.
     */
    inline virtual int getData ( const int x, const int y, const int z )
    {
        if (mEntireVolumeIsLoaded)    return CavassData::getData( x, y, z );

        void*  slice = getSlice( z );
        if (m_size%2==1) {
            if (m_min>=0) {
                unsigned char*  ucData = (unsigned char*)slice;
                unsigned char   uc = CavassData::getData( ucData, x, y );
                return uc;
            } else {
                signed char*  scData = (signed char*)slice;
                signed char   sc = CavassData::getData( scData, x, y );
                return sc;
            }
        } else if (m_size==2) {
            if (m_min>=0) {
                unsigned short*  usData = (unsigned short*)slice;
                return CavassData::getData( usData, x, y );
            } else {
                signed short*  ssData = (signed short*)slice;
                return CavassData::getData( ssData, x, y );
            }
        } else if (m_size==4) {
            int*  iData = (int*)slice;
            return CavassData::getData( iData, x, y );
        } else if (m_size==8) {
            double*  dData = (double*)slice;
            return (int)CavassData::getData( dData, x, y );
        }
        assert(0);
        return 0;
    }
    //------------------------------------------------------------------
    /** \brief    get the double data value at a particular (x,y,z) location.
     *  \param    value is the double pixel value at the specified location.
     *  \returns  the double data value.
     */
    inline virtual void getData ( double& value, int x, int y, int z ) {
        if (mEntireVolumeIsLoaded)
            return CavassData::getData( value, x, y, z );
        assert( m_size==8 );
        value = CavassData::getData( (double*)getSlice(z), x, y );
    }
    //------------------------------------------------------------------
    /** \brief get the index for a particular 3D (x,y,z) location.
     *  \param x is the x location.
     *  \param y is the y location.
     *  \param z is the z location.
     *  \returns the index value.
     */
    inline virtual unsigned long index ( const int x, const int y, const int z )
    const {
        assert( mEntireVolumeIsLoaded );
        return CavassData::index( x, y, z );
    }
};

#endif
