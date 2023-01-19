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

 
 








/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/



/*-------------------------------------------------------------------------*/
float
cinterpolate(i1, i2, i3, i4, off)
float i1, i2, i3, i4;
float off; /* offset of the second (i2) point. Offset is between 0 and 1. */
{
	float A, B, C, D, C_D, A_B;
	float value;

/*
    GLOBALS: float Spacing, s_cube, two_s_cube, six_s_cube;
 
    Spacing = spacing between points (slice spacing)
    s_cube = s*s*s;
    two_s_cube = s_cube*2.0;
    six_s_cube = s_cube*6.0;
*/
	float s;
	float s_cube, two_s_cube, six_s_cube;

	s = 1.0;
	s_cube = s*s*s;
	two_s_cube = 2*s_cube;
	six_s_cube = s_cube*6.0;

 
	A = off + s;
	B = off;
	C = off - s;
	D = C - s;
 
	A_B = A * B;
	C_D = C * D;
 
	value =	(B*C_D/-six_s_cube) * i1 +
			(A*C_D/ two_s_cube) * i2 +
			(A_B*D/-two_s_cube) * i3 +
			(A_B*C/ six_s_cube) * i4;

	return(value);
}



main()
{
	char line[100];
	float i1, i2, i3, i4, off;

	printf("Point 1 = ");
	gets(line);
	sscanf(line, "%f", &i1);

	printf("Point 2 = ");
	gets(line);
	sscanf(line, "%f", &i2);

	printf("Point 3 = ");
	gets(line);
	sscanf(line, "%f", &i3);

	printf("Point 4 = ");
	gets(line);
	sscanf(line, "%f", &i4);

	printf("Offset  = ");
	gets(line);
	sscanf(line, "%f", &off);

	printf("RESULT = %f\n", cinterpolate(i1, i2, i3, i4, off));

}


