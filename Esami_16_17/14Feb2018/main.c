
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

int mia_random(int n)
{
    int casuale;
    casuale = rand() % n;
    return casuale;
}
int main(int argc, char **argv)
{
    srand(time(NULL));
    if (argc < 4)
    {
        printf("Errore: numero dei parametri errato\n");
        exit(1);
    }
    int N = argc - 2;
    int H = atoi(argv[argc - 1]); // LUNGHEZZA IN LINEE DI UN FILE
    if (H <= 0 || H >= 255)
    {
        printf("Errore: %s non numerico positivo compreso tra 0 e 255 esclusi\n", argv[argc - 1]);
        exit(2);
    }

    printf("Il numero di figli dal generare è %d\n", N);

    int L = 0;
    int min = 256;
    int r;
    int num = 0;
    int pid, status, ritorno = 0, fd;

    char c;

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    pipe_t *PF = malloc(N * sizeof(pipe_t));
    if (!FP || !PF)
    {
        printf("Errore impossibile allocare la memoria per le pipe\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0 || pipe(PF[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(4);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore generazione figlio n.%d fallita\n", i);
            exit(5);
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
                printf("Impossibile aprire il file associato\n");
                exit(-1);
            }

            while (read(fd, &c, 1))
            {
                ++L;
                if (c == 10)
                {
                    // mando al padre la lunghezza
                    ++num;
                    write(FP[i][1], &L, sizeof(int));
                    read(PF[i][0], &r, sizeof(int));
                    printf("Valore di r -->%d\n", r);
                    lseek(fd, -(L - r), SEEK_CUR);
                    read(fd, &c, 1);
                    // c = linea[r];
                    printf("Il figlio num.%d con PID.%d e associato al file %s ha trovato in posizione %d della linea n.%d il carattere %c\n", i, getpid(), argv[i + 1], r, num, c);
                    lseek(fd, (L - r - 1), SEEK_CUR);
                    L = 0;
                    ++ritorno;
                }
            }

            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
        close(PF[i][0]);
    }

    for (int i = 0; i < H; ++i)
    {

        for (int k = 0; k < N; ++k)
        {
            read(FP[k][0], &L, sizeof(int));
            printf("Valore letto di L-->%d\n", L);
            if (L < min)
            {
                min = L;
            }
        }
        r = mia_random(min);
        // printf("L'indice che sto per mandare per la linea %d ha valore %d\n", i + 1, r);
        for (int l = 0; l < N; ++l)
        {
            write(PF[l][1], &r, sizeof(r));
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