/*
  Copyright 1993-2009 Medical Image Processing Group
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
#include <errno.h>
#include <fcntl.h>
#include <math.h>
//#include <unistd.h>
#include <string.h>

#include <mpi.h>
#include <Viewnix.h>
#include  "cv3dv.h"

#define	DIETAG		1
#define WORKTAG		2

#define MAX_SCALE 10

#define OUTPUTSCALEIMAGE   // switch for output scale image

/* local variables */
unsigned char             *data_8;
unsigned short            *data_16, *filter_data;   /* input and out data for whole image */

unsigned char             *chunk_data8;
unsigned short            *chunk_data16,*chunk_out;  /* input and out data for chunk image */

char             *feature_scale;   /* scale data */

float                     *scale_map;
float                     sigma;          /* region-homogeneity standard deviation for scale computation*/
int                       *circle_no_points, (**circle_points)[2];
int                       chunk_info[6];   /* chunk information for each process */

static int                 pslice,prow,pcol,num_of_bits,volume_size,slice_size,chunk_size,chunk_slice;
static int                 largest_density_value;
static int                 myrank;			/* compute task's rank */
static ViewnixHeader       vh;
int                        tolerance = 13.0;
int                        cutoff_threshold = 0;

/* local functions */
static void                master(char* , char*);
static void                slave(void);

void                       do_computation_work(void);
double                     estimate_sigma(void);
void                       scale_prepare(void);
void                       compute_feature_scale(void);
void                       compute_feature_filtering(void);


int main(int argc, char* argv[])
{

  char *datafile, *outfile;

  int ntasks;
  int n,rank;
  int i,j,x,y,z,slice,error_code;
  static ViewnixHeader vh;
  char group[6],element[6];
  FILE *fp;
  float *scale_map;
  MPI_Status status;
  int min,max;


  if (argc != 3) 
    {
      printf("Usage: %s <in file> <out file> \n",argv[0]);
      exit(-1);
    }

  datafile = argv[1];
  outfile = argv[2];

  /* Initialize MPI. */
  MPI_Init(&argc, &argv);
  /* find out my identity in the default communicator */
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  
  if(myrank == 0)
    master(datafile, outfile);
  else 
    slave();

  /* shut down MPI */
  MPI_Finalize();

  return 0;
}
/*----------------- master program -----------------------------*/

