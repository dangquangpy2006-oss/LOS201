#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>

/*
 * Sensor Monitoring System
 * Shared sensor data structures
 *
 * Author: Nguyen Dang Quang
 * Student ID: SE201072
 * Date: 10 October 2026
 */

#define SENSOR_BUFFER_CAPACITY 64
#define ROLLING_WINDOW_SIZE 10

typedef struct {
    uint64_t timestamp_ms;
    float temperature;
    float humidity;
} SensorData;

typedef enum {
    TEMP_HIGH = 1,
    HUMID_HIGH,
    TEMP_SPIKE
} AlertType;

typedef struct {
    uint64_t timestamp_ms;
    AlertType type;
    float value;
    float threshold;
} AlertMessage;

#endif

