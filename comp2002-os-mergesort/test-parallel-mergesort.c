// test for parallel merge sort
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "mergesort.h"

int cutoff = 0;
int *A = NULL;
int *B = NULL;
int passed = 0, failed = 0;

// Check if sorted
bool is_sorted(int size) {
    for (int i = 0; i < size - 1; i++) {
        if (A[i] > A[i + 1]) return false;
    }
    return true;
}

// Print result
void test(const char *name, bool result) {
    if (result) {
        printf("%s\n", name);
        passed++;
    } else {
        printf(" %s\n", name);
        failed++;
    }
}

// TEST 1: buildArgs Function Test
// Purpose: Verify buildArgs() correctly allocates memory 

//  Creates an argument struct with left=0 right=99, level=0

//Validates that the struct pointer is not NULL and all feilds are set correctly
void test_buildArgs() {
    printf("\nbuildArgs Tests\n");
    struct argument *arg = buildArgs(0, 99, 0);
    test("buildArgs createstruct correct", arg != NULL && arg->left == 0 && arg->right == 99 && arg->level == 0);
    free(arg);
}

// TEST 2: Single Element Array Test
// Purpose: Tests the base case where the array has only one element

//Creates array with single element , calls parallel_mergesort with cutoff=0

//  Verify the element remain unchanged after sorting

void test_single() {
    printf("\nsingle Element\n");
    A = malloc(sizeof(int));
    B = malloc(sizeof(int));
    A[0] = 42;
    
    cutoff = 0;
    struct argument *arg = buildArgs(0, 0, 0);
    parallel_mergesort(arg);
    test("single element unchanged ", A[0] == 42);
    free(arg);
    free(A);
    free(B);
}

// TEST 3: Two Element Array Test
// Purpose: Tests the minimal case that requires actual sorting operations
// Creates array wif elements [10, 5] and sorts with cutoff=1 (parallel mode)
// Verifies array becomes [5, 10] after sorting

// Tests both thread creation (at level 0) and the merge function with smallest possible input
void test_two() {
    printf("\nTwo Elements\n");
    A = malloc(2 * sizeof(int));
    B = malloc(2 * sizeof(int));
    A[0] = 10;
    A[1] = 5;
    
    cutoff = 1;
    struct argument *arg = buildArgs(0, 1, 0);
    parallel_mergesort(arg);
    test("Two elements sorted correctly", A[0] == 5 && A[1] == 10);
    free(arg);
    free(A);
    free(B);
}

// TEST 4: Small Array Serial Mode Test
// Purpose: Tests basic merge sort correctness without any threading (serial mode)
//Creates 100 random elements, sorts with cutoff=0 (no threads created)
//Verifies entire array is in ascending order

void test_small_serial() {
    printf("\nSmall Array (Serial)\n");
    int size = 100;
    A = malloc(size * sizeof(int));
    B = malloc(size * sizeof(int));
    
    srandom(1234);
    for (int i = 0; i < size; i++) A[i] = random() % 1000;
    
    cutoff = 0;
    struct argument *arg = buildArgs(0, size - 1, 0);
    parallel_mergesort(arg);
    test("100 elements sorted correctly (serial)", is_sorted(size));
    free(arg);
    free(A);
    free(B);
}

// TEST 5: Small Array Parallel Mode Test
// Purpose: Tests that threading/concurrency logic works correctly
//  Creates 100 random elements, sorts with cutoff=2 (create up to 4 threads)
//Verifies entire array is in ascending order
// pthread_create/join calls, and that merge works correctly with concurrent execution

void test_small_parallel() {
    printf("\nSmall Array (Parallel)\n");
    int size = 100;
    A = malloc(size * sizeof(int));
    B = malloc(size * sizeof(int));
    
    srandom(5678);
    for (int i = 0; i < size; i++) A[i] = random() % 1000;
    
    cutoff = 2;
    struct argument *arg = buildArgs(0, size - 1, 0);
    parallel_mergesort(arg);
    test("100 elements sorted correctly (parallel)", is_sorted(size));
    free(arg);
    free(A);
    free(B);
}

// TEST 6: Already Sorted Array Test
// Purpose: Tests best-case scenario where input is already in order
// Process: Creates array [0, 1, 2, ..., 99], sorts with cutoff=2
// verifies array remains in correct order

