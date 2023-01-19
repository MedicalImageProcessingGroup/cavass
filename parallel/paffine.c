/*
  Copyright 1993-2009, 2015 Medical Image Processing Group
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
#include "Etime.h"
#include "Viewnix.h"
#include "mpi.h"

#define DEFAULT_INTERPOLATION 0
#define DEFAULT_INIT_MODE 0
#define DEFAULT_THRESHOLD 0
#define DEFAULT_SHIFT 0
#define DEFAULT_DEPTH 2

#define LOG_SCALE 10000
double log_LUT[LOG_SCALE + 1];

double tr[4][4];
double param[6];
double param1[3] = {1,1,1};
double param2[3];
int *hist1, *hist2, *hist12;
int *hists1, *hists2, *hists12;
int count, counts;
int bins1, bins2;
int shift1, shift2;
unsigned short thr1, thr2;
unsigned short max1, max2;
int func_count;
int interpolation;
int init_mode, rigid, scale, shear;
int depth;

char group[6], elem[6];

int bytes1, bytes2, vsize1, vsize2, error;
ViewnixHeader vh1, vh2;
int xdim1, ydim1, zdim1, xdim2, ydim2, zdim2;
double voxelsize_x1, voxelsize_y1, voxelsize_z1;
double voxelsize_x2, voxelsize_y2, voxelsize_z2;
unsigned char *inc1, *inc2;
unsigned short *ptrs1, *ptrs2, *pyrs1, *pyrs2, *pyrstr1;
int xdimp1, ydimp1, zdimp1, xdimp2, ydimp2, zdimp2,ssizep, vsizep;

double best_energy;

/***********parallel variables**************/
unsigned short  *chunks2;  /* chunk of source image */
int myrank;   /* compute task's rank */
int ntasks;   /* number of processors */
int slice_begin, slice_end, chunk_size, chunk_slice, r; /*chunck info*/
double q;
/*******************************************/




void
make_affine_transform(ox1, oy1, oz1, ox2, oy2, oz2, rx, ry, rz, tx, ty, tz, zx, zy, zz, sxy, sxz, syz, inv)
  double ox1, oy1, oz1, ox2, oy2, oz2, rx, ry, rz, tx, ty, tz, zx, zy, zz, sxy, sxz, syz;
  int inv;
{
  double sign, rax, ray, raz, temp[4][4];
  static double trx[4][4], try[4][4], trz[4][4], tt[4][4],
                to1[4][4], to2[4][4], tzm[4][4], tsr[4][4];

  sign = inv ? -1.0 : 1.0;

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

  to1[0][0] = inv ? 1.0 / voxelsize_x1 : voxelsize_x1;
  to1[1][1] = inv ? 1.0 / voxelsize_y1 : voxelsize_y1;
  to1[2][2] = inv ? 1.0 / voxelsize_z1 : voxelsize_z1;
  to1[3][3] = 1.0;
  to1[0][3] = inv ? ox1 : -voxelsize_x1 * ox1;
  to1[1][3] = inv ? oy1 : -voxelsize_y1 * oy1;
  to1[2][3] = inv ? oz1 : -voxelsize_z1 * oz1;

  to2[0][0] = inv ? voxelsize_x2 : 1.0 / voxelsize_x2;
  to2[1][1] = inv ? voxelsize_y2 : 1.0 / voxelsize_y2;
  to2[2][2] = inv ? voxelsize_z2 : 1.0 / voxelsize_z2;
  to2[3][3] = 1.0;
  to2[0][3] = inv ? -voxelsize_x2 * ox2 : ox2;
  to2[1][3] = inv ? -voxelsize_y2 * oy2 : oy2;
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
  memset(&hist12[0], 0, (bins1 + 1) * (bins2 + 1) * sizeof(int));
  count = 0;
  for (i = 0; i < size; in1++, in2++, i++)
  {
    if (*in1 > thr1 && *in2 > thr2)
    {
      v1 = (*in1) >> shift1;
      v2 = (*in2) >> shift2;
      hist1[v1]++;
      hist2[v2]++;
      
      hist12[v2*(bins1+1) + v1]++;
      count++;
    }
  }
  //printf("count: %d  (%d)\n",count, myrank);
}

double
MI()
{
  int i, j, iii, jjj;
  int h1, h2, h12;
  double mi, scale;

  mi = 0.0;
  scale = (double)LOG_SCALE / count;
  for (i = 0; i <= bins1; i++)
    hist1[i] = (int)(hist1[i] * scale);
  for (j = 0; j <= bins2; j++)
    hist2[j] = (int)(hist2[j] * scale);
  for (j = 0, jjj=0; j <= bins2; j++, jjj += (bins1+1))
    for (i = 0, iii=jjj; i <= bins1; i++, iii++)
      hist12[iii] = (int)(hist12[iii] * scale);
  for (j = 0, jjj=0; j <= bins2; j++, jjj += (bins1+1))
  {
    h2 = hist2[j];
    if (h2 > 0)
    {
      for (i = 0, iii=jjj; i <= bins1; i++, iii++)
      {
        h1 = hist1[i];
        if (h1 > 0)
        {
          h12 = hist12[iii];
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
  double corr;

  corr = 0.0;
  for (k = 0, kkk = 0; k < chunk_slice; k++, kkk += ssizep)
  {
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        corr += pyrstr1[iii] * chunks2[iii];
      }
    }
  }
  return corr;
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
    
  sum1 = 1.0;
  sum2 = 1.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        if (pyrstr1[iii] > 0 || pyrs2[iii] > 0)		
	      sum1 = sum1 + 1;
	if (pyrstr1[iii] > 0 && pyrs2[iii] > 0)		
	      sum2 = sum2 + 1;
      }
    }
  }
  ks = 10*(sum2/sum1); 
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
  fs = 10*(sum2/sum1); 
  return fs;
}

double
SSD()
{
  int i, j, k, iii, jjj, kkk;
  double ssd;

  ssd = 0.0;
  for (k = 0, kkk = 0; k < zdimp2; k++, kkk += ssizep)
  {
    for (j = 0, jjj = kkk; j < ydimp2; j++, jjj += xdimp2)
    {
      for (i = 0, iii = jjj; i < xdimp2; i++, iii++)
      {
        ssd += pow((double)(pyrstr1[iii] - pyrs2[iii]),2);
      }
    }
  }
  return ssd;
}

