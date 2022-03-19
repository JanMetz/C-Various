#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <string.h>
#include <time.h>

// defines used for controlling programs' enviroment
#define bookshelfCap 1
#define readersNum 5

// defines made strictly for cosmetic reasons
#define reader 0
#define writer 1
#define relax 2


// structure used for sending messages via message queue
// each message represents a book that you put on the bookshelf
// apart from the ID number it also has a list of people who are obligated to read it
struct bookStruct{
    long bookId;
    int *readersIdArr;
};

// function used to preform actions such as lowering or raising (adding or subtracting given value from the semaphore) on the semaphores
void semOp(int semArrID, short semNum, short semOp){
    struct sembuf semBuf;
    semBuf.sem_num = semNum;
    semBuf.sem_op = semOp;
    semBuf.sem_flg = 0;
    if (semop(semArrID, &semBuf, 1) == -1){
        fprintf(stderr, "Error while operating on semaphore %s\n", strerror(errno));
        exit(1);
    }
}

// function simulating actions that happen when you want to read a book
void readABook(int bookshelf, int readerId, int semaphore){
    // checking if there is at least one book on the bookshelf
    struct msqid_ds buf;
    semOp(semaphore, 1, -1);    // lowering the semaphore to get mutual exclusion when entering the critical phase
    msgctl(bookshelf, IPC_STAT, &buf);
    if (buf.msg_qnum < 1){ 
        semOp(semaphore, 1, 1);
        return ;    // if there are no books on the shelf leave the function
    }

    // taking the book off of the bookshelf
    struct bookStruct book;
    if (msgrcv(bookshelf, &book, sizeof(book), 0, 0) == -1){
        fprintf(stderr, "Error while receiving a message %s", strerror(errno));
        semOp(semaphore, 1, 1);
        return ;
    }
    semOp(semaphore, 1, 1); // leaving critical phase thus lifting the semaphore

    int *tmpArr = book.readersIdArr;
    if (tmpArr[readerId] == 1){  // read the book if this reader hasn't read this book yet and he is obligated to
        tmpArr[readerId] = 0;
        printf("Book %ld read by reader %d\n", book.bookId, readerId);
        fflush(stdout);
    }

    // checking if there is still anybody that hasn't, but should have, read it
    int sum = 0;
    for (int i = 0; i < readersNum; i++){   
        sum+= tmpArr[i];
    }

    // putting the book back on the shelf if somebody still needs to read it
    if (sum != 0){  
        struct bookStruct bookS;
        bookS.bookId = book.bookId;
        bookS.readersIdArr = tmpArr;
        if (msgsnd(bookshelf, &bookS, sizeof(bookS), 0) == -1){
            fprintf(stderr, "Error while sending a message %s.\n", strerror(errno));
        }
    }
    else{ // removing the book if everybody who needs to has read it already

        printf("Book %ld removed from the shelf\n", book.bookId);
        fflush(stdout);
    }

    return;
}


// function simulating actions that happen when you want to write a book
void writeABook(int bookshelf, int readerId, int *readersArr, int semaphore){
    // checking if the bookshelf capacity hasn't been exceeded
    struct msqid_ds buf;
    msgctl(bookshelf, IPC_STAT, &buf);
    if (buf.msg_qnum >= bookshelfCap){ 
        return ;
    }

    // creating a book
    struct bookStruct book; 
    semOp(semaphore, 1, -1);    // lowering the semaphore to get mutual exclusion when entering the critical phase  
    book.bookId = (rand() % 100) + 1;
    book.readersIdArr = readersArr;
    semOp(semaphore, 1, 1); // leaving critical phase thus lifting the semaphore

    // putting the book on the bookshelf
    if (msgsnd(bookshelf, &book, sizeof(book), 0) == -1){ // putting the book on the shelf
        fprintf(stderr, "Error while sending a message %s.\n", strerror(errno));
        return ;
    }

    printf("Book %ld put on the bookshelf by writer %d\n", book.bookId, readerId);
    fflush(stdout);

    return;
}


