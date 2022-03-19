#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>

// defines used for regulating ammount of resources available in the program
#define tugNum 20
#define shipsNum 10
#define docksNum 8

// defines done for cosmetic and readability reasons
#define Free 1
#define Parked 2

// table in which weight of each ship is stored
int shipWeight[shipsNum];

// variables for keeping track of number of docks and tugs left
static volatile int availTugs = tugNum;
static volatile int availDocks = docksNum;

// mutex & cond for handling tug boats
pthread_mutex_t tugBoats = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tugFreed = PTHREAD_COND_INITIALIZER;

// mutex & cond for handling dock spaces
pthread_mutex_t docks = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t dockFreed = PTHREAD_COND_INITIALIZER;


// fucntion used for simulation of the ship behaviour
void *ship(void *num){
    int shipNum = *(int*) num;
    int currState = Free;
    while(1){
        
        // if the ship is currently "free" it "wants to" get in the dock
        if (currState == Free){
            // lock the mutex before changing the availDocks global variable
            pthread_mutex_lock(&docks);

            // wait while all the docks are taken
            while(availDocks < 1){
                pthread_cond_wait(&dockFreed, &docks);
            }

            // park in the dock
            availDocks--;
            pthread_mutex_unlock(&docks);
            
            // communicate the fact
            printf("Ship number %d has acquired a dock place.\n", shipNum + 1); fflush(stdout);
            currState = Parked;
        }

        // lock the mutex before changing the availTugs global variable
        pthread_mutex_lock(&tugBoats);

        // wait while there is too small number of tug boats free to handle dockage/ leaving the dock
        while(availTugs < shipWeight[shipNum]){
            pthread_cond_wait(&tugFreed, &tugBoats);
        }

        // take needed ammount of tug boats
        availTugs -= shipWeight[shipNum];
        pthread_mutex_unlock(&tugBoats);

        // communicate the fact of accuiring tug boats
        printf("Currently available tug boats: %d\n", availTugs);
        printf("Ship number %d has acquired %d tug boats.\n", shipNum + 1, shipWeight[shipNum]); fflush(stdout);

    
        
        // if the ship is currently docked it "wants to" get out of the dock
        if (currState == Parked){
            // lock the mutex before changing the availDocks global variable
            pthread_mutex_lock(&docks);

            // leave the dock
            availDocks++;
            pthread_mutex_unlock(&docks);

            // communicate the fact
            pthread_cond_signal(&dockFreed);
            printf("Ship number %d has left a dock place.\n", shipNum + 1); fflush(stdout);
            currState = Free;
        }


        // return the tug boats
        pthread_mutex_lock(&tugBoats);  // lock the mutex before changing the availTugs global variable
        availTugs += shipWeight[shipNum];
        pthread_mutex_unlock(&tugBoats);

        // communicate the fact of leaving tug boats
        printf("Ship number %d has freed %d tug boats.\n", shipNum + 1, shipWeight[shipNum]); fflush(stdout);
        pthread_cond_signal(&tugFreed);

        usleep(200000);
    }

    return NULL;
}


// function in which there are picked weight values for each ship and creating (and joining) threads happens
int main(){

    srand((time_t) NULL);
    
    // pick weight for each ship
    printf("Ships number: %d\n", shipsNum);
    for (int i = 0; i < shipsNum ; i++){
        shipWeight[i] = (rand() % 5) + 1;
        printf("Ship %d weights %d\n", i+1, shipWeight[i]);
	}

    // print info about ammount of available resources
    printf("Available tugs: %d\n", availTugs);
    printf("Available docks: %d\n", availDocks);
	sleep(3);

    pthread_t shipT[shipsNum];

    // create threads for each ship
	int shipsIds[shipsNum];
	for(int i = 0 ; i < shipsNum ; i++){
        shipsIds[i] = i;
		pthread_create(&shipT[i], NULL, ship, (void *) &shipsIds[i]);
	}

    // join threads of each ship
	for(int i = 0 ; i < shipsNum ; i++){
		pthread_join(shipT[i], NULL);
	}

	return 0;
}

