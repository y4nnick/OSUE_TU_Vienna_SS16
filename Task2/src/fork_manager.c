/**
 * @file fork_manager.c
 * @brief Source file for fork manager
 * @author Yannick Schwarenthorer 1229026
 * @date 2016-05-07
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include "fork_manager.h"

pid_t ownFork(fork_func_callback_t fork_function, fork_func_param_t params){

    pid_t pid = fork();

    switch (pid) {
        case -1:
            (void) fprintf(stderr, "Cannot fork!\n");
            exit(EXIT_FAILURE);
        case 0:
            // child tasks ...
            break;
        default:
            // parent tasks ...
            break;
    }

    switch(pid){
        case 0:
            exit(fork_function(params));
        default:
            return pid;
    }
}


int redirectOutput(pipe_t p, FILE *f, pipe_channel_t channel){

    //Get file descriptor
    int fd = fileno(f);
    (void) close(fd);

    /* Redirect read end of pipe ... */
    if(channel & READ_CHANNEL) {
        if(dup2(p[0], fd) == -1) {
            fprintf(stderr, "Could not redirect read channel to file descriptor %d\n", fd);
            return -1;
        }
    }

    /* redirect write end of pipe ... */
    if(channel & WRITE_CHANNEL) {
        if(dup2(p[1], fd) == -1) {
            fprintf(stderr, "Could not redirect file descriptor %d to write channel\n", fd);
            return -1;
        }
    }

    return 0;
}

int wait_for_child(pid_t child)
{
    int status;

    if (waitpid(child, &status, 0) == -1) {
        return -1;
    }

    return WEXITSTATUS(status);
}

int open_pipe(pipe_t p){
    return pipe(p);
}

void close_pipe(pipe_t p, pipe_channel_t c){
    if(c & READ_CHANNEL) {
        (void) close(p[0]);
    }

    if(c & WRITE_CHANNEL) {
        (void) close(p[1]);
    }
}
