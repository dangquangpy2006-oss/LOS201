/*
 * Circular Buffer - Thread-safe sensor data storage
 * Author: Nguyen Dang Quang - SE201072
 * Date: 10 October 2026
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "sensor_data.h"

typedef struct {
    SensorData *data;

    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;

    uint64_t total_produced;
    uint64_t total_consumed;
    uint64_t overflow_count;

    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} CircularBuffer;

typedef struct {
    size_t current_size;

    uint64_t total_produced;
    uint64_t total_consumed;
    uint64_t overflow_count;
} BufferStats;

/* Initialize and allocate buffer memory */
int circular_buffer_init(CircularBuffer *buffer, size_t capacity);

/* Add a new sensor sample */
int circular_buffer_push(CircularBuffer *buffer,
                         const SensorData *sample);

/* Take a sensor sample, waiting when buffer is empty */
int circular_buffer_pop(CircularBuffer *buffer,
                        SensorData *sample);

/* Read buffer statistics safely */
void circular_buffer_get_stats(CircularBuffer *buffer,
                               BufferStats *stats);

/* Wake waiting threads during shutdown */
void circular_buffer_shutdown(CircularBuffer *buffer);

/* Release allocated resources */
void circular_buffer_destroy(CircularBuffer *buffer);

#endif
