#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#define THREAD_THRESHOLD 50000 
#define SEQUENTIAL_THRESHOLD 32 // New: Below this size, don't even recurse

typedef struct 
{
    int* arr;
    int left;
    int right;
    int depth;
} qsort_args;

void swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

// Standard qsort comparison function
int compare_ints(const void* a, const void* b) 
{
    return (*(int*)a - *(int*)b);
}

int partition(int* arr, int left, int right) 
{
    int mid = left + (right - left) / 2;
    if (arr[mid] < arr[left]) swap(&arr[mid], &arr[left]);
    if (arr[right] < arr[left]) swap(&arr[right], &arr[left]);
    if (arr[right] < arr[mid]) swap(&arr[right], &arr[mid]);

    swap(&arr[mid], &arr[right]);
    int pivot = arr[right];
    
    int i = left;
    for (int j = left; j < right; j++) 
    {
        if (arr[j] < pivot) 
        {
            swap(&arr[i], &arr[j]);
            i++;
        }
    }
    swap(&arr[i], &arr[right]);
    return i;
}

void* parallel_quicksort(void* args) 
{
    qsort_args* qa = (qsort_args*)args;
    int size = qa->right - qa->left + 1;

    if (size <= 1) 
    {
        return NULL;
    }


    if (size < SEQUENTIAL_THRESHOLD) 
    {
        qsort(&qa->arr[qa->left], size, sizeof(int), compare_ints);
        return NULL;
    }

    int pivot_idx = partition(qa->arr, qa->left, qa->right);

    qsort_args left_args = {qa->arr, qa->left, pivot_idx - 1, qa->depth - 1};
    qsort_args right_args = {qa->arr, pivot_idx + 1, qa->right, qa->depth - 1};


    if (qa->depth > 0 && size > THREAD_THRESHOLD) 
    {
        pthread_t thread;
        qsort_args* heap_l = malloc(sizeof(qsort_args));
        *heap_l = left_args;

        pthread_create(&thread, NULL, parallel_quicksort, heap_l);
        
        
        parallel_quicksort(&right_args);

        pthread_join(thread, NULL);
        free(heap_l);
    } 
    else 
    {

        parallel_quicksort(&left_args);
        parallel_quicksort(&right_args);
    }
    return NULL;
}

    int n = atoi(argv[1]);
    int max_depth = atoi(argv[2]); 
    int* arr = malloc(n * sizeof(int)); 

    srand(time(NULL));
    for (int i = 0; i < n; i++) 
        {
            arr[i] = rand() % 100000;
        }

   
    struct timeval start, end;
    gettimeofday(&start, NULL); 

 
    qsort_args initial_args = {arr, 0, n - 1, max_depth};
    parallel_quicksort(&initial_args);

    gettimeofday(&end, NULL);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);


    int sorted = 1;
    for (int i = 0; i < n - 1; i++) 
        {
            if (arr[i] > arr[i+1]) 
            {
                sorted = 0;
            }
        }
    printf("\n=== RESULTS ===\n");
    printf("Array sorted correctly: %s\n", sorted ? "YES" : "NO");
    printf("Execution time: %.6f seconds\n", elapsed); 
    printf("Array size: %d\n", n);

    free(arr);
    return 0;
}
