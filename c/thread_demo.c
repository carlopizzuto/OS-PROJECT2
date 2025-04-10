#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

// Shared Data
int gCount = 0;
sem_t gLock;

void *threadCode(void *tid)
{
    intptr_t id = (intptr_t)tid;

    sem_wait(&gLock);
    printf("Thread %d has count %d\n", id, gCount);
    fflush(stdout);
    gCount++;
    sem_post(&gLock);

    pthread_exit(NULL);
}

int main(int argc, const char** argv)
{
    // Initialize Semaphore
    //  0 — Use multithreading
    //  1 — Initial Value
    // Does not work on MacOs, use different language
    sem_init(&gLock, 0, 1);

    pthread_t threads[5];

    for(intptr_t i=0; i<5; i++)
    {
        int rc = pthread_create(&threads[i],NULL,threadCode, (void*)i);
        if(rc)
        {
            fprintf(stderr, "Unable to spawn thread %d\n", i);
            return 1; //Exit with error
        }
    }

    // wait for threads to exit
    for(int i=0; i<5; i++) {
        pthread_join(threads[i],NULL);
    }

    return 0;
}
