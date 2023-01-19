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


 
#include  <stdio.h>
#include  <assert.h>
 
/* uobyqb.f -- translated by f2c (version 20000817).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

#include "f2c.h"

/* Table of constant values */



/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% uobyqb.f %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* Subroutine */ int uobyqb_(n, x, rhobeg, rhoend, iprint, maxfun, npt, xbase,
	 xopt, xnew, xpt, pq, pl, h__, g, d__, vlag, w, objective)
integer *n;
doublereal *x, *rhobeg, *rhoend;
integer *iprint, *maxfun, *npt;
doublereal *xbase, *xopt, *xnew, *xpt, *pq, *pl, *h__, *g, *d__, *vlag, *w;
double (*objective)();
{

    /* System generated locals */
    integer xpt_dim1, xpt_offset, pl_dim1, pl_offset, h_dim1, h_offset, i__1, 
	    i__2, i__3;
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt();

    /* Local variables */
    static doublereal diff, half;
    static integer knew;
    static doublereal temp, fopt, sumg, sumh;
    static integer kopt, nptm;
    static doublereal zero, vmax, f;
    static integer i__, j, k;
    static doublereal fbase, delta, fsave, tempa;
    static integer ksave;
    static doublereal ratio, dnorm, vquad;
    static integer ktemp;
    static doublereal estim, rhosq, wmult;
    static integer ih, nf, ip, iq, iw;
    extern /* Subroutine */ int lagmax_(integer *n, doublereal *g,
	    doublereal *h__, doublereal *rho, doublereal *d__, doublereal *v,
		doublereal *vmax);
    static doublereal ddknew, evalue, detrat;
    static integer nftest;
    static doublereal errtol, sixthm;
    extern /* Subroutine */ int trstep_(integer *n, doublereal *g,
	    doublereal *h__, doublereal *delta, doublereal *tol, doublereal *d__,
	    doublereal *gg, doublereal *td, doublereal *tn, doublereal *w,
		doublereal *piv, doublereal *z__, doublereal *evalue);
    static doublereal tworsq, one, rho;
    static integer nnp;
    static doublereal tol, sum, two;
    static integer jswitch;
    static doublereal distest;



/*     The arguments N, X, RHOBEG, RHOEND, IPRINT and MAXFUN are identical to */
/*       the corresponding arguments in SUBROUTINE UOBYQA. */
/*     NPT is set by UOBYQA to (N*N+3*N+2)/2 for the above dimension statement. */
/*     XBASE will contain a shift of origin that reduces the contributions from */
/*       rounding errors to values of the model and Lagrange functions. */
/*     XOPT will be set to the displacement from XBASE of the vector of */
/*       variables that provides the least calculated F so far. */
/*     XNEW will be set to the displacement from XBASE of the vector of */
/*       variables for the current calculation of F. */
/*     XPT will contain the interpolation point coordinates relative to XBASE. */
/*     PQ will contain the parameters of the quadratic model. */
/*     PL will contain the parameters of the Lagrange functions. */
/*     H will provide the second derivatives that TRSTEP and LAGMAX require. */
/*     G will provide the first derivatives that TRSTEP and LAGMAX require. */
/*     D is reserved for trial steps from XOPT, except that it will contain */
/*       diagonal second derivatives during the initialization procedure. */
/*     VLAG will contain the values of the Lagrange functions at a new point X. */
/*     The array W will be used for working space. Its length must be at least */
/*     max [ 6*N, ( N**2 + 3*N + 2 ) / 2 ]. */

/*     Set some constants. */

    /* Parameter adjustments */
    h_dim1 = *n;
    h_offset = 1 + h_dim1 * 1;
    h__ -= h_offset;
    --x;
    pl_dim1 = *npt;
    pl_offset = 1 + pl_dim1 * 1;
    pl -= pl_offset;
    xpt_dim1 = *npt;
    xpt_offset = 1 + xpt_dim1 * 1;
    xpt -= xpt_offset;
    --xbase;
    --xopt;
    --xnew;
    --pq;
    --g;
    --d__;
    --vlag;
    --w;

    /* Function Body */
    one = 1.;
    two = 2.;
    zero = 0.;
    half = .5;
    tol = .01;
    nnp = *n + *n + 1;
    nptm = *npt - 1;
    nftest = max(*maxfun,1);

/*     Initialization. NF is the number of function calculations so far. */

    rho = *rhobeg;
    rhosq = rho * rho;
    nf = 0;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	xbase[i__] = x[i__];
	i__2 = *npt;
	for (k = 1; k <= i__2; ++k) {
/* L10: */
	    xpt[k + i__ * xpt_dim1] = zero;
	}
    }
    i__2 = *npt;
    for (k = 1; k <= i__2; ++k) {
	i__1 = nptm;
	for (j = 1; j <= i__1; ++j) {
/* L20: */
	    pl[k + j * pl_dim1] = zero;
	}
    }

