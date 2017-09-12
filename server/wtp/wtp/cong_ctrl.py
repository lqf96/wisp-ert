from __future__ import absolute_import, unicode_literals
import logging

from wtp.constants import WTP_OPSPEC_MIN, WTP_OPSPEC_MAX

# Module logger
_logger = logging.getLogger(__name__)
# Logger level
_logger.setLevel(logging.DEBUG)

# TODO: More sophisticated OpSpec size control
# TODO: Current implementation is naive and is demo only
class OpSpecSizeControl(object):
    """ A demo OpSpec size control. """
    def __init__(self, read_size, write_size):
        # Desired size of Read and BlockWrite OpSpec
        self.read_size = read_size
        self.write_size = write_size
        # Sizes of pending Read and Write/BlockWrite OpSpec
        self._pending_reads = []
        self._pending_writes = []
    def add_read(self, size):
        """
        Add size of a new Read OpSpec.

        :param size: Size of the Read OpSpec
        """
        self._pending_reads.append(size)
    def add_write(self, size):
        """
        Add size of a new BlockWrite OpSpec.

        :param size: Size of the BlockWrite OpSpec
        """
        self._pending_writes.append(size)
    def report_read_result(self, succeeded, actual_size):
        """
        Report Read OpSpec result to OpSpec size control.

        :param succeeded: Whether the Read operation succeeded or not
        :param actual_size: Actual Read size
        """
        # Pop original Read size
        read_size = self._pending_reads.pop(0)
        # Increase Read size by 2 if:
        # 1) Read succeeded
        # 2) Actual Read size is no smaller than current Read size
        # 3) Read size is smaller than maximum OpSpec payload size
        if succeeded and actual_size>=self.read_size and self.read_size<WTP_OPSPEC_MAX:
            self.read_size += 2
            _logger.debug("Read size increased by 2; currently %d", self.read_size)
        # Decrease Read size by 2 if:
        # 1) Read failed
        # 2) Read size is bigger than minimum OpSpec payload size
        if not succeeded and self.read_size>WTP_OPSPEC_MIN:
            _logger.debug("Read size decreased by 2; currently %d", self.read_size)
            self.read_size -= 2
    def report_write_result(self, succeeded, actual_size):
        """
        Report BlockWrite result to OpSpec size control.

        :param succeeded: Whether the BlockWrite operation succeeded or not
        :param actual_size: Actual BlockWrite size
        """
        # Pop original BlockWrite size
        blockwrite_size = self._pending_writes.pop(0)
        # Increase BlockWrite size by 2 if:
        # 1) BlockWrite succeeded
        # 2) Actual BlockWrite size is no smaller than current BlockWrite size
        # 3) BlockWrite size is smaller than maximum OpSpec payload size
        if succeeded and actual_size>=self.write_size and self.write_size<WTP_OPSPEC_MAX:
            _logger.debug("BlockWrite size increased by 2; currently %d", self.write_size)
            self.write_size += 2
        # Decrease BlockWrite size by 2 if:
        # 1) BlockWrite failed
        # 2) BlockWrite size is bigger than minimum OpSpec payload size
        if not succeeded and self.write_size>WTP_OPSPEC_MIN:
            _logger.debug("BlockWrite size decreased by 2; currently %d", self.write_size)
            self.write_size -= 2
