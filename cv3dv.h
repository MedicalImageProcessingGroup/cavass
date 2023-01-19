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

#ifndef __3dv_h
#define __3dv_h

#include <stdio.h>
#include <sys/types.h>

#if defined (WIN32) || defined (_WIN32)
#define unlink(fn) _unlink(fn)
#endif

typedef struct { int x, y; } X_Point;

#ifdef __cplusplus
extern "C" {
#else
#include <Viewnix.h>
#endif
  int VCloseData     ( FILE* fp );
  int VDecodeError   ( char* process, char* function, int error, char msg[200] );
  int VDeleteBackgroundProcessInformation ( void );
  int VAddBackgroundProcessInformation ( char* command );
  int VPackByteToBit ( unsigned char* idata, int nbytes, unsigned char* odata );
  int VReadData      ( char* data, int size, int items, FILE* fp,
                       int* items_read );
  int VReadHeader    ( FILE* fp, ViewnixHeader* vh, char group[5],
                       char element[5] );
  int VSeekData      ( FILE* fp, off_t offset );
  int VWriteData     ( char* data, int size, int items, FILE* fp,
                       int* items_written );
  int VWriteHeader   ( FILE *fp, ViewnixHeader *vh, char group[5],
                       char element[5] );
  int VPrintFatalError ( const char function[], const char msg[], int value );
  int VLSeekData     ( FILE* fp, double offset );
  int VLSeek         ( FILE* fp, double offset );
  int VGetHeaderLength ( FILE* fp, int* hdrlen );
  int VComputeLine   ( int x1, int y1, int x2, int y2, X_Point** points, int* npoints );
#ifdef __cplusplus
};
#endif

#endif
