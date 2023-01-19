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

/************************************************************************
 
File:   distance3D.c
Author: George J. Grevera
Date:   10/11/93
Description:
        This program takes as input a 3D binary scene (a BIM file) and
        produces a 3D grey scene which corresponds to a 3D distance
        map.
Command line:
        (See the first function, usage, for a description.)

************************************************************************/
#include        <assert.h>
#include        <limits.h>
#include        <math.h>
#include        <stdio.h>
#include        <stdlib.h>
//#include        <values.h>
#include        <float.h>
#include        <ctype.h>
#include        <cv3dv.h>

#define  MAXSHORT  SHRT_MAX
#define  MAXFLOAT  FLT_MAX

typedef unsigned char   uchar;
typedef unsigned short  ushort;
/*
typedef unsigned long   ulong;
*/
#define ulong   unsigned long
typedef unsigned char   INDATA;
/*
typedef float          OUTDATA;
#define INF             (MAXFLOAT / 2.0)
#define OUTDATA_FLOAT

typedef short           OUTDATA;
#define INF             MAXSHORT
#define OUTDATA_SHORT
*/
typedef float          OUTDATA;
#define INF            (MAXFLOAT / 2.0)
#define OUTDATA_FLOAT

#define public
#define private static

#define NODIR   0       /* none == 0 */
#define XDIR    1
#define YDIR    2
#define ZDIR    4
#define ALL     (XDIR | YDIR | ZDIR)

enum { EUCLIDEAN, UNIFORM };

private FILE            *infp,
                        *outfp;
private ViewnixHeader   vhin;
private ulong           inbytes,
                        outbytes,
                        *in_zindex = NULL,
                        *out_yindex = NULL,
                        *out_zindex = NULL,
                        *yindex_2d = NULL;
private int             dflag = 0,
                        eflag = UNIFORM,
						fflag = FALSE,
                        iflag = NODIR,
                        oflag = 0,
                        pflag = ALL,
                        rflag = 0,
                        sflag = 0,
                        zflag = 0;
private OUTDATA         *outdata,
                        d1_x, d1_y, d1_z,
                        d1_x_half, d1_y_half, d1_z_half,
                        d2_xy, d2_xz, d2_yz,
                        d3_xyz;
private INDATA          *indata;
private char            group[5],
                        element[5];
