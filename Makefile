<<<<<<< HEAD
obj-m := joystick_module.o 
=======
obj-m += joystick_module.o 
>>>>>>> 262b98b37eb67dafaf74fb95e5926b73431fe861

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
