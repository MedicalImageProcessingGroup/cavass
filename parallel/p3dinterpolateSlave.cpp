/*
  Copyright 1993-2009 Medical Image Processing Group
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

//----------------------------------------------------------------------
/*
file: p3dinterpolateSlave.cpp
*/
//----------------------------------------------------------------------
#include  <assert.h>
#include  <iostream>
#include  <limits.h>
#include  <math.h>
#include  <mpi.h>
#if defined (WIN32) || defined (_WIN32)
    #include  <Winsock2.h>
#else
    #include  <sys/times.h>
    #include  <unistd.h>
#endif
#include  "Viewnix.h"
#include  "cv3dv.h"
#include  "ElapsedTime.h"

#include  "p3dinterpolate.h"

using namespace std;

#define  epsilon  (1e-3)  //for float comparisons for equality
//----------------------------------------------------------------------
static int distance_map ( unsigned char* bin, int xsize, int ysize,
    float* out, int chamfer, float Dx, float Dy );

//global variables:
extern int   execution_mode;  //0=foreground, 1=background

extern int   SlicesPerChunk;  //number of slices in a chunk
extern int   myRank;          //process number
extern char  myName[ MPI_MAX_PROCESSOR_NAME ];  //host name
//----------------------------------------------------------------------
#if 0
    #define say
#else
    static void say ( const char* const msg, const int indentChange=0 ) {
        static int  indentLevel=0;
        if (indentChange==-1)    --indentLevel;
        if (indentLevel<0)       indentLevel=0;
        printf( "%*s slave %d %s \n", 3*indentLevel, " ", ::myRank, msg );
        if (indentChange==1)     ++indentLevel;
    }
                                                                                
    static void say ( const float value, const char* const msg,
                      const int indentChange=0 )
    {
        char  buff[256];
        sprintf( buff, msg, value );
        say( buff, indentChange );
    }
                                                                                
    static void say ( const int value, const char* const msg,
                      const int indentChange=0 )
    {
        char  buff[256];
        sprintf( buff, msg, value );
        say( buff, indentChange );
    }
                                                                                
    static void say ( const long value, const char* const msg,
                      const int indentChange=0 )
    {
        char  buff[256];
        sprintf( buff, msg, value );
        say( buff, indentChange );
    }
#endif
//----------------------------------------------------------------------
/** \brief Given the subscript of an input slice, this function returns
 *  its associated physical location.
 */
static WorkChunk::locType getZLoc ( const int inZSub,
                                    const WorkChunk* const wc ) {
    WorkChunk::locType*  ptr =
        (WorkChunk::locType*) &wc->inData[wc->inZLocationOffset];
    assert( inZSub>=0 && inZSub<wc->inCount[2] );
    return ptr[inZSub];
}
//----------------------------------------------------------------------
/** \brief Given a physical slice location of an input slice, this
 *  function returns the subscript of the slice before or at this location.
 *
 *  \return The integer subscript of the slice is with respect to the 
 *  slices in the chunk and starts with 0.
 */
static int whichInputSlice ( const WorkChunk::locType z_location,
                             const WorkChunk* const wc ) {
    WorkChunk::locType*  ptr =
        (WorkChunk::locType*) &wc->inData[wc->inZLocationOffset];
    int  i=0;
    for (i=0; i<wc->inCount[2]; i++) {
        if (ptr[i] == z_location)    return i;
        if (ptr[i] >= z_location)    break;
    }
    assert( i>0 );
    return i-1;
}
/*****************************************************************************
 * FUNCTION:    interpolate_1d
 * DESCRIPTION: Computes a polynomial interpolant of a single variable.
 * PARAMETERS:
 *    xi: The independent variable mapped to [0, 1] except in quadratic case
 *        mapped to [-alpha, beta].
 *    degree: The degree of the interpolant, 0 to 3.
 *    y0, y1, y2, y3: The sample function values at the following points:
 *       degree    (xi, y)
 *         0       (0, y0), (1, y1)
 *         1       (0, y0), (1, y1)
 *         2       (-alpha, y0), (0, y1), (beta, y2)
 *         3       (-alpha, y0), (0, y1), (1, y2), (1+beta, y3)
 *    alpha: quadratic case:  x1-x0,
 *           cubic case:      (x1-x0)/(x2-x1).
 *    beta:  quadratic case:  x2-x1,
 *           cubic case:      (x3-x2)/(x2-x1).
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The interpolant value at xi.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 12/11/96 by Dewey Odhner
 *
 *****************************************************************************/
