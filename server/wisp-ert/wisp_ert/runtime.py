from __future__ import absolute_import, unicode_literals
import functools, struct, logging
from abc import ABC, abstractproperty
from six import iteritems
from sllurp.llrp import LLRPClientFactory
from urpc import URPC, urpc_sig, StringType, urpc_type_repr, VARY
from wtp import WTPServer

from wisp_ert.util import not_implemented

## Module logger
_logger = logging.getLogger(__name__)
# Logger level
_logger.setLevel(logging.DEBUG)

class Service(ABC):
    """!
    @brief The WISP extended runtime service base class.
    """
    @abstractproperty
    def constants(self):
        """!
        @brief Get C constants provided by this service.

        @return A list of constants and their u-RPC low-level types.
        """
        not_implemented()
    @abstractproperty
    def functions(self):
        """!
        @brief Get C functions provided by this service.

        @return A mapping from function names to functions
        """
        not_implemented()

class Runtime(object):
    """!
    @brief The WISP extended runtime class.
    """
    def __init__(self, antennas=[1], n_tags_per_report=5, **kwargs):
        """!
        @brief The WISP extended runtime constructor.

        @param antennas Antennas to be enabled.
        @param n_tags_per_report Number of tags per tag report.
        @param kwargs Other arguments.
        """
        ## Services
        self._services = {}
        ## Services factory
        self._services_factory = {}
        ## WTP connection to services mapping
        self._clients = {}
        ## WTP endpoint
        wtp_ep = self._wtp_ep = WTPServer(
            antennas=antennas,
            n_tags_per_report=n_tags_per_report
        )
        # Add connect event handler
        wtp_ep.on("connect", self._handle_new_client)
    def _handle_new_client(self, connection):
        """!
        @brief Handle new WISP client.

        @param connection New WTP connection.
        """
        _logger.debug("New WISP ERT client: #%d", connection.wisp_id)
        # Create u-RPC endpoint for new client
        rpc_ep = URPC(
            send_callback=connection.send
        )
        # Add service constants query function
        rpc_ep.add_func(
            func=functools.partial(Runtime._service_constants, self, connection),
            arg_types=[StringType],
            ret_types=[VARY],
            name="ert_srv_consts"
        )
        # Services instances for new client
        service_insts = {}
        for name, service_factory in iteritems(self._services_factory):
            service = service_factory()
            # Add service to instances
            service_insts[name] = service
            # Add functions to u-RPC endpoint
            for name, func in iteritems(service.functions):
                rpc_ep.add_func(func=func, name=name)
        # Add RPC endpoint
        service_insts["_rpc"] = rpc_ep
        # Add to runtime client table
        self._services[connection] = service_insts
        # Start receiving messages from WTP endpoint
        wtp_recv_cb = functools.partial(Runtime._wtp_recv_cb, self, connection)
        connection.recv().addCallback(wtp_recv_cb)
    def _service_constants(self, connection, name):
        """!
        @brief Get service C constants by service name.

        @param connection WTP connection.
        @param name Service name.
        @return Respective service C constants in binary format.
        """
        # Get service constants
        service = self._services[connection][name]
        # Pack service constants
        consts_repr = "<"
        consts_list = []
        for const, urpc_type in service.constants:
            consts_repr += urpc_type_repr[urpc_type]
            consts_list.append(const)
        # Pack constants into binary formats
        return struct.pack(consts_repr, *consts_list)
    def _wtp_recv_cb(self, connection, data):
        """!
        @brief WTP on message received callback.

        @param connection WTP connection.
        @param data Received data.
        """
        # Call u-RPC endpoint
        rpc_ep = self._services[connection]["_rpc"]
        rpc_ep.recv_callback(data)
        # Wait for next message
        wtp_recv_cb = functools.partial(Runtime._wtp_recv_cb, self, connection)
        connection.recv().addCallback(wtp_recv_cb)
    def add_service(self, name, factory, *args, **kwargs):
        """!
        @brief Add a service class to runtime.

        @param name Name of the service.
        @param factory Service instance factory.
        @param args Arguments to be passed to service factory.
        @param kwargs Keyword arguments to be passed to service factory.
        """
        factory = functools.partial(factory, *args, **kwargs)
        # Add to runtime services factories
        self._services_factory[name] = factory
    def start(self, server, port):
        """!
        @brief Start WISP extended runtime.

        @param server LLRP reader IP or domain name.
        @param port LLRP reader port.
        """
        self._wtp_ep.start(
            server=server,
            port=port
        )
    def stop(self):
        """!
        @brief Stop WISP extended runtime.
        """
        self._wtp_ep.stop()
