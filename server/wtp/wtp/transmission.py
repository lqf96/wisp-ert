from __future__ import absolute_import, unicode_literals
import struct, functools
from six.moves import range
from recordclass import recordclass
from twisted.internet.defer import Deferred

import wtp.constants as consts
from wtp.util import CyclicInt, CyclicRange, ChecksumStream

# Transmit fragment type
TxFragment = recordclass("TxFragment", ["seq_num", "msg_size", "data", "d", "need_send"])

class SlidingWindowTxControl(object):
    """ Sliding window-based transmit control class. """
    def __init__(self, reactor, write_size, window_size, checksum_func,
        checksum_type, timeout, request_access_spec):
        # Write OpSpec data size
        self.write_size = write_size
        # Sliding window size
        self.window_size = window_size
        # Sending data timeout
        self.timeout = timeout
        # Request sending AccessSpec function
        self.request_access_spec = request_access_spec
        # Checksum function and type
        self.checksum_func = checksum_func
        self.checksum_type = checksum_type
        # Reactor
        self._reactor = reactor
        # Sequence number
        self._seq_num = CyclicInt(0, consts.WTP_SEQ_MAX)
        # Pending packets
        self._packets = []
        # Pending messages
        self._messages = []
        # Begin position and fragmented part size of next message
        self._msg_begin = CyclicInt(0, consts.WTP_SEQ_MAX)
        self._msg_fragmented = 0
        # Sequence number of message ends
        self._msg_ends = []
        # Sending data fragments
        self._fragments = []
    def _make_fragment(self, avail_size):
        """
        Make new data fragment with given available size.

        :param avail_size: Size of space available.
        :returns: Transmit data fragment, or None if no fragment is available
        """
        # No message to make fragment from
        messages = self._messages
        if not messages:
            return None
        msg = messages[0]
        # Load next message to fragment
        if self._msg_fragmented>=len(msg):
            # Update message begin and fragmented position
            self._msg_begin += len(msg)
            self._msg_fragmented = 0
            # Add message end
            self._msg_ends.append(self._msg_begin)
            # Remove old message
            messages.pop(0)
            if not messages:
                return None
            msg = messages[0]
        # Sequence number
        msg_fragmented = self._msg_fragmented
        seq_num = self._msg_begin+msg_fragmented
        # Maximum packet data size using different criterion
        max_avail = avail_size-(6 if msg_fragmented==0 else 4)
        if self.checksum_type:
            max_avail -= struct.calcsize(self.checksum_type)
        max_msg = len(msg)-msg_fragmented
        max_window = self._seq_num+self.window_size-seq_num
        # Packet data size
        packet_data_size = min(max_avail, max_msg, max_window)
        if packet_data_size<=0:
            return None
        # Update fragmented position
        self._msg_fragmented += packet_data_size
        # Make fragment
        packet_data = msg[msg_fragmented:msg_fragmented+packet_data_size]
        return TxFragment(
            seq_num=seq_num,
            msg_size=len(msg) if msg_fragmented==0 else 0,
            data=packet_data,
            d=None,
            need_send=False
        )
    def _handle_packet_timeout(self, fragment, *args):
        """
        Handle data packet timeout.

        :param fragment: Timeout data fragment
        """
        try:
            print("Retransmit seq=%d size=%d" % (fragment.seq_num, len(fragment.data)))
            # Set need send flag
            fragment.need_send = True
            # Request sending AccessSpec
            self.request_access_spec()
        except:
            import traceback
            traceback.print_exc()
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

        :returns: Number of messages sent
        """
        # Acknowledged sequence number
        seq_num = CyclicInt(seq_num, consts.WTP_SEQ_MAX)
        # Number of messages sent
        n_sent_msgs = 0
        with self._seq_num.as_zero():
            # Invalid sequence number; drop acknowledgement
            if seq_num>self._msg_begin+self._msg_fragmented:
                return 0
            # Get number of acknowledged fragments
            fragments = self._fragments
            index = -1
            for index, fragment in enumerate(fragments):
                fragment_end = fragment.seq_num+len(fragment.data)
                # End of acknowledged range
                if fragment_end==seq_num:
                    break
                # No fragment to acknowledge or sequence number not at fragments border
                elif fragment_end>seq_num:
                    return 0
            # Remove acknowledged fragments
            msg_ends = self._msg_ends
            for _ in range(index+1):
                fragment = fragments.pop(0)
                fragment_end = fragment.seq_num+len(fragment.data)
                # Whole message sent
                if msg_ends and msg_ends[0]<=fragment_end:
                    n_sent_msgs += 1
                    msg_ends.pop(0)
                # Resolve fragment deferreds
                print("Resolve seq=%d size=%d" % (fragment.seq_num, len(fragment.data)))
                fragment.d.callback(True)
        # Update sequence number
        self._seq_num = seq_num
        return n_sent_msgs
    def get_write_data(self):
        """
        Get Write/BlockWrite OpSpec data.
        """
        stream = ChecksumStream(
            checksum_func=self.checksum_func,
            checksum_type=self.checksum_type
        )
        estimate_size = 0
        # Write packets to stream
        packets = self._packets
        while packets:
            # Calculate new estimate payload length
            packet = packets.pop(0)
            estimate_size += len(packet)
            # OpSpec data will be too long
            if estimate_size>self.write_size:
                return stream.getvalue()
            # Write packet and checksum to stream
            stream.write(packet)
            stream.write_checksum()
        # Write message data to stream
        fragments = self._fragments
        while True:
            send_fragment = None
            # Try to find existing data fragment to send
            for fragment in fragments:
                if fragment.need_send:
                    send_fragment = fragment
                    send_fragment.need_send = False
                    break
            # Try to make new data fragment to send
            if not send_fragment:
                send_fragment = self._make_fragment(self.write_size-estimate_size)
                if send_fragment:
                    fragments.append(send_fragment)
                # No more fragments to send
                else:
                    break
            # Calculate new estimate payload length
            packet_size = 4+len(send_fragment.data)
            estimate_size += packet_size
            # OpSpec data will be too long
            if estimate_size>self.write_size:
                return stream.getvalue()
            # Write packet data
            stream.begin_checksum()
            if send_fragment.msg_size:
                stream.write_data("BH", consts.WTP_PKT_BEGIN_MSG, send_fragment.msg_size)
            else:
                stream.write_data("B", consts.WTP_PKT_CONT_MSG)
            stream.write_data("HB", send_fragment.seq_num, len(send_fragment.data))
            stream.write(send_fragment.data)
            stream.write_checksum()
            # Set fragment timeout
            d = Deferred()
            d.addTimeout(self.timeout, self._reactor, onTimeoutCancel=functools.partial(
                SlidingWindowTxControl._handle_packet_timeout, self, send_fragment
            ))
            send_fragment.d = d
        # Return send data
        return stream.getvalue()

# Received message information
RxMsgInfo = recordclass("RxMsgInfo", ["begin", "size"])
# Received data fragment
RxFragment = recordclass("RxFragment", ["seq_num", "data"])

class SlidingWindowRxControl(object):
    """ Sliding window-based receive control class. """
    def __init__(self, window_size):
        # Sequence number
        self.seq_num = CyclicInt(0, consts.WTP_SEQ_MAX)
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
        seq_num = CyclicInt(seq_num, consts.WTP_SEQ_MAX)
        # Packet data range and sliding window
        pkt_range = CyclicRange(seq_num, size=len(data), radix=consts.WTP_SEQ_MAX)
        sliding_window = CyclicRange(self.seq_num, size=self.window_size, radix=consts.WTP_SEQ_MAX)
        # Packet data range must be within sliding window, or the packet is dropped
        if pkt_range not in sliding_window:
            return []
        # Begin of message
        msg_info = self._msg_info
        if new_msg_size:
            new_msg_size = CyclicInt(new_msg_size, radix=consts.WTP_SEQ_MAX)
            with self.seq_num.as_zero():
                i = 0
                # Find position to insert new message information
                for i in range(len(msg_info)):
                    if seq_num<msg_info[i].begin:
                        break
                # Drop new message packet if it overlaps with declared messages
                if i<len(msg_info) and seq_num+new_msg_size>msg_info[i].begin:
                    return []
                # Insert new message information
                msg_info.insert(i, RxMsgInfo(begin=seq_num, size=new_msg_size))
        # Inserting data to data fragments
        fragments = self._fragments
        with self.seq_num.as_zero():
            i = 0
            # Find position to insert data fragment
            for i in range(len(fragments)):
                if seq_num<fragments[i].seq_num:
                    break
            # Drop data packet if it overlaps with other data fragments
            if i<len(fragments) and seq_num+len(data)>fragments[i].seq_num:
                return []
            # Insert data fragment
            fragments.insert(i, RxFragment(seq_num=seq_num, data=data))
        # Newly received messages
        new_msgs = []
        # Try to assemble received data fragments
        index = -1
        for index, fragment in enumerate(fragments):
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
        for _ in range(index+1):
            fragments.pop(0)
        return new_msgs
