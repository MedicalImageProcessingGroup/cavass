/*
  Copyright 1993-2011 Medical Image Processing Group
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


#include <stdlib.h>
#include <string.h>



/*****************************************************************************
 * FUNCTION: VInvertMatrix
 * DESCRIPTION: Computes the inverse of a matrix. (Gauss-Jordan method)
 * PARAMETERS:
 *    Ainv: The result
 *    A: The matrix to be inverted
 *    N: The dimension of the matrix
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: Successful completion
 *    1: Memory allocation failure
 *    402: The matrix is singular.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 6/18/96 by Dewey Odhner
 *
 *****************************************************************************/
int VInvertMatrix(Ainv, A, N)
	double Ainv[], A[];
	int N;
{
	int ii, j, p;
	double *L, m;

	L = (double *)malloc(N*N*sizeof(double));
	if (L == 0)
		return (1);
	memcpy(L, A, N*N*sizeof(double));
	for (ii=0; ii<N; ii++)
		for (j=0; j<N; j++)
			Ainv[ii*N+j] = ii==j;
	for (ii=0; ii<N; ii++)
	{
		for (p=ii; p<N; p++)
			if (L[p*N+ii])
				break;
		if (p == N)
		{
			free(L);
			return (402);
		}
		if (p != ii)
			for (j=0; j<N; j++)
			{
				m = L[ii*N+j];
				L[ii*N+j] = L[p*N+j];
				L[p*N+j] = m;
				m = Ainv[ii*N+j];
				Ainv[ii*N+j] = Ainv[p*N+j];
				Ainv[p*N+j] = m;
			}
		m = 1/L[ii*N+ii];
		L[ii*N+ii] = 1;
		for (j=ii+1; j<N; j++)
			L[ii*N+j] *= m;
		for (j=0; j<N; j++)
			Ainv[ii*N+j] *= m;
		for (j=0; j<N; j++)
			if (j != ii)
			{
				m = L[j*N+ii];
				L[j*N+ii] = 0;
				for (p=ii+1; p<N; p++)
					L[j*N+p] -= m*L[ii*N+p];
				for (p=0; p<N; p++)
					Ainv[j*N+p] -= m*Ainv[ii*N+p];
			}
	}
	free(L);
	return (0);
}

/*****************************************************************************
 * FUNCTION: VComputeDeterminant
 * DESCRIPTION: Computes the determinant of a matrix.
 * PARAMETERS:
 *    det: The result
 *    A: The matrix to find the determinant of
 *    N: The dimension of the matrix
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: Successful completion
 *    1: Memory allocation failure
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 7/2/96 by Dewey Odhner
 *
 *****************************************************************************/
int VComputeDeterminant(det, A, N)
	double *det, A[];
	int N;
{
	int ii, j, k;
	double *sub, sum, subdet;

	if (N == 1)
	{
		*det = A[0];
		return (0);
	}
	if (N == 2)
	{
		*det = A[0]*A[3]-A[1]*A[2];
		return (0);
	}
	sub = (double *)malloc((N-1)*(N-1)*sizeof(double));
	if (sub == 0)
		return (1);
	for (j=0; j<N-1; j++)
		for (k=0; k<N-1; k++)
			sub[j*(N-1)+k] = A[(1+j)*N+1+k];
	sum = 0;
	for (ii=0; ; ii++)
	{
		if (VComputeDeterminant(&subdet, sub, N-1))
		{
			free(sub);
			return (1);
		}
		sum += A[ii]*(ii%2? -subdet: subdet);
		if (ii == N-1)
			break;
		for (j=0; j<N-1; j++)
			sub[j*(N-1)+ii] = A[(1+j)*N+ii];
	}
	*det = sum;
	free(sub);
	return (0);
}

/************************************************************************
 *
 *      FUNCTION        : MultMat
 *
 *      DESCRIPTION     : This multiplies two 4x4 matrices.
 *                        
 *
 *      RETURN VALUE    : None.
 *
 *      PARAMETERS      : out - the result matrix.
 *                        mat1,mat2 - matrices to be multiplied.
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : mat1 and mat2 should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
int MultMat(out,mat1,mat2)
     double out[4][4], mat1[4][4], mat2[4][4];
{
  
  int i,j;
  
  for(i=0;i<4;i++)
    for(j=0;j<4;j++)
      out[i][j]= mat1[i][0]*mat2[0][j] + mat1[i][1]*mat2[1][j]+
	mat1[i][2]*mat2[2][j]+mat1[i][3]*mat2[3][j];
  return 0;
}
