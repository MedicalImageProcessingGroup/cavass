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


 
 
/* trstep.f -- translated by f2c (version 20000817).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

#include "f2c.h"

/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% trstep.f %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* Subroutine */ int trstep_(integer *n, doublereal *g, doublereal *h__,
    doublereal *delta, doublereal *tol, doublereal *d__, doublereal *gg,
	doublereal *td, doublereal *tn, doublereal *w, doublereal *piv,
	doublereal *z__, doublereal *evalue)
{
    /* System generated locals */
    integer h_dim1, h_offset, i__1, i__2, i__3;
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double sqrt();

    /* Local variables */
    static doublereal phil, parl;
    static integer ksav;
    static doublereal temp, phiu, paru, zero, wwsq;
    static integer i__, j, k;
    static doublereal scale;
    static integer iterc;
    static doublereal tempa, delsq, tempb;
    static integer ksave;
    static doublereal tdmin, shift, dnorm, gnorm, hnorm, slope, pivot;
    static integer jp, nm, kp;
    static doublereal posdef, wz, shfmin, shfmax, pivksv, dhd, gam, dtg, phi, 
	    one, par, dsq;
    static integer kpp;
    static doublereal gsq, dtz, sum, two, wsq, zsq, parlest, paruest;


/*     N is the number of variables of a quadratic objective function, Q say. */
/*     G is the gradient of Q at the origin. */
/*     H is the Hessian matrix of Q. Only the upper triangular and diagonal */
/*       parts need be set. The lower triangular part is used to store the */
/*       elements of a Householder similarity transformation. */
/*     DELTA is the trust region radius, and has to be positive. */
/*     TOL is the value of a tolerance from the open interval (0,1). */
/*     D will be set to the calculated vector of variables. */
/*     The arrays GG, TD, TN, W, PIV and Z will be used for working space. */
/*     EVALUE will be set to the least eigenvalue of H if and only if D is a */
/*     Newton-Raphson step. Then EVALUE will be positive, but otherwise it */
/*     will be set to zero. */

/*     Let MAXRED be the maximum of Q(0)-Q(D) subject to ||D|| .LEQ. DELTA, */
/*     and let ACTRED be the value of Q(0)-Q(D) that is actually calculated. */
/*     We take the view that any D is acceptable if it has the properties */

/*             ||D|| .LEQ. DELTA  and  ACTRED .LEQ. (1-TOL)*MAXRED. */

/*     The calculation of D is done by the method of Section 2 of the paper */
/*     by MJDP in the 1997 Dundee Numerical Analysis Conference Proceedings, */
/*     after transforming H to tridiagonal form. */

/*     Initialization. */

    /* Parameter adjustments */
    h_dim1 = *n;
    h_offset = 1 + h_dim1 * 1;
    h__ -= h_offset;
    --g;
    --d__;
    --gg;
    --td;
    --tn;
    --w;
    --piv;
    --z__;

    /* Function Body */
    one = 1.;
    two = 2.;
    zero = 0.;
    delsq = *delta * *delta;
    *evalue = zero;
    nm = *n - 1;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	d__[i__] = zero;
	td[i__] = h__[i__ + i__ * h_dim1];
	i__2 = i__;
	for (j = 1; j <= i__2; ++j) {
/* L10: */
	    h__[i__ + j * h_dim1] = h__[j + i__ * h_dim1];
	}
    }

