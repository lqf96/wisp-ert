from __future__ import absolute_import, unicode_literals
from binascii import unhexlify
from collections import Container
from twisted.internet import reactor
from sllurp.llrp import LLRP_PORT

import wtp.constants as consts
from wtp.util import EventTarget, ChecksumStream, xor_checksum
from wtp.llrp_util import read_opspec, write_opspec, wisp_target_info, access_stop_param
from wtp.connection import WTPConnection
from wtp.error import WTPError

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
    def start(self, server, port=LLRP_PORT):
        """
        Start the WTP server.

        :param ip: Reader IP
        :param port: Reader port
        """
        # Add tag report callback
        self._llrp_factory.addTagReportCallback(self._handle_tag_report)
        # Connect to reader
        reactor.connectTCP(server, port, self._llrp_factory)
        # Run reactor
        reactor.run()
    def stop(self):
        """
        Stop the WTP server.
        """
        reactor.stop()
    def _handle_tag_report(self, llrp_msg):
        """
        Handle LLRP tag report from reader.

        :param llrp_msg: LLRP tag report message
        """
        reports = llrp_msg.msgdict["RO_ACCESS_REPORT"]["TagReportData"]
        for report in reports:
            # Get and parse EPC data
            epc_data = unhexlify(report["EPC-96"])
            stream = ChecksumStream(epc_data)
            wisp_id = stream.read_stream("H")
            # Temporaily block non-WISP device
            if wisp_id!=0:
                continue
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
                    stream = ChecksumStream(read_data, checksum_func=xor_checksum)
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
            # Packet type
            packet_type = stream.read_stream("B")
            print(packet_type)
            # No more packets
            if packet_type==consts.WTP_PKT_END:
                break
            # Open connection
            if packet_type==consts.WTP_PKT_OPEN:
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
            if packet_type==consts.WTP_PKT_CLOSE:
                # Remove connection object when fully closed
                if connection._uplink_state==consts.WTP_STATE_CLOSED and connection._downlink_state==consts.WTP_STATE_CLOSED:
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
        return reactor.callLater(time, func, *args, **kwargs)
    def _send_opspecs(self, wisp_id, opspecs):
        """
        Send OpSpecs to WISP.

        :param wisp_id: WISP ID
        :param opspecs: OpSpecs to send
        """
        # Ensure OpSpecs is a list
        if not isinstance(opspecs, Container):
            opspecs = [opspecs]
        # Sending multiple OpSpecs at once
        if len(opspecs)>1:
            raise WTPError(consts.WTP_ERR_UNSUPPORT_OP)
        # TODO: nextAccess
        proto = self._llrp_factory.protocols[0]
        proto.startAccess(
            param=opspecs[0],
            accessStopParam=access_stop_param(),
            target=wisp_target_info(wisp_id)
        )
