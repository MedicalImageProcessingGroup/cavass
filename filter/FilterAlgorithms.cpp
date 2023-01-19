/*
  Copyright 1993-2012 Medical Image Processing Group
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

#include  "cavass.h"
#include  "FilterCanvas.h"
/*
    external programs used:
        b_scale_anisotrop_diffus_2D
        distance3D
        ndvoi
        scale_based_filtering_2D
*/

/*---------------------------------------------------------------------------------------*/
#define SQR(X) ( (X)*(X) )

/*-----------------------------------------------*/
/* Convolve a gradient operator on the input image */
/* 8bit/pixel */
/*-----------------------------------------------*/
/* Modified: 10/27/95 edge pixels set by Dewey Odhner */
void FilterCanvas::gradient8(unsigned char *input, int width, int height, 
						unsigned char *output, float MIN, float MAX)
{
        unsigned char *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	//register j, k;
	int j,k;
	short	*temporary, *temp;
	int square;


	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
    if( (temporary = (short *) calloc(1, length*sizeof(short)) ) == NULL)
    {
       printf("ERROR: Can't allocate temporary grey-level convolved image buffer.\n");
       exit(0);
    }	

	temp = temporary;
	in = input;
	out = output;


	/* offsets for the kernel */
	a = -width-1;
	b = -width;
	c = -width+1;
	d = -1;
	f = 1;
	g = width-1;
	h = width;
	i = width+1;

	/* 
	Horizontal Kernel:

	*-------*-------*-------*
	|   a	|   b	|   c	|
	| 	|  	| 	|
	*-------*-------*-------*
	|   d	|   e	|   f	|
	|  	|  	|  	|
	*-------*-------*-------*
	|   g	|   h	|   i	|
	| 	|  	| 	|
	*-------*-------*-------*

	*/

	/* loop over the pixels beggining from 2nd pixel of 2nd row */
	/* No, do all the pixels (10/27/95) */

	/* Horizontal */
	for(j=0; j<height; j++)
	{
	   in = input + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   *temp =
		    (short)(j==0? 0:*(in+c))+
		   	(short)*(in+f)+
			(short)(j==height-1? 0:*(in+i));
	   temp++;
	   in++;
	   for(k=1; k<width-1; k++)
	   {
		/* apply the kernel */
		*temp =
		    (j==0? 0:abs( -((short)*(in+a)) +  ((short)*(in+c)) ))+
		   	abs( -((short)*(in+d)) +  ((short)*(in+f)) )+
			(j==height-1? 0:abs( -((short)*(in+g)) +  ((short)*(in+i)) ));
		temp++;
		in++;
	   }
	   *temp =
		    (short)(j==0? 0:*(in+a))+
		   	(short)*(in+d)+
			(short)(j==height-1? 0:*(in+g));

	}


	/* Vertical */
	for(j=0; j<width; j++)
	{
	   in = input + j; 	/* position to 1st pixel of each column */
	   temp = temporary + j; 

	   square=
			(short)(j==width-1? 0:*(in+i))+
		   	(short)*(in+h)+
			(short)(j==0? 0:*(in+g));
	   *temp = (short) sqrt( SQR((double)*temp) + SQR((double)square) );
	   temp += width;
	   in += width;
	   for(k=1; k<height-1; k++)
	   {
		/* apply the kernel */
		square=
			(j==width-1? 0:abs( -((short)*(in+c)) +  ((short)*(in+i)) ))+
		   	abs( -((short)*(in+b)) +  ((short)*(in+h)) )+
			(j==0? 0:abs( -((short)*(in+a)) +  ((short)*(in+g)) ));
		*temp = (short) sqrt( SQR((double)*temp) + SQR((double)square) );
		temp += width;
		in += width;
	   }
	   square=
			(short)(j==width-1? 0:*(in+c))+
		   	(short)*(in+b)+
			(short)(j==0? 0:*(in+a));
	   *temp = (short) sqrt( SQR((double)*temp) + SQR((double)square) );

	}


	/* scale the data */
	for(j=0; j<height; j++)
	{
	   out = output + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   for(k=0; k<width; k++)
	   {
		*out = (unsigned char) (*temp/6.0 + 0.5);
		temp++;
		out++;
	   }
	}

	free(temporary);
}