/*     Apply Householder transformations to obtain a tridiagonal matrix that */
/*     is similar to H, and put the elements of the Householder vectors in */
/*     the lower triangular part of H. Further, TD and TN will contain the */
/*     diagonal and other nonzero elements of the tridiagonal matrix. */

    i__2 = nm;
    for (k = 1; k <= i__2; ++k) {
	kp = k + 1;
	sum = zero;
	if (kp < *n) {
	    kpp = kp + 1;
	    i__1 = *n;
	    for (i__ = kpp; i__ <= i__1; ++i__) {
/* L20: */
/* Computing 2nd power */
		d__1 = h__[i__ + k * h_dim1];
		sum += d__1 * d__1;
	    }
	}
	if (sum == zero) {
	    tn[k] = h__[kp + k * h_dim1];
	    h__[kp + k * h_dim1] = zero;
	} else {
	    temp = h__[kp + k * h_dim1];
	    d__1 = sqrt(sum + temp * temp);
	    tn[k] = d_sign(&d__1, &temp);
	    h__[kp + k * h_dim1] = -sum / (temp + tn[k]);
/* Computing 2nd power */
	    d__1 = h__[kp + k * h_dim1];
	    temp = sqrt(two / (sum + d__1 * d__1));
	    i__1 = *n;
	    for (i__ = kp; i__ <= i__1; ++i__) {
		w[i__] = temp * h__[i__ + k * h_dim1];
		h__[i__ + k * h_dim1] = w[i__];
/* L30: */
		z__[i__] = td[i__] * w[i__];
	    }
	    wz = zero;
	    i__1 = nm;
	    for (j = kp; j <= i__1; ++j) {
		jp = j + 1;
		i__3 = *n;
		for (i__ = jp; i__ <= i__3; ++i__) {
		    z__[i__] += h__[i__ + j * h_dim1] * w[j];
/* L40: */
		    z__[j] += h__[i__ + j * h_dim1] * w[i__];
		}
/* L50: */
		wz += w[j] * z__[j];
	    }
	    wz += w[*n] * z__[*n];
	    i__1 = *n;
	    for (j = kp; j <= i__1; ++j) {
		td[j] += w[j] * (wz * w[j] - two * z__[j]);
		if (j < *n) {
		    jp = j + 1;
		    i__3 = *n;
		    for (i__ = jp; i__ <= i__3; ++i__) {
/* L60: */
			h__[i__ + j * h_dim1] = h__[i__ + j * h_dim1] - w[i__]
				 * z__[j] - w[j] * (z__[i__] - wz * w[i__]);
		    }
		}
/* L70: */
	    }
	}
/* L80: */
    }

/*     Form GG by applying the similarity transformation to G. */

    gsq = zero;
    i__2 = *n;
    for (i__ = 1; i__ <= i__2; ++i__) {
	gg[i__] = g[i__];
/* L90: */
/* Computing 2nd power */
	d__1 = g[i__];
	gsq += d__1 * d__1;
    }
    gnorm = sqrt(gsq);
    i__2 = nm;
    for (k = 1; k <= i__2; ++k) {
	kp = k + 1;
	sum = zero;
	i__1 = *n;
	for (i__ = kp; i__ <= i__1; ++i__) {
/* L100: */
	    sum += gg[i__] * h__[i__ + k * h_dim1];
	}
	i__1 = *n;
	for (i__ = kp; i__ <= i__1; ++i__) {
/* L110: */
	    gg[i__] -= sum * h__[i__ + k * h_dim1];
	}
    }

/*     Begin the trust region calculation with a tridiagonal matrix by */
/*     calculating the norm of H. Then treat the case when H is zero. */

    hnorm = abs(td[1]) + abs(tn[1]);
    tdmin = td[1];
    tn[*n] = zero;
    i__1 = *n;
    for (i__ = 2; i__ <= i__1; ++i__) {
	temp = (d__1 = tn[i__ - 1], abs(d__1)) + (d__2 = td[i__], abs(d__2)) 
		+ (d__3 = tn[i__], abs(d__3));
	hnorm = max(hnorm,temp);
/* L120: */
/* Computing MIN */
	d__1 = tdmin, d__2 = td[i__];
	tdmin = min(d__1,d__2);
    }
    if (hnorm == zero) {
	if (gnorm == zero) {
	    goto L400;
	}
	scale = *delta / gnorm;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L130: */
	    d__[i__] = -scale * gg[i__];
	}
	goto L370;
    }

/*     Set the initial values of PAR and its bounds. */

/* Computing MAX */
    d__1 = zero, d__2 = -tdmin, d__1 = max(d__1,d__2), d__2 = gnorm / *delta 
	    - hnorm;
    parl = max(d__1,d__2);
    parlest = parl;
    par = parl;
    paru = zero;
    paruest = zero;
    posdef = zero;
    iterc = 0;

/*     Calculate the pivots of the Cholesky factorization of (H+PAR*I). */

L140:
    ++iterc;
    ksav = 0;
    piv[1] = td[1] + par;
    k = 1;
L150:
    if (piv[k] > zero) {
/* Computing 2nd power */
	d__1 = tn[k];
	piv[k + 1] = td[k + 1] + par - d__1 * d__1 / piv[k];
    } else {
	if (piv[k] < zero || tn[k] != zero) {
	    goto L160;
	}
	ksav = k;
	piv[k + 1] = td[k + 1] + par;
    }
    ++k;
    if (k < *n) {
	goto L150;
    }
    if (piv[k] < zero) {
	goto L160;
    }
    if (piv[k] == zero) {
	ksav = k;
    }

