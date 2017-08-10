from __future__ import absolute_import, unicode_literals
import functools, struct
from abc import ABC, abstractproperty
from six import iteritems
from sllurp.llrp import LLRPClientFactory
from urpc import URPC, urpc_sig, StringType, urpc_type_repr, VARY
from wtp import WTPServer

from wisp_ert.util import not_implemented

class Service(ABC):
    """ The WISP extended runtime service class. """
    @abstractproperty
    def constants(self):
        """
        Get C constants provided by this service.

        :returns: A list of constants and their u-RPC low-level types.
        """
        not_implemented()
    @abstractproperty
    def functions(self):
        """
        Get C functions provided by this service.

        :returns: A mapping from function names to functions
        """
        not_implemented()

class Runtime(object):
    """ The WISP extended runtime class. """
    def __init__(self, antennas=[1], n_tags_per_report=5, **kwargs):
        """
        Runtime constructor.

        :param antennas: Antennas to be enabled
        :param n_tags_per_report: Report every N tags
        """
        # Services
        self._services = {}
        # WTP connection to services mapping
        self._clients = {}
        # LLRP factory
        factory = self._llrp_factory = LLRPClientFactory(
            antennas=antennas,
            modulation="WISP5",
            report_every_n_tags=n_tags_per_report,
            start_inventory=True
        )
        # WTP endpoint
        wtp_ep = self._wtp_ep = WTPServer(
            llrp_factory=factory
        )
        # Add connect event handler
        wtp_ep.on("connect", self._handle_new_client)
    def _handle_new_client(self, connection):
        """
        Handle new WISP client.

        :param connection: New WTP connection
        """
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
        for name, service_factory in iteritems(self._services):
            service = service_factory()
            # Add functions to u-RPC endpoint
            for name, func in iteritems(service.functions):
                rpc_ep.add_func(func=func, name=name)
        # Add RPC endpoint
        service_insts["_rpc"] = rpc_ep
        # Add to runtime client table
        self._services[connection] = service_insts
    def _service_constants(self, connection, name):
        """
        Get service C constants by service name.

        :param connection: WISP client WTP connection
        :param name: Service name
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
    def add_service(self, name, factory, *args, **kwargs):
        """
        Add a service class to runtime.

        :param name: Name of the service
        :param factory: Service instance factory
        :param args: Arguments to be passed to service factory
        :param kwargs: Keyword arguments to be passed to service factory
        """
        factory = functools.partial(factory, *args, **kwargs)
        # Add to runtime services mapping
        self._services[name] = factory
    def start(self, server, port):
        """
        Start WISP extended runtime.

        :param server: LLRP reader IP or domain name
        :param port: LLRP reader port
        """
        self._wtp_ep.start(
            server=server,
            port=port
        )
    def stop(self):
        """
        Stop WISP extended runtime.
        """
        self._wtp_ep.stop()
