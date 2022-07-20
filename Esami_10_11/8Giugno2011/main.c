#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>

typedef int pipe_t[2];
typedef struct
{
    char C;
    long int POS;
} car_pos;

int main(int argc, char **argv)
{

    int i, k, fd;
    int occ = 0;
    int pidn, pid, status, ritorno;
    if (argc < 5)
    {
        printf("Errore numero dei parametri errato! (N file + 2 caratteri singoli.)\n");
        exit(1);
    }

    int N = argc - 3;

    if ((argv[argc - 2][1] != 0) || (argv[argc - 1][1] != 0))
    {
        printf("Errore : %s oppure %s deve essere un carattere singolo.\n", argv[argc - 2], argv[argc - 1]);
        exit(2);
    }

    char C1 = argv[argc - 2][0]; // CARATTERE CERCATO DAL FIGLIO
    char C2 = argv[argc - 1][0]; // CARATTERE CERCATO DAL NIPOTE
    char c;
    car_pos x, y;
    pipe_t *NP = malloc(N * sizeof(pipe_t));
    pipe_t *FN = malloc(N * sizeof(pipe_t));

    if (NP == NULL || FN == NULL)
    {
        printf("Errore: allocazione per le pipe fallita.\n");
        exit(3);
    }

    for (i = 0; i < N; ++i)
    {
        if ((pipe(NP[i]) < 0) || (pipe(FN[i]) < 0))
        {
            printf("Errore generazione pipe.\n");
            exit(4);
        }
    }

    for (i = 0; i < N; ++i)
    {
        if ((pid = fork()) < 0)
        {
            printf("Errore generazione figlio num.%i!\n", i);
            exit(5);
        }
        if (!pid)
        {

            // presentazione del figlio
            printf("Sono il figlio con PID.%d e indice %d associato al file %s\n", getpid(), i, argv[i + 1]);
            // generazione nipote
            pidn = fork();
            if (pidn < 0)
            {
                printf("Errore generazione nipote num.%i!\n", i);
                exit(-1);
            }
            if (!pidn)
            {
                /*codice nipote */
                for (k = 0; k < N; ++k)
                {
                    if (k != i)
                    {
                        close(NP[k][1]);
                    }
                    close(NP[k][0]);
                    close(FN[k][1]);
                }

                /*apriamo il file associato*/
                if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
                {
                    printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                    exit(-1);
                }
                y.C = C2;
                y.POS = 0;
                while (read(fd, &c, 1))
                {
                    if (c == C2)
                    {
                        ++occ;
                        y.POS = lseek(fd, 0L, SEEK_CUR) - 1;

                        // ho trovato la pos del carattere associato al nipote.
                        if (read(FN[i][0], &x, sizeof(car_pos)) == sizeof(x))
                        {
                            if (x.POS < y.POS)
                            {

                                write(NP[i][1], &y, sizeof(y));
                            }
                            else
                            {
                                write(NP[i][1], &x, sizeof(x));
                            }
                        }
                        else
                        {
                            exit(occ);
                        }
                    }
                }
                exit(occ);
            }

            // chiusura lati pipe

            for (k = 0; k < N; ++k)
            {
                if (k != i)
                {
                    close(FN[k][1]);
                }
                close(FN[k][0]);
                close(NP[k][1]);
                close(NP[k][0]);
            }
            if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
            {
                printf("Errore: il file associato al figlio con PID.%d non è valido.\n", getpid());
                exit(-1);
            }
            x.C = C1;
            x.POS = 0;
            while (read(fd, &c, 1))
            {
                if (c == C2)
                {
                    // ho trovato la pos del carattere associato al nipote.
                    ++occ;
                    x.POS = lseek(fd, 0L, SEEK_CUR);
                    write(FN[i][1], &x, sizeof(x));
                }
            }

            // wait nipote
            sleep(1);
            if ((pidn = wait(&status)) < 0)
            {
                printf("Errore WAIT\n");
                exit(8);
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
            exit(occ);
        }
    }
    for (k = 0; k < N; ++k)
    {

        close(FN[k][1]);
        close(NP[k][1]);
        close(FN[k][0]);
    }

    for (i = 0; i < N; ++i)
    {
        while (read(NP[i][0], &x, sizeof(x)) == sizeof(x))
        {
            // stampa
            printf("Il nipote associato al file %s ha mandato la seguente struttura:\n\t-CARATTERE: '%c'\n\t-POSIZIONE:%ld\n", argv[i + 1], x.C, x.POS);
        }
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
// /* Soluzione della parte C del compito dell'8 Giugno 2011 - testo a */
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
// #include <string.h>
// #include <fcntl.h>
// #include <ctype.h>
// #include <signal.h>
// /* includiamo anche signal.h perche' il figlio usera' il segnale SIGUSR1 per fare terminare il nipote e non incorrere quindi in una condizione di deadlock */

// typedef int pipe_t[2];
// typedef struct
// {
//     char Cx;      /* carattere C1 o C2 del testo */
//     long int pos; /* posizione corrente del carattere nel file */
// } s_pos;

// int occ; /* numero di occorrenze usato sia da figli che da nipoti: A CAUSA DELLO SCHEMA DEL NIPOTE DEVE ESSERE UNA VARIABILE GLOBALE! */

// void handler(int signo)
// {
//     printf("STAMPA DI DEBUGGING-Processo %d ricevuto il segnale %d e uscira' con %d\n", getpid(), signo, occ);
//     exit(occ);
// }

// int main(int argc, char **argv)
// {
//     int N;               /* numero di file */
//     int pid;             /* per fork */
//     pipe_t *pipes;       /* array di pipe usate a per la comunicazione fra nipoti e padre */
//     pipe_t p;            /* pipe usata fra figlio e nipote */
//     int i, j;            /* contatori */
//     int fd;              /* file descriptor */
//     int status, ritorno; /* per valore di ritorno figli */
//     char C1, C2, ch;     /* caratteri da cercare e carattere letto da linea */
//     s_pos curF, curN;    /* strutture usate dal figlio e dal nipote */
//     int nr;              /* variabile per salvare valore di ritorno di read su pipe */

//     /* controllo sul numero di parametri almeno 2 file (anche se non richiesto) e due caratteri */
//     if (argc < 5)
//     {
//         printf("Errore numero di parametri\n");
//         exit(1);
//     }

//     /* controlliamo che il penultimo parametro e l'ultimo parametro siano singoli caratteri */
//     if (strlen(argv[argc - 2]) != 1 || strlen(argv[argc - 1]) != 1)
//     {
//         printf("Errore penultimo o ultimo parametro non singolo carattere\n");
//         exit(2);
//     }

//     /* individuiamo i caratteri da cercare */
//     C1 = argv[argc - 2][0];
//     C2 = argv[argc - 1][0];
//     /* stampa di debugging */
//     printf("Caratteri da cercare %c e %c\n", C1, C2);

//     N = argc - 3;
//     printf("Numero di processi da creare %d\n", N);

//     /* allocazione pipe */
//     if ((pipes = (pipe_t *)malloc(N * sizeof(pipe_t))) == NULL)
//     {
//         printf("Errore allocazione pipe\n");
//         exit(3);
//     }

//     /* creazione pipe */
//     for (i = 0; i < N; i++)
//         if (pipe(pipes[i]) < 0)
//         {
//             printf("Errore creazione pipe\n");
//             exit(4);
//         }

//     /* creazione figli */
//     for (i = 0; i < N; i++)
//     {
//         if ((pid = fork()) < 0)
//         {
//             printf("Errore creazione figli\n");
//             exit(5);
//         }
//         else if (pid == 0)
//         { /* codice figlio */
//             printf("Sono il processo figlio di indice %d e pid %d sto per creare il nipote che leggera' sempre dal mio stesso file %s\n", i, getpid(), argv[i + 1]);
//             /* nel caso di errore in un figlio o in un nipote decidiamo di ritornare un valore -1 */
//             /* ATTENZIONE IL FIGLIO NON USA LE PIPE CON IL PADRE, LA CHIUSURA DI TUTTI I LATI LA DOBBIAMO FARE PERO' DOPO AVER CREATO IL NIPOTE ALTRIMENTI IL NIPOTE AVREBBE DEI PROBLEMI */

//             /* creiamo la pipe di comunicazione con il nipote */
//             if (pipe(p) < 0)
//             {
//                 printf("FIGLIO-Errore creazione pipe\n");
//                 exit(-1); /* decidiamo, in caso di errore, di tornare -1 che verra' interpretato come 255 e quindi un valore NON accettabile */
//             }

//             /* agganciamo la funzione handler per fare in modo che il nipote gestisca, come vuole il figlio, il segnale SIGUSR1 */
//             signal(SIGUSR1, handler);

//             if ((pid = fork()) < 0)
//             {
//                 printf("Errore nella fork di creazione del nipote\n");
//                 exit(-1); /* decidiamo, in caso di errore, di tornare -1 che verra' interpretato come 255 e quindi un valore NON accettabile */
//             }
//             if (pid == 0)
//             {
//                 /* codice del nipote */
//                 printf("Sono il processo nipote del figlio di indice %d e pid %d, associato al file %s\n", i, getpid(), argv[i + 1]);
//                 /* Chiusura delle pipe non usate nella comunicazione con il padre  */
//                 for (j = 0; j < N; j++)
//                 {
//                     close(pipes[j][0]);
//                     if (i != j)
//                         close(pipes[j][1]);
//                 }
//                 /* chiusura pipe non usata nella comunicazione con il figlio */
//                 close(p[1]);

//                 /* sia il figlio che il nipote devono aprire (con due open separate per avere l'I/O pointer separato) lo stesso file in lettura, dato che dovranno cercare un carattere */
//                 if ((fd = open(argv[i + 1], O_RDONLY)) < 0)
//                 {
//                     printf("NIPOTE-Errore nella open del file %s\n", argv[i + 1]);
//                     exit(-1); /* decidiamo, in caso di errore, di tornare -1 che verra' interpretato come 255 e quindi un valore NON accettabile */
//                 }
//                 /* inizializzazione contatore di occorrenze */
//                 occ = 0;
//                 while (read(fd, &ch, 1))
//                 {
//                     /* inizializzazione campo carattere della struttura: nota bene dentro al ciclo perche' questo campo puo' cambiare */
//                     curN.Cx = C2;
//                     /* controlliamo se abbiamo trovato il carattere C2 */
//                     if (ch == C2)
//                     {
//                         /* per ricavare la posizione del carattere si puo' usare la funzione lseek */
//                         curN.pos = lseek(fd, 0L, SEEK_CUR) - 1; /* decrementiamo perche' consideriamo le posizioni da 0 */
//                         /* si deve a questo punto leggere la struttura inviata dal figlio */
//                         nr = read(p[0], &curF, sizeof(curF));
//                         if (nr != 0)
//                         {
//                             /* se siamo riusciti a leggere, allora verifichiamo i dati */
//                             if (curN.pos >= curF.pos)
//                             {
//                                 /* se la posizione e' maggiore o uguale, al padre dobbiamo mandare le informazioni del figlio */
//                                 curN.Cx = curF.Cx;
//                                 curN.pos = curF.pos;
//                             }
//                             /* si deve mandare l'informazione al padre */
//                             write(pipes[i][1], &curN, sizeof(curN));
//                             /* incrementiamo numero di occorrenze che andra' ritornato al padre */
//                             occ++;
//                         }
//                         else
//                             break; /* decidiamo di uscire dal ciclo while */
//                     }
//                     else
//                         ; /* non si deve fare nulla */
//                 }
//                 exit(occ); /* torniamo il valore richiesto dal testo che ci indica che sara' minore di 255 */
//                            /* NOTA BENE: il nipote eseguira' questa exit solo nel caso trovi un numero di occorrenze di C2 nel file, minore o uguale al numero di occorrenze di C1 trovate dal figlio */
//                            /* Altrimenti il nipote non esegue questa exit e termina invece per la ricezione del segnale SIGUSR1 inviato dal figlio */
//             }
//             else
//             {
//                 /* codice figlio */
//                 /* Chiusura di tutte le pipe non usate nella comunicazione con il padre  */
//                 for (j = 0; j < N; j++)
//                 {
//                     close(pipes[j][0]);
//                     close(pipes[j][1]);
//                 }
//                 /* chiusura pipe non usata nella comunicazione con il nipote */
//                 close(p[0]);

//                 /* il figlio ha un codice molto simile al nipote, con la differenza che NON si deve leggere */

//                 /* sia il figlio che il nipote devono aprire (con due open separate per avere l'I/O pointer separato) lo stesso file in lettura, dato che dovranno cercare un carattere */
//                 if ((fd = open(argv[i + 1], O_RDWR)) < 0)
//                 {
//                     printf("FIGLIO-Errore nella open del file %s\n", argv[i + 1]);
//                     exit(-1); /* decidiamo, in caso di errore, di tornare -1 che verra' interpretato come 255 e quindi un valore NON accettabile */
//                 }

//                 /* inizializzazione contatore di occorrenze */
//                 occ = 0;
//                 /* inizializzazione campo carattere della struttura */
//                 curF.Cx = C1;
//                 while (read(fd, &ch, 1))
//                 {
//                     /* controlliamo se abbiamo trovato il carattere  */
//                     if (ch == C1)
//                     {
//                         /* per ricavare la posizione del carattere si puo' usare la funzione lseek */
//                         curF.pos = lseek(fd, 0L, SEEK_CUR) - 1; /* decrementiamo perche' consideriamo le posizioni da 0 */
//                         /* si deve mandare l'informazione al nipote */
//                         write(p[1], &curF, sizeof(curF));
//                         /* incrementiamo numero di occorrenze che andra' ritornato al padre */
//                         occ++;
//                     }
//                     else
//                         ; /* non si deve fare nulla */
//                 }
//                 /* ATTENZIONE: il figlio deve inviare il segnale SIGUSR1 al nipote che potrebbe essere bloccato sulla read in attesa di altri dati dal figlio; la read fino a che il figlio non termina sarebbe bloccante, ma dato che la specifica dice che il figlio deve stampare il PID e il valore di ritorno NON si puo' NON aspettare il nipote */
//                 sleep(1);           /* fatta per sicurezza */
//                 kill(pid, SIGUSR1); /* il segnale viene chiaramente perso se il nipote e' gia' terminato */

//                 /* il figlio deve aspettare il nipote e stampare il suo pid con il valore ritornato (come richiesto dal testo) */
//                 pid = wait(&status);
//                 if (pid < 0)
//                 {
//                     printf("Errore in wait\n");
//                     exit(-1);
//                 }
//                 if ((status & 0xFF) != 0)
//                     printf("Nipote con pid %d terminato in modo anomalo\n", pid);
//                 else
//                     printf("Il nipote con pid=%d ha ritornato %d\n", pid, ritorno = (int)((status >> 8) & 0xFF));

//                 exit(occ); /* torniamo il valore richiesto dal testo che ci indica che sara' minore di 255 */
//             }
//         }
//     }

//     /* Codice del padre */
//     /* Il padre chiude i lati delle pipe che non usa */
//     for (i = 0; i < N; i++)
//     {
//         close(pipes[i][1]);
//     }

//     /* Il padre recupera le informazioni dai nipoti in ordine di indice */
//     for (i = 0; i < N; i++)
//     {
//         /* si devono recuperare tutte le infomazione inviate da ogni nipote */
//         while (read(pipes[i][0], &curN, sizeof(curN)))
//         {
//             if (curN.Cx == C1)
//                 printf("Il figlio di indice %d ha trovato il carattere %c nella posizione %ld nel file %s\n", i, curN.Cx, curN.pos, argv[i + 1]);
//             else
//                 printf("Il nipote del figlio di indice %d ha trovato il carattere %c nella posizione %ld nel file %s\n", i, curN.Cx, curN.pos, argv[i + 1]);
//         }
//     }

//     /* Il padre aspetta i figli */
//     for (i = 0; i < N; i++)
//     {
//         pid = wait(&status);
//         if (pid < 0)
//         {
//             printf("Errore in wait\n");
//             exit(7);
//         }

//         if ((status & 0xFF) != 0)
//             printf("Figlio con pid %d terminato in modo anomalo\n", pid);
//         else
//         {
//             ritorno = (int)((status >> 8) & 0xFF);
//             if (ritorno == 255)
//                 printf("Il figlio con pid=%d ha ritornato %d e quindi vuole dire che ci sono stati dei problemi\n", pid, ritorno);
//             else
//                 printf("Il figlio con pid=%d ha ritornato %d\n", pid, ritorno);
//         }
//     }
//     exit(0);
// }