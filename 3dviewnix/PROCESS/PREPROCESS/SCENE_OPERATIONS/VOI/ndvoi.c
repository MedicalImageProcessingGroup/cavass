/*
  Copyright 1993-2015, 2017, 2020 Medical Image Processing Group
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

/*
	This program performs filtering on an nD Scene (3DViewnix format).

	The type of filter applied is:

		ROI

	Each slice is is loaded and filtered individually.



Author: Roberto J. Goncalves
Date  : 11/02/92

% cc ndvoi.c -o ndvoi  $VIEWNIX_ENV/LIBRARY/lib3dviewnix.a -I$VIEWNIX_ENV/INCLUDE -lX11 -lm

*/


#include <math.h>
#include <cv3dv.h> 
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif
 

#include "slices.c"

void voi_image16(), voi_image8(), roi_image1(), roi_image8(), roi_image16();

int IOI_flag=0;		/* inicates IOI being specified */
int min_value, max_value;	/* grey window */



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
        exit(-1);
    }
 
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        VDeleteBackgroundProcessInformation();
        exit(-1);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        VDeleteBackgroundProcessInformation();
        exit(-1);
    }
 
    fclose(fp);
 
    return(0);
 
}

/*---------------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
int bin_to_grey(bin_buffer, length_bin, grey_buffer, min_value, max_value)
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
 
    return 0;
}
 
 
 
/*---------------------------------------------------------------------------------------*/
/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
int grey_to_bin(grey_buffer, length_grey, bin_buffer, min_value, max_value)
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

	memset(bin_buffer, 0, (length_grey+7)/8);
    bin = bin_buffer;
    grey = grey_buffer;
    for(i=0; i<length_grey; i++)
    {
        bit = i%8;

        /* ONE */
        if( *grey >= min_value  &&  *grey <= max_value)
            *bin += mask[7-bit];
 
        if( (i+1)%8 == 0) 
			bin++;

        grey++;
    }
 
    return 0;
}


/*-------------------------------------------------------------------------*/
/* Modified: 9/26/03 conversion to gray completed by Dewey Odhner.
 */
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
	t8a = (unsigned char *) malloc(w*h+7);
	/* ROI Grey */
	t8b = (unsigned char *) malloc((int)(nw*nh*sizeof(unsigned char)));

	neww = offx+nw;
	newh = offy+nh;

	/* Convert BINARY_in => GREYa */
	bin_to_grey(in, (w*h+7)/8, t8a, 0, 255);

	/* Perform ROI operation (GREYa-> Roi ->GREYb) */
	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
			t8b[l_nw+m] = j<0||j>=h||i<0||i>=w? 0: t8a[j_w+i];
	}
	/* Convert GREYb => BINARY_out */
	grey_to_bin(t8b, nw*nh, (unsigned char *)out, 100, 255);

	free(t8a);
	free(t8b);
}


/*-------------------------------------------------------------------------*/
void roi_image8(in, w, h, offx, offy, nw, nh, out)
unsigned char *in, *out;	/* input and output buffers */
int w, h, /* size of input image (in pixels) */
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
			out[l_nw+m] = j<0||j>=h||i<0||i>=w? 0: in[j_w+i];
	}

}

/*-------------------------------------------------------------------------*/
/* Modified: 1/5/04 values not scaled to 255 by Dewey Odhner. */
void voi_image8(in, w, h, offx, offy, nw, nh, min, max, out)
unsigned char *in, *out;	/* input buffers */
int w, h, /* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
int min, max;	/* lower and upper boundaries of the intensity dynamic range */
{

	register int i,j, l, m;
	int neww, newh;
	int j_w, l_nw;
	int delta;
	int value;


	neww = offx+nw;
	newh = offy+nh;
	delta = max - min;

	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
		{
			value = j<0||j>=h||i<0||i>=w? 0: in[j_w+i];
			if(value > max) out[l_nw+m] = max-min;
			else
			if(value < min) out[l_nw+m] = 0;
			else
			    out[l_nw+m] = value-min;
		}
	}
}

/*-------------------------------------------------------------------------*/
/* Modified: 1/5/04 values not scaled to 255 by Dewey Odhner. */
void voi_image16(in, w, h, offx, offy, nw, nh, min, max, out)
unsigned short *in;	/* input buffers */
int w, h, /* size of input image (in pixels) */
	offx,offy,	/* offset of output image in input image (in pixels) */
	nw,nh;	/* output size (in pixels) */
