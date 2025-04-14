#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

// global definitions
#define NUM_TELLERS 3
#define NUM_CUSTOMERS 50

// strucutre for thread parameters
typedef struct {
	intptr_t id;
} thread_param_t;

// semaphore definitions
sem_t door_sem;					// only 2 customers enter the door at a time
sem_t safe_sem;					// only 2 tellers open the safe at a time
sem_t teller_customer_sem[NUM_TELLERS]; 	// only 1 customer per teller
sem_t communication_sem[NUM_TELLERS];
sem_t communication_sem_2[NUM_TELLERS];
sem_t teller_available_sem;		// only 1 customer 

pthread_mutex_t teller_mutex = PTHREAD_MUTEX_INITIALIZER;
int available_tellers[NUM_TELLERS];	// 1 if teller is free; 0 if busy
int assigned_customers[NUM_TELLERS];	// IDs of customers assigned to tellers
					
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int remaining_customers = NUM_CUSTOMERS;

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
	
	// 1. teller lets everyone know it is ready to serve
	printf("Teller %d []: ready to serve\n", tid);
	
	// teller loop
	while (1) {
		// 2. wait for customer to approach
		printf("Teller %d []: waiting for a new customer\n", tid);
		sem_wait(&teller_customer_sem[tid]);
		// once approached
		
		// get the customer's ID
		pthread_mutex_lock(&teller_mutex);
		int cid = assigned_customers[tid];
		pthread_mutex_unlock(&teller_mutex);
		
		// exit condition
		if (cid == -1) {
			break;
		}

		// 3. ask for customer's transaction
		printf("Teller %d [Customer %d]: serving a customer\n", tid, cid);
		printf("Teller %d [Customer %d]: asking for transaction\n", tid, cid);
		
		// signal customer to choose transaction
		sem_post(&communication_sem[tid]);

		// 4. wait for customer's transaction
		sem_wait(&communication_sem_2[tid]);
		// once customer prints transaction

		// get the transaction from the list
		int transaction = -1;
		pthread_mutex_lock(&customer_mutex);
		transaction = customer_transactions[cid];
		pthread_mutex_unlock(&customer_mutex);
		
		// check the transaction type
		if (transaction == 0) { // deposit
			printf("Teller %d [Customer %d]: handling deposit transaction\n", tid, cid);
		} else if(transaction == 1) { // withdrawal
			// 5. if the deposit is a widthdrawal
			printf("Teller %d [Customer %d]: handling withdrawal transaction\n", tid, cid);
			// go to the manager & ask for permission
			printf("Teller %d [Customer %d]: going to the manager\n", tid, cid);
			printf("Teller %d [Customer %d]: asking manager for permission\n", tid, cid);
			usleep(((rand() % 25) + 6) * 1000);
		} else {
			perror("ERROR with teller transaction type");
		}
		
		// 6. go to the safe
		printf("Teller %d [Customer %d]: going to the safe\n", tid, cid);
		// if occupied by 2 tellers, wait
		sem_wait(&safe_sem);
		// once unoccupied, enter
		printf("Teller %d [Customer %d]: entering the safe\n", tid, cid);
		
		// 7. perform the transaction and leave the safe
		usleep(((rand() % 40) + 11) * 1000);
		printf("Teller %d [Customer %d]: leaving the safe\n", tid, cid);
		// free up a spot 
		sem_post(&safe_sem);

		// 8. notify customer of transaction completion
		if (transaction == 0) { 
			printf("Teller %d [Customer %d]: finishing deposit transaction\n", tid, cid);
		} else if(transaction == 1) { 
			printf("Teller %d [Customer %d]: finishing withdrawal transaction\n", tid, cid);
		} else {
			perror("ERROR with teller transaction type");
		}
		sem_post(&communication_sem[tid]);
			
		// 9. wait for customer to leave
		printf("Teller %d [Customer %d]: waiting for customer to leave\n", tid, cid);
		sem_wait(&communication_sem_2[tid]);
		// once customer leaves
		
		// make thread available again
		pthread_mutex_lock(&teller_mutex);
		available_tellers[tid] = 1;
		assigned_customers[tid] = -1;
		pthread_mutex_unlock(&teller_mutex);

		// notify of availability
		sem_post(&teller_available_sem);
	}
		
	printf("Teller %d []: leaving for the day\n", tid);
	pthread_exit(NULL);
}

