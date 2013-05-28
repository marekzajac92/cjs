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
#include <linux/delay.h>
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

static void chronos_ez430_send(const unsigned char *command, size_t size) {
	int i = 0;

	if(!ez430_fd) {
		printk(KERN_CRIT "CHRONOS EZ430 IS NOT OPEN!");
		return ;
	}

	// Logujemy wysyłaną komendę
	printk(KERN_INFO "CHRONOS EZ430 COMMAND: ");
	for(i = 0; i < size; i++) {
		printk(KERN_CONT "0x%x ", (int) command[i]);
	}
	printk(KERN_CONT "\n");

	ez430_fd->f_op->write(ez430_fd, command, size, NULL);
}

static void chronos_ez430_reply(char *buffor, size_t size) {
	int i = 0;

	if(!ez430_fd) {
		printk(KERN_CRIT "CHRONOS EZ430 IS NOT OPEN!");
		return ;
	}

	if(size == 0) {
		// Odczytujmey ilość dostępych bajtów
	}

	// Odczytujemy odpowiedź
	ez430_fd->f_op->read(ez430_fd, buffor, size, NULL);

	// Logujemy odpowiedź
	printk(KERN_INFO "CHRONOS EZ430 ANSWER: ");
	for(i = 0; i < size; i++) {
		printk(KERN_CONT "0x%x ", (unsigned char) buffor[i]);
	}
	printk(KERN_CONT "\n");
}

static void chronos_ez430_reset() {
	// RESET command
	chronos_ez430_send("\xFF\x01\x03", 3);
	//
	msleep(15);
}

static void chronos_ez430_status() {
	char status[4] = { 0 };

	// STATUS command
	chronos_ez430_send("\xFF\x00\x04\x00", 4);
	//
	msleep(15);

	chronos_ez430_reply(status, 4);
}

static void chronos_ez430_spl_start() {
	printk(KERN_INFO "CHRONOS EZ430 ACCESS POINT START\n");

	// ACCESS POINT START
	chronos_ez430_send("\xFF\x07\x03", 3);
	msleep(500);
}

static void chronos_ez430_spl_stop() {
	printk(KERN_INFO "CHRONOS EZ430 ACCESS POINT STOP\n");

	chronos_ez430_send("\xFF\x31\x16\x07\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 22);
	msleep(15);

	// Wczytujemy odpowiedź
	msleep(750);

	chronos_ez430_reply("\xFF\x09\x03", 4);
	msleep(15);
}

static void chronos_ez430_get_pos() {
	char data[4] = { 0 };

	printk(KERN_INFO "CHRONOS EZ430 GET POS\n");

	chronos_ez430_send("\xFF\x08\x07\x00\x00\x00\x00", 7);
	msleep(150);
	return ;

	// Odczytujemy zmianę pozycji
	chronos_ez430_reply(data, 4);
}

static void chronos_ez430_open(const char *device) {
	int i = 0;
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

	chronos_ez430_reset();
	msleep(20);

	for(i = 0; i < 10; i++) {
		chronos_ez430_status();
		msleep(10);
	}

	chronos_ez430_spl_start();

	printk(KERN_INFO "CHRONOS EZ430 OPEN SUCCESS\n");

}

static void chronos_ez430_close() {
	printk(KERN_INFO "CHRONOS EZ430 CLOSE\n");

	if(!IS_ERR(ez430_fd)) {
		chronos_ez430_spl_stop();

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
	if(mod_timer(&timer, jiffies + msecs_to_jiffies(150))) {
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
	chronos_ez430_get_pos();

	// Generujemy syganł
	chronos_joystick_interrupt();

	if(mod_timer(&timer,  jiffies + msecs_to_jiffies(150))) {
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
