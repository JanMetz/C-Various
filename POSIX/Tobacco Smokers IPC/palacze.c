#include <string.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// defines done for cosmetic and readability reasons
#define Tobacco (long)1
#define Matches (long)2
#define Paper (long)3

#define TobaccoGuy (long)1
#define MatchesGuy (long)2
#define PaperGuy (long)3

#define Send 1
#define Receive 2

// structure used for sending messages about roles to fulfill to each smoker
struct msgStruct{
    long who;
    int what;
};

// structure used for sending resources from one smoker to another
struct resStruct{
    long what;
};

// structure used for visual reduction of the ammount of arguments passed to functions
struct context{
    int *resPrices;
    int *accBalance;
    int msgQueue;
    int resQueue;
    int semId;
};

// structure used for visual reduction of the ammount of arguments passed to functions
struct whoContext{
    long who;
    long resR1;
    long resR2;
    long resS;

    char* what;
};

// function performing an increment operation on chosen semaphore
void semLift(int semId, int semNum){
    struct sembuf semBuf;
    semBuf.sem_num = semNum;
    semBuf.sem_op = 1;
    semBuf.sem_flg = 0;
    if ( semop(semId, &semBuf, 1) == -1){
        fprintf(stderr, "Error while lifting semaphore.\n");
        printf("%s", strerror(errno));
        fflush(stdout);
    }
}

// function performing a decrement operation on chosen semaphore
void semLower(int semId, int semNum){
    struct sembuf semBuf;
    semBuf.sem_num = semNum;
    semBuf.sem_op = -1;
    semBuf.sem_flg = 0;
    if (semop(semId, &semBuf, 1) == -1){
        fprintf(stderr, "Error while lowering semaphore.\n");
        printf("%s", strerror(errno));
        fflush(stdout);
    }
}

// function used to manage the buying process
void agent(struct context con){
    struct msgStruct buyer, provider1, provider2;

    // unpacking universal context
    int msgQueue = con.msgQueue;
    int *accBalance = con.accBalance;
    int *resPrices = con.resPrices;

    int semId = con.semId;

    while(1){
        semLower(semId, 0);
        // picking new prices for each resource
        resPrices[Paper] = (rand() % 4) + 1;
        resPrices[Tobacco] = (rand() % 4) + 1;
        resPrices[Matches] = (rand() % 4) + 1;


        puts("\n**********\n");
        fflush(stdout);


        // checking which one of the smokers can buy resources needed for a ciggarette
       if((resPrices[Matches] + resPrices[Tobacco]) <= accBalance[PaperGuy]){
           // setting functions: buyer gets to receive resources, the providers get to send them
            buyer.who = PaperGuy;
            buyer.what = Receive;

            provider1.who = TobaccoGuy;
            provider1.what = Send;

            provider2.who = MatchesGuy;
            provider2.what = Send;
            puts("Paper guy buys resources");
            fflush(stdout);
        }
        else if((resPrices[Paper] + resPrices[Matches]) <= accBalance[TobaccoGuy]) {
            buyer.who = TobaccoGuy;
            buyer.what = Receive;

            provider1.who = MatchesGuy;
            provider1.what = Send;

            provider2.who = PaperGuy;
            provider2.what = Send;
            puts("Tobacco guy buys resources");
            fflush(stdout);
        }
        else if ((resPrices[Paper] + resPrices[Tobacco]) <= accBalance[MatchesGuy]){
            buyer.who = MatchesGuy;
            buyer.what = Receive;

            provider1.who = TobaccoGuy;
            provider1.what = Send;

            provider2.who = PaperGuy;
            provider2.what = Send;
            puts("Matches guy buys resources");
            fflush(stdout);
        }
        else{
            // stop condition - no more transactions can be done
            puts("No more transactions can be done, none of the smokers has sufficient account balance. Terminating program.");
            exit(0);
        }

        semLift(semId, 0);

        // notyfing buyer and both other smokers about the transaction and specifying the roles they have to fulfill with regard to it
        if ( msgsnd(msgQueue, &buyer, sizeof(buyer), 0) == -1){
            fprintf(stderr, "Error while sending message to buyer.\n");
            printf("%s", strerror(errno));
            fflush(stdout);
        }

        if (msgsnd(msgQueue, &provider1, sizeof(provider1), 0) == -1){
            fprintf(stderr, "Error while sending message to 1st provider.\n");
            printf("%s", strerror(errno));
            fflush(stdout);
        }

        if (msgsnd(msgQueue, &provider2, sizeof(provider2), 0) == -1){
            fprintf(stderr, "Error while sending message to 2nd provider.\n");
            printf("%s", strerror(errno));
            fflush(stdout);
        }

        usleep(300000);
    }
}

