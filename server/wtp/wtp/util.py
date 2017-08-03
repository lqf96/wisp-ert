from __future__ import absolute_import, unicode_literals
import struct
from io import BytesIO

from wtp.error import WTPError
from wtp.constants import WTP_ERR_INVALID_CHECKSUM

class EventTarget(object):
    """ Event target mix-in class. """
    def __init__(self):
        # Event handlers
        self._all_handlers = {}
    def on(self, event, handler=None):
        """
        Add an event handler.

        :param event: Event name
        :param handler: Event handler function
        :returns: Event handler
        """
        # Used as decorator
        if not handler:
            return lambda _handler: self.on(event, _handler)
        # Get specific event handlers
        event_handlers = self._all_handlers.setdefault(event, [])
        # Add handler
        event_handlers.append(handler)
        # Return handler function
        return handler
    def off(self, event, handler):
        """
        Remove an event handler.

        :param event: Event name
        :param handler: Event handler function
        """
        # Get specific event handlers
        event_handlers = self._all_handlers.get(event)
        # Remove handler
        if event_handlers:
            event_handlers.remove(handler)
    def trigger(self, event, result=None):
        """
        Trigger an event.

        :param event: Event name
        :param result: Event result
        """
        # Get specific event handlers
        event_handlers = self._all_handlers.get(event)
        # Call handlers
        if event_handlers:
            for handler in event_handlers:
                handler(result)

class ChecksumStream(BytesIO):
    """ Stream with checksum functionality. """
    def __init__(self, *args, **kwargs):
        # Checksum function
        self._checksum_func = kwargs.pop("checksum_func", None)
        # Checksum data type
        self._checksum_type = kwargs.pop("checksum_type", ">B")
        # Begin checksum position
        self._begin_pos = 0
        # Initialize base class
        super(ChecksumStream, self).__init__(*args, **kwargs)
    def begin_checksum(self):
        """
        Begin checksum calculation.
        """
        self._begin_pos = self.tell()
    def validate_checksum(self):
        """
        Validate checksum.
        """
        end_pos = self.tell()
        # Calculate checksum
        calc_checksum = 0
        if not self._checksum_func:
            return
        buffer_slice = self.getvalue()[self._begin_pos:end_pos]
        calc_checksum = self._checksum_func(buffer_slice)
        # Read checksum
        read_checksum = self.read_stream(self._checksum_type)
        # Throw checksum error if validation failed
        if read_checksum!=calc_checksum:
            raise WTPError(WTP_ERR_INVALID_CHECKSUM)
    def read_stream(self, fmt, endian="<"):
        """
        Read binary data from stream with given format.

        :param fmt: Binary data format
        :returns: Read data
        """
        # Read data
        read_len = struct.calcsize(endian+fmt)
        result = struct.unpack(endian+fmt, self.read(read_len))
        # One result
        if len(result)==1:
            return result[0]
        else:
            return result
    def write_stream(self, fmt, *args, **kwargs):
        """
        Write binary data to stream with given format.

        :param fmt: Binary data format
        """
        # Endianness
        endian = kwargs.pop("endian", "<")
        # Write data
        self.write(struct.pack(endian+fmt, *args))
    def write_checksum(self):
        """
        Write checksum to stream.
        """
        end_pos = self.tell()
        # Calculate checksum
        if not self._checksum_func:
            return
        buffer_slice = self.getvalue()[self._begin_pos:end_pos]
        checksum = self._checksum_func(buffer_slice)
        # Write checksum to stream
        self.write(struct.pack(self._checksum_type, checksum))

def xor_checksum(buf):
    """
    Xor checksum function.
    """
    checksum = 0
    for byte in buf:
        checksum ^= byte
    return checksum
