/*
  Copyright 1993-2009 Medical Image Processing Group
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

/**
 * \file   fft.cpp
 * \brief  fft algorithm from NRC, section 12.2, p. 507,
 *         least squares fit from NRC, p. 665, and
 *         a function that returns true if a given number is a power of 2.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
#include  <assert.h>
#include  <math.h>
#include  <stdlib.h>
#include  "fft.h"

#define  SWAP(a,b)  tempr=(a);  (a)=(b);  (b)=tempr
//----------------------------------------------------------------------
bool isPowerOf2 ( const unsigned long value ) {
    static const unsigned long p2[] = {
      (unsigned long)(1<< 0), (unsigned long)(1<< 1), (unsigned long)(1<< 2), (unsigned long)(1<< 3), (unsigned long)(1<< 4), (unsigned long)(1<< 5), (unsigned long)(1<< 6), (unsigned long)(1<< 7), (unsigned long)(1<< 8), (unsigned long)(1<< 9),
      (unsigned long)(1<<10), (unsigned long)(1<<11), (unsigned long)(1<<12), (unsigned long)(1<<13), (unsigned long)(1<<14), (unsigned long)(1<<15), (unsigned long)(1<<16), (unsigned long)(1<<17), (unsigned long)(1<<18), (unsigned long)(1<<19),
      (unsigned long)(1<<20), (unsigned long)(1<<21), (unsigned long)(1<<22), (unsigned long)(1<<23), (unsigned long)(1<<24), (unsigned long)(1<<25), (unsigned long)(1<<26), (unsigned long)(1<<27), (unsigned long)(1<<28), (unsigned long)(1<<29),
      (unsigned long)(1<<30), (unsigned long)(1<<31)
    };
    for (int i=0; i<32; i++) {
        if (value == p2[i])    return true;
    }
    return false;
}
//----------------------------------------------------------------------
/**
 * \brief least squares fit.
 * from Numerical Recipes in C, p. 665.
 * Given a set of data points x[1..ndata],y[1..ndata], fit them to a 
 * straight line y = a + bx by minimizing chi-square .
 *
 * Returned are a, b and their respective probable uncertainties siga and 
 * sigb, the chi-square chi2, and the goodness-of-fit probability q (that
 * the fit would have chi-square this large or larger).
 *
 * Since the std dev's of the individual (x,y) are unavailable, q 
 * is returned as 1.0 and the normalization of chi2 is to unit standard 
 * deviation on all points.
 */
void fit ( double* x, double* y, const int ndata, double& a, double& b,
           double& siga, double& sigb, double& chi2, double& q )
{
    --x;    --y;  //because Numerical Recipes arrays are [1..ndata].

    b = 0.0;
    double  sx=0.0, sy=0.0;
    int  i;
    for (i=1; i<=ndata; i++) {
        sx += x[i];
        sy += y[i];
    }
    const double  ss = ndata;
    const double  sxoss = sx/ss;
    double  st2 = 0.0;
    for (i=1; i<=ndata; i++) {
        const double  t = x[i]-sxoss;
        st2 += t*t;
        b += t*y[i];
    }
    b /= st2;
    a = (sy-sx*b)/ss;
    siga = sqrt( (1.0+sx*sx/(ss*st2))/ss );
    sigb = sqrt( 1.0/st2 );
    chi2 = 0.0;
    for (i=1; i<=ndata; i++) {
        const double  tmp = y[i]-a-b*x[i];
        chi2 += tmp*tmp;
    }
    q = 1.0;
    const double  sigdat = sqrt( chi2/(ndata-2) );
    siga *= sigdat;
    sigb *= sigdat;
}
//----------------------------------------------------------------------
void fit ( vector< double >* xv, vector< double >* yv, const int ndata,
           double& a, double& b,
           double& siga, double& sigb, double& chi2, double& q )
{
    assert( ndata>=0 );
    assert( xv->size()==yv->size() && xv->size()==(unsigned int)ndata );
    double*  x = (double*)malloc( xv->size()*sizeof(double) );
    double*  y = (double*)malloc( yv->size()*sizeof(double) );
    int  i;
    vector< double >::iterator  it;
    for (it=xv->begin(),i=0 ; it!=xv->end(); it++,i++) {
        x[i] = *it;
    }
    for (it=yv->begin(),i=0 ; it!=yv->end(); it++,i++) {
        y[i] = *it;
    }
    fit( x, y, ndata, a, b, siga, sigb, chi2, q );
    free(y);    y=NULL;
    free(x);    x=NULL;
}
//----------------------------------------------------------------------
/**
 * \brief Replaces data[1..2*nn] by its discrete Fourier transform,
 * if isign is input as 1; or replaces data[1..2*nn] by nn times its 
 * inverse discrete Fourier transform, if isign is input as -1. data is
 * a complex array of length nn or, equivalently, a real array of length
 * 2*nn.  nn MUST be an integer power of 2 (this is checked!).
 */
void dfour1 ( double data[], unsigned long nn, int isign ) {
    --data;  //because NRC arrays are subscripted [1..N] (as in fortran)
             // instead of C++ standard [0..N-1]
    if (!isPowerOf2(nn))  return;

    unsigned long  n, mmax, m, j, istep, i;
    double  wtemp, wr, wpr, wpi, wi, theta;   //trigonometric recurrences
    double  tempr, tempi;

    n = nn << 1;
    j = 1;
    for (i=1; i<n; i+=2) { //this is the bit-reversal section of the routine.
        if (j > i) {
            SWAP(data[j],   data[i]);  //exchange the two complex numbers.
            SWAP(data[j+1], data[i+1]);
        }
        m=n >> 1;
        while (m>=2 && j>m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }

    //begin the Danielson-Lanczos section of the routine.
    mmax = 2;
    while (n > mmax) {  //outer loop executed log2 nn times.
        istep = mmax << 1;
        theta = isign*(6.28318530717959/mmax);  //initialize the trigonometric
                                                // recurrence.
        wtemp = sin(0.5*theta);
        wpr = -2.0*wtemp*wtemp;
        wpi = sin(theta);
        wr = 1.0;
        wi = 0.0;
        for (m=1; m<mmax; m+=2) {  //here are the two nested inner loops.
            for (i=m; i<=n; i+=istep) {
                j = i+mmax;  //this is the Danielson-Lanczos formula:
                tempr = wr*data[j]-wi*data[j+1];
                tempi = wr*data[j+1]+wi*data[j];
                data[j] = data[i]-tempr;
                data[j+1] = data[i+1]-tempi;
                data[i] += tempr;
                data[i+1] += tempi;
            }
            wr = (wtemp=wr)*wpr-wi*wpi+wr;  //trigonometric recurrence.
            wi = wi*wpr+wtemp*wpi+wi;
        }
        mmax = istep;
    }
}
//----------------------------------------------------------------------
