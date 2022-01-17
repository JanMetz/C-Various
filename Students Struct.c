#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct student
{
    char name [255];
    char surname [255];
    const int index;
    char major [255];
    int current_year;
    char group [255];

} student;

const void printer(const student person);
void reader(student *person);
int compare(const void* a, const void* b);

const int K=15;

int main(){
    student st, s_INF [K], *d_INF;
    int n=0, i=0;

    do{
    printf("Insert number of students you would like to input: ");
    scanf("%d", &n);
    } while (n < 1);

    d_INF = (student*) malloc(n*sizeof(student));

    printf("Please, input your personal data:\n");
    reader(&st);
    printf("Your data:\n");
    printer(st);


    printf("\nPlease, input data about students from your group:\n");
    for (i=0; i<4; i++){
        printf("-------------\n");
        reader(&s_INF[i]);
    }

    printf("\nPlease, input chosen quantity of data about other students:\n");
    for (i=0; i<n; i++){
        printf("-------------\n");
        reader(&d_INF[i]);
    }

    qsort(s_INF, sizeof(s_INF)/sizeof(*s_INF), sizeof(*s_INF), compare);
    qsort(d_INF, sizeof(d_INF)/sizeof(*d_INF), sizeof(*d_INF), compare);


    printf("\nYour group:\n");
    for (i=0; i<K; i++)
        printer(s_INF[i]);



    printf("\nChosen students:\n");
    for (i=0; i<n; i++)
        printer(d_INF[i]);


    free(d_INF);
}

void reader(student *person){

    printf("Name: \n");
    fflush(stdout);
    scanf(" %s", (*person).name);


    printf("Surname: \n");
    fflush(stdout);
    scanf(" %s", (*person).surname);


    printf("Index: \n");
    fflush(stdout);
    scanf("%d", &(*person).index);


    printf("Major: \n");
    fflush(stdout);
    scanf(" %s", (*person).major);


    printf("Current year: \n");
    fflush(stdout);
    scanf(" %d", &(*person).current_year);


    printf("Group: \n");
    fflush(stdout);
    scanf(" %s", (*person).group);

}

const void printer(const student person)
{
    printf("Name: \t\t%s", person.name);

    printf("\nSurname: \t%s", person.surname);

    printf("\nIndex: \t\t%d", person.index);

    printf("\nMajor: \t\t%s", person.major);

    printf("\nCurrent year: \t%d", person.current_year);

    printf("\nGroup: \t\t%s\n", person.group);

    printf("------------\n");
}


int compare(const void* a, const void* b)
{
    student *s1 = (student*)a, *s2 = (student*)b;

    if (strcmp( (*s1).surname, (*s2).surname )== 0)
        return strcmp( (*s1).name, (*s2).name);
    else
        return strcmp( (*s1).surname, (*s2).surname );
}

