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

/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/


/*
	This program performs filtering on an nD Scene (3DViewnix format).

	The type of filter applied is: connected components



	Each volume is is loaded and filtered individually.



Author: Roberto J. Goncalves
Date  : 11/02/92

Adapted 6/13/01 from gaussian3d.c by Dewey Odhner.

*/


#include <math.h>
#include <stdlib.h>
#include <cv3dv.h>


#include "slices.c"

typedef struct { int x, y, z; } Point;

#define QueueItem Point
#define HandleQueueError {fprintf(stderr,"Out of memory.\n");exit(1);}
#include "port_data/queue.c"



/*-------------------------------------------------------------------------*/
/* Read the file header and its length */
int get_file_info(file, vh, len)
ViewnixHeader *vh;
char *file; /* filename */
int *len;   /* length of header */
{
    char group[5], element[5];
    FILE *fp;
    int error;
 
    /* open file */
    if ((fp=fopen(file,"rb"))==NULL)
    {
        fprintf(stderr,"ERROR: Can't open file [%s] !\n", file);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }
 
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        VDeleteBackgroundProcessInformation();
        exit(0);
    }
 
    fclose(fp);
 
    return(0);
 
}
 



/* Modified: 3/26/02 first seed of component printed by Dewey Odhner. */
/* Modified: 4/10/02 first seed printed in measurement units by Dewey Odhner. */
int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin, *fpout;	/* inpput/output files */
	int execution_mode;	/* execution mode */
	ViewnixHeader vh;	/* 3DViewnix header */
	SLICES	sl;			/* Structure containing information about the slices of the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;			/* length of a slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int i,j,k;			/* general use */
	int error;			/* error code */
	char *comments;     /* used to modify the header (description field) */
	int ll, m, n, p, q;
	int largest_density_value=0;
	int neighbor[14], connectivity=6;
	unsigned short eq_tab[65535];
	int max_label=0, nseeds;
	unsigned char *in_buffer1;
	unsigned short *out_buffer, *outptr[14];
	unsigned char flags[65535];
	Point cur, nex, scanning;


	if (argc<4 || (argc>=5 && (argc%3!=2 || (atoi(argv[4])>26))))
	{
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%s input output mode [connectivity [x y z] ... ]\n", argv[0]);
		fprintf(stderr, "where:\n");
		fprintf(stderr, "input    : name of input file;\n");
		fprintf(stderr, "output   : name of output file;\n");
		fprintf(stderr, "mode     : mode of operation (0=foreground, 1=background);\n");
		fprintf(stderr, "connectivity: [4 | 6 | 18 | 26];\n");
		fprintf(stderr, "x y z    : seed coordinates\n");
		exit(1);
	}
	

	nseeds = argc/3-1;
	 
 
    /* Open INPUT and OUTPUT Files */
    if( (fpin = fopen(argv[1], "rb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open INPUT file !\n");
        exit(1);
    }
    if( (fpout = fopen(argv[2], "wb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }
 
    /* Get EXECUTION MODE */
    sscanf(argv[3], "%d", &execution_mode);


	if (argc == 5)
		connectivity = atoi(argv[4]);

	/* If in background mode, then place an entry in the BG_PROCESS.COM file */
    if(execution_mode == 1)
        VAddBackgroundProcessInformation("bin_smooth");


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);

	if (sl.Min_spacing3 <= .99*sl.Max_spacing3)
    {
        fprintf(stderr, "ERROR: Slice spacing not uniform.\n");
        exit(1);
    }

	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	length = (width * height + 7) / 8;

	neighbor[ 0] = (1*(height+2)+1)*(width+2)+1;

	neighbor[ 1] = (1*(height+2)+0)*(width+2)+1;
	neighbor[ 2] = (1*(height+2)+1)*(width+2)+0;

	neighbor[ 3] = (0*(height+2)+1)*(width+2)+1;

	neighbor[ 4] = (0*(height+2)+0)*(width+2)+1;
	neighbor[ 5] = (0*(height+2)+1)*(width+2)+0;
	neighbor[ 6] = (0*(height+2)+1)*(width+2)+2;
	neighbor[ 7] = (0*(height+2)+2)*(width+2)+1;
	neighbor[ 8] = (1*(height+2)+0)*(width+2)+0;
	neighbor[ 9] = (1*(height+2)+0)*(width+2)+2;

	neighbor[10] = (0*(height+2)+0)*(width+2)+0;
	neighbor[11] = (0*(height+2)+0)*(width+2)+2;
	neighbor[12] = (0*(height+2)+2)*(width+2)+0;
	neighbor[13] = (0*(height+2)+2)*(width+2)+2;


	/* Allocate memory */
	if(vh.scn.num_of_bits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1 = (unsigned char *) calloc(length, 1) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}

    	/* create buffer for one grey image */
    	if( (out_buffer = (unsigned short *) malloc(2*(width+2)*(height+2)*(sl.max_slices+2)) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
 
	}
	else
	{
    	fprintf(stderr, "bin_components: Can't process a non-binary image.\n");
       	exit(1);
	}

	if (nseeds == 0)
	{
		vh.scn.num_of_bits = 16;
		vh.scn.bit_fields[0] = 0;
		vh.scn.bit_fields[1] = 15;
	}

	/* Get the filenames right (own and parent) */
    strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1)-1);
	vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename)-1);
	vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
    /* Build "description" header entry */
	comments = (char *)calloc(1,1);
    for(i=0; i<argc; i++)
    {
        comments=(char *)realloc(comments, strlen(comments)+strlen(argv[i])+2);
		strcat(comments,argv[i]);
        strcat(comments," ");
    }
    vh.scn.description = comments;
    vh.scn.description_valid = 0x1;
	vh.scn.smallest_density_value[0] = 0;

	memset(eq_tab, 0, sizeof(eq_tab));
	largest_density_value = 1;


	/************************/	
	/* Traverse ALL VOLUMES */
	k=0;	/* k=index of the current slice (within all slices) */
	for(j=0; j<sl.volumes; k+=sl.slices[j++])
	{
		/* Seek the appropriate location */
		fseek(fpin, (k*length)+hlength, 0);

		/* Initialize output volume */
	    memset(out_buffer, 0, 2*(width+2)*(height+2)*(sl.slices[j]+2));
		for(i=0; i<sl.slices[j]; i++)
		{
	        /* read first slice */
	        if(fread(in_buffer1, 1, length, fpin) != length)
   	        {
   		        fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", 1, j+1);
   		        exit(2);
   	        }
			m = 0;
			for (ll=0; ll<height; ll++)
				for (n=0; n<width; n++, m++)
					if (in_buffer1[m/8] & (128>>(m%8)))
						(out_buffer+1+(width+2)*(1+ll+(height+2)*(1+i)))[n] =
							65535;
		}


#define Is_Set(pt) (out_buffer[(width+2)*(height+2)*((pt).z+1)+ \
	(width+2)*((pt).y+1)+(pt).x+1] == 1)