void master(char* in, char*out)
{
  char *datafile, *outfile;
  int ntasks;
  int n,rank;
  int i,j,x,y,z,slice_begin,slice_end,error_code;
  char group[6],element[6];
  FILE *fp;
  float *scale_map;
  MPI_Status status;
  int min,max;
  unsigned char *pdata8;
  unsigned short *pdata16;
  unsigned char  *scaleData = NULL;
 
  time_t t1,t2;

  time(&t2);

 
  MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

  /* read a 3D input image */
  fp = fopen(in,"rb");
  if(fp == NULL)
    {
      printf("Can not open input file!\n");
      exit(-1);
    }
  error_code = VReadHeader(fp, &vh, group, element);
  switch (error_code)
	{
	case 0:
	case 106:
	case 107:
	  break;
	default:
	  fprintf(stderr, "file = %s; group = %s; element = %s\n", in, group, element);
	}
 
  pcol = vh.scn.xysize[0];
  prow = vh.scn.xysize[1];
  pslice = vh.scn.num_of_subscenes[0];
  slice_size = pcol * prow;
  volume_size = slice_size * pslice;
  num_of_bits = vh.scn.num_of_bits;
  largest_density_value = vh.scn.largest_density_value[0];

  //chunk_slice = (pslice + ntasks - 2)/(ntasks-1); /* master process does't do computation work */
  chunk_slice = (pslice + ntasks - 1)/ntasks; /* master process needs to do some computation work also*/
  chunk_size = chunk_slice * slice_size;

  chunk_info[2] = prow;
  chunk_info[3] = pcol;
  chunk_info[4] = num_of_bits;
  chunk_info[5] = largest_density_value;

  VSeekData(fp, 0);
  if(num_of_bits == 8)
    {
      data_8 = (unsigned char*)malloc(volume_size*sizeof(unsigned char));
      if(data_8 == 0)	MPI_Abort(MPI_COMM_WORLD,errno);

      error_code = VReadData((char*)data_8, num_of_bits/8, volume_size, fp, &j);
    }
  else if(num_of_bits == 16)
    {
      data_16 = (unsigned short*)malloc(volume_size*sizeof(unsigned short));
      if(data_16 == 0) MPI_Abort(MPI_COMM_WORLD,errno);

      error_code = VReadData((char*)data_16, num_of_bits/8, volume_size, fp, &j);
    }
  fclose(fp);
  /* end of reading image */

  /* estimate homogeneity-based standard deviation value */
  
  sigma = estimate_sigma();
  
  /* Send this parameter to all workers */
  for(n = 0;n<ntasks;n++)
    if(n != myrank)
      {
	MPI_Send(&sigma,1, MPI_FLOAT,n,WORKTAG,MPI_COMM_WORLD);
      }

  /* allocate memory for processed data */
  //filter_data = (unsigned short*)malloc(volume_size*sizeof(short));
  //if(filter_data == 0) MPI_Abort(MPI_COMM_WORLD,errno);


  /* Distribute work to all slaves */

  slice_begin = 0;slice_end = 0;

  if(num_of_bits == 8)
    pdata8 = data_8;
  else if(num_of_bits == 16)
    pdata16 = data_16;

  for(n = 1;n<ntasks;n++)
    {
	  slice_end = slice_begin + chunk_slice-1;
	  chunk_info[0] = slice_begin;
	  chunk_info[1] = slice_end;
	  chunk_size = (chunk_info[1] - chunk_info[0] + 1)*slice_size;
	     
	  printf("master: allocating slice %d - %d to slave process %d.\n",slice_begin,slice_end,n);
	  /* master just distribute chunk information to slaves */
	  MPI_Send(&chunk_info,6, MPI_INT,n,WORKTAG,MPI_COMM_WORLD);

	  slice_begin = slice_end + 1;

	  /* transfer data to slaves */
	  if(num_of_bits == 8)
	    {
	      MPI_Send(pdata8,chunk_size,MPI_CHAR,n,WORKTAG,MPI_COMM_WORLD);
	      pdata8 = pdata8 + chunk_size;
	    }
	  else if(num_of_bits == 16)
	    {
	      MPI_Send(pdata16,chunk_size,MPI_UNSIGNED_SHORT,n,WORKTAG,MPI_COMM_WORLD);
	      pdata16 = pdata16 + chunk_size;
	    }
    }


   scaleData = (unsigned char*)malloc(volume_size*sizeof(unsigned char));
   if(scaleData == 0)	MPI_Abort(MPI_COMM_WORLD,errno);


  /* master will do computation for the left chunk */
  slice_end = (slice_begin + chunk_slice) < pslice? slice_begin + chunk_slice-1: pslice-1;
  chunk_info[0] = slice_begin;
  chunk_info[1] = slice_end;
  chunk_slice = chunk_info[1] - chunk_info[0] + 1; /* The last chunk could be smaller than regular one */
  chunk_size = chunk_slice*slice_size;
  printf("master: allocating slice %d - %d to master process %d.\n",slice_begin,slice_end,0);

  if(num_of_bits == 8)
    chunk_data8 = data_8 + slice_begin*slice_size;
  else  if(num_of_bits == 16)
    chunk_data16 = data_16 + slice_begin*slice_size;

  //chunk_out = filter_data + slice_begin*slice_size;

   /* allocate scale image */
	feature_scale = ( char*)malloc(chunk_size*sizeof( char));
	if(feature_scale == 0) 
		MPI_Abort(MPI_COMM_WORLD,errno);

  /* master does computation work on this chunk */
  do_computation_work();

  /* collect processed data from slaves */  
 
 // pdata16 = filter_data;
  pdata8 = scaleData;
  for( n = 1;n<ntasks;++n)
    {
	  MPI_Recv(chunk_info, 6, MPI_INT, n, 0,MPI_COMM_WORLD,&status);
	  chunk_size = (chunk_info[1] - chunk_info[0] + 1)*slice_size;
//	  MPI_Recv(pdata16,chunk_size,MPI_UNSIGNED_SHORT,n,0,MPI_COMM_WORLD,&status);
//	  pdata16 = pdata16 + chunk_size;


	  MPI_Recv(pdata8,chunk_size,MPI_CHAR,n,0,MPI_COMM_WORLD,&status);
	  pdata8 = pdata8 + chunk_size;

    }
   
  printf("receving: done. \n");

 /*
 * We have all the answers now, so kill the workers.
 */ 
  for(rank = 0;rank<ntasks;++rank)
    {
      if (rank != myrank) 
	{
	  MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
	}
    }

 
  ///* write processed data to output file */
  //min = 65535;max = 0;
  //for(i = 0;i<volume_size;i++)
  //  {
  //    min = min > filter_data[i]? filter_data[i]: min;
  //    max = max < filter_data[i]? filter_data[i]: max;
  //  }

  //vh.scn.num_of_bits = 16;
  //vh.scn.bit_fields[1] = vh.scn.num_of_bits - 1;

  //vh.scn.smallest_density_value[0] = 0;  
  //vh.scn.largest_density_value[0] = max;

  //fp = fopen(out,"wb");
  //    
  //error_code = VWriteHeader(fp, &vh, group, element);  
  //error_code = VWriteData(filter_data, num_of_bits/8, volume_size, fp, &j);  
  //fclose(fp);

  //free(filter_data);
  if(num_of_bits == 8)
    free(data_8);
  else if (num_of_bits == 16)
    free(data_16);


   chunk_size = (slice_end -slice_begin+1) *slice_size;
   pdata8 = scaleData;
   pdata8 = pdata8 + slice_begin*slice_size;
   memcpy( pdata8, feature_scale, chunk_size );
 
  printf("slice_begin = %d, slice_size = %d, chunk_size = %d, volume_size = %d \n", slice_begin, slice_size, chunk_size, volume_size);
  /* write the scale image data to output file */
  min = 65535;max = 0;
  for(i = 0;i<volume_size;i++)
  {
	  min = min > scaleData[i]? scaleData[i]: min;
	  max = max < scaleData[i]? scaleData[i]: max;
  }

  vh.scn.num_of_bits = 8;
  vh.scn.bit_fields[1] = vh.scn.num_of_bits - 1;

  vh.scn.smallest_density_value[0] = 0;  
  vh.scn.largest_density_value[0] = max;

  fp = fopen(out,"wb");

  error_code = VWriteHeader(fp, &vh, group, element);  
  error_code = VWriteData((char*)scaleData, 1, volume_size, fp, &j);  
  fclose(fp);

  free( scaleData );



    if( feature_scale != NULL )
	  free( feature_scale );

  printf("master: done. \n");

  time(&t1);
  printf("Total computation last:%f seconds\n",difftime(t1,t2)); 
}

