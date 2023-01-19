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

#define QUEUE_CHUNK 131072

struct queue {
	char *buffer;
	int limit, entry, exit;
};

#define ClearQueue(Q) \
{ \
	Q.exit = Q.entry = 0; \
	if (Q.buffer == NULL) \
	{	Q.buffer = (char *)malloc(QUEUE_CHUNK); \
		if (Q.buffer == NULL) \
			HandleQueueError; \
		Q.limit = QUEUE_CHUNK; \
	} \
}

#define InitQueue(Q) \
{ \
	Q.buffer = NULL; \
	ClearQueue(Q); \
}

#define QueueIsEmpty(Q) (Q.entry == 0)

#define Dequeue(item, Q, type) \
{ \
	assert(!QueueIsEmpty(Q)); \
	item = *(type *)(Q.buffer+Q.exit); \
	Q.exit += sizeof(type); \
	if (Q.exit == Q.entry) \
		ClearQueue(Q) \
	else if (Q.exit == Q.limit) \
		Q.exit = 0; \
}

#define Enqueue(item, Q, type) \
{ \
	if (Q.entry == (Q.exit? Q.exit: Q.limit)) \
		EnlargeQueue(Q); \
	if (Q.entry == Q.limit) \
		Q.entry = 0; \
	*(type *)(Q.buffer+Q.entry) = item; \
	Q.entry += sizeof(type); \
}

#define EnlargeQueue(Q) \
{ \
	char *t; \
	int j; \
\
	t = (char *)realloc(Q.buffer, Q.limit+QUEUE_CHUNK); \
	if (t == NULL) \
		HandleQueueError; \
	Q.buffer = t; \
	if (Q.exit >= Q.entry) \
	{	for (j=Q.limit-1; j>=Q.exit; j--) \
			Q.buffer[j+QUEUE_CHUNK] = Q.buffer[j]; \
		Q.exit += QUEUE_CHUNK; \
	} \
	Q.limit += QUEUE_CHUNK; \
}

#define Handle_error(message) \
{ \
  fprintf(stderr, "%s\n", message); \
  exit(1); \
}

#define HandleQueueError Handle_error("Out of memory.")

typedef struct {unsigned short low, high;} feature;

void accumulate(int low, int high, int on_surface);
void destroy_scene_header(ViewnixHeader *vh);
int compar(const void *, const void *);

static double hist_buf[0x800800], *hist[0x1000] /* [low][high] */;
static unsigned short gray_map[0x10000];
static double srf_hist_buf[0x800800], *srf_hist[0x1000];

