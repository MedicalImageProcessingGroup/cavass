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

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	double x, sum=0;
	int n;

	for (n=1; n<argc; n++)
	{
		if (sscanf(argv[n], "%lf", &x) != 1)
		{
			fprintf(stderr, "mean failed.\n");
			exit(1);
		}
		sum += x;
	}
	printf("%f\n", sum/(n-1));
	exit(0);
}
