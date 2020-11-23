#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/sched.h>
// #include <linux/delay.h>
#include <linux/proc_fs.h>
#include <net/inetpeer.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#define DEVICE_NAME "macro_man"
#define DEVICE_PROC_FILE_NAME "MacroManager_proc"
#define MACRO_FILE "/home/dj/test.txt"
#define BUFF_LEN 1000
#define MAX_MACRO_LENGTH 5

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group-19");
MODULE_DESCRIPTION("Keyboard Macro Manager");
MODULE_VERSION("0.1");

static struct input_dev *app_device;
static struct proc_dir_entry *app_proc;
static int keys_pressed[MAX_MACRO_LENGTH] = {-1, -1, -1, -1, -1};
static int recording = 0;
static int recording_full_macro = 0;
static int num_macros = 0;
static int i, j;
static long long int roffset = 0, woffset = 0;
char proc_buff[BUFF_LEN];
//--------------------------------------

// File operations

struct file *file_open(const char *path, int flags,int rights) {
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		printk(KERN_ERR "File open error: %d\n", err);
		return NULL;
	}
	return filp;
}

void file_close(struct file *file)
{
	filp_close(file,NULL);
}

int file_read(struct file *file, unsigned long long *offset, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_read(file, data, size, offset);
	set_fs(oldfs);
	return ret;
}

int file_write (struct file *file, unsigned long long *offset, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	ret = vfs_write(file, data, size, offset);

	set_fs(oldfs);
	return ret;
}

/**
* Macro related definitions
*/

static struct _full_macro {
	int size;
	int keycodes[20];
	int status[20];
};

static struct _macro {
    int keycodes[MAX_MACRO_LENGTH];
	int len;
	struct _full_macro keys;
	struct _macro *next;
} *macrosHead = NULL, *macrosTail = NULL, *curr_macro = NULL;

static struct _macro *tbd = NULL;

static int write_macro(struct file* mac_file, struct _macro* macro) {
	int ret = file_write(mac_file, &woffset, (char*)macro, sizeof(struct _macro));
	return ret;
}

static int write_macros(void) 
{
	struct file* mac_file = file_open(MACRO_FILE,O_CREAT | O_WRONLY | O_TRUNC, 
    						S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP);
	if(mac_file == NULL) {
		printk(KERN_ERR "Unable to open file!\n");
		return -1;
	}
	woffset = 0;
	curr_macro = macrosHead;
	while(curr_macro) {
		printk(KERN_INFO "Writing macro: %x\n",curr_macro);
		write_macro(mac_file,curr_macro);
		curr_macro = curr_macro->next;
	}
	file_close(mac_file);
	return 0;
}

static struct _macro* new_macro(void) {
	struct _macro* tmp = kzalloc(sizeof(struct _macro), GFP_KERNEL);

	for(i=0; i<MAX_MACRO_LENGTH; i++)
		tmp->keycodes[i] = -1;
	tmp->len = 0;
	tmp->next = NULL;
	tmp->keys.size = 0;
	return tmp;
}

static void add_macro(void) {
	num_macros++;
	if(macrosHead == NULL) {
		macrosHead = curr_macro;
		macrosTail = curr_macro;
	}
	else {
		macrosTail->next = curr_macro;
		macrosTail = curr_macro;
	}
		
	curr_macro = NULL;
}

static int read_macros(void)
{
	int cnt = 0;
	struct file* mac_file;
	// struct _macro* tmp_macro;
	if(macrosHead != NULL) {
		return -1;
	}
	mac_file = file_open(MACRO_FILE,O_RDONLY ,0);
	if(mac_file == NULL) {
		return 0;
	}
	curr_macro = new_macro();
	roffset = 0;
	while(file_read(mac_file,&roffset, (char*) curr_macro, sizeof(struct _macro)))
	{
		cnt++;
		curr_macro->next = NULL;
		add_macro();
		curr_macro = new_macro();
	}
	kfree(curr_macro);
	return cnt;
}

//-----------------------------------------

static void execute_macro(struct _full_macro tmp) {
	int cnt = 0;
	// input_report_key(app_device, 30, 1);
	// input_report_key(app_device, 30, 0);
	// input_sync(app_device);
	for(i=0; i<tmp.size; i++) {
		printk(KERN_INFO "%d : %d \n", tmp.keycodes[i], tmp.status[i]);
		input_report_key(app_device, tmp.keycodes[i], tmp.status[i]);
		// cnt += (2*tmp.status[i] - 1);
		// printk(KERN_INFO "%d\n", cnt);
		// if(cnt == 0)
		// 	input_sync(app_device);
	}
	input_sync(app_device);
}

