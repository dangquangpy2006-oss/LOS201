/*
 * Sensor Thread Integration Test
 * Student ID: SE201072
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <time.h>

#include "sensor_thread.h"
#include "circular_buffer.h"

int main(void)
{
    CircularBuffer buffer;
    SensorThreadConfig config;
    BufferStats stats;
    pthread_t sensor_thread;

    atomic_bool stop_requested = ATOMIC_VAR_INIT(false);

    if (circular_buffer_init(&buffer, 64) != 0) {
        fprintf(stderr, "Failed to initialize buffer\n");
        return 1;
    }

    config.buffer = &buffer;
    config.interval_ms = 500;
    config.device_path = "/dev/sms_sensor";
    config.stop_requested = &stop_requested;

    if (pthread_create(&sensor_thread, NULL,
                       sensor_thread_main, &config) != 0) {
        fprintf(stderr, "Failed to create Sensor Thread\n");
        circular_buffer_destroy(&buffer);
        return 1;
    }

    printf("[TEST] Sensor Thread started\n");

    struct timespec duration = {5, 0};
    nanosleep(&duration, NULL);

    atomic_store(&stop_requested, true);

    pthread_join(sensor_thread, NULL);

    circular_buffer_get_stats(&buffer, &stats);

    printf("\n[TEST] Final statistics:\n");
    printf("Produced: %llu\n",
           (unsigned long long)stats.total_produced);
    printf("Consumed: %llu\n",
           (unsigned long long)stats.total_consumed);
    printf("Buffer size: %zu\n", stats.current_size);
    printf("Overflow count: %llu\n",
           (unsigned long long)stats.overflow_count);

    circular_buffer_shutdown(&buffer);
    circular_buffer_destroy(&buffer);

    printf("[TEST] Completed\n");

    return 0;
}
