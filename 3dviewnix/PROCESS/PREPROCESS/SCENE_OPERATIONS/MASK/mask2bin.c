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
	The output is a Binary Scene.


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

% cc mask2bin.c -o mask2bin  -I$VIEWNIX_ENV/INCLUDE $VIEWNIX_ENV/LIBRARY/lib3dviewnix.a -lX11 -lm

*/


 
#include <cv3dv.h>
#include "slices.c"


void mask8tobin(unsigned char *mask, int n, int w, int h, unsigned char *out),
 mask8togrey(unsigned char *in, unsigned char *mask, int n, int w, int h,
   unsigned char *out),
 mask16togrey(unsigned short *in, unsigned char *mask, int n, int w, int h,
   unsigned short *out);


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
        exit(-1);
    }
 
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        exit(-1);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        exit(-1);
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
	int nbits;			/* number of bits on the original scene */
	char mother[101];	/* name of the 3dviewnix mother file */
	int execution_mode;	/* execution mode */
	ViewnixHeader vh;	/* 3DViewnix header */
	SLICES	sl;			/* Structure containing information about the slices of the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;			/* length of a slice */
	int length_bin;		/* length of a binary slice (in bytes) */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int i,j,k, l;		/* general use */
	int error;			/* error code */
	char comments[300];	/* used to modify the header (description field) */
	int object_number;	/* index of the object being converted (0=first, -1=all) */
	char filenames[8][500];	/* name for the output files for each object */
	int nobjects;		/* No. of objects in the file */

	unsigned char *out_buffer1;

	unsigned char *mask;
	int new_format;		/* indicates if mask file is Version 1.5 */



	if(argc<5)
	{
		printf("Usage:\n");
		printf("%% mask2bin mask output object mode [name1 name2 ... nameN]\n");
		printf("where:\n");
		printf("mask     : name of MASK file;\n");
		printf("output   : name of output file (without IM0 or BIM termination);\n");
		printf("object   : index of the object being saved (0=first, -1=all objects)\n");
		printf("mode     : mode of operation (0=foreground, 1=background);\n");
		printf("Optionals: names for each of the scenes generated.\n");
		printf("nameI    : if 'object= -1' than the user can specify the individual filename for each object\n");
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
		sprintf(filenames[i], "%s_%d.BIM", argv[2], i);
	/* replace by name given by user (if applicable) */
	if(argc > 5)
		for(i=5; i<argc; i++)
			sprintf(filenames[i-5], "%s.BIM", argv[i]);
	/* IF ONLY ONE OBJECT */
	if(object_number >= 0)
		sprintf(filenames[0], "%s.BIM", argv[2]);

 
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
	length_bin = (width * height + 7) / 8;
	nbits = vh.scn.num_of_bits;


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
		

	/* OPEN OUTPUT FILES */
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
	/* ONLY ONE OBJECT */
	{
    	if( (fpout[0] = fopen(filenames[0], "wb")) == NULL)
    	{
       		printf("ERROR: Can't open OUTPUT file !\n");
       		exit(1);
    	}
		
	}



   	/* create buffer for one binary image */
   	if( (out_buffer1 = (unsigned char *) malloc(length_bin) ) == NULL)
   	{
   		printf("ERROR: Can't allocate output image buffer.\n");
   		exit(1);
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
	vh.scn.num_of_bits = 1;
	vh.scn.bit_fields[0] = 0;
	vh.scn.bit_fields[1] = 0;
	vh.scn.smallest_density_value[0] = 0;
	vh.scn.largest_density_value[0] = 1;

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
	/* Only ONE object */
	if(gbit==1  || object_number >= 0)
	{
		/* Get the filenames right (own and parent) */
    	strncpy(vh.gen.filename1, mother+4*new_format,
			sizeof(vh.gen.filename1)-1);
		vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    	strncpy(vh.gen.filename, filenames[0], sizeof(vh.gen.filename)-1);
		vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
		error = VWriteHeader(fpout[0], &vh, group, element);
		if(error && error < 106)
		{
			printf("ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
			exit(error);
		}
		fseek(fpout[0], 0, 2);
	}
	else
	/* ALL OBJECTS on the file */
	for(i=0; i<8; i++)
	{
		if( (gbit & onbit[i]) > 0 )
		{
			/* Get the filenames right (own and parent) */
    		strncpy(vh.gen.filename1, mother+4*new_format,
				sizeof(vh.gen.filename1)-1);
			vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
    		strncpy(vh.gen.filename,filenames[i], sizeof(vh.gen.filename)-1);
			vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;

			error = VWriteHeader(fpout[i], &vh, group, element);
			if(error && error < 106)
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
	for(i=0; i<=final_slice; i++, k++)
	{

		if(execution_mode == 0)
		{
			if(sl.volumes > 1)
			printf("Masking volume #%d/%d, slice #%d/%d ...\n", j+1,sl.volumes, i+1, final_slice+1);
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
			

			/* SAVE ONLY ONE OBJECT */
			if(object_number >= 0)
			{
				mask8tobin(mask, object_number, width, height, out_buffer1);
				/* Save output slice */
        		if(fwrite(out_buffer1, 1, length_bin, fpout[0]) != length_bin)
        		{
           			printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           			exit(3);
        		}
			}
			else
			/* SAVE ALL AVAILABLE OBJECTS */
			{
				/* Calculate Image for Each Valid Object  */
				for(l=0; l<8; l++)
				{
					if( (gbit & onbit[l]) > 0)
					{
						mask8tobin(mask, l, width, height, out_buffer1);

						/* Save output slice */
        				if(fwrite(out_buffer1, 1, length_bin, fpout[l]) != length_bin)
        				{
           					printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           					exit(3);
        				}
					}
				}
			}



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
void mask8tobin(unsigned char *mask, int n, int w, int h, unsigned char *out)
//unsigned char *mask;        /* Input Mask */
//int n,                      /* Object number (bit) */
//    w, h;                   /* size of images */
//unsigned char *out;         /* Output binary masked image */
{
	register int i,j,k;
	int sizebin;

	sizebin = (w*h+7)/8;

	k=0;
	for(i=0; i<sizebin; i++)
	{
		out[i] = 0;
		for(j=0; j<8&&k<w*h; j++)
		{
			if( (mask[k] & onbit[n]) > 0)
				out[i] += binpix[j];
			k++;
		}
	}
}




/*------------------------------------------------------------------------------------*/
void mask8togrey(unsigned char *in, unsigned char *mask, int n, int w, int h,
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
}


/*------------------------------------------------------------------------------------*/
void mask16togrey(unsigned short *in, unsigned char *mask, int n, int w, int h,
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
}


/*---------------------------------------------------------------------------------------*/


