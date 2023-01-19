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

 
 




/*******************
*  In this file:   *
*  TAB = 4 spaces  *
* (Set you editor) *
*******************/
#define SQR(X) ( (X)*(X) )

/*---------------------------------------------------------------------------------------*/

/*-----------------------------------------------*/
/* Return the value of the Normal distribution   */
/*-----------------------------------------------*/
float normal(float x, float sigma)
{
	float result, value1, value2, sigma_square;

	/* 
		Normal Distribution Equation:

		y = 1/(sqrt(2*PI*sigma^2)) * exp(- x^2/sigma^2)

	*/

	sigma_square = sigma*sigma;
	/*value1 = 2.0*3.1415927*sigma_square;*/
	value1 = (float)(6.2831854*sigma_square);
    value2 = (float)(-(x*x)/(2.0*sigma_square));
	result = (float) (exp(value2) / sqrt(value1));
	return(result);	


}


 
/*---------------------------------------------------------------------------------------*/
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


/*---------------------------------------------------------------------------------------*/
/*-----------------------------------------------*/
/* Convolve a gradient operator on the input image */
/* 8bit/pixel */
/*-----------------------------------------------*/
/* Modified: 10/27/95 edge pixels set by Dewey Odhner */
void gradient8(unsigned char *input, int width, int height, unsigned char *output, float MIN, float MAX)
{
	unsigned char *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	int j, k;
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
void gradient16(unsigned short *input, int width, int height, unsigned short *output, float MIN, float MAX)
{
	unsigned short *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	int j, k;
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


/*-----------------------------------------------*/
/* Convolve a sobel operator on the input image */
/*-----------------------------------------------*/
/* Modified: 11/2/95 edge pixels set by Dewey Odhner */
void sobel8(unsigned char *input, int width, int height, unsigned char *output, float MIN, float MAX)
{
	unsigned char *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	int j, k;
	int	*temporary, *temp, square;
	double square_root;

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
        if( (temporary = (int *) malloc(length*sizeof(int)) ) == NULL)
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
	/* No, do all the pixels (11/2/95) */

	/* Horizontal */
	for(j=0; j<height; j++)
	{
	   in = input + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   square = (j==0? 0: ((int)*(in+c)) ) +
		   	 	((int)*(in+f))*2 +
			 	(j==height-1? 0: ((int)*(in+i)) );
	   *temp = square * square;
	   temp++;
	   in++;
	   for(k=1; k<width-1; k++)
	   {
		/* apply the kernel */
		square = (j==0? 0: -((int)*(in+a)) + ((int)*(in+c)) ) +
		   	 	-((int)*(in+d))*2 +  ((int)*(in+f))*2 +
			 	(j==height-1? 0: -((int)*(in+g)) + ((int)*(in+i)) );
		*temp = square * square;
		temp++;
		in++;
	   }
	   square = (j==0? 0: -((int)*(in+a)) ) +
		   	 	-((int)*(in+d))*2 +
			 	(j==height-1? 0: -((int)*(in+g)) );
	   *temp = square * square;

	}


	/* Vertical */
	for(j=0; j<width; j++)
	{
	   in = input + j; 	/* position to 1st pixel of each column */
	   temp = temporary + j; 

	   square = (j==width-1? 0: ((int)*(in+i)) ) +
		   	 	((int)*(in+h))*2 +
			 	(j==0? 0: ((int)*(in+g)) );
	   square = square * square;
	   *temp += square;
	   square_root = (double) *temp;
	   *temp = (int) sqrt( square_root );
	   temp += width;
	   in += width;
	   for(k=1; k<height-1; k++)
	   {
		/* apply the kernel */
		square = (j==width-1? 0: -((int)*(in+c)) + ((int)*(in+i)) ) +
		   	 	-((int)*(in+b))*2 +  ((int)*(in+h))*2 +
			 	(j==0? 0: -((int)*(in+a)) + ((int)*(in+g)) );
		square = square * square;
		*temp += square;
		square_root = (double) *temp;
		*temp = (int) sqrt( square_root );
		temp += width;
		in += width;
	   }
	   square = (j==width-1? 0: -((int)*(in+c)) ) +
		   	 	-((int)*(in+b))*2 +
			 	(j==0? 0: -((int)*(in+a)) );
	   square = square * square;
	   *temp += square;
	   square_root = (double) *temp;
	   *temp = (int) sqrt( square_root );

	}


	/* scale the data */
	for(j=0; j<height; j++)
	{
	   out = output + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   for(k=0; k<width; k++)
	   {
		*out = (unsigned char)(*temp/8.0+0.5);
		temp++;
		out++;
	   }

	}


	free(temporary);
}

/*-----------------------------------------------*/
/* Convolve a sobel operator on the input image */
/*-----------------------------------------------*/
/* Modified: 11/2/95 edge pixels set by Dewey Odhner */
void sobel16(unsigned short *input, int width, int height, unsigned short *output, float MIN, float MAX)
{
	unsigned short *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	int j, k;
	int	*temporary, *temp,square;
	double square_root;

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
        if( (temporary = (int *) malloc(length*sizeof(int)) ) == NULL)
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
	/* No, do all the pixels (11/2/95) */

	/* Horizontal */
	for(j=0; j<height; j++)
	{
	   in = input + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   square = (j==0? 0: ((int)*(in+c)) ) +
		   	 	((int)*(in+f))*2 +
			 	(j==height-1? 0: ((int)*(in+i)) );
	   *temp = square * square;
	   temp++;
	   in++;
	   for(k=1; k<width-1; k++)
	   {
		/* apply the kernel */
		square = (j==0? 0: -((int)*(in+a)) + ((int)*(in+c)) ) +
		   	 	-((int)*(in+d))*2 +  ((int)*(in+f))*2 +
			 	(j==height-1? 0: -((int)*(in+g)) + ((int)*(in+i)) );
		*temp = square * square;
		temp++;
		in++;
	   }
	   square = (j==0? 0: -((int)*(in+a)) ) +
		   	 	-((int)*(in+d))*2 +
			 	(j==height-1? 0: -((int)*(in+g)) );
	   *temp = square * square;

	}


	/* Vertical */
	for(j=0; j<width; j++)
	{
	   in = input + j; 	/* position to 1st pixel of each column */
	   temp = temporary + j; 

	   square = (j==width-1? 0: ((int)*(in+i)) ) +
		   	 	((int)*(in+h))*2 +
			 	(j==0? 0: ((int)*(in+g)) );
	   square = square * square;
	   *temp += square;
	   square_root = (double) *temp;
	   *temp = (int) sqrt( square_root );
	   temp += width;
	   in += width;
	   for(k=1; k<height-1; k++)
	   {
		/* apply the kernel */
		square = (j==width-1? 0: -((int)*(in+c)) + ((int)*(in+i)) ) +
		   	 	-((int)*(in+b))*2 +  ((int)*(in+h))*2 +
			 	(j==0? 0: -((int)*(in+a)) + ((int)*(in+g)) );
		square = square * square;
		*temp += square;
		square_root = (double) *temp;
		*temp = (int) sqrt( square_root );
		temp += width;
		in += width;
	   }
	   square = (j==width-1? 0: -((int)*(in+c)) ) +
		   	 	-((int)*(in+b))*2 +
			 	(j==0? 0: -((int)*(in+a)) );
	   square = square * square;
	   *temp += square;
	   square_root = (double) *temp;
	   *temp = (int) sqrt( square_root );

	}


	/* scale the data */
	for(j=0; j<height; j++)
	{
	   out = output + j*width; /* position to 1st pixel of each row */
	   temp = temporary + j*width; 

	   for(k=0; k<width; k++)
	   {
		*out = (unsigned short)(*temp/8.0+0.5);
		temp++;
		out++;
	   }

	}


	free(temporary);
}




/*---------------------------------------------------------------------------------------*/


/*****************************************************************************
 * FUNCTION: median8
 * DESCRIPTION: Applies a 2-D median filter to a gray image of 8 bits per
 *    pixel.
 * PARAMETERS:
 *    in2: the input data array
 *    width, height: size of the image in pixels
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: successful completion
 *    1: out of memory
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/21/98 by Dewey Odhner
 *
 *****************************************************************************/

int median8(unsigned char *in2, int width, int height,unsigned char * out)
{
	unsigned char temp, a[9], *t_in2, *t_out, *out_buf;
	int j, k, n, x0, y0, x, y;

	out_buf = (unsigned char *)malloc(width*height);
	if (out_buf == NULL)
		return -1;
	for(j=0; j<height; j++)
	{	
	   	/* position within the first pixel of each row */
		t_in2 = in2 + j*width;
	   	t_out = out_buf + j*width;

	    y0 = (j==0 || j==height-1)? 0: -1;
		for(k=0; k<width; k++)
	    {
	   	    x0 = (k==0 || k==width-1)? 0: -1;
			n = 0;
			for (y=y0; y<= -y0; y++)
				for (x=x0; x<= -x0; x++)
					a[n++] = t_in2[y*width+x];
			for (y=1; y<n; y++)
			{
				temp = a[y];
				for (x=y-1; x>=0&&a[x]>temp; x--)
					a[x+1] = a[x];
				a[x+1] = temp;
			}
	    	*t_out = a[n/2];
			t_out++;
			t_in2++;
	    }
	}
	memcpy(out, out_buf, width*height);
	free(out_buf);
	return 0;
}

/*****************************************************************************
 * FUNCTION: median16
 * DESCRIPTION: Applies a 2-D median filter to a gray image of 16 bits per
 *    pixel.
 * PARAMETERS:
 *    in2: the input data array
 *    width, height: size of the image in pixels
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: successful completion
 *    1: out of memory
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/21/98 by Dewey Odhner
 *
 *****************************************************************************/

int median16( unsigned short *in2, int width, int height, unsigned short * out)
{
	unsigned short temp, a[9], *t_in2, *t_out, *out_buf;
	int j, k, n, x0, y0, x, y;

	out_buf = (unsigned short *)malloc(width*height*sizeof(short));
	if (out_buf == NULL)
		return -1;
	for(j=0; j<height; j++)
	{	
	   	/* position within the first pixel of each row */
		t_in2 = in2 + j*width;
	   	t_out = out_buf + j*width;

	    y0 = (j==0 || j==height-1)? 0: -1;
		for(k=0; k<width; k++)
	    {
	   	    x0 = (k==0 || k==width-1)? 0: -1;
			n = 0;
			for (y=y0; y<= -y0; y++)
				for (x=x0; x<= -x0; x++)
					a[n++] = t_in2[y*width+x];
			for (y=1; y<n; y++)
			{
				temp = a[y];
				for (x=y-1; x>=0&&a[x]>temp; x--)
					a[x+1] = a[x];
				a[x+1] = temp;
			}
	    	*t_out = a[n/2];
			t_out++;
			t_in2++;
	    }
	}
	memcpy(out, out_buf, width*height*sizeof(short));
	free(out_buf);
	return 0;
}

/*****************************************************************************
 * FUNCTION: median3d_8
 * DESCRIPTION: Applies a 3-D median filter to a gray image of 8 bits per
 *    pixel.
 * PARAMETERS:
 *    in1, in2, in3: the input data arrays of three input slices
 *    width, height: size of the image in pixels
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: successful completion
 *    1: out of memory
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/21/98 by Dewey Odhner
 *
 *****************************************************************************/
int median3d_8(unsigned char * in1,unsigned char *in2, unsigned char *in3, int width, int height, unsigned char *out)
{
	unsigned char temp, a[27], *t_in1, *t_in2, *t_in3, *t_out, *out_buf;
	int j, k, n, x0, y0, x, y;

	out_buf = (unsigned char *)malloc(width*height);
	if (out_buf == NULL)
		return -1;
	for(j=0; j<height; j++)
	{	
	   	/* position within the first pixel of each row */
		t_in1 = in1 + j*width;
		t_in2 = in2 + j*width;
		t_in3 = in3 + j*width;
	   	t_out = out_buf + j*width;

	    y0 = (j==0 || j==height-1)? 0: -1;
		for(k=0; k<width; k++)
	    {
	   	    x0 = (k==0 || k==width-1)? 0: -1;
			n = 0;
			for (y=y0; y<= -y0; y++)
				for (x=x0; x<= -x0; x++)
				{
					a[n++] = t_in1[y*width+x];
					a[n++] = t_in2[y*width+x];
					a[n++] = t_in3[y*width+x];
				}
			for (y=1; y<n; y++)
			{
				temp = a[y];
				for (x=y-1; x>=0&&a[x]>temp; x--)
					a[x+1] = a[x];
				a[x+1] = temp;
			}
	    	*t_out = a[n/2];
			t_out++;
			t_in1++;
			t_in2++;
			t_in3++;
	   }
	}
	memcpy(out, out_buf, width*height);
	free(out_buf);
	return 0;
}

/*****************************************************************************
 * FUNCTION: median3d_16
 * DESCRIPTION: Applies a 3-D median filter to a gray image of 16 bits per
 *    pixel.
 * PARAMETERS:
 *    in1, in2, in3: the input data arrays of three input slices
 *    width, height: size of the image in pixels
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: successful completion
 *    1: out of memory
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/21/98 by Dewey Odhner
 *
 *****************************************************************************/
int median3d_16( unsigned short *in1, unsigned short *in2, unsigned short *in3, int width, int height, unsigned short *out)
{
	unsigned short temp, a[27], *t_in1, *t_in2, *t_in3, *t_out, *out_buf;
	int j, k, n, x0, y0, x, y;

	out_buf = (unsigned short *)malloc(width*height*sizeof(short));
	if (out_buf == NULL)
		return -1;
	for(j=0; j<height; j++)
	{	
	   	/* position within the first pixel of each row */
		t_in1 = in1 + j*width;
		t_in2 = in2 + j*width;
		t_in3 = in3 + j*width;
	   	t_out = out_buf + j*width;

	    y0 = (j==0 || j==height-1)? 0: -1;
		for(k=0; k<width; k++)
	    {
	   	    x0 = (k==0 || k==width-1)? 0: -1;
			n = 0;
			for (y=y0; y<= -y0; y++)
				for (x=x0; x<= -x0; x++)
				{
					a[n++] = t_in1[y*width+x];
					a[n++] = t_in2[y*width+x];
					a[n++] = t_in3[y*width+x];
				}
			for (y=1; y<n; y++)
			{
				temp = a[y];
				for (x=y-1; x>=0&&a[x]>temp; x--)
					a[x+1] = a[x];
				a[x+1] = temp;
			}
	    	*t_out = a[n/2];
			t_out++;
			t_in1++;
			t_in2++;
			t_in3++;
	   }
	}
	memcpy(out, out_buf, width*height*sizeof(short));
	free(out_buf);
	return 0;
}



/*-----------------------------------*/
/* Gaussian 2D */
/* 8bits/pixel */
/*-----------------------------------*/
/* Modified: 11/2/95 edge pixels set by Dewey Odhner */
int gaussian8( unsigned char *in2, int width, int height, unsigned char *out, float sigma)
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
       return -1;
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

	return 0;


}



/*-----------------------------------*/
/* Gaussian 2D */
/* 16bits/pixel */
/*-----------------------------------*/
/* Modified: 11/2/95 edge pixels set by Dewey Odhner */
int gaussian16(unsigned short *in2, int width, int height, unsigned short *out, float sigma)
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
       return -1;
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
void gaussian_contiguous(unsigned char *in1, unsigned char *in2, unsigned char *in3, int width, int height, unsigned char *out, float sigma)
{
	unsigned char *t_in1, *t_in2, *t_in3, *t_out;
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
}

/*---------------------------------------------------------------------------------------*/

/*-----------------------------------*/
/* Gaussian Based on Separated Slices */
/*-----------------------------------*/
/* Modified: 10/25/99 edge pixels set by Dewey Odhner */
void gaussian_separated8( unsigned char *in1, unsigned char *in2, unsigned char *in3, int width, int height, float space, float pixel,
					unsigned char *out, float sigma)
{
	unsigned char *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;
	float	k0, k1, k2, k3, k4, k5;	/* weights for the filter */
	float 	coef; /* for normalizing the weights */

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
	coef = 1/(k0 + 2*k1 + 4*k2 + 4*k3 + 8*k4 + 8*k5);

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

	   /* position within the second pixel of each row */
	   t_in1 = in1; 
	   t_in2 = in2;
	   t_in3 = in3;
	   t_out = out; 

	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1)               +               *(t_in2+v7) ) +
			k3*(                                           *(t_in2+v8) ) +
			k4*( *(t_in1+v1)               +               *(t_in1+v7) +
			     *(t_in3+v1)               +               *(t_in3+v7) ) +
			k5*(                                           *(t_in1+v8) +
			                                               *(t_in3+v8) )
				) *coef);
		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   for(k=1; k<width-1; k++)
	   {
		 *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) +               *(t_in2+v5) + *(t_in2+v7) ) +
			k3*(                             *(t_in2+v6) + *(t_in2+v8) ) +
			k4*( *(t_in1+v1) +               *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) +               *(t_in3+v5) + *(t_in3+v7) ) +
			k5*(                             *(t_in1+v6) + *(t_in1+v8) +
			                                 *(t_in3+v6) + *(t_in3+v8) )
				) *coef);

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*(                             *(t_in2+v5) + *(t_in2+v7) ) +
			k3*(                             *(t_in2+v6)               ) +
			k4*(                             *(t_in1+v5) + *(t_in1+v7) +
			                                 *(t_in3+v5) + *(t_in3+v7) ) +
			k5*(                             *(t_in1+v6) +
			                                 *(t_in3+v6)               )
				) *coef);

	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width; 
	   t_in2 = in2 + j*width;
	   t_in3 = in3 + j*width;
	   t_out = out + j*width; 

	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) +               *(t_in2+v7) ) +
			k3*( *(t_in2+v2) +                             *(t_in2+v8) ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) +               *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) +               *(t_in3+v7) ) +
			k5*( *(t_in1+v2) +                             *(t_in1+v8) +
			     *(t_in3+v2) +                             *(t_in3+v8) )
				) *coef);
		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   for(k=1; k<width-1; k++)
	   {
		   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) ) +
			k3*( *(t_in2+v2) + *(t_in2+v4) + *(t_in2+v6) + *(t_in2+v8) ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k5*( *(t_in1+v2) + *(t_in1+v4) + *(t_in1+v6) + *(t_in1+v8) +
			     *(t_in3+v2) + *(t_in3+v4) + *(t_in3+v6) + *(t_in3+v8) )
				) *coef);

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*(               *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) ) +
			k3*(               *(t_in2+v4) + *(t_in2+v6)               ) +
			k4*(               *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			                   *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k5*(               *(t_in1+v4) + *(t_in1+v6) +
			                   *(t_in3+v4) + *(t_in3+v6)               )
				) *coef);
	}
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width; 
	   t_in2 = in2 + j*width;
	   t_in3 = in3 + j*width;
	   t_out = out + j*width; 

	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) ) +
			k3*( *(t_in2+v2)               ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) +                            
			     *(t_in3+v1) + *(t_in3+v3) ) +
			k5*( *(t_in1+v2) +                                          
			     *(t_in3+v2)               )
				) *coef);
		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   for(k=1; k<width-1; k++)
	   {
		   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5)               ) +
			k3*( *(t_in2+v2) + *(t_in2+v4)                             ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5)               +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5)               ) +
			k5*( *(t_in1+v2) + *(t_in1+v4)                             +
			     *(t_in3+v2) + *(t_in3+v4)                             )
				) *coef);

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	   *t_out = (unsigned char)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*(               *(t_in2+v3) + *(t_in2+v5)               ) +
			k3*(               *(t_in2+v4)                             ) +
			k4*(               *(t_in1+v3) + *(t_in1+v5)               +
			                   *(t_in3+v3) + *(t_in3+v5)               ) +
			k5*(               *(t_in1+v4)               +
			                   *(t_in3+v4)                             )
				) *coef);
}