/*-----------------------------------------------*/
/* Convolve a gradient operator on the input image */
/* 16bit/pixel */
/*-----------------------------------------------*/
/* Modified: 10/27/95 edge pixels set by Dewey Odhner */
void FilterCanvas::gradient16(unsigned short *input, int width, int height, 
							  unsigned short *output, float MIN, float MAX)
{
	unsigned short *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	//register j, k;
	int j,k;
	int	*temporary, *temp;
	int square;

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
    if( (temporary = (int *) calloc(sizeof(int), length) ) == NULL)
    {
       printf("ERROR: Can't allocate temporary grey-level convolved image buffer.\n");
       exit(0);
    }	

	temp = temporary;
	in = input;
	out = output;


	/* offsets for the kernel */
	a = -width-1;
	b = -width;
	c = -width+1;
	d = -1;
	f = 1;
	g = width-1;
	h = width;
	i = width+1;

	/* 
	Horizontal Kernel:

	*-------*-------*-------*
	|   a	|   b	|   c	|
	| 	|  	| 	|
	*-------*-------*-------*
	|   d	|   e	|   f	|
	|  	|  	|  	|
	*-------*-------*-------*
	|   g	|   h	|   i	|
	| 	|  	| 	|
	*-------*-------*-------*

	*/

	/* loop over the pixels beggining from 2nd pixel of 2nd row */
	/* No, do all the pixels (10/27/95) */

	/* Horizontal */
	for(j=0; j<height; j++)
	{
	   in = input + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   *temp =
		    (int)(j==0? 0:*(in+c))+
		   	(int)*(in+f)+
			(int)(j==height-1? 0:*(in+i));
	   temp++;
	   in++;
	   for(k=1; k<width-1; k++)
	   {
		/* apply the kernel */
		*temp =
		    (j==0? 0:abs( -((int)*(in+a)) +  ((int)*(in+c)) ))+
		   	abs( -((int)*(in+d)) +  ((int)*(in+f)) )+
			(j==height-1? 0:abs( -((int)*(in+g)) +  ((int)*(in+i)) ));
		temp++;
		in++;
	   }
	   *temp =
		    (int)(j==0? 0:*(in+a))+
		   	(int)*(in+d)+
			(int)(j==height-1? 0:*(in+g));

	}


	/* Vertical */
	for(j=0; j<width; j++)
	{
	   in = input + j; 	/* position to 1st pixel of each column */
	   temp = temporary + j; 

	   square=
			(int)(j==width-1? 0:*(in+i))+
		   	(int)*(in+h)+
			(int)(j==0? 0:*(in+g));
	   *temp = (int) sqrt( SQR((double)*temp) + SQR((double)square) );
	   temp += width;
	   in += width;
	   for(k=1; k<height-1; k++)
	   {
		/* apply the kernel */
		square=
			(j==width-1? 0:abs( -((int)*(in+c)) +  ((int)*(in+i)) ))+
		   	abs( -((int)*(in+b)) +  ((int)*(in+h)) )+
			(j==0? 0:abs( -((int)*(in+a)) +  ((int)*(in+g)) ));
		*temp = (int) sqrt( SQR((double)*temp) + SQR((double)square) );
		temp += width;
		in += width;
	   }
	   square=
			(int)(j==width-1? 0:*(in+c))+
		   	(int)*(in+b)+
			(int)(j==0? 0:*(in+a));
	   *temp = (int) sqrt( SQR((double)*temp) + SQR((double)square) );

	}

	/* scale the data */
	for(j=0; j<height; j++)
	{
	   out = output + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   for(k=0; k<width; k++)
	   {
		*out = (unsigned short) (*temp/6.0 + 0.5);
		temp++;
		out++;
	   }
	}
	free(temporary);
}
/*-----------------------------------*/
/* Gradient3D Based on Separated Slices */
/*-----------------------------------*/
int FilterCanvas::gradient_separated8(unsigned char *in1, unsigned char *in2,unsigned char *in3,
					int width, int height, float space,float pixel,
					unsigned char *out)
