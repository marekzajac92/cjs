#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x15b2dc7b, "module_layout" },
	{ 0x60dacd3, "input_unregister_device" },
	{ 0xf634e88d, "input_register_device" },
	{ 0xc5e0de59, "input_set_abs_params" },
	{ 0xfda321fa, "input_allocate_device" },
	{ 0xa8bb5c8b, "input_event" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x8834396c, "mod_timer" },
	{ 0x7d11c268, "jiffies" },
	{ 0x3bd1b1f6, "msecs_to_jiffies" },
	{ 0x593a99b, "init_timer_key" },
	{ 0xd3c60f8b, "filp_open" },
	{ 0xe4351088, "vfs_read" },
	{ 0x85651533, "filp_close" },
	{ 0xf9a482f9, "msleep" },
	{ 0xc996d097, "del_timer" },
	{ 0x50eedeb8, "printk" },
	{ 0xb4390f9a, "mcount" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "70B6362385E6E24B4CF58D6");
