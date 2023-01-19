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


 
 
 
#define CHUNKS 16
#define CHUNKSIZE 8


#include <stdlib.h>
#include <assert.h>

typedef int Chunk[CHUNKSIZE];

static Chunk storage[CHUNKS];
static int nused, thisn;
static char flags[CHUNKS];

/*****************************************************************************
 * FUNCTION: salloc
 * DESCRIPTION: Allocates memory just as malloc, but this memory must be freed
 *    by sfree.
 * PARAMETERS:
 *    size: The size in bytes of the memory block to be allocated.  Must be
 *       non-zero.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: The address of the memory block allocated, or NULL.
 * EXIT CONDITIONS: Returns NULL if it fails.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
char *salloc(unsigned long size)
{
	if (size>sizeof(Chunk) || nused==CHUNKS)
		return (char *)malloc(size);
	while (flags[thisn])
		thisn = (thisn+1)%CHUNKS;
	flags[thisn] = 1;
	nused++;
	return ((char *)storage[thisn]);
}

/*****************************************************************************
 * FUNCTION: sfree
 * DESCRIPTION: Frees allocated memory.
 * PARAMETERS:
 *    ptr: The address of the memory block to be freed.  Must be allocated by
 *       salloc, calloc, or malloc and not subsequently freed.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: None
 * RETURN VALUE: None
 * EXIT CONDITIONS: Undefined if ptr is not address of a memory block allocated
 *    by salloc, calloc, or malloc and not subsequently freed.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void sfree(void *ptr)
{
	if ((unsigned long)ptr<(unsigned long)storage ||
			(unsigned long)ptr>(unsigned long)(storage+CHUNKS-1))
	{	free(ptr);
		return;
	}
	thisn = (Chunk *)ptr-storage;
	assert(flags[thisn]);
	flags[thisn] = 0;
	nused--;
}
