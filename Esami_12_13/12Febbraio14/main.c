#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

char nomefile[255];

void go()
{
    return;
}

typedef int pipe_t[2];

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Errore parametri errati\n");
        exit(1);
    }

    int N = argc - 1;
    printf("DEBUG - Il numero di figli da generare è %d\n", N);
    int pid, pidn, status, ritorno, fd;
    char c;
    pipe_t *FP = malloc(N * sizeof(pipe_t));
    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            exit(2);
        }
    }

    int pos = 0;
    long int finish = 0;
    for (int i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            exit(3);
        }
        if (pid == 0)
        {
            // codice figlio

            // dobbiamo creare il file in cui scrivere.
            sprintf(nomefile, "inverso%d", i);
            printf("Lunghezza nome file %s --> %zu\n", nomefile, strlen(nomefile));
            int Fout = creat(nomefile, 0644);
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }
            finish = lseek(fd, 0L, SEEK_END);
            pidn = fork();
            if (pidn < 0)
            {
                printf("Errore creazione nipote\n");
                exit(-1);
            }
            if (pidn == 0)
            {
                // CODICE NIPOTE
                for (int k = 0; k < len; ++k)
                {
                    close(FP[k][0]);
                    close(FP[k][1]);
                }

                for (int j = 0; j < finish - finish / 2; ++j)
                {
                    lseek(fd, finish - pos - 1, SEEK_SET);
                    read(fd, &c, 1);
                    write(Fout, &c, sizeof(char));
                    ++pos;
                }
                // finito;

                exit(pos);
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

            for (int k = 0; k < len; ++k)
            {
                close(FP[k][0]);
            }

            lseek(fd, finish / 2 - 1, SEEK_SET);
            lseek(Fout, 0L, SEEK_END);

            printf("CIAO ARRIVATO SEGNALE PER CONTINUARE A SCRIVERE\n");
            for (int j = 0; j < finish / 2; ++j)
            {
                read(fd, &c, 1);
                write(Fout, &c, sizeof(char));
                ++pos;
                lseek(fd, -2L, SEEK_CUR);
            }
            write(FP[i][1], nomefile, strlen(nomefile));
            exit(pos);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    for (int i = 0; i < N; ++i)
    {
        pos = 0;
        while (read(FP[i][0], nomefile + pos, 1))
        {
            ++pos;
        }
        nomefile[pos] = 0;
        printf("Il file %s è stato completato\n", nomefile);
        fd = open(nomefile, O_RDONLY);
        while (read(fd, &c, 1))
        {
            write(1, &c, 1);
        }

        puts("");
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