int main(argc,argv)
int argc;
char *argv[];
{
  int j, error, slices1, slices2;
  unsigned k;
  ViewnixHeader vh1, vh2;
  FILE *in1, *in2, *out;
  char group[6], elem[6];
  int ns; // number of samples
  float gray_max=0;
  unsigned short sgray_max, *affinity_buf;
  double tot_count, med_dist, tmp_count, tot_faces, surf_faces;
  struct queue Q;
  feature cur, nei;
  static feature feature_buf[0x800800];

  if (argc < 4) {
    fprintf(stderr, "Usage: face_stats <input BIM_file> ... <feature IM0_file> ... <output raw file>\n");
	fprintf(stderr, " The list of samples of ");
	fprintf(stderr, "the object to be modeled must be followed by the ");
	fprintf(stderr, "corresponding feature scenes.\n");
    exit(-1);
  }
  if (argc%2)
  {
    fprintf(stderr, "Specify feature scene for each subject.\n");
    exit(-1);
  }
  for (j=0; j<4096; j++)
  {
    hist[j] = hist_buf+(8191-j)*j/2;
	srf_hist[j] = srf_hist_buf+(8191-j)*j/2;
  }
  ns = (argc-1)/2;
  for (j=0; j<ns; j++)
  {
	in2 = fopen(argv[ns+j+1], "rb");
	if (in2==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
	error=VReadHeader(in2,&vh2,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
    if (vh2.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	if (vh2.scn.largest_density_value_valid == 0)
	  gray_max = 65535;
	else
	  if (vh2.scn.largest_density_value[0] > gray_max)
	    gray_max = vh2.scn.largest_density_value[0];
    if (vh2.scn.dimension != 3)
	{
	  fprintf(stderr, "Input file should be 3-D\n");
	  exit(-1);
	}
	fclose(in2);
	destroy_scene_header(&vh2);
  }
  for (j=0; j<=gray_max; j++)
    if (gray_max <= 4095)
	  gray_map[j] = j;
	else
	  gray_map[j] = (unsigned short)(j*4095/gray_max);
  for (j=0; j<ns; j++)
  {
	int ii, jj, kk, a, b, slice_size, low, high, ai, bi;
	unsigned short *d2_16;
	unsigned char *d1_1, *d2_8;

    memset(&vh1, 0, sizeof(vh1));
	in1 = fopen(argv[j+1], "rb");
    if (in1==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
    error=VReadHeader(in1,&vh1,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
    if (vh1.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	slices1 = vh1.scn.num_of_subscenes[0];
	slice_size = vh1.scn.xysize[0]*vh1.scn.xysize[1];
	in2 = fopen(argv[ns+j+1], "rb");
	if (in2==NULL) {
      fprintf(stderr, "Error in opening the files\n");
      exit(-1);
    }
	error=VReadHeader(in2,&vh2,group,elem);
    if (error && error<=105) {
      fprintf(stderr, "Fatal error in reading header\n");
      exit(-1);
    }
    if (vh2.gen.data_type!=IMAGE0) {
      fprintf(stderr, "Input file should be IMAGE0\n");
      exit(-1);
    }
	slices2 = vh2.scn.num_of_subscenes[0];
	if (slices2<slices1 ||
		vh2.scn.xysize[0]!=vh1.scn.xysize[0] ||
		vh2.scn.xysize[1]!=vh1.scn.xysize[1])
	{
	  fprintf(stderr, "Different scene sizes not implemented yet.\n");
	  exit(-1);
	}

    d1_1=(unsigned char *)malloc((slice_size+7)/8*2);
    VSeekData(in1,0);
	VSeekData(in2,0);
	if (VReadData((char *)d1_1, 1,
		(slice_size+7)/8, in1, &error))
	{
	  fprintf(stderr, "Could not read data\n");
	  exit(-1);
	}
	if (vh2.scn.num_of_bits == 16)
	{
	  d2_16 = (unsigned short *)malloc(slice_size*4);
	  for (kk=0; kk<slices2; kk++)
	  {
		if (kk == 0)
		{
		  if (VReadData((char *)d2_16, 2, slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		}
		else
		{
		  memcpy(d1_1, d1_1+(slice_size+7)/8, (slice_size+7)/8);
		  memcpy(d2_16, d2_16+slice_size, slice_size*2);
		}
		if (kk < slices2-1)
		{
		  if (kk >= slices1-1)
		    memset(d1_1+(slice_size+7)/8, 0, (slice_size+7)/8);
		  else if (VReadData((char *)d1_1+(slice_size+7)/8, 1,
			  (slice_size+7)/8, in1, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  if (VReadData((char *)(d2_16+slice_size), 2,slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  // z-faces
		  for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		    for (ii=0; ii<vh1.scn.xysize[0]; ii++)
			{
			  ai = vh1.scn.xysize[0]*jj+ii;
			  bi = ai+slice_size;
			  a = d2_16[ai];
			  b = d2_16[bi];
			  if (a > b)
			  	high=a, low=b;
			  else
			    low=a, high=b;
			  if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			      ((d1_1[ai/8+(slice_size+7)/8]&(128>>(ai%8)))>0))
				accumulate(low, high, TRUE);
			  else
			    accumulate(low, high, FALSE);
			}
		}
		// y-faces
		for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]-1; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+vh1.scn.xysize[0];
			a = d2_16[ai];
			b = d2_16[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(low, high, TRUE);
			else
			  accumulate(low, high, FALSE);
		  }
		// x-faces
		for (jj=0; jj<vh1.scn.xysize[1]-1; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+1;
			a = d2_16[ai];
			b = d2_16[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(low, high, TRUE);
			else
			  accumulate(low, high, FALSE);
		  }
	  }
	  free(d2_16);
	}
	else
	{
	  d2_8 = (unsigned char *)malloc(slice_size*2);
	  for (kk=0; kk<slices2; kk++)
	  {
	    if (kk == 0)
		{
		  if (VReadData((char *)d2_8, 1, slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		}
		else
		{
		  memcpy(d1_1, d1_1+(slice_size+7)/8, (slice_size+7)/8);
		  memcpy(d2_8, d2_8+slice_size, slice_size);
		}
		if (kk < slices2-1)
		{
		  if (VReadData((char *)d1_1+(slice_size+7)/8, 1,
			  (slice_size+7)/8, in1, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  if (kk >= slices1-1)
		    memset(d1_1+(slice_size+7)/8, 0, (slice_size+7)/8);
		  else if (VReadData((char *)d2_8+slice_size, 1, slice_size, in2, &error))
		  {
		    fprintf(stderr, "Could not read data\n");
		    exit(-1);
		  }
		  // z-faces
		  for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		    for (ii=0; ii<vh1.scn.xysize[0]; ii++)
			{
			  ai = vh1.scn.xysize[0]*jj+ii;
			  bi = ai+slice_size;
			  a = d2_8[ai];
			  b = d2_8[bi];
			  if (a > b)
			  	high=a, low=b;
			  else
			    low=a, high=b;
			  if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			      ((d1_1[ai/8+(slice_size+7)/8]&(128>>(ai%8)))>0))
				accumulate(low, high, TRUE);
			  else
			    accumulate(low, high, FALSE);
			}
		}
		// y-faces
		for (jj=0; jj<vh1.scn.xysize[1]; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]-1; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+vh1.scn.xysize[0];
			a = d2_8[ai];
			b = d2_8[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(low, high, TRUE);
			else
			  accumulate(low, high, FALSE);
		  }
		// x-faces
		for (jj=0; jj<vh1.scn.xysize[1]-1; jj++)
		  for (ii=0; ii<vh1.scn.xysize[0]; ii++)
		  {
		    ai = vh1.scn.xysize[0]*jj+ii;
			bi = ai+1;
			a = d2_8[ai];
			b = d2_8[bi];
			if (a > b)
			  high=a, low=b;
			else
			  low=a, high=b;
			if (((d1_1[ai/8]&(128>>(ai%8)))>0) !=
			    ((d1_1[bi/8]&(128>>(bi%8)))>0))
			  accumulate(low, high, TRUE);
			else
			  accumulate(low, high, FALSE);
		  }
	  }
	  free(d2_8);
	}
	fclose(in1);
    fclose(in2);
	free(d1_1);
	destroy_scene_header(&vh1);
	destroy_scene_header(&vh2);
  }
  tot_faces = surf_faces = 0;
  for (j=0; j<0x800800; j++)
  {
    tot_faces += hist_buf[j];
	surf_faces += srf_hist_buf[j];
  }
  for (j=0; j<0x800800; j++)
    srf_hist_buf[j] =
	  srf_hist_buf[j]>6*surf_faces/tot_faces*hist_buf[j]? 0: 1.e100;
  // compute distance map
  InitQueue(Q);
  for (cur.low=0; cur.low<4096; cur.low++)
    for (cur.high=cur.low; cur.high<4096; cur.high++)
	  if (srf_hist[cur.low][cur.high] == 0)
	    Enqueue(cur, Q, feature);
  while (!QueueIsEmpty(Q))
  {
    Dequeue(cur, Q, feature);
	if (cur.low>0 && srf_hist[cur.low-1][cur.high]>srf_hist[cur.low][cur.high]+
	    hist[cur.low][cur.high]+hist[cur.low-1][cur.high]+1)
	{
	  nei.low = cur.low-1;
	  nei.high = cur.high;
	  srf_hist[cur.low-1][cur.high] = srf_hist[cur.low][cur.high]+
	    hist[cur.low][cur.high]+hist[cur.low-1][cur.high]+1;
	  Enqueue(nei, Q, feature);
	}
	if (cur.high<4095 && srf_hist[cur.low][cur.high+1]>
	    srf_hist[cur.low][cur.high]+hist[cur.low][cur.high]+
		hist[cur.low][cur.high+1]+1)
    {
	  nei.low = cur.low;
	  nei.high = cur.high+1;
	  srf_hist[nei.low][nei.high] = srf_hist[cur.low][cur.high]+
	    hist[cur.low][cur.high]+hist[cur.low][cur.high+1]+1;
	  Enqueue(nei, Q, feature);
	}
	if (cur.high > cur.low)
	{
	  if (srf_hist[cur.low][cur.high-1] > srf_hist[cur.low][cur.high]+
	      hist[cur.low][cur.high]+hist[cur.low][cur.high-1]+1)
	  {
	    nei.low = cur.low;
		nei.high = cur.high-1;
		srf_hist[nei.low][nei.high] = srf_hist[cur.low][cur.high]+
		  hist[cur.low][cur.high]+hist[cur.low][cur.high-1]+1;
		Enqueue(nei, Q, feature);
	  }
	  if (srf_hist[cur.low+1][cur.high] > srf_hist[cur.low][cur.high]+
	      hist[cur.low][cur.high]+hist[cur.low+1][cur.high]+1)
	  {
	    nei.low = cur.low+1;
		nei.high = cur.high;
		srf_hist[nei.low][nei.high] = srf_hist[cur.low][cur.high]+
		  hist[cur.low][cur.high]+hist[cur.low+1][cur.high]+1;
		Enqueue(nei, Q, feature);
	  }
	}
  }
  // find median distance value
  for (j=cur.low=0; cur.low<4096; cur.low++)
    for (cur.high=cur.low; cur.high<4096; cur.high++,j++)
	  feature_buf[j] = cur;
  qsort(feature_buf, 0x800800, sizeof(feature), compar);
  for (k=0; srf_hist[feature_buf[k].low][feature_buf[k].high]==0; k++)
    ;
  for (tot_count=0,j=k; j<0x800800; j++)
    tot_count += hist[feature_buf[j].low][feature_buf[j].high];
  for (tmp_count=0,j=k; tmp_count<tot_count/6; j++)
    tmp_count += hist[feature_buf[j].low][feature_buf[j].high];
  med_dist = srf_hist[feature_buf[j].low][feature_buf[j].high];
  sgray_max = (unsigned short)gray_max;
  affinity_buf = &feature_buf[0].low;
  out = fopen(argv[argc-1], "wb");
  if (out == NULL)
  {
    fprintf(stderr, "Error in opening the output file\n");
	exit(-1);
  }
  error = VWriteData((char *)&sgray_max, 2, 1, out, &j);
  if (error)
  {
    fprintf(stderr, "Error in writing the output file\n");
	exit(-1);
  }
  for (j=0; j<0x800800; j++)
  {
    tmp_count = srf_hist_buf[j]*(1/med_dist);
    affinity_buf[j] = (unsigned short)(65534*(1-exp(-tmp_count*tmp_count)));
  }
  error = VWriteData((char *)affinity_buf, 2, 0x800800, out, &j);
  if (error)
  {
    fprintf(stderr, "Error in writing the output file\n");
	exit(-1);
  }
  exit(0);
}

int compar(const void *f1, const void *f2)
{
	const feature *fea1=(const feature *)f1, *fea2=(const feature *)f2;
	if (srf_hist[fea1->low][fea1->high] < srf_hist[fea2->low][fea2->high])
		return -1;
	if (srf_hist[fea1->low][fea1->high] > srf_hist[fea2->low][fea2->high])
		return 1;
	return 0;
}

void accumulate(int low, int high, int on_surface)
{
	assert(low <= high);
	assert(low >= 0);
	assert(high < 65536);
	low = gray_map[low];
	high = gray_map[high];
	assert(low <= high);
	assert(high < 4096);
	if (on_surface)
		srf_hist[low][high]++;
	hist[low][high]++;
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
