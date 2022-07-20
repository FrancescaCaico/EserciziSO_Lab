
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
char linea[512];
typedef struct
{
    int c1; // indice del processo
    int c2; // num di occorrenze
} S;

int main(int argc, char **argv)
{

    if (argc < 4)
    {
        printf("Errore: numero parametri errato... almeno 1 file, 1 singolo carattere, 1 num pos\n");
        exit(1);
    }

    int pid, status, ritorno, fd;

    S curr;
    int occ = 0;
    char c;
    int N = argc - 3;
    if (argv[argc - 2][1] != 0)
    {
        printf("%s dev'essere un singolo carattere\n", argv[argc - 2]);
        exit(2);
    }
    char Cx = argv[argc - 2][0];

    int H = atoi(argv[argc - 1]);
    if (H <= 0)
    {
        printf("Errore: %s dev'essere numerico positivo\n", argv[argc - 1]);
        exit(3);
    }

    // comunicazione a pipeline
    pipe_t *pipes = malloc(N * sizeof(pipe_t));
    pipe_t *PadreFiglio = malloc(N * sizeof(pipe_t));

    if (!pipes || !PadreFiglio)
    {
        printf("Errore malloc\n");
        exit(4);
    }

    for (int k = 0; k < N; ++k)
    {
        if (pipe(PadreFiglio[k]) < 0 || pipe(pipes[k]) < 0)
        {
            exit(5);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore:impossibile generare il figlio num.%d\n", i);
            exit(6);
        }
        if (!pid)
        {

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            // chiusura pipe
            for (int k = 0; k < N; ++k)
            {
                if (k != i)
                {
                    close(PadreFiglio[k][0]);
                    close(pipes[k][1]);
                }
                if (!i || k != i - 1)
                {

                    close(pipes[k][0]);
                }

                close(PadreFiglio[k][1]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Impossibile aprire il file associato\n");
                exit(-1);
            }
            int pos = 0;
            int num_linea = 0;
            while (read(fd, linea + pos, 1))
            {
                if (linea[pos] == Cx)
                {
                    ++occ;
                }
                if (linea[pos] == 10)
                {
                    ++num_linea;
                    linea[pos] = 0;
                    // sono arrivata alla fine della linea devo passare avanti la struttura con maggiore occorrenza
                    // printf("Occorrenze per la %d^linea di %s --> %d\n", num_linea, argv[i + 1], occ);

                    if (i != 0)
                    {
                        read(pipes[i - 1][0], &curr, sizeof(S));
                        if (curr.c2 < occ)
                        {
                            curr.c1 = i;
                            curr.c2 = occ;
                        }
                    }
                    else
                    {
                        curr.c1 = i;
                        curr.c2 = occ;
                    }
                    write(pipes[i][1], &curr, sizeof(S));

                    // aspetto il padre per capire se devo stampare le mie info.
                    read(PadreFiglio[i][0], &c, 1);
                    if (c == 'S')
                    {
                        ++ritorno;
                        printf("Sono il figlio d'ordine %d con PID.%d.\nQuesta è la mia %d^ linea \"%s\"\n", i, getpid(), num_linea, linea);
                    }

                    pos = 0;
                    occ = 0;
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
        if (i != N - 1)
        {
            close(pipes[i][0]);
        }
        close(pipes[i][1]);
        close(PadreFiglio[i][0]);
    }

    sleep(1);
    for (int k = 0; k < H; ++k)
    {
        printf("Sto per ricevere la struttura riferita alla %d^ linea\n", k + 1);
        read(pipes[N - 1][0], &curr, sizeof(S));
        // printf("Ricevuta struttura %d %d\n", curr.c1, curr.c2);
        for (int i = 0; i < N; ++i)
        {
            if (curr.c1 != i)
            {
                c = 'N';
                write(PadreFiglio[i][1], &c, 1);
            }
            else
            {
                c = 'S';
                write(PadreFiglio[i][1], &c, 1);
            }
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