// function to simulate the actions of the bookstore user
void readerWriter(int readerId, int *writerWaiting, int *readersArr, int bookshelf, int semaphore){

    // changing the seed of the pseudo-random generator
    srand((time_t)time(NULL));

    // setting initial state of each bookstore user as reader in the relax phase
    int currState = relax;
    int function = reader;

    while(1){

        // if the user is not in the relax phase thus he wants to use the bookstore
        if (currState != relax) {

            // writer has a priority on using the bookstore - user can try to get to the bookstore only if no writer is currently waiting to use it
            if ((function == reader) && ((*writerWaiting) == 0)){
                // lower the semaphore to make sure that there can be unlimited number of readers OR just one writer in the bookstore at a time
                semOp(semaphore, 0, -1);
                readABook(bookshelf, readerId, semaphore);
                semOp(semaphore, 0, 1);
            }
            else if ((function == writer) && ((*writerWaiting) == 0)){
                // indicate that the writer is waiting
                (*writerWaiting) = 1;

                // lower the semaphore to make sure that there can be unlimited number of readers OR just one writer in the bookstore at a time
                semOp(semaphore, 0, -readersNum);
                readABook(bookshelf, readerId, semaphore);
                writeABook(bookshelf, readerId, readersArr, semaphore);
                semOp(semaphore, 0, readersNum);

                // indicate that the writer is not waiting anymore
                (*writerWaiting) = 0;
            }

            // go to the relax phase
            currState = relax;
        }
        // if the user is in the relax state he can change his role to reader or writer
        else if (currState == relax){
            // picking the new role
            int functionNew = rand() % 2;

            // if the new role is differenet from the old one you need to update the array keeping track of who is a reader currently
            if (function != functionNew){
                semOp(semaphore, 1, -1);    // lowering the semaphore to get mutual exclusion when entering the critical phase
                if (functionNew == reader){
                    readersArr[readerId] = 0;
                }
                else{
                    readersArr[readerId] = 1;
                }
                semOp(semaphore, 1, 1);     // lifting the semaphore after leaving critical phase

                // update current function
                function = functionNew;
            }

            // get to the bookstore
            currState = relax - 1;
        }

        usleep(200000);
    }
}


int main(){
    // getting the message queue used as a bookshelf for every book
    int bookshelf;
    if ((bookshelf = msgget(61564, IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((bookshelf = msgget(61564, IPC_CREAT|0666)) == -1){
            fprintf (stderr, "Error while creating message queue.\n");
            exit (1);
        }
    }

    // getting semaphore array used for mutual exclusion
    int semaphore;
	if ((semaphore = semget(16223, 2, IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((semaphore = semget(16223, 2, IPC_CREAT|0666)) == -1){
            fprintf(stderr, "Error while creating array of semaphores.\n");
            exit(1);
        }
	}

    // setting value of the first semaphore used to limit number of people in the bookstore 
    if(semctl(semaphore, 0, SETVAL, (int) readersNum) == -1){
        fprintf(stderr, "Error while setting value of semaphore for readers.\n");
        exit(1);
	}

    // setting value of the first semaphore used for mutual exclusion when enetering critical phases 
    if(semctl(semaphore, 1, SETVAL, (int) 1) == -1){
        fprintf(stderr, "Error while setting value of semaphore for readers.\n");
        exit(1);
	}

    // getting shared memory used for indicating that a writer is waiting for the access to the bookstore
    int writerWaitingGet;
    if ((writerWaitingGet = shmget(14321, sizeof(int), IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((writerWaitingGet = shmget(14321, sizeof(int), IPC_CREAT|0666)) == -1){
            fprintf(stderr, "Error while creating shared memory.\n");
            exit(1);
        }
    }

    // attaching shared memory used for indicating that a writer is waiting for the access to the bookstore
    int* writerWaiting;
    if ((writerWaiting = (int*) shmat(writerWaitingGet, NULL, 0)) == NULL){
        fprintf(stderr, "Error while attaching shared memory.\n");
        exit(1);
    }

    // setting writerWaiting vairable to 0 because no writer is waiting currently
    (*writerWaiting) = 0;


    // getting shared memory used for keeping track of all of bookstore users roles
    int readersArrGet;
    if ((readersArrGet = shmget(11321, sizeof(int), IPC_CREAT|IPC_EXCL|0666)) == -1){
        if ((readersArrGet = shmget(11321, sizeof(int), IPC_CREAT|0666)) == -1){
            fprintf(stderr, "Error while creating shared memory.\n");
            exit(1);
        }
    }

    // attaching shared memory used for keeping track of all of bookstore users roles
    int* readersArr;
    if ((readersArr = (int*) shmat(readersArrGet, NULL, 0)) == NULL){
        fprintf(stderr, "Error while attaching shared memory.\n");
        exit(1);
    }

    // setting role of every bookstore user to reader (the same value with which all processes are initialized)
    for (int i = 0; i < readersNum; i++){
        readersArr[i] = 1;
    }


    // creating processes
    for (int readerId = 0; readerId < readersNum; readerId++){
        if (fork() == 0){
            readerWriter(readerId, writerWaiting, readersArr, bookshelf, semaphore);
            exit(0);
        }
    }

    // waiting for the processes to end to prevent creation of 'zombie' children
    for (int i = 0; i < readersNum; i++){
        wait(NULL);
    }

    // deleting message queue
    if (msgctl(bookshelf, IPC_RMID, NULL) == -1) {
		fprintf(stderr, "Error while deleting message queue.\n");
		exit(1);
	}

    // detachng shared memory
    if(shmdt(readersArr) == -1){
        fprintf(stderr, "Error while detaching memory for readers array.\n");
        exit(1);
	}

    // detachng shared memory
    if(shmdt(writerWaiting) == -1){
        fprintf(stderr, "Error while detaching memory for waiting writer.\n");
        exit(1);
	}

    // deleting array of semaphores
    if (semctl(semaphore, 1, IPC_RMID) == -1) {
		fprintf(stderr, "Error while deleting semaphore array.\n");
		exit(1);
	}

    return 0;
}

