#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/mutex.h>

#define LOG(msg) "hello: " msg "\n"

#define SYSFS_ENTRY "hello"
#define SYSFS_FNAME hello_val
#define CDEV_LABEL "hello_cdev"
#define CDEV_CLASS "hello_cdev_cl"
#define CDEV_NAME "hello"

/*
 * General declarations
 */
static int  __init hello_init(void);
static void __exit hello_exit(void);

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
static int cdev_open(struct inode *inode, struct file *fp);
static int cdev_release(struct inode *inode, struct file *fp);
static ssize_t cdev_read(struct file *f, char __user *buf,
                         size_t len, loff_t *off);
static ssize_t cdev_write(struct file *f, const char __user *buf,
                          size_t len, loff_t *off);

static dev_t first; // Global variable for the first device number
static struct cdev c_dev; // Global variable for the character device structure
static struct class *cl; // Global variable for the device class
static int major;
static struct file_operations cdev_fops = {
	.read = cdev_read,
	.write = cdev_write,
	.open = cdev_open,
	.release = cdev_release,
};

#define CDEV_BUFSIZE 512
char *cdev_buf;
unsigned int cdev_buflen = 0;

/*
 * Locks
 */
DEFINE_MUTEX(cdev_lk);
DEFINE_MUTEX(sysfs_lk);

/**
 * Read from sysfs file.
 */
static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
                          char *buf)
{
	int n;

	pr_info(LOG("sysfs read"));
	mutex_lock(&sysfs_lk);
	n = sprintf(buf, "%d\n", val);
	mutex_unlock(&sysfs_lk);

	return n;
}

/**
 * Write to sysfs file.
 */
static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr,
                           const char *buf, size_t count)
{
	pr_info(LOG("sysfs write"));
	mutex_lock(&sysfs_lk);
	sscanf(buf, "%d", &val);
	mutex_unlock(&sysfs_lk);

	return count;
}

/**
 * Read from character device.
 */
static ssize_t cdev_read(struct file *f, char __user *buf,
                         size_t len, loff_t *off)
{
	size_t bytes_read, to_read;

	pr_info(LOG("Read from character device"));

	mutex_lock(&cdev_lk);

	to_read = *off + len > cdev_buflen ? cdev_buflen - *off : len;
	if (copy_to_user(buf + *off, cdev_buf, to_read)) {
		bytes_read = 0;
		goto r_unlock;
	}
	*off += to_read;
	bytes_read = to_read;

r_unlock:
	mutex_unlock(&cdev_lk);
	pr_info(LOG("Returning bytes_read = %lu with offset %lld"), bytes_read, *off);
	return bytes_read;
}

/**
 * Write to character device.
 */
static ssize_t cdev_write(struct file *f, const char __user *buf,
                          size_t len, loff_t *off)
{
	int error;
	size_t start;

	pr_info(LOG("Write to character device with offset %lld"), *off);

	mutex_lock(&cdev_lk);

	start = *off;
	if (f->f_flags & O_APPEND)
		start = cdev_buflen;

	if (start + len > CDEV_BUFSIZE) {
		pr_err(LOG("Write failed: not enough space in buffer"));
		error = -ENOMEM;
		goto r_unlock;
	}

	if (copy_from_user(cdev_buf + start, buf, len)) {
		error = -EFAULT;
		goto r_unlock;
	}
	cdev_buflen = start + len;
	*off += len;

	mutex_unlock(&cdev_lk);

	pr_info(LOG("Wrote %lu bytes"), len);
	return len;

r_unlock:
	mutex_unlock(&cdev_lk);
	return error;
}

/**
 * Open character device.
 */
static int cdev_open(struct inode *inode, struct file *fp)
{
	pr_info(LOG("Open dev node {major: %d, minor: %d}"), imajor(inode), iminor(inode));
	return 0;
}

/**
 * Release character device.
 */
static int cdev_release(struct inode *inode, struct file *fp)
{
	pr_info(LOG("File is closed"));
	return 0;
}

static int __init hello_init(void)
{
	int error = 0;

	pr_info(LOG("Hello, kernel!"));

	// Register character device number
	int minor = 0;
	major = register_chrdev(minor, CDEV_LABEL, &cdev_fops);
	if (major < 0) {
		pr_err(LOG("Error registering cdev"));
		return major;
	}
	first = MKDEV(major, minor);
	pr_info(LOG("Major device number: %d"), major);

	// Create sysfs class for character device
	cl = class_create(CDEV_CLASS);
	if (cl == NULL) {
		pr_err(LOG("Error creating cdev class"));
		error = -ENOMEM;
		goto r_unreg;
	}

	// Create device
	if (device_create(cl, NULL, first, NULL, CDEV_NAME) == NULL) {
		pr_err(LOG("Error creating device"));
		error = -ENOMEM;
		goto r_class;
	}

	// Add dev node
	cdev_init(&c_dev, &cdev_fops);
	if (cdev_add(&c_dev, first, 1) < 0) {
		pr_err(LOG("Error adding device file"));
		error = -ENOMEM;
		goto r_dev;
	}

	// Add directory entry to /sys/kernel/
	kobj_ref = kobject_create_and_add(SYSFS_ENTRY, kernel_kobj);
	if (kobj_ref == NULL) {
		pr_err(LOG("Error creating sysfs entry"));
		error = -ENOMEM;
		goto r_devnode;
	}

	// Add file to the entry
	error = sysfs_create_file(kobj_ref, &kobj_attr.attr);
	if (error) {
		pr_err(LOG("Error creating sysfs file"));
		goto r_sysfs;
	}

	cdev_buf = kmalloc(CDEV_BUFSIZE, GFP_KERNEL);
	if (cdev_buf == NULL) {
		pr_err(LOG("Error allocating memory for cdev buffer"));
		error = -ENOMEM;
		return error;
	}

	return 0;

r_sysfs:
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
r_devnode:
	cdev_del(&c_dev);
r_dev:
	device_destroy(cl, first);
r_class:
	class_destroy(cl);
r_unreg:
	unregister_chrdev(major, CDEV_LABEL);
	return error;
}

static void __exit hello_exit(void)
{
	kfree(cdev_buf);
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &kobj_attr.attr);
	cdev_del(&c_dev);
	device_destroy(cl, first);
	class_destroy(cl);
	unregister_chrdev(major, CDEV_LABEL);
	pr_info(LOG("Goodbye, kernel!"));
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Noah Wager");
MODULE_DESCRIPTION("A hello world module");

