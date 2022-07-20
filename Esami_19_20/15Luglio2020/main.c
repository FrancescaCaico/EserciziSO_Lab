
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
    int c1;
    int c2;
    int c3;
} S;
char linea[250];
int main(int argc, char **argv)
{

    int pid, status, ritorno;
    int fd;
    int num_linea;
    S curr;

    if (argc != 3)
    {
        printf("Errore: mi serve solo un file e il suo numero di linee\n");
        exit(1);
    }

    int L = atoi(argv[2]);
    if (L <= 0 || L >= 255)
    {
        printf("ERRORE L=%s dev'essere un numero compreso tra 1 e 254\n", argv[2]);
        exit(2);
    }

    pipe_t *FP = malloc(L * sizeof(pipe_t));

    for (int q = 0; q < L; q++)
    {
        if (pipe(FP[q]) < 0)
        {
            printf("Errore generazione pipe\n");
            exit(3);
        }
    }

    for (int q = 0; q < L; q++)
    {
        if ((pid = fork()) < 0)
        {
            printf("Impossibile generare il figlio n.%d\n", q);
            exit(4);
        }
        if (!pid)
        {
            // codice
            for (int i = 0; i < L; ++i)
            {
                close(FP[i][0]);
                if (i != q)
                {
                    close(FP[i][1]);
                }
            }

            if ((fd = open(argv[1], O_RDONLY)) < 0)
            {
                printf("Errore file non valido\n");
                exit(-1);
            }
            int pos = 0;
            num_linea = 1;
            while (read(fd, linea + pos, 1))
            {
                ++pos;
                if (linea[pos - 1] == 10)
                {
                    if (num_linea == q + 1)
                    {
                        //è la linea interessata al processo...
                        curr.c1 = getpid();
                        curr.c2 = linea[1];
                        curr.c3 = linea[pos - 2];
                        write(FP[q][1], &curr, sizeof(curr));
                        break;
                    }
                    pos = 0;
                    ++num_linea;
                }
            }
            exit(q + 1);
        }
    }

    for (int q = 0; q < L; q++)
    {
        close(FP[q][1]);
    }

    for (int q = 0; q < L; q++)
    {
        read(FP[q][0], &curr, sizeof(S));
        if (curr.c2 == curr.c3)
        {
            printf("Il figlio d'indice %d ha trovato lo stesso carattere '%c' in seconda e penultima posizione della linea %d\n", q, curr.c2, q + 1);
        }
    }

    sleep(1);
    for (int k = 0; k < L; ++k)
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