//======================================================================
// Box-Muller algorithm
//----------------------------------------------------------------------
#ifndef __BoxMuller_h
#define __BoxMuller_h

#include  <math.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <time.h>

/** \brief definition of Box-Muller algorithm class.
 *  The B-M algorithm samples random numbers from a normal/Gaussian
 *  distribution using the Box-Muller transform.
 */
class BoxMuller {
private:
    bool    mValid;  ///< has the second random number been cached?
    double  mX2;     ///< this method generates pairs of random numbers, caching the second one here.

    /** \brief this method returns a uniformly distributed random number in the range [0.0 ... 1.0]. */
    double Random ( void ) const {
        const double  r = rand();
        return r / RAND_MAX;
    }

public:
    /** \brief class constructor. */
    BoxMuller ( void ) {
        mValid = false;
        mX2    = 0;
        srand( (unsigned int)time(NULL) );
    }

    /** \brief given a mean m and std dev s, return a sample from a
     *  normal/Gaussian distribution.
     */
    double sample ( double m=0.0, double s=1.0 ) {
        // normal distribution with mean m and standard deviation s
        if (mValid) {  // we have a valid result from last call (so use it)
            mValid = false;
            return mX2 * s + m;
        }

        // make two normally distributed variates by Box-Muller transformation
        double  x1;  // first random coordinate (second one is class member)
        double  w;   // radius
        do {
            x1  = 2.0 * Random() - 1.0;  // -1..1
            mX2 = 2.0 * Random() - 1.0;  // -1..1
            w = x1*x1 + mX2*mX2;
        } while (w >= 1.0 || w < 1E-30);
        w = sqrt( log(w)*(-2.0/w) );
        x1     *= w;
        mX2    *= w;     // x1 and x2 are independent, normally distributed variables
        mValid =  true;  // cache x2 for next call
        return x1 * s + m;
    }

};

#endif
//======================================================================

