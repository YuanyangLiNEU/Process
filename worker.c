#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// result = x ^ n / n!
int main(int argc, char *argv[]) {
	int x = 1;
	int n = 1;
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-x") == 0 && i < argc - 1) {
			x = atoi(argv[i + 1]);
		} else if (strcmp(argv[i], "-n") == 0 && i < argc - 1) {
			n = atoi(argv[i + 1]);
		}
	}

	double dividend = 1;
	int divisor = 1;
	for (int i = 1; i <= n; ++i) {
		dividend *= x;
		divisor *= i;
	}

	printf("%d^%d / %d! : %f\n", x, n, n, dividend / divisor);
	return 0;
}