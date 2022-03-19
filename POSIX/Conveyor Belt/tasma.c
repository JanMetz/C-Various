#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>


// global variables used for handling the conveyor belt
static volatile int load = 0;
static int belt = 0;


// mutex & cond for handling the belt
static pthread_mutex_t beltMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cnd_empty = PTHREAD_COND_INITIALIZER, cnd_full  = PTHREAD_COND_INITIALIZER;


// structure used for sending messages via message queue
// each message represents one brick, and its only parameter is its weight
struct Brick{
    int weight;
};


// function simulating worker putting the brick on the belt
void *put(void *arg){
    srand((time_t) NULL);
    while(1){
        // lock the mutex before changing the load global variable
        pthread_mutex_lock( &beltMtx );

        // pick weight of the new brick that is about to be layed on the belt
        int brickWeight = (rand() % 2) + 1;

        // wait while the belt is too loaded to put anything on it
        while ( load + brickWeight > 10 ) { 
            pthread_cond_wait( &cnd_empty, &beltMtx );
        }

        // put the brick on the belt
        load += brickWeight;
        pthread_mutex_unlock( &beltMtx );

        // put the brick on the belt and thereofer send info regarding its weight    
        struct Brick brick;
        brick.weight = brickWeight;
        if (msgsnd(belt, &brick, sizeof(brick), 0) == -1){
            fprintf(stderr, "Error while sending a message\n");
            exit(1);
        }
        
        // communicate action
        printf("Brick of weight %d has been put on the conveyor belt\n", brickWeight); fflush(stdout);
        pthread_cond_signal( &cnd_full );

        // print the current load of the belt
        printf("Current load of the conveyor belt: %d\n", load);

        usleep(rand() % 444444); 
    }
}


// function simulating worker getting the brick off of the belt
void *get(void *arg){
    srand((time_t) NULL);
    while(1){
        // lock the mutex before changing the load global variable
        pthread_mutex_lock( &beltMtx );

        // wait while there are no bricks on the belt
        while ( load < 1 ){ 
            pthread_cond_wait( &cnd_full, &beltMtx );
        }

        // read the weight of the brick that is about to be taken off of the belt
        struct Brick brick;
        if (msgrcv(belt, &brick, sizeof(brick), 0, 0) == -1){
            fprintf(stderr, "Error while receiving a message\n");
            exit(1);
        }

        // unload the brick
        load -= brick.weight;
        pthread_mutex_unlock( &beltMtx );

        // communicate action
        printf("Brick of weight %d has been taken off of the conveyor belt\n", brick.weight); fflush(stdout);
        pthread_cond_signal( &cnd_empty );

        usleep(rand() % 555555);   
    }
}


int main(){

    // create message queue which will be used to share information about each bricks' weight
    if ((belt = msgget(12333, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((belt = msgget(12333, IPC_CREAT|0600)) == -1){
            fprintf(stderr, "Error while creating message queue");
            exit(1);
        }
    }

    // create threads for both workers
    pthread_t putT, getT;
    pthread_create(&putT, NULL, put, NULL);
    pthread_create(&getT, NULL, get, NULL);

    // join threads
    pthread_join(putT, NULL);
    pthread_join(getT, NULL);

    return 0;
}
