// La parte in C accetta un numero variabile di parametri (maggiore o uguale a 3, da controllare) che
// rappresentano le seguenti informazioni: i primi N parametri rappresentano nomi di file F1...FN, mentre l’ultimo
// parametro deve essere considerato un numero intero positivo (H, da controllare) che rappresenta la lunghezza
// media in linee dei file F1...FN.
// Il processo padre deve generare N processi figli (P0 … PN-1): ogni processo figlio è associato al corrispondente
// file Fi. Ognuno di tali processi figli deve creare a sua volta un processo nipote (PP0 … PPN-1): ogni processo
// nipote PPi esegue concorrentemente calcolando la lunghezza in linee del file associato Fi usando in modo
// opportuno il comando wc di UNIX/Linux.
// Ogni processo figlio Pi, terminato il nipote PPi corrispondente, deve convertire in termini di valore intero
// (lunghezza) quanto scritto dal processo nipote PPi in termini di caratteri sullo standard output; quindi deve
// controllare tale valore rispetto al valore H: se lunghezza è maggiore o uguale a H, deve comunicare al padre la
// stringa “Sopra media”, altrimenti la stringa “Sotto media”.
// Per ogni figlio, il padre ha il compito di stampare su standard output, rispettando l'ordine dei file, la stringa
// ricevuta riportando sia il nome del file che il valore della stringa.
// Al termine, ogni processo figlio Pi deve ritornare al padre il valore di ritorno del proprio processo nipote PPi e il
// padre deve stampare su standard output il PID di ogni figlio e il valore ritornato.

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
typedef int pipe_t[2];
char sotto[12] = "Sotto media";
char sopra[12] = "Sopra media";

int main(int argc, char **argv)
{

    if (argc < 5)
    {
        printf("Errore numero dei parametri errato!\n");
        exit(1);
    }

    int H = atoi(argv[argc - 1]); // Lunghezza media in linee dei file
    if (H <= 0)
    {
        printf("Errore: %s non è numerico positivo\n", argv[argc - 1]);
        exit(2);
    }

    int N = argc - 2;

    pipe_t *FP = malloc(N * sizeof(pipe_t));
    if (!FP)
    {
        printf("Errore memoria\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(FP[i]) < 0)
        {
            printf("imposssibile generare la pipe\n");
            exit(4);
        }
    }

    int pid, pidn, status, ritorno, fd;

    char ret;
    for (int i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("errore impossibile generare il figlio %d\n", i);
            exit(5);
        }
        if (pid == 0)
        {

            printf("Sono il figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);
            pipe_t NF;
            if (pipe(NF) < 0)
            {
                exit(-1);
            }

            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }
            // senza pipe con il nipote
            if ((pidn = fork()) < 0)
            {
            }
            if (pidn == 0)
            {

                close(0);
                dup(fd);
                close(2);
                open("/dev/null", O_WRONLY);
                close(1);
                dup(NF[1]);
                close(NF[1]);
                close(NF[0]);
                execlp("wc", "wc", "-l", (char *)0);
                perror("Se vedi questa linea il wc -l non è andato a buon fine");
                exit(-2);
            }

            for (int k = 0; k < N; ++k)
            {
                close(FP[k][0]);
                if (k != i)
                {
                    close(FP[k][1]);
                }
            }
            close(NF[1]);

            // leggo un char da stdout
            read(NF[0], &ret, 1);
            printf("Il numero di linee del file %s è %c\n", argv[i + 1], ret);
            ret -= '\0';
            if (ret < H)
            {
                // sotto la media
                write(FP[i][1], sotto, 12 * sizeof(char));
            }
            else
            {
                write(FP[i][1], sopra, 12 * sizeof(char));
            }

            if ((pidn = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(8);
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
            exit(ritorno);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(FP[i][1]);
    }

    for (int k = 0; k < N; ++k)
    {
        if (read(FP[k][0], &sotto, 12 * sizeof(char)) == 12 * sizeof(char))
        {
            printf("Il file %s ha un numero di linee '%s'\n", argv[k + 1], sotto);
        }
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
            printf("Il figlio con pid.%d è terminato in modo anomalo con status :%d\n", pid, status & 0xff);
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