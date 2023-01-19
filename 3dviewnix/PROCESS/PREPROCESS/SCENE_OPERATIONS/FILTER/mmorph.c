/*
  Copyright 2020 Medical Image Processing Group
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if ! defined (WIN32) && ! defined (_WIN32)
	#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int iterations, itern, pathlength, retval=0;
	char *tempfile, *command;
	struct stat statbuf;

	if (argc!=5 || sscanf(argv[4], "%d", &iterations)!=1 || strlen(argv[2])<4)
	{
		fprintf(stderr, "Usage: mmorph <input> <output> <operation> <iterations>\n operation: [+|-][5|7|9|19|27]\n");
		exit(1);
	}
	pathlength = strlen(argv[2]);
	tempfile = (char *)malloc(pathlength+11);
	command = (char *)malloc(strlen(argv[1])+pathlength+30);
	strncpy(tempfile, argv[2], pathlength-4);
	strncpy(tempfile+pathlength-4, "_morphTEMP", 10);
	strcpy(tempfile+pathlength+6, argv[2]+pathlength-4);
	for (itern=0; itern<iterations; itern++)
	{
		if (itern == 0)
		{
			sprintf(command, "morph %s %s %s 2", argv[1], argv[2], argv[3]);
			retval |= system(command);
		}
		else
		{
			if (stat(argv[2], &statbuf))
			{
				fprintf(stderr, "mmorph failed at iteration %d\n", itern);
				exit(1);
			}
			rename(argv[2], tempfile);
			sprintf(command, "morph %s %s %s 2", tempfile, argv[2], argv[3]);
			retval |= system(command);
			unlink(tempfile);
		}
	}
	exit(retval);
}
