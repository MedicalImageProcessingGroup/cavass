/*
  Copyright 1993-2015 Medical Image Processing Group
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

	The type of filter applied is:

	SOBEL


	Each slice is is loaded and filtered individually.



Author: Roberto J. Goncalves
Date  : 11/02/92

% cc sobel.c -o sobel  $VIEWNIX_ENV/INCLUDE $VIEWNIX_ENV/LIBRARY/3dviewnix.a -lX11 -lm

*/


#include <math.h>
 
#include <cv3dv.h>
 
 

#include "slices.c"

#include "fff.c"

 
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
	int binary_flag=0;	/* indicates if scene is binary or not */
	float MIN, MAX;		/* min and max intensity values */
	int ll;

	unsigned char *in_buffer1;
	unsigned char *in_buffer8, *out_buffer8;
	unsigned short *in_buffer16, *out_buffer16;


	if(argc<4)
	{
		printf("Usage:\n");
		printf("%% sobel input output mode\n");
		printf("where:\n");
		printf("input    : name of input file;\n");
		printf("output   : name of output file;\n");
		printf("mode     : mode of operation (0=foreground, 1=background);\n");
		exit(1);
	}
	


	 
 
    /* Open INPUT and OUTPUT Files */
    if( (fpin = fopen(argv[1], "rb")) == NULL)
    {
        printf("ERROR: Can't open INPUT file !\n");
        exit(1);
    }
    if( (fpout = fopen(argv[2], "wb")) == NULL)
    {
        printf("ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }
 
    /* Get EXECUTION MODE */
    sscanf(argv[3], "%d", &execution_mode);

 


	/* If in background mode, then place an entry in the BG_PROCESS.COM file */
    if(execution_mode == 1)
        VAddBackgroundProcessInformation("ndinterpolate");


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);


	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	length = width * height * vh.scn.num_of_bits / 8;
	if(vh.scn.num_of_bits == 1)
		length = (width * height + 7) / 8;
	MIN = vh.scn.smallest_density_value[0];
    MAX = vh.scn.largest_density_value[0];


	/* Allocate memory */
	if(vh.scn.num_of_bits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
		length = length*8;
		binary_flag = 1;

    	/* create buffer for one grey image */
    	if( (in_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey-level image */
    	if( (out_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	if(vh.scn.num_of_bits == 8)
	{
    	/* create buffer for one grey image */
    	if( (in_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	else
	{
    	/* create buffer for one grey image */
    	if( (in_buffer16 = (unsigned short *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer16 = (unsigned short *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
	/* In case of binary, change header to 8 bits */
	if(vh.scn.num_of_bits == 1)
	{
		vh.scn.num_of_bits = 8;
		vh.scn.bit_fields[0] = 0;
		vh.scn.bit_fields[1] = 7;
		vh.scn.smallest_density_value[0] = MIN = 0;
		vh.scn.largest_density_value[0] = MAX = 255;
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
    vh.scn.description = (char *) malloc( strlen(comments) + 1);
    strcpy(vh.scn.description, comments);
    vh.scn.description_valid = 0x1;

	/* Write output 3DViewnix Header */
	error = VWriteHeader(fpout, &vh, group, element);
	if(error < 100)
	{
		printf("ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
		exit(error);
	}

	
	/* Traverse ALL VOLUMES */
	k=0;
	for(j=0; j<sl.volumes; j++)
	/* For each Volume, traverse ALL SLICES */
	for(i=0; i<sl.slices[j]; i++, k++)
	{
		/* Seek the appropriate location */
		if(binary_flag == 1)
			fseek(fpin, (k*length/8)+hlength, 0);
		else
			fseek(fpin, k*length+hlength, 0);

		if(execution_mode == 0)
		{
			if(sl.volumes > 1)
			printf("Filtering volume #%d/%d, slice #%d/%d ...\n", j+1,sl.volumes,i+1, sl.slices[j]);
			else
			printf("Filtering slice #%d/%d ...\n", i+1, sl.slices[j]);

			fflush(stdout);
		}


		/* 1 or 8 Bits/Pixel */
		if(vh.scn.num_of_bits == 8)
		{
			/* Load input slice */
			/* BINARY */
			if(binary_flag == 1)
			{
				if(fread(in_buffer1, 1, (length/8), fpin) != length/8)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}
				
				/* Convert to 8 Bits */
				bin_to_grey(in_buffer1, (length/8), in_buffer8, 0, 255);
			}
			/* 8 BITS/PIXEL */
			else
			{
				if(fread(in_buffer8, 1, length, fpin) != length)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}
			}

			/* Filter */
			sobel8(in_buffer8, width, height, out_buffer8, MIN, MAX);

			/* Save output slice */
        	if(fwrite(out_buffer8, 1, width*height, fpout) != width*height)
        	{
           		printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}
		/* 16 Bits/Pixel */
		else
		{
			/* Load input slice */
			VReadData((char *)in_buffer16, 2, length/2, fpin, &ll);
			if( ll != length/2)
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

			/* Filter */
			sobel16(in_buffer16, width, height, out_buffer16, MIN, MAX);

			/* Save output slice */
			VWriteData((char *)out_buffer16, 2, width*height, fpout, &ll);
			if(ll != length/2)
        	{
           		printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}


	}


	VCloseData(fpout);

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
