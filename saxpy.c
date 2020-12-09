/**
 * @defgroup   SAXPY saxpy
 *
 * @brief      This file implements an iterative saxpy operation
 * 
 * @param[in] <-p> {vector size} 
 * @param[in] <-s> {seed}
 * @param[in] <-n> {number of threads to create} 
 * @param[in] <-i> {maximum itertions} 
 *
 * @author     Danny Munera
 * @date       2020
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

// Variables to obtain command line parameters
unsigned int seed = 1;
int p = 10000000;
int n_threads = 1;
int max_iters = 1000;

// Variables to perform SAXPY operation
double *X;
double a;
double *Y;
double *Y_avgs;
int i, it;

// Variables to get execution time
struct timeval t_start, t_end;
double exec_time;

//Objeto para definir limites de los hilos
typedef struct _param
{
	int ini;
	int end;
} param_t;

//Semaforo
sem_t mutex;

void *funcionSaxpy(void *arg)
{
	int primero, segundo;
	double total;
	param_t *limites = (param_t *)arg;
	int ini = limites->ini;
	int end = limites->end;

	for (primero = 0; primero < max_iters; primero++)
	{
		total = 0;
		for (segundo = ini; segundo < end; segundo++)
		{
			Y[segundo] = Y[segundo] + a * X[segundo];
			total += Y[segundo];
		}
		sem_wait(&mutex);
		Y_avgs[primero] += total / p;
		sem_post(&mutex);
	}
	return NULL;
};
int main(int argc, char *argv[])
{
	// Getting input values
	int opt;
	while ((opt = getopt(argc, argv, ":p:s:n:i:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			printf("vector size: %s\n", optarg);
			p = strtol(optarg, NULL, 10);
			assert(p > 0 && p <= 2147483647);
			break;
		case 's':
			printf("seed: %s\n", optarg);
			seed = strtol(optarg, NULL, 10);
			break;
		case 'n':
			printf("threads number: %s\n", optarg);
			n_threads = strtol(optarg, NULL, 10);
			break;
		case 'i':
			printf("max. iterations: %s\n", optarg);
			max_iters = strtol(optarg, NULL, 10);
			break;
		case ':':
			printf("option -%c needs a value\n", optopt);
			break;
		case '?':
			fprintf(stderr, "Usage: %s [-p <vector size>] [-s <seed>] [-n <threads number>]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	srand(seed);

	printf("p = %d, seed = %d, n_threads = %d, max_iters = %d\n",
		   p, seed, n_threads, max_iters);

	// initializing data
	X = (double *)malloc(sizeof(double) * p);
	Y = (double *)malloc(sizeof(double) * p);
	Y_avgs = (double *)malloc(sizeof(double) * max_iters);

	for (i = 0; i < p; i++)
	{
		X[i] = (double)rand() / RAND_MAX;
		Y[i] = (double)rand() / RAND_MAX;
	}
	for (i = 0; i < max_iters; i++)
	{
		Y_avgs[i] = 0.0;
	}
	a = (double)rand() / RAND_MAX;

#ifdef DEBUG
	printf("vector X= [ ");
	for (i = 0; i < p - 1; i++)
	{
		printf("%f, ", X[i]);
	}
	printf("%f ]\n", X[p - 1]);

	printf("vector Y= [ ");
	for (i = 0; i < p - 1; i++)
	{
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p - 1]);

	printf("a= %f \n", a);
#endif

	/*
	 *	Function to parallelize 
	 */
	gettimeofday(&t_start, NULL);
	sem_init(&mutex, 0, 1);
	//SAXPY iterative SAXPY mfunction
	param_t **args = (param_t **)malloc(n_threads * sizeof(param_t *));
	int chunk = p / n_threads;
	int rest = p % n_threads;
	int ini = 0;
	int end = 0;
	int aux = n_threads;
	int n;
	while (aux--)
	{
		args[aux] = (param_t *)malloc(sizeof(param_t));
		if (rest)
		{
			n = chunk + 1;
			rest--;
		}
		else
		{
			n = chunk;
		}
		args[aux]->ini = ini;
		end = ini + n - 1;
		args[aux]->end = end;
		ini = end + 1;
	}
	pthread_t *threads = (pthread_t *)malloc(n_threads * sizeof(pthread_t));
	aux = n_threads;
	while (aux--)
	{
		pthread_create(&threads[aux], NULL, funcionSaxpy, args[aux]);
	}
	aux = n_threads;
	while (aux--)
	{
		pthread_join(threads[aux], NULL);
	}
	gettimeofday(&t_end, NULL);

#ifdef DEBUG
	printf("RES: final vector Y= [ ");
	for (i = 0; i < p - 1; i++)
	{
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p - 1]);
#endif

	// Computing execution time
	exec_time = (t_end.tv_sec - t_start.tv_sec) * 1000.0;	 // sec to ms
	exec_time += (t_end.tv_usec - t_start.tv_usec) / 1000.0; // us to ms
	printf("Execution time: %f ms \n", exec_time);
	printf("Last 3 values of Y: %f, %f, %f \n", Y[p - 3], Y[p - 2], Y[p - 1]);
	printf("Last 3 values of Y_avgs: %f, %f, %f \n", Y_avgs[max_iters - 3], Y_avgs[max_iters - 2], Y_avgs[max_iters - 1]);
	return 0;
}