// function for representing a smoker
void smoker(struct context con, struct whoContext whoCon){
    // unpacking universal context
    int resQueue = con.resQueue;
    int msgQueue = con.msgQueue;

    int *accBalance = con.accBalance;
    int *resPrices = con.resPrices;

    int semId = con.semId;

    // unpacking smoker-specific context
    long who = whoCon.who;
    long resS = whoCon.resS;
    long resR1 = whoCon.resR1;
    long resR2 = whoCon.resR2;

    char* what = whoCon.what;

    while(1){
        // waiting to receive a message containing role to fulfill
        struct msgStruct msg;
        if (msgrcv(msgQueue, &msg, sizeof(msg), who, 0) == -1){
            fprintf(stderr, "Error while receiving information.\n");
            printf("%s", strerror(errno));
            fflush(stdout);
        }

        if (msg.what == Send){
            // adjusting account balance to the fact that an item has just been sold
            semLower(semId, 0);
            accBalance[who] += resPrices[resS];
            semLift(semId, 0);

            // "putting" resource on the "table" (sending via message queue)
            struct resStruct resSend;
            resSend.what = resS;
            if (msgsnd(resQueue, &resSend, sizeof(resSend), 0) == -1){
                fprintf(stderr, "Error while sending resource %ld.\n", resS);
                printf("%s", strerror(errno));
                fflush(stdout);
            }
            else{
                printf("%s sent\n", what);
                fflush(stdout);
            }

        }

        if (msg.what == Receive){
            // receiving both needed resources ("taking" them from the "table")
            struct resStruct res1Rec;
            if (msgrcv(resQueue, &res1Rec, sizeof(res1Rec), resR1, MSG_NOERROR) == -1){
                fprintf(stderr, "Error while receiving resource %ld.\n", resR1);
                printf("%s", strerror(errno));
                fflush(stdout);
            }

            struct resStruct res2Rec;
            if (msgrcv(resQueue, &res2Rec, sizeof(res2Rec), resR2, MSG_NOERROR) == -1){
                fprintf(stderr, "Error while receiving resource %ld.\n", resR2);
                printf("%s", strerror(errno));
                fflush(stdout);
            }

            puts("Resources received.");
            fflush(stdout);

            // adjusting account balance to the fact that two items have just been purchased
            semLower(semId, 0);
            accBalance[who] -= resPrices[resR1];
            accBalance[who] -= resPrices[resR2];
            semLift(semId, 0);

            printf("%s guy is rolling a cigarette.\n", what);   fflush(stdout);
            printf("%s guy is smoking...\n", what); fflush(stdout);

        }
    }
}

