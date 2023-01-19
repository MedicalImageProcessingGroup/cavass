/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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
#include <assert.h>

#include "../VOI/slices.c"

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

typedef int pointpair[2][3];

void Compute3DLine(pointpair endpts, int (**linepts)[3], int *nlinepts)
{
	int longdim, j, k;
	X_Point *points2d[2];

	longdim = 0;
	j = endpts[1][0]-endpts[0][0];
	if (j < 0)
		j = -j;
	k = endpts[1][1]-endpts[0][1];
	if (k < 0)
		k = -k;
	if (k > j)
	{
		longdim = 1;
		j = k;
	}
	k = endpts[1][2]-endpts[0][2];
	if (k < 0)
		k = -k;
	if (k > j)
		longdim = 2;
	VComputeLine(endpts[0][longdim], endpts[0][(longdim+1)%3],
		endpts[1][longdim], endpts[1][(longdim+1)%3], points2d, &j);
	VComputeLine(endpts[0][longdim], endpts[0][(longdim+2)%3],
		endpts[1][longdim], endpts[1][(longdim+2)%3], points2d+1, &k);
	assert(j == k);
	*nlinepts = j;
	*linepts = (int (*)[3])malloc(j*sizeof(int [3]));
	for (k=0; k<j; k++)
	{
		assert(points2d[0][k].x == points2d[1][k].x);
		(*linepts)[k][longdim] = points2d[0][k].x;
		(*linepts)[k][(longdim+1)%3] = points2d[0][k].y;
		(*linepts)[k][(longdim+2)%3] = points2d[1][k].y;
	}
	free(points2d[0]);
	free(points2d[1]);
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
	int length_out;		/* length of output slice in bytes */
	int hlength;		/* length of the input header */
	int width, height;	/* dimensions of a slice */
	int i,j,k,m,n;		/* general use */
	int error;			/* error code */
	char *comments; 	/* used to modify the header (description field) */


	int nw, nh;			/* dimensions of new scene */

	unsigned char *out_buffer1;

	int tot_lines, *vol_lines;
	pointpair **endpoints;
	int (*linepoints)[3], nlinepoints;

	if(argc<9 || (argc%6!=3 && argc%8!=3))
	{
		printf("Usage:\n");
		printf("  sketch <input> <output> [<endpoint> <endpoint>] ...\n");
		printf("where:\n");
		printf("input    : name of input file;\n");
		printf("output   : name of output file;\n");
		printf("endpoint : scanner coordinates\n");
		exit(1);
	}

    /* Open INPUT File */
    if( (fpin = fopen(argv[1], "rb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open INPUT file !\n");
        exit(1);
    }


 
    /*-----------------------*/
    /* Read 3DViewnix header */
    /*-----------------------*/
    get_file_info(argv[1], &vh, &hlength);
 

	/* Compute information about the slices of the scene (number, locations, etc...) */
	compute_slices(&vh, &sl);

	endpoints = (pointpair **)malloc(sl.volumes*sizeof(pointpair *));
	vol_lines = (int *)calloc(sl.volumes, sizeof(int));

	if (vh.scn.dimension == 4)
	{
		if (argc%8 != 3)
		{
			fprintf(stderr, "8 coordinates per line required.\n");
			exit(1);
		}
		tot_lines = argc/8;
	}
	else // vh.scn.dimension == 3
	{
		if (argc%6 != 3)
		{
			fprintf(stderr, "6 coordinates per line required.\n");
			exit(1);
		}
		tot_lines = argc/6;
	}

	for (j=0; j<sl.volumes; j++)
		endpoints[j] = (pointpair *)malloc(tot_lines*sizeof(pointpair));

	for (k=0; k<tot_lines; k++)
	{
		float fpointbuf[2][3];

		if (vh.scn.dimension == 3)
		{
			j = 0;
			m = 3+6*k;
			i = 6+6*k;
		}
		else
		{
			if (sscanf(argv[6+8*k], "%d", &j)!=1 ||
					sscanf(argv[10+8*k], "%d", &m)!=1 ||
					j>=sl.volumes || m>=sl.volumes || j!=m)
			{
				fprintf(stderr, "Line is not in a volume.\n");
				exit(1);
			}
			m = 3+8*k;
			i = 7+8*k;
		}
		if (sscanf(argv[m], "%f", fpointbuf[0])!=1 ||
				sscanf(argv[m+1], "%f", fpointbuf[0]+1)!=1 ||
				sscanf(argv[m+2], "%f", fpointbuf[0]+2)!=1 ||
				sscanf(argv[i], "%f", fpointbuf[1])!=1 ||
				sscanf(argv[i+1], "%f", fpointbuf[1]+1)!=1 ||
				sscanf(argv[i+2], "%f", fpointbuf[1]+2)!=1)
		{
			fprintf(stderr, "Bad argument.\n");
			exit(1);
		}
		if (vh.scn.domain_valid)
			for (n=0; n<2; n++)
			{
				float ftmp[3];

				fpointbuf[n][0] -= vh.scn.domain[0];
				fpointbuf[n][1] -= vh.scn.domain[1];
				fpointbuf[n][2] -= vh.scn.domain[2];
				ftmp[0] = vh.scn.domain[3]*fpointbuf[n][0]+
						  vh.scn.domain[4]*fpointbuf[n][1]+
						  vh.scn.domain[5]*fpointbuf[n][2];
				ftmp[1] = vh.scn.domain[6]*fpointbuf[n][0]+
						  vh.scn.domain[7]*fpointbuf[n][1]+
						  vh.scn.domain[8]*fpointbuf[n][2];
				ftmp[2] = vh.scn.domain[9]*fpointbuf[n][0]+
						  vh.scn.domain[10]*fpointbuf[n][1]+
						  vh.scn.domain[11]*fpointbuf[n][2];
				fpointbuf[n][0] = ftmp[0];
				fpointbuf[n][1] = ftmp[1];
				fpointbuf[n][2] = ftmp[2];
			}
		fpointbuf[0][2] -= sl.location3[j][0];
		fpointbuf[1][2] -= sl.location3[j][0];
		fpointbuf[0][0] /= vh.scn.xypixsz[0];
		fpointbuf[0][1] /= vh.scn.xypixsz[1];
		fpointbuf[1][0] /= vh.scn.xypixsz[0];
		fpointbuf[1][1] /= vh.scn.xypixsz[1];
		if (sl.slices[j] > 1)
		{
			fpointbuf[0][2] /= sl.location3[j][1]-sl.location3[j][0];
			fpointbuf[1][2] /= sl.location3[j][1]-sl.location3[j][0];
		}
		endpoints[j][vol_lines[j]][0][0] = (int)rint(fpointbuf[0][0]);
		endpoints[j][vol_lines[j]][0][1] = (int)rint(fpointbuf[0][1]);
		endpoints[j][vol_lines[j]][0][2] = (int)rint(fpointbuf[0][2]);
		endpoints[j][vol_lines[j]][1][0] = (int)rint(fpointbuf[1][0]);
		endpoints[j][vol_lines[j]][1][1] = (int)rint(fpointbuf[1][1]);
		endpoints[j][vol_lines[j]][1][2] = (int)rint(fpointbuf[1][2]);
		vol_lines[j]++;
	}

    /* Open OUTPUT File */
    if( (fpout = fopen(argv[2], "wb")) == NULL)
    {
        fprintf(stderr, "ERROR: Can't open OUTPUT file !\n");
        exit(1);
    }

	/* EXTENT of VOI operation */

	/* Calculate length of a slice */
	width =  sl.width;
	height =  sl.height;
	length = (width * height + 7) / 8;
	nw = width;
	nh = height;

	length_out = (nw * nh +7) / 8 * sl.max_slices;



	/* Allocate memory */

   	/* create buffer for one grey-level image */
   	if( (out_buffer1 = (unsigned char *) malloc(length_out) ) == NULL)
   	{
   		fprintf(stderr, "ERROR: Can't allocate output image buffer.\n");
   		exit(1);
   	}

	/*-------------------------*/
	/* Modify 3DViewnix Header */
	vh.scn.num_of_bits = 1;
	vh.scn.signed_bits[0] = 0;
	vh.scn.smallest_density_value[0] = 0;
	vh.scn.largest_density_value[0] = 1;
	vh.scn.bit_fields[0] = 0;
	vh.scn.bit_fields[1] = 0;

	/* Get the filenames right (own and parent) */
    strncpy(vh.gen.filename1, argv[1], sizeof(vh.gen.filename1)-1);
	vh.gen.filename1[sizeof(vh.gen.filename1)-1] = 0;
	vh.gen.filename1_valid = 1;
    strncpy(vh.gen.filename, argv[2], sizeof(vh.gen.filename)-1);
	vh.gen.filename[sizeof(vh.gen.filename)-1] = 0;
	vh.gen.filename_valid = 1;
    /* Build "description" header entry */
	for (i=j=0; i<argc; i++)
		j = j+(int)strlen(argv[i]);
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
		exit(error);
	}


	/* Traverse ALL VOLUMES */
	for (j=0; j<sl.volumes; j++)
	{
	    length_out = (nw * nh +7) / 8 * sl.slices[j];
		memset(out_buffer1, 0, length_out);
		for (k=0; k<vol_lines[j]; k++)
		{
			Compute3DLine(endpoints[j][k], &linepoints, &nlinepoints);
			for (m=0; m<nlinepoints; m++)
			{
				if (linepoints[m][0]>=0 && linepoints[m][0]<nw &&
						linepoints[m][1]>=0 && linepoints[m][1]<nh &&
						linepoints[m][2]>=0 && linepoints[m][2]<sl.slices[j])
					out_buffer1[(nw * nh +7) / 8 * linepoints[m][2] +
						(nw*linepoints[m][1]+linepoints[m][0])/8] |=
						(128>>((nw*linepoints[m][1]+linepoints[m][0])%8));
			}
			free(linepoints);
		}

		/* Save output slice */
       	if(fwrite(out_buffer1, 1, length_out, fpout) != length_out)
       	{
       		fprintf(stderr, "ERROR: Couldn't write volume #%d.\n", j+1);
       		exit(3);
       	}
	}
	VCloseData(fpout);


	exit(0);
}
