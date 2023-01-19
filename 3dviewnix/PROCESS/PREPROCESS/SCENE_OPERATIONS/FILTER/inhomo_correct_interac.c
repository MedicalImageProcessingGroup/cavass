/*
  Copyright 1993-2014, 2017, 2021 Medical Image Processing Group
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
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <cv3dv.h>

/**********************************************************************************/
#define VOLUME_THRESH 6
#define ZERO_VALUE 0.00001
#define MAX_INTERVAL 4 /* maximum number of intensity masks allowed for correction */

#define SQR(a) ((a)*(a))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

#define FMAX(a,b) (maxarg1=(a),maxarg2=(b), maxarg1 > maxarg2 ?\
        maxarg1 : maxarg2)

/*************************************************************************************/
typedef struct
{
  short x,y,z;
} voxelS;

typedef struct 
{
  short x,y,z;
  double inhomo_value;
} voxelElem;

typedef struct
{
  int num_voxels;
  voxelElem *pStart;
} regionElem;


static double maxarg1,maxarg2;

int GenerateLabels(char *dt, int w, int h, int s),
  get_factor_nD(voxelElem *pElem, long int mask_volume, double factor[]),
  compute_3Ddistance(unsigned short out_buffer[]), get_factor_1D(double cof[]);
void destroy_scene_header(ViewnixHeader *vh);

unsigned int *tbz,*tby; /* Multiplication tables for slices and rows */

int push=0;
int tot_dt=0;
int pre_tot_dt=0;
int _t =0;
char *dt_8;
int cur_mask;

voxelS *Stack;      // define stack;
regionElem *pSort,*pRegion;

voxelElem *pGlobal;


/*****************************************************************************************/
/**************************** for function optimization **********************************/
#define GOLD 1.618034
#define GLIMIT 100.0
#define TINY 1.0e-20
#define ITMAX 100
#define CGOLD 0.3819660
#define ZEPS 1.0e-10
#define TOL 2.0e-4

int ncom;
double *pcom,*xicom;
double (*nrfunc)(double a[]);

#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);

/*****************************************************************************************/

unsigned short *data16;
unsigned char *data1, *data8;
float *data_f;

unsigned short *dis_map1,*dis_map2;

int height,width,slices,space,num_of_bits,slice_size,volume_size,size_bin;
float pxsize,pysize;
int num_sets;   // total object number
double **aV, **dir;
int iteration;
double factor[66];
double factor_1[6];


double function_value_nD(double a[]);
double function_value_1D(double x);
int powell(double p[],double **xi,int n, double ftol,int *iter, double *fret, double (*func)(double a[]));
double brent(double ax,double bx,double cx,double (*f)(double),double tol,double *xmin);
int mnbrak(double *ax, double *bx,double *cx, double *fa, double *fb,double *fc,double (*func)(double));
int FindObject(char *dt, int w, int h,int s,short startx,short starty,short startz);

/******************************************************************************************/

double *vector(int n)
{
        double *v;

        v = (double *)malloc(n*sizeof(*v));
        if (v == NULL)
        {
                fprintf(stderr, "Out of memory.\n");
                exit(1);
        }
        return v;
}
/******************************************************************************************/

