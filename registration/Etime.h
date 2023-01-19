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

//----------------------------------------------------------------------
/*
file:   ETime.h
author: George J. Grevera (modifided by Andre Souza 09/22/2005)
date:   05/05/2005
desc.:  to use it, simply call the resetTime() method whenever
        you wish to reset the timer back to zero.  to get the elapsed
        time, call getElapsedTime().  Time is a double with resolution of
        10e-6 on linux and is reported in seconds.

        resetTime();
          <your code here>
        double d = getElapsedTime();
*/
//----------------------------------
static double  mStart;

#if defined (WIN32) || defined (_WIN32)

#include  <Windows.h>

void resetTime ( ) {
    mStart = GetTickCount();
}

double getElapsedTime ( ) {
    long  tc_end = GetTickCount();
    return  (tc_end-mStart) / 1E3;
}

#else

#include  <sys/time.h>

void resetTime ( ) {
  struct timeval   tv_start;
  struct timezone  tz_start;
  gettimeofday( &tv_start, &tz_start );
  mStart = tv_start.tv_sec + (tv_start.tv_usec / 1E6);
}

double getElapsedTime ( ) {
  struct timeval  tv_end;
  struct timezone tz_end;
  gettimeofday(&tv_end, &tz_end);
  double d_end   = tv_end.tv_sec + (tv_end.tv_usec / 1E6);
  return (d_end-mStart);
}

#endif
