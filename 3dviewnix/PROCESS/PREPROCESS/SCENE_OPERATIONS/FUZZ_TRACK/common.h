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

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <float.h>



/* Error messages */

#define MSG1  "Cannot allocate memory space"
#define MSG2  "Cannot open file"
#define MSG3  "Invalid option"

/* Common data types to all programs */

#ifndef __cplusplus
typedef enum boolean {false,true} bool;
#endif
typedef unsigned short ushort;
typedef unsigned int uint;

//#ifndef __cplusplus
 //#ifndef _WIN32  
   //#ifndef __cplusplus
      //typedef enum boolean {false,true} bool;
   //#endif
   //#else
   //typedef unsigned short ushort;
   //typedef unsigned int uint;
//#endif
//#endif


#ifdef _MSC_VER
#define __inline__  /* */
#else
#define __inline__  inline
#endif


typedef struct timeval timer;
typedef unsigned char uchar;

typedef int* ap_int;
typedef double* ap_double;

  // Real type values.
  // Can be changed to float, double.
typedef float real;
#define REAL_MAX FLT_MAX //DBL_MAX
#define REAL_MIN FLT_MIN //DBL_MIN

/* Common definitions */


#ifndef PI
#define PI          3.14159265358979323846
#endif
#define INTERIOR    0
#define EXTERIOR    1
#define BOTH        2
#define WHITE       0
#define GRAY        1
#define BLACK       2
#define NIL        -1
#define INCREASING  1
#define DECREASING  0
#define Epsilon     1E-05

/* Common operations */

#ifndef MAX
#define MAX(x,y) (((x) > (y))?(x):(y))
#endif

#ifndef MIN
#define MIN(x,y) (((x) < (y))?(x):(y))
#endif

#define ROUND(x) ((x < 0)?(int)(x-0.5):(int)(x+0.5))

#define SIGN(x) ((x >= 0)?1:-1)

char   *AllocCharArray(int n);  /* It allocates 1D array of n characters */
uchar  *AllocUCharArray(int n);  /* It allocates 1D array of n characters */
ushort *AllocUShortArray(int n);  /* It allocates 1D array of n ushorts */
uint   *AllocUIntArray(int n); /* It allocates 1D array of n uints */
int    *AllocIntArray(int n);   /* It allocates 1D array of n integers */
float  *AllocFloatArray(int n); /* It allocates 1D array of n floats */
double *AllocDoubleArray(int n);/* It allocates 1D array of n doubles */
real   *AllocRealArray(int n); /* It allocates 1D array of n reals */

void Error(const char *msg, const char *func); /* It prints error message and
                                     exits the program. */
void Warning(char *msg,char *func); /* It prints warning message and
                                       leaves the routine. */
void Change(int *a, int *b); /* It changes content between a and b */

void FChange(float *a, float *b); /* It changes content between floats a and b */

int NCFgets(char *s, int m, FILE *f); /* It skips # comments */

/**
 * Gera um número inteiro aleatório no intervalo [low,high].
http://www.ime.usp.br/~pf/algoritmos/aulas/random.html
 **/
int RandomInteger (int low, int high);

int SafeMod(int a, int n);

/// Algorithm from http://aggregate.org/MAGIC/#Is%20Power%20of%202
int IsPowerOf2(int x);
#endif
