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
// GBScale
//----------------------------------------------------------------------
#ifndef __GBScale_h
#define __GBScale_h

#include  "BallScale.h"
#include  "GScale.h"

using namespace std;

/** \brief definition of GB Scale class */
template< typename T >  //type of input data
class GBScale : public GScale< T > {
public:
#if ! defined (WIN32) && ! defined (_WIN32)
  using Scale<T>::mInData;
  using Scale<T>::getTwoDOnlyFlag;
  using Scale<T>::mXSize;
  using Scale<T>::mYSize;
  using Scale<T>::mZSize;
  using Scale<T>::mXSpace;
  using Scale<T>::mYSpace;
  using Scale<T>::mZSpace;
  using Scale<T>::mKpsi;
  using Scale<T>::index;
  using Scale<T>::inBounds;
  using Scale<T>::B;
#endif
  using GScale<T>::mOutData;

private:  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  BallScale<T>*  mBallScale;
  //made private to ensure that this constructor is NOT used.
  GBScale ( void ) { }
public: // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  GBScale ( const T* const inData,
           const int xSize, const int ySize, const int zSize,
           const double xSpace, const double ySpace, const double zSpace,
           const bool twoDOnlyFlag=true, const bool zeroBorderFlag=false,
           const bool verboseFlag=false )
      : GScale<T>( inData, xSize, ySize, zSize, xSpace, ySpace, zSpace,
                   twoDOnlyFlag, zeroBorderFlag, verboseFlag )
  {
      cout << "GBScale::GBScale" << endl;
      mBallScale = new BallScale<T>( (T*)mInData, mXSize, mYSize, mZSize,
          mXSpace, mYSpace, mZSpace, twoDOnlyFlag, zeroBorderFlag,
          verboseFlag, mKpsi );
      mBallScale->setTs(1.0);
      assert( mBallScale );
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ~GBScale ( ) {
      cout << "GBScale::~GBScale" << endl;
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  deque<point3d*> estimateScale ( const int cx, const int cy, const int cz )
  {
      point3d*  p = new point3d(cx, cy, cz);
      deque<point3d*>  open;
      open.push_back(p);
      //indicate that it is either on open or closed
      memset(mOutData, 0, mXSize * mYSize * mZSize * sizeof(unsigned char));
      mOutData[index(cx,cy,cz)] = 1;

      deque<point3d*>  closed;
      while (!open.empty()) {
          // cout << "sizes: open=" << open.size() << ", closed="
          //      << closed.size() << endl;
          p = open.front();
          open.pop_front();
          closed.push_back(p);

          //get the ball at this point
          const int  ball = mBallScale->estimateScale( p->x, p->y, p->z );
          if (getTwoDOnlyFlag()) {
              const int  z=p->z;
                  for (int y=p->y-ball; y<=p->y+ball; y++) {
                      for (int x=p->x-ball; x<=p->x+ball; x++) {
                          if (!inBounds(x,y,z))          continue;
                          if (mOutData[index(x,y,z)])    continue;
                          if (!B(p->x, p->y, p->z, x, y, z, ball))    continue;

                          point3d*  t = new point3d(x,y,z);
                          open.push_back(t);
                          mOutData[index(x,y,z)] = 1;
                      }
                  }
          } else {
              for (int z=p->z-ball; z<=p->z+ball; z++) {
                  for (int y=p->y-ball; y<=p->y+ball; y++) {
                      for (int x=p->x-ball; x<=p->x+ball; x++) {
                          if (!inBounds(x,y,z))          continue;
                          if (mOutData[index(x,y,z)])    continue;
                          if (!B(p->x, p->y, p->z, x, y, z, ball))    continue;

                          point3d*  t = new point3d(x,y,z);
                          open.push_back(t);
                          mOutData[index(x,y,z)] = 1;
                      }
                  }
              }
          }
      }
      cout << "estimateScale:: closed size=" << closed.size() << endl;
      return closed;
  }

};

#endif
//======================================================================

