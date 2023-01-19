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


#ifdef _C_PLUS_PLUS
extern "C"{
#endif
 
 
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
  unsigned short (*val)(VoxelWithValue* v); /* returns value of item; highest value item is popped first */
  int (*idcmp)(VoxelWithValue* v, VoxelWithValue* vv); /* returns relative ordering of two items by identity (not value).  Must return non-zero if only one pointer is NULL. */
} Chash;

Chash* chash_create(long hashsize, int itemsize, unsigned short (*val)(), int (*idcmp)());
int chash_destroy(Chash *H);
int chash_isempty(Chash *H);
int chash_push(Chash *H, char *v);
int chash_repush(Chash *H, char *v, unsigned short wo);
int chash_pop(Chash *H, char *v);
int chash_enlarge(Chash *H);

Chash* chash_create2(long hashsize, int itemsize, unsigned short (*val)(VoxelWithValue *v), int (*idcmp)());
int chash_destroy2();
int chash_isempty2();
int chash_push2(Chash *H, char *v);
void chash_repush2(Chash *H, char *v, unsigned short wo);
void chash_pop2(Chash *H, char *v);
int chash_enlarge2(Chash *H);

#ifdef _C_PLUS_PLUS
}
#endif
