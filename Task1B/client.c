/**
 * name     client
 * @file    client.c
 *
 * @Author  Yannick Schwarenthorer, 1229026
 * @date    11. April 2016
 * @brief   Client for mastermind
 * @detail  This client tries to guess the right combination (currently only static tries)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define MAX_TRIES (35)
#define SLOTS (5)

#define EXIT_PARITY_ERROR (2)
#define EXIT_GAME_LOST (3)
#define EXIT_MULTIPLE_ERRORS (4)

/* === Macros === */

#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* Length of an array */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

/* === Global Variables === */

extern int errno;

/* Name of the program */
static const char *progname = "client"; /* default name */

/* File descriptor for server socket */
static int sockfd = -1;

/* File descriptor for connection socket */
static int connfd = -1;

int roundNumber = 0;

enum { beige, darkblue, green, orange, red, black, violet, white };

/* === Prototypes === */

/**
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * @brief Creates the connection to server
 * @details Creates the connection to the given server on the given port
 * @param hostname the hostname of the server
 * @param port the portname of the server
 * @return the filedescriptor if connected successfully
 */
int createConnection(const char *hostname, const int port);

/**
 * @brief Generates and returns the next guess
 * @return the next guess
 */
uint16_t nextGuess();

/**
 * @brief Calculates the parity bit for a color scheme
 * @param color the color scheme
 * @return the paritybit
 * @details the parity bit will be calculatted by connecting all the bits from the color with a XOR
 */
uint16_t getParity(uint16_t color);

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success, EXIT_PARITY_ERROR in case of an parity
 * error, EXIT_GAME_LOST in case client needed to many guesses,
 * EXIT_MULTIPLE_ERRORS in case multiple errors occured in one round
 */
int main(int argc, const char * argv[]) {

    if(argc > 0) progname = argv[0];

    //Handle args
    if (argc != 3) {
        errno = 0;
        fprintf(stderr, "Usage: %s <server-hostname> <server-port>\n", progname);
        return EXIT_FAILURE;
    }

    //Read out and check port
    char *ptr;
    int port = strtol(argv[2],&ptr,10);
    if(port > 65535 || port < 1){
        bail_out(EXIT_FAILURE,"Port must be in the TCP/IP port range (1.65535)");
    }

    connfd = createConnection(argv[1],port);

    //Game start
    uint8_t result;
    uint16_t guess;

    while(1){
        roundNumber++;

        //Send guess
        guess = nextGuess();
        send(connfd,&guess,2,0);
        DEBUG("Sent 0x%x\n", guess);

        //Get result
        recv(connfd, &result, 1, 0);
        DEBUG("Got byte 0x%x\n", result);


        // Check result errors
        errno = 0;
        switch (result >> 6) {
            case 1:
                fprintf(stderr, "Parity error\n");
                return 2;
            case 2:
                fprintf(stderr, "Game lost\n");
                return 3;
            case 3:
                fprintf(stderr, "Parity error AND game lost!\n");
                return 4;
            default:
                break;
        }

        // Check if game is won
        if ((result & 7) == 5 ) {
            printf("Runden: %d\n", roundNumber);
            return 0;
        }
    }

    return 1;
}

uint16_t nextGuess(){
    uint16_t color = 0x0000;
    return color | getParity(color);
}

uint16_t getParity(uint16_t color){

    uint16_t parity = 0;

    uint16_t one = 1;

    for(int x = 0; x < 14; x++){
        parity ^= (color ^ one);
        one <<= 1;
    }

    return parity;
}



int createConnection(const char *hostname, const int port){

    int sock = 0;
    int success = 0;

    //Port
    char port_string[6];
    sprintf(port_string,"%i",port);

    //Address info
    struct addrinfo *ai = malloc(sizeof(struct addrinfo));
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;

    if (getaddrinfo(hostname, port_string, &hints, &ai) != 0) {
        bail_out(EXIT_FAILURE, "getaddrinfo");
    }

    struct addrinfo *ai_head = ai;

    if (ai == NULL) {
        bail_out(EXIT_FAILURE, "Address infromation is empty");
    }

    do {
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock >= 0) {
            if (connect(sock, ai->ai_addr, ai->ai_addrlen) >= 0){
                success = 1;
                break;
            }
        }
    } while ((ai = ai->ai_next) != NULL);

    freeaddrinfo(ai_head);

    if (success == 0) {
        bail_out(EXIT_FAILURE, "No address for valid socket found");
    }

    return sock;
}


static void free_resources(void)
{
    /* clean up resources */
    DEBUG("Shutting down server\n");
    if(connfd >= 0) {
        (void) close(connfd);
    }
    if(sockfd >= 0) {
        (void) close(sockfd);
    }
}

static void bail_out(int exitcode, const char *fmt, ...)
{
    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }

    (void) fprintf(stderr, "\n");

    free_resources();
    exit(exitcode);
}