/*---------------------------------------------------------------------------------------*/

/*-----------------------------------*/
/* Gaussian Based on Separated Slices */
/*-----------------------------------*/
/* Modified: 10/25/99 edge pixels set by Dewey Odhner */
void gaussian_separated16( unsigned short *in1,unsigned short *in2, unsigned short *in3, int width,int height, float space,float pixel, unsigned short *out, float sigma)
{
	unsigned short *t_in1, *t_in2, *t_in3, *t_out;
	int v0, v1, v2, v3, v4, v5, v6, v7, v8;
	int j, k;
	float	k0, k1, k2, k3, k4, k5;	/* weights for the filter */
	float 	coef; /* for normalizing the weights */

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
        coef = 1/(k0 + 2*k1 + 4*k2 + 4*k3 + 8*k4 + 8*k5);

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


	   /* position within the second pixel of each row */
	   t_in1 = in1; 
	   t_in2 = in2;
	   t_in3 = in3;
	   t_out = out; 

	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1)               +               *(t_in2+v7) ) +
			k3*(                                           *(t_in2+v8) ) +
			k4*( *(t_in1+v1)               +               *(t_in1+v7) +
			     *(t_in3+v1)               +               *(t_in3+v7) ) +
			k5*(                                           *(t_in1+v8) +
			                                               *(t_in3+v8) )
				) *coef);
		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   for(k=1; k<width-1; k++)
	   {
		 *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) +               *(t_in2+v5) + *(t_in2+v7) ) +
			k3*(                             *(t_in2+v6) + *(t_in2+v8) ) +
			k4*( *(t_in1+v1) +               *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) +               *(t_in3+v5) + *(t_in3+v7) ) +
			k5*(                             *(t_in1+v6) + *(t_in1+v8) +
			                                 *(t_in3+v6) + *(t_in3+v8) )
				) *coef);

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*(                             *(t_in2+v5) + *(t_in2+v7) ) +
			k3*(                             *(t_in2+v6)               ) +
			k4*(                             *(t_in1+v5) + *(t_in1+v7) +
			                                 *(t_in3+v5) + *(t_in3+v7) ) +
			k5*(                             *(t_in1+v6) +
			                                 *(t_in3+v6)               )
				) *coef);

	/* if columns go from 1 to n, loop from column 2 to n-1 */
	for(j=1; j<height-1; j++)
	{	
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width; 
	   t_in2 = in2 + j*width;
	   t_in3 = in3 + j*width;
	   t_out = out + j*width; 

	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) +               *(t_in2+v7) ) +
			k3*( *(t_in2+v2) +                             *(t_in2+v8) ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) +               *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) +               *(t_in3+v7) ) +
			k5*( *(t_in1+v2) +                             *(t_in1+v8) +
			     *(t_in3+v2) +                             *(t_in3+v8) )
				) *coef);
		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   for(k=1; k<width-1; k++)
	   {
		   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) ) +
			k3*( *(t_in2+v2) + *(t_in2+v4) + *(t_in2+v6) + *(t_in2+v8) ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k5*( *(t_in1+v2) + *(t_in1+v4) + *(t_in1+v6) + *(t_in1+v8) +
			     *(t_in3+v2) + *(t_in3+v4) + *(t_in3+v6) + *(t_in3+v8) )
				) *coef);

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*(               *(t_in2+v3) + *(t_in2+v5) + *(t_in2+v7) ) +
			k3*(               *(t_in2+v4) + *(t_in2+v6)               ) +
			k4*(               *(t_in1+v3) + *(t_in1+v5) + *(t_in1+v7) +
			                   *(t_in3+v3) + *(t_in3+v5) + *(t_in3+v7) ) +
			k5*(               *(t_in1+v4) + *(t_in1+v6) +
			                   *(t_in3+v4) + *(t_in3+v6)               )
				) *coef);
	}
	   /* position within the second pixel of each row */
	   t_in1 = in1 + j*width; 
	   t_in2 = in2 + j*width;
	   t_in3 = in3 + j*width;
	   t_out = out + j*width; 

	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) ) +
			k3*( *(t_in2+v2)               ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) +                            
			     *(t_in3+v1) + *(t_in3+v3) ) +
			k5*( *(t_in1+v2) +                                          
			     *(t_in3+v2)               )
				) *coef);
		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   for(k=1; k<width-1; k++)
	   {
		   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*( *(t_in2+v1) + *(t_in2+v3) + *(t_in2+v5)               ) +
			k3*( *(t_in2+v2) + *(t_in2+v4)                             ) +
			k4*( *(t_in1+v1) + *(t_in1+v3) + *(t_in1+v5)               +
			     *(t_in3+v1) + *(t_in3+v3) + *(t_in3+v5)               ) +
			k5*( *(t_in1+v2) + *(t_in1+v4)                             +
			     *(t_in3+v2) + *(t_in3+v4)                             )
				) *coef);

		t_out++;
		t_in1++;
		t_in2++;
		t_in3++;
	   }
	   *t_out = (unsigned short)((
			k0*( *(t_in2+v0) ) +
			k1*( *(t_in1+v0) + *(t_in3+v0) ) +
			k2*(               *(t_in2+v3) + *(t_in2+v5)               ) +
			k3*(               *(t_in2+v4)                             ) +
			k4*(               *(t_in1+v3) + *(t_in1+v5)               +
			                   *(t_in3+v3) + *(t_in3+v5)               ) +
			k5*(               *(t_in1+v4)               +
			                   *(t_in3+v4)                             )
				 ) *coef);
}


