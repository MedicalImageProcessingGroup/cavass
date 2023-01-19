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

/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/


/*
	This program performs MASKING on an nD Scene (3DViewnix format).
	It creates one file for each of the objects represented in the MASK file.
	The output is a Grey Scene.


	Each slice is is loaded and MASKED individually based on a mask file.

	The format for the mask file is as follows (the file below contains 'n' masked images):

	- name of file (scene) in which mask was created (100 characters)
	- byte0	(mask byte indicating what objects are represented by the mask image, unsigned char)
	- areas0 (eight unsigned shorts with the value for the area of each mask in pixels)
	- image0(array of unsigned chars containing the mask byte for each pixel, width x height)
	- byte1
	- areas1 
	- image1
	- byte2
	- areas2 
	- image2
		.
		.
		.
		.
	- byte(n-1)
	- areas(n-1)
	- image(n-1)



Author: Roberto J. Goncalves
Date  : 01/29/93

% cc mask2grey.c -o mask2grey -I$VIEWNIX_ENV/INCLUDE  $VIEWNIX_ENV/LIBRARY/lib3dviewnix.a -lX11 -lm

*/


 
#include <cv3dv.h>
 
 

#include "slices.c"


int mask16togrey(unsigned short *in, unsigned char *mask, int n, int w, int h,
   unsigned short *out),
 mask8togrey(unsigned char *in, unsigned char *mask, int n, int w, int h,
   unsigned char *out),
 bin_to_grey(unsigned char *bin_buffer, int length_bin,
   unsigned char *grey_buffer, int min_value, int max_value);


unsigned char onbit[8] = { 1, 2, 4, 8, 16, 32, 64, 128};    /* bits on a byte */
unsigned char binpix[8] = {128, 64, 32, 16, 8, 4, 2, 1};	/* binary pixels */


 
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
 




