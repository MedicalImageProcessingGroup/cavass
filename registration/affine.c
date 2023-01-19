/*
  Copyright 1993-2014, 2016-2017 Medical Image Processing Group
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



#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
    #define M_PI 3.14159265358979323846  //oh, bill!
#endif

#include  "Etime.h"
#include  "Viewnix.h"


#define DEFAULT_INTERPOLATION 0
#define DEFAULT_INIT_MODE 0
#define DEFAULT_THRESHOLD 0
#define DEFAULT_SHIFT 0
#define DEFAULT_DEPTH 2

#define LOG_SCALE 10000
#define MAX(a,b) a > b ? a:b
#define MIN(a,b) a < b ? a:b
#define srint(f) (int)((f)>=0? (f)+(float).5: (f)-(float).5)
#define sfloor(f) (int)((f)>=0? f: (f)-1)

typedef double landmark_t[3];

/* Modified: 6/10/09 added declaration to fix warning - by Xiaofen Zheng. */
int VCloseData ( FILE* fp );
int VReadData ( char* data, int size, int items, FILE* fp, int* items_read );
int VReadHeader ( FILE* fp, ViewnixHeader* vh, char group[5], char element[5] );
int VSeekData ( FILE* fp, long offset );
int VWriteData ( char* data, int size, int items, FILE* fp, int* items_written );
int VWriteHeader ( FILE *fp, ViewnixHeader *vh, char group[5], char element[5] );
void MultMat(double out[4][4],double mat1[4][4],double mat2[4][4]);
int VInvertMatrix(double Ainv[], double A[], int N);

int NewUOA( long int n, double *x,  double rhobeg,  double rhoend, int iprint, int maxfun, double (*objective)());
void set_param();


double log_LUT[LOG_SCALE + 1];

double tr[4][4];
double param[6];
double param1[3] = {1,1,1};
double param2[3];

double vector[12];
int vecsize;

int *hist1, *hist2, **hist12;
int count;
int bins1, bins2;
int shift1, shift2;
unsigned short thr1, thr2;
unsigned short max1, max2;
int func_count;
int interpolation;
int init_mode;
int max_depth, cur_depth;
int m_flag;

char group[6], elem[6];

int i, j, bytes1, bytes2, vsize1, vsize2, error;
ViewnixHeader vh1, vh2;
int xdim1, ydim1, zdim1, xdim2, ydim2, zdim2;
double voxelsize_x1, voxelsize_y1, voxelsize_z1;
double voxelsize_x2, voxelsize_y2, voxelsize_z2;
unsigned char *inc1, *inc2;
unsigned short *ptrs1, *ptrs2, *pyrs1, *pyrs2, *pyrstr1;
int xdimp1, ydimp1, zdimp1, xdimp2, ydimp2, zdimp2,ssizep, vsizep;

double best_energy;

double (*cost_func)();

landmark_t *source_landmark, *target_landmark, *trans_landmark;
int num_landmarks;
double scanner_tr[4][4];


static char *command_line(argc, argv)
	int argc;
	char **argv;
{
	int j, k;
	char *cl;

	k = argc;
	for (j=0; j<argc; j++)
		k += (int)strlen(argv[j]);
	cl = (char *)malloc(k);
	if (cl == NULL)
	{	(void)fprintf(stderr, "Memory allocation failure\n");
		exit(1);
	}
	k=0;
	for (j=0; j<argc; j++)
	{	strcpy(cl+k, argv[j]);
		k += (int)strlen(argv[j]);
		cl[k] = ' ';
		k++;
	}
	cl[k-1] = 0;
	return (cl);
}

void set_affine_transform(double ox1, double oy1, double oz1,
    double ox2, double oy2, double oz2, int inv)
{
  double temp[4][4], pyramid_factor,
         pixelsize_x1, pixelsize_y1, pixelsize_x2, pixelsize_y2, ti[4][4];
  static double tt[4][4], to1[4][4], to2[4][4];

  pyramid_factor = pow(2.0, (double)(max_depth-cur_depth));
  pixelsize_x1 = pyramid_factor*voxelsize_x1;
  pixelsize_y1 = pyramid_factor*voxelsize_y1;
  pixelsize_x2 = pyramid_factor*voxelsize_x2;
  pixelsize_y2 = pyramid_factor*voxelsize_y2;

  tt[0][0] = vector[0];
  tt[0][1] = vector[1];
  tt[0][2] = vector[2];
  tt[0][3] = vector[3];
  tt[1][0] = vector[4];
  tt[1][1] = vector[5];
  tt[1][2] = vector[6];
  tt[1][3] = vector[7];
  tt[2][0] = vector[8];
  tt[2][1] = vector[9];
  tt[2][2] = vector[10];
  tt[2][3] = vector[11];
  tt[3][3] = 1.0;

  to1[0][0] = inv ? 1.0 / pixelsize_x1 : pixelsize_x1;
  to1[1][1] = inv ? 1.0 / pixelsize_y1 : pixelsize_y1;
  to1[2][2] = inv ? 1.0 / voxelsize_z1 : voxelsize_z1;
  to1[3][3] = 1.0;
  to1[0][3] = inv ? ox1 : -pixelsize_x1 * ox1;
  to1[1][3] = inv ? oy1 : -pixelsize_y1 * oy1;
  to1[2][3] = inv ? oz1 : -voxelsize_z1 * oz1;

  to2[0][0] = inv ? pixelsize_x2 : 1.0 / pixelsize_x2;
  to2[1][1] = inv ? pixelsize_y2 : 1.0 / pixelsize_y2;
  to2[2][2] = inv ? voxelsize_z2 : 1.0 / voxelsize_z2;
  to2[3][3] = 1.0;
  to2[0][3] = inv ? -pixelsize_x2 * ox2 : ox2;
  to2[1][3] = inv ? -pixelsize_y2 * oy2 : oy2;
  to2[2][3] = inv ? -voxelsize_z2 * oz2 : oz2;

  if (inv)
  {
    VInvertMatrix(ti[0], tt[0], 4);
    MultMat(temp, to1, ti);
    MultMat(tr, temp, to2);
  }
  else
  {
    MultMat(temp, to2, tt);
    MultMat(tr, temp, to1);
  }
}

void set_scanner_transform(double ox1, double oy1, double oz1,
    double ox2, double oy2, double oz2)
{
  double temp[4][4], ti[4][4], tt[4][4], to1[4][4], to2[4][4];
  int j, k;

  tt[0][0] = vector[0];
  tt[0][1] = vector[1];
  tt[0][2] = vector[2];
  tt[0][3] = vector[3];
  tt[1][0] = vector[4];
  tt[1][1] = vector[5];
  tt[1][2] = vector[6];
  tt[1][3] = vector[7];
  tt[2][0] = vector[8];
  tt[2][1] = vector[9];
  tt[2][2] = vector[10];
  tt[2][3] = vector[11];
  tt[3][0] = 0.0;
  tt[3][1] = 0.0;
  tt[3][2] = 0.0;
  tt[3][3] = 1.0;

  memset(to1, 0, sizeof(to1));
  to1[0][0] = 1.0;
  to1[1][1] = -1.0;
  to1[2][2] = 1.0;
  to1[3][3] = 1.0;
  to1[0][3] = -voxelsize_x1 * ox1;
  to1[1][3] = voxelsize_y1 * oy1;
  to1[2][3] = -voxelsize_z1 * oz1 - vh1.scn.loc_of_subscenes[0];

  memset(to2, 0, sizeof(to2));
  to2[0][0] = 1.0;
  to2[1][1] = -1.0;
  to2[2][2] = 1.0;
  to2[3][3] = 1.0;
  to2[0][3] = voxelsize_x2 * ox2;
  to2[1][3] = voxelsize_y2 * oy2;
  to2[2][3] = voxelsize_z2 * oz2 + vh2.scn.loc_of_subscenes[0];

  MultMat(temp, to2, tt);
  MultMat(ti, temp, to1);

  // transform from source scanner system to source scene system

  for (j=0; j<4; j++)
    for (k=0; k<3; k++)
	  tt[j][k] = j==k;
  for (j=0; j<3; j++)
    tt[j][3] = vh1.scn.domain_valid? -vh1.scn.domain[j]: 0;

  for (j=0; j<3; j++)
    for (k=0; k<3; k++)
	  temp[j][k] = vh1.scn.domain_valid? vh1.scn.domain[3+3*j+k]: j==k;
  for (j=0; j<3; j++)
    temp[j][3] = temp[3][j] = 0;
  temp[3][3] = 1;

  MultMat(to1, temp, tt);

  // transform from target scene system to target scanner system

  for (j=0; j<3; j++)
    for (k=0; k<3; k++)
	  to2[j][k] = vh2.scn.domain_valid? vh2.scn.domain[3+3*k+j]: j==k;
  for (j=0; j<3; j++)
    to2[j][3] = vh2.scn.domain_valid? vh2.scn.domain[j]: 0;
  for (k=0; k<4; k++)
    to2[3][k] = k==3;

  MultMat(temp, to2, ti);
  MultMat(scanner_tr, temp, to1);
}