int main(argc,argv)
int argc;
char *argv[];
{
  int i,j,k, ii,jj, error,tti1,tti2;
  int xx,yy,zz;
  double tt1,tt2;
  ViewnixHeader vh, vh_mask;
  FILE *in,*out;
  unsigned short value16;

  char group[6], elem[6];
  int pars;
  char *datafile, *mask_name[MAX_INTERVAL];
  char *outfile;
  
  double mean_value[MAX_INTERVAL];  
  double sum;
  long int volume[MAX_INTERVAL],sum_volume;

  int num_mask = argc-3;

  voxelElem *pLocation;
  regionElem *pTemp;
  regionElem swap;

  double freturn, ftol=1.0e-5;

  double ax1,bx1,xx1,fa1,fb1,fx1,xmin;

  double cof_t[10],sum_t[10];

  double fmin,fmax,old_min,old_max;

  if (argc < 4) {
    fprintf(stderr,"Usage: inhomo_correct_interac <Data_file> <Outfile> <mask_file>...\n"); 
    exit(-1);
  }

  if (num_mask > MAX_INTERVAL)
  {
    fprintf(stderr, "too many masks\n");
	exit(-1);
  }

  pars = 1;
  datafile = argv[pars++];
  outfile = argv[pars++];

  for( i=0; i<num_mask; i++)
    {
      mask_name[i] = argv[pars++];
    }

  /********* open original file **********/
  in = fopen(datafile,"rb");
  if (in==NULL) {
    fprintf(stderr,"Error in opening the files\n");
    exit(-1);
  }
  
  error=VReadHeader(in,&vh,group,elem);
  if (error<=104) {
    fprintf(stderr,"Fatal error in reading header\n");
    exit(-1);
  }
  fclose(in);

  num_of_bits = vh.scn.num_of_bits;
  slices = vh.scn.num_of_subscenes[0];
  width =  vh.scn.xysize[0];
  height = vh.scn.xysize[1];
  pxsize = vh.scn.xypixsz[0];
  pysize = vh.scn.xypixsz[1];

  slice_size = height*width;
  volume_size = slices*slice_size;
  space = (int)(vh.scn.loc_of_subscenes[1] - vh.scn.loc_of_subscenes[0]);  /* assume equal slice spacing */

  old_min = vh.scn.smallest_density_value[0]; /* in case for intensity normalization */
  old_max = vh.scn.largest_density_value[0];

  data1 = (unsigned char *)malloc((slice_size+7)/8);

  if(num_of_bits == 8)
    {
      data8 = (unsigned char *)malloc(volume_size*sizeof(char));
      if(data8==NULL)
        {
          fprintf(stderr,"Memory allocation error \n");
          exit(-1);
        }
      in = fopen(datafile,"rb");
      if (in == NULL)
        {
          fprintf(stderr,"Fatal error in opening file\n");
          exit(-1);
        }
      VSeekData(in, 0);
      error = VReadData((char *)data8, 1, volume_size, in, &j);  
      fclose(in);
    }
  else if(num_of_bits == 16)
    {
      data16 = (unsigned short *)malloc(volume_size*sizeof(short));
      if(data16==NULL)
        {
          fprintf(stderr,"Memory allocation error \n");
          exit(-1);
        }
      in = fopen(datafile,"rb");
      if (in == NULL)
        {
          fprintf(stderr,"Fatal error in opening file\n");
          exit(-1);
        }
      VSeekData(in, 0);
      error = VReadData((char *)data16, 2, volume_size, in, &j);  
      fclose(in);
    }

  /* log transform to original data */

  data_f = (float *)malloc(volume_size*sizeof(float));

  /* allocate memory for the correction procedure */
  dt_8 = (char *)calloc(volume_size, 1);

  // store all connected objects
  pGlobal = (voxelElem *)malloc(volume_size*sizeof(voxelElem));
  if(pGlobal == NULL)
    {
      fprintf(stderr,"Error allocating memory\n");
      exit(-1);
    }

  /* Build Multiplication tables */
  tbz=(unsigned int *)malloc(sizeof(int)*slices);
  tby=(unsigned int *)malloc(sizeof(int)*vh.scn.xysize[1]);
  if (tbz==NULL || tby==NULL) 
    {
      fprintf(stderr,"Error in allocating data\n");
      exit(-1);
    }
  for(j=vh.scn.xysize[0],i=0;i<vh.scn.xysize[1];i++)
    tby[i]=i*j;
  
  for(j=vh.scn.xysize[0]*vh.scn.xysize[1],i=0;i<slices;i++)
    tbz[i]=i*j;


  Stack = (voxelS *)malloc(volume_size*sizeof(voxelS));  /* used in stack operation like push and pop */
  pRegion = (regionElem *)malloc(sizeof(regionElem)*num_mask);

  aV = (double **)malloc(num_mask*sizeof(double*));
  aV[0] = (double*)malloc(num_mask*10*sizeof(double));
  for(i = 0;i<num_mask;i++)
    aV[i] = aV[0] + i*10;

  dir = (double **)malloc(10*sizeof(double *));
  dir[0] = (double *)malloc(10*10*sizeof(double));
  for(i = 0;i<10;i++)
    dir[i] = dir[0] + i*10;
  dis_map1 =(unsigned short*) malloc((width+2)*(height+2)*(slices+2)*sizeof(unsigned short));
  dis_map2 = (unsigned short*) malloc((width+2)*(height+2)*(slices+2)*sizeof(unsigned short));

  if((dis_map1 == NULL)||(dis_map2 == NULL))
    {
      fprintf(stderr,"Error in allocating data\n");
      exit(-1);
    }

  for (j=0; j<9; j++)
    aV[0][j] = 0.0;
  aV[0][9] = 1.0;

  for(j = 0;j<volume_size;j++)
    if(num_of_bits == 8)
      data_f[j] = (float)data8[j];
    else if(num_of_bits == 16)
      data_f[j] = (float)data16[j];

  /* remove inhomogeneity and obtain corrected scene */
  for(zz = 0;zz<slices;zz++)
    for(yy = 0;yy<height;yy++)
      for(xx = 0;xx<width;xx++)
        {
          tti1 = zz*height*width + yy*width + xx;
          tt1 = aV[0][0]*xx*xx + aV[0][1]*yy*yy + aV[0][2]*zz*zz + aV[0][3]*xx*yy +
            aV[0][4]*yy*zz + aV[0][5]*xx*zz + aV[0][6]*xx + aV[0][7]*yy + aV[0][8]*zz + aV[0][9];

          data_f[tti1] = (float)(data_f[tti1]/tt1);
        }

  sum_volume = 0;

  pre_tot_dt = tot_dt = 0; //moved from findobject, only for removing those small components less than certain #

  for(ii = 0;ii<num_mask;ii++)
    {
      in = fopen(mask_name[ii], "rb");
      if (in == NULL)
      {
        fprintf(stderr,"Error in opening the files\n");
        exit(-1);
      }
      error = VReadHeader(in, &vh_mask, group,elem);
      if (error && error<=105)
      {
        fprintf(stderr,"Fatal error in reading header\n");
        exit(-1);
      }
      if (vh_mask.scn.xysize[0]!=width || vh_mask.scn.xysize[1]!=height ||
          vh_mask.scn.num_of_bits!=1 || vh_mask.scn.dimension!=vh.scn.dimension
          || vh_mask.scn.num_of_subscenes[0]>vh.scn.num_of_subscenes[0])
      {
        fprintf(stderr, "Mask file does not match Data_file.\n");
        exit(-1);
      }
      VSeekData(in, 0);

      pRegion[ii].pStart = pGlobal+sum_volume;
      assert(pRegion[ii].pStart==pGlobal ||
        pRegion[ii].pStart==pRegion[ii-1].pStart+pRegion[ii-1].num_voxels);
      for(zz=0;zz<slices;zz++)
      {
        if (zz >= vh_mask.scn.num_of_subscenes[0])
        {
          memset(dt_8+zz*slice_size, 0, slice_size);
          continue;
        }
        error = VReadData((char *)data1, 1, (slice_size+7)/8, in, &j);
        if (error)
        {
          fprintf(stderr,"Fatal error in reading data\n");
          exit(-1);
        }
        for(yy=0;yy<height;yy++)
          for(xx = 0;xx<width;xx++)
            {
              j = yy*width+xx;
              tti1 = zz*slice_size+yy*width+xx;
              if (data1[j/8] & (128>>(j%8)))
                dt_8[tti1] = 1;
              else
                dt_8[tti1] = 0;
            }
      }
      fclose(in);
      destroy_scene_header(&vh_mask);

      // get rid of those components less than certain volume
      volume[ii] = GenerateLabels(dt_8,width,height,slices);
      printf("Total voxel number of object %d : %d\n\n",ii,(int)volume[ii]);

      pRegion[ii].num_voxels = volume[ii];
      sum_volume = sum_volume + volume[ii];
    }
  num_sets = num_mask;

  pSort = pRegion;
  for(i = 0;i<num_sets-1;i++)
    {
      k = i;
      for(j = i+1;j<num_sets;j++)
        if(pSort[j].num_voxels > pSort[k].num_voxels)
          k = j;
      swap = pSort[i];
      pSort[i] = pSort[k];
      pSort[k] = swap;
    }

  //compute average intensity value of each region

  pTemp = pSort;
  for(i=0;i<num_sets;i++)
    {
      pLocation = pTemp->pStart;
      tti1 = pTemp->num_voxels;
      sum = 0.0;
      for(k=0;k<tti1;k++)
        {
          xx = pLocation->x;
          yy = pLocation->y;
          zz = pLocation->z;
          tti2 = zz*slice_size + yy*width + xx;
          sum = sum + data_f[tti2];
          pLocation++;
        }
      if (tti1 == 0)
        {
          fprintf(stderr, "Object %d mean undefined\n", i);
          exit(-1);
        }
      mean_value[i] = sum /(double)tti1;
      printf("Object %d mean: %lf \n", i, mean_value[i]);
      pTemp++;
    }

  for(j = 0;j<volume_size;j++)
    if(num_of_bits == 8)
      data_f[j] = (float)data8[j];
    else if(num_of_bits == 16)
      data_f[j] = (float)data16[j];

  pTemp = pSort;
  for(i=0;i<num_sets;i++)
    {
      pLocation = pTemp->pStart;
      tti1 = pTemp->num_voxels;

      pLocation = pTemp->pStart;
      for(k=0;k<tti1;k++)
        {
          xx = pLocation->x;
          yy = pLocation->y;
          zz = pLocation->z;
          tti2 = zz*slice_size + yy*width + xx;

          tt1 = data_f[tti2]/mean_value[i];

          pLocation->inhomo_value = tt1;
          pLocation++;
        }
      pTemp++;
    }

  //-----------------------------------------------------------------------
  // for each region, compute the n-Dimenensional optimization function
  // return value is kept in aV[i]
  for (i=1; i<num_sets; i++)
    for(j=0; j<10; j++)
      aV[i][j] = aV[0][j];
  pTemp = pSort;
  for(i = 0;i<num_sets;i++)
    {
      for(ii=0;ii<10;ii++)
        for(jj=0;jj<10;jj++)
          if(jj==ii)
            dir[ii][jj]= 1.0;
          else
            dir[ii][jj]= 0.0;

      get_factor_nD(pTemp->pStart,pTemp->num_voxels,factor);
      pTemp++;

      powell(aV[i],dir,10,ftol,&iteration,&freturn, &function_value_nD);

      printf("The region %d's factors are: \n",i);
      for(j = 0;j<10;j++)
        printf("%4f ",aV[i][j]);
      printf("\n");
    }

  //-----------------------------------------------------------------------
  // keep balance among all regions

  pTemp = pSort;
  for(i = 1;i<num_sets;i++)
    {
      memset(dis_map1,255, 2*(width+2)*(height+2)*(slices+2));
      memset(dis_map2,255, 2*(width+2)*(height+2)*(slices+2));

      for(j = 0;j<i; j++ )
        {
          // from 0 to i-1 th regions, initialized for 3D distance transform
          pLocation = pSort[j].pStart;
          tti1 = pSort[j].num_voxels;
          for(k=0;k<tti1;k++)
            {
              xx = pLocation->x;
              yy = pLocation->y;
              zz = pLocation->z;
              tti2 = (zz+1)*(width+2)*(height+2) + (yy+1)*(width+2) + xx+1;
              dis_map1[tti2] = 0;
              pLocation++;
            }
          pTemp++;
        }

      compute_3Ddistance(dis_map1);

      // the ith region, initialized for 3D distance transform
      pLocation = pSort[i].pStart;
      tti1 = pSort[i].num_voxels;
      for(j=0;j<tti1;j++)
        {
          xx = pLocation->x;
          yy = pLocation->y;
          zz = pLocation->z;
          tti2 = (zz+1)*(width+2)*(height+2) + (yy+1)*(width+2) + xx+1;
          dis_map2[tti2] = 0;
          pLocation++;
        }
      compute_3Ddistance(dis_map2);

      get_factor_1D(aV[i]);

      ax1 = 0.0;
      xx1 = 1.0;
      mnbrak(&ax1,&xx1,&bx1,&fa1,&fx1,&fb1,function_value_1D);
      freturn = brent(ax1,xx1,bx1,function_value_1D,TOL,&xmin);

      printf("The weight factor for the %d polynomial is %4f \n", i, xmin);
      //xmin is what we need

      for(j = 0;j<10;j++)
        {
          cof_t[j]=0;
          sum_t[j]=0;
        }

      for(zz = 0;zz < slices;zz++)
        for(yy = 0;yy < height;yy++)
          for(xx = 0;xx < width; xx++)
            {
              cof_t[0] = cof_t[0] + xx*xx;
              cof_t[1] = cof_t[1] + yy*yy;
              cof_t[2] = cof_t[2] + zz*zz;
              cof_t[3] = cof_t[3] + xx*yy;
              cof_t[4] = cof_t[4] + yy*zz;
              cof_t[5] = cof_t[5] + xx*zz;
              cof_t[6] = cof_t[6] + xx;
              cof_t[7] = cof_t[7] + yy;
              cof_t[8] = cof_t[8] + zz;
              cof_t[9] = cof_t[9] + 1;
            }
      for(zz = 0;zz < slices;zz++)
        for(yy = 0;yy < height;yy++)
          for(xx = 0;xx < width; xx++)
            {
              tti2 = (zz+1)*(height+2)*(width+2) + (yy+1)*(width+2) + xx + 1;
              tt1 = dis_map1[tti2];
              tt2 = dis_map2[tti2];

              if(tt1<tt2)
                {
                  sum_t[0] = sum_t[0] + xx*xx*aV[0][0];
                  sum_t[1] = sum_t[1] + yy*yy*aV[0][1];
                  sum_t[2] = sum_t[2] + zz*zz*aV[0][2];
                  sum_t[3] = sum_t[3] + xx*yy*aV[0][3];
                  sum_t[4] = sum_t[4] + yy*zz*aV[0][4];
                  sum_t[5] = sum_t[5] + xx*zz*aV[0][5];
                  sum_t[6] = sum_t[6] + xx*aV[0][6];
                  sum_t[7] = sum_t[7] + yy*aV[0][7];
                  sum_t[8] = sum_t[8] + zz*aV[0][8];
                  sum_t[9] = sum_t[9] + aV[0][9];
                }
              else
                {
                  sum_t[0] = sum_t[0] + xx*xx*aV[i][0]*xmin;
                  sum_t[1] = sum_t[1] + yy*yy*aV[i][1]*xmin;
                  sum_t[2] = sum_t[2] + zz*zz*aV[i][2]*xmin;
                  sum_t[3] = sum_t[3] + xx*yy*aV[i][3]*xmin;
                  sum_t[4] = sum_t[4] + yy*zz*aV[i][4]*xmin;
                  sum_t[5] = sum_t[5] + xx*zz*aV[i][5]*xmin;
                  sum_t[6] = sum_t[6] + xx*aV[i][6]*xmin;
                  sum_t[7] = sum_t[7] + yy*aV[i][7]*xmin;
                  sum_t[8] = sum_t[8] + zz*aV[i][8]*xmin;
                  sum_t[9] = sum_t[9] + aV[i][9]*xmin;
                }
            }

      for (j=0; j<10; j++)
        aV[0][j] = sum_t[j]/cof_t[j];
    }

  printf("The final factors are: \n");
  for(i = 0;i<10;i++)
    printf("%4f ",aV[0][i]);
  printf("\n");

  for(j = 0;j<volume_size;j++)
    if(num_of_bits == 8)
      data_f[j] = (float)data8[j];
    else if(num_of_bits == 16)
      data_f[j] = (float)data16[j];

  /* remove inhomogeneity and obtain corrected scene */
  fmin = 65535.0;fmax = 0.0;
  for(zz = 0;zz<slices;zz++)
    for(yy = 0;yy<height;yy++)
      for(xx = 0;xx<width;xx++)
        {
          tti1 = zz*height*width + yy*width + xx;
          tt1 = aV[0][0]*xx*xx + aV[0][1]*yy*yy + aV[0][2]*zz*zz + aV[0][3]*xx*yy +
            aV[0][4]*yy*zz + aV[0][5]*xx*zz + aV[0][6]*xx + aV[0][7]*yy + aV[0][8]*zz + aV[0][9];
          data_f[tti1] = (float)(data_f[tti1]/tt1);
          if(data_f[tti1] > fmax ) fmax = data_f[tti1];
          if(data_f[tti1] < fmin ) fmin = data_f[tti1];
        }
  printf("Min %f and max %f intensity of corrected scene.\n", fmin, fmax);

  /*shift intensity scale if minimum intensity is negative */
  if (fmin<0 || fmax>65535)
    {
      for (j=0; j<volume_size; j++)
        {
          if (data_f[j] < 0)
            data_f[j] = 0;
          if (data_f[j] > 65535)
            data_f[j] = 65535;
        }
      if (fmin < 0)
        fmin = 0;
      if (fmax > 65535)
        fmax = 65535;
    }

  /* write the corrected images */
  out=fopen(outfile,"wb");

  vh.scn.smallest_density_value[0] = (float)fmin;
  vh.scn.largest_density_value[0] = (float)fmax;
  if(fmin<0)
    {
      vh.scn.smallest_density_value[0] = 0;
      vh.scn.largest_density_value[0] = (float)(fmax-fmin);
    }

  vh.scn.largest_density_value_valid = 1;
  vh.scn.smallest_density_value_valid = 1;

  vh.scn.num_of_bits=16;
  if (vh.scn.bit_fields_valid) vh.scn.bit_fields[1] = num_of_bits-1;

  error=VWriteHeader(out,&vh,group,elem);
  if (error<=104)
    {
      fprintf(stderr,"Fatal error in reading header\n");
      exit(-1);
    }
  for(i = 0;i<volume_size;i++)
    {
      value16 = (unsigned short)(data_f[i]+0.5);
      VWriteData((char *)&value16,sizeof(short),1,out,&j);
    }
  fclose(out);

  /* end of writting of corrected image */

  /* free memory used for correction */
  free(Stack);
  free(dt_8);
  free(pGlobal);
  free(pSort);

  free(tby);
  free(tbz);

  free(dis_map1);
  free(dis_map2);


  if(num_of_bits == 8)
    free(data8);
  else if(num_of_bits == 16)
    free(data16);
  free(data_f);
  free(data1);

  printf("program exit normally \n");
  return 0;
}

