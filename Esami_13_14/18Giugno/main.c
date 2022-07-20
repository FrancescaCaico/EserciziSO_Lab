
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
char linea[255];
int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Errore: il numero dei parametri devono essere almeno 3 \n");
        exit(1);
    }

    int N = argc - 2;
    int X = atoi(argv[argc - 1]);
    int *f = malloc(N * sizeof(int));
    for (int i = 0; i < X; ++i)
    {
        sprintf(linea, "/tmp/%d", i);
        f[i] = creat(linea, 0644);
    }

    pipe_t *FP = calloc(N, sizeof(pipe_t));
    if (!FP)
    {
        printf("Errore calloc\n");
        exit(2);
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        if (pipe(FP[i]) < 0)
        {
            exit(3);
        }
    }

    int pid, status, ritorno, fd, len = 0, tot = 0;

    for (int i = 0; i < N; i++)
    {
        /* code */
        pid = fork();
        if (pid < 0)
        {
            exit(4);
        }
        if (pid == 0)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            // chiusure pipe
            for (int k = 0; k < N; k++)
            {
                /* code */
                close(FP[k][0]);
                if (i != k)
                {
                    close(FP[k][1]);
                }
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("File di nome %s non valido.\n", argv[i + 1]);
                exit(-1);
            }

            while (read(fd, linea + len, 1))
            {
                if (linea[len] == 10)
                {
                    ++len;
                    // fine linea mando la lunghezza
                    write(FP[i][1], &len, sizeof(int));

                    write(FP[i][1], linea, len);
                    tot += len;
                    len = 0;
                }
                else
                {
                    ++len;
                }
            }
            exit(tot / X);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    for (int j = 0; j < N; ++j)
    {
        for (int i = 0; i < X; ++i)
        {
            read(FP[i][0], &len, sizeof(int));
            // leggo la linea
            read(FP[i][0], linea, len * sizeof(char));
            write(f[i], linea, len * sizeof(char));
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
