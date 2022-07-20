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
    int c1; // indice del processo
    int c2; // numero corrispondente a Cn
} S;

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Errore mi aspetto almeno un nome di file\n");
        exit(1);
    }
    int N = argc - 1;
    printf("Il padre genererà %d figli\n", N);
    char Cn; // carattere numerico.
    pipe_t *pipeline = malloc(sizeof(pipe_t) * N);
    for (int i = 0; i < N; ++i)
    {
        if (pipe(pipeline[i]) < 0)
        {
            printf("Errore impossibile generare la pipeline\n");
            exit(2);
        }
    }
    int pid, status, ritorno, fd;
    int *pids = malloc(N * sizeof(int));
    for (int i = 0; i < N; ++i)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            printf("Errore generazione figlio num.%d\n", i);
            exit(3);
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
                if (i == 0 || (k != i - 1))
                {
                    close(pipeline[k][0]);
                }
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("il file %s non è valido\n", argv[i + 1]);
                exit(-1);
            }

            while (read(fd, &Cn, 1))
            {
                if (isdigit(Cn))
                {
                    break;
                }
            }
            S *p = malloc((i + 1) * sizeof(S));

            if (i != 0)
            {
                read(pipeline[i - 1][0], p, i * sizeof(S));
            }
            p[i].c1 = i;
            p[i].c2 = atoi(&Cn);
            write(pipeline[i][1], p, (i + 1) * sizeof(S));

            exit(Cn);
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

    S *p = malloc(N * sizeof(S));
    read(pipeline[N - 1][0], p, N * sizeof(S));
    long int sum = 0;
    for (int i = 0; i < N; ++i)
    {
        printf("Il figlio con pid.%d ha fornito la seguente struttura:\n\t-INDICE:%d\n\t-VALORE NUMERICO:%d\n", pids[i], p[i].c1, p[i].c2);
        sum += p[i].c2;
    }

    printf("LA SOMMA TOTALE DEI CARATTERI NUMERICI TROVATI È:%ld\n", sum);

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
            printf("Il figlio con pid.%d  è terminato correttamente con valore di ritorno:%c.\n", pid, ritorno);
        }
    }

    exit(0);
}