/********************************************************************************************
 * FUNCTION: GenerateLabels
 * DESCRIPTION:
 * PARAMETERS:
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 * 
 * 
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 07/07/00 by Tad
 *    Modified : 11/13/00 by Ying Zhuge
 *               removed the flag -1 between two adjacent regions
 *               sorting regions in terms of the voxel number of region
 *
 *******************************************************************************************/
int GenerateLabels(char *dt, int w, int h, int s)
{
  short x,y,z;
  int nSets,component_volume, sum_voxel=0;
  char *pt;

  regionElem* pTemp;

  nSets=0;

  pTemp = pRegion;
  pt = dt;

  for(z=0;z<s;z++)
    for(y=0;y<h;y++)
      for(x=0;x<w;x++)
        {
          if (*pt==1)
            {
              nSets++;
      
              component_volume = FindObject(dt,w,h,s,x,y,z);

              if (component_volume < VOLUME_THRESH)
                {
                  nSets--;
                  tot_dt=pre_tot_dt;
                  push = 0;
                }
              else
                {
                  printf("Component# %-5d Volume %-6d \n",nSets,component_volume);
  
                  sum_voxel = sum_voxel + component_volume;
  
                  pre_tot_dt = tot_dt;
                  push = 0;
                }
            }
          pt++;
        }

  return sum_voxel;
}

