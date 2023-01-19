/*
  Copyright 1993-2011 Medical Image Processing Group
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

#include <stdlib.h> 
#include <string.h> 
 
typedef struct VoxelWithValue {
  short x, y, z;
  unsigned short val;
} VoxelWithValue;


typedef struct {
  char *q;
  long *hh;
  long allocsize;
  long size;
  long topindex;
  int itemsize;
  int elem_size;
  unsigned short (*val)(void *); /* returns value of item; highest value item is popped first */
  int (*idcmp)(void *, void *); /* returns relative ordering of two items by identity (not value).  Must return non-zero if only one pointer is NULL. */
} Chash;

Chash* chash_create(long hashsize, int itemsize, unsigned short (*val)(void *), int (*idcmp)(void *, void *));
void chash_destroy(Chash *H);
int chash_isempty(Chash *H);
int chash_push(Chash *H, VoxelWithValue *v);
void chash_repush(Chash *H, VoxelWithValue *v, unsigned short wo);
void chash_pop(Chash *H, VoxelWithValue *v);

Chash* chash_create2(long hashsize, int itemsize, unsigned short (*val)(void *), int (*idcmp)(void *, void *));
int chash_push2(Chash *H, VoxelWithValue *v);
void chash_repush2(Chash *H, VoxelWithValue *v, unsigned short wo);
void chash_pop2(Chash *H, VoxelWithValue *v);

