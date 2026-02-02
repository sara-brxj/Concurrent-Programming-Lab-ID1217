#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <time.h>

#define THREAD_THRESHOLD 1000 // Adjust based on experimentation

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
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

void quicksort(int* arr, int left, int right) {
    if (left < right) {
        int p = partition(arr, left, right);

        // Optimization: only create tasks if the sub-array is large enough
        if (right - left > THREAD_THRESHOLD) {
            #pragma omp task firstprivate(arr, left, p)
            quicksort(arr, left, p);

            #pragma omp task firstprivate(arr, p, right)
            quicksort(arr, p + 1, right);
        } else {
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

    omp_set_num_threads(num_threads);

    // Measure ONLY the parallel part
    double start_time = omp_get_wtime();
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            quicksort(arr, 0, n - 1);
        }
    }
    
    double end_time = omp_get_wtime();

    printf("Threads: %d | Size: %d | Time: %.6f seconds\n", num_threads, n, end_time - start_time);

    free(arr);
    return 0;
}