//********************************************************************************************
//********************************************************************************************
void PushStack(char *d, short x, short y, short z, int* total_num)
{
  voxelElem voxel;
   
   *total_num =  *total_num + 1;
   _t=tbz[z] + tby[y] + x; 
   voxel.x = x;
   voxel.y = y;
   voxel.z = z;
   voxel.inhomo_value = 0;
   d[_t] = 0;           //The point will never be selected again
   pGlobal[tot_dt++] = voxel;
   Stack[push].x=x;
   Stack[push].y=y; 
   Stack[push].z=z; 
   push++;

 }


//********************************************************************************************
//********************************************************************************************
void PopStack(voxelS* p)
{ 
  push--;
  *p = Stack[push]; 

}

/********************************************************************************************
 * FUNCTION: FindObject
 * DESCRIPTION:
 * PARAMETERS:
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *     
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 07/07/00 by Tad
 *    Modified: Take labeled components as input to get connected regions. by Ying Zhuge 6/2/04
 *
 *******************************************************************************************/
int FindObject(char *dt, int w, int h,int s,short startx,short starty,short startz)
{
  int  nsum;
  short x,y,z;
  voxelS retP;
 
  nsum=0;
  x = startx;
  y = starty;
  z = startz;
  PushStack(dt,x,y,z,&nsum);

    while (push != 0) {
      PopStack(&retP);
      x = retP.x;
      y = retP.y;
      z = retP.z;

      /* Face Connected */

      if (x!=0 && dt[tbz[z]+tby[y]+x-1]==1) 
        PushStack(dt,x-1,y,z,&nsum);
      if (x!=(w-1) && dt[tbz[z]+tby[y]+x+1]==1)
        PushStack(dt,x+1,y,z,&nsum);
      if (y!=0 && dt[tbz[z]+tby[y-1]+x]==1)
        PushStack(dt,x,y-1,z,&nsum);
      if (y!=(h-1) && dt[tbz[z]+tby[y+1]+x]==1)
        PushStack(dt,x,y+1,z,&nsum);
      if (z!=0 && dt[tbz[z-1]+tby[y]+x]==1)
        PushStack(dt,x,y,z-1,&nsum);
      if (z!=(s-1) && dt[tbz[z+1]+tby[y]+x]==1)
        PushStack(dt,x,y,z+1,&nsum);
    }

   return nsum;
}


/****************************************************************************************************
 * FUNCTION: bin_to_grey16
 * DESCRIPTION:
 * PARAMETERS:
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *     
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 07/07/00 by Tad
 *
 ***************************************************************************************************/
void bin_to_grey16(unsigned char *bin_buffer, int length, char *grey_buffer, int min_value,int max_value)
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin;
  char *grey;
  int c;

  bin = bin_buffer;
  grey = grey_buffer;
  
  c=0;
  for(j=7,i=length; i>0; i--)    {
    if ( (*bin & mask[j]) )  {
      *grey = max_value;
      c++;
    }
    else 
      *grey = min_value;
    
    grey++;

    if (j==0) {
      bin++; j=7;
    }
    else
      j--;
  }
}
/**********************************************************************************************/
/*                                   program from Numeric recipes                             */
/**********************************************************************************************/
/**********************************************************************************************
 * FUNCTION: mnbrak
 * DESCRIPTION:Given a function func, and given distinct initial points ax and bx, this routine
 *  searches in the downhill direction (defined by the function as evaluated at the initial points) 
 * and returns new points ax, bx,cx that bracket a minimum of the function. Also returned are the
 *  function values at the three points,fa,fb,and fc.
 *********************************************************************************************/

int mnbrak(double *ax, double *bx,double *cx, double *fa, double *fb,double *fc,double (*func)(double))
{
  double ulim,u,r,q,fu,dum;

  *fa = (*func)(*ax);
  *fb = (*func)(*bx);
  if (*fb > *fa)
    {
      SHFT(dum,*ax,*bx,dum);
      SHFT(dum,*fb,*fa,dum);
    }
  *cx = (*bx) +GOLD*(*bx-*ax);
  *fc = (*func)(*cx);
  while (*fb > *fc)
    {
      r = (*bx-*ax)*(*fb-*fc);
      q = (*bx-*cx)*(*fb-*fa);
      u = (*bx)-((*bx-*cx)*q-(*bx-*ax)*r)/(2.0*SIGN(FMAX(fabs(q-r),TINY),q-r));
      ulim = (*bx)+GLIMIT*(*cx-*bx);
      
      if((*bx-u)*(u-*cx)>0.0)
        {
          fu = (*func)(u);
          if(fu < *fc)
            {
              *ax = (*bx);
              *bx = u;
              *fa=(*fb);
              *fb = fu;
              return 0;
            }
          else if(fu>*fb)
            {
              *cx = u;
              *fc = fu;
              return 0;
            }
          u = (*cx)+GOLD*(*cx-*bx);
          fu = (*func)(u);
        }
      else if((*cx-u)*(u-ulim)>0.0)
        {
          fu = (*func)(u);
          if(fu<*fc)
            {
              SHFT(*bx,*cx,u,*cx+GOLD*(*cx-*bx));
              SHFT(*fb,*fc,fu,(*func)(u));
            }
        }
      else if((u-ulim)*(ulim-*cx)>=0.0)
        {
          u = ulim;
          fu = (*func)(u);
        }
      else
        {
          u = (*cx)+GOLD*(*cx-*bx);
          fu = (*func)(u);
        }
      SHFT(*ax,*bx,*cx,u);
      SHFT(*fa,*fb,*fc,fu);
    }
  return 1;
}

