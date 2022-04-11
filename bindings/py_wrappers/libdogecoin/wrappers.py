"""This module provides a python API for libdogecoin."""

import ctypes as ct
import os

def load_libdogecoin():
    """Load the libdogecoin library from "libdogecoin.so"."""
    # TODO: change path to be more flexible
    path = os.path.join(os.getcwd(), ".libs/libdogecoin.so")

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
    # verify arguments are valid
    assert int(chain_code)==0 or int(chain_code)==1

    # prepare arguments
    size = 100
    wif_privkey = (ct.c_char * size)()
    p2pkh_pubkey = (ct.c_char * size)()
    is_testnet = ct.c_bool(int(chain_code))

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
    hierarchical deterministic wallets. Public key can be used for
    child key derivation using generate_derived_hd_pub_key().
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # verify arguments are valid
    assert int(chain_code)==0 or int(chain_code)==1

    # prepare arguments
    size = 111
    wif_privkey_master = (ct.c_char * size)()
    p2pkh_pubkey_master = (ct.c_char * size)()
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

    # prepare arguments
    size = 100
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')
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
    assert isinstance(wif_privkey, str)
    assert isinstance(p2pkh_pubkey, str)
    assert int(chain_code)==0 or int(chain_code)==1

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


def verify_master_priv_pub_keypair(wif_privkey_master, p2pkh_pubkey_master, chain_code=0):
    """Given a keypair from generate_hd_master_pub_key_pair, verify that the
    keys are valid and are associated with each other.
    Keyword arguments:
    wif_privkey_master -- string containing wif-encoded private master key
    p2pkh_pubkey_master -- string containing address derived from wif_privkey
    chain_code -- 0 for mainnet, 1 for testnet
    """
    # verify arguments are valid
    assert isinstance(wif_privkey_master, str) 
    assert isinstance(p2pkh_pubkey_master, str)
    assert int(chain_code)==0 or int(chain_code)==1

    # prepare arguments
    wif_privkey_master_ptr = ct.c_char_p(wif_privkey_master.encode('utf-8'))
    p2pkh_pubkey_master_ptr = ct.c_char_p(p2pkh_pubkey_master.encode('utf-8'))

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    res = ct.c_bool()
    res = lib.verifyHDMasterPubKeypair(wif_privkey_master_ptr, p2pkh_pubkey_master_ptr, ct.c_bool(chain_code))

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
    assert isinstance(p2pkh_pubkey, str)
    assert int(chain_code)==0 or int(chain_code)==1

    # prepare arguments
    p2pkh_pubkey_ptr = ct.c_char_p(p2pkh_pubkey.encode('utf-8'))
    strlen = ct.c_ubyte(len(p2pkh_pubkey))

    # start context
    lib.dogecoin_ecc_start()

    # call c function
    res = ct.c_bool()
    res = lib.verifyP2pkhAddress(p2pkh_pubkey_ptr, strlen, ct.c_int(chain_code))

    # stop context
    lib.dogecoin_ecc_stop()

    # return boolean result
    return res


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
#===================================================TRANSACTION.C
def start_transaction():
    # set return type
    lib.start_transaction.restype = ct.c_int

    # call c function
    res = lib.start_transaction()

    # return result
    return int(res)

def add_utxo(tx_index, hex_utxo_txid, vout):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()
    assert isinstance(vout, int) or vout.isnumeric()
    assert isinstance(hex_utxo_txid, str)

    # convert string to be c-compatible
    hex_utxo_txid_ptr = ct.c_char_p(hex_utxo_txid.encode('utf-8'))

    # set types for parameters and return
    lib.add_utxo.argtypes = [ct.c_int, ct.c_char_p, ct.c_int]
    lib.add_utxo.restype = ct.c_int

    # call c function and return result
    res = ct.c_int()
    res = lib.add_utxo(int(tx_index), hex_utxo_txid_ptr, int(vout))

    # return result
    return int(res)
    
def add_output(tx_index, destination_address, amount):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()
    assert isinstance(destination_address, str)
    assert isinstance(amount, (float, int))

    # convert args to be c-compatible
    destination_address_ptr = ct.c_char_p(destination_address.encode('utf-8'))
    if isinstance(amount, float):
        amount = int(amount) # TODO: will truncate! is this preferred?

    # set types for parameters and return
    lib.add_output.argtypes = [ct.c_int, ct.c_char_p, ct.c_uint64]
    lib.add_output.restype = ct.c_int

    # call c function
    res = lib.add_output(int(tx_index), destination_address_ptr, amount)

    # return result
    return int(res)