void
make_affine_transform(ox1, oy1, oz1, ox2, oy2, oz2, rx, ry, rz, tx, ty, tz, zx, zy, zz, sxy, sxz, syz, inv)
  double ox1, oy1, oz1, ox2, oy2, oz2, rx, ry, rz, tx, ty, tz, zx, zy, zz, sxy, sxz, syz;
  int inv;
{
  double sign, rax, ray, raz, temp[4][4], pyramid_factor,
         pixelsize_x1, pixelsize_y1, pixelsize_x2, pixelsize_y2;
  static double trx[4][4], try[4][4], trz[4][4], tt[4][4],
                to1[4][4], to2[4][4], tzm[4][4], tsr[4][4];

  sign = inv ? -1.0 : 1.0;
  pyramid_factor = pow(2.0, (double)(max_depth-cur_depth));
  pixelsize_x1 = pyramid_factor*voxelsize_x1;
  pixelsize_y1 = pyramid_factor*voxelsize_y1;
  pixelsize_x2 = pyramid_factor*voxelsize_x2;
  pixelsize_y2 = pyramid_factor*voxelsize_y2;

  rax = rx / 180.0 * M_PI;
  ray = ry / 180.0 * M_PI;
  raz = rz / 180.0 * M_PI;


  tsr[0][0] = 1.0;
  tsr[1][1] = 1.0;
  tsr[2][2] = 1.0;
  tsr[3][3] = 1.0;
  tsr[0][1] = - tan(sxy / 180.0 * M_PI) * sign;
  tsr[0][2] = - tan(sxz / 180.0 * M_PI) * sign;
  tsr[1][2] = - tan(syz / 180.0 * M_PI) * sign;

  tzm[0][0] = inv ? 1.0 / zx : zx;
  tzm[1][1] = inv ? 1.0 / zy : zy;
  tzm[2][2] = inv ? 1.0 / zz : zz;
  tzm[3][3] = 1.0;

  trx[0][0] = 1.0;
  trx[1][1] = cos(rax);
  trx[1][2] = -sin(rax) * sign;
  trx[2][1] = -trx[1][2];
  trx[2][2] = trx[1][1];
  trx[3][3] = 1.0;

  try[0][0] = cos(ray);
  try[0][2] = -sin(ray) * sign;
  try[1][1] = 1.0;
  try[2][0] = -try[0][2];
  try[2][2] = try[0][0];
  try[3][3] = 1.0;

  trz[0][0] = cos(raz);
  trz[0][1] = -sin(raz) * sign;
  trz[1][0] = -trz[0][1];
  trz[1][1] = trz[0][0];
  trz[2][2] = 1.0;
  trz[3][3] = 1.0;

  tt[0][0] = 1.0;
  tt[1][1] = 1.0;
  tt[2][2] = 1.0;
  tt[3][3] = 1.0;
  tt[0][3] = tx * sign;
  tt[1][3] = ty * sign;
  tt[2][3] = tz * sign;

  to1[0][0] = inv ? 1.0 / pixelsize_x1 : pixelsize_x1;
  to1[1][1] = inv ? 1.0 / pixelsize_y1 : pixelsize_y1;
  to1[2][2] = inv ? 1.0 / voxelsize_z1 : voxelsize_z1;
  to1[3][3] = 1.0;
  to1[0][3] = inv ? ox1 : -pixelsize_x1 * ox1;
  to1[1][3] = inv ? oy1 : -pixelsize_y1 * oy1;
  to1[2][3] = inv ? oz1 : -voxelsize_z1 * oz1;

  to2[0][0] = inv ? pixelsize_x2 : 1.0 / pixelsize_x2;
  to2[1][1] = inv ? pixelsize_y2 : 1.0 / pixelsize_y2;
  to2[2][2] = inv ? voxelsize_z2 : 1.0 / voxelsize_z2;
  to2[3][3] = 1.0;
  to2[0][3] = inv ? -pixelsize_x2 * ox2 : ox2;
  to2[1][3] = inv ? -pixelsize_y2 * oy2 : oy2;
  to2[2][3] = inv ? -voxelsize_z2 * oz2 : oz2;

  if (inv)
  {
    MultMat(tr, to1,tsr);
    MultMat(temp, tr, tzm);
    MultMat(tr, temp, trz);
    MultMat(temp, tr, try);
    MultMat(tr, temp, trx);
    MultMat(temp,tr, tt);
    MultMat(tr, temp, to2);
  }
  else
  {
    MultMat(tr, to2,tt);
    MultMat(temp, tr, trx);
    MultMat(tr, temp, try);
    MultMat(temp, tr, trz);
    MultMat(tr, temp, tzm);
    MultMat(temp, tr, tsr);
    MultMat(tr, temp, to1);
  }
}

void
make_scanner_transform(ox1, oy1, oz1, ox2, oy2, oz2, rx, ry, rz, tx, ty, tz, zx, zy, zz, sxy, sxz, syz)
  double ox1, oy1, oz1, ox2, oy2, oz2, rx, ry, rz, tx, ty, tz, zx, zy, zz, sxy, sxz, syz;
{
  double rax, ray, raz, tt[4][4], to1[4][4], to2[4][4], temp[4][4], ti[4][4];
  static double trx[4][4], try[4][4], trz[4][4],
                tzm[4][4], tsr[4][4];
  int j, k;

  rax = rx / 180.0 * M_PI;
  ray = ry / 180.0 * M_PI;
  raz = rz / 180.0 * M_PI;


  tsr[0][0] = 1.0;
  tsr[1][1] = 1.0;
  tsr[2][2] = 1.0;
  tsr[3][3] = 1.0;
  tsr[0][1] = - tan(sxy / 180.0 * M_PI);
  tsr[0][2] = - tan(sxz / 180.0 * M_PI);
  tsr[1][2] = - tan(syz / 180.0 * M_PI);

  tzm[0][0] = zx;
  tzm[1][1] = zy;
  tzm[2][2] = zz;
  tzm[3][3] = 1.0;

  trx[0][0] = 1.0;
  trx[1][1] = cos(rax);
  trx[1][2] = -sin(rax);
  trx[2][1] = -trx[1][2];
  trx[2][2] = trx[1][1];
  trx[3][3] = 1.0;

  try[0][0] = cos(ray);
  try[0][2] = -sin(ray);
  try[1][1] = 1.0;
  try[2][0] = -try[0][2];
  try[2][2] = try[0][0];
  try[3][3] = 1.0;

  trz[0][0] = cos(raz);
  trz[0][1] = -sin(raz);
  trz[1][0] = -trz[0][1];
  trz[1][1] = trz[0][0];
  trz[2][2] = 1.0;
  trz[3][3] = 1.0;

  memset(tt, 0, sizeof(tt));
  tt[0][0] = 1.0;
  tt[1][1] = 1.0;
  tt[2][2] = 1.0;
  tt[3][3] = 1.0;
  tt[0][3] = tx;
  tt[1][3] = ty;
  tt[2][3] = tz;

  memset(to1, 0, sizeof(to1));
  to1[0][0] = 1.0;
  to1[1][1] = -1.0;
  to1[2][2] = 1.0;
  to1[3][3] = 1.0;
  to1[0][3] = -voxelsize_x1 * ox1;
  to1[1][3] = voxelsize_y1 * oy1;
  to1[2][3] = -voxelsize_z1 * oz1 - vh1.scn.loc_of_subscenes[0];

  memset(to2, 0, sizeof(to2));
  to2[0][0] = 1.0;
  to2[1][1] = -1.0;
  to2[2][2] = 1.0;
  to2[3][3] = 1.0;
  to2[0][3] = voxelsize_x2 * ox2;
  to2[1][3] = voxelsize_y2 * oy2;
  to2[2][3] = voxelsize_z2 * oz2 + vh2.scn.loc_of_subscenes[0];

  MultMat(ti, to2,tt);
  MultMat(temp, ti, trx);
  MultMat(ti, temp, try);
  MultMat(temp, ti, trz);
  MultMat(ti, temp, tzm);
  MultMat(temp, ti, tsr);
  MultMat(ti, temp, to1);

  // transform from source scanner system to source scene system

  for (j=0; j<4; j++)
    for (k=0; k<3; k++)
	  tt[j][k] = j==k;
  for (j=0; j<3; j++)
    tt[j][3] = vh1.scn.domain_valid? -vh1.scn.domain[j]: 0;

  for (j=0; j<3; j++)
    for (k=0; k<3; k++)
	  temp[j][k] = vh1.scn.domain_valid? vh1.scn.domain[3+3*j+k]: j==k;
  for (j=0; j<3; j++)
    temp[j][3] = temp[3][j] = 0;
  temp[3][3] = 1;

  MultMat(to1, temp, tt);

  // transform from target scene system to target scanner system

  for (j=0; j<3; j++)
    for (k=0; k<3; k++)
	  to2[j][k] = vh2.scn.domain_valid? vh2.scn.domain[3+3*k+j]: j==k;
  for (j=0; j<3; j++)
    to2[j][3] = vh2.scn.domain_valid? vh2.scn.domain[j]: 0;
  for (k=0; k<4; k++)
    to2[3][k] = k==3;

  MultMat(temp, to2, ti);
  MultMat(scanner_tr, temp, to1);
}

