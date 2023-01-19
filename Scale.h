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
// Scale
//----------------------------------------------------------------------
#ifndef  __Scale_h
#define  __Scale_h

#include  <algorithm>
#include  <assert.h>
#ifdef WIN32_V6  //vc++ version 6 only
    #include  <iostream.h>
#else
    #include  <iostream>
#endif
#include  <stdio.h>
#include  <stdlib.h>
#include  <vector>

using namespace std;

/** \brief definition of Scale class */
template< typename T >  //type of input data
class Scale {

protected: //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  double          mTs;        //default is   0.85 (threshold on fraction of object)
  double          mTh;        //default is   3.0  (# of stddev's for determining Kpsi, global homogeneity value)
  T               mMaxScale;  //default is 500    (threshold/limit on max scale value)

  const T* const  mInData;
  const int       mXSize,  mYSize,  mZSize;
  const double    mXSpace, mYSpace, mZSpace;  ///< physical spacing (in mm)
  double          mMinSpace;
//to maintain order in constructor:
private: //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool            mTwoDOnlyFlag;    ///< true = 2D processing only; false = 3D processing.
  bool            mZeroBorderFlag;  ///< true = values outside of the extent of the data are to be considered as existing with a value of 0; false = stop when we get out of the bounds of the data.
public: // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool            mVerboseFlag;
protected: //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  unsigned long*  mZSub;
  unsigned long*  mYSub;

