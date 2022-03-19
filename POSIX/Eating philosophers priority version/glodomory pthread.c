#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// number of philosophers
#define philosophersNumber 5

pthread_mutex_t forks[philosophersNumber];
pthread_cond_t wakeupCall[philosophersNumber];

// struct for keeping track of the amount of food philosophers ate
struct{
    long foodEaten;
}phili[philosophersNumber];

void *waiter(void *arg){
    while(1){
        // check amounts of food each philosopher ate and pick the one who has eaten the least
        int minn = phili[0].foodEaten;
        int minnPos = 0;
        for (int i = 0; i < philosophersNumber; i++){
            if(phili[i].foodEaten < minn ){
                minn = phili[i].foodEaten;
                minnPos = i;
            }
        }

        // send a 'wake up' signal to the one who has eaten the least
        pthread_cond_signal(&wakeupCall[minnPos]);
    }

    return NULL;
}

void *philosopher(void *num){
    int philosopherID = *(int*) num;
    srand((size_t)NULL);
    while(1){
        // acquire forks
        pthread_mutex_lock(&forks[philosopherID]);

        // wait for signal to start eating. Used to ensure priority
        pthread_cond_wait(&wakeupCall[philosopherID], &forks[philosopherID]);
        pthread_mutex_lock(&forks[(philosopherID + 1) % philosophersNumber]);

        // eat

        // communicate the fact of eating
        printf("Philosopher %d is eating\n",philosopherID);
        fflush(stdout);

        // update values in the array
        phili[philosopherID].foodEaten += (rand() % 60) + 1;

        // wait for some time (simulate eating time)
        usleep(30000);

        // put forks down
        pthread_mutex_unlock(&forks[(philosopherID + 1) % philosophersNumber]);
        pthread_mutex_unlock(&forks[philosopherID]);
    }

    return NULL;
}

int main(){
    // initializing mutexes used for representing forks
    for (int i = 0; i < philosophersNumber; i++){
        if(pthread_mutex_init(&forks[i], NULL) == -1){
            fprintf(stderr, "Error while initializing mutex %d", i);
            exit(1);
        }
	}


    // initializing conditionals used for 'waking up' the philosophers to obtain priority
    for (int i = 0; i < philosophersNumber; i++){
        if(pthread_cond_init(&wakeupCall[i], NULL) == -1){
            fprintf(stderr, "Error while initializing cond %d", i);
            exit(1);
        }
	}

   
    // setting initial values for array of philosophers - none of them ate anything yet
    for (int i = 0; i < philosophersNumber ; i++){
        phili[i].foodEaten = 0;
	}


    // creating thread for the waiter
    pthread_t waiterT;
	pthread_create(&waiterT, NULL, waiter, NULL);


    // creating threads for each philosopher
    pthread_t philT[philosophersNumber];
	int numbers[philosophersNumber];
	for(int i = 0 ; i < philosophersNumber ; i++){
        numbers[i] = i;
		pthread_create(&philT[i], NULL, philosopher, (void *) &numbers[i]);
	}


	// joining threads
	for(int i = 0 ; i < philosophersNumber ; i++){
		pthread_join(philT[i], NULL);
	}

	pthread_join(waiterT, NULL);

	return 0;
}

