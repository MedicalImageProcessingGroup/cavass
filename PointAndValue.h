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


// file: PointAndValue.h

#ifndef __PointAndValue_h
#define __PointAndValue_h

class PointAndValue {
  private:
    const int  mX, mY, mZ;  ///< 3d point
    const int  mValue;      ///< value at that point
  public:
    PointAndValue ( const int x, const int y, const int z, const int value )
        : mX(x), mY(y), mZ(z), mValue(value) {
    }

    int getX     ( ) const {  return mX;      }
    int getY     ( ) const {  return mY;      }
    int getZ     ( ) const {  return mZ;      }
    int getValue ( ) const {  return mValue;  }
};

#endif

