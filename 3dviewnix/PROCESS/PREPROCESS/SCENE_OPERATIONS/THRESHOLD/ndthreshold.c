/*
  Copyright 1993-2014, 2017-2018 Medical Image Processing Group
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
#include <stdlib.h>

int get_file_info(char *file, ViewnixHeader *vh, int *len);
void sixteen2bin(unsigned short *in, int w, int h, unsigned char *out),
 eight2bin(unsigned char *in, int w, int h, unsigned char *out);

unsigned short *threshold_table;	/* threshold table */
int table_size;	/* size of threshold table */
unsigned int range[100][2];	/* threshold intervals */
unsigned int nintervals;	/* #of intervals */
unsigned char binpix[8] = {128, 64, 32, 16, 8, 4, 2, 1};   /* binary pixels */
char *env;

/************************************************************************
 *                                                                      *
 *      Function        : main                                          *
 *      Description     : This function will perform filtering on an	*
 *			  nD Scene (3DViewnix format). Each slice is	*
 *			  loaded and filtered individually. The process	*
 *			  can run either in the foreground or 		*
 *			  background.					*
 *      Return Value    :  0 - works successfully.                      *
 *      Parameters      :  argc - the number of arguments.              *
 *                         argv - the argument list.                    *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on January 15, 1992 by 		*
 *			  Roberto J. Goncalves.				*
 *                        Modified: 8/14/95 comments initialized by
 *                        Dewey Odhner.
 *                        Modified: 8/14/95 byte swapping fixed by
 *                        Dewey Odhner.
 *                                                                      *
 ************************************************************************/
