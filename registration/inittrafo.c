/*
  Copyright 1993-2015 Medical Image Processing Group
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
#include <stdlib.h>
#include <string.h>

#define BEST_PI 3.1415926535897932384626433832795028841971693993751

void GtEval( double mat[3][3], double eval[3] );
int GtEvec(double mat[3][3], double eval[3], double evec[3][3]);

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

/* Modified: 6/6/03 Don't know what this is supposed to do, but surely not
      what it was doing.  by Dewey Odhner */
/* Modified: 1/23/04 Taking another stab at it.  by Dewey Odhner */
void
create_transformation(ox1, oy1, oz1, ox2, oy2, oz2, ox, oy, oz, V1, V2, T)
  double ox1, oy1, oz1, ox2, oy2, oz2, ox, oy, oz;
  double V1[3][3], V2[3][3], T[4][4];
{
  double m1[4][4], m2[4][4], m3[4][4];
  int i, j;

  m1[0][3] = ox1 - ox;
  m1[1][3] = oy1 - oy;
  m1[2][3] = oz1 - oz;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m1[i][j] = V1[i][j];
  for (j = 0; j < 4; j++)
    m1[3][j] = j==3;

  VInvertMatrix(m3, m1, 4);

  m2[0][3] = ox2 - ox;
  m2[1][3] = oy2 - oy;
  m2[2][3] = oz2 - oz;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m2[i][j] = V2[i][j];
  for (j = 0; j < 4; j++)
    m2[3][j] = j==3;

  MultMat(T, m2, m3);
}
/*@ Was:
void
create_transformation(ox1, oy1, oz1, ox2, oy2, oz2, ox, oy, oz, V1, V2, T)
  double ox1, oy1, oz1, ox2, oy2, oz2, ox, oy, oz;
  double V1[3][3], V2[3][3], T[4][4];
{
  double m1[4][4], m2[4][4], m3[4][4], m4[4][4];
  int i, j;

  m4[0][0] = ox1 - ox;
  m4[1][0] = oy1 - oy;
  m4[2][0] = oz1 - oz;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m4[i][j + 1] = m4[i][0] + V1[i][j];
  for (j = 0; j < 4; j++)
    m4[3][j] = 1;

  VInvertMatrix(m3, m4, 4);
  for (i = 0; i < 4; i++)
  	for (j = 0; j < 4; j++)
	  m1[3][j] = m3[3][j];

  m2[0][0] = ox2 - ox;
  m2[1][0] = oy2 - oy;
  m2[2][0] = oz2 - oz;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m2[i][j + 1] = m2[i][0] + V2[i][j];
  for (j = 0; j < 4; j++)
    m2[3][j] = 1;

  MultMat(T, m2, m1);
}
*/

void
center_of_mass(in1, xdim, ydim, zdim, slice_dist, thr, mode, xctr, yctr, zctr)
  unsigned short *in1;
  int xdim, ydim, zdim;
  double slice_dist;
  unsigned short thr;
  int mode;
  double *xctr, *yctr, *zctr;
{
  int i, j, k, iii, jjj, kkk;
  int slice_size;
  double *x_sum, *y_sum, *z_sum;
  double x_cntr, y_cntr, z_cntr;
  double sum;
  double dval;

  slice_size = xdim * ydim;
  x_sum = (double *)malloc(xdim * sizeof(double));
  memset(x_sum, 0, xdim * sizeof(double));
  y_sum = (double *)malloc(ydim * sizeof(double));
  memset(y_sum, 0, ydim * sizeof(double));
  z_sum = (double *)malloc(zdim * sizeof(double));
  memset(z_sum, 0, zdim * sizeof(double));
  sum = 0.0;
  switch (mode % 10) {
    case 1:
      dval = 1.0;
      for (k = 0, kkk = 0; k < zdim; k++, kkk += slice_size)
      {
        for (j = 0, jjj = kkk + (ydim - 1) * xdim; j < ydim; j++, jjj -= xdim)
        {
          for (i = 0, iii = jjj; i < xdim; i++, iii++)
          {
            if (in1[iii] > thr)
            {
              x_sum[i] += dval;
              y_sum[j] += dval;
              z_sum[k] += dval;
              sum += dval;
            }
          }
        }
      }
      break;

    case 2:
      for (k = 0, kkk = 0; k < zdim; k++, kkk += slice_size)
      {
        for (j = 0, jjj = kkk + (ydim - 1) * xdim; j < ydim; j++, jjj -= xdim)
        {
          for (i = 0, iii = jjj; i < xdim; i++, iii++)
          {
            if (in1[iii] > thr)
            {
              dval = (double)in1[iii];
              x_sum[i] += dval;
              y_sum[j] += dval;
              z_sum[k] += dval;
              sum += dval;
            }
          }
        }
      }
      break;
  }
  x_cntr = 0.0;
  for (i = 0; i < xdim; i++)
    x_cntr += i * x_sum[i];
  y_cntr = 0.0;
  for (j = 0; j < ydim; j++)
    y_cntr += j * y_sum[j];
  z_cntr = 0.0;
  for (k = 0; k < zdim; k++)
    z_cntr += k * z_sum[k];
  *xctr = x_cntr / sum;
  *yctr = y_cntr / sum;
  *zctr = z_cntr / sum * slice_dist;
  free(x_sum);
  free(y_sum);
  free(z_sum);
}

