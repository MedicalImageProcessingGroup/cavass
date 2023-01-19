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
	This program performs filtering on an nD Scene (3DViewnix format).

	The type of filter applied is:

		ROI

	Each slice is is loaded and filtered individually.



Author: Roberto J. Goncalves
Date  : 11/02/92

% cc ndclass.c -o ndclass  $VIEWNIX_ENV/LIBRARY/lib3dviewnix.a -I$VIEWNIX_ENV/INCLUDE -lX11 -lm

*/


#include <math.h>
 
#include <cv3dv.h>
 
 

#include "slices.c"

int IOI_flag=0;		/* inicates IOI being specified */
int min_value, max_value, int1_value, int2_value;	/* grey window */



unsigned char mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

 
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

/*---------------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
void bin_to_grey(bin_buffer, length_bin, grey_buffer, min_value, max_value)
unsigned char *bin_buffer;
int length_bin;
unsigned char *grey_buffer;
int min_value, max_value;
{
        register int i, j;
        unsigned char *bin, *grey;
		/* GLOBAL */
		/*unsigned char mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};*/
 
        bin = bin_buffer;
        grey = grey_buffer;
 
        for(i=length_bin; i>0; i--, bin++)
        {
           for(j=7; j>=0; j--, grey++)
           {
                if( (*bin & mask[j]) > 0) 
					*grey = max_value;
                else 
					*grey = min_value;
 
           }
        }
 
 
}
 
 
 
/*---------------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
void grey_to_bin(grey_buffer, length_grey, bin_buffer, min_value, max_value)
unsigned char *grey_buffer;
int length_grey;
unsigned char *bin_buffer;
int min_value, max_value;
{
    register int i;
    unsigned char *bin, *grey;
    int bit;
	/* GLOBAL */
	/*unsigned char mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};*/
 
    bin = bin_buffer;
    grey = grey_buffer;
	*bin = 0;
    for(i=0; i<length_grey; i++)
    {
        bit = i%8;

        /* ONE */
        if( *grey >= min_value  &&  *grey <= max_value)
            *bin += mask[7-bit];
 
        if( (i+1)%8 == 0) 
		{
			bin++;
			*bin = 0;
		}

        grey++;
    }
 
 
}


/*-------------------------------------------------------------------------*/
void roi_image1(in, w, h, offx, offy, nw, nh, out)
unsigned char *in, *out;	/* input and output buffers */
int w,h,	/* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
{

	register int i,j, l, m;
	int neww, newh;
	int j_w, l_nw;
	unsigned char *t8a, *t8b;


	
	/* Full Grey */
printf("in...%d\n", (int)(w*h*sizeof(unsigned char)));
	t8a = (unsigned char *) malloc(w*h);
printf("...\n");
	/* ROI Grey */
	t8b = (unsigned char *) malloc((int)(nw*nh*sizeof(unsigned char)));
printf("out...\n");

	neww = offx+nw;
	newh = offy+nh;

	/* Convert BINARY_in => GREYa */
	bin_to_grey(in, (w*h/8), t8a, 0, 255);

	/* Perform ROI operation (GREYa-> Roi ->GREYb) */
	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
			t8b[l_nw+m] = t8a[j_w+i];
	}
	/* Convert GREYb => BINARY_out */
	grey_to_bin(t8b, nw*nh, (unsigned char *)out, 100, 255);

	free(t8a);
	free(t8b);
}


/*-------------------------------------------------------------------------*/
void roi_image8(in, w, offx, offy, nw, nh, out)
unsigned char *in, *out;	/* input and output buffers */
int w,	/* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
{

	register int i,j, l, m;
	int neww, newh;
	int j_w, l_nw;


	neww = offx+nw;
	newh = offy+nh;

	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
			out[l_nw+m] = in[j_w+i];
	}


}

/*-------------------------------------------------------------------------*/
/*    Modified: 6/1/95 upper value taken if min == int1 by Dewey Odhner.
 */
