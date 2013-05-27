#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/termios.h>
#include <asm/uaccess.h>

// Serial port device
#define CHRONOS_EZ430_DEV "/dev/ttyACM0"

/**
 * CHRONOS EZ430 API:
 */
struct coords {
	int x;
	int y;
	int z;
} typedef COORDS;

//http://stackoverflow.com/questions/1184274/how-to-read-write-files-within-a-linux-kernel-module
static struct file* ez430_fd = 0;

static mm_segment_t old_fs;

static void chronos_ez430_open(const char *device) {
	struct termios settings;

	printk(KERN_INFO "CHRONOS EZ430 OPEN\n");

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	// Otwieramy plik urządzenia
	ez430_fd = filp_open(device, O_RDWR | O_NOCTTY, S_IRWXU|S_IRWXG|S_IRWXO);
	if(IS_ERR(ez430_fd)) {
		printk(KERN_ALERT "CHRONOS EZ430 OPEN ERROR: %d\n", (int) ez430_fd);
		return ;
	}

	printk(KERN_INFO "CHRONOS EZ430 SERIAL PORT CONFIGURATION\n");

	ez430_fd->f_op->unlocked_ioctl(ez430_fd, TCGETS,  (unsigned long) &settings);

	// Konfiguracja portu szeregowego
    settings.c_iflag = IGNBRK | B115200;
    settings.c_oflag = B115200;
    settings.c_cflag = CS8 | CREAD | CLOCAL;
    settings.c_lflag = 0;
    settings.c_cc[VMIN] = 1;

	ez430_fd->f_op->unlocked_ioctl(ez430_fd, TCSETS, (unsigned long) &settings);

	printk(KERN_INFO "CHRONOS EZ430 Hardware reset\n");

	printk(KERN_INFO "CHRONOS EZ430 OPEN SUCCESS\n");
}

static void chronos_ez430_close() {
	printk(KERN_INFO "CHRONOS EZ430 CLOSE\n");

	if(!IS_ERR(ez430_fd)) {
		// Zamykamy plik urządzenia (jeśli został otwarty)
		filp_close(ez430_fd, NULL);

		set_fs(old_fs);
	}
}

/**
 * KERNEL MODULE:
 */
static void timer_callback(unsigned long data);

static struct input_dev* joystick_dev;

static struct timer_list timer;

static int det = 100;

static void chronos_joystick_interrupt(void) {
	//printk(KERN_INFO "Chronos Joystick interrupt\n");

	det += 1;

	input_report_abs(joystick_dev, ABS_X, det % 255);
	input_report_abs(joystick_dev, ABS_Y, 0);
	input_report_abs(joystick_dev, ABS_Z, 0);
	//input_sync(joystick_dev);
	return;
}

static int chronos_joystick_open(char *filename) {
	printk(KERN_INFO "Chronos Joystick open_file\n");

	chronos_ez430_open(CHRONOS_EZ430_DEV);

	setup_timer(&timer, timer_callback, 0);

	// Ustawiamy timer
	if(mod_timer(&timer,  jiffies + msecs_to_jiffies(2))) {
		printk(KERN_INFO "Error in mod_timer\n");
	}

	return 0;
}

static void chronos_joystick_close(struct file* filp) {
	printk(KERN_INFO "Chronos Joystick close_file\n");

	if(del_timer(&timer)) {
		printk(KERN_INFO "The timer is still in use...\n");
	}

	chronos_ez430_close();
}

static void timer_callback(unsigned long data) {
	// Generujemy syganł
	chronos_joystick_interrupt();

	if(mod_timer(&timer,  jiffies + msecs_to_jiffies(2))) {
		printk(KERN_INFO "Error in mod_timer\n");
	}
}

static int __init init(void) {
	printk(KERN_INFO "Chronos Joystick Init\n");
	//	filp = open_file("/dev/ttyACM0");

	// Rejestracja urządzenia
	joystick_dev = input_allocate_device();
	joystick_dev->evbit[0] = BIT_MASK(EV_ABS);
	joystick_dev->absbit[BIT_WORD(ABS_X)] = BIT_MASK(ABS_X);
	joystick_dev->absbit[BIT_WORD(ABS_Y)] = BIT_MASK(ABS_Y);
	joystick_dev->absbit[BIT_WORD(ABS_Z)] = BIT_MASK(ABS_Z);
	joystick_dev->name = "Chronos Joystick";
	joystick_dev->open = chronos_joystick_open;
	joystick_dev->close = chronos_joystick_close;

	input_set_abs_params(joystick_dev, ABS_X, 0, 255, 4, 8);
	input_set_abs_params(joystick_dev, ABS_Y, 0, 255, 4, 8);
	input_set_abs_params(joystick_dev, ABS_Z, 0, 255, 4, 8);

	input_register_device(joystick_dev);

	return 0;
}

static void __exit exit(void)
{
	input_unregister_device(joystick_dev);

	printk(KERN_INFO "Chronos Joystick Exit\n");
	//close_file(filp);
}

MODULE_AUTHOR("Adam Wójs <adam@wojs.pl>, Marek Zając <marek.zajac92@gmail.com>");
MODULE_LICENSE("GPL");

module_init(init);
module_exit(exit);
