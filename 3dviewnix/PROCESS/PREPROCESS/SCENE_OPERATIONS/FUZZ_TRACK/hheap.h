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
#include "chash.h"

typedef struct {
  char *q;
  long *h;
  long *hh;
  long allocsize;
  long size;
  long hashsize;
  int itemsize;
  int elem_size;
  long (*hf)(long, void *); /* hash function */
  int (*valcmp)(void *, void *); /* returns relative ordering of two items by value of item; highest value item is popped first */
  int (*idcmp)(void *, void *); /* returns relative ordering of two items by identity (not value).  Must return positive if only the second pointer is NULL. */
} Hheap;

Hheap* hheap_create(long hashsize, int itemsize, long (*hf)(long hashsize, void *v), int (*valcmp)(void *v, void *vv), int (*idcmp)(void *v, void *vv));
void hheap_destroy(Hheap *H);
int hheap_isempty(Hheap *H);
int hheap_push(Hheap *H, void *v);
void hheap_repush(Hheap *H, void *v);
void hheap_pop(Hheap *H, void *v);

/*****************************************************************************
 * FUNCTION: Voxel_Cmp
 * DESCRIPTION: Returns relative ordering of two voxels in a volume
 * PARAMETERS:
 *    v, vv: Pointers to the voxels.  A null pointer is smaller than any voxel.
 * SIDE EFFECTS: Parameters may be evaluated multiple times.
 * ENTRY CONDITIONS: None
 * RETURN VALUE: -1, 0, or 1
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 9/23/10 by Dewey Odhner
 *
 *****************************************************************************/
#define Voxel_Cmp(v, vv)     \
(     \
	(v)==NULL? (vv)==NULL?     \
		0:     \
		-1:     \
	(vv) == NULL?     \
		1:     \
	(((VoxelWithValue *)(v))->x > ((VoxelWithValue *)(vv))->x)?     \
		1:     \
	(((VoxelWithValue *)(v))->x < ((VoxelWithValue *)(vv))->x)?     \
		-1:     \
	(((VoxelWithValue *)(v))->y > ((VoxelWithValue *)(vv))->y)?     \
		1:     \
	(((VoxelWithValue *)(v))->y < ((VoxelWithValue *)(vv))->y)?     \
		-1:     \
	(((VoxelWithValue *)(v))->z > ((VoxelWithValue *)(vv))->z)?     \
		1:     \
	(((VoxelWithValue *)(v))->z < ((VoxelWithValue *)(vv))->z)?     \
		-1:     \
	0     \
)

#define Value_Cmp(v, vv)     \
(     \
	((VoxelWithValue *)(v))->val>((VoxelWithValue *)(vv))->val? 1:     \
	((VoxelWithValue *)(v))->val<((VoxelWithValue *)(vv))->val? -1: 0  \
)
