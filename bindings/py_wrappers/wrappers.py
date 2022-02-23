"""This module provides a python API for libdogecoin."""

import ctypes as ct
import sys
import os

# LOAD SHARED LIBRARY ON INIT - TODO: change path to be more flexible
path = os.path.abspath(__file__+"../../../../.libs")
sys.path.insert(0, path)
path = os.path.join(path, "libdogecoin.so")

# all functions from libdogecoin accessed through lib variable
ct.cdll.LoadLibrary(path)
lib = ct.CDLL(path)

#=======================================================ADDRESS.C
def generate_priv_pub_key_pair(chain_code=0, as_bytes=False):
    """Generate a valid private and public key pair.

    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # prepare arguments
    size = 100
    wif_privkey = (ct.c_char * size)()
    p2pkh_pubkey = (ct.c_char * size)()
    assert(chain_code==0 or chain_code==1)
    is_testnet = ct.c_bool(chain_code)

    # call c functions
    lib.dogecoin_ecc_start()
    lib.generatePrivPubKeypair(wif_privkey, p2pkh_pubkey, is_testnet)
    lib.dogecoin_ecc_stop()

    # return results in str/bytes tuple form
    if as_bytes:
        return (wif_privkey.value, p2pkh_pubkey.value)
    return (wif_privkey.value.decode('utf-8'), p2pkh_pubkey.value.decode('utf-8'))


def generate_hd_master_pub_key_pair(chain_code=0, as_bytes=False):
    """Generate a master private and public key pair for use in
    heirarchical deterministic wallets. Public key can be used for
    child key derivation using generate_derived_hd_pub_key().

    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # prepare arguments
    size = 111
    wif_privkey_master = (ct.c_char * size)()
    p2pkh_pubkey_master = (ct.c_char * size)()
    assert(int(chain_code)==0 or int(chain_code)==1)
    is_testnet = ct.c_bool(int(chain_code))

    # call c functions
    lib.dogecoin_ecc_start()
    lib.generateHDMasterPubKeypair(wif_privkey_master, p2pkh_pubkey_master, is_testnet)
    lib.dogecoin_ecc_stop()

    # return results in bytes tuple form
    if as_bytes:
        return (wif_privkey_master.value, p2pkh_pubkey_master.value)
    return (wif_privkey_master.value.decode('utf-8'), p2pkh_pubkey_master.value.decode('utf-8'))


def generate_derived_hd_pub_key(wif_privkey_master, as_bytes=False):
    """Given a HD master public key, derive a child key from it.

    Keyword arguments:
    wif_privkey_master -- HD master public key as wif-encoded string
    as_bytes -- flag to return key pair as bytes object
    """
    # verify arguments are valid
    assert(isinstance(wif_privkey_master, str) or isinstance(wif_privkey_master, bytes))
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')

    # prepare arguments
    size = 100
    wif_privkey_master_ptr = ct.c_char_p(wif_privkey_master)
    child_p2pkh_pubkey = (ct.c_char * size)()

    # call c functions
    lib.dogecoin_ecc_start()
    lib.generateDerivedHDPubkey(wif_privkey_master_ptr, child_p2pkh_pubkey)
    lib.dogecoin_ecc_stop()

    # return results in bytes
    if as_bytes:
        return child_p2pkh_pubkey.value
    return child_p2pkh_pubkey.value.decode('utf-8')

#=======================================================TOOLFUNC.C (deprecated)
class DogecoinChain(ct.Structure):
    """Class to mimic dogecoin_chain struct from libdogecoin"""
    _fields_ = [
        ("chainname",                   ct.c_char * 32),
        ("b58prefix_pubkey_address",    ct.c_ubyte),
        ("b58prefix_script_address",    ct.c_ubyte),
        ("b58prefix_secret_address",    ct.c_ubyte),
        ("b58prefix_bip32_privkey",     ct.c_uint32),
        ("b58prefix_bip32_pubkey",      ct.c_uint32),
        ("netmagic",                    ct.c_ubyte * 4),
    ]

class DogecoinHDNode(ct.Structure):
    """Class to mimic dogecoin_hdnode struct from libdogecoin"""
    _fields_ = [
        ("depth",                       ct.c_uint32),
        ("fingerprint",                 ct.c_uint32),
        ("child_num",                   ct.c_uint32),
        ("chain_code",                  ct.c_ubyte * 32),
        ("private_key",                 ct.c_ubyte * 32),
        ("public_key",                  ct.c_ubyte * 33),
    ]

