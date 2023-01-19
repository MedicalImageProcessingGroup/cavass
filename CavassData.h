/*
  Copyright 1993-2015 Medical Image Processing Group
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
 * \file   CavassData.h
 * \brief  Definition and implementation of CavassData and SliceData
 *         classes.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __CavassData_h
#define  __CavassData_h

#ifdef WIN32_V6  //vc++ version 6 only
    #include  <iostream.h>
    #include  <iomanip.h>
#else
    #include  <iostream>
    #include  <iomanip>
#endif
#include  <sstream>
#include  "cv3dv.h"
#include  "wx/wx.h"
#include  "wx/image.h"
#include  "Dicom.h"
#include  "DicomInfoFrame.h"
#ifndef WIN32_TEST
    #include  "tiffio.h"
#endif

#define ERR_OPENFILE                101
#define ERR_READFILE                102
#define ERR_ONLYSUPPORT3D           103
#define ERR_ONLYSUPPORT_1_8_16_32   104
#define ERR_OUTOFMEMORY             105   // xinjian chen Error definition
#define ERR_LOADCAVASSFILE          106
#define ERR_FILENAME                107
#define ERR_CLASSCONSTRUCTION       108

/** \brief Definition and implementation of CavassData class.
 *
 *  This class represents data objects in CAVASS.  Data objects can either
 *  be read in from files or can be constructed from blocks of data in
 *  memory.  A pointer to the next CavassData object is part of the data
 *  structure as well so that linked lists of CavassData can be grouped
 *  together as well.
 *  Currently, the following file types are supported: 3DVIEWNIX (.BIM,
 *  .IM0), DICOM (.dcm, .dicom), JPEG (.jpg, .jpeg), TIFF (.tif, .tiff),
 *  PNG (.png), and GIF (.gif).
 */