/* Modified: 12/10/99 new mask file format used by Dewey Odhner. */
int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin, 		/* input file */
		 *fpout[8],		/* output files (one for each object) */
		 *fpmask;		/* input file containing the mask information */
	unsigned char bit,	/* byte representing the used objects in a given mask */
				  gbit;	/* byte representing the used objtecs in the entire MASK file */
	int nmask;			/* number of masks in the MASK file */
	int final_volume,	/* final volume for masking */
		final_slice;	/* final slice for masking */
	char mother[101];	/* name of the 3dviewnix mother file */
	int execution_mode;	/* execution mode */
	ViewnixHeader vh;	/* 3DViewnix header */
	SLICES	sl;			/* Structure containing information about the slices of the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;			/* length of a slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int i,j,k, l;		/* general use */
	int error;			/* error code */
	char comments[300];	/* used to modify the header (description field) */
	int binary_flag=0;	/* indicates if scene is binary or not */
	int object_number;  /* index of the object being converted (0=first, -1=all) */
    char filenames[8][500]; /* name for the output files for each object */
	int nobjects;

	unsigned char *in_buffer1b;
	unsigned char *in_buffer8b, *out_buffer8;
	unsigned short *in_buffer16, *out_buffer16;

	unsigned char *mask;
	int new_format;		/* indicates if mask file is Version 1.5 */



	if(argc<5)
	{
		printf("Usage:\n");
		printf("%% mask2grey mask output object mode [name1 name2 ... nameN]\n");
		printf("where:\n");
		printf("mask     : name of MASK file;\n");
		printf("output   : name of output file (without IM0 or BIM termination);\n");
		printf("object   : index of the object being saved (0=first, -1=all objects)\n");
		printf("mode     : mode of operation (0=foreground, 1=background);\n");
		printf("Optionals: names for each of the scenes generated.\n");
		printf("nameI    : if 'object= -1' than the user can specify the individual filename for each object\
n");
		exit(1);
	}
	


	/* Open MASK file */
    if( (fpmask = fopen(argv[1], "rb")) == NULL)
    {
        printf("ERROR: Can't open input MASK file !\n");
        exit(1);
    }

	/* Open mother file name */
	for(i=0; i<101; i++) mother[i] = 0;
	if( fread(mother, 100, 1, fpmask) != 1)
    {
        printf("ERROR: Can't read MASK file !\n");
        exit(1);
	}
	new_format = strcmp(mother, "///")==0;
	 
 
    /* Open INPUT scene */
    if( (fpin = fopen(mother+4*new_format, "rb")) == NULL)
    {
        printf("ERROR: Can't open INPUT file !\n");
        exit(1);
    }

	/* Get OBJECT NUMBER */
    object_number = atoi(argv[3]);
 
    /* Get EXECUTION MODE */
    sscanf(argv[4], "%d", &execution_mode);



	/* Get the filename for each object */
    for(i=0; i<8; i++)
        sprintf(filenames[i], "%s_%d.IM0", argv[2], i);
    /* replace by name given by user (if applicable) */
    if(argc > 5)
        for(i=5; i<argc; i++)
            sprintf(filenames[i-5], "%s.IM0", argv[i]);
    /* IF ONLY ONE OBJECT */
    if(object_number >= 0)
        sprintf(filenames[0], "%s.IM0", argv[2]);


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(mother+4*new_format, &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);


	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	length = width * height * vh.scn.num_of_bits / 8;


	/*---------------------------------------*/
	/* Assemble information about MASK file: */
	/* - starting image */
	/* - ending image */
	/* - total number of output scenes */
	/*---------------------------------------*/
	/* Check how many slices in the mask file, and the global object usage */
	nmask = gbit = 0;
	while( nmask<sl.total_slices && fread(&bit, 1, 1, fpmask) == 1  &&
            fseek(fpmask, ((new_format? 32:16)+width*height), 1) == 0)
	{
		gbit |= bit;
		nmask++;
	}
	/* Put pointer in the first mask */
	fseek(fpmask, 100, 0);
	
	/* check what volume and image is the final one */
	if(sl.sd == 3)
	{
		final_slice = nmask-1;
		final_volume = 0;
	}
	else
	{
		i = k = 0;
		while(k < nmask)
		{
			k += sl.slices[i];
			i++;
		}

		final_volume = i-1;
		final_slice = nmask - sl.slice_index[final_volume][0]-1;
	
	}


	/* Get No of objects specified on the file */
	nobjects = 0;
	for(i=0; i<8; i++)
	if( (gbit & onbit[i]) > 0)
		nobjects++;
 
 


	/* More than 1 output file */
	if(object_number < 0)
	{
		j = 0;
		for(i=0; i<8; i++)
		{
			if( (gbit & onbit[i]) > 0)
			{
    			if( (fpout[i] = fopen(filenames[i], "wb")) == NULL)
    			{
        			printf("ERROR: Can't open OUTPUT file !\n");
        			exit(1);
    			}
				j++;
			}
		}
	}
	else
	{
		if( (fpout[0] = fopen(filenames[0], "wb")) == NULL)
        {
            printf("ERROR: Can't open OUTPUT file !\n");
            exit(1);
        }
	}



	/*-----------------*/
	/* Allocate memory */
	if(vh.scn.num_of_bits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1b = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
		length = length*8;
		binary_flag = 1;

    	/* create buffer for one grey image */
    	if( (in_buffer8b = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey-level image */
    	if( (out_buffer8 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	if(vh.scn.num_of_bits == 8)
	{
    	/* create buffer for one grey image */
    	if( (in_buffer8b = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer8 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	else
	{
    	/* create buffer for one grey image */
    	if( (in_buffer16 = (unsigned short *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer16 = (unsigned short *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}

	
	/*----------------------------------*/
   	/* create buffer for one grey image */
   	if( (mask = (unsigned char *) malloc(width*height) ) == NULL)
   	{
   		printf("ERROR: Can't allocate MASK image buffer.\n");
   		exit(1);
   	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
	/* Change header to binary */
	/* Correct for eventual new Number of Subscenes Tree */
	/* 4D Scene */
	/*if(sl.sd == 4)*/
	if(final_volume > 0)
	{
		vh.scn.num_of_subscenes[0] = final_volume+1;
		k=1;
		for(i=0; i< final_volume  ; i++)
		{
			vh.scn.num_of_subscenes[k] = sl.slices[i];
			k++;
		}
		vh.scn.num_of_subscenes[k] = final_slice + 1;
	}
	else
	/* 3D Scene */
	{
		vh.scn.dimension = 3;
		vh.scn.num_of_subscenes[0] = final_slice+1;
	}


	/* Correct for eventual new Location Tree */
	/* 4D Scene */
	/*if(sl.sd == 4)*/
	if(final_volume > 0)
	{
		k = 0;
		for(i=0; i<= final_volume  ; i++)
		{
			vh.scn.loc_of_subscenes[i] = sl.location4[i];

			/* Even though the final volume should be trated differently, we don't do it because */
			/* the different #of slices is taken care by the 'num_of_subscenes' field */
			for(j=0; j<sl.slices[i]; j++)
			{
				vh.scn.loc_of_subscenes[k+i] = sl.location3[i][j];
				k++;
			}
			
		}
	}
	else
	/* 3D Scene */
	{
		for(i=0; i<=final_slice ; i++)
			vh.scn.loc_of_subscenes[i] = sl.location3[0][i];
	}
	


    /* Build "description" header entry */
	strcpy(comments, "");
    for(i=0;i<argc&&strlen(comments)+strlen(argv[i])+2<sizeof(comments); i++)
    {
      	strcat(comments,argv[i]);
      	strcat(comments," ");
    }
    vh.scn.description = (char *) malloc( strlen(comments) + 1);
    strcpy(vh.scn.description, comments);
    vh.scn.description_valid = 0x1;

	/* Write output 3DViewnix Header */
	if(gbit==1  ||  object_number >= 0)
	{
		/* Get the filenames right (own and parent) */
    	strncpy(vh.gen.filename1, mother+4*new_format,
			sizeof(vh.gen.filename1)-1);
		vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    	strncpy(vh.gen.filename, filenames[0], sizeof(vh.gen.filename)-1);
		vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
		error = VWriteHeader(fpout[0], &vh, group, element);
		if(error < 100)
		{
			printf("ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
			exit(error);
		}
		fseek(fpout[0], 0, 2);
	}
	else
	for(i=0; i<8; i++)
	{
		if( (gbit & onbit[i]) > 0)
		{
			/* Get the filenames right (own and parent) */
    		strncpy(vh.gen.filename1, mother+4*new_format,
				sizeof(vh.gen.filename1)-1);
			vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    		strncpy(vh.gen.filename,filenames[i], sizeof(vh.gen.filename)-1);
			vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;

			error = VWriteHeader(fpout[i], &vh, group, element);
			if(error < 100)
			{
				printf("ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
				exit(error);
			}
			fseek(fpout[i], 0, 2);
		}
	}

	
	/* Traverse ALL VOLUMES */
	k=0;
	for(j=0; j<=final_volume; j++)
	{
	/* For each Volume, traverse ALL SLICES */
	for(i=0; i<=final_slice; i++)
	{
		/* Seek the appropriate location */
		if(binary_flag == 1)
			fseek(fpin, k*length/8+hlength, 0);
		else
			fseek(fpin, k*length+hlength, 0);

		if(execution_mode == 0)
		{
			if(sl.volumes > 1)
			printf("Masking volume #%d/%d, slice #%d/%d ...\n", j+1,sl.volumes,i+1, final_volume+1);
			else
			printf("Masking slice #%d/%d ...\n", i+1, final_slice+1);

			fflush(stdout);
		}

		/* Load MASK */
		fseek(fpmask, 1+(new_format? 32:16), 1);
		if(fread(mask, (width*height), 1, fpmask) != 1)
        {
        	printf("ERROR: Couldn't read MASK #%d of volume #%d.\n", i+1, j+1);
           	exit(2);
        }

		/* 1 or 8 Bits/Pixel */
		if(vh.scn.num_of_bits == 8)
		{
			/* Load input slice */
			/* BINARY */
			if(binary_flag == 1)
			{
				if(fread(in_buffer1b, 1, (length/8), fpin) != length/8)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}
				
				/* Convert to 8 Bits */
				bin_to_grey(in_buffer1b, (length/8), in_buffer8b, 0, 255);
			}
			/* 8 BITS/PIXEL */
			else
			{
				if(fread(in_buffer8b, 1, length, fpin) != length)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}
			}

			/* SAVE ONLY ONE OBJECT */
			if(object_number >= 0)
			{
				mask8togrey(in_buffer8b, mask, object_number, width, height, out_buffer8);
				/* Save output slice */
        		if(fwrite(out_buffer8, 1, length, fpout[0]) != length)
        		{
           			printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           			exit(3);
        		}
			}
			else
			/* For each Valid OBJECT, perform the MASKING operation */
			for(l=0; l<8; l++)
			{
				/* If object is valid */
				if( (gbit & onbit[l]) > 0)
				{
					mask8togrey(in_buffer8b, mask, l, width, height, out_buffer8);

					/* Save output slice */
        			if(fwrite(out_buffer8, 1, length, fpout[l]) != length)
        			{
           				printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           				exit(3);
        			}
				}
			}

		}
		/* 16 Bits/Pixel */
		else
		{
			/* Load input slice */
			if(fread(in_buffer16, 1, length, fpin) != length)
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

			/* SAVE ONLY ONE OBJECT */
			if(object_number >= 0)
			{
				mask16togrey(in_buffer16, mask, object_number, width, height, out_buffer16);
				/* Save output slice */
        		if(fwrite(out_buffer16, 1, length, fpout[0]) != length)
        		{
           			printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           			exit(3);
        		}
			}
			else
			/* For each Valid OBJECT, perform the MASKING operation */
			for(l=0; l<8; l++)
			{
				if((gbit & onbit[l])  > 0)
				{
					mask16togrey(in_buffer16, mask, l, width, height, out_buffer16);

					/* Save output slice */
        			if(fwrite(out_buffer16, 1, length, fpout[l]) != length)
        			{
           				printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           				exit(3);
        			}
				}
			}

		}
		k++;


	}
	}


	/* Close files */
    if(object_number < 0)
    {
        for(i=0; i<8; i++)
            if( (gbit & onbit[l]) > 0)
                fclose(fpout[i]);
    }
    else
        fclose(fpout[0]);

	if(execution_mode == 0)
	{
		printf("Done.\n");
		fflush(stdout);
	}



	exit(0);
}


/*------------------------------------------------------------------------------------*/
int mask8togrey(unsigned char *in, unsigned char *mask, int n, int w, int h,
    unsigned char *out)
//unsigned char *in;   /* Input Image */
//unsigned char *mask; /* Mask */
//int n,               /* Object number (bit) */
//    w, h;            /* size of images */
//unsigned char *out;  /* Output grey masked image */
{
	register int i;
	int size;

	size = w*h;


	for(i=0; i<size; i++)
	{
		if( (mask[i] & onbit[n]) > 0)
			out[i] = in[i];
		else
			out[i] = 0;
	}
	return 0;
}


/*------------------------------------------------------------------------------------*/
int mask16togrey(unsigned short *in, unsigned char *mask, int n, int w, int h,
    unsigned short *out)
//unsigned short *in;  /* Input Image */
//unsigned char *mask; /* Mask */
//int n,               /* Object number (bit) */
//    w, h;            /* size of images */
//unsigned short *out; /* Output grey masked image */
{
	register int i;
	int size;

	size = w*h;

	for(i=0; i<size; i++)
	{
		if( (mask[i] & onbit[n]) > 0)
			out[i] = in[i];
		else
			out[i] = 0;
	}
	return 0;
}


/*---------------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
int bin_to_grey(unsigned char *bin_buffer, int length_bin,
   unsigned char *grey_buffer, int min_value, int max_value)
{
        register int i, j;
        unsigned char mask[8];
        unsigned char *bin, *grey;
 
        bin = bin_buffer;
        grey = grey_buffer;
 
        /* initialize masks */
        mask[0] = 1;
        for(i=1; i<8; i++)
           mask[i] = mask[i-1] * 2;
 
        for(i=length_bin; i>0; i--)
        {
           for(j=7; j>=0; j--)
           {
                if( (*bin & mask[j]) != 0) *grey = max_value;
                else *grey = min_value;
 
                grey++;
           }
           bin++;
        }
 
		return 0;
}
 

