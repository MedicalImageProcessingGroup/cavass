/*
  Copyright 1993-2012, 2017 Medical Image Processing Group
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
This standalone program converts mathematica, matlab, r, or vtk formats to
an IM0.

Issues:  need to determine x, y, z number of pixels/voxels.
         need to determine min & max values.
         handle negative values.

set VIEWNIX_ENV=c:\documents and settings\ggrevera\my documents\cavass-build\debug
..\..\..\cavass-build\Debug\importMath.exe expmth-tmp.m mathematica fred.IM0 256 256 10 0.5 0.5 1.0
*/
#include  <assert.h>
#include  <ctype.h>
#include  <Viewnix.h>
#include  "cv3dv.h"
#include  <limits.h>
#include  "itk_tiff_mangle.h"

static bool  verbose = true;
//----------------------------------------------------------------------
/**
 * simply plow through all of the numbers (ints) in a file and determine
 * the min and max.
 */
static void getInfoFromVtk ( char* inname,
    int& min, int& max, int& bits, bool& ascii, int& count, int& skipCount )
{
    if (::verbose)    puts( "determining min and max, etc." );

    min       = INT_MAX;
    max       = INT_MIN;
    bits      = 0;
    ascii     = true;
    count     = 0;
    skipCount = 0;

    //if necessary, determine number of header lines to skip
    char  buff[ BUFSIZ ];
    FILE*  fp = fopen( inname, "rb" );    assert( fp != NULL );
    while (!feof(fp)) {
        fgets( buff, BUFSIZ, fp );
        if (buff[0]=='#' || isalpha(buff[0])) {
            ++skipCount;
            if (strstr(buff,"ASCII") == buff || strstr(buff,"ascii"))
                ascii = true;
            else if (strstr(buff,"BINARY") == buff || strstr(buff,"binary") == buff)
                ascii = false;
            else if (strstr(buff,"SCALARS") == buff || strstr(buff,"scalars") == buff) {
                if (strstr(buff," int ") != NULL || strstr(buff," INT ") != NULL)
                    bits = 32;
                else if (strstr(buff," short ") != NULL || strstr(buff," SHORT ") != NULL)
                    bits = 16;
                else if (strstr(buff," char ") != NULL || strstr(buff," char ") != NULL)
                    bits = 8;
                else {
                    assert( 0 );
                }
            } else if ((strstr(buff,"LOOKUP_TABLE") != NULL || strstr(buff,"lookup_table") != NULL) && !ascii) {
                break;
            }
        } else {
            break;
        }
    }
    fclose( fp );    fp = NULL;

    if (::verbose)    printf( "skip count = %d \n", skipCount );

    fp = fopen( inname, "rb" );    assert( fp != NULL );
    //skip header lines
    for (int i=0; i<skipCount; i++) {
        fgets( buff, BUFSIZ, fp );
        if (::verbose)    puts( buff );
    }
    if (ascii) {
        if (::verbose)    puts( "ascii" );
        while (!feof(fp)) {
            //try to read the next integer value
            int  value = 0;
            int  n = fscanf( fp, "%d", &value );
            if (n==1) {
                //success
                if (value < min)    min = value;
                if (value > max)    max = value;
                ++count;
            } else {
                //failure - skip a char
                getc( fp );
            }
        }
    } else {
        if (::verbose)    puts( "binary" );
        while (!feof(fp)) {
            int  value;
            size_t  n = fread( &value, sizeof( value ), 1, fp );
            if (n!=1)    break;
            //success
            if (value < min)    min = value;
            if (value > max)    max = value;
            ++count;
        }
    }
    fclose( fp );    fp = NULL;
    if (::verbose)
        printf( "min=%d, max=%d, count=%d \n", min, max, count );
}
//----------------------------------------------------------------------
/*
  VTK format (from http://www.vtk.org/VTK/img/file-formats.pdf)

      # vtk DataFile Version 2.0
      Volume example
      ASCII                                (\todo support binary too)
      DATASET STRUCTURED_POINTS
      DIMENSIONS 3 4 6
      SPACING 0.5 0.5 1.0                  (\todo or possibly ASPECT_RATIO 1 1 1)
      ORIGIN 0 0 0
      POINT_DATA 72
      SCALARS volume_scalars char 1
      LOOKUP_TABLE default
      0 0 0 0 0 0 0 0 0 0 0 0
      0 5 10 15 20 25 25 20 15 10 5 0
      0 10 20 30 40 50 50 40 30 20 10 0
      0 10 20 30 40 50 50 40 30 20 10 0
      0 5 10 15 20 25 25 20 15 10 5 0
      0 0 0 0 0 0 0 0 0 0 0 0

  VTK format not supported:

    <VTKFile type=ImageData ...>
        <ImageData WholeExtent=x1 x2 y1 y2 z1 z2
             Origin=x0 y0 z0 Spacing=dx dy dz>
            <Piece Extent=x1 x2 y1 y2 z1 z2>
                <PointData>...</PointData>
                <CellData>...</CellData>
            </Piece>
        </ImageData>
    </VTKFile>
*/
static void readVtkAndWrite ( char* inname, char* outname,
                              int xSize, int ySize, int zSize,
                              double xSpace, double ySpace, double zSpace,
                              bool skipHeader=true )
{
    int   min=0, max=0, bits=0, count=0, skipCount=0;
    bool  ascii=false;

    //determine min, max, and skipCount (# of header lines to skip)
    getInfoFromVtk( inname, min, max, bits, ascii, count, skipCount );

    assert( min >= 0 );
    assert( max <= USHRT_MAX );
    assert( bits==8 || bits==16 || bits==32 );
    assert( count >= (xSize * ySize * zSize) );  //sanity check

    float  smallest_density_value = (float) min;
    float  largest_density_value  = (float) max;
    short  num_of_bits = (max <= UCHAR_MAX) ? 8 : 16;

    ViewnixHeader  vh;
    memset( &vh, 0, sizeof(vh) );  //set all fields to empty/invalid

    //init contents of header

    //gen
    vh.gen.recognition_code_valid = 1;
    strcpy( vh.gen.recognition_code, "VIEWNIX1.0" );

    vh.gen.data_type_valid = 1;
    vh.gen.data_type = 0;  //for IM0

    //scn
    vh.scn.bit_fields_valid = 1;
    short  bit_fields = 0;
    vh.scn.bit_fields = &bit_fields;

    vh.scn.bytes_in_alignment_valid = 1;
    vh.scn.bytes_in_alignment = 1;

    vh.scn.dimension_valid = 1;
    vh.scn.dimension = 3;

    vh.scn.dimension_in_alignment_valid = 1;
    vh.scn.dimension_in_alignment = 2;

    vh.scn.measurement_unit_valid = 1;
    short  measurement_unit = 3;  //mm
    vh.scn.measurement_unit = &measurement_unit;

    vh.scn.num_of_density_values_valid = 1;
    vh.scn.num_of_density_values = 1;

    vh.scn.num_of_integers_valid = 1;
    vh.scn.num_of_integers = 1;

    vh.scn.signed_bits_valid = 1;
    short  signed_bits = 0;
    vh.scn.signed_bits = &signed_bits;

    vh.scn.xypixsz_valid = 1;
    vh.scn.xypixsz[0] = (float) xSpace;
    vh.scn.xypixsz[1] = (float) ySpace;

    vh.scn.num_of_subscenes_valid = 1;
    short  tmp = zSize;
    vh.scn.num_of_subscenes = &tmp;

    vh.scn.loc_of_subscenes_valid = 1;
    vh.scn.loc_of_subscenes = (float*) malloc( zSize * sizeof(float) );
    assert( vh.scn.loc_of_subscenes != NULL );
    for (int z=0; z<zSize; z++) {
        vh.scn.loc_of_subscenes[z] = (float) (z * zSpace);
    }

    vh.scn.xysize_valid = 1;
    vh.scn.xysize[0] = xSize;
    vh.scn.xysize[1] = ySize;

    vh.scn.smallest_density_value_valid = 1;
    vh.scn.smallest_density_value = &smallest_density_value;

    vh.scn.largest_density_value_valid = 1;
    vh.scn.largest_density_value = &largest_density_value;

    vh.scn.num_of_bits_valid = 1;
    vh.scn.num_of_bits = num_of_bits;

    //write header
    FILE*  out = fopen( outname, "wb+" );
    char  group[5], element[5];
    int  err = VWriteHeader( out, &vh, group, element );
    if (err && err < 106) {
        fprintf( stderr, "Can't write the output file header, %d. \n", err );
        exit( -1 );
    }

    //process slice data
    FILE*  in = fopen( inname, "rb" );    assert( in != NULL );
    if (skipHeader) {
        char  buff[ BUFSIZ ];
        //skip header lines
        for (int i=0; i<skipCount; i++) {
            fgets( buff, BUFSIZ, in );
        }
    }
    VSeekData( out, 0 );
    if (num_of_bits==8) {
        //allocate a slice
        unsigned char*  slice = (unsigned char*) malloc( xSize * ySize );
        assert( slice != NULL );

        for (int z=0; z<zSize; z++) {
            if (::verbose)    printf( "processing slice=%d \n", z );
            for (int i=0; i<xSize*ySize; i++) {
                //read the next int
                assert( !feof(in) );
                int  value = 0;
                size_t  n = 0;
                if (ascii) {
                    n = fscanf( in, "%d", &value );
                } else {
                    n = fread( &value, sizeof( value ), 1, in );
                }
                assert( n == 1 );
                slice[i] = value;
            }
            //write a slice
            int  items_written = 0;
            VWriteData( (char*) slice, 1, xSize*ySize, out, &items_written );
            assert( items_written = xSize*ySize );
        }

        free( slice );    slice = NULL;
    } else if (num_of_bits==16) {
        //allocate a slice
        unsigned short*  slice = (unsigned short*) malloc( xSize * ySize * 2 );
        assert( slice != NULL );

        for (int z=0; z<zSize; z++) {
            if (::verbose)    printf( "processing slice=%d \n", z );
            for (int i=0; i<xSize*ySize; i++) {
                //read the next int
                assert( !feof(in) );
                int  value = 0;
                size_t  n = 0;
                if (ascii) {
                    n = fscanf( in, "%d", &value );
                } else {
                    n = fread( &value, sizeof( value ), 1, in );
                }
                assert( n == 1 );
                slice[i] = value;
            }
            //write a slice
            int  items_written = 0;
            VWriteData( (char*) slice, 2, xSize*ySize, out, &items_written );
            assert( items_written = xSize*ySize );
        }

        free( slice );    slice = NULL;
    } else {
        assert( 0 );
    }
    fclose( in );    in = NULL;
    VCloseData( out );
}
//----------------------------------------------------------------------
/** \brief output description of command line parameters and then exits
 *  program.
 */
