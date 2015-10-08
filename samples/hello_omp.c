#include <stdio.h>
#include <omp.h>

int main (int argc, char ** argv)
{
#pragma omp parallel //num_threads (8)
	printf ("\nHello Omp");
	return 0;
}
