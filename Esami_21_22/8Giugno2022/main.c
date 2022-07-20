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

#define MSGSIZE 72

// conto le posizioni a partire da 0;

typedef int pipe_t[2];

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Errore mi aspetto almeno 3 nomi di file\n");
        exit(1);
    }

    int pid, pidNipote, status, ritorno; // per gestire le wait e la creazione dei processi
    int fd;                              // file descriptor
    int N = argc - 1;                    // numero dei file e dei figli
    int n;                               // variabile per operare nei cicli
    char linea[MSGSIZE];                 // per mantenere la lettura degli n-1 figli rispetto al figlio 0
    char buffer[MSGSIZE];                // usata dal figlio 0 per leggere i nomi dei file da confrontare

    // il primo figlio comunica con tutti gli altri per ogni linea che legge.
    //  si comporta da produttore mentre gli altri n-1 figli da consumatori.

    pipe_t *pipes = malloc((N - 1) * sizeof(pipe_t));
    if (!pipes)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (int n = 0; n < N - 1; ++n)
    {
        if (pipe(pipes[n]) < 0)
        {
            printf("Errore: impossibile generare la pipe\n");
            exit(3);
        }
    }

    for (n = 0; n < N; ++n)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore: impossibile generare il figlio con indice %d\n", n);
            exit(4);
        }
        if (!pid)
        {
            printf("Sono il figlio con PID,%d e ordine %d associato al file \"%s\"\n", getpid(), n, argv[n + 1]);
            // apertura del suo file associato --> codice comune a tutti i figli
            if ((fd = open(argv[n + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file \"%s\" non è valido\n", argv[n + 1]);
                exit(-1);
            }

            if (n == 0)
            {
                // il primo figlio comunica con gli altri... si deve quindi realizzare come produttore
                // effettuo la chiusura corretta della pipe di nome pipes che permette la comunicazione.

                for (int k = 0; k < N - 1; ++k)
                {
                    close(pipes[k][0]); // comunica con tutti come produttore
                }

                while (read(fd, buffer, MSGSIZE * sizeof(char)) == MSGSIZE)
                {
                    // metto il terminatore
                    buffer[MSGSIZE - 1] = 0;
                    for (int i = 0; i < N - 1; ++i)
                    {
                        write(pipes[i][1], buffer, MSGSIZE);
                    }
                }
            }
            else
            {
                // sono un figlio n-1
                // chiusura lati pipes --> mi configuro come consumatore!
                for (int i = 0; i < N - 1; ++i)
                {
                    close(pipes[i][1]);
                    if (i != n - 1)
                    {
                        close(pipes[i][0]);
                    }
                }
                while (read(pipes[n - 1][0], buffer, MSGSIZE) == MSGSIZE)
                {

                    // leggo dal mio file associato un file.

                    while (read(fd, linea, MSGSIZE) == MSGSIZE)
                    {

                        linea[MSGSIZE - 1] = 0;
                        // devo fare la diff
                        printf("Letto dal file %s la stringa %s\n", argv[n + 1], linea);
                        pidNipote = fork();
                        if (pidNipote < 0)
                        {
                            printf("Impossibile creare il nipote %d\n", n);
                            exit(-1);
                        }
                        if (!pidNipote)
                        {
                            close(pipes[n - 1][0]);
                            close(2);
                            open("/dev/null", O_WRONLY);
                            close(1);
                            open("/dev/null", O_WRONLY);

                            execlp("diff", "diff", linea, buffer, (char *)0);
                            perror("Se vedi questa linea l'execlp non è andata a buon fine");
                            exit(2);
                        }

                        if ((pidNipote = wait(&status)) < 0)
                        {
                            printf("Errore WAIT\n");
                            exit(-1);
                        }
                        if ((status & 0xff) != 0)
                        {
                            printf("Il figlio con pid.%d è terminato in modo anomalo con status:%d.\n", pidNipote, status & 0xff);
                        }
                        else
                        {
                            ritorno = (int)(status >> 8);
                            ritorno &= 0xff;
                        }
                        if (ritorno == 0)
                        {
                            // i file sono uguali!
                            printf("Il file %s e %s sono uguali!\n", linea, buffer);
                        }
                    }
                    lseek(fd, 0L, SEEK_SET);
                }
            }
            exit(n);
        }
    }

    for (n = 0; n < N - 1; ++n)
    {
        close(pipes[n][0]);
        close(pipes[n][1]);
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