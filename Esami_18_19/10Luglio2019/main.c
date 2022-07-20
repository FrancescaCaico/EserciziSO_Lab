
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

    int pid, status, ritorno = 0;
    int fd;
    long int pos, posLetta;
    char c;

    if (argc < 4)
    {
        printf("Errore parametri errati\n");
        exit(1);
    }
    int N = argc - 2;
    if (argv[argc - 1][1] != 0)
    {
        exit(2);
    }
    char Cz = argv[argc - 1][0];

    pipe_t *ps = malloc(N * sizeof(pipe_t));
    pipe_t *sp = malloc(N * sizeof(pipe_t));

    for (int k = 0; k < N; ++k)
    {
        if (pipe(ps[k]) < 0 || pipe(sp[k]) < 0)
        {
            printf("Errore generazione pipe\n");
            exit(3);
        }
    }

    for (int i = 0; i < 2 * N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore generazione figlio n.%d\n", i);
            exit(4);
        }
        if (!pid)
        {
            if (i < N)
            {
                // sono nei primi N processi...
                for (int k = 0; k < N; ++k)
                {
                    if (i != k)
                    {
                        close(ps[k][1]);
                        close(sp[k][0]);
                    }
                    close(ps[k][0]);
                    close(sp[k][1]);
                }

                if ((fd = open(argv[1 + i], O_RDONLY)) < 0)
                {
                    printf("Errore: il file %s non è valido\n", argv[i + 1]);
                    exit(-2);
                }

                while (read(fd, &c, 1))
                {
                    if (c == Cz)
                    {
                        ++ritorno;
                        pos = lseek(fd, 0L, SEEK_CUR) - 1;
                        printf("Valore di pos per il figlio1 associato al file %s:%ld\n", pos, argv[i + 1]);
                        // la comunico al processo con cui è in coppia
                        if ((write(ps[i][1], &pos, sizeof(long int)) != sizeof(long int)))
                        {
                            printf("Errore scrittura della posizione nella pipe per il processo2\n");
                            exit(-1);
                        }
                        printf("Valore di pos inviato al figlio2 associato a %s:%ld\n", pos, argv[i + 1]);

                        // leggo la posizione della prossima occorrenza trovata dal secondo processo. se non c0è non leggo nulla dalla pipe>
                        if (read(sp[i][0], &posLetta, sizeof(long int)) != sizeof(long int))
                        {
                            break;
                        }

                        // sposto il puntatore e continuo la ricerca.
                        lseek(fd, posLetta + 1, SEEK_SET);
                    }
                }
                exit(ritorno);
            }
            else
            {
                // sono nei secondi N processi...
                for (int k = 0; k < N; ++k)
                {
                    if (2 * N - i - 1 != k)
                    {
                        close(ps[k][0]);
                        close(sp[k][1]);
                    }
                    close(ps[k][1]);
                    close(sp[k][0]);
                }

                if ((fd = open(argv[2 * N - i], O_RDONLY)) < 0)
                {
                    printf("Errore: il file %s non è valido\n", argv[2 * N - i]);
                    exit(-2);
                }

                read(sp[2 * N - i - 1][0], &posLetta, sizeof(long int));

                lseek(fd, posLetta + 1, SEEK_SET);

                while (read(fd, &c, 1))
                {

                    if (c == Cz)
                    {

                        ++ritorno;
                        pos = lseek(fd, 0L, SEEK_CUR) - 1;
                        printf("Valore di pos per il figlio2 associato al file %s:%ld\n", argv[2 * N - i], pos);

                        // la comunico al processo con cui è in coppia
                        if ((write(sp[2 * N - i - 1][1], &pos, sizeof(long int)) != sizeof(long int)))
                        {
                            printf("Errore scrittura della posizione nella pipe per il processo2\n");
                            exit(-1);
                        }
                        printf("Valore di pos inviato per il figlio1 associato al file %s:%ld\n", argv[i + 1], pos);

                        // leggo la posizione della prossima occorrenza trovata dal secondo processo. se non c0è non leggo nulla dalla pipe>
                        if (read(ps[2 * N - i - 1][0], &posLetta, sizeof(long int)) != sizeof(long int))
                        {
                            break;
                        }

                        // sposto il puntatore e continuo la ricerca.
                        lseek(fd, posLetta + 1, SEEK_SET);
                    }
                }
                exit(ritorno);
            }
        }
    }
    for (int k = 0; k < N; ++k)
    {

        close(ps[k][0]);
        close(sp[k][1]);

        close(ps[k][1]);
        close(sp[k][0]);
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
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%d.\n", pid, ritorno);
        }
    }

    exit(0);
}