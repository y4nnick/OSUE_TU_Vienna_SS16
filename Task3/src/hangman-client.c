/** hangman-client
  * @author Yannick Schwarenthorer, 1229026
  * @brief Client implementation of hangman
  * @detail Communicates to the server via Linux shared memory,  allows to play until the server runs out of words
  * @date 10.6.2016
  **/

#include "hangman-common.h"
#include "gallows.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <semaphore.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <unistd.h>

/* === Global Variables === */

/** @brief program name */
static const char *progname = "hangman-client";

/** @brief Signal indicator, set to 1 on SIGINT or SIGTERM */
volatile sig_atomic_t sig_caught = 0;

/** @brief ID of current client */
static int clientID = -1;

/** @brief Shared memory for client-server communication */
static struct Hangman_SHM *shared;

/** @brief Semaphores which tells the server when there is a request */
static sem_t *server_sem;

/** @brief Semaphores which tells the clients when the server is ready */
static sem_t *client_sem;

/** @brief Semaphores which tells the client who has sent the last request when there is an answer */
static sem_t *return_sem;


/* === Prototypes === */

/**
 * @brief terminate program on program error
 * @details global variables: progname
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief Signal handler
 * @details global variables: sig_caught
 * @param sig Signal number
 */
static void signal_handler(int sig);

/**
 * @brief free allocated resources
 * @details global variables: shared, server_sem, client_sem, return_sem
 * @param soft if true, the server is informed about the coming shutdown, else the resources are free'd silently
 */
static void free_resources(bool soft);

/* === Implementations === */

static void bail_out(int exitcode, const char *fmt, ...)
{
    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (errno != 0) {
        (void) fprintf(stderr, ": %s", strerror(errno));
    }
    (void) fprintf(stderr, "\n");

    free_resources(true);
    exit(exitcode);
}

static void signal_handler(int sig)
{
    sig_caught = 1;
}

static void free_resources(bool soft)
{
    DEBUG("Free resources\n");

    if (soft && shared != NULL) {

        /*** Critical Section: sending termination info ***/

        if (sem_wait(client_sem) == -1) {

            if (errno != EINTR) {
                (void)fprintf(stderr, "%s: sem_wait\n", progname);
            }else{
                (void) fprintf(stderr, "%s: interrupted while trying to inform server about shutdown\n", progname);
            }

        } else {

            DEBUG("Sending termination info\n");
            shared->terminate = true;
            shared->clientID = clientID;

            if (sem_post(server_sem) == -1){
                (void) fprintf(stderr, "%s: sem_post\n", progname);
            }

        }

        /* End critical Section: sending termination info */
    }

    if (shared != NULL) {
        if (munmap(shared, sizeof *shared) == -1) {
            (void) fprintf(stderr, "%s: munmap: %s\n", progname, strerror(errno));
        }
    }

    if (sem_close(server_sem) == -1) (void) fprintf(stderr, "%s: sem_close on %s: %s\n", progname, SERVER_SEM, strerror(errno));
    if (sem_close(client_sem) == -1) (void) fprintf(stderr, "%s: sem_close on %s: %s\n", progname, CLIENT_SEM, strerror(errno));
    if (sem_close(return_sem) == -1) (void) fprintf(stderr, "%s: sem_close on %s: %s\n", progname, RETURN_SEM, strerror(errno));
}


/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS, if no error occurs. Exits with EXIT_FAILURE otherwise
 */
