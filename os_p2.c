#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

int n_processes = 0;
int length = 0;
int column = 0;
int t_mem_move = 0;

struct Process {
    char id;
    char* mem_loc; // points to process' first frame in memory
    int mem_frames;
    int* arrival;
    int* ogArrival;
    int* length;
    int i_arrival;
    int n_arrival;
    int time_complete; // time when the current interval should finish
    struct pageTable* pages; // for non-contiguous memory
    int n_pages;
};

struct pageTable {
    char* startFrame;
    int length;
};

// return 1 if all processes are complete (i_arrival == n_arrival means complete)
int isAllComplete(struct Process* p) {
    int i;
    for (i = 0; i < n_processes; i++) {
        if (p[i].i_arrival != p[i].n_arrival) {
            return 0;
        }
    }
    return 1;
}

void printMem(char * memory) {
    int j = 0;
    for(int i = 0; i < column; i++) {
        printf("=");
    }
    printf("\n");
    for (int i = 0; i < length; i++) {
        if (j == column) {
            printf("\n");
            j = 0;
        }
        printf("%c", memory[i]);
        j++;
    }
    printf("\n");
    for(int i = 0; i < column; i++) {
        printf("=");
    }
    printf("\n");
}

void removeContiguousPartition(char * memory, struct Process* p, int counter) {
    // checks if p is in memory
    if (p->mem_loc == NULL) {
        return;
    }
    
    char* pStart = p->mem_loc;
    for(int i = 0; i < p->mem_frames; i++) {
        *(pStart + i) = '.';
    }
    p->mem_loc = NULL;
    
    printf("time %dms: Process %c removed:\n", counter, p->id);
    printMem(memory);
}


int defragmentation (char* memory, struct Process* ps, int time) {
    int p_start = -1, n_framesMoved = 0; // p_start is where the free memory partition starts
    char psMoved[n_processes*3];
    memset(psMoved, '\0', sizeof(psMoved));
    int i_psMoved = 0;
    int i, j;
    char currID;
    int n_pFrames;
    for (i = 0; i < length; i++) {
        // find start of free partition
        if (memory[i] == '.' && p_start == -1) {
            p_start = i;
        } else if (memory[i] != '.' && p_start != -1) {
            currID = memory[i];
            // get mem_frames for process with currID; also set process' mem_loc to start of new partition
            for (j = 0; j < n_processes; j++) {
                if (ps[j].id == currID) {
                    n_pFrames = ps[j].mem_frames;
                    ps[j].mem_loc = memory + p_start;
                    break;
                }
            }
            // shift memory down closer to the top of memory
            for (j = 0; j < n_pFrames; j++) {
                memory[p_start + j] = currID;
                memory[i + j] = '.';
            }
            n_framesMoved += n_pFrames;
            i = p_start + n_pFrames - 1;
            p_start = -1;
            if (psMoved[0] != '\0' && i_psMoved) {
                psMoved[i_psMoved] = ',';
                psMoved[i_psMoved+1] = ' ';
                i_psMoved += 2;
            }
            psMoved[i_psMoved] = currID;
            i_psMoved += 1;
        }
    }
    int defragTime = n_framesMoved * t_mem_move;
    // modify arrival and length of all processes
    for (i = 0; i < n_processes; i++) {
        if (ps[i].mem_loc != NULL) {
            // process is in memory, so increase the time that the process will complete
            ps[i].time_complete += defragTime;
        } else {
            ps[i].arrival[ps[i].i_arrival] += defragTime;
        }
        // increase future arrival times for all processes
        for (j = ps[i].i_arrival+1; j < ps[i].n_arrival; j++) {
            ps[i].arrival[j] += defragTime;
        }
    }
    
    printf("time %dms: Defragmentation complete (moved %d frames: %s)\n", time + defragTime, n_framesMoved, psMoved);
    return defragTime;
}

