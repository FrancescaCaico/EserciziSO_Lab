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
    long int c1; // pari
    long int c2; // dispari
} conta;

int main(int argc, char **argv)
{
    int pid, status, ritorno;
    int fd;
    conta curr;
    char c;

    if (argc < 3)
    {
        printf("Errore mi aspetto un numero di file pari e maggiore o uguale a 2\n");
        exit(1);
    }
    int N = argc - 1;
    if (N % 2)
    {
        printf("Errore: voglio un numero di file pari!\n");
        exit(2);
    }

    pipe_t *FiglioPadre = calloc(N, sizeof(pipe_t));

    if (FiglioPadre == NULL)
    {
        printf("Errore calloc\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FiglioPadre[i]) < 0)
        {
            printf("Impossibile generare la pipe\n");
            exit(4);
        }
    }

    for (int i = 0; i < N; ++i)
    {

        pid = fork();
        if (pid < 0)
        {
            printf("Errore:impossibile generare il figlio n.%d\n", i);
            exit(5);
        }
        if (!pid)
        {
            if (i % 2)
            {
                printf("Sono il figlio %d e PID.%d associato al file %s e cercherò le occorrenze nelle posizioni dispari\n", i, getpid(), argv[i + 1]);
            }
            else
            {
                printf("Sono il figlio %d e PID.%d associato al file %s e cercherò le occorrenze nelle posizioni pari\n", i, getpid(), argv[i + 1]);
            }

            // per tutti i figli devo chiudere allo stesso modo le pipe e aprire i file associati...
            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FiglioPadre[k][1]);
                }
                close(FiglioPadre[k][0]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore --> il file di nome \"%s\" non è valido\n", argv[i + 1]);
                exit(-1);
            }

            // per entrambi devo settare la struttura per la conta...

            curr.c1 = 0;
            curr.c2 = 0;
            int pos = 0; // la uso per la conta delle posizioni.

            // inzio la lettura
            while (read(fd, &c, 1))
            {

                if (i % 2 && pos % 2)
                {
                    if (c % 2)
                    {
                        ++curr.c1;
                    }
                    else
                    {
                        ++curr.c2;
                    }
                }
                else if ((i % 2) == 0 && (pos % 2) == 0)
                {
                    if (c % 2)
                    {
                        ++curr.c2;
                    }
                    else
                    {
                        ++curr.c1;
                    }
                }
                ++pos;
            }
            write(FiglioPadre[i][1], &curr, sizeof(conta));
            if (curr.c1 > curr.c2)
            {
                exit(0);
            }
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FiglioPadre[i][1]);
    }

    for (int i = 0; i < N; ++i)
    {
        if (i % 2 == 0)
        {
            read(FiglioPadre[i][0], &curr, sizeof(conta));
            printf("Sto per mostrare la struttura mandata dal processo pari n.%d associato al file %s\n\t-Conteggi Ascii Pari --> %ld\n\t-Conteggi Ascii Dispari --> %ld\n", i, argv[i + 1], curr.c1, curr.c2);
        }
    }
    for (int i = 0; i < N; ++i)
    {
        if (i % 2)
        {
            read(FiglioPadre[i][0], &curr, sizeof(conta));
            printf("Sto per mostrare la struttura mandata dal processo dispari n.%d associato al file %s\n\t-Conteggi Ascii Dispari --> %ld\n\t-Conteggi Ascii Pari --> %ld\n", i, argv[i + 1], curr.c1, curr.c2);
        }
    }

    for (int k = 0; k < N; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(5);
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