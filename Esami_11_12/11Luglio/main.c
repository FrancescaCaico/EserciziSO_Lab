#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

const char *media[12] = {"Sopra media", "Sotto media", "Equal media"};
char linea[255];
typedef int pipe_t[2];

int main(int argc, char **argv)
{

    if (argc < 5)
    {
        printf("Errore numero di parametri errato.\n");
        exit(1);
    }

    int K = atoi(argv[argc - 1]);
    if (K <= 0)
    {
        printf("Errore %s non numerico positivo\n", argv[argc - 1]);
        exit(2);
    }

    char *nome = calloc(strlen(argv[argc - 1]) + 8, sizeof(char));
    if (nome == NULL)
    {
        exit(3);
    }

    strcpy(nome, "output.");
    strcat(nome, argv[argc - 1]);

    printf("Il nome del file da creare sarà --> %s\n", nome);

    int Fout = creat(nome, 0644);
    if (Fout < 0)
    {
        printf("Errore creazione del fine di nome %s fallita! :(\n", nome);
        exit(4);
    }

    int N = argc - 2;
    printf("Il numero di processi da generare è --> %d\n", N);

    int pid, status, ritorno, fd, L = 0;
    char c;

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    pipe_t *PF = malloc(N * sizeof(pipe_t));
    if (!FP || !PF)
    {
        exit(6);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0 || pipe(PF[i]) < 0)
        {
            printf("Errore generazione pipe fallita!\n");
            exit(7);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore: impossibile generare il figlio num %d\n", i);
            exit(5);
        }
        if (!pid)
        {
            // codice figlio
            for (int k = 0; k < N; ++k)
            {
                close(FP[k][0]);
                close(PF[k][1]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }

            while (read(fd, &c, 1))
            {
                ++L;
            }

            write(FP[i][1], &L, sizeof(L));
            read(PF[i][0], linea, 12 * sizeof(char));

            if (!strcmp(linea, media[0]))
            {
                ritorno = 2;
            }
            else if (!strcmp(linea, media[1]))
            {
                ritorno = 1;
            }
            else if (!strcmp(linea, media[2]))
            {
                ritorno = 0;
            }

            char *str = calloc(12 + strlen(argv[i + 1]), sizeof(char));
            lseek(Fout, 0L, SEEK_END);
            if (str == NULL)
            {
                printf("Errore malloc per stringa\n");
                exit(-1);
            }
            strcpy(str, argv[i + 1]);
            strcat(str, linea);
            write(Fout, str, strlen(str));
            exit(ritorno);
        }
    }

    // codice padre
    for (int i = 0; i < N; i++)
    {
        /* code */
        close(FP[i][1]);
        close(PF[i][0]);
    }

    for (int i = 0; i < N; ++i)
    {
        read(FP[i][0], &L, sizeof(L));
        if (L == K)
        {
            // equal media
            write(PF[i][1], media[2], 12 * sizeof(char));
        }
        else if (L < K)
        {
            // sotto media
            write(PF[i][1], media[1], 12 * sizeof(char));
        }
        else
        {
            // equal media
            write(PF[i][1], media[0], 12 * sizeof(char));
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