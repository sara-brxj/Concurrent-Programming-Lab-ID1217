#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

// Define a constant to prevent over-threading; small arrays are sorted serially
#define THREAD_THRESHOLD 1000 // Adjust based on experimentation

// Function to swap two integer values using pointers
void swap(int* a, int* b) {
    int t = *a; // Store value of a in temporary variable t
    *a = *b; // Assign value of b to a
    *b = t; // Assign value of t (original a) to b
}

int partition(int* arr, int left, int right) {
    int mid = left + (right - left) / 2;
    if (arr[mid] < arr[left]) swap(&arr[mid], &arr[left]);
    if (arr[right] < arr[left]) swap(&arr[right], &arr[left]);
    if (arr[right] < arr[mid]) swap(&arr[right], &arr[mid]);
    
    int pivot = arr[mid]; 
    int i = left - 1;
    int j = right + 1;

    while (1) {
        do { i++; } while (arr[i] < pivot); 
        do { j--; } while (arr[j] > pivot);
        if (i >= j) return j;
        swap(&arr[i], &arr[j]);
    }
}

// Recursive Quicksort function with OpenMP tasking
void quicksort(int* arr, int left, int right) {
    if (left < right) { 
        int p = partition(arr, left, right);
        
        // Optimization: only create tasks if the sub-array is large enough
        if (right - left > THREAD_THRESHOLD) {
            // Create a parallel task for the left partition
            #pragma omp task firstprivate(arr, left, p)
            quicksort(arr, left, p);
// Create a parallel task for the right partition
            #pragma omp task firstprivate(arr, p, right)
            quicksort(arr, p + 1, right);
        } else {
            // Sort small subarrays serially to save CPU overhead
            quicksort(arr, left, p);
            quicksort(arr, p + 1, right);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <size> <num_threads>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    int* arr = malloc(n * sizeof(int));

    srand(time(NULL));
    for (int i = 0; i < n; i++) arr[i] = rand() % 10000;

    omp_set_num_threads(num_threads); // Set the number of threads for OpenMP

    // Measure ONLY the parallel part
    double start_time = omp_get_wtime();
    //Start a parallel region (spawn threads)
    #pragma omp parallel 
    {
     // Ensure only ONE thread starts the recursion
        
        #pragma omp single 
        {
            // Begin the recursive sorting process
            quicksort(arr, 0, n - 1);
        }
    }
    // Implicit barrier: all threads wait here until done
    double end_time = omp_get_wtime();
// Get wall-clock time after sorting
    printf("Threads: %d | Size: %d | Time: %.6f seconds\n", num_threads, n, end_time - start_time);

    free(arr);
    return 0;
}
