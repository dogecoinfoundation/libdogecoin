/* libdogecoin.h v0.1 - (c)2022 The Dogecoin Foundation */
/* need licensestatement here */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* basic address functions: return 1 if succesful 
   ----------------------------------------------
*///!init static ecc context
void dogecoin_ecc_start(void);

//!destroys the static ecc context
void dogecoin_ecc_stop(void);

/* generates a private and public keypair (a wallet import format private key and a p2pkh ready-to-use corresponding dogecoin address)*/
int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* generates a hybrid deterministic WIF master key and p2pkh ready-to-use corresponding dogecoin address. */
int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* generates a new dogecoin address from a HD master key */
int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey);

/* verify that a private key and dogecoin address match */
int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* verify that a HD Master key and a dogecoin address matches */
int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* verify that a dogecoin address is valid. */
int verifyP2pkhAddress(char* p2pkh_pubkey, uint8_t len);


/*transaction creation functions - builds a dogecoin transaction
----------------------------------------------------------------
*/

/* create a new dogecoin transaction: Returns the (txindex) in memory of the transaction being worked on. */
int start_transaction();

/* add a utxo to the transaction being worked on at (txindex), specifying the utxo's txid and vout. returns 1 if successful.*/
int add_utxo(int txindex, char* hex_utxo_txid, int vout);

/* add an output to the transaction being worked on at (txindex) of amount (amount) in dogecoins, returns 1 if successful. */
int add_output(int txindex, char* destinationaddress, long double amount);

/* finalize the transaction being worked on at (txindex), with the (destinationaddress) paying a fee of (subtractedfee), */
/* re-specify the amount in dogecoin for verification, and change address for change. If not specified, change will go to the first utxo's address. */
char* finalize_transaction(int txindex, char* destinationaddress, double subtractedfee, uint64_t out_dogeamount_for_verification, char* changeaddress);

/* sign a raw transaction in memory at (txindex), sign (inputindex) with (scripthex) of (sighashtype) for (amount - koinu? dogecoin? why a 3rd time), with (privkey) */
int sign_transaction(int txindex, long double amounts[], char* script_pubkey, char* privkey);

/* clear all internal working transactions */
void remove_all();

/* retrieve the raw transaction at (txindex) as a hex string (char*) */
char* get_raw_transaction(int txindex);

/* clear the transaction at (txindex) in memory */
void clear_transaction(int txindex);

/* Advanced API functions for operating on already formed raw transactions
--------------------------------------------------------------------------
*/

/*Sign a raw transaction hexadecimal string using inputindex, scripthex, sighashtype, amount and privkey. */
int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, long double amount, char* privkey);

/*Store a raw transaction that's already formed, and give it a txindex in memory. (txindex) is returned as int. */
int store_raw_transaction(char* incomingrawtx);