/*******************************************************************************************/
/********************************************************************************************
 * FUNCTION: brent
 * DESCRIPTION: Given a function, and given a bracketing triplet of abscissas ax,bx,cx(such 
 * that bx is between ax and cx, and f(bx) is less than both f(ax) and f(cx)), this routine 
 * isolates the minimum to a fractional precision of about tol using Brent's method. The 
 * abscissa of the minimum is returned as xmin, and the minimum function value is returned 
 * as brent, the returned function value. 
 *
 * PARAMETERS:
 *    ax,bx,cx : a bracketing triplet of abscissas
 *    *f       : function address;
 *    tol      : fractional precision 
 *    *xmin    : minimum abscissa
 *    
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *      *xmin
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/05/00 by Ying Zhuge
 *
 *******************************************************************************************/
double brent(double ax,double bx,double cx,double (*f)(double),double tol,double *xmin)
{

  int iter;
  double a,b,d,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
  double e = 0.0;

  a = (ax < cx ? ax : cx);
  b = (ax > cx ? ax : cx);

  x = w = v = bx;
  fw = fv = fx = (*f)(x);
  for (iter=0;iter<ITMAX;iter++)
    {
      xm = 0.5*(a+b);
      tol2 = 2.0*(tol1 = tol * fabs(x)+ZEPS);
      if(fabs(x-xm) <= (tol2-0.5*(b-a)))

        {
          *xmin = x;
          return fx;
        }
      if(fabs(e)>tol1)
        {
          r = (x-w)*(fx-fv);
          q = (x-v)*(fx-fw);
          p = (x-v)*q-(x-w)*r;
          q = 2.0*(q-r);
          if(q>0.0) p = -p;
          q = fabs(q);
          etemp = e;
          e = d;
          if(fabs(p)>=fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x))
            d = CGOLD*(e=(x>=xm ? a-x:b-x));
          else 
            {
              d = p/q;
              u = x+d;
              if(u-a < tol2 || b-u < tol2)
                d = SIGN(tol1,xm-x);
            }
        }
      else
        {
          d = CGOLD*(e = (x>=xm?a-x: b-x));
        }
      u = (fabs(d)>=tol1?x+d:x+SIGN(tol1,d));
      fu = (*f)(u);

      if(fu<=fx)
        {
          if(u>=x)
            a = x;
          else 
            b = x;
          SHFT(v,w,x,u);
          SHFT(fv,fw,fx,fu);
        }
      else
        {
          if(u<x) 
            a = u;
          else
            b = u;
          if(fu<=fw || w ==x)
            {
              v = w;
              w = u;
              fv = fw;
              fw = fu;
            }
          else if (fu <=fv ||v ==x || v ==w)
            {
              v = u;
              fv = fu;
            }
        }
    }
  fprintf(stderr,"too many iterations in brent \n");
  *xmin = x;
  return fx;
}

/*******************************************************************************************/
/********************************************************************************************
 * FUNCTION: powell
 * DESCRIPTION: Minimization of a function func of n variables. Input consists of an initial starting point 
 *              p[1..n]; an initial matrix xi[1..n][1..n], whose columns contain the initial set of directions
 *              (usually the n unit vectors); and ftol, the fractional tolerance in the function value such that
 *              failure to decrease by more than this amount on one iteration signals doneness. On output, p is 
 *              set to the best point found, xi is the then-current direction set, fret is the returned function
 *              value at p, and iter is the number of iterations taken. The routine linmin is used. 
 *
 * PARAMETERS:
 *    p[]: n-dimensional point location
 *    xi[][]:n*n direction matrix; 
 *    n : dimension number;
 *    ftol: fractional tolerance in the function value
 *    iter: iteration number
 *    *fret : return function value
 *    *func: function address 
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *      function value
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/05/00 by Ying Zhuge
 *
 *******************************************************************************************/

int powell(double p[],double **xi,int n, double ftol,int *iter, double *fret, double (*func)(double []))
{

  void linmin(double p[],double xi[],int n, double *fret, double (*func)(double []));
  int i,ibig,j;
  double del,fp,fptt,t;
  double *pt,*ptt,*xit;

  pt = vector(n);
  ptt = vector(n);
  xit = vector(n);
  *fret = (*func)(p);
 
  for (j = 0;j<n;j++) pt[j]=p[j];
  for (*iter = 1;;++(*iter))
    {
      fp = (*fret);
      ibig = 0;
      del = 0.0;
      for(i=0;i<n;i++)
        {
          for (j=0;j<n;j++)
            xit[j] = xi[j][i];
          fptt = (*fret);
          linmin(p,xit,n,fret,func);
          if(fptt - (*fret)>del)
            {
              del = fptt-(*fret);
              ibig = i;
            }
        }
      if((double)4.0*(fp-(*fret))<(double)ftol*((double)fabs(fp)+(double)fabs(*fret))+(double)TINY)
        {
          free(xit);
          free(ptt);
          free(pt);
          return 0;
        }
      if(*iter ==ITMAX)
        {
          fprintf(stderr,"powell exceeding maximum iterations. \n");
          free(xit);
          free(ptt);
          free(pt);
          return 0;

        }

      for(j = 0;j<n;j++)
        {
          ptt[j] = (double)2.0*p[j]-pt[j];
          xit[j] = p[j]-pt[j];
          pt[j]=p[j];
        }
      fptt = (*func)(ptt);
      if(fptt < fp)
        {
          t = (double)2.0*(fp-2.0*(*fret)+fptt)*(double)SQR(fp-(*fret)-del)-del*(double)SQR(fp-fptt);
          if(t<0.0)
            {
              linmin(p,xit,n,fret,func);
              for(j= 0;j<n;j++)
                {
                  xi[j][ibig]=xi[j][n-1];
                  xi[j][n-1] = xit[j];
                }
            }
        }
    }
}


/*******************************************************************************************/
/********************************************************************************************
 * FUNCTION: linmin
 * DESCRIPTION: Given an n-dimensional point p and an n-dimensioanl xi direction,
 *              moves and reset p to where the function func(p) takes on a minimum
 *              along the direction xi from p,and replaces xi by the actual vector
 *              displacement that p was moved. Also returns as fret the value of func
 *              at the returned location p. This is actually all accomplished by calling
 *               the routines mnbrak and brent.
 * PARAMETERS:
 *    p[]: n-dimensional point location
 *    xi[]:n-dimensional direction; 
 *    n : dimension number;
 *    *fret: minimum function value
 *    *func: function address 
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *      function value
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/05/00 by Ying Zhuge
 *
 *******************************************************************************************/

void linmin(double p[],double xi[],int n, double *fret, double (*func)(double []))
{
  double f1dim(double x);

  int j;
  double xx,xmin,fx,fb,fa,bx,ax;

  ncom = n;
  pcom = vector(n);
  xicom = vector(n);
  nrfunc = func;
  for(j=0;j<n;j++)
    {
      pcom[j]=p[j];
      xicom[j]=xi[j];
    }
  ax = 0.0;
  xx = 1.0;
  mnbrak(&ax,&xx,&bx,&fa,&fx,&fb,f1dim);
  *fret = brent(ax,xx,bx,f1dim,TOL,&xmin);
  for(j=0;j<n;j++)
    {
      xi[j] *=xmin;
      p[j]+=xi[j];
    }
  free(xicom);
  free(pcom);
}

/*****************************************************************************
 * FUNCTION: f1dim
 * DESCRIPTION: compute the function value for n-dimension
 * PARAMETERS:
 *    x: direction displacement
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *      function value
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/05/00 by Ying Zhuge
 *
 *****************************************************************************/
