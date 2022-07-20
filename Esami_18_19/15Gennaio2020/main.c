
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
char nomefile[10];
typedef int pipe_t[2];
int main(int argc, char **argv)
{
    int pid, pidNipote, status, ritorno;
    int fd, Fout;
    int ok;
    char c;

    if (argc < 3)
    {
        printf("Errore: mi aspetto almeno 2 nomi di file\n");
        exit(1);
    }

    int N = argc - 1;
    if (N % 2)
    {
        printf("Errore: ho bisogno di un numero di file pari per poter compiere il mio lavoro.\n");
        exit(2);
    }

    printf("Il numero di figli da generare è %d\n", N / 2);

    for (int i = 0; i < N / 2; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore creazione figlio n.%i\n", i);
            exit(3);
        }
        if (pid == 0)
        {

            pipe_t FiglioNipote; // per la comunicazione.
            pipe_t NipoteFiglio; // per la comunicazione

            if (pipe(FiglioNipote) < 0 || pipe(NipoteFiglio) < 0)
            {
                exit(-1);
            }
            sprintf(nomefile, "merge%d", i);
            if ((Fout = creat(nomefile, 0644)) < 0)
            {
                printf("Impossibile creare il file di nome %s\n", nomefile);
                exit(-1);
            }

            pidNipote = fork();
            if (pidNipote < 0)
            {
                printf("Errore creazione nipote del figlio n.%i\n", i);
                exit(3);
            }
            if (pidNipote == 0)
            {
                // chiusura dei suoi lati
                close(FiglioNipote[1]);
                close(NipoteFiglio[0]);

                if ((fd = open(argv[N / 2 + 1 + i], O_RDONLY)) < 0)
                {
                    printf("Errore: il file %s non è valido\n", argv[N / 2 + 1 + i]);
                    exit(-2);
                }

                while (read(fd, &c, 1))
                {
                    if (read(FiglioNipote[0], &ok, sizeof(int)))
                    {
                        // lo posso scrivere sul file
                        write(Fout, &c, 1);
                        write(NipoteFiglio[1], &ok, sizeof(int));
                    }
                }
                exit(c);
            }
            close(FiglioNipote[0]);
            close(NipoteFiglio[1]);

            if ((fd = open(argv[1 + i], O_RDONLY)) < 0)
            {
                printf("Errore: il file %s non è valido\n", argv[N / 2 + 1 + i]);
                exit(-2);
            }
            read(fd, &c, 1);
            write(Fout, &c, 1);
            write(FiglioNipote[1], &ok, sizeof(int));

            while (read(fd, &c, 1))
            {
                if (read(NipoteFiglio[0], &ok, sizeof(int)))
                {
                    // lo posso scrivere sul file
                    write(Fout, &c, 1);
                    write(FiglioNipote[1], &ok, sizeof(int));
                }
            }
            if ((pidNipote = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(-1);
            }

            if ((status & 0xff) != 0)
            {
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato in modo anomalo con status :%d\n", pidNipote, getppid(), status & 0xff);
            }
            else
            {
                ritorno = (int)(status >> 8);
                ritorno &= 0xff;
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato correttamente con valore di ritorno:%c\n", pidNipote, getpid(), ritorno);
            }

            exit(ritorno);
        }
    }

    for (int k = 0; k < N / 2; ++k)
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
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%c.\n", pid, ritorno);
        }
    }
    exit(0);
}