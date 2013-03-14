/* quicksort.c: test program for  concurrent and distributed quicksort 
   algorithms for COMP2310 Assignment 2, 2012

   written by Peter Strazdins, RSCS ANU, 09/12
   version 1.1 16/10/12
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> /*getopt()*/

#include "quicklib.h"
#include "distquicklib.h"

// handle command line parameter error
void paramError(char *msg) {
  printf("quicksort: parameter error: %s\n", msg);
  printf("usage: quicksort [-t|-s|-p] [-d] [-v v] n [p [s]]\n");
  exit(1);
} //paramError()


// perform distributed or concurrent sort of A[0:n-1] using p processes 
// orthreads
int main(int argc, char *argv[]) {
  char optchar; // option character from command line
  extern char *optarg; // points to option argument 
  extern int optind;   // index of last option parsed by getopt()
  const char *sortAlgs[] = {"pipes", "sockets", "threads"};
  int selectAlg;// algorithm to select; indexes sortAlgs
  int n, p, s;	// command line parameters
  int printA;   // records if n was negative on the command line
  int *A;       // storage for array to be sorted
  enum WaitMechanismType  threadSync; // sync method for quickThread() 

  // parse command line parameters
  n = 0;
  p = 1;
  s = 0;
  printA = 0;
  selectAlg = 0;
  threadSync = WAIT_JOIN;
  
  while ((optchar = getopt(argc, argv, "+pstv:d")) != -1) {
    // extract next option from the command line 
    int v = 0;
    switch (optchar) {
    case 'p': 
      break;
    case 's':
      selectAlg = 1; break;
    case 't':
      selectAlg = 2; break;
    case 'd':
      printA = 1; break;
    case 'v':
      if (sscanf(optarg, "%d", &v) != 1 || v<0 || v>2)
	paramError("invalid integer with -v");
      if (v == 1)
	threadSync = WAIT_MUTEX;
      else if (v == 2)
	threadSync = WAIT_MEMLOC;
      break;
    default:
      paramError("invalid option");
    }
  }
  if (optind >= argc) 
    paramError("array length n is missing");
  n = atoi(argv[optind]);
  if (optind+1 < argc)
    p = atoi(argv[optind+1]);
  if (optind+2 < argc)
    s = atoi(argv[optind+2]);
  if (p <= 0 || (p & (p-1)) != 0) 
    paramError("number of processes p must be a power of 2");
 
  printf("sort array of length %d via %d %s %s with seed %d %s\n", 
	 n, p, selectAlg==2? "threads": "processes using ", 
         selectAlg==2? "": sortAlgs[selectAlg], s,
	 selectAlg!=2 || threadSync==WAIT_JOIN? "": 
	 threadSync==WAIT_MUTEX? "(wait mutex)": "(wait memlock)");

  A = (int *) malloc(n*sizeof(int));
  assert (A!=NULL);
  genArray(A, n, s);
  if (printA)
    printArray(A, n);

  startSortTimer();
  switch(selectAlg) {
  case 1:
    quickSocket(A, n, p);
    break;
  case 2:
    quickThread(A, n, p, threadSync);
    break;
  default:
    quickPipe(A, n, p);
    break;
  }
  stopSortTimer();

  if (printA)
    printArray(A, n);  
  checkArray(A, n, s);

  free(A);
  return 0;
} //main() 