static inline double interpolate_1d_nn ( const double xi,
                                         const double y0, const double y1 )
{
    //nearest-neighbor
    return (xi<0.5) ? y0 : y1;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static inline double interpolate_1d_ln ( const double xi,
                                         const double y0, const double y1 )
{
    //linear
    return xi*y1 + (1.0-xi)*y0;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static inline double interpolate_1d_qu ( const double xi,
    const double y0, const double y1, const double y2, const double y3,
    const double alpha, const double beta )
{
    //quadratic
    if (alpha==1.0 && beta==1.0)
         return 0.5*((y2-2.0*y1+y0)*xi+y2-y0)*xi+y1;

    assert( alpha*beta*(alpha+beta) != 0.0 );
    double  d = 1.0 / (alpha*beta*(alpha+beta));
    double  a = d * (alpha*(y2-y1)-beta*(y1-y0));
    double  b = d * (alpha*alpha*(y2-y1)+beta*beta*(y1-y0));
    double  c = y1;
    return (a*xi+b)*xi + c;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static inline double interpolate_1d_cu ( const double xi,
    const double y0, const double y1, const double y2, const double y3,
    const double alpha, const double beta )
{
    //cubic
    assert( xi>=0.0 && xi<=1.0 );
    if (alpha==1.0 && beta==1.0)
        return 0.5*(((y3-3.0*y2+3.0*y1-y0)*xi +
                             (-y3+4.0*y2-5.0*y1+2.0*y0))*xi +
                            (y2-y0))*xi + y1;

    assert( alpha>0.0 && beta>0.0 );
    double  a = (y3-y2)/(beta*(beta+1.0))+(y1-y0)*(1.0/(alpha*(alpha+1.0)))+
                    (alpha/(alpha+1.0)+beta/(beta+1.0)-2.0)*(y2-y1);
    double  c = (alpha*alpha*(y2-y1)+(y1-y0))*(1.0/(alpha*(alpha+1.0)));
    double  b = y2-y1-a-c;
    double  d = y1;
    return ((a*xi+b)*xi+c)*xi+d;
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static inline double interpolate_1d ( const double xi, const int degree,
    const double y0, const double y1 )
{
    switch (degree) {
        case 0:  //nearest neighbor
            return (xi<0.5 ? y0 : y1);
            break;
        case 1:  //linear
            return (xi*y1 + (1.0-xi)*y0);
            break;
        default:
            assert( 0 );
            break;
    }
    return 0;  //should never get here
}
// . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
static inline double interpolate_1d ( const double xi, const int degree,
    const double y0, const double y1, const double y2, const double y3,
    const double alpha, const double beta )
{
    switch (degree) {
        case 0:  //nearest neighbor
            return (xi<0.5 ? y0 : y1);
            break;
        case 1:  //linear
            return (xi*y1 + (1.0-xi)*y0);
            break;
        case 2:  //quadratic
            if (alpha==1.0 && beta==1.0)
                return 0.5*((y2-2.0*y1+y0)*xi+y2-y0)*xi+y1;
            else {
                assert( alpha*beta*(alpha+beta) != 0.0 );
                double  a, b, c, d;
                d = 1.0 / (alpha*beta*(alpha+beta));
                a = d * (alpha*(y2-y1)-beta*(y1-y0));
                b = d * (alpha*alpha*(y2-y1)+beta*beta*(y1-y0));
                c = y1;
                return (a*xi+b)*xi + c;
            }
            break;
        case 3:  //cubic
            assert( xi>=0.0 && xi<=1.0 );
            if (alpha==1.0 && beta==1.0)
                return 0.5*(((y3-3.0*y2+3.0*y1-y0)*xi +
                             (-y3+4.0*y2-5.0*y1+2.0*y0))*xi +
                            (y2-y0))*xi + y1;
            else {
                assert( alpha>0.0 && beta>0.0 );
                double  a, b, c, d;
                a = (y3-y2)/(beta*(beta+1.0))+(y1-y0)*(1.0/(alpha*(alpha+1.0)))+
                    (alpha/(alpha+1.0)+beta/(beta+1.0)-2.0)*(y2-y1);
                c = (alpha*alpha*(y2-y1)+(y1-y0))*(1.0/(alpha*(alpha+1.0)));
                b = y2-y1-a-c;
                d = y1;
                return ((a*xi+b)*xi+c)*xi+d;
            }
            break;
        default:
            assert( 0 );
            break;
    }
    return 0;  //should never get here
}
/*****************************************************************************
 * FUNCTION: abort_ndinterpolate
 * DESCRIPTION: Issues a message and exits the process.
 * PARAMETERS:
 *    code: One of the 3DVIEWNIX error codes
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable execution_mode should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 12/5/96 by Dewey Odhner
 *
 *****************************************************************************/
static void abort_ndinterpolate ( int code ) {
    char msg[200];
                                                                                
    if (execution_mode == 1) {
        VDeleteBackgroundProcessInformation();
        system("job_done ndinterpolate -abort &");
    }
    VDecodeError("p3dinterpolate", "abort_ndinterpolate", code, msg);
    fprintf(stderr, "%s\n", msg);
    exit(1);
}
/*========================================================================= */
#define MAX(a,b)                ((a) > (b) ? a : b)
#define MIN(a,b)                ((a) < (b) ? a : b)

static float MIN5 ( float v, float w, float x, float y, float z ) {
        float a,b,c;

        a = MIN(v,w);
        b = MIN(x,y);
        c = MIN(a,b);
        return( MIN(c,z) );
}

static float MAX5 ( float v, float w, float x, float y, float z ) {
        float a,b,c;

        a = MAX(v,w);
        b = MAX(x,y);
        c = MAX(a,b);
        return( MAX(c,z) );
}
/*========================================================================= */
/* Convert a binary image into an "int" */
/* Modified: 2/7/00 write overflow corrected by Dewey Odhner. */
static void one2eight ( unsigned char* binin, int w, int h,
                        unsigned char* charout )
{
    assert( binin!=NULL );
    assert( charout!=NULL );
    //static const int  mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    static const int  mask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    int  i, j;
    unsigned char  *tin, *tout;
    int  l_8;

    tin  = binin;
    tout = charout;
/*
    mask[7] = 1;
    for (i=6; i>=0; i--)
        mask[i] = mask[i+1] << 1;
*/
    l_8 = w*h/8;

    for (i=0; i<l_8; i++, tin++) {
        for(j=0; j<8; j++, tout++) {
            assert( 0<=j && j<=7 );
            *tout = *tin & mask[j];
        }
    }

    for (j=0; j<w*h%8; j++, tout++) {
        assert( 0<=j && j<=7 );
        *tout = *tin & mask[j];
    }
}
/*========================================================================= */
/* Modified: 8/11/95 initialization corrected by Dewey Odhner */
/* Modified: 12/17/96 float used for output by Dewey Odhner */
/* Chamfer valid only for 1/3. <= Dx/Dy <= 3. */
static int distance_map ( unsigned char* bin, int xsize, int ysize,
                          float* out, int chamfer, float Dx, float Dy )
{
    say( "in distance_map", 1 );
    int	 n, i, j, k, border=2, nrows, ncols;
    static float *slice;
    static unsigned char *bin8;
    static int old_xsize, old_ysize;
    float Dxy, much, halfx, halfy;

    nrows = ysize + border*2;
    ncols = xsize + border*2;

    n =nrows*ncols;

    if (xsize!=old_xsize || ysize!=old_ysize || slice==NULL || bin8==NULL) {
        if (slice)    free(slice);
        if (bin8)     free(bin8);
        slice = NULL;
        bin8 = NULL;

        /* Allocate Memory for the DISTANCE MAP (Padded) */
        if ( (slice = (float *) malloc(n*sizeof(float))) == NULL) {
            printf("ERROR: Memory Allocation Error !\n");
            return 1;
        }

        /* Allocate Memory for the 8bit BINARY */
        if ( (bin8 = (unsigned char *) malloc(xsize*ysize) ) == NULL) {
            free(slice);
            slice = NULL;
            printf("ERROR: Memory Allocation Error !\n");
            return 1;
        }
    }

    old_xsize = xsize;
    old_ysize = ysize;
    Dxy   = sqrt(Dx*Dx+Dy*Dy);
    much  = ncols*Dx+nrows*Dy;
    halfx = Dx/2;
    halfy = Dy/2;

    /* Initialize DISTANCE MAP (Padded) */
    for (j=0; j<n; j++) 
        slice[j] = -much;	

        k = ncols*border+border;
        n = 0;
        /* CONVERT BINARY SLICE INTO an UNSIGNED CHAR SLICE */
        one2eight(bin, xsize, ysize, bin8);

        for (j=0; j<ysize; j++) {
            for (i=0; i<xsize; i++) {   
                slice[k] = bin8[n];
                n++;
                k++;
        }
        k += 2*border;
    }   

    /* initialization */
    for (j=1; j<nrows-1; j++) {
        for (i=1, k=j*ncols+1; i<ncols-1; i++) { 
            if (halfx < halfy)
                if (slice[k] <= 0)
                    if (slice[k+1] > 0 || slice[k-1] > 0)
                        slice[k] = -halfx;
                    else if (slice[k+ncols] > 0 || slice[k-ncols] > 0) 
                        slice[k] = -halfy;
                    else 
                        slice[k] = -much;
                else
                    if (slice[k+1] <= 0 || slice[k-1] <= 0)
                        slice[k] = halfx;
                    else if (slice[k+ncols]<=0 || slice[k-ncols]<= 0) 
                        slice[k] = halfy;
                    else 
                        slice [k] = much;
            else
                if (slice[k] <= 0)
                    if (slice[k+ncols] > 0 || slice[k-ncols] > 0) 
                        slice[k] = -halfy;
                    else if (slice[k+1] > 0 || slice[k-1] > 0)
                        slice[k] = -halfx;
                    else 
                        slice[k] = -much;
                else
                    if (slice[k+ncols]<=0 || slice[k-ncols]<= 0) 
                        slice[k] = halfy;
                    else if (slice[k+1] <= 0 || slice[k-1] <= 0)
                        slice[k] = halfx;
                    else 
                        slice [k] = much;
            k++;
        }
    }


/* forward pass */
    
    for (j=1; j<nrows-1; j++)
       for (i=1; i<ncols-1; i++) 
       {   
         k = j*ncols+i;
         if (chamfer)
         {
            if (slice[k]>0 && slice[k]!=halfx && slice[k]!=halfy)
                slice[k] = MIN5(slice[k-ncols-1]+Dxy, 
                                slice[k-ncols]+Dy,
                                slice[k-ncols+1]+Dxy, 
                                slice[k-1]+Dx, 
                                slice[k]);
            else 
            if (slice[k]<0  && slice[k]!= -halfx && slice[k]!= -halfy)
                slice[k] = MAX5(slice[k-ncols-1]-Dxy, 
                                slice[k-ncols]-Dy,
                                slice[k-ncols+1]-Dxy, 
                                slice[k-1]-Dx, 
                                slice[k]);
         }
         else
            if (slice[k]>0)
            {
                if (slice[k-1]+Dx < slice[k])
                    slice[k] = slice[k-1]+Dx;
                if (slice[k-ncols]+Dy < slice[k])
                    slice[k] = slice[k-ncols]+Dy;
            }
            else 
            {
                if (slice[k-1]-Dx > slice[k])
                    slice[k] = slice[k-1]-Dx;
                if (slice[k-ncols]-Dy > slice[k])
                    slice[k] = slice[k-ncols]-Dy;
            }
       }

/* backward pass */

    for (j=nrows-2; j>0; j--)
       for (i=ncols-2; i>0; i--)
       {  
          k = j*ncols + i;
          if (chamfer)
          {
             if (slice[k]>0 && slice[k]!=halfx && slice[k]!=halfy) 
                slice[k] = MIN5(slice[k], 
                                slice[k+1]+Dx, 
                                slice[k+ncols-1]+Dxy,
                                slice[k+ncols]+Dy, 
                                slice[k+ncols+1]+Dxy);
             else 
             if (slice[k]<0  && slice[k]!= -halfx && slice[k]!= -halfy)
                slice[k] = MAX5(slice[k], 
                                slice[k+1]-Dx, 
                                slice[k+ncols-1]-Dxy,
                                slice[k+ncols]-Dy, 
                                slice[k+ncols+1]-Dxy);
          }
          else
             if (slice[k]>0) 
             {
                if (slice[k+1]+Dx < slice[k])
                    slice[k] = slice[k+1]+Dx;
                if (slice[k+ncols]+Dy < slice[k])
                    slice[k] = slice[k+ncols]+Dy;
             }
             else 
             {
                if (slice[k+1]-Dx > slice[k])
                    slice[k] = slice[k+1]-Dx;
                if (slice[k+ncols]-Dy > slice[k])
                    slice[k] = slice[k+ncols]-Dy;
             }
       }
       
    k = ncols*border+border;
    n = 0;
    for (j=0; j<ysize; j++) 
    {  
        for (i=0; i<xsize; i++)
        {   
            out[n] = slice[k];
            n++;
            k++;
        }
        k += 2*border;
    }   

    say( "out distance_map", -1 );
    return 0;
}
//----------------------------------------------------------------------
/* Convert a float image into a binary */
/* Modified: 2/7/00 read overflow corrected by Dewey Odhner. */
static void dist_to_bin ( float* distin,  //input image
                          int w, int h,   //dimensions of image
                          unsigned char* binout,      //output binary image
                          float value )   //value to threshold the input image
{
    int  mask[8], l_8, i, j, final_value;
    float*  in;
    unsigned char*  out;

    l_8 = w*h / 8;
    in = distin;
    out = binout;

    mask[7] = 1;
    for (i=6; i>=0; i--)
        mask[i] = mask[i+1] << 1;

    for (i=0; i<l_8; i++, out++) {
        final_value = 0;
        for (j=0; j<8; j++,in++)
            if (*in > value)
                final_value += mask[j];
        *out = final_value;
    }

    if (w*h%8) {
        final_value = 0;
        for (j=0; j<w*h%8; j++,in++)
            if (*in > value)
                final_value += mask[j];
        *out = final_value;
    }
}
/*****************************************************************************
 * FUNCTION: interpolate_within_slice (formerly get_slice)
 * DESCRIPTION: interpolates within the slice plane, giving float values.
 * PARAMETERS:
 *    out_data: Output goes here.
 *    in_size_x, in_size_y: Size of the slice in the file in pixels.
 *    pixel_bits: Bits per pixel in the file: 1, 8, or 16.
 *    in_pixel_width, in_pixel_height: Size of an input pixel.
 *    out_size_x, out_size_y: Size of the output slice in pixels.
 *    out_pixel_width, out_pixel_height: Size of an output pixel.
 *    degree_x, degree_y: The degree of the interpolant, 0 1, or 3 in each
 *       direction.
 *    chamfer: Flag to use chamfer distance rather than city-block.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The global variable execution_mode should be set.
 * RETURN VALUE: None
 * EXIT CONDITIONS: On error issues a message and exits the process.
 * HISTORY:
 *    Created:  12/11/96 by Dewey Odhner
 *    Modified: 11-aug-2005 by George Grevera for use in parallel version
 *****************************************************************************/
static void interpolate_within_slice ( float* out_data,
    int in_size_x, int in_size_y,
    const int pixel_bits, float in_pixel_width, float in_pixel_height,
    int out_size_x, int out_size_y,
    float out_pixel_width, float out_pixel_height,
    int degree_x, int degree_y, int chamfer,
    unsigned char* in_data, int* in_pixel_x, int* in_pixel_y,
    float* frac_x, float* frac_y, float* in_float_data, float rel_pixel_width,
    float rel_pixel_height, int in_slice_size )
{
    //say( "in interpolate_within_slice", 1 );

    assert( degree_x==0 || degree_x==1 || degree_x==3 );
    assert( degree_y==0 || degree_y==1 || degree_y==3 );

    //const float      rel_pixel_width  = out_pixel_width  / in_pixel_width;
    //const float      rel_pixel_height = out_pixel_height / in_pixel_height;
    //const int        in_slice_size    = in_size_x * in_size_y;
    int              error_code;
    unsigned short*  in_data_16       = NULL;
    //do we need to interpolate within the slice at all?
    if ( in_size_x==out_size_x && in_size_y==out_size_y &&
         rel_pixel_width==1 && rel_pixel_height==1 )
    {
        //say( "not interpolating within the slice" );
        switch (pixel_bits) {
            case 1:
                //convert binary data to distance map
                error_code = distance_map( in_data, in_size_x, in_size_y,
                    out_data, chamfer, in_pixel_width, in_pixel_height );
                if (error_code)    abort_ndinterpolate( error_code );
                break;
            case 8:
                //convert 8-bit input data to float
                for (long i=0; i<in_size_x*in_size_y; i++)
                    out_data[i] = in_data[i];
                break;
            case 16:
                //convert 16-bit input data to float
                in_data_16 = (unsigned short*)in_data;
                for (long i=0; i<in_size_x*in_size_y; i++)
                    out_data[i] = in_data_16[i];
                break;
            default:
                assert( 0 );
                break;
        }
        //say( "out interpolate_within_slice", -1 );
        return;
    }

    //here if we do need to interpolate within a slice
    //say( "interpolating within the slice" );
    switch (pixel_bits) {
        case 1:
            //convert binary data to distance map
            error_code = distance_map( in_data, in_size_x, in_size_y,
                in_float_data, chamfer, in_pixel_width, in_pixel_height );
            if (error_code)    abort_ndinterpolate( error_code );
            break;
        case 8:
            //convert 8-bit input data to float
            for (long i=0; i<in_size_x*in_size_y; i++) {
                in_float_data[i] = in_data[i];
                //if (in_data[i]!=0)    cout << (int)in_data[i] << " ";
            }
            break;
        case 16:
            //convert 16-bit input data to float
            in_data_16 = (unsigned short*)in_data;
            for (long i=0; i<in_size_x*in_size_y; i++) {
                in_float_data[i] = in_data_16[i];
                //if (in_data_16[i]!=0)    cout << (int)in_data_16[i] << " ";
            }
            break;
        default:
            assert( 0 );
            break;
    }

    //interpolate within the slice.
    for (int row=0; row<out_size_y; row++) {
        for (int col=0; col<out_size_x; col++) {
            float  f[4];
            switch (degree_x) {
                case 0:
                case 1:
                    switch (degree_y) {
                        case 0:
                        case 1:
                            f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                in_float_data[ in_size_x*in_pixel_y[row]+
                                               in_pixel_x[col] ],
                                in_float_data[ in_size_x*in_pixel_y[row]+
                                               in_pixel_x[col]+1 ],
                                0., 0., 0., 0.);
                            f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                in_pixel_x[col]],
                                in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                in_pixel_x[col]+1], 0., 0., 0., 0.);
                            out_data[out_size_x*row+col] =
                                (float) interpolate_1d(frac_y[row], degree_y,
                                f[0], f[1], 0., 0., 0., 0.);
                            break;
                        case 3:
                            if (in_pixel_y[row] == 0) {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_pixel_x[col]],
                                    in_float_data[in_pixel_x[col]+1],
                                    0., 0., 0., 0.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x+in_pixel_x[col]],
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]+1], 0., 0., 0., 0.);
                                f[2] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*2+in_pixel_x[col]],
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]+1], 0., 0., 0., 0.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row]-1, 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                            } else if (in_pixel_y[row] == in_size_y-2) {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1], 0., 0., 0., 0.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1], 0., 0., 0., 0.);
                                f[2] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1], 0., 0., 0., 0.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                            } else {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1], 0., 0., 0., 0.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1], 0., 0., 0., 0.);
                                f[2] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1], 0., 0., 0., 0.);
                                f[3] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]+1], 0., 0., 0., 0.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 3,
                                    f[0], f[1], f[2], f[3], 1., 1.);
                            }
                            break;
                    }
                    break;
                case 3:
                    switch (degree_y) {
                        case 0:
                        case 1:
                            if (in_pixel_x[col] == 0) {
                                f[0] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_size_x*in_pixel_y[row]],
                                    in_float_data[in_size_x*in_pixel_y[row]+1],
                                    in_float_data[in_size_x*in_pixel_y[row]+2],
                                    0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col]-1, 2,
                                    in_float_data[
                                    in_size_x*(in_pixel_y[row]+1)],
                                    in_float_data[
                                    in_size_x*(in_pixel_y[row]+1)+1],
                                    in_float_data[in_size_x*
                                    (in_pixel_y[row]+1)+2], 0., 1., 1.);
                            } else if (in_pixel_x[col] == in_size_x-2) {
                                f[0] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1], 0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1], 0., 1., 1.);
                            } else {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+2], 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+2], 1., 1.);
                            }
                            out_data[out_size_x*row+col] =
                                (float) interpolate_1d(frac_y[row], degree_y,
                                f[0], f[1], 0., 0., 0., 0.);
                            break;
                        case 3:
                            if (in_pixel_x[col] == 0) {
                              if (in_pixel_y[row] == 0) {
                                f[0] = (float) interpolate_1d(frac_x[col]-1, 2,
                                    in_float_data[in_pixel_x[col]],
                                    in_float_data[in_pixel_x[col]+1],
                                    in_float_data[in_pixel_x[col]+2],
                                    0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col]-1, 2,
                                    in_float_data[in_size_x+in_pixel_x[col]],
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]+2], 0., 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col]-1, 2,
                                    in_float_data[in_size_x*2+in_pixel_x[col]],
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]+2], 0., 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row]-1, 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                              } else if (in_pixel_y[row] == in_size_y-2) {
                                f[0] = (float) interpolate_1d(frac_x[col]-1, 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+2], 0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col]-1, 2,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+2], 0., 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col]-1, 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+2], 0., 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                              } else {
                                f[0] = (float) interpolate_1d(frac_x[col]-1, 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+2], 0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col]-1, 2,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+2], 0., 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col]-1, 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+2], 0., 1., 1.);
                                f[3] = (float) interpolate_1d(frac_x[col]-1, 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]+2], 0., 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 3,
                                    f[0], f[1], f[2], f[3], 1., 1.);
                              }
                            } else if (in_pixel_x[col] == in_size_x-2) {
                              if (in_pixel_y[row] == 0) {
                                f[0] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_pixel_x[col]-1],
                                    in_float_data[in_pixel_x[col]],
                                    in_float_data[in_pixel_x[col]+1],
                                    0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x+in_pixel_x[col]],
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]+1], 0., 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*2+in_pixel_x[col]],
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]+1], 0., 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row]-1, 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                              } else if (in_pixel_y[row] == in_size_y-2) {
                                f[0] = (float) interpolate_1d(frac_x[col], 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1], 0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1], 0., 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col], 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1], 0., 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                              } else {
                                f[0] = (float) interpolate_1d(frac_x[col], 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1], 0., 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], 2,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1], 0., 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col], 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1], 0., 1., 1.);
                                f[3] = (float) interpolate_1d(frac_x[col], 2,
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]+1], 0., 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 3,
                                    f[0], f[1], f[2], f[3], 1., 1.);
                              }
                            } else {
                              if (in_pixel_y[row] == 0) {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_pixel_x[col]-1],
                                    in_float_data[in_pixel_x[col]],
                                    in_float_data[in_pixel_x[col]+1],
                                    in_float_data[in_pixel_x[col]+2],
                                    1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x+in_pixel_x[col]],
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x+
                                    in_pixel_x[col]+2], 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*2+in_pixel_x[col]],
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*2+
                                    in_pixel_x[col]+2], 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row]-1, 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                              } else if (in_pixel_y[row] == in_size_y-2) {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+2], 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+2], 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+2], 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 2,
                                    f[0], f[1], f[2], 0., 1., 1.);
                              } else {
                                f[0] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]-1)+
                                   in_pixel_x[col]+2], 1., 1.);
                                f[1] = (float) interpolate_1d(frac_x[col], degree_x,
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]-1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+1],
                                    in_float_data[in_size_x*in_pixel_y[row]+
                                    in_pixel_x[col]+2], 1., 1.);
                                f[2] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+1)+
                                   in_pixel_x[col]+2], 1., 1.);
                                f[3] = (float) interpolate_1d(frac_x[col], degree_x,
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]-1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]+1],
                                   in_float_data[in_size_x*(in_pixel_y[row]+2)+
                                   in_pixel_x[col]+2], 1., 1.);
                                out_data[out_size_x*row+col] =
                                    (float) interpolate_1d(frac_y[row], 3,
                                    f[0], f[1], f[2], f[3], 1., 1.);
                              }
                            }
                            break;
                    }
                    break;
            }  //end switch degree_x
        }  //end for col
    }  //end for row
    //say( "out interpolate_within_slice", -1 );
}
//----------------------------------------------------------------------
//cache of up to 4 slices (4 for cubic interpolation)
#define  CacheSize  4