void voi_image8(in, w, offx, offy, nw, nh, min, int1, int2, max, out)
unsigned char *in, *out;	/* input buffers */
int w,	/* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
int min, max, int1, int2;	/* lower and upper boundaries of the intensity dynamic range */
{
	register int i,j, l, m;
	int neww, newh;
	int j_w, l_nw;

	neww = offx+nw;
	newh = offy+nh;

	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		
		for(i=offx, m=0; i<neww; i++, m++)
			if (in[j_w+i] > max)
				out[l_nw+m] = 0;
			else if (in[j_w+i] > int2) 
				out[l_nw+m] = 255 * (max - in[j_w+i]) / (max - int2);
			else if (in[j_w+i] >= int1) 
				out[l_nw+m] = 255;
			else if (in[j_w+i] >= min) 
				out[l_nw+m] = 255 * (in[j_w+i] - min) / (int1 - min);
			else
				out[l_nw+m] = 0;
	}
}

/*-------------------------------------------------------------------------*/
/*    Modified: 6/1/95 upper value taken if min == int1 by Dewey Odhner.
 */
void voi_image16(in, w, offx, offy, nw, nh, min, int1, int2, max, out)
unsigned short *in;	/* input buffers */
int w,	/* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
int min, max, int1, int2;	/* lower and upper boundaries of the intensity dynamic range */
unsigned char *out;	/* output buffer */
{
	register int i,j, l, m;
	int neww, newh;
	int j_w, l_nw;

	neww = offx+nw;
	newh = offy+nh;

	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
			if (in[j_w+i] > max)
				out[l_nw+m] = 0;
			else if (in[j_w+i] > int2) 
				out[l_nw+m] = 255 * (max - in[j_w+i]) / (max - int2);
			else if (in[j_w+i] >= int1) 
				out[l_nw+m] = 255;
			else if (in[j_w+i] >= min) 
				out[l_nw+m] = 255 * (in[j_w+i] - min) / (int1 - min);
			else
				out[l_nw+m] = 0;
	}

}

