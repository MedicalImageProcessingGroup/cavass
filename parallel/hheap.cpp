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
#include "hheap.h"

#define CHUNK_SIZE 1024
#define HEHS (2*sizeof(long)) /* heap element header size */

#define HQElem(s) (*((HheapElem*)(H->q + (s)*H->elem_size)))

typedef struct {
  long l; /* link to next */
  long i; /* index into array */
  char c[1]; /* content */
} HheapElem;

/*****************************************************************************
 * FUNCTION: hheap_create
 * DESCRIPTION: Allocates and initializes a hashed heap structure.
 * PARAMETERS:
 *    hashsize: Number of hash bins
 *    itemsize: Size in bytes of items to be stored in the heap
 *    hf: Hash function; called as hf(hashsize, v) where v is a pointer to an
 *       item; must return a value between 0 and hashsize - 1.
 *    valcmp: returns relative ordering of two items by value of item;
 *       highest value item is popped first.  valcmp(v1, v2) returns positive
 *       if v1 has a greater value than v2, negative if less, 0 if same.
 *    idcmp: returns relative ordering of two items by identity (not value).
 *       Must return positive if only the second pointer is NULL.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Pointer to the heap structure.
 * EXIT CONDITIONS: Returns NULL on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns NULL on failure by Dewey Odhner
 *    Modified: 1/26/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
Hheap* hheap_create(long hashsize, int itemsize, long (*hf)(), int (*valcmp)(), int (*idcmp)())
//  long hashsize;
//  int itemsize;
//  long (*hf)();
//  int (*valcmp)();
//  int (*idcmp)();
{
  Hheap *H;
  long s;

  H = (Hheap*)malloc(sizeof(Hheap));
  if (H == NULL)
    return NULL;
  H->h = (long*)malloc(CHUNK_SIZE * sizeof(long));
  if (H->h == NULL)
  {
    free(H);
	return NULL;
  }
  H->hh = (long*)malloc(hashsize * sizeof(long));
  if (H->hh == NULL)
  {
	free(H->h);
    free(H);
	return NULL;
  }
  for (s = 0L; s < hashsize; s++)
    H->hh[s] = 0L;
  H->hashsize = hashsize;
  H->itemsize = itemsize;
  H->elem_size = HEHS+itemsize;
  while (H->elem_size % (sizeof(HheapElem)-HEHS))
    H->elem_size++;
  H->hf = (long (*)(long, char *))hf;
  H->valcmp = (int (*)(char*, char *))valcmp;
  H->idcmp = (int (*)(char*, char *))idcmp;
  H->q = (char*)malloc(CHUNK_SIZE * H->elem_size);
  if (H->q == NULL)
  {
	free(H->h);
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
 * FUNCTION: hheap_destroy
 * DESCRIPTION: Frees a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to allocated memory, and H->hh, H->h, H->q must be
 *       allocated or NULL.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: None
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *
 *****************************************************************************/
int hheap_destroy(Hheap *H)  
{
  if (H->hh)
    free(H->hh);
  if (H->h)
    free(H->h);
  if (H->q)
    free(H->q);
  free(H);

  return 1;
}

