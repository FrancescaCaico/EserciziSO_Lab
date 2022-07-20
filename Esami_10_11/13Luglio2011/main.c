// La parte in C accetta un numero variabile N + 1 di parametri che rappresentano i primi N nomi di file (F0, F1, ... FN-
// 1), mentre l’ultimo rappresenta un singolo carattere (C) (da controllare).
// Il processo padre deve generare 2 * N processi figli (P0 ... P2 * N-1) e i processi figli vanno considerati a coppie,
// ognuna delle quali è associata ad uno dei file Fi. In particolare, la prima coppia è costituita dal processo P0 e dal
// processo P2*N-1, la seconda dal processo P1 e dal processo P2*N-2 e così via fino alla coppia costituita dal processo
// PN-1 e dal processo PN: in generale una coppia è costituita dal processo Pi (con i che varia da 0 a ... N-1) e dal
// processo P2*N-1-i. Entrambi i processi della coppia devono cercare il carattere C nel file associato Fi sempre
// fino alla fine attuando una sorta di staffetta così come illustrato nel seguito. Il processo Pi deve cominciare a leggere
// cercando la prima occorrenza del carattere C; appena trovata deve comunicare all’altro processo della coppia P2*N-1-i
// la posizione del carattere trovato all’interno del file (in termini di long int); quindi il processo P2*N-1-i deve partire
// con la sua ricerca del carattere C dalla posizione seguente a quella ricevuta; appena trovata una nuova occorrenza
// di C deve comunicare all’altro processo della coppia Pi la posizione del carattere trovato all’interno del file (in termini
// di long int); tale staffetta deve avere termine quando il file è finito.
// Al termine, ogni processo figlio deve ritornare al padre il numero di occorrenze del carattere C trovate dal singolo
// processo della coppia e il padre deve stampare su standard output il PID di ogni figlio e il valore ritornato.

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <limits.h>

typedef int pipe_t[2];

int main(int argc, char **argv)
{

    int i, j, k;
    int pid, ritorno, status;
    int fd;
    int occ = 0;
    long int pos;
    char c;

    // assumiamo che ci sia almeno un file
    if (argc < 3)
    {
        printf("Numero dei parametri errato, almeno un file e un carattere singolo.\n");
        exit(1);
    }
    if (argv[argc - 1][1] != 0)
    {
        printf("L'ultimo carattere non è SINGOLO.\n");
        exit(2);
    }
    char Cx = argv[argc - 1][0];

    int N = argc - 2;
    printf("Il numero di coppie da formare è --> %d\n", N);

    pipe_t *ps = malloc(N * sizeof(pipe_t));
    pipe_t *sp = malloc(N * sizeof(pipe_t));

    if (ps == NULL || sp == NULL)
    {
        exit(3);
    }

    for (i = 0; i < N; ++i)
    {
        if (pipe(ps[i]) < 0 || pipe(sp[i]) < 0)
        {
            exit(4);
        }
    }

    for (i = 0; i < 2 * N; ++i)
    {

        if ((pid = fork()) < 0)
        {
            exit(5);
        }
        if (pid == 0)
        {
            if (i < N)
            {
                printf("Sono il figlio con PID.%d e indice %d associato al file %s.\n", getpid(), i, argv[i + 1]);

                for (k = 0; k < N; ++k)
                {
                    if (i != k)
                    {
                        close(ps[k][1]);
                        close(sp[k][0]);
                    }
                    close(ps[k][0]);
                    close(sp[k][1]);
                }
                /*apriamo il file associato*/
                if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                    exit(-1);
                }
                while (read(fd, &c, 1))
                {
                    if (c == Cx)
                    {
                        ++occ;
                        // mando la posizione al primo processo
                        write(ps[i][1], &pos, sizeof(long int));
                        read(sp[i][0], &pos, sizeof(long int));
                        lseek(fd, pos, SEEK_SET);
                    }
                }
            }
            else
            {
                j = 2 * N - i - 1;
                printf("Sono il figlio con PID.%d e indice %d associato al file %s.\n", getpid(), i, argv[j + 1]);
                printf("PID --> %d : %d\n", getpid(), j);
                for (k = 0; k < N; ++k)
                {
                    if (j != k)
                    {
                        close(ps[k][0]);
                        close(sp[k][1]);
                    }
                    close(ps[k][1]);
                    close(sp[k][0]);
                }
                /*apriamo il file associato*/
                if ((fd = open(argv[j + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                    exit(-1);
                }
                // leggo la prima posizione mandata dal processo 1;

                read(ps[j][0], &pos, sizeof(long int));
                lseek(fd, pos, SEEK_SET);
                while (read(fd, &c, 1))
                {
                    if (c == Cx)
                    {
                        ++occ;
                        // mando la posizione al primo processo
                        write(sp[j][1], &pos, sizeof(long int));
                        read(ps[j][0], &pos, sizeof(long int));
                        lseek(fd, pos, SEEK_SET);
                    }
                }
            }
            exit(occ);
        }
    }

    // padre
    for (k = 0; k < N; ++k)
    {

        close(ps[k][0]);
        close(sp[k][1]);
        close(ps[k][1]);
        close(sp[k][0]);
    }
    // wait figli
    sleep(1);
    for (k = 0; k < N; ++k)
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