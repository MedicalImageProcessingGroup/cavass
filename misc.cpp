/*
  Copyright 1993-2012 Medical Image Processing Group
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
 * \file   misc.cpp
 * \brief  miscellaneous functions.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#include  <assert.h>
#include  <deque>
#include  <iostream>
#include  <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif
#include  <stdio.h>
#include  <stdlib.h>
#include  "misc.h"

using namespace std;
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// E. W. Weisstein, "Point-Line Distance--2-Dimensional."
// From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html

double distancePointLine2D ( const double x0, const double y0,   //point
                             const double x1, const double y1,   //line
                             const double x2, const double y2 )  //line
{
    const double numerator   = fabs( (x2-x1)*(y1-y0) - (x1-x0)*(y2-y1) );
    const double denominator = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    if ( denominator==0.0) {
        assert( denominator!=0.0 );
    }
    return numerator / denominator;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//determine the centroid of a set of points
void determineCentroid ( double& cx, double& cy, double& cz,
    deque<point3d*> list, const bool verboseFlag )
{
    cx=0.0;  cy=0.0;  cz=0.0;
    for ( deque<point3d*>::iterator it=list.begin(); it!=list.end(); it++ ) {
        cx += (*it)->x;
        cy += (*it)->y;
        cz += (*it)->z;
    }
    if (list.size()>0) {
        cx /= list.size();  cy /= list.size();  cz /= list.size();
    }
    if (verboseFlag) {
        cout << "::determineCentroid: (" << cx << "," << cy << ","
             << cz << ") " << list.size() << " points." << endl;
    }
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//determine the centroid of a set of points
void determineCentroid ( int& cx, int& cy, int& cz,
    deque<point3d*> list, const bool verboseFlag )
{
    double  x=0.0, y=0.0, z=0.0;
    ::determineCentroid( x, y, z, list, verboseFlag );
    cx = (int)(x+0.5);
    cy = (int)(y+0.5);
    cz = (int)(z+0.5);
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// from p. 84-9 of Weeks' book (needs to be cleaned up to work for
// degenerative cases (i.e., horizontal and vertical))

void get2DEigen ( deque<point3d*> list, double& ex, double& ey,
                  const bool verboseFlag )
{
    ex = ey = 0.0;
    double  cx, cy, cz;
    ::determineCentroid( cx, cy, cz, list, verboseFlag );
    const double  Mx2=cx*cx, Mxy=cx*cy, My2=cy*cy;
    double  C1=0.0, C2=0.0, C4=0.0;  //note:  c2 and c3 are the same
    for ( deque<point3d*>::iterator it=list.begin(); it!=list.end(); it++ ) {
        C1 += (*it)->x * (*it)->x - Mx2;
        C2 += (*it)->x * (*it)->y - Mxy;
        C4 += (*it)->y * (*it)->y - My2;
    }
    if (list.size()<1) {
        cout << "::get2DEigen: empty list of points!" << endl;
        return;
    }
    C1 /= list.size();
    C2 /= list.size();
    const double  C3 = C2;
    C4 /= list.size();
    const double  lambdaMax = (C1+C4+sqrt((C1+C4)*(C1+C4)-4.0*(C1*C4-C2*C3)))
                              / 2.0;
    const double  e1y       = (C1-lambdaMax) / C2;
    const double  theta     = atan( e1y );

    if (verboseFlag) {
        cout << "::get2DEigen: lambdaMax = " << lambdaMax << endl
             << "              [ 1 "         << e1y << "]" << endl
             << "                  theta = " << theta     << endl;
    }

    ::jacobi2D( C1, C2,
                C3, C4,
                ex, ey );
    // ex=1.0;  ey = e1y;    cout << "using Weeks' pca" << endl;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//from nrc, p. 467-9.

#define  ROTATE(a,i,j,k,l)  g = a[i][j];              \
                            h = a[k][l];              \
                            a[i][j] = g-s*(h+g*tau);  \
                            a[k][l] = h+s*(g-h*tau);

// Computes all eigenvalues and eigenvectors of a real symmetric matrix
// a[1..n][1..n].  On output, elements of 'a' above the diagonal are
// destroyed.  d[1..n] returns the eigenvalues of 'a'.  v[1..n][1..n] 
// is a matrix whose columns contain, on output, the normalized 
// eigenvectors of 'a'.  'nrot' returns the number of Jacobi rotations
// that were required.

//special case for 2d:
void jacobi ( double a[3][3], double d[3], double v[3][3], int& nrot ) {
    const int  n=2;
    const int  maxIterations=50;
    int  ip;
    for (ip=1; ip<=n; ip++) {    //Initialize to the identity matrix.
        for (int iq=1; iq<=n; iq++)    v[ip][iq]=0.0;
        v[ip][ip]=1.0;
    }

    double  b[n+1], z[n+1];
    for (ip=1; ip<=n; ip++) {    //Initialize b and d to the diagonal of a.
        b[ip] = d[ip] = a[ip][ip];
        z[ip] = 0.0;               //This vector will accumulate terms of the
                                   // form tapq as in equation (11.1.14).
    }

    nrot=0;
    for (int i=1; i<=maxIterations; i++) {    //may want to increase this limit.
        double  sm=0.0;
        for (ip=1; ip<=n-1; ip++) {    //Sum off-diagonal elements.
            for (int iq=ip+1; iq<=n; iq++)    sm += fabs(a[ip][iq]);
        }
        if (sm == 0.0) {    //The normal return, which relies on quadratic
                            // convergence to machine underflow.
            return;  //only good way out
        }

        double  tresh;
        if (i < 4)    tresh=0.2*sm/(n*n);    //...on the first three sweeps.
        else          tresh=0.0;             //...thereafter.
        for (ip=1; ip<=n-1; ip++) {
            for (int iq=ip+1; iq<=n; iq++) {
                double  g = 100.0 * fabs(a[ip][iq]);
                //After four sweeps, skip the rotation if the off-diagonal 
                // element is small.
                if ( i > 4 && (fabs(d[ip])+g) == fabs(d[ip])
                           && (fabs(d[iq])+g) == fabs(d[iq]) )
                    a[ip][iq]=0.0;
                else if (fabs(a[ip][iq]) > tresh) {
                    double  h = d[iq]-d[ip];
                    double  t;
                    if ((fabs(h)+g) == fabs(h))
                        t = (a[ip][iq])/h;    //t=1/(2theta)
                    else {
                        const double  theta = 0.5*h/(a[ip][iq]);    //Equation (11.1.10).
                        t = 1.0/(fabs(theta)+sqrt(1.0+theta*theta));
                        if (theta < 0.0)    t = -t;
                    }

                    const double  c   = 1.0/sqrt(1+t*t);
                    const double  s   = t*c;
                    const double  tau = s/(1.0+c);
                    h = t*a[ip][iq];
                    z[ip] -= h;
                    z[iq] += h;
                    d[ip] -= h;
                    d[iq] += h;
                    a[ip][iq] = 0.0;

                    int  j;
                    for (j=1; j<=ip-1; j++) {    //Case of rotations 1<=j<p.
                        ROTATE(a,j,ip,j,iq);
                    }
                    for (j=ip+1; j<=iq-1; j++) { //Case of rotations p<j<q.
                        ROTATE(a,ip,j,j,iq);
                    }
                    for (j=iq+1; j<=n; j++) {    //Case of rotations q<j<=n.
                        ROTATE(a,ip,j,iq,j);
                    }
                    for (j=1; j<=n; j++) {
                        ROTATE(v,j,ip,j,iq);
                    }
                    ++nrot;
                }
            }
        }

        for (ip=1; ip<=n; ip++) {
            b[ip] += z[ip];
            d[ip]=b[ip];    //Update d with the sum of tapq,
            z[ip]=0.0;      //and reinitialize z.
        }
    }

    printf( "::jacobi: Too many iterations. \n" );
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
void jacobi ( double **a, int n, double d[], double **v, int *nrot ) {
    const int  maxIterations=50;
    int     j, iq, ip, i;
    double  tresh, theta, tau, t, sm, s, h, g, c, *b, *z;

    b = (double*)malloc((n+1)*sizeof(double));  //b=vector(1,n);
    assert(b!=NULL);
    z = (double*)malloc((n+1)*sizeof(double));  //z=vector(1,n);
    assert(z!=NULL);

    for (ip=1; ip<=n; ip++) {    //Initialize to the identity matrix.
        for (iq=1; iq<=n; iq++)    v[ip][iq]=0.0;
        v[ip][ip]=1.0;
    }

    for (ip=1; ip<=n; ip++) {    //Initialize b and d to the diagonal of a.
        b[ip]=d[ip]=a[ip][ip];
        z[ip]=0.0;               //This vector will accumulate terms of the
                                 // form tapq as in equation (11.1.14).
    }

    *nrot=0;
    for (i=1; i<=maxIterations; i++) {    //may want to increase this limit.
        sm=0.0;
        for (ip=1; ip<=n-1; ip++) {    //Sum off-diagonal elements.
            for (iq=ip+1; iq<=n; iq++)    sm += fabs(a[ip][iq]);
        }
        if (sm == 0.0) {    //The normal return, which relies on quadratic
                            // convergence to machine underflow.
            free(z);  z=NULL;  //free_vector(z,1,n);
            free(b);  b=NULL;  //free_vector(b,1,n);
            return;  //only good way out
        }

        if (i < 4)    tresh=0.2*sm/(n*n);    //...on the first three sweeps.
        else          tresh=0.0;             //...thereafter.
        for (ip=1; ip<=n-1; ip++) {
            for (iq=ip+1; iq<=n; iq++) {
                g = 100.0 * fabs(a[ip][iq]);
                //After four sweeps, skip the rotation if the off-diagonal 
                // element is small.
                if ( i > 4 && (fabs(d[ip])+g) == fabs(d[ip])
                           && (fabs(d[iq])+g) == fabs(d[iq]) )
                    a[ip][iq]=0.0;
                else if (fabs(a[ip][iq]) > tresh) {
                    h = d[iq]-d[ip];
                    if ((fabs(h)+g) == fabs(h))
                        t = (a[ip][iq])/h;    //t=1/(2theta)
                    else {
                        theta = 0.5*h/(a[ip][iq]);    //Equation (11.1.10).
                        t = 1.0/(fabs(theta)+sqrt(1.0+theta*theta));
                        if (theta < 0.0)    t = -t;
                    }

                    c = 1.0/sqrt(1+t*t);
                    s = t*c;
                    tau = s/(1.0+c);
                    h = t*a[ip][iq];
                    z[ip] -= h;
                    z[iq] += h;
                    d[ip] -= h;
                    d[iq] += h;
                    a[ip][iq] = 0.0;

                    for (j=1; j<=ip-1; j++) {    //Case of rotations 1<=j<p.
                        ROTATE(a,j,ip,j,iq);
                    }
                    for (j=ip+1; j<=iq-1; j++) { //Case of rotations p<j<q.
                        ROTATE(a,ip,j,j,iq);
                    }
                    for (j=iq+1; j<=n; j++) {    //Case of rotations q<j<=n.
                        ROTATE(a,ip,j,iq,j);
                    }
                    for (j=1; j<=n; j++) {
                        ROTATE(v,j,ip,j,iq);
                    }
                    ++(*nrot);
                }
            }
        }

        for (ip=1; ip<=n; ip++) {
            b[ip] += z[ip];
            d[ip]=b[ip];    //Update d with the sum of tapq,
            z[ip]=0.0;      //and reinitialize z.
        }
    }

    printf( "::jacobi: Too many iterations. \n" );
    free(z);  z=NULL;  //free_vector(z,1,n);
    free(b);  b=NULL;  //free_vector(b,1,n);
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static double atan2_to_degrees ( const double y, const double x ) {
    const double  a = atan2( y, x );
    // return a*180.0 / M_PI;
    if (a>=0)    return a*180.0 / M_PI;
    return 180.0 + a*180.0 / M_PI;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
void jacobi2D ( const double a11, const double a12,
                const double a21, const double a22,
                double& ex,       double& ey )
{
    ex = ey = 0.0;
    const int  n=2;  //2d

    double** a = (double**)malloc( (n+1) * sizeof(double*) );
    assert( a!=NULL );

    double** v = (double**)malloc( (n+1) * sizeof(double*) );
    assert( v!=NULL );
    int  i;
    for (i=0; i<=n; i++) {
        a[i] = (double*)malloc( (n+1) * sizeof(double) );
        assert( a[i]!=NULL );
        v[i] = (double*)malloc( (n+1) * sizeof(double) );
        assert( v[i]!=NULL );
    }
    int  r;
    for (r=0; r<=n; r++) {
        for (int c=0; c<=n; c++) {
            a[r][c] = 0.0;
            v[r][c] = 0.0;
        }
    }

    double*  d = (double*)malloc( (n+1)*sizeof(double) );
    assert( d!=NULL );
    for (i=0; i<=n; i++)    d[i] = 0.0;

    int  nrot=0;
    a[1][1]=a11;    a[1][2]=a12;
    a[2][1]=a21;    a[2][2]=a22;
    jacobi( a, 2, d, v, &nrot );
    {
        double  A[3][3];
        A[1][1]=a11;    A[1][2]=a12;
        A[2][1]=a21;    A[2][2]=a22;
        double  D[3], V[3][3];
        int     NROT;
        jacobi( A, D, V, NROT );

        assert( d[1]==D[1] && d[2]==D[2] );
        assert( v[1][1]==V[1][1] && v[1][2]==V[1][2] &&
                v[2][1]==V[2][1] && v[2][2]==V[2][2] );

        cout << "::jacobi2D: jacobi2D and jacobi agree." << endl;
    }

    //pretty-print the results
    printf( "eigenvalues: " );
    for (i=1; i<=n; i++)    printf( " %f", d[i] );
    printf( "\n" );
    printf( "eigenvectors (in columns): \n" );
    for (r=1; r<=n; r++) {
        for (int c=1; c<=n; c++) {
            printf( "    %f", v[r][c] );
        }
        printf( "\n" );
    }

    eigsrt( d, v, n );
    //pretty-print the results
    printf( "sorted eigenvalues: " );
    for (i=1; i<=n; i++)    printf( " %f", d[i] );
    printf( "\n" );
    printf( "sorted eigenvectors (in columns): \n" );
    for (r=1; r<=n; r++) {
        for (int c=1; c<=n; c++) {
            printf( "    %f", v[r][c] );
        }
        printf( "\n" );
    }

    ex = v[1][1];
    ey = v[2][1];

    cout << "    angle=" << atan2_to_degrees(-ey, ex) << endl;

    free(d);    d=NULL;
    for (i=0; i<=n; i++) {
        free(a[i]);    a[i]=NULL;
        free(v[i]);    v[i]=NULL;
    }
    free(v);    v=NULL;
    free(a);    a=NULL;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
// Give the eigenvalues d[1..n] and eigenvectors v[1..n][1..n] as output 
// from jacobi (11.1) or tqli (11.3), this routine sorts the eigenvalues 
// into descending order, and rearranges the columns of v correspondingly.
// The method is straight insertion.
void eigsrt ( double d[], double **v, int n ) {
    int     k, j, i;
    double  p;
    for (i=1; i<n; i++) {
        p = d[k=i];
        for (j=i+1; j<=n; j++) {
            if (d[j] >= p)    p = d[k=j];
        }
        if (k != i) {
            d[k] = d[i];
            d[i] = p;
            for (j=1; j<=n; j++) {
                p = v[j][i];
                v[j][i] = v[j][k];
                v[j][k] = p;
            }
        }
    }
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
template<class T>
static inline T SQR ( T x )  {  return x * x;  }

static double gammq ( double a, double x ) {
    assert(0);
    return 0.0;
}

#if 0

// 2d linear least-squares fit from NRC, p. 665-5.

void fit ( double x[], double y[], int ndata, double sig[], int mwt, double *a,
    double *b, double *siga, double *sigb, double *chi2, double *q )

// Given a set of data points x[1..ndata],y[1..ndata] with individual
// standard deviations sig[1..ndata], fit them to a straight line 
// y = a + bx by minimizing chi-square.  Returned are a,b and their 
// respective probable uncertainties siga and sigb, the chi-square chi2,
// and the goodness-of-fit probability q (that the fit would have chi-square
// this large or larger).
//
// If mwt=0 on input, then the standard deviations are assumed to be 
// unavailable: q is returned as 1.0 and the normalization of chi2 is to 
// unit standard deviation on all points.

{
    assert( mwt==0 );  //because we need gammq for mwt!=0 and it's not
                       // implemented.
    int i;
    double wt, t, sxoss, sx=0.0, sy=0.0, st2=0.0, ss, sigdat;

    *b=0.0;
    if (mwt) {  //Accumulate sums ...
        ss=0.0;
        for (i=1; i<=ndata; i++) {  //...with weights
            wt=1.0/SQR(sig[i]);
            ss += wt;
            sx += x[i]*wt;
            sy += y[i]*wt;
        }
    } else {
        for (i=1; i<=ndata; i++) {  //...or without weights.
            sx += x[i];
            sy += y[i];
        }
        ss=ndata;
    }
    sxoss=sx/ss;
    if (mwt) {
        for (i=1; i<=ndata; i++) {
            t=(x[i]-sxoss)/sig[i];
            st2 += t*t;
            *b += t*y[i]/sig[i];
        }
    } else {
        for (i=1; i<=ndata; i++) {
            t=x[i]-sxoss;
            st2 += t*t;
            *b += t*y[i];
        }
    }
    *b /= st2;  //Solve for a, b, sigma_a, and sigma_b.
    *a=(sy-sx*(*b))/ss;
    *siga=sqrt((1.0+sx*sx/(ss*st2))/ss);
    *sigb=sqrt(1.0/st2);

    *chi2=0.0;  //Calculate chi^2.
    *q=1.0;
    if (mwt == 0) {
        for (i=1; i<=ndata; i++)    *chi2 += SQR(y[i]-(*a)-(*b)*x[i]);
        //For unweighted data evaluate typical sig using chi2, and adjust
        // the standard deviations.
        sigdat=sqrt((*chi2)/(ndata-2));
        *siga *= sigdat; *sigb *= sigdat;
    } else {
        for (i=1; i<=ndata; i++)    *chi2 += SQR((y[i]-(*a)-(*b)*x[i])/sig[i]);
        if (ndata>2)
            *q=gammq(0.5*(ndata-2),0.5*(*chi2));  //Equation (15.2.12).
    }
}
#endif
//----------------------------------------------------------------------
//----------------------------------------------------------------------
/*
from nrc, p. 115:

Given arrays x[1..n] and y[1..n] containing a tabulated function, i.e.,
yi = f(xi), with x1 < x2 < ... < xN , and given values yp1 and ypn for 
the first derivative of the interpolating function at points 1 and n, 
respectively, this routine returns an array y2[1..n] that contains the 
second derivatives of the interpolating function at the tabulated points 
xi.  If yp1 and/or ypn are equal to 1 × 10^30 or larger, the routine is 
signaled to set the corresponding boundary condition for a natural spline, 
with zero second derivative on that boundary.
*/
void spline ( double* x, double* y, const int n, const double yp1,
              const double ypn, double* y2 )
{
    --x;  --y;  --y2;  //because nrc subscripts 1..n (fortran) instead of
                       // 0..n-1 (c++)

    int  i, k;
    double  p, qn, sig, un, *u;

    u = new double[n+1];    //u=vector(1,n-1);
    if (yp1 > 0.99e30) {
        //The lower boundary condition is set either to be "natural"
        y2[1]=u[1]=0.0;
    } else {
        //or else to have a specified first derivative.
        y2[1] = -0.5;
        u[1]=(3.0/(x[2]-x[1]))*((y[2]-y[1])/(x[2]-x[1])-yp1);
    }

    for (i=2; i<=n-1; i++) {
        //This is the decomposition loop of the tridiagonal algorithm.
        // y2 and u are used for temporary storage of the decomposed 
        // factors.
        sig=(x[i]-x[i-1])/(x[i+1]-x[i-1]);
        p=sig*y2[i-1]+2.0;
        y2[i]=(sig-1.0)/p;
        u[i]=(y[i+1]-y[i])/(x[i+1]-x[i]) - (y[i]-y[i-1])/(x[i]-x[i-1]);
        u[i]=(6.0*u[i]/(x[i+1]-x[i-1])-sig*u[i-1])/p;
    }
    if (ypn > 0.99e30) {
        //The upper boundary condition is set either to be "natural"
        qn=un=0.0;
    } else {
        //or else to have a specified first derivative.
        qn=0.5;
        un=(3.0/(x[n]-x[n-1]))*(ypn-(y[n]-y[n-1])/(x[n]-x[n-1]));
    }
    y2[n]=(un-qn*u[n-1])/(qn*y2[n-1]+1.0);
    for (k=n-1; k>=1; k--) {
        //This is the backsubstitution loop of the tridiagonal algorithm.
        y2[k]=y2[k]*y2[k+1]+u[k];
    }
    delete u;    //free_vector(u,1,n-1);
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
/*
from nrc, p. 116:

Given the arrays xa[1..n] and ya[1..n], which tabulate a function (with 
the xai's in order), and given the array y2a[1..n], which is the output 
from spline() above, and given a value of x, this routine returns a 
cubic-spline interpolated value y.
*/
void splint ( double* xa, double* ya, double* y2a, const int n, const double x, double* y )
{
    --xa;  --ya;  --y2a;  //because nrc subscripts 1..n (fortran) instead of
                          // 0..n-1 (c++)
    int klo, khi, k;
    double h, b, a;
    //We will find the right place in the table by means of bisection. 
    // This is optimal if sequential calls to this routine are at random 
    // values of x.  If sequential calls are in order, and closely spaced, 
    // one would do better to store previous values of klo and khi and test 
    // if they remain appropriate on the next call.
    klo=1;
    khi=n;
    while (khi-klo > 1) {
        k=(khi+klo) >> 1;
        if (xa[k] > x) khi=k;
        else           klo=k;
    }
    //klo and khi now bracket the input value of x.
    h=xa[khi]-xa[klo];
    //The xa s must be distinct.
    if (h == 0.0)    puts("Bad xa input to routine splint");
    a=(xa[khi]-x)/h;
    b=(x-xa[klo])/h;
    //Cubic spline polynomial is now evaluated.
    *y = a*ya[klo] + b*ya[khi]
       + ((a*a*a-a)*y2a[klo]+(b*b*b-b)*y2a[khi]) * (h*h) / 6.0;
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
template<class T>
inline void SWAP ( T& a, T& b ) {
    T  temp = a;
    a = b;
    b = temp;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//
//from p. 39 of Nuermical Recipes in C,
// Chapter 2. Solution of Linear Algebraic Equations
//
//Linear equation solution by Gauss-Jordan elimination, equation (2.1.1)
// above. a[1..n][1..n] is the input matrix. b[1..n][1..m] is input containing
//the m right-hand side vectors. On output, a is replaced by its matrix inverse,
//and b is replaced by the corresponding set of solution vectors.
//
template< class T >
static void gaussj ( T **a, int n, T **b, int m ) {
    int    *indxc, *indxr, *ipiv;
    int    i, j, k, l, ll;
    int    icol=0, irow=0;  //could be used uninitialized below
    T      big, dum, pivinv;
    //indxc = ivector(1,n);  //The integer arrays ipiv, indxr, and indxc are
    //                       // used for bookkeeping on the pivoting.
    //indxr = ivector(1,n);
    //ipiv  = ivector(1,n);
    indxc = new int[n+1];
    indxr = new int[n+1];
    ipiv  = new int[n+1];
    for (j=1; j<=n; j++)    ipiv[j] = 0;
    for (i=1; i<=n; i++) {  //This is the main loop over the columns to be reduced.
        big = (T)0.0;
        for (j=1; j<=n; j++) {  //This is the outer loop of the search for a pivot element.
            if (ipiv[j] != 1) {
                for (k=1; k<=n; k++) {
                    if (ipiv[k] == 0) {
                        if (fabs(a[j][k]) >= big) {
                            big  = (T)fabs(a[j][k]);
                            irow = j;
                            icol = k;
                        }
                    }
                }
            }
        }
        ++(ipiv[icol]);
        //We now have the pivot element, so we interchange rows, if needed, to
        //put the pivot element on the diagonal. The columns are not physically
        //interchanged, only relabeled: indxc[i], the column of the ith pivot
        //element, is the ith column that is reduced, while indxr[i] is the row
        //in which that pivot element was originally located. If indxr[i]<>indxc[i]
        //there is an implied column interchange. With this form of bookkeeping,
        //the solution bs will end up in the correct order, and the inverse
        //matrix will be scrambled by columns.
        if (irow != icol) {
            for (l=1; l<=n; l++)    SWAP( a[irow][l], a[icol][l] );
            for (l=1; l<=m; l++)    SWAP( b[irow][l], b[icol][l] );
        }
        indxr[i] = irow;  //We are now ready to divide the pivot row by the
                          //pivot element, located at irow and icol.
        indxc[i] = icol;
        if (a[icol][icol] == 0.0)    puts("gaussj: Singular Matrix");
        pivinv = (T)(1.0 / a[icol][icol]);
        a[icol][icol] = (T)1.0;
        for (l=1; l<=n; l++)    a[icol][l] *= pivinv;
        for (l=1; l<=m; l++)    b[icol][l] *= pivinv;


        for (ll=1; ll<=n; ll++) {  //Next, we reduce the rows...
            if (ll != icol) {      //...except for the pivot one, of course.
                dum = a[ll][icol];
                a[ll][icol] = (T)0.0;
                for (l=1; l<=n; l++)    a[ll][l] -= a[icol][l]*dum;
                for (l=1; l<=m; l++)    b[ll][l] -= b[icol][l]*dum;
            }
        }
    }
    //This is the end of the main loop over columns of the reduction. It only
    //remains to unscramble the solution in view of the column interchanges.
    //We do this by interchanging pairs of columns in the reverse order that
    //the permutation was built up.
    for (l=n; l>=1; l--) {
        if (indxr[l] != indxc[l]) {
            for (k=1; k<=n; k++) {
                SWAP(a[k][indxr[l]],a[k][indxc[l]]);
            }
        }
    }  //And we are done.

    //free_ivector(ipiv,1,n);
    //free_ivector(indxr,1,n);
    //free_ivector(indxc,1,n);
    delete ipiv;     ipiv =NULL;
    delete indxr;    indxr=NULL;
    delete indxc;    indxc=NULL;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//from p. 665, Numerical Recipes in C, Chapter 15. Modeling of Data
//Given a set of data points x[1..ndata],y[1..ndata] with individual standard
//deviations sig[1..ndata], fit them to a straight line y = a + bx by minimizing
//chi-square. Returned are a,b and their respective probable uncertainties siga
//and sigb, the chi-square chi2, and the goodness-of-fit probability q (that the
//fit would have chi-square this large or larger). If mwt=0 on input, then the
// standard deviations are assumed to be unavailable: q is returned as 1.0 and
//the normalization of chi2 is to unit standard deviation on all points.
void fit ( float x[], float y[], int ndata, float sig[], int mwt, float *a,
           float *b, float *siga, float *sigb, float *chi2, float *q )
{
    int  i;
    float  wt, t, sxoss, sx=0.0, sy=0.0, st2=0.0, ss, sigdat;
    *b=0.0;
    if (mwt) {  //Accumulate sums ...
        ss=0.0;
        for (i=1; i<=ndata; i++) {  //...with weights
            wt=(float)(1.0/SQR(sig[i]));
            ss += wt;
            sx += x[i]*wt;
            sy += y[i]*wt;
        }
    } else {
        for (i=1; i<=ndata; i++) {  //...or without weights.
            sx += x[i];
            sy += y[i];
        }
        ss=(float)ndata;
    }
    sxoss=sx/ss;
    if (mwt) {
        for (i=1; i<=ndata; i++) {
            t=(x[i]-sxoss)/sig[i];
            st2 += t*t;
            *b += t*y[i]/sig[i];
        }
    } else {
        for (i=1; i<=ndata; i++) {
            t=x[i]-sxoss;
            st2 += t*t;
            *b += t*y[i];
        }
    }
    *b /= st2;  //Solve for a, b, sigma_a, and sigma_b.
    *a=(sy-sx*(*b))/ss;
    *siga=(float)sqrt((1.0+sx*sx/(ss*st2))/ss);
    *sigb=(float)sqrt(1.0/st2);

    *chi2 = 0.0;  //Calculate chi-square.
    *q    = 1.0;
    if (mwt == 0) {
        for (i=1; i<=ndata; i++)
            *chi2 += SQR(y[i]-(*a)-(*b)*x[i]);
            sigdat = sqrt((*chi2)/(ndata-2));  //For unweighted data evaluate typical
                                               // sig using chi2, and adjust the
                                               // standard deviations.
            *siga *= sigdat;
            *sigb *= sigdat;
    } else {
        for (i=1; i<=ndata; i++)
            *chi2 += SQR((y[i]-(*a)-(*b)*x[i])/sig[i]);
        if (ndata>2) {
            *q = (float)gammq(0.5*(ndata-2),0.5*(*chi2));  //Equation (15.2.12).
        }
    }
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//from p. 675
//Expand in storage the covariance matrix covar, so as to take into account
//parameters that are being held .xed. (For the latter, return zero covariances.)
static void covsrt ( float **covar, int ma, int ia[], int mfit )
{
    int  i,j,k;
    for (i=mfit+1; i<=ma; i++) {
        for (j=1; j<=i; j++) {
            covar[i][j]=covar[j][i]=0.0;
        }
    }
    k=mfit;
    for (j=ma; j>=1; j--) {
        if (ia[j]) {
            for (i=1; i<=ma; i++)    SWAP(covar[i][k], covar[i][j]);
            for (i=1; i<=ma; i++)    SWAP(covar[k][i], covar[j][i]);
            k--;
        }
    }
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//from appendix b:
//allocate a float matrix with subscript range m[nrl..nrh][ncl..nch].
#define  NR_END  1

template< class T >
static void matrix ( T**& m, long nrl, long nrh, long ncl, long nch ) {
    long  i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
    //T**   m;
    //allocate pointers to rows
    m = (T**)malloc( (nrow+NR_END)*sizeof(T*) );
    if (m==NULL) {
        puts("allocation failure 1 in matrix()");
        return;
    }
    m += NR_END;
    m -= nrl;
    //allocate rows and set pointers to them
    m[nrl] = (T*)malloc( (nrow*ncol+NR_END)*sizeof(T) );
    if (m[nrl]==NULL) {
        puts("allocation failure 2 in matrix()");
        return;
    }
    m[nrl] += NR_END;
    m[nrl] -= ncl;
    for (i=nrl+1; i<=nrh; i++)    m[i] = m[i-1]+ncol;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//p. 946 - free a float marix allocated by matrix()

template< class T >
static void free_matrix ( T** m, long nrl, long nrh, long ncl, long nch )
{
    free( m[nrl]+ncl-NR_END );
    free( m+nrl-NR_END );
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
//15.4 General Linear Least Squares 674
//
//Given a set of data points x[1..ndat], y[1..ndat] with individual standard
//deviations sig[1..ndat], use chi-square minimization to fit for some or all
//of the coefficients a[1..ma] of a function that depends linearly on a,
//y = sigma_i a_i * afunc_i(x). The input array ia[1..ma] indicates by
//nonzero entries those components of a that should be fitted for, and by
//zero entries those components that should be held fixed at their input
//values. The program returns values for a[1..ma], chi-square = chisq, and
//the covariance matrix covar[1..ma][1..ma]. (Parameters held fixed will
//return zero covariances.)  The user supplies a routine funcs(x,afunc,ma)
//that returns the ma basis functions evaluated at x = x in the array
//afunc[1..ma].
//
void lfit ( const float x[], const float y[], const float sig[],
    const int ndat, float a[], int ia[], const int ma, float **covar,
    float *chisq, void (*funcs)(const float, float[], const int) )
{
    int    i, j, k, l, m, mfit=0;
    float  ym, wt, sum, sig2i, **beta, *afunc;
    matrix(beta, 1, ma, 1, 1);
    //afunc = vector(1,ma);
    afunc = new float[ma+1];    assert( afunc!=NULL );
    for (j=1; j<=ma; j++)
        if (ia[j])    mfit++;
    if (mfit == 0)    puts("lfit: no parameters to be fitted");
    for (j=1; j<=mfit; j++) {  //Initialize the (symmetric) matrix.
        for (k=1; k<=mfit; k++)    covar[j][k]=0.0;
        beta[j][1]=0.0;
    }
    for (i=1; i<=ndat; i++) {  //Loop over data to accumulate coefficients of
                               // the normal equations.
        (*funcs)(x[i],afunc,ma);
        ym = y[i];
        if (mfit < ma) {  //Subtract off dependences on known pieces
                          // of the fitting function.
            for (j=1; j<=ma; j++)
                if (!ia[j])    ym -= a[j]*afunc[j];
        }
        sig2i = (float)(1.0/SQR(sig[i]));
        for (j=0, l=1; l<=ma;l++) {
            if (ia[l]) {
                wt = afunc[l]*sig2i;
                for (j++,k=0,m=1;m<=l;m++)
                    if (ia[m])    covar[j][++k] += wt*afunc[m];
                beta[j][1] += ym*wt;
            }
        }
    }
    for (j=2; j<=mfit; j++)  //Fill in above the diagonal from symmetry.
        for (k=1; k<j; k++)
            covar[k][j] = covar[j][k];
    gaussj(covar, mfit, beta, 1);  //Matrix solution.
    for (j=0,l=1; l<=ma; l++)
        if (ia[l])    a[l]=beta[++j][1];  //Partition solution to appropriate coefficients a.
    *chisq=0.0;
    for (i=1; i<=ndat; i++) {  //Evaluate chi-square of the fit.
        (*funcs)(x[i],afunc,ma);
        for (sum=0.0,j=1; j<=ma; j++)    sum += a[j]*afunc[j];
        *chisq += SQR((y[i]-sum)/sig[i]);
    }
    covsrt(covar, ma, ia, mfit);  //Sort covariance matrix to true order of
                               // fitting coefficients.
    //free_vector(afunc,1,ma);
    delete afunc;    afunc=NULL;
    free_matrix(beta, 1, ma, 1, 1);
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static void fPolynomial ( const float x, float p[], const int np ) {
    //example of "funcs" for lfit().
    //fitting routine for a polynomial of degree np-1 with coefficients in
    //the array p[1..np].
    p[1] = 1.0;  // x^0 = 1
    for (int j=2; j<=np; j++) {
        p[j] = p[j-1] * x;  // x^n = x^n-1 * x
    }
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
void fLegendrePolynomial ( const float x, float pl[], const int nl ) {
    //example of "funcs" for lfit().
    //fitting routine for an expansion of nl Legendre polynomials pl,
    //evaluated using the recurrence relation as in section 5.5.
    pl[1] = 1.0;
    pl[2] = x;
    if (nl>2) {
        float  twox = (float)2.0 * x;
        float  f2 = x;
        float  d = 1.0;
        for (int j=3; j<=nl; j++) {
            const float  f1 = d++;
            f2 += twox;
            pl[j] = (f2*pl[j-1] - f1*pl[j-2]) / d;
        }
    }
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
void test_lfit ( void ) {
    #define NDAT 10
    float   x[NDAT+1], y[NDAT+1], sigma[NDAT+1];
    #define MA 4
    float   a[MA+1];
    int     ia[MA+1];

    for (int i=0; i<NDAT+1; i++) {
        x[i] = (float)i;
        y[i] = (float)-i;
        sigma[i] = 1;
    }
    for (int j=0; j<MA+1; j++) {  a[j]=0.0;  ia[j]=1;  }

    float**  covar;
    matrix(covar, 1, MA, 1, MA);
    float  chisq;

    lfit( x, y, sigma, NDAT, a, ia, MA, covar, &chisq, fPolynomial );
    free_matrix(covar, 1, MA, 1, MA);
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------

