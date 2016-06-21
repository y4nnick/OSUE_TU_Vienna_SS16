/**
 * @file hangman-server.c
 * @author Yannick Schwarenthorer
 * @date 12.6.2016
 *
 * @brief This module behaves as a server in the hangman game. Clients can connect via shared memory. Synchronization via semaphores.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "fileRead.h"
#include <getopt.h>
#include "hangman-common.h"
#include <unistd.h>


/* === Structures and Enumerations === */

/**
 * @brief Structure representing a game
 */
struct Game {
    char *secret_word; 						/**< Pointer to the secret word of the game */
    char obscured_word[MAX_WORD_LENGTH];	/**< A buffer storing the current partly unobscured version of the secret word */
    enum GameStatus status;					/**< Indicating in which state the game is */
    unsigned int errors;					/**< Error counter in the game */
};

/**
 * @brief Structure representing a linked list of clients
 */
struct Client {
    int clientID;					/**< The ID of the client used for identification */
    struct WordNode *used_words;	/**< A linked list of the used words */
    struct Game current_game;		/**< The currently played game */
    struct Client *next;			/**< The next client or NULL */
};

/**
 * @brief Structure representing a linked list of words
 */
struct WordNode {
    char *word;					/**< A pointer to the stored string */
    struct WordNode *next;		/**< The next WordNode in the linked list or NULL */
};

/* === Global Variables === */

/** @brief Name of the program */
static const char *progname = "hangman-server";

/** @brief word buffer */
static struct Buffer word_buffer;

/** @brief Signal indicator, gets set to 1 on SIGINT or SIGTERM */
volatile sig_atomic_t sig_caught = 0;

/** @brief Linked List of active clients */
static struct Client *clients = NULL;

/** @brief client count for client number assignment */
static unsigned int client_count = 0;

/** @brief A boolean flag indicating whether the semaphores have been started creating or created yet */
static bool semaphores_set = false;

/** @brief Shared memory for client-server communication */
static struct Hangman_SHM *shared;

/** @brief Semaphores which tells the server when there is a request */
static sem_t *server_sem;

/** @brief Semaphores which tells the clients when the server is ready */
static sem_t *client_sem;

/** @brief Semaphores which tells the client who has sent the last request when there is an answer */
static sem_t *return_sem;

/* === Prototypes === */

//TODO rewrite this

/**
 * @brief starts a new game for a given client
 * @details global variables: word_buffer
 * @param *client pointer to the struct Client who wants a new game
 */
static void new_game(struct Client *client);

/**
 * @brief Calculates the game result for a given client for a given character and stores the results inside the client
 * @param *client pointer to the struct Client who sent the request
 * @param try the character he guessed
 */
static void calculate_results(struct Client *client, char try);

/**
 * @brief This function tells whether or not a certain client has played a given word in the past
 * @param *node pointer to the head of the word list of the client
 * @param *word pointer to the word that shall be tested
 * @return true, if the word is inside the list, false otherwise
 */
static bool contains(struct WordNode *node, char *word);

/**
 * @brief Signal handler
 * @details global variables: sig_caught
 * @param sig Signal number catched
 */
static void signal_handler(int sig);

/**
 * @brief terminate program on program error
 * @details global variables: progname
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief Free allocated resources and inform clients via shared memory about shutdown
 * @details global variables: word_buffer, clients, semaphores_set, srv_sem, clt_sem, ret_sem
 */
static void free_resources(void);

/**
 * @brief Frees the linked list of word pointers for a given client
 * @param *client pointer to the struct Client whose word list shall be freed
 */
static void free_words(struct Client *client);

/**
 * @brief This function Frees the whole linked list of struct Client pointers whose head is the global variable clients.
 * Because these are all active clients, client_sem is incremented for each of them, so that they can terminate, when they
 * try to send something and see the terminate flag is set to true.
 * @details global variables: *clients
 */
static void free_clients();


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

    free_resources();
    exit(exitcode);
}

