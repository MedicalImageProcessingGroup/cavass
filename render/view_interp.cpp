/*
  Copyright 1993-2008, 2020 Medical Image Processing Group
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

#ifndef PI
#define PI 3.14159265358979323846
#endif

void AtoM(double M[3][3], double A1, double A2, double A3);
void matrix_multiply(double dest[3][3],double mleft[3][3],double mright[3][3]);
void transpose(double dest[3][3], double src[3][3]);
void MtoA(double *A1, double *A2, double *A3, double M[3][3]);

/*****************************************************************************
 * FUNCTION: view_interpolate
 * DESCRIPTION: Finds intermediate orientations between key poses.
 * PARAMETERS:
 *    between: The angles defining the intermediate orientation will go here:
 *       between[0], the angle in radians between z-axis and rotation axis;
 *       between[1], the angle in radians of rotation axis projection on x-y
 *          plane;
 *       between[2], the rotation angle in radians.
 *    pose1, pose2: The angles defining the key poses; same interpretation.
 *    partway: The intermediate orientation is partway from pose1 to pose2.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1/24/92 by Dewey Odhner
 *
 *****************************************************************************/
void view_interpolate(double between[3], double pose1[3], double pose2[3],
	double partway)
{
	double Mi[3][3], Mf[3][3], Mt[3][3], At[3];

	AtoM(Mi, pose1[0], pose1[1], pose1[2]);
	AtoM(Mf, pose2[0], pose2[1], pose2[2]);
	transpose(Mt, Mi);
	matrix_multiply(Mt, Mf, Mt);
	MtoA(At, At+1, At+2, Mt);
	while (At[2] < -PI)
		At[2] += 2*PI;
	while (At[2] > PI)
		At[2] -= 2*PI;
	if (fabs(At[2]) < .0001)
	{
		At[0] = .5*PI;
		At[1] = .5*PI;
		At[2] = 2*PI;
	}
	AtoM(Mt, At[0], At[1], At[2]*partway);
	matrix_multiply(Mi, Mt, Mi);
	MtoA(between, between+1, between+2, Mi);
}