class Cache {
    public:
        int     mInW, mInH, mOutW, mOutH;
        float*  mInSlice;
        float*  mOutSlice;
        float   mInLoc,  mOutLoc;
        int     mInZSub, mOutZSub;
        bool    mMovedToOutput, mInitialized;

        void print ( void ) {
            cout << "Cache " << this << " mInW=" << mInW << " mInH=" << mInH
                 << " mOutW=" << mOutW << " mOutH=" << mOutH
                 << " mInSlice=" << mInSlice << " mOutSlice=" << mOutSlice
                 << " mInLoc=" << mInLoc << " mOutLoc=" << mOutLoc
                 << " mInZSub=" << mInZSub << " mOutZSub=" << mOutZSub
                 << " mMovedToOutPut=" << mMovedToOutput
                 << " mInialized=" << mInitialized << endl;
        }

        Cache ( ) {
            //cout << "Cache::Cache(): (" << getpid() << " " << ::myRank << ") " << this << endl;
            mInW = mInH = mOutW = mOutH = mInZSub = mOutZSub = -1;
            mInLoc   = mOutLoc   = -1;
            mInSlice = mOutSlice = NULL;
            mMovedToOutput = mInitialized = false;
        }

        void setup ( const int inW,  const int inH,
                     const int outW, const int outH ) {
            //cout << "Cache::setup(): (" << ::myRank << ") " << this << endl;

            if (inW != mInW || inH != mInH) {
                mInW = inW;
                mInH = inH;
                if (mInSlice!=NULL)    free( mInSlice );
                mInSlice  = (float*)malloc( inW*inH*sizeof(float) );
            }
            assert( mInSlice!=NULL );

            if (outW != mOutW || outH != mOutH) {
                mOutW = outW;
                mOutH = outH;
                if (mOutSlice!=NULL)    free( mOutSlice );
                mOutSlice = (float*)malloc( outW*outH*sizeof(float) );
            }
            assert( mOutSlice!=NULL );
        }

