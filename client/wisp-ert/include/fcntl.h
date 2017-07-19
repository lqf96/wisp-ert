#pragma once

//Open for reading only
extern const int O_RDONLY;
//Open for writing only
extern const int O_WRONLY;
//Open for reading and writing
extern const int O_RDWR;
//Append on each write
extern const int O_APPEND;
//Create file if it does not exist
extern const int O_CREAT;

/**
 * Open a file.
 *
 * @param path File path
 * @param flags Open flags
 * @return File descriptor on success, or -1 on failure
 */
extern int open(const char* path, int flags, ...);
