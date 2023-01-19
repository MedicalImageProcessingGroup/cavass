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


 
 
#include <stdlib.h> 
#include <stdio.h>

/*****************************************************************************
 * FUNCTION: manipulate_abort
 * DESCRIPTION: Writes an error message to standard error and aborts the
 *    program.
 * PARAMETERS:
 *    caller: The name of the function that calls this.
 *    called: The name of the function that returned the error code.
 *    error_code: One of the 3DVIEWNIX error codes.
 * SIDE EFFECTS: The error message is logged in the 3DVIEWNIX.ERR file(s).
 * ENTRY CONDITIONS: The global variables argv0, windows_open must be
 *    appropriately initialized.  The signal must not be caught or ignored.
 * RETURN VALUE: None
 * EXIT CONDITIONS: Produces a non-zero exit code.
 * HISTORY:
 *    Created: 1992 by Dewey Odhner
 *
 *****************************************************************************/
void manipulate_abort(const char caller[], const char called[], int error_code)
{
	fprintf(stderr, "%s error %d\n", called, error_code);
	exit(error_code);
}

void display_error ( int error_code ) {
	if (error_code)
		fprintf( stderr, "Error %d detected. \n", error_code );
}

void report_malloc_error(void)
{
	fprintf(stderr, "Memory allocation failure.\n");
	exit(1);
}
		
