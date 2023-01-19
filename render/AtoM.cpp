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


 
 
 
#include <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif

void matrix_multiply(double dest[3][3],double mleft[3][3],double mright[3][3]);
void transpose(double dest[3][3], double src[3][3]);
void matrix_vector_multiply(double dest[3], double matrix[3][3],
	double vector[3]);
void vector_subtract(double dest[3], double vector1[3], double vector2[3]);

/*****************************************************************************
 * FUNCTION: AtoM
 * DESCRIPTION: Computes the rotation matrix from the given angles.
 * PARAMETERS:
 *    M, the array where the rotation matrix elements will be put.
 *    A1, the angle in radians between z-axis and rotation axis.
 *    A2, the angle in radians of rotation axis projection on x-y plane.
 *    A3, the rotation angle in radians.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: M must be a valid address to write the output.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not fulfilled
 * HISTORY:
 *    Created: 1/18/90 by Dewey Odhner
 *
 *****************************************************************************/
void AtoM(double M[3][3], double A1, double A2, double A3)
{
	double cosa1, sina1, cosa2, sina2, cosa3, sina3, R2R1[3][3];

	cosa1 = cos(A1);
	sina1 = sin(A1);
	cosa2 = cos(A2);
	sina2 = sin(A2);
	cosa3 = cos(A3);
	sina3 = sin(A3);

	/* compute R2xR1. M=R1 */
	M[0][0] = cosa1;
	M[0][1] = 0;
	M[0][2] = sina1;
	M[1][0] = 0;
	M[1][1] = 1;
	M[1][2] = 0;
	M[2][0] = -sina1;
	M[2][1] = 0;
	M[2][2] = cosa1;

	R2R1[0][0] = cosa2;
	R2R1[0][1] = -sina2;
	R2R1[0][2] = 0;
	R2R1[1][0] = sina2;
	R2R1[1][1] = cosa2;
	R2R1[1][2] = 0;
	R2R1[2][0] = 0;
	R2R1[2][1] = 0;
	R2R1[2][2] = 1;
	matrix_multiply(R2R1, R2R1, M);

	/* M=R3. */
	M[0][0] = cosa3;
	M[0][1] = -sina3;
	M[0][2] = 0;
	M[1][0] = sina3;
	M[1][1] = cosa3;
	M[1][2] = 0;
	M[2][0] = 0;
	M[2][1] = 0;
	M[2][2] = 1;

	matrix_multiply(M, R2R1, M);
	transpose(R2R1, R2R1);
	matrix_multiply(M, M, R2R1);
}


double Atan2(double y, double x)
{
	return (y==0&&x==0? 0: atan2(y, x));
}


/*****************************************************************************
 * FUNCTION: MtoA
 * DESCRIPTION: Computes the rotation angles from the given matrix.
 * PARAMETERS:
 *    A1, address to store the angle in radians between z-axis and rotation
 *       axis.
 *    A2, address to store the angle in radians of rotation axis projection on
 *       x-y plane.
 *    A3, address to store the rotation angle in radians.
 *    M, rotation matrix from which the angles will be computed.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The paramerers must be valid addresses. The matrix at M
 *    is assumed to be orthogonal with determinant +1.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 1/18/90 by Dewey Odhner
 *    Modified: 5/26/93 to handle more general case by Dewey Odhner
 *    Modified: 9/4/96 *A1, *A2 initialized in no rotation case by Dewey Odhner
 *    Modified: 9/28/06 calculation tweaked by Dewey Odhner
 *    Modified: 7/13/09 check consistency of *A1 by Dewey Odhner.
 *
 *****************************************************************************/
