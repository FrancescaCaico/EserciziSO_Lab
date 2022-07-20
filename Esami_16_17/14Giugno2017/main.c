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
bool ok(bool *presente, int N)
{
    for (int j = 0; j < N; ++j)
    {
        if (!presente[j])
        {
            return true;
        }
    }
    return false;
}
int main(int argc, char **argv)
{

    int pid, status, ritorno = 0;
    int fd;

    if (argc < 3)
    {
        printf("Errore parametri errati\n");
        exit(1);
    }

    int N = argc - 2;
    if (argv[argc - 1][1] != 0)
    {
        printf("Errore %s non è un carattere singolo\n", argv[argc - 1]);
        exit(2);
    }

    char Cx = argv[argc - 1][0];
    long int pos; // per mandare la posizione dell'occorrenza al padre
    char car;

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    pipe_t *PF = malloc(N * sizeof(pipe_t));
    bool *presente = calloc(N, sizeof(bool));

    if (!FP || !PF)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(PF[i]) < 0 || pipe(FP[i]) < 0)
        {
            exit(4);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("errore creazione figlio n.%d\n", i);
            exit(5);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);
            for (int k = 0; k < N; ++k)
            {
                if (k != i)
                {
                    close(FP[k][1]);
                    close(PF[k][0]);
                }

                close(FP[k][0]);
                close(PF[k][1]);
            }

            // apriamo il file associato in RD_Wr
            if ((fd = open(argv[i + 1], O_RDWR)) < 0)
            {
                printf("Errore: impossibile aprire il file associato\n");
                exit(-1);
            }

            while (read(fd, &car, 1))
            {
                if (car == Cx)
                {
                    pos = lseek(fd, 0L, SEEK_CUR);
                    write(FP[i][1], &pos, sizeof(long int));
                    if (read(PF[i][0], &car, 1))
                    {
                        // ho letto un carattere... devo scriverlo.
                        // sposto il cursore indietro di 1
                        if (car != 10)
                        {
                            lseek(fd, -1L, SEEK_CUR);
                            write(fd, &car, 1);
                            ++ritorno;
                        }
                        // vado avanti
                    }
                }
            }
            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
        close(PF[i][0]);
    }
    char scarto;
    while (ok(presente, N))
    {
        for (int i = 0; i < N; ++i)
        {
            if (!presente[i])
            {
                if (read(FP[i][0], &pos, sizeof(long int)) != sizeof(long int))
                {
                    printf("Letta posizione %ld\n", pos);
                    presente[i] = 1;
                }
                else
                {
                    // RICHIEDO ALL'UTENTE UN CARATTERE
                    printf("Dammi un carattere da sostituire:\n");
                    read(0, &car, 1);
                    if (car != 10)
                    {
                        read(0, &scarto, 1);
                    }
                    write(PF[i][1], &car, 1);
                }
            }
        }
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