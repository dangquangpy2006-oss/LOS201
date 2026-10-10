/*
 * Circular Buffer Unit Test
 * Author: Nguyen Dang Quang - SE201072
 * Date: 10 October 2026
 */

#include <stdio.h>
#include "circular_buffer.h"

int main(void)
{
    CircularBuffer buffer;
    BufferStats stats;
    SensorData input = {1000, 25.5f, 60.0f};
    SensorData output;

    if (circular_buffer_init(&buffer, 2) != 0) {
        printf("FAIL: Buffer initialization\n");
        return 1;
    }

    /* Add the first sample */
    if (circular_buffer_push(&buffer, &input) != 0) {
        printf("FAIL: First push\n");
        circular_buffer_destroy(&buffer);
        return 1;
    }

    /* Add the second sample */
    input.timestamp_ms = 2000;
    input.temperature = 30.0f;
    input.humidity = 70.0f;

    if (circular_buffer_push(&buffer, &input) != 0) {
        printf("FAIL: Second push\n");
        circular_buffer_destroy(&buffer);
        return 1;
    }

    /* Verify buffer overflow */
    if (circular_buffer_push(&buffer, &input) == 0) {
        printf("FAIL: Overflow not detected\n");
        circular_buffer_destroy(&buffer);
        return 1;
    }

    /* Get the oldest sample */
    if (circular_buffer_pop(&buffer, &output) != 0) {
        printf("FAIL: Pop operation\n");
        circular_buffer_destroy(&buffer);
        return 1;
    }

    printf("First sample: %.1f C, %.1f %%\n",
           output.temperature, output.humidity);

    circular_buffer_get_stats(&buffer, &stats);

    printf("Current size: %zu\n", stats.current_size);
    printf("Total produced: %llu\n",
           (unsigned long long)stats.total_produced);
    printf("Total consumed: %llu\n",
           (unsigned long long)stats.total_consumed);
    printf("Overflow count: %llu\n",
           (unsigned long long)stats.overflow_count);

    circular_buffer_shutdown(&buffer);
    circular_buffer_destroy(&buffer);

    printf("TEST COMPLETED\n");

    return 0;
}

