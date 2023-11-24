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

char *progname;
volatile sig_atomic_t quit = 0;

typedef struct{
    int first;
    int second;
} edge;

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

void usage(char *error){
    fprintf(stderr, "%s: %s\n", progname, error);
    exit(EXIT_FAILURE);
}

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

static void handle_signal(int signal)
{
	quit = 1;
}

int main(int argc, char* argv[])
{
    progname = argv[0];
    int limit = -1;
    int delay = -1;
    int p = -1;
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

    handleArguments(argc, argv, &limit, &delay, &p);

    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if(shmfd == -1){
        usage("error occured while opening shared memory.");
    }
    if(ftruncate(shmfd, sizeof(sharedMemory)) == -1){
        close(shmfd);
        usage("error occured while truncating memory.");
    }

    sharedMemory *myMemory;
    myMemory = mmap(NULL, sizeof(*myMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if(myMemory == MAP_FAILED){
        close(shmfd);
        usage("error occured while mapping memory.");
    }

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
        sem_unlink(SEM_2);
        usage("error on opening the usedspace semaphore.");
    }

    sem_t *writeacces = sem_open(SEM_3, O_CREAT | O_EXCL, 0600, 1);
    if (usedspace == SEM_FAILED) {
        munmap(myMemory, sizeof(*myMemory));
        close(shmfd);
        sem_unlink(SEM_3);
        usage("error on opening the writeacces semaphore.");
    }

    int best_size = NUMBER_OF_SOLUTIONS + 1;
    int found = 0;
    myMemory->bestSolution = NUMBER_OF_SOLUTIONS + 1;

    myMemory->limit = limit;

    if(delay != -1){
        sleep(delay);
    }

    while(!quit){
        sem_wait(usedspace);

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
                fprintf(stdout, "Read from [%d], The graph is 3-colorable!\n", myMemory->currentReading);
                myMemory->state = 1;
                sem_post(freespace);
                break;
            }
            else{
                fprintf(stdout, "Read from [%d], Solution with %d edges: ", myMemory->currentReading, best_size);

                for(int i=0; i<best_size; i++){
                    fprintf(stdout, "%d-%d ", myMemory->edgeArray[myMemory->currentReading].edges[i].first, myMemory->edgeArray[myMemory->currentReading].edges[i].second);
                }
                fprintf(stdout, "\n");
            }
        }
        myMemory->currentReading = (myMemory->currentReading + 1) % MAX_DATA;

        if(myMemory->limit == 0 && ((((myMemory->currentReading)) % MAX_DATA) == myMemory->currentWriting)){
            if(!found){
                fprintf(stdout, "The graph might not be 3-colorable, best solution removes %d edges.\n", best_size);
            }
            sem_post(freespace);
            break;
        }

        sem_post(freespace);
    }


    munmap(myMemory, sizeof(*myMemory));
    close(shmfd);
    shm_unlink(SHM_NAME);

    sem_unlink(SEM_1);
    sem_unlink(SEM_2);
    sem_unlink(SEM_3);
    sem_close(freespace);
    sem_close(usedspace);
    sem_close(writeacces);

    return 0;

}