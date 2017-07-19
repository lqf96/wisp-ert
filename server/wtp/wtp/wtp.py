from __future__ import absolute_import, unicode_literals
from io import BytesIO

from wtp.llrp_util import read_opspec, write_opspec, wisp_target_info
from wtp.util import ChecksumTool, read_stream, write_stream, read_compare_checksum

# Unreliable mode
WTP_MODE_UNRELIABLE = 0x00
# Reliable stream mode
WTP_MODE_STREAM = 0x01
# Reliable datagram mode
WTP_MODE_DGRAM = 0x02

# No more packets
WTP_PKT_END = 0x00
# Open WTP connection
WTP_PKT_OPEN = 0x01
# Close WTP connection
WTP_PKT_CLOSE = 0x02
# Acknowledgement
WTP_PKT_ACK = 0x03
# Begin message
WTP_PKT_BEGIN_MSG = 0x04
# Continue message
WTP_PKT_CONT_MSG = 0x05
# Request uplink transfer
WTP_PKT_REQ_UPLINK = 0x06
# Protocol error
WTP_PKT_ERR = 0x07

class WTPServer(object):
    def __init__(self):
        # Last seen EPC data
        self._last_epc = {}
        # WTP connections
        self._connections = {}
        # Checksum tool
        self._checksum_tool = ChecksumTool()
    def _handle_tag_report(self, llrp_msg):
        """
        Handle LLRP tag report from reader.

        :param llrp_msg: LLRP tag report message
        """
        reports = llrp_msg["RO_ACCESS_REPORT"]["TagReportData"]
        for report in reports:
            # Get and parse EPC data
            epc_data = report["EPC-96"]
            stream = BytesIO(epc_data)
            wisp_id = read_stream(stream, ">H")
            # Read and handle WTP packets from EPC data only when EPC changed
            if self._last_epc.get(wisp_id)!=epc_data:
                self._last_epc[wisp_id] = epc_data
                self._handle_packets(stream, wisp_id)
            # OpSpec result present
            opspec_result = report.get("OpSpecResult")
            if opspec_result:
                opspec_id = opspec_result["OpSpecID"]
                # Read data result
                read_data = opspec_result.get("ReadData")
                if read_data:
                    stream = BytesIO(read_data)
                    self._handle_packets(stream, wisp_id, self._checksum_tool)
            # TODO: Send WTP packets?
            pass
    def _handle_packets(self, stream, wisp_id, checksum_tool=None):
        """
        Handle WTP packets.

        :param stream: Data stream containing WTP packets
        :param wisp_id: WISP ID
        :param checksum_tool: Checksum tool
        """
        # Get WTP connection
        connection = self._connections.get(wisp_id)
        # Read packets
        while True:
            # Reset checksum tool
            if checksum_tool:
                checksum_tool.reset()
            # Packet type
            packet_type = read_stream(stream, ">B", checksum_tool)
            # No more packets
            if packet_type==WTP_PKT_END:
                break
            # Open connection
            if packet_type==WTP_PKT_OPEN:
                # Create connection object
                if not connection:
                    connection = self._connections[wisp_id] = WTPConnection(
                        server=self,
                        wisp_id=wisp_id
                    )
            # Do not process packets without corresponding connection
            if not connection:
                break
            # Handle packet in connection
            connection._handle_packet(stream, packet_type, checksum_tool)
            # Close connection
            if packet_type==WTP_PKT_CLOSE:
                # Remove connection object when fully closed
                if not connection._uplink_open and not connection._downlink_open:
                    del self._connections[wisp_id]

