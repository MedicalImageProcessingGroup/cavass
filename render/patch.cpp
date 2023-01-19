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
#include <stdlib.h>
#include "patch.h"

#ifndef rint
#define rint(x) floor(0.5+(x))
#endif

/*****************************************************************************
 * FUNCTION: get_patch
 * DESCRIPTION: Initializes a patch describing the projection of a voxel.
 * PARAMETERS:
 *    patch: A pointer to a structure which describes a shape to be painted
 *       on an object's image buffer as a list of scan-line segments is put
 *       here.  The number of segments is (*patch)->bottom - (*patch)->top and
 *       the left and right ends of the n'th segment are
 *       (*patch)->lines[n-1].left and (*patch)->lines[n-1].right.
 *    projection_matrix: The matrix which transforms a vector in object space
 *       in units of voxels to image space in units of pixels.
 * SIDE EFFECTS: The memory at *patch will be freed.
 * ENTRY CONDITIONS: *patch must be NULL or a valid patch pointer.
 * RETURN VALUE:
 *    0: success
 *    1: memory allocation failure
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 2/19/90 by Dewey Odhner
 *    Modified: 2/28/94 return error code by Dewey Odhner
 *    Modified: 12/19/08 old pointer not kept by Dewey Odhner.
 *
 *****************************************************************************/