void MtoA(double *A1, double *A2, double *A3, double M[3][3])
{
	double cosa2, sina2, TrM, cos2a1, cosa1, sina1, R[3][3], RR[3][3],
		V[3], W[3], v_w1, a1b;

	TrM = M[0][0]+M[1][1]+M[2][2];
	if (3-TrM <= 0)
	{	*A1 = *A2 = *A3 = 0;
		return;
	}
  /*
	We want to set *A1, *A2, *A3 such that
	0 <= *A1 < PI,
	0 <= *A2 < 2*PI,
	-PI < *A3 <= PI,
	M[0][0]-M[1][1] == sin(*A1)*sin(*A1)*(1-2*cos(*A2)*cos(*A2))*(1-cos(*A3)),
	M[0][1]+M[1][0] == 2*sin(*A1)*sin(*A1)*cos(*A2)*sin(*A2)*(1-cos(*A3)),
	TrM == 2*cos(*A3)+1,
	M[0][2]+M[2][0] == 2*cos(*A1)*sin(*A1)*cos(*A2)*(1-cos(*A3)),
	M[1][2]+M[2][1] == 2*cos(*A1)*sin(*A1)*sin(*A2)*(1-cos(*A3)),
	M[2][0]-M[0][2] == 2*sin(*A1)*sin(*A2)*sin(*A3),
	M[1][2]-M[2][1] == 2*sin(*A1)*cos(*A2)*sin(*A3),
	M[0][1]-M[1][0] == cos(*A1)*(2*sin(*A2)*sin(*A2)+1)*sin(*A3)
  */
	*A2 = .5*(Atan2(M[0][1]+M[1][0], M[0][0]-M[1][1]));
	cosa2 = cos(*A2);
	sina2 = sin(*A2);
	cos2a1 = (4*M[2][2]-TrM-1)/(3-TrM);
	if (cos2a1 > 1)
		cos2a1 = 1;
	if (cos2a1 < -1)
		cos2a1 = -1;
	*A1 = .5*acos(cos2a1);
	sina1 = sin(*A1);
	cosa1 = cos(*A1);

	/* check consistency (The rotation axis should be invariant under M.) */
	V[0] = cosa2*sina1;
	V[1] = sina2*sina1;
	V[2] = cosa1;
	matrix_vector_multiply(W, M, V);
	vector_subtract(V, W, V);
	v_w1 = V[0]*V[0]+V[1]*V[1]+V[2]*V[2];
	a1b = M_PI-*A1;
	V[0] = cosa2*sin(a1b);
	V[1] = sina2*sin(a1b);
	V[2] = cos(a1b); 
	matrix_vector_multiply(W, M, V);
	vector_subtract(V, W, V);
	if (V[0]*V[0]+V[1]*V[1]+V[2]*V[2] < v_w1)
	{
		*A1 = a1b;
		sina1 = sin(*A1);
		cosa1 = cos(*A1);
	}

	/* R=R1 */
	R[0][0] = cosa1;
	R[0][1] = 0;
	R[0][2] = sina1;
	R[1][0] = 0;
	R[1][1] = 1;
	R[1][2] = 0;
	R[2][0] = -R[0][2];
	R[2][1] = 0;
	R[2][2] = R[0][0];

	/* RR=R2 */
	RR[0][0] = cosa2;
	RR[0][1] = -sina2;
	RR[0][2] = 0;
	RR[1][0] = sina2;
	RR[1][1] = cosa2;
	RR[1][2] = 0;
	RR[2][0] = 0;
	RR[2][1] = 0;
	RR[2][2] = 1;

	/* RR=R2xR1 */
	matrix_multiply(RR, RR, R);

	/* R=MxR2xR1 */
	matrix_multiply(R, M, RR);

	/* R=R3 */
	transpose(RR, RR);
	matrix_multiply(R, RR, R);

	*A3 = Atan2(R[1][0], R[0][0]);
}

/*****************************************************************************
 * FUNCTION: AtoV
 * DESCRIPTION: Computes the unit vector from the given angles.
 * PARAMETERS:
 *    V, the array where the vector elements will be put.
 *    A1, the angle in radians between z-axis and vector.
 *    A2, the angle in radians of vector projection on x-y plane.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: A must be a valid address to write the output.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not fulfilled
 * HISTORY:
 *    Created: 2/23/93 by Dewey Odhner
 *
 *****************************************************************************/
void AtoV(double V[3], double A1, double A2)
{
	double sina1;

	sina1 = sin(A1);
	V[0] = cos(A2)*sina1;
	V[1] = sin(A2)*sina1;
	V[2] = cos(A1);
}

/*****************************************************************************
 * FUNCTION: VtoA
 * DESCRIPTION: Computes the angles from the given vector.
 * PARAMETERS:
 *    A1, address to store the angle in radians between z-axis and vector.
 *    A2, address to store the angle in radians of vector projection on
 *       x-y plane.
 *    V, vector from which the angles will be computed.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The paramerers must be valid addresses.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 2/23/93 by Dewey Odhner
 *
 *****************************************************************************/
void VtoA(double *A1, double *A2, double V[3])
{
	*A2 = Atan2(V[1], V[0]);
	*A1 = Atan2(sqrt(V[1]*V[1]+V[0]*V[0]), V[2]);
}

/*****************************************************************************
 * FUNCTION: RtoM
 * DESCRIPTION: Computes the rotation matrix from the given rotation vector.
 * PARAMETERS:
 *    M, the array where the rotation matrix elements will be put.
 *    R, the rotation vector.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: M must be a valid address to write the output.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry condition is not fulfilled
 * HISTORY:
 *    Created: 3/13/12 by Dewey Odhner
 *
 *****************************************************************************/
void RtoM(double M[3][3], double R[3])
{
	double A1, A2, A3, N[3];
	int j, k;

	A3 = sqrt(R[0]*R[0]+R[1]*R[1]+R[2]*R[2]);
	if (A3 == 0)
	{
		for (j=0; j<3; j++)
			for (k=0; k<3; k++)
				M[j][k] = j==k;
		return;
	}
	for (j=0; j<3; j++)
		N[j] = 1/A3*R[j];
	VtoA(&A1, &A2, N);
	AtoM(M, A1, A2, A3);
}

/*****************************************************************************
 * FUNCTION: MtoR
 * DESCRIPTION: Computes the rotation vector from the given matrix.
 * PARAMETERS:
 *    R, the array where the vector elements will be put.
 *    M, rotation matrix from which the angles will be computed.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The paramerers must be valid addresses. The matrix at M
 *    is assumed to be orthogonal with determinant +1.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled
 * HISTORY:
 *    Created: 3/13/12 by Dewey Odhner
 *
 *****************************************************************************/
void MtoR(double R[3], double M[3][3])
{
	double A1, A2, A3, N[3];
	int j;

	MtoA(&A1, &A2, &A3, M);
	AtoV(N, A1, A2);
	for (j=0; j<3; j++)
		R[j] = N[j]*A3;
}
