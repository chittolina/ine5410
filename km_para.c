#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RANDNUM_W 521288629;
#define RANDNUM_Z 362436069;

struct param_thread {
    int startPoint;
    int endPoint;
    int startCentroid;
    int endCentroid;
};

unsigned int randum_w = RANDNUM_W;
unsigned int randum_z = RANDNUM_Z;

void srandnum(int seed) {
    unsigned int w, z;
    w = (seed * 104623) & 0xffffffff;
    randum_w = (w) ? w : RANDNUM_W;
    z = (seed * 48947) & 0xffffffff;
    randum_z = (z) ? z : RANDNUM_Z;
}

unsigned int randnum(void) {
    unsigned int u;
    randum_z = 36969 * (randum_z & 65535) + (randum_z >> 16);
    randum_w = 18000 * (randum_w & 65535) + (randum_w >> 16);
    u = (randum_z << 16) + randum_w;
    return (u);
}

typedef float* vector_t;

int npoints;
int dimension;
int ncentroids;
float mindistance;
int seed;
vector_t *data, *centroids;
int *map;
int *dirty;
int too_far;
int has_changed;
int nthreads;

float v_distance(vector_t a, vector_t b) {
    int i;
    float distance = 0;
    for (i = 0; i < dimension; i++)
        distance +=  pow(a[i] - b[i], 2);
    return sqrt(distance);
}


void* populate(void* param) {
    struct param_thread *p = (struct param_thread *) param;
    int i, j;
    float tmp;
    float distance;

    for (i = p->startPoint; i < p->endPoint; i++) {
        distance = v_distance(centroids[map[i]], data[i]);
        /* Look for closest cluster. */
        for (j = 0; j < ncentroids; j++) {
            /* Point is in this cluster. */
            if (j == map[i]) continue;
            tmp = v_distance(centroids[j], data[i]);
            if (tmp < distance) {
                map[i] = j;
                distance = tmp;
                dirty[j] = 1;
            }
        }
        /* Cluster is too far away. */
        if (distance > mindistance)
            too_far = 1;
    }
}

static void _populate() {
    too_far = 0;
    pthread_t *threads = malloc(sizeof(pthread_t)*nthreads);
    struct param_thread *params = malloc(sizeof(struct param_thread) * nthreads);

    int i;
    for (i = 0; i < nthreads; i++) {
        params[i].startPoint = i*npoints/nthreads;
        params[i].endPoint =(i+1)*npoints/nthreads;
        pthread_create(&threads[i], NULL, populate, (void *) &params[i]);
    }

    for (i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(params);
}

void* compute_centroids(void* param) {
    struct param_thread *p = (struct param_thread *) param;
    int i, j, k;
    int population;

    // Compute means.
    for (i = p->startCentroid; i < p->endCentroid; i++) {
        if (!dirty[i]) continue;
        memset(centroids[i], 0, sizeof(float) * dimension);
        // Compute cluster's mean.
        population = 0;
        for (j = 0; j < npoints; j++) {
            if (map[j] != i) continue;
            for (k = 0; k < dimension; k++)
                centroids[i][k] += data[j][k];
            population++;
        }
        if (population > 1) {
            for (k = 0; k < dimension; k++)
                centroids[i][k] *= 1.0/population;
        }
        has_changed = 1;
    }
}

static void _compute_centroids() {
    has_changed = 0;
    pthread_t *threads = malloc(sizeof(pthread_t)*nthreads);
    struct param_thread *params = malloc(sizeof(struct param_thread) * nthreads);

    int i;
    for (i = 0; i < nthreads; i++) {
        params[i].startCentroid = i*ncentroids/nthreads;
        params[i].endCentroid =(i+1)*ncentroids/nthreads;
        pthread_create(&threads[i], NULL, compute_centroids, (void *) &params[i]);
    }

    int j;
    for (j = 0; j < nthreads; j++) {
        pthread_join(threads[j], NULL);
    }

    memset(dirty, 0, ncentroids * sizeof(int));

    free(threads);
    free(params);
}

int* kmeans(void) {
    int i, j, k;
    too_far = 0;
    has_changed = 0;

    if (!(map  = calloc(npoints, sizeof(int))))
        exit (1);
    if (!(dirty = malloc(ncentroids*sizeof(int))))
        exit (1);
    if (!(centroids = malloc(ncentroids*sizeof(vector_t))))
        exit (1);

    for (i = 0; i < ncentroids; i++)
        centroids[i] = malloc(sizeof(float) * dimension);
    for (i = 0; i < npoints; i++)
        map[i] = -1;
    for (i = 0; i < ncentroids; i++) {
        dirty[i] = 1;
        j = randnum() % npoints;
        for (k = 0; k < dimension; k++)
            centroids[i][k] = data[j][k];
        map[j] = i;
    }
    /* Map unmapped data points. */
    for (i = 0; i < npoints; i++)
        if (map[i] < 0)
            map[i] = randnum() % ncentroids;

    do { /* Cluster data. */
        _populate();
        //_compute_centroids();
        _compute_centroids();
    } while (too_far && has_changed);

    for (i = 0; i < ncentroids; i++)
        free(centroids[i]);
    free(centroids);
    free(dirty);

    return map;
}

int main(int argc, char **argv) {
    int i, j;

    if (argc != 7) {
        printf("Usage: npoints dimension ncentroids mindistance seed threads\n");
        exit (1);
    }

    npoints = atoi(argv[1]);
    dimension = atoi(argv[2]);
    ncentroids = atoi(argv[3]);
    mindistance = atoi(argv[4]);
    seed = atoi(argv[5]);
    nthreads = atoi(argv[6]);

    srandnum(seed);

    if (!(data = malloc(npoints*sizeof(vector_t))))
        exit(1);

    for (i = 0; i < npoints; i++) {
        data[i] = malloc(sizeof(float) * dimension);
        for (j = 0; j < dimension; j++)
            data[i][j] = randnum() & 0xffff;
    }

    map = kmeans();

    for (i = 0; i < ncentroids; i++) {
        printf("\nPartition %d:\n", i);
        for (j = 0; j < npoints; j++)
            if(map[j] == i)
                printf("%d ", j);
    }
    printf("\n");

    free(map);
    for (i = 0; i < npoints; i++)
        free(data[i]);
    free(data);

    return (0);
}
