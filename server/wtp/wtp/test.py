# Minimal example; see inventory.py for more.
from __future__ import absolute_import, unicode_literals
from sllurp.llrp import LLRPClientFactory
from sllurp.log import init_logging

from wtp.core import WTPServer
from wtp.connection import WTPConnection
import wtp.constants as consts

init_logging()

# Factory
factory = LLRPClientFactory(
    antennas=[4],
    report_every_n_tags=1,
    modulation="WISP5",
    start_inventory=True
)
# Server
server = WTPServer(factory)

# Start server
server.start("192.168.10.15")
