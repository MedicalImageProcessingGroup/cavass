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

 
 



typedef enum Classification_type {BINARY, GRADIENT, PERCENT}
	Classification_type;

typedef unsigned short Pixel_unit; /* Must be big enough to hold the value
	GRAY_INDEX_OFFSET-1 */

#include "glob_vars.h"
#include "functions.h"
#include <sys/types.h>
#if ! defined (WIN32) && ! defined (_WIN32)
    #include <unistd.h>
#else
	int getpid(), kill();
#endif