// implemented modulus function because % sometimes returned a negative #
int modulus(int x, int y) {
    while (x >= y) {
        x -= y;
    }
    return x;
}

int findContiguousPartition(char* memory, struct Process* p, char* lastProc, int time, int best) {
    int n_frames = p->mem_frames;
    int i, index;
    int i_start = (int)(lastProc - memory);
    int n_iterations = length + modulus( length - i_start, length );
    int i_currPartition = -1, currPartitionSize = 0;
    int i_bestPartition = -1, bestPartitionSize = length+1;
    // for loop will go from lastProc to the end of memory, then start from beginning, and then up to the end of memory
    //  this is because for Next-Fit algorithm, last process could have left before the next process arrived.. so
    //   memory is free before and after where lastProc points to (for First-Fit and Best-Fit, for loop will simply
    //    iterate from the beginning to the end of memory once because lastProc == memory)
    for (i = i_start; i < i_start + n_iterations; i++) {
        index = modulus(i, length);
        if (memory[index] == '.') {
            if (currPartitionSize == 0) {
                i_currPartition = index;
            }
            currPartitionSize++;
            
            // before the loop iterates from the end of memory to the beginning of memory, partition must reset
            //  so check if the last partition is valid (only for Next-Fit)
            if (index == length-1) {
                if (currPartitionSize >= n_frames && currPartitionSize < bestPartitionSize) {
                    i_bestPartition = i_currPartition;
                    bestPartitionSize = currPartitionSize;
                    if (best == 0) break;
                }
                currPartitionSize = 0;
            }
        } else {
            // this frame is allocated for another process
            if (currPartitionSize >= n_frames && currPartitionSize < bestPartitionSize) {
                i_bestPartition = i_currPartition;
                bestPartitionSize = currPartitionSize;
                if (best == 0) break;
            }
            currPartitionSize = 0;
        }
    }
    
    if (i_bestPartition != -1) {
        // update process' mem_loc
        p->mem_loc = memory + i_bestPartition;
        char pid = p->id;
        for (i = 0; i < n_frames; i++) {
            memory[i_bestPartition+i] = pid;
        }
        // placed the process in memory
        printf("time %dms: Placed process %c:\n", time, p->id);
        printMem(memory);
        return 1;
    }
    
    return 0;
}


