/*
  Copyright 1993-2016 Medical Image Processing Group
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
#include <cv3dv.h>

int get_slices(int dim, short *list);
void bin_to_grey(unsigned char *bin_buffer, int length,
     unsigned char *grey_buffer, int min_value, int max_value),
  Stat16(unsigned short *in, unsigned char *msk, int size),
  SSD16(unsigned short *in, unsigned char *msk, int xsize, int ysize),
  Stat8(unsigned char *in, unsigned char *msk, int size),
  SSD8(unsigned char *in, unsigned char *msk, int xsize, int ysize);
void destroy_scene_header(ViewnixHeader *vh);
void get_stats(double *mean, double *sd, double *mode, int *median,
    double *inlier_mean, double *inlier_sd, double *inhomogeneity,
    int *low_decile, int *high_decile);
void get_percentiles(int percentile_incr);
void get_stats2(double *mean, double *sd, double *mode, int *median,
	int *low_quartile, int *high_quartile, double *mu_2, double *mu_3,
	double *mu_4, double *peak_height, double *skewness, double *kurtosis,
	int bin_size);
void Stat_2D(unsigned char *in1, unsigned char *msk, unsigned char *in3,
    int size, int bytes1, int bytes3);
int get_percentile(double percentile);

static double sum_sqr=0.0,sum=0.0, ssd=0.0;
static double count=0, face_count=0;
static unsigned int min_dens=0xFFFFFFFF,max_dens=0x0, min_1, min_2;
static double dist[0x10000], *twoDhist;
static int percentiles[100], bin_size1, bin_size2, nbins1, nbins2;

int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,size2,size3,error,slices1,bytes, subject,nsubjects;
  int low_decile, high_decile, percentile_incr=0, bin_size=0, write_hist=0;
  int low_quartile, high_quartile, median;
  int orig_threshold[2], new_threshold[2], delta=0;
  ViewnixHeader vh1, vh2, vh3;
  FILE *in1, *in2, *in3;
  unsigned char *data1, *data2, *d1_8, *d2_8, *d3_8;
  char group[6],elem[6];
  double mean,sd,mode, inlier_mean, inlier_sd, inhomogeneity;
  double mu_2, mu_3, mu_4, skewness, kurtosis, peak_height, find_percentile=0;

  if (argc>6 && strcmp(argv[argc-4], "-adjust_threshold")==0 &&
      sscanf(argv[argc-3], "%d", orig_threshold)==1 &&
	  sscanf(argv[argc-2], "%d", orig_threshold+1)==1 &&
	  sscanf(argv[argc-1], "%d", &delta)==1)
    argc -= 4;
  if (argc>4 && strcmp(argv[argc-2], "-find_percentile")==0 &&
      sscanf(argv[argc-1], "%lf", &find_percentile)==1)
    argc -= 2;
  if (argc>3 && strcmp(argv[argc-1], "-write_hist")==0)
  {
    write_hist = 1;
	argc--;
  }
  if (argc>3 && sscanf(argv[argc-1], "-min=%d,%d", &min_1, &min_2)==2)
    argc--;
  else
    min_1 = min_2 = 0;
  if (argc>4 && sscanf(argv[argc-1], "-nbins=%dx%d", &nbins1, &nbins2)==2 &&
      sscanf(argv[argc-2], "-bin_size=%dx%d", &bin_size1, &bin_size2)==2)
  {
    argc -= 2;
	twoDhist = (double *)calloc(nbins1*nbins2, sizeof(double));
	if (twoDhist == NULL)
	{
	  fprintf(stderr, "Out of memory\n");
	  exit(-1);
	}
	write_hist = 1;
  }
  if (argc>3 && sscanf(argv[argc-1], "-bin_size=%d", &bin_size)==1)
    argc--;
  if (argc>3 && sscanf(argv[argc-1], "-increment=%d", &percentile_incr)==1)
    argc--;

  if (argc<3 || (argc-1)%(twoDhist? 3:2)!=0) {
    fprintf(stderr, "Usage: masked_stats <IM0_file> ... <BIM_file> ... [-increment=<n>] [-bin_size=<n>[x<n> -nbins=<n>x<n> [-min=<n>,<n>]]] [-write_hist] [-find_percentile <p>] [-adjust_threshold <orig_threshold> <orig_threshold> <delta>]\n");
    exit(-1);
  }

  nsubjects = (argc-1)/(twoDhist? 3:2);

  for (subject=0; subject<nsubjects; subject++)
  {
    in1=fopen(argv[1+subject],"rb");
	if (twoDhist)
	{
	  in2 = fopen(argv[1+2*nsubjects+subject], "rb");
	  in3 = fopen(argv[1+nsubjects+subject], "rb");
	}
	else
      in2 = fopen(argv[1+nsubjects+subject], "rb");

    if (in1==NULL || in2==NULL || (twoDhist && in3==NULL)) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }

    error=VReadHeader(in1,&vh1,group,elem);
    if (error && error<=104) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }

    error=VReadHeader(in2,&vh2,group,elem);
    if (error && error<=104) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }

    if (vh1.gen.data_type!=IMAGE0 || vh2.gen.data_type!= IMAGE0) {
      fprintf(stderr, "Both input files should be IMAGE0\n");
      exit(-1);
    }
    if (vh2.scn.num_of_bits!=1) {
      fprintf(stderr, "The Mask file should be binary\n");
      exit(-1);
    }
    if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] || vh1.scn.xysize[1]!=vh2.scn.xysize[1]) {
      fprintf(stderr, "Both Scenes should be of the same width and height\n");
      exit(-1);
    }

    slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
    slices1= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
    if (slices < slices1) { 
      fprintf(stderr,  "Mask Scene has more slices\n");
      fprintf(stderr,  "Ignoring the extra slices\n");
    }

    size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
    if (vh1.scn.num_of_bits==16)
      bytes=2;
    else
      bytes=1;

    size1= (size*vh1.scn.num_of_bits+7)/8;
    data1= (unsigned char *)malloc(size1);
    d1_8= (unsigned char *)malloc(size1*2);

    size2= (size*vh2.scn.num_of_bits+7)/8;
    data2= (unsigned char *)malloc(size2);
    d2_8 = (unsigned char *)malloc(size*2);

    VSeekData(in1,0);
    VSeekData(in2,0);

    if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    if (vh1.scn.num_of_bits==1)
      bin_to_grey(data1,size,d1_8+size,0,1);
    else
      memcpy(d1_8+size1, data1, size1);
    if (VReadData((char *)data2,1,size2,in2,&j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    bin_to_grey(data2,size,d2_8+size,0,1);

	if (twoDhist)
	{
	  error = VReadHeader(in3, &vh3, group,elem);
	  if (error && error<=104) {
	    fprintf(stderr, "Fatal error in reading header\n");
		exit(-1);
	  }

	  if (vh3.gen.data_type != IMAGE0)
	  {
	    fprintf(stderr, "Both input files should be IMAGE0\n");
		exit(-1);
	  }
	  if ((vh3.scn.num_of_bits!=8 && vh3.scn.num_of_bits!=16) ||
	      vh3.scn.xysize[0]!=vh1.scn.xysize[0] ||
		  vh3.scn.xysize[1]!=vh1.scn.xysize[1] ||
		  get_slices(vh3.scn.dimension, vh3.scn.num_of_subscenes)!=slices)
	  {
	    fprintf(stderr, "Problem with matching scene\n");
		exit(-1);
	  }
	  size3 = size*vh3.scn.num_of_bits/8;
	  d3_8= (unsigned char *)malloc(size3*2);
	  VSeekData(in3,0);
	  if (VReadData((char *)d3_8+size3, vh3.scn.num_of_bits/8, (int)size, in3,
	      &j))
	  {
	    fprintf(stderr, "Could not read data\n");
		exit(-1);
	  }
	}

    for(i=0;i<slices;i++) {
      memcpy(d1_8, d1_8+size1, size1);
      memcpy(d2_8, d2_8+size, size);
	  if (twoDhist)
	    memcpy(d3_8, d3_8+size3, size3);
      if (i < slices-1)
      {
        if (VReadData((char *)data1,bytes,(int)(size1/bytes),in1,&j)) {
          fprintf(stderr, "Could not read data\n");
          exit(-1);
        }
        if (vh1.scn.num_of_bits==1)
          bin_to_grey(data1,size,d1_8+size,0,1);
        else
          memcpy(d1_8+size1, data1, size1);
        if (twoDhist)
		{
		  if (VReadData((char *)d3_8+size3, vh3.scn.num_of_bits/8, (int)size,
		      in3, &j))
		  {
		    fprintf(stderr, "Could not read data\n");
			exit(-1);
		  }
		}
      }

      if (i < slices1-1) {
        if (VReadData((char *)data2,1,size2,in2,&j)) {
          fprintf(stderr, "Could not read data\n");
          exit(-1);
        }
        bin_to_grey(data2,size,d2_8+size,0,1);
      }
      else
        memset(d2_8+size,0,size);

      if (twoDhist)
	  {
	    Stat_2D(d1_8, d2_8, d3_8, size, bytes, vh3.scn.num_of_bits/8);
	  }
	  else if (vh1.scn.num_of_bits==16)
      {
        Stat16((unsigned short *)d1_8,d2_8,size);
        SSD16((unsigned short *)d1_8, d2_8, vh1.scn.xysize[0], vh1.scn.xysize[1]);
      }
      else
      {
        Stat8(d1_8,d2_8,size);
        SSD8(d1_8, d2_8, vh1.scn.xysize[0], vh1.scn.xysize[1]);
      }

    }

	free(data1);
	free(d1_8);
	free(data2);
	free(d2_8);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
	fclose(in1);
	fclose(in2);
	if (twoDhist)
	{
	  free(d3_8);
	  destroy_scene_header(&vh3);
	  fclose(in3);
	}
  }


  if (count) {
    if (delta > 0)
	{
	  double cur_count, best_count=0;
	  int w=orig_threshold[1]-orig_threshold[0]+1;
	  int f;
	  new_threshold[1] = orig_threshold[1];
	  i = orig_threshold[0]-delta;
	  if (i < (int)min_dens)
	    i = min_dens;
	  f = orig_threshold[0]+delta;
	  if (f > (int)max_dens-w+1)
	    f = max_dens-w+1;
	  for (new_threshold[0]=i; new_threshold[0]<=f; new_threshold[0]++)
	  {
	    cur_count = 0;
		for (j=new_threshold[0]; j<new_threshold[0]+w; j++)
		  cur_count += dist[j];
		if (cur_count > best_count)
		{
		  best_count = cur_count;
		  new_threshold[1] = new_threshold[0]+w-1;
		}
	  }
	  new_threshold[0] = new_threshold[1]-w+1;
	  printf("new threshold = [%d, %d]\n", new_threshold[0], new_threshold[1]);
	  exit(0);
	}
    if (find_percentile > 0)
	{
	  printf("%d\n", get_percentile(find_percentile));
	  exit(0);
	}
    if (write_hist)
	{
	  printf("Histogram\n");
	  if (twoDhist)
	  {
	    printf("bin_size=%dx%d nbins=%dx%d min=%d,%d\n", bin_size1, bin_size2,
		  nbins1, nbins2, min_1, min_2);
		for (j=0; j<nbins1*nbins2; j++)
		  printf(" %.0f\n", twoDhist[j]);
		printf("End histogram\n");
	  }
	  else if (bin_size)
	  {
	    double cur_count=0;
		int k=0;
		for (j=min_dens; j<=(int)max_dens; j++)
        {
          cur_count += dist[j];
          if (k==bin_size-1 || j==max_dens)
          {
		    printf(" %.0f\n", cur_count);
            cur_count = k = 0;
          }
          else
            k++;
        }
      }
	  else
	  {
	    for (i=min_dens; i<(int)max_dens; i++)
	      printf(" %.0f ,", dist[i]);
	    printf(" %.0f\n", dist[i]);
	  }
	  exit(0);
	}
    if (bin_size)
	{
	  get_stats2(&mean, &sd, &mode, &median, &low_quartile, &high_quartile,
	    &mu_2, &mu_3, &mu_4, &peak_height, &skewness, &kurtosis, bin_size);

      printf("Total number of pixels retained -> %f\n",count);
      printf("Minimum intensity               -> %u\n",min_dens);
      printf("Maximum intensity               -> %u\n",max_dens);
      printf("Average intensity               -> %lf\n",mean);
      printf("Standard Dev.                   -> %lf\n",sd);
      printf("Mode                            -> %d\n",(int)mode);
      printf("Median                          -> %d\n",median);
      printf("Low quartile                    -> %d\n",low_quartile);
      printf("High quartile                   -> %d\n",high_quartile);
      printf("Second moment                   -> %lf\n",mu_2);
      printf("Third moment                    -> %lf\n",mu_3);
	  printf("Fourth moment                   -> %lf\n",mu_4);
      printf("Skewness                        -> %lf\n",skewness);
	  printf("Kurtosis                        -> %lf\n",kurtosis);
      printf("Peak height                     -> %lf%%\n",peak_height);
      exit(0);
	}
    if (percentile_incr)
	{
	  get_percentiles(percentile_incr);
	  for (j=percentile_incr; j<100; j+=percentile_incr)
	    printf("%d\n", percentiles[j]);
	  exit(0);
	}
    get_stats(&mean,&sd,&mode,&median,&inlier_mean,&inlier_sd, &inhomogeneity,
	  &low_decile, &high_decile);

    printf("Total number of pixels retained -> %f\n",count);
    printf("Minimum intensity               -> %u\n",min_dens);
    printf("Maximum intensity               -> %u\n",max_dens);
    printf("Average intensity               -> %lf\n",mean);
    printf("Standard Dev.                   -> %lf\n",sd);
    printf("Mode                            -> %d\n",(int)mode);
    printf("Median                          -> %d\n",median);
    printf("Average inlier intensity        -> %lf\n",inlier_mean);
    printf("Inlier standard Dev.            -> %lf\n",inlier_sd);
    printf("Inhomogeneity                   -> %lf\n",inhomogeneity);
	printf("Low decile                      -> %d\n",low_decile);
	printf("High decile                     -> %d\n",high_decile);
  }
  else {
    fprintf(stderr, "Mask is empty. No stats taken.\n");
  }

  exit(0);
}

void get_percentiles(int percentile_incr)
{
  unsigned int j, k;
  double cur_count;

  for (j=percentile_incr-1; j<100; j+=percentile_incr)
    percentiles[j] = 0x10000;
  cur_count=0;
  for(j=min_dens;j<=max_dens;j++) {
    for (k=percentile_incr; k<100; k+=percentile_incr)
	  if (cur_count<(count*k+percentile_incr/2)/100 &&
	      cur_count+dist[j]>(count*k+percentile_incr/2-1)/100)
	    percentiles[k] = j;
    cur_count+= dist[j];
  }
}

int get_percentile(double percentile)
{
  unsigned int j;
  double cur_count, target_count=count*percentile/100;

  cur_count=0;
  for (j=min_dens; j<=max_dens; j++)
  {
    cur_count += dist[j];
    if (cur_count >= target_count)
	    return j;
  }
  return max_dens;
}

void get_stats(double *mean, double *sd, double *mode, int *median,
    double *inlier_mean, double *inlier_sd, double *inhomogeneity,
    int *low_decile, int *high_decile)
{
  unsigned int i;
  double max_count, cur_count;
  double inlier_sum, inlier_sum_sqr, inlier_count;

  *mean= sum/count;
  *sd  = sqrt( sum_sqr/count - *mean * *mean );

  max_count=0;
  *mode=0;
  cur_count=0;
  inlier_count = 0;
  inlier_sum = inlier_sum_sqr = 0;
  get_percentiles(10);
  for(i=min_dens;i<=max_dens;i++) {
    if (dist[i] > max_count)  {
      max_count=dist[i];
      *mode=i;
    }
    cur_count+= dist[i];
    if ((int)i>=percentiles[10] && (int)i<=percentiles[90])
    {
      inlier_sum += dist[i]*(double)i;
      inlier_sum_sqr += dist[i]*(double)i*i;
      inlier_count += dist[i];
    }
  }
  *median = percentiles[50];
  *inlier_mean = inlier_sum/inlier_count;
  *inlier_sd = sqrt(inlier_sum_sqr/inlier_count - *inlier_mean * *inlier_mean);
  *inhomogeneity = sqrt(ssd/face_count);
  *low_decile = percentiles[10];
  *high_decile = percentiles[90];
}

void get_stats2(double *mean, double *sd, double *mode, int *median,
	int *low_quartile, int *high_quartile, double *mu_2, double *mu_3,
	double *mu_4, double *peak_height, double *skewness, double *kurtosis,
	int bin_size)
{
  unsigned int j, k, imode;
  double cur_count;

  *mean= sum/count;
  *sd  = sqrt( sum_sqr/count - *mean * *mean );

  *peak_height = 0;
  cur_count = 0;
  *mode = 0;
  imode = 0;
  k = 0;
  for (j=min_dens; j<=max_dens; j++)
  {
    cur_count += dist[j];
	if (dist[j] > dist[imode])
	  imode = j;
	if (k==bin_size-1 || j==max_dens)
	{
	  if (cur_count > *peak_height)
	  {
	    *peak_height = cur_count;
		*mode = imode;
	  }
	  cur_count = k = 0;
	  imode = j+1;
	}
	else
	  k++;
  }

  cur_count = 0;
  *mu_2 = 0;
  *mu_3 = 0;
  *mu_4 = 0;
  for (j=min_dens; j<=max_dens; j++) {
    cur_count+= dist[j];
	*mu_2 += (j-*mean)*(j-*mean)*dist[j];
	*mu_3 += (j-*mean)*(j-*mean)*(j-*mean)*dist[j];
	*mu_4 += (j-*mean)*(j-*mean)*(j-*mean)*(j-*mean)*dist[j];
  }
  get_percentiles(25);
  *median = percentiles[50];
  *low_quartile = percentiles[25];
  *high_quartile = percentiles[75];
  *mu_2 /= sum;
  *mu_3 /= sum;
  *mu_4 /= sum;
  *peak_height *= 100/count;
  *skewness = *mu_3/pow(*mu_2, 1.5);
  *kurtosis = *mu_4/(*mu_2 * *mu_2) - 3;
}




void Stat_2D(unsigned char *in1, unsigned char *msk, unsigned char *in3,
	int size, int bytes1, int bytes3)
{
  unsigned short in1_16, in3_16;
  int j;

  for (j=0; j<size; msk++,in1+=bytes1,in3+=bytes3,j++)
    if (*msk)
	{
	  in1_16 = bytes1==1? *in1: *((unsigned short *)in1);
	  in3_16 = bytes3==1? *in3: *((unsigned short *)in3);
	  if (in1_16>=min_1 && in1_16<min_1+bin_size1*nbins1 &&
	      in3_16>=min_2 && in3_16<min_2+bin_size2*nbins2)
	  {
		twoDhist[nbins2*((in1_16-min_1)/bin_size1)+(in3_16-min_2)/bin_size2]++;
		count++;
	  }
	}
}

void Stat16(unsigned short *in, unsigned char *msk, int size)
{
  int i;

  for(i=0;i<size;msk++,in++,i++)
    if (*msk) {
      if (*in > max_dens) max_dens= *in;
      if (*in < min_dens) min_dens= *in;
      dist[*in]++;
      sum += *in;
      sum_sqr += (double)(*in) * (*in);
      count++;
    }

}


void Stat8(unsigned char *in, unsigned char *msk, int size)
{
  int i;

  for(i=0;i<size;msk++,in++,i++)
    if (*msk)  {
      if (*in > max_dens) max_dens= *in;
      if (*in < min_dens) min_dens= *in;
      dist[*in]++;
      sum += *in;
      sum_sqr += (double)(*in) * (*in);
      count++;
    }
  
}


void SSD16(unsigned short *in, unsigned char *msk, int xsize, int ysize)
{
  int j, k;

  for (j=0; j<ysize-1; j++)
    for (k=0; k<xsize-1; k++)
      if (msk[xsize*j+k])
      {
        if (msk[xsize*ysize+xsize*j+k])
        {
          ssd +=
            ((double)in[xsize*ysize+xsize*j+k]-
                     in[xsize*j+k])*
            ((double)in[xsize*ysize+xsize*j+k]-
                     in[xsize*j+k]);
          face_count++;
        }
        if (msk[xsize*(j+1)+k])
        {
          ssd +=
            ((double)in[xsize*(j+1)+k]-
                     in[xsize*j+k])*
            ((double)in[xsize*(j+1)+k]-
                     in[xsize*j+k]);
          face_count++;
        }
        if (msk[xsize*j+k+1])
        {
          ssd +=
            ((double)in[xsize*j+k+1]-
                     in[xsize*j+k])*
            ((double)in[xsize*j+k+1]-
                     in[xsize*j+k]);
          face_count++;
        }
      }
}

void SSD8(unsigned char *in, unsigned char *msk, int xsize, int ysize)
{
  int j, k;

  for (j=0; j<ysize-1; j++)
    for (k=0; k<xsize-1; k++)
      if (msk[xsize*j+k])
      {
        if (msk[xsize*ysize+xsize*j+k])
        {
          ssd +=
            ((double)in[xsize*ysize+xsize*j+k]-
                     in[xsize*j+k])*
            ((double)in[xsize*ysize+xsize*j+k]-
                     in[xsize*j+k]);
          face_count++;
        }
        if (msk[xsize*(j+1)+k])
        {
          ssd +=
            ((double)in[xsize*(j+1)+k]-
                     in[xsize*j+k])*
            ((double)in[xsize*(j+1)+k]-
                     in[xsize*j+k]);
          face_count++;
        }
        if (msk[xsize*j+k+1])
        {
          ssd +=
            ((double)in[xsize*j+k+1]-
                     in[xsize*j+k])*
            ((double)in[xsize*j+k+1]-
                     in[xsize*j+k]);
          face_count++;
        }
      }
}



int get_slices(int dim, short *list)
{
  int i,sm;

  if (dim==3) return (int)(list[0]);
  if (dim==4) {
    for(sm=0,i=0;i<list[0];i++)
      sm+= list[1+i];
    return(sm);
  }
  return(0);

}

void bin_to_grey(unsigned char *bin_buffer, int length,
     unsigned char *grey_buffer, int min_value, int max_value)
{
  register int i, j;
  static unsigned char mask[8]= { 1,2,4,8,16,32,64,128 };
  unsigned char *bin, *grey;
  
  bin = bin_buffer;
  grey = grey_buffer;
  
  
  for(j=7,i=length; i>0; i--)    {
    if( (*bin & mask[j]) != 0) 
      *grey = max_value;
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

