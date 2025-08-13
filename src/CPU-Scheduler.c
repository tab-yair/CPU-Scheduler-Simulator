#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>

#define MAX_PROCESSES 1000
#define LINE_LENGTH 256
#define MAX_NAME_LEN 51
#define MAX_DESCRIPTION_LENGTH 101

// Process structure
typedef struct {
    char name[LINE_LENGTH];
    char description[LINE_LENGTH];
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
    int start_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    pid_t pid;
    int original_order;
} Process;

// Global variables
Process processes[MAX_PROCESSES];
int process_count = 0;
int current_time = 0;
volatile sig_atomic_t alarm_flag = 0;

// Function prototypes
// Signal handlers
void alarm_handler(int sig);
void setup_signal_handler(void);

// CSV parsing and process setup
int parse_csv(char* filename);
void reset_processes(Process* processes, int count);

// Utility printing
void print_header(const char* algorithm);
void print_avg_time(double avg_waiting_time);
void print_turnaround_time (int turnaround_time);

// Comparison functions for scheduling algorithms
int compare_fcfs(const void* a, const void* b);
int compare_sjf(const void* a, const void* b);
int compare_priority(const void* a, const void* b);

// Execution simulation
void execute_process(Process* proc, int duration);
void simulate_idle(int duration);

// Scheduling
void schedule_non_preemptive(const char *algo_name, int (*cmp_func)(const void *, const void *));
void RRschedule(int time_quantum);
void runCPUScheduler(char* csv_file, int time_quantum);



// alarm handler
void alarm_handler(int sig) {
    alarm_flag = 1; // Set the flag to indicate that the alarm has gone off
}

// signal setup
void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = alarm_handler; // Set the handler function
    sa.sa_flags = 0; // No special flags
    sigemptyset(&sa.sa_mask); // No additional signals to block
    sigaction(SIGALRM, &sa, NULL);
}


// Function to parse CSV file
int parse_csv(char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening CSV file");
        return -1;
    }
    
    char line[LINE_LENGTH];

    while (fgets(line, sizeof(line), file) && process_count < MAX_PROCESSES) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Parse CSV fields
        char* token = strtok(line, ",");
        if (!token) continue;
        
        strncpy(processes[process_count].name, token, MAX_NAME_LEN - 1);
        processes[process_count].name[MAX_NAME_LEN - 1] = '\0';
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(processes[process_count].description, token, LINE_LENGTH - 1);
        processes[process_count].description[LINE_LENGTH - 1] = '\0';
        
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[process_count].arrival_time = atoi(token);
        
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[process_count].burst_time = atoi(token);
        processes[process_count].remaining_time = processes[process_count].burst_time;
        
        token = strtok(NULL, ",");
        if (!token) continue;
        processes[process_count].priority = atoi(token);
        
        processes[process_count].original_order = process_count;
        process_count++;
    }
    
    fclose(file);
    return process_count;
}

void reset_processes(Process* processes, int count) {
    for (int i = 0; i < count; i++) {
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].waiting_time = 0;
        processes[i].start_time = -1;
        processes[i].completion_time = -1;
    }
}


void print_header(const char* algorithm) {
    char buffer[1024];
    int len = sprintf(buffer, 
        "══════════════════════════════════════════════\n"
        ">> Scheduler Mode : %s\n"
        ">> Engine Status  : Initialized\n"
        "──────────────────────────────────────────────\n\n",
        algorithm);
    write(STDOUT_FILENO, buffer, len);
}

void print_avg_time(double avg_waiting_time) {
    char buffer[1024];
    int len = sprintf(buffer,
        "\n──────────────────────────────────────────────\n"
        ">> Engine Status  : Completed\n"
        ">> Summary        :\n"
        "   └─ Average Waiting Time : %.2f time units\n"
        ">> End of Report\n"
        "══════════════════════════════════════════════\n\n",
        avg_waiting_time);
    write(STDOUT_FILENO, buffer, len);
}

void print_turnaround_time(int turnaround_time) {
    char buffer[1024];
    int len = sprintf(buffer,
        "\n──────────────────────────────────────────────\n"
        ">> Engine Status  : Completed\n"
        ">> Summary        :\n"
        "   └─ Total Turnaround Time : %d time units\n\n"
        ">> End of Report\n"
        "══════════════════════════════════════════════\n\n",
        turnaround_time);
    write(STDOUT_FILENO, buffer, len);
}

// Comparison functions for sorting
int compare_fcfs(const void* a, const void* b) {
    Process* p1 = *(Process**)a;
    Process* p2 = *(Process**)b;
    
    if (p1->arrival_time != p2->arrival_time)
        return p1->arrival_time - p2->arrival_time;
    return p1->original_order - p2->original_order;
}