void contiguousMemoryAllocation(struct Process* ps, char* algorithm) {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    
    int time = 0, n_freeMemory = length;
    printf("time %dms: Simulator started (Contiguous -- %s)\n", time, algorithm);
    int i, i_pArrival, added, defragTime;
    // in First-Fit and Best-Fit algorithm, lastProc will never be updated in the
    //  loop, so beginning of memory is always passed in as argument to func findContiguousPartition
    char* lastProc = memory;
    while ( isAllComplete(ps) == 0 ) {
        // check for finished processes (remove from memory)
        for (i = 0; i < n_processes; i++) {
            i_pArrival = ps[i].i_arrival;
            if (i_pArrival == ps[i].n_arrival) continue; // process is complete
            if (ps[i].time_complete == time) {
                // ps[i] process finished
                ps[i].i_arrival += 1;
                removeContiguousPartition(memory, &ps[i], time);
                n_freeMemory += ps[i].mem_frames;
            }
        }
        
        // check for arriving processes
        for (i = 0; i < n_processes; i++) {
            i_pArrival = ps[i].i_arrival;
            if (i_pArrival == ps[i].n_arrival) continue; // process is complete
            if (ps[i].arrival[i_pArrival] == time) {
                // ps[i] process arrives
                printf("time %dms: Process %c arrived (requires %d frames)\n", time, ps[i].id, ps[i].mem_frames);
                if ( strcmp(algorithm, "First-Fit") == 0 ) {
                    added = findContiguousPartition(memory, &ps[i], lastProc, time, 0);
                } else if ( strcmp(algorithm, "Next-Fit") == 0 ) {
                    added = findContiguousPartition(memory, &ps[i], lastProc, time, 0);
                } else if ( strcmp(algorithm, "Best-Fit") == 0 ) {
                    added = findContiguousPartition(memory, &ps[i], lastProc, time, 1);
                }
                
                if (added == 1) {
                    n_freeMemory -= ps[i].mem_frames;
                    ps[i].time_complete = time + ps[i].length[i_pArrival];
                    // update lastProc if algorithm is Next-Fit
                    if ( strcmp(algorithm, "Next-Fit") == 0 ) {
                        lastProc = ps[i].mem_loc + ps[i].mem_frames;
                        if (lastProc == memory + length) {
                            // if last process added was added at the end of memory and filled up to the end
                            //  so set next free space after last process as the beginnning of memory
                            lastProc = memory;
                        }
                    }
                } else {
                    // process could not be added because no available contiguous partition
                    if (ps[i].mem_frames <= n_freeMemory) {
                        // process can be added once memory is defragmented
                        printf("time %dms: Cannot place process %c -- starting defragmentation\n", time, ps[i].id);
                        defragTime = defragmentation(memory, ps, time);
                        time += defragTime;
                        if ( strcmp(algorithm, "First-Fit") == 0 ) {
                            findContiguousPartition(memory, &ps[i], lastProc, time, 0);
                        } else if ( strcmp(algorithm, "Next-Fit") == 0 ) {
                            findContiguousPartition(memory, &ps[i], lastProc, time, 0);
                        } else if ( strcmp(algorithm, "Best-Fit") == 0 ) {
                            findContiguousPartition(memory, &ps[i], lastProc, time, 1);
                        }
                        n_freeMemory -= ps[i].mem_frames;
                        ps[i].time_complete = time + ps[i].length[i_pArrival];
                        // update lastProc if algorithm is Next-Fit
                        if ( strcmp(algorithm, "Next-Fit") == 0 ) {
                            lastProc = ps[i].mem_loc + ps[i].mem_frames;
                            if (lastProc == memory + length) {
                                // if last process added was added at the end of memory and filled up to the end
                                //  so set next free space after last process as the beginnning of memory
                                lastProc = memory;
                            }
                        }
                    } else {
                        // process can not be added even if memory were to be defragmented so skip it
                        printf("time %dms: Cannot place process %c -- skipped!\n", time, ps[i].id);
                        ps[i].i_arrival += 1;
                    }
                }
            }
        }
        
        time++;
    }
    time--;
    printf("time %dms: Simulator ended (Contiguous -- %s)\n\n", time, algorithm);
    free(memory);
}


// non-Contiguous Memory Allocation Scheme
// nonC_findPartition searches for a First-Fit memory partition that can fit process p
void nonC_findPartition(char* memory, struct Process* p, int time) {
    int n_frames = p->mem_frames;
    char* partitionStart = NULL;
    int i = 0, partitionSize;
    struct pageTable* newPageTable = NULL;
    while (n_frames != 0) {
        if (memory[i] == '.') {
            // found a free memory frame.. determine size of whole partition
            partitionStart = memory + i;
            partitionSize = 0;
            while (partitionSize < n_frames && i < length && memory[i] == '.') {
                // simultaneously look for new frames and add process to memory for efficiency 8)
                memory[i] = p->id;
                partitionSize++;
                i++;
            }
            
            if (p->pages == NULL /* && p->n_pages == 0 */) {
                // first page to be added
                p->n_pages = 1;
                p->pages = (struct pageTable*) calloc(1, sizeof(struct pageTable));
                (p->pages)[0].startFrame = partitionStart;
                (p->pages)[0].length = partitionSize;
            } else {
                newPageTable = (struct pageTable*) calloc(p->n_pages + 1, sizeof(struct pageTable));
                memcpy(newPageTable, p->pages, sizeof(struct pageTable) * (p->n_pages));
                newPageTable[p->n_pages].startFrame = partitionStart;
                newPageTable[p->n_pages].length = partitionSize;
                free( p->pages );
                p->pages = newPageTable;
                newPageTable = NULL;
                (p->n_pages)++;
            }
            partitionStart = NULL;
            n_frames -= partitionSize;
        } else {
            i++;
        }
    }
    printf("time %dms: Placed process %c:\n", time, p->id);
    printMem(memory);
}

