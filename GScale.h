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
// GScale
//----------------------------------------------------------------------
#ifndef __GScale_h
#define __GScale_h

#include  "Scale.h"

using namespace std;
//----------------------------------------------------------------------
/** \brief definition of Generalized Scale class */
template< typename T >  //type of input data
class GScale : public Scale< T > {
public:
  using Scale<T>::mVerboseFlag;
  using Scale<T>::mXSize;
  using Scale<T>::mYSize;
  using Scale<T>::mZSize;
  using Scale<T>::index;
  using Scale<T>::getTwoDOnlyFlag;
  using Scale<T>::inBounds;
  using Scale<T>::mTs;
  using Scale<T>::getInData;
protected:
  unsigned char*  mOutData;
private:
  //made private to ensure that this constructor is not used.
  GScale ( void ) { }
  //----------------------------------------------------------------------
public:
  //----------------------------------------------------------------------
  GScale ( const T* const inData,
           const int xSize, const int ySize, const int zSize,
           const double xSpace, const double ySpace, const double zSpace,
           const bool twoDOnlyFlag=true, const bool zeroBorderFlag=false,
           const bool verboseFlag=false )
      : Scale<T>( inData, xSize, ySize, zSize, xSpace, ySpace, zSpace,
                  twoDOnlyFlag, zeroBorderFlag, verboseFlag )
  {
      cout << "GScale::GScale" << endl;
      mOutData = (unsigned char*) malloc( xSize * ySize * zSize
                                          * sizeof(unsigned char) );
      assert( mOutData );
  }
  //----------------------------------------------------------------------
  ~GScale ( ) {
      cout << "GScale::~GScale" << endl;
      if (mOutData!=NULL)  {  free(mOutData);  mOutData=NULL;  }
  }
  //----------------------------------------------------------------------
  deque<point3d*> estimateScale ( const int cx, const int cy, const int cz )
  {
      //estimate the scale set at the given point (by growing the set/region)
      if (mVerboseFlag) {
          cout << "GScale::estimateScale: center=(" << cx << "," << cy
               << "," << cz << ")" << endl;
      }
      point3d*  p = new point3d(cx, cy, cz);
      deque<point3d*>  open;
      open.push_back(p);

      //indicate that it is either on open or closed
      memset(mOutData, 0, mXSize * mYSize * mZSize * sizeof(unsigned char));
      mOutData[index(cx,cy,cz)] = 1;

      deque<point3d*>  closed;

      // while (!open.empty() && open.size()<100000 && closed.size()<100000) {
      while (!open.empty()) {
          //cout << "open=" << open.size() << " closed=" << closed.size() << endl;
          p = open.front();
          open.pop_front();
          closed.push_back(p);

          //try and grow the region by checking p's neighbors (called t below)
          tryToAdd(p->x-1, p->y,   p->z,    cx, cy, cz, &open, &closed);
          tryToAdd(p->x+1, p->y,   p->z,    cx, cy, cz, &open, &closed);
          tryToAdd(p->x,   p->y-1, p->z,    cx, cy, cz, &open, &closed);
          tryToAdd(p->x,   p->y+1, p->z,    cx, cy, cz, &open, &closed);
          if (!getTwoDOnlyFlag()) {
              tryToAdd(p->x,   p->y,   p->z-1,  cx, cy, cz, &open, &closed);
              tryToAdd(p->x,   p->y,   p->z+1,  cx, cy, cz, &open, &closed);
          }
      }

      if (mVerboseFlag) {
          int  count=0;
          for ( deque<point3d*>::iterator it=closed.begin(); it!=closed.end();
                                                             it++ )
          {
              cout << "    " << (*it)->x << "," << (*it)->y << ","
                   << (*it)->z;
              if ((++count % 6)==0)    cout << endl;
          }
          cout << "  (" << count << ")" << endl;
      }
      return closed;
  }
  //----------------------------------------------------------------------
private:
  //----------------------------------------------------------------------
  bool tryToAdd ( const int x,  const int y,  const int z,
                  const int cx, const int cy, const int cz,
                  deque<point3d*>  *open, deque<point3d*>  *closed )
  {
      if (!inBounds(x,y,z))  return false;  //in bounds?
      //already on either open or closed?
      if ( mOutData[index(x,y,z)] )    return false;
      if ( Wpsi3( abs(getInData(cx,cy,cz) - getInData(x,y,z)) ) < mTs )
          return false;

      #ifdef maxScale
          if ( sqrt( (double)((cx-x)*(cx-x) + (cy-y)*(cy-y) + (cz-z)*(cz-z)) ) > maxScale )    return false;
          // if ( abs(cx-x) > maxScale || abs(cy-y) > maxScale ||
          //      abs(cz-z) > maxScale )    return false;
      #endif

      point3d* t = new point3d(x, y, z);
      (*open).push_back(t);
      //indicate that it is either on open or closed
      mOutData[index(x,y,z)] = 1;
      return true;
  }

};

#endif
//======================================================================

