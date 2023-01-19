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

#include <stdio.h>
#include <math.h>
#include <cv3dv.h>
#include <assert.h>
#if ! defined (WIN32) && ! defined (_WIN32)
  #include <unistd.h>
#endif


#define QUEUE_CHUNK 131072

struct queue {
	char *buffer;
	long limit, entry, exit;
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
	long j; \
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


int main(int argc, char *argv[])
{
  int j, bytes, bytes1, slices, error, max_l, keep_singletons=0;
  long seed, size;
  ViewnixHeader vh1, vh3;
  FILE *in1, *out, *in3;
  unsigned short *data1, *data2;
  unsigned char *data3;
  char group[6],elem[6];
  struct queue Q;
  double threshold, sigma_psi, w_phi, m_phi, sigma_phi;

  if (argc>4 && strcmp(argv[argc-1], "-keep_singletons")==0)
  {
    keep_singletons = 1;
	argc--;
  }

  if (argc>3 && (sscanf(argv[3], "%lf", &threshold)!=1 || threshold<=0 ||
      (threshold<1)!=(argc>=8)))
    Handle_error("Bad threshold.\n");

  if (argc>=8 && sscanf(argv[argc-4], "%lf", &sigma_psi)==1 &&
      sscanf(argv[argc-3], "%lf", &w_phi)==1 &&
	  sscanf(argv[argc-2], "%lf", &m_phi)==1 &&
	  sscanf(argv[argc-1], "%lf", &sigma_phi)==1)
  {
    argc -= 4;
  }
  if (argc!=4 && argc!=5) {
    printf("Usage: gscale <input file> <output file> <threshold> [<maskfile>] [<sigma_psi> <w_phi> <m_phi> <sigma_phi>] [-keep_singletons]\n");
    exit(-1);
  }

  in1=fopen(argv[1],"rb");
  if (in1==NULL )
    Handle_error("Error in opening the input file\n");

  out=fopen(argv[2],"wb+");
  if ( out==NULL )
    Handle_error("Error in opening output file\n");

  error=VReadHeader(in1,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in reading header\n");

  if (argc == 5)
  {
    in3 = fopen(argv[4], "rb");
	if (in3 == NULL)
	  Handle_error("Error in opening the mask file\n");
    error = VReadHeader(in3, &vh3, group,elem);
	if (error>0 && error<=104)
	  Handle_error("Fatal error in reading header\n");
  }

  if (vh1.gen.data_type!=IMAGE0 || (argc==5 && vh3.gen.data_type!=IMAGE0))
    Handle_error("This is not an IMAGE0 file\n");

  if (vh1.scn.dimension!=3 || (argc==5 && vh3.scn.dimension!=3))
    Handle_error("Only 3-D scenes handled.");

  if (argc == 5)
  {
    if (vh3.scn.num_of_bits != 1)
      Handle_error("Mask must be binary.");
    if (vh3.scn.xysize[0]!=vh1.scn.xysize[0] ||
	    vh3.scn.xysize[1]!=vh1.scn.xysize[1] ||
		vh3.scn.num_of_subscenes[0]!=vh1.scn.num_of_subscenes[0])
	  Handle_error("Mask must match inputfile.");
  }

  bytes1 = vh1.scn.num_of_bits/8;
  slices = vh1.scn.num_of_subscenes[0];
  size = (long)vh1.scn.xysize[0]*vh1.scn.xysize[1];

  vh1.scn.smallest_density_value[0] = 0;

  bytes = 2;
  vh1.scn.num_of_bits=bytes*8;
  vh1.scn.bit_fields[0] = 0;
  vh1.scn.bit_fields[1] = vh1.scn.num_of_bits-1;
  strncpy(vh1.gen.filename, argv[2], sizeof(vh1.gen.filename)-1);
  vh1.gen.filename[sizeof(vh1.gen.filename)-1] = 0;

  data2= (unsigned short *)malloc(size*slices*2);
  if (data2==NULL)
    Handle_error("Could not allocate output data. Aborting fuzz_ops\n");
  if (argc == 5)
  {
    data3 = (unsigned char *)malloc((size+7)/8);
	if (data3==NULL)
	  Handle_error("Could not allocate output data. Aborting fuzz_ops\n");
	VSeekData(in3, 0);
	for (j=0; j<slices; j++)
	{
	  error = VReadData((char *)data3, 1, (size+7)/8, in3, (int *)&seed);
	  if (error)
	    Handle_error("Could not read data");
	  for(seed=0; seed<size; seed++)
	    data2[size*j+seed] = data3[seed/8]&(128>>(seed%8))? 65535: 0;
	}
	free(data3);
  }
  else
    memset(data2, 255, size*slices*2);

  data1= (unsigned short *)malloc(size*slices*2);
  if (data1==NULL)
    Handle_error("Could not allocate data. Aborting fuzz_ops\n");

  VSeekData(in1,0);
  error = VReadData((char *)data1, bytes1, size*slices, in1, &j);
  if (error)
    Handle_error("Could not read data");
  if (bytes1 == 1)
    for (seed=size*slices-1; seed>=0; seed--)
	  data1[seed] = ((unsigned char *)data1)[seed];
  InitQueue(Q);
  max_l = 0;
  for(seed=0; seed<size*slices; seed++)
	if (data2[seed] == 65535)
	{
	  long region_size=0;
	  max_l++;
	  if (max_l >= 65535)
	  {
	    fclose(out);
		unlink(argv[2]);
		Handle_error("Too many regions.");
	  }
	  data2[seed] = max_l;
	  assert(QueueIsEmpty(Q));
	  Enqueue(seed, Q, long);
	  while (!QueueIsEmpty(Q))
	  {
	    long c, cx, cy, cz, cv, nv, dv, minv;

		region_size++;
		Dequeue(c, Q, long);
        cz = c/size;
	    cy = (c-size*cz)/vh1.scn.xysize[0];
	    cx = c-size*cz-vh1.scn.xysize[0]*cy;
		cv = data1[c];
		if (cx>0 && data2[c-1]==65535)
		{
		  nv = data1[c-1];
		  if ((dv=nv>cv? nv-cv: cv-nv), threshold>=1? dv<(long)threshold:
		      ((minv = nv<cv? nv:cv),
		      (1-w_phi)*exp(-(double)dv*dv/(2*sigma_psi*sigma_psi))+
			  (minv>=m_phi? w_phi:
			  w_phi*exp(-(double)minv*minv/(2*sigma_phi*sigma_phi)))
			  >=threshold))
		  {
		    data2[c-1] = max_l;
			Enqueue(c-1, Q, long);
		  }
		}
		if (cx<vh1.scn.xysize[0]-1 && data2[c+1]==65535)
		{
		  nv = data1[c+1];
		  if ((dv=nv>cv? nv-cv: cv-nv), threshold>=1? dv<(long)threshold:
		      ((minv = nv<cv? nv:cv),
		      (1-w_phi)*exp(-(double)dv*dv/(2*sigma_psi*sigma_psi))+
			  (minv>=m_phi? w_phi:
			  w_phi*exp(-(double)minv*minv/(2*sigma_phi*sigma_phi)))
			  >=threshold))
		  {
		    data2[c+1] = max_l;
			Enqueue(c+1, Q, long);
		  }
		}
		if (cy>0 && data2[c-vh1.scn.xysize[0]]==65535)
		{
		  nv = data1[c-vh1.scn.xysize[0]];
		  if ((dv=nv>cv? nv-cv: cv-nv), threshold>=1? dv<(long)threshold:
		      ((minv = nv<cv? nv:cv),
		      (1-w_phi)*exp(-(double)dv*dv/(2*sigma_psi*sigma_psi))+
			  (minv>=m_phi? w_phi:
			  w_phi*exp(-(double)minv*minv/(2*sigma_phi*sigma_phi)))
			  >=threshold))
		  {
		    data2[c-vh1.scn.xysize[0]] = max_l;
			Enqueue(c-vh1.scn.xysize[0], Q, long);
		  }
		}
		if (cy<vh1.scn.xysize[1]-1 && data2[c+vh1.scn.xysize[0]]==65535)
		{
		  nv = data1[c+vh1.scn.xysize[0]];
		  if ((dv=nv>cv? nv-cv: cv-nv), threshold>=1? dv<(long)threshold:
		      ((minv = nv<cv? nv:cv),
		      (1-w_phi)*exp(-(double)dv*dv/(2*sigma_psi*sigma_psi))+
			  (minv>=m_phi? w_phi:
			  w_phi*exp(-(double)minv*minv/(2*sigma_phi*sigma_phi)))
			  >=threshold))
		  {
		    data2[c+vh1.scn.xysize[0]] = max_l;
			Enqueue(c+vh1.scn.xysize[0], Q, long);
		  }
		}
		if (cz>0 && data2[c-size]==65535)
		{
		  nv = data1[c-size];
		  if ((dv=nv>cv? nv-cv: cv-nv), threshold>=1? dv<(long)threshold:
		      ((minv = nv<cv? nv:cv),
		      (1-w_phi)*exp(-(double)dv*dv/(2*sigma_psi*sigma_psi))+
			  (minv>=m_phi? w_phi:
			  w_phi*exp(-(double)minv*minv/(2*sigma_phi*sigma_phi)))
			  >=threshold))
		  {
		    data2[c-size] = max_l;
			Enqueue(c-size, Q, long);
		  }
		}
		if (cz<slices-1 && data2[c+size]==65535)
		{
		  nv = data1[c+size];
		  if ((dv=nv>cv? nv-cv: cv-nv), threshold>=1? dv<(long)threshold:
		      ((minv = nv<cv? nv:cv),
		      (1-w_phi)*exp(-(double)dv*dv/(2*sigma_psi*sigma_psi))+
			  (minv>=m_phi? w_phi:
			  w_phi*exp(-(double)minv*minv/(2*sigma_phi*sigma_phi)))
			  >=threshold))
		  {
		    data2[c+size] = max_l;
			Enqueue(c+size, Q, long);
		  }
		}
	  }
	  if (region_size==1 && !keep_singletons)
	  {
	    max_l--;
		data2[seed] = 0;
	  }
    }

  vh1.scn.largest_density_value[0] = (float)max_l;
  error=VWriteHeader(out,&vh1,group,elem);
  if (error>0 && error<=104)
    Handle_error("Fatal error in writing header\n");
  if (VWriteData((char *)data2,bytes,size*slices,out,&j))
    Handle_error("Could not write data\n");

  fclose(in1);
  VCloseData(out);
  exit(0);
}