int compare_sjf(const void* a, const void* b) {
    Process* p1 = *(Process**)a;
    Process* p2 = *(Process**)b;
    
    if (p1->burst_time != p2->burst_time)
        return p1->burst_time - p2->burst_time;
    if (p1->arrival_time != p2->arrival_time)
        return p1->arrival_time - p2->arrival_time;
    return p1->original_order - p2->original_order;
}

int compare_priority(const void* a, const void* b) {
    Process* p1 = *(Process**)a;
    Process* p2 = *(Process**)b;
    
    if (p1->priority != p2->priority)
        return p1->priority - p2->priority;
    if (p1->arrival_time != p2->arrival_time)
        return p1->arrival_time - p2->arrival_time;
    return p1->original_order - p2->original_order;
}

// Function to simulate process execution
void execute_process(Process* proc, int duration) {
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "%d → %d: %s Running %s.\n",
                   current_time, current_time + duration,
                   proc->name, proc->description);
    write(STDOUT_FILENO, buffer, len);

    // Simulate execution time
    alarm_flag = 0; // Reset the alarm flag
    alarm(duration);
    while (!alarm_flag) {
        pause(); // Wait for the alarm to go off
    }
}

// Function to simulate idle time
void simulate_idle(int duration) {
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "%d → %d: Idle.\n",
                   current_time, current_time + duration);
    write(STDOUT_FILENO, buffer, len);
    
    // Simulate idle time
    alarm(duration);
    pause();
}

void schedule_non_preemptive(const char *algo_name, int (*cmp_func)(const void *, const void *)) {
    print_header(algo_name);

    current_time = 0;
    float total_waiting_time = 0;
    int completed = 0;
    int executed[MAX_PROCESSES] = {0};  // Track which processes have finished

    while (completed < process_count) {
        // Collect all processes that have arrived and are not yet executed
        Process* ready_queue[MAX_PROCESSES];
        int ready_count = 0;

        for (int i = 0; i < process_count; i++) {
            if (!executed[i] && processes[i].arrival_time <= current_time) {
                ready_queue[ready_count++] = &processes[i];
            }
        }

        // If no processes are ready, accumulate idle time until next arrival
        if (ready_count == 0) {
            // Find the earliest arrival time of next process that hasn't executed yet
            int next_arrival = INT_MAX;
            for (int i = 0; i < process_count; i++) {
                if (!executed[i] && processes[i].arrival_time > current_time) {
                    if (processes[i].arrival_time < next_arrival)
                        next_arrival = processes[i].arrival_time;
                }
            }
            // If no future processes found, break (shouldn't happen if all processes eventually run)
            if (next_arrival == INT_MAX)
                break;

            int idle_duration = next_arrival - current_time;
            simulate_idle(idle_duration);
            current_time += idle_duration;
            continue;
        }

        // Sort the ready processes according to the scheduling criteria
        qsort(ready_queue, ready_count, sizeof(Process*), cmp_func);

        // Pick the first process after sorting (the "best" one according to cmp_func)
        Process* p = ready_queue[0];
        int index = p - processes;  // Calculate the index in the original array

        // Calculate the waiting time for this process
        processes[index].waiting_time = current_time - processes[index].arrival_time;
        if (processes[index].waiting_time < 0)
            processes[index].waiting_time = 0;

        total_waiting_time += processes[index].waiting_time;

        // Fork a child process to simulate execution
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process: execute the selected process for its burst time
            execute_process(&processes[index], processes[index].burst_time);
            exit(0);
        } else {
            // Parent process: wait for the child to finish and mark process as executed
            wait(NULL);
            current_time += processes[index].burst_time;
            executed[index] = 1;
            completed++;
        }
    }

    // Calculate and print average waiting time after all processes complete
    float avg_wait = total_waiting_time / process_count;
    print_avg_time(avg_wait);
}