int main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin, *fpout;	/* inpput/output files */
	int execution_mode;	/* execution mode */
	ViewnixHeader vh;	/* 3DViewnix header */
        int slices;		/* total number of slices in the scene */
	char group[5],		/* Used in VWriteHeader */
		element[5];
	int length;		/* length of a slice */
	int outlength;		/* length of a output slice */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int depth;		/* bits per pixel */
	int i,j;		/* general use */
	int error;		/* error code */
	char *comments;	/* used to modify the header (description field) */
	int tmp;
	unsigned char *out_buffer1;
	unsigned char *in_buffer8;
	unsigned short *in_buffer16;
	char msg[200];

	if(argc<6)
	{
		printf("Usage:\n");
		printf("%% threshold input output mode min1 max1 [min2 max2  ...   minn maxn]\n");
		printf("where:\n");
		printf("input    : name of input file;\n");
		printf("output   : name of output file;\n");
		printf("mode     : 0=foreground,  1=background\n");
		printf("min1,max1: intensity range within which pixels are converted to 1s\n");
		printf("The user can specify as many intervals as necessary.\n");
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

	/* Get INTENSITY RANGE */
	range[0][0] = atoi(argv[4]);
	range[0][1] = atoi(argv[5]);

	/* Get optional INTENSITY RANGE */
	i=6;
	nintervals=1;
	while(argc > i)
	{
		if(argc - i >=2)
		{
			range[nintervals][0] = atoi(argv[i]);
			range[nintervals][1] = atoi(argv[i+1]);
			nintervals++;
		}
		i+=2;
	}

	/* If in background mode, then place an entry in the BG_PROCESS.COM file */
    if(execution_mode == 1)
        VAddBackgroundProcessInformation("ndinterpolate");


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
	memset(&vh, 0, sizeof(vh));
    get_file_info(argv[1], &vh, &hlength);

	/* Calculate length of a slice */
	width =  vh.scn.xysize[0];
	height =  vh.scn.xysize[1];
	depth= vh.scn.num_of_bits;
	length = (width*height*depth + 7)/8;
	outlength=(width*height +7)/8;
        if (vh.scn.dimension==3) {
		slices=vh.scn.num_of_subscenes[0];
	}
	else if (vh.scn.dimension==4)  {
		slices=0;
		for(i=0;i<vh.scn.num_of_subscenes[0];i++)
			slices += vh.scn.num_of_subscenes[i+1];
	}	
	else {
	   printf("ERROR: Cannot handle %d dimensional data\n",vh.scn.dimension);
	   exit(-1);
	}

	/* Build thresholding table */
	table_size = 1<<depth;
	if( (threshold_table = (unsigned short *) calloc(1, table_size*sizeof(short))) == NULL)
	{
      		printf("ERROR: Can't allocate threshold table.\n");
      		exit(1);
	}
	for(i=0; i<table_size; i++)
	{
		threshold_table[i] = 0;
		for(j=0; j<(int)nintervals; j++)
		{
			if(i>=(int)range[j][0]  &&  i<=(int)range[j][1])
				threshold_table[i] = 1;
		}
	}
 
   	/* create buffer for one binary image */
   	if( (out_buffer1 = (unsigned char *) malloc(outlength) ) == NULL)
   	{
   		printf("ERROR: Can't allocate output image buffer.\n");
   		exit(1);
   	}

	if(depth == 8)
	{
    	/* create buffer for one grey image */
    	if( (in_buffer8 = (unsigned char *) malloc(length) ) == NULL)
    	{
       		printf("ERROR: Can't allocate input image buffer.\n");
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
    strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename)-1);
	vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
    /* Build "description" header entry */
	j = argc*2;
	for(i=0; i<argc; i++)
		j += (int)strlen(argv[i]);
	comments = (char *)malloc(j);
	comments[0] = 0;
    for(i=0; i<argc; i++)
    {
        strcat(comments,argv[i]);
        strcat(comments," ");
    }
    vh.scn.description = comments;
    vh.scn.description_valid = 0x1;

	/* Write output 3DViewnix Header */
	error = VWriteHeader(fpout,&vh,group,element);
	if(error < 100)
	{
		printf("ERROR: Can't write output Header (ERROR #%d, %s-%s)!\n", error,group, element);
		exit(error);
	}

	VSeekData(fpin,0);	
	for(j=0; j<slices; j++)
	{

		if(execution_mode == 0)
		{
			printf("Thresholding slice #%d/%d ...\n", j+1,slices);
			fflush(stdout);
		}


		/* 8 BITS/PIXEL */
		if(depth == 8)
		{
			/* Load input slice */
			if(fread(in_buffer8, 1, length, fpin) != length)
       		{
       			printf("ERROR: Couldn't read slice #%d.\n", j+1);
       			exit(2);
       		}

			/* Threshold */
			eight2bin(in_buffer8, width, height, out_buffer1);


			/* Save output slice */
        	if(VWriteData((char *)out_buffer1, 1, outlength, fpout,&tmp))
        	{
           		printf("ERROR: Couldn't write slice #%d.\n", j+1);
           		exit(3);
        	}
		}
		/* 16 Bits/Pixel */
		else
		{
			/* Load input slice */
			if(VReadData((char *)in_buffer16,2,length/2,fpin,&tmp)) 
        	{
           		printf("ERROR: Couldn't read slice #%d.\n", j+1);
           		exit(2);
        	}

			/* Threshold */
			sixteen2bin(in_buffer16, width, height, out_buffer1);

			/* Save output slice */
        	if(VWriteData((char *)out_buffer1,1,outlength,fpout,&tmp))
        	{
           		printf("ERROR: Couldn't write slice #%d.\n", j+1);
           		exit(3);
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
	{
	env=getenv("VIEWNIX_ENV");
	sprintf(msg,"%s/PROCESS/BIN/job_done ndthreshold",env);
	system(msg);
        VDeleteBackgroundProcessInformation();
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
int get_file_info(char *file, ViewnixHeader *vh, int *len)
//ViewnixHeader *vh;
//char *file; /* filename */
//int *len;   /* length of header */
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
 


/************************************************************************
 *                                                                      *
 *      Function        : eight2bin                                     *
 *      Description     : This function converts 8-bit image to a 	*
 *			  binary image.					*
 *      Return Value    :  0 - works successfully.                      *
 *      Parameters      :  in - input mask.				*
 *			   w,h - size of the image.			*
 *			   out - output binary masked image.		*
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on January 15, 1992 by                *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
void eight2bin(unsigned char *in, int w, int h, unsigned char *out)
//unsigned char *in;        /* Input Mask */
//int w, h;                   /* size of images */
//unsigned char *out;         /* Output binary masked image */
{
    register int i,j,k;
    int sizebin,size;

    size=w*h; 
    sizebin = (size+7)/8;
     
    k=0;
    for(i=0; i<sizebin; i++)
    {
        out[i] = 0;
        for(j=0; j<8 && k < size; j++)
        {
            if( threshold_table[in[k]] >0)
                out[i] += binpix[j];
            k++;
        }
    }
}
 

/************************************************************************
 *                                                                      *
 *      Function        : sixteen2bin                                   *
 *      Description     : This function converts 16-bit image to a      *
 *                        binary image.                                 *
 *      Return Value    :  0 - works successfully.                      *
 *      Parameters      :  in - input mask.                             *
 *                         w,h - size of the image.                     *
 *                         out - output binary masked image.            *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on January 15, 1992 by                *
 *                        Roberto J. Goncalves.                         *
 *                                                                      *
 ************************************************************************/
void sixteen2bin(unsigned short *in, int w, int h, unsigned char *out)
//unsigned short *in;        /* Input Mask */
//int w, h;                   /* size of images */
//unsigned char *out;         /* Output binary masked image */
{
    register int i,j,k;
    int size,sizebin;
 
    size=w*h; 
    sizebin = (size+7)/8;
 
    k=0;
    for(i=0; i<sizebin; i++)
    {
        out[i] = 0;
        for(j=0; j<8 && k < size; j++)
        {
            if( threshold_table[in[k]] >0)
                out[i] += binpix[j];
            k++;
        }
    }
}
 
/************************************************************************/ 
 











