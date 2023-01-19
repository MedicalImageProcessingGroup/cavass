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

#ifndef __Misc_h
#define __Misc_h

#include  <deque>

using namespace std;

class point3d {
public:
    int  x, y, z;
    point3d ( const int x, const int y, const int z ) {
        this->x=x;  this->y=y;  this->z=z;
    }
};

//determine the centroid of a set of points
void determineCentroid ( double& cx, double& cy, double& cz,
    deque<point3d*> list, const bool verboseFlag=false );

//determine the centroid of a set of points
void determineCentroid ( int& cx, int& cy, int& cz,
    deque<point3d*> list, const bool verboseFlag=false );

//determine the grey centroid
template< typename C, typename D, typename M >
void determineCentroid ( C& cx, C& cy, C& cz, const D* const data,
    const M max, const long xSize, const long ySize, const long zSize )
{
    cx = cy = cz = -1;
    double  xCenter=0, yCenter=0, zCenter=0;
    const double  oneOverMax = 1.0 / max;
    long  i=0;
    for (int z=0; z<zSize; z++) {
        for (int y=0; y<ySize; y++) {
            for (int x=0; x<xSize; x++) {
                xCenter += x * (data[i] * oneOverMax);
                yCenter += y * (data[i] * oneOverMax);
                zCenter += z * (data[i] * oneOverMax);
                i++;
            }
        }
    }
    xCenter /= xSize * ySize * zSize;
    yCenter /= xSize * ySize * zSize;
    zCenter /= xSize * ySize * zSize;

    cx = xCenter;
    cy = yCenter;
    cz = zCenter;

    cout << "centroid=(" << cx << "," << cy << "," << cz << ")" << endl;
}

void get2DEigen ( deque<point3d*> list, double& ex, double& ey,
    const bool verboseFlag=false );

double distancePointLine2D ( const double x0, const double y0,    //point
                             const double x1, const double y1,    //line
                             const double x2, const double y2 );  //line

//from nrc, p. 467-9.
void jacobi ( double **a, int n, double d[], double **v, int *nrot );

void jacobi2D ( const double a11, const double a12,
                const double a21, const double a22,
                double& ex,       double& ey );

void eigsrt ( double d[], double **v, int n );

//from nrc, p. 113-116
void spline ( double* x, double* y, const int n, const double yp1, const double ypn, double* y2 );
void splint ( double* xa, double* ya, double* y2a, const int n, const double x, double* y );
//----------------------------------------------------------------------

#endif