// checks if current set of keys pressed is a macro
static void check_keys_pressed(struct input_handle *handle) {
	int f, cnt = 0, pos;
	struct _macro* tmp = macrosHead;

	for(j=0; j<MAX_MACRO_LENGTH; j++)
		if(keys_pressed[j] != -1)
			cnt++;

	while(tmp) {
		if(tmp->len == cnt) {
			pos = 0;
			for(i=0; i<tmp->len; i++) {
				f = 0;
				for(j=0; j<MAX_MACRO_LENGTH; j++) {
					if(keys_pressed[j] == tmp->keycodes[i]) {
						f++;
						break;
					}
				}
				if(!f) {
					pos++;
					break;
				}
				
			}

			if(!pos) {
				printk(KERN_INFO "A Macro is pressed: ");
				udelay(1000);
				// input_close_device(handle);
				// struct input_dev* dt = input_get_device(handle->dev);
				tbd = tmp;
				for(i=0; i<tmp->len; i++){
					// delay(5000);
					printk(KERN_INFO "K: %d \n", tmp->keycodes[i]);
					// input_report_key(dt,tmp->keycodes[i],0);
					// mdelay(5000);
					// input_report_key(handle->dev,tmp->keycodes[i],0);
					// if(i%2)
					// 	input_report_key(app_device, tmp->keycodes[i], 1);
					// input_report_key(app_device, tmp->keycodes[i], 0);
				}
				// input_open_device(handle);
				// input_sync(handle->dev);
				printk(KERN_INFO "\n");
				return;
				// execute_macro(tmp->keys);
			}
		}

		tmp = tmp->next;
	}
}

static int handle_key_press(struct input_handle *handle, int code, int pressed)
{
	int cnt=0;
	printk(KERN_INFO "%d : %d", code, pressed);
	if(recording) {
		// printk(KERN_INFO "Pressed Key Scancode is %d %d\n", code, pressed);
		if(pressed == 0) {
			// each key will be reported twice(press and release), take any one
			curr_macro->keycodes[curr_macro->len++] = code;
		}
	}
	else if(recording_full_macro) {
		printk(KERN_INFO "%d : %d", code, pressed);
		if(pressed < 2) {
			macrosTail->keys.keycodes[macrosTail->keys.size] = code;
			macrosTail->keys.status[macrosTail->keys.size] = pressed;
			macrosTail->keys.size++;
		}
	}
	else {
		if(pressed == 1) {
			for(i=0; i<MAX_MACRO_LENGTH; i++) {
				if(keys_pressed[i] == -1) {
					keys_pressed[i] = code;
					break;
				}
			}
			// check if current set of keys pressed is a macro
			check_keys_pressed(handle);
		}
		else if(pressed == 0) {
			// A key is released
			cnt = 0;
			for(i=0; i<MAX_MACRO_LENGTH; i++) {
				if(keys_pressed[i] == code) {
					keys_pressed[i] = -1;
					// break;
				}
				if(keys_pressed[i] != -1)
					cnt++;
			}
			// if(cnt == 0 && tbd){
			// 	printk(KERN_INFO "Executing macro!");
			// 	struct _macro *q = tbd;
			// 	tbd = NULL;
			// 	execute_macro(q->keys);
			// }
		}
	}

	return 0;
}

static void act_accordingly(void) {
	// printk(KERN_INFO "ACT ACC-> %s", proc_buff);
	if(strcmp(proc_buff, "start") == 0) {
		recording = 1;
		curr_macro = new_macro();
	}
	else if(strcmp(proc_buff, "end") == 0) {
		recording = 0;
		printk(KERN_INFO "Added New Macro: ");
		add_macro();
		// write procedure to save this as well
	}
	else if(strcmp(proc_buff, "startFullMacro") == 0) {
		recording_full_macro = 1;
	}
	else if(strcmp(proc_buff, "endFullMacro") == 0) {
		recording_full_macro = 0;
		write_macros();
	}
}


/**
* Input Handler and Device Definitions
*/
static bool app_filter(struct input_handle *handle, unsigned int type, unsigned int code, int value)
{
	// printk(KERN_INFO "What am i doing? %d %d %d\n",type,code,value);
	if(type != EV_SYN && handle->dev != app_device) {
		// input_sync(app_device);
		int cnt = 0;
		for(i=0; i<MAX_MACRO_LENGTH; i++) {
			if(keys_pressed[i] != -1)
				cnt++;
		}
		if(cnt == 0 && tbd){
			printk(KERN_INFO "Executing macro!\n");
			struct _macro *q = tbd;
			tbd = NULL;
			// input_sync(handle->dev);
			execute_macro(q->keys);
		}
	}
	if(type == EV_KEY && code != 272) // 272 is mouse button scancode
	{
		handle_key_press(handle, code, value);
	}
	return 0;
}

