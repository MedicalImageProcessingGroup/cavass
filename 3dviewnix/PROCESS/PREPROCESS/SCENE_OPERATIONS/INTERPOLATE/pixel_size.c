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

 
 
 
#include <Viewnix.h>

/*****************************************************************************
 * FUNCTION: main
 * DESCRIPTION: Prints the pixel size in the X1 direction of a scene to
 *    standard output.
 * PARAMETERS:
 *    argc: Argument count -- should be 2.
 *    argv: Argument vector -- argv[1] is the scene file name.
 * SIDE EFFECTS: None
 * ENTRY CONDITIONS: The environment variable VIEWNIX_ENV must be properly set.
 * RETURN VALUE: Exits with 0 on normal completion.
 * EXIT CONDITIONS: Undefined if entry conditions are not met.
 * HISTORY:
 *    Created: 9/25/97 by Dewey Odhner
 *
 *****************************************************************************/
main(argc, argv)
	int argc;
	char **argv;
{
	ViewnixHeader vh;
	FILE *fp;
  	char group[5], element[5];

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s file_name\n", argv[0]);
		exit (1);
	}
	fp = fopen(argv[1], "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "%s: Cannot open file %s\n", argv[0], argv[1]);
		exit (1);
	}
	switch (VReadHeader(fp, &vh, group, element))
	{
		case 0:
		case 106:
		case 107:
			break;
		default:
			fprintf(stderr, "%s: Cannot read file %s header\n", argv[0],
				argv[1]);
			exit (1);
	}
	if (vh.gen.data_type != IMAGE0)
	{
		fprintf(stderr, "%s: %s: Wrong data type.\n", argv[0], argv[1]);
		exit (1);
	}
	printf("%f\n", vh.scn.xypixsz[0]);
	exit (0);
}
