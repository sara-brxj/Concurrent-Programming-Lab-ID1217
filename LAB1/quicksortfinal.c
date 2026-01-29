#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define THREAD_THRESHOLD 100000 

// this struct baically holds the arguments for quicksort
typedef struct {
    int* arr; 
    int left; 
    int right; 
    int depth; 
} qsort_args;
//helper method
void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

//ADDED FOR VERIFICATION so it s not rlly necessary but it s good for us to check
void print_array(int* arr, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n"); 
    fflush(stdout); //forces the output out of the buffer - PUSHES EVERYTHING OUT ON THE SCREEN
}

int partition(int* arr, int left, int right) {
    // it s called median of 3 operation and it picks the pivot by finding the median of the first, middle, and last elements
    int mid = left + (right - left) / 2;
    if (arr[mid] < arr[left]) swap(&arr[mid], &arr[left]); // & because their contents get swapped
    if (arr[right] < arr[left]) swap(&arr[right], &arr[left]);
    if (arr[right] < arr[mid]) swap(&arr[right], &arr[mid]);
    
    int pivot = arr[mid]; 
    int i = left - 1;
    int j = right + 1;

    // optimised partitioning we use 2 pointers instead of one to minimise switching
    while (1) {
        do { i++; } while (arr[i] < pivot); 
        do { j--; } while (arr[j] > pivot);
        if (i >= j) return j;
        swap(&arr[i], &arr[j]);
    }
}

// the parallelisation
void* parallel_quicksort(void* args) {

    qsort_args* qa = (qsort_args*)args;
    int left = qa->left;
    int right = qa->right;

    // base case to prevent infinite recursion
    if (left >= right) return NULL;
    // patition step
    int p = partition(qa->arr, left, right);

    qsort_args left_args = {qa->arr, left, p, qa->depth - 1};
    qsort_args right_args = {qa->arr, p + 1, right, qa->depth - 1};

    // pretty adaptive since it only creates a thread if it makes sense so like in the case where there is enough depth and enough data
    if (qa->depth > 0 && (right - left) > THREAD_THRESHOLD) {
        pthread_t thread;
    // we used malloc to make sure the thread has its own copy of the arguments that wouldn't get destroyed later in the function
        qsort_args* heap_l = malloc(sizeof(qsort_args));
        *heap_l = left_args;

        // new thread for left branch
        pthread_create(&thread, NULL, parallel_quicksort, heap_l);
        // current thread continues on the same branch
        parallel_quicksort(&right_args);
        
        // wait for the left thread to finish
        pthread_join(thread, NULL);
        free(heap_l);
    } else {
        //if too small/depth is reached we just stick to recursion to avoid the thread creation cost
        parallel_quicksort(&left_args);
        parallel_quicksort(&right_args);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <size> <max_depth>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]); 
    int max_depth = atoi(argv[2]); 
    int* arr = malloc(n * sizeof(int)); 

   srand(time(NULL));
    for (int i = 0; i < n; i++) arr[i] = rand() % 1000;

    printf("Original Array:\n");
    print_array(arr, n);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    qsort_args initial_args = {arr, 0, n - 1, max_depth};
    parallel_quicksort(&initial_args);
    gettimeofday(&end, NULL);
    double elapsed = (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);

    printf("Sorted Array:\n");
    print_array(arr, n);

    int sorted = 1;
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            sorted = 0;
            break;
        }
    }

    printf("\n=== RESULTS ===\n");
    printf("Array sorted correctly: %s\n", sorted ? "YES" : "NO");
    printf("Execution time: %.6f seconds\n", elapsed);
    free(arr);
    return 0;
}
