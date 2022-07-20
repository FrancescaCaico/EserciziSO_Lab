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

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Errore: numero dei parametri errato...\n");
        exit(1);
    }

    int X = atoi(argv[argc - 1]);
    if (X <= 0)
    {
        printf("Errore X=%s non numerico positivo\n", argv[argc - 1]);
        exit(2);
    }

    int N = argc - 2;
    printf("Il numero di figli da generare è %d\n", N);

    char *primiNumCar = calloc(X, sizeof(char));
    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (FP == NULL || primiNumCar == NULL)
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

    int pid, status, ritorno, fd;
    char c;
    int num_linea = 0;

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore: impossibile generare il figlio num.%d\n", i);
            exit(5);
        }
        if (!pid)
        {
            // codice figlio
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("il file %s non è valido\n", argv[i + 1]);
                exit(-1);
            }

            bool trovato = false;

            while ((read(fd, &c, 1)))
            {
                /* code */
                if (!trovato && isdigit(c))
                {
                    trovato = 1;
                    primiNumCar[num_linea] = c;
                }
                else if (c == 10)
                {
                    ++num_linea;
                    trovato = 0;
                }
            }

            write(FP[i][1], primiNumCar, X * sizeof(char));
            exit(primiNumCar[X - 1]);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }
    char max = CHAR_MIN;
    int index = -1;
    for (int i = 0; i < N; ++i)
    {
        read(FP[i][0], primiNumCar, X * sizeof(char));
        printf("Sto per farti visualizzare i caratteri trovati dal figlio %d associato al file %s\n", i, argv[i + 1]);
        for (int k = 0; k < X; ++k)
        {
            printf("Questo è il carattere trovato alla linea %d dal figlio %d --> '%c'\n", k + 1, i, primiNumCar[k]);
            if (primiNumCar[k] > max)
            {
                max = primiNumCar[k];
                index = i;
            }
        }
    }
    printf("Il figlio %d ha trovato il numero maggiore tra le sue linee pari a %c\n", index, max);
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
            printf("Il figlio con pid.%d  è terminato correttamente con valore di ritorno:%c.\n", pid, ritorno);
        }
    }

    exit(0);
}