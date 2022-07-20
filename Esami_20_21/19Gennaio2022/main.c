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

    if (argc < 3)
    {
        printf("Mi aspetto almeno un file + un numero positivo\n");
        exit(1);
    }

    int C = atoi(argv[argc - 1]);
    if (C <= 0)
    {
        printf("Errore: la lunghezza in byte non è positiva/numerica\n");
        exit(2);
    }

    int N = argc - 2;
    printf("Debug - IL numero dei figli da generare è %d\n", N);

    int pid, status, ritorno = 0;
    int fd;
    int fcreato; // file descriptor per ilfile da creare
    int i;       // variabile x cicli dei figli
    char b[C];   // dove inserire i blocchi letti
    int nro;     // valore di ritorno

    pipe_t *ps = malloc(N * sizeof(pipe_t));
    if (!ps)
    {
        printf("errore malloc\n");
        exit(3);
    }

    for (i = 0; i < N; ++i)
    {

        if (pipe(ps[i]) < 0)
        {
            printf("Errore generazione pipe fallita!\n");
            exit(4);
        }
    }

    for (i = 0; i < 2 * N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore creazione figlio n.%d\n", i);
            exit(5);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i % N + 1]);

            if ((fd = open(argv[i % N + 1], O_RDONLY)) < 0)
            {
                printf("Errore il file %s non è valido\n", argv[i % N + 1]);
                exit(-1);
            }

            int dim = lseek(fd, 0L, SEEK_END);
            dim /= 2; // ottengo la metà

            if (i < N)
            {
                lseek(fd, 0L, SEEK_SET);
                for (int k = 0; k < N; ++k)
                {
                    if (i != k)
                    {
                        close(ps[k][1]);
                    }
                    close(ps[k][0]);
                }

                while (lseek(fd, 0L, SEEK_CUR) < dim)
                {
                    read(fd, b, C);
                    ++nro;
                    write(ps[i][1], b, C);
                }
            }
            else
            {
                lseek(fd, dim, SEEK_SET);

                for (int k = 0; k < N; ++k)
                {
                    if (i % N != k)
                    {
                        close(ps[k][0]);
                    }
                    close(ps[k][1]);
                }

                // il 2^ figlio deve creare il file
                char *nome = calloc(strlen(argv[i % N + 1]) + 11, sizeof(char));
                if (!nome)
                {
                    exit(-1);
                }
                sprintf(nome, "%s.mescolato", argv[i % N + 1]);
                fcreato = creat(nome, 0644);
                if (fcreato < 0)
                {
                    printf("Errore:impossibile creare il file %s\n", nome);
                    exit(-1);
                }

                // sposto il file pointer a metà.
                dim *= 2;
                while (lseek(fd, 0L, SEEK_CUR) < dim)
                {
                    read(fd, b, C);
                    ++nro;
                    write(fcreato, b, C);
                    read(ps[i % N][0], b, C);
                    write(fcreato, b, C);
                }
            }
            exit(nro);
        }
    }

    for (i = 0; i < N; ++i)
    {
        close(ps[i][1]);
        close(ps[i][0]);
    }

    for (int k = 0; k < 2 * N; ++k)
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
            printf("Il figlio con pid.%d è terminato correttamente leggendo %d blocchi.\n", pid, ritorno);
        }
    }

    exit(0);
}