/*     The branch to label 120 obtains a new value of the objective function */
/*     and then there is a branch back to label 50, because the new function */
/*     value is needed to form the initial quadratic model. The least function */
/*     value so far and its index are noted below. */

L30:
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L40: */
	x[i__] = xbase[i__] + xpt[nf + 1 + i__ * xpt_dim1];
    }
    goto L120;
L50:
    if (nf == 1) {
	fopt = f;
	kopt = nf;
	fbase = f;
	j = 0;
	jswitch = -1;
	ih = *n;
    } else {
	if (f < fopt) {
	    fopt = f;
	    kopt = nf;
	}
    }

/*     Form the gradient and diagonal second derivatives of the initial */
/*     quadratic model and Lagrange functions. */

    if (nf <= nnp) {
	jswitch = -jswitch;
	if (jswitch > 0) {
	    if (j >= 1) {
		ih += j;
		if (w[j] < zero) {
		    d__[j] = (fsave + f - two * fbase) / rhosq;
		    pq[j] = (fsave - f) / (two * rho);
			assert(pq[j]>=0 || pq[j]<0);
		    pl[ih * pl_dim1 + 1] = -two / rhosq;
		    pl[nf - 1 + j * pl_dim1] = half / rho;
		    pl[nf - 1 + ih * pl_dim1] = one / rhosq;
		} else {
		    pq[j] = (fsave * 4. - fbase * 3. - f) / (two * rho);
			assert(pq[j]>=0 || pq[j]<0);
		    d__[j] = (fbase + f - two * fsave) / rhosq;
		    pl[j * pl_dim1 + 1] = -1.5 / rho;
		    pl[ih * pl_dim1 + 1] = one / rhosq;
		    pl[nf - 1 + j * pl_dim1] = two / rho;
		    pl[nf - 1 + ih * pl_dim1] = -two / rhosq;
		}
		pq[ih] = d__[j];
		assert(pq[ih]>=0 || pq[ih]<0);
		pl[nf + j * pl_dim1] = -half / rho;
		pl[nf + ih * pl_dim1] = one / rhosq;
	    }

/*     Pick the shift from XBASE to the next initial interpolation point */
/*     that provides diagonal second derivatives. */

	    if (j < *n) {
		++j;
		xpt[nf + 1 + j * xpt_dim1] = rho;
	    }
	} else {
	    fsave = f;
	    if (f < fbase) {
		w[j] = rho;
		xpt[nf + 1 + j * xpt_dim1] = two * rho;
	    } else {
		w[j] = -rho;
		xpt[nf + 1 + j * xpt_dim1] = -rho;
	    }
	}
	if (nf < nnp) {
	    goto L30;
	}

/*     Form the off-diagonal second derivatives of the initial quadratic model. */

	ih = *n;
	ip = 1;
	iq = 2;
    }
    ++ih;
    if (nf > nnp) {
	temp = one / (w[ip] * w[iq]);
	tempa = f - fbase - w[ip] * pq[ip] - w[iq] * pq[iq];
	pq[ih] = (tempa - half * rhosq * (d__[ip] + d__[iq])) * temp;
	assert(pq[ih]>=0 || pq[ih]<0);
	pl[ih * pl_dim1 + 1] = temp;
	iw = ip + ip;
	if (w[ip] < zero) {
	    ++iw;
	}
	pl[iw + ih * pl_dim1] = -temp;
	iw = iq + iq;
	if (w[iq] < zero) {
	    ++iw;
	}
	pl[iw + ih * pl_dim1] = -temp;
	pl[nf + ih * pl_dim1] = temp;

/*     Pick the shift from XBASE to the next initial interpolation point */
/*     that provides off-diagonal second derivatives. */

	++ip;
    }
    if (ip == iq) {
	++ih;
	ip = 1;
	++iq;
    }
    if (nf < *npt) {
	xpt[nf + 1 + ip * xpt_dim1] = w[ip];
	xpt[nf + 1 + iq * xpt_dim1] = w[iq];
	goto L30;
    }

