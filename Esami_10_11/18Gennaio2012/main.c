#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
// La parte in C accetta un numero variabile di parametri che rappresentano nomi assoluti di file (F1, F2, ... FN) un
// singolo carattere (C) e un numero intero strettamente positivo (K): il numero di file è variabile ma comunque
// superiore ad 1; si effettuino i necessari controlli sul numero dei parametri e sugli ultimi due parametri
// considerando,senza verificarlo,che tutti i file abbiano esattamente K linee.
// Il processo padre deve creare N processi figli (P0 ... PN-1): ogni processo figlio è associato ad uno dei file Fi.
// Ogni processo figlio deve leggere le linee del file associato Fi sempre fino alla fine, contando in ogni linea le
// occorrenze del carattere C. Ognuno dei figli, una volta calcolato il numero di occorrenze di C trovate in una
// linea, deve inviare questo conteggio (in termini di long int) al processo padre: quindi ogni figlio deve inviare al
// padre K conteggi, uno per ogni linea del file Fi associato.
// Il padre ha il compito di stampare su standard output, rispettando l'ordine dei file, i conteggi ricevuti da ogni
// processo figlio riportando il nome del file e il numero di linea corrispondente: quindi il padre deve ricevere e
// stampare il primo conteggio ricevuto dal figlio P0, poi il primo conteggio ricevuto dal figlio P1 e così via fino ai
// primo conteggio ricevuto dal figlio PN-1 e quindi così via fino al K-esimo conteggio dei vari figli.
// Al termine, ogni processo figlio deve ritornare al padre:
// a) il valore 0 se non sono state trovate occorrenze del carattere C nel proprio file associato Fi;
// b) il valore 1 se sono state trovate occorrenze del carattere C nel proprio file associato Fi;
// e il padre deve stampare su standard output i PID di ogni figlio con il corrispondente valore ritornato

// char linea[255];
typedef int pipe_t[2];

int main(int argc, char **argv)
{
    int i, k;                 // cicli for
    int pid, ritorno, status; // gestione figli
    int fd;                   // file descriptor

    if (argc < 4)
    {
        printf("Numero parametri errato!");
        exit(1);
    }
    char C;
    int K;
    if ((K = atoi(argv[argc - 1])) <= 0)
    {
        printf("Non numerico positivo %s\n", argv[argc - 1]);
        exit(2);
    }

    if (argv[argc - 2][1] != 0)
    {
        printf("Non singolo carattere -> %s\n", argv[argc - 2]);
        exit(3);
    }
    C = argv[argc - 2][0];

    /*N processi figli*/
    int N = argc - 3;
    printf("Il numero di figli da generare è %d\n", N);
    long int occ = 0;
    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (FP == NULL)
    {
        printf("Errore: allocazione di memoria fallita!\n");
        exit(4);
    }
    /*generazione pipe*/
    for (i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            exit(5);
        }
    }

    for (i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            exit(6);
        }
        if (pid == 0)
        {
            printf("Sono il figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);

            /*chiusura pipe --> produttore*/

            for (k = 0; k < N; ++k)
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
            char c;
            bool trovate = 0;
            while (read(fd, &c, 1))
            {
                if (c == C)
                {
                    trovate = 1;
                    ++occ;
                }
                else if (c == 10)
                {
                    write(FP[i][1], &occ, sizeof(long int));
                    occ = 0;
                }
            }
            if (trovate)
                exit(1);
            else
                exit(0);
        }
    }

    for (i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    for (i = 1; i <= K; ++i)
    {

        /*ricevere da tutti le occorrenze di ogni linea. */
        sleep(1);
        puts(" ");
        printf("Queste sono le occorrenze di %c trovate da ogni figlio per la %d^ linea:\n", C, i);

        for (k = 0; k < N; ++k)
        {
            read(FP[k][0], &occ, sizeof(long int));
            printf("Il figlio n.%d ha trovato %ld occorrenze :)\n", i, occ);
        }
        puts(" ");
    }

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