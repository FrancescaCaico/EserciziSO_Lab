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
    int c1; // ordine del figlio...
    int c2; // occorrenze di Cx
} info;

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

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Errore numero dei parametri errato\n");
        exit(1);
    }

    int N = argc - 2;
    int H = atoi(argv[argc - 1]);
    if (H <= 0)
    {
        printf("L'ultimo parametro non numerico positivo\n");
        exit(2);
    }
    int occ = 0;
    char c;
    char Cx;
    info curr;
    scanf("%c", &Cx);

    int pid, status, ritorno, fd;

    // L'ultimo parametro è Il numero di linee dei file...

    printf("Si devono generare %d figli\n", N);

    pipe_t *pipeline = malloc(N * sizeof(pipe_t));
    int *pids = malloc(N * sizeof(int));
    // bool *to_kill = calloc(N, sizeof(bool));
    if (!pipeline || !pids)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (size_t i = 0; i < N; i++)
    {
        /* code */
        if (pipe(pipeline[i]) < 0)
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

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

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
                printf("il file %s non è valido\n", argv[i + 1]);
                exit(-1);
            }

            while (read(fd, &c, 1))
            {
                if (c == Cx)
                {
                    ++occ;
                }
                else if (c == 10)
                {
                    // fine linea
                    if (i != 0)
                    {
                        if (read(pipeline[i - 1][0], &curr, sizeof(info)) != sizeof(info))
                        {
                            exit(2);
                        }
                        if (curr.c2 > occ)
                        {
                            curr.c1 = i;
                            curr.c2 = occ;

                            if (write(pipeline[i][1], &curr, sizeof(info)) != sizeof(info))
                                exit(2);
                        }
                        else
                        {
                            if (write(pipeline[i][1], &curr, sizeof(info)) != sizeof(info))
                                exit(2);
                        }
                    }
                    else
                    {
                        curr.c1 = i;
                        curr.c2 = occ;
                        if (write(pipeline[i][1], &curr, sizeof(info)) != sizeof(info))
                            exit(2);
                    }
                    occ = 0;
                }
            }
            exit(0); // solo se è andata a buon fine... il file AF E FN sono uguali...
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

    for (int i = 0; i < H; ++i)
    {

        read(pipeline[N - 1][0], &curr, sizeof(info));
        printf("Ricevuta struttura con campi: C1 --> %d (indice); C2 --> %d (occorrenze). Associata al figlio con PID.%d\n", curr.c1, curr.c2, pids[curr.c1]);
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
