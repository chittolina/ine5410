/******************************************************************************
 * FILE: arrayloops.c
 * DESCRIPTION:
 *   Example code demonstrating decomposition of array processing by
 *   distributing loop iterations.  A global sum is maintained by a mutex
 *   variable.
 * AUTHOR: Blaise Barney
 * LAST REVISED: 01/29/09
 ******************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int tid;
    int start;
    int end;
} parametros_thread_t;

int nthreads;
int arraysize;
int iterations;

int *a;

void *do_work(void *param) {
    int i, start,mytid, end;
    parametros_thread_t *p = (parametros_thread_t *) param;

    /* Initialize my part of the global array and keep local sum */
    mytid = p->tid;
    start = p->start;
    end = p->end;
    printf ("Thread %d doing iterations %d to %d\n",mytid,start,end);
    for (i = start; i < end ; i++) {
        a[i] += 1;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
    printf("Usage: threads arraysize\n");
    exit (1);
  }

    nthreads = atoi(argv[1]);
    arraysize = atoi(argv[2]);
    
    a = calloc(arraysize,sizeof(int));

    int i;
    
    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t)*nthreads);
    parametros_thread_t *params = (parametros_thread_t *) malloc(sizeof(parametros_thread_t) * nthreads);

    /* Pthreads setup: explicitly create threads.  Pass each thread its loop offset */
    for (i = 0; i < nthreads; i++) {
        params[i].tid = i;
        params[i].start = i*arraysize/nthreads;
        params[i].end =(i+1)*arraysize/nthreads;
        pthread_create(&threads[i], NULL, do_work, (void *) &params[i]);
    }
    /* Wait for all threads to complete then print global sum */
    for (i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }
    /* Print array after operation*/
    for (i = 0;i < arraysize; i++) {
        printf("Check a[%d]= %d\n",i, a[i]);
    }
    /* Clean up and exit */
    free(threads);
    free(params);
    pthread_exit (NULL);
}