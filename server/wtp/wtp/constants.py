from __future__ import absolute_import, unicode_literals

# === WTP packet types ===
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

# === WTP connection states ===
# Closed
WTP_STATE_CLOSED = 0x00
# Opening
WTP_STATE_OPENING = 0x01
# Opened
WTP_STATE_OPENED = 0x02
# Closing
WTP_STATE_CLOSING = 0x03