void nonC_removePartition(char* memory, struct Process* p, int time) {
    // checks if p is in memory
    if (p->pages == NULL) {
        return;
    }
    
    struct pageTable* pageTable = NULL;
    char* pageStart = NULL;
    int i_page, i_memory;
    for (i_page = 0; i_page < p->n_pages; i_page++) {
        pageTable = &(p->pages[i_page]);
        pageStart = pageTable->startFrame;
        for (i_memory = 0; i_memory < pageTable->length; i_memory++) {
            *(pageStart + i_memory) = '.';
        }
        pageTable->startFrame = NULL;
    }
    free( p->pages );
    p->pages = NULL;
    p->n_pages = 0;
    
    printf("time %dms: Process %c removed:\n", time, p->id);
    printMem(memory);
}

void nonContiguousMemoryAllocation(struct Process* ps) {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    int time = 0, n_freeMemory = length;
    printf("time %dms: Simulator started (Non-Contiguous)\n", time);
    
    int i, i_pArrival;
    while ( isAllComplete(ps) == 0 ) {
        // check for finished processes (remove from memory)
        for (i = 0; i < n_processes; i++) {
            i_pArrival = ps[i].i_arrival;
            if (i_pArrival == ps[i].n_arrival) continue; // process is complete
            if (ps[i].time_complete == time) {
                // ps[i] process finished
                ps[i].i_arrival += 1;
                nonC_removePartition(memory, &ps[i], time);
                n_freeMemory += ps[i].mem_frames;
            }
        }
        
        // check for arriving processes
        for (i = 0; i < n_processes; i++) {
            i_pArrival = ps[i].i_arrival;
            if (i_pArrival == ps[i].n_arrival) continue; // process is completed
            if (ps[i].arrival[i_pArrival] == time) {
                // ps[i] process arrives
                printf("time %dms: Process %c arrived (requires %d frames)\n", time, ps[i].id, ps[i].mem_frames);
                if (n_freeMemory >= ps[i].mem_frames) {
                    nonC_findPartition(memory, &ps[i], time);
                    n_freeMemory -= ps[i].mem_frames;
                    ps[i].time_complete = time + ps[i].length[i_pArrival];
                } else {
                    // process could not be added because not enough free memory
                    printf("time %dms: Cannot place process %c -- skipped!\n", time, ps[i].id);
                    ps[i].i_arrival += 1;
                }
            }
        }
        time++;
    }
    time--;
    printf("time %dms: Simulator ended (Non-Contiguous)\n", time);
    free(memory);
}

