#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

// global definitions
#define NUM_TELLERS 3
#define NUM_CUSTOMERS 5

// strucutre for thread parameters
typedef struct {
	intptr_t id;
	intptr_t other_id;
} thread_param_t;

// semaphore definitions
sem_t door_sem;					// only 2 customers enter the door at a time
sem_t safe_sem;					// only 2 tellers open the safe at a time
sem_t manager_sem;				// only 1 teller talks to the manager at a time
sem_t teller_customer_sem[NUM_TELLERS]; 	// only 1 customer per teller
sem_t communication_sem[NUM_TELLERS];
sem_t communication_sem_2[NUM_TELLERS];
sem_t teller_available_sem;		// only 1 customer 

pthread_mutex_t teller_mutex = PTHREAD_MUTEX_INITIALIZER;
int available_tellers[NUM_TELLERS];	// 1 if teller is free; 0 if busy
int assigned_customers[NUM_TELLERS];	// IDs of customers assigned to tellers


pthread_mutex_t customer_mutex = PTHREAD_MUTEX_INITIALIZER;
int customer_transactions[NUM_CUSTOMERS];

int line[NUM_CUSTOMERS];
int line_count = 0;

void pop_first(int list[], int *list_count) {
	if (*list_count == 0) {
		return;
	}

	for (int i=0; i<*list_count-1; i++) {
		list[i] = list[i+1];
	}

	(*list_count)--;
}

// code for teller threads
void *teller_thread(void *arg) {
	thread_param_t *params = (thread_param_t*)arg;
	intptr_t tid = params->id;

	printf("Teller %d []: ready to serve\n", tid);
	printf("Teller %d []: waiting for a customer\n", tid);
	
	// wait for customer to select this teller
	sem_wait(&teller_customer_sem[tid]);
	// once selected
	
	// get the customer's ID
	pthread_mutex_lock(&teller_mutex);
	int cid = assigned_customers[tid];
	pthread_mutex_unlock(&teller_mutex);
	printf("Teller %d [Customer %d]: serving a customer\n", tid, cid);
	printf("Teller %d [Customer %d]: asking for transaction\n", tid, cid);
	
	// signal customer to choose transaction
	sem_post(&communication_sem[tid]);

	// wait for a response
	sem_wait(&communication_sem_2[tid]);
	
	int transaction = -1;
	pthread_mutex_lock(&customer_mutex);
	transaction = customer_transactions[cid];
	pthread_mutex_unlock(&customer_mutex);

	if (transaction == 0) {
		printf("Teller %d [Customer %d]: handling deposit transaction\n", tid, cid);
	} else if(transaction == 1) {
		printf("Teller %d [Customer %d]: handling withdrawal transaction\n", tid, cid);
	} else {
		perror("ERROR with teller transaction type");
	}


	// TODO: implement teller logic
	
	printf("Teller %d []: leaving for the day\n", tid);
	pthread_exit(NULL);
}

// code for customer threads
void *customer_thread(void *arg) {
	// extract parameters
	thread_param_t *params = (thread_param_t*)arg;
	intptr_t cid = params->id;
	
	// choose a transaction type
	int transaction_type = rand() % 2; // 0 -> Deposit, 1 -> Withdrawal 
	if (transaction_type == 0) {
		printf("Customer %d []: wants to perform a deposit\n", cid);
		transaction_type = 0;
	} else {
		printf("Customer %d []: wants to perform a withdrawal\n", cid);
		transaction_type = 1;
	}

	fflush(stdout);
	
	// wait for 0 - 100 ms
	usleep(rand() % 101 * 1000);
	
	// assign transaction type to customer
	pthread_mutex_lock(&customer_mutex);
	customer_transactions[cid] = transaction_type;
	pthread_mutex_unlock(&customer_mutex);

	printf("Customer %d []: going to the bank\n", cid);
	
	// enter ther bank (2 at a time)
	sem_wait(&door_sem);
	printf("Customer %d []: entering the bank\n", cid);
	fflush(stdout);
	sem_post(&door_sem);
	
	// get in line
	printf("Customer %d []: getting in line\n", cid);
	pthread_mutex_lock(&customer_mutex);
	line[line_count] = cid;
	line_count++;
	pthread_mutex_unlock(&customer_mutex);
	
	// wait for an available teller
	sem_wait(&teller_available_sem);
	// teller is available
	
	// select a teller
	printf("Customer %d []: selecting a teller\n", cid);
	pthread_mutex_lock(&teller_mutex);
	int tid = -1;
	for (int i=0; i<NUM_TELLERS; i++) {
		if (available_tellers[i] == 1) {
			tid = i;
			available_tellers[i] = 0;
			assigned_customers[i] = cid;
			// printf("[T %d] INSIDE TELLER %d: %d\n", cid, tid, available_tellers[i]);
			break;
		}
	}
	pthread_mutex_unlock(&teller_mutex);

	if (tid == -1) {
		perror("ERROR with assigning customer to teller");
		pthread_exit(NULL);
	}

	printf("Customer %d [Teller %d]: selects teller\n", cid, tid);
	printf("Customer %d [Teller %d]: introduces itself\n", cid, tid);
	
	// signal teller
	sem_post(&teller_customer_sem[tid]);
	
	// wait for teller to ask for transaction
	sem_wait(&communication_sem[tid]);
	// after teller asks

	// print transaction
	if (transaction_type == 0) {
		printf("Customer %d [Teller %d]: asks for a deposit transaction\n", cid, tid);
	} else {
		printf("Customer %d [Teller %d]: asks for a withdrawal transaction\n", cid, tid);
	}

	sem_post(&communication_sem_2[tid]);

	printf("Customer %d []: leaving\n", cid);
	pthread_exit(NULL);
}

// main code
int main () {
	// seed the random num generator
	srand(time(NULL));

	// initialize semaphores
	sem_init(&door_sem, 0, 2);
	sem_init(&safe_sem, 0, 2);
	sem_init(&manager_sem, 0, 1);
	sem_init(&teller_available_sem, 0, NUM_TELLERS);

	// initialize teller-customer sems and list of available tellers
	for (int i=0; i<NUM_TELLERS; i++) {
		available_tellers[i] = 1;			// all tellers are initially available
		sem_init(&teller_customer_sem[i], 0, 0);	// all tellers wait on a customer
		sem_init(&communication_sem[i], 0, 0);
		sem_init(&communication_sem_2[i], 0, 0);
	}

	// declare arrays for threads
	pthread_t tellers[NUM_TELLERS];
	pthread_t customers[NUM_CUSTOMERS];
	// for thread params
	thread_param_t teller_params[NUM_TELLERS];
	thread_param_t customer_params[NUM_CUSTOMERS];

	// create teller threads
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
	for (int i=0; i<NUM_TELLERS; i++) {
		sem_destroy(&teller_customer_sem[i]);
		sem_destroy(&communication_sem[i]);
	}
	// and mutexes
	pthread_mutex_destroy(&teller_mutex);
	pthread_mutex_destroy(&customer_mutex);
	
	// terminate
	printf("The bank closes for the day\n");
	return 0;	
}	
