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



	Each slice is is loaded and filtered individually.



Author: Roberto J. Goncalves
Date  : 11/02/92


Modified for morphological operations by Dewey Odhner
Date: 11/10/00

*/


#include <math.h>
 
#include <cv3dv.h>
#include <assert.h>
 
 

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


/*****************************************************************************
 * FUNCTION: maxima_8
 * DESCRIPTION: Applies a maximaological filter to a gray image of 8 bits per
 *    pixel.
 * PARAMETERS:
 *    in1, in2, in3: the input data arrays of three input slices
 *    width, height: size of the image in pixels
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 7/13/15 by Dewey Odhner
 *
 *****************************************************************************/
void maxima_8( unsigned char *in1, unsigned char *in2, unsigned char *in3, int width, int height, unsigned char *out)
{
	unsigned char *t_in=NULL, *c_in, *t_out;
	int n, x, y;
	static int neighbor[26][3]={
		{ 0, 0,-1},
		{ 0, 0, 1},
		{-1, 0, 0},
		{ 1, 0, 0},
		{ 0,-1, 0},
		{ 0, 1, 0},
		{-1,-1, 0},
		{ 1,-1, 0},
		{-1, 1, 0},
		{ 1, 1, 0},
		{-1, 0,-1},
		{ 1, 0,-1},
		{-1, 0, 1},
		{ 1, 0, 1},
		{ 0,-1,-1},
		{ 0, 1,-1},
		{ 0,-1, 1},
		{ 0, 1, 1},
		{-1,-1,-1},
		{ 1,-1,-1},
		{-1, 1,-1},
		{ 1, 1,-1},
		{-1,-1, 1},
		{ 1,-1, 1},
		{-1, 1, 1},
		{ 1, 1, 1}};

	for (y=0; y<height; y++)
		for (x=0; x<width; x++)
		{
			c_in = in2+y*width+x;
		   	t_out = out + y*width+x;
			*t_out = 0;
			for (n=0; n<26; n++)
			{
				if (y+neighbor[n][1]<0 || y+neighbor[n][1]>=height ||
						x+neighbor[n][0]<0 || x+neighbor[n][0]>=width)
					continue;
				switch (neighbor[n][2])
				{
					case -1:
						t_in = in1+(y+neighbor[n][1])*width+x+neighbor[n][0];
						break;
					case 0:
						t_in = in2+(y+neighbor[n][1])*width+x+neighbor[n][0];
						break;
					case 1:
						t_in = in3+(y+neighbor[n][1])*width+x+neighbor[n][0];
						break;
				}
				if (*t_in < *c_in)
					(*t_out)++;
				else if (*t_in > *c_in)
				{
					*t_out = 0;
					break;
				}
			}
			*t_out = *t_out>=25;
		}
	return;
}

/*****************************************************************************
 * FUNCTION: maxima_16
 * DESCRIPTION: Applies a maximaological filter to a gray image of 16 bits per
 *    pixel.
 * PARAMETERS:
 *    in1, in2, in3: the input data arrays of three input slices
 *    width, height: size of the image in pixels
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 7/13/15 by Dewey Odhner
 *
 *****************************************************************************/
void maxima_16( unsigned short *in1,unsigned short *in2, unsigned short *in3, int width, int height, unsigned char *out)
{
	unsigned short *t_in=NULL, *c_in;
	unsigned char *t_out;
	int n, x, y;
	static int neighbor[26][3]={
		{ 0, 0,-1},
		{ 0, 0, 1},
		{-1, 0, 0},
		{ 1, 0, 0},
		{ 0,-1, 0},
		{ 0, 1, 0},
		{-1,-1, 0},
		{ 1,-1, 0},
		{-1, 1, 0},
		{ 1, 1, 0},
		{-1, 0,-1},
		{ 1, 0,-1},
		{-1, 0, 1},
		{ 1, 0, 1},
		{ 0,-1,-1},
		{ 0, 1,-1},
		{ 0,-1, 1},
		{ 0, 1, 1},
		{-1,-1,-1},
		{ 1,-1,-1},
		{-1, 1,-1},
		{ 1, 1,-1},
		{-1,-1, 1},
		{ 1,-1, 1},
		{-1, 1, 1},
		{ 1, 1, 1}};

	for (y=0; y<height; y++)
		for (x=0; x<width; x++)
		{
			c_in = in2+y*width+x;
		   	t_out = out + y*width+x;
			*t_out = 0;
			for (n=0; n<26; n++)
			{
				if (y+neighbor[n][1]<0 || y+neighbor[n][1]>=height ||
						x+neighbor[n][0]<0 || x+neighbor[n][0]>=width)
					continue;
				switch (neighbor[n][2])
				{
					case -1:
						t_in = in1+(y+neighbor[n][1])*width+x+neighbor[n][0];
						break;
					case 0:
						t_in = in2+(y+neighbor[n][1])*width+x+neighbor[n][0];
						break;
					case 1:
						t_in = in3+(y+neighbor[n][1])*width+x+neighbor[n][0];
						break;
				}
				if (*t_in < *c_in)
					(*t_out)++;
				else if (*t_in > *c_in)
				{
					*t_out = 0;
					break;
				}
			}
			*t_out = *t_out>=25;
		}
	return;
}



