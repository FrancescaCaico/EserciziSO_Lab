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
typedef int pipe_t[2];
#define BSIZE 11

int main(int argc, char **argv)
{

    if (argc <= 3)
    {
        printf("Errore: il numero dei parametri è errato!\n");
        exit(1);
    }

    int X = argc - 1; // numero di file = numero di figli.
    // pipe tra il primo processo e gli altri fratelli.
    pipe_t *piped = malloc((X - 1) * sizeof(pipe_t));
    if (piped == NULL)
    {
        printf("Errore: allocazione di memoria per la pipe fallita\n");
        exit(2);
    }

    // Genero le pipe
    for (int x = 0; x < X - 1; ++x)
    {
        if (pipe(piped[x]) < 0)
        {
            printf("Errore: impossibile creare la pipe.%d\n", x);
            exit(3);
        }
    }

    char Store[BSIZE];
    char L[BSIZE];

    int pid, pidn, status, ritorno, fd;
    // il padre deve generare X figli
    // il primo figlio manda la stringa del primo nome di file a tutti e tutti i processi fratelli confrontano il loro primo file con il file ricevuto dal primo figlio

    for (int x = 0; x < X; ++x)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore; creazione figlio n.%d fallita\n", x);
            exit(4);
        }
        if (pid == 0)
        {

            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), x, argv[x + 1]);

            if (x == 0)
            {
                // sono nel primo figlio. Mando il nome di file.
                //  il primo figlio si identifica come produttore
                // Effettuo le chiusure
                for (int i = 0; i < X - 1; ++i)
                {
                    close(piped[i][0]);
                }

                // apro il mio file associato
                if ((fd = open(argv[x + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il nome del file associato al figlio con indice.%d non è valido.\n", x);
                    exit(-1); // esco con codice errore
                }

                // il primo figlio legge dal suo file associato i nomi dei file e li mette in Store

                while (read(fd, L, BSIZE * sizeof(char)) == BSIZE * sizeof(char))
                {
                    L[BSIZE - 1] = '\0'; // metto il terminatore.
                    // mando ai fratelli
                    for (int j = 0; j < X - 1; ++j)
                    {
                        printf("Sto per mandare la stringa %s al fratello %d\n", L, j + 1);
                        write(piped[j][1], L, BSIZE);
                    }
                }
                exit(x);
            }

            else
            {

                // sono uno dei figli che riceve dal primo figlio il nome per il confronto
                //  i figli si identificano come consumatori
                for (int i = 0; i < X - 1; ++i)
                {
                    if (i != x - 1)
                    {
                        close(piped[i][0]);
                    }
                    close(piped[i][1]);
                }

                // apro il file associato al figlio
                if ((fd = open(argv[x + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il nome del file associato al figlio con indice.%d non è valido.\n", x);
                    exit(-1); // esco con codice errore
                }

                while (read(piped[x - 1][0], Store, BSIZE * sizeof(char)) == BSIZE * sizeof(char))
                {

                    lseek(fd, 0L, SEEK_SET);
                    Store[BSIZE - 1] = '\0';
                    printf("Ricevuta Store --> %s\n", Store);
                    // il nipote deve comunicare con il figlio che l'ha invocato per poter dire cosa ha ritornato diff.

                    while (read(fd, L, BSIZE * sizeof(char)) == BSIZE * sizeof(char))
                    {
                        L[BSIZE - 1] = '\0';
                        printf("Confronto %s (L) con %s (Store)\n", L, Store);
                        pidn = fork();
                        if (pidn < 0)
                        {

                            printf("Errore: impossibile creare il nipote d'ordine %d.\n", x);
                            exit(-1);
                        }
                        if (pidn == 0)
                        {
                            // il nipote deve comunicare con il figlio che l'ha invocato per poter dire cosa ha ritornato diff.
                            close(1);
                            open("/dev/null", O_WRONLY);
                            close(2);
                            open("/dev/null", O_WRONLY);

                            execlp("diff", "diff", Store, L, (char *)0);
                            perror("Se vedi questo avviso: la execlp non è andata a buon fine.");
                            exit(-2);
                        }
                        // se il comando diff non scrive nulla allora i due file sono uguali
                        // chiudo il lato della pipe nipotefiglio

                        if ((pidn = wait(&status)) < 0)
                        {
                            printf("Errore WAIT\n");
                            exit(-1);
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
                        if (ritorno == 0)
                        {
                            // i file sono uguali e procedo alla stampa
                            printf("I file %s e %s sono uguali\n", Store, L);
                        }
                    }
                }
                exit(x);
            }
        }
    }

    /*Codice del padre*/
    // Il padre chiude l'intera pipe perchè non è coinvolto nel processo.
    for (int i = 0; i < X - 1; ++i)
    {
        close(piped[i][0]);
        close(piped[i][1]);
    }

    // attende i figli
    for (int k = 0; k < X; ++k)
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