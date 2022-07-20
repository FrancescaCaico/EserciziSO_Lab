
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

#define PERM 0644

char linea[250]; // array static per mantenere l'ultima linea
typedef int pipe_t[2];

int main(int argc, char **argv)
{

    int pid, status, ritorno; // variabili per gestire le wait, i valori di ritorno e la creazione dei processi
    int fd;                   // variabile per contenere i File descriptor dei file aperti.
    int N;                    // variabile per salvare il numero dei processi da generare
    int L = 0;
    long int last;
    long int pos = 0;

    if (argc < 3)
    {
        printf("Errore:numero dei parametri errato\n");
        exit(1);
    }
    N = argc - 1;
    printf("Il numero di processi da generare è %d\n", N);

    int Fcreato = creat("Ultime_Linee", PERM);
    if (Fcreato < 0)
    {
        printf("Errore: impossibile generare il file Ultime_Linee\n");
        exit(2);
    }

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (FP == NULL)
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

    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Impossibile generare figlio num.%d\n", i);
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
                }
                close(FP[k][0]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Impossibile aprire il file associato\n");
                exit(-1);
            }

            // calcolo la lunghezza del file
            last = lseek(fd, 0L, SEEK_END);
            lseek(fd, 0L, SEEK_SET);
            // printf("Valore di last --> %ld\n", last);

            while (read(fd, linea + L, 1))
            {
                ++L;
                if (linea[L - 1] == 10)
                {
                    // sono a fine linea...
                    // printf("Valore di pos --> %ld\n", pos);
                    if (pos == last - 1)
                    {
                        //è finito il file e sono nella linea giusta da mandare al padre
                        write(FP[i][1], &L, sizeof(int));
                        write(FP[i][1], linea, L * sizeof(char));
                    }
                    ritorno = L;
                    L = 0;
                }
                ++pos;
            }

            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    for (int i = 0; i < N; ++i)
    {
        read(FP[i][0], &L, sizeof(int));
        read(FP[i][0], linea, L);
        write(Fcreato, linea, L);
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