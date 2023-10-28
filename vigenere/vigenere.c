/**
 * @file vigenere.c
 * @author Arslan Smajevic <e12127678@student.tuwien.ac.at>
 * @date 14.10.2023
 *
 * @brief (De)Encryption program.
 * 
 * This program takes a speffic key and ciphers the given text to a file or stdout.
 * ./vigenere "arslan" input.txt (for Example)
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/**
* Usage function.
* @brief Prints a message to stderr and stops the program with EXIT_FAILURE.
* @details This function is used for usage messages - meaning, what the user has invoked is not allowed. 
*          Message contains the detailed error.
*
* @param error string
*/
void usage(char *error){
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
}

/**
* Argument handler and assigner.
* @brief Takes the arguments given via bash and handles and assigns the accordingly.
* @details Handles the arguments on the following synopsis: vigenere [-d] [-o outfile] key [file...].
*          If the synopsis is not satisfied, it will exit with EXIT_FAILURE and the corresponding usage message.
*          decrypt, outfile, key, input_files will be assigned.
*
* @param argc number of arguments
* @param argv array of all arguments
* @param outfile output file
* @param key key argument
* @param input_files starting index of input files
*/
void handle_arguments(int argc, char* argv[], int *decrypt, char **outfile, char **key, int *input_files){
    int option;
    int key_position = 1;

    while ((option = getopt(argc, argv, "do:")) != -1) {
        switch (option) {
            case 'd':
                if(*decrypt == -1){
                    *decrypt = 1;
                }
                else{
                    usage("./vigenere: option -d was declared more than once.");
                }
                break;

            case 'o':
                *outfile = optarg;
                break;

            case '?':
                exit(EXIT_FAILURE);

            default:
               break;
        }
    }


    if(*decrypt != -1){
        key_position++;
    }
    if(*outfile != NULL){
        key_position = key_position + 2;
    }

    int input_start = key_position;

    if(key_position < argc){
        *key = argv[key_position];
        input_start++;
    }
    else
    {
        usage("./vigenere: the key was not specified.");
    }

    if(input_start < argc){
        *input_files = input_start;
    }
}

/**
* Converting a letter to [0-25] Range
* @brief Covnerts a character to a range [0-25].
* @details The function will turn all uppercase and lowercase alphabetical characters to a range of [0-25] and 
*          give the corresponding value back.
*
* @param ch parsed character
* @return the corresponding integer of the range [0-25], otherwise -1.
*/
int convert_char(char ch){
    if(ch >= 65 && ch <= 90){
        return (int)(ch) - 65;
    }
    if(ch >=97 && ch <= 122){
        return (int)(ch) - 97;
    }
    return -1;
}

/**
* Lower or uppercase
* @brief Returns the lowbound of the range needed.
* @details This function will check for lower or uppercase and return accordingly 65 or 97 (starting ASCII values used
*          for calculation). 
*
* @param ch parsed character
* @return 65 for uppercase, 97 lowercase, otherwise -1.
*/
int lower_upper(char ch){
    if(ch >= 65 && ch <= 90){
        return 65;
    }
    if(ch >=97 && ch <= 122){
        return 97;
    }
    return -1;
}

/**
* Special Character Check
* @brief Checks whether the parsed character is special.
* @details This function checks for .,:-!=?% and whitespaces, as well as for numbers and \n characters. 
*
* @param ch parsed character
* @return 1 on ch=special, -1 otherwise.
*/
int special_character(char ch){

    if(ch >= 48 && ch <= 57){
        return 1;
    }

    if(ch == '.'){
        return 1;
    }
    if(ch == ','){
        return 1;
    }
    if(ch == ':'){
        return 1;
    }
    if(ch == '-'){
        return 1;
    }
    if(ch == '!'){
        return 1;
    }
    if(ch == '='){
        return 1;
    }
    if(ch == '?'){
        return 1;
    }
    if(ch == '%'){
        return 1;
    }
    if(ch == ' '){
        return 1;
    }
    if(ch == '\n'){
        return 1;
    }
    
    return 0;
}

/**
* (De)Encryption Function
* @brief Takes a key and (de)encrypts the letter accordingly.
* @details This function encrpyts one character on the following form: E[i] = (L[i] + K[i mod lK] mod 26.
*          If the decrpyt flag is on (decrypt = 1), then it decrypts on the form: D[i] = (L[i] − K[i mod lK] + 26) mod 26.
*          E[i] is the encrypted character. L[i] is the parsed character(letter). i is the index_counter (current index of)
*          K is the key.
*
* @param decrypt decrypt flag
* @param key (de)encryption key
* @param letter character to be encrypted
* @param index_counter index of the letter
* @param key_length length of the key
* @return (de)encrypted character
*/
char de_encryption(int *decrypt, char *key, char letter, int index_counter, int key_length){
    if(*decrypt != 1){
        return (char) (((convert_char(letter) + convert_char(key[index_counter % key_length])) % 26) + lower_upper(letter));
    }
    else{
        return (char) (((convert_char(letter) - convert_char(key[index_counter % key_length]) + 26) % 26) + lower_upper(letter));
    }
}

