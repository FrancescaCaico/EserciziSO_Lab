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
    int pid, pidNipote, status, ritorno, fd; // variabili per definire  I processi
    int pos = 0;
    char c;
    if (argc < 3)
    {
        printf("Errore parametri\n");
        exit(1);
    }

    int M = argc - 1;
    printf("Si devono generare %d figli\n", M);

    pipe_t *FP = malloc(M * sizeof(pipe_t));
    if (!FP)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (size_t i = 0; i < M; i++)
    {
        /* code */
        if (pipe(FP[i]) < 0)
        {
            printf("Errore pipe\n");
            exit(4);
        }
    }

    for (int i = 0; i < M; i++)
    {
        /* code */
        pid = fork();

        if (pid < 0)
        {
            printf("Errore generazione figlio num.%d\n", i);
            exit(5);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("il file %s non è valido\n", argv[i + 1]);
                exit(-1);
            }

            for (int k = 0; k < M; ++k)
            {
                if (i != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            pipe_t NF;
            if (pipe(NF) < 0)
            {
                printf("Errore pipe nf\n");
                exit(-1);
            }

            pidNipote = fork();
            if (pidNipote < 0)
            {
                printf("Errore generazione del nipote del  figlio num.%d\n", i);
                exit(-2);
            }
            if (!pidNipote)
            {
                close(FP[i][1]);
                close(0);
                dup(fd);
                close(1);
                dup(NF[1]);
                close(NF[1]);
                close(NF[0]);

                execlp("tail", "tail", "-1", (char *)0);
                perror("Se vedi questo avviso la tail non è andata a buon fine!");
                exit(-3);
            }

            close(NF[1]);
            while (read(NF[0], &c, 1))
            {
                /* code */
                ++pos;
            }

            write(FP[i][1], &pos, sizeof(int));
            sleep(1);

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
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato correttamente con valore di ritorno:%d\n", pidNipote, getpid(), ritorno);
            }

            exit(ritorno);
        }
    }

    for (int i = 0; i < M; i++)
    {
        /* code */
        close(FP[i][1]);
    }

    for (int i = 0; i < M; ++i)
    {
        if (read(FP[i][0], &pos, sizeof(int)) == sizeof(int))
        {
            printf("Ho ricevuto la seguente lunghezza dal figlio associato al file %s per l'ultima linea:%d\n", argv[i + 1], pos);
        }
    }
    sleep(1);
    for (int k = 0; k < M; ++k)
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