static void free_resources(void)
{
    DEBUG("Free resources\n");

    if (shared != NULL) {
        shared->terminate = true;
        freeBuffer(&word_buffer);
        free_clients();

        if (munmap(shared, sizeof *shared) == -1) {
            (void) fprintf(stderr, "%s: munmap: %s\n", progname, strerror(errno));
        }
        if(shm_unlink(SHM_NAME) == -1) {
            (void) fprintf(stderr, "%s: shm_unlink: %s\n", progname, strerror(errno));
        }
    }
    if (semaphores_set) {
        if (sem_close(server_sem) == -1) (void) fprintf(stderr, "%s: sem_close on %s: %s\n", progname, SERVER_SEM, strerror(errno));
        if (sem_close(client_sem) == -1) (void) fprintf(stderr, "%s: sem_close on %s: %s\n", progname, CLIENT_SEM, strerror(errno));
        if (sem_close(return_sem) == -1) (void) fprintf(stderr, "%s: sem_close on %s: %s\n", progname, RETURN_SEM, strerror(errno));
        if (sem_unlink(SERVER_SEM) == -1) (void) fprintf(stderr, "%s: sem_unlink on %s: %s\n", progname, SERVER_SEM, strerror(errno));
        if (sem_unlink(CLIENT_SEM) == -1) (void) fprintf(stderr, "%s: sem_unlink on %s: %s\n", progname, CLIENT_SEM, strerror(errno));
        if (sem_unlink(RETURN_SEM) == -1) (void) fprintf(stderr, "%s: sem_unlink on %s: %s\n", progname, RETURN_SEM, strerror(errno));
    }
}

static void signal_handler(int sig)
{
    sig_caught = 1;
}

static void new_game(struct Client *client)
{
    //Get new word
    srand(time(NULL));
    unsigned int pos = rand() % word_buffer.length;
    unsigned int i = 0;
    while (contains(client->used_words, word_buffer.content[pos])) {
        pos = (pos + 1) % word_buffer.length;
        if (i >= word_buffer.length) {
            client->current_game.status = Impossible;
            return;
        }
        i++;
    }

    (void) memset(&client->current_game, 0, sizeof (struct Game));
    client->current_game.secret_word = word_buffer.content[pos];
    client->current_game.status = Open;

    /* add the selected word to the client's word list so that he won't get it again */
    struct WordNode *used_word = malloc(sizeof (struct WordNode));
    if (used_word == NULL) {
        bail_out(EXIT_FAILURE, "malloc while creating a WordNode");
    }
    used_word->word = word_buffer.content[pos];
    used_word->next = client->used_words;
    client->used_words = used_word;

    /* represent the secret word with '_', except for whitespaces which are visible immediately */
    size_t length = strlen(client->current_game.secret_word/*, MAX_WORD_LENGTH*/);
    for (i = 0; i < length; i++) {
        if (client->current_game.secret_word[i] == ' ') {
            client->current_game.obscured_word[i] = ' ';
        } else {
            client->current_game.obscured_word[i] = '_';
        }
    }
    client->current_game.obscured_word[i+1] = '\0';
}

static void free_words(struct Client *client)
{
    struct WordNode *cur = client->used_words;
    struct WordNode *next;

    while (cur != NULL) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}

static void free_clients()
{
    struct Client *cur = clients;
    struct Client *next;

    while (cur != NULL) {
        next = cur->next;
        free_words(cur);
        free(cur);
        cur = next;

        /* increment client semaphore for each client so that they can terminate */
        if (sem_post(client_sem) == -1) {
            (void) fprintf(stderr, "sem_post");
        }
    }
}

static bool contains(struct WordNode *node, char *word)
{
    for (; node != NULL; node = node->next) {
        if (strncmp(node->word, word, MAX_WORD_LENGTH) == 0) {
            return true;
        }
    }
    return false;
}

static void calculate_results(struct Client *client, char try)
{
    bool error = true;
    bool won = true;

    /* unobscure secret string */
    size_t length = strlen(client->current_game.secret_word/*, MAX_WORD_LENGTH*/);
    for (size_t i = 0; i < length; i++) {
        if (client->current_game.secret_word[i] == try) {
            client->current_game.obscured_word[i] = try;
            error = false;
        }
        won = won && (client->current_game.obscured_word[i] != '_');
    }

    if (error) {
        client->current_game.errors++;
        if (client->current_game.errors >= MAX_ERROR) {
            client->current_game.status = Lost;
            (void) strncpy(client->current_game.obscured_word, client->current_game.secret_word, MAX_WORD_LENGTH);
        }
    } else if (won) {
        client->current_game.status = Won;
    }
}

//TODO rewrite

/**
 * @brief Program entry point
 * @details global variables: all above defined
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS or aborts with exit(EXIT_FAILURE)
 */