class CavassData {
public:
    CavassData*  mNext;    ///< next pointer for linked list
    int          m_xSize,  ///< count of pixels in x direction
                 m_ySize,  ///< count of pixels in y direction
                 m_zSize,  ///< count of pixels in z direction
                 m_tSize;  ///< count of pixels in t direction
    double       m_xSpacing,  ///< physical spacing/size of voxels in mm
                 m_ySpacing,  ///< physical spacing/size of voxels in mm
                 m_zSpacing,  ///< physical spacing/size of voxels in mm
                 m_tSpacing;  ///< physical spacing/size of voxels in s
    int          m_size;           ///< 1=byte, 2=short, 4=int
    long         m_bytesPerSlice;  ///< # of bytes per slice
	const int    mLogLevel;
    /** \todo need to fully support these data types
     *  (instead of looking at m_size because an m_size doesn't indicate whether
     *  or not the data are signed or unsigned and a value of 4 can either be
     *  an int or a float).
     *  \brief enumerate the possible data types (u=unsigned, s=signed, c=char,
     *         s=short, i=int, f=float, d=double).
     */
    enum { ucType,  ///< unsigned char type
           usType,  ///< unsigned short type
           uiType,  ///< unsigned int type
           scType,  ///< signed char type
           ssType,  ///< signed short type
           siType,  ///< signed int type
           fType,   ///< float type
           dType    ///< double type
    } mDataType;    ///< one of the above enumerated data types
    /** \brief In the case of the CavassData class, m_data will point to the
     *  entire volume of data that was loaded into memory.
     *  For the ChunkData subclass, m_data will point to an array of pointers.
     *  Each pointer in this array will point to the individual slice data.
     */
    void*           m_data;
    bool            mEntireVolumeIsLoaded;  ///< true if entire volume was loaded
    bool            mIsCavassFile;     ///< true if source was a CAVASS file
    bool            mIsDicomFile;      ///< true if source was a DICOM file
    bool            mIsImageFile;      ///< true if source was an image file
    bool            mIsAsciiVtkFile;   ///< true if source was an ASCII VTK file
    bool            mIsBinaryVtkFile;  ///< true if source was a binary VTK file
    bool            m_vh_initialized;  ///< true if m_vh has been initialized
    ViewnixHeader   m_vh;              ///< 3dviewnix header
    /** \brief file_name.  tested to see if anything has been loaded.
     *         in the case of data, it will also be set to something reasonable.
     */
    char*           m_fname;
    /** \todo add color support (currently, only gray is supported) */
    bool            mIsColor;           ///< true->color; false->gray
    int             mSamplesPerPixel;   ///< 1->gray;  3->color
    /** \brief min data value.
     *  if the entire data set is not loaded initially, then this value may
     *  not be know and may change as more and more slices are loaded.
     */
    int             m_min;
    /** \brief max data value.
     *  if the entire data set is not loaded initially, then this value may
     *  not be know and may change as more and more slices are loaded.
     */
    int             m_max;
    //display/view information for this data:
    unsigned char*  m_lut;              ///< lookup table for contrast
    bool            mInvert;
    /** \todo make scale (m_scale) independent in both directions */
    double          m_scale;            ///< scale for both directions
    bool            m_overlay;          ///< toggle overlay
    int             m_sliceNo;          ///< slice # of first displayed slice
    int             m_volNo;            ///< volume # of first displayed slice
    int             m_center,           ///< contrast setting
                    m_width;            ///< contrast setting
    bool            mDisplay;           ///< display this data or not
    double          mR, mG, mB;         ///< rgb weights for blending [0.0..1.0]

protected:
    /** \brief fseek file offset to start of data part of file.
     *  this allows us to seek and read the individual slice data as needed.
     *  this is currently used only for vtk format files.
     */
#if defined (WIN32) || defined (_WIN32)
    long  mFileOffsetToData;
    #define  fseeko      fseek
    #define  ftello      ftell
    #define  strcasestr  strstr
#else
    off_t  mFileOffsetToData;
#endif
    //------------------------------------------------------------------
public:
    /** \brief this function determines if a given string ends with another
     *  specified string
     */
    static bool endsWith ( const char* const str, const char* const end )
    {
        if (strlen(str)<strlen(end))    return false;
        wxString  myStr( str );
        wxString  myEnd( end );
        #ifdef Right
            #undef Right
        #endif
        myStr = myStr.Right( myEnd.Length() );
        return (myStr.CmpNoCase( end ) == 0);
    }
    //----------------------------------------------------------------------
    /** \brief CavassData ctor. */
    CavassData ( void ) : mLogLevel(2) {
        init();
    }
    //------------------------------------------------------------------
    /** \brief CavassData ctor to load data from a file.
     *  \param fn is the name of file to read.
     *  \param loadHeaderOnly should be set to true when only header information should be loaded; otherwise, (if loadHeaderOnly is false) then all of the image data (in addition to the header information) will be loaded.
     */
    CavassData ( const char* const fn, const bool loadHeaderOnly=false, bool grayOnly=true, int loglevel=2 ) : mLogLevel(loglevel) {
        init();
        int err = loadFile( fn, loadHeaderOnly, grayOnly );
        if (err > 0)    return;

        initSubs();
    }
    //------------------------------------------------------------------
    /** \brief CavassData ctor to load data from memory.
     *  \param name is the name (not filename) associated with this data.
     *  \param xSize is the columns in the data.
     *  \param ySize is the rows in the data.
     *  \param zSize is the slices in the data.
     *  \param xSpacing is the space between columns.
     *  \param ySpacing is the space between rows.
     *  \param zSpacing is the space between slices.
     *  \param data is the actual data.
     *  \param vh is the optional 3dviewnix header associated with this data; otherwise NULL.
     *  \param vh_initialized is true if vh should be used; false otherwise.
     *  \todo  expand this to include data other than ints.
     *  \todo  expand this to include rgb/vector valued data.
     */
    CavassData ( char* name,
        const int    xSize,    const int    ySize,    const int    zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized ) : mLogLevel(2)
    {
        init();
        loadData( name, xSize, ySize, zSize, xSpacing, ySpacing, zSpacing,
                  data, vh, vh_initialized );
        initSubs();
    }
    //------------------------------------------------------------------
    /** \brief CavassData dtor. */
    virtual ~CavassData ( void ) {
        if (m_data!=NULL) { free(m_data);  m_data=NULL; }
        if (m_ysub!=NULL) { free(m_ysub);  m_ysub=NULL; }
        if (m_zsub!=NULL) { free(m_zsub);  m_zsub=NULL; }
        if (m_lut !=NULL) { free(m_lut );  m_lut =NULL; }
    }
    //------------------------------------------------------------------
    /** \brief indicates whether or not actual data has been associated 
     *  with this object.
     *  \returns true if actual data has been associated with this object; false otherwise.
     */
    bool dataIsLoaded ( void ) const {
        return (m_data!=NULL);
    }
    //------------------------------------------------------------------
    /** \brief this method returns true if the data are binary and false
     *  otherwise.
     */
    bool isBinary ( void ) const {
        if (m_vh_initialized)
        {
            if (m_vh.gen.data_type == IMAGE0)
                return m_vh.scn.num_of_bits==1;
            return m_vh.gen.data_type!=SHELL0 ||
                m_vh.str.num_of_bits_in_TSE==32;
        }
        if (m_min==0 && m_max==1)    return true;
        return false;
    }
    //------------------------------------------------------------------
    /** \brief this method returns the dimensionality of the data
     *  (currently on 2d and 3d are supported).
     */
    int getNumberOfDimensions ( void ) const {
        if (m_vh_initialized && m_vh.gen.data_type == IMAGE0)
            return m_vh.scn.dimension;
        if (m_zSize<=1)          return 2;
        return 3;
    }
    //------------------------------------------------------------------
    /** \brief this method determines and updates the global min and max
     *  according to the current contents of the data and resets the lut
     *  accordingly.  this method does not alter m_center or m_width.
     */
    void resetLUT ( void ) {
        determineGlobalMinAndMax();
        resetLUT( m_min, m_max );
    }
    //------------------------------------------------------------------
    /** \brief this method can be called to modify the lut (this is 
     *  typically useful whenever the data's min and/or max has changed.
     *  note that m_center and m_width are not changed.
     */
    void resetLUT ( int newMin, int newMax ) {
        assert( newMin <= newMax );
        if (m_lut != NULL) {
            free( m_lut );
            m_lut = NULL;
        }

        m_min = newMin;
        m_max = newMax;
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
    /** \brief    this function returns a pointer to the beginning of the
     *            slice data.
     *  \param    which is the specific slice number.
     *  \returns  the pointer to the slice data; 0 if the slice doesn't
     *            exist i.e., is before or beyond the limits of the data set.
     */
    virtual void* getSlice ( const int which );
    //------------------------------------------------------------------
private:
    /** \brief method to load data from memory.
     *  \param name is the name (not filename) associated with this data.
     *  \param xSize is the columns in the data.
     *  \param ySize is the rows in the data.
     *  \param zSize is the slices in the data.
     *  \param xSpacing is the space between columns.
     *  \param ySpacing is the space between rows.
     *  \param zSpacing is the space between slices.
     *  \param data is the actual data.
     *  \param vh is the optional 3dviewnix header associated with this data; otherwise NULL.
     *  \param vh_initialized is true if vh should be used; false otherwise.
     *  \todo  expand this to include data other than ints.
     *  \todo  expand this to include rgb/vector valued data.
     */
    void loadData ( char* name,
        const int    xSize,    const int    ySize,    const int    zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized );
    //------------------------------------------------------------------
    /** \brief method to load data from a file.
     *
     *  Currently, the following file types are supported:
     *      CAVASS/3DVIEWNIX (.BIM,.IM0), DICOM (.dcm, .dicom), GIF (.gif),
     *      JPEG (.jpg, .jpeg), PNG (.png), TIFF (.tif, .tiff),
     *      and VTK (.vtk).
     *  All uppercase or all lowercase is supported.
     *  \param fn is the name of file to read.
     *  \param loadHeaderOnly should be set to true when only header information should be loaded; otherwise, (if loadHeaderOnly is false) then all of the image data (in addition to the header information) will be loaded.
     */
    int loadFile ( const char* const fn, const bool loadHeaderOnly, bool grayOnly );
    //------------------------------------------------------------------
    void determineGlobalMinAndMax ( void );
    //------------------------------------------------------------------
public:
    /** \brief get the current blue emphasis value.
     *  \returns the current blue emphasis value in the range [0.0 .. 1.0].
     */
    double getB       ( void ) const {  return mB;         }
    /** \brief get the current center contrast value.
     *  \returns the current center contrast value.
     */
    int    getCenter  ( void ) const {  return m_center;   }
    /** \brief get the current green emphasis value.
     *  \returns the current green emphasis value in the range [0.0 .. 1.0].
     */
    double getG       ( void ) const {  return mG;         }
    /** \brief get the current overlay setting.
     *  \returns true if overlay is on; false otherwise.
     */
    bool   getOverlay ( void ) const {  return m_overlay;  }
    /** \brief get the current red emphasis value.
     *  \returns the current red emphasis value in the range [0.0 .. 1.0].
     */
    double getR       ( void ) const {  return mR;         }
    /** \brief get the current global scale value.
     *  \returns the current global scale value.
     */
    double getScale   ( void ) const {  return m_scale;    }
    /** \brief get the current width contrast value.
     *  \returns the current width contrast value.
     */
    int    getWidth   ( void ) const {  return m_width;    }
    //------------------------------------------------------------------
    /** \brief set the current blue emphasis value.
     *  \param b is the new blue emphasis value in the range [0.0 .. 1.0].
     */
    void setB ( const double b ) {
        assert( 0.0<=b && b<=1.0 );
        mB = b;
    }
    /** \brief set the current center contrast value.
     *  \param center is the new center contrast value.
     */
    void   setCenter  ( const int center   )  {  m_center  = center;   }
    /** \brief set the current green emphasis value.
     *  \param g is the new green emphasis value in the range [0.0 .. 1.0].
     */
    void setG ( const double g ) {
        assert( 0.0<=g && g<=1.0 );
        mG = g;
    }
    /** \brief set the current overlay value.
     *  \param overlay is the new overlay value.
     */
    void setOverlay ( const bool overlay )  {  m_overlay = overlay;  }
    /** \brief set the current red emphasis value.
     *  \param r is the new red emphasis value in the range [0.0 .. 1.0].
     */
    void setR ( const double r ) {
        assert( 0.0<=r && r<=1.0 );
        mR = r;
    }
    /** \brief set the current scale value.
     *  \param scale is the new scale value.
     */
    void setScale   ( const double scale )  {  m_scale   = scale;    }
    /** \brief set the current width contrast value.
     *  \param width is the new width contrast value.
     */
    void   setWidth   ( const int width    )  {  m_width   = width;    }
    //------------------------------------------------------------------
protected:
    unsigned long*  m_ysub;  ///< indexing to a particular row
    unsigned long*  m_zsub;  ///< indexing to a particular slice

    /** \brief method to speed up indexing into 3D data. */
    void initSubs ( void );
    //------------------------------------------------------------------
public:
    /** \brief determine if a particular subscript location (x,y,z) is
     *      within bounds.
     *  \returns true if (x,y,z) is within bounds; false otherwise.
     */
    inline bool inBounds ( const int x, const int y, const int z ) const {
        if (x<0 || y<0 || z<0 || x>=m_xSize || y>=m_ySize || z>=m_zSize)
            return false;
        return true;
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
        if ( x<0 || x>=m_xSize || y<0 || y>=m_ySize || z<0 || z>=m_zSize )
            return 0;
        //return p[z*m_ySize*m_xSize + y*m_xSize + x];  //too slow
        return p[ m_zsub[z] + m_ysub[y] + x ];
    }
    //------------------------------------------------------------------
    /** \brief get the data value at a particular (x,y,z) location.
     *  \returns the int data value.
     */
    inline virtual int getData ( const int x, const int y, const int z )
    {
        if (m_data==NULL)    return 0;
        //out of bounds check
        if (x<0 || y<0 || z<0 || x>=m_xSize || y>=m_ySize || z>=m_zSize)
            return 0;

        //const int  i = z*m_xSize*m_ySize + y*m_xSize + x;  //too slow
        const int  i = m_zsub[z] + m_ysub[y] + x;
        if (m_size==1) {
            if (m_min>=0) {
                unsigned char*  d = (unsigned char*)m_data;
                return d[i];
            } else {
                signed char*  d = (signed char*)m_data;
                return d[i];
            }
        } else if (m_size==2) {
            if (m_min>=0) {
                unsigned short*  d = (unsigned short*)m_data;
                return d[i];
            } else {
                signed short*  d = (signed short*)m_data;
                return d[i];
            }
        } else if (m_size==4) {
            int*  d = (int*)m_data;
            return d[i];
        } else if (m_size==8) {
            double*  d = (double*)m_data;
            return (int)d[i];
        } else {
            assert(0);
        }
        return 0;
    }
    //------------------------------------------------------------------
    inline virtual void getData ( double& value, int x, int y, int z ) {
        if (m_data==NULL) {
            value = 0;
            return;
        }
        //out of bounds check
        if (x<0 || y<0 || z<0 || x>=m_xSize || y>=m_ySize || z>=m_zSize) {
            value = 0;
            return;
        }
        const int  i = m_zsub[z] + m_ysub[y] + x;
        assert( m_size==8 );
        double*  d = (double*)m_data;
        value = d[i];
    }
    //------------------------------------------------------------------
    /** \brief get the data value at a particular 2D (x,y) location.
     *  \param p is the source of the data of type T.
     *  \param x is the x location.
     *  \param y is the y location.
     *  \returns the data value of type T.
     */
    template< typename T >
    //inline T get2DData ( const T* const p, const int x, const int y ) const {
    inline T getData ( const T* const p, const int x, const int y ) const {
        if (x<0 || x>=m_xSize || y<0 || y>=m_ySize)    return 0;
        //return p[y*m_xSize + x];  //too slow
        return p[ m_ysub[y] + x ];
    }
    //------------------------------------------------------------------
    /** \brief get the index for a particular 2D (x,y) location.
     *  \param x is the x location.
     *  \param y is the y location.
     *  \returns the index value.
     */
    inline unsigned long index ( const int x, const int y ) const {
        assert( x>=0 && x<m_xSize && y>=0 && y<m_ySize );
        //return y * m_xSize + x;  //too slow
        return m_ysub[y] + x;
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
        assert( x>=0 && x<m_xSize && y>=0 && y<m_ySize
             && z>=0 && z<m_zSize );
        //return z*m_ySize*m_xSize + y*m_xSize + x;  //too slow
        return m_zsub[z] + m_ysub[y] + x;
    }
    //------------------------------------------------------------------
    /** \brief initialize this object's LUT by using values from another LUT.
     *  \param newLUT is the other LUT used to set this object's LUT.
     *  \param size is the number of entries in the LUT.
     */
    void initLUT ( const unsigned char* const newLUT, const int size ) {
        assert( size == (m_max-m_min+1) );
        assert( m_lut != NULL && newLUT != NULL );
        for (int i=0; i<size; i++)    m_lut[i] = newLUT[i];
    }
    //------------------------------------------------------------------
    /** \brief initialize this object's LUT (LookUp Table) using the 
     *      current values for
     *  m_center, m_width, m_min, m_max, and mInvert.
     */
    void initLUT ( void );

    //------------------------------------------------------------------
    int sliceIndex ( const int volume, const int slice_of_volume );
	int sliceOfVolume ( const int slice_index );
	int volumeOfSlice ( const int slice_index );
	void getVolumeAndSlice ( int &volume, int &slice_of_volume,
        const int slice_index );
    //------------------------------------------------------------------
protected:
    /** \brief initialize this object. */
    void init ( void );
    //------------------------------------------------------------------
    /** \brief load image data (the entire volume, all at once) from a file
     *  (specified by m_fname).
     */
    virtual void loadImageFile ( bool grayOnly );
    //------------------------------------------------------------------
#ifndef WIN32_TEST
    /** \brief load TIFF image data (the entire volume, all at once) from a file
     *  (specified by m_fname).
     */
    virtual void loadImageFileTIFF ( bool grayOnly );
#endif
    //------------------------------------------------------------------
    /** \brief load image data (the entire volume, all at once) from a
     *  DICOM file (specified by m_fname).
     */
    void loadDicomFile ( void );
    //------------------------------------------------------------------
    /** \brief change the endian-ness of one short. */
    static inline void swap ( unsigned short* usptr ) {
        unsigned char*  ucptr = (unsigned char*)usptr;
        unsigned char   tmp = ucptr[0];
        ucptr[0] = ucptr[1];
        ucptr[1] = tmp;
    }
    //------------------------------------------------------------------
    /** \brief load image data from a VTK file (specified by m_fname).
     *
     *  example created by ITK:
     *  (VTK format from http://www.vtk.org/VTK/img/file-formats.pdf)
     *  <pre>
     *  # vtk DataFile Version 3.0
     *  VTK File Generated by Insight Segmentation and Registration Toolkit (ITK)
     *  BINARY or ASCII
     *  DATASET STRUCTURED_POINTS
     *  DIMENSIONS 255 255 138
     *  SPACING 9.7656e-01 9.7656e-01 9.7656e-01
     *  ORIGIN 0.0e+00 0.0e+00 0.0e+00
     *  POINT_DATA 8973450
     *  SCALARS scalars unsigned_short 1
     *  LOOKUP_TABLE default
     *  ...
     *  </pre>
     *
     *  \param loadHeaderOnly should be set to true when only header information should be loaded; otherwise, (if loadHeaderOnly is false) then all of the image data (in addition to the header information) will be loaded.
     */
    void loadVtkFile ( const bool loadHeaderOnly );
    //----------------------------------------------------------------------
    /** \brief unpack the packed binary data bits to either 0 or 1 in 8 bit data
     */
    static void unpack ( unsigned char* dst, unsigned char* src, int xSize, int ySize )
    {
        static const int  masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
        int  i=0;
        for (int y=0; y<ySize; y++) {
            for (int x=0; x<xSize; x++) {
                const int  s = y * xSize + x;
                if (src[s/8] & masks[s%8])  dst[i] = 1;
                else                        dst[i] = 0;
                i++;
            }
        }
    }
    //------------------------------------------------------------------
    /** \brief  load image data from a CAVASS file (specified by m_fname).
     *  \param  loadHeaderOnly should be set to true when only header
     *          information should be loaded; otherwise, (if loadHeaderOnly
     *          is false) then all of the image data (in addition to the 
     *          header information) will be loaded.
     */
     int loadCavassFile ( const bool loadHeaderOnly );
    //------------------------------------------------------------------
    /** \brief useful function to show the contents of the 3DVIEWNIX header.
     */
    void showHeaderContents ( void );

};
//======================================================================
//======================================================================
/**
 *  \brief  The CavassData class loads the entire volume of data.  This
 *  causes noticeable delays for large data sets.  This class can be used
 *  to loads data one slice at a time as needed.
 */
class SliceData : public CavassData
{
protected:
    FILE*  mFp;  ///< cache the file pointer so slices may be loaded as needed
public:
    void* pSliceData;
    int   nCurSliceNo;

