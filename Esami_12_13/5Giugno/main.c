#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
char linea[255];
typedef int pipe_t[2];
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Errore: numero dei parametri errato!\n");
        exit(1);
    }
    int N = argc - 1;
    printf("Il numero di figli da generare è pari a %d\n", N);

    pipe_t *NP = malloc(N * sizeof(pipe_t));
    if (!NP)
        exit(2);

    int pid, status, ritorno, fd, pidn;

    for (int i = 0; i < N; ++i)
    {
        if (pipe(NP[i]) < 0)
        {
            exit(3);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            exit(4);
        }
        if (!pid)
        {
            // CODICE FIGLIO
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: impossibile aprire il file di nome %s", argv[i + 1]);
                exit(-1);
            }

            pidn = fork();
            if (pidn < 0)
            {
                exit(-1);
            }
            if (pidn == 0)
            {
                for (int j = 0; j < N; ++j)
                {
                    if (i != j)
                    {
                        close(NP[j][1]);
                    }
                    close(NP[j][0]);
                }

                close(0);
                dup(fd);
                close(1);
                dup(NP[i][1]);
                execlp("head", "head", "-1", (char *)0);
                perror("Se vedi questo avviso l'head non è andata a buon fine");
                exit(-2);
            }

            for (int j = 0; j < N; ++j)
            {
                close(NP[j][1]);
                close(NP[j][0]);
            }

            sleep(1);

            if ((pidn = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(-1);
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

            exit(0);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(NP[i][1]);
    }
    int pos = 0;
    for (int i = 0; i < N; ++i)
    {
        while (read(NP[i][0], linea + pos, 1))
        {
            ++pos;
        }
        linea[pos - 1] = 0;
        printf("Letta  prima linea del file %s: \"%s\"\n", argv[i + 1], linea);
        pos = 0;
    }

    sleep(1);
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