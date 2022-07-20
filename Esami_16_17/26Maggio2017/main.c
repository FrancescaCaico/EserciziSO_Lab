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
    long int c1; // valore massimo di occorrenze calcolate dal processo
    int c2;      // indice d'ordine di chi ha calcolato c1
    long int c3; // somma totale delle occorrenze calcolate da tutti i processi
} S;

int main(int argc, char **argv)
{
    int pid, status, ritorno;
    long int occ = 0;
    int fd;
    S curr;

    if (argc < 4)
    {
        printf("Errore parametri errati\n");
        exit(1);
    }

    int N = argc - 2;
    if (argv[argc - 1][1] != 0)
    {
        printf("%s non è un CARATTERE SINGOLO\n", argv[argc - 1]);
        exit(2);
    }
    char Cx = argv[argc - 1][0];
    char c; // per contenere via via i caratteri letti

    printf("DEBUG - Il numero di processi da generare è %d\n", N);

    pipe_t *pipes = calloc(N, sizeof(pipe_t));
    int *pids = calloc(N, sizeof(int));

    if (!pipes || !pids)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(pipes[i]) < 0)
        {
            exit(4);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if ((pids[i] = fork()) < 0)
        {
            exit(5);
        }
        if (pids[i] == 0)
        {
            // codice figlio i-esimo
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(pipes[k][1]);
                }
                if (i == 0 || k != i - 1)
                {
                    close(pipes[k][0]);
                }
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore il file \"%s\" non è valido\n", argv[i + 1]);
                exit(-1);
            }

            while (read(fd, &c, 1))
            {
                if (c == Cx)
                {
                    ++occ;
                }
            }

            if (i != 0)
            {
                // se non sono il primo processo devo fare una comparazione.
                read(pipes[i - 1][0], &curr, sizeof(S));
                if (curr.c1 < occ)
                {
                    // bisogna cambiare la struttura massima
                    curr.c1 = occ;
                    curr.c2 = i;
                    curr.c3 += occ;
                }
            }
            else
            {
                curr.c2 = i;
                curr.c1 = occ;
                curr.c3 = occ;
            }
            write(pipes[i][1], &curr, sizeof(S));
            exit(i);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if (i != N - 1)
        {
            close(pipes[i][0]);
        }
        close(pipes[i][1]);
    }

    read(pipes[N - 1][0], &curr, sizeof(S));
    printf("Ho ricevuto la struttura dal figlio con PID.%d associato al file %s:\n-OCCORRENZE MASSIME --> %ld\n-INDICE D'ORDINE --> %d\n-OCCORRENZE TOTALI TRA I FILE --> %ld\n", pids[curr.c2], argv[curr.c2 + 1], curr.c1, curr.c2, curr.c3);

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