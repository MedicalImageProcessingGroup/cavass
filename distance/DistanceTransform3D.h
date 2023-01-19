/**
    \file DistanceTransform3D.h
    Header file for DistanceTransform3D class which, given an input
    binary image, calculates the corresponding distance transform via
    various methods.

    \author George J. Grevera, Ph.D., ggrevera@sju.edu

    Copyright (C) 2002, George J. Grevera

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
    USA or from http://www.gnu.org/licenses/gpl.txt.

    This General Public License does not permit incorporating this
    code into proprietary programs.  (So a hypothetical company such
    as GH (Generally Hectic) should NOT incorporate this code into
    their proprietary programs.)
 */
#ifndef  DistanceTransform3D_h
#define  DistanceTransform3D_h

#include  <assert.h>
#include  <float.h>
#include  <limits.h>
#include  <math.h>
/// Define the sqrt(2) if not defined in math.h (such as the Windows NiceTry
/// compiler).
#ifndef  M_SQRT2
    #define  M_SQRT2  1.41421356237309504880
#endif

#include  <stdio.h>
//----------------------------------------------------------------------
/// an abstract 3D distance transform class
class DistanceTransform3D {

public:
    /**
     * an integer representing infinity (but one such that smaller values
     * may still be added to it)
     */
    static const int     IntInfinity;
    /**
     * a double representing infinity (but one such that smaller values
     * may still be added to it)
     */
    static const double  FloatInfinity;

    enum { Face6 = 6, Edge18 = 18, Vertex26 = 26 };
    int  mConnectivity;

    /// construct a distance transform object for an image of size 
    /// (xSize,ySize,zSize).
    /**
     * if 'unload' is true, transforms that use ratios of integers should
     * convert their integer results to the actual double values (of the 
     * ratios).
     */
    DistanceTransform3D (
      const int xSize, const int ySize, const int zSize,
      const double xSpace=1.0, const double ySpace=1.0, const double zSpace=1.0,
      const double halfXSpace=0.0, const double halfYSpace=0.0,
      const double halfZSpace=0.0,
      const bool unload=true, const bool symmetric=true,
      const int connectivity=Face6 );

    virtual ~DistanceTransform3D ( void );

    /// calculate the distance transform (note: pure virtual)
    virtual void doTransform ( const unsigned char* const I ) = 0;

    /// return the distance value assigned to a particular position
    inline virtual double getD ( const int x, const int y, const int z ) const {
        if (dD != NULL)  return dD[sub(x,y,z)];
        //otherwise, use int's
        assert( iD!=NULL );
        return iD[sub(x,y,z)];
    }

    /// save the result
    void saveResult ( const char* const fname ) const {
        FILE*  fp=fopen( fname, "wb" );     assert( fp!=NULL );
        if (dD!=NULL) {
#ifndef  NDEBUG
            size_t  n =
#endif
            fwrite( dD, sizeof *dD, xSize*ySize*zSize, fp );
            assert( n == (size_t)xSize*ySize*zSize );
        } else {
            assert( iD!=NULL );
#ifndef  NDEBUG
            size_t  n =
#endif
            fwrite( iD, sizeof *iD, xSize*ySize*zSize, fp );
            assert( n == (size_t)xSize*ySize*zSize );
        }
        fclose( fp );    fp=NULL;
    }

    /// return the "parent" (border point) associated with this point
    /**
     * @param x is the x location
     * @param y is the y location
     * @param px is the parent's x location to be returned
     * @param py is the parent's y location to be returned
     * @return true if the parent is know; otherwise false.
     */
    virtual inline bool getP ( const int x, const int y, const int z,
                               int& px, int& py, int& pz ) const {
        px = py = pz = -1;
        return false;
    }

    void setSymmetricFlag ( bool b ) {  mSymmetricFlag = b;  }

protected:
    bool mSymmetricFlag; ///< transform is symmetric (inside is not different from outside)

	int        xSize;    ///< integer size (width) in pixels
    int        ySize;    ///< integer size (height) in pixels
    int        zSize;    ///< integer size (depth/slices) in pixels

