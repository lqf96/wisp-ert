from __future__ import unicode_literals, absolute_import

class WTPError(Exception):
    """!
    @brief WTP error class.
    """
    def __init__(self, reason):
        """!
        @brief WTP error constructor.

        @param reason Reason of error.
        """
        ## Reason of error
        self.reason = reason
