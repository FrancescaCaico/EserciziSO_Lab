
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

void scrivi_linea(int sig)
{
    printf("Linea del figlio con PID.%d --> \"%s\"\n", getpid(), linea);
}

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Errore parametri!\n");
        exit(1);
    }

    int N = argc - 2;
    int K = atoi(argv[argc - 1]);
    if (K <= 0)
    {
        printf("Errore: K=%s non è un numero intero pos\n", argv[argc - 1]);
        exit(2);
    }

    int X;
    scanf("%d", &X);
    printf("Valore di X --> %d\n", X);
    if (X > K || X <= 0)
    {
        printf("Mi serve un numero valido.\n");
    }

    printf("il numero di figli da generare è %d\n", N);
    int *pids = malloc(N * sizeof(int));

    // genero una singola pipe.
    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (!pids || !FP)
    {
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            exit(4);
        }
    }
    int pid, status, ritorno, fd, L = 0;
    int n = 0;
    for (int i = 0; i < N; ++i)
    {
        pids[i] = fork();
        if (pids[i] < 0)
        {
            printf("Errore generazione processo figlio num %i\n", i);
            exit(5);
        }
        if (!pids[i])
        {
            signal(16, scrivi_linea);
            // codice figlio
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore file %s non valido\n", argv[i + 1]);
                exit(-1);
            }
            // printf("Sto per leggere il file\n");

            while (read(fd, linea + L, 1))
            {
                // printf("Letto carattere %c\n", linea[L]);
                if (linea[L] == 10)
                {
                    linea[L] = '\0';
                    ++L;
                    // fine di una linea... è quella interessata?
                    ++n;
                    //  printf("Linea num %d\n", n);
                    if (n == X)
                    {
                        // dico al padre la mia lunghezza.
                        //   printf("Sto per scrivere sulla pipe L=%d\n", L);
                        write(FP[i][1], &L, sizeof(int));
                        //  printf("Sto per andare in pausa...\n");
                        pause();
                        break;
                    }
                    L = 0;
                }
                else
                {
                    ++L;
                }
            }

            exit(L / 255);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    int *lunghezze = malloc(N);

    for (int i = 0; i < N; ++i)
    {
        read(FP[i][0], lunghezze + i, sizeof(int));
    }

    // sort lunghezze e pid.

    int k;
    int dim = N;
    bool ordinato = false;
    while (dim > 1 && !ordinato)
    {
        ordinato = true; /* hp: è ordinato */
        for (k = 0; k < dim - 1; k++)
            if (lunghezze[k] > lunghezze[k + 1])
            {
                int tmp = lunghezze[k];
                lunghezze[k] = lunghezze[k + 1];
                lunghezze[k + 1] = tmp;

                tmp = pids[k];
                pids[k] = pids[k + 1];
                pids[k + 1] = tmp;

                ordinato = false;
            }
        dim--;
    }

    for (int i = N - 1; i >= 0; --i)
    {

        printf("Sto per stampare la linea num. %d^ del file %s\n", X, argv[i + 1]);
        sleep(1);
        kill(pids[i], 16);
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