class DogecoinKey(ct.Structure):
    """Class to mimic dogecoin_key struct from libdogecoin"""
    _fields_ = [
        ("privkey",                     ct.c_ubyte * 32),
    ]

class DogecoinPubkey(ct.Structure):
    """Class to mimic dogecoin_pubkey struct from libdogecoin"""
    _fields_ = [
        ("compressed",                  ct.c_bool),
        ("pubkey",                      ct.c_ubyte * 65),
    ]

def get_chain(code):
    """Load info into DogecoinChain object, depending on which net.

    Keyword arguments:
    code -- 0 for mainnet, 1 for testnet, 2 for regtest
    """
    byte_buf = (ct.c_ubyte * 4)()

    # main
    if int(code)==0:
        byte_buf[:] = [0xc0, 0xc0, 0xc0, 0xc0]
        return DogecoinChain(bytes("main", 'utf-8'),
                             0x1E, 0x16, 0x9E, 0x02fac398, 0x02facafd, byte_buf)

    # testnet
    elif int(code)==1:
        byte_buf[:] = [0xfc, 0xc1, 0xb7, 0xdc]
        return DogecoinChain(bytes("testnet3", 'utf-8'),
                             0x71, 0xc4, 0xf1, 0x04358394, 0x043587cf, byte_buf)

    # regtest
    elif int(code)==2:
        byte_buf[:] = [0xfa, 0xbf, 0xb5, 0xda]
        return DogecoinChain(bytes("regtest", 'utf-8'),
                             0x6f, 0xc4, 0xEF, 0x04358394, 0x043587CF, byte_buf)

    else:
        print("Bad chain code provided\n")



def gen_privkey(chain_code):
    """Generate private key only.

    Keyword arguments:
    chain_code -- 0 for mainnet, 1 for testnet, 2 for regtest
    """
    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h... 0=main, 1=testnet, 2=regtest
    chain = get_chain(chain_code)

    #prepare arguments
    sizeout = 128
    newprivkey_wif = (ct.c_char*sizeout)()
    newprivkey_hex = (ct.c_char*sizeout)()

    #call gen_privatekey()
    lib.gen_privatekey.restype = ct.c_bool
    lib.gen_privatekey(ct.byref(chain), newprivkey_wif, sizeout, newprivkey_hex)

    #stop context
    lib.dogecoin_ecc_stop()

    #return (wif-encoded private key, private key hex)
    return (str(bytes(newprivkey_wif).decode('utf-8')), str(bytes(newprivkey_hex).decode('utf-8')))


def pubkey_from_privatekey(chain, pkey_wif):
    """Generate public key given a valid private key.

    Keyword arguments:
    chain -- DogecoinChain object built from get_chain()
    pkey_wif -- private key as wif-encoded string
    """
    #start context
    lib.dogecoin_ecc_start()

    #prepare arguments
    pkey = ct.c_char_p(pkey_wif.encode('utf-8'))
    size = 128
    sizeout = ct.c_ulong(size)
    pubkey_hex = (ct.c_char * size)()

    #call pubkey_from_privatekey()
    lib.pubkey_from_privatekey.restype = ct.c_bool
    lib.pubkey_from_privatekey(ct.byref(chain), pkey, ct.byref(pubkey_hex), ct.byref(sizeout))

    #stop context
    lib.dogecoin_ecc_stop()

    #return pubkey hex
    return str(bytes(pubkey_hex).decode('utf-8'))


def address_from_pubkey(chain_code, pubkey_hex):
    """Generate dogecoin address given valid public key.

    Keyword arguments:
    chain_code -- 0 for mainnet, 1 for testnet, 2 for regtest
    pubkey_hex -- public key as hex-encoded string"""

    #start context
    lib.dogecoin_ecc_start()

    #init constants from chain.h (0=main, 1=testnet, 2=regtest)
    chain = get_chain(chain_code)

    #prepare arguments
    pubkey = ct.c_char_p(pubkey_hex.encode('utf-8'))
    address = (ct.c_char * 40)() # temporary fix, want ct.c_char_p but causes segfault

    #call address_from_pubkey()
    lib.address_from_pubkey.restype = ct.c_bool
    lib.address_from_pubkey(ct.byref(chain), pubkey, ct.byref(address))

    #stop context
    lib.dogecoin_ecc_stop()

    #return dogecoin address
    return str(bytes(address).decode('utf-8'))


