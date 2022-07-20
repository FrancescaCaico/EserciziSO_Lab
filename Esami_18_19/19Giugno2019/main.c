
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

char linea[250];

typedef int pipe_t[2];

bool ok(bool *presenti, int N)
{
    for (int k = 0; k < N; ++k)
    {
        if (presenti[k] == 0)
        {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv)
{

    int pid, status, ritorno = 0;
    int fd;
    int pos;
    char c;
    char MAX;

    if (argc < 3)
    {
        printf("Errore numero dei parametri errato!\n");
        exit(1);
    }
    int N = argc - 1;

    pipe_t *FiglioPadre = malloc(N * sizeof(pipe_t));
    pipe_t *PadreFiglio = malloc(N * sizeof(pipe_t));
    bool *presenti = calloc(N, sizeof(bool));
    if (!FiglioPadre || !PadreFiglio || !presenti)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        if (pipe(FiglioPadre[i]) < 0 || pipe(PadreFiglio[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        pid = fork();
        if (pid < 0)
        {
            printf("Errore impossibile generare il figlio n.%d\n", i);
            exit(4);
        }
        if (!pid)
        {

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FiglioPadre[k][1]);
                    close(PadreFiglio[k][0]);
                }
                close(FiglioPadre[k][0]);
                close(PadreFiglio[k][1]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: impossibile aprire il file associato\n");
                exit(-1);
            }
            pos = 0;
            while (read(fd, linea + pos, 1))
            {
                if (linea[pos] == 10)
                {
                    // FINE LINEA MANDO AL PADRE IL PRIMO CARATTERE
                    write(FiglioPadre[i][1], &linea[0], 1);
                    // attendo dal padre l'ordine di stampa o meno
                    if (read(PadreFiglio[i][0], &c, 1))
                    {
                        // ha mandato qualcosa e mi tocca stampare...
                        if (c == 's')
                        {
                            linea[pos] = 0;
                            ++ritorno;
                            printf("Sono il figlio con PID.%d e ordine %d associato al file \"%s\" e ho il primo carattere massimo nella mia linea...\nLinea --> \"%s\"\n", getpid(), i, argv[i + 1], linea);
                        }
                    }
                    pos = 0;
                }
                else
                {
                    ++pos;
                }
            }

            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(PadreFiglio[i][0]);
        close(FiglioPadre[i][1]);
    }

    while (ok(presenti, N))
    {

        for (int j = 0; j < N; ++j)
        {
            if (!presenti[j])
            {
                if (read(FiglioPadre[j][0], &c, 1) != 1)
                {
                    presenti[j] = 1;
                }
                else
                {
                    if (c > MAX)
                    {
                        pos = j;
                        MAX = c;
                    }
                }
            }
        }

        for (int i = 0; i < N; ++i)
        {

            if (i == pos)
            {
                c = 's';
            }
            else
            {
                c = 'n';
            }
            if (!presenti[i])
            {
                write(PadreFiglio[i][1], &c, 1);
            }
        }

        MAX = CHAR_MIN;
        pos = -1;
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