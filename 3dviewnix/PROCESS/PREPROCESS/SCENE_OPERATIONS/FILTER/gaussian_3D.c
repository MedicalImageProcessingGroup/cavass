/*
  Copyright 2018, 2020 Medical Image Processing Group
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
#include <cv3dv.h>
#include "slices.c"

/*------------------------------------------------*/
/* Transform a binary image into a grey-level one */
/*------------------------------------------------*/
void bin_to_grey(unsigned char * bin_buffer,int length_bin, unsigned char * grey_buffer,int min_value, int max_value)
{
        int i, j;
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
 
 
}

/*-------------------------------------------------------------------------*/
/* Read the file header and its length */
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


int main(int argc, char *argv[])
{
	FILE *fpin, *fpout;	/* inpput/output files */
	ViewnixHeader vh;	/* 3DViewnix header */
	SLICES	sl;			/* Structure containing information about the slices of the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;			/* length of a slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	float sigma;		/* Standard Deviation for gaussian Distribution */
	int i,j,k,ii,jj,kk;	/* general use */
	int error;			/* error code */
	char *comments;     /* used to modify the header (description field) */
	int binary_flag=0;	/* indicates if scene is binary or not */
	int ll, kernel_radius[3], **kernel_limit, row, col;
	float ***kernel, max_value;
	double kn=0., kt;

	unsigned char *in_buffer1b;
	unsigned char *in_buffer8b, *out_buffer8;
	unsigned short *in_buffer16, *out_buffer16;


	if (argc != 4)
	{
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "%% gaussian_3D input output sigma\n");
		fprintf(stderr, "where:\n");
		fprintf(stderr, "input    : name of input file;\n");
		fprintf(stderr, "output   : name of output file;\n");
		fprintf(stderr, "sigma    : standard deviation used for the Gaussian distribution;\n");
		exit(1);
	}
	


	 
 
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

	/* Get SIGMA */
	if (sscanf(argv[3], "%f", &sigma)!=1 || !(sigma>0))
	{
		fprintf(stderr, "ERROR: sigma must be positive.\n");
		exit(1);
	}

    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Comoute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);

	if (vh.scn.largest_density_value_valid && vh.scn.num_of_bits>1)
		max_value = vh.scn.largest_density_value[0];
	else if (vh.scn.num_of_bits <= 8)
		max_value = 255;
	else
		max_value = 65535;
	for (kernel_radius[0]=0; erf(1/sigma*kernel_radius[0]*vh.scn.xypixsz[0])+
			.3/max_value<1; kernel_radius[0]++)
		;
	for (kernel_radius[1]=0; erf(1/sigma*kernel_radius[1]*vh.scn.xypixsz[1])+
			.3/max_value<1; kernel_radius[1]++)
		;
	for (kernel_radius[2]=0; erf(1/sigma*kernel_radius[2]*sl.min_spacing3[0])+
			.3/max_value<1; kernel_radius[2]++)
		;
	kernel = (float ***)malloc((1+2*kernel_radius[0])*sizeof(float **))+
		kernel_radius[0];
	for (i= -kernel_radius[0]; i<=kernel_radius[0]; i++)
	{
		kernel[i] = (float **)malloc((1+2*kernel_radius[1])*sizeof(float *))+
			kernel_radius[1];
		for (j= -kernel_radius[1]; j<=kernel_radius[1]; j++)
		{
			kernel[i][j]=(float *)malloc((1+2*kernel_radius[2])*sizeof(float))+
				kernel_radius[2];
			for (k= -kernel_radius[2]; k<=kernel_radius[2]; k++)
			{
				kernel[i][j][k] = (float)exp(
					-vh.scn.xypixsz[0]*vh.scn.xypixsz[0]*i*i/(2*sigma*sigma)-
					vh.scn.xypixsz[1]*vh.scn.xypixsz[1]*j*j/(2*sigma*sigma)-
					sl.min_spacing3[0]*sl.min_spacing3[0]*k*k/(2*sigma*sigma));
				kn += kernel[i][j][k];
			}
		}
	}
	for (i= -kernel_radius[0]; i<=kernel_radius[0]; i++)
		for (j= -kernel_radius[1]; j<=kernel_radius[1]; j++)
			for (k= -kernel_radius[2]; k<=kernel_radius[2]; k++)
				kernel[i][j][k] = (float)(1/kn*kernel[i][j][k]);
	kt = .25/(max_value*
		(1+kernel_radius[0])*(1+kernel_radius[1])*(1+kernel_radius[2]));
	kernel_limit =
		(int **)malloc((1+2*kernel_radius[0])*sizeof(int *))+kernel_radius[0];
	for (i= -kernel_radius[0]; i<=kernel_radius[0]; i++)
	{
		kernel_limit[i] = (int *)malloc((1+2*kernel_radius[1])*sizeof(int))+
			kernel_radius[1];
		for (j= -kernel_radius[1]; j<=kernel_radius[1]; j++)
			for (kernel_limit[i][j]=kernel_radius[2]; kernel_limit[i][j]>=0 &&
					kernel[i][j][kernel_limit[i][j]]<kt; kernel_limit[i][j]--)
				;
	}

	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	length = (width * height * vh.scn.num_of_bits)  / 8;
	if(vh.scn.num_of_bits == 1)
		length = (width * height + 7)  / 8;


	/* Allocate memory */
	if(vh.scn.num_of_bits == 1)
	{
    	/* create buffer for one binary image */
    	if( (in_buffer1b = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
		length = length*8;
		binary_flag = 1;

    	/* create buffer for one grey image */
    	if ((in_buffer8b=(unsigned char *)malloc(length*sl.max_slices))== NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey-level image */
    	if( (out_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	if(vh.scn.num_of_bits == 8)
	{
    	/* create buffer for one grey image */
    	if ((in_buffer8b=(unsigned char *)malloc(length*sl.max_slices))== NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer8 = (unsigned char *) calloc(1, length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}
	else
	{
    	/* create buffer for one grey image */
    	if ((in_buffer16=(unsigned short *)malloc(length*sl.max_slices) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate input image buffer.\n");
       		exit(1);
    	}
 
    	/* create buffer for one grey image */
    	if( (out_buffer16 = (unsigned short *) calloc(1, length) ) == NULL)
    	{
       		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
       		exit(1);
    	}
	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
	vh.scn.smallest_density_value[0] = 0;
	/* In case of binary, change header to 8 bits */
	if(vh.scn.num_of_bits == 1)
	{
		vh.scn.num_of_bits = 8;
		vh.scn.bit_fields[0] = 0;
		vh.scn.bit_fields[1] = 7;
		vh.scn.largest_density_value[0] = 255;
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
		fprintf(stderr, "ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
		exit(error);
	}

	
	/* Traverse ALL VOLUMES */
	k=0;
	for(j=0; j<sl.volumes; j++)
	{
	  /* For each Volume, traverse ALL SLICES */
	  for(i=0; i<sl.slices[j]; i++, k++)
	  {
		/* Seek the appropriate location */
		if(binary_flag == 1)
			fseek(fpin, (k*length/8)+hlength, 0);
		else
			fseek(fpin, k*length+hlength, 0);


		/* 1 or 8 Bits/Pixel */
		if(vh.scn.num_of_bits == 8)
		{
			/* Load input slice */
			/* BINARY */
			if(binary_flag == 1)
			{
				if(fread(in_buffer1b, 1, (length/8), fpin) != length/8)
        		{
           			fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}
				
				/* Convert to 8 Bits */
				bin_to_grey(in_buffer1b, (length/8), in_buffer8b+i*length, 0, 255);
			}
			/* 8 BITS/PIXEL */
			else
			{
				if(fread(in_buffer8b+i*length, 1, length, fpin) != length)
        		{
           			fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           			exit(2);
        		}
			}
		}
		/* 16 Bits/Pixel */
		else
		{
			/* Load input slice */
			VReadData((char *)(in_buffer16+i*length/2), 2, (length/2), fpin, &ll);
			if( ll != (length/2) )
        	{
           		fprintf(stderr, "ERROR: Couldn't read slice #%d of volume #%d.\n", i+1, j+1);
           		exit(2);
        	}
		}
	  }
	  for(i=0; i<sl.slices[j]; i++)
	  {
		/* 1 or 8 Bits/Pixel */
		if(vh.scn.num_of_bits == 8)
		{

			/* Filter */
			for (row=0; row<height; row++)
				for (col=0; col<width; col++)
				{
					int llimit0, llimit1, llimit2, ulimit0, ulimit1, ulimit2;
					float sum, ssum, csum;

					llimit0 = col<kernel_radius[0]? -col: -kernel_radius[0];
					ulimit0 = width-1-col>kernel_radius[0]?
					    kernel_radius[0]: width-1-col;
					sum = 0;
					for (ii=llimit0; ii<=ulimit0; ii++)
					{
						llimit1 = row<kernel_radius[1]? -row:-kernel_radius[1];
						ulimit1 = height-1-row>kernel_radius[1]?
							kernel_radius[1]: height-1-row;
						csum = 0;
						for (jj=llimit1; jj<=ulimit1; jj++)
						{
							llimit2 =
							  i<kernel_limit[ii][jj]? -i:-kernel_limit[ii][jj];
							ulimit2 = sl.slices[j]-1-i>kernel_limit[ii][jj]?
								kernel_limit[ii][jj]: sl.slices[j]-1-i;
							ssum = 0;
							for (kk=llimit2; kk<=ulimit2; kk++)
								ssum += kernel[ii][jj][kk]*in_buffer8b[
									width*(height*(i+kk)+(row+jj))+col+ii];
							csum += ssum;
						}
						sum += csum;
					}
					out_buffer8[width*row+col] = (unsigned char)rint(sum);
				}

			/* Save output slice */
        	if(fwrite(out_buffer8, width*height, 1, fpout) != 1)
        	{
           		fprintf(stderr, "ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}
		/* 16 Bits/Pixel */
		else
		{

			/* Filter */
			for (row=0; row<height; row++)
				for (col=0; col<width; col++)
				{
					int llimit0, llimit1, llimit2, ulimit0, ulimit1, ulimit2;
					float sum, ssum, csum;

					llimit0 = col<kernel_radius[0]? -col: -kernel_radius[0];
					ulimit0 = width-1-col>kernel_radius[0]?
					    kernel_radius[0]: width-1-col;
					sum = 0;
					for (ii=llimit0; ii<=ulimit0; ii++)
					{
						llimit1 = row<kernel_radius[1]? -row:-kernel_radius[1];
						ulimit1 = height-1-row>kernel_radius[1]?
							kernel_radius[1]: height-1-row;
						csum = 0;
						for (jj=llimit1; jj<=ulimit1; jj++)
						{
							llimit2 =
							  i<kernel_limit[ii][jj]? -i:-kernel_limit[ii][jj];
							ulimit2 = sl.slices[j]-1-i>kernel_limit[ii][jj]?
								kernel_limit[ii][jj]: sl.slices[j]-1-i;
							ssum = 0;
							for (kk=llimit2; kk<=ulimit2; kk++)
								ssum += kernel[ii][jj][kk]*in_buffer16[
									width*(height*(i+kk)+(row+jj))+col+ii];
							csum += ssum;
						}
						sum += csum;
					}
					out_buffer16[width*row+col]=(unsigned short)rint(sum);
				}

			/* Save output slice */
			if(VWriteData((char *)out_buffer16, 2, (length/2), fpout, &ll) !=0)
        	{
           		fprintf(stderr, "ERROR: Couldn't write slice #%d of volume #%d.\n", i+1, j+1);
           		exit(3);
        	}
		}


	  }
	}

	VCloseData(fpout);

	exit(0);
}
