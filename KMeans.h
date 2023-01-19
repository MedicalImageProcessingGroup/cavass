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
/**
 * \file   KMeans.h
 * \brief  Definition and implementation of K Means segmentation.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __KMeans_h
#define  __KMeans_h

#include  <stdlib.h>
#include  <time.h>
#include  <vector>

using namespace std;
/** \brief Definition and implementation of K Means segmentation.
 */
template< typename T >
class KMeans1D {
  public:
    const int  mK;           ///< number of classes
    int        mIteration;   ///< number of iterations
    double*    mCenters;     ///< location of cluster centers
    double     mDelta;       ///< last change
    typedef  vector< T >  Vector;
    Vector*    mPartitions;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /** \brief ctor which also performs k means segmentation.
     *  \param k  number of classes
     *  \param L  pointer to input gray pixel values
     *  \param length  # of input gray pixel values
     */
    KMeans1D ( const int k, const T* const L, const int length )
        : mK(k)
    {
        //init the centers
        mCenters = new double[k];    assert( mCenters!=NULL );
        double  min = L[0];
        double  max = L[0];
        int  i;
        for (i=0; i<length; i++) {
            if (L[i]<min)  min=L[i];
            if (L[i]>max)  max=L[i];
        }
        //pick k points (initial cluster centers) at random
        srand( time(NULL) );
        for (i=0; i<k; i++) {
            mCenters[i] = rand() * (max-min+1) / RAND_MAX + min;
        }

        //init the partitions
        mPartitions = new Vector[k];    assert( mPartitions!=NULL );
        for (i=0; i<length; i++) {
            mPartitions[0].push_back( L[i] );
        }

        for (mIteration=1; mIteration<=1000; mIteration++) {
            wxLogMessage( "iteration=%d/%d.", mIteration, 1000 );
            wxYield();
            //redistribute the contents of mPartitions according
            // to the cluster centers
            Vector*  newPartitions = new Vector[k];
            assert( newPartitions!=NULL );
            int  j;
            for (j=0; j<k; j++) {
                while (mPartitions[j].size() > 0) {
                    T  t = mPartitions[j].back();
                    mPartitions[j].pop_back();

                    int     best = 0;
                    double  bestValue = fabs( mCenters[0] - t );
                    for (int m=1; m<k; m++) {
                        const double  temp = fabs( mCenters[m] - t );
                        if (temp<bestValue) {
                            best = m;
                            bestValue = temp;
                        }
                    }
                    newPartitions[best].push_back( t );
                }
            }
            //now that we've re-partitioned the data, the next step
            // is to recalculate the cluster centers
            mDelta = 0.0;
            for (j=0; j<k; j++) {
                double  sum   = 0.0;
                //for ( Vector::iterator vi=newPartitions[j].begin();
                for ( typename Vector::iterator vi=newPartitions[j].begin();
                      vi!=newPartitions[j].end(); vi++ )
                {
                    sum += *vi;
                }
                sum    /= newPartitions[j].size();
                mDelta += fabs(mCenters[j]-sum);
                mCenters[j] = sum;
            }
            //finally, update the current set of partitions
#ifndef WIN32
            //delete mPartitions;
#endif
            mPartitions = newPartitions;

            //cout << "k-means: " << mIteration << ": delta=" << mDelta << endl;
            if (mDelta<0.0001*(max-min))  break;
        }
        //if (k==2) {
        //    cout << "result: " << (mCenters[1]-mCenters[0])/2+mCenters[0] << endl;
        //}
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ~KMeans1D ( void ) {
        if (mCenters!=NULL) {
            delete mCenters;
            mCenters=NULL;
        }
#if ! defined (WIN32) && ! defined (_WIN32)
        if (mPartitions!=NULL) {
            //delete mPartitions;
            mPartitions=NULL;
        }
#endif
    }

};

#endif
//======================================================================
