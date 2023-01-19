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

 
 
 
#include <stdio.h>
#include "chash.h"

#define CHUNK_SIZE 1024
#define HEHS sizeof(long) /* heap element header size */

#define HQElem(s) (*((ChashElem*)(H->q + (s)*H->elem_size)))

typedef struct {
  long l; /* link to next */
  char c[1]; /* content */
} ChashElem;

typedef struct {
  long l; /* link to next */
  VoxelWithValue c; /* content */
} VoxelChashElem;

/*****************************************************************************
 * FUNCTION: chash_create
 * DESCRIPTION: Allocates and initializes a hashed heap structure with one
 *    bin per value.
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
 *    Created: 1/1999 by Laszlo Nyul
 *    Modified: 1/29/99 returns NULL on failure by Dewey Odhner
 *    Modified: 1/29/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
Chash*
chash_create(long hashsize, int itemsize, unsigned short (*val)(void *), int (*idcmp)(void *, void *))
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
    HQElem(s).l = s + 1;
  HQElem(s).l = 0L;
  H->allocsize = CHUNK_SIZE;
  H->size = 0L;
  return H;
}

/*****************************************************************************
 * FUNCTION: chash_destroy
 * DESCRIPTION: Frees a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to allocated memory, and H->hh, H->q must be
 *       allocated or NULL.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
void chash_destroy(Chash *H)
{
  if (H->hh)
    free(H->hh);
  if (H->q)
    free(H->q);
  free(H);
}

/*****************************************************************************
 * FUNCTION: chash_isempty
 * DESCRIPTION: Checks whether the heap is empty.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Non-zero if the heap is empty.
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
int chash_isempty(Chash *H)
{
  return H->size == 0L;
}

/*****************************************************************************
 * FUNCTION: chash_enlarge
 * DESCRIPTION: Enlarges the capacity of a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *    Modified: 1/29/99 returns 1 on failure by Dewey Odhner
 *    Modified: 1/29/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int chash_enlarge(Chash *H)
{
  long s;
  char *q;

  q = (char*)realloc(H->q, (H->allocsize + CHUNK_SIZE) * H->elem_size);
  if (q == NULL)
    return 1;
  H->q = q;
  for (s = H->allocsize; s < H->allocsize + CHUNK_SIZE - 1; s++)
    HQElem(s).l = s + 1;
  HQElem(s).l = HQElem(0).l;
  HQElem(0).l = H->allocsize;
  H->allocsize += CHUNK_SIZE;
  return 0;
}

/*****************************************************************************
 * FUNCTION: chash_push
 * DESCRIPTION: Stores an item in a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 *    v: Pointer to the item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *    Modified: 2/1/99 returns 1 on failure by Dewey Odhner
 *    Modified: 2/1/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int chash_push(Chash *H, VoxelWithValue *v)
{
  unsigned short w;
  long t;
 
  w = H->val(v);
  if (H->size == H->allocsize - 1 && chash_enlarge(H))
    return 1;
  H->size++;
  t = HQElem(0).l;
  HQElem(0).l = HQElem(t).l;
  memcpy(HQElem(t).c, v, H->itemsize);
  HQElem(t).l = H->hh[w];
  H->hh[w] = t;
  if (w > H->topindex)
    H->topindex = w;
  return 0;
}

/*****************************************************************************
 * FUNCTION: chash_repush
 * DESCRIPTION: Reorders an item in a hashed heap structure according to a
 *    new value.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure where the item is stored.
 *    v: Pointer to the item with the new value.
 *    wo: The former value of this item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *    Modified: 2/3/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
void chash_repush(Chash *H, VoxelWithValue *v, unsigned short wo)
{
  unsigned short w;
  VoxelWithValue *vv;
  long t, tt;
  VoxelWithValue vvv;
 
  w = H->val(v);
  vv = NULL;
  if (H->idcmp)
    for (t = H->hh[wo], tt = 0L; t > 0L; tt = t, t = HQElem(t).l)
    {
      vv = (VoxelWithValue *)HQElem(t).c;
      if (H->idcmp(v, vv) == 0)
        break;
    }
  else
  {
    vvv.x = vvv.y = vvv.z = -1;
	for (t= H->hh[wo], tt= 0L; t > 0L; tt= t, t= ((VoxelChashElem*)H->q)[t].l)
    {
      vvv = ((VoxelChashElem*)H->q)[t].c;
      if (v->x==vvv.x && v->y==vvv.y &&
	      v->z==vvv.z)
        break;
    }
  }
  if (tt > 0L)
    HQElem(tt).l = HQElem(t).l;
  else
    H->hh[wo] = HQElem(t).l;
  memcpy(HQElem(t).c, v, H->itemsize);
  HQElem(t).l = H->hh[w];
  H->hh[w] = t;
  if (w > H->topindex)
    H->topindex = w;
}

/*****************************************************************************
 * FUNCTION: chash_pop
 * DESCRIPTION: Retrieves an item from a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure where at least one item is
 *       stored.
 *    v: Stores the item at this address.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1/1999 by Laszlo Nyul
 *    Modified: 2/1/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
void chash_pop(Chash *H, VoxelWithValue *v)
{
  long hr, t, tt;
 
  hr = H->topindex;
  t = H->hh[hr];
  tt = HQElem(t).l;
  memcpy(v, HQElem(t).c, H->itemsize);
  H->hh[hr] = tt;
  if (tt == 0L)
  {
    for (hr--; hr > 0L && H->hh[hr] == 0L; hr--) ;
    H->topindex = hr;
  }
  HQElem(t).l = HQElem(0).l;
  HQElem(0).l = t;
  H->size--;
}

