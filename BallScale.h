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
// BallScale
//----------------------------------------------------------------------
#ifndef  __BallScale_h
#define  __BallScale_h

#include  "Scale.h"

using namespace std;

/** \brief definition of Ball Scale class */
template< typename T >  //type of input data
class BallScale : public Scale< T > {

protected:
  //made protected to ensure that this constructor is only used by subclasses.
  BallScale ( void ) { }
  //----------------------------------------------------------------------
public:
  //----------------------------------------------------------------------
  BallScale ( const T* const inData,
              const int xSize,     const int ySize,     const int zSize,
              const double xSpace, const double ySpace, const double zSpace,
              const bool twoDOnlyFlag=true, const bool zeroBorderFlag=false,
              const bool verboseFlag=false )
      : Scale<T>( inData, xSize, ySize, zSize, xSpace, ySpace, zSpace,
                  twoDOnlyFlag, zeroBorderFlag, verboseFlag )
  {
  }
  //----------------------------------------------------------------------
  BallScale ( const T* const inData,
              const int xSize, const int ySize, const int zSize,
              const double xSpace, const double ySpace, const double zSpace,
              const bool twoDOnlyFlag, const bool zeroBorderFlag,
              const bool verboseFlag, const double kPsi )
      : Scale<T>( inData, xSize, ySize, zSize, xSpace, ySpace, zSpace,
                  twoDOnlyFlag, zeroBorderFlag, verboseFlag, kPsi )
  {
  }
  //----------------------------------------------------------------------
  ~BallScale ( void ) {
      cout << "~BallScale" << endl;
  }
  //----------------------------------------------------------------------
  using Scale<T>::fractionOfObject;
  using Scale<T>::mTs;
  using Scale<T>::mMaxScale;
  using Scale<T>::mVerboseFlag;

  /** \brief estimate the scale at the given point by growing a ball. */
  int estimateScale ( const int cx, const int cy, const int cz ) {
      int  k;
      for (k=1; fractionOfObject(cx, cy, cz, k)>=mTs && k<=mMaxScale; k++) {
          if (mVerboseFlag) {
              cout << "estimateScale: (" << cx << "," << cy << "," << cz
                   << ") k=" << k << " fraction="
                   << fractionOfObject(cx,cy,cz,k) << endl;
          }
      }
      if (mVerboseFlag && k>mMaxScale)
          cout << "estimateScale: max scale limit exceeded." << endl;
      return k-1;
  }

};

#endif
//======================================================================
