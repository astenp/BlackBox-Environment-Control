#define _DEFAULT_SOURCE

#include "darkbox/transport.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int configure_serial_port(int file_descriptor)
{
    struct termios settings;

    if (tcgetattr(file_descriptor, &settings) != 0) {
        return -1;
    }

    cfmakeraw(&settings);
    cfsetispeed(&settings, B115200);
    cfsetospeed(&settings, B115200);
    settings.c_cflag |= CLOCAL | CREAD;

    return tcsetattr(file_descriptor, TCSANOW, &settings);
}

int darkbox_transport_open(darkbox_transport *transport, const char *device_path)
{
    if (transport == NULL) {
        errno = EINVAL;
        return -1;
    }

    transport->file_descriptor = -1;
    transport->mock_mode = 0;
    transport->mock_sequence = 0;

    if (device_path == NULL) {
        transport->mock_mode = 1;
        return 0;
    }

    transport->file_descriptor = open(device_path, O_RDONLY | O_NOCTTY);
    if (transport->file_descriptor < 0) {
        return -1;
    }

    if (configure_serial_port(transport->file_descriptor) != 0) {
        close(transport->file_descriptor);
        transport->file_descriptor = -1;
        return -1;
    }

    return 0;
}

int darkbox_transport_read_line(
    darkbox_transport *transport,
    char *buffer,
    size_t buffer_size)
{
    size_t position = 0;

    if (transport == NULL || buffer == NULL || buffer_size < 2) {
        errno = EINVAL;
        return -1;
    }

    if (transport->mock_mode) {
        int written = snprintf(
            buffer,
            buffer_size,
            "h: %.1f  t: %.1f\n",
            45.0 + (double)(transport->mock_sequence % 5),
            23.0 + (double)(transport->mock_sequence % 3));
        transport->mock_sequence++;
        return written > 0 && (size_t)written < buffer_size ? 1 : -1;
    }

    while (position + 1 < buffer_size) {
        char character;
        ssize_t result = read(transport->file_descriptor, &character, 1);

        if (result == 0) {
            return position == 0 ? 0 : -1;
        }
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (character == '\n') {
            break;
        }
        if (character != '\r') {
            buffer[position++] = character;
        }
    }

    buffer[position] = '\0';
    return position > 0 ? 1 : -1;
}

void darkbox_transport_close(darkbox_transport *transport)
{
    if (transport == NULL) {
        return;
    }

    if (transport->file_descriptor >= 0) {
        close(transport->file_descriptor);
        transport->file_descriptor = -1;
    }
}