/*     Set parameters to begin the iterations for the current RHO. */

    sixthm = zero;
    delta = rho;
L60:
/* Computing 2nd power */
    d__1 = two * rho;
    tworsq = d__1 * d__1;
    rhosq = rho * rho;

/*     Form the gradient of the quadratic model at the trust region centre. */

L70:
    knew = 0;
    ih = *n;
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	xopt[j] = xpt[kopt + j * xpt_dim1];
	g[j] = pq[j];
	i__2 = j;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    ++ih;
	    g[i__] += pq[ih] * xopt[j];
	    if (i__ < j) {
		g[j] += pq[ih] * xopt[i__];
	    }
	    assert(g[i__]>=0 || g[i__]<0);
	    assert(g[j]>=0 || g[j]<0);
/* L80: */
	    h__[i__ + j * h_dim1] = pq[ih];
	}
    }

/*     Generate the next trust region step and test its length. Set KNEW */
/*     to -1 if the purpose of the next F will be to improve conditioning, */
/*     and also calculate a lower bound on the Hessian term of the model Q. */

    trstep_(n, &g[1], &h__[h_offset], &delta, &tol, &d__[1], &w[1], &w[*n + 1]
	    , &w[(*n << 1) + 1], &w[*n * 3 + 1], &w[(*n << 2) + 1], &w[*n * 5 
	    + 1], &evalue);
    temp = zero;
    i__2 = *n;
    for (i__ = 1; i__ <= i__2; ++i__) {
/* L90: */
/* Computing 2nd power */
	d__1 = d__[i__];
	temp += d__1 * d__1;
    }
/* Computing MIN */
    d__1 = delta, d__2 = sqrt(temp);
    dnorm = min(d__1,d__2);
    errtol = -one;
    if (dnorm < half * rho) {
	knew = -1;
	errtol = half * evalue * rho * rho;
	if (nf <= *npt + 9) {
	    errtol = zero;
	}
	goto L290;
    }

/*     Calculate the next value of the objective function. */

L100:
    i__2 = *n;
    for (i__ = 1; i__ <= i__2; ++i__) {
	xnew[i__] = xopt[i__] + d__[i__];
/* L110: */
	x[i__] = xbase[i__] + xnew[i__];
    }
L120:
    if (nf >= nftest) {
	if (*iprint > 0) {
	    printf("Return from UOBYQA because CALFUN has been called MAXFUN times\n");
	}
	goto L420;
    }
    ++nf;
	f = objective(x+1);
    if (*iprint == 3) {
		printf("Function number%6d    F = %.10f\n    The corresponding X is: (",
			(int)nf, f);
		for (i__ = 1; i__ <= *n; ++i__) {
	 	   printf(" %f", x[i__]);
		}
		printf(")\n");
    }
    if (nf <= *npt) {
	goto L50;
    }
    if (knew == -1) {
	goto L420;
    }

/*     Use the quadratic model to predict the change in F due to the step D, */
/*     and find the values of the Lagrange functions at the new point. */

    vquad = zero;
    ih = *n;
    i__2 = *n;
    for (j = 1; j <= i__2; ++j) {
	w[j] = d__[j];
	vquad += w[j] * pq[j];
	i__1 = j;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    ++ih;
	    w[ih] = d__[i__] * xnew[j] + d__[j] * xopt[i__];
	    if (i__ == j) {
		w[ih] = half * w[ih];
	    }
/* L150: */
	    vquad += w[ih] * pq[ih];
	}
    }
    i__1 = *npt;
    for (k = 1; k <= i__1; ++k) {
	temp = zero;
	i__2 = nptm;
	for (j = 1; j <= i__2; ++j) {
/* L160: */
	    temp += w[j] * pl[k + j * pl_dim1];
	}
/* L170: */
	vlag[k] = temp;
    }
    vlag[kopt] += one;

/*     Update SIXTHM, which is a lower bound on one sixth of the greatest */
/*     third derivative of F. */

    diff = f - fopt - vquad;
    sum = zero;
    i__1 = *npt;
    for (k = 1; k <= i__1; ++k) {
	temp = zero;
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
/* L180: */
/* Computing 2nd power */
	    d__1 = xpt[k + i__ * xpt_dim1] - xnew[i__];
	    temp += d__1 * d__1;
	}
	temp = sqrt(temp);
/* L190: */
	sum += (d__1 = temp * temp * temp * vlag[k], abs(d__1));
    }
/* Computing MAX */
    d__1 = sixthm, d__2 = abs(diff) / sum;
    sixthm = max(d__1,d__2);

