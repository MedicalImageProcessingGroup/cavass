/*
  Copyright 1993-2013 Medical Image Processing Group
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
#include <math.h>


static double *FP, *FN;

int compar_FP(const void *s1, const void *s2)
{
	int i1=*((int *)s1), i2=*((int *)s2);
	if (FP[i1] < FP[i2])
		return -1;
	if (FP[i1] > FP[i2])
		return 1;
	if (FN[i1] < FN[i2])
		return -1;
	if (FN[i1] > FN[i2])
		return 1;
	return 0;
}

int main(int argc, char **argv)
{
	int num_points, *by_FP, j, best_rms=0;
	double best_FN=1;

	if (argc<7 || argc%3!=1)
	{
		fprintf(stderr,"Usage: screen_params <label> ... <FP> ... <FN> ...\n");
		exit(1);
	}
	num_points = argc/3;
	FP = (double *)malloc(num_points*sizeof(double));
	FN = (double *)malloc(num_points*sizeof(double));
	by_FP = (int *)malloc(num_points*sizeof(int));
	for (j=0; j<num_points; j++)
	{
		by_FP[j] = j;
		if (sscanf(argv[1+num_points+j], "%lf", FP+j)!=1 ||
				sscanf(argv[1+2*num_points+j], "%lf", FN+j)!=1 ||
				FP[j]<0 || FN[j]<0 || FN[j]>1)
		{
			fprintf(stderr, "Invalid FP or FN value.\n");
			exit(1);
		}
	}
	qsort(by_FP, num_points, sizeof(int), compar_FP);
	for (j=0; j<num_points; j++)
	{
		if (FN[by_FP[j]] < best_FN)
		{
			double rms;
			best_FN = FN[by_FP[j]];
			printf("%s\n", argv[1+by_FP[j]]);
			rms = sqrt(best_FN*best_FN+FP[by_FP[j]]*FP[by_FP[j]]);
			if (rms< sqrt(FN[best_rms]*FN[best_rms]+FP[best_rms]*FP[best_rms]))
				best_rms = by_FP[j];
		}
		while (j+1<num_points && FP[by_FP[j+1]]==FP[by_FP[j]])
			j++;
	}
	printf("best: %s\n", argv[1+best_rms]);
	exit(0);
}
