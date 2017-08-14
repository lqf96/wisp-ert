from __future__ import absolute_import, unicode_literals
import struct
from functools import total_ordering
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

def _check_radix(x, y):
    """
    Check the radix of two cyclic integers or ranges.

    :param x, y: Cyclic integer or range
    :raises: ValueError if radix of x and y mismatch
    """
    if x._radix!=y._radix:
        raise ValueError("Radix mismatch.")

@total_ordering
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
            _check_radix(self, other)
            return self._value==other._value
        # Other types
        else:
            return self._value==int(other)
    def __lt__(self, other):
        # Cyclic integer
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
            # Get zero point context
            zero = self._zero_ctx()
            _check_radix(self, zero)
            # Do comparison
            rel_self = (self._value-zero._value)%self._radix
            rel_other = (other._value-zero._value)%self._radix
            return rel_self<rel_other
        # Other types
        else:
            return self._value<int(other)
    def __gt__(self, other):
        # Cyclic integer
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
            # Get zero point context
            zero = self._zero_ctx()
            _check_radix(self, zero)
            # Do comparison
            rel_self = (self._value-zero._value)%self._radix
            rel_other = (other._value-zero._value)%self._radix
            return rel_self>rel_other
        # Other types
        else:
            return self._value>int(other)
    def __add__(self, other):
        """
        Add cyclic integer with another value.

        :param other: Other value
        """
        # For cyclic integer, check its radix
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
        # Return new cyclic integer
        return CyclicInt(self._value+int(other), self._radix)
    def __radd__(self, other):
        """
        Add cyclic integer with another value.
        (Proxied to self.__add__)

        :param other: Other value
        """
        return self.__add__(other)
    def __sub__(self, other):
        """
        Subtract a value from self.

        :param other: Other value
        """
        # For cyclic integer, check its radix
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
        # Return new cyclic integer
        return CyclicInt(self._value-int(other), self._radix)
    def __neg__(self):
        """
        Get supplementary cyclic integer.
        """
        return CyclicInt(-self._value, self._radix)
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
    @classmethod
    def _zero_ctx(cls):
        """
        Get current zero point context.

        :returns: Current zero point context
        :raises: Exception if not in a zero point context
        """
        if cls._zero_ctx_stack:
            return cls._zero_ctx_stack[-1]
        # Not in zero point context
        else:
            raise Exception("Not in a zero point context.")
    @contextmanager
    def as_zero(self):
        """
        Enter a context with this cyclic integer as zero point.
        """
        # Add to contexts stack
        self._zero_ctx_stack.append(self)
        # Enter context
        yield
        # Remove from stack
        self._zero_ctx_stack.pop()
    # Zero point contexts stack
    _zero_ctx_stack = []

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
            _check_radix(self, other)
            return self._x==other._x and self._y==other._y
        else:
            return False
    def __contains__(self, other):
        """
        Check if given value, cyclic integer or cyclic range is within this cyclic range.

        :param other: Other value
        """
        # Cyclic range and cyclic integer
        if isinstance(other, (CyclicRange, CyclicInt)):
            # Check radix
            _check_radix(self, other)
        # Radix
        radix = self._radix
        # Range boundaries
        self_x = CyclicInt(self._x, radix)
        self_y = CyclicInt(self._y, radix)
        # Cyclic range
        if isinstance(other, CyclicRange):
            # Range boundaries
            other_x = CyclicInt(other._x, radix)
            other_y = CyclicInt(other._y, radix)
            # Do comparison
            with self_x.as_zero():
                return other_y>=other_x and self_y>=other_y
        # TODO: Cyclic integer or other values
        else:
            pass
