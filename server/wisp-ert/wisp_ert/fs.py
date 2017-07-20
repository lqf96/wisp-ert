from __future__ import unicode_literals
import os, errno, functools
from abc import ABCMeta, abstractmethod

from six import with_metaclass
from urpc.util import AllocTable

# Abstract filesystem base class
class FileSystem(with_metaclass(ABCMeta, object)):
    # Open file
    @abstractmethod
    def open(path, flags, mode=0666):
        """
        Open a file and return  file descriptor.

        :param path: Path of the file.
        :param flags:
        :param mode:
        """
        pass
    # Close file
    @abstractmethod
    def close(fd):
        """
        Close a file.

        :param fd: File descriptor.
        """
        pass
    # Read file
    @abstractmethod
    def read(fd, size):
        """
        Read data from a file descriptor.

        :param size: Size of the data to read
        """
        pass
    # Write file
    @abstractmethod
    def write(fd, data):
        """
        Write data to a file descriptor.

        :param data: Data to write
        """
        pass
    # Seek
    @abstractmethod
    def lseek(fd, offset, whence):
        """
        Set the current position of a file descriptor.
        Return the new cursor position in bytes, starting from the beginning.

        :param fd: File descriptor
        :param offset: New position of the file descriptor
        :param whence: How the offset is interpreted
        """
        pass

# System call function proxy
def _proxy_sys(name):
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
class LocalFS(FileSystem):
    # Constructor
    def __init__(self, root_dir="/"):
        # Root directory
        self._root_dir = root_dir
        # File descriptor mapping
        self._fd_mapping = AllocTable()
    # Open file
    def open(path, flags, mode):
        # Try to open the file
        try:
            real_path = os.path.join(self._root_dir, path)
            real_fd = os.open(real_path, flags, mode)
        except OSError as e:
            return -1*e.errno
        # Add file descriptor to table
        return self._fd_table.add(real_fd)
    # Close file
    def close(fd):
        # Try to close the file first
        result = self._close(fd)
        if result:
            return result
        # Remove file descriptor from table
        del self._fd_table[fd]
        return 0
    # System function proxies
    _close = _proxy_sys("close")
    read = _proxy_sys("read")
    write = _proxy_sys("write")
    lseek = _proxy_sys("lseek")
