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

#include "neighbors.h"

/*****************************************************************************
 * FUNCTION: G_code
 * DESCRIPTION: Computes the normal code from a gradient.
 * PARAMETERS:
 *    gx, gy, gz: The components of the gradient
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The normal code
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1992 Dewey Odhner
 *
 *****************************************************************************/
unsigned short G_code(double gx, double gy, double gz)
{
	int face;
	double max_component, nu2, nu3;
	unsigned n2, n3;

	face = 6;
	max_component = 0;
	if (gx < 0)
	{	face = 0;
		max_component = -gx;
	}
	else if (gx > 0)
	{	face = 3;
		max_component = gx;
	}
	if (gy < -max_component)
	{	face = 1;
		max_component = -gy;
	}
	else if (gy > max_component)
	{	face = 4;
		max_component = gy;
	}
	if (gz < -max_component)
	{	face = 2;
		max_component = -gz;
	}
	else if (gz > max_component)
	{	face = 5;
		max_component = gz;
	}
	switch (face)
	{	case 0:
		case 3:
			nu2 = -gy;
			nu3 = -gz;
			break;
		case 1:
		case 4:
			nu2 = -gx;
			nu3 = -gz;
			break;
		case 2:
		case 5:
			nu2 = -gx;
			nu3 = -gy;
			break;
		case 6:
			return (6 << 2*G_COMPONENT_BITS);
	}
	n2 = (unsigned)(nu2/max_component*G_NORM+G_NORM);
	if (n2 >= 2*G_NORM)
		n2 = 2*G_NORM-1;
	n3 = (unsigned)(nu3/max_component*G_NORM+G_NORM);
	if (n3 >= 2*G_NORM)
		n3 = 2*G_NORM-1;
	return((unsigned short)
		(face<<2*G_COMPONENT_BITS | n2<<G_COMPONENT_BITS | n3));
}

/*****************************************************************************
 * FUNCTION: BG_code
 * DESCRIPTION: Computes the normal code from a gradient.
 * PARAMETERS:
 *    gx, gy, gz: The components of the gradient
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The normal code
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 5/16/01 Dewey Odhner
 *
 *****************************************************************************/
unsigned short BG_code(double gx, double gy, double gz)
{
	int face;
	double max_component, nu2, nu3;
	unsigned n2, n3;

	face = 6;
	max_component = 0;
	if (gx < 0)
	{	face = 0;
		max_component = -gx;
	}
	else if (gx > 0)
	{	face = 3;
		max_component = gx;
	}
	if (gy < -max_component)
	{	face = 1;
		max_component = -gy;
	}
	else if (gy > max_component)
	{	face = 4;
		max_component = gy;
	}
	if (gz < -max_component)
	{	face = 2;
		max_component = -gz;
	}
	else if (gz > max_component)
	{	face = 5;
		max_component = gz;
	}
	switch (face)
	{	case 0:
		case 3:
			nu2 = -gy;
			nu3 = -gz;
			break;
		case 1:
		case 4:
			nu2 = -gx;
			nu3 = -gz;
			break;
		case 2:
		case 5:
			nu2 = -gx;
			nu3 = -gy;
			break;
		case 6:
			return (6 << 2*BG_COMPONENT_BITS);
	}
	n2 = (unsigned)(nu2/max_component*BG_NORM+BG_NORM);
	if (n2 >= 2*BG_NORM)
		n2 = 2*BG_NORM-1;
	n3 = (unsigned)(nu3/max_component*BG_NORM+BG_NORM);
	if (n3 >= 2*BG_NORM)
		n3 = 2*BG_NORM-1;
	return((unsigned short)
		(face<<2*BG_COMPONENT_BITS | n2<<BG_COMPONENT_BITS | n3));
}
