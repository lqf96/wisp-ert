from __future__ import unicode_literals, absolute_import

class WTPError(Exception):
    """ WTP error class. """
    def __init__(self, reason):
        self.reason = reason