//unsigned char *in1,*in2,*in3;	/* three input slices */
//int width,height;		/* dimensions of the buffers */
//float space, pixel;		/* pixel size and separation between slices (in mm) */
//unsigned char *out;		/* output buffer */
{
	unsigned char *temp, *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v3, v5, v7;
	int j, k;
	long gx, gy, gz;
	float fgz;
	double	k1, k2;	/* weights for the filter */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |      in1
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*      ---
	|  v5  |  v0  |  v1  |     (in2)
	*------*------*------*      ---
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |      in3
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	OPITON I:
	gradient Ox:   Gx = in2/v1 - in2/v5
	gradient Oy:   Gy = in2/v7 - in2/v3
	gradient Oz:   Gz = in3/v0 - in1/v0

	*/

	temp = (unsigned char *) calloc(width*height, 1);
	if(temp == NULL) return 1;

	/* Weight of Oz Gradient in respect to Ox and Oy */
	k1 = pixel/space;


	/* offsets for the kernel */
	/*
	v4 = -width-1;
	v2 = -width+1;
	v6 = width-1;
	v8 = width+1;
	*/
	v0 = 0;  
	v1 = 1;
	v3 = -width;
	v5 = -1;
	v7 = width;


	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width + 1; 
	   t_in2 = in2 + j*width + 1;
	   t_in3 = in3 + j*width + 1;
	   t_out = temp + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
			gx = *(t_in2+v1) - *(t_in2+v5);
			gy = *(t_in2+v7) - *(t_in2+v3);
			gz = *(t_in3+v0) - *(t_in1+v0);
			fgz = gz;
			fgz *= k1; /* give appropriate weight to Oz gradient */
			k2 = gx*gx + gy*gy + fgz*fgz;
			*t_out = (unsigned short) sqrt(k2);

			t_out++;
			t_in1++;
			t_in2++;
			t_in3++;
	   }
	}

	/* Copy image to output buffer */
	for(k=width*height, t_out = out, t_in1 = temp; k>0; k--, t_out++, t_in1++)
		*t_out = *t_in1;

	free(temp);
	return 0;
}


/*---------------------------------------------------------------------------------------*/

/*-----------------------------------*/
/* Gradient3D Based on Separated Slices */
/*-----------------------------------*/
int FilterCanvas::gradient_separated16(unsigned short *in1, unsigned short *in2,unsigned short *in3,
					int width, int height, float space,float pixel,unsigned short *out)

//unsigned short *in1,*in2,*in3;	/* three input slices */
//int width,height;		/* dimensions of the buffers */
//float space, pixel;		/* pixel size and separation between slices */
//unsigned short *out;		/* output buffer */
{
	unsigned short *temp, *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v3, v5, v7;
	int j, k;
	long gx, gy, gz;
	float fgz;
	double	k1, k2;	/* weights for the filter */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |      in1
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*      ---
	|  v5  |  v0  |  v1  |     (in2)
	*------*------*------*      ---
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |      in3
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	OPITON I:
	gradient Ox:   Gx = in2/v1 - in2/v5
	gradient Oy:   Gy = in2/v7 - in2/v3
	gradient Oz:   Gz = in3/v0 - in1/v0

	*/

	temp = (unsigned short *) calloc(width*height*sizeof(short), 1);
	if(temp == NULL) return 1;

	/* Weight of Oz Gradient in respect to Ox and Oy */
	k1 = pixel/space;

	/* offsets for the kernel */
	/*
	v4 = -width-1;
	v2 = -width+1;
	v6 = width-1;
	v8 = width+1;
	*/
	v3 = -width;
	v5 = -1;
	v0 = 0;  
	v1 = 1;
	v7 = width;


	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width + 1; 
	   t_in2 = in2 + j*width + 1;
	   t_in3 = in3 + j*width + 1;
	   t_out = temp + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
			gx = *(t_in2+v1) - *(t_in2+v5);
			gy = *(t_in2+v7) - *(t_in2+v3);
			gz = *(t_in3+v0) - *(t_in1+v0);
			fgz = gz;
			fgz *= k1; /* give appropriate weight to Oz gradient */
			k2 = gx*gx + gy*gy + fgz*fgz;
			*t_out = (unsigned short) sqrt(k2);

			t_out++;
			t_in1++;
			t_in2++;
			t_in3++;
	   }
	}

	/* Copy image to output buffer */
	for(k=width*height, t_out = out, t_in1 = temp; k>0; k--, t_out++, t_in1++)
		*t_out = *t_in1;

	free(temp);
	return 0;
}


