#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>


pthread_mutex_t table = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t agentNeeded = PTHREAD_COND_INITIALIZER;
pthread_cond_t tobaccoNeeded = PTHREAD_COND_INITIALIZER;
pthread_cond_t matchesNeeded = PTHREAD_COND_INITIALIZER;
pthread_cond_t paperNeeded = PTHREAD_COND_INITIALIZER;

pthread_cond_t tobaccoGuy = PTHREAD_COND_INITIALIZER;
pthread_cond_t matchesGuy = PTHREAD_COND_INITIALIZER;
pthread_cond_t paperGuy = PTHREAD_COND_INITIALIZER;

int tobacco = 0;
int paper = 0;
int matches = 0;

int paperNeed = 0;
int tobaccoNeed = 0;
int matchesNeed = 0;

int agentJob = 1;
int tobaccoGuyJob = 0;
int matchesGuyJob = 0;
int paperGuyJob = 0;


void *agent(void *arg){
    srand(getpid());
    while(1){
        sleep(1);

        pthread_mutex_lock(&table);

        while(agentJob == 0)
            pthread_cond_wait(&agentNeeded, &table);

        agentJob = 0;
        int opt = rand() % 3;

        puts("\n*****Agent starts working*****\n");

        switch(opt){
            case 0: {
                paperNeed = 1;
                pthread_cond_signal(&paperNeeded);
                tobaccoNeed = 1;
                pthread_cond_signal(&tobaccoNeeded);
                break;
            }

            case 1: {
                paperNeed = 1;
                pthread_cond_signal(&paperNeeded);
                matchesNeed = 1;
                pthread_cond_signal(&matchesNeeded);
                break;
            }

            case 2: {
                matchesNeed = 1;
                pthread_cond_signal(&matchesNeeded);
                tobaccoNeed = 1;
                pthread_cond_signal(&tobaccoNeeded);
                break;
            }

            default:{
                fprintf(stderr, "Error in agent()\n");
                exit(1);
            }
        }

        pthread_mutex_unlock(&table);
    }

    return 0;
}

void *paperProvider(void *arg){
    while(1){
    	pthread_mutex_lock(&table);

    	while(paperNeed == 0)
    		pthread_cond_wait(&paperNeeded, &table);

        paperNeed = 0;
        paper = 1;
        puts("Paper provided");

    	if (matches == 1){
            puts("Calling tobacco guy");
    	    tobaccoGuyJob = 1;
    	    pthread_cond_signal(&tobaccoGuy);
    	}

    	if (tobacco == 1){
            puts("Calling matches guy");
    	    matchesGuyJob = 1;
    	    pthread_cond_signal(&matchesGuy);
    	}

    	pthread_mutex_unlock(&table);
    }

    return 0;
}

void *matchesProvider(void *arg){
    while(1){
    	pthread_mutex_lock(&table);

    	while(matchesNeed == 0)
    		pthread_cond_wait(&matchesNeeded, &table);

        matchesNeed = 0;
        matches = 1;
        puts("Matches provided");

    	if (paper == 1){
            puts("Calling tobacco guy");
    	    tobaccoGuyJob = 1;
    	    pthread_cond_signal(&tobaccoGuy);
    	}

    	if (tobacco == 1){
            puts("Calling paper guy");
    	    paperGuyJob = 1;
    	    pthread_cond_signal(&paperGuy);
    	}

    	pthread_mutex_unlock(&table);
    }

    return 0;
}

void *tobaccoProvider(void *arg){
    while(1){
    	pthread_mutex_lock(&table);

    	while(tobaccoNeed == 0)
    		pthread_cond_wait(&tobaccoNeeded, &table);

        tobaccoNeed = 0;
        tobacco = 1;
        puts("Tobacco provided");


    	if (matches == 1){
    	    puts("Calling paper guy");
    	    paperGuyJob = 1;
    	    pthread_cond_signal(&paperGuy);
    	}

    	if (paper == 1){
            puts("Calling matches guy");
    	    matchesGuyJob = 1;
    	    pthread_cond_signal(&matchesGuy);
    	}

    	pthread_mutex_unlock(&table);
    }

    return 0;
}

void *paperRoller(void *arg){

    while(1){
        pthread_mutex_lock(&table);

        while(paperGuyJob == 0)
            pthread_cond_wait(&paperGuy, &table);

        paperGuyJob = 0;
        tobacco = 0;
        matches = 0;
        puts("Paper guy rolls a cig");

        puts("Everybody smokes...");
        puts("Calling agent");
        agentJob = 1;
        pthread_cond_signal(&agentNeeded);

        pthread_mutex_unlock(&table);
    }

    return 0;
}

void *tobaccoRoller(void *arg){

    while(1){
        pthread_mutex_lock(&table);

        while(tobaccoGuyJob == 0)
            pthread_cond_wait(&tobaccoGuy, &table);

        tobaccoGuyJob = 0;
        paper = 0;
        matches = 0;
        puts("Tobacco guy rolls a cig");


        puts("Everybody smokes...");
        puts("Calling agent");
        agentJob = 1;
        pthread_cond_signal(&agentNeeded);

        pthread_mutex_unlock(&table);
    }

    return 0;
}

void *matchesRoller(void *arg){

    while(1){
        pthread_mutex_lock(&table);

        while(matchesGuyJob == 0)
            pthread_cond_wait(&matchesGuy, &table);

        matchesGuyJob = 0;
        paper = 0;
        tobacco = 0;
        puts("Matches guy rolls a cig");


        puts("Everybody smokes...");
        puts("Calling agent");
        agentJob = 1;
        pthread_cond_signal(&agentNeeded);

        pthread_mutex_unlock(&table);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    pthread_t agentNeeded, tobaccoNeeded, paperNeeded, matchesNeeded,
              tobaccoGuy, paperGuy, matchesGuy;


    // "Needed" section
    if (pthread_create(&agentNeeded,NULL,agent,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for agent\n");
        exit (1);
    }

    if (pthread_create(&tobaccoNeeded,NULL,tobaccoProvider,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for tobaccoProvider\n");
        exit (1);
    }

    if (pthread_create(&paperNeeded,NULL,paperProvider,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for paperProvider\n");
        exit (1);
    }

    if (pthread_create(&matchesNeeded,NULL,matchesProvider,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for matchesProvider\n");
        exit (1);
    }


    // "Roller" section
    if (pthread_create(&tobaccoGuy,NULL,tobaccoRoller,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for tobaccoRoller\n");
        exit (1);
    }

    if (pthread_create(&paperGuy,NULL,paperRoller,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for paperRoller\n");
        exit (1);
    }

    if (pthread_create(&matchesGuy,NULL,matchesRoller,NULL) != 0) {
        fprintf (stderr, "Error while creating thread for matchesRoller\n");
        exit (1);
    }


    // Join section
    pthread_join(agentNeeded, NULL);
    pthread_join(tobaccoNeeded, NULL);
    pthread_join(paperNeeded, NULL);
    pthread_join(matchesNeeded, NULL);

    pthread_join(tobaccoGuy, NULL);
    pthread_join(paperGuy, NULL);
    pthread_join(matchesGuy, NULL);


    return 0;
}