/*---------------------------------------------------------------------------------------*/


/*-----------------------------------*/
/* Gradient3D Based on Separated Slices */
/*-----------------------------------*/
void gradient_separated8( unsigned char *in1, unsigned char *in2, unsigned char *in3, int width,int height, float space, float pixel, unsigned char * out)
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
	if(temp == NULL) return;

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
			fgz = (float)(gz*k1); /* give appropriate weight to Oz gradient */
			k2 = gx*gx + gy*gy + fgz*fgz;
			*t_out = (unsigned char) sqrt(k2);

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
}


/*---------------------------------------------------------------------------------------*/

/*-----------------------------------*/
/* Gradient3D Based on Separated Slices */
/*-----------------------------------*/
void gradient_separated16( unsigned short *in1, unsigned short *in2, unsigned short *in3, int width, int height, float space, float pixel, unsigned short *out)
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
	if(temp == NULL) return;

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
			fgz = (float)(k1*gz); /* give appropriate weight to Oz gradient */
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
}


/*---------------------------------------------------------------------------------------*/

/*-----------------------------------------------*/
/* Low-pass filter on the input image */
/*-----------------------------------------------*/
void low_pass8(unsigned char *input, int width, int height, unsigned char *output)
{
	unsigned char *temporary, *temp, *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	int j, k;

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
    if( (temporary = (unsigned char *) malloc(length) ) == NULL)
    {
        printf("ERROR allocating temporary grey-level convolved image buffer.\n");
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

	/* Kernel:

	*-------*-------*-------*
	|   a	|   b	|   c	|
	| 1/16	|  1/8	| 1/16	|
	*-------*-------*-------*
	|   d	|   e	|   f	|
	|  1/8	|  1/4	|  1/8	|
	*-------*-------*-------*
	|   g	|   h	|   i	|
	| 1/16	|  1/8	| 1/16	|
	*-------*-------*-------*

	*/

	/* loop over the pixels beggining from 2nd pixel of 2nd row */
	for(j=1; j<height-1; j++)
	{
	   in = input + j*width + 1; /* position to 2nd pixel of each row */
	   temp = temporary + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
		/* apply the kernel */
		*temp = (*(in+a)>>4) + (*(in+b)>>3) + (*(in+c)>>4) +
		   	(*(in+d)>>3) +   (*(in)>>2) + (*(in+f)>>3) +
			(*(in+g)>>4) + (*(in+h)>>3) + (*(in+i)>>4);

		temp++;
		in++;
	   }

	}

	/* copy temporary convolved buffer to output buffer */
	memcpy(output, temporary, length);
	
	free(temporary);
}