int main(int argc, char *argv[])
{
    /* ###### Argument parsing ###### */
    if (argc > 0) {
        progname = argv[0];
    }

    if (argc != 1) {
        fprintf(stderr, "No command line arguments allowed.\nUSAGE: %s", progname);
        exit(EXIT_FAILURE);
    }

    /* ###### signal handlers ###### */
    const int signals[] = {SIGINT, SIGTERM};
    struct sigaction s;

    s.sa_handler = signal_handler;
    s.sa_flags   = 0;
    if(sigfillset(&s.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset");
    }
    for(int i = 0; i < COUNT_OF(signals); i++) {
        if (sigaction(signals[i], &s, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction");
        }
    }

    /* ###### Initialization of shared memory ###### */
    DEBUG("shared memory initialization\n");

    //Create
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT , PERMISSION);
    if (shmfd == -1) {
        fprintf(stderr, "%s: No server present. Start hangman-server first\n", progname);
        exit(EXIT_FAILURE);
    }

    //Set size
 /*   if ( ftruncate(shmfd, sizeof *shared) == -1) {
        (void) close(shmfd);
        bail_out(EXIT_FAILURE, "Could not ftruncate shared memory");
    }*/

    //Map
    shared = mmap(NULL, sizeof *shared, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shared == MAP_FAILED) {
        (void) close(shmfd);
        bail_out(EXIT_FAILURE, "Could not mmap shared memory");
    }

    //Close file descriptor
    if (close(shmfd) == -1) {
        bail_out(EXIT_FAILURE, "Could not close shared memory file descriptor");
    }

    /* ###### Initialization of Semaphores ###### */
    DEBUG("Semaphores initialization\n");
    server_sem = sem_open(SERVER_SEM, 0);
    client_sem = sem_open(CLIENT_SEM, 0);
    return_sem = sem_open(RETURN_SEM, 0);
    if (server_sem == SEM_FAILED || client_sem == SEM_FAILED || return_sem == SEM_FAILED) {
        bail_out(EXIT_FAILURE, "sem_open");
    }

    /* ###### Start new game ###### */
    DEBUG("Starting Game\n");

    //Counter
    unsigned int round = 0;
    unsigned int errors = 0;
    unsigned int wins = 0;
    unsigned int losses = 0;

    enum GameStatus game_status = New;
    char c = '\0';
    char buf[MAX_WORD_LENGTH];
    char tried_chars[MAX_WORD_LENGTH];

    (void) memset(&buf[0], 0, sizeof buf);
    (void) memset(&tried_chars[0], 0, sizeof tried_chars);

    while(sig_caught == 0) {
        if (game_status == Open) {

            //Read letter
            (void) printf("Enter one letter you want to try\n");

            if (fgets(&buf[0], MAX_WORD_LENGTH, stdin) == NULL) {
                if (errno == EINTR) continue;
                bail_out(EXIT_FAILURE, "fgets");
            }

            //Make upper case
            c = toupper(buf[0]);

            //Check char
            if (buf[1] != '\n') {
                (void) printf("Only one letter is allowed.\n");
                continue;
            }
            if (!isalpha(c)) {
                (void) printf("Please enter a valid letter.\n");
                continue;
            }

            //Check if letter is allready tried
            bool validChar = true;
            for (int i=0; i < round; i++) {
                if (tried_chars[i] == c) {
                    validChar = false;
                    break;
                }
            }
            if (!validChar) {
                (void) printf("Enter a letter you have not tried yet.\n");
                continue;
            }

            tried_chars[round] = c;
            round++;

        }

        /*  Begin critical Section: sending request */
        if (sem_wait(client_sem) == -1) {
            if (errno == EINTR) continue;
            bail_out(EXIT_FAILURE, "sem_wait");
        }

        if(shared->terminate) {
            DEBUG("Server terminated. Shutting down.\n");
            free_resources(false);
            exit(EXIT_FAILURE);
        }

        shared->status = game_status;
        shared->clientID = clientID;
        shared->tried_char = c;

        if (sem_post(server_sem) == -1) {
            bail_out(EXIT_FAILURE, "sem_post");
        }
        /* End critical Section: sending request */

        /* Begin critical Section: receiving answer */

        if (sem_wait(return_sem) == -1) {
            if (errno == EINTR) {
                (void) sem_post(client_sem);
                continue;
            }
            bail_out(EXIT_FAILURE, "sem_wait");
        }

        clientID = shared->clientID;
        strncpy(buf, shared->word, MAX_WORD_LENGTH);
        errors = shared->errors;
        game_status = shared->status;

        if (sem_post(client_sem) == -1) {
            bail_out(EXIT_FAILURE, "sem_post");
        }
        /* End critical Section: recieving answer */

        /* Parse answer */

        if (game_status == Impossible) {
            (void) printf("Congratulation the server is out of words. \n");
            break;
        }

        (void) printf("%s", gallows[errors]);

        if (game_status == Open) {

            (void) printf ("\n%s You have already tried the following characters \"%s\"\n", buf, tried_chars);

        } else {
            (void) printf("The correct word was %s\n", buf);

            if (game_status == Won)  {
                (void) printf("Congratulations! You won.\n");
                wins++;
            }
            if (game_status == Lost) {
                (void) printf("Game Over! Want to try again?\n");
                losses++;
            }

            //Print stats
            (void) printf("You have now won %d games and lost %d.\n", wins, losses);
            (void) printf("Press 'y' to start a new game or 'n' to stop playing.\n");

            c = tolower(fgetc(stdin));
            if (ferror(stdin)){
                bail_out(EXIT_FAILURE, "fgetc");
            }

            if (c == 'y') {
                game_status = New;
                round = 0;
                errors = 0;
                (void) memset(tried_chars, 0, sizeof tried_chars);
            } else {
                break;
            }
        }
    }

    /* no next game possible or client wants quit */
    (void) printf("You have won %d games and lost %d. Good bye!\n", wins, losses);

    free_resources(true);

    return EXIT_SUCCESS;
}