int main(int argc, char *argv[])
{
    /* ######  Argument parsing ###### */
    if (argc > 0) {
        progname = argv[0];
    }
    if (argc > 2) {
        fprintf(stderr, "To many files\nUSAGE: %s [input_file]", progname);
        exit(EXIT_FAILURE);
    }

    int c;
    while ( (c = getopt(argc, argv, "")) != -1 ) {
        switch(c) {
            case '?': /* invalid Argument */
                fprintf(stderr, "USAGE: %s [input_file]", progname);
                exit(EXIT_FAILURE);
            default:  /* imposible */
                assert(0);
        }
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

    /* ###### Reading the words for the game ###### */

    //Reading out file
    if(argc == 2) {
        DEBUG("Reading dictionary ... ");
        FILE *f;
        char *path = argv[1];

        if( (f = fopen(path, "r")) == NULL ) {
            bail_out(EXIT_FAILURE, "fopen failed %s", path);
        }

        if(readFile(f, &word_buffer, MAX_WORD_LENGTH, false) != 0) {
            (void) fclose(f);
            bail_out(EXIT_FAILURE, "Error while reading file %s", path);
        }

        if (fclose(f) != 0) {
            bail_out(EXIT_FAILURE, "fclose failed on file %s", path);
        }
        DEBUG("File read finished\n");

    } else {	/* there are no files --> read from stdin */
        (void) printf("Please enter the game dictionary and finish the step with EOF\n");

        if ( readFile(stdin, &word_buffer, MAX_WORD_LENGTH, false) != 0) {
            if (sig_caught) {
                DEBUG("Caught signal, shutting down\n");
                free_resources();
                exit(EXIT_FAILURE);
            }
            bail_out(EXIT_FAILURE, "Error while reading dictionary from stdin");
        }

        (void) printf("Successfully read the dictionary. Ready.\n");
    }

    /* ###### Initialization of SHM ###### */
    DEBUG("SHM initialization\n");

    //Open
    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, PERMISSION);
    if (shmfd == -1) {
        bail_out(EXIT_FAILURE, "Could not open shared memory");
    }

    //Truncate
    if ( ftruncate(shmfd, sizeof *shared) == -1) {
        (void) close(shmfd);
        bail_out(EXIT_FAILURE, "Could not ftruncate shared memory");
    }

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
    semaphores_set = true;
    server_sem = sem_open(SERVER_SEM, O_CREAT | O_EXCL, PERMISSION, 0);
    client_sem = sem_open(CLIENT_SEM, O_CREAT | O_EXCL, PERMISSION, 1);
    return_sem = sem_open(RETURN_SEM, O_CREAT | O_EXCL, PERMISSION, 0);
    if (server_sem == SEM_FAILED || client_sem == SEM_FAILED || return_sem == SEM_FAILED) {
        bail_out(EXIT_FAILURE, "sem_open");
    }

    /* ############ Ready for clients ############ */
    DEBUG("Server Ready!\n");
    struct Client *pre = NULL;
    struct Client *cur = NULL;

    while (sig_caught == 0) {

        /* ###### Begin critical Section ###### */

        if (sem_wait(server_sem) == -1) {
            if (errno == EINTR) continue;
            bail_out(EXIT_FAILURE, "sem_wait");
        }

        /* new client */
        if (shared->clientID == -1) {
            cur = (struct Client *) calloc (1, sizeof (struct Client));
            if (cur == NULL) {
                bail_out(EXIT_FAILURE, "calloc on creation of new client");
            }
            cur->clientID = client_count++;
            cur->next = clients;
            clients = cur;
            DEBUG("Created new client with ID %d\n", cur->clientID);

        } else {	/* existing client */
            cur = clients;
            while (cur != NULL && cur->clientID != shared->clientID) {
                pre = cur;
                cur = cur->next;
            }
            if (cur == NULL) bail_out(EXIT_FAILURE, "Could not find client with ID %d", shared->clientID);
        }

        /* Check if client has terminated --> free resources */
        if (shared->terminate) {

            if (cur == clients){
                clients   = cur->next;
            } else {
                pre->next = cur->next;
            }

            free_words(cur);
            free(cur);
            DEBUG("Finished free resources of client %d\n", shared->clientno);

            shared->terminate = false;
            if (sem_post(client_sem) == -1){
                bail_out(EXIT_FAILURE, "sem_post");
            }
            continue;
        }

        switch (shared->status) {
            case New:
                new_game(cur);		/* client wants a new game */
                break;
            case Open:
                calculate_results(cur, shared->tried_char);		/* client sends a guess */
                break;
            default:
                assert(0 && "status from client may only be in {New, Open}");
        }

        shared->clientID = cur->clientID;
        shared->status = cur->current_game.status;
        shared->errors = cur->current_game.errors;
        strncpy(shared->word, cur->current_game.obscured_word, MAX_WORD_LENGTH);
        DEBUG("clientID %d ... status: %d, errors: %d, secret: \"%s\", obscured: \"%s\"\n", shared->clientno, shared->status, shared->errors, cur->current_game.secret_word, shared->word);

        if (sem_post(return_sem) == -1){
            bail_out(EXIT_FAILURE, "sem_post");
        }

        /*** End critical Section ***/
    }

    DEBUG("Caught signal, shutting down\n");
    free_resources();
    return EXIT_SUCCESS;
}
