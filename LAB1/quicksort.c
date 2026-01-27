/*
The quicksort algorithm sorts the list of numbers by first dividing the list into two 
sublists so that all the numbers in one sublist are smaller than all the numbers in the other sublist. 
This is done by selecting one number (called a pivot) against which all other numbers are compared:
the numbers which are less than the pivot are placed in one sublist,
and the numbers which more than the pivot are placed in another sublist. 
The pivot can be either placed in one sublist or withheld and placed in its final position. 
Develop a parallel multithreaded program (in C using Pthreads or in Java) with recursive parallelism that implements 
the quicksort algorithm for sorting an array of n values. Performance evaluation: Measure and print also 
the execution time of your program using the timesLinks to an external site. function or the gettimeofdayLinks 
to an external site. function (see how it is done in matrixSum.c Download matrixSum.c.). 
To calculate the execution time, read the clock after initializing all variables and just before creating the threads.
  Read the clock again after the computation is complete and the worker threads have terminated.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#define THREAD_THRESHOLD 50000  // only create threads for arrays larger than this why? because thread creation has overhead which is what we are trying to avoid in this case

typedef struct {
    int* arr; // array we are sorting
    int left; //starting index
    int right; // ending index
    int depth; //about how many levels of threads are we allowed to create which is more of a safety to not make too many threads
} qsort_args;

// helper method to swap two elements
void swap(int* a, int* b) {
    int t = *a; // t is basically the temporary variable
    *a = *b;
    *b = t;
}

// method that is basically the centre of the sorting algorithm as it basically chooses a pivot around which it moves every larger value to the right and every smaller value to the left
int partition(int* arr, int left, int right) 
{
    int mid = left + (right - left) / 2; // aka pivot
    if (arr[mid] < arr[left]) 
    {
        swap(&arr[mid], &arr[left]);
    }

    if (arr[right] < arr[left]) 
    {
        swap(&arr[right], &arr[left]);
    }

    if (arr[right] < arr[mid]) 
    {
        swap(&arr[right], &arr[mid]);
    }

    int pivot = arr[mid];

    swap(&arr[mid], &arr[right - 1]); // hide the pivot why? because we dont want to compare it with itself
    
    int i = left, j = right - 1;

    while (1) {
        while (arr[++i] < pivot); // find what should be on the right
        while (arr[--j] > pivot); // find what should be on the left
        if (i >= j) break; // if the indices cross it means we are done yippiee
        swap(&arr[i], &arr[j]); // put i and j in their place
    }
    swap(&arr[i], &arr[right - 1]);// put back the pivot
    return i; // this returns where the pivot ended up
}

void* parallel_quicksort(void* args) {
    qsort_args* qa = (qsort_args*)args; // unpack the arguments from the struct defined higher up
    int left = qa->left;
    int right = qa->right;
    int* arr = qa->arr;
    int depth = qa->depth;

// recursion's base case
   if (left >= right) 
   {
        return NULL; 
    }

    
    int pivot_idx = partition(arr, left, right);// partition the array and get the pivot index

    // prepare arguments for the left and right subarrays
    qsort_args left_args = {arr, left, pivot_idx - 1, depth - 1};
    qsort_args right_args = {arr, pivot_idx + 1, right, depth - 1};


    if (depth > 0 && (pivot_idx - left) > THREAD_THRESHOLD) // decide whether to create a new thread for the left subarray based on depth and size
    {
        pthread_t thread;
       
        pthread_create(&thread, NULL, parallel_quicksort, &left_args); //actually start a new thrad on the left
        parallel_quicksort(&right_args);// the same thread works on the right at the same time
  
        pthread_join(thread, NULL);  // wait for the left thread to finish

    } 
    else 
    {
        // just do it one by one without threading
        parallel_quicksort(&left_args);
        parallel_quicksort(&right_args);
    }

    return NULL;
}



void initialize_array(int* arr, int n)
{
    srand(time(NULL));

    for (int i = 0; i < n; i++) 
    {
        arr[i] = rand() % 100000;
    }
}


int is_sorted(int* arr, int n) {
    for (int i = 1; i < n; i++) 
    {
        if (arr[i] < arr[i-1])
        {
            return 0;
        }
    }
    return 1;
}


int main(int argc, char* argv[]) 
{
    if (argc != 3) //basic ch=eck of the input fromt eh user
    {
        printf("Usage: %s <size> <max_depth>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]); //input to no
    int max_depth = atoi(argv[2]);//input to no
    int* arr = malloc(n * sizeof(int)); // allocate memory

    //fill the array w random ints
    srand(time(NULL));
    for (int i = 0; i < n; i++) 
    {
        arr[i] = rand() % 100000;
    }

    struct timeval start, end; // timestamp to measure speed
    gettimeofday(&start, NULL); // read the time before starting the sort

    qsort_args initial_args = {arr, 0, n - 1, max_depth}; // start sorting
    parallel_quicksort(&initial_args);


    gettimeofday(&end, NULL);// read the time after finishing the sort
    
    //calc the diff in ms
    double elapsed_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0;
    elapsed_ms += (double)(end.tv_usec - start.tv_usec) / 1000.0;

 //just loop to make sure
    int sorted = 1;
    for (int i = 0; i < n - 1; i++)
     {
        if (arr[i] > arr[i+1]) 
        {
            sorted = 0;
            break;
        }
    }
//display results yippiee
    printf("\n=== RESULTS ===\n");
    printf("Array sorted correctly: %s\n", sorted ? "YES" : "NO");
    printf("Execution time: %.4f ms\n", elapsed_ms); 
    printf("Array size: %d\n", n);

    free(arr);
    return 0;
}
