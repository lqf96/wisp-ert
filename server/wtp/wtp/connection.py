from __future__ import absolute_import, unicode_literals
from twisted.internet.defer import Deferred, deferLater

import wtp.constants as consts
from wtp.util import EventTarget, read_stream

class WTPConnection(EventTarget)
    """ WTP connection type. """
    def __init__(self, server, wisp_id):
        # Initialize base classes
        super(WTPConnection, self).__init__()
        # WTP server reference
        self._server = server
        # WISP ID
        self._wisp_id = wisp_id
        # Downlink and uplink state
        self._uplink_state = consts.WTP_STATE_CLOSED
        self._downlink_state = consts.WTP_STATE_CLOSED
        # Send and receive sequence number
        self._send_seq = 0
        self._recv_seq = 0
        # Sending messages and control packets queue
        self._send_msgs = []
        self._send_packets = []
        # Sending message begin position and length
        self._send_msg_begin = 0
        self._send_msg_size = 0
        # Received messages
        self._recv_msgs = []
        # Receiving message buffer
        self._recv_msg_buf = None
        # Receiving message begin position and length
        self._recv_msg_begin = 0
        self._recv_msg_size = 0
        # Unstable internal variables
        self.__write_pending = False
        self.__recv_deferreds = []
        self.__current_opspec_id = 1
    def _handle_packet(self, stream):
        """
        Handle WTP packet for current connection.

        :param stream: Data stream containing WTP packet
        :param packet_type: Packet type
        """
        handler = self._pkt_handler.get(packet_type)
        # Drop packet with unknown packet type
        if handler:
            handler(self, stream)
    def _handle_data_packet(self, stream):
        """
        Handle data packet (Begin message and continue message packet).

        :param stream: Data stream containing packet data
        """
        # Read offset and payload size
        offset, payload_size = read_stream(stream, "HB")
        # Read payload
        payload = stream.read(payload_size)
        # Validate checksum
        stream.validate_checksum()
        # Return offset and payload
        return offset, payload
    def _handle_open(self, stream):
        """
        Handle WTP open packet.

        :param stream: Data stream containing open packet
        """
        # Read connection mode
        mode = read_stream(stream, "B")
        # Validate checksum
        stream.validate_checksum()
        # Update connection information
        self._reliable = True
        self._uplink_state = WTP_STATE_OPENED
        # Send Ack message
        self._send_ack()
    def _handle_close(self, stream):
        """
        Handle WTP close packet.

        :param stream: Data stream containing close packet
        """
        # Validate checksum
        stream.validate_checksum()
        # Update connection information
        self._uplink_state = WTP_STATE_CLOSED
        # Trigger half close or close event
        if self._downlink_state==WTP_STATE_OPENED:
            self.trigger("half-close")
        elif self._downlink_state==WTP_STATE_CLOSED:
            self.trigger("close")
        # Send Ack message
        self._send_ack()
    def _handle_ack(self, stream):
        """
        Handle WTP acknowledgement packet.

        :param stream: Data stream containing acknowledgement packet
        """
        # Read acknowledged bytes
        acked = read_stream(stream, "H")
        # Validate checksum
        stream.validate_checksum()
        # TODO: What to do next
        pass
    def _handle_begin_msg(self, stream):
        """
        Handle WTP begin message packet.
        This is a stub function and all works are actually done in "_handle_msg_data()".

        :param stream: Data stream containing begin message packet
        """
        self._handle_msg_data(stream, True)
    def _handle_msg_data(self, stream, msg_begin=False):
        """
        Handle WTP message data packet.

        :param stream: Data stream containing continue message packet
        :param begin: Is this packet a begin message packet?
        """
        # Read message size
        msg_size = read_stream(stream, "H") if msg_begin else self._recv_msg_size
        # Read offset and payload size
        offset, payload_size = read_stream(stream, "HB")
        # Read payload
        payload = stream.read(payload_size)
        # Validate checksum
        stream.validate_checksum()
        # Message end
        msg_end = self._recv_msg_begin+self._recv_msg_size
        # Check if packet begins at acknowledged position
        if offset!=self._recv_seq:
            return
        # For begin of the message
        if begin_msg:
            # Check if new message begins at the ent of old message
            if offset!=msg_end:
                return
            # Check if payload size is greater than message size
            if payload_size>msg_size:
                return
            # Update message begin and message size
            self._recv_msg_begin = offset
            self._recv_msg_size = msg_size
            # Update message end
            msg_end = offset+msg_size
            # Reset received message buffer
            self._recv_msg_buf = b""
        else:
            # Check if end of payload exceeds message data range
            if offset+payload_size>msg_end:
                return
        # Update sequence number
        self._recv_seq += payload_size
        # Append data to buffer
        self._recv_msg_buf += payload
        # End of message
        if offset+payload_sie==self._recv_msg_begin+self._recv_msg_size:
            if len(self.__recv_deferreds)>0:
                self.__recv_deferreds.pop(0).callback(self._recv_msg_buf)
            else:
                self._recv_msgs.append(self._recv_msg_buf)
        # Send acknowledgement
        self._send_ack()
    def _handle_req_uplink(self, stream):
        """
        Handle WTP request uplink packet.

        :param stream: Data stream containing request uplink packet
        """
        # Number of Read operations requested
        n_reads = read_stream(stream, "B")
        # Validate checksum
        stream.validate_checksum()
        # Send Read OpSpec
        self._send_opspec(read_opspec(24))
    def send(self, msg):
        """
        Send a message to WISP.

        :param msg: Message data
        """
        pass
    def recv(self):
        """
        Receive a message from WISP.

        :returns: A deferred object that will be resolved with message data
        """
        d = Deferred()
        # Resolve deferred object if there are pending messages in queue
        if self._recv_msgs:
            d.callback(self._recv_msgs.pop(0))
        # Add deferred object to queue
        else:
            self.__recv_deferreds.append(d)
        return d
    def __send_read_opspec(self):
        self._server._send_opspecs(self._wisp_id, [read_opspec(24, self.__current_opspec_id)])
        self.__current_opspec_id += 1
    def __do_send_write_opspec(self):
        self.__write_pending = False
        send_data = b""
        # Send all control packets
        while self._send_packets and len(send_data)<24:
            send_data += self._send_packets.pop(0)
        # Send data
        if self._send_seq==self._send_msg_begin+self._send_msg_size:
            self._send_msgs.pop(0)
            self._send_msg_begin = self._send_seq
            self._send_msg_size = len(self._send_msgs[0])
        begin = self._send_seq-self._send_msg_begin
        end = begin+24-len(send_data)
        send_data += self._send_msgs[0][begin:end]
        # Terminate symbol
        send_data += b"\x00"
        # Send write opspec
        self._server._send_opspecs(self._wisp_id, [write_opspec(send_data, self.__current_opspec_id)])
        self.__current_opspec_id += 1
        # Continue send data
        if self._send_packets or self._send_msgs:
            self.__send_write_opspec()
    def __send_write_opspec(self):
        # Pending write operation
        if self.__write_pending:
            return
        self.__write_pending = True
        # Wait for 50ms and send
        self._server._set_timeout(0.05, self.__do_send_write_opspec)
