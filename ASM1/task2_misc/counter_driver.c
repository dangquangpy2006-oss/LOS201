#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
static atomic_t counter = ATOMIC_INIT(0);
static atomic_t read_count = ATOMIC_INIT(0);
static atomic_t write_count = ATOMIC_INIT(0);
static ssize_t counter_read(struct file *file,
                            char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
    char data[32];
    int value;
    int len;

    if (*ppos != 0)
        return 0;

    value = atomic_read(&counter);
atomic_inc(&counter);
atomic_inc(&read_count);

    len = snprintf(data, sizeof(data), "%d\n", value);

    if (count < len)
        return -EINVAL;

    if (copy_to_user(buf, data, len))
        return -EFAULT;

    *ppos += len;

    return len;
}
static ssize_t counter_write(struct file *file,
                             const char __user *buf,
                             size_t count,
                             loff_t *ppos)
{
    char data[32];
    int value;
    size_t len;

    len = count;

    if (len >= sizeof(data))
        len = sizeof(data) - 1;

    if (copy_from_user(data, buf, len))
        return -EFAULT;

    data[len] = '\0';

    if (kstrtoint(data, 10, &value))
        return -EINVAL;
    if (value < 0)
        return -EINVAL;

    atomic_set(&counter, value);
    atomic_inc(&write_count);
    *ppos = 0;

    return count;
}
static const struct file_operations counter_fops = {
    .owner = THIS_MODULE,
.read = counter_read,
    .write = counter_write,
};

static struct miscdevice counter_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "counter",
    .fops = &counter_fops,
};
static ssize_t counter_proc_read(struct file *file,
                                 char __user *buf,
                                 size_t count,
                                 loff_t *ppos)
{
    char data[128];
    int len;

    if (*ppos != 0)
        return 0;

    len = snprintf(data, sizeof(data),
                   "Counter: %d\n"
"Total reads: %d\n"
               "Total writes/resets: %d\n",
                   atomic_read(&counter),
   atomic_read(&read_count),
               atomic_read(&write_count));

    if (copy_to_user(buf, data, len))
        return -EFAULT;

    *ppos += len;

    return len;
}
static const struct proc_ops counter_proc_ops = {
    .proc_read = counter_proc_read,
};
static struct proc_dir_entry *counter_proc_entry;
static int __init counter_driver_init(void)
{
int ret;

    ret = misc_register(&counter_device);

    if (ret) {
        pr_err("counter_driver: misc_register failed\n");
        return ret;
    }
counter_proc_entry = proc_create("counter_info",
                                     0444,
                                     NULL,
                                     &counter_proc_ops);

    if (!counter_proc_entry) {
        misc_deregister(&counter_device);
        pr_err("counter_driver: proc_create failed\n");
        return -ENOMEM;
    }
    pr_info("counter_driver: module loaded\n");
  pr_info("counter_driver: /proc/counter_info created\n");
    return 0;
}

static void __exit counter_driver_exit(void)
{
 proc_remove(counter_proc_entry);

    misc_deregister(&counter_device);

    pr_info("counter_driver: module unloaded\n");
}

module_init(counter_driver_init);
module_exit(counter_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Dang Quang - SE201072");
MODULE_DESCRIPTION("ASM1 Counter Misc Device Driver");
MODULE_VERSION("1.0");