/*-----------------------------------*/
/* Gaussian 2D */
/* 8bits/pixel */
/* input slice */
/* dimensions of the buffers */
/* output buffer */
/* standard deviation */
/*-----------------------------------*/
int FilterCanvas::gaussian8(unsigned char *in2, int width,int height, 
							 unsigned char *out, float sigma)
{
	unsigned char *temp, *t_in2, *t_out;
	int length, v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;

	float	k0, k1, k2;	/* weights for the filter */
	float 	denom; /* denominator for normalizing the weights */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*/

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
    if( (temp = (unsigned char *) malloc(length*sizeof(unsigned char)) ) == NULL)
    {
       printf("ERROR: Can't allocate temporary grey-level convolved image buffer.\n");
       return(-1);
    }	

	/* Normal distribution of weights (where X=distance from central pixel) */
    k0 = normal(0.0, sigma);
    k1 = normal(1.0, sigma);
    k2 = normal((float)sqrt(2.0), sigma);
	denom = k0 + 4*k1 + 4*k2;

	/* offsets for the kernel */
	v4 = -width-1;
	v3 = -width;
	v2 = -width+1;
	v5 = -1;
	v0 = 0;  
	v1 = 1;
	v6 = width-1;
	v7 = width;
	v8 = width+1;


	for(j=0; j<height; j++)
	{	
	   /* position within the first pixel of each row */
	   t_in2 = in2 + j*width;
	   t_out = temp + j*width; 

	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in2+v1) + (j==0? 0: *(t_in2+v3)) +
			 (j==height-1? 0: *(t_in2+v7)) ) +
			k2*( (j==0? 0: (int)*(t_in2+v2)) +
			 (j==height-1? 0: *(t_in2+v8)) )
				) / denom);

	   t_out++;
	   t_in2++;
	   for(k=1; k<width-1; k++)
	   {
		 *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in2+v1) + (j==0? 0: *(t_in2+v3)) +
			 *(t_in2+v5) + (j==height-1? 0: *(t_in2+v7)) ) +
			k2*( (j==0? 0: (int)*(t_in2+v2) + *(t_in2+v4)) +
			 (j==height-1? 0: (int)*(t_in2+v6) + *(t_in2+v8)) )
				) / denom);

		 t_out++;
		 t_in2++;
	   }
	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( (j==0? 0: *(t_in2+v3)) +
			 *(t_in2+v5) + (j==height-1? 0: *(t_in2+v7)) ) +
			k2*( (j==0? 0: *(t_in2+v4)) +
			 (j==height-1? 0: *(t_in2+v6)) )
				) / denom);
	}


	/* copy temporary convolved buffer to output buffer */
	memcpy(out, temp, length*sizeof(unsigned char));
	free(temp);

	return(0);
}



/*-----------------------------------*/
/* Gaussian 2D */
/* 16bits/pixel */
/*-----------------------------------*/
/* Modified: 11/2/95 edge pixels set by Dewey Odhner */
/*-----------------------------------*/
int FilterCanvas::gaussian16(unsigned short *in2, int width,int height, 
							 unsigned short *out, float sigma)
{
	unsigned short *temp, *t_in2, *t_out;
	int length, v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;

	float	k0, k1, k2;	/* weights for the filter */
	float 	denom; /* denominator for normalizing the weights */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*/

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
    if( (temp = (unsigned short *) malloc(length*sizeof(unsigned short)) ) == NULL)
    {
       printf("ERROR: Can't allocate temporary grey-level convolved image buffer.\n");
       return(-1);
    }	

	/* Normal distribution of weights (where X=distance from central pixel) */
    k0 = normal(0.0, sigma);
    k1 = normal(1.0, sigma);
    k2 = normal((float)sqrt(2.0), sigma);
	denom = k0 + 4*k1 + 4*k2;

	/* offsets for the kernel */
	v4 = -width-1;
	v3 = -width;
	v2 = -width+1;
	v5 = -1;
	v0 = 0;  
	v1 = 1;
	v6 = width-1;
	v7 = width;
	v8 = width+1;


	for(j=0; j<height; j++)
	{	
	   /* position within the first pixel of each row */
	   t_in2 = in2 + j*width;
	   t_out = temp + j*width; 

	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in2+v1) + (j==0? 0: *(t_in2+v3)) +
			 (j==height-1? 0: *(t_in2+v7)) ) +
			k2*( (j==0? 0: (int)*(t_in2+v2)) +
			 (j==height-1? 0: *(t_in2+v8)) )
				) / denom);

	   t_out++;
	   t_in2++;
	   for(k=1; k<width-1; k++)
	   {
		 *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in2+v1) + (j==0? 0: *(t_in2+v3)) +
			 *(t_in2+v5) + (j==height-1? 0: *(t_in2+v7)) ) +
			k2*( (j==0? 0: (int)*(t_in2+v2) + *(t_in2+v4)) +
			 (j==height-1? 0: (int)*(t_in2+v6) + *(t_in2+v8)) )
				) / denom);

		 t_out++;
		 t_in2++;
	   }
	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( (j==0? 0: *(t_in2+v3)) +
			 *(t_in2+v5) + (j==height-1? 0: *(t_in2+v7)) ) +
			k2*( (j==0? 0: *(t_in2+v4)) +
			 (j==height-1? 0: *(t_in2+v6)) )
				) / denom);
	}


	/* copy temporary convolved buffer to output buffer */
	memcpy(out, temp, length*sizeof(unsigned short));

	free(temp);

	return(0);


}

