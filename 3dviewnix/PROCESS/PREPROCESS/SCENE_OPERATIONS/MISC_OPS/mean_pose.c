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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "render/matrix.cpp"

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

#ifndef M_PI
#define M_PI 3.141592653589793238462433832795
#endif

double Det3(double m[3][3]);

int main(argc,argv)
int argc;
char *argv[];
{
  int i, j, rel_flag=FALSE;
  double loc_sum[3], ploc_sum[3];
  double vol_sum=0, objsz_sum=0, pobjsz_sum=0;
  double *vol;
  double *objsz, *pobjsz;
  double (*loc)[3], (*ploc)[3];
  int scale_method=1;
  double (*eval)[3], (*peval)[3];
  double (*evec)[3][3], (*pevec)[3][3];
  double eval_sum[3], evec_sum[3][3], ang_sumsqr[3];
  double peval_sum[3], pevec_sum[3][3];
  int alignment_consistent=FALSE;
  double tfom;
  int ns; // number of samples
  double rel_evec_sum[3][3];

  if (strncmp(argv[argc-1], "-m", 2) == 0)
  {
    if (sscanf(argv[argc-1]+2, "%d", &scale_method)<1 || scale_method<1 ||
	    scale_method>4)
	{
	  fprintf(stderr, "Not a known scale estimation method.\n");
	  exit(-1);
	}
    argc--;
  }
  if (strncmp(argv[argc-1], "-r", 2) == 0)
  {
    rel_flag = TRUE;
	argc--;
  }
  ns = rel_flag? (argc-1)/2: argc-1;
  if (ns<1 || (rel_flag && argc%2==0)) {
    fprintf(stderr,"Usage: mean_pose <input scene> ... [<parent scene> ... -r] [-m<n>]\n");
	fprintf(stderr, " <n>: scale method (1 to 4)\n");
    exit(-1);
  }
  if (scale_method == 4)
	scale_method = 2;
  vol = (double *)malloc(ns*sizeof(double));
  objsz = (double *)malloc(ns*sizeof(double));
  pobjsz = (double *)malloc(ns*sizeof(double));
  loc = (double (*)[3])malloc(ns*sizeof(double)*3);
  ploc = (double (*)[3])malloc(ns*sizeof(double)*3);
  eval = (double (*)[3])malloc(ns*sizeof(double)*3);
  peval = (double (*)[3])malloc(ns*sizeof(double)*3);
  evec = (double (*)[3][3])malloc(ns*sizeof(double)*9);
  pevec = (double (*)[3][3])malloc(ns*sizeof(double)*9);
  eval_sum[0] = eval_sum[1] = eval_sum[2] = 0;
  peval_sum[0] = peval_sum[1] = peval_sum[2] = 0;
  for (j=0; j<ns; j++)
  {
    FILE *tfp;
    char *cmd=(char *)malloc(strlen(argv[j+1])+32);
	char line[80];

	sprintf(cmd, "bin_volume %s -l%d > FOM-TMP-INFO", argv[j+1], scale_method);
	system(cmd);
	tfp = fopen("FOM-TMP-INFO", "rb");
	fgets(line, 80, tfp);
	fgets(line, 80, tfp);
	fscanf(tfp, "Volume                  -> %lf\n", vol+j);
    fscanf(tfp, "Center                  -> (%lf, %lf, %lf)\n",
	  &loc[j][0], &loc[j][1], &loc[j][2]);
	fscanf(tfp, "Scale Measure           -> %lf\n", objsz+j);
	fscanf(tfp, "Eigenvalues             -> {%lf, %lf, %lf}\n",
	  eval[j], eval[j]+1, eval[j]+2);
    fscanf(tfp, "Eigenvector 1           -> (%lf, %lf, %lf)\n",
	  evec[j][0], evec[j][0]+1, evec[j][0]+2);
    fscanf(tfp, "Eigenvector 2           -> (%lf, %lf, %lf)\n",
	  evec[j][1], evec[j][1]+1, evec[j][1]+2);
    fscanf(tfp, "Eigenvector 3           -> (%lf, %lf, %lf)\n",
	  evec[j][2], evec[j][2]+1, evec[j][2]+2);
	fclose(tfp);
	vol_sum += vol[j];
    objsz_sum += objsz[j];
	eval_sum[0] += eval[j][0];
	eval_sum[1] += eval[j][1];
	eval_sum[2] += eval[j][2];
	free(cmd);
	if (rel_flag)
	{
	  cmd = (char *)malloc(strlen(argv[ns+1+j])+32);
	  sprintf(cmd, "bin_volume %s -l%d > FOM-TMP-INFO", argv[ns+1+j],
	    scale_method);
	  system(cmd);
	  tfp = fopen("FOM-TMP-INFO", "rb");
	  fgets(line, 80, tfp);
	  fgets(line, 80, tfp);
	  fscanf(tfp, "Volume                  -> %*f\n");
	  fscanf(tfp, "Center                  -> (%lf, %lf, %lf)\n",
	    &ploc[j][0], &ploc[j][1], &ploc[j][2]);
	  fscanf(tfp, "Scale Measure           -> %lf\n", pobjsz+j);
	  fscanf(tfp, "Eigenvalues             -> {%lf, %lf, %lf}\n",
	    peval[j], peval[j]+1, peval[j]+2);
	  fscanf(tfp, "Eigenvector 1           -> (%lf, %lf, %lf)\n",
	    pevec[j][0], pevec[j][0]+1, pevec[j][0]+2);
	  fscanf(tfp, "Eigenvector 2           -> (%lf, %lf, %lf)\n",
	    pevec[j][1], pevec[j][1]+1, pevec[j][1]+2);
	  fscanf(tfp, "Eigenvector 3           -> (%lf, %lf, %lf)\n",
	    pevec[j][2], pevec[j][2]+1, pevec[j][2]+2);
	  fclose(tfp);
	  pobjsz_sum += pobjsz[j];
	  peval_sum[0] += peval[j][0];
	  peval_sum[1] += peval[j][1];
	  peval_sum[2] += peval[j][2];
	  free(cmd);
	}
  }
  for (j=0; j<ns; j++)
  {
    if (Det3(evec[j]) < 0)
    {
      evec[j][2][0] = -evec[j][2][0];
      evec[j][2][1] = -evec[j][2][1];
      evec[j][2][2] = -evec[j][2][2];
    }
	if (Det3(pevec[j]) < 0)
	{
	  pevec[j][2][0] = -pevec[j][2][0];
	  pevec[j][2][1] = -pevec[j][2][1];
	  pevec[j][2][2] = -pevec[j][2][2];
	}
  }
  for (i=0; !alignment_consistent; i++)
  {
    int k, m, j_consistent;
    memset(evec_sum, 0, sizeof(evec_sum));
    for (j=0; j<ns; j++)
	  for (k=0; k<3; k++)
	    for (m=0; m<3; m++)
	      evec_sum[k][m] += evec[j][k][m];
    alignment_consistent = TRUE;
	for (j=0; j<ns; j++)
    {
	  j_consistent = TRUE;
	  for (k=0; k<3 && j_consistent; k++)
          if (evec_sum[k][0]*evec[j][k][0]+
              evec_sum[k][1]*evec[j][k][1]+
              evec_sum[k][2]*evec[j][k][2] < 0)
          {
            j_consistent = alignment_consistent = FALSE;
            evec[j][k][0] = -evec[j][k][0];
            evec[j][k][1] = -evec[j][k][1];
            evec[j][k][2] = -evec[j][k][2];
            m = evec_sum[(k+1)%3][0]*evec[j][(k+1)%3][0]+
                evec_sum[(k+1)%3][1]*evec[j][(k+1)%3][1]+
                evec_sum[(k+1)%3][2]*evec[j][(k+1)%3][2] <
                evec_sum[(k+2)%3][0]*evec[j][(k+2)%3][0]+
                evec_sum[(k+2)%3][1]*evec[j][(k+2)%3][1]+
                evec_sum[(k+2)%3][2]*evec[j][(k+2)%3][2] ? (k+1)%3: (k+2)%3;
            evec[j][m][0] = -evec[j][m][0];
            evec[j][m][1] = -evec[j][m][1];
            evec[j][m][2] = -evec[j][m][2];
          }
    }
	if (!alignment_consistent && i>ns*ns*3)
    {
      fprintf(stderr, "Axes are inconsistent!\n");
      break;
    }
  }
  alignment_consistent = FALSE;
  for (i=0; !alignment_consistent; i++)
  {
    int k, m, j_consistent;
    memset(pevec_sum, 0, sizeof(pevec_sum));
    for (j=0; j<ns; j++)
	  for (k=0; k<3; k++)
	    for (m=0; m<3; m++)
	      pevec_sum[k][m] += pevec[j][k][m];
    alignment_consistent = TRUE;
	for (j=0; j<ns; j++)
    {
	  j_consistent = TRUE;
	  for (k=0; k<3 && j_consistent; k++)
          if (pevec_sum[k][0]*pevec[j][k][0]+
              pevec_sum[k][1]*pevec[j][k][1]+
              pevec_sum[k][2]*pevec[j][k][2] < 0)
          {
            j_consistent = alignment_consistent = FALSE;
            pevec[j][k][0] = -pevec[j][k][0];
            pevec[j][k][1] = -pevec[j][k][1];
            pevec[j][k][2] = -pevec[j][k][2];
            m = pevec_sum[(k+1)%3][0]*pevec[j][(k+1)%3][0]+
                pevec_sum[(k+1)%3][1]*pevec[j][(k+1)%3][1]+
                pevec_sum[(k+1)%3][2]*pevec[j][(k+1)%3][2] <
                pevec_sum[(k+2)%3][0]*pevec[j][(k+2)%3][0]+
                pevec_sum[(k+2)%3][1]*pevec[j][(k+2)%3][1]+
                pevec_sum[(k+2)%3][2]*pevec[j][(k+2)%3][2] ? (k+1)%3: (k+2)%3;
            pevec[j][m][0] = -pevec[j][m][0];
            pevec[j][m][1] = -pevec[j][m][1];
            pevec[j][m][2] = -pevec[j][m][2];
          }
    }
	if (!alignment_consistent && i>ns*ns*3)
    {
      fprintf(stderr, "Axes are inconsistent!\n");
      break;
    }
  }
  vol_sum *= 1./ns;
  objsz_sum *= 1./ns;
  pobjsz_sum *= 1./ns;
  loc_sum[0] = loc_sum[1] = loc_sum[2] = 0;
  ploc_sum[0] = ploc_sum[1] = ploc_sum[2] = 0;
  for (j=0; j<ns; j++)
  {
    double rel_scale=objsz[j]/objsz_sum, prel_scale=pobjsz[j]/pobjsz_sum;
	for (i=0; i<3; i++)
	{
	  loc_sum[i] += loc[j][i]/rel_scale;
	  ploc_sum[i] += ploc[j][i]/prel_scale;
	}
  }
  loc_sum[0] *= 1./ns;
  loc_sum[1] *= 1./ns;
  loc_sum[2] *= 1./ns;
  ploc_sum[0] *= 1./ns;
  ploc_sum[1] *= 1./ns;
  ploc_sum[2] *= 1./ns;
  printf("Volume                  -> %f\n", vol_sum);
  printf("Center                  -> (%f, %f, %f)\n",
    loc_sum[0], loc_sum[1], loc_sum[2]);
  printf("Scale Measure           -> %f\n", objsz_sum);
  printf("Eigenvalues             -> %f %f %f\n",
    eval_sum[0], eval_sum[1], eval_sum[2]);
  if (eval_sum[0]-eval_sum[1] >= eval_sum[1]-eval_sum[2])
  {
    tfom = 1/sqrt(evec_sum[0][0]*evec_sum[0][0]+
                  evec_sum[0][1]*evec_sum[0][1]+
                  evec_sum[0][2]*evec_sum[0][2]);
    for (j=0; j<3; j++)
      evec_sum[0][j] *= tfom;
    tfom = 0;
    for (j=0; j<3; j++)
      tfom += evec_sum[2][j]*evec_sum[0][j];
    for (j=0; j<3; j++)
      evec_sum[2][j] -= tfom*evec_sum[0][j];
    tfom = 1/sqrt(evec_sum[2][0]*evec_sum[2][0]+
                  evec_sum[2][1]*evec_sum[2][1]+
                  evec_sum[2][2]*evec_sum[2][2]);
    for (j=0; j<3; j++)
      evec_sum[2][j] *= tfom;
  }
  else
  {
    tfom = 1/sqrt(evec_sum[2][0]*evec_sum[2][0]+
                  evec_sum[2][1]*evec_sum[2][1]+
                  evec_sum[2][2]*evec_sum[2][2]);
    for (j=0; j<3; j++)
      evec_sum[2][j] *= tfom;
    tfom = 0;
    for (j=0; j<3; j++)
      tfom += evec_sum[2][j]*evec_sum[0][j];
    for (j=0; j<3; j++)
      evec_sum[0][j] -= tfom*evec_sum[2][j];
    tfom = 1/sqrt(evec_sum[0][0]*evec_sum[0][0]+
                  evec_sum[0][1]*evec_sum[0][1]+
                  evec_sum[0][2]*evec_sum[0][2]);
    for (j=0; j<3; j++)
      evec_sum[0][j] *= tfom;
  }
  for (j=0; j<3; j++)
    evec_sum[1][j] = evec_sum[2][(j+1)%3]*evec_sum[0][(j+2)%3]-
                     evec_sum[2][(j+2)%3]*evec_sum[0][(j+1)%3];
  assert(Det3(evec_sum) > 0);
  printf("Eigenvectors            -> (%f, %f, %f)\n",
    evec_sum[0][0], evec_sum[0][1], evec_sum[0][2]);
  printf("                           (%f, %f, %f)\n",
    evec_sum[1][0], evec_sum[1][1], evec_sum[1][2]);
  printf("                           (%f, %f, %f)\n",
    evec_sum[2][0], evec_sum[2][1], evec_sum[2][2]);
  if (rel_flag)
  {
    double inv_pevec[3][3];
    if (peval_sum[0]-peval_sum[1] >= peval_sum[1]-peval_sum[2])
    {
      tfom = 1/sqrt(pevec_sum[0][0]*pevec_sum[0][0]+
                    pevec_sum[0][1]*pevec_sum[0][1]+
                    pevec_sum[0][2]*pevec_sum[0][2]);
      for (j=0; j<3; j++)
        pevec_sum[0][j] *= tfom;
      tfom = 0;
      for (j=0; j<3; j++)
        tfom += pevec_sum[2][j]*pevec_sum[0][j];
      for (j=0; j<3; j++)
        pevec_sum[2][j] -= tfom*pevec_sum[0][j];
      tfom = 1/sqrt(pevec_sum[2][0]*pevec_sum[2][0]+
                    pevec_sum[2][1]*pevec_sum[2][1]+
                    pevec_sum[2][2]*pevec_sum[2][2]);
      for (j=0; j<3; j++)
        pevec_sum[2][j] *= tfom;
    }
    else
    {
      tfom = 1/sqrt(pevec_sum[2][0]*pevec_sum[2][0]+
                    pevec_sum[2][1]*pevec_sum[2][1]+
                    pevec_sum[2][2]*pevec_sum[2][2]);
      for (j=0; j<3; j++)
        pevec_sum[2][j] *= tfom;
      tfom = 0;
      for (j=0; j<3; j++)
        tfom += pevec_sum[2][j]*pevec_sum[0][j];
      for (j=0; j<3; j++)
        pevec_sum[0][j] -= tfom*pevec_sum[2][j];
      tfom = 1/sqrt(pevec_sum[0][0]*pevec_sum[0][0]+
                    pevec_sum[0][1]*pevec_sum[0][1]+
                    pevec_sum[0][2]*pevec_sum[0][2]);
      for (j=0; j<3; j++)
        pevec_sum[0][j] *= tfom;
    }
    for (j=0; j<3; j++)
      pevec_sum[1][j] = pevec_sum[2][(j+1)%3]*pevec_sum[0][(j+2)%3]-
                        pevec_sum[2][(j+2)%3]*pevec_sum[0][(j+1)%3];
    assert(Det3(pevec_sum) > 0);
	transpose(inv_pevec, pevec_sum);
	matrix_multiply(rel_evec_sum, inv_pevec, evec_sum);
    printf("Relative Center         -> (%f, %f, %f)\n",
	  loc_sum[0]-ploc_sum[0], loc_sum[1]-ploc_sum[1], loc_sum[2]-ploc_sum[2]);
    printf("Relative Eigenvectors   -> (%f, %f, %f)\n",
      rel_evec_sum[0][0], rel_evec_sum[0][1], rel_evec_sum[0][2]);
    printf("                           (%f, %f, %f)\n",
      rel_evec_sum[1][0], rel_evec_sum[1][1], rel_evec_sum[1][2]);
    printf("                           (%f, %f, %f)\n",
      rel_evec_sum[2][0], rel_evec_sum[2][1], rel_evec_sum[2][2]);
  }
  memset(ang_sumsqr, 0, sizeof(ang_sumsqr));
  for (j=0; j<ns; j++)
  {
    int k;
    double rel_evec[3][3], inv_pevec[3][3], a;
	if (rel_flag)
	{
	  transpose(inv_pevec, pevec[j]);
	  matrix_multiply(rel_evec, inv_pevec, evec[j]);
    }
	for (k=0; k<3; k++)
    {
	  a = acos(rel_flag?
	      rel_evec[k][0]*rel_evec_sum[k][0]+
          rel_evec[k][1]*rel_evec_sum[k][1]+
          rel_evec[k][2]*rel_evec_sum[k][2]
		: evec[j][k][0]*evec_sum[k][0]+
          evec[j][k][1]*evec_sum[k][1]+
          evec[j][k][2]*evec_sum[k][2]);
      ang_sumsqr[k] += a*a;
    }
  }
  if (ns > 1)
    printf("Eigenvector Deviation   -> %f %f %f\n",
      180/M_PI*sqrt(ang_sumsqr[0]/(ns-1)),
      180/M_PI*sqrt(ang_sumsqr[1]/(ns-1)),
      180/M_PI*sqrt(ang_sumsqr[2]/(ns-1)));
  exit(0);
}



/************************************************************************
 *
 *      FUNCTION        : Det3
 *
 *      DESCRIPTION     : returns the determinant of a 3x3 martrix.
 *
 *      RETURN VALUE    : determinant of the matrix.
 *
 *      PARAMETERS      : m - input matrix
 *
 *      SIDE EFFECTS    : None.
 *
 *      ENTRY CONDITION : m should be initialized.
 *
 *      EXIT CONDITIONS : None.
 *
 *      RELATED FUNCS   : None.
 *
 *      History         : 08/10/1993 Supun Samarasekera
 *
 ************************************************************************/
double Det3(double m[3][3])
{
  double result;
  
  result = 
    m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] +
      m[0][2]*m[1][0]*m[2][1] - m[0][2]*m[1][1]*m[2][0] -
	m[0][0]*m[1][2]*m[2][1] - m[0][1]*m[1][0]*m[2][2];
  return(result);
}

