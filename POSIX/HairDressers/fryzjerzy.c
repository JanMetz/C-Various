#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

// defines used strictly for cosmetic reasons
#define AtWork 1
#define AtSalon 2

// defines used for controlling program enviroment
#define SeatsNum 5
#define WaitRoomCap (long)3
#define HaircutCost (long)3

// structure used for sending messages via message queue
// those messages will be like check - they will have the name (id) of the payor, 
// the face value in which he pays and number of coins included in the transaction
struct Client{
    long who;
    long faceValue;
    int coinNum;
};

// function used for lifting the semaphore (adding 1 to its value)
void semapLift(int seats){
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = 1;
    semaphore.sem_flg = 0;
    if (semop(seats, &semaphore, 1) == -1){
        fprintf(stderr, "Error while lowering semaphore.\n");
        exit(1);
    }
}

// function used for lowering the semaphore (subtracting 1 from its value)
void semapLower(int seats){
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = -1;
    semaphore.sem_flg = 0;
    if (semop(seats, &semaphore, 1) == -1){
        fprintf(stderr, "Error while lowering semaphore.\n");
        exit(1);
    }
}

// function used to simulate hairdressers actions
void hairDresser(int waitingRoom, int *cashRegister, int rest, int *seatsTaken, int seats){
    while(1){

        // wait while all the seats are taken
        while((*seatsTaken) >= SeatsNum)
            usleep(10);

        // get the 'check' from the client in line
        struct Client client;
        if (msgrcv(waitingRoom, &client, sizeof(client), 0, 0) == -1){
            fprintf(stderr, "Error while receiving client IDs.\n");
            exit(1);
        }
        else{
            printf("%ld$ in %d coins from client %ld received.\n", client.faceValue*client.coinNum, client.coinNum, client.who);
            fflush(stdout);
        }

        // lower the semaphore to access critical section which is changing value of shared memory variable seatsTaken
        semapLower(seats);
        (*seatsTaken)++;
        // lif the semaphore back up
        semapLift(seats);

        // lower the semaphore to acces cash register (also critial section). Lift the semaphore after finished action
        semapLower(seats);
        if (client.faceValue == 5)
            cashRegister[2]+= client.coinNum;   // increase number of five's in the register
        else if (client.faceValue == 2)
            cashRegister[1]+= client.coinNum;   // increase number of two's in the register
        else
            cashRegister[0]+= client.coinNum;   // increase number of one's in the register
        semapLift(seats);


        printf("Client %ld is getting his hair done.\n", client.who); fflush(stdout);

        // lower the semaphore to access critical section (changing the value of a shared memory variable seatsTaken). Lift the semaphore after finishing
        semapLower(seats);
        (*seatsTaken)--;
        semapLift(seats);

        // calculate the needed rest and send it to client if he payed over the cost of getting his hair cut
        // and therefore needs to get the rest given to him
        long restNeeded = (long)((client.faceValue * client.coinNum) - HaircutCost);
        if (restNeeded > 0){
            int coinNum = 0;
            long faceValue;
            // subtract the needed ammount of coins from the cash register
            do{
                // if he needs rest of value 1 or value of the rest should be 2, but there are no more coins with the face value 2 in the register
                if (((restNeeded == 1) && (cashRegister[0] >= 1)) || ((restNeeded == 2) && (cashRegister[1] == 0) && (cashRegister[0] >= 2))){
                    semapLower(seats);
                    cashRegister[0]-= restNeeded;
                    semapLift(seats);
                    faceValue = 1;
                    coinNum = restNeeded;
                }
                // else, if he needs rest of value 2 and there are enough coins valued as 2 in the register to give it to him
                else if ((restNeeded == 2) && (cashRegister[1] >= 1)){ 
                    semapLower(seats);
                    cashRegister[1]--;
                    semapLift(seats);
                    faceValue = 2;
                    coinNum = 1;
                }
                else{
                    printf("Unable to give rest. Waiting for enough money in the register...\n");
                    usleep(1000);
                }
             } while(coinNum == 0);

            // sending rest via message queue
            struct Client clientsRest;
            clientsRest.who = client.who;
            clientsRest.faceValue = faceValue;
            clientsRest.coinNum = coinNum;

            if (msgsnd(rest, &clientsRest, sizeof(clientsRest), 0) == -1){
                fprintf(stderr, "Error while sending rest to the client.\n");
                exit(1);
            }
            else{
                printf("%ld$ of rest sent to the client %ld in %d coins\n",
                       clientsRest.faceValue * clientsRest.coinNum, clientsRest.who, clientsRest.coinNum);
                fflush(stdout);
            }
        }
        else{   // else - the client does not need any rest given to him

            printf("No rest sent to the client %ld\n", client.who);
            fflush(stdout);
        }

    }
}


