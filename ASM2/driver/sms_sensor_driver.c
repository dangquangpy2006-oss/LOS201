/*
 * Sensor Monitoring System - Kernel Driver
 *
 * Purpose: Simulate temperature and humidity sensors.
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 * Date: 10 October 2026
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/random.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>

#include "sms_sensor.h"

/* Driver information */
#define DRIVER_NAME "sms_sensor"

/* Shared driver state */
static DEFINE_MUTEX(sensor_lock);

static unsigned long read_count;
static unsigned int last_temperature_x10;
static unsigned int last_humidity_x10;
static unsigned int sampling_interval_ms = SMS_DEFAULT_INTERVAL_MS;

static u64 start_time_ms;
static u64 last_read_time_ms;

/* Character device operations - implemented next */
static ssize_t sms_read(struct file *file,
                        char __user *buffer,
                        size_t count,
                        loff_t *offset);

static long sms_ioctl(struct file *file,
                      unsigned int cmd,
                      unsigned long arg);

/* Character device interface */
static const struct file_operations sms_fops = {
    .owner = THIS_MODULE,
    .read = sms_read,
    .unlocked_ioctl = sms_ioctl,
    .llseek = no_llseek,
};

/*
 * Read simulated sensor data.
 *
 * Output format:
 * timestamp_ms,temperature,humidity
 */
static ssize_t sms_read(struct file *file,
                        char __user *buffer,
                        size_t count,
                        loff_t *offset)
{
    char data[128];
    int len;
    u64 now_ms;
    unsigned int temperature_x10;
    unsigned int humidity_x10;

    (void)file;

   

    mutex_lock(&sensor_lock);

   now_ms = ktime_to_ms(ktime_get());

    /* Enforce minimum sampling interval */
    if (last_read_time_ms != 0 &&
        now_ms - last_read_time_ms < sampling_interval_ms) {
        mutex_unlock(&sensor_lock);
        return -EAGAIN;
    }

    /* Temperature: 15.0 - 45.0 C */
    temperature_x10 = 150 + get_random_u32() % 301;

    /* Humidity: 30.0 - 90.0 % */
    humidity_x10 = 300 + get_random_u32() % 601;

    len = scnprintf(
        data,
        sizeof(data),
        "%llu,%u.%u,%u.%u\n",
        (unsigned long long)now_ms,
        temperature_x10 / 10,
        temperature_x10 % 10,
        humidity_x10 / 10,
        humidity_x10 % 10
    );

    if (count < len) {
        mutex_unlock(&sensor_lock);
        return -EINVAL;
    }

    if (copy_to_user(buffer, data, len)) {
        mutex_unlock(&sensor_lock);
        return -EFAULT;
    }

    read_count++;

    last_temperature_x10 = temperature_x10;
    last_humidity_x10 = humidity_x10;
    last_read_time_ms = now_ms;

    mutex_unlock(&sensor_lock);

   (void)offset;

    return len;
}
/*
 * Configure the minimum sensor reading interval.
 *
 * SMS_SET_INTERVAL receives an unsigned integer
 * representing milliseconds.
 */
static long sms_ioctl(struct file *file,
                      unsigned int cmd,
                      unsigned long arg)
{
    unsigned int interval_ms;

    (void)file;

    /* Reject unsupported ioctl commands */
    if (cmd != SMS_SET_INTERVAL)
        return -ENOTTY;

    /* Read the interval provided by userspace */
    if (copy_from_user(&interval_ms,
                       (void __user *)arg,
                       sizeof(interval_ms)))
        return -EFAULT;

    /* Reject invalid values */
    if (interval_ms == 0 || interval_ms > 60000)
        return -EINVAL;

    mutex_lock(&sensor_lock);

    sampling_interval_ms = interval_ms;

    mutex_unlock(&sensor_lock);

    pr_info("sms_sensor: interval set to %u ms\n",
            interval_ms);

    return 0;
}
/*
 * Display sensor statistics through /proc/sms_stats.
 */
static int sms_proc_show(struct seq_file *m, void *v)
{
    u64 now_ms;
    u64 uptime_seconds;

    unsigned long count;
    unsigned int temp;
    unsigned int hum;

    (void)v;

    mutex_lock(&sensor_lock);

    count = read_count;
    temp = last_temperature_x10;
    hum = last_humidity_x10;

    mutex_unlock(&sensor_lock);

   now_ms = ktime_to_ms(ktime_get());
   uptime_seconds = div_u64(now_ms - start_time_ms, 1000);

    seq_printf(m, "read_count: %lu\n", count);

    seq_printf(m, "last_temperature: %u.%u\n",
               temp / 10, temp % 10);

    seq_printf(m, "last_humidity: %u.%u\n",
               hum / 10, hum % 10);

    seq_printf(m, "uptime_seconds: %llu\n",
               (unsigned long long)uptime_seconds);

    return 0;
}

/*
 * Open /proc/sms_stats.
 */
static int sms_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, sms_proc_show, NULL);
}

/*
 * Operations supported by the procfs entry.
 */
static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};
/*
 * Character device registration.
 */
static struct miscdevice sms_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DRIVER_NAME,
    .fops = &sms_fops,
    .mode = 0666,
};

/*
 * Initialize the sensor driver.
 */
static struct proc_dir_entry *sms_proc_entry;
static int __init sms_init(void)
{
    int ret;

    read_count = 0;
    last_temperature_x10 = 0;
    last_humidity_x10 = 0;

    sampling_interval_ms = SMS_DEFAULT_INTERVAL_MS;

   start_time_ms = ktime_to_ms(ktime_get());
    last_read_time_ms = 0;

    ret = misc_register(&sms_misc_device);

    if (ret) {
        pr_err("sms_sensor: Failed to register device\n");
        return ret;
    }
sms_proc_entry = proc_create("sms_stats", 0444, NULL, &sms_proc_ops);
   if (!sms_proc_entry) {
    pr_err("sms_sensor: Failed to create /proc/sms_stats\n");

        misc_deregister(&sms_misc_device);

        return -ENOMEM;
    }

    pr_info("sms_sensor: Driver initialized successfully\n");
    pr_info("sms_sensor: Device /dev/sms_sensor registered\n");

    return 0;
}

/*
 * Clean up the sensor driver.
 */
static void __exit sms_exit(void)
{
   proc_remove(sms_proc_entry);
    misc_deregister(&sms_misc_device);

    pr_info("sms_sensor: Driver unloaded successfully\n");
}
module_init(sms_init);
module_exit(sms_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Dang Quang - SE201072");
MODULE_DESCRIPTION("Sensor Monitoring System - Simulated Sensor Driver");
MODULE_VERSION("1.0");
