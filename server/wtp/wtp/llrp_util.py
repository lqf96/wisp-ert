from __future__ import absolute_import, unicode_literals
import struct

import wtp.constants as consts
from wtp.error import WTPError

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
        raise WTPError(consts.WTP_ERR_INVALID_SIZE)
    # Read OpSpec
    return {
        "OpSpecID": opspec_id,
        # Memory bank
        "MB": consts.RFID_MB_USER,
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
    n_words, remainder = divmod(len(data), 2)
    # Write data padding
    if remainder!=0:
        n_words += 1
        data += b"\x00"
    # BlockWrite OpSpec
    return {
        "OpSpecID": opspec_id,
        # Memory bank
        "MB": consts.RFID_MB_USER,
        # Write position
        "WordPtr": 0,
        # Access password (Dummy)
        "AccessPassword": 0,
        # Data to write
        "WriteData": data,
        # Number of words to write
        "WriteDataWordCount": n_words
    }

def wisp_target_info(wisp_id, wisp_class=consts.WISP_CLASS):
    """
    Generate LLRP RFID target information from WISP ID.

    :param wisp_id 16-bit WISP ID
    :returns: LLRP RFID target information
    """
    return {
        # Memory bank
        "MB": consts.RFID_MB_EPC,
        # ?
        "M": 1,
        # Beginning address of EPC data in EPC memory bank
        "Pointer": 0x20,
        # Tag data mask
        "TagMask": b"\xff"*struct.calcsize("<BH"),
        # Length of mask in bits
        "MaskBitCount": 16,
        # Tag data for selection
        "TagData": struct.pack("<BH", wisp_class, wisp_id),
        # Length of data in bits
        "DataBitCount": 16
    }

def access_stop_param(trigger_type=1, count=1):
    """
    Generate AccessSpec stop parameter.

    :param trigger_type: Trigger type
    :param count: AccessSpec result count
    :returns: AccessSpec stop parameter
    """
    return {
        # Trigger type
        "AccessSpecStopTriggerType": trigger_type,
        # Count
        "OperationCountValue": count
    }
