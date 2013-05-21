#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>

//http://stackoverflow.com/questions/1184274/how-to-read-write-files-within-a-linux-kernel-module

struct file* filp;

struct coords{
	int x;
	int y;
	int z;
} typedef COORDS;

static COORDS crs;

static struct input_dev* joystick_dev;

static void joystick_interrupt(void)
{
	input_report_abs(joystick_dev, ABS_X, crs.x);
	input_report_abs(joystick_dev, ABS_Y, crs.y);
	input_report_abs(joystick_dev, ABS_Z, crs.z);
	//input_sync(joystick_dev);
	return;
}

static struct file* open_file(char *filename)
{
	struct file* filp = NULL;
	mm_segment_t old;
  old = get_fs();
  set_fs(get_ds());

  filp = filp_open(filename, O_RDWR, 0);

	set_fs(old);
	return filp;
}

static void close_file(struct file* filp)
{
	filp_close(filp, NULL);
}

static COORDS read_file(int fd)
{
	COORDS crd;
	//=========
	//tutaj kod pobierania wartosci...
	//=========
	return crd;
}

static void execute(void)
{
	//while(1)
	//{
		//read_file(fd);
		//joystick_interrupt();
		printk(KERN_INFO "Petla!\n");
	//}
}

static int __init init(void)
{
  //	filp = open_file("/dev/ttyACM0");
	joystick_dev = input_allocate_device();
	joystick_dev->evbit[0] = BIT_MASK(EV_ABS);
	joystick_dev->absbit[BIT_WORD(ABS_X)] = BIT_MASK(ABS_X);
	joystick_dev->absbit[BIT_WORD(ABS_Y)] = BIT_MASK(ABS_Y);
	joystick_dev->absbit[BIT_WORD(ABS_Z)] = BIT_MASK(ABS_Z);

	input_set_abs_params(joystick_dev, ABS_X, 0, 255, 4, 8);
	input_set_abs_params(joystick_dev, ABS_Y, 0, 255, 4, 8);
	input_set_abs_params(joystick_dev, ABS_Z, 0, 255, 4, 8);

	input_register_device(joystick_dev);
	execute();
  	return 0;
}

static void __exit exit(void)
{
	input_unregister_device(joystick_dev);
	//close_file(filp);
}

module_init(init);
module_exit(exit);
