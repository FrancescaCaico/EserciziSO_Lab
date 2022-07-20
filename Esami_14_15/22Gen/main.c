// CORRETTO CONFRONTATO ANCHE CON LA PROF
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

typedef struct
{
    char chr;     // contiene il carattere maiuscolo
    long int occ; // il massimo tra le occorrenze torvate dal figlio e dal nipote
    char proc;    // N O F --> CHI HA TROVATO IL MASSIMO
    int pid;      // il pid di chi lo ha trovato
} info;

int main(int argc, char **argv)
{

    char c;

    if (argc < 5)
    {
        printf("Errore: numero dei parametri errato\n");
        exit(1);
    }

    int N = argc - 1;
    if (N % 2 || N < 4)
    {
        printf("NON PARI O NON MAGGIORE UGUALE A 4\n");
        exit(2);
    }
    N /= 2; // Il numero dei processi da generare
    printf("Il numero di processi da generare è %d\n", N);

    int pid, pidNipote, status, ritorno, ritorno2, fd;
    long int Pocc = 0, diff = 0;
    info max;
    char maius;

    // N PIPE FIGLIO PADRE.

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (FP == NULL)
    {
        printf("Errore malloc\n");
        exit(2);
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        if (pipe(FP[i]) < 0)
        {
            printf("Errore generazione pipe\n");
            exit(3);
        }
    }

    for (int i = 0; i < N; i++)
    {
        /* code */
        pid = fork();
        if (pid < 0)
        {
            printf("Errorecreazione figlio num %d\n", i);
            exit(4);
        }
        if (pid == 0)
        {
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i + 1]);

            for (int k = 0; k < N; ++k)
            {
                if (i != k)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
            }

            // creazione pipe con il nipote
            pipe_t NF, FN;
            if (pipe(NF) < 0 || pipe(FN) < 0)
            {
                printf("Errore creazione pipe NIPOTE FIGLIO o FIGLIO NIPOTE\n");
            }

            pidNipote = fork();
            if (pidNipote < 0)
            {
                printf("Errore: impossibile creare il nipote\n");
                exit(-1);
            }
            if (pidNipote == 0)
            {
                // codice nipote
                printf("Sono il nipote con pid.%d del figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), getppid(), i, argv[i + N + 1]);

                close(FP[i][1]); // NON MI INTERESSA
                close(NF[0]);    // DEVO MANDARE IL MIO NUMERO DI OCCORRENZE DELLA MAIUSCOLA.
                close(FN[1]);    // DEVO RICEVERE LA MAIUSCOLA

                // APRO IL MIO FILE ASSOCIATO
                if ((fd = open(argv[i + 1 + N], O_RDONLY)) < 0)
                {
                    printf("Il file %s non è valido.\n", argv[i + N + 1]);
                    exit(-1);
                }

                if ((read(FN[0], &maius, sizeof(char))) != 1)
                {
                    printf("Impossibile leggere la lettera maiuscola mandata dal figlio\n");
                    exit(-1);
                }

                while ((ritorno = read(fd, &c, 1)))
                {
                    if (c == maius)
                    {
                        ++Pocc;
                    }
                }

                write(NF[1], &Pocc, sizeof(long int));

                exit(ritorno);
            }

            close(NF[1]);
            close(FN[0]);
            // FILE ASSOCIATO
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Il file %s non è valido.\n", argv[i + N + 1]);
                exit(-1);
            }

            printf("Cerco la prima lettera maiuscola nel mio file %s\n", argv[i + 1]);
            bool trovato = false;

            while ((ritorno2 = read(fd, &c, 1)))
            {
                if (trovato == false)
                {
                    if (isalpha(c) && isupper(c))
                    {
                        // trovato la prima occorrenza
                        printf("Il carattere maiuscolo da cercare è '%c'\n", c);
                        write(FN[1], &c, 1);
                        maius = c;
                        trovato = true;
                    }
                }
                else
                {
                    if (c == maius)
                    {
                        Pocc++;
                    }
                }
            }

            // finito il while leggo le occorrenze trovate dal figlio
            if (read(NF[0], &diff, sizeof(long int)) != sizeof(long int))
            {
                printf("Errore impossibile leggere le occorrenze trovate dal figlio\n");
                exit(-1);
            }

            if (Pocc < diff)
            {

                // Il nipote ha trovato piu occorrenze...
                max.chr = maius;
                max.occ = diff;
                max.proc = 'N';
                max.pid = pidNipote;
            }
            else
            {
                max.chr = maius;
                max.occ = Pocc;
                max.proc = 'F';
                max.pid = getpid();
            }

            write(FP[i][1], &max, sizeof(info));

            // wait nipote
            sleep(1);
            if ((pidNipote = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(8);
            }

            if ((status & 0xff) != 0)
            {
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato in modo anomalo con status :%d\n", pidNipote, getppid(), status & 0xff);
            }
            else
            {
                ritorno = (int)(status >> 8);
                ritorno &= 0xff;
                printf("Il nipote con pid.%d figlio del processo con pid.%d è terminato correttamente con valore di ritorno:%d\n", pidNipote, getpid(), ritorno);
            }
            exit(ritorno2);
        }
    }

    // padre

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    for (int i = 0; i < N; i++)
    {
        /* code */

        read(FP[i][0], &max, sizeof(info));
        if (max.proc == 'N')
        {
            // struttura del nipote
            printf("Il nipote ha trovato un maggior numero di occorrenze nel file %s\n", argv[i + 1 + N]);
        }
        else if (max.proc == 'F')
        {
            printf("Il figlio ha trovato un maggior numero di occorrenze nel file %s\n", argv[i + 1]);
        }
        printf("Lettera maiuscola --> %c\nOccorrenza massima --> %ld\nChi l'ha trovata --> %c\nIl suo pid --> %d\n", max.chr, max.occ, max.proc, max.pid);
    }

    sleep(1);
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