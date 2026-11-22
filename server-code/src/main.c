#include "darkbox/protocol.h"
#include "darkbox/transport.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [--mock] [--device PATH] [--once]\n", program_name);
}

int main(int argc, char **argv)
{
    const char *device_path = NULL;
    int read_once = 0;
    darkbox_transport transport;
    char line[256];

    for (int argument = 1; argument < argc; ++argument) {
        if (strcmp(argv[argument], "--mock") == 0) {
            device_path = NULL;
        } else if (strcmp(argv[argument], "--once") == 0) {
            read_once = 1;
        } else if (strcmp(argv[argument], "--device") == 0 && argument + 1 < argc) {
            device_path = argv[++argument];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }

    if (darkbox_transport_open(&transport, device_path) != 0) {
        perror("Unable to open Pico transport");
        return 1;
    }

    fprintf(stderr, "Darkbox server skeleton started (%s transport).\n",
            device_path == NULL ? "mock" : "serial");

    for (;;) {
        darkbox_measurement measurement;
        int result = darkbox_transport_read_line(&transport, line, sizeof(line));

        if (result == 0) {
            break;
        }
        if (result < 0) {
            fprintf(stderr, "Transport read failed.\n");
            darkbox_transport_close(&transport);
            return 1;
        }

        if (darkbox_protocol_parse_line(line, &measurement) != DARKBOX_PROTOCOL_OK) {
            fprintf(stderr, "Ignoring invalid Pico frame: %s\n", line);
        } else {
            printf("measurement humidity=%.3f%% temperature=%.3fC\n",
                   measurement.humidity_percent,
                   measurement.temperature_celsius);
            fflush(stdout);
        }

        if (read_once) {
            break;
        }
    }

    darkbox_transport_close(&transport);
    return 0;
}