// function inside which happens declaration of needed resources, setting up and running the program
int main()
{
    // getting shared memory used for storing resources price
    int resPricesId;
    if ((resPricesId = shmget(14671, 3 * sizeof(int), IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((resPricesId = shmget(14671, 3 * sizeof(int), IPC_CREAT|0666)) == -1){
            fprintf(stderr, "Error while creating shared memory for resources prices.\n");
            exit(1);
        }
    }

    // attaching shared memory used for storing resources price
    int *resPrices;
    if ((resPrices = (int*) shmat(resPricesId, NULL, 0)) == NULL){
        fprintf(stderr, "Error while attaching shared memory for resources prices.\n");
        exit(1);
    }

    // getting shared memory used for storing account balance of each smoker
    int accBalanceId;
    if ((accBalanceId = shmget(14681, 3 * sizeof(int), IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((accBalanceId = shmget(14681, 3 * sizeof(int), IPC_CREAT|0666)) == -1){
            fprintf(stderr, "Error while creating shared memory for accounts balance.\n");
            exit(1);
        }
    }

    // attaching shared memory used for storing account balance of each smoker
    int *accBalance;
    if ((accBalance = (int*) shmat(accBalanceId, NULL, 0)) == NULL){
        fprintf(stderr, "Error while attaching shared memory for accounts balance.\n");
        exit(1);
    }

    // resetting values on each smoker account
    for (int i = 0; i < 3; i++)
        accBalance[i] = 10;

    // creating a message queue used for communicating roles between smokers
    int msgQueue;
    if ((msgQueue = msgget(41254, IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((msgQueue = msgget(41254, IPC_CREAT|0666)) == -1){
            fprintf (stderr, "Error while creating message queue\n");
            exit (1);
        }
    }

    // creating a message queue used for sending resources
    int resQueue;
    if ((resQueue = msgget(44544, IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((resQueue = msgget(44544, IPC_CREAT|0666)) == -1){
            fprintf (stderr, "Error while creating resource message queue\n");
            exit (1);
        }
    }

    // creating a semaphore array (of size 1), used for mutual exclusion when working with shared memory
    int semId;
	if ((semId = semget(11223, 1, IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((semId = semget(11223, 1, IPC_CREAT|0666)) == -1){
            fprintf(stderr, "Error while creating semaphore");
            exit(1);
        }
	}

    // setting initial value of the semaphore to 1
    if(semctl(semId, 0, SETVAL, (int) 1) == -1){
        fprintf(stderr, "Error while setting value of the semaphore.\n");
        exit(1);
	}


    srand((time_t)time(NULL));

    // filling up the universal context
    struct context con;

    con.accBalance = accBalance;
    con.msgQueue = msgQueue;
    con.resPrices = resPrices;
    con.resQueue = resQueue;
    con.semId = semId;


    // filling up the contexts for each smoker
    struct whoContext papGuy, tobGuy, matGuy;

    papGuy.who = PaperGuy;
    papGuy.resS = Paper;
    papGuy.resR1 = Matches;
    papGuy.resR2 = Tobacco;
    papGuy.what = "Paper";

    matGuy.who = MatchesGuy;
    matGuy.resS = Matches;
    matGuy.resR1 = Paper;
    matGuy.resR2 = Tobacco;
    matGuy.what = "Matches";

    tobGuy.who = TobaccoGuy;
    tobGuy.resS = Tobacco;
    tobGuy.resR1 = Matches;
    tobGuy.resR2 = Paper;
    tobGuy.what = "Tobacco";


    // creating processes
    int process1 = fork();
    int process2 = fork();

    // making each process simulate different personas
    if (process1 > 0 && process2 > 0){
        agent(con);
    }
    else if (process1 > 0 && process2 == 0){
        smoker(con, papGuy);
    }
    else if (process1 == 0 && process2 > 0){
        smoker(con, matGuy);
    }
    else{
        smoker(con, tobGuy);
    }

    // detaching shared memory
    if(shmdt(resPrices) == -1){
        fprintf(stderr, "Error while detaching memory for resources prices.\n");
        exit(1);
	}

    // detaching shared memory
	if(shmdt(accBalance) == -1){
        fprintf(stderr, "Error while detaching memory for account balance.\n");
        exit(1);
	}

    // deleting message queue
    if (msgctl(resQueue, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Error while deleting message queue.\n");
		exit(1);
	}

    // deleting message queue
    if (msgctl(msgQueue, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Error while deleting message queue.\n");
		exit(1);
	}

    // deleting semaphore array
    if (semctl(semId, 1, IPC_RMID) == -1) {
		fprintf(stderr, "Error while deleting semaphore array.\n");
		exit(1);
	}


    return 0;
}