/*-------------------------------------------------------------------------*/
void roi_image16(in, w, offx, offy, nw, nh, out)
unsigned short *in, *out;	/* input and output buffers */
int w,	/* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
{

	register int i,j, l,m;
	int neww, newh;
	int j_w, l_nw;


	neww = offx+nw;
	newh = offy+nh;

	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
			out[l_nw+m] = in[j_w+i];
	}


}

/*-------------------------------------------------------------------------*/
/*
 *    Modified: 5/31/95 command line slice, volume numbering from 1
 *       by Dewey Odhner.
 *    Modified: 10/6/95 byte ordering done for 16-bit data by Dewey Odhner.
 */
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
	int length_out;		/* length of output slice in bytes */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int i,j,k;			/* general use */
	int error;			/* error code */
	char comments[500];	/* used to modify the header (description field) */


	int offx,offy;		/* offset of new scene */
	int nw, nh;			/* dimensions of new scene */
	short min3, max3,	/* extents of the VOI operation */
		min4, max4;

	unsigned char *in_buffer1, *out_buffer1;
	unsigned char *in_buffer8, *out_buffer8;
	unsigned short *in_buffer16, *out_buffer16;


	if(argc<8)
	{
		printf("Usage:\n");
		printf("%% ndclass input output mode offx offy new_width new_height min int1 int2 max [min3 max3] [min4 max4]\n");
		printf("where:\n");
		printf("input    : name of input file;\n");
		printf("output   : name of output file;\n");
		printf("mode     : mode of operation (0=foreground, 1=background);\n");
		printf("offx,offy: offset of the origin of the new scene in respect to input scene;\n");
		printf("new_width, new_height: dimensions (in pixels) of new scene(0,0=original dimensions);\n");
		printf("min, int1, int2, max : grey window for the pixels values \n");
		printf("Optionals:\n");
		printf("min3,max3: min and max slice along the 3rd dimension \n");
		printf("min4,max4: min and max instance along the 4th dimension \n");
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

	/* OUTPUT offset and dimensions */
	offx = atoi(argv[4]);
	offy = atoi(argv[5]);
	nw = atoi(argv[6]);
	nh = atoi(argv[7]);

	/* IOI values */
	min_value = atoi(argv[8]);
	int1_value = atoi(argv[9]);
	int2_value = atoi(argv[10]);
	max_value = atoi(argv[11]);
	if(min_value || max_value)
		IOI_flag = 1;


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*----------VOIing-------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);

	/* EXTENT of VOI operation */
	min3 = 0;
	max3 = sl.max_slices-1;
	min4 = 0;
	max4 = sl.volumes-1;
	if(argc >= 14)
	{
		min3 = atoi(argv[12])-1;
		max3 = atoi(argv[13])-1;
		if (max3 > sl.max_slices-1)
			max3 = sl.max_slices-1;
	}
	if(argc >= 16)
	{
		min4 = atoi(argv[14])-1;
		max4 = atoi(argv[15])-1;
	}

	/* Calculate length of a slice */
	width =  sl.width;
	height =  sl.height;
	length = width * height * sl.bits / 8;
	if(sl.bits == 1)
		length = (width * height + 7) / 8;

	/* (0,0) = take whatever is left of the image (after applying the offset) */
	if(nw == 0) nw = width - offx;
	if(nh == 0) nh = height - offy;

	if(sl.bits == 1)
		length_out = (nw * nh +7) / 8;
	else
	if(IOI_flag == 0)
		length_out = nw * nh * sl.bits / 8;
	else
		length_out = nw * nh;



	/* Allocate memory */
	if(sl.bits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 

    	/* create buffer for one grey-level image */
    	if( (out_buffer1 = (unsigned char *) malloc(length_out) ) == NULL)
    	{
       		printf("ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	else
	if(sl.bits == 8)
	{
    	/* create buffer for one grey image */
    	if( (in_buffer8 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer8 = (unsigned char *) malloc(length_out) ) == NULL)
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
 
		if(IOI_flag == 1)
		{
    		/* create buffer for one grey image */
    		if( (out_buffer8 = (unsigned char *) malloc(length_out) ) == NULL)
    		{
       			printf("ERROR: Can't allocate output image buffer.\n");
       			exit(1);
    		}
		}
		else
		{
    		/* create buffer for one grey image */
    		if( (out_buffer16 = (unsigned short *) malloc(length_out) ) == NULL)
    		{
       			printf("ERROR: Can't allocate output image buffer.\n");
       			exit(1);
    		}
		}
	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
	free(vh.scn.num_of_subscenes);
	free(vh.scn.loc_of_subscenes);
	if(sl.sd == 4)
	{
		vh.scn.num_of_subscenes = (short *)malloc( (1+max4-min4+1)*sizeof(short) );
		vh.scn.num_of_subscenes[0] = max4-min4+1;
		vh.scn.loc_of_subscenes = (float *) malloc( ((max3-min3+1)*(max4-min4+1)+(max4-min4+1)) * sizeof(float));
		k=0;
		vh.scn.num_of_subscenes[k] = max4-min4+1;
		/* Volume locations */
		for(i=min4; i<=max4; i++, k++)
		{
			vh.scn.num_of_subscenes[k+1] = max3-min3+1;
			vh.scn.loc_of_subscenes[k] = sl.location4[i];
		}

		/* Slice locations (for each of the volumes) */
		for(j=min4; j<=max4; j++)
			for(i=min3; i<=max3; i++, k++)
				vh.scn.loc_of_subscenes[k] = sl.location3[j][i];
	}
	else
	{
		vh.scn.num_of_subscenes = (short *)malloc( sizeof(short) );
		vh.scn.num_of_subscenes[0] = max3 - min3 + 1;
		vh.scn.loc_of_subscenes = (float *) malloc( (max3-min3+1) * sizeof(float));
		for(i=min3; i<=max3; i++)
			vh.scn.loc_of_subscenes[i-min3] = sl.location3[0][i];
	}
	vh.scn.xysize[0] = nw;
	vh.scn.xysize[1] = nh;
	vh.scn.domain[0] = vh.scn.domain[0] + offx * vh.scn.xypixsz[0];
	vh.scn.domain[1] = vh.scn.domain[1] + offy * vh.scn.xypixsz[1];
	vh.scn.domain[2] = sl.location3[0][min3];
	if(IOI_flag == 1 && sl.bits > 1)
	{
		vh.scn.num_of_bits = 8;
		vh.scn.signed_bits[0] = 0;
		vh.scn.smallest_density_value[0] = 0;
		vh.scn.largest_density_value[0] = 255;
		vh.scn.bit_fields[0] = 0;
		vh.scn.bit_fields[1] = 7;
	}
	/* Get the filenames right (own and parent) */
    strcpy(vh.gen.filename1, argv[1]);
    strcpy(vh.gen.filename, argv[2]);
    /* Build "description" header entry */
	strcpy(comments,"");
    for(i=0; i<argc; i++)
    {
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
	/*for(j=0; j<sl.volumes; j++)*/
	for(j=min4; j<=max4; j++)
	{
	  /* For each Volume, traverse ALL SLICES */
	  /*for(i=0; i<sl.slices[j]; i++, k++)*/
	  for(i=min3; i<=max3; i++, k++)
	  {
		/* Seek the appropriate location */
		/*fseek(fpin, k*length+hlength, 0);*/
		fseek(fpin, sl.slice_index[j][i]*length + hlength, 0);

		if(execution_mode == 0)
		{
			if(sl.volumes > 1)
				printf("Classifying volume #%d/%d, slice #%d/%d ...\n",j-min4+1,max4-min4+1,i-min3+1,max3-min3+1);
			else
				printf("Classifying slice #%d/%d ...\n", i-min3+1, max3-min3+1);

			fflush(stdout);
		}


		/* BINARY */
		if(sl.bits == 1)
		{
			/* Load input slice */
			if(fread(in_buffer1, 1, length, fpin) != length)
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

			/* ROI Only */
			roi_image1(in_buffer1, width, height, offx, offy, nw, nh, out_buffer1);

			/* Save output slice */
        	if(fwrite(out_buffer1, 1, length_out, fpout) != length_out)
        	{
           		printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}
		else
		/* 8 Bits/Pixel */
		if(sl.bits == 8)
		{
			if(fread(in_buffer8, 1, length, fpin) != length)
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

			/* ROI Only */
			if(IOI_flag == 0)
				roi_image8(in_buffer8, width, offx, offy, nw, nh, out_buffer8);
			/* VOI */
			else
				voi_image8(in_buffer8, width, offx, offy, nw, nh, min_value, int1_value, int2_value, max_value, out_buffer8);

			/* Save output slice */
        	if(fwrite(out_buffer8, 1, length_out, fpout) != length_out)
        	{
           		printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}
		/* 16 Bits/Pixel */
		else
		{
			int items;

			/* Load input slice */
			if(VReadData((char *)in_buffer16, 2, length/2, fpin, &items))
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

			/* ROI Only */
			if(IOI_flag == 0)
			{
				roi_image16(in_buffer16, width, offx, offy, nw, nh, out_buffer16);
				/* Save output slice */
        		if(VWriteData((char *)out_buffer16, 2, length_out/2, fpout, &items))
        		{
           			printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           			exit(3);
        		}
			}
			/* VOI */
			else
			{
				voi_image16(in_buffer16, width, offx, offy, nw, nh, min_value, int1_value, int2_value, max_value, out_buffer8);
				/* Save output slice */
        		if(fwrite(out_buffer8, 1, length_out, fpout) != length_out)
        		{
           			printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           			exit(3);
        		}
			}

		}


	  }
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
