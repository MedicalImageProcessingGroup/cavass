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
 *      File            : 3DVIEWNIX.PAR                                 *
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
/* common parameters for 3dviewnix software package */
#ifndef FALSE
  #define FALSE 0
#endif
#ifndef TRUE
  #define TRUE !FALSE
#endif
#define BITS_PER_WORD 16
#define BITS_PER_BYTE 8
#define BYTES_PER_BLK ((unsigned long)0xffffffff)
#define rint(x) floor(0.5+(x))
#define rindex strrchr
#define SINGLE_FILE 0
#define MULTIPLE_FILES 1
#define MSG_WIDTH 120
#define NUM_OF_RESERVED_COLORCELLS 12
#define MAX_SUBWINS 20
#define MAX_SCALES 15
#define FILES_PATH "FILES"
#define PROCESS_PATH "PROCESS/BIN"
#define MAX_HORIZONTAL_MENUS 6
#define Default 0
#define MAX_BUTT_CHARS 10
#define MAX_BUTT_MSG_LINES 3 
#define MAX_PANEL_CHARS 12
#define MAX_HELP_CHARS 10
#define COPYRIGHT_MSG "3DVIEWNIX 1.5, (c) 1993-2004, M I P G, UPenn"
/* saving output filename (default)*/
#define MAX_DEFAULT_FILES 20
#define MAX_DEFAULT_CHAR 20

/* data types */
#define MAX_DATA_TYPES 		3
#define IMAGE0 			0
#define IMAGE1			1
#define MOVIE0			200
#define SHELL0			120
#define SHELL1			121
#define SHELL2			122
#define CURVE0			100
#define SURFACE0		110
#define SURFACE1		111

/* file extension types */
#define MAX_FILE_TYPES  14
#define IM0		1
#define CIM		2
#define MV0		4
#define BIM 		8
#define CBI		16
#define SH0 		32
#define BS0		64
#define CV0		128
#define S03		256
#define S04		512
#define SH1 		1024
#define BS1 		2048	
#define S13 		4096
#define BS2 		8192	

/* cursor types */
#define DEWEY_CURSOR 160  
#define DEFAULT_CURSOR XC_top_left_arrow 

/* window types */
#define IMAGE_WINDOW 0
#define BUTTON_WINDOW 1
#define DIALOG_WINDOW 2
#define ALL_WINDOWS 3 
#define LEFT_BUTTON 1 
#define MIDDLE_BUTTON 2 
#define RIGHT_BUTTON 3 
#define NO_BUTTON 4

/* visual class type */
#define INHERIT 1000
#define DEFAULT 1001
#define GRAYSCALE 1002
#define PSEUDOCOLOR 1003
#define DIRECTCOLOR 1004

/*To accomodate SGI peculiarities*/
#ifdef SIGQUIT
#define LIB_EXIT SIGQUIT
#else
#define LIB_EXIT 3
#endif


