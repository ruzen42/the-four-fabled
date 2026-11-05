/* SPDX-License-Identifier: AGPL-3.0-only 
 * 4/11/2026 ruzen42 */
#pragma once

#include <stddef.h>
#include <string.h>
#include <unistd.h>

static inline void
ClientLog(const char *message)
{
    if (!message) return;
    size_t length = strlen(message);
    while (length > 0) {
        ssize_t written = write(STDERR_FILENO, message, length);
        if (written <= 0) return;
        message += written;
        length -= (size_t)written;
    }
    (void)write(STDERR_FILENO, "\n", 1);
}
