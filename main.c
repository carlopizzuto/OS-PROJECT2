#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

// Global Definitions
#define NUM_TELLERS 3
#define NUM_CUSTOMERS 50


sem_t door_sem;		// Door Semaphore; only 2 customers at a time
sem_t safe_sem;		// Safe Semaphore; only 2 tellers at a time
sem_t manager_sem;	// Manager Semaphore; only 1 teller at a time


// strucutre for thread parameters
typedef struct {
	int id;
} thread_param_t;


// code for teller threads
void *teller_thread(void *arg) {
	thread_param_t *params = (thread_param_t*)arg;
	int tid = params->id;

	printf("Teller %d []: ready to serve\n", tid);

	// TODO: implement teller logic
	
	printf("Teller %d []: leaving for the day\n", tid);
	pthread_exit(NULL);
}

// code for customer threads
void *customer_thread(void *arg) {
	thread_param_t *params = (thread_param_t*)arg;
	int cid = params->id;
	

	printf("Customer %d []: deciding what to do\n", cid);

	// TODO: implement customer logic
	
	printf("Customer %d []: leaving\n", cid);
	pthread_exit(NULL);
}

// main code
int main () {
	// initialize semaphores
	sem_init(&door_sem, 0, 2);
	sem_init(&safe_sem, 0, 2);
	sem_init(&manager_sem, 0, 1);

	// declare arrays for threads
	pthread_t tellers[NUM_TELLERS];
	pthread_t customers[NUM_CUSTOMERS];
	// for thread params
	thread_param_t teller_params[NUM_TELLERS];
	thread_param_t customer_params[NUM_CUSTOMERS];

	// create telle threads
	for (int i=0; i<NUM_TELLERS; i++) {
		teller_params[i].id = i;
		if (pthread_create(&tellers[i], NULL, teller_thread, (void*)&teller_params[i]) != 0) {
			perror("Error creating teller thread");
			exit(EXIT_FAILURE);
		}
	}

	// create customer threads
	for (int i=0; i<NUM_CUSTOMERS; i++) {
		customer_params[i].id = i;
		if (pthread_create(&customers[i], NULL, customer_thread, (void*)&customer_params[i]) != 0) {
			perror("ERROR creating customer thread");
			exit(EXIT_FAILURE);
		}
	}
	
	// wait for customer threads to exit
	for (int i=0; i<NUM_CUSTOMERS; i++) {
		pthread_join(customers[i], NULL);
	}
	
	// wait for teller threads to exit
	for (int i=0; i<NUM_TELLERS; i++) {
		pthread_join(tellers[i], NULL);
	}
	

	// clean up semaphores
	sem_destroy(&door_sem);
	sem_destroy(&safe_sem);
	sem_destroy(&manager_sem);
	
	// terminate
	printf("The bank closes for the day\n");
	return 0;	
}	
