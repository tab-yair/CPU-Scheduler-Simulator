#include "CPU-Scheduler.c"  

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <CSV_FILE> <TIME_QUANTUM>\n", argv[0]);
        return 1;
    }

    char* csv_file = argv[1];
    int time_quantum = atoi(argv[2]);
    if (time_quantum <= 0) {
        fprintf(stderr, "Time quantum must be greater than 0.\n");
        return 1;
    }

    runCPUScheduler(csv_file, time_quantum);
    return 0;
}
