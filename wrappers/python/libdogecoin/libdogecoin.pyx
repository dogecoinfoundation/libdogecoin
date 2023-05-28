cimport cython
from libc.string cimport strncpy, memset
from libc.stdint cimport uint32_t, uint8_t

from . import error
from . import constants
from . cimport defines
from . cimport types
from . cimport _wow
from . cimport _libdogecoin


# PYTHON INTERFACE
#========================================================
# wow.h
def wow():
    """ wow.h wow """
    _wow.wow()
# end wow.h

#
# mnemonic
def generateRandomEnglishMnemonic(entropy_size):
    """libdogecoin.h generateRandomEnglishMnemonic
    
    :param int entropy_size: constants.ENTROPY_SIZE_LIST
    :returns: mnemonic string
    :rtype: str
    """
    assert entropy_size in constants.ENTROPY_SIZE_LIST
    cdef types.MNEMONIC mnemonic

    entropy_size_bytes = str(entropy_size).encode("utf-8")
    res = _libdogecoin.generateRandomEnglishMnemonic(entropy_size_bytes, mnemonic)
    if res != 0:
        raise error.LibdogecoinAPIException("generateRandomEnglishMnemonic")
    
    return mnemonic.decode('utf-8')

def generateEnglishMnemonic(entropy, entropy_size):
    """ libdogecoin.h generateEnglishMnemonic

    :param bytes entropy:
    :param int entropy_size: constants.ENTROPY_SIZE_LIST
    :returns: mnemonic
    :rtype: str
    """
    assert entropy_size in constants.ENTROPY_SIZE_LIST
    cdef types.MNEMONIC mnemonic

    entropy_size_bytes = str(entropy_size).encode("utf-8")
    res = _libdogecoin.generateEnglishMnemonic(entropy, entropy_size_bytes, mnemonic)
    if res != 0:
        raise error.LibdogecoinAPIException("generateEnglishMnemonic")

    return mnemonic.decode('utf-8')

# end mnemonic


# ADDRESS FUNCTIONS
def context_start():
    """Start the secp256k1 context necessary for key pair 
    generation. Must be started before calling any functions
    dealing with private or public keys.
    """
    _libdogecoin.dogecoin_ecc_start()

def context_stop():
    """Stop the current instance of the secp256k1 context. It is
    advised to wait until the session is completely over before
    stopping the context.
    """
    _libdogecoin.dogecoin_ecc_stop()

def generate_priv_pub_key_pair(chain_code=0):
    """Generate a valid private key paired with the corresponding
    p2pkh address.
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    """
    # verify arguments are valid
    assert isinstance(chain_code, int) and chain_code in [0,1]
    
    # prepare arguments
    cdef char privkey[53]
    cdef char p2pkh_pubkey[35]
    cdef bint is_testnet = chain_code

    # call c function
    _libdogecoin.generatePrivPubKeypair(privkey, p2pkh_pubkey, chain_code)

    # return keys as bytes object
    return privkey, p2pkh_pubkey


def generate_hd_master_pub_key_pair(chain_code=0):
    """Generate a master private and public key pair for use in
    hierarchical deterministic wallets. Public key can be used for
    child key derivation using generate_derived_hd_pub_key().
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    """
    # verify arguments are valid
    assert isinstance(chain_code, int) and chain_code in [0,1]
    
    # prepare arguments
    cdef char master_privkey[128]
    cdef char master_p2pkh_pubkey[35]

    # call c function
    _libdogecoin.generateHDMasterPubKeypair(master_privkey, master_p2pkh_pubkey, chain_code)

    # return keys
    # TODO: extra bytes added to end of testnet keys?? truncate after 34 as a temp patch 
    return master_privkey, master_p2pkh_pubkey[:34]


def generate_derived_hd_pub_key(wif_privkey_master):
    """Given a HD master private key, derive a child key from it.
    Keyword arguments:
    wif_privkey_master -- HD master public key as wif-encoded string
    """
    # verify arguments are valid
    assert isinstance(wif_privkey_master, (str, bytes))

    # prepare arguments
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')
    cdef char child_p2pkh_pubkey[128]

    # call c function
    _libdogecoin.generateDerivedHDPubkey(wif_privkey_master, child_p2pkh_pubkey)

    # return results in bytes
    return child_p2pkh_pubkey


