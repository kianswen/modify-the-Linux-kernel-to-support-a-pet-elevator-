#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

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

static const char *state_names[] = { "OFFLINE", "IDLE", "LOADING", "UP", "DOWN" };
static const int pet_weights[] = { 3, 14, 10, 16 };
static const char pet_letters[] = { 'C', 'P', 'H', 'D' };

// 3d: a pet is a kmalloc'd node that lives in exactly one linked list at a time
struct pet {
    struct list_head list;
    int type;
    int weight;
    int start_floor;
    int dest_floor;
    unsigned long long seq; // arrival order, used to find the oldest waiting pet
};

struct elevator {
    int state;
    int current_floor;
    int current_load;
    int num_pets;
    int num_waiting;
    int num_serviced;
    bool deactivating; // set by stop, cleared once everyones off
    unsigned long long next_seq;
    struct list_head riders;                   // pets inside the elevator
    struct list_head floors[MAX_FLOOR + 1];    // waiting pets, index 1..5
    struct task_struct *thread;
    struct mutex lock;                         // 3e: protects everything above
};

static struct elevator elevator;
static struct proc_dir_entry *elevator_proc;

// these live in syscalls.c in the kernel
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

/* ---------------------------- system call hooks ---------------------------- */

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
    struct pet *p;

    if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR)
        return 1;
    if (destination_floor < MIN_FLOOR || destination_floor > MAX_FLOOR)
        return 1;
    if (start_floor == destination_floor)
        return 1;
    if (type < CHIHUAHUA || type > DACHSHUND)
        return 1;

    // 3d: allocate the pet before taking the lock (kmalloc can sleep)
    p = kmalloc(sizeof(*p), GFP_KERNEL);
    if (!p)
        return -ENOMEM;
    p->type = type;
    p->weight = pet_weights[type];
    p->start_floor = start_floor;
    p->dest_floor = destination_floor;

    mutex_lock(&elevator.lock);
    p->seq = elevator.next_seq++;
    list_add_tail(&p->list, &elevator.floors[start_floor]); // FIFO per floor
    elevator.num_waiting++;
    mutex_unlock(&elevator.lock);
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

/* ------------------------- scheduling helpers (3f) ------------------------- */
/* all of these must be called with elevator.lock held */

// drop off every rider whose destination is this floor, returns how many
static int unload_pets(void) {
    struct pet *p, *tmp;
    int count = 0;

    list_for_each_entry_safe(p, tmp, &elevator.riders, list) {
        if (p->dest_floor != elevator.current_floor)
            continue;
        list_del(&p->list);
        elevator.current_load -= p->weight;
        elevator.num_pets--;
        elevator.num_serviced++;
        kfree(p);
        count++;
    }
    return count;
}

// board waiting pets on this floor in FIFO order, stop at the first that doesnt fit
static int load_pets(void) {
    struct pet *p, *tmp;
    int count = 0;

    list_for_each_entry_safe(p, tmp, &elevator.floors[elevator.current_floor], list) {
        if (elevator.num_pets + 1 > MAX_PETS)
            break;
        if (elevator.current_load + p->weight > MAX_WEIGHT)
            break;
        list_move_tail(&p->list, &elevator.riders);
        elevator.current_load += p->weight;
        elevator.num_pets++;
        elevator.num_waiting--;
        count++;
    }
    return count;
}

// where should we head next? 0 means nowhere (nothing to do)
static int pick_target_floor(void) {
    struct pet *p, *oldest = NULL;
    int f;

    // riders first: go to the destination of whoever got on first
    if (!list_empty(&elevator.riders)) {
        p = list_first_entry(&elevator.riders, struct pet, list);
        return p->dest_floor;
    }
    // stopping means no new pets get picked up
    if (elevator.deactivating)
        return 0;
    // otherwise go to the pet that has been waiting the longest
    for (f = MIN_FLOOR; f <= MAX_FLOOR; f++) {
        if (list_empty(&elevator.floors[f]))
            continue;
        p = list_first_entry(&elevator.floors[f], struct pet, list);
        if (!oldest || p->seq < oldest->seq)
            oldest = p;
    }
    return oldest ? oldest->start_floor : 0;
}