double f1dim(double x)
{
  int j;
  double f;
  double *xt;

  xt = vector(ncom);
  for(j=0;j<ncom;j++)
    xt[j]=pcom[j]+x*xicom[j];
  f = (*nrfunc)(xt);
  free(xt);
  return f;
}

/*****************************************************************************
 * FUNCTION: get_factor_nD
 * DESCRIPTION: compute the factor array for estimated function
 * PARAMETERS:
 *    *pElem: point to regionElem array;
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *      factor array
 *
 *      f(x,y,z) = a0*X*X+a1*Y*Y+a2*Z*Z+a3*X*Y+a4*Y*Z+a5*X*Z+a6*X+a7*Y+a8*Z+a9
 *      (f(x,y,z)-inhomo(x,y,z)) * (f(x,y,z)-inhomo(x,y,z))
 *
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/14/00 by Ying Zhuge
 *
 *****************************************************************************/
int get_factor_nD(voxelElem *pElem, long int mask_volume, double factor[])
{
  long int i,tti1;
  int x,y,z;
  double inhomo;
  voxelElem *pLocation;

  tti1 = mask_volume;
  pLocation = pElem;

   for(i=0;i<66;i++)
     factor[i] = 0.0;

  for(i = 0;i<tti1;i++)
    {
      x = pLocation->x;
      y = pLocation->y;
      z = pLocation->z;
      inhomo = pLocation->inhomo_value;
      
      factor[0] = factor[0] + (double)x*x*x*x;
      factor[1] = factor[1] + (double)2*x*x*y*y;
      factor[2] = factor[2] + (double)2*x*x*z*z;
      factor[3] = factor[3] + (double)2*x*x*x*y;
      factor[4] = factor[4] + (double)2*x*x*y*z;
      factor[5] = factor[5] + (double)2*x*x*x*z;
      factor[6] = factor[6] + (double)2*x*x*x;
      factor[7] = factor[7] + (double)2*x*x*y;
      factor[8] = factor[8] + (double)2*x*x*z;
      factor[9] = factor[9] + (double)2*x*x;
      factor[10] = factor[10] + (double)2*x*x*inhomo;

      factor[11] = factor[11] + (double)y*y*y*y;
      factor[12] = factor[12] + (double)2*y*y*z*z;
      factor[13] = factor[13] + (double)2*x*y*y*y;
      factor[14] = factor[14] + (double)2*y*y*y*z;
      factor[15] = factor[15] + (double)2*x*y*y*z;
      factor[16] = factor[16] + (double)2*x*y*y;
      factor[17] = factor[17] + (double)2*y*y*y;
      factor[18] = factor[18] + (double)2*y*y*z;
      factor[19] = factor[19] + (double)2*y*y;
      factor[20] = factor[20] + (double)2*y*y*inhomo;

      factor[21] = factor[21] + (double)z*z*z*z;
      factor[22] = factor[22] + (double)2*x*y*z*z;
      factor[23] = factor[23] + (double)2*y*z*z*z;
      factor[24] = factor[24] + (double)2*x*z*z*z;
      factor[25] = factor[25] + (double)2*x*z*z;
      factor[26] = factor[26] + (double)2*y*z*z;
      factor[27] = factor[27] + (double)2*z*z*z;
      factor[28] = factor[28] + (double)2*z*z;
      factor[29] = factor[29] + (double)2*z*z*inhomo;

      factor[30] = factor[30] + (double)x*x*y*y;
      factor[31] = factor[31] + (double)2*x*y*y*z;
      factor[32] = factor[32] + (double)2*x*x*y*z;
      factor[33] = factor[33] + (double)2*x*x*y;
      factor[34] = factor[34] + (double)2*x*y*y;
      factor[35] = factor[35] + (double)2*x*y*z;
      factor[36] = factor[36] + (double)2*x*y;
      factor[37] = factor[37] + (double)2*x*y*inhomo;

      factor[38] = factor[38] +(double)y*y*z*z;
      factor[39] = factor[39] +(double)2*x*y*z*z;
      factor[40] = factor[40] +(double)2*x*y*z;
      factor[41] = factor[41] +(double)2*y*y*z;
      factor[42] = factor[42] +(double)2*y*z*z;
      factor[43] = factor[43] +(double)2*y*z;
      factor[44] = factor[44] +(double)2*y*z*inhomo;

      factor[45] = factor[45] + (double)x*x*z*z;
      factor[46] = factor[46] + (double)2*x*x*z;
      factor[47] = factor[47] + (double)2*x*y*z;
      factor[48] = factor[48] + (double)2*x*z*z;
      factor[49] = factor[49] + (double)2*x*z;
      factor[50] = factor[50] + (double)2*x*z*inhomo;

      factor[51] = factor[51] + (double)x*x;
      factor[52] = factor[52] + (double)2*x*y;
      factor[53] = factor[53] + (double)2*x*z;
      factor[54] = factor[54] + (double)2*x;
      factor[55] = factor[55] + (double)2*x*inhomo;

      factor[56] = factor[56] + (double)y*y;
      factor[57] = factor[57] + (double)2*y*z;
      factor[58] = factor[58] + (double)2*y;
      factor[59] = factor[59] + (double)2*y*inhomo;

      factor[60] = factor[60] + (double)z*z;
      factor[61] = factor[61] + (double)2*z;
      factor[62] = factor[62] + (double)2*z*inhomo;

      factor[63] = factor[63] + (double)1;
      factor[64] = factor[64] + (double)2*inhomo;

      factor[65] = factor[65] + (double)inhomo*inhomo;
    
      pLocation++;
    }
   
  return 0;
}

/*****************************************************************************
 * FUNCTION: function_value_nD
 * DESCRIPTION: compute the function value
 * PARAMETERS:
 *    a: parameters value
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: function get_factor should be called first 
 * RETURN VALUE:
 *      function value
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/14/00 by Ying Zhuge
 *
 *****************************************************************************/
double function_value_nD(double a[10])
{
  double ret;

  ret = (double)a[0]*a[0]*factor[0] + (double)a[0]*a[1]*factor[1] + (double)a[0]*a[2]*factor[2] + 
    (double)a[0]*a[3]*factor[3] + (double)a[0]*a[4]*factor[4] + (double)(double)a[0]*a[5]*factor[5] +
    (double)a[0]*a[6]*factor[6] + (double)a[0]*a[7]*factor[7] + (double)a[0]*a[8]*factor[8] +
    (double)a[0]*a[9]*factor[9] - (double)a[0]*factor[10] +
    (double)a[1]*a[1]*factor[11] + (double)a[1]*a[2]*factor[12] + (double)a[1]*a[3]*factor[13] +
    (double)a[1]*a[4]*factor[14] + (double)a[1]*a[5]*factor[15] + (double)a[1]*a[6]*factor[16] +
    (double)a[1]*a[7]*factor[17] + (double)a[1]*a[8]*factor[18] + (double)a[1]*a[9]*factor[19] -
    (double)a[1]*factor[20] +
    (double)a[2]*a[2]*factor[21] + (double)a[2]*a[3]*factor[22] + (double)a[2]*a[4]*factor[23] +
    (double)a[2]*a[5]*factor[24] + (double)a[2]*a[6]*factor[25] + (double)a[2]*a[7]*factor[26] +
    (double)a[2]*a[8]*factor[27] + (double)a[2]*a[9]*factor[28] - (double)a[2]*factor[29] +
    (double)a[3]*a[3]*factor[30] + (double)a[3]*a[4]*factor[31] + (double)a[3]*a[5]*factor[32] +
    (double)a[3]*a[6]*factor[33] + (double)a[3]*a[7]*factor[34] + (double)a[3]*a[8]*factor[35] +
    (double)a[3]*a[9]*factor[36] - (double)a[3]*factor[37] +
    (double)a[4]*a[4]*factor[38] + (double)a[4]*a[5]*factor[39] + (double)a[4]*a[6]*factor[40] +
    (double)a[4]*a[7]*factor[41] + (double)a[4]*a[8]*factor[42] + (double)a[4]*a[9]*factor[43] -
    (double)a[4]*factor[44]+
    (double)a[5]*a[5]*factor[45] + (double)a[5]*a[6]*factor[46] + (double)a[5]*a[7]*factor[47] + 
    (double)a[5]*a[8]*factor[48] + (double)a[5]*a[9]*factor[49] - (double)a[5]*factor[50] +
    (double)a[6]*a[6]*factor[51] + (double)a[6]*a[7]*factor[52] + (double)a[6]*a[8]*factor[53] +
    (double)a[6]*a[9]*factor[54] - (double)a[6]*factor[55]+
    (double)a[7]*a[7]*factor[56] + (double)a[7]*a[8]*factor[57] + (double)a[7]*a[9]*factor[58] -
    (double)a[7]*factor[59] +
    (double)a[8]*a[8]*factor[60] + (double)a[8]*a[9]*factor[61] - (double)a[8]*factor[62] +
    (double)a[9]*a[9]*factor[63] - (double)a[9]*factor[64];

  return ret;

}


