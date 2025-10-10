#include "stdio.h"
#include <stdlib.h>
#include <time.h>
#include "math.h"
#include <windows.h>


/// <summary>
/// Computes element-wise: A[i] = B[i] + C[i] * D[i] for i in [0, n).
/// It can serve as a baseline for x86-64.
/// </summary>
/// <param name="n">Number of float elements to process.</param>
/// <param name="A">Destination array. Written with the results.</param>
/// <param name="B">Source array. Read-only.</param>
/// <param name="C">Source array. Read-only.</param>
/// <param name="D">Source array. Read-only.</param>
extern void vecaddmulx86_64(size_t n, float* A, float* B, float* C, float* D);


/// <summary>
/// SIMD implementation using 128-bit XMM vectors (using SIMD operations).
/// Computes A[i] = B[i] + C[i] * D[i] in chunks of 4 floats with a boundary check for 
/// vectors with a size not divisible by 4.
/// It also handles unaligned inputs/outputs.
/// </summary>
/// <param name="n">Number of float elements to process.</param>
/// <param name="A">Destination array. Written with the results.</param>
/// <param name="B">Source array. Read-only.</param>
/// <param name="C">Source array. Read-only.</param>
/// <param name="D">Source array. Read-only.</param>
extern void vecaddmulxmm(size_t n, float* A, float* B, float* C, float* D);


/// <summary>
/// SIMD implementation using 256-bit YMM vectors (using SIMD operations).
/// Computes A[i] = B[i] + C[i] * D[i] in chunks of 8 floats.
/// It also handles unaligned inputs/outputs.
/// </summary>
/// <param name="n">Number of float elements to process.</param>
/// <param name="A">Destination array. Written with the results.</param>
/// <param name="B">Source array. Read-only.</param>
/// <param name="C">Source array. Read-only.</param>
/// <param name="D">Source array. Read-only.</param>
extern void vecaddmulymm(size_t n, float* A, float* B, float* C, float* D);


/// <summary>
/// vecaddc is the first kernel of the project that performs the vector operation for
/// vectors B, C, D and place the result in vector A. It is purely a C program, as it
/// doesn't call any external SASM function. 
/// 
/// Assuming that vectors A, B, C, and D have the same size, the function
/// iterates through each element in the aformentioned vectors and performs the operation. 
/// Then, the result is stored in vector A.
/// </summary>
/// <param name="n">the size of the vectors</param>
/// <param name="A">Contains the result</param>
/// <param name="B">First Operand</param>
/// <param name="C">Second Operand</param>
/// <param name="D">Third Operand</param>
void vecaddmulc(size_t n, float* A, float* B, float* C, float* D) {
	int i;
	for (i = 0; i < n; i++) {
		A[i] = B[i] + C[i] * D[i];
	}
}


/// <summary>
/// printVec is the function used for verification. It displays all of the elements 
/// in vector v in the console, if the size of the vector is less than 10. Otherwise, 
/// it displays the first five elements and the last five elements, which are used
/// for verification
/// </summary>
/// <param name="n">size of the vector v</param>
/// <param name="v">the vector to be printed</param>
void printVec(size_t n, float* v) {
	if (n <= 10) {
		for (int i = 0; i < n; i++) {
			printf("%f ", v[i]);
		}
	}
	else {
		for (int i = 0; i < 5; i++) {
			printf("%.4f ", v[i]);
		}
		printf(" ... ");
		for (int i = n-5; i < n; i++) {
			printf("%.4f ", v[i]);
		}
	}
	printf("\n");
}


/// <summary>
/// zeroOut sets all the elements of v to zero (0). This is used for initialization 
/// to avoid garbage values after memory allocation.
/// </summary>
/// <param name="n">size of the vector v</param>
/// <param name="v">the vector to be initialized with 0</param>
void zeroOut(size_t n, float* v) {
	for (int i = 0; i < n; i++) {
		v[i] = 0.0f;
	}
}


/// <summary>
/// correctnessChecks validates the output of different implementations of a computational kernel
/// (e.g., C reference vs. SIMD-optimized x86 version). It compares their results by printing the
/// computed vectors side-by-side for manual verification.
/// </summary>
/// <param name="n">Number of elements in each vector (A, B, C, D)</param>
/// <param name="A">Output vector used to store and display results of each kernel</param>
/// <param name="B">First input vector used by kernel computations</param>
/// <param name="C">Second input vector used by kernel computations</param>
/// <param name="D">Third input vector used by kernel computations</param>
void correctnessChecks(size_t n, float* A, float* B, float* C, float* D) {
	printf("===================================== CORRECTNESS CHECKS =====================================\n");
	zeroOut(n, A);
	vecaddmulc(n, A, B, C, D);
	printf("C kernel: ");
	printVec(n, A);

	zeroOut(n, A);
	vecaddmulx86_64(n, A, B, C, D);
	printf("x86:      ");
	printVec(n, A);

	zeroOut(n, A);
	vecaddmulxmm(n, A, B, C, D);
	printf("XMM:      ");
	printVec(n, A);

	zeroOut(n, A);
	vecaddmulymm(n, A, B, C, D);
	printf("YMM:      ");
	printVec(n, A);

	printf("\n\n");
}