#=======================================================BLOCK.C
class ConstBuffer(ct.Structure):
    """Class to mimic const_buffer struct from libdogecoin"""
    _fields_ = [
        ("p",           ct.c_void_p),
        ("len",         ct.c_ulong),
    ]

class Cstring(ct.Structure):
    """Class to mimic cstring struct from libdogecoin"""
    _fields_ = [
        ("str",         ct.c_void_p), #void to handle null chars as raw data
        ("len",         ct.c_ulong),
        ("alloc",       ct.c_ulong),
    ]

class DogecoinBlockHeader(ct.Structure):
    """Class to mimic dogecoin_block_header struct from libdogecoin"""
    _fields_ = [
        ("version",     ct.c_int32),
        ("prev_block",  ct.c_byte * 32),   #simulates the uint256 type
        ("merkle_root", ct.c_byte * 32),
        ("timestamp",   ct.c_uint32),
        ("bits",        ct.c_uint32),
        ("nonce",       ct.c_uint32),
    ]

class Uint256(ct.Structure):
    """Class to mimic dogecoin_key struct from libdogecoin"""
    _fields_ = [
        ("value",       ct.c_byte * 32),
    ]

def print_dogecoin_block_header_data(header_ptr):
    """Given a DogecoinBlockHeader pointer, prints out header information."""
    header = ct.cast(header_ptr, ct.POINTER(DogecoinBlockHeader)).contents
    b_prev_block = bytes(header.prev_block)
    b_merkle_root = bytes(header.merkle_root)
    print("="*85)
    print("Version:".ljust(20, " "),        header.version)
    print("Previous block:".ljust(20, " "), b_prev_block.hex())
    print("Merkle root:".ljust(20, " "),    b_merkle_root.hex())
    print("Timestamp:".ljust(20, " "),      header.timestamp)
    print("Bits:".ljust(20, " "),           header.bits)
    print("Nonce".ljust(20, " "),           header.nonce)
    print("="*85 + "\n\n")

def print_cstring_data(cstring):
    """Given a Cstring object, prints out string/mem information."""
    raw_bytes = bytes((ct.c_byte*cstring.len).from_address(cstring.str))
    print("="*85)
    print("Serialized string:".ljust(25, " "),         raw_bytes.hex())
    print("Length (bytes):".ljust(25, " "),            cstring.len)
    print("Memory allocated (bytes):".ljust(25, " "),  cstring.alloc)
    print("="*85 + "\n\n")

def build_byte_string(version, prev_block, merkle_root, timestamp, bits, nonce):
    """Package the information to be put on the block into a bytes object."""
    b_string = bytes()
    b_string += version.to_bytes(4, 'little')
    b_string += bytes.fromhex(prev_block)
    b_string += bytes.fromhex(merkle_root)
    b_string += timestamp.to_bytes(4, 'little')
    b_string += bits.to_bytes(4, 'little')
    b_string += nonce.to_bytes(4, 'little')
    return b_string

def dogecoin_block_header_new():
    """Creates a new empty block header (all fields initialized to 0)."""
    lib.dogecoin_block_header_new.restype = ct.c_void_p
    header = ct.c_void_p(lib.dogecoin_block_header_new())
    return header

def dogecoin_block_header_deserialize(header_ser, ilen, header_ptr):
    """Puts information from buf into header."""
    lib.dogecoin_block_header_new.restype = ct.c_int32
    if not lib.dogecoin_block_header_deserialize(header_ser, ilen, header_ptr):
        print("dogecoin_block_header_deserialize: deserialization failed!\n")

def dogecoin_block_header_serialize(cstr_ptr, header_ptr):
    """Given non-empty block header, translate info into cstring pointer."""
    lib.dogecoin_block_header_new.restype = None
    lib.dogecoin_block_header_serialize(cstr_ptr, header_ptr)

def dogecoin_block_header_copy(new_header_ptr, old_header_ptr):
    """Given existing block header, copy information to a new block header."""
    lib.dogecoin_block_header_copy.restype = None
    lib.dogecoin_block_header_copy(new_header_ptr, old_header_ptr)

def dogecoin_block_header_free(header_ptr):
    """Given existing block header, set to zero and free the memory allocated for that header."""
    lib.dogecoin_block_header_free.restype = None
    lib.dogecoin_block_header_free(header_ptr)

def dogecoin_block_header_hash(header_ptr, hash_ptr):
    """Calculate and store the hash of a given block header."""
    lib.dogecoin_block_header_hash.restype = ct.c_byte
    lib.dogecoin_block_header_hash(header_ptr, hash_ptr)
