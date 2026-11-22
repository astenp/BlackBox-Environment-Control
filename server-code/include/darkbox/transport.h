#ifndef DARKBOX_TRANSPORT_H
#define DARKBOX_TRANSPORT_H

#include <stddef.h>

typedef struct {
    int file_descriptor;
    int mock_mode;
    unsigned int mock_sequence;
} darkbox_transport;

/* Pass NULL as device_path to use the deterministic development mock. */
int darkbox_transport_open(darkbox_transport *transport, const char *device_path);

/* Reads one newline-terminated frame. Returns 1, 0 on EOF, or -1 on error. */
int darkbox_transport_read_line(
    darkbox_transport *transport,
    char *buffer,
    size_t buffer_size);

void darkbox_transport_close(darkbox_transport *transport);

#endif
