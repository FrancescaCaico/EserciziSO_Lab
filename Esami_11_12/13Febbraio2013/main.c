#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>

// struttura
typedef struct
{
    int indice; // indice del file in cui sta l'occorrenza minima
    int occmin; // quale è stata l'occorrenza minima trovata in un file
    int occtot; // quali sono state le occorrenze totali
} info;

typedef int pipe_t[2];
int main(int argc, char **argv)
{

    int i, k, fd, pid, status, ritorno;
    char c;
    if (argc < 7)
    {
        printf("Errore: mi aspetto almeno 4 file e 2 caratteri singoli!\n");
        exit(1);
    }

    int N = argc - 3;
    // ci sono 2 pipeline
    // 1) tutti i primi N figli con il padre
    // 2) tutti gli altri N con il padre
    //  Le coppie non comunicano ma sono associate allo stesso file
    if ((argv[argc - 2][1] != 0) || (argv[argc - 1][1] != 0))
    {
        printf("Errore : %s oppure %s deve essere un carattere singolo.\n", argv[argc - 2], argv[argc - 1]);
        exit(2);
    }

    char C1 = argv[argc - 2][0]; // CARATTERE CERCATO DAI PRIMI
    char C2 = argv[argc - 1][0]; // CARATTERE CERCATO DAI SECONDI
    int occ_curr = 0;

    info message, curr;
    pipe_t *first = malloc(N * sizeof(pipe_t));
    pipe_t *second = malloc(N * sizeof(pipe_t));

    if (!first || !second)
    {
        printf("Errore allocazione memoria.\n");
        exit(3);
    }

    for (i = 0; i < N; ++i)
    {
        if (pipe(first[i]) < 0 || pipe(second[i]) < 0)
        {
            printf("Errore generazione PIPE.\n");
            exit(4);
        }
    }

    printf("Genererò %d figli\n", 2 * N);

    for (i = 0; i < 2 * N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore genetazione figlio n.%d\n", i);
            exit(5);
        }
        if (pid == 0)
        {
            // distiguiamo i due casi!
            curr.indice = i;
            curr.occtot = 0;
            if (i < N)
            {
                printf("Sono il figlio n.%d con PID.%d associato al file %s!\n", i, getpid(), argv[i + 1]);

                // chiusure pipe

                for (k = 0; k < N; ++k)
                {

                    close(second[k][0]);
                    close(second[k][1]);

                    if (i != k)
                    {
                        close(first[k][1]);
                    }
                    if ((i == 0) || (k != i - 1))
                    {
                        close(first[k][0]);
                    }
                }
                if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                    exit(-1);
                }

                while (read(fd, &c, 1))
                {
                    if (c == C1)
                    {
                        curr.occtot++;
                    }
                }
                printf("occorrenze di C1=%c in %s --> %d\n", C1, argv[i + 1], occ_curr);
                if (i != 0)
                {
                    read(first[i - 1][0], &message, sizeof(info));
                    if (curr.occtot < message.occmin)
                    {
                        message.occmin = curr.occtot;
                        message.indice = i;
                        message.occtot += curr.occtot;
                    }
                }
                else
                {
                    message.indice = curr.indice;
                    message.occmin = curr.occtot;
                    message.occtot += curr.occtot;
                }
                write(first[i][1], &message, sizeof(message));
                exit(C1);
            }
            else
            {
                printf("Sono il figlio n.%d con PID.%d associato al file %s!\n", i, getpid(), argv[i - N + 1]);
                for (k = 0; k < N; ++k)
                {

                    close(first[k][0]);
                    close(first[k][1]);

                    if (i - N != k)
                    {
                        close(second[k][1]);
                    }
                    if ((i == N) || (k != i - N - 1))
                    {
                        close(second[k][0]);
                    }
                }

                if ((fd = open(argv[i - N + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                    exit(-1);
                }
                while (read(fd, &c, 1))
                {
                    if (c == C2)
                    {
                        curr.occtot++;
                    }
                }
                printf("occorrenze di C2=%c in %s --> %d\n", C2, argv[i - N + 1], occ_curr);

                if (i != N)
                {
                    read(second[i - N - 1][0], &message, sizeof(info));
                    if (occ_curr <= message.occmin)
                    {
                        message.occmin = occ_curr;
                        message.indice = i;
                        message.occtot += occ_curr;
                    }
                }
                else
                {
                    message.indice = i;
                    message.occmin = occ_curr;
                    message.occtot = occ_curr;
                }
                printf("%d\n", i - N);
                write(second[i - N][1], &message, sizeof(message));
                exit(C2);
            }
        }
    }

    // padre
    for (i = 0; i < N; ++i)
    {
        if (i != N - 1)
        {
            close(second[i][0]);
            close(first[i][0]);
        }
        close(first[i][1]);
        close(second[i][1]);
    }

    sleep(1);
    read(first[N - 1][0], &message, sizeof(message));
    printf("La struttura dei primi N figli è la seguente:\nINDICE_MIN--> %d\nOCCORRENZE MINIMA:%d\nOCCORRENZE TOTALI : %d\n", message.indice, message.occmin, message.occtot);

    sleep(1);
    read(second[N - 1][0], &message, sizeof(message));
    printf("La struttura dei secondi N figli è la seguente:\nINDICE_MIN--> %d\nOCCORRENZE MINIMA:%d\nOCCORRENZE TOTALI : %d\n", message.indice, message.occmin, message.occtot);

    for (k = 0; k < 2 * N; ++k)
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
            printf("Il figlio con pid.%d è terminato correttamente con valore di ritorno:%c.\n", pid, ritorno);
        }
    }

    exit(0);
}