/*
  Copyright 1993-2014 Medical Image Processing Group
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


 
 
/* lagmax.f -- translated by f2c (version 20000817).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

#include "f2c.h"

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% lagmax.f %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* Subroutine */ int lagmax_(integer *n, doublereal *g, doublereal *h__,
    doublereal *rho, doublereal *d__, doublereal *v, doublereal *vmax)
{
    /* System generated locals */
    integer h_dim1, h_offset, i__1, i__2;
    doublereal d__1, d__2, d__3, d__4;

    /* Builtin functions */
    double sqrt();

    /* Local variables */
    static doublereal half, dlin, hmax, temp, vlin, wcos, zero, wsin, sumv;
    static integer i__, j, k;
    static doublereal scale, tempa, tempb, tempc, tempd, ratio, gnorm, tempv, 
	    vnorm, dd, gd, gg, vv, halfrt, dhd, ghg, one, vhg, dsq, vhv, sum, 
	    whw, vhw, vmu, vsq, wsq;


/*     N is the number of variables of a quadratic objective function, Q say. */
/*     G is the gradient of Q at the origin. */
/*     H is the symmetric Hessian matrix of Q. Only the upper triangular and */
/*       diagonal parts need be set. */
/*     RHO is the trust region radius, and has to be positive. */
/*     D will be set to the calculated vector of variables. */
/*     The array V will be used for working space. */
/*     VMAX will be set to |Q(0)-Q(D)|. */

/*     Calculating the D that maximizes |Q(0)-Q(D)| subject to ||D|| .LEQ. RHO */
/*     requires of order N**3 operations, but sometimes it is adequate if */
/*     |Q(0)-Q(D)| is within about 0.9 of its greatest possible value. This */
/*     subroutine provides such a solution in only of order N**2 operations, */
/*     where the claim of accuracy has been tested by numerical experiments. */

/*     Preliminary calculations. */

    /* Parameter adjustments */
    h_dim1 = *n;
    h_offset = 1 + h_dim1 * 1;
    h__ -= h_offset;
    --g;
    --d__;
    --v;

    /* Function Body */
    half = .5;
    halfrt = sqrt(half);
    one = 1.;
    zero = 0.;

/*     Pick V such that ||HV|| / ||V|| is large. */

    hmax = zero;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	sum = zero;
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    h__[j + i__ * h_dim1] = h__[i__ + j * h_dim1];
/* L10: */
/* Computing 2nd power */
	    d__1 = h__[i__ + j * h_dim1];
	    sum += d__1 * d__1;
	}
	if (sum > hmax) {
	    hmax = sum;
	    k = i__;
	}
/* L20: */
    }
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
/* L30: */
	v[j] = h__[k + j * h_dim1];
    }

/*     Set D to a vector in the subspace spanned by V and HV that maximizes */
/*     |(D,HD)|/(D,D), except that we set D=HV if V and HV are nearly parallel. */
/*     The vector that has the name D at label 60 used to be the vector W. */

    vsq = zero;
    vhv = zero;
    dsq = zero;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing 2nd power */
	d__1 = v[i__];
	vsq += d__1 * d__1;
	d__[i__] = zero;
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
/* L40: */
	    d__[i__] += h__[i__ + j * h_dim1] * v[j];
	}
	vhv += v[i__] * d__[i__];
/* L50: */
/* Computing 2nd power */
	d__1 = d__[i__];
	dsq += d__1 * d__1;
    }
    if (vhv * vhv <= dsq * .9999 * vsq) {
	temp = vhv / vsq;
	wsq = zero;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    d__[i__] -= temp * v[i__];
/* L60: */
/* Computing 2nd power */
	    d__1 = d__[i__];
	    wsq += d__1 * d__1;
	}
	whw = zero;
	ratio = sqrt(wsq / vsq);
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    temp = zero;
	    i__2 = *n;
	    for (j = 1; j <= i__2; ++j) {
/* L70: */
		temp += h__[i__ + j * h_dim1] * d__[j];
	    }
	    whw += temp * d__[i__];
/* L80: */
	    v[i__] = ratio * v[i__];
	}
	vhv = ratio * ratio * vhv;
	vhw = ratio * wsq;
	temp = half * (whw - vhv);
/* Computing 2nd power */
	d__2 = temp;
/* Computing 2nd power */
	d__3 = vhw;
	d__1 = sqrt(d__2 * d__2 + d__3 * d__3);
	d__4 = whw + vhv;
	temp += d_sign(&d__1, &d__4);
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L90: */
	    d__[i__] = vhw * v[i__] + temp * d__[i__];
	}
    }

