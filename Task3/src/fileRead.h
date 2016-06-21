/**
 * @file fileRead.h
 * @author Yannick Schwarenthorer
 * @date 11.06.2016
 *
 * @brief Module for reading the content of a file into a struct buffer defined in this header.
 * @details This module contains methods for reading from a file into a buffer and freeing the buffer afterwards.
 **/

#ifndef FILEREAD_H
#define FILEREAD_H

#include <stdio.h>
#include <stdbool.h>


/**
 * @brief A structure to store lines of strings.
 */
struct Buffer
{
	char **content;	/**< Pointer to the String array. */
	int length;	/**< Can be used to keep track of the number of currently stored strings. */
};

/**
 * @brief Reads the content of a FILE* into a struct buffer.
 * @details The content of FILE* is read line by line into the specified Buffer *. The size of 
 * buffer->content gets increased for every line and buffer->length gets incremented.
 * @param *f The already opened file to read from.
 * @param *buffer A struct of type Buffer to store the data in.
 * @param maxLineLength The maximum length of a line that can be read from the file
 * @param all_characters if true, all characters are read, only normal english characters otherwise
 * @return A value different from 0 if an error (such as memory allocation or too long lines) occurs, 0 otherwise.
 */
int readFile(FILE *f, struct Buffer *buffer, size_t maxLineLength, bool all_characters);


/**
 * @brief Frees the allocated space of a buffer and all the content inside
 * @param *buffer A pointer to the buffer to be freed.
 * @return nothing
 */
void freeBuffer(struct Buffer *buffer);

/**
 * @brief Prints the given Buffer to the given stream. Lines are separated by '\n'
 * @param *buffer the buffer
 * @param *stream the stream to print to
 */
void printBuffer(struct Buffer *buffer, FILE *stream);
	
#endif /* FILEREAD_H */
