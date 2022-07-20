
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

struct info
{
    int pid;
    long int occmax;
    long int occtotale;
};

int main(int argc, char **argv)
{

    if (argc < 5)
    {
        printf("Errore numero dei parametri errato!\n");
        exit(1);
    }

    int N = argc - 3;
    if (N < 2 || N % 2)
    {
        printf("2N non valido.\n");
        exit(2);
    }
    N /= 2;
    printf("Il numero totale dei figli da generare %d\n", N);
    // 2 pipeline
    pipe_t *FIRST = malloc(N * sizeof(pipe_t));
    pipe_t *SECOND = malloc(N * sizeof(pipe_t));

    if (!FIRST || !SECOND)
    {
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FIRST[i]) < 0 || pipe(SECOND[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(4);
        }
    }
    int pid, status, ritorno, fd;
    long int occ = 0;
    struct info prev, curr;
    char c;
    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore creazione figlio num %i\n", i);
            exit(5);
        }
        if (pid == 0)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);
            // chiusure pipe
            for (int k = 0; k < N; ++k)
            {
                close(SECOND[k][0]);
                close(SECOND[k][1]);

                if (k != i)
                {
                    close(FIRST[k][1]);
                }
                if (i == 0 || (k != i - 1))
                {
                    close(FIRST[k][0]);
                }
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore file %s non valido\n", argv[i + 1]);
                exit(-1);
            }

            curr.pid = getpid();

            while (read(fd, &c, 1))
            {
                if (c == argv[argc - 2][0])
                {
                    ++occ;
                }
            }

            printf("Occorrenze di %c- %ld\n", argv[argc - 2][0], occ);
            if (i == 0)
            {
                // se sono il primo devo solo scrivere nella prossima pipe
                curr.pid = getpid();
                curr.occmax = occ;
                curr.occtotale = occ;
                write(FIRST[i][1], &curr, sizeof(struct info));
            }
            else
            {
                read(FIRST[i - 1][0], &prev, sizeof(struct info));
                if (prev.occmax < occ)
                {
                    curr.pid = getpid();
                    curr.occmax = occ;
                    curr.occtotale += occ;
                    write(FIRST[i][1], &curr, sizeof(struct info));
                }
                else
                {
                    prev.occtotale += occ;
                    write(FIRST[i][1], &prev, sizeof(struct info));
                }
            }
            exit(i);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore creazione figlio num %i\n", i);
            exit(5);
        }
        if (pid == 0)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + N + 1]);
            // chiusure pipe
            for (int k = 0; k < N; ++k)
            {
                close(FIRST[k][0]);
                close(FIRST[k][1]);

                if (k != i)
                {
                    close(SECOND[k][1]);
                }
                if (i == 0 || (k != i - 1))
                {
                    close(SECOND[k][0]);
                }
            }

            if ((fd = open(argv[i + N + 1], O_RDONLY)) < 0)
            {
                printf("Errore file %s non valido\n", argv[i + 1]);
                exit(-1);
            }

            curr.pid = getpid();

            while (read(fd, &c, 1))
            {
                if (c == argv[argc - 1][0])
                {
                    ++occ;
                }
            }

            // printf("Occorrenze lette= %ld", occ);
            if (i == 0)
            {
                // se sono il primo devo solo scrivere nella prossima pipe
                curr.pid = getpid();
                curr.occmax = occ;
                curr.occtotale = occ;
                write(SECOND[i][1], &curr, sizeof(struct info));
            }
            else
            {
                read(SECOND[i - 1][0], &prev, sizeof(struct info));
                if (prev.occmax < occ)
                {
                    curr.pid = getpid();
                    curr.occmax = occ;
                    curr.occtotale += occ;
                    write(SECOND[i][1], &curr, sizeof(struct info));
                }
                else
                {
                    prev.occtotale += occ;
                    write(SECOND[i][1], &prev, sizeof(struct info));
                }
            }
            exit(2 * i);
        }
    }

    // padre
    for (int i = 0; i < N; ++i)
    {
        if (i != N - 1)
        {
            close(FIRST[i][0]);
            close(SECOND[i][0]);
        }
        close(FIRST[i][1]);
        close(SECOND[i][1]);
    }

    // tutti i primi N

    if (read(FIRST[N - 1][0], &curr, sizeof(struct info)) == sizeof(struct info))
    {
        //  printf("Lettura ok! \n");
        printf("Ricevuta struttura dai primi N figli che cercavano il carattere %c:\n", argv[argc - 2][0]);
        printf("PID:%d; OCCMAX=%ld; OCCTOT=%ld\n", curr.pid, curr.occmax, curr.occtotale);
    }
    else
    {
        // printf("Lettura ko!\n");
    }

    if (read(SECOND[N - 1][0], &curr, sizeof(struct info)) == sizeof(struct info))
    {
        printf("Ricevuta struttura dai secondi N figli che cercavano il carattere %c:\n", argv[argc - 1][0]);
        printf("PID:%d; OCCMAX=%ld; OCCTOT=%ld\n", curr.pid, curr.occmax, curr.occtotale);
    }
    else
    {
        // printf("Lettura ko!\n");
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