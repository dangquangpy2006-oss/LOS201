/*
 * Sensor Monitoring System - Processor Thread
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 *
 * Purpose:
 * Process sensor samples and detect alerts.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <mqueue.h>

#include "processor_thread.h"
#include "circular_buffer.h"
#include "sensor_data.h"
/*
 * Store the latest 10 temperature samples.
 */
typedef struct {
    float values[ROLLING_WINDOW_SIZE];

    size_t count;
    size_t next_index;

    float sum;
} RollingAverage;
/*
 * Initialize a rolling average window.
 */
static void rolling_average_init(RollingAverage *avg)
{
    if (avg == NULL)
        return;

    memset(avg, 0, sizeof(*avg));
}
/*
 * Add one temperature and return the rolling average.
 */
static float rolling_average_add(RollingAverage *avg,
                                 float temperature)
{
    if (avg == NULL)
        return 0.0f;

    /* Remove the oldest value when the window is full */
    if (avg->count == ROLLING_WINDOW_SIZE) {
        avg->sum -= avg->values[avg->next_index];
    } else {
        avg->count++;
    }

    /* Store the new temperature */
    avg->values[avg->next_index] = temperature;

    avg->sum += temperature;

    /* Move to the next position */
    avg->next_index =
        (avg->next_index + 1) % ROLLING_WINDOW_SIZE;

    return avg->sum / (float)avg->count;
}
/*
 * Send an alert to the POSIX message queue.
 *
 * Return:
 *  0  = success
 * -1  = sending failed
 */
static int send_alert(mqd_t queue,
                      uint64_t timestamp_ms,
                      AlertType type,
                      float value,
                      float threshold)
{
    AlertMessage message;

    memset(&message, 0, sizeof(message));

    message.timestamp_ms = timestamp_ms;
    message.type = type;
    message.value = value;
    message.threshold = threshold;

    if (mq_send(queue,
                (const char *)&message,
                sizeof(message),
                0) == -1) {
        perror("[PROCESSOR] mq_send");
        return -1;
    }

    return 0;
}
/*
 * Check temperature, humidity and sudden temperature rise.
 *
 * A sample may trigger more than one alert.
 */
static void check_sensor_alerts(const SensorData *sample,
                                float previous_temp,
                                bool has_previous,
                                mqd_t alert_queue)
{
    if (sample == NULL)
        return;

    /* High temperature alert */
    if (sample->temperature > TEMP_ALERT_THRESHOLD) {
        send_alert(alert_queue,
                   sample->timestamp_ms,
                   TEMP_HIGH,
                   sample->temperature,
                   TEMP_ALERT_THRESHOLD);
    }

    /* High humidity alert */
    if (sample->humidity > HUMID_ALERT_THRESHOLD) {
        send_alert(alert_queue,
                   sample->timestamp_ms,
                   HUMID_HIGH,
                   sample->humidity,
                   HUMID_ALERT_THRESHOLD);
    }

    /* Sudden temperature rise alert */
    if (has_previous &&
        (sample->temperature - previous_temp) >
        TEMP_SPIKE_THRESHOLD) {

        send_alert(alert_queue,
                   sample->timestamp_ms,
                   TEMP_SPIKE,
                   sample->temperature - previous_temp,
                   TEMP_SPIKE_THRESHOLD);
    }
}
/*
 * Main function executed by the Processor Thread.
 */
void *processor_thread_main(void *arg)
{
    ProcessorThreadConfig *config = (ProcessorThreadConfig *)arg;

    RollingAverage avg;
    SensorData sample;

    float previous_temp = 0.0f;
    bool has_previous = false;

    unsigned long long processed_count = 0;

    if (config == NULL ||
        config->buffer == NULL ||
        config->stop_requested == NULL) {
        return NULL;
    }

    rolling_average_init(&avg);

    printf("[PROCESSOR] Thread started\n");

    while (circular_buffer_pop(config->buffer, &sample) == 0) {
        if (config->log_queue != NULL) {
            char log_message[LOG_MESSAGE_SIZE];

            snprintf(
                log_message,
                sizeof(log_message),
                "[PROCESSOR] Time=%llu ms | Temp=%.1f C | Hum=%.1f%%",
                (unsigned long long)sample.timestamp_ms,
                sample.temperature,
                sample.humidity
            );

            if (log_queue_push(
                    config->log_queue,
                    log_message) != 0) {
                fprintf(stderr,
                        "[PROCESSOR] Log queue full or stopped\\n");
            }
        }


        float average_temp =
            rolling_average_add(&avg, sample.temperature);

        processed_count++;

        printf("[PROCESSOR] Sample=%llu | "
               "Temp=%.1f C | Hum=%.1f%% | "
               "Avg(10)=%.2f C\n",
               processed_count,
               sample.temperature,
               sample.humidity,
               average_temp);

        check_sensor_alerts(&sample,
                            previous_temp,
                            has_previous,
                            config->alert_queue);

        previous_temp = sample.temperature;
        has_previous = true;
    }

    printf("[PROCESSOR] Thread stopped. "
           "Total processed: %llu\n",
           processed_count);

    return NULL;
}