// code for customer threads
void *customer_thread(void *arg) {
	// extract parameters
	thread_param_t *params = (thread_param_t*)arg;
	intptr_t cid = params->id;
	
	// 1. choose a transaction type
	pthread_mutex_lock(&customer_mutex);
	int transaction_type = rand() % 2;  
	if (transaction_type == 0) { // -> deposit
		printf("Customer %d []: wants to perform a deposit\n", cid);
		customer_transactions[cid] = 0;
	} else if (transaction_type == 1) { // -> withdrawal
		printf("Customer %d []: wants to perform a withdrawal\n", cid);
		customer_transactions[cid] = 1;
	} else {
		perror("ERROR customer transaction type");
		pthread_exit(NULL);
	}
	pthread_mutex_unlock(&customer_mutex);
	fflush(stdout);
	
	// 2. wait for 0 - 100 ms
	usleep(rand() % 101 * 1000);
	
	// 3. go to the bank
	printf("Customer %d []: going to the bank\n", cid);
	// if doot is occupied by 2 customer, wait
	sem_wait(&door_sem);
	// once unoccupied, enter
	printf("Customer %d []: entering the bank\n", cid);
	fflush(stdout);
	sem_post(&door_sem);
	
	// 4. get in line
	printf("Customer %d []: getting in line\n", cid);
	pthread_mutex_lock(&customer_mutex);
	line[line_count] = cid;
	line_count++;
	pthread_mutex_unlock(&customer_mutex);
	
	// wait for an available teller
	sem_wait(&teller_available_sem);
	// once teller is available
	
	// select available teller
	printf("Customer %d []: selecting a teller\n", cid);
	pthread_mutex_lock(&teller_mutex);
	int tid = -1;
	for (int i=0; i<NUM_TELLERS; i++) {
		if (available_tellers[i] == 1) {
			tid = i;
			available_tellers[i] = 0;
			assigned_customers[i] = cid;
			break;
		}
	}
	pthread_mutex_unlock(&teller_mutex);
	if (tid == -1) {
		fprintf(stderr, "Customer %ld: no available teller found (logical error)", cid);
		pthread_exit(NULL);
	}
	
	// 5. intoduce itself to the teller
	printf("Customer %d [Teller %d]: selects teller\n", cid, tid);
	printf("Customer %d [Teller %d]: introduces itself\n", cid, tid);
	// send signal
	sem_post(&teller_customer_sem[tid]);
	
	// 6. wait for teller to ask for transaction
	sem_wait(&communication_sem[tid]);
	// once teller asks

	// 7. tell the teller the transaction
	if (transaction_type == 0) {
		printf("Customer %d [Teller %d]: asks for a deposit transaction\n", cid, tid);
	} else {
		printf("Customer %d [Teller %d]: asks for a withdrawal transaction\n", cid, tid);
	}
	// signal the teller
	sem_post(&communication_sem_2[tid]);
	
	// 8. wait for the teller to finish transaction
	sem_wait(&communication_sem[tid]);
		
	// 9. leave the bank (& simulation)
	sem_wait(&door_sem);
	printf("Customer %d [Teller %d]: leaving the bank\n", cid, tid);
	sem_post(&door_sem);
	// singal teller of exit
	sem_post(&communication_sem_2[tid]);
	pthread_exit(NULL);
}

// main code
int main () {
	// seed the random num generator
	srand(time(NULL));

	// initialize semaphores
	sem_init(&door_sem, 0, 2);
	sem_init(&safe_sem, 0, 2);
	sem_init(&teller_available_sem, 0, NUM_TELLERS);

	// initialize teller-customer sems and list of available tellers
	for (int i=0; i<NUM_TELLERS; i++) {
		available_tellers[i] = 1;			// all tellers are initially available
		assigned_customers[i] = -1;
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
		pthread_mutex_lock(&teller_mutex);
		assigned_customers[i] = -1;
		pthread_mutex_unlock(&teller_mutex);
		
		sem_post(&teller_customer_sem[i]);
	}

	for (int i=0; i<NUM_TELLERS; i++) {
		pthread_join(tellers[i], NULL);
	}
	
	// clean up semaphores
	sem_destroy(&door_sem);
	sem_destroy(&safe_sem);
	sem_destroy(&teller_available_sem);
	for (int i=0; i<NUM_TELLERS; i++) {
		sem_destroy(&teller_customer_sem[i]);
		sem_destroy(&communication_sem[i]);
		sem_destroy(&communication_sem_2[i]);
	}
	// and mutexes
	pthread_mutex_destroy(&teller_mutex);
	pthread_mutex_destroy(&customer_mutex);
	
	// terminate
	printf("The bank closes for the day\n");
	return 0;	
}	