def verify_priv_pub_keypair(wif_privkey, p2pkh_pubkey, chain_code=0):
    """Given a private/public key pair, verify that the keys are
    valid and are associated with each other.
    Keyword arguments:
    wif_privkey -- string containing wif-encoded private key
    p2pkh_pubkey -- string containing address derived from wif_privkey
    chain_code -- 0 for mainnet, 1 for testnet
    """
    # verify arguments are valid
    assert isinstance(wif_privkey, (str, bytes))
    assert isinstance(p2pkh_pubkey, (str, bytes))
    assert isinstance(chain_code, int) and chain_code in [0,1]

    # prepare arguments
    if not isinstance(wif_privkey, bytes):
        wif_privkey = wif_privkey.encode('utf-8')
    if not isinstance(p2pkh_pubkey, bytes):
        p2pkh_pubkey = p2pkh_pubkey.encode('utf-8')

    # call c function
    res = _libdogecoin.verifyPrivPubKeypair(wif_privkey, p2pkh_pubkey, chain_code)

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
    assert isinstance(wif_privkey_master, (str, bytes))
    assert isinstance(p2pkh_pubkey_master, (str, bytes))
    assert isinstance(chain_code, int) and chain_code in [0,1]

    # prepare arguments
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')
    if not isinstance(p2pkh_pubkey_master, bytes):
        p2pkh_pubkey_master = p2pkh_pubkey_master.encode('utf-8')

    # call c function
    res = _libdogecoin.verifyHDMasterPubKeypair(wif_privkey_master, p2pkh_pubkey_master, chain_code)

    # return boolean result
    return res


def verify_p2pkh_address(p2pkh_pubkey):
    """Given a p2pkh address, confirm address is in correct Dogecoin
    format.
    Keyword arguments:
    p2pkh_pubkey -- string containing basic p2pkh address
    """
    # verify arguments are valid
    assert isinstance(p2pkh_pubkey, (str, bytes))

    # prepare arguments
    if not isinstance(p2pkh_pubkey, bytes):
        p2pkh_pubkey = p2pkh_pubkey.encode('utf-8')

    # call c function
    res = _libdogecoin.verifyP2pkhAddress(p2pkh_pubkey, len(p2pkh_pubkey))

    # return boolean result
    return res



# TRANSACTION FUNCTIONS

def start_transaction():
    """Create a new, empty dogecoin transaction."""
    # call c function
    res = _libdogecoin.start_transaction()

    # return boolean result
    return res


def add_utxo(tx_index, hex_utxo_txid, vout):
    """Given the index of a working transaction, add another
    input to it.
    Keyword arguments:
    tx_index -- the index of the working transaction to update
    hex_utxo_txid -- the transaction id of the utxo to be spent
    vout -- the number of outputs associated with the specified utxo
    """
    # verify arguments are valid
    assert isinstance(tx_index, int)
    assert isinstance(hex_utxo_txid, (str, bytes))
    assert isinstance(vout, (int, str))

    # prepare arguments
    if not isinstance(hex_utxo_txid, bytes):
        hex_utxo_txid = hex_utxo_txid.encode('utf-8')

    # call c function
    res = _libdogecoin.add_utxo(tx_index, hex_utxo_txid, vout)

    # return boolean result
    return res


def add_output(tx_index, destination_address, amount):
    """Given the index of a working transaction, add another
    output to it.
    Keyword arguments:
    tx_index -- the index of the working transaction to update
    destination_address -- the address of the output being added
    amount -- the amount of dogecoin to send to the specified address
    """
    # verify arguments are valid
    assert isinstance(tx_index, int)
    assert isinstance(destination_address, (str, bytes))
    assert isinstance(amount, (int, str))

    # prepare arguments
    if not isinstance(destination_address, bytes):
        destination_address = destination_address.encode('utf-8')
    amount = str(amount).encode('utf-8')

    # call c function
    res = _libdogecoin.add_output(tx_index, destination_address, amount)

    # return boolean result
    return res


def finalize_transaction(tx_index, destination_address, subtracted_fee, out_dogeamount_for_verification, changeaddress):
    """Given the index of a working transaction, prepares it
    for signing by specifying the recipient and fee to subtract,
    directing extra change back to the sender.
    Keyword arguments:
    tx_index -- the index of the working transaction
    destination address -- the address to send coins to
    subtracted_fee -- the amount of dogecoin to assign as a fee
    out_dogeamount_for_verification -- the total amount of dogecoin being sent (fee included)
    changeaddress -- the address of the sender to receive their change
    """
    print(tx_index, destination_address, subtracted_fee, out_dogeamount_for_verification, changeaddress)
    # verify arguments are valid
    assert isinstance(tx_index, int)
    assert isinstance(destination_address, (str, bytes))
    assert isinstance(subtracted_fee, (int, str))
    assert isinstance(out_dogeamount_for_verification, (int, str))
    assert isinstance(changeaddress, (str, bytes))

    # prepare arguments
    if not isinstance(destination_address, bytes):
        destination_address = destination_address.encode('utf-8')
    if not isinstance(changeaddress, bytes):
        changeaddress = changeaddress.encode('utf-8')
    subtracted_fee = str(subtracted_fee).encode('utf-8')
    out_dogeamount_for_verification = str(out_dogeamount_for_verification).encode('utf-8')

    # call c function
    cdef void* res
    cdef char* finalized_transaction_hex
    res = _libdogecoin.finalize_transaction(tx_index, destination_address, subtracted_fee, out_dogeamount_for_verification, changeaddress)

    # return hex result
    try:
        if (res==<void*>0):
            raise TypeError
        finalized_transaction_hex = <char*>res
        return finalized_transaction_hex.decode('utf-8')
    except:
        return 0


