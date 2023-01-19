/**
    \file Simple3D.cpp
    Implementation of Simple 3D distance transform (simply an exhaustive search)
    class which, given an input binary image, calculates the corresponding
    distance transform.

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
/*
    no OUT OF BOUNDS checks!
*/
#include  <stdlib.h>
#include  "DistanceTransform3D.h"
#include  "Simple3D.h"

using namespace std;
//----------------------------------------------------------------------
/** \brief do the 3D distance transform
 *
 *  Input : I - a 3D binary scene of domain size X by Y by Z
 *  Output: d - a 3D grey scene of domain size X, Y, and Z representing
 *              the distance scene
 */
void Simple3D::doTransform ( const unsigned char* const I ) {
    borderCheck( I );
    cleanUp();
    int  x, y, z;

    //initialize d (the distance values) to infinity.
    // initialize p (the parent coordinates) to -1.
    
    double*  d = (double*)malloc( zSize*ySize*xSize*sizeof(double)   );
    assert( d!=NULL );
    this->p = (P<int>*)malloc( zSize*ySize*xSize*sizeof(P<int>) );
    assert( this->p!=NULL );
    for (z=0; z<zSize; z++) {
        for (y=0; y<ySize; y++) {
            for (x=0; x<xSize; x++) {
                const int  i=sub(x,y,z);
                d[i] = DistanceTransform3D::FloatInfinity;
                //pointer to immediate interior or exterior i.e.
                //p(x,y).x is the x coordinate of the closest boundary
                //point (and similarly for p(x,y).y)
                this->p[i].x = this->p[i].y = this->p[i].z = -1;
            }
        }
    }

    if (mSymmetricFlag)  initializeSymmetric(  I, d );
    else                 initializeAsymmetric( I, d );

    //isotropic?
    if (mXSpace!=1.0 || mYSpace!=1.0 || mZSpace!=1.0) {
        //iterate over all (non border element) points
        for (z=0; z<zSize; z++) {
            for (y=0; y<ySize; y++) {
                for (x=0; x<xSize; x++) {
                    const int  c = sub(x,y,z);
                    if (d[c]==0)    continue;  //skip border elements
                    
                    //iterate over all border elements
                    for (int z1=0; z1<zSize; z1++) {
                        for (int y1=0; y1<ySize; y1++) {
                            for (int x1=0; x1<xSize; x1++) {
                                const int  t = sub(x1,y1,z1);
                                if (d[t]!=0)    continue;
                                //calculate the distance to this border element
                                const double  dTmp = sqrt( (double)
                                    ((x-x1)*(x-x1) + (y-y1)*(y-y1) + (z-z1)*(z-z1)) );
                                //is it better than what's already been assigned?
                                if (dTmp < d[c]) {
                                    //yes, then change to this border element
                                    d[c] = dTmp;
                                    this->p[c].x = x1;
                                    this->p[c].y = y1;
                                    this->p[c].z = z1;
                                }
                            }  //end for x1
                        }  //end for y1
                    }  //end for z1
                    
                }  //end for x
            }  //end for y
        }  //end for z
    } else {
        for (z=0; z<zSize; z++) {
            for (y=0; y<ySize; y++) {
                for (x=0; x<xSize; x++) {
                    const int  c = sub(x,y,z);
                    if (d[c]==0)    continue;  //skip border elements

                    //calculate actual physical location
                    const double  X = x*mXSpace;
                    const double  Y = y*mYSpace;
                    const double  Z = z*mZSpace;
                    
                    //at this stage, we have a point that is not an element of 
                    // the border.  now, iterate over all border elements.
                    for (int z1=0; z1<zSize; z1++) {
                        for (int y1=0; y1<ySize; y1++) {
                            for (int x1=0; x1<xSize; x1++) {
                                const int  t = sub(x1,y1,z1);
                                if (d[t]!=0)    continue;

                                //calculate the actual physical location
                                const double  X1 = x1*mXSpace;
                                const double  Y1 = y1*mYSpace;
                                const double  Z1 = z1*mZSpace;
                                //calculate the distance to this border element
                                const double  dTmp = sqrt(
                                    (X-X1)*(X-X1) + (Y-Y1)*(Y-Y1) + (Z-Z1)*(Z-Z1) );
                                //is it better than what's already been assigned?
                                if (dTmp < d[c]) {
                                    //yes, then change to this border element
                                    d[c] = dTmp;
                                    this->p[c].x = x1;
                                    this->p[c].y = y1;
                                    this->p[c].z = z1;
                                }
                            }  //end for x1
                        }  //end for y1
                    }  //end for z1
                    
                }  //end for x
            }  //end for y
        }  //end for z
    }

    finish( I, d );
}
//----------------------------------------------------------------------
///initialize immediate interior & exterior elements
void Simple3D::initializeAsymmetric ( const unsigned char* const I, double* d )
{
    assert( !mSymmetricFlag );

    int  x, y, z;
    for (z=1; z<zSize-1; z++) {
        for (y=1; y<ySize-1; y++) {
            for (x=1; x<xSize-1; x++) {
                const int  i=sub(x,y,z);
                if ( I[sub(x-1,y,z)] < I[i] ||
                     I[sub(x+1,y,z)] < I[i] ) {
                    if (this->mHalfXSpace < d[i]) {
                        d[i] = mHalfXSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                    }
                }
                if ( I[sub(x,y-1,z)] < I[i] ||
                     I[sub(x,y+1,z)] < I[i] ) {
                    if (mHalfYSpace < d[i]) {
                        d[i] = mHalfYSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                    }
                }
                if ( I[sub(x,y,z-1)] < I[i] ||
                     I[sub(x,y,z+1)] < I[i] ) {
                    if (mHalfZSpace < d[i]) {
                        d[i] = mHalfZSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                    }
                }  //endif
            }  //endfor x
        }  //endfor y
    }  //endfor y
}
//----------------------------------------------------------------------
///initialize immediate interior & exterior elements
void Simple3D::initializeSymmetric ( const unsigned char* const I, double* d )
{
    assert( mSymmetricFlag );

    int  x, y, z;
    for (z=1; z<zSize-1; z++) {
        for (y=1; y<ySize-1; y++) {
            for (x=1; x<xSize-1; x++) {
                const int  i=sub(x,y,z);
                if ( I[sub(x-1,y,z)] != I[i] ||
                     I[sub(x+1,y,z)] != I[i] ) {
                    if (this->mHalfXSpace < d[i]) {
                        d[i] = mHalfXSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                    }
                }
                if ( I[sub(x,y-1,z)] != I[i] ||
                     I[sub(x,y+1,z)] != I[i] ) {
                    if (mHalfYSpace < d[i]) {
                        d[i] = mHalfYSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                    }
                }
                if ( I[sub(x,y,z-1)] != I[i] ||
                     I[sub(x,y,z+1)] != I[i] ) {
                    if (mHalfZSpace < d[i]) {
                        d[i] = mHalfZSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                    }
                }  //endif
            }  //endfor x
        }  //endfor y
    }  //endfor z
}
//----------------------------------------------------------------------

