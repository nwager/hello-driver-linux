#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

static int  __init hello_init(void);
static void __exit hello_exit(void);

static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf);
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count);

#define LOG(msg) "hello: " msg "\n"

struct kobject *kobj_ref;
struct kobj_attribute kobj_attr = __ATTR(hello_val, 0660, sysfs_show, sysfs_store);
static int val = 0;

static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf)
{
	printk(KERN_INFO LOG("sysfs read"));
	return sprintf(buf, "%d\n", val);
}

static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
	printk(KERN_INFO LOG("sysfs write"));
	sscanf(buf, "%d", &val);
	return count;
}

static int __init hello_init(void)
{
	int error = 0;

	kobj_ref = kobject_create_and_add("hello", kernel_kobj);
	if (kobj_ref == NULL)
		return -ENOMEM;

	error = sysfs_create_file(kobj_ref, &kobj_attr.attr);
	if (error) {
		goto r_sysfs;
	}

	printk(KERN_INFO LOG("Hello, kernel!"));

	return 0;

r_sysfs:
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);

	return error;
}

static void __exit hello_exit(void)
{
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
	printk(KERN_INFO LOG("Goodbye, kernel!"));
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Noah Wager");
MODULE_DESCRIPTION("A hello world module");

