/**
 * @file hangman-common.h
 * @author Yannick Schwarenthorer
 * @date 11.06.2016
 *
 * @brief This module defines common constants and macros for both hangman-server and hangman-client.
 **/

#ifndef HANGMAN_COMMON_H
#define HANGMAN_COMMON_H

#include <stdbool.h>
#include <stdint.h>

/* === Constants === */
#define MAX_ERROR (9)           /**< number of errors a client can make until the game is over */
#define MAX_WORD_LENGTH (50)	/**< max. length a word to guess may be */

#define PERMISSION (0600)							    /**< UNIX file permission for semaphores and shm */
#define SHM_NAME   ( "/1229026_hangman_shm" )			/**< name of the shared memory */
#define SERVER_SEM ( "/1229026_hangman_server_sem" )    /**< name of the server semaphore */
#define CLIENT_SEM ( "/1229026_hangman_client_sem" )	/**< name of the client semaphore */
#define RETURN_SEM ( "/1229026_hangman_return_sem" )	/**< name of the return semaphore */

/* === Structures === */

/**
 * @brief Enumeration describing the states of a game
 */
enum GameStatus {
            New,			/**< set by client -> Requests a new game */
            Open,			/**< set by server after deciding upon a new word */
            Impossible,		/**< set by server if a new game is requested and no new word is available */
            Lost,			/**< set by server if MAX_ERROR is exceeded */
            Won,			/**< set by server if the word is guessed correctly */
};

/**
 * @brief Structure used for client-server communication. Will be stored in shared memory and allow client and server
 * to exchange all the relevant informations needed for gameplay.
 */
struct Hangman_SHM {
    unsigned int errors;			/**< the number of errors that this client already made */
    int clientID;					/**< the clientID */
    enum GameStatus status;			/**< the status of the game */
    char tried_char;				/**< the character guessed by the client */
    char word[MAX_WORD_LENGTH];		/**< the answer of the server */
    bool terminate;					/**< a flag to allow client and server tell each other if they terminated */
};


/* === Macros === */
#ifdef _ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

#endif // HANGMAN_COMMON_H