class WTPConnection(object):
    def __init__(self, server, wisp_id):
        # WTP server reference
        self._server = server
        # WISP ID
        self._wisp_id = wisp_id
        # Downlink and uplink status
        self._uplink_open = False
        self._downlink_open = False
        # Connection mode
        self._mode = None
        # Temporary message data
        self._tmp_msg_data = b""
        # Sliding window size
        self._sliding_window = 128
        # Acknowledged received bytes and sliced received data
        self._recv_acked = 0
        self._recv_data = {}
    def _handle_packet(self, stream, packet_type, checksum_tool):
        """
        Handle WTP packet for current connection.

        :param stream: Data stream containing WTP packet
        :param packet_type: Packet type
        :param checksum_tool: Checksum tool
        """
        handler = self._pkt_handler.get(packet_type)
        # Drop packet with unknown packet type
        if handler:
            handler(self, stream, checksum_tool)
    def _handle_open(self, stream, checksum_tool):
        """
        Handle WTP open packet.

        :param stream: Data stream containing open packet
        :param checksum_tool: Checksum tool
        """
        # Read connection mode
        mode = read_stream(stream, ">B", checksum_tool)
        # Validate checksum
        read_compare_checksum(stream, checksum_tool)
        # Update connection information
        self._mode = mode
        self._uplink_open = True
        # Send Ack message
        self._send_packet(self._build_ack())
    def _handle_close(self, stream, checksum_tool):
        """
        Handle WTP close packet.

        :param stream: Data stream containing close packet
        :param checksum_tool: Checksum tool
        """
        # Validate checksum
        read_compare_checksum(stream, checksum_tool)
        # Update connection information
        self._uplink_open = False
        # Send Ack message
        self._send_packet(self._build_ack())
    def _handle_ack(self, stream, checksum_tool):
        """
        Handle WTP acknowledgement packet.

        :param stream: Data stream containing acknowledgement packet
        :param checksum_tool: Checksum tool
        """
        # Read acknowledged bytes
        acked = read_stream(stream, ">H", checksum_tool)
        # Validate checksum
        read_compare_checksum(stream, checksum_tool)
        # TODO: What to do next
        pass
    def _handle_begin_msg(self, stream, checksum_tool):
        """
        Handle WTP begin message packet.

        :param stream: Data stream containing begin message packet
        :param checksum_tool: Checksum tool
        """
        # Invalid under reliable stream mode
        if self._mode==WTP_MODE_STREAM:
            raise WTPError(WTP_ERR_INVALID_OP)
        # Read offset, message size and payload size
        offset, msg_size, payload_size = read_stream(stream, ">HHB", checksum_tool)
        # Read payload data
        payload = read_stream_vary(stream, payload_size, checksum_tool)
        # Validate checksum
        read_compare_checksum(stream, checksum_tool)
        # TODO: What to do next
        pass
    def _handle_cont_msg(self, stream, checksum_tool):
        """
        Handle WTP continue message packet.

        :param stream: Data stream containing continue message packet
        :param checksum_tool: Checksum tool
        """
        # Read offset and payload size
        offset, payload_size = read_stream(stream, ">HB", checksum_tool)
        # Read payload data
        payload = read_stream_vary(stream, payload_size, checksum_tool)
        # Validate checksum
        read_compare_checksum(stream, checksum_tool)
        # TODO: What to do next
        pass
    def _handle_req_uplink(self, stream, checksum_tool):
        """
        Handle WTP request uplink packet.

        :param stream: Data stream containing request uplink packet
        :param checksum_tool: Checksum tool
        """
        # Number of READ operations requests
        n_reads = read_stream(stream, ">B", checksum_tool)
        # Validate checksum
        read_compare_checksum(stream, checksum_tool)
        # TODO: Send Read opspecs
    # Packet type to handler mapping
    _pkt_handler = {
        WTP_PKT_OPEN: _handle_open,
        WTP_PKT_CLOSE: _handle_close,
        WTP_PKT_ACK: _handle_ack,
        WTP_PKT_BEGIN_MSG: _handle_begin_msg,
        WTP_PKT_CONT_MSG: _handle_cont_msg,
        WTP_PKT_REQ_UPLINK: _handle_req_uplink
    }
