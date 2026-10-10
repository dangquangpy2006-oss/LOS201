#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

/*
 * Sensor Monitoring System
 * Shared ioctl definitions
 *
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 * Date: 10 October 2026
 */

/* ioctl magic number */
#define SMS_IOC_MAGIC 'S'

/*
 * Configure the minimum interval between
 * two sensor readings (milliseconds).
 */
#define SMS_SET_INTERVAL _IOW(SMS_IOC_MAGIC, 1, unsigned int)

/* Default interval */
#define SMS_DEFAULT_INTERVAL_MS 100

/* Sensor device and proc paths */
#define SMS_DEVICE_PATH "/dev/sms_sensor"
#define SMS_PROC_PATH   "/proc/sms_stats"

#endif