        ~Cache ( ) {
            //cout << "Cache::~Cache(): (" << ::myRank << ") " << this << endl;
            if (mInSlice != NULL) {
                free( mInSlice );
                mInSlice = NULL;
            }
            if (mOutSlice != NULL) {
                free( mOutSlice );
                mOutSlice = NULL;
            }
        }
};
//----------------------------------------------------------------------
static int cacheInputSlice ( Cache** cache,
    const int inZ, const WorkChunk* const workChunk,
    int* in_pixel_x, int* in_pixel_y, float* frac_x, float* frac_y,
    const double rel_pixel_width, const double rel_pixel_height,
    const int in_slice_size )
{
    //is it already in our cache?
    for (int i=0; i<CacheSize; i++) {
        if (cache[i]->mInZSub == inZ) {
            return i;
        }
    }
    //it's not already in the cache so put it in.
    //first, find space in the cache.
    int  where = -1;
    for (int i=0; i<CacheSize; i++) {
        if (!cache[i]->mInitialized) {  where=i;  break;  }
    }
    //any free space in the cache?
    if (where==-1) { //no, so shuffle and use the last space
        //make a copy of the first as it will be overwritten
        Cache*  tmp = cache[0];
        for (int i=0; i<CacheSize-1; i++) {
            cache[i] = cache[i+1];
        }
        where = CacheSize-1;
        //copy the (old) first to the last position
        cache[where] = tmp;
    }
    //calc offset to input slice in bytes
//float*  mInSlice;
//float*  mOutSlice;
//float   mInLoc, mOutLoc;
//int     mInZSub, mOutZSub;
//bool    mMovedToOutput, mInitialized;
    cache[where]->mInLoc   = getZLoc( inZ, workChunk );
    cache[where]->mOutLoc  = cache[where]->mInLoc;  //doesn't move
    cache[where]->mInZSub  = inZ;
    cache[where]->mOutZSub = -1;  //don't know (probably don't care)
    cache[where]->mMovedToOutput = false;
    cache[where]->mInitialized   = true;

    const long  inSliceOffset =
        inZ * workChunk->inCount[0] * workChunk->inCount[1]
        * ::dataTypeSizes[ workChunk->inDataType ] + workChunk->inDataOffset;
    interpolate_within_slice ( cache[where]->mOutSlice,
        workChunk->inCount[0],  workChunk->inCount[1],
        ::dataTypeBits[ workChunk->inDataType ],
        (float) workChunk->inSize[0],   (float) workChunk->inSize[1],
        workChunk->outCount[0], workChunk->outCount[1],
        (float) workChunk->outSize[0],  (float) workChunk->outSize[1],
        workChunk->degree[0],   workChunk->degree[1],
        workChunk->chamferFlag,
        (unsigned char*) &workChunk->inData[ inSliceOffset ],
        in_pixel_x, in_pixel_y, frac_x, frac_y, cache[where]->mInSlice,
        (float) rel_pixel_width, (float) rel_pixel_height, in_slice_size );
    return where;
}
//----------------------------------------------------------------------
static void moveToResult ( ResultChunk* resultChunk, const int outZ,
                           const float* const tempOutSlice ) {
    //convert from float (tempOutSlice) back to result data type
    // and save results
    const long  count = resultChunk->count[0] * resultChunk->count[1];
    const long  outSliceOffset =  //calc offset to output slice in bytes
        outZ * resultChunk->count[0] * resultChunk->count[1]
        * ::dataTypeSizes[ resultChunk->dataType ];
    void*  vptr = (void*) &resultChunk->data[ outSliceOffset ];
    switch (resultChunk->dataType) {
        case ResultChunk::INT1:
            {
                dist_to_bin( (float*)tempOutSlice, resultChunk->count[0],
                             resultChunk->count[1], (unsigned char*)vptr, 0 );
                resultChunk->min = 0;
                resultChunk->max = 1;
            }
            break;
        case ResultChunk::INT8:
            {
                char*  ptr = (char*)vptr;
                for (int i=0; i<count; i++) {
                    //clamp the result values
                    const double  temp = tempOutSlice[i] + 0.5;  //round
                    if      (temp<SCHAR_MIN)    ptr[i] = SCHAR_MIN;
                    else if (temp>SCHAR_MAX)    ptr[i] = SCHAR_MAX;
                    else                        ptr[i] = (char)temp;
                    if (ptr[i] < resultChunk->min)    resultChunk->min = ptr[i];
                    if (ptr[i] > resultChunk->max)    resultChunk->max = ptr[i];
                }
            }
            break;
        case ResultChunk::INT16:
            {
                short*  ptr = (short*)vptr;
                for (int i=0; i<count; i++) {
                    //clamp the result values
                    const double  temp = tempOutSlice[i] + 0.5;  //round
                    if      (temp<SHRT_MIN)    ptr[i] = SHRT_MIN;
                    else if (temp>SHRT_MAX)    ptr[i] = SHRT_MAX;
                    else                       ptr[i] = (short)temp;
                    if (ptr[i] < resultChunk->min)    resultChunk->min = ptr[i];
                    if (ptr[i] > resultChunk->max)    resultChunk->max = ptr[i];
                }
            }
            break;
        case ResultChunk::UINT8:
            {
                //static int  sl=0;
                //bool  foundNonZero=false;
                //int   zeroCount=0;
                unsigned char*  ptr = (unsigned char*)vptr;
                for (int i=0; i<count; i++) {
                    //clamp the result values
                    const double  temp = tempOutSlice[i] + 0.5;  //round
                    if      (temp<0)            ptr[i] = 0;
                    else if (temp>UCHAR_MAX)    ptr[i] = UCHAR_MAX;
                    else                        ptr[i] = (unsigned char)temp;
                    if (ptr[i] < resultChunk->min)    resultChunk->min = ptr[i];
                    if (ptr[i] > resultChunk->max)    resultChunk->max = ptr[i];
                    //if (ptr[i]!=0)    foundNonZero = true;
                    //if (ptr[i]==0)    ++zeroCount;
                }
                //cout << "slice=" << sl << " foundNonZero=" << foundNonZero
                //         << " zeroCount=" << zeroCount << "/" << count << endl;
                //sl++;
            }
            break;
        case ResultChunk::UINT16:
            {
                unsigned short*  ptr = (unsigned short*)vptr;
                for (int i=0; i<count; i++) {
                    //clamp the result values
                    const double  temp = tempOutSlice[i] + 0.5;  //round
                    if      (temp<0)            ptr[i] = 0;
                    else if (temp>USHRT_MAX)    ptr[i] = USHRT_MAX;
                    else                        ptr[i] = (unsigned short)temp;
                    if (ptr[i] < resultChunk->min)    resultChunk->min = ptr[i];
                    if (ptr[i] > resultChunk->max)    resultChunk->max = ptr[i];
                }
            }
            break;
        default:
            assert( 0 );
            break;
    }  //end switch
}
//----------------------------------------------------------------------
static void interpolateBetweenSlices ( Cache** cache,
    const WorkChunk::locType neededZLoc,
    const int cacheSub, const WorkChunk* const workChunk, float* tempOutSlice )
{
    const double  prevWhere = cache[cacheSub]->mInLoc;
    assert( cacheSub < CacheSize-1 );
    const double  nextWhere = cache[cacheSub+1]->mInLoc;
    assert( prevWhere<=neededZLoc && neededZLoc<=nextWhere );
    const double  xi = (neededZLoc - prevWhere) / (nextWhere - prevWhere);
    switch ( workChunk->degree[2] ) {
        case WorkChunk::D_QU:
            assert( 0 );
            break;
        case WorkChunk::D_CU:
            if (cacheSub==1) {
                assert( cache[0]->mInitialized && cache[3]->mInitialized );
                const double  prevPrevWhere = cache[0]->mInLoc;
                const double  nextNextWhere = cache[3]->mInLoc;
                const double  alpha = (prevWhere-prevPrevWhere)
                                    / (nextWhere-prevWhere);
                const double  beta  = (nextNextWhere-nextWhere)
                                    / (nextWhere-prevWhere);
                for (int i=0; i<workChunk->outCount[0]*workChunk->outCount[1];
                                                                          i++)
                {
                    tempOutSlice[i] = (float) interpolate_1d_cu( xi,
                        cache[0]->mOutSlice[i],
                        cache[1]->mOutSlice[i],
                        cache[2]->mOutSlice[i],
                        cache[3]->mOutSlice[i], alpha, beta );
                }
                break;
            }
            //fall through to linear if we don't have 4 slices for cubic
        case WorkChunk::D_LN:
            for (int i=0; i<workChunk->outCount[0]*workChunk->outCount[1]; i++)
            {
                tempOutSlice[i] = (float) interpolate_1d_ln( xi,
                    cache[cacheSub]->mOutSlice[i],
                    cache[cacheSub+1]->mOutSlice[i] );
            }
            break;
        case WorkChunk::D_NN:
            for (int i=0; i<workChunk->outCount[0]*workChunk->outCount[1]; i++)
            {
                tempOutSlice[i] = (float) interpolate_1d_nn( xi,
                    cache[cacheSub]->mOutSlice[i],
                    cache[cacheSub+1]->mOutSlice[i] );
            }
            break;
        default:    assert( 0 );    break;
    }  //end switch
}
//----------------------------------------------------------------------
/** \brief main entry point for slave (worker) process.
 */