/*     Update FOPT and XOPT if the new F is the least value of the objective */
/*     function so far. Then branch if D is not a trust region step. */

    fsave = fopt;
    if (f < fopt) {
	fopt = f;
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/* L200: */
	    xopt[i__] = xnew[i__];
	}
    }
    ksave = knew;
    if (knew > 0) {
	goto L240;
    }

/*     Pick the next value of DELTA after a trust region step. */

    if (vquad >= zero) {
		if (*iprint > 0) {
		    printf("Return from UOBYQA because a trust region step has failed to reduce Q\n");
		}
		goto L420;
    }
    ratio = (f - fsave) / vquad;
    if (ratio <= .1) {
	delta = half * dnorm;
    } else if (ratio <= .7) {
/* Computing MAX */
	d__1 = half * delta;
	delta = max(d__1,dnorm);
    } else {
/* Computing MAX */
	d__1 = delta, d__2 = dnorm * 1.25, d__1 = max(d__1,d__2), d__2 = 
		dnorm + rho;
	delta = max(d__1,d__2);
    }
    if (delta <= rho * 1.5) {
	delta = rho;
    }

/*     Set KNEW to the index of the next interpolation point to be deleted. */

    ktemp = 0;
    detrat = zero;
    if (f >= fsave) {
	ktemp = kopt;
	detrat = one;
    }
    i__1 = *npt;
    for (k = 1; k <= i__1; ++k) {
	sum = zero;
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
/* L220: */
/* Computing 2nd power */
	    d__1 = xpt[k + i__ * xpt_dim1] - xopt[i__];
	    sum += d__1 * d__1;
	}
	temp = (d__1 = vlag[k], abs(d__1));
	if (sum > rhosq) {
	    d__1 = sum / rhosq;
	    temp *= d__1*sqrt(d__1);
	}
	if (temp > detrat && k != ktemp) {
	    detrat = temp;
	    ddknew = sum;
	    knew = k;
	}
/* L230: */
    }
    if (knew == 0) {
	goto L290;
    }

/*     Replace the interpolation point that has index KNEW by the point XNEW, */
/*     and also update the Lagrange functions and the quadratic model. */

L240:
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* L250: */
	xpt[knew + i__ * xpt_dim1] = xnew[i__];
    }
    temp = one / vlag[knew];
    i__1 = nptm;
    for (j = 1; j <= i__1; ++j) {
	pl[knew + j * pl_dim1] = temp * pl[knew + j * pl_dim1];
/* L260: */
	pq[j] += diff * pl[knew + j * pl_dim1];
	assert(pq[j]>=0 || pq[j]<0);
    }
    i__1 = *npt;
    for (k = 1; k <= i__1; ++k) {
	if (k != knew) {
	    temp = vlag[k];
	    i__2 = nptm;
	    for (j = 1; j <= i__2; ++j) {
/* L270: */
		pl[k + j * pl_dim1] -= temp * pl[knew + j * pl_dim1];
	    }
	}
/* L280: */
    }

/*     Update KOPT if F is the least calculated value of the objective */
/*     function. Then branch for another trust region calculation. The */
/*     case KSAVE>0 indicates that a model step has just been taken. */

    if (f < fsave) {
	kopt = knew;
	goto L70;
    }
    if (ksave > 0) {
	goto L70;
    }
    if (dnorm > two * rho) {
	goto L70;
    }
    if (ddknew > tworsq) {
	goto L70;
    }

/*     Alternatively, find out if the interpolation points are close */
/*     enough to the best point so far. */

L290:
    i__1 = *npt;
    for (k = 1; k <= i__1; ++k) {
	w[k] = zero;
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
/* L300: */
/* Computing 2nd power */
	    d__1 = xpt[k + i__ * xpt_dim1] - xopt[i__];
	    w[k] += d__1 * d__1;
	}
    }
L310:
    knew = -1;
    distest = tworsq;
    i__2 = *npt;
    for (k = 1; k <= i__2; ++k) {
	if (w[k] > distest) {
	    knew = k;
	    distest = w[k];
	}
/* L320: */
    }

