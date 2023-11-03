/**
 * @file intmul.c
 * @author Arslan Smajevic <e12127678@student.tuwien.ac.at>
 * @date 14.10.2023
 *
 * @brief A program that takes two hexadecimal numbers and multiplies them.
 * @details This a recursive program that takes in two strings, treats them as hexadecimal numbers and multiplies them by making use of fork() and pipe().
 *          The base case of the recursion is when the numbers have a length of 1.
 *          Otherwise, program invokes four child processes that will be executed as ./intmul once again, but with other input.
 *          Pipes are used to write to children and to read from them.
 *          Parent process writes the original input splitted to the child processes and reads the result of the child processes.
 *          A child process can behave as parent process if the length of the inputs is greater than 1.
 *          
 *          A visual representation of the program:
 *                                                                                  (abcde, 1234)
 *                                        /                           /                             \                                   \
 *                                  (ab, 12)                     (ab, 34)                        (cd, 12)                           (cd, 34)
 *                     /          /         \        \              ...                             ...                                 ...
 *                  (a, 1)      (a, 2)      (b, 1)   (b, 2) 
 *                    ||          ||          ||       ||
 *                  a*1 = a     a*2 = 14    b*1 = b   b*2 = 16
 *
 *                  (ab, 12) = a * 16^2 + 14 * 16^1 + b * 16^1 + 16
 *                  (ab, 12) = a00 + 140 + b0 + 16
 *                  (ab, 12) = c06 = ab * 12
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * Structure of a hexadecimal pair
 * @brief The strucutre of a pair consisting of two numbers as strings and their length.
 */
typedef struct{
    char *first;
    char *second;
    size_t length;
} HeximalPair;

/**
 * Pipes used by the parent process.
 * @brief 
 */
int parent_pipe[4][2];

/**
 * Wait function.
 * @brief This function is intended for the parent process and it waits on the child processes.
 */
void waitOnChildren(void){
    int status, pid, error = 0;
    while ((pid = wait(&status)) != -1)
    {
        if (WEXITSTATUS(status) != EXIT_SUCCESS)
            error = 1;
    }

    if (error)
        exit(EXIT_FAILURE);

}

/**
 * Usage function.
 * @brief This function prints a message to the stderr and exits with 1 (stops the program).
 * @param erorr a message for the user.
 */
void usage(char *error){
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
}

/**
 * Check Input function.
 * @brief This function checks the given input and validates it. 
 * @details Exits on lineCounter = 1 || lineCounter > 2 indicating an invalid number of strings (numbers).
 *          Exits on empty strings (numbers).
 *          Exits if a number contains a character that does not fall in the range of a hexadecimal number. 
 *          Pair will not be changed.
 * @param pair hexadecimal pair.
 * @param lineCounter number of parsed lines
 */
void checkInput(HeximalPair pair, int lineCounter){
    if(lineCounter == 1){
        free(pair.first);
        usage("./intmul: one input given.");
    }

    if(lineCounter > 2){
        free(pair.first);
        free(pair.second);
        usage("./intmul: too many inputs given.");
    }

    if(strlen(pair.first) == 0 || strlen(pair.second) == 0){
        free(pair.first);
        free(pair.second);
        usage("./intmul: empty input found");
    }

    char* validateInput = "abcdefABCDEF0123456789";

    if((strspn(pair.first, validateInput) != strlen(pair.first))){
        free(pair.first);
        free(pair.second);
        usage("./intmul: first input is not valid.");
    }

    if((strspn(pair.second, validateInput) != strlen(pair.second))){
        free(pair.first);
        free(pair.second);
        usage("./intmul: second input is not valid.");
    }

}

/**
 * Appoint Nulls function.
 * @brief This function appoints a number of '0' characters to a string. ('543', 3) -> '000543' 
 * @param number string (number).
 * @param numberOfNulls number of nulls to be appointed.
 * @return Upon success a pointer to the new string is returned.
 * @warning The caller is responsible for freeing the result.
 */
char* appointNulls(char *number, int numberOfNulls){

    if(numberOfNulls < 0){
        usage("./intmul: cannot appoint negative number of nulls.");
    }

    if(numberOfNulls == 0){
        return NULL;
    }

    int originalLen = strlen(number);
    char* newNumber = (char*)malloc(originalLen + numberOfNulls + 1);
    if (!newNumber) {
        usage("./intmul: error while allocating memory");
    }

    for(int i = 0; i < numberOfNulls; i++) {
        newNumber[i] = '0';
    }
    strcpy(newNumber + numberOfNulls, number);

    return newNumber;  
}

