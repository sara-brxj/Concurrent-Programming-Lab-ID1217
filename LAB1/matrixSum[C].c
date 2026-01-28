
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

//pthread_mutex_t barrier;  /* mutex lock for the barrier */
//pthread_cond_t go;        /* condition variable for leaving */
//int numWorkers;           /* number of workers */ 
//int numArrived = 0;    /* number who have arrived */

/* Added this struct to store value and position together */
typedef struct
 {
    int val;
    int row;
    int col;
} Element;


typedef struct {
    int partialSum;
    Element localMax;
    Element localMin;
} WorkerResults;
//=======================C===========================
 /* The "Bag": shared row counter */
int nextRow = 0;            
pthread_mutex_t bagMutex;
//==================================================
//Element max_elements[MAXWORKERS]; 
//Element min_elements[MAXWORKERS];



/*void Barrier() 
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
}*/





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

int numWorkers;           
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

//==============B========================
  int finalTotal = 0;
  Element finalMax = {INT_MIN, -1, -1};
  Element finalMin = {INT_MAX, -1, -1};
//=====================================
  /* set global thread attributes */
  /*pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);*/

  /* initialize mutex and condition variable */
 // pthread_mutex_init(&barrier, NULL);
  //pthread_cond_init(&go, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;

  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;

//========================C==========================
  pthread_mutex_init(&bagMutex, NULL);

//===================================================

  /* initialize the matrix */
  /*
  for (i = 0; i < size; i++) 
  {
	  for (j = 0; j < size; j++) 
    {
          matrix[i][j] = 1;//rand()%99;
	  }

  }*/

  //modified intiliasiation so that it is not filled only with 1s anymore
srand(time(NULL)); 
for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++) {
        matrix[i][j] = rand() % 1000; // Random numbers 0-999
    }
}
pthread_attr_init(&attr);
pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
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
  //pthread_exit(NULL);
  for (l = 0; l < numWorkers; l++)
  {
     WorkerResults *res;
     
// The main thread joins each worker and performs the global reduction
     pthread_join(workerid[l], (void **)&res); 


     finalTotal += res->partialSum;
     if (res->localMax.val > finalMax.val) finalMax = res->localMax;
     if (res->localMin.val < finalMin.val) finalMin = res->localMin;

     // Free the heap memory allocated by the Worker thread
     free(res);
  }
  end_time = read_timer();

  /* Final results printed by the Main Thread */
  printf("The total sum is %d\n", finalTotal);
  printf("The maximum value is %d at [%d][%d]\n", finalMax.val, finalMax.row, finalMax.col);
  printf("The minimum value is %d at [%d][%d]\n", finalMin.val, finalMin.row, finalMin.col);
  printf("Execution time: %g sec\n", end_time - start_time);

  return 0;
}




/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */






void *Worker(void *arg) {
  long myid = (long) arg;
  int total, i, j, first, last, row;
  
  WorkerResults *myResults = (WorkerResults *)malloc(sizeof(WorkerResults));
  myResults->partialSum = 0;
  myResults->localMax = (Element){INT_MIN, -1, -1};
  myResults->localMin = (Element){INT_MAX, -1, -1};

#ifdef DEBUG
  printf("worker %ld (pthread id %p) has started\n", myid, (void *)pthread_self());
#endif

  /* determine first and last rows of my strip */
	// ====================C======================
  while (1) {
    // 1. Lock the bag to get a task
    pthread_mutex_lock(&bagMutex);

    row = nextRow;      // Read current row
    nextRow++;          // Increment for the next worker
	  
    pthread_mutex_unlock(&bagMutex); // 2. Exit Critical Section

    // 3. Check if there are rows left to process
    if (row >= size) 
	{
		break;
	}

	  // 4. Execute the Task: Process the row we just grabbed
    for (j = 0; j < size; j++) {
      int val = matrix[row][j];
      myResults->partialSum += val;

      if(val > myResults->localMax.val) {
          myResults->localMax = (Element){val, row, j};
      }
      if (val < myResults->localMin.val) {
          myResults->localMin = (Element){val, row, j};
      }
    }
    

  }
	// Hand the pointer back to the main thread's join call
  return (void *)myResults;
}