  double          mKpsi;  ///< global homogeneity value
  //double          mGrayMean;
  double          mMinSpace2;  //min spacing squared
  double          mXSpace2;    //x spacing squared
  double          mYSpace2;    //y spacing squared
  double          mZSpace2;    //z spacing squared
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //purposely made protected to ensure that this constructor is only used 
  // by subclasses.
  Scale ( void ) : mInData(NULL),
                   mXSize(0), mYSize(0), mZSize(0),
                   mXSpace(0), mYSpace(0), mZSpace(0)
  {
      if (mVerboseFlag)    cout << "Scale::Scale(void) ctor #0" << endl;
  }
public: // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //calculates Kpsi
  Scale ( const T* const inData,
          const int xSize, const int ySize, const int zSize,
          const double xSpace, const double ySpace, const double zSpace,
          const bool twoDOnlyFlag=true, const bool zeroBorderFlag=false,
          const bool verboseFlag=false )
      : mInData(inData),
        mXSize(xSize), mYSize(ySize), mZSize(zSize),
        mXSpace(xSpace), mYSpace(ySpace), mZSpace(zSpace),
        mTwoDOnlyFlag(twoDOnlyFlag), mZeroBorderFlag(zeroBorderFlag),
        mVerboseFlag(verboseFlag)
  {
      if (mVerboseFlag)    cout << "Scale::Scale ctor #1" << endl;
      init();
      getHomogeneity();
      //if (mVerboseFlag)
          cout << "Scale: 2d=" << mTwoDOnlyFlag
               << ", zero border=" << mZeroBorderFlag << endl;
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //allows user to specify Kpsi
  Scale ( const T* const inData,
          const int xSize, const int ySize, const int zSize,
          const double xSpace, const double ySpace, const double zSpace,
          const bool twoDOnlyFlag, const bool zeroBorderFlag,
          const bool verboseFlag, const double kPsi )
      : mInData(inData),
        mXSize(xSize), mYSize(ySize), mZSize(zSize),
        mXSpace(xSpace), mYSpace(ySpace), mZSpace(zSpace),
        mTwoDOnlyFlag(twoDOnlyFlag), mZeroBorderFlag(zeroBorderFlag),
        mVerboseFlag(verboseFlag), mKpsi(kPsi)
  {
      if (mVerboseFlag)    cout << "Scale::Scale 2" << endl;
      init();
      //if (mVerboseFlag)
          cout << "Scale: 2d=" << mTwoDOnlyFlag
               << ", zero border=" << mZeroBorderFlag 
               << ", Kpsi=" << mKpsi << endl;
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ~Scale ( ) {
      cout << "~Scale" << endl;
      if (mVerboseFlag)    cout << "~Scale" << endl;
      if (mZSub!=NULL)  {  free(mZSub);  mZSub=NULL;  }
      if (mYSub!=NULL)  {  free(mYSub);  mYSub=NULL;  }
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void   setMaxScale    ( const T max )     {  mMaxScale = max;         }
  void   setTs          ( const double ts ) {  mTs = ts;                }
  void   setVerboseFlag ( const bool b )    {  mVerboseFlag = b;        }

  double getKpsi           ( void ) const   {  return mKpsi;            }
  int    getMaxScale       ( void ) const   {  return mMaxScale;        }
  double getTh             ( void ) const   {  return mTh;              }
  double getTs             ( void ) const   {  return mTs;              }
  bool   getTwoDOnlyFlag   ( void ) const   {  return mTwoDOnlyFlag;    }
  bool   getVerboseFlag    ( void ) const   {  return mVerboseFlag;     }
  bool   getZeroBorderFlag ( void ) const   {  return mZeroBorderFlag;  }
private: //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void init ( void ) {
      if (mVerboseFlag)    cout << "Scale::init" << endl;
      assert( mInData!=NULL );
      mXSpace2   = mXSpace * mXSpace;
      mYSpace2   = mYSpace * mYSpace;
      mZSpace2   = mZSpace * mZSpace;

      mMinSpace = mXSpace;
      if (mYSpace<mMinSpace)  mMinSpace = mYSpace;
      if (mZSpace<mMinSpace)  mMinSpace = mZSpace;
      mMinSpace2 = mMinSpace * mMinSpace;

      mTs       =   0.85;
      mTh       =   3.0;
      mMaxScale = 500;

      mZSub = (unsigned long*)malloc( mZSize * sizeof(unsigned long) );
      assert( mZSub != NULL );
      for (int z=0; z<mZSize; z++) {
          mZSub[z] = z * mXSize * mYSize;
      }

      mYSub = (unsigned long*)malloc( mYSize * sizeof(unsigned long) );
      for (int y=0; y<mYSize; y++) {
          mYSub[y] = y * mXSize;
      }
  }
protected: //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //From the IEEE TMI 20(11), 2001 paper, p. 1143, where it is called,
  // FO_k,v(c).  This is the correct verion (as opposed to the CVIU paper 
  // which contains an error).
  //
  //Inputs:  (cx,cy,cz) is the location of the center
  //         k is the size (radius) of the ball
  //Returns: the average homogeneity of the "rind" of the growing ball
  //
  double fractionOfObject ( const int cx, const int cy, const int cz,
                            const int k )
  {
      int     count=0;
      double  sum=0.0;

      if (mTwoDOnlyFlag) {
          const int z=cz;
              for (int y=cy-k; y<=cy+k; y++) {
                  for (int x=cx-k; x<=cx+k; x++) {
                      //does the ball extend beyond the data?
                      if (inBounds(x,y,z)) {
                          if (B(cx,cy,cz, x,y,z, k) && !B(cx,cy,cz, x,y,z, k-1)) {
                              ++count;
                              sum += Wpsi3( abs(getInData(cx,cy,cz)
                                                - getInData(x,y,z)) );
                          }
                      } else {
                          if (!mZeroBorderFlag)    continue;
                          //pretend out-of-bounds is zero
                          if (B(cx,cy,cz, x,y,z, k) && !B(cx,cy,cz, x,y,z, k-1)) {
                              ++count;
                              sum += Wpsi3( getInData(cx,cy,cz) );
                          }
                      }
                  }
              }
      } else {
          for (int z=cz-k; z<=cz+k; z++) {
              for (int y=cy-k; y<=cy+k; y++) {
                  for (int x=cx-k; x<=cx+k; x++) {
                      //does the ball extend beyond the data?
                      if (inBounds(x,y,z)) {
                          if (B(cx,cy,cz, x,y,z, k) && !B(cx,cy,cz, x,y,z, k-1)) {
                              ++count;
                              sum += Wpsi3( abs(getInData(cx,cy,cz)
                                                - getInData(x,y,z)) );
                          }
                      } else {
                          if (!mZeroBorderFlag)    continue;
                          //pretend out-of-bounds is zero
                          if (B(cx,cy,cz, x,y,z, k) && !B(cx,cy,cz, x,y,z, k-1)) {
                              ++count;
                              sum += Wpsi3( getInData(cx,cy,cz) );
                          }
                      }
                  }
              }
          }
      }
  
      if (count==0) {
          cout << "fractionOfObject: count=0" << endl;
          return 0.0;
      }
      return (sum/count);
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  inline bool inBounds ( const int x, const int y, const int z ) {
      if (x<0 || y<0 || z<0)  return false;
      if (x>=mXSize || y>=mYSize || z>=mZSize)  return false;
      return true;
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  inline unsigned long index ( const int x, const int y, const int z ) {
      assert( inBounds(x,y,z) );
      return mZSub[z] + mYSub[y] + x;
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  inline T getInData ( const int x,  const int y, const int z ) {
      return mInData[index(x,y,z)];
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //calculates Kpsi
  void getHomogeneity ( void ) {
    // if (mVerboseFlag)
        cout << "getHomogeneity" << endl;
    //build a list of the mag of the neighboring differences
    vector<unsigned short>  vi;
    //mGrayMean=0.0;  //to calc the mean of the gray values (not diffs)
    //long  count=0;
    if (mZSize<2 && !mTwoDOnlyFlag) {
        cout << "automatically switching to 2d for a single slice." << endl;
        mTwoDOnlyFlag = true;
    }
    if (mTwoDOnlyFlag) {
        for (int z=0; z<mZSize; z++) {
            if (mVerboseFlag)  cout << "." << flush;
            for (int y=1; y<mYSize-1; y++) {
                for (int x=1; x<mXSize-1; x++) {
                    const int  c = (int)getInData(x,y,z);
                    //mGrayMean += c;    ++count;
                    const int dz=0;  //for in-plane only
                    for (int dy=-1; dy<=1; dy++) {
                        for (int dx=-1; dx<=1; dx++) {
                            if (dx==0 && dy==0 && dz==0)    continue;
                            const int  d = (int)getInData(x+dx, y+dy, z+dz);
                            int v=c-d;
                            if (v<0)  v = -v;  //abs
                            vi.push_back(v);
                        }
                    }
                }
            }
        }
    } else {
        for (int z=1; z<mZSize-1; z++) {
            if (mVerboseFlag)  cout << "." << flush;
            for (int y=1; y<mYSize-1; y++) {
                for (int x=1; x<mXSize-1; x++) {
                    const int  c = (int)getInData(x,y,z);
                    //mGrayMean += c;    ++count;
                    if (mTwoDOnlyFlag) {
                        const int dz=0;  //for in-plane only
                        for (int dy=-1; dy<=1; dy++) {
                            for (int dx=-1; dx<=1; dx++) {
                                if (dx==0 && dy==0 && dz==0)    continue;
                                const int  d = (int)getInData(x+dx, y+dy, z+dz);
                                int v=c-d;
                                if (v<0)  v = -v;  //abs
                                vi.push_back(v);
                            }
                        }
                    } else {
                        //all directions (not in-plane only)
                        for (int dz=-1; dz<=1; dz++) {
                            for (int dy=-1; dy<=1; dy++) {
                                for (int dx=-1; dx<=1; dx++) {
                                    if (dx==0 && dy==0 && dz==0)    continue;
                                    const int  d = (int)getInData(x+dx, y+dy, z+dz);
                                    int v=c-d;
                                    if (v<0)  v = -v;  //abs
                                    vi.push_back(v);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //assert(count>0);
    //mGrayMean /= count;

    cout << endl << "    size=" << vi.size() << endl;
    //for (vector<int>::iterator it=vi.begin(); it!=vi.end(); it++) {
    //    cout << "it=" << *it << endl;
    //}

    //calc mean & std dev of bottom 90% (of neighboring difference mags)
    const int  N = (int)(0.90*vi.size());
    assert( N>1 );
    if (mVerboseFlag)  cout << "    sorting" << endl;
    sort(vi.begin(), vi.end());

    cout << "    min=" << vi[0] << endl;
    cout << "    90%=" << vi[N-1] << endl;
    cout << "    max=" << vi[vi.size()-1] << endl;
    //for (vector<int>::iterator it=vi.begin(); it!=vi.end(); it++) {
    //    cout << "it=" << *it << endl;
    //}
    double magDiffMean = 0.0;
    int  i;
    for (i=0; i<N; i++)    magDiffMean += vi[i];
    magDiffMean /= N;

    double stddev=0.0;
    for (i=0; i<N; i++)    stddev += (vi[i]-magDiffMean)*(vi[i]-magDiffMean);
    stddev /= N-1.0;
    stddev = sqrt(stddev);

    cout << "    for the bottom 90% of mag of the neighboring differences:"
         << endl
         << "        mean=" << magDiffMean << " stddev=" << stddev << endl;
    mKpsi = magDiffMean + mTh * stddev;
    //if (mVerboseFlag)
        cout << "        Kpsi=" << mKpsi << endl;
    if (mKpsi==0.0) {
        const int  N = vi.size();
        assert( N>1 );
        double magDiffMean = 0.0;
        int  i;
        for (i=0; i<N; i++)    magDiffMean += vi[i];
        magDiffMean /= N;

        double stddev=0.0;
        for (i=0; i<N; i++)
            stddev += (vi[i]-magDiffMean)*(vi[i]-magDiffMean);
        stddev /= N-1.0;
        stddev = sqrt(stddev);

        cout << "    for 100% of the mag of the neighboring differences:"
             << endl
             << "        mean=" << magDiffMean << " stddev=" << stddev << endl;
        mKpsi = magDiffMean + mTh * stddev;
        //if (mVerboseFlag)
            cout << "        Kpsi=" << mKpsi << endl;
    }
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  inline double Wpsi3 ( const T x ) {
       assert( mKpsi!=0.0 );
       return exp(-(((double)x*x)/(2.0 * mKpsi * mKpsi)));
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  inline bool B ( const int cx, const int cy, const int cz,
                  const int ex, const int ey, const int ez,
                  const int k ) const
  {
      //is a given point (ex,ey,ez) within a ball centered at (cx,cy,cz) with
      //the given radius (k*mMinSpace)?
      //assert( inBounds(ex,ey,ez) );  //not necessary w/ mZeroBorderFlag
      const double  dx    = (cx-ex) * mXSpace;
      const double  dy    = (cy-ey) * mYSpace;
      const double  dz    = (cz-ez) * mZSpace;
      const double  dist  = sqrt( dx*dx + dy*dy + dz*dz );
      const double  bound = k * mMinSpace;
      return (dist <= bound);
      #if 0
          //Punam's version (IMHO, less clear and an additional *//).
          const double  dx   = cx-ex;
          const double  dy   = cy-ey;
          const double  dz   = cz-ez;
          const double  sum  = mXSpace2 * (dx * dx) / mMinSpace2 +
                               mYSpace2 * (dy * dy) / mMinSpace2 +
                               mZSpace2 * (dz * dz) / mMinSpace2;
          return (sqrt(sum) <= k);
      #endif
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
};

#endif
//======================================================================
