
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
        printf("Errore: numero parametri errati\n");
        exit(1);
    }

    int N = argc - 2;
    int X = atoi(argv[argc - 1]);

    printf("I figli da generare sono %d\n", N);

    pipe_t *pipeline = malloc(N * sizeof(pipe_t));
    if (pipeline == NULL)
    {
        printf("Errore: impossibile allocare memoria\n");
        exit(2);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(pipeline[i]) < 0)
        {
            printf("Errore impossibile generare la pipeline\n");
            exit(3);
        }
    }

    int pid, status, ritorno, fd;
    char first;
    char curr;
    // al padre deve arruvare un array di primiCar contenente N primi caratteri e lo deve ricevere X volte un arry per ogni linea
    char *primiCar = calloc((N), sizeof(char));

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
        }
        if (pid == 0)
        {
            // codice figlio
            // chiusura pipeline
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

            // apertura file associato
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file '%s' non è valido.\n", argv[i + 1]);
                exit(-1);
            }
            bool iniziolinea = 0;

            while (read(fd, &curr, 1))
            {
                if (iniziolinea == 0)
                {
                    // sono a inizio della linea... salvo il primo carattere
                    first = curr;
                    iniziolinea = 1;
                }
                else
                {
                    if (curr == 10)
                    {
                        //è finita la linea corrente. Comunico il mio primo carattere mettendolo nell'array mandatomi dal figlio prec
                        if (i != 0)
                        {
                            read(pipeline[i - 1][0], primiCar, N * sizeof(char));
                            // printf("sto per mettere nell'array primi car %c\n", first);
                            primiCar[i] = first;
                        }
                        primiCar[i] = first;
                        write(pipeline[i][1], primiCar, N);
                        iniziolinea = 0;
                    }
                }
            }
            exit(first);
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

    for (int i = 0; i < X; ++i)
    {

        printf("Sto per mostrare l'array di char PRIMI CARATTERI per la linea num.%d\n", i + 1);

        read(pipeline[N - 1][0], primiCar, N);
        for (int i = 0; i < N; ++i)
        {
            printf("Letto primo carattere pari a '%c' dal figlio num.%i associato al file %s\n", primiCar[i], i, argv[i + 1]);
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
            printf("Il figlio con pid.%d è terminato in modo anomalo con status :%c.\n", pid, status & 0xff);
        }
        else
        {
            ritorno = (int)(status >> 8);
            ritorno &= 0xff;
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%c.\n", pid, ritorno);
        }
    }

    exit(0);
}