/*-----------------------------------*/
/* Gaussian Based on Contiguous Slices */
/*-----------------------------------*/
int FilterCanvas::gaussian_contiguous(unsigned char *in1,unsigned char *in2,unsigned char *in3,
									  int width,int height, unsigned char *out, float sigma)
{
	unsigned char  *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;

	float	k0, k1, k2, k3;	/* weights for the filter */
	float 	denom; /* denominator for normalizing the weights */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*/

	/* kernel */
/* old way
	k0 = 2;
	k1 = 1;
	k2 = sqrt(2)/2;
	k3 = sqrt(3)/2;
	denom = k0 + 6*k1 + 12*k2 + 8*k3;
*/

	/* Normal distribution of weights (where X=distance from central pixel) */
    k0 = normal(0.0, sigma);
    k1 = normal(1.0, sigma);
    k2 = normal((float)sqrt(2.0), sigma);
    k3 = normal((float)sqrt(3.0), sigma);
	denom = k0 + 6*k1 + 12*k2 + 8*k3;

	/* offsets for the kernel */
	v4 = -width-1;
	v3 = -width;
	v2 = -width+1;
	v5 = -1;
	v0 = 0;  
	v1 = 1;
	v6 = width-1;
	v7 = width;
	v8 = width+1;


	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width + 1; 
	   t_in2 = in2 + j*width + 1;
	   t_in3 = in3 + j*width + 1;
	   t_out = out + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
		/* if all voxels are the same, theres no reason to filter */

		   *t_out = (unsigned char) ( (
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) +
			     *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v2) + *(t_in2+v4) + *(t_in2+v6) + *(t_in2+v8) +
			     *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k3*( *(t_in1+v2) + *(t_in1+v4) + *(t_in1+v6) + *(t_in1+v8) +
			     *(t_in3+v2) + *(t_in3+v4) + *(t_in3+v6) + *(t_in3+v8) )
				) / denom );

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	}

    return 0;
}

/*---------------------------------------------------------------------------------------*/

/*-----------------------------------*/
/* Gaussian Based on Separated Slices */
/* parameters: */
/*unsigned char *in1,*in2,*in3;	 three input slices */
/*int width,height;		 dimensions of the buffers */
/*float space, pixel;		 pixel size and separation between slices */
/*unsigned char *out;		 output buffer */
/*float sigma;			    standar deviation */

