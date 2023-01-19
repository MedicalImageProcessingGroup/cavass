/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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
#include <string.h>
#include <math.h>
#include <assert.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#else
	#define unlink(fn) _unlink(fn)
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971694
#endif

void GtEval( double mat[3][3], double eval[3] );
int GtEvec(double mat[3][3], double eval[3], double evec[3][3]);
int CrossP(double vec1[3], double vec2[3], double res[3]);
void SortEval(double e[3] );

int main(argc,argv)
int argc;
char *argv[];
{
	double (*loc)[3];
	double *objsz;
	double parsz_sum=0, subsz_sum=0;
	double loc_sum[3];
	double (*rel_loc)[3];
	int j, scale_method=1;
	FILE *outfp=stdout;
	double eval[3], evec[3][3], m[3][3];
	double dist_sum=0, dist_sum_sqr=0;

	if (argc>4 && strcmp(argv[argc-2], "-o") == 0)
	{
		outfp = fopen(argv[argc-1], "w");
		argc -= 2;
	}
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
	if (argc<3 || argc%2==0) {
		fprintf(stderr, "Usage: relpos <parent BIM_file> ... <sub BIM_file> ... [-m<n>] [-o <out file>]\n");
		exit(-1);
	}
	loc = (double (*)[3])malloc((argc-1)*sizeof(double)*3);
	objsz = (double *)malloc((argc-1)*sizeof(double));
	rel_loc = (double (*)[3])malloc((argc-1)/2*sizeof(double)*3);
	for (j=0; j<argc-1; j++)
	{
		FILE *tfp;
		char *cmd=(char *)malloc(strlen(argv[j+1])+30);
		char line[80];

		sprintf(cmd, "bin_volume %s -l%d > FOM-TMP-INFO", argv[j+1],
		    scale_method);
		system(cmd);
		free(cmd);
		tfp = fopen("FOM-TMP-INFO", "rb");
		fgets(line, 80, tfp);
		fgets(line, 80, tfp);
		fscanf(tfp, "Volume                  -> %*f\n");
		fscanf(tfp, "Center                  -> (%lf, %lf, %lf)\n",
			&loc[j][0], &loc[j][1], &loc[j][2]);
		fscanf(tfp, "Scale Measure           -> %lf\n", objsz+j);
		fclose(tfp);
	}
	unlink("FOM-TMP-INFO");
	for (j=0; j<(argc-1)/2; j++)
	{
		parsz_sum += objsz[j];
		subsz_sum += objsz[(argc-1)/2+j];
	}
	parsz_sum /= (argc-1)/2;
	subsz_sum /= (argc-1)/2;
	memset(loc_sum, 0, sizeof(loc_sum));
	for (j=0; j<(argc-1)/2; j++)
	{
		int ii;

		for (ii=0; ii<3; ii++)
		{
			rel_loc[j][ii] =
				(loc[(argc-1)/2+j][ii]-loc[j][ii])*(parsz_sum/objsz[j]);
			loc_sum[ii] += rel_loc[j][ii];
		}
	}
	for (j=0; j<3; j++)
		loc_sum[j] /= (argc-1)/2;
	for (j=0; j<(argc-1)/2; j++)
	{
		double dist=rel_loc[j][0]*rel_loc[j][0]+
		            rel_loc[j][1]*rel_loc[j][1]+
		            rel_loc[j][2]*rel_loc[j][2];
		double a;

		dist_sum_sqr += dist;
		dist = sqrt(dist);
		dist_sum += dist;
		fprintf(outfp, "%s: (d_lk, theta, phi, psi)\n", argv[(argc+1)/2+j]);
		fprintf(outfp, "  %f\n", dist);
		a = 180/M_PI*atan2(
			sqrt(rel_loc[j][1]*rel_loc[j][1]+rel_loc[j][2]*rel_loc[j][2]),
			rel_loc[j][0]);
		fprintf(outfp, "  %f\n", a);
		a = 180/M_PI*atan2(
			sqrt(rel_loc[j][2]*rel_loc[j][2]+rel_loc[j][0]*rel_loc[j][0]),
			rel_loc[j][1]);
		fprintf(outfp, "  %f\n", a);
		a = 180/M_PI*atan2(
			sqrt(rel_loc[j][0]*rel_loc[j][0]+rel_loc[j][1]*rel_loc[j][1]),
			rel_loc[j][2]);
		if (a < 0)
			a += 180;
		fprintf(outfp, "  %f\n", a);
	}
	memset(m, 0, sizeof(m));
	for (j=0; j<(argc-1)/2; j++)
	{
		int ii, jj;

		for (ii=0; ii<3; ii++)
			for (jj=0; jj<3; jj++)
				m[ii][jj] += (rel_loc[j][ii]-loc_sum[ii])*
				             (rel_loc[j][jj]-loc_sum[jj])/((argc-1)/2);
	}
	dist_sum = dist_sum/((argc-1)/2);
	fprintf(outfp, "Mean distance           -> %f\n", dist_sum);
	dist_sum_sqr = dist_sum_sqr/((argc-1)/2);
	fprintf(outfp, "Distance std. dev.      -> %f\n",
		sqrt(dist_sum_sqr-dist_sum*dist_sum));
	GtEval(m,eval);
	SortEval(eval);
	GtEvec(m,eval,evec);
    fprintf(outfp, "Center                  -> (%f, %f, %f)\n",
		loc_sum[0], loc_sum[1], loc_sum[2]);
    fprintf(outfp, "Eigenvalue square roots -> {%f, %f, %f}\n",
        sqrt(eval[0]), sqrt(eval[1]), sqrt(eval[2]));
    fprintf(outfp, "Eigenvector 1           -> (%f, %f, %f)\n",
	    evec[0][0], evec[0][1], evec[0][2]);
    fprintf(outfp, "Eigenvector 2           -> (%f, %f, %f)\n",
	    evec[1][0], evec[1][1], evec[1][2]);
    fprintf(outfp, "Eigenvector 3           -> (%f, %f, %f)\n",
	    evec[2][0], evec[2][1], evec[2][2]);
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
double Det3(m)
     double m[3][3];
{
  double result;
  
  result = 
    m[0][0]*m[1][1]*m[2][2] + m[0][1]*m[1][2]*m[2][0] +
      m[0][2]*m[1][0]*m[2][1] - m[0][2]*m[1][1]*m[2][0] -
	m[0][0]*m[1][2]*m[2][1] - m[0][1]*m[1][0]*m[2][2];
  return(result);
}

void SortEval(double e[3] )
{
  int i,j;
  double temp;

  for(i=0;i<2;i++)
    for(j=i+1;j<3;j++)
      if (e[i]<e[j])
	  {
	temp=e[j]; e[j]=e[i]; e[i]=temp;
      }
}

void GtEval( double mat[3][3],double eval[3] )
{       
  double  p,q,r,a,b,m,theta,delta;
  
  p = -mat[0][0]-mat[1][1]-mat[2][2];
  q  = -mat[0][1]*mat[1][0]-mat[0][2]*mat[2][0]-mat[1][2]*mat[2][1]
                +mat[0][0]*mat[1][1]+mat[0][0]*mat[2][2]+mat[1][1]*mat[2][2];
  r = -Det3(mat);

  a= q - p*p/3.0; b= p*p*p/13.5 - p*q/3.0 + r;
  m= 2.0*sqrt(a/-3.0);
  delta=b*b/4.0+a*a*a/27.0;
  if (delta >0.0 || a*m==0)
  {
	theta = M_PI/3.0;
	m = fabs(b);
  }
  else
    theta=(acos((3.0*b)/(a*m)))/3.0;
  eval[0]= p/-3.0 + m*cos(theta);
  eval[1]= p/-3.0 + m*cos(theta+ 2.0*M_PI/3.0);
  eval[2]= p/-3.0 + m*cos(theta+ 4.0*M_PI/3.0);

} 

/*------       Computation of the Eigenvectors of a 3 by 3 matrix       ------*/
int GtEvec(double mat[3][3], double eval[3], double evec[3][3])
{ 
  int     i,j, vn[3], cm1, cm2;
  double  vec0[3],vec1[3],vec2[3],outvec[3], mm[3];

  if (eval[0] == eval[2])
  {
    for (j=0; j<3; j++)
      for (i=0; i<3; i++)
        evec[j][i] = i==j;
    return(0);
  }

  if (eval[1]-eval[2] > eval[0]-eval[1])
  {
    vn[0] = 2;
	vn[1] = 0;
	vn[2] = 1;
  }
  else
  {
    vn[0] = 0;
	vn[1] = 1;
	vn[2] = 2;
  }
  for (i=0; i<3; i++)
  {
    vec0[i] = (i==0) ? mat[0][i]-eval[vn[0]] : mat[0][i];
    vec1[i] = (i==1) ? mat[1][i]-eval[vn[0]] : mat[1][i];
	vec2[i] = (i==2) ? mat[2][i]-eval[vn[0]] : mat[2][i];
  }
  mm[0] = vec0[0]*vec0[0]+vec0[1]*vec0[1]+vec0[2]*vec0[2];
  mm[1] = vec1[0]*vec1[0]+vec1[1]*vec1[1]+vec1[2]*vec1[2];
  mm[2] = vec2[0]*vec2[0]+vec2[1]*vec2[1]+vec2[2]*vec2[2];
  cm1 = mm[0]<mm[1]? 0:1;
  if (mm[2] < mm[cm1])
    cm1 = 2;
  switch (cm1)
  {
    case 0:
	  break;
	case 1:
	  memcpy(vec1, vec0, sizeof(vec0));
	  mm[1] = mm[0];
	  break;
	case 2:
	  memcpy(vec2, vec0, sizeof(vec0));
	  mm[2] = mm[0];
	  break;
  }
  if (CrossP(vec1, vec2, outvec) != 0) {
    fprintf(stderr, "Cannot compute Eigenvectors\n");
	exit(-1);
  }
  for (i=0; i<3; i++)
    evec[vn[0]][i] = outvec[i];
  // One Eigenvector has been computed.

  if (eval[vn[1]] == eval[vn[2]])
  {
    if (CrossP(outvec, vec2, vec1) != 0) {
	  fprintf(stderr, "Cannot compute Eigenvectors\n");
	  exit(-1);
	}
	for (i=0; i<3; i++)
	  evec[vn[1]][i] = vec1[i];
  }
  else
  {
	int nm1=cm1==0?1:0, nm2=cm1==2?!nm1:2;
    for (i=0; i<3; i++)
	{
	  vec1[i] = (i==nm1) ? mat[nm1][i]-eval[vn[1]] : mat[nm1][i];
	  vec2[i] = (i==nm2) ? mat[nm2][i]-eval[vn[1]] : mat[nm2][i];
	}
	mm[1] = vec1[0]*vec1[0]+vec1[1]*vec1[1]+vec1[2]*vec1[2];
	mm[2] = vec2[0]*vec2[0]+vec2[1]*vec2[1]+vec2[2]*vec2[2];
	cm2 = mm[1]<mm[2]? nm1:nm2;
	if (cm2 == nm2)
	  memcpy(vec2, vec1, sizeof(vec0));
	for (i=0; i<3; i++)
	{
	  vec1[i] = evec[vn[0]][i];
	}
	if (CrossP(vec1, vec2, outvec) != 0) {
	  fprintf(stderr, "Cannot compute Eigenvectors\n");
	  exit(-1);
	}
	for (i=0; i<3; i++)
	  evec[vn[1]][i] = outvec[i];
  }
  // Two Eigenvectors have been computed.

  /* calculate the third Eigenvector as a cross product of the first ones */
  CrossP(evec[vn[0]], evec[vn[1]], outvec);
  for (i=0; i<3; i++)
    evec[vn[2]][i] = outvec[i];
  return(0);
}

int CrossP(double vec1[3], double vec2[3], double res[3])
{     
  double len;
  
  res[0] = vec1[1]*vec2[2]-vec1[2]*vec2[1];
  res[1] = vec1[2]*vec2[0]-vec1[0]*vec2[2];
  res[2] = vec1[0]*vec2[1]-vec1[1]*vec2[0];
  len = sqrt(pow(res[0],2.)+pow(res[1],2.)+pow(res[2],2.));
  if (len <= 0.0)
    return(-1);
  else {
    res[0] = res[0]/len;
    res[1] = res[1]/len;
    res[2] = res[2]/len;
  }
  return(0);
}       
