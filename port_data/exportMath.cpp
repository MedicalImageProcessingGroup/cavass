/*
  Copyright 1993-2012, 2015, 2019 Medical Image Processing Group
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
This standalone program converts IM0s/BIMs to mathematica, matlab, r, or vtk
formats.
*/
#include  "cavass.h"
#include  "ChunkData.h"

using namespace std;

static bool  verbose = true;
//----------------------------------------------------------------------
/** \brief output description of command line parameters and then exits
 *  program.
 */
static void usage ( char* msg ) {
    if (msg != NULL)    fputs( msg, stderr );

    fprintf( stderr, "usage: \n" );
    fprintf( stderr, "    exportMath  input-file  output-file-type  output-file-name  first-slice  last-slice \n" );
    fprintf( stderr, "        output-file-type is one of mathematica|matlab|r|vtk. \n" );
    fprintf( stderr, "        first-slice and last-slice are in [0..n-1]. \n" );

    exit( 1 );
}
//----------------------------------------------------------------------
/*
  Mathematica format:
      slice1 = { {a, b}, {c, d} };

          a b
          c d
*/
static void saveMathematica ( ChunkData* gray, char* outname, int first, int last )
{
    FILE*  fp = fopen( outname, "wb" );
    assert( fp != NULL );
    if (fp == NULL)  usage( (char *)"please specify a valid output file name" );
    for (int z=first; z<=last; z++) {
        printf( "slice=%d \n", (z+1) );    fflush(stdout);
        fprintf( fp, "slice%d = {", (z+1) );
        for (int y=0; y<gray->m_ySize; y++) {
            for (int x=0; x<gray->m_xSize; x++) {
                int  value = gray->getData( x, y, z );
                if (x == 0)    fprintf( fp, " {" );
                fprintf( fp, "%d", value );
                if (x < gray->m_xSize-1)    fprintf( fp, ", " );
                else                        fprintf( fp, "}" );
            }
            if (y < gray->m_ySize-1)    fprintf( fp, ", " );
            else                        fprintf( fp, " " );
        }
        fprintf( fp, " }; \n\n" );
    }

    fclose( fp );    fp = NULL;
}
//----------------------------------------------------------------------
/*
MatLab/Octave format:
  slice1 = [1 2 0; 2 5 -1; 4 10 -1]

  slice1 =1     2     0
          2     5    -1
          4    10    -1
*/
static void saveOctave ( ChunkData* gray, char* outname, int first, int last )
{
    FILE*  fp = fopen( outname, "wb" );
    assert( fp != NULL );
    if (fp == NULL)  usage( (char *)"please specify a valid output file name" );
    for (int z=first; z<=last; z++) {
        printf( "slice=%d \n", (z+1) );    fflush(stdout);
        fprintf( fp, "slice%d = [", (z+1) );
        for (int y=0; y<gray->m_ySize; y++) {
            for (int x=0; x<gray->m_xSize; x++) {
                int  value = gray->getData( x, y, z );
                fprintf( fp, " %d", value );
            }
            if (y < gray->m_ySize-1)    fprintf( fp, ";" );
        }
        fprintf( fp, " ]; \n\n" );
    }

    fclose( fp );    fp = NULL;
}
//----------------------------------------------------------------------
#define miINT8 1
#define miUINT8 2
#define miUINT16 4
#define miINT32 5
#define miUINT32 6
#define mxUINT8_CLASS 9
#define mxUINT16_CLASS 11
#define miMATRIX 14

