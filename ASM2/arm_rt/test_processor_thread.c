/*
 * Processor Thread Integration Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "processor_thread.h"
#include "circular_buffer.h"
#include "sensor_data.h"

/*
 * Fixed test samples.
 *
 * Expected alerts:
 * TEMP_HIGH, HUMID_HIGH and TEMP_SPIKE.
 */
static const SensorData test_samples[] = {
    {1000, 30.0f, 60.0f},
    {2000, 32.0f, 65.0f},
    {3000, 42.0f, 85.0f},
    {4000, 35.0f, 70.0f},
    {5000, 41.0f, 82.0f}
};
/*
 * Run Processor Thread integration test.
 */
int main(void)
{
    const char *queue_name = "/sms_processor_test";

    CircularBuffer buffer;
    ProcessorThreadConfig config;
    BufferStats stats;
    pthread_t processor_thread;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(AlertMessage);

    mq_unlink(queue_name);

    mqd_t queue = mq_open(
        queue_name,
        O_CREAT | O_RDWR | O_NONBLOCK,
        0600,
        &attr
    );

    if (queue == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    if (circular_buffer_init(&buffer, 64) != 0) {
        fprintf(stderr, "Buffer initialization failed\n");
        mq_close(queue);
        mq_unlink(queue_name);
        return 1;
    }

    config.buffer = &buffer;
    config.alert_queue = queue;
    config.stop_requested = &stop_requested;

    if (pthread_create(&processor_thread, NULL,
                       processor_thread_main, &config) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        circular_buffer_destroy(&buffer);
        mq_close(queue);
        mq_unlink(queue_name);
        return 1;
    }

    printf("[TEST] Processor Thread started\n");

    /* Push all five test samples */
    size_t sample_count =
        sizeof(test_samples) / sizeof(test_samples[0]);

    for (size_t i = 0; i < sample_count; i++) {
        if (circular_buffer_push(&buffer, &test_samples[i]) != 0) {
            fprintf(stderr, "Failed to push sample %zu\n", i);
        }
    }

    /* Stop after queued samples have been processed */
    atomic_store(&stop_requested, true);
    circular_buffer_shutdown(&buffer);

    pthread_join(processor_thread, NULL);

    circular_buffer_get_stats(&buffer, &stats);

    printf("\n[TEST] Buffer statistics:\n");
    printf("Produced: %llu\n",
           (unsigned long long)stats.total_produced);
    printf("Consumed: %llu\n",
           (unsigned long long)stats.total_consumed);
    printf("Buffer size: %zu\n", stats.current_size);

    printf("\n[TEST] Received alerts:\n");

    AlertMessage message;
    unsigned int alert_count = 0;

    while (mq_receive(queue,
                      (char *)&message,
                      sizeof(message),
                      NULL) >= 0) {

        printf("Type=%d | Value=%.1f | Threshold=%.1f\n",
               message.type,
               message.value,
               message.threshold);

        alert_count++;
    }

    if (errno != EAGAIN) {
        perror("mq_receive");
    }

    printf("Total alerts: %u\n", alert_count);

    int test_passed =
        (stats.total_produced == 5 &&
         stats.total_consumed == 5 &&
         stats.current_size == 0 &&
         alert_count == 6);

    circular_buffer_destroy(&buffer);
    mq_close(queue);
    mq_unlink(queue_name);

    if (test_passed) {
        printf("[TEST] PASSED\n");
        return 0;
    }

    printf("[TEST] FAILED\n");
    return 1;
}