/*-----------------------------------*/
int FilterCanvas::gaussian_separated8(unsigned char *in1,unsigned char *in2,unsigned char *in3,
									  int width,int height,float space,float pixel,
									  unsigned char *out, float sigma)
{
	unsigned char *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;

	float	k0, k1, k2, k3, k4, k5;	/* weights for the filter */
	float 	denom; /* for normalizing the weights */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*/


	/* kernel */

	/* Normal distribution of weights (where X=distance from central pixel) */
        k0 = normal(0.0, sigma);
        k1 = normal((float)(space/pixel), sigma);
        k2 = normal(1.0, sigma);
        k3 = normal((float)sqrt(2.0), sigma);
	    k4 = normal((float)sqrt((space*space)+(pixel*pixel))/pixel, sigma);
	    k5 = normal((float)sqrt((space*space)+2*(pixel*pixel))/pixel, sigma);
        denom = k0 + 2*k1 + 4*k2 + 4*k3 + 8*k4 + 8*k5;

	/* offsets for the kernel */
	v4 = -width-1;
	v3 = -width;
	v2 = -width+1;
	v5 = -1;
	v0 = 0;  
	v1 = 1;
	v6 = width-1;
	v7 = width;
	v8 = width+1;


	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width + 1; 
	   t_in2 = in2 + j*width + 1;
	   t_in3 = in3 + j*width + 1;
	   t_out = out + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
		/* if all voxels are the same, theres no reason to filter */
		   *t_out = (unsigned char)(	(
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) ) +
			k3*( *(t_in2+v2) + *(t_in2+v4) + *(t_in2+v6) + *(t_in2+v8) ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k5*( *(t_in1+v2) + *(t_in1+v4) + *(t_in1+v6) + *(t_in1+v8) +
			     *(t_in3+v2) + *(t_in3+v4) + *(t_in3+v6) + *(t_in3+v8) )
				) /denom );

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	}

    return 0;
}


/*---------------------------------------------------------------------------------------*/

/*-----------------------------------*/
/* Gaussian Based on Separated Slices */
/* parameters: */
/*unsigned short *in1,*in2,*in3;	 three input slices */
/*int width,height;		 dimensions of the buffers */
/*float space, pixel;		 pixel size and separation between slices */
/*unsigned char *out;		 output buffer */
/*float sigma;			    standar deviation */

/*-----------------------------------*/
int FilterCanvas::gaussian_separated16(unsigned short *in1,unsigned short *in2,unsigned short *in3,
									  int width,int height,float space,float pixel,
									  unsigned short *out, float sigma)
{
	unsigned short *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;

	float	k0, k1, k2, k3, k4, k5;	/* weights for the filter */
	float 	denom; /* for normalizing the weights */

	/* Kernel within each slice:

	*------*------*------*
	|  v4  |  v3  |  v2  |
	*------*------*------*
	|  v5  |  v0  |  v1  |
	*------*------*------*
	|  v6  |  v7  |  v8  |
	*------*------*------*

	*/


	/* kernel */
/* old way
        k0 = 2;
        k1 = 1/space;;
        k2 = 1;
        k3 = sqrt(2)/2;
	k4 = sqrt( (space*space) + (pixel*pixel) );
	k5 = sqrt( (space*space) + 2*(pixel*pixel) );
        denom = k0 + 2*k1 + 4*k2 + 4*k3 + 8*k4 + 8*k5;
*/

	/* Normal distribution of weights (where X=distance from central pixel) */
        k0 = normal(0.0, sigma);
        k1 = normal((float)(space/pixel), sigma);
        k2 = normal(1.0, sigma);
        k3 = normal((float)sqrt(2.0), sigma);
	k4 = normal((float)sqrt((space*space)+(pixel*pixel))/pixel, sigma);
	k5 = normal((float)sqrt((space*space)+2*(pixel*pixel))/pixel, sigma);
        denom = k0 + 2*k1 + 4*k2 + 4*k3 + 8*k4 + 8*k5;

	/* offsets for the kernel */
	v4 = -width-1;
	v3 = -width;
	v2 = -width+1;
	v5 = -1;
	v0 = 0;  
	v1 = 1;
	v6 = width-1;
	v7 = width;
	v8 = width+1;


	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width + 1; 
	   t_in2 = in2 + j*width + 1;
	   t_in3 = in3 + j*width + 1;
	   t_out = out + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
		/* if all voxels are the same, theres no reason to filter */
/*
Takes too much time !!!
		i = *(t_in2+v0);
		if(*(t_in2+v1)==1 && *(t_in2+v3)==i && *(t_in2+v5)==i && *(t_in2+v7)==i &&
		   *(t_in1+v0)==i && *(t_in3+v0)==i &&
		   *(t_in2+v2)==i && *(t_in2+v4)==i && *(t_in2+v6)==i && *(t_in2+v8)==i &&
		   *(t_in1+v1)==i && *(t_in1+v3)==i && *(t_in1+v5)==i && *(t_in1+v7)==i &&
		   *(t_in3+v1)==i && *(t_in3+v3)==i && *(t_in3+v5)==i && *(t_in3+v7)==i &&
		   *(t_in1+v2)==i && *(t_in1+v4)==i && *(t_in1+v6)==i && *(t_in1+v8)==i &&
		   *(t_in3+v2)==i && *(t_in3+v4)==i && *(t_in3+v6)==i && *(t_in3+v8)==i )
		   *t_out = i;
		else
*/
		   *t_out = (unsigned short)(	(
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) ) +
			k3*( *(t_in2+v2) + *(t_in2+v4) + *(t_in2+v6) + *(t_in2+v8) ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k5*( *(t_in1+v2) + *(t_in1+v4) + *(t_in1+v6) + *(t_in1+v8) +
			     *(t_in3+v2) + *(t_in3+v4) + *(t_in3+v6) + *(t_in3+v8) )
				) /denom );

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	}

    return 0;
}
/*-----------------------------------------------*/
/* Return the value of the Normal distribution   */
/*-----------------------------------------------*/
float FilterCanvas::normal(float x, float sigma)
{
	float result, value1, value2, sigma_square;

	/* 
		Normal Distribution Equation:

		y = 1/(sqrt(2*PI*sigma^2)) * exp(- x^2/sigma^2)

	*/

	sigma_square = sigma*sigma;
	/*value1 = 2.0*3.1415927*sigma_square;*/
	value1 = 6.2831854*sigma_square;
    value2 = -(x*x)/(2.0*sigma_square);
	result = (float) (exp(value2) / sqrt(value1));
	return(result);	
}


