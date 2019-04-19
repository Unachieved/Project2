#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>


int length = 0;
int column = 0;
int t_mem_move = 0;

struct Process {
    char * id;
    int mem_frames;
    int * arrival;
    int * length;
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

void Best_Fit() {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    int counter = 0;
    printf("time %dms: Simulator started (Contiguous -- Best-Fit)", counter);

    free(memory);
}

void Next_Fit() {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    int counter = 0;
    printf("time %dms: Simulator started (Contiguous -- Next-Fit)\n", counter);
    
    
    free(memory);
}

void defragmentation (char * memory) {
    int p_start = 0;
    int p_end = 0;
    char * id_moved = calloc (26, sizeof(char));
    int * id_partition_space = calloc(26, sizeof(int));
    for(int i = 0; i < 26; i++) {
        id_partition_space[i] = 0;
    }
    int moved = 0;
    
    for (int i = 0; i < strlen(memory); i++) {
        if (p_start == 0 && memory[i] == '.') {
            p_start = i;
        } else if (p_start != 0 && memory[i] != '.') {
            int found = 0;
            for(int j = 0; j < moved; j++) {
                if (id_moved[j] == memory[i]) {
                    found = 1;
                    id_partition_space[j]++;
                    break;
                }
            }
            if (!found) {
                id_moved[moved] = memory[i];
                moved++;
            }
            memory[i] = '.';
        }
    }
/*    printf("processes moved %s\n", id_moved);
    for (int i = 0; i < 26; i++) {
        if (id_partition_space[i] != 0) {
            printf("partition space is %d\n", id_partition_space[i]++);
        }
    }
 
 */
    
    for(int i = 0; i < moved; i++){
        id_partition_space[i]++;
    }
    
//    print_mem(memory);
    int frames_moved = 0;
    int space = 0;
    for (int i = p_start; i < strlen(memory); i++) {
        for (int j = 0; j < id_partition_space[frames_moved]; j++) {
            memory[i + j] = id_moved[frames_moved];
        }
        i += (id_partition_space[frames_moved] - 1);
        frames_moved++;
        //print_mem(memory);
        if(frames_moved == moved) {
  //          printf("space left is %lu\n", strlen(memory) - i);
            break;
        }
        
    }
    free(id_moved);
}

int ff_find_partition(char * memory, struct Process p, int counter) {
    int partition_space = 0;
    int total_space = 0;
    
    int p_space = p.mem_frames;
    int p_start = 0;
    int found = 0;
   // printf("p_space is %d\n", p_space);
    for (int i = 0; i < strlen(memory); i++) {
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
    
   // printf("p_start is %d\n", p_start);
   // printf("partition space is %d\n", partition_space);
    //printf("p_space is %d\n", p_space);
    if (found == 1 || partition_space == p_space) {
        for (int i = p_start; i < p_start + p_space; i++){
            memory[i] = p.id[0];
        }
        printf("time %dms: Placed process %s:\n", counter, p.id);
    } else if (total_space >= p_space) {
        printf("time %dms: Cannot place process %s -- starting defragmentation!\n", counter, p.id);
        
        defragmentation(memory);
        printf("time %d: Defragmentation complete\n", counter);
        ff_find_partition(memory, p, counter);
        return 0;
    } else {
        printf("time %dms: Cannot place process %s -- skipped!\n", counter, p.id);
        return -1;
    }
    
    print_mem(memory);
    return 0;
}

void remove_partition(char * memory, struct Process p, int counter) {
    int complete = -1;
    for(int i = 0; i < strlen(memory); i++) {
        if (memory[i] == p.id[0]) {
            memory[i] = '.';
            if (memory[i+1] != p.id[0]) {
                complete = 0;
            }
        } else if (complete == 0) {
            break;
        }
    }
    
    printf("time %dms: Process %s removed:\n", counter, p.id);
    print_mem(memory);
}

void First_Fit(struct Process * p) {
    char * memory = calloc(length, sizeof(char));
    for (int i = 0; i < length; i++) {
        memory[i] = '.';
    }
    
    int counter = 0;
    int finished = 0;
    printf("time %dms: Simulator started (Contiguous -- First-Fit)\n", counter);
    
    for (int i = 0; i < 26; i++) {
        if (p[i].arrival[p[i].arrival_num] == -1) {
            finished++;
        }
    }

    while(counter < 5000) { //later change to when finished != process numbers
        for (int i = 0; i < 26; i++) {
            if(p[i].arrival[p[i].arrival_num] == counter){
                printf("time %dms: Process %s arrived (requires %d frames)\n", counter, p[i].id, p[i].mem_frames);
                int found = ff_find_partition(memory, p[i], counter);
                if (found == -1) {
                    p[i].time_complete = -1;
                   // printf("time %dms: could not fit process %s\n", counter, p[i].id);
                } else {
                    p[i].time_complete = counter + p[i].length[p[i].arrival_num];
                    printf("Process %s will complete at time %d\n", p[i].id, p[i].time_complete);
                }
                p[i].arrival_num++;
            }
        }
        
        for (int i = 0; i < 26; i++) {
            if (p[i].time_complete == counter) {
                remove_partition(memory, p[i], counter);
            }
        }
        counter++;
    }
    
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
        proc[i].id = calloc(4, sizeof(char));
        proc[i].arrival = calloc(40, sizeof(int));
        proc[i].length = calloc(40, sizeof(int));
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
