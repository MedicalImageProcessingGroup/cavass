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


 
 
/* uobyqa.f -- translated by f2c (version 20000817).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

#include <stdio.h>
#include <stdlib.h>
#include "f2c.h"
/* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% uobyqa.f %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% */
/* Subroutine */ int UObyQA(n, x, rhobeg, rhoend, iprint, maxfun, objective)
integer n;
doublereal *x, rhobeg, rhoend;
integer iprint, maxfun;
double (*objective)();
{
    static integer id, ig, ih, iw;
    extern /* Subroutine */ int uobyqb_(integer *n, doublereal *x,
	    doublereal *rhobeg, doublereal *rhoend, integer *iprint,
		integer *maxfun, integer *npt, doublereal *xbase, doublereal *xopt,
		doublereal *xnew, doublereal *xpt, doublereal *pq, doublereal *pl,
		doublereal *h__, doublereal *g, doublereal *d__, doublereal *vlag,
		doublereal *w, double (*objective)());
    static integer ipl, ivl, npt;
	doublereal *w, *xbase, *xopt, *xnew, *xpt, *pq, *pl, *h__, *g, *d__, *vlag;


/*     This subroutine seeks the least value of a function of many variables, */
/*     by a trust region method that forms quadratic models by interpolation. */
/*     The algorithm is described in "UOBYQA: unconstrained optimization by */
/*     quadratic approximation" by M.J.D. Powell, Report DAMTP 2000/NA14, */
/*     University of Cambridge. The arguments of the subroutine are as follows. */

/*     N must be set to the number of variables and must be at least two. */
/*     Initial values of the variables must be set in X(1),X(2),...,X(N). They */
/*       will be changed to the values that give the least calculated F. */
/*     RHOBEG and RHOEND must be set to the initial and final values of a trust */
/*       region radius, so both must be positive with RHOEND<=RHOBEG. Typically */
/*       RHOBEG should be about one tenth of the greatest expected change to a */
/*       variable, and RHOEND should indicate the accuracy that is required in */
/*       the final values of the variables. */
/*     The value of IPRINT should be set to 0, 1, 2 or 3, which controls the */
/*       amount of printing. Specifically, there is no output if IPRINT=0 and */
/*       there is output only at the return if IPRINT=1. Otherwise, each new */
/*       value of RHO is printed, with the best vector of variables so far and */
/*       the corresponding value of the objective function. Further, each new */
/*       value of F with its variables are output if IPRINT=3. */
/*     MAXFUN must be set to an upper bound on the number of calls of CALFUN. */
/*     The array W will be used for working space. Its length must be at least */
/*       ( N**4 + 8*N**3 + 23*N**2 + 42*N + max [ 2*N**2 + 4, 18*N ] ) / 4. */

/*     SUBROUTINE CALFUN (N,X,F) must be provided by the user. It must set F to */
/*     the value of the objective function for the variables X(1),X(2),...,X(N). */

/*     Partition the working space array, so that different parts of it can be */
/*     treated separately by the subroutine that performs the main calculation. */

    /* Parameter adjustments */
    npt = (n * n + n * 3 + 2) / 2;
	xbase = (doublereal *)malloc(n*sizeof(*w));
	xopt = (doublereal *)malloc(n*sizeof(*w));
	xnew = (doublereal *)malloc(n*sizeof(*w));
	xpt = (doublereal *)malloc((n * npt)*sizeof(*w));
	pq = (doublereal *)malloc((npt - 1)*sizeof(*w));
	pl = (doublereal *)malloc((n*n*n*n+6*n*n*n+15*n*n+24*n+max(2*n*n+4, 18*n))/4*sizeof(*w));
	if (xbase==NULL || xopt==NULL || xnew==NULL || xpt==NULL || pq==NULL || pl==NULL) {
		fprintf(stderr, "Out of memory.\n");
		return 1;
	}
	--pl;
    --x;

    /* Function Body */
	ipl = 1;
    ih = ipl + (npt - 1) * npt;
    ig = ih + n * n;
    id = ig + n;
    ivl = ih;
    iw = id + n;
    uobyqb_(&n, &x[1], &rhobeg, &rhoend, &iprint, &maxfun, &npt, xbase, xopt, 
	    xnew, xpt, pq, &pl[ipl], &pl[ih], &pl[ig], &pl[id], &pl[
	    ivl], &pl[iw], objective);
    free(xbase);
	free(xopt);
	free(xnew);
	free(xpt);
	free(pq);
	free(++pl);
	return 0;
} /* UObyQA */

