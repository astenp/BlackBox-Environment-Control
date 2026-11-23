#ifndef DARKBOX_PROTOCOL_H
#define DARKBOX_PROTOCOL_H

#include <stdbool.h>

/*
 * This is an internal measurement model, not the final wire protocol.
 * The parser currently accepts the legacy Pico diagnostic line:
 *     h: 45.0  t: 23.5
 */
typedef struct {
    double humidity_percent;
    double temperature_celsius;
} darkbox_measurement;

typedef enum {
    DARKBOX_PROTOCOL_OK = 0,
    DARKBOX_PROTOCOL_INVALID_ARGUMENT,
    DARKBOX_PROTOCOL_INVALID_MESSAGE
} darkbox_protocol_result;

darkbox_protocol_result darkbox_protocol_parse_line(
    const char *line,
    darkbox_measurement *measurement);

#endif
