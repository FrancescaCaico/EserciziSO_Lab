// La parte in C accetta un numero variabile N+2 di parametri che rappresentano i primi N nomi di file (F0, F1, ... FN-1),
// mentre il penultimo rappresenta un singolo carattere (C) (da controllare) e l’ultimo rappresenta un numero intero (H)
// strettamente positivo (da controllare) che indica la lunghezza in linee dei file: infatti, la lunghezza in linee dei file è la
// stessa (questo viene garantito dalla parte shell e NON deve essere controllato).
// Il processo padre deve generare N processi figli (P0 ... PN-1) ognuno dei quali è associato ad uno dei file Fi. Ogni
// processo figlio Pi deve leggere le linee del file associato Fi sempre fino alla fine.
// I processi figli devono attenersi a
// questo schema di comunicazione a pipeline: il figlio P0 comunica con il figlio P1 che comunica con il figlio P2 etc.
// fino al figlio PN-2 che comunica con il figlio PN-1; questo schema a pipeline deve essere ripetuto H volte e cioè per
// ogni linea letta dai file associati Fi e deve prevedere l’invio in avanti, per ogni linea letta, via via di una struttura che
// deve contenere due campi, ind e occ, con ind uguale all’indice d’ordine i di uno dei processi Pi e con occ uguale al
// numero di occorrenze di C trovate da Pi nella linea corrente. In particolare, il figlio P0 passa in avanti (cioè comunica)
// per ogni linea letta via via una struttura S0, con ind uguale a 0 e con occ uguale al numero di occorrenze di C trovate
// nella linea corrente; il figlio seguente P1, dopo la lettura della propria linea corrente, verifica il numero di occorrenze di
// C trovate nella linea corrente nei confronti del valore di occ ricevuto da P0 e se il suo numero di occorrenze è maggiore
// passa avanti la struttura S0 ricevuta, altrimenti confeziona la struttura S1 con i propri dati e la passa al figlio seguente
// P2, etc. fino al figlio PN-2 che si comporta in modo analogo e passa all’ultimo processo figlio PN-1. Quindi, all’ultimo
// processo figlio PN-1 devono arrivare H strutture, una per ogni linea letta dai processi P0 ... PN-2 che rappresentano
// l’informazione di quale altro figlio ha trovato il numero minore di occorrenze di C. Per ogni linea, l’ultimo processo
// figlio PN-1 dopo la lettura della propria linea corrente, verifica il numero di occorrenze di C trovate nella linea corrente
// nei confronti del valore ricevuto da PN-2 e se il suo numero di occorrenze è minore deve stampare la sua linea corrente
// su standard output, altrimenti deve chiedere* al figlio il cui indice risulta nella struttura ricevuta di stampare la sua linea
// corrente su standard output.
// Al termine, ogni processo figlio Pi deve ritornare al padre il numero di linee stampate su standard output e il padre deve
// stampare su standard output il PID di ogni figlio e il valore ritornato.
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef int pipe_t[2];
char linea[255];
int count = 0, count2 = 0;
typedef struct
{
    int occ; // occorrenze trovate
    int ind; // indice del processo
} info;

void stampa()
{
    printf("ecco la %d^ linea migliore --> %s\n", count, linea);
    ++count2;
}
void non_stampa()
{
    return;
}
int main(int argc, char **argv)
{
    int i, j, k;
    int pid;
    int status, ritorno;
    int fd;

    // assumiamo che ci sia almeno un file
    if (argc < 4)
    {
        printf("Error code n.1: number of params not valid.\n");
        exit(1);
    }

    int H;
    char c;
    int pos = 0;
    if ((H = atoi(argv[argc - 1])) <= 0)
    {
        printf("Error code n.2: last one parameter must be a positive number.\n");
        exit(2);
    }

    if (argv[argc - 2][1] != 0)
    {
        printf("Error code n.3: second last one parameter must be a single character.\n");
        exit(3);
    }

    c = argv[argc - 2][0];
    int N = argc - 3;
    printf("Number of process to generate --> %d\n", N);

    // PIPELINE COMMUNICATION
    pipe_t *pipeline = malloc((N - 1) * sizeof(pipe_t));
    int *pids = calloc(N, sizeof(int));
    if (!pipeline || !pids)
    {
        exit(4);
    }

    for (i = 0; i < N; ++i)
    {
        if (pipe(pipeline[i]) < 0)
        {
            exit(5);
        }
    }

    for (i = 0; i < N; ++i)
    {
        if ((pids[i] = fork()) < 0)
        {
            exit(6);
        }
        if (pids[i] == 0)
        {
            /*generated process code*/
            // closing its pipes edge
            signal(16, stampa);
            signal(17, non_stampa);
            if (i == 0)
            {
                close(pipeline[i][0]);
            }

            // opening its file
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }
            info curr, prev;
            curr.ind = i;
            curr.occ = 0;
            while (read(fd, linea + pos, 1))
            {
                ++pos;
                if (linea[pos - 1] == c)
                {
                    ++curr.occ;
                }
                else if (linea[pos - 1] == 10)
                {
                    ++count;
                    // end of current analized line.
                    linea[pos - 1] = 0;
                    if (i != 0)
                    {
                        read(pipeline[i - 1][0], &prev, sizeof(info));

                        if (prev.occ > curr.occ)
                        {
                            curr = prev;
                        }
                    }
                    if (i != N - 1)
                    {
                        write(pipeline[i][1], &curr, sizeof(info));
                        pause();
                    }
                    else
                    {
                        if (curr.ind == N - 1)
                        {
                            // STAMPO LA MIA OWN STRUTTURA e dico agli altri di non fare nulla
                            for (j = 0; j < N; ++j)
                            {
                                kill(pids[j], 17);
                            }
                            printf("ecco la %d^ linea migliore --> %s\n", count, linea);
                            ++count2;
                        }
                        else
                        {
                            for (j = 0; j < N; ++j)
                            {
                                if (j != curr.ind)
                                {
                                    kill(pids[j], 17);
                                }
                                else
                                {
                                    kill(pids[curr.ind], 16);
                                }
                            }
                        }
                    }

                    pos = 0;
                }
            }
            exit(count2);
        }
    }

    for (i = 0; i < N; ++i)
    {
        close(pipeline[i][0]);
        close(pipeline[i][1]);
    }
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