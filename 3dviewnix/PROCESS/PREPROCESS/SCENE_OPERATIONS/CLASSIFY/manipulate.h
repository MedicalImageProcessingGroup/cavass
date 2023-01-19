/*
  Copyright 1993-2008 Medical Image Processing Group
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

 
 



#define WINDOW_BORDER_WIDTH 1
#define PAINT_INDEX_OFFSET 128
#define OBJECT_IMAGE_BACKGROUND (PAINT_INDEX_OFFSET-2)
#define MARK_SHADE (PAINT_INDEX_OFFSET-1)
#define GRAY_INDEX_OFFSET PAINT_INDEX_OFFSET
#define PLANE_INDEX_OFFSET (PAINT_INDEX_OFFSET*2)
#define Z_BUFFER_LEVELS 0x100000
#define MAX_ANGLE_SHADE 3840
#define Z_SUBLEVELS ((unsigned)256)
#define SHADE_LUT_SIZE 257

#include "manip_etc.h"
