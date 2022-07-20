
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

typedef char L[250];

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("Errore: almeno 2 file\n");
        exit(1);
    }

    int N = argc - 1;

    // creazione array di stringhe + pipe_t ring

    int fcreato = creat("Caico", 0644);
    if (fcreato < 0)
    {
        printf("Errore creazione del file da parte del padre\n");
        exit(2);
    }

    L *tutteLinee = calloc(N, sizeof(L));
    pipe_t *ring = malloc(N * sizeof(pipe_t));
    if (!ring || !tutteLinee)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    int pid, status, ritorno;
    int fd;
    int n;

    for (n = 0; n < N; ++n)
    {
        if (pipe(ring[n]) < 0)
        {
            printf("Errore di generazione del ring\n");
            exit(4);
        }
    }

    for (n = 0; n < N; ++n)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore di generazione figlio n.%d\n", n);
            exit(5);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), n, argv[n + 1]);

            for (int i = 0; i < N; ++i)
            {
                // legge da j
                if (i != n)
                { // allora chiudi il lato lettura
                    close(ring[i][0]);
                }
                // scrive su (j+1)%N
                if ((i != (n + 1) % N))
                { // allora chiudi lato scrittura.
                    close(ring[i][1]);
                }
            }

            // apertura file associato
            if ((fd = open(argv[n + 1], O_RDONLY)) < 0)
            {
                printf("Errore il file %s non è valido!\n", argv[n + 1]);
                exit(-1);
            }
            L linea;
            int pos = 0;

            while (read(fd, linea + pos, 1))
            {
                if (linea[pos] == 10)
                {
                    linea[pos + 1] = 0; // la rendo stringa

                    // devo leggere l'array
                    if (read(ring[n][0], tutteLinee, N * sizeof(L)) == N * sizeof(L))
                    {

                        for (int k = 0; k < pos + 1; k++)
                        {
                            tutteLinee[n][k] = linea[k];
                        }

                        write(ring[(n + 1) % N][1], tutteLinee, N * sizeof(L));
                        ritorno = pos;
                        pos = 0;
                    }

                    if (n == N - 1)
                    {

                        for (int j = 0; j < N; ++j)
                        {
                            write(fcreato, tutteLinee[j], strlen(tutteLinee[j]));
                        }
                    }
                }
                else
                {
                    ++pos;
                }
            }
            exit(ritorno);
        }
    }

    for (int k = 1; k < N; ++k)
    {
        close(ring[k][1]);
        close(ring[k][0]);
    }

    write(ring[0][1], tutteLinee, N * sizeof(L));

    for (int j = 0; j < N; ++j)
    {
        write(fcreato, tutteLinee[j], strlen(tutteLinee[j]));
    }

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