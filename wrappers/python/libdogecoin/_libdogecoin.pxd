cimport cython
from libc.stdint cimport uint32_t, uint8_t

from . cimport types

cdef extern from "libdogecoin.h":
    # ecc.c
    void dogecoin_ecc_start()
    void dogecoin_ecc_stop()

    # address.c
    int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bint is_testnet)
    int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bint is_testnet)
    int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey)
    int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bint is_testnet)
    int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bint is_testnet)
    int verifyP2pkhAddress(char* p2pkh_pubkey, cython.size_t len)

    # transaction.c
    int start_transaction()
    int add_utxo(int txindex, char* hex_utxo_txid, int vout)
    int add_output(int txindex, char* destinationaddress, char* amount)
    char* finalize_transaction(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress)
    char* get_raw_transaction(int txindex) 
    void clear_transaction(int txindex)
    int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey)
    int sign_transaction(int txindex, char* script_pubkey, char* privkey)
    int store_raw_transaction(char* incomingrawtx)
    # bip39
    int generateRandomEnglishMnemonic(const types.ENTROPY_SIZE size, types.MNEMONIC mnemonic)

    int generateEnglishMnemonic(const types.HEX_ENTROPY entropy, const types.ENTROPY_SIZE size, types.MNEMONIC mnemonic)

    int dogecoin_seed_from_mnemonic(const types.MNEMONIC mnemonic, const types.PASS passphrase, types.SEED seed)
    
    int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const types.CHANGE_LEVEL change_level, const types.MNEMONIC mnemonic, const types.PASS passphrase, char* p2pkh_pubkey, const int is_testnet)

    # qrengine.h