private char            *iname;
/************************************************************************/
/* Modified: 9/27/99 -f flag added by Dewey Odhner. */
#ifdef  __STDC__
private void usage ( char* msg )  {
#else
private void usage ( msg )
char* msg;
{
#endif
    fprintf(stderr, "\n\
        distance3D [-d] [-e] [-f] [-i x|y|xy|xyz] [-p x|y|z|xy|xz|yz|xyz] [-r]\n\
                [-z 0|1|r|m] input-file output-file [interpolate-file]\n\
\n\
        where\n\
\n\
        -d indicates that twice as many z slices are to be produced in\n\
           the output file for positive and negative distances\n\
        -e indicates that the program should use scaled euclidean\n\
           distance units when calculating the distance transform\n\
           (grid steps (\"uniform\") is the default)\n\
        -f compute the distance in the foreground only\n\
        -i interpolate the data in the x, y, x & y, or in all directions\n\
        -p specifies the directions or planes to consider when calculating\n\
           the distance map (xyz i.e. all is the default)\n\
        -r indicates that the interpolated data should be rounded before\n\
           being binarized (default is truncate)\n\
        -s indicates that the output distance map should use unsigned short\n\
           for the distance values - to represent negative numbers, MAXSHORT\n\
           is added to each value - also note that -d and -s are mutually\n\
           exclusive\n\
        -z specifies whether a value of zero is to be converted to 0 or to\n\
           1 upon binarization\n\
        input-file is the input 3D .BIM file\n\
        output-file is the output 3D .IM0 file containing the distance map\n\
        interpolate-file is the output file of interpolated data\n\n");
    if (msg != NULL)    fprintf(stderr, "%s\n\n", msg);
    exit(-1);
}
/************************************************************************/
#if defined (WIN32) || defined (_WIN32)
  //not implemented by mr gates so we'll implement a version of our own
  static int     optind=1;
  static char*   optarg=NULL;
  static int getopt ( int argc, char* argv[], char* options ) {
      char*  cp;
      char   c;
      char*  where;

    if (optind >= argc)  return EOF;
    cp = argv[optind];
    if (cp[0] != '-' && cp[0] != '/')  return EOF;
    c = tolower(cp[1]);
    where = strchr(options, c);
    if (where == NULL)  usage("unrecognized command option");
    ++optind;
    //check and see if this argument/option requires a value
    ++where;
    if (*where == ':')  optarg = argv[optind++];

    return c;
  }
#else
  #include <unistd.h>
  int getopt();
  extern int     optind;
  extern char*   optarg;
#endif
/************************************************************************/
/*
This function initializes arrays that are used so speed up indexing.
 Modified: 6/25/97 corrected for odd width scenes by Dewey Odhner.
*/
#ifdef  __STDC__
private void init_index ( void )  {
#else
private void init_index ( )  {
#endif
    int i;
    /*
    init the in indices
    */
    assert(in_zindex == NULL);

    in_zindex = (ulong*) malloc(vhin.scn.num_of_subscenes[0] *
                                sizeof *in_zindex);
    assert(in_zindex != NULL);
	if (in_zindex == NULL)
		exit(1);
    for (i = 0; i < vhin.scn.num_of_subscenes[0]; i++)
        in_zindex[i] = i * ((vhin.scn.xysize[1] * vhin.scn.xysize[0] + 7) / 8);
    /*
    init the out indices
    */
    assert(out_yindex == NULL && out_zindex == NULL);
    out_yindex = (ulong*) malloc(vhin.scn.xysize[1] * sizeof *out_yindex);
    assert(out_yindex != NULL);
	if (out_yindex == NULL)
		exit(1);
    for (i = 0; i < vhin.scn.xysize[1]; i++)
        out_yindex[i] = i * vhin.scn.xysize[0];

    out_zindex = (ulong*) malloc(vhin.scn.num_of_subscenes[0] *
                                 sizeof *out_zindex);
    assert(out_zindex != NULL);
	if (out_zindex == NULL)
		exit(1);
    for (i = 0; i < vhin.scn.num_of_subscenes[0]; i++)
        out_zindex[i] = i * (vhin.scn.xysize[0] * vhin.scn.xysize[1]);
    /*
    init the 2d indices
    */
    assert(yindex_2d == NULL);
    switch (iflag)  {
        case NODIR:
            break;
        case XDIR :
            yindex_2d = (ulong*) malloc(vhin.scn.xysize[1] *
                                        sizeof *yindex_2d);
            assert(yindex_2d != NULL);
			if (yindex_2d == NULL)
				exit(1);
            for (i = 0; i < vhin.scn.xysize[1]; i++)
                yindex_2d[i] = i * (2 * vhin.scn.xysize[0] - 1);
            break;
        case YDIR :
            yindex_2d = (ulong*) malloc((2 * vhin.scn.xysize[1] - 1) *
                                        sizeof *yindex_2d);
            assert(yindex_2d != NULL);
			if (yindex_2d == NULL)
				exit(1);
            for (i = 0; i < 2 * vhin.scn.xysize[1] - 1; i++)
                yindex_2d[i] = i * vhin.scn.xysize[0];
            break;
        case (XDIR | YDIR) :
        case ALL  :
            yindex_2d = (ulong*) malloc((2 * vhin.scn.xysize[1] - 1) *
                                        sizeof *yindex_2d);
            assert(yindex_2d != NULL);
			if (yindex_2d == NULL)
				exit(1);
            for (i = 0; i < 2 * vhin.scn.xysize[1] - 1; i++)
                yindex_2d[i] = i * (2 * vhin.scn.xysize[0] - 1);
            break;
        default :
            assert(0);
            break;
    }
}
/************************************************************************/
/*
This function returns the index within a slice of the interpolate data.
*/
#ifdef NDEBUG
#define idx2d( x, y) (yindex_2d[y] + x)
#else
#ifdef  __STDC__
private long idx2d ( int x, int y )  {
#else
private long idx2d ( x, y )
int x, y;
{
#endif
    return yindex_2d[y] + x;
}
#endif
/************************************************************************/
/*
This function returns an element of the unpacked output data.
*/
#ifdef NDEBUG
#define get_outdata(x, y, z) outdata[ out_zindex[z] + out_yindex[y] + (x) ]
#else
#ifdef  __STDC__
private OUTDATA get_outdata ( int x, int y, int z )  {
#else
private OUTDATA get_outdata ( x, y, z )
int x, y, z;
{
#endif
    assert(x >= 0 && x < vhin.scn.xysize[0]);
    assert(y >= 0 && y < vhin.scn.xysize[1]);
    assert(z >= 0 && z < vhin.scn.num_of_subscenes[0]);
    return outdata[ out_zindex[z] + out_yindex[y] + x ];
}
#endif
/************************************************************************/
/*
This function sets an element of the unpacked output data.
*/
private void set_outdata ( int x, int y, int z, OUTDATA val )  {
    assert(x >= 0 && x < vhin.scn.xysize[0]);
    assert(y >= 0 && y < vhin.scn.xysize[1]);
    assert(z >= 0 && z < vhin.scn.num_of_subscenes[0]);
    outdata[ out_zindex[z] + out_yindex[y] + x ] = val;
}
/************************************************************************/
/*
This function returns an element of the packed input data.
 Modified: 4/2/97 alignment by slice, not by line assumed by Dewey Odhner.
*/
#ifdef NDEBUG
private int masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 }, xy_index;
#define get_indata( x, y, z ) (xy_index = (y)*vhin.scn.xysize[0] + (x), \
    indata[ in_zindex[z] + xy_index / 8 ] & masks[xy_index % 8]? 1: 0)
#else
#ifdef  __STDC__
private INDATA get_indata ( int x, int y, int z )  {
#else
private INDATA get_indata ( x, y, z )
int x, y, z;
{
#endif
    private int masks[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
	int xy_index;
    assert(x >= 0 && x < vhin.scn.xysize[0]);
    assert(y >= 0 && y < vhin.scn.xysize[1]);
    assert(z >= 0 && z < vhin.scn.num_of_subscenes[0]);
	xy_index = y*vhin.scn.xysize[0] + x;
    if (indata[ in_zindex[z] + xy_index / 8 ] & masks[xy_index % 8])
        return 1;
    else
        return 0;
}
#endif
/************************************************************************/
/*
This function returns 1 if the data point is within bounds and 0 otherwise.
*/
#ifdef NDEBUG
#define in_bounds( x, y, z ) !(x < 0 || x >= vhin.scn.xysize[0] || \
    y < 0 || y >= vhin.scn.xysize[1] || \
    z < 0 || z >= vhin.scn.num_of_subscenes[0])
#else
#ifdef  __STDC__
private int in_bounds ( int x, int y, int z )  {
#else
private int in_bounds ( x, y, z )
int x, y, z;
{
#endif
    if (x < 0 || x >= vhin.scn.xysize[0])               return 0;
    if (y < 0 || y >= vhin.scn.xysize[1])               return 0;
    if (z < 0 || z >= vhin.scn.num_of_subscenes[0])     return 0;
    return 1;
}
#endif
/************************************************************************/
/*
This function initializes the distances in each direction.
Modified: 9/19/03 scaled euclidean distance units used by Dewey Odhner.
*/
#ifdef  __STDC__
private void init_directional_distances ( void )  {
#else
private void init_directional_distances ( )  {
#endif

	double dist_unit;

    switch (eflag)  {
        case EUCLIDEAN  :
			dist_unit = (vhin.scn.xypixsz[1]*vhin.scn.xysize[1]+
				vhin.scn.xypixsz[0]*vhin.scn.xysize[0]+
				vhin.scn.loc_of_subscenes[vhin.scn.num_of_subscenes[0]-1]-
				vhin.scn.loc_of_subscenes[0])/32767;
            printf("scaled euclidean distance unit: %f\n", dist_unit);
			d1_x = (OUTDATA)(vhin.scn.xypixsz[0]/dist_unit);
            d1_y = (OUTDATA)(vhin.scn.xypixsz[1]/dist_unit);
            if (vhin.scn.num_of_subscenes[0] > 1)
                d1_z = (OUTDATA)((vhin.scn.loc_of_subscenes[1] -
					vhin.scn.loc_of_subscenes[0])/dist_unit);
            else
                d1_z = (d1_x < d1_y) ? d1_x : d1_y;
            d1_x_half = (OUTDATA)(d1_x / 2.0);
            d1_y_half = (OUTDATA)(d1_y / 2.0);
            d1_z_half = (OUTDATA)(d1_z / 2.0);

            d2_xy = (OUTDATA)sqrt( (d1_x * d1_x) + (d1_y * d1_y) );
            d2_xz = (OUTDATA)sqrt( (d1_x * d1_x) + (d1_z * d1_z) );
            d2_yz = (OUTDATA)sqrt( (d1_y * d1_y) + (d1_z * d1_z) );

            d3_xyz = (OUTDATA)sqrt( (d1_x * d1_x) + (d1_y * d1_y) + (d1_z * d1_z) );
            break;
        case UNIFORM    :
            /*
            puts("uniform chamfer distance");
            d1_x = d1_y = d1_z = 6;
            d1_x_half = d1_y_half = d1_z_half = d1_x / 2.0;
            d2_xy = d2_xz = d2_yz = 8;
            d3_xyz = 10;
            */
            puts("uniform euclidean distance units");
            d1_x = d1_y = d1_z = 1.0;
            d1_x_half = d1_y_half = d1_z_half = (OUTDATA)(d1_x / 2.0);
            d2_xy = d2_xz = d2_yz =
                (OUTDATA)((sqrt(3.0) + 1.0 + sqrt(2.0 * sqrt(3.0)-2.0)) / 3.0);
            d3_xyz = (OUTDATA)((2.0 * sqrt(3.0) - 1.0 + 2.0 *
                      sqrt(2.0 * sqrt(3.0) - 2.0)) / 3.0);
            break;
        default         :
            assert(0);
            break;
    }

#ifdef  OUTDATA_FLOAT
    printf("directional distances initialized to x=%.2f, y=%.2f, z=%.4f\n",
           d1_x, d1_y, d1_z);
#endif
#ifdef  OUTDATA_SHORT
    printf("directional distances initialized to x=%d, y=%d, z=%d\n",
           d1_x, d1_y, d1_z);
#endif
}
/************************************************************************/
/*
This function performs any necessary initialization such as opening files
and allocating memory.
 Modified: 6/25/97 corrected for odd width scenes by Dewey Odhner.
 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
private void init ( int argc, char* argv[] )  {
#   define OPTIONS "defhi:op:rsz:"
    int c, num, err;
    fprintf(stderr, "\nInitializing . . .\n");
    /*
    process command option(s)
    */
    while ((c = getopt(argc, argv, OPTIONS)) != EOF)
        switch (c)  {
            case 'd' :  dflag = 1;
                        assert(!oflag && !sflag);
                        break;
            case 'o' :  oflag = 1;
                        assert(!dflag && !sflag);
                        break;
            case 's' :  sflag = 1;
                        assert(!dflag && !oflag);
                        break;

            case 'e' :  eflag = EUCLIDEAN;      break;
            case 'f' :  fflag = TRUE;
                        break;
            case 'i' :  iflag = 0;
                        for (num = 0; optarg[num]; num++)
                            switch (optarg[num])  {
                                case 'x' :
                                case 'X' :      iflag |= XDIR;  break;
                                case 'y' :
                                case 'Y' :      iflag |= YDIR;  break;
                                case 'z' :
                                case 'Z' :      iflag |= ZDIR;  break;
                                default  :      usage("-i: bad option");
                                                break;
                            }
                        assert( iflag == XDIR || iflag == YDIR ||
                                iflag == (XDIR | YDIR) ||
                                iflag == ALL );
                        break;

            case 'p' :  pflag = 0;
                        for (num = 0; optarg[num]; num++)
                            switch (optarg[num])  {
                                case 'x' :
                                case 'X' :      pflag |= XDIR;  break;
                                case 'y' :
                                case 'Y' :      pflag |= YDIR;  break;
                                case 'z' :
                                case 'Z' :      pflag |= ZDIR;  break;
                                default  :      usage("-p: bad option");
                                                break;
                            }
                        break;
            case 'r' :  rflag = 1;              break;
            case 'z' :  if (optarg[0] == '0')           zflag = 0;
                        else if (optarg[0] == '1')      zflag = 1;
                        else if (optarg[0] == 'r')      zflag = 'r';
                        else if (optarg[0] == 'm')      zflag = 'm';
                        else                            usage("z");
                        if (optarg[1])                  usage("z");
                        break;
            default  :  usage(NULL);            break;
        }
    assert(pflag);
    /****************************************
    open the input file & read the input data
    ****************************************/
    if (optind >= argc)         usage("No input file specified!");
    infp = fopen(argv[optind++], "rb");          assert(infp != NULL);
    /*
    read the input file's header
    */
    err = VReadHeader(infp, &vhin, group, element);
    if (err && err < 106)
        {  puts("Can't read the input file.");  exit(-1);  }
    if (vhin.scn.dimension != 3)
	{
		fprintf(stderr, "%s only takes 3-D scenes!\n", argv[0]);
		exit(1);
	}
    if (vhin.scn.num_of_bits != 1)
	{
		fprintf(stderr, "%s only takes binary scenes!\n", argv[0]);
		exit(1);
	}
    if (vhin.scn.xypixsz[0] != vhin.scn.xypixsz[1])
        fprintf(stderr, "x pixel size does not match y pixel size.\n");
    if (vhin.scn.xypixsz[0] != (vhin.scn.loc_of_subscenes[1]-
                                vhin.scn.loc_of_subscenes[0]))
        fprintf(stderr, "x pixel size does not match slice spacing.\n");
    if (vhin.scn.xypixsz[1] != (vhin.scn.loc_of_subscenes[1]-
                                vhin.scn.loc_of_subscenes[0]))
        fprintf(stderr, "y pixel size does not match slice spacing.\n");
    fflush(stderr);

    inbytes = (vhin.scn.xysize[0] * vhin.scn.xysize[1] + 7) / 8 *
              vhin.scn.num_of_subscenes[0] * sizeof *indata;
    indata = (INDATA*) malloc(inbytes);
    assert(indata != NULL);
	if (indata == NULL)
		exit(1);
    /*
    read the input data
    */
    err = VSeekData(infp, 0);                   assert(err == 0);
    err = VReadData((char *)indata, 1, inbytes, infp, &num);
    if (err)
      {  fputs("\nCan't read the input file's data.\n", stderr);  exit (-1);  }
    assert(inbytes == num);
    /********************
    init the output data
    ********************/
    assert(optind < argc);
    outfp = fopen(argv[optind++], "w+b");        assert(outfp != NULL);
    outbytes = vhin.scn.xysize[0] * vhin.scn.xysize[1] *
               vhin.scn.num_of_subscenes[0] * sizeof *outdata;
    outdata = (OUTDATA*) malloc(outbytes);
    assert(outdata != NULL);
	if (outdata == NULL)
		exit(1);
    /*
    remember the name of the interpolated file
    */
    if (iflag)  {
      assert(optind < argc);
      iname = argv[optind++];
    }

    init_index();
    init_directional_distances();
}
/************************************************************************/
/*
This function initializes the distance map.
 Modified: 9/28/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void init_distance ( void )  {
#else
private void init_distance ( )  {
#endif
    int x, y, z;
    OUTDATA *outptr;

    fprintf(stderr, "\nInitializing distance map\t");   fflush(stderr);
    /*
    initialize the distance values to infinity
    */
    for (outptr = outdata+vhin.scn.xysize[0]*vhin.scn.xysize[1]*
            vhin.scn.num_of_subscenes[0]-1; outptr >= outdata; outptr--)
        *outptr = INF;
    /*
    initializes the distances for the elements of the immediate exterior
    & the immediate interior
    */
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)
              /*
              is this point outside or inside?
              */
              switch (get_indata(x,y,z))  {
                case 0 :/* outside */
                    if (fflag)  {
                        set_outdata(x,y,z, (OUTDATA)0);
                        break;
                    }
                    /*
                    is this point an element of the ie?
                    */
                    if (pflag & XDIR)  {
                        if (in_bounds(x-1,y,z) && get_indata(x-1,y,z) &&
                            d1_x_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_x_half);
                        if (in_bounds(x+1,y,z) && get_indata(x+1,y,z) &&
                            d1_x_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_x_half);
                    }
                    if (pflag & YDIR)  {
                        if (in_bounds(x,y-1,z) && get_indata(x,y-1,z) &&
                            d1_y_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_y_half);
                        if (in_bounds(x,y+1,z) && get_indata(x,y+1,z) &&
                            d1_y_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_y_half);
                    }
                    if (pflag & ZDIR)  {
                        if (in_bounds(x,y,z-1) && get_indata(x,y,z-1) &&
                            d1_z_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_z_half);
                        if (in_bounds(x,y,z+1) && get_indata(x,y,z+1) &&
                            d1_z_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_z_half);
                    }
                    break;
                case 1 :/* inside */
                    /*
                    is this point an element of the ii?
                    */
                    /*
                    check boundary . . .
                    */
                    if (pflag & XDIR)
                        if (!in_bounds(x-1,y,z) || !in_bounds(x+1,y,z))
                            if (d1_x_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_x_half);
                    if (pflag & YDIR)
                        if (!in_bounds(x,y-1,z) || !in_bounds(x,y+1,z))
                            if (d1_y_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_y_half);
                    if (pflag & ZDIR)
                        if (!in_bounds(x,y,z-1) || !in_bounds(x,y,z+1))
                            if (d1_z_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_z_half);
                    /*
                    check other side of faces . . .
                    */
                    if (pflag & XDIR)  {
                        if (in_bounds(x-1,y,z) && !get_indata(x-1,y,z) &&
                            d1_x_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_x_half);
                        if (in_bounds(x+1,y,z) && !get_indata(x+1,y,z) &&
                            d1_x_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_x_half);
                    }
                    if (pflag & YDIR)  {
                        if (in_bounds(x,y-1,z) && !get_indata(x,y-1,z) &&
                            d1_y_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_y_half);
                        if (in_bounds(x,y+1,z) && !get_indata(x,y+1,z) &&
                            d1_y_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_y_half);
                    }
                    if (pflag & ZDIR)  {
                        if (in_bounds (x,y,z-1) && !get_indata(x,y,z-1) &&
                            d1_z_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_z_half);
                        if (in_bounds(x,y,z+1) && !get_indata(x,y,z+1) &&
                            d1_z_half < get_outdata(x,y,z))
                                set_outdata(x,y,z, d1_z_half);
                    }
                    break;
                default :
                    assert(0);
                    break;
              }
    }  /* end for z */
}
/************************************************************************/
/*
forward pass:   (distance changes)

+d3_xyz +d2_yz +d3_xyz          +d2_xy +d1_y +d2_xy
+d2_xz  +d1_z  +d2_xz           +d1_x    0
+d3_xyz +d2_yz +d_3xyz

backward pass:

                                                        +d3_xyz +d2_yz +d3_xyz
                                         0   +d1_x      +d2_xz  +d1_z  +d2_xz
                                +d2_xy +d1_y +d2_xy     +d3_xyz +d2_yz +d3_xyz

flags:

mxyz myz mxyz           mxy my mxy
mxz  mz  mxz            mx  -  mx
mxyz myz mxyz           mxy my mxy

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_xyz ( void )  {
#else
private void distance_transform_xyz ( )  {
#endif
    OUTDATA min, tod;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;
              /*
              check those values in the previous z slice
              */
              if (in_bounds(x-1,y-1,z-1) && (tod=get_outdata(x-1,y-1,z-1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x+1,y-1,z-1) && (tod=get_outdata(x+1,y-1,z-1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x-1,y+1,z-1) && (tod=get_outdata(x-1,y+1,z-1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x+1,y+1,z-1) && (tod=get_outdata(x+1,y+1,z-1))+d3_xyz < min)
                  min = get_outdata(x+1,y+1,z-1) + d3_xyz;
              if (in_bounds(x,y-1,z-1) && (tod=get_outdata(x,y-1,z-1))+d2_yz < min)
                  min = tod + d2_yz;
              if (in_bounds(x,y+1,z-1) && (tod=get_outdata(x,y+1,z-1))+d2_yz < min)
                  min = tod + d2_yz;
              if (in_bounds(x-1,y,z-1) && (tod=get_outdata(x-1,y,z-1))+d2_xz < min)
                  min = tod + d2_xz;
              if (in_bounds(x+1,y,z-1) && (tod=get_outdata(x+1,y,z-1))+d2_xz < min)
                  min = tod + d2_xz;
              if (in_bounds(x,y,z-1) && (tod=get_outdata(x,y,z-1))+d1_z < min)
                    min = tod + d1_z;
              /*
              check those values in the same z slice
              */
              if (in_bounds(x-1,y-1,z) && (tod=get_outdata(x-1,y-1,z))+d2_xy < min)
                  min = tod + d2_xy;
              if (in_bounds(x+1,y-1,z) && (tod=get_outdata(x+1,y-1,z))+d2_xy < min)
                  min = tod + d2_xy;
              if (in_bounds(x,y-1,z) && (tod=get_outdata(x,y-1,z))+d1_y < min)
                  min = tod + d1_y;
              if (in_bounds(x-1,y,z) && (tod=get_outdata(x-1,y,z))+d1_x < min)
                  min = tod + d1_x;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (y = vhin.scn.xysize[1]-1; y >= 0; y--)
            for (x = vhin.scn.xysize[0]-1; x >= 0; x--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;
              /*
              check those values in the same z slice
              */
              if (in_bounds(x+1,y,z) && (tod=get_outdata(x+1,y,z))+d1_x < min)
                  min = tod + d1_x;
              if (in_bounds(x-1,y+1,z) && (tod=get_outdata(x-1,y+1,z))+d2_xy < min)
                  min = tod + d2_xy;
              if (in_bounds(x+1,y+1,z) && (tod=get_outdata(x+1,y+1,z))+d2_xy < min)
                  min = tod + d2_xy;
              if (in_bounds(x,y+1,z) && (tod=get_outdata(x,y+1,z))+d1_y < min)
                  min = tod + d1_y;
              /*
              check those values in the next z slice
              */
              if (in_bounds(x-1,y-1,z+1) && (tod=get_outdata(x-1,y-1,z+1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x+1,y-1,z+1) && (tod=get_outdata(x+1,y-1,z+1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x-1,y+1,z+1) && (tod=get_outdata(x-1,y+1,z+1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x+1,y+1,z+1) && (tod=get_outdata(x+1,y+1,z+1))+d3_xyz < min)
                  min = tod + d3_xyz;
              if (in_bounds(x,y-1,z+1) && (tod=get_outdata(x,y-1,z+1))+d2_yz < min)
                  min = tod + d2_yz;
              if (in_bounds(x,y+1,z+1) && (tod=get_outdata(x,y+1,z+1))+d2_yz < min)
                  min = tod + d2_yz;
              if (in_bounds(x-1,y,z+1) && (tod=get_outdata(x-1,y,z+1))+d2_xz < min)
                  min = tod + d2_xz;
              if (in_bounds(x+1,y,z+1) && (tod=get_outdata(x+1,y,z+1))+d2_xz < min)
                  min = tod + d2_xz;
              if (in_bounds(x,y,z+1) && (tod=get_outdata(x,y,z+1))+d1_z < min)
                  min = tod + d1_z;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
}
/************************************************************************/
/*
forward pass:   (distance changes)

                +d2_yz +d1_y +d2_yz
                +d1_z    0

backward pass:
                         0   +d1_z
                +d2_yz +d1_y +d2_yz

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_yz ( void )  {
#else
private void distance_transform_yz ( )  {
#endif
    OUTDATA min;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (x = 0; x < vhin.scn.xysize[0]; x++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y-1,z-1) && get_outdata(x,y-1,z-1)+d2_yz < min)
                  min = get_outdata(x,y-1,z-1) + d2_yz;
              if (in_bounds(x,y-1,z) && get_outdata(x,y-1,z)+d1_y < min)
                  min = get_outdata(x,y-1,z) + d1_y;
              if (in_bounds(x,y-1,z+1) && get_outdata(x,y-1,z+1)+d2_yz < min)
                  min = get_outdata(x,y-1,z+1) + d2_yz;
              if (in_bounds(x,y,z-1) && get_outdata(x,y,z-1)+d1_z < min)
                  min = get_outdata(x,y,z-1) + d1_z;

              set_outdata(x,y,z, min);
            }
    }  /* end for x */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (x = vhin.scn.xysize[0]-1; x >= 0; x--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (y = vhin.scn.xysize[1]-1; y >= 0; y--)
            for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y,z+1) && get_outdata(x,y,z+1)+d1_z < min)
                  min = get_outdata(x,y,z+1) + d1_z;
              if (in_bounds(x,y+1,z-1) && get_outdata(x,y+1,z-1)+d2_yz < min)
                  min = get_outdata(x,y+1,z-1) + d2_yz;
              if (in_bounds(x,y+1,z) && get_outdata(x,y+1,z)+d1_y < min)
                  min = get_outdata(x,y+1,z) + d1_y;
              if (in_bounds(x,y+1,z+1) && get_outdata(x,y+1,z+1)+d2_yz < min)
                  min = get_outdata(x,y+1,z+1) + d2_yz;

              set_outdata(x,y,z, min);
            }
    }  /* end for x */
}
/************************************************************************/
/*
forward pass:   (distance changes)

                +d2_xy +d1_x +d2_xy
                +d1_y    0

backward pass:
                         0   +d1_y
                +d2_xy +d1_x +d2_xy

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_xy ( void )  {
#else
private void distance_transform_xy ( )  {
#endif
    OUTDATA min;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = 0; x < vhin.scn.xysize[0]; x++)
            for (y = 0; y < vhin.scn.xysize[1]; y++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x-1,y-1,z) && get_outdata(x-1,y-1,z)+d2_xy < min)
                  min = get_outdata(x-1,y-1,z) + d2_xy;
              if (in_bounds(x-1,y,z) && get_outdata(x-1,y,z)+d1_x < min)
                  min = get_outdata(x-1,y,z) + d1_x;
              if (in_bounds(x-1,y+1,z) && get_outdata(x-1,y+1,z)+d2_xy < min)
                  min = get_outdata(x-1,y+1,z) + d2_xy;
              if (in_bounds(x,y-1,z) && get_outdata(x,y-1,z)+d1_y < min)
                  min = get_outdata(x,y-1,z) + d1_y;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = vhin.scn.xysize[0]-1; x >= 0; x--)
            for (y = vhin.scn.xysize[1]-1; y >= 0; y--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y+1,z) && get_outdata(x,y+1,z)+d1_y < min)
                  min = get_outdata(x,y+1,z) + d1_y;
              if (in_bounds(x+1,y-1,z) && get_outdata(x+1,y-1,z)+d2_xy < min)
                  min = get_outdata(x+1,y-1,z) + d2_xy;
              if (in_bounds(x+1,y,z) && get_outdata(x+1,y,z)+d1_x < min)
                  min = get_outdata(x+1,y,z) + d1_x;
              if (in_bounds(x+1,y+1,z) && get_outdata(x+1,y+1,z)+d2_xy < min)
                  min = get_outdata(x+1,y+1,z) + d2_xy;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
}
/************************************************************************/
/*
forward pass:   (distance changes)

                +d2_xz +d1_x +d2_xz
                +d1_z    0

backward pass:
                         0   +d1_z
                +d2_xz +d1_x +d2_xz

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_xz ( void )  {
#else
private void distance_transform_xz ( )  {
#endif
    OUTDATA min;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (y = 0; y < vhin.scn.xysize[1]; y++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = 0; x < vhin.scn.xysize[0]; x++)
            for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x-1,y,z-1) && get_outdata(x-1,y,z-1)+d2_xz < min)
                  min = get_outdata(x-1,y,z-1) + d2_xz;
              if (in_bounds(x-1,y,z) && get_outdata(x-1,y,z)+d1_x < min)
                  min = get_outdata(x-1,y,z) + d1_x;
              if (in_bounds(x-1,y,z+1) && get_outdata(x-1,y,z+1)+d2_xz < min)
                  min = get_outdata(x-1,y,z+1) + d2_xz;
              if (in_bounds(x,y,z-1) && get_outdata(x,y,z-1)+d1_z < min)
                  min = get_outdata(x,y,z-1) + d1_z;

              set_outdata(x,y,z, min);
            }
    }  /* end for y */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (y = vhin.scn.xysize[1]-1; y >= 0; y--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = vhin.scn.xysize[0]-1; x >= 0; x--)
            for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y,z+1) && get_outdata(x,y,z+1)+d1_z < min)
                  min = get_outdata(x,y,z+1) + d1_z;
              if (in_bounds(x+1,y,z-1) && get_outdata(x+1,y,z-1)+d2_xz < min)
                  min = get_outdata(x+1,y,z-1) + d2_xz;
              if (in_bounds(x+1,y,z) && get_outdata(x+1,y,z)+d1_x < min)
                  min = get_outdata(x+1,y,z) + d1_x;
              if (in_bounds(x+1,y,z+1) && get_outdata(x+1,y,z+1)+d2_xz < min)
                  min = get_outdata(x+1,y,z+1) + d2_xz;

              set_outdata(x,y,z, min);
            }
    }  /* end for y */
}
/************************************************************************/
/*
forward pass:   (distance changes)
                +d1_z    0
backward pass:
                         0   +d1_z

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_z ( void )  {
#else
private void distance_transform_z ( )  {
#endif
    OUTDATA min;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (y = 0; y < vhin.scn.xysize[1]; y++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = 0; x < vhin.scn.xysize[0]; x++)
            for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y,z-1) && get_outdata(x,y,z-1)+d1_z < min)
                  min = get_outdata(x,y,z-1) + d1_z;

              set_outdata(x,y,z, min);
            }
    }  /* end for y */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (y = vhin.scn.xysize[1]-1; y >= 0; y--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = vhin.scn.xysize[0]-1; x >= 0; x--)
            for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y,z+1) && get_outdata(x,y,z+1)+d1_z < min)
                  min = get_outdata(x,y,z+1) + d1_z;

              set_outdata(x,y,z, min);
            }
    }  /* end for y */
}
/************************************************************************/
/*
forward pass:   (distance changes)
                +d1_y    0
backward pass:
                         0   +d1_y

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_y ( void )  {
#else
private void distance_transform_y ( )  {
#endif
    OUTDATA min;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = 0; x < vhin.scn.xysize[0]; x++)
            for (y = 0; y < vhin.scn.xysize[1]; y++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y-1,z) && get_outdata(x,y-1,z)+d1_y < min)
                  min = get_outdata(x,y-1,z) + d1_y;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (x = vhin.scn.xysize[0]-1; x >= 0; x--)
            for (y = vhin.scn.xysize[1]-1; y >= 0; y--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x,y+1,z) && get_outdata(x,y+1,z)+d1_y < min)
                  min = get_outdata(x,y+1,z) + d1_y;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
}
/************************************************************************/
/*
forward pass:   (distance changes)
                +d1_x    0
backward pass:
                         0   +d1_x

 Modified: 9/27/99 -f flag added by Dewey Odhner.
*/
#ifdef  __STDC__
private void distance_transform_x ( void )  {
#else
private void distance_transform_x ( )  {
#endif
    OUTDATA min;
    int x, y, z;
    /*
    initialize . . .
    */
    init_distance();
    /*
    forward pass . . .
    */
    fprintf(stderr, "\nforward pass\t\t\t");            fflush(stderr);
    for (y = 0; y < vhin.scn.xysize[1]; y++)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x-1,y,z) && get_outdata(x-1,y,z)+d1_x < min)
                  min = get_outdata(x-1,y,z) + d1_x;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
    /*
    backward pass . . .
    */
    fprintf(stderr, "\nbackward pass\t\t\t");           fflush(stderr);
    for (y = vhin.scn.xysize[1]-1; y >= 0; y--)  {
        fprintf(stderr, ".");           fflush(stderr);
        for (z = vhin.scn.num_of_subscenes[0]-1; z >= 0; z--)
            for (x = vhin.scn.xysize[0]-1; x >= 0; x--)  {
              min = get_outdata(x,y,z);
              if (fflag && min==0)
                  continue;

              if (in_bounds(x+1,y,z) && get_outdata(x+1,y,z)+d1_x < min)
                  min = get_outdata(x+1,y,z) + d1_x;

              set_outdata(x,y,z, min);
            }
    }  /* end for z */
}
/************************************************************************/
private long packed_bytes;
/*
It's the caller's responsibility to free the returned, malloc'ed data.
*/
private uchar *convert2binary ( data, vhout )
float* data;
ViewnixHeader* vhout;
{
    long size, zero_count = 0;
    float val;
    int x, y, i, out, ysize, xsize;
    uchar *unpacked, *packed;
    assert(data != NULL);

    /* this is the output size (with padding at the end of each line) */
    size = vhout->scn.xysize[0] * vhout->scn.xysize[1];
    /* this is the output size (without padding) */
    if (iflag & YDIR)   ysize = 2 * vhin.scn.xysize[1] - 1;
    else                ysize = vhin.scn.xysize[1];
    if (iflag & XDIR)   xsize = 2 * vhin.scn.xysize[0] - 1;
    else                xsize = vhin.scn.xysize[0];

    unpacked = (uchar*) calloc(size, sizeof *unpacked);
    assert(unpacked != NULL);
	if (unpacked == NULL)
		exit(1);
    i = out = 0;
    for (y = 0; y < ysize; y++)  {
        for (x = 0; x < xsize; x++)  {
            if (rflag)  {
                if (data[i] > 0.0)      val = (float)(data[i] + 0.5); /* round it off */
                else if (data [i] < 0.0)        val = (float)(data[i] - 0.5);
                else                    {  ++zero_count;  val = 0;  }
            }
            else  {
                if (data[i] == 0.0)             ++zero_count;
                val = data[i];
            }
            /*
            binarize the value
            */
            if (val < 0)                unpacked[out] = 0;
            else if (val > 0)           unpacked[out] = 1;
            else                        unpacked[out] = zflag;

            i++, out++;
        }
        /* skip any extra padding */
        for ( ; x < vhout->scn.xysize[0]; x++)  out++;
    }
    /*
    allocate the packed data & pack the data
    */
    packed_bytes = size / 8;
    assert((size % 8) == 0);
    if ((size % 8) != 0)        ++packed_bytes;
    packed = (uchar*) calloc(packed_bytes, sizeof *packed);
    assert(packed != NULL);
	if (packed ==NULL)
		exit(1);
    VPackByteToBit(unpacked, size, packed);
    free(unpacked);
    fprintf(stderr, "%ld zero's", zero_count);
    return packed;
}
/************************************************************************/
/*
This functions is supposed to be called from the debugger.
*/
#ifdef  NDEBUG
#define helper(vhout,intrp)
#else
public void helper ( vhout, intrp )
ViewnixHeader* vhout;
float* intrp;
{
    int x, y;
return;
    puts("");
    /* for (y = 0; y < vhout->scn.xysize[1]; y++)  { */
    y = 4;  {
        printf("y=%2d:", y);
        for (x = 0; x < vhout->scn.xysize[0]; x++)  {
            printf("%5.1lf", intrp[idx2d(x,y)]);
        }
        printf("\n");
    }
}
#endif
/************************************************************************/
/*
This function interpolates the output data in the x direction.
*/
#ifdef  __STDC__
private void interpolate_x ( void )  {
#else
private void interpolate_x ( )  {
#endif
    ViewnixHeader vhout;
    FILE *outfp = fopen(iname, "w+b");
    OUTDATA val;
    float *intrp;
    private float smallest=0, largest=1;
    int x, y, z, num, err;
    uchar *packed;

    assert(outfp != NULL);
    /*
    init the output file's header
    */
    vhout = vhin;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value = &largest;
    vhout.scn.xysize[0] = 2 * vhin.scn.xysize[0] - 1;
if (vhout.scn.xysize[0] % 8)
        vhout.scn.xysize[0] += 8 - (vhout.scn.xysize[0] % 8);
    vhout.scn.xypixsz[0] /= 2;
    vhout.scn.num_of_bits = 1;
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  puts("Can't write the output file header.");  exit(-1);  }
    err = VSeekData(outfp, 0);
    assert(err == 0);
    /*
    grab some memory for the interpolated data & the binary version
    */
    intrp = (float*) malloc(vhout.scn.xysize[0] * vhout.scn.xysize[1] *
                             sizeof *intrp);
    assert(intrp != NULL);
	if (intrp == NULL)
		exit(1);
    /*
    process each slice
    */
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        /*
        initialize the interpolated data to the result of the distance transform
        */
        for (y = 0; y < vhin.scn.xysize[1]; y++)
          for (x = 0; x < vhin.scn.xysize[0]; x++)  {
            val = get_outdata(x,y,z);
            if (!get_indata(x,y,z))     val = -val;
            intrp[idx2d(x*2,y)] = val;
          }
        /*
        interpolate the data
        */
        for (y = 0; y < vhout.scn.xysize[1]; y++)
          for (x = 0; x < vhout.scn.xysize[0] - 2; x+=2)  {
            if (zflag != 'm')
                intrp[idx2d(x+1,y)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x+2,y)]) / 2.0);
            else  {
                intrp[idx2d(x+1,y)] = 0;
                if (intrp[idx2d(x,y)] > 0)              intrp[idx2d(x+1,y)] += 1;
                else if (intrp[idx2d(x,y)] < 0)         intrp[idx2d(x+1,y)] -= 1;
                if (intrp[idx2d(x+2,y)] > 0)            intrp[idx2d(x+1,y)] += 1;
                else if (intrp[idx2d(x+2,y)] > 0)       intrp[idx2d(x+1,y)] -= 1;
            }
          }
        /*
        convert back to binary
        */
        printf("\nz = %d\n", z);
        helper(&vhout, intrp);
        packed = convert2binary(intrp, &vhout);
        err = VWriteData((char*)packed, sizeof *packed, packed_bytes, outfp, &num);
        assert(err == 0 && num == packed_bytes);
        free(packed);
    }  /* end for z */
    free(intrp);        intrp = NULL;
    fclose(outfp);
}
/************************************************************************/
/*
This function interpolates the output data in the x direction.
*/
#ifdef  __STDC__
private void interpolate_y ( void )  {
#else
private void interpolate_y ( )  {
#endif
    ViewnixHeader vhout;
    FILE *outfp = fopen(iname, "w+b");
    OUTDATA val;
    float *intrp;
    private float smallest=0, largest=1;
    int x, y, z, num, err;
    uchar *packed;

    assert(outfp != NULL);
    /*
    init the output file's header
    */
    vhout = vhin;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value = &largest;
    vhout.scn.xysize[1] = 2 * vhin.scn.xysize[1] - 1;
    vhout.scn.xypixsz[1] /= 2;
    vhout.scn.num_of_bits = 1;
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  puts("Can't write the output file header.");  exit(-1);  }
    err = VSeekData(outfp, 0);
    assert(err == 0);
    /*
    grab some memory for the interpolated data & the binary version
    */
    intrp = (float*) malloc(vhout.scn.xysize[0] * vhout.scn.xysize[1] *
                             sizeof *intrp);
    assert(intrp != NULL);
	if (intrp == NULL)
		exit(1);
    /*
    process each slice
    */
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        /*
        initialize the interpolated data to the result of the distance transform
        */
        for (y = 0; y < vhin.scn.xysize[1]; y++)
          for (x = 0; x < vhin.scn.xysize[0]; x++)  {
            val = get_outdata(x,y,z);
            if (!get_indata(x,y,z))     val = -val;
            intrp[idx2d(x,y*2)] = val;
          }
        /*
        interpolate the data
        */
        for (y = 0; y < vhout.scn.xysize[1] - 2; y+=2)
          for (x = 0; x < vhout.scn.xysize[0]; x++)
            intrp[idx2d(x,y+1)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x,y+2)]) /
                                  2.0);
        /*
        convert back to binary
        */
        printf("\nz = %d\n", z);
        helper(&vhout, intrp);
        packed = convert2binary(intrp, &vhout);
        err = VWriteData((char*)packed, sizeof *packed, packed_bytes, outfp, &num);
        assert(err == 0 && num == packed_bytes);
        free(packed);
    }  /* end for z */
    free(intrp);        intrp = NULL;
    fclose(outfp);
}
/************************************************************************/
/*
This function interpolates the output data in both the x & y directions.
*/
#ifdef  __STDC__
private void interpolate_xy ( void )  {
#else
private void interpolate_xy ( )  {
#endif
    ViewnixHeader vhout;
    FILE *outfp = fopen(iname, "w+b");
    OUTDATA val;
    float *intrp;
    private float smallest=0, largest=1;
    int x, y, z, num, err;
    uchar *packed;

    assert(outfp != NULL);
    /*
    init the output file's header
    */
    vhout = vhin;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value = &largest;
    vhout.scn.xysize[0] = 2 * vhin.scn.xysize[0] - 1;
if (vhout.scn.xysize[0] % 8)
        vhout.scn.xysize[0] += 8 - (vhout.scn.xysize[0] % 8);
    vhout.scn.xysize[1] = 2 * vhin.scn.xysize[1] - 1;
    vhout.scn.xypixsz[0] /= 2;
    vhout.scn.xypixsz[1] /= 2;
    vhout.scn.num_of_bits = 1;
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  puts("Can't write the output file header.");  exit(-1);  }
    err = VSeekData(outfp, 0);
    assert(err == 0);
    /*
    grab some memory for the interpolated data & the binary version
    */
    intrp = (float*) malloc(vhout.scn.xysize[0] * vhout.scn.xysize[1] *
                             sizeof *intrp);
    assert(intrp != NULL);
	if (intrp == NULL)
		exit(1);
    /*
    process each slice
    */
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        /*
        initialize the interpolated data to the result of the distance transform
        */
        for (y = 0; y < vhin.scn.xysize[1]; y++)
          for (x = 0; x < vhin.scn.xysize[0]; x++)  {
            val = get_outdata(x,y,z);
            if (!get_indata(x,y,z))     val = -val;
            intrp[idx2d(x*2,y*2)] = val;
          }
        /*
        interpolate the data
        */
        for (y = 0; y < vhout.scn.xysize[1] - 2; y+=2)
          for (x = 0; x < vhout.scn.xysize[0] - 2; x+=2)  {
            intrp[idx2d(x+1,y)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x+2,y)]) /
                                  2.0);
            intrp[idx2d(x,y+1)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x,y+2)]) /
                                  2.0);
            intrp[idx2d(x+1,y+1)] = (float)(( intrp[idx2d(x,y)] + intrp[idx2d(x+2,y)] +
                                      intrp[idx2d(x,y+2)] + intrp[idx2d(x+2,y+2)] ) /
                                    4.0);
          }  /* end for x */
        /*
        interpolate the last row
        */
        y = vhout.scn.xysize[1] - 1;
        for (x = 0; x < vhout.scn.xysize[0] - 2; x+=2)
            intrp[idx2d(x+1,y)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x+2,y)]) /
                                  2.0);
        /*
        interpolate the last col
        */
        x = vhout.scn.xysize[0] - 1;
        for (y = 0; y < vhout.scn.xysize[1] - 2; y+=2)
            intrp[idx2d(x,y+1)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x,y+2)]) /
                                  2.0);
        /*
        convert back to binary
        */
        printf("\nz = %d\n", z);
        helper(&vhout, intrp);
        packed = convert2binary(intrp, &vhout);
        err = VWriteData((char*)packed, sizeof *packed, packed_bytes, outfp, &num);
        assert(err == 0 && num == packed_bytes);
        free(packed);
    }  /* end for z */
    free(intrp);        intrp = NULL;
    fclose(outfp);
}
/************************************************************************/
#ifdef  __STDC__
private void interpolate_new_z_slice ( ViewnixHeader* vhout,
                                       float* last_intrp, float* z_intrp,
                                       float* intrp )
