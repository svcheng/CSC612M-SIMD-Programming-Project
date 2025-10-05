#include "stdio.h"
#include <stdlib.h>
#include <time.h>
#include "math.h"

extern void kernel2(int n, float* A, float* B, float* C, float* D);


void kernel1(int n, float* A, float* B, float* C, float* D) {
	for (int i = 0; i < n; i++) {
		A[i] = B[i] + C[i] * D[i];
	}
}

// prints first five and last five elements of v
void printVec(int n, float* v) {
	if (n <= 10) {
		for (int i = 0; i < n; i++) {
			printf("%f ", v[i]);
		}
	}
	else {
		for (int i = 0; i < 5; i++) {
			printf("%f ", v[i]);
		}
		printf(" ... ");
		for (int i = n-5; i < n; i++) {
			printf("%f ", v[i]);
		}
	}
	printf("\n");
}

// set all elements of v to zero
void zeroOut(int n, float* v) {
	for (int i = 0; i < n; i++) {
		v[i] = 0.0f;
	}
}

// performs corerctness checks for each kernel with a given input size
void correctnessChecks(int n, float* A, float* B, float* C, float* D) {
	printf("------CORRECTNESS CHECK------\n");
	zeroOut(n, A);
	kernel1(n, A, B, C, D);
	printf("C kernel: ");
	printVec(n, A);

	zeroOut(n, A);
	kernel2(n, A, B, C, D);
	printf("x86:      ");
	printVec(n, A);

	/*zeroOut(n, A);
	kernel1(n, A, B, C, D);
	printf("SIMD XMM: ");
	printVec(n, A);

	zeroOut(n, A);
	kernel1(n, A, B, C, D);
	printf("SIMD YMM: ");
	printVec(n, A);*/

	printf("\n\n");
}

// tests all 4 kernels for a given n
void testKernels(int n, float* A, float* B, float* C, float* D, int exp, int extra) {
	clock_t end;
	clock_t start;
	double k1Time = 0.0;
	double k2Time = 0.0;
	double k3Time = 0.0;
	double k4Time = 0.0;

	printf("-------------  INPUT SIZE: 2^%d + %d  -------------\n", exp, extra);
	correctnessChecks(n, A, B, C, D);

	for (int i = 0; i < 30; i++) {
		start = clock();
		kernel1(n, A, B, C, D);
		end = clock();
		k1Time += (double)(end - start) / CLOCKS_PER_SEC;

		start = clock();
		kernel2(n, A, B, C, D);
		end = clock();
		k2Time += (double)(end - start) / CLOCKS_PER_SEC;
	
		/*start = clock();
		kernel3(n, A, B, C, D);
		end = clock();
		k3Time += (double)(end - start) / CLOCKS_PER_SEC;

		start = clock();
		kernel4(n, A, B, C, D);
		end = clock();
		k4Time += (double)(end - start) / CLOCKS_PER_SEC;*/
	}

	printf("------AVG EXECUTION TIMES------\n");
	printf("C kernel: %f\n", k1Time / 30.0);
	printf("x86     : %f\n", k2Time / 30.0);
	printf("SIMD XMM: %f\n", k3Time / 30.0);
	printf("SIMD YMM: %f\n", k4Time / 30.0);
	printf("---------------------------------------------------\n\n\n");
}

int main() {
	// initialize variables
	
	//int n1 = 1048576 + 7; // 2^20 + 7
	//int n2 = 67108864 + 3; // 2^26 + 3
	//int n3 = 268435456; // 2^28
	int n1 = 1024;
	int n2 = 1024*2;
	int n3 = 1024*4;
	float* A = (float*)malloc(n3 * sizeof(float));
	float* B = (float*)malloc(n3 * sizeof(float));
	float* C = (float*)malloc(n3 * sizeof(float));
	float* D = (float*)malloc(n3 * sizeof(float));
	for (int i = 0; i < n3; i++) {
		B[i] = (float)sin(i * 0.001);
		C[i] = (float)cos(i * 0.002);
		D[i] = (float)tan(i * 0.0005 + 1.0);
	}

	// begin test for each input size
	testKernels(n1, A, B, C, D, 20, 7);
	testKernels(n2, A, B, C, D, 26, 3);
	testKernels(n3, A, B, C, D, 28, 0);

	return 0;
}