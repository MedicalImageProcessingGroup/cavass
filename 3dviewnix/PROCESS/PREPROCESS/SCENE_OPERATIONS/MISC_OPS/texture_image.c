/*
  Copyright 1993-2015, 2018 Medical Image Processing Group
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
#include <cv3dv.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

void bin_to_grey(unsigned char *bin_buffer, int length,
     unsigned char *grey_buffer, int min_value, int max_value);
void destroy_scene_header(ViewnixHeader *vh);
void get_co_occurrence(double **joint_hist, unsigned char *in, int bytes,
	int width, int height, int angle, int distance,
	int window_size, int x, int y, unsigned short gray_map[0x10000]);
void get_RLM(double **joint_hist, unsigned char *in, int bytes,
	int width, int height, int angle,
    int window_size, int x, int y,
    unsigned short gray_map[0x10000], int *run_map);
void get_LBP_image(unsigned char *out_8, unsigned char *in, int bytes,
    int width, int height, float radius, int nsamples);
int get_slices(int dim, short *list);
void compute_run_bins(int *bin_end[2], int window_size, int num_run_bins);

int main(argc,argv)
int argc;
char *argv[];
{

  int i,j,slices,size,size1,size2,error,slices2,bytes, max_dens=0, out_bytes;
  int angle, distance, nsamples, nbins, *bin_limit, window_size, feature;
  int gray_low, gray_high;
  ViewnixHeader vh1, vh2;
  FILE *in1, *in2, *out;
  unsigned char *data1,*data2,*d1_8,*d2_8, *data_out_8;
  unsigned short *data_out_16;
  char group[6],elem[6];
  float radius;
  double **joint_hist, count=0, cur_count;
  static double gray_hist[0x10000];
  static unsigned short gray_map[0x10000];
  int *run_bin_end[2]; // along axis / diagonal
  int rl_feature=0;
  int *run_map[2]; // along axis / diagonal

  if (!((argc>=8 && argc<=11 && sscanf(argv[3], "%d", &angle)==1 &&
      sscanf(argv[4], "%d", &distance)==1 &&
	  sscanf(argv[5], "%d", &nbins)==1 &&
	  sscanf(argv[6], "%d", &window_size)==1 &&
	  (sscanf(argv[7], "%d", &feature)==1 ||
       sscanf(argv[7], "RF%d", &rl_feature)==1)) ||
	  (argc==5 && sscanf(argv[3], "%f", &radius)==1 &&
	  sscanf(argv[4], "%d", &nsamples)==1)))
  {
    fprintf(stderr, "Usage: texture_image <IM0_in> <IM0_out> [<angle> [<distance> | <rl_bins>] <bins> <window_size> <feature>  [[<BIM_in> | <low> <high> [<BIM_in>]]] | <radius> <# samples>]\n");
	fprintf(stderr, "feature 1=energy, 2=entropy, 3=maximum probability, 4=contrast, 5=inverse difference moment, 6=correlation\n");
	fprintf(stderr, "feature RF1=short runs emphasis, RF2=long runs emphasis, RF3=gray level nonuniformity, RF4=run length nonuniformity, RF5=run percentage\n");
    exit(-1);
  }

  if (rl_feature)
  {
    run_bin_end[0] = (int *)malloc(distance*sizeof(int));
	run_bin_end[1] = (int *)malloc(distance*sizeof(int));
    compute_run_bins(run_bin_end, window_size*2+1, distance);
	run_map[0] = (int *)malloc((window_size*2+2)*sizeof(int));
    run_map[1] = (int *)malloc((window_size*2+2)*sizeof(int));
	for (i=0,j=1; j<=window_size*2+1; j++)
    {
      run_map[0][j] = i;
      if (run_bin_end[0][i] == j)
        i++;
    }
	for (i=0,j=1; j<=window_size*2+1; j++)
    {
      run_map[1][j] = i;
      if (run_bin_end[1][i] == j)
        i++;
    }
  }
  if (argc<8 && nsamples>254)
  {
    fprintf(stderr, "Too many samples\n");
	exit(-1);
  }
  in1 = fopen(argv[1], "rb");
  if (in1 == NULL)
  {
    fprintf(stderr, "Cannot open %s\n", argv[1]);
    exit(-1);
  }
  error=VReadHeader(in1,&vh1,group,elem);
  if (vh1.gen.data_type != IMAGE0)
  {
    fprintf(stderr, "Input files should be IMAGE0\n");
    exit(-1);
  }
  if (error && error<=104) {
    fprintf(stderr, "Fatal error in reading header\n");
    exit(-1);
  }
  slices=get_slices(vh1.scn.dimension,vh1.scn.num_of_subscenes);
  size = vh1.scn.xysize[0]* vh1.scn.xysize[1];
  if (vh1.scn.num_of_bits==16)
    bytes=2;
  else
    bytes=1;
  size1= (size*vh1.scn.num_of_bits+7)/8;
  data1= (unsigned char *)malloc(size1);
  d1_8= (unsigned char *)malloc(size1);
  VSeekData(in1,0);
  if (argc > 5)
  {
    bin_limit = (int *)malloc(nbins*sizeof(int));
    if (argc==9 || argc==11)
    {
      in2 = fopen(argv[argc-1], "rb");
      if (in2 == NULL)
      {
        fprintf(stderr, "Cannot open %s\n", argv[argc-1]);
        exit(-1);
      }
      error=VReadHeader(in2,&vh2,group,elem);
      if (vh2.gen.data_type!= IMAGE0)
      {
        fprintf(stderr, "Input files should be IMAGE0\n");
        exit(-1);
      }
      if (error<=104) {
        fprintf(stderr, "Fatal error in reading header\n");
        exit(-1);
      }
      if (vh2.scn.num_of_bits!=1) {
        fprintf(stderr, "The Mask file should be binary\n");
        exit(-1);
      }
      if (vh1.scn.xysize[0]!=vh2.scn.xysize[0] ||
          vh1.scn.xysize[1]!=vh2.scn.xysize[1]) {
        fprintf(stderr,
          "Both Scenes should be of the same width and height\n");
        exit(-1);
      }
      slices2= get_slices(vh2.scn.dimension,vh2.scn.num_of_subscenes);
      if (slices < slices2) { 
        fprintf(stderr,  "Mask Scene has more slices\n");
        fprintf(stderr,  "Ignoring the extra slices\n");
      }
      size2= (size*vh2.scn.num_of_bits+7)/8;
      data2= (unsigned char *)malloc(size2);
      d2_8 = (unsigned char *)malloc(size);
      VSeekData(in2,0);
    }
    if (argc >= 10)
    {
      if (sscanf(argv[8], "%d", &gray_low)!=1 || gray_low<0 ||
          sscanf(argv[9], "%d", &gray_high)!=1 || gray_high>65535)
      {
        fprintf(stderr, "Low and high must be integer.\n");
        exit(-1);
      }
      for (j=gray_low; j<=gray_high; j++)
        gray_hist[j] = 1;
    }
    else
    {
      if (argc == 9)
      {
        for(i=0;i<slices;i++) {
          if (VReadData((char*)data1,bytes,(int)(size1/bytes),in1,&j)) {
            fprintf(stderr, "Could not read data\n");
            exit(-1);
          }
          if (vh1.scn.num_of_bits==1)
            bin_to_grey(data1,size,d1_8,0,1);
          else
            memcpy(d1_8, data1, size1);
          if (i < slices2) {
            if (VReadData((char*)data2,1,size2,in2,&j)) {
              fprintf(stderr, "Could not read data\n");
              exit(-1);
            }
            bin_to_grey(data2,size,d2_8,0,1);
          }
          else
            memset(d2_8,0,size);
          for (j=0; j<size; j++)
            if (d2_8[j])
              gray_hist[vh1.scn.num_of_bits==16? ((unsigned short *)d1_8)[j]:
                d1_8[j]]++;
        }
      }
      else
      {
        for(i=0; i<slices; i++)
        {
          if (VReadData((char*)data1,bytes,(int)(size1/bytes),in1,&j)) {
            fprintf(stderr, "Could not read data\n");
            exit(-1);
          }
          if (vh1.scn.num_of_bits==1)
            bin_to_grey(data1,size,d1_8,0,1);
          else
            memcpy(d1_8, data1, size1);
          for (j=0; j<size; j++)
            gray_hist[vh1.scn.num_of_bits==16? ((unsigned short *)d1_8)[j]:
              d1_8[j]]++;
        }
      }
    }
    for (j=0; j<0x10000; j++)
      if (gray_hist[j])
      {
        count += gray_hist[j];
        max_dens = j;
      }
    for (j=0; j<nbins; j++)
      bin_limit[j] = 0x10000;
    cur_count = 0;
    for (j=0; j<=max_dens; j++)
    {
      for (i=0; i<nbins-1; i++)
        if (cur_count<(i+1)*(count/nbins) &&
            cur_count+gray_hist[j]>=(i+1)*(count/nbins))
          bin_limit[i] = j;
      cur_count += gray_hist[j];
    }
    for (i=j=0; j<0x10000; j++)
    {
      gray_map[j] = i;
      if (bin_limit[i] == j)
      {
        i++;
        printf("gray level = %d\n", j);
      }
    }
    joint_hist =
      (double **)malloc((rl_feature? distance:nbins)*sizeof(double *));
    for (j=0; j<(rl_feature? distance:nbins); j++)
    {
      joint_hist[j] = (double *)calloc(nbins, sizeof(double));
      if (joint_hist[j] == NULL)
      {
        fprintf(stderr, "Out of memory\n");
        exit(1);
      }
    }
  }

  out = fopen(argv[2], "w+b");
  if (out == NULL)
  {
    fprintf(stderr, "Cannot open %s\n", argv[2]);
    exit(-1);
  }
  vh1.scn.smallest_density_value[0] = 0;
  if (argc < 8) // LBP
    vh1.scn.largest_density_value[0] = (float)(nsamples+1);
  else // co-occurrence
    vh1.scn.largest_density_value[0] = 65535;
  strncpy(vh1.gen.filename, argv[2], sizeof(vh1.gen.filename));
  if (vh1.scn.largest_density_value[0] <= 255)
  {
    out_bytes = 1;
    vh1.scn.num_of_bits = 8;
    data_out_8 = (unsigned char *)malloc(size);
  }
  else
  {
    out_bytes = 2;
    vh1.scn.num_of_bits = 16;
    data_out_16 = (unsigned short *)malloc(size*2);
    data_out_8 = (unsigned char *)data_out_16;
  }
  vh1.scn.bit_fields[1] = vh1.scn.num_of_bits-1;
  error=VWriteHeader(out,&vh1,group,elem);
  if (error>0 && error<=104)
  {
    fprintf(stderr, "Fatal error in writing header\n");
    exit(1);
  }
  if (argc == 9)
  {
    free(data2);
    free(d2_8);
    fclose(in2);
    destroy_scene_header(&vh2);
  }
  VSeekData(in1,0);
  for (j=0; j<slices; j++)
  {
    if (VReadData((char*)data1,bytes,(int)(size1/bytes),in1,&i)) {
      fprintf(stderr, "Could not read data\n");
      exit(-1);
    }
    if (vh1.scn.num_of_bits==1)
      bin_to_grey(data1,size,d1_8,0,1);
    else
      memcpy(d1_8, data1, size1);
    if (argc < 8) // LBP
      get_LBP_image(data_out_8, d1_8, bytes,
        vh1.scn.xysize[0], vh1.scn.xysize[1], radius, nsamples);
    else // co-occurrence or run length
    {
      int x, y, h;
	  double ps;
      if (argc == 11)
      {
          if (j < slices2) {
            if (VReadData((char*)data2, 1, size2, in2, &i)) {
              fprintf(stderr, "Could not read data\n");
              exit(-1);
            }
            bin_to_grey(data2,size,d2_8,0,1);
          }
          else
            memset(d2_8,0,size);
      }
      for (y=0; y<vh1.scn.xysize[1]; y++)
        for (x=0; x<vh1.scn.xysize[0]; x++)
        {
          double mu_x, mu_y, mu2_x, mu2_y, ov;
          if (argc==11 && d2_8[vh2.scn.xysize[0]*y+x]==0)
		  {
		    data_out_16[y*vh1.scn.xysize[0]+x] = 0;
			continue;
		  }
          if (rl_feature)
          {
            if (y<window_size || y>=vh1.scn.xysize[1]-window_size ||
                x<window_size || x>=vh1.scn.xysize[0]-window_size)
            {
		      data_out_16[y*vh1.scn.xysize[0]+x] = 0;
			  continue;
            }
            if (angle > 180)
              angle -= 180;
            for (h=0; h<distance; h++)
              for (i=0; i<nbins; i++)
                joint_hist[h][i] = 0;
            if (angle == 180)
              for (h=0; h<180; h+=45)
                get_RLM(joint_hist, d1_8, bytes,
                  vh1.scn.xysize[0], vh1.scn.xysize[1], h,
                  2*window_size+1, x-window_size, y-window_size,
                  gray_map, run_map[h/45%2]);
            else
              get_RLM(joint_hist, d1_8, bytes,
                vh1.scn.xysize[0], vh1.scn.xysize[1], angle,
                2*window_size+1, x-window_size, y-window_size,
                gray_map, run_map[angle/45%2]);
            // compute feature
            cur_count = count = 0;
            for (h=0; h<distance; h++)
              for (i=0; i<nbins; i++)
                count += joint_hist[h][i];
            assert(count <=
              (2*window_size+1)*(2*window_size+1)*(angle<180? 1:4));
            switch (rl_feature)
            {
              case 1: // short runs emphasis
                for (h=0; h<distance; h++)
                  for (i=0; i<nbins; i++)
                    cur_count += joint_hist[h][i]/((h+1)*(h+1));
                ov = 65535./count*cur_count;
                if (ov > 65535)
                  ov = 65535;
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(ov);
                break;
              case 2: // long runs emphasis
                for (h=0; h<distance; h++)
                  for (i=0; i<nbins; i++)
                    cur_count += joint_hist[h][i]*((h+1)*(h+1));
                ov = 65535./(distance*distance)/count*cur_count;
                if (ov > 65535)
                  ov = 65535;
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(ov);
                break;
              case 3: // gray level nonuniformity
                for (i=0; i<nbins; i++)
                {
                  ps = 0;
                  for (h=0; h<distance; h++)
                    ps += joint_hist[h][i];
                  cur_count += ps*ps;
                }
                h = 2*window_size+1;
                ov = 65535./(angle<180? 1:4)/h/h/count*cur_count;
                if (ov > 65535)
                  ov = 65535;
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(ov);
                break;
              case 4: // run length nonuniformity
                for (h=0; h<distance; h++)
                {
                  ps = 0;
                  for (i=0; i<nbins; i++)
                    ps += joint_hist[h][i];
                  cur_count += ps*ps;
                }
                h = 2*window_size+1;
                ov = 65535./h/h/count*cur_count;
                if (ov > 65535)
                  ov = 65535;
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(ov);
                break;
              case 5:
                h = 2*window_size+1;
                ov = 65535./(angle<180? 1:4)/h/h*count;
                if (ov > 65535)
                  ov = 65535;
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(ov);
                break;
              default:
                fprintf(stderr, "Feature %d not implemented.\n", feature);
                exit(-1);
            }
          }
          else // co_occurrence
          {
            for (h=0; h<nbins; h++)
              for (i=0; i<nbins; i++)
                joint_hist[h][i] = 0;
            if (angle == 360)
              for (h=0; h<360; h+=45)
                get_co_occurrence(joint_hist, d1_8, bytes,
                  vh1.scn.xysize[0], vh1.scn.xysize[1], h, distance,
                  window_size, x, y, gray_map);
            else
              get_co_occurrence(joint_hist, d1_8, bytes,
                vh1.scn.xysize[0], vh1.scn.xysize[1], angle, distance,
                window_size, x, y, gray_map);
            // compute feature
            cur_count = count = 0;
            for (h=0; h<nbins; h++)
              for (i=0; i<nbins; i++)
                count += joint_hist[h][i];
            assert(count <=
              (2*window_size+1)*(2*window_size+1)*(angle<360? 1:8));
            assert(x<window_size+distance ||
                   x>=vh1.scn.xysize[0]-window_size-distance ||
                   y<window_size+distance ||
                   y>=vh1.scn.xysize[1]-window_size-distance ||
                   angle==360 || count==(2*window_size+1)*(2*window_size+1));
            switch (feature)
            {
              case 1: // energy
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                    cur_count += joint_hist[h][i]*joint_hist[h][i];
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(
                  65535*cur_count/(count*count));
                break;
              case 2: // entropy
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                    if (joint_hist[h][i])
                      cur_count +=
                        joint_hist[h][i]*log(1/count*joint_hist[h][i]);
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(
                  -65535*M_E/(nbins*nbins)/count*cur_count);
                break;
              case 3: // maximum probability
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                    if (joint_hist[h][i] > cur_count)
                      cur_count = joint_hist[h][i];
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(
                  65535/count*cur_count);
                break;
              case 4: // contrast
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                    if (i != h)
                      cur_count += joint_hist[h][i]*(i-h)*(i-h);
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(
                  65535./((nbins-1)*(nbins-1))/count*cur_count);
                break;
              case 5: // inverse difference moment
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                    if (i != h)
                      cur_count += joint_hist[h][i]/((i-h)*(i-h));
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)rint(
                  65535/count*cur_count);
                break;
              case 6: // correlation
                mu_x = mu_y = mu2_x = mu2_y = 0;
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                  {
                    if (h)
                      mu_x += joint_hist[h][i]*h;
                    if (i)
                      mu_y += joint_hist[h][i]*i;
                  }
                mu_x *= 1/count;
                mu_y *= 1/count;
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                  {
                    mu2_x += (h-mu_x)*(h-mu_x)*joint_hist[h][i];
                    mu2_y += (i-mu_y)*(i-mu_y)*joint_hist[h][i];
                  }
                if (mu2_x<=0 || mu2_y<=0)
                {
                  data_out_16[y*vh1.scn.xysize[0]+x] = 65535;
                  break;
                }
                mu2_x *= 1/count;
                mu2_y *= 1/count;
                for (h=0; h<nbins; h++)
                  for (i=0; i<nbins; i++)
                    cur_count += h*i*joint_hist[h][i];
                cur_count *= 1/count;
                data_out_16[y*vh1.scn.xysize[0]+x] = (unsigned short)
                  rint(32767.5*(1+(cur_count-mu_x*mu_y)/sqrt(mu2_x*mu2_y)));
                break;
              default:
                fprintf(stderr, "Feature %d not implemented.\n", feature);
                exit(-1);
            }
          }
        }
    }
    error = VWriteData((char*)data_out_8, out_bytes, size, out, &i);
    if (error)
    {
      fprintf(stderr, "Could not write data\n");
      exit(1);
    }
  }
  VCloseData(out);
  if (argc == 11)
  {
    free(data2);
    free(d2_8);
    fclose(in2);
    destroy_scene_header(&vh2);
  }
  free(data1);
  free(d1_8);
  destroy_scene_header(&vh1);
  fclose(in1);
  exit(0);
}


void get_RLM(double **joint_hist, unsigned char *in, int bytes,
	int width, int height, int angle,
    int window_size, int x, int y,
    unsigned short gray_map[0x10000], int *run_map)
{
	int cx, cy, cv, pv, rl, s;

	switch (angle)
	{
		case 0:
			for (cy=y; cy<y+window_size; cy++)
			{
				for (cx=x,rl=1,cv=-1; ;)
				{
					pv = cv;
					if (cx == x+window_size)
						cv = -1;
					else
						switch (bytes)
						{
							case 1:
								cv = gray_map[in[width*cy+cx]];
								break;
							case 2:
								cv =
								 gray_map[((unsigned short *)in)[width*cy+cx]];
								break;
							default:
								fprintf(stderr,
									"%d-byte data not supported.\n", bytes);
								exit(-1);
						}
					cx++;
					if (cv == pv)
						rl++;
					else
					{
						if (pv >= 0)
						{
							joint_hist[run_map[rl]][pv]++;
							rl = 1;
						}
						if (cv < 0)
							break;
					}
				}
			}
			break;
		case 45:
			for (s=0; s<2*window_size-1; s++)
			{
				if (s < window_size)
				{
					cy = y+s;
					cx = x;
				}
				else
				{
					cy = y+window_size-1;
					cx = x-window_size+1+s;
				}
				for (rl=1,cv=-1; ;)
				{
					pv = cv;
					if (cx==x+window_size || cy==-1)
						cv = -1;
					else
						switch (bytes)
						{
							case 1:
								cv = gray_map[in[width*cy+cx]];
								break;
							case 2:
								cv =
								 gray_map[((unsigned short *)in)[width*cy+cx]];
								break;
							default:
								fprintf(stderr,
									"%d-byte data not supported.\n", bytes);
								exit(-1);
						}
					cx++;
					cy--;
					if (cv == pv)
						rl++;
					else
					{
						if (pv >= 0)
						{
							joint_hist[run_map[rl]][pv]++;
							rl = 1;
						}
						if (cv < 0)
							break;
					}
				}
			}
			break;
		case 90:
			for (cx=x; cx<x+window_size; cx++)
			{
				for (cy=y,rl=1,cv=-1; ;)
				{
					pv = cv;
					if (cy == y+window_size)
						cv = -1;
					else
						switch (bytes)
						{
							case 1:
								cv = gray_map[in[width*cy+cx]];
								break;
							case 2:
								cv =
								 gray_map[((unsigned short *)in)[width*cy+cx]];
								break;
							default:
								fprintf(stderr,
									"%d-byte data not supported.\n", bytes);
								exit(-1);
						}
					cy++;
					if (cv == pv)
						rl++;
					else
					{
						if (pv >= 0)
						{
							joint_hist[run_map[rl]][pv]++;
							rl = 1;
						}
						if (cv < 0)
							break;
					}
				}
			}
			break;
		case 135:
			for (s=0; s<2*window_size-1; s++)
			{
				if (s < window_size)
				{
					cy = y;
					cx = x+window_size-1-s;
				}
				else
				{
					cy = y-window_size+1+s;
					cx = x;
				}
				for (rl=1,cv=-1; ;)
				{
					pv = cv;
					if (cx==x+window_size || cy==y+window_size)
						cv = -1;
					else
						switch (bytes)
						{
							case 1:
								cv = gray_map[in[width*cy+cx]];
								break;
							case 2:
								cv =
								 gray_map[((unsigned short *)in)[width*cy+cx]];
								break;
							default:
								fprintf(stderr,
									"%d-byte data not supported.\n", bytes);
								exit(-1);
						}
					cx++;
					cy++;
					if (cv == pv)
						rl++;
					else
					{
						if (pv >= 0)
						{
							joint_hist[run_map[rl]][pv]++;
							rl = 1;
						}
						if (cv < 0)
							break;
					}
				}
			}
			break;
		default:
			fprintf(stderr,
				"Angle must be one of 0, 45, 90, 135.\n");
			exit(-1);
	}

}


void get_co_occurrence(double **joint_hist, unsigned char *in, int bytes,
	int width, int height, int angle, int distance,
	int window_size, int x, int y, unsigned short gray_map[0x10000])
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
	if (start_row < y-window_size)
		start_row = y-window_size;
	if (end_row > y+window_size+1)
		end_row = y+window_size+1;
	if (start_col < x-window_size)
		start_col = x-window_size;
	if (end_col > x+window_size+1)
		end_col = x+window_size+1;
	for (j=start_row; j<end_row; j++)
		for (k=start_col; k<end_col; k++)
		{
			switch (bytes)
			{
				case 1:
					cen_val = in[width*j+k];
					nei_val = in[width*j+k+offset];
					break;
				case 2:
					cen_val = ((unsigned short *)in)[width*j+k];
					nei_val = ((unsigned short *)in)[width*j+k+offset];
					break;
				default:
					fprintf(stderr, "%d-byte data not supported.\n", bytes);
					exit(-1);
			}
			joint_hist[gray_map[cen_val]][gray_map[nei_val]]++;
		}
}

void get_LBP_image(unsigned char *out_8, unsigned char *in, int bytes,
    int width, int height, float radius, int nsamples)
{
	int start_row=0, end_row=height, start_col=0, end_col=width, *offset,
		cen_val, *nei_val, j, k, m, ix, iy, transitions, up;
	float *fx, *fy;
	double tx, ty;
	unsigned short *in2=(unsigned short *)in;

	memset(out_8, nsamples+1, width*height);
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
		if (width-1-ix < end_col)
			end_col = width-1-ix;
		if (height-1-iy < end_row)
			end_row = height-1-iy;
		offset[m] = width*iy+ix;
	}
	for (j=start_row; j<end_row; j++)
		for (k=start_col; k<end_col; k++)
			if (bytes==2? in2[width*j+k]:in[width*j+k])
			{
				cen_val = bytes==2? in2[width*j+k]:in[width*j+k];
				for (m=0; m<nsamples; m++)
				{
					float v;
					if (bytes == 2)
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
				out_8[width*j+k] = transitions<=2? up: nsamples+1;
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


void compute_run_bins(int *bin_end[2], int window_size, int num_run_bins)
{
	int j, run_lengths_left;
	double sqrttwo=sqrt(2.);

	bin_end[0][num_run_bins-1] = bin_end[1][num_run_bins-1] = window_size;
	for (j=num_run_bins-2; j>=0; j--)
	{
		bin_end[0][j] = bin_end[0][j+1];
		bin_end[1][j] = bin_end[1][j+1];
		run_lengths_left = bin_end[0][j]+bin_end[1][j];
		while (bin_end[0][j]+bin_end[1][j] > run_lengths_left/2-1)
		{
			if ((bin_end[1][j]-1)*sqrttwo > bin_end[0][j]-1)
				bin_end[1][j]--;
			else
				bin_end[0][j]--;
		}
	}
}
