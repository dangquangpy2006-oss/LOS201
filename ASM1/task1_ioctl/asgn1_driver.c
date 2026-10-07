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
#include  "asgn1_ioctl.h"
#include <linux/string.h>
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Dang Quang - SE201072");
MODULE_DESCRIPTION("ASM1 Character Device Driver with IOCTL");
MODULE_VERSION("1.0");

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 241

static int major_number = DRIVER_MAJOR;

static char device_buffer[BUFFER_SIZE];
static size_t buffer_len;
static int open_count;
static int write_count;
static int read_count;
static int last_write_size;
static int driver_mode = 1;
static struct class *lab2_class;
static struct device *lab2_device;

static DEFINE_MUTEX(asgn1_mutex);


/* =========================
 * CHARACTER DEVICE
 * ========================= */

static int asgn1_open(struct inode *inode, struct file *file)
{
    mutex_lock(&asgn1_mutex);

    open_count++;

    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1: device opened, open_count=%d\n", open_count);

    return 0;
}


static int asgn1_release(struct inode *inode, struct file *file)
{
    pr_info("asgn1: device released\n");

    return 0;
}


static ssize_t asgn1_read(struct file *file,
                         char __user *user_buffer,
                         size_t len,
                         loff_t *offset)
{
    size_t bytes_to_copy;

    mutex_lock(&asgn1_mutex);
    if (*offset >= buffer_len){
        mutex_unlock(&asgn1_mutex);
        return 0;
    }

    bytes_to_copy = min(len, buffer_len - (size_t)*offset);

    if (copy_to_user(user_buffer,
                     device_buffer + *offset,
                     bytes_to_copy)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
}
    *offset += bytes_to_copy;
    read_count++;
    mutex_unlock(&asgn1_mutex);
    pr_info("asgn1: read %zu bytes\n", bytes_to_copy);

    return bytes_to_copy;
}


static ssize_t asgn1_write(struct file *file,
                          const char __user *user_buffer,
                          size_t len,
                          loff_t *offset)
{
    size_t bytes_to_copy;
size_t prefix_len = 0;
const char *prefix = "[KERNEL]";
   if (driver_mode == 0) {
    prefix_len = strlen(prefix);
    bytes_to_copy = min(len, (size_t)(BUFFER_SIZE - 1 - prefix_len));
} else {
    bytes_to_copy = min(len, (size_t)(BUFFER_SIZE - 1));
}

    mutex_lock(&asgn1_mutex);

    if (driver_mode == 0) {
    memcpy(device_buffer, prefix, prefix_len);

    if (copy_from_user(device_buffer + prefix_len,
                       user_buffer,
                       bytes_to_copy)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    device_buffer[prefix_len + bytes_to_copy] = '\0';
    buffer_len = prefix_len + bytes_to_copy;
} else {
    if (copy_from_user(device_buffer,
                       user_buffer,
                       bytes_to_copy)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }

    device_buffer[bytes_to_copy] = '\0';
    buffer_len = bytes_to_copy;
}
    write_count++;
    last_write_size = bytes_to_copy;
    mutex_unlock(&asgn1_mutex);

    pr_info("asgn1: wrote %zu bytes\n", bytes_to_copy);

    return bytes_to_copy;
}

static long asgn1_ioctl(struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    struct asgn1_stats stats;
    int mode;
    char version[64];

    switch (cmd) {
    case ASGN1_RESET_BUFFER:
        mutex_lock(&asgn1_mutex);

        memset(device_buffer, 0, BUFFER_SIZE);
        buffer_len = 0;
        write_count = 0;
        read_count = 0;
        last_write_size = 0;

        mutex_unlock(&asgn1_mutex);

        pr_info("asgn1: ioctl RESET_BUFFER\n");
        return 0;

    case ASGN1_GET_STATS:
        mutex_lock(&asgn1_mutex);

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&asgn1_mutex);

        if (copy_to_user((void __user *)arg,
                         &stats,
                         sizeof(stats))) {
            return -EFAULT;
        }

        pr_info("asgn1: ioctl GET_STATS\n");
        return 0;

    case ASGN1_SET_MODE:
        if (copy_from_user(&mode,
                           (int __user *)arg,
                           sizeof(mode))) {
            return -EFAULT;
        }

        if (mode != 0 && mode != 1)
            return -EINVAL;

        mutex_lock(&asgn1_mutex);
        driver_mode = mode;
        mutex_unlock(&asgn1_mutex);

        pr_info("asgn1: ioctl SET_MODE mode=%d\n", mode);
        return 0;

    case ASGN1_GET_VERSION:
        memset(version, 0, sizeof(version));
        snprintf(version, sizeof(version), "asgn1_driver v1.0");

        if (copy_to_user((void __user *)arg,
                         version,
                         sizeof(version))) {
            return -EFAULT;
        }

        pr_info("asgn1: ioctl GET_VERSION\n");
        return 0;

    default:
        pr_info("asgn1: ioctl invalid command\n");
        return -ENOTTY;
    }
}
static const struct file_operations asgn1_fops = {
   
 .owner = THIS_MODULE,
    .open = asgn1_open,
    .release = asgn1_release,
    .read = asgn1_read,
    .write = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl,
};


/* =========================
 * PROCFS
 * ========================= */

static int lab2_proc_show(struct seq_file *m, void *v)
{
    mutex_lock(&asgn1_mutex);

    seq_printf(m, "Driver name : %s\n", DEVICE_NAME);
    seq_printf(m, "Major       : %d\n", major_number);
    seq_printf(m, "Buffer size : %d\n", BUFFER_SIZE);
    seq_printf(m, "Data length : %zu\n", buffer_len);
    seq_printf(m, "Open count  : %d\n", open_count);
    seq_printf(m, "Last data   : %s\n", device_buffer);

    mutex_unlock(&asgn1_mutex);

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

    mutex_lock(&asgn1_mutex);

    ret = sysfs_emit(buf, "%zu\n", buffer_len);

    mutex_unlock(&asgn1_mutex);

    return ret;
}


static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&asgn1_mutex);

    ret = sysfs_emit(buf, "%d\n", open_count);

    mutex_unlock(&asgn1_mutex);

    return ret;
}


static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    ssize_t ret;

    mutex_lock(&asgn1_mutex);

    ret = sysfs_emit(buf, "%s\n", device_buffer);

    mutex_unlock(&asgn1_mutex);

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

static int __init asgn1_init(void)
{
    int ret;

    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &asgn1_fops);

    if (ret < 0) {
        pr_err("asgn1: failed to register chrdev\n");
        return ret;
    }

    pr_info("asgn1: registered with major %d\n",
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


/*    if (!proc_create("lab2_info",
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
*/

    pr_info("asgn1: module loaded successfully\n");
 
    pr_info("asgn1: sysfs attributes created\n");

    return 0;
}


/* =========================
 * MODULE EXIT
 * ========================= */

static void __exit asgn1_exit(void)
{
  //O  remove_proc_entry("lab2_info", NULL);

    sysfs_remove_group(&lab2_device->kobj,
                       &lab2_attr_group);

    device_destroy(lab2_class,
                   MKDEV(major_number, 0));

    class_destroy(lab2_class);

    unregister_chrdev(major_number,
                      DEVICE_NAME);

    pr_info("asgn1: module unloaded\n");
}


module_init(asgn1_init);
module_exit(asgn1_exit);
