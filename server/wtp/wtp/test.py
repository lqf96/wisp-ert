# Minimal example; see inventory.py for more.
from __future__ import absolute_import, unicode_literals
from sllurp.llrp import LLRPClientFactory
from twisted.internet import reactor
from sllurp.log import init_logging

from wtp.core import WTPServer
from wtp.connection import WTPConnection
import wtp.constants as consts

def test_send():
    print(factory.protocols)
    c.send(b"test")

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
# Connection (temp)
c = server._connections[0] = WTPConnection(server, 0)
c._downlink_state = consts.WTP_STATE_OPENED
c._uplink_state = consts.WTP_STATE_OPENED

reactor.callLater(2, test_send)

# Start server
server.start("192.168.10.15")
