/*
  Copyright 1993-2008 Medical Image Processing Group
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

/* Return the gradient code */
unsigned short G_code(gx, gy, gz)
	double gx, gy, gz;
{
	int face;
	double max_component, n1, n2;
	unsigned t1, t2;

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
			n1 = -gy;
			n2 = -gz;
			break;
		case 1:
		case 4:
			n1 = -gx;
			n2 = -gz;
			break;
		case 2:
		case 5:
			n1 = -gx;
			n2 = -gy;
			break;
		case 6:
			return (6 << 2*G_COMPONENT_BITS);
	}
	t1 = n1/max_component*G_NORM+G_NORM;
	if (t1 >= 2*G_NORM)
		t1 = 2*G_NORM-1;
	t2 = n2/max_component*G_NORM+G_NORM;
	if (t2 >= 2*G_NORM)
		t2 = 2*G_NORM-1;
	return((unsigned short)
		(face<<2*G_COMPONENT_BITS | t1<<G_COMPONENT_BITS | t2));
}
