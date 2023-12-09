/**
 * @file generator.c
 * @author Arslan Smajevic <e12127678@student.tuwien.ac.at>
 * @date 22.10.2023
 *
 * @brief A generator program that creates 3-coloring solutions for a given graph.
 * 
 * @details This program will receive a graph in its synopsis, ./generator edge... whereas edge is denoted as 0-1 or 12-1.
 *          Further on, it will color this graph with a random permutation, check for color confilicts, and remove the edges that produce the conflict.
 *          This generator program is completely dependent on the supervisor program - meaning, it cannot be started until the supervisor has started.
 *          More generator instances can be executed at once, but with the same graph! Otherwise, it does not really make sense.
 *          It uses shared memory from the supervisor to write its solutions, and semaphores to ensure non-blocking and raceconditions.
 * **/

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

// region: SEMAPHORES
#define SEM_1 "/sem_1_12127678" // freespace
#define SEM_2 "/sem_2_12127678" // usedspace
#define SEM_3 "/sem_3_12127678" // writing privileges

// region: SHARED_MEMORY
#define MAX_DATA (100) // maximal buffer
#define SHM_NAME "/shared_12127678" // shared memory name
#define NUMBER_OF_SOLUTIONS (20) // maximal size of one solution

// program name
static char *progname;

/**
 * @struct edge
 * @brief This structure represents an edge in a graph.
 */
typedef struct{
    int first;
    int second;
} edge;

/**
 * @struct edgeArray
 * @brief This structure represents an array of edges in a graph.
 */
typedef struct{
    edge* edges;
    size_t size;
} edgeArray;

/**
 * @struct nodesArray
 * @brief This structure represents a color permutation for all nodes.
 */
typedef struct{
    int* nodes;
    size_t size;
} nodesArray;

/**
 * @struct solutionEdgeArr
 * @brief This structure represents an array of edges to be written to the shared memory. edges array is limited.
 */
typedef struct{
    edge edges[NUMBER_OF_SOLUTIONS];
    size_t size;
} solutionEdgeArr;

/**
 * @struct sharedMemory
 * @brief This structure represents the shared memory object. 
 * @details state is 1 initially. 0 will be set, when the limit is reached or the supervisor found a solution.
 *          currentReading is the current reading element by the supervisor.
 *          currentWriting is the current writing element by the generator.
 *          numberOfGenerators is the number of currently running generators.
 *          limit is the limit set by the supervisor, maximal number of solutions to be written.
 *          edgeArray is the solutions array.
 */
typedef struct{
    int state;
    int currentReading;
    int currentWriting;
    int numberOfGenerators;
    int bestSolution;
    int limit;
    solutionEdgeArr edgeArray[MAX_DATA];
} sharedMemory;

