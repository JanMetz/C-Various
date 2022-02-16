#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define levelsNum 5
#define browniesNum 10
#define decMaxNum 5

pthread_mutex_t level[levelsNum];
pthread_mutex_t base = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t lvl_left[levelsNum];
pthread_cond_t decoAdded = PTHREAD_COND_INITIALIZER;
pthread_cond_t decoTaken = PTHREAD_COND_INITIALIZER;

static volatile int decNum[levelsNum];
static volatile int scaffLoad[levelsNum];
static volatile int availDecNum = 0;

void* claus(void *arg){
    srand((time_t) NULL);
    while(1){

        // drawing number of decorations the Claus will bring next time
        int num = (rand() % 5) + 1;

        pthread_mutex_lock(&base);

        while (availDecNum >= browniesNum)  // wait while the number of decorations available is bigger than number of brownies
            pthread_cond_wait(&decoTaken, &base);

        // updating number of available decorations, signaling action
        availDecNum  += num;

        // printf("***%d decorations added***\n", num);

        pthread_cond_signal(&decoAdded);
        pthread_mutex_unlock(&base);

        usleep(300000);

    }
}

void* brownie(void *number){
    int num = *(int*) number;
    while(1){
        usleep(300000);
        // checking if there is at least one decoration available to be taken
        pthread_mutex_lock(&base);
        while(availDecNum  < 1)
            pthread_cond_wait(&decoAdded, &base);

        availDecNum --;
        pthread_cond_signal(&decoTaken);

        // printf("Decoration taken by brownie number %d\n", num); fflush(stdout);
        pthread_mutex_unlock(&base);


        int lvl = 0;
        // putting decoration on adequate level - ascending if necessary
        while (1){

            pthread_mutex_lock(&level[lvl]);
            if (decNum[lvl] < decMaxNum - lvl){ // if it is possible to put a decoration on current level
                // putting a decoration
                printf("Decoration %d on level %d put by brownie number %d\n", decNum[lvl]+1, lvl, num); fflush(stdout);
                decNum[lvl]++;
                pthread_mutex_unlock(&level[lvl]);
                break;
            }
            else{   // ascend while each level is already full of decorations
                pthread_mutex_unlock(&level[lvl]);

                if (lvl < levelsNum - 1){   // checking if it is possible to climb to a higher level
                    // printf("Putting decoration on level %d impossible. Climbing...\n", lvl); fflush(stdout);
                    pthread_mutex_lock(&level[lvl+1]);

                    // waiting till there is a place on the scaffolding
                    while(scaffLoad[lvl+1] >= levelsNum - (lvl+1)) // one place on scaffolding left for any brownie that wants to descend to prevent locking
                        pthread_cond_wait(&lvl_left[lvl+1], &level[lvl+1]);


                    // leaving current level
                    scaffLoad[lvl]--;
                    pthread_cond_signal(&lvl_left[lvl]);

                     // getting to the higher level
                    lvl++;
                    scaffLoad[lvl]++;

                    pthread_mutex_unlock(&level[lvl]);

                }
                else{ // climbing not possible and current level is already full of decorations - stop condition
                    puts("Whole tree has been decorated. Terminating program..."); fflush(stdout);
                    exit(0);
                }
            }
        }

        // descending
        while (lvl > 0){
            pthread_mutex_lock(&level[lvl-1]);

            // waiting till there is a place on the scaffolding
            while(scaffLoad[lvl-1] >= levelsNum - (lvl-1) + 1)
                pthread_cond_wait(&lvl_left[lvl-1], &level[lvl-1]);


            // leaving current level
            scaffLoad[lvl]--;
            pthread_cond_signal(&lvl_left[lvl]);

            // descending to the lower level
            lvl--;
            scaffLoad[lvl]++;

            pthread_mutex_unlock(&level[lvl]);
        }

    }
}

int main(){

    // setting initial values for arrays - no decorations on the tree and no brownies on the scaffolding yet
    for (int i = 0; i < levelsNum ; i++){
        decNum[i] = 0;
        scaffLoad[i] = 0;
	}

    // initializing mutexes and conds used for representing levels and signaling leaving level
    for (int i = 0; i < levelsNum; i++){
        if(pthread_mutex_init(&level[i], NULL) == -1){
            fprintf(stderr, "Error while initializing mutex %d", i);
            exit(1);
        }

        if(pthread_cond_init(&lvl_left[i], NULL) == -1){
            fprintf(stderr, "Error while initializing conditional %d", i);
            exit(1);
        }
	}



	// creating threads for Claus and brownies
    pthread_t clausT, browniesT[browniesNum];

	pthread_create(&clausT, NULL, claus, NULL);

	int arr[browniesNum];
	for(int i = 0 ; i < browniesNum ; i++){
        arr[i] = i;
		pthread_create(&browniesT[i], NULL, brownie, (void *) &arr[i]);
	}

	// joining threads
	for(int i = 0 ; i < browniesNum ; i++){
		pthread_join(browniesT[i], NULL);
	}

	pthread_join(clausT, NULL);

	return 0;
}

