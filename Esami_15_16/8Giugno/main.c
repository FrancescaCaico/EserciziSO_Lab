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
#include <time.h>

typedef int pipe_t[2];
char linea[255];

int mia_random(int n)
{
    int casuale;
    casuale = rand() % n;
    return casuale;
}
int main(int argc, char **argv)
{
    srand(time(NULL));
    int pid, status, ritorno, fd;
    int L = 0;
    int x = 0;

    if (argc < 6)
    {
        printf("Errore almeno 4 file + numero intero\n");
        exit(1);
    }

    int H = atoi(argv[argc - 1]); // lunghezza in linee
    if (H <= 0)
    {
        printf("Errore: %s non numerico positivo\n", argv[argc - 1]);
        exit(2);
    }

    int N = argc - 2;
    int Fcreato = creat("/tmp/creato", 0644);
    if (Fcreato < 0)
    {
        printf("Errore: impossibile creare il file temporaneo\n");
        exit(3);
    }

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    pipe_t *PF = malloc(N * sizeof(pipe_t));
    if (FP == NULL || PF == NULL)
    {
        printf("Errore malloc\n");
        exit(4);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0 || pipe(PF[i]) < 0)
        {
            exit(5);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore generazione figlio num%d\n", i);
            exit(6);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);
            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FP[k][1]);
                    close(PF[k][0]);
                }
                close(FP[k][0]);
                close(PF[k][1]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Il file non è valido\n");
                exit(-1);
            }
            ritorno = 0;
            while (read(fd, linea + L, 1))
            {
                ++L;
                if (linea[L - 1] == 10)
                {
                    // fine linea
                    write(FP[i][1], &L, sizeof(int));
                    read(PF[i][0], &x, sizeof(int));
                    if (x <= L)
                    {
                        write(Fcreato, linea + x, 1);
                        ++ritorno;
                    }
                    L = 0;
                }
            }
            exit(ritorno);
        }
    }

    for (int k = 0; k < N; ++k)
    {

        close(FP[k][1]);
        close(PF[k][0]);
    }

    for (int k = 0; k < H; ++k)
    {

        // quale considerare tra quelle ricevute.
        int index = mia_random(N);
        int L = 0;
        for (int i = 0; i < N; ++i)
        {
            if (i != index)
            {
                read(FP[i][0], &x, sizeof(int));
            }
            else
            {
                read(FP[i][0], &L, sizeof(int));
            }
        }

        index = mia_random(L);
        for (int i = 0; i < N; ++i)
        {
            write(PF[i][1], &index, sizeof(int));
        }
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
            printf("Il figlio con pid.%d  è terminato correttamente con valore di ritorno:%d.\n", pid, ritorno);
        }
    }

    exit(0);
}