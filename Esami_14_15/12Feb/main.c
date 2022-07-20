
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
    int c1;      // indice del processo
    long int c2; // numero di occorrenze di Cx trovate nel file.
} info;

int main(int argc, char **argv)
{

    int pid, status, ritorno;
    int fd;
    char c;
    if (argc < 4)
    {
        printf("Mi aspetto almeno due file + un carattere singolo.\n");
        exit(1);
    }

    int N = argc - 2;

    if (argv[argc - 1][1] != 0)
    {
        printf("ERRORE: %s non carattere singolo.\n", argv[argc - 1]);
        exit(2);
    }

    char Cx = argv[argc - 1][0];
    info curr;

    // schema di comunicazione in pipeline
    pipe_t *pipeline = malloc(N * sizeof(pipe_t));
    int *pids = malloc(N * sizeof(int));
    if (pipeline == NULL || !pids)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        if (pipe(pipeline[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
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
            exit(4);
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
                printf("Errore file --> %s non valido...\n", argv[i + 1]);
                exit(-1);
            }

            curr.c1 = i;
            curr.c2 = 0;
            while (read(fd, &c, 1))
            {
                if (c == Cx)
                {
                    ++curr.c2;
                }
            }

            info *AN = calloc((i + 1), sizeof(info));
            if (AN == NULL)
            {
                printf("Errore malloc dell'array AN\n");
                exit(-2);
            }

            if (i != 0)
            {
                read(pipeline[i - 1][0], AN, i * sizeof(info));
            }
            AN[i] = curr;
            write(pipeline[i][1], AN, (i + 1) * sizeof(info));
            exit(curr.c1);
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

    // devo ricevere la struttura
    info *TOT = calloc(N, sizeof(info));
    if (TOT == NULL)
    {
        printf("Errore malloc TOT\n");
        exit(6);
    }

    read(pipeline[N - 1][0], TOT, N * sizeof(info));

    for (int i = 0; i < N; i++)
    {
        /* code */

        printf("Struttura del figlio con PID.%d:\n\tINDICE --> %d\n\tOCCORRENZE DI '%c' --> %ld\n", pids[i], TOT[i].c1, Cx, TOT[i].c2);
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
            printf("Il figlio con pid.%d è terminato in modo anomalo con status :%d.\n", pid, status & 0xff);
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