void
inertia_matrix_bin(in1, xdim, ydim, zdim, slice_dist, thr, xctr, yctr, zctr, m)
  unsigned short *in1;
  int xdim, ydim, zdim;
  double slice_dist;
  double xctr, yctr, zctr;
  double m[3][3];
{
  int i, j, k, iii, jjj, kkk;
  int slice_size;
  double sum00, sum01, sum02, sum11, sum12, sum22;
  double di, dj, dk;

  slice_size = xdim * ydim;
  sum00 = sum01 = sum02 = sum11 = sum12 = sum22 = 0.0;
  for (k = 0, kkk = 0; k < zdim; k++, kkk += slice_size)
  {
    dk = k * slice_dist - zctr;
    for (j = 0, jjj = kkk + (ydim - 1) * xdim; j < ydim; j++, jjj -= xdim)
    {
      dj = j - yctr;
      for (i = 0, iii = jjj; i < xdim; i++, iii++)
      {
        di = i - xctr;
        if (in1[iii] > thr)
        {
          sum00 += di * di;
          sum01 += di * dj;
          sum02 += di * dk;
          sum11 += dj * dj;
          sum12 += dj * dk;
          sum22 += dk * dk;
        }
      }
    }
  }
  m[0][0] = sum00;
  m[0][1] = m[1][0] = sum01;
  m[0][2] = m[2][0] = sum02;
  m[1][1] = sum11;
  m[1][2] = m[2][1] = sum12;
  m[2][2] = sum22;
}

void
inertia_matrix(in1, xdim, ydim, zdim, slice_dist, thr, xctr, yctr, zctr, m)
  unsigned short *in1;
  int xdim, ydim, zdim;
  double slice_dist;
  double xctr, yctr, zctr;
  double m[3][3];
{
  int i, j, k, iii, jjj, kkk;
  int slice_size;
  double sum00, sum01, sum02, sum11, sum12, sum22;
  double di, dj, dk;
  double v1, v2, v3, d;

  slice_size = xdim * ydim;
  sum00 = sum01 = sum02 = sum11 = sum12 = sum22 = 0.0;
  for (k = 0, kkk = 0; k < zdim; k++, kkk += slice_size)
  {
    dk = k * slice_dist - zctr;
    for (j = 0, jjj = kkk + (ydim - 1) * xdim; j < ydim; j++, jjj -= xdim)
    {
      dj = j - yctr;
      for (i = 0, iii = jjj; i < xdim; i++, iii++)
      {
        di = i - xctr;
        d = in1[iii];
        if (d > thr)
        {
          v1 = di * d;
          v2 = dj * d;
          v3 = dk * d;
          sum00 += di * v1;
          sum01 += di * v2;
          sum02 += di * v3;
          sum11 += dj * v2;
          sum12 += dj * v3;
          sum22 += dk * v3;
        }
      }
    }
  }
  m[0][0] = sum00;
  m[0][1] = m[1][0] = sum01;
  m[0][2] = m[2][0] = sum02;
  m[1][1] = sum11;
  m[1][2] = m[2][1] = sum12;
  m[2][2] = sum22;
}

