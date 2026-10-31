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
- **Responsibilities**: Add start_elevator, issue_request, and stop_elevator system calls (548-550) to the kernel
- **Assigned to**: Maxwell Hackner

### Part 3b: Kernel Compilation
- **Responsibilities**: Compile and install linux-6.16.x with the new system calls
- **Assigned to**: Maxwell Hackner

### Part 3c: Threads
- **Responsibilities**: Elevator module that runs the elevator in a kthread and hooks into the system calls
- **Assigned to**: Maxwell Hackner

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
│   ├── build_kernel.py
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

### Adding System Calls & Kernel Compilation
```bash
sudo ln -s /usr/src/linux-6.16.x ~/linux-6.16.x
sudo python3 part3/build_kernel.py ~/linux-6.16.x --build
sudo reboot
uname -r
```
`build_kernel.py` does the following (safe to rerun, it skips anything already done):
- copies `part3/syscalls.c` into `[kernel]/syscalls/` and writes `syscalls/Makefile`
- adds syscalls 548-550 to `arch/x86/entry/syscalls/syscall_64.tbl`
- adds the syscall prototypes to `include/linux/syscalls.h`
- adds `syscalls/` to the `core-y` line in the top level kernel `Makefile`
- with `--build`: disables the trusted/revocation keys, then runs `make`,
  `make modules_install`, and `make install`

Leave off `--build` to only do the file edits. To tweak the config, run `make menuconfig`
in the kernel directory before using `--build`.

### Testing System Calls
```bash
cd part3/tests/system-calls-test
make
sudo insmod syscheck.ko
make run
sudo rmmod syscheck
```

### Compilation
```bash
cd part3
make
```
This will build the kernel module `elevator.ko` in `part3/`.
### Execution
```bash
sudo insmod elevator.ko
cd tests/elevator-test
make
./consumer --start
./producer [number_of_pets]
./consumer --stop
sudo rmmod elevator
```

## Development Log
Each member records their contributions here.

### Kian Sweeney

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-01 | Completed part 1       |
| 2026-10-21 | Completed part 2       |


### Maxwell Hackner

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-29 | Added system calls, kernel compilation steps, and elevator kthread |


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
