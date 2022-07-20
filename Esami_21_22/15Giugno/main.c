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

// conto le posizioni a partire da 0;

typedef int pipe_t[2];

// la funzione ok ritorna true se c'è ancora un processo in esecuzione, in questo caso che sta cercando una lettera minuscola-
// false se tutti hanno concluso
bool ok(bool *presenti, int N)
{
    for (int k = 0; k < N; ++k)
    {
        if (presenti[k] == 0)
        {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv)
{

    int pid, status, ritorno; // per gestire i processi
    int fd;                   // file descriptor
    char ch;                  // per il carattere letto via via dai figli
    long int cur;             // posizione della lettera minuscola trovata
    char token;               // lo uso per segnalare la stampa se vale 's' allora bisogna stampare le info se vale 'n' no.
    if (argc < 3)
    {
        printf("Errore numero dei parametri: mi aspetto almeno 2 nomi di file\n");
        exit(1);
    }
    int N = argc - 1;
    printf("Il numero di figli da generare è %d\n", N);

    // alloco dinamicamente un array di N booleani per tenere traccia di quali processi subiscono una terminazione
    // e non devono essere contati quando il padre raggruppa i vari caratteri minuscoli ricevuti.

    bool *presenti = calloc(N, sizeof(bool));

    // alloco le pipe che fungeranno per FiglioPadre --> comunicazione dei caratteri
    // PadreFiglio --> avviso di stampa.

    pipe_t *FiglioPadre = calloc(N, sizeof(pipe_t));
    pipe_t *PadreFiglio = calloc(N, sizeof(pipe_t));

    if (!FiglioPadre || !PadreFiglio || !presenti)
    {
        printf("Errore: calloc fallita\n");
        exit(2);
    }

    // genero le 2 pipe.
    for (int i = 0; i < N; ++i)
    {
        if (pipe(FiglioPadre[i]) < 0 || pipe(PadreFiglio[i]) < 0)
        {
            printf("Errore generazione pipe fallita\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore generazione figlio n.%d fallita\n", i);
            exit(4);
        }
        if (!pid)
        {
            printf("Sono il figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);

            // chiusura pipe...
            /*Il figlio è un produttore per il padre ma un consumatore per la pipe padre figlio*/
            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(PadreFiglio[k][0]);
                    close(FiglioPadre[k][1]);
                }
                close(PadreFiglio[k][1]);
                close(FiglioPadre[k][0]);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file %s non è valido\n", argv[i + 1]);
                exit(-1);
            }

            while (read(fd, &ch, 1))
            {

                if (islower(ch))
                {
                    // ho trovato un occorrenza minuscola
                    //  la comunico al padre tramite la pipe
                    write(FiglioPadre[i][1], &ch, 1);

                    // attendo l'esito della ricerca del massimo
                    read(PadreFiglio[i][0], &token, 1);

                    if (token == 's')
                    {
                        // devo stampare le info
                        cur = lseek(fd, 0L, SEEK_CUR) - 1;
                        printf("Il figlio con PID.%d e ordine %d ha trovato in posizione %ld del file associato %s il carattere massimo %c\n", getpid(), i, cur, argv[i + 1], ch);
                        ++ritorno;
                    }
                }
            }
            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FiglioPadre[i][1]);
        close(PadreFiglio[i][0]);
    }
    // Uso queste variabili per aiutarmi nella ricerca del minimo.
    char max = CHAR_MIN;
    int index;

    while (ok(presenti, N))
    {

        for (int i = 0; i < N; ++i)
        {
            if (presenti[i] == 0)
            {
                // se non riesco + a leggere allora il figlio non ha piu trovato lettere minuscole.
                if (read(FiglioPadre[i][0], &ch, 1) != 1)
                {
                    presenti[i] = 1;
                }
                else
                {
                    // altrimenti effettuo la ricerca del massimo per il carattere che ho appena letto

                    if (ch > max)
                    {
                        max = ch;
                        index = i;
                    }
                }
            }
        }

        // mando l'avviso ai figli.
        for (int i = 0; i < N; ++i)
        {
            if (presenti[i] == 0)
            {
                if (i == index)
                {
                    token = 's';
                }
                else
                {
                    token = 'n';
                }
                write(PadreFiglio[i][1], &token, 1);
            }
        }
        max = CHAR_MIN;
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