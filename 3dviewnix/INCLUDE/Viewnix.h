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
 
 
 
 
/************************************************************************
 *                                                                      *
 *      File            : 3DVIEWNIX.H                                   *
 *      Description     : To be kept in the INCLUDE dir. Global file.   *
 *      Return Value    : None.                                         *
 *      Parameters      : None.                                         *
 *      Side effects    : None.                                         *
 *      Entry condition : None.                                         *
 *      Related funcs   : None.                                         *
 *      History         : Written on February 28, 1989 by Hsiu-Mei Hung *
 *                        Modified on January 10, 1993 by Krishna Iyer. *
 *                                                                      *
 ************************************************************************/

/* standard include files */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#if defined (WIN32) || defined (_WIN32)
#include "Xlib.h"
#else
#include <X11/Xlib.h>
#endif

/* 3dviewnix include files */
#include "Vparam.h"
#include "Vtypedef.h"
#include "Vdataheader.h"

#ifdef _LARGEFILE_SOURCE
    #define  fseek  fseeko
    #define  ftell  ftello
#endif

//int VSeekData ( FILE* fp, off_t offset );

