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

//======================================================================
/**
 * \file   fft.h
 * \brief  FFT (fast fourier transform) from NRC, least squares fit from NRC,
 *         and a funtion that returns true is a given number is a power of 2.
 * \author George J. Grevera, Ph.D.
 *
 * Copyright: (C) 2003, George Grevera
 *
 * Rise and shine and give God your glory (glory).
 */
//======================================================================
#ifndef  __fft_h
#define  __fft_h

#include  <vector>
using namespace std;

bool isPowerOf2 ( const unsigned long value );

//from NRC:
void fit ( double* x, double* y, const int ndata, double& a, double& b,
           double& siga, double& sigb, double& chi2, double& q );

void fit ( vector< double >* xv, vector< double >* yv, const int ndata,
           double& a, double& b,
           double& siga, double& sigb, double& chi2, double& q );
//from NRC:
void dfour1 ( double data[], unsigned long nn, int isign=1 );

#endif