#else
private void interpolate_new_z_slice ( vhout, last_intrp, z_intrp, intrp )
ViewnixHeader* vhout;
float *last_intrp, *z_intrp, *intrp;
#endif
{
    int x, y;
    assert(vhout != NULL);
    assert(last_intrp != NULL && z_intrp != NULL && intrp != NULL);
    for (y = 0; y < vhout->scn.xysize[1]; y++)
        for (x = 0; x < vhout->scn.xysize[0]; x++)
            *z_intrp++ = (float)((*last_intrp++ + *intrp++) / 2.0);
}
/*----------------------------------------------------------------------*/
#define _even(x)        (!((x) & 1))
/*
This function interpolates the output data in all directions (ALL = X,Y,Z).
*/
#ifdef  __STDC__
private void interpolate_xyz ( void )  {
#else
private void interpolate_xyz ( )  {
#endif
    ViewnixHeader vhout;
    FILE *outfp = fopen(iname, "w+b");
    OUTDATA val;
    float *last_intrp, *z_intrp, *intrp, *temp;
    private float smallest=0, largest=1;
    int x, y, z, num, err;
    uchar *packed;

    assert(outfp != NULL);
    /*
    init the output file's header
    */
    vhout = vhin;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value = &largest;
    /*
    adjust the number of slices
    */
    vhout.scn.num_of_subscenes =
        (short*) malloc(sizeof *(vhout.scn.num_of_subscenes));
    assert(vhout.scn.num_of_subscenes != NULL);
	if (vhout.scn.num_of_subscenes == NULL)
		exit(1);
    vhout.scn.num_of_subscenes[0] = 2 * vhin.scn.num_of_subscenes[0] - 1;
    /*
    adjust the slice spacing
    */
    vhout.scn.loc_of_subscenes = (float*) malloc(
        (2 * vhin.scn.num_of_subscenes[0] - 1) * sizeof *(vhout.scn.loc_of_subscenes) );
    assert(vhout.scn.loc_of_subscenes != NULL);
	if (vhout.scn.loc_of_subscenes == NULL)
		exit(1);
    for (z = 0; z < vhout.scn.num_of_subscenes[0]; z++)  {
        if (_even(z))
            vhout.scn.loc_of_subscenes[z] = vhin.scn.loc_of_subscenes[z/2];
        else
            vhout.scn.loc_of_subscenes[z] = (vhin.scn.loc_of_subscenes[z/2] +
                                             vhin.scn.loc_of_subscenes[z/2+1]) / 2;
    }
    /*
    adjust the x & y pixel size
    */
    vhout.scn.xysize[0] = 2 * vhin.scn.xysize[0] - 1;
    vhout.scn.xysize[1] = 2 * vhin.scn.xysize[1] - 1;
    vhout.scn.xypixsz[0] /= 2;
    vhout.scn.xypixsz[1] /= 2;
    vhout.scn.num_of_bits = 1;
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  puts("Can't write the output file header.");  exit(-1);  }
    err = VSeekData(outfp, 0);
    assert(err == 0);
    /*
    grab some memory for the interpolated data & and the previous results of
    interpolation
    */
    intrp = (float*) malloc(vhout.scn.xysize[0] * vhout.scn.xysize[1] *
                             sizeof *intrp);
    assert(intrp != NULL);
	if (intrp == NULL)
		exit(1);
    z_intrp = (float*) malloc(vhout.scn.xysize[0] * vhout.scn.xysize[1] *
                                  sizeof *z_intrp);
    assert(z_intrp != NULL);
	if (z_intrp == NULL)
		exit(1);
    last_intrp = (float*) malloc(vhout.scn.xysize[0] * vhout.scn.xysize[1] *
                                  sizeof *last_intrp);
    assert(last_intrp != NULL);
	if (last_intrp == NULL)
		exit(1);
    /*
    process each slice
    */
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        /*
        initialize the interpolated data to the result of the distance transform
        */
        for (y = 0; y < vhin.scn.xysize[1]; y++)
          for (x = 0; x < vhin.scn.xysize[0]; x++)  {
            val = get_outdata(x,y,z);
            if (!get_indata(x,y,z))     val = -val;
            intrp[idx2d(x*2,y*2)] = val;
          }
        /*
        interpolate the data
        */
        for (y = 0; y < vhout.scn.xysize[1] - 2; y+=2)
          for (x = 0; x < vhout.scn.xysize[0] - 2; x+=2)  {
            intrp[idx2d(x+1,y)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x+2,y)]) / 2.0);
            intrp[idx2d(x,y+1)] = (float)((intrp[idx2d(x,y)] + intrp[idx2d(x,y+2)]) / 2.0);
            intrp[idx2d(x+1,y+1)] = (float)(( intrp[idx2d(x,y)] +
                                      intrp[idx2d(x+2,y)] +
									  intrp[idx2d(x,y+2)] +
                                      intrp[idx2d(x+2,y+2)] ) / 4.0);
          }  /* end for x */
        /*
        interpolate the last row
        */
        y = vhout.scn.xysize[1] - 1;
        for (x = 0; x < vhout.scn.xysize[0] - 2; x+=2)
            intrp[idx2d(x+1,y)] = (float)((intrp[idx2d(x,y)] +
                                  intrp[idx2d(x+2,y)]) / 2.0);
        /*
        interpolate the last col
        */
        x = vhout.scn.xysize[0] - 1;
        for (y = 0; y < vhout.scn.xysize[1] - 2; y+=2)
            intrp[idx2d(x,y+1)] = (float)((intrp[idx2d(x,y)] +
                                  intrp[idx2d(x,y+2)]) / 2.0);
        /*
        before we write out this new interpolated slice, we must interpolate the
        extra z slice and write it out first (if z != 0)
        */
        if (z)  {
            /*
            interpolate the new, intervening z slice (z_intrp)
            */
            interpolate_new_z_slice(&vhout, last_intrp, z_intrp, intrp);
            /*
            convert the new z slice back to binary & write it out
            */
            packed = convert2binary(z_intrp, &vhout);
            err = VWriteData((char*)packed, sizeof *packed, packed_bytes, outfp, &num);
            assert(err == 0 && num == packed_bytes);
            free(packed);
        }
        /*
        convert back to binary & write it out
        */
        /* if (z == 4)  helper(&vhout, intrp); */
        packed = convert2binary(intrp, &vhout);
        err = VWriteData((char*)packed, sizeof *packed, packed_bytes, outfp, &num);
        assert(err == 0 && num == packed_bytes);
        free(packed);
        /*
        switch last_intrp & intrp
        */
        temp       = last_intrp;
        last_intrp = intrp;
        intrp      = temp;
    }  /* end for z */

    free(last_intrp);   last_intrp = NULL;
    free(z_intrp);      z_intrp    = NULL;
    free(intrp);        intrp      = NULL;
    fclose(outfp);
}
/************************************************************************/
/*
This function saves the output data.
*/
#ifdef  __STDC__
private void save_outdata ( void )  {
#else
private void save_outdata ( )  {
#endif
/*
This function scales the output to 8-bits [0-255].
 Modified: 4/2/97 vhout.scn.bit_fields corrected by Dewey Odhner.
 Modified: 9/28/99 -f flag added by Dewey Odhner.
 Modified: 12/22/99 output scaled with -f flag by Dewey Odhner.
 Modified: 2/8/00 vhout.scn.domain and .measurement_unit set by Dewey Odhner.
*/
    ViewnixHeader vhout;
    int x, y, z, num, err;
    OUTDATA min = INF, max = -INF, val, *outptr, *outend;
    private float smallest, largest;
	uchar *outcptr;
#if     0
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        printf("\nz = %d\n", z);
        for (y = 0; y < vhin.scn.xysize[1]; y++)  {
            if (y != 4)         continue;
            printf("y=%2d:", y);
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                float v = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) v = -v;
                printf("%5.1lf", v);
            }  /* end for x */
            puts("");
        }  /* end for y */
    }  /* end for z */