/*     Branch if all the pivots are positive, allowing for the case when */
/*     G is zero. */

    if (ksav == 0 && gsq > zero) {
	goto L230;
    }
    if (gsq == zero) {
	if (par == zero) {
	    goto L370;
	}
	paru = par;
	paruest = par;
	if (ksav == 0) {
	    goto L190;
	}
    }
    k = ksav;

/*     Set D to a direction of nonpositive curvature of the given tridiagonal */
/*     matrix, and thus revise PARLEST. */

L160:
    d__[k] = one;
    if ((d__1 = tn[k], abs(d__1)) <= (d__2 = piv[k], abs(d__2))) {
	dsq = one;
	dhd = piv[k];
    } else {
	temp = td[k + 1] + par;
	if (temp <= (d__1 = piv[k], abs(d__1))) {
	    d__1 = -tn[k];
	    d__[k + 1] = d_sign(&one, &d__1);
	    dhd = piv[k] + temp - two * (d__1 = tn[k], abs(d__1));
	} else {
	    d__[k + 1] = -tn[k] / temp;
	    dhd = piv[k] + tn[k] * d__[k + 1];
	}
/* Computing 2nd power */
	d__1 = d__[k + 1];
	dsq = one + d__1 * d__1;
    }
L170:
    if (k > 1) {
	--k;
	if (tn[k] != zero) {
	    d__[k] = -tn[k] * d__[k + 1] / piv[k];
/* Computing 2nd power */
	    d__1 = d__[k];
	    dsq += d__1 * d__1;
	    goto L170;
	}
	i__1 = k;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L180: */
	    d__[i__] = zero;
	}
    }
    parl = par;
    parlest = par - dhd / dsq;

/*     Terminate with D set to a multiple of the current D if the following */
/*     test suggests that it suitable to do so. */

L190:
    temp = paruest;
    if (gsq == zero) {
	temp *= one - *tol;
    }
    if (paruest > zero && parlest >= temp) {
	dtg = zero;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L200: */
	    dtg += d__[i__] * gg[i__];
	}
	d__1 = *delta / sqrt(dsq);
	scale = -d_sign(&d__1, &dtg);
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L210: */
	    d__[i__] = scale * d__[i__];
	}
	goto L370;
    }

/*     Pick the value of PAR for the next iteration. */

L220:
    if (paru == zero) {
	par = two * parlest + gnorm / *delta;
    } else {
	par = (parl + paru) * .5;
	par = max(par,parlest);
    }
    if (paruest > zero) {
	par = min(par,paruest);
    }
    goto L140;

/*     Calculate D for the current PAR in the positive definite case. */

L230:
    w[1] = -gg[1] / piv[1];
    i__1 = *n;
    for (i__ = 2; i__ <= i__1; ++i__) {
/* L240: */
	w[i__] = (-gg[i__] - tn[i__ - 1] * w[i__ - 1]) / piv[i__];
    }
    d__[*n] = w[*n];
    for (i__ = nm; i__ >= 1; --i__) {
/* L250: */
	d__[i__] = w[i__] - tn[i__] * d__[i__ + 1] / piv[i__];
    }

/*     Branch if a Newton-Raphson step is acceptable. */

    dsq = zero;
    wsq = zero;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing 2nd power */
	d__1 = d__[i__];
	dsq += d__1 * d__1;
/* L260: */
/* Computing 2nd power */
	d__1 = w[i__];
	wsq += piv[i__] * (d__1 * d__1);
    }
    if (par == zero && dsq <= delsq) {
	goto L320;
    }

/*     Make the usual test for acceptability of a full trust region step. */

    dnorm = sqrt(dsq);
    phi = one / dnorm - one / *delta;
    temp = *tol * (one + par * dsq / wsq) - dsq * phi * phi;
    if (temp >= zero) {
	scale = *delta / dnorm;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L270: */
	    d__[i__] = scale * d__[i__];
	}
	goto L370;
    }
    if (iterc >= 2 && par <= parl) {
	goto L370;
    }
    if (paru > zero && par >= paru) {
	goto L370;
    }

/*     Complete the iteration when PHI is negative. */

    if (phi < zero) {
	parlest = par;
	if (posdef == one) {
	    if (phi <= phil) {
		goto L370;
	    }
	    slope = (phi - phil) / (par - parl);
	    parlest = par - phi / slope;
	}
	slope = one / gnorm;
	if (paru > zero) {
	    slope = (phiu - phi) / (paru - par);
	}
	temp = par - phi / slope;
	if (paruest > zero) {
	    temp = min(temp,paruest);
	}
	paruest = temp;
	posdef = one;
	parl = par;
	phil = phi;
	goto L220;
    }