static void saveMatlab ( ChunkData* gray, char* outname, int first, int last )
{
    FILE*  fp = fopen( outname, "wb" );
    if (fp == NULL)
	  usage( (char *)"please specify a valid output file name" );
    char header[128];
	memset(header, ' ', 124);
	if (gray->m_vh_initialized && gray->m_vh.gen.description_valid)
	{
		int descriplen = strlen(gray->m_vh.gen.description);
		if (descriplen > 116)
			descriplen = 116;
	    strncpy(header, gray->m_vh.gen.description, descriplen);
	}
	else if (gray->m_fname && gray->m_fname[0])
	{
		int descriplen = strlen(gray->m_fname);
		if (descriplen > 116)
			descriplen = 116;
		strncpy(header, gray->m_fname, descriplen);
	}
	else
		strncpy(header, "CAVASS scene", 12);
	*(short *)(header+124) = 0x0100;
	*(short *)(header+126) = (short)(('M'<<8)|'I');
	if (fwrite(header, 1, 128, fp) != 128)
	{
		fprintf(stderr, "Failure writing header.\n");
		exit(-1);
	}
	memset(header, 0, 72);
	*(int *)header = miMATRIX;
	int post_padding=gray->m_xSize*gray->m_ySize*(last-first+1)*gray->m_size%8;
	if (post_padding)
		post_padding = 8-post_padding;
	*(unsigned int *)(header+4) = 64+
	    gray->m_xSize*gray->m_ySize*(last-first+1)*gray->m_size+post_padding;
	*(int *)(header+8) = miUINT32;
	*(unsigned int *)(header+12) = 8;
	*(int *)(header+16) = gray->m_size>1? mxUINT16_CLASS: mxUINT8_CLASS;
	*(int *)(header+24) = miINT32;
	*(int *)(header+28) = 12;
	*(int *)(header+32) = gray->m_ySize;
	*(int *)(header+36) = gray->m_xSize;
	*(int *)(header+40) = last-first+1;
	*(int *)(header+48) = miINT8;
	*(int *)(header+52) = 5;
	strcpy(header+56, "scene");
	*(int *)(header+64) = gray->m_size>1? miUINT16: miUINT8;
	*(unsigned *)(header+68) =
	    gray->m_xSize*gray->m_ySize*(last-first+1)*gray->m_size;
	if (fwrite(header, 1, 72, fp) != 72)
	{
		fprintf(stderr, "Failure writing header.\n");
		exit(-1);
	}
	switch (gray->m_size)
	{
		unsigned char *data_8;
		unsigned short *data_16;
		case 1:
			data_8 = (unsigned char *)malloc(gray->m_xSize*gray->m_ySize);
			if (data_8 == NULL)
			{
				fprintf(stderr, "Out of memory.\n");
				exit(-1);
			}
			for (int z=first; z<=last; z++)
			{
				for (int y=0; y<gray->m_ySize; y++)
					for (int x=0; x<gray->m_xSize; x++)
						data_8[gray->m_ySize*x+y] =
						    (unsigned char)gray->getData( x, y, z );
				if (fwrite(data_8, 1, gray->m_xSize*gray->m_ySize,
					fp) != (unsigned)gray->m_xSize*gray->m_ySize)
				{
					fprintf(stderr, "Failure writing data.\n");
					exit(-1);
				}
			}
			free(data_8);
			break;
		case 2:
			data_16 = (unsigned short *)malloc(gray->m_xSize*gray->m_ySize*2);
			if (data_16 == NULL)
			{
				fprintf(stderr, "Out of memory.\n");
				exit(-1);
			}
			for (int z=first; z<=last; z++)
			{
				for (int y=0; y<gray->m_ySize; y++)
					for (int x=0; x<gray->m_xSize; x++)
						data_16[gray->m_ySize*x+y] =
						    (unsigned short)gray->getData( x, y, z );
				if (fwrite(data_16, 2,gray->m_xSize*gray->m_ySize,
					fp) != (unsigned)gray->m_xSize*gray->m_ySize)
				{
					fprintf(stderr, "Failure writing data.\n");
					exit(-1);
				}
			}
			free(data_16);
			break;
		default:
			fprintf(stderr, "Not implemented for this data type.\n");
			exit(-1);
	}
	if (post_padding)
	{
		memset(header, 0, post_padding);
		fwrite(header, 1, post_padding, fp);
    }
	fclose( fp );    fp = NULL;
}
//----------------------------------------------------------------------
/*
R format:
  slice1 = matrix( c(1,2,3,4,5,6), nrow = 3, byrow = T )

  yields
    1 2
    3 4
    5 6
*/
static void saveR ( ChunkData* gray, char* outname, int first, int last )
{
    FILE*  fp = fopen( outname, "wb" );
    assert( fp != NULL );
    if (fp == NULL)  usage( (char *)"please specify a valid output file name" );
    for (int z=first; z<=last; z++) {
        printf( "slice=%d \n", (z+1) );    fflush(stdout);
        fprintf( fp, "slice%d = matrix( c(", (z+1) );
        for (int y=0; y<gray->m_ySize; y++) {
            for (int x=0; x<gray->m_xSize; x++) {
                int  value = gray->getData( x, y, z );
                fprintf( fp, "%d", value );
                if (!(x==gray->m_xSize-1 && y==gray->m_ySize-1))
                    fprintf( fp, "," );
            }
        }
        fprintf( fp, "), nrow = 3, byrow = T )  \n\n" );
    }

    fclose( fp );    fp = NULL;
}
//----------------------------------------------------------------------
/*
from http://www.vtk.org/VTK/img/file-formats.pdf

# vtk DataFile Version 2.0
Volume example
ASCII or BINARY
DATASET STRUCTURED_POINTS
DIMENSIONS 3 4 6
SPACING 0.5 0.5 1.0
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
*/
static void saveVtk ( ChunkData* gray, char* outname, int first, int last, bool binary=true )
{
    FILE*  fp = fopen( outname, "wb" );
    assert( fp != NULL );
    if (fp == NULL)  usage( (char *)"please specify a valid output file name" );

    //header
    fprintf( fp, "# vtk DataFile Version 2.0\n" );
    fprintf( fp, "Volume example\n" );
    if (binary)    fprintf( fp, "BINARY\n" );
    else           fprintf( fp, "ASCII\n" );
    fprintf( fp, "DATASET STRUCTURED_POINTS\n" );
    fprintf( fp, "DIMENSIONS %d %d %d\n", gray->m_xSize, gray->m_ySize, gray->m_zSize );
    //fprintf( fp, "ASPECT_RATIO 1 1 1\n" );
    fprintf( fp, "SPACING %f %f %f\n", gray->m_xSpacing, gray->m_ySpacing, gray->m_zSpacing );
    fprintf( fp, "ORIGIN 0 0 0\n" );
    fprintf( fp, "POINT_DATA %d\n", gray->m_xSize * gray->m_ySize * (last-first+1) );
    /** \todo might want to support types other than int (4 bytes) for binary. */
    fprintf( fp, "SCALARS volume_scalars int 1\n" );
    fprintf( fp, "LOOKUP_TABLE default\n" );

	//write data
	for (int z=first; z<=last; z++) {
		printf( "slice=%d \n", (z+1) );    fflush(stdout);
		for (int y=0; y<gray->m_ySize; y++) {
			for (int x=0; x<gray->m_xSize; x++) {
				int  value = gray->getData( x, y, z );
				if (binary) {
				    /** \todo might want to support types other than (signed) int (4 bytes) for binary
					 *  depending upon min and max.
					 */
					if (fwrite( &value, sizeof( value ), 1, fp ) != 1)
					{
						fprintf(stderr, "Write failed.\n");
						exit(-1);
					}
				} else {
					fprintf( fp, " %d", value );
				}
			}
			if (!binary)    fprintf( fp, " \n" );
		}
	}

    fclose( fp );    fp = NULL;
}
//----------------------------------------------------------------------
/*
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
//----------------------------------------------------------------------
int main ( int argc, char* argv[] ) {
    if (verbose) {
        fprintf( stderr, "VIEWNIX_ENV=%s \n", getenv("VIEWNIX_ENV") );
    }
    if (argc < 2)    usage( 0 );

    printf( "file=%s \n", argv[1] );
    ChunkData* gray = new ChunkData( argv[1], ChunkData::defaultSlicesPerChunk,
		ChunkData::defaultOverlapSliceCount, true, 1 );
    assert( gray != NULL );
	if (!gray->mIsCavassFile || !gray->m_vh_initialized)
	{
		fprintf(stderr, "Failed to load file.");
		exit(1);
	}

    assert( argc >= 3 );
    if (argc < 3)    usage( (char *)"please specify an output file type" );

    assert( argc >= 4 );
    if (argc < 4)    usage( (char *)"please specify an output file name" );

    char*  pEnd;

    assert( argc >= 5 );
    if (argc < 5)    usage( (char *)"please specify the first slice number (0 .. n-1)" );
    int  first = strtol( argv[4], &pEnd, 10 );
    assert( *pEnd == 0 );
    if (*pEnd != 0)    usage( (char *)"please specify a valid first slice number (0 .. n-1)" );

    assert( argc >= 6 );
    if (argc < 6)    usage( (char *)"please specify the last slice number (0.. n-1)" );
    int  last = strtol( argv[5], &pEnd, 10 );
    assert( *pEnd == 0 );
    if (*pEnd != 0)    usage( (char *)"please specify a valid last slice number (0 .. n-1)" );

    if (::verbose) {
        fprintf( stdout, "input=%s, type=%s, output=%s, first=%s, last=%s \n",
            argv[1], argv[2], argv[3], argv[4], argv[5] );
    }

    if (strcmp("mathematica",argv[2])==0) {
        saveMathematica( gray, argv[3], first, last );
    } else if (strcmp("matlab",argv[2])==0) {
        saveMatlab( gray, argv[3], first, last );
	} else if (strcmp("octave",argv[2])==0) {
        saveOctave( gray, argv[3], first, last );
    } else if (strcmp("r",argv[2])==0 || strcmp("R",argv[2])==0) {
        saveR( gray, argv[3], first, last );
    } else if (strcmp("vtk",argv[2])==0 || strcmp("VTK",argv[2])==0) {
        saveVtk( gray, argv[3], first, last );
    } else {
        usage( (char *)"unsupported output file type" );
    }

    return 0;
}
//----------------------------------------------------------------------
