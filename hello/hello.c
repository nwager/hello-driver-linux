#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fs.h>

#define LOG(msg) "hello: " msg "\n"

#define SYSFS_ENTRY "hello"
#define SYSFS_FNAME hello_val
#define CDEV_LABEL "hello_cdev"

/*
 * General declarations
 */
static int  __init driver_init(void);
static void __exit driver_exit(void);

/*
 * sysfs declarations
 */
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf);
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count);

struct kobject *kobj_ref;
// File attributes
struct kobj_attribute kobj_attr = __ATTR(SYSFS_FNAME, 0660, sysfs_show, sysfs_store);

// Global value store
static int val = 0;

/*
 * Character device declarations
 */
static ssize_t cdev_read(struct file *f, char __user *u, size_t l, loff_t *o);

static int major;
static struct file_operations cdev_fops = {
	.read = cdev_read,
};

/**
 * Read from sysfs file.
 */
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf)
{
	pr_info(LOG("sysfs read"));
	return sprintf(buf, "%d\n", val);
}

/**
 * Write to sysfs file.
 */
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
	pr_info(LOG("sysfs write"));
	sscanf(buf, "%d", &val);
	return count;
}

/**
 * Read from character device.
 */
static ssize_t cdev_read(struct file *f, char __user *u, size_t l, loff_t *o)
{
	pr_info(LOG("Read from character device"));
	return 0;
}

static int __init driver_init(void)
{
	int error = 0;

	pr_info(LOG("Hello, kernel!"));

	major = register_chrdev(0, CDEV_LABEL, &cdev_fops);
	if (major < 0) {
		pr_err(LOG("Error registering cdev"));
		return major;
	}
	pr_info(LOG("Major device number: %d"), major);

	// Add directory entry to /sys/kernel/
	kobj_ref = kobject_create_and_add(SYSFS_ENTRY, kernel_kobj);
	if (kobj_ref == NULL) {
		pr_err(LOG("Error creating sysfs entry"));
		return -ENOMEM;
	}
	// Add file to the entry
	error = sysfs_create_file(kobj_ref, &kobj_attr.attr);
	if (error) {
		pr_err(LOG("Error creating sysfs file"));
		goto r_sysfs;
	}

	return 0;

r_sysfs:
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);

	return error;
}

static void __exit driver_exit(void)
{
	unregister_chrdev(major, CDEV_LABEL);
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
	pr_info(LOG("Goodbye, kernel!"));
}

module_init(driver_init);
module_exit(driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Noah Wager");
MODULE_DESCRIPTION("A hello world module");

