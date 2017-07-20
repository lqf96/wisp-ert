from __future__ import unicode_literals, absolute_import
from collections import Container
import struct

from llrp import llrp_proto

def encode_access_cmd(param):
    # Access command message type
    access_cmd_type = llrp_proto.Message_struct['AccessCommand']['type']
    # Message header format and length
    msg_header = '!HH'
    msg_header_len = struct.calcsize(msg_header)
    # Encode tag specification
    data = llrp_proto.encode_C1G2TagSpec(param['TagSpecParameter'])
    # Wrap single OpSpec in list
    opspec_param = param['OpSpecParameter']
    if not isinstance(opspec_param, Container):
        opspec_param = [opspec_param]
    # Encode OpSpec parameters
    for opspec in opspec_param:
        if 'WriteData' in opspec:
            # BlockWrite
            if opspec['WriteDataWordCount']>1:
                data += llrp_proto.encode_C1G2BlockWrite(opspec)
            # Write
            else:
                data += llrp_proto.encode_C1G2Write(opspec)
        # Lock
        elif 'LockPayload' in opspec:
            data += llrp_proto.encode_C1G2Lock(opspec)
        # Read
        else:
            data += llrp_proto.encode_C1G2Read(opspec)
    # Pack and return data
    data = struct.pack(msg_header, access_cmd_type, len(data)+msg_header_len)+data
    return data

def patch_all():
    # Encode access command
    llrp_proto.encode_AccessCommand = encode_access_cmd