/**
 * Is Power of Two function.
 * @brief This function checks whether the given number is a power of 2. 
 * @param n number.
 * @return 1 on true, 0 on false
 */
int isPowerOfTwo(size_t n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

/**
 * Next Power of Two function.
 * @brief This function finds the next power of two to any number. (24) -> 32
 * @details Counts the number of possible powers of two inside the input.
 * @param n number.
 * @return the next power of two.
 */
size_t nextPowerOfTwo(size_t n) {
    size_t count = 0;
    if (n == 0) return 1;

    while (n != 0) {
        n >>= 1;
        count += 1;
    }
    return 1 << count;
}

/**
 * Check Input Length function.
 * @brief This function checks the length of the numbers inside a pair, and sets it to be the same and also a power of 2.
 * @details In order to set the length to be the same, a number of nulls is added right of the number so its real value does not change.
 *          The pair will be changed if needed.
 * @param pair hexadecimal pair.
 */
void checkInputLength(HeximalPair *pair){
    if(strlen(pair->first) < strlen(pair->second)){
        char* newFirst = appointNulls(pair->first, strlen(pair->second) - strlen(pair->first));
        if (newFirst) {
            free(pair->first);  // freeing old memory
            pair->first = newFirst;
        }
        
    } else if (strlen(pair->second) < strlen(pair->first)) {
        char* newSecond = appointNulls(pair->second, strlen(pair->first) - strlen(pair->second));
        if (newSecond) {
            free(pair->second);  // freeing old memory
            pair->second = newSecond;
        }
    }

    if(isPowerOfTwo(strlen(pair->first)) != 1){
            size_t nextPower = nextPowerOfTwo(strlen(pair->first));
            char* newFirst = appointNulls(pair->first, nextPower - strlen(pair->first));
            char* newSecond = appointNulls(pair->second, nextPower - strlen(pair->second));
            if (newFirst) {
                free(pair->first);  // freeing old memory
                pair->first = newFirst;
            }
            if (newSecond) {
            free(pair->second);  // freeing old memory
            pair->second = newSecond;
            }
        }

    pair->length = strlen(pair->first);
}

/**
 * Read Input function.
 * @brief This function prompts the user to input numbers and reads them, appointing them to a pair.
 * @param pair hexadecimal pair.
 * @param lineCounter number of read lines.
 * @warning The caller is responsible for freeing pair.
 */
void readInput(HeximalPair *pair, int *lineCounter){
    
    int read;
    char *line = NULL;
    size_t lineLength = 0;

    while((read = getline(&line, &lineLength, stdin)) != -1){
        line[strlen(line) - 1] = '\0';
        if(*lineCounter == 0){
            pair->first = strdup(line);
        } else if(*lineCounter == 1){
            pair->second = strdup(line);
            (*lineCounter)++;
            break;
        }
        (*lineCounter)++;
    }
    free(line);
}

/**
 * Base Case of the recursion.
 * @brief This function takes a pair and prints the multiplication of its two numbers to stdout.
 * @details This is intended for only numbers of a length of 1.
 *          The pair will be freed and program will fflush stdout and exit.
 *          
 * @param pair a valid hexadecimal pair with length = 1.
 */
void runBaseCase(HeximalPair *pair){
    long result = strtoul(pair->first, NULL, 16) * strtoul(pair->second, NULL, 16);

    if(result >= 0x10){
        fprintf(stdout, "%lx\n", result);
    }
    else{
        fprintf(stdout, "0%lx\n", result);
    }
    
    fflush(stdout);

    free(pair->first);
    free(pair->second);

    exit(EXIT_SUCCESS);
}

/**
 * Divide into Two function.
 * @brief This function takes in a string and splits into half to the first and second parameter.
 * @details Memory for first and second will be allocated with length in consideration. 
 *          
 * @param number original non-empty string with length >= 2.
 * @param length length of the number >= 2.
 * @param first A pointer to a char* that will point to the first half of the string after the function returns.
 * @param second A pointer to a char* that will point to the second half of the string after the function returns.
 * @warning The caller is responsible for freeing the memory of first and second.
 */
void divideIntoTwo(char *number, size_t length, char **first, char **second){
    int half = length / 2;

    *first = (char *)malloc(half + 2);
    *second = (char *)malloc(half + 2);

    strncpy(*first, number, half);
    first[0][half] = '\n';
    first[0][half+1] = '\0';

    strcpy(*second, number + half);
    second[0][half] = '\n';
    second[0][half+1] = '\0';
}

/**
 * Write to Pipe function
 * @brief This function opens a file descriptor and writes to two strings.
 * @details Exits on failure to write to file descriptor.
 *          Will be used for stdin of the child processes.
 *          
 * @param fd file descriptor (pipe intended).
 * @param number1 first number (non empty).
 * @param number2 second number (non empty)
 */
void writeToPipe(int fd, char *number1, char *number2){
    FILE *input = fdopen(fd, "w");

    if (fputs(number1, input) == -1){
        usage("./intmul: cannot write to pipe");
    }
        
        
    if (fputs(number2, input) == -1){
        usage("./intmul: cannot write to pipe");
    }
    
    fflush(input);
    fclose(input);
}

/**
 * Fork and open Pipe function
 * @brief This function is the core of the intmul program.
 * @details This function takes in a pair and invokes the child processes, writes and reads to pipes.
 *          Divides the pair->first and pair->second into four parts.
 *          Creates the pipes using pipe() and redirects them using dup2().
 *          Child processes will be executed with execlp as ./intmul.
 *          The parent process writes the corresponding input to every child (one of the four parts)
 *          Fails if fork(), pipe(), dup2() or execlp() fails.
 *          
 * @param pair Hexadecimal pair with length of >= 2.
 */
void forkPipe(HeximalPair *pair){

    char *Ah;
    char *Al;
    char *Bh;
    char *Bl;
    
    divideIntoTwo(pair->first, pair->length, &Ah, &Al);
    divideIntoTwo(pair->second, pair->length, &Bh, &Bl);

    free(pair->first);
    free(pair->second);

    pid_t process_id[4];
    int child_pipe[4][2];

    for(int i=0; i<4; i++){

        pipe(child_pipe[i]);
        pipe(parent_pipe[i]);

        process_id[i] = fork();

        if(process_id[i] == -1) { // erorr on forking
            usage("./intmul: could not fork."); 
        }

        if(process_id[i] == 0) { // child process
            close(child_pipe[i][1]);
            close(parent_pipe[i][0]);

            if(dup2(child_pipe[i][0], STDIN_FILENO) == -1){
                usage("./intmul: could not redirect pipe.");
            }
            if(dup2(parent_pipe[i][1], STDOUT_FILENO) == -1){
                usage("./intmul: could not redirect pipe.");
            }
            
            close(child_pipe[i][0]);
            close(parent_pipe[i][1]);

            free(Ah);
            free(Al);
            free(Bh);
            free(Bl);

            execlp("./intmul", "./intmul", NULL);

            usage("./intmul: could not execute child process.");
        }

        if(process_id[i] > 0) { // parent process

            close(child_pipe[i][0]);
            close(parent_pipe[i][1]);

            int pipe_to_write = child_pipe[i][1];

            switch (i)
            {
            case 0:
                writeToPipe(pipe_to_write, Ah, Bh);
                break;
            
            case 1:
                writeToPipe(pipe_to_write, Ah, Bl);
                break;

            case 2: 
                writeToPipe(pipe_to_write, Al, Bh);
                break;

            case 3:
                writeToPipe(pipe_to_write, Al, Bl);
                break;
            
            default:
                break;
            }
            
        }
    }

    free(Ah);
    free(Al);
    free(Bh);
    free(Bl);
}

/**
 * Append Nulls function
 * @brief This function appends a number of nulls to the right of a string. ('12', 3) -> '12000'
 *          
 * @param number a file pointer to a string
 * @param numberOfNulls number of nulls to be added (>=0).
 */
void appendRightNulls(char **number, int numberOfNulls){

    int newLength = strlen(*number) + numberOfNulls;

    // Reallocate memory for the string, including the additional null characters
    *number = (char *)realloc(*number, newLength + 1);

    if (*number != NULL) {
        for (int i = strlen(*number); i < newLength; i++) {
            (*number)[i] = '0';
        }
        (*number)[newLength] = '\0';
    } else {
        usage("./intmul: realloc failed.");
    }
}

/**
 * Add two Characters as hexadecimals
 * @brief This function adds two characters and returns the result.
 *        Overflow will be written to overflow.
 *          
 * @param a first character.
 * @param b second character.
 * @param overflow overflow of the addition (should initially be "0")
 * @return a+b
 */
char addHexChar(char a, char b, char *overflow){
    
    char strA[2] = {a, '\0'};
    char strB[2] = {b, '\0'};
    char strOverflow[2] = {*overflow, '\0'};

    unsigned long first = strtoul(strA, NULL, 16);
    unsigned long second = strtoul(strB, NULL, 16);
    unsigned long over = strtoul(strOverflow, NULL, 16);

    unsigned long result = first + second + over;

    // Check if there's an overflow.
    if (result > 0xF) {
        *overflow = '1';
        result -= 0x10;
    } else {
        *overflow = '0';
    }

    // Convert result back to hexadecimal character
    char lookup[] = "0123456789abcdef";
    char output = lookup[result];

    return output;
}

/**
 * Add Hexadecimal function
 * @brief This function adds two strings together treating them as hexadecimal numbers.
 *        Result is written in the first parameter.
 *          
 * @param result a file pointer to a string
 * @param number number to be added to result
 */
void addHexadecimal(char *result, char *number){
    
    char *newNumber = appointNulls(number, (strlen(result) - strlen(number)));

    char overflow = '0';
    for(int i = strlen(result) - 1; i >= 0; i--){
        char add = addHexChar(result[i], newNumber[i], &overflow);
        result[i] = add;
    }

    free(newNumber);
}

/**
 * Calculate Result function
 * @brief This function takes in four strings and adds them up, returning the first.
 *        The calculation is done as followed: first*16^n + second*16^(n/2) + third*16^(n/2) + fourth = result
 *        The parameters are intended to be the results of the child processes.
 *          
 * @param first a file pointer to a string
 * @param second a file pointer to a string
 * @param third a file pointer to a string
 * @param fourth a file pointer to a string
 * @param length length of the pair that was initially used
 * @return upon success the result of addition is returned, saved in the first parameter
 */
char* calculateResult(char *first, char *second, char *third, char *fourth, size_t length){
    appendRightNulls(&first, length);
    appendRightNulls(&second, length/2);
    appendRightNulls(&third, length/2);

    addHexadecimal(first, second);
    addHexadecimal(first, third);
    addHexadecimal(first, fourth);

    // free(first);
    free(second);
    free(third);
    free(fourth);

    return first;
}

/**
 * Read from pipes function
 * @brief This function reads from the output pipes and assigns them to the results array.
 *          
 * @param results 4 Element array where the reads will be stored
 */
void read_from_pipes(char **results){
    for (int i = 0; i < 4; i++)
    {
        FILE *out = fdopen(parent_pipe[i][0], "r");
        size_t lencap = 0;
        ssize_t len = 0;
        char *tempLine = NULL;
        if ((len = getline(&tempLine, &lencap, out)) == -1){
            usage("./intmul: failed to read from pipes.");
        }
        
        results[i] = tempLine;
        results[i][len - 1] = '\0';
        close(parent_pipe[i][0]);
        fclose(out);
    }
}

/**
 * main function
 * @brief This is the main function of the program, the starting point upon starting.
 *        It takes no argument upon calling.
 *        Calls on other functions for executing the functionality of intmul.
 *        Input will be read, checked and assigned.
 *        If the input is the base case format, then the program will handle it and exit.
 *        Otherwise, forkPipe will be called and child processes will be created.
 *        Parent process will wait in the main for its children to finish and then read from them, assign the results to results array.
 *        Finally calucation will be performed and the results will be written to stdout.
 *          
 * @param argc number of arguments
 * @param argv parsed arguments
 */
int main(int argc, char* argv[]){
    if(argc > 1){
        usage("./intmul: too many arguments were specified.");
    }

    HeximalPair pair;
    int lineCounter = 0;

    readInput(&pair, &lineCounter);

    checkInput(pair, lineCounter);
    checkInputLength(&pair);
    
    if(pair.length == 1){
        runBaseCase(&pair);
    }
    forkPipe(&pair);

    waitOnChildren();
    
    char *results[4];

    read_from_pipes(results);
    
    char *result = calculateResult(results[0], results[1], results[2], results[3], pair.length);
    
    fprintf(stdout, "%s\n", result);
    
    free(result);

    fflush(stdout);
    close(parent_pipe[0][0]);
    close(parent_pipe[1][0]);
    close(parent_pipe[2][0]);
    close(parent_pipe[3][0]);

    exit(EXIT_SUCCESS);
}