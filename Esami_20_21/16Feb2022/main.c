
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
    int c1;      // PID DEL PROCESSO CHE LE HA CERCATE NEL FILE ASSOCIATO
    long int c2; // occorrenze di C trovate
} S;
int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Errore numero dei parametri errato\n");
        exit(1);
    }

    if (argv[argc - 1][1] != 0)
    {
        printf("Errore %s dev'essere un singolo carattere\n", argv[argc - 1]);
        exit(2);
    }
    char C = argv[argc - 1][0];
    int N = argc - 2;
    printf("Il numero di figli da generare è %d\n", N);

    pipe_t *pipeline = malloc(N * sizeof(pipe_t));
    if (pipeline == NULL)
    {
        printf("ERRORE MALLOC\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(pipeline[i]) < 0)
        {
            printf("errore generazione pipe\n");
            exit(4);
        }
    }

    int pid, status, ritorno;
    int fd;
    char ch;
    S curr;
    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore creazione figlio n.%d\n", i);
            exit(5);
        }
        if (!pid)
        {

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);
            // CHIUSURE IN CASO DI PIPELINE
            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(pipeline[k][1]);
                }
                if ((i == 0) || (k != i - 1))
                {
                    close(pipeline[k][0]);
                }
            }
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore il file non è valido\n");
                exit(-1);
            }

            curr.c1 = getpid();
            curr.c2 = 0;

            while (read(fd, &ch, 1))
            {
                if (ch == C)
                {
                    ++curr.c2;
                }
            }

            printf("Il figlio %d ha trovato %ld occorrenze\n", i, curr.c2);
            S *cur = calloc((i + 1), sizeof(S));
            if (i != 0)
            {
                read(pipeline[i - 1][0], cur, i * sizeof(S));
            }
            cur[i] = curr;
            write(pipeline[i][1], cur, (i + 1) * sizeof(S));
            exit(i);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if (i != N - 1)
        {
            close(pipeline[i][0]);
        }
        close(pipeline[i][1]);
    }

    S *cur = malloc(N * sizeof(S));
    if (!cur)
    {
        printf("Errore malloc\n");
        exit(6);
    }

    read(pipeline[N - 1][0], cur, N * sizeof(S));

    for (int i = 0; i < N; i++)
    {
        printf("Il figlio con indice %d e pid %d associato al file %s ha trovato %ld occorrenze di '%c'\n", i, cur[i].c1, argv[i + 1], cur[i].c2, C);
    }

    for (int i = 0; i < N; ++i)
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