
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
char opzione[5];
typedef int pipe_t[2];

typedef struct
{
    int c1;       // pid nipote
    int c2;       // numero della linea
    char c3[250]; // linea
} S;

int mia_random(int n)
{
    int casuale;
    casuale = rand() % n;
    casuale++;
    return casuale;
}

bool ok(bool *presente, int N)
{
    for (int j = 0; j < N; ++j)
    {
        if (!presente[j])
        {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv)
{

    int pid, pidNipote, status, ritorno, random;
    int fd;
    int num = 0;
    int pos = 0;
    S curr;

    if (argc < 3)
    {
        printf("Errore numero errato di parametri\n");
        exit(1);
    }

    int N = argc - 1;
    N /= 2;
    printf("Il numero dei file è %d\n", N);
    printf("Il numero dei figli da generare è %d\n", N);

    // controllo numeri positivi.

    for (int i = 0; i < N; i += 2)
    {
        if (atoi(argv[i + 1]) <= 0)
        {
            printf("Errore %s non numerico positivo\n", argv[i + 1]);
            exit(2);
        }
    }

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    bool *presente = calloc(N, sizeof(bool));
    if (!presente || !FP)
    {
        printf("Errore: impossibile allocare memoria\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            printf("Errore: impossibile generare la pipe\n");
            exit(4);
        }
    }

    for (int i = 1; i < 2 * N; i += 2)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore impossibile generare il figlio n.%d\n", i);
            exit(5);
        }
        if (!pid)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if ((i / 2) != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            pipe_t NF;
            if (pipe(NF) < 0)
            {
                exit(-1);
            }

            pidNipote = fork();
            if (pidNipote < 0)
            {

                exit(-1);
            }
            if (!pidNipote)
            {
                srand(time(NULL));
                ritorno = mia_random(atoi(argv[i]));
                // printf("random numero --> %d per il file %s\n", ritorno, argv[i + 1]);

                close(NF[0]);
                write(NF[1], &ritorno, sizeof(int));
                // codice nipote
                printf("Sono il nipote (PID.%d) figlio del processo con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), getppid(), i, argv[i + 1]);
                close(FP[i / 2][1]);
                close(0);
                if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: impossibile aprire il file associato\n");
                    exit(-1);
                }
                close(1);
                dup(NF[1]);
                close(NF[1]);

                // calcolo il numero random...
                sprintf(opzione, "-%d", ritorno);
                printf("%s", opzione);
                execlp("head", "head", opzione, (char *)0);
                perror("Se vedi questo messaggio l'head non è andato a buon fine");
                exit(-2);
            }

            // codice figlio
            close(NF[1]);
            read(NF[0], &random, sizeof(int)); // quello da ritornare.

            // ci serve adesso per ogni linea la struttura da mandare al padre
            curr.c1 = pidNipote;
            curr.c2 = num + 1;
            pos = 0;
            while (read(NF[0], curr.c3 + pos, 1))
            {
                /* code */
                if (curr.c3[pos] == 10)
                {
                    // fine linea
                    curr.c3[pos] = 0;
                    // printf("Sto per mandare la struttura PID_NIPOTE:%d\nLINEA:%d \"%s\"\n", curr.c1, curr.c2, curr.c3);
                    write(FP[i / 2][1], &curr, sizeof(S));
                    pos = 0;
                    ++curr.c2;
                }
                else
                {
                    pos++;
                }
            }

            // faccio la wait del figlio
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
            exit(random);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    while (ok(presente, N))
    {

        for (int i = 0; i < N; ++i)
        {

            if (!presente[i])
            {
                if (read(FP[i][0], &curr, sizeof(curr)) != sizeof(S))
                {
                    presente[i] = 1;
                }
                else
                {
                    printf("Ricevuta struttura dal figlio %i\nPID_NIPOTE:%d\nLINEA:%d \"%s\"\n", i, curr.c1, curr.c2, curr.c3);
                }
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
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%d.\n", pid, ritorno);
        }
    }
    exit(0);
}