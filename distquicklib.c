/* distquicklib: library of concurrent and distributed quicksort algorithms 
   for COMP2310 Assignment 2, 2012.

   Name: REN DING  
   StudentId: u5111810

***Disclaimer***: (modify as appropriate)
  The work that I am submitting for this program is without significant
  contributions from others (excepting course staff).
*/

// uncomment when debugging. Example call: PRINTF(("x=%d\n", x));
//#define PRINTF(x) do { printf x; fflush(stdout); } while (0) 
#define PRINTF(x) /* use when not debugging */

#include <stdio.h>
#include <stdlib.h>  	/* malloc, free */
#include <strings.h> 	/* bcopy() */
#include <string.h>
#include <assert.h>  
#include <unistd.h>     /* fork(), pipe() */
#include <sys/types.h>
#include <sys/wait.h>   
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "quicklib.h"
#include "distquicklib.h"


// distributed quick sort using pipes
void quickPipe(int A[], int n, int p) {
	//the base condition of the recursion
	if(p == 1) {
		quickSort(A,n);
		return;
	}

	int status, nbytes;
	int my_pipe[2];
	status = pipe(my_pipe);

	//partition
	assert(n > 0);
	int pivotIndex = partition(A, n);

	if(fork() == 0) {//child process
		quickPipe(&A[pivotIndex+1], n-pivotIndex-1,p/2);
		//close child read
		close(my_pipe[0]);
		//write to parent
		nbytes = write(my_pipe[1],&A[pivotIndex+1],(n-pivotIndex-1)*sizeof(int));
		//child process should exit, when their participation in the sort is finished
		exit(0);
	} else {//parent process
		quickPipe(A,pivotIndex+1,p/2);
		//close parent write
		close(my_pipe[1]);
		//read from child
		nbytes = read(my_pipe[0],&A[pivotIndex+1],(n-pivotIndex-1)*sizeof(int));
		//parent process should wait for their child process finished
		wait(NULL);
	}

	return;
} //quickPipe()


// distributed quick sort using sockets
void quickSocket(int A[], int n, int p) {
	//the base condition of the recursion
	if(p == 1) {
		quickSort(A,n);
		return;
	}

	//partition
	assert(n > 0);
	int pivotIndex = partition(A, n);

	int sock_1, sock_2;                //two socket descriptors
	struct sockaddr_in client, server; // address info;see /usr/include/linux/in.h
	int nbytes;
	socklen_t namelen;

	//Create TCP/IP socket
	sock_1 = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_1 == -1) {
		perror("socket() Socket was not created");
		exit(-1);
	}
	//printf("Socket created successfully.\n");

	//Address information for use with bind
	server.sin_family = AF_INET;         //it is an IP address
	server.sin_port   = 0;               //use O/S defined port number
	server.sin_addr.s_addr = INADDR_ANY; //use any interface on this host

	//Bind socket to address and port
	if (bind(sock_1, (struct sockaddr *) &server, sizeof(server))) {
		perror("Server bind error");
		exit(-1);
	}

	//Find out what port number was assigned
	namelen = sizeof(server);
	if (getsockname(sock_1, (struct sockaddr *) &server, &namelen)) {
		perror("Server get port number");
		exit(-1);
	}
	//printf("The assigned server port number is %d\n", ntohs(server.sin_port));


	//Set queue limits on socket
	if (listen(sock_1, 1)) {
		perror("Server listen error");
		exit(-1);
	}

	if(fork() == 0) {//child process
		quickSocket(&A[pivotIndex+1], n-pivotIndex-1,p/2);
		int sock_client;                //client socket
		int nbytes;

 		//Create TCP/IP socket
		sock_client = socket(AF_INET, SOCK_STREAM, 0);
		if (sock_client < 0) {
			perror("Client socket creation");
			exit(-1);
		}
		//printf("Client socket created\n");

		//Attempt to connect to the server
		if (connect(sock_client, (struct sockaddr *) &server, sizeof(server))) {
			perror("Client connection failure");
			exit(-1);
		}

		//write to parent
		nbytes = send(sock_client, &A[pivotIndex+1], (n-pivotIndex-1)*sizeof(int), 0);
		if (nbytes < 0) {
			perror("Client failed to send data");
			exit(-1);
		}
		//Close socket and terminate
		close(sock_1);
		//printf("Client terminating\n");

		//child process should exit, when their participation in the sort is finished
		exit(0);
	} else {//parent process
		quickSocket(A,pivotIndex+1,p/2);

		//Now we block waiting for a connection
		namelen = sizeof(client);
		sock_2 = accept(sock_1, (struct sockaddr *) &client, &namelen);
		if (sock_2 < 0) {
			perror("Server accept failed");
			exit(-1);
		}
		getsockname(sock_2, (struct sockaddr *) &server, &namelen);
		//printf("Server received connection from %s, now on port %d\n",inet_ntoa(client.sin_addr), ntohs(server.sin_port));

		//Wait to receive some data from child process
		//nbytes = recv(sock_2, &A[pivotIndex+1], (n-pivotIndex-1)*sizeof(int), 0);
		nbytes = recv(sock_2, &A[pivotIndex+1], (n-pivotIndex-1)*sizeof(int), MSG_WAITALL);
		if (nbytes < 0) {
			perror("Server recv error");
			exit(-1);
		}

		//Close sockets and terminate
		close(sock_2);
		close(sock_1);
		//printf("Server finished.\n");

		//parent process should wait for their child process finished
		wait(NULL);
	}
} //quickSocket()

static int* globalArray;		//global array
static int treeHeight;				//how many levels of the trees
//static pthread_mutex_t* threadsReadyLocks;	//an array of locks for each thread

