#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define THREAD_THRESHOLD 50000 

typedef struct {
    int* arr; // Pointer to the array being sorted
    int left; // Starting index of the sub-array
    int right; // Ending index of the sub-array
    int depth; // Remaining levels of thread creation allowed
} qsort_args;

// Standard utility function to swap two integers in memory
void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

// Partitioning exactly as described: pivot is withheld and placed in final position
int partition(int* arr, int left, int right) {
    // Median-of-three for better sublist balance
    // Select median of left, center, and right to avoid O(n^2) on sorted data
    int mid = left + (right - left) / 2;
    if (arr[mid] < arr[left]) swap(&arr[mid], &arr[left]);
    if (arr[right] < arr[left]) swap(&arr[right], &arr[left]);
    if (arr[right] < arr[mid]) swap(&arr[right], &arr[mid]);
 
    // Withhold the pivot by moving it to the 'right' index
    swap(&arr[mid], &arr[right]);
    int pivot = arr[right]; // The value we compare others against
    
    int i = left; // Tracks the boundary for elements smaller than the pivot
    for (int j = left; j < right; j++) {
        if (arr[j] < pivot) {
            swap(&arr[i], &arr[j]);
            i++;
        }
    }
    // Place pivot in its final position
    swap(&arr[i], &arr[right]);
    return i;
}

void* parallel_quicksort(void* args)
{
    // Cast void* back to struct pointer
    qsort_args* qa = (qsort_args*)args;
    // Base case: if sub-array has 0 or 1 element, it is already sorted
    if (qa->left >= qa->right)
    {
        return NULL;
    }
// Divide phase: Get the pivot index
    int pivot_idx = partition(qa->arr, qa->left, qa->right);

// Prepare argument structures for the two sublists (left and right of pivot)
    qsort_args left_args = {qa->arr, qa->left, pivot_idx - 1, qa->depth - 1};
    qsort_args right_args = {qa->arr, pivot_idx + 1, qa->right, qa->depth - 1};
    
// Parallelism Check: Only spawn a thread if depth remains and workload is large enough
    if (qa->depth > 0 && (qa->right - qa->left) > THREAD_THRESHOLD) 
    {
        pthread_t thread; // Thread identifier
        
        // Allocate memory on the heap so the new thread can safely access arguments
        qsort_args* heap_l = malloc(sizeof(qsort_args));
        *heap_l = left_args;
        
        // Create a new thread to handle the left sub-array
        pthread_create(&thread, NULL, parallel_quicksort, heap_l);
        // Current thread handles the right sub-array (recursive call)
        parallel_quicksort(&right_args);
        // Wait for the spawned thread to finish before proceeding
        pthread_join(thread, NULL);
        free(heap_l);
    } else {
        // Sequential fallback: If threshold not met, sort both sides in the current thread
        parallel_quicksort(&left_args);
        parallel_quicksort(&right_args);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Ensure user provided array size and max thread depth
    if (argc != 3) {
        printf("Usage: %s <size> <max_depth>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]); // Convert size string to integer
    int max_depth = atoi(argv[2]); // Convert depth string to integer
    int* arr = malloc(n * sizeof(int)); // Allocate the array

    srand(time(NULL));
    for (int i = 0; i < n; i++) arr[i] = rand() % 100000;

    // Timing logic as per matrixSum.c requirements
    struct timeval start, end;
    gettimeofday(&start, NULL); 

    // Initial call: Start the recursive quicksort from depth 0 to n-1
    qsort_args initial_args = {arr, 0, n - 1, max_depth};
    parallel_quicksort(&initial_args);

    gettimeofday(&end, NULL);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);

    // Verify
    int sorted = 1;
    for (int i = 0; i < n - 1; i++) if (arr[i] > arr[i+1]) sorted = 0;

    printf("\n=== RESULTS ===\n");
    printf("Array sorted correctly: %s\n", sorted ? "YES" : "NO");
    printf("Execution time: %.6f seconds\n", elapsed); 
    printf("Array size: %d\n", n);

    free(arr);
    return 0;
}
