/*
 * Circular Buffer Implementation
 *
 * Purpose: Thread-safe storage for sensor samples.
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 * Date: 10 October 2026
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "circular_buffer.h"

/*
 * Initialize the circular buffer.
 *
 * Return:
 *  0  = success
 * -1  = failure
 */
int circular_buffer_init(CircularBuffer *buffer, size_t capacity)
{
    if (buffer == NULL || capacity == 0)
        return -1;

    /* Initialize all fields to zero */
    memset(buffer, 0, sizeof(*buffer));

    /* Allocate memory for sensor samples */
    buffer->data = calloc(capacity, sizeof(SensorData));

    if (buffer->data == NULL)
        return -1;

    buffer->capacity = capacity;

    /* Initialize mutex */
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        free(buffer->data);
        buffer->data = NULL;
        return -1;
    }

    /* Initialize condition variable */
    if (pthread_cond_init(&buffer->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->data);
        buffer->data = NULL;
        return -1;
    }

    return 0;
}
/*
 * Add a sensor sample to the circular buffer.
 *
 * Return:
 *  0  = success
 * -1  = buffer full or shutdown
 */
int circular_buffer_push(CircularBuffer *buffer,
                         const SensorData *sample)
{
    if (buffer == NULL || sample == NULL)
        return -1;

    pthread_mutex_lock(&buffer->mutex);

    /* Reject new samples during shutdown */
    if (buffer->shutdown) {
        pthread_mutex_unlock(&buffer->mutex);
        return -1;
    }

    /* Check whether buffer is full */
    if (buffer->size == buffer->capacity) {
        buffer->overflow_count++;

        pthread_mutex_unlock(&buffer->mutex);
        return -1;
    }

    /* Insert sample at tail position */
    buffer->data[buffer->tail] = *sample;

    /* Move tail forward and wrap around */
    buffer->tail = (buffer->tail + 1) % buffer->capacity;

    buffer->size++;
    buffer->total_produced++;

    /* Wake up Processor Thread */
    pthread_cond_signal(&buffer->not_empty);

    pthread_mutex_unlock(&buffer->mutex);

    return 0;
}
/*
 * Take a sensor sample from the circular buffer.
 *
 * Return:
 *  0  = success
 * -1  = buffer shutdown or invalid arguments
 */
int circular_buffer_pop(CircularBuffer *buffer,
                        SensorData *sample)
{
    if (buffer == NULL || sample == NULL)
        return -1;

    pthread_mutex_lock(&buffer->mutex);

    /* Wait while buffer is empty */
    while (buffer->size == 0 && !buffer->shutdown) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    /* Stop if shutdown and no data remains */
    if (buffer->size == 0 && buffer->shutdown) {
        pthread_mutex_unlock(&buffer->mutex);
        return -1;
    }

    /* Read sample at head position */
    *sample = buffer->data[buffer->head];

    /* Move head forward and wrap around */
    buffer->head = (buffer->head + 1) % buffer->capacity;

    buffer->size--;
    buffer->total_consumed++;

    pthread_mutex_unlock(&buffer->mutex);

    return 0;
}
/*
 * Get circular buffer statistics safely.
 */
void circular_buffer_get_stats(CircularBuffer *buffer,
                               BufferStats *stats)
{
    if (buffer == NULL || stats == NULL)
        return;

    pthread_mutex_lock(&buffer->mutex);

    stats->current_size = buffer->size;
    stats->total_produced = buffer->total_produced;
    stats->total_consumed = buffer->total_consumed;
    stats->overflow_count = buffer->overflow_count;

    pthread_mutex_unlock(&buffer->mutex);
}
/*
 * Wake up waiting threads during shutdown.
 */
void circular_buffer_shutdown(CircularBuffer *buffer)
{
    if (buffer == NULL)
        return;

    pthread_mutex_lock(&buffer->mutex);

    buffer->shutdown = 1;

    pthread_cond_broadcast(&buffer->not_empty);

    pthread_mutex_unlock(&buffer->mutex);
}

/*
 * Release circular buffer resources.
 *
 * Call only after all worker threads have stopped.
 */
void circular_buffer_destroy(CircularBuffer *buffer)
{
    if (buffer == NULL)
        return;

    pthread_cond_destroy(&buffer->not_empty);
    pthread_mutex_destroy(&buffer->mutex);

    free(buffer->data);

    buffer->data = NULL;
    buffer->capacity = 0;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->size = 0;
}