int main() {
	const size_t ARRAY_SIZE = 1 << 20;
	const size_t ARRAY_BYTES = ARRAY_SIZE * sizeof(float);
	int i;
	printf("Number of Elements = %zd\n", ARRAY_SIZE);

	// Declare test variables
	const size_t TEST_NUM = 30;
	int j;
	printf("Number of Tests per Kernel = %zd\n", TEST_NUM);

	// Declare the timer variable
	LARGE_INTEGER li;
	long long int start, end;
	double PCFreq, k1_elapse, k2_elapse, k3_elapse, k4_elapse;
	QueryPerformanceFrequency(&li);
	PCFreq = (double)(li.QuadPart);

	// Declare Array
	float* A, * B, * C, * D, * Z;
	A = (float*)malloc(ARRAY_BYTES);
	B = (float*)malloc(ARRAY_BYTES);
	C = (float*)malloc(ARRAY_BYTES);
	D = (float*)malloc(ARRAY_BYTES);
	Z = (float*)malloc(ARRAY_BYTES);

	// Initialize Array
	for (i = 0; i < ARRAY_SIZE; i++) {
		B[i] = (float)sin(i * 0.001);
		C[i] = (float)sin(i * 0.002);
		D[i] = (float)sin(i * 0.001 + 1.0);
	}

	// check for correctness
	correctnessChecks(ARRAY_SIZE, A, B, C, D);


	// ============================ FIRST KERNEL: PROGRAM IN C ============================
	k1_elapse = 0;
	for (j = 0; j < TEST_NUM; j++) {
		QueryPerformanceCounter(&li);
		start = li.QuadPart;
		vecaddmulc(ARRAY_SIZE, A, B, C, D);
		QueryPerformanceCounter(&li);
		end = li.QuadPart;
		k1_elapse += ((double)(end - start)) * 1000.0 / PCFreq;
		zeroOut(ARRAY_SIZE, A);
	}
	printf("Average Time in C = %f ms\n \n", k1_elapse / TEST_NUM);

	// Save output for checking other kernels
	vecaddmulc(ARRAY_SIZE, Z, B, C, D);


	// ============================ SECOND KERNEL: PROGRAM IN x86-64 REGISTER ============================
	k2_elapse = 0;
	for (j = 0; j < TEST_NUM; j++) {
		QueryPerformanceCounter(&li);
		start = li.QuadPart;
		vecaddmulx86_64(ARRAY_SIZE, A, B, C, D);
		QueryPerformanceCounter(&li);
		end = li.QuadPart;
		k2_elapse += ((double)(end - start)) * 1000.0 / PCFreq;

		// Check if the output is correct by comparing it to the output of the first kernel
		int fail = 0;
		for (i = 0; i < ARRAY_SIZE; i++) {
			if (Z[i] != A[i])
				fail += 1;
		}
		if (fail > 0)
			printf("Failed %d times\n \n", fail);

		zeroOut(ARRAY_SIZE, A);
	}
	printf("Average Time in x86_64 = %f ms\n \n", k2_elapse / TEST_NUM);


	// ============================ THIRD KERNEL: PROGRAM IN x86-64 SIMD AVX2 XMM REGISTER ============================
	k3_elapse = 0;
	for (j = 0; j < TEST_NUM; j++) {
		QueryPerformanceCounter(&li);
		start = li.QuadPart;
		vecaddmulxmm(ARRAY_SIZE, A, B, C, D);
		QueryPerformanceCounter(&li);
		end = li.QuadPart;
		k3_elapse += ((double)(end - start)) * 1000.0 / PCFreq;

		// Check if the output is correct by comparing it to the output of the first kernel
		int fail = 0;
		for (i = 0; i < ARRAY_SIZE; i++) {
			if (Z[i] != A[i])
				fail += 1;
		}
		if (fail > 0)
			printf("Failed %d times", fail);

		zeroOut(ARRAY_SIZE, A);
	}
	printf("Average Time in SIMD XMM = %f ms\n \n", k3_elapse / TEST_NUM);
	

	// ============================ FOURTH KERNEL: PROGRAM IN x86-64 SIMD AVX2 YMM REGISTER ============================
	k4_elapse = 0;
	for (j = 0; j < TEST_NUM; j++) {
		QueryPerformanceCounter(&li);
		start = li.QuadPart;
		vecaddmulymm(ARRAY_SIZE, A, B, C, D);
		QueryPerformanceCounter(&li);
		end = li.QuadPart;
		k4_elapse = ((double)(end - start)) * 1000.0 / PCFreq;

		// Check if the output is correct by comparing it to the output of the first kernel
		int fail = 0;
		for (i = 0; i < ARRAY_SIZE; i++) {
			if (Z[i] != A[i])
				fail += 1;
		}
		if (fail > 0)
			printf("Failed %d times", fail);

		zeroOut(ARRAY_SIZE, A);
	}
	printf("Average Time in SIMD YMM = %f ms\n \n", k4_elapse / TEST_NUM);


	return 0;
}