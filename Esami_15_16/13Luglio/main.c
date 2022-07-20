// CORRETTO
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
    char car;
    int pos;
} car_pos;

bool processi_vivi(bool *check, int N)
{
    for (int i = 0; i < N; ++i)
    {
        if (check[i] == 0)
        {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int pid, status, ritorno, fd;
    char c;
    car_pos x;
    if (argc < 3)
    {
        printf("Errore: numero dei parametri errato... almeno 2 \n");
        exit(1);
    }
    int N = argc - 1;

    if (N % 2)
    {
        printf("Errore: il numero dei parametri passati non è pari (file con lunghezza)\n");
        exit(2);
    }

    printf("IL NUMERO DEI PROCESSI DA GENERARE È %d\n", 26);

    bool *check = calloc(N, sizeof(bool));
    pipe_t *divisori = calloc(N, sizeof(pipe_t));
    pipe_t *caratteri = calloc(N, sizeof(pipe_t));
    if (check == NULL || caratteri == NULL || caratteri == NULL)
    {
        printf("Errore calloc\n");
        exit(3);
    }

    for (int i = 0; i < N; ++i)
    {
        if (pipe(divisori[i]) < 0 || pipe(caratteri[i]) < 0)
        {
            printf("Errore generazione delle pipe\n");
            exit(4);
        }
    }

    for (int i = 0; i < N / 2; ++i)
    {
        pid = fork();
        if (pid < 0)
        {
            printf("Errore: impossibile generare il figlio %d\n", i);
            exit(5);
        }
        if (pid == 0)
        {
            // codice del figlio i-esimo
            // presentazione:
            printf("Sono il figlio con pid.%d e indice d'ordine %d associato al file '%s'\n", getpid(), i, argv[i * 2 + 1]);

            for (int k = 0; k < N / 2; ++k)
            {
                if (i != k)
                {
                    close(divisori[k][0]);
                    close(caratteri[k][1]);
                }
                close(divisori[k][1]);
                close(caratteri[k][0]);
            }

            // apertura file
            if ((fd = open(argv[2 * i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: impossibile aprire il file\n");
                exit(-1);
            }

            // devo ricevere dal padre ill divisore
            int div;
            read(divisori[i][0], &div, sizeof(int));

            int pos = 0;
            ritorno = 0;
            while (read(fd, &c, 1))
            {
                pos++;
                if (pos % div == 0)
                {
                    // posizione multipla...
                    x.car = c;
                    x.pos = pos;
                    write(caratteri[i][1], &x, sizeof(car_pos));
                    ++ritorno;
                }
            }

            exit(ritorno);
        }
    }

    // codice padre
    for (int i = 0; i < N / 2; ++i)
    {
        close(caratteri[i][1]);
        close(divisori[i][0]);
    }

    // chiedo il divisore all'utente
    int div;
    for (int i = 0; i < N / 2; ++i)
    {
        scanf("%d", &div);
        while (div <= 0 || (atoi(argv[i + 3]) % div))
        {
            printf("Non mi hai dato un divisore valido...\n");
            scanf("%d", &div);
        }
        printf("Il divisore del figlio associato al file %s è %d\n", argv[2 * i + 1], div);
        write(divisori[i][1], &div, sizeof(int));
    }

    for (int i = 0; i < N / 2; ++i)
    {
        close(divisori[i][1]);
    }
    int ch = 0;
    while ((ch = processi_vivi(check, N / 2)))
    {

        // finche tutti i processi sono vivi e non hanno finito i file continuo a leggere i caratteri che mi mandano
        for (int i = 0; i < N / 2; ++i)
        {
            // se non è finito leggo la pipe associata
            if (!check[i])
            {
                if (read(caratteri[i][0], &x, sizeof(car_pos)) != sizeof(car_pos))
                {
                    // allora è morto
                    check[i] = 1;
                }
                else
                {
                    printf("Il figlio d'indice %d ha mandato il carattere %c in posizione %d nel file %s\n", i, x.car, x.pos, argv[2 * i + 1]);
                }
            }
        }
    }

    for (int k = 0; k < N / 2; ++k)
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
            printf("Il figlio con pid.%d  è terminato correttamente con valore di ritorno:%i.\n", pid, ritorno);
        }
    }

    exit(0);
}