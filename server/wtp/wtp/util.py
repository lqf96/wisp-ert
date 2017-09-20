from __future__ import absolute_import, unicode_literals
import struct, logging, functools
from io import BytesIO
from contextlib import contextmanager
from collections import Container
from traceback import print_exc
from six import text_type

from wtp.error import WTPError
from wtp.constants import WTP_ERR_INVALID_CHECKSUM

class EventTarget(object):
    """!
    @brief Event target mix-in class.
    """
    def __init__(self):
        """!
        @brief Event target mix-in constructor.
        """
        ## All event handlers
        self._all_handlers = {}
    def on(self, event, handler=None):
        """!
        @brief Add an event handler.

        @param event Event name.
        @param handler Event handler function.
        @return Event handler function.
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
        """!
        @brief Remove an event handler.

        @param event Event name.
        @param handler Event handler function.
        """
        # Get specific event handlers
        event_handlers = self._all_handlers.get(event)
        # Remove handler
        if event_handlers:
            event_handlers.remove(handler)
    def trigger(self, event, result=None):
        """!
        @brief Trigger an event.

        @param event Event name.
        @param result Event result.
        """
        # Get specific event handlers
        event_handlers = self._all_handlers.get(event)
        # Call handlers
        if event_handlers:
            for handler in event_handlers:
                handler(result)

class DataStreamMixin(object):
    """!
    @brief Byte stream mix-in for reading and writing Python values.
    """
    def read_data(self, fmt, endian="<"):
        """!
        @brief Read binary data from stream with given format.

        @param fmt Binary data format.
        @param endian Endianness of data to read.
        @return (A tuple of) read data.
        """
        # Read format
        read_fmt = endian+fmt
        # Read data
        read_data = self.read(struct.calcsize(read_fmt))
        if not read_data:
            return None
        # Unpack data
        result = struct.unpack(read_fmt, read_data)
        # One result
        if len(result)==1:
            return result[0]
        else:
            return result
    def write_data(self, fmt, *args, **kwargs):
        """!
        @brief Write binary data to stream with given format.

        @param fmt Binary data format.
        @param args Data to write.
        @param kwargs Options for writing data\n
            endian: Endianness of data to write.
        """
        # Endianness
        endian = kwargs.pop("endian", "<")
        # Write data
        self.write(struct.pack(endian+fmt, *args))

class ChecksumStream(BytesIO, DataStreamMixin):
    """!
    @brief Byte data stream with checksum functionality.
    """
    def __init__(self, *args, **kwargs):
        """!
        @brief Checksum stream constructor.
        """
        ## Checksum function
        self._checksum_func = kwargs.pop("checksum_func", None)
        ## Checksum data type
        self._checksum_type = kwargs.pop("checksum_type", "<B")
        ## Begin checksum position
        self._begin_pos = 0
        # Initialize base class
        super(ChecksumStream, self).__init__(*args, **kwargs)
    def begin_checksum(self):
        """!
        @brief Begin checksum calculation.
        """
        self._begin_pos = self.tell()
    def validate_checksum(self):
        """!
        @brief Validate checksum.

        @throws WTPError if checksum validation failed.
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
    def write_checksum(self):
        """!
        @brief Write checksum to stream.
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
    """!
    @brief Xor checksum function.

    @param buf Buffer to calculate checksum.
    """
    # Convert buffer to byte array
    buf = bytearray(buf)
    # Calculate checksum
    checksum = 0
    for byte in buf:
        checksum ^= byte
    return checksum

def _check_radix(x, y):
    """!
    @brief Check the radix of two cyclic integers or ranges.

    @param x Cyclic integer or range.
    @param y Cyclic integer or range.
    @throws ValueError if radix of x and y mismatch.
    """
    if x._radix!=y._radix:
        raise ValueError("Radix mismatch.")

@functools.total_ordering
class CyclicInt(object):
    """!
    @brief Cyclic integer type.
    """
    def __init__(self, value, radix):
        """!
        @brief Cyclic integer type constructor.

        @param value Value of the cyclic integer.
        @param radix Radix of the cyclic integer.
        """
        ## Value
        self._value = value%radix
        ## Radix
        self._radix = radix
    def __eq__(self, other):
        """!
        @brief Compare equality of cyclic integer with another value.

        @param other Other value.
        @return Whether two cyclic integer is equal or not.
        """
        # Cyclic integer
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
            return self._value==other._value
        # Other types
        else:
            return self._value==int(other)
    def __lt__(self, other):
        """!
        @brief Compare if a cyclic integer is smaller than another value.

        You should enter a zero context before comparing two cyclic integers.

        @param other Other value.
        @return Whether this cyclic integer is smaller than the other value.
        """
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
        """!
        @brief Compare if a cyclic integer is bigger than another value.

        You should enter a zero context before comparing two cyclic integers.

        @param other Other value.
        @return Whether this cyclic integer is bigger than the other value.
        """
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
        """!
        @brief Add cyclic integer with another value.

        @param other Other value.
        @return A cyclic sum of the two values.
        """
        # For cyclic integer, check its radix
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
        # Return new cyclic integer
        return CyclicInt(self._value+int(other), self._radix)
    def __radd__(self, other):
        """!
        @brief Add cyclic integer with another value.

        (Proxied to self.__add__)

        @param other Other value.
        @return A cyclic sum of the two values.
        """
        return self.__add__(other)
    def __sub__(self, other):
        """!
        @brief Subtract a value from self.

        @param other Other value
        @return A cyclic difference of the two values.
        """
        # For cyclic integer, check its radix
        if isinstance(other, CyclicInt):
            _check_radix(self, other)
        # Return new cyclic integer
        return CyclicInt(self._value-int(other), self._radix)
    def __neg__(self):
        """!
        @brief Get supplementary cyclic integer.

        @return Supplementary cyclic integer.
        """
        return CyclicInt(-self._value, self._radix)
    def __int__(self):
        """!
        @brief Convert a cyclic integer to built-in integer.

        @return Corresponding Python integer.
        """
        return self._value
    def __index__(self):
        """!
        @brief Convert cyclic integer to collection index.

        (Proxied to self.__int__)

        @return Corresponding Python integer.
        """
        return self.__int__()
    def __repr__(self):
        """!
        @brief Representation of cyclic integer.

        @return A readable representation of the cyclic integer.
        """
        return "CyclicInt(%d, %d)" % (self._value, self._radix)
    @classmethod
    def _zero_ctx(cls):
        """!
        @brief Get current zero point context.

        @return Current zero point context.
        @throws Exception if not in a zero point context.
        """
        if cls._zero_ctx_stack:
            return cls._zero_ctx_stack[-1]
        # Not in zero point context
        else:
            raise Exception("Not in a zero point context.")
    @contextmanager
    def as_zero(self):
        """!
        @brief Enter a context with this cyclic integer as zero point.

        (The old zero context will be saved in a zero context stack)
        """
        # Add to contexts stack
        self._zero_ctx_stack.append(self)
        # Enter context
        yield
        # Remove from stack
        self._zero_ctx_stack.pop()
    ## Zero point contexts stack
    _zero_ctx_stack = []

class CyclicRange(object):
    """!
    @brief Cyclic range type.
    """
    def __init__(self, x, radix, y=None, size=None):
        """!
        @brief Cyclic range type constructor.

        @param x Begin of the range.
        @param radix Radix of the cyclic range.
        @param y End of the range.
        @param size Size of the range.
        """
        # Range end
        if y==None and size==None:
            raise ValueError("Either y or size must be given to construct cyclic range.")
        y = y if y else x+size
        # Range bounds
        self._x = int(x)%radix
        self._y = int(y)%radix
        # Radix
        self._radix = radix
    def __eq__(self, other):
        """!
        @brief Check if other value equals with this cyclic range.

        @param other Other value.
        @return Whether two values equal or not.
        """
        if isinstance(other, CyclicRange):
            _check_radix(self, other)
            return self._x==other._x and self._y==other._y
        else:
            return False
    def __contains__(self, other):
        """!
        @brief Check if given value, cyclic integer or cyclic range is within this cyclic range.

        @param other Other value.
        @return Whether the other value is in this cyclic range.
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

def force_print_exc(func):
    """!
    @brief Force printing traceback when exception is thrown.

    (Used for debugging code inside Twisted deferred object)

    @param func Function to wrap.
    @return Wrapper function.
    """
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except:
            print_exc()
    return wrapper