void FilterCanvas::scale2d(char *fname, int sliceno, int width,int height, int num_of_bits, unsigned char *out, float sigma)
{
  CavassData*  sliceCD;
  wxString  cmd;
  cmd = wxString::Format("ndvoi\" \"%s\" voi_tmp.IM0 %d %d %d %d %d %d %d %d %d", 
			 fname,0,0,0,width,height, 0,0,sliceno,sliceno);
  wxString  tmp = wxString::Format( "\"%s/%s", (const char *)Preferences::getHome().c_str(), (const char *)cmd.c_str() );
  wxLogMessage( "command=%s", (const char *)tmp.c_str() );
  ProcessManager  p1( "filter running...", tmp );
  
  cmd = wxString::Format("scale_based_filtering_2D\" voi_tmp.IM0 /dev/null scale_t.IM0 %f",sigma);
  tmp = wxString::Format( "\"%s/%s", (const char *)Preferences::getHome().c_str(), (const char *)cmd.c_str() );
  wxLogMessage( "command=%s", (const char *)tmp.c_str() );
  ProcessManager  p2( "filter running...", tmp );
  
  sliceCD = new CavassData( "scale_t.IM0" );
  
  if(num_of_bits == 8)
    memcpy((char*)out, (char*)sliceCD->m_data,sliceCD->m_xSize * sliceCD->m_ySize);
  else if(num_of_bits == 16)
    memcpy((char*)out, (char*)sliceCD->m_data,sliceCD->m_xSize * sliceCD->m_ySize*2);

  unlink( "voi_tmp.IM0" );
  unlink( "scale_t.IM0" );
  
  if(sliceCD)
    delete sliceCD;
}

