
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

typedef char L[250];
typedef int pipe_t[2];
int main(int argc, char **argv)
{

    int pid, status, ritorno;
    int fd;
    int fcreato;
    int N;        // per il numero di processi figli
    int nroLinee; // per contenere il numero di linee dei file
    int n;        // indice dei processi figli
    if (argc < 4)
    {
        printf("Errore: mi aspetto almeno 2 file + un numero positivo\n");
        exit(1);
    }

    nroLinee = atoi(argv[argc - 1]);
    if (nroLinee <= 0)
    {
        printf("Errore: il numero di linee %s dev'essere numerico positivo\n", argv[argc - 1]);
        exit(2);
    }

    N = argc - 2;
    printf("DEBUG - Il numero di figli da generare è %d\n", N);

    fcreato = creat("CAICO", 0644);
    if (fcreato < 0)
    {
        printf("Impossibile creare il file CAICO\n");
        exit(3);
    }

    pipe_t *pipeline = malloc(N * sizeof(pipe_t));
    L *tutteLinee = calloc(N, sizeof(L));
    L linea;
    if (pipeline == NULL)
    {
        printf("Errore malloc\n");
        exit(4);
    }

    for (n = 0; n < N; ++n)
    {
        if (pipe(pipeline[n]) < 0)
        {
            printf("Errore. Impossibile generare la pipe\n");
            exit(5);
        }
    }

    for (n = 0; n < N; ++n)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore generazione figlio num.%d\n", n);
            exit(6);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), n, argv[n + 1]);
            for (int k = 0; k < N; ++k)
            {
                if (n != k)
                {
                    close(pipeline[k][1]);
                }
                if ((n == 0) || (k != n - 1))
                {
                    close(pipeline[k][0]);
                }
            }

            // apertura del file associato
            if ((fd = open(argv[n + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file '%s' non è valido\n", argv[n + 1]);
                exit(-1);
            }

            int pos = 0;
            while (read(fd, &linea[pos], 1))
            {

                if (linea[pos] == 10)
                {
                    // fine linea
                    linea[pos + 1] = 0;
                    if (n != 0)
                    {
                        read(pipeline[n - 1][0], tutteLinee, n * sizeof(L));
                    }
                    for (int k = 0; k < pos + 1; ++k)
                    {
                        tutteLinee[n][k] = linea[k];
                    }
                    write(pipeline[n][1], tutteLinee, (n + 1) * sizeof(L));
                    ritorno = pos;
                    pos = 0;
                }
                else
                {
                    ++pos;
                }
            }
            exit(ritorno);
        }
    }

    for (n = 0; n < N; ++n)
    {
        close(pipeline[n][1]);
        if (N - 1 != n)
        {
            close(pipeline[n][0]);
        }
    }

    for (int i = 0; i < nroLinee; i++)
    {
        sleep(1);
        read(pipeline[N - 1][0], tutteLinee, N * sizeof(L));
        // le scrivo sul file.
        for (int k = 0; k < N; ++k)
        {
            printf("Sto per scrivere la linea n.%d del file %s\n", i + 1, argv[k + 1]);
            write(fcreato, tutteLinee[k], strlen(tutteLinee[k]));
        }
    }
    for (int k = 0; k < N; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(7);
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
