#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/types.h>

#define SEM_1 "/sem_1_12127678"
#define SEM_2 "/sem_2_12127678"
#define SEM_3 "/sem_3_12127678" // writing privileges
#define MAX_DATA (100)
#define SHM_NAME "/shared_12127678"
#define NUMBER_OF_SOLUTIONS (20)

char *progname;

typedef struct{
    int first;
    int second;
} edge;

typedef struct{
    edge* edges;
    size_t size;
} edgeArray;

typedef struct{
    int* nodes;
    size_t size;
} nodesArray;

typedef struct{
    edge edges[NUMBER_OF_SOLUTIONS];
    size_t size;
} solutionEdgeArr;

typedef struct{
    int state;
    int currentReading;
    int currentWriting;
    int numberOfGenerators;
    int bestSolution;
    int limit;
    solutionEdgeArr edgeArray[MAX_DATA];
} sharedMemory;

// edgeArray edges;
// nodesArray nodes;

/**
* Usage function.
* @brief Prints a message to stderr and stops the program with EXIT_FAILURE.
* @details This function is used for usage messages - meaning, what the user or the program has invoked is not allowed. 
*          Message contains the detailed error.
*
* @param error string
*/
void usage(char *error){
    fprintf(stderr, "%s: %s\n", progname, error);
    exit(EXIT_FAILURE);
}

/**
* Handling Arguments Function.
* @brief Processes the arguments given to the program and gives back an object of edgeArray.
* @details The program will exit and memory will be cleaned up, if the arguments do not follow the pattern of: "a-b" where a and b are integers.
*          On succesful finish, .edges need to be freed, before exiting!
* 
* @param argc number of arguments
* @param argv program arguments
* @return edgeArray struct with edges and size of the array
*/
edgeArray handleArguments(int argc, char **argv){

    edgeArray result;
    result.size = argc - 1;
    result.edges = (edge*) malloc(result.size * sizeof(edge));

    if (result.edges == NULL) {
        usage("memory allocation on edges failed");
    }

    for(int i=1; i<argc; i++){
        
        edge e;

        char *arg = argv[i];
        char *part = strtok(arg, "-");
        char *endptr;

        if(part == NULL){
            free(result.edges);
            usage("invalid input");
        }
        
        e.first = (int)strtol(part, &endptr, 10);

        if (*endptr != '\0') {
            free(result.edges);
            usage("invalid input, numbers only");
        }

        part = strtok(NULL, "-");    
        if(part == NULL){
            free(result.edges);
            usage("invalid input");
        }

        e.second = (int)strtol(part, &endptr, 10);

        if (*endptr != '\0') {
            free(result.edges);
            usage("invalid input, numbers only");
        }

        result.edges[i-1] = e;
        // fprintf(stdout, "got edge: %d-%d\n", e.first, e.second);
    }    
    
    return result;
}

/**
* Is Number in Array.
* @brief This function checks if a number present in the parsed array.
*
* @param num number to be checked
* @param array array
* @param size array size
* @return 1 on present, 0 on not present
*/
int isNumberInArray(int num, int *array, int size) {
    for (int i = 0; i < size; ++i) {
        if (array[i] == num) {
            return 1;
        }
    }
    return 0;
}

/**
* Separate Nodes function.
* @brief This function takes in an edgeArray and separates the nodes, returning them in nodesArray struct.
* @details The cause of this function, is that edges can contain the same nodes, meaning there can be for example: "0-1" "0-2", where we have 2 edges, but 3 nodes.
*
* @param arr edgeArray containing all the edges
* @return nodesArray on success, with the unique nodes; on failure memory will be freed and the program will terminate.
*/
nodesArray separateNodes(edgeArray arr){

    nodesArray result;

    result.nodes = (int *) malloc(arr.size * 2 * sizeof(int));

    if (result.nodes == NULL) {
        free(arr.edges);
        usage("memory allocation failed on creating nodes array");
    }

    for(int i=0; i<arr.size * 2; i++){
        result.nodes[i] = -1;
    }
    
    int count = 0;

    for (int i = 0; i < arr.size; i++) {
        if (isNumberInArray(arr.edges[i].first, result.nodes, count) == 0) {
            result.nodes[count++] = arr.edges[i].first;
        }
        if (isNumberInArray(arr.edges[i].second, result.nodes, count) == 0) {
            result.nodes[count++] = arr.edges[i].second;
        }
    }

    result.nodes = (int *) realloc(result.nodes, count * sizeof(int));
    result.size = count;

    return result;
}

/**
* ColorPermutation function.
* @brief This function takes in an array and assings a random color to it.
* @details 0 - Red, 1 - Blue, 2 - Green.
*
* @param colorArr array for the colors
* @param nodesSize size of the array
*/
void colorPermutation(int *colorArr, int nodesSize){
    for(int i = 0; i < nodesSize; i++){
        colorArr[i] = rand() % 3;
    }
}

