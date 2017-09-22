#pragma once

#include <ert/rpc.h>

/// Open file for reading only
#define O_RDONLY ERT_CONST(O_RDONLY)
/// Open file for writing only
#define O_WRONLY ERT_CONST(O_WRONLY)
/// Open file for reading and writing
#define O_RDWR ERT_CONST(O_RDWR)
/// Append on each write
#define O_APPEND ERT_CONST(O_APPEND)
/// Create file if it does not exist
#define O_CREAT ERT_CONST(O_CREAT)