#endif
    if (fflag)  {
        min = 0;
        for (outptr = outdata, outend = outdata+vhin.scn.xysize[0]*
                vhin.scn.xysize[1]*vhin.scn.num_of_subscenes[0];
                outptr < outend; outptr++)  {
            val = *outptr;
            if (val > max)  max = val;
        }
    }
    else
        for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
            for (y = 0; y < vhin.scn.xysize[1]; y++)
                for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                    val = get_outdata(x,y,z);
                    if (!get_indata(x,y,z)) val = -val;
                    if (val < min)  min = val;
                    if (val > max)  max = val;
                }
#ifdef  OUTDATA_FLOAT
    fprintf(stderr, "min = %.2f, max = %.2f\n", min, max);
#endif
#ifdef  OUTDATA_SHORT
    fprintf(stderr, "min = %d, max = %d\n", (int)min, (int)max);
#endif
    fflush(stderr);

if (min < 0 && -min > max)      max = (-min);
min = 0;

    vhout = vhin;
    smallest = 0;
    if (fflag || max-min > 255)  largest = 255;
    else                largest = max-min;
    vhout.scn.num_of_density_values  = 1;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value  = &largest;
    vhout.scn.num_of_bits = 8;
	vhout.scn.bit_fields[1] = 7;
    /*
    write the output header
    */
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  fprintf(stderr, "Can't write the output file header.\n");  exit(-1);  }
    err = VSeekData(outfp, 0);                  assert(err == 0);
    if (fflag)  {
        min = 0;
        for (outptr = outdata, outend = outdata+vhin.scn.xysize[0]*
                vhin.scn.xysize[1]*vhin.scn.num_of_subscenes[0],
                outcptr = (uchar *)outdata; outptr < outend;
                outptr++, outcptr++)
            *outcptr = (uchar)ceil(255/max* *outptr);
        err = VWriteData((char *)outdata, 1, vhin.scn.xysize[0]*
                vhin.scn.xysize[1]*vhin.scn.num_of_subscenes[0], outfp, &num);
        if (err)
        {  fprintf(stderr, "Can't write the output file data.\n");  exit(-1); }
    }
    else
      for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                uchar   ucout;
                val = get_outdata(x,y,z);
                /*
                if (!get_indata(x,y,z)) val = -val;
                */
                if (max-min > 255)      ucout = (uchar)(((val - min) * 255) / (max - min));
                else                    ucout = (uchar)(val - min);
                err = VWriteData((char *)&ucout, sizeof ucout, 1, outfp, &num);
                assert(err == 0 && num == 1);
            }
}
/************************************************************************/
/*
This function saves the output data.
*/
#ifdef  __STDC__
private void save_outdata_dflag ( void )  {
#else
private void save_outdata_dflag ( )  {
#endif
/*
This function scales all of the inside distance values into 8-bits [0-255]
and then writes out each slice.

Then it scales all of the outside distance values into 8-bits [0-255] and
then writes out each slice.
 Modified: 4/2/97 vhout.scn.bit_fields corrected by Dewey Odhner.
 Modified: 2/9/00 vhout.scn.domain and .measurement_unit set by Dewey Odhner.
*/
    ViewnixHeader vhout;
    int x, y, z, num, err;
    OUTDATA min = INF, max = -INF, val;
    private float smallest, largest;

    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                val = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) val = -val;
                if (val < min)  min = val;
                if (val > max)  max = val;
            }
