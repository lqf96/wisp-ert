from __future__ import absolute_import, unicode_literals
import struct

from wtp.error import WTPError, WTP_ERR_INVALID_SIZE

# EPC memory bank
RFID_MB_EPC = 0x01
# User memory bank
RFID_MB_USER = 0x03

def read_opspec(data_size, opspec_id=0):
    """
    Create a LLRP Read OpSpec.

    :param data_size: Size of data to read
    :param opspec_id: OpSpec ID
    :returns: LLRP Read OpSpec
    """
    # Data size must be multiple of 2
    n_words, remainder = divmod(data_size, 2)
    if remainder!=0:
        raise WTPError(WTP_ERR_INVALID_SIZE)
    # Read OpSpec
    return {
        "OpSpecID": opspec_id,
        # ?
        "M": 1,
        # Memory bank
        "MB": RFID_MB_USER,
        # Read position
        "WordPtr": 0,
        # Access password (Dummy)
        "AccessPassword": 0,
        # Number of words to read
        "WordCount": n_words
    }

def write_opspec(data, opspec_id=0):
    """
    Create a LLRP Write or BlockWrite OpSpec.

    :param data: Data to write
    :param opspec_id: OpSpec ID
    :returns: LLRP Write or BlockWrite OpSpec
    """
    # Data size must be multiple of 2
    n_words, remainder = divmod(len(data), 2)
    if remainder!=0:
        raise WTPError(WTP_ERR_INVALID_SIZE)
    # BlockWrite OpSpec
    return {
        "OpSpecID": opspec_id,
        # ?
        "M": 1,
        # Memory bank
        "MB": RFID_MB_USER,
        # Write position
        "WordPtr": 0,
        # Access password (Dummy)
        "AccessPassword": 0,
        # Data to write
        "WriteData": data,
        # Number of words to write
        "WriteDataWordCount": n_words
    }

def wisp_target_info(wisp_id):
    """
    Generate LLRP RFID target information from WISP ID.

    :param wisp_id 16-bit WISP ID
    :returns: LLRP RFID target information
    """
    return {
        # Memory bank
        "MB": RFID_MB_EPC,
        # Beginning address of EPC data in EPC memory bank
        "Pointer": 0x20,
        # Tag data mask
        "TagMask": b"\xff\xff",
        # Length of mask in bits
        "MaskBitCount": 16,
        # Tag data for selection
        "TagData": struct.pack(">H", wisp_id),
        # Length of data in bits
        "DataBitCount": 16
    }
