import secrets

from . import constants

utf8 = "utf-8"

def generate_entropy_hex_bytes_128():
    """ return 128 entropy hex bytes
    :param
    :returns: 128 hex bytes
    :rtype: bytes
    """

    size = constants.ENTROPY_SIZE_128 / 8
    entropy = secrets.token_hex(int(size))
    return entropy.encode(utf8)

def generate_entropy_hex_bytes_256():
    """ return 256 entropy hex bytes
    :param
    :returns: 256 hex bytes
    :rtype: bytes
    """
    size = constants.ENTROPY_SIZE_256 / 8
    entropy = secrets.token_hex(int(size))
    return entropy.encode(utf8)

def convert_to_bytes(source):
    """ convert [str|bytes] to bytes
    :params str|bytes source
    :returns:
    :rtype: bytes
    """
    assert isinstance(source, (str, bytes)), type(source)

    if isinstance(source, str):
        return source.encode(utf8)
    else:
        return source

def convert_to_str(source):
    """ convert str|bytes to string
    :param str|bytes source
    :returns:
    :rtype: str
    """
    assert isinstance(source, (str, bytes)), type(source)

    if isinstance(source, bytes):
        return source.decode(utf8)
    else:
        return source

    
