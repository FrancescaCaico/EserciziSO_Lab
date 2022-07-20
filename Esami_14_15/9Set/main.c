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

int CercaFile(int pid, int *pids, int N)
{

    for (int i = 0; i < N; ++i)
    {
        if (pid == pids[i])
        {
            return i;
        }
    }
    return -1;
}

void go()
{
    printf("Il figlio con pid %d puo continuare la lettura...\n", getpid());
}

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Errore numero dei parametri errato\n");
        exit(1);
    }

    int N = argc - 2;
    char c;
    char cp;
    int pid, status, ritorno, fd;

    // L'ultimo parametro è il file a cui è associato il padre...

    printf("Si devono generare %d figli\n", N);

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    int *pids = malloc(N * sizeof(int));
    bool *to_kill = calloc(N, sizeof(bool));
    if (!FP || !pids || !to_kill)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (size_t i = 0; i < N; i++)
    {
        /* code */
        if (pipe(FP[i]) < 0)
        {
            printf("Errore pipe\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        pids[i] = fork();
        if (pids[i] < 0)
        {
            printf("Errore generazione figlio num.%d\n", i);
        }
        if (!pids[i])
        {
            signal(16, go);
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            // prima di iniziare la lettura devo ricevere il segnale dal padre

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("il file %s non è valido\n", argv[i + 1]);
                exit(-1);
            }

            // pause();

            while (read(fd, &c, 1))
            {
                write(FP[i][1], &c, 1);
                pause();
            }

            exit(0); // solo se è andata a buon fine... il file AF E FN sono uguali...
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    if ((fd = open(argv[argc - 1], O_RDONLY)) < 0)
    {
        printf("il file %s non è valido\n", argv[argc + 1]);
        exit(4);
    }
    // mando i segnali a tutti per iniziare la lettura
    // for (int i = 0; i < N; ++i)
    // {
    //     kill(pids[i], 16);
    // }

    while (read(fd, &cp, 1))
    {

        // sleep(1);
        for (int k = 0; k < N; ++k)
        {

            if (!to_kill[k])
            { // SIGNIFICA CHE ANCORA MANDA LETTERE

                sleep(1);
                read(FP[k][0], &c, 1);
                if (c == cp)
                {

                    // gli dico di continuare
                    // printf("Il figlio con PID.%d non è da killare.\n", pids[k]);

                    kill(pids[k], 16);
                }
                else
                {
                    // printf("Il figlio con PID.%d è da killare.\n", pids[k]);
                    to_kill[k] = 1;
                }
            }
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if (to_kill[i])
        {

            kill(pids[i], SIGKILL);
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
            printf("Il figlio con pid.%d associato al file %s è terminato correttamente con valore di ritorno:%d.\n", pid, argv[CercaFile(pid, pids, N) + 1], ritorno);
        }
    }
    exit(0);
}