/*
 Estimate the  region-homoegneoty mean and standard deviation 
 based on the gradient information.
*/

double estimate_sigma()
{
  int i, j, k,tti1;
  int xx, yy, zz, x, y, z,feature_thr;
  long count;
  double sigma_mean,sigma_sum,sigma_std,homogeneity_sigma;
  int hist_sum,histogram[65535];
  int sigma_constant = 1;
  float sigma_t = 1;
  float HIST_THRESHOLD = 0.80;


  /*----to compute the histogram for each feature and the threshold for true edge---*/
  for(j=0;j<=largest_density_value;j++)
    histogram[j] = 0;
       
  for (i=0;i<pslice;i++)
    for (j=0;j<prow;j++)
      for (k=0;k<pcol-1;k++)
	{
	       xx = k+1;
	       yy = j;
	       zz = i;
	       
	       if(num_of_bits == 8)
		 tti1 = abs(data_8[i*slice_size+j*pcol+k] - data_8[zz*slice_size+yy*pcol+xx]);
	       else if(num_of_bits == 16)
		 tti1 = abs(data_16[i*slice_size+j*pcol+k] - data_16[zz*slice_size+yy*pcol+xx]);
	       histogram[tti1]++;
	}
       
  for (i=0;i<pslice;i++)
    for (j=0;j<prow-1;j++)
      for (k=0;k<pcol;k++)
	{
	       xx = k;
	       yy = j+1;
	       zz = i;
	       
	       if(num_of_bits == 8)
		 tti1 = abs(data_8[i*slice_size+j*pcol+k] - data_8[zz*slice_size+yy*pcol+xx]);
	       else if(num_of_bits == 16)
		 tti1 = abs(data_16[i*slice_size+j*pcol+k] - data_16[zz*slice_size+yy*pcol+xx]);
	       histogram[tti1]++;
	       
	}
       
  for (i=0;i<pslice-1;i++)
    for(j=0;j<prow;j++)
      for (k=0;k<pcol;k++)
	{
	       xx = k;
	       yy = j;
	       zz = i+1;
	       
	       if(num_of_bits == 8)
		 tti1 = abs(data_8[i*slice_size+j*pcol+k] - data_8[zz*slice_size+yy*pcol+xx]);
	       else if(num_of_bits == 16)
		 tti1 = abs(data_16[i*slice_size+j*pcol+k] - data_16[zz*slice_size+yy*pcol+xx]);
	       histogram[tti1]++;
	       
	}
       
  hist_sum = 0;
  for(j=0;j<largest_density_value;j++)
    hist_sum = hist_sum + histogram[j];
       
       
  for(j=0;j<largest_density_value;j++)
    {
	   tti1 = 0;
	   feature_thr = (double)j;
	   for(k=0;k<=j;k++)
	     tti1 = tti1+histogram[k];
	   if (((double)tti1 /(double) hist_sum)>=HIST_THRESHOLD)
	     break;
    }
       
  printf("Histogram threshold computation is done \n");
  printf("Features Threshold %d : %d \n", i,feature_thr); 
       
       
  //--------------------compute the homogeneity mean-------------------------------
  sigma_sum = 0;
  count = 0;
  sigma_mean = 0;
  {
    for(z = 0;z<pslice;z++)
      for(y = 0;y<prow;y++)
	for(x = 0;x<pcol-1;x++)
	  {
		 zz = z;
		 yy = y;
		 xx = x + 1;
		 if(num_of_bits == 8)
		   tti1 = abs(data_8[z*slice_size + y*pcol +x] - data_8[zz*slice_size + yy*pcol +xx]);
		 else if(num_of_bits == 16)
		   tti1 =  abs(data_16[z*slice_size + y*pcol +x] - data_16[zz*slice_size + yy*pcol +xx]);
		 if(tti1 < feature_thr)
		   {
		     sigma_sum = sigma_sum + tti1;
		     count ++;
		   }
	  }
	 
    for(z = 0;z<pslice;z++)
      for(y = 0;y<prow-1;y++)
	for(x = 0;x<pcol;x++)
	  {
		 zz = z;
		 yy = y+1;
		 xx = x;
		 if(num_of_bits == 8)
		   tti1 = abs(data_8[z*slice_size + y*pcol +x] - data_8[zz*slice_size + yy*pcol +xx]);
		 else if(num_of_bits == 16)
		   tti1 =  abs(data_16[z*slice_size + y*pcol +x] - data_16[zz*slice_size + yy*pcol +xx]);
		 if(tti1 < feature_thr)
		   {
		     sigma_sum = sigma_sum + tti1;
		     count ++;
		   }
	  }
    for(z = 0;z<pslice-1;z++)
      for(y = 0;y<prow;y++)
	for(x = 0;x<pcol;x++)
	  {
		 zz = z+1;
		 yy = y;
		 xx = x;
		 if(num_of_bits == 8)
		   tti1 = abs(data_8[z*slice_size + y*pcol +x] - data_8[zz*slice_size + yy*pcol +xx]);
		 else if(num_of_bits == 16)
		   tti1 =  abs(data_16[z*slice_size + y*pcol +x] - data_16[zz*slice_size + yy*pcol +xx]);
		 if(tti1 < feature_thr)
		   {
		     sigma_sum = sigma_sum + tti1;
		     count ++;
		   }
	  }
	 
    sigma_mean = sigma_sum / count;
  
    printf("homogeneity_mean value is: %f \n", sigma_mean);
       

    //--------------------compute the homogeneity sigma-------------------------------
    sigma_sum = 0;
    //sigma_mean = 0;
    count = 0;
    for(z = 0;z<pslice;z++)
      for(y = 0;y<prow;y++)
	for(x = 0;x<pcol-1;x++)
	  {
		  zz = z;
		  yy = y;
		  xx = x + 1;
		  if(num_of_bits == 8)
		     tti1 = abs(data_8[z*slice_size + y*pcol +x] - data_8[zz*slice_size + yy*pcol +xx]) ;
		  else if(num_of_bits == 16)
		    tti1 =  abs(data_16[z*slice_size + y*pcol +x] - data_16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr)
		    {
		      sigma_sum += (tti1-sigma_mean) * (tti1-sigma_mean);
		      count ++;
		    }
	  }

    for(z = 0;z<pslice;z++)
      for(y = 0;y<prow-1;y++)
	for(x = 0;x<pcol;x++)
	  {
		  zz = z;
		  yy = y+1;
		  xx = x;
		  if(num_of_bits == 8)
		     tti1 = abs(data_8[z*slice_size + y*pcol +x] - data_8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 = abs(data_16[z*slice_size + y*pcol +x] - data_16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr)
		    {
		      sigma_sum += (tti1-sigma_mean) * (tti1-sigma_mean);
		      count ++;
		    }
	  }
    for(z = 0;z<pslice-1;z++)
      for(y = 0;y<prow;y++)
	for(x = 0;x<pcol;x++)
	  {
		  zz = z+1;
		  yy = y;
		  xx = x;
		  if(num_of_bits == 8)
		     tti1 = abs(data_8[z*slice_size + y*pcol +x] - data_8[zz*slice_size + yy*pcol +xx]);
		  else if(num_of_bits == 16)
		    tti1 =  abs(data_16[z*slice_size + y*pcol +x] - data_16[zz*slice_size + yy*pcol +xx]);
		  if(tti1 < feature_thr)
		    {
		      sigma_sum += (tti1-sigma_mean) * (tti1-sigma_mean); //pow((tti1 - sigma_mean),2);
		      count ++;
		    }
	  }
  }

  sigma_std = sqrt(sigma_sum/count); // sqrt
  homogeneity_sigma = sigma_constant * (sigma_mean + sigma_t*sigma_std);
  printf("homogeneity_sigma value: %f, sigma_sum = %f, count = %d \n",sigma_std, sigma_sum, count); 

  return homogeneity_sigma;
}


/*---------------- slave program-------------------------------- */
void slave()
{
	int		i, j;			/* loop counters */
	int             x,y,z;                   /* voxel cordinate */
	int		source;			/* sender's rank */
	int		tag;			/* message tag */
	MPI_Status	status;			/* returned from MPI */
	FILE            *fp;
        
	//MPI_Init(&argc, &argv);
	/* Receive region-homogeneity standard deviation */

	while (1)
	  {
	    MPI_Recv(&sigma,1,MPI_FLOAT, MPI_ANY_SOURCE,MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	    if(status.MPI_TAG == DIETAG)
	      {
		/* free memory allocated in slave program when computation is done and return to master program */
		if(num_of_bits == 8)
		  free(chunk_data8);
		else if(num_of_bits == 16)
		  free(chunk_data16);
	//	free(chunk_out);
		return;
	      }

	    /* Receive chunk size information */
	    MPI_Recv(chunk_info, 6, MPI_INT,0,WORKTAG, MPI_COMM_WORLD, &status);
	    
	    source = status.MPI_SOURCE;
	    
	    chunk_slice = chunk_info[1]-chunk_info[0]+1 ;
	    
	    prow = chunk_info[2];
	    pcol = chunk_info[3];
	    num_of_bits =  chunk_info[4];
	    largest_density_value = chunk_info[5];
	    
	    slice_size = prow*pcol;
	    chunk_size = chunk_slice*prow*pcol;
	    
	    if(num_of_bits == 8)
	      {
		chunk_data8 = (unsigned char *)malloc(chunk_size*sizeof(unsigned char));
		if(chunk_data8 == 0) 
		  MPI_Abort(MPI_COMM_WORLD,errno);
		MPI_Recv(chunk_data8,chunk_size,MPI_CHAR,0,WORKTAG, MPI_COMM_WORLD, &status);
	      }
	    else if(num_of_bits == 16)
	      {
		chunk_data16 = (unsigned short *) malloc(chunk_size*sizeof(unsigned short));
		if(chunk_data16 == 0) 
		  MPI_Abort(MPI_COMM_WORLD,errno);
		MPI_Recv(chunk_data16,chunk_size,MPI_UNSIGNED_SHORT,0,WORKTAG,MPI_COMM_WORLD, &status);
		
	      }
	    
	    /* allocate memory for filtered chunk data */
	/*    chunk_out = (unsigned short *) malloc(chunk_size*sizeof(short));
	    if(chunk_out == 0) 
	      MPI_Abort(MPI_COMM_WORLD,errno);
	*/    
		 /* allocate scale image */
		feature_scale = (char*)malloc(chunk_size*sizeof(char));
		if(feature_scale == 0) 
			MPI_Abort(MPI_COMM_WORLD,errno);

	    /* once all parameters are ready, slave can start it computation work */
	    do_computation_work();

	    /* send back the chunk information to master */
	    MPI_Send(chunk_info, 6, MPI_INT,source,0,MPI_COMM_WORLD);
	    
	    /* send back the filtered sub-image to master */
	//    MPI_Send(chunk_out, chunk_size, MPI_UNSIGNED_SHORT,source,0,MPI_COMM_WORLD);


	    /* send back the scale sub-image to master */
	    MPI_Send(feature_scale, chunk_size, MPI_CHAR,source,0,MPI_COMM_WORLD);

		free(feature_scale);
	  }
}
	     
void do_computation_work()
{
  int       i;
  FILE     *fp;

  /* compute look up table for scale computation */
  scale_map = (float *)malloc((largest_density_value+1)*sizeof(float));
  if(scale_map == 0) 
    MPI_Abort(MPI_COMM_WORLD,errno);
	
  for (i = 0; i <= largest_density_value; i++)
    scale_map[i] = exp(-0.5*((float)(i*i))/(4*sigma*sigma)); 


  /* look up table for point number and point position for each scale */
  scale_prepare();

  ///* allocate scale image */
  //feature_scale = (char*)malloc(chunk_size*sizeof(char));
  //if(feature_scale == 0) 
  //  MPI_Abort(MPI_COMM_WORLD,errno);


  /* compute the scale value for each voxel in the chunk image */	
  compute_feature_scale();
  	
  /* write feature scale for testing */
  /*
  fp = fopen("scale.raw","wb");
  fwrite(feature_scale,1,chunk_size,fp);
  fclose(fp);
  */
  
  /* compute the filtering based on the scale value */
//  compute_feature_filtering();

  free(scale_map);
//  free(feature_scale);
  //return;
}

void scale_prepare()
{
  
    int    i,j,k;          /* loop counter */
    int    **pptti1;        
    int    tti1,tti2;
    double tt1;

    pptti1 = (int **) malloc(2 * (MAX_SCALE + 5) * sizeof(int *));
    if (pptti1 == 0)  
      MPI_Abort(MPI_COMM_WORLD,errno);

    pptti1[0] = (int *) malloc(2 * (MAX_SCALE + 5) * 2 * (MAX_SCALE + 5) * sizeof(int));
    if (pptti1[0] == 0)  
      MPI_Abort(MPI_COMM_WORLD,errno); 

    for (i = 0; i < 2 * (MAX_SCALE + 5); i++)
        pptti1[i] = pptti1[0] + i * 2 * (MAX_SCALE + 5);
    tti1 = MAX_SCALE + 5;
    for (i = 0; i < 2 * (MAX_SCALE + 5); i++)
        for (j = 0; j < 2 * (MAX_SCALE + 5); j++)
            pptti1[i][j] = 0;

    circle_no_points = (int *) malloc(MAX_SCALE * sizeof(int));
    if (circle_no_points == 0)  
      MPI_Abort(MPI_COMM_WORLD,errno); 

    circle_points = (int (**)[2]) malloc(MAX_SCALE * sizeof(int (*)[2]));
    if(circle_points == 0) 
      MPI_Abort(MPI_COMM_WORLD,errno); 

    for (k = 0; k < MAX_SCALE; k++) {
        circle_no_points[k] = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                if (pptti1[tti1 + i][tti1 + j] == 0) {
                    tt1 = sqrt(pow(((double) i), 2.0) + pow(((double) j), 2.0));
                    if (tt1 <= ((double) k) + 0.5) {
                        circle_no_points[k] = circle_no_points[k] + 1;
                        pptti1[tti1 + i][tti1 + j] = 2;
                    }
                }
        circle_points[k] = (int (*)[2]) malloc(2 * circle_no_points[k] * sizeof(int));
        if (circle_points[k] == 0) 
	  MPI_Abort(MPI_COMM_WORLD,errno); 


        tti2 = 0;
        for (i = -k - 2; i <= k + 2; i++)
            for (j = -k - 2; j <= k + 2; j++)
                if (pptti1[tti1 + i][tti1 + j] == 2) {
                    pptti1[tti1 + i][tti1 + j] = 1;
                    circle_points[k][tti2][0] = i;
                    circle_points[k][tti2][1] = j;
                    tti2 = tti2 + 1;
                }
    }
    
    free(pptti1[0]);
    free(pptti1);
} 
	      

void compute_feature_scale()
{
    double     mask[3][3],mask_total;   /* for 3 by 3 distance-weighted average */
    int        xx, yy, x, y,z,slice,row,col;      /* loop counter */
    int        mean_g,flag,tti2,tti5;
    double     count_obj, count_nonobj, tt1, tt2, tt3;
    int        i,k;
    
	mask_total = 0;

    for (yy = -1; yy <= 1; yy++)
      for (xx = -1; xx <= 1; xx++)
	{
	  tt2 = pow( xx, 2.0);
	  tt2 = tt2 + pow(yy, 2.0);
	  tt2 = 1 / (1 + tt2);
	  mask[yy+1][xx + 1] = tt2;
	  mask_total = mask_total + tt2;
	}
 
  if (num_of_bits == 8)  {
    for (slice = 0; slice < chunk_slice; slice++)
      for (row = 0; row < prow; row++)
	for (col = 0; col < pcol; col++)
	  if(chunk_data8[slice* slice_size + row * pcol + col] < cutoff_threshold)
	    feature_scale[slice *prow*pcol + row * pcol + col] = 1;
	  else  
	    {
	      flag = 0;
	      tt1 = 0.0;
	      tt3 = 0.0;
	      for (yy = -1; yy <= 1; yy++)
		for (xx = -1; xx <= 1; xx++)  
		  {
		    x = xx + col;
		    y = yy + row;
		    z = slice;
		    if (x >= 0 && y >= 0 && x < pcol && y < prow)
		      tt3 = tt3 + mask[yy + 1][xx + 1]	
			* (double) chunk_data8[z * slice_size + y * pcol + x];
		    else
		      tt3 = tt3 + mask[yy + 1][xx + 1] 
			* (double) chunk_data8[slice * slice_size + row * pcol + col];
		  }
	      mean_g = (int) (tt3 / mask_total + 0.5);
	    
	    for (k = 1; k < MAX_SCALE && !flag; k++)  
	      {
		count_obj = 0;
		count_nonobj = 0;
		for (i = 0; i < circle_no_points[k]; i++)  
		  {
		    x = col + circle_points[k][i][0];
		    y = row + circle_points[k][i][1];
		    z = slice;

		    if (x < 0 || x >= pcol)
		      x = col;
		    if (y < 0 || y >= prow)
		      y = row;
		    
		    tti5 = (int) chunk_data8[z * slice_size + y * pcol + x];
		    tti5 = tti5 - mean_g;
		    if (tti5 < 0)
		      tti5 = -tti5;
		    count_obj = count_obj + scale_map[tti5];
		    count_nonobj = count_nonobj + 1.0 - scale_map[tti5];
		  }
		if (100.0 * count_nonobj >= tolerance
		    * (count_nonobj + count_obj)) {
		  feature_scale[slice * slice_size + row * pcol + col] = k;
		  flag = 1;
		}
	      }
	    if (!flag)
	      feature_scale[slice * slice_size + row * pcol + col] = k;
	    }
  }
  else if (num_of_bits == 16) {
    for (slice = 0; slice < chunk_slice; slice++)
      for (row = 0; row < prow; row++)
	for (col = 0; col < pcol; col++)
	  if((int) chunk_data16[slice* slice_size + row * pcol + col] < cutoff_threshold)
	    feature_scale[slice * slice_size + row * pcol + col] = 1;
	  else  
	    {
	      flag = 0;
	      tt1 = 0.0;
	      tt3 = 0.0;
	      for (yy = -1; yy <= 1; yy++)
		for (xx = -1; xx <= 1; xx++)  
		  {
		    x = xx + col;
		    y = yy + row;
		    z = slice;
		    if (x >= 0 && y >= 0  
			&& x < pcol && y < prow)
		      tt3 = tt3 + mask[yy + 1][xx + 1] 
			* (double) chunk_data16[z * slice_size + y * pcol + x];
		    else
		      tt3 = tt3 + mask[yy + 1][xx + 1] 
			* (double) chunk_data16[slice * slice_size+ row * pcol + col];
		  }
	      mean_g = (int) (tt3 / mask_total + 0.5);
	      
	      for (k = 1; k < MAX_SCALE && !flag; k++)  
		{
		  count_obj = 0;
		  count_nonobj = 0;
		  for (i = 0; i < circle_no_points[k]; i++)  {
		    x = col + circle_points[k][i][0];
		    y = row + circle_points[k][i][1];
		    if (x < 0 || x >= pcol)
		      x = col;
		    if (y < 0 || y >= prow)
		      y = row;
		    
		    tti5 = (int) chunk_data16[z * slice_size + y * pcol + x];
		    tti5 = tti5 - mean_g;
		    if (tti5 < 0)
		      tti5 = -tti5;
		    count_obj = count_obj + scale_map[tti5];
		    count_nonobj = count_nonobj + 1.0 - scale_map[tti5];
		  }
		  if (100.0 * count_nonobj >= tolerance
		      * (count_nonobj + count_obj)) {
		    feature_scale[slice * slice_size + row * pcol + col] = k;
		    flag = 1;
		  }
		}
	      if (!flag)
		feature_scale[slice * slice_size + row * pcol + col] = k;
	    }
  }
} /* end of compute_feature_scale */

//void compute_feature_filtering()
//{
//  int i,j,k,iscale,tti3;
//  int xx,yy,zz,x,y,z;
//  int col, row, slice, col1, row1, slice1;
//  double tt1,tt2,inv_k,count;
//  double weight[MAX_SCALE][MAX_SCALE],temp;
//
//  /*
//  // non-scale based
//  for(i = 0;i<chunk_slice;i++)
//    for(j = 0;j<prow;j++)
//      for(k = 0;k<pcol;k++)
//	feature_scale[i*slice_size+j*pcol+k] = 1;
//  //
//  */
//  
//  for(i = 0;i<MAX_SCALE;i++)
//    for(j = 0;j<MAX_SCALE;j++)
//      weight[i][j] = 0;
//
//  for(i = 1;i<=MAX_SCALE;i++)
//    {
//      tt1 = (double)i*0.5;
//      tt2 = -0.5 / pow(tt1, 2.0);
//	
//      for(j = 0;j<i;j++)
//	{
//	  inv_k = exp(tt2 * pow((double)j, 2.0));
//	  weight[i-1][j] = inv_k;
//	}
//    }
//  
//  for (slice = 0; slice < chunk_slice; slice++)
//    for (row = 0; row < prow; row++)
//      for (col = 0; col < pcol; col++)
//	{
//	  if(num_of_bits == 8)
//	    tti3 = chunk_data8[slice * slice_size + row * pcol + col];
//	  else if(num_of_bits == 16)
//	    tti3 = chunk_data16[slice * slice_size + row * pcol + col];
//
//	  if(tti3 < cutoff_threshold)
//	    chunk_out[slice * slice_size + row * pcol + col] = 0;
//	  else
//	    {
//	      iscale = feature_scale[slice * slice_size + row * pcol + col];
//	      count = 0.0;
//	      temp = 0;
//		  
//	      for (k = 0; k < iscale; k++)
//		{
//		      tt1 = weight[iscale-1][k];
//		      
//		      for (i = 0; i < circle_no_points[k]; i++)
//			{
//			  xx = circle_points[k][i][0];
//			  yy = circle_points[k][i][1];
//
//			  x = col + xx;
//			  y = row + yy;
//			  z = slice;
//			  if (x >= 0 && y >= 0 &&  x < pcol && y < prow) 
//			    {
//				  if(num_of_bits == 8)
//				    tti3 = chunk_data8[z * slice_size + y * pcol + x];
//				  else if(num_of_bits == 16)
//				    tti3 = chunk_data16[z * slice_size + y * pcol + x];
//				  
//				  temp = temp + tt1*tti3;
//
//				  count = count + tt1;
//			    }
//			}
//		}
//	      chunk_out[slice * slice_size + row * pcol + col] = (int) temp/count;
//	    }
//	}
//
//  printf("filtering computation of process %d is done!\n", myrank);
//}
//