/**
* Printing Output
* @brief Takes a character and prints it to an output.
* @details This function takes one character and prints it to stdout or specified output file. Note that, this is
*          for writing one character at a time. If writing to a file, first opening should be set to 0.
*
* @param ch character to be written
* @param output output file
* @param first_opening 0 if writing to an output file
* @param input input file (in case output crashes)
*/
void print_output(char ch, char *output, int *first_opening, FILE *input){
    
    if(output != NULL){
        FILE *output_file;
        if(*first_opening == 0){
            output_file = fopen(output, "w");

            if(output_file == NULL){
                if(input != NULL){
                    fclose(input);
                }
                usage("./viginere: Erorr occured while opening the output file.");
            }
            *first_opening = 1;
        }
        else{
            output_file = fopen(output, "a");
        }
        fprintf(output_file, "%c", ch);
        fclose(output_file);
    }
    else
    {
        fprintf(stdout, "%c", ch);
    }
}

/**
* Ciphering text from a file
* @brief Ciphers the data from the file with the parsed key to the specified output.
* @details This function opens a speffic input file and reads character for character. Every character is
*          (de)encrypted along the way and printed to the output or stdout.

* @param key (de)encryption key
* @param key_length length of the key
* @param decrypt decrypt flag (1 for present)
* @param file input file
* @param outfile output file
* @param first_opening 0 for deleting previous data from output file, 1 for adding
*/
void cipher_text_from_file(char *key, int key_length, int *decrypt, char *file, char *outfile, int *first_opening){
    FILE* input;
    char ch;
    input = fopen(file, "r");

    if(input == NULL){
        usage("./viginere: Erorr occured while opening the input file.");
    }

    char encrypt_char;
    int index_counter = 0;

    while ((ch = fgetc(input)) != EOF) {
        if(special_character(ch) == 1){
            encrypt_char = ch;
            if(encrypt_char == '\n')
            {
                index_counter = 0;
            }
            else
            {
                index_counter++;
            }
        }
        else{
            
            encrypt_char = de_encryption(decrypt, key, ch, index_counter, key_length);
            index_counter++;
        }
        
        //printf("Encrypted: %c\n", encrypt_char);
        print_output(encrypt_char, outfile, first_opening, input);
    }

    // fprintf(stdout, "\n");
    fclose(input);
}

/**
* Ciphering text from stdin
* @brief Ciphers the data from stdin with the parsed key to the specified output.
* @details This function reads character for character from stdin. Every character is
*          (de)encrypted along the way and printed to the output or stdout.

* @param key (de)encryption key
* @param key_length length of the key
* @param decrypt decrypt flag (1 for present)
* @param file input file
* @param outfile output file
* @param first_opening 0 for deleting previous data from output file, 1 for adding
*/
void cipher_text_from_stdin(char *key, int key_length, int *decrypt, char *outfile, int *first_opening){
    int ch;
    char encrypt_char;
    int index_counter = 0;

    while ((ch = getchar()) != EOF) {
        if(special_character(ch) == 1){
            // fprintf(stdout, "%c\n", ch);
            encrypt_char = ch;
            if(encrypt_char == '\n')
            {
                index_counter = 0;
            }
            else
            {
                index_counter++;
            }
        }
        else{
            encrypt_char = de_encryption(decrypt, key, ch, index_counter, key_length);
            index_counter++;
        }
        
        //printf("Encrypted: %c\n", encrypt_char);
        print_output(encrypt_char, outfile, first_opening, NULL);
    }
}

/**
* Uppercasing
* @brief Takes a string and uppercases it.
* @details This function takes a string and uppercases it. If there non-alphabetical characters, usage is called
*          and the program exits with EXIT_FAILURE. Note: the key is case-insensitive.
*
* @param key (de)encryption key
* @param key_length length of the key
*/
void change_key(char *key, size_t key_length){
    for(int i=0; i<key_length; i++){
        if(convert_char(key[i]) == -1){
            usage("./vigenere: parsed key is invalid.");
        }
        key[i] = toupper(key[i]);
    }
}

/**
 * @brief Starting point of the program.
 * @details Entry point of the program. 
 * @param argc counter of arguments
 * @param argv arguments and options.
 */
int main(int argc, char* argv[])
{
    int decrypt = -1;
    char *outfile = NULL;
    char *key = NULL;
    int input_files = -1;


    handle_arguments(argc, argv, &decrypt, &outfile, &key, &input_files);

    size_t key_length = strlen(key);

    change_key(key, key_length);

    

    int first_opening = 0;

    if(input_files != -1){
        for(int i = input_files; i < argc; i++){
            cipher_text_from_file(key, key_length, &decrypt, argv[i], outfile, &first_opening);
        }
    }
    else{
        cipher_text_from_stdin(key, key_length, &decrypt, outfile, &first_opening);
    }

    return 0;
}