    //////////// init LUT ///////
     void init ( void );

    SliceData ( void ) : CavassData() 
    {
        mFp = 0;
        pSliceData = NULL;
        nCurSliceNo = -1;
    }

    /** \brief load only the header. */
    SliceData ( const char* const fn ) : CavassData( fn, true ) 
    {
        if (!mIsCavassFile || !m_vh_initialized)
            mFp = 0;
        else
        {
            init();

            mFp = fopen(fn, "rb");
        }
        if (mFp==NULL) 
        {
            cerr << "    Can't open " << fn << "." << endl;
            wxLogMessage( "    Can't open %s", fn );			
        }
    }

    SliceData ( char* name,
        const int    xSize,    const int    ySize,    const int    zSize,
        const double xSpacing, const double ySpacing, const double zSpacing,
        const int* const data, const ViewnixHeader* const vh,
        const bool vh_initialized )
        : CavassData ( name, xSize, ySize, zSize, xSpacing, ySpacing, zSpacing,
                       data, vh, vh_initialized )
    {
        init();
    }

    virtual ~SliceData ( void ) 
    {
        if (mFp) 
        {
            fclose( mFp );
            mFp = NULL;
        }
        if (!mEntireVolumeIsLoaded && m_data) 
        {
            for (int i=0; i<m_zSize; i++)
            {
                freeSlice( i );
            }
        }
        if( pSliceData != NULL )
        {
            free( pSliceData );
            pSliceData = NULL;
        }
    }

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
    /** \brief    this function returns a pointer to the beginning of the
     *            slice data.  this function will load the slice from disk
     *            into memory if it hasn't already been loaded.
     *  \param    which is the specific slice number.
     *  \returns  the pointer to the slice data; 0 if the slice doesn't
     *            exist (on disk), i.e., is before or beyond the limits of the 
     *            data set.
     *
     *   Modify by Xinjian Chen  2008.2.20	 
     */
    void* getSlice ( const int which );
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
    /** \brief get the data value at a particular (x,y,z) location.
     *  \returns the int data value.
     */
    inline virtual int getData ( const int x, const int y, const int z )
    {
        if (mEntireVolumeIsLoaded)    return CavassData::getData( x, y, z );

        void*  slice = getSlice( z );
        if (m_size==1) {
            unsigned char*  ucData = (unsigned char*)slice;
            unsigned char   uc = CavassData::getData( ucData, x, y );
            return uc;
        } else if (m_size==2) {
            unsigned short*  usData = (unsigned short*)slice;
            return CavassData::getData( usData, x, y );
        } else if (m_size==4) {
            int*  iData = (int*)slice;
            return CavassData::getData( iData, x, y );
        }
        assert(0);
        return 0;
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
