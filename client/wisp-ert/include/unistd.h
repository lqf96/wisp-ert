#pragma once

#include <stddef.h>
#include <stdint.h>

typedef int16_t ssize_t;

/**
 * Close a file.
 *
 * @param fd File descriptor
 * @return 0 on success, or -1 on failure
 */
extern int close(int fd);

/**
 * Read data from file.
 *
 * @param fd File descriptor
 * @param buf Read data
 * @param size Data size
 * @return Bytes of data read, or -1 on failure
 */
extern ssize_t read(int fd, void* buf, size_t size);

/**
 * Write data to file.
 *
 * @param fd File descriptor
 * @param buf Write data
 * @param size Data size
 * @return Bytes of data written, or -1 on failure
 */
extern ssize_t write(int fd, void* buf, size_t size);
