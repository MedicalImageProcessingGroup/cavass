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

#ifndef ELAPSEDTIME_H
#define ELAPSEDTIME_H

//----------------------------------------------------------------------
/*
file:   ElapsedTime.h
author: george j. grevera, ph.d.
date:   18-may-2005
desc.:  this header file contains a timer class.  to use it, simply
        declare a variable or call the resetTime() method whenever
        you wish to reset the timer back to zero.  to get the elapsed
        time, call getElapsedTime().  time is a double with resolution of
        10e-6 on linux and is reported in seconds.

        example:
        ElapsedTime  et;
          <your code here>
        double d = et.getElapsedTime();

        or

        et.resetTime();
          <your code here>
        d = et.getElapsedTime();
*/
//----------------------------------------------------------------------
#if defined (WIN32) || defined (_WIN32)
#else
    #include  <sys/time.h>
#endif

class ElapsedTime {
    double  mStart;
public:
    ElapsedTime ( ) {
        resetTime();
    }

    void resetTime ( ) {
        #if defined (WIN32) || defined (_WIN32)
            mStart = GetTickCount();
        #else
            struct timeval   tv_start;
            struct timezone  tz_start;
            gettimeofday( &tv_start, &tz_start );
            mStart = tv_start.tv_sec + (tv_start.tv_usec / 1E6);
        #endif
    }

    //return time in seconds but accurate to at least milliseconds
    double getElapsedTime ( ) {
        #if defined (WIN32) || defined (_WIN32)
            long  tc_end = GetTickCount();
            return  (tc_end-mStart)/1E3;
        #else
            struct timeval  tv_end;
            struct timezone tz_end;
            gettimeofday(&tv_end, &tz_end);
            double d_end   = tv_end.tv_sec + (tv_end.tv_usec / 1E6);
            return (d_end-mStart);
        #endif
    }
};
#endif


