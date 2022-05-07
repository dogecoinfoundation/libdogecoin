cimport cython as cy
from libc.string cimport strlen

# FUNCTIONS FROM STATIC LIBRARY
#========================================================
cdef extern from "ecc.h":
    void dogecoin_ecc_start()
    void dogecoin_ecc_stop()

cdef extern from "address.h":
    int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bint is_testnet)
    int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bint is_testnet)
    int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey)
    int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bint is_testnet)
    int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bint is_testnet)
    int verifyP2pkhAddress(char* p2pkh_pubkey, cy.uchar len)

cdef extern from "transaction.h":
    int start_transaction()
    int save_raw_transaction(int txindex, const char* hexadecimal_transaction)
    int add_utxo(int txindex, char* hex_utxo_txid, int vout)
    int add_output(int txindex, char* destinationaddress,  cy.ulong amount)
    char* finalize_transaction(int txindex, char* destinationaddress, double subtractedfee, cy.ulong out_dogeamount_for_verification, char* public_key)
    char* get_raw_transaction(int txindex) 
    void clear_transaction(int txindex) 
    char* sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, int amount, char* privkey)
    char* sign_indexed_raw_transaction(int txindex, int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, int amount, char* privkey)


# PYTHON INTERFACE
#========================================================
# TODO: get correct key lengths
def context_start():
    dogecoin_ecc_start()

def context_stop():
    dogecoin_ecc_stop()

def generate_priv_pub_key_pair(chain_code=0):
    """Generate a valid private key paired with the corresponding
    p2pkh address
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # verify arguments are valid
    assert isinstance(chain_code, int) and chain_code in [0,1]
    
    # prepare arguments
    cdef char privkey[53]
    cdef char p2pkh_pubkey[35]
    cdef bint is_testnet = chain_code

    # call c function
    generatePrivPubKeypair(privkey, p2pkh_pubkey, chain_code)

    # return keys as bytes object
    return privkey, p2pkh_pubkey


def generate_hd_master_pub_key_pair(chain_code=0):
    """Generate a master private and public key pair for use in
    heirarchical deterministic wallets. Public key can be used for
    child key derivation using generate_derived_hd_pub_key().
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # verify arguments are valid
    assert isinstance(chain_code, int) and chain_code in [0,1]
    
    # prepare arguments
    cdef char master_privkey[128]
    cdef char master_p2pkh_pubkey[35]

    # call c function
    generateHDMasterPubKeypair(master_privkey, master_p2pkh_pubkey, chain_code)

    # return keys
    # TODO: extra bytes added to end of testnet keys?? truncate after 34 as a temp patch 
    return master_privkey, master_p2pkh_pubkey[:34]


def generate_derived_hd_pub_key(wif_privkey_master):
    """Given a HD master public key, derive a child key from it.
    Keyword arguments:
    wif_privkey_master -- HD master public key as wif-encoded string
    as_bytes -- flag to return key pair as bytes object
    """
    # verify arguments are valid
    assert isinstance(wif_privkey_master, (str, bytes))

    # prepare arguments
    if not isinstance(wif_privkey_master, bytes):
        wif_privkey_master = wif_privkey_master.encode('utf-8')
    cdef char child_p2pkh_pubkey[128]

    # call c function
    generateDerivedHDPubkey(wif_privkey_master, child_p2pkh_pubkey)

    # return results in bytes
    return child_p2pkh_pubkey


def verify_priv_pub_keypair(wif_privkey, p2pkh_pubkey, chain_code=0):
    """Given a keypair from generate_priv_pub_key_pair, verify that the keys
    are valid and are associated with each other.
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
    res = verifyPrivPubKeypair(wif_privkey, p2pkh_pubkey, chain_code)

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
    res = verifyHDMasterPubKeypair(wif_privkey_master, p2pkh_pubkey_master, chain_code)

    # return boolean result
    return res


def verify_p2pkh_address(p2pkh_pubkey):
    """Given a p2pkh address, confirm address is in correct Dogecoin
    format
    Keyword arguments:
    p2pkh_pubkey -- string containing basic p2pkh address
    """
    # verify arguments are valid
    assert isinstance(p2pkh_pubkey, (str, bytes))

    # prepare arguments
    if not isinstance(p2pkh_pubkey, bytes):
        p2pkh_pubkey = p2pkh_pubkey.encode('utf-8')

    # call c function
    res = verifyP2pkhAddress(p2pkh_pubkey, len(p2pkh_pubkey))

    # return boolean result
    return res