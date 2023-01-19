/*
  Copyright 1993-2014 Medical Image Processing Group
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
#include <assert.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

int get_slices(int dim, short *list);
void bin_to_grey(unsigned char *bin_buffer, int length,
     unsigned char *grey_buffer, int min_value, int max_value);
void destroy_scene_header(ViewnixHeader *vh);
void get_co_occurrence(double **joint_hist, unsigned char *in, int bits,
	unsigned char *msk, int width, int height, int angle, int distance,
	int bin_size, int include_edge, int *max_dens, int *min_dens);
void get_LBP_hist(double *lbp_hist, unsigned char *in, int bits,
    unsigned char *msk, int width, int height, float radius, int nsamples,
	int include_edge);
int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,size2,error,slices1,bytes, subject, max_dens=0;
  int min_dens=65535;
  int include_edge=0, bin_size, angle, distance, nsamples, nbins, max_bins;
  ViewnixHeader vh1, vh2;
  FILE *in1, *in2;
  unsigned char *data1,*data2,*d1_8,*d2_8;
  char group[6],elem[6];
  float radius;
  double **joint_hist, *lbp_hist, count=0;

  if (argc>6 && strcmp(argv[argc-1], "-include_edge")==0)
  {
    include_edge = 1;
    argc--;
  }

  if (argc<6 || argc%2!=0 || (strcmp(argv[argc-3], "-LBP")==0?
      sscanf(argv[argc-2], "%f", &radius)!=1 ||
	  sscanf(argv[argc-1], "%d", &nsamples)!=1:
	  sscanf(argv[argc-3], "%d", &angle)!=1 ||
	  sscanf(argv[argc-2], "%d", &distance)!=1 ||
	  sscanf(argv[argc-1], "%d", &bin_size)!=1))
  {
    fprintf(stderr, "Usage: texture_stats <IM0_file> ... <BIM_file> ... [<angle> <distance> <bin size> | -LBP <radius> <# samples>] [-include_edge]\n");
    exit(-1);
  }

  if (strcmp(argv[argc-3], "-LBP"))
  {
    max_bins = 65536/bin_size;
    joint_hist = (double **)malloc(max_bins*sizeof(double *));
	for (j=0; j<max_bins; j++)
	{
	  joint_hist[j] = (double *)calloc(max_bins, sizeof(double));
	  if (joint_hist[j] == NULL)
	  {
	    fprintf(stderr, "Out of memory\n");
		exit(1);
	  }
	}
  }
  else
    lbp_hist = (double *)calloc(nsamples+2, sizeof(double));
  for (subject=0; subject<(argc-4)/2; subject++)
  {
    in1=fopen(argv[1+subject],"rb");
    in2=fopen(argv[(argc-2)/2+subject],"rb");

    if (in1==NULL || in2==NULL ) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }

    error=VReadHeader(in1,&vh1,group,elem);
    if (error<=104) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }

    error=VReadHeader(in2,&vh2,group,elem);
    if (error<=104) {
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

    if (VReadData((char*)data1,bytes,(int)(size1/bytes),in1,&j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    if (vh1.scn.num_of_bits==1)
      bin_to_grey(data1,size,d1_8+size,0,1);
    else
      memcpy(d1_8+size1, data1, size1);
    if (VReadData((char*)data2,1,size2,in2,&j)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    bin_to_grey(data2,size,d2_8+size,0,1);

    for(i=0;i<slices;i++) {
      memcpy(d1_8, d1_8+size1, size1);
      memcpy(d2_8, d2_8+size, size);
      if (i < slices-1)
      {
        if (VReadData((char*)data1,bytes,(int)(size1/bytes),in1,&j)) {
          fprintf(stderr, "Could not read data\n");
          exit(-1);
        }
        if (vh1.scn.num_of_bits==1)
          bin_to_grey(data1,size,d1_8+size,0,1);
        else
          memcpy(d1_8+size1, data1, size1);
      }

      if (i < slices1-1) {
        if (VReadData((char*)data2,1,size2,in2,&j)) {
          fprintf(stderr, "Could not read data\n");
          exit(-1);
        }
        bin_to_grey(data2,size,d2_8+size,0,1);
      }
      else
        memset(d2_8+size,0,size);

      if (strcmp(argv[argc-3], "-LBP"))
	  {
	    if (angle != 360)
		  get_co_occurrence(joint_hist, d1_8, vh1.scn.num_of_bits, d2_8,
		    vh1.scn.xysize[0], vh1.scn.xysize[1], angle, distance, bin_size,
		    include_edge, &max_dens, &min_dens);
	    else
		  for (angle=0; angle!=360; angle+=45)
		    get_co_occurrence(joint_hist, d1_8, vh1.scn.num_of_bits, d2_8,
		      vh1.scn.xysize[0], vh1.scn.xysize[1], angle, distance, bin_size,
		      include_edge, &max_dens, &min_dens);
	  }
	  else
	    get_LBP_hist(lbp_hist, d1_8, vh1.scn.num_of_bits, d2_8,
		  vh1.scn.xysize[0], vh1.scn.xysize[1], radius, nsamples,
		  include_edge);
    }

	free(data1);
	free(d1_8);
	free(data2);
	free(d2_8);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
	fclose(in1);
	fclose(in2);
  }

  if (strcmp(argv[argc-3], "-LBP")) // co-occurrence
  {
	double mu_x, mu_y, mu2_x, mu2_y, cur_count;
	int h;

    nbins = max_dens/bin_size+1;
	printf("%d bins from %d\n", nbins-min_dens/bin_size,
	  min_dens-min_dens%bin_size);
    count = 0;
	for (i=min_dens/bin_size; i<nbins; i++)
	  for (j=min_dens/bin_size; j<nbins; j++)
	    count += joint_hist[i][j];
	for (i=min_dens/bin_size; i<nbins; i++)
	{
	  for (j=min_dens/bin_size; j<nbins; j++)
	  {
		printf(" %f", joint_hist[i][j]/count);
	  }
	  printf("\n");
	}
	printf("energy: ");
	cur_count = 0;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
        cur_count += joint_hist[h][i]*joint_hist[h][i];
    printf("%f\n", cur_count/(count*count));
	printf("entropy: ");
	cur_count = 0;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
        if (joint_hist[h][i])
          cur_count += joint_hist[h][i]*log(1/count*joint_hist[h][i]);
    printf("%f\n", -M_E/((nbins-min_dens/bin_size)*(nbins-min_dens/bin_size))/
		count*cur_count);
	printf("maximum probability: ");
	cur_count = 0;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
        if (joint_hist[h][i] > cur_count)
          cur_count = joint_hist[h][i];
    printf("%f\n", 1/count*cur_count);
	printf("contrast: ");
	cur_count = 0;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
        if (i != h)
          cur_count += joint_hist[h][i]*(i-h)*(i-h);
    printf("%f\n", cur_count/count);
	printf("inverse difference moment: ");
	cur_count = 0;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
        if (i != h)
          cur_count += joint_hist[h][i]/((i-h)*(i-h));
    printf("%f\n", 1/count*cur_count);
	printf("correlation: ");
	cur_count = 0;
    mu_x = mu_y = mu2_x = mu2_y = 0;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
      {
        if (h)
          mu_x += joint_hist[h][i]*h;
        if (i)
          mu_y += joint_hist[h][i]*i;
      }
    mu_x *= 1/count;
    mu_y *= 1/count;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
      {
        mu2_x += (h-mu_x)*(h-mu_x)*joint_hist[h][i];
        mu2_y += (i-mu_y)*(i-mu_y)*joint_hist[h][i];
      }
    mu2_x *= 1/count;
    mu2_y *= 1/count;
    for (h=min_dens/bin_size; h<nbins; h++)
      for (i=min_dens/bin_size; i<nbins; i++)
        cur_count += h*i*joint_hist[h][i];
    cur_count *= 1/count;
    printf("%f\n", (cur_count-mu_x*mu_y)/sqrt(mu2_x*mu2_y));
  }
  else // LBP
  {
    for (j=0; j<nsamples+2; j++)
	  count += lbp_hist[j];
	for (j=0; j<nsamples+2; j++)
	  printf("%f\n", lbp_hist[j]/count);
  }
  exit(0);
}


void get_co_occurrence(double **joint_hist, unsigned char *in, int bits,
	unsigned char *msk, int width, int height, int angle, int distance,
	int bin_size, int include_edge, int *max_dens, int *min_dens)
{
	int start_row, end_row, start_col, end_col, offset, cen_val, nei_val, j, k;

	switch (angle)
	{
		case 0:
			start_row = 0;
			end_row = height;
			start_col = 0;
			end_col = width-distance;
			offset = distance;
			break;
		case 45:
			start_row = distance;
			end_row = height;
			start_col = 0;
			end_col = width-distance;
			offset = distance*(1-width);
			break;
		case 90:
			start_row = distance;
			end_row = height;
			start_col = 0;
			end_col = width;
			offset = distance*-width;
			break;
		case 135:
			start_row = distance;
			end_row = height;
			start_col = distance;
			end_col = width;
			offset = distance*(-1-width);
			break;
		case 180:
			start_row = 0;
			end_row = height;
			start_col = distance;
			end_col = width;
			offset = distance*-1;
			break;
		case 225:
			start_row = 0;
			end_row = height-distance;
			start_col = distance;
			end_col = width;
			offset = distance*(-1+width);
			break;
		case 270:
			start_row = 0;
			end_row = height-distance;
			start_col = 0;
			end_col = width;
			offset = distance*width;
			break;
		case 315:
			start_row = 0;
			end_row = height-distance;
			start_col = 0;
			end_col = width-distance;
			offset = distance*(1+width);
			break;
		default:
			fprintf(stderr,
				"Angle must be one of 0, 45, 90, 135, 180, 225, 270, 315.\n");
			exit(-1);
	}
	for (j=start_row; j<end_row; j++)
		for (k=start_col; k<end_col; k++)
		{
			if (msk[width*j+k] && (include_edge || msk[width*j+k+offset]))
			{
				switch (bits)
				{
					case 1:
					case 8:
						cen_val = in[width*j+k];
						nei_val = in[width*j+k+offset];
						break;
					case 16:
						cen_val = ((unsigned short *)in)[width*j+k];
						nei_val = ((unsigned short *)in)[width*j+k+offset];
						break;
					default:
						fprintf(stderr, "%d-bit data not supported.\n", bits);
						exit(-1);
				}
				if (cen_val > *max_dens)
					*max_dens = cen_val;
				if (cen_val < *min_dens)
					*min_dens = cen_val;
				if (nei_val > *max_dens)
					*max_dens = cen_val;
				if (nei_val < *min_dens)
					*min_dens = cen_val;
				joint_hist[cen_val/bin_size][nei_val/bin_size]++;
			}
		}
}

void get_LBP_hist(double *lbp_hist, unsigned char *in, int bits,
	unsigned char *msk, int width, int height, float radius, int nsamples,
	int include_edge)
{
	int start_row=0, end_row=height, start_col=0, end_col=width, *offset,
		cen_val, *nei_val, j, k, m, ix, iy, transitions, up;
	float *fx, *fy;
	double tx, ty;
	unsigned short *in2=(unsigned short *)in;

	offset = (int *)malloc(nsamples*sizeof(int));
	nei_val = (int *)malloc(nsamples*sizeof(int));
	fx = (float *)malloc(nsamples*sizeof(float));
	fy = (float *)malloc(nsamples*sizeof(float));
	for (m=0; m<nsamples; m++)
	{
		tx = radius*cos(2*M_PI*m/nsamples);
		ty = radius*sin(2*M_PI*m/nsamples);
		ix = (int)floor(tx);
		iy = (int)floor(ty);
		fx[m] = (float)(tx-ix);
		fy[m] = (float)(ty-iy);
		if (ix>0 && fx[m]<=0)
		{
			ix--;
			fx[m] = 1;
		}
		if (iy>0 && fy[m]<=0)
		{
			iy--;
			fy[m] = 1;
		}
		if (ix<0 && fx[m]>=1)
		{
			ix++;
			fx[m] = 0;
		}
		if (iy<0 && fy[m]>=1)
		{
			iy++;
			fy[m] = 0;
		}
		if (-ix > start_col)
			start_col = -ix;
		if (-iy > start_row)
			start_row = -iy;
		if (width-ix < end_col)
			end_col = width-ix;
		if (height-iy < end_row)
			end_row = height-iy;
		offset[m] = width*iy+ix;
	}
	for (j=start_row; j<end_row; j++)
		for (k=start_col; k<end_col; k++)
			if (in[width*j+k])
			{
				if (!include_edge)
				{
					int in_flag = 1;
					for (m=0; m<nsamples; m++)
					{
					  if ((1-fy[m])*((1-fx[m])*msk[width*j+k+offset[m]]+
						                fx[m] *msk[width*j+k+offset[m]+1])+
						     fy[m] *((1-fx[m])*msk[width*j+k+offset[m]+width]+
						                fx[m] *msk[width*j+k+offset[m]+width+1]
					      ) < .5)
					  {
						in_flag = 0;
						break;
					  }
					}
					if (!in_flag)
						continue;
				}
				cen_val = bits==16? in2[width*j+k]:in[width*j+k];
				for (m=0; m<nsamples; m++)
				{
					float v;
					if (bits == 16)
					  v = (1-fy[m])*((1-fx[m])*in2[width*j+k+offset[m]]+
					                    fx[m] *in2[width*j+k+offset[m]+1])+
					         fy[m] *((1-fx[m])*in2[width*j+k+offset[m]+width]+
					                    fx[m] *in2[width*j+k+offset[m]+width+1]
					                );
					else
					  v = (1-fy[m])*((1-fx[m])*in[width*j+k+offset[m]]+
					                    fx[m] *in[width*j+k+offset[m]+1])+
					         fy[m] *((1-fx[m])*in[width*j+k+offset[m]+width]+
					                    fx[m] *in[width*j+k+offset[m]+width+1]
					                );
					nei_val[m] = v >= cen_val;
				}
				up = nei_val[nsamples-1];
				transitions = nei_val[0] != nei_val[nsamples-1];
				for (m=0; m<nsamples-1; m++)
				{
					if (nei_val[m])
						up++;
					if (nei_val[m] != nei_val[m+1])
						transitions++;
				}
				assert(transitions%2 == 0);
				if (transitions <= 2)
					lbp_hist[up]++;
				else
					lbp_hist[nsamples+1]++;
			}
	free(fy);
	free(fx);
	free(nei_val);
	free(offset);
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

