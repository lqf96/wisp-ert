# Minimal example; see inventory.py for more.
from __future__ import absolute_import, unicode_literals
from sllurp import llrp
from twisted.internet import reactor
from wtp.llrp_util import read_opspec, write_opspec, wisp_target_info
import logging, json, sys

from sllurp.log import init_logging

#init_logging(True, False)

def tags_report(report):
    data = report.msgdict['RO_ACCESS_REPORT']['TagReportData']
    for t in data:
        if t["EPC-96"].startswith(b"00"):
            print(t)

def test_blockwrite(proto):
    print("test blockwrite")
    return proto.startAccess(
        readWords=None,
        writeWords=write_opspec(b"test", 1),
        target=wisp_target_info(0)
    )

def test_read(proto):
    print("test read")
    return proto.startAccess(
        readWords=read_opspec(4, 1),
        writeWords=None,
        target=wisp_target_info(0)
    )

factory = llrp.LLRPClientFactory(
    report_every_n_tags=1,
    modulation="WISP5",
    start_inventory=True,
    antennas=[4],
)
factory.addTagReportCallback(tags_report)
factory.addStateCallback(llrp.LLRPClient.STATE_INVENTORYING, test_read)

reactor.connectTCP('192.168.10.15', llrp.LLRP_PORT, factory)
reactor.run()
