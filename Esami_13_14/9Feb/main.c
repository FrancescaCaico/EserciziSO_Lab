// 9 FEBBRAIO 2014
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

char opzione[255];
char linea[255];
int main(int argc, char **argv)
{

    if (argc < 5)
    {
        printf("Errore numero dei parametri errato.\n");
        exit(1);
    }

    int N = argc - 1;
    N /= 2;
    int pos = 0;
    int l = 0;
    printf("Il numero di processi da creare è : %d\n", N);

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (!FP)
    {
        printf("Errore allocazione di memoria\n");
        exit(2);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            exit(3);
        }
    }

    int pid, status, ritorno, fd, pidn;

    for (int i = 0; i < 2 * N; i += 2)
    {
        if ((pid = fork()) < 0)
        {
            exit(4);
        }
        if (pid == 0)
        {
            // codice figlio
            pipe_t NF;
            if (pipe(NF) < 0)
            {
                printf("Errore impossibile creare NF\n");
                exit(-1);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file '%s' non è valido.\n", argv[i + 1]);
                exit(-1);
            }

            pidn = fork();
            if (pidn < 0)
            {
                printf("Errore: impossibile generare il nipote\n");
                exit(-2);
            }
            if (pidn == 0)
            {
                // codice nipote

                for (int j = 0; j < N; ++j)
                {
                    close(FP[j][0]);
                    close(FP[j][1]);
                }

                close(NF[0]);
                close(0);
                dup(fd);
                close(1);
                dup(NF[1]);
                sprintf(opzione, "-%s", argv[i + 2]);
                execlp("head", "head", opzione, (char *)0);
                perror("Se vedi questo avviso la head non è andata a buon fine.\n");
                exit(-2);
            }

            for (int j = 0; j < N; ++j)
            {
                close(FP[j][0]);
                if ((i / 2) != j)
                    close(FP[j][1]);
            }

            close(NF[1]);

            while (read(NF[0], linea + pos, 1))
            {
                if (linea[pos] == 10)
                {
                    ++l;
                    if (l == atoi(argv[i + 2]))
                    {
                        // sono nella linea corretta
                        linea[pos] = 0;
                        write(FP[i / 2][1], linea, pos * sizeof(char));
                        break;
                    }
                    pos = 0;
                }
                else
                {
                    ++pos;
                }
            }
            if ((pidn = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
            }

            if ((status & 0xff) != 0)
            {
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato in modo anomalo con status :%d\n", pidn, getppid(), status & 0xff);
            }
            else
            {
                ritorno = (int)(status >> 8);
                ritorno &= 0xff;
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato correttamente con valore di ritorno:%d\n", pidn, getpid(), ritorno);
            }
            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    sleep(1);
    for (int i = 0; i < 2 * N; i += 2)
    {
        printf("La prossima linea è riferita al file %s ed è la numero -> %s\n", argv[i + 1], argv[i + 2]);
        while (read(FP[i / 2][0], linea + pos, 1))
        {
            ++pos;
        }
        linea[pos - 1] = 0;
        printf("\"%s\"\n", linea);
        pos = 0;
    }

    sleep(1);
    for (int k = 0; k < N; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(8);
        }
        if ((status & 0xff) != 0)
        {
            printf("Il figlio con pid.%d è terminato in modo anomalo con status :%d.\n", pid, status & 0xff);
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