// main elevator loop, runs until rmmod
static int elevator_run(void *data) {
    int changed, target;

    while (!kthread_should_stop()) {
        mutex_lock(&elevator.lock);

        if (elevator.state == OFFLINE) {
            mutex_unlock(&elevator.lock);
            msleep(100);
            continue;
        }

        // stop was requested and everyone is off, shut down
        if (elevator.deactivating && elevator.num_pets == 0) {
            elevator.state = OFFLINE;
            elevator.deactivating = false;
            mutex_unlock(&elevator.lock);
            continue;
        }

        // unload and load in the same stop (no loading once stop was requested)
        changed = unload_pets();
        if (!elevator.deactivating)
            changed += load_pets();
        if (changed) {
            elevator.state = LOADING;
            mutex_unlock(&elevator.lock);
            ssleep(1);
            continue;
        }

        target = pick_target_floor();
        if (target == 0 || target == elevator.current_floor) {
            elevator.state = IDLE;
            mutex_unlock(&elevator.lock);
            msleep(100);
            continue;
        }

        // move exactly one floor, takes 2 seconds
        elevator.state = (target > elevator.current_floor) ? UP : DOWN;
        mutex_unlock(&elevator.lock);
        ssleep(2);
        mutex_lock(&elevator.lock);
        elevator.current_floor += (elevator.state == UP) ? 1 : -1;
        mutex_unlock(&elevator.lock);
    }
    return 0;
}

/* ------------------------------- proc file -------------------------------- */

static int elevator_proc_show(struct seq_file *m, void *v) {
    struct pet *p;
    int f;

    if (mutex_lock_interruptible(&elevator.lock))
        return -ERESTARTSYS;

    seq_printf(m, "Elevator state: %s\n", state_names[elevator.state]);
    seq_printf(m, "Current floor: %d\n", elevator.current_floor);
    seq_printf(m, "Current load: %d lbs\n", elevator.current_load);

    seq_puts(m, "Elevator status:");
    list_for_each_entry(p, &elevator.riders, list)
        seq_printf(m, " %c%d", pet_letters[p->type], p->dest_floor);
    seq_puts(m, "\n");

    for (f = MAX_FLOOR; f >= MIN_FLOOR; f--) {
        int count = 0;

        list_for_each_entry(p, &elevator.floors[f], list)
            count++;
        seq_printf(m, "[%c] Floor %d: %d", (f == elevator.current_floor) ? '*' : ' ', f, count);
        list_for_each_entry(p, &elevator.floors[f], list)
            seq_printf(m, " %c%d", pet_letters[p->type], p->dest_floor);
        seq_puts(m, "\n");
    }

    seq_printf(m, "Number of pets: %d\n", elevator.num_pets);
    seq_printf(m, "Number of pets waiting: %d\n", elevator.num_waiting);
    seq_printf(m, "Number of pets serviced: %d\n", elevator.num_serviced);

    mutex_unlock(&elevator.lock);
    return 0;
}

static int elevator_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, elevator_proc_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
    .proc_open = elevator_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/* --------------------------- module init / exit --------------------------- */

// free every pet left in a list
static void free_pet_list(struct list_head *head) {
    struct pet *p, *tmp;

    list_for_each_entry_safe(p, tmp, head, list) {
        list_del(&p->list);
        kfree(p);
    }
}

static int __init elevator_init(void) {
    int f;

    mutex_init(&elevator.lock);
    elevator.state = OFFLINE;
    elevator.current_floor = MIN_FLOOR;
    elevator.current_load = 0;
    elevator.num_pets = 0;
    elevator.num_waiting = 0;
    elevator.num_serviced = 0;
    elevator.deactivating = false;
    elevator.next_seq = 0;
    INIT_LIST_HEAD(&elevator.riders);
    for (f = MIN_FLOOR; f <= MAX_FLOOR; f++)
        INIT_LIST_HEAD(&elevator.floors[f]);

    elevator_proc = proc_create("elevator", 0444, NULL, &elevator_proc_ops);
    if (!elevator_proc) {
        mutex_destroy(&elevator.lock);
        return -ENOMEM;
    }

    elevator.thread = kthread_run(elevator_run, NULL, "elevator_thread");
    if (IS_ERR(elevator.thread)) {
        int err = PTR_ERR(elevator.thread);

        proc_remove(elevator_proc);
        mutex_destroy(&elevator.lock);
        return err;
    }

    // hook our functions into the syscalls
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    return 0;
}

static void __exit elevator_exit(void) {
    int f;

    // unhook first so no syscalls come in while were cleaning up
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;
    kthread_stop(elevator.thread);
    proc_remove(elevator_proc);

    // give back all memory
    free_pet_list(&elevator.riders);
    for (f = MIN_FLOOR; f <= MAX_FLOOR; f++)
        free_pet_list(&elevator.floors[f]);
    mutex_destroy(&elevator.lock);
}

module_init(elevator_init);
module_exit(elevator_exit);
