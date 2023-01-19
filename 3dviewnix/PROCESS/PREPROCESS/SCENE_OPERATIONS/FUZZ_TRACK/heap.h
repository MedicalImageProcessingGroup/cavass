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

#ifndef _HEAP_H_
#define _HEAP_H_

#include "common.h"

typedef struct _heap {
  int *cost;
  char *color;
  int *pixel;
  int *pos;
  int last;
  int n;
} Heap;


/* Auxiliary Functions */

#define HEAP_DAD(i) ((i - 1) / 2)
#define HEAP_LEFTSON(i) (2 * i + 1)
#define HEAP_RIGHTSON(i) (2 * i + 2)

/* Heap Functions */


//This method is deprecated; you 
//should use "IsFullHeap" instead.
//It is still available only for 
//compatibility purposes.

bool HeapIsFull(Heap *H);


//This method is deprecated; you 
//should use "IsEmptyHeap" instead.
//It is still available only for 
//compatibility purposes.
bool HeapIsEmpty(Heap *H);

bool IsFullHeap(Heap *H);
bool IsEmptyHeap(Heap *H);
Heap *CreateHeap(int n, int *cost);
void DestroyHeap(Heap **H);
bool InsertHeap(Heap *H, int pixel);
bool RemoveHeap(Heap *H, int *pixel);
void GoUpHeap(Heap *H, int i);
void GoDownHeap(Heap *H, int i);

#endif



