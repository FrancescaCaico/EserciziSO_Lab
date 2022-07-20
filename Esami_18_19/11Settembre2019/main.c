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

#define PERM 0644
char nomefile[255];
typedef int pipe_t[2];

int main(int argc, char **argv)
{

    int pid, status, ritorno;
    int fd;
    char cp, cd;
    int Fcreato;

    if (argc < 3)
    {
        printf("Errore: numero dei parametri errato\n");
        exit(1);
    }
    int N = argc - 1;
    if (N < 2 || N % 2)
    {
        printf("Errore i parametri passati devono essere pari e maggiori o uguali a 2");
        exit(2);
    }

    pipe_t *paridispari = malloc((N / 2) * sizeof(pipe_t));
    if (paridispari == NULL)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (int k = 0; k < N / 2; ++k)
    {
        if (pipe(paridispari[k]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(4);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
        }
        if (!pid)
        {

            printf("Sono il figlio num.%d e PID.%d associato al file \"%s\"\n", i, getpid(), argv[i + 1]);
            // apertura del file associato
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore --> il file di nome \"%s\" non è valido\n", argv[i + 1]);
                exit(-1);
            }

            if (i % 2)
            {
                // se sono il figlio dispari --> consumo
                for (int k = 0; k < N / 2; ++k)
                {
                    if (i / 2 != k)
                    {
                        close(paridispari[k][0]);
                    }
                    close(paridispari[k][1]);
                }

                // per prima cosa devo creare il file Fcreato
                char *nomefile = calloc((strlen(argv[i + 1]) + 10), sizeof(char));
                if (nomefile == NULL)
                {
                    printf("Errore calloc per il nome del file\n");
                    exit(-1);
                }
                sprintf(nomefile, "%s.MAGGIORE", argv[i + 1]);
                Fcreato = creat(nomefile, PERM);

                while (read(fd, &cd, 1))
                {
                    /* code */
                    read(paridispari[i / 2][0], &cp, 1);
                    if (cp > cd)
                    {
                        write(Fcreato, &cp, 1);
                    }
                    else
                    {
                        write(Fcreato, &cd, 1);
                    }
                }
            }
            else
            {

                // se sono il figlio pari --> produco

                for (int k = 0; k < N / 2; ++k)
                {
                    if (i / 2 != k)
                    {
                        close(paridispari[k][1]);
                    }
                    close(paridispari[k][0]);
                }
                while (read(fd, &cp, 1))
                {
                    /* code */
                    write(paridispari[i / 2][1], &cp, 1);
                }
            }
            int pos = lseek(fd, 0L, SEEK_CUR);
            exit(pos);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(paridispari[i][0]);
        close(paridispari[i][1]);
    }

    for (int k = 0; k < N; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(5);
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