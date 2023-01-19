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

#include <math.h>
#include <stdlib.h>
#include <Viewnix.h>
#include <cv3dv.h>
#include  "port_data/from_dicom.h"


#define QueueItem X_Point
#define HandleQueueError {fprintf(stderr,"Out of memory.\n");exit(1);}
#include "queue.c"

int get_file_info(char *file /* filename */, ViewnixHeader *vh,
    int *len /* length of header */);

int main(int argc, char **argv)
{
	FILE *fpin, *fpout;	/* inpput/output files */
	ViewnixHeader vh;	/* 3DViewnix header */
        int slices;		/* total number of slices in the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;		/* length of a slice */
	int outlength;		/* length of a output slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int depth;		/* bits per pixel */
	int j, k;		/* general use */
	int error;		/* error code */
	char *comments;	/* used to modify the header (description field) */
	int tmp;
	unsigned char *out_buffer1, *out_buffer2, *out_buffer3;
	int *slice_points;
	pairp *contour_data;
	int *ncontours, **start_point;
	X_Point cur, nex;
	contour_struct_set ss;
	int obj_number;
	char *out_filename=NULL;
	float halfsl=.5;


	if (argc != 4)
	{
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "fillcontours <in_IM0> <in_RT_dcm> <out_prefix>\n");
		exit(1);
	}

	fpin = fopen(argv[2], "rb");
	if (fpin == NULL)
	{
		fprintf(stderr, "ERROR: Can't open INPUT file !\n");
		exit(1);
	}
	error = get_contour_struct_set(fpin, &ss);
	if (error)
	{
		fprintf(stderr, "error = %d\n", error);
		exit(error);
	}

    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
	memset(&vh, 0, sizeof(vh));
    get_file_info(argv[1], &vh, &hlength);

	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	depth= vh.scn.num_of_bits;
	length = width*height;
	outlength=(width*height +7)/8;
    if (vh.scn.dimension==3) {
		slices=vh.scn.num_of_subscenes[0];
		if (slices > 1)
			halfsl = (vh.scn.loc_of_subscenes[1]-vh.scn.loc_of_subscenes[0])/2;
	}
	else if (vh.scn.dimension==4)  {
		slices=0;
		for(j=0;j<vh.scn.num_of_subscenes[0];j++)
			slices += vh.scn.num_of_subscenes[j+1];
	}	
	else {
	   fprintf(stderr, "ERROR: Cannot handle %d dimensional data\n",vh.scn.dimension);
	   exit(-1);
	}

   	/* create buffer for one binary image */
	out_buffer1 = (unsigned char *)malloc(length);
	out_buffer2 = (unsigned char *)malloc(length);
	out_buffer3 = (unsigned char *)malloc(length);
   	if (out_buffer1==NULL || out_buffer2==NULL || out_buffer3==NULL)
   	{
   		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
   		exit(1);
   	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
	vh.scn.num_of_bits = 1;
	vh.scn.bit_fields[0] = 0;
	vh.scn.bit_fields[1] = 0;
	vh.scn.smallest_density_value[0] = 0;
	vh.scn.largest_density_value[0] = 1;
	/* Get the filenames right (own and parent) */
    strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1)-1);
	vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    strncpy(vh.gen.filename, argv[3], sizeof(vh.gen.filename)-1);
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

	slice_points = (int *)calloc(slices, sizeof(int));
	contour_data = (pairp *)calloc(slices, sizeof(int (*)[2]));
	ncontours = (int *)calloc(slices, sizeof(int));
	start_point = (int **)calloc(slices, sizeof(int *));

	for (obj_number=0; obj_number<ss.num_rois; obj_number++)
	{

		/* Open INPUT or OUTPUT Files */
		if (ss.roi[obj_number].num_contours == 0)
			continue;
		if (out_filename)
			free(out_filename);
		out_filename = (char *)malloc(strlen(ss.roi[obj_number].roi_name)+
			strlen(argv[3])+5);
		sprintf(out_filename, "%s%s.BIM", argv[3],
			ss.roi[obj_number].roi_name);
		if( (fpout = fopen(out_filename, "wb")) == NULL)
		{
			fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
			exit(1);
		}
		/* Write output 3DViewnix Header */
		error = VWriteHeader(fpout,&vh,group,element);
		if (error && error < 100)
		{
			fprintf(stderr,
				"ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n",
				error,group, element);
			exit(error);
		}

		memset(ncontours, 0, slices*sizeof(int));
		for (j=0; j<slices; j++)
		{
			slice_points[j] = 0;
			for (k=0; k<ss.roi[obj_number].num_contours; k++)
			{
				if (ss.roi[obj_number].contour_num_points[k] &&
						ss.roi[obj_number].contour_coord[k][0][2]-
						vh.scn.domain[2]>
						vh.scn.loc_of_subscenes[j]-halfsl &&
						ss.roi[obj_number].contour_coord[k][0][2]-
						vh.scn.domain[2]<vh.scn.loc_of_subscenes[j]+halfsl)
				{
					slice_points[j] +=
						ss.roi[obj_number].contour_num_points[k];
					ncontours[j]++;
				}
			}
			if (slice_points[j] == 0)
				continue;
			if (contour_data[j])
				free(contour_data[j]);
			contour_data[j] =
				(int (*)[2])malloc(slice_points[j]*sizeof(int [2]));
			if (contour_data[j] == NULL)
			{
				fprintf(stderr, "Out of memory.\n");
				exit(1);
			}
		}

		for (j=0; j<slices; j++)
		{
			if (start_point[j])
				free(start_point[j]);
			start_point[j] = (int *)malloc((ncontours[j]+1)*sizeof(int));
			start_point[j][0] = 0;
			ncontours[j] = 0;
			int sp=0;
			for (k=0; k<ss.roi[obj_number].num_contours; k++)
			{
				if (ss.roi[obj_number].contour_num_points[k] &&
						ss.roi[obj_number].contour_coord[k][0][2]-
						vh.scn.domain[2]>
						vh.scn.loc_of_subscenes[j]-halfsl &&
						ss.roi[obj_number].contour_coord[k][0][2]-
						vh.scn.domain[2]<vh.scn.loc_of_subscenes[j]+halfsl)
				{
					for (unsigned int cp=0;
							cp<ss.roi[obj_number].contour_num_points[k];
							cp++,sp++)
					{
						contour_data[j][sp][0] = (int)rint(
							(ss.roi[obj_number].contour_coord[k][cp][0]-
							vh.scn.domain[0])*(1/vh.scn.xypixsz[0]));
						contour_data[j][sp][1] = (int)rint(
							(ss.roi[obj_number].contour_coord[k][cp][1]-
							vh.scn.domain[1])*(1/vh.scn.xypixsz[1]));
					}
					start_point[j][ncontours[j]+1] =
						start_point[j][ncontours[j]]+
						ss.roi[obj_number].contour_num_points[k];
					ncontours[j]++;
				}
			}
			memset(out_buffer1, 0, length);
			memset(out_buffer2, 0, length);
			if (slice_points[j] > 0)
			{
				for (k=0; k<ncontours[j]; k++)
				{
					int x1, y1, x2, y2, npoints, m, n;
					X_Point* points;
					for (m=start_point[j][k]; m<start_point[j][k+1]; m++)
					{
						x1 = contour_data[j][m][0];
						y1 = contour_data[j][m][1];
						if (m+1 == start_point[j][k+1])
						{
							x2 = contour_data[j][start_point[j][k]][0];
							y2 = contour_data[j][start_point[j][k]][1];
						}
						else
						{
							x2 = contour_data[j][m+1][0];
							y2 = contour_data[j][m+1][1];
						}
						if (x1==x2 && y1==y2)
							continue;
						VComputeLine(x1, y1, x2, y2, &points, &npoints);
						for (n=0; n<npoints-1; n++)
							out_buffer1[points[n].x+points[n].y*width] = 1;
						free(points);
					}
				}

				// fill exterior to out_buffer2
				ClearQueue;
				if (out_buffer1[0] == 0)
				{
					cur.x = 0;
					cur.y = 0;
					out_buffer2[0] = 128;
					Enqueue(cur);
				}
				if (out_buffer1[width-1] == 0)
				{
					cur.x = width-1;
					cur.y = 0;
					out_buffer2[width-1] = 128;
					Enqueue(cur);
				}
				if (out_buffer1[width*(height-1)] == 0)
				{
					cur.x = 0;
					cur.y = height-1;
					out_buffer2[width*(height-1)] = 128;
					Enqueue(cur);
				}
				if (out_buffer1[width*height-1] == 0)
				{
					cur.x = width-1;
					cur.y = height-1;
					out_buffer2[width*height-1] = 128;
					Enqueue(cur);
				}
				while (!QueueIsEmpty)
				{
					Dequeue(cur);
					nex.x = cur.x-1;
					nex.y = cur.y;
					if (cur.x>0 && (out_buffer2[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer2[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
					nex.x = cur.x+1;
					if (cur.x<width-1 && (out_buffer2[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer2[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
					nex.x = cur.x;
					nex.y = cur.y-1;
					if (cur.y>0 && (out_buffer2[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer2[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
					nex.y = cur.y+1;
					if (cur.y<height-1 && (out_buffer2[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer2[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
				}

				// dilate to out_buffer3
				memcpy(out_buffer3, out_buffer2, length);
				for (cur.y=0; cur.y<height; cur.y++)
					for (cur.x=0; cur.x<width; cur.x++)
					{
						if (cur.x < width-1)
						{
							if (out_buffer2[cur.x+cur.y*width])
								out_buffer3[cur.x+1+cur.y*width] = 1;
							if (out_buffer2[cur.x+1+cur.y*width])
								out_buffer3[cur.x+cur.y*width] = 1;
						}
						if (cur.y < height-1)
						{
							if (out_buffer2[cur.x+cur.y*width])
								out_buffer3[cur.x+(cur.y+1)*width] = 1;
							if (out_buffer2[cur.x+(cur.y+1)*width])
								out_buffer3[cur.x+cur.y*width] = 1;
						}
					}

				// subtract from out_buffer1
				for (k=0; k<length; k++)
					if (out_buffer3[k])
						out_buffer1[k] = 0;

				// fill exterior to out_buffer3
				memset(out_buffer3, 0, length);
				ClearQueue;
				cur.x = 0;
				cur.y = 0;
				out_buffer3[0] = 128;
				Enqueue(cur);
				while (!QueueIsEmpty)
				{
					Dequeue(cur);
					nex.x = cur.x-1;
					nex.y = cur.y;
					if (cur.x>0 && (out_buffer3[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer3[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
					nex.x = cur.x+1;
					if (cur.x<width-1 && (out_buffer3[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer3[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
					nex.x = cur.x;
					nex.y = cur.y-1;
					if (cur.y>0 && (out_buffer3[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer3[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
					nex.y = cur.y+1;
					if (cur.y<height-1 && (out_buffer3[nex.x+nex.y*width])==0 &&
							(out_buffer1[nex.x+nex.y*width])==0)
					{
						out_buffer3[nex.x+nex.y*width] = 1;
						Enqueue(nex);
					}
				}

				// add from out_buffer1
				for (k=0; k<length; k++)
					if (out_buffer1[k])
						out_buffer3[k] = 1;

				// subtract out_buffer2
				for (k=0; k<length; k++)
					if (out_buffer2[k])
						out_buffer3[k] = 0;

				// pack
				for (k=0; k<length; k++)
					if (out_buffer3[k])
						out_buffer1[k/8] |= 128>>(k%8);
			}

			/* Save output slice */
			if(VWriteData((char *)out_buffer1, 1, outlength, fpout,&tmp))
			{
				fprintf(stderr, "ERROR: Couldn't write slice #%d.\n", j+1);
				exit(3);
			}
		}
		VCloseData(fpout);
	}

	exit(0);
}

 
/************************************************************************
 *                                                                      *
 *      Function        : get_file_info                                 *
 *      Description     : This function will read the file header and 	*
 *			  its length.					*
 *      Return Value    :  0 - works successfully.                      *
 *      Parameters      :  file - the filename.				*
 *			   vh - the viewnix header.			*
 *			   len - the length of the header.		*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on January 15, 1992 by                *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
int get_file_info(char *file /* filename */, ViewnixHeader *vh,
    int *len /* length of header */)
{
    char group[5], element[5];
    FILE *fp;
    int error;
 
    /* open file */
    if ((fp=fopen(file,"rb"))==NULL)
    {
        fprintf(stderr,"ERROR: Can't open file [%s] !\n", file);
        exit(0);
    }
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        exit(0);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        exit(0);
    }
 
    fclose(fp);
 
    return(0);
 
}
