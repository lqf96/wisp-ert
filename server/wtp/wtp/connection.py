from __future__ import absolute_import, unicode_literals
import functools
from six.moves import range
from twisted.internet.defer import Deferred

import wtp.constants as consts
from wtp.util import EventTarget, ChecksumStream
from wtp.transmission import SlidingWindowTxControl, SlidingWindowRxControl
from wtp.llrp_util import read_opspec, write_opspec

class WTPConnection(EventTarget):
    """ WTP connection class. """
    def __init__(self, server, wisp_id, checksum_func, checksum_type):
        # Initialize base classes
        super(WTPConnection, self).__init__()
        # WTP server reference
        self.server = server
        # WISP ID
        self.wisp_id = wisp_id
        # Downlink and uplink state
        self.uplink_state = consts.WTP_STATE_CLOSED
        self.downlink_state = consts.WTP_STATE_CLOSED
        # Transmission control tools
        self._tx_ctrl = SlidingWindowTxControl(
            reactor=self.server._reactor,
            write_size=24,
            window_size=64,
            checksum_func=checksum_func,
            checksum_type=checksum_type,
            timeout=100,
            request_access_spec=self._request_access_spec
        )
        self._rx_ctrl = SlidingWindowRxControl(
            window_size=64
        )
        # Receive messages and deferreds
        self._recv_msgs = []
        self._recv_deferreds = []
        # Sending deferreds
        self._send_deferreds = []
        # Sizes of pending Read OpSpecs
        self._read_opspec_sizes = []
        # Ongoing AccessSpec flag
        self._ongoing_access_spec = False
    def _build_header(self, packet_type):
        """
        Build WTP packet header for sending.

        :param packet_type: WTP packet type
        :returns: Checksum stream containing WTP packet header
        """
        stream = ChecksumStream()
        # Write packet type
        stream.write_data("B", packet_type)
        return stream
    def _build_ack(self):
        """
        Build WTP acknowledgement packet.

        :returns: WTP acknowledgement packet data
        """
        stream = self._build_header(consts.WTP_PKT_ACK)
        # Sequence number
        stream.write_data("H", self._rx_ctrl.seq_num)
        return stream.getvalue()
    def _handle_packet(self, stream, packet_type):
        """
        Handle WTP packet for current connection.

        :param stream: Data stream containing WTP packet
        :param packet_type: Packet type
        """
        handler = self._pkt_handler.get(packet_type)
        # Drop packet with unknown packet type
        if handler:
            handler(self, stream)
    def _handle_open(self, stream):
        """
        Handle WTP open packet.

        :param stream: Data stream containing open packet
        """
        # Update connection state
        self.uplink_state = consts.WTP_STATE_OPENED
        self.downlink_state = consts.WTP_STATE_OPENING
        # Send acknowledgement packet
        self._tx_ctrl.add_packet(self._build_ack())
        # Send open packet
        open_stream = self._build_header(consts.WTP_PKT_OPEN)
        self._tx_ctrl.add_packet(open_stream.getvalue())
        # Request sending AccessSpec
        self._request_access_spec()
    def _handle_close(self, stream):
        """
        Handle WTP close packet.

        :param stream: Data stream containing close packet
        """
        # Update connection information
        self.uplink_state = WTP_STATE_CLOSED
        # Trigger half close or close event
        if self.downlink_state==WTP_STATE_OPENED:
            self.trigger("half-close")
        elif self.downlink_state==WTP_STATE_CLOSED:
            self.trigger("close")
        # Send Ack message
        self._tx_ctrl.add_packet(self._build_ack())
    def _handle_ack(self, stream):
        """
        Handle WTP acknowledgement packet.

        :param stream: Data stream containing acknowledgement packet
        """
        # Read acknowledged bytes
        seq_num = stream.read_data("H")
        # Downlink opened
        if self.downlink_state==consts.WTP_STATE_OPENING and seq_num==0:
            self.downlink_state = consts.WTP_STATE_OPENED
        # Downlink closed
        elif self.downlink_state==consts.WTP_STATE_CLOSING:
            self.downlink_state = consts.WTP_STATE_CLOSED
        # Handle acknowledgement with transmit control tool
        else:
            n_sent_msgs = self._tx_ctrl.handle_ack(seq_num)
            # Resolve send deferreds
            for _ in range(n_sent_msgs):
                self._send_deferreds.pop(0).callback(None)
    def _handle_data_packet(self, stream, msg_begin):
        """
        Handle WTP message data packet.

        :param stream: Data stream containing continue message packet
        :param begin: Is this packet a begin message packet?
        """
        # Read message size
        msg_size = stream.read_data("H") if msg_begin else None
        # Read sequence number and payload size
        seq_num, payload_size = stream.read_data("HB")
        # Read payload
        payload = stream.read(payload_size)
        # Handle received data with receive control tool
        new_msgs = self._rx_ctrl.handle_packet(seq_num, payload, msg_size)
        # Resolve remaining deferreds
        while self._recv_deferreds and new_msgs:
            d = self._recv_deferreds.pop(0)
            d.callback(new_msgs.pop(0))
        # Store remaining messages
        if new_msgs:
            self._recv_msgs += new_msgs
        # Send acknowledgement
        self._tx_ctrl.add_packet(self._build_ack())
    def _handle_req_uplink(self, stream):
        """
        Handle WTP request uplink packet.

        :param stream: Data stream containing request uplink packet
        """
        # Number of read operations and read OpSpec size
        n_reads, read_size = stream.read_data("BB")
        # Add read OpSpecs
        self._read_opspec_sizes += [read_size]*n_reads
        # Request sending AccessSpec
        self._request_access_spec()
    def _request_access_spec(self):
        """
        Request sending AccessSpec to WISP.
        """
        # Ongoing AccessSpec
        if self._ongoing_access_spec:
            return
        self._ongoing_access_spec = True
        # Collect OpSpecs for sending
        opspecs = []
        opspec_id = 0
        while True:
            # Add a Read OpSpec
            if self._read_opspec_sizes:
                opspecs.append(read_opspec(self._read_opspec_sizes.pop(0), opspec_id))
                # Update OpSpec ID
                opspec_id += 1
            if opspec_id>=consts.LLRP_N_OPSPECS_MAX:
                break
            # Add a Write OpSpec
            write_data = self._tx_ctrl.get_write_data()
            if write_data:
                opspecs.append(write_opspec(write_data, opspec_id))
                # Update OpSpec ID
                opspec_id += 1
            if opspec_id>=consts.LLRP_N_OPSPECS_MAX:
                break
            # No more OpSpec to add
            if not self._read_opspec_sizes and not write_data:
                break
        # Send AccessSpec and schedule next sending
        if opspecs:
            d = self.server._send_access_spec(self.wisp_id, opspecs)
            # Send AccessSpec callback
            def send_access_spec_cb(_):
                # Set ongoing AccessSpec flag
                self._ongoing_access_spec = False
                # Next AccessSpec sending
                self._request_access_spec()
            d.addCallback(send_access_spec_cb)
        # No more OpSpecs to send, stop
        else:
            self._ongoing_access_spec = False
    def send(self, msg_data):
        """
        Send a message to WISP.

        :param msg_data: Message data
        """
        # Add message to transmit control
        self._tx_ctrl.add_msg(msg_data)
        # Request sending AccessSpec
        self._request_access_spec()
        # Add a new deferred
        d = Deferred()
        self._send_deferreds.append(d)
        return d
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
            self._recv_deferreds.append(d)
        return d
    def close(self):
        """
        Close WTP connection with WISP.
        """
        # TODO: Close connection
        pass
    # Packet handlers
    _pkt_handler = {
        consts.WTP_PKT_OPEN: _handle_open,
        consts.WTP_PKT_CLOSE: _handle_close,
        consts.WTP_PKT_ACK: _handle_ack,
        consts.WTP_PKT_BEGIN_MSG: functools.partial(_handle_data_packet, msg_begin=True),
        consts.WTP_PKT_CONT_MSG: functools.partial(_handle_data_packet, msg_begin=False),
        consts.WTP_PKT_REQ_UPLINK: _handle_req_uplink
    }