#ifdef  OUTDATA_FLOAT
    fprintf(stderr, "min = %.2f, max = %.2f\n", min, max);
#endif
#ifdef  OUTDATA_SHORT
    fprintf(stderr, "min = %d, max = %d\n", (int)min, (int)max);
#endif
    vhout = vhin;
    smallest = 0;
    largest = max;
    if (-min > largest) largest = -min;
    if (largest > 255)  largest = 255;
    vhout.scn.num_of_density_values  = 1;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value  = &largest;
    vhout.scn.num_of_bits = 8;
	vhout.scn.bit_fields[1] = 7;
    vhout.scn.num_of_subscenes = (short*) malloc(sizeof *vhout.scn.num_of_subscenes);
    assert(vhout.scn.num_of_subscenes != NULL);
	if (vhout.scn.num_of_subscenes == NULL)
		exit(1);
    vhout.scn.num_of_subscenes[0] = vhin.scn.num_of_subscenes[0] * 2;
    /*
    write the output header
    */
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  fprintf(stderr, "Can't write the output file header.\n");  exit(-1);  }
    err = VSeekData(outfp, 0);                  assert(err == 0);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        /*
        do the inside values first
        */
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                uchar   ucout;
                val = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) val = -val;
                if (val < 0)            ucout = 0;
                else if (max > 255)     ucout = (uchar)(val * 255 / max);
                else if (-min > 255)    ucout = (uchar)(val * 255 / -min);
                else                    ucout = (uchar)val;
                err = VWriteData((char *)&ucout, sizeof ucout, 1, outfp, &num);
                assert(err == 0 && num == 1);
            }
    }
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        /*
        do the outside values
        */
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                uchar   ucout;
                val = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) val = -val;
                if (val > 0)            ucout = 0;
                else if (max > 255)     ucout = (uchar)(-val * 255 / max);
                else if (-min > 255)    ucout = (uchar)(-val * 255 / -min);
                else                    ucout = (uchar)-val;
                err = VWriteData((char *)&ucout, sizeof ucout, 1, outfp, &num);
                assert(err == 0 && num == 1);
            }
    }
}
/************************************************************************/
/*
This function saves the output data (as unsigned shorts).  (oflag for
(Dewey) Odhner oflag.)
 Modified: 4/2/97 vhout.scn.bit_fields corrected by Dewey Odhner.
 Modified: 2/9/00 vhout.scn.domain and .measurement_unit set by Dewey Odhner.
*/
#ifdef  __STDC__
private void save_outdata_oflag ( void )  {
#else
private void save_outdata_oflag ( )  {
#endif
#   define _translate_and_scale(V)      (                               \
                ((V - min) * MAXSHORT / (max - min)) +                  \
                (MAXSHORT - ((0 - min) * MAXSHORT / (max - min)))       \
                )
    ViewnixHeader vhout;
    int x, y, z, num, err;
    OUTDATA min = INF, max = -INF, val;
    private float smallest, largest;
    ushort *usslice, *usout;

    fprintf(stderr, "\nSaving\t\t\t");  fflush(stderr);

    usslice = (ushort*) malloc(vhin.scn.xysize[0] * vhin.scn.xysize[1] *
                               sizeof *usslice);
    assert(usslice != NULL);
	if (usslice == NULL)
		exit(1);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                val = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) val = -val;
                if (val < min)  min = val;
                if (val > max)  max = val;
            }
