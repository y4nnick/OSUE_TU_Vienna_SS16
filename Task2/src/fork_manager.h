/**
 * @file fork_manager.h
 * @brief header file for fork manager
 * @author Yannick Schwarenthorer 1229026
 * @date 2016-05-07
 */

#ifndef FORK_MANAGER_H
#define FORK_MANAGER_H
#endif
#include <sys/types.h>

/**
* @brief enum of pipe channels, 01 = read, 10 = write, 11 = all;
*/
typedef enum pipe_channel {
    READ_CHANNEL = 1,
    WRITE_CHANNEL,
    ALL_CHANNELS
} pipe_channel_t;

/**
* @brief typedef pipe_t
*/
typedef int pipe_t[2];

/**
* @brief void * Pointer with the generic params for the callback
*/
typedef void * fork_func_param_t;

/**
* @brief type definition of a forkable callback.
* @param param a generic void * Pointer that holds params for callback
* @return exit value the forked process shall exit() with
*/
typedef unsigned int (*fork_func_callback_t)(fork_func_param_t param);

/**
 * @brief forks a process and executes the given callback
 * @param fork_function callback which should be executed
 * @param paras parameters for callback
 * @return the value from the function callback if child process or the pit of the child if in parent process
 */
pid_t ownFork(fork_func_callback_t fork_function, fork_func_param_t params);

/**
* @brief wrapper around waitpid
* @param child pid of child process
* @return exit code of child, or -1 if waitpid didn't work
*/
int wait_for_child(pid_t child);

/**
* @brief wrapper around pipe() for opening a pipe
* @param pipe pipe_t that hold the pipe
* @return 0 on success, -1 else
*/
int open_pipe(pipe_t pipe);

/**
* @brief Close channel of pipe
* @param p the pipe
* @param c channel which should be closed
*/
void close_pipe(pipe_t, pipe_channel_t c);

/**
* @brief redirect stream to pipe
* @param p pipe to redirect to or from
* @param fd strem to redirect to or from
* @param c channel to be redirected
* @return 0 on success, -1 else
*/
int redirectOutput(pipe_t p, FILE *fd, pipe_channel_t c);