/*****************************************************************************
 * FUNCTION: get_factor_1D
 * DESCRIPTION: compute the factor array for estimated function
 * PARAMETERS:
 *    *pElem: point to regionElem array;
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: 
 *      factor array
 *
 *      f(x,y,z) = a0*X*X+a1*Y*Y+a2*Z*Z+a3*X*Y+a4*Y*Z+a5*X*Z+a6*X+a7*Y+a8*Z+a9
 *      (f(x,y,z)-inhomo(x,y,z)) * (f(x,y,z)-inhomo(x,y,z))
 *
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/14/00 by Ying Zhuge
 *
 *****************************************************************************/
int get_factor_1D(double cof[])
{
  int i, xx,yy,zz;
  double f1,f2;

  for(i=0;i<3;i++)
    factor_1[i] = 0;

  // ---------------- for whole image ------------------------------
  for(zz = 0;zz < slices; zz++)
    for(yy = 0; yy < height; yy++)
      for(xx = 0;xx < width; xx++)
	{
	  f1 = aV[0][0]*xx*xx + aV[0][1]*yy*yy + aV[0][2]*zz*zz + aV[0][3]*xx*yy + aV[0][4]*yy*zz +
	    aV[0][5]*xx*zz + aV[0][6]*xx + aV[0][7]*yy + aV[0][8]*zz + aV[0][9];

	  f2 = cof[0]*xx*xx + cof[1]*yy*yy + cof[2]*zz*zz + cof[3]*xx*yy + cof[4]*yy*zz +
	    cof[5]*xx*zz + cof[6]*xx + cof[7]*yy + cof[8]*zz + cof[9];
	  
	    factor_1[0] = factor_1[0] + (double)f1*f1;
	    factor_1[1] = factor_1[1] + (double)2*f1*f2;
	    factor_1[2] = factor_1[2] + (double)f2*f2;
	    
	 
	}

 
  return 0;
}

/*****************************************************************************
 * FUNCTION: function_value_1D
 * DESCRIPTION: compute the function value
 * PARAMETERS:
 *    a: parameters value
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: function get_factor should be called first 
 * RETURN VALUE:
 *      function value
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 11/26/00 by Ying Zhuge
 *
 *****************************************************************************/
double function_value_1D(double x)
{

  double ret;
  
  ret = factor_1[0] - x*factor_1[1] + x*x*factor_1[2];
  
  return ret;
}


/*****************************************************************************
 * FUNCTION: compute_3Ddistance
 * DESCRIPTION: perform 3D distance transform
 * PARAMETERS:
 *    out_buffer: initialized buffer
 *                for object part, set value 0, for background, set value 65535
 *     
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: 
 * RETURN VALUE:
 *      distance map stored in out_buffer
 *         
 * EXIT CONDITIONS: None
 * HISTORY:
 *    
 *    Created: 11/28/00 modified by Ying Zhuge
 *
 *****************************************************************************/
