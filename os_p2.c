#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>


int length = 0;
int column = 0;
int t_mem_move = 0;

struct Process {
    char* id;
    char* mem_loc; // points to process' first frame in memory
    int mem_frames;
    int* arrival;
    int* length;
    int arrival_num;
    int time_complete;
};

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

void remove_partition(char * memory, struct Process p, int counter) {
    int complete = -1;
    for(int i = 0; i < length; i++) {
        if (memory[i] == p.id[0]) {
            memory[i] = '.';
            if (i != strlen(memory)-1 && memory[i+1] != p.id[0]) {
                break;
            }
        }
    }
    
    printf("time %dms: Process %s removed:\n", counter, p.id);
    print_mem(memory);
}


int defragmentation (char* memory, int counter, struct Process* processes) {
    int p_start = 0;
    char * id = calloc (26, sizeof(char));
    int * id_length = calloc(26, sizeof(int));
    int num_moved = 0;
    for (int i = 0; i < length; i++) {
        //       printf("int i is %d\nmemory is %c\n", i, memory[i]);
        if (memory[i] == '.' && p_start == 0) {
            if (i == 0) {
                p_start = -1;
            } else {
                p_start = i;
            }
        } else if(memory[i] != '.' && p_start != 0) {
            int found = 0;
            // printf("memory[i] is %c\n", memory[i]);
            for(int j = 0; j < num_moved; j++) {
                if (id[j] == memory[i]) {
                    found = 1;
                    break;
                }
            }
            if (found == 0) {
                //                printf("moving %c\n", memory[i]);
                id[num_moved] = memory[i];
                num_moved++;
            }
            memory[i] = '.';
        }
    }
    
    //  printf("num moved is %d\n", num_moved);
    for (int i = 0; i < num_moved; i++) {
        for (int j = 0; j < 26; j++){
            if (id[i] == processes[j].id[0]) {
                id_length[i] = processes[j].mem_frames;
                break;
            }
        }
    }
    
    int placed = 0;
    int spaces = 0;
    int moved = 0;
    //  print_mem(memory);
    if(p_start == -1) {
        p_start = 0;
    }
    for(int i = p_start; i < length; i++) {
        //    printf("spaces is %d\n", spaces);
        if (spaces == id_length[placed]){
            //       printf("finished placing %c\n", id[placed]);
            //     print_mem(memory);
            placed++;
            //     printf("placing :%c       new length is %d\n", id[placed], id_length[placed]);
            spaces = 0;
        }
        if (placed == num_moved) {
            break;
        }
        memory[i] = id[placed];
        spaces++;
        moved++;
    }
    
    
    printf("time %dms: Defragmentation complete (moved %d frames: ", counter + (moved * t_mem_move), moved);
    for (int i = 0; i < num_moved - 1; i++) {
        printf("%c, ", id[i]);
    }
    printf("%c)\n", id[num_moved-1]);
    //print_mem(memory);
    
    return moved;
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

int findNextFitPartition(char* memory, struct Process* p, char* lastProc) {
    int n_frames = p->mem_frames;
    int i_lastProc = lastProc - memory;
    int i = 0, i_currPartition = i_lastProc, currPartitionSize = 0;
    int found = 0;
    // try i < length + lastProcesses's frame cause last process could have left before next
    // process arrived.. so memory is free before and after where lastProc is pointed to
    while (i < length) {
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
            i_currPartition = i_lastProc+i+1;
        }
        
        i++;
    }
    
    if (found == 1) {
        // update process' mem_loc
        p->mem_loc = memory + i_currPartition;
        char pid = (p->id)[0];
        for (i = 0; i < n_frames; i++) {
            memory[i_currPartition+i] = pid;
        }
    }
    
    return found;
}