void
exec_regist(in1, out1, xd1, yd1, zd1, xd2, yd2, zd2,interp)
  unsigned short *in1, *out1;
  int xd1, yd1, zd1, xd2, yd2, zd2;
  int interp;
{
  int i, j, k;
  double di2, dj2, dk2;
  double di3, dj3, dk3;
  double di4, dj4, dk4;
  int i5, j5, k5;
  int i6, j6, k6;
  double t11, t12, t13, t14, t21, t22, t23, t24, t31, t32, t33, t34;
  int iii, jjj, kkk, iii5;
  double wx, wy, wz;
  double wx1, wy1, wz1;
  int slicesize;
  double w, ws, temp[4][4];
 
  for (j = 0; j < 4; j++)
    for (i = 0; i < 4; i++)
      temp[j][i] = tr[j][i];
  
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
      for (k = 0, kkk = 0, di2 = t14 + (slice_begin*t13), dj2 = t24 + (slice_begin*t23), dk2 = t34 + (slice_begin*t33);
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
            i5 = (int)(di4 + .5);
            j5 = (int)(dj4 + .5);
            k5 = (int)(dk4 + .5);
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
      for (k = 0, kkk = 0, di2 = t14 + (slice_begin*t13), dj2 = t24 + (slice_begin*t23), dk2 = t34 + (slice_begin*t33);
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
            i5 = (int)(di4 + .5);
            j5 = (int)(dj4 + .5);
            k5 = dk4 >= 0.0 ? dk4 : dk4 - 1.0;
            wz = dk4 - k5;
            wz1 = 1.0 - wz;
            k6 = k5 + 1;
            ws = 0.0;
            if (i5 >= 0 && i5 < xd1 && j5 >= 0 && j5 < yd1)
            {
              if (k5 >= 0 && k5 < zd1)
              {
                w = wz1;
                iii5 = (k5 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
                ws += in1[iii5] * w;
              }
              if (k6 >= 0 && k6 < zd1)
              {
                w = wz;
                iii5 = (k6 * yd1 + (yd1 - 1 - j5)) * xd1 + i5;
                ws += in1[iii5] * w;
              }
            }
            out1[iii] = ws;
          }
        }
      }
      break;

    case 2:
      for (k = 0, kkk = 0, di2 = t14 + (slice_begin*t13), dj2 = t24 + (slice_begin*t23), dk2 = t34 + (slice_begin*t33);
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
            ws = 0.0;
            i5 = di4 >= 0.0 ? di4 : di4 - 1.0;
            j5 = dj4 >= 0.0 ? dj4 : dj4 - 1.0;
            k5 = dk4 >= 0.0 ? dk4 : dk4 - 1.0;
            wx = di4 - i5;
            wy = dj4 - j5;
            wz = dk4 - k5;
            wx1 = 1.0 - wx;
            wy1 = 1.0 - wy;
            wz1 = 1.0 - wz;
            i6 = i5 + 1;
            j6 = j5 + 1;
            k6 = k5 + 1;
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
            out1[iii] = ws;
          }
        }
      }
      break;
  }
}