void transform_landmarks()
{
  int j, k;

  for (j=0; j<num_landmarks; j++)
    for (k=0; k<3; k++)
	  trans_landmark[j][k] = scanner_tr[k][0]*source_landmark[j][0]+
	                         scanner_tr[k][1]*source_landmark[j][1]+
							 scanner_tr[k][2]*source_landmark[j][2]+
							 scanner_tr[k][3];
}

void get_euler(double euler[3], double tr[4][4])
{
  double costheta;

  if (tr[2][0]*tr[2][0] < .5)
  {
    euler[0] = atan2(tr[2][1], tr[2][2]);
    euler[1] = acos(fabs(tr[2][1])>fabs(tr[2][2])?
				 tr[2][1]/sin(euler[0]):
				 tr[2][2]/cos(euler[0]));
	costheta = cos(euler[1]);
    euler[2] = atan2(tr[1][0]/costheta, tr[0][0]/costheta);
	return;
  }
  euler[1] = -asin(tr[2][0]);
  costheta = cos(euler[1]);
  if (costheta==0 || (tr[2][1]==0&&tr[2][2]==0) || (tr[1][0]==0&&tr[0][0]==0))
  {
	euler[2] = 0;
	euler[0] = atan2(tr[0][1]/-tr[2][0], tr[0][2]/-tr[2][0]);
	return;
  }
  euler[0] = atan2(tr[2][1]/costheta, tr[2][2]/costheta);
  euler[2] = atan2(tr[1][0]/costheta, tr[0][0]/costheta);
}

void
create_log_LUT()
{
  unsigned int i;
  double f;
  double fstep = 1.0 / (double)LOG_SCALE;

  log_LUT[0] = 0.0;
  for (i = 1, f = fstep; i <= LOG_SCALE; i++, f += fstep)
    log_LUT[i] = log(f);
}

void
get_hist(in1, in2, size)
  unsigned short *in1, *in2;
  int size;
{
  int i;
  unsigned short v1, v2;

  memset(&hist1[0], 0, (bins1 + 1) * sizeof(int));
  memset(&hist2[0], 0, (bins2 + 1) * sizeof(int));
  memset(&hist12[0][0], 0, (bins1 + 1) * (bins2 + 1) * sizeof(int));
  count = 0;
  for (i = 0; i < size; in1++, in2++, i++)
  {
    if (*in1 > thr1 && *in2 > thr2)
    {
      v1 = (*in1) >> shift1;
      v2 = (*in2) >> shift2;
      hist1[v1]++;
      hist2[v2]++;
      hist12[v2][v1]++;
      count++;
    }
  }
}

double
MI()
{
  int i, j;
  int h1, h2, h12;
  double mi, scale;

  get_hist(pyrstr1, pyrs2, vsizep);
  mi = 0.0;
  scale = (double)LOG_SCALE / count;
  for (i = 0; i <= bins1; i++)
    hist1[i] = (int)(hist1[i] * scale);
  for (j = 0; j <= bins2; j++)
    hist2[j] = (int)(hist2[j] * scale);
  for (j = 0; j <= bins2; j++)
    for (i = 0; i <= bins1; i++)
      hist12[j][i] = (int)(hist12[j][i] * scale);
  for (j = 0; j <= bins2; j++)
  {
    h2 = hist2[j];
    if (h2 > 0)
    {
      for (i = 0; i <= bins1; i++)
      {
        h1 = hist1[i];
        if (h1 > 0)
        {
          h12 = hist12[j][i];
          if (h12 > 0)
          {
            mi += h12 * (log_LUT[h1]
                       + log_LUT[h2]
                       - log_LUT[h12]);
          }
        }
      }
    }
  }
  return mi;
}

double
CORR()
{
  int i, j, k, iii, jjj, kkk;
  double sum_a, sum_ab, sum_aa, denom;
  static double sum_b, sum_bb; 

  if (func_count == 1)
    sum_b = sum_bb = 0.0;
  sum_a = sum_ab = sum_aa = 0.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        sum_a += pyrstr1[iii];
		sum_ab += (double)pyrstr1[iii] * pyrs2[iii];
		sum_aa += (double)pyrstr1[iii] * pyrstr1[iii];
		if (func_count == 1)
		{
		  sum_b += pyrs2[iii];
		  sum_bb += (double)pyrs2[iii] * pyrs2[iii];
		}
      }
    }
  }
  denom = (ssizep*zdimp2*sum_aa - sum_a*sum_a)*
          (ssizep*zdimp2*sum_bb - sum_b*sum_b);
  return denom<=0? 0: (ssizep*zdimp2*sum_ab - sum_a*sum_b)/sqrt(denom);
}


double
NMI()
{
  int i, j;
  int h1, h2, h12;
  double nmi, scale;

  get_hist(pyrstr1, pyrs2, vsizep);
  nmi = 0.0;
  scale = (double)LOG_SCALE / count;
  for (i = 0; i <= bins1; i++)
    hist1[i] = (int)(hist1[i] * scale);
  for (j = 0; j <= bins2; j++)
    hist2[j] = (int)(hist2[j] * scale);
  for (j = 0; j <= bins2; j++)
    for (i = 0; i <= bins1; i++)
      hist12[j][i] = (int)(hist12[j][i] * scale);
  for (j = 0; j <= bins2; j++)
  {
    h2 = hist2[j];
    if (h2 > 0)
    {
      for (i = 0; i <= bins1; i++)
      {
        h1 = hist1[i];
        if (h1 > 0)
        {
          h12 = hist12[j][i];
          if (h12 > 0)
          {
            nmi += -(h1 * log_LUT[h1] + h2 * log_LUT[h2]) /
                    (h12 *log_LUT[h12]);

          }
        }
      }
    }
  }
  return nmi;
}

double
KS()
{
  int i, j, k, iii, jjj, kkk;
  double ks, sum1, sum2;

  sum1 = 0.0;
  sum2 = 0.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        if (pyrstr1[iii] >= thr1 || pyrs2[iii] >= thr2)
	      sum1 = sum1 + 1;
	if (pyrstr1[iii] >= thr1 && pyrs2[iii] >= thr2)
	      sum2 = sum2 + 1;
      }
    }
  }
  ks = (double)sum2/sum1;
  return ks;
}

double
FS()
{
  int i, j, k, iii, jjj, kkk;
  double fs, sum1, sum2,max, min;

  fs=0.0;
  sum1 = 1.0;
  sum2 = 1.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        if (pyrstr1[iii] > 0 || pyrs2[iii] > 0)
              max = MAX(pyrstr1[iii],pyrs2[iii]);
	      sum1 = sum1 + max;
	if (pyrstr1[iii] > 0 && pyrs2[iii] > 0)
              min = MIN(pyrstr1[iii],pyrs2[iii]);
	      sum2 = sum2 + min;
      }
    }
  }
  fs = 100*(sum2/sum1);
  return fs;
}

double
SSD()
{
  int i, j, k, iii, jjj, kkk;
  double ssd, sssd, ssssd;

  ssssd = 0.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    sssd = 0.0;
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
	  ssd = 0.0;
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        ssd +=    ((int)pyrstr1[iii] - (int)pyrs2[iii])*
          (double)((int)pyrstr1[iii] - (int)pyrs2[iii]);
      }
	  sssd += ssd;
    }
	ssssd += sssd;
  }
  return ssssd*(1/((double)zdimp2*ssizep));
}

double
SAD()
{
  int i, j, k, iii, jjj, kkk;
  double sad, ssad, sssad;

  sssad = 0.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    ssad = 0.0;
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      sad = 0.0;
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        sad += (double)(pyrs2[iii]>pyrstr1[iii]?
                        pyrs2[iii]-pyrstr1[iii]:
                        pyrstr1[iii]-pyrs2[iii]);
      }
      ssad += sad;
    }
    sssad += ssad;
  }
  return sssad*(1/((double)zdimp2*ssizep));
}

double
SPD() // square Procrustes distance
{
  int i;
  double spd, dx, dy, dz;

  spd = 0.0;
  for (i=0; i<num_landmarks; i++)
  {
    dx = trans_landmark[i][0]-target_landmark[i][0];
	dy = trans_landmark[i][1]-target_landmark[i][1];
	dz = trans_landmark[i][2]-target_landmark[i][2];
    spd += dx*dx+dy*dy+dz*dz;
  }
  return spd;
}

unsigned short
get_max(in, vsize)
  unsigned short *in;
  int vsize;
{
  int i;
  unsigned short mx = 0;

  for (i = 0; i < vsize; in++, i++)
    if (*in > mx)
      mx = *in;
  return mx;
}