edgeArray reduceEdges(edgeArray edgesArr, nodesArray nodesArr, int *colorArr){
    edgeArray result;

    result.edges = (edge*) malloc(edgesArr.size * sizeof(edge));
    int count = 0;

    for(int i=0; i<edgesArr.size; i++){
        int firstColor = -1;
        int secondColor = -1;

        for(int j=0; j<nodesArr.size; j++){
            if(edgesArr.edges[i].first == nodesArr.nodes[j]){
                firstColor = j;
            }
            if(edgesArr.edges[i].second == nodesArr.nodes[j]){
                secondColor = j;
            }
        }

        // if(colorArr[firstColor] != colorArr[secondColor]){
        //     result.edges[count] = edgesArr.edges[i];
        //     count++;
        // }
        // else{
        //     fprintf(stdout, "conflict on edge: %d-%d\n", edgesArr.edges[i].first, edgesArr.edges[i].second);
        //     fprintf(stdout, "conflict: [%d: %d] [%d: %d]\n", nodesArr.nodes[firstColor], colorArr[firstColor], nodesArr.nodes[secondColor], colorArr[secondColor]);
        // }

        if(colorArr[firstColor] == colorArr[secondColor]){
            result.edges[count] = edgesArr.edges[i];
            count++;
            // fprintf(stdout, "conflict on edge: %d-%d\n", edgesArr.edges[i].first, edgesArr.edges[i].second);
            // fprintf(stdout, "conflict: [%d: %d] [%d: %d]\n", nodesArr.nodes[firstColor], colorArr[firstColor], nodesArr.nodes[secondColor], colorArr[secondColor]);
        }
    }

    result.size = count;
    result.edges = (edge *) realloc(result.edges, count * sizeof(edge));

    return result;
}

edgeArray createSolution(edgeArray edgesArr, nodesArray nodesArr){
    
    int colorArr[nodesArr.size];    
    colorPermutation(colorArr, nodesArr.size);

    for(int i=0; i<nodesArr.size; i++){
        // fprintf(stdout, "[%d: %d] ", nodesArr.nodes[i], colorArr[i]);
    }
    // fprintf(stdout, "\n");

    edgeArray reduced = reduceEdges(edgesArr, nodesArr, colorArr);

    // fprintf(stdout, "\na reduction to %ld: \n", reduced.size);
    for(size_t i = 0; i < reduced.size; i++) {
        // fprintf(stdout, "%d-%d ", reduced.edges[i].first, reduced.edges[i].second);
    }

    // fprintf(stdout, "\n");

    return reduced;

}

void writeToSharedMemory(sharedMemory *myMemory, edgeArray solution){
    
    pid_t pid = getpid();

        myMemory->edgeArray[myMemory->currentWriting].size = solution.size;
        fprintf(stdout, "[%d] Writing to [%d] in shared memory: ", pid, myMemory->currentWriting);
        for(int i = 0; i < solution.size; i++){
            fprintf(stdout, "%d-%d ", solution.edges[i].first, solution.edges[i].second);
            myMemory->edgeArray[myMemory->currentWriting].edges[i].first = solution.edges[i].first;
            myMemory->edgeArray[myMemory->currentWriting].edges[i].second = solution.edges[i].second;
        }
        fprintf(stdout, "\n");
        
        myMemory->currentWriting = (myMemory->currentWriting + 1) % MAX_DATA;

        free(solution.edges);
}

int main(int argc, char* argv[])
{
    int addedAsGenerator = 0;
    progname = argv[0];
    srand(time(NULL)); // seeding the rand function

    if(argc == 1){
        usage("no arguments given.");
    }
    edgeArray edges = handleArguments(argc, argv);
    nodesArray nodes = separateNodes(edges);

    int shmfd;
    if((shmfd = shm_open(SHM_NAME, O_RDWR, 0600))  == -1){
        usage("supervisor has not been started.");
    }
    
    ftruncate(shmfd, sizeof(sharedMemory)); // erorr
    sharedMemory *myMemory;
    myMemory = mmap(NULL, sizeof(*myMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0); // erorr

    sem_t *freespace = sem_open(SEM_1, 0);
    sem_t *usedspace = sem_open(SEM_2, 0);
    sem_t *writeacces = sem_open(SEM_3, 0);

    while(myMemory->state != 1){
        edgeArray solution = createSolution(edges, nodes);
        while(solution.size > myMemory->bestSolution){
            free(solution.edges);
            solution = createSolution(edges, nodes);
        }
        
        sem_wait(writeacces);
        if(myMemory->state == 1){
            sem_post(writeacces);
            break;
        }

        if(addedAsGenerator == 0){
            addedAsGenerator = 1;
            myMemory->numberOfGenerators = myMemory->numberOfGenerators + 1;
        }
        sem_wait(freespace);
        sleep(1);
        if(myMemory->limit != -1){
            myMemory->limit = myMemory->limit - 1;
        }
        
        writeToSharedMemory(myMemory, solution);
        
        sem_post(usedspace);
        sem_post(writeacces);

        if(myMemory->limit == 0){
            break;
        }
    }

    munmap(myMemory, sizeof(*myMemory));
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_1);
    sem_unlink(SEM_2);
    sem_unlink(SEM_3);
    
    free(edges.edges);
    free(nodes.nodes);

    return 0;

}