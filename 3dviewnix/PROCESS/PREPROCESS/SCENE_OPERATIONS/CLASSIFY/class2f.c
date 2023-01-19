/*
  Copyright 1993-2015, 2017 Medical Image Processing Group
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



	Each slice is is loaded and filtered individually.



Author: Roberto J. Goncalves
Date  : 11/02/92

% cc classify_2feat.c -o class2f -I/usr/chromos2_3/3dviewnix/INCLUDE $VIEWNIX_ENV/LIBRARY/3dviewnix.a -lX11 -lm

*/


#include <math.h>
 
#include <cv3dv.h>
 
 

#include "slices.c"


void load_histogram(char histfile[], double ***hist_count, int bin_size[2],
	int nbins[2], int *min1, int *min2);
int Clas_2d88(unsigned char *orig, int min_orig, int max_orig, unsigned char *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned char *Class_2d);
int Clas_2d816(unsigned char *orig, int min_orig, int max_orig, unsigned short *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned char *Class_2d);
int Clas_2d168(unsigned short *orig, int min_orig, int max_orig, unsigned char *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned short *Class_2d);
int Clas_2d1616(unsigned short *orig, int min_orig, int max_orig, unsigned short *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned short *Class_2d);
int Clas_2Dhist88(unsigned char *orig, int min1, unsigned char *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned char *Class_2d);
int Clas_2Dhist816(unsigned char *orig, int min1, unsigned short *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned char *Class_2d);
int Clas_2Dhist168(unsigned short *orig, int min1, unsigned char *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned short *Class_2d);
int Clas_2Dhist1616(unsigned short *orig, int min1, unsigned short *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned short *Class_2d);



 
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
    if ((fp=fopen(file,"r"))==NULL)
    {
        fprintf(stderr,"ERROR: Can't open file [%s] !\n", file);
        VDeleteBackgroundProcessInformation();
        exit(1);
    }
 
 
    error = VReadHeader(fp, vh, group, element);
    if( error>0 && error < 106)
    {
        fprintf(stderr,"ERROR #%d: Inconsistent 3DVIEWNIX header on file [%s] !\n", error, file);
        fprintf(stderr,"group=%s,  element=%s\n", group, element);
        VDeleteBackgroundProcessInformation();
        exit(1);
    }
 
    error = VGetHeaderLength(fp, len);
    if( error>0 )
    {
        fprintf(stderr,"ERROR #%d: Can't get header length on file [%s] !\n", error, file);
        VDeleteBackgroundProcessInformation();
        exit(1);
    }
 
    fclose(fp);
 
    return(0);
 
}
 