int min, max;	/* lower and upper boundaries of the intensity dynamic range */
unsigned char *out;	/* output buffer */
{

	register int i,j, l, m;
	int neww, newh;
	int j_w, l_nw;
	int delta;
	int value;
	unsigned short *out16;


	neww = offx+nw;
	newh = offy+nh;
	delta = max - min;
	out16 = (unsigned short *)out;

	for(j=offy, l=0; j<newh; j++, l++)
	{
		j_w = j * w;
		l_nw = l * nw;
		for(i=offx, m=0; i<neww; i++, m++)
		{
			value = j<0||j>=h||i<0||i>=w? 0: in[j_w+i];
			if (max-min > 255)
				out16[l_nw+m] =
					value>max? max-min: value<min? 0: value-min;
			else
				if(value > max) out[l_nw+m] = max-min;
				else
				if(value < min) out[l_nw+m] = 0;
				else
					out[l_nw+m] = value-min;
		}
	}

}

/*-------------------------------------------------------------------------*/
void roi_image16(in, w, h, offx, offy, nw, nh, out)
unsigned short *in, *out;	/* input and output buffers */
int w, h, /* size of input image (in pixels) */
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
			out[l_nw+m] = j<0||j>=h||i<0||i>=w? 0: in[j_w+i];
	}


}

