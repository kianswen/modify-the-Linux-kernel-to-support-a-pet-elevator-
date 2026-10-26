# Elevator

COP4610 Project 2

## Group Members
- **Kian Sweeney**: kms24b@fsu.edu
- **Liam Tingley**: lt22m@fsu.edu
- **Maxwell Hackner**: msh24@fsu.edu
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Compare System calls of a empty program, to a non empty program.
- **Assigned to**: Kian Sweeney 

### Part 2: Timer Kernel Module
- **Responsibilities**: Write a kernel module that displays time information when you read a proc file
- **Assigned to**: Kian Sweeney

### Part 3a: Adding System Calls
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown

### Part 3b: Kernel Compilation
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown, Jane Smith

### Part 3c: Threads
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown, Jane Smith

### Part 3d: Linked List
- **Responsibilities**: [Description]
- **Assigned to**: Jane Smith

### Part 3e: Mutexes
- **Responsibilities**: [Description]
- **Assigned to**: John Doe

### Part 3f: Scheduling Algorithm
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown, John Doe

## File Listing
```
elevator/
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   └── Makefile
├── part3/
│   ├── src/
│   ├── tests/
│   ├── Makefile
│   └── sys_call.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

## Part 1

Inspect and compare the files empty.c empty.trace part1.c part1.trace

## Part 2

### Compilation
For a C/C++ example:
```bash
make
```
This will build kernel module file ...
### Execution
```bash
sudo insmod my_timer.ko
```
This will install the module


## Part 3

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in ...
### Execution
```bash
make run
```
This will run the program ...

## Development Log
Each member records their contributions here.

### Kian Sweeney

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-01 | Completed part 1       |
| 2026-10-21 | Completed part 2       |


### [Member 2]

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


### [Member 3]

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |



## Bugs
- There are no known bugs at the moment.
