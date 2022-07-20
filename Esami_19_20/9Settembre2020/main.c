
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
    int pid, status, ritorno;
    int fd;
    char c;
    int occ = 0;

    if (argc < 5)
    {
        printf("Errore: numero dei parametri errato\n");
        exit(1);
    }

    int Q = argc - 3;
    printf("Il numero di processi da creare è %d\n", Q);

    int L = atoi(argv[2]);
    if (L <= 0)
    {
        printf("Errore L=%s dev'essere numerico positivo\n", argv[3]);
        exit(2);
    }

    int ok;

    pipe_t *PadreFiglio = malloc(Q * sizeof(pipe_t));

    for (int q = 0; q < Q; ++q)
    {
        if (pipe(PadreFiglio[q]) < 0)
        {
            printf("Impossibile generare la pipee\n");
            exit(3);
        }
    }

    for (int q = 0; q < Q; ++q)
    {
        if ((pid = fork()) < 0)
        {
            printf("Impossibile generare figlio n.%d\n", q);
            exit(4);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s' per il carattere %s\n", getpid(), q, argv[1], argv[q + 3]);
            for (int i = 0; i < Q; ++i)
            {
                if (i != q)
                {
                    close(PadreFiglio[i][0]);
                }
                close(PadreFiglio[i][1]);
            }

            // apro il file associato
            if ((fd = open(argv[1], O_RDONLY)) < 0)
            {
                printf("Errore: impossibile aprire il file associato\n");
                exit(-1);
            }

            char Cz = argv[q + 3][0];
            while (read(fd, &c, 1))
            {
                if (c == 10)
                {
                    // fine linea attendo l'ok dal padre per la stampa.
                    if (read(PadreFiglio[q][0], &ok, sizeof(ok)) == sizeof(ok))
                    {
                        printf("%d occorrenze del carattere '%c'\n", occ, Cz);
                    }
                    ritorno = occ;
                    occ = 0;
                }
                else if (Cz == c)
                {
                    ++occ;
                }
            }
            exit(ritorno);
        }
    }

    for (int q = 0; q < Q; ++q)
    {
        close(PadreFiglio[q][0]);
    }

    for (int l = 0; l < L; ++l)
    {
        printf("Calcoli in arrivo per la linea %d^ del file \"%s\"\n", l + 1, argv[1]);

        for (int q = 0; q < Q; ++q)
        {
            write(PadreFiglio[q][1], &ok, sizeof(int));
        }
    }
    sleep(1);
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