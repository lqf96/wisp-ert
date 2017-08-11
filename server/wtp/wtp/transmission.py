from __future__ import absolute_import, unicode_literals
import struct
from abc import ABC
from six.moves import range
from collections import namedtuple
from twisted.internet.defer import Deferred

from wtp.constants import WTP_SEQ_MAX
from wtp.util import CyclicInt, CyclicRange, ChecksumStream, xor_checksum, xor_checksum_size

class SlidingWindowTxControl(object):
    """ Sliding window-based transmit control class. """
    def __init__(self, write_size, window_size, checksum_func, checksum_type, timeout):
        # Write OpSpec data size
        self.write_size = write_size
        # Sliding window size
        self.window_size = window_size
        # Sending data timeout
        self.timeout = timeout
        # Checksum function and type
        self.checksum_func = checksum_func
        self.checksum_type = checksum_type
        # Pending packets
        self._packets = []
    def add_msg(self, msg_data):
        """
        Add a new message for sending.

        :param msg_data: Message data to send
        """
        self._messages.append(msg_data)
    def add_packet(self, packet_data):
        """
        Add a new packet for sending.

        :param packet_data: Packet data to send
        """
        self._packets.append(packet_data)
    def handle_ack(self, seq_num):
        """
        Handle acknowledgement.
        """
        # TODO: Clear message timeout
        pass
    def get_write_data(self):
        """
        Get Write/BlockWrite OpSpec data.
        """
        stream = ChecksumStream(
            checksum_func=self.checksum_func,
            checksum_type=self.checksum_type
        )
        estimate_size = 0
        # Write packets to the stream
        packets = self._packets
        while packets:
            # Calculate new estimate packet length
            packet = packets.pop(0)
            estimate_size += len(packet)+struct.calcsize(self.checksum_type)
            # OpSpec data will be too long
            if estimate_size>self.write_size:
                return stream.getvalue()
            # Write data and checksum to stream
            stream.begin_checksum()
            stream.write(packet)
            stream.write_checksum()
        # TODO: Write message data to stream

# Received message information
RxMsgInfo = namedtuple("RxMsgInfo", ["begin", "size"])
# Received data fragment
RxFragment = namedtuple("RxFragment", ["seq_num", "data"])

class SlidingWindowRxControl(object):
    """ Sliding window-based receive control class. """
    def __init__(self, window_size):
        # Sequence number
        self.seq_num = CyclicInt(0, WTP_SEQ_MAX)
        # Sliding window size
        self.window_size = window_size
        # Acknowledged next message data
        self._msg_data = bytearray()
        # Non-acknowledged data fragments
        self._fragments = []
        # Message begin sequence number and size
        self._msg_info = []
    def handle_packet(self, seq_num, data, new_msg_size=None):
        """
        Handle new data packet that arrives on the connection.

        :param seq_num: Packet sequence number
        :param data: Packet payload data
        :param new_msg_size: Message size for begin message data packet
        :returns: Newly received messages
        """
        # Packet sequence number
        seq_num = CyclicInt(seq_num, WTP_SEQ_MAX)
        # Packet data range and sliding window
        pkt_range = CyclicRange(seq_num, size=len(data), radix=WTP_SEQ_MAX)
        sliding_window = CyclicRange(self.seq_num, size=self.window_size, radix=WTP_SEQ_MAX)
        # Packet data range must be within sliding window, or the packet is dropped
        if pkt_range not in sliding_window:
            return []
        # Begin of message
        msg_info = self._msg_info
        if new_msg_size:
            new_msg_size = CyclicInt(new_msg_size)
            with sliding_window.compare_in_range():
                i = 0
                # Find position to insert new message information
                for i in range(len(msg_info)):
                    if seq_num<msg_info[i].begin:
                        break
                # Drop new message packet if it overlaps with declared messages
                if seq_num+new_msg_size>msg_info[i].begin:
                    return []
                # Insert new message information
                msg_info.insert(i, RxMsgInfo(begin=seq_num, size=new_msg_size))
        # Inserting data to data fragments
        fragments = self._fragments
        with sliding_window.compare_in_range():
            i = 0
            # Find position to insert data fragment
            for i in range(len(fragments)):
                if seq_num<fragments[i].seq_num:
                    break
            # Drop data packet if it overlaps with other data fragments
            if seq_num+len(data)>fragments[i].seq_num:
                return []
            # Insert data fragment
            fragments.insert(i, RxFragment(seq_num=seq_num, data=data))
        # Newly received messages
        new_msgs = []
        # Try to assemble received data fragments
        i = 0
        for i, fragment in enumerate(fragments):
            # Append consecutive data fragments
            if fragment.seq_num==self.seq_num:
                self._msg_data += fragment.data
                self.seq_num += len(fragment.data)
            else:
                break
            # Calculate end of current message
            if not msg_info:
                continue
            current_msg_info = msg_info[0]
            current_msg_end = current_msg_info.begin+current_msg_info.size
            # End of current message reached
            if self.seq_num==current_msg_end:
                # Update newly received messages
                new_msgs.append(self._msg_data)
                # Reset message data
                self._msg_data = bytearray()
                # Pop message information
                msg_info.pop(0)
        # Remove acknowledged data fragments
        for _ in range(i):
            fragments.pop(0)
        return new_msgs