//power function
int power(int x, int y) {
	int res = 1;
	int i;	
	for(i = 0; i < y;i++)
		res*= x;
	return res;
}

//
// WAIT_JOIN
//
void* quickThreadJoin(void* variables) {
	int* vars = (int*) variables;
	//copy variables
	int id 		= vars[0];
	int length 	= vars[1];
	int startIndex 	= vars[2];
	int p		= vars[3];
	//printf("id=%d,length=%d,startIndex=%d,totoalThreads=%d,treeDepth=%d\n",vars[0],vars[1],vars[2],vars[3],treeDepth);

	//calculate tree depth of the thread
	int treeDepth = treeHeight - lg2(p);
	
	if(p == 1) {
		quickSort(globalArray+startIndex, length);
		return variables;
	} 

	//partition
	int pivotIndex = partition(globalArray+startIndex, length);
	
	//spawns thread based on right subtree
	int* prVars = (int*) malloc(4 * sizeof(int));
	prVars[0] = id+ power(2,treeDepth);
	prVars[1] = length - pivotIndex - 1;
	prVars[2] = startIndex+ pivotIndex + 1;
	prVars[3] = p/2;

	//create thread
	pthread_t thread;
	pthread_create(&thread,NULL,quickThreadJoin, (void *) prVars);

	//process the left subtree itself
	int* plVars = (int*) malloc(4 * sizeof(int));
	plVars[0] = id;
	plVars[1] = pivotIndex+1;
	plVars[2] = startIndex;
	plVars[3] = p/2;
	quickThreadJoin((void*) plVars);
	
	//join thread	
	pthread_join(thread,NULL);

	free(plVars);
	free(prVars);

	return variables;
}

//
// WAIT_MUTEX
//
void* quickThreadMutex(void* variables) {
	int* vars = (int*) variables;
	//copy variables
	int id 		= vars[0];
	int length 	= vars[1];
	int startIndex 	= vars[2];
	int p		= vars[3];

	//calculate tree depth of the thread
	int treeDepth = treeHeight - lg2(p);
	
	if(p == 1) {
		quickSort(globalArray+startIndex, length);
		return variables;
	} 

	//partition
	int pivotIndex = partition(globalArray+startIndex, length);

	//lock parent thread
	pthread_mutex_t threadsReadyLock;
	pthread_mutex_init(&threadsReadyLock, NULL);
	pthread_mutex_lock(&threadsReadyLock);
	
	//spawns thread based on right subtree
	int* prVars = (int*) malloc(4 * sizeof(int));
	prVars[0] = id+ power(2,treeDepth);
	prVars[1] = length - pivotIndex - 1;
	prVars[2] = startIndex+ pivotIndex + 1;
	prVars[3] = p/2;

	//this function is for unlock its parent's lock
	void threadBlock() {
		quickThreadJoin((void*) prVars);
		pthread_mutex_unlock(&threadsReadyLock);
	}
	//create thread
	pthread_t thread;
	pthread_create(&thread,NULL,(void*) threadBlock, NULL);

	//process the left subtree itself
	int* plVars = (int*) malloc(4 * sizeof(int));
	plVars[0] = id;
	plVars[1] = pivotIndex+1;
	plVars[2] = startIndex;
	plVars[3] = p/2;
	quickThreadJoin((void*) plVars);
	pthread_mutex_lock(&threadsReadyLock);

	free(plVars);
	free(prVars);

	return variables;
}

//
// WAIT_MEMLOC
//
void* quickThreadMemloc(void* variables) {
	int* vars = (int*) variables;
	//copy variables
	int id 		= vars[0];
	int length 	= vars[1];
	int startIndex 	= vars[2];
	int p		= vars[3];
	//printf("id=%d,length=%d,startIndex=%d,totoalThreads=%d,treeDepth=%d\n",vars[0],vars[1],vars[2],vars[3],treeDepth);

	//calculate tree depth of the thread
	int treeDepth = treeHeight - lg2(p);
	
	if(p == 1) {
		quickSort(globalArray+startIndex, length);
		return variables;
	} 

	//partition
	int pivotIndex = partition(globalArray+startIndex, length);
	
	//spawns thread based on right subtree
	int* prVars = (int*) malloc(4 * sizeof(int));
	prVars[0] = id+ power(2,treeDepth);
	prVars[1] = length - pivotIndex - 1;
	prVars[2] = startIndex+ pivotIndex + 1;
	prVars[3] = p/2;

	//create thread
	pthread_t thread;
	pthread_create(&thread,NULL,quickThreadMemloc, (void *) prVars);

	//process the left subtree itself
	int* plVars = (int*) malloc(4 * sizeof(int));
	plVars[0] = id;
	plVars[1] = pivotIndex+1;
	plVars[2] = startIndex;
	plVars[3] = p/2;
	quickThreadJoin((void*) plVars);
	
	//join thread	
	pthread_join(thread,NULL);

	free(plVars);
	free(prVars);

	return variables;
}

// concurrent quick sort using pthreads 
void quickThread(int *pA, int pn, int p, enum WaitMechanismType pWaitMech) {
	treeHeight = lg2(p);
	globalArray = pA;

	//start from root thread, variable(id, length, startIndex, the number of the sub threads)
	int* pVars = (int*) malloc(4 * sizeof(int));
	pVars[0] = 0;
	pVars[1] = pn;
	pVars[2] = 0;
	pVars[3] = p;
	
	switch(pWaitMech) {
		case WAIT_JOIN:
			quickThreadJoin((void*) pVars);
			break;
		case WAIT_MUTEX:
			quickThreadMutex((void*) pVars);
			break;
		case WAIT_MEMLOC:
			quickThreadMemloc((void*) pVars);
			break;
	}

	free(pVars);
} //quickThread()