/**
* Usage function.
* @brief Prints a message to stderr and stops the program with EXIT_FAILURE.
* @details This function is used for usage messages - meaning, what the user or the program has invoked is not allowed. 
*          Message contains the detailed error.
*
* @param error string
*/
static void usage(char *error){
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
static edgeArray handleArguments(int argc, char **argv){

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
static int isNumberInArray(int num, int *array, int size) {
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
static nodesArray separateNodes(edgeArray arr){

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

    if(count != 0){
      result.nodes = (int *) realloc(result.nodes, count * sizeof(int));
    }

    // result.nodes = (int *) realloc(result.nodes, count * sizeof(int));
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
static void colorPermutation(int *colorArr, int nodesSize){
    for(int i = 0; i < nodesSize; i++){
        colorArr[i] = rand() % 3;
    }
}

/**
* reduceEdges function.
* @brief This function takes in the edges, the nodes and the color permutation;
*        returning a new edgeArray with edges that caused conflicts on same color.
*
* @param edgesArr edges array
* @param nodesArr nodes array
* @param colorArr color permutation array
* @return a new edgeArray containing the edges to be deleted for 3-coloring (do not forget cleanup!)
*/
static edgeArray reduceEdges(edgeArray edgesArr, nodesArray nodesArr, int *colorArr){
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

        if(colorArr[firstColor] == colorArr[secondColor]){
            result.edges[count] = edgesArr.edges[i];
            count++;
        }
    }

    result.size = count;
    if(count != 0 && count != edgesArr.size){
        result.edges = (edge *) realloc(result.edges, count * sizeof(edge));
    }
    // result.edges = (edge *) realloc(result.edges, count * sizeof(edge));

    return result;
}

/**
* createSolution function.
* @brief This function takes in the edges and the nodes. It creates a color permutation array 
*        and calls on reduceEdges. 
*
* @param edgesArr edges array
* @param nodesArr nodes array
* @return a new edgeArray containing the edges to be deleted for 3-coloring (do not forget cleanup!)
*/
static edgeArray createSolution(edgeArray edgesArr, nodesArray nodesArr){
    
    int colorArr[nodesArr.size];    
    colorPermutation(colorArr, nodesArr.size);

    edgeArray reduced = reduceEdges(edgesArr, nodesArr, colorArr);

    return reduced;

}

/**
* writeToSharedMemory function.
* @brief This function writes a solution to the shared memory. 
* @details This function should be exclusive.
* @param myMemory shared memory
* @param solution solution with the edges
*/
static void writeToSharedMemory(sharedMemory *myMemory, edgeArray solution){
    
    #ifdef DEBUG
        pid_t pid = getpid();
    #endif

    myMemory->edgeArray[myMemory->currentWriting].size = solution.size;
    #ifdef DEBUG
        fprintf(stdout, "[%d] Writing to [%d] in shared memory a solution of size [%ld]: ", pid, myMemory->currentWriting, solution.size);
    #endif
    for(int i = 0; i < solution.size; i++){
        #ifdef DEBUG
            fprintf(stdout, "%d-%d ", solution.edges[i].first, solution.edges[i].second);
        #endif
        myMemory->edgeArray[myMemory->currentWriting].edges[i].first = solution.edges[i].first;
        myMemory->edgeArray[myMemory->currentWriting].edges[i].second = solution.edges[i].second;
    }
    #ifdef DEBUG
        fprintf(stdout, "; with current limit: %d\n", myMemory->limit);
    #endif
        
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
    
    if(ftruncate(shmfd, sizeof(sharedMemory)) == -1){
        usage("error occured while truncating memory.");
    }
    sharedMemory *myMemory;
    myMemory = mmap(NULL, sizeof(*myMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0); // erorr
    if(myMemory == MAP_FAILED){
        close(shmfd);
        usage("error occured while mapping memory.");
    }

    sem_t *freespace = sem_open(SEM_1, 0);
    if (freespace == SEM_FAILED) {
        if (munmap(myMemory, sizeof(*myMemory)) == -1) {
            usage("Error in munmap");
        }
        if (close(shmfd) == -1) {
            usage("Error in close");
        }
        if (sem_close(freespace) == -1) {
            usage("Error in sem_close for freespace");
        }
        usage("error on opening the freespace semaphore.");
    }
    sem_t *usedspace = sem_open(SEM_2, 0);
    if (usedspace == SEM_FAILED) {
        if (munmap(myMemory, sizeof(*myMemory)) == -1) {
            usage("Error in munmap");
        }
        if (close(shmfd) == -1) {
            usage("Error in close");
        }
        if (sem_close(freespace) == -1) {
            usage("Error in sem_close for freespace");
        }
        if (sem_close(usedspace) == -1) {
            usage("Error in sem_close for usedspace");
        }
        usage("error on opening the usedspace semaphore.");
    }
    sem_t *writeacces = sem_open(SEM_3, 0);
    if (usedspace == SEM_FAILED) {
        if (munmap(myMemory, sizeof(*myMemory)) == -1) {
            usage("Error in munmap");
        }
        if (close(shmfd) == -1) {
            usage("Error in close");
        }
        if (sem_close(freespace) == -1) {
            usage("Error in sem_close for freespace");
        }
        if (sem_close(usedspace) == -1) {
            usage("Error in sem_close for usedspace");
        }

        if (sem_close(writeacces) == -1) {
            usage("Error in sem_close for writeacces");
        }
        usage("error on opening the writeacces semaphore.");
    }
    
    while(myMemory->state != 1){
        edgeArray solution = createSolution(edges, nodes);
        
        // while(solution.size > myMemory->bestSolution){ // essentially a good approach, but timeout related
        //     free(solution.edges);
        //     solution = createSolution(edges, nodes);
        // }

        while(solution.size > NUMBER_OF_SOLUTIONS){
            free(solution.edges);
            solution = createSolution(edges, nodes);
        }
        
        if(sem_wait(writeacces) == -1){
            free(solution.edges);
            break;
        }
        if(myMemory->state == 1){
            free(solution.edges);    
            sem_post(writeacces);
            break;
        }

        if(addedAsGenerator == 0){
            addedAsGenerator = 1;
            myMemory->numberOfGenerators = myMemory->numberOfGenerators + 1;
        }
        if(sem_wait(freespace) == -1){
            free(solution.edges);
            break;
        }
        if(myMemory->limit != 0){
            myMemory->limit = myMemory->limit - 1;
            writeToSharedMemory(myMemory, solution);
	    
        }
        else{
            free(solution.edges);
        }
        
        if(sem_post(usedspace) == -1){
            break;
        }
        if(sem_post(writeacces) == -1){
            break;
        }

        if(myMemory->limit == 0){
            break;
        }
    }
    
    free(edges.edges);
    free(nodes.nodes);

    // sem_wait(writeacces);
    // myMemory->numberOfGenerators = myMemory->numberOfGenerators - 1;
    // sem_post(writeacces);

    if (munmap(myMemory, sizeof(*myMemory)) == -1) {
        usage("Error in munmap");
    }

    if (close(shmfd) == -1) {
        usage("Error in close");
    }
    
    if (sem_close(freespace) == -1) {
        usage("Error in sem_close for freespace");
    }

    if (sem_close(usedspace) == -1) {
        usage("Error in sem_close for usedspace");
    }

    if (sem_close(writeacces) == -1) {
        usage("Error in sem_close for writeacces");
    }

    return 0;

}
