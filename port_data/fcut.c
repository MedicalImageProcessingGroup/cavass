/*
  Copyright 1993-2014, 2017 Medical Image Processing Group
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
#include <string.h>

#define BUFFER_SIZE 1024

void quit(int err, char argv0[]);

static unsigned bytes_done;

/* Modified: 9/15/99 file arg and -e flag allowed by Dewey Odhner. */
/* Modified: 10/11/01 Modified to work with large files by Dewey Odhner. */
int main(argc, argv)
	int argc;
	char **argv;
{
	double chunk_offset, chunk_size;
	unsigned bytes_just_read, bytes_just_writ;
	char buffer[BUFFER_SIZE];
	FILE *fp;

	if (argc<3 || argc>5 || sscanf(argv[1], "%lf", &chunk_offset)!=1 ||
			sscanf(argv[2], "%lf", &chunk_size)!=1 ||
			(argc==5 && strcmp(argv[4], "-e")))
	{
		fprintf(stderr, "Usage: %s chunk_offset chunk_size [file [-e]]\n",
			argv[0]);
		exit(1);
	}
	if (argc == 3)
		fp = stdin;
	else
		fp = fopen(argv[3], "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Can't open %s\n", argv[3]);
		exit(1);
	}
	if (argc == 5)
	{
		if (fseek(fp, -(long)chunk_offset, 2))
		{	fprintf(stderr, "seek error\n");
			exit(1);
		}
	}
	else
	{
		while (chunk_offset > 0x40000000)
		{
			if (fseek(fp, 0x40000000, 1))
			{
				fprintf(stderr, "seek error\n");
				exit(1);
			}
			chunk_offset -= 0x40000000;
		}
		if (fseek(fp, (long)chunk_offset, 1))
		{
			fprintf(stderr, "seek error\n");
			exit(1);
		}
	}
	while (chunk_size>=.5 && !feof(fp))
	{	bytes_just_read = (unsigned)fread(buffer, 1,
			chunk_size>BUFFER_SIZE? BUFFER_SIZE: (size_t)chunk_size, fp);
		if (ferror(fp))
			quit(1, argv[0]);
		chunk_size -= bytes_just_read;
		bytes_just_writ = (unsigned)fwrite(buffer, 1, bytes_just_read, stdout);
		bytes_done += bytes_just_writ;
		if (bytes_just_writ != bytes_just_read)
			quit(1, argv[0]);
	}
	quit(0, argv[0]);
	exit(0);
}

void quit(int err, char argv0[])
{
	if (err)
		perror(argv0);
	fprintf(stderr, "%u bytes copied\n", bytes_done);
	exit(err);
}

