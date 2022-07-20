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

long int contaoccorrenze(int fd, char ch)
{
    char c;
    long int ret = 0;
    while (read(fd, &c, 1))
    {
        /* code */
        if (ch == c)
        {
            ++ret;
        }
    }
    return ret;
}

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Mi aspetto almeno un parametro\n");
        exit(1);
    }
    int M = argc - 1;
    pipe_t *PF = malloc(M * sizeof(pipe_t));
    pipe_t *FP = malloc(M * sizeof(pipe_t));
    for (int i = 0; i < M; ++i)
    {
        if (pipe(PF[i]) < 0 || pipe(FP[i]) < 0)
        {
            exit(2);
        }
    }
    bool trovata = false;
    int pid, status, ritorno, fd;
    char c;
    long int occ = 0;
    for (int i = 0; i < M; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            exit(3);
        }
        if (pid == 0)
        {
            for (int k = 0; k < M; ++k)
            {
                close(PF[k][1]);
                close(FP[k][0]);
                if (i != k)
                {
                    close(FP[k][1]);
                }
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore file associato non valido (%s)\n", argv[i + 1]);
                exit(-2);
            }

            while (read(PF[i][0], &c, 1))
            {
                occ = contaoccorrenze(fd, c);
                if (occ > 0)
                {
                    trovata = 1;
                }
                write(FP[i][1], &occ, sizeof(long int));
                lseek(fd, 0L, SEEK_SET);
            }
            if (trovata)
            {
                exit(1);
            }
            exit(0);
        }
    }

    for (int i = 0; i < M; ++i)
    {
        close(PF[i][0]);
        close(FP[i][1]);
    }

    while (read(0, &c, 1))
    {

        // lo mando a tutti i figli
        for (int i = 0; i < M; ++i)
        {
            write(PF[i][1], &c, 1);
        }
        for (int k = 0; k < M; ++k)
        {
            read(FP[k][0], &occ, sizeof(long int));
            printf("Trovate %ld occorrenze per il carattere %c nel file %s\n", occ, c, argv[k + 1]);
        }
    }
    for (int i = 0; i < M; ++i)
    {
        close(PF[i][1]);
        close(FP[i][0]);
    }

    for (int k = 0; k < M; ++k)
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