/**
 * @file fileRead.c
 * @author Yannick Schwarenthorer
 * @date 11.06.2016
 *
 * @brief Implementation of the fileRead module
 **/

#include "fileRead.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int readFile(FILE *f, struct Buffer *buffer, size_t maxLineLength, bool all_characters) {
	
	char tmpBuffer[maxLineLength];
	char *linePointer;
	size_t lineLength = 0;
	int c;

	while ( feof(f) == 0 ) {
		
		c = fgetc(f);
		if (lineLength + 1 >= maxLineLength || ferror(f) != 0 ) {
			return -1;
		}
		
		if ( (c == '\n' || c == '\r' || c == EOF) && lineLength > 0 ) {
			/* correctly terminate the String */
			tmpBuffer[lineLength] = '\0';
			lineLength++;
			
			/* resize buffer->content to fit one more string pointer */
			buffer->content = realloc(buffer->content,(buffer->length + 1) * sizeof(char **));
			if(buffer->content == NULL) {
				return -1;
			}

			/* initialize memory for a new String to store the characters of the current line */
			linePointer = calloc( lineLength, sizeof (char));
			if(linePointer == NULL ) {
				return -1;
			};
		
			strncpy(linePointer, &tmpBuffer[0], lineLength);
			buffer->content[buffer->length] = linePointer;
			buffer->length++;
			lineLength = 0;
					
		} else {
			if (all_characters || isalpha(c) || c == ' ') {
				tmpBuffer[lineLength] = (char)toupper(c);
				lineLength++;
			}
		}		
	}
	
	return 0;
}


void printBuffer(struct Buffer *buffer, FILE *stream) 
{
	for(int i=0; i < buffer->length; i++) {
		(void) fprintf(stream, "%s\n", buffer->content[i]);
	}
}

void freeBuffer(struct Buffer *buffer) {
	if (buffer != NULL) {
		for (int i=0; i < buffer->length; i++) {
			free(buffer->content[i]);
		}
		
		free(buffer->content);	
	}
	return;
}
