/**
    \file Simple3D.h
    Header file for Simple3D 3D distance transform class which, given
    an input binary image, calculates the corresponding distance transform.

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
#ifndef  Simple3D_h
#define  Simple3D_h

#include  "DistanceTransform3D.h"
#include  <iostream>

using namespace std;
//----------------------------------------------------------------------
/** \brief Simple3D 3D distance transform algorithm.
 *
 *  The Simple3D distance transform is simply an exhaustive search.  It
 *  compares each non border point to its distance from each border point
 *  and determines the minimum distance (from the non border point to the
 *  border points).  Border points are not stored in any auxillary data
 *  structure so they must be repeatedly search for in the input binary
 *  image.  Hence this algorithm is not very efficient.
 */
class Simple3D : public DistanceTransform3D {

public:
    Simple3D ( const int xSize, const int ySize, const int zSize,
      const double xSpace=1.0, const double ySpace=1.0, const double zSpace=1.0,
      const double halfXSpace=0.0, const double halfYSpace=0.0,
      const double halfZSpace=0.0,
      const bool unload=true, const bool symmetric=true )
      : DistanceTransform3D( xSize, ySize, zSize, xSpace, ySpace, zSpace,
          halfXSpace, halfYSpace, halfZSpace, unload, symmetric )
    {
    }

    void doTransform ( const unsigned char* const I );

    /// return the distance value assigned to a particular position.
	/// (if out-of-bounds, 0 is returned.
    inline virtual double getD ( const int x, const int y, const int z ) const {
        if (x<0 || x>=this->xSize || y<0 || y>=this->ySize || z<0 || z>=this->zSize) {
			return 0;
        }
        if (dD != NULL)  return dD[sub(x,y,z)];
        //otherwise, use int's
        assert( iD!=NULL );
        return iD[sub(x,y,z)];
    }

    /**
     * @param x is the x location
     * @param y is the y location
     * @param z is the z location
     * @param px is the parent's x location to be returned
     * @param py is the parent's y location to be returned
     * @param pz is the parent's z location to be returned
     * @return true if the parent is know; otherwise false.
     */
    virtual inline bool getP ( const int x, const int y, const int z,
                               int& px, int& py, int& pz ) const {
        if (p==NULL)    return false;
        const int  s = sub(x,y,z);
        px = p[s].x;
        py = p[s].y;
        pz = p[s].z;
        return true;
    }

protected:
    P<int>*  p;
    void initializeSymmetric  ( const unsigned char* const I, double* d );
    void initializeAsymmetric ( const unsigned char* const I, double* d );
};

#endif
//----------------------------------------------------------------------
