/**
 * @file supervisor.c
 * @author Arslan Smajevic <e12127678@student.tuwien.ac.at>
 * @date 22.10.2023
 *
 * @brief A supervisor program that reads from the shared memory and prints the best solution found.
 * 
 * @details supervisor [-n limit] [-w delay] [-p]
 *          If limit is specified, the generators will produce only limit amount of solutions.
 *          If delay is specified, the supervisor will start the shared memory and semaphores and wait delay time, in order to let generators create solutions.
 *          If p is specified, a graph will be drawn.
 *          The supervisor program opens and maps the memory to be used as a circular buffer.
 *          Further on, it will open semaphores that ensure non-blocking.
 *          It will read created solutions, and print the best solutions to the stderr.
 *          If the graph is not 3-coloring compatible, it will work for ever, or until the limit is reached, or until CTRL+C or CTRL+D is given.
 *          
 * **/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <limits.h>
#include <signal.h>
#include <string.h>

#define SEM_1 "/sem_1_12127678" // freespace
#define SEM_2 "/sem_2_12127678" // usedspace
#define SEM_3 "/sem_3_12127678" // writing privileges

#define MAX_DATA (100)
#define SHM_NAME "/shared_12127678"
#define NUMBER_OF_SOLUTIONS (20)

static char *progname;
static volatile sig_atomic_t quit = 0;

/**
 * @struct edge
 * @brief This structure represents an edge in a graph.
 */
typedef struct{
    int first;
    int second;
} edge;

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
* @brief Processes the arguments given to the program.
* @details This function checks for the given input according to this synopsis: ./supervisor [-n limit] [-w delay] [-p]
*          Function will exit if the synopsis is not satisfied. 
*
* @param argc number of arguments
* @param argv program arguments
* @param limit given limit?
* @param delay given delay?
* @param p given p?
*/
static void handleArguments(int argc, char* argv[], int *limit, int *delay, int *p){
    int option;

    while ((option = getopt(argc, argv, "n:w:p")) != -1) {
        switch (option) {
            case 'n':
                if(*limit == -1){
                    *limit = strtoul(optarg, NULL, 10);
                }
                else{
                    usage("-n was declared too many times.");
                }
                break;

            case 'w':
                if(*delay == -1){
                    *delay = strtoul(optarg, NULL, 10);
                }
                else{
                    usage("-w was declared too many times.");
                }
                break;

            case 'p':
                if(*p == -1){
                    *p = 1;
                }
                else
                {
                    usage("-p was declared too many times.");
                }
                break;

            case '?':
                exit(EXIT_FAILURE);

            default:
               break;
        }
    }
}

/**
* Handle Signal function.
* @brief This function sets quit to 1 if a signal of CTRL+C is detected.
*
* @param signal signal
*/
static void handle_signal(int signal)
{
	quit = 1;
}

/**
 * main function
 * @brief This is the entry point of the ./supervisor program
 *        It creates the necessary resources, like shared-memory, semaphores, signal declaration.
 *        It then waits for solutions to be created and reads from them, handling them in a way of finding the best solution or reaching a limit.
 *          
 * @param argc number of arguments
 * @param argv parsed arguments
 */
