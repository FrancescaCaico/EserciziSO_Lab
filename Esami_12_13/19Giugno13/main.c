#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

typedef int pipe_t[2];

typedef struct
{
    int c1; // PID
    int c2; // NUMERO INTERO
} info;

int main(int argc, char **argv)
{

    if (argc < 1)
    {
        printf("Errore numero parametri\n");
        exit(1);
    }

    int N = argc - 1;
    printf("Il numero di figli da generare è %d\n", N);

    pipe_t *pipeline = malloc(N * sizeof(pipe_t));
    if (!pipeline)
    {
        exit(2);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(pipeline[i]) < 0)
        {
            exit(3);
        }
    }
    info prev;
    info curr;
    int pid, status, ritorno;
    int fd;
    char c;
    for (int i = N - 1; i >= 0; --i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore creazione figlio %d.\n", i);
            exit(4);
        }
        if (pid == 0)
        {
            for (int j = N - 1; j >= 0; --j)
            {
                if (j != i)
                {
                    close(pipeline[j][1]);
                }
                if ((i == N - 1) || (j != i + 1))
                {
                    close(pipeline[j][0]);
                }
            }

            printf("Sono il figlio num.%d con PID.%d associato al file %s\n", i, getpid(), argv[i + 1]);

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                exit(-1);
            }

            curr.c1 = getpid();

            while (read(fd, &c, 1))
            {
                c -= 0; // in numero positivo
                curr.c2 = c;
                if (i != N - 1)
                {

                    read(pipeline[i + 1][0], &prev, sizeof(info));
                    if (prev.c2 > c)
                    {
                        curr = prev;
                    }
                }

                write(pipeline[i][1], &curr, sizeof(curr));
            }
            exit(c);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if (i != 0)
        {
            close(pipeline[i][0]);
        }
        close(pipeline[i][1]);
    }
    int num = 0;
    while (read(pipeline[0][0], &curr, sizeof(curr)) == sizeof(curr))
    {
        ++num;
        printf("Ricevuta struttura num.%d dal figlio P0 pari a :\n\t-PID:%d\n\t-NUMERO:%d (%c)\n", num, curr.c1, curr.c2, curr.c2);
    }

    for (int k = 0; k < N; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(9);
        }
        if ((status & 0xff) != 0)
        {
            printf("Il figlio con pid.%d è terminato in modo anomalo con status :%d\n", pid, status & 0xff);
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