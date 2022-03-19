#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>

// number of philosophers
#define PhilosophersNumber 5

// struct for sending messages
struct messageStruct{
    long who;
    int mealWeight;
};


// function to lift the semaphore (add 1 to its value)
void semLift(int semArrID, short semNum){
    struct sembuf semBuf;
    semBuf.sem_num = semNum;
    semBuf.sem_op = 1;
    semBuf.sem_flg = 0;
    if ( semop(semArrID, &semBuf, 1) == -1){
        perror("Error while lifting.");
        exit(1);
    }
}

// function to lower the semaphore (subtract 1 from its value)
void semLower(int semArrID, short semNum){
    struct sembuf semBuf;
    semBuf.sem_num = semNum;
    semBuf.sem_op = -1;
    semBuf.sem_flg = 0;
    if (semop(semArrID, &semBuf, 1) == -1){
        perror("Error while lowering.");
        exit(1);
    }
}

// function used for philosophers management
void waiter(int msgQID, int *foodEaten){
    // change the pseudo-random machine seed
    srand((size_t)NULL);

    while(1){
        // set weight of the next meal
        int mealWeight = (rand() % 10) + 1;

         // check amounts of food each philosopher has eaten and pick the one who has eaten the least
        int minn = foodEaten[0];
        int minnPos = 0;
        for (int i = 0; i < PhilosophersNumber; i++){
            if(foodEaten[i] < minn ){
                minn = foodEaten[i];
                minnPos = i;
            }
        }

        // wait while there are messages in the queue to make sure that the priority will remain as itended
        static struct msqid_ds buf;
        while (buf.msg_qnum != 0)
            msgctl(msgQID, IPC_STAT, &buf);


        // send message enabling to accquire forks to the philosopher with the least amount eaten
        struct messageStruct msg;
        msg.who = minnPos + 1;
        msg.mealWeight = mealWeight;
        if (msgsnd(msgQID, &msg, sizeof(msg), 0) == -1){
            fprintf(stderr, "Error while sending message.\n");
            exit(1);
        }
    }
}


// function simulating behavoiur of single philosopher
void philosopher(short phil, int msgQID, int semArrID, int* foodEaten){
    while(1){
        // waiting for the message to start trying to get forks
        struct messageStruct msg;
        if (msgrcv(msgQID, &msg, sizeof(msg), phil+1, 0) == -1){
            fprintf(stderr, "Error while receiving message");
            exit(1);
        }

        // acquire forks
        semLower(semArrID, phil);
        semLower(semArrID, (phil+1) % PhilosophersNumber);

        // eat & update the ammount of food eaten
        printf("Philosopher %d is eating\n",phil);fflush(stdout);
        foodEaten[phil]+= msg.mealWeight;

        // put forks down
        semLift(semArrID, (phil+1) % PhilosophersNumber);
        semLift(semArrID, phil);

        // wait for the next round
        sleep(1);
    }
}

int main(){
    // creating array of semaphores used for representing forks
    int semArrID;
	if ((semArrID = semget(74509, PhilosophersNumber, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((semArrID = semget(74509, PhilosophersNumber, IPC_CREAT|0600)) == -1){
            fprintf(stderr, "Error while creating array of semaphores.\n");
            exit(1);
        }
	}

	// setting values for the semaphores in the array used for representing forks to 1 (lifted)
    for (int i = 0; i < PhilosophersNumber; i++){
        if(semctl(semArrID, i, SETVAL, (int) 1) == -1){
            fprintf(stderr, "Error while setting value of semaphore %d.\n", i);
            exit(1);
        }
	}

    // creating message queue used to communicate which philosopher has the highest priority of eating
    int msgQID;
	if ((msgQID = msgget(66976, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((msgQID = msgget(66976, IPC_CREAT|0600)) == -1){
            fprintf (stderr, "Error while creating message queue.\n");
            exit (1);
        }
    }

    // getting shared memory used for keeping track of the ammount of food each philosopher has eaten
    int foodEatenGet;
    if ((foodEatenGet = shmget(12856, PhilosophersNumber * sizeof(int), IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((foodEatenGet = shmget(12856, PhilosophersNumber * sizeof(int), IPC_CREAT|0600)) == -1){
            fprintf (stderr, "Error while creating shared memory.\n");
            exit (1);
        }
    }

    // attaching shared memory used for keeping track of the ammount of food each philosopher has eaten
    int *foodEaten;
    if ((foodEaten = (int*) shmat(foodEatenGet, NULL, 0)) == NULL){
        fprintf (stderr, "Error while attaching shared memory.\n");
        exit (1);
    }

    // setting initial values for array for keeping track of ammount of food each philosopher has eaten
    // since nobody has eaten anything yet the value for each and every philosopher is 0
    for (int i = 0; i < PhilosophersNumber ; i++){
        foodEaten[i] = 0;
	}

	// creating processes for each philosopher
	for(short i = 0 ; i < PhilosophersNumber ; i++){
        if (fork() == 0){
            philosopher(i, msgQID, semArrID, foodEaten);
            exit(0);
        }
	}

    // creating process for the waiter
	if (fork() == 0){
        waiter(msgQID, foodEaten);
        exit(0);
	}

    // waiting for the processes to end
	for(int i = 0 ; i < PhilosophersNumber + 1 ; i++){
		wait(NULL);
	}

    // removing message queue
    if (msgctl(msgQID, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Error while deleting message queue.\n");
		exit(1);
	}

    // detaching shared memory
    if(shmdt(foodEaten) == -1){
        fprintf(stderr, "Error while detaching shared memory.\n");
        exit(1);
	}

    // removing array of semaphores
    if (semctl(semArrID, 1, IPC_RMID) == -1) {
		fprintf(stderr, "Error while deleting semaphore array.\n");
		exit(1);
	}

	return 0;
}

