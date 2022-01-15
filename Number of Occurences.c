#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void wpiszznak (char *napis, int len);
void sortujtablice();
void wypisztablice();

const int K=52;
int tab[2][52]; //przy zmianie K zmieniæ te¿ to

int main()
{
    char napis [129];
    int i=0;

    scanf("%s", &napis);

    for (i=0; i<K; i++)
    {
        tab[1][i]=i;
        tab[0][i]=0;
    }


    wpiszznak(napis, strlen(napis));

    sortujtablice();

    wypisztablice();


}
void sortujtablice()
{
    int j=0, tmp_0 = 0, tmp_1 = 0, czy = 1;

    while (czy == 1)
    {
        czy = 0;
        for (j=0; j<K-1; j++)
        {
            if (tab[0][j]<tab[0][j+1])
            {
                tmp_0 = tab[0][j+1];
                tmp_1 = tab[1][j+1];

                tab[0][j+1] = tab[0][j];
                tab[1][j+1] = tab[1][j];

                tab[0][j] = tmp_0;
                tab[1][j] = tmp_1;

                czy = 1;
            }
        }
    }
}


void wpiszznak (char *napis, int len)
{
    int i=0;

    for (i=0; i<len; i++)
    {
        if (napis[i]<91 && napis[i]>64)
            tab[0][napis[i]-65]++;
        if (napis[i]>96 && napis[i]<123)
            tab[0][napis[i]-71]++;
    }
}

void wypisztablice()
{
    int i=0;

    for (i=0; i<K; i++)
    {
        if (tab[1][i]<26)
            printf("\nNumber of occurences: %d \tLetter: %c", tab[0][i], tab[1][i]+65);
        else
            printf("\nNumber of occurences: %d \tLetter: %c", tab[0][i], tab[1][i]+71);
    }

}