static int app_connect(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id)
{
	int err;
    struct input_handle *handle;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if(!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "keyboard_handle";

	err = input_register_handle(handle);
	if(err) {
        kfree(handle);
	    return err;
    }

	err = input_open_device(handle);
	if(err) {
        input_unregister_handle(handle);
        kfree(handle);
	    return err;
    }
    
    printk(KERN_INFO "Connected device: %s (%s at %s)\n",
		dev_name(&dev->dev), dev->name ?: "unknown", dev->phys ?: "unknown");
	return 0;
}

static void app_disconnect(struct input_handle *handle) {
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static int app_device_open(struct input_dev *dev) { return 0; }
static void app_device_close(struct input_dev *dev) {}

static const struct input_device_id app_ids[] = {
	{ .driver_info = 1 },
	{ },
};

MODULE_DEVICE_TABLE(input, app_ids);

static struct input_handler app_handler = {
	.filter 	=	app_filter,
	.connect 	=	app_connect,
	.disconnect =	app_disconnect,
	.name 		=	DEVICE_NAME,
	.id_table 	=	app_ids,
};
//--------------------------------------------

/**
* Proc File related definitions
*/
static ssize_t app_proc_write(struct file *fp, const char *buf, size_t len, loff_t *off) {
	printk(KERN_INFO "Write Handler\n");
	if(len > BUFF_LEN)
		return -EFAULT; 

	if(copy_from_user(proc_buff, buf, len))
		return -EFAULT;

	act_accordingly();
	return -1;
}

static ssize_t app_proc_read(struct file *fp, char *buff, size_t len, loff_t *off) {
	printk(KERN_INFO "Read Handler\n");
	return 0;
}

static struct file_operations app_proc_fops = {
	.owner = THIS_MODULE,
	.read  = app_proc_read,
	.write = app_proc_write,
};


static int __init my_init(void) {
    int err, i, num_keys;
    char file_buffer[2048];
    printk(KERN_INFO "HELLO\n");
    read_macros();
    // struct file* mac_file = file_open("/home/dj/test.txt",O_CREAT | O_RDWR, 
    // 						S_IWUSR | S_IRUSR);

    // file_read(mac_file,&roffset,file_buffer,1);
    // file_read(mac_file,&roffset,file_buffer+1,1);
    // printk(KERN_INFO "Found Line: %s\n",file_buffer);
    // file_close(mac_file);
    printk(KERN_INFO "Size: %d %d\n", sizeof(struct _macro),
    						sizeof(struct _full_macro));
    app_device = input_allocate_device();
    if(!app_device) {
        printk(KERN_INFO "Error in allocating device %s, Memory Full", DEVICE_NAME);
        return -ENOMEM;
    }
    printk(KERN_INFO "%s allocated succesfully\n", DEVICE_NAME);

    app_device->name = DEVICE_NAME;
    app_device->phys = "keyboard/input0";
    app_device->id.bustype = BUS_VIRTUAL;
	app_device->id.vendor = 0x0000;
	app_device->id.product = 0x0000;
	app_device->id.version = 0x0000;
    app_device->evbit[0] = BIT_MASK(EV_KEY);

    num_keys = KEY_CNT / BITS_PER_LONG;
    for(i=0; i<num_keys; i++) {
        app_device->keybit[i] = (1UL << BITS_PER_LONG)- 1;
    }

    app_device->open  = app_device_open;
	app_device->close = app_device_close;

	err = input_register_device(app_device);
	if(err) {
        printk(KERN_INFO "Error in registering device Macro Manager %d\n", err);
		input_free_device(app_device);
		return err;
	}
    printk(KERN_INFO "%s registered succesfully\n", DEVICE_NAME);

	err = input_register_handler(&app_handler);
	if(err) {
        printk(KERN_INFO "Registering input handler failed with (%d)\n", err);
		input_unregister_device(app_device);
        input_free_device(app_device);
        return err;
	}

	app_proc = proc_create(DEVICE_PROC_FILE_NAME, 0666, NULL, &app_proc_fops);
	// 0664 permission -> user space app can read, write but not execute
	if(!app_proc) {
		printk(KERN_INFO "Error in creating Proc file for %s\n", DEVICE_NAME);
	}

	return 0;
}

static void __exit my_exit(void) {
    input_unregister_handler(&app_handler);
    input_unregister_device(app_device);
    input_free_device(app_device);
	remove_proc_entry(DEVICE_PROC_FILE_NAME, NULL);
    printk(KERN_INFO "EXIT called for %s\n", DEVICE_NAME);
}

module_init(my_init);
module_exit(my_exit);