def get_raw_transaction(tx_index):
    """Given the index of a working transaction, returns
    the serialized object in hex format.
    Keyword arguments:
    tx_index -- the index of the working transaction
    """
    # verify arguments are valid
    assert isinstance(tx_index, int)

    # call c function
    cdef void* res
    cdef char* raw_transaction_hex
    res = _libdogecoin.get_raw_transaction(tx_index)

    # return hex result
    try:
        if (res==<void*>0):
            raise TypeError
        raw_transaction_hex = <char*>res
        return raw_transaction_hex.decode('utf-8') 
    except:
        return 0
        

def clear_transaction(tx_index):
    """Discard a working transaction.
    Keyword arguments:
    tx_index -- the index of the working transaction
    """
    # verify arguments are valid
    assert isinstance(tx_index, int)

    # call c function
    _libdogecoin.clear_transaction(tx_index)


def sign_raw_transaction(tx_index, incoming_raw_tx, script_hex, sig_hash_type, privkey):
    """Sign a finalized raw transaction using the specified
    private key.
    Keyword arguments:
    tx_index -- the index of the working transaction to sign
    incoming_raw_tx -- the serialized string of the transaction to sign
    script_hex -- the hex of the script to be signed
    sig_hash_type -- the type of signature hash to be used
    privkey -- the private key to sign with
    """
    # verify arguments are valid
    assert isinstance(tx_index, int)
    assert isinstance(incoming_raw_tx, (str, bytes))
    assert isinstance(script_hex, (str, bytes))
    assert isinstance(sig_hash_type, int)
    assert isinstance(privkey, (str, bytes))

    # prepare arguments
    if not isinstance(incoming_raw_tx, bytes):
        incoming_raw_tx = incoming_raw_tx.encode('utf-8')
    if not isinstance(script_hex, bytes):
        script_hex = script_hex.encode('utf-8')
    if not isinstance(privkey, bytes):
        privkey = privkey.encode('utf-8')

    # allocate enough mem to cover extension of transaction hex
    cdef char c_incoming_raw_tx[1024*100] # max size for a valid signable transaction
    memset(c_incoming_raw_tx, 0, (1024*100))
    strncpy(c_incoming_raw_tx, incoming_raw_tx, len(incoming_raw_tx))
    
    # call c function and return result
    if _libdogecoin.sign_raw_transaction(tx_index, c_incoming_raw_tx, script_hex, sig_hash_type, privkey):
        res = c_incoming_raw_tx.decode('utf-8')
        return res
    else:
        return 0


def sign_transaction(tx_index, script_pubkey, privkey):
    """Sign all the inputs of a working transaction using the
    specified private key and public key script.
    Keyword arguments:
    tx_index -- the index of the working transaction to sign
    script_pubkey -- the pubkey script associated with the private key
    privkey -- the private key used to sign the specified transaction"""
    # verify arguments are valid
    assert isinstance(tx_index, int)
    assert isinstance(script_pubkey, (str, bytes))
    assert isinstance(privkey, (str, bytes))

    # prepare arguments
    if not isinstance(script_pubkey, bytes):
        script_pubkey = script_pubkey.encode('utf-8')
    if not isinstance(privkey, bytes):
        privkey = privkey.encode('utf-8')
    
    # call c function
    res = _libdogecoin.sign_transaction(tx_index, script_pubkey, privkey)

    # return result
    return res    


def store_raw_transaction(incoming_raw_tx):
    """Stores a raw transaction at the next available index
    in the hash table.
    Keyword arguments:
    incoming_raw_tx -- the serialized string of the transaction to store.
    """
    # verify arguments are valid
    assert isinstance(incoming_raw_tx, (str, bytes))

    # prepare arguments
    if not isinstance(incoming_raw_tx, bytes):
        incoming_raw_tx = incoming_raw_tx.encode('utf-8')
    
    # call c function
    res = _libdogecoin.store_raw_transaction(incoming_raw_tx)

    # return result
    return res
