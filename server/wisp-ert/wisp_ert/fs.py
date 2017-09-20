from __future__ import absolute_import, unicode_literals
import os, errno, functools, logging
from collections import OrderedDict
from urpc import StringType, urpc_sig, U16, I16, VARY
from urpc.util import AllocTable

from wisp_ert.runtime import Service

## Module logger
_logger = logging.getLogger(__name__)
# Logger level
_logger.setLevel(logging.DEBUG)

def _proxy_sys(name):
    """!
    @brief Wrap system call function as LocalFS service function.

    @param name System call function name.
    @return LocalFS service wrapper.
    """
    sys_func = getattr(os, name)
    # Proxy function
    def proxy(self, fd, *args):
        """!
        @brief LocalFS system call proxy function.

        @param fd LocalFS virtual file descriptor.
        @param args System call arguments.
        @return Non-negative number on succeed, or negative error number on failure.
        """
        _logger.debug("Proxying %s system call", name)
        # Get and check file descriptor validity
        real_fd = self._fd_mapping.get(fd)
        if not real_fd:
            return -1*errno.EBADF
        # Call system function
        try:
            result = sys_func(real_fd, *args)
            return 0 if result==None else result
        except OSError as e:
            return -1*e.errno
    return proxy

class LocalFS(Service):
    """!
    @brief Local file system service class.
    """
    # Constructor
    def __init__(self, root_dir="/"):
        """!
        @brief Local file system service constructor.

        @param root_dir Local file system root.
        """
        ## Local file system root
        self._root_dir = root_dir
        ## File descriptor mapping
        self._fd_mapping = AllocTable(
            capacity=32
        )
    # Open file
    @urpc_sig([StringType, I16, U16], [I16])
    def open(self, path, flags, mode=0o666):
        """!
        @brief Open a file with given flags and mode.

        @param path Path of the file.
        @param flags Open flags.
        @param mode File mode when a new file is going to be created.
        @return File descriptor on success, or negative error number on failure.
        """
        # Try to open the file
        try:
            # Get real file path
            real_path = os.path.join(self._root_dir, path)
            _logger.debug("Opening file %s", real_path)
            # Open file
            real_fd = os.open(real_path, flags, mode)
        except OSError as e:
            return -1*e.errno
        # Add file descriptor to table
        return self._fd_mapping.add(real_fd)
    # Close file
    @urpc_sig([I16], [I16])
    def close(self, fd):
        """!
        @brief Close a file.

        @param fd LocalFS virtual file descriptor.
        @return 0 on success, or negative error number on failure.
        """
        # Try to close the file first
        result = self._close(fd)
        if result:
            return result
        # Remove file descriptor from table
        del self._fd_mapping[fd]
        return 0
    # Read file
    @urpc_sig([I16, U16], [I16, VARY])
    def read(self, fd, size):
        """!
        @brief Read given size of data from file.

        @param fd LocalFS virtual file descriptor.
        @param size Size of data to read.
        @return: Data and its size on success, or negative error number on failure.
        """
        result = self._read(fd, size)
        # Successful read
        if isinstance(result, bytes):
            return 0, result
        # Failed to read
        else:
            return result, b""
    # Private system function proxies
    ## Close system call proxy
    _close = _proxy_sys("close")
    ## Read system call proxy
    _read = _proxy_sys("read")
    # Public system function proxies
    ## Write system call proxy
    write = urpc_sig([I16, VARY], [I16], _proxy_sys("write"))
    ## Lseek system call proxy
    lseek = urpc_sig([I16, I16, I16], [I16], _proxy_sys("lseek"))
    # ERT functions
    @property
    def functions(self):
        """!
        @brief Get ERT service functions.
        """
        return OrderedDict([
            ("open", self.open),
            ("close", self.close),
            ("read", self.read),
            ("write", self.write),
            ("lseek", self.lseek)
        ])
    ## ERT constants
    constants = [
        # Open flags
        (os.O_CREAT, I16),
        (os.O_RDONLY, I16),
        (os.O_WRONLY, I16),
        (os.O_RDWR, I16),
        (os.O_CREAT, I16),
        # Whence
        (os.SEEK_SET, I16),
        (os.SEEK_CUR, I16),
        (os.SEEK_END, I16),
        # Error numbers
        (errno.EBADF, I16),
        (errno.EINVAL, I16),
    ]
