/**
 * @file websh.c
 * @brief executes a programm and transform its output for the web
 * @author Yannick Schwarenthorer 1229026
 * @date 2016-05-07
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fork_manager.h"

/* === Macros === */

#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

/* === Structures === */

struct worker_params {
    pipe_t pipe; /**< pipe to handle communication between processes */
    char *cmd /**< command to execute */;
};

/* === Constants === */

/**
* @brief Maximum line length
*/
#define MAX_LENGTH 255


/* === Global Variables === */

/**
* @brief struct for command line options
*/
static struct {

    int opt_e; /**< 1 (true) if programm called with -e  */
    int opt_h; /**< 1 (true) if programm called with -h  */
    int opt_s; /**< 1 (true) if programm called with -s  */
    char *s_word; /**< if called with -s search for output lines containing s_word ... */
    char *s_tag; /**< and wrap the line withing s_tag  */

} options;

/* Name of the program */
static const char *progname = "websh"; /* default name */


/* === Prototypes === */

/**
* @brief starts a worker for the given command
*
* @param cmd command to be executed.
* @details uses progname global var
*
* @return -1 if
* @return -1 if pipe can't be created or one of the processes could not be forked, 0 otherwise (even if one of the processes returns nonzero)
*/
static int startWorker(char *cmd);

/**
* @brief Parses command line Args and stores them in the global options struct
*
* @param argc argument counter
* @param argv argument array
* @details uses global options struct
*
* @return 0 if all arguments are correctly, -1 otherwise
*/
static int parseArgs(int argc, char **argv);

/**
* @brief Prints the usage message
* @details uses progname global var
*/
void usage(void);

/**
* @brief Callback the execution of a command
* @param param worker_params arguments for the command
* @return 1 if something failed, otherwise execlp takes over
*/
static unsigned int executeCommand(fork_func_param_t param);

/**
* @brief Callback for formating the ouput (redirected to the pipe)
* @param param  worker_params arguments for formating
* @return 1 if redirect fails, 0 otherwise
*/
static unsigned int formatOutput(fork_func_param_t param);

/**
* @brief Remove trailing newline char in string
* @param str string which should be trimmed
*/
static void trimStr(char *str);


/* === Implementations === */

static int startWorker(char *cmd) {

    //Params
    struct worker_params params;
    int status;
    pid_t executerProcess,formaterProcess;

    trimStr(cmd);
    params.cmd = cmd;

    //Create pipe
    if(open_pipe(params.pipe) == -1) {
        (void) fprintf(stderr, "%s: Could not create pipe\n", progname);
        return -1;
    }

    //Flush for clean start
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    //Executer
    if((executerProcess = ownFork(executeCommand,&params)) == -1){
        (void) fprintf(stderr,"%s: Could not start executer process\n", progname);
        close_pipe(params.pipe,ALL_CHANNELS);
        return -1;
    }

    //Output Worker
    if((formaterProcess = ownFork(formatOutput,&params)) == -1){
        (void) fprintf(stderr,"%s: Could not start format process\n", progname);

        //Wait for execture
        if(wait_for_child(executerProcess) == -1) {
           (void) fprintf(stderr, "%s: Error waiting for executer process to finish\n", progname);
        }

        close_pipe(params.pipe,ALL_CHANNELS);
        return -1;
    }

    close_pipe(params.pipe,ALL_CHANNELS);

    if((status = wait_for_child(executerProcess)) != 0) {
        (void) fprintf(stderr, "%s: Executer process returned %d\n", progname, status);
    }

    if((status = wait_for_child(formaterProcess)) != 0) {
        (void) fprintf(stderr, "%s: Format process returned %d\n", progname, status);
    }

    return 0;
}

