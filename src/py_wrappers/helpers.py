from ctypes import *
import sys
import os


# LOAD SHARED LIBRARY - there has to be a better way to do this
def get_lib(library_file):
    path = os.path.abspath(__file__+"../../../../.libs")
    sys.path.insert(0, path)
    path = os.path.join(path, library_file)

    cdll.LoadLibrary(path)
    return CDLL(path)

# ADDRESS_GEN===================================================
class Dogecoin_chain(Structure):
    _fields_ = [
        ("chainname",                   c_char * 32),
        ("b58prefix_pubkey_address",    c_ubyte),
        ("b58prefix_script_address",    c_ubyte),
        ("b58prefix_secret_address",    c_ubyte),
        ("b58prefix_bip32_privkey",     c_uint32),
        ("b58prefix_bip32_pubkey",      c_uint32),
        ("netmagic",                    c_ubyte * 4),
    ]

class Dogecoin_hdnode(Structure):
    _fields_ = [
        ("depth",                       c_uint32),
        ("fingerprint",                 c_uint32),
        ("child_num",                   c_uint32),
        ("chain_code",                  c_ubyte * 32),
        ("private_key",                 c_ubyte * 32),
        ("public_key",                  c_ubyte * 33),
    ]

def get_chain(code):
    byte_buf = (c_ubyte * 4)()
    
    # main
    if int(code)==0:
        byte_buf[:] = [0xc0, 0xc0, 0xc0, 0xc0]
        return Dogecoin_chain(bytes("main", 'utf-8'), 0x1E, 0x16, 0x9E, 0x02fac398, 0x02facafd, byte_buf)
        
    # testnet
    elif int(code)==1:           
        byte_buf[:] = [0xfc, 0xc1, 0xb7, 0xdc]
        return Dogecoin_chain(bytes("testnet3", 'utf-8'), 0x71, 0xc4, 0xf1, 0x04358394, 0x043587cf, byte_buf)

    # regtest
    elif int(code)==2:
        byte_buf[:] = [0xfa, 0xbf, 0xb5, 0xda]
        return Dogecoin_chain(bytes("regtest", 'utf-8'), 0x6f, 0xc4, 0xEF, 0x04358394, 0x043587CF, byte_buf)

    else:
        print("Bad chain code provided\n")

# BLOCK_HEADERS===============================================
class Const_buffer(Structure):
    _fields_ = [
        ("p", c_void_p),
        ("len", c_ulong),
    ]

class Cstring(Structure):
    _fields_ = [
        ("str", c_void_p),              #uses void to handle present null chars as raw data
        ("len", c_ulong),
        ("alloc", c_ulong),
    ]

class Dogecoin_block_header(Structure):
    _fields_ = [
        ("version",     c_int32),
        ("prev_block",  c_byte * 32),   #simulates the uint256 type
        ("merkle_root", c_byte * 32),
        ("timestamp",   c_uint32),
        ("bits",        c_uint32),
        ("nonce",       c_uint32),
    ]

class Uint256(Structure):
    _fields_ = [
        ("value",      c_byte * 32),
    ]

