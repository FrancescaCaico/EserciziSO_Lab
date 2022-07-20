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

int main(int argc, char **argv)
{
    int pid, status, ritorno, fd, pidn;
    char c, ch, uguali;
    int N = argc - 1; // NUMERO DI FILE
    if ((N % 2) != 0 || N < 2)
    {
        printf("Errore numero di parametri errato.\n");
        exit(1);
    }
    int len = N / 2; // LUNGHEZZA DEI FILE E NUMERO DEI FIGLI

    printf("Il numero di processi figli da generare è %d\n", len);

    pipe_t *FP = malloc(len * sizeof(pipe_t));

    for (int i = 0; i < len; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            exit(2);
        }
    }

    for (int j = 0; j < len; ++j)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Impossibile generare figlio num-%d\n", j);
        }
        if (pid == 0)
        {
            pipe_t NF;
            if (pipe(NF) < 0)
            {
                exit(-1);
            }
            pidn = fork();
            if (pidn < 0)
            {
                printf("Errore creazione nipote num j=%d", j);
                exit(-2);
            }
            if (pidn == 0)
            {
                // codice nipote

                for (int i = 0; i < len; ++i)
                {
                    close(FP[i][0]);
                    close(FP[i][1]);
                }
                close(NF[0]);
                // legge il carattere j-esimo dalla fine del file associato
                if ((fd = open(argv[len + j + 1], O_RDONLY)) < 0)
                {
                    printf("Errore file associato al nipote di %d non valido (%s)\n", j, argv[len + j + 1]);
                    exit(-2);
                }

                long int pos = lseek(fd, 0L, SEEK_END);
                lseek(fd, 0L, SEEK_SET);
                lseek(fd, pos - j - 1, SEEK_SET);

                int nr = read(fd, &c, 1);
                printf("Carattere letto (nipote)--> %c\n", c);
                if (nr != 1)
                {
                    exit(-1);
                }
                else
                {
                    write(NF[1], &c, 1);
                    exit(0);
                }
            }

            for (int i = 0; i < len; ++i)
            {
                close(FP[i][0]);
                if (i != j)
                {
                    close(FP[i][1]);
                }
            }

            close(NF[1]);
            if ((fd = open(argv[j + 1], O_RDONLY)) < 0)
            {
                printf("Errore file associato al nipote di %d non valido (%s)\n", j, argv[j + 1]);
                exit(-1);
            }
            lseek(fd, j, SEEK_SET);
            int nr = read(fd, &c, 1);
            printf("Carattere letto (figlio)--> %c\n", c);

            if (nr != 1)
            {
                exit(-1);
            }
            else
            {
                read(NF[0], &ch, 1);
                if (ch == c)
                {
                    printf("Sono uguali\n");
                    uguali = ch;
                    // scriviamo sulla pipe del padre
                }
                else
                {
                    printf("Sono diversi\n");
                    uguali = -1;
                    printf("UGUALI --> %c\n", uguali);
                }
                if ((write(FP[j][1], &uguali, sizeof(char))) != sizeof(char))
                {
                    printf("Errore Write\n");
                }
            }
            if ((pidn = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(8);
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

    // codice padre
    for (int i = 0; i < len; ++i)
    {
        close(FP[i][1]);
    }

    for (int i = 0; i < len; ++i)
    {

        if (read(FP[i][0], &c, sizeof(char)))
        {
            if (c == -1)
            {
                printf("Nel file numero.%d di nome %s il carattere in posizione num.%d dall'inizio non è lo stesso rispetto alla fine\n", i, argv[i + 1], i);
            }
            else
            {
                printf("Nel file numero.%d di nome %s il carattere in posizione num.%d --> %c dall'inizio è lo stesso rispetto alla fine\n", i, argv[i + 1], i, c);
            }
        }
    }
    for (int k = 0; k < len; ++k)
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