static void usage ( char* msg ) {
    if (msg != NULL)    fputs( msg, stderr );

    fprintf( stderr, "usage: \n" );
    fprintf( stderr, "    importMath  input-file  input-file-type  output-file-name xSize ySize zSize [xSpace ySpace zSpace] \n" );
    fprintf( stderr, "        input-file-type is one of mathematica|matlab|r|vtk. \n" );
    fprintf( stderr, "        xSize ySize zSize - number of pixels/voxels. \n" );
    fprintf( stderr, "        xSpace ySpace zSpace - optional spacing in mm; default = 1.0 \n" );

    exit( 0 );
}
//----------------------------------------------------------------------
/**
 * simply plow through all of the numbers (ascii ints) in a file and 
 * determine the min and max.
 */
static void getMinMax ( char* inname, int& min, int& max, int& count )
{
    if (::verbose)    puts( "determining min and max" );

    FILE*  fp = fopen( inname, "rb" );    assert( fp != NULL );
    min = INT_MAX;
    max = INT_MIN;
    count = 0;
    while (!feof(fp)) {
        //try to read the next integer value
        int  value = 0;
        int  n = fscanf( fp, "%d", &value );
        if (n==1) {
            //success
            if (value < min)    min = value;
            if (value > max)    max = value;
            ++count;
        } else {
            //failure - skip a char
            getc( fp );
        }
    }
    fclose( fp );    fp = NULL;
    if (::verbose)
        printf( "min=%d, max=%d, count=%d \n", min, max, count );
}
//----------------------------------------------------------------------

