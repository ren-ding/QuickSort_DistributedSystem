/* quicklib: library of provided functions supporting the quicksort
   for comp2310 assignment 2, 2012

   written by Peter Strazdins, RSCS ANU, 09/12
   version 1.1 16/10/12 
*/

#include <stdio.h>
#include <stdlib.h>  	/* srand, rand, malloc, free */
#include <sys/time.h> 	/* gettimeofday() */
#include <assert.h>  

#include "quicklib.h"

//#define DEBUG        /* test correctness of quickSort */
//#define CHECK_PIVOT_QUALITY /* check quality of pivot strategy */

int lg2(int n) {
  int k=0;
  while (n > 1) {
    k++; n /= 2;
  }
  return (k);
} //log2()

// pre: 0<=k<n
// return k-median of A[0..n-1] 
static int median(int A[], int n, int k) {
  int m;
  assert (0 <= k && k < n);
  m = partition(A, n);
  if (m == k) 
    return (A[m]);
  else if (m < k)
    return (median(&A[m+1], n-m-1, k-m-1));
  else
    return (median(A, m+1, k));      
} //median    

#define MIN_PIVOT_SEARCH 8 /* min. array size to search for good pivot */

// create a partition of A[0..n-1] that is likely to be balanced for
// random and near-sorted arrays. 
int partition(int A[], int n) {
  int i, j, pivot;
  if (n <= 1)
    return 0;
  else if (n < MIN_PIVOT_SEARCH)
    pivot = A[n/2];
  else { // select median from a small portion of A[] 
    // note: it is essential that partition() be thread-safe for quickThread(),
    // so sA must not be in a static memory area, i.e. be on the stack 
    int sz = 2*lg2(n), stride = n / sz, sA[sz], i;
    for (i=0; i < sz; i++)
      sA[i] = A[i*stride];
    pivot = median(sA, sz, sz/2);
  }
  i = 0, j = n-1;
  while (i <= j) { // invariant: A[lo..i-1] <= pivot <= A[j+1..hi]    
    while (A[i] < pivot)
      i++;
    while (pivot < A[j])
      j--;
    if (i <= j) {
      int t = A[i];
      A[i] = A[j], A[j] = t;
      i++; j--;
    }
  } //while(...)           
  if (j < 0) j = 0;
  return (j);
} //partition()

#ifdef CHECK_PIVOT_QUALITY
static long int qDepth = 0;  // keep track of average depth of call 
static long int qDepthSum = 0;// this should be log2(n)/2  in ideal case
static long int qCalls = 0;  //
#endif

void quickSort(int A[], int n) {
  int m;
  if (n <= 1) 
    return;
#ifdef CHECK_PIVOT_QUALITY
  qDepth++;
  qDepthSum += qDepth;
  qCalls++;
#endif
  m = partition(A, n);
  quickSort(A, m+1);
  quickSort(&A[m+1], n-m-1);
#ifdef CHECK_PIVOT_QUALITY
  qDepth--;
#endif
} //quickSort()

 
#define MAX_ELT 1000 /* bounds largest element for sorting */  
void genArray(int A[], int n, int s) {
  int i;
  srand(s);
  for (i=0; i< n; i++) 
    A[i] = rand() % MAX_ELT;
} //genArray()

static int compint(const void *vx, const void *vy) {
  int x = * ((int *) vx),
    y = * ((int *) vy);
  return ((x < y)? -1: (x > y)? +1 : 0);
}

void checkArray(int A[], int n, int s) {
  int *B; // temporary buffer
  int nerrs;
#ifdef DEBUG
  int checkPre, checkPost; // checksums for unsorted and sorted arrays
#endif
  int i;
  B = (int *) malloc(n*sizeof(int));
  assert(B!= NULL);
  genArray(B, n, s);
#ifdef DEBUG
  checkPre = 0;
  for (i=0; i<n; i++)
    checkPre += B[i];
#endif
#ifdef CHECK_PIVOT_QUALITY
  printf("average quick sort call depth = %ld (ideal=%d)\n", 
	 qCalls==0? 0: qDepthSum / qCalls, lg2(n));
#endif
  qsort(B, n, sizeof(int), compint);
#ifdef DEBUG
  checkPost = n>0? B[0]: 0;
  nerrs = 0;
  for (i=1; i<n; i++) {
     nerrs += (B[i-1] > B[i]);
     checkPost += B[i];
  }    
  if (nerrs) 
    printf("Error in sort() detected: %d out-of-order elements\n", nerrs);
  if (checkPost != checkPre)
    printf("Error in sort() detected: unexpected values present\n");
#endif
  nerrs = 0;
  for (i=0; i<n; i++) 
    nerrs += (B[i] != A[i]);
  printf("check of sort of array of length %d from seed %d: %d errors\n",
	  n, s, nerrs); 
  free(B);
} //checkArray()
  
void printArray(int A[], int n) {
  int i;
  for (i=0; i<n; i++) {
    printf(" %2d", A[i]);
  }
  printf("\n");
} //printArray()

static double startTime = 0.0;
#define TVAL2SEC(t) ((t).tv_sec + (t).tv_usec*1.0e-06)

void startSortTimer() {
  struct timeval t;
  gettimeofday(&t, NULL);
  startTime = TVAL2SEC(t);
} //startSortTimer()

void stopSortTimer() {
  struct timeval t;
  gettimeofday(&t, NULL);
  printf("elapsed time for sort: %fs\n", TVAL2SEC(t) - startTime);
} //stopSortTimer()

