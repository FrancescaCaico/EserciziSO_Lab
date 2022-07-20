
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
char linea1[250];
char linea[250];
int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Errore: il programma necessita almeno di un nome di file\n");
        exit(1);
    }

    int N = argc - 1;
    printf("Il numero di file è %d.\nIl numero di figli da generare è %d\n", N, 2 * N);

    // creo due pipe pari dispari x comunicare con in padre.

    pipe_t *pari = malloc(N * sizeof(pipe_t));
    pipe_t *dispari = malloc(N * sizeof(pipe_t));
    if (!pari || !dispari)
    {
        printf("Errore malloc\n");
    }

    for (int n = 0; n < N; ++n)
    {
        if (pipe(pari[n]) < 0 || pipe(dispari[n]) < 0)
        {
            printf("Errore generazione pipe\n");
            exit(3);
        }
    }

    int pid, status, ritorno;
    int fd;            // per il fd
    int nro = INT_MIN; // per la lunghezza max trovata in linea
    int nro_curr = 0;  // per la lunghezza corrente
    int fcreato;       // per il file creato dal padre
    int x = 0;         // mi serve per contare se la linea è pari o dispari

    fcreato = creat("caico.log", 0644);

    if (fcreato < 0)
    {
        printf("Impossibile creare caico.log\n");
    }

    for (int n = 0; n < 2 * N; ++n)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore creazione figlio n.%d\n", n);
            exit(4);
        }
        if (!pid)
        {
            if (n % 2 == 0)
            {
                // un figlio pari
                for (int i = 0; i < N; ++i)
                {
                    if (i != n / 2)
                    {
                        close(pari[i][1]);
                    }
                    close(pari[i][0]);
                    close(dispari[i][1]);
                    close(dispari[i][0]);
                }

                // apertura del file associato
                if ((fd = open(argv[n / 2 + 1], O_RDONLY)) < 0)
                {
                    printf("Errore il file %s non è valido\n", argv[n / 2 + 1]);
                    exit(-1);
                }

                while (read(fd, linea + nro_curr, 1))
                {
                    ++nro_curr;
                    if (linea[nro_curr - 1] == 10)
                    {
                        ++x;
                        if (x % 2 == 0)
                        {
                            // sono in una linea pari...
                            write(pari[n / 2][1], &nro_curr, sizeof(int));
                            if (nro_curr > nro)
                            {
                                nro = nro_curr;
                            }
                        }
                        nro_curr = 0;
                    }
                }
                exit(nro);
            }
            else
            {
                // figlio dispari
                for (int i = 0; i < N; ++i)
                {
                    if (i != n / 2)
                    {
                        close(dispari[i][1]);
                    }
                    close(pari[i][0]);
                    close(pari[i][1]);
                    close(dispari[i][0]);
                }

                if ((fd = open(argv[(n + 1) / 2], O_RDONLY)) < 0)
                {
                    printf("Errore il file %s non è valido\n", argv[n / 2 + 1]);
                    exit(-1);
                }
                while (read(fd, linea + nro_curr, 1))
                {
                    ++nro_curr;
                    if (linea[nro_curr - 1] == 10)
                    {
                        ++x;
                        if (x % 2 != 0)
                        {
                            // sono in una linea pari...
                            write(dispari[n / 2][1], &nro_curr, sizeof(int));
                            if (nro_curr > nro)
                            {
                                nro = nro_curr;
                            }
                        }
                        nro_curr = 0;
                    }
                }
                exit(nro);
            }
        }
    }

    for (int n = 0; n < N; ++n)
    {
        close(pari[n][1]);
        close(dispari[n][1]);
    }
    int d = 1;
    int p = 1;
    int nrd = 0;
    int nrp = 0;
    nro = 0;

    for (int i = 0; i < N; ++i)
    {
        printf("Queste sono le lunghezze per il %s\n", argv[i + 1]);
        nro = 0;
        do
        {

            nrd = read(dispari[i][0], &d, sizeof(int));
            nrp = read(pari[i][0], &p, sizeof(int));

            if (nrd != 0)
            {
                printf("la linea num.%d ha lunghezza %d\n", nro, d);
                ++nro;
            }
            if (nrp != 0)
            {
                printf("la linea num.%d ha lunghezza %d\n", nro, p);
                ++nro;
            }

        } while (nrp || nrd);
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
            sprintf(linea, "Il figlio con pid.%d è terminato in modo anomalo con status:%d.\n", pid, status & 0xff);
            write(fcreato, linea, strlen(linea));
        }
        else
        {
            ritorno = (int)(status >> 8);
            ritorno &= 0xff;
            sprintf(linea, "Il figlio con pid.%d è terminato correttamente leggendo una lunghezza di %d caratteri al max.\n", pid, ritorno);
            write(fcreato, linea, strlen(linea));
        }
    }

    exit(0);
}