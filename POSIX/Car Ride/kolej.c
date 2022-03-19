#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>

// defines used for controling program enviroment
#define carCap 5
#define passengersNum 20

// defines used for cosmetic purposes only
#define Out 1
#define In 2
#define Rest 3

static volatile int placesLeft = carCap;
static volatile int boardingAllowed = 1;

pthread_mutex_t carMtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t carBoarded = PTHREAD_COND_INITIALIZER;
pthread_cond_t rideDone = PTHREAD_COND_INITIALIZER;
pthread_cond_t readyForNewRide = PTHREAD_COND_INITIALIZER;


void *car(void *arg){
    while(1){
        // put visual separator of each round
        puts("*******************");

        // lock the mutex to change boardingAllowed variable
        pthread_mutex_lock(&carMtx);

        // allow boarding
        boardingAllowed = 1;

        // broadcast signal to all passengers that they can start entering the car
        pthread_cond_broadcast(&readyForNewRide);

        // wait while not all the places are taken
        while(placesLeft != 0){
            pthread_cond_wait(&carBoarded, &carMtx);
        }

        // communicate ride begginig
        printf("The car is full! The ride begins!\n"); fflush(stdout);

        // communicate ride end
        printf("The ride is over! All passengers must now leave the car.\n"); fflush(stdout);

        // disallow boarding
        boardingAllowed = 0;

        pthread_mutex_unlock(&carMtx);
   
        // broadcast signal to every passenger that should can now leave the car (if they are inside it in the first place)
        pthread_cond_broadcast(&rideDone);

        usleep(200000);
    }

    return NULL;
}

void *passenger(void* num){
    int passNum = *(int*) num;
    int where = Out;
    while(1){

        // if the passenger is currently out of the car, but wants to get in
        if (where == Out){
            // lock mutex to change placesLeft variable
            pthread_mutex_lock(&carMtx);

            // wait while there are no places left in the car or boarding is not allowed
            while((placesLeft == 0) || (boardingAllowed == 0)){
                pthread_cond_wait(&readyForNewRide, &carMtx);
            }

            // board the car
            placesLeft--;
            pthread_mutex_unlock(&carMtx);

            // communicate fact of boarding
            pthread_cond_signal(&carBoarded);   // to the car function
            printf("Passenger %d has boarded the car.\n", passNum + 1); fflush(stdout); // to the user
            where = In;
        }
        // if the passenger is currently in the car
        else if (where == In){
            // lock mutex to change placesLeft variable
            pthread_mutex_lock(&carMtx);

            // wait for the signal to leave the car
            pthread_cond_wait(&rideDone, &carMtx);

            // leave the car
            placesLeft++;
            pthread_mutex_unlock(&carMtx);

            // communicate fact of leaving
            printf("Passenger %d has left the car.\n", passNum + 1); fflush(stdout);
            where = Rest;
        }
        // if passenger is currently out of the car and he does not want to get in
        else if (where == Rest){
            usleep(100000);
            where = Out;
        }

    }

    return NULL;
}

int main(){
    // creating thread for car
    pthread_t carT;
    pthread_create(&carT, NULL, car, NULL);

    // creating threads for each passenger
	int passengers[passengersNum];
    pthread_t passengersT[passengersNum];
	for(int i = 0 ; i < passengersNum ; i++){
        passengers[i] = i;
		pthread_create(&passengersT[i], NULL, passenger, (void *) &passengers[i]);
	}

    // joining threads
	for(int i = 0 ; i < passengersNum ; i++){
		pthread_join(passengersT[i], NULL);
	}

	pthread_join(carT, NULL);

	return 0;
}

