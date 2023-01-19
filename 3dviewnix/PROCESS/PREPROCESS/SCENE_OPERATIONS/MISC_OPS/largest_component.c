/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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
#include <stdlib.h>
#include <cv3dv.h>

typedef struct { int x, y, z; } Point;

#define QueueItem Point
#define HandleQueueError {fprintf(stderr,"Out of memory.\n");exit(1);}
#include "port_data/queue.c"

int get_slices(int dim, short *list);

int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin, *fpout;	/* inpput/output files */
	ViewnixHeader vh;	/* 3DViewnix header */
        int slices;		/* total number of slices in the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int width, height;	/* dimensions of a slice */
	int j, k;		/* general use */
	int error;		/* error code */
	char *comments;	/* used to modify the header (description field) */
	unsigned char *in_buffer, *cur_buffer, *max_buffer, *cum_buffer;
	Point cur, nex, scanning;
	unsigned long cur_size, max_size=0, cum_size=0, tot_size=0;


	if(argc != 3)
	{
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "largest_component in.BIM out.BIM\n");
		exit(1);
	}

    /* Open INPUT and OUTPUT Files */
	if( (fpin = fopen(argv[1], "rb")) == NULL)
	{
		fprintf(stderr, "ERROR: Can't open INPUT file !\n");
		exit(1);
	}
	error = VReadHeader(fpin, &vh, group,element);
	if (error && error<=105)
	{
		fprintf(stderr, "Fatal error in reading header\n");
		exit(error);
	}
	if (vh.scn.num_of_bits != 1)
	{
		fprintf(stderr, "Not a binary scene.\n");
		exit(1);
	}
	slices = get_slices(vh.scn.dimension, vh.scn.num_of_subscenes);
	width = vh.scn.xysize[0];
	height = vh.scn.xysize[1];
	in_buffer = (unsigned char *)malloc((width*height+7)/8*slices);
	cur_buffer = (unsigned char *)malloc((width*height+7)/8*slices);
	cum_buffer = (unsigned char *)malloc((width*height+7)/8*slices);
	max_buffer = (unsigned char *)malloc((width*height+7)/8*slices);
	if (max_buffer == NULL)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	VSeekData(fpin, 0);
	if (VReadData((char *)in_buffer, 1, (width*height+7)/8*slices, fpin, &j))
	{
		fprintf(stderr, "Could not read data\n");
		exit(-1);
	}
	fclose(fpin);

    if( (fpout = fopen(argv[2], "wb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }

#define Is_Set(buf, pt) (((buf)[(width*height+7)/8*(pt).z+ \
	(width*(pt).y+(pt).x)/8] & (128>>((width*(pt).y+(pt).x)%8))) != 0)

#define Set_Bit(buf, pt) ((buf)[(width*height+7)/8*(pt).z+ \
	(width*(pt).y+(pt).x)/8] |= (128>>((width*(pt).y+(pt).x)%8)))

#define Step_Fwd {if (++scanning.x == width) \
                  {   scanning.x = 0; \
				      if (++scanning.y == height) \
					  { \
					      scanning.y = 0; \
						  scanning.z++; \
					  } \
				  } \
				 }

#define Enqueue_nex { Set_Bit(cur_buffer, nex); Enqueue(nex); cur_size++; }

	scanning.x = scanning.y = scanning.z = 0;
	while (scanning.z < slices)
	{
		if (Is_Set(in_buffer, scanning))
			tot_size++;
		Step_Fwd
	}
	memset(cum_buffer, 0, (width*height+7)/8*slices);
	scanning.x = scanning.y = scanning.z = 0;
	while (max_size < tot_size-cum_size)
	{
		memset(cur_buffer, 0, (width*height+7)/8*slices);
		cur_size = 0;
		while (!Is_Set(in_buffer, scanning) || Is_Set(cum_buffer, scanning))
			Step_Fwd
		ClearQueue;
		cur = scanning;
		Set_Bit(cur_buffer, cur);
		Enqueue(cur);
		while (!QueueIsEmpty)
		{
			Dequeue(cur);
			nex.x = cur.x-1;
			nex.y = cur.y;
			nex.z = cur.z;
			if (cur.x>0 && !Is_Set(cur_buffer, nex) &&
					Is_Set(in_buffer, nex))
				Enqueue_nex
			nex.x = cur.x+1;
			if (cur.x<width-1 && !Is_Set(cur_buffer, nex) &&
					Is_Set(in_buffer, nex))
				Enqueue_nex
			nex.x = cur.x;
			nex.y = cur.y-1;
			if (cur.y>0 && !Is_Set(cur_buffer, nex) &&
					Is_Set(in_buffer, nex))
				Enqueue_nex
			nex.y = cur.y+1;
			if (cur.y<height-1 && !Is_Set(cur_buffer, nex) &&
					Is_Set(in_buffer, nex))
				Enqueue_nex
			nex.y = cur.y;
			nex.z = cur.z-1;
			if (cur.z>0 && !Is_Set(cur_buffer, nex) &&
					Is_Set(in_buffer, nex))
				Enqueue_nex
			nex.z = cur.z+1;
			if (cur.z<slices-1 && !Is_Set(cur_buffer, nex) &&
					Is_Set(in_buffer, nex))
				Enqueue_nex
		}
		if (cur_size > max_size)
		{
			max_size = cur_size;
			memcpy(max_buffer, cur_buffer, (width*height+7)/8*slices);
		}
		for (j=0; j<(width*height+7)/8*slices; j++)
			cum_buffer[j] |= cur_buffer[j];
		cum_size += cur_size;
	}

	/* Get the filenames right (own and parent) */
    strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1)-1);
	vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename)-1);
	vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
    /* Build "description" header entry */
	j = argc*2;
	for(k=0; k<argc; k++)
		j += (int)strlen(argv[k]);
	comments = (char *)malloc(j);
	comments[0] = 0;
    for(k=0; k<argc; k++)
    {
        strcat(comments,argv[k]);
        strcat(comments," ");
    }
    vh.scn.description = comments;
    vh.scn.description_valid = 0x1;

	/* Write output 3DViewnix Header */
	error = VWriteHeader(fpout,&vh,group,element);
	if (error && error < 100)
	{
		fprintf(stderr,"ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n"
			, error,group, element);
		exit(error);
	}

	/* Save output data */
   	if (VWriteData((char *)max_buffer, 1, (width*height+7)/8*slices, fpout,&j))
   	{
   		fprintf(stderr, "ERROR: Couldn't write data.\n");
   		exit(3);
   	}
	exit(0);
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
