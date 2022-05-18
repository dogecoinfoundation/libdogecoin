# def load_libdogecoin():
#     """Load the libdogecoin library from "libdogecoin.so"."""
#     # TODO: change path to be more flexible
#     path = os.path.join(os.getcwd(), ".libs/libdogecoin.so")

#     # all functions from libdogecoin accessed through lib variable
#     ct.cdll.LoadLibrary(path)
#     return ct.CDLL(path)



# FUNCTIONS FROM STATIC LIBRARY
#========================================================
cdef extern from "ecc.h":
    void dogecoin_ecc_start()
    void dogecoin_ecc_stop()

cdef extern from "address.h":
    int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bint is_testnet)
    int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bint is_testnet)


# PYTHON INTERFACE
#========================================================
def generate_priv_pub_key_pair(chain_code=0, as_bytes=False):
    """Generate a valid private key paired with the corresponding
    p2pkh address
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # prepare arguments
    cdef char privkey[53]
    cdef char p2pkh_pubkey[35]
    cdef bint is_testnet = chain_code

    # call c function
    dogecoin_ecc_start()
    generatePrivPubKeypair(privkey, p2pkh_pubkey, is_testnet)
    dogecoin_ecc_stop()

    # convert keys back to string
    new_privkey = privkey.decode('utf-8')
    new_p2pkh_pubkey = p2pkh_pubkey.decode('utf-8')

    # return keys
    if as_bytes: return privkey, p2pkh_pubkey
    else: return new_privkey, new_p2pkh_pubkey


def generate_hd_master_pub_key_pair(chain_code=0, as_bytes=False):
    """Generate a master private and public key pair for use in
    heirarchical deterministic wallets. Public key can be used for
    child key derivation using generate_derived_hd_pub_key().
    Keyword arguments:
    chain_code -- 0 for mainnet pair, 1 for testnet pair
    as_bytes -- flag to return key pair as bytes object
    """
    # prepare arguments
    cdef char master_privkey[111]
    cdef char master_p2pkh_pubkey[111]
    cdef bint is_testnet = chain_code

    # call c function
    dogecoin_ecc_start()
    generateHDMasterPubKeypair(master_privkey, master_p2pkh_pubkey, is_testnet)
    dogecoin_ecc_stop()

    # convert keys back to string
    new_master_privkey = master_privkey.decode('utf-8')
    new_master_p2pkh_pubkey = master_p2pkh_pubkey.decode('utf-8')

    # return keys
    if as_bytes: return master_privkey, master_p2pkh_pubkey
    else: return new_master_privkey, new_master_p2pkh_pubkey


# def generate_derived_hd_pub_key(wif_privkey_master, as_bytes=False):
#     """Given a HD master public key, derive a child key from it.
#     Keyword arguments:
#     wif_privkey_master -- HD master public key as wif-encoded string
#     as_bytes -- flag to return key pair as bytes object
#     """
#     # verify arguments are valid
#     assert(isinstance(wif_privkey_master, (str, bytes)))
#     if not isinstance(wif_privkey_master, bytes):
#         wif_privkey_master = wif_privkey_master.encode('utf-8')

#     # prepare arguments
#     size = 100
#     wif_privkey_master_ptr = ct.c_char_p(wif_privkey_master)
#     child_p2pkh_pubkey = (ct.c_char * size)()

#     # start context
#     lib.dogecoin_ecc_start()

#     # call c function
#     # TODO derived key is the same each time, most likely because of context start
#     # should ecc_start/stop be wrapped too?
#     lib.generateDerivedHDPubkey(wif_privkey_master_ptr, child_p2pkh_pubkey)

#     # stop context
#     lib.dogecoin_ecc_stop()

#     # return results in bytes
#     if as_bytes:
#         return child_p2pkh_pubkey.value
#     return child_p2pkh_pubkey.value.decode('utf-8')


