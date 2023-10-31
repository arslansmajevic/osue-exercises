/**
 * @file intmul.c
 * @author Arslan Smajevic <e12127678@student.tuwien.ac.at>
 * @date 14.10.2023
 *
 * @brief A program that takes two hexadecimal numbers and multiplies them.
 * 
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct{
    char *first;
    char *second;
    size_t length;
} HeximalPair;

int parent_pipe[4][2];

void wait_handler(int err)
{
    int status,
        pid, error = err;
    while ((pid = wait(&status)) != -1)
    {
        if (WEXITSTATUS(status) != EXIT_SUCCESS)
            error = 1;
    }

    if (error)
        exit(EXIT_FAILURE);

}

void usage(char *error){
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
}

void checkInput(HeximalPair pair, int lineCounter){
    if(lineCounter == 1){
        free(pair.first);
        usage("./intmul: one inputs given.");
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

char* appointNulls(char *number, int numberOfNulls){

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

int isPowerOfTwo(size_t n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

size_t nextPowerOfTwo(size_t n) {
    size_t count = 0;

    // Zero is not a power of two
    if (n == 0) return 1;

    while (n != 0) {
        n >>= 1;
        count += 1;
    }

    // Return 2 raised to the count, which gives the next power of two
    return 1 << count;
}

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
        // free(newSecond);
    }

    if(isPowerOfTwo(strlen(pair->first)) != 1){
            size_t nextPower = nextPowerOfTwo(strlen(pair->first));
            // fprintf(stdout, "next power of two is: %ld\n", nextPower);
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

void writeToPipe(int fd, char *number1, char *number2){
    FILE *input = fdopen(fd, "w");

    if (fputs(number1, input) == -1)
        usage("cannot write to pipe");

        
    if (fputs(number2, input) == -1)
        usage("cannot write to pipe");
    
    fflush(input);
    fclose(input);
}

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

            dup2(child_pipe[i][0], STDIN_FILENO);
            dup2(parent_pipe[i][1], STDOUT_FILENO);
            
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
                // writeToPipe(pipe_to_write, Ah, 1);
                // writeToPipe(pipe_to_write, Bh, 2);
                writeToPipe(pipe_to_write, Ah, Bh);
                break;
            
            case 1:
                // writeToPipe(pipe_to_write, Ah, 1 );
                // writeToPipe(pipe_to_write, Bl, 2);
                writeToPipe(pipe_to_write, Ah, Bl);
                break;

            case 2: 
                // writeToPipe(pipe_to_write, Al, 1);
                // writeToPipe(pipe_to_write, Bh, 2);
                writeToPipe(pipe_to_write, Al, Bh);
                break;

            case 3:
                // writeToPipe(pipe_to_write, Al, 1);
                // writeToPipe(pipe_to_write, Bl, 2);
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

void appointRightNulls(char **number, int numberOfNulls){
    // // fprintf(stdout, "Length: %ld\n", strlen(number));
    // char *newNumber = (char*)malloc(strlen(*number) + numberOfNulls + 1);
    // // fprintf(stdout, "old number %s\n", number);
    // strcpy(newNumber, *number);

    // for(int i = strlen(*number); i < strlen(*number) + numberOfNulls; i++){
    //     newNumber[i] = '0';
    // }
    // newNumber[strlen(*number) + numberOfNulls] = '\0';

    // // fprintf(stdout, "new number %s\n", newNumber);
    // // free(*number);
    // *number = newNumber;

    // Calculate the new length of the string
    int newLength = strlen(*number) + numberOfNulls;

    // Reallocate memory for the string, including the additional null characters
    *number = (char *)realloc(*number, newLength + 1);

    if (*number != NULL) {
        // Fill the newly allocated space with '0' characters
        for (int i = strlen(*number); i < newLength; i++) {
            (*number)[i] = '0';
        }
        (*number)[newLength] = '\0'; // Don't forget to null-terminate the string
    } else {
        // Handle memory allocation failure
        fprintf(stderr, "Memory allocation failed\n");
        // You can choose how to handle this error in your code.
    }
}

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

void addHexadecimal(char *result, char *number){
    
    char *newNumber = appointNulls(number, (strlen(result) - strlen(number)));

    char overflow = '0';
    for(int i = strlen(result) - 1; i >= 0; i--){
        char add = addHexChar(result[i], newNumber[i], &overflow);
        result[i] = add;
    }

    free(newNumber);
}



char* calculateResult(char *first, char *second, char *third, char *fourth, size_t length){
    appointRightNulls(&first, length);
    appointRightNulls(&second, length/2);
    appointRightNulls(&third, length/2);

    addHexadecimal(first, second);
    addHexadecimal(first, third);
    addHexadecimal(first, fourth);

    // free(first);
    free(second);
    free(third);
    free(fourth);

    return first;
}

void read_from_pipes(char **results)
{
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

    wait_handler(0);
    
    char *results[4];

    read_from_pipes(results);
    
    char *result = calculateResult(results[0], results[1], results[2], results[3], pair.length);
    
    fprintf(stdout, "%s\n", result);
    
    // free(results[0]);
    free(result);

    fflush(stdout);
    close(parent_pipe[0][0]);
    close(parent_pipe[1][0]);
    close(parent_pipe[2][0]);
    close(parent_pipe[3][0]);


    exit(EXIT_SUCCESS);
}