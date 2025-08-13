# CPU Scheduler Simulator

## Overview
A C-based CPU Scheduler Simulator that implements **four scheduling algorithms**:  
- **FCFS (First-Come-First-Served)**  
- **SJF (Shortest Job First)**  
- **Priority Scheduling**  
- **Round Robin (RR)**  

The simulator reads process data from a CSV file and logs execution details, including idle times. It uses **fork and signals** to simulate process execution and calculates waiting and turnaround times for each algorithm.

---

## Features
- Non-preemptive scheduling: FCFS, SJF, Priority  
- Preemptive Round Robin scheduling with configurable time quantum  
- CSV input for process definitions  
- Detailed execution log showing process start and end times  
- Automatic calculation of **average waiting time** and **total turnaround time**  

---

## CSV Input Format
Each line in the CSV should have the following fields (comma-separated):

**Example:**

P1, Process 1, 0, 5, 2

P2, Process 2, 1, 3, 1

P3, Process 3, 2, 8, 3

---

## Compilation
Compile the source code using `gcc`:

```bash
gcc src/*.c -o scheduler


Run the simulator with your CSV file and a time quantum for Round Robin:
./scheduler processes.csv 2

---

## Example Output
FCFS Example:
══════════════════════════════════════════════
>> Scheduler Mode : FCFS
>> Engine Status  : Initialized
──────────────────────────────────────────────
0 → 5: P1 Running Process 1.
5 → 8: P2 Running Process 2.
8 → 16: P3 Running Process 3.
──────────────────────────────────────────────
>> Engine Status  : Completed
>> Summary        :
   └─ Average Waiting Time : 4.33 time units
>> End of Report
══════════════════════════════════════════════

Round Robin Example:
══════════════════════════════════════════════
>> Scheduler Mode : Round Robin
>> Engine Status  : Initialized
──────────────────────────────────────────────
0 → 2: P1 Running Process 1.
2 → 4: P2 Running Process 2.
4 → 6: P3 Running Process 3.
...
>> End of Report
══════════════════════════════════════════════

Project Structure
CPU-Scheduler-Simulator/
 ├─ src/                # Source code files (CPU-Scheduler.c, ex3.c, etc.)
 ├─ tests/              # CSV test inputs and expected outputs
 ├─ logs/               # Optional: execution logs
 └─ README.md           # This file

Notes
Ensure time quantum > 0 for Round Robin.

Idle times are handled automatically if no processes are ready to run.

Waiting time and turnaround time are calculated for each scheduling algorithm.

The simulator uses fork() to create child processes and signals for timing simulation.