int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin1, *fpin2, *fpout;	/* inpput/output files */
	int execution_mode;	/* execution mode */
	ViewnixHeader vh, vh2;	/* 3DViewnix header */
	SLICES	sl, sl2;	/* Structure containing information about the slices of the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length, length2;		      /* length of a slice */
	int hlength, hlength2;		      /* length of the input header */
	int width, height, width2, height2;   /* dimensions of a slice */
	int i,j,k;			/* general use */
	int error;			/* error code */
	char *comments;	/* used to modify the header (description field) */
	int ll1, ll2;

	unsigned char *out_buffer8,
			*in_buffer1_8, *in_buffer2_8;
	unsigned short *in_buffer1_16, *in_buffer2_16, *out_buffer16;

	int min1, int11, int12, max1, min2, int21, int22, max2,
		min_inp1, min_inp2, max_inp1, max_inp2, nbins[2], bin_size[2];

	double **hist_count=NULL, hist_min, hist_max;

	if (argc!=13 && argc!=8)
	{
		printf("Usage:\n");
		printf(" class2f input1 input2 output mode [min1 int11 int12 max1 min2 int21 int22 max2 | histfile minhist maxhist]\n");
		printf("where:\n");

		printf("input1    : name of input file 1;\n");
		printf("input2    : name of input file 2;\n");
		printf("output   : name of output file;\n");
		printf("mode     : mode of operation (0=foreground, 1=background);\n");
		printf("min1, int11, int12, max1: intervals for feature 1;\n");
		printf("min2, int21, int22, max2: intervals for feature 2;\n");
		printf("histfile : name of histogram file;\n");
		printf("minhist, maxhist: interval for histogram feature.\n");
		exit(1);
	}
	


	 
 
    /* Open INPUT and OUTPUT Files */
    if( (fpin1 = fopen(argv[1], "r")) == NULL)
    {
        printf("ERROR: Can't open FEATURE 1 file !\n");
        exit(1);
    }

    if( (fpin2 = fopen(argv[2], "r")) == NULL)
    {
        printf("ERROR: Can't open FEATURE 2 file !\n");
        exit(1);
    }

    if( (fpout = fopen(argv[3], "w")) == NULL)
    {
        printf("ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }
 
    /* Get EXECUTION MODE */
    sscanf(argv[4], "%d", &execution_mode);

	if (argc == 13)
	{
		/* Get SIGMA */
		sscanf(argv[5], "%d", &min1);

		sscanf(argv[6], "%d", &int11);

		sscanf(argv[7], "%d", &int12);

		sscanf(argv[8], "%d", &max1);


		sscanf(argv[9], "%d", &min2);

		sscanf(argv[10], "%d", &int21);

		sscanf(argv[11], "%d", &int22);

		sscanf(argv[12], "%d", &max2);
	}
	else
	{
		load_histogram(argv[5], &hist_count, bin_size, nbins, &min1, &min2);
		sscanf(argv[6], "%lf", &hist_min);
		sscanf(argv[7], "%lf", &hist_max);
	}


	/* Get optional parameter */
	/* if(argc == 5) filt3d_flag = 1;  */


/* If in background mode, then place an entry in the BG_PROCESS.COM file */

    if(execution_mode == 1)
        VAddBackgroundProcessInformation("classify_2feature");


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
    get_file_info(argv[2], &vh2, &hlength2);

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);
	compute_slices(&vh2, &sl2);

	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	length = width * height * vh.scn.num_of_bits / 8;

	width2 =  vh2.scn.xysize[0];
	height2 =  vh2.scn.xysize[1];
	length2 = width2 * height2 * vh2.scn.num_of_bits / 8;

/*
	bit1 =  vh.scn.num_of_bits;
	bit2 =  vh2.scn.num_of_bits;
*/

	min_inp1 = (int)vh.scn.smallest_density_value[0];
	max_inp1 = (int)vh.scn.largest_density_value[0];

	min_inp2 = (int)vh2.scn.smallest_density_value[0];
	max_inp2 = (int)vh2.scn.largest_density_value[0];

/* printf("%d\t%d\t%d\t%d\t", min_inp1, max_inp1, min_inp2, max_inp2); */

	if(vh.scn.num_of_bits == 8)
	{
    	   /* create buffer for one grey image */
    	   if( (in_buffer1_8 = (unsigned char *) malloc(length) ) == NULL)
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

	if(vh2.scn.num_of_bits == 8) 
	{

	   if( (in_buffer2_8 = (unsigned char *) malloc(length) ) == NULL)
    	   {
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	   }
	}

	if(vh.scn.num_of_bits == 16)

	{
    	/* create buffer for one grey image */
    	if( (in_buffer1_16 = (unsigned short *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}

	
	if( (in_buffer2_16 = (unsigned short *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
 	}

	if(vh2.scn.num_of_bits == 16) {
    	
	if( (out_buffer16 = (unsigned short *) malloc(length) ) == NULL)
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
	
	printf("ERROR: Binary files cannot be processed");
	exit(1);

	}
	/* Get the filenames right (own and parent) */

    strcpy(vh.gen.filename1, argv[1]);
    strcpy(vh.gen.filename, argv[3]);

    /* Build "description" header entry */
    for (i=j=0; i<argc; i++)
		j += (int)strlen(argv[i]);
	comments = (char *) malloc(j + argc + 1);
	comments[0] = 0;
    for(i=0; i<argc; i++)
    {
        strcat(comments,argv[i]);
        strcat(comments," ");
    }
    vh.scn.description = comments;
    vh.scn.description_valid = 0x1;
	if (vh.scn.smallest_density_value_valid==0 ||
	    vh.scn.largest_density_value_valid==0)
	{
		vh.scn.smallest_density_value = (float *)malloc(sizeof(float));
		vh.scn.largest_density_value = (float *)malloc(sizeof(float));
	}
	vh.scn.smallest_density_value[0] = 0;
	vh.scn.largest_density_value[0] = 65534;
	vh.scn.smallest_density_value_valid = 1;
	vh.scn.largest_density_value_valid = 1;

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
		
		fseek(fpin1, k*length+hlength, 0);
		fseek(fpin2, k*length2+hlength2, 0);

		if(execution_mode == 0)
		{
			if(sl.volumes > 1)
			printf("Classifying of volume #%d/%d, slice #%d/%d ...\n", j+1,sl.volumes,i+1, sl.slices[j]);
			else
			printf("Classifying slice #%d/%d ...\n", i+1, sl.slices[j]);

			fflush(stdout);
		}


		/* 1 or 8 Bits/Pixel */
		if(vh.scn.num_of_bits == 8)
		{
			/* Load input slice */
			

			if(fread(in_buffer1_8, 1, length, fpin1) != length)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}

		
		    if(vh2.scn.num_of_bits == 8) {

			if(fread(in_buffer2_8, 1, length2, fpin2) != length2)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}

		   if (hist_count)
		       Clas_2Dhist88(in_buffer1_8, min1, in_buffer2_8, min2, height, width, hist_count, bin_size, nbins, hist_min, hist_max, out_buffer8);
		   else
		       Clas_2d88(in_buffer1_8, min_inp1, max_inp1, in_buffer2_8, min_inp2, max_inp2, height, width, min1, int11, int12, max1, min2, int21, int22, max2, out_buffer8);

		    }

		    else {
			
			if(fread(in_buffer2_16, 1, length2, fpin2) != length2)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}

		  if (hist_count)
		      Clas_2Dhist816(in_buffer1_8, min1, in_buffer2_16, min2, height, width, hist_count, bin_size, nbins, hist_min, hist_max, out_buffer8);
		  else
		      Clas_2d816(in_buffer1_8, min_inp1, max_inp1, in_buffer2_16, min_inp2, max_inp2, height, width, min1, int11, int12, max1, min2, int21, int22, max2, out_buffer8);

		    }


			/* Filter */
	
		
			/* Save output slice */
        	if(fwrite(out_buffer8, 1, length, fpout) != length)
        	{
           		printf("ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}
		/* 16 Bits/Pixel */
		else
		{
			/* Load input slice */
			/*if(fread(in_buffer16, 1, length, fpin) != length)*/

			VReadData((char *)in_buffer1_16, 2, (length/2), fpin1, &ll1);
			
			if( ll1 != (length/2) )
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

		
		if(vh2.scn.num_of_bits == 8) {

			if(fread(in_buffer2_8, 1, length2, fpin2) != length2)
        		{
           			printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}



			/* Filter */
			

			if (hist_count)
			    Clas_2Dhist168(in_buffer1_16, min1, in_buffer2_8, min2, height, width, hist_count, bin_size, nbins, hist_min, hist_max, out_buffer16);
			else
			    Clas_2d168(in_buffer1_16, min_inp1, max_inp1, in_buffer2_8, min_inp2, max_inp2, height, width, min1, int11, int12, max1, min2, int21, int22, max2, out_buffer16);

		}

		else {

			VReadData((char *)in_buffer2_16, 2, (length2/2), fpin2, &ll2);
			
			if( ll2 != (length2/2) )
        	{
           		printf("ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}

		if (hist_count)
		    Clas_2Dhist1616(in_buffer1_16, min1, in_buffer2_16, min2, height, width, hist_count, bin_size, nbins, hist_min, hist_max, out_buffer16);
		else
		    Clas_2d1616(in_buffer1_16, min_inp1, max_inp1, in_buffer2_16, min_inp2, max_inp2, height, width, min1, int11, int12, max1, min2, int21, int22, max2, out_buffer16);


		}

			/* Save output slice */

			VWriteData((char *)out_buffer16, 2, (length/2), fpout, &ll1);
			if(ll1 != (length/2) )
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


void load_histogram(char histfile[], double ***hist_count, int bin_size[2],
	int nbins[2], int *min1, int *min2)
{
	FILE *fp;
	int j, k;

	fp = fopen(histfile, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open file %s\n", histfile);
		exit(1);
	}
	if (fscanf(fp, "Histogram\nbin_size=%dx%d nbins=%dx%d min=%d,%d\n",
			bin_size, bin_size+1, nbins, nbins+1, min1, min2) != 6)
	{
		fprintf(stderr, "Cannot read histogram.\n");
		exit(1);
	}
	*hist_count = (double **)malloc(nbins[0]*sizeof(double *));
	for (j=0; j<nbins[0]; j++)
	{
		(*hist_count)[j] = (double *)malloc(nbins[1]*sizeof(double));
		if ((*hist_count)[j] == NULL)
		{
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
		for (k=0; k<nbins[1]; k++)
			if (fscanf(fp, " %lf\n", (*hist_count)[j]+k) != 1)
			{
				fprintf(stderr, "Cannot read histogram.\n");
				exit(1);
			}
	}
}

/*------------------------------------------------------------------------*/

/*    Modified: 3/2/95 multiplication by orig[i] removed by Dewey Odhner.
 */
int Clas_2d1616(unsigned short *orig, int min_orig, int max_orig, unsigned short *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned short *Class_2d)
{

	unsigned int i, MaxValue, Hheight,
			Hwidth;
	float	value,value2;

	float		f1, f2;

		
	float min_thresh = 0.0;
	float max_thresh = 1.0;

	float xkj1, ykj1, zkj1, xlj1, ylj1, zlj1, 
	      xkj2, ykj2, zkj2, xlj2, ylj2, zlj2,
	      xkj3, ykj3, zkj3, xlj3, ylj3, zlj3,
              xkj4, ykj4, zkj4, xlj4, ylj4, zlj4;
	
	float a1, b1, c1, d1,
	      a2, b2, c2, d2,
	      a3, b3, c3, d3,
	      a4, b4, c4, d4;

	float slop1, slop2, slop3, slop4;

	int eq1, eq2;

	double grey_x;

	Hheight = ht;
	Hwidth = wh;

	MaxValue = Hwidth * Hheight;


	for (i=0; i < MaxValue; i++) Class_2d[i] = 0; 


	f1 = (float)(1.0 / (max_orig - min_orig) * Hheight * 0.7);
	f2 = (float)(1.0 / (max_orig2 - min_orig2) * Hwidth * 0.7);

if ((min != 0 || int1 != 0) || (int2 != 0 || max != 0)) 
	  if ((min_g != 0 || int1_g != 0) || (int2_g != 0 || max_g != 0))
	
	{

	
	xkj1 = (float)(max - min);
	ykj1 = 0;
	zkj1 = 0;

	xlj1 = (float)(int1 - min);
	ylj1 = (float)(int1_g - min_g);
	zlj1 = max_thresh - min_thresh;


	xkj2 = 0;
	ykj2 = (float)(max_g - min_g);
	zkj2 = 0;

	xlj2 = (float)(int2 - max);
	ylj2 = (float)(int1_g - min_g);
	zlj2 = max_thresh - min_thresh;

	
	xkj3 = (float)(min - max);
	ykj3 = 0;
	zkj3 = 0;

	xlj3 = (float)(int2 - max);
	ylj3 = (float)(int2_g - max_g);
	zlj3 = max_thresh - min_thresh;


	xkj4 = 0;
	ykj4 = (float)(min_g - max_g);
	zkj4 = 0;

	xlj4 = (float)(int1 - min);
	ylj4 = (float)(int2_g - max_g);
	zlj4 = max_thresh - min_thresh;

	a1 = ykj1 * zlj1 - zkj1 * ylj1;
	b1 = zkj1 * xlj1 - xkj1 * zlj1;
	c1 = xkj1 * ylj1 - ykj1 * xlj1;
	d1 = -1 * (max * a1 + min_g * b1 + 0 * c1);

	a2 = ykj2 * zlj2 - zkj2 * ylj2;
	b2 = zkj2 * xlj2 - xkj2 * zlj2;
	c2 = xkj2 * ylj2 - ykj2 * xlj2;
	d2 = -1 * (max * a2 + max_g * b2 + 0 * c2);
	
	a3 = ykj3 * zlj3 - zkj3 * ylj3;
	b3 = zkj3 * xlj3 - xkj3 * zlj3;
	c3 = xkj3 * ylj3 - ykj3 * xlj3;
	d3 = -1 * (min * a3 + max_g * b3 + 0 * c3);

	a4 = ykj4 * zlj4 - zkj4 * ylj4;
	b4 = zkj4 * xlj4 - xkj4 * zlj4;
	c4 = xkj4 * ylj4 - ykj4 * xlj4;
	d4 = -1 * (min * a4 + min_g * b4 + 0 * c4);


	slop1 = (float)((int1_g - min_g) / (int1 - min));
	slop2 = (float)((int1_g - min_g) / (max - int2));
	slop3 = (float)((max_g - int2_g) / (max - int2));
	slop4 = (float)((max_g - int2_g) / (int1 - min));

	for (i = 0; i < Hheight * Hwidth; i++) {
		

		value = orig[i];
		value2 = orig2[i];

		if (value >= max_orig) value = (float)max_orig;
	  	if (value2 >= max_orig2) value2 = (float)max_orig2;
		if (value < min_orig) value = (float)min_orig;
		if (value < min_orig2) value = (float)min_orig2;

            if ((value >= min &&  value <= int1) && ( value2 >= min_g && value2 <= int1_g))

	    {

	eq1 = (int)(value * (int1_g - min_g) - value2 * (int1 - min) 
				+ (int1*min_g - min*int1_g));
	eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));

		if ((eq1*eq2) > 0) 
		     grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] = (int)(grey_x * max_orig);

	   }


	 if ((value >= int1 && value <= int2) && 
			( value2 >= min_g && value2 <= int1_g))
	   
	 {

		grey_x = ( -1 * d1 - b1 * value2 - a1 * value ) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	 }

	 if ((value >= int2 && value <= max) && 
			(value2 >= min_g && value2 <= int1_g)) 

	 {

	
	eq1 = (int)(value * (int1_g - min_g) - value2 * (int2 - max) 
				+ (int2*min_g - max*int1_g));
	eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));

 		if ((eq1*eq2) < 0) 
		     grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}

	if ((value >= min && value <= int1) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d4 - b4 * value2 - a4 * value ) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}


	if ((value >= min && value <= int1) && 
		(value2 >= int2_g && value2 <= max_g))

        {
	
	
	eq1 = (int)(value * (int2_g - max_g) - value2 * (int1 - min) 
				+ (int1*max_g - min*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1*eq2) < 0 ) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig);

	}

	if ((value >= int2 && value <= max ) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d2 - b2 * value2 - a2 * value ) / c2;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	if ((value >= int2 && value <= max) 
		&& (value2 >= int2_g && value2 <= max_g))

        {

	
	eq1 = (int)(value * (int2_g - max_g) - value2 * (int2 - max) 
				+ (int2*max_g - max*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1*eq2) > 0 ) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	if ((value >= int1 && value <= int2) 
		&& (value2 >= int2_g && value2 <= max_g))

	{

		grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	 if ((value >= int1 && value <=int2) 
		&& (value2 >= int1_g && value2 <= int2_g))

	 {

		Class_2d[i] = (unsigned short)(max_thresh * max_orig);

	  }

	}  
		
	}

	
	for (i = 0; i < Hheight * Hwidth; i++) {

Class_2d[i] = min_orig + Class_2d[i] * (max_orig - min_orig) / max_orig;

	}


	return(0);

}
/*-----------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/

/*    Modified: 3/2/95 multiplication by orig[i] removed by Dewey Odhner.
 */
int Clas_2d168(unsigned short *orig, int min_orig, int max_orig, unsigned char *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned short *Class_2d)
{

	unsigned int i, MaxValue, Hheight,
			Hwidth;
	float	value,value2;
	int eq1, eq2;

	float		f1, f2;

		
	float min_thresh = 0.0;
	float max_thresh = 1.0;

	float xkj1, ykj1, zkj1, xlj1, ylj1, zlj1, 
	      xkj2, ykj2, zkj2, xlj2, ylj2, zlj2,
	      xkj3, ykj3, zkj3, xlj3, ylj3, zlj3,
              xkj4, ykj4, zkj4, xlj4, ylj4, zlj4;
	
	float a1, b1, c1, d1,
	      a2, b2, c2, d2,
	      a3, b3, c3, d3,
	      a4, b4, c4, d4;

	float slop1, slop2, slop3, slop4;


	double grey_x;

	Hheight = ht;
	Hwidth = wh;

	MaxValue = Hwidth * Hheight;


	for (i=0; i < MaxValue; i++) Class_2d[i] = 0; 


	f1 = (float)(1.0 / (max_orig - min_orig) * Hheight * 0.7);
	f2 = (float)(1.0 / (max_orig2 - min_orig2) * Hwidth * 0.7);

if ((min != 0 || int1 != 0) || (int2 != 0 || max != 0)) 
	  if ((min_g != 0 || int1_g != 0) || (int2_g != 0 || max_g != 0))
	
	{

	
	xkj1 = (float)(max - min);
	ykj1 = 0;
	zkj1 = 0;

	xlj1 = (float)(int1 - min);
	ylj1 = (float)(int1_g - min_g);
	zlj1 = max_thresh - min_thresh;


	xkj2 = 0;
	ykj2 = (float)(max_g - min_g);
	zkj2 = 0;

	xlj2 = (float)(int2 - max);
	ylj2 = (float)(int1_g - min_g);
	zlj2 = max_thresh - min_thresh;

	
	xkj3 = (float)(min - max);
	ykj3 = 0;
	zkj3 = 0;

	xlj3 = (float)(int2 - max);
	ylj3 = (float)(int2_g - max_g);
	zlj3 = max_thresh - min_thresh;


	xkj4 = 0;
	ykj4 = (float)(min_g - max_g);
	zkj4 = 0;

	xlj4 = (float)(int1 - min);
	ylj4 = (float)(int2_g - max_g);
	zlj4 = max_thresh - min_thresh;

	a1 = ykj1 * zlj1 - zkj1 * ylj1;
	b1 = zkj1 * xlj1 - xkj1 * zlj1;
	c1 = xkj1 * ylj1 - ykj1 * xlj1;
	d1 = -1 * (max * a1 + min_g * b1 + 0 * c1);

	a2 = ykj2 * zlj2 - zkj2 * ylj2;
	b2 = zkj2 * xlj2 - xkj2 * zlj2;
	c2 = xkj2 * ylj2 - ykj2 * xlj2;
	d2 = -1 * (max * a2 + max_g * b2 + 0 * c2);
	
	a3 = ykj3 * zlj3 - zkj3 * ylj3;
	b3 = zkj3 * xlj3 - xkj3 * zlj3;
	c3 = xkj3 * ylj3 - ykj3 * xlj3;
	d3 = -1 * (min * a3 + max_g * b3 + 0 * c3);

	a4 = ykj4 * zlj4 - zkj4 * ylj4;
	b4 = zkj4 * xlj4 - xkj4 * zlj4;
	c4 = xkj4 * ylj4 - ykj4 * xlj4;
	d4 = -1 * (min * a4 + min_g * b4 + 0 * c4);


	slop1 = (float)((int1_g - min_g) / (int1 - min));
	slop2 = (float)((int1_g - min_g) / (max - int2));
	slop3 = (float)((max_g - int2_g) / (max - int2));
	slop4 = (float)((max_g - int2_g) / (int1 - min));

	for (i = 0; i < Hheight * Hwidth; i++) {
		

		value = orig[i];
		value2 = orig2[i];

		if (value >= max_orig) value = (float)max_orig;
	  	if (value2 >= max_orig2) value2 = (float)max_orig2;
		if (value < min_orig) value = (float)min_orig;
		if (value < min_orig2) value = (float)min_orig2;

            if ((value >= min &&  value <= int1) && ( value2 >= min_g && value2 <= int1_g))

	    {

	eq1 = (int)(value * (int1_g - min_g) - value2 * (int1 - min) 
				+ (int1*min_g - min*int1_g));
	eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));

		if ((eq1*eq2) > 0) 
		     grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] = (int)(grey_x * max_orig);

	   }


	 if ((value >= int1 && value <= int2) && 
			( value2 >= min_g && value2 <= int1_g))
	   
	 {

		grey_x = ( -1 * d1 - b1 * value2 - a1 * value ) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	 }

	 if ((value >= int2 && value <= max) && 
			(value2 >= min_g && value2 <= int1_g)) 

	 {

	
	eq1 = (int)(value * (int1_g - min_g) - value2 * (int2 - max) 
				+ (int2*min_g - max*int1_g));
	eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));

 		if ((eq1*eq2) < 0) 
		     grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}

	if ((value >= min && value <= int1) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d4 - b4 * value2 - a4 * value ) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}


	if ((value >= min && value <= int1) && 
		(value2 >= int2_g && value2 <= max_g))

        {

	eq1 = (int)(value * (int2_g - max_g) - value2 * (int1 - min) 
				+ (int1*max_g - min*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1*eq2) < 0) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig);

	}

	if ((value >= int2 && value <= max ) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d2 - b2 * value2 - a2 * value ) / c2;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	if ((value >= int2 && value <= max) 
		&& (value2 >= int2_g && value2 <= max_g))

        {

	eq1 = (int)(value * (int2_g - max_g) - value2 * (int2 - max) 
				+ (int2*max_g - max*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1*eq2) > 0) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	if ((value >= int1 && value <= int2) 
		&& (value2 >= int2_g && value2 <= max_g))

	{

		grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	 if ((value >= int1 && value <=int2) 
		&& (value2 >= int1_g && value2 <= int2_g))

	 {

		Class_2d[i] = (unsigned short)(max_thresh * max_orig);

	  }

	}  
		
	}

	
	for (i = 0; i < Hheight * Hwidth; i++) {

Class_2d[i] = min_orig + Class_2d[i] * (max_orig - min_orig) / max_orig;

	}


	return(0);

}
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/*    Modified: 3/2/95 multiplication by orig[i] removed by Dewey Odhner.
 */
int Clas_2d88(unsigned char *orig, int min_orig, int max_orig, unsigned char *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned char *Class_2d)
{

	unsigned int i, MaxValue, Hheight,
			Hwidth;
	float	value,value2;

	float		f1, f2;

	int eq1, eq2;
	
	float min_thresh = 0.0;
	float max_thresh = 1.0;

	float xkj1, ykj1, zkj1, xlj1, ylj1, zlj1, 
	      xkj2, ykj2, zkj2, xlj2, ylj2, zlj2,
	      xkj3, ykj3, zkj3, xlj3, ylj3, zlj3,
              xkj4, ykj4, zkj4, xlj4, ylj4, zlj4;
	
	float a1, b1, c1, d1,
	      a2, b2, c2, d2,
	      a3, b3, c3, d3,
	      a4, b4, c4, d4;

	float slop1, slop2, slop3, slop4;


	double grey_x;


	Hheight = ht;
	Hwidth = wh;

	MaxValue = Hwidth * Hheight;


	for (i=0; i < MaxValue; i++) Class_2d[i] = 0; 


	f1 = (float)(1.0 / (max_orig - min_orig) * Hheight * 0.7);
	f2 = (float)(1.0 / (max_orig2 - min_orig2) * Hwidth * 0.7);

if ((min != 0 || int1 != 0) || (int2 != 0 || max != 0)) 
	  if ((min_g != 0 || int1_g != 0) || (int2_g != 0 || max_g != 0))
	
	{

	
	xkj1 = (float)(max - min);
	ykj1 = 0;
	zkj1 = 0;

	xlj1 = (float)(int1 - min);
	ylj1 = (float)(int1_g - min_g);
	zlj1 = max_thresh - min_thresh;


	xkj2 = 0;
	ykj2 = (float)(max_g - min_g);
	zkj2 = 0;

	xlj2 = (float)(int2 - max);
	ylj2 = (float)(int1_g - min_g);
	zlj2 = max_thresh - min_thresh;

	
	xkj3 = (float)(min - max);
	ykj3 = 0;
	zkj3 = 0;

	xlj3 = (float)(int2 - max);
	ylj3 = (float)(int2_g - max_g);
	zlj3 = max_thresh - min_thresh;


	xkj4 = 0;
	ykj4 = (float)(min_g - max_g);
	zkj4 = 0;

	xlj4 = (float)(int1 - min);
	ylj4 = (float)(int2_g - max_g);
	zlj4 = max_thresh - min_thresh;

	a1 = ykj1 * zlj1 - zkj1 * ylj1;
	b1 = zkj1 * xlj1 - xkj1 * zlj1;
	c1 = xkj1 * ylj1 - ykj1 * xlj1;
	d1 = -1 * (max * a1 + min_g * b1 + 0 * c1);

	a2 = ykj2 * zlj2 - zkj2 * ylj2;
	b2 = zkj2 * xlj2 - xkj2 * zlj2;
	c2 = xkj2 * ylj2 - ykj2 * xlj2;
	d2 = -1 * (max * a2 + max_g * b2 + 0 * c2);
	
	a3 = ykj3 * zlj3 - zkj3 * ylj3;
	b3 = zkj3 * xlj3 - xkj3 * zlj3;
	c3 = xkj3 * ylj3 - ykj3 * xlj3;
	d3 = -1 * (min * a3 + max_g * b3 + 0 * c3);

	a4 = ykj4 * zlj4 - zkj4 * ylj4;
	b4 = zkj4 * xlj4 - xkj4 * zlj4;
	c4 = xkj4 * ylj4 - ykj4 * xlj4;
	d4 = -1 * (min * a4 + min_g * b4 + 0 * c4);


	slop1 = (float)((int1_g - min_g) / (int1 - min));
	slop2 = (float)((int1_g - min_g) / (max - int2));
	slop3 = (float)((max_g - int2_g) / (max - int2));
	slop4 = (float)((max_g - int2_g) / (int1 - min));

	for (i = 0; i < Hheight * Hwidth; i++) {

		value = orig[i];
		value2 = orig2[i];

		if (value >= max_orig) value = (float)max_orig;
	  	if (value2 >= max_orig2) value2 = (float)max_orig2;
		if (value < min_orig) value = (float)min_orig;
		if (value < min_orig2) value = (float)min_orig2;

            if ((value >= min &&  value <= int1) && ( value2 >= min_g && value2 <= int1_g))

	    {
		

	eq1 = (int)(value * (int1_g - min_g) - value2 * (int1 - min) 
				+ (int1*min_g - min*int1_g));
	eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));

		if ((eq1*eq2) > 0) 
		     grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] = (int)(grey_x * max_orig);

	   }


	 if ((value >= int1 && value <= int2) && 
			( value2 >= min_g && value2 <= int1_g))
	   
	 {

		grey_x = ( -1 * d1 - b1 * value2 - a1 * value ) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	 }

	 if ((value >= int2 && value <= max) && 
			(value2 >= min_g && value2 <= int1_g)) 

	 {

		
	    eq1 = (int)(value * (int1_g - min_g) - value2 * (int2 - max) 
				+ (int2*min_g - max*int1_g));
	    eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));


 		if ((eq1*eq2) < 0) 
		     grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}

	if ((value >= min && value <= int1) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d4 - b4 * value2 - a4 * value ) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}


	if ((value >= min && value <= int1) && 
		(value2 >= int2_g && value2 <= max_g))

        {
		
	
	eq1 = (int)(value * (int2_g - max_g) - value2 * (int1 - min) 
				+ (int1*max_g - min*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1 * eq2) < 0) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}

	if ((value >= int2 && value <= max ) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d2 - b2 * value2 - a2 * value ) / c2;

		Class_2d[i] = (int)(grey_x * max_orig); 
	}

	if ((value >= int2 && value <= max) 
		&& (value2 >= int2_g && value2 <= max_g))

        {
	    

	eq1 = (int)(value * (int2_g - max_g) - value2 * (int2 - max) 
				+ (int2*max_g - max*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1 * eq2) > 0 ) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	if ((value >= int1 && value <= int2) 
		&& (value2 >= int2_g && value2 <= max_g))

	{

		grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	 if ((value >= int1 && value <=int2) 
		&& (value2 >= int1_g && value2 <= int2_g))

	 {
 	 
		Class_2d[i] = (unsigned char)(max_thresh * max_orig);

	  }

	}  
		
	}

	for (i = 0; i < Hheight * Hwidth; i++) {

Class_2d[i] = min_orig + Class_2d[i] * (max_orig - min_orig) / max_orig;

	}

	return(0);

}

