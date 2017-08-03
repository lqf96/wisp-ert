from __future__ import unicode_literals, absolute_import

class WTPError(BaseException):
    def __init__(self, reason):
        self.reason = reason
