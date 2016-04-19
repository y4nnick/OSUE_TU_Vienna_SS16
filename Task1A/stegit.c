/**
 * name     stegit
 * @file    stegit.c
 *
 * @Author  Yannick Schwarenthorer, 1229026
 * @date    11. April 2016
 * @brief   Simple chiffre module
 * @detail  This program encrypts plain text into encrypted text and reverse. The output could be written to a file
 */
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <string.h>

#define MAXLENGTH (300)

static void usage(void);
void encryptText(void);
void decryptText(void);
const char * encryptChar(char plain);
const char decryptChar(char* chiffreChar);
char *command = "<not set>";

char* chiffre[28] = {
        "die", "sonne", "der", "das", "um",
        "neun", "Uhr", "aufgeht", "blumme", "baum",
        "Alice", "Bob", "schauen", "schlafen", "und",
        "darum", "deshalb", "Stein", "Baum", "blau", "gelb",
        "groß", "klein", "Fahne", "stehen",
        "zehn", "Mond", "Ende"
};

/**
 * @brief       Main entry point
 * @param argc  Argument count
 * @param argv  Argument array
 * @return      0 if success otherwise > 0
 */
int main(int argc, char **argv) {

    if(argc > 0) command = argv[0];

    int c;
    int opt_f = 0;
    int opt_h = 0;
    int opt_o = 0;
    char* val_o = NULL;
    FILE* file = NULL;

    srand(time(NULL));

    while ((c = getopt(argc,argv,"fho:")) != -1){

        switch(c){
            case 'f':
                opt_f = 1;
                break;
            case 'h':
                opt_h = 1;
                break;
            case 'o':
                opt_o = 1;
                val_o = optarg;
                break;
            default:
                usage();
                break;
        }
    }

    //File given
    if(opt_o){
        if(val_o == NULL){
            (void) printf("Missing argument for -o");
            usage();
        }else{
            file = fopen(val_o,"w");

            if(file == NULL){
                (void) printf("Could not open file %s",val_o);
            }

            stdout = file;
        }
    }

    //Only find xor hide mode is allowed
    if(opt_f && opt_h){
        usage();
    }

    //Hide mode
    if(opt_h){
        encryptText();
    }else if(opt_f){ //Find mode
        decryptText();
    }else{
        usage();
    }

    //Close file
    if(opt_o && file != NULL){
        (void) fclose(file);
    }

    return 0;
}

/**
 * @name        encryptText
 * @brief       encrypts text from the standart input
 * @detail      encrypts text from the standart input and writes the outcome to stdout
 */
void encryptText(void){

    char buff[MAXLENGTH];
    int dotcount = 0;
    int i = 0;

    while(fgets(buff, MAXLENGTH, stdin) != NULL && i > -1){

        i = 0;
        while(i < MAXLENGTH && buff[i] != EOF && buff[i] != '\n' && buff[i] != '\0'){

            //Encrypted character
            (void) fputs(encryptChar(buff[i]),stdout);

            //Dot
            if(dotcount >= 5 && dotcount <= 15){
                if(rand() % 3 == 0 || dotcount == 15){
                    (void) fputs(".",stdout);
                    dotcount = 0;
                }
            }

            //Space after word
            (void) fputs(" ",stdout);

            i++;
            dotcount++;
        }

        if(buff[i] == EOF || buff[i] == '\n'){
            (void) fputs("\n",stdout);
            i = -1;
        }
    }
}

/**
 * @name        decryptText
 * @brief       decrypts text from the standart input
 * @detail      decrypts text from the standart input and writes the outcome to stdout
 */
void decryptText(void) {
    char buff[MAXLENGTH];

    int end = 0;

    char* wordstart = &buff[0];

    while( fgets(buff,MAXLENGTH,stdin) != NULL){


        int i = 0;

        while(i < MAXLENGTH && !end){

            //End of word with space or dot
            if(buff[i] == ' ' || buff[i] == '.'){
                buff[i] = '\0';
            }

            if(buff[i] == '\0'){
                if(wordstart != &buff[i]){
                    (void) fputc(decryptChar(wordstart),stdout);
                }

                if(i + 1 < MAXLENGTH ){
                    wordstart = &buff[i+1];
                }
            }

            if(buff[i] == EOF){
                buff[i] = '\0';
                end = 1;

                if(wordstart != &buff[i]){
                    (void) fputc(decryptChar(wordstart),stdout);
                }
            }

            i++;
        }
    }
}

/**
 * @name    decryptChar
 * @brief   decrypts a string
 * @param   chiffreChar the string which should be decrypted
 * @return  plain character
 * @detail  decrypts a string according to the chiffre array
 */
const char decryptChar(char* chiffreChar){

    int i = 0;

    while(i < 28){

        if(strcmp(chiffreChar,chiffre[i]) == 0){

            //Dot
            if(i == 27){
                return (char) 46;
            }

            //Space
            if(i == 26){
                return (char) 32;
            }

            return (char) i+97;
        }

        i++;
    }

    return '\0';
}


/**
 * @name    encryptChar
 * @brief   encrypts a single character
 * @param   plain the character which should be encrypted
 * @return  encrypted string
 * @detail  decrypts a single character according to the chiffre array
 */
const char* encryptChar(char plain){

    //ASCII Table under https://www.uni-due.de/hummell/infos/ascii/

    //Dot
    if(plain == 46){
        return chiffre[27];
    }

    //Space
    if(plain == 32){
        return chiffre[26];
    }

    //Upper char
    if(plain >= 65 && plain <= 90){
        return chiffre[plain-65];
    }

    //Lower char
    if(plain >= 97 && plain <= 122){
        return chiffre[plain - 97];
    }

    //any other char
    return "";
}

/**
 * @name    usage
 * @brief   Writes the usage information for this program to stderr
 * @details allways exits with EXIT_FAILURE
 */
static void usage(void) {
    (void) fprintf(stderr,"Usage: %s -f|-h [-o <filename>]\n",command);
    (void) fprintf(stderr,"\t-f\t\tfind mode\n");
    (void) fprintf(stderr,"\t-h\t\thide mode\n");
    (void) fprintf(stderr,"\t[-o <filename>]\t\toutput filename\n");
    exit(EXIT_FAILURE);
}
