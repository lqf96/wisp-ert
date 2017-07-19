WTP_ERR_INVALID_SIZE = 0x01

class WTPError(object):
    def __init__(self, reason):
        self.reason = reason
