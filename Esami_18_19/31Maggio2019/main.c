
#include <stdio.h>    //stampe
#include <stdlib.h>   //allocazioni
#include <fcntl.h>    //macro per i file: apertura
#include <sys/wait.h> //per le wait dei processi
#include <unistd.h>   //funzioni primitive dei file
#include <stdbool.h>  //booleano per controlli
#include <limits.h>   //se mi servono INT_MAX , MIN, LONG_MAX ECC
#include <time.h>     //in caso serva una funzione random
#include <signal.h>   //per gestire i segnali
#include <string.h>   //per lavorare con stringhe
#include <ctype.h>    //per lavorare con caratteri.

typedef int pipe_t[2];

typedef struct
{
    int c1;       // PID DEL NIPOTE
    char c2[250]; // L'ultima linea della sort
    int c3;       // la lunghezza senza il terminatore.
} S;

int main(int argc, char **argv)
{

    int pid, pidNipote, status, ritorno;
    long int size;

    S curr;

    if (argc < 4)
    {
        printf("Errore numero dei parametri errato\n");
        exit(1);
    }

    int N = argc - 1;
    printf("DEBUG - il numero di figli da generare è %d\n", N);

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (FP == NULL)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            printf("Impossibile generare la pipe\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        if ((pid = fork()) < 0)
        {
            printf("Errore: impossibile generare il figlio n.%i\n", i);
            exit(4);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            pipe_t NF;
            if (pipe(NF) < 0)
            {
                exit(-1);
            }

            if ((pidNipote = fork()) < 0)
            {
                printf("Errore impossibile generare il nipote del figlio con pid.%d\n", getpid());
                exit(-1);
            }
            if (pidNipote == 0)
            {
                printf("Sono il nipote del figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getppid(), i, argv[i + 1]);
                // ridireziono

                close(1);
                dup(NF[1]);
                close(NF[1]);
                close(NF[0]);
                close(FP[i][1]);
                execlp("sort", "sort", "-f", argv[i + 1], (char *)0);
                exit(-1);
            }

            close(NF[1]);

            int j = 0;
            while (read(NF[0], curr.c2 + j, 1))
            {
                if (curr.c2[j] == 10)
                {

                    // ho finito la line e quella corrente è l'ultima
                    curr.c3 = j - 1;
                    curr.c1 = pidNipote;

                    j = 0;
                }
                else
                {
                    ++j;
                }
            }
            write(FP[i][1], &curr, sizeof(curr));

            //  mando la struttura al padre.
            sleep(1);
            if ((pidNipote = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(-1);
            }

            if ((status & 0xff) != 0)
            {
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato in modo anomalo con status :%d\n", pidNipote, getppid(), status & 0xff);
            }
            else
            {
                ritorno = (int)(status >> 8);
                ritorno &= 0xff;
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato correttamente con valore di ritorno:%d\n", pidNipote, getpid(), ritorno);
            }
            exit(curr.c3 + 1);
        }
    }

    for (int k = 0; k < N; ++k)
    {
        close(FP[k][1]);
    }

    for (int k = 0; k < N; ++k)
    {
        read(FP[k][0], &curr, sizeof(S));
        curr.c2[curr.c3 + 1] = 0;
        printf("Ricevuta struttura dal figlio associato al file %s:PID NIPOTE --> %d\nLINEA --> \"%s\"\nLUNGHEZZA --> %d\n", argv[k + 1], curr.c1, curr.c2, curr.c3);
    }
    sleep(1);

    for (int k = 0; k < N; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(8);
        }
        if ((status & 0xff) != 0)
        {
            printf("Il figlio con pid.%d è terminato in modo anomalo con status:%d.\n", pid, status & 0xff);
        }
        else
        {
            ritorno = (int)(status >> 8);
            ritorno &= 0xff;
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%d.\n", pid, ritorno);
        }
    }

    exit(0);
}
