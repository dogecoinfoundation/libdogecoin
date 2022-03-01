"""This module provides a python API for libdogecoin."""

import ctypes as ct
import sys
import os

def load_libdogecoin():
    """Load the libdogecoin library from "libdogecoin.so"."""
    # TODO: change path to be more flexible
    path = os.path.abspath(__file__+"../../../../.libs")
    sys.path.insert(0, path)
    path = os.path.join(path, "libdogecoin.so")

    # all functions from libdogecoin accessed through lib variable
    ct.cdll.LoadLibrary(path)
    return ct.CDLL(path)

def dogecoin_ecc_start():
    """Starts ecc context (required for running any key function)."""
    lib.dogecoin_ecc_start()

def dogecoin_ecc_stop():
    """Stops currently running ecc context."""
    lib.dogecoin_ecc_stop()

lib = load_libdogecoin()

#=======================================================ADDRESS.C
def generate_priv_pub_key_pair(chain_code=0, as_bytes=False):
    """Generate a valid private key paired with the corresponding
    p2pkh address
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

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    lib.generatePrivPubKeypair(wif_privkey, p2pkh_pubkey, is_testnet)

    # stop context
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

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    lib.generateHDMasterPubKeypair(wif_privkey_master, p2pkh_pubkey_master, is_testnet)

    # stop context
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
    assert(isinstance(wif_privkey_master, (str, bytes)))
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')

    # prepare arguments
    size = 100
    wif_privkey_master_ptr = ct.c_char_p(wif_privkey_master)
    child_p2pkh_pubkey = (ct.c_char * size)()

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    # TODO derived key is the same each time, most likely because of context start
    # should ecc_start/stop be wrapped too?
    lib.generateDerivedHDPubkey(wif_privkey_master_ptr, child_p2pkh_pubkey)

    # stop context
    lib.dogecoin_ecc_stop()

    # return results in bytes
    if as_bytes:
        return child_p2pkh_pubkey.value
    return child_p2pkh_pubkey.value.decode('utf-8')


def verify_priv_pub_keypair(wif_privkey, p2pkh_pubkey, chain_code=0):
    """Given a keypair from generate_priv_pub_key_pair, verify that the keys
    are valid and are associated with each other.
    Keyword arguments:
    wif_privkey -- string containing wif-encoded private key
    p2pkh_pubkey -- string containing address derived from wif_privkey
    chain_code -- 0 for mainnet, 1 for testnet
    """
    # verify arguments are valid
    assert isinstance(wif_privkey, str) and isinstance(p2pkh_pubkey, str)
    assert isinstance(chain_code, int)

    # prepare arguments
    wif_privkey_ptr = ct.c_char_p(wif_privkey.encode('utf-8'))
    p2pkh_pubkey_ptr = ct.c_char_p(p2pkh_pubkey.encode('utf-8'))

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    res = ct.c_bool()
    res = lib.verifyPrivPubKeypair(wif_privkey_ptr, p2pkh_pubkey_ptr, ct.c_int(chain_code))

    # stop context
    lib.dogecoin_ecc_stop()

    # return boolean result
    return res


def verify_p2pkh_address(p2pkh_pubkey, chain_code=0):
    """Given a p2pkh address, confirm address is in correct format and
    passes simple Shannon entropy threshold
    Keyword arguments:
    p2pkh_pubkey -- string containing basic p2pkh address
    chain_code -- 0 for mainnet, 1 for testnet
    """
    # verify arguments are valid
    assert isinstance(p2pkh_pubkey, str) and isinstance(chain_code, int)

    # prepare arguments
    p2pkh_pubkey_ptr = ct.c_char_p(p2pkh_pubkey.encode('utf-8'))

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    res = ct.c_bool()
    res = lib.verifyP2pkhAddress(p2pkh_pubkey_ptr, ct.c_int(chain_code))

    # return boolean result
    return res


#=======================================================TOOLFUNC.C (deprecated)
class DogecoinDNSSeed(ct.Structure):
    """Class to mimic dogecoin_dns_seed struct from libdogecoin"""
    _fields_ = [
        ("domain",                      ct.c_char * 256),
    ]

class DogecoinChainparams(ct.Structure):
    """Class to mimic dogecoin_chainparams struct from libdogecoin"""
    _fields_ = [
        ("chainname",                   ct.c_char * 32),
        ("b58prefix_pubkey_address",    ct.c_uint8),
        ("b58prefix_script_address",    ct.c_uint8),
        ("bech32_hrp",                  ct.c_char * 5),
        ("b58prefix_secret_address",    ct.c_ubyte),
        ("b58prefix_bip32_privkey",     ct.c_uint32),
        ("b58prefix_bip32_pubkey",      ct.c_uint32),
        ("netmagic",                    ct.c_ubyte * 4),
        ("genesisblockhash",            ct.c_uint8 * 32),
        ("default port",                ct.c_int),
        ("dnsseeds",                    DogecoinDNSSeed * 8),
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
        ("compressed",                  ct.c_ubyte),
        ("pubkey",                      ct.c_ubyte * 65),
    ]

def get_chainparams(code):
    """Load info into DogecoinChainparams object, depending on which net.
    Keyword arguments:
    code -- 0 for mainnet, 1 for testnet, 2 for regtest
    """
    magic_bytes = (ct.c_ubyte * 4)()
    genesis_blockhash = (ct.c_uint8 * 32)()
    dns_seeds = (DogecoinDNSSeed * 8)()

    # TODO: fix dns_seeds part below, no info for now

    # main
    if int(code)==0:
        magic_bytes[:] = [0xc0, 0xc0, 0xc0, 0xc0]
        genesis_blockhash[:] = [0x91, 0x56, 0x35, 0x2c, 0x18, 0x18, 0xb3, 0x2e,
                                0x90, 0xc9, 0xe7, 0x92, 0xef, 0xd6, 0xa1, 0x1a,
                                0x82, 0xfe, 0x79, 0x56, 0xa6, 0x30, 0xf0, 0x3b,
                                0xbe, 0xe2, 0x36, 0xce, 0xda, 0xe3, 0x91, 0x1a]

        dns_seeds = (DogecoinChainparams * 8)()
        return DogecoinChainparams(bytes("main", 'utf-8'),
                                   0x1E, 0x16, bytes("doge", 'utf-8'), 0x9E,
                                   0x02fac398, 0x02facafd,
                                   magic_bytes, genesis_blockhash, 22556, dns_seeds)

    # testnet
    elif int(code)==1:
        magic_bytes[:] = [0xfc, 0xc1, 0xb7, 0xdc]
        genesis_blockhash[:] = [0x9e, 0x55, 0x50, 0x73, 0xd0, 0xc4, 0xf3, 0x64,
                                0x56, 0xdb, 0x89, 0x51, 0xf4, 0x49, 0x70, 0x4d,
                                0x54, 0x4d, 0x28, 0x26, 0xd9, 0xaa, 0x60, 0x63,
                                0x6b, 0x40, 0x37, 0x46, 0x26, 0x78, 0x0a, 0xbb]
        return DogecoinChainparams(bytes("testnet3", 'utf-8'),
                                   0x71, 0xc4, bytes("tdge", 'utf-8'), 0xf1,
                                   0x04358394, 0x043587cf,
                                   magic_bytes, genesis_blockhash, 44556, dns_seeds)

    # regtest
    elif int(code)==2:
        magic_bytes[:] = [0xfa, 0xbf, 0xb5, 0xda]
        genesis_blockhash[:] = [0xa5, 0x73, 0xe9, 0x1c, 0x17, 0x72, 0x07, 0x6c,
                                0x0d, 0x40, 0xf7, 0x0e, 0x44, 0x08, 0xc8, 0x3a,
                                0x31, 0x70, 0x5f, 0x29, 0x6a, 0xe6, 0xe7, 0x62,
                                0x9d, 0x4a, 0xdc, 0xb5, 0xa3, 0x60, 0x21, 0x3d]
        return DogecoinChainparams(bytes("regtest", 'utf-8'),
                             0x6f, 0xc4, bytes("dcrt", 'utf-8'), 0xEF,
                             0x04358394, 0x043587CF,
                             magic_bytes, genesis_blockhash, 18332, dns_seeds)

    else:
        print("Bad chain code provided\n")

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
