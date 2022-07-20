// CORRETTO
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
    char v1;
    long int v2; // numero di occorrenze di v1

} S;

void bubbleSort(S v[], int dim)
{
    int i;
    bool ordinato = false;
    while (dim > 1 && !ordinato)
    {
        ordinato = true; /* hp: è ordinato */
        for (i = 0; i < dim - 1; i++)
            if (v[i].v2 > v[i + 1].v2)
            {
                S tmp = v[i];
                v[i] = v[i + 1];
                v[i + 1] = tmp;
                ordinato = false;
            }
        dim--;
    }
}
int main(int argc, char **argv)
{
    int pid, status, ritorno, fd;
    if (argc != 2)
    {
        printf("Errore: numero dei parametri errato... 1 solo file\n");
        exit(1);
    }

    int N = 26;
    printf("IL NUMERO DEI PROCESSI DA GENERARE È %d\n", 26);

    S *alfabeto = calloc(N, sizeof(S));
    int *pids = calloc(N, sizeof(int));
    pipe_t *pipes = calloc(N, sizeof(pipe_t));
    if (alfabeto == NULL || pids == NULL || pipes == NULL)
    {
        printf("Errore calloc\n");
        exit(2);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(pipes[i]) < 0)
        {
            printf("Errore generazione della pipeline\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            printf("Errore: impossibile generare il figlio %d associato al carattere %c", i, i + 'a');
            exit(4);
        }
        if (pids[i] == 0)
        {
            // codice del figlio i-esimo
            // presentazione:
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s' e cerco le occorrenze di %c\n", getpid(), i, argv[1], i + 'a');
            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(pipes[k][1]);
                }
                if ((i == 0) || (k != i - 1))
                {
                    close(pipes[k][0]);
                }
            }

            // apertura file
            if ((fd = open(argv[1], O_RDONLY)) < 0)
            {
                printf("Errore: impossibile aprire il file\n");
                exit(-1);
            }
            char c;
            long int occ = 0;

            while (read(fd, &c, 1))
            {
                if (c == ('a' + i))
                {
                    ++occ;
                }
            }

            if (i != 0)
            {
                read(pipes[i - 1][0], alfabeto, i * sizeof(S));
            }
            alfabeto[i].v1 = 'a' + i;
            alfabeto[i].v2 = occ;
            write(pipes[i][1], alfabeto, (i + 1) * sizeof(S));
            exit(c);
        }
    }

    // codice padre
    for (int i = 0; i < N; ++i)
    {
        if (i != N - 1)
        {
            close(pipes[i][0]);
        }
        close(pipes[i][1]);
    }

    // ricevo la struttura
    read(pipes[N - 1][0], alfabeto, N * sizeof(S));
    // for (int i = 0; i < N; i++)
    // {
    //     /* code */

    //     printf("Il figlio con pid.%d ha calcolato %ld occorrenze di %c\n", pids[i], alfabeto[i].v2, alfabeto[i].v1);
    // }
    bubbleSort(alfabeto, N);

    for (int i = 0; i < N; i++)
    {
        /* code */
        int index = alfabeto[i].v1 - 'a';

        printf("Il figlio con pid.%d ha calcolato %ld occorrenze di %c\n", pids[index], alfabeto[i].v2, alfabeto[i].v1);
    }

    sleep(1);
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
            printf("Il figlio con pid.%d  è terminato correttamente con valore di ritorno:%c (%d).\n", pid, ritorno, ritorno);
        }
    }

    exit(0);
}