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
    int pivot = arr[left + (right - left) / 2];
    int i = left - 1;
    int j = right + 1;
    while (1) {
        
        while (arr[i] < pivot)
        {
            i++;
        }
         while (arr[j] > pivot)
         {
              j--; 
         }
        if (i >= j) 
        {
            return j;
        }

        int t = arr[i]; 
        arr[i] = arr[j]; 
        arr[j] = t;
    }
}

void* parallel_quicksort(void* args) {
    qsort_args* qa = (qsort_args*)args;
    int left = qa->left;
    int right = qa->right;
    
    if (left >= right)
    {
        return NULL;
    } 

    int p =partition(qa->arr, left, right);

    qsort_args l_args = {qa->arr, left, p, qa->depth - 1};
    qsort_args r_args = {qa->arr, p + 1, right, qa->depth - 1};

    // Parallel branch: only if depth allows and the task is large enough
    if (qa->depth > 0 && (right - left) > THREAD_THRESHOLD) 
    {
        pthread_t thread;
        qsort_args* heap_l = malloc(sizeof(qsort_args));
        *heap_l = l_args;

        pthread_create(&thread, NULL, parallel_quicksort, heap_l);
        parallel_quicksort(&r_args);
        
        pthread_join(thread, NULL);
        free(heap_l);
    } 
    else
    {
        // Sequential recursion
        parallel_quicksort(&l_args);
        parallel_quicksort(&r_args);
    }

    return NULL;
}

int main(int argc, char* argv[])
 {
    if (argc != 3) 
    {
        printf("Usage: %s <size> <max_depth>\n", argv[0]);
        return 1;
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
    
    double elapsed_ms = (double)(end.tv_sec - start.tv_sec) * 1000.0;
    elapsed_ms += (double)(end.tv_usec - start.tv_usec) / 1000.0;

    int sorted = 1;
    for (int i = 0; i < n - 1; i++) 
    {
        if (arr[i] > arr[i+1])
        {
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