static int parseArgs(int argc, char **argv) {

    char c;
    char *arg_s = NULL;

    //Store programm name
    if(argc > 0){
        progname = argv[0];
    }

    while( (c = getopt(argc,argv,"ehs:")) != -1){

        switch(c){
            case 'e':
                if(options.opt_e == 1){
                    (void) fprintf(stderr, "Option 'e' only allowed once\n");
                    return -1;
                }
                options.opt_e = 1;
                break;
            case 'h':
                if(options.opt_h == 1){
                    (void) fprintf(stderr, "Option 'h' only allowed once\n");
                    return -1;
                }
                options.opt_h = 1;
                break;
            case 's':
                if(options.opt_s == 1){
                    (void) fprintf(stderr, "Option 's' only allowed once\n");
                    return -1;
                }
                options.opt_s = 1;
                arg_s = optarg;
                break;
            default:
                return -1;
        }
    }

    //No programm args allowed
    if(optind != argc){
        return -1;
    }

    if(options.opt_s == 1){

        //Check if argument of s is given
        if(arg_s == NULL){
            return -1;
        }

        //Check if argument of s is in the format WORD:TAG?
        int correct = -1;
        options.s_word = arg_s;
        while(*arg_s != '\0'){

            if(*arg_s == ':'){
                correct = 1;
                *arg_s = '\0';
                options.s_tag = arg_s + 1;
            }

            arg_s++;
        }

        if(correct == -1){
            (void) fprintf(stderr, "Argument for option 's' has to be in the format WORD:TAG\n");
            return -1;
        }
    }

    return 0;
}

static unsigned int executeCommand(fork_func_param_t param)
{
    // Cast params
    struct worker_params *params = (struct worker_params *) param;

    //Close read end of the pipe
    close_pipe(params->pipe, READ_CHANNEL);

    //Forward stdout to write end
    if(redirectOutput(params->pipe, stdout, WRITE_CHANNEL) == -1) {
        return 1;
    }

    //Directly call sh to prevent parsing the command string
    (void) execlp("/bin/sh", "sh", "-c", params->cmd, (char *) NULL);

    (void) fprintf(stderr, "ERROR should not be reached");
    return 1;
}

void usage(void)
{
    (void) fprintf(stderr, "Usage: %s [-e] [-h] [-s WORD:TAG]\n", progname);
}

/**
* @brief Main entry point
*
* @param argc argument counter
* @param argv argument array
* @details uses opts global var
*
* @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise
*/
int main(int argc, char **argv) {

    //Check arguments
    if(parseArgs(argc,argv) < 0){
        usage();
        return EXIT_FAILURE;
    }

    DEBUG("options.opt_e: %x\n", options.opt_e);
    DEBUG("options.opt_h: %x\n", options.opt_h);
    DEBUG("options.opt_s: %x\n", options.opt_s);
    DEBUG("options.s_word: %s\n", options.s_word);
    DEBUG("options.s_tag: %s\n", options.s_tag);

    if(options.opt_e == 1){
        (void) fprintf(stdout, "<html><head></head><body>\n");
    }

    //Read command output
    char cmd[MAX_LENGTH];
    while(fgets(cmd,MAX_LENGTH,stdin) != NULL){

        DEBUG("!!!!! Start childs with Command: %s\n", cmd);

        //start child workers
        if(startWorker(cmd) == -1){
            return EXIT_FAILURE;
        }
    }

    if(options.opt_e == 1){
        (void) fprintf(stdout, "</body></html>\n");
    }

    return EXIT_SUCCESS;
}

static unsigned int formatOutput(fork_func_param_t param)
{
    struct worker_params *params = (struct worker_params *) param;
    char output[MAX_LENGTH];

    //Close write end of pipe
    close_pipe(params->pipe, WRITE_CHANNEL);

    //Forward stdin to read from pipe
    if(redirectOutput(params->pipe, stdin, READ_CHANNEL) == -1) {
        return 1;
    }

    //Print command if option h
    if(options.opt_h) {
        (void) fprintf(stdout, "<h1>%s</h1>\n", params->cmd);
    }

    /* Read cmd's output line by line */
    while(fgets(output, MAX_LENGTH, stdin) != NULL) {

        trimStr(output);

        //Add surrounding tags to word
        if(options.opt_s && strstr(output, options.s_word)) {
            (void) fprintf(stdout, "<%s>%s</%s><br />\n", options.s_tag, output, options.s_tag);
        } else {
            (void) fprintf(stdout, "%s<br />\n", output);
        }

    }

    return 0;
}

static void trimStr(char *str)
{
    while(str[strlen(str) - 1] == '\n') {
        str[strlen(str) - 1] = '\0' ;
    }
}
