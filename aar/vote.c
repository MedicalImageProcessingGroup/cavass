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

static double **x;
static int voter, *nvotes, *nvotes2;

int compar_x(const void *s1, const void *s2)
{
	int i1=*((int *)s1), i2=*((int *)s2);
	if (x[i1][voter] < x[i2][voter])
		return -1;
	if (x[i1][voter] > x[i2][voter])
		return 1;
	return 0;
}

int compar_nvotes(const void *s1, const void *s2)
{
	int i1=*((int *)s1), i2=*((int *)s2);
	if (nvotes[i1] > nvotes[i2])
		return -1;
	if (nvotes[i1] < nvotes[i2])
		return 1;
	if (nvotes2[i1] > nvotes2[i2])
		return -1;
	if (nvotes2[i1] < nvotes2[i2])
		return 1;
	return 0;
}

// vote for file with smallest cost
int main(int argc, char **argv)
{
	FILE *fp;
	int num_lines, *rankedlist, j, k;

	if (argc<3 || sscanf(argv[1], "%d", &num_lines)!=1)
	{
		fprintf(stderr, "Usage: vote <num lines> file ...\n");
		exit(1);
	}
	x = (double **)malloc((argc-2)*sizeof(double *));
	rankedlist = (int *)malloc((argc-2)*sizeof(int));
	nvotes = (int *)malloc((argc-2)*sizeof(int));
	nvotes2 = (int *)malloc((argc-2)*sizeof(int));
	for (j=0; j<argc-2; j++)
	{
		x[j] = (double *)malloc(num_lines*sizeof(double));
		fp = fopen(argv[j+2], "rb");
		if (fp == NULL)
		{
			fprintf(stderr, "Could not open %s\n", argv[j+2]);
			exit(1);
		}
		for (k=0; k<num_lines; k++)
		{
			if (fscanf(fp, "%lf\n", x[j]+k) != 1)
			{
				fprintf(stderr, "Failure reading file %s\n", argv[j+2]);
				exit(1);
			}
		}
		fclose(fp);
		nvotes[j] = nvotes2[j] = 0;
	}
	for (voter=0; voter<num_lines; voter++)
	{
		for (j=0; j<argc-2; j++)
			rankedlist[j] = j;
		qsort(rankedlist, argc-2, sizeof(int), compar_x);
		nvotes[rankedlist[0]]++;
		nvotes2[rankedlist[1]]++;
	}
	qsort(rankedlist, argc-2, sizeof(int), compar_nvotes);
	for (j=0; j<argc-2; j++)
		printf("%s %d %d\n", argv[rankedlist[j]+2], nvotes[rankedlist[j]],
			nvotes2[rankedlist[j]]);
	exit(0);
}
