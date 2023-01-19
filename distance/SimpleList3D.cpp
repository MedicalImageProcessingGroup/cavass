/**
    \file SimpleList3D.cpp
    Implementation of SimpleList 3D distance transform (simply an exhaustive 
    search employing a list) class which, given an input binary image, 
    calculates the corresponding distance transform.

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
#include  <vector>
#include  "DistanceTransform3D.h"
#include  "SimpleList3D.h"

using namespace std;
//----------------------------------------------------------------------
/** \brief do the 3D distance transform
 *
 *  Input : I - a 3D binary scene of domain size X by Y by Z.
 *  Output: d - a 3D grey scene of domain size X, Y, and Z representing the
 *              distance scene.
 */
void SimpleList3D::doTransform ( const unsigned char* const I ) {
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
                const int i=sub(x,y,z);
                d[i] = DistanceTransform3D::FloatInfinity;
                //pointer to immediate interior or exterior i.e.
                //p(x,y).x is the x coordinate of the closest boundary
                //point (and similarly for p(x,y).y)
                this->p[i].x = this->p[i].y = this->p[i].z = -1;
            }
        }
    }

    //isotropic (i.e., same spacing in all 3 directions)?
    if (mXSpace==1.0 && mYSpace==1.0 && mZSpace==1.0) {
        performIsotropicTransform( I, d );
    } else {
        performAnisotropicTransform( I, d );
    }

    //perform the sqrt that was not performed above and delayed until now
    for (z=0; z<zSize; z++) {
        for (y=0; y<ySize; y++) {
            for (x=0; x<xSize; x++) {
                const int  c = sub(x,y,z);
                d[c] = sqrt( d[c] );
            }
        }
    }

    finish( I, d );
}
//----------------------------------------------------------------------
///perform isotropic distance transform
void SimpleList3D::performIsotropicTransform ( const unsigned char* const I,
    double* d )
{
    int  x, y, z;
    //initialize immediate interior & exterior elements
    // (i.e. border elements)
    vector< P<int>* >  v;
    if (mSymmetricFlag) {
        for (z=0; z<zSize; z++) {
            for (y=0; y<ySize; y++) {
                for (x=0; x<xSize; x++) {
                    const int  i = sub(x,y,z);
                    if (mConnectivity>=Face6) {
                        if ( get(I,x-1,y,z) != I[i] || get(I,x+1,y,z) != I[i] ||
                             get(I,x,y-1,z) != I[i] || get(I,x,y+1,z) != I[i] ||
                             get(I,x,y,z-1) != I[i] || get(I,x,y,z+1) != I[i] )
                        {
                            assert( mHalfXSpace == mHalfYSpace &&
                                    mHalfYSpace == mHalfZSpace );
                            d[i] = mHalfXSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;
                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                        }
                    }
                    if (mConnectivity>=Edge18) {
                        if ( get(I,x-1,y-1,z) != I[i] || get(I,x-1,y+1,z) != I[i] ||
                             get(I,x+1,y-1,z) != I[i] || get(I,x+1,y+1,z) != I[i] ||
                             get(I,x,y-1,z-1) != I[i] || get(I,x,y-1,z+1) != I[i] ||
                             get(I,x,y+1,z-1) != I[i] || get(I,x,y+1,z+1) != I[i] ||
                             get(I,x-1,y,z-1) != I[i] || get(I,x-1,y,z+1) != I[i] ||
                             get(I,x+1,y,z-1) != I[i] || get(I,x+1,y,z+1) != I[i] )
                        {
                            assert( mHalfXYDistance==mHalfXZDistance &&
                                    mHalfXZDistance==mHalfYZDistance );
                            d[i] = mHalfXYDistance;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;
                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                        }
                    }
                    if (mConnectivity>=Vertex26) {
                        if ( get(I,x-1,y-1,z-1) != I[i] || get(I,x-1,y-1,z+1) != I[i] ||
                             get(I,x-1,y+1,z-1) != I[i] || get(I,x-1,y+1,z+1) != I[i] ||
                             get(I,x+1,y-1,z-1) != I[i] || get(I,x+1,y-1,z+1) != I[i] ||
                             get(I,x+1,y+1,z-1) != I[i] || get(I,x+1,y+1,z+1) != I[i] )
                        {
                            d[i] = mHalfXYZDistance;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;
                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                        }
                    }
                }  //endfor x
            }  //endfor y
        }  //endfor z
    } else {
		cout << "isotropic, asymmetric" << endl;

        const int  cx=11-1, cy=11-1, cz=13-1;
        {
			cout << "(cx,cy,cz)=(" << cx+1 << "," << cy+1 << "," << cz+1 << ")" << endl;
            for (int z=cz-1; z<=cz+1; z++) {
                for (int y=cy-1; y<=cy+1; y++) {
                    for (int x=cx-1; x<=cx+1; x++) {
                        cout << get(I,x,y,z);
                    }
					cout << endl;
                }
                cout << endl;
            }
            cout << endl;
        }

        for (z=0; z<zSize; z++) {
            for (y=0; y<ySize; y++) {
				if (z==cz)    cout << endl;
                for (x=0; x<xSize; x++) {
                    const int i=sub(x,y,z);
#if 0
                    if ( I[sub(x-1,y,z)] < I[i] || I[sub(x+1,y,z)] < I[i] ||
                         I[sub(x,y-1,z)] < I[i] || I[sub(x,y+1,z)] < I[i] ||
                         I[sub(x,y,z-1)] < I[i] || I[sub(x,y,z+1)] < I[i] )
                    {
                        assert( mHalfXSpace == mHalfYSpace &&
                                mHalfYSpace == mHalfZSpace );
                        d[i] = mHalfXSpace;
                        this->p[i].x = x;
                        this->p[i].y = y;
                        this->p[i].z = z;
                        P<int>*  p = new P<int>(x,y,z);
                        v.push_back(p);
                    }  //endif
#else
					if (z==cz) {
						if (x==cx && y==cy) {
							if (I[i]==0)    cout << "*";
							else            cout << "X";
						} else {
						    if (I[i]==0)    cout << "0";
						    else            cout << "1";
						}
					}

					if (mConnectivity >= Face6) {
                        if ( get(I,x-1,y,z) < I[i] || get(I,x+1,y,z) < I[i] ||
                             get(I,x,y-1,z) < I[i] || get(I,x,y+1,z) < I[i] ||
                             get(I,x,y,z-1) < I[i] || get(I,x,y,z+1) < I[i] )
                        {
						    if (x==cx && y==cy && z==cz)    cout << "-6-";
                            assert( mHalfXSpace == mHalfYSpace &&
                                    mHalfYSpace == mHalfZSpace );
                            d[i] = mHalfXSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;
                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                        }
                    }
                    if (mConnectivity >= Edge18) {
                        if ( get(I,x-1,y-1,z) < I[i] || get(I,x-1,y+1,z) < I[i] ||
                             get(I,x+1,y-1,z) < I[i] || get(I,x+1,y+1,z) < I[i] ||
                             get(I,x,y-1,z-1) < I[i] || get(I,x,y-1,z+1) < I[i] ||
                             get(I,x,y+1,z-1) < I[i] || get(I,x,y+1,z+1) < I[i] ||
                             get(I,x-1,y,z-1) < I[i] || get(I,x-1,y,z+1) < I[i] ||
                             get(I,x+1,y,z-1) < I[i] || get(I,x+1,y,z+1) < I[i] )
                        {
						    if (x==cx && y==cy && z==cz)    cout << "-18-";
                            assert( mHalfXYDistance==mHalfXZDistance &&
                                    mHalfXZDistance==mHalfYZDistance );
                            d[i] = mHalfXYDistance;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;
                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                        }
                    }
                    if (mConnectivity >= Vertex26) {
                        if ( get(I,x-1,y-1,z-1) < I[i] || get(I,x-1,y-1,z+1) < I[i] ||
                             get(I,x-1,y+1,z-1) < I[i] || get(I,x-1,y+1,z+1) < I[i] ||
                             get(I,x+1,y-1,z-1) < I[i] || get(I,x+1,y-1,z+1) < I[i] ||
                             get(I,x+1,y+1,z-1) < I[i] || get(I,x+1,y+1,z+1) < I[i] )
                        {
                            if (x==cx && y==cy && z==cz)    cout << "-26-";
                            d[i] = mHalfXYZDistance;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;
                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                        }
                    }
#endif
                }  //endfor x
            }  //endfor y
        }  //endfor z
    }
    
    //iterate over all (non border element) points
    for (z=0; z<zSize; z++) {
        for (y=0; y<ySize; y++) {
            for (x=0; x<xSize; x++) {
                const int  c = sub(x,y,z);
                if (d[c]==0)    continue;  //skip border elements

                //at this stage, we have a point that is not an element of 
                // the border.  now, iterate over all border elements.
                for (int i=0; i<(int)v.size(); i++) {
                    const int  x1 = v[i]->x;
                    const int  y1 = v[i]->y;
                    const int  z1 = v[i]->z;
                    //calculate the distance to this border element
#if 0
                    const double  dTmp = sqrt( (double)
                        ( (x-x1)*(x-x1) + (y-y1)*(y-y1) + (z-z1)*(z-z1) ) );
#else
                    const double  dTmp = 
                        (x-x1)*(x-x1) + (y-y1)*(y-y1) + (z-z1)*(z-z1);
#endif
                    //is it better than what's already been assigned?
                    if (dTmp < d[c]) {
                        //yes, then change to this border element
                        d[c] = dTmp;
                        this->p[c].x = x1;
                        this->p[c].y = y1;
                        this->p[c].z = z1;
                    }
                }
            }  //end for x
        }  //end for y
    }  //end for z
}
//----------------------------------------------------------------------
///perform anisotropic distance transform
void SimpleList3D::performAnisotropicTransform ( const unsigned char* const I,
    double* d )
{
    int  x, y, z;
    //initialize immediate interior & exterior elements
    // (i.e. border elements)
    vector< P<int>* >     v;
    vector< P<double>* >  physical;
    if (mSymmetricFlag) {
        for (z=1; z<zSize-1; z++) {
            for (y=1; y<ySize-1; y++) {
                for (x=1; x<xSize-1; x++) {
                    const int i=sub(x,y,z);
                    if ( I[sub(x-1,y,z)] != I[i] || I[sub(x+1,y,z)] != I[i] ) {
                        if (mHalfXSpace < d[i]) {
                            d[i] = mHalfXSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;

                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                            //calculate actual physical location
                            P<double>*  phys = new P<double>( x*mXSpace, y*mYSpace, z*mZSpace );
                            physical.push_back( phys );
                        }
                    }
                    if ( I[sub(x,y-1,z)] != I[i] || I[sub(x,y+1,z)] != I[i] ) {
                        if (mHalfYSpace < d[i]) {
                            d[i] = mHalfYSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;

                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                            //calculate actual physical location
                            P<double>*  phys = new P<double>( x*mXSpace, y*mYSpace, z*mZSpace );
                            physical.push_back( phys );
                        }
                    }
                    if ( I[sub(x,y,z-1)] != I[i] || I[sub(x,y,z+1)] != I[i] ) {
                        if (mHalfZSpace < d[i]) {
                            d[i] = mHalfZSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;

                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                            //calculate actual physical location
                            P<double>*  phys = new P<double>( x*mXSpace, y*mYSpace, z*mZSpace );
                            physical.push_back( phys );
                        }  //endif
                    }  //endif
                }  //endfor x
            }  //endfor y
        }  //endfor z
    } else {
        for (z=1; z<zSize-1; z++) {
            for (y=1; y<ySize-1; y++) {
                for (x=1; x<xSize-1; x++) {
                    const int i=sub(x,y,z);
                    if ( I[sub(x-1,y,z)] < I[i] || I[sub(x+1,y,z)] < I[i] ) {
                        if (mHalfXSpace < d[i]) {
                            d[i] = mHalfXSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;

                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                            //calculate actual physical location
                            P<double>*  phys = new P<double>( x*mXSpace, y*mYSpace, z*mZSpace );
                            physical.push_back( phys );
                        }
                    }
                    if ( I[sub(x,y-1,z)] < I[i] || I[sub(x,y+1,z)] < I[i] ) {
                        if (mHalfYSpace < d[i]) {
                            d[i] = mHalfYSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;

                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                            //calculate actual physical location
                            P<double>*  phys = new P<double>( x*mXSpace, y*mYSpace, z*mZSpace );
                            physical.push_back( phys );
                        }
                    }
                    if ( I[sub(x,y,z-1)] < I[i] || I[sub(x,y,z+1)] < I[i] ) {
                        if (mHalfZSpace < d[i]) {
                            d[i] = mHalfZSpace;
                            this->p[i].x = x;
                            this->p[i].y = y;
                            this->p[i].z = z;

                            P<int>*  p = new P<int>(x,y,z);
                            v.push_back(p);
                            //calculate actual physical location
                            P<double>*  phys = new P<double>( x*mXSpace, y*mYSpace, z*mZSpace );
                            physical.push_back( phys );
                        }  //endif
                    }  //endif
                }  //endfor x
            }  //endfor y
        }  //endfor z
    }
        
    //iterate over all (non border element) points
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
                for (int i=0; i<(int)v.size(); i++) {
                    const int     x1 = v[i]->x;
                    const int     y1 = v[i]->y;
                    const int     z1 = v[i]->z;
                    const double  X1 = physical[i]->x;
                    const double  Y1 = physical[i]->y;
                    const double  Z1 = physical[i]->z;
                    //calculate the distance to this border element
#if 0
                    const double  dTmp = sqrt(
                        (X-X1)*(X-X1) + (Y-Y1)*(Y-Y1) + (Z-Z1)*(Z-Z1) );
#else
                    const double  dTmp = 
                        (X-X1)*(X-X1) + (Y-Y1)*(Y-Y1) + (Z-Z1)*(Z-Z1);
#endif
                    //is it better than what's already been assigned?
                    if (dTmp < d[c]) {
                        //yes, then change to this border element
                        d[c] = dTmp;
                        this->p[c].x = x1;
                        this->p[c].y = y1;
                        this->p[c].z = z1;
                    }
                }
            }  //end for x
        }  //end for y
    }  //end for z
}
//----------------------------------------------------------------------