/*---------------------------------------------------------------------------------------*/

/*-----------------------------------------------*/
/* Low-pass filter on the input image */
/*-----------------------------------------------*/
void low_pass16(unsigned short * input, int width, int height, unsigned short *output)
{
	unsigned short *temporary, *temp, *in, *out;
	int length, a, b, c, d, f, g, h, i;	
	int j, k;

	length = width*height;

	/* create convolution temporary buffer for one grey-level image */
    if( (temporary = (unsigned short *) malloc(length) ) == NULL)
    {
       	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
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

	/* Kernel:

	*-------*-------*-------*
	|   a	|   b	|   c	|
	| 1/16	|  1/8	| 1/16	|
	*-------*-------*-------*
	|   d	|   e	|   f	|
	|  1/8	|  1/4	|  1/8	|
	*-------*-------*-------*
	|   g	|   h	|   i	|
	| 1/16	|  1/8	| 1/16	|
	*-------*-------*-------*

	*/

	/* loop over the pixels beggining from 2nd pixel of 2nd row */
	for(j=1; j<height-1; j++)
	{
	   in = input + j*width + 1; /* position to 2nd pixel of each row */
	   temp = temporary + j*width + 1; 

	   for(k=1; k<width-1; k++)
	   {
		/* apply the kernel */
		*temp = (*(in+a)>>4) + (*(in+b)>>3) + (*(in+c)>>4) +
		   	(*(in+d)>>3) +   (*(in)>>2) + (*(in+f)>>3) +
			(*(in+g)>>4) + (*(in+h)>>3) + (*(in+i)>>4);

		temp++;
		in++;
	   }

	}

	/* copy temporary convolved buffer to output buffer */
	memcpy(output, temporary, length);
	
	free(temporary);
}



/*----------------------------------------------------------------------------------*/


/*----------------------------*/
/* Create Discontinuity Image */
/*----------------------------*/
void dce8(unsigned char *in, int width, int height, unsigned char *out)
{
        unsigned char *t_in, *t_out, *tbuffer;
        int length, off[9];     /* length of image in pixels, and 8-neighborhood offsets */
		int i, k;
        int j;
        int pixel_value, center_pixel;
        int min, max;   /* minimum and maximum grey-value on a pixel neighborhood */


		length = width*height*sizeof(unsigned char);


		/* create convolution temporary buffer for one grey-level image */
    	if( (tbuffer = (unsigned char *) malloc(length) ) == NULL)
    	{
        	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
        	exit(0);
    	}	



        /* Kernel within each slice:

        *------*------*------*
        |  v4  |  v3  |  v2  |
        *------*------*------*
        |  v5  |  v0  |  v1  |
        *------*------*------*
        |  v6  |  v7  |  v8  |
        *------*------*------*

        Distance to Closest Extremum (DCE) function:

           for a pixel p and its lowest and highest grey-value 8-neighbor,

                DCE(p) = MIN( p-low(p), high(p)-p )


        */
 
 
        /* offsets for the kernel */
        off[4] = -width-1;
        off[3] = -width;
        off[2] = -width+1;
        off[5] = -1;
        off[0] = 0;
        off[1] = 1;
        off[6] = width-1;
        off[7] = width;
        off[8] = width+1;
 
 
 
 
    /* if columns go from 1 to n, loop from column 2 to n-1 */
    for(j=1; j<height-1; j++)
    {
       /* position within the second pixel of each row */
       t_in = in + j*width + 1;
       t_out = tbuffer + j*width + 1;

       for(k=1; k<width-1; k++)
       {
			center_pixel = *t_in;

            /* get the low and high grey-value on the pixel neighborhood */
            min = 50000;
            max = -50000;
			/* get min and max in the neighborhood */
            for(i=0; i<9; i++)
            {
               pixel_value = *(t_in+off[i]);
               if( pixel_value < min) min = pixel_value;
               if( pixel_value > max) max = pixel_value;
            }

            /* get the DCE of the pixel */
            if(center_pixel-min <= max-center_pixel) *t_out = center_pixel-min;
            else *t_out = max-center_pixel;
    
            t_out++;
            t_in++;
       }
    }
	
	/* copy temporary convolved buffer to output buffer */
	memcpy(out, tbuffer, length);
	
	free(tbuffer);
}
 
/*----------------------------------------------------------------------------------*/

/*----------------------------*/
/* Create Discontinuity Image */
/*----------------------------*/
void dce16(unsigned short *in, int width, int height, unsigned short * out)
{
        unsigned short *t_in, *t_out, *tbuffer;
        int length, off[9];     /* length of image in pixels, and 8-neighborhood offsets */
		int i, k;
        int j;
        int pixel_value, center_pixel;
        int min, max;   /* minimum and maximum grey-value on a pixel neighborhood */


		length = width*height*sizeof(unsigned short);


		/* create convolution temporary buffer for one grey-level image */
    	if( (tbuffer = (unsigned short *) malloc(length) ) == NULL)
    	{
        	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
        	exit(0);
    	}	



        /* Kernel within each slice:

        *------*------*------*
        |  v4  |  v3  |  v2  |
        *------*------*------*
        |  v5  |  v0  |  v1  |
        *------*------*------*
        |  v6  |  v7  |  v8  |
        *------*------*------*

        Distance to Closest Extremum (DCE) function:

           for a pixel p and its lowest and highest grey-value 8-neighbor,

                DCE(p) = MIN( p-low(p), high(p)-p )


        */
 
 
        /* offsets for the kernel */
        off[4] = -width-1;
        off[3] = -width;
        off[2] = -width+1;
        off[5] = -1;
        off[0] = 0;
        off[1] = 1;
        off[6] = width-1;
        off[7] = width;
        off[8] = width+1;
 
 
 
 
    /* if columns go from 1 to n, loop from column 2 to n-1 */
    for(j=1; j<height-1; j++)
    {
       /* position within the second pixel of each row */
       t_in = in + j*width + 1;
       t_out = tbuffer + j*width + 1;

       for(k=1; k<width-1; k++)
       {
			center_pixel = *t_in;

            /* get the low and high grey-value on the pixel neighborhood */
            min = 50000;
            max = -50000;
			/* get min and max in the neighborhood */
            for(i=0; i<9; i++)
            {
               pixel_value = *(t_in+off[i]);
               if( pixel_value < min) min = pixel_value;
               if( pixel_value > max) max = pixel_value;
            }

            /* get the DCE of the pixel */
            if(center_pixel-min <= max-center_pixel) *t_out = center_pixel-min;
            else *t_out = max-center_pixel;
    
            t_out++;
            t_in++;
       }
    }
	
	/* copy temporary convolved buffer to output buffer */
	memcpy(out, tbuffer, length);
	
	free(tbuffer);
}
 
/*----------------------------------------------------------------------------------*/

/*---------------------*/
/* Perform Tobogganing */
/*---------------------*/
void toboggan8(unsigned char 	*i_buffer, unsigned char 	*d_buffer, int width, int height, unsigned char 	*t_buffer)
{
	unsigned char *temp_i, *temp_d, *temp_t, *t_i, *t_d, *t_t; /* temporary buffers */
	unsigned char *first;	/* first point of a tobogganing sequence */
	unsigned char *tbuffer;
	char *t_state, *temp_state;
	char *t_state_buffer; 	/* state of each pixel on the toboggan image */
    int length, off[9];     /* length of image in pixels, and 8-neighborhood offsets */
	int j;
	int i, k;
	short	min;		/* value for min disc. value on a neighborhhod */
	short	min_value;	/* value of orig. image on the bottom of the disc. ramp */
	int 	min_index;	/* neighborhood index of min value */
	int  	toboggan_list[1000];
	int  	toboggan_counter;
	short	bottom_flag; /* indicates when to stop going down the 'toboggan' */



		length = width*height*sizeof(unsigned char);

		/* create convolution temporary buffer for one grey-level image */
    	if( (tbuffer = (unsigned char *) malloc(length) ) == NULL)
    	{
        	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
        	exit(0);
    	}	

		/* create convolution temporary buffer for one grey-level image */
    	if( (t_state_buffer = ( char *) malloc(length) ) == NULL)
    	{
        	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
        	exit(0);
    	}	

        /*
        *-------*-------*-------*
        |   4   |   3   |   2   |
        *-------*-------*-------*
        |   5   |   0   |   1   |
        *-------*-------*-------*
        |   6   |   7   |   8   |
        *-------*-------*-------*
        */
        /* offsets for the kernel */
        off[4] = -width-1;
        off[3] = -width;
        off[2] = -width+1;
        off[5] = -1;
        off[0] = 0;
        off[1] = 1;
        off[6] = width-1;
        off[7] = width;
        off[8] = width+1;


	/* sets `t' state image to undefined (-1) (defined=1)*/
	for(i=width*height, t_state=t_state_buffer; i>0; i--, t_state++)
	   *t_state = -1;		

	/* clear `t' buffer */
	for(i=width*height, t_t=tbuffer; i>0; i--, t_t++)
	   *t_t = 0/*255*/;


        /* if columns are indexed from 1 to n, loop from column 2 to n-1 */
        for(j=1; j<height-1; j++)
        {
           /* position within the second pixel of each row */
           t_i = i_buffer + j*width + 1; /* image */
           t_d = d_buffer + j*width + 1; /* discontinuity */
           t_t = tbuffer + j*width + 1; /* tobogganned */
	   	   t_state = t_state_buffer + j*width + 1;  /* toboganned state */
 
	   		/* traverse all pixels */
           for(k=1; k< width-1; k++)
           {


		/* if pixel wasn't tobogganned yet (t-state pixel is undefined (-1) )*/
		if(1 /* *t_state < 0*/)
		{

		   /* reset toboggan list */
		   toboggan_counter = 0; /* don't count first point */

		   /* get the first pixel on the trail (footsteps) */
		   first = t_t;

		   /* temporary pointers to pixels on all images (i, d, and t) */
		   temp_t = t_t;
		   temp_d = t_d;
		   temp_i = t_i;
		   temp_state = t_state;
		   bottom_flag = FALSE; 
		   
		   /* while not yet on the bottom of the disc. ramp */
		   while(bottom_flag == FALSE)
		   {
               	   	/* get the smallest discontinuity pixel on the 8-neighborhood */
                   	min = 500;
                   	for(i=1; i<9; i++)
                   	   if( *(temp_d+off[i]) < min)
			   {
			      min = *(temp_d+off[i]); /* value of smallest neighboor */
			      min_index = i; /* get the offset index of the min neighb. */
			   }
 

		   	/* if pixel in neighborhood has a smaller disc. value */
		   	if(min < *temp_d )
		   	{
			   /* push point to the stack */
			   toboggan_list[toboggan_counter] = min_index;
			   toboggan_counter++;
			   /* toboggan_list contains the trail of pixels to the */
			   /* bottom of the disc. ramp. It stores this trail by */
			   /* keeping the offset to the next pixel. The offset  */
			   /* is stored as an index, i.e., index for the "off"  */
			   /* variable declared in the beggining of this function */

			   /* relocate scope on top of min disc. neighbor */
			   temp_i = temp_i + off[min_index];
			   temp_d = temp_d + off[min_index];
			   temp_t = temp_t + off[min_index];
			   temp_state = temp_state + off[min_index];
		   	}
			else
			{
			   /* we just got to the bottom of the local disc. ramp */
			   bottom_flag = TRUE;

			   /* assign the image value to this last t pixel */
			   *temp_t = *temp_i;
			   /* mark it as defined */
			   *temp_state = 1;
			}

		   }

		   /* get the value of the orig. image on the bottom of the ramp */
		   min_value = *temp_i;

		   /* traverse toboggan list and assign values for 't' image */
		   temp_t = first;
		   *temp_t = (unsigned char)min_value;
		   *temp_state = 1;
		   for(i=0; i<toboggan_counter; i++)
		   {
			/* go to next pixel on the ramp (toboggan list) */
			temp_t = temp_t + off[ toboggan_list[i] ];
			*temp_t = (unsigned char)min_value;

			/* update state image */
			temp_state = temp_state + off[ toboggan_list[i] ];
			*temp_state = 1;
		   }
		
        	}




		/* goto next pixel */
                t_i++;
                t_d++;
                t_t++;
				t_state++;
           }
        }

	/* copy temporary convolved buffer to output buffer */
	memcpy(t_buffer, tbuffer, length);
	
	free(tbuffer);
	free(t_state_buffer);

}



/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/

/*---------------------*/
/* Perform Tobogganing */
/*---------------------*/
void toboggan16(unsigned short 	*i_buffer, unsigned short 	*d_buffer, int width, int height, unsigned short 	*t_buffer)
{
	unsigned short *temp_i, *temp_d, *temp_t, *t_i, *t_d, *t_t; /* temporary buffers */
	unsigned short *first;	/* first point of a tobogganing sequence */
	char *t_state, *temp_state;
	char *t_state_buffer; 	/* state of each pixel on the toboggan image */
	unsigned short *tbuffer;
    int length, off[9];     /* length of image in pixels, and 8-neighborhood offsets */
	int j;
	int i, k;
	short	min;		/* value for min disc. value on a neighborhhod */
	short	min_value;	/* value of orig. image on the bottom of the disc. ramp */
	int 	min_index=0;	/* neighborhood index of min value */
	int  	toboggan_list[1000];
	int  	toboggan_counter;
	short	bottom_flag; /* indicates when to stop going down the 'toboggan' */



		length = width*height*sizeof(unsigned short);

		/* create convolution temporary buffer for one grey-level image */
    	if( (tbuffer = (unsigned short *) malloc(length) ) == NULL)
    	{
        	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
        	exit(0);
    	}	

		/* create convolution temporary buffer for one grey-level image */
    	if( (t_state_buffer = ( char *) malloc(length) ) == NULL)
    	{
        	printf("ERROR allocating temporary grey-level convolved image buffer.\n");
        	exit(0);
    	}	

        /*
        *-------*-------*-------*
        |   4   |   3   |   2   |
        *-------*-------*-------*
        |   5   |   0   |   1   |
        *-------*-------*-------*
        |   6   |   7   |   8   |
        *-------*-------*-------*
        */
        /* offsets for the kernel */
        off[4] = -width-1;
        off[3] = -width;
        off[2] = -width+1;
        off[5] = -1;
        off[0] = 0;
        off[1] = 1;
        off[6] = width-1;
        off[7] = width;
        off[8] = width+1;


	/* sets `t' state image to undefined (-1) (defined=1)*/
	for(i=width*height, t_state=t_state_buffer; i>0; i--, t_state++)
	   *t_state = -1;		

	/* clear `t' buffer */
	for(i=width*height, t_t=tbuffer; i>0; i--, t_t++)
	   *t_t = 0/*255*/;


        /* if columns are indexed from 1 to n, loop from column 2 to n-1 */
        for(j=1; j<height-1; j++)
        {
           /* position within the second pixel of each row */
           t_i = i_buffer + j*width + 1; /* image */
           t_d = d_buffer + j*width + 1; /* discontinuity */
           t_t = tbuffer + j*width + 1; /* tobogganned */
	   	   t_state = t_state_buffer + j*width + 1;  /* toboganned state */
 
	   		/* traverse all pixels */
           for(k=1; k< width-1; k++)
           {


		/* if pixel wasn't tobogganned yet (t-state pixel is undefined (-1) )*/
		if(1 /* *t_state < 0*/)
		{

		   /* reset toboggan list */
		   toboggan_counter = 0; /* don't count first point */

		   /* get the first pixel on the trail (footsteps) */
		   first = t_t;

		   /* temporary pointers to pixels on all images (i, d, and t) */
		   temp_t = t_t;
		   temp_d = t_d;
		   temp_i = t_i;
		   temp_state = t_state;
		   bottom_flag = FALSE; 
		   
		   /* while not yet on the bottom of the disc. ramp */
		   while(bottom_flag == FALSE)
		   {
               	   	/* get the smallest discontinuity pixel on the 8-neighborhood */
                   	min = 500;
                   	for(i=1; i<9; i++)
                   	   if( *(temp_d+off[i]) < min)
			   {
			      min = *(temp_d+off[i]); /* value of smallest neighboor */
			      min_index = i; /* get the offset index of the min neighb. */
			   }
 

		   	/* if pixel in neighborhood has a smaller disc. value */
		   	if(min < *temp_d )
		   	{
			   /* push point to the stack */
			   toboggan_list[toboggan_counter] = min_index;
			   toboggan_counter++;
			   /* toboggan_list contains the trail of pixels to the */
			   /* bottom of the disc. ramp. It stores this trail by */
			   /* keeping the offset to the next pixel. The offset  */
			   /* is stored as an index, i.e., index for the "off"  */
			   /* variable declared in the beggining of this function */

			   /* relocate scope on top of min disc. neighbor */
			   temp_i = temp_i + off[min_index];
			   temp_d = temp_d + off[min_index];
			   temp_t = temp_t + off[min_index];
			   temp_state = temp_state + off[min_index];
		   	}
			else
			{
			   /* we just got to the bottom of the local disc. ramp */
			   bottom_flag = TRUE;

			   /* assign the image value to this last t pixel */
			   *temp_t = *temp_i;
			   /* mark it as defined */
			   *temp_state = 1;
			}

		   }

		   /* get the value of the orig. image on the bottom of the ramp */
		   min_value = *temp_i;

		   /* traverse toboggan list and assign values for 't' image */
		   temp_t = first;
		   *temp_t = min_value;
		   *temp_state = 1;
		   for(i=0; i<toboggan_counter; i++)
		   {
			/* go to next pixel on the ramp (toboggan list) */
			temp_t = temp_t + off[ toboggan_list[i] ];
			*temp_t = min_value;

			/* update state image */
			temp_state = temp_state + off[ toboggan_list[i] ];
			*temp_state = 1;
		   }
		
        	}




		/* goto next pixel */
                t_i++;
                t_d++;
                t_t++;
				t_state++;
           }
        }

	/* copy temporary convolved buffer to output buffer */
	memcpy(t_buffer, tbuffer, length);
	
	free(tbuffer);
	free(t_state_buffer);

}



/*----------------------------------------------------------------------------------*/



/*****************************************************************************
 * FUNCTION: morph_8
 * DESCRIPTION: Applies a morphological filter to a gray image of 8 bits per
 *    pixel.
 * PARAMETERS:
 *    in1, in2, in3: the input data arrays of three input slices
 *    width, height: size of the image in pixels
 *    op: The operation to perform:
 *       5, 9: 2D dilation, structuring element of op pixels 
 *       7, 19, 27: 3D dilation, structuring element of op pixels 
 *       -5, -9: 2D erosion, structuring element of -op pixels 
 *       -7, -19, -27: 3D erosion, structuring element of -op pixels 
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: successful completion
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/14/00 by Dewey Odhner
 *
 *****************************************************************************/
void morph_8( unsigned char *in1, unsigned char *in2, unsigned char *in3, int width, int height, int op, unsigned char *out)
{
	unsigned char *t_in=NULL, *t_out;
	int erode=FALSE, n, xf, yf, x, y, (*neighbor)[3];
	static int neighbor3d[26][3]={
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

	if (op < 0)
	{
		erode = TRUE;
		op = -op;
		memset(out, 0, width);
		for (y=1; y<height-1; y++)
		{
			out[width*y] = out[width*(y+1)-1] = 0;
			memcpy(out+width*y+1, in2+width*y+1, width-2);
		}
		memset(out+width*(height-1), 0, width);
	}
	else
		memcpy(out, in2, width*height);
	op &= ~1;
	switch (op)
	{
		case 4:
		case 8:
			neighbor = neighbor3d+2;
			break;
		default:
			neighbor = neighbor3d;
			break;
	}
	for (n=0; n<op; n++)
	{
		y = neighbor[n][1]<0? 1: 0;
		yf = neighbor[n][1]>0? height-1: height;
		for (; y<yf; y++)
		{
			x = neighbor[n][0]<0? 1: 0;
			xf = neighbor[n][0]>0? width-1: width;
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
		   	t_out = out + y*width+x;
			for (; x<xf; x++,t_in++,t_out++)
				if (erode? *t_in<*t_out: *t_in>*t_out)
					*t_out = *t_in;
		}
	}
	return;
}

/*****************************************************************************
 * FUNCTION: morph_16
 * DESCRIPTION: Applies a morphological filter to a gray image of 16 bits per
 *    pixel.
 * PARAMETERS:
 *    in1, in2, in3: the input data arrays of three input slices
 *    width, height: size of the image in pixels
 *    op: The operation to perform:
 *       5, 9: 2D dilation, structuring element of op pixels 
 *       7, 19, 27: 3D dilation, structuring element of op pixels 
 *       -5, -9: 2D erosion, structuring element of -op pixels 
 *       -7, -19, -27: 3D erosion, structuring element of -op pixels 
 *    out: the output data array
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE:
 *    0: successful completion
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 11/14/00 by Dewey Odhner
 *
 *****************************************************************************/
void morph_16( unsigned short *in1,unsigned short *in2, unsigned short *in3, int width, int height, int op, unsigned short *out)
{
	unsigned short *t_in=NULL, *t_out;
	int erode=FALSE, n, xf, yf, x, y, (*neighbor)[3];
	static int neighbor3d[26][3]={
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

	if (op < 0)
	{
		erode = TRUE;
		op = -op;
		memset(out, 0, width*sizeof(short));
		for (y=1; y<height-1; y++)
		{
			out[width*y] = out[width*(y+1)-1] = 0;
			memcpy(out+width*y+1, in2+width*y+1, (width-2)*sizeof(short));
		}
		memset(out+width*(height-1), 0, width*sizeof(short));
	}
	else
		memcpy(out, in2, width*height*sizeof(short));
	op &= ~1;
	switch (op)
	{
		case 4:
		case 8:
			neighbor = neighbor3d+2;
			break;
		default:
			neighbor = neighbor3d;
			break;
	}
	for (n=0; n<op; n++)
	{
		y = neighbor[n][1]<0? 1: 0;
		yf = neighbor[n][1]>0? height-1: height;
		for (; y<yf; y++)
		{
			x = neighbor[n][0]<0? 1: 0;
			xf = neighbor[n][0]>0? width-1: width;
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
		   	t_out = out + y*width+x;
			for (; x<xf; x++,t_in++,t_out++)
				if (erode? *t_in<*t_out: *t_in>*t_out)
					*t_out = *t_in;
		}
	}
	return;
}
