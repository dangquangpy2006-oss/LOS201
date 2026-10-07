#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LAB-02");
MODULE_DESCRIPTION("LAB-02 Character Device Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "lab2"
#define CLASS_NAME "lab2_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 240

static int major_number = DRIVER_MAJOR;

static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;
static int open_count;

static struct class *lab2_class;
static struct device *lab2_device;

static DEFINE_MUTEX(lab2_mutex);


/* =========================
 * CHARACTER DEVICE
 * ========================= */

static int lab2_open(struct inode *inode, struct file *file)
{
    mutex_lock(&lab2_mutex);

    open_count++;

    mutex_unlock(&lab2_mutex);

    pr_info("lab2: device opened, open_count=%d\n", open_count);

    return 0;
}


static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("lab2: device released\n");

    return 0;
}


static ssize_t lab2_read(struct file *file,
                         char __user *user_buffer,
                         size_t len,
                         loff_t *offset)
{
    size_t bytes_to_copy;

    if (*offset >= buffer_len)
        return 0;

    bytes_to_copy = min(len, buffer_len - (size_t)*offset);

    if (copy_to_user(user_buffer,
                     device_buffer + *offset,
                     bytes_to_copy))
        return -EFAULT;

    *offset += bytes_to_copy;

    pr_info("lab2: read %zu bytes\n", bytes_to_copy);

    return bytes_to_copy;
}


static ssize_t lab2_write(struct file *file,
                          const char __user *user_buffer,
                          size_t len,
                          loff_t *offset)
{
    size_t bytes_to_copy;

    bytes_to_copy = min(len, (size_t)(BUFFER_SIZE - 1));

    mutex_lock(&lab2_mutex);

    if (copy_from_user(device_buffer,
                       user_buffer,
                       bytes_to_copy)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }

    device_buffer[bytes_to_copy] = '\0';
    buffer_len = bytes_to_copy;

    mutex_unlock(&lab2_mutex);

    pr_info("lab2: wrote %zu bytes\n", bytes_to_copy);

    return bytes_to_copy;
}


static const struct file_operations lab2_fops = {
    .owner = THIS_MODULE,
    .open = lab2_open,
    .release = lab2_release,
    .read = lab2_read,
    .write = lab2_write,
};


/* =========================
 * PROCFS
 * ========================= */

static int lab2_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&lab2_mutex);

    seq_printf(m, "Driver name : %s\n", DEVICE_NAME);
    seq_printf(m, "Major       : %d\n", major_number);
    seq_printf(m, "Buffer size : %d\n", BUFFER_SIZE);
    seq_printf(m, "Data length : %zu\n", buffer_len);
    seq_printf(m, "Open count  : %d\n", open_count);
    seq_printf(m, "Last data   : %s\n", device_buffer);

    mutex_unlock(&lab2_mutex);

    return 0;
}


static int lab2_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, lab2_proc_show, NULL);
}


static const struct proc_ops lab2_proc_ops = {
    .proc_open = lab2_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};


/* =========================
 * SYSFS
 * ========================= */

static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);

    ret = sysfs_emit(buf, "%zu\n", buffer_len);

    mutex_unlock(&lab2_mutex);

    return ret;
}


static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);

    ret = sysfs_emit(buf, "%d\n", open_count);

    mutex_unlock(&lab2_mutex);

    return ret;
}


static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);

    ret = sysfs_emit(buf, "%s\n", device_buffer);

    mutex_unlock(&lab2_mutex);

    return ret;
}


static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);


static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};


static const struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};


/* =========================
 * MODULE INIT
 * ========================= */

static int __init lab2_init(void)
{
    int ret;

    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &lab2_fops);

    if (ret < 0) {
        pr_err("lab2: failed to register chrdev\n");
        return ret;
    }

    pr_info("lab2: registered with major %d\n",
            major_number);


    lab2_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(lab2_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }


    lab2_device = device_create(lab2_class,
                                NULL,
                                MKDEV(major_number, 0),
                                NULL,
                                DEVICE_NAME);

    if (IS_ERR(lab2_device)) {
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }


    ret = sysfs_create_group(&lab2_device->kobj,
                             &lab2_attr_group);

    if (ret) {
        device_destroy(lab2_class,
                       MKDEV(major_number, 0));
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);

        return ret;
    }


    if (!proc_create("lab2_info",
                     0444,
                     NULL,
                     &lab2_proc_ops)) {

        sysfs_remove_group(&lab2_device->kobj,
                           &lab2_attr_group);

        device_destroy(lab2_class,
                       MKDEV(major_number, 0));

        class_destroy(lab2_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return -ENOMEM;
    }


    pr_info("lab2: module loaded successfully\n");
    pr_info("lab2: /proc/lab2_info created\n");
    pr_info("lab2: sysfs attributes created\n");

    return 0;
}


/* =========================
 * MODULE EXIT
 * ========================= */

static void __exit lab2_exit(void)
{
    remove_proc_entry("lab2_info", NULL);

    sysfs_remove_group(&lab2_device->kobj,
                       &lab2_attr_group);

    device_destroy(lab2_class,
                   MKDEV(major_number, 0));

    class_destroy(lab2_class);

    unregister_chrdev(major_number,
                      DEVICE_NAME);

    pr_info("lab2: module unloaded\n");
}


module_init(lab2_init);
module_exit(lab2_exit);
