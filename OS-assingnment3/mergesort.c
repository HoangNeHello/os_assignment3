/**
 * This file implements a parallel version of merge sort using pthreads.
 * It supports recursive thread creation up to a cutoff level.
 */

#include <stdio.h>
#include <string.h>   /* for memcpy */
#include <stdlib.h>   /* for malloc/free */
#include <pthread.h>  /* for pthread_create/join */
#include "mergesort.h" /* gives access to A, B, cutoff, and struct argument */

/* --------------------------------------------------------------------
 * merge()
 * --------------------------------------------------------------------
 * Purpose:
 *   Merge two sorted subarrays [leftstart..leftend] and [rightstart..rightend]
 *   from global array A into sorted order, using global temporary array B.
 *   Then copy merged results back to A.
 * 
 * Logic:
 *   - i indexes the left subarray, j the right subarray, k the merged position.
 *   - Compare elements and copy smaller one into B[k].
 *   - Copy leftovers from either side.
 *   - Finally, copy merged block from B back into A.
 */
void merge(int leftstart, int leftend, int rightstart, int rightend) {
    int i = leftstart, j = rightstart, k = leftstart; // starting indices

    // Merge while both halves have elements remaining
    while (i <= leftend && j <= rightend) {
        B[k++] = (A[i] <= A[j]) ? A[i++] : A[j++];
    }

    // Copy any remaining elements from the left half
    while (i <= leftend)
        B[k++] = A[i++];

    // Copy any remaining elements from the right half
    while (j <= rightend)
        B[k++] = A[j++];

    // Copy merged results back into A
    memcpy(&A[leftstart], &B[leftstart], (rightend - leftstart + 1) * sizeof(int));
}

/* --------------------------------------------------------------------
 * my_mergesort()
 * --------------------------------------------------------------------
 * Purpose:
 *   Standard recursive (serial) merge sort.
 *   This is used when we reach the cutoff level or run in single-thread mode.
 * 
 * Logic:
 *   - Base case: single element (already sorted).
 *   - Recursive case: divide into two halves, sort each, then merge.
 */
void my_mergesort(int left, int right) {
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;

    // Sort left and right halves recursively
    my_mergesort(left, mid);
    my_mergesort(mid + 1, right);

    // Merge sorted halves
    merge(left, mid, mid + 1, right);
}

/* --------------------------------------------------------------------
 * parallel_mergesort()
 * --------------------------------------------------------------------
 * Purpose:
 *   Recursively spawns threads to sort halves of the array in parallel.
 *   Thread creation stops when 'level' reaches 'cutoff'.
 * 
 * Parameters:
 *   arg -> pointer to a struct argument containing:
 *           - left  : left index of subarray
 *           - right : right index of subarray
 *           - level : current recursion depth
 * 
 * Logic:
 *   - If level >= cutoff → perform serial merge sort.
 *   - Otherwise:
 *       1. Split array in half.
 *       2. Spawn two new threads, each handling one half.
 *       3. Wait for both threads (pthread_join).
 *       4. Merge results.
 *   - Free dynamically allocated argument before returning.
 */
void *parallel_mergesort(void *arg) {
    struct argument *a = (struct argument *)arg;
    int left = a->left;
    int right = a->right;
    int level = a->level;

    // Base case: single element
    if (left >= right) {
        // Only free if this is a child thread (level > 0)
        // The root thread's arg (level 0) is freed by test-mergesort.c
        if (level > 0) free(a);
        return NULL;
    }

    // Stop spawning new threads once cutoff depth is reached
    if (level >= cutoff) {
        my_mergesort(left, right);
        if (level > 0) free(a);
        return NULL;
    }

    // Correct midpoint calculation (ensure proper split)
    int mid = left + (right - left) / 2;

    // Prepare arguments for left and right halves
    struct argument *leftArg = buildArgs(left, mid, level + 1);
    struct argument *rightArg = buildArgs(mid + 1, right, level + 1);

    pthread_t t1, t2;

    // Create threads for left and right halves
    pthread_create(&t1, NULL, parallel_mergesort, leftArg);
    pthread_create(&t2, NULL, parallel_mergesort, rightArg);

    // Wait for both threads to finish
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // Merge the two sorted halves
    merge(left, mid, mid + 1, right);

    // Free argument structure before returning
    if(level>0) free(a);
    return NULL;
}

/* --------------------------------------------------------------------
 * buildArgs()
 * --------------------------------------------------------------------
 * Purpose:
 *   Helper to allocate and initialize the argument struct used by threads.
 * 
 * Parameters:
 *   left, right → indices of the subarray
 *   level       → recursion depth
 * 
 * Returns:
 *   Pointer to a heap-allocated struct argument.
 */
struct argument *buildArgs(int left, int right, int level) {
    struct argument *arg = malloc(sizeof(struct argument));
    arg->left = left;
    arg->right = right;
    arg->level = level;
    return arg;
}