void Next_Fit(struct Process* ps) {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    int time = 0, n_freeMemory = length;
    printf("time %dms: Simulator started (Contiguous -- Next-Fit)\n", counter);
    // points to the memory location right after the last added process's frames
    char* lastProc = memory;
    int i, i_pArrival, added;
    while (time < 5000) {
        // check for finished processes
        for (i = 0; i < 26; i++) {
            i_pArrival = ps[i].arrival_num;
            if (ps[i].arrival[i_pArrival] == -1) continue; // process does not exist or is completed
            if (ps[i].arrival[i_pArrival] + ps[i].length[i_pArrival] == time) {
                // ps[i] process finished
                ps[i].arrival_num += 1;
                i_pArrival++;
                // check if process will not arrive again in the future (completely done)
                if (ps[i].arrival[i_pArrival] == -1) {
                    ps[i].time_complete = time;
                }
                remove_partition(memory, ps[i], time);
                ps[i].mem_loc = NULL;
                n_freeMemory += ps[i].mem_frames;
            }
        }
        
        // check for arriving processes
        for (i = 0; i < 26; i++) {
            i_pArrival = ps[i].arrival_num;
            if (ps[i].arrival[i_pArrival] == -1) continue; // process does not exist or is completed
            if (ps[i].arrival[i_pArrival] == time) {
                // ps[i] process arrives
                added = findNextFitPartition(memory, &ps[i], lastProc);
                if (added == 1) {
                    n_freeMemory -= ps[i].mem_frames;
                    // update lastProc
                    lastProc = ps[i].mem_loc + ps[i].mem_frames;
                    if (lastProc == memory + length) {
                        // if last process added was added at the end of memory and filled up to the end
                        lastProc = memory;
                    }
                } else {
                    // process could not be added because no available partition
                    if (ps[i].mem_frames <= n_freeMemory) {
                        // process can be added once memory is defragmented
                        defragmentation(memory);
                        // memory+(length-n_freeMemory) should point to address in memory where
                        //  the first free memory is
                        findNextFitPartition( memory, &ps[i], memory+(length-n_freeMemory) );
                    } else {
                        // process can not be added even if memory were to be defragmented
                        // skip it?? according to PDF
                        ps[i].arrival_num++;
                        i_pArrival = ps[i].arrival_num;
                        if (ps[i].arrival[i_pArrival] == -1) {
                            // the skipped arrival was the process' last arrival, so it's completed
                            ps[i].time_complete = time;
                        }
                    }
                }
            }
        }
        
        
        
        time++;
    }
    
    
    free(memory);
}


