from __future__ import absolute_import, unicode_literals
import struct
from io import BytesIO
from contextlib import contextmanager
from six import text_type

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
        self._checksum_type = kwargs.pop("checksum_type", "<B")
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
        read_checksum = self.read_data(self._checksum_type)
        # Throw checksum error if validation failed
        if read_checksum!=calc_checksum:
            raise WTPError(WTP_ERR_INVALID_CHECKSUM)
    def read_data(self, fmt, endian="<"):
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
    def write_data(self, fmt, *args, **kwargs):
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

    :param buf: Buffer to calculate checksum
    """
    checksum = 0
    for byte in buf:
        checksum ^= byte
    return checksum

class CyclicInt(object):
    """ Cyclic integer type. """
    def __init__(self, value, radix):
        # Value
        self._value = value%radix
        # Radix
        self._radix = radix
    def __eq__(self, other):
        """
        Compare equality of cyclic integer with another value.

        :param other: Other value
        """
        # Cyclic integer
        if isinstance(other, CyclicInt):
            return self._value==other._value and self._radix==other._radix
        # Other types
        else:
            return self._value==int(other)
    def __lt__(self, other):
        # TODO: Lower than
        pass
    def __gt__(self, other):
        # TODO: Greater than
        pass
    def __add__(self, other):
        """
        Add cyclic integer with another value.

        :param other: Other value
        """
        # For cyclic integer, check its radix
        if isinstance(other, CyclicInt):
            if self._radix!=other._radix:
                raise ValueError("Attempt to add another cyclic integer with different radix.")
        # Return new cyclic integer
        return CyclicInt(self._value+int(other), self._radix)
    def __radd__(self, other):
        """
        Add cyclic integer with another value.
        (Proxied to self.__add__)

        :param other: Other value
        """
        return self.__add__(other)
    def __int__(self):
        """
        Convert cyclic integer to built-in integer.
        """
        return self._value
    def __index__(self):
        """
        Convert cyclic integer to collection index.
        (Proxied to self.__int__)
        """
        return self.__int__()
    def __repr__(self):
        """
        Representation of cyclic integer.
        """
        return "CyclicInt(%d, %d)" % (self._value, self._radix)

class CyclicRange(object):
    """ Cyclic range type. """
    def __init__(self, x, radix, y=None, size=None):
        # Range end
        if not y and not size:
            raise ValueError("Either y or size must be given to construct cyclic range.")
        y = y if y else x+size
        # Range bounds
        self._x = x%radix
        self._y = y%radix
        # Radix
        self._radix = radix
    def __eq__(self, other):
        """
        Check if other value equals with this cyclic range.

        :param other: Other value
        :returns: Whether two values equal or not
        """
        if isinstance(other, CyclicRange):
            return self._x==other._x and self._y==other._y and self._radix==other._radix
        else:
            return False
    def __contains__(self, other):
        """
        Check if given value, cyclic integer or cyclic range is within this cyclic range.

        :param other: Other value
        """
        # Cyclic range and cyclic integer
        if isinstance(other, (CyclicRange, CyclicInt)):
            # Check its radix
            if other._radix!=self._radix:
                raise ValueError("Parameter radix mismatch.")
        # TODO: Cyclic range
        if isinstance(other, CyclicRange):
            pass
        # TODO: Cyclic integer or other values
        else:
            pass
    @contextmanager
    def compare_in_range(self):
        """
        Compare two cyclic integer within given range.
        """
        # Push current range into contexts
        self._cmp_ctx.append(self)
        # Enter context
        yield
        # Pop current range from contexts
        self._cmp_ctx.pop()
    # Compare contexts
    _cmp_ctx = []
