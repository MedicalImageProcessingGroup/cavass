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


 
  
#ifdef _c_plus_plus
extern "C"{
#endif
 

typedef struct {
  char *q;
  long *h;
  long *hh;
  long allocsize;
  long size;
  long hashsize;
  int itemsize;
  int elem_size;
  long (*hf)(long size, char *v); /* hash function */
  int (*valcmp)(char* v, char *vv); /* returns relative ordering of two items by value of item; highest value item is popped first */
  int (*idcmp)(char* v, char *vv); /* returns relative ordering of two items by identity (not value).  Must return positive if only the second pointer is NULL. */
} Hheap;

Hheap* hheap_create(long hashsize, int itemsize, long (*hf)(), int (*valcmp)(), int (*idcmp)());
int hheap_destroy(Hheap *H);
int hheap_isempty(Hheap *H);
int hheap_push(Hheap *H, char *v);
int hheap_repush(Hheap *H, char *v);
int hheap_pop(Hheap *H, char *v);

#ifdef _c_plus_plus
}
#endif