int get_patch(Patch **patch, double projection_matrix[3][3])
{
	int top_cornern, second_left, second_right, j, k, y,
			last_right_end, new_right_end=0, left_cornern, right_cornern,
			new_left_end=0, last_left_end;
	double voxel_corner[8][2];
	double *left_corners[4], *right_corners[4];

	voxel_corner[0][0] = (projection_matrix[0][0]+projection_matrix[0][1]+
			projection_matrix[0][2])/2;
	voxel_corner[1][0] = (projection_matrix[0][0]-projection_matrix[0][1]+
			projection_matrix[0][2])/2;
	voxel_corner[2][0] = (projection_matrix[0][0]+projection_matrix[0][1]-
			projection_matrix[0][2])/2;
	voxel_corner[3][0] = (projection_matrix[0][0]-projection_matrix[0][1]-
			projection_matrix[0][2])/2;
	voxel_corner[4][0] = -voxel_corner[3][0];
	voxel_corner[5][0] = -voxel_corner[2][0];
	voxel_corner[6][0] = -voxel_corner[1][0];
	voxel_corner[7][0] = -voxel_corner[0][0];
	voxel_corner[0][1] = (projection_matrix[1][0]+projection_matrix[1][1]+
			projection_matrix[1][2])/2;
	voxel_corner[1][1] = (projection_matrix[1][0]-projection_matrix[1][1]+
			projection_matrix[1][2])/2;
	voxel_corner[2][1] = (projection_matrix[1][0]+projection_matrix[1][1]-
			projection_matrix[1][2])/2;
	voxel_corner[3][1] = (projection_matrix[1][0]-projection_matrix[1][1]-
			projection_matrix[1][2])/2;
	voxel_corner[4][1] = -voxel_corner[3][1];
	voxel_corner[5][1] = -voxel_corner[2][1];
	voxel_corner[6][1] = -voxel_corner[1][1];
	voxel_corner[7][1] = -voxel_corner[0][1];

	/* Change coordinates */
	for (j=0; j<=7; j++)
		for (k=0; k<=1; k++)
			voxel_corner[j][k] += .25;

	for (j=1, top_cornern=0; j<8; j++)
		if (voxel_corner[j][1]<voxel_corner[top_cornern][1])
			top_cornern=j;
	second_left = second_right = top_cornern^1;
	for (j=2; j<=4; j<<=1)
	{	if
		(	(voxel_corner[top_cornern^j][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[second_left][1]-voxel_corner[top_cornern][1]) <
			(voxel_corner[second_left][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[top_cornern^j][1]-voxel_corner[top_cornern][1])
		)
			second_left = top_cornern^j;
		if
		(	(voxel_corner[top_cornern^j][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[second_right][1]-voxel_corner[top_cornern][1]) >
			(voxel_corner[second_right][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[top_cornern^j][1]-voxel_corner[top_cornern][1])
		)
			second_right = top_cornern^j;
	}
	left_corners[0] = right_corners[0] = voxel_corner[top_cornern];
	left_corners[1] = voxel_corner[second_left];
	right_corners[1] = voxel_corner[second_right];
	left_corners[2] = voxel_corner[7-second_right];
	right_corners[2] = voxel_corner[7-second_left];
	left_corners[3] = right_corners[3] = voxel_corner[7-top_cornern];
	int array_size = (int)(left_corners[3][1]-left_corners[0][1])+10;
	if (*patch)
		free(*patch);
	*patch = (Patch *)malloc(sizeof(Patch)+
		(array_size-1)*sizeof(struct Patch_line));
	if (*patch == NULL)
		return (1);
	left_cornern = right_cornern = 0;
	last_left_end = (int)floor(left_corners[0][0]);
	last_right_end = (int)ceil(right_corners[0][0]);
	y = (*patch)->top = (int)floor(left_corners[0][1]);
	while (left_cornern < 3)
	{	while (left_cornern<3 && left_corners[left_cornern+1][1]<y+1)
		{	left_cornern++;
			new_left_end = (int)floor(left_corners[left_cornern][0]);
			if (new_left_end < last_left_end)
				last_left_end = new_left_end;
		}
		while (right_cornern<3 && right_corners[right_cornern+1][1]<y+1)
		{	right_cornern++;
			new_right_end = (int)ceil(right_corners[right_cornern][0]);
			if (new_right_end > last_right_end)
				last_right_end = new_right_end;
		}
		j = left_cornern<3? left_cornern: 2;
		if (left_corners[j+1][1] > left_corners[j][1]+.00001)
		{	new_left_end =
				(int)floor(left_corners[j][0]+
					(y+1-left_corners[j][1])*
					(left_corners[j+1][0]-
						left_corners[j][0])/
					(left_corners[j+1][1]-
						left_corners[j][1]));
			if (new_left_end < last_left_end)
				last_left_end = new_left_end;
		}
		k = right_cornern<3? right_cornern: 2;
		if (right_corners[k+1][1] > right_corners[k][1]+.00001)
		{	new_right_end =
				(int)ceil(right_corners[k][0]+
					(y+1-right_corners[k][1])*
					(right_corners[k+1][0]-
						right_corners[k][0])/
					(right_corners[k+1][1]-
						right_corners[k][1]));
			if (new_right_end > last_right_end)
				last_right_end = new_right_end;
		}
		(*patch)->lines[y-(*patch)->top].left = last_left_end;
		(*patch)->lines[y-(*patch)->top].right = last_right_end;
		last_left_end = new_left_end;
		last_right_end = new_right_end;
		y++;
	}
	(*patch)->bottom = y;
	return (0);
}

/*****************************************************************************
 * FUNCTION: get_triangle_patch
 * DESCRIPTION: Initializes one in list of 64827 patches describing the
 *    projections of triangles.
 * PARAMETERS:
 *    patch: A pointer one of a list of structures which describe shapes to be
 *       painted on an object's image buffer as a list of scan-line segments is
 *       put here.  The number of segments is patch[i]->bottom - patch[i]->top
 *       and the left and right ends of the n'th segment are
 *       patch[i]->lines[n-1].left and patch[i]->lines[n-1].right.
 *    projection_matrix: The matrix which transforms a vector in object space
 *       in units of voxels to image space in units of pixels.
 *    edge_vertices: A table defining the numbered edges of a voxel in terms of
 *       numbered vertices (1 to 8).
 *    triangle_edges: A list of the numbered edges (1 to 12) of a voxel on
 *       which the vertices of each triangle lie.
 *    T_x, T_y, T_z: A table defining the numbered vertices of a voxel in terms
 *       of direction from the center along the coordinate axes (-1 or 1).
 *    v: Which of the 64827 triangle patches to compute.
 *    new_view: Flag indicating view may be different from the previous call.
 *    tweak: Adjust for only 3 voxel edge points per edge.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS:
 * RETURN VALUE:
 *    0: success
 *    1: memory allocation failure
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 9/10/03 by Dewey Odhner
 *
 *****************************************************************************/
int get_triangle_patch(Patch **patch, double projection_matrix[3][3],
	const int edge_vertices[13][2], const int triangle_edges[189][3],
	const int T_x[9], const int T_y[9], const int T_z[9], int v, int new_view,
	int tweak)
{
	int top_cornern, bottom_cornern, j, k, y, tn,
			last_right_end, new_right_end, left_cornern, right_cornern,
			new_left_end, last_left_end, m, env[3];
	static double voxel_edge_point[13][7][2];
	double second_left[2], second_right[1];

	if (new_view)
	{
		for (j=1; j<=12; j++)
		  for (k=0; k<2; k++)
		  {
			for (m=0; m<7; m++)
				voxel_edge_point[j][m][k] = (
					projection_matrix[k][0]*((13-2*m)*T_x[edge_vertices[j][0]]+
						(2*m+1)*T_x[edge_vertices[j][1]])+
					projection_matrix[k][1]*((13-2*m)*T_y[edge_vertices[j][0]]+
						(2*m+1)*T_y[edge_vertices[j][1]])+
					projection_matrix[k][2]*((13-2*m)*T_z[edge_vertices[j][0]]+
						(2*m+1)*T_z[edge_vertices[j][1]]))/28;
		    if (tweak)
			{
				voxel_edge_point[j][1][k] =
					.5*(voxel_edge_point[j][1][k]+voxel_edge_point[j][0][k]);
				voxel_edge_point[j][5][k] =
					.5*(voxel_edge_point[j][5][k]+voxel_edge_point[j][6][k]);
			}
		  }

		/* Change coordinates */
		for (j=1; j<=12; j++)
		  for (k=0; k<=1; k++)
			for (m=0; m<7; m++)
				voxel_edge_point[j][m][k] += .25;
	}

#define VertexPoint(j) voxel_edge_point[triangle_edges[tn][j]][env[j]]

	tn = v%189;
	env[2] = (v-tn)/189%7;
	env[1] = ((v-tn)/189-env[2])/7%7;
	env[0] = (((v-tn)/189-env[2])/7-env[1])/7;

	top_cornern = bottom_cornern = left_cornern = right_cornern = 0;
	for (j=1; j<3; j++)
	{
		if (VertexPoint(j)[1] < VertexPoint(top_cornern)[1])
			top_cornern = j;
		if (VertexPoint(j)[1] > VertexPoint(bottom_cornern)[1])
			bottom_cornern = j;
	}
	if (VertexPoint(bottom_cornern)[1]-VertexPoint(top_cornern)[1] > 0)
	{
		for (j=1; j<3; j++)
		{
			if (VertexPoint(j)[0]-(VertexPoint(j)[1]-
				VertexPoint(top_cornern)[1])*
				(VertexPoint(bottom_cornern)[0]-
				VertexPoint(top_cornern)[0])/
				(VertexPoint(bottom_cornern)[1]-
				VertexPoint(top_cornern)[1]) <
				VertexPoint(left_cornern)[0]-
				(VertexPoint(left_cornern)[1]-
				VertexPoint(top_cornern)[1])*
				(VertexPoint(bottom_cornern)[0]-
				VertexPoint(top_cornern)[0])/
				(VertexPoint(bottom_cornern)[1]-
				VertexPoint(top_cornern)[1]))
			  left_cornern = j;
			if (VertexPoint(j)[0]-(VertexPoint(j)[1]-
				VertexPoint(top_cornern)[1])*
				(VertexPoint(bottom_cornern)[0]-
				VertexPoint(top_cornern)[0])/
				(VertexPoint(bottom_cornern)[1]-
				VertexPoint(top_cornern)[1]) >
				VertexPoint(right_cornern)[0]-
				(VertexPoint(right_cornern)[1]-
				VertexPoint(top_cornern)[1])*
				(VertexPoint(bottom_cornern)[0]-
				VertexPoint(top_cornern)[0])/
				(VertexPoint(bottom_cornern)[1]-
				VertexPoint(top_cornern)[1]))
			  right_cornern = j;
		}
		if ((left_cornern==top_cornern&&right_cornern==bottom_cornern) ||
			(left_cornern==bottom_cornern && right_cornern==top_cornern))
		  for (j=0; j<3; j++)
		   if (j!=top_cornern && j!=bottom_cornern)
		   {
			 if (VertexPoint(j)[0]-(VertexPoint(j)[1]-
				VertexPoint(top_cornern)[1])*
				(VertexPoint(bottom_cornern)[0]-
				VertexPoint(top_cornern)[0])/
				(VertexPoint(bottom_cornern)[1]-
				VertexPoint(top_cornern)[1]) >
			    VertexPoint(right_cornern)[0]-
				(VertexPoint(right_cornern)[1]-
				VertexPoint(top_cornern)[1])*
				(VertexPoint(bottom_cornern)[0]-
				VertexPoint(top_cornern)[0])/
				(VertexPoint(bottom_cornern)[1]-
				VertexPoint(top_cornern)[1]))
			  right_cornern = j;
			 else
			  left_cornern = j;
			 break;
		   }
	}
	if (left_cornern==top_cornern || left_cornern==bottom_cornern)
	{
		second_right[0] = VertexPoint(right_cornern)[0];
		second_left[1] = VertexPoint(right_cornern)[1];
		second_left[0] = VertexPoint(top_cornern)[0]+
			(VertexPoint(bottom_cornern)[1]-
			VertexPoint(top_cornern)[1]>0? (second_left[1]-
			VertexPoint(top_cornern)[1])/
			(VertexPoint(bottom_cornern)[1]-
			VertexPoint(top_cornern)[1])*
			(VertexPoint(bottom_cornern)[0]-
			VertexPoint(top_cornern)[0]): 0);
	}
	else
	{
		second_left[0] = VertexPoint(left_cornern)[0];
		second_left[1] = VertexPoint(left_cornern)[1];
		second_right[0] = VertexPoint(top_cornern)[0]+
			(VertexPoint(bottom_cornern)[1]-
			VertexPoint(top_cornern)[1]>0? (second_left[1]-
			VertexPoint(top_cornern)[1])/
			(VertexPoint(bottom_cornern)[1]-
			VertexPoint(top_cornern)[1])*
			(VertexPoint(bottom_cornern)[0]-
			VertexPoint(top_cornern)[0]): 0);
	}
	patch[0] = (Patch *)malloc(sizeof(Patch)+
		(int)(VertexPoint(bottom_cornern)[1]-
		VertexPoint(top_cornern)[1]+3)*sizeof(struct Patch_line));
	if (patch[0] == NULL)
		return (1);
	last_left_end = (int)floor(VertexPoint(top_cornern)[0]-.01);
	last_right_end = (int)ceil(VertexPoint(top_cornern)[0]+.01);
	y = patch[0]->top = (int)floor(VertexPoint(top_cornern)[1]-.01);
	if (second_left[1]-VertexPoint(top_cornern)[1] > 0)
		while (y+1.02 < second_left[1])
		{
			new_left_end = (int)floor(VertexPoint(top_cornern)[0]+(
				y+1.02-VertexPoint(top_cornern)[1])*
				(second_left[0]-VertexPoint(top_cornern)[0])/
				(second_left[1]-VertexPoint(top_cornern)[1])-.01);
			if (new_left_end < last_left_end)
				last_left_end = new_left_end;
			new_right_end = (int)ceil(VertexPoint(top_cornern)[0]+(
				y+1.02-VertexPoint(top_cornern)[1])*
				(second_right[0]-VertexPoint(top_cornern)[0])/
				(second_left[1]-VertexPoint(top_cornern)[1])+.01);
			if (new_right_end > last_right_end)
				last_right_end = new_right_end;
			patch[0]->lines[y-patch[0]->top].left = last_left_end;
			patch[0]->lines[y-patch[0]->top].right = last_right_end;
			last_left_end = new_left_end;
			last_right_end = new_right_end;
			y++;
		}
	if (y < second_left[1])
	{
		new_left_end = (int)floor(second_left[0]-.01);
		if (new_left_end < last_left_end)
			last_left_end = new_left_end;
		new_right_end = (int)ceil(second_right[0]+.01);
		if (new_right_end > last_right_end)
			last_right_end = new_right_end;
		patch[0]->lines[y-patch[0]->top].left = last_left_end;
		patch[0]->lines[y-patch[0]->top].right = last_right_end;
		last_left_end = new_left_end;
		last_right_end = new_right_end;
		y++;
	}
	if (VertexPoint(bottom_cornern)[1]-second_left[1] > 0)
		while (y+.98 < VertexPoint(bottom_cornern)[1])
		{
			new_left_end = (int)floor(second_left[0]+(y+.98-second_left[1])*
				(VertexPoint(bottom_cornern)[0]-second_left[0])/
				(VertexPoint(bottom_cornern)[1]-second_left[1])-.01);
			if (new_left_end < last_left_end)
				last_left_end = new_left_end;
			new_right_end = (int)ceil(second_right[0]+(y+.98-second_left[1])*
				(VertexPoint(bottom_cornern)[0]-second_right[0])/
				(VertexPoint(bottom_cornern)[1]-second_left[1])+.01);
			if (new_right_end > last_right_end)
				last_right_end = new_right_end;
			patch[0]->lines[y-patch[0]->top].left = last_left_end;
			patch[0]->lines[y-patch[0]->top].right = last_right_end;
			last_left_end = new_left_end;
			last_right_end = new_right_end;
			y++;
		}
	new_left_end = (int)floor(VertexPoint(bottom_cornern)[0]-.01);
	if (new_left_end < last_left_end)
		last_left_end = new_left_end;
	new_right_end = (int)ceil(VertexPoint(bottom_cornern)[0]+.01);
	if (new_right_end > last_right_end)
		last_right_end = new_right_end;
	patch[0]->lines[y-patch[0]->top].left = last_left_end;
	patch[0]->lines[y-patch[0]->top].right = last_right_end;
	patch[0]->bottom = y+1;

	return (0);
}

/*****************************************************************************
 * FUNCTION: get_margin_patch
 * DESCRIPTION: Initializes a patch describing the projection of a voxel.
 * PARAMETERS:
 *    patch: A pointer to a structure which describes a shape to be painted
 *       on an object's image buffer as a list of scan-line segments is put
 *       here.  The number of segments is (*patch)->bottom - (*patch)->top and
 *       the left and right ends of the n'th segment are
 *       (*patch)->lines[n-1].left and (*patch)->lines[n-1].right.
 *    top_margin, bottom_margin, left_margin, right_margin: Estimates of the
 *       average overlap or margin that the patch extends beyond the projection
 *       of the voxel in pixels or fraction of a pixel go here.  These are used
 *       to fade the edges when the fade option is on.
 *    projection_matrix: The matrix which transforms a vector in object space
 *       in units of voxels to image space in units of pixels.
 *    max_depth: The diameter of the voxel in depth units
 * SIDE EFFECTS: The memory at *patch will be freed.
 * ENTRY CONDITIONS: *patch must be NULL or a valid patch pointer.
 * RETURN VALUE:
 *    0: success
 *    1: memory allocation failure
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 3/27/91 by Dewey Odhner
 *    Modified: 2/28/94 return error code by Dewey Odhner
 *    Modified: 10/28/98 weights calculated by Dewey Odhner
 *    Modified: 12/19/08 old pointer not kept by Dewey Odhner.
 *
 *****************************************************************************/
int get_margin_patch(Patch **patch, double *top_margin, double *bottom_margin,
	double *left_margin, double *right_margin,
	double projection_matrix[3][3], double max_depth)
{
	int top_cornern, second_left, second_right, j, k, x, y,
			last_right_end, new_right_end=0, left_cornern, right_cornern,
			new_left_end=0, last_left_end, leftmost_end, rightmost_end;
	double voxel_corner[8][2];
	double *left_corners[4], *right_corners[4];
	double *leftmost_corner, *rightmost_corner;
	double front, back, qp;
	double xp[3][3], q[3][3], qm[3];

	voxel_corner[0][0] = (projection_matrix[0][0]+projection_matrix[0][1]+
			projection_matrix[0][2])/2;
	voxel_corner[1][0] = (projection_matrix[0][0]-projection_matrix[0][1]+
			projection_matrix[0][2])/2;
	voxel_corner[2][0] = (projection_matrix[0][0]+projection_matrix[0][1]-
			projection_matrix[0][2])/2;
	voxel_corner[3][0] = (projection_matrix[0][0]-projection_matrix[0][1]-
			projection_matrix[0][2])/2;
	voxel_corner[4][0] = -voxel_corner[3][0];
	voxel_corner[5][0] = -voxel_corner[2][0];
	voxel_corner[6][0] = -voxel_corner[1][0];
	voxel_corner[7][0] = -voxel_corner[0][0];
	voxel_corner[0][1] = (projection_matrix[1][0]+projection_matrix[1][1]+
			projection_matrix[1][2])/2;
	voxel_corner[1][1] = (projection_matrix[1][0]-projection_matrix[1][1]+
			projection_matrix[1][2])/2;
	voxel_corner[2][1] = (projection_matrix[1][0]+projection_matrix[1][1]-
			projection_matrix[1][2])/2;
	voxel_corner[3][1] = (projection_matrix[1][0]-projection_matrix[1][1]-
			projection_matrix[1][2])/2;
	voxel_corner[4][1] = -voxel_corner[3][1];
	voxel_corner[5][1] = -voxel_corner[2][1];
	voxel_corner[6][1] = -voxel_corner[1][1];
	voxel_corner[7][1] = -voxel_corner[0][1];

	/* Change coordinates */
	for (j=0; j<=7; j++)
		for (k=0; k<=1; k++)
			voxel_corner[j][k] -= .25;

	for (j=1, top_cornern=0; j<8; j++)
		if (voxel_corner[j][1]<voxel_corner[top_cornern][1])
			top_cornern=j;
	second_left = second_right = top_cornern^1;
	for (j=2; j<=4; j<<=1)
	{	if
		(	(voxel_corner[top_cornern^j][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[second_left][1]-voxel_corner[top_cornern][1]) <
			(voxel_corner[second_left][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[top_cornern^j][1]-voxel_corner[top_cornern][1])
		)
			second_left = top_cornern^j;
		if
		(	(voxel_corner[top_cornern^j][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[second_right][1]-voxel_corner[top_cornern][1]) >
			(voxel_corner[second_right][0]-voxel_corner[top_cornern][0])*
			(voxel_corner[top_cornern^j][1]-voxel_corner[top_cornern][1])
		)
			second_right = top_cornern^j;
	}
	left_corners[0] = right_corners[0] = voxel_corner[top_cornern];
	left_corners[1] = voxel_corner[second_left];
	right_corners[1] = voxel_corner[second_right];
	left_corners[2] = voxel_corner[7-second_right];
	right_corners[2] = voxel_corner[7-second_left];
	left_corners[3] = right_corners[3] = voxel_corner[7-top_cornern];
	if (*patch && (*patch)->lines[0].weight)
	{
		free((*patch)->lines[0].weight);
		(*patch)->lines[0].weight = NULL;
	}
	int array_size = (int)(left_corners[3][1]-left_corners[0][1])+15;
	if (*patch)
		free(*patch);
	*patch = (Patch *)malloc(sizeof(Patch)+
		(array_size-1)*sizeof(struct Patch_line));
	if (*patch == NULL)
		return (1);
	left_cornern = right_cornern = 0;
	(*patch)->lines[0].left =
		leftmost_end = last_left_end = (int)floor(left_corners[0][0]);
	(*patch)->lines[0].right =
		rightmost_end = last_right_end = (int)ceil(right_corners[0][0]);
	y = (int)floor(left_corners[0][1]);
	(*patch)->top = y-1;
	while (left_cornern < 3)
	{	while (left_cornern<3 && left_corners[left_cornern+1][1]<y+1)
		{	left_cornern++;
			new_left_end = (int)floor(left_corners[left_cornern][0]);
			if (new_left_end < last_left_end)
				last_left_end = new_left_end;
		}
		while (right_cornern<3 && right_corners[right_cornern+1][1]<y+1)
		{	right_cornern++;
			new_right_end = (int)ceil(right_corners[right_cornern][0]);
			if (new_right_end > last_right_end)
				last_right_end = new_right_end;
		}
		j = left_cornern<3? left_cornern: 2;
		if (left_corners[j+1][1] > left_corners[j][1]+.00001)
		{	new_left_end =
				(int)floor(left_corners[j][0]+
					(y+1-left_corners[j][1])*
					(left_corners[j+1][0]-
						left_corners[j][0])/
					(left_corners[j+1][1]-
						left_corners[j][1]));
			if (new_left_end < last_left_end)
				last_left_end = new_left_end;
		}
		k = right_cornern<3? right_cornern: 2;
		if (right_corners[k+1][1]>right_corners[k][1]+.00001)
		{	new_right_end =
				(int)ceil(right_corners[k][0]+
					(y+1-right_corners[k][1])*
					(right_corners[k+1][0]-
						right_corners[k][0])/
					(right_corners[k+1][1]-
						right_corners[k][1]));
			if (new_right_end > last_right_end)
				last_right_end = new_right_end;
		}
		(*patch)->lines[y-(*patch)->top].left = last_left_end;
		(*patch)->lines[y-(*patch)->top].right = last_right_end;
		if (last_left_end < leftmost_end)
			leftmost_end = last_left_end;
		if (last_right_end > rightmost_end)
			rightmost_end = last_right_end;
		last_left_end = new_left_end;
		last_right_end = new_right_end;
		y++;
	}
	(*patch)->lines[y-(*patch)->top].left = last_left_end;
	(*patch)->lines[y-(*patch)->top].right = last_right_end;
	(*patch)->bottom = y+1;

	/* dilate patch: */
	last_left_end = (*patch)->lines[0].left;
	last_right_end = (*patch)->lines[0].right;
	k = 0;
	for (y=(*patch)->top; y<(*patch)->bottom; y++)
	{	new_left_end = --(*patch)->lines[y-(*patch)->top].left;
		if (last_left_end < new_left_end)
			(*patch)->lines[y-(*patch)->top].left = last_left_end;
		if (y<(*patch)->bottom-1 &&
				(*patch)->lines[y-(*patch)->top+1].left-1<
				(*patch)->lines[y-(*patch)->top].left)
			(*patch)->lines[y-(*patch)->top].left =
				(*patch)->lines[y-(*patch)->top+1].left-1;
		last_left_end = new_left_end;
		new_right_end = ++(*patch)->lines[y-(*patch)->top].right;
		if (last_right_end > new_right_end)
			(*patch)->lines[y-(*patch)->top].right = last_right_end;
		if (y<(*patch)->bottom-1 &&
				(*patch)->lines[y-(*patch)->top+1].right+1>
				(*patch)->lines[y-(*patch)->top].right)
			(*patch)->lines[y-(*patch)->top].right =
				(*patch)->lines[y-(*patch)->top+1].right+1;
		last_right_end = new_right_end;
		k += (*patch)->lines[y-(*patch)->top].right-
			(*patch)->lines[y-(*patch)->top].left;
	}
	leftmost_end--;
	rightmost_end++;

	/* compute weights */
	(*patch)->lines[0].weight = (unsigned char *)malloc(k);
	if ((*patch)->lines[0].weight == NULL)
		return (1);
	for (y=(*patch)->top+1; y<(*patch)->bottom; y++)
		(*patch)->lines[y-(*patch)->top].weight =
			(*patch)->lines[y-(*patch)->top-1].weight+
			(*patch)->lines[y-(*patch)->top-1].right-
			(*patch)->lines[y-(*patch)->top-1].left;
	for (j=0; j<3; j++)
		for (k=0; k<3; k++)
			xp[j][k] =
				projection_matrix[(k+1)%3][(j+1)%3]*
				projection_matrix[(k+2)%3][(j+2)%3]-
				projection_matrix[(k+2)%3][(j+1)%3]*
				projection_matrix[(k+1)%3][(j+2)%3];
	qp = .25;
	for (j=0; j<3; j++)
	{
		q[j][0] = 0;
		for (k=0; k<3; k++)
			q[j][0] += xp[j][k]*projection_matrix[k][j];
		q[j][0] /= 2*(xp[j][0]*xp[j][0]+xp[j][1]*xp[j][1]+xp[j][2]*xp[j][2]);
		q[j][1] = q[j][2] = q[j][0];
		qm[j] = 0;
		for (k=0; k<3; k++)
		{
			q[j][k] *= xp[j][k];
			qm[j] += q[j][k]*q[j][k];
		}
		if (qm[j]*.25 < qp*qp)
			qp = sqrt(qm[j]*.25);
	}
	for (y=(*patch)->top; y<(*patch)->bottom; y++)
		for (x=(*patch)->lines[y-(*patch)->top].left;
				x<(*patch)->lines[y-(*patch)->top].right; x++)
		{
			front = .5*max_depth;
			back = -front;
			for (j=0; j<3; j++)
			if (q[j][2])
			{
				if (front*q[j][2] > qm[j]-((x+qp)*q[j][0]+(y+qp)*q[j][1]))
					front = (qm[j]-((x+qp)*q[j][0]+(y+qp)*q[j][1]))/q[j][2];
				if (front*-q[j][2] > qm[j]+((x+qp)*q[j][0]+(y+qp)*q[j][1]))
					front = (qm[j]+((x+qp)*q[j][0]+(y+qp)*q[j][1]))/-q[j][2];
				if (back*q[j][2] > qm[j]-((x+qp)*q[j][0]+(y+qp)*q[j][1]))
					back = (qm[j]-((x+qp)*q[j][0]+(y+qp)*q[j][1]))/q[j][2];
				if (back*-q[j][2] > qm[j]+((x+qp)*q[j][0]+(y+qp)*q[j][1]))
					back = (qm[j]+((x+qp)*q[j][0]+(y+qp)*q[j][1]))/-q[j][2];
			}
			else
				if (0>qm[j]-((x+qp)*q[j][0]+(y+qp)*q[j][1]) || 
						0>qm[j]+((x+qp)*q[j][0]+(y+qp)*q[j][1]))
					front = back;
			(*patch)->lines[y-(*patch)->top].weight[x-(*patch)->lines[
				y-(*patch)->top].left] = (int)rint(255/max_depth*(front-back));
		}

	*top_margin = left_corners[0][1]-(*patch)->top;
	*bottom_margin = (*patch)->bottom-right_corners[3][1];
	leftmost_corner = left_corners[0];
	rightmost_corner = right_corners[0];
	for (j=1; j<3; j++)
	{	if (left_corners[j][0] < leftmost_corner[0])
			leftmost_corner = left_corners[j];
		if (right_corners[j][0] > rightmost_corner[0])
			rightmost_corner = right_corners[j];
	}
	*left_margin = leftmost_corner[0]-leftmost_end;
	*right_margin = rightmost_end-rightmost_corner[0];
	return (0);
}

/*****************************************************************************
 * FUNCTION: trim_patch
 * DESCRIPTION: Trims a weighted patch describing the projection of a voxel
 *    to remove pixels with zero weight.
 * PARAMETERS:
 *    patch: A pointer to a structure which describes a shape to be painted
 *       on an object's image buffer as a list of scan-line segments is
 *       here.  The number of segments is (*patch)->bottom - (*patch)->top and
 *       the left and right ends of the n'th segment are
 *       (*patch)->lines[n-1].left and (*patch)->lines[n-1].right.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 10/19/98 by Dewey Odhner
 *
 *****************************************************************************/
void trim_patch(Patch *patch)
{
	int y;

	if (patch->lines[0].weight == NULL)
		return;
	for (y=patch->top; y<patch->bottom-1; )
	{
		while (patch->lines[y-patch->top].right>patch->lines[y-patch->top].left
				&& patch->lines[y-patch->top].weight[patch->lines[y-patch->top]
				.right-patch->lines[y-patch->top].left-1]==0)
			patch->lines[y-patch->top].right--;
		y++;
		while (patch->lines[y-patch->top].right>patch->lines[y-patch->top].left
				&& patch->lines[y-patch->top].weight[0]==0)
		{
			patch->lines[y-patch->top].weight++;
			patch->lines[y-patch->top].left++;
		}
	}
}
