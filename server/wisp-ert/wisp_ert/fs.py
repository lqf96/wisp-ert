from __future__ import absolute_import, unicode_literals
import os, errno, functools
from abc import ABCMeta, abstractmethod
from six import with_metaclass
from urpc import StringType, urpc_sig, U16, I16, VARY
from urpc.util import AllocTable

from wisp_ert.runtime import Service

# System call function proxy
def _proxy_sys(name):
    """
    Wrap system call function as LocalFS service function.

    :param name: System call function name
    :returns: LocalFS service wrapper
    """
    sys_func = getattr(os, name)
    # Proxy function
    def proxy(self, fd, *args):
        # Get and check file descriptor validity
        real_fd = self._fd_mapping.get(fd)
        if not real_fd:
            return -1*errno.EBADF
        # Call system function
        try:
            return sys_func(real_fd, *args)
        except OSError as e:
            return -1*e.errno
    return proxy

# Local filesystem class
class LocalFS(Service):
    # Constructor
    def __init__(self, root_dir="/"):
        # Root directory
        self._root_dir = root_dir
        # File descriptor mapping
        self._fd_mapping = AllocTable(
            capacity=32
        )
    # Open file
    @urpc_sig([StringType, I16, I16], [I16])
    def open(self, path, flags, mode=0o666):
        """
        Open a file with given flags and mode.

        :param flags: Open flags
        :param mode: File mode when a new file is going to be created
        :returns: File descriptor on success, or negative error number on failure
        """
        # Try to open the file
        try:
            real_path = os.path.join(self._root_dir, path)
            real_fd = os.open(real_path, flags, mode)
        except OSError as e:
            return -1*e.errno
        # Add file descriptor to table
        return self._fd_table.add(real_fd)
    # Close file
    @urpc_sig([I16], [I16])
    def close(self, fd):
        """
        Close a file.

        :param fd: File descriptor of the file
        :returns: 0 on success, or negative error number on failure
        """
        # Try to close the file first
        result = self._close(fd)
        if result:
            return result
        # Remove file descriptor from table
        del self._fd_table[fd]
        return 0
    # Read file
    @urpc_sig([I16, U16], [I16, VARY])
    def read(self, fd, size):
        """
        Read given size of data from file.

        :param fd: File descriptor of the file
        :returns: Data and its size on success, or negative error number on failure
        """
        result = self._read(fd, size)
        # Successful read
        if isinstance(result, bytes):
            return len(result), result
        # Failed to read
        else:
            return result, b""
    # Private system function proxies
    _close = _proxy_sys("close")
    _read = _proxy_sys("read")
    # Public system function proxies
    write = urpc_sig([I16, VARY], [I16], _proxy_sys("write"))
    lseek = urpc_sig([I16, I16, I16], [I16], _proxy_sys("lseek"))
    # ERT functions
    functions = {
        "open": open,
        "close": close,
        "read": read,
        "write": write,
        "lseek": lseek
    }
    # ERT constants
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
