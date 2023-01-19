/**
    \file DistanceTransform3D.cpp
    Implementation for DistanceTransform3D abstract base class which, given an
    input binary image, calculates the corresponding 3D distance transform via
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
#include <assert.h>
#include <float.h>
#include <iostream>
#include <set>
#include <math.h>
#include <stdlib.h>

// no OUT OF BOUNDS checks!

#include "DistanceTransform3D.h"

// since many distance transforms add pairs of values, one or both of which
// may be "infinity," we need to define "infinity" to be half of the max
// value allowed for a particular numeric type
const int    DistanceTransform3D::IntInfinity   = INT_MAX / 2;
const double DistanceTransform3D::FloatInfinity = FLT_MAX / 2.0;

using namespace std;
//----------------------------------------------------------------------
DistanceTransform3D::DistanceTransform3D (
    const int xSize,  const int ySize, const int zSize,
    const double xSpace, const double ySpace, const double zSpace,
    const double xHalfSpace, const double yHalfSpace, const double zHalfSpace,
    const bool unload, const bool symmetric, const int connectivity )
    : mConnectivity(connectivity), mSymmetricFlag(symmetric),
      mXYDistance(0), mXZDistance(0), mYZDistance(0),
	  mHalfXYDistance(0), mHalfXZDistance(0), mHalfYZDistance(0),
	  mXYZDistance(0), mHalfXYZDistance(0)
{
    assert( mConnectivity==Face6 || mConnectivity==Edge18 || mConnectivity==Vertex26 );

    this->iD = NULL;
    this->dD = NULL;
    //this->xSize = this->ySize = -1;
    //this->rowOffsets = NULL;

    assert( xSize>0 && ySize>0 && zSize>0 );
    this->xSize = xSize;
    this->ySize = ySize;
    this->zSize = zSize;

    assert( xSpace>0 && ySpace>0 && zSpace>0 );
    this->mXSpace = xSpace;
    this->mYSpace = ySpace;
    this->mZSpace = zSpace;

    assert( xHalfSpace>=0 && yHalfSpace>=0 && zHalfSpace>=0 );
    this->mHalfXSpace = xHalfSpace;
    this->mHalfYSpace = yHalfSpace;
    this->mHalfZSpace = zHalfSpace;
    assert( xHalfSpace<=xSpace && yHalfSpace<=ySpace && zHalfSpace<=zSpace );

    this->rowOffsets = (int*)malloc( ySize * sizeof(int) );
    assert( this->rowOffsets!=NULL );
    for (int y=0; y<ySize; y++)    this->rowOffsets[y] = y*xSize;

    this->sliceOffsets = (int*)malloc( zSize * sizeof(int) );
    assert( this->sliceOffsets!=NULL );
    for (int z=0; z<zSize; z++)    this->sliceOffsets[z] = z*xSize*ySize;

    this->unloadFlag = unload;
    if (unload)
        cout << "ints WILL be converted to doubles." << endl;
    else
        cout << "ints will NOT be converted to doubles." << endl;
}
//----------------------------------------------------------------------
DistanceTransform3D::~DistanceTransform3D ( void ) {
    cleanUp();
    if (this->rowOffsets!=NULL)   { free(this->rowOffsets);    this->rowOffsets=NULL;   }
    if (this->sliceOffsets!=NULL) { free(this->sliceOffsets);  this->sliceOffsets=NULL; }
}
//----------------------------------------------------------------------
void DistanceTransform3D::cleanUp ( void ) {
    if (this->iD != NULL)  {  free(this->iD);  this->iD=NULL;  }
    if (this->dD != NULL)  {  free(this->dD);  this->dD=NULL;  }
}
//----------------------------------------------------------------------
void DistanceTransform3D::borderCheck ( const unsigned char* const I ) {
    for (int z=0; z<zSize; z++) {
        for (int x=0; x<xSize; x++) {
            //check the first row
            if (I[sub(x,0,z)]!=0)
                cout << "borderCheck: failed at (" << x << "," << 0 << ","
                     << z << ")." << endl;
            //check the last row
            if (I[sub(x,ySize-1,z)]!=0)
                cout << "borderCheck: failed at (" << x << "," << ySize-1 
                     << "," << z << ")." << endl;
        }

        for (int y=0; y<ySize; y++) {
            //check the first col
            if (I[sub(0,y,z)]!=0)
                cout << "borderCheck: failed at (" << 0 << "," << y << ","
                     << z << ")." << endl;
            //check the last col
            if (I[sub(xSize-1,y,z)]!=0)
                cout << "borderCheck: failed at (" << xSize-1 << "," << y
                     << "," << z << ")." << endl;
        }
    }
    for (int y=0; y<ySize; y++) {
        for (int x=0; x<xSize; x++) {
            //check the first slice
            if (I[sub(x,y,0)]!=0)
                cout << "borderCheck: failed at (" << x << "," << y << ","
                     << 0 << ")." << endl;
            //check the last slice
            if (I[sub(x,y,zSize-1)]!=0)
                cout << "borderCheck: failed at (" << x << "," << y << ","
                     << zSize-1 << ")." << endl;
        }
    }
}
//----------------------------------------------------------------------
/** called to finish up after distance transform employing doubles */
void DistanceTransform3D::finish ( const unsigned char* const I, double* d )
{
    //indicate inside & outside
    for (int z=0; z<zSize; z++) {
        for (int y=0; y<ySize; y++) {
            for (int x=0; x<xSize; x++) {
                const int i=sub(x,y,z);
                if (I[i] == 0)    d[i] = -d[i];
            }
        }
    }

    this->dD = d;
}
//----------------------------------------------------------------------
/** called to finish up after distance transform employing ints */
void DistanceTransform3D::finish ( const unsigned char* const I,
    int* d, const int dx, const int dy, const int dz )
{
    // "unload" == "convert ints to doubles"
    if (this->unloadFlag) {
        assert( dx==dy );
        double*  dblResult = (double*)malloc( zSize*ySize*xSize*sizeof(double) );
        assert( dblResult!=NULL );
        //indicate inside & outside
        for (int z=0; z<zSize; z++) {
            for (int y=0; y<ySize; y++) {
                for (int x=0; x<xSize; x++) {
                    const int i=sub(x,y,z);
                    if (I[i] == 0)    d[i] = -d[i];
                    //if (d[i] != IntInfinity)  dblResult[i] = d[i]/((double)dx);
                    //else                      dblResult[i] = FloatInfinity;
                    dblResult[i] = d[i];
                    if (d[i] > -DistanceTransform3D::IntInfinity && d[i] < DistanceTransform3D::IntInfinity) {
                        dblResult[i] /= dx;
                        assert( 0 );  //shouldn't get here
                    }
                }
            }
        }
        this->dD = dblResult;
        free(d);  d=NULL;  this->iD=NULL;

        return;
    }

    //otherwise, don't "unload" ints (i.e., don't convert them to doubles)

    //indicate inside & outside
    for (int z=0; z<zSize; z++) {
        for (int y=0; y<ySize; y++) {
            for (int x=0; x<xSize; x++) {
                const int i=sub(x,y,z);
                if (I[i] == 0)    d[i] = -d[i];
            }
        }
    }
    this->iD = d;
}
//----------------------------------------------------------------------
