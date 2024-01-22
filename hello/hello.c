#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Noah Wager");
MODULE_DESCRIPTION("A hello world module");

#define LOG(msg) "hello: " msg "\n"

static int __init hello_init(void) {
	printk(KERN_INFO LOG("Hello, kernel!"));
	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO LOG("Goodbye, kernel!"));
}

module_init(hello_init);
module_exit(hello_exit);

