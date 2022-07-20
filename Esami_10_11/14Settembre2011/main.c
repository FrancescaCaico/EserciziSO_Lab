// La parte in C accetta un numero variabile N di parametri che rappresentano nomi di file (F0, F1, ... FN-1).
// Il processo padre deve generare N processi figli Pi (P0 ... PN-1): ogni processo figlio è associato al corrispondente file
// Fi. Ognuno di tali processi figli deve creare a sua volta un processo nipote PPi (PP0 ... PPN-1) associato sempre al
// corrispondente file Fi. Ogni processo figlio Pi e ogni nipote PPi esegue concorrentemente andando a cercare nel file
// associato Fi tutte le occorrenze dei caratteri alfabetici maiuscolo per il figlio e tutte le occorrenze dei caratteri numerici
// per il nipote. Ognuno dei processi figlio e nipote deve operare una modifica del file Fi: in specifico, ogni figlio deve
// trasformare ogni carattere alfabetico maiuscolo nel corrispondente carattere alfabetico minuscolo, mentre ogni nipote
// deve trasformare ogni carattere numerico nel carattere spazio. Una volta terminate le trasformazioni, sia i processi figli
// Pi che i processi nipoti PPi devono comunicare al padre il numero (in termini di long int) di trasformazioni effettuate.
// Il padre ha il compito di stampare su standard output, rispettando l'ordine dei file, il numero di trasformazioni ricevute
// da ogni figlio Pi e da ogni nipote PPi, riportando opportuni commenti esplicativi: ad esempio “nel file F0 sono stati
// trasformati tot1 caratteri alfabetici maiuscolo in caratteri alfabetici minuscolo e tot2 caratteri numerici nel carattere
// spazio”.
// Al termine, ogni processo nipote PPi deve ritornare al figlio Pi un opportuno codice ed analogamente ogni processo
// figlio Pi deve ritornare al padre un opportuno codice; il codice che ogni nipote PPi e ogni figlio Pi deve ritornare è:
// a) 0 se il numero di trasformazioni attuate è minore di 256;
// b) 1 se il numero di trasformazioni attuate è maggiore o uguale a 256, ma minore di 512;
// c) 2 se il numero di trasformazioni attuate è maggiore o uguale a 512, ma minore di 768;
// d) etc.
// Sia ogni figlio Pi e sia il padre devono stampare su standard output il PID di ogni nipote/figlio e il valore ritornato.

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

    int i, k, j;
    int pid, pidn;
    int ritorno, status;
    long int changed_alf = 0;
    long int changed_num = 0;
    // per contenere le occorrenze numeriche e alfabetiche trasformate da nipote e figlio.
    char c; // per contenere i caratteri letti.
    int fd; // file descriptor
    if (argc < 2)
    {
        printf("Errore: necessario almeno un nome di file.\n");
        exit(1);
    }

    int N = argc - 1;
    printf("Il numero di figli da creare è -> %i\n", N);

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    pipe_t *NP = malloc(N * sizeof(pipe_t));
    if (!FP || !NP)
    {
        printf("Errore allocazione memoria.\n");
        exit(2);
    }

    // generazione pipe
    for (i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0 || pipe(NP[i]) < 0)
        {
            printf("Errore: impossibile generare pipe.\n");
            exit(3);
        }
    }

    // GENERAZIONE FIGLI E NIPOTI CON LE VARIE OPERAZIONI
    for (i = 0; i < N; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            // errore
            printf("Errore: creazione figlio num.%d fallita!\n", i);
            exit(4);
        }
        if (pid == 0)
        {

            // codice figlio.

            printf("Sono il figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);
            pidn = fork();
            if (pidn < 0)
            {
                printf("Errore: creazione nipote num.%d fallita!\n", i);
                exit(-1);
            }
            if (pidn == 0)
            {
                // codice nipote
                for (k = 0; k < N; ++k)
                {
                    if (k != i)
                    {
                        close(NP[k][1]);
                    }
                    close(NP[k][0]);
                    close(FP[k][1]);
                    close(FP[k][0]);
                }
                printf("Sono il nipote del figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);

                /*apriamo il file associato*/
                if ((fd = open(argv[i + 1], O_RDWR)) < 0)
                {
                    printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                    exit(-1);
                }

                while (read(fd, &c, 1))
                {
                    if (isdigit(c))
                    {
                        changed_num += 1;
                        c = 32;
                        lseek(fd, -1L, SEEK_CUR);
                        write(fd, &c, 1);
                    }
                }
                // printf("trasformazioni numeriche--> %ld\n", changed_num);
                write(NP[i][1], &changed_num, sizeof(long int));
                exit(changed_num / 256);
            }
            for (k = 0; k < N; ++k)
            {
                if (k != i)
                {
                    close(FP[k][1]);
                }
                close(FP[k][0]);
                close(NP[k][1]);
                close(NP[k][0]);
            }

            /*apriamo il file associato*/
            if ((fd = open(argv[i + 1], O_RDWR)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }

            while (read(fd, &c, 1))
            {
                if (isalpha(c) && islower(c))
                {
                    ++changed_alf;
                    c = toupper(c);
                    lseek(fd, -1L, SEEK_CUR);
                    write(fd, &c, 1);
                }
            }
            // printf("trasformazioni alfabetiche --> %ld\n", changed_alf);

            write(FP[i][1], &changed_alf, sizeof(long int));
            sleep(1);
            if ((pidn = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(-1);
            }
            if ((status & 0xff) != 0)
            {
                printf("Il nipote del figlio con pid.%d è terminato in modo anomalo con status :%d.\n", pidn, status & 0xff);
            }
            else
            {
                ritorno = (int)(status >> 8);
                ritorno &= 0xff;
                printf("Il nipote del figlio con pid.%d è terminato correttamente con valore di ritorno:%d.\n", pidn, ritorno);
            }

            exit(changed_alf / 256);
        }
    }

    // chiusura pipe lato padre

    for (k = 0; k < N; ++k)
    {

        close(FP[k][1]);
        close(NP[k][1]);
    }
    // operazioni
    // int nr1 = 0, nr2 = 0;
    for (j = 0; j < N; ++j)
    {

        read(FP[j][0], &changed_alf, sizeof(long int));
        read(NP[j][0], &changed_num, sizeof(long int));

        // printf("Valori di nr1 = %d e nr2= %d\n", nr1, nr2);

        printf("nel file '%s' sono stati trasformati %ld caratteri alfabetici maiuscolo in caratteri alfabetici minuscolo e %ld caratteri numerici nel carattere spazio\n", argv[j + 1], changed_alf, changed_num);
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