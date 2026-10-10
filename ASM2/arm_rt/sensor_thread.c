#include <pthread.h>
#include <sched.h>
/*
 * Sensor Monitoring System - Sensor Thread
 *
 * Purpose: Read data from the kernel sensor driver.
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 * Date: 10 October 2026
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>

#include "sensor_thread.h"
#include "../driver/sms_sensor.h"
/*
 * Add milliseconds to an absolute time.
 */
static void add_ms_to_timespec(struct timespec *ts,
                               unsigned int ms)
{
    ts->tv_sec += ms / 1000;
    ts->tv_nsec += (long)(ms % 1000) * 1000000L;

    if (ts->tv_nsec >= 1000000000L) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}
/*
 * Parse a CSV sensor sample.
 *
 * Expected format:
 * timestamp_ms,temperature,humidity
 *
 * Return:
 *  0  = success
 * -1  = invalid data
 */
static int parse_sensor_csv(const char *csv, SensorData *sample)
{
    unsigned long long timestamp;
    float temperature;
    float humidity;
    char extra;

    if (csv == NULL || sample == NULL)
        return -1;

    int matched = sscanf(csv, "%llu,%f,%f %c",
                         &timestamp,
                         &temperature,
                         &humidity,
                         &extra);

    if (matched != 3)
        return -1;

    if (temperature < 15.0f || temperature > 45.0f)
        return -1;

    if (humidity < 30.0f || humidity > 90.0f)
        return -1;

    sample->timestamp_ms = (uint64_t)timestamp;
    sample->temperature = temperature;
    sample->humidity = humidity;

    return 0;
}
/*
 * Main function executed by the Sensor Thread.
 */
void *sensor_thread_main(void *arg)
{
    SensorThreadConfig *config = (SensorThreadConfig *)arg;
    const char *device_path;
    unsigned int interval_ms;
    int fd;
    struct timespec next_wakeup;
    char csv_buffer[128];
    if (config == NULL ||
        config->buffer == NULL ||
        config->stop_requested == NULL)
        return NULL;

    device_path = config->device_path;
    interval_ms = config->interval_ms;

    if (device_path == NULL)
        device_path = SMS_DEVICE_PATH;

    if (interval_ms == 0)
        interval_ms = 500;

    /* Open the kernel sensor device */
    fd = open(device_path, O_RDONLY);

    if (fd < 0) {
        perror("sensor_thread: open");
        return NULL;
    }

    /* Configure the driver's minimum reading interval */
    if (ioctl(fd, SMS_SET_INTERVAL, &interval_ms) < 0) {
        perror("sensor_thread: ioctl");
        close(fd);
        return NULL;
    }

    {
        const char *mode = getenv("SMS_RT_MODE");
        int policy = SCHED_OTHER;
        int actual_policy = -1;
        struct sched_param param = {0};
        struct sched_param actual_param = {0};

        if (mode != NULL && strcmp(mode, "FIFO") == 0) {
            policy = SCHED_FIFO;
            param.sched_priority = 50;
        }

        int rc = pthread_setschedparam(
            pthread_self(), policy, &param
        );

        if (rc != 0) {
            fprintf(stderr,
                    "[RT] pthread_setschedparam failed: %s\n",
                    strerror(rc));
            close(fd);
            return NULL;
        }

        rc = pthread_getschedparam(
            pthread_self(), &actual_policy, &actual_param
        );

        if (rc != 0) {
            fprintf(stderr,
                    "[RT] pthread_getschedparam failed: %s\n",
                    strerror(rc));
            close(fd);
            return NULL;
        }

        printf("[RT] Requested=%s Actual=%s Priority=%d\n",
               policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_OTHER",
               actual_policy == SCHED_FIFO
                   ? "SCHED_FIFO" : "SCHED_OTHER",
               actual_param.sched_priority);
        fflush(stdout);
    }

    printf("[SENSOR] Device opened: %s\n", device_path);
    printf("[SENSOR] Sampling interval: %u ms\n", interval_ms);
 if (clock_gettime(CLOCK_MONOTONIC, &next_wakeup) != 0) {
        perror("sensor_thread: clock_gettime");
    close(fd);

    return NULL;
}
while (!atomic_load(config->stop_requested)) {
        ssize_t bytes_read;
        SensorData sample;

        bytes_read = read(fd, csv_buffer, sizeof(csv_buffer) - 1);

        if (bytes_read > 0) {
            csv_buffer[bytes_read] = '\0';

            if (parse_sensor_csv(csv_buffer, &sample) == 0) {
                if (circular_buffer_push(config->buffer, &sample) == 0) {
                    if (config->log_queue != NULL) {
                        char log_message[LOG_MESSAGE_SIZE];

                        snprintf(
                            log_message,
                            sizeof(log_message),
                            "[SENSOR] Time=%llu ms | Temp=%.1f C | Hum=%.1f%%",
                            (unsigned long long)sample.timestamp_ms,
                            sample.temperature,
                            sample.humidity
                        );

                        if (log_queue_push(
                                config->log_queue,
                                log_message) != 0) {
                            fprintf(stderr,
                                    "[SENSOR] Log queue full or stopped\\n");
                        }
                    }

                    printf("[SENSOR] Time=%llu ms | Temp=%.1f C | Hum=%.1f%%\n",
                           (unsigned long long)sample.timestamp_ms,
                           sample.temperature,
                           sample.humidity);
                } else {
                    fprintf(stderr, "[SENSOR] Buffer full or shutting down\n");
                }
            } else {
                fprintf(stderr, "[SENSOR] Invalid CSV: %s\n", csv_buffer);
            }
        } else if (bytes_read < 0) {
            if (errno != EAGAIN && errno != EINTR) {
                perror("sensor_thread: read");
                break;
            }
        }

        add_ms_to_timespec(&next_wakeup, interval_ms);

        int sleep_result;
        do {
            
        {
            struct timespec rt_expected = next_wakeup;
            struct timespec rt_actual;

            sleep_result = clock_nanosleep(
                CLOCK_MONOTONIC,
                TIMER_ABSTIME,
                &next_wakeup,
                NULL
            );

            if (sleep_result == 0 &&
                clock_gettime(CLOCK_MONOTONIC, &rt_actual) == 0) {

                long long jitter_us =
                    (long long)(rt_actual.tv_sec -
                                rt_expected.tv_sec) * 1000000LL +
                    (long long)(rt_actual.tv_nsec -
                                rt_expected.tv_nsec) / 1000LL;

                printf("RT_JITTER_US=%lld\n", jitter_us);
                fflush(stdout);
            }
        }

        } while (sleep_result == EINTR &&
                 !atomic_load(config->stop_requested));

        if (sleep_result != 0 && sleep_result != EINTR) {
            errno = sleep_result;
            perror("sensor_thread: clock_nanosleep");
            break;
        }
    }

    close(fd);

    printf("[SENSOR] Thread stopped\n");

    return NULL;
}