void FilterCanvas::scaleAv2d(char *fname, int sliceno, int width,int height,int num_of_bits,  unsigned char *out, float sigma)
{
  CavassData*  sliceCD;
  wxString  cmd;
  cmd = wxString::Format("ndvoi\" \"%s\" voi_tmp.IM0 %d %d %d %d %d %d %d %d %d", 
			 fname,0,0,0,width,height, 0,0,sliceno,sliceno);
  wxString  tmp = wxString::Format( "\"%s/%s", (const char *)Preferences::getHome().c_str(), (const char *)cmd.c_str() );
  wxLogMessage( "command=%s", (const char *)tmp.c_str() );
  ProcessManager  p1( "filter running...", tmp );
  
  cmd = wxString::Format("scale_based_filtering_2D\" voi_tmp.IM0 SBAv2D.IM0 /dev/null  %f",sigma);
  tmp = wxString::Format( "\"%s/%s", (const char *)Preferences::getHome().c_str(), (const char *)cmd.c_str() );
  wxLogMessage( "command=%s", (const char *)tmp.c_str() );
  ProcessManager  p2( "filter running...", tmp );
  
  sliceCD = new CavassData( "SBAv2D.IM0" );

  if(num_of_bits == 8)
    memcpy((char*)out, (char*)sliceCD->m_data,sliceCD->m_xSize * sliceCD->m_ySize);
  else if(num_of_bits == 16)
    memcpy((char*)out, (char*)sliceCD->m_data,sliceCD->m_xSize * sliceCD->m_ySize*2);

  
  unlink( "voi_tmp.IM0" );
  unlink( "SBAv2D.IM0"  );
  
  if(sliceCD)
    delete sliceCD;
}

void FilterCanvas::scaleAD2d(char *fname, int sliceno, int width,int height,int num_of_bits,  unsigned char *out, float sigma, int iteration)
{
  CavassData*  sliceCD;
  wxString  cmd;
  cmd = wxString::Format("ndvoi\" \"%s\" voi_tmp.IM0 %d %d %d %d %d %d %d %d %d", 
			 fname,0,0,0,width,height, 0,0,sliceno,sliceno);
  wxString  tmp = wxString::Format( "\"%s/%s", (const char *)Preferences::getHome().c_str(), (const char *)cmd.c_str() );
  wxLogMessage( "command=%s", (const char *)tmp.c_str() );
  ProcessManager  p1( "filter running...", tmp );
  
  cmd = wxString::Format("\"%s/b_scale_anisotrop_diffus_2D\" voi_tmp.IM0 SBAD2D.IM0 %f %d", (const char *)Preferences::getHome().c_str(), sigma, iteration);
  wxLogMessage( "command=%s", (const char *)cmd.c_str() );
  ProcessManager  p2( "filter running...", cmd );
  
  sliceCD = new CavassData( "SBAD2D.IM0" );

  if(num_of_bits == 8)
    memcpy((char*)out, (char*)sliceCD->m_data,sliceCD->m_xSize * sliceCD->m_ySize);
  else if(num_of_bits == 16)
    memcpy((char*)out, (char*)sliceCD->m_data,sliceCD->m_xSize * sliceCD->m_ySize*2);

  unlink( "voi_tmp.IM0" );
  unlink( "SBAD2D.IM0"  );
  
  if(sliceCD)
    delete sliceCD;
}


void FilterCanvas::dist2d(char *fname, int sliceno,int width,int height, unsigned char *out)
{
  CavassData*  sliceCD;
  wxString  cmd;
  cmd = wxString::Format("\"%s/ndvoi\" \"%s\" voi_tmp.BIM %d %d %d %d %d %d %d %d %d", 
			 (const char *)Preferences::getHome().c_str(),
			 fname,0,0,0,width,height, 0,0,sliceno,sliceno);
  wxLogMessage( "command=%s", (const char *)cmd.c_str() );
  ProcessManager  p1( "filter running...", cmd );

  cmd = wxString::Format("\"%s/distance3D\" -e -p xy -s voi_tmp.BIM filter-tmp.IM0", (const char *)Preferences::getHome().c_str());
  wxLogMessage( "command=%s", (const char *)cmd.c_str() );
  ProcessManager  p2( "filter running...", cmd );

  sliceCD = new CavassData( "filter-tmp.IM0" );
  
  if (sliceCD->m_max>255) {
    unsigned char*  ptr = (unsigned char*)sliceCD->m_data;
    for (int i = 0; i<sliceCD->m_xSize * sliceCD->m_ySize; i++) {
      /* rescale to 8 bits intensity scale */
      //out[i] = (sliceCD->m_data[i])*255/(sliceCD->m_max - sliceCD->m_min);
      out[i] = ptr[i] * 255 / (sliceCD->m_max - sliceCD->m_min);
    }
  } else {
    memcpy( (char*)out, (char*)sliceCD->m_data, sliceCD->m_xSize * sliceCD->m_ySize);
  }

  unlink( "voi_tmp.BIM"    );
  unlink( "filter-tmp.IM0" );
  
  if(sliceCD)
    delete sliceCD;

}
