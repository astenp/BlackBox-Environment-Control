#include "darkbox/protocol.h"

#include <assert.h>
#include <stddef.h>

int main(void)
{
    darkbox_measurement measurement;

    assert(darkbox_protocol_parse_line(
               "h: 42.5  t: 21.75",
               &measurement) == DARKBOX_PROTOCOL_OK);
    assert(measurement.humidity_percent == 42.5);
    assert(measurement.temperature_celsius == 21.75);

    assert(darkbox_protocol_parse_line("invalid", &measurement) ==
           DARKBOX_PROTOCOL_INVALID_MESSAGE);
    assert(darkbox_protocol_parse_line(NULL, &measurement) ==
           DARKBOX_PROTOCOL_INVALID_ARGUMENT);

    return 0;
}