/*-------------------------------------------------------------------------*/
/*    Modified: 10/5/95 slice ranges handled by Dewey Odhner */
/*    Modified: 10/17/95 job_done program run by Dewey Odhner */
/*    Modified: 2/2/96 correct_min_max program run by Dewey Odhner */
/*    Modified: 3/4/96 correct_min_max not run for BIM files by Dewey Odhner */
/*    Modified: 10/31/96 location of scene domain corrected by Dewey Odhner */
/*    Modified: 11/24/98 byte ordering corrected by Dewey Odhner */
/*    Modified: 10/22/01 reverse slice ordering done by Dewey Odhner */
/*    Modified: 1/5/04 values not scaled to 255 by Dewey Odhner. */
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
	int i,j,k,m;		/* general use */
	int error;			/* error code */
	char *comments; 	/* used to modify the header (description field) */


	int offx,offy;		/* offset of new scene */
	int nw, nh;			/* dimensions of new scene */
	short min3, max3,	/* extents of the VOI operation */
		min4, max4;

	unsigned char *in_buffer1, *out_buffer1;
	unsigned char *in_buffer8, *out_buffer8;
	unsigned short *in_buffer16, *out_buffer16;

	int nslices, *slice_list; /* list of slices in a volume */
	int this_slice;
	char *temp_filename;
	float margin;
	int z_margin_flag=0;

	if (argc!=5 && argc!=10 && argc!=12 && argc!=14)
	{
		printf("Usage:\n");
		printf("  ndvoi input output mode [offx offy new_width new_height min max [min3 max3] [min4 max4] | [z]margin]\n");
		printf("where:\n");
		printf("input    : name of input file;\n");
		printf("output   : name of output file;\n");
		printf("mode     : mode of operation (0=foreground, 1=background);\n");
		printf("offx,offy: offset of the origin of the new scene in respect to input scene;\n");
		printf("new_width, new_height: dimensions (in pixels) of new scene(0,0=original dimensions);\n");
		printf("min, max : grey window for the pixels values (0,0 = entire range)\n");
		printf("Optionals:\n");
		printf("min3,max3: min and max slice along the 3rd dimension \n");
		printf("min4,max4: min and max instance along the 4th dimension \n");
		exit(1);
	}
	

	temp_filename = malloc(strlen(argv[2])*2+23);
	if (temp_filename == NULL)
    {
        fprintf(stderr, "ERROR: Out of memory !\n");
        exit(1);
    }
 
    /* Open INPUT File */
    if( (fpin = fopen(argv[1], "rb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open INPUT file !\n");
        exit(1);
    }
 
    /* Get EXECUTION MODE */
    sscanf(argv[3], "%d", &execution_mode);

	/* If in background mode, then place an entry in the BG_PROCESS.COM file */
    if(execution_mode == 1)
        VAddBackgroundProcessInformation("ndinterpolate");

	if (argc > 5)
	{
		/* OUTPUT offset and dimensions */
		offx = atoi(argv[4]);
		offy = atoi(argv[5]);
		nw = atoi(argv[6]);
		nh = atoi(argv[7]);

		/* IOI values */
		min_value = atoi(argv[8]);
		max_value = atoi(argv[9]);
		IOI_flag = 1;
		if(min_value == max_value && min_value == 0)
			IOI_flag = 0;
	}
	else
	{
		if (argv[4][0] == 'z')
			z_margin_flag = 1;
		if (sscanf(argv[4]+z_margin_flag, "%f", &margin) != 1)
		{
			fprintf(stderr, "ERROR: unrecognized value %s\n", argv[4]);
			exit(1);
		}
		IOI_flag = 0;
	}


    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);

    /* Open OUTPUT File */
	if(sl.bits == 1)
		strcpy(temp_filename, argv[2]);
	else
		sprintf(temp_filename, "%s_", argv[2]);
    if( (fpout = fopen(temp_filename, "wb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }

	/* EXTENT of VOI operation */
	if (argc > 5)
	{
		min3 = 0;
		max3 = sl.max_slices-1;
		nslices = max3-min3+1;
	}
	min4 = 0;
	max4 = sl.volumes-1;
	slice_list = NULL;
	if(argc >= 12)
	{
	  if (strchr(argv[10], '/')==NULL && strchr(argv[11], '/')==NULL)
	  {
	    min3 = atoi(argv[10]);
		max3 = atoi(argv[11]);
		nslices = abs(max3-min3)+1;
	  }
	  else
	  {
		char *min_str, *max_str;
		int range_min, range_max;

		slice_list = (int *)malloc(sl.max_slices*sizeof(int));
		if (slice_list == NULL)
   		{
   			fprintf(stderr, "ERROR: Can't allocate slice list.\n");
			if (execution_mode) system("job_done VOI -abort &");
   			exit(1);
   		}
		nslices = 0;
		min_str = argv[10];
		max_str = argv[11];
		do
		{
			if (sscanf(min_str, "%d", &range_min)!=1 ||
					sscanf(max_str, "%d", &range_max)!=1 ||
					range_min>range_max ||
					range_min<nslices || range_max>=sl.max_slices)
   			{
   				fprintf(stderr, "ERROR: Can't parse range.\n");
				if (execution_mode) system("job_done VOI -abort &");
   				exit(1);
   			}
			for (j=range_min; j<=range_max; j++)
				slice_list[nslices++] = j;
			min_str = strchr(min_str, '/');
			max_str = strchr(max_str, '/');
		} while (min_str++ && max_str++);
	  }
	}
	if(argc >= 14)
	{
		min4 = atoi(argv[12]);
		max4 = atoi(argv[13]);
	}

	/* Calculate length of a slice */
	width =  sl.width;
	height =  sl.height;
	length = width * height * sl.bits / 8;
	if(sl.bits == 1)
		length = (width * height + 7) / 8;

	/* Allocate memory */
	if(sl.bits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
			if (execution_mode) system("job_done VOI -abort &");
       		exit(1);
    	}
 
	}
	else
	if(sl.bits == 8)
	{
    	/* create buffer for one grey image */
    	if( (in_buffer8 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
			if (execution_mode) system("job_done VOI -abort &");
       		exit(1);
    	}
	}
	else
	{
    	/* create buffer for one grey image */
    	if( (in_buffer16 = (unsigned short *) malloc(length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
			if (execution_mode) system("job_done VOI -abort &");
       		exit(1);
    	}
	}

	if (argc == 5)
	{
		int min_x, min_y, min_z, max_x, max_y, max_z, this_row, this_col,
		    this_vol;

		min_x = sl.width;
		min_y = sl.height;
		min_z = sl.max_slices;
		max_x = max_y = max_z = -1;

		/* Traverse ALL VOLUMES */
		for (this_vol=min4; this_vol<=max4; this_vol++)
		{
		  /* For each Volume, traverse ALL SLICES */
		  for (this_slice=0; this_slice<sl.slices[this_vol]; this_slice++)
		  {
			/* Seek the appropriate location */
			VLSeek(fpin,
			    sl.slice_index[this_vol][this_slice]*(double)length+hlength);

			/* BINARY */
			if(sl.bits == 1)
			{
				/* Load input slice */
				if(fread(in_buffer1, 1, length, fpin) != length)
		        {
		        	fprintf(stderr,
					    "ERROR: Couldn't read slice #%d of volume #%d.\n",
						this_slice+1, this_vol+1);
					if (execution_mode) system("job_done VOI -abort &");
		        	exit(2);
		        }
				for (this_row=0; this_row<sl.height; this_row++)
					for (this_col=0; this_col<sl.width; this_col++)
						if (in_buffer1[(this_row*sl.width+this_col)/8] &
								(128>>((this_row*sl.width+this_col)%8)))
						{
							if (this_col < min_x)
								min_x = this_col;
							if (this_col > max_x)
								max_x = this_col;
							if (this_row < min_y)
								min_y = this_row;
							if (this_row > max_y)
								max_y = this_row;
							if (this_slice < min_z)
								min_z = this_slice;
							if (this_slice > max_z)
								max_z = this_slice;
						}
			}
			else
			/* 8 Bits/Pixel */
			if(sl.bits == 8)
			{
				if(fread(in_buffer8, 1, length, fpin) != length)
	    		{
	       			fprintf(stderr,
					    "ERROR: Couldn't read slice #%d of volume #%d.\n",
						this_slice+1, this_vol+1);
					if (execution_mode) system("job_done VOI -abort &");
	       			exit(2);
	    		}
				for (this_row=0; this_row<sl.height; this_row++)
					for (this_col=0; this_col<sl.width; this_col++)
						if (in_buffer8[this_row*sl.width+this_col])
						{
							if (this_col < min_x)
								min_x = this_col;
							if (this_col > max_x)
								max_x = this_col;
							if (this_row < min_y)
								min_y = this_row;
							if (this_row > max_y)
								max_y = this_row;
							if (this_slice < min_z)
								min_z = this_slice;
							if (this_slice > max_z)
								max_z = this_slice;
						}
			}
			/* 16 Bits/Pixel */
			else
			{
				/* Load input slice */
				if(VReadData((char *)in_buffer16, 2, length/2, fpin, &m))
	    		{
	       			fprintf(stderr,
					    "ERROR: Couldn't read slice #%d of volume #%d.\n",
						this_slice+1, this_vol+1);
					if (execution_mode) system("job_done VOI -abort &");
	       			exit(2);
	    		}
				for (this_row=0; this_row<sl.height; this_row++)
					for (this_col=0; this_col<sl.width; this_col++)
						if (in_buffer16[this_row*sl.width+this_col])
						{
							if (this_col < min_x)
								min_x = this_col;
							if (this_col > max_x)
								max_x = this_col;
							if (this_row < min_y)
								min_y = this_row;
							if (this_row > max_y)
								max_y = this_row;
							if (this_slice < min_z)
								min_z = this_slice;
							if (this_slice > max_z)
								max_z = this_slice;
						}
			}
		  }
		}
		if (z_margin_flag)
		{
			offx = 0;
			offy = 0;
			nw = sl.width;
			nh = sl.height;
		}
		else
		{
			offx = min_x-(int)ceil(margin/vh.scn.xypixsz[0]);
			offy = min_y-(int)ceil(margin/vh.scn.xypixsz[1]);
			nw = 1-offx+max_x+(int)ceil(margin/vh.scn.xypixsz[0]);
			nh = 1-offy+max_y+(int)ceil(margin/vh.scn.xypixsz[1]);
		}
		min3 = min_z-(int)ceil(margin/sl.max_spacing3[0]);
		max3 = max_z+(int)ceil(margin/sl.max_spacing3[0]);
		nslices = max3-min3+1;
	}

	/* (0,0) = take whatever is left of the image (after applying the offset) */
	if(nw == 0) nw = width - offx;
	if(nh == 0) nh = height - offy;

	if(sl.bits == 1)
		length_out = (nw * nh +7) / 8;
	else
	if(IOI_flag == 0 || max_value-min_value>255)
		length_out = nw * nh * sl.bits / 8;
	else
		length_out = nw * nh;



	/* Allocate memory */
	if(sl.bits == 1)
	{

    	/* create buffer for one grey-level image */
    	if( (out_buffer1 = (unsigned char *) malloc(length_out) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
			if (execution_mode) system("job_done VOI -abort &");
       		exit(1);
    	}
	}
	else
	if(sl.bits == 8)
	{
 
    	/* create buffer for one grey image */
    	if( (out_buffer8 = (unsigned char *) malloc(length_out) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
			if (execution_mode) system("job_done VOI -abort &");
       		exit(1);
    	}
	}
	else
	{
 
		if(IOI_flag == 1)
		{
    		/* create buffer for one grey image */
    		if( (out_buffer8 = (unsigned char *) malloc(length_out) ) == NULL)
    		{
       			fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
				if (execution_mode) system("job_done VOI -abort &");
       			exit(1);
    		}
		}
		else
		{
    		/* create buffer for one grey image */
    		if( (out_buffer16 = (unsigned short *) malloc(length_out) ) == NULL)
    		{
       			fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
				if (execution_mode) system("job_done VOI -abort &");
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
		vh.scn.loc_of_subscenes = (float *) malloc( (nslices*(max4-min4+1)+(max4-min4+1)) * sizeof(float));
		k=0;
		vh.scn.num_of_subscenes[k] = max4-min4+1;
		/* Volume locations */
		for(i=min4; i<=max4; i++, k++)
		{
			vh.scn.num_of_subscenes[k+1] = nslices;
			vh.scn.loc_of_subscenes[k] = sl.location4[i];
		}

		/* Slice locations (for each of the volumes) */
		for(j=min4; j<=max4; j++)
			if (slice_list)
				for (i=0; i<nslices; i++,k++)
					vh.scn.loc_of_subscenes[k]= sl.location3[j][slice_list[i]];
			else
				for(i=min3; i<=max3; i++, k++)
					vh.scn.loc_of_subscenes[k] = i<0?
						sl.location3[j][0]+sl.max_spacing3[j]*i:
						i<sl.slices[j]? sl.location3[j][i]:
						vh.scn.loc_of_subscenes[k-1]+sl.max_spacing3[j];
	}
	else
	{
		vh.scn.num_of_subscenes = (short *)malloc( sizeof(short) );
		vh.scn.num_of_subscenes[0] = nslices;
		vh.scn.loc_of_subscenes = (float *) malloc( nslices * sizeof(float));
		if (slice_list)
			for (i=0; i<nslices; i++)
				vh.scn.loc_of_subscenes[i] = sl.location3[0][slice_list[i]];
		else
		{
			j = max3>=min3? min3: max3;
			for(i=j; i<=max3||i<=min3; i++)
				vh.scn.loc_of_subscenes[i-j] = i<0?
					sl.location3[0][0]+sl.max_spacing3[0]*i:
					i<sl.slices[0]? sl.location3[0][i]:
					vh.scn.loc_of_subscenes[i-j-1]+sl.max_spacing3[0];
		}
	}
	vh.scn.xysize[0] = nw;
	vh.scn.xysize[1] = nh;
	for (j=0; j<sl.sd; j++)
	{
		vh.scn.domain[j] += offx*vh.scn.xypixsz[0]*vh.scn.domain[sl.sd+j];
		vh.scn.domain[j] += offy*vh.scn.xypixsz[1]*vh.scn.domain[2*sl.sd+j];
	}
	vh.scn.domain_valid = 1;
	if(IOI_flag == 1 && sl.bits > 1)
	{
		vh.scn.num_of_bits = max_value-min_value>255? 16:8;
		vh.scn.signed_bits[0] = 0;
		vh.scn.smallest_density_value[0] = 0;
		vh.scn.largest_density_value[0] = (float)(max_value-min_value);
		vh.scn.bit_fields[0] = 0;
		vh.scn.bit_fields[1] = vh.scn.num_of_bits-1;
	}
	/* Get the filenames right (own and parent) */
    strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1)-1);
	vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
	vh.gen.filename1_valid = 1;
    strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename)-1);
	vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
	vh.gen.filename_valid = 1;
    /* Build "description" header entry */
	for (i=j=0; i<argc; i++)
		j += (int)strlen(argv[i]);
	comments = (char *)malloc(j+argc+2);
	strcpy(comments,"");
    for(i=0; i<argc; i++)
    {
        strcat(comments,argv[i]);
        strcat(comments," ");
    }
    vh.scn.description = comments;
    vh.scn.description_valid = 0x1;

	/* Write output 3DViewnix Header */
	error = VWriteHeader(fpout, &vh, group, element);
	if (error && error < 106)
	{
		fprintf(stderr, "ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
		if (execution_mode) system("job_done VOI -abort &");
		exit(error);
	}

	
	/* Traverse ALL VOLUMES */
	k=0;
	for(j=min4; j<=max4; j++)
	{
	  /* For each Volume, traverse ALL SLICES */
	  for(this_slice=0,i=min3; this_slice<nslices;
		  this_slice++, min3>max3?i--:i++, k++)
	  {
		/* Seek the appropriate location */
		if (i>=0 && i<sl.slices[j])
			VLSeek(fpin, sl.slice_index[j][slice_list? slice_list[this_slice]:i]*(double)length + hlength);

		if(execution_mode == 0)
		{
			if(sl.volumes > 1)
				printf("VOIing volume #%d/%d, slice #%d/%d ...\n",j-min4+1,max4-min4+1,this_slice+1,nslices);
			else
				printf("VOIing slice #%d/%d ...\n", this_slice+1, nslices);

			fflush(stdout);
		}


		/* BINARY */
		if(sl.bits == 1)
		{
			/* Load input slice */
			if (i>=0 && i<sl.slices[j])
			{
				if(fread(in_buffer1, 1, length, fpin) != length)
	        	{
	           		fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", this_slice+1, j+1);
					if (execution_mode) system("job_done VOI -abort &");
	           		exit(2);
	        	}
			}
			else
				memset(in_buffer1, 0, length);

			/* ROI Only */
			roi_image1(in_buffer1, width, height, offx, offy, nw, nh, out_buffer1);

			/* Save output slice */
        	if(fwrite(out_buffer1, 1, length_out, fpout) != length_out)
        	{
           		fprintf(stderr, "ERROR: Couldn't write slice #%d of volume #%d.\n", this_slice+1, j+1);
				if (execution_mode) system("job_done VOI -abort &");
           		exit(3);
        	}
		}
		else
		/* 8 Bits/Pixel */
		if(sl.bits == 8)
		{
			if (i>=0 && i<sl.slices[j])
			{
				if(fread(in_buffer8, 1, length, fpin) != length)
        		{
           			fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", this_slice+1, j+1);
					if (execution_mode) system("job_done VOI -abort &");
           			exit(2);
        		}
			}
			else
				memset(in_buffer8, 0, length);

			/* ROI Only */
			if(IOI_flag == 0)
				roi_image8(in_buffer8, width, height, offx, offy, nw, nh, out_buffer8);
			/* VOI */
			else
				voi_image8(in_buffer8, width, height, offx, offy, nw, nh, min_value, max_value, out_buffer8);

			/* Save output slice */
        	if(fwrite(out_buffer8, 1, length_out, fpout) != length_out)
        	{
           		fprintf(stderr, "ERROR: Couldn't write slice #%d of volume #%d.\n", this_slice+1, j+1);
           		exit(3);
				if (execution_mode) system("job_done VOI -abort &");
        	}
		}
		/* 16 Bits/Pixel */
		else
		{
			/* Load input slice */
			if (i>=0 && i<sl.slices[j])
			{
				if(VReadData((char *)in_buffer16, 2, length/2, fpin, &m))
        		{
           			fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", this_slice+1, j+1);
					if (execution_mode) system("job_done VOI -abort &");
           			exit(2);
        		}
			}
			else
				memset(in_buffer16, 0, length);

			/* ROI Only */
			if(IOI_flag == 0)
			{
				roi_image16(in_buffer16, width, height, offx, offy, nw, nh, out_buffer16);
				/* Save output slice */
        		if(VWriteData((char *)out_buffer16, 2, length_out/2, fpout, &m))
        		{
           			fprintf(stderr, "ERROR: Couldn't write slice #%d of volume #%d.\n", this_slice+1, j+1);
					if (execution_mode) system("job_done VOI -abort &");
           			exit(3);
        		}
			}
			/* VOI */
			else
			{
				voi_image16(in_buffer16, width, height, offx, offy, nw, nh, min_value, max_value, out_buffer8);
				/* Save output slice */
        		if(max_value-min_value>255?
					VWriteData((char *)out_buffer8, 2, length_out/2, fpout, &m):
					fwrite(out_buffer8, 1, length_out, fpout) != length_out)
        		{
           			fprintf(stderr, "ERROR: Couldn't write slice #%d of volume #%d.\n", this_slice+1, j+1);
					if (execution_mode) system("job_done VOI -abort &");
           			exit(3);
        		}
			}

		}


	  }
	}
	VCloseData(fpout);
	if(sl.bits != 1)
	{
		sprintf(temp_filename, "correct_min_max \"%s_\" \"%s\"", argv[2], argv[2]);
		if (system(temp_filename))
    	{
        	fprintf(stderr, "ERROR: correct_min_max failed!\n");
			if (execution_mode) system("job_done VOI -abort &");
        	exit(1);
		}
		sprintf(temp_filename, "%s_", argv[2]);
		unlink(temp_filename);
    }


	if(execution_mode == 0)
	{
		printf("Done.\n");
		fflush(stdout);
	}

 
	/* If in Background mode remove the entry from the BG_PROCESS.COM */
    if(execution_mode == 1)
    {
		VDeleteBackgroundProcessInformation();
		system("job_done VOI &");
	}

	exit(0);
}
