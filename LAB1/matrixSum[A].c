#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */

/* Added this struct to store value and position together */
//======================A========================
typedef struct
 {
    int val;
    int row;
    int col;
} Element;

//======================== A ==========================
// Global arrays to store the best results found by each individual worker
Element max_elements[MAXWORKERS]; 
Element min_elements[MAXWORKERS];
//===================================================

/* a reusable counter barrier */
//did not change anything
void Barrier() 
{
  pthread_mutex_lock(&barrier);
  numArrived++;

  if (numArrived == numWorkers) 
  {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } 
  else
  {
     pthread_cond_wait(&go, &barrier);
  }
   
  pthread_mutex_unlock(&barrier);
}

/* timer */
//did not change anything
double read_timer() 
{
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;

    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }

    gettimeofday( &end, NULL );

    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}


double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);


/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) 
{
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;

  if (size > MAXSIZE) size = MAXSIZE;

  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

  stripSize = size/numWorkers;

  /* initialize the matrix */
  /*
  for (i = 0; i < size; i++) 
  {
	  for (j = 0; j < size; j++) 
    {
          matrix[i][j] = 1;//rand()%99;
	  }

  }*/

  //!!!! modified intiliasiation so that it is not filled only with 1s anymore
srand(time(NULL)); 
for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++) {
        matrix[i][j] = rand() % 1000; // Random numbers 0-999
    }
}

  /* print the matrix */
#ifdef DEBUG
  for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

  /* do the parallel work: create the workers */
  start_time = read_timer();

  for (l = 0; l < numWorkers; l++)
  {
     pthread_create(&workerid[l], &attr, Worker, (void *) l);
  } 
  pthread_exit(NULL);
}


/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */


void *Worker(void *arg) {
  long myid = (long) arg;
  int total, i, j, first, last;
  //======A========
// LOCAL TRACKING: Each worker starts with the most extreme possible values
  Element myMax = {INT_MIN, -1, -1};
  Element myMin = {INT_MAX, -1, -1};

//==========================================

#ifdef DEBUG
  printf("worker %ld (pthread id %p) has started\n", myid, (void *)pthread_self());
#endif

  /* determine first and last rows of my strip */
  first = myid*stripSize;
  last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

  /* sum values in my strip */
  total = 0;
  for (i = first; i <= last; i++)
  {
    for (j = 0; j < size; j++)
    {
      int val = matrix[i][j];
      total += val;
//===============A==================
// COMPARISON LOGIC: Check if current element is a new local max or min
      if (val > myMax.val) {
          myMax.val = val;
          myMax.row = i;
          myMax.col = j;
      }
      if (val < myMin.val) {
          myMin.val = val;
          myMin.row = i;
          myMin.col = j;
      }
    }

  }

//==========================================
  sums[myid] = total;
	
//===============A==================
// SHARING RESULTS: Save local results to global arrays for Worker 0 to access
  max_elements[myid] = myMax;
  min_elements[myid] = myMin;
//==========================================
  Barrier();

  if (myid == 0) 
  {
    total = 0;
	 
// INITIALIZE GLOBAL WINNERS: Start with the first possible values
    Element finalMax = {INT_MIN, -1, -1};
    Element finalMin = {INT_MAX, -1, -1};
// GLOBAL REDUCTION: Iterate through the "winners" of every worker
    for (i = 0; i < numWorkers; i++)
    {
      total += sums[i];
     //==============A===========================
		// Update global max if a worker found a higher value
      if (max_elements[i].val > finalMax.val) 
	  {
          finalMax = max_elements[i];
      }
		// Update global min if a worker found a smaller value
      if (min_elements[i].val < finalMin.val)
	  {
          finalMin = min_elements[i];
      }
    }
     //==========================================
    /* get end time */
    end_time = read_timer();
    /* print results */
    printf("Max value: %d at [%d][%d]\n", finalMax.val, finalMax.row, finalMax.col);
    printf("Min value: %d at [%d][%d]\n", finalMin.val, finalMin.row, finalMin.col);
    printf("The execution time is %g sec\n", end_time - start_time);
  }
  return NULL;
}
