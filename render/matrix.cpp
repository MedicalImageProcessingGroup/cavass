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


 
 

#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*****************************************************************************
 * FUNCTION: matrix_multiply
 * DESCRIPTION: Computes the matrix product of 3x3 double precision matrices.
 * PARAMETERS:
 *    dest: The product goes here.
 *    mleft: The matrix to multiply on the left
 *    mright: The matrix to multiply on the right
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/15/90 by Dewey Odhner
 *
 *****************************************************************************/
void matrix_multiply(double dest[3][3], double mleft[3][3],double mright[3][3])
{
	double result[3][3];
	int i, j;

	for (i=0; i<3; i++)
		for (j=0; j<3; j++)
			result[i][j] =
				mleft[i][0]*mright[0][j]+
				mleft[i][1]*mright[1][j]+
				mleft[i][2]*mright[2][j];
	memcpy(dest, result, sizeof(result));
}


/*****************************************************************************
 * FUNCTION: matrix_vector_multiply
 * DESCRIPTION: Computes the product of a 3x3 double precision matrix and a
 *    column 3-vector.
 * PARAMETERS:
 *    dest: The product goes here.
 *    matrix: The matrix to multiply on the left
 *    vector: The vector to multiply on the right
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/13/90 by Dewey Odhner
 *
 *****************************************************************************/
void matrix_vector_multiply(double dest[3], double matrix[3][3],
	double vector[3])
{
	double result[3];
	int i;

	for (i=0; i<3; i++)
		result[i] =
			matrix[i][0]*vector[0]+
			matrix[i][1]*vector[1]+
			matrix[i][2]*vector[2];
	memcpy(dest, result, sizeof(result));
}


/*****************************************************************************
 * FUNCTION: vector_matrix_multiply
 * DESCRIPTION: Computes the product of a row 3-vector and a 3x3 matrix.
 * PARAMETERS:
 *    dest: The product goes here.
 *    vector: The vector to multiply on the left
 *    matrix: The matrix to multiply on the right
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/13/90 by Dewey Odhner
 *
 *****************************************************************************/
void vector_matrix_multiply(double dest[3], double vector[3],
	double matrix[3][3])
{
	double result[3];
	int i;

	for (i=0; i<3; i++)
		result[i] =
			matrix[0][i]*vector[0]+
			matrix[1][i]*vector[1]+
			matrix[2][i]*vector[2];
	memcpy(dest, result, sizeof(result));
}


/*****************************************************************************
 * FUNCTION: vector_add
 * DESCRIPTION: Computes the sum of two double precision 3-vectors.
 * PARAMETERS:
 *    dest: The sum goes here.
 *    vector1, vector2: The vectors to add
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: dest may coincide with vector1 and/or vector2, but
 *    must not partially overlap.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 2/13/90 by Dewey Odhner
 *
 *****************************************************************************/
void vector_add(double dest[3], double vector1[3], double vector2[3])
{
	dest[0] = vector1[0]+vector2[0];
	dest[1] = vector1[1]+vector2[1];
	dest[2] = vector1[2]+vector2[2];
}

/*****************************************************************************
 * FUNCTION: vector_subtract
 * DESCRIPTION: Computes the difference of two double precision 3-vectors.
 * PARAMETERS:
 *    dest: The difference goes here.
 *    vector1, vector2: The vectors to subtract (vector1[] - vector2[])
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: dest may coincide with vector1 and/or vector2, but
 *    must not partially overlap.
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void vector_subtract(double dest[3], double vector1[3], double vector2[3])
{
	dest[0] = vector1[0]-vector2[0];
	dest[1] = vector1[1]-vector2[1];
	dest[2] = vector1[2]-vector2[2];
}


/*****************************************************************************
 * FUNCTION: axis_rot
 * DESCRIPTION: Computes the rotation matrix R such that if w is the coordinate
 *    vector of a feature on an object, Rw is the coordinate vector of the same
 *    feature after rotation of the object about the specified coordinate axis.
 * PARAMETERS:
 *    dest: The output goes here.
 *    axis: An integer 0, 1, or 2 to specify rotation about the x-, y-, or z-
 *       axis respectively.
 *    angle: The rotation angle in radians.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/15/90 by Dewey Odhner
 *
 *****************************************************************************/

extern double sin(double), cos(double);

void axis_rot(double dest[3][3], int axis, double angle)
{
	double sin_angle, cos_angle;
	int A0, A1, A2;

	assert(axis>=0 && axis<=2);
	sin_angle = sin(angle);
	cos_angle = cos(angle);
	A0 = axis;
	A1 = (axis+1)%3;
	A2 = (axis+2)%3;
	dest[A0][A0] = 1;
	dest[A0][A1] = 0;
	dest[A0][A2] = 0;
	dest[A1][A0] = 0;
	dest[A1][A1] = cos_angle;
	dest[A1][A2] = -sin_angle;
	dest[A2][A0] = 0;
	dest[A2][A1] = sin_angle;
	dest[A2][A2] = cos_angle;
}


/*****************************************************************************
 * FUNCTION: transpose
 * DESCRIPTION: Computes the transpose of a 3x3 matrix.
 * PARAMETERS:
 *    dest: The output goes here.
 *    src: The matrix to be transposed
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 8/16/91 by Dewey Odhner
 *
 *****************************************************************************/
void transpose(double dest[3][3], double src[3][3])
{
	double sjk;
	int j, k;

	for (j=0; j<3; j++)
	{	dest[j][j] = src[j][j];
		for (k=0; k<j; k++)
		{	sjk = src[j][k];
			dest[j][k] = src[k][j];
			dest[k][j] = sjk;
		}
	}
}

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
int VInvertMatrix(double Ainv[], double A[], int N)
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