double
mi_func()
{
  double e;
  /***********parallel variables**************/
  int tag, flag, n, init_pos, i, j, iii, jjj;
  double es;
  MPI_Status status; /* returned from MPI */
  /******************************************/
  func_count++;
  if (scale1 == 1) {
     param1[0] = param1[0]; 
     param1[1] = param1[0];
     param1[2] = param1[0];
  } 
  make_affine_transform((double)(xdimp1 + 1) / 2.0, (double)(ydimp1 + 1) / 2.0, (double)(zdimp1 + 1) / 2.0,
                       (double)(xdimp2 + 1) / 2.0, (double)(ydimp2 + 1) / 2.0, (double)(zdimp2 + 1) / 2.0,
                       param[3], param[4], param[2], param[0], param[1], param[5],
                       param1[0], param1[1], param1[2], param2[0], param2[1], param2[2],1);
  e = 0;
  flag = 1;
  tag = 5;
  for (n = 1; n < ntasks; n++)
  {
     MPI_Send(&flag,1, MPI_INT,n,tag,MPI_COMM_WORLD);
  }

  /* Distribute tasks to all workers*/
  for(n = 1; n < ntasks; n++)
      {
        /*transfering afffine matrix */
        tag = 6;
        MPI_Send(tr,16,MPI_DOUBLE,n,tag,MPI_COMM_WORLD);        
      }

  tag = 7;
  MPI_Recv(hist1, bins1 + 1, MPI_INT, 1, tag,MPI_COMM_WORLD, &status);
  tag = 8;
  MPI_Recv(hist2, bins2 + 1, MPI_INT, 1, tag,MPI_COMM_WORLD, &status);
  tag = 9;
  MPI_Recv(hist12, (bins1 + 1) * (bins2 + 1), MPI_INT, 1, tag,MPI_COMM_WORLD, &status);
  tag = 10;
  MPI_Recv(&count, 1, MPI_INT, 1, tag,MPI_COMM_WORLD, &status);

  for( n = 2; n < ntasks; ++n)
      {
        tag = 7;
        MPI_Recv(hists1, bins1 + 1, MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        tag = 8;
        MPI_Recv(hists2, bins2 + 1, MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        tag = 9;
        MPI_Recv(hists12, (bins1 + 1) * (bins2 + 1), MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        tag = 10;
        MPI_Recv(&counts, 1, MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        for (i = 0; i <= bins1; i++)
            hist1[i] += hists1[i];
        for (j = 0; j <= bins2; j++)
            hist2[j] += hists2[j];
        for (j = 0, jjj=0; j <= bins2; j++, jjj += (bins1+1))
          for (i = 0, iii=jjj; i <= bins1; i++, iii++)
            hist12[iii] += hists12[iii];
        count += counts;
      }
  e = MI();
 
  if (func_count==1 || e<best_energy)
        best_energy = e;
  return e;
}

double
scalar_func()  
{
  double e;
  /***********parallel variables**************/
  int tag, flag, n, init_pos, i, ii;
  double es;
  MPI_Status status; /* returned from MPI */
  /******************************************/
  func_count++;
  if (scale1 == 1) {
     param1[0] = param1[0]; 
     param1[1] = param1[0];
     param1[2] = param1[0];
  } 
  make_affine_transform((double)(xdimp1 + 1) / 2.0, (double)(ydimp1 + 1) / 2.0, (double)(zdimp1 + 1) / 2.0,
                       (double)(xdimp2 + 1) / 2.0, (double)(ydimp2 + 1) / 2.0, (double)(zdimp2 + 1) / 2.0,
                       param[3], param[4], param[2], param[0], param[1], param[5],
                       param1[0], param1[1], param1[2], param2[0], param2[1], param2[2],1);
  e = 0;
  flag = 1;
  tag = 5;
  for (n = 1; n < ntasks; n++)
  {
     MPI_Send(&flag,1, MPI_INT,n,tag,MPI_COMM_WORLD);
  }
  
  /* Distribute tasks to all workers*/
  for(n = 1; n < ntasks; n++)
      {
 	/*transfering afffine matrix */
        tag = 6;
 	MPI_Send(tr,16,MPI_DOUBLE,n,tag,MPI_COMM_WORLD);
      }
  for(n = 1; n < ntasks; n++)
      { 
        tag = 7;
        MPI_Recv(&es,1,MPI_DOUBLE,n,tag,MPI_COMM_WORLD, &status);
        e += es;	         
      }
  
  if (func_count==1 || e<best_energy)
    best_energy = e;
  return e;
}

double
nmi_func()
{
  double e;
  /***********parallel variables**************/
  int tag, flag, n, init_pos, i, j, iii, jjj;
  double es;
  MPI_Status status; /* returned from MPI */
  /******************************************/
  func_count++;
  if (scale1 == 1) {
     param1[0] = param1[0]; 
     param1[1] = param1[0];
     param1[2] = param1[0];
  } 
  make_affine_transform((double)(xdimp1 + 1) / 2.0, (double)(ydimp1 + 1) / 2.0, (double)(zdimp1 + 1) / 2.0,
                       (double)(xdimp2 + 1) / 2.0, (double)(ydimp2 + 1) / 2.0, (double)(zdimp2 + 1) / 2.0,
                       param[3], param[4], param[2], param[0], param[1], param[5],
                       param1[0], param1[1], param1[2], param2[0], param2[1], param2[2],1);
  e = 0;
  flag = 1;
  tag = 5;
  for (n = 1; n < ntasks; n++)
  {
     MPI_Send(&flag,1, MPI_INT,n,tag,MPI_COMM_WORLD);
  }

  /* Distribute tasks to all workers*/
  for(n = 1; n < ntasks; n++)
      {
        /*transfering afffine matrix */
        tag = 6;
        MPI_Send(tr,16,MPI_DOUBLE,n,tag,MPI_COMM_WORLD);        
      }

  tag = 7;
  MPI_Recv(hist1, bins1 + 1, MPI_INT, 1, tag,MPI_COMM_WORLD, &status);
  tag = 8;
  MPI_Recv(hist2, bins2 + 1, MPI_INT, 1, tag,MPI_COMM_WORLD, &status);
  tag = 9;
  MPI_Recv(hist12, (bins1 + 1) * (bins2 + 1), MPI_INT, 1, tag,MPI_COMM_WORLD, &status);
  tag = 10;
  MPI_Recv(&count, 1, MPI_INT, 1, tag,MPI_COMM_WORLD, &status);

  for( n = 2; n < ntasks; ++n)
      {
        tag = 7;
        MPI_Recv(hists1, bins1 + 1, MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        tag = 8;
        MPI_Recv(hists2, bins2 + 1, MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        tag = 9;
        MPI_Recv(hists12, (bins1 + 1) * (bins2 + 1), MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        tag = 10;
        MPI_Recv(&counts, 1, MPI_INT, n, tag,MPI_COMM_WORLD, &status);
        for (i = 0; i <= bins1; i++)
            hist1[i] += hists1[i];
        for (j = 0; j <= bins2; j++)
            hist2[j] += hists2[j];
        for (j = 0, jjj=0; j <= bins2; j++, jjj += (bins1+1))
          for (i = 0, iii=jjj; i <= bins1; i++, iii++)
            hist12[iii] += hists12[iii];
        count += counts;
      }
  e = NMI();
 
  if (func_count==1 || e<best_energy)
        best_energy = e;
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
        (*m2)[i2] = (int)(sum + .5);
      }
    }
  }
  *xdd2 = x2;
  *ydd2 = y2;
  *zdd2 = z2;
}


 void
find_mi_trf(depth, m1, m2, xd1, yd1, zd1, xd2, yd2, zd2)
  int depth;
  unsigned short *m1, *m2;
  int xd1, yd1, zd1, xd2, yd2, zd2;
{
  int i, ii, init_pos,j, iter, target_info[3], source_info[3], bins_info[2], n, tag, flag;
  double fret;
  unsigned short *ipyr1, *ipyr2;
  int xd3, yd3, zd3, xd4, yd4, zd4;;

  if (depth > 0)
  {

    GaussianVolPyramid2D(m1, xd1, yd1, zd1, &ipyr1, &xd3, &yd3, &zd3, .5);
    GaussianVolPyramid2D(m2, xd2, yd2, zd2, &ipyr2, &xd4, &yd4, &zd4, .5);
    find_mi_trf(depth - 1, ipyr1, ipyr2, xd3, yd3, zd3, xd4, yd4, zd4);
    free(ipyr1);
    free(ipyr2);
    if (rigid == 1) {
      param[0] *= 2.0;
      param[1] *= 2.0;
      param[5] *= 2.0;
    } 
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

  source_info[0] = xdimp1;
  source_info[1] = ydimp1;
  source_info[2] = zdimp1;
  tag = 1;
  for(n = 1; n < ntasks; n++)
      MPI_Send(source_info,3, MPI_INT,n,tag,MPI_COMM_WORLD);
  
  target_info[0] = xdimp2;
  target_info[1] = ydimp2;
  target_info[2] = zdimp2;
  tag = 2;
  for(n = 1; n < ntasks; n++)
      MPI_Send(target_info,3, MPI_INT,n,tag,MPI_COMM_WORLD);



  init_pos = 0;
  /* Distribute work to all slaves */
  /*transfer chunk_source data to slaves */
  tag = 3;
  for(n = 1; n < ntasks; n++)
     MPI_Send(pyrs1,xdimp1 * ydimp1 * zdimp1,MPI_UNSIGNED_SHORT,n,tag,MPI_COMM_WORLD);
  
  
  r = zdim2  % (ntasks-1);
  q = (double) zdimp2 / (ntasks-1);
  chunk_size = ceil(q) * ssizep;
  chunks2  = (unsigned short *)malloc(chunk_size * 2);
  slice_begin = 0; slice_end = 0;
  for(n = 1; n < ntasks; n++)
     {
        chunk_slice = n < (r + 1) ? ceil (q) : floor(q);
        
        chunk_size = chunk_slice * ssizep;
        slice_end = slice_begin + (chunk_slice-1);
                
        for (i = 0, ii = init_pos; i < chunk_size; i++, ii++)
          {
            chunks2[i] = ptrs2[ii];            
          }
        init_pos += chunk_size; 
        
 	/*transfer chunk_target data to slaves */
        tag = 4;
        MPI_Send(chunks2,chunk_size,MPI_UNSIGNED_SHORT,n,tag,MPI_COMM_WORLD);
        printf("master: allocating slice %d - %d to slave process %d.\n",slice_begin,slice_end,n);
        slice_begin = slice_end + 1;
      }
  
  func_count = 0;
  if (rigid == 1) {  
    NewUOA(6, param, 10.0, 0.01, 0, 1000, mi_func);
    }
  if (scale == 1) { 
    NewUOA(3, param1, 10.0, 0.01, 0, 1000, mi_func);
    }
  if (shear == 1) { 
    NewUOA(3, param2, 10.0, 0.01, 0, 1000, mi_func);
  }
  printf("\nResult affine 15 parameters: %lf\t\tCount: %d\nParams: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n\n", best_energy, func_count,param[0], param[1], param[5], param[3], param[4], param[2], param1[0], param1[1], param1[2], param2[0], param2[1], param2[2]);
 free(chunks2);
 flag = 0;
 tag  = 5;
 for(n = 1; n < ntasks; n++)
    {
     MPI_Send(&flag,1, MPI_INT,n,tag,MPI_COMM_WORLD);
     printf("process %d done.\n", n);
    }

}


 void
find_scalar_trf(depth, m1, m2, xd1, yd1, zd1, xd2, yd2, zd2)
  int depth;
  unsigned short *m1, *m2;
  int xd1, yd1, zd1, xd2, yd2, zd2;
{
  int i, ii, init_pos,j, iter, target_info[3], source_info[3], n, tag, flag;
  double fret;
  unsigned short *ipyr1, *ipyr2;
  int xd3, yd3, zd3, xd4, yd4, zd4;;

  if (depth > 0)
  {

    GaussianVolPyramid2D(m1, xd1, yd1, zd1, &ipyr1, &xd3, &yd3, &zd3, .5);
    GaussianVolPyramid2D(m2, xd2, yd2, zd2, &ipyr2, &xd4, &yd4, &zd4, .5);
    find_corr_trf(depth - 1, ipyr1, ipyr2, xd3, yd3, zd3, xd4, yd4, zd4);
    free(ipyr1);
    free(ipyr2);
    if (rigid == 1) {
      param[0] *= 2.0;
      param[1] *= 2.0;
      param[5] *= 2.0;
    } 
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

  source_info[0] = xdimp1;
  source_info[1] = ydimp1;
  source_info[2] = zdimp1;
  tag = 1;
  for(n = 1; n < ntasks; n++)
      MPI_Send(source_info,3, MPI_INT,n,tag,MPI_COMM_WORLD);
  
  target_info[0] = xdimp2;
  target_info[1] = ydimp2;
  target_info[2] = zdimp2;
  tag = 2;
  for(n = 1; n < ntasks; n++)
      MPI_Send(target_info,3, MPI_INT,n,tag,MPI_COMM_WORLD);
  init_pos = 0;
  /* Distribute work to all slaves */
  /*transfer chunk_source data to slaves */
  tag = 3;
  for(n = 1; n < ntasks; n++)
     MPI_Send(pyrs1,xdimp1 * ydimp1 * zdimp1,MPI_UNSIGNED_SHORT,n,tag,MPI_COMM_WORLD);
  
  
  r = zdim2  % (ntasks-1);
  q = (double) zdimp2 / (ntasks-1);
  chunk_size = ceil(q) * ssizep;
  chunks2  = (unsigned short *)malloc(chunk_size * 2);
  slice_begin = 0; slice_end = 0;
  for(n = 1; n < ntasks; n++)
     {
        chunk_slice = n < (r + 1) ? ceil (q) : floor(q);
        
        chunk_size = chunk_slice * ssizep;
        slice_end = slice_begin + (chunk_slice-1);
                
        for (i = 0, ii = init_pos; i < chunk_size; i++, ii++)
          {
            chunks2[i] = ptrs2[ii];            
          }
        init_pos += chunk_size; 
        
 	/*transfer chunk_target data to slaves */
        tag = 4;
        MPI_Send(chunks2,chunk_size,MPI_UNSIGNED_SHORT,n,tag,MPI_COMM_WORLD);
        printf("master: allocating slice %d - %d to slave process %d.\n",slice_begin,slice_end,n);
        slice_begin = slice_end + 1;
      }
  
  func_count = 0;
  if (rigid == 1) {  
    NewUOA(6, param, 10.0, 0.01, 0, 1000, scalar_func);
    }
  if (scale == 1) { 
    NewUOA(3, param1, 10.0, 0.01, 0, 1000, scalar_func);
    }
  if (shear == 1) { 
    NewUOA(3, param2, 10.0, 0.01, 0, 1000, scalar_func);
  }
  printf("\nResult affine 15 parameters: %lf\t\tCount: %d\nParams: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n\n", best_energy, func_count,param[0], param[1], param[5], param[3], param[4], param[2], param1[0], param1[1], param1[2], param2[0], param2[1], param2[2]);
 free(chunks2);
 flag = 0;
 tag  = 5;
 for(n = 1; n < ntasks; n++)
    {
     MPI_Send(&flag,1, MPI_INT,n,tag,MPI_COMM_WORLD);
     printf("process %d done.\n", n);
    }

}


 void
find_nmi_trf(depth, m1, m2, xd1, yd1, zd1, xd2, yd2, zd2)
  int depth;
  unsigned short *m1, *m2;
  int xd1, yd1, zd1, xd2, yd2, zd2;
{
  int i, ii, init_pos,j, iter, target_info[3], source_info[3], bins_info[2], n, tag, flag;
  double fret;
  unsigned short *ipyr1, *ipyr2;
  int xd3, yd3, zd3, xd4, yd4, zd4;;

  if (depth > 0)
  {

    GaussianVolPyramid2D(m1, xd1, yd1, zd1, &ipyr1, &xd3, &yd3, &zd3, .5);
    GaussianVolPyramid2D(m2, xd2, yd2, zd2, &ipyr2, &xd4, &yd4, &zd4, .5);
    find_nmi_trf(depth - 1, ipyr1, ipyr2, xd3, yd3, zd3, xd4, yd4, zd4);
    free(ipyr1);
    free(ipyr2);
    if (rigid == 1) {
      param[0] *= 2.0;
      param[1] *= 2.0;
      param[5] *= 2.0;
    } 
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

  source_info[0] = xdimp1;
  source_info[1] = ydimp1;
  source_info[2] = zdimp1;
  tag = 1;
  for(n = 1; n < ntasks; n++)
      MPI_Send(source_info,3, MPI_INT,n,tag,MPI_COMM_WORLD);
  
  target_info[0] = xdimp2;
  target_info[1] = ydimp2;
  target_info[2] = zdimp2;
  tag = 2;
  for(n = 1; n < ntasks; n++)
      MPI_Send(target_info,3, MPI_INT,n,tag,MPI_COMM_WORLD);



  init_pos = 0;
  /* Distribute work to all slaves */
  /*transfer chunk_source data to slaves */
  tag = 3;
  for(n = 1; n < ntasks; n++)
     MPI_Send(pyrs1,xdimp1 * ydimp1 * zdimp1,MPI_UNSIGNED_SHORT,n,tag,MPI_COMM_WORLD);
  
  
  r = zdim2  % (ntasks-1);
  q = (double) zdimp2 / (ntasks-1);
  chunk_size = ceil(q) * ssizep;
  chunks2  = (unsigned short *)malloc(chunk_size * 2);
  slice_begin = 0; slice_end = 0;
  for(n = 1; n < ntasks; n++)
     {
        chunk_slice = n < (r + 1) ? ceil (q) : floor(q);
        
        chunk_size = chunk_slice * ssizep;
        slice_end = slice_begin + (chunk_slice-1);
                
        for (i = 0, ii = init_pos; i < chunk_size; i++, ii++)
          {
            chunks2[i] = ptrs2[ii];            
          }
        init_pos += chunk_size; 
        
 	/*transfer chunk_target data to slaves */
        tag = 4;
        MPI_Send(chunks2,chunk_size,MPI_UNSIGNED_SHORT,n,tag,MPI_COMM_WORLD);
        printf("master: allocating slice %d - %d to slave process %d.\n",slice_begin,slice_end,n);
        slice_begin = slice_end + 1;
      }
  
  func_count = 0;
  if (rigid == 1) {  
    NewUOA(6, param, 10.0, 0.01, 0, 1000, nmi_func);
    }
  if (scale == 1) { 
    NewUOA(3, param1, 10.0, 0.01, 0, 1000, nmi_func);
    }
  if (shear == 1) { 
    NewUOA(3, param2, 10.0, 0.01, 0, 1000, nmi_func);
  }
  printf("\nResult affine 15 parameters: %lf\t\tCount: %d\nParams: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n\n", best_energy, func_count,param[0], param[1], param[5], param[3], param[4], param[2], param1[0], param1[1], param1[2], param2[0], param2[1], param2[2]);
 free(chunks2);
 flag = 0;
 tag  = 5;
 for(n = 1; n < ntasks; n++)
    {
     MPI_Send(&flag,1, MPI_INT,n,tag,MPI_COMM_WORLD);
     printf("process %d done.\n", n);
    }

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

get_slices(dim, list)
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

/* Modified: 5/8/01 exit code 0 passed on completion by Dewey Odhner. */
/* Modified: 5/13/03 hist12[j] assignment corrected by Dewey Odhner. */
/* Modified: 9/21/05 affine transformation and code extension by Andre Souza. */
main(argc,argv)
  int argc;
  char *argv[];
{
 /* Initialize MPI. */
 MPI_Init(&argc, &argv);

 /* find out my identity in the default communicator */
 MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
 MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
 MPI_Status status;
 double e;
 int i, ii, j, target_info[3], source_info[3], bins_info[2], n, tag, flag;
 FILE *in1, *in2, *out1, *fp_par;
 
 
 if (myrank != 0) {
   

   if (argc > 5)
      {
        sscanf(argv[5], "%d", &interpolation);
        if (interpolation < 0)
           interpolation = DEFAULT_INTERPOLATION;
      }
   
   if (argc > 7)
     {
       sscanf(argv[7], "%hu", &thr1);
       if (thr1 == 0xFFFF)
         thr1 = DEFAULT_THRESHOLD;
     }
   else
     thr1 = DEFAULT_THRESHOLD;
   if (argc > 8)
     {
       sscanf(argv[8], "%hu", &thr2);
       if (thr2 == 0xFFFF)
         thr2 = DEFAULT_THRESHOLD;
     }
   else
      thr2 = DEFAULT_THRESHOLD;

   if (argc > 9)
     {
       sscanf(argv[9], "%d", &shift1);
       if (shift1 < 0)
         shift1 = DEFAULT_SHIFT;
     }
   else
      shift1 = DEFAULT_SHIFT;
   if (argc > 10)
     {
       sscanf(argv[10], "%d", &shift2);
       if (shift2 < 0)
         shift2 = DEFAULT_SHIFT;
     }
   else
        shift2 = DEFAULT_SHIFT;
   if (argc > 11)
      {
       sscanf(argv[11], "%d", &depth);
       if (depth < 0)
          depth = DEFAULT_DEPTH;
      }
   else
       depth = DEFAULT_DEPTH;
   
  if (argv[1][0] == 'm' | argv[1][0] == 'n')
     {
       tag = 0;
       MPI_Recv(bins_info,2, MPI_INT,0,tag,MPI_COMM_WORLD, &status);
       bins1  = bins_info[0];
       bins2  = bins_info[1];       
       
       hist1 =  (int *)malloc((bins1+1)*sizeof(int));
       hist2 =  (int *)malloc((bins2+1)*sizeof(int));
       hist12 = (int *)malloc((bins1+1)*(bins2+1)*sizeof(int));
     }
   while (depth >= 0)
   {
   /*compute slice_chunk*/
   tag = 1;
   MPI_Recv(source_info,3, MPI_INT,0,tag,MPI_COMM_WORLD, &status);
   xdimp1 = source_info[0];
   ydimp1 = source_info[1];
   zdimp1 = source_info[2];
   
   tag = 2;
   MPI_Recv(target_info,3, MPI_INT,0,tag,MPI_COMM_WORLD, &status);
   xdimp2 = target_info[0];
   ydimp2 = target_info[1];
   zdimp2 = target_info[2];   

   r = zdimp2  % (ntasks-1);
   q = (double) zdimp2 / (ntasks-1);
   chunk_slice = myrank < (r + 1) ? ceil(q) : floor(q);
   ssizep = xdimp2 * ydimp2;
   slice_begin = 0;
   for (n = 1; n < myrank; n++)
      { 
          slice_begin += n < (r + 1) ? ceil(q) : floor(q);
      }
   slice_end = slice_begin + chunk_slice - 1;  
   chunk_size = chunk_slice * ssizep;
   chunks2  = (unsigned short *)malloc(chunk_size * 2);
   if (chunks2 == NULL)
     { 
        fprintf(stderr, "Out of memory.\n");
	exit(1);
     }
   pyrs1    = (unsigned short *)malloc(xdimp1 * ydimp1 * zdimp1 * 2);
   if (pyrs1 == NULL)
     {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
     }
   pyrstr1  = (unsigned short *)malloc(chunk_size * 2);
   if (pyrstr1 == NULL)
     {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
     }
   tag = 3;
   MPI_Recv(pyrs1,xdimp1 * ydimp1 * zdimp1,MPI_UNSIGNED_SHORT,0,tag, MPI_COMM_WORLD, &status);
   tag = 4;
   MPI_Recv(chunks2,chunk_size,MPI_UNSIGNED_SHORT,0,tag, MPI_COMM_WORLD, &status);
   while(1)
    { 
      tag = 5;
      MPI_Recv(&flag,1, MPI_INT,0,tag,MPI_COMM_WORLD, &status);
      if (flag == 0)
           break;
     tag = 6;
     MPI_Recv(tr,16,MPI_DOUBLE,0,tag, MPI_COMM_WORLD, &status);
     exec_regist(pyrs1, pyrstr1, xdimp1, ydimp1, zdimp1, xdimp2, ydimp2, chunk_slice,interpolation);
     switch (argv[1][0])
     {
        case 'c':
          e = -CORR();
          tag = 7;
          MPI_Send(&e,1, MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
          break;

        case 'm': case 'n':
          get_hist(pyrstr1, chunks2, chunk_size);
          tag = 7;
          MPI_Send(hist1,bins1 + 1, MPI_INT,0,tag,MPI_COMM_WORLD);
          
          tag = 8;
          MPI_Send(hist2,bins2 + 1, MPI_INT,0,tag,MPI_COMM_WORLD);
          
          tag = 9;
          MPI_Send(hist12,(bins1 + 1) * (bins2 + 1), MPI_INT,0,tag,MPI_COMM_WORLD);
          
          tag = 10;
          MPI_Send(&count,1, MPI_INT,0,tag,MPI_COMM_WORLD);
          
          break;        
       
        case 'k':
          e = -KS();
          tag = 7;
          MPI_Send(&e,1, MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
          break;

        case 'f':
          e = -FS();
          tag = 7;
          MPI_Send(&e,1, MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
          break;

        case 's':
          e = SSD();
          tag = 7;
          MPI_Send(&e,1, MPI_DOUBLE,0,tag,MPI_COMM_WORLD);
          break;


     }  
    }
   depth--;
   free(chunks2);
   free(pyrs1);
   free(pyrstr1);
   }
   exit(0);
 }

 if (myrank == 0) {   

  if (argc < 2)
  {
    printf("\nUsage 1: paffine [s c m n k f] <source> <target> <paramfile> [<interpolation>] [<init_mode> 0: rigid, 1: scale, 2: rigid + scale, 3: shear, 4: rigid + scale + shear, 5: rigid + uniforme scale] [<shift1> <shift2>] [<depth>]\n");    
    
    exit(-1);
  }

  resetTime();

  switch (argv[1][0]) {

/***************/
/* mi matching */
/***************/
  case 'm':
  if (argc < 5)
  {
     exit(-1);
  }

  if (argc > 5)
  {
    sscanf(argv[5], "%d", &interpolation);
    if (interpolation < 0)
      interpolation = DEFAULT_INTERPOLATION;
  }
  else
    interpolation = DEFAULT_INTERPOLATION;

  if (argc > 6)
  {
    sscanf(argv[6], "%d", &init_mode);
    if (init_mode < 0)
      init_mode = DEFAULT_INIT_MODE;
  }
  else
    init_mode = DEFAULT_INIT_MODE;  

  if (argc > 7)
  {
    sscanf(argv[7], "%d", &shift1);
    if (shift1 < 0)
      shift1 = DEFAULT_SHIFT;
  }
  else
    shift1 = DEFAULT_SHIFT;
  if (argc > 8)
  {
    sscanf(argv[8], "%d", &shift2);
    if (shift2 < 0)
      shift2 = DEFAULT_SHIFT;
  }
  else
    shift2 = DEFAULT_SHIFT;

  if (argc > 9)
  {
    sscanf(argv[9], "%d", &depth);
    if (depth < 0)
      depth = DEFAULT_DEPTH;
  }
  else
    depth = DEFAULT_DEPTH;

  printf("\nMI Matching...\n");
  in1 = fopen(argv[2], "rb");
  if (in1 == NULL)
  {
    printf("Error in opening source file 1\n");
    exit(-1);
  }
  in2 = fopen(argv[3], "rb");
  if (in2 == NULL)
  {
    printf("Error in opening target file 2\n");
    exit(-1);
  }
  error = VReadHeader(in1, &vh1, group, elem);
  if (error <= 104) {
    printf("Fatal error in reading source header \n");
    exit(-1);
  }
  error = VReadHeader(in2, &vh2, group, elem);
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
  if (vh1.scn.num_of_bits != 8 && vh1.scn.num_of_bits != 16
   || vh2.scn.num_of_bits != 8 && vh2.scn.num_of_bits != 16)
  {
    printf("Cannot handle %d bit data\n", vh1.scn.num_of_bits);
    exit(-1);
  }

  if (vh1.scn.num_of_bits == 8) 
    bytes1 = 1;
  else
    bytes1 = 2;
  
  if (vh2.scn.num_of_bits == 8) 
    bytes2 = 1;
  else
    bytes2 = 2;


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

  VSeekData(in1, 0);
  VSeekData(in2, 0);

  inc1 = (unsigned char *)malloc(vsize1 * bytes1);
  inc2 = (unsigned char *)malloc(vsize2 * bytes2);

  if (inc1 == NULL || inc2 == NULL)
  {
    printf("Cannot allocate image space\n");
    exit(-1);
  }

  VReadData(inc1, bytes1, vsize1, in1, &j);
  VReadData(inc2, bytes2, vsize2, in2, &j);
  switch (bytes1)
  {
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

  fclose(in1);
  fclose(in2);

  create_log_LUT();

  max1 = get_max(ptrs1, vsize1);
  max2 = get_max(ptrs2, vsize2);
  bins1 = max1 >> shift1;
  bins2 = max2 >> shift2;
  
  bins_info[0] = bins1;
  bins_info[1] = bins2;
  tag = 0;
  for(n = 1; n < ntasks; n++)
      MPI_Send(bins_info,2, MPI_INT,n,tag,MPI_COMM_WORLD);

  hist1  = (int *)malloc((bins1+1)*sizeof(int));
  hist2  = (int *)malloc((bins2+1)*sizeof(int));
  hist12 = (int *)malloc((bins1+1)*(bins2+1)*sizeof(int *));
  
  hists1  = (int *)malloc((bins1+1)*sizeof(int));
  hists2  = (int *)malloc((bins2+1)*sizeof(int));
  hists12 = (int *)malloc((bins1+1)*(bins2+1)*sizeof(int *));
  
  printf("Image size source: %dx%dx%d - %lfx%lfx%lf\n", xdim1, ydim1, zdim1, voxelsize_x1, voxelsize_y1, voxelsize_z1);
  printf("Image size target: %dx%dx%d - %lfx%lfx%lf\n", xdim2, ydim2, zdim2, voxelsize_x2, voxelsize_y2, voxelsize_z2);
  
 if (init_mode == 0) {
      rigid  = 1;
      scale  = 0;
      scale1 = 0; 
      shear  = 0;
     }

  if (init_mode == 1) {
      rigid  = 0;
      scale  = 1;
      scale1 = 0;
      shear  = 0;
     }
  
  if (init_mode == 2) {
      rigid  = 1;
      scale  = 1;
      scale1 = 0;
      shear  = 0;
     }

  if (init_mode == 3) {
      rigid  = 0;
      scale  = 0;
      scale1 = 0;
      shear  = 1;
     }
  
  if (init_mode == 4) {
      rigid  = 1;
      scale  = 1;
      scale1 = 0;
      shear  = 1;
     } 

  if (init_mode == 5) {
      rigid  = 1;
      scale  = 1;
      scale1 = 1;
      shear  = 0;
     } 

  find_mi_trf(depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);
  
  if ((fp_par = fopen(argv[4], "wb")) == NULL)
  {
    printf("ERROR: Can't open the PARAMETER file !\n");
    exit(-1);
  }
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
  fclose(fp_par);
  break;


/******************************/
/* corr, ks, fs, ssd matching */
/******************************/
  case 'c': case 'f': case 'k': case 's':
  if (argc < 5)
  {
     exit(-1);
  }

    if (argc > 5)
  {
    sscanf(argv[5], "%d", &interpolation);
    if (interpolation < 0)
      interpolation = DEFAULT_INTERPOLATION;
  }
  else
    interpolation = DEFAULT_INTERPOLATION;

  if (argc > 6)
  {
    sscanf(argv[6], "%d", &init_mode);
    if (init_mode < 0)
      init_mode = DEFAULT_INIT_MODE;
  }
  else
    init_mode = DEFAULT_INIT_MODE;  

  if (argc > 7)
  {
    sscanf(argv[7], "%d", &shift1);
    if (shift1 < 0)
      shift1 = DEFAULT_SHIFT;
  }
  else
    shift1 = DEFAULT_SHIFT;
  if (argc > 8)
  {
    sscanf(argv[8], "%d", &shift2);
    if (shift2 < 0)
      shift2 = DEFAULT_SHIFT;
  }
  else
    shift2 = DEFAULT_SHIFT;

  if (argc > 9)
  {
    sscanf(argv[9], "%d", &depth);
    if (depth < 0)
      depth = DEFAULT_DEPTH;
  }
  else
    depth = DEFAULT_DEPTH;

  printf("\nMatching...\n");
  in1 = fopen(argv[2], "rb");
  if (in1 == NULL)
  {
    printf("Error in opening source file 1\n");
    exit(-1);
  }
  in2 = fopen(argv[3], "rb");
  if (in2 == NULL)
  {
    printf("Error in opening target file 2\n");
    exit(-1);
  }
  error = VReadHeader(in1, &vh1, group, elem);
  if (error <= 104) {
    printf("Fatal error in reading source header \n");
    exit(-1);
  }
  error = VReadHeader(in2, &vh2, group, elem);
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
  if (vh1.scn.num_of_bits != 8 && vh1.scn.num_of_bits != 16
   || vh2.scn.num_of_bits != 8 && vh2.scn.num_of_bits != 16)
  {
    printf("Cannot handle %d bit data\n", vh1.scn.num_of_bits);
    exit(-1);
  }

  if (vh1.scn.num_of_bits == 8) 
    bytes1 = 1;
  else
    bytes1 = 2;
  
  if (vh2.scn.num_of_bits == 8) 
    bytes2 = 1;
  else
    bytes2 = 2;


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

  VSeekData(in1, 0);
  VSeekData(in2, 0);

  inc1 = (unsigned char *)malloc(vsize1 * bytes1);
  inc2 = (unsigned char *)malloc(vsize2 * bytes2);

  if (inc1 == NULL || inc2 == NULL)
  {
    printf("Cannot allocate image space\n");
    exit(-1);
  }

  VReadData(inc1, bytes1, vsize1, in1, &j);
  VReadData(inc2, bytes2, vsize2, in2, &j);
  switch (bytes1)
  {
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

  fclose(in1);
  fclose(in2);

  printf("Image size source: %dx%dx%d - %lfx%lfx%lf\n", xdim1, ydim1, zdim1, voxelsize_x1, voxelsize_y1, voxelsize_z1);
  printf("Image size target: %dx%dx%d - %lfx%lfx%lf\n", xdim2, ydim2, zdim2, voxelsize_x2, voxelsize_y2, voxelsize_z2);
  
 if (init_mode == 0) {
      rigid  = 1;
      scale  = 0;
      scale1 = 0; 
      shear  = 0;
     }

  if (init_mode == 1) {
      rigid  = 0;
      scale  = 1;
      scale1 = 0;
      shear  = 0;
     }
  
  if (init_mode == 2) {
      rigid  = 1;
      scale  = 1;
      scale1 = 0;
      shear  = 0;
     }

  if (init_mode == 3) {
      rigid  = 0;
      scale  = 0;
      scale1 = 0;
      shear  = 1;
     }
  
  if (init_mode == 4) {
      rigid  = 1;
      scale  = 1;
      scale1 = 0;
      shear  = 1;
     } 

  if (init_mode == 5) {
      rigid  = 1;
      scale  = 1;
      scale1 = 1;
      shear  = 0;
     } 

  find_scalar_trf(depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);
 
  if ((fp_par = fopen(argv[4], "wb")) == NULL)
  {
    printf("ERROR: Can't open the PARAMETER file !\n");
    exit(-1);
  }
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
  fclose(fp_par);
  break;

/***************/
/* nmi matching */
/***************/
  case 'n':
  if (argc < 5)
  {
     exit(-1);
  }

  if (argc > 5)
  {
    sscanf(argv[5], "%d", &interpolation);
    if (interpolation < 0)
      interpolation = DEFAULT_INTERPOLATION;
  }
  else
    interpolation = DEFAULT_INTERPOLATION;

  if (argc > 6)
  {
    sscanf(argv[6], "%d", &init_mode);
    if (init_mode < 0)
      init_mode = DEFAULT_INIT_MODE;
  }
  else
    init_mode = DEFAULT_INIT_MODE;  

  if (argc > 7)
  {
    sscanf(argv[7], "%d", &shift1);
    if (shift1 < 0)
      shift1 = DEFAULT_SHIFT;
  }
  else
    shift1 = DEFAULT_SHIFT;
  if (argc > 8)
  {
    sscanf(argv[8], "%d", &shift2);
    if (shift2 < 0)
      shift2 = DEFAULT_SHIFT;
  }
  else
    shift2 = DEFAULT_SHIFT;

  if (argc > 9)
  {
    sscanf(argv[9], "%d", &depth);
    if (depth < 0)
      depth = DEFAULT_DEPTH;
  }
  else
    depth = DEFAULT_DEPTH;

  printf("\nNMI Matching...\n");
  in1 = fopen(argv[2], "rb");
  if (in1 == NULL)
  {
    printf("Error in opening source file 1\n");
    exit(-1);
  }
  in2 = fopen(argv[3], "rb");
  if (in2 == NULL)
  {
    printf("Error in opening target file 2\n");
    exit(-1);
  }
  error = VReadHeader(in1, &vh1, group, elem);
  if (error <= 104) {
    printf("Fatal error in reading source header \n");
    exit(-1);
  }
  error = VReadHeader(in2, &vh2, group, elem);
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
  if (vh1.scn.num_of_bits != 8 && vh1.scn.num_of_bits != 16
   || vh2.scn.num_of_bits != 8 && vh2.scn.num_of_bits != 16)
  {
    printf("Cannot handle %d bit data\n", vh1.scn.num_of_bits);
    exit(-1);
  }

  if (vh1.scn.num_of_bits == 8) 
    bytes1 = 1;
  else
    bytes1 = 2;
  
  if (vh2.scn.num_of_bits == 8) 
    bytes2 = 1;
  else
    bytes2 = 2;


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

  VSeekData(in1, 0);
  VSeekData(in2, 0);

  inc1 = (unsigned char *)malloc(vsize1 * bytes1);
  inc2 = (unsigned char *)malloc(vsize2 * bytes2);

  if (inc1 == NULL || inc2 == NULL)
  {
    printf("Cannot allocate image space\n");
    exit(-1);
  }

  VReadData(inc1, bytes1, vsize1, in1, &j);
  VReadData(inc2, bytes2, vsize2, in2, &j);
  switch (bytes1)
  {
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

  fclose(in1);
  fclose(in2);

  create_log_LUT();

  max1 = get_max(ptrs1, vsize1);
  max2 = get_max(ptrs2, vsize2);
  bins1 = max1 >> shift1;
  bins2 = max2 >> shift2;
  
  bins_info[0] = bins1;
  bins_info[1] = bins2;
  tag = 0;
  for(n = 1; n < ntasks; n++)
      MPI_Send(bins_info,2, MPI_INT,n,tag,MPI_COMM_WORLD);

  hist1  = (int *)malloc((bins1+1)*sizeof(int));
  hist2  = (int *)malloc((bins2+1)*sizeof(int));
  hist12 = (int *)malloc((bins1+1)*(bins2+1)*sizeof(int *));
  
  hists1  = (int *)malloc((bins1+1)*sizeof(int));
  hists2  = (int *)malloc((bins2+1)*sizeof(int));
  hists12 = (int *)malloc((bins1+1)*(bins2+1)*sizeof(int *));
  
  printf("Image size source: %dx%dx%d - %lfx%lfx%lf\n", xdim1, ydim1, zdim1, voxelsize_x1, voxelsize_y1, voxelsize_z1);
  printf("Image size target: %dx%dx%d - %lfx%lfx%lf\n", xdim2, ydim2, zdim2, voxelsize_x2, voxelsize_y2, voxelsize_z2);
  
 if (init_mode == 0) {
      rigid  = 1;
      scale  = 0;
      scale1 = 0; 
      shear  = 0;
     }

  if (init_mode == 1) {
      rigid  = 0;
      scale  = 1;
      scale1 = 0;
      shear  = 0;
     }
  
  if (init_mode == 2) {
      rigid  = 1;
      scale  = 1;
      scale1 = 0;
      shear  = 0;
     }

  if (init_mode == 3) {
      rigid  = 0;
      scale  = 0;
      scale1 = 0;
      shear  = 1;
     }
  
  if (init_mode == 4) {
      rigid  = 1;
      scale  = 1;
      scale1 = 0;
      shear  = 1;
     } 

  if (init_mode == 5) {
      rigid  = 1;
      scale  = 1;
      scale1 = 1;
      shear  = 0;
     } 
 
  find_nmi_trf(depth, ptrs1, ptrs2, xdim1, ydim1, zdim1, xdim2, ydim2, zdim2);
  
  if ((fp_par = fopen(argv[4], "wb")) == NULL)
  {
    printf("ERROR: Can't open the PARAMETER file !\n");
    exit(-1);
  }
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
  fclose(fp_par);
  break;

  }
  printf("process %d done.\n", myrank);
  printf("\nTotal computation last:%f seconds\n", getElapsedTime());
  /* Shut down MPI */  
  MPI_Finalize();
 }
}/*main end*/