#define Was_Set(pt) (out_buffer[(width+2)*(height+2)*((pt).z+1)+ \
	(width+2)*((pt).y+1)+(pt).x+1] > 0)

#define Set_Word(pt) (out_buffer[(width+2)*(height+2)*((pt).z+1)+ \
	(width+2)*((pt).y+1)+(pt).x+1] = 1)

#define Enqueue_nex { Set_Word(nex); Enqueue(nex); }


		if (nseeds && connectivity==6)
		{
			eq_tab[1] = max_label = 1;
			for (p=0; p<nseeds; p++)
			{
				scanning.x = atoi(argv[3*p+5]);
				scanning.y = atoi(argv[3*p+6]);
				scanning.z = atoi(argv[3*p+7]);
				if (scanning.x<0 || scanning.x>=width ||
						scanning.y<0 || scanning.y>=height ||
						scanning.z<0 || scanning.z>=sl.slices[j])
				{
					fprintf(stderr, "Seed out of scene.\n");
					exit(1);
				}
				ClearQueue;
				cur = scanning;
				Set_Word(cur);
				Enqueue(cur);
				while (!QueueIsEmpty)
				{
					Dequeue(cur);
					nex.x = cur.x-1;
					nex.y = cur.y;
					nex.z = cur.z;
					if (!Is_Set(nex) &&
							Was_Set(nex))
						Enqueue_nex
					nex.x = cur.x+1;
					if (!Is_Set(nex) &&
							Was_Set(nex))
						Enqueue_nex
					nex.x = cur.x;
					nex.y = cur.y-1;
					if (!Is_Set(nex) &&
							Was_Set(nex))
						Enqueue_nex
					nex.y = cur.y+1;
					if (!Is_Set(nex) &&
							Was_Set(nex))
						Enqueue_nex
					nex.y = cur.y;
					nex.z = cur.z-1;
					if (!Is_Set(nex) &&
							Was_Set(nex))
						Enqueue_nex
					nex.z = cur.z+1;
					if (!Is_Set(nex) &&
							Was_Set(nex))
						Enqueue_nex
				}
			}
		}
		else
	      /* Forward pass */
	      for (i=0; i<sl.slices[j]; i++)
	      {
			if(execution_mode == 0)
			{
				if(sl.volumes > 1)
					printf("Filtering volume #%d/%d, slice #%d/%d ...\n", j+1,sl.volumes,i+1, sl.slices[j]);
				else
					printf("Filtering slice #%d/%d ...\n", i+1, sl.slices[j]);

				fflush(stdout);
			}
			for (ll=0; ll<height; ll++)
			{
				for (m=0; m<=connectivity/2; m++)
					outptr[m] =
						out_buffer+(ll+i*(height+2))*(width+2)+neighbor[m];
				for (n=0; n<width; n++)
				{
					if (*outptr[0])
					{
						for (m=1; m<=connectivity/2; m++)
							if (*outptr[m])
							{
								if (*outptr[0] == 65535)
									*outptr[0] = *outptr[m];
								else if (*outptr[0]!=*outptr[m] &&
										eq_tab[*outptr[0]]!=eq_tab[*outptr[m]])
								{
									if (eq_tab[*outptr[0]]> eq_tab[*outptr[m]])
									{
										q = eq_tab[*outptr[0]];
										for (p=q; p<=max_label; p++)
											if (eq_tab[p] == q)
												eq_tab[p] = eq_tab[*outptr[m]];
									}
									else
									{
										q = eq_tab[*outptr[m]];
										for (p=q; p<=max_label; p++)
											if (eq_tab[p] == q)
												eq_tab[p] = eq_tab[*outptr[0]];
									}
								}
							}
						if (*outptr[0] == 65535)
						{
							++max_label;
							*outptr[0] = eq_tab[max_label] = max_label;
							if (max_label == 65535)
							{
    							fprintf(stderr,
									"%s: Too many components.\n", argv[0]);
    					  	 	exit(1);
							}
						}
					}
					for (m=0; m<=connectivity/2; m++)
						outptr[m]++;
				}
			}

	      } /* end forward pass */

		for (p=1; p<=max_label; p++)
		{
			for (m=p; m<=max_label; m++)
				if (eq_tab[m] == p)
					goto next_p;
			for (m=p; eq_tab[m]<p; m++)
				if (m > max_label)
					goto next_p;
			for (ll=m; ll<=max_label; ll++)
				if (eq_tab[ll] == m)
					eq_tab[ll] = p;
			next_p:;
		}
		if (nseeds)
		{
			memset(flags, FALSE, max_label+1);
			for (p=0; p<nseeds; p++)
			{
				m = eq_tab[out_buffer[atoi(argv[3*p+5])+1+(atoi(argv[3*p+6])+1+
					(atoi(argv[3*p+7])+1)*(height+2))*(width+2)]];
				for (ll=m; ll<=max_label; ll++)
					if (eq_tab[ll] == m)
						flags[ll] = TRUE;
				for (ll=0; ll<p; ll++)
					if (eq_tab[out_buffer[atoi(argv[3*ll+5])+1+
							(atoi(argv[3*ll+6])+1+(atoi(argv[3*ll+7])+1)*
							(height+2))*(width+2)]] == m)
						break;
				if (ll == p)
					printf("First seed of component: (%f, %f, %f)\n",
						vh.scn.xypixsz[0]*atoi(argv[3*p+5]),
						vh.scn.xypixsz[1]*atoi(argv[3*p+6]),
						sl.location3[j][atoi(argv[3*p+7])]);
			}
		}
		else
		{
			for (m=0,p=1; p<=max_label; p++)
				if (eq_tab[p] > m)
					m = eq_tab[p];
			if (m > largest_density_value)
				largest_density_value = m;
		}
		if (j == 0)
		{
			vh.scn.largest_density_value[0] = (float)largest_density_value;

			/* Write output 3DViewnix Header */
			error = VWriteHeader(fpout, &vh, group, element);
			if(error && error < 106)
			{
				fprintf(stderr, "ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n"
					,error,group, element);
				exit(error);
			}
		}

		/* Save output volume */
	    for(i=0; i<sl.slices[j]; i++)
		  if (nseeds)
		  {
		    memset(in_buffer1, 0, length);
			for (ll=0; ll<height; ll++)
			{
				outptr[0] = out_buffer+(ll+i*(height+2))*(width+2)+neighbor[0];
				for (n=0; n<width; n++)
					if (outptr[0][n]<65535 && flags[outptr[0][n]])
						in_buffer1[(ll*width+n)/8] |= 128>>(ll*width+n)%8;
			}
			if(VWriteData((char *)in_buffer1, 1, length, fpout, &error))
   		 	{
   				fprintf(stderr, "ERROR: Couldn't write volume #%d.\n", j+1);
   				exit(3);
   			}
		  }
		  else
			for (ll=0; ll<height; ll++)
			{
				outptr[0] = out_buffer+(ll+i*(height+2))*(width+2)+neighbor[0];
				for (n=0; n<width; n++)
					outptr[0][n] = eq_tab[outptr[0][n]];
				if(VWriteData((char *)(out_buffer+1+(width+2)*(1+ll+(height+2)*(1+i))),
						2, width, fpout, &error))
      		 	{
       				fprintf(stderr, "ERROR: Couldn't write volume #%d.\n",j+1);
       				exit(3);
       			}
			}

	} /* end for-loop for volumes[j] */

	VCloseData(fpout);

	if (largest_density_value != vh.scn.largest_density_value[0])
	{
		sprintf(comments, "_%s", argv[2]);
		rename(argv[2], comments);
		sprintf(comments, "correct_min_max %s %s", comments, argv[2]);
		error = system(comments);
		if (error)
			exit(error);
	}

	if(execution_mode == 0)
	{
		printf("Done.\n");
		fflush(stdout);
	}

 
	/* If in Background mode remove the entry from the BG_PROCESS.COM */
    if(execution_mode == 1)
        VDeleteBackgroundProcessInformation();


	exit(0);
}
