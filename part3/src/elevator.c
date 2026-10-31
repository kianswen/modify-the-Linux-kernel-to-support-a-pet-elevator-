#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("max hackner");
MODULE_DESCRIPTION("Pet elevator kernel module");
MODULE_VERSION("1.0");

#define MIN_FLOOR 1
#define MAX_FLOOR 5
#define MAX_PETS 5
#define MAX_WEIGHT 50

// pet types match what producer.c sends
#define CHIHUAHUA 0
#define PUG 1
#define PUGHUAHUA 2
#define DACHSHUND 3

enum elevator_state { OFFLINE, IDLE, LOADING, UP, DOWN };

struct elevator {
    int state;
    int current_floor;
    int current_load;
    int num_pets;
    bool deactivating; // set by stop, cleared once everyones off
    struct task_struct *thread;
    struct mutex lock;
};

static struct elevator elevator;

// these live in syscalls.c in the kernel
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

static int start_elevator(void) {
    mutex_lock(&elevator.lock);
    // already running
    if (elevator.state != OFFLINE) {
        mutex_unlock(&elevator.lock);
        return 1;
    }
    elevator.state = IDLE;
    elevator.current_floor = MIN_FLOOR;
    elevator.current_load = 0;
    elevator.num_pets = 0;
    elevator.deactivating = false;
    mutex_unlock(&elevator.lock);
    return 0;
}

// returns 1 for a bad request, 0 if its good
static int issue_request(int start_floor, int destination_floor, int type) {
    if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR)
        return 1;
    if (destination_floor < MIN_FLOOR || destination_floor > MAX_FLOOR)
        return 1;
    if (start_floor == destination_floor)
        return 1;
    if (type < CHIHUAHUA || type > DACHSHUND)
        return 1;
    // todo (3d): kmalloc the pet and add it to the floor list
    return 0;
}

static int stop_elevator(void) {
    mutex_lock(&elevator.lock);
    if (elevator.deactivating) {
        mutex_unlock(&elevator.lock);
        return 1;
    }
    // dont actually go offline here, the thread does it once its empty
    if (elevator.state != OFFLINE)
        elevator.deactivating = true;
    mutex_unlock(&elevator.lock);
    return 0;
}

// main elevator loop, runs until rmmod
static int elevator_run(void *data) {
    while (!kthread_should_stop()) {
        mutex_lock(&elevator.lock);
        if (elevator.deactivating && elevator.num_pets == 0) {
            elevator.state = OFFLINE;
            elevator.deactivating = false;
        }
        // todo (3f): move between floors (2s) and load/unload (1s)
        mutex_unlock(&elevator.lock);
        ssleep(1);
    }
    return 0;
}

static int __init elevator_init(void) {
    mutex_init(&elevator.lock);
    elevator.state = OFFLINE;
    elevator.current_floor = MIN_FLOOR;
    elevator.current_load = 0;
    elevator.num_pets = 0;
    elevator.deactivating = false;

    elevator.thread = kthread_run(elevator_run, NULL, "elevator_thread");
    if (IS_ERR(elevator.thread)) {
        mutex_destroy(&elevator.lock);
        return PTR_ERR(elevator.thread);
    }

    // hook our functions into the syscalls
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    return 0;
}

static void __exit elevator_exit(void) {
    // unhook first so no syscalls come in while were cleaning up
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;
    kthread_stop(elevator.thread);
    mutex_destroy(&elevator.lock);
}

module_init(elevator_init);
module_exit(elevator_exit);