/*****************************************************************************
 * FUNCTION: hheap_isempty
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
int hheap_isempty(Hheap *H)  
{
  return H->size == 0L;
}

/*****************************************************************************
 * FUNCTION: hheap_enlarge
 * DESCRIPTION: Enlarges the capacity of a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns 1 on failure by Dewey Odhner
 *    Modified: 1/20/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int hheap_enlarge(Hheap *H)  
{
  long s, *h;
  char *q;

  h = (long*)realloc(H->h, (H->allocsize + CHUNK_SIZE) * sizeof(long));
  if (h)
    H->h = h;
  q = (char*)realloc(H->q, (H->allocsize + CHUNK_SIZE) * H->elem_size);
  if (q)
    H->q = q;
  if (h==NULL || q==NULL)
    return 1;
  for (s = H->allocsize; s < H->allocsize + CHUNK_SIZE - 1; s++)
    HQElem(s).l = s + 1;
  HQElem(s).l = HQElem(0).l;
  HQElem(0).l = H->allocsize;
  H->allocsize += CHUNK_SIZE;
  return 0;
}

/*****************************************************************************
 * FUNCTION: hheap_push
 * DESCRIPTION: Stores an item in a hashed heap structure.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure.
 *    v: Pointer to the item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: Zero if successful.
 * EXIT CONDITIONS: Non-zero on failure.
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/19/99 returns 1 on failure by Dewey Odhner
 *    Modified: 1/26/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int hheap_push(Hheap *H, char *v)  
{
  char *vv;
  long hr, t, tt, n, p, s;

  hr = H->hf(H->hashsize, v);
  vv = NULL;
  for (t = H->hh[hr], tt = 0L; t > 0L; tt = t, t = HQElem(t).l)
  {
    vv = HQElem(t).c;
    if (H->idcmp(v, vv) >= 0)
      break;
  }
  if (H->size == H->allocsize - 1 && hheap_enlarge(H))
    return 1;
  n = ++H->size;
  p = n / 2;
  while (n > 1L && H->valcmp(HQElem(H->h[p]).c, v) < 0)
  {
    H->h[n] = H->h[p];
    HQElem(H->h[p]).i = n;
    n = p;
    p = n / 2;
  }
  s = HQElem(0).l;
  HQElem(0).l = HQElem(s).l;
  memcpy(HQElem(s).c, v, H->itemsize);
  H->h[n] = s;
  HQElem(s).i = n;
  HQElem(s).l = t;
  if (tt > 0L)
    HQElem(tt).l = s;
  else
    H->hh[hr] = s;
  return 0;
}

/*****************************************************************************
 * FUNCTION: hheap_repush
 * DESCRIPTION: Reorders an item in a hashed heap structure according to a
 *    new value.
 * PARAMETERS:
 *    H: Must point to a hashed heap structure where the item is stored.
 *    v: Pointer to the item.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS:
 * HISTORY:
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/26/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int hheap_repush(Hheap *H, char *v)
{
  char *vv;
  long hr, t, n, p, s;
 
  hr = H->hf(H->hashsize, v);
  vv = NULL;
  for (t = H->hh[hr]; t > 0L; t = HQElem(t).l)
  {
    vv = HQElem(t).c;
    if (H->idcmp(v, vv) == 0)
      break;
  }
  n = HQElem(t).i;
  s = H->h[n];
  p = n / 2;
  while (n > 1L && H->valcmp(HQElem(H->h[p]).c, v) < 0)
  {
    H->h[n] = H->h[p];
    HQElem(H->h[p]).i = n;
    n = p;
    p = n / 2;
  }
  H->h[n] = s;
  memcpy(HQElem(s).c, v, H->itemsize);
  HQElem(s).i = n;

  return 1;
}

/*****************************************************************************
 * FUNCTION: hheap_pop
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
 *    Created: 1998 by Laszlo Nyul
 *    Modified: 1/27/99 for use with specified item size by Dewey Odhner
 *
 *****************************************************************************/
int hheap_pop(Hheap *H, char *v)  
{
  long hr, t, tt, n, l, r, m, s;
  char *w, *wm;
 
  s = H->h[1];
  memcpy(v, HQElem(s).c, H->itemsize);
  hr = H->hf(H->hashsize, v);
  for (t = H->hh[hr], tt = 0L; t > 0L; tt = t, t = HQElem(t).l)
  {
    if (HQElem(t).i == 1)
      break;
  }
  if (tt > 0L)
    HQElem(tt).l = HQElem(t).l;
  else
    H->hh[hr] = HQElem(t).l;
  HQElem(s).l = HQElem(0).l;
  HQElem(0).l = s;
  s = H->size--;
  if (s > 2L)
  {
    w = HQElem(H->h[s]).c;
    m = 1L;
    n = 0L;
    while (m != n)
    {
      n = m;
      l = n + n;
      r = l + 1;
      if (l <= H->size && H->valcmp(HQElem(H->h[l]).c, w) > 0)
      {
        m = l;
        wm = HQElem(H->h[l]).c;
      }
      else
      {
        m = n;
        wm = w;
      }
      if (r <= H->size && H->valcmp(HQElem(H->h[r]).c, wm) > 0)
        m = r;
      if (m != n)
      {
        H->h[n] = H->h[m];
        HQElem(H->h[m]).i = n;
      }
      else
      {
        H->h[n] = H->h[s];
        HQElem(H->h[s]).i = n;
      }
    }
  }
  else if (s > 1L)
  {
    H->h[1] = H->h[s];
    HQElem(H->h[s]).i = 1;
  }

  return 1;
}