def make_change(tx_index, prvkey_wif, destination_address, subtracted_fee, amount):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()
    assert isinstance(prvkey_wif, str)
    assert isinstance(destination_address, str)
    assert isinstance(subtracted_fee, (int, float))
    assert isinstance(amount, (int, float))

    # convert args to be c-compatible
    prvkey_wif_ptr = ct.c_char_p(prvkey_wif.encode('utf-8'))
    destination_address_ptr = ct.c_char_p(destination_address.encode('utf-8'))
    if isinstance(amount, float):
        amount = int(amount) # TODO: will truncate! is this preferred?

    # set types for parameters and return
    lib.make_change.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_float, ct.c_uint64]
    lib.make_change.restype = ct.c_void_p
    
    # call c function
    lib.dogecoin_ecc_start()
    res = lib.make_change(int(tx_index), prvkey_wif_ptr, destination_address_ptr, subtracted_fee, amount)
    lib.dogecoin_ecc_stop()

    # return result
    try:
        res = ct.c_char_p(res)
        return res.value.decode("utf-8")
    except:
        return 0

def finalize_transaction(tx_index, destination_address, subtracted_fee, out_dogeamount_for_verification, sender_p2pkh):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()
    assert isinstance(destination_address, str)
    assert isinstance(subtracted_fee, (int, float))
    assert isinstance(out_dogeamount_for_verification, (int, float))
    assert isinstance(sender_p2pkh, str)
    
    # convert strings to be c-compatible
    destination_address_ptr = ct.c_char_p(destination_address.encode('utf-8'))
    sender_p2pkh_ptr = ct.c_char_p(sender_p2pkh.encode('utf-8'))
    if isinstance(out_dogeamount_for_verification, float):
        out_dogeamount_for_verification = int(out_dogeamount_for_verification) # TODO: will truncate! is this preferred?

    # set types for parameters and return
    lib.finalize_transaction.argtypes = [ct.c_int, ct.c_char_p, ct.c_float, ct.c_uint64, ct.c_char_p]
    lib.finalize_transaction.restype = ct.c_void_p
    
    # call c function
    res = lib.finalize_transaction(int(tx_index), destination_address_ptr, subtracted_fee, out_dogeamount_for_verification, sender_p2pkh_ptr)
    
    # return result
    try:
        res = ct.c_char_p(res)
        return res.value.decode("utf-8")
    except:
        return 0

def get_raw_transaction(tx_index):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()

    # set types for parameters and return
    lib.get_raw_transaction.argtypes = [ct.c_int]
    lib.get_raw_transaction.restype = ct.c_void_p

    # call c function
    res = lib.get_raw_transaction(tx_index)

    # return result
    try:
        res = ct.c_char_p(res)
        return res.value.decode("utf-8")
    except:
        return 0

def clear_transaction(tx_index):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()

    # set parameter types
    lib.get_raw_transaction.argtypes = [ct.c_int]

    # call c function (void return)
    lib.clear_transaction(tx_index)

def sign_indexed_transaction(tx_index, privkey):
    # verify arguments are valid
    assert isinstance(tx_index, int) or tx_index.isnumeric()
    assert isinstance(privkey, str)
    
    # convert string to be c-compatible
    privkey_ptr = ct.c_char_p(privkey.encode('utf-8'))

    # set types for parameters and return
    lib.sign_indexed_transaction.argtypes = [ct.c_int, ct.c_char_p]
    lib.sign_indexed_transaction.restype = ct.c_void_p

    # call c function and return result
    res = lib.sign_indexed_transaction(tx_index, privkey_ptr)

    # return result
    try:
        res = ct.c_char_p(res)
        return res.value.decode("utf-8")
    except:
        return 0

def sign_raw_transaction(input_index, incoming_raw_tx, script_hex, sig_hash_type, amount, privkey):
    # verify arguments are valid
    assert isinstance(input_index, int) or input_index.isnumeric()
    assert isinstance(incoming_raw_tx, str)
    assert isinstance(script_hex, str)
    assert isinstance(sig_hash_type, int) or sig_hash_type.isnumeric()
    assert isinstance(amount, (int, float))

    # convert strings to be c-compatible
    incoming_raw_tx_ptr = ct.c_char_p(incoming_raw_tx.encode('utf-8'))
    script_hex_ptr = ct.c_char_p(script_hex.encode('utf-8'))
    privkey_ptr = ct.c_char_p(privkey.encode('utf-8'))

    # set types for parameters and return
    lib.sign_raw_transaction.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_char_p]
    lib.sign_raw_transaction.restype = ct.c_int

    # call c function and return result
    lib.dogecoin_ecc_start()
    res = ct.c_int()
    res = lib.sign_raw_transaction(input_index, incoming_raw_tx_ptr, script_hex_ptr, sig_hash_type, amount, privkey_ptr)
    lib.dogecoin_ecc_stop()

    # return signed transaction hex if successful, 0 otherwise
    if res==1:
        return incoming_raw_tx_ptr.value.decode("utf-8")
    else:
        return int(res)
    