void slave ( void ) {
    Cache**  cache = (Cache**) malloc( CacheSize * sizeof( Cache* ) );
    assert( cache != NULL );
    for (int i=0; i<CacheSize; i++) {
        cache[i] = new Cache();
        assert( cache[i] != NULL );
    }

#if 0  //only when debugging with: gdb program pid
    int  kk=1;
    while (kk==1) {
        cout << "slave=" << ::myRank << " pid=" << getpid() << endl;
        sleep( 5 );
    }
#endif
    cout << "    slave " << ::myRank << ": " << ::myName << ": hello" << endl;
    //for each message . . .
    for ( ; ; ) {
        //wait for a message (first, get new message length)
        MPI_Status  status;
        MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
#ifdef Verbose
        //cout << "    slave " << ::myRank << ": receiving; length is "
        //     << status.st_length << endl;
#endif
//        if (status.st_length<=0)    continue;
        int  st_length = 0;
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
#ifdef Verbose
        cout << "    slave " << ::myRank << ": receiving; length is " << st_length << endl;
#endif
        if (st_length <= 0)    continue;

        WorkChunk*  workChunk = (WorkChunk*)malloc( st_length );
        assert( workChunk!=NULL );
        //receive a message from the master
        MPI_Recv( workChunk, st_length, MPI_UNSIGNED_CHAR, 0,
                  MPI_ANY_TAG, MPI_COMM_WORLD, &status );
        if (status.MPI_TAG == WorkChunk::OP_EXIT) {
#ifdef Verbose
            MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
            cout << "    slave " << ::myRank << ": received; length is "
                 << st_length << " command is exit." << endl;
            workChunk->print();
#endif
            break;
        }
        if (status.MPI_TAG != WorkChunk::OP_INTERPOLATE) {
            cout << "    slave " << ::myRank << ": " << ::myName
                 << ": unrecognized command." << endl;
            workChunk->print();
            continue;
        }
#ifdef Verbose
        MPI_Get_count( &status, MPI_UNSIGNED_CHAR, &st_length );
        cout << "    slave " << ::myRank << ": received; length is "
             << st_length << endl;
        workChunk->print();
#endif
#if 0
        {
        //check for a first slice of all zeros
        bool  firstSliceIsAllZeros = true;
        const int  bytes = workChunk->inCount[0] * workChunk->inCount[1] *
                           ::dataTypeSizes[ workChunk->inDataType ];
        unsigned char*  inPtr  = (unsigned char*)workChunk->inData;
        if (workChunk->inDataOffset>0)    inPtr += workChunk->inDataOffset;
        for (int i=0; i<bytes; i++) {
            if (*inPtr++ != 0) {
                firstSliceIsAllZeros = false;
                cout << "i=" << i << endl;
                break;
            }
        }
        if (firstSliceIsAllZeros) {
            cout << "    slave " << ::myRank << ": first slice is all zeros."
                 << endl;
        } else {
            cout << "    slave " << ::myRank << ": first slice is not all zeros."
                 << endl;
        }
        }
#endif
        //start timers
        ElapsedTime  et;
//        struct tms   usageBefore;
//        times( &usageBefore );

        //perform interpolation

        //make sure that we can allocate the result size
        double  d = workChunk->outCount[0];
        d *= workChunk->outCount[1];
        d *= workChunk->outCount[2];
        d *= ::dataTypeSizes[ workChunk->outDataType ];
        d += sizeof(ResultChunk);
        assert( d<=LONG_MAX );

        const long  resultSize = workChunk->outCount[0]
                               * workChunk->outCount[1]
                               * workChunk->outCount[2] 
                               * ::dataTypeSizes[ workChunk->outDataType ]
                               + sizeof(ResultChunk);
#ifdef Verbose
        cout << "    slave " << ::myRank << ": allocating " << resultSize
             << " for result." << endl;
#endif
        ResultChunk*  resultChunk = (ResultChunk*) malloc( resultSize );
        assert( resultChunk!=NULL );
        resultChunk->totalBytes  = resultSize;
        resultChunk->operation   = ResultChunk::OP_RESULTS;
        resultChunk->count[0]    = workChunk->outCount[0];
        resultChunk->count[1]    = workChunk->outCount[1];
        resultChunk->count[2]    = workChunk->outCount[2];
        resultChunk->size[0]     = workChunk->outSize[0];
        resultChunk->size[1]     = workChunk->outSize[1];
        resultChunk->size[2]     = workChunk->outSize[2];
        resultChunk->firstZSlice = workChunk->outFirstZSlice;
        resultChunk->min         = LONG_MAX;
        resultChunk->max         = LONG_MIN;

        //make sure we have all of the input z locations (one for each input
        // slice)
        assert( workChunk->inZLocationOffset >= 0 );  //at least one
        assert( workChunk->inDataOffset >= 0 );       //at least one
        assert( workChunk->inZLocationOffset != workChunk->inDataOffset );
        //check that there is one z location for each slice
        if (workChunk->inZLocationOffset < workChunk->inDataOffset) {
            assert( workChunk->inCount[2]
                    <= (workChunk->inDataOffset-workChunk->inZLocationOffset)
                           / sizeof(WorkChunk::locType) );
        } else {
            assert( workChunk->inCount[2]
                    <= (workChunk->inZLocationOffset-workChunk->inDataOffset)
                           / sizeof(WorkChunk::locType) );
        }

        WorkChunk::locType*  ptr = (WorkChunk::locType*)
            &workChunk->inData[ workChunk->inZLocationOffset ];
        resultChunk->firstZLocation = workChunk->outFirstZLoc;  // was ptr[0];
        resultChunk->lastZLocation  = workChunk->outLastZLoc;
        resultChunk->computeTime    = 0;
        resultChunk->cpuTime        = 0;
        resultChunk->dataType       = workChunk->outDataType;

        //allocate space for temporary input and output float data slice
        const int  tempOutSliceSize = workChunk->outCount[0]
                                    * workChunk->outCount[1] * sizeof(float);

        const int  in_slice_size = workChunk->inCount[0] * workChunk->inCount[1];
        const double  rel_pixel_width  = workChunk->outSize[0]
                                       / workChunk->inSize[0];
        const double  rel_pixel_height = workChunk->outSize[1]
                                       / workChunk->inSize[1];
        int*  in_pixel_x = (int*)malloc( workChunk->outCount[0]*sizeof(int) );
        assert( in_pixel_x!=NULL );
        int*  in_pixel_y = (int*)malloc( workChunk->outCount[1]*sizeof(int) );
        assert( in_pixel_y!=NULL );
        float*  frac_x = (float*)malloc( workChunk->outCount[0] * sizeof(float) );
        assert( frac_x!=NULL );
        float*  frac_y = (float*)malloc( workChunk->outCount[1] * sizeof(float) );
        assert( frac_y!=NULL );

//todo: why is it in_pixel_x[row] and not in_pixel_x[col]?
        for (int row=0; row<workChunk->outCount[0]; row++) {
//end todo
            in_pixel_x[row] = (int)(row*rel_pixel_width);  //should be rounded?
            if (in_pixel_x[row] > workChunk->inCount[0]-2)
                in_pixel_x[row] = workChunk->inCount[0]-2;
            frac_x[row] = (float) (row*rel_pixel_width-in_pixel_x[row]);
        }

//todo: why is it in_pixel_y[col] and not in_pixel_y[row]?
        for (int col=0; col<workChunk->outCount[1]; col++) {
//end todo
            in_pixel_y[col] = (int)(col*rel_pixel_height);  //should be rounded?
            if (in_pixel_y[col] > workChunk->inCount[1]-2)
                in_pixel_y[col] = workChunk->inCount[1]-2;
            frac_y[col] = (float) (col*rel_pixel_height-in_pixel_y[col]);
        }

        //allocate cache slices
        for (int i=0; i<CacheSize; i++) {
            cache[i]->setup( workChunk->inCount[0],  workChunk->inCount[1],
                            workChunk->outCount[0], workChunk->outCount[1] );
        }

        //allocate an additional, temporary output slice
        float*  tempOutSlice = (float*)malloc( tempOutSliceSize );
        assert( tempOutSlice!=NULL );

        WorkChunk::locType  neededZLoc = workChunk->outFirstZLoc;
        for (int outZ=0; outZ<workChunk->outCount[2]; outZ++) {
//workChunk->print();
//cout << "\nneededZLoc=" << neededZLoc << " outLastZLoc=" << workChunk->outLastZLoc << " diff=" << (workChunk->outLastZLoc-neededZLoc) << endl;
            assert( neededZLoc <= (workChunk->outLastZLoc+epsilon) );

            //make sure everything we need is in our cache

            int  inZ = whichInputSlice( neededZLoc, workChunk );
            //for cubic, we'll need the previous slice
            if (workChunk->degree[2] == WorkChunk::D_CU && inZ>0) {
                cacheInputSlice( cache,
                    inZ-1, workChunk, in_pixel_x, in_pixel_y,
                    frac_x, frac_y, rel_pixel_width, rel_pixel_height,
                    in_slice_size );
            }
            //make sure it's in out cache
            cacheInputSlice( cache,
                inZ, workChunk, in_pixel_x, in_pixel_y,
                frac_x, frac_y, rel_pixel_width, rel_pixel_height,
                in_slice_size );
            //we'll need the next slice as well
            if (inZ < workChunk->inCount[2]-1) {
                cacheInputSlice( cache,
                    inZ+1, workChunk, in_pixel_x, in_pixel_y,
                    frac_x, frac_y, rel_pixel_width, rel_pixel_height,
                    in_slice_size );
            }
            //and for cubic, we'll need the next+1 slice as well
            if ( workChunk->degree[2] == WorkChunk::D_CU
              && inZ < workChunk->inCount[2]-2) {
                cacheInputSlice( cache,
                    inZ+2, workChunk, in_pixel_x, in_pixel_y,
                    frac_x, frac_y, rel_pixel_width, rel_pixel_height,
                    in_slice_size );
            }

            //a cache miss might have caused a shuffle so we need to obtain
            // the index now (again, in this order or a shuffle might move it).
            const int  cacheSub = cacheInputSlice( cache,
                inZ, workChunk,
                in_pixel_x, in_pixel_y,
                frac_x, frac_y, rel_pixel_width, rel_pixel_height,
                in_slice_size );

            //at this point, we have everything that we need in our cache
            //for (int i=0; i<CacheSize; i++) {
            //    cache[i].print();
            //}

            //check for the special case of having the exact position that
            // we need.
            //if (cache[cacheSub].mInLoc == neededZLoc) {
            if (fabs(cache[cacheSub]->mInLoc-neededZLoc)<epsilon) {
//cout << "slave: exact!  outZ=" << outZ << endl;
                //we have exactly what we need (in the cache).
                moveToResult( resultChunk, outZ, cache[cacheSub]->mOutSlice );
            } else {
                //we don't have exactly what we need (in the cache - because
                // the desired output slice location is between the slices
                // that we have in input), so we need to interpolate it.
//cout << "slave: not exact!  outZ=" << outZ << " neededZLoc=" << neededZLoc << " cache=" << cache[cacheSub].mInLoc << " diff=" << (neededZLoc-cache[cacheSub].mInLoc) << endl;
                interpolateBetweenSlices( cache, neededZLoc, cacheSub, workChunk,
                                          tempOutSlice );
                moveToResult( resultChunk, outZ, tempOutSlice );
            }

            neededZLoc += (float) workChunk->outSize[2];
        }

        free( frac_y );
        frac_y = NULL;
        free( frac_x );
        frac_x = NULL;
        free( in_pixel_y );
        in_pixel_y = NULL;
        free( in_pixel_x );
        in_pixel_x = NULL;
        //free space for temporary output float data slice
        free( tempOutSlice );
        tempOutSlice = NULL;

        //stop timers and report time
//        struct tms  usageAfter;
//        times( &usageAfter );
//        resultChunk->cpuTime = ((double)usageAfter.tms_utime
//            - usageBefore.tms_utime) / sysconf(_SC_CLK_TCK);
        resultChunk->computeTime = et.getElapsedTime();
#ifdef Verbose
        cout << "    slave " << ::myRank << ": compute time="
             << resultChunk->computeTime << endl;
        cout << "    slave " << ::myRank << ": cpu time="
             << resultChunk->cpuTime << endl;
#endif
        //send results back to master
#ifdef Verbose
        cout << "    slave " << ::myRank << ": sending." << endl;
#endif
#if 0
        {
        //simply copy the input data to the output data and send it back.
        cout << "    slave " << ::myRank << ": echo test." << endl;
        assert( workChunk->inCount[0]==resultChunk->count[0] &&
                workChunk->inCount[1]==resultChunk->count[1] &&
                workChunk->inCount[2]==resultChunk->count[2] );
        assert( workChunk->inDataType==workChunk->outDataType );
        assert( workChunk->outDataType==resultChunk->dataType );
        unsigned char*  inPtr  = (unsigned char*)workChunk->inData;
        if (workChunk->inDataOffset>0)    inPtr += workChunk->inDataOffset;
        unsigned char*  outPtr = (unsigned char*)resultChunk->data;
        if (true) {  //copy input to output
            const int  bytes = workChunk->inCount[0] * workChunk->inCount[1] *
                               workChunk->inCount[2] *
                               ::dataTypeSizes[workChunk->inDataType];
            for (int i=0; i<bytes; i++)    *outPtr++ = *inPtr++;
        } else {  //make output each output slice z+10
            assert( ::dataTypeSizes[workChunk->inDataType]==1 );
            const int  bytes = workChunk->inCount[0] * workChunk->inCount[1] *
                               ::dataTypeSizes[workChunk->inDataType];
            for (int z=0; z<workChunk->inCount[2]; z++) {
                for (int i=0; i<bytes; i++)    *outPtr++ = z+10;
            }
        }  //end else
        }  //end block
#endif
#if 0
        {
        //calc diff between input and output
        assert( workChunk->inCount[0]==resultChunk->count[0] &&
                workChunk->inCount[1]==resultChunk->count[1] &&
                workChunk->inCount[2]==resultChunk->count[2] );
        assert( workChunk->inDataType==workChunk->outDataType );
        assert( workChunk->outDataType==resultChunk->dataType );
        assert( ::dataTypeSizes[workChunk->inDataType]==1 );

        unsigned char*  inPtr  = (unsigned char*)workChunk->inData;
        if (workChunk->inDataOffset>0)    inPtr += workChunk->inDataOffset;
        unsigned char*  outPtr = (unsigned char*)resultChunk->data;
        const int  bytes = workChunk->inCount[0] * workChunk->inCount[1] *
                               ::dataTypeSizes[workChunk->inDataType];
        for (int z=0; z<workChunk->inCount[2]; z++) {
            int  sum = 0;
            for (int i=0; i<bytes; i++) {
                int  diff = *inPtr - *outPtr;
                if (diff<0)    diff = -diff;
                sum += diff;
                inPtr++;
                outPtr++;
            }
            cout << "slave: z=" << z << " sum of mag of diff=" << sum << endl;
        }  //end for
        }  //end block
#endif
        MPI_Send( resultChunk, resultSize, MPI_UNSIGNED_CHAR, 0, 0,
                  MPI_COMM_WORLD );
#ifdef Verbose
        cout << "    slave " << ::myRank << ": sent." << endl;
        cout << "slave: free result chunk" << endl;
#endif
        free( resultChunk );
        resultChunk = NULL;
        //sleep( 10 );
#ifdef Verbose
        cout << "slave: free work chunk" << endl;
#endif
        free( workChunk );
        workChunk = NULL;
    }  //end forever

    for (int i=0; i<CacheSize; i++) {
        delete cache[i];
        cache[i] = NULL;
    }
    free( cache );
    cache = NULL;
}
//----------------------------------------------------------------------