#   ifdef       OUTDATA_FLOAT
      fprintf(stderr, "min = %.2f, max = %.2f\n", min, max);
#   endif
#   ifdef       OUTDATA_SHORT
      fprintf(stderr, "min = %d, max = %d\n", (int)min, (int)max);
#   endif
    fflush(stderr);

    vhout = vhin;
    smallest = _translate_and_scale(min);
    largest  = _translate_and_scale(max);
    vhout.scn.num_of_density_values = 1;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value = &largest;
    vhout.scn.num_of_bits = 16;
	vhout.scn.bit_fields[1] = 15;
    /*
    write the output header
    */
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        {  fprintf(stderr, "Can't write the output file header.\n");  exit(-1);  }
    /*
    output the distance map
    */
    err = VSeekData(outfp, 0);                  assert(err == 0);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        usout = usslice;
        fprintf(stderr, ".");           fflush(stderr);
        for (y = 0; y < vhin.scn.xysize[1]; y++)  {
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                val = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) val = -val;
                val = _translate_and_scale(val);
                *usout++ = (ushort)val;
            }  /* end for x */
        }  /* end for y */
        /* write out a slice of the distance map */
        err = VWriteData((char *)usslice, sizeof *usslice,
                         vhin.scn.xysize[0] * vhin.scn.xysize[1], outfp, &num);
        assert(err == 0 && num == (vhin.scn.xysize[0] * vhin.scn.xysize[1]));
    }  /* end for z */

    free(usslice);
}
/************************************************************************/
/*
This function saves the output data (as unsigned shorts) by simply adding
MAXSHORT to each distance value.  (sflag is for Supun's flag.)
 Modified: 4/2/97 vhout.scn.bit_fields corrected by Dewey Odhner.
 Modified: 2/9/00 vhout.scn.domain and .measurement_unit set by Dewey Odhner.
 Modified: 7/22/10 negative values set to zero by Dewey Odhner.
*/
#ifdef  __STDC__
private void save_outdata_sflag ( void )  {
#else
private void save_outdata_sflag ( )  {
#endif
    ViewnixHeader vhout;
    int x, y, z, num, err, inf_flag;
/*gjg
    long min = MAXLONG, max = -MAXLONG, val;
*/
    OUTDATA min = INF, max = -INF, val;
    private float smallest, largest;
    ushort *usslice, *usout;

    fprintf(stderr, "\nSaving\t\t\t");  fflush(stderr);

    usslice = (ushort*) malloc(vhin.scn.xysize[0] * vhin.scn.xysize[1] *
                               sizeof *usslice);
    assert(usslice != NULL);
	if (usslice == NULL)
		exit(1);
    /*
    find the min & max but ignore infinities
    */
    inf_flag = 0;
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
        for (y = 0; y < vhin.scn.xysize[1]; y++)
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                val = get_outdata(x,y,z);
                if (val < INF)  {
                    if (!get_indata(x,y,z))     val = -val;
                    if (val < min)              min =  val;
                    if (val > max)              max =  val;
                }
                else
                    inf_flag = 1;
            }
    /*
    this code fixes infinities to reasonable values
    */
    if (inf_flag)  {
        fprintf(stderr, "fixing infinities . . .\n");  fflush(stderr);
        for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)
            for (y = 0; y < vhin.scn.xysize[1]; y++)
                for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                    val = get_outdata(x,y,z);
                    if (val >= INF)  {
                        /* found one! */

                        if (get_indata(x,y,z))  set_outdata(x,y,z, max);
                        else  {
                            if (min < 0)  set_outdata(x,y,z, -min);
                            else          set_outdata(x,y,z,  min);
                        }
                    }
                }
    }