int compute_3Ddistance( unsigned short out_buffer[])
{
  int i,ll,m,n;
  unsigned short *outptr[27];
  int ivoxsp[27];
  double reff;




  reff = 10/sqrt(pxsize*pxsize+pysize*pysize);
  ivoxsp[0] = ivoxsp[2] = ivoxsp[6] = ivoxsp[8] = ivoxsp[18] = ivoxsp[20] =
    ivoxsp[24] = ivoxsp[26] =
    2*(int)rint(reff*sqrt(pxsize*pxsize+pysize*pysize+space*space));
  ivoxsp[1] = ivoxsp[7] = ivoxsp[19] = ivoxsp[25] =
    2*(int)rint(reff*sqrt(pysize*pysize+space*space));
  ivoxsp[3] = ivoxsp[5] = ivoxsp[21] = ivoxsp[23] =
    2*(int)rint(reff*sqrt(pxsize*pxsize+pysize*pysize+space*space));
  ivoxsp[4] = ivoxsp[22] = 2*(int)rint(reff*space);
  ivoxsp[9] = ivoxsp[11] = ivoxsp[15] = ivoxsp[17] = 20;
  ivoxsp[10] = ivoxsp[16] = 2*(int)rint(reff*pysize);
  ivoxsp[12] = ivoxsp[14] = 2*(int)rint(reff*pxsize);





  //initialize the distances for the elements of the immediate interior
  for(i=0; i<slices; i++)
    {
      for (ll=0; ll<height; ll++)
	{
	  outptr[0] = out_buffer+(ll+i*(height+2))*(width+2);
	  outptr[1] = out_buffer+(ll+i*(height+2))*(width+2)+1;
	  outptr[2] = out_buffer+(ll+i*(height+2))*(width+2)+2;
	  outptr[3] = out_buffer+(ll+i*(height+2)+1)*(width+2);
	  outptr[4] = out_buffer+(ll+i*(height+2)+1)*(width+2)+1;
	  outptr[5] = out_buffer+(ll+i*(height+2)+1)*(width+2)+2;
	  outptr[6] = out_buffer+(ll+i*(height+2)+2)*(width+2);
	  outptr[7] = out_buffer+(ll+i*(height+2)+2)*(width+2)+1;
	  outptr[8] = out_buffer+(ll+i*(height+2)+2)*(width+2)+2;
	  outptr[9] = out_buffer+(ll+(i+1)*(height+2))*(width+2);
	  outptr[10] = out_buffer+(ll+(i+1)*(height+2))*(width+2)+1;
	  outptr[11] = out_buffer+(ll+(i+1)*(height+2))*(width+2)+2;
	  outptr[12] = out_buffer+(ll+(i+1)*(height+2)+1)*(width+2);
	  outptr[13] = out_buffer+(ll+(i+1)*(height+2)+1)*(width+2)+1;
	  outptr[14] = out_buffer+(ll+(i+1)*(height+2)+1)*(width+2)+2;
	  outptr[15] = out_buffer+(ll+(i+1)*(height+2)+2)*(width+2);
	  outptr[16] = out_buffer+(ll+(i+1)*(height+2)+2)*(width+2)+1;
	  outptr[17] = out_buffer+(ll+(i+1)*(height+2)+2)*(width+2)+2;
	  outptr[18] = out_buffer+(ll+(i+2)*(height+2))*(width+2);
	  outptr[19] = out_buffer+(ll+(i+2)*(height+2))*(width+2)+1;
	  outptr[20] = out_buffer+(ll+(i+2)*(height+2))*(width+2)+2;
	  outptr[21] = out_buffer+(ll+(i+2)*(height+2)+1)*(width+2);
	  outptr[22] = out_buffer+(ll+(i+2)*(height+2)+1)*(width+2)+1;
	  outptr[23] = out_buffer+(ll+(i+2)*(height+2)+1)*(width+2)+2;
	  outptr[24] = out_buffer+(ll+(i+2)*(height+2)+2)*(width+2);
	  outptr[25] = out_buffer+(ll+(i+2)*(height+2)+2)*(width+2)+1;
	  outptr[26] = out_buffer+(ll+(i+2)*(height+2)+2)*(width+2)+2;
	  for (n=0; n<width; n++)
	    {
	      if (*outptr[13])
		for (m=0; m<27; m++)
		  if (*outptr[m]==0 && *outptr[13]>ivoxsp[m]/2)
		    *outptr[13] = ivoxsp[m]/2;
	      for (m=0; m<27; m++)
		outptr[m]++;
	    }
	}
    }
  
  // Forward pass 
  for(i=0; i<slices; i++)
    {
      for (ll=0; ll<height; ll++)
	{
	  outptr[0] = out_buffer+(ll+i*(height+2))*(width+2);
	  outptr[1] = out_buffer+(ll+i*(height+2))*(width+2)+1;
	  outptr[2] = out_buffer+(ll+i*(height+2))*(width+2)+2;
	  outptr[3] = out_buffer+(ll+i*(height+2)+1)*(width+2);
	  outptr[4] = out_buffer+(ll+i*(height+2)+1)*(width+2)+1;
	  outptr[5] = out_buffer+(ll+i*(height+2)+1)*(width+2)+2;
	  outptr[6] = out_buffer+(ll+i*(height+2)+2)*(width+2);
	  outptr[7] = out_buffer+(ll+i*(height+2)+2)*(width+2)+1;
	  outptr[8] = out_buffer+(ll+i*(height+2)+2)*(width+2)+2;
	  outptr[9] = out_buffer+(ll+(i+1)*(height+2))*(width+2);
	  outptr[10] = out_buffer+(ll+(i+1)*(height+2))*(width+2)+1;
	  outptr[11] = out_buffer+(ll+(i+1)*(height+2))*(width+2)+2;
	  outptr[12] = out_buffer+(ll+(i+1)*(height+2)+1)*(width+2);
	  outptr[13] = out_buffer+(ll+(i+1)*(height+2)+1)*(width+2)+1;
	  for (n=0; n<width; n++)
	    {
	      if (*outptr[13])
		for (m=0; m<13; m++)
		  if (*outptr[13] > *outptr[m]+ivoxsp[m])
		    *outptr[13] = *outptr[m]+ivoxsp[m];
	      for (m=0; m<=13; m++)
		outptr[m]++;
	    }
	}
      
    } // end forward pass 
	    
  // Backward pass 
  for(i=slices-1; i>=0; i--)
    {
      for (ll=height-1; ll>=0; ll--)
	{
	  outptr[13] = out_buffer+(1+ll+(i+1)*(height+2)+1)*(width+2)-2;
	  outptr[14] = out_buffer+(1+ll+(i+1)*(height+2)+1)*(width+2)-1;
	  outptr[15] = out_buffer+(1+ll+(i+1)*(height+2)+2)*(width+2)-3;
	  outptr[16] = out_buffer+(1+ll+(i+1)*(height+2)+2)*(width+2)-2;
	  outptr[17] = out_buffer+(1+ll+(i+1)*(height+2)+2)*(width+2)-1;
	  outptr[18] = out_buffer+(1+ll+(i+2)*(height+2))*(width+2)-3;
	  outptr[19] = out_buffer+(1+ll+(i+2)*(height+2))*(width+2)-2;
	  outptr[20] = out_buffer+(1+ll+(i+2)*(height+2))*(width+2)-1;
	  outptr[21] = out_buffer+(1+ll+(i+2)*(height+2)+1)*(width+2)-3;
	  outptr[22] = out_buffer+(1+ll+(i+2)*(height+2)+1)*(width+2)-2;
	  outptr[23] = out_buffer+(1+ll+(i+2)*(height+2)+1)*(width+2)-1;
	  outptr[24] = out_buffer+(1+ll+(i+2)*(height+2)+2)*(width+2)-3;
	  outptr[25] = out_buffer+(1+ll+(i+2)*(height+2)+2)*(width+2)-2;
	  outptr[26] = out_buffer+(1+ll+(i+2)*(height+2)+2)*(width+2)-1;
	  for (n=width-1; n>=0; n--)
	    {
	      if (*outptr[13])
		for (m=14; m<27; m++)
		  if (*outptr[13] > *outptr[m]+ivoxsp[m])
		    *outptr[13] = *outptr[m]+ivoxsp[m];
	      for (m=13; m<27; m++)
		outptr[m]--;
	    }
	}
      
    } // end backward pass
 
  return 0;
}

/*****************************************************************************
 * FUNCTION: destroy_scene_header
 * DESCRIPTION: Frees the memory referenced by the pointer fields of a scene
 *    file header.
 * PARAMETERS:
 *    vh: The scene file header.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The pointer fields of vh must point to allocated memory or
 *    be NULL.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if entry conditions are not fulfilled.
 * HISTORY:
 *    Created: 5/11/93 by Dewey Odhner
 *
 *****************************************************************************/
void destroy_scene_header(ViewnixHeader *vh)
{
	if (vh->gen.description)
		free(vh->gen.description);
	if (vh->gen.comment)
		free(vh->gen.comment);
	if (vh->gen.imaged_nucleus)
		free(vh->gen.imaged_nucleus);
	if (vh->gen.gray_lookup_data)
		free(vh->gen.gray_lookup_data);
	if (vh->gen.red_lookup_data)
		free(vh->gen.red_lookup_data);
	if (vh->gen.green_lookup_data)
		free(vh->gen.green_lookup_data);
	if (vh->gen.blue_lookup_data)
		free(vh->gen.blue_lookup_data);
	if (vh->scn.domain)
		free(vh->scn.domain);
	if (vh->scn.axis_label)
		free(vh->scn.axis_label);
	if (vh->scn.measurement_unit)
		free(vh->scn.measurement_unit);
	if (vh->scn.density_measurement_unit)
		free(vh->scn.density_measurement_unit);
	if (vh->scn.smallest_density_value)
		free(vh->scn.smallest_density_value);
	if (vh->scn.largest_density_value)
		free(vh->scn.largest_density_value);
	if (vh->scn.signed_bits)
		free(vh->scn.signed_bits);
	if (vh->scn.bit_fields)
		free(vh->scn.bit_fields);
	if (vh->scn.num_of_subscenes)
		free(vh->scn.num_of_subscenes);
	if (vh->scn.loc_of_subscenes)
		free(vh->scn.loc_of_subscenes);
	if (vh->scn.description)
		free(vh->scn.description);
	memset(vh, 0, sizeof(*vh));
}
