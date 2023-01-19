/* newuoa.f -- translated by f2c (version 20000121).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

#include <stdio.h>
#include <stdlib.h>
#include "f2c.h"

/* Subroutine */ int NewUOA(integer n, doublereal *x, 
	double rhobeg, double rhoend, integer iprint, integer
	maxfun, double (*objective)())
{
    /* Format strings */
    static char fmt_10[] = "(/4x,\002Return from NEWUOA because NPT is not\
 in\002,\002 the required interval\002)";

    /* Local variables */
    static integer ndim, nptm, ibmat, izmat, id, np, iw;
    extern /* Subroutine */ int newuob_(integer *, integer *, doublereal *, 
	    doublereal *, doublereal *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, doublereal *,
		double (*objective)())
	    ;
    static integer igq, ihq, ixb, ifv, ipq, ivl, ixn, ixo, ixp;
	integer npt=2*n+1;
	doublereal *w;



/*     This subroutine seeks the least value of a function of many variables, */
/*     by a trust region method that forms quadratic models by interpolation. */
/*     There can be some freedom in the interpolation conditions, which is */
/*     taken up by minimizing the Frobenius norm of the change to the second */
/*     derivative of the quadratic model, beginning with a zero matrix. The */
/*     arguments of the subroutine are as follows. */

/*     N must be set to the number of variables and must be at least two. */
/*     NPT is the number of interpolation conditions. Its value must be in the */
/*       interval [N+2,(N+1)(N+2)/2]. */
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
/*     (NPT+13)*(NPT+N)+3*N*(N+3)/2. */

/*     SUBROUTINE CALFUN (N,X,F) must be provided by the user. It must set F to */
/*     the value of the objective function for the variables X(1),X(2),...,X(N). */

/*     Partition the working space array, so that different parts of it can be */
/*     treated separately by the subroutine that performs the main calculation. */

	w = (doublereal *)malloc(((npt+13)*(npt+n)+3*n*(n+3)/2)*sizeof(*w));
	if (w == NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		return 1;
	}

    /* Parameter adjustments */
    --w;
    --x;

    /* Function Body */
    np = n + 1;
    nptm = npt - np;
    if (npt < n + 2 || npt > (n + 2) * np / 2) {
	goto L20;
    }
    ndim = npt + n;
    ixb = 1;
    ixo = ixb + n;
    ixn = ixo + n;
    ixp = ixn + n;
    ifv = ixp + n * npt;
    igq = ifv + npt;
    ihq = igq + n;
    ipq = ihq + n * np / 2;
    ibmat = ipq + npt;
    izmat = ibmat + ndim * n;
    id = izmat + npt * nptm;
    ivl = id + n;
    iw = ivl + ndim;

/*     The above settings provide a partition of W for subroutine NEWUOB. */
/*     The partition requires the first NPT*(NPT+N)+5*N*(N+3)/2 elements of */
/*     W plus the space that is needed by the last array of NEWUOB. */

    newuob_(&n, &npt, &x[1], &rhobeg, &rhoend, &iprint, &maxfun, &w[ixb], &w[ixo], &
	    w[ixn], &w[ixp], &w[ifv], &w[igq], &w[ihq], &w[ipq], &w[ibmat], &
	    w[izmat], &ndim, &w[id], &w[ivl], &w[iw], objective);
L20:
    free(++w);
	return 0;
} /* NewUOA */

