#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	double x;

	if (argc==2 && sscanf(argv[1], "%lf", &x)==1)
	{
		printf("%.0f\n", x);
		exit(0);
	}
	fprintf(stderr, "rint failed.\n");
	exit(1);
}
