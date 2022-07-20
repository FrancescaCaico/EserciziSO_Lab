
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
char linea[200];
char dim[4];
typedef int pipe_t[2];
int main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("Errore numero di parametri errato!\n");
        exit(1);
    }
    int N = argc - 1;
    int n;

    int fcreato = creat("caicofrancesca", 0644);

    // calcolo la lunghezza creando un processo figlio 'speciale' posso prendere un file qualsiasi perchè assumiamo che tutti abbiamo
    // lo stesso numero di linee.

    int pid, status, ritorno, fd;
    pipe_t X;
    char c;
    if (pipe(X) < 0)
    {
        exit(9);
    }

    pid = fork();
    if (pid < 0)
    {
        printf("Errore: impossibile generare il figlio per la conta delle linee dei file!\n");
        exit(2);
    }
    if (!pid)
    {

        // effettuo la ridirezione dell'input per avere il solo valore numerico
        close(0);
        open(argv[1], O_RDONLY);
        close(1);
        dup(X[1]);
        close(X[0]);
        close(X[1]);
        execlp("wc", "wc", "-l", (char *)0);
        perror("Se vedi questa linea la wc -l non è andata a buon fine");
        exit(-2);
    }
    close(X[1]);

    // leggo dal file
    int pos = 0;
    while (read(X[0], &c, 1))
    {
        dim[pos] = c;
        ++pos;
    }
    close(X[0]);
    dim[pos] = 0;
    int nroLinee = atoi(dim);
    int len = 0;
    printf("Il numero delle linee dei file è %d\n", nroLinee);

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

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (!FP)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(4);
        }
    }

    for (n = 0; n < N; n++)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore generazione figlio n.%d\n", n);
            exit(5);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), n, argv[n + 1]);

            // i figli producono per il padre una lunghezza in termini di int e una linea
            for (int k = 0; k < N; ++k)
            {
                if (n != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            // apertura file associato

            if ((fd = open(argv[n + 1], O_RDONLY)) < 0)
            {
                printf("Errore file '%s' non valido\n", argv[n + 1]);
                exit(-1);
            }

            while (read(fd, linea + len, 1))
            {
                ++len;
                if (linea[len - 1] == 10)
                {
                    // fine linea
                    write(FP[n][1], &len, sizeof(int));
                    write(FP[n][1], linea, len);
                    ritorno = len;
                    len = 0;
                }
            }
            exit(ritorno);
        }
    }

    for (n = 0; n < N; ++n)
    {
        close(FP[n][1]);
    }

    for (int k = 0; k < nroLinee; ++k)
    {
        for (int i = 0; i < N; ++i)
        {
            read(FP[i][0], &len, sizeof(int));
            read(FP[i][0], linea, len);
            write(fcreato, linea, len);
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
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%d.\n", pid, ritorno);
        }
    }

    exit(0);
}