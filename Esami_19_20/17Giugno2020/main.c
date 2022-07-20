
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
char buff[255];
typedef int pipe_t[2];
int main(int argc, char **argv)
{
    int pid, status, ritorno;
    int fd;

    if (argc != 4)
    {
        printf("Errore mi aspetto UN NOME DI FILE, UN NUMERO POSITIVO B, E UN NUMERO POSITIVO L\n");
        exit(1);
    }

    int L = atoi(argv[2]);
    int B = atoi(argv[3]);

    printf("DEBUG- Il numero di processi da generare è %d\n", B);

    char *nome = calloc(7 + strlen(argv[1]), sizeof(char));
    sprintf(nome, "%s.Chiara", argv[1]);

    int Fcreato = creat(nome, 0644);
    if (Fcreato < 0)
    {
        printf("Errore creazione file .Chiara\n");
        exit(2);
    }

    pipe_t *FP = malloc(B * sizeof(pipe_t));
    if (FP == NULL)
    {
        printf("Errore malloc\n");
        exit(3);
    }

    for (int q = 0; q < B; ++q)
    {
        if (pipe(FP[q]) < 0)
        {
            printf("Errore generazione pipe\n");
            exit(4);
        }
    }

    if ((fd = open(argv[1], O_RDONLY)) < 0)
    {
        printf("Errore: il file passato come parametro non è valido\n");
        exit(-1);
    }

    // calcolo la dimensione di ogni blocco;
    int dim = L / B; // valore di ritorno
    printf("Dimensione di ogni blocco --> %d\n", dim);

    for (int q = 0; q < B; ++q)
    {
        if ((pid = fork()) < 0)
        {
            printf("Impossibile poter generare il figlio n.%d\n", q);
            exit(5);
        }
        if (!pid)
        {

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al BLOCCO N.%d\n", getpid(), q, q + 1);
            for (int i = 0; i < B; ++i)
            {
                if (i != q)
                {
                    close(FP[i][1]);
                }
                close(FP[i][0]);
            }

            // sposto l'io pointer sul primo carattere del blocco associato al figlio corrente
            long int x = (q * L / B);
            lseek(fd, x, SEEK_SET);

            // leggo dal file dim byte
            read(fd, buff, dim * sizeof(char));
            printf("%s\n", buff);
            write(FP[q][1], &buff[dim - 1], 1);
            exit(dim);
        }
    }

    for (int i = 0; i < B; ++i)
    {
        close(FP[i][1]);
    }
    char c;
    for (int i = 0; i < B; ++i)
    {
        read(FP[i][0], &c, 1);
        write(Fcreato, &c, 1);
    }

    sleep(1);
    for (int k = 0; k < B; ++k)
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