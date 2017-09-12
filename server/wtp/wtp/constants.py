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
# Set parameter
WTP_PKT_SET_PARAM = 0x07

# === WTP connection states ===
# Closed
WTP_STATE_CLOSED = 0x00
# Opening
WTP_STATE_OPENING = 0x01
# Opened
WTP_STATE_OPENED = 0x02
# Closing
WTP_STATE_CLOSING = 0x03

# === WTP error code ===
# Not acknowledged
WTP_ERR_NOT_ACKED = 0x10
# Required action already done
WIO_ERR_ALREADY = 0x11
# Connection is busy
WTP_ERR_BUSY = 0x12
# Invalid parameter
WTP_ERR_INVALID_PARAM = 0x13
# Invalid checksum
WTP_ERR_INVALID_CHECKSUM = 0x14
# Unsupported operation
WTP_ERR_UNSUPPORT_OP = 0x15
# Invalid size
WTP_ERR_INVALID_SIZE = 0x16
# Ongoing AccessSpec
WTP_ERR_ONGOING_ACCESS_SPEC = 0x17

# === WTP parameter code ===
# Sliding window size
WTP_PARAM_WINDOW_SIZE = 0x00
# Read size
WTP_PARAM_READ_SIZE = 0x01

# === Miscellaneous ===
# WTP max sequence number
WTP_SEQ_MAX = 0x10000
# WTP previous EPC size
WTP_PREV_EPC_SIZE = 3

# WTP initial size per OpSpec
WTP_OPSPEC_INIT = 24
# WISP minimum size per OpSpec
WTP_OPSPEC_MIN = 8
# WISP maximum size per OpSpec
WTP_OPSPEC_MAX = 30

# Maximum OpSpecs in 1 AccessSpec
LLRP_N_OPSPECS_MAX = 1 #4

# RFID WISP class
RFID_WISP_CLASS = 0x51

# RFID EPC size
RFID_EPC_SIZE = 12

# EPC memory bank
RFID_MB_EPC = 0x01
# User memory bank
RFID_MB_USER = 0x03
