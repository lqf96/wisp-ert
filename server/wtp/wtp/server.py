from __future__ import absolute_import, unicode_literals
import logging
from binascii import unhexlify
from twisted.internet import reactor as inet_reactor
from twisted.internet.defer import Deferred
from sllurp.llrp import LLRPClientFactory, LLRP_PORT

import wtp.constants as consts
from wtp.util import EventTarget, ChecksumStream, xor_checksum
from wtp.llrp_util import read_opspec, write_opspec, wisp_target_info, access_stop_param
from wtp.connection import WTPConnection
from wtp.error import WTPError

## Module logger
_logger = logging.getLogger(__name__)
# Logger level
_logger.setLevel(logging.DEBUG)

class WTPServer(EventTarget):
    """!
    @brief WTP server class.
    """
    def __init__(self, antennas=[1], n_tags_per_report=1, reactor=inet_reactor):
        """!
        @brief WTP server constructor.

        @param antennas Antennas to be enabled.
        @param n_tags_per_report Number of tags per tag report.
        @param reactor Twisted reactor.
        """
        # Initialize base classes
        super(WTPServer, self).__init__()
        ## LLRP factory
        llrp_factory = self._llrp_factory = LLRPClientFactory(
            antennas=antennas,
            report_every_n_tags=n_tags_per_report,
            modulation="WISP5",
            start_inventory=True
        )
        ## Previous seen EPC data
        self._prev_epcs = {}
        ## WTP connections
        self._connections = {}
        ## Twisted reactor
        self._reactor = reactor
        ## AccessSpec deferreds
        self._access_spec_deferreds = {}
        ## Add tag report callback
        llrp_factory.addTagReportCallback(self._handle_tag_report)
    def start(self, server, port=LLRP_PORT):
        """!
        @brief Start the WTP server.

        @param server Reader IP or domain name.
        @param port Reader port.
        """
        # Connect to reader
        self._reactor.connectTCP(server, port, self._llrp_factory)
        # Run reactor
        self._reactor.run()
    def stop(self):
        """!
        @brief Stop the WTP server.
        """
        self._reactor.stop()
    def _handle_tag_report(self, llrp_msg):
        """!
        @brief Handle LLRP tag report from reader.

        @param llrp_msg LLRP message.
        """
        reports = llrp_msg.msgdict["RO_ACCESS_REPORT"]["TagReportData"]
        for report in reports:
            # Get and parse EPC data
            epc_hex = report["EPC-96"]
            epc_data = bytearray(unhexlify(epc_hex))
            stream = ChecksumStream(epc_data)
            wisp_id, wisp_class = stream.read_data("BB")
            # Ignore non-WISP devices
            if wisp_class!=consts.RFID_WISP_CLASS:
                continue
            # Read and handle WTP packets from EPC data only when EPC changed
            prev_epcs = self._prev_epcs.get(wisp_id)
            if not prev_epcs:
                prev_epcs = [bytearray(consts.RFID_EPC_SIZE)]*consts.WTP_PREV_EPC_SIZE
                self._prev_epcs[wisp_id] = prev_epcs
            if epc_data not in prev_epcs:
                _logger.debug("New EPC %s for WISP #%d", epc_hex, wisp_id)
                # Update previous EPCs
                prev_epcs.pop(0)
                prev_epcs.append(epc_data)
                # Handle packets inside EPC
                self._handle_packets(stream, wisp_id)
            # OpSpec results
            opspec_results = report.get("OpSpecResult")
            if not opspec_results:
                continue
            _logger.debug("Tag report with OpSpec result: %s", report)
            # Ensure OpSpec results is container
            if not isinstance(opspec_results, list):
                opspec_results = [opspec_results]
            # Resolve pending AccessSpec deferreds
            d = self._access_spec_deferreds.pop(wisp_id, None)
            if d:
                d.callback(opspec_results)
            # Handle Read
            for opspec_result in opspec_results:
                # OpSpec result status
                op_status = opspec_result["Result"]
                # Read data result
                read_data = opspec_result.get("ReadData")
                if op_status==0 and read_data:
                    stream = ChecksumStream(read_data)
                    self._handle_packets(stream, wisp_id)
    def _handle_packets(self, stream, wisp_id):
        """!
        @brief Handle WTP packets.

        @param stream Data stream containing WTP packets.
        @param wisp_id WISP ID.
        """
        # Get WTP connection
        connection = self._connections.get(wisp_id)
        # Read packets
        while True:
            # Packet type
            packet_type = stream.read_data("B")
            # No more packets
            if packet_type==None or packet_type==consts.WTP_PKT_END:
                break
            # Open connection
            elif packet_type==consts.WTP_PKT_OPEN:
                # Do nothing if connection already established
                if not connection:
                    # Create new connection
                    connection = self._connections[wisp_id] = WTPConnection(
                        server=self,
                        wisp_id=wisp_id,
                        checksum_func=xor_checksum,
                        checksum_type="B"
                    )
                    # Handle packet in connection
                    connection._handle_packet(stream, packet_type)
                    # Trigger connect event
                    self.trigger("connect", connection)
            # Do not process packets without corresponding connection
            elif connection:
                # Handle packet in connection
                connection._handle_packet(stream, packet_type)
                # Close connection
                if packet_type==consts.WTP_PKT_CLOSE:
                    # Remove connection object when fully closed
                    if connection.uplink_state==consts.WTP_STATE_CLOSED and connection.downlink_state==consts.WTP_STATE_CLOSED:
                        del self._connections[wisp_id]
    def _send_access_spec(self, wisp_id, opspecs):
        """!
        @brief Send AccessSpec to WISP.

        @param wisp_id WISP ID.
        @param opspecs OpSpecs to send.
        @throws WTPError If there are ongoing AccessSpec for current WISP ID.
        """
        # Ongoing AccessSpec for current WISP ID
        if wisp_id in self._access_spec_deferreds:
            raise WTPError(consts.WTP_ERR_ONGOING_ACCESS_SPEC)
        # Ensure OpSpecs is a list
        if not isinstance(opspecs, list):
            opspecs = [opspecs]
        # Get LLRP client
        proto = self._llrp_factory.protocols[0]
        # Do next access
        access_deferred = proto.nextAccess(
            stopSpecPar=access_stop_param(),
            # Use WISP ID as AccessSpec ID
            accessSpecID=wisp_id,
            param=opspecs,
            target=wisp_target_info(wisp_id)
        )
        # Return deferred object
        d = Deferred()
        # Chain deferreds in case of error
        access_deferred.addErrback(d.errback)
        # Add to deferreds mapping
        self._access_spec_deferreds[wisp_id] = d
        return d