int ff_find_partition(char * memory, struct Process p, int * counter, struct Process * processes) {
    int partition_space = 0;
    int total_space = 0;
    
    int p_space = p.mem_frames;
    int p_start = 0;
    int found = 0;
   // printf("p_space is %d\n", p_space);
    for (int i = 0; i < length; i++) {
        if (partition_space == p_space) {
            found = 1;
            break;
        }
        if (memory[i] != '.') {
            //printf("i is %d\n", i);
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
            memory[i] = p.id[0];
        }
        printf("time %dms: Placed process %s:\n", *counter, p.id);
        
    //if the total amount of space is large enough for the process, defragment memory
    } else if (total_space >= p_space) {
        printf("time %dms: Cannot place process %s -- starting defragmentation\n", *counter, p.id);
        int moved = defragmentation(memory, *counter, processes);
        moved = moved * t_mem_move;
        for (int i = 0; i < 26; i++) {
            if (processes[i].arrival[processes[i].arrival_num] != -1) {
                processes[i].arrival[processes[i].arrival_num] += moved;
            }
            if(processes[i].time_complete != -1) {
                processes[i].time_complete += moved;
            }
        }
        *counter += moved;
        ff_find_partition(memory, p, counter, processes);
        return 0;
    } else {
        //
        printf("time %dms: Cannot place process %s -- skipped!\n", *counter, p.id);
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
    int finished = 0;
    printf("time %dms: Simulator started (Contiguous -- First-Fit)\n", counter);
   // int stop_time = -1;
    int placed = 0;
    while(1) { //later change to when finished != process numbers
        finished = 0;
        for (int i = 0; i < 26; i++) {
            if (p[i].time_complete == counter) {
                remove_partition(memory, p[i], counter);
                p[i].time_complete = -1;
                placed--;
            }
        }
        
        for (int i = 0; i < 26; i++) {
            if(p[i].arrival[p[i].arrival_num] == counter){
                printf("time %dms: Process %s arrived (requires %d frames)\n", counter, p[i].id, p[i].mem_frames);
                int found = ff_find_partition(memory, p[i], &counter, p);
                if (found == -1) {
                    p[i].time_complete = -1;
                   // printf("time %dms: could not fit process %s\n", counter, p[i].id);
                } else {
                    p[i].time_complete = counter + p[i].length[p[i].arrival_num];
                    placed++;
                }
                p[i].arrival_num++;
            }
            if(p[i].arrival[p[i].arrival_num] == -1) {
                finished++;
            }
        }
        
        if (finished == 26 && placed == 0) {
            break;
        }
        counter++;
    }
    
    printf("time %dms: Simulator ended (Contiguous -- First-Fit)\n", counter);
    free(memory);
}

int parse(char * line, struct Process p) {
    char * temp = calloc (1024, sizeof(char));
    //printf("size of line is %lu\n", strlen(line));
    int j = 0;
    int i = 0;
    for (i = 0; i < strlen(line); i++) {
        if (isspace(line[i]) || line[i] =='\0') {
            break;
        }
        p.id[i] = line[i];
    }
    i++;
    for (; i < strlen(line); i++) {
        if (isspace(line[i]) || line[i] == '\0') {
            break;
        }
        temp[j] = line[i];
        j++;
    }
    int frames = atoi(temp);
   // p.mem_frames = atoi(temp);
  // printf("process memory frames is %d\n", p.mem_frames);
    free(temp);
    temp = calloc(1024, sizeof(char));
    i++;
    j = 0;
    int last = i;
    int x = 0;
    
    for (; i < strlen(line); i++) {
     //   printf("line[i] is %c\n", line[i]);
        if (isspace(line[i])) {
      //      printf("space - temp is %s\n", temp);
            p.length[x] = atoi(temp);
            free(temp);
            temp = calloc(1024, sizeof(char));
            x++;
            j = 0;
            continue;
        } else if (line[i] == '\0') {
            p.length[x] = atoi(temp);
            free(temp);
            break;
        } else if (line[i] == '/') {
        //    printf("slash - temp is %s\n", temp);
            p.arrival[x] = atoi(temp);
            free(temp);
            temp = calloc(1024, sizeof(char));
            j = 0;
            continue;
        }
        temp[j] = line[i];
        j++;
        
    }
    return frames;

}
/*

struct Process {
    char * id;
    int pid;
    int mem_frames;
    int * arrival;
    int * length;
};
*/
int main(int argc, char ** argv) {
    length = atoi(argv[2]);
    column = atoi(argv[1]);
    char * filename = argv[3];
    t_mem_move = atoi(argv[4]);
    
    FILE *file = fopen (filename, "r");
    
    struct Process * proc = calloc(26, sizeof(struct Process));
    for (int i = 0; i < 26; i++) {
        proc[i].id = (char*) calloc(4, sizeof(char));
        proc[i].arrival = (int*) calloc(40, sizeof(int));
        proc[i].length = (int*) calloc(40, sizeof(int));
        for (int s = 0; s < 40; s++) {
            proc[i].arrival[s] = -1;
            proc[i].length[s] = -1;
        }
        proc[i].arrival_num = 0;
        proc[i].time_complete = -1;
    }
    
    char line[100];
    int i = 0;
    if (file != NULL) {
        while (fgets(line, sizeof(line), file) != NULL) {
   //         printf("line is %s", line);
            proc[i].mem_frames = parse(line, proc[i]);
/*            printf("Process id is %s\n", proc[i].id);
            printf("process memory frames is %d\n", proc[i].mem_frames);
            for(int s = 0; s < 20; s++){
                printf("Process arrival time is %d\n", proc[i].arrival[s]);
                printf("Process length time is %d\n", proc[i].length[s]);
            }
*/            i++;
        }
        fclose(file);
    } else {
        perror ("open() failed");
    }
    
    First_Fit(proc);
    
    
    
}

