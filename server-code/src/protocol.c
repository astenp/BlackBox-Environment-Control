#include "darkbox/protocol.h"

#include <stdio.h>

darkbox_protocol_result darkbox_protocol_parse_line(
    const char *line,
    darkbox_measurement *measurement)
{
    double humidity = 0.0;
    double temperature = 0.0;

    if (line == NULL || measurement == NULL) {
        return DARKBOX_PROTOCOL_INVALID_ARGUMENT;
    }

    if (sscanf(line, " h: %lf t: %lf", &humidity, &temperature) != 2) {
        return DARKBOX_PROTOCOL_INVALID_MESSAGE;
    }

    measurement->humidity_percent = humidity;
    measurement->temperature_celsius = temperature;
    return DARKBOX_PROTOCOL_OK;
}
