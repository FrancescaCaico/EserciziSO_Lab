#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>

char linea[255];
int count = 0;
typedef int pipe_t[2];

void stampa()
{
    printf("Il figlio con PID.%d ha ricevuto il segnale di stampa per la linea:\n\"%s\"\n", getpid(), linea);
    ++count;
    return;
}

void non_stampa()
{
    return;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Errore: il numero dei parametri è errato! almeno 2 file e un numero positivo.\n");
        exit(1);
    }

    int M = argc - 2; /*numero di files = numero di figli*/
    int H;            /*per contenere il numero di linee degli M files*/
    if ((H = atoi(argv[argc - 1])) <= 0)
    {
        printf("Errore: H = %s non numerico strettamente positivo.\n", argv[argc - 1]);
        exit(2);
    }

    /*il padre genera M figli*/
    pipe_t *FP = malloc(M * sizeof(FP));
    if (FP == NULL)
    {
        printf("Errore: allocazione di memoria fallita.\n");
        exit(3);
    }

    /*GENERAZIONE PIPE*/
    int i, k; /*per ciclare i for*/

    for (i = 0; i < M; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            printf("Errore generazione pipe n.%i\n", i);
            exit(4);
        }
    }

    int pos = 0;
    int pid, status, ritorno; /*variabili per gestire i figli*/
    int *pids = malloc(M * sizeof(int));
    int fd; /*file descriptor*/
    for (i = 0; i < M; ++i)
    {
        if ((pids[i] = fork()) < 0)
        {
            printf("Errore: impossibile creare il figlio n.%i\n", i);
            exit(5);
        }
        if (!pids[i])
        {
            /*codice del figlio*/

            printf("Sono il figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);

            /*attacchiamo le funzione handler per i segnali*/
            signal(16, stampa);
            signal(17, non_stampa);

            /*chiusura pipe --> produttore*/
            for (k = 0; k < M; ++k)
            {
                if (k != i)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            /*apriamo il file associato*/
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }

            while (read(fd, linea + pos, 1))
            {
                if (linea[pos] == 10)
                {
                    linea[pos] = 0;
                    // comunico al padre il primo carattere
                    write(FP[i][1], linea, 1);
                    /*mi metto in pausa aspettando il segnale.*/
                    pause();

                    /*una volta ricevuto continuo la mia lettura.*/
                    pos = 0;
                }
                else
                    ++pos;
            }
            exit(count);
        }
    }

    /*codice padre*/
    for (i = 0; i < M; ++i)
    {
        close(FP[i][1]);
    }
    char max = 0, curr;

    for (k = 0; k < H; ++k)
    {
        sleep(1);
        for (i = 0; i < M; ++i)
        {

            read(FP[i][0], &curr, 1);
            if (curr > max)
            {
                max = curr;
                pos = i;
            }
        }

        for (i = 0; i < M; ++i)
        {
            if (i != pos)
            {
                /*mando il segnale 17*/
                kill(pids[i], 17);
            }
            else
            {
                kill(pids[i], 16);
            }
        }

        max = 0;
        pos = -1;
    }

    /*aspetto i figli*/
    for (k = 0; k < M; ++k)
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