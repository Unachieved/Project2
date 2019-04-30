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

void print_mem(char * memory) {
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

void remove_partition(char * memory, struct Process* p, int counter) {
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
    print_mem(memory);
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


void Best_Fit() {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    int counter = 0;
    printf("time %dms: Simulator started (Contiguous -- Best-Fit)", counter);

    free(memory);
}

int findNextFitPartition(char* memory, struct Process* p, char* lastProc, int time) {
    int n_frames = p->mem_frames;
    int i_lastProc = lastProc - memory;
    int i = 0, i_currPartition = i_lastProc, currPartitionSize = 0;
    int found = 0;
    // while loop will go from lastProc to the end of memory, then start from beginning, and then end at end of memory
    //  this is because last process could have left before next process arrived.. so memory is free before and after
    //   where lastProc points to
    while (i < length + (length - (lastProc - memory))) {
        if (i_lastProc+i == length) {
            // reached end of memory, have to loop back to start with a new partition
            currPartitionSize = 0;
            i_currPartition = 0;
        }
        
        if (memory[(i_lastProc+i)%length] == '.') {
            currPartitionSize++;
            if (currPartitionSize == n_frames) {
                found = 1;
                break;
            }
        } else {
            // this frame is allocated for another process
            currPartitionSize = 0;
            i_currPartition = (i_lastProc+i+1)%length;
        }
        
        i++;
    }
    
    if (found == 1) {
        // update process' mem_loc
        p->mem_loc = memory + i_currPartition;
        char pid = p->id;
        for (i = 0; i < n_frames; i++) {
            memory[i_currPartition+i] = pid;
        }
        // placed the process in memory
        printf("time %dms: Placed process %c:\n", time, p->id);
        print_mem(memory);
    }
    
    return found;
}

void Next_Fit(struct Process* ps) {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    int time = 0, n_freeMemory = length;
    printf("time %dms: Simulator started (Contiguous -- Next-Fit)\n", time);
    // points to the memory location right after the last added process's frames
    char* lastProc = memory;
    int i, i_pArrival, added, defragTime;
    while ( isAllComplete(ps) == 0 ) {
        // check for finished processes (remove from memory)
        for (i = 0; i < n_processes; i++) {
            i_pArrival = ps[i].i_arrival;
            if (i_pArrival == ps[i].n_arrival) continue; // process is complete
            if (ps[i].time_complete == time) {
                // ps[i] process finished
                ps[i].i_arrival += 1;
                remove_partition(memory, &ps[i], time);
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
                added = findNextFitPartition(memory, &ps[i], lastProc, time);
                if (added == 1) {
                    n_freeMemory -= ps[i].mem_frames;
                    // update lastProc
                    lastProc = ps[i].mem_loc + ps[i].mem_frames;
                    if (lastProc == memory + length) {
                        // if last process added was added at the end of memory and filled up to the end
                        //  so set next free space after last process as the beginnning of memory
                        lastProc = memory;
                    }
                    ps[i].time_complete = time + ps[i].length[i_pArrival];
                } else {
                    // process could not be added because no available contiguous partition
                    if (ps[i].mem_frames <= n_freeMemory) {
                        // process can be added once memory is defragmented
                        printf("time %dms: Cannot place process %c -- starting defragmentation\n", time, ps[i].id);
                        defragTime = defragmentation(memory, ps, time);
                        time += defragTime;
                        // memory+(length-n_freeMemory) should point to address in memory where
                        //  the first free memory is
                        findNextFitPartition( memory, &ps[i], memory+(length-n_freeMemory), time );
                        n_freeMemory -= ps[i].mem_frames;
                        // update lastProc
                        lastProc = ps[i].mem_loc + ps[i].mem_frames;
                        if (lastProc == memory + length) {
                            // if last process added was added at the end of memory and filled up to the end
                            //  so set next free space after last process as the beginnning of memory
                            lastProc = memory;
                        }
                        ps[i].time_complete = time + ps[i].length[i_pArrival];
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
    printf("time %dms: Simulator ended (Contiguous -- Next-Fit)\n\n", time);
    free(memory);
}


int ff_find_partition(char * memory, struct Process* p, int * counter, struct Process * processes) {
    int partition_space = 0;
    int total_space = 0;
    
    int p_space = p->mem_frames;
    int p_start = 0;
    int found = 0;
    for (int i = 0; i < length; i++) {
        if (partition_space == p_space) {
            found = 1;
            break;
        } else if (memory[i] != '.') {
            partition_space = 0;
            p_start = i + 1;
            continue;
        }
        partition_space++;
        total_space++;
    }
    
    // if a partition large enough for the process is found
    if (found == 1 || partition_space == p_space) {
        for (int i = p_start; i < p_start + p_space; i++){
            memory[i] = p->id;
        }
        p->mem_loc = memory + p_start;
        printf("time %dms: Placed process %c:\n", *counter, p->id);
        
    //if the total amount of space is large enough for the process, defragment memory
    } else if (total_space >= p_space) {
        printf("time %dms: Cannot place process %c -- starting defragmentation\n", *counter, p->id);
        int defragTime = defragmentation(memory, processes, *counter);
        *counter += defragTime;
        ff_find_partition(memory, p, counter, processes);
        return 0;
    } else {
        //
        printf("time %dms: Cannot place process %c -- skipped!\n", *counter, p->id);
        return -1;
    }
    
    print_mem(memory);
    return 0;
}

void First_Fit(struct Process * p) {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    
    int counter = 0;
    printf("time %dms: Simulator started (Contiguous -- First-Fit)\n", counter);
    int i;
    while( isAllComplete(p) == 0 ) {
        // remove completed processes from memory
        for (i = 0; i < n_processes; i++) {
            if (p[i].time_complete == counter) {
                remove_partition(memory, &p[i], counter);
                p[i].i_arrival += 1;
                p[i].time_complete = -1;
            }
        }
        
        for (i = 0; i < n_processes; i++) {
            if (p[i].i_arrival == p[i].n_arrival) continue; // process is already finished
            if(p[i].arrival[p[i].i_arrival] == counter){
                printf("time %dms: Process %c arrived (requires %d frames)\n", counter, p[i].id, p[i].mem_frames);
                int found = ff_find_partition(memory, &p[i], &counter, p);
                if (found == -1) {
                    p[i].i_arrival += 1;
                    p[i].time_complete = -1;
                    // printf("time %dms: could not fit process %c\n", counter, p[i].id);
                } else {
                    p[i].time_complete = counter + p[i].length[p[i].i_arrival];
                }
            }
        }
        counter++;
    }
    counter--;
    printf("time %dms: Simulator ended (Contiguous -- First-Fit)\n\n", counter);
    free(memory);
}


void nonC_addFirstPartition(char* memory, struct Process* p, int time) {
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
    print_mem(memory);
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
    print_mem(memory);
}

void nonContiguous(struct Process* ps) {
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
                    nonC_addFirstPartition(memory, &ps[i], time);
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
    
    First_Fit(proc);
    resetProcessesStats(proc);
    Next_Fit(proc);
    resetProcessesStats(proc);
    
    nonContiguous(proc);
    
}