void parse(char * line, struct Process* p) {
    int i;
    // get processID
    for (i = 0; i < strlen(line); i++) {
        if (isspace(line[i]) || line[i] =='\0') {
            break;
        }
        p->id = line[i];
    }
    
    char* buffer = calloc (1024, sizeof(char));
    // get frames count of process
    i++; // skip over ' '
    int j = 0;
    for (; i < strlen(line); i++) {
        if (isspace(line[i]) || line[i] == '\0') {
            break;
        }
        buffer[j] = line[i];
        j++;
    }
    
    p->mem_frames = atoi(buffer); // assign process' # of frames
    int n_arrivals = 0;
    int* arrBuffer = (int*) calloc(10, sizeof(int));
    int* lenBuffer = (int*) calloc(10, sizeof(int));
    
    free(buffer);
    buffer = calloc(1024, sizeof(char));
    i++; // skip over ' '
    j = 0;
    for (; i < strlen(line); i++) {
        if (isspace(line[i])) {
            lenBuffer[n_arrivals] = atoi(buffer);
            free(buffer);
            buffer = calloc(1024, sizeof(char));
            j = 0;
            n_arrivals++;
            continue;
        } else if (line[i] == '\0') {
            lenBuffer[n_arrivals] = atoi(buffer);
            free(buffer);
            buffer = NULL;
            n_arrivals++;
            break;
        } else if (line[i] == '/') {
            arrBuffer[n_arrivals] = atoi(buffer);
            free(buffer);
            buffer = calloc(1024, sizeof(char));
            j = 0;
            continue;
        }
        buffer[j] = line[i];
        j++;
        
    }
    // move arrBuffer and lenBuffer to process
    p->n_arrival = n_arrivals;
    p->arrival = (int*) calloc(n_arrivals, sizeof(int));
    p->ogArrival = (int*) calloc(n_arrivals, sizeof(int));
    p->length = (int*) calloc(n_arrivals, sizeof(int));
    for (i = 0; i < n_arrivals; i++) {
        (p->arrival)[i] = arrBuffer[i];
        (p->ogArrival)[i] = arrBuffer[i];
        (p->length)[i] = lenBuffer[i];
    }
    free(arrBuffer);
    arrBuffer = NULL;
    free(lenBuffer);
    lenBuffer = NULL;
}

void resetProcessesStats(struct Process* ps) {
    int i;
    for (i = 0; i < n_processes; i++) {
        ps[i].mem_loc = NULL;
        ps[i].arrival = memcpy( ps[i].arrival, ps[i].ogArrival, ps[i].n_arrival * sizeof(int) );
        ps[i].i_arrival = 0;
        ps[i].time_complete = -1;
        ps[i].pages = NULL;
    }
}

int main(int argc, char ** argv) {
    length = atoi(argv[2]);
    column = atoi(argv[1]);
    char* filename = argv[3];
    t_mem_move = atoi(argv[4]);
    
    FILE* file = fopen (filename, "r");
    
    struct Process* proc = calloc(26, sizeof(struct Process));
    int i;
    for (i = 0; i < 26; i++) {
        proc[i].mem_loc = NULL;
        proc[i].mem_frames = 0;
        proc[i].arrival = NULL;
        proc[i].ogArrival = NULL;
        proc[i].length = NULL;
        proc[i].i_arrival = 0;
        proc[i].n_arrival = 0;
        proc[i].time_complete = -1;
        proc[i].pages = NULL;
        proc[i].n_pages = 0;
    }
    
    char line[100];
    n_processes = 0;
    if (file != NULL) {
        while (fgets(line, sizeof(line), file) != NULL) {
            // printf("line is %s", line);
            parse(line, &proc[n_processes]);
            /* printf("Process id is %s\n", proc[i].id);
            printf("process memory frames is %d\n", proc[i].mem_frames);
            for(int s = 0; s < 20; s++){
                printf("Process arrival time is %d\n", proc[i].arrival[s]);
                printf("Process length time is %d\n", proc[i].length[s]);
            } */
            n_processes++;
        }
        fclose(file);
    } else {
        perror ("open() failed");
    }
    
    // condense proc to not have extraneous empty Processes
    struct Process * procTemp = calloc(n_processes, sizeof(struct Process));
    for (i = 0; i < n_processes; i++) {
        procTemp[i] = proc[i];
    }
    free(proc);
    proc = procTemp;
    procTemp = NULL;
    
    contiguousMemoryAllocation(proc, "First-Fit");
    resetProcessesStats(proc);
    
    contiguousMemoryAllocation(proc, "Next-Fit");
    resetProcessesStats(proc);
    
    contiguousMemoryAllocation(proc, "Best-Fit");
    resetProcessesStats(proc);
    
    nonContiguousMemoryAllocation(proc);
    
}

