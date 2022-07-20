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

    if (argc < 3)
    {
        printf("Errore numero dei parametri errato...almeno 2 file\n");
        exit(1);
    }
    char c;
    int pid, status, ritorno, fd; // variabili per definire  I processi
    int N = argc - 1;
    printf("Si devono generare %d figli\n", N);

    fd = creat("Merge", 0644);
    if (fd < 0)
    {
        /* code */
        printf("Impossibile creare il file merge\n");
        exit(2);
    }

    int *pids = malloc(N * sizeof(int));
    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (!FP || !pids)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (size_t i = 0; i < N; i++)
    {
        /* code */
        if (pipe(FP[i]) < 0)
        {
            printf("Errore pipe\n");
            exit(4);
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

            pause();

            while (read(fd, &c, 1))
            {
                write(FP[i][1], &c, 1);
                pause();
            }

            exit(c); // ILFILE+CORTOMANDAL'ULTIMO CARATTERE AL PADRE
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }
    // mando i segnali a tutti per iniziare la lettura
    sleep(1);
    for (int i = 0; i < N; ++i)
    {
        kill(pids[i], 16);
    }
    int index = -1;
    while (1)
    {
        sleep(1);
        for (int k = 0; k < N; ++k)
        {

            if (read(FP[k][0], &c, 1) != 1)
            {
                printf("IL FILE %s È IL + CORTO...\n", argv[k + 1]);
                index = k;
                for (int j = 0; j < N; ++j)
                {

                    if (index != j)
                    {

                        kill(pids[j], SIGKILL);
                    }
                }
                break;
            }

            write(fd, &c, 1);
            kill(pids[k], 16);
        }
        if (index != -1)
        {

            break;
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
            printf("Il figlio con pid.%d associato al file %s è terminato correttamente con valore di ritorno: %c.\n", pid, argv[CercaFile(pid, pids, N) + 1], ritorno);
        }
    }

    exit(0);
}
