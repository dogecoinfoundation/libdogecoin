"""
libdogecoin error
"""

class LibdogecoinBaseException(Exception):
    """ libdogecoin
    """
    def __init__(self, message="libdogecoin base error"):
        """init exception
        :param str message: exception message
        """
        self.message = message

    def __str__(self):
        return self.message


class LibdogecoinAPIException(LibdogecoinBaseException):
    """ exception for call libdogecoin LIBDOGECOIN_API """
    def __init__(self, message="libdogecoin API error"):
        self.message = message

        
class LibdogecoinException(LibdogecoinBaseException):
    """ libdogecoin exception exclude LibdogecoinAPIException """
    def __init__(self, message="libdogecoin error"):
        self.message = message
        
