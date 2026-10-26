#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KianSweeneyCOP4610"); //thats me
MODULE_DESCRIPTION("A timer module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "timer" //name of the procf file
#define PERMS 0666 // read and write
#define PARENT NULL //no parent

static struct timespec64 lastTime; //last time since they checked the time

static struct proc_dir_entry* proc_entry; //pointer to the file
static bool checked;

static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t* ppos)
	{
	if (*ppos > 0) //if offset isnt 0, we have already read, return success
		return 0;
	char time_msg[128];  //message to print
	struct timespec64 curTime;
	ktime_get_real_ts64(&curTime); //curTime now has the current time
	sprintf(time_msg, "Current time is: %lld.%09ld\n",curTime.tv_sec, curTime.tv_nsec); //building a string to output
	if (checked) // we have checked before
		{
		strcat(time_msg, "elapsed time: "); //add this
		long long int  elap_s = curTime.tv_sec - lastTime.tv_sec; //elapsed seconds
		long int  elap_ns; //elapsed nano-seconds
		if (lastTime.tv_nsec > curTime.tv_nsec) //if the last nano second is larger than current one
			{
			elap_ns = 1000000000 + (curTime.tv_nsec - lastTime.tv_nsec); //1b - the difference then we decrement seconds
			elap_s--;
			}
		else
			elap_ns = curTime.tv_nsec - lastTime.tv_nsec; //regular subtraction

		char elap_msg[32]; //message for the elapsed time + . + \n + '\0'
		sprintf(elap_msg, "%lld.%09ld\n",elap_s,elap_ns); //the elapsed time message
		strcat(time_msg, elap_msg); //add the elapsed time to the full message
		}

	int msg_len = strlen(time_msg);
	if (count < msg_len)
		return 0; 
	if (copy_to_user(ubuf, time_msg, msg_len)) //if copy was a failure
		return -EFAULT;
	*ppos += msg_len; //update  offset
	lastTime = curTime; //update the last time
	checked = true;
	return msg_len; //return bytes read
	}


static const struct proc_ops pops = { //procfile operations
	.proc_read = procfile_read
};


int init_module()
	{
	checked = false; //set checked flag
	proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &pops); //create the file
	if (proc_entry == NULL) //out of memory of if file is still there
		{
		return -ENOMEM;
		}
	return 0;
	}

void cleanup_module()
	{
	proc_remove(proc_entry);
	}