#   ifdef       OUTDATA_FLOAT
      fprintf(stderr, "min = %.2f, max = %.2f\n", min, max);
#   endif
#   ifdef       OUTDATA_SHORT
      fprintf(stderr, "min = %d, max = %d\n", (int)min, (int)max);
#   endif
    fflush(stderr);

    vhout = vhin;
    smallest = min + MAXSHORT;
    largest  = max + MAXSHORT;
    vhout.scn.num_of_density_values  = 1;
    vhout.scn.smallest_density_value = &smallest;
    vhout.scn.largest_density_value  = &largest;
    vhout.scn.num_of_bits = 16;
	vhout.scn.bit_fields[1] = 15;
    /*
    write the output header
    */
    err = VWriteHeader(outfp, &vhout, group, element);
    if (err && err < 106)
        { fprintf(stderr, "Can't write the output file header.\n"); exit(-1); }
    /*
    output the distance map
    */
    err = VSeekData(outfp, 0);                  assert(err == 0);
    for (z = 0; z < vhin.scn.num_of_subscenes[0]; z++)  {
        usout = usslice;
        fprintf(stderr, ".");           fflush(stderr);
        for (y = 0; y < vhin.scn.xysize[1]; y++)  {
            for (x = 0; x < vhin.scn.xysize[0]; x++)  {
                val = get_outdata(x,y,z);
                if (!get_indata(x,y,z)) val = -val;
                val += MAXSHORT;
				if (val < 0) val = 0;
                *usout++ = (ushort)val;
            }  /* end for x */
        }  /* end for y */
        /* write out a slice of the distance map */
        err = VWriteData((char *)usslice, sizeof *usslice,
                         vhin.scn.xysize[0] * vhin.scn.xysize[1], outfp, &num);
        assert(err == 0 && num == (vhin.scn.xysize[0] * vhin.scn.xysize[1]));
    }  /* end for z */

    free(usslice);
}
/************************************************************************/
/*
This function frees memory and closes files.
*/
#ifdef  __STDC__
private void finish ( void )  {
#else
private void finish ( )  {
#endif
    if (indata != NULL) {  free(indata);   indata = NULL;       }
    if (outdata != NULL){  free(outdata);  outdata = NULL;      }
    /*
    close files
    */
    if (infp != NULL)   {  VCloseData(infp);    infp = NULL;   }
    if (outfp != NULL)  {  VCloseData(outfp);   outfp = NULL;  }
}
/************************************************************************/
#ifdef  __STDC__
public int main ( int argc, char* argv[] )  {
#else
public int main ( argc, argv )
int argc;
char* argv[];
{
#endif
    time_t start = time(NULL);
    init(argc, argv);
    switch (pflag)  {
        case (XDIR | YDIR | ZDIR) :     distance_transform_xyz();       break;
        case (YDIR | ZDIR) :            distance_transform_yz();        break;
        case (XDIR | YDIR) :            distance_transform_xy();        break;
        case (XDIR | ZDIR) :            distance_transform_xz();        break;
        case XDIR :                     distance_transform_x();         break;
        case YDIR :                     distance_transform_y();         break;
        case ZDIR :                     distance_transform_z();         break;
        default :                       assert(0);                      break;
    }
    /*
    save the distance map
    */
    if (dflag)          save_outdata_dflag();
    else if (oflag)     save_outdata_oflag();
    else if (sflag)     save_outdata_sflag();
    else                save_outdata();
    /*
    save the interpolated data if necessary
    */
    if (iflag == XDIR)                  interpolate_x();
    else if (iflag == YDIR)             interpolate_y();
    else if (iflag == (XDIR | YDIR))    interpolate_xy();
    else if (iflag == ALL)              interpolate_xyz();
    else                                puts("no interpolation");

    finish();
    fprintf(stderr, "\nElapsed time = %ld sec.\n", (long)(time(NULL)-start));
    exit(0);
}
/************************************************************************/