// function to simulate the clients behaviour
void client(long clientNum, int faceValue, int waitingRoom, int rest){
    // setting initial values for each client - he starts at work with coins of value 20 in his pocket
    int currState = AtSalon;
    int account = 20;   // you can change that value, however, keep in mind that it needs to be divisible by 5 and 2 at the same time

    // creating message ('check') used for sending the money to the hairdresser
    struct Client sendMoney;
    sendMoney.who = clientNum;
    sendMoney.faceValue = (long) faceValue;     // every client earns only one type of coin (valued 5, 2 or 1)
    sendMoney.coinNum = (int) ceilf((float)HaircutCost/(float)faceValue);

    // calculating the ammount of the rest needed after every payment judging by the face value in which client will earn his money
    int restNeeded = ((sendMoney.faceValue * sendMoney.coinNum) - HaircutCost);

    while(1){
        // going to work
        if (currState == AtWork){
            // if the client is at work he will be working, earning one coin at the time, until he has enough money to go to the salon
            account += faceValue;

            if (account >= HaircutCost)
                currState = AtSalon;
        }
        // going to the salon
        else if (currState == AtSalon){

            // checking if the waiting room is full - if so, returning back to work
            struct msqid_ds buf;
            msgctl(waitingRoom, IPC_STAT, &buf);
            if (buf.msg_qnum > WaitRoomCap){
                currState = AtWork;
                printf("No space in the waiting room, going back to work...\n");
                continue;
            }

            // subtracting the cost of the haircut from the pocket
            account -= HaircutCost;

            // sending 'check' to the salon - equivalent to taking a place in the line
            if (msgsnd(waitingRoom, &sendMoney, sizeof(sendMoney), 0) == -1){
                fprintf(stderr, "Error while sending money.\n");
                exit(1);
            }
            // this part has gotten removed to get better visibility while running the program - you can unlock it if you wish so
            /*else{ 
                 printf("%ld$ given by client %ld\n", sendMoney.faceValue * sendMoney.coinNum, clientNum);
                 fflush(stdout);
            }*/

            // if the client will need the rest to be given to him
            if (restNeeded > 0){
                // waiting for the rest
                struct Client getRest;
                if (msgrcv(rest, &getRest, sizeof(getRest), clientNum, 0) == -1){
                    fprintf(stderr, "Error while receiving rest.\n");
                    exit(1);
                }
                // this part has gotten removed to get better visibility while running the program - you can unlock it if you wish so
                /*else{
                     printf("%ld$ of rest received by client %ld\n", getRest.faceValue * getRest.coinNum, clientNum);
                     fflush(stdout);
                }*/

                // updating ammount of money in the clients' pocket
                account += (getRest.faceValue * getRest.coinNum);
            }

            // going back to work
            currState = AtWork;
        }

        usleep(200000);
    }
}

