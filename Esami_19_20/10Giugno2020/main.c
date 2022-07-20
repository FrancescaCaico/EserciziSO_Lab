
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
char linea[250];
typedef int pipe_t[2];
int main(int argc, char **argv)
{

    int pid, status, ritorno = 0;
    int fd;
    int len = 0;

    if (argc < 3)
    {
        printf("Errore: numero dei parametri errato\n");
        exit(1);
    }

    int Q = argc - 1;
    printf("Il numero dei figli da generare è %d\n", Q);

    // CREAZIONE FILE CAMILLA
    char nome[8];
    sprintf(nome, "Camilla");
    int fdw = creat(nome, 0644);
    if (fdw < 0)
    {
        printf("IMPOSSIBILE CREARE IL FILE CAMILLA\n");
        exit(2);
    }

    pipe_t *FP = malloc(Q * sizeof(pipe_t));
    if (FP == NULL)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (int q = 0; q < Q; ++q)
    {
        if (pipe(FP[q]) < 0)
        {
            printf("Errore generazione pipe\n");
            exit(4);
        }
    }

    for (int q = 0; q < Q; ++q)
    {
        if ((pid = fork()) < 0)
        {
            printf("Impossibie generare il figlio n.%d\n", q);
            exit(5);
        }
        if (!pid)
        {

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), q, argv[q + 1]);
            for (int i = 0; i < Q; ++i)
            {
                if (i != q)
                {
                    close(FP[i][1]);
                }
                close(FP[i][0]);
            }

            // apertura file associato
            if ((fd = open(argv[q + 1], O_RDONLY)) < 0)
            {
                printf("Errore file non valido\n");
                exit(-1);
            }

            while (read(fd, linea + len, 1))
            {
                ++len;
                if (linea[len - 1] == 10)
                {
                    // finita la linea. Verifico se rispetta le specifiche.
                    if (isdigit(linea[0]) && len < 10)
                    {
                        // rispetta la specifica... la mando quindi al padre
                        ++ritorno;
                        write(FP[q][1], linea, len * sizeof(char));
                    }
                    len = 0;
                }
            }
            exit(ritorno);
        }
    }

    for (int i = 0; i < Q; ++i)
    {
        close(FP[i][1]);
    }

    for (int q = 0; q < Q; ++q)
    {
        len = 0;
        while (read(FP[q][0], linea + len, 1))
        {

            if (linea[len] == 10)
            {
                linea[len + 1] = '\0';
                printf("Il figlio d'indice %d associato al file %s ha mandato la linea '%s'\n", q, argv[q + 1], linea);
                write(fdw, linea, len + 1);
                len = 0;
            }
            else
            {
                ++len;
            }
        }
    }
    for (int k = 0; k < Q; ++k)
    {
        if ((pid = wait(&status)) < 0)
        {
            printf("Errore WAIT\n");
            exit(8);
        }
        if ((status & 0xff) != 0)
        {
            printf("Il figlio con pid.%d è terminato in modo anomalo con status:%d.\n", pid, status & 0xff);
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