static int get_int(char *ptr, int swap_bytes)
{
	union {int i; char c[4];} u;
	int j;

	if (!swap_bytes)
		return *((int *)ptr);
	for (j=0; j<4; j++)
		u.c[j] = ptr[3-j];
	return u.i;
}

static short get_short(char *ptr, int swap_bytes)
{
	union {short i; char c[2];} u;
	int j;

	if (!swap_bytes)
		return *((short *)ptr);
	for (j=0; j<2; j++)
		u.c[j] = ptr[1-j];
	return u.i;
}

#define miINT8 1
#define miUINT8 2
#define miINT16 3
#define miUINT16 4
#define miMATRIX 14

static void readMatAndWrite ( char* inname, char* outname,
                           double xSpace, double ySpace, double zSpace )
{
	FILE *in, *out;
	char header[128], tag[8], *data_buf, *out_buf;
	int swap_bytes, value_type, number_of_bytes, numberOfDimensions,
		xSize, ySize, zSize, namelength, j, err;
	ViewnixHeader  vh;
    char  group[5], element[5];
	short bit_fields[2], num_of_bits, signed_bits, *data_s;
	unsigned short *data_us;
	unsigned char *data_uc;
	signed char *data_c;
	float smallest_density_value=65536, largest_density_value=-32768;

	in = fopen( inname, "rb" );
	if (in == NULL)
	{
		fprintf(stderr, "Can't open input file.\n");
		exit(-1);
	}
	if (fread(header, 1, 128, in) != 128)
	{
		fprintf(stderr, "Can't read header.\n");
		exit(-1);
	}
	switch (*((unsigned short *)(header+124)))
	{
		case 0x0100:
			swap_bytes = 0;
			break;
		case 0x0001:
			swap_bytes = 1;
			break;
		default:
			fprintf(stderr, "Not a Level 5 MAT file.\n");
			exit(-1);
	}
	if (fread(tag, 1, 8, in) != 8)
	{
		fprintf(stderr, "Can't read tag.\n");
		exit(-1);
	}
	if (get_int(tag, swap_bytes) != miMATRIX)
	{
		fprintf(stderr, "Non-matrix encountered.\n");
		exit(-1);
	}
	fread(tag, 1, 8, in); // array flags tag
	fread(tag, 1, 8, in); // array flags
	if (fread(tag, 1, 8, in) != 8)
	{
		fprintf(stderr, "Can't read tag.\n");
		exit(-1);
	}
	numberOfDimensions = get_int(tag+4, swap_bytes)/4;
	if (fread(tag, 1, 8, in) != 8)
	{
		fprintf(stderr, "Can't read tag.\n");
		exit(-1);
	}
	if (numberOfDimensions > 3)
	{
		fprintf(stderr, "Too many dimensions.\n");
		exit(-1);
	}
	xSize = get_int(tag+4, swap_bytes);
	ySize = get_int(tag, swap_bytes);
	if (numberOfDimensions == 3)
	{
		if (fread(tag, 1, 8, in) != 8)
		{
			fprintf(stderr, "Can't read tag.\n");
			exit(-1);
		}
		zSize = get_int(tag, swap_bytes);
	}
	else
		zSize = 1;
	if (fread(tag, 1, 8, in) != 8)
	{
		fprintf(stderr, "Can't read tag.\n");
		exit(-1);
	}
	namelength = get_int(tag, swap_bytes)==miINT8?
		get_int(tag+4, swap_bytes): get_short(tag+2, swap_bytes)-4;
	for (j=0; j<(namelength+7)/8; j++)
		fread(tag, 1, 8, in);
	if (fread(tag, 1, 8, in) != 8)
	{
		fprintf(stderr, "Can't read tag.\n");
		exit(-1);
	}
	value_type = get_int(tag, swap_bytes);
	number_of_bytes = get_int(tag+4, swap_bytes);
	switch (value_type)
	{
		case miINT8:
			num_of_bits = 8;
			signed_bits = 1;
			break;
		case miUINT8:
			num_of_bits = 8;
			signed_bits = 0;
			break;
		case miINT16:
			num_of_bits = 16;
			signed_bits = 1;
			break;
		case miUINT16:
			num_of_bits = 16;
			signed_bits = 0;
			break;
		default:
			fprintf(stderr, "Support for this value range not implemented.\n");
			exit(-1);
	}
	assert(number_of_bytes == xSize*ySize*zSize*(num_of_bits/8));
	out_buf = (char *)malloc(number_of_bytes);
	data_buf = (char *)malloc(xSize*ySize*(num_of_bits/8));
	if (data_buf==NULL || out_buf==NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		exit(-1);
	}
	if (xSpace <= 0)
		xSpace = 1;
	if (ySpace <= 0)
		ySpace = 1;
	if (zSpace <= 0)
		zSpace = 1;
    memset( &vh, 0, sizeof(vh) );  //set all fields to empty/invalid

    //init contents of header

    //gen
    vh.gen.recognition_code_valid = 1;
    strcpy( vh.gen.recognition_code, "VIEWNIX1.0" );

    vh.gen.data_type_valid = 1;
    vh.gen.data_type = 0;  //for IM0

    //scn
    vh.scn.bit_fields_valid = 1;
    bit_fields[0] = 0;
	num_of_bits = value_type==miINT8||value_type==miUINT8? 8: 16;
	bit_fields[1] = num_of_bits-1;
    vh.scn.bit_fields = bit_fields;

    vh.scn.bytes_in_alignment_valid = 1;
    vh.scn.bytes_in_alignment = 1;

    vh.scn.dimension_valid = 1;
    vh.scn.dimension = 3;

    vh.scn.dimension_in_alignment_valid = 1;
    vh.scn.dimension_in_alignment = 2;

    vh.scn.num_of_density_values_valid = 1;
    vh.scn.num_of_density_values = 1;

    vh.scn.num_of_integers_valid = 1;
    vh.scn.num_of_integers = 1;

    vh.scn.signed_bits_valid = 1;
    vh.scn.signed_bits = &signed_bits;

    vh.scn.xypixsz_valid = 1;
    vh.scn.xypixsz[0] = (float) xSpace;
    vh.scn.xypixsz[1] = (float) ySpace;

    vh.scn.num_of_subscenes_valid = 1;
    short  tmp = zSize;
    vh.scn.num_of_subscenes = &tmp;

    vh.scn.loc_of_subscenes_valid = 1;
    vh.scn.loc_of_subscenes = (float*) malloc( zSize * sizeof(float) );
    if (vh.scn.loc_of_subscenes == NULL)
	{
		fprintf(stderr, "Out of memory.\n");
		exit(-1);
	}
    for (int z=0; z<zSize; z++) {
        vh.scn.loc_of_subscenes[z] = (float) (z * zSpace);
    }

    vh.scn.xysize_valid = 1;
    vh.scn.xysize[0] = xSize;
    vh.scn.xysize[1] = ySize;

    vh.scn.smallest_density_value_valid = 1;
    vh.scn.smallest_density_value = &smallest_density_value;

    vh.scn.largest_density_value_valid = 1;
    vh.scn.largest_density_value = &largest_density_value;

    vh.scn.num_of_bits_valid = 1;
    vh.scn.num_of_bits = num_of_bits;

	vh.scn.description = header;
	header[116] = 0;
	vh.scn.description_valid = 1;

	data_c = (signed char *)out_buf;
	data_uc = (unsigned char *)out_buf;
	data_s = (short *)out_buf;
	data_us = (unsigned short *)out_buf;
	for (int z=0; z<zSize; z++)
	{
	  if ((int)fread(data_buf, num_of_bits/8, xSize*ySize, in) != xSize*ySize)
	  {
		fprintf(stderr, "Failure reading data.\n");
		exit(-1);
	  }
	  switch (value_type)
	  {
		case miINT8:
			for (int y=0; y<ySize; y++)
				for (int x=0; x<xSize; x++,data_c++)
				{
					*((char *)data_c) = data_buf[x*ySize+y];
					if (*data_c < smallest_density_value)
						smallest_density_value = *data_c;
					if (*data_c > largest_density_value)
						largest_density_value = *data_c;
				}
			break;
		case miUINT8:
			for (int y=0; y<ySize; y++)
				for (int x=0; x<xSize; x++,data_uc++)
				{
					*((char *)data_uc) = data_buf[x*ySize+y];
					if (*data_uc < smallest_density_value)
						smallest_density_value = *data_uc;
					if (*data_uc > largest_density_value)
						largest_density_value = *data_uc;
				}
			break;
		case miINT16:
			for (int y=0; y<ySize; y++)
				for (int x=0; x<xSize; x++,data_s++)
				{
					*data_s = get_short(data_buf+2*x*ySize+2*y, swap_bytes);
					if (*data_s < smallest_density_value)
						smallest_density_value = *data_s;
					if (*data_s > largest_density_value)
						largest_density_value = *data_s;
				}
			break;
		case miUINT16:
			for (int y=0; y<ySize; y++)
				for (int x=0; x<xSize; x++,data_us++)
				{
					*((short *)data_us) =
						get_short(data_buf+2*x*ySize+2*y, swap_bytes);
					if (*data_us < smallest_density_value)
						smallest_density_value = *data_us;
					if (*data_us > largest_density_value)
						largest_density_value = *data_us;
				}
			break;
	  }
	}

    //write header
    out = fopen( outname, "wb+" );
    err = VWriteHeader( out, &vh, group, element );
    if (err && err < 106) {
        fprintf( stderr, "Can't write the output file header, %d. \n", err );
        exit( -1 );
    }

	VSeekData( out, 0 );
	int  items_written = 0;
	if (VWriteData(out_buf, num_of_bits/8, xSize*ySize*zSize,
			out, &items_written ))
	{
		fprintf(stderr, "Failure writing data.\n");
		exit(-1);
	}
	assert( items_written == xSize*ySize*zSize );
	VCloseData( out );
	fclose( in );
	free(vh.scn.loc_of_subscenes);
	free(data_buf);
	free(out_buf);
}
//----------------------------------------------------------------------
/*
  Mathematica format:
      slice1 = { {a, b}, {c, d} };

          a b
          c d

  MatLab/Octave format:
      slice1 = [1 2 0; 2 5 -1; 4 10 -1]

      slice1 =1     2     0
              2     5    -1
              4    10    -1

  R format:
      slice1 = matrix( c(1,2,3,4,5,6), nrow = 3, byrow = T )
      yields
          1 2
          3 4
          5 6
*/
static void readAndWrite ( char* inname, char* outname,
                           int xSize, int ySize, int zSize,
                           double xSpace, double ySpace, double zSpace )
{
    int  min=0, max=0, count=0;
    //determine min, max, and skipCount (# of header lines to skip)
    getMinMax( inname, min, max, count );
    assert( min >= 0 );
    assert( max <= USHRT_MAX );
    assert( count >= (xSize * ySize * zSize) );  //sanity check

    float  smallest_density_value = (float) min;
    float  largest_density_value  = (float) max;
    short  num_of_bits = (max <= UCHAR_MAX) ? 8 : 16;

    ViewnixHeader  vh;
    memset( &vh, 0, sizeof(vh) );  //set all fields to empty/invalid

    //init contents of header

    //gen
    vh.gen.recognition_code_valid = 1;
    strcpy( vh.gen.recognition_code, "VIEWNIX1.0" );

    vh.gen.data_type_valid = 1;
    vh.gen.data_type = 0;  //for IM0

    //scn
    vh.scn.bit_fields_valid = 1;
    short  bit_fields = 0;
    vh.scn.bit_fields = &bit_fields;

    vh.scn.bytes_in_alignment_valid = 1;
    vh.scn.bytes_in_alignment = 1;

    vh.scn.dimension_valid = 1;
    vh.scn.dimension = 3;

    vh.scn.dimension_in_alignment_valid = 1;
    vh.scn.dimension_in_alignment = 2;

    vh.scn.measurement_unit_valid = 1;
    short  measurement_unit = 3;  //mm
    vh.scn.measurement_unit = &measurement_unit;

    vh.scn.num_of_density_values_valid = 1;
    vh.scn.num_of_density_values = 1;

    vh.scn.num_of_integers_valid = 1;
    vh.scn.num_of_integers = 1;

    vh.scn.signed_bits_valid = 1;
    short  signed_bits = 0;
    vh.scn.signed_bits = &signed_bits;

    vh.scn.xypixsz_valid = 1;
    vh.scn.xypixsz[0] = (float) xSpace;
    vh.scn.xypixsz[1] = (float) ySpace;

    vh.scn.num_of_subscenes_valid = 1;
    short  tmp = zSize;
    vh.scn.num_of_subscenes = &tmp;

    vh.scn.loc_of_subscenes_valid = 1;
    vh.scn.loc_of_subscenes = (float*) malloc( zSize * sizeof(float) );
    assert( vh.scn.loc_of_subscenes != NULL );
    for (int z=0; z<zSize; z++) {
        vh.scn.loc_of_subscenes[z] = (float) (z * zSpace);
    }

    vh.scn.xysize_valid = 1;
    vh.scn.xysize[0] = xSize;
    vh.scn.xysize[1] = ySize;

    vh.scn.smallest_density_value_valid = 1;
    vh.scn.smallest_density_value = &smallest_density_value;

    vh.scn.largest_density_value_valid = 1;
    vh.scn.largest_density_value = &largest_density_value;

    vh.scn.num_of_bits_valid = 1;
    vh.scn.num_of_bits = num_of_bits;

    //write header
    FILE*  out = fopen( outname, "wb+" );
    char  group[5], element[5];
    int  err = VWriteHeader( out, &vh, group, element );
    if (err && err < 106) {
        fprintf( stderr, "Can't write the output file header, %d. \n", err );
        exit( -1 );
    }

    //process slice data
    FILE*  in = fopen( inname, "rb" );    assert( in != NULL );
    VSeekData( out, 0 );
    if (num_of_bits==8) {
        //allocate a slice
        unsigned char*  slice = (unsigned char*) malloc( xSize * ySize );
        assert( slice != NULL );

        for (int z=0; z<zSize; z++) {
            if (::verbose)    printf( "processing slice=%d \n", z );
            for (int i=0; i<xSize*ySize; i++) {
                //read the next int
                while (!feof(in)) {
                    int  value = 0;
                    int  n = fscanf( in, "%d", &value );
                    if (n==1) {
                        slice[i] = value;
                        break;
                    } else {
                        //failure - skip a char
                        getc( in );
                    }
                }
            }
            //write a slice
            int  items_written = 0;
            if (VWriteData( (char*) slice, 1, xSize*ySize, out,
                    &items_written ))
            {
                fprintf(stderr, "Failure writing data.\n");
                exit(-1);
            }
            assert( items_written == xSize*ySize );
        }

        free( slice );    slice = NULL;
    } else if (num_of_bits==16) {
        //allocate a slice
        unsigned short*  slice = (unsigned short*) malloc( xSize * ySize * 2 );
        assert( slice != NULL );

        for (int z=0; z<zSize; z++) {
            if (::verbose)    printf( "processing slice=%d \n", z );
            for (int i=0; i<xSize*ySize; i++) {
                //read the next int
                while (!feof(in)) {
                    int  value = 0;
                    int  n = fscanf( in, "%d", &value );
                    if (n==1) {
                        slice[i] = value;
                        break;
                    } else {
                        //failure - skip a char
                        getc( in );
                    }
                }
            }
            //write a slice
            int  items_written = 0;
            if (VWriteData( (char*) slice, 2, xSize*ySize, out,
                    &items_written ))
            {
                fprintf(stderr, "Failure reading data.\n");
                exit(-1);
            }
            assert( items_written = xSize*ySize );
        }

        free( slice );    slice = NULL;
    } else {
        assert( 0 );
    }
    fclose( in );    in = NULL;
    VCloseData( out );
}
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    if (verbose) {
        fprintf( stderr, "VIEWNIX_ENV=%s \n", getenv("VIEWNIX_ENV") );
    }
    if (argc < 7)    usage( 0 );

    char* pEnd;

    int  xSize = strtol( argv[4], &pEnd, 10 );
    assert( *pEnd == 0 );
    if (*pEnd != 0)    usage( (char *)"please specify a valid xSize" );

    int  ySize = strtol( argv[5], &pEnd, 10 );
    assert( *pEnd == 0 );
    if (*pEnd != 0)    usage( (char *)"please specify a valid ySize" );

    int  zSize = strtol( argv[6], &pEnd, 10 );
    assert( *pEnd == 0 );
    if (*pEnd != 0)    usage( (char *)"please specify a valid zSize" );

    double  xSpace = 1.0, ySpace = 1.0, zSpace = 1.0;
    if (argc > 7) {
        //parse spacing specified on command line
        assert( argc >= 10 );

        xSpace = strtod( argv[7], &pEnd );
        assert( *pEnd == 0 );
        if (*pEnd != 0)    usage( (char *)"please specify a valid xSpace" );

        ySpace = strtod( argv[8], &pEnd );
        assert( *pEnd == 0 );
        if (*pEnd != 0)    usage( (char *)"please specify a valid ySpace" );

        zSpace = strtod( argv[9], &pEnd );
        assert( *pEnd == 0 );
        if (*pEnd != 0)    usage( (char *)"please specify a valid zSpace" );
    }

    if ( strcmp("matlab",argv[2]) == 0 )
		readMatAndWrite( argv[1], argv[3], xSpace, ySpace, zSpace );
	else if ( strcmp("mathematica",argv[2]) == 0
      || strcmp("octave",argv[2]) == 0
      || strcmp("r",argv[2]) == 0 || strcmp("R",argv[2]) == 0) {
        readAndWrite( argv[1], argv[3], xSize, ySize, zSize, xSpace, ySpace, zSpace );
    } else if (strcmp("vtk",argv[2])==0 || strcmp("VTK",argv[2])==0) {
        //could be ascii or binary
        readVtkAndWrite( argv[1], argv[3], xSize, ySize, zSize, xSpace, ySpace, zSpace );
    } else {
        usage( (char *)"unsupported output file type" );
    }

    return 0;
}
//----------------------------------------------------------------------