/* Modified: 6/15/09 (1) change double type to float (2) add +0.5 to round instead of
cast truncating - by Xiaofen Zheng. */
void
exec_regist(in1, out1, xd1, yd1, zd1, xd2, yd2, zd2,interp)
  unsigned short *in1, *out1;
  int xd1, yd1, zd1, xd2, yd2, zd2;
  int interp;
{
  int i, j, k;
  float di2, dj2, dk2;
  float di3, dj3, dk3;
  float di4, dj4, dk4;
  int i5, j5, k5;
  int i6, j6, k6;
  float t11, t12, t13, t14, t21, t22, t23, t24, t31, t32, t33, t34;
  int iii, jjj, kkk, iii5;
  float wx, wy, wz;
  float wx1, wy1, wz1;
  int slicesize;
  float w, ws, temp[4][4];

  for (j = 0; j < 4; j++)
    for (i = 0; i < 4; i++)
      temp[j][i] = (float)tr[j][i];

  t11 = temp[0][0];
  t12 = temp[0][1];
  t13 = temp[0][2];
  t14 = temp[0][3];
  t21 = temp[1][0];
  t22 = temp[1][1];
  t23 = temp[1][2];
  t24 = temp[1][3];
  t31 = temp[2][0];
  t32 = temp[2][1];
  t33 = temp[2][2];
  t34 = temp[2][3];

  slicesize = xd2 * yd2;
  switch (interp) {
    case 0:
      for (k = 0, kkk = 0, di2 = t14, dj2 = t24, dk2 = t34;
           k < zd2;
           k++, kkk += slicesize, di2 += t13, dj2 += t23, dk2 += t33)
      {
        for (j = 0, jjj = kkk + (yd2 - 1) * xd2, di3 = di2, dj3 = dj2, dk3 = dk2;
             j < yd2;
             j++, jjj -= xd2, di3 += t12, dj3 += t22, dk3 += t32)
        {
          for (i = 0, iii = jjj, di4 = di3, dj4 = dj3, dk4 = dk3;
               i < xd2;
               i++, iii++, di4 += t11, dj4 += t21, dk4 += t31)
          {
            i5 = srint(di4);
            j5 = srint(dj4);
            k5 = srint(dk4);
            if (i5 >= 0 && i5 < xd1 && j5 >= 0 && j5 < yd1 && k5 >= 0 && k5 < zd1)
            {
              iii5 = (k5 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
              out1[iii] = in1[iii5];
            }
            else
              out1[iii] = 0;
          }
        }
      }
      break;

    case 1:
      for (k = 0, kkk = 0, di2 = t14, dj2 = t24, dk2 = t34;
           k < zd2;
           k++, kkk += slicesize, di2 += t13, dj2 += t23, dk2 += t33)
      {
        for (j = 0, jjj = kkk + (yd2 - 1) * xd2, di3 = di2, dj3 = dj2, dk3 = dk2;
             j < yd2;
             j++, jjj -= xd2, di3 += t12, dj3 += t22, dk3 += t32)
        {
          for (i = 0, iii = jjj, di4 = di3, dj4 = dj3, dk4 = dk3;
               i < xd2;
               i++, iii++, di4 += t11, dj4 += t21, dk4 += t31)
          {
            i5 = sfloor(di4);
            j5 = sfloor(dj4);
            k5 = sfloor(dk4);
            i6 = i5 + 1;
            j6 = j5 + 1;
            k6 = k5 + 1;
            ws = 0.0;
            if (i5 >= 0 && i6 < xd1 && j5 >= 0 && j6 < yd1)
            {
			  wx = di4 - i5;
			  wy = dj4 - j5;
              wz = dk4 - k5;
			  wx1 = (float)1.0 - wx;
			  wy1 = (float)1.0 - wy;
              if (k5 >= 0 && k5 < zd1)
              {
                wz1 = (float)1.0 - wz;
                w = wz1*wx1*wy1;
                iii5 = (k5 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
                ws += in1[iii5] * w;
				w = wz1*wx1*wy;
				iii5 = (k5 * yd1 + (yd1 - 1 - j6)) * xd1 + i5;
				ws += in1[iii5] * w;
				w = wz1*wx*wy1;
				iii5 = (k5 * yd1 + (yd1 - 1 - j5)) * xd1 + i6;
				ws += in1[iii5] * w;
				w = wz1*wx*wy;
				iii5 = (k5 * yd1 + (yd1 - 1 - j6)) * xd1 + i6;
				ws += in1[iii5] * w;
              }
              if (k6 >= 0 && k6 < zd1)
              {
                w = wz*wx1*wy1;
                iii5 = (k6 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
                ws += in1[iii5] * w;
				w = wz*wx1*wy;
				iii5 = (k6 * yd1 + (yd1 - 1 - j6)) * xd1 + i5;
				ws += in1[iii5] * w;
				w = wz*wx*wy1;
				iii5 = (k6 * yd1 + (yd1 - 1 - j5)) * xd1 + i6;
				ws += in1[iii5] * w;
				w = wz*wx*wy;
				iii5 = (k6 * yd1 + (yd1 - 1 - j6)) * xd1 + i6;
				ws += in1[iii5] * w;
              }
            }
            out1[iii] = (unsigned short)(ws+0.5);
          }
        }
      }

      break;

    case 2:
      for (k = 0, kkk = 0, di2 = t14, dj2 = t24, dk2 = t34;
           k < zd2;
           k++, kkk += slicesize, di2 += t13, dj2 += t23, dk2 += t33)
      {
        for (j = 0, jjj = kkk + (yd2 - 1) * xd2, di3 = di2, dj3 = dj2, dk3 = dk2;
             j < yd2;
             j++, jjj -= xd2, di3 += t12, dj3 += t22, dk3 += t32)
        {
          for (i = 0, iii = jjj, di4 = di3, dj4 = dj3, dk4 = dk3;
               i < xd2;
               i++, iii++, di4 += t11, dj4 += t21, dk4 += t31)
          {
            i5 = di4 >= 0.0 ? (int)di4 : (int)(di4 - 1.0);
            j5 = dj4 >= 0.0 ? (int)dj4 : (int)(dj4 - 1.0);
            k5 = dk4 >= 0.0 ? (int)dk4 : (int)(dk4 - 1.0);
            wx = di4 - i5;
            wy = dj4 - j5;
            wz = dk4 - k5;
            wx1 = (float)1.0 - wx;
            wy1 = (float)1.0 - wy;
            wz1 = (float)1.0 - wz;
            i6 = i5 + 1;
            j6 = j5 + 1;
            k6 = k5 + 1;
            ws = 0.0;
            if (i5 >= 0 && i5 < xd1 && j5 >= 0 && j5 < yd1 && k5 >= 0 && k5 < zd1)
            {
              w = wx1 * wy1 * wz1;
              iii5 = (k5 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
              ws += in1[iii5] * w;
            }
            if (i6 >= 0 && i6 < xd1 && j5 >= 0 && j5 < yd1 && k5 >= 0 && k5 < zd1)
            {
              w = wx * wy1 * wz1;
              iii5 = (k5 * yd1 + (yd1 - 1 - j5)) * xd1 + i6;
              ws += in1[iii5] * w;
            }
            if (i5 >= 0 && i5 < xd1 && j6 >= 0 && j6 < yd1 && k5 >= 0 && k5 < zd1)
            {
              w = wx1 * wy * wz1;
              iii5 = (k5 * yd1 + (yd1 - 1 - j6)) * xd1 + i5;
              ws += in1[iii5] * w;
            }
            if (i5 >= 0 && i5 < xd1 && j5 >= 0 && j5 < yd1 && k6 >= 0 && k6 < zd1)
            {
              w = wx1 * wy1 * wz;
              iii5 = (k6 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
              ws += in1[iii5] * w;
            }
            if (i6 >= 0 && i6 < xd1 && j6 >= 0 && j6 < yd1 && k5 >= 0 && k5 < zd1)
            {
              w = wx * wy * wz1;
              iii5 = (k5 * yd1 + (yd1 - 1 - j6)) * xd1 + i6;
              ws += in1[iii5] * w;
            }
            if (i5 >= 0 && i5 < xd1 && j6 >= 0 && j6 < yd1 && k6 >= 0 && k6 < zd1)
            {
              w = wx1 * wy * wz;
              iii5 = (k6 * yd1 + (yd1 - 1 - j6)) * xd1 + i5;
              ws += in1[iii5] * w;
            }
            if (i6 >= 0 && i6 < xd1 && j5 >= 0 && j5 < yd1 && k6 >= 0 && k6 < zd1)
            {
              w = wx * wy1 * wz;
              iii5 = (k6 * yd1 + (yd1 - 1 - j5)) * xd1 + i6;
              ws += in1[iii5] * w;
            }
            if (i6 >= 0 && i6 < xd1 && j6 >= 0 && j6 < yd1 && k6 >= 0 && k6 < zd1)
            {
              w = wx * wy * wz;
              iii5 = (k6 * yd1 + (yd1 - 1 - j6)) * xd1 + i6;
              ws += in1[iii5] * w;
            }
            out1[iii] = (unsigned short)(ws+0.5);
          }
        }
      }
      break;
  }
}

void make_transform_and_regist()
{
  func_count++;
  if (m_flag)
    if (num_landmarks)
    {
      set_scanner_transform((xdimp1 + 1) / 2.0, (ydimp1 + 1) / 2.0, (zdimp1 + 1) / 2.0,
                        (xdimp2 + 1) / 2.0, (ydimp2 + 1) / 2.0, (zdimp2 + 1) / 2.0);
      transform_landmarks();
    }
    else
      set_affine_transform((xdimp1 + 1) / 2.0, (ydimp1 + 1) / 2.0, (zdimp1 + 1) / 2.0,
                        (xdimp2 + 1) / 2.0, (ydimp2 + 1) / 2.0, (zdimp2 + 1) / 2.0, 1);
  else
  {
    set_param();
    if (num_landmarks)
    {
      make_scanner_transform((xdimp1 + 1) / 2.0, (ydimp1 + 1) / 2.0, (zdimp1 + 1) / 2.0,
                        (xdimp2 + 1) / 2.0, (ydimp2 + 1) / 2.0, (zdimp2 + 1) / 2.0,
                        param[3], param[4], param[2], param[0], param[1], param[5],
                        param1[0], param1[1], param1[2], param2[0], param2[1], param2[2]);
      transform_landmarks();
    }
    else
      make_affine_transform((xdimp1 + 1) / 2.0, (ydimp1 + 1) / 2.0, (zdimp1 + 1) / 2.0,
                        (xdimp2 + 1) / 2.0, (ydimp2 + 1) / 2.0, (zdimp2 + 1) / 2.0,
                        param[3], param[4], param[2], param[0], param[1], param[5],
                        param1[0], param1[1], param1[2], param2[0], param2[1], param2[2],
						1);
  }
  if (num_landmarks == 0)
    exec_regist(pyrs1, pyrstr1, xdimp1, ydimp1, zdimp1, xdimp2, ydimp2, zdimp2,interpolation);
}

double
mi_func()
{
  double e;

  make_transform_and_regist();
  e = MI();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}

double
corr_func()
{
  double e;

  make_transform_and_regist();
  e = -CORR();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}

double
nmi_func()
{
  double e;

  make_transform_and_regist();
  e = NMI();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}


double
ks_func()
{
  double e;

  make_transform_and_regist();
  e = -KS();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}

double
fs_func()
{
  double e;

  make_transform_and_regist();
  e = -FS();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}


double
ssd_func()
{
  double e;

  make_transform_and_regist();
  e = SSD();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}

double
sad_func()
{
  double e;

  make_transform_and_regist();
  e = SAD();
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}

double
spd_func()
{
  double e;

  make_transform_and_regist();
  e = SPD();
  if (func_count==1 || e<best_energy)
  {
#if 0
    int j;
	printf("best_energy = %f\n", e);
	for (j=0; j<4; j++)
      printf("{%f, %f, %f, %f}\n", scanner_tr[j][0], scanner_tr[j][1],
        scanner_tr[j][2], scanner_tr[j][3]);
#endif
    best_energy = e;
  }
  return e;
}

void
GaussianVolPyramid2D(m1, xd, yd, zd, m2, xdd2, ydd2, zdd2, a)
  unsigned short *m1, **m2;
  int xd, yd, zd;
  int *xdd2, *ydd2, *zdd2;
  double a;
{
  double kernel2d[5][5];
  double w[5];
  double sum;
  int k, l, s, j, i, s2, j2, i2, jj, ii, jjj, iii, sss;
  int x2, y2, z2;

  x2 = xd / 2;
  y2 = yd / 2;
  z2 = zd;
  *m2 = (unsigned short *)malloc(x2 * y2 * z2 * 2);
  memset(*m2, 0, x2 * y2 * z2 * 2);

  w[0] = w[4] = .25 - a / 2.0;
  w[1] = w[3] = .25;
  w[2] = a;

  for (k = 0; k < 5; k++)
    for (l = 0; l < 5; l++)
      kernel2d[k][l] = w[k] * w[l];

  for (s = 0, s2 = 0, sss = 0; s < z2; s++, s2 += y2 * x2, sss += yd * xd)
  {
    for (j = 0, j2 = s2, jj = 0; j < y2; j++, j2 += x2, jj += 2)
    {
      for (i = 0, i2 = j2, ii = 0; i < x2; i++, i2++, ii += 2)
      {
        sum = 0.0;
        for (k = 0; k < 5; k++)
        {
          for (l = 0; l < 5; l++)
          {
            jjj = jj + k - 2;
            iii = ii + l - 2;
            if (jjj >= 0 && jjj < yd && iii >= 0 && iii < xd)
              sum += kernel2d[k][l] * m1[sss + jjj * xd + iii];
          }
        }
        (*m2)[i2] = (unsigned short)(sum + .5);
      }
    }
  }
  *xdd2 = x2;
  *ydd2 = y2;
  *zdd2 = z2;
}

void find_trf(depth, m1, m2, xd1, yd1, zd1, xd2, yd2, zd2)
  int depth;
  unsigned short *m1, *m2;
  int xd1, yd1, zd1, xd2, yd2, zd2;
{
  unsigned short *ipyr1, *ipyr2;
  int xd3, yd3, zd3, xd4, yd4, zd4;

  if (depth > 0)
  {
    GaussianVolPyramid2D(m1, xd1, yd1, zd1, &ipyr1, &xd3, &yd3, &zd3, .5);
    GaussianVolPyramid2D(m2, xd2, yd2, zd2, &ipyr2, &xd4, &yd4, &zd4, .5);
    find_trf(depth - 1, ipyr1, ipyr2, xd3, yd3, zd3, xd4, yd4, zd4);
    free(ipyr1);
    free(ipyr2);
  }

  pyrstr1 = (unsigned short *)malloc(xd2 * yd2 * zd2 * 2);
  if (pyrstr1 == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(1);
  }
  pyrs1 = m1;
  pyrs2 = m2;
  xdimp1 = xd1;
  ydimp1 = yd1;
  zdimp1 = zd1;
  xdimp2 = xd2;
  ydimp2 = yd2;
  zdimp2 = zd2;
  ssizep = xdimp2 * ydimp2;
  vsizep = ssizep * zdimp2;
  printf("\nPyramid level for source: %d (%dx%dx%d)\n", depth + 1, xd1, yd1, zd1);
  printf("Pyramid level for target: %d (%dx%dx%d)\n", depth + 1, xd2, yd2, zd2);
  cur_depth = depth;
  func_count = 0;

  if (vecsize == 0)
    printf("Result: %lf\n", cost_func());
  else
  {
    NewUOA(vecsize, vector, 10.0, 0.01, 0, 1000, cost_func);
	if (m_flag)
      printf("Result affine parameters: %lf\t\tCount: %d\nParams: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
        best_energy, func_count, vector[0], vector[1], vector[2], vector[3], vector[4], vector[5], vector[6], vector[7], vector[8], vector[9], vector[10], vector[11]);
    else
      printf("Result affine parameters: %lf\t\tCount: %d\nParams: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", best_energy, func_count,param[0], param[1], param[5], param[3], param[4], param[2], param1[0], param1[1], param1[2], param2[0], param2[1], param2[2]);
  }
  free(pyrstr1);
}

int get_slices(dim, list)
  int dim;
  short *list;
{
  int i, sum;
  if (dim == 3) return (int)(list[0]);
  if (dim == 4) {
    for (sum = 0, i = 0; i < list[0]; i++)
      sum += list[1 + i];
    return(sum);
  }
  return 0;
}

void set_vector()
{
	vector[0] = param[0];
	vector[1] = param[1];
	vector[2] = param[5];
	switch (init_mode)
	{
	  case 0:
	    vector[3] = param[3];
		vector[4] = param[4];
		vector[5] = param[2];
	    break;
      case 9:
	    vector[4] = param[3];
		// fall through
	  case 1:
	    vector[3] = log(param1[0]*param1[1]*param1[2]);
	    break;
	  case 2:
	    vector[3] = log(param1[0]*param1[1]*param1[2]);
		vector[4] = log(param1[2]/param1[0]);
		vector[5] = log(param1[2]/param1[1]);
	    break;
	  case 3:
	    vector[3] = param[3];
		vector[4] = param[4];
		vector[5] = param[2];
		vector[6] = param2[0];
		vector[7] = param2[1];
		vector[8] = param2[2];
		vector[9] = log(param1[2]/param1[0]);
		vector[10] = log(param1[2]/param1[1]);
	    break;
	  case 4:
	    vector[3] = param[3];
		vector[4] = param[4];
		vector[5] = param[2];
		vector[6] = param2[0];
		vector[7] = param2[1];
		vector[8] = param2[2];
		vector[9] = log(param1[2]/param1[0]);
		vector[10] = log(param1[2]/param1[1]);
		vector[11] = log(param1[0]*param1[1]*param1[2]);
	    break;
	  case 5:
	    vector[3] = param[3];
		vector[4] = param[4];
		vector[5] = param[2];
		vector[6] = log(param1[0]*param1[1]*param1[2]);
	    break;
	  case 6:
	    break;
	}
}

void set_param()
{
	param[0] = vector[0];
	param[1] = vector[1];
	param[5] = vector[2];
	switch (init_mode)
	{
	  case 0:
	    param[3] = vector[3];
		param[4] = vector[4];
		param[2] = vector[5];
	    break;
	  case 9:
	    param[3] = vector[4];
		// fall through
	  case 1:
	    param1[0] = param1[1] = param1[2] = exp(vector[3]/3);
	    break;
	  case 2:
	    param1[2] = exp((vector[3]+vector[4]+vector[5])/3);
		param1[0] = param1[2]/exp(vector[4]);
		param1[1] = param1[2]/exp(vector[5]);
	    break;
	  case 3:
	    param[3] = vector[3];
		param[4] = vector[4];
		param[2] = vector[5];
		param2[0] = vector[6];
		param2[1] = vector[7];
		param2[2] = vector[8];
		param1[2] = exp((vector[9]+vector[10])/3);
		param1[0] = param1[2]/exp(vector[9]);
		param1[1] = param1[2]/exp(vector[10]);
	    break;
	  case 4:
	    param[3] = vector[3];
		param[4] = vector[4];
		param[2] = vector[5];
		param2[0] = vector[6];
		param2[1] = vector[7];
		param2[2] = vector[8];
		param1[2] = exp((vector[11]+vector[9]+vector[10])/3);
		param1[0] = param1[2]/exp(vector[9]);
		param1[1] = param1[2]/exp(vector[10]);
	    break;
	  case 5:
	    param[3] = vector[3];
		param[4] = vector[4];
		param[2] = vector[5];
		param1[0] = param1[1] = param1[2] = exp(vector[6]/3);
	    break;
	  case 6:
	    break;
	}
}

void write_out_header(FILE **out1, int argc, char *argv[])
{
  *out1 = fopen(argv[4], "wb");
  if (*out1 == NULL)
  {
    printf("Error in opening the output file\n");
    exit(-1);
  }
  memcpy(&vh2.gen, &vh1.gen, sizeof(vh2.gen));
  strncpy(vh2.gen.filename, argv[4], sizeof(vh2.gen.filename)-1);
  vh2.gen.filename[sizeof(vh2.gen.filename)-1] = 0;
  vh2.gen.filename_valid = 1;
  strncpy(vh2.gen.filename1, argv[2], sizeof(vh2.gen.filename1)-1);
  vh2.gen.filename1_valid = 1;
  vh2.scn.density_measurement_unit = vh1.scn.density_measurement_unit;
  vh2.scn.density_measurement_unit_valid = vh1.scn.density_measurement_unit_valid;
  vh2.scn.smallest_density_value = vh1.scn.smallest_density_value;
  vh2.scn.smallest_density_value_valid = vh1.scn.smallest_density_value_valid;
  vh2.scn.largest_density_value = vh1.scn.largest_density_value;
  vh2.scn.largest_density_value_valid = vh1.scn.largest_density_value_valid;
  vh2.scn.num_of_bits = vh1.scn.num_of_bits;
  vh2.scn.bit_fields = vh1.scn.bit_fields;
  vh2.scn.description = command_line(argc, argv);
  vh2.scn.description_valid = 1;

  error = VWriteHeader(*out1, &vh2, group, elem);
  if (error <= 104) {
    printf("Fatal error in writing output header\n");
    exit(-1);
  }
}

void finish_up(FILE **out1, int argc, char *argv[])
{
  FILE *fp_par;
  unsigned short *pout1;

  if ((fp_par = fopen(argv[5], "wb")) == NULL)
  {
    printf("ERROR: Can't open the PARAMETER file !\n");
    exit(-1);
  }
  if (m_flag)
  {
	double to[16];
	VInvertMatrix(to, vector, 4);
    if (fprintf(fp_par, "(%f %f %f %f)\n",
        to[0], to[1], to[2], to[3])<10 ||
        fprintf(fp_par, "(%f %f %f %f)\n",
		to[4], to[5], to[6], to[7])<10 ||
		fprintf(fp_par, "(%f %f %f %f)\n",
		to[8], to[9], to[10], to[11])<10)
    {
      fprintf(stderr, "Error writing parameter file.\n");
      exit(-1);
    }
  }
  else
  {
    fprintf(fp_par, "%lf\n", (double)param[0]);
    fprintf(fp_par, "%lf\n", (double)param[1]);
    fprintf(fp_par, "%lf\n", (double)param[5]);
    fprintf(fp_par, "%lf\n", (double)param[3]);
    fprintf(fp_par, "%lf\n", (double)param[4]);
    fprintf(fp_par, "%lf\n", (double)param[2]);
    fprintf(fp_par, "%lf\n", (double)param1[0]);
    fprintf(fp_par, "%lf\n", (double)param1[1]);
    fprintf(fp_par, "%lf\n", (double)param1[2]);
    fprintf(fp_par, "%lf\n", (double)param2[0]);
    fprintf(fp_par, "%lf\n", (double)param2[1]);
    fprintf(fp_par, "%lf\n", (double)param2[2]);
  }
  fclose(fp_par);

  free(ptrs2);

  write_out_header(out1, argc, argv);

  pout1 = (unsigned short *)malloc(vsize2 * 2);
  if (m_flag)
    set_affine_transform((xdimp1 + 1) / 2.0, (ydimp1 + 1) / 2.0, (zdimp1 + 1) / 2.0,
                        (xdimp2 + 1) / 2.0, (ydimp2 + 1) / 2.0, (zdimp2 + 1) / 2.0, 1);
  else
    make_affine_transform((double)(xdim1 + 1) / 2.0, (double)(ydim1 + 1) / 2.0, (double)(zdim1 + 1) / 2.0,
                       (double)(xdim2 + 1) / 2.0, (double)(ydim2 + 1) / 2.0, (double)(zdim2 + 1) / 2.0,
                       param[3], param[4], param[2], param[0], param[1], param[5],
                       param1[0], param1[1], param1[2], param2[0], param2[1], param2[2],
					   1);
  exec_regist(ptrs1, pout1, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2, interpolation);

  free(ptrs1);

  switch (bytes1)
  {
    case 1:
	  inc2 = (unsigned char *)malloc(vsize2 * bytes1);
      for (i = 0; i < vsize2; i++)
        inc2[i] = (unsigned char)pout1[i];
      free(pout1);
      break;

    case 2:
    case 0:
      inc2 = (unsigned char *)pout1;
      break;
  }

  if (bytes1)
    error = VWriteData((char *)inc2, bytes1, vsize2, *out1, &j);
  else
  {
    int k, m, n;
    for (j=0; j<zdim2; j++)
	  for (i=k=n=0,m=128; i<xdim2*ydim2; i++,m>>=1)
	  {
	    if (pout1[xdim2*ydim2 * j + i] >= 32767)
		  n |= m;
		if (m==1 || i==xdim2*ydim2-1)
		{
		  inc2[(xdim2*ydim2+7)/8*j + k++] = n;
		  n = 0;
		  m = 256;
		}
	  }
    error = VWriteData((char *)inc2, 1, (xdim2*ydim2+7)/8*zdim2, *out1, &j);
  }
  if (error)
  {
    fprintf(stderr, "Error writing data\n");
	exit(-1);
  }

  free(inc2);
  VCloseData(*out1);
}

void setup0(int argc, char *argv[])
{
  if (argc < 6)
    exit(-1);


  if (argc > 6)
  {
    sscanf(argv[6], "%d", &interpolation);
    if (interpolation < 0)
      interpolation = DEFAULT_INTERPOLATION;
  }
  else
    interpolation = DEFAULT_INTERPOLATION;

  if (argc > 7)
  {
    sscanf(argv[7], "%d", &init_mode);
    if (init_mode < 0)
      init_mode = DEFAULT_INIT_MODE;
  }
  else
    init_mode = DEFAULT_INIT_MODE;

  if (argc > 8)
  {
    sscanf(argv[8], "%d", &shift1);
    if (shift1 < 0)
      shift1 = DEFAULT_SHIFT;
  }
  else
    shift1 = DEFAULT_SHIFT;
  if (argc > 9)
  {
    sscanf(argv[9], "%d", &shift2);
    if (shift2 < 0)
      shift2 = DEFAULT_SHIFT;
  }
  else
    shift2 = DEFAULT_SHIFT;

  if (argc > 10)
  {
    sscanf(argv[10], "%d", &max_depth);
    if (max_depth < 0)
      max_depth = DEFAULT_DEPTH;
  }
  else
    max_depth = DEFAULT_DEPTH;
}

void setup1(int argc, char *argv[], FILE **in1, FILE **in2)
{
  *in1 = fopen(argv[2], "rb");
  if (*in1 == NULL)
  {
    printf("Error in opening source file \n");
    exit(-1);
  }
  *in2 = fopen(argv[3], "rb");
  if (*in2 == NULL)
  {
    printf("Error in opening target file \n");
    exit(-1);
  }
  error = VReadHeader(*in1, &vh1, group, elem);
  if (error <= 104) {
    printf("Fatal error in reading source header \n");
    exit(-1);
  }
  error = VReadHeader(*in2, &vh2, group, elem);
  if (error <= 104)
  {
    printf("Fatal error in reading target header \n");
    exit(-1);
  }

  if (vh1.gen.data_type != IMAGE0 || vh2.gen.data_type != IMAGE0)
  {
    printf("Cannot handle other than IMAGE0 files\n");
    exit(-1);
  }
  if (vh1.scn.dimension!=3 || vh2.scn.dimension!=3)
  {
    printf("Cannot handle other than 3 dimensional scenes.\n");
	exit(-1);
  }

  if (argv[1][0]!='p' && vh1.scn.domain_valid && vh2.scn.domain_valid)
  {
    double domain1[4][4], domain2[4][4], euler[3];
	int j, k;

	for (j=0; j<4; j++)
	  for (k=0; k<4; k++)
	    if (j==3 || k==3)
		  domain1[j][k] = domain2[j][k] = j==k;
		else
		{
		  domain1[j][k] = vh1.scn.domain[3+3*j+k];
		  domain2[k][j] = vh2.scn.domain[3+3*j+k];
		}
	MultMat(tr, domain1, domain2);
	get_euler(euler, tr);
	param[3] = 180/M_PI*euler[0];
	param[4] = 180/M_PI*euler[1];
	param[2] = 180/M_PI*euler[2];
  }

  bytes1 = vh1.scn.num_of_bits/8;
  bytes2 = argv[1][0]=='t'? bytes1: vh2.scn.num_of_bits/8;

  if (argv[1][0] == 'k')
  {
    thr1 = bytes1==1? 127: 32767;
	thr2 = bytes2==1? 127: 32767;
  }

  zdim1 = get_slices(vh1.scn.dimension, vh1.scn.num_of_subscenes);
  xdim1 = vh1.scn.xysize[0];
  ydim1 = vh1.scn.xysize[1];
  vsize1 = xdim1 * ydim1 * zdim1;
  voxelsize_x1 = vh1.scn.xypixsz[0];
  voxelsize_y1 = vh1.scn.xypixsz[1];
  voxelsize_z1 = vh1.scn.loc_of_subscenes[1] - vh1.scn.loc_of_subscenes[0];

  zdim2 = get_slices(vh2.scn.dimension, vh2.scn.num_of_subscenes);
  xdim2 = vh2.scn.xysize[0];
  ydim2 = vh2.scn.xysize[1];
  vsize2 = xdim2 * ydim2 * zdim2;
  voxelsize_x2 = vh2.scn.xypixsz[0];
  voxelsize_y2 = vh2.scn.xypixsz[1];
  voxelsize_z2 = vh2.scn.loc_of_subscenes[1] - vh2.scn.loc_of_subscenes[0];

  VSeekData(*in1, 0);
  VSeekData(*in2, 0);

  if (bytes1 == 0)
    inc1 = (unsigned char *)malloc((xdim1*ydim1+7)/8 * zdim1);
  else
    inc1 = (unsigned char *)malloc(vsize1 * bytes1);
  if (bytes2 == 0)
    inc2 = (unsigned char *)malloc((xdim2*ydim2+7)/8 * zdim2);
  else
    inc2 = (unsigned char *)malloc(vsize2 * bytes2);

  if (inc1 == NULL || (argv[1][0] != 't' && inc2 == NULL))
  {
    printf("Cannot allocate image space\n");
    exit(-1);
  }

  if (bytes1 == 0)
    error = VReadData((char *)inc1, 1, (xdim1*ydim1+7)/8 * zdim1, *in1, &j);
  else
    error = VReadData((char *)inc1, bytes1, vsize1, *in1, &j);
  if (error==0 && argv[1][0] != 't')
  {
    if (bytes2 == 0)
	  error = VReadData((char *)inc2, 1, (xdim2*ydim2+7)/8 * zdim2, *in2, &j);
	else
	  error = VReadData((char *)inc2, bytes2, vsize2, *in2, &j);
  }
  if (error)
  {
    fprintf(stderr, "Error reading data\n");
	exit(-1);
  }
  switch (bytes1)
  {
    case 0:
	  ptrs1 = (unsigned short *)malloc(vsize1 * 2);
      for (j=0; j<zdim1; j++)
	    for (i=0; i<xdim1*ydim1; i++)
		  ptrs1[xdim1*ydim1*j+i] =
		    inc1[(xdim1*ydim1+7)/8*j+i/8] & (128>>(i%8))? 65534: 0;
	  free(inc1);
	  break;

    case 1:
      ptrs1 = (unsigned short *)malloc(vsize1 * 2);
      for (i = 0; i < vsize1; i++)
        ptrs1[i] = inc1[i];
      free(inc1);
      break;

    case 2:
      ptrs1 = (unsigned short *)inc1;
      break;
  }

  switch (bytes2)
  {
    case 0:
	  ptrs2 = (unsigned short *)malloc(vsize2 * 2);
      for (j=0; j<zdim2; j++)
	    for (i=0; i<xdim2*ydim2; i++)
		  ptrs2[xdim2*ydim2*j+i] =
		    inc2[(xdim2*ydim2+7)/8*j+i/8] & (128>>(i%8))? 65534: 0;
	  free(inc2);
	  break;

    case 1:
      ptrs2 = (unsigned short *)malloc(vsize2 * 2);
      for (i = 0; i < vsize2; i++)
        ptrs2[i] = inc2[i];
      free(inc2);
      break;

    case 2:
      ptrs2 = (unsigned short *)inc2;
      break;
  }

  fclose(*in1);
  fclose(*in2);
}

void setup2()
{
  printf("Image size source: %dx%dx%d - %lfx%lfx%lf\n", xdim1, ydim1, zdim1, voxelsize_x1, voxelsize_y1, voxelsize_z1);
  printf("Image size target: %dx%dx%d - %lfx%lfx%lf\n", xdim2, ydim2, zdim2, voxelsize_x2, voxelsize_y2, voxelsize_z2);

  if (init_mode == 0) {
      vecsize = 6;
     }

  if (init_mode == 1) {
      vecsize = 4;
     }

  if (init_mode == 2) {
      vecsize = 6;
     }

  if (init_mode == 3) {
      vecsize = 11;
     }

  if (init_mode == 4) {
      vecsize = 12;
     }

  if (init_mode == 5) {
      vecsize = 7;
     }

  if (init_mode == 6) {
      vecsize = 3;
     }

  if (init_mode == 7) {
      vecsize = 0;
     }

  if (init_mode == 8) {
      vecsize = 12;
      m_flag = 1;
	  vector[0] = vector[5] = vector[10] = 1;
      return;
     }

  if (init_mode == 9) {
      vecsize = 5;
	 }

  set_vector();
}

void setup_hist()
{
  create_log_LUT();

  max1 = get_max(ptrs1, vsize1);
  max2 = get_max(ptrs2, vsize2);
  bins1 = max1 >> shift1;
  bins2 = max2 >> shift2;

  hist1 = (int *)malloc((bins1+1)*sizeof(int));
  hist2 = (int *)malloc((bins2+1)*sizeof(int));
  hist12 = (int **)malloc((bins2+1)*sizeof(int *));
  if (hist12 == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(1);
  }
  hist12[0] = (int *)malloc((bins2+1)*(bins1+1)*sizeof(int));
  if (hist12[0] == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
	exit(1);
  }
  for (j=1; j<=bins2; j++)
    hist12[j] = hist12[0]+j*(bins1+1);
}

void setup_landmarks(char *argv[3])
{
  FILE *fp;
  int j;

  if (sscanf(argv[0], "%d", &num_landmarks)!=1 || num_landmarks<1)
  {
    fprintf(stderr, "Specify 1 or more landmarks.\n");
    exit(1);
  }
  source_landmark = (landmark_t *)malloc(num_landmarks*sizeof(landmark_t));
  target_landmark = (landmark_t *)malloc(num_landmarks*sizeof(landmark_t));
  trans_landmark = (landmark_t *)malloc(num_landmarks*sizeof(landmark_t));
  if (trans_landmark == NULL)
  {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  fp = fopen(argv[1], "rb");
  if (fp == NULL)
  {
    fprintf(stderr, "Cannot open source landmark file.\n");
    exit(1);
  }
  for (j=0; j<num_landmarks; j++)
    if (fscanf(fp, "(%lf, %lf, %lf)\n",
      source_landmark[j], source_landmark[j]+1, source_landmark[j]+2) !=3)
    {
      fprintf(stderr, "Cannot read source landmark file.\n");
      exit(1);
    }
  fclose(fp);
  fp = fopen(argv[2], "rb");
  if (fp == NULL)
  {
    fprintf(stderr, "Cannot open target landmark file.\n");
    exit(1);
  }
  for (j=0; j<num_landmarks; j++)
    if (fscanf(fp, "(%lf, %lf, %lf)\n",
      target_landmark[j], target_landmark[j]+1, target_landmark[j]+2) !=3)
    {
      fprintf(stderr, "Cannot read target landmark file.\n");
      exit(1);
    }
  fclose(fp);
}

/* Modified: 5/8/01 exit code 0 passed on completion by Dewey Odhner. */
/* Modified: 5/13/03 hist12[j] assignment corrected by Dewey Odhner. */
/* Modified: 9/21/05 affine transformation and code extension by Andre Souza. */
int main(argc,argv)
  int argc;
  char *argv[];
{
  FILE *in1, *in2, *out1, *fp_par;
  double d;
  int j;

  if (argc < 2)
  {
    printf("\nUsage 1: affine [s c m n k f a p] <source> <target> <outfile> <paramfile> [<interpolation>] [<init_mode> 0: rigid, 1: scale, 2: anisotropic scale, 3: volume preserving, 4/8: affine, 5: homothetic, 6: translation only, 9: x-rotation] [<shift1> <shift2>] [<depth>] [<num_landmarks> <source_landmark_file> <target_landmark_file>]\n");

    printf("\nUsage 2: affine t <source> <target> <outfile> <paramfile> [<interpolation>]\n");

	puts( "    where paramfile is 12 parameters (rotations in degrees):" );
	puts( "              tx ty tz    rx ry rz    zx zy zz    sxy sxz syz" );
	puts( "          interpolation is one of {0=nn, 1=ln, 2=cu}" );
	puts( "" );
    exit(-1);
  }

  resetTime();

  switch (argv[1][0]) {
/***************/
/* mi matching */
/***************/
    case 'm':
  setup0(argc, argv);
printf("MI Matching...\n");
  setup1(argc, argv, &in1, &in2);

  setup_hist();

  setup2();

  cost_func = mi_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  free(hist12[0]);
  free(hist12);
  free(hist2);
  free(hist1);

  finish_up(&out1, argc, argv);

      break;

/***************/
/* corr matching */
/***************/
    case 'c':
  setup0(argc, argv);

printf("CORR Matching...\n");
  setup1(argc, argv, &in1, &in2);

  setup2();

  cost_func = corr_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);
  finish_up(&out1, argc, argv);



      break;

/*************/
/* transform */
/*************/
    case 't':
  if (argc < 5)
    exit(-1);


  if (argc > 6)
  {
    sscanf(argv[6], "%d", &interpolation);
    if (interpolation < 0)
      interpolation = DEFAULT_INTERPOLATION;
  }
  else
    interpolation = DEFAULT_INTERPOLATION;


printf("Transform...\n");
  setup1(argc, argv, &in1, &in2);

  write_out_header(&out1, argc, argv);

  if ((fp_par = fopen(argv[5], "rb")) == NULL)
  {
    printf("ERROR: Can't open the PARAMETER file !\n");
    exit(-1);
  }
  if (fscanf(fp_par, "(%lf %lf %lf %lf)\n",
      vector, vector+1, vector+2, vector+3) == 4)
  {
    if (fscanf(fp_par, "(%lf %lf %lf %lf)\n",
	    vector+4, vector+5, vector+6, vector+7)!=4 ||
		fscanf(fp_par, "(%lf %lf %lf %lf)\n",
		vector+8, vector+9, vector+10, vector+11)!=4)
	{
	  fprintf(stderr, "Error reading parameter file.\n");
	  exit(-1);
	}
    fclose(fp_par);
    m_flag = 1;
    set_affine_transform((xdim1 + 1) / 2.0, (ydim1 + 1) / 2.0, (zdim1 + 1) / 2.0,
                        (xdim2 + 1) / 2.0, (ydim2 + 1) / 2.0, (zdim2 + 1) / 2.0, 1);
  }
  else
  {
    fseek(fp_par, 0, 0);
    fscanf(fp_par, "%lf\n", &d);
    param[0] = d;
    fscanf(fp_par, "%lf\n", &d);
    param[1] = d;
    fscanf(fp_par, "%lf\n", &d);
    param[5] = d;
    fscanf(fp_par, "%lf\n", &d);
    param[3] = d;
    fscanf(fp_par, "%lf\n", &d);
    param[4] = d;
    fscanf(fp_par, "%lf\n", &d);
    param[2] = d;

    fscanf(fp_par, "%lf\n", &d);
    param1[0] = d;
    fscanf(fp_par, "%lf\n", &d);
    param1[1] = d;
    fscanf(fp_par, "%lf\n", &d);
    param1[2] = d;

    fscanf(fp_par, "%lf\n", &d);
    param2[0] = d;
    fscanf(fp_par, "%lf\n", &d);
    param2[1] = d;
    fscanf(fp_par, "%lf\n", &d);
    param2[2] = d;
    fclose(fp_par);

    make_affine_transform( (double)(xdim1 + 1) / 2.0, (double)(ydim1 + 1) / 2.0, (double)(zdim1 + 1) / 2.0,
                        (double)(xdim2 + 1) / 2.0, (double)(ydim2 + 1) / 2.0, (double)(zdim2 + 1) / 2.0,
                        param[3], param[4], param[2],
					    param[0], param[1], param[5],
                        param1[0], param1[1], param1[2],
					    param2[0], param2[1], param2[2], 1 );
  }
  exec_regist(ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2, interpolation);

  switch (bytes1)
  {
    case 1:
	  inc2 = (unsigned char *)malloc(vsize2 * bytes1);
      for (i = 0; i < vsize2; i++)
        inc2[i] = (unsigned char)ptrs2[i];
      free(ptrs1);
      free(ptrs2);
      break;

    case 2:
	case 0:
      inc1 = (unsigned char *)ptrs1;
      inc2 = (unsigned char *)ptrs2;
      break;
  }


  if (bytes1)
    error = VWriteData((char *)inc2, bytes1, vsize2, out1, &j);
  else
  {
    int k, m, n;
    for (j=0; j<zdim2; j++)
	  for (i=k=n=0,m=128; i<xdim2*ydim2; i++,m>>=1)
	  {
	    if (ptrs2[xdim2*ydim2 * j + i] >= 32767)
		  n |= m;
		if (m==1 || i==xdim2*ydim2-1)
		{
		  inc2[(xdim2*ydim2+7)/8*j + k++] = n;
		  n = 0;
		  m = 256;
		}
	  }
    error = VWriteData((char *)inc2, 1, (xdim2*ydim2+7)/8*zdim2, out1, &j);
  }
  if (error)
  {
    fprintf(stderr, "Error writing data\n");
	exit(-1);
  }
  VCloseData(out1);

  free(inc2);    inc2=NULL;
      break;

/***************/
/* nmi matching */
/***************/
    case 'n':
  setup0(argc, argv);

printf("NMI Matching...\n");
  setup1(argc, argv, &in1, &in2);

  setup_hist();

  setup2();

  cost_func = nmi_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  free(hist12[0]);
  free(hist12);
  free(hist2);
  free(hist1);
  finish_up(&out1, argc, argv);
      break;


/***************/
/* ks matching */
/***************/
    case 'k':
  setup0(argc, argv);

printf("KS Matching...\n");
  setup1(argc, argv, &in1, &in2);


  setup2();

  cost_func = ks_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  finish_up(&out1, argc, argv);

      break;


/***************/
/* fs matching */
/***************/
    case 'f':
  setup0(argc, argv);

printf("Fuzzy Matching...\n");
  setup1(argc, argv, &in1, &in2);


  setup2();

  cost_func = fs_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  finish_up(&out1, argc, argv);
      break;

/***************/
/* ssd matching */
/***************/
    case 's':
  setup0(argc, argv);

printf("SSD Matching...\n");
  setup1(argc, argv, &in1, &in2);

  setup2();

  cost_func = ssd_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  finish_up(&out1, argc, argv);


      break;

/***************/
/* sad matching */
/***************/
    case 'a':
  setup0(argc, argv);

printf("SAD Matching...\n");
  setup1(argc, argv, &in1, &in2);

  setup2();

  cost_func = sad_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  finish_up(&out1, argc, argv);

      break;

/***************/
/* procrustes matching */
/***************/
    case 'p':
  setup_landmarks(argv + argc-3);
  setup0(argc-3, argv);

printf("Procrustes Matching...\n");
  setup1(argc-3, argv, &in1, &in2);

  setup2();

  cost_func = spd_func;
  find_trf(max_depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);

  finish_up(&out1, argc-3, argv);

      break;

  } //main switch ends


  printf("Total computation lasted: %f seconds\n", getElapsedTime());
  exit(0);
}
