/* matrix summation using OpenMP

   usage with gcc (version 4.2 or higher required):
     gcc -O -fopenmp -o matrixSum-openmp matrixSum-openmp.c 
     ./matrixSum-openmp size numWorkers

*/
// gcc-15 -fopenmp -o matrixSum-openmp matrixSum-openmp.c


// Changes are labelled "ADDED"

#include <omp.h>

double start_time, end_time;

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 8   /* maximum number of workers */

int numWorkers;
int size; 
int matrix[MAXSIZE][MAXSIZE];
void *Worker(void *);


// ADDED:
// a struct containing matrix element's value, row and column
typedef struct {
  int val;
  int row;
  int col;
} MatrixElement;


/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j, total=0;

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

  omp_set_num_threads(numWorkers);

  // ADDED:
  // initialise global results
  MatrixElement globalMax = {INT_MIN, -1, -1};
  MatrixElement globalMin = {INT_MAX, -1, -1};


  /* initialize the matrix */
  for (i = 0; i < size; i++) {
    //  printf("[ ");
	  for (j = 0; j < size; j++) {
      matrix[i][j] = rand()%99;
      //  printf(" %d", matrix[i][j]);
	  }
	  //	printf(" ]\n");
  }

  start_time = omp_get_wtime();

  // ADDED:
  // =================================================================================================

  // start parallel region
  #pragma omp parallel num_threads(numWorkers)
  {
    // DEBUGGING:
    // int id = omp_get_thread_num(); // Get the thread's ID
    
    
    // Thread-local variables to find local max and min in each thread
    int localTotal = 0;
    MatrixElement localMax = {INT_MIN, -1, -1};
    MatrixElement localMin = {INT_MAX, -1, -1};

    #pragma omp for nowait schedule(dynamic, 10) // dynamic scheduling: each thread "grabs" 10 rows at a time
    for (i = 0; i < size; i++) {
      
      /*
      // DEBUGGING: (print only for first column)
      if (i % (size/numWorkers) == 0) {
        printf("Thread %d is processing row %d\n", id, i);
      }
      */
      
      for (j = 0; j < size; j++){
        int currentVal = matrix[i][j]; // set current value
        localTotal += currentVal; // for each element, add it to local total sum (same as old code)

        if (currentVal > localMax.val) { // if current value is larger than local max, update local max
          localMax.val = currentVal;
          localMax.row = i;
          localMax.col = j;
        }

        if (currentVal < localMin.val) { // if current value is smaller than local min, update local min
          localMin.val = currentVal;
          localMin.row = i;
          localMin.col = j;
        }
      }
    }

    // Update global results
    #pragma omp atomic // for simple math atomic is faster than critical. (still a critical section, but it doesn't slow down program much becasue it is just one sum operation)
    total += localTotal; // add local sum to global total sum

    if (localMax.val > globalMax.val || localMin.val < globalMin.val) { // check if a new max or min has been found, otherwise do not enter critical section
      #pragma omp critical // Critical section! only one thread at a time can execute this block of code. Only enter if necessary
      {
        if (localMax.val > globalMax.val) { // if local max is larger than global max, update global max
          globalMax = localMax;
        }
        if (localMin.val < globalMin.val) { // if local min is smaller than global min, update global min
          globalMin = localMin;
        }
      }
    }
  
  }
  // =================================================================================================


/*
OLD CODE FOR SUM:

#pragma omp parallel for reduction (+:total) private(j)
  for (i = 0; i < size; i++)
    for (j = 0; j < size; j++){
      total += matrix[i][j];
    }
// implicit barrier

*/

  
  end_time = omp_get_wtime(); 


  printf("The total sum is: %d\n", total);
  printf("The maximum value is: %d at [%d, %d]\n", globalMax.val, globalMax.row, globalMax.col);
  printf("The minimum value is: %d at [%d, %d]\n", globalMin.val, globalMin.row, globalMin.col);
  printf("It took %g seconds\n", end_time - start_time);

}