/*     We now turn our attention to the subspace spanned by G and D. A multiple */
/*     of the current D is returned if that choice seems to be adequate. */

    gg = zero;
    gd = zero;
    dd = zero;
    dhd = zero;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing 2nd power */
	d__1 = g[i__];
	gg += d__1 * d__1;
	gd += g[i__] * d__[i__];
/* Computing 2nd power */
	d__1 = d__[i__];
	dd += d__1 * d__1;
	sum = zero;
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
/* L100: */
	    sum += h__[i__ + j * h_dim1] * d__[j];
	}
/* L110: */
	dhd += sum * d__[i__];
    }
    temp = gd / gg;
    vv = zero;
    d__1 = *rho / sqrt(dd);
    d__2 = gd * dhd;
    scale = d_sign(&d__1, &d__2);
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	v[i__] = d__[i__] - temp * g[i__];
/* Computing 2nd power */
	d__1 = v[i__];
	vv += d__1 * d__1;
/* L120: */
	d__[i__] = scale * d__[i__];
    }
    gnorm = sqrt(gg);
    if (gnorm * dd <= *rho * .005 * abs(dhd) || vv / dd <= 1e-4) {
	*vmax = (d__1 = scale * (gd + half * scale * dhd), abs(d__1));
	goto L170;
    }

/*     G and V are now orthogonal in the subspace spanned by G and D. Hence */
/*     we generate an orthonormal basis of this subspace such that (D,HV) is */
/*     negligible or zero, where D and V will be the basis vectors. */

    ghg = zero;
    vhg = zero;
    vhv = zero;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	sum = zero;
	sumv = zero;
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    sum += h__[i__ + j * h_dim1] * g[j];
/* L130: */
	    sumv += h__[i__ + j * h_dim1] * v[j];
	}
	ghg += sum * g[i__];
	vhg += sumv * g[i__];
/* L140: */
	vhv += sumv * v[i__];
    }
    vnorm = sqrt(vv);
    ghg /= gg;
    vhg /= vnorm * gnorm;
    vhv /= vv;
/* Computing MAX */
    d__1 = abs(ghg), d__2 = abs(vhv);
    if (abs(vhg) <= max(d__1,d__2) * .01) {
	vmu = ghg - vhv;
	wcos = one;
	wsin = zero;
    } else {
	temp = half * (ghg - vhv);
/* Computing 2nd power */
	d__2 = temp;
/* Computing 2nd power */
	d__3 = vhg;
	d__1 = sqrt(d__2 * d__2 + d__3 * d__3);
	vmu = temp + d_sign(&d__1, &temp);
/* Computing 2nd power */
	d__1 = vmu;
/* Computing 2nd power */
	d__2 = vhg;
	temp = sqrt(d__1 * d__1 + d__2 * d__2);
	wcos = vmu / temp;
	wsin = vhg / temp;
    }
    tempa = wcos / gnorm;
    tempb = wsin / vnorm;
    tempc = wcos / vnorm;
    tempd = wsin / gnorm;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	d__[i__] = tempa * g[i__] + tempb * v[i__];
/* L150: */
	v[i__] = tempc * v[i__] - tempd * g[i__];
    }

/*     The final D is a multiple of the current D, V, D+V or D-V. We make the */
/*     choice from these possibilities that is optimal. */

    dlin = wcos * gnorm / *rho;
    vlin = -wsin * gnorm / *rho;
    tempa = abs(dlin) + half * (d__1 = vmu + vhv, abs(d__1));
    tempb = abs(vlin) + half * (d__1 = ghg - vmu, abs(d__1));
    tempc = halfrt * (abs(dlin) + abs(vlin)) + (d__1 = ghg + vhv, abs(d__1)) *
	     .25;
    if (tempa >= tempb && tempa >= tempc) {
	d__1 = dlin * (vmu + vhv);
	tempd = d_sign(rho, &d__1);
	tempv = zero;
    } else if (tempb >= tempc) {
	tempd = zero;
	d__1 = vlin * (ghg - vmu);
	tempv = d_sign(rho, &d__1);
    } else {
	d__1 = halfrt * *rho;
	d__2 = dlin * (ghg + vhv);
	tempd = d_sign(&d__1, &d__2);
	d__1 = halfrt * *rho;
	d__2 = vlin * (ghg + vhv);
	tempv = d_sign(&d__1, &d__2);
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L160: */
	d__[i__] = tempd * d__[i__] + tempv * v[i__];
    }
/* Computing MAX */
    d__1 = max(tempa,tempb);
    *vmax = *rho * *rho * max(d__1,tempc);
L170:
    return 0;
} /* lagmax_ */