int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin, *fpout;	/* inpput/output files */
	ViewnixHeader vh;	/* 3DViewnix header */
	SLICES	sl;			/* Structure containing information about the slices of the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;			/* length of a slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int nbits;			/* Number of bits of input data */
	int i,j,k;			/* general use */
	int error;			/* error code */
	char *comments;     /* used to modify the header (description field) */
	float space,pixel;
	int ll, m;

	unsigned char *in_buffer1;
	unsigned char *in_buffer8a, *in_buffer8b, *in_buffer8c, *out_buffer8, *temp8;
	unsigned short *in_buffer16a, *in_buffer16b, *in_buffer16c, *temp16;


	if(argc != 3)
	{
		printf("Usage:\n");
		printf("%s input output\n", argv[0]);
		printf("where:\n");
		printf("input    : name of input file;\n");
		printf("output   : name of output file;\n");
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


    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);


	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	nbits = vh.scn.num_of_bits;
	length = width * height * nbits / 8;
	if(nbits == 1)
		length = (width * height + 7) / 8;
	pixel = vh.scn.xypixsz[0];
	space = sl.Min_spacing3;


	/* Allocate memory */
	if(nbits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}

		length = length*8;

    	/* create buffer for one grey image */
    	if( (in_buffer8b = (unsigned char *) calloc(1, length) ) == NULL  ||
    		(in_buffer8a = (unsigned char *) calloc(1, length) ) == NULL  ||
    		(in_buffer8c = (unsigned char *) calloc(1, length) ) == NULL  ||
    		(out_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
	}
	else
	if(nbits == 8)
	{
    	/* create buffer for one grey image */
    	if( (in_buffer8b = (unsigned char *) calloc(1, length) ) == NULL  ||
    		(in_buffer8a = (unsigned char *) calloc(1, length) ) == NULL  ||
    		(in_buffer8c = (unsigned char *) calloc(1, length) ) == NULL  ||
    		(out_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
	}
	else
	{
    	/* create buffer for one grey image */
    	if( (in_buffer16b = (unsigned short *) calloc(1, length) ) == NULL  ||
    		(in_buffer16a = (unsigned short *) calloc(1, length) ) == NULL  ||
    		(in_buffer16c = (unsigned short *) calloc(1, length) ) == NULL  ||
    		(out_buffer8 = (unsigned char *) calloc(width*height, 1) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
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
	vh.scn.num_of_bits = 1;
	vh.scn.bit_fields[1] = 0;
	if (vh.scn.largest_density_value_valid)
		vh.scn.largest_density_value[0] = 1;
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


	/************************/	
	/* Traverse ALL VOLUMES */
	k=0;	/* k=index of the current slice (within all slices) */
	for(j=0; j<sl.volumes; j++)
	{
		/* Seek the appropriate location */
		if(nbits == 1)
			fseek(fpin, (k*length/8)+hlength, 0);
		else
			fseek(fpin, k*length+hlength, 0);


	   /* Read slices for filtering first slice on the scene (a=b=1st, c=2nd slice) */
	   if(nbits == 1)
	   {
		   /* read first slice */
		   if(fread(in_buffer1, 1, (length/8), fpin) != length/8)
       	   {
       		   printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
       		   exit(2);
       	   }
		   bin_to_grey(in_buffer1, (length/8), in_buffer8b, 0, 255);

		   for(i=length-1; i>=0; i--)
			   in_buffer8a[i] = 0;
			
	   }
	   /* 8 BITS/PIXEL */
	   else
	   if (nbits <= 8)
	   {
		   if(fread(in_buffer8b, 1, length, fpin) != length)
       	   {
       		   printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
       		   exit(2);
       	   }

		   memcpy(in_buffer8a, in_buffer8b, length);
	   }
	   /* 16 BITS/PIXEL */
	   else
	   {
			VReadData((char *)in_buffer16b, 2, length/2, fpin, &ll);
			if( ll != length/2)
       	   	{
       		   printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
       		   exit(2);
       	   	}
			memcpy(in_buffer16a, in_buffer16b, length);
	   }
	
	  /*--------------------------------------*/
	  /* For each Volume, traverse ALL SLICES */
	  for(i=0; i<sl.slices[j]; i++)
	  {

		/*--------------------*/
		/* LOAD NEXT SLICE IN */

		if(i>0)
		{
			if(nbits <= 8)
			{
				temp8 = in_buffer8a;
				in_buffer8a = in_buffer8b;
				in_buffer8b = in_buffer8c;
				in_buffer8c = temp8;
			}
			else
			{
				temp16 = in_buffer16a;
				in_buffer16a = in_buffer16b;
				in_buffer16b = in_buffer16c;
				in_buffer16c = temp16;
			}
		}


		/* 1 or 8 Bits/Pixel */
		if(nbits <= 8)
		{
			/* Load input slice */
			/* BINARY */
			if(nbits == 1)
			{
				/* 3rd slice */
				if (i < sl.slices[j]-1)
				{
				  if (fread(in_buffer1, 1, (length/8), fpin) != length/8)
       			  {
       				printf("ERROR: Couldn't read slice #%d of volume #%d.\n",
					  i+1, j+1);
       				exit(2);
       			  }
				  bin_to_grey(in_buffer1, (length/8), in_buffer8c, 0, 255);
				}
				else
				  memcpy(in_buffer8c, in_buffer8b, length);
			}
			/* 8 BITS/PIXEL */
			else
			{
				/* 3rd slice */
				if (i < sl.slices[j]-1)
				{
				  if (fread(in_buffer8c, 1, length, fpin) != length)
       			  {
       				printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
       				exit(2);
       			  }
				}
				else
				  memcpy(in_buffer8c, in_buffer8b, length);
			}

			/* Filter */
			maxima_8(in_buffer8a, in_buffer8b, in_buffer8c, width,height,
								out_buffer8);

			/* Save output slice */
			for (ll=0; ll<width*height/8; ll++)
				out_buffer8[ll] =
					(out_buffer8[ll*8]? 128:0) |
					(out_buffer8[ll*8+1]? 64:0) |
					(out_buffer8[ll*8+2]? 32:0) |
					(out_buffer8[ll*8+3]? 16:0) |
					(out_buffer8[ll*8+4]? 8:0) |
					(out_buffer8[ll*8+5]? 4:0) |
					(out_buffer8[ll*8+6]? 2:0) |
					(out_buffer8[ll*8+7]? 1:0);
			if (width*height%8)
			{
				m = 0;
				for (ll=0; ll<width*height%8; ll++)
					if (out_buffer8[width*height/8*8+ll])
						m |= 128>>ll;
				out_buffer8[width*height/8] = m;
			}
			if (fwrite(out_buffer8, 1, (width*height+7)/8, fpout) !=
					(width*height+7)/8)
       		{
       			printf("ERROR: Couldn't write slice #%d of volume #%d.\n",
					i+1, j+1);
       			exit(3);
       		}
		}
		/* 16 Bits/Pixel */
		else
		{
			/* 3rd slice */
			if( i < sl.slices[j] - 1)
			{
				VReadData((char *)in_buffer16c, 2, length/2, fpin, &ll);
				if( ll != length/2)
       			{
       				printf("ERROR: Couldn't read slice #%d of volume #%d.\n",
						i+1, j+1);
       				exit(2);
       			}
			}
			else
				memcpy(in_buffer16c, in_buffer16b, length);

			/* Filter */
			maxima_16(in_buffer16a, in_buffer16b, in_buffer16c,
								width, height, out_buffer8);

			/* Save output slice */
			for (ll=0; ll<width*height/8; ll++)
				out_buffer8[ll] =
					(out_buffer8[ll*8]? 128:0) |
					(out_buffer8[ll*8+1]? 64:0) |
					(out_buffer8[ll*8+2]? 32:0) |
					(out_buffer8[ll*8+3]? 16:0) |
					(out_buffer8[ll*8+4]? 8:0) |
					(out_buffer8[ll*8+5]? 4:0) |
					(out_buffer8[ll*8+6]? 2:0) |
					(out_buffer8[ll*8+7]? 1:0);
			if (width*height%8)
			{
				m = 0;
				for (ll=0; ll<width*height%8; ll++)
					if (out_buffer8[width*height/8*8+ll])
						m |= 128>>ll;
				out_buffer8[width*height/8] = m;
			}
			VWriteData((char *)out_buffer8, 1, (width*height+7)/8, fpout, &ll);
			if(ll != (width*height+7)/8)
        	{
           		printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}

		k++;

	  } /* end for-loop for all slices[i] */
	} /* end for-loop for volumes[j] */

	VCloseData(fpout);

	exit(0);
}