/*     If a point is sufficiently far away, then set the gradient and Hessian */
/*     of its Lagrange function at the centre of the trust region, and find */
/*     half the sum of squares of components of the Hessian. */

    if (knew > 0) {
	ih = *n;
	sumh = zero;
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    g[j] = pl[knew + j * pl_dim1];
	    i__1 = j;
	    for (i__ = 1; i__ <= i__1; ++i__) {
		++ih;
		temp = pl[knew + ih * pl_dim1];
		g[j] += temp * xopt[i__];
		if (i__ < j) {
		    g[i__] += temp * xopt[j];
		    sumh += temp * temp;
		}
/* L330: */
		h__[i__ + j * h_dim1] = temp;
	    }
/* L340: */
	    sumh += half * temp * temp;
	}

/*     If ERRTOL is positive, test whether to replace the interpolation point */
/*     with index KNEW, using a bound on the maximum modulus of its Lagrange */
/*     function in the trust region. */

	if (errtol > zero) {
	    w[knew] = zero;
	    sumg = zero;
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
/* L350: */
/* Computing 2nd power */
		d__1 = g[i__];
		sumg += d__1 * d__1;
	    }
	    estim = rho * (sqrt(sumg) + rho * sqrt(half * sumh));
	    wmult = sixthm * distest * sqrt(distest);
	    if (wmult * estim <= errtol) {
		goto L310;
	    }
	}

/*     If the KNEW-th point may be replaced, then pick a D that gives a large */
/*     value of the modulus of its Lagrange function within the trust region. */
/*     Here the vector XNEW is used as temporary working space. */

	lagmax_(n, &g[1], &h__[h_offset], &rho, &d__[1], &xnew[1], &vmax);
	if (errtol > zero) {
	    if (wmult * vmax <= errtol) {
		goto L310;
	    }
	}
	goto L100;
    }
    if (dnorm > rho) {
	goto L70;
    }

/*     Prepare to reduce RHO by shifting XBASE to the best point so far, */
/*     and make the corresponding changes to the gradients of the Lagrange */
/*     functions and the quadratic model. */

    if (rho > *rhoend) {
	ih = *n;
	i__2 = *n;
	for (j = 1; j <= i__2; ++j) {
	    xbase[j] += xopt[j];
	    i__1 = *npt;
	    for (k = 1; k <= i__1; ++k) {
/* L360: */
		xpt[k + j * xpt_dim1] -= xopt[j];
	    }
	    i__1 = j;
	    for (i__ = 1; i__ <= i__1; ++i__) {
		++ih;
		pq[i__] += pq[ih] * xopt[j];
		assert(pq[i__]>=0 || pq[i__]<0);
		if (i__ < j) {
		    pq[j] += pq[ih] * xopt[i__];
			assert(pq[j]>=0 || pq[j]<0);
		    i__3 = *npt;
		    for (k = 1; k <= i__3; ++k) {
/* L370: */
			pl[k + j * pl_dim1] += pl[k + ih * pl_dim1] * xopt[
				i__];
		    }
		}
		i__3 = *npt;
		for (k = 1; k <= i__3; ++k) {
/* L380: */
		    pl[k + i__ * pl_dim1] += pl[k + ih * pl_dim1] * xopt[j];
		}
	    }
	}

/*     Pick the next values of RHO and DELTA. */

	delta = half * rho;
	ratio = rho / *rhoend;
	if (ratio <= 16.) {
	    rho = *rhoend;
	} else if (ratio <= 250.) {
	    rho = sqrt(ratio) * *rhoend;
	} else {
	    rho *= .1;
	}
	delta = max(delta,rho);
	if (*iprint >= 2) {
	    if (*iprint >= 3) {
			printf("\n     ");
	    }
		printf("New RHO = %.4f     Number of function values =%6d\n", rho, (int)nf);
		printf("Least value of F = %.15f        The corresponding X is: (", fopt);
	    for (i__ = 1; i__ <= *n; ++i__) {
			printf(" %f", xbase[i__]);
	    }
	    printf(")\n");
	}
	goto L60;
    }

/*     Return from the calculation, after another Newton-Raphson step, if */
/*     it is too short to have been tried before. */

    if (errtol >= zero) {
	goto L100;
    }
L420:
    if (fopt <= f) {
	i__3 = *n;
	for (i__ = 1; i__ <= i__3; ++i__) {
/* L430: */
	    x[i__] = xbase[i__] + xopt[i__];
	}
	f = fopt;
    }
    if (*iprint >= 1) {
		printf("At the return from UOBYQA     Number of function values =%6d\n", (int)nf);
		printf("Least value of F = %.15f        The corresponding X is: (", f);
	    for (i__ = 1; i__ <= *n; ++i__) {
			printf(" %f", xbase[i__]);
	    }
	    printf(")\n");
    }
    return 0;
} /* uobyqb_ */

