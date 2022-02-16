#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define TOB 1
#define PAP 2
#define MAT 3

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


struct msgStruct{
    long value;
}elem;

int MSGID;

void *agent(void *arg){
    srand(getpid());

    int paperGuyAcc = 5;
    int matchesGuyAcc = 5;
    int tobaccoGuyAcc = 5;

    int paperPrice = 2;
    int tobaccoPrice = 2;
    int matchesPrice = 2;

    if ((MSGID = msgget(44444, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((MSGID = msgget(44444, IPC_CREAT|0600)) == -1){
            fprintf (stderr, "Error while creating message queue\n");
            exit (1);
        }
    }

    while(1){
        sleep(1);

        pthread_mutex_lock(&table);

        while(agentJob == 0)
            pthread_cond_wait(&agentNeeded, &table);

        agentJob = 0;

        paperPrice = rand() % 6;
        tobaccoPrice = rand() % 6;
        matchesPrice = rand() % 6;


        puts("\n*****Agent starts working*****\n");

        if ((tobaccoPrice + paperPrice) <= matchesGuyAcc){
            puts("Matches guy does transaction");
            matchesGuyAcc -= (tobaccoPrice + paperPrice);
            tobaccoGuyAcc += tobaccoPrice;
            paperGuyAcc += paperPrice;

            paperNeed = 1;
            pthread_cond_signal(&paperNeeded);
            tobaccoNeed = 1;
            pthread_cond_signal(&tobaccoNeeded);
        }
        else if((matchesPrice + paperPrice) <= tobaccoGuyAcc) {
            puts("Tobacco guy does transaction");
            tobaccoGuyAcc -= (matchesPrice + paperPrice);
            matchesGuyAcc += matchesPrice;
            paperGuyAcc += paperPrice;

            paperNeed = 1;
            pthread_cond_signal(&paperNeeded);
            matchesNeed = 1;
            pthread_cond_signal(&matchesNeeded);
        }

        else if((tobaccoPrice + matchesPrice) <= paperGuyAcc){
            puts("Paper guy does transaction");
            paperGuyAcc -= (tobaccoPrice + matchesPrice);
            matchesGuyAcc += matchesPrice;
            tobaccoGuyAcc += tobaccoPrice;

            matchesNeed = 1;
            pthread_cond_signal(&matchesNeeded);
            tobaccoNeed = 1;
            pthread_cond_signal(&tobaccoNeeded);
        }
        else{
            puts("Nobody can buy anything.");
            printf("Matches guy account balance:\t%d\n", matchesGuyAcc);
            printf("Tobacco guy account balance:\t%d\n", tobaccoGuyAcc);
            printf("Paper guy account balance:\t%d\n", paperGuyAcc);
            puts("Current prices:");
            printf("Tobacco:\t%d\n", tobaccoPrice);
            printf("Paper:\t\t%d\n", paperPrice);
            printf("Matches:\t%d\n\n", matchesPrice);

            exit(0);
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
        elem.value = PAP;
        if (msgsnd(MSGID, &elem, sizeof(elem), 0) == -1){
            fprintf(stderr, "Error while sending paper.\n");
            exit(1);
        }
        else
            puts("Paper sent");

    	if (matches == 1){
    	    tobaccoGuyJob = 1;
    	    pthread_cond_signal(&tobaccoGuy);
    	}

    	if (tobacco == 1){
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
        elem.value = MAT;
        if (msgsnd(MSGID, &elem, sizeof(elem), 0) == -1){
            fprintf(stderr, "Error while sending matches.\n");
            exit(1);
        }
        else
            puts("Matches sent");

    	if (paper == 1){
    	    tobaccoGuyJob = 1;
    	    pthread_cond_signal(&tobaccoGuy);
    	}

    	if (tobacco == 1){
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
        elem.value = TOB;
        if (msgsnd(MSGID, &elem, sizeof(elem), 0) == -1){
            fprintf(stderr, "Error while sending tobacco.\n");
            exit(1);
        }
        else
            puts("Tobacco sent");


    	if (matches == 1){
    	    paperGuyJob = 1;
    	    pthread_cond_signal(&paperGuy);
    	}

    	if (paper == 1){
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

        if ((msgrcv(MSGID, &elem, sizeof(elem), TOB, 0) == -1) || (msgrcv(MSGID, &elem, sizeof(elem), MAT, 0) == -1)){
            fprintf(stderr, "Error while receiving tobacco or match");
            exit(1);
        }
        else{
            puts("Tobacco and match received.");
        }

        paperGuyJob = 0;
        tobacco = 0;
        matches = 0;
        puts("Paper guy rolls a cig");
        puts("Smoking...");
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

        if ((msgrcv(MSGID, &elem, sizeof(elem), PAP, 0) == -1) || (msgrcv(MSGID, &elem, sizeof(elem), MAT, 0) == -1)){
            fprintf(stderr, "Error while receiving paper or match");
            exit(1);
        }
        else{
            puts("Paper and match received.");
        }

        tobaccoGuyJob = 0;
        paper = 0;
        matches = 0;
        puts("Tobacco guy rolls a cig");
        puts("Smoking...");
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

        if ((msgrcv(MSGID, &elem, sizeof(elem), TOB, 0) == -1) || (msgrcv(MSGID, &elem, sizeof(elem), PAP, 0) == -1)){
            fprintf(stderr, "Error while receiving tobacco or match");
            exit(1);
        }
        else{
            puts("Paper and tobacco received.");
        }

        matchesGuyJob = 0;
        tobacco = 0;
        paper = 0;
        puts("Matches guy rolls a cig");
        puts("Smoking...");
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