# def verify_priv_pub_keypair(wif_privkey, p2pkh_pubkey, chain_code=0):
#     """Given a keypair from generate_priv_pub_key_pair, verify that the keys
#     are valid and are associated with each other.
#     Keyword arguments:
#     wif_privkey -- string containing wif-encoded private key
#     p2pkh_pubkey -- string containing address derived from wif_privkey
#     chain_code -- 0 for mainnet, 1 for testnet
#     """
#     # verify arguments are valid
#     assert isinstance(wif_privkey, str) and isinstance(p2pkh_pubkey, str)
#     assert isinstance(chain_code, int)

#     # prepare arguments
#     wif_privkey_ptr = ct.c_char_p(wif_privkey.encode('utf-8'))
#     p2pkh_pubkey_ptr = ct.c_char_p(p2pkh_pubkey.encode('utf-8'))

#     # start context
#     lib.dogecoin_ecc_start()

#     # call c function
#     res = ct.c_bool()
#     res = lib.verifyPrivPubKeypair(wif_privkey_ptr, p2pkh_pubkey_ptr, ct.c_int(chain_code))

#     # stop context
#     lib.dogecoin_ecc_stop()

#     # return boolean result
#     return res


# def verify_master_priv_pub_keypair(wif_privkey_master, p2pkh_pubkey_master, chain_code=0):
#     """Given a keypair from generate_hd_master_pub_key_pair, verify that the
#     keys are valid and are associated with each other.
#     Keyword arguments:
#     wif_privkey_master -- string containing wif-encoded private master key
#     p2pkh_pubkey_master -- string containing address derived from wif_privkey
#     chain_code -- 0 for mainnet, 1 for testnet
#     """
#     # verify arguments are valid
#     assert isinstance(wif_privkey_master, str) and isinstance(p2pkh_pubkey_master, str)
#     assert isinstance(chain_code, int)

#     # prepare arguments
#     wif_privkey_master_ptr = ct.c_char_p(wif_privkey_master.encode('utf-8'))
#     p2pkh_pubkey_master_ptr = ct.c_char_p(p2pkh_pubkey_master.encode('utf-8'))

#     # start context
#     lib.dogecoin_ecc_start()

#     # call c function
#     res = ct.c_bool()
#     res = lib.verifyHDMasterPubKeypair(wif_privkey_master_ptr, p2pkh_pubkey_master_ptr, ct.c_bool(chain_code))

#     # stop context
#     lib.dogecoin_ecc_stop()

#     # return boolean result
#     return res


# def verify_p2pkh_address(p2pkh_pubkey, chain_code=0):
#     """Given a p2pkh address, confirm address is in correct format and
#     passes simple Shannon entropy threshold
#     Keyword arguments:
#     p2pkh_pubkey -- string containing basic p2pkh address
#     chain_code -- 0 for mainnet, 1 for testnet
#     """
#     # verify arguments are valid
#     assert isinstance(p2pkh_pubkey, str) and isinstance(chain_code, int)

#     # prepare arguments
#     p2pkh_pubkey_ptr = ct.c_char_p(p2pkh_pubkey.encode('utf-8'))
#     strlen = ct.c_ubyte(len(p2pkh_pubkey))

#     # start context
#     lib.dogecoin_ecc_start()

#     # call c function
#     res = ct.c_bool()
#     res = lib.verifyP2pkhAddress(p2pkh_pubkey_ptr, strlen, ct.c_int(chain_code))

#     # stop context
#     lib.dogecoin_ecc_stop()

#     # return boolean result
#     return res


# class DogecoinKey(ct.Structure):
#     """Class to mimic dogecoin_key struct from libdogecoin"""
#     _fields_ = [
#         ("privkey",                     ct.c_ubyte * 32),
#     ]

# class DogecoinPubkey(ct.Structure):
#     """Class to mimic dogecoin_pubkey struct from libdogecoin"""
#     _fields_ = [
#         ("compressed",                  ct.c_ubyte),
#         ("pubkey",                      ct.c_ubyte * 65),
#     ]