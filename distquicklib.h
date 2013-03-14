/* distquicklib: library of concurrent and distributed quicksort algorithms
   for COMP2310 Assignment 2, 2012.

   written by Peter Strazdins, RSCS ANU, 09/12
         
   DO NOT MODIFY THIS FILE! Don't even think about it!!!
*/


// distributed quick sort using pipes
void quickPipe(int A[], int n, int p);

// distributed quick sort using sockets 
void quickSocket(int A[], int n, int p);

enum WaitMechanismType { // parent thread waits on:
  WAIT_JOIN,  // pthread_join() on child thread
  WAIT_MUTEX, // pthread_mutex() unlocked by child
  WAIT_MEMLOC, // simple lockword unlocked by child
};
// concurrent quick sort using pthreads
void quickThread(int A[], int n, int p, enum WaitMechanismType waitMechanism);
