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

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

int main(int argc, char **argv)
{
	double *mean, *std, factor, *high, *low;
	int nfg, ntot, fg_flag, j, ncomp, nout, *tissue_type;

	if (argc!=3 || (argv[1][0]!='f' && argv[1][0]!='b') || argv[1][1]!=0 ||
			sscanf(argv[2], "%lf", &factor)!=1)
	{
		fprintf(stderr, "Usage: comp_interv [f|b] <factor>\n");
		exit(-1);
	}
	fg_flag = argv[1][0]=='f';
	if (scanf("%d\n", &ntot) != 1)
	{
		fprintf(stderr, "read failure\n");
		exit(-1);
	}
	mean = (double *)malloc(ntot*sizeof(double));
	std = (double *)malloc(ntot*sizeof(double));
	tissue_type = (int *)malloc(ntot*sizeof(int));
	for (j=0; j<ntot; j++)
	{
		if (scanf("%lf %lf\n", mean+j, std+j) != 2)
		{
			fprintf(stderr, "read failure\n");
			exit(-1);
		}
	}
	if (scanf("%d\n", &nfg) != 1)
	{
		fprintf(stderr, "read failure\n");
		exit(-1);
	}
	for (j=0; j<ntot; j++)
		if (scanf(" %d", tissue_type+j) != 1)
		{
			fprintf(stderr, "read failure\n");
			exit(-1);
		}
	if (fg_flag)
	{
		ncomp = nfg;
	}
	else
	{
		ncomp = ntot-nfg;
		mean += nfg;
		std += nfg;
		tissue_type += nfg;
	}
	high = (double *)malloc(ncomp*sizeof(double));
	low = (double *)malloc(ncomp*sizeof(double));
	for (j=nout=0; j<ncomp; j++)
	{
		int k, m;
		double lowj=mean[j]-factor*std[j], highj=mean[j]+factor*std[j];
		if (tissue_type[j]==-1 && lowj>0)
			lowj = 0;
		if (tissue_type[j]==1 && highj<65535)
			highj = 65535;
		// merge intervals
		for (k=0; k<nout-1; k++)
		{
			while (k<nout-1 && lowj<=high[k] && 
					highj>=low[k+1])
			{
				high[k] = high[k+1];
				for (m=k+1; m<nout-1; m++)
				{
					low[m] = low[m+1];
					high[m] = high[m+1];
				}
				nout--;
			}
		}
		// adjust existing interval
		m = FALSE;
		for (k=0; k<nout; k++)
		{
			if (lowj>=low[k] &&
					highj<=high[k])
			{
				m = TRUE;
				break;
			}
			if (lowj<low[k] &&
					highj>=low[k])
			{
				low[k] = lowj;
				m = TRUE;
			}
			if (lowj<=high[k] &&
					highj>high[k])
			{
				high[k] = highj;
				m = TRUE;
			}
		}
		if (m)
		{
			continue;
		}
		// insert new interval
		for (k=0; k<nout; k++)
			if (highj < low[k])
				break;
		for (m=nout; m>k; m--)
		{
			low[m] = low[m-1];
			high[m] = high[m-1];
		}
		low[k] = lowj;
		high[k] = highj;
		nout++;
	}
	for (j=0; j<nout; j++)
	{
		if (high[j] <= 0)
			continue;
		if (low[j] < 0)
			low[j] = 0;
		printf(" %.0f %.0f", low[j], high[j]);
	}
	printf("\n");
	exit(0);
}