int main(){
    // getting shared memory used for storing information about current state of the salons seats
    int seatsTakenGet;
    if ((seatsTakenGet = shmget(14691, sizeof(int), IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((seatsTakenGet = shmget(14691, sizeof(int), IPC_CREAT|0600)) == -1){
            fprintf(stderr, "Error while creating shared memory for taken seats.\n");
            exit(1);
        }
    }

    // attaching shared memory used for storing information about current state of the salons seats
    int *seatsTaken;
    if ((seatsTaken = (int*) shmat(seatsTakenGet, NULL, 0)) == NULL){
        fprintf(stderr, "Error while attaching shared memory for taken seats.\n");
        exit(1);
    }

    // since there are no clients in the salon yet the number of taken seats is equal to 0
    (*seatsTaken) = 0;

    
    // getting shared memory used for storing information about number of each type of coin (valued: 5, 2, 1) in the register
    int cashRegGet;
    if ((cashRegGet = shmget(15691, 3 * sizeof(int), IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((cashRegGet = shmget(15691, 3 * sizeof(int), IPC_CREAT|0600)) == -1){
            fprintf(stderr, "Error while creating shared memory for cash register.\n");
            exit(1);
        }
    }

    // attaching shared memory used for storing information about number of each type of coin (valued: 5, 2, 1) in the register
    int *cashRegister;
    if ((cashRegister = (int*) shmat(cashRegGet, NULL, 0)) == NULL){
        fprintf(stderr, "Error while attaching shared memory for cash register.\n");
        exit(1);
    }

    // setting the cash register to have 40 coins of value 1 and 20 of value 2 to provide smooth working conditions for the salon
    cashRegister[0] = 40;
    cashRegister[1] = 20;
    cashRegister[2] = 0;


    // getting message queue used for sending checks (it also doubles as the 'waiting room')
    int waitingRoom;
    if ((waitingRoom = msgget(67544, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((waitingRoom = msgget(67544, IPC_CREAT|0600)) == -1){
            fprintf (stderr, "Error while creating waiting room\n");
            exit (1);
        }
    }

    // getting message queue used for sending the rest back to clients
    int rest;
    if ((rest = msgget(46589, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((rest = msgget(46589, IPC_CREAT|0600)) == -1){
            fprintf (stderr, "Error while creating rest\n");
            exit (1);
        }
    }

    // getting semaphore array (of size 1) used for mutual exclusion of hairdressers when performing critical actions
    int seats;
	if ((seats = semget(11223, 1, IPC_CREAT|IPC_EXCL|0600)) == -1){
        if ((seats = semget(11223, 1, IPC_CREAT|0600)) == -1){
            fprintf(stderr, "Error while creating semaphore for seats");
            exit(1);
        }
	}

    // setting the value of the semaphore to 1 (lifted)
    if(semctl(seats, 0, SETVAL, (int) 1) == -1){
        fprintf(stderr, "Error while setting value of the seats semaphore.\n");
        exit(1);
	}

    // creating processes for each hairdresser
    for (int i = 0; i < 6; i++){
        if (fork() == 0){
            hairDresser(waitingRoom, cashRegister, rest, seatsTaken, seats);
            exit(0);
        }
    }

    // creating processes for each client - keep in mind that the lopp starts from 1 not 0 (in order to make the message queue not go nuts)
    int values[3] = {1,2,5};    // array used to store available face values
    for (long i = 1; i < 11; i++){
        if (fork() == 0){
            srand(time(NULL) ^ (getpid()<<16)); // changing the seed in the pseudo-random generator
            int val = values[rand()%3]; // picking the face value at which the client will earn and pay
            printf("Face value of process %ld is %d\n", i, val);
            client(i, val, waitingRoom, rest);
            exit(0);
        }
    }

    // wait for every 'child' process to end
    for (int i = 0 ; i < 17; i++){
        wait(NULL);
    }


    // detach shared memory
    if(shmdt(seatsTaken) == -1){
        fprintf(stderr, "Error while detaching shared memory.\n");
        exit(1);
	}

    // detach shared memory
	if(shmdt(cashRegister) == -1){
        fprintf(stderr, "Error while detaching shared memory.\n");
        exit(1);
	}

    // delete message queue
    if (msgctl(rest, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Error while deleting message queue.\n");
		exit(1);
	}

    // delete message queue
    if (msgctl(waitingRoom, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Error while deleting message queue.\n");
		exit(1);
	}

    // delete semaphore array
    if (semctl(seats, 1, IPC_RMID) == -1) {
		fprintf(stderr, "Error while deleting semaphore array.\n");
		exit(1);
	}

    return 0;
}
