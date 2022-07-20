// CORRETTO RING
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
int main(int argc, char **argv)
{
    int ok;
    int pid, status, ritorno, fd;
    char c;

    if (argc < 3)
    {
        printf("Errore: mi aspetto almeno 2 nomi fi file\n");
        exit(1);
    }

    int Q = argc - 1;
    printf("DEBUG - il numero di figli da generare è %d\n", Q);
    int k = 0;

    pipe_t *ring = malloc(Q * sizeof(pipe_t));
    for (int i = 0; i < Q; ++i)
    {
        if (pipe(ring[i]) < 0)
        {
            printf("Impossibile generare il ring");
            exit(2);
        }
    }

    for (int q = 0; q < Q; ++q)
    {
        if ((pid = fork()) < 0)
        {
            exit(3);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), q, argv[q + 1]);

            // chiusure ring
            for (int i = 0; i < Q; ++i)
            {
                // legge da j
                if (i != q)
                { // allora chiudi il lato lettura
                    close(ring[i][0]);
                }
                // scrive su (j+1)%N
                if ((i != (q + 1) % Q))
                { // allora chiudi lato scrittura.
                    close(ring[i][1]);
                }
            }
            if ((fd = open(argv[q + 1], O_RDONLY)) < 0)
            {
                printf("Errore: impossibile aprire il file associato\n");
                exit(-1);
            }
            sleep(1);
            while (read(fd, &c, 1))
            {
                if (lseek(fd, 0L, SEEK_CUR) - 1 == (q + k * Q))
                {
                    // ho trovato il carattere e lo devo stampare in caso di ok da parte del figlio prima di me
                    if (read(ring[q][0], &ok, sizeof(int)) == sizeof(int))
                    {
                        // scrivo
                        printf("Figlio con indice %d e pid %d ha letto il carattere %c\n", q, getpid(), c);
                        write(ring[(q + 1) % Q][1], &ok, sizeof(int));
                        ++k;
                    }
                }
            }
            exit(q);
        }
    }
    for (int q = 1; q < Q; ++q)
    {
        close(ring[q][0]);
        close(ring[q][1]);
    }

    write(ring[0][1], &ok, sizeof(int));

    for (int k = 0; k < Q; ++k)
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