/*----------------------------------------------------------------------*/

/*    Modified: 3/2/95 multiplication by orig[i] removed by Dewey Odhner.
 */
int Clas_2d816(unsigned char *orig, int min_orig, int max_orig, unsigned short *orig2, int min_orig2, int max_orig2, int ht, int wh, int min, int int1, int int2, int max, int min_g, int int1_g, int int2_g, int max_g, unsigned char *Class_2d)
{

	unsigned int i, MaxValue, Hheight,
			Hwidth;
	float	value,value2;

	float		f1, f2;

	int eq1, eq2;
	
	float min_thresh = 0.0;
	float max_thresh = 1.0;

	float xkj1, ykj1, zkj1, xlj1, ylj1, zlj1, 
	      xkj2, ykj2, zkj2, xlj2, ylj2, zlj2,
	      xkj3, ykj3, zkj3, xlj3, ylj3, zlj3,
              xkj4, ykj4, zkj4, xlj4, ylj4, zlj4;
	
	float a1, b1, c1, d1,
	      a2, b2, c2, d2,
	      a3, b3, c3, d3,
	      a4, b4, c4, d4;

	float slop1, slop2, slop3, slop4;


	double grey_x;


	Hheight = ht;
	Hwidth = wh;

	MaxValue = Hwidth * Hheight;


	for (i=0; i < MaxValue; i++) Class_2d[i] = 0; 


	f1 = (float)(1.0 / (max_orig - min_orig) * Hheight * 0.7);
	f2 = (float)(1.0 / (max_orig2 - min_orig2) * Hwidth * 0.7);

if ((min != 0 || int1 != 0) || (int2 != 0 || max != 0)) 
	  if ((min_g != 0 || int1_g != 0) || (int2_g != 0 || max_g != 0))
	
	{

	
	xkj1 = (float)(max - min);
	ykj1 = 0;
	zkj1 = 0;

	xlj1 = (float)(int1 - min);
	ylj1 = (float)(int1_g - min_g);
	zlj1 = max_thresh - min_thresh;


	xkj2 = 0;
	ykj2 = (float)(max_g - min_g);
	zkj2 = 0;

	xlj2 = (float)(int2 - max);
	ylj2 = (float)(int1_g - min_g);
	zlj2 = max_thresh - min_thresh;

	
	xkj3 = (float)(min - max);
	ykj3 = 0;
	zkj3 = 0;

	xlj3 = (float)(int2 - max);
	ylj3 = (float)(int2_g - max_g);
	zlj3 = max_thresh - min_thresh;


	xkj4 = 0;
	ykj4 = (float)(min_g - max_g);
	zkj4 = 0;

	xlj4 = (float)(int1 - min);
	ylj4 = (float)(int2_g - max_g);
	zlj4 = max_thresh - min_thresh;

	a1 = ykj1 * zlj1 - zkj1 * ylj1;
	b1 = zkj1 * xlj1 - xkj1 * zlj1;
	c1 = xkj1 * ylj1 - ykj1 * xlj1;
	d1 = -1 * (max * a1 + min_g * b1 + 0 * c1);

	a2 = ykj2 * zlj2 - zkj2 * ylj2;
	b2 = zkj2 * xlj2 - xkj2 * zlj2;
	c2 = xkj2 * ylj2 - ykj2 * xlj2;
	d2 = -1 * (max * a2 + max_g * b2 + 0 * c2);
	
	a3 = ykj3 * zlj3 - zkj3 * ylj3;
	b3 = zkj3 * xlj3 - xkj3 * zlj3;
	c3 = xkj3 * ylj3 - ykj3 * xlj3;
	d3 = -1 * (min * a3 + max_g * b3 + 0 * c3);

	a4 = ykj4 * zlj4 - zkj4 * ylj4;
	b4 = zkj4 * xlj4 - xkj4 * zlj4;
	c4 = xkj4 * ylj4 - ykj4 * xlj4;
	d4 = -1 * (min * a4 + min_g * b4 + 0 * c4);


	slop1 = (float)((int1_g - min_g) / (int1 - min));
	slop2 = (float)((int1_g - min_g) / (max - int2));
	slop3 = (float)((max_g - int2_g) / (max - int2));
	slop4 = (float)((max_g - int2_g) / (int1 - min));

	for (i = 0; i < Hheight * Hwidth; i++) {

		value = orig[i];
		value2 = orig2[i];

		if (value >= max_orig) value = (float)max_orig;
	  	if (value2 >= max_orig2) value2 = (float)max_orig2;
		if (value < min_orig) value = (float)min_orig;
		if (value < min_orig2) value = (float)min_orig2;

            if ((value >= min &&  value <= int1) && ( value2 >= min_g && value2 <= int1_g))

	    {
	
	     eq1 = (int)(value * (int1_g - min_g) - value2 * (int1 - min) 
				+ (int1*min_g - min*int1_g));
	     eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));


		if ((eq1*eq2) > 0) 
		     grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] = (int)(grey_x * max_orig);

	   }


	 if ((value >= int1 && value <= int2) && 
			( value2 >= min_g && value2 <= int1_g))
	   
	 {

		grey_x = ( -1 * d1 - b1 * value2 - a1 * value ) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	 }

	 if ((value >= int2 && value <= max) && 
			(value2 >= min_g && value2 <= int1_g)) 

	 {

	
	eq1 = (int)(value * (int1_g - min_g) - value2 * (int2 - max) 
				+ (int2*min_g - max*int1_g));
	eq2 = (int)(value * 0 - value2 * (max - min) + (max*min_g - min*min_g));

 		if ((eq1*eq2) < 0) 
		     grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		else grey_x = ( -1 * d1 - b1 * value2 - a1 * value) / c1;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}

	if ((value >= min && value <= int1) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d4 - b4 * value2 - a4 * value ) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig); 
	}


	if ((value >= min && value <= int1) && 
		(value2 >= int2_g && value2 <= max_g))

        {
	

	eq1 = (int)(value * (int2_g - max_g) - value2 * (int1 - min) 
				+ (int1*max_g - min*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1*eq2) < 0 ) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d4 - b4 * value2 - a4 * value) / c4;

		Class_2d[i] =  (int)(grey_x * max_orig); 

	}

	if ((value >= int2 && value <= max ) && 
		(value2 >= int1_g && value2 <= int2_g))
 
	{

		grey_x = ( -1 * d2 - b2 * value2 - a2 * value ) / c2;

		Class_2d[i] = (int)(grey_x * max_orig); 

	}

	if ((value >= int2 && value <= max) 
		&& (value2 >= int2_g && value2 <= max_g))

        {

	
	eq1 = (int)(value * (int2_g - max_g) - value2 * (int2 - max) 
				+ (int2*max_g - max*int2_g));

	eq2 = (int)(value * 0 - value2 * (max - min) + (max*max_g - min*max_g));

		if ((eq1*eq2) > 0) 
		     grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		else grey_x = ( -1 * d2 - b2 * value2 - a2 * value) / c2;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	if ((value >= int1 && value <= int2) 
		&& (value2 >= int2_g && value2 <= max_g))

	{

		grey_x = ( -1 * d3 - b3 * value2 - a3 * value) / c3;

		Class_2d[i] = (int)(grey_x * max_orig);

	}

	 if ((value >= int1 && value <=int2) 
		&& (value2 >= int1_g && value2 <= int2_g))

	 {
 	 
		Class_2d[i] = (unsigned char)(max_thresh * max_orig);

	  }

	}  
		
	}

	for (i = 0; i < Hheight * Hwidth; i++) {

Class_2d[i] = min_orig + Class_2d[i] * (max_orig - min_orig) / max_orig;

	}

	return(0);

}