// Round Robin scheduling
void RRschedule(int time_quantum) {
    if (time_quantum <= 0) {
        fprintf(stderr, "Time quantum must be greater than 0.\n");
        exit(EXIT_FAILURE);
    }

    print_header("Round Robin");

    // Use a dynamic array to maintain sorted ready queue
    int ready_queue[MAX_PROCESSES];
    int queue_size = 0;
    int completed = 0;
    int process_in_queue[MAX_PROCESSES] = {0}; // Track which processes are in queue

    current_time = 0;

    // Helper function to insert process into queue maintaining sort order
    auto void insert_into_queue(int proc_idx) {
        if (process_in_queue[proc_idx]) return; // Already in queue
        
        // Find insertion position (sorted by arrival time, then original order)
        int pos = 0;
        while (pos < queue_size) {
            int existing_idx = ready_queue[pos];
            if (processes[proc_idx].arrival_time < processes[existing_idx].arrival_time ||
                (processes[proc_idx].arrival_time == processes[existing_idx].arrival_time &&
                 processes[proc_idx].original_order < processes[existing_idx].original_order)) {
                break;
            }
            pos++;
        }
        
        // Shift elements to make room
        for (int i = queue_size; i > pos; i--) {
            ready_queue[i] = ready_queue[i-1];
        }
        
        ready_queue[pos] = proc_idx;
        queue_size++;
        process_in_queue[proc_idx] = 1;
    }

    // Helper function to add process to end of queue (for re-queuing after execution)
    auto void add_to_end_of_queue(int proc_idx) {
        ready_queue[queue_size] = proc_idx;
        queue_size++;
        process_in_queue[proc_idx] = 1;
    }

    // Helper function to remove process from front of queue
    auto int dequeue_process() {
        if (queue_size == 0) return -1;
        
        int proc_idx = ready_queue[0];
        for (int i = 0; i < queue_size - 1; i++) {
            ready_queue[i] = ready_queue[i + 1];
        }
        queue_size--;
        process_in_queue[proc_idx] = 0;
        return proc_idx;
    }

    while (completed < process_count) {
        // Add all processes that have arrived by current time to the ready queue
        for (int i = 0; i < process_count; i++) {
            if (processes[i].arrival_time <= current_time &&
                processes[i].remaining_time > 0 &&
                !process_in_queue[i]) {
                insert_into_queue(i);
            }
        }

        // If nothing is ready to run, advance time until next process arrives
        if (queue_size == 0) {
            int next_arrival = INT_MAX;
            for (int i = 0; i < process_count; i++) {
                if (processes[i].remaining_time > 0 && processes[i].arrival_time > current_time) {
                    if (processes[i].arrival_time < next_arrival) {
                        next_arrival = processes[i].arrival_time;
                    }
                }
            }
            
            if (next_arrival == INT_MAX) break;
            
            int idle_time = next_arrival - current_time;
            simulate_idle(idle_time);
            current_time = next_arrival;
            continue;
        }

        // Get the next process from the front of the queue
        int current_proc_idx = dequeue_process();
        if (current_proc_idx == -1) continue;

        Process* current_proc = &processes[current_proc_idx];
        int start_time = current_time;

        // Determine execution time for this time slice
        int exec_time = (current_proc->remaining_time < time_quantum) 
                       ? current_proc->remaining_time 
                       : time_quantum;

        // Execute the process
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            execute_process(current_proc, exec_time);
            exit(0);
        } else {
            // Parent process
            wait(NULL);
            current_time += exec_time;
            current_proc->remaining_time -= exec_time;

            // STEP 1: Add processes that arrived DURING execution (after start_time, before current_time)
            for (int i = 0; i < process_count; i++) {
                if (processes[i].arrival_time > start_time &&
                    processes[i].arrival_time < current_time &&
                    processes[i].remaining_time > 0 &&
                    !process_in_queue[i]) {
                    insert_into_queue(i);
                }
            }

            // STEP 2: Re-add the current process if not completed
            if (current_proc->remaining_time == 0) {
                current_proc->completion_time = current_time;
                current_proc->turnaround_time = current_time - current_proc->arrival_time;
                current_proc->waiting_time = current_proc->turnaround_time - current_proc->burst_time;
                completed++;
            } else {
                // Process still has work to do, add it to the end of queue
                add_to_end_of_queue(current_proc_idx);
            }

            // STEP 3: Add processes that arrive exactly at current_time (after re-adding current process)
            for (int i = 0; i < process_count; i++) {
                if (processes[i].arrival_time == current_time &&
                    processes[i].remaining_time > 0 &&
                    !process_in_queue[i]) {
                    insert_into_queue(i);
                }
            }
        }
    }

    print_turnaround_time(current_time);
}

void runCPUScheduler(char* csv_file, int time_quantum) {
    // Set up signal handler for alarm
    setup_signal_handler();

    parse_csv(csv_file);
    
    reset_processes(processes,  process_count);
    schedule_non_preemptive("FCFS", compare_fcfs);
    reset_processes(processes,  process_count);
    schedule_non_preemptive("SJF", compare_sjf);
    reset_processes(processes,  process_count);
    schedule_non_preemptive("Priority", compare_priority);
    reset_processes(processes,  process_count);
    RRschedule(time_quantum);
}

