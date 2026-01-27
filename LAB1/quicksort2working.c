#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define THREAD_THRESHOLD 2000 

typedef struct {
    int* arr;
    int left;
    int right;
    int depth;
} qsort_args;

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

// Improved Partition Logic
int partition(int* arr, int left, int right) {
    int pivot = arr[right]; // Use the rightmost element as pivot for simplicity
    int i = (left - 1);

    for (int j = left; j <= right - 1; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[right]);
    return (i + 1);
}

void* parallel_quicksort(void* args) {
    qsort_args* qa = (qsort_args*)args;
    int left = qa->left;
    int right = qa->right;
    int* arr = qa->arr;
    int depth = qa->depth;

    if (left >= right) return NULL;

    int pivot_idx = partition(arr, left, right);

    // Prepare arguments for sub-problems
    qsort_args left_args = {arr, left, pivot_idx - 1, depth - 1};
    qsort_args right_args = {arr, pivot_idx + 1, right, depth - 1};

    if (depth > 0 && (right - left) > THREAD_THRESHOLD) {
        pthread_t thread;
        
        // We MUST use malloc here for the thread argument 
        // to prevent the stack from being cleared before the thread starts
        qsort_args* heap_args = malloc(sizeof(qsort_args));
        *heap_args = left_args;

        pthread_create(&thread, NULL, parallel_quicksort, heap_args);
        
        // Current thread handles the right side
        parallel_quicksort(&right_args);
        
        pthread_join(thread, NULL);
        free(heap_args);
    } else {
        // Sequential fallback
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
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 100000;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    qsort_args initial_args = {arr, 0, n - 1, max_depth};
    parallel_quicksort(&initial_args);

    gettimeofday(&end, NULL);
    
    double elapsed_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0;
    elapsed_ms += (double)(end.tv_usec - start.tv_usec) / 1000.0;

    int sorted = 1;
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i+1]) {
            sorted = 0;
            break;
        }
    }

    printf("\n=== RESULTS ===\n");
    printf("Array sorted correctly: %s\n", sorted ? "YES" : "NO");
    printf("Execution time: %.4f ms\n", elapsed_ms); 
    printf("Array size: %d\n", n);

    free(arr);
    return 0;
}