/*------------------------------------------------------------------------*/
int Clas_2Dhist1616(unsigned short *orig, int min1, unsigned short *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned short *Class_2d)
{

	unsigned int i, NumPixels, height, width;
	float value;

	height = ht;
	width = wh;

	NumPixels = width * height;

	for (i=0; i < NumPixels; i++)
	    Class_2d[i] = 0;

	for (i = 0; i < height * width; i++) {
		
		if (orig[i]>=min1 && orig[i]<min1+bin_size[0]*nbins[0] &&
			orig2[i]>=min2 && orig2[i]<min2+bin_size[1]*nbins[1])
		{
		    value = (float)hist_count[(orig[i]-min1)/bin_size[0]]
		                             [(orig2[i]-min2)/bin_size[1]];
		    if (value <= hist_min)
		        continue;
		    if (value >= hist_max)
		        value = 65534;
		    else
		        value *= (float)(65534./(hist_max-hist_min));
		    Class_2d[i] = (unsigned short)value;
		}
	}
	return(0);

}


/*------------------------------------------------------------------------*/
int Clas_2Dhist168(unsigned short *orig, int min1, unsigned char *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned short *Class_2d)
{

	unsigned int i, NumPixels, height, width;
	float value;

	height = ht;
	width = wh;

	NumPixels = width * height;

	for (i=0; i < NumPixels; i++)
	    Class_2d[i] = 0;

	for (i = 0; i < height * width; i++) {
		
		if (orig[i]>=min1 && orig[i]<min1+bin_size[0]*nbins[0] &&
			orig2[i]>=min2 && orig2[i]<min2+bin_size[1]*nbins[1])
		{
		    value = (float)hist_count[(orig[i]-min1)/bin_size[0]]
		                             [(orig2[i]-min2)/bin_size[1]];
		    if (value <= hist_min)
		        continue;
		    if (value >= hist_max)
		        value = 65534;
		    else
		        value *= (float)(65534./(hist_max-hist_min));
		    Class_2d[i] = (unsigned short)value;
		}
	}
	return(0);

}