	double     mXSpace;  ///< physical size in x direction
    double     mYSpace;  ///< physical size in y direction
    double     mZSpace;  ///< physical size in z direction

	double     mHalfXSpace;  ///< half physical size in x direction
    double     mHalfYSpace;  ///< half physical size in y direction
    double     mHalfZSpace;  ///< half physical size in z direction

	double     mXYDistance;
	double     mXZDistance;
	double     mYZDistance;

	double     mHalfXYDistance;
	double     mHalfXZDistance;
	double     mHalfYZDistance;

	double     mXYZDistance;
	double     mHalfXYZDistance;

    //calculated distance transforms (some distance transforms use int's
    // and others use double's to represent the distance values
    double*    dD;       ///< result of distance transform (as doubles)
    int*       iD;       ///< the calculated distance transform (as ints)

    /// DistanceTransform::P class.  Simply a 3D point.
    template <typename T>
    class P {
        public:
            T  x, y, z;
            inline P ( const T x, const T y, const T z ) {
                this->x = x;
                this->y = y;
                this->z = z;
            }
    };

    /// given a point (x,y,z) in an image, this method returns the corresponding
    /// 1d subscript into the 1d image array
    inline int sub ( const int x, const int y, const int z ) const {
        if (x<0 || x>=this->xSize) {
            assert( x>=0 && x<this->xSize );
        }
        if (y<0 || y>=this->ySize) {
            assert( y>=0 && y<this->ySize );
        }
        if (z<0 || z>=this->zSize) {
            assert( z>=0 && z<this->zSize );
        }
        assert( rowOffsets!=NULL );
        assert( sliceOffsets!=NULL );
        return sliceOffsets[z] + rowOffsets[y] + x;
    }
#if 0
    /// init elements of the immediate interior (i.e., border points).
    template <class T>
        void initImmediate ( const unsigned char* const I, T* d,
                             const T halfDx=0, const T halfDy=0, const T halfDz=0 ) {
            //initialize elements of the immediate exterior
            // & the immediate interior
            for (int z=1; z<zSize-1; z++) {
                for (int y=1; y<ySize-1; y++) {
                    for (int x=1; x<xSize-1; x++) {
                        if ( I[sub(x-1,y,z)] != I[sub(x,y,z)] ||
                             I[sub(x+1,y,z)] != I[sub(x,y,z)] ) {
                            if (halfDx < d[sub(x,y,z)])    d[sub(x,y,z)] = halfDx;
                        }
                        if ( I[sub(x,y-1,z)] != I[sub(x,y,z)] ||
                             I[sub(x,y+1,z)] != I[sub(x,y,z)] ) {
                            if (halfDy < d[sub(x,y,z)])    d[sub(x,y,z)] = halfDy;
                        }
                        if ( I[sub(x,y,z-1)] != I[sub(x,y,z)] ||
                             I[sub(x,y,z+1)] != I[sub(x,y,z)] ) {
                            if (halfDz < d[sub(x,y,z)])    d[sub(x,y,z)] = halfDz;
                        }
                    }
                }
            }
    }
#endif
#if 0
    template <class T>
        inline void check ( T* d, const int center, const int X, const int Y, const int Z,
                            const T Delta ) {
            const int  Near     = sub(X,Y,Z);
            const T    possible = d[Near] + Delta;
            if (possible < d[center]) {
                d[center] = possible;
            }
    }
#endif
    virtual void borderCheck ( const unsigned char* const I );
    void cleanUp ( void );
    void finish ( const unsigned char* const I, double* d );
    void finish ( const unsigned char* const I, int* d,
                  const int dx, const int dy, const int dz );

private:
    int*  rowOffsets;     ///< used to speed up 3d array indexing
    int*  sliceOffsets;   ///< used to spped up 3d array indexing
    bool  unloadFlag;     ///< convert ints to doubles (if necessary)
};

#endif
//----------------------------------------------------------------------