int main(int argc, char* argv[])
{
    progname = argv[0];
    int limit = -1;
    int delay = -1;
    int p = -1;
    
    #ifdef DEBUG
    pid_t pid = getpid();
    fprintf(stdout, "Process id: [%d]\n", pid);
    #endif

    struct sigaction sa; // assinging signals
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        usage("failed on sigaction.");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        usage("failed on sigaction.");
    }

    handleArguments(argc, argv, &limit, &delay, &p); // handling given arguments

    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600); // opening shared memory
    if(shmfd == -1){
        usage("error occured while opening shared memory.");
    }

    if(ftruncate(shmfd, sizeof(sharedMemory)) == -1){ // truncating shared memory
        close(shmfd);
        usage("error occured while truncating memory.");
    }

    sharedMemory *myMemory;
    myMemory = mmap(NULL, sizeof(*myMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0); // mapping the shared memory
    if(myMemory == MAP_FAILED){
        close(shmfd);
        usage("error occured while mapping memory.");
    }

    #ifdef DEBUG
    fprintf(stderr, "Memory size: %zu\n", sizeof(*myMemory));
    #endif
    
    myMemory->state = 0;
    myMemory->currentReading = 0;
    myMemory->currentWriting = 0;
    myMemory->numberOfGenerators = 0;

    sem_t *freespace = sem_open(SEM_1, O_CREAT | O_EXCL, 0600, MAX_DATA);
    if (freespace == SEM_FAILED) {
        munmap(myMemory, sizeof(*myMemory));
        close(shmfd);
        sem_unlink(SEM_1);
        usage("error on opening the freespace semaphore.");
    }

    sem_t *usedspace = sem_open(SEM_2, O_CREAT | O_EXCL, 0600, 0);
    if (usedspace == SEM_FAILED) {
        munmap(myMemory, sizeof(*myMemory));
        close(shmfd);
        sem_unlink(SEM_1);
        sem_unlink(SEM_2);
        usage("error on opening the usedspace semaphore.");
    }

    sem_t *writeacces = sem_open(SEM_3, O_CREAT | O_EXCL, 0600, 1);
    if (usedspace == SEM_FAILED) {
        munmap(myMemory, sizeof(*myMemory));
        close(shmfd);
        sem_unlink(SEM_1);
        sem_unlink(SEM_2);
        sem_unlink(SEM_3);
        usage("error on opening the writeacces semaphore.");
    }

    int best_size = NUMBER_OF_SOLUTIONS + 1;
    int found = 0;
    myMemory->bestSolution = NUMBER_OF_SOLUTIONS + 1;

    myMemory->limit = limit;

    int freespaceSem = 0;

    if(delay != -1){
        sleep(delay);
    }

    while(!quit){

        if(sem_wait(usedspace) == -1){
            quit = 1;
        }

        if(quit == 1){
            if(!found){
                if(best_size != (NUMBER_OF_SOLUTIONS+1)){
                    fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges.\n", best_size);
                }
                else{
                    fprintf(stdout, "No solutions were registered.\n");
                }
                myMemory->state = 1;
            }
            break;
        }

        if(myMemory->edgeArray[myMemory->currentReading].size < best_size){
            best_size = myMemory->edgeArray[myMemory->currentReading].size;
            myMemory->bestSolution = best_size;
            if(best_size == 0){
                found = 1;
                fprintf(stdout, "The graph is 3-colorable!\n");
                myMemory->state = 1;
                if(sem_post(freespace) == -1){
                    freespaceSem = 282;
                }
                break;
            }
            else{
                fprintf(stderr, "Solution with %d edges: ", best_size);

                for(int i=0; i<best_size; i++){
                    fprintf(stderr, "%d-%d ", myMemory->edgeArray[myMemory->currentReading].edges[i].first, myMemory->edgeArray[myMemory->currentReading].edges[i].second);
                }
                fprintf(stderr, "\n");
            }
        }
        myMemory->currentReading = (myMemory->currentReading + 1) % MAX_DATA;

        if(myMemory->limit == 0 && ((((myMemory->currentReading)) % MAX_DATA) == myMemory->currentWriting)){
            if(!found){
                
                fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges.\n", best_size);
            }
            myMemory->state = 1;
            if(sem_post(freespace) == -1){
                freespaceSem = 304;    
            }
            break;
        }

        if(sem_post(freespace) == -1){
            freespaceSem = 310;
            myMemory->state = 1;
            break;
        }
    }

    // while(myMemory->numberOfGenerators != 0){
    //     if(sem_post(freespace) == -1){
    //         freespaceSem = 318;
    //         break;
    //     }
    // }
    int numGenerator = myMemory->numberOfGenerators;
    for(int i = 0; i < numGenerator; i++){
        sem_post(freespace);
        sem_post(writeacces);
    }

    // munmap(myMemory, sizeof(*myMemory));
    // close(shmfd);
    // shm_unlink(SHM_NAME);

    // sem_unlink(SEM_1);
    // sem_unlink(SEM_2);
    // sem_unlink(SEM_3);
    // sem_close(freespace);
    // sem_close(usedspace);
    // sem_close(writeacces);

    if(freespaceSem != 0){
        fprintf(stderr, "error: %d\n", freespaceSem);
        usage("freespace semaphore caused an error");
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

    if (sem_unlink(SEM_1) == -1) {
        usage("Error in sem_unlink for SEM_1");
    }

    if (sem_unlink(SEM_2) == -1) {
        usage("Error in sem_unlink for SEM_2");
    }

    if (sem_unlink(SEM_3) == -1) {
        usage("Error in sem_unlink for SEM_3");
    }

    if (close(shmfd) == -1) {
        usage("Error in close");
    }

    if (munmap(myMemory, sizeof(*myMemory)) == -1) {
        usage("Error in munmap");
    }

    if (shm_unlink(SHM_NAME) == -1) {
        usage("Error in shm_unlink");
    }

    return 0;

}