/*     If required, calculate Z for the alternative test for convergence. */

    if (posdef == zero) {
	w[1] = one / piv[1];
	i__1 = *n;
	for (i__ = 2; i__ <= i__1; ++i__) {
	    temp = -tn[i__ - 1] * w[i__ - 1];
/* L280: */
	    w[i__] = (d_sign(&one, &temp) + temp) / piv[i__];
	}
	z__[*n] = w[*n];
	for (i__ = nm; i__ >= 1; --i__) {
/* L290: */
	    z__[i__] = w[i__] - tn[i__] * z__[i__ + 1] / piv[i__];
	}
	wwsq = zero;
	zsq = zero;
	dtz = zero;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing 2nd power */
	    d__1 = w[i__];
	    wwsq += piv[i__] * (d__1 * d__1);
/* Computing 2nd power */
	    d__1 = z__[i__];
	    zsq += d__1 * d__1;
/* L300: */
	    dtz += d__[i__] * z__[i__];
	}

/*     Apply the alternative test for convergence. */

	tempa = (d__1 = delsq - dsq, abs(d__1));
	tempb = sqrt(dtz * dtz + tempa * zsq);
	gam = tempa / (d_sign(&tempb, &dtz) + dtz);
	temp = *tol * (wsq + par * delsq) - gam * gam * wwsq;
	if (temp >= zero) {
	    i__1 = *n;
	    for (i__ = 1; i__ <= i__1; ++i__) {
/* L310: */
		d__[i__] += gam * z__[i__];
	    }
	    goto L370;
	}
/* Computing MAX */
	d__1 = parlest, d__2 = par - wwsq / zsq;
	parlest = max(d__1,d__2);
    }

/*     Complete the iteration when PHI is positive. */

    slope = one / gnorm;
    if (paru > zero) {
	if (phi >= phiu) {
	    goto L370;
	}
	slope = (phiu - phi) / (paru - par);
    }
/* Computing MAX */
    d__1 = parlest, d__2 = par - phi / slope;
    parlest = max(d__1,d__2);
    paruest = par;
    if (posdef == one) {
	slope = (phi - phil) / (par - parl);
	paruest = par - phi / slope;
    }
    paru = par;
    phiu = phi;
    goto L220;

/*     Set EVALUE to the least eigenvalue of the second derivative matrix if */
/*     D is a Newton-Raphson step. SHFMAX will be an upper bound on EVALUE. */

L320:
    shfmin = zero;
    pivot = td[1];
    shfmax = pivot;
    i__1 = *n;
    for (k = 2; k <= i__1; ++k) {
/* Computing 2nd power */
	d__1 = tn[k - 1];
	pivot = td[k] - d__1 * d__1 / pivot;
/* L330: */
	shfmax = min(shfmax,pivot);
    }

/*     Find EVALUE by a bisection method, but occasionally SHFMAX may be */
/*     adjusted by the rule of false position. */

    ksave = 0;
L340:
    shift = (shfmin + shfmax) * .5;
    k = 1;
    temp = td[1] - shift;
L350:
    if (temp > zero) {
	piv[k] = temp;
	if (k < *n) {
/* Computing 2nd power */
	    d__1 = tn[k];
	    temp = td[k + 1] - shift - d__1 * d__1 / temp;
	    ++k;
	    goto L350;
	}
	shfmin = shift;
    } else {
	if (k < ksave) {
	    goto L360;
	}
	if (k == ksave) {
	    if (pivksv == zero) {
		goto L360;
	    }
	    if (piv[k] - temp < temp - pivksv) {
		pivksv = temp;
		shfmax = shift;
	    } else {
		pivksv = zero;
		shfmax = (shift * piv[k] - shfmin * temp) / (piv[k] - temp);
	    }
	} else {
	    ksave = k;
	    pivksv = temp;
	    shfmax = shift;
	}
    }
    if (shfmin <= shfmax * .99) {
	goto L340;
    }
L360:
    *evalue = shfmin;

/*     Apply the inverse Householder transformations to D. */

L370:
    nm = *n - 1;
    for (k = nm; k >= 1; --k) {
	kp = k + 1;
	sum = zero;
	i__1 = *n;
	for (i__ = kp; i__ <= i__1; ++i__) {
/* L380: */
	    sum += d__[i__] * h__[i__ + k * h_dim1];
	}
	i__1 = *n;
	for (i__ = kp; i__ <= i__1; ++i__) {
/* L390: */
	    d__[i__] -= sum * h__[i__ + k * h_dim1];
	}
    }

/*     Return from the subroutine. */

L400:
    return 0;
} /* trstep_ */

