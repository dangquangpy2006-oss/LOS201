/*
 * Sensor Monitoring System - Alert Thread
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 *
 * Purpose:
 * Receive and display alerts from POSIX Message Queue.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include <mqueue.h>
#include <time.h>

#include "alert_thread.h"
#include "sensor_data.h"

/*
 * Convert alert type into a readable description.
 */
static const char *alert_type_to_string(AlertType type)
{
    switch (type) {
        case TEMP_HIGH:
            return "HIGH TEMPERATURE";

        case HUMID_HIGH:
            return "HIGH HUMIDITY";

        case TEMP_SPIKE:
            return "TEMPERATURE SPIKE";

        default:
            return "UNKNOWN ALERT";
    }
}

/*
 * Display one alert message.
 */
static void display_alert(const AlertMessage *message)
{
    if (message == NULL)
        return;

    printf("\n[ALERT] %s\n",
           alert_type_to_string(message->type));

    printf("[ALERT] Timestamp: %llu ms\n",
           (unsigned long long)message->timestamp_ms);

    printf("[ALERT] Value: %.1f | Threshold: %.1f\n",
           message->value,
           message->threshold);
}
/*
 * Main function executed by the Alert Thread.
 */
void *alert_thread_main(void *arg)
{
    AlertThreadConfig *config = (AlertThreadConfig *)arg;
    AlertMessage message;

    unsigned long long alert_count = 0;

    if (config == NULL || config->stop_requested == NULL)
        return NULL;

    printf("[ALERT] Thread started\n");

    while (1) {
        struct timespec timeout;

        if (clock_gettime(CLOCK_REALTIME, &timeout) != 0) {
            perror("[ALERT] clock_gettime");
            break;
        }

        /* Wait for at most 200 milliseconds */
        timeout.tv_nsec += 200000000L;

        if (timeout.tv_nsec >= 1000000000L) {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000L;
        }

        ssize_t received = mq_timedreceive(
            config->alert_queue,
            (char *)&message,
            sizeof(message),
            NULL,
            &timeout
        );

        if (received >= 0) {
            if ((size_t)received != sizeof(message)) {
                fprintf(stderr, "[ALERT] Invalid message size\n");
                continue;
            }

            display_alert(&message);

            if (config->log_queue != NULL) {
                char log_message[LOG_MESSAGE_SIZE];

                snprintf(
                    log_message,
                    sizeof(log_message),
                    "[ALERT] %s | Time=%llu ms | Value=%.1f | Threshold=%.1f",
                    alert_type_to_string(message.type),
                    (unsigned long long)message.timestamp_ms,
                    message.value,
                    message.threshold
                );

                if (log_queue_push(
                        config->log_queue,
                        log_message) != 0) {
                    fprintf(stderr,
                            "[ALERT] Log queue full or stopped\\n");
                }
            }

            alert_count++;
            continue;
        }

        if (errno == EINTR)
            continue;

        if (errno == ETIMEDOUT || errno == EAGAIN) {
            if (atomic_load(config->stop_requested))
                break;

            continue;
        }

        perror("[ALERT] mq_timedreceive");
        break;
    }

    printf("[ALERT] Thread stopped. "
           "Total alerts: %llu\n",
           alert_count);

    return NULL;
}
