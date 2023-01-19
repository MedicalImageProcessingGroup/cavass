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


 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chash.h"

#define CHUNK_SIZE 1024
#define HEHS (2*sizeof(long)) /* heap element header size */

#define HQElem(s) (*((ChashElem*)(H->q + (s)*H->elem_size)))

typedef struct {
  long l; /* link to next */
  long b; /* link to previous */
  char c[1]; /* content */
} ChashElem;

typedef struct {
  long l; /* link to next */
  long b; /* link to previous */
  VoxelWithValue c; /* content */
} VoxelChashElem;

/*****************************************************************************
 * FUNCTION: chash_create2
 * DESCRIPTION: Allocates and initializes a hashed heap structure with one
 *    bin per value and doubly linked lists.
 * PARAMETERS:
 *    hashsize: Number of hash bins
 *    itemsize: Size in bytes of items to be stored in the heap
 *    val: Hash function; called as val(v) where v is a pointer to an
 *       item; must return a value between 1 and hashsize - 1.
 *    idcmp: returns relative ordering of two items by identity (not value).
 *       Must return positive if only the second pointer is NULL.
 *       idcmp may be NULL if item type is VoxelWithValue.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Pointer to the heap structure.
 * EXIT CONDITIONS: Returns NULL on failure.
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
Chash*
chash_create2(hashsize, itemsize, val, idcmp)
  long hashsize;
  int itemsize;
  unsigned short (*val)(); /* range 1 to hashsize-1 */
  int (*idcmp)(); /* if NULL, item must be VoxelWithValue */
{
  Chash *H;
  long s;

  H = (Chash*)malloc(sizeof(Chash));
  if (H == NULL)
    return NULL;
  H->hh = (long*)malloc(hashsize * sizeof(long));
  if (H->hh == NULL)
  {
    free(H);
	return NULL;
  }
  for (s = 0L; s < hashsize; s++)
    H->hh[s] = 0L;
  H->itemsize = itemsize;
  H->elem_size = HEHS+itemsize;
  while (H->elem_size % (sizeof(ChashElem)-HEHS))
    H->elem_size++;
  H->val = val;
  H->idcmp = idcmp;
  H->topindex = 0L;
  H->q = (char*)malloc(CHUNK_SIZE * H->elem_size);
  if (H->q == NULL)
  {
	free(H->hh);
    free(H);
	return NULL;
  }
  for (s = 0L; s < CHUNK_SIZE - 1; s++)
  {
    HQElem(s).l = s + 1;
    HQElem(s + 1).b = s;
  }
  HQElem(s).l = 0L;
  HQElem(0L).b = s;
  H->allocsize = CHUNK_SIZE;
  H->size = 0L;
  return H;
}

/*****************************************************************************
 * FUNCTION: chash_enlarge2
 * DESCRIPTION: Enlarges the capacity of a hashed heap structure with one
 *    bin per value and doubly linked lists.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
chash_enlarge2(H)
  Chash *H;
{
  long s;
  char *q;

  q = (char*)realloc(H->q, (H->allocsize + CHUNK_SIZE) * H->elem_size);
  if (q == NULL)
    return 1;
  H->q = q;
  for (s = H->allocsize; s < H->allocsize + CHUNK_SIZE - 1; s++)
  {
    HQElem(s).l = s + 1;
	HQElem(s + 1).b = s;
  }
  HQElem(H->allocsize).b = HQElem(0L).b;
  HQElem(HQElem(0L).b).l = H->allocsize;
  HQElem(s).l = 0L;
  HQElem(0L).b = s;
  H->allocsize += CHUNK_SIZE;
  return 0;
}

/*****************************************************************************
 * FUNCTION: chash_push2
 * DESCRIPTION: Stores an item in a hashed heap structure with one
 *    bin per value and doubly linked lists.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 *    v: Pointer to the item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
chash_push2(H, v)
  Chash *H;
  char *v;
{
  unsigned short w;
  long t;
 
  w = H->val(v);
  if (H->size == H->allocsize - 1 && chash_enlarge2(H))
    return 1;
  H->size++;
  t = HQElem(0).l;
  HQElem(0).l = HQElem(t).l;
  HQElem(HQElem(t).l).b = 0L;
  memcpy(HQElem(t).c, v, H->itemsize);
  if (H->hh[w] == 0L)
    H->hh[w] = HQElem(t).l = HQElem(t).b = t;
  else
  {
    HQElem(t).l = H->hh[w];
	HQElem(t).b = HQElem(H->hh[w]).b;
	HQElem(H->hh[w]).b = t;
	HQElem(HQElem(t).b).l = t;
    H->hh[w] = t;
  }
  if (w > H->topindex)
    H->topindex = w;
  return 0;
}

/*****************************************************************************
 * FUNCTION: chash_repush2
 * DESCRIPTION: Reorders an item in a hashed heap structure according to a
 *    new value.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure with one
 *       bin per value and doubly linked lists where the item is stored.
 *    v: Pointer to the item with the new value.
 *    wo: The former value of this item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
chash_repush2(H, v, wo)
  Chash *H;
  char *v;
  unsigned short wo;
{
  unsigned short w;
  long t;
 
  w = H->val(v);
  t = H->hh[wo];
  if (H->idcmp)
	for (t=H->hh[wo]; H->idcmp(v, HQElem(t).c); t=HQElem(t).l)
	  ;
  else
    for (t=H->hh[wo];
	    ((VoxelWithValue*)v)->x!=((VoxelChashElem*)H->q)[t].c.x||
		((VoxelWithValue*)v)->y!=((VoxelChashElem*)H->q)[t].c.y||
		((VoxelWithValue*)v)->z!=((VoxelChashElem*)H->q)[t].c.z;
		t=((VoxelChashElem*)H->q)[t].l)
	  ;
  if (HQElem(t).l == t)
    H->hh[wo] = 0L;
  else
  {
    if (H->hh[wo] == t)
	  H->hh[wo] = HQElem(t).l;
	HQElem(HQElem(t).l).b = HQElem(t).b;
	HQElem(HQElem(t).b).l = HQElem(t).l;
  }
  memcpy(HQElem(t).c, v, H->itemsize);
  if (H->hh[w] == 0L)
    H->hh[w] = HQElem(t).l = HQElem(t).b = t;
  else
  {
    HQElem(t).l = H->hh[w];
	HQElem(t).b = HQElem(H->hh[w]).b;
	HQElem(H->hh[w]).b = t;
	HQElem(HQElem(t).b).l = t;
    H->hh[w] = t;
  }
  if (w > H->topindex)
    H->topindex = w;
}

/*****************************************************************************
 * FUNCTION: chash_pop2
 * DESCRIPTION: Retrieves an item from a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure with one bin
 *       per value and doubly linked lists where at least one item is stored.
 *    v: Stores the item at this address.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 2/3/99 by Dewey Odhner
 *
 *****************************************************************************/
chash_pop2(H, v)
  Chash *H;
  char *v;
{
  long t, tb;
 
  t = H->hh[H->topindex];
  tb = HQElem(t).b;
  memcpy(v, HQElem(tb).c, H->itemsize);
  if (tb == t)
  {
    H->hh[H->topindex] = 0L;
    while (H->topindex > 0L && H->hh[H->topindex] == 0L)
	   H->topindex--;
  }
  else
  {
    HQElem(t).b = HQElem(tb).b;
	HQElem(HQElem(tb).b).l = t;
  }
  HQElem(tb).b = HQElem(0L).b;
  HQElem(HQElem(0L).b).l = tb;
  HQElem(0L).b = tb;
  HQElem(tb).l = 0L;
  H->size--;
}

