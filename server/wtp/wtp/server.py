from __future__ import absolute_import, unicode_literals
from binascii import unhexlify
from twisted.internet import reactor as inet_reactor
from twisted.internet.defer import Deferred
from sllurp.llrp import LLRPClientFactory, LLRP_PORT

import wtp.constants as consts
from wtp.util import EventTarget, ChecksumStream, xor_checksum
from wtp.llrp_util import read_opspec, write_opspec, wisp_target_info, access_stop_param
from wtp.connection import WTPConnection
from wtp.error import WTPError

class WTPServer(EventTarget):
    """ WTP server type. """
    def __init__(self, antennas=[1], n_tags_per_report=5, reactor=inet_reactor):
        # Initialize base classes
        super(WTPServer, self).__init__()
        # LLRP factory
        llrp_factory = self._llrp_factory = LLRPClientFactory(
            antennas=antennas,
            report_every_n_tags=n_tags_per_report,
            modulation="WISP5",
            start_inventory=True
        )
        # Last seen EPC data
        self._last_epc = {}
        # WTP connections
        self._connections = {}
        # Reactor
        self._reactor = reactor
        # AccessSpec deferreds
        self._access_spec_deferreds = {}
        # Add tag report callback
        llrp_factory.addTagReportCallback(self._handle_tag_report)
    def start(self, server, port=LLRP_PORT, reactor=inet_reactor):
        """
        Start the WTP server.

        :param server: Reader
        :param port: Reader port
        """
        # Connect to reader
        self._reactor.connectTCP(server, port, self._llrp_factory)
        # Run reactor
        self._reactor.run()
    def stop(self):
        """
        Stop the WTP server.
        """
        self._reactor.stop()
    def _handle_tag_report(self, llrp_msg):
        """
        Handle LLRP tag report from reader.

        :param llrp_msg: LLRP tag report message
        """
        reports = llrp_msg.msgdict["RO_ACCESS_REPORT"]["TagReportData"]
        for report in reports:
            # Get and parse EPC data
            epc_data = bytearray(unhexlify(report["EPC-96"]))
            stream = ChecksumStream(epc_data)
            wisp_class, wisp_id = stream.read_data("BH")
            # Ignore non-WISP devices
            if wisp_class!=consts.WISP_CLASS:
                continue
            # Read and handle WTP packets from EPC data only when EPC changed
            if self._last_epc.get(wisp_id)!=epc_data:
                self._last_epc[wisp_id] = epc_data
                self._handle_packets(stream, wisp_id)
            # OpSpec results
            opspec_results = report.get("OpSpecResult")
            if not opspec_results:
                continue
            # Resolve pending AccessSpec deferreds
            d = self._access_spec_deferreds.pop(wisp_id, None)
            if d:
                d.callback(opspec_results)
            # Ensure OpSpec results is container
            if not isinstance(opspec_results, list):
                opspec_results = [opspec_results]
            for opspec_result in opspec_results:
                opspec_id = opspec_result["OpSpecID"]
                # Read data result
                read_data = opspec_result.get("ReadData")
                if read_data:
                    stream = ChecksumStream(read_data, checksum_func=xor_checksum)
                    self._handle_packets(stream, wisp_id)
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
            packet_type = stream.read_data("B")
            # No more packets
            if packet_type==consts.WTP_PKT_END:
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
                        checksum_type="<B"
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
        """
        Send AccessSpec to WISP.

        :param wisp_id: WISP ID
        :param opspecs: OpSpecs to send
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