/*-----------------------------------------------------------------------*/
int Clas_2Dhist88(unsigned char *orig, int min1, unsigned char *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned char *Class_2d)
{

	unsigned int i, NumPixels, height, width;
	float value;

	height = ht;
	width = wh;

	NumPixels = width * height;

	for (i=0; i < NumPixels; i++)
	    Class_2d[i] = 0;

	for (i = 0; i < height * width; i++) {
		
		if (orig[i]>=min1 && orig[i]<min1+bin_size[0]*nbins[0] &&
			orig2[i]>=min2 && orig2[i]<min2+bin_size[1]*nbins[1])
		{
		    value = (float)hist_count[(orig[i]-min1)/bin_size[0]]
		                             [(orig2[i]-min2)/bin_size[1]];
		    if (value <= hist_min)
		        continue;
		    if (value >= hist_max)
		        value = 65534;
		    else
		        value *= (float)(65534./(hist_max-hist_min));
		    Class_2d[i] = (unsigned char)value;
		}
	}
	return(0);

}

/*----------------------------------------------------------------------*/
int Clas_2Dhist816(unsigned char *orig, int min1, unsigned short *orig2, int min2, int ht, int wh, double **hist_count, int bin_size[2], int nbins[2], double hist_min, double hist_max, unsigned char *Class_2d)
{

	unsigned int i, NumPixels, height, width;
	float value;

	height = ht;
	width = wh;

	NumPixels = width * height;

	for (i=0; i < NumPixels; i++)
	    Class_2d[i] = 0;

	for (i = 0; i < height * width; i++) {
		
		if (orig[i]>=min1 && orig[i]<min1+bin_size[0]*nbins[0] &&
			orig2[i]>=min2 && orig2[i]<min2+bin_size[1]*nbins[1])
		{
		    value = (float)hist_count[(orig[i]-min1)/bin_size[0]]
		                             [(orig2[i]-min2)/bin_size[1]];
		    if (value <= hist_min)
		        continue;
		    if (value >= hist_max)
		        value = 65534;
		    else
		        value *= (float)(65534./(hist_max-hist_min));
		    Class_2d[i] = (unsigned char)value;
		}
	}
	return(0);

}
