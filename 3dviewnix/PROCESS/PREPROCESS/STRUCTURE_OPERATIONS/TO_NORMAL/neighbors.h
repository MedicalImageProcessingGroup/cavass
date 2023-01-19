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

 
 
 
#define PX ((unsigned short)0x8000)
#define PY ((unsigned short)0x4000)
#define PZ ((unsigned short)0x2000)
#define NX ((unsigned short)0x1000)
#define NY ((unsigned short)0x0800)
#define NZ ((unsigned short)0x0400)
#define ALL_NEIGHBORS (PZ|NZ|NY|PY|PX|NX)
#define XMASK ((unsigned short)~ALL_NEIGHBORS)

/* Note: gradient in this file refers to a vector opposite in direction
 *	to the unit normal. Its magnitude is irrelevant.
 */

#define G_COMPONENT_BITS 4
#define G_NORM (1<<(G_COMPONENT_BITS-1))
#define G_CODES ((6<<2*G_COMPONENT_BITS)+1)

/* The following macro returns the first component of the gradient
 *	represented by gcode.  It is either gx or gy.
 */
#define G_1(gcode) \
	(G_NORM-(((gcode)>>G_COMPONENT_BITS&((1<<G_COMPONENT_BITS)-1))+.5))

/* The following macro returns the second component of the gradient
 *	represented by gcode.  It is either gy or gz.
 */
#define G_2(gcode) (G_NORM-(((gcode)&((1<<G_COMPONENT_BITS)-1))+.5))

/* The following macro assigns the values of the components of the gradient
 *	represented by gcode to gx, gy, gz.
 */
#define G_decode(gx, gy, gz, gcode)  \
switch ((gcode)>>2*G_COMPONENT_BITS) \
{   case 0:	                         \
        gx = (float)-G_NORM;         \
        gy = (float)G_1(gcode);      \
        gz = (float)G_2(gcode);      \
        break;                       \
    case 1:                          \
        gx = (float)G_1(gcode);      \
        gy = (float)-G_NORM;         \
        gz = (float)G_2(gcode);      \
        break;                       \
    case 2:                          \
        gx = (float)G_1(gcode);      \
        gy = (float)G_2(gcode);      \
        gz = (float)-G_NORM;         \
        break;                       \
    case 3:                          \
        gx = (float)G_NORM;          \
        gy = (float)G_1(gcode);      \
        gz = (float)G_2(gcode);      \
        break;                       \
    case 4:                          \
        gx = (float)G_1(gcode);      \
        gy = (float)G_NORM;          \
        gz = (float)G_2(gcode);      \
        break;                       \
    case 5:                          \
        gx = (float)G_1(gcode);      \
        gy = (float)G_2(gcode);      \
        gz = (float)G_NORM;          \
        break;                       \
    case 6:                          \
        gx = gy = gz = 0;            \
}

extern unsigned short G_code(double gx, double gy, double gz);

#define BG_COMPONENT_BITS 6
#define BG_NORM (1<<(BG_COMPONENT_BITS-1))
#define BG_CODES ((6<<2*BG_COMPONENT_BITS)+1)

/* The following macro returns the first component of the gradient
 *	represented by gcode.  It is either gx or gy.
 */
#define BG_1(gcode) \
	(BG_NORM-(((gcode)>>BG_COMPONENT_BITS&((1<<BG_COMPONENT_BITS)-1))+.5))

/* The following macro returns the second component of the gradient
 *	represented by gcode.  It is either gy or gz.
 */
#define BG_2(gcode) (BG_NORM-(((gcode)&((1<<BG_COMPONENT_BITS)-1))+.5))

/* The following macro assigns the values of the components of the gradient
 *	represented by gcode to gx, gy, gz.
 */
#define BG_decode(gx, gy, gz, gcode)  \
switch ((gcode)>>2*BG_COMPONENT_BITS) \
{   case 0:                           \
        gx = (float)-BG_NORM;         \
        gy = (float)BG_1(gcode);      \
        gz = (float)BG_2(gcode);      \
        break;                        \
    case 1:                           \
        gx = (float)BG_1(gcode);      \
        gy = (float)-BG_NORM;         \
        gz = (float)BG_2(gcode);      \
        break;                        \
    case 2:                           \
        gx = (float)BG_1(gcode);      \
        gy = (float)BG_2(gcode);      \
        gz = (float)-BG_NORM;         \
        break;                        \
    case 3:                           \
        gx = (float)BG_NORM;          \
        gy = (float)BG_1(gcode);      \
        gz = (float)BG_2(gcode);      \
        break;                        \
    case 4:                           \
        gx = (float)BG_1(gcode);      \
        gy = (float)BG_NORM;          \
        gz = (float)BG_2(gcode);      \
        break;                        \
    case 5:                           \
        gx = (float)BG_1(gcode);      \
        gy = (float)BG_2(gcode);      \
        gz = (float)BG_NORM;          \
        break;                        \
    case 6:                           \
        gx = gy = gz = 0;             \
}

extern unsigned short BG_code(double gx, double gy, double gz);
