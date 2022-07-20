
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
char WC[255];

typedef struct
{
    int c1;      // indice del processo
    long int c2; // numero di occorrenze di Cx trovate nel file.
} info;

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("Errore: mi aspetto almeno 2 parametri\n");
        exit(1);
    }
    int N = argc - 1;
    printf("Il numero di figli da generare è %d\n", N);
    int pid, pidNipote, status, ritorno;
    int fd;
    int len;
    pipe_t *FP = calloc(N, sizeof(pipe_t));
    if (FP == NULL)
    {
        printf("Errore allocazione memoria fallita\n");
        exit(2);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; ++i)
    {

        pid = fork();
        if (pid < 0)
        {
            printf("Errore creazione figlio num.%d\n", i);
            exit(4);
        }
        if (pid == 0)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; k++)
            {
                /* code */
                if (i != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore file --> %s non valido...\n", argv[i + 1]);
                exit(-1);
            }
            pipe_t NF;
            if (pipe(NF) < 0)
            {
                printf("Errore creazione pipe nipote-figlio num.%d\n", i);
                exit(-1);
            }

            pidNipote = fork();
            if (pidNipote < 0)
            {
                printf("Errore creazione nipote del figlio num.%d\n", i);
                exit(-1);
            }
            if (pidNipote == 0)
            {

                close(0);
                dup(fd);
                close(1);
                dup(NF[1]);
                close(NF[1]);
                close(NF[0]);
                execlp("wc", "wc", "-l", (char *)0);
                perror("Se vedi questa linea la 'wc -l'  non è andata a buon fine");
                exit(-2);
            }

            // chiusura pipe
            close(NF[1]);
            int pos = 0;
            while (read(NF[0], WC + pos, 1))
            {
                ++pos;
            }
            WC[pos] = 0;
            // converto la stringa in numero

            len = atoi(WC);
            // la mando al padre
            write(FP[i][1], &len, sizeof(int));

            if ((pidNipote = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(-3);
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

    for (int k = 0; k < N; k++)
    {
        /* code */
        close(FP[k][1]);
    }
    int len_tot = 0;
    for (int k = 0; k < N; k++)
    {
        /* code */
        read(FP[k][0], &len, sizeof(int));
        len_tot += len;
    }

    printf("--> I file passati hanno in totale un numero di linee pari a %d. <--\n", len_tot);

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