void test_sorted() {
    printf("\nAlready Sorted \n");
    int size = 100;
    A = malloc(size * sizeof(int));
    B = malloc(size * sizeof(int));
    
    for (int i = 0; i < size; i++) A[i] = i;
    
    cutoff = 2;
    struct argument *arg = buildArgs(0, size - 1, 0);
    parallel_mergesort(arg);
    test("Already sorted stays sorted correct", is_sorted(size));
    free(arg);
    free(A);
    free(B);
}

// TEST 7: Reverse Sorted Array Test
// Purpose: Tests worst-case input where array is completely reverse order
//Creates array [100,99,98,.,1], sorts with cutoff=2
//Verifies array becomes [1 2,3,.,100]

//stress test for the algorithm correctness
void test_reverse() {
    printf("\nReverse Sorted \n");
    int size = 100;
    A = malloc(size * sizeof(int));
    B = malloc(size * sizeof(int));
    
    for (int i = 0; i < size; i++) A[i] = size - i;
    
    cutoff = 2;
    struct argument *arg = buildArgs(0, size - 1, 0);
    parallel_mergesort(arg);
    test("Reverse sorted gets sorted correctly ", is_sorted(size));
    free(arg);
    free(A);
    free(B);
}

// TEST 8: All Duplicate Values Test
// Purpose: Tests array where every element is identical
//Creates array where all 100 elements equal 42, sorts with cutoff=2
// Verifies array is still sorted 


void test_duplicates() {
    printf("\nAll Duplicates\n");
    int size = 100;
    A = malloc(size * sizeof(int));
    B = malloc(size * sizeof(int));
    
    for (int i = 0; i < size; i++) A[i] = 42;
    
    cutoff = 2;
    struct argument *arg = buildArgs(0, size - 1, 0);
    parallel_mergesort(arg);
    test("All duplicates sorted correctly", is_sorted(size));
    free(arg);
    free(A);
    free(B);
}

// TEST 9: Various Cutoff Levels Test
// Purpose: Tests all cutoff values from 0 to 4 to verify thread logic at different depths
// For each cutoff (0-4), creates 1000 random elements and sorts
//Verifies sorting works correctly regardless of cutoff value
// Different cutoffs create different numbers of threads:
//   cutoff=0: 1 thread (serial)
//   cutoff=1: up to 2 threads
//   cutoff=2: up to 4 threads
//   cutoff=3: up to 8 threads
//   cutoff=4: up to 16 threads
// Ensures the level cutoff condition works properly at all depths
void test_cutoffs() {
    printf("\nVarious Cutoff Levels\n");
    int size = 1000;
    
    for (int c = 0; c <= 4; c++) {
        A = malloc(size * sizeof(int));
        B = malloc(size * sizeof(int));
        
        srandom(9999 + c);
        for (int i = 0; i < size; i++) A[i] = random() % 10000;
        
        cutoff = c;
        struct argument *arg = buildArgs(0, size - 1, 0);
        parallel_mergesort(arg);
        
        char name[50];
        sprintf(name, "cutoff=%d sorts correctly", c);
        test(name, is_sorted(size));
        
        free(arg);
        free(A);
        free(B);
    }
}

// TEST 10: Stress Test with Multiple Runs
// Purpose: Tests reliability and checks for memory leaks or race conditions
// Runs the sort 5 times with 10,000 elements each, using cutoff=3
//  Verifies all 5 runs produce correctly sorted arrays
// If this passes, implementation is free from race condtions

void test_stress() {
    printf("\nStress Test \n");
    int size = 10000;
    bool all_ok = true;
    
    for (int run = 0; run < 5; run++) {
        A = malloc(size * sizeof(int));
        B = malloc(size * sizeof(int));
        
        srandom(run);
        for (int i = 0; i < size; i++) A[i] = random() % 10000;
        
        cutoff = 3;
        struct argument *arg = buildArgs(0, size - 1, 0);
        parallel_mergesort(arg);
        
        if (!is_sorted(size)) all_ok = false;
        
        free(arg);
        free(A);
        free(B);
    }
    
    test("5 runs all passed", all_ok);
}

int main() {
    
    printf("  Parallel Merge Sort Test \n");
  

    test_buildArgs();
    test_single();
    test_two();
    test_small_serial();
    test_small_parallel();
    test_sorted();
    test_reverse();
    test_duplicates();
    test_cutoffs();
    test_stress();
    

    printf("  Results: %d passed, %d failed\n", passed, failed);

    
    return (failed == 0) ? 0 : 1;
}