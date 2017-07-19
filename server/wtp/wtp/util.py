class ChecksumTool(object):
    """ Checksum tool for reading from and writing to stream. """
    def __init__(self):
        pass
    def update(self, data):
        pass
    def validate(self, checksum):
        pass
    def checksum(self):
        pass
    def reset(self):
        pass

def read_stream(stream, format, checksum_tool=None):
    pass

def write_stream(stream, format, *args, **kwargs):
    pass