SortEval(e)
double e[3];
{
  int i,j;
  double temp;

  for(i=0;i<2;i++)
    for(j=i+1;j<3;j++)
      if (e[i]<e[j]) {
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

CrossP(vec1,vec2,res)
     double vec1[],vec2[],res[];
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

/* Modified: 3/18/02 Numerical Recipes function calls replaced with functions
 *    from auto_register.c by Dewey Odhner. */
void
principal_axis(in1, xdim, ydim, zdim, slice_dist, thr, mode, xctr, yctr, zctr, evec, d)
  unsigned short *in1;
  int xdim, ydim, zdim;
  double slice_dist;
  unsigned short thr;
  int mode;
  double xctr, yctr, zctr;
  double evec[3][3];
  double *d;
{
  double mat[3][3];
  int j, k;

  switch (mode % 10) {
    case 1:
      inertia_matrix_bin(in1, xdim, ydim, zdim, slice_dist, thr, xctr, yctr, zctr, mat);
      break;
    case 2:
      inertia_matrix(in1, xdim, ydim, zdim, slice_dist, thr, xctr, yctr, zctr, mat);
      break;
  }

  GtEval(mat, d);
  SortEval(d); /* Sort and get primary secodary and ternary axis */
  GtEvec(mat, d, evec);
}

void
get_initial_transformation(in1, in2, xdim, ydim, zdim, slice_dist, thr1, thr2, mode, p)
  unsigned short *in1, *in2;
  int xdim, ydim, zdim;
  double slice_dist;
  unsigned short thr1, thr2;
  int mode;
  double p[6];
{
  double xctr1, yctr1, zctr1, xctr2, yctr2, zctr2;
  double cy, xx, yy, zz;
  double V1[3][3], V2[3][3], V3[3][3], R[4][4], D1[3], D2[3];
  double U1[3], U2[3];
  double det1, det2;
  double a, c, d, a_min, c_max;
  int i, j;
  int ind[3];

  for (i = 0; i < 6; i++)
    p[i] = 0.0;
  if (((mode / 10) % 10) == 0 || (mode % 10) == 0)
  {
    return;
  }

  center_of_mass(in1, xdim, ydim, zdim, slice_dist, thr1, mode, &xctr1, &yctr1, &zctr1);
printf("   1st image: ( %g, %g, %g ) \n", xctr1, yctr1, zctr1 );
  center_of_mass(in2, xdim, ydim, zdim, slice_dist, thr2, mode, &xctr2, &yctr2, &zctr2);
printf("   2nd image: ( %g, %g, %g ) \n", xctr2, yctr2, zctr2 );

  p[0] = xctr2 - xctr1;
  p[1] = yctr2 - yctr1;
  p[5] = zctr2 - zctr1;
  if (((mode / 10) % 10) == 1)
  {
    return;
  }

  principal_axis(in1, xdim, ydim, zdim, slice_dist, thr1, mode, xctr1, yctr1, zctr1, V1, D1);
  principal_axis(in2, xdim, ydim, zdim, slice_dist, thr2, mode, xctr2, yctr2, zctr2, V2, D2);

printf("eigenvalues 1: ");
for(j=0;j<3;j++)
printf(" %f ",D1[j]);
printf("\n");
printf("eigenvalues 2: ");
for(j=0;j<3;j++)
printf(" %f ",D2[j]);
printf("\n");

  det1 = (V1[1][0] * V1[2][1] - V1[2][0] * V1[1][1]) * V1[0][2] + 
         (V1[2][0] * V1[0][1] - V1[2][1] * V1[0][0]) * V1[1][2] + 
         (V1[0][0] * V1[1][1] - V1[0][1] * V1[1][0]) * V1[2][2];
printf("det1: %lf\n", det1);
  det2 = (V2[1][0] * V2[2][1] - V2[2][0] * V2[1][1]) * V2[0][2] + 
         (V2[2][0] * V2[0][1] - V2[2][1] * V2[0][0]) * V2[1][2] + 
         (V2[0][0] * V2[1][1] - V2[0][1] * V2[1][0]) * V2[2][2];
printf("det2: %lf\n", det2);
  if (det1 < 0.0)
  {
    for (i = 0; i <= 2; i++)
    {
      V1[i][2] = -V1[i][2];
    }
  }
  if (det2 < 0.0)
  {
    for (i = 0; i <= 2; i++)
    {
      V2[i][2] = -V2[i][2];
    }
  }
  printf("V1:\n");
  for (i = 0; i <= 2; i++)
  {
    for (j = 0; j <= 2; j++)
    {
      printf("%12.6lf ", V1[i][j]);
    }
    printf("\n");
  }

  printf("V2:\n");
  for (i = 0; i <= 2; i++)
  {
    for (j = 0; j <= 2; j++)
    {
      printf("%12.6lf ", V2[i][j]);
    }
    printf("\n");
  }

  a_min = 1000000000.0;
  c_max = 3.0 * cos(M_PI / 8.0);
  for (ind[0] = 0; ind[0] < 2; ind[0]++)
  {
    for (ind[1] = 0; ind[1] < 2; ind[1]++)
    {
      for (ind[2] = 0; ind[2] < 2; ind[2]++)
      {
        if (! ((ind[0] + ind[1] + ind[2]) % 2))
        {
          for (i = 0; i < 3; i++)
          {
            for (j = 0; j < 3; j++)
            {
              V3[i][j] = ind[j] ? -V1[i][j] : V1[i][j];
            }
          }
  for (i = 0; i <= 2; i++)
  {
    U1[i] = 0.0;
    U2[i] = 0.0;
    for (j = 0; j <= 2; j++)
    {
      U1[i] += V3[i][j];
      U2[i] += V2[i][j];
    }
  }
  c = 0.0;
  for (i = 0; i <= 2; i++)
  {
    c += U1[i] * U2[i];
  }
  {
    printf("U1           U2\n");
    for (i = 0; i <= 2; i++)
    {
      printf("%12.6lf ", U1[i]);
      printf("%12.6lf ", U2[i]);
      printf("\n");
    }
  }
  if (c >= c_max)
  {
printf("c=%f\n", c);
    c_max = c;
  a = 0.0;
          create_transformation(xctr1, yctr1, zctr1, xctr2, yctr2, zctr2, (double)(xdim - 1) / 2.0, (double)(ydim - 1) / 2.0, (double)(zdim - 1) / 2.0 * slice_dist, V3, V2, R);
          printf("Matrix %d %d %d:\n", ind[0], ind[1], ind[2]);
          for (i = 0; i < 4; i++)
          {
            for (j = 0; j < 4; j++)
            {
              printf("%12.6lf ", R[i][j]);
            }
            printf("\n");
          }
  for (i = 0; i <= 2; i++)
  {
    a += R[i][3] * R[i][3];
  }
  if (a < a_min)
  {
printf("a=%f\n", a);
    a_min = a;
    cy = 1.0 - R[0][2] * R[0][2];
    if (cy > 0.0 && (cy=sqrt(cy)) > 0.0)
    {
/* might there be a sign change in beta? */
      p[3] = -asin(R[1][2] / cy) * 180.0 / M_PI;
      p[4] = asin(R[0][2]) * 180.0 / M_PI;
      p[2] = -asin(R[0][1] / cy) * 180.0 / M_PI;
    }
    else
    {
/* to be handled */
      p[3] = 3 * M_PI * 180.0 / M_PI;
      p[4] = asin(R[0][2]) * 180.0 / M_PI;
      p[2] = 3 * M_PI * 180.0 / M_PI;
    }
    p[0] = R[0][3];
    p[1] = R[1][3];
    p[5] = R[2][3];
  printf("Translation parameters:\n");
  printf("%12.6lf ", p[0]);
  printf("%12.6lf ", p[1]);
  printf("%12.6lf ", p[5]);
  printf("\n");
  printf("Rotation parameters:\n");
  printf("%12.6lf ", p[3]);
  printf("%12.6lf ", p[4]);
  printf("%12.6lf ", p[2]);
  printf("\n");
  }
  }
        }
      }
    }
  }

  if (det1 < 0.0)
  {
    for (i = 0; i <= 2; i++)
    {
      V1[i][2] = -V1[i][2];
    }
  }
  if (det2 < 0.0)
  {
    for (i = 0; i <= 2; i++)
    {
      V2[i][2] = -V2[i][2];
    }
  }

  if (fabs(D1[1] - D1[2])/(D1[1] + D1[2]) < 0.05)
  {
    for (i = 0; i <= 2; i++)
    {
      double t;
      t = V1[i][1];
      V1[i][1] = V1[i][2];
      V1[i][2] = t;
    }

  det1 = (V1[1][0] * V1[2][1] - V1[2][0] * V1[1][1]) * V1[0][2] + 
         (V1[2][0] * V1[0][1] - V1[2][1] * V1[0][0]) * V1[1][2] + 
         (V1[0][0] * V1[1][1] - V1[0][1] * V1[1][0]) * V1[2][2];
printf("det1: %lf\n", det1);
  det2 = (V2[1][0] * V2[2][1] - V2[2][0] * V2[1][1]) * V2[0][2] + 
         (V2[2][0] * V2[0][1] - V2[2][1] * V2[0][0]) * V2[1][2] + 
         (V2[0][0] * V2[1][1] - V2[0][1] * V2[1][0]) * V2[2][2];
printf("det2: %lf\n", det2);
  if (det1 < 0.0)
  {
    for (i = 0; i <= 2; i++)
    {
      V1[i][2] = -V1[i][2];
    }
  }
  if (det2 < 0.0)
  {
    for (i = 0; i <= 2; i++)
    {
      V2[i][2] = -V2[i][2];
    }
  }
  printf("V1:\n");
  for (i = 0; i <= 2; i++)
  {
    for (j = 0; j <= 2; j++)
    {
      printf("%12.6lf ", V1[i][j]);
    }
    printf("\n");
  }

  printf("V2:\n");
  for (i = 0; i <= 2; i++)
  {
    for (j = 0; j <= 2; j++)
    {
      printf("%12.6lf ", V2[i][j]);
    }
    printf("\n");
  }

  for (ind[0] = 0; ind[0] < 2; ind[0]++)
  {
    for (ind[1] = 0; ind[1] < 2; ind[1]++)
    {
      for (ind[2] = 0; ind[2] < 2; ind[2]++)
      {
        if (! ((ind[0] + ind[1] + ind[2]) % 2))
        {
          for (i = 0; i < 3; i++)
          {
            for (j = 0; j < 3; j++)
            {
              V3[i][j] = ind[j] ? -V1[i][j] : V1[i][j];
            }
          }
  for (i = 0; i <= 2; i++)
  {
    U1[i] = 0.0;
    U2[i] = 0.0;
    for (j = 0; j <= 2; j++)
    {
      U1[i] += V3[i][j];
      U2[i] += V2[i][j];
    }
  }
  c = 0.0;
  for (i = 0; i <= 2; i++)
  {
    c += U1[i] * U2[i];
  }
  {
    printf("U1           U2\n");
    for (i = 0; i <= 2; i++)
    {
      printf("%12.6lf ", U1[i]);
      printf("%12.6lf ", U2[i]);
      printf("\n");
    }
  }
/**/  if (c >= c_max)/**/
  {
printf("c=%f\n", c);
    c_max = c;
  a = 0.0;
          create_transformation(xctr1, yctr1, zctr1, xctr2, yctr2, zctr2, (double)(xdim - 1) / 2.0, (double)(ydim - 1) / 2.0, (double)(zdim - 1) / 2.0 * slice_dist, V3, V2, R);
          printf("Matrix %d %d %d:\n", ind[0], ind[1], ind[2]);
          for (i = 0; i < 4; i++)
          {
            for (j = 0; j < 4; j++)
            {
              printf("%12.6lf ", R[i][j]);
            }
            printf("\n");
          }
  for (i = 0; i <= 2; i++)
  {
    a += R[i][3] * R[i][3];
  }
  if (a < a_min)
  {
printf("a=%f\n", a);
    a_min = a;
    cy = 1.0 - R[0][2] * R[0][2];
    if (cy > 0.0 && (cy=sqrt(cy)) > 0.0)
    {
/* might there be a sign change in beta? */
      p[3] = -asin(R[1][2] / cy) * 180.0 / M_PI;
      p[4] = asin(R[0][2]) * 180.0 / M_PI;
      p[2] = -asin(R[0][1] / cy) * 180.0 / M_PI;
    }
    else
    {
/* to be handled */
      p[3] = 3 * M_PI * 180.0 / M_PI;
      p[4] = asin(R[0][2]) * 180.0 / M_PI;
      p[2] = 3 * M_PI * 180.0 / M_PI;
    }
    p[0] = R[0][3];
    p[1] = R[1][3];
    p[2] = R[2][3];
  printf("Translation parameters:\n");
  printf("%12.6lf ", p[0]);
  printf("%12.6lf ", p[1]);
  printf("%12.6lf ", p[5]);
  printf("\n");
  printf("Rotation parameters:\n");
  printf("%12.6lf ", p[3]);
  printf("%12.6lf ", p[4]);
  printf("%12.6lf ", p[2]);
  printf("\n");
  }
  }
        }
      }
    }
  }

  }

  printf("Final translation parameters:\n");
  printf("%12.6lf ", p[0]);
  printf("%12.6lf ", p[1]);
  printf("%12.6lf ", p[5]);
  printf("\n");
  printf("Final rotation parameters:\n");
  printf("%12.6lf ", p[3]);
  printf("%12.6lf ", p[4]);
  printf("%12.6lf ", p[2]);
  printf("\n");

}

