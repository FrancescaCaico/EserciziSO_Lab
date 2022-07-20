#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

char linea[255];
typedef int pipe_t[2];

int main(int argc, char **argv)
{

    if (argc != 4)
    {
        printf("Errore: parametri errati.\n");
        exit(1);
    }

    // 1) FILE
    // 2)NUMERO INTERO POSITIVO N --> LUNGHEZZA  IN LINEE DI F
    // 3) NUMERO INTERO POSITIVO H --> LUNGHEZZA MEDIA IN CARATTERI DI OGNI LINEA DEL FILE F

    int N = atoi(argv[2]);
    int H = atoi(argv[3]);

    if (H <= 0 || N <= 0)
    {
        printf("Errore %s o %s non intero positivo.\n", argv[argc - 1], argv[argc - 2]);
        exit(2);
    }

    // creazione di due file
    char *nome_file1 = calloc(strlen(argv[1]) + 7, sizeof(char));
    char *nome_file2 = calloc(strlen(argv[1]) + 7, sizeof(char));

    if (nome_file1 == NULL || nome_file2 == NULL)
    {
        printf("Errore allocazione memoria fallita.\n");
        exit(2);
    }

    strcpy(nome_file1, argv[1]);
    strcpy(nome_file2, argv[1]);
    strcat(nome_file1, ".sopra");
    strcat(nome_file2, ".sotto");

    printf("Sto per creare il file --> %s\n", nome_file1);
    printf("Sto per creare il file --> %s\n", nome_file2);

    int Fsotto = creat(nome_file2, 0644);
    int Fsopra = creat(nome_file1, 0644);

    if (Fsopra < 0 || Fsotto < 0)
    {
        printf("Errore creazione dei file\n");
        exit(3);
    }

    printf("Il numero dei figli da generare è %d\n", N + 2);

    int pid, status, ritorno, fd;

    int L = 0;
    int pos = 0;
    // pipe tra un processo e gli altri

    pipe_t *sotto = malloc(N * sizeof(pipe_t));
    pipe_t *sopra = malloc(N * sizeof(pipe_t));
    for (int i = 0; i < N; ++i)
    {
        if (pipe(sotto[i]) < 0 || pipe(sopra[i]) < 0)
        {
            printf("Errore generazione pipe.\n");
            exit(4);
        }
    }

    for (int i = 0; i < N + 2; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore creazione figlio num.%i", i);
        }
        if (pid == 0)
        {
            // sono nel figlio --> possibile produttore per sotto e sopra
            if (i < N)
            {
                printf("Sono il figlio con PID.%d associato alla %d^ linea del file %s\n", getpid(), i, argv[1]);
                for (int k = 0; k < N; ++k)
                {
                    if (k != i)
                    {
                        close(sotto[k][1]);
                        close(sopra[k][1]);
                    }

                    close(sotto[k][0]);
                    close(sopra[k][0]);
                }
                if ((fd = open(argv[1], O_RDONLY)) < 0)
                {
                    exit(5);
                }

                while (read(fd, linea + L, 1))
                {
                    if (linea[L] == 10)
                    {
                        // fine linea... è la mia?
                        if (i == pos)
                        {

                            linea[L + 1] = 0;
                            //è la mia linea. Devo comunicarla al figlio sotto o sopra
                            printf("La mia linea è %s\n", linea);
                            if (L < H)
                            {
                                // la mando al sotto

                                write(sotto[i][1], linea, sizeof(linea));
                            }
                            else
                            {
                                write(sopra[i][1], linea, sizeof(linea));
                            }
                        }
                        ++pos;
                        L = 0;
                    }
                    else
                    {
                        ++L;
                    }
                }
                exit(L);
            }
            else if (i == N)

            {

                for (int k = 0; k < N; ++k)
                {

                    close(sotto[k][1]);
                    close(sopra[k][1]);
                    close(sopra[k][0]);
                }

                Fsotto = open(nome_file2, O_WRONLY);
                for (int j = 0; j < N; j++)
                {
                    /* code */
                    // LEGGO DALLA PIPE
                    if (read(sotto[j][0], linea, sizeof(linea)) == sizeof(linea))
                    {
                        // la scrivo nel file sotto associato al Fsotto
                        printf("Ricevuta linea --> %s\n", linea);

                        write(Fsotto, linea, strlen(linea) * sizeof(char));
                        ++pos;
                    }
                }
                exit(pos);
            }
            else if (i == N + 1)
            {
                for (int k = 0; k < N; ++k)
                {

                    close(sotto[k][1]);
                    close(sopra[k][1]);
                    close(sotto[k][0]);
                }
                Fsopra = open(nome_file1, O_WRONLY);

                for (int j = 0; j < N; j++)
                {
                    /* code */
                    // LEGGO DALLA PIPE
                    if (read(sopra[j][0], linea, sizeof(linea)) == sizeof(linea))
                    {
                        printf("Ricevuta linea --> %s\n", linea);
                        // la scrivo nel file sotto associato al Fsotto
                        write(Fsopra, linea, strlen(linea) * sizeof(char));
                        ++pos;
                    }
                }
                exit(pos);
            }
        }
    }

    for (int i = 0; i < N; ++i)
    {
        close(sotto[i][0]);
        close(sotto[i][1]);
        close(sopra[i][0]);
        close(sopra[i][1]);
    }
    for (int k = 0; k < N + 2; ++k)
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