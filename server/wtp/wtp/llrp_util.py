from __future__ import absolute_import, unicode_literals
import struct
from six.moves import range

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
    # Read extra byte when necessary
    if remainder!=0:
        n_words += 1
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
    (A 1-byte data length will be prepended before data)

    :param data: Data to write
    :param opspec_id: OpSpec ID
    :returns: LLRP Write or BlockWrite OpSpec
    """
    # Convert data to byte array
    data = bytearray(data)
    # Prepend data length
    data_len = len(data)
    data.insert(0, data_len)
    # Add a padding byte when necessary
    n_words, remainder = divmod(data_len+1, 2)
    if remainder:
        n_words += 1
        data.append(0)
    # Swap bytes of every word
    for i in range(n_words):
        tmp = data[2*i]
        data[2*i] = data[2*i+1]
        data[2*i+1] = tmp
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
    # Tag data mask format
    tag_mask_fmt = "<BB"
    tag_mask_size = struct.calcsize(tag_mask_fmt)
    # WISP target information
    return {
        # Memory bank
        "MB": consts.RFID_MB_EPC,
        # ?
        "M": 1,
        # Beginning address of EPC data in EPC memory bank
        "Pointer": 0x20,
        # Tag data mask
        "TagMask": b"\xff"*tag_mask_size,
        # Length of mask in bits
        "MaskBitCount": 8*tag_mask_size,
        # Tag data for selection
        "TagData": struct.pack(tag_mask_fmt, wisp_id, consts.WISP_CLASS),
        # Length of data in bits
        "DataBitCount": 8*tag_mask_size
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
