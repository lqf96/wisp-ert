from __future__ import absolute_import, unicode_literals
from twisted.internet.deferred import deferLater

import wtp.constants as consts
from wtp.util import EventTarget, ChecksumStream, xor_checksum, read_stream, write_stream
from wtp.llrp_util import read_opspec, write_opspec, wisp_target_info
from wtp.connection import WTPConnection

class WTPServer(EventTarget):
    """ WTP server type. """
    def __init__(self, llrp_factory):
        # Initialize base classes
        super(WTPServer, self).__init__()
        # LLRP factory
        self._llrp_factory = llrp_factory
        # Last seen EPC data
        self._last_epc = {}
        # WTP connections
        self._connections = {}
        # Checksum
        self._checksum = 0
        # TODO: Reactor
        self._reactor = None
    def _handle_tag_report(self, llrp_msg):
        """
        Handle LLRP tag report from reader.

        :param llrp_msg: LLRP tag report message
        """
        reports = llrp_msg["RO_ACCESS_REPORT"]["TagReportData"]
        for report in reports:
            # Get and parse EPC data
            epc_data = report["EPC-96"]
            stream = ChecksumStream(epc_data)
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
                    stream = ChecksumStream(read_data, checksum_func=util.xor_checksum)
                    self._handle_packets(stream, wisp_id)
            # TODO: Send WTP packets?
            pass
    def _handle_packets(self, stream, wisp_id):
        """
        Handle WTP packets.

        :param stream: Data stream containing WTP packets
        :param wisp_id: WISP ID
        """
        # Get WTP connection
        connection = self._connections.get(wisp_id)
        # Read packets
        while True:
            # Reset checksum
            self._reset_checksum()
            # Packet type
            packet_type = util.read_stream(stream, ">B")
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
                # Trigger connect event
                self.trigger("connect", connection)
            # Do not process packets without corresponding connection
            if not connection:
                break
            # Handle packet in connection
            connection._handle_packet(stream, packet_type)
            # Close connection
            if packet_type==WTP_PKT_CLOSE:
                # Remove connection object when fully closed
                if connection._uplink_state==WTP_STATE_CLOSED and connection._downlink_state==WTP_STATE_CLOSED:
                    del self._connections[wisp_id]
    def _set_timeout(self, time, func, *args, **kwargs):
        """
        Call function after given timeout.

        :param time: Timeout
        :param func: Function to call
        :param args: Arguments
        :param kwargs: Keyword arguments
        :returns: A deferred object which will be resolved when the function is executed
        """
        return deferLater(self._reactor, time, func, *args, **kwargs)
    def _send_opspecs(self, wisp_id, opspecs):
        """
        Send OpSpecs to WISP.

        :param wisp_id: WISP ID
        :param opspecs: OpSpecs to send
        """
        # Ensure OpSpecs is a list
        if not isinstance(opspecs, Container):
            opspecs = [opspecs]
        # TODO: Sending multiple OpSpecs at once
        if len(opspecs)>1:
            return
        # TODO: AccessSpec callback
        self._llrp_factory.nextAccess()